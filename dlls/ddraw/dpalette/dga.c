/*		DirectDrawPalette XF86DGA implementation
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 */

#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "debugtools.h"

#include "dga_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#define DPPRIVATE(x) dga_dp_private *dppriv = ((dga_dp_private*)(x)->private)
#define DDPRIVATE(x) dga_dd_private *ddpriv = ((dga_dd_private*)(x)->private)

HRESULT WINAPI DGA_IDirectDrawPaletteImpl_SetEntries(
    LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
    ICOM_THIS(IDirectDrawPaletteImpl,iface);
    DPPRIVATE(This);
    DDPRIVATE(This->ddraw);
    XColor	xc;
    Colormap	cm;
    int		i;

    TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",This,x,start,count,palent);
    if (!dppriv->cm) /* should not happen */ {
	FIXME("app tried to set colormap in non-palettized mode\n");
	return DDERR_GENERIC;
    }
    /* FIXME: free colorcells instead of freeing whole map */
    cm = dppriv->cm;
    dppriv->cm = TSXCopyColormapAndFree(display,dppriv->cm);
    TSXFreeColormap(display,cm);

    for (i=0;i<count;i++) {
	xc.red = palent[i].peRed<<8;
	xc.blue = palent[i].peBlue<<8;
	xc.green = palent[i].peGreen<<8;
	xc.flags = DoRed|DoBlue|DoGreen;
	xc.pixel = i+start;

	TSXStoreColor(display,dppriv->cm,&xc);

	This->palents[start+i].peRed = palent[i].peRed;
	This->palents[start+i].peBlue = palent[i].peBlue;
	This->palents[start+i].peGreen = palent[i].peGreen;
	This->palents[start+i].peFlags = palent[i].peFlags;
    }
#ifdef HAVE_LIBXXF86DGA2
    if (ddpriv->version == 2) {
	TSXDGAInstallColormap(display,DefaultScreen(display),dppriv->cm);
    } else
#endif /* defined(HAVE_LIBXXF86DGA2) */
	TSXF86DGAInstallColormap(display,DefaultScreen(display),dppriv->cm);
    return DD_OK;
}
ICOM_VTABLE(IDirectDrawPalette) dga_ddpalvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectDrawPaletteImpl_QueryInterface,
	IDirectDrawPaletteImpl_AddRef,
	Xlib_IDirectDrawPaletteImpl_Release,
	IDirectDrawPaletteImpl_GetCaps,
	IDirectDrawPaletteImpl_GetEntries,
	IDirectDrawPaletteImpl_Initialize,
	DGA_IDirectDrawPaletteImpl_SetEntries
};
