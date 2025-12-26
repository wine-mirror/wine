/*
 * MACDRV Cocoa display settings
 *
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers Inc.
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

#import <AppKit/AppKit.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#ifdef HAVE_MTLDEVICE_REGISTRYID
#import <Metal/Metal.h>
#endif
#include <dlfcn.h>
#include "macdrv_cocoa.h"

#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

static uint64_t dedicated_gpu_id;
static uint64_t integrated_gpu_id;

/***********************************************************************
 *              convert_display_rect
 *
 * Converts an NSRect in Cocoa's y-goes-up-from-bottom coordinate system
 * to a CGRect in y-goes-down-from-top coordinates.
 */
static inline CGRect convert_display_rect(NSRect in_rect, NSRect primary_frame)
{
    CGRect out_rect = NSRectToCGRect(in_rect);
    out_rect.origin.y = NSMaxY(primary_frame) - NSMaxY(in_rect);
    return out_rect;
}


/***********************************************************************
 *              get_entry_property_uint32
 *
 * Get an io registry entry property of type uint32 and store it in value parameter.
 *
 * Returns non-zero value on failure.
 */
static int get_entry_property_uint32(io_registry_entry_t entry, CFStringRef property_name, uint32_t* value)
{
    CFDataRef data = IORegistryEntrySearchCFProperty(entry, kIOServicePlane, property_name, kCFAllocatorDefault, 0);
    if (!data)
        return -1;

    if (CFGetTypeID(data) != CFDataGetTypeID() || CFDataGetLength(data) != sizeof(uint32_t))
    {
        CFRelease(data);
        return -1;
    }

    CFDataGetBytes(data, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)value);
    CFRelease(data);
    return 0;
}

/***********************************************************************
 *              get_entry_property_string
 *
 * Get an io registry entry property of type string and write it in buffer parameter.
 *
 * Returns non-zero value on failure.
 */
static int get_entry_property_string(io_registry_entry_t entry, CFStringRef property_name, char* buffer,
                                     size_t buffer_size)
{
    CFTypeRef type_ref;
    CFDataRef data_ref;
    CFStringRef string_ref;
    size_t length;
    int ret = -1;

    type_ref = IORegistryEntrySearchCFProperty(entry, kIOServicePlane, property_name, kCFAllocatorDefault, 0);
    if (!type_ref)
        goto done;

    if (CFGetTypeID(type_ref) == CFDataGetTypeID())
    {
        data_ref = type_ref;
        length = CFDataGetLength(data_ref);
        if (length + 1 > buffer_size)
            goto done;
        CFDataGetBytes(data_ref, CFRangeMake(0, length), (UInt8*)buffer);
        buffer[length] = 0;
    }
    else if (CFGetTypeID(type_ref) == CFStringGetTypeID())
    {
        string_ref = type_ref;
        if (!CFStringGetCString(string_ref, buffer, buffer_size, kCFStringEncodingUTF8))
            goto done;
    }
    else
        goto done;

    ret = 0;
done:
    if (type_ref)
        CFRelease(type_ref);
    return ret;
}

/***********************************************************************
 *              macdrv_get_gpu_info_from_entry
 *
 * Starting from entry (which must be the GPU or a child below the GPU),
 * search upwards to find the IOPCIDevice and get information from it.
 * In case the GPU is not a PCI device, get properties from 'entry'.
 *
 * Returns non-zero value on failure.
 */
static int macdrv_get_gpu_info_from_entry(struct macdrv_gpu* gpu, io_registry_entry_t entry)
{
@autoreleasepool
{
    io_registry_entry_t parent_entry;
    io_registry_entry_t gpu_entry;
    kern_return_t result;
    int ret = -1;

    gpu_entry = entry;
    while (![@"IOPCIDevice" isEqualToString:[(NSString*)IOObjectCopyClass(gpu_entry) autorelease]])
    {
        result = IORegistryEntryGetParentEntry(gpu_entry, kIOServicePlane, &parent_entry);
        if (gpu_entry != entry)
            IOObjectRelease(gpu_entry);
        if (result == kIOReturnNoDevice)
        {
            /* If no IOPCIDevice node is found, get properties from the given entry. */
            gpu_entry = entry;
            break;
        }
        else if (result != kIOReturnSuccess)
        {
            return ret;
        }

        gpu_entry = parent_entry;
    }

    if (IORegistryEntryGetRegistryEntryID(gpu_entry, &gpu->id) != kIOReturnSuccess)
        goto done;

    ret = 0;

    get_entry_property_uint32(gpu_entry, CFSTR("vendor-id"), &gpu->vendor_id);
    get_entry_property_uint32(gpu_entry, CFSTR("device-id"), &gpu->device_id);
    get_entry_property_uint32(gpu_entry, CFSTR("subsystem-id"), &gpu->subsys_id);
    get_entry_property_uint32(gpu_entry, CFSTR("revision-id"), &gpu->revision_id);
    get_entry_property_string(gpu_entry, CFSTR("model"), gpu->name, sizeof(gpu->name));

done:
    if (gpu_entry != entry)
        IOObjectRelease(gpu_entry);
    return ret;
}
}

#ifdef HAVE_MTLDEVICE_REGISTRYID

/***********************************************************************
 *              macdrv_get_gpu_info_from_registry_id
 *
 * Get GPU information from a Metal device registry id.
 *
 * Returns non-zero value on failure.
 */
static int macdrv_get_gpu_info_from_registry_id(struct macdrv_gpu* gpu, uint64_t registry_id)
{
    int ret;
    io_registry_entry_t entry;

    entry = IOServiceGetMatchingService(0, IORegistryEntryIDMatching(registry_id));
    ret = macdrv_get_gpu_info_from_entry(gpu, entry);
    IOObjectRelease(entry);
    return ret;
}

/***********************************************************************
 *              macdrv_get_gpu_info_from_mtldevice
 *
 * Get GPU information from a Metal device that responds to the registryID selector.
 *
 * Returns non-zero value on failure.
 */
static int macdrv_get_gpu_info_from_mtldevice(struct macdrv_gpu* gpu, id<MTLDevice> device)
{
    int ret;
    if ((ret = macdrv_get_gpu_info_from_registry_id(gpu, [device registryID])))
        return ret;
#if defined(MAC_OS_X_VERSION_10_15) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_15
    /* Apple GPUs aren't PCI devices and therefore have no device ID
     * Use the Metal GPUFamily as the device ID */
    if (!gpu->device_id && [device respondsToSelector:@selector(supportsFamily:)] && [device supportsFamily:MTLGPUFamilyApple1])
    {
        MTLGPUFamily highest = MTLGPUFamilyApple1;
        while (1)
        {
            /* Apple2, etc are all in order */
            MTLGPUFamily next = highest + 1;
            if ([device supportsFamily:next])
                highest = next;
            else
                break;
        }
        gpu->device_id = highest;
    }
#endif
    return 0;
}

/***********************************************************************
 *              macdrv_get_gpus_from_metal
 *
 * Get a list of GPUs from Metal.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
static int macdrv_get_gpus_from_metal(struct macdrv_gpu** new_gpus, int* count)
{
@autoreleasepool
{
    struct macdrv_gpu* gpus = NULL;
    struct macdrv_gpu primary_gpu;
    id<MTLDevice> primary_device;
    BOOL hide_integrated = FALSE;
    int primary_index = 0, i;
    int gpu_count = 0;

    /* Test if Metal is available */
    if (&MTLCopyAllDevices == NULL)
        return -1;
    NSArray<id<MTLDevice>>* devices = [MTLCopyAllDevices() autorelease];
    if (!devices.count || ![devices[0] respondsToSelector:@selector(registryID)])
        return -1;

    gpus = calloc(devices.count, sizeof(*gpus));
    if (!gpus)
        return -1;

    /* Use MTLCreateSystemDefaultDevice instead of CGDirectDisplayCopyCurrentMetalDevice(CGMainDisplayID()) to get
     * the primary GPU because we need to hide the integrated GPU for an automatic graphic switching pair to avoid apps
     * using the integrated GPU. This is the behavior of Windows on a Mac. */
    primary_device = [MTLCreateSystemDefaultDevice() autorelease];
    if (macdrv_get_gpu_info_from_mtldevice(&primary_gpu, primary_device))
        goto fail;

    /* Hide the integrated GPU if the system default device is a dedicated GPU */
    if (!primary_device.isLowPower)
    {
        dedicated_gpu_id = primary_gpu.id;
        hide_integrated = TRUE;
    }

    for (i = 0; i < devices.count; i++)
    {
        if (macdrv_get_gpu_info_from_mtldevice(&gpus[gpu_count], devices[i]))
            goto fail;

        if (hide_integrated && devices[i].isLowPower)
        {
            integrated_gpu_id = gpus[gpu_count].id;
            continue;
        }

        if (gpus[gpu_count].id == primary_gpu.id)
            primary_index = gpu_count;

        gpu_count++;
    }

    /* Make sure the first GPU is primary */
    if (primary_index)
    {
        struct macdrv_gpu tmp;
        tmp = gpus[0];
        gpus[0] = gpus[primary_index];
        gpus[primary_index] = tmp;
    }

    *new_gpus = gpus;
    *count = gpu_count;
    return 0;
fail:
    macdrv_free_gpus(gpus);
    return -1;
}
}

/***********************************************************************
 *              macdrv_get_gpu_info_from_display_id_using_metal
 *
 * Get GPU information for a CG display id using Metal.
 *
 * Returns non-zero value on failure.
 */
static int macdrv_get_gpu_info_from_display_id_using_metal(struct macdrv_gpu* gpu, CGDirectDisplayID display_id)
{
@autoreleasepool
{
    id<MTLDevice> device;

    /* Test if Metal is available */
    if (&CGDirectDisplayCopyCurrentMetalDevice == NULL)
        return -1;

    device = [CGDirectDisplayCopyCurrentMetalDevice(display_id) autorelease];
    if (device && [device respondsToSelector:@selector(registryID)])
        return macdrv_get_gpu_info_from_registry_id(gpu, device.registryID);
    else
        return -1;
}
}

#else

static int macdrv_get_gpus_from_metal(struct macdrv_gpu** new_gpus, int* count)
{
    return -1;
}

static int macdrv_get_gpu_info_from_display_id_using_metal(struct macdrv_gpu* gpu, CGDirectDisplayID display_id)
{
    return -1;
}

#endif

/***********************************************************************
 *              macdrv_get_gpu_info_from_display_id
 *
 * Get GPU information from a display id.
 *
 * Returns non-zero value on failure.
 */
static int macdrv_get_gpu_info_from_display_id(struct macdrv_gpu* gpu, CGDirectDisplayID display_id)
{
    int ret;
    io_registry_entry_t entry;

    ret = macdrv_get_gpu_info_from_display_id_using_metal(gpu, display_id);
    if (ret)
    {
        entry = CGDisplayIOServicePort(display_id);
        ret = macdrv_get_gpu_info_from_entry(gpu, entry);
    }
    return ret;
}

/***********************************************************************
 *              macdrv_get_gpus_from_iokit
 *
 * Get a list of GPUs from IOKit.
 * This is a fallback for 32bit build or older Mac OS version where Metal is unavailable.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
static int macdrv_get_gpus_from_iokit(struct macdrv_gpu** new_gpus, int* count)
{
    static const int MAX_GPUS = 4;
    struct macdrv_gpu primary_gpu = {0};
    io_registry_entry_t entry;
    io_iterator_t iterator;
    struct macdrv_gpu* gpus;
    int integrated_index = -1;
    int primary_index = 0;
    int gpu_count = 0;
    char buffer[64];
    int ret = -1;
    int i;

    gpus = calloc(MAX_GPUS, sizeof(*gpus));
    if (!gpus)
        goto done;

    if (IOServiceGetMatchingServices(0, IOServiceMatching("IOPCIDevice"), &iterator)
        != kIOReturnSuccess)
        goto done;

    while ((entry = IOIteratorNext(iterator)))
    {
        if (!get_entry_property_string(entry, CFSTR("IOName"), buffer, sizeof(buffer)) &&
            !strcmp(buffer, "display") &&
            !macdrv_get_gpu_info_from_entry(&gpus[gpu_count], entry))
        {
            gpu_count++;
            assert(gpu_count < MAX_GPUS);
        }
        IOObjectRelease(entry);
    }
    IOObjectRelease(iterator);

    macdrv_get_gpu_info_from_display_id(&primary_gpu, CGMainDisplayID());

    /* If there are more than two GPUs and an Intel card exists,
     * assume an automatic graphics pair exists and hide the integrated GPU */
    if (gpu_count > 1)
    {
        for (i = 0; i < gpu_count; i++)
        {
            /* FIXME:
             * Find integrated GPU without Metal support.
             * Assuming integrated GPU vendor is Intel for now */
            if (gpus[i].vendor_id == 0x8086)
            {
                integrated_gpu_id = gpus[i].id;
                integrated_index = i;
            }

            if (gpus[i].id == primary_gpu.id)
            {
                primary_index = i;
            }
        }

        if (integrated_index != -1)
        {
            if (integrated_index != gpu_count - 1)
                gpus[integrated_index] = gpus[gpu_count - 1];

            /* FIXME:
             * Find the dedicated GPU in an automatic graphics switching pair and use that as primary GPU.
             * Choose the first dedicated GPU as primary */
            if (primary_index == integrated_index)
                primary_index = 0;
            else if (primary_index == gpu_count - 1)
                primary_index = integrated_index;

            dedicated_gpu_id = gpus[primary_index].id;
            gpu_count--;
        }
    }

    /* Make sure the first GPU is primary */
    if (primary_index)
    {
        struct macdrv_gpu tmp;
        tmp = gpus[0];
        gpus[0] = gpus[primary_index];
        gpus[primary_index] = tmp;
    }

    *new_gpus = gpus;
    *count = gpu_count;
    ret = 0;
done:
    if (ret)
        macdrv_free_gpus(gpus);
    return ret;
}

/***********************************************************************
 *              macdrv_get_gpus
 *
 * Get a list of GPUs currently in the system. The first GPU is primary.
 * Call macdrv_free_gpus() when you are done using the data.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
int macdrv_get_gpus(struct macdrv_gpu** new_gpus, int* count)
{
    integrated_gpu_id = 0;
    dedicated_gpu_id = 0;

    if (!macdrv_get_gpus_from_metal(new_gpus, count))
        return 0;
    else
        return macdrv_get_gpus_from_iokit(new_gpus, count);
}

/***********************************************************************
 *              macdrv_free_gpus
 *
 * Frees a GPU list allocated from macdrv_get_gpus()
 */
void macdrv_free_gpus(struct macdrv_gpu* gpus)
{
    if (gpus)
        free(gpus);
}

/***********************************************************************
 *              macdrv_get_adapters
 *
 * Get a list of adapters under gpu_id. The first adapter is primary if GPU is primary.
 * Call macdrv_free_adapters() when you are done using the data.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
int macdrv_get_adapters(uint64_t gpu_id, struct macdrv_adapter** new_adapters, int* count)
{
    CGDirectDisplayID display_ids[16];
    uint32_t display_id_count;
    struct macdrv_adapter* adapters;
    struct macdrv_gpu gpu;
    int primary_index = 0;
    int adapter_count = 0;
    int ret = -1;
    uint32_t i;

    if (CGGetOnlineDisplayList(sizeof(display_ids) / sizeof(display_ids[0]), display_ids, &display_id_count)
        != kCGErrorSuccess)
        return -1;

    if (!display_id_count)
    {
        *new_adapters = NULL;
        *count = 0;
        return 0;
    }

    /* Actual adapter count may be less */
    adapters = calloc(display_id_count, sizeof(*adapters));
    if (!adapters)
        return -1;

    for (i = 0; i < display_id_count; i++)
    {
        /* Mirrored displays are under the same adapter with primary display, so they doesn't increase adapter count */
        if (CGDisplayMirrorsDisplay(display_ids[i]) != kCGNullDirectDisplay)
            continue;

        if (macdrv_get_gpu_info_from_display_id(&gpu, display_ids[i]))
            goto done;

        if (gpu.id == gpu_id || (gpu_id == dedicated_gpu_id && gpu.id == integrated_gpu_id))
        {
            adapters[adapter_count].id = display_ids[i];
            adapters[adapter_count].state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

            if (CGDisplayIsMain(display_ids[i]))
            {
                adapters[adapter_count].state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
                primary_index = adapter_count;
            }

            adapter_count++;
        }
    }

    /* Make sure the first adapter is primary if the GPU is primary */
    if (primary_index)
    {
        struct macdrv_adapter tmp;
        tmp = adapters[0];
        adapters[0] = adapters[primary_index];
        adapters[primary_index] = tmp;
    }

    *new_adapters = adapters;
    *count = adapter_count;
    ret = 0;
done:
    if (ret)
        macdrv_free_adapters(adapters);
    return ret;
}

/***********************************************************************
 *              macdrv_free_adapters
 *
 * Frees an adapter list allocated from macdrv_get_adapters()
 */
void macdrv_free_adapters(struct macdrv_adapter* adapters)
{
    if (adapters)
        free(adapters);
}

static CFDataRef get_edid_from_dcpav_service_proxy(uint32_t vendor_number, uint32_t model_number, uint32_t serial_number)
{
    typedef CFTypeRef IOAVServiceRef;
    static IOAVServiceRef (*pIOAVServiceCreateWithService)(CFAllocatorRef, io_service_t);
    static IOReturn (*pIOAVServiceCopyEDID)(IOAVServiceRef, CFDataRef*);
    static dispatch_once_t once;
    io_iterator_t iterator;
    CFDataRef edid = NULL;
    kern_return_t result;
    io_service_t service;

    dispatch_once(&once, ^{
            void *handle = dlopen("/System/Library/Frameworks/IOKit.framework/IOKit", RTLD_LAZY | RTLD_LOCAL);
            if (handle)
            {
                pIOAVServiceCreateWithService = dlsym(handle, "IOAVServiceCreateWithService");
                pIOAVServiceCopyEDID = dlsym(handle, "IOAVServiceCopyEDID");
            }
        });

    if (!pIOAVServiceCreateWithService || !pIOAVServiceCopyEDID)
        return NULL;

    result = IOServiceGetMatchingServices(0, IOServiceMatching("DCPAVServiceProxy"), &iterator);
    if (result != KERN_SUCCESS)
        return NULL;

    while((service = IOIteratorNext(iterator)))
    {
        uint32_t vendor_number_edid, model_number_edid, serial_number_edid;
        const unsigned char *edid_ptr;
        IOAVServiceRef avservice;
        IOReturn edid_result;

        avservice = pIOAVServiceCreateWithService(kCFAllocatorDefault, service);
        IOObjectRelease(service);
        if (!avservice)
            continue;

        edid_result = pIOAVServiceCopyEDID(avservice, &edid);
        CFRelease(avservice);
        if (edid_result != kIOReturnSuccess || !edid || CFDataGetLength(edid) < 13)
        {
            if (edid)
            {
                CFRelease(edid);
                edid = NULL;
            }
            continue;
        }

        edid_ptr = CFDataGetBytePtr(edid);
        vendor_number_edid = (uint16_t)(edid_ptr[9] | (edid_ptr[8] << 8));
        model_number_edid = *((uint16_t *)&edid_ptr[10]);
        serial_number_edid = *((uint32_t *)&edid_ptr[12]);
        if (vendor_number == vendor_number_edid &&
                model_number == model_number_edid &&
                serial_number == serial_number_edid)
            break;

        CFRelease(edid);
        edid = NULL;
    }

    IOObjectRelease(iterator);
    return edid;
}

static CFDataRef get_edid_from_io_display_edid(uint32_t vendor_number, uint32_t model_number, uint32_t serial_number)
{
    io_iterator_t iterator;
    CFDataRef data = NULL;
    kern_return_t result;
    io_service_t service;

    result = IOServiceGetMatchingServices(0, IOServiceMatching("IODisplayConnect"), &iterator);
    if (result != KERN_SUCCESS)
        return NULL;

    while((service = IOIteratorNext(iterator)))
    {
        uint32_t vendor_number_edid, model_number_edid, serial_number_edid;
        const unsigned char *edid_ptr;
        CFDictionaryRef display_dict;
        CFDataRef edid;

        display_dict = IOCreateDisplayInfoDictionary(service, 0);
        if (display_dict)
        {
            edid = CFDictionaryGetValue(display_dict, CFSTR(kIODisplayEDIDKey));
            if (edid && (CFDataGetLength(edid) >= 13))
            {
                edid_ptr = CFDataGetBytePtr(edid);
                vendor_number_edid = (uint16_t)(edid_ptr[9] | (edid_ptr[8] << 8));
                model_number_edid = *((uint16_t *)&edid_ptr[10]);
                serial_number_edid = *((uint32_t *)&edid_ptr[12]);
                if (vendor_number == vendor_number_edid &&
                        model_number == model_number_edid &&
                        /* CGDisplaySerialNumber() isn't reliable on Intel machines; it returns 0 sometimes. */
                        (!serial_number || serial_number == serial_number_edid))
                    data = CFDataCreateCopy(NULL, edid);
            }
            CFRelease(display_dict);
        }
        IOObjectRelease(service);
        if (data)
            break;
    }

    IOObjectRelease(iterator);
    return data;
}

static uint16_t get_manufacturer_from_vendor(uint32_t vendor)
{
    uint16_t manufacturer = 0;

    manufacturer |= ((vendor >> 10) & 0x1f) << 10;
    manufacturer |= ((vendor >> 5)  & 0x1f) << 5;
    manufacturer |= (vendor & 0x1f);
    manufacturer = (manufacturer >> 8) | (manufacturer << 8);
    return manufacturer;
}

static void fill_detailed_timing_desc(uint8_t *data, int horizontal, int vertical,
        double refresh, size_t mwidth, size_t mheight)
{
    int h_front_porch = 8, h_sync_pulse = 32, h_back_porch = 40;
    int v_front_porch = 6, v_sync_pulse = 8, v_back_porch = 40;
    int h_blanking, v_blanking, h_total, v_total, pixel_clock;

    h_blanking = h_front_porch + h_sync_pulse + h_back_porch;
    v_blanking = v_front_porch + v_sync_pulse + v_back_porch;
    h_total = horizontal + h_blanking;
    v_total = vertical + v_blanking;

    if (refresh <= 0.0) refresh = 60.0;
    pixel_clock = (int)round(h_total * v_total * refresh / 10000.0);

    memset(data, 0, 18);
    data[0] = pixel_clock & 0xff;
    data[1] = (pixel_clock >> 8) & 0xff;
    data[2] = horizontal;
    data[3] = h_total - horizontal;
    data[4] = (((h_total - horizontal) >> 8) & 0xf) | (((horizontal >> 8) & 0xf) << 4);
    data[5] = vertical;
    data[6] = v_total - vertical;
    data[7] = (((v_total - vertical) >> 8) & 0xf) | (((vertical >> 8) & 0xf) << 4);
    data[8] = h_front_porch;
    data[9] = h_sync_pulse;
    data[10] = ((v_front_porch & 0xf) << 4) | (v_sync_pulse & 0xf);
    data[11] = (((h_front_porch >> 8) & 3) << 6)
            | (((h_sync_pulse >> 8) & 3) << 4)
            | (((v_front_porch >> 4) & 3) << 2)
            | ((v_sync_pulse >> 4) & 3);
    data[12] = mwidth;
    data[13] = mheight;
    data[14] = (((mwidth >> 8) & 0xf) << 4) | ((mheight >> 8) & 0xf);
    data[17] = 0x1e;
}

static CFDataRef generate_edid(CGDirectDisplayID display_id, uint32_t vendor_number, uint32_t model_number,
        uint32_t serial_number)
{
    struct display_param {
        size_t width;
        size_t height;
        double refresh;
    } best_params[3];
    double mwidth, mheight, refresh;
    CGDisplayModeRef mode = NULL;
    size_t width = 0, height = 0;
    uint8_t edid[128], sum, *p;
    CFDataRef data = NULL;
    int i, j, timings = 0;
    CGSize screen_size;
    CFArrayRef modes;

    screen_size = CGDisplayScreenSize(display_id);
    mwidth = screen_size.width;
    mheight = screen_size.height;

    memset(&edid, 0, sizeof(edid));
    *(uint64_t *)&edid[0] = 0x00ffffffffffff00;

    *(uint16_t *)&edid[8] = get_manufacturer_from_vendor(vendor_number);
    *(uint16_t *)&edid[10] = (uint16_t)model_number;
    *(uint32_t *)&edid[12] = serial_number;
    edid[16] = 52; /* weeks */
    edid[17] = 30; /* year */
    edid[18] = 1; /* version */
    edid[19] = 4; /* revision */
    edid[20] = 0xb5; /* video input parameters: 10 bits DisplayPort */
    edid[21] = (uint8_t)(mwidth / 10.0); /* horizontal screen size in cm */
    edid[22] = (uint8_t)(mheight / 10.0); /* vertical screen size in cm */
    edid[23] = 0x78; /* gamma: 2.2 */
    edid[24] = 0x06; /* supported features: RGB 4:4:4, sRGB, native pixel format and refresh rate */

    /* color characteristics: sRGB */
    edid[25] = 0xee;
    edid[26] = 0x91;
    edid[27] = 0xa3;
    edid[28] = 0x54;
    edid[29] = 0x4c;
    edid[30] = 0x99;
    edid[31] = 0x26;
    edid[32] = 0x0f;
    edid[33] = 0x50;
    edid[34] = 0x54;

    for (i = 0; i < 16; ++i) edid[38 + i] = 1;

    /* detailed timing descriptors */
    p = edid + 54;
    modes = CGDisplayCopyAllDisplayModes(display_id, NULL);
    if (modes)
        i = CFArrayGetCount(modes);
    else
        i = 0;
    for (i--; i >= 0; i--)
    {
        CGDisplayModeRef candidate_mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        int flags = CGDisplayModeGetIOFlags(candidate_mode);
        if (flags & kDisplayModeNativeFlag || flags & kDisplayModeDefaultFlag)
        {
            width = CGDisplayModeGetPixelWidth(candidate_mode);
            height = CGDisplayModeGetPixelHeight(candidate_mode);
            refresh = CGDisplayModeGetRefreshRate(candidate_mode);
            if (!timings)
            {
                best_params[0].width = width;
                best_params[0].height = height;
                best_params[0].refresh = refresh;
                timings++;
            }
            else
            {
                for (j = 0; j < timings; j++)
                {
                    if (best_params[j].width < width ||
                            (best_params[j].width == width && best_params[j].refresh < refresh))
                    {
                        struct display_param swap_display_param = best_params[j];

                        best_params[j].width = width;
                        best_params[j].height = height;
                        best_params[j].refresh = refresh;
                        width = swap_display_param.width;
                        height = swap_display_param.height;
                        refresh = swap_display_param.refresh;
                    }
                }
                if (timings != 3)
                {
                    best_params[timings].width = width;
                    best_params[timings].height = height;
                    best_params[timings].refresh = refresh;
                    timings++;
                }
            }
        }
    }
    if (modes)
        CFRelease(modes);
    if (!timings)
    {
        mode = CGDisplayCopyDisplayMode(display_id);
        width = CGDisplayModeGetPixelWidth(mode);
        height = CGDisplayModeGetPixelHeight(mode);
        refresh = CGDisplayModeGetRefreshRate(mode);
        fill_detailed_timing_desc(p, width, height, refresh, (size_t)mwidth, (size_t)mheight);
        timings++;
        CGDisplayModeRelease(mode);
    }
    else
    {
        for (i = 0; i < timings; i++)
        {
            fill_detailed_timing_desc(p, best_params[i].width, best_params[i].height,
                    best_params[i].refresh, (size_t)mwidth, (size_t)mheight);
            p += 18;
            p[3] = 0x10;
        }
    }
    while (timings != 3)
    {
        p += 18;
        p[3] = 0x10;
        timings++;
    }

    /* display product name */
    p[3] = 0xfc;
    strcpy((char *)p + 5, "Wine Monitor");
    p[17] = 0x0a;

    /* checksum */
    sum = 0;
    for (i = 0; i < 127; ++i)
        sum += edid[i];
    edid[127] = 256 - sum;

    data = CFDataCreate(NULL, edid, 128);
    return data;
}

/***********************************************************************
 *              macdrv_get_monitors
 *
 * Get a list of monitors under adapter_id. The first monitor is primary if adapter is primary.
 * Call macdrv_free_monitors() when you are done using the data.
 * An adapter_id of kCGNullDirectDisplay will return monitors for all adapters.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
int macdrv_get_monitors(CGDirectDisplayID adapter_id, struct macdrv_monitor** new_monitors, int* count)
{
@autoreleasepool
{
    struct macdrv_monitor* monitors = NULL;
    struct macdrv_monitor* realloc_monitors;
    CGDirectDisplayID display_ids[16];
    uint32_t display_id_count, vendor_number, model_number, serial_number;
    NSArray<NSScreen *> *screens = [NSScreen screens];
    NSRect primary_frame;
    int primary_index = 0;
    int monitor_count = 0;
    int capacity;
    int ret = -1;
    int i, j;

    /* 2 should be enough for most cases */
    capacity = 2;
    monitors = calloc(capacity, sizeof(*monitors));
    if (!monitors)
        return -1;

    if (CGGetOnlineDisplayList(sizeof(display_ids) / sizeof(display_ids[0]), display_ids, &display_id_count)
        != kCGErrorSuccess)
        goto done;

    screens = [NSScreen screens];
    if (!screens || screens.count < 1)
        goto done;

    primary_frame = screens[0].frame;

    for (i = 0; i < display_id_count; i++)
    {
        if (adapter_id     != kCGNullDirectDisplay &&
            display_ids[i] != adapter_id           &&
            CGDisplayMirrorsDisplay(display_ids[i]) != adapter_id)
            continue;

        /* Find and fill in monitor info */
        for (j = 0; j < screens.count; j++)
        {
            NSScreen* screen = screens[j];
            CGDirectDisplayID screen_displayID = [[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
            CFDataRef edid_data;
            size_t length;

            if (screen_displayID == display_ids[i]
                || CGDisplayMirrorsDisplay(display_ids[i]) == screen_displayID)
            {
                /* Allocate more space if needed */
                if (monitor_count >= capacity)
                {
                    capacity *= 2;
                    realloc_monitors = realloc(monitors, sizeof(*monitors) * capacity);
                    if (!realloc_monitors)
                        goto done;
                    monitors = realloc_monitors;
                }

                if (j == 0)
                    primary_index = monitor_count;

                monitors[monitor_count].id = display_ids[i];
                monitors[monitor_count].rc_monitor = cgrect_win_from_mac(convert_display_rect(screen.frame, primary_frame));
                monitors[monitor_count].rc_work = cgrect_win_from_mac(convert_display_rect(screen.visibleFrame, primary_frame));

                vendor_number = CGDisplayVendorNumber(monitors[monitor_count].id);
                model_number = CGDisplayModelNumber(monitors[monitor_count].id);
                serial_number = CGDisplaySerialNumber(monitors[monitor_count].id);

                edid_data = get_edid_from_dcpav_service_proxy(vendor_number, model_number, serial_number);
                if (!edid_data)
                    edid_data = get_edid_from_io_display_edid(vendor_number, model_number, serial_number);
                if (!edid_data)
                    edid_data = generate_edid(monitors[monitor_count].id, vendor_number, model_number, serial_number);
                if (edid_data && (length = CFDataGetLength(edid_data)))
                {
                    const unsigned char *edid_ptr = CFDataGetBytePtr(edid_data);

                    if ((monitors[monitor_count].edid = malloc(length)))
                    {
                        monitors[monitor_count].edid_len = length;
                        memcpy(monitors[monitor_count].edid, edid_ptr, length);
                    }
                    CFRelease(edid_data);
                }

                monitors[monitor_count].hdr_enabled = false;
#if defined(MAC_OS_X_VERSION_10_15) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_15
                if (@available(macOS 10.15, *))
                    monitors[monitor_count].hdr_enabled = (screen.maximumPotentialExtendedDynamicRangeColorComponentValue > 1.0) ? true : false;
#endif
                monitor_count++;
                break;
            }
        }
    }

    /* Make sure the first monitor on primary adapter is primary */
    if (primary_index)
    {
        struct macdrv_monitor tmp;
        tmp = monitors[0];
        monitors[0] = monitors[primary_index];
        monitors[primary_index] = tmp;
    }

    *new_monitors = monitors;
    *count = monitor_count;
    ret = 0;
done:
    if (ret)
        macdrv_free_monitors(monitors, capacity);
    return ret;
}
}

/***********************************************************************
 *              macdrv_free_monitors
 *
 * Frees an monitor list allocated from macdrv_get_monitors()
 */
void macdrv_free_monitors(struct macdrv_monitor* monitors, int monitor_count)
{
    while (monitor_count--)
        if (monitors[monitor_count].edid)
            free(monitors[monitor_count].edid);
    if (monitors)
        free(monitors);
}
