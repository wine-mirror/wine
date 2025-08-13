/*
 * WINE Platform native bus driver
 *
 * Copyright 2016 Aric Stewart
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
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "winioctl.h"
#include "hidusage.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "ddk/hidtypes.h"
#include "ddk/hidpddi.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

static DRIVER_OBJECT *driver_obj;

static DEVICE_OBJECT *mouse_obj;
static DEVICE_OBJECT *keyboard_obj;

/* The root-enumerated device stack. */
static DEVICE_OBJECT *bus_pdo;
static DEVICE_OBJECT *bus_fdo;

static struct bus_options options = {.devices = LIST_INIT(options.devices)};
static HANDLE driver_key;

struct hid_report
{
    struct list entry;
    ULONG length;
    BYTE buffer[1];
};

enum device_state
{
    DEVICE_STATE_STOPPED,
    DEVICE_STATE_STARTED,
    DEVICE_STATE_REMOVED,
};

#define HIDRAW_FIXUP_DUALSHOCK_BT 0x1
#define HIDRAW_FIXUP_DUALSENSE_BT 0x2

struct device_extension
{
    struct list entry;
    DEVICE_OBJECT *device;

    CRITICAL_SECTION cs;
    enum device_state state;

    struct device_desc desc;
    DWORD index;

    BYTE *report_desc;
    UINT report_desc_length;
    HIDP_DEVICE_DESC collection_desc;

    struct hid_report *last_reports[256];
    struct list reports;
    IRP *pending_read;

    UINT32 report_fixups;
    UINT64 unix_device;
};

static CRITICAL_SECTION device_list_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &device_list_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": device_list_cs") }
};
static CRITICAL_SECTION device_list_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static struct list device_list = LIST_INIT(device_list);

static NTSTATUS winebus_call(unsigned int code, void *args)
{
    return WINE_UNIX_CALL(code, args);
}

static void unix_device_remove(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct device_remove_params params = {.device = ext->unix_device};
    winebus_call(device_remove, &params);
}

static NTSTATUS unix_device_start(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct device_start_params params = {.device = ext->unix_device};
    return winebus_call(device_start, &params);
}

static void unix_device_set_output_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct device_report_params params =
    {
        .device = ext->unix_device,
        .packet = packet,
        .io = io,
    };
    winebus_call(device_set_output_report, &params);
}

static void unix_device_get_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct device_report_params params =
    {
        .device = ext->unix_device,
        .packet = packet,
        .io = io,
    };
    winebus_call(device_get_feature_report, &params);
}

static void unix_device_set_feature_report(DEVICE_OBJECT *device, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct device_report_params params =
    {
        .device = ext->unix_device,
        .packet = packet,
        .io = io,
    };
    winebus_call(device_set_feature_report, &params);
}

static DWORD get_device_index(struct device_desc *desc, struct list **before)
{
    struct device_extension *ext;
    DWORD index = 0;

    *before = NULL;

    /* The device list is sorted, so just increment the index until it doesn't match an index already in the list */
    LIST_FOR_EACH_ENTRY(ext, &device_list, struct device_extension, entry)
    {
        if (ext->desc.vid == desc->vid && ext->desc.pid == desc->pid && ext->desc.input == desc->input)
        {
            if (ext->index != index)
            {
                *before = &ext->entry;
                break;
            }
            index++;
        }
    }

    return index;
}

static WCHAR *get_instance_id(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD len = wcslen(ext->desc.serialnumber) + 33;
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, len * sizeof(WCHAR))))
    {
        swprintf(dst, len, L"%u&%s&%x&%u&%u", ext->desc.version, ext->desc.serialnumber,
                 ext->desc.uid, ext->index, ext->desc.is_gamepad);
    }

    return dst;
}

static WCHAR *get_device_id(DEVICE_OBJECT *device)
{
    static const WCHAR input_format[] = L"&MI_%02u";
    static const WCHAR winebus_format[] = L"WINEBUS\\VID_%04X&PID_%04X";
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD pos = 0, len = 0, input_len = 0, winebus_len = 25;
    WCHAR *dst;

    if (ext->desc.input != -1) input_len = 14;

    len += winebus_len + input_len + 1;

    if ((dst = ExAllocatePool(PagedPool, len * sizeof(WCHAR))))
    {
        pos += swprintf(dst + pos, len - pos, winebus_format, ext->desc.vid, ext->desc.pid);
        if (input_len) pos += swprintf(dst + pos, len - pos, input_format, ext->desc.input);
    }

    return dst;
}

static WCHAR *get_hardware_ids(DEVICE_OBJECT *device)
{
    static const WCHAR input_format[] = L"&MI_%02u";
    static const WCHAR winebus_format[] = L"WINEBUS\\VID_%04X&PID_%04X";
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD pos = 0, len = 0, input_len = 0, winebus_len = 25;
    WCHAR *dst;

    if (ext->desc.input != -1) input_len = 14;

    len += winebus_len + input_len + 1;

    if ((dst = ExAllocatePool(PagedPool, (len + 1) * sizeof(WCHAR))))
    {
        pos += swprintf(dst + pos, len - pos, winebus_format, ext->desc.vid, ext->desc.pid);
        if (input_len) pos += swprintf(dst + pos, len - pos, input_format, ext->desc.input);
        pos += 1;
        dst[pos] = 0;
    }

    return dst;
}

static WCHAR *get_compatible_ids(DEVICE_OBJECT *device)
{
    static const WCHAR xinput_compat[] = L"WINEBUS\\WINE_COMP_XINPUT";
    static const WCHAR hid_compat[] = L"WINEBUS\\WINE_COMP_HID";
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD size = sizeof(hid_compat);
    WCHAR *dst;

    if (ext->desc.is_gamepad) size += sizeof(xinput_compat);

    if ((dst = ExAllocatePool(PagedPool, size + sizeof(WCHAR))))
    {
        if (ext->desc.is_gamepad) memcpy(dst, xinput_compat, sizeof(xinput_compat));
        memcpy((char *)dst + size - sizeof(hid_compat), hid_compat, sizeof(hid_compat));
        dst[size / sizeof(WCHAR)] = 0;
    }

    return dst;
}

static WCHAR *get_device_text(DEVICE_OBJECT *device)
{
    struct device_extension *ext = device->DeviceExtension;
    const WCHAR *src = ext->desc.product;
    DWORD size;
    WCHAR *dst;

    size = (wcslen(src) + 1) * sizeof(WCHAR);
    if ((dst = ExAllocatePool( PagedPool, size )))
        memcpy( dst, src, size );

    TRACE("Returning %s.\n", debugstr_w(dst));
    return dst;
}

static IRP *pop_pending_read(struct device_extension *ext)
{
    IRP *pending;

    RtlEnterCriticalSection(&ext->cs);
    pending = ext->pending_read;
    ext->pending_read = NULL;
    RtlLeaveCriticalSection(&ext->cs);

    return pending;
}

static void remove_pending_irps(DEVICE_OBJECT *device)
{
    struct device_extension *ext = device->DeviceExtension;
    IRP *pending;

    if ((pending = pop_pending_read(ext)))
    {
        pending->IoStatus.Status = STATUS_DELETE_PENDING;
        pending->IoStatus.Information = 0;
        IoCompleteRequest(pending, IO_NO_INCREMENT);
    }
}

static DEVICE_OBJECT *bus_create_hid_device(struct device_desc *desc, UINT64 unix_device)
{
    struct device_extension *ext;
    DEVICE_OBJECT *device;
    UNICODE_STRING nameW;
    WCHAR dev_name[256];
    struct list *before;
    NTSTATUS status;

    TRACE("desc %s, unix_device %#I64x\n", debugstr_device_desc(desc), unix_device);

    swprintf(dev_name, ARRAY_SIZE(dev_name), L"\\Device\\WINEBUS#%p", unix_device);
    RtlInitUnicodeString(&nameW, dev_name);
    status = IoCreateDevice(driver_obj, sizeof(struct device_extension), &nameW, 0, 0, FALSE, &device);
    if (status)
    {
        FIXME("failed to create device error %#lx\n", status);
        return NULL;
    }

    RtlEnterCriticalSection(&device_list_cs);

    /* fill out device_extension struct */
    ext = (struct device_extension *)device->DeviceExtension;
    ext->device             = device;
    ext->desc               = *desc;
    ext->index              = get_device_index(desc, &before);
    ext->unix_device        = unix_device;
    list_init(&ext->reports);

    if (desc->is_hidraw && desc->is_bluetooth && is_dualshock4_gamepad(desc->vid, desc->pid))
    {
        TRACE("Enabling report fixup for Bluetooth DualShock4 device %p\n", device);
        ext->report_fixups |= HIDRAW_FIXUP_DUALSHOCK_BT;
    }
    if (desc->is_hidraw && desc->is_bluetooth && is_dualsense_gamepad(desc->vid, desc->pid))
    {
        TRACE("Enabling report fixup for Bluetooth DualSense device %p\n", device);
        ext->report_fixups |= HIDRAW_FIXUP_DUALSENSE_BT;
    }

    InitializeCriticalSectionEx(&ext->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    ext->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": cs");

    /* add to list of pnp devices */
    if (before)
        list_add_before(before, &ext->entry);
    else
        list_add_tail(&device_list, &ext->entry);

    RtlLeaveCriticalSection(&device_list_cs);

    TRACE("created device %p/%#I64x\n", device, unix_device);
    return device;
}

static DEVICE_OBJECT *bus_find_unix_device(UINT64 unix_device)
{
    struct device_extension *ext;

    LIST_FOR_EACH_ENTRY(ext, &device_list, struct device_extension, entry)
        if (ext->unix_device == unix_device) return ext->device;

    return NULL;
}

static void bus_unlink_hid_device(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;

    RtlEnterCriticalSection(&device_list_cs);
    list_remove(&ext->entry);
    RtlLeaveCriticalSection(&device_list_cs);
}

#ifdef __ASM_USE_FASTCALL_WRAPPER
extern void * WINAPI wrap_fastcall_func1(void *func, const void *a);
__ASM_STDCALL_FUNC(wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax");
#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)
#else
#define call_fastcall_func1(func,a) func(a)
#endif

static NTSTATUS build_device_relations(DEVICE_RELATIONS **devices)
{
    struct device_extension *ext;
    int i;

    RtlEnterCriticalSection(&device_list_cs);
    *devices = ExAllocatePool(PagedPool, offsetof(DEVICE_RELATIONS, Objects[list_count(&device_list)]));

    if (!*devices)
    {
        RtlLeaveCriticalSection(&device_list_cs);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(ext, &device_list, struct device_extension, entry)
    {
        (*devices)->Objects[i] = ext->device;
        call_fastcall_func1(ObfReferenceObject, ext->device);
        i++;
    }
    RtlLeaveCriticalSection(&device_list_cs);
    (*devices)->Count = i;
    return STATUS_SUCCESS;
}

static DWORD check_bus_option(const WCHAR *option, DWORD default_value)
{
    char buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    UNICODE_STRING str;
    DWORD size;

    /* @@ Wine registry key: HKLM\System\CurrentControlSet\Services\WineBus */
    RtlInitUnicodeString(&str, option);
    if (NtQueryValueKey(driver_key, &str, KeyValuePartialInformation, info, sizeof(buffer), &size) == STATUS_SUCCESS)
    {
        if (info->Type == REG_DWORD) return *(DWORD *)info->Data;
    }

    return default_value;
}

static BOOL is_hidraw_enabled(WORD vid, WORD pid, const USAGE_AND_PAGE *usages, UINT buttons)
{
    char buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[1024])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    struct device_options *device;
    WCHAR vidpid[MAX_PATH], *tmp;
    BOOL prefer_hidraw = FALSE;
    UNICODE_STRING str;
    DWORD size;

    if (options.disable_hidraw) return FALSE;

    LIST_FOR_EACH_ENTRY(device, &options.devices, struct device_options, entry)
    {
        if (device->vid != vid) continue;
        if (device->pid != -1 && device->pid != pid) continue;
        if (device->hidraw == -1) continue;
        return !!device->hidraw;
    }

    if (usages->UsagePage == HID_USAGE_PAGE_DIGITIZER)
    {
        WARN("Ignoring unsupported %04X:%04X hidraw touchscreen\n", vid, pid);
        return FALSE;
    }
    if (usages->UsagePage != HID_USAGE_PAGE_GENERIC) return TRUE;
    if (usages->Usage == HID_USAGE_GENERIC_MOUSE || usages->Usage == HID_USAGE_GENERIC_KEYBOARD)
    {
        WARN("Ignoring unsupported %04X:%04X hidraw mouse/keyboard\n", vid, pid);
        return FALSE;
    }
    if (usages->Usage != HID_USAGE_GENERIC_GAMEPAD && usages->Usage != HID_USAGE_GENERIC_JOYSTICK) return TRUE;

    if (options.disable_sdl && options.disable_input) prefer_hidraw = TRUE;
    if (is_dualshock4_gamepad(vid, pid)) prefer_hidraw = TRUE;
    if (is_dualsense_gamepad(vid, pid)) prefer_hidraw = TRUE;

    switch (vid)
    {
    case 0x044f:
        if (pid == 0xb679) prefer_hidraw = TRUE; /* ThrustMaster T-Rudder */
        if (pid == 0xb687) prefer_hidraw = TRUE; /* ThrustMaster TWCS Throttle */
        if (pid == 0xb10a) prefer_hidraw = TRUE; /* ThrustMaster T.16000M Joystick */
        break;
    case 0x16d0:
        if (pid == 0x0d61) prefer_hidraw = TRUE; /* Simucube 2 Sport */
        if (pid == 0x0d60) prefer_hidraw = TRUE; /* Simucube 2 Pro */
        if (pid == 0x0d5f) prefer_hidraw = TRUE; /* Simucube 2 Ultimate */
        if (pid == 0x0d5a) prefer_hidraw = TRUE; /* Simucube 1 */
        break;
    case 0x0eb7:
        if (pid == 0x183b) prefer_hidraw = TRUE; /* Fanatec ClubSport Pedals v3 */
        if (pid == 0x1839) prefer_hidraw = TRUE; /* Fanatec ClubSport Pedals v1/v2 */
        break;
    case 0x231d:
        /* comes with 128 buttons in the default configuration */
        if (buttons == 128) prefer_hidraw = TRUE;
        /* if customized, less than 128 buttons may be shown, decide by PID */
        if (pid == 0x0200) prefer_hidraw = TRUE; /* VKBsim Gladiator EVO Right Grip */
        if (pid == 0x0201) prefer_hidraw = TRUE; /* VKBsim Gladiator EVO Left Grip */
        if (pid == 0x0126) prefer_hidraw = TRUE; /* VKB-Sim Space Gunfighter */
        if (pid == 0x0127) prefer_hidraw = TRUE; /* VKB-Sim Space Gunfighter L */
        break;
    case 0x3344:
        /* all VPC devices require hidraw, have variable numbers of axis/buttons, & in many cases
         * have functionally random PID. due to this, the only safe way to grab all VPC devices is
         * a catch-all on VID and exclude any hypothetical future device that wants hidraw=false */
        prefer_hidraw = TRUE;
        break;
    case 0x03eb:
        /* users may have configured button limits, usually 32/50/64 */
        if ((buttons == 32) || (buttons == 50) || (buttons == 64)) prefer_hidraw = TRUE;
        if (pid == 0x2055) prefer_hidraw = TRUE; /* ATMEL/VIRPIL/200325 VPC Throttle MT-50 CM2 */
        break;
    }

    RtlInitUnicodeString(&str, L"EnableHidraw");
    if (!NtQueryValueKey(driver_key, &str, KeyValuePartialInformation, info,
                         sizeof(buffer) - sizeof(WCHAR), &size))
    {
        UINT len = swprintf(vidpid, ARRAY_SIZE(vidpid), L"%04X:%04X", vid, pid);
        size -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
        tmp = (WCHAR *)info->Data;

        while (size >= len * sizeof(WCHAR))
        {
            if (!wcsnicmp(tmp, vidpid, len)) prefer_hidraw = TRUE;
            size -= (len + 1) * sizeof(WCHAR);
            tmp += len + 1;
        }
    }

    return prefer_hidraw;
}

static BOOL deliver_next_report(struct device_extension *ext, IRP *irp)
{
    struct hid_report *report;
    struct list *entry;
    ULONG i;

    if (!(entry = list_head(&ext->reports))) return FALSE;
    report = LIST_ENTRY(entry, struct hid_report, entry);
    list_remove(&report->entry);

    memcpy(irp->UserBuffer, report->buffer, report->length);
    irp->IoStatus.Information = report->length;
    irp->IoStatus.Status = STATUS_SUCCESS;

    if (TRACE_ON(hid))
    {
        TRACE("device %p/%#I64x input report length %lu:\n", ext->device, ext->unix_device, report->length);
        for (i = 0; i < report->length;)
        {
            char buffer[256], *buf = buffer;
            buf += sprintf(buf, "%08lx ", i);
            do { buf += sprintf(buf, " %02x", report->buffer[i]); }
            while (++i % 16 && i < report->length);
            TRACE("%s\n", buffer);
        }
    }

    RtlFreeHeap(GetProcessHeap(), 0, report);
    return TRUE;
}

static void process_hid_report(DEVICE_OBJECT *device, BYTE *report_buf, DWORD report_len)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    ULONG size = offsetof(struct hid_report, buffer[report_len]);
    struct hid_report *report, *last_report;
    IRP *irp;

    TRACE("device %p report_buf %p (%#x), report_len %#lx\n", device, report_buf, *report_buf, report_len);

    if (!(report = RtlAllocateHeap(GetProcessHeap(), 0, size))) return;
    memcpy(report->buffer, report_buf, report_len);
    report->length = report_len;

    if (ext->report_fixups & HIDRAW_FIXUP_DUALSHOCK_BT)
    {
        /* As described in the Linux kernel driver, when connected over bluetooth, DS4 controllers
         * start sending input through report #17 as soon as they receive a feature report #2, which
         * the kernel sends anyway for calibration.
         *
         * Input report #17 is the same as the default input report #1, with additional gyro data and
         * two additional bytes in front, but is only described as vendor specific in the report descriptor,
         * and applications aren't expecting it.
         *
         * We have to translate it to input report #1, like native driver does.
         */
        if (report->buffer[0] == 0x11 && report->length >= 12)
        {
            memmove(report->buffer, report->buffer + 2, 10);
            report->buffer[0] = 1; /* fake report #1 */
            report->length = 10;
        }
    }

    if (ext->report_fixups & HIDRAW_FIXUP_DUALSENSE_BT)
    {
        /* The behavior of DualSense is very similar to DS4 described above with a few exceptions.
         *
         * The report number #41 is used for the extended bluetooth input report. The report comes
         * with only one extra byte in front and the format is not exactly the same as the one used
         * for the report #1 so we need to shuffle a few bytes around.
         *
         * Basic #1 report:
         *   X  Y  Z  RZ  Buttons[3]  TriggerLeft  TriggerRight
         *
         * Extended #41 report:
         *   Prefix X  Y  Z  Rz  TriggerLeft  TriggerRight  Counter  Buttons[3] ...
         */
        if (report->buffer[0] == 0x31 && report->length >= 12)
        {
            BYTE trigger[2];

            memmove(report->buffer, report->buffer + 1, 11);
            report->buffer[0] = 1; /* fake report #1 */
            report->length = 10;

            trigger[0] = report->buffer[5]; /* TriggerLeft*/
            trigger[1] = report->buffer[6]; /* TriggerRight */

            report->buffer[5] = report->buffer[8];  /* Buttons[0] */
            report->buffer[6] = report->buffer[9];  /* Buttons[1] */
            report->buffer[7] = report->buffer[10]; /* Buttons[2] */
            report->buffer[8] = trigger[0]; /* TriggerLeft */
            report->buffer[9] = trigger[1]; /* TirggerRight */
        }
    }

    RtlEnterCriticalSection(&ext->cs);

    if (ext->state != DEVICE_STATE_STARTED)
    {
        RtlLeaveCriticalSection(&ext->cs);
        return;
    }

    if (!ext->collection_desc.ReportIDs[0].ReportID) last_report = ext->last_reports[0];
    else last_report = ext->last_reports[report_buf[0]];
    if (!last_report)
    {
        WARN("Ignoring report with unexpected id %#x\n", *report_buf);
        RtlLeaveCriticalSection(&ext->cs);
        return;
    }

    list_add_tail(&ext->reports, &report->entry);

    memcpy(last_report->buffer, report_buf, report_len);

    if ((irp = pop_pending_read(ext)))
    {
        deliver_next_report(ext, irp);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    RtlLeaveCriticalSection(&ext->cs);
}

static NTSTATUS handle_IRP_MN_QUERY_DEVICE_RELATIONS(IRP *irp)
{
    NTSTATUS status = irp->IoStatus.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );

    TRACE("IRP_MN_QUERY_DEVICE_RELATIONS\n");
    switch (irpsp->Parameters.QueryDeviceRelations.Type)
    {
        case EjectionRelations:
        case RemovalRelations:
        case TargetDeviceRelation:
        case PowerRelations:
            FIXME("Unhandled Device Relation %x\n",irpsp->Parameters.QueryDeviceRelations.Type);
            break;
        case BusRelations:
            status = build_device_relations((DEVICE_RELATIONS**)&irp->IoStatus.Information);
            break;
        default:
            FIXME("Unknown Device Relation %x\n",irpsp->Parameters.QueryDeviceRelations.Type);
            break;
    }

    return status;
}

static NTSTATUS handle_IRP_MN_QUERY_ID(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BUS_QUERY_ID_TYPE type = irpsp->Parameters.QueryId.IdType;

    TRACE("(%p, %p)\n", device, irp);

    switch (type)
    {
        case BusQueryHardwareIDs:
            TRACE("BusQueryHardwareIDs\n");
            irp->IoStatus.Information = (ULONG_PTR)get_hardware_ids(device);
            break;
        case BusQueryCompatibleIDs:
            TRACE("BusQueryCompatibleIDs\n");
            irp->IoStatus.Information = (ULONG_PTR)get_compatible_ids(device);
            break;
        case BusQueryDeviceID:
            TRACE("BusQueryDeviceID\n");
            irp->IoStatus.Information = (ULONG_PTR)get_device_id(device);
            break;
        case BusQueryInstanceID:
            TRACE("BusQueryInstanceID\n");
            irp->IoStatus.Information = (ULONG_PTR)get_instance_id(device);
            break;
        default:
            WARN("Unhandled type %08x\n", type);
            return status;
    }

    status = irp->IoStatus.Information ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    return status;
}

static NTSTATUS handle_IRP_MN_QUERY_DEVICE_TEXT(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    DEVICE_TEXT_TYPE type = irpsp->Parameters.QueryDeviceText.DeviceTextType;
    NTSTATUS status = irp->IoStatus.Status;

    TRACE("(%p, %p)\n", device, irp);

    switch (type)
    {
        case DeviceTextDescription:
            TRACE("DeviceTextDescription.\n");
            irp->IoStatus.Information = (ULONG_PTR)get_device_text(device);
            break;

        default:
            WARN("Unhandled type %08x.\n", type);
            return status;
    }

    status = irp->IoStatus.Information ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    return status;
}

static void mouse_device_create(void)
{
    struct device_create_params params = {{0}};

    if (winebus_call(mouse_create, &params)) return;
    mouse_obj = bus_create_hid_device(&params.desc, params.device);
    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
}

static void keyboard_device_create(void)
{
    struct device_create_params params = {{0}};

    if (winebus_call(keyboard_create, &params)) return;
    keyboard_obj = bus_create_hid_device(&params.desc, params.device);
    IoInvalidateDeviceRelations(bus_pdo, BusRelations);
}

static NTSTATUS get_device_descriptors(UINT64 unix_device, BYTE **report_desc, UINT *report_desc_length,
                                       HIDP_DEVICE_DESC *device_desc)
{
    struct device_descriptor_params params =
    {
        .device = unix_device,
        .out_length = report_desc_length,
    };
    NTSTATUS status;

    status = winebus_call(device_get_report_descriptor, &params);
    if (status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
    {
        ERR("Failed to get device %#I64x report descriptor, status %#lx\n", unix_device, status);
        return status;
    }

    if (!(params.buffer = RtlAllocateHeap(GetProcessHeap(), 0, *report_desc_length)))
        return STATUS_NO_MEMORY;
    params.length = *report_desc_length;

    if ((status = winebus_call(device_get_report_descriptor, &params)))
    {
        ERR("Failed to get device %#I64x report descriptor, status %#lx\n", unix_device, status);
        RtlFreeHeap(GetProcessHeap(), 0, params.buffer);
        return status;
    }

    params.length = *report_desc_length;
    status = HidP_GetCollectionDescription(params.buffer, params.length, PagedPool, device_desc);
    if (status != HIDP_STATUS_SUCCESS)
    {
        ERR("Failed to get device %#I64x report descriptor, status %#lx\n", unix_device, status);
        RtlFreeHeap(GetProcessHeap(), 0, params.buffer);
        return status;
    }

    *report_desc = params.buffer;
    return STATUS_SUCCESS;
}

static USAGE_AND_PAGE get_device_usages(UINT64 unix_device, UINT *buttons)
{
    HIDP_DEVICE_DESC device_desc;
    USAGE_AND_PAGE usages = {0};
    UINT i, count = 0, report_desc_length;
    HIDP_BUTTON_CAPS *button_caps;
    BYTE *report_desc;
    NTSTATUS status;
    HIDP_CAPS caps;

    if (!(status = get_device_descriptors(unix_device, &report_desc, &report_desc_length, &device_desc)))
    {
        PHIDP_PREPARSED_DATA preparsed = device_desc.CollectionDesc[0].PreparsedData;
        usages.UsagePage = device_desc.CollectionDesc[0].UsagePage;
        usages.Usage = device_desc.CollectionDesc[0].Usage;

        if ((status = HidP_GetCaps(preparsed, &caps)) == HIDP_STATUS_SUCCESS &&
            (button_caps = malloc(sizeof(*button_caps) * caps.NumberInputButtonCaps)))
        {
            status = HidP_GetButtonCaps(HidP_Input, button_caps, &caps.NumberInputButtonCaps, preparsed);
            if (status != HIDP_STATUS_SUCCESS) WARN("HidP_GetButtonCaps returned %#lx\n", status);
            else for (i = 0; i < caps.NumberInputButtonCaps; i++)
            {
                if (button_caps[i].UsagePage != HID_USAGE_PAGE_BUTTON) continue;
                if (button_caps[i].IsRange) count = max(count, button_caps[i].Range.UsageMax);
                else count = max(count, button_caps[i].NotRange.Usage);
            }
            free(button_caps);
        }

        HidP_FreeCollectionDescription(&device_desc);
        RtlFreeHeap(GetProcessHeap(), 0, report_desc);
    }

    *buttons = count;
    return usages;
}

static DWORD bus_count;
static HANDLE bus_thread[16];

struct bus_main_params
{
    const WCHAR *name;

    void *init_args;
    HANDLE init_done;
    NTSTATUS *init_status;
    unsigned int init_code;
    unsigned int wait_code;
    struct bus_event *bus_event;
};

static DWORD CALLBACK bus_main_thread(void *args)
{
    struct bus_main_params bus = *(struct bus_main_params *)args;
    DEVICE_OBJECT *device;
    NTSTATUS status;

    TRACE("%s main loop starting\n", debugstr_w(bus.name));
    status = winebus_call(bus.init_code, bus.init_args);
    *bus.init_status = status;
    SetEvent(bus.init_done);
    TRACE("%s main loop started\n", debugstr_w(bus.name));

    bus.bus_event->type = BUS_EVENT_TYPE_NONE;
    if (status) WARN("%s bus init returned status %#lx\n", debugstr_w(bus.name), status);
    else while ((status = winebus_call(bus.wait_code, bus.bus_event)) == STATUS_PENDING)
    {
        struct bus_event *event = bus.bus_event;
        switch (event->type)
        {
        case BUS_EVENT_TYPE_NONE: break;
        case BUS_EVENT_TYPE_DEVICE_REMOVED:
            RtlEnterCriticalSection(&device_list_cs);
            device = bus_find_unix_device(event->device);
            if (!device) WARN("could not find device for %s bus device %#I64x\n", debugstr_w(bus.name), event->device);
            else bus_unlink_hid_device(device);
            RtlLeaveCriticalSection(&device_list_cs);
            IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            break;
        case BUS_EVENT_TYPE_DEVICE_CREATED:
        {
            struct device_desc desc = event->device_created.desc;
            USAGE_AND_PAGE usages;
            UINT buttons;

            usages = get_device_usages(event->device, &buttons);
            if (!desc.is_hidraw != !is_hidraw_enabled(desc.vid, desc.pid, &usages, buttons))
            {
                struct device_remove_params params = {.device = event->device};
                WARN("ignoring %shidraw device %04x:%04x with usages %04x:%04x\n", desc.is_hidraw ? "" : "non-",
                     desc.vid, desc.pid, usages.UsagePage, usages.Usage);
                winebus_call(device_remove, &params);
                break;
            }

            TRACE("creating %shidraw device %04x:%04x with usages %04x:%04x\n", desc.is_hidraw ? "" : "non-",
                  desc.vid, desc.pid, usages.UsagePage, usages.Usage);

            device = bus_create_hid_device(&event->device_created.desc, event->device);
            if (device) IoInvalidateDeviceRelations(bus_pdo, BusRelations);
            else
            {
                struct device_remove_params params = {.device = event->device};
                WARN("failed to create device for %s bus device %#I64x\n", debugstr_w(bus.name), event->device);
                winebus_call(device_remove, &params);
            }
            break;
        }
        case BUS_EVENT_TYPE_INPUT_REPORT:
            RtlEnterCriticalSection(&device_list_cs);
            device = bus_find_unix_device(event->device);
            if (!device) WARN("could not find device for %s bus device %#I64x\n", debugstr_w(bus.name), event->device);
            else process_hid_report(device, event->input_report.buffer, event->input_report.length);
            RtlLeaveCriticalSection(&device_list_cs);
            break;
        }
    }

    if (status) WARN("%s bus wait returned status %#lx\n", debugstr_w(bus.name), status);
    else TRACE("%s main loop exited\n", debugstr_w(bus.name));
    RtlFreeHeap(GetProcessHeap(), 0, bus.bus_event);
    return status;
}

static NTSTATUS bus_main_thread_start(struct bus_main_params *bus)
{
    DWORD i = bus_count++, max_size;
    NTSTATUS status;

    if (!(bus->init_done = CreateEventW(NULL, FALSE, FALSE, NULL)))
    {
        ERR("failed to create %s bus init done event.\n", debugstr_w(bus->name));
        bus_count--;
        return STATUS_UNSUCCESSFUL;
    }

    max_size = offsetof(struct bus_event, input_report.buffer[0x10000]);
    if (!(bus->bus_event = RtlAllocateHeap(GetProcessHeap(), 0, max_size)))
    {
        ERR("failed to allocate %s bus event.\n", debugstr_w(bus->name));
        CloseHandle(bus->init_done);
        bus_count--;
        return STATUS_UNSUCCESSFUL;
    }

    bus->init_status = &status;
    if (!(bus_thread[i] = CreateThread(NULL, 0, bus_main_thread, bus, 0, NULL)))
    {
        ERR("failed to create %s bus thread.\n", debugstr_w(bus->name));
        CloseHandle(bus->init_done);
        bus_count--;
        return STATUS_UNSUCCESSFUL;
    }

    WaitForSingleObject(bus->init_done, INFINITE);
    CloseHandle(bus->init_done);
    return status;
}

static void sdl_bus_free_mappings(struct bus_options *options)
{
    DWORD count = options->mappings_count;
    char **mappings = options->mappings;

    while (count) RtlFreeHeap(GetProcessHeap(), 0, mappings[--count]);
    RtlFreeHeap(GetProcessHeap(), 0, mappings);
}

static void sdl_bus_load_mappings(struct bus_options *options)
{
    ULONG idx = 0, len, count = 0, capacity, info_size, info_max_size;
    UNICODE_STRING path = RTL_CONSTANT_STRING(L"map");
    KEY_VALUE_FULL_INFORMATION *info;
    OBJECT_ATTRIBUTES attr = {0};
    char **mappings = NULL;
    NTSTATUS status;
    HANDLE key;

    options->mappings_count = 0;
    options->mappings = NULL;

    InitializeObjectAttributes(&attr, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, driver_key, NULL);
    status = NtOpenKey(&key, KEY_ALL_ACCESS, &attr);
    if (status) return;

    capacity = 1024;
    mappings = RtlAllocateHeap(GetProcessHeap(), 0, capacity * sizeof(*mappings));
    info_max_size = offsetof(KEY_VALUE_FULL_INFORMATION, Name) + 512;
    info = RtlAllocateHeap(GetProcessHeap(), 0, info_max_size);

    while (!status && info && mappings)
    {
        status = NtEnumerateValueKey(key, idx, KeyValueFullInformation, info, info_max_size, &info_size);
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            info_max_size = info_size;
            if (!(info = RtlReAllocateHeap(GetProcessHeap(), 0, info, info_max_size))) break;
            status = NtEnumerateValueKey(key, idx, KeyValueFullInformation, info, info_max_size, &info_size);
        }

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            options->mappings_count = count;
            options->mappings = mappings;
            goto done;
        }

        idx++;
        if (status) break;
        if (info->Type != REG_SZ) continue;

        RtlUnicodeToMultiByteSize(&len, (WCHAR *)((char *)info + info->DataOffset), info_size - info->DataOffset);
        if (!len) continue;

        if (!(mappings[count++] = RtlAllocateHeap(GetProcessHeap(), 0, len + 1))) break;
        if (count > capacity)
        {
            capacity = capacity * 3 / 2;
            if (!(mappings = RtlReAllocateHeap(GetProcessHeap(), 0, mappings, capacity * sizeof(*mappings))))
                break;
        }

        RtlUnicodeToMultiByteN(mappings[count], len, NULL, (WCHAR *)((char *)info + info->DataOffset),
                               info_size - info->DataOffset);
        if (mappings[len - 1]) mappings[len] = 0;
    }

    if (mappings) while (count) RtlFreeHeap(GetProcessHeap(), 0, mappings[--count]);
    RtlFreeHeap(GetProcessHeap(), 0, mappings);

done:
    RtlFreeHeap(GetProcessHeap(), 0, info);
    NtClose(key);
}

static struct device_options *add_device_options(UINT vid, UINT pid)
{
    struct device_options *device, *next;

    LIST_FOR_EACH_ENTRY(device, &options.devices, struct device_options, entry)
        if (device->vid == vid && device->pid == pid) return device;

    if (!(device = calloc(1, sizeof(*device)))) return NULL;
    device->vid = vid;
    device->pid = pid;
    device->hidraw = -1;

    LIST_FOR_EACH_ENTRY(next, &options.devices, struct device_options, entry)
        if (next->vid > vid || (next->vid == vid && next->pid > pid)) break;
    list_add_before(&next->entry, &device->entry);

    return device;
}

static void load_device_options(void)
{
    char buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[1024])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    UNICODE_STRING path = RTL_CONSTANT_STRING(L"Devices");
    ULONG idx = 0, size, name_max_size;
    OBJECT_ATTRIBUTES attr = {0};
    KEY_NAME_INFORMATION *name;
    WCHAR name_buffer[32];
    HANDLE key, subkey;
    NTSTATUS status;

    /* @@ Wine registry key: HKLM\System\CurrentControlSet\Services\WineBus\Devices */
    InitializeObjectAttributes(&attr, &path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, driver_key, NULL);
    status = NtOpenKey(&key, KEY_ALL_ACCESS, &attr);
    if (status) return;

    name_max_size = offsetof(KEY_NAME_INFORMATION, Name) + 512;
    name = RtlAllocateHeap(GetProcessHeap(), 0, name_max_size);

    while (!status && name)
    {
        static const UNICODE_STRING hidraw = RTL_CONSTANT_STRING(L"Hidraw");
        static const UNICODE_STRING backslash = RTL_CONSTANT_STRING(L"\\");
        struct device_options *device;
        UNICODE_STRING name_str;
        UINT vid, pid;
        USHORT pos;
        int ret;

        status = NtEnumerateKey(key, idx, KeyNameInformation, name, name_max_size, &size);
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            name_max_size = size;
            if (!(name = RtlReAllocateHeap(GetProcessHeap(), 0, name, name_max_size))) break;
            status = NtEnumerateKey(key, idx, KeyNameInformation, name, name_max_size, &size);
        }
        if (status == STATUS_NO_MORE_ENTRIES) break;
        idx++;

        /* @@ Wine registry key: HKLM\System\CurrentControlSet\Services\WineBus\Devices\<VID[/PID]> */
        name_str.Buffer = name->Name;
        name_str.Length = name->NameLength;
        InitializeObjectAttributes(&attr, &name_str, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, NULL);
        if (NtOpenKey(&subkey, KEY_ALL_ACCESS, &attr)) continue;

        if (!RtlFindCharInUnicodeString(1, &name_str, &backslash, &pos)) pos += sizeof(WCHAR);
        if (name->NameLength - pos >= sizeof(name_buffer)) continue;

        memcpy(name_buffer, name->Name + pos / sizeof(WCHAR), name->NameLength - pos);
        name_buffer[(name->NameLength - pos) / sizeof(WCHAR)] = 0;

        if ((ret = swscanf(name_buffer, L"%04x/%04x", &vid, &pid)) < 1) continue;
        if (!(device = add_device_options(vid, ret == 1 ? -1 : pid))) continue;

        if (!NtQueryValueKey(subkey, &hidraw, KeyValuePartialInformation, info, sizeof(buffer), &size) && info->Type == REG_DWORD)
            device->hidraw = *(DWORD *)info->Data;
        if (device->hidraw != -1) TRACE("- %04x/%04x: %sabling hidraw\n", device->vid, device->pid, device->hidraw ? "en" : "dis");

        NtClose(subkey);
    }

    RtlFreeHeap(GetProcessHeap(), 0, name);
    NtClose(key);
}

static void bus_options_init(void)
{
    options.disable_sdl = !check_bus_option(L"Enable SDL", 1);
    if (options.disable_sdl) TRACE("SDL devices disabled in registry\n");
    options.disable_hidraw = check_bus_option(L"DisableHidraw", 0);
    if (options.disable_hidraw) TRACE("UDEV hidraw devices disabled in registry\n");
    options.disable_input = check_bus_option(L"DisableInput", 0);
    if (options.disable_input) TRACE("UDEV input devices disabled in registry\n");
    options.disable_udevd = check_bus_option(L"DisableUdevd", 0);
    if (options.disable_udevd) TRACE("UDEV udevd use disabled in registry\n");

    if (!options.disable_sdl)
    {
        options.split_controllers = check_bus_option(L"Split Controllers", 0);
        if (options.split_controllers) TRACE("SDL controller splitting enabled\n");
        options.map_controllers = check_bus_option(L"Map Controllers", 1);
        if (!options.map_controllers) TRACE("SDL controller to XInput HID gamepad mapping disabled\n");
        sdl_bus_load_mappings(&options);
    }

    load_device_options();
}

static void bus_options_cleanup(void)
{
    struct device_options *device, *next;

    if (!options.disable_sdl) sdl_bus_free_mappings(&options);

    LIST_FOR_EACH_ENTRY_SAFE(device, next, &options.devices, struct device_options, entry)
    {
        list_remove(&device->entry);
        free(device);
    }

    memset(&options, 0, sizeof(options));
    list_init(&options.devices);
}

static NTSTATUS sdl_driver_init(void)
{
    struct bus_main_params bus =
    {
        .name = L"SDL",
        .init_args = &options,
        .init_code = sdl_init,
        .wait_code = sdl_wait,
    };
    if (options.disable_sdl) return STATUS_NOT_SUPPORTED;
    return bus_main_thread_start(&bus);
}

static NTSTATUS udev_driver_init(void)
{
    struct bus_main_params bus =
    {
        .name = L"UDEV",
        .init_args = &options,
        .init_code = udev_init,
        .wait_code = udev_wait,
    };
    return bus_main_thread_start(&bus);
}

static NTSTATUS iohid_driver_init(void)
{
    struct bus_main_params bus =
    {
        .name = L"IOHID",
        .init_args = &options,
        .init_code = iohid_init,
        .wait_code = iohid_wait,
    };
    if (options.disable_hidraw) return STATUS_SUCCESS;
    return bus_main_thread_start(&bus);
}

static NTSTATUS fdo_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    NTSTATUS ret;

    switch (irpsp->MinorFunction)
    {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        irp->IoStatus.Status = handle_IRP_MN_QUERY_DEVICE_RELATIONS(irp);
        break;
    case IRP_MN_START_DEVICE:
        bus_options_init();

        mouse_device_create();
        keyboard_device_create();

        if (!sdl_driver_init()) options.disable_input = TRUE;
        udev_driver_init();
        iohid_driver_init();

        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_SURPRISE_REMOVAL:
        irp->IoStatus.Status = STATUS_SUCCESS;
        break;
    case IRP_MN_REMOVE_DEVICE:
        winebus_call(sdl_stop, NULL);
        winebus_call(udev_stop, NULL);
        winebus_call(iohid_stop, NULL);

        WaitForMultipleObjects(bus_count, bus_thread, TRUE, INFINITE);
        while (bus_count--) CloseHandle(bus_thread[bus_count]);

        irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(irp);
        ret = IoCallDriver(bus_pdo, irp);
        IoDetachDevice(bus_pdo);
        IoDeleteDevice(device);

        bus_options_cleanup();
        return ret;
    default:
        FIXME("Unhandled minor function %#x.\n", irpsp->MinorFunction);
    }

    IoSkipCurrentIrpStackLocation(irp);
    return IoCallDriver(bus_pdo, irp);
}

static NTSTATUS pdo_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    struct device_extension *ext = device->DeviceExtension;
    NTSTATUS status = irp->IoStatus.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    struct hid_report *report, *next;
    HIDP_REPORT_IDS *reports;
    ULONG i, size;

    TRACE("device %p, irp %p, minor function %#x.\n", device, irp, irpsp->MinorFunction);

    switch (irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
            status = handle_IRP_MN_QUERY_ID(device, irp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            status = handle_IRP_MN_QUERY_DEVICE_TEXT(device, irp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_START_DEVICE:
            RtlEnterCriticalSection(&ext->cs);
            if (ext->state != DEVICE_STATE_STOPPED) status = STATUS_SUCCESS;
            else if (ext->state == DEVICE_STATE_REMOVED) status = STATUS_DELETE_PENDING;
            else if ((status = unix_device_start(device)))
                ERR("Failed to start device %p, status %#lx\n", device, status);
            else if (!(status = get_device_descriptors(ext->unix_device, &ext->report_desc, &ext->report_desc_length,
                                                       &ext->collection_desc)))
            {
                status = STATUS_SUCCESS;
                reports = ext->collection_desc.ReportIDs;
                for (i = 0; i < ext->collection_desc.ReportIDsLength; ++i)
                {
                    if (!(size = reports[i].InputLength)) continue;
                    size = offsetof( struct hid_report, buffer[size] );
                    if (!(report = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, size))) status = STATUS_NO_MEMORY;
                    else
                    {
                        report->length = reports[i].InputLength;
                        report->buffer[0] = reports[i].ReportID;
                        ext->last_reports[reports[i].ReportID] = report;
                    }
                }
                if (!status) ext->state = DEVICE_STATE_STARTED;
            }
            RtlLeaveCriticalSection(&ext->cs);
            break;

        case IRP_MN_SURPRISE_REMOVAL:
            RtlEnterCriticalSection(&ext->cs);
            remove_pending_irps(device);
            ext->state = DEVICE_STATE_REMOVED;
            RtlLeaveCriticalSection(&ext->cs);
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
            remove_pending_irps(device);

            bus_unlink_hid_device(device);
            unix_device_remove(device);

            ext->cs.DebugInfo->Spare[0] = 0;
            DeleteCriticalSection(&ext->cs);

            irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(irp, IO_NO_INCREMENT);

            LIST_FOR_EACH_ENTRY_SAFE(report, next, &ext->reports, struct hid_report, entry)
                RtlFreeHeap(GetProcessHeap(), 0, report);

            reports = ext->collection_desc.ReportIDs;
            for (i = 0; i < ext->collection_desc.ReportIDsLength; ++i)
            {
                if (!reports[i].InputLength) continue;
                RtlFreeHeap(GetProcessHeap(), 0, ext->last_reports[reports[i].ReportID]);
            }
            HidP_FreeCollectionDescription(&ext->collection_desc);
            RtlFreeHeap(GetProcessHeap(), 0, ext->report_desc);

            IoDeleteDevice(device);
            return STATUS_SUCCESS;

        default:
            FIXME("Unhandled function %08x\n", irpsp->MinorFunction);
            /* fall through */

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            break;
    }

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS WINAPI common_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    if (device == bus_fdo)
        return fdo_pnp_dispatch(device, irp);
    return pdo_pnp_dispatch(device, irp);
}

static NTSTATUS hid_get_device_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD buffer_len)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD len;

    switch (index)
    {
    case HID_STRING_ID_IMANUFACTURER:
        len = (wcslen(ext->desc.manufacturer) + 1) * sizeof(WCHAR);
        if (len > buffer_len) return STATUS_BUFFER_TOO_SMALL;
        else memcpy(buffer, ext->desc.manufacturer, len);
        return STATUS_SUCCESS;
    case HID_STRING_ID_IPRODUCT:
        len = (wcslen(ext->desc.product) + 1) * sizeof(WCHAR);
        if (len > buffer_len) return STATUS_BUFFER_TOO_SMALL;
        else memcpy(buffer, ext->desc.product, len);
        return STATUS_SUCCESS;
    case HID_STRING_ID_ISERIALNUMBER:
        len = (wcslen(ext->desc.serialnumber) + 1) * sizeof(WCHAR);
        if (len > buffer_len) return STATUS_BUFFER_TOO_SMALL;
        else memcpy(buffer, ext->desc.serialnumber, len);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_IMPLEMENTED;
}

static void hidraw_disable_report_fixups(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;

    /* FIXME: we may want to validate CRC at the end of the outbound HID reports,
     * as controllers do not switch modes if it is incorrect.
     */

    if ((ext->report_fixups & HIDRAW_FIXUP_DUALSHOCK_BT))
    {
        TRACE("Disabling report fixup for Bluetooth DualShock4 device %p\n", device);
        ext->report_fixups &= ~HIDRAW_FIXUP_DUALSHOCK_BT;
    }

    if ((ext->report_fixups & HIDRAW_FIXUP_DUALSENSE_BT))
    {
        TRACE("Disabling report fixup for Bluetooth DualSense device %p\n", device);
        ext->report_fixups &= ~HIDRAW_FIXUP_DUALSENSE_BT;
    }
}

static NTSTATUS WINAPI hid_internal_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    ULONG i, code, buffer_len = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS status;

    if (device == bus_fdo)
    {
        IoSkipCurrentIrpStackLocation(irp);
        return IoCallDriver(bus_pdo, irp);
    }

    RtlEnterCriticalSection(&ext->cs);

    if (ext->state == DEVICE_STATE_REMOVED)
    {
        RtlLeaveCriticalSection(&ext->cs);
        irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    switch ((code = irpsp->Parameters.DeviceIoControl.IoControlCode))
    {
        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            HID_DEVICE_ATTRIBUTES *attr = (HID_DEVICE_ATTRIBUTES *)irp->UserBuffer;
            TRACE("IOCTL_HID_GET_DEVICE_ATTRIBUTES\n");

            if (buffer_len < sizeof(*attr))
            {
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            memset(attr, 0, sizeof(*attr));
            attr->Size = sizeof(*attr);
            attr->VendorID = ext->desc.vid;
            attr->ProductID = ext->desc.pid;
            attr->VersionNumber = ext->desc.version;

            irp->IoStatus.Status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(*attr);
            break;
        }
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            HID_DESCRIPTOR *descriptor = (HID_DESCRIPTOR *)irp->UserBuffer;
            irp->IoStatus.Information = sizeof(*descriptor);
            if (buffer_len < sizeof(*descriptor)) irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                memset(descriptor, 0, sizeof(*descriptor));
                descriptor->bLength = sizeof(*descriptor);
                descriptor->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
                descriptor->bcdHID = HID_REVISION;
                descriptor->bCountry = 0;
                descriptor->bNumDescriptors = 1;
                descriptor->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
                descriptor->DescriptorList[0].wReportLength = ext->report_desc_length;
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            break;
        }
        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
            irp->IoStatus.Information = ext->report_desc_length;
            if (buffer_len < irp->IoStatus.Information)
                irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            else
            {
                memcpy(irp->UserBuffer, ext->report_desc, ext->report_desc_length);
                irp->IoStatus.Status = STATUS_SUCCESS;
            }
            break;
        case IOCTL_HID_GET_STRING:
        {
            UINT index = (UINT_PTR)irpsp->Parameters.DeviceIoControl.Type3InputBuffer;
            TRACE("IOCTL_HID_GET_STRING[%08x]\n", index);

            irp->IoStatus.Status = hid_get_device_string(device, index, (WCHAR *)irp->UserBuffer, buffer_len);
            if (irp->IoStatus.Status == STATUS_SUCCESS)
                irp->IoStatus.Information = (wcslen((WCHAR *)irp->UserBuffer) + 1) * sizeof(WCHAR);
            break;
        }
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET *)irp->UserBuffer;
            struct hid_report *last_report = ext->last_reports[packet->reportId];
            memcpy(packet->reportBuffer, last_report->buffer, last_report->length);
            packet->reportBufferLen = last_report->length;
            irp->IoStatus.Information = packet->reportBufferLen;
            irp->IoStatus.Status = STATUS_SUCCESS;
            if (TRACE_ON(hid))
            {
                TRACE("read input report id %u length %lu:\n", packet->reportId, packet->reportBufferLen);
                for (i = 0; i < packet->reportBufferLen;)
                {
                    char buffer[256], *buf = buffer;
                    buf += sprintf(buf, "%08lx ", i);
                    do { buf += sprintf(buf, " %02x", packet->reportBuffer[i]); }
                    while (++i % 16 && i < packet->reportBufferLen);
                    TRACE("%s\n", buffer);
                }
            }
            break;
        }
        case IOCTL_HID_READ_REPORT:
        {
            if (!deliver_next_report(ext, irp))
            {
                /* hidclass.sys should guarantee this */
                assert(!ext->pending_read);
                ext->pending_read = irp;
                IoMarkIrpPending(irp);
                irp->IoStatus.Status = STATUS_PENDING;
            }
            break;
        }
        case IOCTL_HID_SET_OUTPUT_REPORT:
        case IOCTL_HID_WRITE_REPORT:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET *)irp->UserBuffer;
            if (TRACE_ON(hid))
            {
                TRACE("write output report id %u length %lu:\n", packet->reportId, packet->reportBufferLen);
                for (i = 0; i < packet->reportBufferLen;)
                {
                    char buffer[256], *buf = buffer;
                    buf += sprintf(buf, "%08lx ", i);
                    do { buf += sprintf(buf, " %02x", packet->reportBuffer[i]); }
                    while (++i % 16 && i < packet->reportBufferLen);
                    TRACE("%s\n", buffer);
                }
            }
            unix_device_set_output_report(device, packet, &irp->IoStatus);
            if (!irp->IoStatus.Status) hidraw_disable_report_fixups(device);
            break;
        }
        case IOCTL_HID_GET_FEATURE:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET *)irp->UserBuffer;
            unix_device_get_feature_report(device, packet, &irp->IoStatus);
            if (!irp->IoStatus.Status) hidraw_disable_report_fixups(device);
            if (!irp->IoStatus.Status && TRACE_ON(hid))
            {
                TRACE("read feature report id %u length %lu:\n", packet->reportId, packet->reportBufferLen);
                for (i = 0; i < packet->reportBufferLen;)
                {
                    char buffer[256], *buf = buffer;
                    buf += sprintf(buf, "%08lx ", i);
                    do { buf += sprintf(buf, " %02x", packet->reportBuffer[i]); }
                    while (++i % 16 && i < packet->reportBufferLen);
                    TRACE("%s\n", buffer);
                }
            }
            break;
        }
        case IOCTL_HID_SET_FEATURE:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET *)irp->UserBuffer;
            if (TRACE_ON(hid))
            {
                TRACE("write feature report id %u length %lu:\n", packet->reportId, packet->reportBufferLen);
                for (i = 0; i < packet->reportBufferLen;)
                {
                    char buffer[256], *buf = buffer;
                    buf += sprintf(buf, "%08lx ", i);
                    do { buf += sprintf(buf, " %02x", packet->reportBuffer[i]); }
                    while (++i % 16 && i < packet->reportBufferLen);
                    TRACE("%s\n", buffer);
                }
            }
            unix_device_set_feature_report(device, packet, &irp->IoStatus);
            if (!irp->IoStatus.Status) hidraw_disable_report_fixups(device);
            break;
        }
        default:
            FIXME("Unsupported ioctl %lx (device=%lx access=%lx func=%lx method=%lx)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            break;
    }

    status = irp->IoStatus.Status;
    RtlLeaveCriticalSection(&ext->cs);

    if (status != STATUS_PENDING) IoCompleteRequest(irp, IO_NO_INCREMENT);
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
    NtClose(driver_key);
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    OBJECT_ATTRIBUTES attr = {0};
    NTSTATUS ret;

    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    if ((ret = __wine_init_unix_call())) return ret;

    attr.Length = sizeof(attr);
    attr.ObjectName = path;
    attr.Attributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
    if ((ret = NtOpenKey(&driver_key, KEY_ALL_ACCESS, &attr)) != STATUS_SUCCESS)
        ERR("Failed to open driver key, status %#lx.\n", ret);

    driver_obj = driver;

    driver->MajorFunction[IRP_MJ_PNP] = common_pnp_dispatch;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = hid_internal_dispatch;
    driver->DriverExtension->AddDevice = driver_add_device;
    driver->DriverUnload = driver_unload;

    return STATUS_SUCCESS;
}
