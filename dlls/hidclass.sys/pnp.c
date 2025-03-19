/*
 * Human Interface Device class driver
 *
 * Copyright 2015 Aric Stewart
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

#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include "initguid.h"
#include "hid.h"
#include "devguid.h"
#include "ntddmou.h"
#include "ntddkbd.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"
#include "regstr.h"
#include "ntuser.h"
#include "wine/debug.h"
#include "wine/asm.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

DEFINE_GUID(GUID_DEVINTERFACE_WINEXINPUT, 0x6c53d5fd, 0x6480, 0x440f, 0xb6, 0x18, 0x47, 0x67, 0x50, 0xc5, 0xe1, 0xa6);

#ifdef __ASM_USE_FASTCALL_WRAPPER

extern void * WINAPI wrap_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax" );

#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)

#else

#define call_fastcall_func1(func,a) func(a)

#endif

static struct list minidriver_list = LIST_INIT(minidriver_list);

static minidriver *find_minidriver(DRIVER_OBJECT *driver)
{
    minidriver *md;
    LIST_FOR_EACH_ENTRY(md, &minidriver_list, minidriver, entry)
    {
        if (md->minidriver.DriverObject == driver)
            return md;
    }
    return NULL;
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR *id)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, &event, &irp_status);
    if (irp == NULL)
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!irp_status.Status)
    {
        wcscpy(id, (WCHAR *)irp_status.Information);
        ExFreePool((WCHAR *)irp_status.Information);
    }

    return irp_status.Status;
}

/* user32 reserves 1 & 2 for winemouse and winekeyboard,
 * keep this in sync with user_private.h */
#define WINE_MOUSE_HANDLE 1
#define WINE_KEYBOARD_HANDLE 2

static UINT32 alloc_rawinput_handle(void)
{
    static LONG counter = WINE_KEYBOARD_HANDLE + 1;
    return InterlockedIncrement(&counter);
}

/* make sure bRawData can hold UsagePage and Usage without requiring additional allocation */
C_ASSERT(offsetof(RAWINPUT, data.hid.bRawData[2 * sizeof(USAGE)]) < sizeof(RAWINPUT));

static void send_wm_input_device_change( struct phys_device *pdo, LPARAM param )
{
    HIDP_COLLECTION_DESC *desc = pdo->collection_desc;
    INPUT input = {.type = INPUT_HARDWARE};
    struct hid_packet hid = {0};

    TRACE( "pdo %p, lparam %p\n", pdo, (void *)param );

    if (!IsEqualGUID( pdo->base.class_guid, &GUID_DEVINTERFACE_HID )) return;

    input.hi.uMsg = WM_INPUT_DEVICE_CHANGE;
    input.hi.wParamH = HIWORD(param);
    input.hi.wParamL = LOWORD(param);

    hid.head.device = pdo->rawinput_handle;
    hid.head.usage = MAKELONG(desc->Usage, desc->UsagePage);
    NtUserSendHardwareInput(0, 0, &input, (LPARAM)&hid);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *bus_pdo)
{
    WCHAR device_id[MAX_DEVICE_ID_LEN], instance_id[MAX_DEVICE_ID_LEN];
    struct func_device *fdo;
    BOOL is_xinput_class;
    DEVICE_OBJECT *device;
    NTSTATUS status;
    minidriver *minidriver;

    if ((status = get_device_id(bus_pdo, BusQueryDeviceID, device_id)))
    {
        ERR( "Failed to get PDO device id, status %#lx.\n", status );
        return status;
    }

    if ((status = get_device_id(bus_pdo, BusQueryInstanceID, instance_id)))
    {
        ERR( "Failed to get PDO instance id, status %#lx.\n", status );
        return status;
    }

    TRACE("Adding device to PDO %p, id %s\\%s.\n", bus_pdo, debugstr_w(device_id), debugstr_w(instance_id));
    minidriver = find_minidriver(driver);

    if ((status = IoCreateDevice( driver, sizeof(*fdo) + minidriver->minidriver.DeviceExtensionSize,
                                  NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &device )))
    {
        ERR( "Failed to create bus FDO, status %#lx.\n", status );
        return status;
    }
    fdo = device->DeviceExtension;
    fdo->base.is_fdo = TRUE;
    fdo->base.hid.MiniDeviceExtension = fdo + 1;
    fdo->base.hid.PhysicalDeviceObject = bus_pdo;
    fdo->base.hid.NextDeviceObject = bus_pdo;
    swprintf( fdo->base.device_id, ARRAY_SIZE(fdo->base.device_id), L"HID\\%s", wcsrchr( device_id, '\\' ) + 1 );
    wcscpy( fdo->base.instance_id, instance_id );

    if (get_device_id( bus_pdo, BusQueryContainerID, fdo->base.container_id ))
        fdo->base.container_id[0] = 0;

    is_xinput_class = !wcsncmp(device_id, L"WINEXINPUT\\", 7) && wcsstr(device_id, L"&XI_") != NULL;
    if (is_xinput_class) fdo->base.class_guid = &GUID_DEVINTERFACE_WINEXINPUT;
    else fdo->base.class_guid = &GUID_DEVINTERFACE_HID;

    status = minidriver->AddDevice( minidriver->minidriver.DriverObject, device );
    if (status != STATUS_SUCCESS)
    {
        ERR( "Minidriver AddDevice failed (%lx)\n", status );
        IoDeleteDevice( device );
        return status;
    }

    IoAttachDeviceToDeviceStack( device, bus_pdo );
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static NTSTATUS get_hid_device_desc( minidriver *minidriver, DEVICE_OBJECT *device, HIDP_DEVICE_DESC *desc )
{
    HID_DESCRIPTOR descriptor = {0};
    IO_STATUS_BLOCK io;
    BYTE *report_desc;
    INT i;

    call_minidriver( IOCTL_HID_GET_DEVICE_DESCRIPTOR, device, NULL, 0, &descriptor, sizeof(descriptor), &io );
    if (io.Status) return io.Status;

    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;
    if (i >= descriptor.bNumDescriptors) return STATUS_NOT_FOUND;

    if (!(report_desc = malloc( descriptor.DescriptorList[i].wReportLength ))) return STATUS_NO_MEMORY;
    call_minidriver( IOCTL_HID_GET_REPORT_DESCRIPTOR, device, NULL, 0, report_desc,
                     descriptor.DescriptorList[i].wReportLength, &io );
    if (!io.Status) io.Status = HidP_GetCollectionDescription( report_desc, descriptor.DescriptorList[i].wReportLength,
                                                               PagedPool, desc );
    free( report_desc );

    if (io.Status && io.Status != HIDP_STATUS_SUCCESS) return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;
}

static NTSTATUS initialize_device( minidriver *minidriver, DEVICE_OBJECT *device )
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    ULONG index = HID_STRING_ID_ISERIALNUMBER;
    IO_STATUS_BLOCK io;
    NTSTATUS status;

    call_minidriver( IOCTL_HID_GET_DEVICE_ATTRIBUTES, device, NULL, 0, &fdo->attrs,
                     sizeof(fdo->attrs), &io );
    if (io.Status != STATUS_SUCCESS)
    {
        ERR( "Minidriver failed to get attributes, status %#lx.\n", io.Status );
        return io.Status;
    }

    call_minidriver( IOCTL_HID_GET_STRING, device, ULongToPtr(index), sizeof(index),
                     &fdo->serial, sizeof(fdo->serial), &io );
    if (io.Status != STATUS_SUCCESS)
    {
        ERR( "Minidriver failed to get serial number, status %#lx.\n", io.Status );
        return io.Status;
    }

    if ((status = get_hid_device_desc( minidriver, device, &fdo->device_desc )))
    {
        ERR( "Failed to get HID device description, status %#lx\n", status );
        return status;
    }

    if (!(fdo->child_pdos = malloc( fdo->device_desc.CollectionDescLength * sizeof(*fdo->child_pdos) )))
    {
        ERR( "Cannot allocate child PDOs array\n" );
        return STATUS_NO_MEMORY;
    }

    fdo->poll_interval = minidriver->minidriver.DevicesArePolled ? DEFAULT_POLL_INTERVAL : 0;
    KeInitializeEvent( &fdo->halt_event, NotificationEvent, FALSE );
    return STATUS_SUCCESS;
}

static NTSTATUS create_child_pdos( minidriver *minidriver, DEVICE_OBJECT *device )
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    DEVICE_OBJECT *child_device;
    struct phys_device *pdo;
    UNICODE_STRING string;
    WCHAR pdo_name[255];
    USAGE page, usage;
    NTSTATUS status;
    INT i;

    for (i = 0; i < fdo->device_desc.CollectionDescLength; ++i)
    {
        if (fdo->device_desc.CollectionDescLength > 1)
            swprintf( pdo_name, ARRAY_SIZE(pdo_name), L"\\Device\\HID#%p&%p&%d", device->DriverObject,
                      fdo->base.hid.PhysicalDeviceObject, i );
        else
            swprintf( pdo_name, ARRAY_SIZE(pdo_name), L"\\Device\\HID#%p&%p", device->DriverObject,
                      fdo->base.hid.PhysicalDeviceObject );

        RtlInitUnicodeString(&string, pdo_name);
        if ((status = IoCreateDevice( device->DriverObject, sizeof(*pdo), &string, 0, 0, FALSE, &child_device )))
        {
            ERR( "Failed to create child PDO, status %#lx.\n", status );
            return status;
        }

        fdo->child_pdos[i] = child_device;
        fdo->child_count++;

        pdo = pdo_from_DEVICE_OBJECT( child_device );
        pdo->base.hid = fdo->base.hid;
        pdo->parent_fdo = device;
        list_init( &pdo->queues );
        KeInitializeSpinLock( &pdo->lock );

        pdo->collection_desc = fdo->device_desc.CollectionDesc + i;

        if (fdo->device_desc.CollectionDescLength > 1)
        {
            swprintf( pdo->base.device_id, ARRAY_SIZE(pdo->base.device_id), L"%s&Col%02d",
                      fdo->base.device_id, pdo->collection_desc->CollectionNumber );
            swprintf( pdo->base.instance_id, ARRAY_SIZE(pdo->base.instance_id), L"%u&%s&%x&%u&%04u",
                      fdo->attrs.VersionNumber, fdo->serial, 0, 0, i );
        }
        else
        {
            wcscpy( pdo->base.device_id, fdo->base.device_id );
            wcscpy( pdo->base.instance_id, fdo->base.instance_id );
        }
        wcscpy( pdo->base.container_id, fdo->base.container_id );
        pdo->base.class_guid = fdo->base.class_guid;

        pdo->information.VendorID = fdo->attrs.VendorID;
        pdo->information.ProductID = fdo->attrs.ProductID;
        pdo->information.VersionNumber = fdo->attrs.VersionNumber;
        pdo->information.Polled = minidriver->minidriver.DevicesArePolled;
        pdo->information.DescriptorSize = pdo->collection_desc->PreparsedDataLength;

        page = pdo->collection_desc->UsagePage;
        usage = pdo->collection_desc->Usage;
        if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_MOUSE)
            pdo->rawinput_handle = WINE_MOUSE_HANDLE;
        else if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_KEYBOARD)
            pdo->rawinput_handle = WINE_KEYBOARD_HANDLE;
        else
            pdo->rawinput_handle = alloc_rawinput_handle();

        TRACE( "created pdo %p, rawinput handle %#x\n", pdo, pdo->rawinput_handle );
    }

    IoInvalidateDeviceRelations( fdo->base.hid.PhysicalDeviceObject, BusRelations );
    return STATUS_SUCCESS;
}

static NTSTATUS fdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    minidriver *minidriver = find_minidriver(device->DriverObject);
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct func_device *fdo = fdo_from_DEVICE_OBJECT( device );
    NTSTATUS status;

    TRACE("irp %p, minor function %#x.\n", irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DEVICE_RELATIONS *devices;
            UINT32 i;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
                return minidriver->PNPDispatch(device, irp);

            if (!(devices = ExAllocatePool( PagedPool, offsetof( DEVICE_RELATIONS, Objects[fdo->child_count] ) )))
            {
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return STATUS_NO_MEMORY;
            }

            for (i = 0, devices->Count = 0; i < fdo->child_count; ++i)
            {
                devices->Objects[i] = fdo->child_pdos[i];
                call_fastcall_func1( ObfReferenceObject, fdo->child_pdos[i] );
                devices->Count++;
            }

            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            return IoCallDriver( fdo->base.hid.NextDeviceObject, irp );
        }

        case IRP_MN_START_DEVICE:
            status = minidriver->PNPDispatch( device, irp );
            if (!status) status = initialize_device( minidriver, device );
            if (!status) status = create_child_pdos( minidriver, device );
            if (!status) fdo->thread = CreateThread( NULL, 0, hid_device_thread, device, 0, NULL );
            return status;

        case IRP_MN_REMOVE_DEVICE:
            if (fdo->thread)
            {
                KeSetEvent( &fdo->halt_event, IO_NO_INCREMENT, FALSE );
                WaitForSingleObject( fdo->thread, INFINITE );
            }

            status = minidriver->PNPDispatch( device, irp );
            HidP_FreeCollectionDescription( &fdo->device_desc );
            free( fdo->child_pdos );
            IoDetachDevice( fdo->base.hid.NextDeviceObject );
            IoDeleteDevice( device );
            return status;

        case IRP_MN_SURPRISE_REMOVAL:
            KeSetEvent( &fdo->halt_event, IO_NO_INCREMENT, FALSE );
            return STATUS_SUCCESS;

        default:
            return minidriver->PNPDispatch(device, irp);
    }
}

static WCHAR *query_hardware_ids(DEVICE_OBJECT *device)
{
    static const WCHAR vid_pid_format[] = L"HID\\VID_%04X&PID_%04X";
    static const WCHAR vid_usage_format[] = L"HID\\VID_%04X&UP:%04X_U:%04X";
    static const WCHAR usage_format[] = L"HID_DEVICE_UP:%04X_U:%04X";
    static const WCHAR hid_format[] = L"HID_DEVICE";

    struct phys_device *pdo = pdo_from_DEVICE_OBJECT( device );
    HIDP_COLLECTION_DESC *desc = pdo->collection_desc;
    HID_COLLECTION_INFORMATION *info = &pdo->information;
    WCHAR *dst;
    DWORD size;

    size = sizeof(vid_pid_format);
    size += sizeof(vid_usage_format);
    size += sizeof(usage_format);
    size += sizeof(hid_format);

    if ((dst = ExAllocatePool(PagedPool, size + sizeof(WCHAR))))
    {
        DWORD len = size / sizeof(WCHAR), pos = 0;
        pos += swprintf( dst + pos, len - pos, vid_pid_format, info->VendorID, info->ProductID ) + 1;
        pos += swprintf( dst + pos, len - pos, vid_usage_format, info->VendorID, desc->UsagePage, desc->Usage ) + 1;
        pos += swprintf( dst + pos, len - pos, usage_format, desc->UsagePage, desc->Usage ) + 1;
        pos += swprintf( dst + pos, len - pos, hid_format ) + 1;
        dst[pos] = 0;
    }

    return dst;
}

static WCHAR *query_compatible_ids(DEVICE_OBJECT *device)
{
    WCHAR *dst;
    if ((dst = ExAllocatePool(PagedPool, sizeof(WCHAR)))) dst[0] = 0;
    return dst;
}

static WCHAR *query_device_id(DEVICE_OBJECT *device)
{
    struct device *ext = device->DeviceExtension;
    DWORD size = (wcslen(ext->device_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size)))
        memcpy(dst, ext->device_id, size);

    return dst;
}

static WCHAR *query_instance_id(DEVICE_OBJECT *device)
{
    struct device *ext = device->DeviceExtension;
    DWORD size = (wcslen(ext->instance_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size)))
        memcpy(dst, ext->instance_id, size);

    return dst;
}

static WCHAR *query_container_id(DEVICE_OBJECT *device)
{
    struct device *ext = device->DeviceExtension;
    DWORD size = (wcslen(ext->container_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size)))
        memcpy(dst, ext->container_id, size);

    return dst;
}

static NTSTATUS pdo_pnp( DEVICE_OBJECT *device, IRP *irp )
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    struct phys_device *pdo = pdo_from_DEVICE_OBJECT( device );
    HIDP_COLLECTION_DESC *desc = pdo->collection_desc;
    NTSTATUS status = irp->IoStatus.Status;
    struct hid_queue *queue, *next;
    KIRQL irql;

    TRACE("irp %p, minor function %#x.\n", irp, irpsp->MinorFunction);

    switch(irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
        {
            BUS_QUERY_ID_TYPE type = irpsp->Parameters.QueryId.IdType;
            switch (type)
            {
            case BusQueryHardwareIDs:
                irp->IoStatus.Information = (ULONG_PTR)query_hardware_ids(device);
                if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
                else status = STATUS_SUCCESS;
                break;
            case BusQueryCompatibleIDs:
                irp->IoStatus.Information = (ULONG_PTR)query_compatible_ids(device);
                if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
                else status = STATUS_SUCCESS;
                break;
            case BusQueryDeviceID:
                irp->IoStatus.Information = (ULONG_PTR)query_device_id(device);
                if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
                else status = STATUS_SUCCESS;
                break;
            case BusQueryInstanceID:
                irp->IoStatus.Information = (ULONG_PTR)query_instance_id(device);
                if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
                else status = STATUS_SUCCESS;
                break;
            case BusQueryContainerID:
                if (pdo->base.container_id[0])
                {
                    irp->IoStatus.Information = (ULONG_PTR)query_container_id(device);
                    if (!irp->IoStatus.Information) status = STATUS_NO_MEMORY;
                    else status = STATUS_SUCCESS;
                }
                break;
            default:
                WARN("IRP_MN_QUERY_ID type %u, not implemented!\n", type);
                break;
            }
            break;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = irpsp->Parameters.DeviceCapabilities.Capabilities;

            caps->RawDeviceOK = 1;
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
            send_wm_input_device_change( pdo, GIDC_ARRIVAL );

            if ((status = IoRegisterDeviceInterface( device, pdo->base.class_guid, NULL, &pdo->link_name )))
            {
                ERR( "Failed to register interface, status %#lx.\n", status );
                break;
            }

            /* FIXME: This should probably be done in mouhid.sys. */
            if (desc->UsagePage == HID_USAGE_PAGE_GENERIC && desc->Usage == HID_USAGE_GENERIC_MOUSE)
            {
                if (!IoRegisterDeviceInterface( device, &GUID_DEVINTERFACE_MOUSE, NULL, &pdo->mouse_link_name ))
                    pdo->is_mouse = TRUE;
            }
            if (desc->UsagePage == HID_USAGE_PAGE_GENERIC && desc->Usage == HID_USAGE_GENERIC_KEYBOARD)
            {
                if (!IoRegisterDeviceInterface( device, &GUID_DEVINTERFACE_KEYBOARD, NULL, &pdo->keyboard_link_name ))
                    pdo->is_keyboard = TRUE;
            }

            IoSetDeviceInterfaceState( &pdo->link_name, TRUE );
            if (pdo->is_mouse) IoSetDeviceInterfaceState( &pdo->mouse_link_name, TRUE );
            if (pdo->is_keyboard) IoSetDeviceInterfaceState( &pdo->keyboard_link_name, TRUE );

            pdo->removed = FALSE;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            send_wm_input_device_change( pdo, GIDC_REMOVAL );

            IoSetDeviceInterfaceState( &pdo->link_name, FALSE );
            if (pdo->is_mouse) IoSetDeviceInterfaceState( &pdo->mouse_link_name, FALSE );
            if (pdo->is_keyboard) IoSetDeviceInterfaceState( &pdo->keyboard_link_name, FALSE );

            KeAcquireSpinLock( &pdo->lock, &irql );
            LIST_FOR_EACH_ENTRY_SAFE( queue, next, &pdo->queues, struct hid_queue, entry )
                hid_queue_destroy( queue );
            KeReleaseSpinLock( &pdo->lock, irql );

            RtlFreeUnicodeString( &pdo->link_name );

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            IoDeleteDevice(device);
            return STATUS_SUCCESS;

        case IRP_MN_SURPRISE_REMOVAL:
            KeAcquireSpinLock( &pdo->lock, &irql );
            pdo->removed = TRUE;
            LIST_FOR_EACH_ENTRY_SAFE( queue, next, &pdo->queues, struct hid_queue, entry )
                hid_queue_remove_pending_irps( queue );
            KeReleaseSpinLock( &pdo->lock, irql );

            status = STATUS_SUCCESS;
            break;

        default:
            FIXME("Unhandled minor function %#x.\n", irpsp->MinorFunction);
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    struct device *ext = device->DeviceExtension;

    if (ext->is_fdo)
        return fdo_pnp(device, irp);
    else
        return pdo_pnp(device, irp);
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    struct device *ext = device->DeviceExtension;

    if (ext->is_fdo)
    {
        irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

    return pdo_create(device, irp);
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_close(device, irp);
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_ioctl(device, irp);
}

static NTSTATUS WINAPI driver_read(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_read(device, irp);
}

static NTSTATUS WINAPI driver_write(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_write(device, irp);
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    minidriver *md;

    TRACE("\n");

    if ((md = find_minidriver(driver)))
    {
        if (md->DriverUnload)
            md->DriverUnload(md->minidriver.DriverObject);
        list_remove(&md->entry);
        free(md);
    }
}

NTSTATUS WINAPI HidRegisterMinidriver(HID_MINIDRIVER_REGISTRATION *registration)
{
    minidriver *driver;

    /* make sure we have a window station and a desktop, we need one to send input */
    if (!GetProcessWindowStation())
        return STATUS_INVALID_PARAMETER;

    if (!(driver = calloc(1, sizeof(*driver))))
        return STATUS_NO_MEMORY;

    driver->DriverUnload = registration->DriverObject->DriverUnload;
    registration->DriverObject->DriverUnload = driver_unload;

    registration->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    registration->DriverObject->MajorFunction[IRP_MJ_READ] = driver_read;
    registration->DriverObject->MajorFunction[IRP_MJ_WRITE] = driver_write;
    registration->DriverObject->MajorFunction[IRP_MJ_CREATE] = driver_create;
    registration->DriverObject->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    driver->PNPDispatch = registration->DriverObject->MajorFunction[IRP_MJ_PNP];
    registration->DriverObject->MajorFunction[IRP_MJ_PNP] = driver_pnp;

    driver->AddDevice = registration->DriverObject->DriverExtension->AddDevice;
    registration->DriverObject->DriverExtension->AddDevice = driver_add_device;

    driver->minidriver = *registration;
    list_add_tail(&minidriver_list, &driver->entry);

    return STATUS_SUCCESS;
}

void call_minidriver( ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size,
                      void *out_buff, ULONG out_size, IO_STATUS_BLOCK *io )
{
    IRP *irp;
    KEVENT event;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( code, device, in_buff, in_size, out_buff, out_size, TRUE, &event, io );

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
}
