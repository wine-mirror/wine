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
#include "winreg.h"
#include "cfgmgr32.h"
#include "d3dkmdt.h"
#include "wine/wingdi16.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(system);

#define WINE_ENUM_PHYSICAL_SETTINGS  ((DWORD) -3)

static LONG dpi_context; /* process DPI awareness context */

static HKEY video_key, enum_key, control_key, config_key, volatile_base_key;

static const char devicemap_video_keyA[] = "\\Registry\\Machine\\HARDWARE\\DEVICEMAP\\VIDEO";
static const char enum_keyA[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Enum";
static const char control_keyA[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Control";
static const char config_keyA[] = "\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current";

static const char devpropkey_gpu_vulkan_uuidA[] = "Properties\\{233A9EF3-AFC4-4ABD-B564-C32F21F1535C}\\0002";
static const char devpropkey_gpu_luidA[] = "Properties\\{60B193CB-5276-4D0F-96FC-F173ABAD3EC6}\\0002";
static const char devpkey_device_matching_device_id[] = "Properties\\{A8B865DD-2E3D-4094-AD97-E593A70C75D6}\\0008";
static const char devpkey_device_bus_number[] = "Properties\\{A45C254E-DF1C-4EFD-8020-67D146A850E0}\\0017";
static const char devpkey_device_removal_policy[] = "Properties\\{A45C254E-DF1C-4EFD-8020-67D146A850E0}\\0021";
static const char devpropkey_device_ispresentA[] = "Properties\\{540B947E-8B40-45BC-A8A2-6A0B894CBDA2}\\0005";
static const char devpropkey_monitor_gpu_luidA[] = "Properties\\{CA085853-16CE-48AA-B114-DE9C72334223}\\0001";
static const char devpropkey_monitor_output_idA[] = "Properties\\{CA085853-16CE-48AA-B114-DE9C72334223}\\0002";
static const char wine_devpropkey_monitor_rcworkA[] = "Properties\\{233a9ef3-afc4-4abd-b564-c32f21f1535b}\\0004";

static const WCHAR linkedW[] = {'L','i','n','k','e','d',0};
static const WCHAR symbolic_link_valueW[] =
    {'S','y','m','b','o','l','i','c','L','i','n','k','V','a','l','u','e',0};
static const WCHAR state_flagsW[] = {'S','t','a','t','e','F','l','a','g','s',0};
static const WCHAR hardware_idW[] = {'H','a','r','d','w','a','r','e','I','D',0};
static const WCHAR device_descW[] = {'D','e','v','i','c','e','D','e','s','c',0};
static const WCHAR driver_descW[] = {'D','r','i','v','e','r','D','e','s','c',0};
static const WCHAR yesW[] = {'Y','e','s',0};
static const WCHAR noW[] = {'N','o',0};
static const WCHAR modesW[] = {'M','o','d','e','s',0};
static const WCHAR mode_countW[] = {'M','o','d','e','C','o','u','n','t',0};
static const WCHAR dpiW[] = {'D','p','i',0};
static const WCHAR depthW[] = {'D','e','p','t','h',0};
static const WCHAR physicalW[] = {'P','h','y','s','i','c','a','l',0};

static const char  guid_devclass_displayA[] = "{4D36E968-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_displayW[] =
    {'{','4','D','3','6','E','9','6','8','-','E','3','2','5','-','1','1','C','E','-',
     'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}',0};
static const char  guid_devclass_monitorA[] = "{4D36E96E-E325-11CE-BFC1-08002BE10318}";
static const WCHAR guid_devclass_monitorW[] =
    {'{','4','D','3','6','E','9','6','E','-','E','3','2','5','-','1','1','C','E','-',
     'B','F','C','1','-','0','8','0','0','2','B','E','1','0','3','1','8','}',0};

static const char guid_devinterface_display_adapterA[] = "{5B45201D-F2F2-4F3B-85BB-30FF1F953599}";
static const char guid_display_device_arrivalA[] = "{1CA05180-A699-450A-9A0C-DE4FBE3DDD89}";
static const char guid_devinterface_monitorA[] = "{E6F07B5F-EE97-4A90-B076-33F57BF4EAA7}";

static const UINT32 qdc_retrieve_flags_mask = QDC_ALL_PATHS | QDC_ONLY_ACTIVE_PATHS | QDC_DATABASE_CURRENT;

#define NEXT_DEVMODEW(mode) ((DEVMODEW *)((char *)((mode) + 1) + (mode)->dmDriverExtra))

struct gpu
{
    LONG refcount;
    struct list entry;
    char path[MAX_PATH];
    WCHAR name[128];
    char guid[39];
    LUID luid;
    UINT index;
    GUID vulkan_uuid;
    UINT source_count;
};

struct source
{
    LONG refcount;
    struct list entry;
    char path[MAX_PATH];
    unsigned int id;
    struct gpu *gpu;
    HKEY key;
    UINT dpi;
    UINT depth; /* emulated depth */
    UINT state_flags;
    UINT monitor_count;
    UINT mode_count;
    DEVMODEW current;
    DEVMODEW physical;
    DEVMODEW *modes;
};

#define MONITOR_INFO_HAS_MONITOR_ID 0x00000001
#define MONITOR_INFO_HAS_MONITOR_NAME 0x00000002
#define MONITOR_INFO_HAS_PREFERRED_MODE 0x00000004
struct edid_monitor_info
{
    unsigned int flags;
    /* MONITOR_INFO_HAS_MONITOR_ID */
    unsigned short manufacturer, product_code;
    char monitor_id_string[8];
    /* MONITOR_INFO_HAS_MONITOR_NAME */
    WCHAR monitor_name[14];
    /* MONITOR_INFO_HAS_PREFERRED_MODE */
    unsigned int preferred_width, preferred_height;
};

struct monitor
{
    LONG refcount;
    struct list entry;
    char path[MAX_PATH];
    struct source *source;
    HANDLE handle;
    unsigned int id;
    unsigned int output_id;
    RECT rc_work;
    BOOL is_clone;
    struct edid_monitor_info edid_info;
};

static struct list gpus = LIST_INIT(gpus);
static struct list sources = LIST_INIT(sources);
static struct list monitors = LIST_INIT(monitors);
static INT64 last_query_display_time;
static UINT64 monitor_update_serial;
static pthread_mutex_t display_lock = PTHREAD_MUTEX_INITIALIZER;

static BOOL emulate_modeset;
BOOL decorated_mode = TRUE;
UINT64 thunk_lock_callback = 0;

#define VIRTUAL_HMONITOR ((HMONITOR)(UINT_PTR)(0x10000 + 1))
static struct monitor virtual_monitor =
{
    .handle = VIRTUAL_HMONITOR,
    .rc_work.right = 1024,
    .rc_work.bottom = 768,
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
    WCHAR                *path;
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
    union sysparam_all_entry *parent;
    UINT                  offset;
    UINT                  mask;
};

union sysparam_all_entry
{
    struct sysparam_entry        hdr;
    struct sysparam_uint_entry   uint;
    struct sysparam_bool_entry   boolean;
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
              (int)NtCurrentTeb()->Peb->SessionId );
    name.MaximumLength = asciiz_to_unicode( bufferW, buffer );
    name.Length = name.MaximumLength - sizeof(WCHAR);

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

static struct gpu *gpu_acquire( struct gpu *gpu )
{
    UINT ref = InterlockedIncrement( &gpu->refcount );
    TRACE( "gpu %p increasing refcount to %u\n", gpu, ref );
    return gpu;
}

static void gpu_release( struct gpu *gpu )
{
    UINT ref = InterlockedDecrement( &gpu->refcount );
    TRACE( "gpu %p decreasing refcount to %u\n", gpu, ref );
    if (!ref) free( gpu );
}

static struct source *source_acquire( struct source *source )
{
    UINT ref = InterlockedIncrement( &source->refcount );
    TRACE( "source %p increasing refcount to %u\n", source, ref );
    return source;
}

static void source_release( struct source *source )
{
    UINT ref = InterlockedDecrement( &source->refcount );
    TRACE( "source %p decreasing refcount to %u\n", source, ref );
    if (!ref)
    {
        if (source->key) NtClose( source->key );
        gpu_release( source->gpu );
        free( source->modes );
        free( source );
    }
}

static void monitor_release( struct monitor *monitor )
{
    UINT ref = InterlockedDecrement( &monitor->refcount );
    TRACE( "monitor %p decreasing refcount to %u\n", monitor, ref );
    if (!ref)
    {
        if (monitor->source) source_release( monitor->source );
        free( monitor );
    }
}

C_ASSERT(sizeof(DEVMODEW) - offsetof(DEVMODEW, dmFields) == 0x94);

static void get_monitor_info_from_edid( struct edid_monitor_info *info, const unsigned char *edid, unsigned int edid_len )
{
    unsigned int i, j;
    unsigned short w;
    unsigned char d;
    const char *s;

    info->flags = 0;
    if (!edid || edid_len < 128) return;

    w = (edid[8] << 8) | edid[9]; /* Manufacturer ID, big endian. */
    for (i = 0; i < 3; ++i)
    {
        d = w & 0x1f;
        if (!d || d - 1 > 'Z' - 'A') return;
        info->monitor_id_string[2 - i] = 'A' + d - 1;
        w >>= 5;
    }
    if (w) return;
    w = edid[10] | (edid[11] << 8); /* Product code, little endian. */
    info->manufacturer = *(unsigned short *)(edid + 8);
    info->product_code = w;
    snprintf( info->monitor_id_string + 3, sizeof(info->monitor_id_string) - 3, "%04X", w );
    info->flags = MONITOR_INFO_HAS_MONITOR_ID;
    TRACE( "Monitor id %s.\n", info->monitor_id_string );

    for (i = 0; i < 4; ++i)
    {
        if (edid[54 + i * 18] || edid[54 + i * 18 + 1])
        {
            /* Detailed timing descriptor. */
            if (info->flags & MONITOR_INFO_HAS_PREFERRED_MODE) continue;
            info->preferred_width = edid[54 + i * 18 + 2] | ((UINT32)(edid[54 + i * 18 + 4] & 0xf0) << 4);
            info->preferred_height = edid[54 + i * 18 + 5] | ((UINT32)(edid[54 + i * 18 + 7] & 0xf0) << 4);
            if (info->preferred_width && info->preferred_height)
                info->flags |= MONITOR_INFO_HAS_PREFERRED_MODE;
            continue;
        }
        if (edid[54 + i * 18 + 3] != 0xfc) continue;
        /* "Display name" ASCII descriptor. */
        s = (const char *)&edid[54 + i * 18 + 5];
        for (j = 0; s[j] && j < 13; ++j)
            info->monitor_name[j] = s[j];
        while (j && isspace(s[j - 1])) --j;
        info->monitor_name[j] = 0;
        info->flags |= MONITOR_INFO_HAS_MONITOR_NAME;
        break;
    }
}

static const char *debugstr_devmodew( const DEVMODEW *devmode )
{
    char position[32] = {0};
    if (devmode->dmFields & DM_POSITION) snprintf( position, sizeof(position), " at %s", wine_dbgstr_point( (POINT *)&devmode->dmPosition ) );
    return wine_dbg_sprintf( "%ux%u %ubits %uHz rotated %u degrees %sstretched %sinterlaced%s",
                             (UINT)devmode->dmPelsWidth, (UINT)devmode->dmPelsHeight, (UINT)devmode->dmBitsPerPel,
                             (UINT)devmode->dmDisplayFrequency, (UINT)devmode->dmDisplayOrientation * 90,
                             devmode->dmDisplayFixedOutput == DMDFO_STRETCH ? "" : "un",
                             devmode->dmDisplayFlags & DM_INTERLACED ? "" : "non-",
                             position );
}

static UINT devmode_get( const DEVMODEW *mode, UINT field )
{
    switch (field)
    {
    case DM_DISPLAYORIENTATION: return mode->dmFields & DM_DISPLAYORIENTATION ? mode->dmDisplayOrientation : 0;
    case DM_BITSPERPEL: return mode->dmFields & DM_BITSPERPEL ? mode->dmBitsPerPel : 0;
    case DM_PELSWIDTH: return mode->dmFields & DM_PELSWIDTH ? mode->dmPelsWidth : 0;
    case DM_PELSHEIGHT: return mode->dmFields & DM_PELSHEIGHT ? mode->dmPelsHeight : 0;
    case DM_DISPLAYFLAGS: return mode->dmFields & DM_DISPLAYFLAGS ? mode->dmDisplayFlags : 0;
    case DM_DISPLAYFREQUENCY: return mode->dmFields & DM_DISPLAYFREQUENCY ? mode->dmDisplayFrequency : 0;
    case DM_DISPLAYFIXEDOUTPUT: return mode->dmFields & DM_DISPLAYFIXEDOUTPUT ? mode->dmDisplayFixedOutput : 0;
    }
    return 0;
}

static BOOL write_source_mode( HKEY hkey, UINT index, const DEVMODEW *mode )
{
    WCHAR bufferW[MAX_PATH] = {0};

    assert( index == ENUM_CURRENT_SETTINGS || index == ENUM_REGISTRY_SETTINGS || index == WINE_ENUM_PHYSICAL_SETTINGS );

    if (index == ENUM_CURRENT_SETTINGS) asciiz_to_unicode( bufferW, "Current" );
    else if (index == ENUM_REGISTRY_SETTINGS) asciiz_to_unicode( bufferW, "Registry" );
    else if (index == WINE_ENUM_PHYSICAL_SETTINGS) asciiz_to_unicode( bufferW, "Physical" );
    else return FALSE;

    return set_reg_value( hkey, bufferW, REG_BINARY, &mode->dmFields, sizeof(*mode) - offsetof(DEVMODEW, dmFields) );
}

static BOOL read_source_mode( HKEY hkey, UINT index, DEVMODEW *mode )
{
    char value_buf[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(*mode)])];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)value_buf;
    const char *key;

    assert( index == ENUM_CURRENT_SETTINGS || index == ENUM_REGISTRY_SETTINGS || index == WINE_ENUM_PHYSICAL_SETTINGS );

    if (index == ENUM_CURRENT_SETTINGS) key = "Current";
    else if (index == ENUM_REGISTRY_SETTINGS) key = "Registry";
    else if (index == WINE_ENUM_PHYSICAL_SETTINGS) key = "Physical";
    else return FALSE;

    if (!query_reg_ascii_value( hkey, key, value, sizeof(value_buf) )) return FALSE;
    memcpy( &mode->dmFields, value->Data, sizeof(*mode) - offsetof(DEVMODEW, dmFields) );
    return TRUE;
}

static BOOL source_get_registry_settings( const struct source *source, DEVMODEW *mode )
{
    BOOL ret = FALSE;
    HANDLE mutex;
    HKEY hkey;

    mutex = get_display_device_init_mutex();

    if (!(hkey = reg_open_ascii_key( config_key, source->path ))) ret = FALSE;
    else
    {
        ret = read_source_mode( hkey, ENUM_REGISTRY_SETTINGS, mode );
        NtClose( hkey );
    }

    release_display_device_init_mutex( mutex );
    return ret;
}

static BOOL source_set_registry_settings( const struct source *source, const DEVMODEW *mode )
{
    HANDLE mutex;
    HKEY hkey;
    BOOL ret;

    mutex = get_display_device_init_mutex();

    if (!(hkey = reg_open_ascii_key( config_key, source->path ))) ret = FALSE;
    else
    {
        ret = write_source_mode( hkey, ENUM_REGISTRY_SETTINGS, mode );
        NtClose( hkey );
    }

    release_display_device_init_mutex( mutex );
    return ret;
}

static BOOL source_get_current_settings( const struct source *source, DEVMODEW *mode )
{
    memcpy( &mode->dmFields, &source->current.dmFields, sizeof(*mode) - offsetof(DEVMODEW, dmFields) );
    if (source->depth) mode->dmBitsPerPel = source->depth;
    return TRUE;
}

static BOOL source_set_current_settings( const struct source *source, const DEVMODEW *mode )
{
    HANDLE mutex;
    HKEY hkey;
    BOOL ret;

    mutex = get_display_device_init_mutex();

    if (!(hkey = reg_open_ascii_key( config_key, source->path ))) ret = FALSE;
    else
    {
        ret = write_source_mode( hkey, ENUM_CURRENT_SETTINGS, mode );
        if (ret) set_reg_value( hkey, depthW, REG_DWORD, &mode->dmBitsPerPel, sizeof(mode->dmBitsPerPel) );
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

static unsigned int query_reg_subkey_value( HKEY hkey, const char *name, KEY_VALUE_PARTIAL_INFORMATION *value, unsigned int size )
{
    HKEY subkey;

    if (!(subkey = reg_open_ascii_key( hkey, name ))) return 0;
    size = query_reg_value( subkey, NULL, value, size );
    NtClose( subkey );
    return size;
}

static BOOL read_source_from_registry( unsigned int index, struct source *source, char *gpu_path )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    DWORD i, size;
    HKEY hkey;

    if (!enum_key && !(enum_key = reg_open_ascii_key( NULL, enum_keyA )))
        return FALSE;

    /* Find source */
    snprintf( buffer, sizeof(buffer), "\\Device\\Video%d", index );
    size = query_reg_ascii_value( video_key, buffer, value, sizeof(buffer) );
    if (!size || value->Type != REG_SZ) return FALSE;

    /* DeviceKey */
    size = sizeof("\\Registry\\Machine");
    if (value->DataLength / sizeof(WCHAR) <= size) return FALSE;
    for (i = 0; i < value->DataLength / sizeof(WCHAR) - size; i++) source->path[i] = value_str[size + i];
    if (!(hkey = reg_open_ascii_key( config_key, source->path ))) return FALSE;

    /* StateFlags */
    if (query_reg_ascii_value( hkey, "StateFlags", value, sizeof(buffer) ) && value->Type == REG_DWORD)
        source->state_flags = *(const DWORD *)value->Data;

    /* Dpi */
    if (query_reg_ascii_value( hkey, "Dpi", value, sizeof(buffer) ) && value->Type == REG_DWORD)
        source->dpi = *(DWORD *)value->Data;

    /* Depth */
    if (query_reg_ascii_value( hkey, "Depth", value, sizeof(buffer) ) && value->Type == REG_DWORD)
        source->depth = *(const DWORD *)value->Data;

    /* ModeCount */
    if (query_reg_ascii_value( hkey, "ModeCount", value, sizeof(buffer) ) && value->Type == REG_DWORD)
        source->mode_count = *(const DWORD *)value->Data;

    /* Modes */
    size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data[(source->mode_count + 1) * sizeof(*source->modes)] );
    if (!(value = malloc( size )) || !query_reg_ascii_value( hkey, "Modes", value, size )) free( value );
    else
    {
        source->modes = (DEVMODEW *)value;
        source->mode_count = value->DataLength / sizeof(*source->modes);
        memmove( source->modes, value->Data, value->DataLength );
        memset( source->modes + source->mode_count, 0, sizeof(*source->modes) ); /* extra empty mode for easier iteration */
        qsort( source->modes, source->mode_count, sizeof(*source->modes), mode_compare );
    }
    value = (void *)buffer;

    /* Cache current and physical display modes */
    if (read_source_mode( hkey, ENUM_CURRENT_SETTINGS, &source->current ))
        source->current.dmSize = sizeof(source->current);
    source->physical = source->current;
    if (read_source_mode( hkey, WINE_ENUM_PHYSICAL_SETTINGS, &source->physical ))
        source->physical.dmSize = sizeof(source->physical);

    /* DeviceID */
    size = query_reg_ascii_value( hkey, "GPUID", value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ || !source->mode_count || !source->modes) return FALSE;

    for (i = 0; i < value->DataLength / sizeof(WCHAR); i++) gpu_path[i] = value_str[i];
    return TRUE;
}

static BOOL read_monitor_from_registry( struct monitor *monitor )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    HKEY hkey, subkey;
    DWORD size;

    if (!(hkey = reg_open_ascii_key( enum_key, monitor->path ))) return FALSE;

    /* Output ID */
    size = query_reg_subkey_value( hkey, devpropkey_monitor_output_idA,
                                   value, sizeof(buffer) );
    if (size != sizeof(monitor->output_id))
    {
        NtClose( hkey );
        return FALSE;
    }
    monitor->output_id = *(const unsigned int *)value->Data;

    /* rc_work, WINE_DEVPROPKEY_MONITOR_RCWORK */
    size = query_reg_subkey_value( hkey, wine_devpropkey_monitor_rcworkA,
                                   value, sizeof(buffer) );
    if (size != sizeof(monitor->rc_work))
    {
        NtClose( hkey );
        return FALSE;
    }
    monitor->rc_work = *(const RECT *)value->Data;

    /* EDID */
    if ((subkey = reg_open_ascii_key( hkey, "Device Parameters" )))
    {
        if (query_reg_ascii_value( subkey, "EDID", value, sizeof(buffer) ))
            get_monitor_info_from_edid( &monitor->edid_info, value->Data, value->DataLength );
        NtClose( subkey );
    }

    NtClose( hkey );
    return TRUE;
}

static BOOL read_source_monitor_path( HKEY hkey, UINT index, char *path )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    DWORD size;
    UINT i;

    snprintf( buffer, sizeof(buffer), "MonitorID%u", index );
    size = query_reg_ascii_value( hkey, buffer, value, sizeof(buffer) );
    if (!size || value->Type != REG_SZ) return FALSE;

    for (i = 0; i < value->DataLength / sizeof(WCHAR); i++) path[i] = value_str[i];
    return TRUE;
}

static void reg_empty_key( HKEY root, const char *key_name )
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (KEY_NODE_INFORMATION *)buffer;
    KEY_VALUE_FULL_INFORMATION *value = (KEY_VALUE_FULL_INFORMATION *)buffer;
    DWORD size;
    HKEY hkey = key_name ? reg_open_ascii_key( root, key_name ) : root;

    while (!NtEnumerateKey( hkey, 0, KeyNodeInformation, key, sizeof(buffer), &size ))
        reg_delete_tree( hkey, key->Name, key->NameLength );

    while (!NtEnumerateValueKey( hkey, 0, KeyValueFullInformation, value, sizeof(buffer), &size ))
    {
        UNICODE_STRING name = { value->NameLength, value->NameLength, value->Name };
        NtDeleteValueKey( hkey, &name );
    }

    if (hkey != root) NtClose( hkey );
}

static void clear_display_devices(void)
{
    struct source *source;
    struct monitor *monitor;
    struct gpu *gpu;

    if (list_head( &monitors ) == &virtual_monitor.entry)
    {
        list_init( &monitors );
        return;
    }

    while (!list_empty( &monitors ))
    {
        monitor = LIST_ENTRY( list_head( &monitors ), struct monitor, entry );
        list_remove( &monitor->entry );
        monitor_release( monitor );
    }

    while (!list_empty( &sources ))
    {
        source = LIST_ENTRY( list_head( &sources ), struct source, entry );
        list_remove( &source->entry );
        source_release( source );
    }

    while (!list_empty( &gpus ))
    {
        gpu = LIST_ENTRY( list_head( &gpus ), struct gpu, entry );
        list_remove( &gpu->entry );
        gpu_release( gpu );
    }
}

static void prepare_devices(void)
{
    char buffer[4096];
    KEY_NODE_INFORMATION *key = (void *)buffer;
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    unsigned i = 0;
    DWORD size;
    HKEY hkey, subkey, device_key, prop_key;

    clear_display_devices();

    if (!enum_key) enum_key = reg_create_ascii_key( NULL, enum_keyA, 0, NULL );
    if (!control_key) control_key = reg_create_ascii_key( NULL, control_keyA, 0, NULL );
    if (!video_key) video_key = reg_create_ascii_key( NULL, devicemap_video_keyA, REG_OPTION_VOLATILE, NULL );

    /* delete monitors */
    reg_empty_key( enum_key, "DISPLAY" );
    snprintf( buffer, sizeof(buffer), "Class\\%s", guid_devclass_monitorA );
    hkey = reg_create_ascii_key( control_key, buffer, 0, NULL );
    reg_empty_key( hkey, NULL );
    set_reg_ascii_value( hkey, "", "Monitors" );
    set_reg_ascii_value( hkey, "Class", "Monitor" );
    NtClose( hkey );

    /* delete sources */
    reg_empty_key( video_key, NULL );

    /* clean GPUs */
    snprintf( buffer, sizeof(buffer), "Class\\%s", guid_devclass_displayA );
    hkey = reg_create_ascii_key( control_key, buffer, 0, NULL );
    reg_empty_key( hkey, NULL );
    set_reg_ascii_value( hkey, "", "Display adapters" );
    set_reg_ascii_value( hkey, "Class", "Display" );
    NtClose( hkey );

    hkey = reg_open_ascii_key( enum_key, "PCI" );

    /* To preserve GPU GUIDs, mark them as not present and delete them in cleanup_devices if needed. */
    while (!NtEnumerateKey( hkey, i++, KeyNodeInformation, key, sizeof(buffer), &size ))
    {
        unsigned int j = 0;

        if (!(subkey = reg_open_key( hkey, key->Name, key->NameLength ))) continue;

        while (!NtEnumerateKey( subkey, j++, KeyNodeInformation, key, sizeof(buffer), &size ))
        {
            if (!(device_key = reg_open_key( subkey, key->Name, key->NameLength ))) continue;
            size = query_reg_ascii_value( device_key, "ClassGUID", value, sizeof(buffer) );
            if (size != sizeof(guid_devclass_displayW) || wcscmp( value_str, guid_devclass_displayW ))
            {
                NtClose( device_key );
                continue;
            }

            if ((prop_key = reg_create_ascii_key( device_key, devpropkey_device_ispresentA, 0, NULL )))
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

    hkey = reg_open_ascii_key( enum_key, "PCI" );

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

            size = query_reg_ascii_value( device_key, "ClassGUID", value, sizeof(buffer) );
            if (size != sizeof(guid_devclass_displayW) || wcscmp( value_str, guid_devclass_displayW ))
            {
                NtClose( device_key );
                continue;
            }

            if ((prop_key = reg_open_ascii_key( device_key, devpropkey_device_ispresentA )))
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
    char buffer[33];

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

    snprintf( buffer, sizeof(buffer), "%u-%u-%u", month, day, year );
    return asciiz_to_unicode( bufferW, buffer );
}

struct device_manager_ctx
{
    UINT gpu_count;
    UINT source_count;
    UINT monitor_count;
    HANDLE mutex;
    struct list vulkan_gpus;
    BOOL has_primary;
    /* for the virtual desktop settings */
    BOOL is_primary;
    DEVMODEW primary;
};

static void link_device( const char *instance, const char *class )
{
    char buffer[MAX_PATH], *ptr;
    HKEY hkey, subkey;
    unsigned int pos;

    pos = snprintf( buffer, ARRAY_SIZE(buffer), "DeviceClasses\\%s\\", class );
    snprintf( buffer + pos, ARRAY_SIZE(buffer) - pos, "##?#%s#%s", instance, class );
    for (ptr = buffer + pos; *ptr; ptr++) if (*ptr == '\\') *ptr = '#';

    hkey = reg_create_ascii_key( control_key, buffer, 0, NULL );
    set_reg_ascii_value( hkey, "DeviceInstance", instance );

    subkey = reg_create_ascii_key( hkey, "#", REG_OPTION_VOLATILE, NULL );
    NtClose( hkey );
    hkey = subkey;

    snprintf( buffer, ARRAY_SIZE(buffer), "\\\\?\\%s#%s", instance, class );
    for (ptr = buffer + 4; *ptr; ptr++) if (*ptr == '\\') *ptr = '#';
    set_reg_ascii_value( hkey, "SymbolicLink", buffer );

    if ((subkey = reg_create_ascii_key( hkey, "Control", REG_OPTION_VOLATILE, NULL )))
    {
        const DWORD linked = 1;
        set_reg_value( subkey, linkedW, REG_DWORD, &linked, sizeof(linked) );
        NtClose( subkey );
    }
}

static BOOL read_gpu_from_registry( struct gpu *gpu )
{
    char buffer[1024];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = (WCHAR *)value->Data;
    HKEY hkey, subkey;
    unsigned int i;

    if (!(hkey = reg_open_ascii_key( enum_key, gpu->path ))) return FALSE;

    if (query_reg_ascii_value( hkey, "Driver", value, sizeof(buffer) ) && value->Type == REG_SZ)
        gpu->index = wcstoul( wcsrchr( value_str, '\\' ) + 1, NULL, 16 );

    if (query_reg_ascii_value( hkey, "DeviceDesc", value, sizeof(buffer) ) && value->Type == REG_SZ)
        memcpy( gpu->name, value->Data, value->DataLength );

    if ((subkey = reg_open_ascii_key( hkey, devpropkey_gpu_luidA )))
    {
        if (query_reg_value( subkey, NULL, value, sizeof(buffer) ) == sizeof(LUID))
            gpu->luid = *(const LUID *)value->Data;
        NtClose( subkey );
    }

    if ((subkey = reg_create_ascii_key( hkey, "Device Parameters", 0, NULL )))
    {
        if (query_reg_ascii_value( subkey, "VideoID", value, sizeof(buffer) ) == sizeof(gpu->guid) * sizeof(WCHAR))
        {
            WCHAR *guidW = (WCHAR *)value->Data;
            for (i = 0; i < sizeof(gpu->guid); i++) gpu->guid[i] = guidW[i];
            TRACE( "got guid %s\n", debugstr_a(gpu->guid) );
        }
        NtClose( subkey );
    }

    if ((subkey = reg_open_ascii_key( hkey, devpropkey_gpu_vulkan_uuidA )))
    {
        if (query_reg_value( subkey, NULL, value, sizeof(buffer) ) == sizeof(GUID))
            gpu->vulkan_uuid = *(const GUID *)value->Data;
        NtClose( subkey );
    }

    NtClose( hkey );

    return TRUE;
}

static BOOL write_gpu_to_registry( const struct gpu *gpu, const struct pci_id *pci,
                                   ULONGLONG memory_size )
{
    const WCHAR *desc;
    char buffer[4096], *tmp;
    WCHAR bufferW[512];
    unsigned int size;
    HKEY subkey;
    LARGE_INTEGER ft;
    ULONG value;
    HKEY hkey;

    static const BOOL present = TRUE;
    static const WCHAR wine_adapterW[] = {'W','i','n','e',' ','A','d','a','p','t','e','r',0};
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
    static const WCHAR qw_memory_sizeW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'q','w','M','e','m','o','r','y','S','i','z','e',0};
    static const WCHAR memory_sizeW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'M','e','m','o','r','y','S','i','z','e',0};
    static const WCHAR dac_typeW[] =
        {'H','a','r','d','w','a','r','e','I','n','f','o','r','m','a','t','i','o','n','.',
         'D','a','c','T','y','p','e',0};
    static const WCHAR ramdacW[] =
        {'I','n','t','e','r','g','r','a','t','e','d',' ','R','A','M','D','A','C',0};
    static const WCHAR driver_dateW[] = {'D','r','i','v','e','r','D','a','t','e',0};


    if (!(hkey = reg_create_ascii_key( enum_key, gpu->path, 0, NULL ))) return FALSE;

    set_reg_ascii_value( hkey, "Class", "Display" );
    set_reg_ascii_value( hkey, "ClassGUID", guid_devclass_displayA );
    snprintf( buffer, sizeof(buffer), "%s\\%04X", guid_devclass_displayA, gpu->index );
    set_reg_ascii_value( hkey, "Driver", buffer );

    strcpy( buffer, gpu->path );
    if ((tmp = strrchr( buffer, '\\' ))) *tmp = 0;
    size = asciiz_to_unicode( bufferW, buffer );
    bufferW[size / sizeof(WCHAR)] = 0; /* for REG_MULTI_SZ */
    set_reg_value( hkey, hardware_idW, REG_MULTI_SZ, bufferW, size + sizeof(WCHAR) );

    if ((subkey = reg_create_ascii_key( hkey, devpkey_device_matching_device_id, 0, NULL )))
    {
        if (pci->vendor && pci->device)
            set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_STRING, bufferW, size );
        else
            set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_STRING, bufferW,
                           asciiz_to_unicode( bufferW, "ROOT\\BasicRender" ));
        NtClose( subkey );
    }

    if (pci->vendor && pci->device)
    {
        if ((subkey = reg_create_ascii_key( hkey, devpkey_device_bus_number, 0, NULL )))
        {
            set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT32,
                           &gpu->index, sizeof(gpu->index) );
            NtClose( subkey );
        }
    }

    if ((subkey = reg_create_ascii_key( hkey, devpkey_device_removal_policy, 0, NULL )))
    {
        unsigned int removal_policy = CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL;

        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT32,
                       &removal_policy, sizeof(removal_policy) );
        NtClose( subkey );
    }

    desc = gpu->name;
    if (!desc[0]) desc = wine_adapterW;
    set_reg_value( hkey, device_descW, REG_SZ, desc, (lstrlenW( desc ) + 1) * sizeof(WCHAR) );

    if ((subkey = reg_create_ascii_key( hkey, "Device Parameters", 0, NULL )))
    {
        set_reg_ascii_value( subkey, "VideoID", gpu->guid );
        NtClose( subkey );
    }

    if ((subkey = reg_create_ascii_key( hkey, devpropkey_gpu_vulkan_uuidA, 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_GUID,
                       &gpu->vulkan_uuid, sizeof(gpu->vulkan_uuid) );
        NtClose( subkey );
    }

    if ((subkey = reg_create_ascii_key( hkey, devpropkey_device_ispresentA, 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_BOOLEAN,
                       &present, sizeof(present) );
        NtClose( subkey );
    }

    if ((subkey = reg_create_ascii_key( hkey, devpropkey_gpu_luidA, 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT64,
                       &gpu->luid, sizeof(gpu->luid) );
        NtClose( subkey );
    }

    NtClose( hkey );


    snprintf( buffer, sizeof(buffer), "Class\\%s\\%04X", guid_devclass_displayA, gpu->index );
    if (!(hkey = reg_create_ascii_key( control_key, buffer, 0, NULL ))) return FALSE;

    NtQuerySystemTime( &ft );
    set_reg_value( hkey, driver_dateW, REG_SZ, bufferW, format_date( bufferW, ft.QuadPart ));

    set_reg_value( hkey, driver_date_dataW, REG_BINARY, &ft, sizeof(ft) );

    size = (lstrlenW( desc ) + 1) * sizeof(WCHAR);
    set_reg_value( hkey, driver_descW, REG_SZ, desc, size );
    set_reg_value( hkey, adapter_stringW, REG_SZ, desc, size );
    set_reg_value( hkey, bios_stringW, REG_SZ, desc, size );
    set_reg_value( hkey, chip_typeW, REG_SZ, desc, size );
    set_reg_value( hkey, dac_typeW, REG_SZ, ramdacW, sizeof(ramdacW) );

    /* If we failed to retrieve the gpu memory size set a default of 1Gb */
    if (!memory_size) memory_size = 1073741824;

    set_reg_value( hkey, qw_memory_sizeW, REG_QWORD, &memory_size, sizeof(memory_size) );
    value = (ULONG)min( memory_size, (ULONGLONG)ULONG_MAX );
    set_reg_value( hkey, memory_sizeW, REG_DWORD, &value, sizeof(value) );

    if (pci->vendor)
    {
        /* The last seven digits are the driver number. */
        switch (pci->vendor)
        {
        /* Intel */
        case 0x8086:
            strcpy( buffer, "31.0.101.4576" );
            break;
        /* AMD */
        case 0x1002:
            strcpy( buffer, "31.0.14051.5006" );
            break;
        /* Nvidia */
        case 0x10de:
            strcpy( buffer, "31.0.15.3625" );
            break;
        /* Default value for any other vendor. */
        default:
            strcpy( buffer, "31.0.10.1000" );
            break;
        }
        set_reg_ascii_value( hkey, "DriverVersion", buffer );
    }

    NtClose( hkey );


    link_device( gpu->path, guid_devinterface_display_adapterA );
    link_device( gpu->path, guid_display_device_arrivalA );

    return TRUE;
}

static struct vulkan_gpu *find_vulkan_gpu_from_uuid( const struct device_manager_ctx *ctx, const GUID *uuid )
{
    struct vulkan_gpu *gpu;

    if (!uuid) return NULL;

    LIST_FOR_EACH_ENTRY( gpu, &ctx->vulkan_gpus, struct vulkan_gpu, entry )
        if (!memcmp( &gpu->uuid, uuid, sizeof(*uuid) )) return gpu;

    return NULL;
}

static struct vulkan_gpu *find_vulkan_gpu_from_pci_id( const struct device_manager_ctx *ctx, const struct pci_id *pci_id )
{
    struct vulkan_gpu *gpu;

    LIST_FOR_EACH_ENTRY( gpu, &ctx->vulkan_gpus, struct vulkan_gpu, entry )
        if (gpu->pci_id.vendor == pci_id->vendor && gpu->pci_id.device == pci_id->device) return gpu;

    return NULL;
}

static void add_gpu( const char *name, const struct pci_id *pci_id, const GUID *vulkan_uuid, void *param )
{
    struct device_manager_ctx *ctx = param;
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    struct vulkan_gpu *vulkan_gpu = NULL;
    struct list *ptr;
    struct gpu *gpu;
    unsigned int i;
    HKEY hkey, subkey;
    DWORD len;

    static const GUID empty_uuid;

    TRACE( "%s %04X %04X %08X %02X %s\n", debugstr_a( name ), pci_id->vendor, pci_id->device,
           pci_id->subsystem, pci_id->revision, debugstr_guid( vulkan_uuid ) );

    if (!enum_key && !(enum_key = reg_create_ascii_key( NULL, enum_keyA, 0, NULL )))
        return;

    if (!ctx->mutex)
    {
        ctx->mutex = get_display_device_init_mutex();
        prepare_devices();
    }

    if (!(gpu = calloc( 1, sizeof(*gpu) ))) return;
    gpu->refcount = 1;
    gpu->index = ctx->gpu_count;

    if ((vulkan_gpu = find_vulkan_gpu_from_uuid( ctx, vulkan_uuid )))
        TRACE( "Found vulkan GPU matching uuid %s, pci_id %#04x:%#04x, name %s\n", debugstr_guid(&vulkan_gpu->uuid),
               vulkan_gpu->pci_id.vendor, vulkan_gpu->pci_id.device, debugstr_a(vulkan_gpu->name));
    else if ((vulkan_gpu = find_vulkan_gpu_from_pci_id( ctx, pci_id )))
        TRACE( "Found vulkan GPU matching pci_id %#04x:%#04x, uuid %s, name %s\n",
               vulkan_gpu->pci_id.vendor, vulkan_gpu->pci_id.device,
               debugstr_guid(&vulkan_gpu->uuid), debugstr_a(vulkan_gpu->name));
    else if ((ptr = list_head( &ctx->vulkan_gpus )))
    {
        vulkan_gpu = LIST_ENTRY( ptr, struct vulkan_gpu, entry );
        WARN( "Using vulkan GPU pci_id %#04x:%#04x, uuid %s, name %s\n",
               vulkan_gpu->pci_id.vendor, vulkan_gpu->pci_id.device,
               debugstr_guid(&vulkan_gpu->uuid), debugstr_a(vulkan_gpu->name));
    }

    if (vulkan_uuid && !IsEqualGUID( vulkan_uuid, &empty_uuid )) gpu->vulkan_uuid = *vulkan_uuid;
    else if (vulkan_gpu) gpu->vulkan_uuid = vulkan_gpu->uuid;

    if (!pci_id->vendor && !pci_id->device && vulkan_gpu) pci_id = &vulkan_gpu->pci_id;

    if ((!name || !strcmp( name, "Wine GPU" )) && vulkan_gpu) name = vulkan_gpu->name;
    if (name) RtlUTF8ToUnicodeN( gpu->name, sizeof(gpu->name) - sizeof(WCHAR), &len, name, strlen( name ) );

    snprintf( gpu->path, sizeof(gpu->path), "PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X\\%08X",
              pci_id->vendor, pci_id->device, pci_id->subsystem, pci_id->revision, gpu->index );
    if (!(hkey = reg_create_ascii_key( enum_key, gpu->path, 0, NULL ))) return;

    if ((subkey = reg_create_ascii_key( hkey, "Device Parameters", 0, NULL )))
    {
        if (query_reg_ascii_value( subkey, "VideoID", value, sizeof(buffer) ) != sizeof(gpu->guid) * sizeof(WCHAR))
        {
            GUID guid;
            uuid_create( &guid );
            snprintf( gpu->guid, sizeof(gpu->guid), "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                      (unsigned int)guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2],
                      guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
            TRACE( "created guid %s\n", debugstr_a(gpu->guid) );
        }
        else
        {
            WCHAR *guidW = (WCHAR *)value->Data;
            for (i = 0; i < sizeof(gpu->guid); i++) gpu->guid[i] = guidW[i];
            TRACE( "got guid %s\n", debugstr_a(gpu->guid) );
        }
        NtClose( subkey );
    }

    if ((subkey = reg_create_ascii_key( hkey, devpropkey_gpu_luidA, 0, NULL )))
    {
        if (query_reg_value( subkey, NULL, value, sizeof(buffer) ) != sizeof(LUID))
        {
            NtAllocateLocallyUniqueId( &gpu->luid );
            TRACE("allocated luid %08x%08x\n", (int)gpu->luid.HighPart, (int)gpu->luid.LowPart );
        }
        else
        {
            memcpy( &gpu->luid, value->Data, sizeof(gpu->luid) );
            TRACE("got luid %08x%08x\n", (int)gpu->luid.HighPart, (int)gpu->luid.LowPart );
        }
        NtClose( subkey );
    }

    NtClose( hkey );

    if (!write_gpu_to_registry( gpu, pci_id, vulkan_gpu ? vulkan_gpu->memory : 0 ))
    {
        WARN( "Failed to write gpu %p to registry\n", gpu );
        gpu_release( gpu );
    }
    else
    {
        list_add_tail( &gpus, &gpu->entry );
        TRACE( "created gpu %p\n", gpu );
        ctx->gpu_count++;
    }

    if (vulkan_gpu)
    {
        list_remove( &vulkan_gpu->entry );
        free_vulkan_gpu( vulkan_gpu );
    }
}

static BOOL write_source_to_registry( struct source *source )
{
    struct gpu *gpu = source->gpu;
    unsigned int len, source_index = gpu->source_count;
    char name[64], buffer[MAX_PATH];
    WCHAR bufferW[MAX_PATH];
    HKEY hkey;

    snprintf( buffer, sizeof(buffer), "%s\\Video\\%s\\%04x", control_keyA, gpu->guid, source_index );
    len = asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR);

    hkey = reg_create_ascii_key( NULL, buffer, REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK, NULL );
    if (!hkey) hkey = reg_create_ascii_key( NULL, buffer, REG_OPTION_VOLATILE | REG_OPTION_OPEN_LINK, NULL );

    snprintf( name, sizeof(name), "\\Device\\Video%u", source->id );
    set_reg_ascii_value( video_key, name, buffer );

    if (!hkey) return FALSE;

    snprintf( buffer, sizeof(buffer), "%s\\Class\\%s\\%04X", control_keyA, guid_devclass_displayA, gpu->index );
    len = asciiz_to_unicode( bufferW, buffer ) - sizeof(WCHAR);
    set_reg_value( hkey, symbolic_link_valueW, REG_LINK, bufferW, len );
    NtClose( hkey );

    /* Following information is Wine specific, it doesn't really exist on Windows. */
    source->key = reg_create_ascii_key( NULL, source->path, REG_OPTION_VOLATILE, NULL );
    set_reg_ascii_value( source->key, "GPUID", gpu->path );
    set_reg_value( source->key, state_flagsW, REG_DWORD, &source->state_flags,
                   sizeof(source->state_flags) );
    set_reg_value( source->key, dpiW, REG_DWORD, &source->dpi, sizeof(source->dpi) );

    snprintf( buffer, sizeof(buffer), "System\\CurrentControlSet\\Control\\Video\\%s\\%04x", gpu->guid, source_index );
    hkey = reg_create_ascii_key( config_key, buffer, REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK, NULL );
    if (!hkey) hkey = reg_create_ascii_key( config_key, buffer, REG_OPTION_VOLATILE | REG_OPTION_OPEN_LINK, NULL );

    len = asciiz_to_unicode( bufferW, source->path ) - sizeof(WCHAR);
    set_reg_value( hkey, symbolic_link_valueW, REG_LINK, bufferW, len );
    NtClose( hkey );

    return TRUE;
}

static void add_source( const char *name, UINT state_flags, UINT dpi, void *param )
{
    struct device_manager_ctx *ctx = param;
    struct source *source;
    struct gpu *gpu;
    BOOL is_primary;

    TRACE( "name %s, state_flags %#x\n", name, state_flags );

    assert( !list_empty( &gpus ) );
    gpu = LIST_ENTRY( list_tail( &gpus ), struct gpu, entry );

    /* in virtual desktop mode, report all physical sources as detached */
    is_primary = !!(state_flags & DISPLAY_DEVICE_PRIMARY_DEVICE);
    if (is_virtual_desktop()) state_flags &= ~(DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE);

    if (!(source = calloc( 1, sizeof(*source) ))) return;
    source->refcount = 1;
    source->gpu = gpu_acquire( gpu );
    source->id = ctx->source_count + (ctx->has_primary ? 0 : 1);
    source->state_flags = state_flags;
    if (state_flags & DISPLAY_DEVICE_PRIMARY_DEVICE)
    {
        source->id = 0;
        ctx->has_primary = TRUE;
    }
    source->dpi = dpi;

    /* Wine specific config key where source settings will be held, symlinked with the logically indexed config key */
    snprintf( source->path, sizeof(source->path), "%s\\%s\\Video\\%s\\Sources\\%s", config_keyA,
              control_keyA + strlen( "\\Registry\\Machine" ), gpu->guid, name );

    if (!write_source_to_registry( source ))
    {
        WARN( "Failed to write source %p to registry\n", source );
        source_release( source );
    }
    else
    {
        list_add_tail( &sources, &source->entry );
        TRACE( "created source %p for gpu %p\n", source, gpu );
        ctx->is_primary = is_primary;
        gpu->source_count++;
        ctx->source_count++;
    }
}

static BOOL write_monitor_to_registry( struct monitor *monitor, const BYTE *edid, UINT edid_len )
{
    char buffer[1024], *tmp;
    WCHAR bufferW[1024];
    HKEY hkey, subkey;
    unsigned int len;

    if (!(hkey = reg_create_ascii_key( enum_key, monitor->path, 0, NULL ))) return FALSE;

    set_reg_ascii_value( hkey, "DeviceDesc", "Generic Non-PnP Monitor" );

    set_reg_ascii_value( hkey, "Class", "Monitor" );
    snprintf( buffer, sizeof(buffer), "%s\\%04X", guid_devclass_monitorA, monitor->output_id );
    set_reg_ascii_value( hkey, "Driver", buffer );
    set_reg_ascii_value( hkey, "ClassGUID", guid_devclass_monitorA );

    snprintf( buffer, sizeof(buffer), "MONITOR\\%s", monitor->path + 8 );
    if ((tmp = strrchr( buffer, '\\' ))) *tmp = 0;
    len = asciiz_to_unicode( bufferW, buffer );
    bufferW[len / sizeof(WCHAR)] = 0;
    set_reg_value( hkey, hardware_idW, REG_MULTI_SZ, bufferW, len + sizeof(WCHAR) );

    if ((subkey = reg_create_ascii_key( hkey, "Device Parameters", 0, NULL )))
    {
        static const WCHAR bad_edidW[] = {'B','A','D','_','E','D','I','D',0};
        static const WCHAR edidW[] = {'E','D','I','D',0};

        if (edid_len)
            set_reg_value( subkey, edidW, REG_BINARY, edid, edid_len );
        else
            set_reg_value( subkey, bad_edidW, REG_BINARY, NULL, 0 );
        NtClose( subkey );
    }

    /* WINE_DEVPROPKEY_MONITOR_RCWORK */
    if ((subkey = reg_create_ascii_key( hkey, wine_devpropkey_monitor_rcworkA, 0, NULL )))
    {
        TRACE( "rc_work %s\n", wine_dbgstr_rect(&monitor->rc_work) );
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_BINARY, &monitor->rc_work,
                       sizeof(monitor->rc_work) );
        NtClose( subkey );
    }

    /* DEVPROPKEY_MONITOR_GPU_LUID */
    if ((subkey = reg_create_ascii_key( hkey, devpropkey_monitor_gpu_luidA, 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_INT64,
                       &monitor->source->gpu->luid, sizeof(monitor->source->gpu->luid) );
        NtClose( subkey );
    }

    /* DEVPROPKEY_MONITOR_OUTPUT_ID */
    if ((subkey = reg_create_ascii_key( hkey, devpropkey_monitor_output_idA, 0, NULL )))
    {
        set_reg_value( subkey, NULL, 0xffff0000 | DEVPROP_TYPE_UINT32,
                       &monitor->output_id, sizeof(monitor->output_id) );
        NtClose( subkey );
    }

    NtClose( hkey );


    snprintf( buffer, sizeof(buffer), "Class\\%s\\%04X", guid_devclass_monitorA, monitor->output_id );
    if (!(hkey = reg_create_ascii_key( control_key, buffer, 0, NULL ))) return FALSE;
    NtClose( hkey );

    link_device( monitor->path, guid_devinterface_monitorA );

    return TRUE;
}

static void add_monitor( const struct gdi_monitor *gdi_monitor, void *param )
{
    struct device_manager_ctx *ctx = param;
    struct monitor *monitor;
    struct source *source;
    char buffer[MAX_PATH];
    char monitor_id_string[16];

    assert( !list_empty( &sources ) );
    source = LIST_ENTRY( list_tail( &sources ), struct source, entry );

    if (!(monitor = calloc( 1, sizeof(*monitor) ))) return;
    monitor->refcount = 1;
    monitor->source = source_acquire( source );
    monitor->id = source->monitor_count;
    monitor->output_id = ctx->monitor_count;
    monitor->rc_work = gdi_monitor->rc_work;

    TRACE( "%u %s %s\n", monitor->id, wine_dbgstr_rect(&gdi_monitor->rc_monitor), wine_dbgstr_rect(&gdi_monitor->rc_work) );

    get_monitor_info_from_edid( &monitor->edid_info, gdi_monitor->edid, gdi_monitor->edid_len );
    if (monitor->edid_info.flags & MONITOR_INFO_HAS_MONITOR_ID)
        strcpy( monitor_id_string, monitor->edid_info.monitor_id_string );
    else
        strcpy( monitor_id_string, "Default_Monitor" );

    snprintf( buffer, sizeof(buffer), "MonitorID%u", monitor->id );
    snprintf( monitor->path, sizeof(monitor->path), "DISPLAY\\%s\\%04X&%04X", monitor_id_string, source->id, monitor->id );
    set_reg_ascii_value( source->key, buffer, monitor->path );

    if (!write_monitor_to_registry( monitor, gdi_monitor->edid, gdi_monitor->edid_len ))
    {
        WARN( "Failed to write monitor %p to registry\n", monitor );
        monitor_release( monitor );
    }
    else
    {
        list_add_tail( &monitors, &monitor->entry );
        TRACE( "created monitor %p for source %p\n", monitor, source );
        source->monitor_count++;
        ctx->monitor_count++;
    }
}

static UINT add_screen_size( SIZE *sizes, UINT count, SIZE size )
{
    UINT i = 0;

    while (i < count && memcmp( sizes + i, &size, sizeof(size) )) i++;
    if (i < count) return 0;

    TRACE( "adding size %s\n", wine_dbgstr_point((POINT *)&size) );
    sizes[i] = size;
    return 1;
}

static UINT add_virtual_mode( DEVMODEW *modes, UINT count, const DEVMODEW *mode )
{
    TRACE( "adding mode %s\n", debugstr_devmodew(mode) );
    modes[count] = *mode;
    return 1;
}

static SIZE *get_screen_sizes( const DEVMODEW *maximum, const DEVMODEW *modes, UINT modes_count,
                               UINT *sizes_count )
{
    static SIZE default_sizes[] =
    {
        /* 4:3 */
        { 640,  480},
        { 800,  600},
        {1024,  768},
        {1600, 1200},
        /* 16:9 */
        { 960,  540},
        {1280,  720},
        {1600,  900},
        {1920, 1080},
        {2560, 1440},
        {2880, 1620},
        {3200, 1800},
        /* 16:10 */
        {1440,  900},
        {1680, 1050},
        {1920, 1200},
        {2560, 1600},
        /* 3:2 */
        {1440,  960},
        {1920, 1280},
        /* 21:9 ultra-wide */
        {2560, 1080},
        /* 12:5 */
        {1920,  800},
        {3840, 1600},
        /* 5:4 */
        {1280, 1024},
        /* 5:3 */
        {1280,  768},
    };
    UINT max_width = devmode_get( maximum, DM_PELSWIDTH ), max_height = devmode_get( maximum, DM_PELSHEIGHT );
    SIZE *sizes, max_size = {.cx = max( max_width, max_height ), .cy = min( max_width, max_height )};
    const DEVMODEW *mode;
    UINT i, count;

    count = 1 + ARRAY_SIZE(default_sizes) + modes_count;
    if (!(sizes = malloc( count * sizeof(*sizes) ))) return NULL;

    count = add_screen_size( sizes, 0, max_size );
    for (i = 0; i < ARRAY_SIZE(default_sizes); i++)
    {
        if (default_sizes[i].cx > max_size.cx || default_sizes[i].cy > max_size.cy) continue;
        count += add_screen_size( sizes, count, default_sizes[i] );
    }

    for (mode = modes; mode && modes_count; mode = NEXT_DEVMODEW(mode), modes_count--)
    {
        UINT width = devmode_get( mode, DM_PELSWIDTH ), height = devmode_get( mode, DM_PELSHEIGHT );
        SIZE size = {.cx = max( width, height ), .cy = min( width, height )};
        if (!size.cx || size.cx > max_size.cx) continue;
        if (!size.cy || size.cy > max_size.cy) continue;
        count += add_screen_size( sizes, count, size );
    }

    *sizes_count = count;
    return sizes;
}

static DEVMODEW *get_virtual_modes( const DEVMODEW *initial, const DEVMODEW *maximum,
                                    const DEVMODEW *host_modes, UINT host_modes_count, UINT32 *modes_count )
{
    UINT depths[] = {8, 16, initial->dmBitsPerPel}, freqs[] = {60, -1}, sizes_count, i, j, f, count = 0;
    DEVMODEW *modes = NULL;
    SIZE *screen_sizes;
    BOOL vertical;

    /* Check the ratio of dmPelsWidth to dmPelsHeight to determine whether the initial display mode
     * is in horizontal or vertical orientation. DMDO_DEFAULT is the natural orientation of the
     * device, which isn't necessarily a horizontal mode */
    vertical = initial->dmPelsHeight > initial->dmPelsWidth;

    freqs[1] = devmode_get( initial, DM_DISPLAYFREQUENCY );
    if (freqs[1] <= 60) freqs[1] = 0;

    if (!(screen_sizes = get_screen_sizes( maximum, host_modes, host_modes_count, &sizes_count ))) return NULL;
    modes = malloc( ARRAY_SIZE(freqs) * ARRAY_SIZE(depths) * (sizes_count + 2) * sizeof(*modes) );

    for (i = 0; modes && i < ARRAY_SIZE(depths); ++i)
    for (f = 0; f < ARRAY_SIZE(freqs); ++f)
    {
        DEVMODEW mode =
        {
            .dmSize = sizeof(mode),
            .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
            .dmDisplayFrequency = freqs[f],
            .dmBitsPerPel = depths[i],
            .dmDisplayOrientation = initial->dmDisplayOrientation,
        };
        if (!mode.dmDisplayFrequency) continue;

        for (j = 0; j < sizes_count; ++j)
        {
            mode.dmPelsWidth = vertical ? screen_sizes[j].cy : screen_sizes[j].cx;
            mode.dmPelsHeight = vertical ? screen_sizes[j].cx : screen_sizes[j].cy;

            if (mode.dmPelsWidth > maximum->dmPelsWidth || mode.dmPelsHeight > maximum->dmPelsHeight) continue;
            if (mode.dmPelsWidth == maximum->dmPelsWidth && mode.dmPelsHeight == maximum->dmPelsHeight) continue;
            if (mode.dmPelsWidth == initial->dmPelsWidth && mode.dmPelsHeight == initial->dmPelsHeight) continue;
            count += add_virtual_mode( modes, count, &mode );
        }

        mode.dmPelsWidth = vertical ? initial->dmPelsHeight : initial->dmPelsWidth;
        mode.dmPelsHeight = vertical ? initial->dmPelsWidth : initial->dmPelsHeight;
        count += add_virtual_mode( modes, count, &mode );

        if (maximum->dmPelsWidth != initial->dmPelsWidth || maximum->dmPelsHeight != initial->dmPelsHeight)
        {
            mode.dmPelsWidth = vertical ? maximum->dmPelsHeight : maximum->dmPelsWidth;
            mode.dmPelsHeight = vertical ? maximum->dmPelsWidth : maximum->dmPelsHeight;
            count += add_virtual_mode( modes, count, &mode );
        }
    }

    *modes_count = count;
    free( screen_sizes );
    return modes;
}

static void add_modes( const DEVMODEW *current, UINT host_modes_count, const DEVMODEW *host_modes, void *param )
{
    struct device_manager_ctx *ctx = param;
    DEVMODEW dummy, physical, detached = *current, virtual, *virtual_modes = NULL;
    UINT virtual_count, modes_count = host_modes_count;
    const DEVMODEW *modes = host_modes;
    struct source *source;

    TRACE( "current %s, host_modes_count %u, host_modes %p, param %p\n", debugstr_devmodew( current ),
           host_modes_count, host_modes, param );

    assert( !list_empty( &sources ) );
    source = LIST_ENTRY( list_tail( &sources ), struct source, entry );

    if (emulate_modeset)
    {
        modes = current;
        modes_count = 1;
    }

    physical = modes_count == 1 ? *modes : *current;
    if (ctx->is_primary) ctx->primary = *current;

    detached.dmPelsWidth = 0;
    detached.dmPelsHeight = 0;
    if (!(source->state_flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) current = &detached;

    if (modes_count > 1 || current == &detached)
    {
        reg_delete_value( source->key, physicalW );
        virtual_modes = NULL;
    }
    else
    {
        if (!read_source_mode( source->key, ENUM_CURRENT_SETTINGS, &virtual ))
            virtual = physical;

        if ((virtual_modes = get_virtual_modes( current, &physical, host_modes, host_modes_count, &virtual_count )))
        {
            modes_count = virtual_count;
            modes = virtual_modes;
            current = &virtual;

            write_source_mode( source->key, WINE_ENUM_PHYSICAL_SETTINGS, &physical );
        }
    }

    if (current == &detached || !read_source_mode( source->key, ENUM_REGISTRY_SETTINGS, &dummy ))
        write_source_mode( source->key, ENUM_REGISTRY_SETTINGS, current );
    write_source_mode( source->key, ENUM_CURRENT_SETTINGS, current );

    assert( !modes_count || modes->dmDriverExtra == 0 );
    set_reg_value( source->key, modesW, REG_BINARY, modes, modes_count * sizeof(*modes) );
    set_reg_value( source->key, mode_countW, REG_DWORD, &modes_count, sizeof(modes_count) );
    source->mode_count = modes_count;
    source->current = *current;
    source->physical = physical;

    free( virtual_modes );
}

static const struct gdi_device_manager device_manager =
{
    add_gpu,
    add_source,
    add_monitor,
    add_modes,
};

static void release_display_manager_ctx( struct device_manager_ctx *ctx )
{
    if (ctx->mutex)
    {
        release_display_device_init_mutex( ctx->mutex );
        ctx->mutex = 0;
    }

    if (!list_empty( &sources )) last_query_display_time = 0;
    if (ctx->gpu_count) cleanup_devices();

    while (!list_empty( &ctx->vulkan_gpus ))
    {
        struct vulkan_gpu *gpu = LIST_ENTRY( list_head( &ctx->vulkan_gpus ), struct vulkan_gpu, entry );
        list_remove( &gpu->entry );
        free_vulkan_gpu( gpu );
    }
}

static BOOL is_detached_mode( const DEVMODEW *mode )
{
    return mode->dmFields & DM_POSITION &&
           mode->dmFields & DM_PELSWIDTH &&
           mode->dmFields & DM_PELSHEIGHT &&
           mode->dmPelsWidth == 0 &&
           mode->dmPelsHeight == 0;
}

static BOOL is_monitor_active( struct monitor *monitor )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
    struct source *source;
    /* services do not have any adapters, only a virtual monitor */
    if (!(source = monitor->source)) return TRUE;
    if (!(source->state_flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) return FALSE;
    source_get_current_settings( source, &current_mode );
    return !is_detached_mode( &current_mode );
}

static BOOL is_monitor_primary( struct monitor *monitor )
{
    struct source *source;
    /* services do not have any adapters, only a virtual monitor */
    if (!(source = monitor->source)) return TRUE;
    return !!(source->state_flags & DISPLAY_DEVICE_PRIMARY_DEVICE);
}

/* display_lock must be held */
static void monitor_virt_to_raw_ratio( struct monitor *monitor, UINT *num, UINT *den )
{
    struct source *source = monitor->source;

    *num = *den = 1;
    if (!source) return;

    if (source->physical.dmPelsWidth * source->current.dmPelsHeight <=
        source->physical.dmPelsHeight * source->current.dmPelsWidth)
    {
        *num = source->physical.dmPelsWidth;
        *den = source->current.dmPelsWidth;
    }
    else
    {
        *num = source->physical.dmPelsHeight;
        *den = source->current.dmPelsHeight;
    }
}

/* display_lock must be held */
static UINT monitor_get_dpi( struct monitor *monitor, MONITOR_DPI_TYPE type, UINT *dpi_x, UINT *dpi_y )
{
    struct source *source = monitor->source;
    float scale_x = 1.0, scale_y = 1.0;
    UINT dpi;

    if (!source || !(dpi = source->dpi)) dpi = system_dpi;
    if (source && type != MDT_EFFECTIVE_DPI)
    {
        scale_x = source->physical.dmPelsWidth / (float)source->current.dmPelsWidth;
        scale_y = source->physical.dmPelsHeight / (float)source->current.dmPelsHeight;
    }

    *dpi_x = round( dpi * scale_x );
    *dpi_y = round( dpi * scale_y );
    return min( *dpi_x, *dpi_y );
}

/* display_lock must be held */
static RECT monitor_get_rect( struct monitor *monitor, UINT dpi, MONITOR_DPI_TYPE type )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
    RECT rect = {0, 0, 1024, 768};
    struct source *source;
    UINT dpi_from, x, y;
    DEVMODEW *mode;

    /* services do not have any adapters, only a virtual monitor */
    if (!(source = monitor->source)) return rect;

    SetRectEmpty( &rect );
    if (!(source->state_flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) return rect;
    source_get_current_settings( source, &current_mode );

    mode = type != MDT_EFFECTIVE_DPI ? &source->physical : &current_mode;
    SetRect( &rect, mode->dmPosition.x, mode->dmPosition.y,
             mode->dmPosition.x + mode->dmPelsWidth,
             mode->dmPosition.y + mode->dmPelsHeight );

    dpi_from = monitor_get_dpi( monitor, type, &x, &y );
    return map_dpi_rect( rect, dpi_from, dpi );
}

/* display_lock must be held */
static void monitor_get_info( struct monitor *monitor, MONITORINFO *info, UINT dpi )
{
    UINT x, y;

    info->rcMonitor = monitor_get_rect( monitor, dpi, MDT_DEFAULT );
    info->rcWork = map_dpi_rect( monitor->rc_work, monitor_get_dpi( monitor, MDT_DEFAULT, &x, &y ), dpi );
    info->dwFlags = is_monitor_primary( monitor ) ? MONITORINFOF_PRIMARY : 0;

    if (info->cbSize >= sizeof(MONITORINFOEXW))
    {
        char buffer[CCHDEVICENAME];
        if (monitor->source) snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d", monitor->source->id + 1 );
        else strcpy( buffer, "WinDisc" );
        asciiz_to_unicode( ((MONITORINFOEXW *)info)->szDevice, buffer );
    }
}

/* display_lock must be held */
static void set_winstation_monitors( BOOL increment )
{
    struct monitor_info *infos, *info;
    struct monitor *monitor;
    UINT count, x, y;

    if (!(count = list_count( &monitors ))) return;
    if (!(info = infos = calloc( count, sizeof(*infos) ))) return;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (is_monitor_primary( monitor )) info->flags |= MONITOR_FLAG_PRIMARY;
        if (!is_monitor_active( monitor )) info->flags |= MONITOR_FLAG_INACTIVE;
        if (monitor->is_clone) info->flags |= MONITOR_FLAG_CLONE;
        info->dpi = monitor_get_dpi( monitor, MDT_EFFECTIVE_DPI, &x, &y );
        info->virt = wine_server_rectangle( monitor_get_rect( monitor, 0, MDT_EFFECTIVE_DPI ) );
        info->raw = wine_server_rectangle( monitor_get_rect( monitor, 0, MDT_RAW_DPI ) );
        info++;
    }

    SERVER_START_REQ( set_winstation_monitors )
    {
        req->increment = increment;
        wine_server_add_data( req, infos, count * sizeof(*infos) );
        if (!wine_server_call( req )) monitor_update_serial = reply->serial;
    }
    SERVER_END_REQ;

    free( infos );
}

static void enum_device_keys( const char *root, const WCHAR *classW, UINT class_size, void (*callback)(const char *) )
{
    char buffer[1024];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    KEY_BASIC_INFORMATION *key2 = (void *)buffer;
    HKEY root_key, device_key, instance_key;
    DWORD size, root_len, i = 0;
    char path[MAX_PATH];

    if (!(root_key = reg_open_ascii_key( enum_key, root ))) return;
    root_len = snprintf( path, sizeof(path), "%s\\", root );

    while (!NtEnumerateKey( root_key, i++, KeyBasicInformation, key2, sizeof(buffer), &size ))
    {
        DWORD j = 0, k, len, device_len;

        if (!(device_key = reg_open_key( root_key, key2->Name, key2->NameLength ))) continue;
        for (k = 0, len = root_len; k < key2->NameLength / sizeof(WCHAR); k++) path[len++] = key2->Name[k];
        path[len++] = '\\';
        device_len = len;

        while (!NtEnumerateKey( device_key, j++, KeyBasicInformation, key2, sizeof(buffer), &size ))
        {
            if (!(instance_key = reg_open_key( device_key, key2->Name, key2->NameLength ))) continue;
            for (k = 0, len = device_len; k < key2->NameLength / sizeof(WCHAR); k++) path[len++] = key2->Name[k];
            path[len++] = 0;

            size = query_reg_ascii_value( instance_key, "ClassGUID", value, sizeof(buffer) );
            if (size != class_size || wcscmp( (WCHAR *)value->Data, classW ))
            {
                NtClose( instance_key );
                continue;
            }

            callback( path );

            NtClose( instance_key );
        }

        NtClose( device_key );
    }

    NtClose( root_key );
}

static void enum_gpus( const char *path )
{
    struct gpu *gpu;
    if (!(gpu = calloc( 1, sizeof(*gpu) ))) return;
    gpu->refcount = 1;
    strcpy( gpu->path, path );
    list_add_tail( &gpus, &gpu->entry );
}

static struct gpu *find_gpu_from_path( const char *path )
{
    struct gpu *gpu;

    LIST_FOR_EACH_ENTRY( gpu, &gpus, struct gpu, entry )
        if (!strcmp( gpu->path, path )) return gpu_acquire( gpu );

    ERR( "Failed to find gpu with path %s\n", debugstr_a(path) );
    return NULL;
}

static void enum_monitors( const char *path )
{
    struct monitor *monitor;
    if (!(monitor = calloc( 1, sizeof(*monitor) ))) return;
    monitor->refcount = 1;
    strcpy( monitor->path, path );
    list_add_tail( &monitors, &monitor->entry );
}

static struct monitor *find_monitor_from_path( const char *path )
{
    struct monitor *monitor;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
        if (!strcmp( monitor->path, path )) return monitor;

    ERR( "Failed to find monitor with path %s\n", debugstr_a(path) );
    return NULL;
}

static BOOL update_display_cache_from_registry( UINT64 serial )
{
    char path[MAX_PATH];
    DWORD source_id, monitor_id, monitor_count = 0, size;
    KEY_BASIC_INFORMATION key;
    struct source *source;
    struct monitor *monitor;
    HANDLE mutex = NULL;
    NTSTATUS status;
    struct gpu *gpu;
    HKEY hkey;
    BOOL ret;

    /* If user driver did initialize the registry, then exit */
    if (!enum_key && !(enum_key = reg_open_ascii_key( NULL, enum_keyA )))
        return FALSE;
    if (!video_key && !(video_key = reg_open_ascii_key( NULL, devicemap_video_keyA )))
        return FALSE;

    status = NtQueryKey( video_key, KeyBasicInformation, &key,
                         offsetof(KEY_BASIC_INFORMATION, Name), &size );
    if (status && status != STATUS_BUFFER_OVERFLOW)
        return FALSE;

    if (key.LastWriteTime.QuadPart <= last_query_display_time)
    {
        monitor_update_serial = serial;
        return TRUE;
    }

    mutex = get_display_device_init_mutex();

    clear_display_devices();

    enum_device_keys( "PCI", guid_devclass_displayW, sizeof(guid_devclass_displayW), enum_gpus );
    enum_device_keys( "DISPLAY", guid_devclass_monitorW, sizeof(guid_devclass_monitorW), enum_monitors );

    LIST_FOR_EACH_ENTRY( gpu, &gpus, struct gpu, entry )
    {
        if (!read_gpu_from_registry( gpu ))
            WARN( "Failed to read gpu from registry\n" );
    }

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!read_monitor_from_registry( monitor ))
            WARN( "Failed to read monitor from registry\n" );
    }

    for (source_id = 0;; source_id++)
    {
        if (!(source = calloc( 1, sizeof(*source) ))) break;
        source->refcount = 1;
        source->id = source_id;

        if (!read_source_from_registry( source_id, source, path ) ||
            !(source->gpu = find_gpu_from_path( path )))
        {
            free( source->modes );
            free( source );
            break;
        }

        list_add_tail( &sources, &source->entry );
        if (!(hkey = reg_open_ascii_key( config_key, source->path ))) continue;

        for (monitor_id = 0;; monitor_id++)
        {
            struct monitor *monitor;

            if (!read_source_monitor_path( hkey, monitor_id, path )) break;
            if (!(monitor = find_monitor_from_path( path ))) continue;

            monitor->id = monitor_id;
            monitor->source = source_acquire( source );
            monitor->handle = UlongToHandle( ++monitor_count );
            if (source->monitor_count++) monitor->is_clone = TRUE;
        }

        NtClose( hkey );
    }

    if ((ret = !list_empty( &sources ) && !list_empty( &monitors )))
        last_query_display_time = key.LastWriteTime.QuadPart;

    set_winstation_monitors( FALSE );
    release_display_device_init_mutex( mutex );
    return ret;
}

static NTSTATUS default_update_display_devices( struct device_manager_ctx *ctx )
{
    /* default implementation: expose an adapter and a monitor with a few standard modes,
     * and read / write current display settings from / to the registry.
     */
    static const DEVMODEW modes[] =
    {
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 32, .dmPelsWidth = 640, .dmPelsHeight = 480, .dmDisplayFrequency = 60, },
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 32, .dmPelsWidth = 800, .dmPelsHeight = 600, .dmDisplayFrequency = 60, },
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 32, .dmPelsWidth = 1024, .dmPelsHeight = 768, .dmDisplayFrequency = 60, },
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 16, .dmPelsWidth = 640, .dmPelsHeight = 480, .dmDisplayFrequency = 60, },
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 16, .dmPelsWidth = 800, .dmPelsHeight = 600, .dmDisplayFrequency = 60, },
        { .dmSize = sizeof(DEVMODEW),
          .dmFields = DM_DISPLAYORIENTATION | DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
          .dmBitsPerPel = 16, .dmPelsWidth = 1024, .dmPelsHeight = 768, .dmDisplayFrequency = 60, },
    };
    static const DWORD source_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE;
    struct pci_id pci_id = {0};
    struct gdi_monitor monitor = {0};
    DEVMODEW mode = {.dmSize = sizeof(mode)};
    struct source *source;

    add_gpu( "Wine GPU", &pci_id, NULL, ctx );
    add_source( "Default", source_flags, system_dpi, ctx );

    assert( !list_empty( &sources ) );
    source = LIST_ENTRY( list_tail( &sources ), struct source, entry );

    if (!read_source_mode( source->key, ENUM_CURRENT_SETTINGS, &mode ))
    {
        mode = modes[2];
        mode.dmFields |= DM_POSITION;
    }
    monitor.rc_monitor.right = mode.dmPelsWidth;
    monitor.rc_monitor.bottom = mode.dmPelsHeight;
    monitor.rc_work.right = mode.dmPelsWidth;
    monitor.rc_work.bottom = mode.dmPelsHeight;

    add_monitor( &monitor, ctx );
    add_modes( &mode, ARRAY_SIZE(modes), modes, ctx );

    return STATUS_SUCCESS;
}

/* parse the desktop size specification */
static BOOL parse_size( const WCHAR *size, DWORD *width, DWORD *height )
{
    WCHAR *end;

    *width = wcstoul( size, &end, 10 );
    if (end == size) return FALSE;
    if (*end != 'x') return FALSE;
    size = end + 1;
    *height = wcstoul( size, &end, 10 );
    return !*end;
}

/* retrieve the default desktop size from the registry */
static BOOL get_default_desktop_size( DWORD *width, DWORD *height )
{
    WCHAR buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    DWORD size;
    HKEY hkey;

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer\Desktops */
    if (!(hkey = reg_open_hkcu_key( "Software\\Wine\\Explorer\\Desktops" ))) return FALSE;

    size = query_reg_ascii_value( hkey, "Default", value, sizeof(buffer) );
    NtClose( hkey );
    if (!size || value->Type != REG_SZ) return FALSE;

    if (!parse_size( (const WCHAR *)value->Data, width, height )) return FALSE;
    return TRUE;
}

static BOOL add_virtual_source( struct device_manager_ctx *ctx )
{
    DEVMODEW current = {.dmSize = sizeof(current)}, initial = ctx->primary, maximum = ctx->primary, *modes;
    struct source *physical, *source;
    struct gdi_monitor monitor = {0};
    UINT modes_count;
    struct gpu *gpu;

    assert( !list_empty( &gpus ) );
    gpu = LIST_ENTRY( list_tail( &gpus ), struct gpu, entry );

    if (list_empty( &sources )) physical = NULL;
    else physical = LIST_ENTRY( list_tail( &sources ), struct source, entry );

    if (!(source = calloc( 1, sizeof(*source) ))) return STATUS_NO_MEMORY;
    source->refcount = 1;
    source->state_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_VGA_COMPATIBLE;

    if (ctx->has_primary && physical) physical->id = ctx->source_count;
    else
    {
        source->state_flags |= DISPLAY_DEVICE_PRIMARY_DEVICE;
        ctx->has_primary = TRUE;
    }
    source->gpu = gpu_acquire( gpu );

    /* Wine specific config key where source settings will be held, symlinked with the logically indexed config key */
    snprintf( source->path, sizeof(source->path), "%s\\%s\\Video\\%s\\Sources\\%s", config_keyA,
              control_keyA + strlen( "\\Registry\\Machine" ), gpu->guid, "Virtual" );

    if (!write_source_to_registry( source ))
    {
        WARN( "Failed to write source to registry\n" );
        source_release( source );
        return STATUS_UNSUCCESSFUL;
    }

    list_add_tail( &sources, &source->entry );
    gpu->source_count++;
    ctx->source_count++;

    if (!get_default_desktop_size( &initial.dmPelsWidth, &initial.dmPelsHeight ))
    {
        initial.dmPelsWidth = maximum.dmPelsWidth;
        initial.dmPelsHeight = maximum.dmPelsHeight;
    }

    if (!read_source_mode( source->key, ENUM_CURRENT_SETTINGS, &current ))
    {
        current = ctx->primary;
        current.dmDisplayFrequency = 60;
        current.dmPelsWidth = initial.dmPelsWidth;
        current.dmPelsHeight = initial.dmPelsHeight;
    }

    monitor.rc_monitor.right = current.dmPelsWidth;
    monitor.rc_monitor.bottom = current.dmPelsHeight;
    monitor.rc_work.right = current.dmPelsWidth;
    monitor.rc_work.bottom = current.dmPelsHeight;
    add_monitor( &monitor, ctx );

    /* Expose the virtual source display modes as physical modes, to avoid DPI scaling */
    if (!(modes = get_virtual_modes( &initial, &maximum, NULL, 0, &modes_count ))) return STATUS_NO_MEMORY;
    add_modes( &current, modes_count, modes, ctx );
    free( modes );

    return STATUS_SUCCESS;
}

static UINT update_display_devices( struct device_manager_ctx *ctx )
{
    UINT status;

    if (!(status = user_driver->pUpdateDisplayDevices( &device_manager, ctx )))
    {
        if (ctx->source_count && is_virtual_desktop()) return add_virtual_source( ctx );
        return status;
    }

    if (status == STATUS_NOT_IMPLEMENTED) return default_update_display_devices( ctx );
    return status;
}

static void commit_display_devices( struct device_manager_ctx *ctx )
{
    struct vulkan_gpu *gpu, *next;

    LIST_FOR_EACH_ENTRY_SAFE( gpu, next, &ctx->vulkan_gpus, struct vulkan_gpu, entry )
    {
        TRACE( "adding vulkan-only gpu uuid %s, name %s\n", debugstr_guid(&gpu->uuid), debugstr_a(gpu->name));
        add_gpu( gpu->name, &gpu->pci_id, &gpu->uuid, ctx );
    }

    set_winstation_monitors( TRUE );
}

static UINT64 get_monitor_update_serial(void)
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const desktop_shm_t *desktop_shm;
    UINT64 serial = 0;
    NTSTATUS status;

    while ((status = get_shared_desktop( &lock, &desktop_shm )) == STATUS_PENDING)
        serial = desktop_shm->monitor_serial;

    return status ? 0 : serial;
}

void reset_monitor_update_serial(void)
{
    pthread_mutex_lock( &display_lock );
    monitor_update_serial = 0;
    pthread_mutex_unlock( &display_lock );
}

static BOOL lock_display_devices( BOOL force )
{
    static const WCHAR wine_service_station_name[] =
        {'_','_','w','i','n','e','s','e','r','v','i','c','e','_','w','i','n','s','t','a','t','i','o','n',0};
    struct device_manager_ctx ctx = {.vulkan_gpus = LIST_INIT(ctx.vulkan_gpus)};
    UINT64 serial;
    UINT status;
    WCHAR name[MAX_PATH];
    BOOL ret = TRUE;

    init_display_driver(); /* make sure to load the driver before anything else */

    pthread_mutex_lock( &display_lock );

    serial = get_monitor_update_serial();
    if (!force && monitor_update_serial >= serial) return TRUE;

    /* services do not have any adapters, only a virtual monitor */
    if (NtUserGetObjectInformation( NtUserGetProcessWindowStation(), UOI_NAME, name, sizeof(name), NULL )
        && !wcscmp( name, wine_service_station_name ))
    {
        clear_display_devices();
        list_add_tail( &monitors, &virtual_monitor.entry );
        set_winstation_monitors( TRUE );
        return TRUE;
    }

    if (!force && !update_display_cache_from_registry( serial )) force = TRUE;
    if (force)
    {
        if (!get_vulkan_gpus( &ctx.vulkan_gpus )) WARN( "Failed to find any vulkan GPU\n" );
        if (!(status = update_display_devices( &ctx ))) commit_display_devices( &ctx );
        else WARN( "Failed to update display devices, status %#x\n", status );
        release_display_manager_ctx( &ctx );

        ret = update_display_cache_from_registry( serial );
    }

    if (!ret)
    {
        ERR( "Failed to read display config.\n" );
        pthread_mutex_unlock( &display_lock );
    }
    return ret;
}

static void unlock_display_devices(void)
{
    pthread_mutex_unlock( &display_lock );
}

BOOL update_display_cache( BOOL force )
{
    if (!lock_display_devices( force )) return FALSE;
    unlock_display_devices();
    return TRUE;
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

HBITMAP get_display_bitmap(void)
{
    static RECT old_virtual_rect;
    static HBITMAP hbitmap;
    RECT virtual_rect;
    HBITMAP ret;

    virtual_rect = get_virtual_screen_rect( 0, MDT_DEFAULT );
    pthread_mutex_lock( &display_dc_lock );
    if (!EqualRect( &old_virtual_rect, &virtual_rect ))
    {
        if (hbitmap) NtGdiDeleteObjectApp( hbitmap );
        hbitmap = NtGdiCreateBitmap( virtual_rect.right - virtual_rect.left,
                                     virtual_rect.bottom - virtual_rect.top, 1, 32, NULL );
        old_virtual_rect = virtual_rect;
    }
    ret = hbitmap;
    pthread_mutex_unlock( &display_dc_lock );
    return ret;
}

static void release_display_dc( HDC hdc )
{
    pthread_mutex_unlock( &display_dc_lock );
}

/* display_lock must be held, keep in sync with server/window.c */
static struct monitor *get_monitor_from_rect( RECT rect, UINT flags, UINT dpi, MONITOR_DPI_TYPE type )
{
    struct monitor *monitor, *primary = NULL, *nearest = NULL, *found = NULL;
    UINT max_area = 0, min_distance = -1;

    if (IsRectEmpty( &rect ))
    {
        rect.right = rect.left + 1;
        rect.bottom = rect.top + 1;
    }

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        RECT intersect, monitor_rect;

        if (!is_monitor_active( monitor ) || monitor->is_clone) continue;

        monitor_rect = monitor_get_rect( monitor, dpi, type );
        if (intersect_rect( &intersect, &monitor_rect, &rect ))
        {
            /* check for larger intersecting area */
            UINT area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
            if (area > max_area)
            {
                max_area = area;
                found = monitor;
            }
        }

        if (!found && (flags & MONITOR_DEFAULTTONEAREST))  /* if not intersecting, check for min distance */
        {
            UINT distance;
            UINT x, y;

            if (rect.right <= monitor_rect.left) x = monitor_rect.left - rect.right;
            else if (monitor_rect.right <= rect.left) x = rect.left - monitor_rect.right;
            else x = 0;
            if (rect.bottom <= monitor_rect.top) y = monitor_rect.top - rect.bottom;
            else if (monitor_rect.bottom <= rect.top) y = rect.top - monitor_rect.bottom;
            else y = 0;
            distance = x * x + y * y;
            if (distance < min_distance)
            {
                min_distance = distance;
                nearest = monitor;
            }
        }

        if (!found && (flags & MONITOR_DEFAULTTOPRIMARY))
        {
            if (is_monitor_primary( monitor )) primary = monitor;
        }
    }

    if (found) return found;
    if (primary) return primary;
    return nearest;
}

/* display_lock must be held */
static struct monitor *get_monitor_from_handle( HMONITOR handle )
{
    struct monitor *monitor;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (monitor->handle != handle) continue;
        if (!is_monitor_active( monitor )) continue;
        return monitor;
    }

    return NULL;
}

/* display_lock must be held */
static RECT monitors_get_union_rect( UINT dpi, MONITOR_DPI_TYPE type )
{
    struct monitor *monitor;
    RECT rect = {0};

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        RECT monitor_rect;
        if (!is_monitor_active( monitor ) || monitor->is_clone) continue;
        monitor_rect = monitor_get_rect( monitor, dpi, type );
        union_rect( &rect, &rect, &monitor_rect );
    }

    return rect;
}

static RECT map_monitor_rect( struct monitor *monitor, RECT rect, UINT dpi_from, MONITOR_DPI_TYPE type_from,
                              UINT dpi_to, MONITOR_DPI_TYPE type_to )
{
    UINT x, y;

    assert( type_from != type_to );

    if (monitor->source)
    {
        DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)}, *mode_from, *mode_to;
        UINT num, den, dpi;

        source_get_current_settings( monitor->source, &current_mode );

        dpi = monitor_get_dpi( monitor, MDT_DEFAULT, &x, &y );
        if (!dpi_from) dpi_from = dpi;
        if (!dpi_to) dpi_to = dpi;

        if (type_from == MDT_RAW_DPI)
        {
            monitor_virt_to_raw_ratio( monitor, &den, &num );
            mode_from = &monitor->source->physical;
            mode_to = &current_mode;
        }
        else
        {
            monitor_virt_to_raw_ratio( monitor, &num, &den );
            mode_from = &current_mode;
            mode_to = &monitor->source->physical;
        }

        rect = map_dpi_rect( rect, dpi_from, dpi * 2 );
        OffsetRect( &rect, -mode_from->dmPosition.x * 2 - mode_from->dmPelsWidth,
                    -mode_from->dmPosition.y * 2 - mode_from->dmPelsHeight );
        rect = map_dpi_rect( rect, den, num );
        OffsetRect( &rect, mode_to->dmPosition.x * 2 + mode_to->dmPelsWidth,
                    mode_to->dmPosition.y * 2 + mode_to->dmPelsHeight );
        return map_dpi_rect( rect, dpi * 2, dpi_to );
    }

    if (!dpi_from) dpi_from = monitor_get_dpi( monitor, type_from, &x, &y );
    if (!dpi_to) dpi_to = monitor_get_dpi( monitor, type_to, &x, &y );
    return map_dpi_rect( rect, dpi_from, dpi_to );
}

/* map a monitor rect from MDT_RAW_DPI to MDT_DEFAULT coordinates */
RECT map_rect_raw_to_virt( RECT rect, UINT dpi_to )
{
    RECT pos = {rect.left, rect.top, rect.left, rect.top};
    struct monitor *monitor;

    if (!lock_display_devices( FALSE )) return rect;
    if ((monitor = get_monitor_from_rect( pos, MONITOR_DEFAULTTONEAREST, 0, MDT_RAW_DPI )))
        rect = map_monitor_rect( monitor, rect, 0, MDT_RAW_DPI, dpi_to, MDT_DEFAULT );
    unlock_display_devices();

    return rect;
}

/* map a monitor rect from MDT_DEFAULT to MDT_RAW_DPI coordinates */
RECT map_rect_virt_to_raw( RECT rect, UINT dpi_from )
{
    RECT pos = {rect.left, rect.top, rect.left, rect.top};
    struct monitor *monitor;

    if (!lock_display_devices( FALSE )) return rect;
    if ((monitor = get_monitor_from_rect( pos, MONITOR_DEFAULTTONEAREST, dpi_from, MDT_DEFAULT )))
        rect = map_monitor_rect( monitor, rect, dpi_from, MDT_DEFAULT, 0, MDT_RAW_DPI );
    unlock_display_devices();

    return rect;
}

/* map (absolute) window rects from MDT_DEFAULT to MDT_RAW_DPI coordinates */
struct window_rects map_window_rects_virt_to_raw( struct window_rects rects, UINT dpi_from )
{
    struct monitor *monitor;
    RECT rect, monitor_rect;
    BOOL is_fullscreen;

    if (!lock_display_devices( FALSE )) return rects;
    if ((monitor = get_monitor_from_rect( rects.window, MONITOR_DEFAULTTONEAREST, dpi_from, MDT_DEFAULT )))
    {
        /* if the visible rect is fullscreen, make it cover the full raw monitor, regardless of aspect ratio */
        monitor_rect = monitor_get_rect( monitor, dpi_from, MDT_DEFAULT );

        is_fullscreen = intersect_rect( &rect, &monitor_rect, &rects.visible ) && EqualRect( &rect, &monitor_rect );
        if (is_fullscreen) rects.visible = monitor_get_rect( monitor, 0, MDT_RAW_DPI );
        else rects.visible = map_monitor_rect( monitor, rects.visible, dpi_from, MDT_DEFAULT, 0, MDT_RAW_DPI );

        rects.window = map_monitor_rect( monitor, rects.window, dpi_from, MDT_DEFAULT, 0, MDT_RAW_DPI );
        rects.client = map_monitor_rect( monitor, rects.client, dpi_from, MDT_DEFAULT, 0, MDT_RAW_DPI );
    }
    unlock_display_devices();

    return rects;
}

static UINT get_monitor_dpi( HMONITOR handle, UINT type, UINT *x, UINT *y )
{
    struct monitor *monitor;
    UINT dpi = system_dpi;

    if (!lock_display_devices( FALSE )) return 0;
    if ((monitor = get_monitor_from_handle( handle ))) dpi = monitor_get_dpi( monitor, type, x, y );
    unlock_display_devices();

    return dpi;
}

/**********************************************************************
 *              get_win_monitor_dpi
 */
UINT get_win_monitor_dpi( HWND hwnd, UINT *raw_dpi )
{
    UINT dpi = NTUSER_DPI_CONTEXT_GET_DPI( get_window_dpi_awareness_context( hwnd ) );
    HWND parent = get_parent( hwnd );
    RECT rect = {0};
    WND *win;

    if (!(win = get_win_ptr( hwnd )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (win == WND_DESKTOP) return monitor_dpi_from_rect( rect, get_thread_dpi(), raw_dpi );
    if (win == WND_OTHER_PROCESS)
    {
        if (!get_window_rect( hwnd, &rect, dpi )) return 0;
    }
    /* avoid recursive calls from get_window_rects for the process windows */
    else if ((parent = win->parent) && parent != get_desktop_window())
    {
        release_win_ptr( win );
        return get_win_monitor_dpi( parent, raw_dpi );
    }
    else
    {
        rect = is_iconic( hwnd ) ? win->normal_rect : win->rects.window;
        release_win_ptr( win );
    }

    return monitor_dpi_from_rect( rect, dpi, raw_dpi );
}

/* keep in sync with user32 */
static BOOL is_valid_dpi_awareness_context( UINT context, UINT dpi )
{
    switch (NTUSER_DPI_CONTEXT_GET_AWARENESS( context ))
    {
    case DPI_AWARENESS_UNAWARE:
        if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & ~NTUSER_DPI_CONTEXT_FLAG_VALID_MASK) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_VERSION( context ) != 1) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_DPI( context ) != USER_DEFAULT_SCREEN_DPI) return FALSE;
        return TRUE;

    case DPI_AWARENESS_SYSTEM_AWARE:
        if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & ~NTUSER_DPI_CONTEXT_FLAG_VALID_MASK) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & NTUSER_DPI_CONTEXT_FLAG_GDISCALED) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_VERSION( context ) != 1) return FALSE;
        if (dpi && NTUSER_DPI_CONTEXT_GET_DPI( context ) != dpi) return FALSE;
        return TRUE;

    case DPI_AWARENESS_PER_MONITOR_AWARE:
        if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & ~NTUSER_DPI_CONTEXT_FLAG_VALID_MASK) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & NTUSER_DPI_CONTEXT_FLAG_GDISCALED) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_VERSION( context ) != 1 && NTUSER_DPI_CONTEXT_GET_VERSION( context ) != 2) return FALSE;
        if (NTUSER_DPI_CONTEXT_GET_DPI( context )) return FALSE;
        return TRUE;
    }

    return FALSE;
}

UINT get_thread_dpi_awareness_context(void)
{
    struct ntuser_thread_info *info = NtUserGetThreadInfo();
    UINT context;

    if (!(context = info->dpi_context)) context = ReadNoFence( &dpi_context );
    return context ? context : NTUSER_DPI_UNAWARE;
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
    switch (NTUSER_DPI_CONTEXT_GET_AWARENESS( get_thread_dpi_awareness_context() ))
    {
    case DPI_AWARENESS_UNAWARE:      return USER_DEFAULT_SCREEN_DPI;
    case DPI_AWARENESS_SYSTEM_AWARE: return system_dpi;
    default:                         return 0;  /* no scaling */
    }
}

/* see GetDpiForSystem */
UINT get_system_dpi(void)
{
    DPI_AWARENESS awareness = NTUSER_DPI_CONTEXT_GET_AWARENESS( get_thread_dpi_awareness_context() );
    if (awareness == DPI_AWARENESS_UNAWARE) return USER_DEFAULT_SCREEN_DPI;
    return system_dpi;
}

/* keep in sync with user32 */
UINT set_thread_dpi_awareness_context( UINT context )
{
    struct ntuser_thread_info *info = NtUserGetThreadInfo();
    UINT prev;

    if (!is_valid_dpi_awareness_context( context, system_dpi ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(prev = info->dpi_context)) prev = NtUserGetProcessDpiAwarenessContext( GetCurrentProcess() ) | NTUSER_DPI_CONTEXT_FLAG_PROCESS;
    if (NTUSER_DPI_CONTEXT_GET_FLAGS( context ) & NTUSER_DPI_CONTEXT_FLAG_PROCESS) info->dpi_context = 0;
    else info->dpi_context = context;

    return prev;
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
 *              map_dpi_region
 */
HRGN map_dpi_region( HRGN hrgn, UINT dpi_from, UINT dpi_to )
{
    RGNDATA *data;
    UINT i, size;

    if (!(size = NtGdiGetRegionData( hrgn, 0, NULL ))) return 0;
    if (!(data = malloc( size ))) return 0;
    NtGdiGetRegionData( hrgn, size, data );

    if (dpi_from && dpi_to && dpi_from != dpi_to)
    {
        RECT *rects = (RECT *)data->Buffer;
        for (i = 0; i < data->rdh.nCount; i++) rects[i] = map_dpi_rect( rects[i], dpi_from, dpi_to );
    }

    hrgn = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
    free( data );
    return hrgn;
}

/**********************************************************************
 *              map_dpi_window_rects
 */
struct window_rects map_dpi_window_rects( struct window_rects rects, UINT dpi_from, UINT dpi_to )
{
    rects.window = map_dpi_rect( rects.window, dpi_from, dpi_to );
    rects.client = map_dpi_rect( rects.client, dpi_from, dpi_to );
    rects.visible = map_dpi_rect( rects.visible, dpi_from, dpi_to );
    return rects;
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
    UINT raw_dpi, dpi = get_win_monitor_dpi( hwnd, &raw_dpi );
    return map_dpi_point( pt, get_dpi_for_window( hwnd ), dpi );
}

/**********************************************************************
 *              point_phys_to_win_dpi
 */
POINT point_phys_to_win_dpi( HWND hwnd, POINT pt )
{
    UINT raw_dpi, dpi = get_win_monitor_dpi( hwnd, &raw_dpi );
    return map_dpi_point( pt, dpi, get_dpi_for_window( hwnd ) );
}

/**********************************************************************
 *              point_thread_to_win_dpi
 */
POINT point_thread_to_win_dpi( HWND hwnd, POINT pt )
{
    UINT dpi = get_thread_dpi(), raw_dpi;
    if (!dpi) dpi = get_win_monitor_dpi( hwnd, &raw_dpi );
    return map_dpi_point( pt, dpi, get_dpi_for_window( hwnd ));
}

/**********************************************************************
 *              rect_thread_to_win_dpi
 */
RECT rect_thread_to_win_dpi( HWND hwnd, RECT rect )
{
    UINT dpi = get_thread_dpi(), raw_dpi;
    if (!dpi) dpi = get_win_monitor_dpi( hwnd, &raw_dpi );
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

RECT get_virtual_screen_rect( UINT dpi, MONITOR_DPI_TYPE type )
{
    RECT rect = {0};

    if (!lock_display_devices( FALSE )) return rect;
    rect = monitors_get_union_rect( dpi, type );
    unlock_display_devices();

    return rect;
}

BOOL is_window_rect_full_screen( const RECT *rect, UINT dpi )
{
    struct monitor *monitor;
    BOOL ret = FALSE;

    if (!lock_display_devices( FALSE )) return FALSE;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        RECT monrect;

        if (!is_monitor_active( monitor ) || monitor->is_clone) continue;

        monrect = monitor_get_rect( monitor, dpi, MDT_DEFAULT );
        if (rect->left <= monrect.left && rect->right >= monrect.right &&
            rect->top <= monrect.top && rect->bottom >= monrect.bottom)
        {
            ret = TRUE;
            break;
        }
    }

    unlock_display_devices();
    return ret;
}

static UINT get_display_index( const UNICODE_STRING *name )
{
    static const WCHAR displayW[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y'};
    WCHAR *end, display[64] = {0};
    int index;

    memcpy( display, name->Buffer, min( name->Length, 63 * sizeof(WCHAR) ) );
    if (wcsnicmp( display, displayW, ARRAY_SIZE(displayW) )) return 0;
    if (!(index = wcstoul( display + ARRAY_SIZE(displayW), &end, 10 )) || *end) return 0;
    return index;
}

RECT get_display_rect( const WCHAR *display )
{
    struct monitor *monitor;
    UNICODE_STRING name;
    RECT rect = {0};
    UINT index, dpi = get_thread_dpi();

    RtlInitUnicodeString( &name, display );
    if (!(index = get_display_index( &name ))) return rect;
    if (!lock_display_devices( FALSE )) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!monitor->source || monitor->source->id + 1 != index) continue;
        rect = monitor_get_rect( monitor, dpi, MDT_DEFAULT );
        break;
    }

    unlock_display_devices();
    return rect;
}

RECT get_primary_monitor_rect( UINT dpi )
{
    struct monitor *monitor;
    RECT rect = {0};

    if (!lock_display_devices( FALSE )) return rect;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!is_monitor_primary( monitor )) continue;
        rect = monitor_get_rect( monitor, dpi, MDT_DEFAULT );
        break;
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

    switch (flags & qdc_retrieve_flags_mask)
    {
    case QDC_ALL_PATHS:
    case QDC_ONLY_ACTIVE_PATHS:
    case QDC_DATABASE_CURRENT:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    if ((flags & ~(qdc_retrieve_flags_mask | QDC_VIRTUAL_MODE_AWARE)))
    {
        FIXME( "unsupported flags %#x.\n", flags );
        return ERROR_INVALID_PARAMETER;
    }

    /* FIXME: semi-stub */
    if ((flags & qdc_retrieve_flags_mask) != QDC_ONLY_ACTIVE_PATHS)
        FIXME( "only returning active paths\n" );

    if (lock_display_devices( FALSE ))
    {
        LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
        {
            if (!is_monitor_active( monitor )) continue;
            count++;
        }
        unlock_display_devices();
    }

    *num_path_info = count;
    *num_mode_info = count * 2;
    if (flags & QDC_VIRTUAL_MODE_AWARE)
        *num_mode_info += count;
    TRACE( "returning %u paths %u modes\n", *num_path_info, *num_mode_info );
    return ERROR_SUCCESS;
}

static DISPLAYCONFIG_ROTATION get_dc_rotation( const DEVMODEW *devmode )
{
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        return devmode->dmDisplayOrientation + 1;
    else
        return DISPLAYCONFIG_ROTATION_IDENTITY;
}

static DISPLAYCONFIG_SCANLINE_ORDERING get_dc_scanline_ordering( const DEVMODEW *devmode )
{
    if (!(devmode->dmFields & DM_DISPLAYFLAGS))
        return DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED;
    else if (devmode->dmDisplayFlags & DM_INTERLACED)
        return DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED;
    else
        return DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
}

static DISPLAYCONFIG_PIXELFORMAT get_dc_pixelformat( DWORD dmBitsPerPel )
{
    if ((dmBitsPerPel == 8) || (dmBitsPerPel == 16) ||
        (dmBitsPerPel == 24) || (dmBitsPerPel == 32))
        return dmBitsPerPel / 8;
    else
        return DISPLAYCONFIG_PIXELFORMAT_NONGDI;
}

static void set_mode_target_info( DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid, UINT32 target_id,
                                  UINT32 flags, const DEVMODEW *devmode )
{
    DISPLAYCONFIG_TARGET_MODE *mode = &info->targetMode;

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_TARGET;
    info->adapterId = *gpu_luid;
    info->id = target_id;

    /* FIXME: Populate pixelRate/hSyncFreq/totalSize with real data */
    mode->targetVideoSignalInfo.pixelRate = devmode->dmDisplayFrequency * devmode->dmPelsWidth * devmode->dmPelsHeight;
    mode->targetVideoSignalInfo.hSyncFreq.Numerator = devmode->dmDisplayFrequency * devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.hSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.vSyncFreq.Numerator = devmode->dmDisplayFrequency;
    mode->targetVideoSignalInfo.vSyncFreq.Denominator = 1;
    mode->targetVideoSignalInfo.activeSize.cx = devmode->dmPelsWidth;
    mode->targetVideoSignalInfo.activeSize.cy = devmode->dmPelsHeight;
    if (flags & QDC_DATABASE_CURRENT)
    {
        mode->targetVideoSignalInfo.totalSize.cx = 0;
        mode->targetVideoSignalInfo.totalSize.cy = 0;
    }
    else
    {
        mode->targetVideoSignalInfo.totalSize.cx = devmode->dmPelsWidth;
        mode->targetVideoSignalInfo.totalSize.cy = devmode->dmPelsHeight;
    }
    mode->targetVideoSignalInfo.videoStandard = D3DKMDT_VSS_OTHER;
    mode->targetVideoSignalInfo.scanLineOrdering = get_dc_scanline_ordering( devmode );
}

static void set_path_target_info( DISPLAYCONFIG_PATH_TARGET_INFO *info, const LUID *gpu_luid,
                                  UINT32 target_id, UINT32 mode_index, UINT32 desktop_mode_index,
                                  UINT32 flags, const DEVMODEW *devmode )
{
    info->adapterId = *gpu_luid;
    info->id = target_id;
    if (flags & QDC_VIRTUAL_MODE_AWARE)
    {
        info->targetModeInfoIdx = mode_index;
        info->desktopModeInfoIdx = desktop_mode_index;
    }
    else info->modeInfoIdx = mode_index;
    info->outputTechnology = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL;
    info->rotation = get_dc_rotation( devmode );
    info->scaling = DISPLAYCONFIG_SCALING_IDENTITY;
    info->refreshRate.Numerator = devmode->dmDisplayFrequency;
    info->refreshRate.Denominator = 1;
    info->scanLineOrdering = get_dc_scanline_ordering( devmode );
    info->targetAvailable = TRUE;
    info->statusFlags = DISPLAYCONFIG_TARGET_IN_USE;
}

static void set_mode_source_info( DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid,
                                  UINT32 source_id, const DEVMODEW *devmode )
{
    DISPLAYCONFIG_SOURCE_MODE *mode = &info->sourceMode;

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE;
    info->adapterId = *gpu_luid;
    info->id = source_id;

    mode->width = devmode->dmPelsWidth;
    mode->height = devmode->dmPelsHeight;
    mode->pixelFormat = get_dc_pixelformat( devmode->dmBitsPerPel );
    if (devmode->dmFields & DM_POSITION)
    {
        mode->position = devmode->dmPosition;
    }
    else
    {
        mode->position.x = 0;
        mode->position.y = 0;
    }
}

static void set_mode_desktop_info( DISPLAYCONFIG_MODE_INFO *info, const LUID *gpu_luid, UINT32 target_id,
                                   const DISPLAYCONFIG_SOURCE_MODE *source_mode )
{
    DISPLAYCONFIG_DESKTOP_IMAGE_INFO *mode = &info->desktopImageInfo;

    info->infoType = DISPLAYCONFIG_MODE_INFO_TYPE_DESKTOP_IMAGE;
    info->adapterId = *gpu_luid;
    info->id = target_id;
    mode->PathSourceSize.x = source_mode->width;
    mode->PathSourceSize.y = source_mode->height;
    mode->DesktopImageRegion.left = 0;
    mode->DesktopImageRegion.top = 0;
    mode->DesktopImageRegion.right = source_mode->width;
    mode->DesktopImageRegion.bottom = source_mode->height;
    mode->DesktopImageClip = mode->DesktopImageRegion;
}

static void set_path_source_info( DISPLAYCONFIG_PATH_SOURCE_INFO *info, const LUID *gpu_luid,
                                  UINT32 source_id, UINT32 mode_index, UINT32 flags )
{
    info->adapterId = *gpu_luid;
    info->id = source_id;
    if (flags & QDC_VIRTUAL_MODE_AWARE)
    {
        info->sourceModeInfoIdx = mode_index;
        info->cloneGroupId = DISPLAYCONFIG_PATH_CLONE_GROUP_INVALID;
    }
    else info->modeInfoIdx = mode_index;
    info->statusFlags = DISPLAYCONFIG_SOURCE_IN_USE;
}

static BOOL source_mode_exists( const DISPLAYCONFIG_MODE_INFO *modes, UINT32 modes_count,
                                UINT32 source_id, UINT32 *found_mode_index )
{
    UINT32 i;

    for (i = 0; i < modes_count; i++)
    {
        if (modes[i].infoType == DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE &&
            modes[i].id == source_id)
        {
            *found_mode_index = i;
            return TRUE;
        }
    }
    return FALSE;
}

/***********************************************************************
 *              NtUserQueryDisplayConfig (win32u.@)
 */
LONG WINAPI NtUserQueryDisplayConfig( UINT32 flags, UINT32 *paths_count, DISPLAYCONFIG_PATH_INFO *paths,
                                      UINT32 *modes_count, DISPLAYCONFIG_MODE_INFO *modes,
                                      DISPLAYCONFIG_TOPOLOGY_ID *topology_id )
{
    ULONG source_index;
    LONG ret;
    UINT32 output_id, source_mode_index, path_index = 0, mode_index = 0;
    const LUID *gpu_luid;
    DEVMODEW devmode;
    struct monitor *monitor;
    DWORD retrieve_flags = flags & qdc_retrieve_flags_mask;

    TRACE( "flags %#x, paths_count %p, paths %p, modes_count %p, modes %p, topology_id %p.\n",
           flags, paths_count, paths, modes_count, modes, topology_id );

    if (!paths_count || !modes_count)
        return ERROR_INVALID_PARAMETER;

    if (!*paths_count || !*modes_count)
        return ERROR_INVALID_PARAMETER;

    if (retrieve_flags != QDC_ALL_PATHS &&
        retrieve_flags != QDC_ONLY_ACTIVE_PATHS &&
        retrieve_flags != QDC_DATABASE_CURRENT)
        return ERROR_INVALID_PARAMETER;

    if (((retrieve_flags == QDC_DATABASE_CURRENT) && !topology_id) ||
        ((retrieve_flags != QDC_DATABASE_CURRENT) && topology_id))
        return ERROR_INVALID_PARAMETER;

    if ((flags & ~(qdc_retrieve_flags_mask | QDC_VIRTUAL_MODE_AWARE)))
    {
        FIXME( "unsupported flags %#x.\n", flags );
        return ERROR_INVALID_PARAMETER;
    }

    if (retrieve_flags != QDC_ONLY_ACTIVE_PATHS)
        FIXME( "only returning active paths\n" );

    if (topology_id)
    {
        FIXME( "setting toplogyid to DISPLAYCONFIG_TOPOLOGY_INTERNAL\n" );
        *topology_id = DISPLAYCONFIG_TOPOLOGY_INTERNAL;
    }

    if (!lock_display_devices( FALSE ))
        return ERROR_GEN_FAILURE;

    ret = ERROR_GEN_FAILURE;

    LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
    {
        if (!is_monitor_active( monitor )) continue;
        if (!monitor->source) continue;

        source_index = monitor->source->id;
        gpu_luid = &monitor->source->gpu->luid;
        output_id = monitor->output_id;

        memset( &devmode, 0, sizeof(devmode) );
        devmode.dmSize = sizeof(devmode);
        if (!source_get_current_settings( monitor->source, &devmode ))
        {
            goto done;
        }

        if (path_index == *paths_count || mode_index == *modes_count)
        {
            ret = ERROR_INSUFFICIENT_BUFFER;
            goto done;
        }

        /* Multiple targets can be driven by the same source, ensure a mode
         * hasn't already been added for this source.
         */
        if (!source_mode_exists( modes, mode_index, source_index, &source_mode_index ))
        {
            set_mode_source_info( &modes[mode_index], gpu_luid, source_index, &devmode );
            source_mode_index = mode_index;
            if (++mode_index == *modes_count)
            {
                ret = ERROR_INSUFFICIENT_BUFFER;
                goto done;
            }
        }

        paths[path_index].flags = DISPLAYCONFIG_PATH_ACTIVE;
        set_mode_target_info( &modes[mode_index], gpu_luid, output_id, flags, &devmode );
        if (flags & QDC_VIRTUAL_MODE_AWARE)
        {
            paths[path_index].flags |= DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE;
            if (++mode_index == *modes_count)
            {
                ret = ERROR_INSUFFICIENT_BUFFER;
                goto done;
            }
            set_mode_desktop_info( &modes[mode_index], gpu_luid, output_id, &modes[source_mode_index].sourceMode );
            set_path_target_info( &paths[path_index].targetInfo, gpu_luid, output_id, mode_index - 1, mode_index,
                                  flags, &devmode );
        }
        else set_path_target_info( &paths[path_index].targetInfo, gpu_luid, output_id, mode_index, ~0u, flags, &devmode );
        ++mode_index;

        set_path_source_info( &paths[path_index].sourceInfo, gpu_luid, source_index, source_mode_index, flags );
        path_index++;
    }

    *paths_count = path_index;
    *modes_count = mode_index;
    ret = ERROR_SUCCESS;

done:
    unlock_display_devices();
    return ret;
}

/* display_lock mutex must be held */
static struct monitor *find_monitor_by_index( struct source *source, UINT index )
{
    struct monitor *monitor;

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
        if (monitor->source == source && index == monitor->id)
            return monitor;

    WARN( "Failed to find source %u monitor with id %u.\n", source->id, index );
    return NULL;
}

/* display_lock mutex must be held */
static struct source *find_source_by_index( UINT index )
{
    struct source *source;

    LIST_FOR_EACH_ENTRY(source, &sources, struct source, entry)
        if (index == source->id) return source;

    WARN( "Failed to find source with id %u.\n", index );
    return NULL;
}

/* display_lock mutex must be held */
static struct source *find_primary_source(void)
{
    struct source *source;

    LIST_FOR_EACH_ENTRY(source, &sources, struct source, entry)
        if (source->state_flags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            return source;

    WARN( "Failed to find primary source.\n" );
    return NULL;
}

/* display_lock mutex must be held */
static struct source *find_source_by_name( UNICODE_STRING *name )
{
    struct source *source;
    UINT index;

    if (!(index = get_display_index( name ))) return NULL;

    LIST_FOR_EACH_ENTRY(source, &sources, struct source, entry)
        if (source->id + 1 == index)
            return source;

    WARN( "Failed to find source with name %s.\n", debugstr_us(name) );
    return NULL;
}

/* Find and acquire the source matching name, or primary source if name is NULL.
 * If not NULL, the returned source needs to be released with source_release.
 */
static struct source *find_source( UNICODE_STRING *name )
{
    struct source *source;

    if (!lock_display_devices( FALSE )) return NULL;

    if (name && name->Length) source = find_source_by_name( name );
    else source = find_primary_source();

    if (source) source = source_acquire( source );

    unlock_display_devices();
    return source;
}

static void monitor_get_interface_name( struct monitor *monitor, WCHAR *interface_name )
{
    char buffer[MAX_PATH] = {0}, *tmp;
    const char *id;

    *interface_name = 0;
    if (!monitor->source) return;

    if (!(monitor->edid_info.flags & MONITOR_INFO_HAS_MONITOR_ID)) id = "Default_Monitor";
    else id = monitor->edid_info.monitor_id_string;

    snprintf( buffer, sizeof(buffer), "\\\\?\\DISPLAY\\%s\\%04X&%04X#%s", id, monitor->source->id,
              monitor->id, guid_devinterface_monitorA );
    for (tmp = buffer + 4; *tmp; tmp++) if (*tmp == '\\') *tmp = '#';

    asciiz_to_unicode( interface_name, buffer );
}

/* see D3DKMTOpenAdapterFromGdiDisplayName */
static NTSTATUS d3dkmt_open_adapter_from_gdi_display_name( D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *desc )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    D3DKMT_OPENADAPTERFROMLUID luid_desc;
    struct source *source;
    UNICODE_STRING name;

    TRACE( "desc %p, name %s\n", desc, debugstr_w( desc->DeviceName ) );

    RtlInitUnicodeString( &name, desc->DeviceName );
    if (!name.Length) return STATUS_UNSUCCESSFUL;
    if (!(source = find_source( &name ))) return STATUS_UNSUCCESSFUL;

    luid_desc.AdapterLuid = source->gpu->luid;
    if ((source->state_flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
        !(status = NtGdiDdDDIOpenAdapterFromLuid( &luid_desc )))
    {
        desc->hAdapter = luid_desc.hAdapter;
        desc->AdapterLuid = luid_desc.AdapterLuid;
        desc->VidPnSourceId = source->id + 1;
    }

    source_release( source );
    return status;
}


/***********************************************************************
 *	     NtUserEnumDisplayDevices    (win32u.@)
 */
NTSTATUS WINAPI NtUserEnumDisplayDevices( UNICODE_STRING *device, DWORD index,
                                          DISPLAY_DEVICEW *info, DWORD flags )
{
    struct monitor *monitor = NULL;
    struct source *source = NULL;
    BOOL found = FALSE;

    TRACE( "%s %u %p %#x\n", debugstr_us( device ), (int)index, info, (int)flags );

    if (!info || !info->cb) return STATUS_UNSUCCESSFUL;

    if (!lock_display_devices( FALSE )) return STATUS_UNSUCCESSFUL;

    if (!device || !device->Length)
    {
        if ((source = find_source_by_index( index ))) found = TRUE;
    }
    else if ((source = find_source_by_name( device )))
    {
        if ((monitor = find_monitor_by_index( source, index ))) found = TRUE;
    }

    if (found)
    {
        char buffer[MAX_PATH], *tmp;

        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceName) + sizeof(info->DeviceName))
        {
            if (monitor) snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d\\Monitor%d", source->id + 1, monitor->id );
            else snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d", source->id + 1 );
            asciiz_to_unicode( info->DeviceName, buffer );
        }
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceString) + sizeof(info->DeviceString))
        {
            if (monitor) asciiz_to_unicode( info->DeviceString, "Generic Non-PnP Monitor" );
            else lstrcpynW( info->DeviceString, source->gpu->name, ARRAY_SIZE(info->DeviceString) );
        }
        if (info->cb >= offsetof(DISPLAY_DEVICEW, StateFlags) + sizeof(info->StateFlags))
        {
            if (!monitor) info->StateFlags = source->state_flags;
            else
            {
                info->StateFlags = DISPLAY_DEVICE_ATTACHED;
                if (is_monitor_active( monitor )) info->StateFlags |= DISPLAY_DEVICE_ACTIVE;
            }
        }
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(info->DeviceID))
        {
            if (flags & EDD_GET_DEVICE_INTERFACE_NAME)
            {
                if (monitor) monitor_get_interface_name( monitor, info->DeviceID );
                else *info->DeviceID = 0;
            }
            else
            {
                if (monitor)
                {
                    snprintf( buffer, sizeof(buffer), "MONITOR\\%s", monitor->path + 8 );
                    if (!(tmp = strrchr( buffer, '\\' ))) tmp = buffer + strlen( buffer );
                    snprintf( tmp, sizeof(buffer) - (tmp - buffer), "\\%s\\%04X", guid_devclass_monitorA, monitor->output_id );
                }
                else
                {
                    strcpy( buffer, source->gpu->path );
                    if ((tmp = strrchr( buffer, '\\' ))) *tmp = 0;
                }
                asciiz_to_unicode( info->DeviceID, buffer );
            }
        }
        if (info->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(info->DeviceKey))
        {
            if (monitor) snprintf( buffer, sizeof(buffer), "%s\\Class\\%s\\%04X", control_keyA, guid_devclass_monitorA, monitor->output_id );
            else snprintf( buffer, sizeof(buffer), "%s\\Video\\%s\\%04x", control_keyA, source->gpu->guid, source->id );
            asciiz_to_unicode( info->DeviceKey, buffer );
        }
    }
    unlock_display_devices();
    return found ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

#define _X_FIELD(prefix, bits)                              \
    if ((fields) & prefix##_##bits)                         \
    {                                                       \
        p += snprintf( p, sizeof(buf) - (p - buf), "%s%s", first ? "" : ",", #bits ); \
        first = FALSE;                                      \
    }

static const char *_CDS_flags( DWORD fields )
{
    BOOL first = TRUE;
    CHAR buf[130];
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
        TRACE( "dmBitsPerPel=%u ", (int)devmode->dmBitsPerPel );
    if (devmode->dmFields & DM_PELSWIDTH)
        TRACE( "dmPelsWidth=%u ", (int)devmode->dmPelsWidth );
    if (devmode->dmFields & DM_PELSHEIGHT)
        TRACE( "dmPelsHeight=%u ", (int)devmode->dmPelsHeight );
    if (devmode->dmFields & DM_DISPLAYFREQUENCY)
        TRACE( "dmDisplayFrequency=%u ", (int)devmode->dmDisplayFrequency );
    if (devmode->dmFields & DM_POSITION)
        TRACE( "dmPosition=(%d,%d) ", (int)devmode->dmPosition.x, (int)devmode->dmPosition.y );
    if (devmode->dmFields & DM_DISPLAYFLAGS)
        TRACE( "dmDisplayFlags=%#x ", (int)devmode->dmDisplayFlags );
    if (devmode->dmFields & DM_DISPLAYORIENTATION)
        TRACE( "dmDisplayOrientation=%u ", (int)devmode->dmDisplayOrientation );
    TRACE("\n");
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

static BOOL source_get_full_mode( const struct source *source, const DEVMODEW *devmode, DEVMODEW *full_mode )
{
    const DEVMODEW *source_mode;

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
        if (!source_get_registry_settings( source, full_mode )) return FALSE;
        TRACE( "Return to original display mode\n" );
    }

    if ((full_mode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
    {
        WARN( "devmode doesn't specify the resolution: %#x\n", (int)full_mode->dmFields );
        return FALSE;
    }

    if (!is_detached_mode( full_mode ) && (!full_mode->dmPelsWidth || !full_mode->dmPelsHeight || !(full_mode->dmFields & DM_POSITION)))
    {
        DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};

        if (!source_get_current_settings( source, &current_mode )) return FALSE;
        if (!full_mode->dmPelsWidth) full_mode->dmPelsWidth = current_mode.dmPelsWidth;
        if (!full_mode->dmPelsHeight) full_mode->dmPelsHeight = current_mode.dmPelsHeight;
        if (!(full_mode->dmFields & DM_POSITION))
        {
            full_mode->dmPosition = current_mode.dmPosition;
            full_mode->dmFields |= DM_POSITION;
        }
    }

    if ((source_mode = find_display_mode( source->modes, full_mode )) && source_mode != full_mode)
    {
        POINTL position = full_mode->dmPosition;
        *full_mode = *source_mode;
        full_mode->dmFields |= DM_POSITION;
        full_mode->dmPosition = position;
    }

    return source_mode != NULL;
}

static DEVMODEW *get_display_settings( struct source *target, const DEVMODEW *devmode )
{
    DEVMODEW *mode, *displays;
    struct source *source;
    BOOL ret;

    /* allocate an extra mode for easier iteration */
    if (!(displays = calloc( list_count( &sources ) + 1, sizeof(DEVMODEW) ))) return NULL;
    mode = displays;

    LIST_FOR_EACH_ENTRY( source, &sources, struct source, entry )
    {
        char buffer[CCHDEVICENAME];

        mode->dmSize = sizeof(DEVMODEW);
        if (devmode && source->id == target->id)
            memcpy( &mode->dmFields, &devmode->dmFields, devmode->dmSize - offsetof(DEVMODEW, dmFields) );
        else
        {
            if (!target) ret = source_get_registry_settings( source, mode );
            else ret = source_get_current_settings( source, mode );
            if (!ret)
            {
                free( displays );
                return NULL;
            }
        }

        snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d", source->id + 1 );
        asciiz_to_unicode( mode->dmDeviceName, buffer );
        mode = NEXT_DEVMODEW(mode);
    }

    return displays;
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

static void place_all_displays( DEVMODEW *displays, const WCHAR *primary_name )
{
    POINT min_offset, offset = {0};
    DEVMODEW *mode, *placing;

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        if (wcsicmp( mode->dmDeviceName, primary_name )) continue;
        offset.x = -mode->dmPosition.x;
        offset.y = -mode->dmPosition.y;
        break;
    }

    for (mode = displays; mode->dmSize; mode = NEXT_DEVMODEW(mode))
    {
        mode->dmPosition.x += offset.x;
        mode->dmPosition.y += offset.y;
        mode->dmFields &= ~DM_POSITION;
    }

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

static BOOL get_primary_source_mode( DEVMODEW *mode )
{
    struct source *primary;
    BOOL ret;

    if (!(primary = find_source( NULL ))) return FALSE;
    ret = source_get_current_settings( primary, mode );
    source_release( primary );

    return ret;
}

static void display_mode_changed( BOOL broadcast )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};

    if (!update_display_cache( TRUE ))
    {
        ERR( "Failed to update display cache after mode change.\n" );
        return;
    }
    if (!get_primary_source_mode( &current_mode ))
    {
        ERR( "Failed to get primary source current display settings.\n" );
        return;
    }

    if (!broadcast)
        send_message( get_desktop_window(), WM_DISPLAYCHANGE, current_mode.dmBitsPerPel,
                      MAKELPARAM( current_mode.dmPelsWidth, current_mode.dmPelsHeight ) );
    else
    {
        /* broadcast to all the windows as well if an application changed the display settings */
        NtUserClipCursor( NULL );
        send_notify_message( get_desktop_window(), WM_DISPLAYCHANGE, current_mode.dmBitsPerPel,
                             MAKELPARAM( current_mode.dmPelsWidth, current_mode.dmPelsHeight ), FALSE );
        send_message_timeout( HWND_BROADCAST, WM_DISPLAYCHANGE, current_mode.dmBitsPerPel,
                              MAKELPARAM( current_mode.dmPelsWidth, current_mode.dmPelsHeight ),
                              SMTO_ABORTIFHUNG, 2000, FALSE );
        /* post clip_fullscreen_window request to the foreground window */
        NtUserPostMessage( NtUserGetForegroundWindow(), WM_WINE_CLIPCURSOR, SET_CURSOR_FSCLIP, 0 );
    }
}

static LONG apply_display_settings( struct source *target, const DEVMODEW *devmode,
                                    HWND hwnd, DWORD flags, void *lparam )
{
    static const WCHAR restorerW[] = {'_','_','w','i','n','e','_','d','i','s','p','l','a','y','_',
                                      's','e','t','t','i','n','g','s','_','r','e','s','t','o','r','e','r',0};
    UNICODE_STRING restorer_str = RTL_CONSTANT_STRING( restorerW );
    WCHAR primary_name[CCHDEVICENAME];
    struct source *primary, *source;
    DEVMODEW *mode, *displays;
    HWND restorer_window;
    LONG ret;

    if (!lock_display_devices( FALSE )) return DISP_CHANGE_FAILED;
    if (!(displays = get_display_settings( target, devmode )))
    {
        unlock_display_devices();
        return DISP_CHANGE_FAILED;
    }

    if (all_detached_settings( displays ))
    {
        unlock_display_devices();
        WARN( "Detaching all modes is not permitted.\n" );
        free( displays );
        return DISP_CHANGE_SUCCESSFUL;
    }

    if (!(primary = find_primary_source())) primary_name[0] = 0;
    else
    {
        char device_name[CCHDEVICENAME];
        snprintf( device_name, sizeof(device_name), "\\\\.\\DISPLAY%d", primary->id + 1 );
        asciiz_to_unicode( primary_name, device_name );
    }

    place_all_displays( displays, primary_name );

    /* use the default implementation in virtual desktop mode */
    if (is_virtual_desktop() || emulate_modeset) ret = DISP_CHANGE_SUCCESSFUL;
    else ret = user_driver->pChangeDisplaySettings( displays, primary_name, hwnd, flags, lparam );

    if (ret == DISP_CHANGE_SUCCESSFUL)
    {
        mode = displays;
        LIST_FOR_EACH_ENTRY( source, &sources, struct source, entry )
        {
            if (!source_set_current_settings( source, mode ))
                WARN( "Failed to write source %u current mode.\n", source->id );
            mode = NEXT_DEVMODEW(mode);
        }
    }
    unlock_display_devices();

    free( displays );
    if (ret) return ret;

    if ((restorer_window = NtUserFindWindowEx( NULL, NULL, &restorer_str, NULL, 0 )))
    {
        if (NtUserGetWindowThread( restorer_window, NULL ) != GetCurrentThreadId())
        {
            DWORD fullscreen_process_id = (flags & CDS_FULLSCREEN) ? GetCurrentProcessId() : 0;
            send_message( restorer_window, WM_USER + 0, 0, fullscreen_process_id );
        }
    }

    display_mode_changed( TRUE );
    return ret;
}

/***********************************************************************
 *	     NtUserChangeDisplaySettings    (win32u.@)
 */
LONG WINAPI NtUserChangeDisplaySettings( UNICODE_STRING *devname, DEVMODEW *devmode, HWND hwnd,
                                         DWORD flags, void *lparam )
{
    DEVMODEW full_mode = {.dmSize = sizeof(DEVMODEW)};
    int ret = DISP_CHANGE_SUCCESSFUL;
    struct source *source;

    TRACE( "%s %p %p %#x %p\n", debugstr_us(devname), devmode, hwnd, (int)flags, lparam );
    TRACE( "flags=%s\n", _CDS_flags(flags) );

    if ((!devname || !devname->Length) && !devmode) return apply_display_settings( NULL, NULL, hwnd, flags, lparam );

    if (!(source = find_source( devname ))) return DISP_CHANGE_BADPARAM;

    if (!source_get_full_mode( source, devmode, &full_mode )) ret = DISP_CHANGE_BADMODE;
    else if ((flags & CDS_UPDATEREGISTRY) && !source_set_registry_settings( source, &full_mode )) ret = DISP_CHANGE_NOTUPDATED;
    else if (flags & (CDS_TEST | CDS_NORESET)) ret = DISP_CHANGE_SUCCESSFUL;
    else ret = apply_display_settings( source, &full_mode, hwnd, flags, lparam );
    source_release( source );

    if (ret) ERR( "Changing %s display settings returned %d.\n", debugstr_us(devname), ret );
    return ret;
}

static BOOL source_enum_display_settings( const struct source *source, UINT index, DEVMODEW *devmode, UINT flags )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
    const DEVMODEW *source_mode;
    unsigned int i = index;

    if (!(flags & EDS_ROTATEDMODE) && !source_get_current_settings( source, &current_mode ))
    {
        WARN( "Failed to query current display mode for EDS_ROTATEDMODE flag.\n" );
        return FALSE;
    }

    for (source_mode = source->modes; source_mode->dmSize; source_mode = NEXT_DEVMODEW(source_mode))
    {
        if (!(flags & EDS_ROTATEDMODE) && (source_mode->dmFields & DM_DISPLAYORIENTATION) &&
            source_mode->dmDisplayOrientation != current_mode.dmDisplayOrientation)
            continue;
        if (!(flags & EDS_RAWMODE) && (source_mode->dmFields & DM_DISPLAYFLAGS) &&
            (source_mode->dmDisplayFlags & WINE_DM_UNSUPPORTED))
            continue;
        if (!i--)
        {
            memcpy( &devmode->dmFields, &source_mode->dmFields, devmode->dmSize - FIELD_OFFSET(DEVMODEW, dmFields) );
            devmode->dmDisplayFlags &= ~WINE_DM_UNSUPPORTED;
            return TRUE;
        }
    }

    WARN( "device %d, index %#x, flags %#x display mode not found.\n", source->id, index, flags );
    RtlSetLastWin32Error( ERROR_NO_MORE_FILES );
    return FALSE;
}

/***********************************************************************
 *	     NtUserEnumDisplaySettings    (win32u.@)
 */
BOOL WINAPI NtUserEnumDisplaySettings( UNICODE_STRING *device, DWORD index, DEVMODEW *devmode, DWORD flags )
{
    static const WCHAR wine_display_driverW[] = {'W','i','n','e',' ','D','i','s','p','l','a','y',' ','D','r','i','v','e','r',0};
    struct source *source;
    BOOL ret;

    TRACE( "device %s, index %#x, devmode %p, flags %#x\n",
           debugstr_us(device), (int)index, devmode, (int)flags );

    if (!(source = find_source( device ))) return FALSE;

    lstrcpynW( devmode->dmDeviceName, wine_display_driverW, ARRAY_SIZE(devmode->dmDeviceName) );
    devmode->dmSpecVersion = DM_SPECVERSION;
    devmode->dmDriverVersion = DM_SPECVERSION;
    devmode->dmSize = offsetof(DEVMODEW, dmICMMethod);
    devmode->dmDriverExtra = 0;

    if (index == ENUM_REGISTRY_SETTINGS) ret = source_get_registry_settings( source, devmode );
    else if (index == ENUM_CURRENT_SETTINGS) ret = source_get_current_settings( source, devmode );
    else if (index == WINE_ENUM_PHYSICAL_SETTINGS) ret = FALSE;
    else ret = source_enum_display_settings( source, index, devmode, flags );
    source_release( source );

    if (!ret) WARN( "Failed to query %s display settings.\n", debugstr_us(device) );
    else TRACE( "position %dx%d, resolution %ux%u, frequency %u, depth %u, orientation %#x.\n",
                (int)devmode->dmPosition.x, (int)devmode->dmPosition.y, (int)devmode->dmPelsWidth,
                (int)devmode->dmPelsHeight, (int)devmode->dmDisplayFrequency,
                (int)devmode->dmBitsPerPel, (int)devmode->dmDisplayOrientation );
    return ret;
}

struct monitor_enum_info
{
    HANDLE handle;
    RECT rect;
};

static unsigned int active_unique_monitor_count(void)
{
    struct monitor *monitor;
    unsigned int count = 0;

    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        if (is_monitor_active( monitor ) && !monitor->is_clone)
            count++;
    }
    return count;
}

INT get_display_depth( UNICODE_STRING *name )
{
    DEVMODEW current_mode = {.dmSize = sizeof(DEVMODEW)};
    struct source *source;
    INT depth;

    if (!lock_display_devices( FALSE ))
        return 32;

    if (name && name->Length) source = find_source_by_name( name );
    else source = find_primary_source();

    if (!source)
    {
        unlock_display_devices();
        return 32;
    }

    if (!source_get_current_settings( source, &current_mode )) depth = 32;
    else depth = current_mode.dmBitsPerPel;

    unlock_display_devices();
    return depth;
}

static BOOL should_enumerate_monitor( struct monitor *monitor, const POINT *origin,
                                      const RECT *limit, RECT *rect )
{
    if (!is_monitor_active( monitor )) return FALSE;
    if (monitor->is_clone) return FALSE;

    *rect = monitor_get_rect( monitor, get_thread_dpi(), MDT_DEFAULT );
    OffsetRect( rect, -origin->x, -origin->y );
    return intersect_rect( rect, rect, limit );
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

    if (!lock_display_devices( FALSE )) return FALSE;

    count = list_count( &monitors );
    if (!count || (count > ARRAYSIZE(enum_buf) &&
                   !(enum_info = malloc( count * sizeof(*enum_info) ))))
    {
        unlock_display_devices();
        return FALSE;
    }

    count = 0;

    /* enumerate primary monitors first */
    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        if (!is_monitor_primary( monitor )) continue;
        if (should_enumerate_monitor( monitor, &origin, &limit, &enum_info[count].rect ))
        {
            enum_info[count++].handle = monitor->handle;
            break;
        }
    }

    /* then non-primary monitors */
    LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
    {
        if (is_monitor_primary( monitor )) continue;
        if (should_enumerate_monitor( monitor, &origin, &limit, &enum_info[count].rect ))
            enum_info[count++].handle = monitor->handle;
    }

    unlock_display_devices();

    params.proc = proc;
    params.hdc = hdc;
    params.lparam = lparam;
    for (i = 0; i < count && ret; i++)
    {
        void *ret_ptr;
        ULONG ret_len;
        NTSTATUS status;
        params.monitor = enum_info[i].handle;
        params.rect = enum_info[i].rect;
        status = KeUserModeCallback( NtUserCallEnumDisplayMonitor, &params, sizeof(params),
                                     &ret_ptr, &ret_len );
        if (!status && ret_len == sizeof(ret)) ret = *(BOOL *)ret_ptr;
        else ret = FALSE;
    }
    if (enum_info != enum_buf) free( enum_info );
    return ret;
}

static BOOL get_monitor_info( HMONITOR handle, MONITORINFO *info, UINT dpi )
{
    struct monitor *monitor;

    if (info->cbSize != sizeof(MONITORINFOEXW) && info->cbSize != sizeof(MONITORINFO)) return FALSE;

    if (!lock_display_devices( FALSE )) return FALSE;

    if ((monitor = get_monitor_from_handle( handle )))
    {
        monitor_get_info( monitor, info, dpi );
        unlock_display_devices();

        TRACE( "flags %04x, monitor %s, work %s\n", (int)info->dwFlags,
               wine_dbgstr_rect(&info->rcMonitor), wine_dbgstr_rect(&info->rcWork));
        return TRUE;
    }

    unlock_display_devices();
    WARN( "invalid handle %p\n", handle );
    RtlSetLastWin32Error( ERROR_INVALID_MONITOR_HANDLE );
    return FALSE;
}

static HMONITOR monitor_from_rect( const RECT *rect, UINT flags, UINT dpi )
{
    struct monitor *monitor;
    HMONITOR ret = 0;
    RECT r;

    r = map_dpi_rect( *rect, dpi, system_dpi );

    if (!lock_display_devices( FALSE )) return 0;
    if ((monitor = get_monitor_from_rect( r, flags, system_dpi, MDT_DEFAULT ))) ret = monitor->handle;
    unlock_display_devices();

    TRACE( "%s flags %x returning %p\n", wine_dbgstr_rect(rect), flags, ret );
    return ret;
}

MONITORINFO monitor_info_from_rect( RECT rect, UINT dpi )
{
    MONITORINFO info = {.cbSize = sizeof(info)};
    struct monitor *monitor;

    if (!lock_display_devices( FALSE )) return info;
    if ((monitor = get_monitor_from_rect( rect, MONITOR_DEFAULTTONEAREST, dpi, MDT_DEFAULT )))
        monitor_get_info( monitor, &info, dpi );
    unlock_display_devices();

    return info;
}

UINT monitor_dpi_from_rect( RECT rect, UINT dpi, UINT *raw_dpi )
{
    struct monitor *monitor;
    UINT ret = system_dpi, x, y;

    if (!lock_display_devices( FALSE )) return 0;
    if ((monitor = get_monitor_from_rect( rect, MONITOR_DEFAULTTONEAREST, dpi, MDT_DEFAULT )))
    {
        *raw_dpi = monitor_get_dpi( monitor, MDT_RAW_DPI, &x, &y );
        ret = monitor_get_dpi( monitor, MDT_DEFAULT, &x, &y );
    }
    unlock_display_devices();

    return ret;
}

/* see MonitorFromWindow */
HMONITOR monitor_from_window( HWND hwnd, UINT flags, UINT dpi )
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

MONITORINFO monitor_info_from_window( HWND hwnd, UINT flags )
{
    MONITORINFO info = {.cbSize = sizeof(info)};
    HMONITOR monitor = monitor_from_window( hwnd, flags, get_thread_dpi() );
    get_monitor_info( monitor, &info, get_thread_dpi() );
    return info;
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
    switch (NTUSER_DPI_CONTEXT_GET_AWARENESS( get_thread_dpi_awareness_context() ))
    {
    case DPI_AWARENESS_UNAWARE:      *x = *y = USER_DEFAULT_SCREEN_DPI; break;
    case DPI_AWARENESS_SYSTEM_AWARE: *x = *y = system_dpi; break;
    default:                         get_monitor_dpi( monitor, type, x, y ); break;
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

/* Set server auto-repeat properties. delay and speed are expressed in terms of
 * SPI_KEYBOARDDELAY and SPI_KEYBOARDSPEED values. Returns whether auto-repeat
 * was enabled before this request. */
static BOOL set_server_keyboard_repeat( int enable, int delay, int speed )
{
    BOOL enabled = FALSE;

    SERVER_START_REQ( set_keyboard_repeat )
    {
        req->enable = enable >= 0 ? (enable > 0) : -1;
        req->delay = delay >= 0 ? (delay + 1) * 250 : -1;
        req->period = speed >= 0 ? 400 / (speed + 1) : -1;
        if (!wine_server_call( req )) enabled = reply->enable;
    }
    SERVER_END_REQ;

    return enabled;
}

/* retrieve the cached base keys for a given entry */
static BOOL get_base_keys( enum parameter_key index, HKEY *base_key, HKEY *volatile_key )
{
    static HKEY base_keys[NB_PARAM_KEYS];
    static HKEY volatile_keys[NB_PARAM_KEYS];
    HKEY key;

    if (!base_keys[index] && base_key)
    {
        if (!(key = reg_create_ascii_key( hkcu_key, parameter_key_names[index], 0, NULL )))
            return FALSE;
        if (InterlockedCompareExchangePointer( (void **)&base_keys[index], key, 0 ))
            NtClose( key );
    }
    if (!volatile_keys[index] && volatile_key)
    {
        if (!(key = reg_create_ascii_key( volatile_base_key, parameter_key_names[index], REG_OPTION_VOLATILE, NULL )))
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

    snprintf( buf, sizeof(buf), "%d", int_param );
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

    snprintf( buf, sizeof(buf), "%d", entry->uint.val );
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

    snprintf( buf, sizeof(buf), "%u", int_param );
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

    snprintf( buf, sizeof(buf), "%u", entry->uint.val );
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
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->boolean.val = wcstol( buf, NULL, 10 ) != 0;
    }
    *(UINT *)ptr_param = entry->boolean.val;
    return TRUE;
}

/* set a bool parameter in the registry */
static BOOL set_bool_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    WCHAR buf[] = { int_param ? '1' : '0', 0 };

    if (!save_entry_string( &entry->hdr, buf, flags )) return FALSE;
    entry->boolean.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter */
static BOOL init_bool_entry( union sysparam_all_entry *entry )
{
    WCHAR buf[] = { entry->boolean.val ? '1' : '0', 0 };

    return init_entry_string( &entry->hdr, buf );
}

/* load a bool parameter using Yes/No strings from the registry */
static BOOL get_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    if (!ptr_param) return FALSE;

    if (!entry->hdr.loaded)
    {
        WCHAR buf[32];
        if (load_entry( &entry->hdr, buf, sizeof(buf) )) entry->boolean.val = !wcsicmp( yesW, buf );
    }
    *(UINT *)ptr_param = entry->boolean.val;
    return TRUE;
}

/* set a bool parameter using Yes/No strings from the registry */
static BOOL set_yesno_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    const WCHAR *str = int_param ? yesW : noW;

    if (!save_entry_string( &entry->hdr, str, flags )) return FALSE;
    entry->boolean.val = int_param != 0;
    entry->hdr.loaded = TRUE;
    return TRUE;
}

/* initialize a bool parameter using Yes/No strings */
static BOOL init_yesno_entry( union sysparam_all_entry *entry )
{
    return init_entry_string( &entry->hdr, entry->boolean.val ? yesW : noW );
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

    snprintf( buf, sizeof(buf), "%u %u %u", GetRValue(int_param), GetGValue(int_param), GetBValue(int_param) );
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

    snprintf( buf, sizeof(buf), "%u %u %u", GetRValue(entry->rgb.val), GetGValue(entry->rgb.val),
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

LONG get_char_dimensions( HDC hdc, TEXTMETRICW *metric, int *height )
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
    static int cx, cy;

    if (!cx)
    {
        HDC hdc;

        if ((hdc = NtUserGetDC( 0 )))
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
            font.lfCharSet = DEFAULT_CHARSET;
            if (font.lfHeight > 0) /* positive height value means points ( inch/72 ) */
                font.lfHeight = -muldiv( font.lfHeight, USER_DEFAULT_SCREEN_DPI, 72 );
            entry->font.val = font;
            break;
        case sizeof(LOGFONT16): /* win9x-winME format */
            logfont16to32( (LOGFONT16 *)&font, &entry->font.val );
            entry->font.val.lfCharSet = DEFAULT_CHARSET;
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
            font.lfCharSet = DEFAULT_CHARSET;
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
    entry->font.val.lfCharSet = DEFAULT_CHARSET;
    entry->font.val.lfHeight = map_from_system_dpi( entry->font.val.lfHeight );
    entry->font.val.lfWeight = entry->font.weight;
    get_real_fontname( &entry->font.val, entry->font.fullname );
    return init_entry( &entry->hdr, &entry->font.val, sizeof(entry->font.val), REG_BINARY );
}

/* get a user pref parameter in the registry */
static BOOL get_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT dpi )
{
    union sysparam_all_entry *parent_entry = entry->pref.parent;
    BYTE prefs[8];

    if (!ptr_param) return FALSE;

    if (!parent_entry->hdr.get( parent_entry, sizeof(prefs), prefs, dpi )) return FALSE;
    *(BOOL *)ptr_param = (prefs[entry->pref.offset] & entry->pref.mask) != 0;
    return TRUE;
}

/* set a user pref parameter in the registry */
static BOOL set_userpref_entry( union sysparam_all_entry *entry, UINT int_param, void *ptr_param, UINT flags )
{
    union sysparam_all_entry *parent_entry = entry->pref.parent;
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

#define UINT_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .uint = { { get_uint_entry, set_uint_entry, init_uint_entry, base, reg }, (val) } }

#define UINT_ENTRY_MIRROR(name,val,base,reg,mirror_base) union sysparam_all_entry entry_##name = \
    { .uint = { { get_uint_entry, set_uint_entry, init_uint_entry, base, reg, mirror_base, reg }, (val) } }

#define INT_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .uint = { { get_uint_entry, set_int_entry, init_int_entry, base, reg }, (val) } }

#define BOOL_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .boolean = { { get_bool_entry, set_bool_entry, init_bool_entry, base, reg }, (val) } }

#define BOOL_ENTRY_MIRROR(name,val,base,reg,mirror_base) union sysparam_all_entry entry_##name = \
    { .boolean = { { get_bool_entry, set_bool_entry, init_bool_entry, base, reg, mirror_base, reg }, (val) } }

#define TWIPS_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .uint = { { get_twips_entry, set_twips_entry, init_int_entry, base, reg }, (val) } }

#define YESNO_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .boolean = { { get_yesno_entry, set_yesno_entry, init_yesno_entry, base, reg }, (val) } }

#define DWORD_ENTRY(name,val,base,reg) union sysparam_all_entry entry_##name = \
    { .dword = { { get_dword_entry, set_dword_entry, init_dword_entry, base, reg }, (val) } }

#define BINARY_ENTRY(name,data,base,reg) union sysparam_all_entry entry_##name = \
    { .bin = { { get_binary_entry, set_binary_entry, init_binary_entry, base, reg }, data, sizeof(data) } }

#define PATH_ENTRY(name,base,reg,buffer) union sysparam_all_entry entry_##name = \
    { .path = { { get_path_entry, set_path_entry, init_path_entry, base, reg }, buffer } }

#define FONT_ENTRY(name,weight,base,reg) union sysparam_all_entry entry_##name = \
    { .font = { { get_font_entry, set_font_entry, init_font_entry, base, reg }, (weight) } }

#define USERPREF_ENTRY(name,offset,mask) union sysparam_all_entry entry_##name = \
    { .pref = { { get_userpref_entry, set_userpref_entry }, &entry_USERPREFERENCESMASK, (offset), (mask) } }

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

static WCHAR desk_pattern_path[MAX_PATH];
static WCHAR desk_wallpaper_path[MAX_PATH];
static PATH_ENTRY( DESKPATTERN, DESKTOP_KEY, "Pattern", desk_pattern_path );
static PATH_ENTRY( DESKWALLPAPER, DESKTOP_KEY, "Wallpaper", desk_wallpaper_path );

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

/***********************************************************************
 *      get_config_key
 *
 * Get a config key from either the app-specific or the default config
 */
static DWORD get_config_key( HKEY defkey, HKEY appkey, const char *name,
                             WCHAR *buffer, DWORD size )
{
    char buf[2048];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buf;

    if (appkey && query_reg_ascii_value( appkey, name, info, sizeof(buf) ))
    {
        size = min( info->DataLength, size - sizeof(WCHAR) );
        memcpy( buffer, info->Data, size );
        buffer[size / sizeof(WCHAR)] = 0;
        return 0;
    }

    if (defkey && query_reg_ascii_value( defkey, name, info, sizeof(buf) ))
    {
        size = min( info->DataLength, size - sizeof(WCHAR) );
        memcpy( buffer, info->Data, size );
        buffer[size / sizeof(WCHAR)] = 0;
        return 0;
    }

    return ERROR_FILE_NOT_FOUND;
}

static char *get_app_compat_flags( const WCHAR *appname )
{
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
    WCHAR *value_str = NULL;
    char *compat_flags;
    HKEY hkey;
    UINT i;

    if ((hkey = reg_open_hkcu_key( "Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers" )))
    {
        if ((query_reg_value( hkey, appname, value, sizeof(buffer) ) && value->Type == REG_SZ) ||
            /* Wine extension: set default application flag using the default key value */
            (query_reg_value( hkey, NULL, value, sizeof(buffer) ) && value->Type == REG_SZ))
            value_str = (WCHAR *)value->Data;
        NtClose( hkey );
    }

    if (!value_str && (hkey = reg_open_ascii_key( NULL, "Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers" )))
    {
        if ((query_reg_value( hkey, appname, value, sizeof(buffer) ) && value->Type == REG_SZ) ||
            /* Wine extension: set default application flag using the default key value */
            (query_reg_value( hkey, NULL, value, sizeof(buffer) ) && value->Type == REG_SZ))
            value_str = (WCHAR *)value->Data;
        NtClose( hkey );
    }

    if (!value_str) return NULL;
    if (!(compat_flags = calloc( 1, value->DataLength / sizeof(WCHAR) + 1 ))) return NULL;
    for (i = 0; compat_flags && i < value->DataLength / sizeof(WCHAR); i++) compat_flags[i] = value_str[i];
    return compat_flags;
}

void sysparams_init(void)
{
    WCHAR buffer[MAX_PATH+16], *p, *appname;
    char *app_compat_flags = NULL;
    DWORD i, dispos, dpi_scaling;
    WCHAR layout[KL_NAMELENGTH];
    pthread_mutexattr_t attr;
    HKEY hkey, appkey = 0;
    DWORD len;

    static const WCHAR oneW[] = {'1',0};
    static const WCHAR x11driverW[] = {'\\','X','1','1',' ','D','r','i','v','e','r',0};

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &user_mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    if ((hkey = reg_create_ascii_key( hkcu_key, "Keyboard Layout\\Preload", 0, NULL )))
    {
        if (NtUserGetKeyboardLayoutName( layout ))
            set_reg_value( hkey, oneW, REG_SZ, (const BYTE *)layout,
                           (lstrlenW(layout) + 1) * sizeof(WCHAR) );
        NtClose( hkey );
    }

    /* this one must be non-volatile */
    if (!(hkey = reg_create_ascii_key( hkcu_key, "Software\\Wine", 0, NULL )))
    {
        ERR("Can't create wine registry branch\n");
        return;
    }

    /* @@ Wine registry key: HKCU\Software\Wine\Temporary System Parameters */
    if (!(volatile_base_key = reg_create_ascii_key( hkey, "Temporary System Parameters", REG_OPTION_VOLATILE, &dispos )))
        ERR("Can't create non-permanent wine registry branch\n");

    NtClose( hkey );

    config_key = reg_create_ascii_key( NULL, config_keyA, 0, NULL );

    get_dword_entry( (union sysparam_all_entry *)&entry_LOGPIXELS, 0, &system_dpi, 0 );
    if (!system_dpi)  /* check fallback key */
    {
        if ((hkey = reg_open_ascii_key( config_key, "Software\\Fonts" )))
        {
            char buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[sizeof(DWORD)])];
            KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;

            if (query_reg_ascii_value( hkey, "LogPixels", value, sizeof(buffer) ) && value->Type == REG_DWORD)
                system_dpi = *(const DWORD *)value->Data;
            NtClose( hkey );
        }
    }
    if (!system_dpi) system_dpi = USER_DEFAULT_SCREEN_DPI;

    /* FIXME: what do the DpiScalingVer flags mean? */
    get_dword_entry( (union sysparam_all_entry *)&entry_DPISCALINGVER, 0, &dpi_scaling, 0 );

    if (volatile_base_key && dispos == REG_CREATED_NEW_KEY)  /* first process, initialize entries */
    {
        for (i = 0; i < ARRAY_SIZE( default_entries ); i++)
            default_entries[i]->hdr.init( default_entries[i] );
    }

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver */
    hkey = reg_open_hkcu_key( "Software\\Wine\\X11 Driver" );

    /* open the app-specific key */

    appname = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    if ((p = wcsrchr( appname, '/' ))) appname = p + 1;
    if ((p = wcsrchr( appname, '\\' ))) appname = p + 1;
    len = lstrlenW( appname );

    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        int i;

        for (i = 0; appname[i]; i++) buffer[i] = RtlDowncaseUnicodeChar( appname[i] );
        buffer[i] = 0;
        appname = buffer;

        app_compat_flags = get_app_compat_flags( appname );
        if (app_compat_flags) TRACE( "Found %s AppCompatFlags %s\n", debugstr_w(appname), debugstr_a(app_compat_flags) );

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\X11 Driver */
        if ((tmpkey = reg_open_hkcu_key( "Software\\Wine\\AppDefaults" )))
        {
            memcpy( appname + i, x11driverW, sizeof(x11driverW) );
            appkey = reg_open_key( tmpkey, appname, lstrlenW( appname ) * sizeof(WCHAR) );
            NtClose( tmpkey );
        }
    }

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

    if (!get_config_key( hkey, appkey, "GrabPointer", buffer, sizeof(buffer) ))
        grab_pointer = IS_OPTION_TRUE( buffer[0] );
    if (!get_config_key( hkey, appkey, "GrabFullscreen", buffer, sizeof(buffer) ))
        grab_fullscreen = IS_OPTION_TRUE( buffer[0] );
    if (!get_config_key( hkey, appkey, "Decorated", buffer, sizeof(buffer) ))
        decorated_mode = IS_OPTION_TRUE( buffer[0] );
    if (!get_config_key( hkey, appkey, "EmulateModeset", buffer, sizeof(buffer) ))
        emulate_modeset = IS_OPTION_TRUE( buffer[0] );

#undef IS_OPTION_TRUE

    if (app_compat_flags)
    {
        if (strstr( app_compat_flags, "HIGHDPIAWARE" )) NtUserSetProcessDpiAwarenessContext( NTUSER_DPI_SYSTEM_AWARE, 0 );
        if (strstr( app_compat_flags, "DPIUNAWARE" )) NtUserSetProcessDpiAwarenessContext( NTUSER_DPI_UNAWARE, 0 );
    }
    free( app_compat_flags );
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
        if ((ret = set_entry( &entry_KEYBOARDSPEED, val, ptr, winini )))
            set_server_keyboard_repeat( -1,  -1, val );
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
        if ((ret = set_entry( &entry_KEYBOARDDELAY, val, ptr, winini )))
            set_server_keyboard_repeat( -1,  val, -1 );
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
        MONITORINFO info = {.cbSize = sizeof(info)};
        UINT dpi = get_thread_dpi();

        if (!ptr) return FALSE;

        spi_idx = SPI_SETWORKAREA_IDX;
        if (!spi_loaded[spi_idx])
        {
            struct monitor *monitor;

            if (!lock_display_devices( FALSE )) return FALSE;

            LIST_FOR_EACH_ENTRY( monitor, &monitors, struct monitor, entry )
            {
                if (!is_monitor_primary( monitor )) continue;
                monitor_get_info( monitor, &info, dpi );
                work_area = info.rcWork;
                break;
            }

            unlock_display_devices();
            spi_loaded[spi_idx] = TRUE;
        }
        *(RECT *)ptr = work_area;
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
        rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
        return rect.left;
    case SM_YVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
        return rect.top;
    case SM_CXVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
        return rect.right - rect.left;
    case SM_CYVIRTUALSCREEN:
        rect = get_virtual_screen_rect( get_thread_dpi(), MDT_DEFAULT );
        return rect.bottom - rect.top;
    case SM_CMONITORS:
        if (!lock_display_devices( FALSE )) return FALSE;
        ret = active_unique_monitor_count();
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
        if (colors[i] >= 0 && colors[i] < ARRAY_SIZE( system_colors ))
            set_entry( &system_colors[colors[i]], values[i], 0, 0 );

    /* Send WM_SYSCOLORCHANGE message to all windows */
    send_message_timeout( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0,
                          SMTO_ABORTIFHUNG, 2000, FALSE );
    /* Repaint affected portions of all visible windows */
    NtUserRedrawWindow( 0, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}

/***********************************************************************
 *	     NtUserSetProcessDpiAwarenessContext    (win32u.@)
 */
BOOL WINAPI NtUserSetProcessDpiAwarenessContext( ULONG context, ULONG unknown )
{
    if (!is_valid_dpi_awareness_context( context, system_dpi ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (InterlockedCompareExchange( &dpi_context, context, 0 ))
    {
        RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    TRACE( "set to %#x\n", (UINT)context );
    return TRUE;
}

/***********************************************************************
 *	     NtUserGetProcessDpiAwarenessContext    (win32u.@)
 */
ULONG WINAPI NtUserGetProcessDpiAwarenessContext( HANDLE process )
{
    ULONG context;

    if (process && process != GetCurrentProcess())
    {
        WARN( "not supported on other process %p\n", process );
        return NTUSER_DPI_UNAWARE;
    }

    context = ReadNoFence( &dpi_context );
    if (!context) return NTUSER_DPI_UNAWARE;
    return context;
}

/***********************************************************************
 *	     NtUserSetProcessDefaultLayout    (win32u.@)
 */
BOOL WINAPI NtUserSetProcessDefaultLayout( ULONG layout )
{
    process_layout = layout;
    return TRUE;
}

/***********************************************************************
 *	     NtUserMessageBeep    (win32u.@)
 */
BOOL WINAPI NtUserMessageBeep( UINT type )
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

    destroy_thread_windows();
    user_driver->pThreadDetach();

    free( thread_info->rawinput );

    cleanup_imm_thread();
    NtClose( thread_info->server_queue );
    free( thread_info->session_data );

    exiting_thread_id = 0;
}

static BOOL set_keyboard_auto_repeat( BOOL enable )
{
    UINT delay, speed;

    get_entry( &entry_KEYBOARDDELAY, 0, &delay );
    get_entry( &entry_KEYBOARDSPEED, 0, &speed );
    return set_server_keyboard_repeat( enable, delay, speed );
}

/***********************************************************************
 *	     NtUserCallNoParam    (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallNoParam( ULONG code )
{
    switch(code)
    {
    case NtUserCallNoParam_GetDesktopWindow:
        return HandleToUlong( get_desktop_window() );

    case NtUserCallNoParam_GetDialogBaseUnits:
        return get_dialog_base_units();

    case NtUserCallNoParam_GetLastInputTime:
        return get_last_input_time();

    case NtUserCallNoParam_GetProcessDefaultLayout:
        return process_layout;

    case NtUserCallNoParam_GetProgmanWindow:
        return HandleToUlong( get_progman_window() );

    case NtUserCallNoParam_GetShellWindow:
        return HandleToUlong( get_shell_window() );

    case NtUserCallNoParam_GetTaskmanWindow:
        return HandleToUlong( get_taskman_window() );

    case NtUserCallNoParam_DisplayModeChanged:
        display_mode_changed( FALSE );
        return TRUE;

    /* temporary exports */
    case NtUserExitingThread:
        exiting_thread_id = GetCurrentThreadId();
        return 0;

    case NtUserThreadDetach:
        thread_detach();
        return 0;

    default:
        FIXME( "invalid code %u\n", (int)code );
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

    case NtUserCallOneParam_EnableDC:
        return set_dce_flags( UlongToHandle(arg), DCHF_ENABLEDC );

    case NtUserCallOneParam_EnableThunkLock:
        thunk_lock_callback = arg;
        return 0;

    case NtUserCallOneParam_GetCursorPos:
        return get_cursor_pos( (POINT *)arg );

    case NtUserCallOneParam_GetIconParam:
        return get_icon_param( UlongToHandle(arg) );

    case NtUserCallOneParam_GetMenuItemCount:
        return get_menu_item_count( UlongToHandle(arg) );

    case NtUserCallOneParam_GetSysColor:
        return get_sys_color( arg );

    case NtUserCallOneParam_GetPrimaryMonitorRect:
        *(RECT *)arg = get_primary_monitor_rect( 0 );
        return 1;

    case NtUserCallOneParam_GetSysColorBrush:
        return HandleToUlong( get_sys_color_brush(arg) );

    case NtUserCallOneParam_GetSysColorPen:
        return HandleToUlong( get_sys_color_pen(arg) );

    case NtUserCallOneParam_GetSystemMetrics:
        return get_system_metrics( arg );

    case NtUserCallOneParam_SetKeyboardAutoRepeat:
        return set_keyboard_auto_repeat( arg );

    case NtUserCallOneParam_SetThreadDpiAwarenessContext:
        return set_thread_dpi_awareness_context( arg );

    case NtUserCallOneParam_D3DKMTOpenAdapterFromGdiDisplayName:
        return d3dkmt_open_adapter_from_gdi_display_name( (void *)arg );

    case NtUserCallOneParam_GetAsyncKeyboardState:
        return get_async_keyboard_state( (void *)arg );

    /* temporary exports */
    case NtUserGetDeskPattern:
        return get_entry( &entry_DESKPATTERN, 256, (WCHAR *)arg );

    default:
        FIXME( "invalid code %u\n", (int)code );
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
        return get_monitor_info( UlongToHandle(arg1), (MONITORINFO *)arg2, get_thread_dpi() );

    case NtUserCallTwoParam_GetSystemMetricsForDpi:
        return get_system_metrics_for_dpi( arg1, arg2 );

    case NtUserCallTwoParam_MonitorFromRect:
        return HandleToUlong( monitor_from_rect( (const RECT *)arg1, arg2, get_thread_dpi() ));

    case NtUserCallTwoParam_SetIconParam:
        return set_icon_param( UlongToHandle(arg1), UlongToHandle(arg2) );

    case NtUserCallTwoParam_SetIMECompositionRect:
        return set_ime_composition_rect( UlongToHandle(arg1), *(const RECT *)arg2 );

    case NtUserCallTwoParam_AdjustWindowRect:
    {
        struct adjust_window_rect_params *params = (void *)arg2;
        return adjust_window_rect( (RECT *)arg1, params->style, params->menu, params->ex_style, params->dpi );
    }

    case NtUserCallTwoParam_GetVirtualScreenRect:
        *(RECT *)arg1 = get_virtual_screen_rect( 0, arg2 );
        return 1;

    /* temporary exports */
    case NtUserAllocWinProc:
        return (UINT_PTR)alloc_winproc( (WNDPROC)arg1, arg2 );

    default:
        FIXME( "invalid code %u\n", (int)code );
        return 0;
    }
}

/***********************************************************************
 *           NtUserDisplayConfigGetDeviceInfo    (win32u.@)
 */
NTSTATUS WINAPI NtUserDisplayConfigGetDeviceInfo( DISPLAYCONFIG_DEVICE_INFO_HEADER *packet )
{
    NTSTATUS ret = STATUS_UNSUCCESSFUL;
    char buffer[CCHDEVICENAME];

    TRACE( "packet %p.\n", packet );

    if (!packet || packet->size < sizeof(*packet))
        return STATUS_UNSUCCESSFUL;

    switch (packet->type)
    {
    case DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME:
    {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME *source_name = (DISPLAYCONFIG_SOURCE_DEVICE_NAME *)packet;
        struct source *source;

        TRACE( "DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME.\n" );

        if (packet->size < sizeof(*source_name))
            return STATUS_INVALID_PARAMETER;

        if (!lock_display_devices( FALSE )) return STATUS_UNSUCCESSFUL;

        LIST_FOR_EACH_ENTRY(source, &sources, struct source, entry)
        {
            if (source_name->header.id != source->id) continue;
            if (memcmp( &source_name->header.adapterId, &source->gpu->luid, sizeof(source->gpu->luid) )) continue;

            snprintf( buffer, sizeof(buffer), "\\\\.\\DISPLAY%d", source->id + 1 );
            asciiz_to_unicode( source_name->viewGdiDeviceName, buffer );
            ret = STATUS_SUCCESS;
            break;
        }

        unlock_display_devices();
        return ret;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME:
    {
        DISPLAYCONFIG_TARGET_DEVICE_NAME *target_name = (DISPLAYCONFIG_TARGET_DEVICE_NAME *)packet;
        char buffer[ARRAY_SIZE(target_name->monitorFriendlyDeviceName)];
        struct monitor *monitor;

        TRACE( "DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME.\n" );

        if (packet->size < sizeof(*target_name))
            return STATUS_INVALID_PARAMETER;

        if (!lock_display_devices( FALSE )) return STATUS_UNSUCCESSFUL;

        memset( &target_name->flags, 0, sizeof(*target_name) - offsetof(DISPLAYCONFIG_TARGET_DEVICE_NAME, flags) );

        LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
        {
            if (target_name->header.id != monitor->output_id) continue;
            if (memcmp( &target_name->header.adapterId, &monitor->source->gpu->luid,
                        sizeof(monitor->source->gpu->luid) ))
                continue;

            target_name->outputTechnology = DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL;
            snprintf( buffer, ARRAY_SIZE(buffer), "Display%u", monitor->output_id + 1 );
            asciiz_to_unicode( target_name->monitorFriendlyDeviceName, buffer );
            monitor_get_interface_name( monitor, target_name->monitorDevicePath );
            if (monitor->edid_info.flags & MONITOR_INFO_HAS_MONITOR_ID)
            {
                target_name->edidManufactureId = monitor->edid_info.manufacturer;
                target_name->edidProductCodeId = monitor->edid_info.product_code;
                target_name->flags.edidIdsValid = 1;
            }
            if (monitor->edid_info.flags & MONITOR_INFO_HAS_MONITOR_NAME)
            {
                wcscpy( target_name->monitorFriendlyDeviceName, monitor->edid_info.monitor_name );
                target_name->flags.friendlyNameFromEdid = 1;
            }
            ret = STATUS_SUCCESS;
            break;
        }

        unlock_display_devices();
        return ret;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE:
    {
        DISPLAYCONFIG_TARGET_PREFERRED_MODE *preferred_mode = (DISPLAYCONFIG_TARGET_PREFERRED_MODE *)packet;
        DISPLAYCONFIG_VIDEO_SIGNAL_INFO *signal_info = &preferred_mode->targetMode.targetVideoSignalInfo;
        unsigned int i, display_freq;
        DEVMODEW *found_mode = NULL;
        BOOL have_edid_mode = FALSE;
        struct monitor *monitor;

        FIXME( "DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE semi-stub.\n" );

        if (packet->size < sizeof(*preferred_mode))
            return STATUS_INVALID_PARAMETER;

        if (!lock_display_devices( FALSE )) return STATUS_UNSUCCESSFUL;

        memset( &preferred_mode->width, 0, sizeof(*preferred_mode) - offsetof(DISPLAYCONFIG_TARGET_PREFERRED_MODE, width) );

        LIST_FOR_EACH_ENTRY(monitor, &monitors, struct monitor, entry)
        {
            if (preferred_mode->header.id != monitor->output_id) continue;
            if (memcmp( &preferred_mode->header.adapterId, &monitor->source->gpu->luid,
                        sizeof(monitor->source->gpu->luid) ))
                continue;

            for (i = 0; i < monitor->source->mode_count; ++i)
            {
                DEVMODEW *mode = &monitor->source->modes[i];

                if (!have_edid_mode && monitor->edid_info.flags & MONITOR_INFO_HAS_PREFERRED_MODE
                    && mode->dmPelsWidth == monitor->edid_info.preferred_width
                    && mode->dmPelsHeight == monitor->edid_info.preferred_height)
                {
                    found_mode = mode;
                    have_edid_mode = TRUE;
                }

                if (!have_edid_mode && (!found_mode
                    || (mode->dmPelsWidth > found_mode->dmPelsWidth && mode->dmPelsHeight >= found_mode->dmPelsHeight)
                    || (mode->dmPelsHeight > found_mode->dmPelsHeight && mode->dmPelsWidth >= found_mode->dmPelsWidth)))
                    found_mode = mode;

                if (mode->dmPelsWidth == found_mode->dmPelsWidth
                    && mode->dmPelsHeight == found_mode->dmPelsHeight
                    && mode->dmDisplayFrequency > found_mode->dmDisplayFrequency)
                    found_mode = mode;
            }

            if (!found_mode)
            {
                ERR( "No mode found.\n" );
                break;
            }
            preferred_mode->width = found_mode->dmPelsWidth;
            preferred_mode->height = found_mode->dmPelsHeight;
            display_freq = found_mode->dmDisplayFrequency;

            signal_info->pixelRate = display_freq * preferred_mode->width * preferred_mode->height;
            signal_info->hSyncFreq.Numerator = display_freq * preferred_mode->width;
            signal_info->hSyncFreq.Denominator = 1;
            signal_info->vSyncFreq.Numerator = display_freq;
            signal_info->vSyncFreq.Denominator = 1;
            signal_info->activeSize.cx = preferred_mode->width;
            signal_info->activeSize.cy = preferred_mode->height;
            signal_info->totalSize.cx = preferred_mode->width;
            signal_info->totalSize.cy = preferred_mode->height;
            signal_info->videoStandard = D3DKMDT_VSS_OTHER;
            if (!(found_mode->dmFields & DM_DISPLAYFLAGS))
                signal_info->scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_UNSPECIFIED;
            else if (found_mode->dmDisplayFlags & DM_INTERLACED)
                signal_info->scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_INTERLACED;
            else
                signal_info->scanLineOrdering = DISPLAYCONFIG_SCANLINE_ORDERING_PROGRESSIVE;
            ret = STATUS_SUCCESS;
            break;
        }

        unlock_display_devices();
        return ret;
    }
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME:
    {
        DISPLAYCONFIG_ADAPTER_NAME *adapter_name = (DISPLAYCONFIG_ADAPTER_NAME *)packet;

        FIXME( "DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME stub.\n" );

        if (packet->size < sizeof(*adapter_name))
            return STATUS_INVALID_PARAMETER;

        return STATUS_NOT_SUPPORTED;
    }
    case DISPLAYCONFIG_DEVICE_INFO_SET_TARGET_PERSISTENCE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_BASE_TYPE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_SET_SUPPORT_VIRTUAL_RESOLUTION:
    case DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO:
    case DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE:
    case DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL:
    default:
        FIXME( "Unimplemented packet type %u.\n", packet->type );
        return STATUS_INVALID_PARAMETER;
    }
}

/******************************************************************************
 *           NtGdiDdDDIEnumAdapters2    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIEnumAdapters2( D3DKMT_ENUMADAPTERS2 *desc )
{
    D3DKMT_OPENADAPTERFROMLUID open_adapter_from_luid;
    struct gpu *gpu, *current_gpus[34];
    D3DKMT_CLOSEADAPTER close_adapter;
    NTSTATUS status = STATUS_SUCCESS;
    UINT idx = 0, count = 0;

    TRACE( "(%p)\n", desc );

    if (!desc) return STATUS_INVALID_PARAMETER;

    if (!desc->pAdapters)
    {
        desc->NumAdapters = ARRAY_SIZE(current_gpus);
        return STATUS_SUCCESS;
    }

    if (!lock_display_devices( FALSE )) return STATUS_UNSUCCESSFUL;
    LIST_FOR_EACH_ENTRY( gpu, &gpus, struct gpu, entry )
    {
        if (count >= ARRAY_SIZE(current_gpus))
        {
            WARN( "Too many adapters (%u), only up to %zu can be enumerated.\n",
                  count, ARRAY_SIZE(current_gpus) );
            break;
        }
        current_gpus[count++] = gpu_acquire( gpu );
    }
    unlock_display_devices();

    if (count > desc->NumAdapters)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto done;
    }

    for (idx = 0; idx < count; idx++)
    {
        gpu = current_gpus[idx];
        if (!gpu->luid.LowPart && !gpu->luid.HighPart)
        {
            ERR( "Adapter %u does not have a LUID.\n", idx );
            status = STATUS_UNSUCCESSFUL;
            break;
        }
        open_adapter_from_luid.AdapterLuid = gpu->luid;

        /* give the GDI driver a chance to be notified about new adapter handle */
        if ((status = NtGdiDdDDIOpenAdapterFromLuid( &open_adapter_from_luid )))
        {
            ERR( "Failed to open adapter %u from LUID, status %#x.\n", idx, (UINT)status );
            break;
        }

        desc->pAdapters[idx].hAdapter = open_adapter_from_luid.hAdapter;
        desc->pAdapters[idx].AdapterLuid = gpu->luid;
        desc->pAdapters[idx].NumOfSources = gpu->source_count;
        desc->pAdapters[idx].bPrecisePresentRegionsPreferred = FALSE;
    }

    if (idx != count)
    {
        while (idx)
        {
            close_adapter.hAdapter = desc->pAdapters[--idx].hAdapter;
            NtGdiDdDDICloseAdapter( &close_adapter );
        }

        memset( desc->pAdapters, 0, desc->NumAdapters * sizeof(D3DKMT_ADAPTERINFO) );
    }
    desc->NumAdapters = idx;

done:
    while (count) gpu_release( current_gpus[--count] );
    return status;
}

/******************************************************************************
 *           NtGdiDdDDIEnumAdapters    (win32u.@)
 */
NTSTATUS WINAPI NtGdiDdDDIEnumAdapters( D3DKMT_ENUMADAPTERS *desc )
{
    NTSTATUS status;
    D3DKMT_ENUMADAPTERS2 desc2 = {0};

    if (!desc) return STATUS_INVALID_PARAMETER;

    if ((status = NtGdiDdDDIEnumAdapters2( &desc2 ))) return status;

    if (!(desc2.pAdapters = calloc( desc2.NumAdapters, sizeof(D3DKMT_ADAPTERINFO) ))) return STATUS_NO_MEMORY;

    if (!(status = NtGdiDdDDIEnumAdapters2( &desc2 )))
    {
        desc->NumAdapters = min( MAX_ENUM_ADAPTERS, desc2.NumAdapters );
        memcpy( desc->Adapters, desc2.pAdapters, desc->NumAdapters * sizeof(D3DKMT_ADAPTERINFO) );
    }

    free( desc2.pAdapters );

    return status;
}

/* Find the Vulkan device UUID corresponding to a LUID */
BOOL get_vulkan_uuid_from_luid( const LUID *luid, GUID *uuid )
{
    BOOL found = FALSE;
    struct gpu *gpu;

    if (!lock_display_devices( FALSE )) return FALSE;

    LIST_FOR_EACH_ENTRY( gpu, &gpus, struct gpu, entry )
    {
        if ((found = !memcmp( &gpu->luid, luid, sizeof(*luid) )))
        {
            *uuid = gpu->vulkan_uuid;
            break;
        }
    }

    unlock_display_devices();
    return found;
}
