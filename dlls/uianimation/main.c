/*
 * Uianimation main file.
 *
 * Copyright (C) 2018 Louis Lenders
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "uianimation.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uianimation);

static HINSTANCE hinstance;

BOOL WINAPI DllMain( HINSTANCE dll, DWORD reason, LPVOID reserved )
{
    TRACE("(%p %d %p)\n", dll, reason, reserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        hinstance = dll;
        DisableThreadLibraryCalls( dll );
        break;
    }
    return TRUE;
}

/******************************************************************
 *             DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **obj )
{
    FIXME( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *              DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    TRACE( "()\n" );
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer( void )
{
    return __wine_register_resources( hinstance );
}

/***********************************************************************
 *          DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer( void )
{
    return __wine_unregister_resources( hinstance );
}
