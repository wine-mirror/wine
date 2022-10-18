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
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "devpropdef.h"
#include "wine/wingdi16.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);


static HKEY video_key, enum_key, control_key, config_key, volatile_base_key;

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
static const WCHAR yesW[] = {'Y','e','s',0};
static const WCHAR noW[] = {'N','o',0};
static const WCHAR bits_per_pelW[] = {'B','i','t','s','P','e','r','P','e','l',0};
static const WCHAR x_resolutionW[] = {'X','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR y_resolutionW[] = {'Y','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR v_refreshW[] = {'V','R','e','f','r','e','s','h',0};
static const WCHAR flagsW[] = {'F','l','a','g','s',0};
static const WCHAR x_panningW[] = {'X','P','a','n','n','i','n','g',0};
static const WCHAR y_panningW[] = {'Y','P','a','n','n','i','n','g',0};
static const WCHAR orientationW[] = {'O','r','i','e','n','t','a','t','i','o','n',0};
static const WCHAR fixed_outputW[] = {'F','i','x','e','d','O','u','t','p','u','t',0};
static const WCHAR mode_countW[] = {'M','o','d','e','C','o','u','n','t',0};

static const char  guid_devclass_displayA[] = "{4D36E968-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_displayW[] =
    {'{','4','D','3','6','E','9','6','8','-','E','3','2','5','-','1','1','C','E','-',
     'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}',0};

static const char  guid_devclass_monitorA[] = "{4D36E96E-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_monitorW[] =
    {'{','4','D','3','6','E','9','6','E','-','E','3','2','5','-','1','1','C','E','-',
     'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}',0};

static const WCHAR guid_devinterface_display_adapterW[] =
    {'{','5','B','4','5','2','0','1','D','-','F','2','F','2','-','4','F','3','B','-',
     '8','5','B','B','-','3','0','F','F','1','F','9','5','3','5','9','9','}',0};

static const WCHAR guid_display_device_arrivalW[] =
    {'{','1','C','A','0','5','1','8','0','-','A','6','9','9','-','4','5','0','A','-',
     '9','A','0','C','-','D','E','4','F','B','E','3','D','D','D','8','9','}',0};

static const WCHAR guid_devinterface_monitorW[] =
    {'{','E','6','F','0','7','B','5','F','-','E','E','9','7','-','4','A','9','0','-',
     'B','0','7','6','-','3','3','F','5','7','B','F','4','E','A','A','7','}',0};

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

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
    LONG refcount;
    struct list entry;
    struct display_device dev;
    unsigned int id;
    const WCHAR *config_key;
    unsigned int mode_count;
    DEVMODEW *modes;
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
    BOOL is_clone;
};

static struct list adapters = LIST_INIT(adapters);
static struct list monitors = LIST_INIT(monitors);
static INT64 last_query_display_time;
static pthread_mutex_t display_lock = PTHREAD_MUTEX_INITIALIZER;

BOOL enable_thunk_lock = FALSE;

#define VIRTUAL_HMONITOR ((HMONITOR)(UINT_PTR)(0x10000 + 1))
static struct monitor virtual_monitor =
{
    .handle = VIRTUAL_HMONITOR,
    .flags = MONITORINFOF_PRIMARY,
    .rc_monitor.right = 1024,
    .rc_monitor.bottom = 768,
    .rc_work.right = 1024,
    .rc_work.bottom = 768,
    .dev.state_flags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED,
};

/* the various registry keys that are used to store parameters */
enum parameter_key
{
    COLORS_KEY,
    DESKTOP_KEY,
    KEYBOARD_KEY,
    MOUSE_KEY,
    METRICS_KEY,
    SOUND_KEY,
    VERSION_KEY,
    SHOWSOUNDS_KEY,
    KEYBOARDPREF_KEY,
    SCREENREADER_KEY,
    AUDIODESC_KEY,
    NB_PARAM_KEYS
};

static const char *parameter_key_names[NB_PARAM_KEYS] =
{
    "Control Panel\\Colors",
    "Control Panel\\Desktop",
    "Control Panel\\Keyboard",
    "Control Panel\\Mouse",
    "Control Panel\\Desktop\\WindowMetrics",
    "Control Panel\\Sound",
    "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
    "Control Panel\\Accessibility\\ShowSounds",
    "Control Panel\\Accessibility\\Keyboard Preference",
    "Control Panel\\Accessibility\\Blind Access",
    "Control Panel\\Accessibility\\AudioDescription",
};

/* System parameters storage */
union sysparam_all_entry;

struct sysparam_entry
{
    BOOL             (*get)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi );
    BOOL             (*set)( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags );
    BOOL             (*init)( union sysparam_all_entry *entry );
    enum parameter_key base_key;
    const char        *regval;
    enum parameter_key mirror_key;
    const char        *mirror;
    BOOL               loaded;
};

struct sysparam_uint_entry
{
    struct sysparam_entry hdr;
    UINT                  val;
};

struct sysparam_bool_entry
{
    struct sysparam_entry hdr;
    BOOL                  val;
};

struct sysparam_dword_entry
{
    struct sysparam_entry hdr;
    DWORD                 val;
};

struct sysparam_rgb_entry
{
    struct sysparam_entry hdr;
    COLORREF              val;
    HBRUSH                brush;
    HPEN                  pen;
};

struct sysparam_binary_entry
{
    struct sysparam_entry hdr;
    void                 *ptr;
    size_t                size;
};

struct sysparam_path_entry
{
    struct sysparam_entry hdr;
    WCHAR                 path[MAX_PATH];
};

struct sysparam_font_entry
{
    struct sysparam_entry hdr;
    UINT                  weight;
    LOGFONTW              val;
    WCHAR                 fullname[LF_FACESIZE];
};

struct sysparam_pref_entry
{
    struct sysparam_entry hdr;
    struct sysparam_binary_entry *parent;
    UINT                  offset;
    UINT                  mask;
};

union sysparam_all_entry
{
    struct sysparam_entry        hdr;
    struct sysparam_uint_entry   uint;
    struct sysparam_bool_entry   bool;
    struct sysparam_dword_entry  dword;
    struct sysparam_rgb_entry    rgb;
    struct sysparam_binary_entry bin;
    struct sysparam_path_entry   path;
    struct sysparam_font_entry   font;
    struct sysparam_pref_entry   pref;
};

static UINT system_dpi;
static RECT work_area;
static DWORD process_layout = ~0u;

static HDC display_dc;
static pthread_mutex_t display_dc_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t user_mutex;
static unsigned int user_lock_thread, user_lock_rec;

void user_lock(void)
{
    pthread_mutex_lock( &user_mutex );
    if (!user_lock_rec++) user_lock_thread = GetCurrentThreadId();
}

void user_unlock(void)
{
    if (!--user_lock_rec) user_lock_thread = 0;
    pthread_mutex_unlock( &user_mutex );
}

void user_check_not_lock(void)
{
    if (user_lock_thread == GetCurrentThreadId())
    {
        ERR( "BUG: holding USER lock\n" );
        assert( 0 );
    }
}

static HANDLE get_display_device_init_mutex( void )
{
    WCHAR bufferW[256];
    UNICODE_STRING name = {.Buffer = bufferW};
    OBJECT_ATTRIBUTES attr;
    char buffer[256];
    HANDLE mutex;

    snprintf( buffer, ARRAY_SIZE(buffer), "\\Sessions\\%u\\BaseNamedObjects\\display_device_init",
              NtCurrentTeb()->Peb->SessionId );
    name.Length = name.MaximumLength = asciiz_to_unicode( bufferW, buffer );

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

static struct adapter *adapter_acquire( struct adapter *adapter )
{
    InterlockedIncrement( &adapter->refcount );
    return adapter;
}

static void adapter_release( struct adapter *adapter )
{
    if (!InterlockedDecrement( &adapter->refcount ))
    {
        free( adapter->modes );
        free( adapter );
    }
}

static BOOL write_adapter_mode( HKEY adapter_key, DWORD index, const DEVMODEW *mode )
{
    WCHAR bufferW[MAX_PATH];
    char buffer[MAX_PATH];
    BOOL ret = TRUE;
    HKEY hkey;

    sprintf( buffer, "Modes\\%08X", index );
    reg_delete_tree( adapter_key, bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR) );
    if (!(hkey = reg_create_key( adapter_key, bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR),
                                 REG_OPTION_VOLATILE, NULL )))
        return FALSE;

#define set_mode_field( name, field, flag )                                                      \
    if ((mode->dmFields & (flag)) &&                                                             \
        !(ret = set_reg_value( hkey, (name), REG_DWORD, &mode->field,                            \
                               sizeof(mode->field) )))                                           \
        goto done

    set_mode_field( bits_per_pelW, dmBitsPerPel, DM_BITSPERPEL );
    set_mode_field( x_resolutionW, dmPelsWidth, DM_PELSWIDTH );
    set_mode_field( y_resolutionW, dmPelsHeight, DM_PELSHEIGHT );
    set_mode_field( v_refreshW, dmDisplayFrequency, DM_DISPLAYFREQUENCY );
    set_mode_field( flagsW, dmDisplayFlags, DM_DISPLAYFLAGS );
    set_mode_field( orientationW, dmDisplayOrientation, DM_DISPLAYORIENTATION );
    set_mode_field( fixed_outputW, dmDisplayFixedOutput, DM_DISPLAYFIXEDOUTPUT );
    if (index == ENUM_CURRENT_SETTINGS || index == ENUM_REGISTRY_SETTINGS)
    {
        set_mode_field( x_panningW, dmPosition.x, DM_POSITION );
        set_mode_field( y_panningW, dmPosition.y, DM_POSITION );
    }

#undef set_mode_field

done:
    NtClose( hkey );
    return ret;
}

static BOOL read_adapter_mode( HKEY adapter_key, DWORD index, DEVMODEW *mode )
{
    char value_buf[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)value_buf;
    WCHAR bufferW[MAX_PATH];
    char buffer[MAX_PATH];
    HKEY hkey;

    sprintf( buffer, "Modes\\%08X", index );
    if (!(hkey = reg_open_key( adapter_key, bufferW, asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR) )))
        return FALSE;

#define query_mode_field( name, field, flag )                                                      \
    do                                                                                             \
    {                                                                                              \
        if (query_reg_value( hkey, (name), value, sizeof(value_buf) ) &&                           \
            value->Type == REG_DWORD)                                                              \
        {                                                                                          \
            mode->field = *(const DWORD *)value->Data;                                             \
            mode->dmFields |= (flag);                                                              \
        }                                                                                          \
    } while (0)

    query_mode_field( bits_per_pelW, dmBitsPerPel, DM_BITSPERPEL );
    query_mode_field( x_resolutionW, dmPelsWidth, DM_PELSWIDTH );
    query_mode_field( y_resolutionW, dmPelsHeight, DM_PELSHEIGHT );
    query_mode_field( v_refreshW, dmDisplayFrequency, DM_DISPLAYFREQUENCY );
    query_mode_field( flagsW, dmDisplayFlags, DM_DISPLAYFLAGS );
    query_mode_field( orientationW, dmDisplayOrientation, DM_DISPLAYORIENTATION );
    query_mode_field( fixed_outputW, dmDisplayFixedOutput, DM_DISPLAYFIXEDOUTPUT );
    if (index == ENUM_CURRENT_SETTINGS || index == ENUM_REGISTRY_SETTINGS)
    {
        query_mode_field( x_panningW, dmPosition.x, DM_POSITION );
        query_mode_field( y_panningW, dmPosition.y, DM_POSITION );
    }

#undef query_mode_field

    NtClose( hkey );
    return TRUE;
}

static BOOL adapter_get_registry_settings( const struct adapter *adapter, DEVMODEW *mode )
{
    BOOL ret = FALSE;
    HANDLE mutex;
    HKEY hkey;

    mutex = get_display_device_init_mutex();

    if (!config_key && !(config_key = reg_open_key( NULL, config_keyW, sizeof(config_keyW) ))) ret = FALSE;
    else if (!(hkey = reg_open_key( config_key, adapter->config_key, lstrlenW( adapter->config_key ) * sizeof(WCHAR) ))) ret = FALSE;
    else
    {
        ret = read_adapter_mode( hkey, ENUM_REGISTRY_SETTINGS, mode );
        NtClose( hkey );
    }

    release_display_device_init_mutex( mutex );
    return ret;
}

static BOOL adapter_set_registry_settings( const struct adapter *adapter, const DEVMODEW *mode )
{
    HANDLE mutex;
    HKEY hkey;
    BOOL ret;

    mutex = get_display_device_init_mutex();

    if (!config_key && !(config_key = reg_open_key( NULL, config_keyW, sizeof(config_keyW) ))) ret = FALSE;
    if (!(hkey = reg_open_key( config_key, adapter->config_key, lstrlenW( adapter->config_key ) * sizeof(WCHAR) ))) ret = FALSE;
    else
    {
        ret = write_adapter_mode( hkey, ENUM_REGISTRY_SETTINGS, mode );
        NtClose( hkey );
    }

    release_display_device_init_mutex( mutex );
    return ret;
}

static int mode_compare(const void *p1, const void *p2)
{
    BOOL a_interlaced, b_interlaced, a_stretched, b_stretched;
    DWORD a_width, a_height, b_width, b_height;
    const DEVMODEW *a = p1, *b = p2;
    int ret;

    /* Depth in descending order */
    if ((ret = b->dmBitsPerPel - a->dmBitsPerPel)) return ret;

    /* Use the width and height in landscape mode for comparison */
    if (a->dmDisplayOrientation == DMDO_DEFAULT || a->dmDisplayOrientation == DMDO_180)
    {
        a_width = a->dmPelsWidth;
        a_height = a->dmPelsHeight;
    }
    else
    {
        a_width = a->dmPelsHeight;
        a_height = a->dmPelsWidth;
    }

    if (b->dmDisplayOrientation == DMDO_DEFAULT || b->dmDisplayOrientation == DMDO_180)
    {
        b_width = b->dmPelsWidth;
        b_height = b->dmPelsHeight;
    }
    else
    {
        b_width = b->dmPelsHeight;
        b_height = b->dmPelsWidth;
    }

    /* Width in ascending order */
    if ((ret = a_width - b_width)) return ret;

    /* Height in ascending order */
    if ((ret = a_height - b_height)) return ret;

    /* Frequency in descending order */
    if ((ret = b->dmDisplayFrequency - a->dmDisplayFrequency)) return ret;

    /* Orientation in ascending order */
    if ((ret = a->dmDisplayOrientation - b->dmDisplayOrientation)) return ret;

    if (!(a->dmFields & DM_DISPLAYFLAGS)) a_interlaced = FALSE;
    else a_interlaced = !!(a->dmDisplayFlags & DM_INTERLACED);
    if (!(b->dmFields & DM_DISPLAYFLAGS)) b_interlaced = FALSE;
    else b_interlaced = !!(b->dmDisplayFlags & DM_INTERLACED);

    /* Interlaced in ascending order */
    if ((ret = a_interlaced - b_interlaced)) return ret;

    if (!(a->dmFields & DM_DISPLAYFIXEDOUTPUT)) a_stretched = FALSE;
    else a_stretched = a->dmDisplayFixedOutput == DMDFO_STRETCH;
    if (!(b->dmFields & DM_DISPLAYFIXEDOUTPUT)) b_stretched = FALSE;
    else b_stretched = b->dmDisplayFixedOutput == DMDFO_STRETCH;

    /* Stretched in ascending order */
    if ((ret = a_stretched - b_stretched)) return ret;

    return 0;
}

static BOOL read_display_adapter_settings( unsigned int index, struct adapter *info )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    DEVMODEW *mode;
    DWORD i, size;
    HKEY hkey;

    if (!enum_key && !(enum_key = reg_open_key( NULL, enum_keyW, sizeof(enum_keyW) )))
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

    /* ModeCount */
    if (query_reg_value( hkey, mode_countW, value, sizeof(buffer) ) && value->Type == REG_DWORD)
        info->mode_count = *(const DWORD *)value->Data;

    /* Modes, allocate an extra mode for easier iteration */
    if ((info->modes = calloc( info->mode_count + 1, sizeof(DEVMODEW) )))
    {
        for (i = 0, mode = info->modes; i < info->mode_count; i++)
        {
            mode->dmSize = offsetof(DEVMODEW, dmICMMethod);
            if (!read_adapter_mode( hkey, i, mode )) break;
            mode = NEXT_DEVMODEW(mode);
        }
        info->mode_count = i;

        qsort(info->modes, info->mode_count, sizeof(*info->modes) + info->modes->dmDriverExtra, mode_compare);
    }

    /* DeviceID */
    size = query_reg_value( hkey, gpu_idW, value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ || !info->mode_count || !info->modes) return FALSE;

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
                j = 0;
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
    unsigned int mode_count;
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

    set_reg_value( hkey, device_instanceW, REG_SZ, instance, (instance_len + 1) * sizeof(WCHAR) );

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
    ctx->mode_count = 0;

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
                     (unsigned int)guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
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

    if (ctx->adapter_key)
    {
        NtClose( ctx->adapter_key );
        ctx->adapter_key = NULL;
    }

    adapter_index = ctx->adapter_count++;
    video_index = ctx->video_count++;
    ctx->monitor_count = 0;
    ctx->mode_count = 0;

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
        static const WCHAR bad_edidW[] = {'B','A','D','_','E','D','I','D',0};
        static const WCHAR edidW[] = {'E','D','I','D',0};

        if (monitor->edid_len)
            set_reg_value( subkey, edidW, REG_BINARY, monitor->edid, monitor->edid_len );
        else
            set_reg_value( subkey, bad_edidW, REG_BINARY, NULL, 0 );
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

static void add_mode( const DEVMODEW *mode, void *param )
{
    struct device_manager_ctx *ctx = param;

    if (!ctx->adapter_count)
    {
        static const struct gdi_adapter default_adapter =
        {
            .state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE,
        };
        TRACE( "adding default fake adapter\n" );
        add_adapter( &default_adapter, ctx );
    }

    if (write_adapter_mode( ctx->adapter_key, ctx->mode_count, mode ))
    {
        ctx->mode_count++;
        set_reg_value( ctx->adapter_key, mode_countW, REG_DWORD, &ctx->mode_count, sizeof(ctx->mode_count) );
    }
}

static const struct gdi_device_manager device_manager =
{
    add_gpu,
    add_adapter,
    add_monitor,
    add_mode,
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
        adapter_release( monitor->adapter );
        list_remove( &monitor->entry );
        free( monitor );
    }

    while (!list_empty( &adapters ))
    {
        adapter = LIST_ENTRY( list_head( &adapters ), struct adapter, entry );
        list_remove( &adapter->entry );
        adapter_release( adapter );
    }
}

static BOOL update_display_cache_from_registry(void)
{
    DWORD adapter_id, monitor_id, monitor_count = 0, size;
    KEY_BASIC_INFORMATION key;
    struct adapter *adapter;
    struct monitor *monitor, *monitor2;
    HANDLE mutex = NULL;
    NTSTATUS status;
    BOOL ret;

    /* If user driver did initialize the registry, then exit */
    if (!video_key && !(video_key = reg_open_key( NULL, devicemap_video_keyW,
                                                  sizeof(devicemap_video_keyW) )))
        return FALSE;

    status = NtQueryKey( video_key, KeyBasicInformation, &key,
                         offsetof(KEY_BASIC_INFORMATION, Name), &size );
    if (status && status != STATUS_BUFFER_OVERFLOW)
        return FALSE;

    if (key.LastWriteTime.QuadPart <= last_query_display_time) return TRUE;

    mutex = get_display_device_init_mutex();
    pthread_mutex_lock( &display_lock );

    clear_display_devices();

    for (adapter_id = 0;; adapter_id++)
    {
        if (!(adapter = calloc( 1, sizeof(*adapter) ))) break;
        adapter->refcount = 1;
        adapter->id = adapter_id;

        if (!read_display_adapter_settings( adapter_id, adapter ))
        {
            adapter_release( adapter );
            break;
        }

        list_add_tail( &adapters, &adapter->entry );
        for (monitor_id = 0;; monitor_id++)
        {
            if (!(monitor = calloc( 1, sizeof(*monitor) ))) break;
            if (!read_monitor_settings( adapter, monitor_id, monitor ))
            {
                free( monitor );
                break;
            }

            monitor->id = monitor_id;
            monitor->adapter = adapter_acquire( adapter );

            LIST_FOR_EACH_ENTRY(monitor2, &monitors, struct monitor, entry)
            {
                if (EqualRect(&monitor2->rc_monitor, &monitor->rc_monitor))
                {
                    monitor->is_clone = TRUE;
                    break;
                }
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
    HWINSTA winstation = NtUserGetProcessWindowStation();
    struct device_manager_ctx ctx = {0};
    USEROBJECTFLAGS flags;

    /* services do not have any adapters, only a virtual monitor */
    if (NtUserGetObjectInformation( winstation, UOI_FLAGS, &flags, sizeof(flags), NULL )
        && !(flags.dwFlags & WSF_VISIBLE))
    {
        pthread_mutex_lock( &display_lock );
        clear_display_devices();
        list_add_tail( &monitors, &virtual_monitor.entry );
        pthread_mutex_unlock( &display_lock );
        return TRUE;
    }

    user_driver->pUpdateDisplayDevices( &device_manager, FALSE, &ctx );
    release_display_manager_ctx( &ctx );

    if (update_display_cache_from_registry()) return TRUE;
    if (ctx.gpu_count)
    {
        ERR( "driver reported devices, but we failed to read them\n" );
        return FALSE;
    }

    if (!user_driver->pUpdateDisplayDevices( &device_manager, TRUE, &ctx ))
    {
        static const DEVMODEW modes[] =
        {
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 32, .dmPelsWidth = 640, .dmPelsHeight = 480, .dmDisplayFrequency = 60, },
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 32, .dmPelsWidth = 800, .dmPelsHeight = 600, .dmDisplayFrequency = 60, },
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 32, .dmPelsWidth = 1024, .dmPelsHeight = 768, .dmDisplayFrequency = 60, },
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 16, .dmPelsWidth = 640, .dmPelsHeight = 480, .dmDisplayFrequency = 60, },
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 16, .dmPelsWidth = 800, .dmPelsHeight = 600, .dmDisplayFrequency = 60, },
            { .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
              .dmBitsPerPel = 16, .dmPelsWidth = 1024, .dmPelsHeight = 768, .dmDisplayFrequency = 60, },
        };
        static const struct gdi_gpu gpu;
        static const struct gdi_adapter adapter =
        {
            .state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE,
        };
        struct gdi_monitor monitor =
        {
            .state_flags = DISPLAY_DEVICE_ACTIVE | DISPLAY_DEVICE_ATTACHED,
            .rc_monitor = {.right = modes[2].dmPelsWidth, .bottom = modes[2].dmPelsHeight},
            .rc_work = {.right = modes[2].dmPelsWidth, .bottom = modes[2].dmPelsHeight},
        };
        UINT i;

        add_gpu( &gpu, &ctx );
        add_adapter( &adapter, &ctx );
        add_monitor( &monitor, &ctx );
        for (i = 0; i < ARRAY_SIZE(modes); ++i) add_mode( modes + i, &ctx );
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

static HDC get_display_dc(void)
{
    pthread_mutex_lock( &display_dc_lock );
    if (!display_dc)
    {
        HDC dc;

        pthread_mutex_unlock( &display_dc_lock );
        dc = NtGdiOpenDCW( NULL, NULL, NULL, 0, TRUE, NULL, NULL, NULL );
        pthread_mutex_lock( &display_dc_lock );
        if (display_dc)
            NtGdiDeleteObjectApp( dc );
        else
            display_dc = dc;
    }
    return display_dc;
}

static void release_display_dc( HDC hdc )
{
    pthread_mutex_unlock( &display_dc_lock );
}

/**********************************************************************
 *           get_monitor_dpi
 */
UINT get_monitor_dpi( HMONITOR monitor )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *              get_win_monitor_dpi
 */
UINT get_win_monitor_dpi( HWND hwnd )
{
    /* FIXME: use the monitor DPI instead */
    return system_dpi;
}

/**********************************************************************
 *           get_thread_dpi_awareness
 */
DPI_AWARENESS get_thread_dpi_awareness(void)
{
    struct ntuser_thread_info *info = NtUserGetThreadInfo();
    ULONG_PTR context = info->dpi_awareness;

    if (!context) context = NtUserGetProcessDpiAwarenessContext( NULL );

    switch (context)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x80000010:
    case 0x80000011:
    case 0x80000012:
        return context & 3;
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
        return ~context;
    default:
        return DPI_AWARENESS_INVALID;
    }
}

DWORD get_process_layout(void)
{
    return process_layout == ~0u ? 0 : process_layout;
}

/**********************************************************************
 *              get_thread_dpi
 */
UINT get_thread_dpi(void)
{
    switch (get_thread_dpi_awareness())
    {
    case DPI_AWARENESS_UNAWARE:      return USER_DEFAULT_SCREEN_DPI;
    case DPI_AWARENESS_SYSTEM_AWARE: return system_dpi;
    default:                         return 0;  /* no scaling */
    }
}

/* see GetDpiForSystem */
UINT get_system_dpi(void)
{
    if (get_thread_dpi_awareness() == DPI_AWARENESS_UNAWARE) return USER_DEFAULT_SCREEN_DPI;
    return system_dpi;
}

/* see GetAwarenessFromDpiAwarenessContext */
static DPI_AWARENESS get_awareness_from_dpi_awareness_context( DPI_AWARENESS_CONTEXT context )
{
    switch ((ULONG_PTR)context)
    {
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x80000010:
    case 0x80000011:
    case 0x80000012:
        return (ULONG_PTR)context & 3;
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_UNAWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
    case (ULONG_PTR)DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
        return ~(ULONG_PTR)context;
    default:
        return DPI_AWARENESS_INVALID;
    }
}

/**********************************************************************
 *           SetThreadDpiAwarenessContext   (win32u.so)
 */
DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT context )
{
    struct ntuser_thread_info *info = NtUserGetThreadInfo();
    DPI_AWARENESS prev, val = get_awareness_from_dpi_awareness_context( context );

    if (val == DPI_AWARENESS_INVALID)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(prev = info->dpi_awareness))
    {
        prev = NtUserGetProcessDpiAwarenessContext( GetCurrentProcess() ) & 3;
        prev |= 0x80000010;  /* restore to process default */
    }
    if (((ULONG_PTR)context & ~(ULONG_PTR)0x13) == 0x80000000) info->dpi_awareness = 0;
    else info->dpi_awareness = val | 0x10;
    return ULongToHandle( prev );
}

/**********************************************************************
 *              map_dpi_rect
 */
RECT map_dpi_rect( RECT rect, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        rect.left   = muldiv( rect.left, dpi_to, dpi_from );
        rect.top    = muldiv( rect.top, dpi_to, dpi_from );
        rect.right  = muldiv( rect.right, dpi_to, dpi_from );
        rect.bottom = muldiv( rect.bottom, dpi_to, dpi_from );
    }
    return rect;
}

/**********************************************************************
 *              map_dpi_point
 */
POINT map_dpi_point( POINT pt, UINT dpi_from, UINT dpi_to )
{
    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        pt.x = muldiv( pt.x, dpi_to, dpi_from );
        pt.y = muldiv( pt.y, dpi_to, dpi_from );
    }
    return pt;
}

/**********************************************************************
 *              point_win_to_phys_dpi
 */
static POINT point_win_to_phys_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, get_dpi_for_window( hwnd ), get_win_monitor_dpi( hwnd ) );
}

/**********************************************************************
 *              point_phys_to_win_dpi
 */
POINT point_phys_to_win_dpi( HWND hwnd, POINT pt )
{
    return map_dpi_point( pt, get_win_monitor_dpi( hwnd ), get_dpi_for_window( hwnd ));
}

/**********************************************************************
 *              point_thread_to_win_dpi
 */
POINT point_thread_to_win_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_point( pt, dpi, get_dpi_for_window( hwnd ));
}

/**********************************************************************
 *              rect_thread_to_win_dpi
 */
RECT rect_thread_to_win_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi();
    if (!dpi) dpi = get_win_monitor_dpi( hwnd );
    return map_dpi_rect( rect, dpi, get_dpi_for_window( hwnd ) );
}

/* map value from system dpi to standard 96 dpi for storing in the registry */
static int map_from_system_dpi( int val )
{
    return muldiv( val, USER_DEFAULT_SCREEN_DPI, get_system_dpi() );
}

/* map value from 96 dpi to system or custom dpi */
static int map_to_dpi( int val, UINT dpi )
{
    if (!dpi) dpi = get_system_dpi();
    return muldiv( val, dpi, USER_DEFAULT_SCREEN_DPI );
}

RECT get_virtual_screen_rect( UINT dpi )
{
    struct monitor *monitor;
    RECT rect = {0};

    if (!lock_display_devices()) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        union_rect( &rect, &rect, &monitor->rc_monitor );
    }

    unlock_display_devices();

    if (dpi) rect = map_dpi_rect( rect, system_dpi, dpi );
    return rect;
}

static BOOL is_window_rect_full_screen( const RECT *rect )
{
    struct monitor *monitor;
    BOOL ret = FALSE;

    if (!lock_display_devices()) return FALSE;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (rect->left <= monitor->rc_monitor.left && rect->right >= monitor->rc_monitor.right &&
            rect->top <= monitor->rc_monitor.top && rect->bottom >= monitor->rc_monitor.bottom)
        {
            ret = TRUE;
            break;
        }
    }

    unlock_display_devices();
    return ret;
}

RECT get_display_rect( const WCHAR *display )
{
    struct monitor *monitor;
    RECT rect = {0};

    if (!lock_display_devices()) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!monitor->adapter || wcsicmp( monitor->adapter->dev.device_name, display )) continue;
        rect = monitor->rc_monitor;
        break;
    }

    unlock_display_devices();
    return map_dpi_rect( rect, system_dpi, get_thread_dpi() );
}

RECT get_primary_monitor_rect( UINT dpi )
{
    struct monitor *monitor;
    RECT rect = {0};

    if (!lock_display_devices()) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!(monitor->flags & MONITORINFOF_PRIMARY)) continue;
        rect = monitor->rc_monitor;
        break;
    }

    unlock_display_devices();
    return map_dpi_rect( rect, system_dpi, dpi );
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

    if (lock_display_devices())
    {
        LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
        {
            if (!(monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE))
                continue;
            count++;
        }
        unlock_display_devices();
    }

    *num_path_info = count;
    *num_mode_info = count * 2;
    TRACE( "returning %u paths %u modes\n", *num_path_info, *num_mode_info );
    return ERROR_SUCCESS;
}

/* display_lock mutex must be held */
static struct display_device *find_monitor_device( struct display_device *adapter, DWORD index )
{
    struct monitor *monitor;

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
        if (&monitor->adapter->dev == adapter && index == monitor->id)
            return &monitor->dev;

    WARN( "Failed to find adapter %s monitor with id %u.\n", debugstr_w(adapter->device_name), index );
    return NULL;
}

/* display_lock mutex must be held */
static struct display_device *find_adapter_device_by_id( DWORD index )
{
    struct adapter *adapter;

    LIST_FOR_EACH_ENTRY(adapter, &adapters, struct adapter, entry)
        if (index == adapter->id) return &adapter->dev;

    WARN( "Failed to find adapter with id %u.\n", index );
    return NULL;
}

/* display_lock mutex must be held */
static struct display_device *find_adapter_device_by_name( UNICODE_STRING *name )
{
    SIZE_T len = name->Length / sizeof(WCHAR);
    struct adapter *adapter;

    LIST_FOR_EACH_ENTRY(adapter, &adapters, struct adapter, entry)
        if (!wcsnicmp( name->Buffer, adapter->dev.device_name, len ) && !adapter->dev.device_name[len])
            return &adapter->dev;

    WARN( "Failed to find adapter with name %s.\n", debugstr_us(name) );
    return NULL;
}

/* Find and acquire the adapter matching name, or primary adapter if name is NULL.
 * If not NULL, the returned adapter needs to be released with adapter_release.
 */
static struct adapter *find_adapter( UNICODE_STRING *name )
{
    struct display_device *device;
    struct adapter *adapter;

    if (!lock_display_devices()) return NULL;

    if (name && name->Length) device = find_adapter_device_by_name( name );
    else device = find_adapter_device_by_id( 0 ); /* use primary adapter */

    if (!device) adapter = NULL;
    else adapter = adapter_acquire( CONTAINING_RECORD( device, struct adapter, dev ) );

    unlock_display_devices();
    return adapter;
}

/***********************************************************************
 *	     NtUserEnumDisplayDevices    (win32u.@)
 */
NTSTATUS WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                          DISPLAY_DEVICEW *info, DWORD flags )
{
    struct display_device *found = NULL;

    TRACE( "%s %u %p %#x\n", debugstr_us( device ), index, info, flags );

    if (!info || !info->cb) return STATUS_UNSUCCESSFUL;

    if (!lock_display_devices()) return STATUS_UNSUCCESSFUL;

    if (!device || !device->Length) found = find_adapter_device_by_id( index );
    else if ((found = find_adapter_device_by_name( device ))) found = find_monitor_device( found, index );

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
    return found ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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

static const DEVMODEW *find_display_mode( const DEVMODEW *modes, DEVMODEW *devmode )
{
    const DEVMODEW *mode;

    if (is_detached_mode( devmode )) return devmode;

    for (mode = modes; mode && mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        if ((mode->dmFields & DM_DISPLAYFLAGS) && (mode->dmDisplayFlags & WINE_DM_UNSUPPORTED))
            continue;
        if ((devmode->dmFields & DM_BITSPERPEL) && devmode->dmBitsPerPel && devmode->dmBitsPerPel != mode->dmBitsPerPel)
            continue;
        if ((devmode->dmFields & DM_PELSWIDTH) && devmode->dmPelsWidth != mode->dmPelsWidth)
            continue;
        if ((devmode->dmFields & DM_PELSHEIGHT) && devmode->dmPelsHeight != mode->dmPelsHeight)
            continue;
        if ((devmode->dmFields & DM_DISPLAYFREQUENCY) && devmode->dmDisplayFrequency != mode->dmDisplayFrequency
            && devmode->dmDisplayFrequency > 1 && mode->dmDisplayFrequency)
            continue;
        if ((devmode->dmFields & DM_DISPLAYORIENTATION) && devmode->dmDisplayOrientation != mode->dmDisplayOrientation)
            continue;
        if ((devmode->dmFields & DM_DISPLAYFLAGS) && (mode->dmFields & DM_DISPLAYFLAGS) &&
            (devmode->dmDisplayFlags & DM_INTERLACED) != (mode->dmDisplayFlags & DM_INTERLACED))
            continue;
        if ((devmode->dmFields & DM_DISPLAYFIXEDOUTPUT) && (mode->dmFields & DM_DISPLAYFIXEDOUTPUT) &&
            devmode->dmDisplayFixedOutput != mode->dmDisplayFixedOutput)
            continue;

        return mode;
    }

    return NULL;
}

static BOOL adapter_get_full_mode( const struct adapter *adapter, const DEVMODEW *devmode, DEVMODEW *full_mode )
{
    const DEVMODEW *adapter_mode;

    if (devmode)
    {
        trace_devmode( devmode );

        if (devmode->dmSize < offsetof(DEVMODEW, dmICMMethod)) return FALSE;
        if (!is_detached_mode( devmode ) &&
            (!(devmode->dmFields & DM_BITSPERPEL) || !devmode->dmBitsPerPel) &&
            (!(devmode->dmFields & DM_PELSWIDTH) || !devmode->dmPelsWidth) &&
            (!(devmode->dmFields & DM_PELSHEIGHT) || !devmode->dmPelsHeight) &&
            (!(devmode->dmFields & DM_DISPLAYFREQUENCY) || !devmode->dmDisplayFrequency))
            devmode = NULL;
    }

    if (devmode) memcpy( full_mode, devmode, devmode->dmSize );
    else
    {
        if (!adapter_get_registry_settings( adapter, full_mode )) return FALSE;
        TRACE( "Return to original display mode\n" );
    }

    if ((full_mode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN( "devmode doesn't specify the resolution: %#x\n", full_mode->dmFields );
        return FALSE;
    }

    if (!is_detached_mode( full_mode ) && (!full_mode->dmPelsWidth || !full_mode->dmPelsHeight || !(full_mode->dmFields & DM_POSITION)))
    {
        DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
        if (!user_driver->pGetCurrentDisplaySettings( adapter->dev.device_name, &current_mode )) return FALSE;
        if (!full_mode->dmPelsWidth) full_mode->dmPelsWidth = current_mode.dmPelsWidth;
        if (!full_mode->dmPelsHeight) full_mode->dmPelsHeight = current_mode.dmPelsHeight;
        if (!(full_mode->dmFields & DM_POSITION))
        {
            full_mode->dmPosition = current_mode.dmPosition;
            full_mode->dmFields |= DM_POSITION;
        }
    }

    if ((adapter_mode = find_display_mode( adapter->modes, full_mode )) && adapter_mode != full_mode)
    {
        POINTL position = full_mode->dmPosition;
        *full_mode = *adapter_mode;
        full_mode->dmFields |= DM_POSITION;
        full_mode->dmPosition = position;
    }

    return adapter_mode != NULL;
}

static DEVMODEW *get_display_settings( const WCHAR *devname, const DEVMODEW *devmode )
{
    DEVMODEW *mode, *displays;
    struct adapter *adapter;
    BOOL ret;

    if (!lock_display_devices()) return NULL;

    /* allocate an extra mode for easier iteration */
    if (!(displays = calloc( list_count( &adapters ) + 1, sizeof(DEVMODEW) ))) goto done;
    mode = displays;

    LIST_FOR_EACH_ENTRY( adapter, &adapters, struct adapter, entry )
    {
        mode->dmSize = sizeof(DEVMODEW);
        if (devmode && !wcsicmp( devname, adapter->dev.device_name ))
            memcpy( &mode->dmFields, &devmode->dmFields, devmode->dmSize - offsetof(DEVMODEW, dmFields) );
        else
        {
            if (!devname) ret = adapter_get_registry_settings( adapter, mode );
            else ret = user_driver->pGetCurrentDisplaySettings( adapter->dev.device_name, mode );
            if (!ret) goto done;
        }

        lstrcpyW( mode->dmDeviceName, adapter->dev.device_name );
        mode = NEXT_DEVMODEW(mode);
    }

    unlock_display_devices();
    return displays;

done:
    unlock_display_devices();
    free( displays );
    return NULL;
}

static INT offset_length( POINT offset )
{
    return offset.x * offset.x + offset.y * offset.y;
}

static void set_rect_from_devmode( RECT *rect, const DEVMODEW *mode )
{
    SetRect( rect, mode->dmPosition.x, mode->dmPosition.y, mode->dmPosition.x + mode->dmPelsWidth,
             mode->dmPosition.y + mode->dmPelsHeight );
}

/* Check if a rect overlaps with placed display rects */
static BOOL overlap_placed_displays( const RECT *rect, const DEVMODEW *displays )
{
    const DEVMODEW *mode;
    RECT intersect;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode( &intersect, mode );
        if ((mode->dmFields & DM_POSITION) && intersect_rect( &intersect, &intersect, rect )) return TRUE;
    }

    return FALSE;
}

/* Get the offset with minimum length to place a display next to the placed displays with no spacing and overlaps */
static POINT get_placement_offset( const DEVMODEW *displays, const DEVMODEW *placing )
{
    POINT points[8], left_top, offset, min_offset = {0, 0};
    INT point_idx, point_count, vertex_idx;
    BOOL has_placed = FALSE, first = TRUE;
    RECT desired_rect, rect;
    const DEVMODEW *mode;
    INT width, height;

    set_rect_from_devmode( &desired_rect, placing );

    /* If the display to be placed is detached, no offset is needed to place it */
    if (IsRectEmpty( &desired_rect )) return min_offset;

    /* If there is no placed and attached display, place this display as it is */
    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode( &rect, mode );
        if ((mode->dmFields & DM_POSITION) && !IsRectEmpty( &rect ))
        {
            has_placed = TRUE;
            break;
        }
    }

    if (!has_placed) return min_offset;

    /* Try to place this display with each of its four vertices at every vertex of the placed
     * displays and see which combination has the minimum offset length */
    width = desired_rect.right - desired_rect.left;
    height = desired_rect.bottom - desired_rect.top;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        set_rect_from_devmode( &rect, mode );
        if (!(mode->dmFields & DM_POSITION) || IsRectEmpty( &rect )) continue;

        /* Get four vertices of the placed display rectangle */
        points[0].x = rect.left;
        points[0].y = rect.top;
        points[1].x = rect.left;
        points[1].y = rect.bottom;
        points[2].x = rect.right;
        points[2].y = rect.top;
        points[3].x = rect.right;
        points[3].y = rect.bottom;
        point_count = 4;

        /* Intersected points when moving the display to be placed horizontally */
        if (desired_rect.bottom >= rect.top && desired_rect.top <= rect.bottom)
        {
            points[point_count].x = rect.left;
            points[point_count++].y = desired_rect.top;
            points[point_count].x = rect.right;
            points[point_count++].y = desired_rect.top;
        }
        /* Intersected points when moving the display to be placed vertically */
        if (desired_rect.left <= rect.right && desired_rect.right >= rect.left)
        {
            points[point_count].x = desired_rect.left;
            points[point_count++].y = rect.top;
            points[point_count].x = desired_rect.left;
            points[point_count++].y = rect.bottom;
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

                offset.x = left_top.x - desired_rect.left;
                offset.y = left_top.y - desired_rect.top;
                rect = desired_rect;
                OffsetRect( &rect, offset.x, offset.y );
                if (!overlap_placed_displays( &rect, displays ))
                {
                    if (first)
                    {
                        min_offset = offset;
                        first = FALSE;
                        continue;
                    }

                    if (offset_length( offset ) < offset_length( min_offset )) min_offset = offset;
                }
            }
        }
    }

    return min_offset;
}

static void place_all_displays( DEVMODEW *displays )
{
    POINT min_offset, offset;
    DEVMODEW *mode, *placing;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        mode->dmFields &= ~DM_POSITION;

    /* Place all displays with no extra space between them and no overlapping */
    while (1)
    {
        /* Place the unplaced display with the minimum offset length first */
        placing = NULL;
        for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        {
            if (mode->dmFields & DM_POSITION) continue;

            offset = get_placement_offset( displays, mode );
            if (!placing || offset_length( offset ) < offset_length( min_offset ))
            {
                min_offset = offset;
                placing = mode;
            }
        }

        /* If all displays are placed */
        if (!placing) break;

        placing->dmPosition.x += min_offset.x;
        placing->dmPosition.y += min_offset.y;
        placing->dmFields |= DM_POSITION;
    }
}

static BOOL all_detached_settings( const DEVMODEW *displays )
{
    const DEVMODEW *mode;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
        if (!is_detached_mode( mode )) return FALSE;

    return TRUE;
}

static LONG apply_display_settings( const WCHAR *devname, const DEVMODEW *devmode,
                                    HWND hwnd, DWORD flags, void *lparam )
{
    struct adapter *adapter;
    DEVMODEW *displays;
    LONG ret;

    displays = get_display_settings( devname, devmode );
    if (!displays) return DISP_CHANGE_FAILED;

    if (all_detached_settings( displays ))
    {
        WARN( "Detaching all modes is not permitted.\n" );
        free( displays );
        return DISP_CHANGE_SUCCESSFUL;
    }

    place_all_displays( displays );

    ret = user_driver->pChangeDisplaySettings( displays, hwnd, flags, lparam );

    free( displays );
    if (ret) return ret;

    if ((adapter = find_adapter( NULL )))
    {
        DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
        user_driver->pGetCurrentDisplaySettings( adapter->dev.device_name, &current_mode );
        adapter_release( adapter );

        send_notify_message( NtUserGetDesktopWindow(), WM_DISPLAYCHANGE, current_mode.dmBitsPerPel,
                             MAKELPARAM( current_mode.dmPelsWidth, current_mode.dmPelsHeight ), FALSE );
        send_message_timeout( HWND_BROADCAST, WM_DISPLAYCHANGE, current_mode.dmBitsPerPel,
                              MAKELPARAM( current_mode.dmPelsWidth, current_mode.dmPelsHeight ),
                              SMTO_ABORTIFHUNG, 2000, FALSE );
    }

    return ret;
}

/***********************************************************************
 *	     NtUserChangeDisplaySettings    (win32u.@)
 */
LONG WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                         DWORD flags, void *lparam )
{
    DEVMODEW full_mode = {.dmSize = sizeof(DEVMODEW)};
    LONG ret = DISP_CHANGE_SUCCESSFUL;
    struct adapter *adapter;

    TRACE( "%s %p %p %#x %p\n", debugstr_us(devname), devmode, hwnd, flags, lparam );
    TRACE( "flags=%s\n", _CDS_flags(flags) );

    if ((!devname || !devname->Length) && !devmode) return apply_display_settings( NULL, NULL, hwnd, flags, lparam );

    if (!(adapter = find_adapter( devname ))) return DISP_CHANGE_BADPARAM;

    if (!adapter_get_full_mode( adapter, devmode, &full_mode )) ret = DISP_CHANGE_BADMODE;
    else if ((flags & CDS_UPDATEREGISTRY) && !adapter_set_registry_settings( adapter, &full_mode )) ret = DISP_CHANGE_NOTUPDATED;
    else if (flags & (CDS_TEST | CDS_NORESET)) ret = DISP_CHANGE_SUCCESSFUL;
    else ret = apply_display_settings( adapter->dev.device_name, &full_mode, hwnd, flags, lparam );
    adapter_release( adapter );

    if (ret) ERR( "Changing %s display settings returned %d.\n", debugstr_us(devname), ret );
    return ret;
}

static BOOL adapter_enum_display_settings( const struct adapter *adapter, DWORD index, DEVMODEW *devmode, DWORD flags )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
    const DEVMODEW *adapter_mode;

    if (!(flags & EDS_ROTATEDMODE) && !user_driver->pGetCurrentDisplaySettings( adapter->dev.device_name, &current_mode ))
    {
        WARN( "Failed to query current display mode for EDS_ROTATEDMODE flag.\n" );
        return FALSE;
    }

    for (adapter_mode = adapter->modes; adapter_mode->dmSize; adapter_mode = NEXT_DEVMODEW(adapter_mode))
    {
        if (!(flags & EDS_ROTATEDMODE) && (adapter_mode->dmFields & DM_DISPLAYORIENTATION) &&
            adapter_mode->dmDisplayOrientation != current_mode.dmDisplayOrientation)
            continue;
        if (!(flags & EDS_RAWMODE) && (adapter_mode->dmFields & DM_DISPLAYFLAGS) &&
            (adapter_mode->dmDisplayFlags & WINE_DM_UNSUPPORTED))
            continue;
        if (!index--)
        {
            memcpy( &devmode->dmFields, &adapter_mode->dmFields, devmode->dmSize - FIELD_OFFSET(DEVMODEW, dmFields) );
            devmode->dmDisplayFlags &= ~WINE_DM_UNSUPPORTED;
            return TRUE;
        }
    }

    WARN( "device %s, index %#x, flags %#x display mode not found.\n",
          debugstr_w( adapter->dev.device_name ), index, flags );
    RtlSetLastWin32Error( ERROR_NO_MORE_FILES );
    return FALSE;
}

/***********************************************************************
 *	     NtUserEnumDisplaySettings    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD index, DEVMODEW *devmode, DWORD flags )
{
    static const WCHAR wine_display_driverW[] = {'W','i','n','e',' ','D','i','s','p','l','a','y',' ','D','r','i','v','e','r',0};
    struct adapter *adapter;
    BOOL ret;

    TRACE( "device %s, index %#x, devmode %p, flags %#x\n", debugstr_us(device), index, devmode, flags );

    if (!(adapter = find_adapter( device ))) return FALSE;

    lstrcpynW( devmode->dmDeviceName, wine_display_driverW, ARRAY_SIZE(devmode->dmDeviceName) );
    devmode->dmSpecVersion = DM_SPECVERSION;
    devmode->dmDriverVersion = DM_SPECVERSION;
    devmode->dmSize = offsetof(DEVMODEW, dmICMMethod);
    devmode->dmDriverExtra = 0;

    if (index == ENUM_REGISTRY_SETTINGS) ret = adapter_get_registry_settings( adapter, devmode );
    else if (index == ENUM_CURRENT_SETTINGS) ret = user_driver->pGetCurrentDisplaySettings( adapter->dev.device_name, devmode );
    else ret = adapter_enum_display_settings( adapter, index, devmode, flags );
    adapter_release( adapter );

    if (!ret) WARN( "Failed to query %s display settings.\n", debugstr_us(device) );
    else TRACE( "position %dx%d, resolution %ux%u, frequency %u, depth %u, orientation %#x.\n",
                devmode->dmPosition.x, devmode->dmPosition.y, devmode->dmPelsWidth, devmode->dmPelsHeight,
                devmode->dmDisplayFrequency, devmode->dmBitsPerPel, devmode->dmDisplayOrientation );
    return ret;
}

struct monitor_enum_info
{
    HANDLE handle;
    RECT rect;
};

static unsigned int active_monitor_count(void)
{
    struct monitor *monitor;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        if ((monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE)) count++;
    }
    return count;
}

/***********************************************************************
 *	     NtUserEnumDisplayMonitors    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplayMonitors( HDC hdc, RECT *rect, MONITORENUMPROC proc, LPARAM lparam )
{
    struct monitor_enum_info enum_buf[8], *enum_info = enum_buf;
    struct enum_display_monitor_params params;
    struct monitor *monitor;
    unsigned int count = 0, i;
    POINT origin;
    RECT limit;
    BOOL ret = TRUE;

    if (hdc)
    {
        DC *dc;
        if (!(dc = get_dc_ptr( hdc ))) return FALSE;
        origin.x = dc->attr->vis_rect.left;
        origin.y = dc->attr->vis_rect.top;
        release_dc_ptr( dc );
        if (NtGdiGetAppClipBox( hdc, &limit ) == ERROR) return FALSE;
    }
    else
    {
        origin.x = origin.y = 0;
        limit.left = limit.top = INT_MIN;
        limit.right = limit.bottom = INT_MAX;
    }
    if (rect && !intersect_rect( &limit, &limit, rect )) return TRUE;

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
        RECT monrect;

        if (!(monitor->dev.state_flags & DISPLAY_DEVICE_ACTIVE)) continue;

        monrect = map_dpi_rect( monitor->rc_monitor, get_monitor_dpi( monitor->handle ),
                                get_thread_dpi() );
        OffsetRect( &monrect, -origin.x, -origin.y );
        if (!intersect_rect( &monrect, &monrect, &limit )) continue;
        if (monitor->is_clone) continue;

        enum_info[count].handle = monitor->handle;
        enum_info[count].rect = monrect;
        count++;
    }

    unlock_display_devices();

    params.proc = proc;
    params.hdc = hdc;
    params.lparam = lparam;
    for (i = 0; i < count; i++)
    {
        void *ret_ptr;
        ULONG ret_len;
        params.monitor = enum_info[i].handle;
        params.rect = enum_info[i].rect;
        if (!KeUserModeCallback( NtUserCallEnumDisplayMonitor, &params, sizeof(params),
                                 &ret_ptr, &ret_len ))
        {
            ret = FALSE;
            break;
        }
    }
    if (enum_info != enum_buf) free( enum_info );
    return ret;
}

BOOL get_monitor_info( HMONITOR handle, MONITORINFO *info )
{
    struct monitor *monitor;
    UINT dpi_from, dpi_to;

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

        if ((dpi_to = get_thread_dpi()))
        {
            dpi_from = get_monitor_dpi( handle );
            info->rcMonitor = map_dpi_rect( info->rcMonitor, dpi_from, dpi_to );
            info->rcWork = map_dpi_rect( info->rcWork, dpi_from, dpi_to );
        }
        TRACE( "flags %04x, monitor %s, work %s\n", info->dwFlags,
               wine_dbgstr_rect(&info->rcMonitor), wine_dbgstr_rect(&info->rcWork));
        return TRUE;
    }

    unlock_display_devices();
    WARN( "invalid handle %p\n", handle );
    RtlSetLastWin32Error( ERROR_INVALID_MONITOR_HANDLE );
    return FALSE;
}

HMONITOR monitor_from_rect( const RECT *rect, DWORD flags, UINT dpi )
{
    HMONITOR primary = 0, nearest = 0, ret = 0;
    UINT max_area = 0, min_distance = ~0u;
    struct monitor *monitor;
    RECT r;

    r = map_dpi_rect( *rect, dpi, system_dpi );
    if (IsRectEmpty( &r ))
    {
        r.right = r.left + 1;
        r.bottom = r.top + 1;
    }

    if (!lock_display_devices()) return 0;

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        RECT intersect;
        RECT monitor_rect = map_dpi_rect( monitor->rc_monitor, get_monitor_dpi( monitor->handle ),
                                          system_dpi );

        if (intersect_rect( &intersect, &monitor_rect, &r ))
        {
            /* check for larger intersecting area */
            UINT area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
            if (area > max_area)
            {
                max_area = area;
                ret = monitor->handle;
            }
        }
        else if (!max_area)  /* if not intersecting, check for min distance */
        {
            UINT distance;
            UINT x, y;

            if (r.right <= monitor_rect.left) x = monitor_rect.left - r.right;
            else if (monitor_rect.right <= r.left) x = r.left - monitor_rect.right;
            else x = 0;
            if (r.bottom <= monitor_rect.top) y = monitor_rect.top - r.bottom;
            else if (monitor_rect.bottom <= r.top) y = r.top - monitor_rect.bottom;
            else y = 0;
            distance = x * x + y * y;
            if (distance < min_distance)
            {
                min_distance = distance;
                nearest = monitor->handle;
            }
        }

        if (monitor->flags & MONITORINFOF_PRIMARY) primary = monitor->handle;
    }

    unlock_display_devices();

    if (!ret)
    {
        if (flags & MONITOR_DEFAULTTOPRIMARY) ret = primary;
        else if (flags & MONITOR_DEFAULTTONEAREST) ret = nearest;
    }

    TRACE( "%s flags %x returning %p\n", wine_dbgstr_rect(rect), flags, ret );
    return ret;
}

HMONITOR monitor_from_point( POINT pt, DWORD flags, UINT dpi )
{
    RECT rect;
    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    return monitor_from_rect( &rect, flags, dpi );
}

/* see MonitorFromWindow */
HMONITOR monitor_from_window( HWND hwnd, DWORD flags, UINT dpi )
{
    RECT rect;
    WINDOWPLACEMENT wp;

    TRACE( "(%p, 0x%08x)\n", hwnd, flags );

    wp.length = sizeof(wp);
    if (is_iconic( hwnd ) && NtUserGetWindowPlacement( hwnd, &wp ))
        return monitor_from_rect( &wp.rcNormalPosition, flags, dpi );

    if (get_window_rect( hwnd, &rect, dpi ))
        return monitor_from_rect( &rect, flags, dpi );

    if (!(flags & (MONITOR_DEFAULTTOPRIMARY|MONITOR_DEFAULTTONEAREST))) return 0;
    /* retrieve the primary */
    SetRect( &rect, 0, 0, 1, 1 );
    return monitor_from_rect( &rect, flags, dpi );
}

/***********************************************************************
 *	     NtUserGetSystemDpiForProcess    (win32u.@)
 */
ULONG WINAPI NtUserGetSystemDpiForProcess( HANDLE process )
{
    if (process && process != GetCurrentProcess())
    {
        FIXME( "not supported on other process %p\n", process );
        return 0;
    }

    return system_dpi;
}

/***********************************************************************
 *           NtUserGetDpiForMonitor   (win32u.@)
 */
BOOL WINAPI NtUserGetDpiForMonitor( HMONITOR monitor, UINT type, UINT *x, UINT *y )
{
    if (type > 2)
    {
        RtlSetLastWin32Error( ERROR_BAD_ARGUMENTS );
        return FALSE;
    }
    if (!x || !y)
    {
        RtlSetLastWin32Error( ERROR_INVALID_ADDRESS );
        return FALSE;
    }
    switch (get_thread_dpi_awareness())
    {
    case DPI_AWARENESS_UNAWARE:      *x = *y = USER_DEFAULT_SCREEN_DPI; break;
    case DPI_AWARENESS_SYSTEM_AWARE: *x = *y = system_dpi; break;
    default:                         *x = *y = get_monitor_dpi( monitor ); break;
    }
    return TRUE;
}

/**********************************************************************
 *           LogicalToPhysicalPointForPerMonitorDPI   (win32u.@)
 */
BOOL WINAPI NtUserLogicalToPerMonitorDPIPhysicalPoint( HWND hwnd, POINT *pt )
{
    RECT rect;

    if (!get_window_rect( hwnd, &rect, get_thread_dpi() )) return FALSE;
    if (pt->x < rect.left || pt->y < rect.top || pt->x > rect.right || pt->y > rect.bottom) return FALSE;
    *pt = point_win_to_phys_dpi( hwnd, *pt );
    return TRUE;
}

/**********************************************************************
 *           NtUserPerMonitorDPIPhysicalToLogicalPoint   (win32u.@)
 */
BOOL WINAPI NtUserPerMonitorDPIPhysicalToLogicalPoint( HWND hwnd, POINT *pt )
{
    RECT rect;
    BOOL ret = FALSE;

    if (get_window_rect( hwnd, &rect, 0 ) &&
        pt->x >= rect.left && pt->y >= rect.top && pt->x <= rect.right && pt->y <= rect.bottom)
    {
        *pt = point_phys_to_win_dpi( hwnd, *pt );
        ret = TRUE;
    }
    return ret;
}

/* retrieve the cached base keys for a given entry */
static BOOL get_base_keys( enum parameter_key index, HKEY *base_key, HKEY *volatile_key )
{
    static HKEY base_keys[NB_PARAM_KEYS];
    static HKEY volatile_keys[NB_PARAM_KEYS];
    WCHAR bufferW[128];
    HKEY key;

    if (!base_keys[index] && base_key)
    {
        if (!(key = reg_create_key( hkcu_key, bufferW,
                asciiz_to_unicode( bufferW, parameter_key_names[index] ) - sizeof(WCHAR),
                0, NULL )))
            return FALSE;
        if (InterlockedCompareExchangePointer( (void **)&base_keys[index], key, 0 ))
            NtClose( key );
    }
    if (!volatile_keys[index] && volatile_key)
    {
        if (!(key = reg_create_key( volatile_base_key, bufferW,
                asciiz_to_unicode( bufferW, parameter_key_names[index] ) - sizeof(WCHAR),
                REG_OPTION_VOLATILE, NULL )))
            return FALSE;
        if (InterlockedCompareExchangePointer( (void **)&volatile_keys[index], key, 0 ))
            NtClose( key );
    }
    if (base_key) *base_key = base_keys[index];
    if (volatile_key) *volatile_key = volatile_keys[index];
    return TRUE;
}

/* load a value to a registry entry */
static DWORD load_entry( struct sysparam_entry *entry, void *data, DWORD size )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    DWORD count;
    HKEY base_key, volatile_key;

    if (!get_base_keys( entry->base_key, &base_key, &volatile_key )) return FALSE;

    if (!(count = query_reg_ascii_value( volatile_key, entry->regval, value, sizeof(buffer) )))
        count = query_reg_ascii_value( base_key, entry->regval, value, sizeof(buffer) );
    if (count > size)
    {
        count = size;
        /* make sure strings are null-terminated */
        if (value->Type == REG_SZ) ((WCHAR *)value->Data)[count / sizeof(WCHAR) - 1] = 0;
    }
    if (count) memcpy( data, value->Data, count );
    entry->loaded = TRUE;
    return count;
}

/* save a value to a registry entry */
static BOOL save_entry( const struct sysparam_entry *entry, const void *data, DWORD size,
                        DWORD type, UINT flags )
{
    HKEY base_key, volatile_key;
    WCHAR nameW[64];

    asciiz_to_unicode( nameW, entry->regval );
    if (flags & SPIF_UPDATEINIFILE)
    {
        if (!get_base_keys( entry->base_key, &base_key, &volatile_key )) return FALSE;
        if (!set_reg_value( base_key, nameW, type, data, size )) return FALSE;
        reg_delete_value( volatile_key, nameW );

        if (entry->mirror && get_base_keys( entry->mirror_key, &base_key, NULL ))
        {
            asciiz_to_unicode( nameW, entry->mirror );
            set_reg_value( base_key, nameW, type, data, size );
        }
    }
    else
    {
        if (!get_base_keys( entry->base_key, NULL, &volatile_key )) return FALSE;
        if (!set_reg_value( volatile_key, nameW, type, data, size )) return FALSE;
    }
    return TRUE;
}

/* save a string value to a registry entry */
static BOOL save_entry_string( const struct sysparam_entry *entry, const WCHAR *str, UINT flags )
{
    return save_entry( entry, str, (lstrlenW(str) + 1) * sizeof(WCHAR), REG_SZ, flags );
}

/* initialize an entry in the registry if missing */
static BOOL init_entry( struct sysparam_entry *entry, const void *data, DWORD size, DWORD type )
{
    KEY_VALUE_PARTIAL_INFORMATION value;
    UNICODE_STRING name;
    WCHAR nameW[64];
    HKEY base_key;
    DWORD count;
    NTSTATUS status;

    if (!get_base_keys( entry->base_key, &base_key, NULL )) return FALSE;

    name.Buffer = nameW;
    name.MaximumLength = asciiz_to_unicode( nameW, entry->regval );
    name.Length = name.MaximumLength - sizeof(WCHAR);
    status = NtQueryValueKey( base_key, &name, KeyValuePartialInformation,
                              &value, sizeof(value), &count );
    if (!status || status == STATUS_BUFFER_OVERFLOW) return TRUE;

    if (!set_reg_value( base_key, nameW, type, data, size )) return FALSE;
    if (entry->mirror && get_base_keys( entry->mirror_key, &base_key, NULL ))
    {
        asciiz_to_unicode( nameW, entry->mirror );
        set_reg_value( base_key, nameW, type, data, size );
    }
    entry->loaded = TRUE;
    return TRUE;
}

/* initialize a string value in the registry if missing */
static BOOL init_entry_string( struct sysparam_entry *entry, const WCHAR *str )
{
    return init_entry( entry, str, (lstrlenW(str) + 1) * sizeof(WCHAR), REG_SZ );
}

/* set an int parameter in the registry */
static BOOL set_int_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR bufW[32];
    char buf[32];

    sprintf( buf, "%d", int_param );
    asciiz_to_unicode( bufW, buf );
    if (!save_entry_string( &entry->hdr, bufW, flags )) return FALSE;
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize an int parameter */
static BOOL init_int_entry( union sysparam_all_entry *entry )
{
    WCHAR bufW[32];
    char buf[32];

    sprintf( buf, "%d", entry->uint.val );
    asciiz_to_unicode( bufW, buf );
    return init_entry_string( &entry->hdr, bufW );
}

/* load a uint parameter from the registry */
static BOOL get_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->uint.val = wcstol( buf, NULL, 10 );
    }
    *(UINT *)ptr_param = entry->uint.val;
    return TRUE;
}

/* set a uint parameter in the registry */
static BOOL set_uint_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR bufW[32];
    char buf[32];

    sprintf( buf, "%u", int_param );
    asciiz_to_unicode( bufW, buf );
    if (!save_entry_string( &entry->hdr, bufW, flags )) return FALSE;
    entry->uint.val = int_param;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a uint parameter */
static BOOL init_uint_entry( union sysparam_all_entry *entry )
{
    WCHAR bufW[32];
    char buf[32];

    sprintf( buf, "%u", entry->uint.val );
    asciiz_to_unicode( bufW, buf );
    return init_entry_string( &entry->hdr, bufW );
}

/* load a twips parameter from the registry */
static BOOL get_twips_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    int val;

    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->uint.val = wcstol( buf, NULL, 10 );
    }

    /* Dimensions are quoted as being "twips" values if negative and pixels if positive.
     * One inch is 1440 twips.
     * See for example
     *       Technical Reference to the Windows 2000 Registry ->
     *       HKEY_CURRENT_USER -> Control Panel -> Desktop -> WindowMetrics
     */
    val = entry->uint.val;
    if (val < 0)
        val = muldiv( -val, dpi, 1440 );
    else
        val = map_to_dpi( val, dpi );

    *(int *)ptr_param = val;
    return TRUE;
}

/* set a twips parameter in the registry */
static BOOL set_twips_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    int val = int_param;
    if (val > 0) val = map_from_system_dpi( val );
    return set_int_entry( entry, val, ptr_param, flags );
}

/* load a bool parameter from the registry */
static BOOL get_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->bool.val = wcstol( buf, NULL, 10 ) != 0;
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter in the registry */
static BOOL set_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[] = { int_param ? '1' : '0', 0 };

    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter */
static BOOL init_bool_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[] = { entry->bool.val ? '1' : '0', 0 };

    return init_entry_string( &entry->hdr, buf );
}

/* load a bool parameter using Yes/No strings from the registry */
static BOOL get_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->bool.val = !wcsicmp( yesW, buf );
    }
    *(UINT *)ptr_param = entry->bool.val;
    return TRUE;
}

/* set a bool parameter using Yes/No strings from the registry */
static BOOL set_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    const WCHAR *str = int_param ? yesW : noW;

    if (!save_entry_string( &entry->hdr, str, flags )) return FALSE;
    entry->bool.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter using Yes/No strings */
static BOOL init_yesno_entry( union sysparam_all_entry *entry )
{
    return init_entry_string( &entry->hdr, entry->bool.val ? yesW : noW );
}

/* load a dword (binary) parameter from the registry */
static BOOL get_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        DWORD val;
        if (load_entry( &entry->hdr, &val, sizeof(val) ) == sizeof(DWORD)) entry->dword.val = val;
    }
    *(DWORD *)ptr_param = entry->dword.val;
    return TRUE;
}

/* set a dword (binary) parameter in the registry */
static BOOL set_dword_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    DWORD val = PtrToUlong( ptr_param );

    if (!save_entry( &entry->hdr, &val, sizeof(val), REG_DWORD, flags )) return FALSE;
    entry->dword.val = val;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a dword parameter */
static BOOL init_dword_entry( union sysparam_all_entry *entry )
{
    return init_entry( &entry->hdr, &entry->dword.val, sizeof(entry->dword.val), REG_DWORD );
}

/* load an RGB parameter from the registry */
static BOOL get_rgb_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];

        if (load_entry( &entry->hdr, buf, sizeof(buf) ))
        {
            DWORD r, g, b;
            WCHAR *end, *str = buf;

            r = wcstoul( str, &end, 10 );
            if (end == str || !*end) goto done;
            str = end + 1;
            g = wcstoul( str, &end, 10 );
            if (end == str || !*end) goto done;
            str = end + 1;
            b = wcstoul( str, &end, 10 );
            if (end == str) goto done;
            if (r > 255 || g > 255 || b > 255) goto done;
            entry->rgb.val = RGB( r, g, b );
        }
    }
done:
    *(COLORREF *)ptr_param = entry->rgb.val;
    return TRUE;
}

/* set an RGB parameter in the registry */
static BOOL set_rgb_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR bufW[32];
    char buf[32];
    HBRUSH brush;
    HPEN pen;

    sprintf( buf, "%u %u %u", GetRValue(int_param), GetGValue(int_param), GetBValue(int_param) );
    asciiz_to_unicode( bufW, buf );
    if (!save_entry_string( &entry->hdr, bufW, flags )) return FALSE;
    entry->rgb.val = int_param;
    entry->hdr.loaded = TRUE;
    if ((brush = InterlockedExchangePointer( (void **)&entry->rgb.brush, 0 )))
    {
        make_gdi_object_system( brush, FALSE );
        NtGdiDeleteObjectApp( brush );
    }
    if ((pen = InterlockedExchangePointer( (void **)&entry->rgb.pen, 0 )))
    {
        make_gdi_object_system( pen, FALSE );
        NtGdiDeleteObjectApp( pen );
    }
    return TRUE;
}

/* initialize an RGB parameter */
static BOOL init_rgb_entry( union sysparam_all_entry *entry )
{
    WCHAR bufW[32];
    char buf[32];

    sprintf( buf, "%u %u %u", GetRValue(entry->rgb.val), GetGValue(entry->rgb.val),
             GetBValue(entry->rgb.val) );
    asciiz_to_unicode( bufW, buf );
    return init_entry_string( &entry->hdr, bufW );
}

/* get a path parameter in the registry */
static BOOL get_path_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buffer[MAX_PATH];

        if (load_entry( &entry->hdr, buffer, sizeof(buffer) ))
            lstrcpynW( entry->path.path, buffer, MAX_PATH );
    }
    lstrcpynW( ptr_param, entry->path.path, int_param );
    return TRUE;
}

/* set a path parameter in the registry */
static BOOL set_path_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buffer[MAX_PATH];
    BOOL ret;

    lstrcpynW( buffer, ptr_param, MAX_PATH );
    ret = save_entry_string( &entry->hdr, buffer, flags );
    if (ret)
    {
        lstrcpyW( entry->path.path, buffer );
        entry->hdr.loaded = TRUE;
    }
    return ret;
}

/* initialize a path parameter */
static BOOL init_path_entry( union sysparam_all_entry *entry )
{
    return init_entry_string( &entry->hdr, entry->path.path );
}

/* get a binary parameter in the registry */
static BOOL get_binary_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        void *buffer = malloc( entry->bin.size );
        DWORD len = load_entry( &entry->hdr, buffer, entry->bin.size );

        if (len)
        {
            memcpy( entry->bin.ptr, buffer, entry->bin.size );
            memset( (char *)entry->bin.ptr + len, 0, entry->bin.size - len );
        }
        free( buffer );
    }
    memcpy( ptr_param, entry->bin.ptr, min( int_param, entry->bin.size ) );
    return TRUE;
}

/* set a binary parameter in the registry */
static BOOL set_binary_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    BOOL ret;
    void *buffer = malloc( entry->bin.size );

    memcpy( buffer, entry->bin.ptr, entry->bin.size );
    memcpy( buffer, ptr_param, min( int_param, entry->bin.size ));
    ret = save_entry( &entry->hdr, buffer, entry->bin.size, REG_BINARY, flags );
    if (ret)
    {
        memcpy( entry->bin.ptr, buffer, entry->bin.size );
        entry->hdr.loaded = TRUE;
    }
    free( buffer );
    return ret;
}

/* initialize a binary parameter */
static BOOL init_binary_entry( union sysparam_all_entry *entry )
{
    return init_entry( &entry->hdr, entry->bin.ptr, entry->bin.size, REG_BINARY );
}

static void logfont16to32( const LOGFONT16 *font16, LPLOGFONTW font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    win32u_mbtowc( &ansi_cp, font32->lfFaceName, LF_FACESIZE, font16->lfFaceName,
                   strlen( font16->lfFaceName ));
    font32->lfFaceName[LF_FACESIZE-1] = 0;
}

static void get_real_fontname( LOGFONTW *lf, WCHAR fullname[LF_FACESIZE] )
{
    struct font_enum_entry enum_entry;
    ULONG count = sizeof(enum_entry);
    HDC hdc;

    hdc = get_display_dc();
    NtGdiEnumFonts( hdc, 0, 0, lstrlenW( lf->lfFaceName ), lf->lfFaceName, lf->lfCharSet,
                    &count, &enum_entry );
    release_display_dc( hdc );

    if (count)
        lstrcpyW( fullname, enum_entry.lf.elfFullName );
    else
        lstrcpyW( fullname, lf->lfFaceName );
}

LONG get_char_dimensions( HDC hdc, TEXTMETRICW *metric, LONG *height )
{
    SIZE sz;
    static const WCHAR abcdW[] =
        {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
         'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
         'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};

    if (metric && !NtGdiGetTextMetricsW( hdc, metric, 0 )) return 0;

    if (!NtGdiGetTextExtentExW( hdc, abcdW, ARRAYSIZE(abcdW), 0, NULL, NULL, &sz, 0 ))
        return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

/* get text metrics and/or "average" char width of the specified logfont
 * for the specified dc */
static void get_text_metr_size( HDC hdc, LOGFONTW *lf, TEXTMETRICW *metric, UINT *psz )
{
    HFONT hfont, hfontsav;
    TEXTMETRICW tm;
    UINT ret;
    if (!metric) metric = &tm;
    hfont = NtGdiHfontCreate( lf, sizeof(*lf), 0, 0, NULL );
    if (!hfont || !(hfontsav = NtGdiSelectFont( hdc, hfont )))
    {
        metric->tmHeight = -1;
        if (psz) *psz = 10;
        if (hfont) NtGdiDeleteObjectApp( hfont );
        return;
    }
    ret = get_char_dimensions( hdc, metric, NULL );
    if (psz) *psz = ret ? ret : 10;
    NtGdiSelectFont( hdc, hfontsav );
    NtGdiDeleteObjectApp( hfont );
}

DWORD get_dialog_base_units(void)
{
    static LONG cx, cy;

    if (!cx)
    {
        HDC hdc;

        if ((hdc = NtUserGetDCEx( 0, 0, DCX_CACHE | DCX_WINDOW )))
        {
            cx = get_char_dimensions( hdc, NULL, &cy );
            NtUserReleaseDC( 0, hdc );
        }
        TRACE( "base units = %d,%d\n", cx, cy );
    }

    return MAKELONG( muldiv( cx, get_system_dpi(), USER_DEFAULT_SCREEN_DPI ),
                     muldiv( cy, get_system_dpi(), USER_DEFAULT_SCREEN_DPI ));
}

/* adjust some of the raw values found in the registry */
static void normalize_nonclientmetrics( NONCLIENTMETRICSW *pncm)
{
    TEXTMETRICW tm;
    HDC hdc = get_display_dc();

    if( pncm->iBorderWidth < 1) pncm->iBorderWidth = 1;
    if( pncm->iCaptionWidth < 8) pncm->iCaptionWidth = 8;
    if( pncm->iScrollWidth < 8) pncm->iScrollWidth = 8;
    if( pncm->iScrollHeight < 8) pncm->iScrollHeight = 8;

    /* adjust some heights to the corresponding font */
    get_text_metr_size( hdc, &pncm->lfMenuFont, &tm, NULL);
    pncm->iMenuHeight = max( pncm->iMenuHeight, 2 + tm.tmHeight + tm.tmExternalLeading );
    get_text_metr_size( hdc, &pncm->lfCaptionFont, &tm, NULL);
    pncm->iCaptionHeight = max( pncm->iCaptionHeight, 2 + tm.tmHeight);
    get_text_metr_size( hdc, &pncm->lfSmCaptionFont, &tm, NULL);
    pncm->iSmCaptionHeight = max( pncm->iSmCaptionHeight, 2 + tm.tmHeight);
    release_display_dc( hdc );
}

/* load a font (binary) parameter from the registry */
static BOOL get_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    LOGFONTW font;

    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        switch (load_entry( &entry->hdr, &font, sizeof(font) ))
        {
        case sizeof(font):
            if (font.lfHeight > 0) /* positive height value means points ( inch/72 ) */
                font.lfHeight = -muldiv( font.lfHeight, USER_DEFAULT_SCREEN_DPI, 72 );
            entry->font.val = font;
            break;
        case sizeof(LOGFONT16): /* win9x-winME format */
            logfont16to32( (LOGFONT16 *)&font, &entry->font.val );
            if (entry->font.val.lfHeight > 0)
                entry->font.val.lfHeight = -muldiv( entry->font.val.lfHeight, USER_DEFAULT_SCREEN_DPI, 72 );
            break;
        default:
            WARN( "Unknown format in key %s value %s\n",
                  debugstr_a( parameter_key_names[entry->hdr.base_key] ),
                  debugstr_a( entry->hdr.regval ));
            /* fall through */
        case 0: /* use the default GUI font */
            NtGdiExtGetObjectW( GetStockObject( DEFAULT_GUI_FONT ), sizeof(font), &font );
            font.lfHeight = map_from_system_dpi( font.lfHeight );
            font.lfWeight = entry->font.weight;
            entry->font.val = font;
            break;
        }
        get_real_fontname( &entry->font.val, entry->font.fullname );
        entry->hdr.loaded = TRUE;
    }
    font = entry->font.val;
    font.lfHeight = map_to_dpi( font.lfHeight, dpi );
    lstrcpyW( font.lfFaceName, entry->font.fullname );
    *(LOGFONTW *)ptr_param = font;
    return TRUE;
}

/* set a font (binary) parameter in the registry */
static BOOL set_font_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    LOGFONTW font;
    WCHAR *ptr;

    memcpy( &font, ptr_param, sizeof(font) );
    /* zero pad the end of lfFaceName so we don't save uninitialised data */
    for (ptr = font.lfFaceName; ptr < font.lfFaceName + LF_FACESIZE && *ptr; ptr++);
    if (ptr < font.lfFaceName + LF_FACESIZE)
        memset( ptr, 0, (font.lfFaceName + LF_FACESIZE - ptr) * sizeof(WCHAR) );
    if (font.lfHeight < 0) font.lfHeight = map_from_system_dpi( font.lfHeight );

    if (!save_entry( &entry->hdr, &font, sizeof(font), REG_BINARY, flags )) return FALSE;
    entry->font.val = font;
    get_real_fontname( &entry->font.val, entry->font.fullname );
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a font (binary) parameter */
static BOOL init_font_entry( union sysparam_all_entry *entry )
{
    NtGdiExtGetObjectW( GetStockObject( DEFAULT_GUI_FONT ), sizeof(entry->font.val), &entry->font.val );
    entry->font.val.lfHeight = map_from_system_dpi( entry->font.val.lfHeight );
    entry->font.val.lfWeight = entry->font.weight;
    get_real_fontname( &entry->font.val, entry->font.fullname );
    return init_entry( &entry->hdr, &entry->font.val, sizeof(entry->font.val), REG_BINARY );
}

/* get a user pref parameter in the registry */
static BOOL get_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    union sysparam_all_entry *parent_entry = (union sysparam_all_entry *)entry->pref.parent;
    BYTE prefs[8];

    if (!ptr_param) return FALSE;

    if (!parent_entry->hdr.get( parent_entry, sizeof(prefs), prefs, dpi )) return FALSE;
    *(BOOL *)ptr_param = (prefs[entry->pref.offset] & entry->pref.mask) != 0;
    return TRUE;
}

/* set a user pref parameter in the registry */
static BOOL set_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *parent_entry = (union sysparam_all_entry *)entry->pref.parent;
    BYTE prefs[8];

    parent_entry->hdr.loaded = FALSE;  /* force loading it again */
    if (!parent_entry->hdr.get( parent_entry, sizeof(prefs), prefs, get_system_dpi() )) return FALSE;

    if (PtrToUlong( ptr_param )) prefs[entry->pref.offset] |= entry->pref.mask;
    else prefs[entry->pref.offset] &= ~entry->pref.mask;

    return parent_entry->hdr.set( parent_entry, sizeof(prefs), prefs, flags );
}

static BOOL get_entry_dpi( void *ptr, UINT int_param, void *ptr_param, UINT dpi )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.get( entry, int_param, ptr_param, dpi );
}

static BOOL get_entry( void *ptr, UINT int_param, void *ptr_param )
{
    return get_entry_dpi( ptr, int_param, ptr_param, get_system_dpi() );
}

static BOOL set_entry( void *ptr, UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *entry = ptr;
    return entry->hdr.set( entry, int_param, ptr_param, flags );
}

#define UINT_ENTRY(name,val,base,reg)                                   \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, init_uint_entry, \
                                                  base, reg }, (val) }

#define UINT_ENTRY_MIRROR(name,val,base,reg,mirror_base) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_uint_entry, init_uint_entry, \
                                                  base, reg, mirror_base, reg }, (val) }

#define INT_ENTRY(name,val,base,reg) \
    struct sysparam_uint_entry entry_##name = { { get_uint_entry, set_int_entry, init_int_entry, \
                                                  base, reg }, (val) }

#define BOOL_ENTRY(name,val,base,reg) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, init_bool_entry, \
                                                  base, reg }, (val) }

#define BOOL_ENTRY_MIRROR(name,val,base,reg,mirror_base) \
    struct sysparam_bool_entry entry_##name = { { get_bool_entry, set_bool_entry, init_bool_entry, \
                                                  base, reg, mirror_base, reg }, (val) }

#define TWIPS_ENTRY(name,val,base,reg) \
    struct sysparam_uint_entry entry_##name = { { get_twips_entry, set_twips_entry, init_int_entry, \
                                                  base, reg }, (val) }

#define YESNO_ENTRY(name,val,base,reg) \
    struct sysparam_bool_entry entry_##name = { { get_yesno_entry, set_yesno_entry, init_yesno_entry, \
                                                  base, reg }, (val) }

#define DWORD_ENTRY(name,val,base,reg)                                  \
    struct sysparam_dword_entry entry_##name = { { get_dword_entry, set_dword_entry, init_dword_entry, \
                                                   base, reg }, (val) }

#define BINARY_ENTRY(name,data,base,reg) \
    struct sysparam_binary_entry entry_##name = { { get_binary_entry, set_binary_entry, init_binary_entry, \
                                                    base, reg }, data, sizeof(data) }

#define PATH_ENTRY(name,base,reg) \
    struct sysparam_path_entry entry_##name = { { get_path_entry, set_path_entry, init_path_entry, \
                                                  base, reg } }

#define FONT_ENTRY(name,weight,base,reg) \
    struct sysparam_font_entry entry_##name = { { get_font_entry, set_font_entry, init_font_entry, \
                                                  base, reg }, (weight) }

#define USERPREF_ENTRY(name,offset,mask) \
    struct sysparam_pref_entry entry_##name = { { get_userpref_entry, set_userpref_entry }, \
                                                &entry_USERPREFERENCESMASK, (offset), (mask) }

static UINT_ENTRY( DRAGWIDTH, 4, DESKTOP_KEY, "DragWidth" );
static UINT_ENTRY( DRAGHEIGHT, 4, DESKTOP_KEY, "DragHeight" );
static UINT_ENTRY( DOUBLECLICKTIME, 500, MOUSE_KEY, "DoubleClickSpeed" );
static UINT_ENTRY( FONTSMOOTHING, 2, DESKTOP_KEY, "FontSmoothing" );
static UINT_ENTRY( GRIDGRANULARITY, 0, DESKTOP_KEY, "GridGranularity" );
static UINT_ENTRY( KEYBOARDDELAY, 1, KEYBOARD_KEY, "KeyboardDelay" );
static UINT_ENTRY( KEYBOARDSPEED, 31, KEYBOARD_KEY, "KeyboardSpeed" );
static UINT_ENTRY( MENUSHOWDELAY, 400, DESKTOP_KEY, "MenuShowDelay" );
static UINT_ENTRY( MINARRANGE, ARW_HIDE, METRICS_KEY, "MinArrange" );
static UINT_ENTRY( MINHORZGAP, 0, METRICS_KEY, "MinHorzGap" );
static UINT_ENTRY( MINVERTGAP, 0, METRICS_KEY, "MinVertGap" );
static UINT_ENTRY( MINWIDTH, 154, METRICS_KEY, "MinWidth" );
static UINT_ENTRY( MOUSEHOVERHEIGHT, 4, MOUSE_KEY, "MouseHoverHeight" );
static UINT_ENTRY( MOUSEHOVERTIME, 400, MOUSE_KEY, "MouseHoverTime" );
static UINT_ENTRY( MOUSEHOVERWIDTH, 4, MOUSE_KEY, "MouseHoverWidth" );
static UINT_ENTRY( MOUSESPEED, 10, MOUSE_KEY, "MouseSensitivity" );
static UINT_ENTRY( MOUSETRAILS, 0, MOUSE_KEY, "MouseTrails" );
static UINT_ENTRY( SCREENSAVETIMEOUT, 300, DESKTOP_KEY, "ScreenSaveTimeOut" );
static UINT_ENTRY( WHEELSCROLLCHARS, 3, DESKTOP_KEY, "WheelScrollChars" );
static UINT_ENTRY( WHEELSCROLLLINES, 3, DESKTOP_KEY, "WheelScrollLines" );
static UINT_ENTRY_MIRROR( DOUBLECLKHEIGHT, 4, MOUSE_KEY, "DoubleClickHeight", DESKTOP_KEY );
static UINT_ENTRY_MIRROR( DOUBLECLKWIDTH, 4, MOUSE_KEY, "DoubleClickWidth", DESKTOP_KEY );
static UINT_ENTRY_MIRROR( MENUDROPALIGNMENT, 0, DESKTOP_KEY, "MenuDropAlignment", VERSION_KEY );

static INT_ENTRY( MOUSETHRESHOLD1, 6, MOUSE_KEY, "MouseThreshold1" );
static INT_ENTRY( MOUSETHRESHOLD2, 10, MOUSE_KEY, "MouseThreshold2" );
static INT_ENTRY( MOUSEACCELERATION, 1, MOUSE_KEY, "MouseSpeed" );

static BOOL_ENTRY( BLOCKSENDINPUTRESETS, FALSE, DESKTOP_KEY, "BlockSendInputResets" );
static BOOL_ENTRY( DRAGFULLWINDOWS, FALSE, DESKTOP_KEY, "DragFullWindows" );
static BOOL_ENTRY( KEYBOARDPREF, TRUE, KEYBOARDPREF_KEY, "On" );
static BOOL_ENTRY( LOWPOWERACTIVE, FALSE, DESKTOP_KEY, "LowPowerActive" );
static BOOL_ENTRY( MOUSEBUTTONSWAP, FALSE, MOUSE_KEY, "SwapMouseButtons" );
static BOOL_ENTRY( POWEROFFACTIVE, FALSE, DESKTOP_KEY, "PowerOffActive" );
static BOOL_ENTRY( SCREENREADER, FALSE, SCREENREADER_KEY, "On" );
static BOOL_ENTRY( SCREENSAVEACTIVE, TRUE, DESKTOP_KEY, "ScreenSaveActive" );
static BOOL_ENTRY( SCREENSAVERRUNNING, FALSE, DESKTOP_KEY, "WINE_ScreenSaverRunning" ); /* FIXME - real value */
static BOOL_ENTRY( SHOWSOUNDS, FALSE, SHOWSOUNDS_KEY, "On" );
static BOOL_ENTRY( SNAPTODEFBUTTON, FALSE, MOUSE_KEY, "SnapToDefaultButton" );
static BOOL_ENTRY_MIRROR( ICONTITLEWRAP, TRUE, DESKTOP_KEY, "IconTitleWrap", METRICS_KEY );
static BOOL_ENTRY( AUDIODESC_ON, FALSE, AUDIODESC_KEY, "On" );

static TWIPS_ENTRY( BORDER, -15, METRICS_KEY, "BorderWidth" );
static TWIPS_ENTRY( CAPTIONHEIGHT, -270, METRICS_KEY, "CaptionHeight" );
static TWIPS_ENTRY( CAPTIONWIDTH, -270, METRICS_KEY, "CaptionWidth" );
static TWIPS_ENTRY( ICONHORIZONTALSPACING, -1125, METRICS_KEY, "IconSpacing" );
static TWIPS_ENTRY( ICONVERTICALSPACING, -1125, METRICS_KEY, "IconVerticalSpacing" );
static TWIPS_ENTRY( MENUHEIGHT, -270, METRICS_KEY, "MenuHeight" );
static TWIPS_ENTRY( MENUWIDTH, -270, METRICS_KEY, "MenuWidth" );
static TWIPS_ENTRY( PADDEDBORDERWIDTH, 0, METRICS_KEY, "PaddedBorderWidth" );
static TWIPS_ENTRY( SCROLLHEIGHT, -240, METRICS_KEY, "ScrollHeight" );
static TWIPS_ENTRY( SCROLLWIDTH, -240, METRICS_KEY, "ScrollWidth" );
static TWIPS_ENTRY( SMCAPTIONHEIGHT, -225, METRICS_KEY, "SmCaptionHeight" );
static TWIPS_ENTRY( SMCAPTIONWIDTH, -225, METRICS_KEY, "SmCaptionWidth" );

static YESNO_ENTRY( BEEP, TRUE, SOUND_KEY, "Beep" );

static DWORD_ENTRY( ACTIVEWINDOWTRACKING, 0, MOUSE_KEY, "ActiveWindowTracking" );
static DWORD_ENTRY( ACTIVEWNDTRKTIMEOUT, 0, DESKTOP_KEY, "ActiveWndTrackTimeout" );
static DWORD_ENTRY( CARETWIDTH, 1, DESKTOP_KEY, "CaretWidth" );
static DWORD_ENTRY( DPISCALINGVER, 0, DESKTOP_KEY, "DpiScalingVer" );
static DWORD_ENTRY( FOCUSBORDERHEIGHT, 1, DESKTOP_KEY, "FocusBorderHeight" );
static DWORD_ENTRY( FOCUSBORDERWIDTH, 1, DESKTOP_KEY, "FocusBorderWidth" );
static DWORD_ENTRY( FONTSMOOTHINGCONTRAST, 0, DESKTOP_KEY, "FontSmoothingGamma" );
static DWORD_ENTRY( FONTSMOOTHINGORIENTATION, FE_FONTSMOOTHINGORIENTATIONRGB, DESKTOP_KEY, "FontSmoothingOrientation" );
static DWORD_ENTRY( FONTSMOOTHINGTYPE, FE_FONTSMOOTHINGSTANDARD, DESKTOP_KEY, "FontSmoothingType" );
static DWORD_ENTRY( FOREGROUNDFLASHCOUNT, 3, DESKTOP_KEY, "ForegroundFlashCount" );
static DWORD_ENTRY( FOREGROUNDLOCKTIMEOUT, 0, DESKTOP_KEY, "ForegroundLockTimeout" );
static DWORD_ENTRY( LOGPIXELS, 0, DESKTOP_KEY, "LogPixels" );
static DWORD_ENTRY( MOUSECLICKLOCKTIME, 1200, DESKTOP_KEY, "ClickLockTime" );
static DWORD_ENTRY( AUDIODESC_LOCALE, 0, AUDIODESC_KEY, "Locale" );

static PATH_ENTRY( DESKPATTERN, DESKTOP_KEY, "Pattern" );
static PATH_ENTRY( DESKWALLPAPER, DESKTOP_KEY, "Wallpaper" );

static BYTE user_prefs[8] = { 0x30, 0x00, 0x00, 0x80, 0x12, 0x00, 0x00, 0x00 };
static BINARY_ENTRY( USERPREFERENCESMASK, user_prefs, DESKTOP_KEY, "UserPreferencesMask" );

static FONT_ENTRY( CAPTIONLOGFONT, FW_BOLD, METRICS_KEY, "CaptionFont" );
static FONT_ENTRY( ICONTITLELOGFONT, FW_NORMAL, METRICS_KEY, "IconFont" );
static FONT_ENTRY( MENULOGFONT, FW_NORMAL, METRICS_KEY, "MenuFont" );
static FONT_ENTRY( MESSAGELOGFONT, FW_NORMAL, METRICS_KEY, "MessageFont" );
static FONT_ENTRY( SMCAPTIONLOGFONT, FW_NORMAL, METRICS_KEY, "SmCaptionFont" );
static FONT_ENTRY( STATUSLOGFONT, FW_NORMAL, METRICS_KEY, "StatusFont" );

static USERPREF_ENTRY( MENUANIMATION,            0, 0x02 );
static USERPREF_ENTRY( COMBOBOXANIMATION,        0, 0x04 );
static USERPREF_ENTRY( LISTBOXSMOOTHSCROLLING,   0, 0x08 );
static USERPREF_ENTRY( GRADIENTCAPTIONS,         0, 0x10 );
static USERPREF_ENTRY( KEYBOARDCUES,             0, 0x20 );
static USERPREF_ENTRY( ACTIVEWNDTRKZORDER,       0, 0x40 );
static USERPREF_ENTRY( HOTTRACKING,              0, 0x80 );
static USERPREF_ENTRY( MENUFADE,                 1, 0x02 );
static USERPREF_ENTRY( SELECTIONFADE,            1, 0x04 );
static USERPREF_ENTRY( TOOLTIPANIMATION,         1, 0x08 );
static USERPREF_ENTRY( TOOLTIPFADE,              1, 0x10 );
static USERPREF_ENTRY( CURSORSHADOW,             1, 0x20 );
static USERPREF_ENTRY( MOUSESONAR,               1, 0x40 );
static USERPREF_ENTRY( MOUSECLICKLOCK,           1, 0x80 );
static USERPREF_ENTRY( MOUSEVANISH,              2, 0x01 );
static USERPREF_ENTRY( FLATMENU,                 2, 0x02 );
static USERPREF_ENTRY( DROPSHADOW,               2, 0x04 );
static USERPREF_ENTRY( UIEFFECTS,                3, 0x80 );
static USERPREF_ENTRY( DISABLEOVERLAPPEDCONTENT, 4, 0x01 );
static USERPREF_ENTRY( CLIENTAREAANIMATION,      4, 0x02 );
static USERPREF_ENTRY( CLEARTYPE,                4, 0x10 );
static USERPREF_ENTRY( SPEECHRECOGNITION,        4, 0x20 );

/* System parameter indexes */
enum spi_index
{
    SPI_SETWORKAREA_IDX,
    SPI_INDEX_COUNT
};

/* indicators whether system parameter value is loaded */
static char spi_loaded[SPI_INDEX_COUNT];

static struct sysparam_rgb_entry system_colors[] =
{
#define RGB_ENTRY(name,val,reg) { { get_rgb_entry, set_rgb_entry, init_rgb_entry, COLORS_KEY, reg }, (val) }
    RGB_ENTRY( COLOR_SCROLLBAR, RGB(212, 208, 200), "Scrollbar" ),
    RGB_ENTRY( COLOR_BACKGROUND, RGB(58, 110, 165), "Background" ),
    RGB_ENTRY( COLOR_ACTIVECAPTION, RGB(10, 36, 106), "ActiveTitle" ),
    RGB_ENTRY( COLOR_INACTIVECAPTION, RGB(128, 128, 128), "InactiveTitle" ),
    RGB_ENTRY( COLOR_MENU, RGB(212, 208, 200), "Menu" ),
    RGB_ENTRY( COLOR_WINDOW, RGB(255, 255, 255), "Window" ),
    RGB_ENTRY( COLOR_WINDOWFRAME, RGB(0, 0, 0), "WindowFrame" ),
    RGB_ENTRY( COLOR_MENUTEXT, RGB(0, 0, 0), "MenuText" ),
    RGB_ENTRY( COLOR_WINDOWTEXT, RGB(0, 0, 0), "WindowText" ),
    RGB_ENTRY( COLOR_CAPTIONTEXT, RGB(255, 255, 255), "TitleText" ),
    RGB_ENTRY( COLOR_ACTIVEBORDER, RGB(212, 208, 200), "ActiveBorder" ),
    RGB_ENTRY( COLOR_INACTIVEBORDER, RGB(212, 208, 200), "InactiveBorder" ),
    RGB_ENTRY( COLOR_APPWORKSPACE, RGB(128, 128, 128), "AppWorkSpace" ),
    RGB_ENTRY( COLOR_HIGHLIGHT, RGB(10, 36, 106), "Hilight" ),
    RGB_ENTRY( COLOR_HIGHLIGHTTEXT, RGB(255, 255, 255), "HilightText" ),
    RGB_ENTRY( COLOR_BTNFACE, RGB(212, 208, 200), "ButtonFace" ),
    RGB_ENTRY( COLOR_BTNSHADOW, RGB(128, 128, 128), "ButtonShadow" ),
    RGB_ENTRY( COLOR_GRAYTEXT, RGB(128, 128, 128), "GrayText" ),
    RGB_ENTRY( COLOR_BTNTEXT, RGB(0, 0, 0), "ButtonText" ),
    RGB_ENTRY( COLOR_INACTIVECAPTIONTEXT, RGB(212, 208, 200), "InactiveTitleText" ),
    RGB_ENTRY( COLOR_BTNHIGHLIGHT, RGB(255, 255, 255), "ButtonHilight" ),
    RGB_ENTRY( COLOR_3DDKSHADOW, RGB(64, 64, 64), "ButtonDkShadow" ),
    RGB_ENTRY( COLOR_3DLIGHT, RGB(212, 208, 200), "ButtonLight" ),
    RGB_ENTRY( COLOR_INFOTEXT, RGB(0, 0, 0), "InfoText" ),
    RGB_ENTRY( COLOR_INFOBK, RGB(255, 255, 225), "InfoWindow" ),
    RGB_ENTRY( COLOR_ALTERNATEBTNFACE, RGB(181, 181, 181), "ButtonAlternateFace" ),
    RGB_ENTRY( COLOR_HOTLIGHT, RGB(0, 0, 200), "HotTrackingColor" ),
    RGB_ENTRY( COLOR_GRADIENTACTIVECAPTION, RGB(166, 202, 240), "GradientActiveTitle" ),
    RGB_ENTRY( COLOR_GRADIENTINACTIVECAPTION, RGB(192, 192, 192), "GradientInactiveTitle" ),
    RGB_ENTRY( COLOR_MENUHILIGHT, RGB(10, 36, 106), "MenuHilight" ),
    RGB_ENTRY( COLOR_MENUBAR, RGB(212, 208, 200), "MenuBar" )
#undef RGB_ENTRY
};

/* entries that are initialized by default in the registry */
static union sysparam_all_entry * const default_entries[] =
{
    (union sysparam_all_entry *)&entry_ACTIVEWINDOWTRACKING,
    (union sysparam_all_entry *)&entry_ACTIVEWNDTRKTIMEOUT,
    (union sysparam_all_entry *)&entry_BEEP,
    (union sysparam_all_entry *)&entry_BLOCKSENDINPUTRESETS,
    (union sysparam_all_entry *)&entry_BORDER,
    (union sysparam_all_entry *)&entry_CAPTIONHEIGHT,
    (union sysparam_all_entry *)&entry_CAPTIONWIDTH,
    (union sysparam_all_entry *)&entry_CARETWIDTH,
    (union sysparam_all_entry *)&entry_DESKWALLPAPER,
    (union sysparam_all_entry *)&entry_DOUBLECLICKTIME,
    (union sysparam_all_entry *)&entry_DOUBLECLKHEIGHT,
    (union sysparam_all_entry *)&entry_DOUBLECLKWIDTH,
    (union sysparam_all_entry *)&entry_DRAGFULLWINDOWS,
    (union sysparam_all_entry *)&entry_DRAGHEIGHT,
    (union sysparam_all_entry *)&entry_DRAGWIDTH,
    (union sysparam_all_entry *)&entry_FOCUSBORDERHEIGHT,
    (union sysparam_all_entry *)&entry_FOCUSBORDERWIDTH,
    (union sysparam_all_entry *)&entry_FONTSMOOTHING,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGCONTRAST,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGORIENTATION,
    (union sysparam_all_entry *)&entry_FONTSMOOTHINGTYPE,
    (union sysparam_all_entry *)&entry_FOREGROUNDFLASHCOUNT,
    (union sysparam_all_entry *)&entry_FOREGROUNDLOCKTIMEOUT,
    (union sysparam_all_entry *)&entry_ICONHORIZONTALSPACING,
    (union sysparam_all_entry *)&entry_ICONTITLEWRAP,
    (union sysparam_all_entry *)&entry_ICONVERTICALSPACING,
    (union sysparam_all_entry *)&entry_KEYBOARDDELAY,
    (union sysparam_all_entry *)&entry_KEYBOARDPREF,
    (union sysparam_all_entry *)&entry_KEYBOARDSPEED,
    (union sysparam_all_entry *)&entry_LOWPOWERACTIVE,
    (union sysparam_all_entry *)&entry_MENUHEIGHT,
    (union sysparam_all_entry *)&entry_MENUSHOWDELAY,
    (union sysparam_all_entry *)&entry_MENUWIDTH,
    (union sysparam_all_entry *)&entry_MOUSEACCELERATION,
    (union sysparam_all_entry *)&entry_MOUSEBUTTONSWAP,
    (union sysparam_all_entry *)&entry_MOUSECLICKLOCKTIME,
    (union sysparam_all_entry *)&entry_MOUSEHOVERHEIGHT,
    (union sysparam_all_entry *)&entry_MOUSEHOVERTIME,
    (union sysparam_all_entry *)&entry_MOUSEHOVERWIDTH,
    (union sysparam_all_entry *)&entry_MOUSESPEED,
    (union sysparam_all_entry *)&entry_MOUSETHRESHOLD1,
    (union sysparam_all_entry *)&entry_MOUSETHRESHOLD2,
    (union sysparam_all_entry *)&entry_PADDEDBORDERWIDTH,
    (union sysparam_all_entry *)&entry_SCREENREADER,
    (union sysparam_all_entry *)&entry_SCROLLHEIGHT,
    (union sysparam_all_entry *)&entry_SCROLLWIDTH,
    (union sysparam_all_entry *)&entry_SHOWSOUNDS,
    (union sysparam_all_entry *)&entry_SMCAPTIONHEIGHT,
    (union sysparam_all_entry *)&entry_SMCAPTIONWIDTH,
    (union sysparam_all_entry *)&entry_SNAPTODEFBUTTON,
    (union sysparam_all_entry *)&entry_USERPREFERENCESMASK,
    (union sysparam_all_entry *)&entry_WHEELSCROLLCHARS,
    (union sysparam_all_entry *)&entry_WHEELSCROLLLINES,
    (union sysparam_all_entry *)&entry_AUDIODESC_LOCALE,
    (union sysparam_all_entry *)&entry_AUDIODESC_ON,
};

void sysparams_init(void)
{

    DWORD i, dispos, dpi_scaling;
    WCHAR layout[KL_NAMELENGTH];
    pthread_mutexattr_t attr;
    HKEY hkey;

    static const WCHAR software_wineW[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e'};
    static const WCHAR temporary_system_parametersW[] =
        {'T','e','m','p','o','r','a','r','y',' ','S','y','s','t','e','m',' ',
         'P','a','r','a','m','e','t','e','r','s'};
    static const WCHAR oneW[] = {'1',0};
    static const WCHAR kl_preloadW[] =
        {'K','e','y','b','o','a','r','d',' ','L','a','y','o','u','t','\\','P','r','e','l','o','a','d'};

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &user_mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    if ((hkey = reg_create_key( hkcu_key, kl_preloadW, sizeof(kl_preloadW), 0, NULL )))
    {
        if (NtUserGetKeyboardLayoutName( layout ))
            set_reg_value( hkey, oneW, REG_SZ, (const BYTE *)layout,
                           (lstrlenW(layout) + 1) * sizeof(WCHAR) );
        NtClose( hkey );
    }

    /* this one must be non-volatile */
    if (!(hkey = reg_create_key( hkcu_key, software_wineW, sizeof(software_wineW), 0, NULL )))
    {
        ERR("Can't create wine registry branch\n");
        return;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Temporary System Parameters */
    if (!(volatile_base_key = reg_create_key( hkey, temporary_system_parametersW,
                                              sizeof(temporary_system_parametersW),
                                              REG_OPTION_VOLATILE, &dispos )))
        ERR("Can't create non-permanent wine registry branch\n");

    NtClose( hkey );

    config_key = reg_create_key( NULL, config_keyW, sizeof(config_keyW), 0, NULL );

    get_dword_entry( (union sysparam_all_entry *)&entry_LOGPIXELS, 0, &system_dpi, 0 );
    if (!system_dpi)  /* check fallback key */
    {
        static const WCHAR log_pixelsW[] = {'L','o','g','P','i','x','e','l','s',0};
        static const WCHAR software_fontsW[] =
            {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s'};

        if ((hkey = reg_open_key( config_key, software_fontsW, sizeof(software_fontsW) )))
        {
            char buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
            KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;

            if (query_reg_value( hkey, log_pixelsW, value, sizeof(buffer) ) && value->Type == REG_DWORD)
                system_dpi = *(const DWORD *)value->Data;
            NtClose( hkey );
        }
    }
    if (!system_dpi) system_dpi = USER_DEFAULT_SCREEN_DPI;

    /* FIXME: what do the DpiScalingVer flags mean? */
    get_dword_entry( (union sysparam_all_entry *)&entry_DPISCALINGVER, 0, &dpi_scaling, 0 );
    if (!dpi_scaling) NtUserSetProcessDpiAwarenessContext( NTUSER_DPI_PER_MONITOR_AWARE, 0 );

    if (volatile_base_key && dispos == REG_CREATED_NEW_KEY)  /* first process, initialize entries */
    {
        for (i = 0; i < ARRAY_SIZE( default_entries ); i++)
            default_entries[i]->hdr.init( default_entries[i] );
    }
}

static BOOL update_desktop_wallpaper(void)
{
    /* FIXME: move implementation from user32 */
    entry_DESKWALLPAPER.hdr.loaded = entry_DESKPATTERN.hdr.loaded = FALSE;
    return TRUE;
}

/***********************************************************************
 *	     NtUserSystemParametersInfoForDpi   (win32u.@)
 */
BOOL WINAPI NtUserSystemParametersInfoForDpi( UINT action, UINT val, PVOID ptr, UINT winini, UINT dpi )
{
    BOOL ret = FALSE;

    switch (action)
    {
    case SPI_GETICONTITLELOGFONT:
        ret = get_entry_dpi( &entry_ICONTITLELOGFONT, val, ptr, dpi );
        break;
    case SPI_GETNONCLIENTMETRICS:
    {
        NONCLIENTMETRICSW *ncm = ptr;

        if (!ncm) break;
        ret = get_entry_dpi( &entry_BORDER, 0, &ncm->iBorderWidth, dpi ) &&
              get_entry_dpi( &entry_SCROLLWIDTH, 0, &ncm->iScrollWidth, dpi ) &&
              get_entry_dpi( &entry_SCROLLHEIGHT, 0, &ncm->iScrollHeight, dpi ) &&
              get_entry_dpi( &entry_CAPTIONWIDTH, 0, &ncm->iCaptionWidth, dpi ) &&
              get_entry_dpi( &entry_CAPTIONHEIGHT, 0, &ncm->iCaptionHeight, dpi ) &&
              get_entry_dpi( &entry_CAPTIONLOGFONT, 0, &ncm->lfCaptionFont, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONWIDTH, 0, &ncm->iSmCaptionWidth, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONHEIGHT, 0, &ncm->iSmCaptionHeight, dpi ) &&
              get_entry_dpi( &entry_SMCAPTIONLOGFONT, 0, &ncm->lfSmCaptionFont, dpi ) &&
              get_entry_dpi( &entry_MENUWIDTH, 0, &ncm->iMenuWidth, dpi ) &&
              get_entry_dpi( &entry_MENUHEIGHT, 0, &ncm->iMenuHeight, dpi ) &&
              get_entry_dpi( &entry_MENULOGFONT, 0, &ncm->lfMenuFont, dpi ) &&
              get_entry_dpi( &entry_STATUSLOGFONT, 0, &ncm->lfStatusFont, dpi ) &&
              get_entry_dpi( &entry_MESSAGELOGFONT, 0, &ncm->lfMessageFont, dpi );
        if (ret && ncm->cbSize == sizeof(NONCLIENTMETRICSW))
            ret = get_entry_dpi( &entry_PADDEDBORDERWIDTH, 0, &ncm->iPaddedBorderWidth, dpi );
        normalize_nonclientmetrics( ncm );
        break;
    }
    case SPI_GETICONMETRICS:
    {
	ICONMETRICSW *im = ptr;
	if (im && im->cbSize == sizeof(*im))
            ret = get_entry_dpi( &entry_ICONHORIZONTALSPACING, 0, &im->iHorzSpacing, dpi ) &&
                  get_entry_dpi( &entry_ICONVERTICALSPACING, 0, &im->iVertSpacing, dpi ) &&
                  get_entry_dpi( &entry_ICONTITLEWRAP, 0, &im->iTitleWrap, dpi ) &&
                  get_entry_dpi( &entry_ICONTITLELOGFONT, 0, &im->lfFont, dpi );
	break;
    }
    default:
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        break;
    }
    return ret;
}

/***********************************************************************
 *	     NtUserSystemParametersInfo    (win32u.@)
 *
 *     Each system parameter has flag which shows whether the parameter
 * is loaded or not. Parameters, stored directly in SysParametersInfo are
 * loaded from registry only when they are requested and the flag is
 * "false", after the loading the flag is set to "true". On interprocess
 * notification of the parameter change the corresponding parameter flag is
 * set to "false". The parameter value will be reloaded when it is requested
 * the next time.
 *     Parameters, backed by or depend on GetSystemMetrics are processed
 * differently. These parameters are always loaded. They are reloaded right
 * away on interprocess change notification. We can't do lazy loading because
 * we don't want to complicate GetSystemMetrics.
 *     Parameters backed by driver settings are read from corresponding setting.
 * On the parameter change request the setting is changed. Interprocess change
 * notifications are ignored.
 *     When parameter value is updated the changed value is stored in permanent
 * registry branch if saving is requested. Otherwise it is stored
 * in temporary branch
 *
 * Some SPI values can also be stored as Twips values in the registry,
 * don't forget the conversion!
 */
BOOL WINAPI NtUserSystemParametersInfo( UINT action, UINT val, void *ptr, UINT winini )
{
#define WINE_SPI_FIXME(x) \
    case x: \
        { \
            static BOOL warn = TRUE; \
            if (warn) \
            { \
                warn = FALSE; \
                FIXME( "Unimplemented action: %u (%s)\n", x, #x ); \
            } \
        } \
        RtlSetLastWin32Error( ERROR_INVALID_SPI_VALUE ); \
        ret = FALSE; \
        break
#define WINE_SPI_WARN(x) \
    case x: \
        WARN( "Ignored action: %u (%s)\n", x, #x ); \
        ret = TRUE; \
        break

    BOOL ret = user_driver->pSystemParametersInfo( action, val, ptr, winini );
    unsigned spi_idx = 0;

    if (!ret) switch (action)
    {
    case SPI_GETBEEP:
        ret = get_entry( &entry_BEEP, val, ptr );
        break;
    case SPI_SETBEEP:
        ret = set_entry( &entry_BEEP, val, ptr, winini );
        break;
    case SPI_GETMOUSE:
        ret = get_entry( &entry_MOUSETHRESHOLD1, val, (INT *)ptr ) &&
              get_entry( &entry_MOUSETHRESHOLD2, val, (INT *)ptr + 1 ) &&
              get_entry( &entry_MOUSEACCELERATION, val, (INT *)ptr + 2 );
        break;
    case SPI_SETMOUSE:
        ret = set_entry( &entry_MOUSETHRESHOLD1, ((INT *)ptr)[0], ptr, winini ) &&
              set_entry( &entry_MOUSETHRESHOLD2, ((INT *)ptr)[1], ptr, winini ) &&
              set_entry( &entry_MOUSEACCELERATION, ((INT *)ptr)[2], ptr, winini );
        break;
    case SPI_GETBORDER:
        ret = get_entry( &entry_BORDER, val, ptr );
        if (*(INT*)ptr < 1) *(INT*)ptr = 1;
        break;
    case SPI_SETBORDER:
        ret = set_entry( &entry_BORDER, val, ptr, winini );
        break;
    case SPI_GETKEYBOARDSPEED:
        ret = get_entry( &entry_KEYBOARDSPEED, val, ptr );
        break;
    case SPI_SETKEYBOARDSPEED:
        if (val > 31) val = 31;
        ret = set_entry( &entry_KEYBOARDSPEED, val, ptr, winini );
        break;

    WINE_SPI_WARN(SPI_LANGDRIVER); /* not implemented in Windows */

    case SPI_ICONHORIZONTALSPACING:
        if (ptr != NULL)
            ret = get_entry( &entry_ICONHORIZONTALSPACING, val, ptr );
        else
        {
            int min_val = map_to_dpi( 32, get_system_dpi() );
            ret = set_entry( &entry_ICONHORIZONTALSPACING, max( min_val, val ), ptr, winini );
        }
        break;
    case SPI_GETSCREENSAVETIMEOUT:
        ret = get_entry( &entry_SCREENSAVETIMEOUT, val, ptr );
        break;
    case SPI_SETSCREENSAVETIMEOUT:
        ret = set_entry( &entry_SCREENSAVETIMEOUT, val, ptr, winini );
        break;
    case SPI_GETSCREENSAVEACTIVE:
        ret = get_entry( &entry_SCREENSAVEACTIVE, val, ptr );
        break;
    case SPI_SETSCREENSAVEACTIVE:
        ret = set_entry( &entry_SCREENSAVEACTIVE, val, ptr, winini );
        break;
    case SPI_GETGRIDGRANULARITY:
        ret = get_entry( &entry_GRIDGRANULARITY, val, ptr );
        break;
    case SPI_SETGRIDGRANULARITY:
        ret = set_entry( &entry_GRIDGRANULARITY, val, ptr, winini );
        break;
    case SPI_SETDESKWALLPAPER:
        if (!ptr || set_entry( &entry_DESKWALLPAPER, val, ptr, winini ))
            ret = update_desktop_wallpaper();
        break;
    case SPI_SETDESKPATTERN:
        if (!ptr || set_entry( &entry_DESKPATTERN, val, ptr, winini ))
            ret = update_desktop_wallpaper();
        break;
    case SPI_GETKEYBOARDDELAY:
        ret = get_entry( &entry_KEYBOARDDELAY, val, ptr );
        break;
    case SPI_SETKEYBOARDDELAY:
        ret = set_entry( &entry_KEYBOARDDELAY, val, ptr, winini );
        break;
    case SPI_ICONVERTICALSPACING:
        if (ptr != NULL)
            ret = get_entry( &entry_ICONVERTICALSPACING, val, ptr );
        else
        {
            int min_val = map_to_dpi( 32, get_system_dpi() );
            ret = set_entry( &entry_ICONVERTICALSPACING, max( min_val, val ), ptr, winini );
        }
        break;
    case SPI_GETICONTITLEWRAP:
        ret = get_entry( &entry_ICONTITLEWRAP, val, ptr );
        break;
    case SPI_SETICONTITLEWRAP:
        ret = set_entry( &entry_ICONTITLEWRAP, val, ptr, winini );
        break;
    case SPI_GETMENUDROPALIGNMENT:
        ret = get_entry( &entry_MENUDROPALIGNMENT, val, ptr );
        break;
    case SPI_SETMENUDROPALIGNMENT:
        ret = set_entry( &entry_MENUDROPALIGNMENT, val, ptr, winini );
        break;
    case SPI_SETDOUBLECLKWIDTH:
        ret = set_entry( &entry_DOUBLECLKWIDTH, val, ptr, winini );
        break;
    case SPI_SETDOUBLECLKHEIGHT:
        ret = set_entry( &entry_DOUBLECLKHEIGHT, val, ptr, winini );
        break;
    case SPI_GETICONTITLELOGFONT:
        ret = get_entry( &entry_ICONTITLELOGFONT, val, ptr );
        break;
    case SPI_SETDOUBLECLICKTIME:
        ret = set_entry( &entry_DOUBLECLICKTIME, val, ptr, winini );
        break;
    case SPI_SETMOUSEBUTTONSWAP:
        ret = set_entry( &entry_MOUSEBUTTONSWAP, val, ptr, winini );
        break;
    case SPI_SETICONTITLELOGFONT:
        ret = set_entry( &entry_ICONTITLELOGFONT, val, ptr, winini );
        break;
    case SPI_GETFASTTASKSWITCH:
        if (!ptr) return FALSE;
        *(BOOL *)ptr = TRUE;
        ret = TRUE;
        break;
    case SPI_SETFASTTASKSWITCH:
        /* the action is disabled */
        ret = FALSE;
        break;
    case SPI_SETDRAGFULLWINDOWS:
        ret = set_entry( &entry_DRAGFULLWINDOWS, val, ptr, winini );
        break;
    case SPI_GETDRAGFULLWINDOWS:
        ret = get_entry( &entry_DRAGFULLWINDOWS, val, ptr );
        break;
    case SPI_GETNONCLIENTMETRICS:
    {
        NONCLIENTMETRICSW *nm = ptr;
        int padded_border;

        if (!ptr) return FALSE;

        ret = get_entry( &entry_BORDER, 0, &nm->iBorderWidth ) &&
              get_entry( &entry_PADDEDBORDERWIDTH, 0, &padded_border ) &&
              get_entry( &entry_SCROLLWIDTH, 0, &nm->iScrollWidth ) &&
              get_entry( &entry_SCROLLHEIGHT, 0, &nm->iScrollHeight ) &&
              get_entry( &entry_CAPTIONWIDTH, 0, &nm->iCaptionWidth ) &&
              get_entry( &entry_CAPTIONHEIGHT, 0, &nm->iCaptionHeight ) &&
              get_entry( &entry_CAPTIONLOGFONT, 0, &nm->lfCaptionFont ) &&
              get_entry( &entry_SMCAPTIONWIDTH, 0, &nm->iSmCaptionWidth ) &&
              get_entry( &entry_SMCAPTIONHEIGHT, 0, &nm->iSmCaptionHeight ) &&
              get_entry( &entry_SMCAPTIONLOGFONT, 0, &nm->lfSmCaptionFont ) &&
              get_entry( &entry_MENUWIDTH, 0, &nm->iMenuWidth ) &&
              get_entry( &entry_MENUHEIGHT, 0, &nm->iMenuHeight ) &&
              get_entry( &entry_MENULOGFONT, 0, &nm->lfMenuFont ) &&
              get_entry( &entry_STATUSLOGFONT, 0, &nm->lfStatusFont ) &&
              get_entry( &entry_MESSAGELOGFONT, 0, &nm->lfMessageFont );
        if (ret)
        {
            nm->iBorderWidth += padded_border;
            if (nm->cbSize == sizeof(NONCLIENTMETRICSW)) nm->iPaddedBorderWidth = 0;
        }
        normalize_nonclientmetrics( nm );
        break;
    }
    case SPI_SETNONCLIENTMETRICS:
    {
        LPNONCLIENTMETRICSW nm = ptr;
        int padded_border;

        if (nm && (nm->cbSize == sizeof(NONCLIENTMETRICSW) ||
                     nm->cbSize == FIELD_OFFSET(NONCLIENTMETRICSW, iPaddedBorderWidth)))
        {
            get_entry( &entry_PADDEDBORDERWIDTH, 0, &padded_border );

            ret = set_entry( &entry_BORDER, nm->iBorderWidth - padded_border, NULL, winini ) &&
                  set_entry( &entry_SCROLLWIDTH, nm->iScrollWidth, NULL, winini ) &&
                  set_entry( &entry_SCROLLHEIGHT, nm->iScrollHeight, NULL, winini ) &&
                  set_entry( &entry_CAPTIONWIDTH, nm->iCaptionWidth, NULL, winini ) &&
                  set_entry( &entry_CAPTIONHEIGHT, nm->iCaptionHeight, NULL, winini ) &&
                  set_entry( &entry_SMCAPTIONWIDTH, nm->iSmCaptionWidth, NULL, winini ) &&
                  set_entry( &entry_SMCAPTIONHEIGHT, nm->iSmCaptionHeight, NULL, winini ) &&
                  set_entry( &entry_MENUWIDTH, nm->iMenuWidth, NULL, winini ) &&
                  set_entry( &entry_MENUHEIGHT, nm->iMenuHeight, NULL, winini ) &&
                  set_entry( &entry_MENULOGFONT, 0, &nm->lfMenuFont, winini ) &&
                  set_entry( &entry_CAPTIONLOGFONT, 0, &nm->lfCaptionFont, winini ) &&
                  set_entry( &entry_SMCAPTIONLOGFONT, 0, &nm->lfSmCaptionFont, winini ) &&
                  set_entry( &entry_STATUSLOGFONT, 0, &nm->lfStatusFont, winini ) &&
                  set_entry( &entry_MESSAGELOGFONT, 0, &nm->lfMessageFont, winini );
        }
        break;
    }
    case SPI_GETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS *mm = ptr;
        if (mm && mm->cbSize == sizeof(*mm)) {
            ret = get_entry( &entry_MINWIDTH, 0, &mm->iWidth ) &&
                  get_entry( &entry_MINHORZGAP, 0, &mm->iHorzGap ) &&
                  get_entry( &entry_MINVERTGAP, 0, &mm->iVertGap ) &&
                  get_entry( &entry_MINARRANGE, 0, &mm->iArrange );
            mm->iWidth = max( 0, mm->iWidth );
            mm->iHorzGap = max( 0, mm->iHorzGap );
            mm->iVertGap = max( 0, mm->iVertGap );
            mm->iArrange &= 0x0f;
        }
        break;
    }
    case SPI_SETMINIMIZEDMETRICS:
    {
        MINIMIZEDMETRICS *mm = ptr;
        if (mm && mm->cbSize == sizeof(*mm))
            ret = set_entry( &entry_MINWIDTH, max( 0, mm->iWidth ), NULL, winini ) &&
                  set_entry( &entry_MINHORZGAP, max( 0, mm->iHorzGap ), NULL, winini ) &&
                  set_entry( &entry_MINVERTGAP, max( 0, mm->iVertGap ), NULL, winini ) &&
                  set_entry( &entry_MINARRANGE, mm->iArrange & 0x0f, NULL, winini );
        break;
    }
    case SPI_GETICONMETRICS:
    {
	ICONMETRICSW *icon = ptr;
	if(icon && icon->cbSize == sizeof(*icon))
	{
            ret = get_entry( &entry_ICONHORIZONTALSPACING, 0, &icon->iHorzSpacing ) &&
                  get_entry( &entry_ICONVERTICALSPACING, 0, &icon->iVertSpacing ) &&
                  get_entry( &entry_ICONTITLEWRAP, 0, &icon->iTitleWrap ) &&
                  get_entry( &entry_ICONTITLELOGFONT, 0, &icon->lfFont );
	}
	break;
    }
    case SPI_SETICONMETRICS:
    {
        ICONMETRICSW *icon = ptr;
        if (icon && icon->cbSize == sizeof(*icon))
            ret = set_entry( &entry_ICONVERTICALSPACING, max(32,icon->iVertSpacing), NULL, winini ) &&
                  set_entry( &entry_ICONHORIZONTALSPACING, max(32,icon->iHorzSpacing), NULL, winini ) &&
                  set_entry( &entry_ICONTITLEWRAP, icon->iTitleWrap, NULL, winini ) &&
                  set_entry( &entry_ICONTITLELOGFONT, 0, &icon->lfFont, winini );
        break;
    }
    case SPI_SETWORKAREA:
    {
        if (!ptr) return FALSE;
        spi_idx = SPI_SETWORKAREA_IDX;
        work_area = *(RECT*)ptr;
        spi_loaded[spi_idx] = TRUE;
        ret = TRUE;
        break;
    }
    case SPI_GETWORKAREA:
    {
        if (!ptr) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        if (!spi_loaded[spi_idx])
        {
            struct monitor *monitor;

            if (!lock_display_devices()) return FALSE;

            LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
            {
                if (!(monitor->flags & MONITORINFOF_PRIMARY)) continue;
                work_area = monitor->rc_work;
                break;
            }

            unlock_display_devices();
            spi_loaded[spi_idx] = TRUE;
        }
        *(RECT *)ptr = map_dpi_rect( work_area, system_dpi, get_thread_dpi() );
        ret = TRUE;
        TRACE("work area %s\n", wine_dbgstr_rect( &work_area ));
        break;
    }

    WINE_SPI_FIXME(SPI_SETPENWINDOWS);

    case SPI_GETFILTERKEYS:
    {
        LPFILTERKEYS filter_keys = ptr;
        WARN("SPI_GETFILTERKEYS not fully implemented\n");
        if (filter_keys && filter_keys->cbSize == sizeof(FILTERKEYS))
        {
            /* Indicate that no FilterKeys feature available */
            filter_keys->dwFlags = 0;
            filter_keys->iWaitMSec = 0;
            filter_keys->iDelayMSec = 0;
            filter_keys->iRepeatMSec = 0;
            filter_keys->iBounceMSec = 0;
            ret = TRUE;
        }
        break;
    }
    WINE_SPI_FIXME(SPI_SETFILTERKEYS);

    case SPI_GETTOGGLEKEYS:
    {
        LPTOGGLEKEYS toggle_keys = ptr;
        WARN("SPI_GETTOGGLEKEYS not fully implemented\n");
        if (toggle_keys && toggle_keys->cbSize == sizeof(TOGGLEKEYS))
        {
            /* Indicate that no ToggleKeys feature available */
            toggle_keys->dwFlags = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETTOGGLEKEYS);

    case SPI_GETMOUSEKEYS:
    {
        MOUSEKEYS *mouse_keys = ptr;
        WARN("SPI_GETMOUSEKEYS not fully implemented\n");
        if (mouse_keys && mouse_keys->cbSize == sizeof(MOUSEKEYS))
        {
            /* Indicate that no MouseKeys feature available */
            mouse_keys->dwFlags = 0;
            mouse_keys->iMaxSpeed = 360;
            mouse_keys->iTimeToMaxSpeed = 1000;
            mouse_keys->iCtrlSpeed = 0;
            mouse_keys->dwReserved1 = 0;
            mouse_keys->dwReserved2 = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETMOUSEKEYS);

    case SPI_GETSHOWSOUNDS:
        ret = get_entry( &entry_SHOWSOUNDS, val, ptr );
        break;
    case SPI_SETSHOWSOUNDS:
        ret = set_entry( &entry_SHOWSOUNDS, val, ptr, winini );
        break;
    case SPI_GETSTICKYKEYS:
    {
        STICKYKEYS *sticky_keys = ptr;
        WARN("SPI_GETSTICKYKEYS not fully implemented\n");
        if (sticky_keys && sticky_keys->cbSize == sizeof(STICKYKEYS))
        {
            /* Indicate that no StickyKeys feature available */
            sticky_keys->dwFlags = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETSTICKYKEYS);

    case SPI_GETACCESSTIMEOUT:
    {
        ACCESSTIMEOUT *access_timeout = ptr;
        WARN("SPI_GETACCESSTIMEOUT not fully implemented\n");
        if (access_timeout && access_timeout->cbSize == sizeof(ACCESSTIMEOUT))
        {
            /* Indicate that no accessibility features timeout is available */
            access_timeout->dwFlags = 0;
            access_timeout->iTimeOutMSec = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETACCESSTIMEOUT);

    case SPI_GETSERIALKEYS:
    {
        LPSERIALKEYSW serial_keys = ptr;
        WARN("SPI_GETSERIALKEYS not fully implemented\n");
        if (serial_keys && serial_keys->cbSize == sizeof(SERIALKEYSW))
        {
            /* Indicate that no SerialKeys feature available */
            serial_keys->dwFlags = 0;
            serial_keys->lpszActivePort = NULL;
            serial_keys->lpszPort = NULL;
            serial_keys->iBaudRate = 0;
            serial_keys->iPortState = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETSERIALKEYS);

    case SPI_GETSOUNDSENTRY:
    {
        SOUNDSENTRYW *sound_sentry = ptr;
        WARN("SPI_GETSOUNDSENTRY not fully implemented\n");
        if (sound_sentry && sound_sentry->cbSize == sizeof(SOUNDSENTRYW))
        {
            /* Indicate that no SoundSentry feature available */
            sound_sentry->dwFlags = 0;
            sound_sentry->iFSTextEffect = 0;
            sound_sentry->iFSTextEffectMSec = 0;
            sound_sentry->iFSTextEffectColorBits = 0;
            sound_sentry->iFSGrafEffect = 0;
            sound_sentry->iFSGrafEffectMSec = 0;
            sound_sentry->iFSGrafEffectColor = 0;
            sound_sentry->iWindowsEffect = 0;
            sound_sentry->iWindowsEffectMSec = 0;
            sound_sentry->lpszWindowsEffectDLL = 0;
            sound_sentry->iWindowsEffectOrdinal = 0;
            ret = TRUE;
        }
        break;
    }

    WINE_SPI_FIXME(SPI_SETSOUNDSENTRY);

    case SPI_GETHIGHCONTRAST:
    {
        HIGHCONTRASTW *high_contrast = ptr;
	WARN("SPI_GETHIGHCONTRAST not fully implemented\n");
	if (high_contrast && high_contrast->cbSize == sizeof(HIGHCONTRASTW))
	{
	    /* Indicate that no high contrast feature available */
	    high_contrast->dwFlags = 0;
	    high_contrast->lpszDefaultScheme = NULL;
            ret = TRUE;
	}
	break;
    }

    WINE_SPI_FIXME(SPI_SETHIGHCONTRAST);

    case SPI_GETKEYBOARDPREF:
        ret = get_entry( &entry_KEYBOARDPREF, val, ptr );
        break;
    case SPI_SETKEYBOARDPREF:
        ret = set_entry( &entry_KEYBOARDPREF, val, ptr, winini );
        break;
    case SPI_GETSCREENREADER:
        ret = get_entry( &entry_SCREENREADER, val, ptr );
        break;
    case SPI_SETSCREENREADER:
        ret = set_entry( &entry_SCREENREADER, val, ptr, winini );
        break;

    case SPI_GETANIMATION:
    {
        ANIMATIONINFO *anim_info = ptr;

	/* Tell it "disabled" */
	if (anim_info && anim_info->cbSize == sizeof(ANIMATIONINFO))
        {
            /* Minimize and restore animation is disabled (nonzero == enabled) */
	    anim_info->iMinAnimate = 0;
            ret = TRUE;
        }
	break;
    }

    WINE_SPI_WARN(SPI_SETANIMATION);

    case SPI_GETFONTSMOOTHING:
        ret = get_entry( &entry_FONTSMOOTHING, val, ptr );
        if (ret) *(UINT *)ptr = (*(UINT *)ptr != 0);
        break;
    case SPI_SETFONTSMOOTHING:
        val = val ? 2 : 0; /* Win NT4/2k/XP behavior */
        ret = set_entry( &entry_FONTSMOOTHING, val, ptr, winini );
        break;
    case SPI_SETDRAGWIDTH:
        ret = set_entry( &entry_DRAGWIDTH, val, ptr, winini );
        break;
    case SPI_SETDRAGHEIGHT:
        ret = set_entry( &entry_DRAGHEIGHT, val, ptr, winini );
        break;

    WINE_SPI_FIXME(SPI_SETHANDHELD);
    WINE_SPI_FIXME(SPI_GETLOWPOWERTIMEOUT);
    WINE_SPI_FIXME(SPI_GETPOWEROFFTIMEOUT);
    WINE_SPI_FIXME(SPI_SETLOWPOWERTIMEOUT);
    WINE_SPI_FIXME(SPI_SETPOWEROFFTIMEOUT);

    case SPI_GETLOWPOWERACTIVE:
        ret = get_entry( &entry_LOWPOWERACTIVE, val, ptr );
        break;
    case SPI_SETLOWPOWERACTIVE:
        ret = set_entry( &entry_LOWPOWERACTIVE, val, ptr, winini );
        break;
    case SPI_GETPOWEROFFACTIVE:
        ret = get_entry( &entry_POWEROFFACTIVE, val, ptr );
        break;
    case SPI_SETPOWEROFFACTIVE:
        ret = set_entry( &entry_POWEROFFACTIVE, val, ptr, winini );
        break;

    WINE_SPI_FIXME(SPI_SETCURSORS);
    WINE_SPI_FIXME(SPI_SETICONS);

    case SPI_GETDEFAULTINPUTLANG:
        ret = NtUserGetKeyboardLayout(0) != 0;
        break;

    WINE_SPI_FIXME(SPI_SETDEFAULTINPUTLANG);
    WINE_SPI_FIXME(SPI_SETLANGTOGGLE);

    case SPI_GETWINDOWSEXTENSION:
	WARN( "pretend no support for Win9x Plus! for now.\n" );
	ret = FALSE; /* yes, this is the result value */
	break;
    case SPI_SETMOUSETRAILS:
        ret = set_entry( &entry_MOUSETRAILS, val, ptr, winini );
        break;
    case SPI_GETMOUSETRAILS:
        ret = get_entry( &entry_MOUSETRAILS, val, ptr );
        break;
    case SPI_GETSNAPTODEFBUTTON:
        ret = get_entry( &entry_SNAPTODEFBUTTON, val, ptr );
        break;
    case SPI_SETSNAPTODEFBUTTON:
        ret = set_entry( &entry_SNAPTODEFBUTTON, val, ptr, winini );
        break;
    case SPI_SETSCREENSAVERRUNNING:
        ret = set_entry( &entry_SCREENSAVERRUNNING, val, ptr, winini );
        break;
    case SPI_GETMOUSEHOVERWIDTH:
        ret = get_entry( &entry_MOUSEHOVERWIDTH, val, ptr );
        break;
    case SPI_SETMOUSEHOVERWIDTH:
        ret = set_entry( &entry_MOUSEHOVERWIDTH, val, ptr, winini );
        break;
    case SPI_GETMOUSEHOVERHEIGHT:
        ret = get_entry( &entry_MOUSEHOVERHEIGHT, val, ptr );
        break;
    case SPI_SETMOUSEHOVERHEIGHT:
        ret = set_entry( &entry_MOUSEHOVERHEIGHT, val, ptr, winini );
        break;
    case SPI_GETMOUSEHOVERTIME:
        ret = get_entry( &entry_MOUSEHOVERTIME, val, ptr );
        break;
    case SPI_SETMOUSEHOVERTIME:
        ret = set_entry( &entry_MOUSEHOVERTIME, val, ptr, winini );
        break;
    case SPI_GETWHEELSCROLLLINES:
        ret = get_entry( &entry_WHEELSCROLLLINES, val, ptr );
        break;
    case SPI_SETWHEELSCROLLLINES:
        ret = set_entry( &entry_WHEELSCROLLLINES, val, ptr, winini );
        break;
    case SPI_GETMENUSHOWDELAY:
        ret = get_entry( &entry_MENUSHOWDELAY, val, ptr );
        break;
    case SPI_SETMENUSHOWDELAY:
        ret = set_entry( &entry_MENUSHOWDELAY, val, ptr, winini );
        break;
    case SPI_GETWHEELSCROLLCHARS:
        ret = get_entry( &entry_WHEELSCROLLCHARS, val, ptr );
        break;
    case SPI_SETWHEELSCROLLCHARS:
        ret = set_entry( &entry_WHEELSCROLLCHARS, val, ptr, winini );
        break;

    WINE_SPI_FIXME(SPI_GETSHOWIMEUI);
    WINE_SPI_FIXME(SPI_SETSHOWIMEUI);

    case SPI_GETMOUSESPEED:
        ret = get_entry( &entry_MOUSESPEED, val, ptr );
        break;
    case SPI_SETMOUSESPEED:
        ret = set_entry( &entry_MOUSESPEED, val, ptr, winini );
        break;
    case SPI_GETSCREENSAVERRUNNING:
        ret = get_entry( &entry_SCREENSAVERRUNNING, val, ptr );
        break;
    case SPI_GETDESKWALLPAPER:
        ret = get_entry( &entry_DESKWALLPAPER, val, ptr );
        break;
    case SPI_GETACTIVEWINDOWTRACKING:
        ret = get_entry( &entry_ACTIVEWINDOWTRACKING, val, ptr );
        break;
    case SPI_SETACTIVEWINDOWTRACKING:
        ret = set_entry( &entry_ACTIVEWINDOWTRACKING, val, ptr, winini );
        break;
    case SPI_GETMENUANIMATION:
        ret = get_entry( &entry_MENUANIMATION, val, ptr );
        break;
    case SPI_SETMENUANIMATION:
        ret = set_entry( &entry_MENUANIMATION, val, ptr, winini );
        break;
    case SPI_GETCOMBOBOXANIMATION:
        ret = get_entry( &entry_COMBOBOXANIMATION, val, ptr );
        break;
    case SPI_SETCOMBOBOXANIMATION:
        ret = set_entry( &entry_COMBOBOXANIMATION, val, ptr, winini );
        break;
    case SPI_GETLISTBOXSMOOTHSCROLLING:
        ret = get_entry( &entry_LISTBOXSMOOTHSCROLLING, val, ptr );
        break;
    case SPI_SETLISTBOXSMOOTHSCROLLING:
        ret = set_entry( &entry_LISTBOXSMOOTHSCROLLING, val, ptr, winini );
        break;
    case SPI_GETGRADIENTCAPTIONS:
        ret = get_entry( &entry_GRADIENTCAPTIONS, val, ptr );
        break;
    case SPI_SETGRADIENTCAPTIONS:
        ret = set_entry( &entry_GRADIENTCAPTIONS, val, ptr, winini );
        break;
    case SPI_GETKEYBOARDCUES:
        ret = get_entry( &entry_KEYBOARDCUES, val, ptr );
        break;
    case SPI_SETKEYBOARDCUES:
        ret = set_entry( &entry_KEYBOARDCUES, val, ptr, winini );
        break;
    case SPI_GETACTIVEWNDTRKZORDER:
        ret = get_entry( &entry_ACTIVEWNDTRKZORDER, val, ptr );
        break;
    case SPI_SETACTIVEWNDTRKZORDER:
        ret = set_entry( &entry_ACTIVEWNDTRKZORDER, val, ptr, winini );
        break;
    case SPI_GETHOTTRACKING:
        ret = get_entry( &entry_HOTTRACKING, val, ptr );
        break;
    case SPI_SETHOTTRACKING:
        ret = set_entry( &entry_HOTTRACKING, val, ptr, winini );
        break;
    case SPI_GETMENUFADE:
        ret = get_entry( &entry_MENUFADE, val, ptr );
        break;
    case SPI_SETMENUFADE:
        ret = set_entry( &entry_MENUFADE, val, ptr, winini );
        break;
    case SPI_GETSELECTIONFADE:
        ret = get_entry( &entry_SELECTIONFADE, val, ptr );
        break;
    case SPI_SETSELECTIONFADE:
        ret = set_entry( &entry_SELECTIONFADE, val, ptr, winini );
        break;
    case SPI_GETTOOLTIPANIMATION:
        ret = get_entry( &entry_TOOLTIPANIMATION, val, ptr );
        break;
    case SPI_SETTOOLTIPANIMATION:
        ret = set_entry( &entry_TOOLTIPANIMATION, val, ptr, winini );
        break;
    case SPI_GETTOOLTIPFADE:
        ret = get_entry( &entry_TOOLTIPFADE, val, ptr );
        break;
    case SPI_SETTOOLTIPFADE:
        ret = set_entry( &entry_TOOLTIPFADE, val, ptr, winini );
        break;
    case SPI_GETCURSORSHADOW:
        ret = get_entry( &entry_CURSORSHADOW, val, ptr );
        break;
    case SPI_SETCURSORSHADOW:
        ret = set_entry( &entry_CURSORSHADOW, val, ptr, winini );
        break;
    case SPI_GETMOUSESONAR:
        ret = get_entry( &entry_MOUSESONAR, val, ptr );
        break;
    case SPI_SETMOUSESONAR:
        ret = set_entry( &entry_MOUSESONAR, val, ptr, winini );
        break;
    case SPI_GETMOUSECLICKLOCK:
        ret = get_entry( &entry_MOUSECLICKLOCK, val, ptr );
        break;
    case SPI_SETMOUSECLICKLOCK:
        ret = set_entry( &entry_MOUSECLICKLOCK, val, ptr, winini );
        break;
    case SPI_GETMOUSEVANISH:
        ret = get_entry( &entry_MOUSEVANISH, val, ptr );
        break;
    case SPI_SETMOUSEVANISH:
        ret = set_entry( &entry_MOUSEVANISH, val, ptr, winini );
        break;
    case SPI_GETFLATMENU:
        ret = get_entry( &entry_FLATMENU, val, ptr );
        break;
    case SPI_SETFLATMENU:
        ret = set_entry( &entry_FLATMENU, val, ptr, winini );
        break;
    case SPI_GETDROPSHADOW:
        ret = get_entry( &entry_DROPSHADOW, val, ptr );
        break;
    case SPI_SETDROPSHADOW:
        ret = set_entry( &entry_DROPSHADOW, val, ptr, winini );
        break;
    case SPI_GETBLOCKSENDINPUTRESETS:
        ret = get_entry( &entry_BLOCKSENDINPUTRESETS, val, ptr );
        break;
    case SPI_SETBLOCKSENDINPUTRESETS:
        ret = set_entry( &entry_BLOCKSENDINPUTRESETS, val, ptr, winini );
        break;
    case SPI_GETUIEFFECTS:
        ret = get_entry( &entry_UIEFFECTS, val, ptr );
        break;
    case SPI_SETUIEFFECTS:
        /* FIXME: this probably should mask other UI effect values when unset */
        ret = set_entry( &entry_UIEFFECTS, val, ptr, winini );
        break;
    case SPI_GETDISABLEOVERLAPPEDCONTENT:
        ret = get_entry( &entry_DISABLEOVERLAPPEDCONTENT, val, ptr );
        break;
    case SPI_SETDISABLEOVERLAPPEDCONTENT:
        ret = set_entry( &entry_DISABLEOVERLAPPEDCONTENT, val, ptr, winini );
        break;
    case SPI_GETCLIENTAREAANIMATION:
        ret = get_entry( &entry_CLIENTAREAANIMATION, val, ptr );
        break;
    case SPI_SETCLIENTAREAANIMATION:
        ret = set_entry( &entry_CLIENTAREAANIMATION, val, ptr, winini );
        break;
    case SPI_GETCLEARTYPE:
        ret = get_entry( &entry_CLEARTYPE, val, ptr );
        break;
    case SPI_SETCLEARTYPE:
        ret = set_entry( &entry_CLEARTYPE, val, ptr, winini );
        break;
    case SPI_GETSPEECHRECOGNITION:
        ret = get_entry( &entry_SPEECHRECOGNITION, val, ptr );
        break;
    case SPI_SETSPEECHRECOGNITION:
        ret = set_entry( &entry_SPEECHRECOGNITION, val, ptr, winini );
        break;
    case SPI_GETFOREGROUNDLOCKTIMEOUT:
        ret = get_entry( &entry_FOREGROUNDLOCKTIMEOUT, val, ptr );
        break;
    case SPI_SETFOREGROUNDLOCKTIMEOUT:
        /* FIXME: this should check that the calling thread
         * is able to change the foreground window */
        ret = set_entry( &entry_FOREGROUNDLOCKTIMEOUT, val, ptr, winini );
        break;
    case SPI_GETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, val, ptr );
        break;
    case SPI_SETACTIVEWNDTRKTIMEOUT:
        ret = get_entry( &entry_ACTIVEWNDTRKTIMEOUT, val, ptr );
        break;
    case SPI_GETFOREGROUNDFLASHCOUNT:
        ret = get_entry( &entry_FOREGROUNDFLASHCOUNT, val, ptr );
        break;
    case SPI_SETFOREGROUNDFLASHCOUNT:
        ret = set_entry( &entry_FOREGROUNDFLASHCOUNT, val, ptr, winini );
        break;
    case SPI_GETCARETWIDTH:
        ret = get_entry( &entry_CARETWIDTH, val, ptr );
        break;
    case SPI_SETCARETWIDTH:
        ret = set_entry( &entry_CARETWIDTH, val, ptr, winini );
        break;
    case SPI_GETMOUSECLICKLOCKTIME:
        ret = get_entry( &entry_MOUSECLICKLOCKTIME, val, ptr );
        break;
    case SPI_SETMOUSECLICKLOCKTIME:
        ret = set_entry( &entry_MOUSECLICKLOCKTIME, val, ptr, winini );
        break;
    case SPI_GETFONTSMOOTHINGTYPE:
        ret = get_entry( &entry_FONTSMOOTHINGTYPE, val, ptr );
        break;
    case SPI_SETFONTSMOOTHINGTYPE:
        ret = set_entry( &entry_FONTSMOOTHINGTYPE, val, ptr, winini );
        break;
    case SPI_GETFONTSMOOTHINGCONTRAST:
        ret = get_entry( &entry_FONTSMOOTHINGCONTRAST, val, ptr );
        break;
    case SPI_SETFONTSMOOTHINGCONTRAST:
        ret = set_entry( &entry_FONTSMOOTHINGCONTRAST, val, ptr, winini );
        break;
    case SPI_GETFOCUSBORDERWIDTH:
        ret = get_entry( &entry_FOCUSBORDERWIDTH, val, ptr );
        break;
    case SPI_GETFOCUSBORDERHEIGHT:
        ret = get_entry( &entry_FOCUSBORDERHEIGHT, val, ptr );
        break;
    case SPI_SETFOCUSBORDERWIDTH:
        ret = set_entry( &entry_FOCUSBORDERWIDTH, val, ptr, winini );
        break;
    case SPI_SETFOCUSBORDERHEIGHT:
        ret = set_entry( &entry_FOCUSBORDERHEIGHT, val, ptr, winini );
        break;
    case SPI_GETFONTSMOOTHINGORIENTATION:
        ret = get_entry( &entry_FONTSMOOTHINGORIENTATION, val, ptr );
        break;
    case SPI_SETFONTSMOOTHINGORIENTATION:
        ret = set_entry( &entry_FONTSMOOTHINGORIENTATION, val, ptr, winini );
        break;
    case SPI_GETAUDIODESCRIPTION:
    {
        AUDIODESCRIPTION *audio = ptr;
        if (audio && audio->cbSize == sizeof(AUDIODESCRIPTION) && val == sizeof(AUDIODESCRIPTION) )
        {
            ret = get_entry( &entry_AUDIODESC_ON, 0, &audio->Enabled ) &&
                  get_entry( &entry_AUDIODESC_LOCALE, 0, &audio->Locale );
        }
        break;
    }
    case SPI_SETAUDIODESCRIPTION:
    {
        AUDIODESCRIPTION *audio = ptr;
        if (audio && audio->cbSize == sizeof(AUDIODESCRIPTION) && val == sizeof(AUDIODESCRIPTION) )
        {
            ret = set_entry( &entry_AUDIODESC_ON, 0, &audio->Enabled, winini) &&
                  set_entry( &entry_AUDIODESC_LOCALE, 0, &audio->Locale, winini );
        }
        break;
    }
    default:
	FIXME( "Unknown action: %u\n", action );
	RtlSetLastWin32Error( ERROR_INVALID_SPI_VALUE );
	ret = FALSE;
	break;
    }

    if (ret && (winini & SPIF_UPDATEINIFILE))
    {
        static const WCHAR emptyW[1];
        if (winini & (SPIF_SENDWININICHANGE | SPIF_SENDCHANGE))
            send_message_timeout( HWND_BROADCAST, WM_SETTINGCHANGE, action, (LPARAM) emptyW,
                                  SMTO_ABORTIFHUNG, 2000, FALSE );
    }
    TRACE( "(%u, %u, %p, %u) ret %d\n", action, val, ptr, winini, ret );
    return ret;

#undef WINE_SPI_FIXME
#undef WINE_SPI_WARN
}

int get_system_metrics( int index )
{
    NONCLIENTMETRICSW ncm;
    MINIMIZEDMETRICS mm;
    ICONMETRICSW im;
    RECT rect;
    UINT ret;
    HDC hdc;

    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXVSCROLL:
    case SM_CYHSCROLL:
        get_entry( &entry_SCROLLWIDTH, 0, &ret );
        return max( ret, 8 );
    case SM_CYCAPTION:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight + 1;
    case SM_CXBORDER:
    case SM_CYBORDER:
        /* SM_C{X,Y}BORDER always returns 1 regardless of 'BorderWidth' value in registry */
        return 1;
    case SM_CXDLGFRAME:
    case SM_CYDLGFRAME:
        return 3;
    case SM_CYVTHUMB:
    case SM_CXHTHUMB:
    case SM_CYVSCROLL:
    case SM_CXHSCROLL:
        get_entry( &entry_SCROLLHEIGHT, 0, &ret );
        return max( ret, 8 );
    case SM_CXICON:
    case SM_CYICON:
        return map_to_dpi( 32, get_system_dpi() );
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        ret = map_to_dpi( 32, get_system_dpi() );
        if (ret >= 64) return 64;
        if (ret >= 48) return 48;
        return 32;
    case SM_CYMENU:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iMenuHeight + 1;
    case SM_CXFULLSCREEN:
        /* see the remark for SM_CXMAXIMIZED, at least this formulation is correct */
        return get_system_metrics( SM_CXMAXIMIZED ) - 2 * get_system_metrics( SM_CXFRAME );
    case SM_CYFULLSCREEN:
        /* see the remark for SM_CYMAXIMIZED, at least this formulation is
         * correct */
        return get_system_metrics( SM_CYMAXIMIZED ) - get_system_metrics( SM_CYMIN );
    case SM_CYKANJIWINDOW:
        return 0;
    case SM_MOUSEPRESENT:
        return 1;
    case SM_DEBUG:
        return 0;
    case SM_SWAPBUTTON:
        get_entry( &entry_MOUSEBUTTONSWAP, 0, &ret );
        return ret;
    case SM_RESERVED1:
    case SM_RESERVED2:
    case SM_RESERVED3:
    case SM_RESERVED4:
        return 0;
    case SM_CXMIN:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfCaptionFont, NULL, &ret );
        release_display_dc( hdc );
        return 3 * ncm.iCaptionWidth + ncm.iCaptionHeight + 4 * ret +
               2 * get_system_metrics( SM_CXFRAME ) + 4;
    case SM_CYMIN:
        return get_system_metrics( SM_CYCAPTION ) + 2 * get_system_metrics( SM_CYFRAME );
    case SM_CXSIZE:
        get_entry( &entry_CAPTIONWIDTH, 0, &ret );
        return max( ret, 8 );
    case SM_CYSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight;
    case SM_CXFRAME:
        get_entry( &entry_BORDER, 0, &ret );
        ret = max( ret, 1 );
        return get_system_metrics( SM_CXDLGFRAME ) + ret;
    case SM_CYFRAME:
        get_entry( &entry_BORDER, 0, &ret );
        ret = max( ret, 1 );
        return get_system_metrics( SM_CYDLGFRAME ) + ret;
    case SM_CXMINTRACK:
        return get_system_metrics( SM_CXMIN );
    case SM_CYMINTRACK:
        return get_system_metrics( SM_CYMIN );
    case SM_CXDOUBLECLK:
        get_entry( &entry_DOUBLECLKWIDTH, 0, &ret );
        return ret;
    case SM_CYDOUBLECLK:
        get_entry( &entry_DOUBLECLKHEIGHT, 0, &ret );
        return ret;
    case SM_CXICONSPACING:
        im.cbSize = sizeof(im);
        NtUserSystemParametersInfo( SPI_GETICONMETRICS, sizeof(im), &im, 0 );
        return im.iHorzSpacing;
    case SM_CYICONSPACING:
        im.cbSize = sizeof(im);
        NtUserSystemParametersInfo( SPI_GETICONMETRICS, sizeof(im), &im, 0 );
        return im.iVertSpacing;
    case SM_MENUDROPALIGNMENT:
        NtUserSystemParametersInfo( SPI_GETMENUDROPALIGNMENT, 0, &ret, 0 );
        return ret;
    case SM_PENWINDOWS:
        return 0;
    case SM_DBCSENABLED:
        return ansi_cp.MaximumCharacterSize > 1;
    case SM_CMOUSEBUTTONS:
        return 3;
    case SM_SECURE:
        return 0;
    case SM_CXEDGE:
        return get_system_metrics( SM_CXBORDER ) + 1;
    case SM_CYEDGE:
        return get_system_metrics( SM_CYBORDER ) + 1;
    case SM_CXMINSPACING:
        mm.cbSize = sizeof(mm);
        NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return get_system_metrics( SM_CXMINIMIZED ) + mm.iHorzGap;
    case SM_CYMINSPACING:
        mm.cbSize = sizeof(mm);
        NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return get_system_metrics( SM_CYMINIMIZED ) + mm.iVertGap;
    case SM_CXSMICON:
    case SM_CYSMICON:
        return map_to_dpi( 16, get_system_dpi() ) & ~1;
    case SM_CYSMCAPTION:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iSmCaptionHeight + 1;
    case SM_CXSMSIZE:
        get_entry( &entry_SMCAPTIONWIDTH, 0, &ret );
        return ret;
    case SM_CYSMSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iSmCaptionHeight;
    case SM_CXMENUSIZE:
        get_entry( &entry_MENUWIDTH, 0, &ret );
        return ret;
    case SM_CYMENUSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iMenuHeight;
    case SM_ARRANGE:
        mm.cbSize = sizeof(mm);
        NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return mm.iArrange;
    case SM_CXMINIMIZED:
        mm.cbSize = sizeof(mm);
        NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(mm), &mm, 0 );
        return mm.iWidth + 6;
    case SM_CYMINIMIZED:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        return ncm.iCaptionHeight + 6;
    case SM_CXMAXTRACK:
        return get_system_metrics( SM_CXVIRTUALSCREEN ) + 4 + 2 * get_system_metrics( SM_CXFRAME );
    case SM_CYMAXTRACK:
        return get_system_metrics( SM_CYVIRTUALSCREEN ) + 4 + 2 * get_system_metrics( SM_CYFRAME );
    case SM_CXMAXIMIZED:
        /* FIXME: subtract the width of any vertical application toolbars*/
        return get_system_metrics( SM_CXSCREEN ) + 2 * get_system_metrics( SM_CXFRAME );
    case SM_CYMAXIMIZED:
        /* FIXME: subtract the width of any horizontal application toolbars*/
        return get_system_metrics( SM_CYSCREEN ) + 2 * get_system_metrics( SM_CYCAPTION );
    case SM_NETWORK:
        return 3;  /* FIXME */
    case SM_CLEANBOOT:
        return 0; /* 0 = ok, 1 = failsafe, 2 = failsafe + network */
    case SM_CXDRAG:
        get_entry( &entry_DRAGWIDTH, 0, &ret );
        return ret;
    case SM_CYDRAG:
        get_entry( &entry_DRAGHEIGHT, 0, &ret );
        return ret;
    case SM_SHOWSOUNDS:
        get_entry( &entry_SHOWSOUNDS, 0, &ret );
        return ret;
    case SM_CXMENUCHECK:
    case SM_CYMENUCHECK:
    {
        TEXTMETRICW tm;
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfMenuFont, &tm, NULL );
        release_display_dc( hdc );
        return tm.tmHeight <= 0 ? 13 : ((tm.tmHeight + tm.tmExternalLeading + 1) / 2) * 2 - 1;
    }
    case SM_SLOWMACHINE:
        return 0;  /* Never true */
    case SM_MIDEASTENABLED:
        return 0;  /* FIXME */
    case SM_MOUSEWHEELPRESENT:
        return 1;
    case SM_CXSCREEN:
        rect = get_primary_monitor_rect( get_thread_dpi() );
        return rect.right - rect.left;
    case SM_CYSCREEN:
        rect = get_primary_monitor_rect( get_thread_dpi() );
        return rect.bottom - rect.top;
    case SM_XVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi() );
        return rect.left;
    case SM_YVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi() );
        return rect.top;
    case SM_CXVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi() );
        return rect.right - rect.left;
    case SM_CYVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi() );
        return rect.bottom - rect.top;
    case SM_CMONITORS:
        if (!lock_display_devices()) return FALSE;
        ret = active_monitor_count();
        unlock_display_devices();
        return ret;
    case SM_SAMEDISPLAYFORMAT:
        return 1;
    case SM_IMMENABLED:
        return 0;  /* FIXME */
    case SM_CXFOCUSBORDER:
    case SM_CYFOCUSBORDER:
        return 1;
    case SM_TABLETPC:
    case SM_MEDIACENTER:
        return 0;
    case SM_CMETRICS:
        return SM_CMETRICS;
    default:
        return 0;
    }
}

static int get_system_metrics_for_dpi( int index, unsigned int dpi )
{
    NONCLIENTMETRICSW ncm;
    ICONMETRICSW im;
    UINT ret;
    HDC hdc;

    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXVSCROLL:
    case SM_CYHSCROLL:
        get_entry_dpi( &entry_SCROLLWIDTH, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CYCAPTION:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iCaptionHeight + 1;
    case SM_CYVTHUMB:
    case SM_CXHTHUMB:
    case SM_CYVSCROLL:
    case SM_CXHSCROLL:
        get_entry_dpi( &entry_SCROLLHEIGHT, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CXICON:
    case SM_CYICON:
        return map_to_dpi( 32, dpi );
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        ret = map_to_dpi( 32, dpi );
        if (ret >= 64) return 64;
        if (ret >= 48) return 48;
        return 32;
    case SM_CYMENU:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iMenuHeight + 1;
    case SM_CXSIZE:
        get_entry_dpi( &entry_CAPTIONWIDTH, 0, &ret, dpi );
        return max( ret, 8 );
    case SM_CYSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iCaptionHeight;
    case SM_CXFRAME:
        get_entry_dpi( &entry_BORDER, 0, &ret, dpi );
        ret = max( ret, 1 );
        return get_system_metrics_for_dpi( SM_CXDLGFRAME, dpi ) + ret;
    case SM_CYFRAME:
        get_entry_dpi( &entry_BORDER, 0, &ret, dpi );
        ret = max( ret, 1 );
        return get_system_metrics_for_dpi( SM_CYDLGFRAME, dpi ) + ret;
    case SM_CXICONSPACING:
        im.cbSize = sizeof(im);
        NtUserSystemParametersInfoForDpi( SPI_GETICONMETRICS, sizeof(im), &im, 0, dpi );
        return im.iHorzSpacing;
    case SM_CYICONSPACING:
        im.cbSize = sizeof(im);
        NtUserSystemParametersInfoForDpi( SPI_GETICONMETRICS, sizeof(im), &im, 0, dpi );
        return im.iVertSpacing;
    case SM_CXSMICON:
    case SM_CYSMICON:
        return map_to_dpi( 16, dpi ) & ~1;
    case SM_CYSMCAPTION:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iSmCaptionHeight + 1;
    case SM_CXSMSIZE:
        get_entry_dpi( &entry_SMCAPTIONWIDTH, 0, &ret, dpi );
        return ret;
    case SM_CYSMSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iSmCaptionHeight;
    case SM_CXMENUSIZE:
        get_entry_dpi( &entry_MENUWIDTH, 0, &ret, dpi );
        return ret;
    case SM_CYMENUSIZE:
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        return ncm.iMenuHeight;
    case SM_CXMENUCHECK:
    case SM_CYMENUCHECK:
    {
        TEXTMETRICW tm;
        ncm.cbSize = sizeof(ncm);
        NtUserSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0, dpi );
        hdc = get_display_dc();
        get_text_metr_size( hdc, &ncm.lfMenuFont, &tm, NULL);
        release_display_dc( hdc );
        return tm.tmHeight <= 0 ? 13 : ((tm.tmHeight + tm.tmExternalLeading - 1) | 1);
    }
    default:
        return get_system_metrics( index );
    }
}

COLORREF get_sys_color( int index )
{
    COLORREF ret = 0;

    if (index >= 0 && index < ARRAY_SIZE( system_colors ))
        get_entry( &system_colors[index], 0, &ret );
    return ret;
}

HBRUSH get_55aa_brush(void)
{
    static const WORD pattern[] = { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };
    static HBRUSH brush_55aa;

    if (!brush_55aa)
    {
        HBITMAP bitmap = NtGdiCreateBitmap( 8, 8, 1, 1, pattern );
        HBRUSH brush = NtGdiCreatePatternBrushInternal( bitmap, FALSE, FALSE );
        NtGdiDeleteObjectApp( bitmap );
        make_gdi_object_system( brush, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&brush_55aa, brush, 0 ))
        {
            make_gdi_object_system( brush, FALSE );
            NtGdiDeleteObjectApp( brush );
        }
    }
    return brush_55aa;
}

HBRUSH get_sys_color_brush( unsigned int index )
{
    if (index == COLOR_55AA_BRUSH) return get_55aa_brush();
    if (index >= ARRAY_SIZE( system_colors )) return 0;

    if (!system_colors[index].brush)
    {
        HBRUSH brush = NtGdiCreateSolidBrush( get_sys_color( index ), NULL );
        make_gdi_object_system( brush, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&system_colors[index].brush, brush, 0 ))
        {
            make_gdi_object_system( brush, FALSE );
            NtGdiDeleteObjectApp( brush );
        }
    }
    return system_colors[index].brush;
}

HPEN get_sys_color_pen( unsigned int index )
{
    if (index >= ARRAY_SIZE( system_colors )) return 0;

    if (!system_colors[index].pen)
    {
        HPEN pen = NtGdiCreatePen( PS_SOLID, 1, get_sys_color( index ), NULL );
        make_gdi_object_system( pen, TRUE );
        if (InterlockedCompareExchangePointer( (void **)&system_colors[index].pen, pen, 0 ))
        {
            make_gdi_object_system( pen, FALSE );
            NtGdiDeleteObjectApp( pen );
        }
    }
    return system_colors[index].pen;
}

/**********************************************************************
 *	     NtUserGetDoubleClickTime    (win32u.@)
 */
UINT WINAPI NtUserGetDoubleClickTime(void)
{
    UINT time = 0;

    get_entry( &entry_DOUBLECLICKTIME, 0, &time );
    if (!time) time = 500;
    return time;
}

/*************************************************************************
 *	     NtUserSetSysColors    (win32u.@)
 */
BOOL WINAPI NtUserSetSysColors( INT count, const INT *colors, const COLORREF *values )
{
    int i;

    if (IS_INTRESOURCE(colors)) return FALSE; /* stupid app passes a color instead of an array */

    for (i = 0; i < count; i++)
        if (colors[i] >= 0 && colors[i] <= ARRAY_SIZE( system_colors ))
            set_entry( &system_colors[colors[i]], values[i], 0, 0 );

    /* Send WM_SYSCOLORCHANGE message to all windows */
    send_message_timeout( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0,
                          SMTO_ABORTIFHUNG, 2000, FALSE );
    /* Repaint affected portions of all visible windows */
    NtUserRedrawWindow( 0, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}


static DPI_AWARENESS dpi_awareness;

/***********************************************************************
 *	     NtUserSetProcessDpiAwarenessContext    (win32u.@)
 */
BOOL WINAPI NtUserSetProcessDpiAwarenessContext( ULONG awareness, ULONG unknown )
{
    switch (awareness)
    {
    case NTUSER_DPI_UNAWARE:
    case NTUSER_DPI_SYSTEM_AWARE:
    case NTUSER_DPI_PER_MONITOR_AWARE:
    case NTUSER_DPI_PER_MONITOR_AWARE_V2:
    case NTUSER_DPI_PER_UNAWARE_GDISCALED:
        break;
    default:
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return !InterlockedCompareExchange( &dpi_awareness, awareness, 0 );
}

/***********************************************************************
 *	     NtUserGetProcessDpiAwarenessContext    (win32u.@)
 */
ULONG WINAPI NtUserGetProcessDpiAwarenessContext( HANDLE process )
{
    if (process && process != GetCurrentProcess())
    {
        WARN( "not supported on other process %p\n", process );
        return NTUSER_DPI_UNAWARE;
    }

    if (!dpi_awareness) return NTUSER_DPI_UNAWARE;
    return dpi_awareness;
}

BOOL message_beep( UINT i )
{
    BOOL active = TRUE;
    NtUserSystemParametersInfo( SPI_GETBEEP, 0, &active, FALSE );
    if (active) user_driver->pBeep();
    return TRUE;
}

static DWORD exiting_thread_id;

/**********************************************************************
 *           is_exiting_thread
 */
BOOL is_exiting_thread( DWORD tid )
{
    return tid == exiting_thread_id;
}

static void thread_detach(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    user_driver->pThreadDetach();

    free( thread_info->key_state );
    thread_info->key_state = 0;
    free( thread_info->rawinput );

    destroy_thread_windows();
    cleanup_imm_thread();
    NtClose( thread_info->server_queue );

    exiting_thread_id = 0;
}

/***********************************************************************
 *	     NtUserCallNoParam    (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallNoParam( ULONG code )
{
    switch(code)
    {
    case NtUserCallNoParam_DestroyCaret:
        return destroy_caret();

    case NtUserCallNoParam_GetDesktopWindow:
        return HandleToUlong( get_desktop_window() );

    case NtUserCallNoParam_GetDialogBaseUnits:
        return get_dialog_base_units();

    case NtUserCallNoParam_GetInputState:
        return get_input_state();

    case NtUserCallNoParam_GetProcessDefaultLayout:
        return process_layout;

    case NtUserCallNoParam_ReleaseCapture:
        return release_capture();

    /* temporary exports */
    case NtUserExitingThread:
        exiting_thread_id = GetCurrentThreadId();
        return 0;

    case NtUserThreadDetach:
        thread_detach();
        return 0;

    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}

/***********************************************************************
 *	     NtUserCallOneParam    (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallOneParam( ULONG_PTR arg, ULONG code )
{
    switch(code)
    {
    case NtUserCallOneParam_BeginDeferWindowPos:
        return HandleToUlong( begin_defer_window_pos( arg ));

    case NtUserCallOneParam_CreateCursorIcon:
        return HandleToUlong( alloc_cursoricon_handle( arg ));

    case NtUserCallOneParam_CreateMenu:
        return HandleToUlong( create_menu( arg ) );

    case NtUserCallOneParam_EnableDC:
        return set_dce_flags( UlongToHandle(arg), DCHF_ENABLEDC );

    case NtUserCallOneParam_EnableThunkLock:
        enable_thunk_lock = arg;
        return 0;

    case NtUserCallOneParam_EnumClipboardFormats:
        return enum_clipboard_formats( arg );

    case NtUserCallOneParam_GetClipCursor:
        return get_clip_cursor( (RECT *)arg );

    case NtUserCallOneParam_GetCursorPos:
        return get_cursor_pos( (POINT *)arg );

    case NtUserCallOneParam_GetIconParam:
        return get_icon_param( UlongToHandle(arg) );

    case NtUserCallOneParam_GetMenuItemCount:
        return get_menu_item_count( UlongToHandle(arg) );

    case NtUserCallOneParam_GetSysColor:
        return get_sys_color( arg );

    case NtUserCallOneParam_IsWindowRectFullScreen:
        return is_window_rect_full_screen( (const RECT *)arg );

    case NtUserCallOneParam_RealizePalette:
        return realize_palette( UlongToHandle(arg) );

    case NtUserCallOneParam_GetPrimaryMonitorRect:
        *(RECT *)arg = get_primary_monitor_rect( 0 );
        return 1;

    case NtUserCallOneParam_GetSysColorBrush:
        return HandleToUlong( get_sys_color_brush(arg) );

    case NtUserCallOneParam_GetSysColorPen:
        return HandleToUlong( get_sys_color_pen(arg) );

    case NtUserCallOneParam_GetSystemMetrics:
        return get_system_metrics( arg );

    case NtUserCallOneParam_GetVirtualScreenRect:
        *(RECT *)arg = get_virtual_screen_rect( 0 );
        return 1;

    case NtUserCallOneParam_MessageBeep:
        return message_beep( arg );

    case NtUserCallOneParam_ReplyMessage:
        return reply_message_result( arg );

    case NtUserCallOneParam_SetCaretBlinkTime:
        return set_caret_blink_time( arg );

    case NtUserCallOneParam_SetProcessDefaultLayout:
        process_layout = arg;
        return TRUE;

    /* temporary exports */
    case NtUserGetDeskPattern:
        return get_entry( &entry_DESKPATTERN, 256, (WCHAR *)arg );

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
    case NtUserCallTwoParam_GetDialogProc:
        return (ULONG_PTR)get_dialog_proc( (DLGPROC)arg1, arg2 );

    case NtUserCallTwoParam_GetMenuInfo:
        return get_menu_info( UlongToHandle(arg1), (MENUINFO *)arg2 );

    case NtUserCallTwoParam_GetMonitorInfo:
        return get_monitor_info( UlongToHandle(arg1), (MONITORINFO *)arg2 );

    case NtUserCallTwoParam_GetSystemMetricsForDpi:
        return get_system_metrics_for_dpi( arg1, arg2 );

    case NtUserCallTwoParam_MonitorFromRect:
        return HandleToUlong( monitor_from_rect( (const RECT *)arg1, arg2, get_thread_dpi() ));

    case NtUserCallTwoParam_SetCaretPos:
        return set_caret_pos( arg1, arg2 );

    case NtUserCallTwoParam_SetIconParam:
        return set_icon_param( UlongToHandle(arg1), arg2 );

    case NtUserCallTwoParam_UnhookWindowsHook:
        return unhook_windows_hook( arg1, (HOOKPROC)arg2 );

    /* temporary exports */
    case NtUserAllocWinProc:
        return (UINT_PTR)alloc_winproc( (WNDPROC)arg1, arg2 );

    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}
