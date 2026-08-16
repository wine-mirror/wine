/*
 * xboxkrnl.exe - IRQL, Ex* interlocked/rwlock/nonvol, Ke* thread helpers,
 * Rtl* stack helpers, Mm* address/map, Dbg* and port-I/O no-ops.
 *
 * Semantics from Cxbx-Reloaded src/core/kernel/exports/ and
 * src/core/kernel/common/*.h.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

NTSYSAPI NTSTATUS WINAPI NtTestAlert(void);

/* ======================================================================
 * IRQL - in usermode always PASSIVE_LEVEL (0)
 * ====================================================================== */
LONG FASTCALL XBOXKRNL_KfRaiseIrql(LONG NewIrql) { return 0; }
void FASTCALL XBOXKRNL_KfLowerIrql(LONG OldIrql) { }
void FASTCALL XBOXKRNL_KiUnlockDispatcherDatabase(LONG OldIrql) { }
LONG WINAPI XBOXKRNL_KeGetCurrentIrql(void) { return 0; }
LONG WINAPI XBOXKRNL_KeRaiseIrqlToDpcLevel(void) { return 0; }
LONG WINAPI XBOXKRNL_KeRaiseIrqlToSynchLevel(void) { return 0; }

/* ======================================================================
 * Ke* thread helpers
 * ====================================================================== */
LONG WINAPI XBOXKRNL_KeBoostPriorityThread(void *Thread, LONG Increment)
{
    NtSetInformationThread((HANDLE)Thread, ThreadBasePriority, &Increment, sizeof(Increment));
    return 0;
}

void WINAPI XBOXKRNL_KeEnterCriticalRegion(LONG unused) { }
void WINAPI XBOXKRNL_KeLeaveCriticalRegion(LONG unused) { }
LONG WINAPI XBOXKRNL_KeIsExecutingDpc(void) { return FALSE; }
LONG WINAPI XBOXKRNL_KeSetDisableBoostThread(void *Thread, LONG Disable) { return 0; }

LONG WINAPI XBOXKRNL_KeTestAlertThread(LONG ApcMode)
{
    return NtTestAlert();
}

LONG WINAPI XBOXKRNL_KeSynchronizeExecution(void *Interrupt, void *Routine, void *Context)
{
    typedef BOOLEAN (WINAPI *PSYNC_ROUTINE)(void *);
    return ((PSYNC_ROUTINE)Routine)(Context);
}

LONG WINAPI XBOXKRNL_KeSaveFloatingPointState(void *State) { return 0; }
LONG WINAPI XBOXKRNL_KeRestoreFloatingPointState(void *State) { return 0; }

/* ======================================================================
 * Ke* device queues (simple linked-list, no waiter support)
 *
 * Xbox KDEVICE_QUEUE layout (offsets confirmed via Cxbx-Reloaded):
 *   +0 SHORT Type (0x08)
 *   +2 SHORT Size
 *   +4 LIST_ENTRY DeviceListHead  (Flink+4, Blink+8)
 *   +12 ULONG Lock (spinlock, ignored in usermode)
 *   +16 BOOLEAN Busy
 *
 * Xbox KDEVICE_QUEUE_ENTRY layout:
 *   +0 LIST_ENTRY DeviceListEntry
 *   +8 ULONG SortKey
 *   +12 BOOLEAN Inserted
 * ====================================================================== */
typedef struct _XBOX_DEV_QUEUE {
    SHORT Type;
    SHORT Size;
    LIST_ENTRY Head;
    ULONG Lock;
    BOOLEAN Busy;
} XBOX_DEV_QUEUE;

typedef struct _XBOX_DEV_QUEUE_ENTRY {
    LIST_ENTRY Entry;
    ULONG SortKey;
    BOOLEAN Inserted;
} XBOX_DEV_QUEUE_ENTRY;

void WINAPI XBOXKRNL_KeInitializeDeviceQueue(XBOX_DEV_QUEUE *q)
{
    q->Type = 0x08;
    q->Size = sizeof(*q);
    InitializeListHead(&q->Head);
    q->Lock = 0;
    q->Busy = FALSE;
}

LONG WINAPI XBOXKRNL_KeInsertDeviceQueue(XBOX_DEV_QUEUE *q, XBOX_DEV_QUEUE_ENTRY *entry)
{
    if (!q->Busy) { q->Busy = TRUE; entry->Inserted = FALSE; return FALSE; }
    InsertTailList(&q->Head, &entry->Entry);
    entry->Inserted = TRUE;
    return TRUE;
}

LONG WINAPI XBOXKRNL_KeInsertByKeyDeviceQueue(XBOX_DEV_QUEUE *q, XBOX_DEV_QUEUE_ENTRY *entry, ULONG key)
{
    LIST_ENTRY *le;
    entry->SortKey = key;
    if (!q->Busy) { q->Busy = TRUE; entry->Inserted = FALSE; return FALSE; }
    for (le = q->Head.Flink; le != &q->Head; le = le->Flink) {
        XBOX_DEV_QUEUE_ENTRY *cur = CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry);
        if (cur->SortKey > key) { InsertTailList(le, &entry->Entry); entry->Inserted = TRUE; return TRUE; }
    }
    InsertTailList(&q->Head, &entry->Entry);
    entry->Inserted = TRUE;
    return TRUE;
}

XBOX_DEV_QUEUE_ENTRY * WINAPI XBOXKRNL_KeRemoveDeviceQueue(XBOX_DEV_QUEUE *q)
{
    LIST_ENTRY *le;
    if (IsListEmpty(&q->Head)) { q->Busy = FALSE; return NULL; }
    le = RemoveHeadList(&q->Head);
    CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry)->Inserted = FALSE;
    return CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry);
}

XBOX_DEV_QUEUE_ENTRY * WINAPI XBOXKRNL_KeRemoveByKeyDeviceQueue(XBOX_DEV_QUEUE *q, ULONG key)
{
    LIST_ENTRY *le;
    if (IsListEmpty(&q->Head)) { q->Busy = FALSE; return NULL; }
    for (le = q->Head.Flink; le != &q->Head; le = le->Flink) {
        XBOX_DEV_QUEUE_ENTRY *cur = CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry);
        if (cur->SortKey >= key) {
            RemoveEntryList(&cur->Entry);
            cur->Inserted = FALSE;
            return cur;
        }
    }
    le = RemoveHeadList(&q->Head);
    CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry)->Inserted = FALSE;
    return CONTAINING_RECORD(le, XBOX_DEV_QUEUE_ENTRY, Entry);
}

LONG WINAPI XBOXKRNL_KeRemoveEntryDeviceQueue(XBOX_DEV_QUEUE *q, XBOX_DEV_QUEUE_ENTRY *entry)
{
    if (!entry->Inserted) return FALSE;
    RemoveEntryList(&entry->Entry);
    entry->Inserted = FALSE;
    return TRUE;
}

/* ======================================================================
 * Ke* interrupt stubs (hardware-only; connect/disconnect are no-ops)
 * ====================================================================== */
void WINAPI XBOXKRNL_KeInitializeInterrupt(void *Interrupt, void *ServiceRoutine,
    void *Context, LONG Vector, LONG Irql, LONG Mode, LONG ShareVector) { }
LONG WINAPI XBOXKRNL_KeConnectInterrupt(void *Interrupt) { return TRUE; }
void WINAPI XBOXKRNL_KeDisconnectInterrupt(void *Interrupt) { }

/* ======================================================================
 * Ex* read/write lock (usermode reader-writer lock using count convention:
 * 0=free, N>0=N shared readers, -1=exclusive)
 * ====================================================================== */
typedef struct { volatile LONG count; volatile LONG pad; } XBOX_RWLOCK;

void WINAPI XBOXKRNL_ExInitializeReadWriteLock(XBOX_RWLOCK *lock)
{
    lock->count = 0;
    lock->pad = 0;
}

void WINAPI XBOXKRNL_ExAcquireReadWriteLockExclusive(XBOX_RWLOCK *lock)
{
    while (InterlockedCompareExchange(&lock->count, -1, 0) != 0)
        SwitchToThread();
}

void WINAPI XBOXKRNL_ExAcquireReadWriteLockShared(XBOX_RWLOCK *lock)
{
    LONG old;
    do {
        old = lock->count;
        if (old < 0) { SwitchToThread(); continue; }
    } while (InterlockedCompareExchange(&lock->count, old + 1, old) != old);
}

void WINAPI XBOXKRNL_ExReleaseReadWriteLock(XBOX_RWLOCK *lock)
{
    LONG old;
    do { old = lock->count; } while (
        InterlockedCompareExchange(&lock->count, old < 0 ? 0 : old - 1, old) != old);
}

/* ======================================================================
 * Ex* interlocked operations
 * ====================================================================== */
LONG WINAPI XBOXKRNL_ExInterlockedAddLargeInteger(LONGLONG *Addend, LONGLONG Increment,
                                                   void *SpinLock)
{
    InterlockedAdd64(Addend, Increment);
    return 0;
}

void FASTCALL XBOXKRNL_ExInterlockedAddLargeStatistic(LONGLONG *Addend, ULONG Increment)
{
    InterlockedAdd64(Addend, (LONGLONG)Increment);
}

LONGLONG FASTCALL XBOXKRNL_ExInterlockedCompareExchange64(LONGLONG *Destination,
                                                           LONGLONG *Exchange,
                                                           LONGLONG *Comperand)
{
    return InterlockedCompareExchange64(Destination, *Exchange, *Comperand);
}

/* ======================================================================
 * Exf* interlocked list operations (fastcall)
 * ====================================================================== */
LIST_ENTRY * FASTCALL XBOXKRNL_ExfInterlockedInsertHeadList(LIST_ENTRY *head, LIST_ENTRY *entry)
{
    LIST_ENTRY *old_flink = head->Flink;
    entry->Flink = old_flink;
    entry->Blink = head;
    old_flink->Blink = entry;
    head->Flink = entry;
    return (old_flink == head) ? NULL : old_flink;
}

LIST_ENTRY * FASTCALL XBOXKRNL_ExfInterlockedInsertTailList(LIST_ENTRY *head, LIST_ENTRY *entry)
{
    LIST_ENTRY *old_blink = head->Blink;
    entry->Blink = old_blink;
    entry->Flink = head;
    old_blink->Flink = entry;
    head->Blink = entry;
    return (old_blink == head) ? NULL : old_blink;
}

LIST_ENTRY * FASTCALL XBOXKRNL_ExfInterlockedRemoveHeadList(LIST_ENTRY *head)
{
    LIST_ENTRY *entry;
    if (head->Flink == head) return NULL;
    entry = head->Flink;
    head->Flink = entry->Flink;
    entry->Flink->Blink = head;
    return entry;
}

/* ======================================================================
 * Ex* non-volatile (EEPROM) settings
 *
 * XC_* indices from Cxbx-Reloaded xboxkrnl/EEPROMManager.h.
 * We keep a small in-process array; games can read back values they save.
 * ====================================================================== */
#define XC_MAX_OS   0xFF
#define NVSTORE_MAX 256

static BYTE nv_type[NVSTORE_MAX];
static ULONG nv_data[NVSTORE_MAX];
static BOOL  nv_valid[NVSTORE_MAX];

static void nv_set_default(ULONG index, ULONG value)
{
    if (index < NVSTORE_MAX && !nv_valid[index]) {
        nv_data[index] = value;
        nv_type[index] = 4; /* REG_DWORD */
        nv_valid[index] = TRUE;
    }
}

static void nv_init(void)
{
    static volatile LONG once = 0;
    if (InterlockedCompareExchange(&once, 1, 0)) return;
    nv_set_default(7,  0);    /* XC_LANGUAGE: English */
    nv_set_default(8,  0x40); /* XC_VIDEO_FLAGS: NTSC 60Hz */
    nv_set_default(9,  0);    /* XC_AUDIO_FLAGS: stereo */
    nv_set_default(10, 0);    /* XC_PARENTAL_CONTROL_GAMES: off */
    nv_set_default(12, 0);    /* XC_PARENTAL_CONTROL_MOVIES: off */
    nv_set_default(17, 0);    /* XC_MISC_FLAGS */
    nv_set_default(18, 1);    /* XC_DVD_REGION: NTSC-US */
    nv_set_default(0x100, 1); /* XC_FACTORY_GAME_REGION: NTSC-US */
    nv_set_default(0x101, 0x400); /* XC_FACTORY_AV_REGION: NTSC 60Hz */
}

NTSTATUS WINAPI XBOXKRNL_ExQueryNonVolatileSetting(ULONG ValueIndex, ULONG *Type,
                                                    void *Value, ULONG ValueLength,
                                                    ULONG *ResultLength)
{
    nv_init();
    if (ValueIndex < NVSTORE_MAX && nv_valid[ValueIndex]) {
        ULONG sz = min(ValueLength, sizeof(ULONG));
        if (Type) *Type = nv_type[ValueIndex];
        if (Value) memcpy(Value, &nv_data[ValueIndex], sz);
        if (ResultLength) *ResultLength = sz;
        return STATUS_SUCCESS;
    }
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS WINAPI XBOXKRNL_ExSaveNonVolatileSetting(ULONG ValueIndex, ULONG Type,
                                                   void *Value, ULONG ValueLength)
{
    nv_init();
    if (ValueIndex < NVSTORE_MAX && ValueLength <= sizeof(ULONG)) {
        memcpy(&nv_data[ValueIndex], Value, ValueLength);
        nv_type[ValueIndex] = (BYTE)Type;
        nv_valid[ValueIndex] = TRUE;
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS WINAPI XBOXKRNL_ExReadWriteRefurbInfo(void *Info, ULONG Length, BOOL Write)
{
    if (!Write && Info) memset(Info, 0, Length);
    return STATUS_SUCCESS;
}

ULONG WINAPI XBOXKRNL_ExQueryPoolBlockSize(void *PoolBlock)
{
    return 0;
}

/* ======================================================================
 * Rtl* stack / unwind
 * ====================================================================== */
NTSYSAPI void  WINAPI RtlCaptureContext(CONTEXT *);
NTSYSAPI USHORT WINAPI RtlCaptureStackBackTrace(ULONG, ULONG, void **, ULONG *);
NTSYSAPI void  WINAPI RtlUnwind(void *, void *, EXCEPTION_RECORD *, void *);

void WINAPI XBOXKRNL_RtlCaptureContext(CONTEXT *ctx)
{
    RtlCaptureContext(ctx);
}

ULONG WINAPI XBOXKRNL_RtlCaptureStackBackTrace(ULONG FramesToSkip, ULONG FramesToCapture,
                                                void **BackTrace, ULONG *BackTraceHash)
{
    return (ULONG)RtlCaptureStackBackTrace(FramesToSkip + 1, FramesToCapture, BackTrace, BackTraceHash);
}

void WINAPI XBOXKRNL_RtlGetCallersAddress(void **CallersAddress, void **CallersCaller)
{
    void *frames[3] = { NULL, NULL, NULL };
    RtlCaptureStackBackTrace(1, 3, frames, NULL);
    if (CallersAddress)  *CallersAddress  = frames[1];
    if (CallersCaller)   *CallersCaller   = frames[2];
}

void WINAPI XBOXKRNL_RtlUnwind(void *TargetFrame, void *TargetIp,
                                EXCEPTION_RECORD *ExceptionRecord, void *ReturnValue)
{
    RtlUnwind(TargetFrame, TargetIp, ExceptionRecord, ReturnValue);
}

ULONG WINAPI XBOXKRNL_RtlWalkFrameChain(void **Callers, ULONG Count, ULONG Flags)
{
    return RtlWalkFrameChain(Callers, Count, Flags);
}

void WINAPI XBOXKRNL_RtlEnterCriticalSectionAndRegion(RTL_CRITICAL_SECTION *cs)
{
    RtlEnterCriticalSection(cs);
}

void WINAPI XBOXKRNL_RtlLeaveCriticalSectionAndRegion(RTL_CRITICAL_SECTION *cs)
{
    RtlLeaveCriticalSection(cs);
}

void WINAPI XBOXKRNL_RtlRip(void *Component, void *Message, void *Description)
{
    ERR("RtlRip called: component=%p message=%p\n", Component, Message);
    (void)Description;
    DebugBreak();
}

/* ======================================================================
 * Mm* address protection and I/O space
 * ====================================================================== */
void WINAPI XBOXKRNL_MmSetAddressProtect(void *BaseAddress, ULONG NumberOfBytes, ULONG NewProtect)
{
    DWORD old;
    VirtualProtect(BaseAddress, NumberOfBytes, NewProtect, &old);
}

ULONG WINAPI XBOXKRNL_MmQueryAddressProtect(void *VirtualAddress)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(VirtualAddress, &mbi, sizeof(mbi)) >= sizeof(mbi))
        return mbi.Protect;
    return 0;
}

void * WINAPI XBOXKRNL_MmMapIoSpace(ULONG PhysicalAddress, ULONG NumberOfBytes, ULONG CacheType)
{
    /* In usermode, "I/O space" is just regular memory */
    return VirtualAlloc(NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void WINAPI XBOXKRNL_MmUnmapIoSpace(void *BaseAddress, ULONG NumberOfBytes)
{
    VirtualFree(BaseAddress, 0, MEM_RELEASE);
}

void WINAPI XBOXKRNL_MmPersistContiguousMemory(void *BaseAddress, ULONG NumberOfBytes, BOOL Persist) { }
void WINAPI XBOXKRNL_MmLockUnlockBufferPages(void *BaseAddress, ULONG NumberOfBytes, LONG Lock) { }
void WINAPI XBOXKRNL_MmLockUnlockPhysicalPage(ULONG PhysicalAddress, LONG Lock) { }
ULONG WINAPI XBOXKRNL_MmClaimGpuInstanceMemory(ULONG NumberOfBytes, ULONG *NumberOfPaddingBytes)
{
    if (NumberOfPaddingBytes) *NumberOfPaddingBytes = 0;
    return 0;
}

/* MmDbg* - debug-only helpers */
void * WINAPI XBOXKRNL_MmDbgAllocateMemory(ULONG NumberOfBytes, ULONG Protect)
{
    return VirtualAlloc(NULL, NumberOfBytes, MEM_COMMIT | MEM_RESERVE,
                        Protect ? Protect : PAGE_READWRITE);
}
ULONG WINAPI XBOXKRNL_MmDbgFreeMemory(void *BaseAddress, ULONG NumberOfBytes)
{
    VirtualFree(BaseAddress, 0, MEM_RELEASE);
    return 0;
}
ULONG WINAPI XBOXKRNL_MmDbgQueryAvailablePages(void) { return 0; }
void  WINAPI XBOXKRNL_MmDbgReleaseAddress(void *VirtualAddress, void *Opaque) { }
ULONG WINAPI XBOXKRNL_MmDbgWriteCheck(void *VirtualAddress, void *Opaque) { return 0; }

/* ======================================================================
 * Dbg* debug helpers
 * ====================================================================== */
void WINAPI XBOXKRNL_DbgBreakPointWithStatus(ULONG Status)
{
    TRACE("DbgBreakPointWithStatus: 0x%08lx\n", Status);
    DebugBreak();
}

ULONG WINAPI XBOXKRNL_DbgPrompt(const char *Output, char *Input, ULONG InputLength)
{
    OutputDebugStringA(Output);
    if (Input && InputLength > 0) Input[0] = '\0';
    return 0;
}

void WINAPI XBOXKRNL_DbgLoadImageSymbols(void *Name, void *Base, ULONG ProcessId) { }
void WINAPI XBOXKRNL_DbgUnLoadImageSymbols(void *Name, void *Base, ULONG ProcessId) { }

/* ======================================================================
 * Port I/O - usermode has no port access; silently no-op / return zero
 * ====================================================================== */
void WINAPI XBOXKRNL_READ_PORT_BUFFER_UCHAR(ULONG Port, BYTE *Buffer, ULONG Count)
{
    if (Buffer) memset(Buffer, 0, Count);
}
void WINAPI XBOXKRNL_READ_PORT_BUFFER_USHORT(ULONG Port, USHORT *Buffer, ULONG Count)
{
    if (Buffer) memset(Buffer, 0, Count * sizeof(USHORT));
}
void WINAPI XBOXKRNL_READ_PORT_BUFFER_ULONG(ULONG Port, ULONG *Buffer, ULONG Count)
{
    if (Buffer) memset(Buffer, 0, Count * sizeof(ULONG));
}
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_UCHAR(ULONG Port, BYTE *Buffer, ULONG Count) { }
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_USHORT(ULONG Port, USHORT *Buffer, ULONG Count) { }
void WINAPI XBOXKRNL_WRITE_PORT_BUFFER_ULONG(ULONG Port, ULONG *Buffer, ULONG Count) { }
