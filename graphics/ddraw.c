/*		DirectDraw using DGA or Xlib
 *
 * Copyright 1997,1998 Marcus Meissner
 */
/* When DirectVideo mode is enabled you can no longer use 'normal' X 
 * applications nor can you switch to a virtual console. Also, enabling
 * only works, if you have switched to the screen where the application
 * is running.
 * Some ways to debug this stuff are:
 * - A terminal connected to the serial port. Can be bought used for cheap.
 *   (This is the method I am using.)
 * - Another machine connected over some kind of network.
 *
 * FIXME: The Xshm implementation has been temporarily removed. It will be
 * later reintegrated into the Xlib implementation.
 */

#include "config.h"
#include <unistd.h>
#include <assert.h>
#include "ts_xlib.h"
#include <sys/signal.h>

#include "windows.h"
#include "winerror.h"
#include "interfaces.h"
#include "gdi.h"
#include "heap.h"
#include "ldt.h"
#include "dc.h"
#include "win.h"
#include "miscemu.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "compobj.h"
#include "spy.h"
#include "message.h"

#ifdef HAVE_LIBXXF86DGA
#include "ts_xf86dga.h"
#endif

/* define this if you want to play Diablo using XF86DGA. (bug workaround) */
#undef DIABLO_HACK

/* restore signal handlers overwritten by XF86DGA 
 * this is a define, for it will only work in emulator mode
 */
#undef RESTORE_SIGNALS

/* Where do these GUIDs come from?  mkuuid.
 * They exist solely to distinguish between the targets Wine support,
 * and should be different than any other GUIDs in existence.
 */
static GUID DGA_DirectDraw_GUID = { /* e2dcb020-dc60-11d1-8407-9714f5d50802 */
	0xe2dcb020,
	0xdc60,
	0x11d1,
	{0x84, 0x07, 0x97, 0x14, 0xf5, 0xd5, 0x08, 0x02}
};

static GUID XLIB_DirectDraw_GUID = { /* 1574a740-dc61-11d1-8407-f7875a7d1879 */
    0x1574a740,
    0xdc61,
    0x11d1,
    {0x84, 0x07, 0xf7, 0x87, 0x5a, 0x7d, 0x18, 0x79}
};

static struct IDirectDrawSurface3_VTable	dga_dds3vt, xlib_dds3vt;
static struct IDirectDraw_VTable		dga_ddvt, xlib_ddvt;
static struct IDirectDraw2_VTable		dga_dd2vt, xlib_dd2vt;
static struct IDirectDrawClipper_VTable	ddclipvt;
static struct IDirectDrawPalette_VTable dga_ddpalvt, xlib_ddpalvt;
static struct IDirect3D_VTable			d3dvt;
static struct IDirect3D2_VTable			d3d2vt;

BOOL32
DDRAW_DGA_Available()
{
#ifdef HAVE_LIBXXF86DGA
	int evbase, evret;
	return (getuid() == 0)&&TSXF86DGAQueryExtension(display,&evbase,&evret);
#else /* defined(HAVE_LIBXXF86DGA) */
	return 0;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	if (DDRAW_DGA_Available()) {
		ddenumproc(&DGA_DirectDraw_GUID,"WINE with XFree86 DGA","display",data);
	}
	ddenumproc(&XLIB_DirectDraw_GUID,"WINE with Xlib","display",data);
	ddenumproc(NULL,"WINE","display",data);
	return 0;
}

/* What is this doing here? */
HRESULT WINAPI 
DSoundHelp(DWORD x,DWORD y,DWORD z) {
	FIXME(ddraw,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x,y,z);
	return 0;
}


/******************************************************************************
 *		internal helper functions
 */
static void _dump_DDBLTFX(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDBLTFX_ARITHSTRETCHY)
		FE(DDBLTFX_MIRRORLEFTRIGHT)
		FE(DDBLTFX_MIRRORUPDOWN)
		FE(DDBLTFX_NOTEARING)
		FE(DDBLTFX_ROTATE180)
		FE(DDBLTFX_ROTATE270)
		FE(DDBLTFX_ROTATE90)
		FE(DDBLTFX_ZBUFFERRANGE)
		FE(DDBLTFX_ZBUFFERBASEDEST)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
	   if (flags[i].mask & flagmask) {
	      DUMP("%s ",flags[i].name);
	      
	   };
	DUMP("\n");
	
}

static void _dump_DDBLTFAST(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDBLTFAST_NOCOLORKEY)
		FE(DDBLTFAST_SRCCOLORKEY)
		FE(DDBLTFAST_DESTCOLORKEY)
		FE(DDBLTFAST_WAIT)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DUMP("%s ",flags[i].name);
	DUMP("\n");
}

static void _dump_DDBLT(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDBLT_ALPHADEST)
		FE(DDBLT_ALPHADESTCONSTOVERRIDE)
		FE(DDBLT_ALPHADESTNEG)
		FE(DDBLT_ALPHADESTSURFACEOVERRIDE)
		FE(DDBLT_ALPHAEDGEBLEND)
		FE(DDBLT_ALPHASRC)
		FE(DDBLT_ALPHASRCCONSTOVERRIDE)
		FE(DDBLT_ALPHASRCNEG)
		FE(DDBLT_ALPHASRCSURFACEOVERRIDE)
		FE(DDBLT_ASYNC)
		FE(DDBLT_COLORFILL)
		FE(DDBLT_DDFX)
		FE(DDBLT_DDROPS)
		FE(DDBLT_KEYDEST)
		FE(DDBLT_KEYDESTOVERRIDE)
		FE(DDBLT_KEYSRC)
		FE(DDBLT_KEYSRCOVERRIDE)
		FE(DDBLT_ROP)
		FE(DDBLT_ROTATIONANGLE)
		FE(DDBLT_ZBUFFER)
		FE(DDBLT_ZBUFFERDESTCONSTOVERRIDE)
		FE(DDBLT_ZBUFFERDESTOVERRIDE)
		FE(DDBLT_ZBUFFERSRCCONSTOVERRIDE)
		FE(DDBLT_ZBUFFERSRCOVERRIDE)
		FE(DDBLT_WAIT)
		FE(DDBLT_DEPTHFILL)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DUMP("%s ",flags[i].name);
}

static void _dump_DDSCAPS(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDSCAPS_RESERVED1)
		FE(DDSCAPS_ALPHA)
		FE(DDSCAPS_BACKBUFFER)
		FE(DDSCAPS_COMPLEX)
		FE(DDSCAPS_FLIP)
		FE(DDSCAPS_FRONTBUFFER)
		FE(DDSCAPS_OFFSCREENPLAIN)
		FE(DDSCAPS_OVERLAY)
		FE(DDSCAPS_PALETTE)
		FE(DDSCAPS_PRIMARYSURFACE)
		FE(DDSCAPS_PRIMARYSURFACELEFT)
		FE(DDSCAPS_SYSTEMMEMORY)
		FE(DDSCAPS_TEXTURE)
		FE(DDSCAPS_3DDEVICE)
		FE(DDSCAPS_VIDEOMEMORY)
		FE(DDSCAPS_VISIBLE)
		FE(DDSCAPS_WRITEONLY)
		FE(DDSCAPS_ZBUFFER)
		FE(DDSCAPS_OWNDC)
		FE(DDSCAPS_LIVEVIDEO)
		FE(DDSCAPS_HWCODEC)
		FE(DDSCAPS_MODEX)
		FE(DDSCAPS_MIPMAP)
		FE(DDSCAPS_RESERVED2)
		FE(DDSCAPS_ALLOCONLOAD)
		FE(DDSCAPS_VIDEOPORT)
		FE(DDSCAPS_LOCALVIDMEM)
		FE(DDSCAPS_NONLOCALVIDMEM)
		FE(DDSCAPS_STANDARDVGAMODE)
		FE(DDSCAPS_OPTIMIZED)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DUMP("%s ",flags[i].name);
	DUMP("\n");
}

static void _dump_DDSD(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
		FE(DDSD_CAPS)
		FE(DDSD_HEIGHT)
		FE(DDSD_WIDTH)
		FE(DDSD_PITCH)
		FE(DDSD_BACKBUFFERCOUNT)
		FE(DDSD_ZBUFFERBITDEPTH)
		FE(DDSD_ALPHABITDEPTH)
		FE(DDSD_PIXELFORMAT)
		FE(DDSD_CKDESTOVERLAY)
		FE(DDSD_CKDESTBLT)
		FE(DDSD_CKSRCOVERLAY)
		FE(DDSD_CKSRCBLT)
		FE(DDSD_MIPMAPCOUNT)
		FE(DDSD_REFRESHRATE)
		FE(DDSD_LINEARSIZE)
		FE(DDSD_LPSURFACE)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DUMP("%s ",flags[i].name);
	DUMP("\n");
}

static int _getpixelformat(LPDIRECTDRAW2 ddraw,LPDDPIXELFORMAT pf) {
	static XVisualInfo	*vi;
	XVisualInfo		vt;
	int			nitems;

	if (!vi)
		vi = TSXGetVisualInfo(display,VisualNoMask,&vt,&nitems);

	pf->dwFourCC = 0;
	if (ddraw->d.depth==8) {
		pf->dwFlags 		= DDPF_RGB|DDPF_PALETTEINDEXED8;
		pf->x.dwRGBBitCount	= 8;
		pf->y.dwRBitMask  	= 0;
		pf->z.dwGBitMask  	= 0;
		pf->xx.dwBBitMask 	= 0;
		pf->xy.dwRGBAlphaBitMask= 0;
		return 0;
	}
	if (ddraw->d.depth==16) {
		pf->dwFlags       	= DDPF_RGB;
		pf->x.dwRGBBitCount	= 16;
		pf->y.dwRBitMask	= vi[0].red_mask;
		pf->z.dwGBitMask	= vi[0].green_mask;
		pf->xx.dwBBitMask	= vi[0].blue_mask;
		pf->xy.dwRGBAlphaBitMask= 0;
		return 0;
	}
	FIXME(ddraw,"_getpixelformat:unknown depth %ld?\n",ddraw->d.depth);
	return DDERR_GENERIC;
}

/******************************************************************************
 *		IDirectDrawSurface,IDirectDrawSurface2,IDirectDrawSurface3
 *
 * Since DDS3 and DDS2 are supersets of DDS, we implement DDS3 and let
 * DDS and DDS2 use those functions. (Function calls did not change (except
 * using different DirectDrawSurfaceX version), just added flags and functions)
 */
static HRESULT WINAPI IDirectDrawSurface3_Lock(
    LPDIRECTDRAWSURFACE3 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
        TRACE(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd);
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	    WARN(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
			 this,lprect,lpddsd,flags,(DWORD)hnd);

	if (lprect) {
		FIXME(ddraw,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		lpddsd->y.lpSurface = this->s.surface +
			(lprect->top*this->s.lpitch) +
			(lprect->left*(this->s.ddraw->d.depth/8));
	} else {
		assert(this->s.surface);
		lpddsd->y.lpSurface = this->s.surface;
	}
	lpddsd->dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_LPSURFACE;
	lpddsd->dwWidth		= this->s.width;
	lpddsd->dwHeight	= this->s.height;
	lpddsd->lPitch		= this->s.lpitch;
	_getpixelformat(this->s.ddraw,&(lpddsd->ddpfPixelFormat));
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface)
{
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);

	if (!this->s.ddraw->e.xlib.paintable)
		return DD_OK;

  /* Only redraw the screen when unlocking the buffer that is on screen */
  if ((this->t.xlib.image != NULL) && (this->t.xlib.on_screen))
	TSXPutImage(		display,
				this->s.ddraw->e.xlib.drawable,
				DefaultGCOfScreen(screen),
				this->t.xlib.image,
				0, 0, 0, 0,
				this->t.xlib.image->width,
		this->t.xlib.image->height);
  
	if (this->s.palette && this->s.palette->cm)
		TSXSetWindowColormap(display,this->s.ddraw->e.xlib.drawable,this->s.palette->cm);

	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,flipto->t.dga.fb_height);

	if (flipto->s.palette && flipto->s.palette->cm) {
		TSXF86DGAInstallColormap(display,DefaultScreen(display),flipto->s.palette->cm);
	}
	while (!TSXF86DGAViewPortChanged(display,DefaultScreen(display),2)) {
	}
	if (flipto!=this) {
		int	tmp;
		LPVOID	ptmp;

		tmp = this->t.dga.fb_height;
		this->t.dga.fb_height = flipto->t.dga.fb_height;
		flipto->t.dga.fb_height = tmp;

		ptmp = this->s.surface;
		this->s.surface = flipto->s.surface;
		flipto->s.surface = ptmp;
	}
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!this->s.ddraw->e.xlib.paintable)
		return 0;

	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
  
	TSXPutImage(display,
				this->s.ddraw->e.xlib.drawable,
				DefaultGCOfScreen(screen),
				flipto->t.xlib.image,
				0, 0, 0, 0,
				flipto->t.xlib.image->width,
				flipto->t.xlib.image->height);
	TSXSetWindowColormap(display,this->s.ddraw->e.xlib.drawable,this->s.palette->cm);
	if (flipto!=this) {
		XImage *tmp;
		LPVOID	*surf;
		tmp = this->t.xlib.image;
		this->t.xlib.image = flipto->t.xlib.image;
		flipto->t.xlib.image = tmp;
		surf = this->s.surface;
		this->s.surface = flipto->s.surface;
		flipto->s.surface = surf;

		flipto->t.xlib.on_screen = TRUE;
		this->t.xlib.on_screen = FALSE;
	}
	return 0;
}

/* The IDirectDrawSurface3::SetPalette method attaches the specified
 * DirectDrawPalette object to a surface. The surface uses this palette for all
 * subsequent operations. The palette change takes place immediately.
 */
static HRESULT WINAPI Xlib_IDirectDrawSurface3_SetPalette(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWPALETTE pal
) {
	TRACE(ddraw,"(%p)->SetPalette(%p)\n",this,pal);
        /* According to spec, we are only supposed to 
         * AddRef if this is not the same palette.
         */
        if( this->s.palette != pal )
        {
          if( pal != NULL )
            pal->lpvtbl->fnAddRef( pal );
          if( this->s.palette != NULL )
            this->s.palette->lpvtbl->fnRelease( this->s.palette );
	  this->s.palette = pal; 

          /* I think that we need to attach it to all backbuffers...*/
          if( this->s.backbuffer ) {
             if( this->s.backbuffer->s.palette )
               this->s.backbuffer->s.palette->lpvtbl->fnRelease(
                 this->s.backbuffer->s.palette );
             this->s.backbuffer->s.palette = pal;
             if( pal )
                pal->lpvtbl->fnAddRef( pal );
           }
      
          /* Perform the refresh */
          TSXSetWindowColormap(display,this->s.ddraw->e.xlib.drawable,this->s.palette->cm);
        }

	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_SetPalette(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWPALETTE pal
) {
	TRACE(ddraw,"(%p)->SetPalette(%p)\n",this,pal);
#ifdef HAVE_LIBXXF86DGA
        /* According to spec, we are only supposed to 
         * AddRef if this is not the same palette.
         */
        if( this->s.palette != pal )
        {
          if( pal != NULL ) 
	  	pal->lpvtbl->fnAddRef( pal );
          if( this->s.palette != NULL )
            this->s.palette->lpvtbl->fnRelease( this->s.palette );
	  this->s.palette = pal; 

          /* I think that we need to attach it to all backbuffers...*/
          if( this->s.backbuffer ) {
             if( this->s.backbuffer->s.palette )
               this->s.backbuffer->s.palette->lpvtbl->fnRelease(this->s.backbuffer->s.palette );
             this->s.backbuffer->s.palette = pal;
             if( pal ) pal->lpvtbl->fnAddRef( pal );
          }
	  TSXF86DGAInstallColormap(display,DefaultScreen(display),this->s.palette->cm);
        }
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */


}

static HRESULT WINAPI IDirectDrawSurface3_Blt(
	LPDIRECTDRAWSURFACE3 this,LPRECT32 rdst,LPDIRECTDRAWSURFACE3 src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	RECT32	xdst,xsrc;
	int	i,j;

	TRACE(ddraw,"(%p)->(%p,%p,%p,%08lx,%p)\n",
	      this,rdst,src,rsrc,dwFlags,lpbltfx);
	if (rdst) TRACE(ddraw,"	destrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	if (rsrc) TRACE(ddraw,"	srcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	TRACE(ddraw,"\tflags: ");_dump_DDBLT(dwFlags);fprintf(stderr,"\n");
	if (dwFlags & DDBLT_DDFX) {
	  TRACE(ddraw,"	blitfx: \n");_dump_DDBLTFX(lpbltfx->dwDDFX);
	}
		
	if (rdst) {
		memcpy(&xdst,rdst,sizeof(xdst));
	} else {
		xdst.top	= 0;
		xdst.bottom	= this->s.height;
		xdst.left	= 0;
		xdst.right	= this->s.width;
	}

	if (rsrc) {
		memcpy(&xsrc,rsrc,sizeof(xsrc));
	} else if (src) {
		xsrc.top	= 0;
		xsrc.bottom	= src->s.height;
		xsrc.left	= 0;
		xsrc.right	= src->s.width;
	}

	if (dwFlags & DDBLT_COLORFILL) {
		int	bpp = this->s.ddraw->d.depth/8;
		LPBYTE	xline,xpixel;

		xline = (LPBYTE)this->s.surface+xdst.top*this->s.lpitch;
		for (i=xdst.top;i<xdst.bottom;i++) {
			xpixel = xline+bpp*xdst.left;

			for (j=xdst.left;j<xdst.right;j++) {
				/* FIXME: this only works on little endian
				 * architectures, where DWORD starts with low
				 * byte first!
				 */
				memcpy(xpixel,&(lpbltfx->b.dwFillColor),bpp);
				xpixel += bpp;
			}
			xline += this->s.lpitch;
		}
		dwFlags &= ~(DDBLT_COLORFILL);
	}
	dwFlags &= ~(DDBLT_WAIT|DDBLT_ASYNC);/* FIXME: can't handle right now */
	if (	(xsrc.top ==0) && (xsrc.bottom ==this->s.height) &&
		(xsrc.left==0) && (xsrc.right  ==this->s.width) &&
		(xdst.top ==0) && (xdst.bottom ==this->s.height) &&
		(xdst.left==0) && (xdst.right  ==this->s.width)  &&
		!dwFlags
	) {
		memcpy(this->s.surface,src->s.surface,this->s.height*this->s.lpitch);
		return 0;
	} else {
	  /* Non full screen Blit. In this case, we need to copy line per line.
	     WARNING : if the program behaves badly (ie sizes of structures are different
	               or buffer not big enough) this may crash Wine... */
	  int bpp = this->s.ddraw->d.depth / 8;
	  int height = xsrc.bottom - xsrc.top;
	  int width = (xsrc.right - xsrc.left) * bpp;
	  int h;

	  for (h = 0; h < height; h++) {
	    memcpy(this->s.surface + ((h + xdst.top) * this->s.lpitch) + xdst.left * bpp,
		   src->s.surface + ((h + xsrc.top) * src->s.lpitch) + xsrc.left * bpp,
		   width);
	  }
	}
	
	if (dwFlags) {
	  TRACE(ddraw,"\tUnsupported flags: ");_dump_DDBLT(dwFlags);fprintf(stderr,"\n");
	}
	
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_Blt(
	LPDIRECTDRAWSURFACE3 this,LPRECT32 rdst,LPDIRECTDRAWSURFACE3 src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
  HRESULT ret;
  
  /* First, call the "common" blit function */
  ret = IDirectDrawSurface3_Blt(this, rdst, src, rsrc, dwFlags, lpbltfx);

  /* Then put the result on screen if blited on main screen buffer */
  if (!this->s.ddraw->e.xlib.paintable)
    return ret;

  if ((this->t.xlib.image != NULL) && (this->t.xlib.on_screen))
    TSXPutImage(display,
		this->s.ddraw->e.xlib.drawable,
		DefaultGCOfScreen(screen),
		this->t.xlib.image,
		0, 0, 0, 0,
		this->t.xlib.image->width,
		this->t.xlib.image->height);
  if (this->s.palette && this->s.palette->cm)
    TSXSetWindowColormap(display,this->s.ddraw->e.xlib.drawable,this->s.palette->cm);

  return ret;
}

static HRESULT WINAPI IDirectDrawSurface3_BltFast(
	LPDIRECTDRAWSURFACE3 this,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE3 src,LPRECT32 rsrc,DWORD trans
) {
	int	i,bpp;
	if (TRACE_ON(ddraw)) {
	    FIXME(ddraw,"(%p)->(%ld,%ld,%p,%p,%08lx),stub!\n",
		    this,dstx,dsty,src,rsrc,trans
	    );
	    TRACE(ddraw,"	trans:");_dump_DDBLTFAST(trans);fprintf(stderr,"\n");
	    TRACE(ddraw,"	srcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	}
	bpp = this->s.ddraw->d.depth/8;
	for (i=0;i<rsrc->bottom-rsrc->top;i++) {
		memcpy(	this->s.surface+((i+dsty)*this->s.width*bpp)+dstx*bpp,
			src->s.surface +(rsrc->top+i)*src->s.width*bpp+rsrc->left*bpp,
			(rsrc->right-rsrc->left)*bpp
		);
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_BltBatch(
	LPDIRECTDRAWSURFACE3 this,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
	FIXME(ddraw,"(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		this,ddbltbatch,x,y
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_GetCaps(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS caps
) {
	TRACE(ddraw,"(%p)->GetCaps(%p)\n",this,caps);
	caps->dwCaps = DDCAPS_PALETTE; /* probably more */
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE3 this,LPDDSURFACEDESC ddsd
) { 
	if (TRACE_ON(ddraw)) {
		TRACE(ddraw, "(%p)->GetSurfaceDesc(%p)\n",
			     this,ddsd);
		fprintf(stderr,"	flags: ");
		_dump_DDSD(ddsd->dwFlags);
		fprintf(stderr,"\n");
	}

	ddsd->dwFlags |= DDSD_PIXELFORMAT|DDSD_CAPS|DDSD_BACKBUFFERCOUNT|DDSD_HEIGHT|DDSD_WIDTH;
	ddsd->ddsCaps.dwCaps	= DDSCAPS_PALETTE;
	ddsd->dwBackBufferCount	= 1;
	ddsd->dwHeight		= this->s.height;
	ddsd->dwWidth		= this->s.width;
	ddsd->lPitch		= this->s.lpitch;
	if (this->s.backbuffer)
		ddsd->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP;
	_getpixelformat(this->s.ddraw,&(ddsd->ddpfPixelFormat));
	
	return 0;
}

static ULONG WINAPI IDirectDrawSurface3_AddRef(LPDIRECTDRAWSURFACE3 this) {
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );

	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

#ifdef HAVE_LIBXXF86DGA
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		/* clear out of surface list */
		if (this->t.dga.fb_height == -1) {
			HeapFree(GetProcessHeap(),0,this->s.surface);
		} else {
			this->s.ddraw->e.dga.vpmask &= ~(1<<(this->t.dga.fb_height/this->s.ddraw->e.dga.fb_height));
		}
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return this->ref;
}

static ULONG WINAPI Xlib_IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		HeapFree(GetProcessHeap(),0,this->s.surface);

                if( this->s.backbuffer )
 		{
		  this->s.backbuffer->lpvtbl->fnRelease(this->s.backbuffer);
 		}

		if (this->t.xlib.image != NULL) {
		this->t.xlib.image->data = NULL;
		TSXDestroyImage(this->t.xlib.image);
		this->t.xlib.image = 0;
		}

		if (this->s.palette)
                	this->s.palette->lpvtbl->fnRelease(this->s.palette);


		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectDrawSurface3_GetAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE3 *lpdsf
) {
        TRACE(ddraw, "(%p)->GetAttachedSurface(%p,%p)\n",
		     this, lpddsd, lpdsf);

	if (TRACE_ON(ddraw)) {
		TRACE(ddraw,"	caps ");
		_dump_DDSCAPS(lpddsd->dwCaps);
	}

	if (!(lpddsd->dwCaps & DDSCAPS_BACKBUFFER)) {
		FIXME(ddraw,"whoops, can only handle backbuffers for now\n");
		return E_FAIL;
	}

	/* FIXME: should handle more than one backbuffer */
	*lpdsf = this->s.backbuffer;
 
        if( this->s.backbuffer )
        {
          this->s.backbuffer->lpvtbl->fnAddRef( this->s.backbuffer );
        }

	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_Initialize(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface3_GetPixelFormat(
	LPDIRECTDRAWSURFACE3 this,LPDDPIXELFORMAT pf
) {
	TRACE(ddraw,"(%p)->(%p)\n",this,pf);
	return _getpixelformat(this->s.ddraw,pf);
}

static HRESULT WINAPI IDirectDrawSurface3_GetBltStatus(LPDIRECTDRAWSURFACE3 this,DWORD dwFlags) {
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",this,dwFlags);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_GetOverlayPosition(
	LPDIRECTDRAWSURFACE3 this,LPLONG x1,LPLONG x2
) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,x1,x2);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_SetClipper(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWCLIPPER clipper
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,clipper);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_AddAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 surf
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,surf);
	this->s.backbuffer = surf;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_GetDC(LPDIRECTDRAWSURFACE3 this,HDC32* lphdc) {
	FIXME(ddraw,"(%p)->GetDC(%p)\n",this,lphdc);
	*lphdc = BeginPaint32(this->s.ddraw->e.xlib.window,&this->s.ddraw->e.xlib.ps);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_ReleaseDC(LPDIRECTDRAWSURFACE3 this,HDC32 hdc) {
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",this,(long)hdc);
	EndPaint32(this->s.ddraw->e.xlib.window,&this->s.ddraw->e.xlib.ps);
	return 0;
}


static HRESULT WINAPI IDirectDrawSurface3_QueryInterface(LPDIRECTDRAWSURFACE3 this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* DirectDrawSurface,DirectDrawSurface2 and DirectDrawSurface3 use
	 * the same interface. And IUnknown does that too of course.
	 */
	if (	!memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID))	||
		!memcmp(&IID_IUnknown,refiid,sizeof(IID))
	) {
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",this,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDrawSurface3_IsLost(LPDIRECTDRAWSURFACE3 this) {
	TRACE(ddraw,"(%p)->(), stub!\n",this);
	return 0; /* hmm */
}

static HRESULT WINAPI IDirectDrawSurface3_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE3 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,context,esfcb);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_Restore(LPDIRECTDRAWSURFACE3 this) {
	FIXME(ddraw,"(%p)->(),stub!\n",this);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_SetColorKey(
	LPDIRECTDRAWSURFACE3 this, DWORD dwFlags, LPDDCOLORKEY ckey
) {
	FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n",this,dwFlags,ckey);

        if( dwFlags & DDCKEY_SRCBLT )
           dwFlags &= ~DDCKEY_SRCBLT;
        if( dwFlags )
          TRACE( ddraw, "unhandled dwFlags: %08lx\n", dwFlags );
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_AddOverlayDirtyRect(
        LPDIRECTDRAWSURFACE3 this, 
        LPRECT32 lpRect )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n",this,lpRect); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_DeleteAttachedSurface(
        LPDIRECTDRAWSURFACE3 this, 
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE3 lpDDSAttachedSurface )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n",this,dwFlags,lpDDSAttachedSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_EnumOverlayZOrders(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpfnCallback )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p,%p),stub!\n", this,dwFlags,
          lpContext, lpfnCallback );

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_GetClipper(
        LPDIRECTDRAWSURFACE3 this,
        LPDIRECTDRAWCLIPPER* lplpDDClipper )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDDClipper);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_GetColorKey(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags,
        LPDDCOLORKEY lpDDColorKey )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n", this, dwFlags, lpDDColorKey);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_GetFlipStatus(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags ) 
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_GetPalette(
        LPDIRECTDRAWSURFACE3 this,
        LPDIRECTDRAWPALETTE* lplpDDPalette )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDDPalette);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_SetOverlayPosition(
        LPDIRECTDRAWSURFACE3 this,
        LONG lX,
        LONG lY)
{
  FIXME(ddraw,"(%p)->(%ld,%ld),stub!\n", this, lX, lY);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_UpdateOverlay(
        LPDIRECTDRAWSURFACE3 this,
        LPRECT32 lpSrcRect,
        LPDIRECTDRAWSURFACE3 lpDDDestSurface,
        LPRECT32 lpDestRect,
        DWORD dwFlags,
        LPDDOVERLAYFX lpDDOverlayFx )
{
  FIXME(ddraw,"(%p)->(%p,%p,%p,0x%08lx,%p),stub!\n", this,
         lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx );  

  return DD_OK;
}
 
static HRESULT WINAPI IDirectDrawSurface3_UpdateOverlayDisplay(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_UpdateOverlayZOrder(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE3 lpDDSReference )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n", this, dwFlags, lpDDSReference);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_GetDDInterface(
        LPDIRECTDRAWSURFACE3 this,
        LPVOID* lplpDD )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDD);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_PageLock(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_PageUnlock(
        LPDIRECTDRAWSURFACE3 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface3_SetSurfaceDesc(
        LPDIRECTDRAWSURFACE3 this,
        LPDDSURFACEDESC lpDDSD,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(%p,0x%08lx),stub!\n", this, lpDDSD, dwFlags);

  return DD_OK;
}

static struct IDirectDrawSurface3_VTable dga_dds3vt = {
	IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	DGA_IDirectDrawSurface3_Release,
	IDirectDrawSurface3_AddAttachedSurface,
	IDirectDrawSurface3_AddOverlayDirtyRect,
	IDirectDrawSurface3_Blt,
	IDirectDrawSurface3_BltBatch,
	IDirectDrawSurface3_BltFast,
	IDirectDrawSurface3_DeleteAttachedSurface,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	IDirectDrawSurface3_EnumOverlayZOrders,
	DGA_IDirectDrawSurface3_Flip,
	IDirectDrawSurface3_GetAttachedSurface,
	IDirectDrawSurface3_GetBltStatus,
	IDirectDrawSurface3_GetCaps,
	IDirectDrawSurface3_GetClipper,
	IDirectDrawSurface3_GetColorKey,
	IDirectDrawSurface3_GetDC,
	IDirectDrawSurface3_GetFlipStatus,
	IDirectDrawSurface3_GetOverlayPosition,
	IDirectDrawSurface3_GetPalette,
	IDirectDrawSurface3_GetPixelFormat,
	IDirectDrawSurface3_GetSurfaceDesc,
	IDirectDrawSurface3_Initialize,
	IDirectDrawSurface3_IsLost,
	IDirectDrawSurface3_Lock,
	IDirectDrawSurface3_ReleaseDC,
	IDirectDrawSurface3_Restore,
	IDirectDrawSurface3_SetClipper,
	IDirectDrawSurface3_SetColorKey,
	IDirectDrawSurface3_SetOverlayPosition,
	DGA_IDirectDrawSurface3_SetPalette,
	DGA_IDirectDrawSurface3_Unlock,
	IDirectDrawSurface3_UpdateOverlay,
	IDirectDrawSurface3_UpdateOverlayDisplay,
	IDirectDrawSurface3_UpdateOverlayZOrder,
	IDirectDrawSurface3_GetDDInterface,
	IDirectDrawSurface3_PageLock,
	IDirectDrawSurface3_PageUnlock,
	IDirectDrawSurface3_SetSurfaceDesc,
};

static struct IDirectDrawSurface3_VTable xlib_dds3vt = {
	IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	Xlib_IDirectDrawSurface3_Release,
	IDirectDrawSurface3_AddAttachedSurface,
	IDirectDrawSurface3_AddOverlayDirtyRect,
	Xlib_IDirectDrawSurface3_Blt,
	IDirectDrawSurface3_BltBatch,
	IDirectDrawSurface3_BltFast,
	IDirectDrawSurface3_DeleteAttachedSurface,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	IDirectDrawSurface3_EnumOverlayZOrders,
	Xlib_IDirectDrawSurface3_Flip,
	IDirectDrawSurface3_GetAttachedSurface,
	IDirectDrawSurface3_GetBltStatus,
	IDirectDrawSurface3_GetCaps,
	IDirectDrawSurface3_GetClipper,
	IDirectDrawSurface3_GetColorKey,
	IDirectDrawSurface3_GetDC,
	IDirectDrawSurface3_GetFlipStatus,
	IDirectDrawSurface3_GetOverlayPosition,
	IDirectDrawSurface3_GetPalette,
	IDirectDrawSurface3_GetPixelFormat,
	IDirectDrawSurface3_GetSurfaceDesc,
	IDirectDrawSurface3_Initialize,
	IDirectDrawSurface3_IsLost,
	IDirectDrawSurface3_Lock,
	IDirectDrawSurface3_ReleaseDC,
	IDirectDrawSurface3_Restore,
	IDirectDrawSurface3_SetClipper,
	IDirectDrawSurface3_SetColorKey,
	IDirectDrawSurface3_SetOverlayPosition,
	Xlib_IDirectDrawSurface3_SetPalette,
	Xlib_IDirectDrawSurface3_Unlock,
	IDirectDrawSurface3_UpdateOverlay,
	IDirectDrawSurface3_UpdateOverlayDisplay,
	IDirectDrawSurface3_UpdateOverlayZOrder,
	IDirectDrawSurface3_GetDDInterface,
	IDirectDrawSurface3_PageLock,
	IDirectDrawSurface3_PageUnlock,
	IDirectDrawSurface3_SetSurfaceDesc,
};

/******************************************************************************
 *			IDirectDrawClipper
 */
static HRESULT WINAPI IDirectDrawClipper_SetHwnd(
	LPDIRECTDRAWCLIPPER this,DWORD x,HWND32 hwnd
) {
	FIXME(ddraw,"(%p)->SetHwnd(0x%08lx,0x%08lx),stub!\n",this,x,(DWORD)hwnd);
	return 0;
}

static ULONG WINAPI IDirectDrawClipper_Release(LPDIRECTDRAWCLIPPER this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

	this->ref--;
	if (this->ref)
		return this->ref;
	HeapFree(GetProcessHeap(),0,this);
	return 0;
}

static HRESULT WINAPI IDirectDrawClipper_GetClipList(
	LPDIRECTDRAWCLIPPER this,LPRECT32 rects,LPRGNDATA lprgn,LPDWORD hmm
) {
	FIXME(ddraw,"(%p,%p,%p,%p),stub!\n",this,rects,lprgn,hmm);
	if (hmm) *hmm=0;
	return 0;
}

static HRESULT WINAPI IDirectDrawClipper_SetClipList(
	LPDIRECTDRAWCLIPPER this,LPRGNDATA lprgn,DWORD hmm
) {
	FIXME(ddraw,"(%p,%p,%ld),stub!\n",this,lprgn,hmm);
	return 0;
}

static struct IDirectDrawClipper_VTable ddclipvt = {
	(void*)1,
	(void*)2,
	IDirectDrawClipper_Release,
	IDirectDrawClipper_GetClipList,
	(void*)5,
	(void*)6,
	(void*)7,
	IDirectDrawClipper_SetClipList,
	IDirectDrawClipper_SetHwnd
};

/******************************************************************************
 *			IDirectDrawPalette
 */
static HRESULT WINAPI IDirectDrawPalette_GetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
	int	i;

	if (!this->cm) /* should not happen */ {
		FIXME(ddraw,"app tried to read colormap for non-palettized mode\n");
		return DDERR_GENERIC;
	}
	for (i=0;i<count;i++) {
                palent[i].peRed   = this->palents[start+i].peRed;
                palent[i].peBlue  = this->palents[start+i].peBlue;
                palent[i].peGreen = this->palents[start+i].peGreen;
                palent[i].peFlags = this->palents[start+i].peFlags;

	}
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
	XColor		xc;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,count,palent
	);
	if (!this->cm) /* should not happen */ {
		FIXME(ddraw,"app tried to set colormap in non-palettized mode\n");
		return DDERR_GENERIC;
	}
	if (!this->ddraw->e.xlib.paintable)
		return 0;
	for (i=0;i<count;i++) {
		xc.red = palent[i].peRed<<8;
		xc.blue = palent[i].peBlue<<8;
		xc.green = palent[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = start+i;

		TSXStoreColor(display,this->cm,&xc);

		this->palents[start+i].peRed = palent[i].peRed;
		this->palents[start+i].peBlue = palent[i].peBlue;
		this->palents[start+i].peGreen = palent[i].peGreen;
                this->palents[start+i].peFlags = palent[i].peFlags;
	}
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
#ifdef HAVE_LIBXXF86DGA
	XColor		xc;
	Colormap	cm;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,count,palent
	);
	if (!this->cm) /* should not happen */ {
		FIXME(ddraw,"app tried to set colormap in non-palettized mode\n");
		return DDERR_GENERIC;
	}
	/* FIXME: free colorcells instead of freeing whole map */
	cm = this->cm;
	this->cm = TSXCopyColormapAndFree(display,this->cm);
	TSXFreeColormap(display,cm);

	for (i=0;i<count;i++) {
		xc.red = palent[i].peRed<<8;
		xc.blue = palent[i].peBlue<<8;
		xc.green = palent[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i+start;

		TSXStoreColor(display,this->cm,&xc);

		this->palents[start+i].peRed = palent[i].peRed;
		this->palents[start+i].peBlue = palent[i].peBlue;
		this->palents[start+i].peGreen = palent[i].peGreen;
                this->palents[start+i].peFlags = palent[i].peFlags;
	}
	TSXF86DGAInstallColormap(display,DefaultScreen(display),this->cm);
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static ULONG WINAPI IDirectDrawPalette_Release(LPDIRECTDRAWPALETTE this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );
	if (!--(this->ref)) {
		if (this->cm) {
			TSXFreeColormap(display,this->cm);
			this->cm = 0;
		}
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static ULONG WINAPI IDirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE this) {

        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );
	return ++(this->ref);
}

static HRESULT WINAPI IDirectDrawPalette_Initialize(
	LPDIRECTDRAWPALETTE this,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawPalette_GetCaps(
         LPDIRECTDRAWPALETTE this, LPDWORD lpdwCaps )
{
   FIXME( ddraw, "(%p)->(%p) stub.\n", this, lpdwCaps );
   return DD_OK;
} 

static HRESULT WINAPI IDirectDrawPalette_QueryInterface(
        LPDIRECTDRAWPALETTE this,REFIID refiid,LPVOID *obj ) 
{
  char    xrefiid[50];

  WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
  FIXME(ddraw,"(%p)->(%s,%p) stub.\n",this,xrefiid,obj);

  return S_OK;
}

static struct IDirectDrawPalette_VTable dga_ddpalvt = {
	IDirectDrawPalette_QueryInterface,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	IDirectDrawPalette_GetCaps,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	DGA_IDirectDrawPalette_SetEntries
};

static struct IDirectDrawPalette_VTable xlib_ddpalvt = {
	IDirectDrawPalette_QueryInterface,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	IDirectDrawPalette_GetCaps,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	Xlib_IDirectDrawPalette_SetEntries
};

static HRESULT WINAPI IDirect3D_QueryInterface(
        LPDIRECT3D this,REFIID refiid,LPVOID *obj
) {
	/* FIXME: Not sure if this is correct */
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
        if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
                *obj = this;
                this->lpvtbl->fnAddRef(this);
                return 0;
        }
        if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
                LPDIRECT3D      d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = (LPDIRECTDRAW)this;
                this->lpvtbl->fnAddRef(this);
                d3d->lpvtbl = &d3dvt;
                *obj = d3d;
                return 0;
        }
        if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
                LPDIRECT3D2     d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = (LPDIRECTDRAW)this;
                this->lpvtbl->fnAddRef(this);
                d3d->lpvtbl = &d3d2vt;
                *obj = d3d;
                return 0;
        }
        FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirect3D_AddRef(LPDIRECT3D this) {
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );

        return ++(this->ref);
}

static ULONG WINAPI IDirect3D_Release(LPDIRECT3D this)
{
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

        if (!--(this->ref)) {
                this->ddraw->lpvtbl->fnRelease(this->ddraw);
                HeapFree(GetProcessHeap(),0,this);
                return 0;
        }
        return this->ref;
}

static HRESULT WINAPI IDirect3D_Initialize(
         LPDIRECT3D this, REFIID refiid )
{
  /* FIXME: Not sure if this is correct */
  char    xrefiid[50];

  WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
  FIXME(ddraw,"(%p)->(%s):stub.\n",this,xrefiid);
  
  return DDERR_ALREADYINITIALIZED;
}

/*******************************************************************************
 *				IDirect3D
 */
static struct IDirect3D_VTable d3dvt = {
	(void*)IDirect3D_QueryInterface,
	(void*)IDirect3D_AddRef,
	(void*)IDirect3D_Release,
	IDirect3D_Initialize,
	(void*)5,
	(void*)6,
	(void*)7,
	(void*)8,
	(void*)9,
};

/*******************************************************************************
 *				IDirect3D2
 */
static ULONG WINAPI IDirect3D2_Release(LPDIRECT3D2 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

	if (!--(this->ref)) {
		this->ddraw->lpvtbl->fnRelease(this->ddraw);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirect3D2_EnumDevices(
	LPDIRECT3D2 this,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
	D3DDEVICEDESC	d1,d2;

	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,cb,context);
#if 0
	d1.dwSize	= sizeof(d1);
	d1.dwFlags	= 0;

	d2.dwSize	= sizeof(d2);
	d2.dwFlags	= 0;

	cb((void*)&IID_IDirect3DHALDevice,"WINE Direct3D HAL","direct3d",&d1,&d2,context);
#endif
	return 0;
}

static struct IDirect3D2_VTable d3d2vt = {
	(void*)1,
	(void*)2,
	IDirect3D2_Release,
	IDirect3D2_EnumDevices,
	(void*)5,
	(void*)6,
	(void*)7,
	(void*)8,
	(void*)9,
};

/*******************************************************************************
 *				IDirectDraw
 */

/* Used in conjunction with cbWndExtra for storage of the this ptr for the window.
 * Please adjust allocation in Xlib_DirectDrawCreate if you store more data here.
 */
static INT32 ddrawXlibThisOffset = 0;

static HRESULT WINAPI DGA_IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
#ifdef HAVE_LIBXXF86DGA
	int	i;

	TRACE(ddraw, "(%p)->(%p,%p,%p)\n",this,lpddsd,lpdsf,lpunk);
	if (TRACE_ON(ddraw)) {
		DUMP("[w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
		_dump_DDSD(lpddsd->dwFlags);
		fprintf(stderr,"caps ");
		_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
		fprintf(stderr,"]\n");
	}

	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dga_dds3vt;
	if (	(lpddsd->dwFlags & DDSD_CAPS) && 
		(lpddsd->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
	) {
		if (!(lpddsd->dwFlags & DDSD_WIDTH))
			lpddsd->dwWidth = this->e.dga.fb_width;
		if (!(lpddsd->dwFlags & DDSD_HEIGHT))
			lpddsd->dwHeight = this->e.dga.fb_height;
		(*lpdsf)->s.surface = (LPBYTE)HeapAlloc(GetProcessHeap(),0,lpddsd->dwWidth*lpddsd->dwHeight*this->d.depth/8);
		(*lpdsf)->t.dga.fb_height = -1;
		(*lpdsf)->s.lpitch = lpddsd->dwWidth*this->d.depth/8;
		TRACE(ddraw,"using system memory for a primary surface\n");
	} else {
		for (i=0;i<32;i++)
			if (!(this->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for a primary surface\n",i);
		/* if i == 32 or maximum ... return error */
		this->e.dga.vpmask|=(1<<i);
		(*lpdsf)->s.surface = this->e.dga.fb_addr+((i*this->e.dga.fb_height)*this->e.dga.fb_width*this->d.depth/8);
		(*lpdsf)->t.dga.fb_height = i*this->e.dga.fb_height;
		(*lpdsf)->s.lpitch = this->e.dga.fb_width*this->d.depth/8;
	}

	lpddsd->lPitch = (*lpdsf)->s.lpitch;

	(*lpdsf)->s.width = this->d.width;
	(*lpdsf)->s.height = this->d.height;
	(*lpdsf)->s.ddraw = this;
	(*lpdsf)->s.backbuffer = NULL;
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE3	back;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

		(*lpdsf)->s.backbuffer = back = (LPDIRECTDRAWSURFACE3)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface3));
		this->lpvtbl->fnAddRef(this);
		back->ref = 1;
		back->lpvtbl = (LPDIRECTDRAWSURFACE3_VTABLE)&dga_dds3vt;
		for (i=0;i<32;i++)
			if (!(this->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		this->e.dga.vpmask|=(1<<i);
		back->s.surface = this->e.dga.fb_addr+((i*this->e.dga.fb_height)*this->e.dga.fb_width*this->d.depth/8);
		back->t.dga.fb_height = i*this->e.dga.fb_height;

		back->s.width = this->d.width;
		back->s.height = this->d.height;
		back->s.ddraw = this;
		back->s.lpitch = this->e.dga.fb_width*this->d.depth/8;
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */
	}
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	TRACE(ddraw, "(%p)->CreateSurface(%p,%p,%p)\n",
		     this,lpddsd,lpdsf,lpunk);

	if (TRACE_ON(ddraw)) {
		fprintf(stderr,"[w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
		_dump_DDSD(lpddsd->dwFlags);
		fprintf(stderr,"caps ");
		_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
		fprintf(stderr,"]\n");
	}

	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));

	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->s.ddraw             = this;
	(*lpdsf)->ref                 = 1;
	(*lpdsf)->lpvtbl              = (LPDIRECTDRAWSURFACE_VTABLE)&xlib_dds3vt;

		if (!(lpddsd->dwFlags & DDSD_WIDTH))
			lpddsd->dwWidth	 = this->d.width;
		if (!(lpddsd->dwFlags & DDSD_HEIGHT))
			lpddsd->dwHeight = this->d.height;
  (*lpdsf)->s.surface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,lpddsd->dwWidth*lpddsd->dwHeight*this->d.depth/8);

	(*lpdsf)->s.width	= lpddsd->dwWidth;
	(*lpdsf)->s.height	= lpddsd->dwHeight;

  if ((lpddsd->dwFlags & DDSD_CAPS) && 
      (lpddsd->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)) {

    /* No XImage for a offscreen buffer */
    (*lpdsf)->t.xlib.image = NULL;
    (*lpdsf)->t.xlib.on_screen = FALSE;
    (*lpdsf)->s.lpitch = lpddsd->dwWidth * (this->d.depth / 8);
    
    TRACE(ddraw,"using system memory for a primary surface\n");
  } else {
    XImage *img;
      
    TRACE(ddraw,"using standard XImage for a primary surface\n");

    /* In this case, create an XImage */
    img =
		TSXCreateImage(	display,
				DefaultVisualOfScreen(screen),
		     this->d.depth,
				ZPixmap,
				0,
				(*lpdsf)->s.surface,
				(*lpdsf)->s.width,
				(*lpdsf)->s.height,
				32,
		     (*lpdsf)->s.width * (this->d.depth / 8)
		);
    (*lpdsf)->t.xlib.image = img;
    (*lpdsf)->t.xlib.on_screen = TRUE;
	(*lpdsf)->s.lpitch = img->bytes_per_line;
    
    /* Check for backbuffers */
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE3	back;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

		(*lpdsf)->s.backbuffer = back = (LPDIRECTDRAWSURFACE3)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface3));

		this->lpvtbl->fnAddRef(this);
		back->s.ddraw = this;

		back->ref = 1;
		back->lpvtbl = (LPDIRECTDRAWSURFACE3_VTABLE)&xlib_dds3vt;
		/* FIXME: !8 bit images */
		back->s.surface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
				  img->width*img->height*(this->d.depth / 8)
		);
      back->t.xlib.image = TSXCreateImage(display,
			DefaultVisualOfScreen(screen),
					  this->d.depth,
			ZPixmap,
			0,
			back->s.surface,
			this->d.width,
			this->d.height,
			32,
					  this->d.width*(this->d.depth / 8)
		);
      back->t.xlib.on_screen = FALSE;
		back->s.width = this->d.width;
		back->s.height = this->d.height;
		back->s.lpitch = back->t.xlib.image->bytes_per_line;
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */
	}
  }
  
	return 0;
}

static HRESULT WINAPI IDirectDraw2_DuplicateSurface(
	LPDIRECTDRAW2 this,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
	FIXME(ddraw,"(%p)->(%p,%p) simply copies\n",this,src,dst);
	*dst = src; /* FIXME */
	return 0;
}

static HRESULT WINAPI IDirectDraw2_SetCooperativeLevel(
	LPDIRECTDRAW2 this,HWND32 hwnd,DWORD cooplevel
) {
	int	i;
	const struct {
		int	mask;
		char	*name;
	} flagmap[] = {
		FE(DDSCL_FULLSCREEN)
		FE(DDSCL_ALLOWREBOOT)
		FE(DDSCL_NOWINDOWCHANGES)
		FE(DDSCL_NORMAL)
		FE(DDSCL_ALLOWMODEX)
		FE(DDSCL_EXCLUSIVE)
		FE(DDSCL_SETFOCUSWINDOW)
		FE(DDSCL_SETDEVICEWINDOW)
		FE(DDSCL_CREATEDEVICEWINDOW)
	};

	FIXME(ddraw,"(%p)->(%08lx,%08lx)\n",this,(DWORD)hwnd,cooplevel);
	if(TRACE_ON(ddraw)){
	  dbg_decl_str(ddraw, 512);
	  for (i=0;i<sizeof(flagmap)/sizeof(flagmap[0]);i++)
	    if (flagmap[i].mask & cooplevel)
	      dsprintf(ddraw, "%s ", flagmap[i].name);
	  TRACE(ddraw,"	cooperative level %s\n", dbg_str(ddraw));
	}
        this->d.mainWindow = hwnd;
	return 0;
}


static HRESULT WINAPI DGA_IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
#ifdef HAVE_LIBXXF86DGA
	int	i,*depths,depcount;

	TRACE(ddraw, "(%p)->(%ld,%ld,%ld)\n", this, width, height, depth);

	depths = TSXListDepths(display,DefaultScreen(display),&depcount);
	for (i=0;i<depcount;i++)
		if (depths[i]==depth)
			break;
	TSXFree(depths);
	if (i==depcount) {/* not found */
		ERR(ddraw,"(w=%ld,h=%ld,d=%ld), unsupported depth!\n",width,height,depth);
		return DDERR_UNSUPPORTEDMODE;
	}
	if (this->d.width < width) {
		ERR(ddraw,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld\n",width,height,depth,width,this->d.width);
		return DDERR_UNSUPPORTEDMODE;
	}
	this->d.width	= width;
	this->d.height	= height;
	/* adjust fb_height, so we don't overlap */
	if (this->e.dga.fb_height < height)
		this->e.dga.fb_height = height;
	this->d.depth	= depth;

	/* FIXME: this function OVERWRITES several signal handlers. 
	 * can we save them? and restore them later? In a way that
	 * it works for the library too?
	 */
	TSXF86DGADirectVideo(display,DefaultScreen(display),XF86DGADirectGraphics);
#ifdef DIABLO_HACK
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,this->e.dga.fb_height);
#endif

#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
	int	i,*depths,depcount;
	char	buf[200];

	TRACE(ddraw, "(%p)->SetDisplayMode(%ld,%ld,%ld)\n",
		      this, width, height, depth);

	depths = TSXListDepths(display,DefaultScreen(display),&depcount);
	for (i=0;i<depcount;i++)
		if (depths[i]==depth)
			break;
	TSXFree(depths);
	if (i==depcount) {/* not found */
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	}
	/*
	if (this->d.width < width) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld",width,height,depth,width,this->d.width);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	}
	*/
	if (this->e.xlib.window)
		DestroyWindow32(this->e.xlib.window);
	this->e.xlib.window = CreateWindowEx32A(
		0,
		"WINE_DirectDraw",
		"WINE_DirectDraw",
		WS_VISIBLE|WS_SYSMENU|WS_THICKFRAME,
		0,0,
		width,
		height,
		0,
		0,
		0,
		NULL
	);

        /* Store this with the window. We'll use it for the window procedure */
	SetWindowLong32A(this->e.xlib.window,ddrawXlibThisOffset,(LONG)this);

	this->e.xlib.paintable = 1;

	ShowWindow32(this->e.xlib.window,TRUE);
	UpdateWindow32(this->e.xlib.window);

	assert(this->e.xlib.window);

        this->e.xlib.drawable  = WIN_FindWndPtr(this->e.xlib.window)->window;  

        /* We don't have a context for this window. Host off the desktop */
        if( !this->e.xlib.drawable )
        {
           this->e.xlib.drawable = WIN_GetDesktop()->window;
        }

	this->d.width	= width;
	this->d.height	= height;

	/* adjust fb_height, so we don't overlap */
	/*
	if (this->e.dga.fb_height < height)
		this->e.dga.fb_height = height;*/
	this->d.depth	= depth;
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",this,caps1,caps2);
	caps1->dwVidMemTotal = this->e.dga.fb_memsize;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	if (caps2) {
		caps2->dwVidMemTotal = this->e.dga.fb_memsize;
		caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",this,caps1,caps2);
	/* FIXME: Xlib */
	caps1->dwVidMemTotal = 2048*1024;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	if (caps2) {
		caps2->dwVidMemTotal = 2048*1024;
		caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
	/* END FIXME: Xlib */
	return 0;
}

static HRESULT WINAPI IDirectDraw2_CreateClipper(
	LPDIRECTDRAW2 this,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
	FIXME(ddraw,"(%p)->(%08lx,%p,%p),stub!\n",
		this,x,lpddclip,lpunk
	);
	*lpddclip = (LPDIRECTDRAWCLIPPER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipper));
	(*lpddclip)->ref = 1;
	(*lpddclip)->lpvtbl = &ddclipvt;
	return 0;
}

static HRESULT WINAPI common_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	if (*lpddpal == NULL) return E_OUTOFMEMORY;
	(*lpddpal)->ref = 1;
	(*lpddpal)->ddraw = (LPDIRECTDRAW)this;
	(*lpddpal)->installed = 0;
	if (this->d.depth<=8) {
		(*lpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(screen),AllocAll);
	} else {
		/* we don't want palettes in hicolor or truecolor */
		(*lpddpal)->cm = 0;
	}
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	HRESULT res;
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",this,x,palent,lpddpal,lpunk);
	res = common_IDirectDraw2_CreatePalette(this,x,palent,lpddpal,lpunk);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &dga_ddpalvt;
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",this,x,palent,lpddpal,lpunk);
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	if (*lpddpal == NULL) return E_OUTOFMEMORY;
	(*lpddpal)->ref = 1;
	(*lpddpal)->installed = 0;

	(*lpddpal)->ddraw = (LPDIRECTDRAW)this;
        this->lpvtbl->fnAddRef(this);

	if (this->d.depth<=8) {
		(*lpddpal)->cm = TSXCreateColormap(display,this->e.xlib.drawable,DefaultVisualOfScreen(screen),AllocAll);
		/* FIXME: this is not correct, when using -managed */
		TSXInstallColormap(display,(*lpddpal)->cm);
	} 
        else
        {
		/* we don't want palettes in hicolor or truecolor */
		(*lpddpal)->cm = 0;
        }

	(*lpddpal)->lpvtbl = &xlib_ddpalvt;
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw, "(%p)->()\n",this);
	Sleep(1000);
	TSXF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif
}

static HRESULT WINAPI Xlib_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	TRACE(ddraw, "(%p)->RestoreDisplayMode()\n", this);
	Sleep(1000);
	return 0;
}

static HRESULT WINAPI IDirectDraw2_WaitForVerticalBlank(
	LPDIRECTDRAW2 this,DWORD x,HANDLE32 h
) {
	TRACE(ddraw,"(%p)->(0x%08lx,0x%08x)\n",this,x,h);
	return 0;
}

static ULONG WINAPI IDirectDraw2_AddRef(LPDIRECTDRAW2 this) {
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );

	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDraw2_Release(LPDIRECTDRAW2 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

#ifdef HAVE_LIBXXF86DGA
	if (!--(this->ref)) {
		TSXF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
		SIGNAL_InitEmulator();
#endif
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return this->ref;
}

static ULONG WINAPI Xlib_IDirectDraw2_Release(LPDIRECTDRAW2 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	/* FIXME: destroy window ... */
	return this->ref;
}

static HRESULT WINAPI DGA_IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&dga_ddvt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&dga_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI Xlib_IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&xlib_ddvt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&xlib_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw2_GetVerticalBlankStatus(
	LPDIRECTDRAW2 this,BOOL32 *status
) {
        TRACE(ddraw,"(%p)->(%p)\n",this,status);
	*status = TRUE;
	return 0;
}

static HRESULT WINAPI IDirectDraw2_EnumDisplayModes(
	LPDIRECTDRAW2 this,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
	DDSURFACEDESC	ddsfd;
	static struct {
		int w,h;
	} modes[5] = { /* some of the usual modes */
		{512,384},
		{640,400},
		{640,480},
		{800,600},
		{1024,768},
	};
	static int depths[4] = {8,16,24,32};
	int	i,j;

	TRACE(ddraw,"(%p)->(0x%08lx,%p,%p,%p)\n",this,dwFlags,lpddsfd,context,modescb);
	ddsfd.dwSize = sizeof(ddsfd);
	ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	if (dwFlags & DDEDM_REFRESHRATES) {
		ddsfd.dwFlags |= DDSD_REFRESHRATE;
		ddsfd.x.dwRefreshRate = 60;
	}

	for (i=0;i<sizeof(depths)/sizeof(depths[0]);i++) {
		ddsfd.dwBackBufferCount = 1;
		ddsfd.ddpfPixelFormat.dwFourCC	= 0;
		ddsfd.ddpfPixelFormat.dwFlags 	= DDPF_RGB;
		ddsfd.ddpfPixelFormat.x.dwRGBBitCount	= depths[i];
		if (depths[i]==8) {
		ddsfd.ddpfPixelFormat.y.dwRBitMask  	= 0;
		ddsfd.ddpfPixelFormat.z.dwGBitMask  	= 0;
		ddsfd.ddpfPixelFormat.xx.dwBBitMask 	= 0;
		ddsfd.ddpfPixelFormat.xy.dwRGBAlphaBitMask= 0;
			ddsfd.ddsCaps.dwCaps=DDSCAPS_PALETTE;
			ddsfd.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
		} else {
			ddsfd.ddpfPixelFormat.xy.dwRGBAlphaBitMask= 0;

			/* FIXME: We should query those from X itself */
			switch (depths[i]) {
			case 16:
				ddsfd.ddpfPixelFormat.y.dwRBitMask = 0x000f;
				ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x00f0;
				ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x0f00;
				break;
			case 24:
				ddsfd.ddpfPixelFormat.y.dwRBitMask = 0x000000ff;
				ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x0000ff00;
				ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x00ff0000;
				break;
			case 32:
				ddsfd.ddpfPixelFormat.y.dwRBitMask = 0x000000ff;
				ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x0000ff00;
				ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x00ff0000;
				break;
			}
		}
		ddsfd.dwWidth = screenWidth;
		ddsfd.dwHeight = screenHeight;
		TRACE(ddraw," enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
		if (!modescb(&ddsfd,context)) return 0;

		for (j=0;j<sizeof(modes)/sizeof(modes[0]);j++) {
			ddsfd.dwWidth	= modes[i].w;
			ddsfd.dwHeight	= modes[i].h;
			TRACE(ddraw," enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
			if (!modescb(&ddsfd,context)) return 0;
		}

		if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
			/* modeX is not standard VGA */

			ddsfd.dwHeight = 200;
			ddsfd.dwWidth = 320;
			TRACE(ddraw," enumerating (320x200x%d)\n",depths[i]);
			if (!modescb(&ddsfd,context)) return 0;
		}
	}
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw,"(%p)->(%p)\n",this,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = screenHeight;
	lpddsfd->dwWidth = screenWidth;
	lpddsfd->lPitch = this->e.dga.fb_width*this->d.depth/8;
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	_getpixelformat(this,&(lpddsfd->ddpfPixelFormat));
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
	TRACE(ddraw,"(%p)->GetDisplayMode(%p)\n",this,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = screenHeight;
	lpddsfd->dwWidth = screenWidth;
	/* POOLE FIXME: Xlib */
	lpddsfd->lPitch = this->e.dga.fb_width*this->d.depth/8;
	/* END FIXME: Xlib */
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	_getpixelformat(this,&(lpddsfd->ddpfPixelFormat));
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_FlipToGDISurface(LPDIRECTDRAW2 this) {
	TRACE(ddraw,"(%p)->()\n",this);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_GetMonitorFrequency(
	LPDIRECTDRAW2 this,LPDWORD freq
) {
	FIXME(ddraw,"(%p)->(%p) returns 60 Hz always\n",this,freq);
	*freq = 60*100; /* 60 Hz */
	return 0;
}

/* what can we directly decompress? */
static HRESULT WINAPI IDirectDraw2_GetFourCCCodes(
	LPDIRECTDRAW2 this,LPDWORD x,LPDWORD y
) {
	FIXME(ddraw,"(%p,%p,%p), stub\n",this,x,y);
	return 0;
}

static HRESULT WINAPI IDirectDraw2_EnumSurfaces(
	LPDIRECTDRAW2 this,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
	FIXME(ddraw,"(%p)->(0x%08lx,%p,%p,%p),stub!\n",this,x,ddsfd,context,ddsfcb);
	return 0;
}

static HRESULT WINAPI IDirectDraw2_Compact(
          LPDIRECTDRAW2 this )
{
  FIXME(ddraw,"(%p)->()\n", this );
 
  return DD_OK;
}


/* Note: Hack so we can reuse the old functions without compiler warnings */
#ifdef __GNUC__
# define XCAST(fun)	(typeof(dga_ddvt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

static struct IDirectDraw_VTable dga_ddvt = {
	XCAST(QueryInterface)DGA_IDirectDraw2_QueryInterface,
	XCAST(AddRef)IDirectDraw2_AddRef,
	XCAST(Release)DGA_IDirectDraw2_Release,
	XCAST(Compact)IDirectDraw2_Compact,
	XCAST(CreateClipper)IDirectDraw2_CreateClipper,
	XCAST(CreatePalette)DGA_IDirectDraw2_CreatePalette,
	XCAST(CreateSurface)DGA_IDirectDraw2_CreateSurface,
	XCAST(DuplicateSurface)IDirectDraw2_DuplicateSurface,
	XCAST(EnumDisplayModes)IDirectDraw2_EnumDisplayModes,
	XCAST(EnumSurfaces)IDirectDraw2_EnumSurfaces,
	XCAST(FlipToGDISurface)IDirectDraw2_FlipToGDISurface,
	XCAST(GetCaps)DGA_IDirectDraw2_GetCaps,
	XCAST(GetDisplayMode)DGA_IDirectDraw2_GetDisplayMode,
	XCAST(GetFourCCCodes)IDirectDraw2_GetFourCCCodes,
	XCAST(GetGDISurface)15,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)17,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)19,
	XCAST(RestoreDisplayMode)DGA_IDirectDraw2_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2_SetCooperativeLevel,
	DGA_IDirectDraw_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2_WaitForVerticalBlank,
};

static struct IDirectDraw_VTable xlib_ddvt = {
	XCAST(QueryInterface)Xlib_IDirectDraw2_QueryInterface,
	XCAST(AddRef)IDirectDraw2_AddRef,
	XCAST(Release)Xlib_IDirectDraw2_Release,
	XCAST(Compact)IDirectDraw2_Compact,
	XCAST(CreateClipper)IDirectDraw2_CreateClipper,
	XCAST(CreatePalette)Xlib_IDirectDraw2_CreatePalette,
	XCAST(CreateSurface)Xlib_IDirectDraw2_CreateSurface,
	XCAST(DuplicateSurface)IDirectDraw2_DuplicateSurface,
	XCAST(EnumDisplayModes)IDirectDraw2_EnumDisplayModes,
	XCAST(EnumSurfaces)IDirectDraw2_EnumSurfaces,
	XCAST(FlipToGDISurface)IDirectDraw2_FlipToGDISurface,
	XCAST(GetCaps)Xlib_IDirectDraw2_GetCaps,
	XCAST(GetDisplayMode)Xlib_IDirectDraw2_GetDisplayMode,
	XCAST(GetFourCCCodes)IDirectDraw2_GetFourCCCodes,
	XCAST(GetGDISurface)15,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)17,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)19,
	XCAST(RestoreDisplayMode)Xlib_IDirectDraw2_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2_SetCooperativeLevel,
	Xlib_IDirectDraw_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2_WaitForVerticalBlank,
};

/*****************************************************************************
 * 	IDirectDraw2
 *
 */


static HRESULT WINAPI DGA_IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return DGA_IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI Xlib_IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return Xlib_IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI DGA_IDirectDraw2_GetAvailableVidMem(
	LPDIRECTDRAW2 this,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		this,ddscaps,total,free
	);
	if (total) *total = this->e.dga.fb_memsize * 1024;
	if (free) *free = this->e.dga.fb_memsize * 1024;
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetAvailableVidMem(
	LPDIRECTDRAW2 this,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		this,ddscaps,total,free
	);
	if (total) *total = 2048 * 1024;
	if (free) *free = 2048 * 1024;
	return 0;
}

static IDirectDraw2_VTable dga_dd2vt = {
	DGA_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	DGA_IDirectDraw2_Release,
	IDirectDraw2_Compact,
	IDirectDraw2_CreateClipper,
	DGA_IDirectDraw2_CreatePalette,
	DGA_IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	IDirectDraw2_FlipToGDISurface,
	DGA_IDirectDraw2_GetCaps,
	DGA_IDirectDraw2_GetDisplayMode,
	IDirectDraw2_GetFourCCCodes,
	(void*)15,
	IDirectDraw2_GetMonitorFrequency,
	(void*)17,
	IDirectDraw2_GetVerticalBlankStatus,
	(void*)19,
	DGA_IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	DGA_IDirectDraw2_SetDisplayMode,
	IDirectDraw2_WaitForVerticalBlank,
	DGA_IDirectDraw2_GetAvailableVidMem
};

static struct IDirectDraw2_VTable xlib_dd2vt = {
	Xlib_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	Xlib_IDirectDraw2_Release,
	IDirectDraw2_Compact,
	IDirectDraw2_CreateClipper,
	Xlib_IDirectDraw2_CreatePalette,
	Xlib_IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	IDirectDraw2_FlipToGDISurface,
	Xlib_IDirectDraw2_GetCaps,
	Xlib_IDirectDraw2_GetDisplayMode,
	IDirectDraw2_GetFourCCCodes,
	(void*)15,
	IDirectDraw2_GetMonitorFrequency,
	(void*)17,
	IDirectDraw2_GetVerticalBlankStatus,
	(void*)19,
	Xlib_IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	Xlib_IDirectDraw2_SetDisplayMode,
	IDirectDraw2_WaitForVerticalBlank,
	Xlib_IDirectDraw2_GetAvailableVidMem	
};

/******************************************************************************
 * 				DirectDrawCreate
 */

HRESULT WINAPI DGA_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
#ifdef HAVE_LIBXXF86DGA
	int	memsize,banksize,width,major,minor,flags,height;
	char	*addr;

	if (getuid() != 0) {
		MSG("Must be root to use XF86DGA!\n");
		MessageBox32A(0,"Using the XF86DGA extension requires the program to be run using UID 0.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return E_UNEXPECTED;
	}
	if (!DDRAW_DGA_Available()) {
		TRACE(ddraw,"No XF86DGA detected.\n");
		return DDERR_GENERIC;
	}
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &dga_ddvt;
	(*lplpDD)->ref = 1;
	TSXF86DGAQueryVersion(display,&major,&minor);
	TRACE(ddraw,"XF86DGA is version %d.%d\n",major,minor);
	TSXF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
		MSG("direct video is NOT PRESENT.\n");
	TSXF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	TRACE(ddraw,"video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
	);
	(*lplpDD)->e.dga.fb_width = width;
	(*lplpDD)->d.width = width;
	(*lplpDD)->e.dga.fb_addr = addr;
	(*lplpDD)->e.dga.fb_memsize = memsize;
	(*lplpDD)->e.dga.fb_banksize = banksize;

	TSXF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
	(*lplpDD)->e.dga.fb_height = screenHeight;
#ifdef DIABLO_HACK
	(*lplpDD)->e.dga.vpmask = 1;
#else
	(*lplpDD)->e.dga.vpmask = 0;
#endif

	/* just assume the default depth is the DGA depth too */
	(*lplpDD)->d.depth = DefaultDepthOfScreen(screen);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return DDERR_INVALIDDIRECTDRAWGUID;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

LRESULT WINAPI Xlib_DDWndProc(HWND32 hwnd,UINT32 msg,WPARAM32 wParam,LPARAM lParam)
{
   LRESULT ret;
   LPDIRECTDRAW ddraw = NULL;
   DWORD lastError;

   /*FIXME(ddraw,"(0x%04x,%s,0x%08lx,0x%08lx),stub!\n",(int)hwnd,SPY_GetMsgName(msg),(long)wParam,(long)lParam); */

   SetLastError( ERROR_SUCCESS );
   ddraw  = (LPDIRECTDRAW)GetWindowLong32A( hwnd, ddrawXlibThisOffset );
   if( (!ddraw)  &&
       ( ( lastError = GetLastError() ) != ERROR_SUCCESS )
     ) 
   {
     ERR( ddraw, "Unable to retrieve this ptr from window. Error %08lx\n", lastError );
   }

   if( ddraw )
   {
      /* Perform any special direct draw functions */
      if (msg==WM_PAINT)
        ddraw->e.xlib.paintable = 1;

      /* Now let the application deal with the rest of this */
      if( ddraw->d.mainWindow )
      {
    
        /* Don't think that we actually need to call this but... 
           might as well be on the safe side of things... */
        ret = DefWindowProc32A( hwnd, msg, wParam, lParam );

        if( !ret )
        {
          /* We didn't handle the message - give it to the application */
	  if (ddraw && ddraw->d.mainWindow && WIN_FindWndPtr(ddraw->d.mainWindow))
          	ret = CallWindowProc32A( WIN_FindWndPtr( ddraw->d.mainWindow )->winproc,
               	                   ddraw->d.mainWindow, msg, wParam, lParam );
        }
        
      }
      else
      {
        ret = DefWindowProc32A( ddraw->d.mainWindow, msg, wParam, lParam );
      } 

    }
    else
    {
	ret = DefWindowProc32A(hwnd,msg,wParam,lParam);
    }

    return ret;
}

HRESULT WINAPI Xlib_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
	WNDCLASS32A	wc;
	int		have_xshm = 0;
        WND*            pParentWindow; 

	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &xlib_ddvt;
	(*lplpDD)->ref = 1;
	(*lplpDD)->e.xlib.drawable = 0; /* in SetDisplayMode */
	(*lplpDD)->e.xlib.use_xshm = have_xshm;
	wc.style	= CS_GLOBALCLASS;
	wc.lpfnWndProc	= Xlib_DDWndProc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= /* Defines extra mem for window. This is used for storing this */
                          sizeof( LPDIRECTDRAW ); /*  ddrawXlibThisOffset */

        /* We can be a child of the desktop since we're really important */
        pParentWindow   = WIN_GetDesktop();
	wc.hInstance	= pParentWindow ? pParentWindow->hwndSelf : 0;
        wc.hInstance    = 0; 

	wc.hIcon	= 0;
	wc.hCursor	= (HCURSOR32)IDC_ARROW32A;
	wc.hbrBackground= NULL_BRUSH;
	wc.lpszMenuName	= 0;
	wc.lpszClassName= "WINE_DirectDraw";

	(*lplpDD)->e.xlib.winclass = RegisterClass32A(&wc);

	(*lplpDD)->d.depth = DefaultDepthOfScreen(screen);
	(*lplpDD)->d.height = screenHeight;
	(*lplpDD)->d.width = screenWidth;
	return 0;
}

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	char	xclsid[50];

	if (HIWORD(lpGUID))
		WINE_StringFromCLSID(lpGUID,xclsid);
	else {
		sprintf(xclsid,"<guid-%0x08x>",(int)lpGUID);
		lpGUID = NULL;
	}

	TRACE(ddraw,"(%s,%p,%p)\n",xclsid,lplpDD,pUnkOuter);

	if (!lpGUID) {
		/* if they didn't request a particular interface, use the best
		 * supported one */
		if (DDRAW_DGA_Available())
			lpGUID = &DGA_DirectDraw_GUID;
		else
			lpGUID = &XLIB_DirectDraw_GUID;
	}

	if (!memcmp(lpGUID, &DGA_DirectDraw_GUID, sizeof(GUID)))
		return DGA_DirectDrawCreate(lplpDD, pUnkOuter);
	else if (!memcmp(lpGUID, &XLIB_DirectDraw_GUID, sizeof(GUID)))
		return Xlib_DirectDrawCreate(lplpDD, pUnkOuter);

	fprintf(stderr,"DirectDrawCreate(%s,%p,%p): did not recognize requested GUID\n",xclsid,lplpDD,pUnkOuter);
	return DDERR_INVALIDDIRECTDRAWGUID;
}
