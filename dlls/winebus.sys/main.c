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
#include "config.h"
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winternl.h"
#include "winreg.h"
#include "setupapi.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "bus.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);
WINE_DECLARE_DEBUG_CHANNEL(hid_report);

#define VID_MICROSOFT 0x045e

static const WORD PID_XBOX_CONTROLLERS[] =  {
    0x0202, /* Xbox Controller */
    0x0285, /* Xbox Controller S */
    0x0289, /* Xbox Controller S */
    0x028e, /* Xbox360 Controller */
    0x028f, /* Xbox360 Wireless Controller */
    0x02d1, /* Xbox One Controller */
    0x02dd, /* Xbox One Controller (Covert Forces/Firmware 2015) */
    0x02e3, /* Xbox One Elite Controller */
    0x02e6, /* Wireless XBox Controller Dongle */
    0x02ea, /* Xbox One S Controller */
    0x0719, /* Xbox 360 Wireless Adapter */
};

struct pnp_device
{
    struct list entry;
    DEVICE_OBJECT *device;
};

struct device_extension
{
    struct pnp_device *pnp_device;

    WORD vid, pid;
    DWORD uid, version, index;
    BOOL is_gamepad;
    WCHAR *serial;
    const WCHAR *busid;  /* Expected to be a static constant */

    const platform_vtbl *vtbl;

    BYTE *last_report;
    DWORD last_report_size;
    BOOL last_report_read;
    DWORD buffer_size;
    LIST_ENTRY irp_queue;
    CRITICAL_SECTION report_cs;

    BYTE platform_private[1];
};

static CRITICAL_SECTION device_list_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &device_list_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": device_list_cs") }
};
static CRITICAL_SECTION device_list_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static struct list pnp_devset = LIST_INIT(pnp_devset);

static const WCHAR zero_serialW[]= {'0','0','0','0',0};
static const WCHAR imW[] = {'I','M',0};
static const WCHAR igW[] = {'I','G',0};

static inline WCHAR *strdupW(const WCHAR *src)
{
    WCHAR *dst;
    if (!src) return NULL;
    dst = HeapAlloc(GetProcessHeap(), 0, (strlenW(src) + 1)*sizeof(WCHAR));
    if (dst) strcpyW(dst, src);
    return dst;
}

void *get_platform_private(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    return ext->platform_private;
}

static DWORD get_vidpid_index(WORD vid, WORD pid)
{
    struct pnp_device *ptr;
    DWORD index = 1;

    LIST_FOR_EACH_ENTRY(ptr, &pnp_devset, struct pnp_device, entry)
    {
        struct device_extension *ext = (struct device_extension *)ptr->device->DeviceExtension;
        if (ext->vid == vid && ext->pid == pid)
            index = max(ext->index + 1, index);
    }

    return index;
}

static WCHAR *get_instance_id(DEVICE_OBJECT *device)
{
    static const WCHAR formatW[] =  {'%','s','\\','V','i','d','_','%','0','4','x','&', 'P','i','d','_','%','0','4','x','&',
                                     '%','s','_','%','i','\\','%','i','&','%','s','&','%','x',0};
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    const WCHAR *serial = ext->serial ? ext->serial : zero_serialW;
    DWORD len = strlenW(ext->busid) + strlenW(serial) + 64;
    WCHAR *dst;

    if ((dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        sprintfW(dst, formatW, ext->busid, ext->vid, ext->pid, ext->is_gamepad ? igW : imW,
                 ext->index, ext->version, serial, ext->uid);

    return dst;
}

static WCHAR *get_device_id(DEVICE_OBJECT *device)
{
    static const WCHAR formatW[] =  {'%','s','\\','V','i','d','_','%','0','4','x','&','P','i','d','_','%','0','4','x',0};
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD len = strlenW(ext->busid) + 19;
    WCHAR *dst;

    if ((dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        sprintfW(dst, formatW, ext->busid, ext->vid, ext->pid);

    return dst;
}

static WCHAR *get_compatible_ids(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    WCHAR *iid, *did, *dst, *ptr;
    DWORD len;

    if (!(iid = get_instance_id(device)))
        return NULL;

    if (!(did = get_device_id(device)))
    {
        HeapFree(GetProcessHeap(), 0, iid);
        return NULL;
    }

    len = strlenW(iid) + strlenW(did) + strlenW(ext->busid) + 4;
    if ((dst = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
    {
        ptr = dst;
        strcpyW(ptr, iid);
        ptr += strlenW(iid) + 1;
        strcpyW(ptr, did);
        ptr += strlenW(did) + 1;
        strcpyW(ptr, ext->busid);
        ptr += strlenW(ext->busid) + 1;
        *ptr = 0;
    }

    HeapFree(GetProcessHeap(), 0, iid);
    HeapFree(GetProcessHeap(), 0, did);
    return dst;
}

DEVICE_OBJECT *bus_create_hid_device(DRIVER_OBJECT *driver, const WCHAR *busidW, WORD vid, WORD pid,
                                     DWORD version, DWORD uid, const WCHAR *serialW, BOOL is_gamepad,
                                     const GUID *class, const platform_vtbl *vtbl, DWORD platform_data_size)
{
    static const WCHAR device_name_fmtW[] = {'\\','D','e','v','i','c','e','\\','%','s','#','%','p',0};
    struct device_extension *ext;
    struct pnp_device *pnp_dev;
    DEVICE_OBJECT *device;
    UNICODE_STRING nameW;
    WCHAR dev_name[256];
    HDEVINFO devinfo;
    NTSTATUS status;
    DWORD length;

    TRACE("(%p, %s, %04x, %04x, %u, %u, %s, %u, %s, %p, %u)\n", driver, debugstr_w(busidW), vid, pid,
          version, uid, debugstr_w(serialW), is_gamepad, debugstr_guid(class), vtbl, platform_data_size);

    if (!(pnp_dev = HeapAlloc(GetProcessHeap(), 0, sizeof(*pnp_dev))))
        return NULL;

    sprintfW(dev_name, device_name_fmtW, busidW, pnp_dev);
    RtlInitUnicodeString(&nameW, dev_name);
    length = FIELD_OFFSET(struct device_extension, platform_private[platform_data_size]);
    status = IoCreateDevice(driver, length, &nameW, 0, 0, FALSE, &device);
    if (status)
    {
        FIXME("failed to create device error %x\n", status);
        HeapFree(GetProcessHeap(), 0, pnp_dev);
        return NULL;
    }

    EnterCriticalSection(&device_list_cs);

    /* fill out device_extension struct */
    ext = (struct device_extension *)device->DeviceExtension;
    ext->pnp_device         = pnp_dev;
    ext->vid                = vid;
    ext->pid                = pid;
    ext->uid                = uid;
    ext->version            = version;
    ext->index              = get_vidpid_index(vid, pid);
    ext->is_gamepad         = is_gamepad;
    ext->serial             = strdupW(serialW);
    ext->busid              = busidW;
    ext->vtbl               = vtbl;
    ext->last_report        = NULL;
    ext->last_report_size   = 0;
    ext->last_report_read   = TRUE;
    ext->buffer_size        = 0;

    memset(ext->platform_private, 0, platform_data_size);

    InitializeListHead(&ext->irp_queue);
    InitializeCriticalSection(&ext->report_cs);
    ext->report_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": report_cs");

    /* add to list of pnp devices */
    pnp_dev->device = device;
    list_add_tail(&pnp_devset, &pnp_dev->entry);

    LeaveCriticalSection(&device_list_cs);

    devinfo = SetupDiGetClassDevsW(class, NULL, NULL, DIGCF_DEVICEINTERFACE);
    if (devinfo)
    {
        SP_DEVINFO_DATA data;
        WCHAR *instance;

        data.cbSize = sizeof(data);
        if (!(instance = get_instance_id(device)))
            ERR("failed to generate instance id\n");
        else if (!SetupDiCreateDeviceInfoW(devinfo, instance, class, NULL, NULL, DICD_INHERIT_CLASSDRVS, &data))
            ERR("failed to create device info: %x\n", GetLastError());
        else if (!SetupDiRegisterDeviceInfo(devinfo, &data, 0, NULL, NULL, NULL))
            ERR("failed to register device info: %x\n", GetLastError());

        HeapFree(GetProcessHeap(), 0, instance);
        SetupDiDestroyDeviceInfoList(devinfo);
    }
    else
        ERR("failed to get ClassDevs: %x\n", GetLastError());

    return device;
}

DEVICE_OBJECT *bus_find_hid_device(const platform_vtbl *vtbl, void *platform_dev)
{
    struct pnp_device *dev;
    DEVICE_OBJECT *ret = NULL;

    TRACE("(%p, %p)\n", vtbl, platform_dev);

    EnterCriticalSection(&device_list_cs);
    LIST_FOR_EACH_ENTRY(dev, &pnp_devset, struct pnp_device, entry)
    {
        struct device_extension *ext = (struct device_extension *)dev->device->DeviceExtension;
        if (ext->vtbl != vtbl) continue;
        if (ext->vtbl->compare_platform_device(dev->device, platform_dev) == 0)
        {
            ret = dev->device;
            break;
        }
    }
    LeaveCriticalSection(&device_list_cs);

    TRACE("returning %p\n", ret);
    return ret;
}

DEVICE_OBJECT* bus_enumerate_hid_devices(const platform_vtbl *vtbl, enum_func function, void* context)
{
    struct pnp_device *dev;
    DEVICE_OBJECT *ret = NULL;

    TRACE("(%p)\n", vtbl);

    EnterCriticalSection(&device_list_cs);
    LIST_FOR_EACH_ENTRY(dev, &pnp_devset, struct pnp_device, entry)
    {
        struct device_extension *ext = (struct device_extension *)dev->device->DeviceExtension;
        if (ext->vtbl != vtbl) continue;
        if (function(dev->device, context) == 0)
        {
            ret = dev->device;
            break;
        }
    }
    LeaveCriticalSection(&device_list_cs);
    return ret;
}

void bus_remove_hid_device(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    struct pnp_device *pnp_device = ext->pnp_device;
    LIST_ENTRY *entry;
    IRP *irp;

    TRACE("(%p)\n", device);

    EnterCriticalSection(&device_list_cs);
    list_remove(&pnp_device->entry);
    LeaveCriticalSection(&device_list_cs);

    /* Cancel pending IRPs */
    EnterCriticalSection(&ext->report_cs);
    while ((entry = RemoveHeadList(&ext->irp_queue)) != &ext->irp_queue)
    {
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.s.ListEntry);
        irp->IoStatus.u.Status = STATUS_CANCELLED;
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    LeaveCriticalSection(&ext->report_cs);

    ext->report_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&ext->report_cs);

    HeapFree(GetProcessHeap(), 0, ext->serial);
    HeapFree(GetProcessHeap(), 0, ext->last_report);
    IoDeleteDevice(device);

    /* pnp_device must be released after the device is gone */
    HeapFree(GetProcessHeap(), 0, pnp_device);
}

static NTSTATUS handle_IRP_MN_QUERY_ID(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation( irp );
    BUS_QUERY_ID_TYPE type = irpsp->Parameters.QueryId.IdType;

    TRACE("(%p, %p)\n", device, irp);

    switch (type)
    {
        case BusQueryHardwareIDs:
            TRACE("BusQueryHardwareIDs\n");
            irp->IoStatus.Information = (ULONG_PTR)get_compatible_ids(device);
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
            FIXME("Unhandled type %08x\n", type);
            return status;
    }

    status = irp->IoStatus.Information ? STATUS_SUCCESS : STATUS_NO_MEMORY;
    return status;
}

NTSTATUS WINAPI common_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);

    switch (irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            TRACE("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            break;
        case IRP_MN_QUERY_ID:
            TRACE("IRP_MN_QUERY_ID\n");
            status = handle_IRP_MN_QUERY_ID(device, irp);
            irp->IoStatus.u.Status = status;
            break;
        case IRP_MN_QUERY_CAPABILITIES:
            TRACE("IRP_MN_QUERY_CAPABILITIES\n");
            break;
        default:
            FIXME("Unhandled function %08x\n", irpsp->MinorFunction);
            break;
    }

    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

static NTSTATUS deliver_last_report(struct device_extension *ext, DWORD buffer_length, BYTE* buffer, ULONG_PTR *out_length)
{
    if (buffer_length < ext->last_report_size)
    {
        *out_length = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        if (ext->last_report)
            memcpy(buffer, ext->last_report, ext->last_report_size);
        *out_length = ext->last_report_size;
        return STATUS_SUCCESS;
    }
}

NTSTATUS WINAPI hid_internal_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;

    TRACE("(%p, %p)\n", device, irp);

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        {
            HID_DEVICE_ATTRIBUTES *attr = (HID_DEVICE_ATTRIBUTES *)irp->UserBuffer;
            TRACE("IOCTL_HID_GET_DEVICE_ATTRIBUTES\n");

            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*attr))
            {
                irp->IoStatus.u.Status = status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            memset(attr, 0, sizeof(*attr));
            attr->Size = sizeof(*attr);
            attr->VendorID = ext->vid;
            attr->ProductID = ext->pid;
            attr->VersionNumber = ext->version;

            irp->IoStatus.u.Status = status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(*attr);
            break;
        }
        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        {
            HID_DESCRIPTOR *descriptor = (HID_DESCRIPTOR *)irp->UserBuffer;
            DWORD length;
            TRACE("IOCTL_HID_GET_DEVICE_DESCRIPTOR\n");

            if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*descriptor))
            {
                irp->IoStatus.u.Status = status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = ext->vtbl->get_reportdescriptor(device, NULL, 0, &length);
            if (status != STATUS_SUCCESS && status != STATUS_BUFFER_TOO_SMALL)
            {
                WARN("Failed to get platform report descriptor length\n");
                irp->IoStatus.u.Status = status;
                break;
            }

            memset(descriptor, 0, sizeof(*descriptor));
            descriptor->bLength = sizeof(*descriptor);
            descriptor->bDescriptorType = HID_HID_DESCRIPTOR_TYPE;
            descriptor->bcdHID = HID_REVISION;
            descriptor->bCountry = 0;
            descriptor->bNumDescriptors = 1;
            descriptor->DescriptorList[0].bReportType = HID_REPORT_DESCRIPTOR_TYPE;
            descriptor->DescriptorList[0].wReportLength = length;

            irp->IoStatus.u.Status = status = STATUS_SUCCESS;
            irp->IoStatus.Information = sizeof(*descriptor);
            break;
        }
        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        {
            DWORD length = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
            TRACE("IOCTL_HID_GET_REPORT_DESCRIPTOR\n");

            irp->IoStatus.u.Status = status = ext->vtbl->get_reportdescriptor(device, irp->UserBuffer, length, &length);
            irp->IoStatus.Information = length;
            break;
        }
        case IOCTL_HID_GET_STRING:
        {
            DWORD length = irpsp->Parameters.DeviceIoControl.OutputBufferLength / sizeof(WCHAR);
            DWORD index = (ULONG_PTR)irpsp->Parameters.DeviceIoControl.Type3InputBuffer;
            TRACE("IOCTL_HID_GET_STRING[%08x]\n", index);

            irp->IoStatus.u.Status = status = ext->vtbl->get_string(device, index, (WCHAR *)irp->UserBuffer, length);
            if (status == STATUS_SUCCESS)
                irp->IoStatus.Information = (strlenW((WCHAR *)irp->UserBuffer) + 1) * sizeof(WCHAR);
            break;
        }
        case IOCTL_HID_GET_INPUT_REPORT:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET*)(irp->UserBuffer);
            TRACE_(hid_report)("IOCTL_HID_GET_INPUT_REPORT\n");
            EnterCriticalSection(&ext->report_cs);
            status = ext->vtbl->begin_report_processing(device);
            if (status != STATUS_SUCCESS)
            {
                irp->IoStatus.u.Status = status;
                LeaveCriticalSection(&ext->report_cs);
                break;
            }

            irp->IoStatus.u.Status = status = deliver_last_report(ext,
                packet->reportBufferLen, packet->reportBuffer,
                &irp->IoStatus.Information);

            if (status == STATUS_SUCCESS)
                packet->reportBufferLen = irp->IoStatus.Information;
            LeaveCriticalSection(&ext->report_cs);
            break;
        }
        case IOCTL_HID_READ_REPORT:
        {
            TRACE_(hid_report)("IOCTL_HID_READ_REPORT\n");
            EnterCriticalSection(&ext->report_cs);
            status = ext->vtbl->begin_report_processing(device);
            if (status != STATUS_SUCCESS)
            {
                irp->IoStatus.u.Status = status;
                LeaveCriticalSection(&ext->report_cs);
                break;
            }
            if (!ext->last_report_read)
            {
                irp->IoStatus.u.Status = status = deliver_last_report(ext,
                    irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                    irp->UserBuffer, &irp->IoStatus.Information);
                ext->last_report_read = TRUE;
            }
            else
            {
                InsertTailList(&ext->irp_queue, &irp->Tail.Overlay.s.ListEntry);
                status = STATUS_PENDING;
            }
            LeaveCriticalSection(&ext->report_cs);
            break;
        }
        case IOCTL_HID_SET_OUTPUT_REPORT:
        case IOCTL_HID_WRITE_REPORT:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET*)(irp->UserBuffer);
            TRACE_(hid_report)("IOCTL_HID_WRITE_REPORT / IOCTL_HID_SET_OUTPUT_REPORT\n");
            irp->IoStatus.u.Status = status = ext->vtbl->set_output_report(
                device, packet->reportId, packet->reportBuffer,
                packet->reportBufferLen, &irp->IoStatus.Information);
            break;
        }
        case IOCTL_HID_GET_FEATURE:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET*)(irp->UserBuffer);
            TRACE_(hid_report)("IOCTL_HID_GET_FEATURE\n");
            irp->IoStatus.u.Status = status = ext->vtbl->get_feature_report(
                device, packet->reportId, packet->reportBuffer,
                packet->reportBufferLen, &irp->IoStatus.Information);
            packet->reportBufferLen = irp->IoStatus.Information;
            break;
        }
        case IOCTL_HID_SET_FEATURE:
        {
            HID_XFER_PACKET *packet = (HID_XFER_PACKET*)(irp->UserBuffer);
            TRACE_(hid_report)("IOCTL_HID_SET_FEATURE\n");
            irp->IoStatus.u.Status = status = ext->vtbl->set_feature_report(
                device, packet->reportId, packet->reportBuffer,
                packet->reportBufferLen, &irp->IoStatus.Information);
            break;
        }
        default:
        {
            ULONG code = irpsp->Parameters.DeviceIoControl.IoControlCode;
            FIXME("Unsupported ioctl %x (device=%x access=%x func=%x method=%x)\n",
                  code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xfff, code & 3);
            break;
        }
    }

    if (status != STATUS_PENDING)
        IoCompleteRequest(irp, IO_NO_INCREMENT);

    return status;
}

void process_hid_report(DEVICE_OBJECT *device, BYTE *report, DWORD length)
{
    struct device_extension *ext = (struct device_extension*)device->DeviceExtension;
    IRP *irp;
    LIST_ENTRY *entry;

    if (!length || !report)
        return;

    EnterCriticalSection(&ext->report_cs);
    if (length > ext->buffer_size)
    {
        HeapFree(GetProcessHeap(), 0, ext->last_report);
        ext->last_report = HeapAlloc(GetProcessHeap(), 0, length);
        if (!ext->last_report)
        {
            ERR_(hid_report)("Failed to alloc last report\n");
            ext->buffer_size = 0;
            ext->last_report_size = 0;
            ext->last_report_read = TRUE;
            LeaveCriticalSection(&ext->report_cs);
            return;
        }
        else
            ext->buffer_size = length;
    }

    if (!ext->last_report_read)
        ERR_(hid_report)("Device reports coming in too fast, last report not read yet!\n");

    memcpy(ext->last_report, report, length);
    ext->last_report_size = length;
    ext->last_report_read = FALSE;

    while ((entry = RemoveHeadList(&ext->irp_queue)) != &ext->irp_queue)
    {
        IO_STACK_LOCATION *irpsp;
        TRACE_(hid_report)("Processing Request\n");
        irp = CONTAINING_RECORD(entry, IRP, Tail.Overlay.s.ListEntry);
        irpsp = IoGetCurrentIrpStackLocation(irp);
        irp->IoStatus.u.Status = deliver_last_report(ext,
            irpsp->Parameters.DeviceIoControl.OutputBufferLength,
            irp->UserBuffer, &irp->IoStatus.Information);
        ext->last_report_read = TRUE;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    LeaveCriticalSection(&ext->report_cs);
}

DWORD check_bus_option(UNICODE_STRING *registry_path, const UNICODE_STRING *option, DWORD default_value)
{
    OBJECT_ATTRIBUTES attr;
    HANDLE key;
    DWORD output = default_value;

    InitializeObjectAttributes(&attr, registry_path, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    if (NtOpenKey(&key, KEY_ALL_ACCESS, &attr) == STATUS_SUCCESS)
    {
        DWORD size;
        char buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];

        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION*)buffer;

        if (NtQueryValueKey(key, option, KeyValuePartialInformation, info, sizeof(buffer), &size) == STATUS_SUCCESS)
        {
            if (info->Type == REG_DWORD)
                output = *(DWORD*)info->Data;
        }

        NtClose(key);
    }

    return output;
}

BOOL is_xbox_gamepad(WORD vid, WORD pid)
{
    int i;

    if (vid != VID_MICROSOFT)
        return FALSE;

    for (i = 0; i < sizeof(PID_XBOX_CONTROLLERS)/sizeof(*PID_XBOX_CONTROLLERS); i++)
        if (pid == PID_XBOX_CONTROLLERS[i]) return TRUE;

    return FALSE;
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR udevW[] = {'\\','D','r','i','v','e','r','\\','U','D','E','V',0};
    static UNICODE_STRING udev = {sizeof(udevW) - sizeof(WCHAR), sizeof(udevW), (WCHAR *)udevW};
    static const WCHAR iohidW[] = {'\\','D','r','i','v','e','r','\\','I','O','H','I','D',0};
    static UNICODE_STRING iohid = {sizeof(iohidW) - sizeof(WCHAR), sizeof(iohidW), (WCHAR *)iohidW};
    static const WCHAR sdlW[] = {'\\','D','r','i','v','e','r','\\','S','D','L','J','O','Y',0};
    static UNICODE_STRING sdl = {sizeof(sdlW) - sizeof(WCHAR), sizeof(sdlW), (WCHAR *)sdlW};
    static const WCHAR SDL_enabledW[] = {'E','n','a','b','l','e',' ','S','D','L',0};
    static const UNICODE_STRING SDL_enabled = {sizeof(SDL_enabledW) - sizeof(WCHAR), sizeof(SDL_enabledW), (WCHAR*)SDL_enabledW};

    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    if (check_bus_option(path, &SDL_enabled, 1))
    {
        if (IoCreateDriver(&sdl, sdl_driver_init) == STATUS_SUCCESS)
            return STATUS_SUCCESS;
    }
    IoCreateDriver(&udev, udev_driver_init);
    IoCreateDriver(&iohid, iohid_driver_init);

    return STATUS_SUCCESS;
}
