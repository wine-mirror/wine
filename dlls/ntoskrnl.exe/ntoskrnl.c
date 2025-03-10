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

#include <assert.h>

#include "ntoskrnl_private.h"
#include "excpt.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "evntprov.h"
#include "ddk/csq.h"
#include "wine/server.h"
#include "wine/heap.h"
#include "wine/svcctl.h"

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

static TP_POOL *dpc_call_tp;
static TP_CALLBACK_ENVIRON dpc_call_tpe;
DECLARE_CRITICAL_SECTION(dpc_call_cs);
static DWORD dpc_call_tls_index;

/* tid of the thread running client request */
static DWORD request_thread;

/* tid of the client thread */
static DWORD client_tid;

static HANDLE ntoskrnl_heap;

static void *ldr_notify_cookie;

static PLOAD_IMAGE_NOTIFY_ROUTINE load_image_notify_routines[8];
static unsigned int load_image_notify_routine_count;

struct irp_data
{
    HANDLE handle;
    IRP *irp;
    BOOL async;
    BOOL complete;
};

static int wine_drivers_rb_compare( const void *key, const struct wine_rb_entry *entry )
{
    const struct wine_driver *driver = WINE_RB_ENTRY_VALUE( entry, const struct wine_driver, entry );
    const UNICODE_STRING *k = key;

    return RtlCompareUnicodeString( k, &driver->driver_obj.DriverName, TRUE );
}

static struct wine_rb_tree wine_drivers = { wine_drivers_rb_compare };

DECLARE_CRITICAL_SECTION(drivers_cs);

struct wine_driver *get_driver( const WCHAR *name )
{
    static const WCHAR driverW[] = L"\\Driver\\";
    struct wine_rb_entry *entry;
    UNICODE_STRING drv_name;

    drv_name.Length = (wcslen( driverW ) + wcslen( name )) * sizeof(WCHAR);
    if (!(drv_name.Buffer = malloc( drv_name.Length + sizeof(WCHAR) )))
        return NULL;
    wcscpy( drv_name.Buffer, driverW );
    wcscat( drv_name.Buffer, name );
    entry = wine_rb_get( &wine_drivers, &drv_name );
    free( drv_name.Buffer );

    if (entry) return WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );
    return NULL;
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
        if (status) FIXME( "set_object_reference failed: %#lx\n", status );
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
    TRACE( "(%p) ref=%lu\n", obj, ref );
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
    TRACE( "(%p) ref=%lu\n", obj, ref );
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

    TRACE( "%p %lx %p %d %p %p\n", handle, access, type, mode, ptr, info );

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

    TRACE( "%p %lx %p %lx %p %d %p\n", obj, attr, access_state, access, type, mode, handle );

    if (mode != KernelMode)
    {
        FIXME( "UserMode access not implemented\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (attr & ~OBJ_KERNEL_HANDLE) FIXME( "attr %#lx not supported\n", attr );
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

static void free_dispatch_irp( struct irp_data *irp_data )
{
    IRP *irp = irp_data->irp;

    if (irp->UserBuffer != irp->AssociatedIrp.SystemBuffer)
    {
        HeapFree( GetProcessHeap(), 0, irp->UserBuffer );
        irp->UserBuffer = NULL;
    }

    free( irp_data );
}

static ULONG get_irp_output_size( IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetNextIrpStackLocation( irp );

    if (!irp->UserBuffer || (irp->Flags & IRP_WRITE_OPERATION))
        return 0;

    /* For IRPs not using buffered I/O, the driver is supposed to have direct
     * access to the user's output buffer, either via an MDL (direct I/O) or
     * with the raw user VA (neither). We can't fully support this, but we
     * should at least copy the entire buffer back to the caller. */
    switch (stack->MajorFunction)
    {
        case IRP_MJ_FILE_SYSTEM_CONTROL:
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            if ((stack->Parameters.DeviceIoControl.IoControlCode & 3) != METHOD_BUFFERED)
                return stack->Parameters.DeviceIoControl.OutputBufferLength;
            break;

        case IRP_MJ_READ:
            /* FIXME: Handle non-buffered reads. */
        default:
            break;
    }

    if (NT_ERROR(irp->IoStatus.Status))
        return 0;
    return irp->IoStatus.Information;
}

/* transfer result of IRP back to wineserver */
static NTSTATUS WINAPI dispatch_irp_completion( DEVICE_OBJECT *device, IRP *irp, void *context )
{
    struct irp_data *irp_data = context;
    NTSTATUS status;
    ULONG out_size;

    EnterCriticalSection( &irp_completion_cs );

    irp_data->complete = TRUE;
    if (!irp_data->async)
    {
        /* main loop will report completion via get_next_device_request */
        LeaveCriticalSection( &irp_completion_cs );
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    out_size = get_irp_output_size( irp );

    SERVER_START_REQ( set_irp_result )
    {
        req->handle   = wine_server_obj_handle( irp_data->handle );
        req->status   = irp->IoStatus.Status;
        req->size     = irp->IoStatus.Information;
        if (out_size) wine_server_add_data( req, irp->UserBuffer, out_size );
        status = wine_server_call( req );
    }
    SERVER_END_REQ;

    free_dispatch_irp( irp_data );

    LeaveCriticalSection( &irp_completion_cs );
    return status;
}

struct dispatch_context
{
    union irp_params params;
    HANDLE handle;
    struct irp_data *irp_data;
    ULONG  in_size;
    void  *in_buff;
};

static NTSTATUS dispatch_irp( DEVICE_OBJECT *device, IRP *irp, struct dispatch_context *context )
{
    struct irp_data *irp_data;
    LARGE_INTEGER count;
    NTSTATUS status;

    if (!(irp_data = malloc( sizeof(*irp_data) )))
        return STATUS_NO_MEMORY;
    irp_data->handle = context->handle;
    irp_data->irp = irp;
    irp_data->async = FALSE;
    irp_data->complete = FALSE;

    IoSetCompletionRoutine( irp, dispatch_irp_completion, irp_data, TRUE, TRUE, TRUE );
    context->irp_data = irp_data;
    context->handle = 0;

    KeQueryTickCount( &count );  /* update the global KeTickCount */

    device->CurrentIrp = irp;
    KeEnterCriticalRegion();
    status = IoCallDriver( device, irp );
    KeLeaveCriticalRegion();
    device->CurrentIrp = NULL;

    if (status != STATUS_PENDING && !irp_data->complete)
        ERR( "dispatch routine returned %#lx but didn't complete the IRP\n", status );

    return status;
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
    irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CREATE_OPERATION;
    return dispatch_irp( device, irp, context );
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
    irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    irp->RequestorMode = UserMode;
    irp->AssociatedIrp.SystemBuffer = NULL;
    irp->UserBuffer = NULL;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    irp->Flags |= IRP_CLOSE_OPERATION;
    return dispatch_irp( device, irp, context );
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

    TRACE( "device %p file %p size %lu\n", device, file, out_size );

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
    return dispatch_irp( device, irp, context );
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

    TRACE( "device %p file %p size %lu\n", device, file, context->in_size );

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
    return dispatch_irp( device, irp, context );
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

    return dispatch_irp( device, irp, context );
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
    NTSTATUS status;

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "ioctl %x device %p file %p in_size %lu out_size %lu\n",
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
            out_buff = context->in_buff;
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
    status = dispatch_irp( device, irp, context );

    HeapFree( GetProcessHeap(), 0, to_free );
    return status;
}

/* process a volume information request for a given device */
static NTSTATUS dispatch_volume( struct dispatch_context *context )
{
    IO_STACK_LOCATION *irpsp;
    IRP *irp;
    void *out_buff = NULL;
    DEVICE_OBJECT *device;
    FILE_OBJECT *file = wine_server_get_ptr( context->params.volume.file );
    ULONG out_size = context->params.volume.out_size;

    if (!file) return STATUS_INVALID_HANDLE;

    device = IoGetAttachedDevice( file->DeviceObject );

    TRACE( "class 0x%x device %p file %p in_size %lu out_size %lu\n",
           context->params.volume.info_class, device, file, context->in_size, out_size );

    if (!(out_buff = HeapAlloc( GetProcessHeap(), 0, out_size ))) return STATUS_NO_MEMORY;

    irp = IoAllocateIrp( device->StackSize, FALSE );
    if (!irp)
    {
        HeapFree( GetProcessHeap(), 0, out_buff );
        return STATUS_NO_MEMORY;
    }

    irpsp = IoGetNextIrpStackLocation( irp );
    irpsp->MajorFunction = IRP_MJ_QUERY_VOLUME_INFORMATION;
    irpsp->Parameters.QueryVolume.FsInformationClass = context->params.volume.info_class;
    irpsp->Parameters.QueryVolume.Length = out_size;
    irpsp->DeviceObject = NULL;
    irpsp->CompletionRoutine = NULL;
    irpsp->FileObject = file;
    irp->AssociatedIrp.SystemBuffer = out_buff;
    irp->RequestorMode = KernelMode;
    irp->UserBuffer = out_buff;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;
    irp->Tail.Overlay.Thread = (PETHREAD)KeGetCurrentThread();
    irp->Tail.Overlay.OriginalFileObject = file;
    irp->RequestorMode = UserMode;

    irp->Flags |= IRP_DEALLOCATE_BUFFER;  /* deallocate out_buff */
    return dispatch_irp( device, irp, context );
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
    dispatch_volume,   /* IRP_CALL_VOLUME */
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
    LDR_DATA_TABLE_ENTRY *ldr;

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

    FreeLibrary( ldr->DllBase );
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
    struct dispatch_context context = {.in_size = 4096};
    NTSTATUS status = STATUS_SUCCESS;
    struct wine_driver *driver;
    HANDLE handles[2];

    /* Set the system process global before setting up the request thread trickery  */
    PsInitialSystemProcess = IoGetCurrentProcess();
    request_thread = GetCurrentThreadId();

    pnp_manager_start();

    handles[0] = stop_event;
    handles[1] = manager;

    for (;;)
    {
        NtCurrentTeb()->Instrumentation[1] = NULL;
        if (!context.in_buff && !(context.in_buff = HeapAlloc( GetProcessHeap(), 0, context.in_size )))
        {
            ERR( "failed to allocate buffer\n" );
            status = STATUS_NO_MEMORY;
            goto done;
        }

        EnterCriticalSection( &irp_completion_cs );

        SERVER_START_REQ( get_next_device_request )
        {
            req->manager  = wine_server_obj_handle( manager );
            req->prev     = wine_server_obj_handle( context.handle );

            if (context.irp_data)
            {
                IRP *irp = context.irp_data->irp;

                req->user_ptr = wine_server_client_ptr( irp );
                req->status   = status;

                if (context.irp_data->complete)
                {
                    /* IRP completed even before we got here; we can report completion now */
                    unsigned int out_size = get_irp_output_size( irp );

                    req->prev        = wine_server_obj_handle( context.irp_data->handle );
                    req->pending     = irp->PendingReturned;
                    req->iosb_status = irp->IoStatus.Status;
                    req->result      = irp->IoStatus.Information;
                    if (out_size) wine_server_add_data( req, irp->UserBuffer, out_size );
                }
                else
                {
                    req->pending     = 1;
                }
            }
            else
            {
                req->user_ptr = 0;
                req->status   = status;
            }

            wine_server_set_reply( req, context.in_buff, context.in_size );
            if (!(status = wine_server_call( req )))
            {
                context.handle  = wine_server_ptr_handle( reply->next );
                context.params  = reply->params;
                context.in_size = reply->in_size;
                client_tid = reply->client_tid;
                NtCurrentTeb()->Instrumentation[1] = wine_server_get_ptr( reply->client_thread );
            }
            else
            {
                context.handle = 0; /* no previous irp */
                if (status == STATUS_BUFFER_OVERFLOW)
                    context.in_size = reply->in_size;
            }
        }
        SERVER_END_REQ;

        if (context.irp_data)
        {
            if (context.irp_data->complete)
            {
                IRP *irp = context.irp_data->irp;
                free_dispatch_irp( context.irp_data );
                IoCompleteRequest( irp, IO_NO_INCREMENT );
            }
            else
            {
                context.irp_data->async = TRUE;
            }
        }

        LeaveCriticalSection( &irp_completion_cs );

        context.irp_data = NULL;

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
     * their unload routine is called. Moreover, we cannot unload a module
     * until we have removed devices for all lower drivers, so we have to stop
     * all devices first, and then unload all drivers. */
    WINE_RB_FOR_EACH_ENTRY( driver, &wine_drivers, struct wine_driver, entry )
        pnp_manager_stop_driver( driver );
    wine_rb_destroy( &wine_drivers, unload_driver, NULL );
    pnp_manager_stop();
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
    FIXME( "stub: %p, %p, %lu, %p\n", DriverObject, ClientIdentificationAddress,
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
    irp->Tail.Overlay.CurrentStackLocation =
            (PIO_STACK_LOCATION)(irp + 1) + stack_size;
}

void WINAPI IoReuseIrp(IRP *irp, NTSTATUS iostatus)
{
    UCHAR AllocationFlags;

    TRACE("irp %p, iostatus %#lx.\n", irp, iostatus);

    AllocationFlags = irp->AllocationFlags;
    IoInitializeIrp(irp, irp->Size, irp->StackCount);
    irp->AllocationFlags = AllocationFlags;
    irp->IoStatus.Status = iostatus;
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

    TRACE("(%p, %lu, %i, %i, %p)\n", va, length, secondary, charge_quota, irp);

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

    TRACE( "%lx, %p, %p, %lu, %p, %lu, %u, %p, %p\n",
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

    TRACE( "(%ld %p %p %ld %p %p)\n", majorfunc, device, buffer, length, startoffset, iosb );

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

    TRACE("(%ld %p %p %ld %p %p)\n", majorfunc, device, buffer, length, startoffset, iosb);

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
    irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
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
    list_init( &driver->root_pnp_devices );

    EnterCriticalSection( &drivers_cs );
    if (wine_rb_put( &wine_drivers, &driver->driver_obj.DriverName, &driver->entry ))
        ERR( "failed to insert driver %s in tree\n", debugstr_us(name) );
    LeaveCriticalSection( &drivers_cs );

    status = driver->driver_obj.DriverInit( &driver->driver_obj, &driver->driver_extension.ServiceKeyName );
    if (status)
    {
        IoDeleteDriver( &driver->driver_obj );
        return status;
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        if (driver->driver_obj.MajorFunction[i]) continue;
        driver->driver_obj.MajorFunction[i] = unhandled_irp;
    }

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
 *           IoCreateDeviceSecure   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateDeviceSecure( DRIVER_OBJECT *driver, ULONG ext_size,
                                      UNICODE_STRING *name, DEVICE_TYPE type,
                                      ULONG characteristics, BOOLEAN exclusive,
                                      PCUNICODE_STRING sddl, LPCGUID guid,
                                      DEVICE_OBJECT **ret_device )
{
    FIXME( "(%p, %lu, %s, %lu, %lx, %u, %s, %s, %p): semi-stub\n",
           driver, ext_size, debugstr_us(name), type, characteristics, exclusive,
           debugstr_us(sddl), wine_dbgstr_guid(guid), ret_device );

    return IoCreateDevice( driver, ext_size, name, type, characteristics, exclusive, ret_device );
}

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
    struct wine_device *wine_device;
    DEVICE_OBJECT *device;
    HANDLE manager = get_device_manager();
    static unsigned int auto_idx = 0;
    WCHAR autoW[17];

    TRACE( "(%p, %lu, %s, %lu, %lx, %u, %p)\n",
           driver, ext_size, debugstr_us(name), type, characteristics, exclusive, ret_device );

    if (!(wine_device = alloc_kernel_object( IoDeviceObjectType, NULL, sizeof(struct wine_device) + ext_size, 1 )))
        return STATUS_NO_MEMORY;
    device = &wine_device->device_obj;

    device->DriverObject    = driver;
    device->DeviceExtension = wine_device + 1;
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
        struct wine_device *wine_device = CONTAINING_RECORD(device, struct wine_device, device_obj);
        DEVICE_OBJECT **prev = &device->DriverObject->DeviceObject;
        DEVICE_RELATIONS *children;
        unsigned int i;

        while (*prev && *prev != device) prev = &(*prev)->NextDevice;
        if (*prev) *prev = (*prev)->NextDevice;
        if ((children = wine_device->children))
        {
            for (i = 0; i < children->Count; ++i)
                ObDereferenceObject( children->Objects[i] );
            ExFreePool( children );
        }
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
    NTSTATUS ret;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    TRACE( "%s -> %s\n", debugstr_us(name), debugstr_us(target) );
    if (!(ret = NtCreateSymbolicLinkObject( &handle, SYMBOLIC_LINK_ALL_ACCESS, &attr, target )))
        NtClose( handle );
    return ret;
}


/***********************************************************************
 *           IoCreateUnprotectedSymbolicLink   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateUnprotectedSymbolicLink( UNICODE_STRING *name, UNICODE_STRING *target )
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS ret;

    attr.Length                   = sizeof(attr);
    attr.RootDirectory            = 0;
    attr.ObjectName               = name;
    attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT;
    attr.SecurityDescriptor       = NULL;
    attr.SecurityQualityOfService = NULL;

    TRACE( "%s -> %s\n", debugstr_us(name), debugstr_us(target) );
    if (!(ret = NtCreateSymbolicLinkObject( &handle, SYMBOLIC_LINK_ALL_ACCESS, &attr, target )))
        NtClose( handle );
    return ret;
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

    if (!(status = NtOpenSymbolicLinkObject( &handle, DELETE, &attr )))
    {
        NtMakeTemporaryObject( handle );
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
    FIXME( "stub: %s %p %lx %p\n", debugstr_guid(InterfaceClassGuid),
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

    FIXME( "stub: %s %lx %p %p\n", debugstr_us(name), access, file, device );

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
    irpsp = --irp->Tail.Overlay.CurrentStackLocation;
    irpsp->DeviceObject = device;
    dispatch = device->DriverObject->MajorFunction[irpsp->MajorFunction];

    TRACE_(relay)( "\1Call driver dispatch %p (device=%p,irp=%p)\n", dispatch, device, irp );

    status = dispatch( device, irp );

    TRACE_(relay)( "\1Ret  driver dispatch %p (device=%p,irp=%p) retval=%08lx\n",
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
 *           IoRegisterBootDriverReinitialization   (NTOSKRNL.EXE.@)
 */
void WINAPI IoRegisterBootDriverReinitialization(DRIVER_OBJECT *driver, PDRIVER_REINITIALIZE proc, void *ctx)
{
    FIXME("driver %p, proc %p, ctx %p, stub!\n", driver, proc, ctx);
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
    FIXME( "(%p, %p, %lu, %p, %p, %lu, %p): stub\n", drv_obj, drv_list, drv_size,
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
    FIXME( "(%s, %p, %p, %lu, %p, %p, %lu, %d, %p): stub\n", debugstr_us(name),
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
    DEVICE_OBJECT *device;
    int call_flag = 0;

    TRACE( "%p %u\n", irp, priority_boost );

    status = irp->IoStatus.Status;
    while (irp->CurrentLocation <= irp->StackCount)
    {
        irpsp = irp->Tail.Overlay.CurrentStackLocation;
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
        ++irp->Tail.Overlay.CurrentStackLocation;
        if (irp->CurrentLocation <= irp->StackCount)
            device = IoGetCurrentIrpStackLocation(irp)->DeviceObject;
        else
            device = NULL;
        irp->PendingReturned = !!(irpsp->Control & SL_PENDING_RETURNED);
        irpsp->Control = 0;
        if (call_flag)
        {
            TRACE( "calling %p( %p, %p, %p )\n", routine, device, irp, irpsp->Context );
            stat = routine( device, irp, irpsp->Context );
            TRACE( "CompletionRoutine returned %lx\n", stat );
            if (STATUS_MORE_PROCESSING_REQUIRED == stat)
                return;
        }
    }

    if (irp->Flags & IRP_DEALLOCATE_BUFFER)
        HeapFree( GetProcessHeap(), 0, irp->AssociatedIrp.SystemBuffer );
    if (irp->UserIosb) *irp->UserIosb = irp->IoStatus;
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
        IoReleaseCancelSpinLock( irql );
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

#ifdef __i386__

/*************************************************************************
 *           RtlUshortByteSwap   (NTOSKRNL.EXE.@)
 */
__ASM_FASTCALL_FUNC(RtlUshortByteSwap, 4,
                    "movb %ch,%al\n\t"
                    "movb %cl,%ah\n\t"
                    "ret")

/*************************************************************************
 *           RtlUlongByteSwap   (NTOSKRNL.EXE.@)
 */
__ASM_FASTCALL_FUNC(RtlUlongByteSwap, 4,
                    "movl %ecx,%eax\n\t"
                    "bswap %eax\n\t"
                    "ret")

/*************************************************************************
 *           RtlUlonglongByteSwap   (NTOSKRNL.EXE.@)
 */
__ASM_FASTCALL_FUNC(RtlUlonglongByteSwap, 8,
                    "movl 4(%esp),%edx\n\t"
                    "bswap %edx\n\t"
                    "movl 8(%esp),%eax\n\t"
                    "bswap %eax\n\t"
                    "ret $8")

#endif  /* __i386__ */

/***********************************************************************
 *           ExAllocatePool2   (NTOSKRNL.EXE.@)
 */
void * WINAPI ExAllocatePool2( POOL_FLAGS flags, SIZE_T size, ULONG tag )
{
    /* FIXME: handle page alignment constraints */
    void *ret = HeapAlloc( ntoskrnl_heap, 0, size );
    TRACE( "(0x%I64x, %Iu, %s) -> %p\n", flags, size, debugstr_fourcc(tag), ret );
    return ret;
}

static POOL_FLAGS pool_type_to_flags( POOL_TYPE type )
{
    switch (type & 7)
    {
    case NonPagedPool:
    case NonPagedPoolMustSucceed:
        return POOL_FLAG_NON_PAGED;
    case PagedPool:
        return POOL_FLAG_PAGED;
    case NonPagedPoolCacheAligned:
    case NonPagedPoolCacheAlignedMustS:
        return POOL_FLAG_NON_PAGED|POOL_FLAG_CACHE_ALIGNED;
    case PagedPoolCacheAligned:
        return POOL_FLAG_PAGED|POOL_FLAG_CACHE_ALIGNED;
    default:
        return 0;
    }
}

/***********************************************************************
 *           ExAllocatePool   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePool( POOL_TYPE type, SIZE_T size )
{
    POOL_FLAGS flags = pool_type_to_flags( type );
    if (type & POOL_RAISE_IF_ALLOCATION_FAILURE)
        flags |= POOL_FLAG_RAISE_ON_FAILURE;

    return ExAllocatePool2( flags, size, 0 );
}

/***********************************************************************
 *           ExAllocatePoolWithQuota   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithQuota( POOL_TYPE type, SIZE_T size )
{
    POOL_FLAGS flags = pool_type_to_flags( type ) | POOL_FLAG_USE_QUOTA;
    if (!(type & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE))
        flags |= POOL_FLAG_RAISE_ON_FAILURE;

    return ExAllocatePool2( flags, size, 0 );
}

/***********************************************************************
 *           ExAllocatePoolWithTag   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithTag( POOL_TYPE type, SIZE_T size, ULONG tag )
{
    POOL_FLAGS flags = pool_type_to_flags( type );
    if (type & POOL_RAISE_IF_ALLOCATION_FAILURE)
        flags |= POOL_FLAG_RAISE_ON_FAILURE;

    return ExAllocatePool2( flags, size, tag );
}

/***********************************************************************
 *           ExAllocatePoolWithQuotaTag   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI ExAllocatePoolWithQuotaTag( POOL_TYPE type, SIZE_T size, ULONG tag )
{
    POOL_FLAGS flags = pool_type_to_flags( type ) | POOL_FLAG_USE_QUOTA;
    if (!(type & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE))
        flags |= POOL_FLAG_RAISE_ON_FAILURE;

    return ExAllocatePool2( flags, size, tag );
}

/***********************************************************************
 *           ExCreateCallback   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ExCreateCallback(PCALLBACK_OBJECT *obj, POBJECT_ATTRIBUTES attr,
                                 BOOLEAN create, BOOLEAN allow_multiple)
{
    FIXME("(%p, %p, %u, %u): stub\n", obj, attr, create, allow_multiple);

    return STATUS_SUCCESS;
}

void * WINAPI ExRegisterCallback(PCALLBACK_OBJECT callback_object,
        PCALLBACK_FUNCTION callback_function, void *callback_context)
{
    FIXME("callback_object %p, callback_function %p, callback_context %p stub.\n",
            callback_object, callback_function, callback_context);

    return (void *)0xdeadbeef;
}

void WINAPI ExUnregisterCallback(void *callback_registration)
{
    FIXME("callback_registration %p stub.\n", callback_registration);
}

/***********************************************************************
 *           ExNotifyCallback   (NTOSKRNL.EXE.@)
 */
void WINAPI ExNotifyCallback(void *obj, void *arg1, void *arg2)
{
    FIXME("(%p, %p, %p): stub\n", obj, arg1, arg2);
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
    HeapFree( ntoskrnl_heap, 0, ptr );
}

static void initialize_lookaside_list( GENERAL_LOOKASIDE *lookaside, PALLOCATE_FUNCTION allocate, PFREE_FUNCTION free,
                                       ULONG type, SIZE_T size, ULONG tag )
{

    RtlInitializeSListHead( &lookaside->ListHead );
    lookaside->Depth                 = 4;
    lookaside->MaximumDepth          = 256;
    lookaside->TotalAllocates        = 0;
    lookaside->AllocateMisses     = 0;
    lookaside->TotalFrees            = 0;
    lookaside->FreeMisses         = 0;
    lookaside->Type                  = type;
    lookaside->Tag                   = tag;
    lookaside->Size                  = size;
    lookaside->Allocate           = allocate ? allocate : ExAllocatePoolWithTag;
    lookaside->Free               = free ? free : ExFreePool;
    lookaside->LastTotalAllocates    = 0;
    lookaside->LastAllocateMisses = 0;

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
    TRACE( "%p, %p, %p, %lu, %Iu, %lu, %u\n", lookaside, allocate, free, flags, size, tag, depth );
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
    TRACE( "%p, %p, %p, %lu, %Iu, %lu, %u\n", lookaside, allocate, free, flags, size, tag, depth );
    initialize_lookaside_list( &lookaside->L, allocate, free, PagedPool | flags, size, tag );
}

static void delete_lookaside_list( GENERAL_LOOKASIDE *lookaside )
{
    void *entry;
    while ((entry = RtlInterlockedPopEntrySList(&lookaside->ListHead)))
        lookaside->FreeEx(entry, (LOOKASIDE_LIST_EX*)lookaside);
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
    FIXME( "stub: %p, %lu, %p, %lu\n", Zone, BlockSize, InitialSegment, InitialSegmentSize );
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
    IsWow64Process( handle, &process->wow64 );
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
    TRACE( "%p -> %Ix\n", process, process->info.UniqueProcessId );
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
    thread->user_affinity = 0;

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
    struct _KTHREAD *thread = NtCurrentTeb()->Instrumentation[1];

    if (!thread)
    {
        HANDLE handle = GetCurrentThread();

        /* FIXME: we shouldn't need it, GetCurrentThread() should be client thread already */
        if (GetCurrentThreadId() == request_thread)
            handle = OpenThread( THREAD_QUERY_INFORMATION, FALSE, client_tid );

        kernel_object_from_handle( handle, PsThreadType, (void**)&thread );
        if (handle != GetCurrentThread()) NtClose( handle );

        NtCurrentTeb()->Instrumentation[1] = thread;
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

/***********************************************************************
 *           KeInsertQueueDpc   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI KeInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2)
{
    FIXME( "stub: (%p %p %p)\n", Dpc, SystemArgument1, SystemArgument2 );
    return TRUE;
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
    DWORD_PTR affinity_mask;

    GetProcessAffinityMask( GetCurrentProcess(), NULL, &affinity_mask);
    return affinity_mask;
}

ULONG WINAPI KeQueryActiveProcessorCountEx(USHORT group_number)
{
    TRACE("group_number %u.\n", group_number);

    return GetActiveProcessorCount(group_number);
}

ULONG WINAPI KeQueryActiveProcessorCount(PKAFFINITY active_processors)
{
    TRACE("active_processors %p.\n", active_processors);

    if(active_processors)
        *active_processors = KeQueryActiveProcessors();

    return KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
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
 *           KeQueryPriorityThread   (NTOSKRNL.EXE.@)
 */
KPRIORITY WINAPI KeQueryPriorityThread( PKTHREAD Thread )
{
    FIXME("(%p): stub.\n", Thread);
    /* priority must be a value between 0 and 31 */
    return 15;
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
    FIXME("(%p %ld)\n", Thread, Priority);
    return Priority;
}

/***********************************************************************
 *           KeSetSystemAffinityThread   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetSystemAffinityThread(KAFFINITY affinity)
{
    KeSetSystemAffinityThreadEx(affinity);
}

KAFFINITY WINAPI KeSetSystemAffinityThreadEx(KAFFINITY affinity)
{
    DWORD_PTR system_affinity = KeQueryActiveProcessors();
    PKTHREAD thread = KeGetCurrentThread();
    GROUP_AFFINITY old, new;

    TRACE("affinity %#Ix.\n", affinity);

    affinity &= system_affinity;

    NtQueryInformationThread(GetCurrentThread(), ThreadGroupInformation,
            &old, sizeof(old), NULL);

    if (old.Mask != system_affinity)
        thread->user_affinity = old.Mask;

    memset(&new, 0, sizeof(new));
    new.Mask = affinity;

    return NtSetInformationThread(GetCurrentThread(), ThreadGroupInformation, &new, sizeof(new))
            ? 0 : thread->user_affinity;
}


/***********************************************************************
 *           KeRevertToUserAffinityThread   (NTOSKRNL.EXE.@)
 */
void WINAPI KeRevertToUserAffinityThread(void)
{
    KeRevertToUserAffinityThreadEx(0);
}

void WINAPI KeRevertToUserAffinityThreadEx(KAFFINITY affinity)
{
    DWORD_PTR system_affinity = KeQueryActiveProcessors();
    PRKTHREAD thread = KeGetCurrentThread();
    GROUP_AFFINITY new;

    TRACE("affinity %#Ix.\n", affinity);

    affinity &= system_affinity;

    memset(&new, 0, sizeof(new));
    new.Mask = affinity ? affinity
            : (thread->user_affinity ? thread->user_affinity : system_affinity);

    NtSetInformationThread(GetCurrentThread(), ThreadGroupInformation, &new, sizeof(new));
    thread->user_affinity = affinity;
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
    WARN("(%p %p %Iu %x %p) semi-stub: ignoring stack expand\n", callout, parameter, size, wait, context);
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
    TRACE( "%Iu\n", size );
    return VirtualAlloc( NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE|PAGE_NOCACHE );
}

/***********************************************************************
 *           MmAllocateContiguousMemory   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmAllocateContiguousMemory( SIZE_T size, PHYSICAL_ADDRESS highest_valid_address )
{
    FIXME( "%Iu, %s stub\n", size, wine_dbgstr_longlong(highest_valid_address.QuadPart) );
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
    FIXME("%s %s %s %Iu: stub\n", wine_dbgstr_longlong(lowaddress.QuadPart), wine_dbgstr_longlong(highaddress.QuadPart),
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
    FIXME("%p %#lx %p %s %#lx %#lx %p %p: stub\n", handle, access, attr,
        wine_dbgstr_longlong(size->QuadPart), protect, alloc_attr, file, file_obj);
    return STATUS_NOT_IMPLEMENTED;
}

/***********************************************************************
 *           MmFreeNonCachedMemory   (NTOSKRNL.EXE.@)
 */
void WINAPI MmFreeNonCachedMemory( void *addr, SIZE_T size )
{
    TRACE( "%p %Iu\n", addr, size );
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
 *           MmGetPhysicalAddress   (NTOSKRNL.EXE.@)
 */
PHYSICAL_ADDRESS WINAPI MmGetPhysicalAddress(void *virtual_address)
{
    PHYSICAL_ADDRESS ret;
    FIXME("(%p): semi-stub\n", virtual_address);
    ret.QuadPart = (ULONG_PTR)virtual_address;
    return ret;
}

/***********************************************************************
 *           MmMapIoSpace   (NTOSKRNL.EXE.@)
 */
PVOID WINAPI MmMapIoSpace( PHYSICAL_ADDRESS PhysicalAddress, DWORD NumberOfBytes, DWORD CacheType )
{
    FIXME( "stub: 0x%08lx%08lx, %ld, %ld\n", PhysicalAddress.HighPart, PhysicalAddress.LowPart, NumberOfBytes, CacheType );
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
    FIXME("(%p, %u, %u, %p, %lu, %u): stub\n", MemoryDescriptorList, AccessMode, CacheType, BaseAddress, BugCheckOnFailure, Priority);

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
    FIXME( "stub: %p, %Iu\n", BaseAddress, NumberOfBytes );
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

    TRACE("mostly-stub:%s %li %p %li %p %i %p %p\n", debugstr_us(ObjectName),
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


/********************************************************************
 *           ObOpenObjectByName   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObOpenObjectByName(POBJECT_ATTRIBUTES attr, POBJECT_TYPE type,
                                   KPROCESSOR_MODE mode, ACCESS_STATE *access_state,
                                   ACCESS_MASK access, PVOID ctx, HANDLE *handle)
{
    NTSTATUS status;
    void *object;

    TRACE( "attr(%p %s %lx) %p %u %p %lu %p %p\n", attr->RootDirectory, debugstr_us(attr->ObjectName),
                                                 attr->Attributes, type, mode, access_state, access, ctx, handle );

    if (mode != KernelMode)
    {
        FIXME( "UserMode access not implemented\n" );
        return STATUS_NOT_IMPLEMENTED;
    }

    if (attr->RootDirectory) FIXME( "RootDirectory unhandled\n" );

    status = ObReferenceObjectByName(attr->ObjectName, attr->Attributes, access_state, access, type, mode, ctx, &object );
    if (status != STATUS_SUCCESS)
        return status;

    status = ObOpenObjectByPointer(object, attr->Attributes, access_state, access, type, mode, handle);

    ObDereferenceObject(object);
    return status;
}


/***********************************************************************
 *           ObReferenceObjectByPointer   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI ObReferenceObjectByPointer(void *obj, ACCESS_MASK access,
                                           POBJECT_TYPE type,
                                           KPROCESSOR_MODE mode)
{
    FIXME("(%p, %lx, %p, %d): stub\n", obj, access, type, mode);

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
NTSTATUS WINAPI ObRegisterCallbacks(POB_CALLBACK_REGISTRATION callback, void **handle)
{
    FIXME( "callback %p, handle %p.\n", callback, handle );

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
 *           PsGetCurrentProcessSessionId   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI PsGetCurrentProcessSessionId(void)
{
    return PsGetCurrentProcess()->info.PebBaseAddress->SessionId;
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
NTSTATUS WINAPI PsRemoveLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE routine)
 {
    unsigned int i;

    TRACE("routine %p.\n", routine);

    for (i = 0; i < load_image_notify_routine_count; ++i)
        if (load_image_notify_routines[i] == routine)
        {
            --load_image_notify_routine_count;
            memmove(&load_image_notify_routines[i], &load_image_notify_routines[i + 1],
                    sizeof(*load_image_notify_routines) * (load_image_notify_routine_count - i));
            return STATUS_SUCCESS;
        }
    return STATUS_PROCEDURE_NOT_FOUND;
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
    TRACE("status %#lx.\n", status);
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
           hMod = LoadLibraryW( halW );
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
 *           MmProtectMdlSystemAddress   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI MmProtectMdlSystemAddress(PMDL MemoryDescriptorList, ULONG NewProtect)
{
    FIXME("(%p, %lu) stub\n", MemoryDescriptorList, NewProtect);
    return STATUS_SUCCESS;
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
void WINAPI KeInitializeDpc(KDPC *dpc, PKDEFERRED_ROUTINE deferred_routine, void *deferred_context)
{
    FIXME("dpc %p, deferred_routine %p, deferred_context %p semi-stub.\n",
            dpc, deferred_routine, deferred_context);

    dpc->DeferredRoutine = deferred_routine;
    dpc->DeferredContext = deferred_context;
}

/***********************************************************************
 *          KeSetImportanceDpc   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetImportanceDpc(PRKDPC dpc, KDPC_IMPORTANCE importance)
{
    FIXME("%p, %d stub\n", dpc, importance);
}

/***********************************************************************
 *          KeSetTargetProcessorDpcEx   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetTargetProcessorDpcEx(PRKDPC dpc, PPROCESSOR_NUMBER process_number)
{
    FIXME("%p, %p stub\n", dpc, process_number);
}

/***********************************************************************
 *          KeSetTargetProcessorDpc   (NTOSKRNL.EXE.@)
 */
VOID WINAPI KeSetTargetProcessorDpc(PRKDPC dpc, CCHAR number)
{
    FIXME("%p, %d stub\n", dpc, number);
}

/***********************************************************************
 *          KeGetCurrentProcessorNumberEx   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI KeGetCurrentProcessorNumberEx(PPROCESSOR_NUMBER process_number)
{
    ULONG cur_number = NtGetCurrentProcessorNumber();

    FIXME("%p semi-stub\n", process_number);

    if (process_number)
    {
        process_number->Group = 0;
        process_number->Reserved = 0;
        process_number->Number = cur_number;
    }

    return cur_number;
}

/***********************************************************************
 *          KeQueryGroupAffinity   (NTOSKRNL.EXE.@)
 */
KAFFINITY WINAPI KeQueryGroupAffinity(USHORT group_number)
{
    FIXME("%u stub\n", group_number);
    return 0;
}

/***********************************************************************
 *          KeQueryMaximumProcessorCountEx   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI KeQueryMaximumProcessorCountEx(USHORT group_number)
{
    return GetMaximumProcessorCount(group_number);
}

/***********************************************************************
 *          KeQueryMaximumProcessorCount   (NTOSKRNL.EXE.@)
 */
ULONG WINAPI KeQueryMaximumProcessorCount(void)
{
    return KeQueryMaximumProcessorCountEx(0);
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
    FIXME("(%p %lu) stub\n", DeviceObject, Action);
    return STATUS_SUCCESS;
}

/*****************************************************
 *           IoWMIOpenBlock   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoWMIOpenBlock(LPCGUID guid, ULONG desired_access, PVOID *data_block_obj)
{
    FIXME("(%p %lu %p) stub\n", guid, desired_access, data_block_obj);
    return STATUS_NOT_IMPLEMENTED;
}

/*****************************************************
 *           PsSetLoadImageNotifyRoutine   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetLoadImageNotifyRoutine(PLOAD_IMAGE_NOTIFY_ROUTINE routine)
{
    return PsSetLoadImageNotifyRoutineEx(routine, 0);
}

/*****************************************************
 *           PsSetLoadImageNotifyRoutineEx   (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI PsSetLoadImageNotifyRoutineEx(PLOAD_IMAGE_NOTIFY_ROUTINE routine, ULONG_PTR flags)
{
    FIXME("routine %p, flags %Ix semi-stub.\n", routine, flags);

    if (load_image_notify_routine_count == ARRAY_SIZE(load_image_notify_routines))
        return STATUS_INSUFFICIENT_RESOURCES;

    load_image_notify_routines[load_image_notify_routine_count++] = routine;

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
 *           Ke386IoSetAccessProcess  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI Ke386IoSetAccessProcess(PEPROCESS *process, ULONG flag)
{
    FIXME("(%p %ld) stub\n", process, flag);
    return FALSE;
}

/*****************************************************
 *           Ke386QueryIoAccessMap  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI Ke386QueryIoAccessMap(ULONG flag, PVOID buffer)
{
    FIXME("(%ld %p) stub\n", flag, buffer);
    return FALSE;
}

/*****************************************************
 *           Ke386SetIoAccessMap  (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI Ke386SetIoAccessMap(ULONG flag, PVOID buffer)
{
    FIXME("(%ld %p) stub\n", flag, buffer);
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
NTSTATUS WINAPI ObQueryNameString( void *object, OBJECT_NAME_INFORMATION *name, ULONG size, ULONG *ret_size )
{
    HANDLE handle;
    NTSTATUS ret;

    TRACE("object %p, name %p, size %lu, ret_size %p.\n", object, name, size, ret_size);

    if ((ret = ObOpenObjectByPointer( object, 0, NULL, 0, NULL, KernelMode, &handle )))
        return ret;
    ret = NtQueryObject( handle, ObjectNameInformation, name, size, ret_size );

    NtClose( handle );
    return ret;
}

/*****************************************************
 *           IoRegisterPlugPlayNotification  (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoRegisterPlugPlayNotification(IO_NOTIFICATION_EVENT_CATEGORY category, ULONG flags, PVOID data,
                                               PDRIVER_OBJECT driver, PDRIVER_NOTIFICATION_CALLBACK_ROUTINE callback,
                                               PVOID context, PVOID *notification)
{
    FIXME("(%u %lu %p %p %p %p %p) stub\n", category, flags, data, driver, callback, context, notification);
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
 *           KeAreAllApcsDisabled    (NTOSKRNL.@)
 */
BOOLEAN WINAPI KeAreAllApcsDisabled(void)
{
    return KeAreApcsDisabled();
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
    ERR( "%lx %Ix %Ix %Ix %Ix\n", code, param1, param2, param3, param4 );
    ExitProcess( code );
}

/***********************************************************************
 *           ProbeForRead   (NTOSKRNL.EXE.@)
 */
void WINAPI ProbeForRead(void *address, SIZE_T length, ULONG alignment)
{
    FIXME("(%p %Iu %lu) stub\n", address, length, alignment);
}

/***********************************************************************
 *           ProbeForWrite   (NTOSKRNL.EXE.@)
 */
void WINAPI ProbeForWrite(void *address, SIZE_T length, ULONG alignment)
{
    FIXME("(%p %Iu %lu) stub\n", address, length, alignment);
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

/* find the LDR_DATA_TABLE_ENTRY corresponding to the driver module */
static LDR_DATA_TABLE_ENTRY *find_ldr_module( HMODULE module )
{
    LDR_DATA_TABLE_ENTRY *ldr;
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

static void WINAPI ldr_notify_callback(ULONG reason, LDR_DLL_NOTIFICATION_DATA *data, void *context)
{
    const IMAGE_DATA_DIRECTORY *relocs;
    IMAGE_BASE_RELOCATION *rel, *end;
    SYSTEM_BASIC_INFORMATION info;
    IMAGE_NT_HEADERS *nt;
    INT_PTR delta;
    char *base;
    HMODULE module;

    if (reason != LDR_DLL_NOTIFICATION_REASON_LOADED) return;
    TRACE( "loading %s\n", debugstr_us(data->Loaded.BaseDllName));

    module = data->Loaded.DllBase;
    nt = RtlImageNtHeader( module );
    base = (char *)nt->OptionalHeader.ImageBase;
    if (!(delta = (char *)module - base)) return;

    /* the loader does not apply relocations to non page-aligned binaries or executables,
     * we have to do it ourselves */

    NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL );
    if (nt->OptionalHeader.SectionAlignment >= info.PageSize && (nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
        return;

    if (nt->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
        WARN( "Need to relocate module from %p to %p, but there are no relocation records\n", base, module );
        return;
    }

    relocs = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (!relocs->Size || !relocs->VirtualAddress) return;

    TRACE( "relocating from %p-%p to %p-%p\n", base, base + nt->OptionalHeader.SizeOfImage,
           module, (char *)module + nt->OptionalHeader.SizeOfImage );

    rel = get_rva( module, relocs->VirtualAddress );
    end = get_rva( module, relocs->VirtualAddress + relocs->Size );

    while (rel < end - 1 && rel->SizeOfBlock)
    {
        char *page = get_rva( module, rel->VirtualAddress );
        DWORD old_prot1, old_prot2;

        if (rel->VirtualAddress >= nt->OptionalHeader.SizeOfImage)
        {
            WARN( "invalid address %p in relocation %p\n", get_rva( module, rel->VirtualAddress ), rel );
            return;
        }

        /* Relocation entries may hang over the end of the page, so we need to
         * protect two pages. */
        VirtualProtect( page, info.PageSize, PAGE_READWRITE, &old_prot1 );
        VirtualProtect( page + info.PageSize, info.PageSize, PAGE_READWRITE, &old_prot2 );
        rel = LdrProcessRelocationBlock( page, (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT),
                                         (USHORT *)(rel + 1), delta );
        VirtualProtect( page, info.PageSize, old_prot1, &old_prot1 );
        VirtualProtect( page + info.PageSize, info.PageSize, old_prot2, &old_prot2 );
        if (!rel)
        {
            WARN( "LdrProcessRelocationBlock failed\n" );
            return;
        }
    }
}

static WCHAR *get_windir_path( const WCHAR *path )
{
    WCHAR buffer[MAX_PATH];
    int len = GetWindowsDirectoryW( buffer, MAX_PATH );
    int len2 = wcslen( path );
    WCHAR *ret = HeapAlloc( GetProcessHeap(), 0, (len + len2 + 2) * sizeof(WCHAR) );
    swprintf( ret, len + len2 + 2, L"%s\\%s", buffer, path );
    return ret;
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
        ERR( "cannot open key %s, err=%lu\n", wine_dbgstr_w(keyname->Buffer), GetLastError() );
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
            str = get_windir_path( path + 12 );
            HeapFree( GetProcessHeap(), 0, path );
            path = str;
        }
        else if (RtlDetermineDosPathNameType_U( path ) == RtlPathTypeRelative)
        {
            str = get_windir_path( path );
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

    module = LoadLibraryExW( str, 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );

    if (module && load_image_notify_routine_count)
    {
        UNICODE_STRING module_name;
        IMAGE_NT_HEADERS *nt;
        IMAGE_INFO info;
        unsigned int i;

        RtlInitUnicodeString(&module_name, str);
        nt = RtlImageNtHeader(module);
        memset(&info, 0, sizeof(info));
        info.ImageAddressingMode = IMAGE_ADDRESSING_MODE_32BIT;
        info.SystemModeImage = TRUE;
        info.ImageSize = nt->OptionalHeader.SizeOfImage;
        info.ImageBase = module;

        for (i = 0; i < load_image_notify_routine_count; ++i)
        {
            TRACE("Calling image load notify %p.\n", load_image_notify_routines[i]);
            load_image_notify_routines[i](&module_name, NULL, &info);
            TRACE("Called image load notify %p.\n", load_image_notify_routines[i]);
        }
    }

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
    driver_object->DriverStart = ((LDR_DATA_TABLE_ENTRY *)driver_object->DriverSection)->DllBase;
    driver_object->DriverSize = ((LDR_DATA_TABLE_ENTRY *)driver_object->DriverSection)->SizeOfImage;

    nt = RtlImageNtHeader( module );
    if (!nt->OptionalHeader.AddressOfEntryPoint) return STATUS_SUCCESS;
    driver_object->DriverInit = (PDRIVER_INITIALIZE)((char *)module + nt->OptionalHeader.AddressOfEntryPoint);

    TRACE_(relay)( "\1Call driver init %p (obj=%p,str=%s)\n",
                   driver_object->DriverInit, driver_object, wine_dbgstr_w(keyname->Buffer) );

    status = driver_object->DriverInit( driver_object, keyname );

    TRACE_(relay)( "\1Ret  driver init %p (obj=%p,str=%s) retval=%08lx\n",
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
        ERR( "failed to create driver %s: %08lx\n", debugstr_us(service_name), status );
        goto error;
    }

    driver = WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );
    driver->service_handle = service_handle;

    wine_enumerate_root_devices( service_name->Buffer + wcslen( servicesW ) );

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
    struct wine_driver *driver;
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
    driver = WINE_RB_ENTRY_VALUE( entry, struct wine_driver, entry );

    if (!list_empty( &driver->root_pnp_devices ))
    {
        ERR( "cannot unload driver %s which still has running PnP devices\n", debugstr_us(service_name) );
        return STATUS_UNSUCCESSFUL;
    }

    unload_driver( entry, NULL );

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           IoCreateFileEx (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateFileEx(HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                              IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size, ULONG attributes, ULONG sharing,
                              ULONG disposition, ULONG create_options, VOID *ea_buffer, ULONG ea_length,
                              CREATE_FILE_TYPE file_type, VOID *parameters, ULONG options, void *driverctx)
{
    FIXME(": semi-stub\n");
    return NtCreateFile(handle, access, attr, io, alloc_size, attributes, sharing, disposition,
                        create_options, ea_buffer, ea_length);
}

/***********************************************************************
 *           IoCreateFile (NTOSKRNL.EXE.@)
 */
NTSTATUS WINAPI IoCreateFile(HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                              IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size, ULONG attributes, ULONG sharing,
                              ULONG disposition, ULONG create_options, VOID *ea_buffer, ULONG ea_length,
                              CREATE_FILE_TYPE file_type, VOID *parameters, ULONG options )
{
    FIXME(": semi-stub\n");
    return IoCreateFileEx(handle, access, attr, io, alloc_size, attributes, sharing, disposition,
                          create_options, ea_buffer, ea_length, file_type, parameters, options, NULL);
}

/**************************************************************************
 *		__chkstk (NTOSKRNL.@)
 */
#ifdef __x86_64__
/* Supposed to touch all the stack pages, but we shouldn't need that. */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );
#elif defined(__i386__)
__ASM_GLOBAL_FUNC( _chkstk,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )
#elif defined(__arm__)
/* Incoming r4 contains words to allocate, converting to bytes then return */
__ASM_GLOBAL_FUNC( __chkstk, "lsl r4, r4, #2\n\t"
                             "bx lr" )
#elif defined(__aarch64__)
/* Supposed to touch all the stack pages, but we shouldn't need that. */
__ASM_GLOBAL_FUNC( __chkstk, "ret" );
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
    static int once;
    if (!once++) FIXME("stub: %08lx%08lx %u\n", privilege.HighPart, privilege.LowPart, mode);
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
 *           DbgQueryDebugFilterState    (NTOSKRNL.@)
 */
NTSTATUS WINAPI DbgQueryDebugFilterState(ULONG component, ULONG level)
{
    FIXME("stub: %ld %ld\n", component, level);
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
NTSTATUS WINAPI MmCopyVirtualMemory(PEPROCESS fromprocess, void *fromaddress, PEPROCESS toprocess,
                                    void *toaddress, SIZE_T bufsize, KPROCESSOR_MODE mode,
                                    SIZE_T *copied)
{
    FIXME("fromprocess %p, fromaddress %p, toprocess %p, toaddress %p, bufsize %Iu, mode %d, copied %p stub.\n",
            fromprocess, fromaddress, toprocess, toaddress, bufsize, mode, copied);

    *copied = 0;
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
    FIXME("stub: %lu %d\n", time, set_resolution);
    return KeQueryTimeIncrement();
}

/***********************************************************************
 *           IoGetRequestorProcess   (NTOSKRNL.EXE.@)
 */
PEPROCESS WINAPI IoGetRequestorProcess(IRP *irp)
{
    TRACE("irp %p.\n", irp);
    return irp->Tail.Overlay.Thread->kthread.process;
}

#ifdef _WIN64
/***********************************************************************
 *           IoIs32bitProcess   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI IoIs32bitProcess(IRP *irp)
{
    TRACE("irp %p.\n", irp);
    return irp->Tail.Overlay.Thread->kthread.process->wow64;
}
#endif

/***********************************************************************
 *           RtlIsNtDdiVersionAvailable   (NTOSKRNL.EXE.@)
 */
BOOLEAN WINAPI RtlIsNtDdiVersionAvailable(ULONG version)
{
    FIXME("stub: %ld\n", version);
    return FALSE;
}

BOOLEAN WINAPI KdRefreshDebuggerNotPresent(void)
{
    TRACE(".\n");

    return !KdDebuggerEnabled;
}

struct generic_call_dpc_context
{
    DEFERRED_REVERSE_BARRIER *reverse_barrier;
    PKDEFERRED_ROUTINE routine;
    ULONG *cpu_count_barrier;
    void *context;
    ULONG cpu_index;
    ULONG current_barrier_flag;
    LONG *barrier_passed_count;
};

static void WINAPI generic_call_dpc_callback(TP_CALLBACK_INSTANCE *instance, void *context)
{
    struct generic_call_dpc_context *c = context;
    GROUP_AFFINITY old, new;

    TRACE("instance %p, context %p.\n", instance, context);

    NtQueryInformationThread(GetCurrentThread(), ThreadGroupInformation,
            &old, sizeof(old), NULL);

    memset(&new, 0, sizeof(new));

    new.Mask = 1 << c->cpu_index;
    NtSetInformationThread(GetCurrentThread(), ThreadGroupInformation, &new, sizeof(new));

    TlsSetValue(dpc_call_tls_index, context);
    c->routine((PKDPC)0xdeadbeef, c->context, c->cpu_count_barrier, c->reverse_barrier);
    TlsSetValue(dpc_call_tls_index, NULL);
    NtSetInformationThread(GetCurrentThread(), ThreadGroupInformation, &old, sizeof(old));
}

void WINAPI KeGenericCallDpc(PKDEFERRED_ROUTINE routine, void *context)
{
    ULONG cpu_count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);
    static struct generic_call_dpc_context *contexts;
    DEFERRED_REVERSE_BARRIER reverse_barrier;
    static ULONG last_cpu_count;
    LONG barrier_passed_count;
    ULONG cpu_count_barrier;
    ULONG i;

    TRACE("routine %p, context %p.\n", routine, context);

    EnterCriticalSection(&dpc_call_cs);

    if (!dpc_call_tp)
    {
        if (!(dpc_call_tp = CreateThreadpool(NULL)))
        {
            ERR("Could not create thread pool.\n");
            LeaveCriticalSection(&dpc_call_cs);
            return;
        }

        SetThreadpoolThreadMinimum(dpc_call_tp, cpu_count);
        SetThreadpoolThreadMaximum(dpc_call_tp, cpu_count);

        memset(&dpc_call_tpe, 0, sizeof(dpc_call_tpe));
        dpc_call_tpe.Version = 1;
        dpc_call_tpe.Pool = dpc_call_tp;
    }

    reverse_barrier.Barrier = cpu_count;
    reverse_barrier.TotalProcessors = cpu_count;
    cpu_count_barrier = cpu_count;

    if (contexts)
    {
        if (last_cpu_count < cpu_count)
        {
            static struct generic_call_dpc_context *new_contexts;
            if (!(new_contexts = heap_realloc(contexts, sizeof(*contexts) * cpu_count)))
            {
                ERR("No memory.\n");
                LeaveCriticalSection(&dpc_call_cs);
                return;
            }
            contexts = new_contexts;
            SetThreadpoolThreadMinimum(dpc_call_tp, cpu_count);
            SetThreadpoolThreadMaximum(dpc_call_tp, cpu_count);
        }
    }
    else if (!(contexts = heap_alloc(sizeof(*contexts) * cpu_count)))
    {
        ERR("No memory.\n");
        LeaveCriticalSection(&dpc_call_cs);
        return;
    }

    memset(contexts, 0, sizeof(*contexts) * cpu_count);
    last_cpu_count = cpu_count;
    barrier_passed_count = 0;

    for (i = 0; i < cpu_count; ++i)
    {
        contexts[i].reverse_barrier = &reverse_barrier;
        contexts[i].cpu_count_barrier = &cpu_count_barrier;
        contexts[i].routine = routine;
        contexts[i].context = context;
        contexts[i].cpu_index = i;
        contexts[i].barrier_passed_count = &barrier_passed_count;

        TrySubmitThreadpoolCallback(generic_call_dpc_callback, &contexts[i], &dpc_call_tpe);
    }

    while (InterlockedCompareExchange((LONG *)&cpu_count_barrier, 0, 0))
        SwitchToThread();

    LeaveCriticalSection(&dpc_call_cs);
}


BOOLEAN WINAPI KeSignalCallDpcSynchronize(void *barrier)
{
    struct generic_call_dpc_context *context = TlsGetValue(dpc_call_tls_index);
    DEFERRED_REVERSE_BARRIER *b = barrier;
    LONG curr_flag, comp, done_value;
    BOOL first;

    TRACE("barrier %p, context %p.\n", barrier, context);

    if (!context)
    {
        WARN("Called outside of DPC context.\n");
        return FALSE;
    }

    context->current_barrier_flag ^= 0x80000000;
    curr_flag = context->current_barrier_flag;

    first = !context->cpu_index;
    comp = curr_flag + context->cpu_index;
    done_value = curr_flag + b->TotalProcessors;

    if (first)
        InterlockedExchange((LONG *)&b->Barrier, comp);

    while (InterlockedCompareExchange((LONG *)&b->Barrier, comp + 1, comp) != done_value)
        ;

    InterlockedIncrement(context->barrier_passed_count);

    while (first && InterlockedCompareExchange(context->barrier_passed_count, 0, b->TotalProcessors))
        ;

    return first;
}

void WINAPI KeSignalCallDpcDone(void *barrier)
{
    InterlockedDecrement((LONG *)barrier);
}

void * WINAPI PsGetProcessSectionBaseAddress(PEPROCESS process)
{
    void *image_base;
    NTSTATUS status;
    SIZE_T size;
    HANDLE h;

    TRACE("process %p.\n", process);

    if ((status = ObOpenObjectByPointer(process, 0, NULL, PROCESS_ALL_ACCESS, NULL, KernelMode, &h)))
    {
        WARN("Error opening process object, status %#lx.\n", status);
        return NULL;
    }

    status = NtReadVirtualMemory(h, &process->info.PebBaseAddress->ImageBaseAddress,
            &image_base, sizeof(image_base), &size);

    NtClose(h);

    if (status || size != sizeof(image_base))
    {
        WARN("Error reading process memory, status %#lx, size %Iu.\n", status, size);
        return NULL;
    }

    TRACE("returning %p.\n", image_base);
    return image_base;
}

void WINAPI KeStackAttachProcess(KPROCESS *process, KAPC_STATE *apc_state)
{
    FIXME("process %p, apc_state %p stub.\n", process, apc_state);
}

void WINAPI KeUnstackDetachProcess(KAPC_STATE *apc_state)
{
    FIXME("apc_state %p stub.\n", apc_state);
}

NTSTATUS WINAPI KdDisableDebugger(void)
{
    FIXME(": stub.\n");
    return STATUS_DEBUGGER_INACTIVE;
}

NTSTATUS WINAPI KdEnableDebugger(void)
{
    FIXME(": stub.\n");
    return STATUS_DEBUGGER_INACTIVE;
}

KPROCESSOR_MODE WINAPI ExGetPreviousMode(void)
{
    TRACE("\n");
    return PsIsSystemThread((PETHREAD)KeGetCurrentThread()) ? KernelMode : UserMode;
}

#ifdef __x86_64__

void WINAPI KfRaiseIrql(KIRQL new, KIRQL *old)
{
    FIXME("new %u old %p: stub.\n", new, old);
}

void WINAPI KeLowerIrql(KIRQL new)
{
    FIXME("new %u: stub.\n", new);
}

#endif

typedef void (WINAPI *PETW_CLASSIC_CALLBACK)(
    const GUID *guid, UCHAR control_code, void *enable_context, void *callback_context);

NTSTATUS WINAPI EtwRegisterClassicProvider(const GUID *provider, ULONG type, PETW_CLASSIC_CALLBACK callback,
                                           void *context, REGHANDLE *handle)
{
    FIXME("provider %s, type %lu, enable_callback %p, context %p, handle %p\n", debugstr_guid(provider), type,
          callback, context, handle);

    *handle = 0xdeadbeef;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI EtwUnregister(REGHANDLE handle)
{
    FIXME("handle %I64x\n", handle);
    return STATUS_SUCCESS;
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
        ntoskrnl_heap = HeapCreate( HEAP_CREATE_ENABLE_EXECUTE, 0, 0 );
        dpc_call_tls_index = TlsAlloc();
        LdrRegisterDllNotification( 0, ldr_notify_callback, NULL, &ldr_notify_cookie );
        break;
    case DLL_PROCESS_DETACH:
        LdrUnregisterDllNotification( ldr_notify_cookie );

        if (reserved) break;

        if (dpc_call_tp)
            CloseThreadpool(dpc_call_tp);

        HeapDestroy( ntoskrnl_heap );
        RtlRemoveVectoredExceptionHandler( handler );
        break;
    }
    return TRUE;
}
