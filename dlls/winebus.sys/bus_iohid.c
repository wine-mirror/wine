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

#define NONAMELESSUNION

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

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);
#ifdef HAVE_IOHIDMANAGERCREATE

static IOHIDManagerRef hid_manager;
static CFRunLoopRef run_loop;
static HANDLE run_loop_handle;

static const WCHAR busidW[] = {'I','O','H','I','D',0};

struct platform_private
{
    IOHIDDeviceRef device;
    uint8_t *buffer;
};

static inline struct platform_private *impl_from_DEVICE_OBJECT(DEVICE_OBJECT *device)
{
    return (struct platform_private *)get_platform_private(device);
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

static int compare_platform_device(DEVICE_OBJECT *device, void *platform_dev)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    IOHIDDeviceRef dev2 = (IOHIDDeviceRef)platform_dev;
    if (private->device != dev2)
        return 1;
    else
        return 0;
}

static NTSTATUS get_reportdescriptor(DEVICE_OBJECT *device, BYTE *buffer, DWORD length, DWORD *out_length)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
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

static NTSTATUS get_string(DEVICE_OBJECT *device, DWORD index, WCHAR *buffer, DWORD length)
{
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
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

static NTSTATUS begin_report_processing(DEVICE_OBJECT *device)
{
    DWORD length;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    CFNumberRef num;

    if (private->buffer)
        return STATUS_SUCCESS;

    num = IOHIDDeviceGetProperty(private->device, CFSTR(kIOHIDMaxInputReportSizeKey));
    length = CFNumberToDWORD(num);
    private->buffer = HeapAlloc(GetProcessHeap(), 0, length);

    IOHIDDeviceRegisterInputReportCallback(private->device, private->buffer, length, handle_IOHIDDeviceIOHIDReportCallback, device);
    return STATUS_SUCCESS;
}

static NTSTATUS set_output_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    IOReturn result;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);
    result = IOHIDDeviceSetReport(private->device, kIOHIDReportTypeOutput, id, report, length);
    if (result == kIOReturnSuccess)
    {
        *written = length;
        return STATUS_SUCCESS;
    }
    else
    {
        *written = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

static NTSTATUS get_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *read)
{
    IOReturn ret;
    CFIndex report_length = length;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

    ret = IOHIDDeviceGetReport(private->device, kIOHIDReportTypeFeature, id, report, &report_length);
    if (ret == kIOReturnSuccess)
    {
        *read = report_length;
        return STATUS_SUCCESS;
    }
    else
    {
        *read = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

static NTSTATUS set_feature_report(DEVICE_OBJECT *device, UCHAR id, BYTE *report, DWORD length, ULONG_PTR *written)
{
    IOReturn result;
    struct platform_private *private = impl_from_DEVICE_OBJECT(device);

    result = IOHIDDeviceSetReport(private->device, kIOHIDReportTypeFeature, id, report, length);
    if (result == kIOReturnSuccess)
    {
        *written = length;
        return STATUS_SUCCESS;
    }
    else
    {
        *written = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

static const platform_vtbl iohid_vtbl =
{
    compare_platform_device,
    get_reportdescriptor,
    get_string,
    begin_report_processing,
    set_output_report,
    get_feature_report,
    set_feature_report,
};

static void handle_DeviceMatchingCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef IOHIDDevice)
{
    DEVICE_OBJECT *device;
    DWORD vid, pid, version, uid;
    CFStringRef str = NULL;
    WCHAR serial_string[256];
    BOOL is_gamepad = FALSE;
    WORD input = -1;

    TRACE("OS/X IOHID Device Added %p\n", IOHIDDevice);

    vid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDVendorIDKey)));
    pid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDProductIDKey)));
    version = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDVersionNumberKey)));
    str = IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDSerialNumberKey));
    if (str) CFStringToWSTR(str, serial_string, ARRAY_SIZE(serial_string));
    uid = CFNumberToDWORD(IOHIDDeviceGetProperty(IOHIDDevice, CFSTR(kIOHIDLocationIDKey)));

    if (IOHIDDeviceConformsTo(IOHIDDevice, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad) ||
       IOHIDDeviceConformsTo(IOHIDDevice, kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick))
    {
        if (is_xbox_gamepad(vid, pid))
            is_gamepad = TRUE;
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
            is_gamepad = (axes == 6  && buttons >= 14);
        }
    }
    if (is_gamepad)
        input = 0;

    device = bus_create_hid_device(busidW, vid, pid, input,
            version, uid, str ? serial_string : NULL, is_gamepad,
            &iohid_vtbl, sizeof(struct platform_private));
    if (!device)
        ERR("Failed to create device\n");
    else
    {
        struct platform_private *private = impl_from_DEVICE_OBJECT(device);
        private->device = IOHIDDevice;
        private->buffer = NULL;
        IoInvalidateDeviceRelations(bus_pdo, BusRelations);
    }
}

static void handle_RemovalCallback(void *context, IOReturn result, void *sender, IOHIDDeviceRef IOHIDDevice)
{
    DEVICE_OBJECT *device;
    TRACE("OS/X IOHID Device Removed %p\n", IOHIDDevice);
    IOHIDDeviceRegisterInputReportCallback(IOHIDDevice, NULL, 0, NULL, NULL);
    /* Note: Yes, we leak the buffer. But according to research there is no
             safe way to deallocate that buffer. */
    device = bus_find_hid_device(&iohid_vtbl, IOHIDDevice);
    if (device)
    {
        bus_unlink_hid_device(device);
        IoInvalidateDeviceRelations(bus_pdo, BusRelations);
        bus_remove_hid_device(device);
    }
}

/* This puts the relevant run loop for event handling into a WINE thread */
static DWORD CALLBACK runloop_thread(void *args)
{
    run_loop = CFRunLoopGetCurrent();

    IOHIDManagerSetDeviceMatching(hid_manager, NULL);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, handle_DeviceMatchingCallback, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, handle_RemovalCallback, NULL);
    IOHIDManagerScheduleWithRunLoop(hid_manager, run_loop, kCFRunLoopDefaultMode);
    if (IOHIDManagerOpen( hid_manager, 0 ) != kIOReturnSuccess)
    {
        ERR("Couldn't open IOHIDManager.\n");
        IOHIDManagerUnscheduleFromRunLoop(hid_manager, run_loop, kCFRunLoopDefaultMode);
        CFRelease(hid_manager);
        return 0;
    }

    CFRunLoopRun();
    TRACE("Run Loop exiting\n");
    return 1;

}

NTSTATUS iohid_driver_init(void)
{
    hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, 0L);
    if (!(run_loop_handle = CreateThread(NULL, 0, runloop_thread, NULL, 0, NULL)))
    {
        ERR("Failed to initialize IOHID Manager thread\n");
        CFRelease(hid_manager);
        return STATUS_UNSUCCESSFUL;
    }

    TRACE("Initialization successful\n");
    return STATUS_SUCCESS;
}

void iohid_driver_unload( void )
{
    TRACE("Unloading Driver\n");

    if (!run_loop_handle)
        return;

    IOHIDManagerUnscheduleFromRunLoop(hid_manager, run_loop, kCFRunLoopDefaultMode);
    CFRunLoopStop(run_loop);
    WaitForSingleObject(run_loop_handle, INFINITE);
    CloseHandle(run_loop_handle);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, NULL, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, NULL, NULL);
    CFRelease(hid_manager);
    TRACE("Driver Unloaded\n");
}

#else

NTSTATUS iohid_driver_init(void)
{
    return STATUS_NOT_IMPLEMENTED;
}

void iohid_driver_unload( void )
{
    TRACE("Stub: Unload Driver\n");
}

#endif /* HAVE_IOHIDMANAGERCREATE */
