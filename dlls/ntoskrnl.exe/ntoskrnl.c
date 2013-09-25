/*
 * ntoskrnl.exe implementation
 *
 * Copyright (C) 2007 Alexandre Julliard
 * Copyright (C) 2010 Damjan Jovanovic
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "excpt.h"
#include "winioctl.h"
#include "ddk/ntddk.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);
WINE_DECLARE_DEBUG_CHANNEL(relay);

BOOLEAN KdDebuggerEnabled = FALSE;

extern LONG CALLBACK vectored_handler( EXCEPTION_POINTERS *ptrs );

KSYSTEM_TIME KeTickCount = { 0, 0, 0 };

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
    PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[4] = { { 0 } };

typedef void (WINAPI *PCREATE_PROCESS_NOTIFY_ROUTINE)(HANDLE,HANDLE,BOOLEAN);
typedef void (WINAPI *PCREATE_THREAD_NOTIFY_ROUTINE)(HANDLE,HANDLE,BOOLEAN);

static struct list Irps = LIST_INIT(Irps);

struct IrpInstance
{
    struct list entry;
    IRP *irp;
};

/* tid of the thread running client request */
static DWORD request_thread;

/* pid/tid of the client thread */
static DWORD client_tid;
static DWORD client_pid;

#ifdef __i386__
#define DEFINE_FASTCALL1_ENTRYPOINT( name ) \
    __ASM_STDCALL_FUNC( name, 4, \
                       "popl %eax\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name __ASM_STDCALL(4))
#define DEFINE_FASTCALL2_ENTRYPOINT( name ) \
    __ASM_STDCALL_FUNC( name, 8, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name __ASM_STDCALL(8))
#define DEFINE_FASTCALL3_ENTRYPOINT( name ) \
    __ASM_STDCALL_FUNC( name, 12, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name __ASM_STDCALL(12))
#endif

static inline LPCSTR debugstr_us( const UNICODE_STRING *us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

static HANDLE get_device_manager(void)
{
    static HANDLE device_manager;
    HANDLE handle = 0, ret = device_manager;

    if (!ret)
    {
        SERVER_START_REQ( create_device_manager )
        {
            req->access     = SYNCHRONIZE;
            req->attributes = 0;
            if (!wine_server_call( req )) handle = wine_server_ptr_handle( reply->handle );
        }
        SERVER_END_REQ;

        if (!handle)
        {
            ERR( "failed to create the device manager\n" );
            return 0;
        }
        if (!(ret = InterlockedCompareExchangePointer( &device_manager, handle, 0 )))
            ret = handle;
        else
            NtClose( handle );  /* somebody beat us to it */
    }
    return ret;
}

/* process an ioctl request for a given device */
static NTSTATUS process_ioctl( DEVICE_OBJECT *device, ULONG code, void *in_buff, ULONG in_size,
                               void *out_buff, ULONG *out_size )
{
    IRP irp;
    MDL mdl;
    IO_STACK_LOCATION irpsp;
    PDRIVER_DISPATCH dispatch = device->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    NTSTATUS status;
    LARGE_INTEGER count;

    TRACE( "ioctl %x device %p in_size %u out_size %u\n", code, device, in_size, *out_size );

    /* so we can spot things that we should initialize */
    memset( &irp, 0x55, sizeof(irp) );
    memset( &irpsp, 0x66, sizeof(irpsp) );
    memset( &mdl, 0x77, sizeof(mdl) );

    irp.RequestorMode = UserMode;
    if ((code & 3) == METHOD_BUFFERED)
    {
        irp.AssociatedIrp.SystemBuffer = HeapAlloc( GetProcessHeap(), 0, max( in_size, *out_size ) );
        if (!irp.AssociatedIrp.SystemBuffer)
            return STATUS_NO_MEMORY;
        memcpy( irp.AssociatedIrp.SystemBuffer, in_buff, in_size );
    }
    else
        irp.AssociatedIrp.SystemBuffer = in_buff;
    irp.UserBuffer = out_buff;
    irp.MdlAddress = &mdl;
    irp.Tail.Overlay.s.u2.CurrentStackLocation = &irpsp;
    irp.UserIosb = NULL;

    irpsp.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpsp.Parameters.DeviceIoControl.OutputBufferLength = *out_size;
    irpsp.Parameters.DeviceIoControl.InputBufferLength = in_size;
    irpsp.Parameters.DeviceIoControl.IoControlCode = code;
    irpsp.Parameters.DeviceIoControl.Type3InputBuffer = in_buff;
    irpsp.DeviceObject = device;
    irpsp.CompletionRoutine = NULL;

    mdl.Next = NULL;
    mdl.Size = 0;
    mdl.StartVa = out_buff;
    mdl.ByteCount = *out_size;
    mdl.ByteOffset = 0;

    device->CurrentIrp = &irp;

    KeQueryTickCount( &count );  /* update the global KeTickCount */

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call driver dispatch %p (device=%p,irp=%p)\n",
                 GetCurrentThreadId(), dispatch, device, &irp );

    status = dispatch( device, &irp );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  driver dispatch %p (device=%p,irp=%p) retval=%08x\n",
                 GetCurrentThreadId(), dispatch, device, &irp, status );

    *out_size = (irp.IoStatus.u.Status >= 0) ? irp.IoStatus.Information : 0;
    if ((code & 3) == METHOD_BUFFERED)
    {
        if (out_buff) memcpy( out_buff, irp.AssociatedIrp.SystemBuffer, *out_size );
        HeapFree( GetProcessHeap(), 0, irp.AssociatedIrp.SystemBuffer );
    }
    return irp.IoStatus.u.Status;
}


/***********************************************************************
 *           wine_ntoskrnl_main_loop   (Not a Windows API)
 */
NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event )
{
    HANDLE manager = get_device_manager();
    obj_handle_t ioctl = 0;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG code = 0;
    void *in_buff, *out_buff = NULL;
    DEVICE_OBJECT *device = NULL;
    ULONG in_size = 4096, out_size = 0;
    HANDLE handles[2];

    request_thread = GetCurrentThreadId();

    if (!(in_buff = HeapAlloc( GetProcessHeap(), 0, in_size )))
    {
        ERR( "failed to allocate buffer\n" );
        return STATUS_NO_MEMORY;
    }

    handles[0] = stop_event;
    handles[1] = manager;

    for (;;)
    {
        SERVER_START_REQ( get_next_device_request )
        {
            req->manager = wine_server_obj_handle( manager );
            req->prev = ioctl;
            req->status = status;
            wine_server_add_data( req, out_buff, out_size );
            wine_server_set_reply( req, in_buff, in_size );
            if (!(status = wine_server_call( req )))
            {
                code       = reply->code;
                ioctl      = reply->next;
                device     = wine_server_get_ptr( reply->user_ptr );
                client_tid = reply->client_tid;
                client_pid = reply->client_pid;
                in_size    = reply->in_size;
                out_size   = reply->out_size;
            }
            else
            {
                ioctl = 0; /* no previous ioctl */
                out_size = 0;
                in_size = reply->in_size;
            }
        }
        SERVER_END_REQ;

        switch(status)
        {
        case STATUS_SUCCESS:
            HeapFree( GetProcessHeap(), 0, out_buff );
            if (out_size) out_buff = HeapAlloc( GetProcessHeap(), 0, out_size );
            else out_buff = NULL;
            status = process_ioctl( device, code, in_buff, in_size, out_buff, &out_size );
            break;
        case STATUS_BUFFER_OVERFLOW:
            HeapFree( GetProcessHeap(), 0, in_buff );
            in_buff = HeapAlloc( GetProcessHeap(), 0, in_size );
            /* restart with larger buffer */
            break;
        case STATUS_PENDING:
            if (WaitForMultipleObjects( 2, handles, FALSE, INFINITE ) == WAIT_OBJECT_0)
            {
                HeapFree( GetProcessHeap(), 0, in_buff );
                HeapFree( GetProcessHeap(), 0, out_buff );
                return STATUS_SUCCESS;
            }
            break;
        }
    }
}


/***********************************************************************
 *           IoAcquireCancelSpinLock  (NTOSKRNL.EXE.@)
 */
void WINAPI IoAcquireCancelSpinLock(PKIRQL irql)
{
    FIXME("(%p): stub\n", irql);
}


/***********************************************************************
 *           IoReleaseCancelSpinLock  (NTOSKRNL.EXE.@)
 */
void WINAPI IoReleaseCancelSpinLock(PKIRQL irql)
{
    FIXME("(%p): stub\n", irql);
}


/***********************************************************************
 *           IoAllocateDriverObjectExtension  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoAllocateDriverObjectExtension( PDRIVER_OBJECT DriverObject,
                                                 PVOID ClientIdentificationAddress,
                                                 ULONG DriverObjectExtensionSize,
                                                 PVOID *DriverObjectExtension )
{
    FIXME( "stub: %p, %p, %u, %p\n", DriverObject, ClientIdentificationAddress,
            DriverObjectExtensionSize, DriverObjectExtension );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoGetDriverObjectExtension  (NTOSKRNL.EXE.@)
 */
PVOID WINAPI IoGetDriverObjectExtension( PDRIVER_OBJECT DriverObject,
                                         PVOID ClientIdentificationAddress )
{
    FIXME( "stub: %p, %p\n", DriverObject, ClientIdentificationAddress );
    return NULL;
}


/***********************************************************************
 *           IoInitializeIrp  (NTOSKRNL.EXE.@)
 */
void WINAPI IoInitializeIrp( IRP *irp, USHORT size, CCHAR stack_size )
{
    TRACE( "%p, %u, %d\n", irp, size, stack_size );

    RtlZeroMemory( irp, size );

    irp->Type = IO_TYPE_IRP;
    irp->Size = size;
    InitializeListHead( &irp->ThreadListEntry );
    irp->StackCount = stack_size;
    irp->CurrentLocation = stack_size + 1;
    irp->Tail.Overlay.s.u2.CurrentStackLocation =
            (PIO_STACK_LOCATION)(irp + 1) + stack_size;
}


/***********************************************************************
 *           IoInitializeTimer   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoInitializeTimer(PDEVICE_OBJECT DeviceObject,
                                  PIO_TIMER_ROUTINE TimerRoutine,
                                  PVOID Context)
{
    FIXME( "stub: %p, %p, %p\n", DeviceObject, TimerRoutine, Context );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoStartTimer   (NTOSKRNL.EXE.@)
 */
void WINAPI IoStartTimer(PDEVICE_OBJECT DeviceObject)
{
    FIXME( "stub: %p\n", DeviceObject );
}


/***********************************************************************
 *           IoAllocateIrp  (NTOSKRNL.EXE.@)
 */
PIRP WINAPI IoAllocateIrp( CCHAR stack_size, BOOLEAN charge_quota )
{
    SIZE_T size;
    PIRP irp;

    TRACE( "%d, %d\n", stack_size, charge_quota );

    size = sizeof(IRP) + stack_size * sizeof(IO_STACK_LOCATION);
    irp = ExAllocatePool( NonPagedPool, size );
    if (irp == NULL)
        return NULL;
    IoInitializeIrp( irp, size, stack_size );
    irp->AllocationFlags = IRP_ALLOCATED_FIXED_SIZE;
    if (charge_quota)
        irp->AllocationFlags |= IRP_LOOKASIDE_ALLOCATION;
    return irp;
}


/***********************************************************************
 *           IoFreeIrp  (NTOSKRNL.EXE.@)
 */
void WINAPI IoFreeIrp( IRP *irp )
{
    TRACE( "%p\n", irp );

    ExFreePool( irp );
}


/***********************************************************************
 *           IoAllocateErrorLogEntry  (NTOSKRNL.EXE.@)
 */
PVOID WINAPI IoAllocateErrorLogEntry( PVOID IoObject, UCHAR EntrySize )
{
    FIXME( "stub: %p, %u\n", IoObject, EntrySize );
    return NULL;
}


/***********************************************************************
 *           IoAllocateMdl  (NTOSKRNL.EXE.@)
 */
PMDL WINAPI IoAllocateMdl( PVOID VirtualAddress, ULONG Length, BOOLEAN SecondaryBuffer, BOOLEAN ChargeQuota, PIRP Irp )
{
    PMDL mdl;
    ULONG_PTR address = (ULONG_PTR)VirtualAddress;
    ULONG_PTR page_address;
    SIZE_T nb_pages, mdl_size;

    TRACE("(%p, %u, %i, %i, %p)\n", VirtualAddress, Length, SecondaryBuffer, ChargeQuota, Irp);

    if (Irp)
        FIXME("Attaching the MDL to an IRP is not yet supported\n");

    if (ChargeQuota)
        FIXME("Charge quota is not yet supported\n");

    /* FIXME: We suppose that page size is 4096 */
    page_address = address & ~(4096 - 1);
    nb_pages = (((address + Length - 1) & ~(4096 - 1)) - page_address) / 4096 + 1;

    mdl_size = sizeof(MDL) + nb_pages * sizeof(PVOID);

    mdl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mdl_size);
    if (!mdl)
        return NULL;

    mdl->Size = mdl_size;
    mdl->Process = IoGetCurrentProcess();
    mdl->StartVa = (PVOID)page_address;
    mdl->ByteCount = Length;
    mdl->ByteOffset = address - page_address;

    return mdl;
}


/***********************************************************************
 *           IoFreeMdl  (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoFreeMdl(PMDL mdl)
{
    FIXME("partial stub: %p\n", mdl);

    HeapFree(GetProcessHeap(), 0, mdl);
}


/***********************************************************************
 *           IoAllocateWorkItem  (NTOSKRNL.EXE.@)
 */
PIO_WORKITEM WINAPI IoAllocateWorkItem( PDEVICE_OBJECT DeviceObject )
{
    FIXME( "stub: %p\n", DeviceObject );
    return NULL;
}


/***********************************************************************
 *           IoAttachDeviceToDeviceStack  (NTOSKRNL.EXE.@)
 */
PDEVICE_OBJECT WINAPI IoAttachDeviceToDeviceStack( DEVICE_OBJECT *source,
                                                   DEVICE_OBJECT *target )
{
    TRACE( "%p, %p\n", source, target );
    target->AttachedDevice = source;
    source->StackSize = target->StackSize + 1;
    return target;
}


/***********************************************************************
 *           IoBuildDeviceIoControlRequest  (NTOSKRNL.EXE.@)
 */
PIRP WINAPI IoBuildDeviceIoControlRequest( ULONG IoControlCode,
                                           PDEVICE_OBJECT DeviceObject,
                                           PVOID InputBuffer,
                                           ULONG InputBufferLength,
                                           PVOID OutputBuffer,
                                           ULONG OutputBufferLength,
                                           BOOLEAN InternalDeviceIoControl,
                                           PKEVENT Event,
                                           PIO_STATUS_BLOCK IoStatusBlock )
{
    PIRP irp;
    PIO_STACK_LOCATION irpsp;
    struct IrpInstance *instance;

    TRACE( "%x, %p, %p, %u, %p, %u, %u, %p, %p\n",
           IoControlCode, DeviceObject, InputBuffer, InputBufferLength,
           OutputBuffer, OutputBufferLength, InternalDeviceIoControl,
           Event, IoStatusBlock );

    if (DeviceObject == NULL)
        return NULL;

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE );
    if (irp == NULL)
        return NULL;

    instance = HeapAlloc( GetProcessHeap(), 0, sizeof(struct IrpInstance) );
    if (instance == NULL)
    {
        IoFreeIrp( irp );
        return NULL;
    }
    instance->irp = irp;
    list_add_tail( &Irps, &instance->entry );

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = InternalDeviceIoControl ?
            IRP_MJ_INTERNAL_DEVICE_CONTROL : IRP_MJ_DEVICE_CONTROL;
    irpsp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    irp->UserIosb = IoStatusBlock;
    irp->UserEvent = Event;

    return irp;
}


/***********************************************************************
 *           IoCreateDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDriver( UNICODE_STRING *name, PDRIVER_INITIALIZE init )
{
    DRIVER_OBJECT *driver;
    DRIVER_EXTENSION *extension;
    NTSTATUS status;

    TRACE("(%s, %p)\n", debugstr_us(name), init);

    if (!(driver = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    sizeof(*driver) + sizeof(*extension) )))
        return STATUS_NO_MEMORY;

    if ((status = RtlDuplicateUnicodeString( 1, name, &driver->DriverName )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, driver );
        return status;
    }

    extension = (DRIVER_EXTENSION *)(driver + 1);
    driver->Size            = sizeof(*driver);
    driver->DriverInit      = init;
    driver->DriverExtension = extension;
    extension->DriverObject   = driver;
    extension->ServiceKeyName = driver->DriverName;

    status = driver->DriverInit( driver, name );

    if (status)
    {
        RtlFreeUnicodeString( &driver->DriverName );
        RtlFreeHeap( GetProcessHeap(), 0, driver );
    }
    return status;
}


/***********************************************************************
 *           IoDeleteDriver   (NTOSKRNL.EXE.@)
 */
void WINAPI IoDeleteDriver( DRIVER_OBJECT *driver )
{
    TRACE("(%p)\n", driver);

    RtlFreeUnicodeString( &driver->DriverName );
    RtlFreeHeap( GetProcessHeap(), 0, driver );
}


/***********************************************************************
 *           IoCreateDevice   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDevice( DRIVER_OBJECT *driver, ULONG ext_size,
                                UNICODE_STRING *name, DEVICE_TYPE type,
                                ULONG characteristics, BOOLEAN exclusive,
                                DEVICE_OBJECT **ret_device )
{
    NTSTATUS status;
    DEVICE_OBJECT *device;
    HANDLE handle = 0;
    HANDLE manager = get_device_manager();

    TRACE( "(%p, %u, %s, %u, %x, %u, %p)\n",
           driver, ext_size, debugstr_us(name), type, characteristics, exclusive, ret_device );

    if (!(device = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*device) + ext_size )))
        return STATUS_NO_MEMORY;

    SERVER_START_REQ( create_device )
    {
        req->access     = 0;
        req->attributes = 0;
        req->rootdir    = 0;
        req->manager    = wine_server_obj_handle( manager );
        req->user_ptr   = wine_server_client_ptr( device );
        if (name) wine_server_add_data( req, name->Buffer, name->Length );
        if (!(status = wine_server_call( req ))) handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (status == STATUS_SUCCESS)
    {
        device->DriverObject    = driver;
        device->DeviceExtension = device + 1;
        device->DeviceType      = type;
        device->StackSize       = 1;
        device->Reserved        = handle;

        device->NextDevice   = driver->DeviceObject;
        driver->DeviceObject = device;

        *ret_device = device;
    }
    else HeapFree( GetProcessHeap(), 0, device );

    return status;
}


/***********************************************************************
 *           IoDeleteDevice   (NTOSKRNL.EXE.@)
 */
void WINAPI IoDeleteDevice( DEVICE_OBJECT *device )
{
    NTSTATUS status;

    TRACE( "%p\n", device );

    SERVER_START_REQ( delete_device )
    {
        req->handle = wine_server_obj_handle( device->Reserved );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status == STATUS_SUCCESS)
    {
        DEVICE_OBJECT **prev = &device->DriverObject->DeviceObject;
        while (*prev && *prev != device) prev = &(*prev)->NextDevice;
        if (*prev) *prev = (*prev)->NextDevice;
        NtClose( device->Reserved );
        HeapFree( GetProcessHeap(), 0, device );
    }
}


/***********************************************************************
 *           IoCreateSymbolicLink   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateSymbolicLink( UNICODE_STRING *name, UNICODE_STRING *target )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    TRACE( "%s -> %s\n", debugstr_us(name), debugstr_us(target) );
    /* FIXME: store handle somewhere */
    return NtCreateSymbolicLinkObject( &handle, SYMBOLIC_LINK_ALL_ACCESS, &attr, target );
}


/***********************************************************************
 *           IoDeleteSymbolicLink   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoDeleteSymbolicLink( UNICODE_STRING *name )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    if (!(status = NtOpenSymbolicLinkObject( &handle, 0, &attr )))
    {
        SERVER_START_REQ( unlink_object )
        {
            req->handle = wine_server_obj_handle( handle );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        NtClose( handle );
    }
    return status;
}


/***********************************************************************
 *           IoGetDeviceInterfaces   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoGetDeviceInterfaces( const GUID *InterfaceClassGuid,
                                       PDEVICE_OBJECT PhysicalDeviceObject,
                                       ULONG Flags, PWSTR *SymbolicLinkList )
{
    FIXME( "stub: %s %p %x %p\n", debugstr_guid(InterfaceClassGuid),
           PhysicalDeviceObject, Flags, SymbolicLinkList );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoGetDeviceObjectPointer   (NTOSKRNL.EXE.@)
 */
NTSTATUS  WINAPI IoGetDeviceObjectPointer( UNICODE_STRING *name, ACCESS_MASK access, PFILE_OBJECT *file, PDEVICE_OBJECT *device )
{
    FIXME( "stub: %s %x %p %p\n", debugstr_us(name), access, file, device );

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoGetDeviceProperty   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoGetDeviceProperty( DEVICE_OBJECT *device, DEVICE_REGISTRY_PROPERTY device_property,
                                     ULONG buffer_length, PVOID property_buffer, PULONG result_length )
{
    FIXME( "%p %d %u %p %p: stub\n", device, device_property, buffer_length,
           property_buffer, result_length );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoCallDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCallDriver( DEVICE_OBJECT *device, IRP *irp )
{
    PDRIVER_DISPATCH dispatch;
    IO_STACK_LOCATION *irpsp;
    NTSTATUS status;

    TRACE( "%p %p\n", device, irp );

    --irp->CurrentLocation;
    irpsp = --irp->Tail.Overlay.s.u2.CurrentStackLocation;
    dispatch = device->DriverObject->MajorFunction[irpsp->MajorFunction];
    status = dispatch( device, irp );

    return status;
}


/***********************************************************************
 *           IofCallDriver   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( IofCallDriver )
NTSTATUS WINAPI __regs_IofCallDriver( DEVICE_OBJECT *device, IRP *irp )
#else
NTSTATUS WINAPI IofCallDriver( DEVICE_OBJECT *device, IRP *irp )
#endif
{
    TRACE( "%p %p\n", device, irp );
    return IoCallDriver( device, irp );
}


/***********************************************************************
 *           IoGetRelatedDeviceObject    (NTOSKRNL.EXE.@)
 */
PDEVICE_OBJECT WINAPI IoGetRelatedDeviceObject( PFILE_OBJECT obj )
{
    FIXME( "stub: %p\n", obj );
    return NULL;
}

static CONFIGURATION_INFORMATION configuration_information;

/***********************************************************************
 *           IoGetConfigurationInformation    (NTOSKRNL.EXE.@)
 */
PCONFIGURATION_INFORMATION WINAPI IoGetConfigurationInformation(void)
{
    FIXME( "partial stub\n" );
    /* FIXME: return actual devices on system */
    return &configuration_information;
}


/***********************************************************************
 *           IoIsWdmVersionAvailable     (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoIsWdmVersionAvailable(UCHAR MajorVersion, UCHAR MinorVersion)
{
    DWORD version;
    DWORD major;
    DWORD minor;

    TRACE( "%d, 0x%X\n", MajorVersion, MinorVersion );

    version = GetVersion();
    major = LOBYTE(version);
    minor = HIBYTE(LOWORD(version));

    if (MajorVersion == 6 && MinorVersion == 0)
    {
        /* Windows Vista, Windows Server 2008, Windows 7 */
    }
    else if (MajorVersion == 1)
    {
        if (MinorVersion == 0x30)
        {
            /* Windows server 2003 */
            MajorVersion = 6;
            MinorVersion = 0;
        }
        else if (MinorVersion == 0x20)
        {
            /* Windows XP */
            MajorVersion = 5;
            MinorVersion = 1;
        }
        else if (MinorVersion == 0x10)
        {
            /* Windows 2000 */
            MajorVersion = 5;
            MinorVersion = 0;
        }
        else if (MinorVersion == 0x05)
        {
            /* Windows ME */
            MajorVersion = 4;
            MinorVersion = 0x5a;
        }
        else if (MinorVersion == 0x00)
        {
            /* Windows 98 */
            MajorVersion = 4;
            MinorVersion = 0x0a;
        }
        else
        {
            FIXME( "unknown major %d minor 0x%X\n", MajorVersion, MinorVersion );
            return FALSE;
        }
    }
    else
    {
        FIXME( "unknown major %d minor 0x%X\n", MajorVersion, MinorVersion );
        return FALSE;
    }
    return major > MajorVersion || (major == MajorVersion && minor >= MinorVersion);
}


/***********************************************************************
 *           IoQueryDeviceDescription    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoQueryDeviceDescription(PINTERFACE_TYPE itype, PULONG bus, PCONFIGURATION_TYPE ctype,
                                     PULONG cnum, PCONFIGURATION_TYPE ptype, PULONG pnum,
                                     PIO_QUERY_DEVICE_ROUTINE callout, PVOID context)
{
    FIXME( "(%p %p %p %p %p %p %p %p)\n", itype, bus, ctype, cnum, ptype, pnum, callout, context);
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoRegisterDriverReinitialization    (NTOSKRNL.EXE.@)
 */
void WINAPI IoRegisterDriverReinitialization( PDRIVER_OBJECT obj, PDRIVER_REINITIALIZE reinit, PVOID context )
{
    FIXME( "stub: %p %p %p\n", obj, reinit, context );
}


/***********************************************************************
 *           IoRegisterShutdownNotification    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoRegisterShutdownNotification( PDEVICE_OBJECT obj )
{
    FIXME( "stub: %p\n", obj );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           IoUnregisterShutdownNotification    (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoUnregisterShutdownNotification( PDEVICE_OBJECT obj )
{
    FIXME( "stub: %p\n", obj );
}


/***********************************************************************
 *           IoReportResourceUsage    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoReportResourceUsage(PUNICODE_STRING name, PDRIVER_OBJECT drv_obj, PCM_RESOURCE_LIST drv_list,
                                      ULONG drv_size, PDRIVER_OBJECT dev_obj, PCM_RESOURCE_LIST dev_list,
                                      ULONG dev_size, BOOLEAN overwrite, PBOOLEAN detected)
{
    FIXME("(%s %p %p %u %p %p %u %d %p) stub\n", debugstr_w(name? name->Buffer : NULL),
          drv_obj, drv_list, drv_size, dev_obj, dev_list, dev_size, overwrite, detected);
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoCompleteRequest   (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoCompleteRequest( IRP *irp, UCHAR priority_boost )
{
    IO_STACK_LOCATION *irpsp;
    PIO_COMPLETION_ROUTINE routine;
    IO_STATUS_BLOCK *iosb;
    struct IrpInstance *instance;
    NTSTATUS status, stat;
    int call_flag = 0;

    TRACE( "%p %u\n", irp, priority_boost );

    iosb = irp->UserIosb;
    status = irp->IoStatus.u.Status;
    while (irp->CurrentLocation <= irp->StackCount)
    {
        irpsp = irp->Tail.Overlay.s.u2.CurrentStackLocation;
        routine = irpsp->CompletionRoutine;
        call_flag = 0;
        /* FIXME: add SL_INVOKE_ON_CANCEL support */
        if (routine)
        {
            if ((irpsp->Control & SL_INVOKE_ON_SUCCESS) && STATUS_SUCCESS == status)
                call_flag = 1;
            if ((irpsp->Control & SL_INVOKE_ON_ERROR) && STATUS_SUCCESS != status)
                call_flag = 1;
        }
        ++irp->CurrentLocation;
        ++irp->Tail.Overlay.s.u2.CurrentStackLocation;
        if (call_flag)
        {
            TRACE( "calling %p( %p, %p, %p )\n", routine,
                    irpsp->DeviceObject, irp, irpsp->Context );
            stat = routine( irpsp->DeviceObject, irp, irpsp->Context );
            TRACE( "CompletionRoutine returned %x\n", stat );
            if (STATUS_MORE_PROCESSING_REQUIRED == stat)
                return;
        }
    }
    if (iosb && STATUS_SUCCESS == status)
    {
        iosb->u.Status = irp->IoStatus.u.Status;
        iosb->Information = irp->IoStatus.Information;
    }
    LIST_FOR_EACH_ENTRY( instance, &Irps, struct IrpInstance, entry )
    {
        if (instance->irp == irp)
        {
            list_remove( &instance->entry );
            HeapFree( GetProcessHeap(), 0, instance );
            IoFreeIrp( irp );
            break;
        }
    }
}


/***********************************************************************
 *           IofCompleteRequest   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( IofCompleteRequest )
void WINAPI __regs_IofCompleteRequest( IRP *irp, UCHAR priority_boost )
#else
void WINAPI IofCompleteRequest( IRP *irp, UCHAR priority_boost )
#endif
{
    TRACE( "%p %u\n", irp, priority_boost );
    IoCompleteRequest( irp, priority_boost );
}


/***********************************************************************
 *           InterlockedCompareExchange   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL3_ENTRYPOINT
DEFINE_FASTCALL3_ENTRYPOINT( NTOSKRNL_InterlockedCompareExchange )
LONG WINAPI __regs_NTOSKRNL_InterlockedCompareExchange( LONG volatile *dest, LONG xchg, LONG compare )
#else
LONG WINAPI NTOSKRNL_InterlockedCompareExchange( LONG volatile *dest, LONG xchg, LONG compare )
#endif
{
    return InterlockedCompareExchange( dest, xchg, compare );
}


/***********************************************************************
 *           InterlockedDecrement   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( NTOSKRNL_InterlockedDecrement )
LONG WINAPI __regs_NTOSKRNL_InterlockedDecrement( LONG volatile *dest )
#else
LONG WINAPI NTOSKRNL_InterlockedDecrement( LONG volatile *dest )
#endif
{
    return InterlockedDecrement( dest );
}


/***********************************************************************
 *           InterlockedExchange   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( NTOSKRNL_InterlockedExchange )
LONG WINAPI __regs_NTOSKRNL_InterlockedExchange( LONG volatile *dest, LONG val )
#else
LONG WINAPI NTOSKRNL_InterlockedExchange( LONG volatile *dest, LONG val )
#endif
{
    return InterlockedExchange( dest, val );
}


/***********************************************************************
 *           InterlockedExchangeAdd   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( NTOSKRNL_InterlockedExchangeAdd )
LONG WINAPI __regs_NTOSKRNL_InterlockedExchangeAdd( LONG volatile *dest, LONG incr )
#else
LONG WINAPI NTOSKRNL_InterlockedExchangeAdd( LONG volatile *dest, LONG incr )
#endif
{
    return InterlockedExchangeAdd( dest, incr );
}


/***********************************************************************
 *           InterlockedIncrement   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( NTOSKRNL_InterlockedIncrement )
LONG WINAPI __regs_NTOSKRNL_InterlockedIncrement( LONG volatile *dest )
#else
LONG WINAPI NTOSKRNL_InterlockedIncrement( LONG volatile *dest )
#endif
{
    return InterlockedIncrement( dest );
}


/***********************************************************************
 *           ExAllocatePool   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePool( POOL_TYPE type, SIZE_T size )
{
    return ExAllocatePoolWithTag( type, size, 0 );
}


/***********************************************************************
 *           ExAllocatePoolWithQuota   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithQuota( POOL_TYPE type, SIZE_T size )
{
    return ExAllocatePoolWithTag( type, size, 0 );
}


/***********************************************************************
 *           ExAllocatePoolWithTag   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithTag( POOL_TYPE type, SIZE_T size, ULONG tag )
{
    /* FIXME: handle page alignment constraints */
    void *ret = HeapAlloc( GetProcessHeap(), 0, size );
    TRACE( "%lu pool %u -> %p\n", size, type, ret );
    return ret;
}


/***********************************************************************
 *           ExAllocatePoolWithQuotaTag   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithQuotaTag( POOL_TYPE type, SIZE_T size, ULONG tag )
{
    return ExAllocatePoolWithTag( type, size, tag );
}


/***********************************************************************
 *           ExCreateCallback   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ExCreateCallback(PCALLBACK_OBJECT *obj, POBJECT_ATTRIBUTES attr,
                                 BOOLEAN create, BOOLEAN allow_multiple)
{
    FIXME("(%p, %p, %u, %u): stub\n", obj, attr, create, allow_multiple);

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           ExFreePool   (NTOSKRNL.EXE.@)
 */
void WINAPI ExFreePool( void *ptr )
{
    ExFreePoolWithTag( ptr, 0 );
}


/***********************************************************************
 *           ExFreePoolWithTag   (NTOSKRNL.EXE.@)
 */
void WINAPI ExFreePoolWithTag( void *ptr, ULONG tag )
{
    TRACE( "%p\n", ptr );
    HeapFree( GetProcessHeap(), 0, ptr );
}


/***********************************************************************
 *           ExInitializeResourceLite   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ExInitializeResourceLite(PERESOURCE Resource)
{
    FIXME( "stub: %p\n", Resource );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           ExInitializeNPagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST Lookaside,
                                            PALLOCATE_FUNCTION Allocate,
                                            PFREE_FUNCTION Free,
                                            ULONG Flags,
                                            SIZE_T Size,
                                            ULONG Tag,
                                            USHORT Depth)
{
    FIXME( "stub: %p, %p, %p, %u, %lu, %u, %u\n", Lookaside, Allocate, Free, Flags, Size, Tag, Depth );
}

/***********************************************************************
 *           ExInitializePagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExInitializePagedLookasideList(PPAGED_LOOKASIDE_LIST Lookaside,
                                           PALLOCATE_FUNCTION Allocate,
                                           PFREE_FUNCTION Free,
                                           ULONG Flags,
                                           SIZE_T Size,
                                           ULONG Tag,
                                           USHORT Depth)
{
    FIXME( "stub: %p, %p, %p, %u, %lu, %u, %u\n", Lookaside, Allocate, Free, Flags, Size, Tag, Depth );
}

/***********************************************************************
 *           ExInitializeZone   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ExInitializeZone(PZONE_HEADER Zone,
                                 ULONG BlockSize,
                                 PVOID InitialSegment,
                                 ULONG InitialSegmentSize)
{
    FIXME( "stub: %p, %u, %p, %u\n", Zone, BlockSize, InitialSegment, InitialSegmentSize );
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
*           FsRtlRegisterUncProvider   (NTOSKRNL.EXE.@)
*/
NTSTATUS WINAPI FsRtlRegisterUncProvider(PHANDLE MupHandle, PUNICODE_STRING RedirDevName,
                                         BOOLEAN MailslotsSupported)
{
    FIXME("(%p %p %d): stub\n", MupHandle, RedirDevName, MailslotsSupported);
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           IoGetCurrentProcess / PsGetCurrentProcess   (NTOSKRNL.EXE.@)
 */
PEPROCESS WINAPI IoGetCurrentProcess(void)
{
    FIXME("() stub\n");
    return NULL;
}

/***********************************************************************
 *           KeGetCurrentThread / PsGetCurrentThread   (NTOSKRNL.EXE.@)
 */
PRKTHREAD WINAPI KeGetCurrentThread(void)
{
    FIXME("() stub\n");
    return NULL;
}

/***********************************************************************
 *           KeInitializeEvent   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeEvent( PRKEVENT Event, EVENT_TYPE Type, BOOLEAN State )
{
    FIXME( "stub: %p %d %d\n", Event, Type, State );
}


 /***********************************************************************
 *           KeInitializeMutex   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeMutex(PRKMUTEX Mutex, ULONG Level)
{
    FIXME( "stub: %p, %u\n", Mutex, Level );
}


 /***********************************************************************
 *           KeWaitForMutexObject   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeWaitForMutexObject(PRKMUTEX Mutex, KWAIT_REASON WaitReason, KPROCESSOR_MODE WaitMode,
                                     BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
    FIXME( "stub: %p, %d, %d, %d, %p\n", Mutex, WaitReason, WaitMode, Alertable, Timeout );
    return STATUS_NOT_IMPLEMENTED;
}


 /***********************************************************************
 *           KeReleaseMutex   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeReleaseMutex(PRKMUTEX Mutex, BOOLEAN Wait)
{
    FIXME( "stub: %p, %d\n", Mutex, Wait );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           KeInitializeSemaphore   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeSemaphore( PRKSEMAPHORE Semaphore, LONG Count, LONG Limit )
{
    FIXME( "(%p %d %d) stub\n", Semaphore , Count, Limit );
}


/***********************************************************************
 *           KeInitializeSpinLock   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeSpinLock( PKSPIN_LOCK SpinLock )
{
    FIXME( "stub: %p\n", SpinLock );
}


/***********************************************************************
 *           KeInitializeTimerEx   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeTimerEx( PKTIMER Timer, TIMER_TYPE Type )
{
    FIXME( "stub: %p %d\n", Timer, Type );
}


/***********************************************************************
 *           KeInitializeTimer   (NTOSKRNL.EXE.@)
 */
void WINAPI KeInitializeTimer( PKTIMER Timer )
{
    KeInitializeTimerEx(Timer, NotificationTimer);
}


/**********************************************************************
 *           KeQueryActiveProcessors   (NTOSKRNL.EXE.@)
 *
 * Return the active Processors as bitmask
 *
 * RETURNS
 *   active Processors as bitmask
 *
 */
KAFFINITY WINAPI KeQueryActiveProcessors( void )
{
    DWORD_PTR AffinityMask;

    GetProcessAffinityMask( GetCurrentProcess(), &AffinityMask, NULL);
    return AffinityMask;
}


/**********************************************************************
 *           KeQueryInterruptTime   (NTOSKRNL.EXE.@)
 *
 * Return the interrupt time count
 *
 */
ULONGLONG WINAPI KeQueryInterruptTime( void )
{
    LARGE_INTEGER totaltime;

    KeQueryTickCount(&totaltime);
    return totaltime.QuadPart;
}


/***********************************************************************
 *           KeQuerySystemTime   (NTOSKRNL.EXE.@)
 */
void WINAPI KeQuerySystemTime( LARGE_INTEGER *time )
{
    NtQuerySystemTime( time );
}


/***********************************************************************
 *           KeQueryTickCount   (NTOSKRNL.EXE.@)
 */
void WINAPI KeQueryTickCount( LARGE_INTEGER *count )
{
    count->QuadPart = NtGetTickCount();
    /* update the global variable too */
    KeTickCount.LowPart   = count->u.LowPart;
    KeTickCount.High1Time = count->u.HighPart;
    KeTickCount.High2Time = count->u.HighPart;
}


/***********************************************************************
 *           KeReleaseSemaphore   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeReleaseSemaphore( PRKSEMAPHORE Semaphore, KPRIORITY Increment,
                                LONG Adjustment, BOOLEAN Wait )
{
    FIXME("(%p %d %d %d) stub\n", Semaphore, Increment, Adjustment, Wait );
    return 0;
}


/***********************************************************************
 *           KeQueryTimeIncrement   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI KeQueryTimeIncrement(void)
{
    return 10000;
}


/***********************************************************************
 *           KeResetEvent   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeResetEvent( PRKEVENT Event )
{
    FIXME("(%p): stub\n", Event);
    return 0;
}


/***********************************************************************
 *           KeSetEvent   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeSetEvent( PRKEVENT Event, KPRIORITY Increment, BOOLEAN Wait )
{
    FIXME("(%p, %d, %d): stub\n", Event, Increment, Wait);
    return 0;
}


/***********************************************************************
 *           KeSetPriorityThread   (NTOSKRNL.EXE.@)
 */
KPRIORITY WINAPI KeSetPriorityThread( PKTHREAD Thread, KPRIORITY Priority )
{
    FIXME("(%p %d)\n", Thread, Priority);
    return Priority;
}


/***********************************************************************
 *           KeWaitForSingleObject   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeWaitForSingleObject(PVOID Object,
                                      KWAIT_REASON WaitReason,
                                      KPROCESSOR_MODE WaitMode,
                                      BOOLEAN Alertable,
                                      PLARGE_INTEGER Timeout)
{
    FIXME( "stub: %p, %d, %d, %d, %p\n", Object, WaitReason, WaitMode, Alertable, Timeout );
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           IoRegisterFileSystem   (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
    FIXME("(%p): stub\n", DeviceObject);
}

/***********************************************************************
*           IoUnregisterFileSystem   (NTOSKRNL.EXE.@)
*/
VOID WINAPI IoUnregisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
    FIXME("(%p): stub\n", DeviceObject);
}

/***********************************************************************
 *           MmAllocateNonCachedMemory   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmAllocateNonCachedMemory( SIZE_T size )
{
    TRACE( "%lu\n", size );
    return VirtualAlloc( NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE|PAGE_NOCACHE );
}

/***********************************************************************
 *           MmAllocateContiguousMemory   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmAllocateContiguousMemory( SIZE_T size, PHYSICAL_ADDRESS highest_valid_address )
{
    FIXME( "%lu, %s stub\n", size, wine_dbgstr_longlong(highest_valid_address.QuadPart) );
    return NULL;
}

/***********************************************************************
 *           MmAllocateContiguousMemorySpecifyCache   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmAllocateContiguousMemorySpecifyCache( SIZE_T size,
                                                     PHYSICAL_ADDRESS lowest_valid_address,
                                                     PHYSICAL_ADDRESS highest_valid_address,
                                                     PHYSICAL_ADDRESS BoundaryAddressMultiple,
                                                     MEMORY_CACHING_TYPE CacheType )
{
    FIXME(": stub\n");
    return NULL;
}

/***********************************************************************
 *           MmAllocatePagesForMdl   (NTOSKRNL.EXE.@)
 */
PMDL WINAPI MmAllocatePagesForMdl(PHYSICAL_ADDRESS lowaddress, PHYSICAL_ADDRESS highaddress,
                                  PHYSICAL_ADDRESS skipbytes, SIZE_T size)
{
    FIXME("%s %s %s %lu: stub\n", wine_dbgstr_longlong(lowaddress.QuadPart), wine_dbgstr_longlong(highaddress.QuadPart),
                                  wine_dbgstr_longlong(skipbytes.QuadPart), size);
    return NULL;
}

/***********************************************************************
 *           MmFreeNonCachedMemory   (NTOSKRNL.EXE.@)
 */
void WINAPI MmFreeNonCachedMemory( void *addr, SIZE_T size )
{
    TRACE( "%p %lu\n", addr, size );
    VirtualFree( addr, 0, MEM_RELEASE );
}

/***********************************************************************
 *           MmIsAddressValid   (NTOSKRNL.EXE.@)
 *
 * Check if the process can access the virtual address without a pagefault
 *
 * PARAMS
 *  VirtualAddress [I] Address to check
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE  (Accessing the Address works without a Pagefault)
 *
 */
BOOLEAN WINAPI MmIsAddressValid(PVOID VirtualAddress)
{
    TRACE("(%p)\n", VirtualAddress);
    return !IsBadWritePtr(VirtualAddress, 1);
}

/***********************************************************************
 *           MmMapIoSpace   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmMapIoSpace( PHYSICAL_ADDRESS PhysicalAddress, DWORD NumberOfBytes, DWORD CacheType )
{
    FIXME( "stub: 0x%08x%08x, %d, %d\n", PhysicalAddress.u.HighPart, PhysicalAddress.u.LowPart, NumberOfBytes, CacheType );
    return NULL;
}


/***********************************************************************
 *           MmMapLockedPagesSpecifyCache  (NTOSKRNL.EXE.@)
 */
PVOID WINAPI  MmMapLockedPagesSpecifyCache(PMDLX MemoryDescriptorList, KPROCESSOR_MODE AccessMode, MEMORY_CACHING_TYPE CacheType,
                                           PVOID BaseAddress, ULONG BugCheckOnFailure, MM_PAGE_PRIORITY Priority)
{
    FIXME("(%p, %u, %u, %p, %u, %u): stub\n", MemoryDescriptorList, AccessMode, CacheType, BaseAddress, BugCheckOnFailure, Priority);

    return NULL;
}


/***********************************************************************
 *           MmPageEntireDriver   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmPageEntireDriver(PVOID AddrInSection)
{
    TRACE("%p\n", AddrInSection);
    return AddrInSection;
}


/***********************************************************************
 *           MmProbeAndLockPages  (NTOSKRNL.EXE.@)
 */
void WINAPI MmProbeAndLockPages(PMDLX MemoryDescriptorList, KPROCESSOR_MODE AccessMode, LOCK_OPERATION Operation)
{
    FIXME("(%p, %u, %u): stub\n", MemoryDescriptorList, AccessMode, Operation);
}


/***********************************************************************
 *           MmResetDriverPaging   (NTOSKRNL.EXE.@)
 */
void WINAPI MmResetDriverPaging(PVOID AddrInSection)
{
    TRACE("%p\n", AddrInSection);
}


/***********************************************************************
 *           MmUnlockPages  (NTOSKRNL.EXE.@)
 */
void WINAPI  MmUnlockPages(PMDLX MemoryDescriptorList)
{
    FIXME("(%p): stub\n", MemoryDescriptorList);
}


/***********************************************************************
 *           MmUnmapIoSpace   (NTOSKRNL.EXE.@)
 */
VOID WINAPI MmUnmapIoSpace( PVOID BaseAddress, SIZE_T NumberOfBytes )
{
    FIXME( "stub: %p, %lu\n", BaseAddress, NumberOfBytes );
}

/***********************************************************************
 *           ObfReferenceObject   (NTOSKRNL.EXE.@)
 */
VOID WINAPI ObfReferenceObject(PVOID Object)
{
    FIXME("(%p): stub\n", Object);
}

 /***********************************************************************
 *           ObReferenceObjectByHandle    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObReferenceObjectByHandle( HANDLE obj, ACCESS_MASK access,
                                           POBJECT_TYPE type,
                                           KPROCESSOR_MODE mode, PVOID* ptr,
                                           POBJECT_HANDLE_INFORMATION info)
{
    FIXME( "stub: %p %x %p %d %p %p\n", obj, access, type, mode, ptr, info);
    return STATUS_NOT_IMPLEMENTED;
}

 /***********************************************************************
 *           ObReferenceObjectByName    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObReferenceObjectByName( UNICODE_STRING *ObjectName,
                                         ULONG Attributes,
                                         ACCESS_STATE *AccessState,
                                         ACCESS_MASK DesiredAccess,
                                         POBJECT_TYPE ObjectType,
                                         KPROCESSOR_MODE AccessMode,
                                         void *ParseContext,
                                         void **Object)
{
    FIXME("stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           ObfDereferenceObject   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( ObfDereferenceObject )
void WINAPI __regs_ObfDereferenceObject( VOID *obj )
#else
void WINAPI ObfDereferenceObject( VOID *obj )
#endif
{
    FIXME( "stub: %p\n", obj );
}


/***********************************************************************
 *           PsCreateSystemThread   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsCreateSystemThread(PHANDLE ThreadHandle, ULONG DesiredAccess,
				     POBJECT_ATTRIBUTES ObjectAttributes,
			             HANDLE ProcessHandle, PCLIENT_ID ClientId,
                                     PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
    if (!ProcessHandle) ProcessHandle = GetCurrentProcess();
    return RtlCreateUserThread(ProcessHandle, 0, FALSE, 0, 0,
                               0, StartRoutine, StartContext,
                               ThreadHandle, ClientId);
}

/***********************************************************************
 *           PsGetCurrentProcessId   (NTOSKRNL.EXE.@)
 */
HANDLE WINAPI PsGetCurrentProcessId(void)
{
    if (GetCurrentThreadId() == request_thread)
        return UlongToHandle(client_pid);
    return UlongToHandle(GetCurrentProcessId());
}


/***********************************************************************
 *           PsGetCurrentThreadId   (NTOSKRNL.EXE.@)
 */
HANDLE WINAPI PsGetCurrentThreadId(void)
{
    if (GetCurrentThreadId() == request_thread)
        return UlongToHandle(client_tid);
    return UlongToHandle(GetCurrentThreadId());
}


/***********************************************************************
 *           PsGetVersion   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI PsGetVersion(ULONG *major, ULONG *minor, ULONG *build, UNICODE_STRING *version )
{
    RTL_OSVERSIONINFOEXW info;

    info.dwOSVersionInfoSize = sizeof(info);
    RtlGetVersion( &info );
    if (major) *major = info.dwMajorVersion;
    if (minor) *minor = info.dwMinorVersion;
    if (build) *build = info.dwBuildNumber;

    if (version)
    {
#if 0  /* FIXME: GameGuard passes an uninitialized pointer in version->Buffer */
        size_t len = min( strlenW(info.szCSDVersion)*sizeof(WCHAR), version->MaximumLength );
        memcpy( version->Buffer, info.szCSDVersion, len );
        if (len < version->MaximumLength) version->Buffer[len / sizeof(WCHAR)] = 0;
        version->Length = len;
#endif
    }
    return TRUE;
}


/***********************************************************************
 *           PsImpersonateClient   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsImpersonateClient(PETHREAD Thread, PACCESS_TOKEN Token, BOOLEAN CopyOnOpen,
                                    BOOLEAN EffectiveOnly, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    FIXME("(%p, %p, %u, %u, %u): stub\n", Thread, Token, CopyOnOpen, EffectiveOnly, ImpersonationLevel);

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           PsSetCreateProcessNotifyRoutine   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetCreateProcessNotifyRoutine( PCREATE_PROCESS_NOTIFY_ROUTINE callback, BOOLEAN remove )
{
    FIXME( "stub: %p %d\n", callback, remove );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           PsSetCreateThreadNotifyRoutine   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetCreateThreadNotifyRoutine( PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine )
{
    FIXME( "stub: %p\n", NotifyRoutine );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           PsTerminateSystemThread   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsTerminateSystemThread(NTSTATUS ExitStatus)
{
    FIXME( "stub: %u\n", ExitStatus );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           MmGetSystemRoutineAddress   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmGetSystemRoutineAddress(PUNICODE_STRING SystemRoutineName)
{
    HMODULE hMod;
    STRING routineNameA;
    PVOID pFunc = NULL;

    static const WCHAR ntoskrnlW[] = {'n','t','o','s','k','r','n','l','.','e','x','e',0};
    static const WCHAR halW[] = {'h','a','l','.','d','l','l',0};

    if (!SystemRoutineName) return NULL;

    if (RtlUnicodeStringToAnsiString( &routineNameA, SystemRoutineName, TRUE ) == STATUS_SUCCESS)
    {
        /* We only support functions exported from ntoskrnl.exe or hal.dll */
        hMod = GetModuleHandleW( ntoskrnlW );
        pFunc = GetProcAddress( hMod, routineNameA.Buffer );
        if (!pFunc)
        {
           hMod = GetModuleHandleW( halW );
           if (hMod) pFunc = GetProcAddress( hMod, routineNameA.Buffer );
        }
        RtlFreeAnsiString( &routineNameA );
    }

    if (pFunc)
        TRACE( "%s -> %p\n", debugstr_us(SystemRoutineName), pFunc );
    else
        FIXME( "%s not found\n", debugstr_us(SystemRoutineName) );
    return pFunc;
}


/***********************************************************************
 *           MmQuerySystemSize   (NTOSKRNL.EXE.@)
 */
MM_SYSTEMSIZE WINAPI MmQuerySystemSize(void)
{
    FIXME("stub\n");
    return MmLargeSystem;
}

/***********************************************************************
 *           KeInitializeDpc   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeInitializeDpc(PRKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext)
{
    FIXME("stub\n");
}

/***********************************************************************
 *           READ_REGISTER_BUFFER_UCHAR   (NTOSKRNL.EXE.@)
 */
VOID WINAPI READ_REGISTER_BUFFER_UCHAR(PUCHAR Register, PUCHAR Buffer, ULONG Count)
{
    FIXME("stub\n");
}

/*****************************************************
 *           PoSetPowerState   (NTOSKRNL.EXE.@)
 */
POWER_STATE WINAPI PoSetPowerState(PDEVICE_OBJECT DeviceObject, POWER_STATE_TYPE Type, POWER_STATE State)
{
    FIXME("(%p %u %u) stub\n", DeviceObject, Type, State.DeviceState);
    return State;
}

/*****************************************************
 *           IoWMIRegistrationControl   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoWMIRegistrationControl(PDEVICE_OBJECT DeviceObject, ULONG Action)
{
    FIXME("(%p %u) stub\n", DeviceObject, Action);
    return STATUS_SUCCESS;
}

/*****************************************************
 *           PsSetLoadImageNotifyRoutine   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE routine)
{
    FIXME("(%p) stub\n", routine);
    return STATUS_SUCCESS;
}

/*****************************************************
 *           PsLookupProcessByProcessId  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsLookupProcessByProcessId(HANDLE processid, PEPROCESS *process)
{
    FIXME("(%p %p) stub\n", processid, process);
    return STATUS_NOT_IMPLEMENTED;
}


/*****************************************************
 *           IoSetThreadHardErrorMode  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI IoSetThreadHardErrorMode(BOOLEAN EnableHardErrors)
{
    FIXME("stub\n");
    return FALSE;
}


/*****************************************************
 *           IoInitializeRemoveLockEx  (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoInitializeRemoveLockEx(PIO_REMOVE_LOCK lock, ULONG tag,
                                     ULONG maxmin, ULONG high, ULONG size)
{
    FIXME("(%p %u %u %u %u) stub\n", lock, tag, maxmin, high, size);
}


/*****************************************************
 *           IoAcquireRemoveLockEx  (NTOSKRNL.EXE.@)
 */

NTSTATUS WINAPI IoAcquireRemoveLockEx(PIO_REMOVE_LOCK lock, PVOID tag,
                                      LPCSTR file, ULONG line, ULONG lock_size)
{
    FIXME("(%p, %p, %s, %u, %u): stub\n", lock, tag, debugstr_a(file), line, lock_size);

    return STATUS_NOT_IMPLEMENTED;
}


/*****************************************************
 *           DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    static void *handler;
    LARGE_INTEGER count;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
#ifdef __i386__
        handler = RtlAddVectoredExceptionHandler( TRUE, vectored_handler );
#endif
        KeQueryTickCount( &count );  /* initialize the global KeTickCount */
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        RtlRemoveVectoredExceptionHandler( handler );
        break;
    }
    return TRUE;
}

/*****************************************************
 *           Ke386IoSetAccessProcess  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI Ke386IoSetAccessProcess(PEPROCESS *process, ULONG flag)
{
    FIXME("(%p %d) stub\n", process, flag);
    return FALSE;
}

/*****************************************************
 *           Ke386SetIoAccessMap  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI Ke386SetIoAccessMap(ULONG flag, PVOID buffer)
{
    FIXME("(%d %p) stub\n", flag, buffer);
    return FALSE;
}

/*****************************************************
 *           IoCreateSynchronizationEvent (NTOSKRNL.EXE.@)
 */
PKEVENT WINAPI IoCreateSynchronizationEvent(PUNICODE_STRING name, PHANDLE handle)
{
    FIXME("(%p %p) stub\n", name, handle);
    return NULL;
}

/*****************************************************
 *           IoStartNextPacket  (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoStartNextPacket(PDEVICE_OBJECT deviceobject, BOOLEAN cancelable)
{
    FIXME("(%p %d) stub\n", deviceobject, cancelable);
}
