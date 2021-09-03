/*
 * WINE XInput device driver
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"

#include "cfgmgr32.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winexinput);

#if defined(__i386__) && !defined(_WIN32)
extern void *WINAPI wrap_fastcall_func1(void *func, const void *a);
__ASM_STDCALL_FUNC(wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax");
#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)
#else
#define call_fastcall_func1(func,a) func(a)
#endif

struct device
{
    BOOL is_fdo;
    BOOL removed;
    WCHAR device_id[MAX_DEVICE_ID_LEN];
};

static inline struct device *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct device *)device->DeviceExtension;
}

struct phys_device
{
    struct device base;
    struct func_device *fdo;
};

struct func_device
{
    struct device base;
    DEVICE_OBJECT *bus_device;

    /* the bogus HID gamepad, as exposed by native XUSB */
    DEVICE_OBJECT *gamepad_device;
    WCHAR instance_id[MAX_DEVICE_ID_LEN];
};

static inline struct func_device *fdo_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    if (impl->is_fdo) return CONTAINING_RECORD(impl, struct func_device, base);
    else return CONTAINING_RECORD(impl, struct phys_device, base)->fdo;
}

static NTSTATUS WINAPI internal_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    struct device *impl = impl_from_DEVICE_OBJECT(device);

    if (InterlockedOr(&impl->removed, FALSE))
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    TRACE("device %p, irp %p, code %#x, bus_device %p.\n", device, irp, code, fdo->bus_device);

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(fdo->bus_device, irp);
}

static WCHAR *query_instance_id(DEVICE_OBJECT *device)
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    DWORD size = (wcslen(fdo->instance_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size)))
        memcpy(dst, fdo->instance_id, size);

    return dst;
}

static WCHAR *query_device_id(DEVICE_OBJECT *device)
{
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    DWORD size = (wcslen(impl->device_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size)))
        memcpy(dst, impl->device_id, size);

    return dst;
}

static WCHAR *query_hardware_ids(DEVICE_OBJECT *device)
{
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    DWORD size = (wcslen(impl->device_id) + 1) * sizeof(WCHAR);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size + sizeof(WCHAR))))
    {
        memcpy(dst, impl->device_id, size);
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static WCHAR *query_compatible_ids(DEVICE_OBJECT *device)
{
    static const WCHAR hid_compat[] = L"WINEBUS\\WINE_COMP_HID";
    DWORD size = sizeof(hid_compat);
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, size + sizeof(WCHAR))))
    {
        memcpy(dst, hid_compat, sizeof(hid_compat));
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static NTSTATUS WINAPI pdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    ULONG code = stack->MinorFunction;
    NTSTATUS status;

    TRACE("device %p, irp %p, code %#x, bus_device %p.\n", device, irp, code, fdo->bus_device);

    switch (code)
    {
    case IRP_MN_START_DEVICE:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        status = STATUS_SUCCESS;
        if (InterlockedExchange(&impl->removed, TRUE)) break;
        break;

    case IRP_MN_REMOVE_DEVICE:
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        IoDeleteDevice(device);
        return STATUS_SUCCESS;

    case IRP_MN_QUERY_ID:
    {
        BUS_QUERY_ID_TYPE type = stack->Parameters.QueryId.IdType;
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
        default:
            FIXME("IRP_MN_QUERY_ID type %u, not implemented!\n", type);
            status = irp->IoStatus.Status;
            break;
        }
        break;
    }

    case IRP_MN_QUERY_CAPABILITIES:
        status = STATUS_SUCCESS;
        break;

    default:
        FIXME("code %#x, not implemented!\n", code);
        status = irp->IoStatus.Status;
        break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS create_child_pdos(DEVICE_OBJECT *device)
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    DEVICE_OBJECT *gamepad_device;
    struct phys_device *pdo;
    UNICODE_STRING name_str;
    WCHAR *tmp, name[255];
    NTSTATUS status;

    swprintf(name, ARRAY_SIZE(name), L"\\Device\\WINEXINPUT#%p&%p&0",
             device->DriverObject, fdo->bus_device);
    RtlInitUnicodeString(&name_str, name);

    if ((status = IoCreateDevice(device->DriverObject, sizeof(struct phys_device),
                                 &name_str, 0, 0, FALSE, &gamepad_device)))
    {
        ERR("failed to create gamepad device, status %#x.\n", status);
        return status;
    }

    fdo->gamepad_device = gamepad_device;
    pdo = gamepad_device->DeviceExtension;
    pdo->fdo = fdo;
    pdo->base.is_fdo = FALSE;
    wcscpy(pdo->base.device_id, fdo->base.device_id);

    if ((tmp = wcsstr(pdo->base.device_id, L"&MI_"))) memcpy(tmp, L"&IG", 6);
    else wcscat(pdo->base.device_id, L"&IG_00");

    TRACE("device %p, gamepad device %p.\n", device, gamepad_device);

    IoInvalidateDeviceRelations(fdo->bus_device, BusRelations);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI set_event_completion(DEVICE_OBJECT *device, IRP *irp, void *context)
{
    if (irp->PendingReturned) KeSetEvent((KEVENT *)context, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS WINAPI fdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    ULONG code = stack->MinorFunction;
    DEVICE_RELATIONS *devices;
    DEVICE_OBJECT *child;
    NTSTATUS status;
    KEVENT event;

    TRACE("device %p, irp %p, code %#x, bus_device %p.\n", device, irp, code, fdo->bus_device);

    switch (stack->MinorFunction)
    {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (stack->Parameters.QueryDeviceRelations.Type == BusRelations)
        {
            if (!(devices = ExAllocatePool(PagedPool, offsetof(DEVICE_RELATIONS, Objects[2]))))
            {
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return STATUS_NO_MEMORY;
            }

            devices->Count = 0;
            if ((child = fdo->gamepad_device))
            {
                devices->Objects[devices->Count] = child;
                call_fastcall_func1(ObfReferenceObject, child);
                devices->Count++;
            }

            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
        }

        IoSkipCurrentIrpStackLocation(irp);
        return IoCallDriver(fdo->bus_device, irp);

    case IRP_MN_START_DEVICE:
        KeInitializeEvent(&event, NotificationEvent, FALSE);
        IoCopyCurrentIrpStackLocationToNext(irp);
        IoSetCompletionRoutine(irp, set_event_completion, &event, TRUE, TRUE, TRUE);

        status = IoCallDriver(fdo->bus_device, irp);
        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = irp->IoStatus.Status;
        }
        if (!status) status = create_child_pdos(device);

        if (status) irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;

    case IRP_MN_REMOVE_DEVICE:
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(fdo->bus_device, irp);
        IoDetachDevice(fdo->bus_device);
        IoDeleteDevice(device);
        return status;

    default:
        IoSkipCurrentIrpStackLocation(irp);
        return IoCallDriver(fdo->bus_device, irp);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    if (impl->is_fdo) return fdo_pnp(device, irp);
    return pdo_pnp(device, irp);
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR *id)
{
    IO_STACK_LOCATION *stack;
    IO_STATUS_BLOCK io;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, &event, &io);
    if (irp == NULL) return STATUS_NO_MEMORY;

    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_ID;
    stack->Parameters.QueryId.IdType = type;

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    wcscpy(id, (WCHAR *)io.Information);
    ExFreePool((WCHAR *)io.Information);
    return io.Status;
}

static NTSTATUS WINAPI add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *bus_device)
{
    WCHAR bus_id[MAX_DEVICE_ID_LEN], *device_id, instance_id[MAX_DEVICE_ID_LEN];
    struct func_device *fdo;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    TRACE("driver %p, bus_device %p.\n", driver, bus_device);

    if ((status = get_device_id(bus_device, BusQueryDeviceID, bus_id)))
    {
        ERR("failed to get bus device id, status %#x.\n", status);
        return status;
    }

    if ((device_id = wcsrchr(bus_id, '\\'))) *device_id++ = 0;
    else
    {
        ERR("unexpected device id %s\n", debugstr_w(bus_id));
        return STATUS_UNSUCCESSFUL;
    }

    if ((status = get_device_id(bus_device, BusQueryInstanceID, instance_id)))
    {
        ERR("failed to get bus device instance id, status %#x.\n", status);
        return status;
    }

    if ((status = IoCreateDevice(driver, sizeof(struct func_device), NULL,
                                 FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &device)))
    {
        ERR("failed to create bus FDO, status %#x.\n", status);
        return status;
    }

    fdo = device->DeviceExtension;
    fdo->base.is_fdo = TRUE;
    swprintf(fdo->base.device_id, MAX_DEVICE_ID_LEN, L"WINEXINPUT\\%s", device_id);

    fdo->bus_device = bus_device;
    wcscpy(fdo->instance_id, instance_id);

    TRACE("device %p, bus_id %s, device_id %s, instance_id %s.\n", device, debugstr_w(bus_id),
          debugstr_w(fdo->base.device_id), debugstr_w(fdo->instance_id));

    IoAttachDeviceToDeviceStack(device, bus_device);
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    TRACE("driver %p\n", driver);
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *path)
{
    TRACE("driver %p, path %s.\n", driver, debugstr_w(path->Buffer));

    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = internal_ioctl;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->DriverExtension->AddDevice = add_device;
    driver->DriverUnload = driver_unload;

    return STATUS_SUCCESS;
}
