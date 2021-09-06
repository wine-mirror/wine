/*  Bus like function for mac HID devices
 *
 * Copyright 2016 CodeWeavers, Aric Stewart
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
#include "wine/port.h"

#include <stdarg.h>

#if defined(HAVE_IOKIT_HID_IOHIDLIB_H)
#define DWORD UInt32
#define LPDWORD UInt32*
#define LONG SInt32
#define LPLONG SInt32*
#define E_PENDING __carbon_E_PENDING
#define ULONG __carbon_ULONG
#define E_INVALIDARG __carbon_E_INVALIDARG
#define E_OUTOFMEMORY __carbon_E_OUTOFMEMORY
#define E_HANDLE __carbon_E_HANDLE
#define E_ACCESSDENIED __carbon_E_ACCESSDENIED
#define E_UNEXPECTED __carbon_E_UNEXPECTED
#define E_FAIL __carbon_E_FAIL
#define E_ABORT __carbon_E_ABORT
#define E_POINTER __carbon_E_POINTER
#define E_NOINTERFACE __carbon_E_NOINTERFACE
#define E_NOTIMPL __carbon_E_NOTIMPL
#define S_FALSE __carbon_S_FALSE
#define S_OK __carbon_S_OK
#define HRESULT_FACILITY __carbon_HRESULT_FACILITY
#define IS_ERROR __carbon_IS_ERROR
#define FAILED __carbon_FAILED
#define SUCCEEDED __carbon_SUCCEEDED
#define MAKE_HRESULT __carbon_MAKE_HRESULT
#define HRESULT __carbon_HRESULT
#define STDMETHODCALLTYPE __carbon_STDMETHODCALLTYPE
#define PAGE_SHIFT __carbon_PAGE_SHIFT
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#undef ULONG
#undef E_INVALIDARG
#undef E_OUTOFMEMORY
#undef E_HANDLE
#undef E_ACCESSDENIED
#undef E_UNEXPECTED
#undef E_FAIL
#undef E_ABORT
#undef E_POINTER
#undef E_NOINTERFACE
#undef E_NOTIMPL
#undef S_FALSE
#undef S_OK
#undef HRESULT_FACILITY
#undef IS_ERROR
#undef FAILED
#undef SUCCEEDED
#undef MAKE_HRESULT
#undef HRESULT
#undef STDMETHODCALLTYPE
#undef DWORD
#undef LPDWORD
#undef LONG
#undef LPLONG
#undef E_PENDING
#undef PAGE_SHIFT
#endif /* HAVE_IOKIT_HID_IOHIDLIB_H */

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
#include "wine/debug.h"

#include "bus.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);
#ifdef HAVE_IOHIDMANAGERCREATE

static IOHIDManagerRef hid_manager;
static CFRunLoopRef run_loop;
static struct list event_queue = LIST_INIT(event_queue);

static const WCHAR busidW[] = {'I','O','H','I','D',0};
static struct iohid_bus_options options;

struct platform_private
{
    struct unix_device unix_device;
    IOHIDDeviceRef device;
    uint8_t *buffer;
};

static inline struct platform_private *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct platform_private, unix_device);
}

static void CFStringToWSTR(CFStringRef cstr, LPWSTR wstr, int length)
{
    int len = min(CFStringGetLength(cstr), length-1);
    CFStringGetCharacters(cstr, CFRangeMake(0, len), (UniChar*)wstr);
    wstr[len] = 0;
}

static DWORD CFNumberToDWORD(CFNumberRef num)
{
    int dwNum = 0;
    if (num)
        CFNumberGetValue(num, kCFNumberIntType, &dwNum);
    return dwNum;
}

static void handle_IOHIDDeviceIOHIDReportCallback(void *context,
        IOReturn result, void *sender, IOHIDReportType type,
        uint32_t reportID, uint8_t *report, CFIndex report_length)
{
    DEVICE_OBJECT *device = (DEVICE_OBJECT*)context;
    process_hid_report(device, report, report_length);
}

static void iohid_device_destroy(struct unix_device *iface)
{
    struct platform_private *private = impl_from_unix_device(iface);
    HeapFree(GetProcessHeap(), 0, private);
}

static int iohid_device_compare(struct unix_device *iface, void *context)
{
    struct platform_private *private = impl_from_unix_device(iface);
    IOHIDDeviceRef dev2 = (IOHIDDeviceRef)context;
    if (private->device != dev2)
        return 1;
    else
        return 0;
}

static NTSTATUS iohid_device_start(struct unix_device *iface, DEVICE_OBJECT *device)
{
    DWORD length;
    struct platform_private *private = impl_from_unix_device(iface);
    CFNumberRef num;

    num = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDMaxInputReportSizeKey));
    length = CFNumberToDWORD(num);
    private->buffer = HeapAlloc(GetProcessHeap(), 0, length);

    IOHIDDeviceRegisterInputReportCallback(private->device, private->buffer, length, handle_IOHIDDeviceIOHIDReportCallback, device);
    return STATUS_SUCCESS;
}

static NTSTATUS iohid_device_get_report_descriptor(struct unix_device *iface, BYTE *buffer,
                                                   DWORD length, DWORD *out_length)
{
    struct platform_private *private = impl_from_unix_device(iface);
    CFDataRef data = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDReportDescriptorKey));
    int data_length = CFDataGetLength(data);
    const UInt8 *ptr;

    *out_length = data_length;
    if (length < data_length)
        return STATUS_BUFFER_TOO_SMALL;

    ptr = CFDataGetBytePtr(data);
    memcpy(buffer, ptr, data_length);
    return STATUS_SUCCESS;
}

static NTSTATUS iohid_device_get_string(struct unix_device *iface, DWORD index, WCHAR *buffer, DWORD length)
{
    struct platform_private *private = impl_from_unix_device(iface);
    CFStringRef str;
    switch (index)
    {
        case HID_STRING_ID_IPRODUCT:
            str = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDProductKey));
            break;
        case HID_STRING_ID_IMANUFACTURER:
            str = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDManufacturerKey));
            break;
        case HID_STRING_ID_ISERIALNUMBER:
            str = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDSerialNumberKey));
            break;
        default:
            ERR("Unknown string index\n");
            return STATUS_NOT_IMPLEMENTED;
    }

    if (str)
    {
        if (length < CFStringGetLength(str) + 1)
            return STATUS_BUFFER_TOO_SMALL;
        CFStringToWSTR(str, buffer, length);
    }
    else
    {
        if (!length) return STATUS_BUFFER_TOO_SMALL;
        buffer[0] = 0;
    }

    return STATUS_SUCCESS;
}

static void iohid_device_set_output_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    IOReturn result;
    struct platform_private *private = impl_from_unix_device(iface);
    result = IOHIDDeviceSetReport(private->device, kIOHIDReportTypeOutput, packet->reportId,
                                  packet->reportBuffer, packet->reportBufferLen);
    if (result == kIOReturnSuccess)
    {
        io->Information = packet->reportBufferLen;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
}

static void iohid_device_get_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    IOReturn ret;
    CFIndex report_length = packet->reportBufferLen;
    struct platform_private *private = impl_from_unix_device(iface);

    ret = IOHIDDeviceGetReport(private->device, kIOHIDReportTypeFeature, packet->reportId,
                               packet->reportBuffer, &report_length);
    if (ret == kIOReturnSuccess)
    {
        io->Information = report_length;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
}

static void iohid_device_set_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    IOReturn result;
    struct platform_private *private = impl_from_unix_device(iface);

    result = IOHIDDeviceSetReport(private->device, kIOHIDReportTypeFeature, packet->reportId,
                                  packet->reportBuffer, packet->reportBufferLen);
    if (result == kIOReturnSuccess)
    {
        io->Information = packet->reportBufferLen;
        io->Status = STATUS_SUCCESS;
    }
    else
    {
        io->Information = 0;
        io->Status = STATUS_UNSUCCESSFUL;
    }
}

static const struct unix_device_vtbl iohid_device_vtbl =
{
    iohid_device_destroy,
    iohid_device_compare,
    iohid_device_start,
    iohid_device_get_report_descriptor,
    iohid_device_get_string,
    iohid_device_set_output_report,
    iohid_device_get_feature_report,
    iohid_device_set_feature_report,
};

static void handle_DeviceMatchingCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef IOHIDDevice)
{
    struct device_desc desc =
    {
        .busid = busidW,
        .input = -1,
        .serial = {'0','0','0','0',0},
    };
    struct platform_private *private;
    CFStringRef str = NULL;

    desc.vid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDVendorIDKey)));
    desc.pid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDProductIDKey)));
    desc.version = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDVersionNumberKey)));
    str = IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDSerialNumberKey));
    if (str) CFStringToWSTR(str, desc.serial, ARRAY_SIZE(desc.serial));
    desc.uid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDLocationIDKey)));

    if (IOHIDDeviceOpen(IOHIDDevice, 0) != kIOReturnSuccess)
    {
        ERR("Failed to open HID device %p (vid %04x, pid %04x)\n", IOHIDDevice, desc.vid, desc.pid);
        return;
    }
    IOHIDDeviceScheduleWithRunLoop(IOHIDDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    if (IOHIDDeviceConformsTo(IOHIDDevice, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) ||
       IOHIDDeviceConformsTo(IOHIDDevice, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick))
    {
        if (is_xbox_gamepad(desc.vid, desc.pid))
            desc.is_gamepad = TRUE;
        else
        {
            int axes=0, buttons=0;
            CFArrayRef element_array = IOHIDDeviceCopyMatchingElements(
                IOHIDDevice, NULL, kIOHIDOptionsTypeNone);

            if (element_array) {
                CFIndex index;
                CFIndex count = CFArrayGetCount(element_array);
                for (index = 0; index < count; index++)
                {
                    IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(element_array, index);
                    if (element)
                    {
                        int type = IOHIDElementGetType(element);
                        if (type == kIOHIDElementTypeInput_Button) buttons++;
                        if (type == kIOHIDElementTypeInput_Axis) axes++;
                        if (type == kIOHIDElementTypeInput_Misc)
                        {
                            uint32_t usage = IOHIDElementGetUsage(element);
                            switch (usage)
                            {
                                case kHIDUsage_GD_X:
                                case kHIDUsage_GD_Y:
                                case kHIDUsage_GD_Z:
                                case kHIDUsage_GD_Rx:
                                case kHIDUsage_GD_Ry:
                                case kHIDUsage_GD_Rz:
                                case kHIDUsage_GD_Slider:
                                    axes ++;
                            }
                        }
                    }
                }
                CFRelease(element_array);
            }
            desc.is_gamepad = (axes == 6  && buttons >= 14);
        }
    }

    TRACE("dev %p, desc %s.\n", IOHIDDevice, debugstr_device_desc(&desc));

    if (!(private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct platform_private))))
        return;
    private->unix_device.vtbl = &iohid_device_vtbl;
    private->device = IOHIDDevice;
    private->buffer = NULL;

    bus_event_queue_device_created(&event_queue, &private->unix_device, &desc);
}

static void handle_RemovalCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef IOHIDDevice)
{
    TRACE("OS/X IOHID Device Removed %p\n", IOHIDDevice);
    IOHIDDeviceRegisterInputReportCallback(IOHIDDevice, NULL, 0, NULL, NULL);
    /* Note: Yes, we leak the buffer. But according to research there is no
             safe way to deallocate that buffer. */
    IOHIDDeviceUnscheduleFromRunLoop(IOHIDDevice, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDDeviceClose(IOHIDDevice, 0);
    bus_event_queue_device_removed(&event_queue, busidW, IOHIDDevice);
}

NTSTATUS iohid_bus_init(void *args)
{
    TRACE("args %p\n", args);

    options = *(struct iohid_bus_options *)args;

    if (!(hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, 0L)))
    {
        ERR("IOHID manager creation failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    run_loop = CFRunLoopGetCurrent();

    IOHIDManagerSetDeviceMatching(hid_manager, NULL);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, handle_DeviceMatchingCallback, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, handle_RemovalCallback, NULL);
    IOHIDManagerScheduleWithRunLoop(hid_manager, run_loop, kCFRunLoopDefaultMode);
    return STATUS_SUCCESS;
}

NTSTATUS iohid_bus_wait(void *args)
{
    struct bus_event *result = args;

    do
    {
        if (bus_event_queue_pop(&event_queue, result)) return STATUS_PENDING;
    } while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, TRUE) != kCFRunLoopRunStopped);

    TRACE("IOHID main loop exiting\n");
    bus_event_queue_destroy(&event_queue);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, NULL, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, NULL, NULL);
    CFRelease(hid_manager);
    return STATUS_SUCCESS;
}

NTSTATUS iohid_bus_stop(void *args)
{
    if (!run_loop) return STATUS_SUCCESS;

    IOHIDManagerUnscheduleFromRunLoop(hid_manager, run_loop, kCFRunLoopDefaultMode);
    CFRunLoopStop(run_loop);
    return STATUS_SUCCESS;
}

#else

NTSTATUS iohid_bus_init(void *args)
{
    WARN("IOHID support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS iohid_bus_wait(void *args)
{
    WARN("IOHID support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS iohid_bus_stop(void *args)
{
    WARN("IOHID support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_IOHIDMANAGERCREATE */
