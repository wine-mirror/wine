/*
 * SHDOCVW - Internet Explorer Web Control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "shlwapi.h"

#include "shdocvw.h"
#include "uuids.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);


DEFINE_GUID( CLSID_MozillaBrowser, 0x1339B54C,0x3453,0x11D2,0x93,0xB9,0x00,0x00,0x00,0x00,0x00,0x00);

typedef HRESULT (WINAPI *fnGetClassObject)(REFCLSID rclsid, REFIID iid, LPVOID *ppv);

HINSTANCE shdocvw_hinstance = 0;
static HMODULE SHDOCVW_hshell32 = 0;
static HMODULE hMozCtl = (HMODULE)~0UL;


/* convert a guid to a wide character string */
static void SHDOCVW_guid2wstr( const GUID *guid, LPWSTR wstr )
{
    char str[40];

    sprintf(str, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           guid->Data1, guid->Data2, guid->Data3,
           guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
           guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    MultiByteToWideChar( CP_ACP, 0, str, -1, wstr, 40 );
}

static BOOL SHDOCVW_GetMozctlPath( LPWSTR szPath, DWORD sz )
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
    SHDOCVW_guid2wstr( &CLSID_MozillaBrowser, &szRegPath[strlenW(szRegPath)] );
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

/*************************************************************************
 * SHDOCVW DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
	TRACE("%p 0x%lx %p\n", hinst, fdwReason, fImpLoad);
	switch (fdwReason)
	{
	  case DLL_PROCESS_ATTACH:
	    shdocvw_hinstance = hinst;
	    break;
	  case DLL_PROCESS_DETACH:
	    if (SHDOCVW_hshell32) FreeLibrary(SHDOCVW_hshell32);
	    if (hMozCtl && hMozCtl != (HMODULE)~0UL) FreeLibrary(hMozCtl);
	    break;
	}
	return TRUE;
}

/*************************************************************************
 *              DllCanUnloadNow (SHDOCVW.@)
 */
HRESULT WINAPI SHDOCVW_DllCanUnloadNow(void)
{
    FIXME("(void): stub\n");

    return S_FALSE;
}


static BOOL SHDOCVW_TryLoadMozillaControl()
{
    WCHAR szPath[MAX_PATH];

    if( hMozCtl != (HMODULE)~0UL )
        return hMozCtl ? TRUE : FALSE;

    if( !SHDOCVW_GetMozctlPath( szPath, sizeof szPath ) )
    {
        MESSAGE("You need to install the Mozilla ActiveX control to\n");
        MESSAGE("use Wine's builtin CLSID_WebBrowser from SHDOCVW.DLL\n");
        return FALSE;
    }
    hMozCtl = LoadLibraryExW(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if( !hMozCtl )
    {
        ERR("Can't load the Mozilla ActiveX control\n");
        return FALSE;
    }
    return TRUE;
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("\n");

    if( IsEqualGUID( &CLSID_WebBrowser, rclsid ) &&
        SHDOCVW_TryLoadMozillaControl() )
    {
        HRESULT r;
        fnGetClassObject pGetClassObject;

        TRACE("WebBrowser class %s\n", debugstr_guid(rclsid) );

        pGetClassObject = (fnGetClassObject)
            GetProcAddress( hMozCtl, "DllGetClassObject" );

        if( !pGetClassObject )
            return CLASS_E_CLASSNOTAVAILABLE;
        r = pGetClassObject( &CLSID_MozillaBrowser, riid, ppv );

        TRACE("r = %08lx  *ppv = %p\n", r, *ppv );

        return r;
    }

    if (IsEqualGUID(&IID_IClassFactory, riid))
    {
        /* Pass back our shdocvw class factory */
        *ppv = (LPVOID)&SHDOCVW_ClassFactory;
        IClassFactory_AddRef((IClassFactory*)&SHDOCVW_ClassFactory);

        return S_OK;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllGetVersion (SHDOCVW.@)
 */
HRESULT WINAPI SHDOCVW_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    FIXME("(void): stub\n");
    return S_FALSE;
}

/*************************************************************************
 *              DllInstall (SHDOCVW.@)
 */
HRESULT WINAPI SHDOCVW_DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
   FIXME("(%s, %s): stub!\n", bInstall ? "TRUE":"FALSE", debugstr_w(cmdline));

   return S_OK;
}

/*************************************************************************
 * SHDOCVW_LoadShell32
 *
 * makes sure the handle to shell32 is valid
 */
 BOOL SHDOCVW_LoadShell32(void)
{
     if (SHDOCVW_hshell32)
       return TRUE;
     return ((SHDOCVW_hshell32 = LoadLibraryA("shell32.dll")) != NULL);
}

/***********************************************************************
 *		@ (SHDOCVW.110)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI WinList_Init(void)
{
    FIXME("(), stub!\n");
    return 0x0deadfeed;
}

/***********************************************************************
 *		@ (SHDOCVW.118)
 *
 * Called by Win98 explorer.exe main binary, definitely has only one
 * parameter.
 */
static BOOL (WINAPI *pShellDDEInit)(BOOL start) = NULL;

BOOL WINAPI ShellDDEInit(BOOL start)
{
    TRACE("(%d)\n", start);

    if (!pShellDDEInit)
    {
      if (!SHDOCVW_LoadShell32())
        return FALSE;
      pShellDDEInit = GetProcAddress(SHDOCVW_hshell32, (LPCSTR)188);
    }

    if (pShellDDEInit)
      return pShellDDEInit(start);
    else
      return FALSE;
}

/***********************************************************************
 *		@ (SHDOCVW.125)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI RunInstallUninstallStubs(void)
{
    FIXME("(), stub!\n");
    return 0x0deadbee;
}
