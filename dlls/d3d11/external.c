#include "config.h"
#include "wine/port.h"

#include "d3d11_private.h"

#define EXTERNAL_D3D_NO_WINDOWS_H
#define EXTERNAL_D3D_NO_VULKAN_H
#include "wine/library.h"
#include "wine/vulkan.h"
#include "wine/vulkan_driver.h"
#include "wine/external_d3d.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

static void* external_d3d_lib;
static const struct vulkan_funcs *vulkan_funcs;
static PFN_native_core_create_d3d11_device pfn_native_core_create_d3d11_device;

static char* get_desired_d3d_library(void)
{
    HKEY defkey;
    HKEY appkey;
    DWORD type, size;
    char buffer[MAX_PATH+10];
    DWORD len;
    LSTATUS status;

    static char* external_d3d_lib_name = NULL;

    if (external_d3d_lib_name)
        return external_d3d_lib_name;

    external_d3d_lib_name = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);

    /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
    if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &defkey ) ) defkey = 0;

    len = GetModuleFileNameA( 0, buffer, MAX_PATH );
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Direct3D */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            char *p, *appname = buffer;
            if ((p = strrchr( appname, '/' ))) appname = p + 1;
            if ((p = strrchr( appname, '\\' ))) appname = p + 1;
            strcat( appname, "\\Direct3D" );
            TRACE("appname = [%s]\n", appname);
            if (RegOpenKeyA( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    size = MAX_PATH;

    if (defkey) status = RegQueryValueExA(defkey, "external_d3d", 0, &type, (BYTE *)external_d3d_lib_name, &size);
    if (type != REG_SZ && appkey)
        status = RegQueryValueExA(appkey, "external_d3d", 0, &type, (BYTE *)external_d3d_lib_name, &size);
    if (type != REG_SZ)
    {
        HeapFree(GetProcessHeap(), 0, external_d3d_lib_name);
        external_d3d_lib_name = NULL;
    }

    RegCloseKey(appkey);
    RegCloseKey(defkey);

    return external_d3d_lib_name;
}

int is_external_d3d11_available(void)
{
    char* lib_name = get_desired_d3d_library();

    if (!lib_name)
        return 0;

    if ( !(external_d3d_lib = wine_dlopen(lib_name, RTLD_LAZY | RTLD_NOLOAD, NULL, 0)) )
    {
        if ( !(external_d3d_lib = wine_dlopen(lib_name, RTLD_LAZY | RTLD_LOCAL, NULL, 0)) )
        {
            ERR("External D3D Library %s could not be found\n", lib_name);
            return 0;
        } else {
            HDC hdc;

            pfn_native_core_create_d3d11_device = wine_dlsym(external_d3d_lib, "native_core_create_d3d11_device", NULL, 0);

            hdc = GetDC(0);
            vulkan_funcs = __wine_get_vulkan_driver(hdc, WINE_VULKAN_DRIVER_VERSION);
            ReleaseDC(0, hdc);
        }
    }
    
    return 1;
}

static VkResult create_vulkan_surface(VkInstance instance, void *window, VkSurfaceKHR *surface)
{
    HINSTANCE window_instance = (HINSTANCE) GetWindowLongPtrA(window, GWLP_HINSTANCE);

    VkWin32SurfaceCreateInfoKHR info;
    info.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext      = NULL;
    info.flags      = 0;
    info.hinstance  = window_instance;
    info.hwnd       = window;

    return vulkan_funcs->p_vkCreateWin32SurfaceKHR(instance, &info, NULL, surface);
}

static native_info info =
{
    NULL,
    create_vulkan_surface
};

HRESULT create_external_d3d11_device(IDXGIFactory *factory, IDXGIAdapter *adapter, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, ID3D11Device **device_out)
{
    info.pfn_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) vulkan_funcs->p_vkGetInstanceProcAddr;

    TRACE("Calling external D3D11 library's entry-point\n");
    return pfn_native_core_create_d3d11_device(info, factory, adapter, flags, feature_levels,
            levels, device_out);
}