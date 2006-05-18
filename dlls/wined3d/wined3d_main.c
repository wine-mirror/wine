/*
 * Direct3D wine internal interface main
 *
 * Copyright 2002-2003 The wine-d3d team
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004      Jason Edmeades
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

#include "initguid.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wine_d3d);

int num_lock = 0;
void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;


wined3d_settings_t wined3d_settings = 
{
  VS_HW,   /* Hardware by default */
  PS_NONE, /* Disabled by default */
  VBO_HW,   /* Hardware by default */
  FALSE    /* Use of GLSL disabled by default */
};

WineD3DGlobalStatistics *wineD3DGlobalStatistics = NULL;
CRITICAL_SECTION resourceStoreCriticalSection;

long globalChangeGlRam(long glram){
    /* FIXME: replace this function with object tracking */
    int result;

    EnterCriticalSection(&resourceStoreCriticalSection); /* this is overkill really, but I suppose it should be thread safe */
    wineD3DGlobalStatistics->glsurfaceram     += glram;
    TRACE("Adjusted gl ram by %ld to %d\n", glram, wineD3DGlobalStatistics->glsurfaceram);
    result = wineD3DGlobalStatistics->glsurfaceram;
    LeaveCriticalSection(&resourceStoreCriticalSection);
    return result;

}

IWineD3D* WINAPI WineDirect3DCreate(UINT SDKVersion, UINT dxVersion, IUnknown *parent) {
    IWineD3DImpl* object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DImpl));
    object->lpVtbl = &IWineD3D_Vtbl;
    object->dxVersion = dxVersion;
    object->ref = 1;
    object->parent = parent;

    /* TODO: Move this off to device and possibly x11drv */
    /* Create a critical section for a dll global data store */
    InitializeCriticalSectionAndSpinCount(&resourceStoreCriticalSection, 0x80000400);

    /*Create a structure for storing global data in*/
    if(wineD3DGlobalStatistics == NULL){
        TRACE("Createing global statistics store\n");
        wineD3DGlobalStatistics = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wineD3DGlobalStatistics));

    }


    TRACE("Created WineD3D object @ %p for d3d%d support\n", object, dxVersion);

    return (IWineD3D *)object;
}

inline static DWORD get_config_key(HKEY defkey, HKEY appkey, const char* name, char* buffer, DWORD size)
{
    if (0 != appkey && !RegQueryValueExA( appkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    if (0 != defkey && !RegQueryValueExA( defkey, name, 0, NULL, (LPBYTE) buffer, &size )) return 0;
    return ERROR_FILE_NOT_FOUND;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("WineD3D DLLMain Reason=%ld\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       HMODULE mod;
       char buffer[MAX_PATH+10];
       DWORD size = sizeof(buffer);
       HKEY hkey = 0;
       HKEY appkey = 0;
       DWORD len;

       DisableThreadLibraryCalls(hInstDLL);

       mod = GetModuleHandleA( "winex11.drv" );
       if (mod)
       {
           wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
           wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
       }
       /* @@ Wine registry key: HKCU\Software\Wine\Direct3D */
       if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Direct3D", &hkey ) ) hkey = 0;

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

       if ( 0 != hkey || 0 != appkey )
       {
            if ( !get_config_key( hkey, appkey, "VertexShaderMode", buffer, size) )
            {
                if (!strcmp(buffer,"none"))
                {
                    TRACE("Disable vertex shaders\n");
                    wined3d_settings.vs_mode = VS_NONE;
                }
                else if (!strcmp(buffer,"emulation"))
                {
                    TRACE("Force SW vertex shaders\n");
                    wined3d_settings.vs_mode = VS_SW;
                }
            }
            if ( !get_config_key( hkey, appkey, "PixelShaderMode", buffer, size) )
            {
                if (!strcmp(buffer,"enabled"))
                {
                    TRACE("Allow pixel shaders\n");
                    wined3d_settings.ps_mode = PS_HW;
                }
                if (!strcmp(buffer,"disabled"))
                {
                    TRACE("Disable pixel shaders\n");
                    wined3d_settings.ps_mode = PS_NONE;
                }
            }
            if ( !get_config_key( hkey, appkey, "VertexBufferMode", buffer, size) )
            {
                if (!strcmp(buffer,"none"))
                {
                    TRACE("Disable Vertex Buffer Hardware support\n");
                    wined3d_settings.vbo_mode = VBO_NONE;
                }
                else if (!strcmp(buffer,"hardware"))
                {
                    TRACE("Allow Vertex Buffer Hardware support\n");
                    wined3d_settings.vbo_mode = VBO_HW;
                }
            }
            if ( !get_config_key( hkey, appkey, "UseGLSL", buffer, size) )
            {
                if (!strcmp(buffer,"enabled"))
                {
                    TRACE("Use of GL Shading Language enabled for systems that support it\n");
                    wined3d_settings.glslRequested = TRUE;
                }
                else
                {
                    TRACE("Use of GL Shading Language disabled\n");
                }
            }
            if ( !get_config_key( hkey, appkey, "Nonpower2Mode", buffer, size) )
            {
                if (!strcmp(buffer,"none"))
                {
                    TRACE("Using default non-power2 textures\n");
                    wined3d_settings.nonpower2_mode = NP2_NONE;

                }
                else if (!strcmp(buffer,"repack"))
                {
                    TRACE("Repacking non-power2 textre\n");
                    wined3d_settings.nonpower2_mode = NP2_REPACK;
                }
                /* There will be a couple of other choices for nonpow2, they are: TextureRecrangle and OpenGL 2 */
            }
       }
       if (wined3d_settings.vs_mode == VS_HW)
           TRACE("Allow HW vertex shaders\n");
       if (wined3d_settings.ps_mode == PS_NONE)
           TRACE("Disable pixel shaders\n");
       if (wined3d_settings.vbo_mode == VBO_NONE)
           TRACE("Disable Vertex Buffer Hardware support\n");
       if (wined3d_settings.glslRequested)
           TRACE("If supported by your system, GL Shading Language will be used\n");
       if (wined3d_settings.nonpower2_mode == NP2_REPACK)
           TRACE("Repacking non-power2 textures\n");

       if (appkey) RegCloseKey( appkey );
       if (hkey) RegCloseKey( hkey );
    }
    return TRUE;
}
