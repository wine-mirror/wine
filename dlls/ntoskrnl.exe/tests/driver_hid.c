/*
 * HID Plug and Play test driver
 *
 * Copyright 2021 Zebediah Figura
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
#include "hidusage.h"
#include "ddk/hidpi.h"
#include "ddk/hidport.h"

#include "wine/list.h"

#include "driver.h"
#include "utils.h"

static UNICODE_STRING control_symlink;

static unsigned int got_start_device;
static DWORD report_id;

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (winetest_debug > 1) trace("pnp %#x\n", stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            ++got_start_device;
            IoSetDeviceInterfaceState(&control_symlink, TRUE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            IoSetDeviceInterfaceState(&control_symlink, FALSE);
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(ext->NextDeviceObject, irp);
}


static NTSTATUS WINAPI driver_power(DEVICE_OBJECT *device, IRP *irp)
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    /* We do not expect power IRPs as part of normal operation. */
    ok(0, "unexpected call\n");

    PoStartNextPowerIrp(irp);
    IoSkipCurrentIrpStackLocation(irp);
    return PoCallDriver(ext->NextDeviceObject, irp);
}

static NTSTATUS WINAPI driver_internal_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
#include "psh_hid_macros.h"
/* Replace REPORT_ID with USAGE_PAGE when id is 0 */
#define REPORT_ID_OR_USAGE_PAGE(size, id, off) SHORT_ITEM_1((id ? 8 : 0), 1, (id + off))
    const unsigned char report_descriptor[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Logical),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                LOGICAL_MINIMUM(1, -128),
                LOGICAL_MAXIMUM(1, 127),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 8),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_MINIMUM(1, 0x18),
                USAGE_MAXIMUM(1, 0x1f),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Cnst|Var|Abs),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Cnst|Var|Abs),
                /* needs to be 8 bit aligned as next has Buff */

                USAGE_MINIMUM(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8),
                USAGE_MAXIMUM(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0xf),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 8),
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 8),
                INPUT(2, Data|Ary|Rel|Wrap|Lin|Pref|Null|Vol|Buff),

                /* needs to be 8 bit aligned as previous has Buff */
                USAGE(1, 0x20),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),
                USAGE_MINIMUM(1, 0x21),
                USAGE_MAXIMUM(1, 0x22),
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 0),
                INPUT(1, Data|Var|Abs),
                USAGE(1, 0x23),
                REPORT_COUNT(1, 0),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 8),
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(4, 0x00000000),
                LOGICAL_MAXIMUM(4, 0x3fffffff),
                PHYSICAL_MINIMUM(4, 0x80000000),
                PHYSICAL_MAXIMUM(4, 0x7fffffff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                /* reset physical range to its default interpretation */
                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_RX),
                PHYSICAL_MINIMUM(4, 0),
                PHYSICAL_MAXIMUM(4, 0),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_RY),
                LOGICAL_MINIMUM(4, 0x7fff),
                LOGICAL_MAXIMUM(4, 0x0000),
                PHYSICAL_MINIMUM(4, 0x0000),
                PHYSICAL_MAXIMUM(4, 0x7fff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 1),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 9),
                USAGE_MAXIMUM(1, 10),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_LED),
            USAGE(1, HID_USAGE_LED_GREEN),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(1, HID_USAGE_PAGE_LED),
                USAGE(1, 1),
                USAGE(1, 2),
                USAGE(1, 3),
                USAGE(1, 4),
                USAGE(1, 5),
                USAGE(1, 6),
                USAGE(1, 7),
                USAGE(1, 8),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
            USAGE(1, HID_USAGE_HAPTICS_SIMPLE_CONTROLLER),
            COLLECTION(1, Logical),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),

                USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_LIST),
                COLLECTION(1, NamedArray),
                    USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                    USAGE(1, 3), /* HID_USAGE_HAPTICS_WAVEFORM_RUMBLE */
                    USAGE(1, 4), /* HID_USAGE_HAPTICS_WAVEFORM_BUZZ */
                    LOGICAL_MINIMUM(2, 0x0000),
                    LOGICAL_MAXIMUM(2, 0xffff),
                    REPORT_COUNT(1, 2),
                    REPORT_SIZE(1, 16),
                    FEATURE(1, Data|Var|Abs|Null),
                END_COLLECTION,

                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
                USAGE(1, HID_USAGE_HAPTICS_DURATION_LIST),
                COLLECTION(1, NamedArray),
                    USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                    USAGE(1, 3), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_RUMBLE) */
                    USAGE(1, 4), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_BUZZ) */
                    LOGICAL_MINIMUM(2, 0x0000),
                    LOGICAL_MAXIMUM(2, 0xffff),
                    REPORT_COUNT(1, 2),
                    REPORT_SIZE(1, 16),
                    FEATURE(1, Data|Var|Abs|Null),
                END_COLLECTION,

                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
                USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME),
                UNIT(2, 0x1001), /* seconds */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(2, 0x8000),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(4, 0x00000000),
                PHYSICAL_MAXIMUM(4, 0xffffffff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 2),
                FEATURE(1, Data|Var|Abs),
                /* reset global items */
                UNIT(1, 0), /* None */
                UNIT_EXPONENT(1, 0),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 1),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 9),
                USAGE_MAXIMUM(1, 10),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
#undef REPORT_ID_OR_USAGE_PAGE
#include "pop_hid_macros.h"

    static BOOL test_failed;
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    const ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
    const ULONG out_size = stack->Parameters.DeviceIoControl.OutputBufferLength;
    const ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS ret;

    if (winetest_debug > 1) trace("ioctl %#x\n", code);

    ok(got_start_device, "expected IRP_MN_START_DEVICE before any ioctls\n");

    irp->IoStatus.Information = 0;

    switch (code)
    {
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            HID_DESCRIPTOR *desc = irp->UserBuffer;

            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(*desc), "got output size %u\n", out_size);

            if (out_size == sizeof(*desc))
            {
                ok(!desc->bLength, "got size %u\n", desc->bLength);

                desc->bLength = sizeof(*desc);
                desc->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
                desc->bcdHID = HID_REVISION;
                desc->bCountry = 0;
                desc->bNumDescriptors = 1;
                desc->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
                desc->DescriptorList[0].wReportLength = sizeof(report_descriptor);
                irp->IoStatus.Information = sizeof(*desc);
            }
            ret = STATUS_SUCCESS;
            break;
        }

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(report_descriptor), "got output size %u\n", out_size);

            if (out_size == sizeof(report_descriptor))
            {
                memcpy(irp->UserBuffer, report_descriptor, sizeof(report_descriptor));
                irp->IoStatus.Information = sizeof(report_descriptor);
            }
            ret = STATUS_SUCCESS;
            break;

        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            HID_DEVICE_ATTRIBUTES *attr = irp->UserBuffer;

            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == sizeof(*attr), "got output size %u\n", out_size);

            if (out_size == sizeof(*attr))
            {
                ok(!attr->Size, "got size %u\n", attr->Size);

                attr->Size = sizeof(*attr);
                attr->VendorID = 0x1209;
                attr->ProductID = 0x0001;
                attr->VersionNumber = 0xface;
                irp->IoStatus.Information = sizeof(*attr);
            }
            ret = STATUS_SUCCESS;
            break;
        }

        case IOCTL_HID_READ_REPORT:
        {
            ULONG expected_size = 23;
            ok(!in_size, "got input size %u\n", in_size);
            if (!test_failed)
            {
                todo_wine_if(!report_id)
                ok(out_size == expected_size, "got output size %u\n", out_size);
            }
            if (out_size != expected_size) test_failed = TRUE;

            ret = STATUS_NOT_IMPLEMENTED;
            break;
        }

        case IOCTL_HID_GET_STRING:
            ok(!in_size, "got input size %u\n", in_size);
            ok(out_size == 128, "got output size %u\n", out_size);

            ret = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            ok(0, "unexpected ioctl %#x\n", code);
            ret = STATUS_NOT_IMPLEMENTED;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    HID_DEVICE_EXTENSION *ext = device->DeviceExtension;

    ok(0, "unexpected call\n");
    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(ext->NextDeviceObject, irp);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *fdo)
{
    HID_DEVICE_EXTENSION *ext = fdo->DeviceExtension;
    NTSTATUS ret;

    /* We should be given the FDO, not the PDO. */
    ok(!!ext->PhysicalDeviceObject, "expected non-NULL pdo\n");
    ok(ext->NextDeviceObject == ext->PhysicalDeviceObject, "got pdo %p, next %p\n",
            ext->PhysicalDeviceObject, ext->NextDeviceObject);
    todo_wine ok(ext->NextDeviceObject->AttachedDevice == fdo, "wrong attached device\n");

    ret = IoRegisterDeviceInterface(ext->PhysicalDeviceObject, &control_class, NULL, &control_symlink);
    ok(!ret, "got %#x\n", ret);

    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    ok(0, "unexpected call\n");
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    ok(0, "unexpected call\n");
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
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );
    char buffer[offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data ) + sizeof(DWORD)];
    HID_MINIDRIVER_REGISTRATION params =
    {
        .Revision = HID_REVISION,
        .DriverObject = driver,
        .RegistryPath = registry,
    };
    UNICODE_STRING name_str;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS ret;
    HANDLE hkey;
    DWORD size;

    if ((ret = winetest_init()))
        return ret;

    InitializeObjectAttributes(&attr, registry, 0, NULL, NULL);
    ret = ZwOpenKey(&hkey, KEY_ALL_ACCESS, &attr);
    ok(!ret, "ZwOpenKey returned %#x\n", ret);

    RtlInitUnicodeString(&name_str, L"ReportID");
    size = info_size + sizeof(report_id);
    ret = ZwQueryValueKey(hkey, &name_str, KeyValuePartialInformation, buffer, size, &size);
    ok(!ret, "ZwQueryValueKey returned %#x\n", ret);
    memcpy(&report_id, buffer + info_size, size - info_size);

    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;
    driver->MajorFunction[IRP_MJ_PNP] = driver_pnp;
    driver->MajorFunction[IRP_MJ_POWER] = driver_power;
    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = driver_internal_ioctl;
    driver->MajorFunction[IRP_MJ_CREATE] = driver_create;
    driver->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    ret = HidRegisterMinidriver(&params);
    ok(!ret, "got %#x\n", ret);

    return STATUS_SUCCESS;
}
