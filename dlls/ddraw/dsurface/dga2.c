/*		DirectDrawSurface XF86DGA implementation
 *
 * DGA2's specific DirectDrawSurface routines
 */
#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "debugtools.h"
#include "dga2_private.h"
#include "bitmap.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#define DDPRIVATE(x) dga2_dd_private *ddpriv = ((dga2_dd_private*)(x)->d->private)
#define DPPRIVATE(x) dga2_dp_private *dppriv = ((dga2_dp_private*)(x)->private)
#define DSPRIVATE(x) dga2_ds_private *dspriv = ((dga2_ds_private*)(x)->private)

static BYTE DGA2_TouchSurface(LPDIRECTDRAWSURFACE4 iface)
{   
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    /* if the DIB section is in GdiMod state, we must
     * touch the surface to get any updates from the DIB */
    return *(BYTE*)(This->s.surface_desc.u1.lpSurface);
}

HRESULT WINAPI DGA2_IDirectDrawSurface4Impl_Flip(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;
    DWORD	xheight;
    DSPRIVATE(This);
    dga_ds_private	*fspriv;
    LPBYTE	surf;

    TRACE("(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);

    DGA2_TouchSurface(iface);
    iflipto = _common_find_flipto(This,iflipto);

    /* and flip! */
    fspriv = (dga_ds_private*)iflipto->private;
    TSXDGASetViewport(display,DefaultScreen(display),0,fspriv->fb_height, XDGAFlipRetrace);
    TSXDGASync(display,DefaultScreen(display));
    TSXFlush(display);
    if (iflipto->s.palette) {
	DPPRIVATE(iflipto->s.palette);
	if (dppriv->cm)
	    TSXDGAInstallColormap(display,DefaultScreen(display),dppriv->cm);
    }

    /* We need to switch the lowlevel surfaces, for DGA this is: */
    /* The height within the framebuffer */
    xheight		= dspriv->fb_height;
    dspriv->fb_height	= fspriv->fb_height;
    fspriv->fb_height	= xheight;

    /* And the assciated surface pointer */
    surf	                         = This->s.surface_desc.u1.lpSurface;
    This->s.surface_desc.u1.lpSurface    = iflipto->s.surface_desc.u1.lpSurface;
    iflipto->s.surface_desc.u1.lpSurface = surf;

    return DD_OK;
}

HRESULT WINAPI DGA2_IDirectDrawSurface4Impl_SetPalette(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DDPRIVATE(This->s.ddraw);
    IDirectDrawPaletteImpl* ipal=(IDirectDrawPaletteImpl*)pal;

    TRACE("(%p)->(%p)\n",This,ipal);

    /* According to spec, we are only supposed to 
     * AddRef if this is not the same palette.
     */
    if( This->s.palette != ipal ) {
	dga_dp_private	*fppriv;
	if( ipal != NULL )
	    IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
	if( This->s.palette != NULL )
	    IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.palette );
	This->s.palette = ipal;
	fppriv = (dga_dp_private*)This->s.palette->private;

	if (!fppriv->cm &&
	    (This->s.ddraw->d->screen_pixelformat.u.dwRGBBitCount<=8) ) {
	  int i;
	  
	  /* Delayed palette creation */
	  fppriv->cm = TSXDGACreateColormap(display,DefaultScreen(display), ddpriv->dev, AllocAll);
	    
	  for (i=0;i<256;i++) {
	    XColor xc;
	    
	    xc.red		= ipal->palents[i].peRed<<8;
	    xc.blue		= ipal->palents[i].peBlue<<8;
	    xc.green	= ipal->palents[i].peGreen<<8;
	    xc.flags	= DoRed|DoBlue|DoGreen;
	    xc.pixel	= i;
	    TSXStoreColor(display,fppriv->cm,&xc);
	  }
	}

	TSXDGAInstallColormap(display,DefaultScreen(display),fppriv->cm);

        if (This->s.hdc != 0) {
	    /* hack: set the DIBsection color map */
	    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(This->s.DIBsection, BITMAP_MAGIC);
	    X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *)bmp->dib;
	    dib->colorMap = This->s.palette ? This->s.palette->screen_palents : NULL;
	    GDI_ReleaseObj(This->s.DIBsection);
	}
	TSXFlush(display);
    }
    return DD_OK;
}


ICOM_VTABLE(IDirectDrawSurface4) dga2_dds4vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirectDrawSurface4Impl_QueryInterface,
    IDirectDrawSurface4Impl_AddRef,
    DGA_IDirectDrawSurface4Impl_Release,
    IDirectDrawSurface4Impl_AddAttachedSurface,
    IDirectDrawSurface4Impl_AddOverlayDirtyRect,
    IDirectDrawSurface4Impl_Blt,
    IDirectDrawSurface4Impl_BltBatch,
    IDirectDrawSurface4Impl_BltFast,
    IDirectDrawSurface4Impl_DeleteAttachedSurface,
    IDirectDrawSurface4Impl_EnumAttachedSurfaces,
    IDirectDrawSurface4Impl_EnumOverlayZOrders,
    DGA2_IDirectDrawSurface4Impl_Flip,
    IDirectDrawSurface4Impl_GetAttachedSurface,
    IDirectDrawSurface4Impl_GetBltStatus,
    IDirectDrawSurface4Impl_GetCaps,
    IDirectDrawSurface4Impl_GetClipper,
    IDirectDrawSurface4Impl_GetColorKey,
    DGA_IDirectDrawSurface4Impl_GetDC,
    IDirectDrawSurface4Impl_GetFlipStatus,
    IDirectDrawSurface4Impl_GetOverlayPosition,
    IDirectDrawSurface4Impl_GetPalette,
    IDirectDrawSurface4Impl_GetPixelFormat,
    IDirectDrawSurface4Impl_GetSurfaceDesc,
    IDirectDrawSurface4Impl_Initialize,
    IDirectDrawSurface4Impl_IsLost,
    IDirectDrawSurface4Impl_Lock,
    IDirectDrawSurface4Impl_ReleaseDC,
    IDirectDrawSurface4Impl_Restore,
    IDirectDrawSurface4Impl_SetClipper,
    IDirectDrawSurface4Impl_SetColorKey,
    IDirectDrawSurface4Impl_SetOverlayPosition,
    DGA2_IDirectDrawSurface4Impl_SetPalette,
    DGA_IDirectDrawSurface4Impl_Unlock,
    IDirectDrawSurface4Impl_UpdateOverlay,
    IDirectDrawSurface4Impl_UpdateOverlayDisplay,
    IDirectDrawSurface4Impl_UpdateOverlayZOrder,
    IDirectDrawSurface4Impl_GetDDInterface,
    IDirectDrawSurface4Impl_PageLock,
    IDirectDrawSurface4Impl_PageUnlock,
    IDirectDrawSurface4Impl_SetSurfaceDesc,
    IDirectDrawSurface4Impl_SetPrivateData,
    IDirectDrawSurface4Impl_GetPrivateData,
    IDirectDrawSurface4Impl_FreePrivateData,
    IDirectDrawSurface4Impl_GetUniquenessValue,
    IDirectDrawSurface4Impl_ChangeUniquenessValue
};
