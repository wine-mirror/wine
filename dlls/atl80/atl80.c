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
 *           AtlRegisterTypeLib         [atl80.18]
 */
HRESULT WINAPI AtlComModuleRegisterServer(_ATL_COM_MODULE *mod, BOOL bRegTypeLib, const CLSID *clsid)
{
    const struct _ATL_CATMAP_ENTRY *catmap;
    _ATL_OBJMAP_ENTRY **iter;
    HRESULT hres;

    TRACE("(%p %x %s)\n", mod, bRegTypeLib, debugstr_guid(clsid));

    for(iter = mod->m_ppAutoObjMapFirst; iter < mod->m_ppAutoObjMapLast; iter++) {
        if(!*iter || (clsid && !IsEqualCLSID((*iter)->pclsid, clsid)))
            continue;

        TRACE("Registering clsid %s\n", debugstr_guid((*iter)->pclsid));
        hres = (*iter)->pfnUpdateRegistry(TRUE);
        if(FAILED(hres))
            return hres;

        catmap = (*iter)->pfnGetCategoryMap();
        if(catmap) {
            hres = AtlRegisterClassCategoriesHelper((*iter)->pclsid, catmap, TRUE);
            if(FAILED(hres))
                return hres;
        }
    }

    if(bRegTypeLib) {
        hres = AtlRegisterTypeLib(mod->m_hInstTypeLib, NULL);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

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
