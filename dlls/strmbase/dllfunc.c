/*
 * Strmbase DLL functions
 *
 * Copyright (C) 2005 Rolf Kalbermatter
 * Copyright (C) 2010 Aric Stewart, CodeWeavers
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
#include "config.h"

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winreg.h"
#include "objbase.h"
#include "uuids.h"
#include "strmif.h"

#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

extern const int g_cTemplates;
extern const FactoryTemplate g_Templates[];

HRESULT WINAPI AMovieSetupRegisterFilter2( const AMOVIESETUP_FILTER const * pFilter, IFilterMapper2  *pIFM2, BOOL  bRegister)
{
    if (!pFilter)
        return S_OK;

    if (bRegister)
    {
        {
            REGFILTER2 rf2;
            rf2.dwVersion = 1;
            rf2.dwMerit = pFilter->merit;
            rf2.u.s.cPins = pFilter->pins;
            rf2.u.s.rgPins = pFilter->pPin;

            return IFilterMapper2_RegisterFilter(pIFM2, pFilter->clsid, pFilter->name, NULL, &CLSID_LegacyAmFilterCategory, NULL, &rf2);
        }
    }
    else
       return IFilterMapper2_UnregisterFilter(pIFM2, &CLSID_LegacyAmFilterCategory, NULL, pFilter->clsid);
}

HRESULT WINAPI AMovieDllRegisterServer2(BOOL bRegister)
{
    HRESULT hr;
    int i;
    IFilterMapper2 *pIFM2 = NULL;

    hr = CoInitialize(NULL);

    TRACE("Getting IFilterMapper2\r\n");
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFilterMapper2, (void **)&pIFM2);

    for (i = 0; SUCCEEDED(hr) && i < g_cTemplates; i++)
        hr = AMovieSetupRegisterFilter2(g_Templates[i].m_pAMovieSetup_Filter, pIFM2, bRegister);

    /* release interface */
    if (pIFM2)
        IFilterMapper2_Release(pIFM2);

    /* and clear up */
    CoFreeUnusedLibraries();
    CoUninitialize();

    return hr;
}
