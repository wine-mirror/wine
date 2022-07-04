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


BOOL macdrv_EnumDisplaySettingsEx(LPCWSTR devname, DWORD mode, LPDEVMODEW devmode, DWORD flags);

static const WCHAR initial_mode_keyW[] = {'I','n','i','t','i','a','l',' ','D','i','s','p','l','a','y',
    ' ','M','o','d','e'};
static const WCHAR pixelencodingW[] = {'P','i','x','e','l','E','n','c','o','d','i','n','g',0};

static CFArrayRef modes;
static BOOL modes_has_8bpp, modes_has_16bpp;
static int default_mode_bpp;
static pthread_mutex_t modes_mutex = PTHREAD_MUTEX_INITIALIZER;

static BOOL inited_original_display_mode;


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
    UNICODE_STRING str;
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

#if defined(MAC_OS_X_VERSION_10_8) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    if (&CGDisplayModeGetPixelWidth != NULL && &CGDisplayModeGetPixelHeight != NULL)
    {
        val = CGDisplayModeGetPixelWidth(display_mode);
        if (!set_setting_value(display_hkey, "PixelWidth", val))
            goto fail;
        val = CGDisplayModeGetPixelHeight(display_mode);
        if (!set_setting_value(display_hkey, "PixelHeight", val))
            goto fail;
    }
#endif

    pixel_encoding = CGDisplayModeCopyPixelEncoding(display_mode);
    len = CFStringGetLength(pixel_encoding);
    buf = malloc((len + 1) * sizeof(WCHAR));
    CFStringGetCharacters(pixel_encoding, CFRangeMake(0, len), (UniChar*)buf);
    buf[len] = 0;
    CFRelease(pixel_encoding);
    RtlInitUnicodeString(&str, pixelencodingW);
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
    if (!(hkey = reg_open_key(NULL, nameW, asciiz_to_unicode(nameW, display_key))))
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

#if defined(MAC_OS_X_VERSION_10_8) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    if (&CGDisplayModeGetPixelWidth != NULL && &CGDisplayModeGetPixelHeight != NULL)
    {
        if (CGDisplayModeGetPixelWidth(mode) != desc->pixel_width ||
            CGDisplayModeGetPixelHeight(mode) != desc->pixel_height)
            return FALSE;
    }
    else
#endif
    if (CGDisplayModeGetWidth(mode) != desc->pixel_width ||
        CGDisplayModeGetHeight(mode) != desc->pixel_height)
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


static int get_default_bpp(void)
{
    int ret;

    if (!default_mode_bpp)
    {
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(kCGDirectMainDisplay);
        if (mode)
        {
            default_mode_bpp = display_mode_bits_per_pixel(mode);
            CFRelease(mode);
        }

        if (!default_mode_bpp)
            default_mode_bpp = 32;
    }

    ret = default_mode_bpp;

    TRACE(" -> %d\n", ret);
    return ret;
}


static BOOL display_mode_is_supported(CGDisplayModeRef display_mode)
{
    uint32_t io_flags = CGDisplayModeGetIOFlags(display_mode);
    return (io_flags & kDisplayModeValidFlag) && (io_flags & kDisplayModeSafeFlag);
}


#if defined(MAC_OS_X_VERSION_10_8) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
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

    io_flags &= kDisplayModeValidFlag | kDisplayModeSafeFlag | kDisplayModeInterlacedFlag |
                kDisplayModeStretchedFlag | kDisplayModeTelevisionFlag;
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


static BOOL mode_is_preferred(CGDisplayModeRef new_mode, CGDisplayModeRef old_mode,
                              struct display_mode_descriptor *original_mode_desc,
                              BOOL include_unsupported)
{
    BOOL new_is_supported;
    CFStringRef pixel_encoding;
    size_t width_points, height_points;
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
#endif


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

#if defined(MAC_OS_X_VERSION_10_8) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    if (&CGDisplayModeGetPixelWidth != NULL && &CGDisplayModeGetPixelHeight != NULL)
    {
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
    }
    else
#endif
        modes = CGDisplayCopyAllDisplayModes(display, NULL);

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

static BOOL get_primary_adapter(WCHAR *name)
{
    DISPLAY_DEVICEW dd;
    DWORD i;

    dd.cb = sizeof(dd);
    for (i = 0; !NtUserEnumDisplayDevices(NULL, i, &dd, 0); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

/***********************************************************************
 *              ChangeDisplaySettingsEx  (MACDRV.@)
 *
 */
LONG macdrv_ChangeDisplaySettingsEx(LPCWSTR devname, LPDEVMODEW devmode,
                                    HWND hwnd, DWORD flags, LPVOID lpvoid)
{
    WCHAR primary_adapter[CCHDEVICENAME];
    LONG ret = DISP_CHANGE_BADMODE;
    DEVMODEW default_mode;
    int bpp;
    struct macdrv_display *displays;
    int num_displays;
    CFArrayRef display_modes;
    struct display_mode_descriptor* desc;
    CFIndex count, i, best;
    CGDisplayModeRef best_display_mode;
    uint32_t best_io_flags;
    BOOL best_is_original;

    TRACE("%s %p %p 0x%08x %p\n", debugstr_w(devname), devmode, hwnd, flags, lpvoid);

    init_original_display_mode();

    if (!get_primary_adapter(primary_adapter))
        return DISP_CHANGE_FAILED;

    if (!devname && !devmode)
    {
        UNICODE_STRING str;
        memset(&default_mode, 0, sizeof(default_mode));
        default_mode.dmSize = sizeof(default_mode);
        RtlInitUnicodeString(&str, primary_adapter);
        if (!NtUserEnumDisplaySettings(&str, ENUM_REGISTRY_SETTINGS, &default_mode, 0))
        {
            ERR("Default mode not found for %s!\n", wine_dbgstr_w(primary_adapter));
            return DISP_CHANGE_BADMODE;
        }

        devname = primary_adapter;
        devmode = &default_mode;
    }

    if (is_detached_mode(devmode))
    {
        FIXME("Detaching adapters is currently unsupported.\n");
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (macdrv_get_displays(&displays, &num_displays))
        return DISP_CHANGE_FAILED;

    display_modes = copy_display_modes(displays[0].displayID, FALSE);
    if (!display_modes)
    {
        macdrv_free_displays(displays);
        return DISP_CHANGE_FAILED;
    }

    pthread_mutex_lock(&modes_mutex);
    bpp = get_default_bpp();
    pthread_mutex_unlock(&modes_mutex);
    if ((devmode->dmFields & DM_BITSPERPEL) && devmode->dmBitsPerPel != bpp)
        TRACE("using default %d bpp instead of caller's request %d bpp\n", bpp, devmode->dmBitsPerPel);

    TRACE("looking for %dx%dx%dbpp @%d Hz",
          (devmode->dmFields & DM_PELSWIDTH ? devmode->dmPelsWidth : 0),
          (devmode->dmFields & DM_PELSHEIGHT ? devmode->dmPelsHeight : 0),
          bpp,
          (devmode->dmFields & DM_DISPLAYFREQUENCY ? devmode->dmDisplayFrequency : 0));
    if (devmode->dmFields & DM_DISPLAYFIXEDOUTPUT)
        TRACE(" %sstretched", devmode->dmDisplayFixedOutput == DMDFO_STRETCH ? "" : "un");
    if (devmode->dmFields & DM_DISPLAYFLAGS)
        TRACE(" %sinterlaced", devmode->dmDisplayFlags & DM_INTERLACED ? "" : "non-");
    TRACE("\n");

    desc = create_original_display_mode_descriptor(displays[0].displayID);

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

        if (is_original && retina_enabled)
        {
            width *= 2;
            height *= 2;
        }

        if (bpp != mode_bpp)
            continue;

        if (devmode->dmFields & DM_PELSWIDTH)
        {
            if (devmode->dmPelsWidth != width)
                continue;
        }
        if (devmode->dmFields & DM_PELSHEIGHT)
        {
            if (devmode->dmPelsHeight != height)
                continue;
        }
        if ((devmode->dmFields & DM_DISPLAYFREQUENCY) &&
            devmode->dmDisplayFrequency != 0 &&
            devmode->dmDisplayFrequency != 1)
        {
            double refresh_rate = CGDisplayModeGetRefreshRate(display_mode);
            if (!refresh_rate)
                refresh_rate = 60;
            if (devmode->dmDisplayFrequency != (DWORD)refresh_rate)
                continue;
        }
        if (devmode->dmFields & DM_DISPLAYFLAGS)
        {
            if (!(devmode->dmDisplayFlags & DM_INTERLACED) != !(io_flags & kDisplayModeInterlacedFlag))
                continue;
        }
        else if (best_display_mode)
        {
            if (io_flags & kDisplayModeInterlacedFlag && !(best_io_flags & kDisplayModeInterlacedFlag))
                continue;
            else if (!(io_flags & kDisplayModeInterlacedFlag) && best_io_flags & kDisplayModeInterlacedFlag)
                goto better;
        }
        if (devmode->dmFields & DM_DISPLAYFIXEDOUTPUT)
        {
            if (!(devmode->dmDisplayFixedOutput == DMDFO_STRETCH) != !(io_flags & kDisplayModeStretchedFlag))
                continue;
        }
        else if (best_display_mode)
        {
            if (io_flags & kDisplayModeStretchedFlag && !(best_io_flags & kDisplayModeStretchedFlag))
                continue;
            else if (!(io_flags & kDisplayModeStretchedFlag) && best_io_flags & kDisplayModeStretchedFlag)
                goto better;
        }

        if (best_display_mode)
            continue;

better:
        best_display_mode = display_mode;
        best = i;
        best_io_flags = io_flags;
        best_is_original = is_original;
    }

    if (best_display_mode)
    {
        /* we have a valid mode */
        TRACE("Requested display settings match mode %ld\n", best);

        if (flags & (CDS_TEST | CDS_NORESET)) ret = DISP_CHANGE_SUCCESSFUL;
        else if (wcsicmp(primary_adapter, devname))
        {
            FIXME("Changing non-primary adapter settings is currently unsupported.\n");
            ret = DISP_CHANGE_SUCCESSFUL;
        }
        else if (macdrv_set_display_mode(&displays[0], best_display_mode))
        {
            int mode_bpp = display_mode_bits_per_pixel(best_display_mode);
            size_t width = CGDisplayModeGetWidth(best_display_mode);
            size_t height = CGDisplayModeGetHeight(best_display_mode);

            macdrv_init_display_devices(TRUE);

            if (best_is_original && retina_enabled)
            {
                width *= 2;
                height *= 2;
            }

            send_message(NtUserGetDesktopWindow(), WM_MACDRV_UPDATE_DESKTOP_RECT, mode_bpp,
                         MAKELPARAM(width, height));
            ret = DISP_CHANGE_SUCCESSFUL;
        }
        else
        {
            WARN("Failed to set display mode\n");
            ret = DISP_CHANGE_FAILED;
        }
    }
    else
    {
        /* no valid modes found */
        ERR("No matching mode found %ux%ux%d @%u!\n", devmode->dmPelsWidth, devmode->dmPelsHeight,
            bpp, devmode->dmDisplayFrequency);
    }

    free_display_mode_descriptor(desc);
    CFRelease(display_modes);
    macdrv_free_displays(displays);

    return ret;
}

/***********************************************************************
 *              EnumDisplaySettingsEx  (MACDRV.@)
 *
 */
BOOL macdrv_EnumDisplaySettingsEx(LPCWSTR devname, DWORD mode, DEVMODEW *devmode, DWORD flags)
{
    struct macdrv_display *displays = NULL;
    int num_displays;
    CGDisplayModeRef display_mode;
    int display_mode_bpp;
    BOOL synthesized = FALSE;
    double rotation;
    uint32_t io_flags;

    TRACE("%s, %u, %p + %hu, %08x\n", debugstr_w(devname), mode, devmode, devmode->dmSize, flags);

    init_original_display_mode();

    if (macdrv_get_displays(&displays, &num_displays))
        goto failed;

    if (mode == ENUM_CURRENT_SETTINGS)
    {
        TRACE("mode %d (current) -- getting current mode\n", mode);
        display_mode = CGDisplayCopyDisplayMode(displays[0].displayID);
        display_mode_bpp = display_mode_bits_per_pixel(display_mode);
    }
    else
    {
        DWORD count, i;

        pthread_mutex_lock(&modes_mutex);

        if (mode == 0 || !modes)
        {
            if (modes) CFRelease(modes);
            modes = copy_display_modes(displays[0].displayID, (flags & EDS_RAWMODE) != 0);
            modes_has_8bpp = modes_has_16bpp = FALSE;

            if (modes)
            {
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
            }
        }

        display_mode = NULL;
        if (modes)
        {
            int default_bpp;
            DWORD seen_modes = 0;

            count = CFArrayGetCount(modes);
            for (i = 0; i < count; i++)
            {
                CGDisplayModeRef candidate = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

                seen_modes++;
                if (seen_modes > mode)
                {
                    display_mode = (CGDisplayModeRef)CFRetain(candidate);
                    display_mode_bpp = display_mode_bits_per_pixel(display_mode);
                    break;
                }
            }

            default_bpp = get_default_bpp();

            /* If all the real modes are exhausted, synthesize lower bpp modes. */
            if (!display_mode && (!modes_has_16bpp || !modes_has_8bpp))
            {
                /* We want to synthesize higher depths first. */
                int synth_bpps[] = { modes_has_16bpp ? 0 : 16, modes_has_8bpp ? 0 : 8 };
                size_t synth_bpp_idx;
                for (synth_bpp_idx = 0; synth_bpp_idx < 2; synth_bpp_idx++)
                {
                    int synth_bpp = synth_bpps[synth_bpp_idx];
                    if (synth_bpp == 0)
                        continue;

                    for (i = 0; i < count; i++)
                    {
                        CGDisplayModeRef candidate = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
                        /* We only synthesize modes from those having the default bpp. */
                        if (display_mode_bits_per_pixel(candidate) != default_bpp)
                            continue;

                        seen_modes++;
                        if (seen_modes > mode)
                        {
                            display_mode = (CGDisplayModeRef)CFRetain(candidate);
                            display_mode_bpp = synth_bpp;
                            synthesized = TRUE;
                            break;
                        }
                    }

                    if (display_mode)
                        break;
                }
            }
        }

        pthread_mutex_unlock(&modes_mutex);
    }

    if (!display_mode)
        goto failed;

    /* We currently only report modes for the primary display, so it's at (0, 0). */
    devmode->dmPosition.x = 0;
    devmode->dmPosition.y = 0;
    devmode->dmFields |= DM_POSITION;

    rotation = CGDisplayRotation(displays[0].displayID);
    devmode->dmDisplayOrientation = ((int)((rotation / 90) + 0.5)) % 4;
    devmode->dmFields |= DM_DISPLAYORIENTATION;

    io_flags = CGDisplayModeGetIOFlags(display_mode);
    if (io_flags & kDisplayModeStretchedFlag)
        devmode->dmDisplayFixedOutput = DMDFO_STRETCH;
    else
        devmode->dmDisplayFixedOutput = DMDFO_CENTER;
    devmode->dmFields |= DM_DISPLAYFIXEDOUTPUT;

    devmode->dmBitsPerPel = display_mode_bpp;
    if (devmode->dmBitsPerPel)
        devmode->dmFields |= DM_BITSPERPEL;

    devmode->dmPelsWidth = CGDisplayModeGetWidth(display_mode);
    devmode->dmPelsHeight = CGDisplayModeGetHeight(display_mode);
    if (retina_enabled)
    {
        struct display_mode_descriptor* desc = create_original_display_mode_descriptor(displays[0].displayID);
        if (display_mode_matches_descriptor(display_mode, desc))
        {
            devmode->dmPelsWidth *= 2;
            devmode->dmPelsHeight *= 2;
        }
        free_display_mode_descriptor(desc);
    }
    devmode->dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT;

    devmode->dmDisplayFlags = 0;
    if (io_flags & kDisplayModeInterlacedFlag)
        devmode->dmDisplayFlags |= DM_INTERLACED;
    devmode->dmFields |= DM_DISPLAYFLAGS;

    devmode->dmDisplayFrequency = CGDisplayModeGetRefreshRate(display_mode);
    if (!devmode->dmDisplayFrequency)
        devmode->dmDisplayFrequency = 60;
    devmode->dmFields |= DM_DISPLAYFREQUENCY;

    CFRelease(display_mode);
    macdrv_free_displays(displays);

    TRACE("mode %d -- %dx%dx%dbpp @%d Hz", mode,
          devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel,
          devmode->dmDisplayFrequency);
    if (devmode->dmDisplayOrientation)
        TRACE(" rotated %u degrees", devmode->dmDisplayOrientation * 90);
    if (devmode->dmDisplayFixedOutput == DMDFO_STRETCH)
        TRACE(" stretched");
    if (devmode->dmDisplayFlags & DM_INTERLACED)
        TRACE(" interlaced");
    if (synthesized)
        TRACE(" (synthesized)");
    TRACE("\n");

    return TRUE;

failed:
    TRACE("mode %d -- not present\n", mode);
    if (displays) macdrv_free_displays(displays);
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
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
 *              init_registry_display_settings
 *
 * Initialize registry display settings when new display devices are added.
 */
static void init_registry_display_settings(void)
{
    DEVMODEW dm = {.dmSize = sizeof(dm)};
    DISPLAY_DEVICEW dd = {sizeof(dd)};
    UNICODE_STRING str;
    DWORD i = 0;
    LONG ret;

    while (!NtUserEnumDisplayDevices(NULL, i++, &dd, 0))
    {
        RtlInitUnicodeString(&str, dd.DeviceName);

        /* Skip if the device already has registry display settings */
        if (NtUserEnumDisplaySettings(&str, ENUM_REGISTRY_SETTINGS, &dm, 0))
            continue;

        if (!NtUserEnumDisplaySettings(&str, ENUM_CURRENT_SETTINGS, &dm, 0))
        {
            ERR("Failed to query current display settings for %s.\n", wine_dbgstr_w(dd.DeviceName));
            continue;
        }

        TRACE("Device %s current display mode %ux%u %ubits %uHz at %d,%d.\n",
              wine_dbgstr_w(dd.DeviceName), dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              dm.dmDisplayFrequency, dm.dmPosition.x, dm.dmPosition.y);

        ret = NtUserChangeDisplaySettings(&str, &dm, NULL,
                                          CDS_GLOBAL | CDS_NORESET | CDS_UPDATEREGISTRY, NULL);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            ERR("Failed to save registry display settings for %s, returned %d.\n",
                wine_dbgstr_w(dd.DeviceName), ret);
    }
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
    {
        CGDirectDisplayID mainDisplay = CGMainDisplayID();
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(mainDisplay);
        size_t width = CGDisplayModeGetWidth(mode);
        size_t height = CGDisplayModeGetHeight(mode);
        int mode_bpp = display_mode_bits_per_pixel(mode);
        struct display_mode_descriptor* desc = create_original_display_mode_descriptor(mainDisplay);
        BOOL is_original = display_mode_matches_descriptor(mode, desc);

        free_display_mode_descriptor(desc);
        CGDisplayModeRelease(mode);

        macdrv_init_display_devices(TRUE);
        init_registry_display_settings();

        if (is_original && retina_enabled)
        {
            width *= 2;
            height *= 2;
        }

        send_message(hwnd, WM_MACDRV_UPDATE_DESKTOP_RECT, mode_bpp,
                     MAKELPARAM(width, height));
    }
}

static BOOL force_display_devices_refresh;

BOOL macdrv_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, BOOL force, void *param )
{
    struct macdrv_adapter *adapters, *adapter;
    struct macdrv_monitor *monitors, *monitor;
    struct macdrv_gpu *gpus, *gpu;
    INT gpu_count, adapter_count, monitor_count;
    DWORD len;

    if (!force && !force_display_devices_refresh) return TRUE;
    force_display_devices_refresh = FALSE;

    /* Initialize GPUs */
    if (macdrv_get_gpus(&gpus, &gpu_count))
    {
        ERR("could not get GPUs\n");
        return FALSE;
    }
    TRACE("GPU count: %d\n", gpu_count);

    for (gpu = gpus; gpu < gpus + gpu_count; gpu++)
    {
        struct gdi_gpu gdi_gpu =
        {
            .id = gpu->id,
            .vendor_id = gpu->vendor_id,
            .device_id = gpu->device_id,
            .subsys_id = gpu->subsys_id,
            .revision_id = gpu->revision_id,
        };
        RtlUTF8ToUnicodeN(gdi_gpu.name, sizeof(gdi_gpu.name), &len, gpu->name, strlen(gpu->name));
        device_manager->add_gpu(&gdi_gpu, param);

        /* Initialize adapters */
        if (macdrv_get_adapters(gpu->id, &adapters, &adapter_count)) break;
        TRACE("GPU: %llx %s, adapter count: %d\n", gpu->id, debugstr_a(gpu->name), adapter_count);

        for (adapter = adapters; adapter < adapters + adapter_count; adapter++)
        {
            struct gdi_adapter gdi_adapter =
            {
                .id = adapter->id,
                .state_flags = adapter->state_flags,
            };
            device_manager->add_adapter( &gdi_adapter, param );

            if (macdrv_get_monitors(adapter->id, &monitors, &monitor_count)) break;
            TRACE("adapter: %#x, monitor count: %d\n", adapter->id, monitor_count);

            /* Initialize monitors */
            for (monitor = monitors; monitor < monitors + monitor_count; monitor++)
            {
                struct gdi_monitor gdi_monitor =
                {
                    .rc_monitor = rect_from_cgrect(monitor->rc_monitor),
                    .rc_work = rect_from_cgrect(monitor->rc_work),
                    .state_flags = monitor->state_flags,
                };
                RtlUTF8ToUnicodeN(gdi_monitor.name, sizeof(gdi_monitor.name), &len,
                                  monitor->name, strlen(monitor->name));
                TRACE("monitor: %s\n", debugstr_a(monitor->name));
                device_manager->add_monitor( &gdi_monitor, param );
            }

            macdrv_free_monitors(monitors);
        }

        macdrv_free_adapters(adapters);
    }

    macdrv_free_gpus(gpus);
    return TRUE;
}

/***********************************************************************
 *              macdrv_init_display_devices
 *
 * Initialize display device registry data.
 */
void macdrv_init_display_devices(BOOL force)
{
    UINT32 num_path, num_mode;

    if (force) force_display_devices_refresh = TRUE;
    /* trigger refresh in win32u */
    NtUserGetDisplayConfigBufferSizes( QDC_ONLY_ACTIVE_PATHS, &num_path, &num_mode );
}
