/*
 *    MSXML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2005 Mike McCormack
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "msxml2.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef HAVE_LIBXML2
        xmlInitParser();
#endif
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
#ifdef HAVE_LIBXML2
        xmlCleanupParser();
#endif
        break;
    }
    return TRUE;
}

static HRESULT add_key_val( LPCSTR key, LPCSTR valname, LPCSTR value )
{
    HKEY hkey;

    if (RegCreateKeyA( HKEY_CLASSES_ROOT, key, &hkey ) != ERROR_SUCCESS) return E_FAIL;
    RegSetValueA( hkey, valname, REG_SZ, value, strlen( value ) + 1 );
    RegCloseKey( hkey );
    return S_OK;
}

HRESULT WINAPI DllRegisterServer(void)
{
    LONG r;

    r = add_key_val( "CLSID\\{2933BF90-7B36-11D2-B20E-00C04F983E60}",
                     NULL,
                     "XML DOM Document" );
    r = add_key_val( "CLSID\\{2933BF90-7B36-11D2-B20E-00C04F983E60}\\InProcServer32",
                     NULL,
                     "msxml3.dll" );
    return r;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    RegDeleteKeyA( HKEY_CLASSES_ROOT, "CLSID\\{2933BF90-7B36-11D2-B20E-00C04F983E60}\\InProcServer32" );
    RegDeleteKeyA( HKEY_CLASSES_ROOT, "CLSID\\{2933BF90-7B36-11D2-B20E-00C04F983E60}" );
    return S_OK;
}
