/*  Support for the XBOX360 controller on OS/X
 *
 * Copyright 2019 CodeWeavers, Aric Stewart
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
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>

#if defined(HAVE_IOKIT_USB_IOUSBLIB_H)
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
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
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
#endif /* HAVE_IOKIT_USB_IOUSBLIB_H */

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ddk/wdm.h"
#include "ddk/hidtypes.h"
#include "wine/debug.h"

#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(plugplay);

#ifdef HAVE_IOSERVICEMATCHING

static pthread_mutex_t xbox_cs = PTHREAD_MUTEX_INITIALIZER;

static CFRunLoopRef run_loop;
static struct list event_queue = LIST_INIT(event_queue);
static struct list device_list = LIST_INIT(device_list);
static struct xbox_bus_options options;

struct xbox_device
{
    struct unix_device unix_device;
    io_object_t object;
    IOUSBDeviceInterface500 **dev;

    IOUSBInterfaceInterface550 **interface;
    CFRunLoopSourceRef source;

    char buffer[64];
};

static inline struct xbox_device *impl_from_unix_device(struct unix_device *iface)
{
    return CONTAINING_RECORD(iface, struct xbox_device, unix_device);
}

static struct xbox_device *find_device_from_io_object(io_object_t object)
{
    struct xbox_device *impl;

    LIST_FOR_EACH_ENTRY(impl, &device_list, struct xbox_device, unix_device.entry)
        if (IOObjectIsEqualTo(impl->object, object)) return impl;

    return NULL;
}

static const unsigned char ReportDescriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x3a,                    //   USAGE (Counted Buffer)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x3f,                    //     USAGE (Reserved)
    0x09, 0x3b,                    //     USAGE (Byte Count)
    0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x39,                    //     USAGE (Hatswitch)
    0x15, 0x01,                    //     LOGICAL_MINIMUM (1)
    0x25, 0x08,                    //     LOGICAL_MAXIMUM (0x08)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x08,                    //     PHYSICAL_MAXIMUM (8)
    0x75, 0x04,                    //     REPORT_SIZE (4)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x08,                    //     USAGE (Button 8)
    0x09, 0x07,                    //     USAGE (Button 7)
    0x09, 0x09,                    //     USAGE (Button 9)
    0x09, 0x0a,                    //     USAGE (Button 10)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x05,                    //     USAGE (Button 5)
    0x09, 0x06,                    //     USAGE (Button 6)
    0x09, 0x0b,                    //     USAGE (Button 11)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x45, 0x01,                    //     PHYSICAL_MAXIMUM (1)
    0x95, 0x04,                    //     REPORT_COUNT (4)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x04,                    //     USAGE_MAXIMUM (Button 4)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //     LOGICAL_MAXIMUM (255)
    0x35, 0x00,                    //     PHYSICAL_MINIMUM (0)
    0x46, 0xff, 0x00,              //     PHYSICAL_MAXIMUM (255)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x32,                    //     USAGE (Z)
    0x09, 0x35,                    //     USAGE (Rz)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x16, 0x00, 0x80,              //     LOGICAL_MINIMUM (-32768)
    0x26, 0xff, 0x7f,              //     LOGICAL_MAXIMUM (32767)
    0x36, 0x00, 0x80,              //     PHYSICAL_MINIMUM (-32768)
    0x46, 0xff, 0x7f,              //     PHYSICAL_MAXIMUM (32767)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x05, 0x01,                    //       USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //       USAGE (X)
    0x09, 0x31,                    //       USAGE (Y)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    //     USAGE (Pointer)
    0xa1, 0x00,                    //     COLLECTION (Physical)
    0x95, 0x02,                    //       REPORT_COUNT (2)
    0x05, 0x01,                    //       USAGE_PAGE (Generic Desktop)
    0x09, 0x33,                    //       USAGE (Rx)
    0x09, 0x34,                    //       USAGE (Ry)
    0x81, 0x02,                    //       INPUT (Data,Var,Abs)
    0xc0,                          //     END_COLLECTION
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

typedef struct _HEADER {
    CHAR cmd;
    CHAR size;
} __attribute__((__packed__)) HEADER;

typedef struct _IN_REPORT {
    HEADER header;
    SHORT buttons;
    CHAR trigger_left;
    CHAR trigger_right;
    SHORT hat_left_x;
    SHORT hat_left_y;
    SHORT hat_right_x;
    SHORT hat_right_y;
    CHAR reserved[6];
} __attribute__((__packed__)) IN_REPORT;

typedef struct _OUT_REPORT {
    HEADER header;
    char data[1];
} __attribute__((__packed__)) OUT_REPORT;

static HRESULT get_device_string(IOUSBDeviceInterface500 **dev, UInt8 idx, WCHAR *buffer, DWORD length)
{
    UInt16 buf[64];
    IOUSBDevRequest request;
    kern_return_t err;
    int count;

    request.bmRequestType = USBmakebmRequestType( kUSBIn, kUSBStandard, kUSBDevice );
    request.bRequest = kUSBRqGetDescriptor;
    request.wValue = (kUSBStringDesc << 8) | idx;
    request.wIndex = 0x409;
    request.wLength = sizeof(buf);
    request.pData = buf;

    err = (*dev)->DeviceRequest(dev, &request);
    if ( err != 0 )
        return STATUS_UNSUCCESSFUL;

   count = ( request.wLenDone - 1 ) / 2;

    if ((count+1) > length)
        return STATUS_BUFFER_TOO_SMALL;

    lstrcpynW(buffer, &buf[1], count+1);
    buffer[count+1] = 0;

    return STATUS_SUCCESS;
}

static void ReadCompletion(void *refCon, IOReturn result, void *arg0)
{
    struct unix_device *device = (struct unix_device *)refCon;
    struct xbox_device *private = impl_from_unix_device(device);
    IN_REPORT *report = (IN_REPORT*)private->buffer;
    UInt32 numBytesRead = (UINT_PTR)arg0;
    int hatswitch;

    /* Invert Y axis */
    if (report->hat_left_y < -32767)
        report->hat_left_y = 32767;
    else
        report->hat_left_y = -1*report->hat_left_y;
    if (report->hat_right_y < -32767)
        report->hat_right_y = 32767;
    else
        report->hat_right_y = -1*report->hat_right_y;

    /* Convert D-pad button bitmask to hat switch:
     * 8 1 2
     * 7 0 3
     * 6 5 4
     */
    switch (report->buttons & 0xF)
    {
        case 0x1: hatswitch = 1; break;
        case 0x2: hatswitch = 5; break;
        case 0x4: hatswitch = 7; break;
        case 0x5: hatswitch = 8; break;
        case 0x6: hatswitch = 6; break;
        case 0x8: hatswitch = 3; break;
        case 0x9: hatswitch = 2; break;
        case 0xA: hatswitch = 4; break;
        default:  hatswitch = 0; break;
    }
    report->buttons &= 0xFFF0;
    report->buttons |= hatswitch;

    bus_event_queue_input_report(&event_queue, device, (BYTE *)report, FIELD_OFFSET(IN_REPORT, reserved));

    numBytesRead = sizeof(private->buffer) - 1;

    (*private->interface)->ReadPipeAsync(private->interface, 1,
                            private->buffer, numBytesRead, ReadCompletion,
                            device);
}

/* Handlers */

static void xbox_device_destroy(struct unix_device *iface)
{
    struct xbox_device *impl = impl_from_unix_device(iface);

    TRACE("iface %p.\n", iface);

    free(impl);
}

static NTSTATUS xbox_device_get_report_descriptor(struct unix_device *iface, BYTE *buffer,
                                                  DWORD length, DWORD *out_length)
{
    int data_length = sizeof(ReportDescriptor);

    TRACE("iface %p, buffer %p, length %u, out_length %p.\n", iface, buffer, length, out_length);

    *out_length = data_length;
    if (length < data_length)
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(buffer, ReportDescriptor, data_length);
    return STATUS_SUCCESS;
}

static void xbox_device_set_output_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    FIXME("iface %p, packet %p, io %p stub!\n", iface, packet, io);
    io->Information = 0;
    io->Status = STATUS_UNSUCCESSFUL;
}

static void xbox_device_get_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    FIXME("iface %p, packet %p, io %p stub!\n", iface, packet, io);
    io->Information = 0;
    io->Status = STATUS_UNSUCCESSFUL;
}

static void xbox_device_set_feature_report(struct unix_device *iface, HID_XFER_PACKET *packet, IO_STATUS_BLOCK *io)
{
    FIXME("iface %p, packet %p, io %p stub!\n", iface, packet, io);
    io->Information = 0;
    io->Status = STATUS_UNSUCCESSFUL;
}

static NTSTATUS xbox_device_start(struct unix_device *iface)
{
    struct xbox_device *private = impl_from_unix_device(iface);
    IOCFPlugInInterface **plugInInterface = NULL;
    IOUSBConfigurationDescriptor *desc = NULL;
    IOUSBFindInterfaceRequest intf;
    io_service_t usbInterface;
    UInt32 numBytesRead;
    io_iterator_t iter;
    UInt8 numConfig;
    IOReturn ret;
    SInt32 score;

    TRACE("iface %p.\n", iface);

    if (private->interface != NULL)
        return STATUS_SUCCESS;

    ret = (*private->dev)->GetNumberOfConfigurations(private->dev, &numConfig);
    if (ret != kIOReturnSuccess || numConfig < 1)
        return STATUS_UNSUCCESSFUL;

    ret = (*private->dev)->GetConfigurationDescriptorPtr(private->dev, 0, &desc);
    if (ret != kIOReturnSuccess || desc == NULL)
        return STATUS_UNSUCCESSFUL;

    ret = (*private->dev)->USBDeviceOpen(private->dev);
    if (ret != kIOReturnSuccess)
        return STATUS_UNSUCCESSFUL;

    ret = (*private->dev)->SetConfiguration(private->dev, desc->bConfigurationValue);
    if (ret != kIOReturnSuccess)
    {
        (*private->dev)->USBDeviceClose(private->dev);
        return STATUS_UNSUCCESSFUL;
    }

    /* XBox 360 */
    intf.bInterfaceClass=kIOUSBFindInterfaceDontCare;
    intf.bInterfaceSubClass=93;
    intf.bInterfaceProtocol=1;
    intf.bAlternateSetting=kIOUSBFindInterfaceDontCare;

    ret = (*private->dev)->CreateInterfaceIterator(private->dev, &intf, &iter);
    usbInterface = IOIteratorNext(iter);
    IOObjectRelease(iter);
    (*private->dev)->USBDeviceClose(private->dev);

    if (!usbInterface)
        return STATUS_UNSUCCESSFUL;

    ret = IOCreatePlugInInterfaceForService(usbInterface,
                                kIOUSBInterfaceUserClientTypeID,
                                kIOCFPlugInInterfaceID,
                                &plugInInterface, &score);

    IOObjectRelease(usbInterface);
    if (ret != kIOReturnSuccess || plugInInterface== NULL)
        return STATUS_UNSUCCESSFUL;

    ret = (*plugInInterface)->QueryInterface(plugInInterface,
                        CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID550),
                        (LPVOID) &private->interface);
    (*plugInInterface)->Release(plugInInterface);

    if (ret != kIOReturnSuccess || !private->interface)
        return STATUS_UNSUCCESSFUL;

    ret = (*private->interface)->USBInterfaceOpen(private->interface);
    if (ret != kIOReturnSuccess)
        return STATUS_UNSUCCESSFUL;

    ret = (*private->interface)->CreateInterfaceAsyncEventSource(private->interface, &private->source);
    if (ret != kIOReturnSuccess)
        return STATUS_UNSUCCESSFUL;

    CFRunLoopAddSource(run_loop, private->source, kCFRunLoopCommonModes);

    /* Start event queue */
    numBytesRead = sizeof(private->buffer) - 1;
    ret = (*private->interface)->ReadPipeAsync(private->interface, 1,
                            private->buffer, numBytesRead, ReadCompletion,
                            iface);
    if (ret != kIOReturnSuccess)
    {
        ERR("Failed Start Device Read Loop\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

static void xbox_device_stop(struct unix_device *iface)
{
    struct xbox_device *ext = impl_from_unix_device(iface);

    TRACE("iface %p.\n", iface);

    if (ext->interface) {
        (*ext->interface)->USBInterfaceClose(ext->interface);
        (*ext->interface)->Release(ext->interface);
    }
    if (ext->source) {
        CFRunLoopRemoveSource(run_loop, ext->source, kCFRunLoopCommonModes);
        CFRelease(ext->source);
    }
    IOObjectRelease(ext->object);
    (*ext->dev)->Release(ext->dev);

    pthread_mutex_lock(&xbox_cs);
    list_remove(&ext->unix_device.entry);
    pthread_mutex_unlock(&xbox_cs);
}

static const struct raw_device_vtbl xbox_device_vtbl =
{
    xbox_device_destroy,
    xbox_device_start,
    xbox_device_stop,
    xbox_device_get_report_descriptor,
    xbox_device_set_output_report,
    xbox_device_get_feature_report,
    xbox_device_set_feature_report,
};

static void process_IOService_Device(io_object_t object)
{
    struct device_desc desc =
    {
        .input = -1,
        .manufacturer = {'X','B','O','X',0},
        .is_gamepad = TRUE,
    };
    struct xbox_device *impl;
    IOReturn err;
    IOCFPlugInInterface **plugInInterface=NULL;
    IOUSBDeviceInterface500 **dev=NULL;
    SInt32 score;
    HRESULT res;
    WORD vid, pid, rel;
    UInt32 uid;
    UInt8 idx;

    TRACE("object %#x\n", object);

    err = IOCreatePlugInInterfaceForService(object, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugInInterface, &score);

    if ((kIOReturnSuccess != err) || (plugInInterface == nil) )
    {
        ERR("Unable to create plug in interface for USB device");
        goto failed;
    }

    res = (*plugInInterface)->QueryInterface(plugInInterface, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500), (LPVOID)&dev);
    (*plugInInterface)->Release(plugInInterface);
    if (res || !dev)
    {
        ERR("Unable to create USB device with QueryInterface\n");
        goto failed;
    }

    (*dev)->GetLocationID(dev, &uid);
    desc.uid = uid;
    (*dev)->GetDeviceVendor(dev, &vid);
    desc.vid = vid;
    (*dev)->GetDeviceProduct(dev, &pid);
    desc.pid = pid;
    (*dev)->GetDeviceReleaseNumber(dev, &rel);
    desc.version = rel;

    (*dev)->USBGetProductStringIndex(dev, &idx);
    get_device_string(dev, idx, desc.product, MAX_PATH);
    (*dev)->USBGetManufacturerStringIndex(dev, &idx);
    get_device_string(dev, idx, desc.manufacturer, MAX_PATH);
    (*dev)->USBGetSerialNumberStringIndex(dev, &idx);
    get_device_string(dev, idx, desc.serialnumber, MAX_PATH);

    TRACE("object %#x, dev %p, desc %s.\n", object, dev, debugstr_device_desc(&desc));

    if (!is_xbox_gamepad(vid, pid))
    {
        TRACE("Not an xbox gamepad\n");
        goto failed;
    }

    if (!(impl = raw_device_create(&xbox_device_vtbl, sizeof(struct xbox_device)))) goto failed;
    list_add_tail(&device_list, &impl->unix_device.entry);
    impl->object = object;
    impl->dev = dev;

    bus_event_queue_device_created(&event_queue, &impl->unix_device, &desc);
    return;

failed:
    if (dev)
        (*dev)->Release(dev);
    IOObjectRelease(object);
    return;
}

static void handle_IOServiceMatchingCallback(void *refcon, io_iterator_t iter)
{
    io_object_t object;

    TRACE("Insertion detected\n");
    while ((object = IOIteratorNext(iter)))
        process_IOService_Device(object);
}

static void handle_IOServiceTerminatedCallback(void *refcon, io_iterator_t iter)
{
    struct xbox_device *impl;
    io_object_t object;

    TRACE("Removal detected\n");
    while ((object = IOIteratorNext(iter)))
    {
        impl = find_device_from_io_object(object);
        if (impl) bus_event_queue_device_removed(&event_queue, &impl->unix_device);
        else WARN("failed to find device for io_object %#x\n", object);
        IOObjectRelease(object);
    }
}

NTSTATUS xbox_bus_init(void *args)
{
    IONotificationPortRef notificationObject;
    CFRunLoopSourceRef notificationRunLoopSource;
    CFMutableDictionaryRef dict;
    io_iterator_t myIterator;
    io_object_t object;

    TRACE("args %p\n", args);

    options = *(struct xbox_bus_options *)args;

    run_loop = CFRunLoopGetCurrent();

    dict = IOServiceMatching("IOUSBDevice");

    notificationObject = IONotificationPortCreate(kIOMasterPortDefault);
    notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationObject);
    CFRunLoopAddSource(run_loop, notificationRunLoopSource, kCFRunLoopDefaultMode);

    CFRetain(dict);
    CFRetain(dict);

    IOServiceAddMatchingNotification(notificationObject, kIOMatchedNotification, dict, handle_IOServiceMatchingCallback, NULL, &myIterator);

    while ((object = IOIteratorNext(myIterator)))
        process_IOService_Device(object);

    IOServiceAddMatchingNotification(notificationObject, kIOTerminatedNotification, dict, handle_IOServiceTerminatedCallback, NULL, &myIterator);
    while ((object = IOIteratorNext(myIterator)))
    {
        ERR("Initial removal iteration returned something...\n");
        IOObjectRelease(object);
    }

    return STATUS_SUCCESS;
}

NTSTATUS xbox_bus_wait(void *args)
{
    struct bus_event *result = args;
    CFRunLoopRunResult ret;

    /* cleanup previously returned event */
    bus_event_cleanup(result);

    do
    {
        if (bus_event_queue_pop(&event_queue, result)) return STATUS_PENDING;
        pthread_mutex_lock(&xbox_cs);
        ret = CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10, TRUE);
        pthread_mutex_unlock(&xbox_cs);
    } while (ret != kCFRunLoopRunStopped);

    TRACE("XBOX main loop exiting\n");
    bus_event_queue_destroy(&event_queue);
    return STATUS_SUCCESS;
}

NTSTATUS xbox_bus_stop(void *args)
{
    if (!run_loop) return STATUS_SUCCESS;

    CFRunLoopStop(run_loop);
    return STATUS_SUCCESS;
}

#else

NTSTATUS xbox_bus_init(void *args)
{
    WARN("XBOX support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS xbox_bus_wait(void *args)
{
    WARN("XBOX support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS xbox_bus_stop(void *args)
{
    WARN("XBOX support not compiled in!\n");
    return STATUS_NOT_IMPLEMENTED;
}

#endif /* HAVE_IOSERVICEMATCHING */
