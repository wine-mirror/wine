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
#include "winreg.h"

#include "cfgmgr32.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "ddk/hidpddi.h"
#include "ddk/hidtypes.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xinput);

#ifdef __ASM_USE_FASTCALL_WRAPPER
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

#include "psh_hid_macros.h"

const BYTE xinput_report_desc[] =
{
    USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
    USAGE(1, HID_USAGE_GENERIC_GAMEPAD),
    COLLECTION(1, Application),
        USAGE(1, 0),
        COLLECTION(1, Physical),
            USAGE(1, HID_USAGE_GENERIC_X),
            USAGE(1, HID_USAGE_GENERIC_Y),
            LOGICAL_MAXIMUM(2, 0xffff),
            PHYSICAL_MAXIMUM(2, 0xffff),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 2),
            INPUT(1, Data|Var|Abs),
        END_COLLECTION,

        COLLECTION(1, Physical),
            USAGE(1, HID_USAGE_GENERIC_RX),
            USAGE(1, HID_USAGE_GENERIC_RY),
            REPORT_COUNT(1, 2),
            INPUT(1, Data|Var|Abs),
        END_COLLECTION,

        COLLECTION(1, Physical),
            USAGE(1, HID_USAGE_GENERIC_Z),
            REPORT_COUNT(1, 1),
            INPUT(1, Data|Var|Abs),
        END_COLLECTION,

        USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
        USAGE_MINIMUM(1, 1),
        USAGE_MAXIMUM(1, 10),
        LOGICAL_MAXIMUM(1, 1),
        PHYSICAL_MAXIMUM(1, 1),
        REPORT_COUNT(1, 10),
        REPORT_SIZE(1, 1),
        INPUT(1, Data|Var|Abs),

        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
        LOGICAL_MINIMUM(1, 1),
        LOGICAL_MAXIMUM(1, 8),
        PHYSICAL_MAXIMUM(2, 0x103b),
        REPORT_SIZE(1, 4),
        REPORT_COUNT(4, 1),
        UNIT(1, 0x0e /* none */),
        INPUT(1, Data|Var|Abs|Null),

        REPORT_COUNT(1, 18),
        REPORT_SIZE(1, 1),
        INPUT(1, Cnst|Var|Abs),
    END_COLLECTION,
};

#include "pop_hid_macros.h"

struct xinput_state
{
    WORD lx_axis;
    WORD ly_axis;
    WORD rx_axis;
    WORD ry_axis;
    WORD trigger;
    WORD buttons;
    WORD padding;
};

struct device
{
    BOOL is_fdo;
    BOOL is_gamepad;
    LONG removed;
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

    /* the Wine-specific hidden HID device, used by XInput */
    DEVICE_OBJECT *xinput_device;

    WCHAR instance_id[MAX_DEVICE_ID_LEN];

    HIDP_VALUE_CAPS lx_caps;
    HIDP_VALUE_CAPS ly_caps;
    HIDP_VALUE_CAPS lt_caps;
    HIDP_VALUE_CAPS rx_caps;
    HIDP_VALUE_CAPS ry_caps;
    HIDP_VALUE_CAPS rt_caps;
    HIDP_DEVICE_DESC device_desc;

    /* everything below requires holding the cs */
    CRITICAL_SECTION cs;
    ULONG report_len;
    char *report_buf;
    IRP *pending_read;
    BOOL pending_is_gamepad;
    struct xinput_state xinput_state;
};

static inline struct func_device *fdo_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    if (impl->is_fdo) return CONTAINING_RECORD(impl, struct func_device, base);
    else return CONTAINING_RECORD(impl, struct phys_device, base)->fdo;
}

static LONG sign_extend(ULONG value, const HIDP_VALUE_CAPS *caps)
{
    UINT sign = 1 << (caps->BitSize - 1);
    if (sign <= 1 || caps->LogicalMin >= 0) return value;
    return value - ((value & sign) << 1);
}

static LONG scale_value(ULONG value, const HIDP_VALUE_CAPS *caps, LONG min, LONG max)
{
    LONG tmp = sign_extend(value, caps);
    if (caps->LogicalMin > caps->LogicalMax) return 0;
    if (caps->LogicalMin > tmp || caps->LogicalMax < tmp) return 0;
    return min + MulDiv(tmp - caps->LogicalMin, max - min, caps->LogicalMax - caps->LogicalMin);
}

static void translate_report_to_xinput_state(struct func_device *fdo)
{
    ULONG lx = 0, ly = 0, rx = 0, ry = 0, lt = 0, rt = 0, hat = 0;
    PHIDP_PREPARSED_DATA preparsed;
    USAGE usages[10];
    NTSTATUS status;
    ULONG i, count;

    preparsed = fdo->device_desc.CollectionDesc->PreparsedData;

    count = ARRAY_SIZE(usages);
    status = HidP_GetUsages(HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages,
                            &count, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsages returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                &hat, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue hat returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                &lx, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue x returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Y,
                                &ly, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue y returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                &lt, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue z returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX,
                                &rx, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue rx returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                &ry, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue ry returned %#lx\n", status);
    status = HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RZ,
                                &rt, preparsed, fdo->report_buf, fdo->report_len);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetUsageValue rz returned %#lx\n", status);

    if (hat < 1 || hat > 8) fdo->xinput_state.buttons = 0;
    else fdo->xinput_state.buttons = hat << 10;
    for (i = 0; i < count; i++)
    {
        if (usages[i] < 1 || usages[i] > 10) continue;
        fdo->xinput_state.buttons |= (1 << (usages[i] - 1));
    }
    fdo->xinput_state.lx_axis = scale_value(lx, &fdo->lx_caps, 0, 65535);
    fdo->xinput_state.ly_axis = scale_value(ly, &fdo->ly_caps, 0, 65535);
    fdo->xinput_state.rx_axis = scale_value(rx, &fdo->rx_caps, 0, 65535);
    fdo->xinput_state.ry_axis = scale_value(ry, &fdo->ry_caps, 0, 65535);
    rt = scale_value(rt, &fdo->rt_caps, 0, 255);
    lt = scale_value(lt, &fdo->lt_caps, 0, 255);
    fdo->xinput_state.trigger = 0x8000 + (lt - rt) * 128;
}

static NTSTATUS WINAPI read_completion(DEVICE_OBJECT *device, IRP *xinput_irp, void *context)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(xinput_irp);
    ULONG offset, read_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    char *read_buf = xinput_irp->UserBuffer;
    IRP *gamepad_irp = context;

    gamepad_irp->IoStatus.Status = xinput_irp->IoStatus.Status;
    gamepad_irp->IoStatus.Information = xinput_irp->IoStatus.Information;

    if (!xinput_irp->IoStatus.Status)
    {
        RtlEnterCriticalSection(&fdo->cs);
        offset = fdo->report_buf[0] ? 0 : 1;
        memcpy(fdo->report_buf + offset, read_buf, read_len);
        translate_report_to_xinput_state(fdo);
        memcpy(gamepad_irp->UserBuffer, &fdo->xinput_state, sizeof(fdo->xinput_state));
        gamepad_irp->IoStatus.Information = sizeof(fdo->xinput_state);
        RtlLeaveCriticalSection(&fdo->cs);
    }

    IoCompleteRequest(gamepad_irp, IO_NO_INCREMENT);
    if (xinput_irp->PendingReturned) IoMarkIrpPending(xinput_irp);
    return STATUS_SUCCESS;
}

/* check for a pending read from the other PDO, and complete both at a time.
 * if there's none, save irp as pending, the other PDO will complete it.
 * if the device is being removed, complete irp with an error. */
static NTSTATUS try_complete_pending_read(DEVICE_OBJECT *device, IRP *irp)
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    IRP *pending, *xinput_irp, *gamepad_irp;
    BOOL removed, pending_is_gamepad;

    RtlEnterCriticalSection(&fdo->cs);
    pending_is_gamepad = fdo->pending_is_gamepad;
    if ((removed = impl->removed))
        pending = NULL;
    else if ((pending = fdo->pending_read))
        fdo->pending_read = NULL;
    else
    {
        fdo->pending_read = irp;
        fdo->pending_is_gamepad = impl->is_gamepad;
        IoMarkIrpPending(irp);
    }
    RtlLeaveCriticalSection(&fdo->cs);

    if (removed)
    {
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    if (!pending) return STATUS_PENDING;

    /* only one read at a time per device from hidclass.sys design */
    if (pending_is_gamepad == impl->is_gamepad) ERR("multiple read requests!\n");
    gamepad_irp = impl->is_gamepad ? irp : pending;
    xinput_irp = impl->is_gamepad ? pending : irp;

    /* pass xinput irp down, and complete gamepad irp on its way back */
    IoCopyCurrentIrpStackLocationToNext(xinput_irp);
    IoSetCompletionRoutine(xinput_irp, read_completion, gamepad_irp, TRUE, TRUE, TRUE);
    return IoCallDriver(fdo->bus_device, xinput_irp);
}

static NTSTATUS WINAPI gamepad_internal_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    ULONG output_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);

    TRACE("device %p, irp %p, code %#lx, bus_device %p.\n", device, irp, code, fdo->bus_device);

    switch (code)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
    {
        HID_DESCRIPTOR *descriptor = (HID_DESCRIPTOR *)irp->UserBuffer;

        irp->IoStatus.Information = sizeof(*descriptor);
        if (output_len < sizeof(*descriptor))
        {
            irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_BUFFER_TOO_SMALL;
        }

        memset(descriptor, 0, sizeof(*descriptor));
        descriptor->bLength = sizeof(*descriptor);
        descriptor->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
        descriptor->bcdHID = HID_REVISION;
        descriptor->bCountry = 0;
        descriptor->bNumDescriptors = 1;
        descriptor->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
        descriptor->DescriptorList[0].wReportLength = sizeof(xinput_report_desc);

        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        irp->IoStatus.Information = sizeof(xinput_report_desc);
        if (output_len < sizeof(xinput_report_desc))
        {
            irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            return STATUS_BUFFER_TOO_SMALL;
        }

        memcpy(irp->UserBuffer, xinput_report_desc, sizeof(xinput_report_desc));
        irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;

    case IOCTL_HID_GET_INPUT_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_SET_FEATURE:
        irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;

    default:
        IoSkipCurrentIrpStackLocation(irp);
        return IoCallDriver(fdo->bus_device, irp);
    }

    return STATUS_SUCCESS;
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

    TRACE("device %p, irp %p, code %#lx, bus_device %p.\n", device, irp, code, fdo->bus_device);

    if (code == IOCTL_HID_READ_REPORT) return try_complete_pending_read(device, irp);
    if (impl->is_gamepad) return gamepad_internal_ioctl(device, irp);

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

static void remove_pending_irps(DEVICE_OBJECT *device)
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    IRP *pending;

    RtlEnterCriticalSection(&fdo->cs);
    pending = fdo->pending_read;
    fdo->pending_read = NULL;
    RtlLeaveCriticalSection(&fdo->cs);

    if (pending)
    {
        pending->IoStatus.Status = STATUS_DELETE_PENDING;
        pending->IoStatus.Information = 0;
        IoCompleteRequest(pending, IO_NO_INCREMENT);
    }
}

static NTSTATUS WINAPI pdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    struct device *impl = impl_from_DEVICE_OBJECT(device);
    UCHAR code = stack->MinorFunction;
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
        remove_pending_irps(device);
        break;

    case IRP_MN_REMOVE_DEVICE:
        remove_pending_irps(device);
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
            WARN("IRP_MN_QUERY_ID type %u, not implemented!\n", type);
            status = irp->IoStatus.Status;
            break;
        }
        break;
    }

    case IRP_MN_QUERY_CAPABILITIES:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        status = irp->IoStatus.Status;
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
    DEVICE_OBJECT *gamepad_device, *xinput_device;
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
        ERR("failed to create gamepad device, status %#lx.\n", status);
        return status;
    }

    swprintf(name, ARRAY_SIZE(name), L"\\Device\\WINEXINPUT#%p&%p&1",
             device->DriverObject, fdo->bus_device);
    RtlInitUnicodeString(&name_str, name);

    if ((status = IoCreateDevice(device->DriverObject, sizeof(struct phys_device),
                                 &name_str, 0, 0, FALSE, &xinput_device)))
    {
        ERR("failed to create xinput device, status %#lx.\n", status);
        IoDeleteDevice(gamepad_device);
        return status;
    }

    fdo->gamepad_device = gamepad_device;
    pdo = gamepad_device->DeviceExtension;
    pdo->fdo = fdo;
    pdo->base.is_fdo = FALSE;
    pdo->base.is_gamepad = TRUE;
    wcscpy(pdo->base.device_id, fdo->base.device_id);

    if ((tmp = wcsstr(pdo->base.device_id, L"&MI_"))) memcpy(tmp, L"&IG", 6);
    else wcscat(pdo->base.device_id, L"&IG_00");

    TRACE("device %p, gamepad device %p.\n", device, gamepad_device);

    fdo->xinput_device = xinput_device;
    pdo = xinput_device->DeviceExtension;
    pdo->fdo = fdo;
    pdo->base.is_fdo = FALSE;
    pdo->base.is_gamepad = FALSE;
    wcscpy(pdo->base.device_id, fdo->base.device_id);

    if ((tmp = wcsstr(pdo->base.device_id, L"&MI_"))) memcpy(tmp, L"&XI", 6);
    else wcscat(pdo->base.device_id, L"&XI_00");

    TRACE("device %p, xinput device %p.\n", device, xinput_device);

    IoInvalidateDeviceRelations(fdo->bus_device, BusRelations);
    return STATUS_SUCCESS;
}

static NTSTATUS sync_ioctl(DEVICE_OBJECT *device, DWORD code, void *in_buf, DWORD in_len, void *out_buf, DWORD out_len)
{
    IO_STATUS_BLOCK io;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(code, device, in_buf, in_len, out_buf, out_len, TRUE, &event, &io);
    if (IoCallDriver(device, irp) == STATUS_PENDING) KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    return io.Status;
}

static void check_value_caps(struct func_device *fdo, USHORT usage, HIDP_VALUE_CAPS *caps)
{
    switch (usage)
    {
    case HID_USAGE_GENERIC_X: fdo->lx_caps = *caps; break;
    case HID_USAGE_GENERIC_Y: fdo->ly_caps = *caps; break;
    case HID_USAGE_GENERIC_Z: fdo->lt_caps = *caps; break;
    case HID_USAGE_GENERIC_RX: fdo->rx_caps = *caps; break;
    case HID_USAGE_GENERIC_RY: fdo->ry_caps = *caps; break;
    case HID_USAGE_GENERIC_RZ: fdo->rt_caps = *caps; break;
    }
}

static NTSTATUS initialize_device(DEVICE_OBJECT *device)
{
    struct func_device *fdo = fdo_from_DEVICE_OBJECT(device);
    UINT i, u, button_count, report_desc_len, report_count;
    PHIDP_REPORT_DESCRIPTOR report_desc;
    PHIDP_PREPARSED_DATA preparsed;
    HIDP_BUTTON_CAPS *button_caps;
    HID_DESCRIPTOR hid_desc = {0};
    HIDP_VALUE_CAPS *value_caps;
    HIDP_REPORT_IDS *reports;
    NTSTATUS status;
    HIDP_CAPS caps;

    if ((status = sync_ioctl(fdo->bus_device, IOCTL_HID_GET_DEVICE_DESCRIPTOR, NULL, 0, &hid_desc, sizeof(hid_desc))))
        return status;

    if (!(report_desc_len = hid_desc.DescriptorList[0].wReportLength)) return STATUS_UNSUCCESSFUL;
    if (!(report_desc = malloc(report_desc_len))) return STATUS_NO_MEMORY;

    status = sync_ioctl(fdo->bus_device, IOCTL_HID_GET_REPORT_DESCRIPTOR, NULL, 0, report_desc, report_desc_len);
    if (!status) status = HidP_GetCollectionDescription(report_desc, report_desc_len, PagedPool, &fdo->device_desc);
    free(report_desc);
    if (status != HIDP_STATUS_SUCCESS) return status;

    preparsed = fdo->device_desc.CollectionDesc->PreparsedData;
    status = HidP_GetCaps(preparsed, &caps);
    if (status != HIDP_STATUS_SUCCESS) return status;

    button_count = 0;
    if (!(button_caps = malloc(sizeof(*button_caps) * caps.NumberInputButtonCaps))) return STATUS_NO_MEMORY;
    status = HidP_GetButtonCaps(HidP_Input, button_caps, &caps.NumberInputButtonCaps, preparsed);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetButtonCaps returned %#lx\n", status);
    else for (i = 0; i < caps.NumberInputButtonCaps; i++)
    {
        if (button_caps[i].UsagePage != HID_USAGE_PAGE_BUTTON) continue;
        if (button_caps[i].IsRange) button_count = max(button_count, button_caps[i].Range.UsageMax);
        else button_count = max(button_count, button_caps[i].NotRange.Usage);
    }
    free(button_caps);
    if (status != HIDP_STATUS_SUCCESS) return status;
    if (button_count < 10) WARN("only %u buttons found\n", button_count);

    if (!(value_caps = malloc(sizeof(*value_caps) * caps.NumberInputValueCaps))) return STATUS_NO_MEMORY;
    status = HidP_GetValueCaps(HidP_Input, value_caps, &caps.NumberInputValueCaps, preparsed);
    if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetValueCaps returned %#lx\n", status);
    else for (i = 0; i < caps.NumberInputValueCaps; i++)
    {
        HIDP_VALUE_CAPS *caps = value_caps + i;
        if (caps->UsagePage != HID_USAGE_PAGE_GENERIC) continue;
        if (!caps->IsRange) check_value_caps(fdo, caps->NotRange.Usage, caps);
        else for (u = caps->Range.UsageMin; u <=caps->Range.UsageMax; u++) check_value_caps(fdo, u, value_caps + i);
    }
    free(value_caps);
    if (status != HIDP_STATUS_SUCCESS) return status;
    if (!fdo->lx_caps.UsagePage) WARN("missing lx axis\n");
    if (!fdo->ly_caps.UsagePage) WARN("missing ly axis\n");
    if (!fdo->lt_caps.UsagePage) WARN("missing lt axis\n");
    if (!fdo->rx_caps.UsagePage) WARN("missing rx axis\n");
    if (!fdo->ry_caps.UsagePage) WARN("missing ry axis\n");
    if (!fdo->rt_caps.UsagePage) WARN("missing rt axis\n");

    reports = fdo->device_desc.ReportIDs;
    report_count = fdo->device_desc.ReportIDsLength;
    for (i = 0; i < report_count; ++i) if (!reports[i].ReportID || reports[i].InputLength) break;
    if (i == report_count) i = 0; /* no input report?!, just use first ID */

    fdo->report_len = caps.InputReportByteLength;
    if (!(fdo->report_buf = malloc(fdo->report_len))) return STATUS_NO_MEMORY;
    fdo->report_buf[0] = reports[i].ReportID;

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
    UCHAR code = stack->MinorFunction;
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
            if ((child = fdo->xinput_device))
            {
                devices->Objects[devices->Count] = child;
                call_fastcall_func1(ObfReferenceObject, child);
                devices->Count++;
            }
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
        if (!status) status = initialize_device(device);
        if (!status) status = create_child_pdos(device);

        if (status) irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return status;

    case IRP_MN_REMOVE_DEVICE:
        IoSkipCurrentIrpStackLocation(irp);
        status = IoCallDriver(fdo->bus_device, irp);
        IoDetachDevice(fdo->bus_device);
        if (fdo->cs.DebugInfo)
            fdo->cs.DebugInfo->Spare[0] = 0;
        RtlDeleteCriticalSection(&fdo->cs);
        HidP_FreeCollectionDescription(&fdo->device_desc);
        free(fdo->report_buf);
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
        ERR("failed to get bus device id, status %#lx.\n", status);
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
        ERR("failed to get bus device instance id, status %#lx.\n", status);
        return status;
    }

    if ((status = IoCreateDevice(driver, sizeof(struct func_device), NULL,
                                 FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &device)))
    {
        ERR("failed to create bus FDO, status %#lx.\n", status);
        return status;
    }

    fdo = device->DeviceExtension;
    fdo->base.is_fdo = TRUE;
    swprintf(fdo->base.device_id, MAX_DEVICE_ID_LEN, L"WINEXINPUT\\%s", device_id);

    fdo->bus_device = bus_device;
    wcscpy(fdo->instance_id, instance_id);

    RtlInitializeCriticalSectionEx(&fdo->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    fdo->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": func_device.cs");

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
