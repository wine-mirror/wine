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
#include <string.h>
#include "winreg.h"
#include "initguid.h"
#include "ole2.h"
#include "shlwapi.h"

#include "shdocvw.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/***********************************************************************
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

/***********************************************************************
 *		SHDOCVW_110 (SHDOCVW.110)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI SHDOCVW_110(void)
{
    FIXME("(), stub!\n");
    return 0x0deadfeed;
}

/***********************************************************************
 *		SHDOCVW_118 (SHDOCVW.118)
 *
 * Called by Win98 explorer.exe main binary, definitely has only one
 * parameter.
 */
DWORD WINAPI SHDOCVW_118(DWORD dw1)
{
    FIXME("(%08lx), stub!\n", dw1);
    return 0xcafedead;
}

/***********************************************************************
 *		SHDOCVW_125 (SHDOCVW.125)
 *
 * Called by Win98 explorer.exe main binary, definitely has 0
 * parameters.
 */
DWORD WINAPI SHDOCVW_125(void)
{
    FIXME("(), stub!\n");
    return 0x0deadbee;
}
