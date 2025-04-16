/*
 * ntoskrnl.exe testing framework
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

#include "wine/list.h"

#include "initguid.h"
#include "devpkey.h"
#include "driver.h"
#include "utils.h"

/* memcmp() isn't exported from ntoskrnl on i386 */
static int kmemcmp( const void *ptr1, const void *ptr2, size_t n )
{
    const unsigned char *p1, *p2;

    for (p1 = ptr1, p2 = ptr2; n; n--, p1++, p2++)
    {
        if (*p1 < *p2) return -1;
        if (*p1 > *p2) return 1;
    }
    return 0;
}

static const GUID bus_class     = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc1}};
static const GUID child_class   = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc2}};
static UNICODE_STRING control_symlink, bus_symlink;

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static unsigned int remove_device_count, surprise_removal_count, query_remove_device_count, cancel_remove_device_count;

struct irp_queue
{
    KSPIN_LOCK lock;
    LIST_ENTRY list;
};

static IRP *irp_queue_pop(struct irp_queue *queue)
{
    KIRQL irql;
    IRP *irp;

    KeAcquireSpinLock(&queue->lock, &irql);
    if (IsListEmpty(&queue->list)) irp = NULL;
    else irp = CONTAINING_RECORD(RemoveHeadList(&queue->list), IRP, Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&queue->lock, irql);

    return irp;
}

static void irp_queue_push(struct irp_queue *queue, IRP *irp)
{
    KIRQL irql;

    KeAcquireSpinLock(&queue->lock, &irql);
    InsertTailList(&queue->list, &irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&queue->lock, irql);
}

static void irp_queue_clear(struct irp_queue *queue)
{
    IRP *irp;

    while ((irp = irp_queue_pop(queue)))
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

static void irp_queue_init(struct irp_queue *queue)
{
    KeInitializeSpinLock(&queue->lock);
    InitializeListHead(&queue->list);
}

struct device
{
    struct list entry;
    DEVICE_OBJECT *device_obj;
    unsigned int id;
    BOOL removed;
    UNICODE_STRING child_symlink;
    DEVICE_POWER_STATE power_state;
    struct irp_queue irp_queue;
    HANDLE plug_event, plug_event2;
};

static struct list device_list = LIST_INIT(device_list);

static FAST_MUTEX driver_lock;

static NTSTATUS fdo_pnp(IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret;

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            irp->IoStatus.Status = IoSetDeviceInterfaceState(&control_symlink, TRUE);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            IoSetDeviceInterfaceState(&control_symlink, FALSE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            ret = IoCallDriver(bus_pdo, irp);
            IoDetachDevice(bus_pdo);
            IoDeleteDevice(bus_fdo);
            RtlFreeUnicodeString(&control_symlink);
            RtlFreeUnicodeString(&bus_symlink);
            return ret;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DEVICE_RELATIONS *devices;
            struct device *device;
            unsigned int i = 0;

            if (stack->Parameters.QueryDeviceRelations.Type == RemovalRelations)
                break;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                ok(0, "Unexpected relations type %#x.\n", stack->Parameters.QueryDeviceRelations.Type);
                break;
            }

            ExAcquireFastMutex(&driver_lock);

            if (!(devices = ExAllocatePool(PagedPool,
                    offsetof(DEVICE_RELATIONS, Objects[list_count(&device_list)]))))
            {
                ExReleaseFastMutex(&driver_lock);
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                break;
            }

            LIST_FOR_EACH_ENTRY(device, &device_list, struct device, entry)
            {
                devices->Objects[i++] = device->device_obj;
                ObfReferenceObject(device->device_obj);
            }

            ExReleaseFastMutex(&driver_lock);

            devices->Count = i;
            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

static NTSTATUS query_id(struct device *device, IRP *irp, BUS_QUERY_ID_TYPE type)
{
    static const WCHAR device_id[] = L"Wine\\Test";
    WCHAR *id = NULL;

    irp->IoStatus.Information = 0;

    switch (type)
    {
        case BusQueryDeviceID:
            if (!(id = ExAllocatePool(PagedPool, sizeof(device_id))))
                return STATUS_NO_MEMORY;
            wcscpy(id, device_id);
            break;

        case BusQueryInstanceID:
            if (!(id = ExAllocatePool(PagedPool, 9 * sizeof(WCHAR))))
                return STATUS_NO_MEMORY;
            swprintf(id, 9, L"%x", device->id);
            break;

        case BusQueryHardwareIDs:
        {
            static const WCHAR hardware_id[] = L"winetest_hardware";
            const size_t size = ARRAY_SIZE(hardware_id) + 27 + 1;
            size_t len;

            if (!(id = ExAllocatePool(PagedPool, size * sizeof(WCHAR))))
                return STATUS_NO_MEMORY;
            wcscpy(id, hardware_id);
            len = swprintf(id + ARRAY_SIZE(hardware_id), size - ARRAY_SIZE(hardware_id),
                    L"winetest_hardware_%x", device->id);
            id[ARRAY_SIZE(hardware_id) + len + 1] = 0;
            break;
        }

        case BusQueryCompatibleIDs:
        {
            static const WCHAR compat_id[] = L"winetest_compat";
            const size_t size = ARRAY_SIZE(compat_id) + 25 + 1;
            size_t len;

            if (!(id = ExAllocatePool(PagedPool, size * sizeof(WCHAR))))
                return STATUS_NO_MEMORY;
            wcscpy(id, compat_id);
            len = swprintf(id + ARRAY_SIZE(compat_id), size - ARRAY_SIZE(compat_id),
                    L"winetest_compat_%x", device->id);
            id[ARRAY_SIZE(compat_id) + len + 1] = 0;
            break;
        }

        case BusQueryContainerID:
            if (!(id = ExAllocatePool(PagedPool, 39 * sizeof(WCHAR))))
                return STATUS_NO_MEMORY;
            wcscpy(id, L"{12345678-1234-1234-1234-123456789123}");
            break;

        default:
            ok(0, "Unexpected ID query type %#x.\n", type);
            return irp->IoStatus.Status;
    }

    irp->IoStatus.Information = (ULONG_PTR)id;
    return STATUS_SUCCESS;
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device_obj, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct device *device = device_obj->DeviceExtension;
    NTSTATUS ret = irp->IoStatus.Status;

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
            ret = query_id(device, irp, stack->Parameters.QueryId.IdType);
            break;

        case IRP_MN_START_DEVICE:
        {
            static const WCHAR expect_symlink[] = L"\\??\\Wine#Test#1#{deadbeef-29ef-4538-a5fd-b69573a362c2}";
            static const LARGE_INTEGER wait_time = {.QuadPart = -500 * 10000};
            POWER_STATE state = {.DeviceState = PowerDeviceD0};
            NTSTATUS status;

            irp_queue_init(&device->irp_queue);

            ok(!stack->Parameters.StartDevice.AllocatedResources, "expected no resources\n");
            ok(!stack->Parameters.StartDevice.AllocatedResourcesTranslated, "expected no translated resources\n");

            status = IoRegisterDeviceInterface(device_obj, &child_class, NULL, &device->child_symlink);
            ok(!status, "Failed to register interface, status %#lx.\n", status);
            ok(device->child_symlink.Length == sizeof(expect_symlink) - sizeof(WCHAR),
                    "Got length %u.\n", device->child_symlink.Length);
            ok(device->child_symlink.MaximumLength == sizeof(expect_symlink),
                    "Got maximum length %u.\n", device->child_symlink.MaximumLength);
            ok(!kmemcmp(device->child_symlink.Buffer, expect_symlink, device->child_symlink.MaximumLength),
                    "Got symlink \"%ls\".\n", device->child_symlink.Buffer);

            IoSetDeviceInterfaceState(&device->child_symlink, TRUE);

            state = PoSetPowerState(device_obj, DevicePowerState, state);
            todo_wine ok(state.DeviceState == device->power_state, "got previous state %u\n", state.DeviceState);
            device->power_state = PowerDeviceD0;

            status = ZwWaitForSingleObject(device->plug_event, TRUE, &wait_time);
            ok(!status, "Failed to wait for child plug event, status %#lx.\n", status);
            status = ZwSetEvent(device->plug_event2, NULL);
            ok(!status, "Failed to set event, status %#lx.\n", status);
            status = ZwWaitForSingleObject(device->plug_event, TRUE, &wait_time);
            ok(!status, "Failed to wait for child plug event, status %#lx.\n", status);

            ret = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
            /* should've been checked and reset by IOCTL_WINETEST_CHILD_CHECK_REMOVED */
            ok(remove_device_count == 0, "expected no IRP_MN_REMOVE_DEVICE\n");
            todo_wine ok(surprise_removal_count == 0, "expected no IRP_MN_SURPRISE_REMOVAL\n");
            ok(query_remove_device_count == 0, "expected no IRP_MN_QUERY_REMOVE_DEVICE\n");
            ok(cancel_remove_device_count == 0, "expected no IRP_MN_CANCEL_REMOVE_DEVICE\n");

            remove_device_count++;
            irp_queue_clear(&device->irp_queue);
            if (device->removed)
            {
                IoSetDeviceInterfaceState(&device->child_symlink, FALSE);
                RtlFreeUnicodeString(&device->child_symlink);
                ZwClose(device->plug_event);
                ZwClose(device->plug_event2);
                irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                IoDeleteDevice(device->device_obj);
                return STATUS_SUCCESS;
            }

            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
            unsigned int i;

            ok(caps->Size == sizeof(*caps), "wrong size %u\n", caps->Size);
            ok(caps->Version == 1, "wrong version %u\n", caps->Version);
            ok(!caps->DeviceD1, "got DeviceD1 %u\n", caps->DeviceD1);
            ok(!caps->DeviceD2, "got DeviceD2 %u\n", caps->DeviceD2);
            ok(!caps->LockSupported, "got LockSupported %u\n", caps->LockSupported);
            ok(!caps->EjectSupported, "got EjectSupported %u\n", caps->EjectSupported);
            ok(!caps->Removable, "got Removable %u\n", caps->Removable);
            ok(!caps->DockDevice, "got DockDevice %u\n", caps->DockDevice);
            ok(!caps->UniqueID, "got UniqueID %u\n", caps->UniqueID);
            ok(!caps->SilentInstall, "got SilentInstall %u\n", caps->SilentInstall);
            ok(!caps->RawDeviceOK, "got RawDeviceOK %u\n", caps->RawDeviceOK);
            ok(!caps->SurpriseRemovalOK, "got SurpriseRemovalOK %u\n", caps->SurpriseRemovalOK);
            ok(!caps->WakeFromD0, "got WakeFromD0 %u\n", caps->WakeFromD0);
            ok(!caps->WakeFromD1, "got WakeFromD1 %u\n", caps->WakeFromD1);
            ok(!caps->WakeFromD2, "got WakeFromD2 %u\n", caps->WakeFromD2);
            ok(!caps->WakeFromD3, "got WakeFromD3 %u\n", caps->WakeFromD3);
            ok(!caps->HardwareDisabled, "got HardwareDisabled %u\n", caps->HardwareDisabled);
            ok(!caps->NonDynamic, "got NonDynamic %u\n", caps->NonDynamic);
            ok(!caps->WarmEjectSupported, "got WarmEjectSupported %u\n", caps->WarmEjectSupported);
            ok(!caps->NoDisplayInUI, "got NoDisplayInUI %u\n", caps->NoDisplayInUI);
            ok(caps->Address == 0xffffffff, "got Address %#lx\n", caps->Address);
            ok(caps->UINumber == 0xffffffff, "got UINumber %#lx\n", caps->UINumber);
            for (i = 0; i < PowerSystemMaximum; ++i)
                ok(caps->DeviceState[i] == PowerDeviceUnspecified, "got DeviceState[%u] %u\n", i, caps->DeviceState[i]);
            ok(caps->SystemWake == PowerSystemUnspecified, "got SystemWake %u\n", caps->SystemWake);
            ok(caps->DeviceWake == PowerDeviceUnspecified, "got DeviceWake %u\n", caps->DeviceWake);
            ok(!caps->D1Latency, "got D1Latency %lu\n", caps->D1Latency);
            ok(!caps->D2Latency, "got D2Latency %lu\n", caps->D2Latency);
            ok(!caps->D3Latency, "got D3Latency %lu\n", caps->D3Latency);

            /* If caps->RawDeviceOK is not set, we won't receive
             * IRP_MN_START_DEVICE unless there's a function driver. */
            caps->RawDeviceOK = 1;
            caps->SurpriseRemovalOK = 1;
            caps->EjectSupported = 1;
            caps->UniqueID = 1;

            caps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
            caps->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
            caps->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
            caps->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
            caps->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
            caps->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

            ret = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_SURPRISE_REMOVAL:
        {
            static const LARGE_INTEGER wait_time = {.QuadPart = -500 * 10000};
            NTSTATUS status;

            surprise_removal_count++;
            irp_queue_clear(&device->irp_queue);

            status = ZwWaitForSingleObject(device->plug_event, TRUE, &wait_time);
            ok(!status, "Failed to wait for child plug event, status %#lx.\n", status);
            status = ZwSetEvent(device->plug_event2, NULL);
            ok(!status, "Failed to set event, status %#lx.\n", status);

            ret = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE:
            query_remove_device_count++;
            irp_queue_clear(&device->irp_queue);
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            cancel_remove_device_count++;
            ret = STATUS_SUCCESS;
            break;
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

static NTSTATUS WINAPI driver_power(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret = STATUS_NOT_SUPPORTED;

    /* We do not expect power IRPs as part of normal operation. */
    ok(0, "unexpected call\n");

    if (device == bus_fdo)
    {
        PoStartNextPowerIrp(irp);
        IoSkipCurrentIrpStackLocation(irp);
        return PoCallDriver(bus_pdo, irp);
    }

    if (stack->MinorFunction == IRP_MN_SET_POWER)
    {
        if (stack->Parameters.Power.Type == DevicePowerState)
            PoSetPowerState(device, DevicePowerState, stack->Parameters.Power.State);
        ret = STATUS_SUCCESS;
    }

    PoStartNextPowerIrp(irp);
    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static void test_bus_query_caps(DEVICE_OBJECT *top_device)
{
    DEVICE_CAPABILITIES caps;
    IO_STACK_LOCATION *stack;
    IO_STATUS_BLOCK io;
    unsigned int i;
    NTSTATUS ret;
    KEVENT event;
    IRP *irp;

    memset(&caps, 0, sizeof(caps));
    caps.Size = sizeof(caps);
    caps.Version = 1;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top_device, NULL, 0, NULL, &event, &io);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    stack->Parameters.DeviceCapabilities.Capabilities = &caps;
    ret = IoCallDriver(top_device, irp);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#lx\n", ret);

    ok(caps.Size == sizeof(caps), "wrong size %u\n", caps.Size);
    ok(caps.Version == 1, "wrong version %u\n", caps.Version);
    ok(!caps.DeviceD1, "got DeviceD1 %u\n", caps.DeviceD1);
    ok(!caps.DeviceD2, "got DeviceD2 %u\n", caps.DeviceD2);
    ok(!caps.LockSupported, "got LockSupported %u\n", caps.LockSupported);
    ok(!caps.EjectSupported, "got EjectSupported %u\n", caps.EjectSupported);
    ok(!caps.Removable, "got Removable %u\n", caps.Removable);
    ok(!caps.DockDevice, "got DockDevice %u\n", caps.DockDevice);
    ok(!caps.UniqueID, "got UniqueID %u\n", caps.UniqueID);
    ok(!caps.SilentInstall, "got SilentInstall %u\n", caps.SilentInstall);
    ok(!caps.RawDeviceOK, "got RawDeviceOK %u\n", caps.RawDeviceOK);
    ok(!caps.SurpriseRemovalOK, "got SurpriseRemovalOK %u\n", caps.SurpriseRemovalOK);
    ok(!caps.WakeFromD0, "got WakeFromD0 %u\n", caps.WakeFromD0);
    ok(!caps.WakeFromD1, "got WakeFromD1 %u\n", caps.WakeFromD1);
    ok(!caps.WakeFromD2, "got WakeFromD2 %u\n", caps.WakeFromD2);
    ok(!caps.WakeFromD3, "got WakeFromD3 %u\n", caps.WakeFromD3);
    ok(!caps.HardwareDisabled, "got HardwareDisabled %u\n", caps.HardwareDisabled);
    ok(!caps.NonDynamic, "got NonDynamic %u\n", caps.NonDynamic);
    ok(!caps.WarmEjectSupported, "got WarmEjectSupported %u\n", caps.WarmEjectSupported);
    ok(!caps.NoDisplayInUI, "got NoDisplayInUI %u\n", caps.NoDisplayInUI);
    ok(!caps.Address, "got Address %#lx\n", caps.Address);
    ok(!caps.UINumber, "got UINumber %#lx\n", caps.UINumber);
    ok(caps.DeviceState[PowerSystemUnspecified] == PowerDeviceUnspecified,
            "got DeviceState[PowerSystemUnspecified] %u\n", caps.DeviceState[PowerSystemUnspecified]);
    todo_wine ok(caps.DeviceState[PowerSystemWorking] == PowerDeviceD0,
            "got DeviceState[PowerSystemWorking] %u\n", caps.DeviceState[PowerSystemWorking]);
    for (i = PowerSystemSleeping1; i < PowerSystemMaximum; ++i)
        todo_wine ok(caps.DeviceState[i] == PowerDeviceD3, "got DeviceState[%u] %u\n", i, caps.DeviceState[i]);
    ok(caps.SystemWake == PowerSystemUnspecified, "got SystemWake %u\n", caps.SystemWake);
    ok(caps.DeviceWake == PowerDeviceUnspecified, "got DeviceWake %u\n", caps.DeviceWake);
    ok(!caps.D1Latency, "got D1Latency %lu\n", caps.D1Latency);
    ok(!caps.D2Latency, "got D2Latency %lu\n", caps.D2Latency);
    ok(!caps.D3Latency, "got D3Latency %lu\n", caps.D3Latency);

    memset(&caps, 0xff, sizeof(caps));
    caps.Size = sizeof(caps);
    caps.Version = 1;

    KeClearEvent(&event);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top_device, NULL, 0, NULL, &event, &io);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    stack->Parameters.DeviceCapabilities.Capabilities = &caps;
    ret = IoCallDriver(top_device, irp);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#lx\n", ret);

    ok(caps.Size == sizeof(caps), "wrong size %u\n", caps.Size);
    ok(caps.Version == 1, "wrong version %u\n", caps.Version);
    ok(caps.DeviceD1, "got DeviceD1 %u\n", caps.DeviceD1);
    ok(caps.DeviceD2, "got DeviceD2 %u\n", caps.DeviceD2);
    ok(caps.LockSupported, "got LockSupported %u\n", caps.LockSupported);
    ok(caps.EjectSupported, "got EjectSupported %u\n", caps.EjectSupported);
    ok(caps.Removable, "got Removable %u\n", caps.Removable);
    ok(caps.DockDevice, "got DockDevice %u\n", caps.DockDevice);
    ok(caps.UniqueID, "got UniqueID %u\n", caps.UniqueID);
    ok(caps.SilentInstall, "got SilentInstall %u\n", caps.SilentInstall);
    ok(caps.RawDeviceOK, "got RawDeviceOK %u\n", caps.RawDeviceOK);
    ok(caps.SurpriseRemovalOK, "got SurpriseRemovalOK %u\n", caps.SurpriseRemovalOK);
    ok(caps.WakeFromD0, "got WakeFromD0 %u\n", caps.WakeFromD0);
    ok(caps.WakeFromD1, "got WakeFromD1 %u\n", caps.WakeFromD1);
    ok(caps.WakeFromD2, "got WakeFromD2 %u\n", caps.WakeFromD2);
    ok(caps.WakeFromD3, "got WakeFromD3 %u\n", caps.WakeFromD3);
    ok(caps.HardwareDisabled, "got HardwareDisabled %u\n", caps.HardwareDisabled);
    ok(caps.NonDynamic, "got NonDynamic %u\n", caps.NonDynamic);
    ok(caps.WarmEjectSupported, "got WarmEjectSupported %u\n", caps.WarmEjectSupported);
    ok(caps.NoDisplayInUI, "got NoDisplayInUI %u\n", caps.NoDisplayInUI);
    ok(caps.Address == 0xffffffff, "got Address %#lx\n", caps.Address);
    ok(caps.UINumber == 0xffffffff, "got UINumber %#lx\n", caps.UINumber);
    todo_wine ok(caps.DeviceState[PowerSystemUnspecified] == PowerDeviceUnspecified,
            "got DeviceState[PowerSystemUnspecified] %u\n", caps.DeviceState[PowerSystemUnspecified]);
    todo_wine ok(caps.DeviceState[PowerSystemWorking] == PowerDeviceD0,
            "got DeviceState[PowerSystemWorking] %u\n", caps.DeviceState[PowerSystemWorking]);
    for (i = PowerSystemSleeping1; i < PowerSystemMaximum; ++i)
        todo_wine ok(caps.DeviceState[i] == PowerDeviceD3, "got DeviceState[%u] %u\n", i, caps.DeviceState[i]);
    ok(caps.SystemWake == 0xffffffff, "got SystemWake %u\n", caps.SystemWake);
    ok(caps.DeviceWake == 0xffffffff, "got DeviceWake %u\n", caps.DeviceWake);
    ok(caps.D1Latency == 0xffffffff, "got D1Latency %lu\n", caps.D1Latency);
    ok(caps.D2Latency == 0xffffffff, "got D2Latency %lu\n", caps.D2Latency);
    ok(caps.D3Latency == 0xffffffff, "got D3Latency %lu\n", caps.D3Latency);
}

static void test_bus_query_id(DEVICE_OBJECT *top_device)
{
    IO_STACK_LOCATION *stack;
    IO_STATUS_BLOCK io;
    NTSTATUS ret;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top_device, NULL, 0, NULL, &event, &io);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_ID;
    stack->Parameters.QueryId.IdType = BusQueryDeviceID;
    ret = IoCallDriver(top_device, irp);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!wcscmp((WCHAR *)io.Information, L"ROOT\\WINETEST"), "got id '%ls'\n", (WCHAR *)io.Information);
    ExFreePool((WCHAR *)io.Information);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top_device, NULL, 0, NULL, &event, &io);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_ID;
    stack->Parameters.QueryId.IdType = BusQueryInstanceID;
    ret = IoCallDriver(top_device, irp);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!wcscmp((WCHAR *)io.Information, L"0"), "got id '%ls'\n", (WCHAR *)io.Information);
    ExFreePool((WCHAR *)io.Information);
}

static void test_bus_query(void)
{
    DEVICE_OBJECT *top_device;

    top_device = IoGetAttachedDeviceReference(bus_pdo);
    ok(top_device == bus_fdo, "wrong top device\n");

    test_bus_query_caps(top_device);
    test_bus_query_id(top_device);

    ObDereferenceObject(top_device);
}

struct winetest_deviceprop
{
    const DEVPROPKEY *key;
    DEVPROPTYPE type;
    union {
        BYTE byte;
        INT16 int16;
        UINT16 uint16;
        INT32 int32;
        UINT32 uint32;
        INT64 int64;
        UINT64 uint64;
        GUID guid;
    } value;
    SIZE_T size;
};

#define WINETEST_DRIVER_DEVPROP(i, typ, val, size) {&DEVPKEY_Winetest_##i, (typ), val, (size)},
static struct winetest_deviceprop deviceprops[] = {
    WINETEST_DEFINE_DEVPROPS
    {&DEVPKEY_Winetest_8, DEVPROP_TYPE_GUID,
    {.guid = {0xdeadbeef, 0xdead, 0xbeef, {0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef}}}, sizeof(GUID)}
};
#undef WINETEST_DRIVER_DEVPROP

static void test_device_properties( DEVICE_OBJECT *device )
{
    SIZE_T i;

    for (i = 0; i < ARRAY_SIZE( deviceprops ); i++)
    {
        NTSTATUS status;
        ULONG size = deviceprops[i].size;
        DEVPROPTYPE type = deviceprops[i].type;
        const DEVPROPKEY *key = deviceprops[i].key;
        void *value = &deviceprops[i].value;

        status = IoSetDevicePropertyData( device, key, LOCALE_NEUTRAL, 0, type, size, value );
        ok( status == STATUS_SUCCESS, "Failed to set device property, status %#lx.\n", status );
        if (status == STATUS_SUCCESS)
        {
            void *buf;
            ULONG req_size = 0;
            DEVPROPTYPE stored_type = DEVPROP_TYPE_EMPTY;

            status = IoGetDevicePropertyData( device, key, LOCALE_NEUTRAL, 0, 0, NULL, &req_size,
                                              &stored_type );
            ok( status == STATUS_BUFFER_TOO_SMALL, "Expected status %#lx, got %#lx.\n",
                STATUS_BUFFER_TOO_SMALL, status );
            ok( req_size == size, "Expected required size %lu, got %lu.\n", req_size, size );
            ok( stored_type == type, "Expected DEVPROPTYPE value %#lx, got %#lx.\n", type,
                stored_type );

            buf = ExAllocatePool( PagedPool, size );
            ok( buf != NULL, "Failed to allocate buffer.\n" );
            if (buf != NULL)
            {
                req_size = 0;
                stored_type = DEVPROP_TYPE_EMPTY;
                memset( buf, 0, size );
                status = IoGetDevicePropertyData( device, key, LOCALE_NEUTRAL, 0, size, buf,
                                                  &req_size, &stored_type );
                ok( status == STATUS_SUCCESS, "Failed to get device property, status %#lx.\n",
                    status );
                ok( req_size == size, "Expected required size %lu, got %lu.\n", req_size, size );
                ok( stored_type == type, "Expected DEVPROPTYPE value %#lx, got %#lx.\n", type,
                    stored_type );
                if (status == STATUS_SUCCESS)
                    ok( kmemcmp( buf, value, size ) == 0,
                        "Got unexpected device property value.\n" );
                ExFreePool( buf );
            }
        }
        status = IoSetDevicePropertyData( device, key, LOCALE_NEUTRAL, 0, type, 0, NULL );
        ok( status == STATUS_SUCCESS, "Failed to delete device property, status %#lx.\n", status );
    }
}

static NTSTATUS fdo_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    switch (code)
    {
        case IOCTL_WINETEST_BUS_MAIN:
            test_bus_query();
            test_device_properties( bus_pdo );
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_BUS_REGISTER_IFACE:
            return IoRegisterDeviceInterface(bus_pdo, &bus_class, NULL, &bus_symlink);

        case IOCTL_WINETEST_BUS_ENABLE_IFACE:
            IoSetDeviceInterfaceState(&bus_symlink, TRUE);
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_BUS_DISABLE_IFACE:
            IoSetDeviceInterfaceState(&bus_symlink, FALSE);
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_BUS_ADD_CHILD:
        {
            static const LARGE_INTEGER wait_time = {.QuadPart = -500 * 10000};
            DEVICE_OBJECT *device_obj;
            OBJECT_ATTRIBUTES attr;
            UNICODE_STRING string;
            struct device *device;
            NTSTATUS status;
            WCHAR name[30];
            int id;

            if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(int))
                return STATUS_BUFFER_TOO_SMALL;
            id = *(int *)irp->AssociatedIrp.SystemBuffer;

            swprintf(name, ARRAY_SIZE(name), L"\\Device\\winetest_pnp_%x", id);
            RtlInitUnicodeString(&string, name);
            status = IoCreateDevice(driver_obj, sizeof(*device), &string, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj);
            ok(!status, "Failed to create device, status %#lx.\n", status);

            device = device_obj->DeviceExtension;
            memset(device, 0, sizeof(*device));
            device->device_obj = device_obj;
            device->id = id;
            device->removed = FALSE;
            InitializeObjectAttributes(&attr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
            status = ZwCreateEvent(&device->plug_event, EVENT_ALL_ACCESS, &attr, SynchronizationEvent, FALSE);
            ok(!status, "Failed to create event, status %#lx.\n", status);
            status = ZwCreateEvent(&device->plug_event2, EVENT_ALL_ACCESS, &attr, SynchronizationEvent, FALSE);
            ok(!status, "Failed to create event, status %#lx.\n", status);

            ExAcquireFastMutex(&driver_lock);
            list_add_tail(&device_list, &device->entry);
            ExReleaseFastMutex(&driver_lock);

            device_obj->Flags &= ~DO_DEVICE_INITIALIZING;

            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            IoInvalidateDeviceRelations(bus_pdo, BusRelations);

            /* Synchronize both ways, to show that the bus invalidation happens
             * completely asynchronously and that neither thread blocks waiting
             * for the other. */

            status = ZwSetEvent(device->plug_event, NULL);
            ok(!status, "Failed to set event, status %#lx.\n", status);
            status = ZwWaitForSingleObject(device->plug_event2, TRUE, &wait_time);
            ok(!status, "Failed to wait for child plug event, status %#lx.\n", status);
            /* IoInvalidateDeviceRelations() here shouldn't deadlock either. */
            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            status = ZwSetEvent(device->plug_event, NULL);
            ok(!status, "Failed to set event, status %#lx.\n", status);

            return STATUS_SUCCESS;
        }

        case IOCTL_WINETEST_BUS_REMOVE_CHILD:
        {
            static const LARGE_INTEGER wait_time = {.QuadPart = -500 * 10000};
            HANDLE plug_event = NULL, plug_event2 = NULL;
            struct device *device;
            NTSTATUS status;
            int id;

            if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(int))
                return STATUS_BUFFER_TOO_SMALL;
            id = *(int *)irp->AssociatedIrp.SystemBuffer;

            ExAcquireFastMutex(&driver_lock);
            LIST_FOR_EACH_ENTRY(device, &device_list, struct device, entry)
            {
                if (device->id == id)
                {
                    plug_event = device->plug_event;
                    plug_event2 = device->plug_event2;
                    list_remove(&device->entry);
                    device->removed = TRUE;
                    break;
                }
            }
            ExReleaseFastMutex(&driver_lock);

            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            IoInvalidateDeviceRelations(bus_pdo, BusRelations);

            /* Synchronize both ways, to show that the bus invalidation happens
             * completely asynchronously and that neither thread blocks waiting
             * for the other. */

            status = ZwSetEvent(plug_event, NULL);
            ok(!status, "Failed to set event, status %#lx.\n", status);
            status = ZwWaitForSingleObject(plug_event2, TRUE, &wait_time);
            ok(!status, "Failed to wait for child plug event, status %#lx.\n", status);
            ok(surprise_removal_count == 1, "Got %u surprise removal events.\n", surprise_removal_count);
            /* We shouldn't get IRP_MN_REMOVE_DEVICE until all user-space
             * handles to the device are closed (and the user-space thread is
             * currently blocked in this ioctl and won't close its handle
             * yet.) */
            todo_wine_if (remove_device_count)
                ok(!remove_device_count, "Got %u remove events.\n", remove_device_count);

            return STATUS_SUCCESS;
        }

        default:
            ok(0, "Unexpected ioctl %#lx.\n", code);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS pdo_ioctl(DEVICE_OBJECT *device_obj, IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    struct device *device = device_obj->DeviceExtension;

    switch (code)
    {
        case IOCTL_WINETEST_CHILD_GET_ID:
            if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(int))
                return STATUS_BUFFER_TOO_SMALL;
            *(int *)irp->AssociatedIrp.SystemBuffer = device->id;
            irp->IoStatus.Information = sizeof(device->id);
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_CHILD_MARK_PENDING:
            IoMarkIrpPending(irp);
            irp_queue_push(&device->irp_queue, irp);
            return STATUS_PENDING;

        case IOCTL_WINETEST_CHILD_CHECK_REMOVED:
            ok(remove_device_count == 0, "expected IRP_MN_REMOVE_DEVICE\n");
            ok(surprise_removal_count == 1, "expected IRP_MN_SURPRISE_REMOVAL\n");
            ok(query_remove_device_count == 0, "expected no IRP_MN_QUERY_REMOVE_DEVICE\n");
            ok(cancel_remove_device_count == 0, "expected no IRP_MN_CANCEL_REMOVE_DEVICE\n");
            remove_device_count = 0;
            surprise_removal_count = 0;
            query_remove_device_count = 0;
            cancel_remove_device_count = 0;
            return STATUS_SUCCESS;

        default:
            ok(0, "Unexpected ioctl %#lx.\n", code);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    if (device == bus_fdo)
        status = fdo_ioctl(irp, stack, code);
    else
        status = pdo_ioctl(device, irp, stack, code);

    irp->IoStatus.Status = status;
    if (status != STATUS_PENDING) IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo)
{
    NTSTATUS ret;

    if ((ret = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &bus_fdo)))
        return ret;

    if ((ret = IoRegisterDeviceInterface(pdo, &control_class, NULL, &control_symlink)))
    {
        IoDeleteDevice(bus_fdo);
        return ret;
    }

    IoAttachDeviceToDeviceStack(bus_fdo, pdo);
    bus_pdo = pdo;
    bus_fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    winetest_cleanup();
}

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *registry)
{
    NTSTATUS ret;

    if ((ret = winetest_init()))
        return ret;

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_POWER] = driver_power;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    driver_obj = driver;

    ExInitializeFastMutex(&driver_lock);

    return STATUS_SUCCESS;
}
