/*
 * Implementation of class factory for IE Web Browser
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include <stdio.h>

#include "shdocvw.h"
#include "winreg.h"
#include "rpcproxy.h"
#include "isguids.h"

#include "winver.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the WebBrowser class factory
 */

static HRESULT get_ieframe_object(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HINSTANCE ieframe_instance;

    static HRESULT (WINAPI *ieframe_DllGetClassObject)(REFCLSID,REFIID,void**);

    if(!ieframe_DllGetClassObject) {
        ieframe_instance = get_ieframe_instance();
        if(!ieframe_instance)
            return CLASS_E_CLASSNOTAVAILABLE;

        ieframe_DllGetClassObject = (void*)GetProcAddress(ieframe_instance, "DllGetClassObject");
        if(!ieframe_DllGetClassObject)
            return CLASS_E_CLASSNOTAVAILABLE;
    }

    return ieframe_DllGetClassObject(rclsid, riid, ppv);
}

/*************************************************************************
 *              DllGetClassObject (SHDOCVW.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TRACE("\n");

    if(IsEqualGUID(&CLSID_WebBrowser, rclsid)
       || IsEqualGUID(&CLSID_WebBrowser_V1, rclsid)
       || IsEqualGUID(&CLSID_InternetShortcut, rclsid)
       || IsEqualGUID(&CLSID_CUrlHistory, rclsid)
       || IsEqualGUID(&CLSID_TaskbarList, rclsid))
        return get_ieframe_object(rclsid, riid, ppv);

    /* As a last resort, figure if the CLSID belongs to a 'Shell Instance Object' */
    return SHDOCVW_GetShellInstanceObjectClassObject(rclsid, riid, ppv);
}

/***********************************************************************
 *          DllRegisterServer (shdocvw.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("\n");
    return __wine_register_resources( shdocvw_hinstance, NULL );
}

/***********************************************************************
 *          DllUnregisterServer (shdocvw.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("\n");
    return __wine_unregister_resources( shdocvw_hinstance, NULL );
}

/******************************************************************
 *             IEWinMain            (SHDOCVW.101)
 *
 * Only returns on error.
 */
DWORD WINAPI IEWinMain(LPSTR szCommandLine, int nShowWindow)
{
    DWORD (WINAPI *pIEWinMain)(LPSTR,int);

    TRACE("%s %d\n", debugstr_a(szCommandLine), nShowWindow);

    pIEWinMain = (void*)GetProcAddress(get_ieframe_instance(), MAKEINTRESOURCEA(101));
    if(!pIEWinMain)
        ExitProcess(1);

    return pIEWinMain(szCommandLine, nShowWindow);
}
