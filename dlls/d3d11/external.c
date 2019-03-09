#include "config.h"
#include "wine/port.h"

#include "d3d11_private.h"

#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

typedef HRESULT (WINAPI* PFN_D3D11_CORE_CREATE_DEVICE)(IDXGIFactory*,IDXGIAdapter*,UINT,const D3D_FEATURE_LEVEL*,
    UINT,ID3D11Device**);

static void* external_d3d_lib;
static PFN_D3D11_CORE_CREATE_DEVICE pfn_native_core_create_d3d11_device;

static char* get_desired_d3d_library(void)
{
    HKEY defkey;
    HKEY appkey;
    DWORD type, size;
    char buffer[MAX_PATH+10];
    DWORD len;

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

    if (defkey) RegQueryValueExA(defkey, "external_d3d", 0, &type, (BYTE *)external_d3d_lib_name, &size);
    if (type != REG_SZ && appkey)
        RegQueryValueExA(appkey, "external_d3d", 0, &type, (BYTE *)external_d3d_lib_name, &size);
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
            pfn_native_core_create_d3d11_device = wine_dlsym(external_d3d_lib, "D3D11CoreCreateDevice", NULL, 0);
        }
    }
    
    return 1;
}

HRESULT create_external_d3d11_device(IDXGIFactory *factory, IDXGIAdapter *adapter, UINT flags,
        const D3D_FEATURE_LEVEL *feature_levels, UINT levels, ID3D11Device **device_out)
{

    TRACE("Calling external D3D11 library's entry-point\n");
    return pfn_native_core_create_d3d11_device(factory, adapter, flags, feature_levels,
            levels, device_out);
}