/*		DirectDraw IDirectDraw XF86DGA interface
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */
/* XF86DGA:
 * When DirectVideo mode is enabled you can no longer use 'normal' X 
 * applications nor can you switch to a virtual console. Also, enabling
 * only works, if you have switched to the screen where the application
 * is running.
 * Some ways to debug this stuff are:
 * - A terminal connected to the serial port. Can be bought used for cheap.
 *   (This is the method I am using.)
 * - Another machine connected over some kind of network.
 */

/*
 * This file contains the XF86 DGA specific interface functions.
 * We are 'allowed' to call X11 specific IDirectDraw functions.
 */

#include "config.h"

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "winerror.h"
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

#define RESTORE_SIGNALS

DEFAULT_DEBUG_CHANNEL(ddraw);

#include "dga_private.h"

struct ICOM_VTABLE(IDirectDraw)		dga_ddvt;
struct ICOM_VTABLE(IDirectDraw2)	dga_dd2vt;
struct ICOM_VTABLE(IDirectDraw4)	dga_dd4vt;

#ifdef HAVE_LIBXXF86VM
static XF86VidModeModeInfo *orig_mode = NULL;
#endif

#define DDPRIVATE(x) dga_dd_private *ddpriv = ((dga_dd_private*)(x)->private)
#define DPPRIVATE(x) dga_dp_private *dppriv = ((dga_dp_private*)(x)->private)

/*******************************************************************************
 *				IDirectDraw
 */

/* This function is used both by DGA and DGA2 drivers, thus the virtual function table
   is not set here, but in the calling function */
HRESULT WINAPI DGA_IDirectDraw2Impl_CreateSurface_with_VT(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,
    IUnknown *lpunk, void *vtable
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawSurfaceImpl* dsurf;
    DDPRIVATE(This);
    dga_ds_private *dspriv;
    int	i, fbheight = ddpriv->fb_height;

    TRACE("(%p)->(%p,%p,%p)\n",This,lpddsd,lpdsf,lpunk);
    if (TRACE_ON(ddraw)) _dump_surface_desc(lpddsd);

    *lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(
    	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(IDirectDrawSurfaceImpl)
    );
    dsurf = *(IDirectDrawSurfaceImpl**)lpdsf;
    dsurf->private = (dga_ds_private*)HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(dga_ds_private)
    );
    ICOM_VTBL(dsurf) = (ICOM_VTABLE(IDirectDrawSurface)*)vtable;

    dspriv = (dga_ds_private*)dsurf->private;
    IDirectDraw2_AddRef(iface);

    dsurf->ref = 1;
    dsurf->s.ddraw = This;
    dsurf->s.palette = NULL;
    dspriv->fb_height = -1; /* This is to have non-on screen surfaces freed */
    dsurf->s.lpClipper = NULL;

    /* Copy the surface description */
    dsurf->s.surface_desc = *lpddsd;

    if (!(lpddsd->dwFlags & DDSD_WIDTH))
	dsurf->s.surface_desc.dwWidth  = This->d.width;
    if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	dsurf->s.surface_desc.dwHeight = This->d.height;

    dsurf->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT;

    /* Check if this a 'primary surface' or not */
    if ((lpddsd->dwFlags & DDSD_CAPS) &&
	(lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
	/* This is THE primary surface => there is DGA-specific code */

	/* Find a viewport */
	for (i=0;i<32;i++)
	    if (!(ddpriv->vpmask & (1<<i)))
		break;
	TRACE("using viewport %d for a primary surface\n",i);
	/* if i == 32 or maximum ... return error */
	ddpriv->vpmask|=(1<<i);
	lpddsd->lPitch = dsurf->s.surface_desc.lPitch = 
		ddpriv->fb_width*PFGET_BPP(This->d.directdraw_pixelformat);

	dsurf->s.surface_desc.u1.lpSurface =
	    ddpriv->fb_addr + i*fbheight*lpddsd->lPitch;

	dspriv->fb_height = i*fbheight;

	/* Add flags if there were not present */
	dsurf->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;
	dsurf->s.surface_desc.dwWidth = This->d.width;
	dsurf->s.surface_desc.dwHeight = This->d.height;
	TRACE("primary surface: dwWidth=%ld, dwHeight=%ld, lPitch=%ld\n",This->d.width,This->d.height,lpddsd->lPitch);
	/* We put our surface always in video memory */
	SDDSCAPS(dsurf) |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	dsurf->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;
	dsurf->s.chain = NULL;

	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
	    IDirectDrawSurface4Impl*	back;
	    dga_ds_private		*bspriv;
	    int	bbc;

	    for (bbc=lpddsd->dwBackBufferCount;bbc--;) {
	        int i;
	      
		back = (IDirectDrawSurface4Impl*)HeapAlloc(
		    GetProcessHeap(),
		    HEAP_ZERO_MEMORY,
		    sizeof(IDirectDrawSurface4Impl)
		);
		IDirectDraw2_AddRef(iface);
		back->ref = 1;
		ICOM_VTBL(back) = (ICOM_VTABLE(IDirectDrawSurface4)*)vtable;
		back->private = HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(dga_ds_private)
		);
		bspriv = (dga_ds_private*)back->private;

		for (i=0;i<32;i++)
		    if (!(ddpriv->vpmask & (1<<i)))
			break;
		TRACE("using viewport %d for backbuffer %d\n",i, bbc);
		/* if i == 32 or maximum ... return error */
		ddpriv->vpmask|=(1<<i);

		bspriv->fb_height = i*fbheight;
		/* Copy the surface description from the front buffer */
		back->s.surface_desc = dsurf->s.surface_desc;
		/* Change the parameters that are not the same */
		back->s.surface_desc.u1.lpSurface =
		    ddpriv->fb_addr + i*fbheight*lpddsd->lPitch;

		back->s.ddraw = This;
		/* Add relevant info to front and back buffers */
		/* FIXME: backbuffer/frontbuffer handling broken here, but
		 * will be fixed up in _Flip().
		 */
		SDDSCAPS(dsurf) |= DDSCAPS_FRONTBUFFER;
		SDDSCAPS(back) |= DDSCAPS_FLIP|DDSCAPS_BACKBUFFER|DDSCAPS_VIDEOMEMORY;
		back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
		SDDSCAPS(back) &= ~(DDSCAPS_VISIBLE|DDSCAPS_PRIMARYSURFACE);
		IDirectDrawSurface4_AddAttachedSurface((LPDIRECTDRAWSURFACE4)(*lpdsf),(LPDIRECTDRAWSURFACE4)back);
	    }
	}
    } else {
	/* There is no DGA-specific code here...
	 * Go to the common surface creation function
	 */
	return common_off_screen_CreateSurface(This, dsurf);
    }
    return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_CreateSurface(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,
    IUnknown *lpunk
) {
  HRESULT ret;

  ret = DGA_IDirectDraw2Impl_CreateSurface_with_VT(iface, lpddsd, lpdsf, lpunk, &dga_dds4vt);

  return ret;
}


static HRESULT WINAPI DGA_IDirectDrawImpl_SetDisplayMode(
    LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
    ICOM_THIS(IDirectDrawImpl,iface);
    DDPRIVATE(This);
    int	i,mode_count;

    TRACE("(%p)->(%ld,%ld,%ld)\n", This, width, height, depth);

    /* We hope getting the asked for depth */
    if ( _common_depth_to_pixelformat(depth,iface) !=  -1 ) {
	/* I.e. no visual found or emulated */
	ERR("(w=%ld,h=%ld,d=%ld), unsupported depth!\n",width,height,depth);
	return DDERR_UNSUPPORTEDMODE;
    }

    if (This->d.width < width) {
	ERR("SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld\n",width,height,depth,width,This->d.width);
	return DDERR_UNSUPPORTEDMODE;
    }
    This->d.width	= width;
    This->d.height	= height;

    /* adjust fb_height, so we don't overlap */
    if (ddpriv->fb_height < height)
	ddpriv->fb_height = height;
    _common_IDirectDrawImpl_SetDisplayMode(This);

#ifdef HAVE_LIBXXF86VM
    {
	XF86VidModeModeInfo **all_modes, *vidmode = NULL;
	XF86VidModeModeLine mod_tmp;
	/* int dotclock_tmp; */

	/* save original video mode and set fullscreen if available*/
	orig_mode = (XF86VidModeModeInfo *)malloc(sizeof(XF86VidModeModeInfo));  
	TSXF86VidModeGetModeLine(display, DefaultScreen(display), &orig_mode->dotclock, &mod_tmp);
	orig_mode->hdisplay = mod_tmp.hdisplay; 
	orig_mode->hsyncstart = mod_tmp.hsyncstart;
	orig_mode->hsyncend = mod_tmp.hsyncend; 
	orig_mode->htotal = mod_tmp.htotal;
	orig_mode->vdisplay = mod_tmp.vdisplay; 
	orig_mode->vsyncstart = mod_tmp.vsyncstart;
	orig_mode->vsyncend = mod_tmp.vsyncend; 
	orig_mode->vtotal = mod_tmp.vtotal;
	orig_mode->flags = mod_tmp.flags; 
	orig_mode->private = mod_tmp.private;

	TSXF86VidModeGetAllModeLines(display,DefaultScreen(display),&mode_count,&all_modes);
	for (i=0;i<mode_count;i++) {
	    if (all_modes[i]->hdisplay == width &&
		all_modes[i]->vdisplay == height
	    ) {
		vidmode = (XF86VidModeModeInfo *)malloc(sizeof(XF86VidModeModeInfo));
		*vidmode = *(all_modes[i]);
		break;
	    } else
		TSXFree(all_modes[i]->private);
	}
	for (i++;i<mode_count;i++) TSXFree(all_modes[i]->private);
	    TSXFree(all_modes);

	if (!vidmode)
	    WARN("Fullscreen mode not available!\n");

	if (vidmode) {
	    TRACE("SwitchToMode(%dx%d)\n",vidmode->hdisplay,vidmode->vdisplay);
	    TSXF86VidModeSwitchToMode(display, DefaultScreen(display), vidmode);
#if 0 /* This messes up my screen (XF86_Mach64, 3.3.2.3a) for some reason, and should now be unnecessary */
	    TSXF86VidModeSetViewPort(display, DefaultScreen(display), 0, 0);
#endif
	}
    }
#endif

    /* FIXME: this function OVERWRITES several signal handlers. 
     * can we save them? and restore them later? In a way that
     * it works for the library too?
     */
    TSXF86DGADirectVideo(display,DefaultScreen(display),XF86DGADirectGraphics);
    TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);

#ifdef RESTORE_SIGNALS
    SIGNAL_Init();
#endif
    return DD_OK;
}

HRESULT WINAPI DGA_IDirectDraw2Impl_GetCaps(
    LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);

    TRACE("(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);
    if (!caps1 && !caps2)
       return DDERR_INVALIDPARAMS;
    if (caps1) {
	caps1->dwVidMemTotal = ddpriv->fb_memsize;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
    }
    if (caps2) {
	caps2->dwVidMemTotal = ddpriv->fb_memsize;
	caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
    }
    return DD_OK;
}

#if 0 /* Not used as of now.... */
static void fill_caps(LPDDCAPS caps) {
    /* This function tries to fill the capabilities of Wine's DDraw
     * implementation. Needs to be fixed, though.. */
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
    /* I put here 8 Mo so that D3D applications will believe they have enough
     * memory to put textures in video memory.
     * BTW, is this only frame buffer memory or also texture memory (for Voodoo
     * boards for example) ?
     */
    caps->dwVidMemTotal = 8192 * 1024;
    caps->dwVidMemFree = 8192 * 1024;
    /* These are all the supported capabilities of the surfaces */
    caps->ddsCaps.dwCaps = DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
    DDSCAPS_FRONTBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN |
      /*DDSCAPS_OVERLAY |*/ DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
	DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;
#ifdef HAVE_OPENGL
    caps->dwCaps |= DDCAPS_3D | DDCAPS_ZBLTS;
    caps->dwCaps2 |=  DDCAPS2_NO2DDURING3DSCENE;
    caps->ddsCaps.dwCaps |= DDSCAPS_3DDEVICE | DDSCAPS_MIPMAP | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER;
#endif
}
#endif

static HRESULT WINAPI DGA_IDirectDraw2Impl_CreatePalette(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawPaletteImpl*	ddpal;
    dga_dp_private		*dppriv;
    HRESULT			res;
    int 			xsize = 0,i;

    TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,lpddpal,lpunk);
    res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,(IDirectDrawPaletteImpl**)lpddpal,lpunk,&xsize);
    if (res != 0)
	return res;
    ddpal = *(IDirectDrawPaletteImpl**)lpddpal;
    ddpal->private = HeapAlloc(
	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(dga_dp_private)
    );
    dppriv = (dga_dp_private*)ddpal->private;

    ICOM_VTBL(ddpal)= &dga_ddpalvt;
    if (This->d.directdraw_pixelformat.u.dwRGBBitCount<=8) {
	dppriv->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);
    } else {
	ERR("why are we doing CreatePalette in hi/truecolor?\n");
	dppriv->cm = 0;
    }
    if (dppriv->cm && xsize) {
	for (i=0;i<xsize;i++) {
	    XColor xc;

	    xc.red = ddpal->palents[i].peRed<<8;
	    xc.blue = ddpal->palents[i].peBlue<<8;
	    xc.green = ddpal->palents[i].peGreen<<8;
	    xc.flags = DoRed|DoBlue|DoGreen;
	    xc.pixel = i;
	    TSXStoreColor(display,dppriv->cm,&xc);
	}
    }
    return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->()\n",This);
    Sleep(1000);
    TSXF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
    SIGNAL_Init();
#endif
    return DD_OK;
}

static ULONG WINAPI DGA_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
      VirtualFree(ddpriv->fb_addr, 0, MEM_RELEASE);
      TSXF86DGADirectVideo(display,DefaultScreen(display),0);
      if (This->d.window && GetPropA(This->d.window,ddProp))
	DestroyWindow(This->d.window);
#ifdef HAVE_LIBXXF86VM
      if (orig_mode) {
	TSXF86VidModeSwitchToMode(
				  display,
				  DefaultScreen(display),
				  orig_mode
				  );
	if (orig_mode->privsize)
		TSXFree(orig_mode->private);		
	free(orig_mode);
	orig_mode = NULL;
      }
#endif
	
#ifdef RESTORE_SIGNALS
	SIGNAL_Init();
#endif
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

HRESULT WINAPI DGA_IDirectDraw2Impl_QueryInterface(
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
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&dga_ddvt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw interface (%p)\n", *obj);
	
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) ) {
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&dga_dd2vt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw2 interface (%p)\n", *obj);

	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) ) {
	ICOM_VTBL(This) = (ICOM_VTABLE(IDirectDraw2)*)&dga_dd4vt;
	IDirectDraw2_AddRef(iface);
	*obj = This;

	TRACE("  Creating IDirectDraw4 interface (%p)\n", *obj);

	return S_OK;
    }
    FIXME("(%p):interface for IID %s _NOT_ found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_EnumDisplayModes(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDSURFACEDESC	ddsfd;
    static struct {
	    int w,h;
    } modes[5] = { /* some usual modes */
	{512,384},
	{640,400},
	{640,480},
	{800,600},
	{1024,768},
    };
    static int depths[4] = {8,16,24,32};
    int	i,j;

    TRACE("(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
    ddsfd.dwSize = sizeof(ddsfd);
    ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
    if (dwFlags & DDEDM_REFRESHRATES) {
	    ddsfd.dwFlags |= DDSD_REFRESHRATE;
	    ddsfd.u.dwRefreshRate = 60;
    }
    ddsfd.ddsCaps.dwCaps = 0;
    ddsfd.dwBackBufferCount = 1;

    for (i=0;i<sizeof(depths)/sizeof(depths[0]);i++) {
      ddsfd.dwBackBufferCount = 1;
      ddsfd.ddpfPixelFormat.dwFourCC	= 0;
      ddsfd.ddpfPixelFormat.dwFlags 	= DDPF_RGB;
      ddsfd.ddpfPixelFormat.u.dwRGBBitCount	= depths[i];
      /* FIXME: those masks would have to be set in depth > 8 */
      if (depths[i]==8) {
	ddsfd.ddpfPixelFormat.u1.dwRBitMask  	= 0;
	ddsfd.ddpfPixelFormat.u2.dwGBitMask  	= 0;
	ddsfd.ddpfPixelFormat.u3.dwBBitMask 	= 0;
	ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;
	ddsfd.ddsCaps.dwCaps=DDSCAPS_PALETTE;
	ddsfd.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
      } else {
	ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;
	
	/* FIXME: We should query those from X itself */
	switch (depths[i]) {
	case 16:
	  ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0xF800;
	  ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0x07E0;
	  ddsfd.ddpfPixelFormat.u3.dwBBitMask= 0x001F;
	  break;
	case 24:
	  ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0x00FF0000;
	  ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0x0000FF00;
	  ddsfd.ddpfPixelFormat.u3.dwBBitMask= 0x000000FF;
	  break;
	case 32:
	  ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0x00FF0000;
	  ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0x0000FF00;
	  ddsfd.ddpfPixelFormat.u3.dwBBitMask= 0x000000FF;
	  break;
	}
      }
      
      ddsfd.dwWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
      ddsfd.dwHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
      TRACE(" enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
      if (!modescb(&ddsfd,context)) return DD_OK;
      
      for (j=0;j<sizeof(modes)/sizeof(modes[0]);j++) {
	ddsfd.dwWidth	= modes[j].w;
	ddsfd.dwHeight	= modes[j].h;
	TRACE(" enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
	if (!modescb(&ddsfd,context)) return DD_OK;
      }
      
      if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
	/* modeX is not standard VGA */
	
	ddsfd.dwHeight = 200;
	ddsfd.dwWidth = 320;
	TRACE(" enumerating (320x200x%d)\n",depths[i]);
	if (!modescb(&ddsfd,context)) return DD_OK;
      }
    }

    return DD_OK;
}

HRESULT WINAPI DGA_IDirectDraw2Impl_GetDisplayMode(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);

    TRACE("(%p)->(%p)\n",This,lpddsfd);
    lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
    lpddsfd->dwHeight = This->d.height;
    lpddsfd->dwWidth = This->d.width;
    lpddsfd->lPitch = ddpriv->fb_width*PFGET_BPP(This->d.directdraw_pixelformat);
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
# define XCAST(fun)	(typeof(dga_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

struct ICOM_VTABLE(IDirectDraw) dga_ddvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)DGA_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)DGA_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)DGA_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)DGA_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)DGA_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)DGA_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)DGA_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)DGA_IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)IDirectDraw2Impl_SetCooperativeLevel,
    DGA_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
};
#undef XCAST

/*****************************************************************************
 * 	IDirectDraw2
 *
 */

static HRESULT WINAPI DGA_IDirectDraw2Impl_SetDisplayMode(
    LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD dwRefreshRate, DWORD dwFlags
) {
    FIXME( "Ignored parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
    return DGA_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

HRESULT WINAPI DGA_IDirectDraw2Impl_GetAvailableVidMem(
    LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    DDPRIVATE(This);

    TRACE("(%p)->(%p,%p,%p)\n",This,ddscaps,total,free);
    if (total) *total = ddpriv->fb_memsize * 1024;
    if (free) *free = ddpriv->fb_memsize * 1024;
    return DD_OK;
}

ICOM_VTABLE(IDirectDraw2) dga_dd2vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    DGA_IDirectDraw2Impl_QueryInterface,
    IDirectDraw2Impl_AddRef,
    DGA_IDirectDraw2Impl_Release,
    IDirectDraw2Impl_Compact,
    IDirectDraw2Impl_CreateClipper,
    DGA_IDirectDraw2Impl_CreatePalette,
    DGA_IDirectDraw2Impl_CreateSurface,
    IDirectDraw2Impl_DuplicateSurface,
    DGA_IDirectDraw2Impl_EnumDisplayModes,
    IDirectDraw2Impl_EnumSurfaces,
    IDirectDraw2Impl_FlipToGDISurface,
    DGA_IDirectDraw2Impl_GetCaps,
    DGA_IDirectDraw2Impl_GetDisplayMode,
    IDirectDraw2Impl_GetFourCCCodes,
    IDirectDraw2Impl_GetGDISurface,
    IDirectDraw2Impl_GetMonitorFrequency,
    IDirectDraw2Impl_GetScanLine,
    IDirectDraw2Impl_GetVerticalBlankStatus,
    IDirectDraw2Impl_Initialize,
    DGA_IDirectDraw2Impl_RestoreDisplayMode,
    IDirectDraw2Impl_SetCooperativeLevel,
    DGA_IDirectDraw2Impl_SetDisplayMode,
    IDirectDraw2Impl_WaitForVerticalBlank,
    DGA_IDirectDraw2Impl_GetAvailableVidMem
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

ICOM_VTABLE(IDirectDraw4) dga_dd4vt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface)DGA_IDirectDraw2Impl_QueryInterface,
    XCAST(AddRef)IDirectDraw2Impl_AddRef,
    XCAST(Release)DGA_IDirectDraw2Impl_Release,
    XCAST(Compact)IDirectDraw2Impl_Compact,
    XCAST(CreateClipper)IDirectDraw2Impl_CreateClipper,
    XCAST(CreatePalette)DGA_IDirectDraw2Impl_CreatePalette,
    XCAST(CreateSurface)DGA_IDirectDraw2Impl_CreateSurface,
    XCAST(DuplicateSurface)IDirectDraw2Impl_DuplicateSurface,
    XCAST(EnumDisplayModes)DGA_IDirectDraw2Impl_EnumDisplayModes,
    XCAST(EnumSurfaces)IDirectDraw2Impl_EnumSurfaces,
    XCAST(FlipToGDISurface)IDirectDraw2Impl_FlipToGDISurface,
    XCAST(GetCaps)DGA_IDirectDraw2Impl_GetCaps,
    XCAST(GetDisplayMode)DGA_IDirectDraw2Impl_GetDisplayMode,
    XCAST(GetFourCCCodes)IDirectDraw2Impl_GetFourCCCodes,
    XCAST(GetGDISurface)IDirectDraw2Impl_GetGDISurface,
    XCAST(GetMonitorFrequency)IDirectDraw2Impl_GetMonitorFrequency,
    XCAST(GetScanLine)IDirectDraw2Impl_GetScanLine,
    XCAST(GetVerticalBlankStatus)IDirectDraw2Impl_GetVerticalBlankStatus,
    XCAST(Initialize)IDirectDraw2Impl_Initialize,
    XCAST(RestoreDisplayMode)DGA_IDirectDraw2Impl_RestoreDisplayMode,
    XCAST(SetCooperativeLevel)IDirectDraw2Impl_SetCooperativeLevel,
    XCAST(SetDisplayMode)DGA_IDirectDrawImpl_SetDisplayMode,
    XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
    XCAST(GetAvailableVidMem)DGA_IDirectDraw2Impl_GetAvailableVidMem,
    IDirectDraw4Impl_GetSurfaceFromDC,
    IDirectDraw4Impl_RestoreAllSurfaces,
    IDirectDraw4Impl_TestCooperativeLevel,
    IDirectDraw4Impl_GetDeviceIdentifier
};
#undef XCAST
