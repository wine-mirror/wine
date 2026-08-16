/*
 * xboxkrnl.exe - synchronization, file I/O and thread forwards.
 *
 * Implements the subset of xboxkrnl.exe ordinals that have a verifiable
 * near-1:1 mapping onto existing ntdll/kernel32 NT primitives:
 *
 *   - Ke* event/mutant/semaphore/timer: Xbox "dispatcher objects" are
 *     caller-allocated structs.  We create a real NT kernel object for each
 *     one and store it in a compact {ptr -> HANDLE} open-addressing hash
 *     table so every subsequent Ke* call can look up the backing handle.
 *   - Nt* file I/O: forwarded to ntdll's NtCreateFile / NtReadFile / ...
 *     with minor Xbox-specific parameter differences adjusted (missing
 *     EaBuffer/EaLength on NtCreateFile, missing Key on NtReadFile/
 *     NtWriteFile, missing ACCESS_MASK on NtCreateDirectoryObject, etc.).
 *   - Ps* thread: forwarded to RtlCreateUserThread / NtTerminateThread.
 *   - Rtl* string/time helpers: direct forwards.
 *   - KeBugCheck[Ex]: terminate the process.
 *
 * Signatures come from Cxbx-Reloaded KernelThunk.cpp + src/core/kernel/.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "ntdef.h"
#include "wine/debug.h"

#ifndef DIRECTORY_ALL_ACCESS
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)
#endif
#ifndef SYMBOLIC_LINK_QUERY
#define SYMBOLIC_LINK_QUERY 0x0001
#endif

typedef struct { LONG Depth; } IO_COMPLETION_BASIC_INFORMATION;

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

/* Forward declarations for ntdll functions not in winternl.h */
NTSYSAPI NTSTATUS WINAPI NtQuerySymbolicLinkObject(HANDLE,UNICODE_STRING*,ULONG*);
NTSYSAPI NTSTATUS WINAPI NtOpenDirectoryObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);
NTSYSAPI NTSTATUS WINAPI NtOpenSymbolicLinkObject(PHANDLE,ACCESS_MASK,const OBJECT_ATTRIBUTES*);

/* ======================================================================
 * Internal {struct-ptr → HANDLE} table for Ke* dispatcher objects.
 *
 * Xbox kernel "dispatcher objects" (KEVENT, KMUTANT, KSEMAPHORE, KTIMER)
 * are caller-allocated structs.  In our usermode Wine context we create a
 * real NT object for each and look up its HANDLE by the struct pointer.
 * Open-addressing hash table; 2048 slots is more than any Xbox title needs.
 * ====================================================================== */
#define XBOX_OBJ_MAX 2048

static struct { void *ptr; HANDLE handle; } xbox_obj_table[XBOX_OBJ_MAX];
RTL_CRITICAL_SECTION xbox_obj_cs;

static HANDLE xbox_obj_lookup(void *ptr)
{
    ULONG h = (ULONG)((ULONG_PTR)ptr >> 4) % XBOX_OBJ_MAX;
    ULONG i;
    for (i = 0; i < XBOX_OBJ_MAX; i++) {
        ULONG idx = (h + i) % XBOX_OBJ_MAX;
        if (xbox_obj_table[idx].ptr == ptr) return xbox_obj_table[idx].handle;
        if (!xbox_obj_table[idx].ptr) return NULL;
    }
    return NULL;
}

static void xbox_obj_insert(void *ptr, HANDLE handle)
{
    ULONG h = (ULONG)((ULONG_PTR)ptr >> 4) % XBOX_OBJ_MAX;
    ULONG i;
    for (i = 0; i < XBOX_OBJ_MAX; i++) {
        ULONG idx = (h + i) % XBOX_OBJ_MAX;
        if (!xbox_obj_table[idx].ptr || xbox_obj_table[idx].ptr == ptr) {
            xbox_obj_table[idx].ptr    = ptr;
            xbox_obj_table[idx].handle = handle;
            return;
        }
    }
    ERR("xbox_obj_table full – ptr=%p\n", ptr);
}

static void xbox_obj_remove(void *ptr)
{
    ULONG h = (ULONG)((ULONG_PTR)ptr >> 4) % XBOX_OBJ_MAX;
    ULONG i;
    for (i = 0; i < XBOX_OBJ_MAX; i++) {
        ULONG idx = (h + i) % XBOX_OBJ_MAX;
        if (xbox_obj_table[idx].ptr == ptr) {
            xbox_obj_table[idx].ptr    = NULL;
            xbox_obj_table[idx].handle = NULL;
            return;
        }
        if (!xbox_obj_table[idx].ptr) return;
    }
}

/* initialise CS once at DLL attach; called from DllMain in main.c */
void xbox_sync_init(void)
{
    RtlInitializeCriticalSection(&xbox_obj_cs);
}

void xbox_sync_fini(void)
{
    RtlDeleteCriticalSection(&xbox_obj_cs);
}

/* ======================================================================
 * Ke* event
 * Xbox EventType: 0=NotificationEvent, 1=SynchronizationEvent
 * ====================================================================== */
void WINAPI XBOXKRNL_KeInitializeEvent(void *Event, LONG EventType, LONG InitialState)
{
    HANDLE h = NULL;
    EVENT_TYPE et = EventType ? SynchronizationEvent : NotificationEvent;
    NtCreateEvent(&h, EVENT_ALL_ACCESS, NULL, et, (BOOLEAN)InitialState);
    RtlEnterCriticalSection(&xbox_obj_cs);
    xbox_obj_insert(Event, h);
    RtlLeaveCriticalSection(&xbox_obj_cs);
}

LONG WINAPI XBOXKRNL_KeSetEvent(void *Event, LONG Increment, LONG Wait)
{
    LONG prev = 0;
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Event);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtSetEvent(h, &prev);
    return prev;
}

LONG WINAPI XBOXKRNL_KeResetEvent(void *Event)
{
    LONG prev = 0;
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Event);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtResetEvent(h, &prev);
    return prev;
}

LONG WINAPI XBOXKRNL_KePulseEvent(void *Event, LONG Increment, LONG Wait)
{
    LONG prev = 0;
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Event);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtPulseEvent(h, &prev);
    return prev;
}

/* ======================================================================
 * Ke* mutant (kernel mutex)
 * ====================================================================== */
void WINAPI XBOXKRNL_KeInitializeMutant(void *Mutant, LONG InitialOwner)
{
    HANDLE h = NULL;
    NtCreateMutant(&h, MUTANT_ALL_ACCESS, NULL, (BOOLEAN)InitialOwner);
    RtlEnterCriticalSection(&xbox_obj_cs);
    xbox_obj_insert(Mutant, h);
    RtlLeaveCriticalSection(&xbox_obj_cs);
}

LONG WINAPI XBOXKRNL_KeReleaseMutant(void *Mutant, LONG Increment, LONG Abandoned, LONG Wait)
{
    LONG prev = 0;
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Mutant);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtReleaseMutant(h, &prev);
    return prev;
}

/* ======================================================================
 * Ke* semaphore
 * ====================================================================== */
void WINAPI XBOXKRNL_KeInitializeSemaphore(void *Semaphore, LONG Count, LONG Limit)
{
    HANDLE h = NULL;
    NtCreateSemaphore(&h, SEMAPHORE_ALL_ACCESS, NULL, Count, Limit);
    RtlEnterCriticalSection(&xbox_obj_cs);
    xbox_obj_insert(Semaphore, h);
    RtlLeaveCriticalSection(&xbox_obj_cs);
}

LONG WINAPI XBOXKRNL_KeReleaseSemaphore(void *Semaphore, LONG Increment, LONG Adjustment, LONG Wait)
{
    LONG prev = 0;
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Semaphore);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtReleaseSemaphore(h, Adjustment, (PULONG)&prev);
    return prev;
}

/* ======================================================================
 * Ke* timer
 * Xbox TimerType: 0=NotificationTimer, 1=SynchronizationTimer
 * ====================================================================== */
void WINAPI XBOXKRNL_KeInitializeTimerEx(void *Timer, LONG TimerType)
{
    HANDLE h = NULL;
    NtCreateTimer(&h, TIMER_ALL_ACCESS, NULL, (TIMER_TYPE)TimerType);
    RtlEnterCriticalSection(&xbox_obj_cs);
    xbox_obj_insert(Timer, h);
    RtlLeaveCriticalSection(&xbox_obj_cs);
}

LONG WINAPI XBOXKRNL_KeSetTimer(void *Timer, INT64 DueTime, void *Dpc)
{
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Timer);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (!h) return FALSE;
    return NT_SUCCESS(NtSetTimer(h, (const LARGE_INTEGER *)&DueTime, NULL, NULL, FALSE, 0, NULL));
}

LONG WINAPI XBOXKRNL_KeSetTimerEx(void *Timer, INT64 DueTime, LONG Period, void *Dpc)
{
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Timer);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (!h) return FALSE;
    return NT_SUCCESS(NtSetTimer(h, (const LARGE_INTEGER *)&DueTime, NULL, NULL, FALSE, Period, NULL));
}

LONG WINAPI XBOXKRNL_KeCancelTimer(void *Timer)
{
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Timer);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (!h) return FALSE;
    return NT_SUCCESS(NtCancelTimer(h, NULL));
}

/* ======================================================================
 * Ke* wait
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_KeWaitForSingleObject(void *Object, LONG WaitReason, LONG WaitMode,
                                                 LONG Alertable, LARGE_INTEGER *Timeout)
{
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Object);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (!h) return STATUS_INVALID_HANDLE;
    return NtWaitForSingleObject(h, (BOOLEAN)Alertable, Timeout);
}

NTSTATUS WINAPI XBOXKRNL_KeWaitForMultipleObjects(LONG Count, void **Object, LONG WaitType,
                                                    LONG WaitReason, LONG WaitMode, LONG Alertable,
                                                    LARGE_INTEGER *Timeout, void *WaitBlockArray)
{
    HANDLE handles[64];
    LONG i;
    if (Count < 0 || Count > 64) return STATUS_INVALID_PARAMETER;
    RtlEnterCriticalSection(&xbox_obj_cs);
    for (i = 0; i < Count; i++) handles[i] = xbox_obj_lookup(Object[i]);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    return NtWaitForMultipleObjects((ULONG)Count, handles,
                                    (WAIT_TYPE)WaitType, (BOOLEAN)Alertable, Timeout);
}

/* ======================================================================
 * Ke* bug check (abnormal termination)
 * ====================================================================== */
void WINAPI XBOXKRNL_KeBugCheck(LONG BugCheckCode)
{
    ERR("KeBugCheck code=%08lx\n", BugCheckCode);
    TerminateProcess(GetCurrentProcess(), (UINT)BugCheckCode);
}

LONG WINAPI XBOXKRNL_KeBugCheckEx(LONG Code, void *P1, void *P2, void *P3, void *P4)
{
    ERR("KeBugCheckEx code=%08lx p1=%p p2=%p p3=%p p4=%p\n", Code, P1, P2, P3, P4);
    TerminateProcess(GetCurrentProcess(), (UINT)Code);
    return 0;
}

/* ======================================================================
 * Nt* file I/O forwards
 * ====================================================================== */

/* Xbox omits EaBuffer/EaLength (last 2 params of desktop NT) */
NTSTATUS WINAPI XBOXKRNL_NtCreateFile(HANDLE *handle, ACCESS_MASK access,
                                        OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io,
                                        LARGE_INTEGER *alloc, ULONG attribs, ULONG sharing,
                                        ULONG disposition, ULONG options)
{
    return NtCreateFile(handle, access, attr, io, alloc, attribs, sharing,
                        disposition, options, NULL, 0);
}

NTSTATUS WINAPI XBOXKRNL_NtOpenFile(HANDLE *handle, ACCESS_MASK access,
                                      OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io,
                                      ULONG sharing, ULONG options)
{
    return NtOpenFile(handle, access, attr, io, sharing, options);
}

/* Xbox omits Key (last param of desktop NT NtReadFile/NtWriteFile) */
NTSTATUS WINAPI XBOXKRNL_NtReadFile(HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                      void *apc_ctx, IO_STATUS_BLOCK *io,
                                      void *buf, ULONG len, LARGE_INTEGER *offset)
{
    return NtReadFile(file, event, apc, apc_ctx, io, buf, len, offset, NULL);
}

NTSTATUS WINAPI XBOXKRNL_NtWriteFile(HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                       void *apc_ctx, IO_STATUS_BLOCK *io,
                                       const void *buf, ULONG len, LARGE_INTEGER *offset)
{
    return NtWriteFile(file, event, apc, apc_ctx, io, buf, len, offset, NULL);
}

NTSTATUS WINAPI XBOXKRNL_NtReadFileScatter(HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                             void *apc_ctx, IO_STATUS_BLOCK *io,
                                             FILE_SEGMENT_ELEMENT *segs, ULONG len,
                                             LARGE_INTEGER *offset)
{
    return NtReadFileScatter(file, event, apc, apc_ctx, io, segs, len, offset, NULL);
}

NTSTATUS WINAPI XBOXKRNL_NtWriteFileGather(HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                             void *apc_ctx, IO_STATUS_BLOCK *io,
                                             FILE_SEGMENT_ELEMENT *segs, ULONG len,
                                             LARGE_INTEGER *offset)
{
    return NtWriteFileGather(file, event, apc, apc_ctx, io, segs, len, offset, NULL);
}

NTSTATUS WINAPI XBOXKRNL_NtDeleteFile(OBJECT_ATTRIBUTES *attr)
{
    return NtDeleteFile(attr);
}

NTSTATUS WINAPI XBOXKRNL_NtFlushBuffersFile(HANDLE file, IO_STATUS_BLOCK *io)
{
    return NtFlushBuffersFile(file, io);
}

NTSTATUS WINAPI XBOXKRNL_NtQueryInformationFile(HANDLE file, IO_STATUS_BLOCK *io,
                                                   void *info, ULONG len,
                                                   FILE_INFORMATION_CLASS cls)
{
    return NtQueryInformationFile(file, io, info, len, cls);
}

NTSTATUS WINAPI XBOXKRNL_NtSetInformationFile(HANDLE file, IO_STATUS_BLOCK *io,
                                                void *info, ULONG len,
                                                FILE_INFORMATION_CLASS cls)
{
    return NtSetInformationFile(file, io, info, len, cls);
}

NTSTATUS WINAPI XBOXKRNL_NtQueryVolumeInformationFile(HANDLE file, IO_STATUS_BLOCK *io,
                                                         void *info, ULONG len,
                                                         FS_INFORMATION_CLASS cls)
{
    return NtQueryVolumeInformationFile(file, io, info, len, cls);
}

NTSTATUS WINAPI XBOXKRNL_NtQueryFullAttributesFile(const OBJECT_ATTRIBUTES *attr,
                                                     FILE_NETWORK_OPEN_INFORMATION *info)
{
    return NtQueryFullAttributesFile(attr, info);
}

/* Xbox drops ReturnSingleEntry param (between FileInformationClass and FileName) */
NTSTATUS WINAPI XBOXKRNL_NtQueryDirectoryFile(HANDLE file, HANDLE event,
                                                PIO_APC_ROUTINE apc, void *apc_ctx,
                                                IO_STATUS_BLOCK *io, void *info, ULONG len,
                                                FILE_INFORMATION_CLASS cls,
                                                UNICODE_STRING *mask, LONG restart)
{
    return NtQueryDirectoryFile(file, event, apc, apc_ctx, io, info, len, cls,
                                FALSE, mask, (BOOLEAN)restart);
}

NTSTATUS WINAPI XBOXKRNL_NtDeviceIoControlFile(HANDLE file, HANDLE event,
                                                  PIO_APC_ROUTINE apc, void *apc_ctx,
                                                  IO_STATUS_BLOCK *io, ULONG code,
                                                  void *in_buf, ULONG in_len,
                                                  void *out_buf, ULONG out_len)
{
    return NtDeviceIoControlFile(file, event, apc, apc_ctx, io, code,
                                  in_buf, in_len, out_buf, out_len);
}

NTSTATUS WINAPI XBOXKRNL_NtFsControlFile(HANDLE file, HANDLE event,
                                           PIO_APC_ROUTINE apc, void *apc_ctx,
                                           IO_STATUS_BLOCK *io, ULONG code,
                                           void *in_buf, ULONG in_len,
                                           void *out_buf, ULONG out_len)
{
    return NtFsControlFile(file, event, apc, apc_ctx, io, code,
                            in_buf, in_len, out_buf, out_len);
}

/* ======================================================================
 * Nt* directory/link – Xbox omits ACCESS_MASK
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_NtCreateDirectoryObject(HANDLE *handle, OBJECT_ATTRIBUTES *attr)
{
    return NtCreateDirectoryObject(handle, DIRECTORY_ALL_ACCESS, attr);
}

NTSTATUS WINAPI XBOXKRNL_NtOpenDirectoryObject(HANDLE *handle, OBJECT_ATTRIBUTES *attr)
{
    return NtOpenDirectoryObject(handle, DIRECTORY_ALL_ACCESS, attr);
}

NTSTATUS WINAPI XBOXKRNL_NtOpenSymbolicLinkObject(HANDLE *handle, OBJECT_ATTRIBUTES *attr)
{
    return NtOpenSymbolicLinkObject(handle, SYMBOLIC_LINK_QUERY, attr);
}

NTSTATUS WINAPI XBOXKRNL_NtQuerySymbolicLinkObject(HANDLE handle, UNICODE_STRING *target, ULONG *len)
{
    return NtQuerySymbolicLinkObject(handle, target, len);
}

/* ======================================================================
 * Nt* IO completion
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_NtCreateIoCompletion(HANDLE *handle, ULONG access,
                                                OBJECT_ATTRIBUTES *attr, ULONG threads)
{
    return NtCreateIoCompletion(handle, access, attr, threads);
}

NTSTATUS WINAPI XBOXKRNL_NtSetIoCompletion(HANDLE port, ULONG_PTR key, ULONG_PTR value,
                                             NTSTATUS status, SIZE_T info)
{
    return NtSetIoCompletion(port, key, value, status, info);
}

NTSTATUS WINAPI XBOXKRNL_NtRemoveIoCompletion(HANDLE port, ULONG_PTR *key, ULONG_PTR *value,
                                                IO_STATUS_BLOCK *io, LARGE_INTEGER *timeout)
{
    return NtRemoveIoCompletion(port, key, value, io, timeout);
}

/* Xbox queries IoCompletionBasicInformation only (2 params vs NT's 5) */
NTSTATUS WINAPI XBOXKRNL_NtQueryIoCompletion(HANDLE port, void *info)
{
    return NtQueryIoCompletion(port, IoCompletionBasicInformation, info,
                                sizeof(IO_COMPLETION_BASIC_INFORMATION), NULL);
}

/* ======================================================================
 * Nt* timer – Xbox NtCreateTimer omits ACCESS_MASK
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_NtCreateTimer(HANDLE *handle, OBJECT_ATTRIBUTES *attr, TIMER_TYPE type)
{
    return NtCreateTimer(handle, TIMER_ALL_ACCESS, attr, type);
}

NTSTATUS WINAPI XBOXKRNL_NtCancelTimer(HANDLE handle, BOOLEAN *current_state)
{
    return NtCancelTimer(handle, current_state);
}

/* Xbox always queries TimerBasicInformation (2 params vs NT's 5) */
NTSTATUS WINAPI XBOXKRNL_NtQueryTimer(HANDLE handle, TIMER_BASIC_INFORMATION *info)
{
    return NtQueryTimer(handle, TimerBasicInformation, info, sizeof(*info), NULL);
}

/* Xbox NtSetTimerEx (8 params): handle, type, due, period, apc, mode, resume, prev_state */
NTSTATUS WINAPI XBOXKRNL_NtSetTimerEx(HANDLE handle, void *type, LARGE_INTEGER *due,
                                        LONG period, PTIMER_APC_ROUTINE apc,
                                        LONG mode, LONG resume, BOOLEAN *prev_state)
{
    return NtSetTimer(handle, due, apc, NULL, (BOOLEAN)resume, period, prev_state);
}

/* ======================================================================
 * Nt* wait variants
 * ====================================================================== */

/* Xbox NtWaitForMultipleObjectsEx has extra WaitMode param (dropped here) */
NTSTATUS WINAPI XBOXKRNL_NtWaitForMultipleObjectsEx(ULONG count, HANDLE *handles,
                                                      LONG wait_type, LONG wait_mode,
                                                      LONG alertable, LARGE_INTEGER *timeout)
{
    return NtWaitForMultipleObjects(count, handles, (WAIT_TYPE)wait_type,
                                    (BOOLEAN)alertable, timeout);
}

/* Xbox NtSignalAndWaitForSingleObjectEx has extra WaitMode param (dropped here) */
NTSTATUS WINAPI XBOXKRNL_NtSignalAndWaitForSingleObjectEx(HANDLE signal, HANDLE wait,
                                                            LONG wait_mode, LONG alertable,
                                                            LARGE_INTEGER *timeout)
{
    return NtSignalAndWaitForSingleObject(signal, wait, (BOOLEAN)alertable, timeout);
}

/* ======================================================================
 * Ps* thread management
 * ====================================================================== */

/* Xbox PsCreateSystemThread(ThreadHandle, ThreadId, StartRoutine, Ctx, DebuggerThread) */
NTSTATUS WINAPI XBOXKRNL_PsCreateSystemThread(HANDLE *handle, ULONG *thread_id,
                                                PRTL_THREAD_START_ROUTINE start,
                                                void *ctx, LONG debugger_thread)
{
    CLIENT_ID cid = { 0 };
    NTSTATUS status = RtlCreateUserThread(NtCurrentProcess(), NULL, FALSE, 0, 0, 0,
                                           start, ctx, handle, &cid);
    if (NT_SUCCESS(status) && thread_id)
        *thread_id = HandleToUlong(cid.UniqueThread);
    return status;
}

NTSTATUS WINAPI XBOXKRNL_PsCreateSystemThreadEx(HANDLE *handle, ULONG kernel_stack_size,
                                                   ULONG tls_data_size, ULONG kernel_stack_size2,
                                                   ULONG *thread_id, void *start_ctx,
                                                   void *start_routine, LONG create_suspended,
                                                   LONG debug_stack, PRTL_THREAD_START_ROUTINE thunk)
{
    CLIENT_ID cid = { 0 };
    NTSTATUS status = RtlCreateUserThread(NtCurrentProcess(), NULL,
                                           (BOOLEAN)create_suspended, 0, 0, 0,
                                           thunk ? thunk : (PRTL_THREAD_START_ROUTINE)start_routine,
                                           start_ctx, handle, &cid);
    if (NT_SUCCESS(status) && thread_id)
        *thread_id = HandleToUlong(cid.UniqueThread);
    return status;
}

void WINAPI XBOXKRNL_PsTerminateSystemThread(NTSTATUS exit_status)
{
    NtTerminateThread(NULL, exit_status);
}

/* ======================================================================
 * Rtl* string helpers that just forward to ntdll
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_RtlAppendStringToString(STRING *dest, const STRING *src)
{
    return RtlAppendStringToString(dest, src);
}

NTSTATUS WINAPI XBOXKRNL_RtlAppendUnicodeStringToString(UNICODE_STRING *dest,
                                                           const UNICODE_STRING *src)
{
    return RtlAppendUnicodeStringToString(dest, src);
}

NTSTATUS WINAPI XBOXKRNL_RtlAppendUnicodeToString(UNICODE_STRING *dest, const WCHAR *src)
{
    return RtlAppendUnicodeToString(dest, src);
}

BOOLEAN WINAPI XBOXKRNL_RtlCreateUnicodeString(UNICODE_STRING *dest, const WCHAR *src)
{
    return RtlCreateUnicodeString(dest, src);
}

NTSTATUS WINAPI XBOXKRNL_RtlDowncaseUnicodeString(UNICODE_STRING *dest,
                                                     const UNICODE_STRING *src,
                                                     BOOLEAN alloc)
{
    return RtlDowncaseUnicodeString(dest, src, alloc);
}

BOOLEAN WINAPI XBOXKRNL_RtlTimeFieldsToTime(TIME_FIELDS *fields, LARGE_INTEGER *time)
{
    return RtlTimeFieldsToTime(fields, time);
}

void WINAPI XBOXKRNL_RtlTimeToTimeFields(const LARGE_INTEGER *time, TIME_FIELDS *fields)
{
    RtlTimeToTimeFields(time, fields);
}

/* ======================================================================
 * Rtl* 64-bit integer arithmetic (implemented directly)
 * ====================================================================== */
INT64 WINAPI XBOXKRNL_RtlExtendedIntegerMultiply(INT64 Multiplicand, LONG Multiplier)
{
    return Multiplicand * (INT64)Multiplier;
}

INT64 WINAPI XBOXKRNL_RtlExtendedLargeIntegerDivide(INT64 Dividend, ULONG Divisor, ULONG *Remainder)
{
    if (Remainder) *Remainder = (ULONG)((ULONGLONG)Dividend % Divisor);
    return (INT64)((ULONGLONG)Dividend / Divisor);
}

/* KeSetEventBoostPriority: signal event, ignore thread boost (usermode) */
void WINAPI XBOXKRNL_KeSetEventBoostPriority(void *Event, void **Thread)
{
    HANDLE h;
    RtlEnterCriticalSection(&xbox_obj_cs);
    h = xbox_obj_lookup(Event);
    RtlLeaveCriticalSection(&xbox_obj_cs);
    if (h) NtSetEvent(h, NULL);
}
