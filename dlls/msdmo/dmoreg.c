/*
 * Copyright (C) 2003 Michael Günnewig
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
#include "winbase.h"
#include "objbase.h"
#include "mediaobj.h"
#include "dmoreg.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdmo);

/***********************************************************************/

HRESULT WINAPI DMORegister(LPCWSTR a, REFCLSID b, REFGUID c, DWORD d, DWORD e,
			   const DMO_PARTIAL_MEDIATYPE* f,
                           DWORD g, const DMO_PARTIAL_MEDIATYPE* h)
{
  FIXME("(%p,%p,%p,%lu,%lu,%p,%lu,%p),stub!\n",a,b,c,d,e,f,g,h);

  return E_NOTIMPL;
}

HRESULT WINAPI DMOUnregister(REFCLSID a,REFGUID b)
{
  FIXME("(%p,%p),stub!\n",a,b);

  return E_NOTIMPL;
}

HRESULT WINAPI DMOEnum(REFGUID guidCategory, DWORD dwFlags, DWORD cInTypes,
		       const DMO_PARTIAL_MEDIATYPE*pInTypes, DWORD cOutTypes,
		       const DMO_PARTIAL_MEDIATYPE*pOutTypes,IEnumDMO**ppEnum)
{
  FIXME("(%s,0x%lX,%lu,%p,%lu,%p,%p),stub!\n",debugstr_guid(guidCategory),
	dwFlags,cInTypes,pInTypes,cOutTypes,pOutTypes,ppEnum);

  if (guidCategory == NULL || ppEnum == NULL)
    return E_FAIL;
  if (dwFlags != 0 && dwFlags != DMO_ENUMF_INCLUDE_KEYED)
    return E_FAIL;

  *ppEnum = NULL;

  return E_NOTIMPL;
}

HRESULT WINAPI DMOGetTypes(REFCLSID a, unsigned long b, unsigned long* c,
			   DMO_PARTIAL_MEDIATYPE* d, unsigned long e,
			   unsigned long* f, DMO_PARTIAL_MEDIATYPE* g)
{
  FIXME("(%p,%lu,%p,%p,%lu,%p,%p),stub!\n",a,b,c,d,e,f,g);

  return E_NOTIMPL;
}

HRESULT WINAPI DMOGetName(REFCLSID pclsid, WCHAR* pstr)
{
  FIXME("(%s,%p),stub!\n", debugstr_guid(pclsid), pstr);

  if (pclsid == NULL || pstr == NULL)
    return E_FAIL;

  pstr[0] = '\0';

  return E_NOTIMPL;
}
