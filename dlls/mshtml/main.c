/*
 *    MSHTML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2003 Mike McCormack
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"

#include "uuids.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#include "initguid.h"

DEFINE_GUID( CLSID_MozillaBrowser, 0x1339B54C,0x3453,0x11D2,0x93,0xB9,0x00,0x00,0x00,0x00,0x00,0x00);

typedef HRESULT (WINAPI *fnGetClassObject)(REFCLSID rclsid, REFIID iid, LPVOID *ppv);
typedef BOOL (WINAPI *fnCanUnloadNow)();

HMODULE hMozCtl;


/* convert a guid to a wide character string */
static void MSHTML_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

static BOOL MSHTML_GetMozctlPath( LPWSTR szPath, DWORD sz )
{
    DWORD r, type;
    BOOL ret = FALSE;
    HKEY hkey;
    const WCHAR szPre[] = {
        'S','o','f','t','w','a','r','e','\\',
        'C','l','a','s','s','e','s','\\',
        'C','L','S','I','D','\\',0 };
    const WCHAR szPost[] = {
        '\\','I','n','p','r','o','c','S','e','r','v','e','r','3','2',0 };
    WCHAR szRegPath[(sizeof(szPre)+sizeof(szPost))/sizeof(WCHAR)+40];

    strcpyW( szRegPath, szPre );
    MSHTML_guid2wstr( &CLSID_MozillaBrowser, &szRegPath[strlenW(szRegPath)] );
    strcatW( szRegPath, szPost );

    TRACE("key = %s\n", debugstr_w( szRegPath ) );

    r = RegOpenKeyW( HKEY_LOCAL_MACHINE, szRegPath, &hkey );
    if( r != ERROR_SUCCESS )
        return FALSE;

    r = RegQueryValueExW( hkey, NULL, NULL, &type, (LPBYTE)szPath, &sz );
    ret = ( r == ERROR_SUCCESS ) && ( type == REG_SZ );
    RegCloseKey( hkey );

    return ret;
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    WCHAR szPath[MAX_PATH];

    switch(fdwReason) {
        case DLL_PROCESS_ATTACH:
            if( !MSHTML_GetMozctlPath( szPath, sizeof szPath ) )
            {
                MESSAGE("You need to install the Mozilla ActiveX control to\n");
                MESSAGE("use Wine's builtin MSHTML dll.\n");
                return FALSE;
            }
            hMozCtl = LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
            if( !hMozCtl )
            {
                ERR("Can't load the Mozilla ActiveX control\n");
                return FALSE;
            }
	    break;
	case DLL_PROCESS_DETACH:
            FreeLibrary( hMozCtl );
	    break;
    }
    return TRUE;
}

HRESULT WINAPI MSHTML_DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    HRESULT r;
    fnGetClassObject pGetClassObject;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv );

    if( !IsEqualGUID( &CLSID_HTMLDocument, rclsid ) )
        WARN("Unknown class %s\n", debugstr_guid(rclsid) );

    pGetClassObject = (fnGetClassObject) GetProcAddress( hMozCtl, "DllGetClassObject" );
    if( !pGetClassObject )
        return CLASS_E_CLASSNOTAVAILABLE;
    r = pGetClassObject( &CLSID_MozillaBrowser, iid, ppv );

    TRACE("r = %08lx  *ppv = %p\n", r, *ppv );

    return S_OK;
}

BOOL WINAPI MSHTML_DllCanUnloadNow(void)
{
    fnCanUnloadNow pCanUnloadNow;
    BOOL r;

    TRACE("\n");

    pCanUnloadNow = (fnCanUnloadNow) GetProcAddress( hMozCtl, "DllCanUnloadNow" );
    if( !pCanUnloadNow )
        return FALSE;
    r = pCanUnloadNow();

    TRACE("r = %d\n", r);

    return r;
}

/* appears to have the same prototype as WinMain */
INT WINAPI RunHTMLApplication( HINSTANCE hinst, HINSTANCE hPrevInst,
                               LPCSTR szCmdLine, INT nCmdShow )
{
    FIXME("%p %p %s %d\n", hinst, hPrevInst, debugstr_a(szCmdLine), nCmdShow );
    return 0;
}
