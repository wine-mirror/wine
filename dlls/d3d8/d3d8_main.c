/* Direct3D 8
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "d3d8.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

void (*wine_tsx11_lock_ptr)(void) = NULL;
void (*wine_tsx11_unlock_ptr)(void) = NULL;

HRESULT WINAPI D3D8GetSWInfo(void)
{
    FIXME("(void): stub\n");
    return 0;
}

void DebugSetMute(void)
{
    /* nothing to do */
}

IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion)
{

    IDirect3D8Impl *object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3D8Impl));

    object->lpVtbl = &Direct3D8_Vtbl;
    object->ref = 1;

    TRACE("SDKVersion = %x, Created Direct3D object at %p\n", SDKVersion, object);

    return (IDirect3D8 *)object;
}

/* At process attach */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    TRACE("fdwReason=%ld\n", fdwReason);
       if (fdwReason == DLL_PROCESS_ATTACH)
       {
           HMODULE mod = GetModuleHandleA( "x11drv.dll" );
           if (mod)
           {
               wine_tsx11_lock_ptr   = (void *)GetProcAddress( mod, "wine_tsx11_lock" );
               wine_tsx11_unlock_ptr = (void *)GetProcAddress( mod, "wine_tsx11_unlock" );
           }
       }
    return TRUE;
}
