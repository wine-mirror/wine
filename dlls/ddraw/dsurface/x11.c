/*		DirectDrawSurface Xlib implementation
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */
#include "config.h"
#include "winerror.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "options.h"
#include "debugtools.h"
#include "x11_private.h"
#include "bitmap.h"

#ifdef HAVE_OPENGL
/* for d3d texture stuff */
# include "mesa_private.h"
#endif

DEFAULT_DEBUG_CHANNEL(ddraw);

#define VISIBLE(x) (SDDSCAPS(x) & (DDSCAPS_VISIBLE|DDSCAPS_PRIMARYSURFACE))

#define DDPRIVATE(x) x11_dd_private *ddpriv = ((x11_dd_private*)(x)->private)
#define DPPRIVATE(x) x11_dp_private *dppriv = ((x11_dp_private*)(x)->private)
#define DSPRIVATE(x) x11_ds_private *dspriv = ((x11_ds_private*)(x)->private)

static BYTE Xlib_TouchData(LPVOID data)
{
    /* this is a function so it doesn't get optimized out */
    return *(BYTE*)data;
}

/******************************************************************************
 *		IDirectDrawSurface methods
 *
 * Since DDS3 and DDS2 are supersets of DDS, we implement DDS3 and let
 * DDS and DDS2 use those functions. (Function calls did not change (except
 * using different DirectDrawSurfaceX version), just added flags and functions)
 */
HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_QueryInterface(
    LPDIRECTDRAWSURFACE4 iface,REFIID refiid,LPVOID *obj
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    
    /* All DirectDrawSurface versions (1, 2, 3 and 4) use
     * the same interface. And IUnknown does that too of course.
     */
    if ( IsEqualGUID( &IID_IDirectDrawSurface4, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface3, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface2, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface,  refiid )	||
	 IsEqualGUID( &IID_IUnknown,            refiid )
    ) {
	    *obj = This;
	    IDirectDrawSurface4_AddRef(iface);

	    TRACE("  Creating IDirectDrawSurface interface (%p)\n", *obj);
	    return S_OK;
    }
#ifdef HAVE_OPENGL
    if ( IsEqualGUID( &IID_IDirect3DTexture2, refiid ) ) {
	/* Texture interface */
	*obj = d3dtexture2_create(This);
	IDirectDrawSurface4_AddRef(iface);
	TRACE("  Creating IDirect3DTexture2 interface (%p)\n", *obj);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DTexture, refiid ) ) {
	/* Texture interface */
	*obj = d3dtexture_create(This);
	IDirectDrawSurface4_AddRef(iface);
	TRACE("  Creating IDirect3DTexture interface (%p)\n", *obj);
	return S_OK;
    }
#endif /* HAVE_OPENGL */
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Lock(
    LPDIRECTDRAWSURFACE4 iface,LPRECT lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE hnd
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DSPRIVATE(This);
    DDPRIVATE(This->s.ddraw);
    
    /* DO NOT AddRef the surface! Lock/Unlock are NOT guaranteed to come in 
     * matched pairs! - Marcus Meissner 20000509 */
    TRACE("(%p)->Lock(%p,%p,%08lx,%08lx)\n",This,lprect,lpddsd,flags,(DWORD)hnd);
    if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	WARN("(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		     This,lprect,lpddsd,flags,(DWORD)hnd);

    /* First, copy the Surface description */
    *lpddsd = This->s.surface_desc;
    TRACE("locked surface: height=%ld, width=%ld, pitch=%ld\n",
	  lpddsd->dwHeight,lpddsd->dwWidth,lpddsd->lPitch);

    /* If asked only for a part, change the surface pointer */
    if (lprect) {
	TRACE("	lprect: %dx%d-%dx%d\n",
		lprect->top,lprect->left,lprect->bottom,lprect->right
	);
	if ((lprect->top < 0) ||
	    (lprect->left < 0) ||
	    (lprect->bottom < 0) ||
	    (lprect->right < 0)) {
	  ERR(" Negative values in LPRECT !!!\n");
          IDirectDrawSurface4_Release(iface);
	  return DDERR_INVALIDPARAMS;
	}
	lpddsd->u1.lpSurface=(LPVOID)((char*)This->s.surface_desc.u1.lpSurface+
		(lprect->top*This->s.surface_desc.lPitch) +
		lprect->left*GET_BPP(This->s.surface_desc));
    } else
	assert(This->s.surface_desc.u1.lpSurface);
    /* wait for any previous operations to complete */
#ifdef HAVE_LIBXXSHM
    if (dspriv->image && VISIBLE(This) && ddpriv->xshm_active) {
/*
	int compl = InterlockedExchange( &(ddpriv->xshm_compl), 0 );
	if (compl) X11DRV_EVENT_WaitShmCompletion( compl );
*/
	X11DRV_EVENT_WaitShmCompletions( ddpriv->drawable );
    }
#endif

    return DD_OK;
}

static void Xlib_copy_surface_on_screen(IDirectDrawSurface4Impl* This) {
  DSPRIVATE(This);
  DDPRIVATE(This->s.ddraw);
  if (This->s.ddraw->d.pixel_convert != NULL)
    This->s.ddraw->d.pixel_convert(This->s.surface_desc.u1.lpSurface,
				   dspriv->image->data,
				   This->s.surface_desc.dwWidth,
				   This->s.surface_desc.dwHeight,
				   This->s.surface_desc.lPitch,
				   This->s.palette);

  /* if the DIB section is in GdiMod state, we must
   * touch the surface to get any updates from the DIB */
  Xlib_TouchData(dspriv->image->data);
#ifdef HAVE_LIBXXSHM
    if (ddpriv->xshm_active) {
/*
	X11DRV_EVENT_WaitReplaceShmCompletion( &(ddpriv->xshm_compl), This->s.ddraw->d.drawable );
*/
	/* let WaitShmCompletions track 'em for now */
	/* (you may want to track it again whenever you implement DX7's partial
	* surface locking, where threads have concurrent access) */
	X11DRV_EVENT_PrepareShmCompletion( ddpriv->drawable );
	TSXShmPutImage(display,
	    ddpriv->drawable,
	    DefaultGCOfScreen(X11DRV_GetXScreen()),
	    dspriv->image,
	    0, 0, 0, 0,
	    dspriv->image->width,
	    dspriv->image->height,
	    True
	);
	/* make sure the image is transferred ASAP */
	TSXFlush(display);
    } else
#endif
	TSXPutImage(	
	    display,
	    ddpriv->drawable,
	    DefaultGCOfScreen(X11DRV_GetXScreen()),
	    dspriv->image,
	    0, 0, 0, 0,
	    dspriv->image->width,
	    dspriv->image->height
	);
}

HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Unlock(
    LPDIRECTDRAWSURFACE4 iface,LPVOID surface
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DDPRIVATE(This->s.ddraw);
    DSPRIVATE(This);
    TRACE("(%p)->Unlock(%p)\n",This,surface);

    /*if (!This->s.ddraw->d.paintable)
	return DD_OK; */

    /* Only redraw the screen when unlocking the buffer that is on screen */
    if (dspriv->image && VISIBLE(This)) {
	Xlib_copy_surface_on_screen(This);
	if (This->s.palette) {
    	    DPPRIVATE(This->s.palette);
	    if(dppriv->cm)
		TSXSetWindowColormap(display,ddpriv->drawable,dppriv->cm);
	}
    }
    /* DO NOT Release the surface! Lock/Unlock are NOT guaranteed to come in 
     * matched pairs! - Marcus Meissner 20000509 */
    return DD_OK;
}

HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Flip(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    XImage	*image;
    DDPRIVATE(This->s.ddraw);
    DSPRIVATE(This);
    x11_ds_private	*fspriv;
    LPBYTE	surf;
    IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;

    TRACE("(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);
    if (!This->s.ddraw->d.paintable)
	return DD_OK;

    iflipto = _common_find_flipto(This,iflipto);
    fspriv = (x11_ds_private*)iflipto->private;

    /* We need to switch the lowlevel surfaces, for xlib this is: */
    /* The surface pointer */
    surf				= This->s.surface_desc.u1.lpSurface;
    This->s.surface_desc.u1.lpSurface	= iflipto->s.surface_desc.u1.lpSurface;
    iflipto->s.surface_desc.u1.lpSurface	= surf;

    /* the associated ximage */
    image		= dspriv->image;
    dspriv->image	= fspriv->image;
    fspriv->image	= image;

#ifdef HAVE_LIBXXSHM
    if (ddpriv->xshm_active) {
/*
	int compl = InterlockedExchange( &(ddpriv->xshm_compl), 0 );
	if (compl) X11DRV_EVENT_WaitShmCompletion( compl );
*/
	X11DRV_EVENT_WaitShmCompletions( ddpriv->drawable );
    }
#endif
    Xlib_copy_surface_on_screen(This);
    if (iflipto->s.palette) {
        DPPRIVATE(iflipto->s.palette);
	if (dppriv->cm)
	    TSXSetWindowColormap(display,ddpriv->drawable,dppriv->cm);
    }
    return DD_OK;
}

/* The IDirectDrawSurface4::SetPalette method attaches the specified
 * DirectDrawPalette object to a surface. The surface uses this palette for all
 * subsequent operations. The palette change takes place immediately.
 */
HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_SetPalette(
    LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DDPRIVATE(This->s.ddraw);
    IDirectDrawPaletteImpl* ipal=(IDirectDrawPaletteImpl*)pal;
    x11_dp_private	*dppriv;
    int i;

    TRACE("(%p)->(%p)\n",This,ipal);

    if (ipal == NULL) {
	if( This->s.palette != NULL )
	    IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);
	This->s.palette = ipal;
	return DD_OK;
    }
    dppriv = (x11_dp_private*)ipal->private;

    if (!dppriv->cm &&
	(This->s.ddraw->d.screen_pixelformat.u.dwRGBBitCount<=8)
    ) {
	dppriv->cm = TSXCreateColormap(
	    display,
	    ddpriv->drawable,
	    DefaultVisualOfScreen(X11DRV_GetXScreen()),
	    AllocAll
	);
	if (!Options.managed)
	    TSXInstallColormap(display,dppriv->cm);

	for (i=0;i<256;i++) {
	    XColor xc;

	    xc.red		= ipal->palents[i].peRed<<8;
	    xc.blue		= ipal->palents[i].peBlue<<8;
	    xc.green	= ipal->palents[i].peGreen<<8;
	    xc.flags	= DoRed|DoBlue|DoGreen;
	    xc.pixel	= i;
	    TSXStoreColor(display,dppriv->cm,&xc);
	}
	TSXInstallColormap(display,dppriv->cm);
    }
    /* According to spec, we are only supposed to 
     * AddRef if this is not the same palette.
     */
    if ( This->s.palette != ipal ) {
	if( ipal != NULL )
	    IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
	if( This->s.palette != NULL )
	    IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.palette );
	This->s.palette = ipal; 
	/* Perform the refresh */
	TSXSetWindowColormap(display,ddpriv->drawable,dppriv->cm);

	if (This->s.hdc != 0) {
	    /* hack: set the DIBsection color map */
	    BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(This->s.DIBsection, BITMAP_MAGIC);
	    X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *)bmp->dib;
	    dib->colorMap = This->s.palette ? This->s.palette->screen_palents : NULL;
	    GDI_HEAP_UNLOCK(This->s.DIBsection);
	}
    }
    return DD_OK;
}

ULONG WINAPI Xlib_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DSPRIVATE(This);
    DDPRIVATE(This->s.ddraw);

    TRACE( "(%p)->() decrementing from %lu.\n", This, This->ref );
    if (--(This->ref))
    	return This->ref;

    IDirectDraw2_Release((IDirectDraw2*)This->s.ddraw);

    if (dspriv->image != NULL) {
	if (This->s.ddraw->d.pixel_convert != NULL) {
	    /* In pixel conversion mode, there are 2 buffers to release. */
	    VirtualFree(This->s.surface_desc.u1.lpSurface, 0, MEM_RELEASE);

#ifdef HAVE_LIBXXSHM
	    if (ddpriv->xshm_active) {
		TSXShmDetach(display, &(dspriv->shminfo));
		TSXDestroyImage(dspriv->image);
		shmdt(dspriv->shminfo.shmaddr);
	    } else
#endif
	    {
		HeapFree(GetProcessHeap(),0,dspriv->image->data);
		dspriv->image->data = NULL;
		TSXDestroyImage(dspriv->image);
	    }
	} else {
	    dspriv->image->data = NULL;

#ifdef HAVE_LIBXXSHM
	    if (ddpriv->xshm_active) {
		VirtualFree(dspriv->image->data, 0, MEM_RELEASE);
		TSXShmDetach(display, &(dspriv->shminfo));
		TSXDestroyImage(dspriv->image);
		shmdt(dspriv->shminfo.shmaddr);
	    } else
#endif
	    {
		VirtualFree(This->s.surface_desc.u1.lpSurface, 0, MEM_RELEASE);
		TSXDestroyImage(dspriv->image);
	    }
	}
	dspriv->image = 0;
    } else
	VirtualFree(This->s.surface_desc.u1.lpSurface, 0, MEM_RELEASE);

    if (This->s.palette)
	IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);

    /* Free the DIBSection (if any) */
    if (This->s.hdc != 0) {
	/* hack: restore the original DIBsection color map */
	BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(This->s.DIBsection, BITMAP_MAGIC);
	X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *)bmp->dib;
	dib->colorMap = dspriv->oldDIBmap;
	GDI_HEAP_UNLOCK(This->s.DIBsection);

	SelectObject(This->s.hdc, This->s.holdbitmap);
	DeleteDC(This->s.hdc);
	DeleteObject(This->s.DIBsection);
    }

    /* Free the clipper if present */
    if(This->s.lpClipper)
	IDirectDrawClipper_Release(This->s.lpClipper);
    HeapFree(GetProcessHeap(),0,This->private);
    HeapFree(GetProcessHeap(),0,This);
    return S_OK;
}

HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DSPRIVATE(This);
    int was_ok = This->s.hdc != 0;
    HRESULT result = IDirectDrawSurface4Impl_GetDC(iface,lphdc);
    if (This->s.hdc && !was_ok) {
	/* hack: take over the DIBsection color map */
	BITMAPOBJ *bmp = (BITMAPOBJ *) GDI_GetObjPtr(This->s.DIBsection, BITMAP_MAGIC);
	X11DRV_DIBSECTION *dib = (X11DRV_DIBSECTION *)bmp->dib;
	dspriv->oldDIBmap = dib->colorMap;
	dib->colorMap = This->s.palette ? This->s.palette->screen_palents : NULL;
	GDI_HEAP_UNLOCK(This->s.DIBsection);
    }
    return result;
}

ICOM_VTABLE(IDirectDrawSurface4) xlib_dds4vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    Xlib_IDirectDrawSurface4Impl_QueryInterface,
    IDirectDrawSurface4Impl_AddRef,
    Xlib_IDirectDrawSurface4Impl_Release,
    IDirectDrawSurface4Impl_AddAttachedSurface,
    IDirectDrawSurface4Impl_AddOverlayDirtyRect,
    IDirectDrawSurface4Impl_Blt,
    IDirectDrawSurface4Impl_BltBatch,
    IDirectDrawSurface4Impl_BltFast,
    IDirectDrawSurface4Impl_DeleteAttachedSurface,
    IDirectDrawSurface4Impl_EnumAttachedSurfaces,
    IDirectDrawSurface4Impl_EnumOverlayZOrders,
    Xlib_IDirectDrawSurface4Impl_Flip,
    IDirectDrawSurface4Impl_GetAttachedSurface,
    IDirectDrawSurface4Impl_GetBltStatus,
    IDirectDrawSurface4Impl_GetCaps,
    IDirectDrawSurface4Impl_GetClipper,
    IDirectDrawSurface4Impl_GetColorKey,
    Xlib_IDirectDrawSurface4Impl_GetDC,
    IDirectDrawSurface4Impl_GetFlipStatus,
    IDirectDrawSurface4Impl_GetOverlayPosition,
    IDirectDrawSurface4Impl_GetPalette,
    IDirectDrawSurface4Impl_GetPixelFormat,
    IDirectDrawSurface4Impl_GetSurfaceDesc,
    IDirectDrawSurface4Impl_Initialize,
    IDirectDrawSurface4Impl_IsLost,
    Xlib_IDirectDrawSurface4Impl_Lock,
    IDirectDrawSurface4Impl_ReleaseDC,
    IDirectDrawSurface4Impl_Restore,
    IDirectDrawSurface4Impl_SetClipper,
    IDirectDrawSurface4Impl_SetColorKey,
    IDirectDrawSurface4Impl_SetOverlayPosition,
    Xlib_IDirectDrawSurface4Impl_SetPalette,
    Xlib_IDirectDrawSurface4Impl_Unlock,
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
