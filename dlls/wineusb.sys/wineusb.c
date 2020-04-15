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
#include <stdlib.h>
#include <libusb.h>

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
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineusb);

static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static BOOL thread_shutdown;
static HANDLE event_thread;

static DWORD CALLBACK event_thread_proc(void *arg)
{
    int ret;

    TRACE("Starting event thread.\n");

    while (!thread_shutdown)
    {
        if ((ret = libusb_handle_events(NULL)))
            ERR("Error handling events: %s\n", libusb_strerror(ret));
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
        case IRP_MN_START_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            thread_shutdown = TRUE;
            libusb_interrupt_event_handler(NULL);
            WaitForSingleObject(event_thread, INFINITE);
            CloseHandle(event_thread);

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            ret = IoCallDriver(bus_pdo, irp);
            IoDetachDevice(bus_pdo);
            IoDeleteDevice(bus_fdo);
            return ret;

        default:
            FIXME("Unhandled minor function %#x.\n", stack->MinorFunction);
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    return fdo_pnp(irp);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo)
{
    NTSTATUS ret;

    TRACE("driver %p, pdo %p.\n", driver, pdo);

    if ((ret = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo)))
    {
        ERR("Failed to create FDO, status %#x.\n", ret);
        return ret;
    }

    IoAttachDeviceToDeviceStack(bus_fdo, pdo);
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    libusb_exit(NULL);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    int err;

    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    if ((err = libusb_init(NULL)))
    {
        ERR("Failed to initialize libusb: %s\n", libusb_strerror(err));
        return STATUS_UNSUCCESSFUL;
    }

    event_thread = CreateThread(NULL, 0, event_thread_proc, NULL, 0, NULL);

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;

    return STATUS_SUCCESS;
}
