/*		DirectDraw IDirectDraw Xlib interface
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */
/*
 * This file contains the Xlib specific interface functions.
 */

#include "config.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "ddraw.h"
#include "d3d.h"
#include "win.h"
#include "debugtools.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#include "x11_private.h"

#define DDPRIVATE(x) x11_dd_private *ddpriv = ((x11_dd_private*)(x)->private)
#define DPPRIVATE(x) x11_dp_private *dppriv = ((x11_dp_private*)(x)->private)
#define DSPRIVATE(x) x11_ds_private *dspriv = ((x11_ds_private*)(x)->private)

static inline BOOL get_option( const char *name, BOOL def ) {
    return PROFILE_GetWineIniBool( "x11drv", name, def );
}

int _common_depth_to_pixelformat(
    DWORD depth, DDPIXELFORMAT *pixelformat,DDPIXELFORMAT *screen_pixelformat,
    int *pix_depth
) {
  XVisualInfo *vi;
  XPixmapFormatValues *pf;
  XVisualInfo vt;
  int nvisuals, npixmap, i;
  int match = 0;
  int index = -2;

  vi = TSXGetVisualInfo(display, VisualNoMask, &vt, &nvisuals);
  pf = XListPixmapFormats(display, &npixmap);

  for (i = 0; i < npixmap; i++) {
    if (pf[i].depth == depth) {
      int j;

      for (j = 0; j < nvisuals; j++) {
	if (vi[j].depth == pf[i].depth) {
	  pixelformat->dwSize = sizeof(*pixelformat);
	  if (depth == 8) {
	    pixelformat->dwFlags = DDPF_PALETTEINDEXED8|DDPF_RGB;
	    pixelformat->u1.dwRBitMask = 0;
	    pixelformat->u2.dwGBitMask = 0;
	    pixelformat->u3.dwBBitMask = 0;
	  } else {
	    pixelformat->dwFlags = DDPF_RGB;
	    pixelformat->u1.dwRBitMask = vi[j].red_mask;
	    pixelformat->u2.dwGBitMask = vi[j].green_mask;
	    pixelformat->u3.dwBBitMask = vi[j].blue_mask;
	  }
	  pixelformat->dwFourCC = 0;
	  pixelformat->u.dwRGBBitCount = pf[i].bits_per_pixel;
	  pixelformat->u4.dwRGBAlphaBitMask= 0;

	  *screen_pixelformat = *pixelformat;

	  if (pix_depth)
	    *pix_depth = vi[j].depth;

	  match = 1;
	  index = -1;
	  goto clean_up_and_exit;
	}
      }
      ERR("No visual corresponding to pixmap format !\n");
    }
  }

  if (match == 0) {
    /* We try now to find an emulated mode */
    int c;

    for (c = 0; c < sizeof(ModeEmulations) / sizeof(Convert); c++) {
      if (ModeEmulations[c].dest.depth == depth) {
	/* Found an emulation function, now tries to find a matching visual / pixel format pair */
	for (i = 0; i < npixmap; i++) {
	  if ((pf[i].depth == ModeEmulations[c].screen.depth) &&
	      (pf[i].bits_per_pixel == ModeEmulations[c].screen.bpp)) {
	    int j;

	    for (j = 0; j < nvisuals; j++) {
	      if (vi[j].depth == pf[i].depth) {
		screen_pixelformat->dwSize = sizeof(*screen_pixelformat);
		screen_pixelformat->dwFlags = DDPF_RGB;
		screen_pixelformat->dwFourCC = 0;
		screen_pixelformat->u.dwRGBBitCount = pf[i].bits_per_pixel;
		screen_pixelformat->u1.dwRBitMask = vi[j].red_mask;
		screen_pixelformat->u2.dwGBitMask = vi[j].green_mask;
		screen_pixelformat->u3.dwBBitMask = vi[j].blue_mask;
		screen_pixelformat->u4.dwRGBAlphaBitMask= 0;

		pixelformat->dwSize = sizeof(*pixelformat);
		pixelformat->dwFourCC = 0;
		if (depth == 8) {
		  pixelformat->dwFlags = DDPF_RGB|DDPF_PALETTEINDEXED8;
		  pixelformat->u.dwRGBBitCount = 8;
		  pixelformat->u1.dwRBitMask = 0;
		  pixelformat->u2.dwGBitMask = 0;
		  pixelformat->u3.dwBBitMask = 0;
		} else {
		  pixelformat->dwFlags = DDPF_RGB;
		  pixelformat->u.dwRGBBitCount = ModeEmulations[c].dest.bpp;
		  pixelformat->u1.dwRBitMask = ModeEmulations[c].dest.rmask;
		  pixelformat->u2.dwGBitMask = ModeEmulations[c].dest.gmask;
		  pixelformat->u3.dwBBitMask = ModeEmulations[c].dest.bmask;
		}
		pixelformat->u4.dwRGBAlphaBitMask= 0;    

		if (pix_depth)
		  *pix_depth = vi[j].depth;

		match = 2;
		index = c;
		goto clean_up_and_exit;
	      }
	      ERR("No visual corresponding to pixmap format !\n");
	    }
	  }
	}
      }
    }
  }

clean_up_and_exit:
  TSXFree(vi);
  TSXFree(pf);

  return index;
}

/*******************************************************************************
 *				IDirectDraw
 */
#ifdef HAVE_LIBXXSHM
/* Error handlers for Image creation */
static int XShmErrorHandler(Display *dpy, XErrorEvent *event) {
    XShmErrorFlag = 1;
    return 0;
}

static XImage *create_xshmimage(
    IDirectDraw2Impl* This, IDirectDrawSurface4Impl* lpdsf
) {
    DSPRIVATE(lpdsf);
    DDPRIVATE(This);
    XImage *img;
    int (*WineXHandler)(Display *, XErrorEvent *);

    img = TSXShmCreateImage(display,
	DefaultVisualOfScreen(X11DRV_GetXScreen()),
	This->d.pixmap_depth,
	ZPixmap,
	NULL,
	&(dspriv->shminfo),
	lpdsf->s.surface_desc.dwWidth,
	lpdsf->s.surface_desc.dwHeight
    );

    if (img == NULL) {
	FIXME("Couldn't create XShm image (due to X11 remote display or failure).\nReverting to standard X images !\n");
	ddpriv->xshm_active = 0;
	return NULL;
    }

    dspriv->shminfo.shmid = shmget( IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT|0777 );
    if (dspriv->shminfo.shmid < 0) {
	FIXME("Couldn't create shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
	ddpriv->xshm_active = 0;
	TSXDestroyImage(img);
	return NULL;
    }

    dspriv->shminfo.shmaddr=img->data=(char*)shmat(dspriv->shminfo.shmid,0,0);

    if (img->data == (char *) -1) {
	FIXME("Couldn't attach shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
	ddpriv->xshm_active = 0;
	TSXDestroyImage(img);
	shmctl(dspriv->shminfo.shmid, IPC_RMID, 0);
	return NULL;
    }
    dspriv->shminfo.readOnly = False;

    /* This is where things start to get trickier....
     * First, we flush the current X connections to be sure to catch all
     * non-XShm related errors
     */
    TSXSync(display, False);
    /* Then we enter in the non-thread safe part of the tests */
    EnterCriticalSection( &X11DRV_CritSection );

    /* Reset the error flag, sets our new error handler and try to attach
     * the surface
     */
    XShmErrorFlag = 0;
    WineXHandler = XSetErrorHandler(XShmErrorHandler);
    XShmAttach(display, &(dspriv->shminfo));
    XSync(display, False);

    /* Check the error flag */
    if (XShmErrorFlag) {
	/* An error occured */
	XFlush(display);
	XShmErrorFlag = 0;
	XDestroyImage(img);
	shmdt(dspriv->shminfo.shmaddr);
	shmctl(dspriv->shminfo.shmid, IPC_RMID, 0);
	XSetErrorHandler(WineXHandler);

	FIXME("Couldn't attach shared memory segment to X server (due to X11 remote display or failure).\nReverting to standard X images !\n");
	ddpriv->xshm_active = 0;

	/* Leave the critical section */
	LeaveCriticalSection( &X11DRV_CritSection );
	return NULL;
    }
    /* Here, to be REALLY sure, I should do a XShmPutImage to check if
     * this works, but it may be a bit overkill....
     */
    XSetErrorHandler(WineXHandler);
    LeaveCriticalSection( &X11DRV_CritSection );

    shmctl(dspriv->shminfo.shmid, IPC_RMID, 0);

    if (This->d.pixel_convert != NULL) {
	int bpp = PFGET_BPP(This->d.directdraw_pixelformat);
	lpdsf->s.surface_desc.u1.lpSurface = HeapAlloc(
	    GetProcessHeap(),
	    HEAP_ZERO_MEMORY,
	    lpdsf->s.surface_desc.dwWidth *
	    lpdsf->s.surface_desc.dwHeight *
	    bpp
	);
    } else
	lpdsf->s.surface_desc.u1.lpSurface = img->data;
    return img;
}
#endif /* HAVE_LIBXXSHM */

static XImage *create_ximage(IDirectDraw2Impl* This, IDirectDrawSurface4Impl* lpdsf) {
    XImage *img = NULL;
    DDPRIVATE(This);
    void *img_data;
    int bpp = PFGET_BPP(This->d.directdraw_pixelformat);
    
#ifdef HAVE_LIBXXSHM
    if (ddpriv->xshm_active)
	img = create_xshmimage(This, lpdsf);

    if (img == NULL) {
#endif
    /* Allocate surface memory */
	lpdsf->s.surface_desc.u1.lpSurface = HeapAlloc(
	    GetProcessHeap(),
	    HEAP_ZERO_MEMORY,
	    lpdsf->s.surface_desc.dwWidth *
	    lpdsf->s.surface_desc.dwHeight *
	    bpp
	);

	if (This->d.pixel_convert != NULL)
	    img_data = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		lpdsf->s.surface_desc.dwWidth *
		lpdsf->s.surface_desc.dwHeight *
		bpp
	    );
	else
	    img_data = lpdsf->s.surface_desc.u1.lpSurface;

	/* In this case, create an XImage */
	img = TSXCreateImage(display,
	    DefaultVisualOfScreen(X11DRV_GetXScreen()),
	    This->d.pixmap_depth,
	    ZPixmap,
	    0,
	    img_data,
	    lpdsf->s.surface_desc.dwWidth,
	    lpdsf->s.surface_desc.dwHeight,
	    32,
	    lpdsf->s.surface_desc.dwWidth*bpp
	);
#ifdef HAVE_LIBXXSHM
    }
#endif
    /* assert(bpp*lpdsf->s.surface_desc.dwWidth == img->bytes_per_line); */

    if (This->d.pixel_convert != NULL)
	lpdsf->s.surface_desc.lPitch = bpp*lpdsf->s.surface_desc.dwWidth;
    else
	lpdsf->s.surface_desc.lPitch = img->bytes_per_line;
    return img;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreateSurface(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,
    IUnknown *lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawSurfaceImpl* dsurf;
    x11_ds_private	*dspriv;

    TRACE("(%p)->CreateSurface(%p,%p,%p)\n", This,lpddsd,lpdsf,lpunk);

    if (TRACE_ON(ddraw)) _dump_surface_desc(lpddsd);

    *lpdsf = HeapAlloc(
    	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(IDirectDrawSurfaceImpl)
    );
    dsurf = (IDirectDrawSurfaceImpl*)*lpdsf;
    dsurf->ref                 = 2;
    dsurf->private = HeapAlloc(
	    GetProcessHeap(),
	    HEAP_ZERO_MEMORY,
	    sizeof(x11_ds_private)
    );
    ICOM_VTBL(dsurf) = (ICOM_VTABLE(IDirectDrawSurface)*)&xlib_dds4vt;
    dspriv = (x11_ds_private*)dsurf->private;

    dsurf->s.ddraw	= This;
    IDirectDraw2_AddRef(iface);

    dsurf->s.palette	= NULL;
    dsurf->s.lpClipper	= NULL;
    dspriv->image	= NULL; /* This is for off-screen buffers */

    /* Copy the surface description */
    dsurf->s.surface_desc = *lpddsd;

    if (!(lpddsd->dwFlags & DDSD_WIDTH))
	dsurf->s.surface_desc.dwWidth  = This->d.width;
    if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	dsurf->s.surface_desc.dwHeight = This->d.height;
    dsurf->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT;

    /* Check if this a 'primary surface' or not */
    if ((lpddsd->dwFlags & DDSD_CAPS) && 
	(lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    ) {
	XImage *img;

	TRACE("using standard XImage for a primary surface (%p)\n", dsurf);
	/* Create the XImage */
	img = create_ximage(This,(IDirectDrawSurface4Impl*)dsurf);
	if (img == NULL)
	    return DDERR_OUTOFMEMORY;
	dspriv->image = img;

	/* Add flags if there were not present */
	dsurf->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;
	dsurf->s.surface_desc.dwWidth = This->d.width;
	dsurf->s.surface_desc.dwHeight = This->d.height;
	dsurf->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	dsurf->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;

	/* Check for backbuffers */
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
	    IDirectDrawSurface4Impl*	back;
	    XImage *img;
	    int	i;

	    for (i=lpddsd->dwBackBufferCount;i--;) {
		x11_ds_private *bspriv;
		back = (IDirectDrawSurface4Impl*)HeapAlloc(
		    GetProcessHeap(),
		    HEAP_ZERO_MEMORY,
		    sizeof(IDirectDrawSurface4Impl)
		);
		TRACE("allocated back-buffer (%p)\n", back);

		IDirectDraw2_AddRef(iface);
		back->s.ddraw = This;
		
		back->ref = 2;
		ICOM_VTBL(back)=(ICOM_VTABLE(IDirectDrawSurface4)*)&xlib_dds4vt;
		/* Copy the surface description from the front buffer */
		back->s.surface_desc = dsurf->s.surface_desc;
		back->s.surface_desc.u1.lpSurface = NULL;
		
		back->private = HeapAlloc(
		    GetProcessHeap(),
		    HEAP_ZERO_MEMORY,
		    sizeof(x11_ds_private)
		);
		bspriv = (x11_ds_private*)back->private;

		/* Create the XImage. */
		img = create_ximage(This, back);
		if (img == NULL)
		    return DDERR_OUTOFMEMORY;
		TRACE("bspriv = %p\n",bspriv);
		bspriv->image = img;

		/* Add relevant info to front and back buffers */
		/* FIXME: backbuffer/frontbuffer handling broken here, but
		 * will be fixed up in _Flip().
		 */
		SDDSCAPS(dsurf) |= DDSCAPS_FRONTBUFFER;
		SDDSCAPS(back) |= DDSCAPS_BACKBUFFER|DDSCAPS_VIDEOMEMORY|DDSCAPS_FLIP;
		back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
		SDDSCAPS(back) &= ~DDSCAPS_VISIBLE;
		TRACE("attaching surface %p to %p\n",back,*lpdsf);
		IDirectDrawSurface4_AddAttachedSurface((LPDIRECTDRAWSURFACE4)(*lpdsf),(LPDIRECTDRAWSURFACE4)back);
	    }
	}
    } else {
	/* There is no Xlib-specific code here...
	 * Go to the common surface creation function
	 */
	return common_off_screen_CreateSurface(This,dsurf);
    }
    return DD_OK;
}

/* 
 * The Xlib Implementation tries to use the passed hwnd as drawing window,
 * even when the approbiate bitmasks are not specified.
 */
static HRESULT WINAPI Xlib_IDirectDraw2Impl_SetCooperativeLevel(
    LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);

    FIXME("(%p)->(%08lx,%08lx)\n",This,(DWORD)hwnd,cooplevel);
    if (TRACE_ON(ddraw))
	_dump_cooperativelevel(cooplevel);
    This->d.mainWindow = hwnd;

    /* This will be overwritten in the case of Full Screen mode.
       Windowed games could work with that :-) */
    if (hwnd) {
	WND *tmpWnd = WIN_FindWndPtr(hwnd);
	ddpriv->drawable  = X11DRV_WND_GetXWindow(tmpWnd);
	WIN_ReleaseWndPtr(tmpWnd);

	if( !ddpriv->drawable ) {
	    ddpriv->drawable = ((X11DRV_WND_DATA *) WIN_GetDesktop()->pDriverData)->window;
	    WIN_ReleaseDesktop();
	}
	TRACE("Setting drawable to %ld\n", ddpriv->drawable);
    }
    return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDrawImpl_SetDisplayMode(
    LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
    ICOM_THIS(IDirectDrawImpl,iface);
    DDPRIVATE(This);
    char	buf[200];
    WND *tmpWnd;
    int c;

    TRACE("(%p)->SetDisplayMode(%ld,%ld,%ld)\n",
		  This, width, height, depth);

    switch ((c = _common_depth_to_pixelformat(depth,
					 &(This->d.directdraw_pixelformat),
					 &(This->d.screen_pixelformat),
					      &(This->d.pixmap_depth)))) {
    case -2:
      sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
      MessageBoxA(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
      return DDERR_UNSUPPORTEDMODE;

    case -1:
      /* No convertion */
      This->d.pixel_convert = NULL;
      This->d.palette_convert = NULL;
      break;

    default:
      DPRINTF("DirectDraw warning: running in depth-convertion mode. Should run using a %ld depth for optimal performances.\n", depth);
      /* Set the depth convertion routines */
      This->d.pixel_convert = ModeEmulations[c].funcs.pixel_convert;
      This->d.palette_convert = ModeEmulations[c].funcs.palette_convert;
    }
	
    This->d.width	= width;
    This->d.height	= height;

    _common_IDirectDrawImpl_SetDisplayMode(This);

    tmpWnd = WIN_FindWndPtr(This->d.window);
    This->d.paintable = 1;
    ddpriv->drawable  = ((X11DRV_WND_DATA *) tmpWnd->pDriverData)->window;
    WIN_ReleaseWndPtr(tmpWnd);

    /* We don't have a context for this window. Host off the desktop */
    if( !ddpriv->drawable )
    {
       ddpriv->drawable = ((X11DRV_WND_DATA *) WIN_GetDesktop()->pDriverData)->window;
	WIN_ReleaseDesktop();
    }
    TRACE("Setting drawable to %ld\n", ddpriv->drawable);

    if (get_option( "DXGrab", 0 )) {
	/* Confine cursor movement (risky, but the user asked for it) */
	TSXGrabPointer(display, ddpriv->drawable, True, 0, GrabModeAsync, GrabModeAsync, ddpriv->drawable, None, CurrentTime);
    }

    return DD_OK;
}

static void fill_caps(LPDDCAPS caps) {
  /* This function tries to fill the capabilities of Wine's DDraw implementation.
     Need to be fixed, though.. */
  if (caps == NULL)
    return;

  caps->dwSize = sizeof(*caps);
  caps->dwCaps = DDCAPS_ALPHA | DDCAPS_BLT | DDCAPS_BLTSTRETCH | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL | DDCAPS_CANBLTSYSMEM |  DDCAPS_COLORKEY | DDCAPS_PALETTE /*| DDCAPS_NOHARDWARE*/;
  caps->dwCaps2 = DDCAPS2_CERTIFIED | DDCAPS2_NOPAGELOCKREQUIRED | DDCAPS2_WIDESURFACES;
  caps->dwCKeyCaps = 0xFFFFFFFF; /* Should put real caps here one day... */
  caps->dwFXCaps = 0;
  caps->dwFXAlphaCaps = 0;
  caps->dwPalCaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
  caps->dwSVCaps = 0;
  caps->dwZBufferBitDepths = DDBD_16;
  /* I put here 8 Mo so that D3D applications will believe they have enough memory
     to put textures in video memory.
     BTW, is this only frame buffer memory or also texture memory (for Voodoo boards
     for example) ? */
  caps->dwVidMemTotal = 8192 * 1024;
  caps->dwVidMemFree = 8192 * 1024;
  /* These are all the supported capabilities of the surfaces */
  caps->ddsCaps.dwCaps = DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
    DDSCAPS_FRONTBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN |
      /*DDSCAPS_OVERLAY |*/ DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
	DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;
#ifdef HAVE_MESAGL
  caps->dwCaps |= DDCAPS_3D | DDCAPS_ZBLTS;
  caps->dwCaps2 |=  DDCAPS2_NO2DDURING3DSCENE;
  caps->ddsCaps.dwCaps |= DDSCAPS_3DDEVICE | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
#endif
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetCaps(
    LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
)  {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);

    /* Put the same caps for the two capabilities */
    fill_caps(caps1);
    fill_caps(caps2);

    return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreatePalette(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,
    LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
    int xsize;
    HRESULT res;

    TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
    res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
    if (res != 0)
	return res;
    (*ilpddpal)->private = HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(x11_dp_private)
    );
    ICOM_VTBL(*ilpddpal) = &xlib_ddpalvt;
    return DD_OK;
}

static ULONG WINAPI Xlib_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	if (This->d.window && GetPropA(This->d.window,ddProp))
	    DestroyWindow(This->d.window);
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    /* FIXME: destroy window ... */
    return This->ref;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_QueryInterface(
    LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj
) {
    ICOM_THIS(IDirectDraw2Impl,iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    if ( IsEqualGUID( &IID_IUnknown, refiid ) ) {
	*obj = This;
	IDirectDraw2_AddRef(iface);

	TRACE("  Creating IUnknown interface (%p)\n", *obj);
	
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDraw, refiid ) ) {
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&xlib_ddvt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw interface (%p)\n", *obj);
	
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) ) {
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&xlib_dd2vt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw2 interface (%p)\n", *obj);
	
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) ) {
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&xlib_dd4vt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw4 interface (%p)\n", *obj);

	return S_OK;
    }
#ifdef HAVE_MESAGL
    if ( IsEqualGUID( &IID_IDirect3D, refiid ) )
	return create_direct3d(obj,This);
    if ( IsEqualGUID( &IID_IDirect3D2, refiid ) )
	return create_direct3d2(obj,This);
#endif
    FIXME("(%p):interface for IID %s _NOT_ found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_EnumDisplayModes(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  XVisualInfo *vi;
  XPixmapFormatValues *pf;
  XVisualInfo vt;
  int xbpp = 1, nvisuals, npixmap, i, emu;
  int has_mode[]  = { 0,  0,  0,  0 };
  int has_depth[] = { 8, 15, 16, 24 };
  DDSURFACEDESC	ddsfd;
  static struct {
	int w,h;
  } modes[] = { /* some of the usual modes */
	{512,384},
	{640,400},
	{640,480},
	{800,600},
	{1024,768},
	{1280,1024}
  };
  DWORD maxWidth, maxHeight;

  TRACE("(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
  ddsfd.dwSize = sizeof(ddsfd);
  ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_CAPS|DDSD_PITCH;
  if (dwFlags & DDEDM_REFRESHRATES) {
    ddsfd.dwFlags |= DDSD_REFRESHRATE;
    ddsfd.u.dwRefreshRate = 60;
  }
  maxWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
  maxHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
  
  vi = TSXGetVisualInfo(display, VisualNoMask, &vt, &nvisuals);
  pf = XListPixmapFormats(display, &npixmap);

  i = 0;
  emu = 0;
  while ((i < npixmap) || (emu != 4)) {
    int mode_index = 0;
    int send_mode = 0;
    int j;

    if (i < npixmap) {
      for (j = 0; j < 4; j++) {
	if (has_depth[j] == pf[i].depth) {
	  mode_index = j;
	  break;
	}
      }
      if (j == 4) {
	i++;
	continue;
      }
      

      if (has_mode[mode_index] == 0) {
	if (mode_index == 0) {
	  send_mode = 1;

	  ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;
	  ddsfd.ddpfPixelFormat.dwSize = sizeof(ddsfd.ddpfPixelFormat);
	  ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB|DDPF_PALETTEINDEXED8;
	  ddsfd.ddpfPixelFormat.dwFourCC = 0;
	  ddsfd.ddpfPixelFormat.u.dwRGBBitCount = 8;
	  ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0;
	  ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0;
	  ddsfd.ddpfPixelFormat.u3.dwBBitMask = 0;
	  ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;

	  xbpp = 1;
	  
	  has_mode[mode_index] = 1;
	} else {
      /* All the 'true color' depths (15, 16 and 24)
	 First, find the corresponding visual to extract the bit masks */
	  for (j = 0; j < nvisuals; j++) {
	    if (vi[j].depth == pf[i].depth) {
	      ddsfd.ddsCaps.dwCaps = 0;
	      ddsfd.ddpfPixelFormat.dwSize = sizeof(ddsfd.ddpfPixelFormat);
	      ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	      ddsfd.ddpfPixelFormat.dwFourCC = 0;
	      ddsfd.ddpfPixelFormat.u.dwRGBBitCount = pf[i].bits_per_pixel;
	      ddsfd.ddpfPixelFormat.u1.dwRBitMask = vi[j].red_mask;
	      ddsfd.ddpfPixelFormat.u2.dwGBitMask = vi[j].green_mask;
	      ddsfd.ddpfPixelFormat.u3.dwBBitMask = vi[j].blue_mask;
	      ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;

	      xbpp = pf[i].bits_per_pixel/8;

	      send_mode = 1;
		  has_mode[mode_index] = 1;
	      break;
	    }
	  }
	  if (j == nvisuals)
	    ERR("Did not find visual corresponding the the pixmap format !\n");
	}
      }
      i++;
    } else {
      /* Now to emulated modes */
      if (has_mode[emu] == 0) {
	int c;
	int l;
	int depth = has_depth[emu];
      
	for (c = 0; (c < sizeof(ModeEmulations) / sizeof(Convert)) && (send_mode == 0); c++) {
	  if (ModeEmulations[c].dest.depth == depth) {
	    /* Found an emulation function, now tries to find a matching visual / pixel format pair */
	    for (l = 0; (l < npixmap) && (send_mode == 0); l++) {
	      if ((pf[l].depth == ModeEmulations[c].screen.depth) &&
		  (pf[l].bits_per_pixel == ModeEmulations[c].screen.bpp)) {
		int j;
		for (j = 0; (j < nvisuals) && (send_mode == 0); j++) {
		  if ((vi[j].depth == pf[l].depth) &&
		      (vi[j].red_mask == ModeEmulations[c].screen.rmask) &&
		      (vi[j].green_mask == ModeEmulations[c].screen.gmask) &&
		      (vi[j].blue_mask == ModeEmulations[c].screen.bmask)) {
		    ddsfd.ddpfPixelFormat.dwSize = sizeof(ddsfd.ddpfPixelFormat);
		    ddsfd.ddpfPixelFormat.dwFourCC = 0;
		    if (depth == 8) {
		      ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB|DDPF_PALETTEINDEXED8;
		      ddsfd.ddpfPixelFormat.u.dwRGBBitCount = 8;
		      ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0;
		      ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0;
		      ddsfd.ddpfPixelFormat.u3.dwBBitMask = 0;
    } else {
		      ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		      ddsfd.ddpfPixelFormat.u.dwRGBBitCount = ModeEmulations[c].dest.bpp;
		      ddsfd.ddpfPixelFormat.u1.dwRBitMask = ModeEmulations[c].dest.rmask;
		      ddsfd.ddpfPixelFormat.u2.dwGBitMask = ModeEmulations[c].dest.gmask;
		      ddsfd.ddpfPixelFormat.u3.dwBBitMask = ModeEmulations[c].dest.bmask;
		    }
		    ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;
		    send_mode = 1;
		  }
		  
		  if (send_mode == 0)
		    ERR("No visual corresponding to pixmap format !\n");
		}
	      }
	    }
          }
	}
      }

      emu++;
    }

    if (send_mode) {
      int mode;

      if (TRACE_ON(ddraw)) {
	TRACE("Enumerating with pixel format : \n");
	_dump_pixelformat(&(ddsfd.ddpfPixelFormat));
	DPRINTF("\n");
      }
      
      for (mode = 0; mode < sizeof(modes)/sizeof(modes[0]); mode++) {
	/* Do not enumerate modes we cannot handle anyway */
	if ((modes[mode].w > maxWidth) || (modes[mode].h > maxHeight))
	  break;

	ddsfd.dwWidth = modes[mode].w;
	ddsfd.dwHeight= modes[mode].h;
	ddsfd.lPitch  = ddsfd.dwWidth * xbpp;
	
	/* Now, send the mode description to the application */
	TRACE(" - mode %4ld - %4ld\n", ddsfd.dwWidth, ddsfd.dwHeight);
	if (!modescb(&ddsfd, context))
	  goto exit_enum;
      }

      if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
	/* modeX is not standard VGA */
	ddsfd.dwWidth = 320;
	ddsfd.dwHeight = 200;
	ddsfd.lPitch  = 320 * xbpp;
	if (!modescb(&ddsfd, context))
	  goto exit_enum;
      }
    }
  }
 exit_enum:
  TSXFree(vi);
  TSXFree(pf);

  return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetDisplayMode(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->GetDisplayMode(%p)\n",This,lpddsfd);
    lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
    lpddsfd->dwHeight = This->d.height;
    lpddsfd->dwWidth = This->d.width;
    lpddsfd->lPitch = lpddsfd->dwWidth * PFGET_BPP(This->d.directdraw_pixelformat);
    lpddsfd->dwBackBufferCount = 2;
    lpddsfd->u.dwRefreshRate = 60;
    lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
    lpddsfd->ddpfPixelFormat = This->d.directdraw_pixelformat;
    if (TRACE_ON(ddraw))
	_dump_surface_desc(lpddsfd);
    return DD_OK;
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(xlib_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

ICOM_VTABLE(IDirectDraw) xlib_ddvt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)Xlib_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)Xlib_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)Xlib_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)Xlib_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)Xlib_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)Xlib_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)Xlib_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)Xlib_IDirectDraw2Impl_SetCooperativeLevel,
    Xlib_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
};

#undef XCAST

/*****************************************************************************
 * 	IDirectDraw2
 *
 */

static HRESULT WINAPI Xlib_IDirectDraw2Impl_SetDisplayMode(
    LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD dwRefreshRate,DWORD dwFlags
) {
    FIXME( "Ignored parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
    return Xlib_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetAvailableVidMem(
    LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->(%p,%p,%p)\n",This,ddscaps,total,free);
    if (total) *total = 16* 1024 * 1024;
    if (free) *free = 16* 1024 * 1024;
    return DD_OK;
}

ICOM_VTABLE(IDirectDraw2) xlib_dd2vt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    Xlib_IDirectDraw2Impl_QueryInterface,
    IDirectDraw2Impl_AddRef,
    Xlib_IDirectDraw2Impl_Release,
    IDirectDraw2Impl_Compact,
    IDirectDraw2Impl_CreateClipper,
    Xlib_IDirectDraw2Impl_CreatePalette,
    Xlib_IDirectDraw2Impl_CreateSurface,
    IDirectDraw2Impl_DuplicateSurface,
    Xlib_IDirectDraw2Impl_EnumDisplayModes,
    IDirectDraw2Impl_EnumSurfaces,
    IDirectDraw2Impl_FlipToGDISurface,
    Xlib_IDirectDraw2Impl_GetCaps,
    Xlib_IDirectDraw2Impl_GetDisplayMode,
    IDirectDraw2Impl_GetFourCCCodes,
    IDirectDraw2Impl_GetGDISurface,
    IDirectDraw2Impl_GetMonitorFrequency,
    IDirectDraw2Impl_GetScanLine,
    IDirectDraw2Impl_GetVerticalBlankStatus,
    IDirectDraw2Impl_Initialize,
    IDirectDraw2Impl_RestoreDisplayMode,
    Xlib_IDirectDraw2Impl_SetCooperativeLevel,
    Xlib_IDirectDraw2Impl_SetDisplayMode,
    IDirectDraw2Impl_WaitForVerticalBlank,
    Xlib_IDirectDraw2Impl_GetAvailableVidMem	
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(xlib_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

ICOM_VTABLE(IDirectDraw4) xlib_dd4vt = {
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)Xlib_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)Xlib_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)Xlib_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)Xlib_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)Xlib_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)Xlib_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)Xlib_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)Xlib_IDirectDraw2Impl_SetCooperativeLevel,
    XCAST(SetDisplayMode)Xlib_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
    XCAST(GetAvailableVidMem)Xlib_IDirectDraw2Impl_GetAvailableVidMem,
    IDirectDraw4Impl_GetSurfaceFromDC,
    IDirectDraw4Impl_RestoreAllSurfaces,
    IDirectDraw4Impl_TestCooperativeLevel,
    IDirectDraw4Impl_GetDeviceIdentifier
};
#undef XCAST
