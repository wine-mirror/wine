/*
 * Android driver initialisation functions
 *
 * Copyright 1996, 2013, 2017 Alexandre Julliard
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

#include <stdarg.h>
#include <string.h>
#include <dlfcn.h>
#include <link.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "android.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

unsigned int screen_width = 0;
unsigned int screen_height = 0;
RECT virtual_screen_rect = { 0, 0, 0, 0 };

static const unsigned int screen_bpp = 32;  /* we don't support other modes */

static RECT monitor_rc_work;
static int device_init_done;

PNTAPCFUNC register_window_callback;
UINT64 start_device_callback;

typedef struct
{
    struct gdi_physdev dev;
} ANDROID_PDEVICE;

static const struct user_driver_funcs android_drv_funcs;


/******************************************************************************
 *           init_monitors
 */
void init_monitors( int width, int height )
{
    static const WCHAR trayW[] = {'S','h','e','l','l','_','T','r','a','y','W','n','d',0};
    UNICODE_STRING name;
    RECT rect;
    HWND hwnd;

    RtlInitUnicodeString( &name, trayW );
    hwnd = NtUserFindWindowEx( 0, 0, &name, NULL, 0 );

    virtual_screen_rect.right = width;
    virtual_screen_rect.bottom = height;
    monitor_rc_work = virtual_screen_rect;

    if (!hwnd || !NtUserIsWindowVisible( hwnd )) return;
    if (!NtUserGetWindowRect( hwnd, &rect, NtUserGetWinMonitorDpi( hwnd, MDT_RAW_DPI ) )) return;
    if (rect.top) monitor_rc_work.bottom = rect.top;
    else monitor_rc_work.top = rect.bottom;
    TRACE( "found tray %p %s work area %s\n", hwnd,
           wine_dbgstr_rect( &rect ), wine_dbgstr_rect( &monitor_rc_work ));

    /* if we're notified from Java thread, update registry */
    if (*p_java_vm) NtUserCallNoParam( NtUserCallNoParam_DisplayModeChanged );
}


/* wrapper for NtCreateKey that creates the key recursively if necessary */
static HKEY reg_create_key( const WCHAR *name, ULONG name_len )
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    status = NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, 0, NULL );
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        static const WCHAR registry_rootW[] = { '\\','R','e','g','i','s','t','r','y','\\' };
        DWORD pos = 0, i = 0, len = name_len / sizeof(WCHAR);

        /* don't try to create registry root */
        if (len > ARRAY_SIZE(registry_rootW) &&
            !memcmp( name, registry_rootW, sizeof(registry_rootW) ))
            i += ARRAY_SIZE(registry_rootW);

        while (i < len && name[i] != '\\') i++;
        if (i == len) return 0;
        for (;;)
        {
            nameW.Buffer = (WCHAR *)name + pos;
            nameW.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey( &ret, MAXIMUM_ALLOWED, &attr, 0, NULL, 0, NULL );

            if (attr.RootDirectory) NtClose( attr.RootDirectory );
            if (status) return 0;
            if (i == len) break;
            attr.RootDirectory = ret;
            while (i < len && name[i] == '\\') i++;
            pos = i;
            while (i < len && name[i] != '\\') i++;
        }
    }
    return ret;
}


/******************************************************************************
 *           set_screen_dpi
 */
void set_screen_dpi( DWORD dpi )
{
    static const WCHAR dpi_value_name[] = {'L','o','g','P','i','x','e','l','s',0};
    static const WCHAR dpi_key_name[] =
    {
        '\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','H','a','r','d','w','a','r','e',' ','P','r','o','f','i','l','e','s',
        '\\','C','u','r','r','e','n','t',
        '\\','S','o','f','t','w','a','r','e',
        '\\','F','o','n','t','s'
    };
    HKEY hkey;

    if ((hkey = reg_create_key( dpi_key_name, sizeof(dpi_key_name ))))
    {
        UNICODE_STRING name;
        RtlInitUnicodeString( &name, dpi_value_name );
        NtSetValueKey( hkey, &name, 0, REG_DWORD, &dpi, sizeof(dpi) );
        NtClose( hkey );
    }
}

/**********************************************************************
 *	     fetch_display_metrics
 */
static void fetch_display_metrics(void)
{
    if (*p_java_vm) return;  /* for Java threads it will be set when the top view is created */

    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( NtUserGetDesktopWindow() );
        req->relative = COORDS_CLIENT;
        if (!wine_server_call( req ))
        {
            screen_width  = reply->window.right;
            screen_height = reply->window.bottom;
        }
    }
    SERVER_END_REQ;

    init_monitors( screen_width, screen_height );
    TRACE( "screen %ux%u\n", screen_width, screen_height );
}


/**********************************************************************
 *           device_init
 *
 * Perform initializations needed upon creation of the first device.
 */
static void device_init(void)
{
    device_init_done = TRUE;
    fetch_display_metrics();
}


/******************************************************************************
 *           create_android_physdev
 */
static ANDROID_PDEVICE *create_android_physdev(void)
{
    ANDROID_PDEVICE *physdev;

    if (!device_init_done) device_init();

    if (!(physdev = calloc( 1, sizeof(*physdev) ))) return NULL;
    return physdev;
}

/**********************************************************************
 *           ANDROID_CreateDC
 */
static BOOL ANDROID_CreateDC( PHYSDEV *pdev, LPCWSTR device, LPCWSTR output, const DEVMODEW *initData )
{
    ANDROID_PDEVICE *physdev = create_android_physdev();

    if (!physdev) return FALSE;

    push_dc_driver( pdev, &physdev->dev, &android_drv_funcs.dc_funcs );
    return TRUE;
}


/**********************************************************************
 *           ANDROID_CreateCompatibleDC
 */
static BOOL ANDROID_CreateCompatibleDC( PHYSDEV orig, PHYSDEV *pdev )
{
    ANDROID_PDEVICE *physdev = create_android_physdev();

    if (!physdev) return FALSE;

    push_dc_driver( pdev, &physdev->dev, &android_drv_funcs.dc_funcs );
    return TRUE;
}


/**********************************************************************
 *           ANDROID_DeleteDC
 */
static BOOL ANDROID_DeleteDC( PHYSDEV dev )
{
    free( dev );
    return TRUE;
}


/***********************************************************************
 *           ANDROID_ChangeDisplaySettings
 */
LONG ANDROID_ChangeDisplaySettings( LPDEVMODEW displays, LPCWSTR primary_name, HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    FIXME( "(%p,%s,%p,0x%08x,%p)\n", displays, debugstr_w(primary_name), hwnd, (int)flags, lpvoid );
    return DISP_CHANGE_SUCCESSFUL;
}


/***********************************************************************
 *           ANDROID_UpdateDisplayDevices
 */
UINT ANDROID_UpdateDisplayDevices( const struct gdi_device_manager *device_manager, void *param )
{
    static const DWORD source_flags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP | DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_VGA_COMPATIBLE;
    struct pci_id pci_id = {0};
    struct gdi_monitor gdi_monitor =
    {
        .rc_monitor = virtual_screen_rect,
        .rc_work = monitor_rc_work,
    };
    const DEVMODEW mode =
    {
        .dmSize = sizeof(mode),
        .dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL |
                    DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY,
        .dmBitsPerPel = screen_bpp, .dmPelsWidth = screen_width, .dmPelsHeight = screen_height, .dmDisplayFrequency = 60,
    };
    UINT dpi = NtUserGetSystemDpiForProcess( NULL );
    DEVMODEW current = mode;

    device_manager->add_gpu( "Wine GPU", &pci_id, NULL, param );
    device_manager->add_source( "Default", source_flags, dpi, param );
    device_manager->add_monitor( &gdi_monitor, param );

    current.dmFields |= DM_POSITION;
    device_manager->add_modes( &current, 1, &mode, param );

    return STATUS_SUCCESS;
}


static const struct user_driver_funcs android_drv_funcs =
{
    .dc_funcs.pCreateCompatibleDC = ANDROID_CreateCompatibleDC,
    .dc_funcs.pCreateDC = ANDROID_CreateDC,
    .dc_funcs.pDeleteDC = ANDROID_DeleteDC,
    .dc_funcs.priority = GDI_PRIORITY_GRAPHICS_DRV,

    .pGetKeyNameText = ANDROID_GetKeyNameText,
    .pMapVirtualKeyEx = ANDROID_MapVirtualKeyEx,
    .pVkKeyScanEx = ANDROID_VkKeyScanEx,
    .pSetCursor = ANDROID_SetCursor,
    .pChangeDisplaySettings = ANDROID_ChangeDisplaySettings,
    .pUpdateDisplayDevices = ANDROID_UpdateDisplayDevices,
    .pCreateDesktop = ANDROID_CreateDesktop,
    .pCreateWindow = ANDROID_CreateWindow,
    .pDesktopWindowProc = ANDROID_DesktopWindowProc,
    .pDestroyWindow = ANDROID_DestroyWindow,
    .pProcessEvents = ANDROID_ProcessEvents,
    .pSetCapture = ANDROID_SetCapture,
    .pSetParent = ANDROID_SetParent,
    .pShowWindow = ANDROID_ShowWindow,
    .pWindowMessage = ANDROID_WindowMessage,
    .pWindowPosChanging = ANDROID_WindowPosChanging,
    .pCreateWindowSurface = ANDROID_CreateWindowSurface,
    .pWindowPosChanged = ANDROID_WindowPosChanged,
    .pwine_get_wgl_driver = ANDROID_wine_get_wgl_driver,
};


static const JNINativeMethod methods[] =
{
    { "wine_desktop_changed", "(II)V", desktop_changed },
    { "wine_config_changed", "(I)V", config_changed },
    { "wine_surface_changed", "(ILandroid/view/Surface;Z)V", surface_changed },
    { "wine_motion_event", "(IIIIII)Z", motion_event },
    { "wine_keyboard_event", "(IIII)Z", keyboard_event },
};

#define DECL_FUNCPTR(f) typeof(f) * p##f = NULL
#define LOAD_FUNCPTR(lib, func) do { \
    if ((p##func = dlsym( lib, #func )) == NULL) \
        { ERR( "can't find symbol %s\n", #func); return; } \
    } while(0)

DECL_FUNCPTR( __android_log_print );
DECL_FUNCPTR( ANativeWindow_fromSurface );
DECL_FUNCPTR( ANativeWindow_release );
DECL_FUNCPTR( hw_get_module );

#ifndef DT_GNU_HASH
#define DT_GNU_HASH 0x6ffffef5
#endif

static unsigned int gnu_hash( const char *name )
{
    unsigned int h = 5381;
    while (*name) h = h * 33 + (unsigned char)*name++;
    return h;
}

static unsigned int hash_symbol( const char *name )
{
    unsigned int hi, hash = 0;
    while (*name)
    {
        hash = (hash << 4) + (unsigned char)*name++;
        hi = hash & 0xf0000000;
        hash ^= hi;
        hash ^= hi >> 24;
    }
    return hash;
}

static void *find_symbol( const struct dl_phdr_info* info, const char *var, int type )
{
    const ElfW(Dyn) *dyn = NULL;
    const ElfW(Phdr) *ph;
    const ElfW(Sym) *symtab = NULL;
    const Elf32_Word *hashtab = NULL;
    const Elf32_Word *gnu_hashtab = NULL;
    const char *strings = NULL;
    Elf32_Word idx;

    for (ph = info->dlpi_phdr; ph < &info->dlpi_phdr[info->dlpi_phnum]; ++ph)
    {
        if (PT_DYNAMIC == ph->p_type)
        {
            dyn = (const ElfW(Dyn) *)(info->dlpi_addr + ph->p_vaddr);
            break;
        }
    }
    if (!dyn) return NULL;

    while (dyn->d_tag)
    {
        if (dyn->d_tag == DT_STRTAB)
            strings = (const char*)(info->dlpi_addr + dyn->d_un.d_ptr);
        if (dyn->d_tag == DT_SYMTAB)
            symtab = (const ElfW(Sym) *)(info->dlpi_addr + dyn->d_un.d_ptr);
        if (dyn->d_tag == DT_HASH)
            hashtab = (const Elf32_Word *)(info->dlpi_addr + dyn->d_un.d_ptr);
        if (dyn->d_tag == DT_GNU_HASH)
            gnu_hashtab = (const Elf32_Word *)(info->dlpi_addr + dyn->d_un.d_ptr);
        dyn++;
    }

    if (!symtab || !strings) return NULL;

    if (gnu_hashtab)  /* new style hash table */
    {
        const unsigned int hash   = gnu_hash(var);
        const Elf32_Word nbuckets = gnu_hashtab[0];
        const Elf32_Word symbias  = gnu_hashtab[1];
        const Elf32_Word nwords   = gnu_hashtab[2];
        const ElfW(Addr) *bitmask = (const ElfW(Addr) *)(gnu_hashtab + 4);
        const Elf32_Word *buckets = (const Elf32_Word *)(bitmask + nwords);
        const Elf32_Word *chains  = buckets + nbuckets - symbias;

        if (!(idx = buckets[hash % nbuckets])) return NULL;
        do
        {
            if ((chains[idx] & ~1u) == (hash & ~1u) &&
                ELF32_ST_BIND(symtab[idx].st_info) == STB_GLOBAL &&
                ELF32_ST_TYPE(symtab[idx].st_info) == type &&
                !strcmp( strings + symtab[idx].st_name, var ))
                return (void *)(info->dlpi_addr + symtab[idx].st_value);
        } while (!(chains[idx++] & 1u));
    }
    else if (hashtab)  /* old style hash table */
    {
        const unsigned int hash   = hash_symbol( var );
        const Elf32_Word nbuckets = hashtab[0];
        const Elf32_Word *buckets = hashtab + 2;
        const Elf32_Word *chains  = buckets + nbuckets;

        for (idx = buckets[hash % nbuckets]; idx; idx = chains[idx])
        {
            if (ELF32_ST_BIND(symtab[idx].st_info) == STB_GLOBAL &&
                ELF32_ST_TYPE(symtab[idx].st_info) == type &&
                !strcmp( strings + symtab[idx].st_name, var ))
                return (void *)(info->dlpi_addr + symtab[idx].st_value);
        }
    }
    return NULL;
}

static int enum_libs( struct dl_phdr_info* info, size_t size, void* data )
{
    const char *p;

    if (!info->dlpi_name) return 0;
    if (!(p = strrchr( info->dlpi_name, '/' ))) return 0;
    if (strcmp( p, "/libhardware.so" )) return 0;
    TRACE( "found libhardware at %p\n", info->dlpi_phdr );
    phw_get_module = find_symbol( info, "hw_get_module", STT_FUNC );
    return 1;
}

static void load_hardware_libs(void)
{
    const struct hw_module_t *module;
    int ret;
    void *libhardware;

    if ((libhardware = dlopen( "libhardware.so", RTLD_GLOBAL )))
    {
        LOAD_FUNCPTR( libhardware, hw_get_module );
    }
    else
    {
        /* Android >= N disallows loading libhardware, so we load libandroid (which imports
         * libhardware), and then we can find libhardware in the list of loaded libraries.
         */
        if (!dlopen( "libandroid.so", RTLD_GLOBAL ))
        {
            ERR( "failed to load libandroid.so: %s\n", dlerror() );
            return;
        }
        dl_iterate_phdr( enum_libs, 0 );
        if (!phw_get_module)
        {
            ERR( "failed to find hw_get_module\n" );
            return;
        }
    }

    if ((ret = phw_get_module( GRALLOC_HARDWARE_MODULE_ID, &module )))
    {
        ERR( "failed to load gralloc module err %d\n", ret );
        return;
    }

    init_gralloc( module );
}

static void load_android_libs(void)
{
    void *libandroid, *liblog;

    if (!(libandroid = dlopen( "libandroid.so", RTLD_GLOBAL )))
    {
        ERR( "failed to load libandroid.so: %s\n", dlerror() );
        return;
    }
    if (!(liblog = dlopen( "liblog.so", RTLD_GLOBAL )))
    {
        ERR( "failed to load liblog.so: %s\n", dlerror() );
        return;
    }
    LOAD_FUNCPTR( liblog, __android_log_print );
    LOAD_FUNCPTR( libandroid, ANativeWindow_fromSurface );
    LOAD_FUNCPTR( libandroid, ANativeWindow_release );
}

#undef DECL_FUNCPTR
#undef LOAD_FUNCPTR

JavaVM **p_java_vm = NULL;
jobject *p_java_object = NULL;
unsigned short *p_java_gdt_sel = NULL;

static HRESULT android_init( void *arg )
{
    struct init_params *params = arg;
    pthread_mutexattr_t attr;
    jclass class;
    jobject object;
    JNIEnv *jni_env;
    JavaVM *java_vm;
    void *ntdll;

    if (!(ntdll = dlopen( "ntdll.so", RTLD_NOW ))) return STATUS_UNSUCCESSFUL;

    p_java_vm = dlsym( ntdll, "java_vm" );
    p_java_object = dlsym( ntdll, "java_object" );
    p_java_gdt_sel = dlsym( ntdll, "java_gdt_sel" );

    object = *p_java_object;

    load_hardware_libs();

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &drawable_mutex, &attr );
    pthread_mutex_init( &win_data_mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    register_window_callback = params->register_window_callback;
    start_device_callback = params->start_device_callback;

    if ((java_vm = *p_java_vm))  /* running under Java */
    {
#ifdef __i386__
        WORD old_fs;
        __asm__( "mov %%fs,%0" : "=r" (old_fs) );
#endif
        load_android_libs();
        (*java_vm)->AttachCurrentThread( java_vm, &jni_env, 0 );
        class = (*jni_env)->GetObjectClass( jni_env, object );
        (*jni_env)->RegisterNatives( jni_env, class, methods, ARRAY_SIZE( methods ));
        (*jni_env)->DeleteLocalRef( jni_env, class );
#ifdef __i386__
        /* the Java VM hijacks %fs for its own purposes, restore it */
        __asm__( "mov %0,%%fs" :: "r" (old_fs) );
#endif
    }
    __wine_set_user_driver( &android_drv_funcs, WINE_GDI_DRIVER_VERSION );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    android_dispatch_ioctl,
    android_init,
    android_java_init,
    android_java_uninit,
    android_register_window,
};


C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );
