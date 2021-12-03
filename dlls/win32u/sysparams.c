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


static HKEY video_key, enum_key, control_key, config_key;

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

static const WCHAR control_keyW[] =
{
    '\\','R','e','g','i','s','t','r','y',
    '\\','M','a','c','h','i','n','e',
    '\\','S','y','s','t','e','m',
    '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
    '\\','C','o','n','t','r','o','l'
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

static const WCHAR devpropkey_gpu_vulkan_uuidW[] =
{
    'P','r','o','p','e','r','t','i','e','s',
    '\\','{','2','3','3','A','9','E','F','3','-','A','F','C','4','-','4','A','B','D',
    '-','B','5','6','4','-','C','3','2','F','2','1','F','1','5','3','5','C','}',
    '\\','0','0','0','2'
};

static const WCHAR devpropkey_gpu_luidW[] =
{
    'P','r','o','p','e','r','t','i','e','s',
    '\\','{','6','0','B','1','9','3','C','B','-','5','2','7','6','-','4','D','0','F',
    '-','9','6','F','C','-','F','1','7','3','A','B','A','D','3','E','C','6','}',
    '\\','0','0','0','2'
};

static const WCHAR devpropkey_device_ispresentW[] =
{
    'P','r','o','p','e','r','t','i','e','s',
    '\\','{','5','4','0','B','9','4','7','E','-','8','B','4','0','-','4','5','B','C',
    '-','A','8','A','2','-','6','A','0','B','8','9','4','C','B','D','A','2','}',
    '\\','0','0','0','5'
};

static const WCHAR devpropkey_monitor_gpu_luidW[] =
{
    'P','r','o','p','e','r','t','i','e','s',
    '\\','{','C','A','0','8','5','8','5','3','-','1','6','C','E','-','4','8','A','A',
    '-','B','1','1','4','-','D','E','9','C','7','2','3','3','4','2','2','3','}',
    '\\','0','0','0','1'
};

static const WCHAR devpropkey_monitor_output_idW[] =
{
    'P','r','o','p','e','r','t','i','e','s',
    '\\','{','C','A','0','8','5','8','5','3','-','1','6','C','E','-','4','8','A','A',
    '-','B','1','1','4','-','D','E','9','C','7','2','3','3','4','2','2','3','}',
    '\\','0','0','0','2'
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

static const WCHAR wine_devpropkey_monitor_adapternameW[] =
{
    'P','r','o','p','e','r','t','i','e','s','\\',
    '{','2','3','3','a','9','e','f','3','-','a','f','c','4','-','4','a','b','d',
    '-','b','5','6','4','-','c','3','2','f','2','1','f','1','5','3','5','b','}',
    '\\','0','0','0','5'
};

static const WCHAR device_instanceW[] = {'D','e','v','i','c','e','I','n','s','t','a','n','c','e',0};
static const WCHAR controlW[] = {'C','o','n','t','r','o','l'};
static const WCHAR device_parametersW[] =
    {'D','e','v','i','c','e',' ','P','a','r','a','m','e','t','e','r','s'};
static const WCHAR linkedW[] = {'L','i','n','k','e','d',0};
static const WCHAR symbolic_link_valueW[] =
    {'S','y','m','b','o','l','i','c','L','i','n','k','V','a','l','u','e',0};
static const WCHAR state_flagsW[] = {'S','t','a','t','e','F','l','a','g','s',0};
static const WCHAR gpu_idW[] = {'G','P','U','I','D',0};
static const WCHAR hardware_idW[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR device_descW[] = {'D','e','v','i','c','e','D','e','s','c',0};
static const WCHAR driver_descW[] = {'D','r','i','v','e','r','D','e','s','c',0};
static const WCHAR driverW[] = {'D','r','i','v','e','r',0};
static const WCHAR class_guidW[] = {'C','l','a','s','s','G','U','I','D',0};
static const WCHAR pciW[] = {'P','C','I'};
static const WCHAR classW[] = {'C','l','a','s','s',0};
static const WCHAR displayW[] = {'D','i','s','p','l','a','y',0};
static const WCHAR monitorW[] = {'M','o','n','i','t','o','r',0};

static const char  guid_devclass_displayA[] = "{4D36E968-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_displayW[] =
    {'{','4','D','3','6','E','9','6','8','-','E','3','2','5','-','1','1','C','E','-',
     'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}',0};

static const char  guid_devclass_monitorA[] = "{4D36E96E-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_monitorW[] =
    {'{','4','D','3','6','E','9','6','E','-','E','3','2','5','-','1','1','C','E','-'
        ,'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}'};

static const WCHAR guid_devinterface_display_adapterW[] =
    {'{','5','B','4','5','2','0','1','D','-','F','2','F','2','-','4','F','3','B','-',
     '8','5','B','B','-','3','0','F','F','1','F','9','5','3','5','9','9','}',0};

static const WCHAR guid_display_device_arrivalW[] =
    {'{','1','C','A','0','5','1','8','0','-','A','6','9','9','-','4','5','0','A','-',
     '9','A','0','C','-','D','E','4','F','B','E','3','D','D','D','8','9','}',0};

static const WCHAR guid_devinterface_monitorW[] =
    {'{','E','6','F','0','7','B','5','F','-','E','E','9','7','-','4','A','9','0','-',
     'B','0','7','6','-','3','3','F','5','7','B','F','4','E','A','A','7','}',0};

#define NULLDRV_DEFAULT_HMONITOR ((HMONITOR)(UINT_PTR)(0x10000 + 1))

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
    HANDLE handle;
    unsigned int id;
    unsigned int flags;
    RECT rc_monitor;
    RECT rc_work;
};

static struct list adapters = LIST_INIT(adapters);
static struct list monitors = LIST_INIT(monitors);
static INT64 last_query_display_time;
static pthread_mutex_t display_lock = PTHREAD_MUTEX_INITIALIZER;

static struct monitor virtual_monitor =
{
    .handle = NULLDRV_DEFAULT_HMONITOR,
    .flags = MONITORINFOF_PRIMARY,
    .rc_monitor.right = 1024,
    .rc_monitor.bottom = 768,
    .rc_work.right = 1024,
    .rc_work.bottom = 768,
    .dev.state_flags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED,
};

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

static void reg_empty_key( HKEY root, const char *key_name )
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_FULL_INFORMATION *value = (KEY_VALUE_FULL_INFORMATION *)buffer;
    WCHAR bufferW[512];
    DWORD size;
    HKEY hkey;

    if (key_name)
        hkey = reg_open_key( root, bufferW, asciiz_to_unicode( bufferW, key_name ) - sizeof(WCHAR) );
    else
        hkey = root;

    while (!NtEnumerateKey( hkey, 0, KeyNodeInformation, key, sizeof(buffer), &size ))
        reg_delete_tree( hkey, key->Name, key->NameLength );

    while (!NtEnumerateValueKey( hkey, 0, KeyValueFullInformation, value, sizeof(buffer), &size ))
    {
        UNICODE_STRING name = { value->NameLength, value->NameLength, value->Name };
        NtDeleteValueKey( hkey, &name );
    }

    if (hkey != root) NtClose( hkey );
}

static void prepare_devices(void)
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (void *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    WCHAR bufferW[128];
    unsigned i = 0;
    DWORD size;
    HKEY hkey, subkey, device_key, prop_key;

    if (!enum_key) enum_key = reg_create_key( NULL, enum_keyW, sizeof(enum_keyW), 0, NULL );
    if (!control_key) control_key = reg_create_key( NULL, control_keyW, sizeof(control_keyW), 0, NULL );
    if (!config_key) config_key = reg_create_key( NULL, config_keyW, sizeof(config_keyW), 0, NULL );
    if (!video_key) video_key = reg_create_key( NULL, devicemap_video_keyW, sizeof(devicemap_video_keyW),
                                                REG_OPTION_VOLATILE, NULL );

    /* delete monitors */
    reg_empty_key( enum_key, "DISPLAY\\DEFAULT_MONITOR" );
    sprintf( buffer, "Class\\%s", guid_devclass_monitorA );
    hkey = reg_create_key( control_key, bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR),
                           0, NULL );
    reg_empty_key( hkey, NULL );
    set_reg_value( hkey, classW, REG_SZ, monitorW, sizeof(monitorW) );
    NtClose( hkey );

    /* delete adapters */
    reg_empty_key( video_key, NULL );

    /* clean GPUs */
    sprintf( buffer, "Class\\%s", guid_devclass_displayA );
    hkey = reg_create_key( control_key, bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR),
                           0, NULL );
    reg_empty_key( hkey, NULL );
    set_reg_value( hkey, classW, REG_SZ, displayW, sizeof(displayW) );
    NtClose( hkey );

    hkey = reg_open_key( enum_key, pciW, sizeof(pciW) );

    /* To preserve GPU GUIDs, mark them as not present and delete them in cleanup_devices if needed. */
    while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key, sizeof(buffer), &size ))
    {
        unsigned int j = 0;

        if (!(subkey = reg_open_key( hkey, key->Name, key->NameLength ))) continue;

        while (!NtEnumerateKey( subkey, j++, KeyNodeInformation, key, sizeof(buffer), &size ))
        {
            if (!(device_key = reg_open_key( subkey, key->Name, key->NameLength ))) continue;
            size = query_reg_value( device_key, class_guidW, value, sizeof(buffer) );
            if (size != sizeof(guid_devclass_displayW) || wcscmp( value_str, guid_devclass_displayW ))
            {
                NtClose( device_key );
                continue;
            }

            size = query_reg_value( device_key, class_guidW, value, sizeof(buffer) );
            if (size == sizeof(guid_devclass_displayW) &&
                !wcscmp( (const WCHAR *)value->Data, guid_devclass_displayW ) &&
                (prop_key = reg_create_key( device_key, devpropkey_device_ispresentW,
                                            sizeof(devpropkey_device_ispresentW), 0, NULL )))
            {
                BOOL present = FALSE;
                set_reg_value( prop_key, NULL, 0xffff0000 | DEVPROP_TYPE_BOOLEAN,
                               &present, sizeof(present) );
                NtClose( prop_key );
            }

            NtClose( device_key );
        }

        NtClose( subkey );
    }

    NtClose( hkey );
}

static void cleanup_devices(void)
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (void *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR bufferW[512], *value_str = (WCHAR *)value->Data;
    unsigned i = 0;
    DWORD size;
    HKEY hkey, subkey, device_key, prop_key;

    hkey = reg_open_key( enum_key, pciW, sizeof(pciW) );

restart:
    while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key, sizeof(buffer), &size ))
    {
        unsigned int j = 0;

        if (!(subkey = reg_open_key( hkey, key->Name, key->NameLength ))) continue;

        while (!NtEnumerateKey( subkey, j++, KeyNodeInformation, key, sizeof(buffer), &size ))
        {
            BOOL present = FALSE;

            if (!(device_key = reg_open_key( subkey, key->Name, key->NameLength ))) continue;
            memcpy( bufferW, key->Name, key->NameLength );
            bufferW[key->NameLength / sizeof(WCHAR)] = 0;

            size = query_reg_value( device_key, class_guidW, value, sizeof(buffer) );
            if (size != sizeof(guid_devclass_displayW) || wcscmp( value_str, guid_devclass_displayW ))
            {
                NtClose( device_key );
                continue;
            }

            if ((prop_key = reg_open_key( device_key, devpropkey_device_ispresentW,
                                          sizeof(devpropkey_device_ispresentW) )))
            {
                if (query_reg_value( prop_key, NULL, value, sizeof(buffer) ) == sizeof(BOOL))
                    present = *(const BOOL *)value->Data;
                NtClose( prop_key );
            }

            NtClose( device_key );

            if (!present && reg_delete_tree( subkey, bufferW, lstrlenW( bufferW ) * sizeof(WCHAR) ))
                goto restart;
        }

        NtClose( subkey );
    }

    NtClose( hkey );
}

/* see UuidCreate */
static void uuid_create( GUID *uuid )
{
    char buf[4096];
    NtQuerySystemInformation( SystemInterruptInformation, buf, sizeof(buf), NULL );
    memcpy( uuid, buf, sizeof(*uuid) );
    uuid->Data3 &= 0x0fff;
    uuid->Data3 |= (4 << 12);
    uuid->Data4[0] &= 0x3f;
    uuid->Data4[0] |= 0x80;
}

#define TICKSPERSEC  10000000
#define SECSPERDAY   86400
#define DAYSPERQUADRICENTENNIUM  (365 * 400 + 97)
#define DAYSPERNORMALQUADRENNIUM (365 * 4 + 1)

static unsigned int format_date( WCHAR *bufferW, LONGLONG time )
{
    int cleaps, years, yearday, months, days;
    unsigned int day, month, year;
    char buffer[32];

    days = time / TICKSPERSEC / SECSPERDAY;

    /* compute year, month and day of month, see RtlTimeToTimeFields */
    cleaps = (3 * ((4 * days + 1227) / DAYSPERQUADRICENTENNIUM) + 3) / 4;
    days += 28188 + cleaps;
    years = (20 * days - 2442) / (5 * DAYSPERNORMALQUADRENNIUM);
    yearday = days - (years * DAYSPERNORMALQUADRENNIUM) / 4;
    months = (64 * yearday) / 1959;
    if (months < 14)
    {
        month = months - 1;
        year = years + 1524;
    }
    else
    {
        month = months - 13;
        year = years + 1525;
    }
    day = yearday - (1959 * months) / 64 ;

    sprintf( buffer, "%u-%u-%u", month, day, year );
    return asciiz_to_unicode( bufferW, buffer );
}

struct device_manager_ctx
{
    unsigned int gpu_count;
    unsigned int adapter_count;
    unsigned int video_count;
    unsigned int monitor_count;
    unsigned int output_count;
    HANDLE mutex;
    WCHAR gpuid[128];
    WCHAR gpu_guid[64];
    LUID gpu_luid;
    HKEY adapter_key;
};

static void link_device( const WCHAR *instance, const WCHAR *class )
{
    unsigned int instance_len = lstrlenW( instance ), len;
    unsigned int class_len = lstrlenW( class );
    WCHAR buffer[MAX_PATH], *ptr;
    HKEY hkey, subkey;

    static const WCHAR symbolic_linkW[] = {'S','y','m','b','o','l','i','c','L','i','n','k',0};
    static const WCHAR hashW[] = {'#'};

    len = asciiz_to_unicode( buffer, "DeviceClasses\\" ) / sizeof(WCHAR) - 1;
    memcpy( buffer + len, class, class_len * sizeof(WCHAR) );
    len += class_len;
    len += asciiz_to_unicode( buffer + len, "\\##?#" ) / sizeof(WCHAR) - 1;
    memcpy( buffer + len, instance, instance_len * sizeof(WCHAR) );
    for (ptr = buffer + len; *ptr; ptr++) if (*ptr == '\\') *ptr = '#';
    len += instance_len;
    buffer[len++] = '#';
    memcpy( buffer + len, class, class_len * sizeof(WCHAR) );
    len += class_len;
    hkey = reg_create_key( control_key, buffer, len * sizeof(WCHAR), 0, NULL );

    set_reg_value( hkey, device_instanceW, REG_SZ, instance, instance_len * sizeof(WCHAR) );

    subkey = reg_create_key( hkey, hashW, sizeof(hashW), REG_OPTION_VOLATILE, NULL );
    NtClose( hkey );
    hkey = subkey;

    len = asciiz_to_unicode( buffer, "\\\\?\\" ) / sizeof(WCHAR) - 1;
    memcpy( buffer + len, instance, (instance_len + 1) * sizeof(WCHAR) );
    len += instance_len;
    memcpy( buffer + len, class, (class_len + 1) * sizeof(WCHAR) );
    len += class_len + 1;
    for (ptr = buffer + 4; *ptr; ptr++) if (*ptr == '\\') *ptr = '#';
    set_reg_value( hkey, symbolic_linkW, REG_SZ, buffer, len * sizeof(WCHAR) );

    if ((subkey = reg_create_key( hkey, controlW, sizeof(controlW), REG_OPTION_VOLATILE, NULL )))
    {
        const DWORD linked = 1;
        set_reg_value( subkey, linkedW, REG_DWORD, &linked, sizeof(linked) );
        NtClose( subkey );
    }
}

static void add_gpu( const struct gdi_gpu *gpu, void *param )
{
    struct device_manager_ctx *ctx = param;
    const WCHAR *desc;
    char buffer[4096];
    WCHAR bufferW[512];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    unsigned int gpu_index, size;
    HKEY hkey, subkey;
    LARGE_INTEGER ft;

    static const BOOL present = TRUE;
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
    static const WCHAR video_idW[] = {'V','i','d','e','o','I','D',0};
    static const WCHAR driver_date_dataW[] =
        {'D','r','i','v','e','r','D','a','t','e','D','a','t','a',0};
    static const WCHAR adapter_stringW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n',
         '.','A','d','a','p','t','e','r','S','t','r','i','n','g',0};
    static const WCHAR bios_stringW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'B','i','o','s','S','t','r','i','n','g',0};
    static const WCHAR chip_typeW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'C','h','i','p','T','y','p','e',0};
    static const WCHAR dac_typeW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'D','a','c','T','y','p','e',0};
    static const WCHAR ramdacW[] =
        {'I','n','t','e','r','g','r','a','t','e','d',' ','R','A','M','D','A','C',0};
    static const WCHAR driver_dateW[] = {'D','r','i','v','e','r','D','a','t','e',0};

    TRACE( "%s %04X %04X %08X %02X\n", debugstr_w(gpu->name),
           gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id );

    gpu_index = ctx->gpu_count++;
    ctx->adapter_count = 0;
    ctx->monitor_count = 0;

    if (!enum_key && !(enum_key = reg_create_key( NULL, enum_keyW, sizeof(enum_keyW), 0, NULL )))
        return;

    if (!ctx->mutex)
    {
        ctx->mutex = get_display_device_init_mutex();
        pthread_mutex_lock( &display_lock );
        prepare_devices();
    }

    sprintf( buffer, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X\\%08X",
             gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id, gpu_index );
    size = asciiz_to_unicode( ctx->gpuid, buffer );
    if (!(hkey = reg_create_key( enum_key, ctx->gpuid, size - sizeof(WCHAR), 0, NULL ))) return;

    set_reg_value( hkey, classW, REG_SZ, displayW, sizeof(displayW) );
    set_reg_value( hkey, class_guidW, REG_SZ, guid_devclass_displayW,
                   sizeof(guid_devclass_displayW) );
    sprintf( buffer, "%s\\%04X", guid_devclass_displayA, gpu_index );
    set_reg_value( hkey, driverW, REG_SZ, bufferW, asciiz_to_unicode( bufferW, buffer ));

    sprintf( buffer, "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
             gpu->vendor_id, gpu->device_id, gpu->subsys_id, gpu->revision_id );
    size = asciiz_to_unicode( bufferW, buffer );
    bufferW[size / sizeof(WCHAR)] = 0; /* for REG_MULTI_SZ */
    set_reg_value( hkey, hardware_idW, REG_MULTI_SZ, bufferW, size + sizeof(WCHAR) );

    desc = gpu->name;
    if (!desc[0]) desc = wine_adapterW;
    set_reg_value( hkey, device_descW, REG_SZ, desc, (lstrlenW( desc ) + 1) * sizeof(WCHAR) );

    if ((subkey = reg_create_key( hkey, device_parametersW, sizeof(device_parametersW), 0, NULL )))
    {
        if (!query_reg_value( subkey, video_idW, value, sizeof(buffer) ))
        {
            GUID guid;
            uuid_create( &guid );
            sprintf( buffer, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                     guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
                     guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
            size = asciiz_to_unicode( ctx->gpu_guid, buffer );
            TRACE( "created guid %s\n", debugstr_w(ctx->gpu_guid) );
            set_reg_value( subkey, video_idW, REG_SZ, ctx->gpu_guid, size );
        }
        else
        {
            memcpy( ctx->gpu_guid, value->Data, value->DataLength );
            TRACE( "got guid %s\n", debugstr_w(ctx->gpu_guid) );
        }
        NtClose( subkey );
    }

    if ((subkey = reg_create_key( hkey, devpropkey_gpu_vulkan_uuidW,
                                  sizeof(devpropkey_gpu_vulkan_uuidW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_GUID,
                       &gpu->vulkan_uuid, sizeof(gpu->vulkan_uuid) );
        NtClose( subkey );
    }

    if ((subkey = reg_create_key( hkey, devpropkey_device_ispresentW,
                                  sizeof(devpropkey_device_ispresentW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_BOOLEAN,
                       &present, sizeof(present) );
        NtClose( subkey );
    }

    if ((subkey = reg_create_key( hkey, devpropkey_gpu_luidW, sizeof(devpropkey_gpu_luidW), 0, NULL )))
    {
        if (query_reg_value( subkey, NULL, value, sizeof(buffer) ) != sizeof(LUID))
        {
            NtAllocateLocallyUniqueId( &ctx->gpu_luid );
            TRACE("allocated luid %08x%08x\n", ctx->gpu_luid.HighPart, ctx->gpu_luid.LowPart );
            set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT64,
                           &ctx->gpu_luid, sizeof(ctx->gpu_luid) );
        }
        else
        {
            memcpy( &ctx->gpu_luid, value->Data, sizeof(ctx->gpu_luid) );
            TRACE("got luid %08x%08x\n", ctx->gpu_luid.HighPart, ctx->gpu_luid.LowPart );
        }
        NtClose( subkey );
    }

    NtClose( hkey );

    sprintf( buffer, "Class\\%s\\%04X", guid_devclass_displayA, gpu_index );
    hkey = reg_create_key( control_key, bufferW,
                           asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR), 0, NULL );

    NtQuerySystemTime( &ft );
    set_reg_value( hkey, driver_dateW, REG_SZ, bufferW, format_date( bufferW, ft.QuadPart ));

    set_reg_value( hkey, driver_date_dataW, REG_BINARY, &ft, sizeof(ft) );

    size = (lstrlenW( desc ) + 1) * sizeof(WCHAR);
    set_reg_value( hkey, driver_descW, REG_SZ, desc, size );
    set_reg_value( hkey, adapter_stringW, REG_BINARY, desc, size );
    set_reg_value( hkey, bios_stringW, REG_BINARY, desc, size );
    set_reg_value( hkey, chip_typeW, REG_BINARY, desc, size );
    set_reg_value( hkey, dac_typeW, REG_BINARY, ramdacW, sizeof(ramdacW) );

    NtClose( hkey );

    link_device( ctx->gpuid, guid_devinterface_display_adapterW );
    link_device( ctx->gpuid, guid_display_device_arrivalW );
}

static void add_adapter( const struct gdi_adapter *adapter, void *param )
{
    struct device_manager_ctx *ctx = param;
    unsigned int adapter_index, video_index, len;
    char name[64], buffer[MAX_PATH];
    WCHAR nameW[64], bufferW[MAX_PATH];
    HKEY hkey;

    TRACE( "\n" );

    if (!ctx->gpu_count)
    {
        static const struct gdi_gpu default_gpu;
        TRACE( "adding default fake GPU\n" );
        add_gpu( &default_gpu, ctx );
    }

    if (ctx->adapter_key)
    {
        NtClose( ctx->adapter_key );
        ctx->adapter_key = NULL;
    }

    adapter_index = ctx->adapter_count++;
    video_index = ctx->video_count++;
    ctx->monitor_count = 0;

    len = asciiz_to_unicode( bufferW, "\\Registry\\Machine\\System\\CurrentControlSet\\"
                             "Control\\Video\\" ) / sizeof(WCHAR) - 1;
    lstrcpyW( bufferW + len, ctx->gpu_guid );
    len += lstrlenW( bufferW + len );
    sprintf( buffer, "\\%04x", adapter_index );
    len += asciiz_to_unicode( bufferW + len, buffer ) / sizeof(WCHAR) - 1;
    hkey = reg_create_key( NULL, bufferW, len * sizeof(WCHAR),
                          REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK, NULL );
    if (!hkey) hkey = reg_create_key( NULL, bufferW, len * sizeof(WCHAR),
                                     REG_OPTION_VOLATILE | REG_OPTION_OPEN_LINK, NULL );

    sprintf( name, "\\Device\\Video%u", video_index );
    asciiz_to_unicode( nameW, name );
    set_reg_value( video_key, nameW, REG_SZ, bufferW, (lstrlenW( bufferW ) + 1) * sizeof(WCHAR) );

    if (hkey)
    {
        sprintf( buffer, "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\"
                 "%s\\%04X", guid_devclass_displayA, ctx->gpu_count - 1 );
        len = asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR);
        set_reg_value( hkey, symbolic_link_valueW, REG_LINK, bufferW, len );
        NtClose( hkey );
    }
    else ERR( "failed to create link key\n" );

    /* Following information is Wine specific, it doesn't really exist on Windows. */
    len = asciiz_to_unicode( bufferW, "System\\CurrentControlSet\\Control\\Video\\" )
        / sizeof(WCHAR) - 1;
    lstrcpyW( bufferW + len, ctx->gpu_guid );
    len += lstrlenW( bufferW + len );
    sprintf( buffer, "\\%04x", adapter_index );
    len += asciiz_to_unicode( bufferW + len, buffer ) / sizeof(WCHAR) - 1;
    ctx->adapter_key = reg_create_key( config_key, bufferW, len * sizeof(WCHAR),
                                       REG_OPTION_VOLATILE, NULL );

    set_reg_value( ctx->adapter_key, gpu_idW, REG_SZ, ctx->gpuid,
                   (lstrlenW( ctx->gpuid ) + 1) * sizeof(WCHAR) );
    set_reg_value( ctx->adapter_key, state_flagsW, REG_DWORD, &adapter->state_flags,
                   sizeof(adapter->state_flags) );
}

static void add_monitor( const struct gdi_monitor *monitor, void *param )
{
    struct device_manager_ctx *ctx = param;
    char buffer[MAX_PATH], instance[64];
    unsigned int monitor_index, output_index;
    WCHAR bufferW[MAX_PATH];
    HKEY hkey, subkey;

    static const WCHAR default_monitorW[] =
        {'M','O','N','I','T','O','R','\\','D','e','f','a','u','l','t','_','M','o','n','i','t','o','r',0,0};

    TRACE( "%s %s %s\n", debugstr_w(monitor->name), wine_dbgstr_rect(&monitor->rc_monitor),
           wine_dbgstr_rect(&monitor->rc_work) );

    if (!ctx->adapter_count)
    {
        static const struct gdi_adapter default_adapter =
        {
            .state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE |
                           DISPLAY_DEVICE_VGA_COMPATIBLE,
        };
        TRACE( "adding default fake adapter\n" );
        add_adapter( &default_adapter, ctx );
    }

    monitor_index = ctx->monitor_count++;
    output_index = ctx->output_count++;

    sprintf( buffer, "MonitorID%u", monitor_index );
    sprintf( instance, "DISPLAY\\Default_Monitor\\%04X&%04X", ctx->video_count - 1, monitor_index );
    set_reg_ascii_value( ctx->adapter_key, buffer, instance );

    hkey = reg_create_key( enum_key, bufferW, asciiz_to_unicode( bufferW, instance ) - sizeof(WCHAR),
                          0, NULL );
    if (!hkey) return;

    link_device( bufferW, guid_devinterface_monitorW );

    lstrcpyW( bufferW, monitor->name );
    if (!bufferW[0]) asciiz_to_unicode( bufferW, "Generic Non-PnP Monitor" );
    set_reg_value( hkey, device_descW, REG_SZ, bufferW, (lstrlenW( bufferW ) + 1) * sizeof(WCHAR) );

    set_reg_value( hkey, classW, REG_SZ, monitorW, sizeof(monitorW) );
    sprintf( buffer, "%s\\%04X", guid_devclass_monitorA, output_index );
    set_reg_ascii_value( hkey, "Driver", buffer );
    set_reg_value( hkey, class_guidW, REG_SZ, guid_devclass_monitorW, sizeof(guid_devclass_monitorW) );
    set_reg_value( hkey, hardware_idW, REG_MULTI_SZ, default_monitorW, sizeof(default_monitorW) );

    if ((subkey = reg_create_key( hkey, device_parametersW, sizeof(device_parametersW), 0, NULL )))
    {
        static const WCHAR edidW[] = {'E','D','I','D',0};
        set_reg_value( subkey, edidW, REG_BINARY, monitor->edid, monitor->edid_len );
        NtClose( subkey );
    }

    /* StateFlags */
    if ((subkey = reg_create_key( hkey, wine_devpropkey_monitor_stateflagsW,
                                  sizeof(wine_devpropkey_monitor_stateflagsW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT32, &monitor->state_flags,
                       sizeof(monitor->state_flags) );
        NtClose( subkey );
    }

    /* WINE_DEVPROPKEY_MONITOR_RCMONITOR */
    if ((subkey = reg_create_key( hkey, wine_devpropkey_monitor_rcmonitorW,
                                  sizeof(wine_devpropkey_monitor_rcmonitorW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_BINARY, &monitor->rc_monitor,
                       sizeof(monitor->rc_monitor) );
        NtClose( subkey );
    }

    /* WINE_DEVPROPKEY_MONITOR_RCWORK */
    if ((subkey = reg_create_key( hkey, wine_devpropkey_monitor_rcworkW,
                                  sizeof(wine_devpropkey_monitor_rcworkW), 0, NULL )))
    {
        TRACE( "rc_work %s\n", wine_dbgstr_rect(&monitor->rc_work) );
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_BINARY, &monitor->rc_work,
                       sizeof(monitor->rc_work) );
        NtClose( subkey );
    }

    /* WINE_DEVPROPKEY_MONITOR_ADAPTERNAME */
    if ((subkey = reg_create_key( hkey, wine_devpropkey_monitor_adapternameW,
                                  sizeof(wine_devpropkey_monitor_adapternameW), 0, NULL )))
    {
        sprintf( buffer, "\\\\.\\DISPLAY%u", ctx->video_count );
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_STRING, bufferW,
                       asciiz_to_unicode( bufferW, buffer ));
        NtClose( subkey );
    }

    /* DEVPROPKEY_MONITOR_GPU_LUID */
    if ((subkey = reg_create_key( hkey, devpropkey_monitor_gpu_luidW,
                                  sizeof(devpropkey_monitor_gpu_luidW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_INT64,
                       &ctx->gpu_luid, sizeof(ctx->gpu_luid) );
        NtClose( subkey );
    }

    /* DEVPROPKEY_MONITOR_OUTPUT_ID */
    if ((subkey = reg_create_key( hkey, devpropkey_monitor_output_idW,
                                  sizeof(devpropkey_monitor_output_idW), 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT32,
                       &output_index, sizeof(output_index) );
        NtClose( subkey );
    }

    NtClose( hkey );

    sprintf( buffer, "Class\\%s\\%04X", guid_devclass_monitorA, output_index );
    hkey = reg_create_key( control_key, bufferW,
                           asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR), 0, NULL );
    if (hkey) NtClose( hkey );
}

static const struct gdi_device_manager device_manager =
{
    add_gpu,
    add_adapter,
    add_monitor,
};

static void release_display_manager_ctx( struct device_manager_ctx *ctx )
{
    if (ctx->mutex)
    {
        pthread_mutex_unlock( &display_lock );
        release_display_device_init_mutex( ctx->mutex );
    }
    if (ctx->adapter_key)
    {
        NtClose( ctx->adapter_key );
        last_query_display_time = 0;
    }
    if (ctx->gpu_count) cleanup_devices();
}

static void clear_display_devices(void)
{
    struct adapter *adapter;
    struct monitor *monitor;

    if (list_head( &monitors ) == &virtual_monitor.entry)
    {
        list_init( &monitors );
        return;
    }

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
}

static BOOL update_display_cache_from_registry(void)
{
    DWORD adapter_id, monitor_id, monitor_count = 0, size;
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

    clear_display_devices();

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

            monitor->handle = UlongToHandle( ++monitor_count );
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
    USEROBJECTFLAGS flags;
    HWINSTA winstation;

    pthread_mutex_lock( &display_lock );

    /* Report physical monitor information only if window station has visible display surfaces */
    winstation = NtUserGetProcessWindowStation();
    if (NtUserGetObjectInformation( winstation, UOI_FLAGS, &flags, sizeof(flags), NULL ) &&
        (flags.dwFlags & WSF_VISIBLE))
    {
        pthread_mutex_unlock( &display_lock );
        if (!update_display_cache()) return FALSE;
        pthread_mutex_lock( &display_lock );
    }
    else
    {
        clear_display_devices();
        list_add_tail( &monitors, &virtual_monitor.entry );
        last_query_display_time = 0;
    }

    return TRUE;
}

static void unlock_display_devices(void)
{
    pthread_mutex_unlock( &display_lock );
}

RECT get_virtual_screen_rect(void)
{
    struct monitor *monitor;
    RECT rect = {0};

    if (!lock_display_devices()) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        union_rect( &rect, &rect, &monitor->rc_monitor );
    }

    unlock_display_devices();
    return rect;
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

static struct adapter *find_adapter( UNICODE_STRING *name )
{
    struct adapter *adapter;

    LIST_FOR_EACH_ENTRY(adapter, &adapters, struct adapter, entry)
    {
        if (!name || !name->Length) return adapter; /* use primary adapter */
        if (!wcsnicmp( name->Buffer, adapter->dev.device_name, name->Length / sizeof(WCHAR) ) &&
            !adapter->dev.device_name[name->Length / sizeof(WCHAR)])
            return adapter;
    }
    return NULL;
}

/***********************************************************************
 *	     NtUserEnumDisplayDevices    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                      DISPLAY_DEVICEW *info, DWORD flags )
{
    struct display_device *found = NULL;
    struct adapter *adapter;
    struct monitor *monitor;

    TRACE( "%s %u %p %#x\n", debugstr_us( device ), index, info, flags );

    if (!lock_display_devices()) return FALSE;

    if (!device || !device->Length)
    {
        /* Enumerate adapters */
        LIST_FOR_EACH_ENTRY(adapter, &adapters, struct adapter, entry)
        {
            if (index == adapter->id)
            {
                found = &adapter->dev;
                break;
            }
        }
    }
    else if ((adapter = find_adapter( device )))
    {
        /* Enumerate monitors */
        LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
        {
            if (monitor->adapter == adapter && index == monitor->id)
            {
                found = &monitor->dev;
                break;
            }
        }
    }

    if (found)
    {
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceName) + sizeof(info->DeviceName))
            lstrcpyW( info->DeviceName, found->device_name );
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceString) + sizeof(info->DeviceString))
            lstrcpyW( info->DeviceString, found->device_string );
        if (info->cb >= offsetof(DISPLAY_DEVICEW, StateFlags) + sizeof(info->StateFlags))
            info->StateFlags = found->state_flags;
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
            lstrcpyW( info->DeviceID, (flags & EDD_GET_DEVICE_INTERFACE_NAME)
                      ? found->interface_name : found->device_id );
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(info->DeviceKey))
            lstrcpyW( info->DeviceKey, found->device_key );
    }
    unlock_display_devices();
    return !!found;
}

#define _X_FIELD(prefix, bits)                              \
    if ((fields) & prefix##_##bits)                         \
    {                                                       \
        p += sprintf( p, "%s%s", first ? "" : ",", #bits ); \
        first = FALSE;                                      \
    }

static const char *_CDS_flags( DWORD fields )
{
    BOOL first = TRUE;
    CHAR buf[128];
    CHAR *p = buf;

    _X_FIELD(CDS, UPDATEREGISTRY)
    _X_FIELD(CDS, TEST)
    _X_FIELD(CDS, FULLSCREEN)
    _X_FIELD(CDS, GLOBAL)
    _X_FIELD(CDS, SET_PRIMARY)
    _X_FIELD(CDS, VIDEOPARAMETERS)
    _X_FIELD(CDS, ENABLE_UNSAFE_MODES)
    _X_FIELD(CDS, DISABLE_UNSAFE_MODES)
    _X_FIELD(CDS, RESET)
    _X_FIELD(CDS, RESET_EX)
    _X_FIELD(CDS, NORESET)

    *p = 0;
    return wine_dbg_sprintf( "%s", buf );
}

static const char *_DM_fields( DWORD fields )
{
    BOOL first = TRUE;
    CHAR buf[128];
    CHAR *p = buf;

    _X_FIELD(DM, BITSPERPEL)
    _X_FIELD(DM, PELSWIDTH)
    _X_FIELD(DM, PELSHEIGHT)
    _X_FIELD(DM, DISPLAYFLAGS)
    _X_FIELD(DM, DISPLAYFREQUENCY)
    _X_FIELD(DM, POSITION)
    _X_FIELD(DM, DISPLAYORIENTATION)

    *p = 0;
    return wine_dbg_sprintf( "%s", buf );
}

#undef _X_FIELD

static void trace_devmode( const DEVMODEW *devmode )
{
    TRACE( "dmFields=%s ", _DM_fields(devmode->dmFields) );
    if (devmode->dmFields & DM_BITSPERPEL)
        TRACE( "dmBitsPerPel=%u ", devmode->dmBitsPerPel );
    if (devmode->dmFields & DM_PELSWIDTH)
        TRACE( "dmPelsWidth=%u ", devmode->dmPelsWidth );
    if (devmode->dmFields & DM_PELSHEIGHT)
        TRACE( "dmPelsHeight=%u ", devmode->dmPelsHeight );
    if (devmode->dmFields & DM_DISPLAYFREQUENCY)
        TRACE( "dmDisplayFrequency=%u ", devmode->dmDisplayFrequency );
    if (devmode->dmFields & DM_POSITION)
        TRACE( "dmPosition=(%d,%d) ", devmode->dmPosition.x, devmode->dmPosition.y );
    if (devmode->dmFields & DM_DISPLAYFLAGS)
        TRACE( "dmDisplayFlags=%#x ", devmode->dmDisplayFlags );
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        TRACE( "dmDisplayOrientation=%u ", devmode->dmDisplayOrientation );
    TRACE("\n");
}

static BOOL is_detached_mode( const DEVMODEW *mode )
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

/***********************************************************************
 *	     NtUserChangeDisplaySettingsExW    (win32u.@)
 */
LONG WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                         DWORD flags, void *lparam )
{
    WCHAR device_name[CCHDEVICENAME];
    struct adapter *adapter;
    BOOL def_mode = TRUE;
    DEVMODEW dm;
    LONG ret;

    TRACE( "%s %p %p %#x %p\n", debugstr_us(devname), devmode, hwnd, flags, lparam );
    TRACE( "flags=%s\n", _CDS_flags(flags) );

    if ((!devname || !devname->Length) && !devmode)
    {
        ret = user_driver->pChangeDisplaySettingsEx( NULL, NULL, hwnd, flags, lparam );
        if (ret != DISP_CHANGE_SUCCESSFUL)
            ERR( "Restoring all displays to their registry settings returned %d.\n", ret );
        return ret;
    }

    if (!lock_display_devices()) return FALSE;
    if ((adapter = find_adapter( devname ))) lstrcpyW( device_name, adapter->dev.device_name );
    unlock_display_devices();
    if (!adapter)
    {
        WARN( "Invalid device name %s.\n", debugstr_us(devname) );
        return DISP_CHANGE_BADPARAM;
    }

    if (devmode)
    {
        trace_devmode( devmode );

        if (devmode->dmSize < FIELD_OFFSET(DEVMODEW, dmICMMethod))
            return DISP_CHANGE_BADMODE;

        if (is_detached_mode(devmode) ||
            ((devmode->dmFields & DM_BITSPERPEL) && devmode->dmBitsPerPel) ||
            ((devmode->dmFields & DM_PELSWIDTH) && devmode->dmPelsWidth) ||
            ((devmode->dmFields & DM_PELSHEIGHT) && devmode->dmPelsHeight) ||
            ((devmode->dmFields & DM_DISPLAYFREQUENCY) && devmode->dmDisplayFrequency))
            def_mode = FALSE;
    }

    if (def_mode)
    {
        memset( &dm, 0, sizeof(dm) );
        dm.dmSize = sizeof(dm);
        if (!NtUserEnumDisplaySettings( devname, ENUM_REGISTRY_SETTINGS, &dm, 0 ))
        {
            ERR( "Default mode not found!\n" );
            return DISP_CHANGE_BADMODE;
        }

        TRACE( "Return to original display mode\n" );
        devmode = &dm;
    }

    if ((devmode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN( "devmode doesn't specify the resolution: %#x\n", devmode->dmFields );
        return DISP_CHANGE_BADMODE;
    }

    if (!is_detached_mode(devmode) && (!devmode->dmPelsWidth || !devmode->dmPelsHeight))
    {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (!NtUserEnumDisplaySettings( devname, ENUM_CURRENT_SETTINGS, &dm, 0 ))
        {
            ERR( "Current mode not found!\n" );
            return DISP_CHANGE_BADMODE;
        }

        if (!devmode->dmPelsWidth)
            devmode->dmPelsWidth = dm.dmPelsWidth;
        if (!devmode->dmPelsHeight)
            devmode->dmPelsHeight = dm.dmPelsHeight;
    }

    ret = user_driver->pChangeDisplaySettingsEx( device_name, devmode, hwnd, flags, lparam );
    if (ret != DISP_CHANGE_SUCCESSFUL)
        ERR( "Changing %s display settings returned %d.\n", debugstr_us(devname), ret );
    return ret;
}

/***********************************************************************
 *	     NtUserEnumDisplaySettings    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD mode,
                                       DEVMODEW *dev_mode, DWORD flags )
{
    WCHAR device_name[CCHDEVICENAME];
    struct adapter *adapter;
    BOOL ret;

    TRACE( "%s %#x %p %#x\n", debugstr_us(device), mode, dev_mode, flags );

    if (!lock_display_devices()) return FALSE;
    if ((adapter = find_adapter( device ))) lstrcpyW( device_name, adapter->dev.device_name );
    unlock_display_devices();
    if (!adapter)
    {
        WARN( "Invalid device name %s.\n", debugstr_us(device) );
        return FALSE;
    }

    ret = user_driver->pEnumDisplaySettingsEx( device_name, mode, dev_mode, flags );
    if (ret)
        TRACE( "device:%s mode index:%#x position:(%d,%d) resolution:%ux%u frequency:%uHz "
               "depth:%ubits orientation:%#x.\n", debugstr_w(device_name), mode,
               dev_mode->dmPosition.x, dev_mode->dmPosition.y, dev_mode->dmPelsWidth,
               dev_mode->dmPelsHeight, dev_mode->dmDisplayFrequency, dev_mode->dmBitsPerPel,
               dev_mode->dmDisplayOrientation );
    else
        WARN( "Failed to query %s display settings.\n", wine_dbgstr_w(device_name) );
    return ret;
}

struct monitor_enum_info
{
    HANDLE handle;
    RECT rect;
};

/***********************************************************************
 *	     NtUserEnumDisplayMonitors    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lparam )
{
    struct monitor_enum_info enum_buf[8], *enum_info = enum_buf;
    struct enum_display_monitor_params params;
    struct monitor *monitor;
    unsigned int count = 0, i;
    BOOL ret = TRUE;

    if (!lock_display_devices()) return FALSE;

    count = list_count( &monitors );
    if (!count || (count > ARRAYSIZE(enum_buf) &&
                   !(enum_info = malloc( count * sizeof(*enum_info) ))))
    {
        unlock_display_devices();
        return FALSE;
    }

    count = 0;
    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        if (!(monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE)) continue;
        enum_info[count].handle = monitor->handle;
        enum_info[count].rect = monitor->rc_monitor;
        count++;
    }

    unlock_display_devices();

    params.proc = proc;
    params.hdc = hdc;
    params.lparam = lparam;
    for (i = 0; i < count; i++)
    {
        params.monitor = enum_info[i].handle;
        params.rect = enum_info[i].rect;
        if (!user32_call( NtUserCallEnumDisplayMonitor, &params, sizeof(params) ))
        {
            ret = FALSE;
            break;
        }
    }
    if (enum_info != enum_buf) free( enum_info );
    return ret;
}

static BOOL get_monitor_info( HMONITOR handle, MONITORINFO *info )
{
    struct monitor *monitor;

    if (info->cbSize != sizeof(MONITORINFOEXW) && info->cbSize != sizeof(MONITORINFO)) return FALSE;

    if (!lock_display_devices()) return FALSE;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (monitor->handle != handle) continue;
        if (!(monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE)) break;

        /* FIXME: map dpi */
        info->rcMonitor = monitor->rc_monitor;
        info->rcWork = monitor->rc_work;
        info->dwFlags = monitor->flags;
        if (info->cbSize >= sizeof(MONITORINFOEXW))
        {
            if (monitor->adapter)
                lstrcpyW( ((MONITORINFOEXW *)info)->szDevice, monitor->adapter->dev.device_name );
            else
                asciiz_to_unicode( ((MONITORINFOEXW *)info)->szDevice, "WinDisc" );
        }
        unlock_display_devices();
        return TRUE;
    }

    unlock_display_devices();
    WARN( "invalid handle %p\n", handle );
    SetLastError( ERROR_INVALID_MONITOR_HANDLE );
    return FALSE;
}

/**********************************************************************
 *	     sysparams_init
 */
void sysparams_init(void)
{
    WCHAR layout[KL_NAMELENGTH];
    HKEY hkey;

    static const WCHAR oneW[] = {'1',0};
    static const WCHAR kl_preloadW[] =
        {'K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','\\','P','r','e','l','o','a','d'};

    if ((hkey = reg_create_key( hkcu_key, kl_preloadW, sizeof(kl_preloadW), 0, NULL )))
    {
        if (NtUserGetKeyboardLayoutName( layout ))
            set_reg_value( hkey, oneW, REG_SZ, (const BYTE *)layout,
                           (lstrlenW(layout) + 1) * sizeof(WCHAR) );
        NtClose( hkey );
    }
}

/***********************************************************************
 *	     NtUserCallOneParam    (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code )
{
    switch(code)
    {
    case NtUserRealizePalette:
        return realize_palette( UlongToHandle(arg) );
    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}

/***********************************************************************
 *	     NtUserCallTwoParam    (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallTwoParam( ULONG_PTR arg1, ULONG_PTR arg2, ULONG code )
{
    switch(code)
    {
    case NtUserGetMonitorInfo:
        return get_monitor_info( UlongToHandle(arg1), (MONITORINFO *)arg2 );
    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}
