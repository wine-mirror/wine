/*
 * ntoskrnl.exe implementation
 *
 * Copyright (C) 2007 Alexandre Julliard
 * Copyright (C) 2010 Damjan Jovanovic
 * Copyright (C) 2016 Sebastian Lackner
 * Copyright (C) 2016 CodeWeavers, Aric Stewart
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
#include "winsvc.h"
#include "winternl.h"
#include "excpt.h"
#include "winioctl.h"
#include "ddk/csq.h"
#include "ddk/ntddk.h"
#include "ddk/ntifs.h"
#include "ddk/wdm.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "wine/debug.h"

#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(plugplay);

BOOLEAN KdDebuggerEnabled = FALSE;
ULONG InitSafeBootMode = 0;

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

static const WCHAR servicesW[] = {'\\','R','e','g','i','s','t','r','y',
                                  '\\','M','a','c','h','i','n','e',
                                  '\\','S','y','s','t','e','m',
                                  '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
                                  '\\','S','e','r','v','i','c','e','s',
                                  '\\',0};

#define MAX_SERVICE_NAME 260

/* tid of the thread running client request */
static DWORD request_thread;

/* pid/tid of the client thread */
static DWORD client_tid;
static DWORD client_pid;

struct wine_driver
{
    struct wine_rb_entry entry;

    DRIVER_OBJECT driver_obj;
    DRIVER_EXTENSION driver_extension;
};

static NTSTATUS get_device_id( DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR **id );

static int wine_drivers_rb_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, const struct wine_driver, entry );
    const UNICODE_STRING *k = key;

    return RtlCompareUnicodeString( k, &driver->driver_obj.DriverName, FALSE );
}

static struct wine_rb_tree wine_drivers = { wine_drivers_rb_compare };

static CRITICAL_SECTION drivers_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &drivers_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": drivers_cs") }
};
static CRITICAL_SECTION drivers_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

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

/* transfer result of IRP back to wineserver */
static NTSTATUS WINAPI dispatch_irp_completion( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    FILE_OBJECT *file = irp->Tail.Overlay.OriginalFileObject;
    HANDLE irp_handle = context;
    void *out_buff = irp->UserBuffer;

    if (irp->Flags & IRP_WRITE_OPERATION)
        out_buff = NULL;  /* do not transfer back input buffer */

    SERVER_START_REQ( set_irp_result )
    {
        req->handle   = wine_server_obj_handle( irp_handle );
        req->status   = irp->IoStatus.u.Status;
        req->file_ptr = wine_server_client_ptr( file );
        if (irp->IoStatus.u.Status >= 0)
        {
            req->size = irp->IoStatus.Information;
            if (out_buff) wine_server_add_data( req, out_buff, irp->IoStatus.Information );
        }
        wine_server_call( req );
    }
    SERVER_END_REQ;

    if (irp->Flags & IRP_CLOSE_OPERATION)
    {
        HeapFree( GetProcessHeap(), 0, file );
        irp->Tail.Overlay.OriginalFileObject = NULL;
    }

    if (irp->UserBuffer != irp->AssociatedIrp.SystemBuffer)
    {
        HeapFree( GetProcessHeap(), 0, irp->UserBuffer );
        irp->UserBuffer = NULL;
    }
    return STATUS_SUCCESS;
}

static void dispatch_irp( DEVICE_OBJECT *device, IRP *irp, HANDLE irp_handle )
{
    LARGE_INTEGER count;

    IoSetCompletionRoutine( irp, dispatch_irp_completion, irp_handle, TRUE, TRUE, TRUE );
    KeQueryTickCount( &count );  /* update the global KeTickCount */

    device->CurrentIrp = irp;
    IoCallDriver( device, irp );
    device->CurrentIrp = NULL;
}

/* process a create request for a given file */
static NTSTATUS dispatch_create( const irp_params_t *params, void *in_buff, ULONG in_size,
                                 ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    IO_STACK_LOCATION *irpsp;
    FILE_OBJECT *file;
    DEVICE_OBJECT *device = wine_server_get_ptr( params->create.device );

    if (!(file = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*file) ))) return STATUS_NO_MEMORY;

    TRACE( "device %p -> file %p\n", device, file );

    file->Type = 5;  /* MSDN */
    file->Size = sizeof(*file);
    file->DeviceObject = device;

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE )))
    {
        HeapFree( GetProcessHeap(), 0, file );
        return STATUS_NO_MEMORY;
    }

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = IRP_MJ_CREATE;
    irpsp->DeviceObject = device;
    irpsp->Parameters.Create.SecurityContext = NULL;  /* FIXME */
    irpsp->Parameters.Create.Options = params->create.options;
    irpsp->Parameters.Create.ShareAccess = params->create.sharing;
    irpsp->Parameters.Create.FileAttributes = 0;
    irpsp->Parameters.Create.EaLength = 0;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CREATE_OPERATION;
    dispatch_irp( device, irp, irp_handle );

    HeapFree( GetProcessHeap(), 0, in_buff );
    return STATUS_SUCCESS;
}

/* process a close request for a given file */
static NTSTATUS dispatch_close( const irp_params_t *params, void *in_buff, ULONG in_size,
                                 ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( params->close.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = file->DeviceObject;

    TRACE( "device %p file %p\n", device, file );

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE )))
    {
        HeapFree( GetProcessHeap(), 0, file );
        return STATUS_NO_MEMORY;
    }

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = IRP_MJ_CLOSE;
    irpsp->DeviceObject = device;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CLOSE_OPERATION;
    dispatch_irp( device, irp, irp_handle );

    HeapFree( GetProcessHeap(), 0, in_buff );
    return STATUS_SUCCESS;
}

/* process a read request for a given device */
static NTSTATUS dispatch_read( const irp_params_t *params, void *in_buff, ULONG in_size,
                               ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    void *out_buff;
    LARGE_INTEGER offset;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( params->read.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = file->DeviceObject;

    TRACE( "device %p file %p size %u\n", device, file, out_size );

    if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;

    offset.QuadPart = params->read.pos;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ, device, out_buff, out_size,
                                              &offset, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, out_buff );
        return STATUS_NO_MEMORY;
    }

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->Parameters.Read.Key = params->read.key;

    irp->Flags |= IRP_READ_OPERATION;
    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate out_buff */
    dispatch_irp( device, irp, irp_handle );

    HeapFree( GetProcessHeap(), 0, in_buff );
    return STATUS_SUCCESS;
}

/* process a write request for a given device */
static NTSTATUS dispatch_write( const irp_params_t *params, void *in_buff, ULONG in_size,
                                ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    LARGE_INTEGER offset;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( params->write.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = file->DeviceObject;

    TRACE( "device %p file %p size %u\n", device, file, in_size );

    offset.QuadPart = params->write.pos;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE, device, in_buff, in_size,
                                              &offset, NULL, NULL )))
        return STATUS_NO_MEMORY;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->Parameters.Write.Key = params->write.key;

    irp->Flags |= IRP_WRITE_OPERATION;
    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate in_buff */
    dispatch_irp( device, irp, irp_handle );

    return STATUS_SUCCESS;
}

/* process a flush request for a given device */
static NTSTATUS dispatch_flush( const irp_params_t *params, void *in_buff, ULONG in_size,
                                ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( params->flush.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = file->DeviceObject;

    TRACE( "device %p file %p\n", device, file );

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_FLUSH_BUFFERS, device, NULL, 0,
                                              NULL, NULL, NULL )))
        return STATUS_NO_MEMORY;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    dispatch_irp( device, irp, irp_handle );

    HeapFree( GetProcessHeap(), 0, in_buff );
    return STATUS_SUCCESS;
}

/* process an ioctl request for a given device */
static NTSTATUS dispatch_ioctl( const irp_params_t *params, void *in_buff, ULONG in_size,
                                ULONG out_size, HANDLE irp_handle )
{
    IRP *irp;
    void *out_buff = NULL;
    void *to_free = NULL;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( params->ioctl.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = file->DeviceObject;

    TRACE( "ioctl %x device %p file %p in_size %u out_size %u\n",
           params->ioctl.code, device, file, in_size, out_size );

    if (out_size)
    {
        if ((params->ioctl.code & 3) != METHOD_BUFFERED)
        {
            if (in_size < out_size) return STATUS_INVALID_DEVICE_REQUEST;
            in_size -= out_size;
            if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;
            memcpy( out_buff, (char *)in_buff + in_size, out_size );
        }
        else if (out_size > in_size)
        {
            if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;
            memcpy( out_buff, in_buff, in_size );
            to_free = in_buff;
            in_buff = out_buff;
        }
        else
        {
            out_buff = in_buff;
            out_size = in_size;
        }
    }

    irp = IoBuildDeviceIoControlRequest( params->ioctl.code, device, in_buff, in_size, out_buff, out_size,
                                         FALSE, NULL, NULL );
    if (!irp)
    {
        HeapFree( GetProcessHeap(), 0, out_buff );
        return STATUS_NO_MEMORY;
    }

    if (out_size && (params->ioctl.code & 3) != METHOD_BUFFERED)
        HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, in_buff, in_size );

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = in_buff;

    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate in_buff */
    dispatch_irp( device, irp, irp_handle );

    HeapFree( GetProcessHeap(), 0, to_free );
    return STATUS_SUCCESS;
}

typedef NTSTATUS (*dispatch_func)( const irp_params_t *params, void *in_buff, ULONG in_size,
                                   ULONG out_size, HANDLE irp_handle );

static const dispatch_func dispatch_funcs[IRP_MJ_MAXIMUM_FUNCTION + 1] =
{
    dispatch_create,   /* IRP_MJ_CREATE */
    NULL,              /* IRP_MJ_CREATE_NAMED_PIPE */
    dispatch_close,    /* IRP_MJ_CLOSE */
    dispatch_read,     /* IRP_MJ_READ */
    dispatch_write,    /* IRP_MJ_WRITE */
    NULL,              /* IRP_MJ_QUERY_INFORMATION */
    NULL,              /* IRP_MJ_SET_INFORMATION */
    NULL,              /* IRP_MJ_QUERY_EA */
    NULL,              /* IRP_MJ_SET_EA */
    dispatch_flush,    /* IRP_MJ_FLUSH_BUFFERS */
    NULL,              /* IRP_MJ_QUERY_VOLUME_INFORMATION */
    NULL,              /* IRP_MJ_SET_VOLUME_INFORMATION */
    NULL,              /* IRP_MJ_DIRECTORY_CONTROL */
    NULL,              /* IRP_MJ_FILE_SYSTEM_CONTROL */
    dispatch_ioctl,    /* IRP_MJ_DEVICE_CONTROL */
    NULL,              /* IRP_MJ_INTERNAL_DEVICE_CONTROL */
    NULL,              /* IRP_MJ_SHUTDOWN */
    NULL,              /* IRP_MJ_LOCK_CONTROL */
    NULL,              /* IRP_MJ_CLEANUP */
    NULL,              /* IRP_MJ_CREATE_MAILSLOT */
    NULL,              /* IRP_MJ_QUERY_SECURITY */
    NULL,              /* IRP_MJ_SET_SECURITY */
    NULL,              /* IRP_MJ_POWER */
    NULL,              /* IRP_MJ_SYSTEM_CONTROL */
    NULL,              /* IRP_MJ_DEVICE_CHANGE */
    NULL,              /* IRP_MJ_QUERY_QUOTA */
    NULL,              /* IRP_MJ_SET_QUOTA */
    NULL,              /* IRP_MJ_PNP */
};


/***********************************************************************
 *           wine_ntoskrnl_main_loop   (Not a Windows API)
 */
NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event )
{
    HANDLE manager = get_device_manager();
    HANDLE irp = 0;
    NTSTATUS status = STATUS_SUCCESS;
    irp_params_t irp_params;
    ULONG in_size = 4096, out_size = 0;
    void *in_buff = NULL;
    HANDLE handles[2];

    request_thread = GetCurrentThreadId();

    handles[0] = stop_event;
    handles[1] = manager;

    for (;;)
    {
        if (!in_buff && !(in_buff = HeapAlloc( GetProcessHeap(), 0, in_size )))
        {
            ERR( "failed to allocate buffer\n" );
            return STATUS_NO_MEMORY;
        }

        SERVER_START_REQ( get_next_device_request )
        {
            req->manager = wine_server_obj_handle( manager );
            req->prev = wine_server_obj_handle( irp );
            req->status = status;
            wine_server_set_reply( req, in_buff, in_size );
            if (!(status = wine_server_call( req )))
            {
                irp        = wine_server_ptr_handle( reply->next );
                irp_params = reply->params;
                client_tid = reply->client_tid;
                client_pid = reply->client_pid;
                in_size    = reply->in_size;
                out_size   = reply->out_size;
            }
            else
            {
                irp = 0; /* no previous irp */
                if (status == STATUS_BUFFER_OVERFLOW)
                    in_size = reply->in_size;
            }
        }
        SERVER_END_REQ;

        switch (status)
        {
        case STATUS_SUCCESS:
            if (irp_params.major > IRP_MJ_MAXIMUM_FUNCTION || !dispatch_funcs[irp_params.major])
            {
                WARN( "unsupported request %u\n", irp_params.major );
                status = STATUS_NOT_SUPPORTED;
                break;
            }
            status = dispatch_funcs[irp_params.major]( &irp_params, in_buff, in_size, out_size, irp );
            if (status == STATUS_SUCCESS)
            {
                irp = 0;  /* status reported by IoCompleteRequest */
                in_size = 4096;
                in_buff = NULL;
            }
            break;
        case STATUS_BUFFER_OVERFLOW:
            HeapFree( GetProcessHeap(), 0, in_buff );
            in_buff = NULL;
            /* restart with larger buffer */
            break;
        case STATUS_PENDING:
            for (;;)
            {
                DWORD ret = WaitForMultipleObjectsEx( 2, handles, FALSE, INFINITE, TRUE );
                if (ret == WAIT_OBJECT_0)
                {
                    HeapFree( GetProcessHeap(), 0, in_buff );
                    return STATUS_SUCCESS;
                }
                if (ret != WAIT_IO_COMPLETION) break;
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
    CCHAR loc_count = stack_size;

    TRACE( "%d, %d\n", stack_size, charge_quota );

    if (loc_count < 8 && loc_count != 1)
        loc_count = 8;

    size = sizeof(IRP) + loc_count * sizeof(IO_STACK_LOCATION);
    irp = ExAllocatePool( NonPagedPool, size );
    if (irp == NULL)
        return NULL;
    IoInitializeIrp( irp, size, stack_size );
    if (stack_size >= 1 && stack_size <= 8)
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
    MDL *mdl;

    TRACE( "%p\n", irp );

    mdl = irp->MdlAddress;
    while (mdl)
    {
        MDL *next = mdl->Next;
        IoFreeMdl( mdl );
        mdl = next;
    }

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
PMDL WINAPI IoAllocateMdl( PVOID va, ULONG length, BOOLEAN secondary, BOOLEAN charge_quota, IRP *irp )
{
    SIZE_T mdl_size;
    PMDL mdl;

    TRACE("(%p, %u, %i, %i, %p)\n", va, length, secondary, charge_quota, irp);

    if (charge_quota)
        FIXME("Charge quota is not yet supported\n");

    mdl_size = sizeof(MDL) + sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(va, length);
    mdl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, mdl_size );
    if (!mdl)
        return NULL;

    MmInitializeMdl( mdl, va, length );

    if (!irp) return mdl;

    if (secondary)  /* add it at the end */
    {
        MDL **pmdl = &irp->MdlAddress;
        while (*pmdl) pmdl = &(*pmdl)->Next;
        *pmdl = mdl;
    }
    else
    {
        mdl->Next = irp->MdlAddress;
        irp->MdlAddress = mdl;
    }
    return mdl;
}


/***********************************************************************
 *           IoFreeMdl  (NTOSKRNL.EXE.@)
 */
void WINAPI IoFreeMdl(PMDL mdl)
{
    TRACE("%p\n", mdl);
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
PIRP WINAPI IoBuildDeviceIoControlRequest( ULONG code, PDEVICE_OBJECT device,
                                           PVOID in_buff, ULONG in_len,
                                           PVOID out_buff, ULONG out_len,
                                           BOOLEAN internal, PKEVENT event,
                                           PIO_STATUS_BLOCK iosb )
{
    PIRP irp;
    PIO_STACK_LOCATION irpsp;
    MDL *mdl;

    TRACE( "%x, %p, %p, %u, %p, %u, %u, %p, %p\n",
           code, device, in_buff, in_len, out_buff, out_len, internal, event, iosb );

    if (device == NULL)
        return NULL;

    irp = IoAllocateIrp( device->StackSize, FALSE );
    if (irp == NULL)
        return NULL;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = internal ? IRP_MJ_INTERNAL_DEVICE_CONTROL : IRP_MJ_DEVICE_CONTROL;
    irpsp->Parameters.DeviceIoControl.IoControlCode = code;
    irpsp->Parameters.DeviceIoControl.InputBufferLength = in_len;
    irpsp->Parameters.DeviceIoControl.OutputBufferLength = out_len;
    irpsp->DeviceObject = device;
    irpsp->CompletionRoutine = NULL;

    switch (code & 3)
    {
    case METHOD_BUFFERED:
        irp->AssociatedIrp.SystemBuffer = in_buff;
        break;
    case METHOD_IN_DIRECT:
    case METHOD_OUT_DIRECT:
        irp->AssociatedIrp.SystemBuffer = in_buff;

        mdl = IoAllocateMdl( out_buff, out_len, FALSE, FALSE, irp );
        if (!mdl)
        {
            IoFreeIrp( irp );
            return NULL;
        }

        mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
        mdl->MappedSystemVa = out_buff;
        break;
    case METHOD_NEITHER:
        irpsp->Parameters.DeviceIoControl.Type3InputBuffer = in_buff;
        break;
    }

    irp->RequestorMode = KernelMode;
    irp->UserBuffer = out_buff;
    irp->UserIosb = iosb;
    irp->UserEvent = event;
    return irp;
}


/**********************************************************
 *           IoBuildSynchronousFsdRequest  (NTOSKRNL.EXE.@)
 */
PIRP WINAPI IoBuildSynchronousFsdRequest(ULONG majorfunc, PDEVICE_OBJECT device,
                                         PVOID buffer, ULONG length, PLARGE_INTEGER startoffset,
                                         PKEVENT event, PIO_STATUS_BLOCK iosb)
{
    PIRP irp;
    PIO_STACK_LOCATION irpsp;

    TRACE("(%d %p %p %d %p %p %p)\n", majorfunc, device, buffer, length, startoffset, event, iosb);

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE ))) return NULL;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = majorfunc;
    irpsp->DeviceObject = device;
    irpsp->CompletionRoutine = NULL;

    irp->AssociatedIrp.SystemBuffer = buffer;

    if (device->Flags & DO_DIRECT_IO)
    {
        MDL *mdl = IoAllocateMdl( buffer, length, FALSE, FALSE, irp );
        if (!mdl)
        {
            IoFreeIrp( irp );
            return NULL;
        }

        mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
        mdl->MappedSystemVa = buffer;
    }

    switch (majorfunc)
    {
    case IRP_MJ_READ:
        irpsp->Parameters.Read.Length = length;
        irpsp->Parameters.Read.ByteOffset = *startoffset;
        break;
    case IRP_MJ_WRITE:
        irpsp->Parameters.Write.Length = length;
        irpsp->Parameters.Write.ByteOffset = *startoffset;
        break;
    }
    irp->RequestorMode = KernelMode;
    irp->UserIosb = iosb;
    irp->UserEvent = event;
    irp->UserBuffer = buffer;
    return irp;
}

static void build_driver_keypath( const WCHAR *name, UNICODE_STRING *keypath )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    WCHAR *str;

    /* Check what prefix is present */
    if (strncmpW( name, servicesW, strlenW(servicesW) ) == 0)
    {
        FIXME( "Driver name %s is malformed as the keypath\n", debugstr_w(name) );
        RtlCreateUnicodeString( keypath, name );
        return;
    }
    if (strncmpW( name, driverW, strlenW(driverW) ) == 0)
        name += strlenW(driverW);
    else
        FIXME( "Driver name %s does not properly begin with \\Driver\\\n", debugstr_w(name) );

    str = HeapAlloc( GetProcessHeap(), 0, sizeof(servicesW) + strlenW(name)*sizeof(WCHAR));
    lstrcpyW( str, servicesW );
    lstrcatW( str, name );
    RtlInitUnicodeString( keypath, str );
}


static NTSTATUS WINAPI unhandled_irp( DEVICE_OBJECT *device, IRP *irp )
{
    TRACE( "(%p, %p)\n", device, irp );
    irp->IoStatus.u.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_INVALID_DEVICE_REQUEST;
}


/***********************************************************************
 *           IoCreateDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDriver( UNICODE_STRING *name, PDRIVER_INITIALIZE init )
{
    struct wine_driver *driver;
    NTSTATUS status;
    unsigned int i;

    TRACE("(%s, %p)\n", debugstr_us(name), init);

    if (!(driver = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    sizeof(*driver) )))
        return STATUS_NO_MEMORY;

    if ((status = RtlDuplicateUnicodeString( 1, name, &driver->driver_obj.DriverName )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, driver );
        return status;
    }

    driver->driver_obj.Size            = sizeof(driver->driver_obj);
    driver->driver_obj.DriverInit      = init;
    driver->driver_obj.DriverExtension = &driver->driver_extension;
    driver->driver_extension.DriverObject   = &driver->driver_obj;
    build_driver_keypath( driver->driver_obj.DriverName.Buffer, &driver->driver_extension.ServiceKeyName );
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        driver->driver_obj.MajorFunction[i] = unhandled_irp;

    status = driver->driver_obj.DriverInit( &driver->driver_obj, &driver->driver_extension.ServiceKeyName );
    if (status)
    {
        RtlFreeUnicodeString( &driver->driver_obj.DriverName );
        RtlFreeUnicodeString( &driver->driver_extension.ServiceKeyName );
        RtlFreeHeap( GetProcessHeap(), 0, driver );
        return status;
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        if (driver->driver_obj.MajorFunction[i]) continue;
        driver->driver_obj.MajorFunction[i] = unhandled_irp;
    }

    EnterCriticalSection( &drivers_cs );
    if (wine_rb_put( &wine_drivers, &driver->driver_obj.DriverName, &driver->entry ))
        ERR( "failed to insert driver %s in tree\n", debugstr_us(name) );
    LeaveCriticalSection( &drivers_cs );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           IoDeleteDriver   (NTOSKRNL.EXE.@)
 */
void WINAPI IoDeleteDriver( DRIVER_OBJECT *driver_object )
{
    TRACE( "(%p)\n", driver_object );

    EnterCriticalSection( &drivers_cs );
    wine_rb_remove_key( &wine_drivers, &driver_object->DriverName );
    LeaveCriticalSection( &drivers_cs );

    RtlFreeUnicodeString( &driver_object->DriverName );
    RtlFreeUnicodeString( &driver_object->DriverExtension->ServiceKeyName );
    RtlFreeHeap( GetProcessHeap(), 0, CONTAINING_RECORD( driver_object, struct wine_driver, driver_obj ) );
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
    static DEVICE_OBJECT stub_device;
    static DRIVER_OBJECT stub_driver;

    FIXME( "stub: %s %x %p %p\n", debugstr_us(name), access, file, device );

    stub_device.StackSize = 0x80; /* minimum value to appease SecuROM 5.x */
    stub_device.DriverObject = &stub_driver;

    *file  = NULL;
    *device = &stub_device;

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           IoGetAttachedDevice   (NTOSKRNL.EXE.@)
 */
DEVICE_OBJECT* WINAPI IoGetAttachedDevice( DEVICE_OBJECT *device )
{
    DEVICE_OBJECT *result = device;

    TRACE( "(%p)\n", device );

    while (result->AttachedDevice)
        result = result->AttachedDevice;

    return result;
}


/***********************************************************************
 *           IoGetDeviceProperty   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoGetDeviceProperty( DEVICE_OBJECT *device, DEVICE_REGISTRY_PROPERTY device_property,
                                     ULONG buffer_length, PVOID property_buffer, PULONG result_length )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    TRACE( "%p %d %u %p %p\n", device, device_property, buffer_length,
           property_buffer, result_length );
    switch (device_property)
    {
        case DevicePropertyEnumeratorName:
        {
            WCHAR *id, *ptr;

            status = get_device_id( device, BusQueryInstanceID, &id );
            if (status != STATUS_SUCCESS)
            {
                ERR( "Failed to get device id\n" );
                break;
            }

            struprW( id );
            ptr = strchrW( id, '\\' );
            if (ptr) *ptr = 0;

            *result_length = sizeof(WCHAR) * (strlenW(id) + 1);
            if (buffer_length >= *result_length)
                memcpy( property_buffer, id, *result_length );
            else
                status = STATUS_BUFFER_TOO_SMALL;

            HeapFree( GetProcessHeap(), 0, id );
            break;
        }
        case DevicePropertyPhysicalDeviceObjectName:
        {
            ULONG used_len, len = buffer_length + sizeof(OBJECT_NAME_INFORMATION);
            OBJECT_NAME_INFORMATION *name = HeapAlloc(GetProcessHeap(), 0, len);

            status = NtQueryObject(device->Reserved, ObjectNameInformation, name, len, &used_len);
            if (status == STATUS_SUCCESS)
            {
                /* Ensure room for NULL termination */
                if (buffer_length >= name->Name.MaximumLength)
                    memcpy(property_buffer, name->Name.Buffer, name->Name.MaximumLength);
                else
                    status = STATUS_BUFFER_TOO_SMALL;
                *result_length = name->Name.MaximumLength;
            }
            else
            {
                if (status == STATUS_INFO_LENGTH_MISMATCH ||
                    status == STATUS_BUFFER_OVERFLOW)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    *result_length = used_len - sizeof(OBJECT_NAME_INFORMATION);
                }
                else
                    *result_length = 0;
            }
            HeapFree(GetProcessHeap(), 0, name);
            break;
        }
        default:
            FIXME("unhandled property %d\n", device_property);
    }
    return status;
}


/***********************************************************************
 *           IoCallDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCallDriver( DEVICE_OBJECT *device, IRP *irp )
{
    PDRIVER_DISPATCH dispatch;
    IO_STACK_LOCATION *irpsp;
    NTSTATUS status;

    --irp->CurrentLocation;
    irpsp = --irp->Tail.Overlay.s.u2.CurrentStackLocation;
    dispatch = device->DriverObject->MajorFunction[irpsp->MajorFunction];

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call driver dispatch %p (device=%p,irp=%p)\n",
                 GetCurrentThreadId(), dispatch, device, irp );

    status = dispatch( device, irp );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  driver dispatch %p (device=%p,irp=%p) retval=%08x\n",
                 GetCurrentThreadId(), dispatch, device, irp, status );

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
    FIXME( "(%s, %p, %p, %u, %p, %p, %u, %d, %p): stub\n", debugstr_us(name),
           drv_obj, drv_list, drv_size, dev_obj, dev_list, dev_size, overwrite, detected );
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoCompleteRequest   (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoCompleteRequest( IRP *irp, UCHAR priority_boost )
{
    IO_STACK_LOCATION *irpsp;
    PIO_COMPLETION_ROUTINE routine;
    NTSTATUS status, stat;
    int call_flag = 0;

    TRACE( "%p %u\n", irp, priority_boost );

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

    if (irp->Flags & IRP_DEALLOCATE_BUFFER)
        HeapFree( GetProcessHeap(), 0, irp->AssociatedIrp.SystemBuffer );

    IoFreeIrp( irp );
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
 *           InterlockedPopEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( NTOSKRNL_InterlockedPopEntrySList )
PSLIST_ENTRY WINAPI __regs_NTOSKRNL_InterlockedPopEntrySList( PSLIST_HEADER list )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_InterlockedPopEntrySList( PSLIST_HEADER list )
#endif
{
    return InterlockedPopEntrySList( list );
}


/***********************************************************************
 *           InterlockedPushEntrySList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( NTOSKRNL_InterlockedPushEntrySList )
PSLIST_ENTRY WINAPI __regs_NTOSKRNL_InterlockedPushEntrySList( PSLIST_HEADER list, PSLIST_ENTRY entry )
#else
PSLIST_ENTRY WINAPI NTOSKRNL_InterlockedPushEntrySList( PSLIST_HEADER list, PSLIST_ENTRY entry )
#endif
{
    return InterlockedPushEntrySList( list, entry );
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
 *           ExDeleteNPagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExDeleteNPagedLookasideList( PNPAGED_LOOKASIDE_LIST lookaside )
{
    FIXME("(%p) stub\n", lookaside);
}


/***********************************************************************
 *           ExDeletePagedLookasideList  (NTOSKRNL.EXE.@)
 */
void WINAPI ExDeletePagedLookasideList( PPAGED_LOOKASIDE_LIST lookaside )
{
    FIXME("(%p) stub\n", lookaside);
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

/***********************************************************************
 *           KeInsertQueue   (NTOSKRNL.EXE.@)
 */
LONG WINAPI KeInsertQueue(PRKQUEUE Queue, PLIST_ENTRY Entry)
{
    FIXME( "stub: %p %p\n", Queue, Entry );
    return 0;
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
 *           KeSetSystemAffinityThread   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetSystemAffinityThread(KAFFINITY Affinity)
{
    FIXME("(%lx) stub\n", Affinity);
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
 *           MmLockPagableSectionByHandle  (NTOSKRNL.EXE.@)
 */
VOID WINAPI MmLockPagableSectionByHandle(PVOID ImageSectionHandle)
{
    FIXME("stub %p\n", ImageSectionHandle);
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
 *           MmUnlockPagableImageSection  (NTOSKRNL.EXE.@)
 */
VOID WINAPI MmUnlockPagableImageSection(PVOID ImageSectionHandle)
{
    FIXME("stub %p\n", ImageSectionHandle);
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
    struct wine_driver *driver;
    struct wine_rb_entry *entry;

    TRACE("mostly-stub:%s %i %p %i %p %i %p %p\n", debugstr_us(ObjectName),
        Attributes, AccessState, DesiredAccess, ObjectType, AccessMode,
        ParseContext, Object);

    if (AccessState) FIXME("Unhandled AccessState\n");
    if (DesiredAccess) FIXME("Unhandled DesiredAccess\n");
    if (ParseContext) FIXME("Unhandled ParseContext\n");
    if (ObjectType) FIXME("Unhandled ObjectType\n");

    if (AccessMode != KernelMode)
    {
        FIXME("UserMode access not implemented\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    EnterCriticalSection(&drivers_cs);
    entry = wine_rb_get(&wine_drivers, ObjectName);
    LeaveCriticalSection(&drivers_cs);
    if (!entry)
    {
        FIXME("Object (%s) not found, may not be tracked.\n", debugstr_us(ObjectName));
        return STATUS_NOT_IMPLEMENTED;
    }

    driver = WINE_RB_ENTRY_VALUE(entry, struct wine_driver, entry);
    *Object = &driver->driver_obj;
    return STATUS_SUCCESS;
}


static void ObReferenceObject( void *obj )
{
    TRACE( "(%p): stub\n", obj );
}


/***********************************************************************
 *           ObDereferenceObject   (NTOSKRNL.EXE.@)
 */
void WINAPI ObDereferenceObject( void *obj )
{
    TRACE( "(%p): stub\n", obj );
}


/***********************************************************************
 *           ObfReferenceObject   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( ObfReferenceObject )
void WINAPI __regs_ObfReferenceObject( void *obj )
#else
void WINAPI ObfReferenceObject( void *obj )
#endif
{
    ObReferenceObject( obj );
}


/***********************************************************************
 *           ObfDereferenceObject   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL1_ENTRYPOINT
DEFINE_FASTCALL1_ENTRYPOINT( ObfDereferenceObject )
void WINAPI __regs_ObfDereferenceObject( void *obj )
#else
void WINAPI ObfDereferenceObject( void *obj )
#endif
{
    ObDereferenceObject( obj );
}


/***********************************************************************
 *           IoGetAttachedDeviceReference   (NTOSKRNL.EXE.@)
 */
DEVICE_OBJECT* WINAPI IoGetAttachedDeviceReference( DEVICE_OBJECT *device )
{
    DEVICE_OBJECT *result = IoGetAttachedDevice( device );
    ObReferenceObject( result );
    return result;
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
 *           PsRemoveCreateThreadNotifyRoutine  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsRemoveCreateThreadNotifyRoutine( PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine )
{
    FIXME( "stub: %p\n", NotifyRoutine );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           PsRemoveLoadImageNotifyRoutine  (NTOSKRNL.EXE.@)
 */
 NTSTATUS WINAPI PsRemoveLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
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
    static int once;
    if (!once++) FIXME("(%p %p) stub\n", processid, process);
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
#if defined(__i386__) || defined(__x86_64__)
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

/*****************************************************
 *           ObQueryNameString  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObQueryNameString(PVOID object, POBJECT_NAME_INFORMATION name, ULONG maxlength, PULONG returnlength)
{
    FIXME("(%p %p %u %p) stub\n", object, name, maxlength, returnlength);
    return STATUS_NOT_IMPLEMENTED;
}

/*****************************************************
 *           IoRegisterPlugPlayNotification  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoRegisterPlugPlayNotification(IO_NOTIFICATION_EVENT_CATEGORY category, ULONG flags, PVOID data,
                                               PDRIVER_OBJECT driver, PDRIVER_NOTIFICATION_CALLBACK_ROUTINE callback,
                                               PVOID context, PVOID *notification)
{
    FIXME("(%u %u %p %p %p %p %p) stub\n", category, flags, data, driver, callback, context, notification);
    return STATUS_SUCCESS;
}

/*****************************************************
 *           IoCsqInitialize  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCsqInitialize(PIO_CSQ csq, PIO_CSQ_INSERT_IRP insert_irp, PIO_CSQ_REMOVE_IRP remove_irp,
                                PIO_CSQ_PEEK_NEXT_IRP peek_irp, PIO_CSQ_ACQUIRE_LOCK acquire_lock,
                                PIO_CSQ_RELEASE_LOCK release_lock, PIO_CSQ_COMPLETE_CANCELED_IRP complete_irp)
{
    FIXME("(%p %p %p %p %p %p %p) stub\n",
          csq, insert_irp, remove_irp, peek_irp, acquire_lock, release_lock, complete_irp);
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           ExAcquireResourceExclusiveLite  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI ExAcquireResourceExclusiveLite( PERESOURCE resource, BOOLEAN wait )
{
    FIXME( ":%p %u stub\n", resource, wait );
    return TRUE;
}

/***********************************************************************
 *           ExDeleteResourceLite  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ExDeleteResourceLite(PERESOURCE resource)
{
    FIXME("(%p): stub\n", resource);
    return STATUS_NOT_IMPLEMENTED;
}

/*****************************************************
 *           ExInterlockedRemoveHeadList  (NTOSKRNL.EXE.@)
 */
PLIST_ENTRY WINAPI ExInterlockedRemoveHeadList(PLIST_ENTRY head, PKSPIN_LOCK lock)
{
    FIXME("(%p %p) stub\n", head, lock);
    return NULL;
}

/***********************************************************************
 *           ExfInterlockedRemoveHeadList   (NTOSKRNL.EXE.@)
 */
#ifdef DEFINE_FASTCALL2_ENTRYPOINT
DEFINE_FASTCALL2_ENTRYPOINT( ExfInterlockedRemoveHeadList )
PLIST_ENTRY WINAPI __regs_ExfInterlockedRemoveHeadList(PLIST_ENTRY head, PKSPIN_LOCK lock)
#else
PLIST_ENTRY WINAPI ExfInterlockedRemoveHeadList(PLIST_ENTRY head, PKSPIN_LOCK lock)
#endif
{
    FIXME("(%p %p) stub\n", head, lock);
    return ExInterlockedRemoveHeadList( head, lock );
}

/***********************************************************************
 *           ExReleaseResourceForThreadLite   (NTOSKRNL.EXE.@)
 */
void WINAPI ExReleaseResourceForThreadLite( PERESOURCE resource, ERESOURCE_THREAD tid )
{
    FIXME( "stub: %p %lu\n", resource, tid );
}

/***********************************************************************
 *           KeEnterCriticalRegion  (NTOSKRNL.EXE.@)
 */
void WINAPI KeEnterCriticalRegion(void)
{
    FIXME(": stub\n");
}

/***********************************************************************
 *           KeLeaveCriticalRegion  (NTOSKRNL.EXE.@)
 */
void WINAPI KeLeaveCriticalRegion(void)
{
    FIXME(": stub\n");
}

/***********************************************************************
 *           ProbeForRead   (NTOSKRNL.EXE.@)
 */
void WINAPI ProbeForRead(void *address, SIZE_T length, ULONG alignment)
{
    FIXME("(%p %lu %u) stub\n", address, length, alignment);
}

/***********************************************************************
 *           ProbeForWrite   (NTOSKRNL.EXE.@)
 */
void WINAPI ProbeForWrite(void *address, SIZE_T length, ULONG alignment)
{
    FIXME("(%p %lu %u) stub\n", address, length, alignment);
}

/***********************************************************************
 *           CmRegisterCallback  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI CmRegisterCallback(EX_CALLBACK_FUNCTION *function, void *context, LARGE_INTEGER *cookie)
{
    FIXME("(%p %p %p): stub\n", function, context, cookie);
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           CmUnRegisterCallback  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI CmUnRegisterCallback(LARGE_INTEGER cookie)
{
    FIXME("(%s): stub\n", wine_dbgstr_longlong(cookie.QuadPart));
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           KeDelayExecutionThread  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeDelayExecutionThread(KPROCESSOR_MODE waitmode, BOOLEAN alertable, PLARGE_INTEGER interval)
{
    FIXME("(%u, %u, %p): stub\n", waitmode, alertable, interval);
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           IoAttachDevice  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoAttachDevice(DEVICE_OBJECT *source, UNICODE_STRING *target, DEVICE_OBJECT *attached)
{
    FIXME("(%p, %s, %p): stub\n", source, debugstr_us(target), attached);
    return STATUS_NOT_IMPLEMENTED;
}


static NTSTATUS open_driver( const UNICODE_STRING *service_name, SC_HANDLE *service )
{
    QUERY_SERVICE_CONFIGW *service_config = NULL;
    SC_HANDLE manager_handle;
    DWORD config_size = 0;
    WCHAR *name;

    if (!(name = RtlAllocateHeap( GetProcessHeap(), 0, service_name->Length + sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    memcpy( name, service_name->Buffer, service_name->Length );
    name[ service_name->Length / sizeof(WCHAR) ] = 0;

    if (strncmpW( name, servicesW, strlenW(servicesW) ))
    {
        FIXME( "service name %s is not a keypath\n", debugstr_us(service_name) );
        RtlFreeHeap( GetProcessHeap(), 0, name );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (!(manager_handle = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT )))
    {
        WARN( "failed to connect to service manager\n" );
        RtlFreeHeap( GetProcessHeap(), 0, name );
        return STATUS_NOT_SUPPORTED;
    }

    *service = OpenServiceW( manager_handle, name + strlenW(servicesW), SERVICE_ALL_ACCESS );
    RtlFreeHeap( GetProcessHeap(), 0, name );
    CloseServiceHandle( manager_handle );

    if (!*service)
    {
        WARN( "failed to open service %s\n", debugstr_us(service_name) );
        return STATUS_UNSUCCESSFUL;
    }

    QueryServiceConfigW( *service, NULL, 0, &config_size );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        WARN( "failed to query service config\n" );
        goto error;
    }

    if (!(service_config = RtlAllocateHeap( GetProcessHeap(), 0, config_size )))
        goto error;

    if (!QueryServiceConfigW( *service, service_config, config_size, &config_size ))
    {
        WARN( "failed to query service config\n" );
        goto error;
    }

    if (service_config->dwServiceType != SERVICE_KERNEL_DRIVER &&
        service_config->dwServiceType != SERVICE_FILE_SYSTEM_DRIVER)
    {
        WARN( "service %s is not a kernel driver\n", debugstr_us(service_name) );
        goto error;
    }

    TRACE( "opened service for driver %s\n", debugstr_us(service_name) );
    RtlFreeHeap( GetProcessHeap(), 0, service_config );
    return STATUS_SUCCESS;

error:
    CloseServiceHandle( *service );
    RtlFreeHeap( GetProcessHeap(), 0, service_config );
    return STATUS_UNSUCCESSFUL;
}


/***********************************************************************
 *           ZwLoadDriver (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ZwLoadDriver( const UNICODE_STRING *service_name )
{
    SERVICE_STATUS_PROCESS service_status;
    SC_HANDLE service_handle;
    ULONGLONG start_time;
    NTSTATUS status;
    DWORD bytes;

    TRACE( "(%s)\n", debugstr_us(service_name) );

    if ((status = open_driver( service_name, &service_handle )) != STATUS_SUCCESS)
        return status;

    TRACE( "trying to start %s\n", debugstr_us(service_name) );

    start_time = GetTickCount64();
    for (;;)
    {
        if (StartServiceW( service_handle, 0, NULL )) break;
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) break;
        if (GetLastError() != ERROR_SERVICE_DATABASE_LOCKED) goto error;
        if (GetTickCount64() - start_time > 30000) goto error;
        Sleep( 100 );
    }

    start_time = GetTickCount64();
    for (;;)
    {
        if (!QueryServiceStatusEx( service_handle, SC_STATUS_PROCESS_INFO,
                                   (BYTE *)&service_status, sizeof(service_status), &bytes )) goto error;
        if (service_status.dwCurrentState != SERVICE_START_PENDING) break;
        if (GetTickCount64() - start_time > 30000) goto error;
        Sleep( 100 );
    }

    if (service_status.dwCurrentState == SERVICE_RUNNING)
    {
        if (service_status.dwProcessId != GetCurrentProcessId())
            FIXME( "driver %s was loaded into a different process\n", debugstr_us(service_name) );

        status = STATUS_SUCCESS;
        goto done;
    }

error:
    WARN( "failed to start service %s\n", debugstr_us(service_name) );
    status = STATUS_UNSUCCESSFUL;

done:
    TRACE( "returning status %08x\n", status );
    CloseServiceHandle( service_handle );
    return status;
}


/***********************************************************************
 *           ZwUnloadDriver (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ZwUnloadDriver( const UNICODE_STRING *service_name )
{
    SERVICE_STATUS service_status;
    SC_HANDLE service_handle;
    ULONGLONG start_time;
    NTSTATUS status;

    TRACE( "(%s)\n", debugstr_us(service_name) );

    if ((status = open_driver( service_name, &service_handle )) != STATUS_SUCCESS)
        return status;

    if (!ControlService( service_handle, SERVICE_CONTROL_STOP, &service_status ))
        goto error;

    start_time = GetTickCount64();
    for (;;)
    {
        if (!QueryServiceStatus( service_handle, &service_status )) goto error;
        if (service_status.dwCurrentState != SERVICE_STOP_PENDING) break;
        if (GetTickCount64() - start_time > 30000) goto error;
        Sleep( 100 );
    }

    if (service_status.dwCurrentState == SERVICE_STOPPED)
    {
        status = STATUS_SUCCESS;
        goto done;
    }

error:
    WARN( "failed to stop service %s\n", debugstr_us(service_name) );
    status = STATUS_UNSUCCESSFUL;

done:
    TRACE( "returning status %08x\n", status );
    CloseServiceHandle( service_handle );
    return status;
}


static NTSTATUS WINAPI internal_complete( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    HANDLE event = context;
    SetEvent( event );
    return STATUS_MORE_PROCESSING_REQUIRED;
}


static NTSTATUS send_device_irp( DEVICE_OBJECT *device, IRP *irp, ULONG_PTR *info )
{
    NTSTATUS status;
    HANDLE event = CreateEventA( NULL, FALSE, FALSE, NULL );
    DEVICE_OBJECT *toplevel_device;

    irp->IoStatus.u.Status = STATUS_NOT_SUPPORTED;
    IoSetCompletionRoutine( irp, internal_complete, event, TRUE, TRUE, TRUE );

    toplevel_device = IoGetAttachedDeviceReference( device );
    status = IoCallDriver( toplevel_device, irp );

    if (status == STATUS_PENDING)
        WaitForSingleObject( event, INFINITE );

    status = irp->IoStatus.u.Status;
    if (info)
        *info = irp->IoStatus.Information;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    ObDereferenceObject( toplevel_device );
    CloseHandle( event );
    return status;
}


static NTSTATUS get_device_id( DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR **id )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    IRP *irp;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    return send_device_irp( device, irp, (ULONG_PTR *)id );
}


static BOOL get_driver_for_id( const WCHAR *id, WCHAR *driver )
{
    static const WCHAR serviceW[] = {'S','e','r','v','i','c','e',0};
    static const UNICODE_STRING service_str = { sizeof(serviceW) - sizeof(WCHAR), sizeof(serviceW), (WCHAR *)serviceW };
    static const WCHAR critical_fmtW[] =
        {'\\','R','e','g','i','s','t','r','y',
         '\\','M','a','c','h','i','n','e',
         '\\','S','y','s','t','e','m',
         '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
         '\\','C','o','n','t','r','o','l',
         '\\','C','r','i','t','i','c','a','l','D','e','v','i','c','e','D','a','t','a','b','a','s','e',
         '\\','%','s',0};
    WCHAR buffer[FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data[MAX_SERVICE_NAME * sizeof(WCHAR)] )];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING key;
    NTSTATUS status;
    HANDLE hkey;
    WCHAR *keyW;
    DWORD len;

    if (!(keyW = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(critical_fmtW) + strlenW(id) * sizeof(WCHAR) )))
        return STATUS_NO_MEMORY;

    sprintfW( keyW, critical_fmtW, id );
    RtlInitUnicodeString( &key, keyW );
    InitializeObjectAttributes( &attr, &key, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );

    status = NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr );
    RtlFreeUnicodeString( &key );
    if (status != STATUS_SUCCESS)
    {
        TRACE_(plugplay)( "no driver found for %s\n", debugstr_w(id) );
        return FALSE;
    }

    status = NtQueryValueKey( hkey, &service_str, KeyValuePartialInformation,
                              info, sizeof(buffer) - sizeof(WCHAR), &len );
    NtClose( hkey );
    if (status != STATUS_SUCCESS || info->Type != REG_SZ)
    {
        TRACE_(plugplay)( "no driver found for %s\n", debugstr_w(id) );
        return FALSE;
    }

    memcpy( driver, info->Data, info->DataLength );
    driver[ info->DataLength / sizeof(WCHAR) ] = 0;
    TRACE_(plugplay)( "found driver %s for %s\n", debugstr_w(driver), debugstr_w(id) );
    return TRUE;
}


static NTSTATUS send_pnp_irp( DEVICE_OBJECT *device, UCHAR minor )
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    IRP *irp;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = minor;

    irpsp->Parameters.StartDevice.AllocatedResources = NULL;
    irpsp->Parameters.StartDevice.AllocatedResourcesTranslated = NULL;

    return send_device_irp( device, irp, NULL );
}


static NTSTATUS send_power_irp( DEVICE_OBJECT *device, DEVICE_POWER_STATE power )
{
    IO_STATUS_BLOCK irp_status;
    IO_STACK_LOCATION *irpsp;
    IRP *irp;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_POWER, device, NULL, 0, NULL, NULL, &irp_status )))
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MinorFunction = IRP_MN_SET_POWER;

    irpsp->Parameters.Power.Type = DevicePowerState;
    irpsp->Parameters.Power.State.DeviceState = power;

    return send_device_irp( device, irp, NULL );
}


static void handle_bus_relations( DEVICE_OBJECT *device )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    WCHAR buffer[MAX_SERVICE_NAME + sizeof(servicesW)/sizeof(WCHAR)];
    WCHAR driver[MAX_SERVICE_NAME] = {0};
    DRIVER_OBJECT *driver_obj;
    UNICODE_STRING string;
    WCHAR *ids, *ptr;
    NTSTATUS status;

    TRACE_(plugplay)( "(%p)\n", device );

    /* We could (should?) do a full IRP_MN_QUERY_DEVICE_RELATIONS query,
     * but we don't have to, we have the DEVICE_OBJECT of the new device
     * so we can simply handle the process here */

    status = get_device_id( device, BusQueryCompatibleIDs, &ids );
    if (status != STATUS_SUCCESS || !ids)
    {
        ERR_(plugplay)( "Failed to get device IDs\n" );
        return;
    }

    for (ptr = ids; *ptr; ptr += strlenW(ptr) + 1)
    {
        if (get_driver_for_id( ptr, driver ))
            break;
    }
    RtlFreeHeap( GetProcessHeap(), 0, ids );

    if (!driver[0])
    {
        ERR_(plugplay)( "No matching driver found for device\n" );
        return;
    }

    strcpyW( buffer, servicesW );
    strcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    if (ZwLoadDriver( &string ) != STATUS_SUCCESS)
    {
        ERR_(plugplay)( "Failed to load driver %s\n", debugstr_w(driver) );
        return;
    }

    strcpyW( buffer, driverW );
    strcatW( buffer, driver );
    RtlInitUnicodeString( &string, buffer );
    if (ObReferenceObjectByName( &string, OBJ_CASE_INSENSITIVE, NULL,
                                 0, NULL, KernelMode, NULL, (void **)&driver_obj ) != STATUS_SUCCESS)
    {
        ERR_(plugplay)( "Failed to locate loaded driver %s\n", debugstr_w(driver) );
        return;
    }

    if (driver_obj->DriverExtension->AddDevice)
        status = driver_obj->DriverExtension->AddDevice( driver_obj, device );
    else
        status = STATUS_NOT_IMPLEMENTED;

    ObDereferenceObject( driver_obj );

    if (status != STATUS_SUCCESS)
    {
        ERR_(plugplay)( "AddDevice failed for driver %s\n", debugstr_w(driver) );
        return;
    }

    send_pnp_irp( device, IRP_MN_START_DEVICE );
    send_power_irp( device, PowerDeviceD0 );
}


static void handle_removal_relations( DEVICE_OBJECT *device )
{
    TRACE_(plugplay)( "(%p)\n", device );

    send_power_irp( device, PowerDeviceD3 );
    send_pnp_irp( device, IRP_MN_SURPRISE_REMOVAL );
    send_pnp_irp( device, IRP_MN_REMOVE_DEVICE );
}


/***********************************************************************
 *           IoInvalidateDeviceRelations (NTOSKRNL.EXE.@)
 */
void WINAPI IoInvalidateDeviceRelations( DEVICE_OBJECT *device_object, DEVICE_RELATION_TYPE type )
{
    TRACE( "(%p, %i)\n", device_object, type );

    switch (type)
    {
        case BusRelations:
            handle_bus_relations( device_object );
            break;
        case RemovalRelations:
            handle_removal_relations( device_object );
            break;
        default:
            FIXME( "unhandled relation %i\n", type );
            break;
    }
}

/***********************************************************************
 *           KeSetTimerEx (NTOSKRNL.EXE.@)
 */
BOOL WINAPI KeSetTimerEx( KTIMER *timer, LARGE_INTEGER duetime, LONG period, KDPC *dpc )
{
    FIXME("stub: %p %s %u %p\n", timer, wine_dbgstr_longlong(duetime.QuadPart), period, dpc);
    return TRUE;
}

/***********************************************************************
 *           IoCreateFile (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateFile(HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                              IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size, ULONG attributes, ULONG sharing,
                              ULONG disposition, ULONG create_options, VOID *ea_buffer, ULONG ea_length,
                              CREATE_FILE_TYPE file_type, VOID *parameters, ULONG options )
{
    FIXME(": stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           KeClearEvent (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeClearEvent(PRKEVENT event)
{
    FIXME("stub: %p\n", event);
}
