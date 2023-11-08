/*
 * USB root device enumerator using libusb
 *
 * Copyright 2020 Zebediah Figura
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
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/usb.h"
#include "ddk/usbioctl.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineusb);

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

#define DECLARE_CRITICAL_SECTION(cs) \
    static CRITICAL_SECTION cs; \
    static CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 };

DECLARE_CRITICAL_SECTION(wineusb_cs);

static struct list device_list = LIST_INIT(device_list);

struct usb_device
{
    struct list entry;
    BOOL removed;

    DEVICE_OBJECT *device_obj;

    bool interface;
    int16_t interface_index;

    uint8_t class, subclass, protocol, busnum, portnum;

    uint16_t vendor, product, revision, usbver;

    struct unix_device *unix_device;

    LIST_ENTRY irp_list;
};

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static void destroy_unix_device(struct unix_device *unix_device)
{
    struct usb_destroy_device_params params =
    {
        .device = unix_device,
    };

    WINE_UNIX_CALL(unix_usb_destroy_device, &params);
}

static void add_unix_device(const struct usb_add_device_event *event)
{
    static unsigned int name_index;
    struct usb_device *device;
    DEVICE_OBJECT *device_obj;
    UNICODE_STRING string;
    NTSTATUS status;
    WCHAR name[26];

    TRACE("Adding new device %p, vendor %04x, product %04x.\n", event->device,
            event->vendor, event->product);

    swprintf(name, ARRAY_SIZE(name), L"\\Device\\USBPDO-%u", name_index++);
    RtlInitUnicodeString(&string, name);
    if ((status = IoCreateDevice(driver_obj, sizeof(*device), &string,
            FILE_DEVICE_USB, 0, FALSE, &device_obj)))
    {
        ERR("Failed to create device, status %#lx.\n", status);
        return;
    }

    device = device_obj->DeviceExtension;
    device->device_obj = device_obj;
    device->unix_device = event->device;
    InitializeListHead(&device->irp_list);
    device->removed = FALSE;

    device->interface = event->interface;
    device->interface_index = event->interface_index;

    device->class = event->class;
    device->subclass = event->subclass;
    device->protocol = event->protocol;
    device->busnum = event->busnum;
    device->portnum = event->portnum;

    device->vendor = event->vendor;
    device->product = event->product;
    device->revision = event->revision;
    device->usbver = event->usbver;

    EnterCriticalSection(&wineusb_cs);
    list_add_tail(&device_list, &device->entry);
    LeaveCriticalSection(&wineusb_cs);

    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
}

static void remove_unix_device(struct unix_device *unix_device)
{
    struct usb_device *device;

    TRACE("Removing device %p.\n", unix_device);

    EnterCriticalSection(&wineusb_cs);
    LIST_FOR_EACH_ENTRY(device, &device_list, struct usb_device, entry)
    {
        if (device->unix_device == unix_device)
        {
            if (!device->removed)
            {
                device->removed = TRUE;
                list_remove(&device->entry);
            }
            break;
        }
    }
    LeaveCriticalSection(&wineusb_cs);

    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
}

static HANDLE event_thread;

static void complete_irp(IRP *irp)
{
    EnterCriticalSection(&wineusb_cs);
    RemoveEntryList(&irp->Tail.Overlay.ListEntry);
    LeaveCriticalSection(&wineusb_cs);

    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

static DWORD CALLBACK event_thread_proc(void *arg)
{
    struct usb_event event;
    struct usb_main_loop_params params =
    {
        .event = &event,
    };

    TRACE("Starting event thread.\n");

    if (WINE_UNIX_CALL(unix_usb_init, NULL) != STATUS_SUCCESS)
        return 0;

    while (WINE_UNIX_CALL(unix_usb_main_loop, &params) == STATUS_PENDING)
    {
        switch (event.type)
        {
            case USB_EVENT_ADD_DEVICE:
                add_unix_device(&event.u.added_device);
                break;

            case USB_EVENT_REMOVE_DEVICE:
                remove_unix_device(event.u.removed_device);
                break;

            case USB_EVENT_TRANSFER_COMPLETE:
                complete_irp(event.u.completed_irp);
                break;
        }
    }

    TRACE("Shutting down event thread.\n");
    return 0;
}

static NTSTATUS fdo_pnp(IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret;

    TRACE("irp %p, minor function %#x.\n", irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            struct usb_device *device;
            DEVICE_RELATIONS *devices;
            unsigned int i = 0;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                FIXME("Unhandled device relations type %#x.\n", stack->Parameters.QueryDeviceRelations.Type);
                break;
            }

            EnterCriticalSection(&wineusb_cs);

            if (!(devices = ExAllocatePool(PagedPool,
                    offsetof(DEVICE_RELATIONS, Objects[list_count(&device_list)]))))
            {
                LeaveCriticalSection(&wineusb_cs);
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                break;
            }

            LIST_FOR_EACH_ENTRY(device, &device_list, struct usb_device, entry)
            {
                devices->Objects[i++] = device->device_obj;
                call_fastcall_func1(ObfReferenceObject, device->device_obj);
            }

            LeaveCriticalSection(&wineusb_cs);

            devices->Count = i;
            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
            event_thread = CreateThread(NULL, 0, event_thread_proc, NULL, 0, NULL);

            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
        {
            struct usb_device *device, *cursor;

            WINE_UNIX_CALL(unix_usb_exit, NULL);
            WaitForSingleObject(event_thread, INFINITE);
            CloseHandle(event_thread);

            EnterCriticalSection(&wineusb_cs);
            /* Normally we unlink all devices either:
             *
             * - as a result of hot-unplug, which unlinks the device, and causes
             *   a subsequent IRP_MN_REMOVE_DEVICE which will free it;
             *
             * - if the parent is deleted (at shutdown time), in which case
             *   ntoskrnl will send us IRP_MN_SURPRISE_REMOVAL and
             *   IRP_MN_REMOVE_DEVICE unprompted.
             *
             * But we can get devices hotplugged between when shutdown starts
             * and now, in which case they'll be stuck in this list and never
             * freed.
             *
             * FIXME: This is still broken, though. If a device is hotplugged
             * and then removed, it'll be unlinked and never freed. */
            LIST_FOR_EACH_ENTRY_SAFE(device, cursor, &device_list, struct usb_device, entry)
            {
                assert(!device->removed);
                destroy_unix_device(device->unix_device);
                list_remove(&device->entry);
                IoDeleteDevice(device->device_obj);
            }
            LeaveCriticalSection(&wineusb_cs);

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            ret = IoCallDriver(bus_pdo, irp);
            IoDetachDevice(bus_pdo);
            IoDeleteDevice(bus_fdo);
            return ret;
        }

        default:
            FIXME("Unhandled minor function %#x.\n", stack->MinorFunction);
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

struct string_buffer
{
    WCHAR *string;
    size_t len;
};

static void WINAPIV append_id(struct string_buffer *buffer, const WCHAR *format, ...)
{
    va_list args;
    WCHAR *string;
    int len;

    va_start(args, format);

    len = _vsnwprintf(NULL, 0, format, args) + 1;
    if (!(string = ExAllocatePool(PagedPool, (buffer->len + len) * sizeof(WCHAR))))
    {
        if (buffer->string)
            ExFreePool(buffer->string);
        buffer->string = NULL;
        return;
    }
    if (buffer->string)
    {
        memcpy(string, buffer->string, buffer->len * sizeof(WCHAR));
        ExFreePool(buffer->string);
    }
    _vsnwprintf(string + buffer->len, len, format, args);
    buffer->string = string;
    buffer->len += len;

    va_end(args);
}

static void get_device_id(const struct usb_device *device, struct string_buffer *buffer)
{
    if (device->interface)
        append_id(buffer, L"USB\\VID_%04X&PID_%04X&MI_%02X",
                device->vendor, device->product, device->interface_index);
    else
        append_id(buffer, L"USB\\VID_%04X&PID_%04X", device->vendor, device->product);
}

static void get_instance_id(const struct usb_device *device, struct string_buffer *buffer)
{
    append_id(buffer, L"%u&%u&%u&%u", device->usbver, device->revision, device->busnum, device->portnum);
}

static void get_hardware_ids(const struct usb_device *device, struct string_buffer *buffer)
{
    if (device->interface)
        append_id(buffer, L"USB\\VID_%04X&PID_%04X&REV_%04X&MI_%02X",
                device->vendor, device->product, device->revision, device->interface_index);
    else
        append_id(buffer, L"USB\\VID_%04X&PID_%04X&REV_%04X",
                device->vendor, device->product, device->revision);

    get_device_id(device, buffer);
    append_id(buffer, L"");
}

static void get_compatible_ids(const struct usb_device *device, struct string_buffer *buffer)
{
    if (device->interface_index != -1)
    {
        append_id(buffer, L"USB\\Class_%02x&SubClass_%02x&Prot_%02x",
                device->class, device->subclass, device->protocol);
        append_id(buffer, L"USB\\Class_%02x&SubClass_%02x", device->class, device->subclass);
        append_id(buffer, L"USB\\Class_%02x", device->class);
    }
    else
    {
        append_id(buffer, L"USB\\DevClass_%02x&SubClass_%02x&Prot_%02x",
                device->class, device->subclass, device->protocol);
        append_id(buffer, L"USB\\DevClass_%02x&SubClass_%02x", device->class, device->subclass);
        append_id(buffer, L"USB\\DevClass_%02x", device->class);
    }
    append_id(buffer, L"");
}

static NTSTATUS query_id(struct usb_device *device, IRP *irp, BUS_QUERY_ID_TYPE type)
{
    struct string_buffer buffer = {0};

    TRACE("type %#x.\n", type);

    switch (type)
    {
        case BusQueryDeviceID:
            get_device_id(device, &buffer);
            break;

        case BusQueryInstanceID:
            get_instance_id(device, &buffer);
            break;

        case BusQueryHardwareIDs:
            get_hardware_ids(device, &buffer);
            break;

        case BusQueryCompatibleIDs:
            get_compatible_ids(device, &buffer);
            break;

        default:
            FIXME("Unhandled ID query type %#x.\n", type);
            return irp->IoStatus.Status;
    }

    if (!buffer.string)
        return STATUS_NO_MEMORY;

    irp->IoStatus.Information = (ULONG_PTR)buffer.string;
    return STATUS_SUCCESS;
}

static void remove_pending_irps(struct usb_device *device)
{
    LIST_ENTRY *entry;
    IRP *irp;

    while ((entry = RemoveHeadList(&device->irp_list)) != &device->irp_list)
    {
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device_obj, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct usb_device *device = device_obj->DeviceExtension;
    NTSTATUS ret = irp->IoStatus.Status;

    TRACE("device_obj %p, irp %p, minor function %#x.\n", device_obj, irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
            ret = query_id(device, irp, stack->Parameters.QueryId.IdType);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;

            caps->RawDeviceOK = 1;

            ret = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            EnterCriticalSection(&wineusb_cs);
            remove_pending_irps(device);
            if (!device->removed)
            {
                device->removed = TRUE;
                list_remove(&device->entry);
            }
            LeaveCriticalSection(&wineusb_cs);
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            assert(device->removed);
            remove_pending_irps(device);

            destroy_unix_device(device->unix_device);

            IoDeleteDevice(device->device_obj);
            ret = STATUS_SUCCESS;
            break;

        default:
            FIXME("Unhandled minor function %#x.\n", stack->MinorFunction);
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    if (device == bus_fdo)
        return fdo_pnp(irp);
    return pdo_pnp(device, irp);
}

static NTSTATUS usb_submit_urb(struct usb_device *device, IRP *irp)
{
    URB *urb = IoGetCurrentIrpStackLocation(irp)->Parameters.Others.Argument1;
    NTSTATUS status;

    TRACE("type %#x.\n", urb->UrbHeader.Function);

    switch (urb->UrbHeader.Function)
    {
        case URB_FUNCTION_ABORT_PIPE:
        {
            LIST_ENTRY *entry, *mark;

            /* The documentation states that URB_FUNCTION_ABORT_PIPE may
             * complete before outstanding requests complete, so we don't need
             * to wait for them. */
            EnterCriticalSection(&wineusb_cs);
            mark = &device->irp_list;
            for (entry = mark->Flink; entry != mark; entry = entry->Flink)
            {
                IRP *queued_irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.ListEntry);
                struct usb_cancel_transfer_params params =
                {
                    .transfer = queued_irp->Tail.Overlay.DriverContext[0],
                };

                WINE_UNIX_CALL(unix_usb_cancel_transfer, &params);
            }
            LeaveCriticalSection(&wineusb_cs);

            return STATUS_SUCCESS;
        }

        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        case URB_FUNCTION_SELECT_CONFIGURATION:
        case URB_FUNCTION_VENDOR_DEVICE:
        case URB_FUNCTION_VENDOR_INTERFACE:
        {
            struct usb_submit_urb_params params =
            {
                .device = device->unix_device,
                .irp = irp,
            };

            switch (urb->UrbHeader.Function)
            {
                case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                {
                    struct _URB_BULK_OR_INTERRUPT_TRANSFER *req = &urb->UrbBulkOrInterruptTransfer;
                    if (req->TransferBufferMDL)
                        params.transfer_buffer = MmGetSystemAddressForMdlSafe(req->TransferBufferMDL, NormalPagePriority);
                    else
                        params.transfer_buffer = req->TransferBuffer;
                    break;
                }

                case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
                {
                    struct _URB_CONTROL_DESCRIPTOR_REQUEST *req = &urb->UrbControlDescriptorRequest;
                    if (req->TransferBufferMDL)
                        params.transfer_buffer = MmGetSystemAddressForMdlSafe(req->TransferBufferMDL, NormalPagePriority);
                    else
                        params.transfer_buffer = req->TransferBuffer;
                    break;
                }

                case URB_FUNCTION_VENDOR_DEVICE:
                case URB_FUNCTION_VENDOR_INTERFACE:
                {
                    struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *req = &urb->UrbControlVendorClassRequest;
                    if (req->TransferBufferMDL)
                        params.transfer_buffer = MmGetSystemAddressForMdlSafe(req->TransferBufferMDL, NormalPagePriority);
                    else
                        params.transfer_buffer = req->TransferBuffer;
                    break;
                }
            }

            /* Hold the wineusb lock while submitting and queuing, and
             * similarly hold it in complete_irp(). That way, if libusb reports
             * completion between submitting and queuing, we won't try to
             * dequeue the IRP until it's actually been queued. */
            EnterCriticalSection(&wineusb_cs);
            status = WINE_UNIX_CALL(unix_usb_submit_urb, &params);
            if (status == STATUS_PENDING)
            {
                IoMarkIrpPending(irp);
                InsertTailList(&device->irp_list, &irp->Tail.Overlay.ListEntry);
            }
            LeaveCriticalSection(&wineusb_cs);

            return status;
        }

        default:
            FIXME("Unhandled function %#x.\n", urb->UrbHeader.Function);
    }

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WINAPI driver_internal_ioctl(DEVICE_OBJECT *device_obj, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct usb_device *device = device_obj->DeviceExtension;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    BOOL removed;

    TRACE("device_obj %p, irp %p, code %#lx.\n", device_obj, irp, code);

    EnterCriticalSection(&wineusb_cs);
    removed = device->removed;
    LeaveCriticalSection(&wineusb_cs);

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    switch (code)
    {
        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            status = usb_submit_urb(device, irp);
            break;

        default:
            FIXME("Unhandled ioctl %#lx (device %#lx, access %#lx, function %#lx, method %#lx).\n",
                    code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
    }

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    return status;
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo)
{
    NTSTATUS ret;

    TRACE("driver %p, pdo %p.\n", driver, pdo);

    if ((ret = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo)))
    {
        ERR("Failed to create FDO, status %#lx.\n", ret);
        return ret;
    }

    IoAttachDeviceToDeviceStack(bus_fdo, pdo);
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    NTSTATUS status;

    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    if ((status = __wine_init_unix_call()))
    {
        ERR("Failed to initialize Unix library, status %#lx.\n", status);
        return status;
    }

    driver_obj = driver;

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;

    return STATUS_SUCCESS;
}
