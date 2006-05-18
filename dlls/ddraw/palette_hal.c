/*	DirectDrawPalette HAL driver
 *
 * Copyright 2001 TransGaming Technologies Inc.
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
#include "winerror.h"
#include "wine/debug.h"

#include <assert.h>
#include <string.h>

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const IDirectDrawPaletteVtbl DDRAW_HAL_Palette_VTable;

/******************************************************************************
 *			IDirectDrawPalette
 */
HRESULT HAL_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					IDirectDrawImpl* pDD, DWORD dwFlags)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = pDD->local.lpGbl;
    DDHAL_CREATEPALETTEDATA data;
    HRESULT hr;

    hr = Main_DirectDrawPalette_Construct(This, pDD, dwFlags);
    if (FAILED(hr)) return hr;

    This->final_release = HAL_DirectDrawPalette_final_release;
    ICOM_INIT_INTERFACE(This, IDirectDrawPalette, DDRAW_HAL_Palette_VTable);

    /* initialize HAL palette */
    data.lpDD = dd_gbl;
    data.lpDDPalette = &This->global;
    data.lpColorTable = NULL;
    data.ddRVal = 0;
    data.CreatePalette = dd_gbl->lpDDCBtmp->HALDD.CreatePalette;
    if (data.CreatePalette)
	data.CreatePalette(&data);

    return DD_OK;
}

HRESULT
HAL_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
			     LPDIRECTDRAWPALETTE* ppPalette,
			     LPUNKNOWN pUnkOuter)
{
    IDirectDrawPaletteImpl* This;
    HRESULT hr;

    if (pUnkOuter != NULL)
	return CLASS_E_NOAGGREGATION; /* unchecked */

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (This == NULL) return E_OUTOFMEMORY;

    hr = HAL_DirectDrawPalette_Construct(This, pDD, dwFlags);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppPalette = ICOM_INTERFACE(This, IDirectDrawPalette);

    return hr;
}

HRESULT WINAPI
HAL_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				 DWORD dwStart, DWORD dwCount,
				 LPPALETTEENTRY palent)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->local.lpDD_lcl->lpGbl;
    DDHAL_SETENTRIESDATA data;

    TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",This,dwFlags,dwStart,dwCount,
	  palent);

    data.lpDD = dd_gbl;
    data.lpDDPalette = &This->global;
    data.dwBase = dwStart;
    data.dwNumEntries = dwCount;
    data.lpEntries = palent;
    data.ddRVal = 0;
    data.SetEntries = dd_gbl->lpDDCBtmp->HALDDPalette.SetEntries;
    if (data.SetEntries)
	data.SetEntries(&data);

    return Main_DirectDrawPalette_SetEntries(iface, dwFlags, dwStart, dwCount, palent);
}

void HAL_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This)
{
    LPDDRAWI_DIRECTDRAW_GBL dd_gbl = This->local.lpDD_lcl->lpGbl;
    DDHAL_DESTROYPALETTEDATA data;

    /* destroy HAL palette */
    data.lpDD = dd_gbl;
    data.lpDDPalette = &This->global;
    data.ddRVal = 0;
    data.DestroyPalette = dd_gbl->lpDDCBtmp->HALDDPalette.DestroyPalette;
    if (data.DestroyPalette)
	data.DestroyPalette(&data);

    Main_DirectDrawPalette_final_release(This);
}

static const IDirectDrawPaletteVtbl DDRAW_HAL_Palette_VTable =
{
    Main_DirectDrawPalette_QueryInterface,
    Main_DirectDrawPalette_AddRef,
    Main_DirectDrawPalette_Release,
    Main_DirectDrawPalette_GetCaps,
    Main_DirectDrawPalette_GetEntries,
    Main_DirectDrawPalette_Initialize,
    HAL_DirectDrawPalette_SetEntries
};
