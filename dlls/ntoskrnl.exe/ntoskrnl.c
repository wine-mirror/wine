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

#include <stdarg.h>
#include <assert.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winsvc.h"
#include "winternl.h"
#include "excpt.h"
#include "winioctl.h"
#include "winbase.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "ddk/csq.h"
#include "ddk/ntddk.h"
#include "ddk/ntifs.h"
#include "ddk/wdm.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/rbtree.h"
#include "wine/svcctl.h"

#include "ntoskrnl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);
WINE_DECLARE_DEBUG_CHANNEL(relay);

BOOLEAN KdDebuggerEnabled = FALSE;
ULONG InitSafeBootMode = 0;
USHORT NtBuildNumber = 0;

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

#define MAX_SERVICE_NAME 260

/* tid of the thread running client request */
static DWORD request_thread;

/* tid of the client thread */
static DWORD client_tid;

struct wine_driver
{
    DRIVER_OBJECT driver_obj;
    DRIVER_EXTENSION driver_extension;
    SERVICE_STATUS_HANDLE service_handle;
    struct wine_rb_entry entry;
};

static int wine_drivers_rb_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, const struct wine_driver, entry );
    const UNICODE_STRING *k = key;

    return RtlCompareUnicodeString( k, &driver->driver_obj.DriverName, FALSE );
}

static struct wine_rb_tree wine_drivers = { wine_drivers_rb_compare };

DECLARE_CRITICAL_SECTION(drivers_cs);

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


struct object_header
{
    LONG ref;
    POBJECT_TYPE type;
};

static void free_kernel_object( void *obj )
{
    struct object_header *header = (struct object_header *)obj - 1;
    HeapFree( GetProcessHeap(), 0, header );
}

void *alloc_kernel_object( POBJECT_TYPE type, HANDLE handle, SIZE_T size, LONG ref )
{
    struct object_header *header;

    if (!(header = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*header) + size)) )
        return NULL;

    if (handle)
    {
        NTSTATUS status;
        SERVER_START_REQ( set_kernel_object_ptr )
        {
            req->manager  = wine_server_obj_handle( get_device_manager() );
            req->handle   = wine_server_obj_handle( handle );
            req->user_ptr = wine_server_client_ptr( header + 1 );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
        if (status) FIXME( "set_object_reference failed: %#x\n", status );
    }

    header->ref = ref;
    header->type = type;
    return header + 1;
}

DECLARE_CRITICAL_SECTION(obref_cs);

/***********************************************************************
 *           ObDereferenceObject   (NTOSKRNL.EXE.@)
 */
void WINAPI ObDereferenceObject( void *obj )
{
    struct object_header *header = (struct object_header*)obj - 1;
    LONG ref;

    if (!obj)
    {
        FIXME("NULL obj\n");
        return;
    }

    EnterCriticalSection( &obref_cs );

    ref = --header->ref;
    TRACE( "(%p) ref=%u\n", obj, ref );
    if (!ref)
    {
        if (header->type->release)
        {
            header->type->release( obj );
        }
        else
        {
            SERVER_START_REQ( release_kernel_object )
            {
                req->manager  = wine_server_obj_handle( get_device_manager() );
                req->user_ptr = wine_server_client_ptr( obj );
                if (wine_server_call( req )) FIXME( "failed to release %p\n", obj );
            }
            SERVER_END_REQ;
        }
    }

    LeaveCriticalSection( &obref_cs );
}

void ObReferenceObject( void *obj )
{
    struct object_header *header = (struct object_header*)obj - 1;
    LONG ref;

    if (!obj)
    {
        FIXME("NULL obj\n");
        return;
    }

    EnterCriticalSection( &obref_cs );

    ref = ++header->ref;
    TRACE( "(%p) ref=%u\n", obj, ref );
    if (ref == 1)
    {
        SERVER_START_REQ( grab_kernel_object )
        {
            req->manager  = wine_server_obj_handle( get_device_manager() );
            req->user_ptr = wine_server_client_ptr( obj );
            if (wine_server_call( req )) FIXME( "failed to grab %p reference\n", obj );
        }
        SERVER_END_REQ;
    }

    LeaveCriticalSection( &obref_cs );
}

/***********************************************************************
 *           ObGetObjectType (NTOSKRNL.EXE.@)
 */
POBJECT_TYPE WINAPI ObGetObjectType( void *object )
{
    struct object_header *header = (struct object_header *)object - 1;
    return header->type;
}

static const POBJECT_TYPE *known_types[] =
{
    &ExEventObjectType,
    &ExSemaphoreObjectType,
    &IoDeviceObjectType,
    &IoDriverObjectType,
    &IoFileObjectType,
    &PsProcessType,
    &PsThreadType,
    &SeTokenObjectType
};

DECLARE_CRITICAL_SECTION(handle_map_cs);

NTSTATUS kernel_object_from_handle( HANDLE handle, POBJECT_TYPE type, void **ret )
{
    void *obj;
    NTSTATUS status;

    EnterCriticalSection( &handle_map_cs );

    SERVER_START_REQ( get_kernel_object_ptr )
    {
        req->manager = wine_server_obj_handle( get_device_manager() );
        req->handle  = wine_server_obj_handle( handle );
        status = wine_server_call( req );
        obj = wine_server_get_ptr( reply->user_ptr );
    }
    SERVER_END_REQ;
    if (status)
    {
        LeaveCriticalSection( &handle_map_cs );
        return status;
    }

    if (!obj)
    {
        char buf[256];
        OBJECT_TYPE_INFORMATION *type_info = (OBJECT_TYPE_INFORMATION *)buf;
        ULONG size;

        status = NtQueryObject( handle, ObjectTypeInformation, buf, sizeof(buf), &size );
        if (status)
        {
            LeaveCriticalSection( &handle_map_cs );
            return status;
        }
        if (!type)
        {
            size_t i;
            for (i = 0; i < ARRAY_SIZE(known_types); i++)
            {
                type = *known_types[i];
                if (!RtlCompareUnicodeStrings( type->name, lstrlenW(type->name), type_info->TypeName.Buffer,
                                               type_info->TypeName.Length / sizeof(WCHAR), FALSE ))
                    break;
            }
            if (i == ARRAY_SIZE(known_types))
            {
                FIXME("Unsupported type %s\n", debugstr_us(&type_info->TypeName));
                LeaveCriticalSection( &handle_map_cs );
                return STATUS_INVALID_HANDLE;
            }
        }
        else if (RtlCompareUnicodeStrings( type->name, lstrlenW(type->name), type_info->TypeName.Buffer,
                                           type_info->TypeName.Length / sizeof(WCHAR), FALSE) )
        {
            LeaveCriticalSection( &handle_map_cs );
            return STATUS_OBJECT_TYPE_MISMATCH;
        }

        if (type->constructor)
            obj = type->constructor( handle );
        else
        {
            FIXME( "No constructor for type %s\n", debugstr_w(type->name) );
            obj = alloc_kernel_object( type, handle, 0, 0 );
        }
        if (!obj) status = STATUS_NO_MEMORY;
    }
    else if (type && ObGetObjectType( obj ) != type) status = STATUS_OBJECT_TYPE_MISMATCH;

    LeaveCriticalSection( &handle_map_cs );
    if (!status) *ret = obj;
    return status;
}

/***********************************************************************
 *           ObReferenceObjectByHandle    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObReferenceObjectByHandle( HANDLE handle, ACCESS_MASK access,
                                           POBJECT_TYPE type,
                                           KPROCESSOR_MODE mode, void **ptr,
                                           POBJECT_HANDLE_INFORMATION info )
{
    NTSTATUS status;

    TRACE( "%p %x %p %d %p %p\n", handle, access, type, mode, ptr, info );

    if (mode != KernelMode)
    {
        FIXME("UserMode access not implemented\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    status = kernel_object_from_handle( handle, type, ptr );
    if (!status) ObReferenceObject( *ptr );
    return status;
}

/***********************************************************************
 *           ObOpenObjectByPointer    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObOpenObjectByPointer( void *obj, ULONG attr, ACCESS_STATE *access_state,
                                       ACCESS_MASK access, POBJECT_TYPE type,
                                       KPROCESSOR_MODE mode, HANDLE *handle )
{
    NTSTATUS status;

    TRACE( "%p %x %p %x %p %d %p\n", obj, attr, access_state, access, type, mode, handle );

    if (mode != KernelMode)
    {
        FIXME( "UserMode access not implemented\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (attr & ~OBJ_KERNEL_HANDLE) FIXME( "access %x not supported\n", access );
    if (access_state) FIXME( "access_state not implemented\n" );

    if (type && ObGetObjectType( obj ) != type) return STATUS_OBJECT_TYPE_MISMATCH;

    SERVER_START_REQ( get_kernel_object_handle )
    {
        req->manager  = wine_server_obj_handle( get_device_manager() );
        req->user_ptr = wine_server_client_ptr( obj );
        req->access   = access;
        if (!(status = wine_server_call( req )))
            *handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;
    return status;
}


static void *create_file_object( HANDLE handle );

static const WCHAR file_type_name[] = {'F','i','l','e',0};

static struct _OBJECT_TYPE file_type = {
    file_type_name,
    create_file_object
};

POBJECT_TYPE IoFileObjectType = &file_type;

static void *create_file_object( HANDLE handle )
{
    FILE_OBJECT *file;
    if (!(file = alloc_kernel_object( IoFileObjectType, handle, sizeof(*file), 0 ))) return NULL;
    file->Type = 5;  /* MSDN */
    file->Size = sizeof(*file);
    return file;
}

DECLARE_CRITICAL_SECTION(irp_completion_cs);

static void WINAPI cancel_completed_irp( DEVICE_OBJECT *device, IRP *irp )
{
    TRACE( "(%p %p)\n", device, irp );

    IoReleaseCancelSpinLock(irp->CancelIrql);

    irp->IoStatus.u.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

/* transfer result of IRP back to wineserver */
static NTSTATUS WINAPI dispatch_irp_completion( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    HANDLE irp_handle = context;
    void *out_buff = irp->UserBuffer;
    NTSTATUS status;

    if (irp->Flags & IRP_WRITE_OPERATION)
        out_buff = NULL;  /* do not transfer back input buffer */

    EnterCriticalSection( &irp_completion_cs );

    SERVER_START_REQ( set_irp_result )
    {
        req->handle   = wine_server_obj_handle( irp_handle );
        req->status   = irp->IoStatus.u.Status;
        req->size     = irp->IoStatus.Information;
        if (!NT_ERROR(irp->IoStatus.u.Status))
        {
            if (out_buff) wine_server_add_data( req, out_buff, irp->IoStatus.Information );
        }
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        /* IRP is complete, but server may have already ordered cancel call. In such case,
         * it will return STATUS_MORE_PROCESSING_REQUIRED, leaving the IRP alive until
         * cancel frees it. */
        if (irp->Cancel)
            status = STATUS_SUCCESS;
        else
            IoSetCancelRoutine( irp, cancel_completed_irp );
    }

    if (irp->UserBuffer != irp->AssociatedIrp.SystemBuffer)
    {
        HeapFree( GetProcessHeap(), 0, irp->UserBuffer );
        irp->UserBuffer = NULL;
    }

    LeaveCriticalSection( &irp_completion_cs );
    return status;
}

struct dispatch_context
{
    irp_params_t params;
    HANDLE handle;
    IRP   *irp;
    ULONG  in_size;
    void  *in_buff;
};

static void dispatch_irp( DEVICE_OBJECT *device, IRP *irp, struct dispatch_context *context )
{
    LARGE_INTEGER count;

    IoSetCompletionRoutine( irp, dispatch_irp_completion, context->handle, TRUE, TRUE, TRUE );
    context->handle = 0;

    KeQueryTickCount( &count );  /* update the global KeTickCount */

    context->irp = irp;
    device->CurrentIrp = irp;
    KeEnterCriticalRegion();
    IoCallDriver( device, irp );
    KeLeaveCriticalRegion();
    device->CurrentIrp = NULL;
}

/* process a create request for a given file */
static NTSTATUS dispatch_create( struct dispatch_context *context )
{
    IRP *irp;
    IO_STACK_LOCATION *irpsp;
    FILE_OBJECT *file;
    DEVICE_OBJECT *device = wine_server_get_ptr( context->params.create.device );
    HANDLE handle = wine_server_ptr_handle( context->params.create.file );

    if (!(file = alloc_kernel_object( IoFileObjectType, handle, sizeof(*file), 0 )))
        return STATUS_NO_MEMORY;

    TRACE( "device %p -> file %p\n", device, file );

    file->Type = 5;  /* MSDN */
    file->Size = sizeof(*file);
    file->DeviceObject = device;

    device = IoGetAttachedDevice( device );

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE ))) return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = IRP_MJ_CREATE;
    irpsp->FileObject = file;
    irpsp->Parameters.Create.SecurityContext = NULL;  /* FIXME */
    irpsp->Parameters.Create.Options = context->params.create.options;
    irpsp->Parameters.Create.ShareAccess = context->params.create.sharing;
    irpsp->Parameters.Create.FileAttributes = 0;
    irpsp->Parameters.Create.EaLength = 0;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CREATE_OPERATION;
    dispatch_irp( device, irp, context );

    return STATUS_SUCCESS;
}

/* process a close request for a given file */
static NTSTATUS dispatch_close( struct dispatch_context *context )
{
    IRP *irp;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.close.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "device %p file %p\n", device, file );

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE )))
    {
        ObDereferenceObject( file );
        return STATUS_NO_MEMORY;
    }

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = IRP_MJ_CLOSE;
    irpsp->FileObject = file;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CLOSE_OPERATION;
    dispatch_irp( device, irp, context );

    return STATUS_SUCCESS;
}

/* process a read request for a given device */
static NTSTATUS dispatch_read( struct dispatch_context *context )
{
    IRP *irp;
    void *out_buff;
    LARGE_INTEGER offset;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.read.file );
    ULONG out_size = context->params.read.out_size;

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "device %p file %p size %u\n", device, file, out_size );

    if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;

    offset.QuadPart = context->params.read.pos;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ, device, out_buff, out_size,
                                              &offset, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, out_buff );
        return STATUS_NO_MEMORY;
    }

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->FileObject = file;
    irpsp->Parameters.Read.Key = context->params.read.key;

    irp->Flags |= IRP_READ_OPERATION;
    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate out_buff */
    dispatch_irp( device, irp, context );

    return STATUS_SUCCESS;
}

/* process a write request for a given device */
static NTSTATUS dispatch_write( struct dispatch_context *context )
{
    IRP *irp;
    LARGE_INTEGER offset;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.write.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "device %p file %p size %u\n", device, file, context->in_size );

    offset.QuadPart = context->params.write.pos;

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_WRITE, device, context->in_buff, context->in_size,
                                              &offset, NULL, NULL )))
        return STATUS_NO_MEMORY;
    context->in_buff = NULL;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->FileObject = file;
    irpsp->Parameters.Write.Key = context->params.write.key;

    irp->Flags |= IRP_WRITE_OPERATION;
    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate in_buff */
    dispatch_irp( device, irp, context );

    return STATUS_SUCCESS;
}

/* process a flush request for a given device */
static NTSTATUS dispatch_flush( struct dispatch_context *context )
{
    IRP *irp;
    IO_STACK_LOCATION *irpsp;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.flush.file );

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "device %p file %p\n", device, file );

    if (!(irp = IoBuildSynchronousFsdRequest( IRP_MJ_FLUSH_BUFFERS, device, NULL, 0,
                                              NULL, NULL, NULL )))
        return STATUS_NO_MEMORY;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->FileObject = file;

    dispatch_irp( device, irp, context );

    return STATUS_SUCCESS;
}

/* process an ioctl request for a given device */
static NTSTATUS dispatch_ioctl( struct dispatch_context *context )
{
    IO_STACK_LOCATION *irpsp;
    IRP *irp;
    void *out_buff = NULL;
    void *to_free = NULL;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.ioctl.file );
    ULONG out_size = context->params.ioctl.out_size;

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "ioctl %x device %p file %p in_size %u out_size %u\n",
           context->params.ioctl.code, device, file, context->in_size, out_size );

    if (out_size)
    {
        if ((context->params.ioctl.code & 3) != METHOD_BUFFERED)
        {
            if (context->in_size < out_size) return STATUS_INVALID_DEVICE_REQUEST;
            context->in_size -= out_size;
            if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;
            memcpy( out_buff, (char *)context->in_buff + context->in_size, out_size );
        }
        else if (out_size > context->in_size)
        {
            if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;
            memcpy( out_buff, context->in_buff, context->in_size );
            to_free = context->in_buff;
            context->in_buff = out_buff;
        }
        else
        {
            out_buff = context->in_buff;
            out_size = context->in_size;
        }
    }

    irp = IoBuildDeviceIoControlRequest( context->params.ioctl.code, device, context->in_buff,
                                         context->in_size, out_buff, out_size, FALSE, NULL, NULL );
    if (!irp)
    {
        HeapFree( GetProcessHeap(), 0, out_buff );
        return STATUS_NO_MEMORY;
    }

    if (out_size && (context->params.ioctl.code & 3) != METHOD_BUFFERED)
        HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, context->in_buff, context->in_size );

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->FileObject = file;

    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = context->in_buff;
    context->in_buff = NULL;

    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate in_buff */
    dispatch_irp( device, irp, context );

    HeapFree( GetProcessHeap(), 0, to_free );
    return STATUS_SUCCESS;
}

static NTSTATUS dispatch_free( struct dispatch_context *context )
{
    void *obj = wine_server_get_ptr( context->params.free.obj );
    TRACE( "freeing %p object\n", obj );
    free_kernel_object( obj );
    return STATUS_SUCCESS;
}

static NTSTATUS dispatch_cancel( struct dispatch_context *context )
{
    IRP *irp = wine_server_get_ptr( context->params.cancel.irp );

    TRACE( "%p\n", irp );

    EnterCriticalSection( &irp_completion_cs );
    IoCancelIrp( irp );
    LeaveCriticalSection( &irp_completion_cs );
    return STATUS_SUCCESS;
}

typedef NTSTATUS (*dispatch_func)( struct dispatch_context *context );

static const dispatch_func dispatch_funcs[] =
{
    NULL,              /* IRP_CALL_NONE */
    dispatch_create,   /* IRP_CALL_CREATE */
    dispatch_close,    /* IRP_CALL_CLOSE */
    dispatch_read,     /* IRP_CALL_READ */
    dispatch_write,    /* IRP_CALL_WRITE */
    dispatch_flush,    /* IRP_CALL_FLUSH */
    dispatch_ioctl,    /* IRP_CALL_IOCTL */
    dispatch_free,     /* IRP_CALL_FREE */
    dispatch_cancel    /* IRP_CALL_CANCEL */
};

/* helper function to update service status */
static void set_service_status( SERVICE_STATUS_HANDLE handle, DWORD state, DWORD accepted )
{
    SERVICE_STATUS status;
    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = state;
    status.dwControlsAccepted        = accepted;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = (state == SERVICE_START_PENDING) ? 10000 : 0;
    SetServiceStatus( handle, &status );
}

static void unload_driver( struct wine_rb_entry *entry, void *context )
{
    struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );
    SERVICE_STATUS_HANDLE service_handle = driver->service_handle;
    LDR_MODULE *ldr;

    if (!service_handle) return;    /* not a service */

    TRACE("%s\n", debugstr_us(&driver->driver_obj.DriverName));

    if (!driver->driver_obj.DriverUnload)
    {
        TRACE( "driver %s does not support unloading\n", debugstr_us(&driver->driver_obj.DriverName) );
        return;
    }

    ldr = driver->driver_obj.DriverSection;

    set_service_status( service_handle, SERVICE_STOP_PENDING, 0 );

    TRACE_(relay)( "\1Call driver unload %p (obj=%p)\n", driver->driver_obj.DriverUnload, &driver->driver_obj );

    driver->driver_obj.DriverUnload( &driver->driver_obj );

    TRACE_(relay)( "\1Ret  driver unload %p (obj=%p)\n", driver->driver_obj.DriverUnload, &driver->driver_obj );

    FreeLibrary( ldr->BaseAddress );
    IoDeleteDriver( &driver->driver_obj );

    set_service_status( service_handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( (void *)service_handle );
}

PEPROCESS PsInitialSystemProcess = NULL;

/***********************************************************************
 *           wine_ntoskrnl_main_loop   (Not a Windows API)
 */
NTSTATUS CDECL wine_ntoskrnl_main_loop( HANDLE stop_event )
{
    HANDLE manager = get_device_manager();
    struct dispatch_context context;
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE handles[2];

    context.in_size = 4096;
    context.in_buff = NULL;

    /* Set the system process global before setting up the request thread trickery  */
    PsInitialSystemProcess = IoGetCurrentProcess();
    request_thread = GetCurrentThreadId();

    pnp_manager_start();

    handles[0] = stop_event;
    handles[1] = manager;

    for (;;)
    {
        NtCurrentTeb()->Reserved5[1] = NULL;
        if (!context.in_buff && !(context.in_buff = HeapAlloc( GetProcessHeap(), 0, context.in_size )))
        {
            ERR( "failed to allocate buffer\n" );
            status = STATUS_NO_MEMORY;
            goto done;
        }

        SERVER_START_REQ( get_next_device_request )
        {
            req->manager  = wine_server_obj_handle( manager );
            req->prev     = wine_server_obj_handle( context.handle );
            req->user_ptr = wine_server_client_ptr( context.irp );
            req->status   = status;
            wine_server_set_reply( req, context.in_buff, context.in_size );
            if (!(status = wine_server_call( req )))
            {
                context.handle  = wine_server_ptr_handle( reply->next );
                context.params  = reply->params;
                context.in_size = reply->in_size;
                client_tid = reply->client_tid;
                NtCurrentTeb()->Reserved5[1] = wine_server_get_ptr( reply->client_thread );
            }
            else
            {
                context.handle = 0; /* no previous irp */
                if (status == STATUS_BUFFER_OVERFLOW)
                    context.in_size = reply->in_size;
            }
            context.irp = NULL;
        }
        SERVER_END_REQ;

        switch (status)
        {
        case STATUS_SUCCESS:
            assert( context.params.type != IRP_CALL_NONE && context.params.type < ARRAY_SIZE(dispatch_funcs) );
            status = dispatch_funcs[context.params.type]( &context );
            if (!context.in_buff) context.in_size = 4096;
            break;
        case STATUS_BUFFER_OVERFLOW:
            HeapFree( GetProcessHeap(), 0, context.in_buff );
            context.in_buff = NULL;
            /* restart with larger buffer */
            break;
        case STATUS_PENDING:
            for (;;)
            {
                DWORD ret = WaitForMultipleObjectsEx( 2, handles, FALSE, INFINITE, TRUE );
                if (ret == WAIT_OBJECT_0)
                {
                    HeapFree( GetProcessHeap(), 0, context.in_buff );
                    status = STATUS_SUCCESS;
                    goto done;
                }
                if (ret != WAIT_IO_COMPLETION) break;
            }
            break;
        }
    }

done:
    /* Native PnP drivers expect that all of their devices will be removed when
     * their unload routine is called, so we must stop the PnP manager first. */
    pnp_manager_stop();
    wine_rb_destroy( &wine_drivers, unload_driver, NULL );
    return status;
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
 *           IoStopTimer   (NTOSKRNL.EXE.@)
 */
void WINAPI IoStopTimer(PDEVICE_OBJECT DeviceObject)
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


struct _IO_WORKITEM
{
    DEVICE_OBJECT *device;
    PIO_WORKITEM_ROUTINE worker;
    void *context;
};

/***********************************************************************
 *           IoAllocateWorkItem  (NTOSKRNL.EXE.@)
 */
PIO_WORKITEM WINAPI IoAllocateWorkItem( PDEVICE_OBJECT device )
{
    PIO_WORKITEM work_item;

    TRACE( "%p\n", device );

    if (!(work_item = ExAllocatePool( PagedPool, sizeof(*work_item) ))) return NULL;
    work_item->device = device;
    return work_item;
}


/***********************************************************************
 *           IoFreeWorkItem  (NTOSKRNL.EXE.@)
 */
void WINAPI IoFreeWorkItem( PIO_WORKITEM work_item )
{
    TRACE( "%p\n", work_item );
    ExFreePool( work_item );
}


static void WINAPI run_work_item_worker(TP_CALLBACK_INSTANCE *instance, void *context)
{
    PIO_WORKITEM work_item = context;
    DEVICE_OBJECT *device = work_item->device;

    TRACE( "%p: calling %p(%p %p)\n", work_item, work_item->worker, device, work_item->context );
    work_item->worker( device, work_item->context );
    TRACE( "done\n" );

    ObDereferenceObject( device );
}

/***********************************************************************
 *           IoQueueWorkItem  (NTOSKRNL.EXE.@)
 */
void WINAPI IoQueueWorkItem( PIO_WORKITEM work_item, PIO_WORKITEM_ROUTINE worker,
                             WORK_QUEUE_TYPE type, void *context )
{
    TRACE( "%p %p %u %p\n", work_item, worker, type, context );

    ObReferenceObject( work_item->device );
    work_item->worker = worker;
    work_item->context = context;
    TrySubmitThreadpoolCallback( run_work_item_worker, work_item, NULL );
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

void WINAPI IoDetachDevice( DEVICE_OBJECT *device )
{
    device->AttachedDevice = NULL;
}

/***********************************************************************
 *           IoAttachDeviceToDeviceStack  (NTOSKRNL.EXE.@)
 */
PDEVICE_OBJECT WINAPI IoAttachDeviceToDeviceStack( DEVICE_OBJECT *source,
                                                   DEVICE_OBJECT *target )
{
    TRACE( "%p, %p\n", source, target );
    target = IoGetAttachedDevice( target );
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
    irpsp->DeviceObject = NULL;
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
    irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    return irp;
}

/***********************************************************************
 *           IoBuildAsynchronousFsdRequest  (NTOSKRNL.EXE.@)
 */
PIRP WINAPI IoBuildAsynchronousFsdRequest(ULONG majorfunc, DEVICE_OBJECT *device,
                                          void *buffer, ULONG length, LARGE_INTEGER *startoffset,
                                          IO_STATUS_BLOCK *iosb)
{
    PIRP irp;
    PIO_STACK_LOCATION irpsp;

    TRACE( "(%d %p %p %d %p %p)\n", majorfunc, device, buffer, length, startoffset, iosb );

    if (!(irp = IoAllocateIrp( device->StackSize, FALSE ))) return NULL;

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = majorfunc;
    irpsp->DeviceObject = NULL;
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
        irpsp->Parameters.Read.ByteOffset.QuadPart = startoffset ? startoffset->QuadPart : 0;
        break;
    case IRP_MJ_WRITE:
        irpsp->Parameters.Write.Length = length;
        irpsp->Parameters.Write.ByteOffset.QuadPart = startoffset ? startoffset->QuadPart : 0;
        break;
    }
    irp->RequestorMode = KernelMode;
    irp->UserIosb = iosb;
    irp->UserEvent = NULL;
    irp->UserBuffer = buffer;
    irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    return irp;
}



/***********************************************************************
 *           IoBuildSynchronousFsdRequest  (NTOSKRNL.EXE.@)
 */
PIRP WINAPI IoBuildSynchronousFsdRequest(ULONG majorfunc, PDEVICE_OBJECT device,
                                         PVOID buffer, ULONG length, PLARGE_INTEGER startoffset,
                                         PKEVENT event, PIO_STATUS_BLOCK iosb)
{
    IRP *irp;

    TRACE("(%d %p %p %d %p %p)\n", majorfunc, device, buffer, length, startoffset, iosb);

    irp = IoBuildAsynchronousFsdRequest( majorfunc, device, buffer, length, startoffset, iosb );
    if (!irp) return NULL;

    irp->UserEvent = event;
    return irp;
}

static void build_driver_keypath( const WCHAR *name, UNICODE_STRING *keypath )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    WCHAR *str;

    /* Check what prefix is present */
    if (wcsncmp( name, servicesW, lstrlenW(servicesW) ) == 0)
    {
        FIXME( "Driver name %s is malformed as the keypath\n", debugstr_w(name) );
        RtlCreateUnicodeString( keypath, name );
        return;
    }
    if (wcsncmp( name, driverW, lstrlenW(driverW) ) == 0)
        name += lstrlenW(driverW);
    else
        FIXME( "Driver name %s does not properly begin with \\Driver\\\n", debugstr_w(name) );

    str = HeapAlloc( GetProcessHeap(), 0, sizeof(servicesW) + lstrlenW(name)*sizeof(WCHAR));
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


static void free_driver_object( void *obj )
{
    struct wine_driver *driver = obj;
    RtlFreeUnicodeString( &driver->driver_obj.DriverName );
    RtlFreeUnicodeString( &driver->driver_obj.DriverExtension->ServiceKeyName );
    free_kernel_object( driver );
}

static const WCHAR driver_type_name[] = {'D','r','i','v','e','r',0};

static struct _OBJECT_TYPE driver_type =
{
    driver_type_name,
    NULL,
    free_driver_object
};

POBJECT_TYPE IoDriverObjectType = &driver_type;


/***********************************************************************
 *           IoCreateDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDriver( UNICODE_STRING *name, PDRIVER_INITIALIZE init )
{
    struct wine_driver *driver;
    NTSTATUS status;
    unsigned int i;

    TRACE("(%s, %p)\n", debugstr_us(name), init);

    if (!(driver = alloc_kernel_object( IoDriverObjectType, NULL, sizeof(*driver), 1 )))
        return STATUS_NO_MEMORY;

    if ((status = RtlDuplicateUnicodeString( 1, name, &driver->driver_obj.DriverName )))
    {
        free_kernel_object( driver );
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
        ObDereferenceObject( driver );
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

    ObDereferenceObject( driver_object );
}


static const WCHAR device_type_name[] = {'D','e','v','i','c','e',0};

static struct _OBJECT_TYPE device_type =
{
    device_type_name,
};

POBJECT_TYPE IoDeviceObjectType = &device_type;


/***********************************************************************
 *           IoCreateDevice   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDevice( DRIVER_OBJECT *driver, ULONG ext_size,
                                UNICODE_STRING *name, DEVICE_TYPE type,
                                ULONG characteristics, BOOLEAN exclusive,
                                DEVICE_OBJECT **ret_device )
{
    static const WCHAR auto_format[] = {'\\','D','e','v','i','c','e','\\','%','0','8','x',0};
    NTSTATUS status;
    DEVICE_OBJECT *device;
    HANDLE manager = get_device_manager();
    static unsigned int auto_idx = 0;
    WCHAR autoW[17];

    TRACE( "(%p, %u, %s, %u, %x, %u, %p)\n",
           driver, ext_size, debugstr_us(name), type, characteristics, exclusive, ret_device );

    if (!(device = alloc_kernel_object( IoDeviceObjectType, NULL, sizeof(DEVICE_OBJECT) + ext_size, 1 )))
        return STATUS_NO_MEMORY;

    device->DriverObject    = driver;
    device->DeviceExtension = device + 1;
    device->DeviceType      = type;
    device->StackSize       = 1;

    if (characteristics & FILE_AUTOGENERATED_DEVICE_NAME)
    {
        do
        {
            swprintf( autoW, ARRAY_SIZE(autoW), auto_format, auto_idx++ );
            SERVER_START_REQ( create_device )
            {
                req->rootdir    = 0;
                req->manager    = wine_server_obj_handle( manager );
                req->user_ptr   = wine_server_client_ptr( device );
                wine_server_add_data( req, autoW, lstrlenW(autoW) * sizeof(WCHAR) );
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        } while (status == STATUS_OBJECT_NAME_COLLISION);
    }
    else
    {
        SERVER_START_REQ( create_device )
        {
            req->rootdir    = 0;
            req->manager    = wine_server_obj_handle( manager );
            req->user_ptr   = wine_server_client_ptr( device );
            if (name) wine_server_add_data( req, name->Buffer, name->Length );
            status = wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    if (status)
    {
        free_kernel_object( device );
        return status;
    }

    device->NextDevice   = driver->DeviceObject;
    driver->DeviceObject = device;

    *ret_device = device;
    return STATUS_SUCCESS;
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
        req->manager = wine_server_obj_handle( get_device_manager() );
        req->device  = wine_server_client_ptr( device );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    if (status == STATUS_SUCCESS)
    {
        DEVICE_OBJECT **prev = &device->DriverObject->DeviceObject;
        while (*prev && *prev != device) prev = &(*prev)->NextDevice;
        if (*prev) *prev = (*prev)->NextDevice;
        ObDereferenceObject( device );
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
 *           IoCallDriver   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCallDriver( DEVICE_OBJECT *device, IRP *irp )
{
    PDRIVER_DISPATCH dispatch;
    IO_STACK_LOCATION *irpsp;
    NTSTATUS status;

    --irp->CurrentLocation;
    irpsp = --irp->Tail.Overlay.s.u2.CurrentStackLocation;
    irpsp->DeviceObject = device;
    dispatch = device->DriverObject->MajorFunction[irpsp->MajorFunction];

    TRACE_(relay)( "\1Call driver dispatch %p (device=%p,irp=%p)\n", dispatch, device, irp );

    status = dispatch( device, irp );

    TRACE_(relay)( "\1Ret  driver dispatch %p (device=%p,irp=%p) retval=%08x\n",
                   dispatch, device, irp, status );

    return status;
}


/***********************************************************************
 *           IofCallDriver   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL_WRAPPER( IofCallDriver, 8 )
NTSTATUS FASTCALL IofCallDriver( DEVICE_OBJECT *device, IRP *irp )
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
 *           IoGetStackLimits    (NTOSKRNL.EXE.@)
 */
void WINAPI IoGetStackLimits(ULONG_PTR *low, ULONG_PTR *high)
{
    TEB *teb = NtCurrentTeb();

    TRACE( "%p %p\n", low, high );

    *low  = (DWORD_PTR)teb->Tib.StackLimit;
    *high = (DWORD_PTR)teb->Tib.StackBase;
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
 *           IoReportResourceForDetection   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoReportResourceForDetection( DRIVER_OBJECT *drv_obj, CM_RESOURCE_LIST *drv_list, ULONG drv_size,
                                              DEVICE_OBJECT *dev_obj, CM_RESOURCE_LIST *dev_list, ULONG dev_size,
                                              BOOLEAN *conflict )
{
    FIXME( "(%p, %p, %u, %p, %p, %u, %p): stub\n", drv_obj, drv_list, drv_size,
           dev_obj, dev_list, dev_size, conflict );

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           IoReportResourceUsage    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoReportResourceUsage( UNICODE_STRING *name, DRIVER_OBJECT *drv_obj, CM_RESOURCE_LIST *drv_list,
                                       ULONG drv_size, DRIVER_OBJECT *dev_obj, CM_RESOURCE_LIST *dev_list,
                                       ULONG dev_size, BOOLEAN overwrite, BOOLEAN *conflict )
{
    FIXME( "(%s, %p, %p, %u, %p, %p, %u, %d, %p): stub\n", debugstr_us(name),
           drv_obj, drv_list, drv_size, dev_obj, dev_list, dev_size, overwrite, conflict );

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
        if (routine)
        {
            if ((irpsp->Control & SL_INVOKE_ON_SUCCESS) && STATUS_SUCCESS == status)
                call_flag = 1;
            if ((irpsp->Control & SL_INVOKE_ON_ERROR) && STATUS_SUCCESS != status)
                call_flag = 1;
            if ((irpsp->Control & SL_INVOKE_ON_CANCEL) && irp->Cancel)
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
    if (irp->UserEvent) KeSetEvent( irp->UserEvent, IO_NO_INCREMENT, FALSE );

    IoFreeIrp( irp );
}


/***********************************************************************
 *           IofCompleteRequest   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL_WRAPPER( IofCompleteRequest, 8 )
void FASTCALL IofCompleteRequest( IRP *irp, UCHAR priority_boost )
{
    TRACE( "%p %u\n", irp, priority_boost );
    IoCompleteRequest( irp, priority_boost );
}


/***********************************************************************
 *           IoCancelIrp   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI IoCancelIrp( IRP *irp )
{
    PDRIVER_CANCEL cancel_routine;
    KIRQL irql;

    TRACE( "(%p)\n", irp );

    IoAcquireCancelSpinLock( &irql );
    irp->Cancel = TRUE;
    if (!(cancel_routine = IoSetCancelRoutine( irp, NULL )))
    {
        IoReleaseCancelSpinLock( irp->CancelIrql );
        return FALSE;
    }

    /* CancelRoutine is responsible for calling IoReleaseCancelSpinLock */
    irp->CancelIrql = irql;
    cancel_routine( IoGetCurrentIrpStackLocation(irp)->DeviceObject, irp );
    return TRUE;
}


/***********************************************************************
 *           InterlockedCompareExchange   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL_WRAPPER( NTOSKRNL_InterlockedCompareExchange, 12 )
LONG FASTCALL NTOSKRNL_InterlockedCompareExchange( LONG volatile *dest, LONG xchg, LONG compare )
{
    return InterlockedCompareExchange( dest, xchg, compare );
}


/***********************************************************************
 *           InterlockedDecrement   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL1_WRAPPER( NTOSKRNL_InterlockedDecrement )
LONG FASTCALL NTOSKRNL_InterlockedDecrement( LONG volatile *dest )
{
    return InterlockedDecrement( dest );
}


/***********************************************************************
 *           InterlockedExchange   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL_WRAPPER( NTOSKRNL_InterlockedExchange, 8 )
LONG FASTCALL NTOSKRNL_InterlockedExchange( LONG volatile *dest, LONG val )
{
    return InterlockedExchange( dest, val );
}


/***********************************************************************
 *           InterlockedExchangeAdd   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL_WRAPPER( NTOSKRNL_InterlockedExchangeAdd, 8 )
LONG FASTCALL NTOSKRNL_InterlockedExchangeAdd( LONG volatile *dest, LONG incr )
{
    return InterlockedExchangeAdd( dest, incr );
}


/***********************************************************************
 *           InterlockedIncrement   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL1_WRAPPER( NTOSKRNL_InterlockedIncrement )
LONG FASTCALL NTOSKRNL_InterlockedIncrement( LONG volatile *dest )
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

static void initialize_lookaside_list( GENERAL_LOOKASIDE *lookaside, PALLOCATE_FUNCTION allocate, PFREE_FUNCTION free,
                                       ULONG type, SIZE_T size, ULONG tag )
{

    RtlInitializeSListHead( &lookaside->u.ListHead );
    lookaside->Depth                 = 4;
    lookaside->MaximumDepth          = 256;
    lookaside->TotalAllocates        = 0;
    lookaside->u2.AllocateMisses     = 0;
    lookaside->TotalFrees            = 0;
    lookaside->u3.FreeMisses         = 0;
    lookaside->Type                  = type;
    lookaside->Tag                   = tag;
    lookaside->Size                  = size;
    lookaside->u4.Allocate           = allocate ? allocate : ExAllocatePoolWithTag;
    lookaside->u5.Free               = free ? free : ExFreePool;
    lookaside->LastTotalAllocates    = 0;
    lookaside->u6.LastAllocateMisses = 0;

    /* FIXME: insert in global list of lookadside lists */
}

/***********************************************************************
 *           ExInitializeNPagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExInitializeNPagedLookasideList(PNPAGED_LOOKASIDE_LIST lookaside,
                                            PALLOCATE_FUNCTION allocate,
                                            PFREE_FUNCTION free,
                                            ULONG flags,
                                            SIZE_T size,
                                            ULONG tag,
                                            USHORT depth)
{
    TRACE( "%p, %p, %p, %u, %lu, %u, %u\n", lookaside, allocate, free, flags, size, tag, depth );
    initialize_lookaside_list( &lookaside->L, allocate, free, NonPagedPool | flags, size, tag );
}

/***********************************************************************
 *           ExInitializePagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExInitializePagedLookasideList(PPAGED_LOOKASIDE_LIST lookaside,
                                           PALLOCATE_FUNCTION allocate,
                                           PFREE_FUNCTION free,
                                           ULONG flags,
                                           SIZE_T size,
                                           ULONG tag,
                                           USHORT depth)
{
    TRACE( "%p, %p, %p, %u, %lu, %u, %u\n", lookaside, allocate, free, flags, size, tag, depth );
    initialize_lookaside_list( &lookaside->L, allocate, free, PagedPool | flags, size, tag );
}

static void delete_lookaside_list( GENERAL_LOOKASIDE *lookaside )
{
    void *entry;
    while ((entry = RtlInterlockedPopEntrySList(&lookaside->u.ListHead)))
        lookaside->u5.FreeEx(entry, (LOOKASIDE_LIST_EX*)lookaside);
}

/***********************************************************************
 *           ExDeleteNPagedLookasideList   (NTOSKRNL.EXE.@)
 */
void WINAPI ExDeleteNPagedLookasideList( PNPAGED_LOOKASIDE_LIST lookaside )
{
    TRACE( "%p\n", lookaside );
    delete_lookaside_list( &lookaside->L );
}


/***********************************************************************
 *           ExDeletePagedLookasideList  (NTOSKRNL.EXE.@)
 */
void WINAPI ExDeletePagedLookasideList( PPAGED_LOOKASIDE_LIST lookaside )
{
    TRACE( "%p\n", lookaside );
    delete_lookaside_list( &lookaside->L );
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
*           FsRtlIsNameInExpression   (NTOSKRNL.EXE.@)
*/
BOOLEAN WINAPI FsRtlIsNameInExpression(PUNICODE_STRING expression, PUNICODE_STRING name,
                                       BOOLEAN ignore, PWCH upcase)
{
    FIXME("stub: %p %p %d %p\n", expression, name, ignore, upcase);
    return FALSE;
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


static void *create_process_object( HANDLE handle )
{
    PEPROCESS process;

    if (!(process = alloc_kernel_object( PsProcessType, handle, sizeof(*process), 0 ))) return NULL;

    process->header.Type = 3;
    process->header.WaitListHead.Blink = INVALID_HANDLE_VALUE; /* mark as kernel object */
    NtQueryInformationProcess( handle, ProcessBasicInformation, &process->info, sizeof(process->info), NULL );
    return process;
}

static const WCHAR process_type_name[] = {'P','r','o','c','e','s','s',0};

static struct _OBJECT_TYPE process_type =
{
    process_type_name,
    create_process_object
};

POBJECT_TYPE PsProcessType = &process_type;


/***********************************************************************
 *           IoGetCurrentProcess / PsGetCurrentProcess   (NTOSKRNL.EXE.@)
 */
PEPROCESS WINAPI IoGetCurrentProcess(void)
{
    return KeGetCurrentThread()->process;
}

/***********************************************************************
 *           PsLookupProcessByProcessId  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsLookupProcessByProcessId( HANDLE processid, PEPROCESS *process )
{
    NTSTATUS status;
    HANDLE handle;

    TRACE( "(%p %p)\n", processid, process );

    if (!(handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, HandleToUlong(processid) )))
        return STATUS_INVALID_PARAMETER;

    status = ObReferenceObjectByHandle( handle, PROCESS_ALL_ACCESS, PsProcessType, KernelMode, (void**)process, NULL );

    NtClose( handle );
    return status;
}

/*********************************************************************
 *           PsGetProcessId    (NTOSKRNL.@)
 */
HANDLE WINAPI PsGetProcessId(PEPROCESS process)
{
    TRACE( "%p -> %lx\n", process, process->info.UniqueProcessId );
    return (HANDLE)process->info.UniqueProcessId;
}

/*********************************************************************
 *           PsGetProcessInheritedFromUniqueProcessId  (NTOSKRNL.@)
 */
HANDLE WINAPI PsGetProcessInheritedFromUniqueProcessId( PEPROCESS process )
{
    HANDLE id = (HANDLE)process->info.InheritedFromUniqueProcessId;
    TRACE( "%p -> %p\n", process, id );
    return id;
}

static void *create_thread_object( HANDLE handle )
{
    THREAD_BASIC_INFORMATION info;
    struct _KTHREAD *thread;
    HANDLE process;

    if (!(thread = alloc_kernel_object( PsThreadType, handle, sizeof(*thread), 0 ))) return NULL;

    thread->header.Type = 6;
    thread->header.WaitListHead.Blink = INVALID_HANDLE_VALUE; /* mark as kernel object */

    if (!NtQueryInformationThread( handle, ThreadBasicInformation, &info, sizeof(info), NULL ))
    {
        thread->id = info.ClientId;
        if ((process = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, HandleToUlong(thread->id.UniqueProcess) )))
        {
            kernel_object_from_handle( process, PsProcessType, (void**)&thread->process );
            NtClose( process );
        }
    }


    return thread;
}

static const WCHAR thread_type_name[] = {'T','h','r','e','a','d',0};

static struct _OBJECT_TYPE thread_type =
{
    thread_type_name,
    create_thread_object
};

POBJECT_TYPE PsThreadType = &thread_type;


/***********************************************************************
 *           KeGetCurrentThread / PsGetCurrentThread   (NTOSKRNL.EXE.@)
 */
PRKTHREAD WINAPI KeGetCurrentThread(void)
{
    struct _KTHREAD *thread = NtCurrentTeb()->Reserved5[1];

    if (!thread)
    {
        HANDLE handle = GetCurrentThread();

        /* FIXME: we shouldn't need it, GetCurrentThread() should be client thread already */
        if (GetCurrentThreadId() == request_thread)
            handle = OpenThread( THREAD_QUERY_INFORMATION, FALSE, client_tid );

        kernel_object_from_handle( handle, PsThreadType, (void**)&thread );
        if (handle != GetCurrentThread()) NtClose( handle );

        NtCurrentTeb()->Reserved5[1] = thread;
    }

    return thread;
}

/*****************************************************
 *           PsLookupThreadByThreadId   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsLookupThreadByThreadId( HANDLE threadid, PETHREAD *thread )
{
    OBJECT_ATTRIBUTES attr;
    CLIENT_ID cid;
    NTSTATUS status;
    HANDLE handle;

    TRACE( "(%p %p)\n", threadid, thread );

    cid.UniqueProcess = 0;
    cid.UniqueThread = threadid;
    InitializeObjectAttributes( &attr, NULL, 0, NULL, NULL );
    status = NtOpenThread( &handle, THREAD_QUERY_INFORMATION, &attr, &cid );
    if (status) return status;

    status = ObReferenceObjectByHandle( handle, THREAD_ALL_ACCESS, PsThreadType, KernelMode, (void**)thread, NULL );

    NtClose( handle );
    return status;
}

/*********************************************************************
 *           PsGetThreadId    (NTOSKRNL.@)
 */
HANDLE WINAPI PsGetThreadId(PETHREAD thread)
{
    TRACE( "%p -> %p\n", thread, thread->kthread.id.UniqueThread );
    return thread->kthread.id.UniqueThread;
}

/*********************************************************************
 *           PsGetThreadProcessId    (NTOSKRNL.@)
 */
HANDLE WINAPI PsGetThreadProcessId( PETHREAD thread )
{
    TRACE( "%p -> %p\n", thread, thread->kthread.id.UniqueProcess );
    return thread->kthread.id.UniqueProcess;
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
 *           KeQueryTimeIncrement   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI KeQueryTimeIncrement(void)
{
    return 10000;
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
 *           KeRevertToUserAffinityThread   (NTOSKRNL.EXE.@)
 */
void WINAPI KeRevertToUserAffinityThread(void)
{
    FIXME("() stub\n");
}


/***********************************************************************
 *           IoRegisterFileSystem   (NTOSKRNL.EXE.@)
 */
VOID WINAPI IoRegisterFileSystem(PDEVICE_OBJECT DeviceObject)
{
    FIXME("(%p): stub\n", DeviceObject);
}

/***********************************************************************
 *           KeExpandKernelStackAndCalloutEx   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeExpandKernelStackAndCalloutEx(PEXPAND_STACK_CALLOUT callout, void *parameter, SIZE_T size,
                                                BOOLEAN wait, void *context)
{
    WARN("(%p %p %lu %x %p) semi-stub: ignoring stack expand\n", callout, parameter, size, wait, context);
    callout(parameter);
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           KeExpandKernelStackAndCallout   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI KeExpandKernelStackAndCallout(PEXPAND_STACK_CALLOUT callout, void *parameter, SIZE_T size)
{
    return KeExpandKernelStackAndCalloutEx(callout, parameter, size, TRUE, NULL);
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
 *           MmBuildMdlForNonPagedPool   (NTOSKRNL.EXE.@)
 */
void WINAPI MmBuildMdlForNonPagedPool(MDL *mdl)
{
    FIXME("stub: %p\n", mdl);
}

/***********************************************************************
 *           MmCreateSection   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI MmCreateSection( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                 LARGE_INTEGER *size, ULONG protect, ULONG alloc_attr,
                                 HANDLE file, FILE_OBJECT *file_obj )
{
    FIXME("%p %#x %p %s %#x %#x %p %p: stub\n", handle, access, attr,
        wine_dbgstr_longlong(size->QuadPart), protect, alloc_attr, file, file_obj);
    return STATUS_NOT_IMPLEMENTED;
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
    return !IsBadReadPtr(VirtualAddress, 1);
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
 *           MmUnmapLockedPages  (NTOSKRNL.EXE.@)
 */
void WINAPI MmUnmapLockedPages( void *base, MDL *mdl )
{
    FIXME( "(%p %p_\n", base, mdl );
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
    ObReferenceObject( *Object = &driver->driver_obj );
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           ObReferenceObjectByPointer   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObReferenceObjectByPointer(void *obj, ACCESS_MASK access,
                                           POBJECT_TYPE type,
                                           KPROCESSOR_MODE mode)
{
    FIXME("(%p, %x, %p, %d): stub\n", obj, access, type, mode);

    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           ObfReferenceObject   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL1_WRAPPER( ObfReferenceObject )
void FASTCALL ObfReferenceObject( void *obj )
{
    ObReferenceObject( obj );
}


/***********************************************************************
 *           ObfDereferenceObject   (NTOSKRNL.EXE.@)
 */
DEFINE_FASTCALL1_WRAPPER( ObfDereferenceObject )
void FASTCALL ObfDereferenceObject( void *obj )
{
    ObDereferenceObject( obj );
}

/***********************************************************************
 *           ObRegisterCallbacks (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObRegisterCallbacks(POB_CALLBACK_REGISTRATION *callBack, void **handle)
{
    FIXME( "stub: %p %p\n", callBack, handle );

    if(handle)
        *handle = UlongToHandle(0xdeadbeaf);

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           ObUnRegisterCallbacks (NTOSKRNL.EXE.@)
 */
void WINAPI ObUnRegisterCallbacks(void *handle)
{
    FIXME( "stub: %p\n", handle );
}

/***********************************************************************
 *           ObGetFilterVersion (NTOSKRNL.EXE.@)
 */
USHORT WINAPI ObGetFilterVersion(void)
{
    FIXME( "stub:\n" );

    return OB_FLT_REGISTRATION_VERSION;
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
    return KeGetCurrentThread()->id.UniqueProcess;
}


/***********************************************************************
 *           PsGetCurrentThreadId   (NTOSKRNL.EXE.@)
 */
HANDLE WINAPI PsGetCurrentThreadId(void)
{
    return KeGetCurrentThread()->id.UniqueThread;
}


/***********************************************************************
 *           PsIsSystemThread   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI PsIsSystemThread(PETHREAD thread)
{
    return thread->kthread.process == PsInitialSystemProcess;
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
        size_t len = min( lstrlenW(info.szCSDVersion)*sizeof(WCHAR), version->MaximumLength );
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
 *           PsRevertToSelf   (NTOSKRNL.EXE.@)
 */
void WINAPI PsRevertToSelf(void)
{
    FIXME("\n");
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
 *           PsSetCreateProcessNotifyRoutineEx   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetCreateProcessNotifyRoutineEx( PCREATE_PROCESS_NOTIFY_ROUTINE_EX callback, BOOLEAN remove )
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
 *           PsReferenceProcessFilePointer  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsReferenceProcessFilePointer(PEPROCESS process, FILE_OBJECT **file)
{
    FIXME("%p %p\n", process, file);
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           PsTerminateSystemThread   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsTerminateSystemThread(NTSTATUS status)
{
    TRACE("status %#x.\n", status);
    ExitThread( status );
}


/***********************************************************************
 *           PsSuspendProcess   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSuspendProcess(PEPROCESS process)
{
    FIXME("stub: %p\n", process);
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           PsResumeProcess   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsResumeProcess(PEPROCESS process)
{
    FIXME("stub: %p\n", process);
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
 *           MmIsThisAnNtAsSystem   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI MmIsThisAnNtAsSystem(void)
{
    TRACE("\n");
    return FALSE;
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
 *          KeSetTargetProcessorDpc   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetTargetProcessorDpc(PRKDPC dpc, CCHAR number)
{
    FIXME("%p, %d stub\n", dpc, number);
}

/***********************************************************************
 *           READ_REGISTER_BUFFER_UCHAR   (NTOSKRNL.EXE.@)
 */
VOID WINAPI READ_REGISTER_BUFFER_UCHAR(PUCHAR Register, PUCHAR Buffer, ULONG Count)
{
    FIXME("stub\n");
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
 *           IoWMIOpenBlock   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoWMIOpenBlock(LPCGUID guid, ULONG desired_access, PVOID *data_block_obj)
{
    FIXME("(%p %u %p) stub\n", guid, desired_access, data_block_obj);
    return STATUS_NOT_IMPLEMENTED;
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
        NtBuildNumber = NtCurrentTeb()->Peb->OSBuildNumber;
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
 *           IoUnregisterPlugPlayNotification    (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoUnregisterPlugPlayNotification(PVOID notification)
{
    FIXME("stub: %p\n", notification);
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
 *           KeEnterCriticalRegion  (NTOSKRNL.EXE.@)
 */
void WINAPI KeEnterCriticalRegion(void)
{
    TRACE( "semi-stub\n" );
    KeGetCurrentThread()->critical_region++;
}

/***********************************************************************
 *           KeLeaveCriticalRegion  (NTOSKRNL.EXE.@)
 */
void WINAPI KeLeaveCriticalRegion(void)
{
    TRACE( "semi-stub\n" );
    KeGetCurrentThread()->critical_region--;
}

/***********************************************************************
 *           KeAreApcsDisabled    (NTOSKRNL.@)
 */
BOOLEAN WINAPI KeAreApcsDisabled(void)
{
    unsigned int critical_region = KeGetCurrentThread()->critical_region;
    TRACE( "%u\n", critical_region );
    return !!critical_region;
}

/***********************************************************************
 *           KeBugCheck    (NTOSKRNL.@)
 */
void WINAPI KeBugCheck(ULONG code)
{
    KeBugCheckEx(code, 0, 0, 0, 0);
}

/***********************************************************************
 *           KeBugCheckEx    (NTOSKRNL.@)
 */
void WINAPI KeBugCheckEx(ULONG code, ULONG_PTR param1, ULONG_PTR param2, ULONG_PTR param3, ULONG_PTR param4)
{
    ERR( "%x %lx %lx %lx %lx\n", code, param1, param2, param3, param4 );
    ExitProcess( code );
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

    if (wcsncmp( name, servicesW, lstrlenW(servicesW) ))
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

    *service = OpenServiceW( manager_handle, name + lstrlenW(servicesW),
                             SERVICE_QUERY_CONFIG | SERVICE_SET_STATUS );
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

/* find the LDR_MODULE corresponding to the driver module */
static LDR_MODULE *find_ldr_module( HMODULE module )
{
    LDR_MODULE *ldr;
    ULONG_PTR magic;

    LdrLockLoaderLock( 0, NULL, &magic );
    if (LdrFindEntryForAddress( module, &ldr ))
    {
        WARN( "module not found for %p\n", module );
        ldr = NULL;
    }
    LdrUnlockLoaderLock( 0, magic );

    return ldr;
}

/* convert PE image VirtualAddress to Real Address */
static inline void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

/* Copied from ntdll with checks for page alignment and characteristics removed */
static NTSTATUS perform_relocations( void *module, SIZE_T len )
{
    IMAGE_NT_HEADERS *nt;
    char *base;
    IMAGE_BASE_RELOCATION *rel, *end;
    const IMAGE_DATA_DIRECTORY *relocs;
    const IMAGE_SECTION_HEADER *sec;
    INT_PTR delta;
    ULONG protect_old[96], i;

    nt = RtlImageNtHeader( module );
    base = (char *)nt->OptionalHeader.ImageBase;

    assert( module != base );

    relocs = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (nt->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        WARN( "Need to relocate module from %p to %p, but there are no relocation records\n",
              base, module );
        return STATUS_CONFLICTING_ADDRESSES;
    }

    if (!relocs->Size) return STATUS_SUCCESS;
    if (!relocs->VirtualAddress) return STATUS_CONFLICTING_ADDRESSES;

    if (nt->FileHeader.NumberOfSections > ARRAY_SIZE( protect_old ))
        return STATUS_INVALID_IMAGE_FORMAT;

    sec = (const IMAGE_SECTION_HEADER *)((const char *)&nt->OptionalHeader +
                                         nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr,
                                &size, PAGE_READWRITE, &protect_old[i] );
    }

    TRACE( "relocating from %p-%p to %p-%p\n",
           base, base + len, module, (char *)module + len );

    rel = get_rva( module, relocs->VirtualAddress );
    end = get_rva( module, relocs->VirtualAddress + relocs->Size );
    delta = (char *)module - base;

    while (rel < end - 1 && rel->SizeOfBlock)
    {
        if (rel->VirtualAddress >= len)
        {
            WARN( "invalid address %p in relocation %p\n", get_rva( module, rel->VirtualAddress ), rel );
            return STATUS_ACCESS_VIOLATION;
        }
        rel = LdrProcessRelocationBlock( get_rva( module, rel->VirtualAddress ),
                                         (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT),
                                         (USHORT *)(rel + 1), delta );
        if (!rel) return STATUS_INVALID_IMAGE_FORMAT;
    }

    for (i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        void *addr = get_rva( module, sec[i].VirtualAddress );
        SIZE_T size = sec[i].SizeOfRawData;
        NtProtectVirtualMemory( NtCurrentProcess(), &addr,
                                &size, protect_old[i], &protect_old[i] );
    }

    return STATUS_SUCCESS;
}

/* load the driver module file */
static HMODULE load_driver_module( const WCHAR *name )
{
    IMAGE_NT_HEADERS *nt;
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    SYSTEM_BASIC_INFORMATION info;
    int i;
    INT_PTR delta;
    ULONG size;
    DWORD old;
    NTSTATUS status;
    HMODULE module = LoadLibraryW( name );

    if (!module) return NULL;
    nt = RtlImageNtHeader( module );

    if (!(delta = (char *)module - (char *)nt->OptionalHeader.ImageBase)) return module;

    /* the loader does not apply relocations to non page-aligned binaries or executables,
     * we have to do it ourselves */

    NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL );
    if (nt->OptionalHeader.SectionAlignment < info.PageSize ||
        !(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
    {
        status = perform_relocations(module, nt->OptionalHeader.SizeOfImage);
        if (status != STATUS_SUCCESS)
            goto error;

        /* make sure we don't try again */
        size = FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) + nt->FileHeader.SizeOfOptionalHeader;
        VirtualProtect( nt, size, PAGE_READWRITE, &old );
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;
        VirtualProtect( nt, size, old, &old );
    }

    /* make sure imports are relocated too */

    if ((imports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for (i = 0; imports[i].Name && imports[i].FirstThunk; i++)
        {
            char *name = (char *)module + imports[i].Name;
            WCHAR buffer[32], *p = buffer;

            while (p < buffer + 32) if (!(*p++ = *name++)) break;
            if (p <= buffer + 32) FreeLibrary( load_driver_module( buffer ) );
        }
    }

    return module;

error:
    FreeLibrary( module );
    return NULL;
}

/* load the .sys module for a device driver */
static HMODULE load_driver( const WCHAR *driver_name, const UNICODE_STRING *keyname )
{
    static const WCHAR driversW[] = {'\\','d','r','i','v','e','r','s','\\',0};
    static const WCHAR systemrootW[] = {'\\','S','y','s','t','e','m','R','o','o','t','\\',0};
    static const WCHAR postfixW[] = {'.','s','y','s',0};
    static const WCHAR ntprefixW[] = {'\\','?','?','\\',0};
    static const WCHAR ImagePathW[] = {'I','m','a','g','e','P','a','t','h',0};
    HKEY driver_hkey;
    HMODULE module;
    LPWSTR path = NULL, str;
    DWORD type, size;

    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, keyname->Buffer + 18 /* skip \registry\machine */, &driver_hkey ))
    {
        ERR( "cannot open key %s, err=%u\n", wine_dbgstr_w(keyname->Buffer), GetLastError() );
        return NULL;
    }

    /* read the executable path from memory */
    size = 0;
    if (!RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, NULL, &size ))
    {
        str = HeapAlloc( GetProcessHeap(), 0, size );
        if (!RegQueryValueExW( driver_hkey, ImagePathW, NULL, &type, (LPBYTE)str, &size ))
        {
            size = ExpandEnvironmentStringsW(str,NULL,0);
            path = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
            ExpandEnvironmentStringsW(str,path,size);
        }
        HeapFree( GetProcessHeap(), 0, str );
        if (!path)
        {
            RegCloseKey( driver_hkey );
            return NULL;
        }

        if (!wcsnicmp( path, systemrootW, 12 ))
        {
            WCHAR buffer[MAX_PATH];

            GetWindowsDirectoryW(buffer, MAX_PATH);

            str = HeapAlloc(GetProcessHeap(), 0, (size -11 + lstrlenW(buffer))
                                                        * sizeof(WCHAR));
            lstrcpyW(str, buffer);
            lstrcatW(str, path + 11);
            HeapFree( GetProcessHeap(), 0, path );
            path = str;
        }
        else if (!wcsncmp( path, ntprefixW, 4 ))
            str = path + 4;
        else
            str = path;
    }
    else
    {
        /* default is to use the driver name + ".sys" */
        WCHAR buffer[MAX_PATH];
        GetSystemDirectoryW(buffer, MAX_PATH);
        path = HeapAlloc(GetProcessHeap(),0,
          (lstrlenW(buffer) + lstrlenW(driversW) + lstrlenW(driver_name) + lstrlenW(postfixW) + 1)
          *sizeof(WCHAR));
        lstrcpyW(path, buffer);
        lstrcatW(path, driversW);
        lstrcatW(path, driver_name);
        lstrcatW(path, postfixW);
        str = path;
    }
    RegCloseKey( driver_hkey );

    TRACE( "loading driver %s\n", wine_dbgstr_w(str) );

    module = load_driver_module( str );
    HeapFree( GetProcessHeap(), 0, path );
    return module;
}

/* call the driver init entry point */
static NTSTATUS WINAPI init_driver( DRIVER_OBJECT *driver_object, UNICODE_STRING *keyname )
{
    unsigned int i;
    NTSTATUS status;
    const IMAGE_NT_HEADERS *nt;
    const WCHAR *driver_name;
    HMODULE module;

    /* Retrieve driver name from the keyname */
    driver_name = wcsrchr( keyname->Buffer, '\\' );
    driver_name++;

    module = load_driver( driver_name, keyname );
    if (!module)
        return STATUS_DLL_INIT_FAILED;

    driver_object->DriverSection = find_ldr_module( module );

    nt = RtlImageNtHeader( module );
    if (!nt->OptionalHeader.AddressOfEntryPoint) return STATUS_SUCCESS;
    driver_object->DriverInit = (PDRIVER_INITIALIZE)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);

    TRACE_(relay)( "\1Call driver init %p (obj=%p,str=%s)\n",
                   driver_object->DriverInit, driver_object, wine_dbgstr_w(keyname->Buffer) );

    status = driver_object->DriverInit( driver_object, keyname );

    TRACE_(relay)( "\1Ret  driver init %p (obj=%p,str=%s) retval=%08x\n",
                   driver_object->DriverInit, driver_object, wine_dbgstr_w(keyname->Buffer), status );

    TRACE( "init done for %s obj %p\n", wine_dbgstr_w(driver_name), driver_object );
    TRACE( "- DriverInit = %p\n", driver_object->DriverInit );
    TRACE( "- DriverStartIo = %p\n", driver_object->DriverStartIo );
    TRACE( "- DriverUnload = %p\n", driver_object->DriverUnload );
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        TRACE( "- MajorFunction[%d] = %p\n", i, driver_object->MajorFunction[i] );

    return status;
}

static BOOLEAN get_drv_name( UNICODE_STRING *drv_name, const UNICODE_STRING *service_name )
{
    static const WCHAR driverW[] = {'\\','D','r','i','v','e','r','\\',0};
    WCHAR *str;

    if (!(str = heap_alloc( sizeof(driverW) + service_name->Length - lstrlenW(servicesW)*sizeof(WCHAR) )))
        return FALSE;

    lstrcpyW( str, driverW );
    lstrcpynW( str + lstrlenW(driverW), service_name->Buffer + lstrlenW(servicesW),
            service_name->Length/sizeof(WCHAR) - lstrlenW(servicesW) + 1 );
    RtlInitUnicodeString( drv_name, str );
    return TRUE;
}

/***********************************************************************
 *           ZwLoadDriver (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ZwLoadDriver( const UNICODE_STRING *service_name )
{
    SERVICE_STATUS_HANDLE service_handle;
    struct wine_rb_entry *entry;
    struct wine_driver *driver;
    UNICODE_STRING drv_name;
    NTSTATUS status;

    TRACE( "(%s)\n", debugstr_us(service_name) );

    if ((status = open_driver( service_name, (SC_HANDLE *)&service_handle )) != STATUS_SUCCESS)
        return status;

    if (!get_drv_name( &drv_name, service_name ))
    {
        CloseServiceHandle( (void *)service_handle );
        return STATUS_NO_MEMORY;
    }

    if (wine_rb_get( &wine_drivers, &drv_name ))
    {
        TRACE( "driver %s already loaded\n", debugstr_us(&drv_name) );
        RtlFreeUnicodeString( &drv_name );
        CloseServiceHandle( (void *)service_handle );
        return STATUS_IMAGE_ALREADY_LOADED;
    }

    set_service_status( service_handle, SERVICE_START_PENDING, 0 );

    status = IoCreateDriver( &drv_name, init_driver );
    entry = wine_rb_get( &wine_drivers, &drv_name );
    RtlFreeUnicodeString( &drv_name );
    if (status != STATUS_SUCCESS)
    {
        ERR( "failed to create driver %s: %08x\n", debugstr_us(service_name), status );
        goto error;
    }

    driver = WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );
    driver->service_handle = service_handle;

    pnp_manager_enumerate_root_devices( service_name->Buffer + wcslen( servicesW ) );

    set_service_status( service_handle, SERVICE_RUNNING,
                        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN );
    return STATUS_SUCCESS;

error:
    set_service_status( service_handle, SERVICE_STOPPED, 0 );
    CloseServiceHandle( (void *)service_handle );
    return status;
}

/***********************************************************************
 *           ZwUnloadDriver (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ZwUnloadDriver( const UNICODE_STRING *service_name )
{
    struct wine_rb_entry *entry;
    UNICODE_STRING drv_name;

    TRACE( "(%s)\n", debugstr_us(service_name) );

    if (!get_drv_name( &drv_name, service_name ))
        return STATUS_NO_MEMORY;

    entry = wine_rb_get( &wine_drivers, &drv_name );
    RtlFreeUnicodeString( &drv_name );
    if (!entry)
    {
        ERR( "failed to locate driver %s\n", debugstr_us(service_name) );
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    unload_driver( entry, NULL );

    return STATUS_SUCCESS;
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
 *           IoCreateNotificationEvent (NTOSKRNL.EXE.@)
 */
PKEVENT WINAPI IoCreateNotificationEvent(UNICODE_STRING *name, HANDLE *handle)
{
    FIXME( "stub: %s %p\n", debugstr_us(name), handle );
    return NULL;
}


#ifdef __x86_64__
/**************************************************************************
 *		__chkstk (NTOSKRNL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );

#elif defined(__i386__)
/**************************************************************************
 *           _chkstk   (NTOSKRNL.@)
 */
__ASM_STDCALL_FUNC( _chkstk, 0,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )
#elif defined(__arm__)
/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Incoming r4 contains words to allocate, converting to bytes then return
 */
__ASM_GLOBAL_FUNC( __chkstk, "lsl r4, r4, #2\n\t"
                             "bx lr" )
#endif

/*********************************************************************
 *           PsAcquireProcessExitSynchronization    (NTOSKRNL.@)
*/
NTSTATUS WINAPI PsAcquireProcessExitSynchronization(PEPROCESS process)
{
    FIXME("stub: %p\n", process);

    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *           PsReleaseProcessExitSynchronization    (NTOSKRNL.@)
 */
void WINAPI PsReleaseProcessExitSynchronization(PEPROCESS process)
{
    FIXME("stub: %p\n", process);
}

typedef struct _EX_PUSH_LOCK_WAIT_BLOCK *PEX_PUSH_LOCK_WAIT_BLOCK;
/*********************************************************************
 *           ExfUnblockPushLock    (NTOSKRNL.@)
 */
DEFINE_FASTCALL_WRAPPER( ExfUnblockPushLock, 8 )
void FASTCALL ExfUnblockPushLock( EX_PUSH_LOCK *lock, PEX_PUSH_LOCK_WAIT_BLOCK block )
{
    FIXME( "stub: %p, %p\n", lock, block );
}

/*********************************************************************
 *           FsRtlRegisterFileSystemFilterCallbacks    (NTOSKRNL.@)
 */
NTSTATUS WINAPI FsRtlRegisterFileSystemFilterCallbacks( DRIVER_OBJECT *object, PFS_FILTER_CALLBACKS callbacks)
{
    FIXME("stub: %p %p\n", object, callbacks);
    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *           SeSinglePrivilegeCheck    (NTOSKRNL.@)
 */
BOOLEAN WINAPI SeSinglePrivilegeCheck(LUID privilege, KPROCESSOR_MODE mode)
{
    FIXME("stub: %08x%08x %u\n", privilege.HighPart, privilege.LowPart, mode);
    return TRUE;
}

/*********************************************************************
 *           SePrivilegeCheck    (NTOSKRNL.@)
 */
BOOLEAN WINAPI SePrivilegeCheck(PRIVILEGE_SET *privileges, SECURITY_SUBJECT_CONTEXT *context, KPROCESSOR_MODE mode)
{
    FIXME("stub: %p %p %u\n", privileges, context, mode);
    return TRUE;
}

/*********************************************************************
 *           SeLocateProcessImageName    (NTOSKRNL.@)
 */
NTSTATUS WINAPI SeLocateProcessImageName(PEPROCESS process, UNICODE_STRING **image_name)
{
    FIXME("stub: %p %p\n", process, image_name);
    if (image_name) *image_name = NULL;
    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *           KeFlushQueuedDpcs    (NTOSKRNL.@)
 */
void WINAPI KeFlushQueuedDpcs(void)
{
    FIXME("stub!\n");
}

/*********************************************************************
 *           IoReleaseRemoveLockAndWaitEx    (NTOSKRNL.@)
 */
void WINAPI IoReleaseRemoveLockAndWaitEx(PIO_REMOVE_LOCK lock, PVOID tag, ULONG size)
{
    FIXME("stub: %p %p %u\n", lock, tag, size);
}

/*********************************************************************
 *           DbgQueryDebugFilterState    (NTOSKRNL.@)
 */
NTSTATUS WINAPI DbgQueryDebugFilterState(ULONG component, ULONG level)
{
    FIXME("stub: %d %d\n", component, level);
    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *           PsGetProcessWow64Process    (NTOSKRNL.@)
 */
PVOID WINAPI PsGetProcessWow64Process(PEPROCESS process)
{
    FIXME("stub: %p\n", process);
    return NULL;
}

/*********************************************************************
 *           MmCopyVirtualMemory    (NTOSKRNL.@)
 */
NTSTATUS WINAPI MmCopyVirtualMemory(PEPROCESS fromprocess, PVOID fromaddress, PEPROCESS toprocess,
                                    PVOID toaddress, SIZE_T bufsize, KPROCESSOR_MODE mode,
                                    PSIZE_T copied)
{
    FIXME("stub: %p %p %p %p %lu %d %p\n", fromprocess, fromaddress, toprocess, toaddress, bufsize, mode, copied);
    return STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 *           KeEnterGuardedRegion    (NTOSKRNL.@)
 */
void WINAPI KeEnterGuardedRegion(void)
{
    FIXME("\n");
}

/*********************************************************************
 *           KeLeaveGuardedRegion    (NTOSKRNL.@)
 */
void WINAPI KeLeaveGuardedRegion(void)
{
    FIXME("\n");
}

static const WCHAR token_type_name[] = {'T','o','k','e','n',0};

static struct _OBJECT_TYPE token_type =
{
    token_type_name
};

POBJECT_TYPE SeTokenObjectType = &token_type;

/*************************************************************************
 *           ExUuidCreate            (NTOSKRNL.@)
 *
 * Creates a 128bit UUID.
 *
 * RETURNS
 *
 *  STATUS_SUCCESS if successful.
 *  RPC_NT_UUID_LOCAL_ONLY if UUID is only locally unique.
 *
 * NOTES
 *
 *  Follows RFC 4122, section 4.4 (Algorithms for Creating a UUID from
 *  Truly Random or Pseudo-Random Numbers)
 */
NTSTATUS WINAPI ExUuidCreate(UUID *uuid)
{
    RtlGenRandom(uuid, sizeof(*uuid));
    /* Clear the version bits and set the version (4) */
    uuid->Data3 &= 0x0fff;
    uuid->Data3 |= (4 << 12);
    /* Set the topmost bits of Data4 (clock_seq_hi_and_reserved) as
     * specified in RFC 4122, section 4.4.
     */
    uuid->Data4[0] &= 0x3f;
    uuid->Data4[0] |= 0x80;

    TRACE("%s\n", debugstr_guid(uuid));

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           ExSetTimerResolution   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI ExSetTimerResolution(ULONG time, BOOLEAN set_resolution)
{
    FIXME("stub: %u %d\n", time, set_resolution);
    return KeQueryTimeIncrement();
}
