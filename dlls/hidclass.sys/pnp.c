/*
 * Human Interface Device class driver
 *
 * Copyright 2015 Aric Stewart
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

#define NONAMELESSUNION
#include <unistd.h>
#include <stdarg.h>
#include "initguid.h"
#include "hid.h"
#include "devguid.h"
#include "devpkey.h"
#include "ntddmou.h"
#include "ntddkbd.h"
#include "ddk/hidtypes.h"
#include "ddk/wdm.h"
#include "regstr.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/asm.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(hid);

DEFINE_DEVPROPKEY(DEVPROPKEY_HID_HANDLE, 0xbc62e415, 0xf4fe, 0x405c, 0x8e, 0xda, 0x63, 0x6f, 0xb5, 0x9f, 0x08, 0x98, 2);

#if defined(__i386__) && !defined(_WIN32)

extern void * WINAPI wrap_fastcall_func1( void *func, const void *a );
__ASM_STDCALL_FUNC( wrap_fastcall_func1, 8,
                   "popl %ecx\n\t"
                   "popl %eax\n\t"
                   "xchgl (%esp),%ecx\n\t"
                   "jmp *%eax" );

#define call_fastcall_func1(func,a) wrap_fastcall_func1(func,a)

#else

#define call_fastcall_func1(func,a) func(a)

#endif

static struct list minidriver_list = LIST_INIT(minidriver_list);

static minidriver *find_minidriver(DRIVER_OBJECT *driver)
{
    minidriver *md;
    LIST_FOR_EACH_ENTRY(md, &minidriver_list, minidriver, entry)
    {
        if (md->minidriver.DriverObject == driver)
            return md;
    }
    return NULL;
}

static NTSTATUS get_device_id(DEVICE_OBJECT *device, BUS_QUERY_ID_TYPE type, WCHAR *id)
{
    IO_STACK_LOCATION *irpsp;
    IO_STATUS_BLOCK irp_status;
    KEVENT event;
    IRP *irp;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP, device, NULL, 0, NULL, &event, &irp_status);
    if (irp == NULL)
        return STATUS_NO_MEMORY;

    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MinorFunction = IRP_MN_QUERY_ID;
    irpsp->Parameters.QueryId.IdType = type;

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    wcscpy(id, (WCHAR *)irp_status.Information);
    ExFreePool((WCHAR *)irp_status.Information);
    return irp_status.u.Status;
}

/* user32 reserves 1 & 2 for winemouse and winekeyboard,
 * keep this in sync with user_private.h */
#define WINE_MOUSE_HANDLE 1
#define WINE_KEYBOARD_HANDLE 2

static UINT32 alloc_rawinput_handle(void)
{
    static LONG counter = WINE_KEYBOARD_HANDLE + 1;
    return InterlockedIncrement(&counter);
}

/* make sure bRawData can hold UsagePage and Usage without requiring additional allocation */
C_ASSERT(offsetof(RAWINPUT, data.hid.bRawData[2 * sizeof(USAGE)]) < sizeof(RAWINPUT));

static void send_wm_input_device_change(BASE_DEVICE_EXTENSION *ext, LPARAM param)
{
    RAWINPUT rawinput;
    INPUT input;

    rawinput.header.dwType = RIM_TYPEHID;
    rawinput.header.dwSize = offsetof(RAWINPUT, data.hid.bRawData[2 * sizeof(USAGE)]);
    rawinput.header.hDevice = ULongToHandle(ext->u.pdo.rawinput_handle);
    rawinput.header.wParam = param;
    rawinput.data.hid.dwCount = 0;
    rawinput.data.hid.dwSizeHid = 0;
    ((USAGE *)rawinput.data.hid.bRawData)[0] = ext->u.pdo.preparsed_data->caps.UsagePage;
    ((USAGE *)rawinput.data.hid.bRawData)[1] = ext->u.pdo.preparsed_data->caps.Usage;

    input.type = INPUT_HARDWARE;
    input.u.hi.uMsg = WM_INPUT_DEVICE_CHANGE;
    input.u.hi.wParamH = 0;
    input.u.hi.wParamL = 0;
    __wine_send_input(0, &input, &rawinput);
}

static NTSTATUS WINAPI driver_add_device(DRIVER_OBJECT *driver, DEVICE_OBJECT *bus_pdo)
{
    WCHAR device_id[MAX_DEVICE_ID_LEN], instance_id[MAX_DEVICE_ID_LEN];
    BASE_DEVICE_EXTENSION *ext;
    DEVICE_OBJECT *fdo;
    NTSTATUS status;
    minidriver *minidriver;

    if ((status = get_device_id(bus_pdo, BusQueryDeviceID, device_id)))
    {
        ERR("Failed to get PDO device id, status %#x.\n", status);
        return status;
    }

    if ((status = get_device_id(bus_pdo, BusQueryInstanceID, instance_id)))
    {
        ERR("Failed to get PDO instance id, status %#x.\n", status);
        return status;
    }

    TRACE("Adding device to PDO %p, id %s\\%s.\n", bus_pdo, debugstr_w(device_id), debugstr_w(instance_id));
    minidriver = find_minidriver(driver);

    if ((status = IoCreateDevice(driver, sizeof(*ext) + minidriver->minidriver.DeviceExtensionSize,
            NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &fdo)))
    {
        ERR("Failed to create bus FDO, status %#x.\n", status);
        return status;
    }
    ext = fdo->DeviceExtension;
    ext->is_fdo = TRUE;
    ext->u.fdo.hid_ext.MiniDeviceExtension = ext + 1;
    ext->u.fdo.hid_ext.PhysicalDeviceObject = bus_pdo;
    ext->u.fdo.hid_ext.NextDeviceObject = bus_pdo;
    swprintf(ext->device_id, ARRAY_SIZE(ext->device_id), L"HID\\%s", wcsrchr(device_id, '\\') + 1);
    wcscpy(ext->instance_id, instance_id);

    status = minidriver->AddDevice(minidriver->minidriver.DriverObject, fdo);
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver AddDevice failed (%x)\n",status);
        IoDeleteDevice(fdo);
        return status;
    }

    IoAttachDeviceToDeviceStack(fdo, bus_pdo);
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static void create_child(minidriver *minidriver, DEVICE_OBJECT *fdo)
{
    BASE_DEVICE_EXTENSION *fdo_ext = fdo->DeviceExtension, *pdo_ext;
    HID_DEVICE_ATTRIBUTES attr = {0};
    HID_DESCRIPTOR descriptor;
    DEVICE_OBJECT *child_pdo;
    BYTE *reportDescriptor;
    UNICODE_STRING string;
    WCHAR pdo_name[255];
    USAGE page, usage;
    NTSTATUS status;
    INT i;

    status = call_minidriver(IOCTL_HID_GET_DEVICE_ATTRIBUTES, fdo, NULL, 0, &attr, sizeof(attr));
    if (status != STATUS_SUCCESS)
    {
        ERR("Minidriver failed to get Attributes(%x)\n",status);
        return;
    }

    swprintf(pdo_name, ARRAY_SIZE(pdo_name), L"\\Device\\HID#%p&%p", fdo->DriverObject,
            fdo_ext->u.fdo.hid_ext.PhysicalDeviceObject);
    RtlInitUnicodeString(&string, pdo_name);
    if ((status = IoCreateDevice(fdo->DriverObject, sizeof(*pdo_ext), &string, 0, 0, FALSE, &child_pdo)))
    {
        ERR("Failed to create child PDO, status %#x.\n", status);
        return;
    }
    fdo_ext->u.fdo.child_pdo = child_pdo;

    pdo_ext = child_pdo->DeviceExtension;
    pdo_ext->u.pdo.parent_fdo = fdo;
    InitializeListHead(&pdo_ext->u.pdo.irp_queue);
    KeInitializeSpinLock(&pdo_ext->u.pdo.irp_queue_lock);
    wcscpy(pdo_ext->device_id, fdo_ext->device_id);
    wcscpy(pdo_ext->instance_id, fdo_ext->instance_id);

    pdo_ext->u.pdo.information.VendorID = attr.VendorID;
    pdo_ext->u.pdo.information.ProductID = attr.ProductID;
    pdo_ext->u.pdo.information.VersionNumber = attr.VersionNumber;
    pdo_ext->u.pdo.information.Polled = minidriver->minidriver.DevicesArePolled;

    status = call_minidriver(IOCTL_HID_GET_DEVICE_DESCRIPTOR, fdo, NULL, 0, &descriptor, sizeof(descriptor));
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Device Descriptor(%x)\n",status);
        IoDeleteDevice(child_pdo);
        return;
    }
    for (i = 0; i < descriptor.bNumDescriptors; i++)
        if (descriptor.DescriptorList[i].bReportType == HID_REPORT_DESCRIPTOR_TYPE)
            break;

    if (i >= descriptor.bNumDescriptors)
    {
        ERR("No Report Descriptor found in reply\n");
        IoDeleteDevice(child_pdo);
        return;
    }

    reportDescriptor = HeapAlloc(GetProcessHeap(), 0, descriptor.DescriptorList[i].wReportLength);
    status = call_minidriver(IOCTL_HID_GET_REPORT_DESCRIPTOR, fdo, NULL, 0,
        reportDescriptor, descriptor.DescriptorList[i].wReportLength);
    if (status != STATUS_SUCCESS)
    {
        ERR("Cannot get Report Descriptor(%x)\n",status);
        HeapFree(GetProcessHeap(), 0, reportDescriptor);
        IoDeleteDevice(child_pdo);
        return;
    }

    pdo_ext->u.pdo.preparsed_data = ParseDescriptor(reportDescriptor, descriptor.DescriptorList[i].wReportLength);
    HeapFree(GetProcessHeap(), 0, reportDescriptor);
    if (!pdo_ext->u.pdo.preparsed_data)
    {
        ERR("Cannot parse Report Descriptor\n");
        IoDeleteDevice(child_pdo);
        return;
    }

    pdo_ext->u.pdo.information.DescriptorSize = pdo_ext->u.pdo.preparsed_data->dwSize;

    page = pdo_ext->u.pdo.preparsed_data->caps.UsagePage;
    usage = pdo_ext->u.pdo.preparsed_data->caps.Usage;
    if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_MOUSE)
        pdo_ext->u.pdo.rawinput_handle = WINE_MOUSE_HANDLE;
    else if (page == HID_USAGE_PAGE_GENERIC && usage == HID_USAGE_GENERIC_KEYBOARD)
        pdo_ext->u.pdo.rawinput_handle = WINE_KEYBOARD_HANDLE;
    else
        pdo_ext->u.pdo.rawinput_handle = alloc_rawinput_handle();

    IoInvalidateDeviceRelations(fdo_ext->u.fdo.hid_ext.PhysicalDeviceObject, BusRelations);

    if ((status = IoSetDevicePropertyData(child_pdo, &DEVPROPKEY_HID_HANDLE, LOCALE_NEUTRAL,
                                          PLUGPLAY_PROPERTY_PERSISTENT, DEVPROP_TYPE_UINT32,
                                          sizeof(pdo_ext->u.pdo.rawinput_handle), &pdo_ext->u.pdo.rawinput_handle)))
    {
        ERR("Failed to set device handle property, status %x\n", status);
        IoDeleteDevice(child_pdo);
        return;
    }

    pdo_ext->u.pdo.poll_interval = DEFAULT_POLL_INTERVAL;

    pdo_ext->u.pdo.ring_buffer = RingBuffer_Create(
            sizeof(HID_XFER_PACKET) + pdo_ext->u.pdo.preparsed_data->caps.InputReportByteLength);

    HID_StartDeviceThread(child_pdo);

    send_wm_input_device_change(pdo_ext, GIDC_ARRIVAL);
}

static NTSTATUS fdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    minidriver *minidriver = find_minidriver(device->DriverObject);
    IO_STACK_LOCATION *stack = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    TRACE("irp %p, minor function %#x.\n", irp, stack->MinorFunction);

    switch (stack->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DEVICE_RELATIONS *devices;
            DEVICE_OBJECT *child;

            if (stack->Parameters.QueryDeviceRelations.Type != BusRelations)
                return minidriver->PNPDispatch(device, irp);

            if (!(devices = ExAllocatePool(PagedPool, offsetof(DEVICE_RELATIONS, Objects[1]))))
            {
                irp->IoStatus.u.Status = STATUS_NO_MEMORY;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return STATUS_NO_MEMORY;
            }

            if ((child = ext->u.fdo.child_pdo))
            {
                devices->Objects[0] = ext->u.fdo.child_pdo;
                call_fastcall_func1(ObfReferenceObject, ext->u.fdo.child_pdo);
                devices->Count = 1;
            }
            else
            {
                devices->Count = 0;
            }

            irp->IoStatus.Information = (ULONG_PTR)devices;
            irp->IoStatus.u.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(irp);
            return IoCallDriver(ext->u.fdo.hid_ext.NextDeviceObject, irp);
        }

        case IRP_MN_START_DEVICE:
        {
            NTSTATUS ret;

            if ((ret = minidriver->PNPDispatch(device, irp)))
                return ret;
            create_child(minidriver, device);
            return STATUS_SUCCESS;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            NTSTATUS ret;

            ret = minidriver->PNPDispatch(device, irp);

            IoDetachDevice(ext->u.fdo.hid_ext.NextDeviceObject);
            IoDeleteDevice(device);
            return ret;
        }

        default:
            return minidriver->PNPDispatch(device, irp);
    }
}

static NTSTATUS pdo_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    IO_STACK_LOCATION *irpsp = IoGetCurrentIrpStackLocation(irp);
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;
    NTSTATUS status = irp->IoStatus.u.Status;

    TRACE("irp %p, minor function %#x.\n", irp, irpsp->MinorFunction);

    switch(irpsp->MinorFunction)
    {
        case IRP_MN_QUERY_ID:
        {
            WCHAR *id = ExAllocatePool(PagedPool, sizeof(WCHAR) * REGSTR_VAL_MAX_HCID_LEN);
            TRACE("IRP_MN_QUERY_ID[%i]\n", irpsp->Parameters.QueryId.IdType);
            switch (irpsp->Parameters.QueryId.IdType)
            {
                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                {
                    WCHAR *ptr;
                    ptr = id;
                    /* Device instance ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id);
                    lstrcpyW(ptr, L"\\");
                    ptr += 1;
                    lstrcpyW(ptr, ext->instance_id);
                    ptr += lstrlenW(ext->instance_id) + 1;
                    /* Device ID */
                    lstrcpyW(ptr, ext->device_id);
                    ptr += lstrlenW(ext->device_id) + 1;
                    /* Bus ID */
                    lstrcpyW(ptr, L"HID");
                    ptr += lstrlenW(L"HID") + 1;
                    *ptr = 0;
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;
                }
                case BusQueryDeviceID:
                    lstrcpyW(id, ext->device_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;
                case BusQueryInstanceID:
                    lstrcpyW(id, ext->instance_id);
                    irp->IoStatus.Information = (ULONG_PTR)id;
                    status = STATUS_SUCCESS;
                    break;

                case BusQueryContainerID:
                case BusQueryDeviceSerialNumber:
                    FIXME("unimplemented id type %#x\n", irpsp->Parameters.QueryId.IdType);
                    ExFreePool(id);
                    break;
            }
            break;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            DEVICE_CAPABILITIES *caps = irpsp->Parameters.DeviceCapabilities.Capabilities;

            caps->RawDeviceOK = 1;
            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_START_DEVICE:
            if ((status = IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_HID, NULL, &ext->u.pdo.link_name)))
            {
                ERR("Failed to register interface, status %#x.\n", status);
                break;
            }

            /* FIXME: This should probably be done in mouhid.sys. */
            if (ext->u.pdo.preparsed_data->caps.UsagePage == HID_USAGE_PAGE_GENERIC
                    && ext->u.pdo.preparsed_data->caps.Usage == HID_USAGE_GENERIC_MOUSE)
            {
                if (!IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_MOUSE, NULL, &ext->u.pdo.mouse_link_name))
                    ext->u.pdo.is_mouse = TRUE;
            }
            if (ext->u.pdo.preparsed_data->caps.UsagePage == HID_USAGE_PAGE_GENERIC
                    && ext->u.pdo.preparsed_data->caps.Usage == HID_USAGE_GENERIC_KEYBOARD)
            {
                if (!IoRegisterDeviceInterface(device, &GUID_DEVINTERFACE_KEYBOARD, NULL, &ext->u.pdo.keyboard_link_name))
                    ext->u.pdo.is_keyboard = TRUE;
            }

            IoSetDeviceInterfaceState(&ext->u.pdo.link_name, TRUE);
            if (ext->u.pdo.is_mouse)
                IoSetDeviceInterfaceState(&ext->u.pdo.mouse_link_name, TRUE);
            if (ext->u.pdo.is_keyboard)
                IoSetDeviceInterfaceState(&ext->u.pdo.keyboard_link_name, TRUE);
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
        {
            IRP *queued_irp;

            send_wm_input_device_change(ext, GIDC_REMOVAL);

            IoSetDeviceInterfaceState(&ext->u.pdo.link_name, FALSE);
            if (ext->u.pdo.is_mouse)
                IoSetDeviceInterfaceState(&ext->u.pdo.mouse_link_name, FALSE);
            if (ext->u.pdo.is_keyboard)
                IoSetDeviceInterfaceState(&ext->u.pdo.keyboard_link_name, TRUE);

            if (ext->u.pdo.thread)
            {
                SetEvent(ext->u.pdo.halt_event);
                WaitForSingleObject(ext->u.pdo.thread, INFINITE);
            }
            CloseHandle(ext->u.pdo.halt_event);

            HeapFree(GetProcessHeap(), 0, ext->u.pdo.preparsed_data);
            if (ext->u.pdo.ring_buffer)
                RingBuffer_Destroy(ext->u.pdo.ring_buffer);

            while ((queued_irp = pop_irp_from_queue(ext)))
            {
                queued_irp->IoStatus.u.Status = STATUS_DEVICE_REMOVED;
                IoCompleteRequest(queued_irp, IO_NO_INCREMENT);
            }

            RtlFreeUnicodeString(&ext->u.pdo.link_name);

            irp->IoStatus.u.Status = STATUS_SUCCESS;
            IoCompleteRequest(irp, IO_NO_INCREMENT);
            IoDeleteDevice(device);
            return STATUS_SUCCESS;
        }

        case IRP_MN_SURPRISE_REMOVAL:
            status = STATUS_SUCCESS;
            break;

        default:
            FIXME("Unhandled minor function %#x.\n", irpsp->MinorFunction);
    }

    irp->IoStatus.u.Status = status;
    IoCompleteRequest( irp, IO_NO_INCREMENT );
    return status;
}

static NTSTATUS WINAPI driver_pnp(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (ext->is_fdo)
        return fdo_pnp(device, irp);
    else
        return pdo_pnp(device, irp);
}

static NTSTATUS WINAPI driver_create(DEVICE_OBJECT *device, IRP *irp)
{
    BASE_DEVICE_EXTENSION *ext = device->DeviceExtension;

    if (ext->is_fdo)
    {
        irp->IoStatus.u.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

    return pdo_create(device, irp);
}

static NTSTATUS WINAPI driver_close(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_close(device, irp);
}

static NTSTATUS WINAPI driver_ioctl(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_ioctl(device, irp);
}

static NTSTATUS WINAPI driver_read(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_read(device, irp);
}

static NTSTATUS WINAPI driver_write(DEVICE_OBJECT *device, IRP *irp)
{
    return pdo_write(device, irp);
}

static void WINAPI driver_unload(DRIVER_OBJECT *driver)
{
    minidriver *md;

    TRACE("\n");

    if ((md = find_minidriver(driver)))
    {
        if (md->DriverUnload)
            md->DriverUnload(md->minidriver.DriverObject);
        list_remove(&md->entry);
        HeapFree(GetProcessHeap(), 0, md);
    }
}

NTSTATUS WINAPI HidRegisterMinidriver(HID_MINIDRIVER_REGISTRATION *registration)
{
    minidriver *driver;

    if (!(driver = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*driver))))
        return STATUS_NO_MEMORY;

    driver->DriverUnload = registration->DriverObject->DriverUnload;
    registration->DriverObject->DriverUnload = driver_unload;

    registration->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver_ioctl;
    registration->DriverObject->MajorFunction[IRP_MJ_READ] = driver_read;
    registration->DriverObject->MajorFunction[IRP_MJ_WRITE] = driver_write;
    registration->DriverObject->MajorFunction[IRP_MJ_CREATE] = driver_create;
    registration->DriverObject->MajorFunction[IRP_MJ_CLOSE] = driver_close;

    driver->PNPDispatch = registration->DriverObject->MajorFunction[IRP_MJ_PNP];
    registration->DriverObject->MajorFunction[IRP_MJ_PNP] = driver_pnp;

    driver->AddDevice = registration->DriverObject->DriverExtension->AddDevice;
    registration->DriverObject->DriverExtension->AddDevice = driver_add_device;

    driver->minidriver = *registration;
    list_add_tail(&minidriver_list, &driver->entry);

    return STATUS_SUCCESS;
}

NTSTATUS call_minidriver(ULONG code, DEVICE_OBJECT *device, void *in_buff, ULONG in_size, void *out_buff, ULONG out_size)
{
    IRP *irp;
    IO_STATUS_BLOCK io;
    KEVENT event;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(code, device, in_buff, in_size,
        out_buff, out_size, TRUE, &event, &io);

    if (IoCallDriver(device, irp) == STATUS_PENDING)
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    return io.u.Status;
}
