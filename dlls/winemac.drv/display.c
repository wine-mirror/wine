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

WINE_DEFAULT_DEBUG_CHANNEL(display);


static CFArrayRef modes;
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
    }
    else
    {
        EnterCriticalSection(&modes_section);

        if (mode == 0 || !modes)
        {
            if (modes) CFRelease(modes);
            modes = CGDisplayCopyAllDisplayModes(displays[0].displayID, NULL);
        }

        display_mode = NULL;
        if (modes)
        {
            if (flags & EDS_RAWMODE)
            {
                if (mode < CFArrayGetCount(modes))
                    display_mode = (CGDisplayModeRef)CFRetain(CFArrayGetValueAtIndex(modes, mode));
            }
            else
            {
                DWORD count, i, safe_modes = 0;
                count = CFArrayGetCount(modes);
                for (i = 0; i < count; i++)
                {
                    CGDisplayModeRef candidate = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, i);
                    io_flags = CGDisplayModeGetIOFlags(candidate);
                    if ((io_flags & kDisplayModeValidFlag) &&
                        (io_flags & kDisplayModeSafeFlag))
                    {
                        safe_modes++;
                        if (safe_modes > mode)
                        {
                            display_mode = (CGDisplayModeRef)CFRetain(candidate);
                            break;
                        }
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

    devmode->dmBitsPerPel = display_mode_bits_per_pixel(display_mode);
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
    TRACE("\n");

    return TRUE;

failed:
    TRACE("mode %d -- not present\n", mode);
    if (displays) macdrv_free_displays(displays);
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
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
 *              macdrv_displays_changed
 *
 * Handler for DISPLAYS_CHANGED events.
 */
void macdrv_displays_changed(const macdrv_event *event)
{
    HWND hwnd = GetDesktopWindow();

    /* A system display change will get delivered to all GUI-attached threads,
       so the desktop-window-owning thread will get it and all others should
       ignore it. */
    if (GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
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
