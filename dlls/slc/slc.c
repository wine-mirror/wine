/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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
#include "wine/debug.h"

#include "slpublic.h"
#include "slerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(slc);

HRESULT WINAPI SLGetWindowsInformation(LPCWSTR name, SLDATATYPE *type, UINT *val, LPBYTE *size)
{
    FIXME("(%s %p %p %p) stub\n", debugstr_w(name), type, val, size );

    return SL_E_RIGHT_NOT_GRANTED;
}

HRESULT WINAPI SLGetWindowsInformationDWORD(LPCWSTR lpszValueName, LPDWORD pdwValue)
{
    FIXME("(%s) stub\n", debugstr_w(lpszValueName) );

    return SL_E_RIGHT_NOT_GRANTED;
}

/***********************************************************************
 *             DllMain   (CLUSAPI.@)
 *
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}
