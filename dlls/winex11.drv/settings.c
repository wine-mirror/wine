/*
 * Wine X11drv display settings functions
 *
 * Copyright 2003 Alexander James Pasadyn
 * Copyright 2020 Zhiyi Zhang for CodeWeavers
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
#include <stdlib.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "x11drv.h"

#include "windef.h"
#include "winreg.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11settings);

struct x11drv_display_setting
{
    ULONG_PTR id;
    BOOL placed;
    RECT new_rect;
    RECT desired_rect;
    DEVMODEW desired_mode;
};

struct x11drv_display_depth
{
    struct list entry;
    ULONG_PTR display_id;
    DWORD depth;
};

/* Display device emulated depth list, protected by modes_section */
static struct list x11drv_display_depth_list = LIST_INIT(x11drv_display_depth_list);

/* All Windows drivers seen so far either support 32 bit depths, or 24 bit depths, but never both. So if we have
 * a 32 bit framebuffer, report 32 bit bpps, otherwise 24 bit ones.
 */
static const unsigned int depths_24[]  = {8, 16, 24};
static const unsigned int depths_32[]  = {8, 16, 32};
const unsigned int *depths;

static struct x11drv_settings_handler handler;

/* Cached display modes for a device, protected by modes_section */
static WCHAR cached_device_name[CCHDEVICENAME];
static DWORD cached_flags;
static DEVMODEW *cached_modes;
static UINT cached_mode_count;

static pthread_mutex_t settings_mutex = PTHREAD_MUTEX_INITIALIZER;

void X11DRV_Settings_SetHandler(const struct x11drv_settings_handler *new_handler)
{
    if (new_handler->priority > handler.priority)
    {
        handler = *new_handler;
        TRACE("Display settings are now handled by: %s.\n", handler.name);
    }
}

/***********************************************************************
 * Default handlers if resolution switching is not enabled
 *
 */
static BOOL nores_get_id(const WCHAR *device_name, ULONG_PTR *id)
{
    WCHAR primary_adapter[CCHDEVICENAME];

    if (!get_primary_adapter( primary_adapter ))
        return FALSE;

    *id = !lstrcmpiW( device_name, primary_adapter ) ? 1 : 0;
    return TRUE;
}

static BOOL nores_get_modes(ULONG_PTR id, DWORD flags, DEVMODEW **new_modes, UINT *mode_count)
{
    RECT primary = get_host_primary_monitor_rect();
    DEVMODEW *modes;

    modes = heap_calloc(1, sizeof(*modes));
    if (!modes)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    modes[0].dmSize = sizeof(*modes);
    modes[0].dmDriverExtra = 0;
    modes[0].dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                        DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    modes[0].u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    modes[0].dmBitsPerPel = screen_bpp;
    modes[0].dmPelsWidth = primary.right;
    modes[0].dmPelsHeight = primary.bottom;
    modes[0].u2.dmDisplayFlags = 0;
    modes[0].dmDisplayFrequency = 60;

    *new_modes = modes;
    *mode_count = 1;
    return TRUE;
}

static void nores_free_modes(DEVMODEW *modes)
{
    heap_free(modes);
}

static BOOL nores_get_current_mode(ULONG_PTR id, DEVMODEW *mode)
{
    RECT primary = get_host_primary_monitor_rect();

    mode->dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT |
                     DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    mode->u1.s2.dmDisplayOrientation = DMDO_DEFAULT;
    mode->u2.dmDisplayFlags = 0;
    mode->u1.s2.dmPosition.x = 0;
    mode->u1.s2.dmPosition.y = 0;

    if (id != 1)
    {
        FIXME("Non-primary adapters are unsupported.\n");
        mode->dmBitsPerPel = 0;
        mode->dmPelsWidth = 0;
        mode->dmPelsHeight = 0;
        mode->dmDisplayFrequency = 0;
        return TRUE;
    }

    mode->dmBitsPerPel = screen_bpp;
    mode->dmPelsWidth = primary.right;
    mode->dmPelsHeight = primary.bottom;
    mode->dmDisplayFrequency = 60;
    return TRUE;
}

static LONG nores_set_current_mode(ULONG_PTR id, DEVMODEW *mode)
{
    WARN("NoRes settings handler, ignoring mode change request.\n");
    return DISP_CHANGE_SUCCESSFUL;
}

/* default handler only gets the current X desktop resolution */
void X11DRV_Settings_Init(void)
{
    struct x11drv_settings_handler nores_handler;

    depths = screen_bpp == 32 ? depths_32 : depths_24;

    nores_handler.name = "NoRes";
    nores_handler.priority = 1;
    nores_handler.get_id = nores_get_id;
    nores_handler.get_modes = nores_get_modes;
    nores_handler.free_modes = nores_free_modes;
    nores_handler.get_current_mode = nores_get_current_mode;
    nores_handler.set_current_mode = nores_set_current_mode;
    X11DRV_Settings_SetHandler(&nores_handler);
}

/* Initialize registry display settings when new display devices are added */
void init_registry_display_settings(void)
{
    DEVMODEW dm = {.dmSize = sizeof(dm)};
    DISPLAY_DEVICEW dd = {sizeof(dd)};
    UNICODE_STRING device_name;
    DWORD i = 0;
    LONG ret;

    while (!NtUserEnumDisplayDevices( NULL, i++, &dd, 0 ))
    {
        RtlInitUnicodeString( &device_name, dd.DeviceName );

        /* Skip if the device already has registry display settings */
        if (NtUserEnumDisplaySettings( &device_name, ENUM_REGISTRY_SETTINGS, &dm, 0 ))
            continue;

        if (!NtUserEnumDisplaySettings( &device_name, ENUM_CURRENT_SETTINGS, &dm, 0 ))
        {
            ERR("Failed to query current display settings for %s.\n", wine_dbgstr_w(dd.DeviceName));
            continue;
        }

        TRACE("Device %s current display mode %ux%u %ubits %uHz at %d,%d.\n",
              wine_dbgstr_w(dd.DeviceName), dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              dm.dmDisplayFrequency, dm.u1.s2.dmPosition.x, dm.u1.s2.dmPosition.y);

        ret = NtUserChangeDisplaySettings( &device_name, &dm, NULL,
                                           CDS_GLOBAL | CDS_NORESET | CDS_UPDATEREGISTRY, NULL );
        if (ret != DISP_CHANGE_SUCCESSFUL)
            ERR("Failed to save registry display settings for %s, returned %d.\n",
                wine_dbgstr_w(dd.DeviceName), ret);
    }
}

static HKEY get_display_device_reg_key( const WCHAR *device_name )
{
    static const WCHAR display[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y'};
    static const WCHAR video_value_fmt[] = {'\\','D','e','v','i','c','e','\\',
                                            'V','i','d','e','o','%','d',0};
    static const WCHAR video_key[] = {
        '\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','H','A','R','D','W','A','R','E',
        '\\','D','E','V','I','C','E','M','A','P',
        '\\','V','I','D','E','O'};
    static const WCHAR current_config_key[] = {
        '\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s',
        '\\','C','u','r','r','e','n','t'};
    WCHAR value_name[MAX_PATH], buffer[4096], *end_ptr;
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    DWORD adapter_index, size;
    HKEY hkey;

    /* Device name has to be \\.\DISPLAY%d */
    if (strncmpiW(device_name, display, ARRAY_SIZE(display)))
        return FALSE;

    /* Parse \\.\DISPLAY* */
    adapter_index = strtolW(device_name + ARRAY_SIZE(display), &end_ptr, 10) - 1;
    if (*end_ptr)
        return FALSE;

    /* Open \Device\Video* in HKLM\HARDWARE\DEVICEMAP\VIDEO\ */
    if (!(hkey = reg_open_key( NULL, video_key, sizeof(video_key) ))) return FALSE;
    sprintfW(value_name, video_value_fmt, adapter_index);
    size = query_reg_value( hkey, value_name, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ) return FALSE;

    /* Replace \Registry\Machine\ prefix with HKEY_CURRENT_CONFIG */
    memmove( buffer + ARRAYSIZE(current_config_key), (const WCHAR *)value->Data + 17,
             size - 17 * sizeof(WCHAR) );
    memcpy( buffer, current_config_key, sizeof(current_config_key) );
    TRACE( "display device %s registry settings key %s.\n", wine_dbgstr_w(device_name),
           wine_dbgstr_w(buffer) );
    return reg_open_key( NULL, buffer, lstrlenW(buffer) * sizeof(WCHAR) );
}

static BOOL query_display_setting( HKEY hkey, const char *name, DWORD *ret )
{
    char buffer[1024];
    WCHAR nameW[128];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;

    asciiz_to_unicode( nameW, name );
    if (query_reg_value( hkey, nameW, value, sizeof(buffer) ) != sizeof(DWORD) ||
        value->Type != REG_DWORD)
        return FALSE;

    *ret = *(DWORD *)value->Data;
    return TRUE;
}

static BOOL read_registry_settings(const WCHAR *device_name, DEVMODEW *dm)
{
    HANDLE mutex;
    HKEY hkey;
    BOOL ret = TRUE;

    dm->dmFields = 0;

    mutex = get_display_device_init_mutex();
    if (!(hkey = get_display_device_reg_key( device_name )))
    {
        release_display_device_init_mutex(mutex);
        return FALSE;
    }

    ret &= query_display_setting( hkey, "DefaultSettings.BitsPerPel", &dm->dmBitsPerPel );
    dm->dmFields |= DM_BITSPERPEL;
    ret &= query_display_setting( hkey, "DefaultSettings.XResolution", &dm->dmPelsWidth );
    dm->dmFields |= DM_PELSWIDTH;
    ret &= query_display_setting( hkey, "DefaultSettings.YResolution", &dm->dmPelsHeight );
    dm->dmFields |= DM_PELSHEIGHT;
    ret &= query_display_setting( hkey, "DefaultSettings.VRefresh", &dm->dmDisplayFrequency );
    dm->dmFields |= DM_DISPLAYFREQUENCY;
    ret &= query_display_setting( hkey, "DefaultSettings.Flags", &dm->u2.dmDisplayFlags );
    dm->dmFields |= DM_DISPLAYFLAGS;
    ret &= query_display_setting( hkey, "DefaultSettings.XPanning", (DWORD *)&dm->u1.s2.dmPosition.x );
    ret &= query_display_setting( hkey, "DefaultSettings.YPanning", (DWORD *)&dm->u1.s2.dmPosition.y );
    dm->dmFields |= DM_POSITION;
    ret &= query_display_setting( hkey, "DefaultSettings.Orientation", &dm->u1.s2.dmDisplayOrientation );
    dm->dmFields |= DM_DISPLAYORIENTATION;
    ret &= query_display_setting( hkey, "DefaultSettings.FixedOutput", &dm->u1.s2.dmDisplayFixedOutput );

    NtClose( hkey );
    release_display_device_init_mutex(mutex);
    return ret;
}

static BOOL set_setting_value( HKEY hkey, const char *name, DWORD val )
{
    WCHAR nameW[128];
    UNICODE_STRING str = { asciiz_to_unicode( nameW, name ) - sizeof(WCHAR), sizeof(nameW), nameW };
    return !NtSetValueKey( hkey, &str, 0, REG_DWORD, &val, sizeof(val) );
}

static BOOL write_registry_settings(const WCHAR *device_name, const DEVMODEW *dm)
{
    HANDLE mutex;
    HKEY hkey;
    BOOL ret = TRUE;

    mutex = get_display_device_init_mutex();
    if (!(hkey = get_display_device_reg_key( device_name )))
    {
        release_display_device_init_mutex(mutex);
        return FALSE;
    }

    ret &= set_setting_value( hkey, "DefaultSettings.BitsPerPel", dm->dmBitsPerPel );
    ret &= set_setting_value( hkey, "DefaultSettings.XResolution", dm->dmPelsWidth );
    ret &= set_setting_value( hkey, "DefaultSettings.YResolution", dm->dmPelsHeight );
    ret &= set_setting_value( hkey, "DefaultSettings.VRefresh", dm->dmDisplayFrequency );
    ret &= set_setting_value( hkey, "DefaultSettings.Flags", dm->u2.dmDisplayFlags );
    ret &= set_setting_value( hkey, "DefaultSettings.XPanning", dm->u1.s2.dmPosition.x );
    ret &= set_setting_value( hkey, "DefaultSettings.YPanning", dm->u1.s2.dmPosition.y );
    ret &= set_setting_value( hkey, "DefaultSettings.Orientation", dm->u1.s2.dmDisplayOrientation );
    ret &= set_setting_value( hkey, "DefaultSettings.FixedOutput", dm->u1.s2.dmDisplayFixedOutput );

    NtClose( hkey );
    release_display_device_init_mutex(mutex);
    return ret;
}

BOOL get_primary_adapter(WCHAR *name)
{
    DISPLAY_DEVICEW dd;
    DWORD i;

    dd.cb = sizeof(dd);
    for (i = 0; !NtUserEnumDisplayDevices( NULL, i, &dd, 0 ); ++i)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static int mode_compare(const void *p1, const void *p2)
{
    DWORD a_width, a_height, b_width, b_height;
    const DEVMODEW *a = p1, *b = p2;

    /* Use the width and height in landscape mode for comparison */
    if (a->u1.s2.dmDisplayOrientation == DMDO_DEFAULT || a->u1.s2.dmDisplayOrientation == DMDO_180)
    {
        a_width = a->dmPelsWidth;
        a_height = a->dmPelsHeight;
    }
    else
    {
        a_width = a->dmPelsHeight;
        a_height = a->dmPelsWidth;
    }

    if (b->u1.s2.dmDisplayOrientation == DMDO_DEFAULT || b->u1.s2.dmDisplayOrientation == DMDO_180)
    {
        b_width = b->dmPelsWidth;
        b_height = b->dmPelsHeight;
    }
    else
    {
        b_width = b->dmPelsHeight;
        b_height = b->dmPelsWidth;
    }

    /* Depth in descending order */
    if (a->dmBitsPerPel != b->dmBitsPerPel)
        return b->dmBitsPerPel - a->dmBitsPerPel;

    /* Width in ascending order */
    if (a_width != b_width)
        return a_width - b_width;

    /* Height in ascending order */
    if (a_height != b_height)
        return a_height - b_height;

    /* Frequency in descending order */
    if (a->dmDisplayFrequency != b->dmDisplayFrequency)
        return b->dmDisplayFrequency - a->dmDisplayFrequency;

    /* Orientation in ascending order */
    return a->u1.s2.dmDisplayOrientation - b->u1.s2.dmDisplayOrientation;
}

static void set_display_depth(ULONG_PTR display_id, DWORD depth)
{
    struct x11drv_display_depth *display_depth;

    pthread_mutex_lock( &settings_mutex );
    LIST_FOR_EACH_ENTRY(display_depth, &x11drv_display_depth_list, struct x11drv_display_depth, entry)
    {
        if (display_depth->display_id == display_id)
        {
            display_depth->depth = depth;
            pthread_mutex_unlock( &settings_mutex );
            return;
        }
    }

    display_depth = heap_alloc(sizeof(*display_depth));
    if (!display_depth)
    {
        ERR("Failed to allocate memory.\n");
        pthread_mutex_unlock( &settings_mutex );
        return;
    }

    display_depth->display_id = display_id;
    display_depth->depth = depth;
    list_add_head(&x11drv_display_depth_list, &display_depth->entry);
    pthread_mutex_unlock( &settings_mutex );
}

static DWORD get_display_depth(ULONG_PTR display_id)
{
    struct x11drv_display_depth *display_depth;
    DWORD depth;

    pthread_mutex_lock( &settings_mutex );
    LIST_FOR_EACH_ENTRY(display_depth, &x11drv_display_depth_list, struct x11drv_display_depth, entry)
    {
        if (display_depth->display_id == display_id)
        {
            depth = display_depth->depth;
            pthread_mutex_unlock( &settings_mutex );
            return depth;
        }
    }
    pthread_mutex_unlock( &settings_mutex );
    return screen_bpp;
}

/***********************************************************************
 *		EnumDisplaySettingsEx  (X11DRV.@)
 *
 */
BOOL X11DRV_EnumDisplaySettingsEx( LPCWSTR name, DWORD n, LPDEVMODEW devmode, DWORD flags)
{
    static const WCHAR dev_name[CCHDEVICENAME] =
        { 'W','i','n','e',' ','X','1','1',' ','d','r','i','v','e','r',0 };
    DEVMODEW *modes;
    UINT mode_count;
    ULONG_PTR id;

    if (n == ENUM_REGISTRY_SETTINGS)
    {
        if (!read_registry_settings(name, devmode))
        {
            ERR("Failed to get %s registry display settings.\n", wine_dbgstr_w(name));
            return FALSE;
        }
        goto done;
    }

    if (n == ENUM_CURRENT_SETTINGS)
    {
        if (!handler.get_id(name, &id) || !handler.get_current_mode(id, devmode))
        {
            ERR("Failed to get %s current display settings.\n", wine_dbgstr_w(name));
            return FALSE;
        }

        if (!is_detached_mode(devmode))
            devmode->dmBitsPerPel = get_display_depth(id);

        goto done;
    }

    pthread_mutex_lock( &settings_mutex );
    if (n == 0 || lstrcmpiW(cached_device_name, name) || cached_flags != flags)
    {
        if (!handler.get_id(name, &id) || !handler.get_modes(id, flags, &modes, &mode_count))
        {
            ERR("Failed to get %s supported display modes.\n", wine_dbgstr_w(name));
            pthread_mutex_unlock( &settings_mutex );
            return FALSE;
        }

        qsort(modes, mode_count, sizeof(*modes) + modes[0].dmDriverExtra, mode_compare);

        if (cached_modes)
            handler.free_modes(cached_modes);
        lstrcpyW(cached_device_name, name);
        cached_flags = flags;
        cached_modes = modes;
        cached_mode_count = mode_count;
    }

    if (n >= cached_mode_count)
    {
        pthread_mutex_unlock( &settings_mutex );
        WARN("handler:%s device:%s mode index:%#x not found.\n", handler.name, wine_dbgstr_w(name), n);
        SetLastError(ERROR_NO_MORE_FILES);
        return FALSE;
    }

    memcpy(devmode, (BYTE *)cached_modes + (sizeof(*cached_modes) + cached_modes[0].dmDriverExtra) * n, sizeof(*devmode));
    pthread_mutex_unlock( &settings_mutex );

done:
    /* Set generic fields */
    devmode->dmSize = FIELD_OFFSET(DEVMODEW, dmICMMethod);
    devmode->dmDriverExtra = 0;
    devmode->dmSpecVersion = DM_SPECVERSION;
    devmode->dmDriverVersion = DM_SPECVERSION;
    lstrcpyW(devmode->dmDeviceName, dev_name);
    return TRUE;
}

BOOL is_detached_mode(const DEVMODEW *mode)
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

/* Get the full display mode with all the necessary fields set.
 * Return NULL on failure. Caller should call free_full_mode() to free the returned mode. */
static DEVMODEW *get_full_mode(ULONG_PTR id, DEVMODEW *dev_mode)
{
    DEVMODEW *modes, *full_mode, *found_mode = NULL;
    UINT mode_count, mode_idx;

    if (is_detached_mode(dev_mode))
        return dev_mode;

    if (!handler.get_modes(id, EDS_ROTATEDMODE, &modes, &mode_count))
        return NULL;

    qsort(modes, mode_count, sizeof(*modes) + modes[0].dmDriverExtra, mode_compare);
    for (mode_idx = 0; mode_idx < mode_count; ++mode_idx)
    {
        found_mode = (DEVMODEW *)((BYTE *)modes + (sizeof(*modes) + modes[0].dmDriverExtra) * mode_idx);

        if (dev_mode->dmFields & DM_BITSPERPEL &&
            dev_mode->dmBitsPerPel &&
            found_mode->dmBitsPerPel != dev_mode->dmBitsPerPel)
            continue;
        if (dev_mode->dmFields & DM_PELSWIDTH && found_mode->dmPelsWidth != dev_mode->dmPelsWidth)
            continue;
        if (dev_mode->dmFields & DM_PELSHEIGHT && found_mode->dmPelsHeight != dev_mode->dmPelsHeight)
            continue;
        if (dev_mode->dmFields & DM_DISPLAYFREQUENCY &&
            dev_mode->dmDisplayFrequency &&
            found_mode->dmDisplayFrequency &&
            dev_mode->dmDisplayFrequency != 1 &&
            dev_mode->dmDisplayFrequency != found_mode->dmDisplayFrequency)
            continue;
        if (dev_mode->dmFields & DM_DISPLAYORIENTATION &&
            found_mode->u1.s2.dmDisplayOrientation != dev_mode->u1.s2.dmDisplayOrientation)
            continue;

        break;
    }

    if (!found_mode || mode_idx == mode_count)
    {
        handler.free_modes(modes);
        return NULL;
    }

    if (!(full_mode = heap_alloc(sizeof(*found_mode) + found_mode->dmDriverExtra)))
    {
        handler.free_modes(modes);
        return NULL;
    }

    memcpy(full_mode, found_mode, sizeof(*found_mode) + found_mode->dmDriverExtra);
    handler.free_modes(modes);

    full_mode->dmFields |= DM_POSITION;
    full_mode->u1.s2.dmPosition = dev_mode->u1.s2.dmPosition;
    return full_mode;
}

static void free_full_mode(DEVMODEW *mode)
{
    if (!is_detached_mode(mode))
        heap_free(mode);
}

static LONG get_display_settings(struct x11drv_display_setting **new_displays,
        INT *new_display_count, const WCHAR *dev_name, DEVMODEW *dev_mode)
{
    struct x11drv_display_setting *displays;
    DEVMODEW registry_mode, current_mode;
    INT display_idx, display_count = 0;
    DISPLAY_DEVICEW display_device;
    LONG ret = DISP_CHANGE_FAILED;
    UNICODE_STRING device_name;

    display_device.cb = sizeof(display_device);
    for (display_idx = 0; !NtUserEnumDisplayDevices( NULL, display_idx, &display_device, 0 ); ++display_idx)
        ++display_count;

    displays = heap_calloc(display_count, sizeof(*displays));
    if (!displays)
        goto done;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        if (NtUserEnumDisplayDevices( NULL, display_idx, &display_device, 0 ))
            goto done;

        if (!handler.get_id(display_device.DeviceName, &displays[display_idx].id))
        {
            ret = DISP_CHANGE_BADPARAM;
            goto done;
        }

        RtlInitUnicodeString( &device_name, display_device.DeviceName );

        if (!dev_mode)
        {
            memset(&registry_mode, 0, sizeof(registry_mode));
            registry_mode.dmSize = sizeof(registry_mode);
            if (!NtUserEnumDisplaySettings( &device_name, ENUM_REGISTRY_SETTINGS, &registry_mode, 0 ))
                goto done;

            displays[display_idx].desired_mode = registry_mode;
        }
        else if (!lstrcmpiW(dev_name, display_device.DeviceName))
        {
            displays[display_idx].desired_mode = *dev_mode;
            if (!(dev_mode->dmFields & DM_POSITION))
            {
                memset(&current_mode, 0, sizeof(current_mode));
                current_mode.dmSize = sizeof(current_mode);
                if (!NtUserEnumDisplaySettings( &device_name, ENUM_CURRENT_SETTINGS, &current_mode, 0 ))
                    goto done;

                displays[display_idx].desired_mode.dmFields |= DM_POSITION;
                displays[display_idx].desired_mode.u1.s2.dmPosition = current_mode.u1.s2.dmPosition;
            }
        }
        else
        {
            memset(&current_mode, 0, sizeof(current_mode));
            current_mode.dmSize = sizeof(current_mode);
            if (!NtUserEnumDisplaySettings( &device_name, ENUM_CURRENT_SETTINGS, &current_mode, 0 ))
                goto done;

            displays[display_idx].desired_mode = current_mode;
        }

        SetRect(&displays[display_idx].desired_rect,
                displays[display_idx].desired_mode.u1.s2.dmPosition.x,
                displays[display_idx].desired_mode.u1.s2.dmPosition.y,
                displays[display_idx].desired_mode.u1.s2.dmPosition.x + displays[display_idx].desired_mode.dmPelsWidth,
                displays[display_idx].desired_mode.u1.s2.dmPosition.y + displays[display_idx].desired_mode.dmPelsHeight);
        lstrcpyW(displays[display_idx].desired_mode.dmDeviceName, display_device.DeviceName);
    }

    *new_displays = displays;
    *new_display_count = display_count;
    return DISP_CHANGE_SUCCESSFUL;

done:
    heap_free(displays);
    return ret;
}

static INT offset_length(POINT offset)
{
    return offset.x * offset.x + offset.y * offset.y;
}

/* Check if a rect overlaps with placed display rects */
static BOOL overlap_placed_displays(const RECT *rect, const struct x11drv_display_setting *displays, INT display_count)
{
    INT display_idx;
    RECT intersect;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        if (displays[display_idx].placed &&
            IntersectRect(&intersect, &displays[display_idx].new_rect, rect))
            return TRUE;
    }
    return FALSE;
}

/* Get the offset with minimum length to place a display next to the placed displays with no spacing and overlaps */
static POINT get_placement_offset(const struct x11drv_display_setting *displays, INT display_count, INT placing_idx)
{
    POINT points[8], left_top, offset, min_offset = {0, 0};
    INT display_idx, point_idx, point_count, vertex_idx;
    BOOL has_placed = FALSE, first = TRUE;
    INT width, height;
    RECT rect;

    /* If the display to be placed is detached, no offset is needed to place it */
    if (IsRectEmpty(&displays[placing_idx].desired_rect))
        return min_offset;

    /* If there is no placed and attached display, place this display as it is */
    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        if (displays[display_idx].placed && !IsRectEmpty(&displays[display_idx].new_rect))
        {
            has_placed = TRUE;
            break;
        }
    }

    if (!has_placed)
        return min_offset;

    /* Try to place this display with each of its four vertices at every vertex of the placed
     * displays and see which combination has the minimum offset length */
    width = displays[placing_idx].desired_rect.right - displays[placing_idx].desired_rect.left;
    height = displays[placing_idx].desired_rect.bottom - displays[placing_idx].desired_rect.top;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        if (!displays[display_idx].placed || IsRectEmpty(&displays[display_idx].new_rect))
            continue;

        /* Get four vertices of the placed display rectangle */
        points[0].x = displays[display_idx].new_rect.left;
        points[0].y = displays[display_idx].new_rect.top;
        points[1].x = displays[display_idx].new_rect.left;
        points[1].y = displays[display_idx].new_rect.bottom;
        points[2].x = displays[display_idx].new_rect.right;
        points[2].y = displays[display_idx].new_rect.top;
        points[3].x = displays[display_idx].new_rect.right;
        points[3].y = displays[display_idx].new_rect.bottom;
        point_count = 4;

        /* Intersected points when moving the display to be placed horizontally */
        if (displays[placing_idx].desired_rect.bottom >= displays[display_idx].new_rect.top &&
            displays[placing_idx].desired_rect.top <= displays[display_idx].new_rect.bottom)
        {
            points[point_count].x = displays[display_idx].new_rect.left;
            points[point_count++].y = displays[placing_idx].desired_rect.top;
            points[point_count].x = displays[display_idx].new_rect.right;
            points[point_count++].y = displays[placing_idx].desired_rect.top;
        }
        /* Intersected points when moving the display to be placed vertically */
        if (displays[placing_idx].desired_rect.left <= displays[display_idx].new_rect.right &&
            displays[placing_idx].desired_rect.right >= displays[display_idx].new_rect.left)
        {
            points[point_count].x = displays[placing_idx].desired_rect.left;
            points[point_count++].y = displays[display_idx].new_rect.top;
            points[point_count].x = displays[placing_idx].desired_rect.left;
            points[point_count++].y = displays[display_idx].new_rect.bottom;
        }

        /* Try moving each vertex of the display rectangle to each points */
        for (point_idx = 0; point_idx < point_count; ++point_idx)
        {
            for (vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
            {
                switch (vertex_idx)
                {
                /* Move the bottom right vertex to the point */
                case 0:
                    left_top.x = points[point_idx].x - width;
                    left_top.y = points[point_idx].y - height;
                    break;
                /* Move the bottom left vertex to the point */
                case 1:
                    left_top.x = points[point_idx].x;
                    left_top.y = points[point_idx].y - height;
                    break;
                /* Move the top right vertex to the point */
                case 2:
                    left_top.x = points[point_idx].x - width;
                    left_top.y = points[point_idx].y;
                    break;
                /* Move the top left vertex to the point */
                case 3:
                    left_top.x = points[point_idx].x;
                    left_top.y = points[point_idx].y;
                    break;
                }

                offset.x = left_top.x - displays[placing_idx].desired_rect.left;
                offset.y = left_top.y - displays[placing_idx].desired_rect.top;
                rect = displays[placing_idx].desired_rect;
                OffsetRect(&rect, offset.x, offset.y);
                if (!overlap_placed_displays(&rect, displays, display_count))
                {
                    if (first)
                    {
                        min_offset = offset;
                        first = FALSE;
                        continue;
                    }

                    if (offset_length(offset) < offset_length(min_offset))
                        min_offset = offset;
                }
            }
        }
    }

    return min_offset;
}

static void place_all_displays(struct x11drv_display_setting *displays, INT display_count)
{
    INT left_most = INT_MAX, top_most = INT_MAX;
    INT placing_idx, display_idx;
    POINT min_offset, offset;

    /* Place all displays with no extra space between them and no overlapping */
    while (1)
    {
        /* Place the unplaced display with the minimum offset length first */
        placing_idx = -1;
        for (display_idx = 0; display_idx < display_count; ++display_idx)
        {
            if (displays[display_idx].placed)
                continue;

            offset = get_placement_offset(displays, display_count, display_idx);
            if (placing_idx == -1 || offset_length(offset) < offset_length(min_offset))
            {
                min_offset = offset;
                placing_idx = display_idx;
            }
        }

        /* If all displays are placed */
        if (placing_idx == -1)
            break;

        displays[placing_idx].new_rect = displays[placing_idx].desired_rect;
        OffsetRect(&displays[placing_idx].new_rect, min_offset.x, min_offset.y);
        displays[placing_idx].placed = TRUE;
    }

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        displays[display_idx].desired_mode.u1.s2.dmPosition.x = displays[display_idx].new_rect.left;
        displays[display_idx].desired_mode.u1.s2.dmPosition.y = displays[display_idx].new_rect.top;
        left_most = min(left_most, displays[display_idx].new_rect.left);
        top_most = min(top_most, displays[display_idx].new_rect.top);
    }

    /* Convert virtual screen coordinates to root coordinates */
    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        displays[display_idx].desired_mode.u1.s2.dmPosition.x -= left_most;
        displays[display_idx].desired_mode.u1.s2.dmPosition.y -= top_most;
    }
}

static LONG apply_display_settings(struct x11drv_display_setting *displays, INT display_count, BOOL do_attach)
{
    DEVMODEW *full_mode;
    BOOL attached_mode;
    INT display_idx;
    LONG ret;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        attached_mode = !is_detached_mode(&displays[display_idx].desired_mode);
        if ((attached_mode && !do_attach) || (!attached_mode && do_attach))
            continue;

        full_mode = get_full_mode(displays[display_idx].id, &displays[display_idx].desired_mode);
        if (!full_mode)
            return DISP_CHANGE_BADMODE;

        TRACE("handler:%s changing %s to position:(%d,%d) resolution:%ux%u frequency:%uHz "
              "depth:%ubits orientation:%#x.\n", handler.name,
              wine_dbgstr_w(displays[display_idx].desired_mode.dmDeviceName),
              full_mode->u1.s2.dmPosition.x, full_mode->u1.s2.dmPosition.y, full_mode->dmPelsWidth,
              full_mode->dmPelsHeight, full_mode->dmDisplayFrequency, full_mode->dmBitsPerPel,
              full_mode->u1.s2.dmDisplayOrientation);

        ret = handler.set_current_mode(displays[display_idx].id, full_mode);
        if (attached_mode && ret == DISP_CHANGE_SUCCESSFUL)
            set_display_depth(displays[display_idx].id, full_mode->dmBitsPerPel);
        free_full_mode(full_mode);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return ret;
    }

    return DISP_CHANGE_SUCCESSFUL;
}

static BOOL all_detached_settings(const struct x11drv_display_setting *displays, INT display_count)
{
    INT display_idx;

    for (display_idx = 0; display_idx < display_count; ++display_idx)
    {
        if (!is_detached_mode(&displays[display_idx].desired_mode))
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		ChangeDisplaySettingsEx  (X11DRV.@)
 *
 */
LONG X11DRV_ChangeDisplaySettingsEx( LPCWSTR devname, LPDEVMODEW devmode,
                                     HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    struct x11drv_display_setting *displays;
    INT display_idx, display_count;
    DEVMODEW *full_mode;
    LONG ret;

    ret = get_display_settings(&displays, &display_count, devname, devmode);
    if (ret != DISP_CHANGE_SUCCESSFUL)
        return ret;

    if (flags & CDS_UPDATEREGISTRY && devname && devmode)
    {
        for (display_idx = 0; display_idx < display_count; ++display_idx)
        {
            if (!lstrcmpiW(displays[display_idx].desired_mode.dmDeviceName, devname))
            {
                full_mode = get_full_mode(displays[display_idx].id, &displays[display_idx].desired_mode);
                if (!full_mode)
                {
                    heap_free(displays);
                    return DISP_CHANGE_BADMODE;
                }

                if (!write_registry_settings(devname, full_mode))
                {
                    ERR("Failed to write %s display settings to registry.\n", wine_dbgstr_w(devname));
                    free_full_mode(full_mode);
                    heap_free(displays);
                    return DISP_CHANGE_NOTUPDATED;
                }

                free_full_mode(full_mode);
                break;
            }
        }
    }

    if (flags & (CDS_TEST | CDS_NORESET))
    {
        heap_free(displays);
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (all_detached_settings(displays, display_count))
    {
        WARN("Detaching all displays is not permitted.\n");
        heap_free(displays);
        return DISP_CHANGE_SUCCESSFUL;
    }

    place_all_displays(displays, display_count);

    /* Detach displays first to free up CRTCs */
    ret = apply_display_settings(displays, display_count, FALSE);
    if (ret == DISP_CHANGE_SUCCESSFUL)
        ret = apply_display_settings(displays, display_count, TRUE);
    if (ret == DISP_CHANGE_SUCCESSFUL)
        X11DRV_DisplayDevices_Update(TRUE);
    heap_free(displays);
    return ret;
}
