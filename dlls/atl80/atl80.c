/*
 * Copyright 2012 Stefan Leichter
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

#define COBJMACROS

#include "atlbase.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(atl);

/***********************************************************************
 *           AtlRegisterTypeLib         [atl80.19]
 */
HRESULT WINAPI AtlRegisterTypeLib(HINSTANCE inst, const WCHAR *index)
{
    ITypeLib *typelib;
    BSTR path;
    HRESULT hres;

    TRACE("(%p %s)\n", inst, debugstr_w(index));

    hres = AtlLoadTypeLib(inst, index, &path, &typelib);
    if(FAILED(hres))
        return hres;

    hres = RegisterTypeLib(typelib, path, NULL); /* FIXME: pass help directory */
    ITypeLib_Release(typelib);
    SysFreeString(path);
    return hres;
}

/***********************************************************************
 *           AtlGetVersion              [atl80.@]
 */
DWORD WINAPI AtlGetVersion(void *pReserved)
{
   return _ATL_VER;
}
