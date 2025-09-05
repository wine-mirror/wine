/*
 * Plug and Play test driver
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
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

#if 0
#pragma makedep testdll
#endif

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ddk/hidsdi.h"
#include "ddk/hidport.h"

#include "wine/list.h"

#define WINE_DRIVER_TEST
#include "initguid.h"
#include "driver_hid.h"

typedef ULONG PNP_DEVICE_STATE;
#define PNP_DEVICE_REMOVED 8

#define check_buffer( a, b ) check_buffer_( __LINE__, a, b )
static void check_buffer_( int line, HID_XFER_PACKET *packet, struct hid_expect *expect )
{
    ULONG match_len, i;

    match_len = RtlCompareMemory( packet->reportBuffer, expect->report_buf, expect->report_len );
    ok( match_len == expect->report_len, "unexpected data:\n" );
    if (match_len == expect->report_len) return;

    for (i = 0; i < packet->reportBufferLen;)
    {
        char buffer[256], *buf = buffer;
        buf += sprintf( buf, "%08lx ", i );
        do buf += sprintf( buf, " %02x", packet->reportBuffer[i] );
        while (++i % 16 && i < packet->reportBufferLen);
        ok( 0, "  %s\n", buffer );
    }
}

struct wait_queue
{
    KSPIN_LOCK lock;
    IRP *pending_wait;
};

static void wait_queue_cleanup( struct wait_queue *queue )
{
    KIRQL irql;
    IRP *irp;

    KeAcquireSpinLock( &queue->lock, &irql );
    if ((irp = queue->pending_wait))
    {
        queue->pending_wait = NULL;
        if (!IoSetCancelRoutine( irp, NULL )) irp = NULL;
    }
    KeReleaseSpinLock( &queue->lock, irql );

    if (irp)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
}

static void wait_queue_unlock( struct wait_queue *queue, KIRQL irql, BOOL empty )
{
    IRP *irp;

    /* complete the pending wait IRP if the queue is now empty */
    if ((irp = empty ? queue->pending_wait : NULL))
    {
        queue->pending_wait = NULL;
        if (!IoSetCancelRoutine( irp, NULL )) irp = NULL;
    }

    KeReleaseSpinLock( &queue->lock, irql );

    if (irp)
    {
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
}

struct expect_queue
{
    struct wait_queue base;
    struct hid_expect *pos;
    struct hid_expect *end;
    struct hid_expect spurious;
    struct hid_expect *buffer;
    char context[64];
};

static void expect_queue_init( struct expect_queue *queue )
{
    KeInitializeSpinLock( &queue->base.lock );
    queue->buffer = ExAllocatePool( PagedPool, EXPECT_QUEUE_BUFFER_SIZE );
    RtlSecureZeroMemory( queue->buffer, EXPECT_QUEUE_BUFFER_SIZE );
    queue->pos = queue->buffer;
    queue->end = queue->buffer;
}

static void expect_queue_cleanup( struct expect_queue *queue )
{
    wait_queue_cleanup( &queue->base );
    ExFreePool( queue->buffer );
}

static void expect_queue_reset( struct expect_queue *queue, void *buffer, unsigned int size )
{
    struct hid_expect *missing, *missing_end, *tmp;
    char context[64];
    KIRQL irql;

    missing = ExAllocatePool( PagedPool, EXPECT_QUEUE_BUFFER_SIZE );
    RtlSecureZeroMemory( missing, EXPECT_QUEUE_BUFFER_SIZE );
    missing_end = missing;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    tmp = queue->pos;
    while (tmp < queue->end) *missing_end++ = *tmp++;

    queue->pos = queue->buffer;
    queue->end = queue->buffer;

    if (size) memcpy( queue->end, buffer, size );
    queue->end = queue->end + size / sizeof(struct hid_expect);
    memcpy( context, queue->context, sizeof(context) );
    KeReleaseSpinLock( &queue->base.lock, irql );

    tmp = missing;
    while (tmp != missing_end)
    {
        winetest_push_context( "%s expect[%Id]", context, tmp - missing );
        if (tmp->broken_id)
        {
            todo_wine_if( tmp->todo )
            win_skip( "broken (code %#lx id %u len %u)\n", tmp->code, tmp->report_id, tmp->report_len );
        }
        else
        {
            todo_wine_if( tmp->todo )
            ok( tmp->wine_only, "missing (code %#lx id %u len %u)\n", tmp->code, tmp->report_id, tmp->report_len );
        }
        winetest_pop_context();
        tmp++;
    }

    ExFreePool( missing );
}

static void WINAPI wait_cancel_routine( DEVICE_OBJECT *device, IRP *irp )
{
    struct wait_queue *queue = irp->Tail.Overlay.DriverContext[0];
    KIRQL irql;

    IoReleaseCancelSpinLock( irp->CancelIrql );

    KeAcquireSpinLock( &queue->lock, &irql );
    queue->pending_wait = NULL;
    KeReleaseSpinLock( &queue->lock, irql );

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
}

static NTSTATUS wait_queue_add_pending_locked( struct wait_queue *queue, IRP *irp )
{
    if (queue->pending_wait) return STATUS_INVALID_PARAMETER;

    IoSetCancelRoutine( irp, wait_cancel_routine );
    if (irp->Cancel && IoSetCancelRoutine( irp, NULL ))
        return STATUS_CANCELLED;

    irp->Tail.Overlay.DriverContext[0] = queue;
    IoMarkIrpPending( irp );
    queue->pending_wait = irp;

    return STATUS_PENDING;
}

static NTSTATUS expect_queue_add_pending( struct expect_queue *queue, IRP *irp )
{
    NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    status = wait_queue_add_pending_locked( &queue->base, irp );
    KeReleaseSpinLock( &queue->base.lock, irql );

    return status;
}

/* complete an expect report previously marked as pending, or wait for one and then for the queue to empty */
static NTSTATUS expect_queue_wait_pending( struct expect_queue *queue, IRP *irp )
{
    NTSTATUS status;
    IRP *pending;
    KIRQL irql;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    if ((pending = queue->base.pending_wait))
    {
        queue->base.pending_wait = NULL;
        if (!IoSetCancelRoutine( pending, NULL )) pending = NULL;
    }

    if (pending && queue->pos == queue->end) status = STATUS_SUCCESS;
    else status = wait_queue_add_pending_locked( &queue->base, irp );
    KeReleaseSpinLock( &queue->base.lock, irql );

    if (pending)
    {
        pending->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( pending, IO_NO_INCREMENT );
    }

    return status;
}

/* wait for the expect queue to empty */
static NTSTATUS expect_queue_wait( struct expect_queue *queue, IRP *irp )
{
    NTSTATUS status;
    KIRQL irql;

    irp->IoStatus.Information = 0;
    KeAcquireSpinLock( &queue->base.lock, &irql );
    if (queue->pos == queue->end) status = STATUS_SUCCESS;
    else status = wait_queue_add_pending_locked( &queue->base, irp );
    KeReleaseSpinLock( &queue->base.lock, irql );

    return status;
}

static void expect_queue_next( struct expect_queue *queue, ULONG code, HID_XFER_PACKET *packet, LONG *index,
                               struct hid_expect *expect, BOOL compare_buf, char *context, ULONG context_size )
{
    struct hid_expect *missing, *missing_end, *tmp;
    ULONG len = packet->reportBufferLen;
    BYTE *buf = packet->reportBuffer;
    BYTE id = packet->reportId;
    KIRQL irql;

    missing = ExAllocatePool( PagedPool, EXPECT_QUEUE_BUFFER_SIZE );
    RtlSecureZeroMemory( missing, EXPECT_QUEUE_BUFFER_SIZE );
    missing_end = missing;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    tmp = queue->pos;
    while (tmp < queue->end)
    {
        BOOL is_missing = tmp->broken_id == (BYTE)-1;
        if (winetest_platform_is_wine && !tmp->todo) break;
        if (!winetest_platform_is_wine && !is_missing && !tmp->wine_only) break;
        if (tmp->code == code && tmp->report_id == id && tmp->report_len == len &&
            (!compare_buf || RtlCompareMemory( tmp->report_buf, buf, len ) == len))
            break;
        *missing_end++ = *tmp++;
    }
    *index = tmp - queue->buffer;
    if (tmp < queue->end) queue->pos = tmp + 1;
    else tmp = &queue->spurious;
    *expect = *tmp;

    while (queue->pos < queue->end)
    {
        if (winetest_platform_is_wine || !queue->pos->wine_only) break;
        queue->pos++;
    }

    /* don't mark the IRP as pending if someone's already waiting */
    if (expect->ret_status == STATUS_PENDING && queue->base.pending_wait)
        expect->ret_status = STATUS_SUCCESS;

    memcpy( context, queue->context, context_size );
    wait_queue_unlock( &queue->base, irql, queue->pos == queue->end );

    ok( tmp != &queue->spurious, "%s got spurious packet\n", context );

    winetest_push_context( "%s expect[%Id]", context, tmp - queue->buffer );
    todo_wine_if( tmp->todo )
    ok( !tmp->wine_only, "found code %#lx id %u len %u\n", tmp->code, tmp->report_id, tmp->report_len );
    winetest_pop_context();

    tmp = missing;
    while (tmp != missing_end)
    {
        winetest_push_context( "%s expect[%Id]", context, tmp - missing );
        if (tmp->broken_id)
        {
            todo_wine_if( tmp->todo )
            win_skip( "broken (code %#lx id %u len %u)\n", tmp->code, tmp->report_id, tmp->report_len );
        }
        else
        {
            todo_wine_if( tmp->todo )
            ok( tmp->wine_only, "missing (code %#lx id %u len %u)\n", tmp->code, tmp->report_id, tmp->report_len );
        }
        winetest_pop_context();
        tmp++;
    }

    ExFreePool( missing );
}

struct irp_queue
{
    KSPIN_LOCK lock;
    LIST_ENTRY list;
};

static IRP *irp_queue_pop( struct irp_queue *queue )
{
    KIRQL irql;
    IRP *irp;

    KeAcquireSpinLock( &queue->lock, &irql );
    if (IsListEmpty( &queue->list )) irp = NULL;
    else irp = CONTAINING_RECORD( RemoveHeadList( &queue->list ), IRP, Tail.Overlay.ListEntry );
    KeReleaseSpinLock( &queue->lock, irql );

    return irp;
}

static void irp_queue_push( struct irp_queue *queue, IRP *irp )
{
    KIRQL irql;

    KeAcquireSpinLock( &queue->lock, &irql );
    InsertTailList( &queue->list, &irp->Tail.Overlay.ListEntry );
    KeReleaseSpinLock( &queue->lock, irql );
}

static void irp_queue_complete( struct irp_queue *queue, BOOL delete )
{
    IRP *irp;

    while ((irp = irp_queue_pop( queue )))
    {
        if (delete) irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
}

static void irp_queue_init( struct irp_queue *queue )
{
    KeInitializeSpinLock( &queue->lock );
    InitializeListHead( &queue->list );
}

struct input_queue
{
    struct wait_queue base;
    BOOL is_polled;
    struct hid_expect *pos;
    struct hid_expect *end;
    struct hid_expect *buffer;
    struct irp_queue pending;
    struct irp_queue completed;
};

static void input_queue_init( struct input_queue *queue, BOOL is_polled )
{
    KeInitializeSpinLock( &queue->base.lock );
    queue->is_polled = is_polled;
    queue->buffer = ExAllocatePool( PagedPool, EXPECT_QUEUE_BUFFER_SIZE );
    RtlSecureZeroMemory( queue->buffer, EXPECT_QUEUE_BUFFER_SIZE );
    queue->pos = queue->buffer;
    queue->end = queue->buffer;
    irp_queue_init( &queue->pending );
    irp_queue_init( &queue->completed );
}

static void input_queue_cleanup( struct input_queue *queue )
{
    wait_queue_cleanup( &queue->base );
    ExFreePool( queue->buffer );
}

static BOOL input_queue_read_locked( struct input_queue *queue, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
    struct hid_expect *tmp = queue->pos;

    if (tmp >= queue->end) return FALSE;
    if (tmp->ret_length) out_size = tmp->ret_length;

    memcpy( irp->UserBuffer, tmp->report_buf, out_size );
    irp->IoStatus.Information = out_size;
    irp->IoStatus.Status = tmp->ret_status;
    if (tmp < queue->end) queue->pos = tmp + 1;

    /* loop on the queue data in polled mode */
    if (queue->is_polled && queue->pos == queue->end) queue->pos = queue->buffer;
    return TRUE;
}

static NTSTATUS input_queue_read( struct input_queue *queue, IRP *irp )
{
    NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    if (input_queue_read_locked( queue, irp ))
    {
        irp_queue_push( &queue->completed, irp );
        status = STATUS_SUCCESS;
    }
    else
    {
        IoMarkIrpPending( irp );
        irp_queue_push( &queue->pending, irp );
        status = STATUS_PENDING;
    }
    wait_queue_unlock( &queue->base, irql, queue->pos == queue->end );

    return status;
}

static void input_queue_reset( struct input_queue *queue, void *in_buf, ULONG in_size )
{
    ULONG remaining;
    KIRQL irql;
    IRP *irp;

    KeAcquireSpinLock( &queue->base.lock, &irql );
    remaining = queue->end - queue->pos;
    queue->pos = queue->buffer;
    queue->end = queue->buffer;
    memcpy( queue->end, in_buf, in_size );
    queue->end += in_size / sizeof(struct hid_expect);

    while (!queue->is_polled && queue->pos < queue->end && (irp = irp_queue_pop( &queue->pending )))
    {
        input_queue_read_locked( queue, irp );
        irp_queue_push( &queue->completed, irp );
    }
    wait_queue_unlock( &queue->base, irql, queue->pos == queue->end );

    if (!queue->is_polled) ok( !remaining, "unread input\n" );

    irp_queue_complete( &queue->completed, FALSE );
}

/* wait for the input queue to empty */
static NTSTATUS input_queue_wait( struct input_queue *queue, IRP *irp )
{
    NTSTATUS status;
    KIRQL irql;

    irp->IoStatus.Information = 0;
    KeAcquireSpinLock( &queue->base.lock, &irql );
    if (queue->pos == queue->end) status = STATUS_SUCCESS;
    else status = wait_queue_add_pending_locked( &queue->base, irp );
    KeReleaseSpinLock( &queue->base.lock, irql );

    return status;
}

struct device
{
    KSPIN_LOCK lock;
    PNP_DEVICE_STATE state;
    BOOL is_phys;
};

static inline struct device *impl_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    return (struct device *)device->DeviceExtension;
}

struct phys_device
{
    struct device base;
    struct func_device *fdo; /* parent FDO */

    WCHAR instance_id[MAX_PATH];
    WCHAR device_id[MAX_PATH];
    IRP *pending_remove;

    BOOL use_report_id;
    DWORD report_descriptor_len;
    char report_descriptor_buf[MAX_HID_DESCRIPTOR_LEN];

    HIDP_CAPS caps;
    HID_DEVICE_ATTRIBUTES attributes;
    struct expect_queue expect_queue;
    struct input_queue input_queue;
};

static inline struct phys_device *pdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    return CONTAINING_RECORD( impl, struct phys_device, base );
}

struct func_device
{
    struct device base;
    DEVICE_OBJECT *pdo; /* lower PDO */
    UNICODE_STRING control_iface;
    char devices_buffer[offsetof( DEVICE_RELATIONS, Objects[128] )];
    DEVICE_RELATIONS *devices;
};

static inline struct func_device *fdo_from_DEVICE_OBJECT( DEVICE_OBJECT *device )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return CONTAINING_RECORD( impl, struct phys_device, base )->fdo;
    return CONTAINING_RECORD( impl, struct func_device, base );
}

#ifdef __ASM_USE_FASTCALL_WRAPPER
extern void *WINAPI wrap_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                    "popl %ecx\n\t"
                    "popl %eax\n\t"
                    "xchgl (%esp),%ecx\n\t"
                    "jmp *%eax" );
#define call_fastcall_func1( func, a ) wrap_fastcall_func1( func, a )
#else
#define call_fastcall_func1( func, a ) func( a )
#endif

static NTSTATUS remove_child_device( struct func_device *impl, DEVICE_OBJECT *device )
{
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
    ULONG i;

    KeAcquireSpinLock( &impl->base.lock, &irql );
    for (i = 0; i < impl->devices->Count; ++i)
        if (impl->devices->Objects[i] == device) break;
    if (i == impl->devices->Count) status = STATUS_NOT_FOUND;
    else impl->devices->Objects[i] = impl->devices->Objects[impl->devices->Count--];
    KeReleaseSpinLock( &impl->base.lock, irql );

    return status;
}

static NTSTATUS append_child_device( struct func_device *impl, DEVICE_OBJECT *device )
{
    NTSTATUS status;
    KIRQL irql;

    KeAcquireSpinLock( &impl->base.lock, &irql );
    if (offsetof( DEVICE_RELATIONS, Objects[impl->devices->Count + 1] ) > sizeof(impl->devices_buffer))
        status = STATUS_NO_MEMORY;
    else
    {
        impl->devices->Objects[impl->devices->Count++] = device;
        status = STATUS_SUCCESS;
    }
    KeReleaseSpinLock( &impl->base.lock, irql );

    return status;
}

static DEVICE_OBJECT *find_child_device( struct func_device *impl, struct hid_device_desc *desc )
{
    DEVICE_OBJECT *device = NULL, **devices;
    WCHAR device_id[MAX_PATH];
    KIRQL irql;
    ULONG i;

    swprintf( device_id, MAX_PATH, L"WINETEST\\VID_%04X&PID_%04X", desc->attributes.VendorID,
              desc->attributes.ProductID );
    if (desc->is_polled) wcscat( device_id, L"&POLL" );

    KeAcquireSpinLock( &impl->base.lock, &irql );
    devices = impl->devices->Objects;
    for (i = 0; i < impl->devices->Count; ++i)
    {
        struct phys_device *phys = pdo_from_DEVICE_OBJECT( (device = devices[i]) );
        if (!wcscmp( phys->device_id, device_id )) break;
        else device = NULL;
    }
    KeReleaseSpinLock( &impl->base.lock, irql );

    return device;
}

static ULONG_PTR get_device_relations( DEVICE_OBJECT *device, DEVICE_RELATIONS *previous,
                                       ULONG count, DEVICE_OBJECT **devices )
{
    DEVICE_RELATIONS *relations;
    ULONG new_count = count;

    if (previous) new_count += previous->Count;
    if (!(relations = ExAllocatePool( PagedPool, offsetof( DEVICE_RELATIONS, Objects[new_count] ) )))
    {
        ok( 0, "Failed to allocate memory\n" );
        return (ULONG_PTR)previous;
    }

    if (!previous) relations->Count = 0;
    else
    {
        memcpy( relations, previous, offsetof( DEVICE_RELATIONS, Objects[previous->Count] ) );
        ExFreePool( previous );
    }

    while (count--)
    {
        call_fastcall_func1( ObfReferenceObject, *devices );
        relations->Objects[relations->Count++] = *devices++;
    }

    return (ULONG_PTR)relations;
}

static WCHAR *query_instance_id( DEVICE_OBJECT *device )
{
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    DWORD size = (wcslen( impl->instance_id ) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, impl->instance_id, size );

    return dst;
}

static WCHAR *query_hardware_ids( DEVICE_OBJECT *device )
{
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    DWORD size = (wcslen( impl->device_id ) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size + sizeof(WCHAR) )))
    {
        memcpy( dst, impl->device_id, size );
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static WCHAR *query_compatible_ids( DEVICE_OBJECT *device )
{
    static const WCHAR hid_compat_id[] = L"WINETEST\\WINE_COMP_HID";
    static const WCHAR hid_poll_compat_id[] = L"WINETEST\\WINE_COMP_POLLHID";
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    const WCHAR *compat_id = impl->input_queue.is_polled ? hid_poll_compat_id : hid_compat_id;
    DWORD size = (wcslen( compat_id ) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size + sizeof(WCHAR) )))
    {
        memcpy( dst, compat_id, size );
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static WCHAR *query_container_id( DEVICE_OBJECT *device )
{
    static const WCHAR winetest_id[] = L"WINETEST";
    DWORD size = sizeof(winetest_id);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, winetest_id, sizeof(winetest_id) );

    return dst;
}

typedef struct _PNP_BUS_INFORMATION
{
    GUID BusTypeGuid;
    INTERFACE_TYPE LegacyBusType;
    ULONG BusNumber;
} PNP_BUS_INFORMATION, *PPNP_BUS_INFORMATION;

static PNP_BUS_INFORMATION *query_bus_information( DEVICE_OBJECT *device )
{
    DWORD size = sizeof(PNP_BUS_INFORMATION);
    PNP_BUS_INFORMATION *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
    {
        memset( &dst->BusTypeGuid, 0, sizeof(dst->BusTypeGuid) );
        dst->LegacyBusType = PNPBus;
        dst->BusNumber = 0;
    }

    return dst;
}

static WCHAR *query_device_text( DEVICE_OBJECT *device )
{
    static const WCHAR device_text[] = L"Wine Test HID device";
    DWORD size = sizeof(device_text);
    WCHAR *dst;

    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, device_text, size );

    return dst;
}

static NTSTATUS pdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    ULONG code = stack->MinorFunction;
    PNP_DEVICE_STATE state;
    NTSTATUS status;
    KIRQL irql;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_pnp(code) );

    switch (code)
    {
    case IRP_MN_START_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_REMOVE_DEVICE:
        state = (code == IRP_MN_START_DEVICE || code == IRP_MN_CANCEL_REMOVE_DEVICE) ? 0 : PNP_DEVICE_REMOVED;
        KeAcquireSpinLock( &impl->base.lock, &irql );
        impl->base.state = state;
        irp_queue_complete( &impl->input_queue.pending, TRUE );
        irp_queue_complete( &impl->input_queue.completed, TRUE );
        KeReleaseSpinLock( &impl->base.lock, irql );
        if (code != IRP_MN_REMOVE_DEVICE) status = STATUS_SUCCESS;
        else
        {
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( irp, IO_NO_INCREMENT );
            if (remove_child_device( fdo, device ))
            {
                input_queue_cleanup( &impl->input_queue );
                expect_queue_cleanup( &impl->expect_queue );
                irp = impl->pending_remove;
                IoDeleteDevice( device );
                if (winetest_debug > 1) trace( "Deleted Bus PDO %p\n", device );
                if (irp)
                {
                    irp->IoStatus.Status = STATUS_SUCCESS;
                    IoCompleteRequest( irp, IO_NO_INCREMENT );
                }
            }
            return STATUS_SUCCESS;
        }
        break;
    case IRP_MN_QUERY_CAPABILITIES:
    {
        DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
        caps->Removable = 1;
        caps->SilentInstall = 1;
        caps->SurpriseRemovalOK = 1;
        /* caps->RawDeviceOK = 1; */
        status = STATUS_SUCCESS;
        break;
    }
    case IRP_MN_QUERY_ID:
    {
        BUS_QUERY_ID_TYPE type = stack->Parameters.QueryId.IdType;
        switch (type)
        {
        case BusQueryDeviceID:
        case BusQueryHardwareIDs:
            irp->IoStatus.Information = (ULONG_PTR)query_hardware_ids( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryInstanceID:
            irp->IoStatus.Information = (ULONG_PTR)query_instance_id( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryCompatibleIDs:
            irp->IoStatus.Information = (ULONG_PTR)query_compatible_ids( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case BusQueryContainerID:
            irp->IoStatus.Information = (ULONG_PTR)query_container_id( device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        default:
            ok( 0, "IRP_MN_QUERY_ID type %u, not implemented!\n", type );
            status = STATUS_NOT_SUPPORTED;
            break;
        }
        break;
    }
    case IRP_MN_QUERY_BUS_INFORMATION:
        irp->IoStatus.Information = (ULONG_PTR)query_bus_information( device );
        if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
        else status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_TEXT:
        irp->IoStatus.Information = (ULONG_PTR)query_device_text( device );
        if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
        else status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        irp->IoStatus.Information = impl->base.state;
        status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        DEVICE_RELATION_TYPE type = stack->Parameters.QueryDeviceRelations.Type;
        switch (type)
        {
        case BusRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS BusRelations\n" );
            ok( irp->IoStatus.Information, "got unexpected BusRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case EjectionRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS EjectionRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected EjectionRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case RemovalRelations:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS RemovalRelations\n" );
            ok( irp->IoStatus.Information, "got unexpected RemovalRelations relations\n" );
            status = irp->IoStatus.Status;
            break;
        case TargetDeviceRelation:
            if (winetest_debug > 1) trace( "pdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS TargetDeviceRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected TargetDeviceRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information, 1, &device );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        default:
            ok( 0, "got unexpected IRP_MN_QUERY_DEVICE_RELATIONS type %#x\n", type );
            status = irp->IoStatus.Status;
            break;
        }
        break;
    }
    default:
        if (winetest_debug > 1) trace( "pdo_pnp code %#lx %s, not implemented!\n", code, debugstr_pnp(code) );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS create_child_pdo( DEVICE_OBJECT *device, struct hid_device_desc *desc )
{
    static ULONG index;

    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    struct phys_device *impl;
    UNICODE_STRING name_str;
    DEVICE_OBJECT *child;
    WCHAR name[MAX_PATH];
    NTSTATUS status;

    if (winetest_debug > 1) trace( "polled %u, report_id %u\n", desc->is_polled, desc->use_report_id );

    swprintf( name, MAX_PATH, L"\\Device\\WINETEST#%p&%p&%u", device->DriverObject, device, index++ );
    RtlInitUnicodeString( &name_str, name );

    if ((status = IoCreateDevice( device->DriverObject, sizeof(struct phys_device), &name_str, 0, 0, FALSE, &child )))
    {
        ok( 0, "Failed to create gamepad device, status %#lx\n", status );
        return status;
    }

    impl = pdo_from_DEVICE_OBJECT( child );
    KeInitializeSpinLock( &impl->base.lock );
    swprintf( impl->device_id, MAX_PATH, L"WINETEST\\VID_%04X&PID_%04X", desc->attributes.VendorID,
              desc->attributes.ProductID );
    /* use a different device ID so that driver cache select the polled driver */
    if (desc->is_polled) wcscat( impl->device_id, L"&POLL" );
    swprintf( impl->instance_id, MAX_PATH, L"0&0000&0" );
    impl->base.is_phys = TRUE;
    impl->fdo = fdo;

    impl->use_report_id = desc->use_report_id;
    impl->caps = desc->caps;
    impl->attributes = desc->attributes;
    impl->report_descriptor_len = desc->report_descriptor_len;
    memcpy( impl->report_descriptor_buf, desc->report_descriptor_buf, desc->report_descriptor_len );
    input_queue_init( &impl->input_queue, desc->is_polled );
    input_queue_reset( &impl->input_queue, desc->input, desc->input_size );
    expect_queue_init( &impl->expect_queue );
    expect_queue_reset( &impl->expect_queue, desc->expect, desc->expect_size );
    memcpy( impl->expect_queue.context, desc->context, desc->context_size );

    if (winetest_debug > 1) trace( "Created Bus PDO %p for Bus FDO %p\n", child, device );

    append_child_device( fdo, child );
    IoInvalidateDeviceRelations( fdo->pdo, BusRelations );
    return STATUS_SUCCESS;
}

static NTSTATUS fdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    struct func_device *impl = fdo_from_DEVICE_OBJECT( device );
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    char relations_buffer[sizeof(impl->devices_buffer)];
    DEVICE_RELATIONS *relations = (void *)relations_buffer;
    ULONG code = stack->MinorFunction;
    PNP_DEVICE_STATE state;
    NTSTATUS status;
    KIRQL irql;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_pnp(code) );

    switch (code)
    {
    case IRP_MN_START_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_REMOVE_DEVICE:
        state = (code == IRP_MN_START_DEVICE || code == IRP_MN_CANCEL_REMOVE_DEVICE) ? 0 : PNP_DEVICE_REMOVED;
        KeAcquireSpinLock( &impl->base.lock, &irql );
        impl->base.state = state;
        if (code == IRP_MN_REMOVE_DEVICE) memcpy( relations, impl->devices, sizeof(relations_buffer) );
        impl->devices->Count = 0;
        KeReleaseSpinLock( &impl->base.lock, irql );
        IoSetDeviceInterfaceState( &impl->control_iface, state != PNP_DEVICE_REMOVED );
        if (code != IRP_MN_REMOVE_DEVICE) status = STATUS_SUCCESS;
        else
        {
            while (relations->Count--) IoDeleteDevice( relations->Objects[relations->Count] );
            IoSkipCurrentIrpStackLocation( irp );
            status = IoCallDriver( impl->pdo, irp );
            IoDetachDevice( impl->pdo );
            RtlFreeUnicodeString( &impl->control_iface );
            IoDeleteDevice( device );
            if (winetest_debug > 1) trace( "Deleted Bus FDO %p from PDO %p, status %#lx\n", impl, impl->pdo, status );
            return status;
        }
        break;
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        KeAcquireSpinLock( &impl->base.lock, &irql );
        irp->IoStatus.Information = impl->base.state;
        KeReleaseSpinLock( &impl->base.lock, irql );
        status = STATUS_SUCCESS;
        break;
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    {
        DEVICE_RELATION_TYPE type = stack->Parameters.QueryDeviceRelations.Type;
        switch (type)
        {
        case BusRelations:
            if (winetest_debug > 1) trace( "fdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS BusRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected BusRelations relations\n" );
            KeAcquireSpinLock( &impl->base.lock, &irql );
            memcpy( relations, impl->devices, sizeof(relations_buffer) );
            KeReleaseSpinLock( &impl->base.lock, irql );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information,
                                                              relations->Count, relations->Objects );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        case RemovalRelations:
            if (winetest_debug > 1) trace( "fdo_pnp IRP_MN_QUERY_DEVICE_RELATIONS RemovalRelations\n" );
            ok( !irp->IoStatus.Information, "got unexpected RemovalRelations relations\n" );
            irp->IoStatus.Information = get_device_relations( device, (void *)irp->IoStatus.Information,
                                                              1, &impl->pdo );
            if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
            else status = STATUS_SUCCESS;
            break;
        default:
            ok( 0, "got unexpected IRP_MN_QUERY_DEVICE_RELATIONS type %#x\n", type );
            status = irp->IoStatus.Status;
            break;
        }
        break;
    }
    case IRP_MN_QUERY_CAPABILITIES:
    {
        DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
        caps->EjectSupported = TRUE;
        caps->Removable = TRUE;
        caps->SilentInstall = TRUE;
        caps->SurpriseRemovalOK = TRUE;
        status = STATUS_SUCCESS;
        break;
    }
    default:
        if (winetest_debug > 1) trace( "fdo_pnp code %#lx %s, not implemented!\n", code, debugstr_pnp(code) );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoSkipCurrentIrpStackLocation( irp );
    return IoCallDriver( impl->pdo, irp );
}

static NTSTATUS WINAPI driver_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_pnp( device, irp );
    return fdo_pnp( device, irp );
}

static NTSTATUS pdo_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    const ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
    const ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct hid_expect expect = {0};
    char context[64];
    NTSTATUS status;
    BOOL removed;
    KIRQL irql;
    LONG index;

    if ((!impl->input_queue.is_polled || code != IOCTL_HID_READ_REPORT) && winetest_debug > 1)
        trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );

    KeAcquireSpinLock( &impl->base.lock, &irql );
    removed = impl->base.state == PNP_DEVICE_REMOVED;
    KeReleaseSpinLock( &impl->base.lock, irql );

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }

    winetest_push_context( "id %d%s", impl->use_report_id, impl->input_queue.is_polled ? " poll" : "" );

    switch (code)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
    {
        HID_DESCRIPTOR *desc = irp->UserBuffer;

        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == sizeof(*desc), "got output size %lu\n", out_size );

        if (out_size == sizeof(*desc))
        {
            ok( !desc->bLength, "got size %u\n", desc->bLength );

            desc->bLength = sizeof(*desc);
            desc->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
            desc->bcdHID = HID_REVISION;
            desc->bCountry = 0;
            desc->bNumDescriptors = 1;
            desc->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
            desc->DescriptorList[0].wReportLength = impl->report_descriptor_len;
            irp->IoStatus.Information = sizeof(*desc);
        }
        status = STATUS_SUCCESS;
        break;
    }

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == impl->report_descriptor_len, "got output size %lu\n", out_size );

        if (out_size == impl->report_descriptor_len)
        {
            memcpy( irp->UserBuffer, impl->report_descriptor_buf, impl->report_descriptor_len );
            irp->IoStatus.Information = impl->report_descriptor_len;
        }
        status = STATUS_SUCCESS;
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == sizeof(impl->attributes), "got output size %lu\n", out_size );

        if (out_size == sizeof(impl->attributes))
        {
            memcpy( irp->UserBuffer, &impl->attributes, sizeof(impl->attributes) );
            irp->IoStatus.Information = sizeof(impl->attributes);
        }
        status = STATUS_SUCCESS;
        break;

    case IOCTL_HID_READ_REPORT:
    {
        ULONG expected_size = impl->caps.InputReportByteLength - (impl->use_report_id ? 0 : 1);
        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == expected_size, "got output size %lu\n", out_size );
        status = input_queue_read( &impl->input_queue, irp );
        irp_queue_complete( &impl->input_queue.completed, FALSE );

        winetest_pop_context();
        return status;
    }

    case IOCTL_HID_WRITE_REPORT:
    {
        HID_XFER_PACKET *packet = irp->UserBuffer;

        ok( in_size == sizeof(*packet), "got input size %lu\n", in_size );
        ok( !out_size, "got output size %lu\n", out_size );
        ok( !!packet->reportBuffer, "got buffer %p\n", packet->reportBuffer );

        expect_queue_next( &impl->expect_queue, code, packet, &index, &expect, TRUE, context, sizeof(context) );
        winetest_push_context( "%s expect[%ld]", context, index );
        ok( code == expect.code, "got %#lx, expected %#lx\n", code, expect.code );
        ok( packet->reportId == expect.report_id || broken( packet->reportId == expect.broken_id ),
            "got id %u\n", packet->reportId );
        ok( packet->reportBufferLen == expect.report_len, "got len %lu\n", packet->reportBufferLen );
        if (!broken( packet->reportId == expect.broken_id )) check_buffer( packet, &expect );
        winetest_pop_context();

        irp->IoStatus.Information = expect.ret_length ? expect.ret_length : expect.report_len;
        status = expect.ret_status;
        if (status == STATUS_PENDING) status = expect_queue_add_pending( &impl->expect_queue, irp );
        break;
    }

    case IOCTL_HID_GET_INPUT_REPORT:
    {
        HID_XFER_PACKET *packet = irp->UserBuffer;

        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == sizeof(*packet), "got output size %lu\n", out_size );
        ok( !!packet->reportBuffer, "got buffer %p\n", packet->reportBuffer );

        expect_queue_next( &impl->expect_queue, code, packet, &index, &expect, FALSE, context, sizeof(context) );
        winetest_push_context( "%s expect[%ld]", context, index );
        ok( code == expect.code, "got %#lx, expected %#lx\n", code, expect.code );
        ok( packet->reportId == expect.report_id, "got id %u\n", packet->reportId );
        ok( packet->reportBufferLen == expect.report_len, "got len %lu\n", packet->reportBufferLen );
        winetest_pop_context();

        irp->IoStatus.Information = expect.ret_length ? expect.ret_length : expect.report_len;
        memcpy( packet->reportBuffer, expect.report_buf, irp->IoStatus.Information );
        status = expect.ret_status;
        if (status == STATUS_PENDING) status = expect_queue_add_pending( &impl->expect_queue, irp );
        break;
    }

    case IOCTL_HID_SET_OUTPUT_REPORT:
    {
        HID_XFER_PACKET *packet = irp->UserBuffer;

        ok( in_size == sizeof(*packet), "got input size %lu\n", in_size );
        ok( !out_size, "got output size %lu\n", out_size );
        ok( !!packet->reportBuffer, "got buffer %p\n", packet->reportBuffer );

        expect_queue_next( &impl->expect_queue, code, packet, &index, &expect, TRUE, context, sizeof(context) );
        winetest_push_context( "%s expect[%ld]", context, index );
        ok( code == expect.code, "got %#lx, expected %#lx\n", code, expect.code );
        ok( packet->reportId == expect.report_id, "got id %u\n", packet->reportId );
        ok( packet->reportBufferLen == expect.report_len, "got len %lu\n", packet->reportBufferLen );
        check_buffer( packet, &expect );
        winetest_pop_context();

        irp->IoStatus.Information = expect.ret_length ? expect.ret_length : expect.report_len;
        status = expect.ret_status;
        if (status == STATUS_PENDING) status = expect_queue_add_pending( &impl->expect_queue, irp );
        break;
    }

    case IOCTL_HID_GET_FEATURE:
    {
        HID_XFER_PACKET *packet = irp->UserBuffer;

        ok( !in_size, "got input size %lu\n", in_size );
        ok( out_size == sizeof(*packet), "got output size %lu\n", out_size );
        ok( !!packet->reportBuffer, "got buffer %p\n", packet->reportBuffer );

        expect_queue_next( &impl->expect_queue, code, packet, &index, &expect, FALSE, context, sizeof(context) );
        winetest_push_context( "%s expect[%ld]", context, index );
        ok( code == expect.code, "got %#lx, expected %#lx\n", code, expect.code );
        ok( packet->reportId == expect.report_id, "got id %u\n", packet->reportId );
        ok( packet->reportBufferLen == expect.report_len, "got len %lu\n", packet->reportBufferLen );
        winetest_pop_context();

        irp->IoStatus.Information = expect.ret_length ? expect.ret_length : expect.report_len;
        memcpy( packet->reportBuffer, expect.report_buf, irp->IoStatus.Information );
        status = expect.ret_status;
        if (status == STATUS_PENDING) status = expect_queue_add_pending( &impl->expect_queue, irp );
        break;
    }

    case IOCTL_HID_SET_FEATURE:
    {
        HID_XFER_PACKET *packet = irp->UserBuffer;

        ok( in_size == sizeof(*packet), "got input size %lu\n", in_size );
        ok( !out_size, "got output size %lu\n", out_size );
        ok( !!packet->reportBuffer, "got buffer %p\n", packet->reportBuffer );

        expect_queue_next( &impl->expect_queue, code, packet, &index, &expect, TRUE, context, sizeof(context) );
        winetest_push_context( "%s expect[%ld]", context, index );
        ok( code == expect.code, "got %#lx, expected %#lx\n", code, expect.code );
        ok( packet->reportId == expect.report_id, "got id %u\n", packet->reportId );
        ok( packet->reportBufferLen == expect.report_len, "got len %lu\n", packet->reportBufferLen );
        check_buffer( packet, &expect );
        winetest_pop_context();

        irp->IoStatus.Information = expect.ret_length ? expect.ret_length : expect.report_len;
        status = expect.ret_status;
        if (status == STATUS_PENDING) status = expect_queue_add_pending( &impl->expect_queue, irp );
        break;
    }

    case IOCTL_HID_GET_STRING:
        memcpy( irp->UserBuffer, L"Wine Test", sizeof(L"Wine Test") );
        irp->IoStatus.Information = sizeof(L"Wine Test");
        status = STATUS_SUCCESS;
        break;

    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        irp->IoStatus.Information = 0;
        status = STATUS_NOT_SUPPORTED;
        break;

    default:
        ok( 0, "unexpected call\n" );
        status = irp->IoStatus.Status;
        break;
    }

    winetest_pop_context();

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
    return status;
}

static NTSTATUS fdo_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );
    ok( 0, "unexpected call\n" );

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS WINAPI driver_internal_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_internal_ioctl( device, irp );
    return fdo_internal_ioctl( device, irp );
}

static NTSTATUS pdo_handle_ioctl( struct phys_device *impl, IRP *irp, ULONG code, void *in_buffer, ULONG in_size )
{
    KIRQL irql;

    switch (code)
    {
    case IOCTL_WINETEST_HID_SET_EXPECT:
        if (in_size > EXPECT_QUEUE_BUFFER_SIZE) return STATUS_BUFFER_OVERFLOW;
        expect_queue_reset( &impl->expect_queue, in_buffer, in_size );
        return STATUS_SUCCESS;
    case IOCTL_WINETEST_HID_WAIT_EXPECT:
    {
        struct wait_expect_params *wait_params = (struct wait_expect_params *)in_buffer;
        if (in_size < sizeof(*wait_params)) return STATUS_BUFFER_TOO_SMALL;
        if (!wait_params->wait_pending) return expect_queue_wait( &impl->expect_queue, irp );
        else return expect_queue_wait_pending( &impl->expect_queue, irp );
    }
    case IOCTL_WINETEST_HID_SEND_INPUT:
        if (in_size > EXPECT_QUEUE_BUFFER_SIZE) return STATUS_BUFFER_OVERFLOW;
        input_queue_reset( &impl->input_queue, in_buffer, in_size );
        return STATUS_SUCCESS;
    case IOCTL_WINETEST_HID_WAIT_INPUT:
        return input_queue_wait( &impl->input_queue, irp );
    case IOCTL_WINETEST_HID_SET_CONTEXT:
        if (in_size > sizeof(impl->expect_queue.context)) return STATUS_BUFFER_OVERFLOW;
        KeAcquireSpinLock( &impl->expect_queue.base.lock, &irql );
        memcpy( impl->expect_queue.context, in_buffer, in_size );
        KeReleaseSpinLock( &impl->expect_queue.base.lock, irql );
        return STATUS_SUCCESS;
    case IOCTL_WINETEST_REMOVE_DEVICE:
        KeAcquireSpinLock( &impl->base.lock, &irql );
        impl->base.state = PNP_DEVICE_REMOVED;
        irp_queue_complete( &impl->input_queue.pending, TRUE );
        irp_queue_complete( &impl->input_queue.completed, TRUE );
        KeReleaseSpinLock( &impl->base.lock, irql );
        impl->pending_remove = irp;
        IoMarkIrpPending( irp );
        return STATUS_PENDING;
    case IOCTL_WINETEST_CREATE_DEVICE:
        ok( 0, "unexpected call\n" );
        return irp->IoStatus.Status;
    default:
        ok( 0, "unexpected call\n" );
        return irp->IoStatus.Status;
    }
}

static NTSTATUS pdo_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    struct phys_device *impl = pdo_from_DEVICE_OBJECT( device );
    ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    struct hid_device_desc *desc = irp->AssociatedIrp.SystemBuffer;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );

    status = pdo_handle_ioctl( impl, irp, code, desc + 1, in_size - sizeof(*desc) );

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest( irp, IO_NO_INCREMENT );
    }
    return status;
}

static NTSTATUS fdo_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation( irp );
    ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    struct hid_device_desc *desc = irp->AssociatedIrp.SystemBuffer;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct func_device *impl = fdo_from_DEVICE_OBJECT( device );
    struct phys_device *pdo;
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl(code) );

    switch (code)
    {
    case IOCTL_WINETEST_CREATE_DEVICE:
        if (in_size < sizeof(*desc)) status = STATUS_INVALID_PARAMETER;
        else status = create_child_pdo( device, desc );
        break;
    case IOCTL_WINETEST_REMOVE_DEVICE:
        if (in_size < sizeof(*desc))
            status = STATUS_INVALID_PARAMETER;
        else if (!(device = find_child_device( impl, desc )) || remove_child_device( impl, device ))
            status = STATUS_NO_SUCH_DEVICE;
        else
        {
            status = pdo_ioctl( device, irp );
            IoInvalidateDeviceRelations( impl->pdo, BusRelations );
            return status;
        }
        break;
    case IOCTL_WINETEST_HID_SET_EXPECT:
    case IOCTL_WINETEST_HID_WAIT_EXPECT:
    case IOCTL_WINETEST_HID_SEND_INPUT:
    case IOCTL_WINETEST_HID_SET_CONTEXT:
    case IOCTL_WINETEST_HID_WAIT_INPUT:
        if (in_size < sizeof(*desc))
            status = STATUS_INVALID_PARAMETER;
        else if (!(device = find_child_device( impl, desc )) || !(pdo = pdo_from_DEVICE_OBJECT( device )))
            status = STATUS_NO_SUCH_DEVICE;
        else
            status = pdo_handle_ioctl( pdo, irp, code, desc + 1, in_size - sizeof(*desc) );
        break;
    default:
        ok( 0, "unexpected call\n" );
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI driver_ioctl( DEVICE_OBJECT *device, IRP *irp )
{
    struct device *impl = impl_from_DEVICE_OBJECT( device );
    if (impl->is_phys) return pdo_ioctl( device, irp );
    return fdo_ioctl( device, irp );
}

static NTSTATUS WINAPI driver_add_device( DRIVER_OBJECT *driver, DEVICE_OBJECT *device )
{
    struct func_device *impl;
    DEVICE_OBJECT *child;
    NTSTATUS status;

    if (winetest_debug > 1) trace( "%s: driver %p, device %p\n", __func__, driver, device );

    if ((status = IoCreateDevice( driver, sizeof(struct func_device), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &child )))
    {
        ok( 0, "Failed to create FDO, status %#lx.\n", status );
        return status;
    }

    impl = (struct func_device *)child->DeviceExtension;
    impl->devices = (void *)impl->devices_buffer;
    KeInitializeSpinLock( &impl->base.lock );
    impl->pdo = device;

    status = IoRegisterDeviceInterface( impl->pdo, &control_class, NULL, &impl->control_iface );
    ok( !status, "IoRegisterDeviceInterface returned %#lx\n", status );

    if (winetest_debug > 1) trace( "Created Bus FDO %p for PDO %p\n", child, device );

    IoAttachDeviceToDeviceStack( child, device );
    child->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p, irp %p\n", __func__, device, irp );

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close( DEVICE_OBJECT *device, IRP *irp )
{
    if (winetest_debug > 1) trace( "%s: device %p, irp %p\n", __func__, device, irp );

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload( DRIVER_OBJECT *driver )
{
    if (winetest_debug > 1) trace( "%s: driver %p\n", __func__, driver );
    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *registry )
{
    NTSTATUS status;

    if ((status = winetest_init())) return status;
    if (winetest_debug > 1) trace( "%s: driver %p\n", __func__, driver );

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    return STATUS_SUCCESS;
}
