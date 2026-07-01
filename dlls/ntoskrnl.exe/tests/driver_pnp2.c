/*
 * ntoskrnl.exe testing framework
 *
 * Copyright 2026 Connor McAdams for CodeWeavers
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
#include "wine/debug.h"

#include "initguid.h"
#include "devpkey.h"
#include "driver.h"
#include "utils.h"

/* memcmp() isn't exported from ntoskrnl on i386 */
static int kmemcmp(const void *ptr1, const void *ptr2, size_t n)
{
    const unsigned char *p1, *p2;

    for (p1 = ptr1, p2 = ptr2; n; n--, p1++, p2++)
    {
        if (*p1 < *p2) return -1;
        if (*p1 > *p2) return 1;
    }
    return 0;
}

static const char *debugstr_pnp(ULONG code)
{
    switch (code)
    {
#define IRP_TO_STR(code) case code: return #code
        IRP_TO_STR(IRP_MN_START_DEVICE);
        IRP_TO_STR(IRP_MN_QUERY_REMOVE_DEVICE);
        IRP_TO_STR(IRP_MN_REMOVE_DEVICE);
        IRP_TO_STR(IRP_MN_CANCEL_REMOVE_DEVICE);
        IRP_TO_STR(IRP_MN_STOP_DEVICE);
        IRP_TO_STR(IRP_MN_QUERY_STOP_DEVICE);
        IRP_TO_STR(IRP_MN_CANCEL_STOP_DEVICE);
        IRP_TO_STR(IRP_MN_QUERY_DEVICE_RELATIONS);
        IRP_TO_STR(IRP_MN_QUERY_INTERFACE);
        IRP_TO_STR(IRP_MN_QUERY_CAPABILITIES);
        IRP_TO_STR(IRP_MN_QUERY_RESOURCES);
        IRP_TO_STR(IRP_MN_QUERY_RESOURCE_REQUIREMENTS);
        IRP_TO_STR(IRP_MN_QUERY_DEVICE_TEXT);
        IRP_TO_STR(IRP_MN_FILTER_RESOURCE_REQUIREMENTS);
        IRP_TO_STR(IRP_MN_READ_CONFIG);
        IRP_TO_STR(IRP_MN_WRITE_CONFIG);
        IRP_TO_STR(IRP_MN_EJECT);
        IRP_TO_STR(IRP_MN_SET_LOCK);
        IRP_TO_STR(IRP_MN_QUERY_ID);
        IRP_TO_STR(IRP_MN_QUERY_PNP_DEVICE_STATE);
        IRP_TO_STR(IRP_MN_QUERY_BUS_INFORMATION);
        IRP_TO_STR(IRP_MN_DEVICE_USAGE_NOTIFICATION);
        IRP_TO_STR(IRP_MN_SURPRISE_REMOVAL);
        IRP_TO_STR(IRP_MN_QUERY_LEGACY_BUS_INFORMATION);
        IRP_TO_STR(IRP_MN_DEVICE_ENUMERATED);
#undef IRP_TO_STR
        default: return "unknown";
    }
}

static const char *debugstr_ioctl_bus(ULONG code)
{
    switch (code)
    {
#define IOCTL_TO_STR(ioctl) case ioctl: return #ioctl
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_MAIN);
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_REGISTER_IFACE);
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_ENABLE_IFACE);
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_DISABLE_IFACE);
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_ADD_CHILD);
        IOCTL_TO_STR(IOCTL_WINETEST_BUS_REMOVE_CHILD);
#undef IOCTL_TO_STR
        default: return "unknown";
    }
}

static const char *debugstr_ioctl_bus_child(ULONG code)
{
    switch (code)
    {
#define IOCTL_TO_STR(ioctl) case ioctl: return #ioctl
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_GET_ID);
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_MARK_PENDING);
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_CHECK_REMOVED);
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_MAIN);
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_ADD_CHILD);
        IOCTL_TO_STR(IOCTL_WINETEST_CHILD_REMOVE_CHILD);
#undef IOCTL_TO_STR
        default: return "unknown";
    }
}

enum bus_device_type
{
    BUS_DEVICE_TYPE_FDO,
    BUS_DEVICE_TYPE_PDO,
};

struct bus_device_pdo
{
    struct list entry;
    struct bus_device_desc desc;
    BOOL removed;
    UNICODE_STRING iface_symlink;
    struct bus_device *parent;
};

struct bus_device_fdo
{
    struct bus_device *pdo;

    /* Maintain a list of child PDOs. */
    FAST_MUTEX child_mutex;
    struct list children;
};

struct bus_device
{
    enum bus_device_type type;
    DEVICE_OBJECT *device;
    unsigned int depth;
    union
    {
        struct bus_device_pdo pdo;
        struct bus_device_fdo fdo;
    } u;
};

static struct bus_device *impl_from_bus_device_pdo(struct bus_device_pdo *pdo)
{
    return CONTAINING_RECORD(pdo, struct bus_device, u.pdo);
}

static UNICODE_STRING control_symlink;

static DRIVER_OBJECT *driver_obj;
static DEVICE_OBJECT *bus_fdo, *bus_pdo;

static struct list device_list = LIST_INIT(device_list);

static FAST_MUTEX driver_lock;

static NTSTATUS bus_fdo_pnp(IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->MinorFunction;
    NTSTATUS ret;

    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, bus_fdo, code, debugstr_pnp(code));
    switch (code)
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
            return ret;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            struct bus_device_pdo *pdo;
            DEVICE_RELATIONS *devices;
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

            LIST_FOR_EACH_ENTRY(pdo, &device_list, struct bus_device_pdo, entry)
            {
                struct bus_device *pdo_device = impl_from_bus_device_pdo(pdo);

                devices->Objects[i++] = pdo_device->device;
                ObfReferenceObject(pdo_device->device);
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

static NTSTATUS fdo_pnp(struct bus_device *dev, IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    struct bus_device_fdo *fdo = &dev->u.fdo;
    NTSTATUS ret;

    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, dev->device, code, debugstr_pnp(code));
    switch (code)
    {
        case IRP_MN_START_DEVICE:
            break;

        case IRP_MN_SURPRISE_REMOVAL:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
        {
            DEVICE_OBJECT *pdo = fdo->pdo->device;

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            ret = IoCallDriver(pdo, irp);
            IoDetachDevice(pdo);
            IoDeleteDevice(dev->device);
            return ret;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            struct bus_device_pdo *pdo;
            DEVICE_RELATIONS *devices;
            unsigned int i = 0;

            if (stack->Parameters.QueryDeviceRelations.Type == RemovalRelations
                    || stack->Parameters.QueryDeviceRelations.Type == EjectionRelations)
                break;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
            {
                ok(0, "Unexpected relations type %#x.\n", stack->Parameters.QueryDeviceRelations.Type);
                break;
            }

            ExAcquireFastMutex(&fdo->child_mutex);

            if (!(devices = ExAllocatePool(PagedPool,
                    offsetof(DEVICE_RELATIONS, Objects[list_count(&fdo->children)]))))
            {
                ExReleaseFastMutex(&fdo->child_mutex);
                irp->IoStatus.Status = STATUS_NO_MEMORY;
                break;
            }

            LIST_FOR_EACH_ENTRY(pdo, &fdo->children, struct bus_device_pdo, entry)
            {
                struct bus_device *pdo_device = impl_from_bus_device_pdo(pdo);

                devices->Objects[i++] = pdo_device->device;
                ObfReferenceObject(pdo_device->device);
            }

            ExReleaseFastMutex(&fdo->child_mutex);

            devices->Count = i;
            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(fdo->pdo->device, irp);
}

static ULONG sizeof_multi_sz(const WCHAR *str)
{
    const WCHAR *p;
    for (p = str; *p; p += wcslen(p) + 1);
    return p + 1 - str;
}

static NTSTATUS query_id(struct bus_device_desc *desc, IRP *irp, BUS_QUERY_ID_TYPE type)
{
    const WCHAR *src = NULL;
    WCHAR *id = NULL;
    ULONG size;

    irp->IoStatus.Information = 0;
    switch (type)
    {
        case BusQueryDeviceID:
            if (winetest_debug > 1)
                trace("%s: BusQueryDeviceID.\n", __func__);
            size = (wcslen(desc->device_id_str) + 1) * sizeof(WCHAR);
            src = desc->device_id_str;
            break;

        case BusQueryInstanceID:
            if (winetest_debug > 1)
                trace("%s: BusQueryInstanceID.\n", __func__);
            size = (wcslen(desc->instance_id_str) + 1) * sizeof(WCHAR);
            src = desc->instance_id_str;
            break;

        case BusQueryHardwareIDs:
            if (winetest_debug > 1)
                trace("%s: BusQueryHardwareIDs.\n", __func__);
            size = (sizeof_multi_sz(desc->hardware_ids_str) + 1) * sizeof(WCHAR);
            src = desc->hardware_ids_str;
            break;

        case BusQueryCompatibleIDs:
            if (winetest_debug > 1)
                trace("%s: BusQueryCompatibleIDs.\n", __func__);
            size = (sizeof_multi_sz(desc->compatible_ids_str) + 1) * sizeof(WCHAR);
            src = desc->compatible_ids_str;
            break;

        case BusQueryDeviceSerialNumber:
            if (winetest_debug > 1)
                trace("%s: BusQueryDeviceSerialNumber.\n", __func__);
            if (!wcslen(desc->serial_number_str))
                return STATUS_NOT_SUPPORTED;
            size = (wcslen(desc->serial_number_str) + 1) * sizeof(WCHAR);
            src = desc->serial_number_str;
            break;

        case BusQueryContainerID:
            if (winetest_debug > 1)
                trace("%s: BusQueryContainerID.\n", __func__);
            if (!wcslen(desc->container_id_str))
                return STATUS_NOT_SUPPORTED;

            size = sizeof(desc->container_id_str);
            src = desc->container_id_str;
            break;

        default:
            ok(0, "Unexpected ID query type %#x.\n", type);
            return irp->IoStatus.Status;
    }

    if (!(id = ExAllocatePool(PagedPool, size)))
        return STATUS_NO_MEMORY;

    memcpy(id, src, size);
    irp->IoStatus.Information = (ULONG_PTR)id;
    return STATUS_SUCCESS;
}

static NTSTATUS query_text(struct bus_device_desc *desc, IRP *irp, DEVICE_TEXT_TYPE type, LCID locale)
{
    const WCHAR *src = NULL;
    WCHAR *text = NULL;
    ULONG size;

    switch (type)
    {
        case DeviceTextDescription:
            if (winetest_debug > 1)
                trace("%s: DeviceTextDescription.\n", __func__);
            todo_wine ok(locale, "Expected locale to be set.\n");
            if (!wcslen(desc->text_desc_str))
                return STATUS_NOT_SUPPORTED;
            size = (wcslen(desc->text_desc_str) + 1) * sizeof(WCHAR);
            src = desc->text_desc_str;
            break;

        case DeviceTextLocationInformation:
            if (winetest_debug > 1)
                trace("%s: DeviceTextLocationInformation.\n", __func__);
            todo_wine ok(locale, "Expected locale to be set.\n");
            if (!wcslen(desc->location_info_str))
                return STATUS_NOT_SUPPORTED;
            size = (wcslen(desc->location_info_str) + 1) * sizeof(WCHAR);
            src = desc->location_info_str;
            break;

        default:
            ok(0, "Unexpected device text type %#x.\n", type);
            return irp->IoStatus.Status;
    }

    if (!(text = ExAllocatePool(PagedPool, size)))
        return STATUS_NO_MEMORY;

    memcpy(text, src, size);
    irp->IoStatus.Information = (ULONG_PTR)text;
    return STATUS_SUCCESS;
}

static void get_parent_id_prefix(DEVICE_OBJECT *parent, WCHAR *prefix_out, ULONG prefix_out_size)
{
    static const WCHAR *enum_key_path = L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum";
    WCHAR instance_id[MAX_DEVICE_ID_LEN] = { 0 };
    KEY_VALUE_PARTIAL_INFORMATION *info;
    WCHAR tmp_buf[MAX_PATH] = { 0 };
    OBJECT_ATTRIBUTES attr = { 0 };
    UNICODE_STRING name_str;
    DEVPROPTYPE type;
    NTSTATUS status;
    WCHAR *tmp_ptr;
    HANDLE hkey;
    DWORD size;

    *prefix_out = 0;
    status = IoGetDevicePropertyData(parent, &DEVPKEY_Device_InstanceId, LOCALE_NEUTRAL, 0, sizeof(instance_id),
            instance_id, &size, &type);
    ok(status == STATUS_SUCCESS, "IoGetDevicePropertyData failed: %#lx.\n", status);
    ok(type == DEVPROP_TYPE_STRING, "Unxpected DEVPROPTYPE value %#lx.\n", type);
    if (status != STATUS_SUCCESS)
        return;

    swprintf(tmp_buf, ARRAY_SIZE(tmp_buf), L"%s\\%s", enum_key_path, instance_id);
    RtlInitUnicodeString(&name_str, tmp_buf);
    attr.Length = sizeof(attr);
    attr.ObjectName = &name_str;
    status = ZwOpenKey(&hkey, GENERIC_READ, &attr);
    ok(status == STATUS_SUCCESS, "ZwOpenKey failed: %#lx.\n", status);
    if (!hkey)
        return;

    RtlInitUnicodeString(&name_str, L"ParentIdPrefix");
    status = ZwQueryValueKey(hkey, &name_str, KeyValuePartialInformation, NULL, 0, &size);
    ok(status == STATUS_BUFFER_TOO_SMALL, "Got unexpected status %#lx.\n", status);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        status = ZwClose(hkey);
        ok(status == STATUS_SUCCESS, "ZwClose failed: %#lx.\n", status);
        return;
    }

    size += sizeof(WCHAR);
    info = ExAllocatePool(PagedPool, size);
    ok(!!info, "Failed to allocate memory.\n");
    if (info)
    {
        memset(info, 0, size);
        status = ZwQueryValueKey(hkey, &name_str, KeyValuePartialInformation, info, size, &size);
        ok(status == STATUS_SUCCESS, "ZwQueryValueKey failed: %#lx.\n", status);
        ok(info->Type == REG_SZ, "Expected type REG_SZ, got %lu.\n", info->Type);
        ok(info->DataLength, "Unexpected DataLength %lu.\n", info->DataLength);

        if (info->Type == REG_SZ && info->DataLength > sizeof(WCHAR))
        {
            tmp_ptr = (WCHAR *)info->Data;
            if (tmp_ptr[(info->DataLength / sizeof(WCHAR))])
                tmp_ptr[(info->DataLength / sizeof(WCHAR)) + 1] = 0;
            swprintf(prefix_out, prefix_out_size, L"%s", (WCHAR *)info->Data);
        }
        ExFreePool(info);
    }

    status = ZwClose(hkey);
    ok(status == STATUS_SUCCESS, "ZwClose failed: %#lx.\n", status);
}

static NTSTATUS pdo_pnp(struct bus_device *dev, IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    struct bus_device_pdo *pdo = &dev->u.pdo;
    struct bus_device_desc *desc = &pdo->desc;
    NTSTATUS ret = irp->IoStatus.Status;

    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, dev->device, code, debugstr_pnp(code));
    switch (code)
    {
        case IRP_MN_QUERY_ID:
            ret = query_id(desc, irp, stack->Parameters.QueryId.IdType);
            break;

        case IRP_MN_START_DEVICE:
        {
            WCHAR parent_id_prefix[MAX_DEVICE_ID_LEN] = { 0 };
            WCHAR instance_id[MAX_DEVICE_ID_LEN] = { 0 };
            GUID iface_guid = control_class2;
            WCHAR expect_symlink[MAX_PATH];
            WCHAR device_id[64];
            NTSTATUS status;
            WCHAR *tmp;
            int offset;

            ok(!stack->Parameters.StartDevice.AllocatedResources, "Expected no resources.\n");
            ok(!stack->Parameters.StartDevice.AllocatedResourcesTranslated, "Expected no translated resources.\n");

            iface_guid.Data4[7] += dev->depth + 1;
            wcscpy(device_id, desc->device_id_str);
            while ((tmp = wcschr(device_id, '\\')))
                *tmp = '#';

            if (!desc->unique_id)
            {
                if (pdo->parent)
                    get_parent_id_prefix(pdo->parent->u.fdo.pdo->device, parent_id_prefix, ARRAY_SIZE(parent_id_prefix));
                else
                    get_parent_id_prefix(bus_pdo, parent_id_prefix, ARRAY_SIZE(parent_id_prefix));
            }

            if (parent_id_prefix[0])
                swprintf(instance_id, ARRAY_SIZE(instance_id), L"%s&%s", parent_id_prefix, desc->instance_id_str);
            else
                swprintf(instance_id, ARRAY_SIZE(instance_id), L"%s", desc->instance_id_str);

            offset = swprintf(expect_symlink, ARRAY_SIZE(expect_symlink), L"\\??\\%s#%s", device_id, instance_id);
            swprintf(&expect_symlink[offset], ARRAY_SIZE(expect_symlink) - offset,
                    L"#{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", iface_guid.Data1, iface_guid.Data2,
                    iface_guid.Data3, iface_guid.Data4[0], iface_guid.Data4[1], iface_guid.Data4[2],
                    iface_guid.Data4[3], iface_guid.Data4[4], iface_guid.Data4[5], iface_guid.Data4[6],
                    iface_guid.Data4[7]);

            status = IoRegisterDeviceInterface(dev->device, &iface_guid, NULL, &pdo->iface_symlink);
            ok(!status, "Failed to register interface, status %#lx.\n", status);
            ok(pdo->iface_symlink.Length == (wcslen(expect_symlink) * 2),
                    "Got length %u.\n", pdo->iface_symlink.Length);
            ok(pdo->iface_symlink.MaximumLength == ((wcslen(expect_symlink) + 1) * 2),
                    "Got maximum length %u.\n", pdo->iface_symlink.MaximumLength);
            ok(!kmemcmp(pdo->iface_symlink.Buffer, expect_symlink, pdo->iface_symlink.MaximumLength),
                    "Got symlink \"%ls\".\n", pdo->iface_symlink.Buffer);
            IoSetDeviceInterfaceState(&pdo->iface_symlink, TRUE);
            ret = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
            if (pdo->removed)
            {
                IoSetDeviceInterfaceState(&pdo->iface_symlink, FALSE);
                RtlFreeUnicodeString(&pdo->iface_symlink);
                irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                IoDeleteDevice(dev->device);
                return STATUS_SUCCESS;
            }

            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = stack->Parameters.DeviceCapabilities.Capabilities;
            unsigned int i;

            ok(caps->Size == sizeof(*caps), "Wrong size %u.\n", caps->Size);
            ok(caps->Version == 1, "Wrong version %u.\n", caps->Version);
            ok(!caps->LockSupported, "Got LockSupported %u.\n", caps->LockSupported);
            ok(!caps->EjectSupported, "Got EjectSupported %u.\n", caps->EjectSupported);
            ok(!caps->Removable, "Got Removable %u.\n", caps->Removable);
            ok(!caps->UniqueID, "Got UniqueID %u.\n", caps->UniqueID);
            ok(!caps->RawDeviceOK, "Got RawDeviceOK %u.\n", caps->RawDeviceOK);
            ok(!caps->SurpriseRemovalOK, "Got SurpriseRemovalOK %u.\n", caps->SurpriseRemovalOK);
            ok(caps->Address == 0xffffffff, "Got Address %#lx.\n", caps->Address);
            ok(caps->UINumber == 0xffffffff, "Got UINumber %#lx.\n", caps->UINumber);
            ok(!caps->WakeFromD0, "Got WakeFromD0 %u.\n", caps->WakeFromD0);
            ok(!caps->WakeFromD1, "Got WakeFromD1 %u.\n", caps->WakeFromD1);
            ok(!caps->WakeFromD2, "Got WakeFromD2 %u.\n", caps->WakeFromD2);
            ok(!caps->WakeFromD3, "Got WakeFromD3 %u.\n", caps->WakeFromD3);
            ok(!caps->HardwareDisabled, "Got HardwareDisabled %u.\n", caps->HardwareDisabled);
            ok(!caps->NonDynamic, "Got NonDynamic %u.\n", caps->NonDynamic);
            ok(!caps->WarmEjectSupported, "Got WarmEjectSupported %u.\n", caps->WarmEjectSupported);
            ok(!caps->NoDisplayInUI, "Got NoDisplayInUI %u.\n", caps->NoDisplayInUI);
            ok(caps->Address == 0xffffffff, "Got Address %#lx.\n", caps->Address);
            ok(caps->UINumber == 0xffffffff, "Got UINumber %#lx.\n", caps->UINumber);
            for (i = 0; i < PowerSystemMaximum; ++i)
                ok(caps->DeviceState[i] == PowerDeviceUnspecified, "Got DeviceState[%u] %u.\n", i, caps->DeviceState[i]);
            ok(caps->SystemWake == PowerSystemUnspecified, "Got SystemWake %u.\n", caps->SystemWake);
            ok(caps->DeviceWake == PowerDeviceUnspecified, "Got DeviceWake %u.\n", caps->DeviceWake);
            ok(!caps->D1Latency, "Got D1Latency %lu.\n", caps->D1Latency);
            ok(!caps->D2Latency, "Got D2Latency %lu.\n", caps->D2Latency);
            ok(!caps->D3Latency, "Got D3Latency %lu.\n", caps->D3Latency);

            caps->RawDeviceOK = 0;
            caps->SurpriseRemovalOK = 1;
            caps->EjectSupported = 1;
            caps->Removable = desc->removable;
            caps->UniqueID = desc->unique_id;
            caps->Address = desc->address;
            caps->UINumber = desc->ui_number;

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
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            ret = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            ret = query_text(desc, irp, stack->Parameters.QueryDeviceText.DeviceTextType,
                    stack->Parameters.QueryDeviceText.LocaleId);
            if (ret == STATUS_NOT_SUPPORTED)
                ret = irp->IoStatus.Status;
            break;
    }

    irp->IoStatus.Status = ret;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ret;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack;
    struct bus_device *dev;
    ULONG code;

    if (device == bus_fdo)
        return bus_fdo_pnp(irp);

    stack = IoGetCurrentIrpStackLocation(irp);
    dev = device->DeviceExtension;
    code = stack->MinorFunction;
    if (dev->type == BUS_DEVICE_TYPE_FDO)
        return fdo_pnp(dev, irp, stack, code);
    else
        return pdo_pnp(dev, irp, stack, code);
}

/*
 * This value is used to create unique device file name strings. If we reuse
 * the same file name strings and quickly create/delete devices, it leads to
 * occasional bugchecks.
 */
static unsigned int dev_idx;
static NTSTATUS bus_fdo_ioctl(IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, bus_fdo, code, debugstr_ioctl_bus(code));
    switch (code)
    {
        case IOCTL_WINETEST_BUS_ADD_CHILD:
        {
            ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
            struct bus_device_desc desc;
            struct bus_device *device;
            DEVICE_OBJECT *device_obj;
            UNICODE_STRING string;
            NTSTATUS status;
            WCHAR name[74];

            if (in_size < sizeof(desc))
                return STATUS_INVALID_PARAMETER;

            desc = *(struct bus_device_desc *)irp->AssociatedIrp.SystemBuffer;
            swprintf(name, ARRAY_SIZE(name), L"\\Device\\winetest_pnp_child_%d", dev_idx++);
            RtlInitUnicodeString(&string, name);
            status = IoCreateDevice(driver_obj, sizeof(*device), &string, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj);
            ok(!status, "Failed to create device, status %#lx.\n", status);

            device = device_obj->DeviceExtension;
            memset(device, 0, sizeof(*device));
            device->type = BUS_DEVICE_TYPE_PDO;
            device->depth = 0;
            device->device = device_obj;
            device->u.pdo.desc = desc;

            ExAcquireFastMutex(&driver_lock);
            list_add_tail(&device_list, &device->u.pdo.entry);
            ExReleaseFastMutex(&driver_lock);

            device_obj->Flags &= ~DO_DEVICE_INITIALIZING;

            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            return STATUS_SUCCESS;
        }

        case IOCTL_WINETEST_BUS_REMOVE_CHILD:
        {
            struct bus_device_pdo *device;
            WCHAR *in_name;

            if (stack->Parameters.DeviceIoControl.InputBufferLength < (sizeof(WCHAR) * 64))
                return STATUS_BUFFER_TOO_SMALL;
            in_name = irp->AssociatedIrp.SystemBuffer;

            ExAcquireFastMutex(&driver_lock);
            LIST_FOR_EACH_ENTRY(device, &device_list, struct bus_device_pdo, entry)
            {
                if (!wcscmp(device->desc.dev_name, in_name))
                {
                    list_remove(&device->entry);
                    device->removed = TRUE;
                    break;
                }
            }
            ExReleaseFastMutex(&driver_lock);

            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            return STATUS_SUCCESS;
        }

        default:
            ok(0, "Unexpected ioctl %#lx.\n", code);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS fdo_ioctl(struct bus_device *device, IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, device->device, code, debugstr_ioctl_bus_child(code));
    switch (code)
    {
        case IOCTL_WINETEST_CHILD_ADD_CHILD:
        {
            ULONG in_size = stack->Parameters.DeviceIoControl.InputBufferLength;
            struct bus_device *child_device;
            struct bus_device_desc desc;
            DEVICE_OBJECT *device_obj;
            UNICODE_STRING string;
            NTSTATUS status;
            WCHAR name[74];

            if (in_size < sizeof(desc))
                return STATUS_INVALID_PARAMETER;

            desc = *(struct bus_device_desc *)irp->AssociatedIrp.SystemBuffer;
            swprintf(name, ARRAY_SIZE(name), L"\\Device\\winetest_pnp_child_%d", dev_idx++);
            RtlInitUnicodeString(&string, name);
            status = IoCreateDevice(driver_obj, sizeof(*child_device), &string, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj);
            ok(!status, "Failed to create device, status %#lx.\n", status);

            child_device = device_obj->DeviceExtension;
            memset(child_device, 0, sizeof(*child_device));
            child_device->type = BUS_DEVICE_TYPE_PDO;
            child_device->depth = device->depth + 1;
            child_device->device = device_obj;
            child_device->u.pdo.desc = desc;
            child_device->u.pdo.parent = device;

            ExAcquireFastMutex(&device->u.fdo.child_mutex);
            list_add_tail(&device->u.fdo.children, &child_device->u.pdo.entry);
            ExReleaseFastMutex(&device->u.fdo.child_mutex);

            device_obj->Flags &= ~DO_DEVICE_INITIALIZING;

            IoInvalidateDeviceRelations(device->u.fdo.pdo->device, BusRelations);
            return STATUS_SUCCESS;
        }

        case IOCTL_WINETEST_CHILD_REMOVE_CHILD:
        {
            struct bus_device_pdo *child_device;
            WCHAR *in_name;

            if (stack->Parameters.DeviceIoControl.InputBufferLength < (sizeof(WCHAR) * 64))
                return STATUS_BUFFER_TOO_SMALL;
            in_name = irp->AssociatedIrp.SystemBuffer;

            ExAcquireFastMutex(&device->u.fdo.child_mutex);
            LIST_FOR_EACH_ENTRY(child_device, &device->u.fdo.children, struct bus_device_pdo, entry)
            {
                if (!wcscmp(child_device->desc.dev_name, in_name))
                {
                    list_remove(&child_device->entry);
                    child_device->removed = TRUE;
                    break;
                }
            }
            ExReleaseFastMutex(&device->u.fdo.child_mutex);

            IoInvalidateDeviceRelations(device->u.fdo.pdo->device, BusRelations);
            return STATUS_SUCCESS;
        }

        case IOCTL_WINETEST_CHILD_GET_ID:
        {
            struct bus_device_pdo *pdo = &device->u.fdo.pdo->u.pdo;

            if (stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(pdo->desc.dev_name))
                return STATUS_BUFFER_TOO_SMALL;

            wcscpy(irp->AssociatedIrp.SystemBuffer, pdo->desc.dev_name);
            irp->IoStatus.Information = sizeof(pdo->desc.dev_name);
            return STATUS_SUCCESS;
        }

        case IOCTL_WINETEST_CHILD_MAIN:
            return STATUS_SUCCESS;

        default:
            ok(0, "Unexpected ioctl %#lx.\n", code);
            return STATUS_NOT_IMPLEMENTED;
    }
}

static NTSTATUS pdo_ioctl(struct bus_device *device, IRP *irp, IO_STACK_LOCATION *stack, ULONG code)
{
    if (winetest_debug > 1)
        trace("%s: device %p, code %#lx %s\n", __func__, device, code, debugstr_ioctl_bus_child(code));
    ok(0, "Unexpected ioctl %#lx.\n", code);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;

    if (device == bus_fdo)
        status = bus_fdo_ioctl(irp, stack, code);
    else
    {
        struct bus_device *dev = device->DeviceExtension;

        if (dev->type == BUS_DEVICE_TYPE_FDO)
            status = fdo_ioctl(dev, irp, stack, code);
        else
            status = pdo_ioctl(dev, irp, stack, code);
    }
    irp->IoStatus.Status = status;
    if (status != STATUS_PENDING) IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *pdo)
{
    DEVICE_OBJECT *fdo;
    NTSTATUS ret;

    if (winetest_debug > 1)
        trace("%s: driver %p, pdo %p.\n", __func__, driver, pdo);
    if (!bus_fdo)
    {
        if ((ret = IoCreateDevice(driver, 0, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &fdo)))
            return ret;

        if ((ret = IoRegisterDeviceInterface(pdo, &control_class2, NULL, &control_symlink)))
        {
            IoDeleteDevice(fdo);
            return ret;
        }

        bus_pdo = pdo;
        bus_fdo = fdo;
    }
    else
    {
        struct bus_device *device = pdo->DeviceExtension;
        struct bus_device *fdo_device;

        if ((ret = IoCreateDevice(driver, sizeof(*fdo_device), NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &fdo)))
            return ret;

        fdo_device = fdo->DeviceExtension;
        fdo_device->type = BUS_DEVICE_TYPE_FDO;
        fdo_device->depth = device->depth;
        fdo_device->device = fdo;
        fdo_device->u.fdo.pdo = device;
        ExInitializeFastMutex(&fdo_device->u.fdo.child_mutex);
        list_init(&fdo_device->u.fdo.children);
    }

    IoAttachDeviceToDeviceStack(fdo, pdo);
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    if (winetest_debug > 1)
        trace("%s: device %p.\n", __func__, device);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    if (winetest_debug > 1)
        trace("%s: device %p.\n", __func__, device);
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    if (winetest_debug > 1)
        trace("%s: driver %p.\n", __func__, driver);
    winetest_cleanup();
}

static NTSTATUS WINAPI driver_power(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret = STATUS_NOT_SUPPORTED;

    if (winetest_debug > 1)
        trace("%s: device %p.\n", __func__, device);
    /* We do not expect power IRPs as part of normal operation. */
    ok(0, "Unexpected call.\n");

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

NTSTATUS WINAPI DriverEntry(DRIVER_OBJECT *driver, UNICODE_STRING *registry)
{
    NTSTATUS ret;

    if (winetest_debug > 1)
        trace("%s: driver %p.\n", __func__, driver);
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
