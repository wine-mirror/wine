/*
 * UrlMon
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
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

#include "windef.h"
#include "objbase.h"
#include "wine/debug.h"

#include "urlmon.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

/* native urlmon.dll uses this key, too */
static WCHAR BSCBHolder[] = { '_','B','S','C','B','_','H','o','l','d','e','r','_',0 };


/***********************************************************************
 *           CreateAsyncBindCtxEx (URLMON.@)
 *
 * not implemented
 *
 */
HRESULT WINAPI CreateAsyncBindCtxEx(IBindCtx *ibind, DWORD options,
    IBindStatusCallback *callback, IEnumFORMATETC *format, IBindCtx** pbind,
    DWORD reserved)
{
     FIXME("stub, returns failure\n");
     return E_INVALIDARG;
}


/***********************************************************************
 *           RegisterBindStatusCallback (URLMON.@)
 *
 * Register a bind status callback
 *
 * RETURNS
 *    S_OK 		success
 *    E_INVALIDARG  invalid argument(s)
 *    E_OUTOFMEMORY	out of memory 
 *
 */
HRESULT WINAPI RegisterBindStatusCallback(
    IBindCtx *pbc,
    IBindStatusCallback *pbsc,
    IBindStatusCallback **ppbscPrevious,
    DWORD dwReserved)
{
    IBindStatusCallback *prev;

	TRACE("(%p,%p,%p,%lu)\n", pbc, pbsc, ppbscPrevious, dwReserved);

    if (pbc == NULL || pbsc == NULL)
        return E_INVALIDARG;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, BSCBHolder, (IUnknown **)&prev)))
    {
        IBindCtx_RevokeObjectParam(pbc, BSCBHolder);
        if (ppbscPrevious)
            *ppbscPrevious = prev;
        else
            IBindStatusCallback_Release(prev);
    }
    
	return IBindCtx_RegisterObjectParam(pbc, BSCBHolder, (IUnknown *)pbsc);
}

/***********************************************************************
 *           RevokeBindStatusCallback (URLMON.@)
 *
 * Unregister a bind status callback
 *
 * RETURNS
 *    S_OK 		success
 *    E_INVALIDARG  invalid argument(s)
 *    E_FAIL pbsc wasn't registered with pbc
 *
 */
HRESULT WINAPI RevokeBindStatusCallback(
    IBindCtx *pbc,
    IBindStatusCallback *pbsc)
{
    IBindStatusCallback *callback;
    HRESULT hr = E_FAIL;

	TRACE("(%p,%p)\n", pbc, pbsc);

    if (pbc == NULL || pbsc == NULL)
        return E_INVALIDARG;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, BSCBHolder, (IUnknown **)&callback)))
    {
        if (callback == pbsc)
        {
            IBindCtx_RevokeObjectParam(pbc, BSCBHolder);
            hr = S_OK;
        }
        IBindStatusCallback_Release(pbsc);
    }

    return hr;
}

/***********************************************************************
 *           Extract (URLMON.@)
 *
 */
HRESULT WINAPI Extract(DWORD Param1, DWORD Param2)
{
   FIXME("%lx %lx\n", Param1, Param2);

   return E_NOTIMPL;
}
