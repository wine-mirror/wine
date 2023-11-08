/*
 * libusb backend
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libusb.h>
#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/usb.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineusb);

struct unix_device
{
    struct list entry;

    libusb_device_handle *handle;
    struct unix_device *parent;
    unsigned int refcount;
};

static libusb_hotplug_callback_handle hotplug_cb_handle;

static volatile bool thread_shutdown;

static struct usb_event *usb_events;
static size_t usb_event_count, usb_events_capacity;

static pthread_mutex_t device_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct list device_list = LIST_INIT(device_list);

static bool array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    unsigned int new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

static void queue_event(const struct usb_event *event)
{
    if (array_reserve((void **)&usb_events, &usb_events_capacity, usb_event_count + 1, sizeof(*usb_events)))
        usb_events[usb_event_count++] = *event;
    else
        ERR("Failed to queue event.\n");
}

static bool get_event(struct usb_event *event)
{
    if (!usb_event_count) return false;

    *event = usb_events[0];
    if (--usb_event_count)
        memmove(usb_events, usb_events + 1, usb_event_count * sizeof(*usb_events));

    return true;
}

static void add_usb_device(libusb_device *libusb_device)
{
    struct libusb_config_descriptor *config_desc;
    struct libusb_device_descriptor device_desc;
    struct unix_device *unix_device;
    struct usb_event usb_event;
    int ret;

    libusb_get_device_descriptor(libusb_device, &device_desc);

    TRACE("Adding new device %p, vendor %04x, product %04x.\n", libusb_device,
            device_desc.idVendor, device_desc.idProduct);

    if (!(unix_device = calloc(1, sizeof(*unix_device))))
        return;

    if ((ret = libusb_open(libusb_device, &unix_device->handle)))
    {
        WARN("Failed to open device: %s\n", libusb_strerror(ret));
        free(unix_device);
        return;
    }
    unix_device->refcount = 1;

    pthread_mutex_lock(&device_mutex);
    list_add_tail(&device_list, &unix_device->entry);
    pthread_mutex_unlock(&device_mutex);

    usb_event.type = USB_EVENT_ADD_DEVICE;
    usb_event.u.added_device.device = unix_device;
    usb_event.u.added_device.vendor = device_desc.idVendor;
    usb_event.u.added_device.product = device_desc.idProduct;
    usb_event.u.added_device.revision = device_desc.bcdDevice;
    usb_event.u.added_device.usbver = device_desc.bcdUSB;
    usb_event.u.added_device.class = device_desc.bDeviceClass;
    usb_event.u.added_device.subclass = device_desc.bDeviceSubClass;
    usb_event.u.added_device.protocol = device_desc.bDeviceProtocol;
    usb_event.u.added_device.busnum = libusb_get_bus_number(libusb_device);
    usb_event.u.added_device.portnum = libusb_get_port_number(libusb_device);
    usb_event.u.added_device.interface = false;
    usb_event.u.added_device.interface_index = -1;

    if (!(ret = libusb_get_active_config_descriptor(libusb_device, &config_desc)))
    {
        const struct libusb_interface *interface;
        const struct libusb_interface_descriptor *iface_desc;

        if (config_desc->bNumInterfaces == 1)
        {
            interface = &config_desc->interface[0];
            if (interface->num_altsetting != 1)
                FIXME("Interface 0 has %u alternate settings; using the first one.\n",
                        interface->num_altsetting);
            iface_desc = &interface->altsetting[0];

            usb_event.u.added_device.class = iface_desc->bInterfaceClass;
            usb_event.u.added_device.subclass = iface_desc->bInterfaceSubClass;
            usb_event.u.added_device.protocol = iface_desc->bInterfaceProtocol;
            usb_event.u.added_device.interface_index = iface_desc->bInterfaceNumber;
        }
        queue_event(&usb_event);

        /* Create new devices for interfaces of composite devices.
         *
         * On Windows this is the job of usbccgp.sys, a separate driver that
         * layers on top of the base USB driver. While we could take this
         * approach as well, implementing usbccgp is a lot more work, whereas
         * interface support is effectively built into libusb.
         *
         * FIXME: usbccgp does not create separate interfaces in some cases,
         * e.g. when there is an interface association descriptor available.
         */
        if (config_desc->bNumInterfaces > 1)
        {
            uint8_t i;

            for (i = 0; i < config_desc->bNumInterfaces; ++i)
            {
                struct unix_device *unix_iface;

                interface = &config_desc->interface[i];
                if (interface->num_altsetting != 1)
                    FIXME("Interface %u has %u alternate settings; using the first one.\n",
                            i, interface->num_altsetting);
                iface_desc = &interface->altsetting[0];

                if (!(unix_iface = calloc(1, sizeof(*unix_iface))))
                    return;

                ++unix_device->refcount;
                unix_iface->refcount = 1;
                unix_iface->handle = unix_device->handle;
                unix_iface->parent = unix_device;
                pthread_mutex_lock(&device_mutex);
                list_add_tail(&device_list, &unix_iface->entry);
                pthread_mutex_unlock(&device_mutex);

                usb_event.u.added_device.device = unix_iface;
                usb_event.u.added_device.class = iface_desc->bInterfaceClass;
                usb_event.u.added_device.subclass = iface_desc->bInterfaceSubClass;
                usb_event.u.added_device.protocol = iface_desc->bInterfaceProtocol;
                usb_event.u.added_device.interface = true;
                usb_event.u.added_device.interface_index = iface_desc->bInterfaceNumber;
                queue_event(&usb_event);
            }
        }
        libusb_free_config_descriptor(config_desc);
    }
    else
    {
        queue_event(&usb_event);

        ERR("Failed to get configuration descriptor: %s\n", libusb_strerror(ret));
    }
}

static void remove_usb_device(libusb_device *libusb_device)
{
    struct unix_device *unix_device;
    struct usb_event usb_event;

    TRACE("Removing device %p.\n", libusb_device);

    LIST_FOR_EACH_ENTRY(unix_device, &device_list, struct unix_device, entry)
    {
        if (libusb_get_device(unix_device->handle) == libusb_device)
        {
            usb_event.type = USB_EVENT_REMOVE_DEVICE;
            usb_event.u.removed_device = unix_device;
            queue_event(&usb_event);
        }
    }
}

static int LIBUSB_CALL hotplug_cb(libusb_context *context, libusb_device *device,
        libusb_hotplug_event event, void *user_data)
{
    if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED)
        add_usb_device(device);
    else
        remove_usb_device(device);

    return 0;
}

static NTSTATUS usb_main_loop(void *args)
{
    const struct usb_main_loop_params *params = args;
    int ret;

    while (!thread_shutdown)
    {
        if (get_event(params->event)) return STATUS_PENDING;

        if ((ret = libusb_handle_events(NULL)))
            ERR("Error handling events: %s\n", libusb_strerror(ret));
    }

    libusb_exit(NULL);
    free(usb_events);
    usb_events = NULL;
    usb_event_count = usb_events_capacity = 0;
    thread_shutdown = false;

    TRACE("USB main loop exiting.\n");
    return STATUS_SUCCESS;
}

static NTSTATUS usb_init(void *args)
{
    int ret;

    if ((ret = libusb_init(NULL)))
    {
        ERR("Failed to initialize libusb: %s\n", libusb_strerror(ret));
        return STATUS_UNSUCCESSFUL;
    }

    if ((ret = libusb_hotplug_register_callback(NULL,
            LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
            LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
            LIBUSB_HOTPLUG_MATCH_ANY, hotplug_cb, NULL, &hotplug_cb_handle)))
    {
        ERR("Failed to register callback: %s\n", libusb_strerror(ret));
        libusb_exit(NULL);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS usb_exit(void *args)
{
    libusb_hotplug_deregister_callback(NULL, hotplug_cb_handle);
    thread_shutdown = true;
    libusb_interrupt_event_handler(NULL);

    return STATUS_SUCCESS;
}

static NTSTATUS usbd_status_from_libusb(enum libusb_transfer_status status)
{
    switch (status)
    {
        case LIBUSB_TRANSFER_CANCELLED:
            return USBD_STATUS_CANCELED;
        case LIBUSB_TRANSFER_COMPLETED:
            return USBD_STATUS_SUCCESS;
        case LIBUSB_TRANSFER_NO_DEVICE:
            return USBD_STATUS_DEVICE_GONE;
        case LIBUSB_TRANSFER_STALL:
            return USBD_STATUS_ENDPOINT_HALTED;
        case LIBUSB_TRANSFER_TIMED_OUT:
            return USBD_STATUS_TIMEOUT;
        default:
            FIXME("Unhandled status %#x.\n", status);
        case LIBUSB_TRANSFER_ERROR:
            return USBD_STATUS_REQUEST_FAILED;
    }
}

struct transfer_ctx
{
    IRP *irp;
    void *transfer_buffer;
};

static void LIBUSB_CALL transfer_cb(struct libusb_transfer *transfer)
{
    struct transfer_ctx *transfer_ctx = transfer->user_data;
    IRP *irp = transfer_ctx->irp;
    URB *urb = IoGetCurrentIrpStackLocation(irp)->Parameters.Others.Argument1;
    unsigned char *transfer_buffer = transfer_ctx->transfer_buffer;
    struct usb_event event;

    TRACE("Completing IRP %p, status %#x.\n", irp, transfer->status);

    free(transfer_ctx);
    urb->UrbHeader.Status = usbd_status_from_libusb(transfer->status);

    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        switch (urb->UrbHeader.Function)
        {
            case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
                urb->UrbBulkOrInterruptTransfer.TransferBufferLength = transfer->actual_length;
                break;

            case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            {
                struct _URB_CONTROL_DESCRIPTOR_REQUEST *req = &urb->UrbControlDescriptorRequest;
                req->TransferBufferLength = transfer->actual_length;
                memcpy(transfer_buffer, libusb_control_transfer_get_data(transfer), transfer->actual_length);
                break;
            }

            case URB_FUNCTION_VENDOR_DEVICE:
            case URB_FUNCTION_VENDOR_INTERFACE:
            {
                struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *req = &urb->UrbControlVendorClassRequest;
                req->TransferBufferLength = transfer->actual_length;
                if (req->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
                    memcpy(transfer_buffer, libusb_control_transfer_get_data(transfer), transfer->actual_length);
                break;
            }

            default:
                ERR("Unexpected function %#x.\n", urb->UrbHeader.Function);
        }
    }

    event.type = USB_EVENT_TRANSFER_COMPLETE;
    event.u.completed_irp = irp;
    queue_event(&event);
}

struct pipe
{
    unsigned char endpoint;
    unsigned char type;
};

static HANDLE make_pipe_handle(unsigned char endpoint, USBD_PIPE_TYPE type)
{
    union
    {
        struct pipe pipe;
        HANDLE handle;
    } u;

    u.pipe.endpoint = endpoint;
    u.pipe.type = type;
    return u.handle;
}

static struct pipe get_pipe(HANDLE handle)
{
    union
    {
        struct pipe pipe;
        HANDLE handle;
    } u;

    u.handle = handle;
    return u.pipe;
}

static NTSTATUS usb_submit_urb(void *args)
{
    const struct usb_submit_urb_params *params = args;
    IRP *irp = params->irp;

    URB *urb = IoGetCurrentIrpStackLocation(irp)->Parameters.Others.Argument1;
    libusb_device_handle *handle = params->device->handle;
    struct libusb_transfer *transfer;
    int ret;

    TRACE("type %#x.\n", urb->UrbHeader.Function);

    switch (urb->UrbHeader.Function)
    {
        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
        {
            struct _URB_PIPE_REQUEST *req = &urb->UrbPipeRequest;
            struct pipe pipe = get_pipe(req->PipeHandle);

            if ((ret = libusb_clear_halt(handle, pipe.endpoint)) < 0)
                ERR("Failed to clear halt: %s\n", libusb_strerror(ret));

            return STATUS_SUCCESS;
        }

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        {
            struct _URB_BULK_OR_INTERRUPT_TRANSFER *req = &urb->UrbBulkOrInterruptTransfer;
            struct pipe pipe = get_pipe(req->PipeHandle);
            struct transfer_ctx *transfer_ctx;

            if (!(transfer_ctx = calloc(1, sizeof(*transfer_ctx))))
                return STATUS_NO_MEMORY;
            transfer_ctx->irp = irp;
            transfer_ctx->transfer_buffer = params->transfer_buffer;

            if (!(transfer = libusb_alloc_transfer(0)))
            {
                free(transfer_ctx);
                return STATUS_NO_MEMORY;
            }
            irp->Tail.Overlay.DriverContext[0] = transfer;

            if (pipe.type == UsbdPipeTypeBulk)
            {
                libusb_fill_bulk_transfer(transfer, handle, pipe.endpoint,
                        params->transfer_buffer, req->TransferBufferLength, transfer_cb, transfer_ctx, 0);
            }
            else if (pipe.type == UsbdPipeTypeInterrupt)
            {
                libusb_fill_interrupt_transfer(transfer, handle, pipe.endpoint,
                        params->transfer_buffer, req->TransferBufferLength, transfer_cb, transfer_ctx, 0);
            }
            else
            {
                WARN("Invalid pipe type %#x.\n", pipe.type);
                free(transfer_ctx);
                libusb_free_transfer(transfer);
                return USBD_STATUS_INVALID_PIPE_HANDLE;
            }

            transfer->flags = LIBUSB_TRANSFER_FREE_TRANSFER;
            ret = libusb_submit_transfer(transfer);
            if (ret < 0)
                ERR("Failed to submit bulk transfer: %s\n", libusb_strerror(ret));

            return STATUS_PENDING;
        }

        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        {
            struct _URB_CONTROL_DESCRIPTOR_REQUEST *req = &urb->UrbControlDescriptorRequest;
            struct transfer_ctx *transfer_ctx;
            unsigned char *buffer;

            if (!(transfer_ctx = calloc(1, sizeof(*transfer_ctx))))
                return STATUS_NO_MEMORY;
            transfer_ctx->irp = irp;
            transfer_ctx->transfer_buffer = params->transfer_buffer;

            if (!(transfer = libusb_alloc_transfer(0)))
            {
                free(transfer_ctx);
                return STATUS_NO_MEMORY;
            }
            irp->Tail.Overlay.DriverContext[0] = transfer;

            if (!(buffer = malloc(sizeof(struct libusb_control_setup) + req->TransferBufferLength)))
            {
                free(transfer_ctx);
                libusb_free_transfer(transfer);
                return STATUS_NO_MEMORY;
            }

            libusb_fill_control_setup(buffer,
                    LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
                    LIBUSB_REQUEST_GET_DESCRIPTOR, (req->DescriptorType << 8) | req->Index,
                    req->LanguageId, req->TransferBufferLength);
            libusb_fill_control_transfer(transfer, handle, buffer, transfer_cb, transfer_ctx, 0);
            transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER;
            ret = libusb_submit_transfer(transfer);
            if (ret < 0)
                ERR("Failed to submit GET_DESCRIPTOR transfer: %s\n", libusb_strerror(ret));

            return STATUS_PENDING;
        }

        case URB_FUNCTION_SELECT_CONFIGURATION:
        {
            struct _URB_SELECT_CONFIGURATION *req = &urb->UrbSelectConfiguration;
            ULONG i;

            /* FIXME: In theory, we'd call libusb_set_configuration() here, but
             * the CASIO FX-9750GII (which has only one configuration) goes into
             * an error state if it receives a SET_CONFIGURATION request. Maybe
             * we should skip setting that if and only if the configuration is
             * already active? */

            for (i = 0; i < req->Interface.NumberOfPipes; ++i)
            {
                USBD_PIPE_INFORMATION *pipe = &req->Interface.Pipes[i];
                pipe->PipeHandle = make_pipe_handle(pipe->EndpointAddress, pipe->PipeType);
            }

            return STATUS_SUCCESS;
        }

        case URB_FUNCTION_VENDOR_DEVICE:
        case URB_FUNCTION_VENDOR_INTERFACE:
        {
            struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *req = &urb->UrbControlVendorClassRequest;
            uint8_t req_type = LIBUSB_REQUEST_TYPE_VENDOR;
            struct transfer_ctx *transfer_ctx;
            unsigned char *buffer;

            if (!(transfer_ctx = calloc(1, sizeof(*transfer_ctx))))
                return STATUS_NO_MEMORY;
            transfer_ctx->irp = irp;
            transfer_ctx->transfer_buffer = params->transfer_buffer;

            if (urb->UrbHeader.Function == URB_FUNCTION_VENDOR_DEVICE)
                req_type |= LIBUSB_RECIPIENT_DEVICE;
            else
                req_type |= LIBUSB_RECIPIENT_INTERFACE;

            if (req->TransferFlags & USBD_TRANSFER_DIRECTION_IN)
                req_type |= LIBUSB_ENDPOINT_IN;
            if (req->TransferFlags & ~USBD_TRANSFER_DIRECTION_IN)
                FIXME("Unhandled flags %#x.\n", (int)req->TransferFlags);

            if (!(transfer = libusb_alloc_transfer(0)))
            {
                free(transfer_ctx);
                return STATUS_NO_MEMORY;
            }
            irp->Tail.Overlay.DriverContext[0] = transfer;

            if (!(buffer = malloc(sizeof(struct libusb_control_setup) + req->TransferBufferLength)))
            {
                free(transfer_ctx);
                libusb_free_transfer(transfer);
                return STATUS_NO_MEMORY;
            }

            libusb_fill_control_setup(buffer, req_type, req->Request,
                    req->Value, req->Index, req->TransferBufferLength);
            if (!(req->TransferFlags & USBD_TRANSFER_DIRECTION_IN))
                memcpy(buffer + LIBUSB_CONTROL_SETUP_SIZE, params->transfer_buffer, req->TransferBufferLength);
            libusb_fill_control_transfer(transfer, handle, buffer, transfer_cb, transfer_ctx, 0);
            transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER;
            ret = libusb_submit_transfer(transfer);
            if (ret < 0)
                ERR("Failed to submit vendor-specific interface transfer: %s\n", libusb_strerror(ret));

            return STATUS_PENDING;
        }

        default:
            FIXME("Unhandled function %#x.\n", urb->UrbHeader.Function);
    }

    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS usb_cancel_transfer(void *args)
{
    const struct usb_cancel_transfer_params *params = args;
    int ret;

    if ((ret = libusb_cancel_transfer(params->transfer)) < 0)
        ERR("Failed to cancel transfer: %s\n", libusb_strerror(ret));

    return STATUS_SUCCESS;
}

static void decref_device(struct unix_device *device)
{
    pthread_mutex_lock(&device_mutex);

    if (--device->refcount)
    {
        pthread_mutex_unlock(&device_mutex);
        return;
    }

    list_remove(&device->entry);

    pthread_mutex_unlock(&device_mutex);

    if (device->parent)
        decref_device(device->parent);
    else
        libusb_close(device->handle);
    free(device);
}

static NTSTATUS usb_destroy_device(void *args)
{
    const struct usb_destroy_device_params *params = args;
    struct unix_device *device = params->device;

    decref_device(device);

    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
#define X(name) [unix_ ## name] = name
    X(usb_main_loop),
    X(usb_init),
    X(usb_exit),
    X(usb_submit_urb),
    X(usb_cancel_transfer),
    X(usb_destroy_device),
};
