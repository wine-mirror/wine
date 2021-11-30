/*
 * System parameters functions
 *
 * Copyright 1994 Alexandre Julliard
 * Copyright 2019 Zhiyi Zhang for CodeWeavers
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "devpropdef.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);


static HKEY video_key, enum_key, config_key;

static const WCHAR devicemap_video_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','H','A','R','D','W','A','R','E',
    '\\','D','E','V','I','C','E','M','A','P',
    '\\','V','I','D','E','O'
};

static const WCHAR enum_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','E','n','u','m'
};

static const WCHAR config_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s',
    '\\','C','u','r','r','e','n','t'
};

static const WCHAR wine_devpropkey_monitor_stateflagsW[] =
{
    'P','r','o','p','e','r','t','i','e','s','\\',
    '{','2','3','3','a','9','e','f','3','-','a','f','c','4','-','4','a','b','d',
    '-','b','5','6','4','-','c','3','2','f','2','1','f','1','5','3','5','b','}',
    '\\','0','0','0','2'
};

static const WCHAR wine_devpropkey_monitor_rcmonitorW[] =
{
    'P','r','o','p','e','r','t','i','e','s','\\',
    '{','2','3','3','a','9','e','f','3','-','a','f','c','4','-','4','a','b','d',
    '-','b','5','6','4','-','c','3','2','f','2','1','f','1','5','3','5','b','}',
    '\\','0','0','0','3'
};

static const WCHAR wine_devpropkey_monitor_rcworkW[] =
{
    'P','r','o','p','e','r','t','i','e','s','\\',
    '{','2','3','3','a','9','e','f','3','-','a','f','c','4','-','4','a','b','d',
    '-','b','5','6','4','-','c','3','2','f','2','1','f','1','5','3','5','b','}',
    '\\','0','0','0','4'
};

static const WCHAR state_flagsW[] = {'S','t','a','t','e','F','l','a','g','s',0};
static const WCHAR gpu_idW[] = {'G','P','U','I','D',0};
static const WCHAR hardware_idW[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR device_descW[] = {'D','e','v','i','c','e','D','e','s','c',0};
static const WCHAR driver_descW[] = {'D','r','i','v','e','r','D','e','s','c',0};
static const WCHAR driverW[] = {'D','r','i','v','e','r',0};

static const WCHAR guid_devinterface_monitorW[] =
    {'{','E','6','F','0','7','B','5','F','-','E','E','9','7','-','4','A','9','0','-',
     'B','0','7','6','-','3','3','F','5','7','B','F','4','E','A','A','7','}',0};

/* Cached display device information */
struct display_device
{
    WCHAR device_name[32];     /* DeviceName in DISPLAY_DEVICEW */
    WCHAR device_string[128];  /* DeviceString in DISPLAY_DEVICEW */
    DWORD state_flags;         /* StateFlags in DISPLAY_DEVICEW */
    WCHAR device_id[128];      /* DeviceID in DISPLAY_DEVICEW */
    WCHAR interface_name[128]; /* DeviceID in DISPLAY_DEVICEW when EDD_GET_DEVICE_INTERFACE_NAME is set */
    WCHAR device_key[128];     /* DeviceKey in DISPLAY_DEVICEW */
};

struct adapter
{
    struct list entry;
    struct display_device dev;
    unsigned int id;
    const WCHAR *config_key;
};

struct monitor
{
    struct list entry;
    struct display_device dev;
    struct adapter *adapter;
    unsigned int id;
    unsigned int flags;
    RECT rc_monitor;
    RECT rc_work;
};

static struct list adapters = LIST_INIT(adapters);
static struct list monitors = LIST_INIT(monitors);
static INT64 last_query_display_time;
static pthread_mutex_t display_lock = PTHREAD_MUTEX_INITIALIZER;

static HANDLE get_display_device_init_mutex( void )
{
    static const WCHAR display_device_initW[] =
        {'\\','B','a','s','e','N','a','m','e','d','O','b','j','e','c','t','s',
         '\\','d','i','s','p','l','a','y','_','d','e','v','i','c','e','_','i','n','i','t'};
    UNICODE_STRING name = { sizeof(display_device_initW), sizeof(display_device_initW),
        (WCHAR *)display_device_initW };
    OBJECT_ATTRIBUTES attr;
    HANDLE mutex;

    InitializeObjectAttributes( &attr, &name, OBJ_OPENIF, NULL, NULL );
    if (NtCreateMutant( &mutex, MUTEX_ALL_ACCESS, &attr, FALSE ) < 0) return 0;
    NtWaitForSingleObject( mutex, FALSE, NULL );
    return mutex;
}

static void release_display_device_init_mutex( HANDLE mutex )
{
    NtReleaseMutant( mutex, NULL );
    NtClose( mutex );
}

static BOOL read_display_adapter_settings( unsigned int index, struct adapter *info )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    HKEY hkey;
    DWORD size;

    if (!enum_key && !(enum_key = reg_open_key( NULL, enum_keyW, sizeof(enum_keyW) )))
        return FALSE;
    if (!config_key && !(config_key = reg_open_key( NULL, config_keyW, sizeof(config_keyW) )))
        return FALSE;

    /* Find adapter */
    sprintf( buffer, "\\Device\\Video%d", index );
    size = query_reg_ascii_value( video_key, buffer, value, sizeof(buffer) );
    if (!size || value->Type != REG_SZ ||
        value->DataLength <= sizeof("\\Registry\\Machine\\") * sizeof(WCHAR))
        return FALSE;

    /* DeviceKey */
    memcpy( info->dev.device_key, value_str, value->DataLength );
    info->config_key = info->dev.device_key + sizeof("\\Registry\\Machine\\") - 1;

    if (!(hkey = reg_open_key( NULL, value_str, value->DataLength - sizeof(WCHAR) )))
        return FALSE;

    /* DeviceString */
    if (query_reg_value( hkey, driver_descW, value, sizeof(buffer) ) && value->Type == REG_SZ)
        memcpy( info->dev.device_string, value_str, value->DataLength );
    NtClose( hkey );

    /* DeviceName */
    sprintf( buffer, "\\\\.\\DISPLAY%d", index + 1 );
    asciiz_to_unicode( info->dev.device_name, buffer );

    if (!(hkey = reg_open_key( config_key, info->config_key,
                               lstrlenW( info->config_key ) * sizeof(WCHAR) )))
        return FALSE;

    /* StateFlags */
    if (query_reg_value( hkey, state_flagsW, value, sizeof(buffer) ) && value->Type == REG_DWORD)
        info->dev.state_flags = *(const DWORD *)value->Data;

    /* Interface name */
    info->dev.interface_name[0] = 0;

    /* DeviceID */
    size = query_reg_value( hkey, gpu_idW, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ) return FALSE;

    if (!(hkey = reg_open_key( enum_key, value_str, value->DataLength - sizeof(WCHAR) )))
        return FALSE;

    size = query_reg_value( hkey, hardware_idW, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || (value->Type != REG_SZ && value->Type != REG_MULTI_SZ))
        return FALSE;

    lstrcpyW( info->dev.device_id, value_str );
    return TRUE;
}

static unsigned int query_reg_subkey_value( HKEY hkey, const WCHAR *name, unsigned int name_size,
                                            KEY_VALUE_PARTIAL_INFORMATION *value, unsigned int size )
{
    HKEY subkey;

    if (!(subkey = reg_open_key( hkey, name, name_size ))) return 0;
    size = query_reg_value( subkey, NULL, value, size );
    NtClose( subkey );
    return size;
}

static BOOL read_monitor_settings( struct adapter *adapter, DWORD index, struct monitor *monitor )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *device_name, *value_str = (WCHAR *)value->Data, *ptr;
    HKEY hkey;
    DWORD size, len;

    monitor->flags = adapter->id ? 0 : MONITORINFOF_PRIMARY;

    /* DeviceName */
    sprintf( buffer, "\\\\.\\DISPLAY%d\\Monitor%d", adapter->id + 1, index );
    asciiz_to_unicode( monitor->dev.device_name, buffer );

    if (!(hkey = reg_open_key( config_key, adapter->config_key,
                               lstrlenW( adapter->config_key ) * sizeof(WCHAR) )))
        return FALSE;

    /* Interface name */
    sprintf( buffer, "MonitorID%u", index );
    size = query_reg_ascii_value( hkey, buffer, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ) return FALSE;
    len = asciiz_to_unicode( monitor->dev.interface_name, "\\\\\?\\" ) / sizeof(WCHAR) - 1;
    memcpy( monitor->dev.interface_name + len, value_str, value->DataLength - sizeof(WCHAR) );
    len += value->DataLength / sizeof(WCHAR) - 1;
    monitor->dev.interface_name[len++] = '#';
    memcpy( monitor->dev.interface_name + len, guid_devinterface_monitorW,
            sizeof(guid_devinterface_monitorW) );

    /* Replace '\\' with '#' after prefix */
    for (ptr = monitor->dev.interface_name + ARRAYSIZE("\\\\\?\\") - 1; *ptr; ptr++)
        if (*ptr == '\\') *ptr = '#';

    if (!(hkey = reg_open_key( enum_key, value_str, value->DataLength - sizeof(WCHAR) )))
        return FALSE;

    /* StateFlags, WINE_DEVPROPKEY_MONITOR_STATEFLAGS */
    size = query_reg_subkey_value( hkey, wine_devpropkey_monitor_stateflagsW,
                                   sizeof(wine_devpropkey_monitor_stateflagsW),
                                   value, sizeof(buffer) );
    if (size != sizeof(monitor->dev.state_flags))
    {
        NtClose( hkey );
        return FALSE;
    }
    monitor->dev.state_flags = *(const DWORD *)value->Data;

    /* rc_monitor, WINE_DEVPROPKEY_MONITOR_RCMONITOR */
    size = query_reg_subkey_value( hkey, wine_devpropkey_monitor_rcmonitorW,
                                   sizeof(wine_devpropkey_monitor_rcmonitorW),
                                   value, sizeof(buffer) );
    if (size != sizeof(monitor->rc_monitor))
    {
        NtClose( hkey );
        return FALSE;
    }
    monitor->rc_monitor = *(const RECT *)value->Data;

    /* rc_work, WINE_DEVPROPKEY_MONITOR_RCWORK */
    size = query_reg_subkey_value( hkey, wine_devpropkey_monitor_rcworkW,
                                   sizeof(wine_devpropkey_monitor_rcworkW),
                                   value, sizeof(buffer) );
    if (size != sizeof(monitor->rc_work))
    {
        NtClose( hkey );
        return FALSE;
    }
    monitor->rc_work = *(const RECT *)value->Data;

    /* DeviceString */
    if (!query_reg_value( hkey, device_descW, value, sizeof(buffer) ) || value->Type != REG_SZ)
    {
        NtClose( hkey );
        return FALSE;
    }
    memcpy( monitor->dev.device_string, value->Data, value->DataLength );

    /* DeviceKey */
    if (!query_reg_value( hkey, driverW, value, sizeof(buffer) ) || value->Type != REG_SZ)
    {
        NtClose( hkey );
        return FALSE;
    }
    size = asciiz_to_unicode( monitor->dev.device_key,
                              "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\" );
    device_name = &monitor->dev.device_key[size / sizeof(WCHAR) - 1];
    memcpy( device_name, value_str, value->DataLength );

    /* DeviceID */
    if (!query_reg_value( hkey, hardware_idW, value, sizeof(buffer) ) ||
        (value->Type != REG_SZ && value->Type != REG_MULTI_SZ))
    {
        NtClose( hkey );
        return FALSE;
    }
    size = lstrlenW( value_str );
    memcpy( monitor->dev.device_id, value_str, size * sizeof(WCHAR) );
    monitor->dev.device_id[size++] = '\\';
    lstrcpyW( monitor->dev.device_id + size, device_name );

    NtClose( hkey );
    return TRUE;
}

struct device_manager_ctx
{
    unsigned int gpu_count;
};

static void add_gpu( const struct gdi_gpu *gpu, void *param )
{
    struct device_manager_ctx *ctx = param;
    FIXME( "\n" );
    ctx->gpu_count++;
}

static void add_adapter( const struct gdi_adapter *adapter, void *param )
{
    FIXME( "\n" );
}

static void add_monitor( const struct gdi_monitor *monitor, void *param )
{
    FIXME( "\n" );
}

static const struct gdi_device_manager device_manager =
{
    add_gpu,
    add_adapter,
    add_monitor,
};

static void release_display_manager_ctx( struct device_manager_ctx *ctx )
{
}

static BOOL update_display_cache_from_registry(void)
{
    DWORD adapter_id, monitor_id, size;
    KEY_FULL_INFORMATION key;
    struct adapter *adapter;
    struct monitor *monitor;
    HANDLE mutex = NULL;
    NTSTATUS status;
    BOOL ret;

    /* If user driver did initialize the registry, then exit */
    if (!video_key && !(video_key = reg_open_key( NULL, devicemap_video_keyW,
                                                  sizeof(devicemap_video_keyW) )))
        return FALSE;

    status = NtQueryKey( video_key, KeyFullInformation, &key, sizeof(key), &size );
    if (status && status != STATUS_BUFFER_OVERFLOW)
        return FALSE;

    if (key.LastWriteTime.QuadPart <= last_query_display_time) return TRUE;

    mutex = get_display_device_init_mutex();
    pthread_mutex_lock( &display_lock );

    while (!list_empty( &monitors ))
    {
        monitor = LIST_ENTRY( list_head( &monitors ), struct monitor, entry );
        list_remove( &monitor->entry );
        free( monitor );
    }

    while (!list_empty( &adapters ))
    {
        adapter = LIST_ENTRY( list_head( &adapters ), struct adapter, entry );
        list_remove( &adapter->entry );
        free( adapter );
    }

    for (adapter_id = 0;; adapter_id++)
    {
        if (!(adapter = calloc( 1, sizeof(*adapter) ))) break;
        adapter->id = adapter_id;

        if (!read_display_adapter_settings( adapter_id, adapter ))
        {
            free( adapter );
            break;
        }

        list_add_tail( &adapters, &adapter->entry );
        for (monitor_id = 0;; monitor_id++)
        {
            if (!(monitor = calloc( 1, sizeof(*monitor) ))) break;
            monitor->id = monitor_id;
            monitor->adapter = adapter;

            if (!read_monitor_settings( adapter, monitor_id, monitor ))
            {
                free( monitor );
                break;
            }

            list_add_tail( &monitors, &monitor->entry );
        }
    }

    if ((ret = !list_empty( &adapters ) && !list_empty( &monitors )))
        last_query_display_time = key.LastWriteTime.QuadPart;
    pthread_mutex_unlock( &display_lock );
    release_display_device_init_mutex( mutex );
    return ret;
}

static BOOL update_display_cache(void)
{
    struct device_manager_ctx ctx = { 0 };

    user_driver->pUpdateDisplayDevices( &device_manager, FALSE, &ctx );
    release_display_manager_ctx( &ctx );

    if (update_display_cache_from_registry()) return TRUE;
    if (ctx.gpu_count)
    {
        ERR( "driver reported devices, but we failed to read them\n" );
        return FALSE;
    }

    user_driver->pUpdateDisplayDevices( &device_manager, TRUE, &ctx );
    if (!ctx.gpu_count)
    {
        static const struct gdi_monitor default_monitor =
        {
            .rc_work.right = 1024,
            .rc_work.bottom = 768,
            .rc_monitor.right = 1024,
            .rc_monitor.bottom = 768,
            .state_flags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED,
        };

        TRACE( "adding default fake monitor\n ");
        add_monitor( &default_monitor, &ctx );
    }
    release_display_manager_ctx( &ctx );

    if (!update_display_cache_from_registry())
    {
        ERR( "failed to read display config\n" );
        return FALSE;
    }

    return TRUE;
}

static BOOL lock_display_devices(void)
{
    if (!update_display_cache()) return FALSE;
    pthread_mutex_lock( &display_lock );
    return TRUE;
}

static void unlock_display_devices(void)
{
    pthread_mutex_unlock( &display_lock );
}

/**********************************************************************
 *           NtUserGetDisplayConfigBufferSizes    (win32u.@)
 */
LONG WINAPI NtUserGetDisplayConfigBufferSizes( UINT32 flags, UINT32 *num_path_info,
                                               UINT32 *num_mode_info )
{
    struct monitor *monitor;
    UINT32 count = 0;

    TRACE( "(0x%x %p %p)\n", flags, num_path_info, num_mode_info );

    if (!num_path_info || !num_mode_info)
        return ERROR_INVALID_PARAMETER;

    *num_path_info = 0;

    switch (flags)
    {
    case QDC_ALL_PATHS:
    case QDC_ONLY_ACTIVE_PATHS:
    case QDC_DATABASE_CURRENT:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    /* FIXME: semi-stub */
    if (flags != QDC_ONLY_ACTIVE_PATHS)
        FIXME( "only returning active paths\n" );

    if (!lock_display_devices()) return FALSE;
    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!(monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE))
            continue;
        count++;
    }
    unlock_display_devices();

    *num_path_info = count;
    *num_mode_info = count * 2;
    TRACE( "returning %u paths %u modes\n", *num_path_info, *num_mode_info );
    return ERROR_SUCCESS;
}
