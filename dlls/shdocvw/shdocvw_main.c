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

#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "ole2.h"
#include "shlwapi.h"

#include "shdocvw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

HINSTANCE shdocvw_hinstance = 0;
HMODULE SHDOCVW_hshell32 = 0;

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

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("\n");

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
