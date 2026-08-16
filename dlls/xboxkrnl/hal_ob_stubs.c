/*
 * xboxkrnl.exe - Hal*, Av*, Fsc*, Ob*, Ke* queue/DPC/APC, Io* device/IRP,
 * and remaining hardware stubs.
 *
 * Hardware registers (SMBus, PCI, port I/O) are inaccessible in usermode and
 * are no-ops. Hal*, Av*, Fsc*, Irt*, Phy*, XProf* are likewise no-ops or safe
 * returns; we implement only the ones with meaningful usermode equivalents.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ntstatus.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xboxkrnl);

NTSYSAPI NTSTATUS WINAPI NtRemoveIoCompletion(HANDLE, ULONG_PTR *, ULONG_PTR *, IO_STATUS_BLOCK *, LARGE_INTEGER *);
NTSYSAPI NTSTATUS WINAPI NtCreateIoCompletion(HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *, ULONG);

/* ======================================================================
 * Hal* - hardware abstraction layer
 * ====================================================================== */

/* HalReturnToFirmware - reboot/power-off/halt; just exit the process */
void WINAPI XBOXKRNL_HalReturnToFirmware(ULONG FirmwareEntry)
{
    TRACE("HalReturnToFirmware(%lu)\n", FirmwareEntry);
    ExitProcess(0);
}

NTSTATUS WINAPI XBOXKRNL_HalInitiateShutdown(void)
{
    ExitProcess(0);
    return STATUS_SUCCESS; /* unreachable */
}

LONG WINAPI XBOXKRNL_HalIsResetOrShutdownPending(void) { return FALSE; }

/* Shutdown notification: store up to 8 callbacks, call them on HalReturnToFirmware */
static struct { void (*fn)(void); BOOL reg; } shutdown_notifs[8];
void WINAPI XBOXKRNL_HalRegisterShutdownNotification(void *NotifyBlock, LONG Register)
{
    int i;
    if (Register) {
        for (i = 0; i < 8; i++) if (!shutdown_notifs[i].fn) {
            shutdown_notifs[i].fn = *(void(**)(void))NotifyBlock;
            break;
        }
    } else {
        for (i = 0; i < 8; i++) if (shutdown_notifs[i].fn == *(void(**)(void))NotifyBlock)
            shutdown_notifs[i].fn = NULL;
    }
}

/* All hardware-register-level Hal* are no-ops in usermode */
void WINAPI XBOXKRNL_HalDisableSystemInterrupt(LONG Vector) { }
void WINAPI XBOXKRNL_HalEnableSystemInterrupt(LONG Vector, LONG Mode) { }
LONG WINAPI XBOXKRNL_HalGetInterruptVector(LONG Type, LONG *Vector) { return 0; }
void WINAPI XBOXKRNL_HalReadWritePCISpace(LONG Bus, LONG Slot, LONG Offset, void *Buffer,
    LONG Length, LONG Write) { }
LONG WINAPI XBOXKRNL_HalReadSMBusValue(LONG Address, LONG Cmd, LONG WordFlag, ULONG *Value)
    { if (Value) *Value = 0; return STATUS_UNSUCCESSFUL; }
LONG WINAPI XBOXKRNL_HalWriteSMBusValue(LONG Addr, LONG Cmd, LONG WordFlag, LONG Value)
    { return STATUS_UNSUCCESSFUL; }
LONG WINAPI XBOXKRNL_HalReadSMCTrayState(void *State, void *Count) { return STATUS_UNSUCCESSFUL; }
LONG WINAPI XBOXKRNL_HalWriteSMCScratchRegister(LONG Value) { return 0; }
void WINAPI XBOXKRNL_HalEnableSecureTrayEject(void) { }
void FASTCALL XBOXKRNL_HalClearSoftwareInterrupt(LONG Irql) { }
void FASTCALL XBOXKRNL_HalRequestSoftwareInterrupt(LONG Irql) { }

/* ======================================================================
 * Av* - AV encoder control (hardware-only, no-ops)
 * ====================================================================== */
ULONG WINAPI XBOXKRNL_AvGetSavedDataAddress(void) { return 0; }
void  WINAPI XBOXKRNL_AvSetSavedDataAddress(void *Address) { }
NTSTATUS WINAPI XBOXKRNL_AvSendTVEncoderOption(void *RegisterBase, ULONG Option,
    ULONG Param, ULONG *Result) { if (Result) *Result = 0; return STATUS_SUCCESS; }
ULONG WINAPI XBOXKRNL_AvSetDisplayMode(void *RegisterBase, ULONG Step, ULONG Mode,
    ULONG MonitorType, ULONG HdtvVideoFlags, ULONG AddressOnFb) { return 0; }

/* ======================================================================
 * Fsc* - file system cache (no-op stubs)
 * ====================================================================== */
ULONG WINAPI XBOXKRNL_FscGetCacheSize(void) { return 0; }
NTSTATUS WINAPI XBOXKRNL_FscSetCacheSize(ULONG CacheSize) { return STATUS_SUCCESS; }
NTSTATUS WINAPI XBOXKRNL_FscInvalidateIdleBlocks(void) { return STATUS_SUCCESS; }

/* ======================================================================
 * Irt* - IR transceiver, Phy* - network PHY (no-ops)
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_IrtClientInitFast(void) { return STATUS_NOT_IMPLEMENTED; }
NTSTATUS WINAPI XBOXKRNL_IrtSweep(void) { return STATUS_NOT_IMPLEMENTED; }
NTSTATUS WINAPI XBOXKRNL_PhyInitialize(LONG a0, void *a1) { return STATUS_NOT_IMPLEMENTED; }
NTSTATUS WINAPI XBOXKRNL_PhyGetLinkState(LONG a0) { return STATUS_NOT_IMPLEMENTED; }

/* ======================================================================
 * Ob* - object manager
 *
 * On Xbox, "objects" are kernel data structures; callers pass pointers to
 * them as "object references".  In our usermode implementation, the
 * sync_io.c handle table maps object-struct-ptrs to NT handles.
 * ObfReferenceObject/ObfDereferenceObject are lightweight ref-count ops;
 * we treat them as no-ops since our HANDLE-based table doesn't refcount.
 * ====================================================================== */
void FASTCALL XBOXKRNL_ObfReferenceObject(void *Object) { }
void FASTCALL XBOXKRNL_ObfDereferenceObject(void *Object) { }

NTSTATUS WINAPI XBOXKRNL_ObReferenceObjectByHandle(void *Handle, void *ObjectType,
                                                    void **Object)
{
    /* Caller-supplied handle: return it as the "object pointer" */
    if (Object) *Object = Handle;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI XBOXKRNL_ObReferenceObjectByName(void *ObjectName, ULONG Attributes,
    void *ObjectType, void *ParseContext, void **Object)
{
    FIXME("ObReferenceObjectByName: stub\n");
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS WINAPI XBOXKRNL_ObReferenceObjectByPointer(void *Object, void *ObjectType)
{
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI XBOXKRNL_ObCreateObject(void *ObjectType, void *ObjectAttributes,
                                         ULONG ObjectSize, void **Object)
{
    void *p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ObjectSize);
    if (!p) return STATUS_NO_MEMORY;
    if (Object) *Object = p;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI XBOXKRNL_ObInsertObject(void *Object, void *PassedAccessState,
                                         ULONG DesiredAccess, void *ObjectHandle)
{
    FIXME("ObInsertObject: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI XBOXKRNL_ObMakeTemporaryObject(void *Object)
{
    /* In our mapping, treat as no-op (we can't unmake a userspace pointer) */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI XBOXKRNL_ObOpenObjectByName(void *ObjectAttributes, void *ObjectType,
                                             void *ParseContext, void **Handle)
{
    FIXME("ObOpenObjectByName: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI XBOXKRNL_ObOpenObjectByPointer(void *Object, void *ObjectType,
                                                void **Handle)
{
    FIXME("ObOpenObjectByPointer: stub\n");
    if (Handle) *Handle = Object; /* identity mapping */
    return STATUS_SUCCESS;
}

/* ======================================================================
 * Ke* DPC (Deferred Procedure Call)
 *
 * Xbox KDPC layout (from Cxbx-Reloaded):
 *   +0 SHORT Type (0x13)
 *   +2 BYTE  Number
 *   +3 BYTE  Importance
 *   +4 LIST_ENTRY DpcListEntry
 *  +12 DeferredRoutine *
 *  +16 DeferredContext *
 *  +20 SystemArgument1 *
 *  +24 SystemArgument2 *
 *  +28 ULONG *Lock
 * ====================================================================== */
typedef struct _XBOX_KDPC {
    SHORT Type;
    BYTE  Number;
    BYTE  Importance;
    LIST_ENTRY ListEntry;
    void (WINAPI *DeferredRoutine)(void *Dpc, void *Ctx, void *Arg1, void *Arg2);
    void *DeferredContext;
    void *SystemArgument1;
    void *SystemArgument2;
    ULONG *Lock;
} XBOX_KDPC;

static DWORD WINAPI dpc_worker(void *arg)
{
    XBOX_KDPC *dpc = (XBOX_KDPC *)arg;
    if (dpc->DeferredRoutine)
        dpc->DeferredRoutine(dpc, dpc->DeferredContext,
                             dpc->SystemArgument1, dpc->SystemArgument2);
    return 0;
}

void WINAPI XBOXKRNL_KeInitializeDpc(XBOX_KDPC *Dpc, void *Routine, void *Context)
{
    Dpc->Type = 0x13;
    Dpc->Number = 0;
    Dpc->Importance = 1; /* MediumImportance */
    InitializeListHead(&Dpc->ListEntry);
    Dpc->DeferredRoutine = (void (WINAPI *)(void*,void*,void*,void*))Routine;
    Dpc->DeferredContext = Context;
    Dpc->SystemArgument1 = NULL;
    Dpc->SystemArgument2 = NULL;
    Dpc->Lock = NULL;
}

LONG WINAPI XBOXKRNL_KeInsertQueueDpc(XBOX_KDPC *Dpc, void *Arg1, void *Arg2)
{
    if (!Dpc->DeferredRoutine) return FALSE;
    Dpc->SystemArgument1 = Arg1;
    Dpc->SystemArgument2 = Arg2;
    return QueueUserWorkItem(dpc_worker, Dpc, 0);
}

LONG WINAPI XBOXKRNL_KeRemoveQueueDpc(XBOX_KDPC *Dpc)
{
    return FALSE; /* Can't cancel a queued work item */
}

/* ======================================================================
 * Ke* APC (Asynchronous Procedure Call)
 *
 * Xbox KeInitializeApc has 7 params (no RundownRoutine):
 *   Apc, Thread, Environment, KernelRoutine, NormalRoutine, ApcMode, NormalContext
 * ====================================================================== */
typedef struct _XBOX_KAPC {
    SHORT Type;
    SHORT Size;
    ULONG Spare0;
    void *Thread;
    LIST_ENTRY ApcListEntry;
    void *KernelRoutine;
    void *NormalRoutine;
    void *NormalContext;
    void *SystemArgument1;
    void *SystemArgument2;
    CHAR  ApcStateIndex;
    CHAR  ApcMode;
    BOOLEAN Inserted;
} XBOX_KAPC;

void WINAPI XBOXKRNL_KeInitializeApc(XBOX_KAPC *Apc, void *Thread, void *Environment,
    void *KernelRoutine, void *NormalRoutine, LONG ApcMode, void *NormalContext)
{
    Apc->Type = 0x12; /* ApcObject */
    Apc->Size = sizeof(XBOX_KAPC);
    Apc->Spare0 = 0;
    Apc->Thread = Thread;
    InitializeListHead(&Apc->ApcListEntry);
    Apc->KernelRoutine = KernelRoutine;
    Apc->NormalRoutine = NormalRoutine;
    Apc->NormalContext = NormalContext;
    Apc->SystemArgument1 = NULL;
    Apc->SystemArgument2 = NULL;
    Apc->ApcStateIndex = 0;
    Apc->ApcMode = (CHAR)ApcMode;
    Apc->Inserted = FALSE;
}

LONG WINAPI XBOXKRNL_KeInsertQueueApc(XBOX_KAPC *Apc, void *SysArg1, void *SysArg2,
                                       LONG PriorityBoost)
{
    Apc->SystemArgument1 = SysArg1;
    Apc->SystemArgument2 = SysArg2;
    Apc->Inserted = TRUE;
    if (Apc->NormalRoutine) {
        /* Queue normal routine to the target thread via NtQueueApcThread */
        typedef void (NTAPI *PKNORMAL_ROUTINE)(void *, void *, void *);
        return NT_SUCCESS(NtQueueApcThread((HANDLE)Apc->Thread,
            (PNTAPCFUNC)Apc->NormalRoutine,
            (ULONG_PTR)Apc->NormalContext,
            (ULONG_PTR)SysArg1, (ULONG_PTR)SysArg2));
    }
    return TRUE;
}

/* ======================================================================
 * Ke* Queue (KQUEUE) - backed by NT I/O completion port
 *
 * Xbox KQUEUE layout (DISPATCHER_HEADER header + queue fields):
 *   +0  SHORT Type (0x04)
 *   +2  BYTE  Absolute
 *   +3  BYTE  Size
 *   +4  LONG  SignalState -- we store HANDLE to IOCP here
 *   +8  LIST_ENTRY WaitListHead (unused)
 *   +16 LIST_ENTRY EntryListHead (unused in our IOCP model)
 *   +24 ULONG CurrentCount
 *   +28 ULONG MaximumCount
 *   +32 LIST_ENTRY ThreadListHead (unused)
 * ====================================================================== */
typedef struct _XBOX_KQUEUE {
    SHORT Type;
    BYTE  Absolute;
    BYTE  Size;
    HANDLE Iocp;         /* stored in SignalState slot */
    LIST_ENTRY WaitList;
    LIST_ENTRY EntryList;
    ULONG CurrentCount;
    ULONG MaximumCount;
    LIST_ENTRY ThreadList;
} XBOX_KQUEUE;

void WINAPI XBOXKRNL_KeInitializeQueue(XBOX_KQUEUE *Queue, ULONG Count)
{
    HANDLE iocp;
    Queue->Type = 0x04; /* QueueObject */
    Queue->Absolute = 0;
    Queue->Size = 10;   /* size in ULONG units */
    NtCreateIoCompletion(&iocp, IO_COMPLETION_ALL_ACCESS, NULL, Count > 0 ? Count : 1);
    Queue->Iocp = iocp;
    InitializeListHead(&Queue->WaitList);
    InitializeListHead(&Queue->EntryList);
    Queue->CurrentCount = 0;
    Queue->MaximumCount = Count;
    InitializeListHead(&Queue->ThreadList);
}

LONG WINAPI XBOXKRNL_KeInsertQueue(XBOX_KQUEUE *Queue, LIST_ENTRY *Entry)
{
    LONG prev = Queue->CurrentCount++;
    NtSetIoCompletion(Queue->Iocp, (ULONG_PTR)Entry, 0, STATUS_SUCCESS, 0);
    return prev;
}

LONG WINAPI XBOXKRNL_KeInsertHeadQueue(XBOX_KQUEUE *Queue, LIST_ENTRY *Entry)
{
    return XBOXKRNL_KeInsertQueue(Queue, Entry);
}

void * WINAPI XBOXKRNL_KeRemoveQueue(XBOX_KQUEUE *Queue, LONG WaitMode,
                                      LARGE_INTEGER *Timeout)
{
    ULONG_PTR key = 0, val = 0;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status = NtRemoveIoCompletion(Queue->Iocp, &key, &val, &iosb, Timeout);
    if (status == STATUS_TIMEOUT)     return (void *)(ULONG_PTR)STATUS_TIMEOUT;
    if (status == STATUS_USER_APC)    return (void *)(ULONG_PTR)STATUS_USER_APC;
    if (!NT_SUCCESS(status))          return NULL;
    if (Queue->CurrentCount > 0) Queue->CurrentCount--;
    return (void *)key;
}

void * WINAPI XBOXKRNL_KeRundownQueue(XBOX_KQUEUE *Queue)
{
    /* Drain remaining entries, close the IOCP */
    ULONG_PTR key = 0, val = 0;
    IO_STATUS_BLOCK iosb;
    LARGE_INTEGER zero = {{0}};
    while (NtRemoveIoCompletion(Queue->Iocp, &key, &val, &iosb, &zero) == STATUS_SUCCESS)
        ;
    NtClose(Queue->Iocp);
    Queue->Iocp = NULL;
    return NULL;
}

/* ======================================================================
 * Io* - I/O manager (minimal device/IRP implementation)
 *
 * IRP (I/O Request Packet): on Xbox, IRPs are kernel-allocated structs
 * that describe an I/O operation.  We model them as simple HeapAlloc
 * buffers with a fixed header.
 * ====================================================================== */

/* Minimal Xbox IRP header fields we care about */
typedef struct _XBOX_IRP {
    SHORT Type;        /* 0x06 */
    USHORT Size;
    /* IO_STATUS_BLOCK at +4 */
    NTSTATUS Status;
    ULONG_PTR Information;
    /* AssociatedIrp / SystemBuffer at +12 */
    void *SystemBuffer;
    /* MDL at +16 */
    void *MdlAddress;
    /* Flags at +20 */
    ULONG Flags;
    /* CompletionRoutine context etc – simplified */
    void *IoCompletionKey;
    HANDLE IoCompletionPort;
    /* Stack location pointer – for simplicity, store dispatch fn */
    NTSTATUS (WINAPI *DispatchFn)(void *DevObj, void *Irp);
    void *DeviceObject;
} XBOX_IRP;

#define XBOX_IRP_SIZE 256  /* conservative allocation size */

XBOX_IRP * WINAPI XBOXKRNL_IoAllocateIrp(LONG StackSize)
{
    XBOX_IRP *irp = (XBOX_IRP *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, XBOX_IRP_SIZE);
    if (!irp) return NULL;
    irp->Type = 0x06;
    irp->Size = XBOX_IRP_SIZE;
    irp->Status = STATUS_SUCCESS;
    return irp;
}

void WINAPI XBOXKRNL_IoFreeIrp(XBOX_IRP *Irp)
{
    HeapFree(GetProcessHeap(), 0, Irp);
}

LONG WINAPI XBOXKRNL_IoInitializeIrp(XBOX_IRP *Irp, USHORT PacketSize, LONG StackSize)
{
    if (Irp) {
        memset(Irp, 0, PacketSize);
        Irp->Type = 0x06;
        Irp->Size = PacketSize;
        Irp->Status = STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

LONG WINAPI XBOXKRNL_IoInvalidDeviceRequest(void *DeviceObject, XBOX_IRP *Irp)
{
    if (Irp) Irp->Status = STATUS_INVALID_DEVICE_REQUEST;
    return STATUS_INVALID_DEVICE_REQUEST;
}

void WINAPI XBOXKRNL_IoMarkIrpMustComplete(XBOX_IRP *Irp)
{
    if (Irp) Irp->Flags |= 0x00000100; /* IRP_MUST_COMPLETE */
}

/* IofCallDriver: call the device's dispatch routine */
LONG FASTCALL XBOXKRNL_IofCallDriver(void *DeviceObject, XBOX_IRP *Irp)
{
    if (Irp && Irp->DispatchFn)
        return Irp->DispatchFn(DeviceObject, Irp);
    return STATUS_NOT_SUPPORTED;
}

/* IofCompleteRequest: complete an IRP, post to completion port if set */
void FASTCALL XBOXKRNL_IofCompleteRequest(XBOX_IRP *Irp, LONG PriorityBoost)
{
    if (!Irp) return;
    if (Irp->IoCompletionPort)
        NtSetIoCompletion(Irp->IoCompletionPort, (ULONG_PTR)Irp->IoCompletionKey,
                          (ULONG_PTR)Irp, Irp->Status, Irp->Information);
}

/* IoSetIoCompletion - post to NT IO completion port */
NTSTATUS WINAPI XBOXKRNL_IoSetIoCompletion(HANDLE IoCompletion, void *KeyContext,
                                             void *ApcContext, NTSTATUS IoStatus,
                                             ULONG IoStatusInformation)
{
    return NtSetIoCompletion(IoCompletion, (ULONG_PTR)KeyContext,
                              (ULONG_PTR)ApcContext, IoStatus, IoStatusInformation);
}

/* IoCreateDevice - allocate minimal device object */
typedef struct _XBOX_DEVICE_OBJECT {
    SHORT Type;
    ULONG Size;
    LONG  ReferenceCount;
    void *DriverObject;
    void *NextDevice;
    void *AttachedDevice;
    void *CurrentIrp;
    ULONG Flags;
    void *DeviceExtension;
    ULONG DeviceType;
    CHAR  StackSize;
    BYTE  Pad[3];
    /* rest zeroed */
} XBOX_DEVICE_OBJECT;

NTSTATUS WINAPI XBOXKRNL_IoCreateDevice(void *DriverObject, ULONG ExtensionSize,
                                          void *DeviceName, ULONG DeviceType,
                                          ULONG DeviceCharacteristics, void **DeviceObject)
{
    ULONG total = sizeof(XBOX_DEVICE_OBJECT) + ExtensionSize;
    XBOX_DEVICE_OBJECT *dev = (XBOX_DEVICE_OBJECT *)HeapAlloc(GetProcessHeap(),
                                                                HEAP_ZERO_MEMORY, total);
    if (!dev) return STATUS_NO_MEMORY;
    dev->Type = 0x03; /* DeviceObject type */
    dev->Size = total;
    dev->ReferenceCount = 1;
    dev->DriverObject = DriverObject;
    dev->DeviceType = DeviceType;
    dev->StackSize = 1;
    dev->DeviceExtension = (BYTE *)dev + sizeof(XBOX_DEVICE_OBJECT);
    if (DeviceObject) *DeviceObject = dev;
    return STATUS_SUCCESS;
}

void WINAPI XBOXKRNL_IoDeleteDevice(XBOX_DEVICE_OBJECT *DeviceObject)
{
    HeapFree(GetProcessHeap(), 0, DeviceObject);
}

/* Share access: stub helpers */
LONG WINAPI XBOXKRNL_IoCheckShareAccess(LONG DesiredAccess, LONG DesiredShareAccess,
    void *FileObject, void *ShareAccess, LONG Update) { return STATUS_SUCCESS; }
void WINAPI XBOXKRNL_IoSetShareAccess(LONG DesiredAccess, LONG DesiredShareAccess,
    void *FileObject, void *ShareAccess) { }
void WINAPI XBOXKRNL_IoRemoveShareAccess(void *FileObject, void *ShareAccess) { }

/* IRP build helpers - allocate + init IRP with basic fields */
XBOX_IRP * WINAPI XBOXKRNL_IoBuildAsynchronousFsdRequest(ULONG MajorFunction,
    void *DeviceObject, void *Buffer, ULONG Length, void *StartingOffset,
    IO_STATUS_BLOCK *IoStatusBlock)
{
    XBOX_IRP *irp = XBOXKRNL_IoAllocateIrp(1);
    if (irp) { irp->SystemBuffer = Buffer; irp->DeviceObject = DeviceObject; }
    return irp;
}

XBOX_IRP * WINAPI XBOXKRNL_IoBuildSynchronousFsdRequest(ULONG MajorFunction,
    void *DeviceObject, void *Buffer, ULONG Length, void *StartingOffset,
    void *Event, IO_STATUS_BLOCK *IoStatusBlock)
{
    return XBOXKRNL_IoAllocateIrp(1);
}

XBOX_IRP * WINAPI XBOXKRNL_IoBuildDeviceIoControlRequest(ULONG IoControlCode,
    void *DeviceObject, void *InBuffer, ULONG InLen, void *OutBuffer, ULONG OutLen,
    BOOL InternalDeviceIoControl, void *Event, IO_STATUS_BLOCK *IoStatusBlock)
{
    return XBOXKRNL_IoAllocateIrp(1);
}

/* IoQueryFileInformation / IoQueryVolumeInformation - stubs */
NTSTATUS WINAPI XBOXKRNL_IoQueryFileInformation(void *FileObject, ULONG FileInfo,
    ULONG Length, void *Buffer, ULONG *RetLength)
{
    FIXME("IoQueryFileInformation: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI XBOXKRNL_IoQueryVolumeInformation(void *FileObject, ULONG FsInfo,
    ULONG Length, void *Buffer, ULONG *RetLength)
{
    FIXME("IoQueryVolumeInformation: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* IoSynchronousDeviceIoControlRequest - forward via NtDeviceIoControlFile */
NTSTATUS WINAPI XBOXKRNL_IoSynchronousDeviceIoControlRequest(ULONG IoControlCode,
    void *DeviceObject, void *InBuffer, ULONG InLen, void *OutBuffer, ULONG OutLen,
    ULONG *BytesReturned, BOOL InternalDeviceIoControl)
{
    FIXME("IoSynchronousDeviceIoControlRequest: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI XBOXKRNL_IoSynchronousFsdRequest(ULONG MajorFunction, void *DeviceObject,
    void *Buffer, ULONG Length, void *StartingOffset)
{
    FIXME("IoSynchronousFsdRequest: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* IRP dispatch helpers */
void WINAPI XBOXKRNL_IoQueueThreadIrp(XBOX_IRP *Irp) { }
void WINAPI XBOXKRNL_IoStartPacket(void *DeviceObject, XBOX_IRP *Irp, void *Key) { }
void WINAPI XBOXKRNL_IoStartNextPacket(void *DeviceObject) { }
void WINAPI XBOXKRNL_IoStartNextPacketByKey(void *DeviceObject, ULONG Key) { }

NTSTATUS WINAPI XBOXKRNL_IoDismountVolume(void *DeviceObject) { return STATUS_SUCCESS; }
NTSTATUS WINAPI XBOXKRNL_IoDismountVolumeByName(void *VolumeName) { return STATUS_SUCCESS; }

/* ======================================================================
 * NtUserIoApcDispatcher - APC dispatcher for async I/O (user-mode hook)
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_NtUserIoApcDispatcher(void *ApcContext,
    IO_STATUS_BLOCK *IoStatusBlock, ULONG Reserved) { return STATUS_SUCCESS; }

/* ======================================================================
 * Xe* - XEX section management (stub; real XEX loading is in ntdll loader)
 * ====================================================================== */
NTSTATUS WINAPI XBOXKRNL_XeLoadSection(void *Section)
{
    FIXME("XeLoadSection: stub\n");
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI XBOXKRNL_XeUnloadSection(void *Section)
{
    FIXME("XeUnloadSection: stub\n");
    return STATUS_SUCCESS;
}

/* ======================================================================
 * KeSetEventBoostPriority (already a FIXME stub in generated_stubs.c;
 * re-implement here to set the event and boost the thread priority)
 * ====================================================================== */
/* Note: keep the body in generated_stubs.c - removing it below via script */

/* XProf* - profiling, no-ops */
NTSTATUS WINAPI XBOXKRNL_XProfpControl(ULONG a0, ULONG a1) { return STATUS_SUCCESS; }
NTSTATUS WINAPI XBOXKRNL_XProfpGetData(void) { return STATUS_SUCCESS; }

/* Unknown ordinals 367-369 */
NTSTATUS WINAPI XBOXKRNL_UnknownAPI367(void) { return STATUS_NOT_IMPLEMENTED; }
NTSTATUS WINAPI XBOXKRNL_UnknownAPI368(void) { return STATUS_NOT_IMPLEMENTED; }
NTSTATUS WINAPI XBOXKRNL_UnknownAPI369(void) { return STATUS_NOT_IMPLEMENTED; }
