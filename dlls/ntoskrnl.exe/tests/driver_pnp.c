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

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"

#include "driver.h"
#include "utils.h"

static const GUID control_class = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc0}};
static const GUID bus_class     = {0xdeadbeef, 0x29ef, 0x4538, {0xa5, 0xfd, 0xb6, 0x95, 0x73, 0xa3, 0x62, 0xc1}};
static UNICODE_STRING control_symlink, bus_symlink;

static DEVICE_OBJECT *bus_fdo, *bus_pdo;

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
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device_obj, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret = irp->IoStatus.Status;

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_SURPRISE_REMOVAL:
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
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#x\n", ret);

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
    ok(!caps.Address, "got Address %#x\n", caps.Address);
    ok(!caps.UINumber, "got UINumber %#x\n", caps.UINumber);
    ok(caps.DeviceState[PowerSystemUnspecified] == PowerDeviceUnspecified,
            "got DeviceState[PowerSystemUnspecified] %u\n", caps.DeviceState[PowerSystemUnspecified]);
    todo_wine ok(caps.DeviceState[PowerSystemWorking] == PowerDeviceD0,
            "got DeviceState[PowerSystemWorking] %u\n", caps.DeviceState[PowerSystemWorking]);
    for (i = PowerSystemSleeping1; i < PowerSystemMaximum; ++i)
        todo_wine ok(caps.DeviceState[i] == PowerDeviceD3, "got DeviceState[%u] %u\n", i, caps.DeviceState[i]);
    ok(caps.SystemWake == PowerSystemUnspecified, "got SystemWake %u\n", caps.SystemWake);
    ok(caps.DeviceWake == PowerDeviceUnspecified, "got DeviceWake %u\n", caps.DeviceWake);
    ok(!caps.D1Latency, "got D1Latency %u\n", caps.D1Latency);
    ok(!caps.D2Latency, "got D2Latency %u\n", caps.D2Latency);
    ok(!caps.D3Latency, "got D3Latency %u\n", caps.D3Latency);

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
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#x\n", ret);

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
    ok(caps.Address == 0xffffffff, "got Address %#x\n", caps.Address);
    ok(caps.UINumber == 0xffffffff, "got UINumber %#x\n", caps.UINumber);
    todo_wine ok(caps.DeviceState[PowerSystemUnspecified] == PowerDeviceUnspecified,
            "got DeviceState[PowerSystemUnspecified] %u\n", caps.DeviceState[PowerSystemUnspecified]);
    todo_wine ok(caps.DeviceState[PowerSystemWorking] == PowerDeviceD0,
            "got DeviceState[PowerSystemWorking] %u\n", caps.DeviceState[PowerSystemWorking]);
    for (i = PowerSystemSleeping1; i < PowerSystemMaximum; ++i)
        todo_wine ok(caps.DeviceState[i] == PowerDeviceD3, "got DeviceState[%u] %u\n", i, caps.DeviceState[i]);
    ok(caps.SystemWake == 0xffffffff, "got SystemWake %u\n", caps.SystemWake);
    ok(caps.DeviceWake == 0xffffffff, "got DeviceWake %u\n", caps.DeviceWake);
    ok(caps.D1Latency == 0xffffffff, "got D1Latency %u\n", caps.D1Latency);
    ok(caps.D2Latency == 0xffffffff, "got D2Latency %u\n", caps.D2Latency);
    ok(caps.D3Latency == 0xffffffff, "got D3Latency %u\n", caps.D3Latency);
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
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#x\n", ret);
    ok(!wcscmp((WCHAR *)io.Information, L"ROOT\\WINETEST"), "got id '%ls'\n", (WCHAR *)io.Information);
    ExFreePool((WCHAR *)io.Information);

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, top_device, NULL, 0, NULL, &event, &io);
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    stack = IoGetNextIrpStackLocation(irp);
    stack->MinorFunction = IRP_MN_QUERY_ID;
    stack->Parameters.QueryId.IdType = BusQueryInstanceID;
    ret = IoCallDriver(top_device, irp);
    ok(ret == STATUS_SUCCESS, "got %#x\n", ret);
    ok(io.Status == STATUS_SUCCESS, "got %#x\n", ret);
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

static NTSTATUS fdo_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    switch (code)
    {
        case IOCTL_WINETEST_BUS_MAIN:
            test_bus_query();
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_BUS_REGISTER_IFACE:
            return IoRegisterDeviceInterface(bus_pdo, &bus_class, NULL, &bus_symlink);

        case IOCTL_WINETEST_BUS_ENABLE_IFACE:
            IoSetDeviceInterfaceState(&bus_symlink, TRUE);
            return STATUS_SUCCESS;

        case IOCTL_WINETEST_BUS_DISABLE_IFACE:
            IoSetDeviceInterfaceState(&bus_symlink, FALSE);
            return STATUS_SUCCESS;

        default:
            ok(0, "Unexpected ioctl %#x.\n", code);
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

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
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
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    return STATUS_SUCCESS;
}
