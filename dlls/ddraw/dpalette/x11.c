/*		DirectDraw IDirectDrawPalette X11 implementation
 *
 * Copyright 1997-2000 Marcus Meissner
 */

#include "config.h"
#include "winerror.h"


#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "debugtools.h"

#include "x11_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#define DPPRIVATE(x) x11_dp_private *dppriv = ((x11_dp_private*)x->private)

/******************************************************************************
 *			IDirectDrawPalette
 */

HRESULT WINAPI Xlib_IDirectDrawPaletteImpl_SetEntries(
    LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    XColor	xc;
    int		i;
    DPPRIVATE(This);

    TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",This,x,start,count,palent);
    for (i=0;i<count;i++) {
	xc.red = palent[i].peRed<<8;
	xc.blue = palent[i].peBlue<<8;
	xc.green = palent[i].peGreen<<8;
	xc.flags = DoRed|DoBlue|DoGreen;
	xc.pixel = start+i;

	if (dppriv->cm)
	    TSXStoreColor(display,dppriv->cm,&xc);

	This->palents[start+i].peRed = palent[i].peRed;
	This->palents[start+i].peBlue = palent[i].peBlue;
	This->palents[start+i].peGreen = palent[i].peGreen;
	This->palents[start+i].peFlags = palent[i].peFlags;
    }

    /* Now, if we are in 'depth conversion mode', update the screen palette */
    /* FIXME: we need to update the image or we won't get palette fading. */
    if (This->ddraw->d->palette_convert != NULL) {
	This->ddraw->d->palette_convert(palent,This->screen_palents,start,count);
    }
    return DD_OK;
}

ULONG WINAPI Xlib_IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE iface) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    DPPRIVATE(This);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );
    if (!--(This->ref)) {
	if (dppriv->cm) {
	    TSXFreeColormap(display,dppriv->cm);
	    dppriv->cm = 0;
	}
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

ICOM_VTABLE(IDirectDrawPalette) xlib_ddpalvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirectDrawPaletteImpl_QueryInterface,
    IDirectDrawPaletteImpl_AddRef,
    Xlib_IDirectDrawPaletteImpl_Release,
    IDirectDrawPaletteImpl_GetCaps,
    IDirectDrawPaletteImpl_GetEntries,
    IDirectDrawPaletteImpl_Initialize,
    Xlib_IDirectDrawPaletteImpl_SetEntries
};
