/*
 * Direct3D 9
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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
 *
 */

#include "config.h"

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "d3d9.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);


void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;


HRESULT WINAPI D3D9GetSWInfo(void) {
    FIXME("(void): stub\n");
    return 0;
}

void WINAPI DebugSetMute(void) {
    /* nothing to do */
}

IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) {
    IDirect3D9Impl* object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3D9Impl));

    object->lpVtbl = &Direct3D9_Vtbl;
    object->ref = 1;

    TRACE("SDKVersion = %x, Created Direct3D object at %p\n", SDKVersion, object);

    return (IDirect3D9*) object;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv) {
    TRACE("fdwReason=%ld\n", fdwReason);
    if (fdwReason == DLL_PROCESS_ATTACH) {
        HMODULE mod;

        DisableThreadLibraryCalls(hInstDLL);

        mod = GetModuleHandleA( "x11drv.dll" );
        if (mod)
        {
            wine_tsx11_lock_ptr   = (void*) GetProcAddress(mod, "wine_tsx11_lock");
            wine_tsx11_unlock_ptr = (void*) GetProcAddress(mod, "wine_tsx11_unlock");
        }
    }
    return TRUE;
}

/***********************************************************************
 *		ValidateVertexShader (D3D9.@)
 */
BOOL WINAPI ValidateVertexShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p %p\n", what, toto);
  return TRUE;
}

/***********************************************************************
 *		ValidatePixelShader (D3D9.@)
 */
BOOL WINAPI ValidatePixelShader(LPVOID what, LPVOID toto) {
  FIXME("(void): stub: %p %p\n", what, toto);
  return TRUE;
}
