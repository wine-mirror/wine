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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "initguid.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wine_d3d);

int num_lock = 0;
void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;
int vs_mode = VS_HW;   /* Hardware by default */
int ps_mode = PS_NONE; /* Disabled by default */

IWineD3D* WINAPI WineDirect3DCreate(UINT SDKVersion, UINT dxVersion, IUnknown *parent) {
    IWineD3DImpl* object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DImpl));
    object->lpVtbl = &IWineD3D_Vtbl;
    object->dxVersion = dxVersion;
    object->ref = 1;
    object->parent = parent;

    TRACE("Created WineD3D object @ %p for d3d%d support\n", object, dxVersion);

    return (IWineD3D *)object;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("WineD3D DLLMain Reason=%ld\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       HMODULE mod;
       char buffer[32];
       DWORD size = sizeof(buffer);
       HKEY hkey = 0;

       DisableThreadLibraryCalls(hInstDLL);

       mod = GetModuleHandleA( "winex11.drv" );
       if (mod)
       {
           wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
           wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
       }
       /* @@ Wine registry key: HKLM\Software\Wine\Direct3D */
       if ( !RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Direct3D", &hkey) )
       {
           if ( !RegQueryValueExA( hkey, "VertexShaderMode", 0, NULL, buffer, &size) )
           {
               if (!strcmp(buffer,"none"))
               {
                   TRACE("Disable vertex shaders\n");
                   vs_mode = VS_NONE;
	       }
	       else if (!strcmp(buffer,"emulation"))
               {
                   TRACE("Force SW vertex shaders\n");
                   vs_mode = VS_SW;
               }
           }
           if ( !RegQueryValueExA( hkey, "PixelShaderMode", 0, NULL, buffer, &size) )
           {
               if (!strcmp(buffer,"enabled"))
               {
                   TRACE("Allow pixel shaders\n");
                   ps_mode = PS_HW;
	       }
           }
       }
       if (vs_mode == VS_HW)
           TRACE("Allow HW vertex shaders\n");
       if (ps_mode == PS_NONE)
           TRACE("Disable pixel shaders\n");
    }
    return TRUE;
}
