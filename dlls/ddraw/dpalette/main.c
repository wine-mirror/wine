/*		DirectDraw - IDirectPalette base interface
 *
 * Copyright 1997-2000 Marcus Meissner
 */

#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "ddraw_private.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *			IDirectDrawPalette
 */
HRESULT WINAPI IDirectDrawPaletteImpl_GetEntries(
    LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    int	i;

    TRACE("(%p)->GetEntries(%08lx,%ld,%ld,%p)\n",This,x,start,count,palent);

    for (i=0;i<count;i++) {
	palent[i].peRed   = This->palents[start+i].peRed;
	palent[i].peBlue  = This->palents[start+i].peBlue;
	palent[i].peGreen = This->palents[start+i].peGreen;
	palent[i].peFlags = This->palents[start+i].peFlags;
    }
    return DD_OK;
}

HRESULT WINAPI IDirectDrawPaletteImpl_SetEntries(
    LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    int		i;

    TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n", This,x,start,count,palent);
    for (i=0;i<count;i++) {
	This->palents[start+i].peRed = palent[i].peRed;
	This->palents[start+i].peBlue = palent[i].peBlue;
	This->palents[start+i].peGreen = palent[i].peGreen;
	This->palents[start+i].peFlags = palent[i].peFlags;
    }

    /* Now, if we are in 'depth conversion mode', update the screen palette */
    /* FIXME: we need to update the image or we won't get palette fading. */
    if (This->ddraw->d->palette_convert != NULL)
	This->ddraw->d->palette_convert(palent,This->screen_palents,start,count);
    return DD_OK;
}

ULONG WINAPI IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE iface) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );
    if (!--(This->ref)) {
	    HeapFree(GetProcessHeap(),0,This);
	    return S_OK;
    }
    return This->ref;
}

ULONG WINAPI IDirectDrawPaletteImpl_AddRef(LPDIRECTDRAWPALETTE iface) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
    return ++(This->ref);
}

HRESULT WINAPI IDirectDrawPaletteImpl_Initialize(
    LPDIRECTDRAWPALETTE iface,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    TRACE("(%p)->(%p,%ld,%p)\n", This, ddraw, x, palent);
    return DDERR_ALREADYINITIALIZED;
}

HRESULT WINAPI IDirectDrawPaletteImpl_GetCaps(
     LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps )
{
   ICOM_THIS(IDirectDrawPaletteImpl,iface);
   FIXME("(%p)->(%p) stub.\n", This, lpdwCaps );
   return DD_OK;
} 

HRESULT WINAPI IDirectDrawPaletteImpl_QueryInterface(
    LPDIRECTDRAWPALETTE iface,REFIID refiid,LPVOID *obj ) 
{
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    FIXME("(%p)->(%s,%p) stub.\n",This,debugstr_guid(refiid),obj);
    return S_OK;
}

ICOM_VTABLE(IDirectDrawPalette) ddraw_ddpalvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirectDrawPaletteImpl_QueryInterface,
    IDirectDrawPaletteImpl_AddRef,
    IDirectDrawPaletteImpl_Release,
    IDirectDrawPaletteImpl_GetCaps,
    IDirectDrawPaletteImpl_GetEntries,
    IDirectDrawPaletteImpl_Initialize,
    IDirectDrawPaletteImpl_SetEntries
};
