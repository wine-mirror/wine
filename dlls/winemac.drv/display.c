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

#include "config.h"

#include "macdrv.h"
#include "winuser.h"
#include "winreg.h"
#include "ddrawi.h"

WINE_DEFAULT_DEBUG_CHANNEL(display);


BOOL CDECL macdrv_EnumDisplaySettingsEx(LPCWSTR devname, DWORD mode, LPDEVMODEW devmode, DWORD flags);


static CFArrayRef modes;
static BOOL modes_has_8bpp, modes_has_16bpp;
static int default_mode_bpp;
static CRITICAL_SECTION modes_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &modes_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": modes_section") }
};
static CRITICAL_SECTION modes_section = { &critsect_debug, -1, 0, 0, 0, 0 };


static inline HMONITOR display_id_to_monitor(CGDirectDisplayID display_id)
{
    return (HMONITOR)(UINT_PTR)display_id;
}

static inline CGDirectDisplayID monitor_to_display_id(HMONITOR handle)
{
    return (CGDirectDisplayID)(UINT_PTR)handle;
}


static BOOL get_display_device_reg_key(char *key, unsigned len)
{
    static const char display_device_guid_prop[] = "__wine_display_device_guid";
    static const char video_path[] = "System\\CurrentControlSet\\Control\\Video\\{";
    static const char display0[] = "}\\0000";
    ATOM guid_atom;

    assert(len >= sizeof(video_path) + sizeof(display0) + 40);

    guid_atom = HandleToULong(GetPropA(GetDesktopWindow(), display_device_guid_prop));
    if (!guid_atom) return FALSE;

    memcpy(key, video_path, sizeof(video_path));

    if (!GlobalGetAtomNameA(guid_atom, key + strlen(key), 40))
        return FALSE;

    strcat(key, display0);

    TRACE("display device key %s\n", wine_dbgstr_a(key));
    return TRUE;
}


static BOOL read_registry_settings(DEVMODEW *dm)
{
    char wine_mac_reg_key[128];
    HKEY hkey;
    DWORD type, size;
    BOOL ret = TRUE;

    dm->dmFields = 0;

    if (!get_display_device_reg_key(wine_mac_reg_key, sizeof(wine_mac_reg_key)))
        return FALSE;

    if (RegOpenKeyExA(HKEY_CURRENT_CONFIG, wine_mac_reg_key, 0, KEY_READ, &hkey))
        return FALSE;

#define query_value(name, data) \
    size = sizeof(DWORD); \
    if (RegQueryValueExA(hkey, name, 0, &type, (LPBYTE)(data), &size) || \
        type != REG_DWORD || size != sizeof(DWORD)) \
        ret = FALSE

    query_value("DefaultSettings.BitsPerPel", &dm->dmBitsPerPel);
    dm->dmFields |= DM_BITSPERPEL;
    query_value("DefaultSettings.XResolution", &dm->dmPelsWidth);
    dm->dmFields |= DM_PELSWIDTH;
    query_value("DefaultSettings.YResolution", &dm->dmPelsHeight);
    dm->dmFields |= DM_PELSHEIGHT;
    query_value("DefaultSettings.VRefresh", &dm->dmDisplayFrequency);
    dm->dmFields |= DM_DISPLAYFREQUENCY;
    query_value("DefaultSettings.Flags", &dm->dmDisplayFlags);
    dm->dmFields |= DM_DISPLAYFLAGS;
    query_value("DefaultSettings.XPanning", &dm->dmPosition.x);
    query_value("DefaultSettings.YPanning", &dm->dmPosition.y);
    query_value("DefaultSettings.Orientation", &dm->dmDisplayOrientation);
    query_value("DefaultSettings.FixedOutput", &dm->dmDisplayFixedOutput);

#undef query_value

    RegCloseKey(hkey);
    return ret;
}


static BOOL write_registry_settings(const DEVMODEW *dm)
{
    char wine_mac_reg_key[128];
    HKEY hkey;
    BOOL ret = TRUE;

    if (!get_display_device_reg_key(wine_mac_reg_key, sizeof(wine_mac_reg_key)))
        return FALSE;

    if (RegCreateKeyExA(HKEY_CURRENT_CONFIG, wine_mac_reg_key, 0, NULL,
                        REG_OPTION_VOLATILE, KEY_WRITE, NULL, &hkey, NULL))
        return FALSE;

#define set_value(name, data) \
    if (RegSetValueExA(hkey, name, 0, REG_DWORD, (const BYTE*)(data), sizeof(DWORD))) \
        ret = FALSE

    set_value("DefaultSettings.BitsPerPel", &dm->dmBitsPerPel);
    set_value("DefaultSettings.XResolution", &dm->dmPelsWidth);
    set_value("DefaultSettings.YResolution", &dm->dmPelsHeight);
    set_value("DefaultSettings.VRefresh", &dm->dmDisplayFrequency);
    set_value("DefaultSettings.Flags", &dm->dmDisplayFlags);
    set_value("DefaultSettings.XPanning", &dm->dmPosition.x);
    set_value("DefaultSettings.YPanning", &dm->dmPosition.y);
    set_value("DefaultSettings.Orientation", &dm->dmDisplayOrientation);
    set_value("DefaultSettings.FixedOutput", &dm->dmDisplayFixedOutput);

#undef set_value

    RegCloseKey(hkey);
    return ret;
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

    EnterCriticalSection(&modes_section);

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

    LeaveCriticalSection(&modes_section);

    TRACE(" -> %d\n", ret);
    return ret;
}


/***********************************************************************
 *              ChangeDisplaySettingsEx  (MACDRV.@)
 *
 */
LONG CDECL macdrv_ChangeDisplaySettingsEx(LPCWSTR devname, LPDEVMODEW devmode,
                                          HWND hwnd, DWORD flags, LPVOID lpvoid)
{
    LONG ret = DISP_CHANGE_BADMODE;
    int bpp;
    DEVMODEW dm;
    BOOL def_mode = TRUE;
    struct macdrv_display *displays;
    int num_displays;
    CFArrayRef display_modes;
    CFIndex count, i, safe;

    TRACE("%s %p %p 0x%08x %p\n", debugstr_w(devname), devmode, hwnd, flags, lpvoid);

    if (devmode)
    {
        /* this is the minimal dmSize that XP accepts */
        if (devmode->dmSize < FIELD_OFFSET(DEVMODEW, dmFields))
            return DISP_CHANGE_FAILED;

        if (devmode->dmSize >= FIELD_OFFSET(DEVMODEW, dmFields) + sizeof(devmode->dmFields))
        {
            if (((devmode->dmFields & DM_BITSPERPEL) && devmode->dmBitsPerPel) ||
                ((devmode->dmFields & DM_PELSWIDTH) && devmode->dmPelsWidth) ||
                ((devmode->dmFields & DM_PELSHEIGHT) && devmode->dmPelsHeight) ||
                ((devmode->dmFields & DM_DISPLAYFREQUENCY) && devmode->dmDisplayFrequency))
                def_mode = FALSE;
        }
    }

    if (def_mode)
    {
        if (!macdrv_EnumDisplaySettingsEx(devname, ENUM_REGISTRY_SETTINGS, &dm, 0))
        {
            ERR("Default mode not found!\n");
            return DISP_CHANGE_BADMODE;
        }

        TRACE("Return to original display mode\n");
        devmode = &dm;
    }

    if ((devmode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN("devmode doesn't specify the resolution: %04x\n", devmode->dmFields);
        return DISP_CHANGE_BADMODE;
    }

    if (macdrv_get_displays(&displays, &num_displays))
        return DISP_CHANGE_FAILED;

    display_modes = CGDisplayCopyAllDisplayModes(displays[0].displayID, NULL);
    if (!display_modes)
    {
        macdrv_free_displays(displays);
        return DISP_CHANGE_FAILED;
    }

    bpp = get_default_bpp();
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

    safe = -1;
    count = CFArrayGetCount(display_modes);
    for (i = 0; i < count; i++)
    {
        CGDisplayModeRef display_mode = (CGDisplayModeRef)CFArrayGetValueAtIndex(display_modes, i);
        uint32_t io_flags = CGDisplayModeGetIOFlags(display_mode);
        int mode_bpp = display_mode_bits_per_pixel(display_mode);
        size_t width = CGDisplayModeGetWidth(display_mode);
        size_t height = CGDisplayModeGetHeight(display_mode);

        if (!(io_flags & kDisplayModeValidFlag) || !(io_flags & kDisplayModeSafeFlag))
            continue;

        safe++;

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
        if ((devmode->dmFields & DM_DISPLAYFREQUENCY) && devmode->dmDisplayFrequency != 0)
        {
            double refresh_rate = CGDisplayModeGetRefreshRate(display_mode);
            if (!refresh_rate)
                refresh_rate = 60;
            if (devmode->dmDisplayFrequency != (DWORD)refresh_rate)
                continue;
        }
        if (devmode->dmFields & DM_DISPLAYFIXEDOUTPUT)
        {
            if (!(devmode->dmDisplayFixedOutput == DMDFO_STRETCH) != !(io_flags & kDisplayModeStretchedFlag))
                continue;
        }
        if (devmode->dmFields & DM_DISPLAYFLAGS)
        {
            if (!(devmode->dmDisplayFlags & DM_INTERLACED) != !(io_flags & kDisplayModeInterlacedFlag))
                continue;
        }

        /* we have a valid mode */
        TRACE("Requested display settings match mode %ld\n", safe);

        if ((flags & CDS_UPDATEREGISTRY) && !write_registry_settings(devmode))
        {
            WARN("Failed to update registry\n");
            ret = DISP_CHANGE_NOTUPDATED;
            break;
        }

        if (flags & (CDS_TEST | CDS_NORESET))
            ret = DISP_CHANGE_SUCCESSFUL;
        else
        {
            if (macdrv_set_display_mode(&displays[0], display_mode))
            {
                SendMessageW(GetDesktopWindow(), WM_MACDRV_UPDATE_DESKTOP_RECT, mode_bpp,
                             MAKELPARAM(width, height));
                ret = DISP_CHANGE_SUCCESSFUL;
            }
            else
            {
                WARN("Failed to set display mode\n");
                ret = DISP_CHANGE_FAILED;
            }
        }

        break;
    }

    CFRelease(display_modes);
    macdrv_free_displays(displays);

    if (i >= count)
    {
        /* no valid modes found */
        ERR("No matching mode found %ux%ux%d @%u!\n", devmode->dmPelsWidth, devmode->dmPelsHeight,
            bpp, devmode->dmDisplayFrequency);
    }

    return ret;
}


/***********************************************************************
 *              EnumDisplayMonitors  (MACDRV.@)
 */
BOOL CDECL macdrv_EnumDisplayMonitors(HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lparam)
{
    struct macdrv_display *displays;
    int num_displays;
    int i;
    BOOL ret = TRUE;

    TRACE("%p, %s, %p, %#lx\n", hdc, wine_dbgstr_rect(rect), proc, lparam);

    if (hdc)
    {
        POINT origin;
        RECT limit;

        if (!GetDCOrgEx(hdc, &origin)) return FALSE;
        if (GetClipBox(hdc, &limit) == ERROR) return FALSE;

        if (rect && !IntersectRect(&limit, &limit, rect)) return TRUE;

        if (macdrv_get_displays(&displays, &num_displays))
            return FALSE;

        for (i = 0; i < num_displays; i++)
        {
            RECT monrect = rect_from_cgrect(displays[i].frame);
            OffsetRect(&monrect, -origin.x, -origin.y);
            if (IntersectRect(&monrect, &monrect, &limit))
            {
                HMONITOR monitor = display_id_to_monitor(displays[i].displayID);
                TRACE("monitor %d handle %p @ %s\n", i, monitor, wine_dbgstr_rect(&monrect));
                if (!proc(monitor, hdc, &monrect, lparam))
                {
                    ret = FALSE;
                    break;
                }
            }
        }
    }
    else
    {
        if (macdrv_get_displays(&displays, &num_displays))
            return FALSE;

        for (i = 0; i < num_displays; i++)
        {
            RECT monrect = rect_from_cgrect(displays[i].frame);
            RECT unused;
            if (!rect || IntersectRect(&unused, &monrect, rect))
            {
                HMONITOR monitor = display_id_to_monitor(displays[i].displayID);
                TRACE("monitor %d handle %p @ %s\n", i, monitor, wine_dbgstr_rect(&monrect));
                if (!proc(monitor, 0, &monrect, lparam))
                {
                    ret = FALSE;
                    break;
                }
            }
        }
    }

    macdrv_free_displays(displays);

    return ret;
}


/***********************************************************************
 *              EnumDisplaySettingsEx  (MACDRV.@)
 *
 */
BOOL CDECL macdrv_EnumDisplaySettingsEx(LPCWSTR devname, DWORD mode,
                                        LPDEVMODEW devmode, DWORD flags)
{
    static const WCHAR dev_name[CCHDEVICENAME] =
        { 'W','i','n','e',' ','M','a','c',' ','d','r','i','v','e','r',0 };
    struct macdrv_display *displays = NULL;
    int num_displays;
    CGDisplayModeRef display_mode;
    int display_mode_bpp;
    BOOL synthesized = FALSE;
    double rotation;
    uint32_t io_flags;

    TRACE("%s, %u, %p + %hu, %08x\n", debugstr_w(devname), mode, devmode, devmode->dmSize, flags);

    memcpy(devmode->dmDeviceName, dev_name, sizeof(dev_name));
    devmode->dmSpecVersion = DM_SPECVERSION;
    devmode->dmDriverVersion = DM_SPECVERSION;
    devmode->dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    devmode->dmDriverExtra = 0;
    memset(&devmode->dmFields, 0, devmode->dmSize - FIELD_OFFSET(DEVMODEW, dmFields));

    if (mode == ENUM_REGISTRY_SETTINGS)
    {
        TRACE("mode %d (registry) -- getting default mode\n", mode);
        return read_registry_settings(devmode);
    }

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

        EnterCriticalSection(&modes_section);

        if (mode == 0 || !modes)
        {
            if (modes) CFRelease(modes);
            modes = CGDisplayCopyAllDisplayModes(displays[0].displayID, NULL);
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
            int default_bpp = get_default_bpp();
            DWORD seen_modes = 0;

            count = CFArrayGetCount(modes);
            for (i = 0; i < count; i++)
            {
                CGDisplayModeRef candidate = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);

                io_flags = CGDisplayModeGetIOFlags(candidate);
                if (!(flags & EDS_RAWMODE) &&
                    (!(io_flags & kDisplayModeValidFlag) || !(io_flags & kDisplayModeSafeFlag)))
                    continue;

                seen_modes++;
                if (seen_modes > mode)
                {
                    display_mode = (CGDisplayModeRef)CFRetain(candidate);
                    display_mode_bpp = display_mode_bits_per_pixel(display_mode);
                    break;
                }

                /* We only synthesize modes from those having the default bpp. */
                if (display_mode_bits_per_pixel(candidate) != default_bpp)
                    continue;

                if (!modes_has_8bpp)
                {
                    seen_modes++;
                    if (seen_modes > mode)
                    {
                        display_mode = (CGDisplayModeRef)CFRetain(candidate);
                        display_mode_bpp = 8;
                        synthesized = TRUE;
                        break;
                    }
                }

                if (!modes_has_16bpp)
                {
                    seen_modes++;
                    if (seen_modes > mode)
                    {
                        display_mode = (CGDisplayModeRef)CFRetain(candidate);
                        display_mode_bpp = 16;
                        synthesized = TRUE;
                        break;
                    }
                }
            }
        }

        LeaveCriticalSection(&modes_section);
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
    int win_entries = sizeof(r->red) / sizeof(r->red[0]);
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
    red = HeapAlloc(GetProcessHeap(), 0, mac_entries * sizeof(red[0]) * 3);
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
    HeapFree(GetProcessHeap(), 0, red);
    macdrv_free_displays(displays);
    return ret;
}


/***********************************************************************
 *              GetMonitorInfo  (MACDRV.@)
 */
BOOL CDECL macdrv_GetMonitorInfo(HMONITOR monitor, LPMONITORINFO info)
{
    static const WCHAR adapter_name[] = { '\\','\\','.','\\','D','I','S','P','L','A','Y','1',0 };
    struct macdrv_display *displays;
    int num_displays;
    CGDirectDisplayID display_id;
    int i;

    TRACE("%p, %p\n", monitor, info);

    if (macdrv_get_displays(&displays, &num_displays))
    {
        ERR("couldn't get display list\n");
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }

    display_id = monitor_to_display_id(monitor);
    for (i = 0; i < num_displays; i++)
    {
        if (displays[i].displayID == display_id)
            break;
    }

    if (i < num_displays)
    {
        info->rcMonitor = rect_from_cgrect(displays[i].frame);
        info->rcWork    = rect_from_cgrect(displays[i].work_frame);

        info->dwFlags = (i == 0) ? MONITORINFOF_PRIMARY : 0;

        if (info->cbSize >= sizeof(MONITORINFOEXW))
            lstrcpyW(((MONITORINFOEXW*)info)->szDevice, adapter_name);

        TRACE(" -> rcMonitor %s rcWork %s dwFlags %08x\n", wine_dbgstr_rect(&info->rcMonitor),
              wine_dbgstr_rect(&info->rcWork), info->dwFlags);
    }
    else
    {
        ERR("invalid monitor handle\n");
        SetLastError(ERROR_INVALID_HANDLE);
    }

    macdrv_free_displays(displays);
    return (i < num_displays);
}


/***********************************************************************
 *              SetDeviceGammaRamp (MACDRV.@)
 */
BOOL macdrv_SetDeviceGammaRamp(PHYSDEV dev, LPVOID ramp)
{
    DDGAMMARAMP *r = ramp;
    struct macdrv_display *displays;
    int num_displays;
    int win_entries = sizeof(r->red) / sizeof(r->red[0]);
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

    red = HeapAlloc(GetProcessHeap(), 0, win_entries * sizeof(red[0]) * 3);
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
    HeapFree(GetProcessHeap(), 0, red);
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
    HWND hwnd = GetDesktopWindow();

    /* A system display change will get delivered to all GUI-attached threads,
       so the desktop-window-owning thread will get it and all others should
       ignore it.  A synthesized display change event due to activation
       will only get delivered to the activated process.  So, it needs to
       process it (by sending it to the desktop window). */
    if (event->displays_changed.activating ||
        GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
    {
        CGDirectDisplayID mainDisplay = CGMainDisplayID();
        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(mainDisplay);
        size_t width = CGDisplayModeGetWidth(mode);
        size_t height = CGDisplayModeGetHeight(mode);
        int mode_bpp = display_mode_bits_per_pixel(mode);

        CGDisplayModeRelease(mode);
        SendMessageW(hwnd, WM_MACDRV_UPDATE_DESKTOP_RECT, mode_bpp,
                     MAKELPARAM(width, height));
    }
}
