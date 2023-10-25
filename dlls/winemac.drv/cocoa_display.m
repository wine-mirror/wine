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
#ifdef HAVE_MTLDEVICE_REGISTRYID
#import <Metal/Metal.h>
#endif
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
static inline void convert_display_rect(CGRect* out_rect, NSRect in_rect,
                                        NSRect primary_frame)
{
    *out_rect = NSRectToCGRect(in_rect);
    out_rect->origin.y = NSMaxY(primary_frame) - NSMaxY(in_rect);
}


/***********************************************************************
 *              macdrv_get_displays
 *
 * Returns information about the displays.
 *
 * Returns 0 on success and *displays contains a newly-allocated array
 * of macdrv_display structures and *count contains the number of
 * elements in that array.  The first element of the array is the
 * primary display.  When the caller is done with the array, it should
 * use macdrv_free_displays() to deallocate it.
 *
 * Returns non-zero on failure and *displays and *count are unchanged.
 */
int macdrv_get_displays(struct macdrv_display** displays, int* count)
{
@autoreleasepool
{
    NSArray* screens = [NSScreen screens];
    if (screens)
    {
        NSUInteger num_screens = [screens count];
        struct macdrv_display* disps = malloc(num_screens * sizeof(disps[0]));

        if (disps)
        {
            NSRect primary_frame;

            NSUInteger i;
            for (i = 0; i < num_screens; i++)
            {
                NSScreen* screen = screens[i];
                NSRect frame = [screen frame];
                NSRect visible_frame = [screen visibleFrame];

                if (i == 0)
                    primary_frame = frame;

                disps[i].displayID = [[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];
                convert_display_rect(&disps[i].frame, frame, primary_frame);
                convert_display_rect(&disps[i].work_frame, visible_frame,
                                     primary_frame);
                disps[i].frame = cgrect_win_from_mac(disps[i].frame);
                disps[i].work_frame = cgrect_win_from_mac(disps[i].work_frame);
            }

            *displays = disps;
            *count = num_screens;
            return 0;
        }
    }

    return -1;
}
}


/***********************************************************************
 *              macdrv_free_displays
 *
 * Deallocates an array of macdrv_display structures previously returned
 * from macdrv_get_displays().
 */
void macdrv_free_displays(struct macdrv_display* displays)
{
    free(displays);
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

/***********************************************************************
 *              macdrv_get_monitors
 *
 * Get a list of monitors under adapter_id. The first monitor is primary if adapter is primary.
 * Call macdrv_free_monitors() when you are done using the data.
 *
 * Returns non-zero value on failure with parameters unchanged and zero on success.
 */
int macdrv_get_monitors(uint32_t adapter_id, struct macdrv_monitor** new_monitors, int* count)
{
    struct macdrv_monitor* monitors = NULL;
    struct macdrv_monitor* realloc_monitors;
    struct macdrv_display* displays = NULL;
    CGDirectDisplayID display_ids[16];
    uint32_t display_id_count;
    int primary_index = 0;
    int monitor_count = 0;
    int display_count;
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

    if (macdrv_get_displays(&displays, &display_count))
        goto done;

    for (i = 0; i < display_id_count; i++)
    {
        if (display_ids[i] != adapter_id && CGDisplayMirrorsDisplay(display_ids[i]) != adapter_id)
            continue;

        /* Find and fill in monitor info */
        for (j = 0; j < display_count; j++)
        {
            if (displays[j].displayID == display_ids[i]
                || CGDisplayMirrorsDisplay(display_ids[i]) == displays[j].displayID)
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

                monitors[monitor_count].state_flags = DISPLAY_DEVICE_ATTACHED | DISPLAY_DEVICE_ACTIVE;
                monitors[monitor_count].rc_monitor = displays[j].frame;
                monitors[monitor_count].rc_work = displays[j].work_frame;
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
    if (displays)
        macdrv_free_displays(displays);
    if (ret)
        macdrv_free_monitors(monitors);
    return ret;
}

/***********************************************************************
 *              macdrv_free_monitors
 *
 * Frees an monitor list allocated from macdrv_get_monitors()
 */
void macdrv_free_monitors(struct macdrv_monitor* monitors)
{
    if (monitors)
        free(monitors);
}
