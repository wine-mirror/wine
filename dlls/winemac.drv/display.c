/*
 * MACDRV display settings
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2011, 2012, 2013 Ken Thomases for CodeWeavers Inc.
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

#include "macdrv.h"
#include "winuser.h"
#include "winreg.h"
#include "ddrawi.h"
#define WIN32_NO_STATUS
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(display);

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

struct display_mode_descriptor
{
    DWORD width;
    DWORD height;
    DWORD pixel_width;
    DWORD pixel_height;
    DWORD io_flags;
    double refresh;
    CFStringRef pixel_encoding;
};

static const WCHAR initial_mode_keyW[] = {'I','n','i','t','i','a','l',' ','D','i','s','p','l','a','y',
    ' ','M','o','d','e'};
static const WCHAR pixelencodingW[] = {'P','i','x','e','l','E','n','c','o','d','i','n','g',0};

static BOOL inited_original_display_mode;


static int display_mode_bits_per_pixel(CGDisplayModeRef display_mode)
{
    CFStringRef pixel_encoding;
    int bits_per_pixel = 0;

    pixel_encoding = CGDisplayModeCopyPixelEncoding(display_mode);
    if (pixel_encoding)
    {
        if (CFEqual(pixel_encoding, CFSTR(kIO32BitFloatPixels)))
            bits_per_pixel = 128;
        else if (CFEqual(pixel_encoding, CFSTR(kIO16BitFloatPixels)))
            bits_per_pixel = 64;
        else if (CFEqual(pixel_encoding, CFSTR(kIO64BitDirectPixels)))
            bits_per_pixel = 64;
        else if (CFEqual(pixel_encoding, CFSTR(kIO30BitDirectPixels)))
            bits_per_pixel = 30;
        else if (CFEqual(pixel_encoding, CFSTR(IO32BitDirectPixels)))
            bits_per_pixel = 32;
        else if (CFEqual(pixel_encoding, CFSTR(IO16BitDirectPixels)))
            bits_per_pixel = 16;
        else if (CFEqual(pixel_encoding, CFSTR(IO8BitIndexedPixels)))
            bits_per_pixel = 8;
        else if (CFEqual(pixel_encoding, CFSTR(IO4BitIndexedPixels)))
            bits_per_pixel = 4;
        else if (CFEqual(pixel_encoding, CFSTR(IO2BitIndexedPixels)))
            bits_per_pixel = 2;
        else if (CFEqual(pixel_encoding, CFSTR(IO1BitIndexedPixels)))
            bits_per_pixel = 1;

        CFRelease(pixel_encoding);
    }

    return bits_per_pixel;
}


static BOOL display_mode_is_supported(CGDisplayModeRef display_mode)
{
    uint32_t io_flags = CGDisplayModeGetIOFlags(display_mode);
    return (io_flags & kDisplayModeValidFlag) && (io_flags & kDisplayModeSafeFlag);
}


static void display_mode_to_devmode_fields(CGDirectDisplayID display_id, CGDisplayModeRef display_mode, DEVMODEW *devmode)
{
    uint32_t io_flags;
    double rotation;

    rotation = CGDisplayRotation(display_id);
    devmode->dmDisplayOrientation = ((int)((rotation / 90) + 0.5)) % 4;
    devmode->dmFields |= DM_DISPLAYORIENTATION;

    io_flags = CGDisplayModeGetIOFlags(display_mode);
    if (io_flags & kDisplayModeStretchedFlag)
        devmode->dmDisplayFixedOutput = DMDFO_STRETCH;
    else
        devmode->dmDisplayFixedOutput = DMDFO_CENTER;
    devmode->dmFields |= DM_DISPLAYFIXEDOUTPUT;

    devmode->dmBitsPerPel = display_mode_bits_per_pixel(display_mode);
    if (devmode->dmBitsPerPel)
        devmode->dmFields |= DM_BITSPERPEL;

    devmode->dmPelsWidth = CGDisplayModeGetWidth(display_mode);
    devmode->dmPelsHeight = CGDisplayModeGetHeight(display_mode);
    devmode->dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;

    devmode->dmDisplayFlags = 0;
    if (io_flags & kDisplayModeInterlacedFlag)
        devmode->dmDisplayFlags |= DM_INTERLACED;
    if (!display_mode_is_supported(display_mode))
        devmode->dmDisplayFlags |= WINE_DM_UNSUPPORTED;
    devmode->dmFields |= DM_DISPLAYFLAGS;

    devmode->dmDisplayFrequency = CGDisplayModeGetRefreshRate(display_mode);
    if (!devmode->dmDisplayFrequency)
        devmode->dmDisplayFrequency = 60;
    devmode->dmFields |= DM_DISPLAYFREQUENCY;
}


static BOOL set_setting_value(HKEY hkey, const char *name, DWORD val)
{
    WCHAR nameW[128];
    UNICODE_STRING str = { asciiz_to_unicode(nameW, name) - sizeof(WCHAR), sizeof(nameW), nameW };
    return !NtSetValueKey(hkey, &str, 0, REG_DWORD, &val, sizeof(val));
}


static BOOL write_display_settings(HKEY parent_hkey, CGDirectDisplayID displayID)
{
    BOOL ret = FALSE;
    char display_key_name[19];
    HKEY display_hkey;
    CGDisplayModeRef display_mode;
    UNICODE_STRING str = RTL_CONSTANT_STRING(pixelencodingW);
    DWORD val;
    CFStringRef pixel_encoding;
    size_t len;
    WCHAR* buf = NULL;

    snprintf(display_key_name, sizeof(display_key_name), "Display 0x%08x", CGDisplayUnitNumber(displayID));
    /* @@ Wine registry key: HKLM\Software\Wine\Mac Driver\Initial Display Mode\Display 0xnnnnnnnn */
    if (!(display_hkey = reg_create_ascii_key(parent_hkey, display_key_name, REG_OPTION_VOLATILE, NULL)))
        return FALSE;

    display_mode = CGDisplayCopyDisplayMode(displayID);
    if (!display_mode)
        goto fail;

    val = CGDisplayModeGetWidth(display_mode);
    if (!set_setting_value(display_hkey, "Width", val))
        goto fail;
    val = CGDisplayModeGetHeight(display_mode);
    if (!set_setting_value(display_hkey, "Height", val))
        goto fail;
    val = CGDisplayModeGetRefreshRate(display_mode) * 100;
    if (!set_setting_value(display_hkey, "RefreshRateTimes100", val))
        goto fail;
    val = CGDisplayModeGetIOFlags(display_mode);
    if (!set_setting_value(display_hkey, "IOFlags", val))
        goto fail;
    val = CGDisplayModeGetPixelWidth(display_mode);
    if (!set_setting_value(display_hkey, "PixelWidth", val))
        goto fail;
    val = CGDisplayModeGetPixelHeight(display_mode);
    if (!set_setting_value(display_hkey, "PixelHeight", val))
        goto fail;

    pixel_encoding = CGDisplayModeCopyPixelEncoding(display_mode);
    len = CFStringGetLength(pixel_encoding);
    buf = malloc((len + 1) * sizeof(WCHAR));
    CFStringGetCharacters(pixel_encoding, CFRangeMake(0, len), (UniChar*)buf);
    buf[len] = 0;
    CFRelease(pixel_encoding);
    if (NtSetValueKey(display_hkey, &str, 0, REG_SZ, (const BYTE*)buf, (len + 1) * sizeof(WCHAR)))
        goto fail;

    ret = TRUE;

fail:
    free(buf);
    if (display_mode) CGDisplayModeRelease(display_mode);
    NtClose(display_hkey);
    if (!ret)
    {
        WCHAR nameW[64];
        reg_delete_tree(parent_hkey, nameW, asciiz_to_unicode(nameW, display_key_name) - sizeof(WCHAR));
    }
    return ret;
}


static void init_original_display_mode(void)
{
    BOOL success = FALSE;
    HKEY mac_driver_hkey, parent_hkey;
    DWORD disposition;
    struct macdrv_display *displays = NULL;
    int num_displays, i;

    if (inited_original_display_mode)
        return;

    /* @@ Wine registry key: HKLM\Software\Wine\Mac Driver */
    mac_driver_hkey = reg_create_ascii_key(NULL, "\\Registry\\Machine\\Software\\Wine\\Mac Driver",
                                           0, NULL);
    if (!mac_driver_hkey)
        return;

    /* @@ Wine registry key: HKLM\Software\Wine\Mac Driver\Initial Display Mode */
    if (!(parent_hkey = reg_create_key(mac_driver_hkey, initial_mode_keyW, sizeof(initial_mode_keyW),
                                       REG_OPTION_VOLATILE, &disposition)))
    {
        parent_hkey = NULL;
        goto fail;
    }

    /* If we didn't create a new key, then it already existed.  Something already stored
       the initial display mode since Wine was started.  We don't want to overwrite it. */
    if (disposition != REG_CREATED_NEW_KEY)
        goto done;

    if (macdrv_get_displays(&displays, &num_displays))
        goto fail;

    for (i = 0; i < num_displays; i++)
    {
        if (!write_display_settings(parent_hkey, displays[i].displayID))
            goto fail;
    }

done:
    success = TRUE;

fail:
    macdrv_free_displays(displays);
    NtClose(parent_hkey);
    if (!success && parent_hkey)
        reg_delete_tree(mac_driver_hkey, initial_mode_keyW, sizeof(initial_mode_keyW));
    NtClose(mac_driver_hkey);
    if (success)
        inited_original_display_mode = TRUE;
}


static BOOL read_dword(HKEY hkey, const char* name, DWORD* val)
{
    char buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(*val)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR nameW[64];
    asciiz_to_unicode(nameW, name);
    if (query_reg_value(hkey, nameW, value, sizeof(buffer)) != sizeof(*val) || value->Type != REG_DWORD)
        return FALSE;
    *val = *(DWORD *)value->Data;
    return TRUE;
}


static void free_display_mode_descriptor(struct display_mode_descriptor* desc)
{
    if (desc)
    {
        if (desc->pixel_encoding)
            CFRelease(desc->pixel_encoding);
        free(desc);
    }
}


static struct display_mode_descriptor* create_original_display_mode_descriptor(CGDirectDisplayID displayID)
{
    static const char display_key_format[] =
        "\\Registry\\Machine\\Software\\Wine\\Mac Driver\\Initial Display Mode\\Display 0x%08x";
    struct display_mode_descriptor* ret = NULL;
    struct display_mode_descriptor* desc;
    char display_key[sizeof(display_key_format) + 10];
    WCHAR nameW[ARRAYSIZE(display_key)];
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    HKEY hkey;
    DWORD refresh100;

    init_original_display_mode();

    snprintf(display_key, sizeof(display_key), display_key_format, CGDisplayUnitNumber(displayID));
    /* @@ Wine registry key: HKLM\Software\Wine\Mac Driver\Initial Display Mode\Display 0xnnnnnnnn */
    if (!(hkey = reg_open_key(NULL, nameW, asciiz_to_unicode(nameW, display_key) - sizeof(WCHAR))))
        return NULL;

    desc = malloc(sizeof(*desc));
    desc->pixel_encoding = NULL;

    if (!read_dword(hkey, "Width", &desc->width) ||
        !read_dword(hkey, "Height", &desc->height) ||
        !read_dword(hkey, "RefreshRateTimes100", &refresh100) ||
        !read_dword(hkey, "IOFlags", &desc->io_flags))
        goto done;
    if (refresh100)
        desc->refresh = refresh100 / 100.0;
    else
        desc->refresh = 60;

    if (!read_dword(hkey, "PixelWidth", &desc->pixel_width) ||
        !read_dword(hkey, "PixelHeight", &desc->pixel_height))
    {
        desc->pixel_width = desc->width;
        desc->pixel_height = desc->height;
    }

    if (!query_reg_value(hkey, pixelencodingW, value, sizeof(buffer)) || value->Type != REG_SZ)
        goto done;
    desc->pixel_encoding = CFStringCreateWithCharacters(NULL, (const UniChar*)value->Data,
                                                        lstrlenW((const WCHAR *)value->Data));

    ret = desc;

done:
    if (!ret)
        free_display_mode_descriptor(desc);
    NtClose(hkey);
    return ret;
}


static BOOL display_mode_matches_descriptor(CGDisplayModeRef mode, const struct display_mode_descriptor* desc)
{
    DWORD mode_io_flags;
    double mode_refresh;
    CFStringRef mode_pixel_encoding;

    if (!desc)
        return FALSE;

    if (CGDisplayModeGetWidth(mode) != desc->width ||
        CGDisplayModeGetHeight(mode) != desc->height)
        return FALSE;

    mode_io_flags = CGDisplayModeGetIOFlags(mode);
    if ((desc->io_flags ^ mode_io_flags) & (kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeStretchedFlag |
                                            kDisplayModeInterlacedFlag | kDisplayModeTelevisionFlag))
        return FALSE;

    mode_refresh = CGDisplayModeGetRefreshRate(mode);
    if (!mode_refresh)
        mode_refresh = 60;
    if (fabs(desc->refresh - mode_refresh) > 0.1)
        return FALSE;

    if (CGDisplayModeGetPixelWidth(mode) != desc->pixel_width ||
        CGDisplayModeGetPixelHeight(mode) != desc->pixel_height)
        return FALSE;

    mode_pixel_encoding = CGDisplayModeCopyPixelEncoding(mode);
    if (!CFEqual(mode_pixel_encoding, desc->pixel_encoding))
    {
        CFRelease(mode_pixel_encoding);
        return FALSE;
    }
    CFRelease(mode_pixel_encoding);

    return TRUE;
}


static int get_default_bpp(void)
{
    static int cached;
    int ret;

    if (!cached)
    {
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
        if (mode)
        {
            cached = display_mode_bits_per_pixel(mode);
            CFRelease(mode);
        }

        if (!cached)
            cached = 32;
    }

    ret = cached;

    TRACE(" -> %d\n", ret);
    return ret;
}


static CFDictionaryRef create_mode_dict(CGDisplayModeRef display_mode, BOOL is_original)
{
    CFDictionaryRef ret;
    SInt32 io_flags = CGDisplayModeGetIOFlags(display_mode);
    SInt64 width = CGDisplayModeGetWidth(display_mode);
    SInt64 height = CGDisplayModeGetHeight(display_mode);
    double refresh_rate = CGDisplayModeGetRefreshRate(display_mode);
    CFStringRef pixel_encoding = CGDisplayModeCopyPixelEncoding(display_mode);
    CFNumberRef cf_io_flags, cf_width, cf_height, cf_refresh;

    if (retina_enabled && is_original)
    {
        width *= 2;
        height *= 2;
    }

    io_flags &= kDisplayModeInterlacedFlag | kDisplayModeStretchedFlag | kDisplayModeTelevisionFlag;
    cf_io_flags = CFNumberCreate(NULL, kCFNumberSInt32Type, &io_flags);
    cf_width = CFNumberCreate(NULL, kCFNumberSInt64Type, &width);
    cf_height = CFNumberCreate(NULL, kCFNumberSInt64Type, &height);
    cf_refresh = CFNumberCreate(NULL, kCFNumberDoubleType, &refresh_rate);

    {
        static const CFStringRef keys[] = {
            CFSTR("io_flags"),
            CFSTR("width"),
            CFSTR("height"),
            CFSTR("pixel_encoding"),
            CFSTR("refresh_rate"),
        };
        const void* values[ARRAY_SIZE(keys)] = {
            cf_io_flags,
            cf_width,
            cf_height,
            pixel_encoding,
            cf_refresh,
        };

        ret = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, ARRAY_SIZE(keys),
                                 &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }

    CFRelease(pixel_encoding);
    CFRelease(cf_io_flags);
    CFRelease(cf_width);
    CFRelease(cf_height);
    CFRelease(cf_refresh);

    return ret;
}


/***********************************************************************
 *              mode_is_preferred
 *
 * Returns whether new_mode ought to be preferred over old_mode - that is,
 * whether new_mode is a better option to expose as a valid mode to switch to.
 * old_mode may be NULL, in which case the function returns true if the mode
 * should be considered at all.
 * old_mode and new_mode are guaranteed to have identical return values from
 * create_mode_dict. So, they will have the same point dimension and relevant
 * IO flags, among other properties.
 */
static BOOL mode_is_preferred(CGDisplayModeRef new_mode, CGDisplayModeRef old_mode,
                              struct display_mode_descriptor *original_mode_desc,
                              BOOL include_unsupported)
{
    BOOL new_is_supported;
    CFStringRef pixel_encoding;
    size_t width_points, height_points;
    BOOL new_usable_for_desktop, old_usable_for_desktop;
    size_t old_width_pixels, old_height_pixels, new_width_pixels, new_height_pixels;
    BOOL old_size_same, new_size_same;

    /* If a given mode is the user's default, then always list it in preference to any similar
       modes that may exist. */
    if (display_mode_matches_descriptor(new_mode, original_mode_desc))
        return TRUE;

    /* Skip unsupported modes unless told to do otherwise. */
    new_is_supported = display_mode_is_supported(new_mode);
    if (!new_is_supported && !include_unsupported)
        return FALSE;

    pixel_encoding = CGDisplayModeCopyPixelEncoding(new_mode);
    if (pixel_encoding)
    {
        BOOL bpp30 = CFEqual(pixel_encoding, CFSTR(kIO30BitDirectPixels));
        CFRelease(pixel_encoding);
        if (bpp30)
        {
            /* This is an odd pixel encoding.  It seems it's only returned
               when using kCGDisplayShowDuplicateLowResolutionModes.  It's
               32bpp in terms of the actual raster layout, but it's 10
               bits per component.  I think that no Windows program is
               likely to need it and they will probably be confused by it.
               Skip it. */
            return FALSE;
        }
    }

    if (!old_mode)
        return TRUE;

    /* Prefer the original mode over any similar mode. */
    if (display_mode_matches_descriptor(old_mode, original_mode_desc))
        return FALSE;

    /* Prefer supported modes over similar unsupported ones. */
    if (!new_is_supported && display_mode_is_supported(old_mode))
        return FALSE;

    /* Prefer modes that are usable for desktop over ones that aren't. */
    new_usable_for_desktop = CGDisplayModeIsUsableForDesktopGUI(new_mode);
    old_usable_for_desktop = CGDisplayModeIsUsableForDesktopGUI(old_mode);
    if (new_usable_for_desktop && !old_usable_for_desktop)
        return TRUE;

    /* Otherwise, prefer a mode whose pixel size equals its point size over one which
       is scaled. */
    width_points = CGDisplayModeGetWidth(new_mode);
    height_points = CGDisplayModeGetHeight(new_mode);
    new_width_pixels = CGDisplayModeGetPixelWidth(new_mode);
    new_height_pixels = CGDisplayModeGetPixelHeight(new_mode);
    old_width_pixels = CGDisplayModeGetPixelWidth(old_mode);
    old_height_pixels = CGDisplayModeGetPixelHeight(old_mode);
    new_size_same = (new_width_pixels == width_points && new_height_pixels == height_points);
    old_size_same = (old_width_pixels == width_points && old_height_pixels == height_points);

    if (new_size_same && !old_size_same)
        return TRUE;

    if (!new_size_same && old_size_same)
        return FALSE;

    /* Otherwise, prefer the mode with the smaller pixel size. */
    return new_width_pixels < old_width_pixels && new_height_pixels < old_height_pixels;
}


static CFComparisonResult mode_compare(const void *p1, const void *p2, void *context)
{
    CGDisplayModeRef a = (CGDisplayModeRef)p1, b = (CGDisplayModeRef)p2;
    size_t a_val, b_val;
    double a_refresh_rate, b_refresh_rate;

    /* Sort by bpp descending, */
    a_val = display_mode_bits_per_pixel(a);
    b_val = display_mode_bits_per_pixel(b);
    if (a_val < b_val)
        return kCFCompareGreaterThan;
    else if (a_val > b_val)
        return kCFCompareLessThan;

    /* then width ascending, */
    a_val = CGDisplayModeGetWidth(a);
    b_val = CGDisplayModeGetWidth(b);
    if (a_val < b_val)
        return kCFCompareLessThan;
    else if (a_val > b_val)
        return kCFCompareGreaterThan;

    /* then height ascending, */
    a_val = CGDisplayModeGetHeight(a);
    b_val = CGDisplayModeGetHeight(b);
    if (a_val < b_val)
        return kCFCompareLessThan;
    else if (a_val > b_val)
        return kCFCompareGreaterThan;

    /* then refresh rate descending. */
    a_refresh_rate = CGDisplayModeGetRefreshRate(a);
    b_refresh_rate = CGDisplayModeGetRefreshRate(b);
    if (a_refresh_rate < b_refresh_rate)
        return kCFCompareGreaterThan;
    else if (a_refresh_rate > b_refresh_rate)
        return kCFCompareLessThan;

    return kCFCompareEqualTo;
}


/***********************************************************************
 *              copy_display_modes
 *
 * Wrapper around CGDisplayCopyAllDisplayModes() to include additional
 * modes on Retina-capable systems, but filter those which would confuse
 * Windows apps (basically duplicates at different DPIs).
 *
 * For example, some Retina Macs support a 1920x1200 mode, but it's not
 * returned from CGDisplayCopyAllDisplayModes() without special options.
 * This is especially bad if that's the user's default mode, since then
 * no "available" mode matches the initial settings.
 *
 * If include_unsupported is FALSE, display modes with IO flags that
 * indicate that they are invalid or unsafe are filtered.
 */
static CFArrayRef copy_display_modes(CGDirectDisplayID display, BOOL include_unsupported)
{
    CFArrayRef modes = NULL;
    CFDictionaryRef options;
    struct display_mode_descriptor* desc;
    CFMutableDictionaryRef modes_by_size;
    CFIndex i, count;
    CGDisplayModeRef* mode_array;

    options = CFDictionaryCreate(NULL, (const void**)&kCGDisplayShowDuplicateLowResolutionModes,
                                 (const void**)&kCFBooleanTrue, 1, &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

    modes = CGDisplayCopyAllDisplayModes(display, options);
    if (options)
        CFRelease(options);
    if (!modes)
        return NULL;

    desc = create_original_display_mode_descriptor(display);

    modes_by_size = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    count = CFArrayGetCount(modes);
    for (i = 0; i < count; i++)
    {
        CGDisplayModeRef new_mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        BOOL new_is_original = display_mode_matches_descriptor(new_mode, desc);
        CFDictionaryRef key = create_mode_dict(new_mode, new_is_original);
        CGDisplayModeRef old_mode = (CGDisplayModeRef)CFDictionaryGetValue(modes_by_size, key);

        if (mode_is_preferred(new_mode, old_mode, desc, include_unsupported))
            CFDictionarySetValue(modes_by_size, key, new_mode);

        CFRelease(key);
    }

    free_display_mode_descriptor(desc);
    CFRelease(modes);

    count = CFDictionaryGetCount(modes_by_size);
    mode_array = malloc(count * sizeof(mode_array[0]));
    CFDictionaryGetKeysAndValues(modes_by_size, NULL, (const void **)mode_array);
    modes = CFArrayCreate(NULL, (const void **)mode_array, count, &kCFTypeArrayCallBacks);
    free(mode_array);
    CFRelease(modes_by_size);

    if (modes)
    {
        CFIndex count = CFArrayGetCount(modes);
        CFMutableArrayRef sorted_modes = CFArrayCreateMutableCopy(NULL, count, modes);
        CFRelease(modes);
        CFArraySortValues(sorted_modes, CFRangeMake(0, count), mode_compare, NULL);
        return sorted_modes;
    }

    return NULL;
}


void check_retina_status(void)
{
    if (retina_enabled)
    {
        struct display_mode_descriptor* desc = create_original_display_mode_descriptor(kCGDirectMainDisplay);
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
        BOOL new_value = display_mode_matches_descriptor(mode, desc);

        CGDisplayModeRelease(mode);
        free_display_mode_descriptor(desc);

        if (new_value != retina_on)
            macdrv_set_cocoa_retina_mode(new_value);
    }
}

static BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

static CGDisplayModeRef find_best_display_mode(DEVMODEW *devmode, CFArrayRef display_modes, int bpp, struct display_mode_descriptor *desc)
{
    CFIndex count, i, best;
    CGDisplayModeRef best_display_mode;

    best_display_mode = NULL;

    count = CFArrayGetCount(display_modes);
    for (i = 0; i < count; i++)
    {
        CGDisplayModeRef display_mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(display_modes, i);
        BOOL is_original = display_mode_matches_descriptor(display_mode, desc);
        uint32_t io_flags = CGDisplayModeGetIOFlags(display_mode);
        int mode_bpp = display_mode_bits_per_pixel(display_mode);
        size_t width = CGDisplayModeGetWidth(display_mode);
        size_t height = CGDisplayModeGetHeight(display_mode);
        double refresh_rate = CGDisplayModeGetRefreshRate(display_mode);
        if (!refresh_rate)
            refresh_rate = 60;

        if (is_original && retina_enabled)
        {
            width *= 2;
            height *= 2;
        }

        if (bpp != mode_bpp)
            continue;

        if (devmode->dmPelsWidth != width)
            continue;
        if (devmode->dmPelsHeight != height)
            continue;
        if (devmode->dmDisplayFrequency != (DWORD)refresh_rate)
            continue;
        if (!(devmode->dmDisplayFlags & DM_INTERLACED) != !(io_flags & kDisplayModeInterlacedFlag))
            continue;
        if (!(devmode->dmDisplayFixedOutput == DMDFO_STRETCH) != !(io_flags & kDisplayModeStretchedFlag))
            continue;

        if (best_display_mode)
            continue;

        best_display_mode = display_mode;
        best = i;
    }

    if (best_display_mode)
        TRACE("Requested display settings match mode %ld\n", best);

    return best_display_mode;
}

/***********************************************************************
 *              ChangeDisplaySettings  (MACDRV.@)
 *
 */
LONG macdrv_ChangeDisplaySettings(LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid)
{
    LONG ret = DISP_CHANGE_SUCCESSFUL;
    DEVMODEW *mode;
    int bpp;
    struct macdrv_display *macdrv_displays;
    int num_displays;
    CFArrayRef display_modes;
    struct display_mode_descriptor *desc;
    CGDisplayModeRef best_display_mode;

    TRACE("%p %s %p 0x%08x %p\n", displays, debugstr_w(primary_name), hwnd, (unsigned int)flags, lpvoid);

    init_original_display_mode();

    if (macdrv_get_displays(&macdrv_displays, &num_displays))
        return DISP_CHANGE_FAILED;

    display_modes = copy_display_modes(macdrv_displays[0].displayID, FALSE);
    if (!display_modes)
    {
        macdrv_free_displays(macdrv_displays);
        return DISP_CHANGE_FAILED;
    }

    bpp = get_default_bpp();

    desc = create_original_display_mode_descriptor(macdrv_displays[0].displayID);

    for (mode = displays; mode->dmSize && !ret; mode = NEXT_DEVMODEW(mode))
    {
        if (wcsicmp(primary_name, mode->dmDeviceName))
        {
            FIXME("Changing non-primary adapter settings is currently unsupported.\n");
            continue;
        }
        if (is_detached_mode(mode))
        {
            FIXME("Detaching adapters is currently unsupported.\n");
            continue;
        }

        if (mode->dmBitsPerPel != bpp)
            TRACE("using default %d bpp instead of caller's request %d bpp\n", bpp, (int)mode->dmBitsPerPel);

        TRACE("looking for %dx%dx%dbpp @%d Hz", (int)mode->dmPelsWidth, (int)mode->dmPelsHeight,
              bpp, (int)mode->dmDisplayFrequency);
        TRACE(" %sstretched", mode->dmDisplayFixedOutput == DMDFO_STRETCH ? "" : "un");
        TRACE(" %sinterlaced", mode->dmDisplayFlags & DM_INTERLACED ? "" : "non-");
        TRACE("\n");

        if (!(best_display_mode = find_best_display_mode(mode, display_modes, bpp, desc)))
        {
            ERR("No matching mode found %ux%ux%d @%u!\n", (unsigned int)mode->dmPelsWidth, (unsigned int)mode->dmPelsHeight,
                bpp, (unsigned int)mode->dmDisplayFrequency);
            ret = DISP_CHANGE_BADMODE;
        }
        else if (!macdrv_set_display_mode(&macdrv_displays[0], best_display_mode))
        {
            WARN("Failed to set display mode\n");
            ret = DISP_CHANGE_FAILED;
        }
    }

    free_display_mode_descriptor(desc);
    CFRelease(display_modes);
    macdrv_free_displays(macdrv_displays);
    macdrv_reset_device_metrics();

    return ret;
}


static DEVMODEW *display_get_modes(CGDirectDisplayID display_id, int *modes_count)
{
    int default_bpp = get_default_bpp(), synth_count = 0, count, i;
    BOOL modes_has_8bpp = FALSE, modes_has_16bpp = FALSE;
    struct display_mode_descriptor *desc;
    DEVMODEW *devmodes;
    CFArrayRef modes;

    modes = copy_display_modes(display_id, TRUE);
    if (!modes)
        return NULL;

    count = CFArrayGetCount(modes);
    for (i = 0; i < count && !(modes_has_8bpp && modes_has_16bpp); i++)
    {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
        int bpp = display_mode_bits_per_pixel(mode);
        if (bpp == 8)
            modes_has_8bpp = TRUE;
        else if (bpp == 16)
            modes_has_16bpp = TRUE;
    }

    if (!(devmodes = calloc(count * 3, sizeof(DEVMODEW))))
    {
        CFRelease(modes);
        return NULL;
    }

    desc = create_original_display_mode_descriptor(display_id);
    for (i = 0; i < count; i++)
    {
        CGDisplayModeRef mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

        memset(devmodes + i, 0, sizeof(*devmodes));
        devmodes[i].dmSize = sizeof(*devmodes);

        display_mode_to_devmode_fields(display_id, mode, devmodes + i);

        if (retina_enabled && display_mode_matches_descriptor(mode, desc))
        {
            devmodes[i].dmPelsWidth *= 2;
            devmodes[i].dmPelsHeight *= 2;
        }
    }
    free_display_mode_descriptor(desc);

    for (i = 0; !modes_has_16bpp && i < count; i++)
    {
        /* We only synthesize modes from those having the default bpp. */
        if (devmodes[i].dmBitsPerPel != default_bpp) continue;
        devmodes[count + synth_count] = devmodes[i];
        devmodes[count + synth_count].dmBitsPerPel = 16;
        synth_count++;
    }

    for (i = 0; !modes_has_8bpp && i < count; i++)
    {
        /* We only synthesize modes from those having the default bpp. */
        if (devmodes[i].dmBitsPerPel != default_bpp) continue;
        devmodes[count + synth_count] = devmodes[i];
        devmodes[count + synth_count].dmBitsPerPel = 8;
        synth_count++;
    }

    CFRelease(modes);
    *modes_count = count + synth_count;
    return devmodes;
}

static void display_get_current_mode(struct macdrv_display *display, DEVMODEW *devmode)
{
    CGDisplayModeRef display_mode;
    CGDirectDisplayID display_id;

    display_id = display->displayID;
    display_mode = CGDisplayCopyDisplayMode(display_id);

    devmode->dmPosition.x = CGRectGetMinX(display->frame);
    devmode->dmPosition.y = CGRectGetMinY(display->frame);
    devmode->dmFields |= DM_POSITION;

    display_mode_to_devmode_fields(display_id, display_mode, devmode);
    if (retina_enabled)
    {
        struct display_mode_descriptor *desc = create_original_display_mode_descriptor(display_id);
        if (display_mode_matches_descriptor(display_mode, desc))
        {
            devmode->dmPelsWidth *= 2;
            devmode->dmPelsHeight *= 2;
        }
        free_display_mode_descriptor(desc);
    }

    CFRelease(display_mode);
}

/***********************************************************************
 *              GetDeviceGammaRamp (MACDRV.@)
 */
BOOL macdrv_GetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
    BOOL ret = FALSE;
    DDGAMMARAMP *r = ramp;
    struct macdrv_display *displays;
    int num_displays;
    uint32_t mac_entries;
    int win_entries = ARRAY_SIZE(r->red);
    CGGammaValue *red, *green, *blue;
    CGError err;
    int win_entry;

    TRACE("dev %p ramp %p\n", dev, ramp);

    if (macdrv_get_displays(&displays, &num_displays))
    {
        WARN("failed to get Mac displays\n");
        return FALSE;
    }

    mac_entries = CGDisplayGammaTableCapacity(displays[0].displayID);
    red = malloc(mac_entries * sizeof(red[0]) * 3);
    if (!red)
        goto done;
    green = red + mac_entries;
    blue = green + mac_entries;

    err = CGGetDisplayTransferByTable(displays[0].displayID, mac_entries, red, green,
                                      blue, &mac_entries);
    if (err != kCGErrorSuccess)
    {
        WARN("failed to get Mac gamma table: %d\n", err);
        goto done;
    }

    if (mac_entries == win_entries)
    {
        for (win_entry = 0; win_entry < win_entries; win_entry++)
        {
            r->red[win_entry]   = red[win_entry]   * 65535 + 0.5;
            r->green[win_entry] = green[win_entry] * 65535 + 0.5;
            r->blue[win_entry]  = blue[win_entry]  * 65535 + 0.5;
        }
    }
    else
    {
        for (win_entry = 0; win_entry < win_entries; win_entry++)
        {
            double mac_pos = win_entry * (mac_entries - 1) / (double)(win_entries - 1);
            int mac_entry = mac_pos;
            double red_value, green_value, blue_value;

            if (mac_entry == mac_entries - 1)
            {
                red_value   = red[mac_entry];
                green_value = green[mac_entry];
                blue_value  = blue[mac_entry];
            }
            else
            {
                double distance = mac_pos - mac_entry;

                red_value   = red[mac_entry]   * (1 - distance) + red[mac_entry + 1]   * distance;
                green_value = green[mac_entry] * (1 - distance) + green[mac_entry + 1] * distance;
                blue_value  = blue[mac_entry]  * (1 - distance) + blue[mac_entry + 1]  * distance;
            }

            r->red[win_entry]   = red_value   * 65535 + 0.5;
            r->green[win_entry] = green_value * 65535 + 0.5;
            r->blue[win_entry]  = blue_value  * 65535 + 0.5;
        }
    }

    ret = TRUE;

done:
    free(red);
    macdrv_free_displays(displays);
    return ret;
}

/***********************************************************************
 *              SetDeviceGammaRamp (MACDRV.@)
 */
BOOL macdrv_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
    DDGAMMARAMP *r = ramp;
    struct macdrv_display *displays;
    int num_displays;
    int win_entries = ARRAY_SIZE(r->red);
    CGGammaValue *red, *green, *blue;
    int i;
    CGError err = kCGErrorFailure;

    TRACE("dev %p ramp %p\n", dev, ramp);

    if (!allow_set_gamma)
    {
        TRACE("disallowed by registry setting\n");
        return FALSE;
    }

    if (macdrv_get_displays(&displays, &num_displays))
    {
        WARN("failed to get Mac displays\n");
        return FALSE;
    }

    red = malloc(win_entries * sizeof(red[0]) * 3);
    if (!red)
        goto done;
    green = red + win_entries;
    blue = green + win_entries;

    for (i = 0; i < win_entries; i++)
    {
        red[i]      = r->red[i] / 65535.0;
        green[i]    = r->green[i] / 65535.0;
        blue[i]     = r->blue[i] / 65535.0;
    }

    err = CGSetDisplayTransferByTable(displays[0].displayID, win_entries, red, green, blue);
    if (err != kCGErrorSuccess)
        WARN("failed to set display gamma table: %d\n", err);

done:
    free(red);
    macdrv_free_displays(displays);
    return (err == kCGErrorSuccess);
}

/***********************************************************************
 *              macdrv_displays_changed
 *
 * Handler for DISPLAYS_CHANGED events.
 */
void macdrv_displays_changed(const macdrv_event *event)
{
    HWND hwnd = NtUserGetDesktopWindow();

    /* A system display change will get delivered to all GUI-attached threads,
       so the desktop-window-owning thread will get it and all others should
       ignore it.  A synthesized display change event due to activation
       will only get delivered to the activated process.  So, it needs to
       process it (by sending it to the desktop window). */
    if (event->displays_changed.activating ||
        NtUserGetWindowThread(hwnd, NULL) == GetCurrentThreadId())
        NtUserCallNoParam(NtUserCallNoParam_DisplayModeChanged);
}

UINT macdrv_UpdateDisplayDevices(const struct gdi_device_manager *device_manager, void *param)
{
    struct macdrv_adapter *adapters, *adapter;
    struct macdrv_monitor *monitors, *monitor;
    struct macdrv_gpu *gpus, *gpu;
    struct macdrv_display *displays, *display;
    INT gpu_count, adapter_count, monitor_count, mode_count, display_count;
    DEVMODEW *modes;

    if (macdrv_get_displays(&displays, &display_count))
    {
        displays = NULL;
        display_count = 0;
    }

    /* Initialize GPUs */
    if (macdrv_get_gpus(&gpus, &gpu_count))
    {
        ERR("could not get GPUs\n");
        return STATUS_UNSUCCESSFUL;
    }
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = gpus; gpu < gpus + gpu_count; gpu++)
    {
        struct pci_id pci_id =
        {
            .vendor = gpu->vendor_id,
            .device = gpu->device_id,
            .subsystem = gpu->subsys_id,
            .revision = gpu->revision_id,
        };
        device_manager->add_gpu(gpu->name, &pci_id, NULL, param);

        /* Initialize adapters */
        if (macdrv_get_adapters(gpu->id, &adapters, &adapter_count)) break;
        TRACE("GPU: %llx %s, adapter count: %d\n", gpu->id, debugstr_a(gpu->name), adapter_count);

        for (adapter = adapters; adapter < adapters + adapter_count; adapter++)
        {
            DEVMODEW current_mode = { .dmSize = sizeof(current_mode) };
            UINT dpi = NtUserGetSystemDpiForProcess( NULL );
            char buffer[32];

            sprintf( buffer, "%04x", adapter->id );
            device_manager->add_source( buffer, adapter->state_flags, dpi, param );

            if (macdrv_get_monitors(adapter->id, &monitors, &monitor_count)) break;
            TRACE("adapter: %#x, monitor count: %d\n", adapter->id, monitor_count);

            /* Initialize monitors */
            for (monitor = monitors; monitor < monitors + monitor_count; monitor++)
            {
                struct gdi_monitor gdi_monitor =
                {
                    .rc_monitor = rect_from_cgrect(monitor->rc_monitor),
                    .rc_work = rect_from_cgrect(monitor->rc_work),
                };
                device_manager->add_monitor( &gdi_monitor, param );
            }

            /* Get the current mode */
            if (displays)
            {
                for (display = displays; display < displays + display_count; display++)
                {
                    if (display->displayID == adapter->id)
                    {
                        display_get_current_mode(display, &current_mode);
                        break;
                    }
                }
            }

            if (!(modes = display_get_modes(adapter->id, &mode_count))) break;
            device_manager->add_modes( &current_mode, mode_count, modes, param );
            free(modes);
            macdrv_free_monitors(monitors);
        }

        macdrv_free_adapters(adapters);
    }

    macdrv_free_gpus(gpus);
    macdrv_free_displays(displays);
    return STATUS_SUCCESS;
}
