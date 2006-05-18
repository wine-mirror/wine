/*		DirectDraw - IDirectPalette base interface
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 2000-2001 TransGaming Technologies Inc.
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

#define SIZE_BITS (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT)

/* For unsigned x. 0 is not a power of 2. */
#define IS_POW_2(x) (((x) & ((x) - 1)) == 0)

static const IDirectDrawPaletteVtbl DDRAW_Main_Palette_VTable;

/******************************************************************************
 *			IDirectDrawPalette
 */
HRESULT Main_DirectDrawPalette_Construct(IDirectDrawPaletteImpl* This,
					 IDirectDrawImpl* pDD, DWORD dwFlags)
{
    if (!IS_POW_2(dwFlags & SIZE_BITS)) return DDERR_INVALIDPARAMS;

    if (dwFlags & DDPCAPS_8BITENTRIES)
	WARN("creating palette with 8 bit entries\n");

    This->palNumEntries = Main_DirectDrawPalette_Size(dwFlags);
    This->ref = 1;

    This->local.lpGbl = &This->global;
    This->local.lpDD_lcl = &pDD->local;
    This->global.lpDD_lcl = &pDD->local;
    This->global.dwProcessId = GetCurrentProcessId();
    This->global.dwFlags = dwFlags;

    This->final_release = Main_DirectDrawPalette_final_release;
    ICOM_INIT_INTERFACE(This, IDirectDrawPalette, DDRAW_Main_Palette_VTable);

    /* we could defer hpal creation until we need it,
     * but does anyone have a case where it would be useful? */
    This->hpal = CreatePalette((const LOGPALETTE*)&(This->palVersion));

    Main_DirectDraw_AddPalette(pDD, This);

    return DD_OK;
}

HRESULT
Main_DirectDrawPalette_Create(IDirectDrawImpl* pDD, DWORD dwFlags,
			      LPDIRECTDRAWPALETTE* ppPalette,
			      LPUNKNOWN pUnkOuter)
{
    IDirectDrawPaletteImpl* This;
    HRESULT hr;

    if (pUnkOuter != NULL)
	return CLASS_E_NOAGGREGATION; /* unchecked */

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (This == NULL) return E_OUTOFMEMORY;

    hr = Main_DirectDrawPalette_Construct(This, pDD, dwFlags);
    if (FAILED(hr))
	HeapFree(GetProcessHeap(), 0, This);
    else
	*ppPalette = ICOM_INTERFACE(This, IDirectDrawPalette);

    return hr;
}

DWORD Main_DirectDrawPalette_Size(DWORD dwFlags)
{
    switch (dwFlags & SIZE_BITS)
    {
    case DDPCAPS_1BIT: return 2;
    case DDPCAPS_2BIT: return 4;
    case DDPCAPS_4BIT: return 16;
    case DDPCAPS_8BIT: return 256;
    default: assert(0); return 256;
    }
}

HRESULT WINAPI
Main_DirectDrawPalette_GetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;

    TRACE("(%p)->GetEntries(%08lx,%ld,%ld,%p)\n",This,dwFlags,dwStart,dwCount,
	  palent);

    if (dwFlags != 0) return DDERR_INVALIDPARAMS; /* unchecked */
    if (dwStart + dwCount > Main_DirectDrawPalette_Size(This->global.dwFlags))
	return DDERR_INVALIDPARAMS;

    if (This->global.dwFlags & DDPCAPS_8BITENTRIES)
    {
	unsigned int i;
	LPBYTE entry = (LPBYTE)palent;

	for (i=dwStart; i < dwCount+dwStart; i++)
	    *entry++ = This->palents[i].peRed;
    }
    else
	memcpy(palent, This->palents+dwStart, dwCount * sizeof(PALETTEENTRY));

    return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;

    TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",This,dwFlags,dwStart,dwCount,
	  palent);

    if (This->global.dwFlags & DDPCAPS_8BITENTRIES)
    {
	unsigned int i;
	const BYTE* entry = (const BYTE*)palent;

	for (i=dwStart; i < dwCount+dwStart; i++)
	    This->palents[i].peRed = *entry++;
    }
    else {
	memcpy(This->palents+dwStart, palent, dwCount * sizeof(PALETTEENTRY));

	if (This->hpal)
	    SetPaletteEntries(This->hpal, dwStart, dwCount, This->palents+dwStart);

	if (This->global.dwFlags & DDPCAPS_PRIMARYSURFACE) {
	    /* update physical palette */
	    LPDIRECTDRAWSURFACE7 psurf = NULL;
	    IDirectDraw7_GetGDISurface(ICOM_INTERFACE(This->ddraw_owner,IDirectDraw7), &psurf);
	    if (psurf) {
		IDirectDrawSurfaceImpl *surf = ICOM_OBJECT(IDirectDrawSurfaceImpl,
							   IDirectDrawSurface7, psurf);
		surf->update_palette(surf, This, dwStart, dwCount, palent);
		IDirectDrawSurface7_Release(psurf);
	    }
	    else ERR("can't find GDI surface!!\n");
	}
    }

#if 0
    /* Now, if we are in 'depth conversion mode', update the screen palette */
    /* FIXME: we need to update the image or we won't get palette fading. */
    if (This->ddraw->d->palette_convert != NULL)
	This->ddraw->d->palette_convert(palent,This->screen_palents,start,count);
#endif

    return DD_OK;
}

void Main_DirectDrawPalette_final_release(IDirectDrawPaletteImpl* This)
{
    Main_DirectDraw_RemovePalette(This->ddraw_owner, This);

    if (This->hpal) DeleteObject(This->hpal);
}

static void Main_DirectDrawPalette_Destroy(IDirectDrawPaletteImpl* This)
{
    This->final_release(This);

    if (This->private != This+1)
	HeapFree(GetProcessHeap(), 0, This->private);

    HeapFree(GetProcessHeap(),0,This);
}

void Main_DirectDrawPalette_ForceDestroy(IDirectDrawPaletteImpl* This)
{
    WARN("deleting palette %p with refcnt %lu\n", This, This->ref);
    Main_DirectDrawPalette_Destroy(This);
}

ULONG WINAPI
Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %lu.\n", This, ref + 1);

    if (!ref)
    {
	Main_DirectDrawPalette_Destroy(This);
	return 0;
    }

    return ref;
}

ULONG WINAPI Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface) {
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %lu.\n", This, ref - 1);

    return ref;
}

HRESULT WINAPI
Main_DirectDrawPalette_Initialize(LPDIRECTDRAWPALETTE iface,
				  LPDIRECTDRAW ddraw, DWORD dwFlags,
				  LPPALETTEENTRY palent)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    TRACE("(%p)->(%p,%ld,%p)\n", This, ddraw, dwFlags, palent);
    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI
Main_DirectDrawPalette_GetCaps(LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps)
{
   IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
   TRACE("(%p)->(%p)\n",This,lpdwCaps);

   *lpdwCaps = This->global.dwFlags;

   return DD_OK;
}

HRESULT WINAPI
Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface,
				      REFIID refiid, LPVOID *obj)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);

    if (IsEqualGUID(refiid, &IID_IUnknown)
	|| IsEqualGUID(refiid, &IID_IDirectDrawPalette))
    {
	*obj = iface;
	IDirectDrawPalette_AddRef(iface);
	return S_OK;
    }
    else
    {
	return E_NOINTERFACE;
    }
}

static const IDirectDrawPaletteVtbl DDRAW_Main_Palette_VTable =
{
    Main_DirectDrawPalette_QueryInterface,
    Main_DirectDrawPalette_AddRef,
    Main_DirectDrawPalette_Release,
    Main_DirectDrawPalette_GetCaps,
    Main_DirectDrawPalette_GetEntries,
    Main_DirectDrawPalette_Initialize,
    Main_DirectDrawPalette_SetEntries
};
