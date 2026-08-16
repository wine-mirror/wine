/*
 * xboxkrnl.exe - miscellaneous forwards: thread control, contiguous memory,
 * Rtl* string/arithmetic, format-string helpers, Ex raise, IO manager wrappers.
 *
 * Signatures and semantics from Cxbx-Reloaded src/core/kernel/exports/ and
 * src/core/kernel/common/*.h.
 */

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

/* ntdll exports not declared in winternl.h */
NTSYSAPI NTSTATUS WINAPI NtQueueApcThread(HANDLE,PNTAPCFUNC,ULONG_PTR,ULONG_PTR,ULONG_PTR);
NTSYSAPI void     WINAPI RtlRaiseException(EXCEPTION_RECORD*);
NTSYSAPI NTSTATUS WINAPI NtSetSystemTime(const LARGE_INTEGER*,LARGE_INTEGER*);
NTSYSAPI NTSTATUS WINAPI NtCreateSymbolicLinkObject(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PUNICODE_STRING);
NTSYSAPI NTSTATUS WINAPI NtMakeTemporaryObject(HANDLE);

#ifndef SYMBOLIC_LINK_ALL_ACCESS
#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x0001)
#endif

/* ======================================================================
 * Ke* thread control
 *
 * Xbox PKTHREAD values: for threads created via PsCreateSystemThread (which
 * we implement via RtlCreateUserThread and return real HANDLEs), the thread
 * "pointer" is the HANDLE itself.  For KeGetCurrentThread() we return the
 * current-thread pseudo-handle so KeWaitForSingleObject(KeGetCurrentThread())
 * and KeSuspendThread(KeGetCurrentThread()) work.
 * ====================================================================== */
HANDLE WINAPI XBOXKRNL_KeGetCurrentThread(void)
{
    return NtCurrentThread();
}

ULONG WINAPI XBOXKRNL_KeResumeThread(void *Thread)
{
    ULONG prev = 0;
    NtResumeThread((HANDLE)Thread, &prev);
    return prev;
}

ULONG WINAPI XBOXKRNL_KeSuspendThread(void *Thread)
{
    ULONG prev = 0;
    NtSuspendThread((HANDLE)Thread, &prev);
    return prev;
}

/* KeSetBasePriorityThread: increment relative to current process priority.
 * Desktop NT ThreadBasePriority info class sets relative priority. */
LONG WINAPI XBOXKRNL_KeSetBasePriorityThread(void *Thread, LONG Increment)
{
    LONG prev = 0;
    NtSetInformationThread((HANDLE)Thread, ThreadBasePriority, &Increment, sizeof(Increment));
    return prev;
}

LONG WINAPI XBOXKRNL_KeSetPriorityThread(void *Thread, LONG Priority)
{
    LONG prev = 0;
    NtSetInformationThread((HANDLE)Thread, ThreadBasePriority, &Priority, sizeof(Priority));
    return prev;
}

LONG WINAPI XBOXKRNL_KeSetPriorityProcess(void *Process, LONG Priority)
{
    /* Xbox has a single process; map to current process priority class via
     * NtSetInformationProcess with ProcessBasePriority. */
    NtSetInformationProcess(NtCurrentProcess(), ProcessBasePriority, &Priority, sizeof(Priority));
    return 0;
}

/* KeQueryBasePriorityThread - return the thread's base priority */
LONG WINAPI XBOXKRNL_KeQueryBasePriorityThread(void *Thread)
{
    THREAD_BASIC_INFORMATION info;
    if (NT_SUCCESS(NtQueryInformationThread((HANDLE)Thread, ThreadBasicInformation,
                                             &info, sizeof(info), NULL)))
        return info.Priority;
    return 0;
}

/* KeQueryInterruptTime - return current interrupt time (100-ns units) */
ULONGLONG WINAPI XBOXKRNL_KeQueryInterruptTime(void)
{
    LARGE_INTEGER t;
    NtQuerySystemTime(&t);
    return t.QuadPart;
}

/* KeStallExecutionProcessor - busy-wait for microseconds */
void WINAPI XBOXKRNL_KeStallExecutionProcessor(ULONG MicroSeconds)
{
    /* No kernel-level stall available; use a delay with 100-ns units */
    LARGE_INTEGER interval;
    interval.QuadPart = -(LONGLONG)MicroSeconds * 10;  /* relative, negative = relative */
    NtDelayExecution(FALSE, &interval);
}

/* KeAlertThread/KeAlertResumeThread - alert a waiting thread */
NTSTATUS WINAPI XBOXKRNL_KeAlertThread(void *Thread)
{
    return NtAlertThread((HANDLE)Thread);
}

NTSTATUS WINAPI XBOXKRNL_KeAlertResumeThread(void *Thread, void *PreviousSuspendCount)
{
    return NtAlertResumeThread((HANDLE)Thread, (PULONG)PreviousSuspendCount);
}

/* ======================================================================
 * Mm* contiguous memory
 *
 * Xbox "contiguous" physical memory == virtually contiguous memory in our
 * usermode context; VirtualAlloc is the honest equivalent.
 * ====================================================================== */
void * WINAPI XBOXKRNL_MmAllocateContiguousMemory(ULONG NumberOfBytes)
{
    return VirtualAlloc(NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void * WINAPI XBOXKRNL_MmAllocateContiguousMemoryEx(ULONG NumberOfBytes,
                                                      ULONG LowestAcceptableAddress,
                                                      ULONG HighestAcceptableAddress,
                                                      ULONG Alignment,
                                                      ULONG Protect)
{
    /* Alignment and address range constraints are hardware-only; ignore. */
    return VirtualAlloc(NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE,
                        Protect ? Protect : PAGE_READWRITE);
}

void WINAPI XBOXKRNL_MmFreeContiguousMemory(void *BaseAddress)
{
    VirtualFree(BaseAddress, 0, MEM_RELEASE);
}

ULONG WINAPI XBOXKRNL_MmGetPhysicalAddress(void *BaseAddress)
{
    /* In usermode we cannot determine the physical address; return the
     * virtual address as-is (same on Xbox since physical=virtual for the
     * flat memory map it used). */
    return (ULONG)(ULONG_PTR)BaseAddress;
}

/* MmCreateKernelStack/MmDeleteKernelStack - allocate a kernel stack for a
 * thread.  In our usermode context, just do VirtualAlloc. */
void * WINAPI XBOXKRNL_MmCreateKernelStack(ULONG NumberOfBytes, ULONG Unknown)
{
    return VirtualAlloc(NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void WINAPI XBOXKRNL_MmDeleteKernelStack(void *EndAddress, void *BaseAddress)
{
    VirtualFree(BaseAddress, 0, MEM_RELEASE);
}

/* MmQueryStatistics - return zeroed stats (no real pool tracking) */
NTSTATUS WINAPI XBOXKRNL_MmQueryStatistics(void *MemoryStatistics)
{
    FIXME("ordinal 181 (MmQueryStatistics): returning zeros\n");
    if (MemoryStatistics) memset(MemoryStatistics, 0, 64);  /* struct is ~64 bytes */
    return STATUS_SUCCESS;
}

/* ======================================================================
 * Ex* raise / status forwards
 * ====================================================================== */
void WINAPI XBOXKRNL_ExRaiseException(EXCEPTION_RECORD *ExceptionRecord)
{
    RtlRaiseException(ExceptionRecord);
}

void WINAPI XBOXKRNL_ExRaiseStatus(NTSTATUS Status)
{
    EXCEPTION_RECORD rec = {0};
    rec.ExceptionCode = Status;
    RtlRaiseException(&rec);
}

/* ======================================================================
 * Rtl* raise forwards
 * ====================================================================== */
void WINAPI XBOXKRNL_RtlRaiseException(EXCEPTION_RECORD *ExceptionRecord)
{
    RtlRaiseException(ExceptionRecord);
}

void WINAPI XBOXKRNL_RtlRaiseStatus(NTSTATUS Status)
{
    EXCEPTION_RECORD rec = {0};
    rec.ExceptionCode = Status;
    RtlRaiseException(&rec);
}

/* ======================================================================
 * Rtl* character/string helpers
 * ====================================================================== */
CHAR WINAPI XBOXKRNL_RtlLowerChar(CHAR Character)
{
    return (CHAR)tolower((unsigned char)Character);
}

CHAR WINAPI XBOXKRNL_RtlUpperChar(CHAR Character)
{
    return (CHAR)toupper((unsigned char)Character);
}

void WINAPI XBOXKRNL_RtlUpperString(STRING *DestinationString, const STRING *SourceString)
{
    USHORT i, len = min(DestinationString->MaximumLength, SourceString->Length);
    for (i = 0; i < len; i++)
        DestinationString->Buffer[i] = (CHAR)toupper((unsigned char)SourceString->Buffer[i]);
    DestinationString->Length = len;
}

/* RtlMapGenericMask - map generic access bits through a GenericMapping */
void WINAPI XBOXKRNL_RtlMapGenericMask(ACCESS_MASK *AccessMask,
                                         GENERIC_MAPPING *GenericMapping)
{
    if (*AccessMask & GENERIC_READ)    *AccessMask |= GenericMapping->GenericRead;
    if (*AccessMask & GENERIC_WRITE)   *AccessMask |= GenericMapping->GenericWrite;
    if (*AccessMask & GENERIC_EXECUTE) *AccessMask |= GenericMapping->GenericExecute;
    if (*AccessMask & GENERIC_ALL)     *AccessMask |= GenericMapping->GenericAll;
    *AccessMask &= ~(GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL);
}

/* ======================================================================
 * Rtl* 64-bit magic divide (fixed-point reciprocal multiply)
 *
 * RtlExtendedMagicDivide(Dividend, MagicDivisor, ShiftCount):
 *   result = (Dividend * MagicDivisor) >> (64 + ShiftCount)
 * Used by Xbox kernel for timer calculations.
 * ====================================================================== */
/* Upper 64 bits of unsigned 64×64 → 128 product; portable, no __int128 needed */
static ULONGLONG mulhi64(ULONGLONG a, ULONGLONG b)
{
    ULONG al = (ULONG)a, ah = (ULONG)(a >> 32);
    ULONG bl = (ULONG)b, bh = (ULONG)(b >> 32);
    ULONGLONG lolo = (ULONGLONG)al * bl;
    ULONGLONG lohi = (ULONGLONG)al * bh;
    ULONGLONG hilo = (ULONGLONG)ah * bl;
    ULONGLONG hihi = (ULONGLONG)ah * bh;
    ULONGLONG cross = (lolo >> 32) + (lohi & 0xFFFFFFFFUL) + (hilo & 0xFFFFFFFFUL);
    return hihi + (lohi >> 32) + (hilo >> 32) + (cross >> 32);
}

INT64 WINAPI XBOXKRNL_RtlExtendedMagicDivide(INT64 Dividend, INT64 MagicDivisor, CCHAR ShiftCount)
{
    /* result = (Dividend * MagicDivisor) >> (64 + ShiftCount) */
    return (INT64)(mulhi64((ULONGLONG)Dividend, (ULONGLONG)MagicDivisor) >> ShiftCount);
}

/* RtlMultiByteToUnicodeSize / RtlUnicodeToMultiByteSize */
NTSTATUS WINAPI XBOXKRNL_RtlMultiByteToUnicodeSize(ULONG *BytesInUnicodeString,
                                                     const CHAR *MultiByteString,
                                                     ULONG BytesInMultiByteString)
{
    return RtlMultiByteToUnicodeSize(BytesInUnicodeString, MultiByteString,
                                      BytesInMultiByteString);
}

NTSTATUS WINAPI XBOXKRNL_RtlUnicodeToMultiByteSize(ULONG *BytesInMultiByteString,
                                                     const WCHAR *UnicodeString,
                                                     ULONG BytesInUnicodeString)
{
    return RtlUnicodeToMultiByteSize(BytesInMultiByteString, UnicodeString,
                                      BytesInUnicodeString);
}

NTSTATUS WINAPI XBOXKRNL_RtlUpcaseUnicodeToMultiByteN(CHAR *MultiByteString,
                                                         ULONG MaxBytesInMultiByteString,
                                                         ULONG *BytesInMultiByteString,
                                                         const WCHAR *UnicodeString,
                                                         ULONG BytesInUnicodeString)
{
    return RtlUpcaseUnicodeToMultiByteN(MultiByteString, MaxBytesInMultiByteString,
                                         BytesInMultiByteString, UnicodeString,
                                         BytesInUnicodeString);
}

/* ======================================================================
 * Rtl* format-string helpers (Xbox-specific C89-compatible vprintf wrappers)
 * ====================================================================== */
LONG WINAPIV XBOXKRNL_RtlSprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    LONG ret;
    va_start(args, fmt);
    ret = vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}

LONG WINAPI XBOXKRNL_RtlVsprintf(char *buf, const char *fmt, va_list args)
{
    return vsprintf(buf, fmt, args);
}

LONG WINAPIV XBOXKRNL_RtlSnprintf(char *buf, ULONG n, const char *fmt, ...)
{
    va_list args;
    LONG ret;
    va_start(args, fmt);
    ret = vsnprintf(buf, n, fmt, args);
    va_end(args);
    return ret;
}

LONG WINAPI XBOXKRNL_RtlVsnprintf(char *buf, ULONG n, const char *fmt, va_list args)
{
    return vsnprintf(buf, n, fmt, args);
}

/* ======================================================================
 * Nt* miscellaneous
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_NtQueueApcThread(HANDLE thread, PNTAPCFUNC apc,
                                            ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3)
{
    return NtQueueApcThread(thread, apc, arg1, arg2, arg3);
}

NTSTATUS WINAPI XBOXKRNL_NtSetSystemTime(const LARGE_INTEGER *new_time, LARGE_INTEGER *old_time)
{
    return NtSetSystemTime(new_time, old_time);
}

/* ======================================================================
 * Io* higher-level wrappers
 *
 * IoCreateFile is the Xbox I/O manager's public entry point that games
 * use directly; underneath it calls NtCreateFile after setting some IRP
 * flags from the Options bitmask.  We forward to NtCreateFile and ignore
 * the Options flags (which only matter to in-kernel device drivers).
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_IoCreateFile(HANDLE *handle, ACCESS_MASK access,
                                        OBJECT_ATTRIBUTES *attr, IO_STATUS_BLOCK *io,
                                        LARGE_INTEGER *alloc, ULONG attribs, ULONG sharing,
                                        ULONG disposition, ULONG options, ULONG io_options)
{
    return NtCreateFile(handle, access, attr, io, alloc, attribs, sharing,
                        disposition, options, NULL, 0);
}

NTSTATUS WINAPI XBOXKRNL_IoCreateSymbolicLink(UNICODE_STRING *link_name,
                                                UNICODE_STRING *device_name)
{
    OBJECT_ATTRIBUTES attr;
    HANDLE h = NULL;
    NTSTATUS status;
    InitializeObjectAttributes(&attr, link_name, OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL, NULL);
    status = NtCreateSymbolicLinkObject(&h, SYMBOLIC_LINK_ALL_ACCESS, &attr, device_name);
    if (NT_SUCCESS(status)) NtClose(h);
    return status;
}

NTSTATUS WINAPI XBOXKRNL_IoDeleteSymbolicLink(UNICODE_STRING *link_name)
{
    OBJECT_ATTRIBUTES attr;
    HANDLE h = NULL;
    NTSTATUS status;
    InitializeObjectAttributes(&attr, link_name, OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = NtOpenSymbolicLinkObject(&h, DELETE, &attr);
    if (NT_SUCCESS(status)) {
        NtMakeTemporaryObject(h);
        NtClose(h);
    }
    return status;
}

/* NtQueryDirectoryObject - Xbox omits ReturnSingleEntry (always TRUE), 6-param form */
NTSTATUS WINAPI XBOXKRNL_NtQueryDirectoryObject(HANDLE handle, void *info, ULONG len,
                                                  BOOLEAN restart, ULONG *context, ULONG *returned)
{
    return NtQueryDirectoryObject(handle, info, len, TRUE, restart, context, returned);
}

/* PsQueryStatistics / PsSetCreateThreadNotifyRoutine - safe stubs */
NTSTATUS WINAPI XBOXKRNL_PsQueryStatistics(void *ProcessStatistics)
{
    FIXME("PsQueryStatistics: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI XBOXKRNL_PsSetCreateThreadNotifyRoutine(void *NotifyRoutine)
{
    FIXME("PsSetCreateThreadNotifyRoutine: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}
