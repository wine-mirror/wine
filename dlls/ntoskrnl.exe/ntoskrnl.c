/*
 * ntoskrnl.exe implementation
 *
 * Copyright (C) 2007 Alexandre Julliard
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
#include "ddk/wdm.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntoskrnl);
WINE_DECLARE_DEBUG_CHANNEL(relay);


KSYSTEM_TIME KeTickCount;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
    PULONG_PTR Base;
    PULONG Count;
    ULONG Limit;
    PUCHAR Number;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[4];


#ifdef __i386__
#define DEFINE_FASTCALL1_ENTRYPOINT( name ) \
    __ASM_GLOBAL_FUNC( name, \
                       "popl %eax\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name )
#define DEFINE_FASTCALL2_ENTRYPOINT( name ) \
    __ASM_GLOBAL_FUNC( name, \
                       "popl %eax\n\t" \
                       "pushl %edx\n\t" \
                       "pushl %ecx\n\t" \
                       "pushl %eax\n\t" \
                       "jmp " __ASM_NAME("__regs_") #name )
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
            if (!wine_server_call( req )) handle = reply->handle;
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

    TRACE( "ioctl %x device %p in_size %u out_size %u\n", code, device, in_size, *out_size );

    /* so we can spot things that we should initialize */
    memset( &irp, 0x55, sizeof(irp) );
    memset( &irpsp, 0x66, sizeof(irpsp) );
    memset( &mdl, 0x77, sizeof(mdl) );

    irp.RequestorMode = UserMode;
    irp.AssociatedIrp.SystemBuffer = in_buff;
    irp.UserBuffer = out_buff;
    irp.MdlAddress = &mdl;
    irp.Tail.Overlay.s.u.CurrentStackLocation = &irpsp;

    irpsp.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpsp.Parameters.DeviceIoControl.OutputBufferLength = *out_size;
    irpsp.Parameters.DeviceIoControl.InputBufferLength = in_size;
    irpsp.Parameters.DeviceIoControl.IoControlCode = code;
    irpsp.Parameters.DeviceIoControl.Type3InputBuffer = in_buff;
    irpsp.DeviceObject = device;

    mdl.Next = NULL;
    mdl.Size = 0;
    mdl.StartVa = out_buff;
    mdl.ByteCount = *out_size;
    mdl.ByteOffset = 0;

    device->CurrentIrp = &irp;

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call driver dispatch %p (device=%p,irp=%p)\n",
                 GetCurrentThreadId(), dispatch, device, &irp );

    status = dispatch( device, &irp );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  driver dispatch %p (device=%p,irp=%p) retval=%08x\n",
                 GetCurrentThreadId(), dispatch, device, &irp, status );

    *out_size = irp.IoStatus.u.Status ? 0 : irp.IoStatus.Information;
    return irp.IoStatus.u.Status;
}


/***********************************************************************
 *           wine_ntoskrnl_main_loop   (Not a Windows API)
 */
NTSTATUS wine_ntoskrnl_main_loop( HANDLE stop_event )
{
    HANDLE manager = get_device_manager();
    HANDLE ioctl = 0;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG code = 0;
    void *in_buff, *out_buff = NULL;
    DEVICE_OBJECT *device = NULL;
    ULONG in_size = 4096, out_size = 0;
    HANDLE handles[2];

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
            req->manager = manager;
            req->prev = ioctl;
            req->status = status;
            wine_server_add_data( req, out_buff, out_size );
            wine_server_set_reply( req, in_buff, in_size );
            if (!(status = wine_server_call( req )))
            {
                code     = reply->code;
                ioctl    = reply->next;
                device   = reply->user_ptr;
                in_size  = reply->in_size;
                out_size = reply->out_size;
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
                return STATUS_SUCCESS;
            break;
        }
    }
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
        req->manager    = manager;
        req->user_ptr   = device;
        if (name) wine_server_add_data( req, name->Buffer, name->Length );
        if (!(status = wine_server_call( req ))) handle = reply->handle;
    }
    SERVER_END_REQ;

    if (status == STATUS_SUCCESS)
    {
        device->DriverObject    = driver;
        device->DeviceExtension = device + 1;
        device->DeviceType      = type;
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
        req->handle = device->Reserved;
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
    /* nothing to do for now */
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
 *           MmAllocateNonCachedMemory   (NTOSKRNL.EXE.@)
 */
LPVOID WINAPI MmAllocateNonCachedMemory( SIZE_T size )
{
    TRACE( "%lu\n", size );
    return VirtualAlloc( NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE|PAGE_NOCACHE );
}


/***********************************************************************
 *           MmFreeNonCachedMemory   (NTOSKRNL.EXE.@)
 */
void WINAPI MmFreeNonCachedMemory( void *addr, SIZE_T size )
{
    TRACE( "%p %lu\n", addr, size );
    VirtualFree( addr, 0, MEM_RELEASE );
}


/*****************************************************
 *           DllMain
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}
