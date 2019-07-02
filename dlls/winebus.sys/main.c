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
#include "cfgmgr32.h"
#include "winioctl.h"
#include "hidusage.h"
#include "ddk/wdm.h"
#include "ddk/hidport.h"
#include "ddk/hidtypes.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "bus.h"
#include "controller.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);
WINE_DECLARE_DEBUG_CHANNEL(hid_report);

static const WCHAR backslashW[] = {'\\',0};

#define VID_MICROSOFT 0x045e

static const WORD PID_XBOX_CONTROLLERS[] =  {
    0x0202, /* Xbox Controller */
    0x0285, /* Xbox Controller S */
    0x0289, /* Xbox Controller S */
    0x028e, /* Xbox360 Controller */
    0x028f, /* Xbox360 Wireless Controller */
    0x02d1, /* Xbox One Controller */
    0x02dd, /* Xbox One Controller (Covert Forces/Firmware 2015) */
    0x02e0, /* Xbox One X Controller */
    0x02e3, /* Xbox One Elite Controller */
    0x02e6, /* Wireless XBox Controller Dongle */
    0x02ea, /* Xbox One S Controller */
    0x02fd, /* Xbox One S Controller (Firmware 2017) */
    0x0719, /* Xbox 360 Wireless Adapter */
};

static DRIVER_OBJECT *driver_obj;

static DEVICE_OBJECT *mouse_obj;

HANDLE driver_key;

struct pnp_device
{
    struct list entry;
    DEVICE_OBJECT *device;
};

struct device_extension
{
    struct pnp_device *pnp_device;

    WORD vid, pid, input;
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
static const WCHAR miW[] = {'M','I',0};
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

static DWORD get_device_index(WORD vid, WORD pid, WORD input)
{
    struct pnp_device *ptr;
    DWORD index = 0;

    LIST_FOR_EACH_ENTRY(ptr, &pnp_devset, struct pnp_device, entry)
    {
        struct device_extension *ext = (struct device_extension *)ptr->device->DeviceExtension;
        if (ext->vid == vid && ext->pid == pid && ext->input == input)
            index = max(ext->index + 1, index);
    }

    return index;
}

static WCHAR *get_instance_id(DEVICE_OBJECT *device)
{
    static const WCHAR formatW[] =  {'%','i','&','%','s','&','%','x','&','%','i',0};
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    const WCHAR *serial = ext->serial ? ext->serial : zero_serialW;
    DWORD len = strlenW(serial) + 33;
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, len * sizeof(WCHAR))))
        sprintfW(dst, formatW, ext->version, serial, ext->uid, ext->index);

    return dst;
}

static WCHAR *get_device_id(DEVICE_OBJECT *device)
{
    static const WCHAR formatW[] = {'%','s','\\','v','i','d','_','%','0','4','x',
            '&','p','i','d','_','%','0','4','x',0};
    static const WCHAR format_inputW[] = {'%','s','\\','v','i','d','_','%','0','4','x',
            '&','p','i','d','_','%','0','4','x','&','%','s','_','%','i',0};
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    DWORD len = strlenW(ext->busid) + 34;
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, len * sizeof(WCHAR))))
    {
        if (ext->input == (WORD)-1)
        {
            sprintfW(dst, formatW, ext->busid, ext->vid, ext->pid);
        }
        else
        {
            sprintfW(dst, format_inputW, ext->busid, ext->vid, ext->pid,
                    ext->is_gamepad ? igW : miW, ext->input);
        }
    }

    return dst;
}

static WCHAR *get_compatible_ids(DEVICE_OBJECT *device)
{
    struct device_extension *ext = (struct device_extension *)device->DeviceExtension;
    WCHAR *dst;

    if ((dst = ExAllocatePool(PagedPool, (strlenW(ext->busid) + 2) * sizeof(WCHAR))))
    {
        strcpyW(dst, ext->busid);
        dst[strlenW(dst) + 1] = 0;
    }

    return dst;
}

DEVICE_OBJECT *bus_create_hid_device(const WCHAR *busidW, WORD vid, WORD pid,
                                     WORD input, DWORD version, DWORD uid, const WCHAR *serialW, BOOL is_gamepad,
                                     const GUID *class, const platform_vtbl *vtbl, DWORD platform_data_size)
{
    static const WCHAR device_name_fmtW[] = {'\\','D','e','v','i','c','e','\\','%','s','#','%','p',0};
    WCHAR *id, instance[MAX_DEVICE_ID_LEN];
    struct device_extension *ext;
    struct pnp_device *pnp_dev;
    DEVICE_OBJECT *device;
    UNICODE_STRING nameW;
    WCHAR dev_name[256];
    HDEVINFO devinfo;
    SP_DEVINFO_DATA data = {sizeof(data)};
    NTSTATUS status;
    DWORD length;

    TRACE("(%s, %04x, %04x, %04x, %u, %u, %s, %u, %s, %p, %u)\n",
            debugstr_w(busidW), vid, pid, input, version, uid, debugstr_w(serialW),
            is_gamepad, debugstr_guid(class), vtbl, platform_data_size);

    if (!(pnp_dev = HeapAlloc(GetProcessHeap(), 0, sizeof(*pnp_dev))))
        return NULL;

    sprintfW(dev_name, device_name_fmtW, busidW, pnp_dev);
    RtlInitUnicodeString(&nameW, dev_name);
    length = FIELD_OFFSET(struct device_extension, platform_private[platform_data_size]);
    status = IoCreateDevice(driver_obj, length, &nameW, 0, 0, FALSE, &device);
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
    ext->input              = input;
    ext->uid                = uid;
    ext->version            = version;
    ext->index              = get_device_index(vid, pid, input);
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

    devinfo = SetupDiCreateDeviceInfoList(class, NULL);
    if (devinfo == INVALID_HANDLE_VALUE)
    {
        ERR("failed to create device info list, error %#x\n", GetLastError());
        goto error;
    }

    if (!(id = get_device_id(device)))
    {
        ERR("failed to generate instance id\n");
        goto error;
    }
    strcpyW(instance, id);
    ExFreePool(id);

    if (!(id = get_instance_id(device)))
    {
        ERR("failed to generate instance id\n");
        goto error;
    }
    strcatW(instance, backslashW);
    strcatW(instance, id);
    ExFreePool(id);

    if (SetupDiCreateDeviceInfoW(devinfo, instance, class, NULL, NULL, DICD_INHERIT_CLASSDRVS, &data))
    {
        if (!SetupDiRegisterDeviceInfo(devinfo, &data, 0, NULL, NULL, NULL))
            ERR("failed to register device info, error %#x\n", GetLastError());
    }
    else if (GetLastError() != ERROR_DEVINST_ALREADY_EXISTS)
        ERR("failed to create device info, error %#x\n", GetLastError());

error:
    SetupDiDestroyDeviceInfoList(devinfo);
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

static NTSTATUS build_device_relations(DEVICE_RELATIONS **devices)
{
    int i;
    struct pnp_device *ptr;

    EnterCriticalSection(&device_list_cs);
    *devices = ExAllocatePool(PagedPool, offsetof(DEVICE_RELATIONS, Objects[list_count(&pnp_devset)]));

    if (!*devices)
    {
        LeaveCriticalSection(&device_list_cs);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(ptr, &pnp_devset, struct pnp_device, entry)
    {
        (*devices)->Objects[i] = ptr->device;
        i++;
    }
    LeaveCriticalSection(&device_list_cs);
    (*devices)->Count = i;
    return STATUS_SUCCESS;
}

static NTSTATUS handle_IRP_MN_QUERY_DEVICE_RELATIONS(IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
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

static NTSTATUS WINAPI common_pnp_dispatch(DEVICE_OBJECT *device, IRP *irp)
{
    NTSTATUS status = irp->IoStatus.u.Status;
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);

    switch (irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            TRACE("IRP_MN_QUERY_DEVICE_RELATIONS\n");
            status = handle_IRP_MN_QUERY_DEVICE_RELATIONS(irp);
            irp->IoStatus.u.Status = status;
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

static NTSTATUS WINAPI hid_internal_dispatch(DEVICE_OBJECT *device, IRP *irp)
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

DWORD check_bus_option(const UNICODE_STRING *option, DWORD default_value)
{
    char buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION*)buffer;
    DWORD size;

    if (NtQueryValueKey(driver_key, option, KeyValuePartialInformation, info, sizeof(buffer), &size) == STATUS_SUCCESS)
    {
        if (info->Type == REG_DWORD)
            return *(DWORD*)info->Data;
    }

    return default_value;
}

BOOL is_xbox_gamepad(WORD vid, WORD pid)
{
    int i;

    if (vid != VID_MICROSOFT)
        return FALSE;

    for (i = 0; i < ARRAY_SIZE(PID_XBOX_CONTROLLERS); i++)
        if (pid == PID_XBOX_CONTROLLERS[i]) return TRUE;

    return FALSE;
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    udev_driver_unload();
    iohid_driver_unload();
    sdl_driver_unload();
    NtClose(driver_key);
}

static NTSTATUS mouse_get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *ret_length)
{
    TRACE("buffer %p, length %u.\n", buffer, length);

    *ret_length = sizeof(REPORT_HEADER) + sizeof(REPORT_TAIL);
    if (length < sizeof(REPORT_HEADER) + sizeof(REPORT_TAIL))
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, REPORT_HEADER, sizeof(REPORT_HEADER));
    memcpy(buffer + sizeof(REPORT_HEADER), REPORT_TAIL, sizeof(REPORT_TAIL));
    buffer[IDX_HEADER_PAGE] = HID_USAGE_PAGE_GENERIC;
    buffer[IDX_HEADER_USAGE] = HID_USAGE_GENERIC_MOUSE;

    return STATUS_SUCCESS;
}

static NTSTATUS mouse_get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    static const WCHAR nameW[] = {'W','i','n','e',' ','H','I','D',' ','m','o','u','s','e',0};
    if (index != HID_STRING_ID_IPRODUCT)
        return STATUS_NOT_IMPLEMENTED;
    if (length < ARRAY_SIZE(nameW))
        return STATUS_BUFFER_TOO_SMALL;
    strcpyW(buffer, nameW);
    return STATUS_SUCCESS;
}

static NTSTATUS mouse_begin_report_processing(DEVICE_OBJECT *device)
{
    return STATUS_SUCCESS;
}

static NTSTATUS mouse_set_output_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *ret_length)
{
    FIXME("id %u, stub!\n", id);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS mouse_get_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *ret_length)
{
    FIXME("id %u, stub!\n", id);
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS mouse_set_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *ret_length)
{
    FIXME("id %u, stub!\n", id);
    return STATUS_NOT_IMPLEMENTED;
}

static const platform_vtbl mouse_vtbl =
{
    .get_reportdescriptor = mouse_get_reportdescriptor,
    .get_string = mouse_get_string,
    .begin_report_processing = mouse_begin_report_processing,
    .set_output_report = mouse_set_output_report,
    .get_feature_report = mouse_get_feature_report,
    .set_feature_report = mouse_set_feature_report,
};

static void mouse_device_create(void)
{
    static const GUID wine_mouse_class = {0xdfe2580e,0x52fd,0x453d,{0xa2,0xc1,0x33,0x81,0xf2,0x32,0x68,0x4c}};
    static const WCHAR busidW[] = {'W','I','N','E','M','O','U','S','E',0};

    mouse_obj = bus_create_hid_device(busidW, 0, 0, -1, 0, 0, busidW, FALSE,
            &wine_mouse_class, &mouse_vtbl, 0);
    IoInvalidateDeviceRelations(mouse_obj, BusRelations);
}

NTSTATUS WINAPI DriverEntry( DRIVER_OBJECT *driver, UNICODE_STRING *path )
{
    static const WCHAR SDL_enabledW[] = {'E','n','a','b','l','e',' ','S','D','L',0};
    static const UNICODE_STRING SDL_enabled = {sizeof(SDL_enabledW) - sizeof(WCHAR), sizeof(SDL_enabledW), (WCHAR*)SDL_enabledW};
    OBJECT_ATTRIBUTES attr = {0};
    NTSTATUS ret;

    TRACE( "(%p, %s)\n", driver, debugstr_w(path->Buffer) );

    attr.Length = sizeof(attr);
    attr.ObjectName = path;
    attr.Attributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
    if ((ret = NtOpenKey(&driver_key, KEY_ALL_ACCESS, &attr)) != STATUS_SUCCESS)
        ERR("Failed to open driver key, status %#x.\n", ret);

    driver_obj = driver;

    driver->MajorFunction[IRP_MJ_PNP] = common_pnp_dispatch;
    driver->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = hid_internal_dispatch;
    driver->DriverUnload = driver_unload;

    mouse_device_create();

    if (check_bus_option(&SDL_enabled, 1))
    {
        if (sdl_driver_init() == STATUS_SUCCESS)
            return STATUS_SUCCESS;
    }
    udev_driver_init();
    iohid_driver_init();

    return STATUS_SUCCESS;
}
