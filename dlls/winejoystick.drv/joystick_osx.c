/*
 * WinMM joystick driver OS X implementation
 *
 * Copyright 1997 Andreas Mohr
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000 Wolfgang Schwotzer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002 David Hagood
 * Copyright 2009 CodeWeavers, Aric Stewart
 * Copyright 2015 Ken Thomases for CodeWeavers Inc.
 * Copyright 2016 David Lawrie
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

#include "joystick.h"

#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(joystick);


#define MAXJOYSTICK (JOYSTICKID2 + 30)


enum {
    AXIS_X,  /* Winmm X */
    AXIS_Y,  /* Winmm Y */
    AXIS_Z,  /* Winmm Z */
    AXIS_RX, /* Winmm V */
    AXIS_RY, /* Winmm U */
    AXIS_RZ, /* Winmm R */
    NUM_AXES
};

struct axis {
    IOHIDElementRef element;
    CFIndex min_value, max_value;
};

typedef struct {
    BOOL                in_use;
    IOHIDElementRef     element;
    struct axis         axes[NUM_AXES];
    CFMutableArrayRef   buttons;
    IOHIDElementRef     hatswitch;
} joystick_t;


static joystick_t joysticks[MAXJOYSTICK];
static CFMutableArrayRef device_main_elements = NULL;

static long get_device_property_long(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef ref;
    long result = 0;

    if (device)
    {
        assert(IOHIDDeviceGetTypeID() == CFGetTypeID(device));

        ref = IOHIDDeviceGetProperty(device, key);

        if (ref && CFNumberGetTypeID() == CFGetTypeID(ref))
            CFNumberGetValue((CFNumberRef)ref, kCFNumberLongType, &result);
    }

    return result;
}

static CFStringRef copy_device_name(IOHIDDeviceRef device)
{
    CFStringRef name;

    if (device)
    {
        CFTypeRef ref_name;

        assert(IOHIDDeviceGetTypeID() == CFGetTypeID(device));

        ref_name = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));

        if (ref_name && CFStringGetTypeID() == CFGetTypeID(ref_name))
            name = CFStringCreateCopy(kCFAllocatorDefault, ref_name);
        else
        {
            long vendID, prodID;

            vendID = get_device_property_long(device, CFSTR(kIOHIDVendorIDKey));
            prodID = get_device_property_long(device, CFSTR(kIOHIDProductIDKey));
            name = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("0x%04lx 0x%04lx"), vendID, prodID);
        }
    }
    else
    {
        ERR("NULL device\n");
        name = CFStringCreateCopy(kCFAllocatorDefault, CFSTR(""));
    }

    return name;
}

static long get_device_location_ID(IOHIDDeviceRef device)
{
    return get_device_property_long(device, CFSTR(kIOHIDLocationIDKey));
}

static const char* debugstr_cf(CFTypeRef t)
{
    CFStringRef s;
    const char* ret;

    if (!t) return "(null)";

    if (CFGetTypeID(t) == CFStringGetTypeID())
        s = t;
    else
        s = CFCopyDescription(t);
    ret = CFStringGetCStringPtr(s, kCFStringEncodingUTF8);
    if (ret) ret = debugstr_a(ret);
    if (!ret)
    {
        const UniChar* u = CFStringGetCharactersPtr(s);
        if (u)
            ret = debugstr_wn((const WCHAR*)u, CFStringGetLength(s));
    }
    if (!ret)
    {
        UniChar buf[200];
        int len = min(CFStringGetLength(s), ARRAY_SIZE(buf));
        CFStringGetCharacters(s, CFRangeMake(0, len), buf);
        ret = debugstr_wn(buf, len);
    }
    if (s != t) CFRelease(s);
    return ret;
}

static const char* debugstr_device(IOHIDDeviceRef device)
{
    return wine_dbg_sprintf("<IOHIDDevice %p product %s IOHIDLocationID %lu>", device,
                            debugstr_cf(IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey))),
                            get_device_location_ID(device));
}

static const char* debugstr_element(IOHIDElementRef element)
{
    return wine_dbg_sprintf("<IOHIDElement %p type %d usage %u/%u device %p>", element,
                            IOHIDElementGetType(element), IOHIDElementGetUsagePage(element),
                            IOHIDElementGetUsage(element), IOHIDElementGetDevice(element));
}

static int axis_for_usage_GD(int usage)
{
    switch (usage)
    {
        case kHIDUsage_GD_X: return AXIS_X;
        case kHIDUsage_GD_Y: return AXIS_Y;
        case kHIDUsage_GD_Z: return AXIS_Z;
        case kHIDUsage_GD_Rx: return AXIS_RX;
        case kHIDUsage_GD_Ry: return AXIS_RY;
        case kHIDUsage_GD_Rz: return AXIS_RZ;
    }

    return -1;
}

static int axis_for_usage_Sim(int usage)
{
    switch (usage)
    {
        case kHIDUsage_Sim_Rudder: return AXIS_RZ;
        case kHIDUsage_Sim_Throttle: return AXIS_Z;
        case kHIDUsage_Sim_Steering: return AXIS_X;
        case kHIDUsage_Sim_Accelerator: return AXIS_Y;
        case kHIDUsage_Sim_Brake: return AXIS_RZ;
    }

    return -1;
}

/**************************************************************************
 *                              joystick_from_id
 */
static joystick_t* joystick_from_id(DWORD_PTR device_id)
{
    int index;

    if ((device_id - (DWORD_PTR)joysticks) % sizeof(joysticks[0]) != 0)
        return NULL;
    index = (device_id - (DWORD_PTR)joysticks) / sizeof(joysticks[0]);
    if (index < 0 || index >= MAXJOYSTICK || !((joystick_t*)device_id)->in_use)
        return NULL;

    return (joystick_t*)device_id;
}

/**************************************************************************
 *                              create_osx_device_match
 */
static CFDictionaryRef create_osx_device_match(int usage)
{
    CFDictionaryRef result = NULL;
    int number;
    CFStringRef keys[] = { CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey) };
    CFNumberRef values[2];
    int i;

    TRACE("usage %d\n", usage);

    number = kHIDPage_GenericDesktop;
    values[0] = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &number);
    values[1] = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);

    if (values[0] && values[1])
    {
        result = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, ARRAY_SIZE(values),
                                    &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        if (!result)
            ERR("CFDictionaryCreate failed.\n");
    }
    else
        ERR("CFNumberCreate failed.\n");

    for (i = 0; i < ARRAY_SIZE(values); i++)
        if (values[i]) CFRelease(values[i]);

    return result;
}

/**************************************************************************
 *                              find_top_level
 */
static CFIndex find_top_level(IOHIDDeviceRef hid_device, CFMutableArrayRef main_elements)
{
    CFArrayRef      elements;
    CFIndex         total = 0;

    TRACE("hid_device %s\n", debugstr_device(hid_device));

    if (!hid_device)
        return 0;

    elements = IOHIDDeviceCopyMatchingElements(hid_device, NULL, 0);

    if (elements)
    {
        CFIndex i, count = CFArrayGetCount(elements);
        for (i = 0; i < count; i++)
        {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(elements, i);
            int type = IOHIDElementGetType(element);

            TRACE("element %s\n", debugstr_element(element));

            /* Check for top-level gaming device collections */
            if (type == kIOHIDElementTypeCollection && IOHIDElementGetParent(element) == 0)
            {
                int usage_page = IOHIDElementGetUsagePage(element);
                int usage = IOHIDElementGetUsage(element);

                if (usage_page == kHIDPage_GenericDesktop &&
                    (usage == kHIDUsage_GD_Joystick || usage == kHIDUsage_GD_GamePad || usage == kHIDUsage_GD_MultiAxisController))
                {
                    CFArrayAppendValue(main_elements, element);
                    total++;
                }
            }
        }
        CFRelease(elements);
    }

    TRACE("-> total %d\n", (int)total);
    return total;
}
/**************************************************************************
 *                              device_name_comparator
 *
 * Virtual joysticks may not have a kIOHIDLocationIDKey and will default to location ID of 0, this orders virtual joysticks by their name
 */
static CFComparisonResult device_name_comparator(IOHIDDeviceRef device1, IOHIDDeviceRef device2)
{
    CFStringRef name1 = copy_device_name(device1), name2 = copy_device_name(device2);
    CFComparisonResult result = CFStringCompare(name1, name2, (kCFCompareForcedOrdering | kCFCompareNumerically));
    CFRelease(name1);
    CFRelease(name2);
    return  result;
}

/**************************************************************************
 *                              device_location_name_comparator
 *
 * Helper to sort device array first by location ID, since location IDs are consistent across boots & launches, then by product name
 */
static CFComparisonResult device_location_name_comparator(const void *val1, const void *val2, void *context)
{
    IOHIDDeviceRef device1 = (IOHIDDeviceRef)val1, device2 = (IOHIDDeviceRef)val2;
    long loc1 = get_device_location_ID(device1), loc2 = get_device_location_ID(device2);

    if (loc1 < loc2)
        return kCFCompareLessThan;
    else if (loc1 > loc2)
        return kCFCompareGreaterThan;
    return device_name_comparator(device1, device2);
}

/**************************************************************************
 *                              copy_set_to_array
 *
 * Helper to copy the CFSet to a CFArray
 */
static void copy_set_to_array(const void *value, void *context)
{
    CFArrayAppendValue(context, value);
}

/**************************************************************************
 *                              find_osx_devices
 */
static int find_osx_devices(void)
{
    IOHIDManagerRef hid_manager;
    int usages[] = { kHIDUsage_GD_Joystick, kHIDUsage_GD_GamePad, kHIDUsage_GD_MultiAxisController };
    int i;
    CFDictionaryRef matching_dicts[ARRAY_SIZE(usages)];
    CFArrayRef matching;
    CFSetRef devset;

    TRACE("()\n");

    hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, 0L);
    if (IOHIDManagerOpen(hid_manager, 0) != kIOReturnSuccess)
    {
        ERR("Couldn't open IOHIDManager.\n");
        CFRelease(hid_manager);
        return 0;
    }

    for (i = 0; i < ARRAY_SIZE(matching_dicts); i++)
    {
        matching_dicts[i] = create_osx_device_match(usages[i]);
        if (!matching_dicts[i])
        {
            while (i > 0)
                CFRelease(matching_dicts[--i]);
            goto fail;
        }
    }

    matching = CFArrayCreate(NULL, (const void**)matching_dicts, ARRAY_SIZE(matching_dicts),
                             &kCFTypeArrayCallBacks);

    for (i = 0; i < ARRAY_SIZE(matching_dicts); i++)
        CFRelease(matching_dicts[i]);

    IOHIDManagerSetDeviceMatchingMultiple(hid_manager, matching);
    CFRelease(matching);
    devset = IOHIDManagerCopyDevices(hid_manager);
    if (devset)
    {
        CFIndex num_devices, num_main_elements;
        CFMutableArrayRef devices;

        num_devices = CFSetGetCount(devset);
        devices = CFArrayCreateMutable(kCFAllocatorDefault, num_devices, &kCFTypeArrayCallBacks);
        CFSetApplyFunction(devset, copy_set_to_array, (void *)devices);
        CFArraySortValues(devices, CFRangeMake(0, num_devices), device_location_name_comparator, NULL);

        CFRelease(devset);
        if (!devices)
            goto fail;

        device_main_elements = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
        if (!device_main_elements)
        {
            CFRelease(devices);
            goto fail;
        }

        num_main_elements = 0;
        for (i = 0; i < num_devices; i++)
        {
            IOHIDDeviceRef hid_device = (IOHIDDeviceRef)CFArrayGetValueAtIndex(devices, i);
            TRACE("hid_device %s\n", debugstr_device(hid_device));
            num_main_elements += find_top_level(hid_device, device_main_elements);
        }

        CFRelease(devices);

        TRACE("found %i device(s), %i collection(s)\n",(int)num_devices,(int)num_main_elements);
        return (int)num_main_elements;
    }

fail:
    IOHIDManagerClose(hid_manager, 0);
    CFRelease(hid_manager);
    return 0;
}

/**************************************************************************
 *                              collect_joystick_elements
 */
static void collect_joystick_elements(joystick_t* joystick, IOHIDElementRef collection)
{
    CFIndex    i, count;
    CFArrayRef children = IOHIDElementGetChildren(collection);

    TRACE("collection %s\n", debugstr_element(collection));

    count = CFArrayGetCount(children);
    for (i = 0; i < count; i++)
    {
        IOHIDElementRef child;
        int type;
        uint32_t usage_page;

        child = (IOHIDElementRef)CFArrayGetValueAtIndex(children, i);
        TRACE("child %s\n", debugstr_element(child));
        type = IOHIDElementGetType(child);
        usage_page = IOHIDElementGetUsagePage(child);

        switch (type)
        {
            case kIOHIDElementTypeCollection:
                collect_joystick_elements(joystick, child);
                break;
            case kIOHIDElementTypeInput_Button:
            {
                TRACE("kIOHIDElementTypeInput_Button usage_page %d\n", usage_page);

                /* avoid strange elements found on the 360 controller */
                if (usage_page == kHIDPage_Button)
                    CFArrayAppendValue(joystick->buttons, child);
                break;
            }
            case kIOHIDElementTypeInput_Axis:
            case kIOHIDElementTypeInput_Misc:
            {
                uint32_t usage = IOHIDElementGetUsage( child );
                switch (usage_page)
                {
                    case kHIDPage_GenericDesktop:
                    {
                        switch(usage)
                        {
                            case kHIDUsage_GD_Hatswitch:
                            {
                                TRACE("kIOHIDElementTypeInput_Axis/Misc / kHIDUsage_GD_Hatswitch\n");
                                if (joystick->hatswitch)
                                    TRACE("    ignoring additional hatswitch\n");
                                else
                                    joystick->hatswitch = (IOHIDElementRef)CFRetain(child);
                                break;
                            }
                            case kHIDUsage_GD_X:
                            case kHIDUsage_GD_Y:
                            case kHIDUsage_GD_Z:
                            case kHIDUsage_GD_Rx:
                            case kHIDUsage_GD_Ry:
                            case kHIDUsage_GD_Rz:
                            {
                                int axis = axis_for_usage_GD(usage);
                                TRACE("kIOHIDElementTypeInput_Axis/Misc / kHIDUsage_GD_<axis> (%d) axis %d\n", usage, axis);
                                if (axis < 0 || joystick->axes[axis].element)
                                    TRACE("    ignoring\n");
                                else
                                {
                                    joystick->axes[axis].element = (IOHIDElementRef)CFRetain(child);
                                    joystick->axes[axis].min_value = IOHIDElementGetLogicalMin(child);
                                    joystick->axes[axis].max_value = IOHIDElementGetLogicalMax(child);
                                }
                                break;
                            }
                            case kHIDUsage_GD_Slider:
                            case kHIDUsage_GD_Dial:
                            case kHIDUsage_GD_Wheel:
                            {
                                /* if one axis is taken, fall to the next until axes are filled */
                                int possible_axes[3] = {AXIS_Z,AXIS_RY,AXIS_RX};
                                int axis = 0;
                                while(axis < 3 && joystick->axes[possible_axes[axis]].element)
                                    axis++;
                                if (axis == 3)
                                    TRACE("kIOHIDElementTypeInput_Axis/Misc / kHIDUsage_GD_<axis> (%d)\n    ignoring\n", usage);
                                else
                                {
                                    TRACE("kIOHIDElementTypeInput_Axis/Misc / kHIDUsage_GD_<axis> (%d) axis %d\n", usage, possible_axes[axis]);
                                    joystick->axes[possible_axes[axis]].element = (IOHIDElementRef)CFRetain(child);
                                    joystick->axes[possible_axes[axis]].min_value = IOHIDElementGetLogicalMin(child);
                                    joystick->axes[possible_axes[axis]].max_value = IOHIDElementGetLogicalMax(child);
                                }
                                break;
                            }
                            default:
                                FIXME("kIOHIDElementTypeInput_Axis/Misc / Unhandled GD Page usage %d\n", usage);
                                break;
                        }
                        break;
                    }
                    case kHIDPage_Simulation:
                    {
                        switch(usage)
                        {
                            case kHIDUsage_Sim_Rudder:
                            case kHIDUsage_Sim_Throttle:
                            case kHIDUsage_Sim_Steering:
                            case kHIDUsage_Sim_Accelerator:
                            case kHIDUsage_Sim_Brake:
                            {
                                int axis = axis_for_usage_Sim(usage);
                                TRACE("kIOHIDElementTypeInput_Axis/Misc / kHIDUsage_Sim_<axis> (%d) axis %d\n", usage, axis);
                                if (axis < 0 || joystick->axes[axis].element)
                                    TRACE("    ignoring\n");
                                else
                                {
                                    joystick->axes[axis].element = (IOHIDElementRef)CFRetain(child);
                                    joystick->axes[axis].min_value = IOHIDElementGetLogicalMin(child);
                                    joystick->axes[axis].max_value = IOHIDElementGetLogicalMax(child);
                                }
                                break;
                            }
                            default:
                                FIXME("kIOHIDElementTypeInput_Axis/Misc / Unhandled Sim Page usage %d\n", usage);
                                break;
                        }
                        break;
                    }
                    default:
                        FIXME("kIOHIDElementTypeInput_Axis/Misc / Unhandled Usage Page %d\n", usage_page);
                        break;
                }
                break;
            }
            case kIOHIDElementTypeFeature:
                /* Describes input and output elements not intended for consumption by the end user. Ignoring. */
                break;
            default:
                FIXME("Unhandled type %i\n",type);
                break;
        }
    }
}

/**************************************************************************
 *                              button_usage_comparator
 */
static CFComparisonResult button_usage_comparator(const void *val1, const void *val2, void *context)
{
    IOHIDElementRef element1 = (IOHIDElementRef)val1, element2 = (IOHIDElementRef)val2;
    int usage1 = IOHIDElementGetUsage(element1), usage2 = IOHIDElementGetUsage(element2);

    if (usage1 < usage2)
        return kCFCompareLessThan;
    if (usage1 > usage2)
        return kCFCompareGreaterThan;
    return kCFCompareEqualTo;
}

/**************************************************************************
 *                              driver_open
 */
LRESULT driver_open(LPSTR str, DWORD index)
{
    if (index >= MAXJOYSTICK || joysticks[index].in_use)
        return 0;

    joysticks[index].in_use = TRUE;
    return (LRESULT)&joysticks[index];
}

/**************************************************************************
 *                              driver_close
 */
LRESULT driver_close(DWORD_PTR device_id)
{
    joystick_t* joystick = joystick_from_id(device_id);
    int i;

    if (joystick == NULL)
        return 0;

    CFRelease(joystick->element);
    for (i = 0; i < NUM_AXES; i++)
    {
        if (joystick->axes[i].element)
            CFRelease(joystick->axes[i].element);
    }
    if (joystick->buttons)
        CFRelease(joystick->buttons);
    if (joystick->hatswitch)
        CFRelease(joystick->hatswitch);

    memset(joystick, 0, sizeof(*joystick));
    return 1;
}

/**************************************************************************
 *                              open_joystick
 */
static BOOL open_joystick(joystick_t* joystick)
{
    CFIndex index;
    CFRange range;

    if (joystick->element)
        return TRUE;

    if (!device_main_elements)
    {
        find_osx_devices();
        if (!device_main_elements)
            return FALSE;
    }

    index = joystick - joysticks;
    if (index >= CFArrayGetCount(device_main_elements))
        return FALSE;

    joystick->element = (IOHIDElementRef)CFArrayGetValueAtIndex(device_main_elements, index);
    joystick->buttons = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
    collect_joystick_elements(joystick, joystick->element);

    /* Sort buttons into correct order */
    range.location = 0;
    range.length = CFArrayGetCount(joystick->buttons);
    CFArraySortValues(joystick->buttons, range, button_usage_comparator, NULL);
    if (range.length > 32)
    {
        /* Delete any buttons beyond the first 32 */
        range.location = 32;
        range.length -= 32;
        CFArrayReplaceValues(joystick->buttons, range, NULL, 0);
    }

    return TRUE;
}


/**************************************************************************
 *                              driver_joyGetDevCaps
 */
LRESULT driver_joyGetDevCaps(DWORD_PTR device_id, JOYCAPSW* caps, DWORD size)
{
    joystick_t* joystick;
    IOHIDDeviceRef device;

    if ((joystick = joystick_from_id(device_id)) == NULL)
        return MMSYSERR_NODRIVER;

    if (!open_joystick(joystick))
        return JOYERR_PARMS;

    caps->szPname[0] = 0;

    device = IOHIDElementGetDevice(joystick->element);
    if (device)
    {
        CFStringRef product_name = IOHIDDeviceGetProperty(device, CFSTR(kIOHIDProductKey));
        if (product_name)
        {
            CFRange range;

            range.location = 0;
            range.length = min(MAXPNAMELEN - 1, CFStringGetLength(product_name));
            CFStringGetCharacters(product_name, range, (UniChar*)caps->szPname);
            caps->szPname[range.length] = 0;
        }
    }

    caps->wMid = MM_MICROSOFT;
    caps->wPid = MM_PC_JOYSTICK;
    caps->wXmin = 0;
    caps->wXmax = 0xFFFF;
    caps->wYmin = 0;
    caps->wYmax = 0xFFFF;
    caps->wZmin = 0;
    caps->wZmax = joystick->axes[AXIS_Z].element ? 0xFFFF : 0;
    caps->wNumButtons = CFArrayGetCount(joystick->buttons);
    if (size == sizeof(JOYCAPSW))
    {
        int i;

        /* complete 95 structure */
        caps->wRmin = 0;
        caps->wRmax = 0xFFFF;
        caps->wUmin = 0;
        caps->wUmax = 0xFFFF;
        caps->wVmin = 0;
        caps->wVmax = 0xFFFF;
        caps->wMaxAxes = 6; /* same as MS Joystick Driver */
        caps->wNumAxes = 0;
        caps->wMaxButtons = 32; /* same as MS Joystick Driver */
        caps->szRegKey[0] = 0;
        caps->szOEMVxD[0] = 0;
        caps->wCaps = 0;

        for (i = 0; i < NUM_AXES; i++)
        {
            if (joystick->axes[i].element)
            {
                caps->wNumAxes++;
                switch (i)
                {
                    case AXIS_Z:  caps->wCaps |= JOYCAPS_HASZ; break;
                    case AXIS_RX: caps->wCaps |= JOYCAPS_HASV; break;
                    case AXIS_RY: caps->wCaps |= JOYCAPS_HASU; break;
                    case AXIS_RZ: caps->wCaps |= JOYCAPS_HASR; break;
                }
            }
        }

        if (joystick->hatswitch)
            caps->wCaps |= JOYCAPS_HASPOV | JOYCAPS_POV4DIR;
    }

    TRACE("name %s buttons %u axes %d caps 0x%08x\n", debugstr_w(caps->szPname), caps->wNumButtons, caps->wNumAxes, caps->wCaps);

    return JOYERR_NOERROR;
}

/*
 * Helper to get the value from an element
 */
static LRESULT driver_getElementValue(IOHIDDeviceRef device, IOHIDElementRef element, IOHIDValueRef *pValueRef)
{
    IOReturn ret;
    ret = IOHIDDeviceGetValue(device, element, pValueRef);
    switch (ret)
    {
        case kIOReturnSuccess:
            return JOYERR_NOERROR;
        case kIOReturnNotAttached:
            return JOYERR_UNPLUGGED;
        default:
            ERR("IOHIDDeviceGetValue returned 0x%x\n",ret);
            return JOYERR_NOCANDO;
    }
}

/**************************************************************************
 *                              driver_joyGetPosEx
 */
LRESULT driver_joyGetPosEx(DWORD_PTR device_id, JOYINFOEX* info)
{
    static const struct {
        DWORD flag;
        off_t offset;
    } axis_map[NUM_AXES] = {
        { JOY_RETURNX, FIELD_OFFSET(JOYINFOEX, dwXpos) },
        { JOY_RETURNY, FIELD_OFFSET(JOYINFOEX, dwYpos) },
        { JOY_RETURNZ, FIELD_OFFSET(JOYINFOEX, dwZpos) },
        { JOY_RETURNV, FIELD_OFFSET(JOYINFOEX, dwVpos) },
        { JOY_RETURNU, FIELD_OFFSET(JOYINFOEX, dwUpos) },
        { JOY_RETURNR, FIELD_OFFSET(JOYINFOEX, dwRpos) },
    };

    joystick_t* joystick;
    IOHIDDeviceRef device;
    CFIndex i, count;
    IOHIDValueRef valueRef;
    long value;
    LRESULT rc;

    if ((joystick = joystick_from_id(device_id)) == NULL)
        return MMSYSERR_NODRIVER;

    if (!open_joystick(joystick))
        return JOYERR_PARMS;

    device = IOHIDElementGetDevice(joystick->element);

    if (info->dwFlags & JOY_RETURNBUTTONS)
    {
        info->dwButtons = 0;
        info->dwButtonNumber = 0;

        count = CFArrayGetCount(joystick->buttons);
        for (i = 0; i < count; i++)
        {
            IOHIDElementRef button = (IOHIDElementRef)CFArrayGetValueAtIndex(joystick->buttons, i);
            rc = driver_getElementValue(device, button, &valueRef);
            if (rc != JOYERR_NOERROR)
                return rc;
            value = IOHIDValueGetIntegerValue(valueRef);
            if (value)
            {
                info->dwButtons |= 1 << i;
                info->dwButtonNumber++;
            }
        }
    }

    for (i = 0; i < NUM_AXES; i++)
    {
        if (info->dwFlags & axis_map[i].flag)
        {
            DWORD* field = (DWORD*)((char*)info + axis_map[i].offset);
            if (joystick->axes[i].element)
            {
                rc = driver_getElementValue(device, joystick->axes[i].element, &valueRef);
                if (rc != JOYERR_NOERROR)
                    return rc;
                value = IOHIDValueGetIntegerValue(valueRef) - joystick->axes[i].min_value;
                *field = MulDiv(value, 0xFFFF, joystick->axes[i].max_value - joystick->axes[i].min_value);
            }
            else
            {
                *field = 0;
                info->dwFlags &= ~axis_map[i].flag;
            }
        }
    }

    if (info->dwFlags & JOY_RETURNPOV)
    {
        if (joystick->hatswitch)
        {
            rc = driver_getElementValue(device, joystick->hatswitch, &valueRef);
            if (rc != JOYERR_NOERROR)
                return rc;
            value = IOHIDValueGetIntegerValue(valueRef);
            if (value >= 8)
                info->dwPOV = JOY_POVCENTERED;
            else
                info->dwPOV = value * 4500;
        }
        else
        {
            info->dwPOV = JOY_POVCENTERED;
            info->dwFlags &= ~JOY_RETURNPOV;
        }
    }

    TRACE("x: %d, y: %d, z: %d, r: %d, u: %d, v: %d, buttons: 0x%04x, pov %d, flags: 0x%04x\n",
          info->dwXpos, info->dwYpos, info->dwZpos, info->dwRpos, info->dwUpos, info->dwVpos, info->dwButtons, info->dwPOV, info->dwFlags);

    return JOYERR_NOERROR;
}

/**************************************************************************
 *                              driver_joyGetPos
 */
LRESULT driver_joyGetPos(DWORD_PTR device_id, JOYINFO* info)
{
    JOYINFOEX ji;
    LONG ret;

    memset(&ji, 0, sizeof(ji));

    ji.dwSize = sizeof(ji);
    ji.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS;
    ret = driver_joyGetPosEx(device_id, &ji);
    if (ret == JOYERR_NOERROR)
    {
        info->wXpos    = ji.dwXpos;
        info->wYpos    = ji.dwYpos;
        info->wZpos    = ji.dwZpos;
        info->wButtons = ji.dwButtons;
    }

    return ret;
}

#endif /* HAVE_IOKIT_HID_IOHIDLIB_H */
