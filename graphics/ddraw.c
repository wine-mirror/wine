/*		DirectDraw using DGA or Xlib(XSHM)
 *
 * Copyright 1997,1998 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
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

#include "config.h"

#include "ts_xlib.h"
#include "ts_xutil.h"

#ifdef HAVE_LIBXXSHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "ts_xshm.h"
#endif /* defined(HAVE_LIBXXSHM) */

#ifdef HAVE_LIBXXF86DGA
#include "ts_xf86dga.h"
#endif /* defined(HAVE_LIBXXF86DGA) */

#ifdef HAVE_LIBXXF86VM
/* X is retarted and insists on declaring INT32, INT16 etc in Xmd.h,
   this is a crude hack to get around it */
#define XMD_H
#include "wintypes.h"
#include "ts_xf86vmode.h"
#endif /* defined(HAVE_LIBXXF86VM) */

#include "x11drv.h"

#include <unistd.h>
#include <assert.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "windows.h"

#include "winerror.h"
#include "gdi.h"
#include "heap.h"
#include "ldt.h"
#include "dc.h"
#include "win.h"
#include "miscemu.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "spy.h"
#include "message.h"
#include "options.h"
#include "objbase.h"
#include "monitor.h"

/* This for all the enumeration and creation of D3D-related objects */
#include "d3d_private.h"

/* define this if you want to play Diablo using XF86DGA. (bug workaround) */
#undef DIABLO_HACK

/* Restore signal handlers overwritten by XF86DGA 
 */
#define RESTORE_SIGNALS
BOOL32 (*SIGNAL_Reinit)(void); /* didn't find any obvious place to put this */

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

static struct IDirectDrawSurface4_VTable	dga_dds4vt, xlib_dds4vt;
static struct IDirectDraw_VTable		dga_ddvt, xlib_ddvt;
static struct IDirectDraw2_VTable		dga_dd2vt, xlib_dd2vt;
static struct IDirectDraw4_VTable		dga_dd4vt, xlib_dd4vt;
static struct IDirectDrawClipper_VTable	ddclipvt;
static struct IDirectDrawPalette_VTable dga_ddpalvt, xlib_ddpalvt;
static struct IDirect3D_VTable			d3dvt;
static struct IDirect3D2_VTable			d3d2vt;

#ifdef HAVE_LIBXXF86VM
static XF86VidModeModeInfo *orig_mode = NULL;
#endif

#ifdef HAVE_LIBXXSHM
static int XShmErrorFlag = 0;
#endif

BOOL32
DDRAW_DGA_Available(void)
{
#ifdef HAVE_LIBXXF86DGA
	int evbase, evret, fd;
	
   	if (Options.noDGA)
     	  return 0;
   
	/* You don't have to be root to use DGA extensions. Simply having access to /dev/mem will do the trick */
        /* This can be achieved by adding the user to the "kmem" group on Debian 2.x systems, don't know about */
        /* others. --stephenc */
        if ((fd = open("/dev/mem", O_RDWR)) != -1)
          close(fd);

	return (fd != -1) && TSXF86DGAQueryExtension(display,&evbase,&evret);
#else /* defined(HAVE_LIBXXF86DGA) */
	return 0;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	if (DDRAW_DGA_Available()) {
	  TRACE(ddraw, "Enumerating DGA interface\n");
		ddenumproc(&DGA_DirectDraw_GUID,"WINE with XFree86 DGA","display",data);
	}
	TRACE(ddraw, "Enumerating Xlib interface\n");
	ddenumproc(&XLIB_DirectDraw_GUID,"WINE with Xlib","display",data);
	TRACE(ddraw, "Enumerating Default interface\n");
	ddenumproc(NULL,"WINE (default)","display",data);
	return DD_OK;
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
	DUMP("\n");
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

static void _dump_DDCOLORKEY(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
	        FE(DDPF_ALPHAPIXELS)
		FE(DDPF_ALPHA)
		FE(DDPF_FOURCC)
		FE(DDPF_PALETTEINDEXED4)
		FE(DDPF_PALETTEINDEXEDTO8)
		FE(DDPF_PALETTEINDEXED8)
		FE(DDPF_RGB)
		FE(DDPF_COMPRESSED)
		FE(DDPF_RGBTOYUV)
		FE(DDPF_YUV)
		FE(DDPF_ZBUFFER)
		FE(DDPF_PALETTEINDEXED1)
		FE(DDPF_PALETTEINDEXED2)
		FE(DDPF_ZPIXELS)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DUMP("%s ",flags[i].name);
	DUMP("\n");
}

static void _dump_paletteformat(DWORD dwFlags) {
  int	i;
  const struct {
    DWORD	mask;
    char	*name;
  } flags[] = {
#define FE(x) { x, #x},
    FE(DDPCAPS_4BIT)
    FE(DDPCAPS_8BITENTRIES)
    FE(DDPCAPS_8BIT)
    FE(DDPCAPS_INITIALIZE)
    FE(DDPCAPS_PRIMARYSURFACE)
    FE(DDPCAPS_PRIMARYSURFACELEFT)
    FE(DDPCAPS_ALLOW256)
    FE(DDPCAPS_VSYNC)
    FE(DDPCAPS_1BIT)
    FE(DDPCAPS_2BIT)
    FE(DDPCAPS_ALPHA)
  };
  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & dwFlags)
      DUMP("%s ",flags[i].name);
  DUMP("\n");
}

static void _dump_pixelformat(LPDDPIXELFORMAT pf) {
  DUMP("Size : %ld\n", pf->dwSize);
  if (pf->dwFlags)
  _dump_DDCOLORKEY(pf->dwFlags);
  DUMP("dwFourCC : %ld\n", pf->dwFourCC);
  DUMP("RGB bit count : %ld\n", pf->x.dwRGBBitCount);
  DUMP("Masks : R %08lx  G %08lx  B %08lx  A %08lx\n",
       pf->y.dwRBitMask, pf->z.dwGBitMask, pf->xx.dwBBitMask, pf->xy.dwRGBAlphaBitMask);
}

static int _getpixelformat(LPDIRECTDRAW2 ddraw,LPDDPIXELFORMAT pf) {
	static XVisualInfo	*vi;
	XVisualInfo		vt;
	int			nitems;

	if (!vi)
		vi = TSXGetVisualInfo(display,VisualNoMask,&vt,&nitems);

	pf->dwFourCC = 0;
	pf->dwSize = sizeof(DDPIXELFORMAT);
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
 *		IDirectDrawSurface methods
 *
 * Since DDS3 and DDS2 are supersets of DDS, we implement DDS3 and let
 * DDS and DDS2 use those functions. (Function calls did not change (except
 * using different DirectDrawSurfaceX version), just added flags and functions)
 */
static HRESULT WINAPI IDirectDrawSurface4_Lock(
    LPDIRECTDRAWSURFACE4 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
        TRACE(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd);
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	    WARN(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
			 this,lprect,lpddsd,flags,(DWORD)hnd);

	/* First, copy the Surface description */
	*lpddsd = this->s.surface_desc;
	TRACE(ddraw,"locked surface: height=%ld, width=%ld, pitch=%ld\n",
	      lpddsd->dwHeight,lpddsd->dwWidth,lpddsd->lPitch);

	/* If asked only for a part, change the surface pointer */
	if (lprect) {
		FIXME(ddraw,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		lpddsd->y.lpSurface = this->s.surface_desc.y.lpSurface +
			(lprect->top*this->s.surface_desc.lPitch) +
			(lprect->left*(this->s.surface_desc.ddpfPixelFormat.x.dwRGBBitCount / 8));
	} else {
		assert(this->s.surface_desc.y.lpSurface);
	}
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4_Unlock(
	LPDIRECTDRAWSURFACE4 this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	return DD_OK;
}

static void Xlib_copy_surface_on_screen(LPDIRECTDRAWSURFACE4 this) {
  if (this->s.ddraw->d.depth != this->s.ddraw->d.screen_depth) {
    /* Pixel convertion ! */
    if ((this->s.ddraw->d.depth == 8) && (this->s.ddraw->d.screen_depth == 16)) {
      unsigned char  *src = (unsigned char  *) this->s.surface_desc.y.lpSurface;
      unsigned short *dst = (unsigned short *) this->t.xlib.image->data;
      unsigned short *pal;
      int x, y;

      if (this->s.palette != NULL) {
	pal = (unsigned short *) this->s.palette->screen_palents;
	for (y = 0; y < this->s.surface_desc.dwHeight; y++) {
	  for (x = 0; x < this->s.surface_desc.dwWidth; x++) {
	    dst[x + y * this->s.surface_desc.lPitch] = pal[src[x + y * this->s.surface_desc.lPitch]];
	  }
	}
      } else {
	WARN(ddraw, "No palette set...\n");
	memset(dst, 0, this->s.surface_desc.lPitch * this->s.surface_desc.dwHeight * 2);
      }
    } else {
      ERR(ddraw, "Unsupported pixel convertion...\n");
    }
  }

#ifdef HAVE_LIBXXSHM
    if (this->s.ddraw->e.xlib.xshm_active)
      TSXShmPutImage(display,
		     this->s.ddraw->d.drawable,
		     DefaultGCOfScreen(X11DRV_GetXScreen()),
		     this->t.xlib.image,
		     0, 0, 0, 0,
		     this->t.xlib.image->width,
		     this->t.xlib.image->height,
		     False);
    else
#endif
	TSXPutImage(		display,
				this->s.ddraw->d.drawable,
				DefaultGCOfScreen(X11DRV_GetXScreen()),
				this->t.xlib.image,
				0, 0, 0, 0,
				this->t.xlib.image->width,
		  this->t.xlib.image->height);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface4_Unlock(
	LPDIRECTDRAWSURFACE4 this,LPVOID surface)
{
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
  
	if (!this->s.ddraw->d.paintable)
		return DD_OK;

  /* Only redraw the screen when unlocking the buffer that is on screen */
	if ((this->t.xlib.image != NULL) &&
	    (this->s.surface_desc.ddsCaps.dwCaps & DDSCAPS_VISIBLE)) {
	  Xlib_copy_surface_on_screen(this);
	  
	if (this->s.palette && this->s.palette->cm)
		TSXSetWindowColormap(display,this->s.ddraw->d.drawable,this->s.palette->cm);
	}

	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4_Flip(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
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

		ptmp = this->s.surface_desc.y.lpSurface;
		this->s.surface_desc.y.lpSurface = flipto->s.surface_desc.y.lpSurface;
		flipto->s.surface_desc.y.lpSurface = ptmp;
	}
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDrawSurface4_Flip(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!this->s.ddraw->d.paintable)
		return DD_OK;

	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
  
	Xlib_copy_surface_on_screen(this);
	
	if (flipto->s.palette && flipto->s.palette->cm) {
	  TSXSetWindowColormap(display,this->s.ddraw->d.drawable,flipto->s.palette->cm);
	}
	if (flipto!=this) {
		XImage *tmp;
		LPVOID	*surf;
		tmp = this->t.xlib.image;
		this->t.xlib.image = flipto->t.xlib.image;
		flipto->t.xlib.image = tmp;
		surf = this->s.surface_desc.y.lpSurface;
		this->s.surface_desc.y.lpSurface = flipto->s.surface_desc.y.lpSurface;
		flipto->s.surface_desc.y.lpSurface = surf;
	}
	return DD_OK;
}


/* The IDirectDrawSurface4::SetPalette method attaches the specified
 * DirectDrawPalette object to a surface. The surface uses this palette for all
 * subsequent operations. The palette change takes place immediately.
 */
static HRESULT WINAPI Xlib_IDirectDrawSurface4_SetPalette(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWPALETTE pal
) {
	int i;
	TRACE(ddraw,"(%p)->(%p)\n",this,pal);

	if (pal == NULL) {
          if( this->s.palette != NULL )
            this->s.palette->lpvtbl->fnRelease( this->s.palette );
	  this->s.palette = pal;

	  return DD_OK;
	}
	
	if( !(pal->cm) && (this->s.ddraw->d.screen_depth<=8)) 
	{
		pal->cm = TSXCreateColormap(display,this->s.ddraw->d.drawable,
					    DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);

    	    	if (!Options.managed)
			TSXInstallColormap(display,pal->cm);

		for (i=0;i<256;i++) {
			XColor xc;

			xc.red = pal->palents[i].peRed<<8;
			xc.blue = pal->palents[i].peBlue<<8;
			xc.green = pal->palents[i].peGreen<<8;
			xc.flags = DoRed|DoBlue|DoGreen;
			xc.pixel = i;
			TSXStoreColor(display,pal->cm,&xc);
		}
		TSXInstallColormap(display,pal->cm);
        }

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
          TSXSetWindowColormap(display,this->s.ddraw->d.drawable,this->s.palette->cm);
        }
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4_SetPalette(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWPALETTE pal
) {
	TRACE(ddraw,"(%p)->(%p)\n",this,pal);
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
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */


}

static HRESULT WINAPI IDirectDrawSurface4_Blt(
	LPDIRECTDRAWSURFACE4 this,LPRECT32 rdst,LPDIRECTDRAWSURFACE4 src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	RECT32	xdst,xsrc;
	DDSURFACEDESC	ddesc,sdesc;
	int	i,j;

	  TRACE(ddraw,"(%p)->(%p,%p,%p,%08lx,%p)\n",
		this,rdst,src,rsrc,dwFlags,lpbltfx);

	if (src != NULL)
	  src ->lpvtbl->fnLock(src, NULL,&sdesc,0,0);
	this->lpvtbl->fnLock(this,NULL,&ddesc,0,0);
	
	if (TRACE_ON(ddraw)) {
	  if (rdst) TRACE(ddraw,"	destrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	  if (rsrc) TRACE(ddraw,"	srcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	  TRACE(ddraw,"\tflags: ");_dump_DDBLT(dwFlags);fprintf(stderr,"\n");
	  if (dwFlags & DDBLT_DDFX) {
	    TRACE(ddraw,"	blitfx: \n");_dump_DDBLTFX(lpbltfx->dwDDFX);
	  }
	}
		
	if (rdst) {
		memcpy(&xdst,rdst,sizeof(xdst));
	} else {
		xdst.top	= 0;
		xdst.bottom	= ddesc.dwHeight;
		xdst.left	= 0;
		xdst.right	= ddesc.dwWidth;
	}

	if (rsrc) {
		memcpy(&xsrc,rsrc,sizeof(xsrc));
	} else {
		if (src) {
		xsrc.top	= 0;
		xsrc.bottom	= sdesc.dwHeight;
		xsrc.left	= 0;
		xsrc.right	= sdesc.dwWidth;
		} else {
		    memset(&xsrc,0,sizeof(xsrc));
		}
	}

	dwFlags &= ~(DDBLT_WAIT|DDBLT_ASYNC);/* FIXME: can't handle right now */
	
	/* First, all the 'source-less' blits */
	if (dwFlags & DDBLT_COLORFILL) {
		int	bpp = ddesc.ddpfPixelFormat.x.dwRGBBitCount / 8;
		LPBYTE	xline,xpixel;

		xline = (LPBYTE) ddesc.y.lpSurface + xdst.top * ddesc.lPitch;
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
			xline += ddesc.lPitch;
		}
		dwFlags &= ~(DDBLT_COLORFILL);
	}

	if (dwFlags & DDBLT_DEPTHFILL) {
#ifdef HAVE_MESAGL
	  GLboolean ztest;
	  
	  /* Clears the screen */
	  TRACE(ddraw, "	Filling depth buffer with %ld\n", lpbltfx->b.dwFillDepth);
	  glClearDepth(lpbltfx->b.dwFillDepth / 65535.0); /* We suppose a 16 bit Z Buffer */
	  glGetBooleanv(GL_DEPTH_TEST, &ztest);
	  glDepthMask(GL_TRUE); /* Enables Z writing to be sure to delete also the Z buffer */
	  glClear(GL_DEPTH_BUFFER_BIT);
	  glDepthMask(ztest);
	  
	  dwFlags &= ~(DDBLT_DEPTHFILL);
#endif HAVE_MESAGL
	}
	
	if (!src) {
	    if (dwFlags) {
	      TRACE(ddraw,"\t(src=NULL):Unsupported flags: ");_dump_DDBLT(dwFlags);fprintf(stderr,"\n");
	    }
	    this->lpvtbl->fnUnlock(this,ddesc.y.lpSurface);
	    return DD_OK;
	}

	/* Now the 'with source' blits */

	/* Standard 'full-surface' blit without special effects */
	if (	(xsrc.top ==0) && (xsrc.bottom ==ddesc.dwHeight) &&
		(xsrc.left==0) && (xsrc.right  ==ddesc.dwWidth) &&
		(xdst.top ==0) && (xdst.bottom ==ddesc.dwHeight) &&
		(xdst.left==0) && (xdst.right  ==ddesc.dwWidth)  &&
		!dwFlags
	) {
		memcpy(ddesc.y.lpSurface,
		       sdesc.y.lpSurface,
		       ddesc.dwHeight * ddesc.lPitch);
	} else {
	  int bpp = ddesc.ddpfPixelFormat.x.dwRGBBitCount / 8;
	  int srcheight = xsrc.bottom - xsrc.top;
	  int srcwidth = xsrc.right - xsrc.left;
	  int dstheight = xdst.bottom - xdst.top;
	  int dstwidth = xdst.right - xdst.left;
	  int width = (xsrc.right - xsrc.left) * bpp;
	  int h;

	  /* Sanity check for rectangle sizes */
	  if ((srcheight != dstheight) || (srcwidth  != dstwidth)) {
	    int x, y;
	    
	    /* I think we should do a Blit with 'stretching' here....
	       Tomb Raider II uses this to display the background during the menu selection
	       when the screen resolution is != than 640x480 */
	    TRACE(ddraw, "Blt with stretching\n");

	    /* This is a basic stretch implementation. It is painfully slow and quite ugly. */
	    if (bpp == 1) {
	      /* In this case, we cannot do any anti-aliasing */
	      if(dwFlags & DDBLT_KEYSRC) {
		for (y = xdst.top; y < xdst.bottom; y++) {
		  for (x = xdst.left; x < xdst.right; x++) {
		    double sx, sy;
		    unsigned char tmp;
		    unsigned char *dbuf = (unsigned char *) ddesc.y.lpSurface;
		    unsigned char *sbuf = (unsigned char *) sdesc.y.lpSurface;
		    
		    sx = (((double) (x - xdst.left) / dstwidth) * srcwidth) + xsrc.left;
		    sy = (((double) (y - xdst.top) / dstheight) * srcheight) + xsrc.top;

		    tmp = sbuf[(((int) sy) * sdesc.lPitch) + ((int) sx)];
		    
		    if ((tmp < src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(tmp > src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
		      dbuf[(y * ddesc.lPitch) + x] = tmp;
		  }
		}
	      } else {
	      for (y = xdst.top; y < xdst.bottom; y++) {
		for (x = xdst.left; x < xdst.right; x++) {
		  double sx, sy;
		  unsigned char *dbuf = (unsigned char *) ddesc.y.lpSurface;
		  unsigned char *sbuf = (unsigned char *) sdesc.y.lpSurface;

		  sx = (((double) (x - xdst.left) / dstwidth) * srcwidth) + xsrc.left;
		  sy = (((double) (y - xdst.top) / dstheight) * srcheight) + xsrc.top;
		  
		  dbuf[(y * ddesc.lPitch) + x] = sbuf[(((int) sy) * sdesc.lPitch) + ((int) sx)];
		}
	      }
	      }
	    } else {
	      FIXME(ddraw, "Not done yet for depth != 8\n");
	    }
	  } else {
	    /* Same size => fast blit */
	    if (dwFlags & DDBLT_KEYSRC) {
	      switch (bpp) {
	      case 1: {
		unsigned char tmp,*psrc,*pdst;
		int h,i;
		
	    for (h = 0; h < srcheight; h++) {
		  psrc=sdesc.y.lpSurface +
		    ((h + xsrc.top) * sdesc.lPitch) + xsrc.left;
		  pdst=ddesc.y.lpSurface +
		    ((h + xdst.top) * ddesc.lPitch) + xdst.left;
		  for(i=0;i<srcwidth;i++) {
		    tmp=*(psrc + i);
		    if ((tmp < src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(tmp > src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
		      *(pdst + i)=tmp;
		  }
		}
		dwFlags&=~(DDBLT_KEYSRC);
	      } break;
		
	      case 2: {
		unsigned short tmp,*psrc,*pdst;
		int h,i;
		
		for (h = 0; h < srcheight; h++) {
		  psrc=sdesc.y.lpSurface +
		    ((h + xsrc.top) * sdesc.lPitch) + xsrc.left;
		  pdst=ddesc.y.lpSurface +
		    ((h + xdst.top) * ddesc.lPitch) + xdst.left;
		  for(i=0;i<srcwidth;i++) {
		    tmp=*(psrc + i);
		    if ((tmp < src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) ||
			(tmp > src->s.surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
		      *(pdst + i)=tmp;
		  }
		}
		dwFlags&=~(DDBLT_KEYSRC);
	      } break;
		
	      default:
		FIXME(ddraw, "Bitblt, KEYSRC: Not done yet for depth > 16\n");
	      }
	    } else {
	      /* Non-stretching Blt without color keying */
	      for (h = 0; h < srcheight; h++) {
	    memcpy(ddesc.y.lpSurface + ((h + xdst.top) * ddesc.lPitch) + xdst.left * bpp,
		   sdesc.y.lpSurface + ((h + xsrc.top) * sdesc.lPitch) + xsrc.left * bpp,
		   width);
	  }
	}
	}
	}
	
	if (dwFlags && FIXME_ON(ddraw)) {
	  FIXME(ddraw,"\tUnsupported flags: ");_dump_DDBLT(dwFlags);
	}

	this->lpvtbl->fnUnlock(this,ddesc.y.lpSurface);
	src ->lpvtbl->fnUnlock(src,sdesc.y.lpSurface);

	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_BltFast(
	LPDIRECTDRAWSURFACE4 this,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE4 src,LPRECT32 rsrc,DWORD trans
) {
	int		i,bpp,w,h;
	DDSURFACEDESC	ddesc,sdesc;

	if (1  || TRACE_ON(ddraw)) {
	    FIXME(ddraw,"(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		    this,dstx,dsty,src,rsrc,trans
	    );
	    FIXME(ddraw,"	trans:");
	    if (FIXME_ON(ddraw))
	      _dump_DDBLTFAST(trans);
	    FIXME(ddraw,"	srcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	}
	/* We need to lock the surfaces, or we won't get refreshes when done. */
	src ->lpvtbl->fnLock(src, NULL,&sdesc,DDLOCK_READONLY, 0);
	this->lpvtbl->fnLock(this,NULL,&ddesc,DDLOCK_WRITEONLY,0);
	bpp = this->s.surface_desc.ddpfPixelFormat.x.dwRGBBitCount / 8;
	h=rsrc->bottom-rsrc->top;
	if (h>ddesc.dwHeight-dsty) h=ddesc.dwHeight-dsty;
	if (h>sdesc.dwHeight-rsrc->top) h=sdesc.dwHeight-rsrc->top;
	if (h<0) h=0;
	w=rsrc->right-rsrc->left;
	if (w>ddesc.dwWidth-dstx) w=ddesc.dwWidth-dstx;
	if (w>sdesc.dwWidth-rsrc->left) w=sdesc.dwWidth-rsrc->left;
	if (w<0) w=0;

	for (i=0;i<h;i++) {
		memcpy(	ddesc.y.lpSurface+(dsty     +i)*ddesc.lPitch+dstx*bpp,
			sdesc.y.lpSurface+(rsrc->top+i)*sdesc.lPitch+rsrc->left*bpp,
			w*bpp
		);
	}
	this->lpvtbl->fnUnlock(this,ddesc.y.lpSurface);
	src ->lpvtbl->fnUnlock(src,sdesc.y.lpSurface);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_BltBatch(
	LPDIRECTDRAWSURFACE4 this,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
	FIXME(ddraw,"(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		this,ddbltbatch,x,y
	);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetCaps(
	LPDIRECTDRAWSURFACE4 this,LPDDSCAPS caps
) {
	TRACE(ddraw,"(%p)->GetCaps(%p)\n",this,caps);
	caps->dwCaps = DDSCAPS_PALETTE; /* probably more */
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE4 this,LPDDSURFACEDESC ddsd
) { 
		TRACE(ddraw, "(%p)->GetSurfaceDesc(%p)\n",
			     this,ddsd);
  
  /* Simply copy the surface description stored in the object */
  *ddsd = this->s.surface_desc;
  
  if (TRACE_ON(ddraw)) {
		fprintf(stderr,"	flags: ");
		_dump_DDSD(ddsd->dwFlags);
    if (ddsd->dwFlags & DDSD_CAPS) {
      fprintf(stderr, "  caps:  ");
      _dump_DDSCAPS(ddsd->ddsCaps.dwCaps);
    }
		fprintf(stderr,"\n");
	}

	return DD_OK;
}

static ULONG WINAPI IDirectDrawSurface4_AddRef(LPDIRECTDRAWSURFACE4 this) {
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );

	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface4_Release(LPDIRECTDRAWSURFACE4 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

#ifdef HAVE_LIBXXF86DGA
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		/* clear out of surface list */
		if (this->t.dga.fb_height == -1) {
			HeapFree(GetProcessHeap(),0,this->s.surface_desc.y.lpSurface);
		} else {
			this->s.ddraw->e.dga.vpmask &= ~(1<<(this->t.dga.fb_height/this->s.ddraw->e.dga.fb_height));
		}

		/* Free the backbuffer */
		if (this->s.backbuffer)
			this->s.backbuffer->lpvtbl->fnRelease(this->s.backbuffer);

		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return this->ref;
}

static ULONG WINAPI Xlib_IDirectDrawSurface4_Release(LPDIRECTDRAWSURFACE4 this) {
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", this, this->ref );

	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);

		if( this->s.backbuffer )
		  this->s.backbuffer->lpvtbl->fnRelease(this->s.backbuffer);

    if (this->t.xlib.image != NULL) {
      if (this->s.ddraw->d.depth != this->s.ddraw->d.screen_depth) {
	/* In pixel conversion mode, there are two buffers to release... */
	HeapFree(GetProcessHeap(),0,this->s.surface_desc.y.lpSurface);
	
#ifdef HAVE_LIBXXSHM
	if (this->s.ddraw->e.xlib.xshm_active) {
	  TSXShmDetach(display, &(this->t.xlib.shminfo));
	  TSXDestroyImage(this->t.xlib.image);
	  shmdt(this->t.xlib.shminfo.shmaddr);
	} else {
#endif
	  HeapFree(GetProcessHeap(),0,this->t.xlib.image->data);
	  this->t.xlib.image->data = NULL;
	  TSXDestroyImage(this->t.xlib.image);
#ifdef HAVE_LIBXXSHM
	}
#endif
	
      } else {
		this->t.xlib.image->data = NULL;
      
#ifdef HAVE_LIBXXSHM
      if (this->s.ddraw->e.xlib.xshm_active) {
	TSXShmDetach(display, &(this->t.xlib.shminfo));
		TSXDestroyImage(this->t.xlib.image);
	shmdt(this->t.xlib.shminfo.shmaddr);
      } else {
#endif
	      HeapFree(GetProcessHeap(),0,this->s.surface_desc.y.lpSurface);
	TSXDestroyImage(this->t.xlib.image);
#ifdef HAVE_LIBXXSHM	
      }
#endif
      }
      
		this->t.xlib.image = 0;
    } else {
	    HeapFree(GetProcessHeap(),0,this->s.surface_desc.y.lpSurface);
    }

		if (this->s.palette)
                	this->s.palette->lpvtbl->fnRelease(this->s.palette);

		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
  
	return this->ref;
}

static HRESULT WINAPI IDirectDrawSurface4_GetAttachedSurface(
	LPDIRECTDRAWSURFACE4 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE4 *lpdsf
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
          this->s.backbuffer->lpvtbl->fnAddRef( this->s.backbuffer );

	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_Initialize(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
	TRACE(ddraw,"(%p)->(%p, %p)\n",this,ddraw,lpdsfd);

	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface4_GetPixelFormat(
	LPDIRECTDRAWSURFACE4 this,LPDDPIXELFORMAT pf
) {
	TRACE(ddraw,"(%p)->(%p)\n",this,pf);

	*pf = this->s.surface_desc.ddpfPixelFormat;
	
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetBltStatus(LPDIRECTDRAWSURFACE4 this,DWORD dwFlags) {
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",this,dwFlags);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetOverlayPosition(
	LPDIRECTDRAWSURFACE4 this,LPLONG x1,LPLONG x2
) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,x1,x2);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_SetClipper(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWCLIPPER clipper
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,clipper);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_AddAttachedSurface(
	LPDIRECTDRAWSURFACE4 this,LPDIRECTDRAWSURFACE4 surf
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,surf);

	this->lpvtbl->fnAddRef(this);
	
	/* This hack will be enough for the moment */
	if (this->s.backbuffer == NULL)
	this->s.backbuffer = surf;
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetDC(LPDIRECTDRAWSURFACE4 this,HDC32* lphdc) {
	FIXME(ddraw,"(%p)->GetDC(%p)\n",this,lphdc);
	*lphdc = BeginPaint32(this->s.ddraw->d.window,&this->s.ddraw->d.ps);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_ReleaseDC(LPDIRECTDRAWSURFACE4 this,HDC32 hdc) {
  	DDSURFACEDESC	desc;
	DWORD x, y;
	
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",this,(long)hdc);
	EndPaint32(this->s.ddraw->d.window,&this->s.ddraw->d.ps);

	/* Well, as what the application did paint in this DC is NOT saved in the surface,
	   I fill it with 'dummy' values to have something on the screen */
	this->lpvtbl->fnLock(this,NULL,&desc,0,0);
	for (y = 0; y < desc.dwHeight; y++) {
	  for (x = 0; x < desc.dwWidth; x++) {
	    ((unsigned char *) desc.y.lpSurface)[x + y * desc.dwWidth] = (unsigned int) this + x + y;
	  }
	}
	this->lpvtbl->fnUnlock(this,NULL);
	
	return DD_OK;
}


static HRESULT WINAPI IDirectDrawSurface4_QueryInterface(LPDIRECTDRAWSURFACE4 this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* All DirectDrawSurface versions (1, 2, 3 and 4) use
	 * the same interface. And IUnknown does that too of course.
	 */
	if (    !memcmp(&IID_IDirectDrawSurface4,refiid,sizeof(IID))	||
	        !memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID))	||
		!memcmp(&IID_IUnknown,refiid,sizeof(IID))
	) {
		*obj = this;
		this->lpvtbl->fnAddRef(this);

		TRACE(ddraw, "  Creating IDirectDrawSurface interface (%p)\n", *obj);
		
		return S_OK;
	}
	else if (!memcmp(&IID_IDirect3DTexture2,refiid,sizeof(IID)))
	  {
	    /* Texture interface */
	    *obj = d3dtexture2_create(this);
	    this->lpvtbl->fnAddRef(this);
	    
	    TRACE(ddraw, "  Creating IDirect3DTexture2 interface (%p)\n", *obj);
	    
	    return S_OK;
	  }
	else if (!memcmp(&IID_IDirect3DTexture,refiid,sizeof(IID)))
	  {
	    /* Texture interface */
	    *obj = d3dtexture_create(this);
	    this->lpvtbl->fnAddRef(this);
	    
	    TRACE(ddraw, "  Creating IDirect3DTexture interface (%p)\n", *obj);
	    
	    return S_OK;
	  }
	else if (is_OpenGL_dx3(refiid, (LPDIRECTDRAWSURFACE) this, (LPDIRECT3DDEVICE *) obj))
	  {
	    /* It is the OpenGL Direct3D Device */
	    this->lpvtbl->fnAddRef(this);

	    TRACE(ddraw, "  Creating IDirect3DDevice interface (%p)\n", *obj);
			
		return S_OK;
	}
	
	FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",this,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDrawSurface4_IsLost(LPDIRECTDRAWSURFACE4 this) {
	TRACE(ddraw,"(%p)->(), stub!\n",this);
	return DD_OK; /* hmm */
}

static HRESULT WINAPI IDirectDrawSurface4_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE4 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,context,esfcb);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_Restore(LPDIRECTDRAWSURFACE4 this) {
	FIXME(ddraw,"(%p)->(),stub!\n",this);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_SetColorKey(
	LPDIRECTDRAWSURFACE4 this, DWORD dwFlags, LPDDCOLORKEY ckey ) 
{
        TRACE(ddraw,"(%p)->(0x%08lx,%p)\n",this,dwFlags,ckey);

        if( dwFlags & DDCKEY_SRCBLT )
        {
           dwFlags &= ~DDCKEY_SRCBLT;
	   this->s.surface_desc.dwFlags |= DDSD_CKSRCBLT;
           memcpy( &(this->s.surface_desc.ddckCKSrcBlt), ckey, sizeof( *ckey ) );
        }

        if( dwFlags & DDCKEY_DESTBLT )
        {
           dwFlags &= ~DDCKEY_DESTBLT;
	   this->s.surface_desc.dwFlags |= DDSD_CKDESTBLT;
           memcpy( &(this->s.surface_desc.ddckCKDestBlt), ckey, sizeof( *ckey ) );
        }

        if( dwFlags & DDCKEY_SRCOVERLAY )
        {
           dwFlags &= ~DDCKEY_SRCOVERLAY;
	   this->s.surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
           memcpy( &(this->s.surface_desc.ddckCKSrcOverlay), ckey, sizeof( *ckey ) );	   
        }
	
        if( dwFlags & DDCKEY_DESTOVERLAY )
        {
           dwFlags &= ~DDCKEY_DESTOVERLAY;
	   this->s.surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
           memcpy( &(this->s.surface_desc.ddckCKDestOverlay), ckey, sizeof( *ckey ) );	   
        }

        if( dwFlags )
        {
          FIXME( ddraw, "unhandled dwFlags: 0x%08lx\n", dwFlags );
        }

        return DD_OK;

}

static HRESULT WINAPI IDirectDrawSurface4_AddOverlayDirtyRect(
        LPDIRECTDRAWSURFACE4 this, 
        LPRECT32 lpRect )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n",this,lpRect); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_DeleteAttachedSurface(
        LPDIRECTDRAWSURFACE4 this, 
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n",this,dwFlags,lpDDSAttachedSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_EnumOverlayZOrders(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpfnCallback )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p,%p),stub!\n", this,dwFlags,
          lpContext, lpfnCallback );

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetClipper(
        LPDIRECTDRAWSURFACE4 this,
        LPDIRECTDRAWCLIPPER* lplpDDClipper )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDDClipper);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetColorKey(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags,
        LPDDCOLORKEY lpDDColorKey )
{
  TRACE(ddraw,"(%p)->(0x%08lx,%p)\n", this, dwFlags, lpDDColorKey);

  if( dwFlags & DDCKEY_SRCBLT )  {
     dwFlags &= ~DDCKEY_SRCBLT;
     memcpy( lpDDColorKey, &(this->s.surface_desc.ddckCKSrcBlt), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_DESTBLT )
  {
    dwFlags &= ~DDCKEY_DESTBLT;
    memcpy( lpDDColorKey, &(this->s.surface_desc.ddckCKDestBlt), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_SRCOVERLAY )
  {
    dwFlags &= ~DDCKEY_SRCOVERLAY;
    memcpy( lpDDColorKey, &(this->s.surface_desc.ddckCKSrcOverlay), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_DESTOVERLAY )
  {
    dwFlags &= ~DDCKEY_DESTOVERLAY;
    memcpy( lpDDColorKey, &(this->s.surface_desc.ddckCKDestOverlay), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags )
  {
    FIXME( ddraw, "unhandled dwFlags: 0x%08lx\n", dwFlags );
  }

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetFlipStatus(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags ) 
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetPalette(
        LPDIRECTDRAWSURFACE4 this,
        LPDIRECTDRAWPALETTE* lplpDDPalette )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDDPalette);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_SetOverlayPosition(
        LPDIRECTDRAWSURFACE4 this,
        LONG lX,
        LONG lY)
{
  FIXME(ddraw,"(%p)->(%ld,%ld),stub!\n", this, lX, lY);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_UpdateOverlay(
        LPDIRECTDRAWSURFACE4 this,
        LPRECT32 lpSrcRect,
        LPDIRECTDRAWSURFACE4 lpDDDestSurface,
        LPRECT32 lpDestRect,
        DWORD dwFlags,
        LPDDOVERLAYFX lpDDOverlayFx )
{
  FIXME(ddraw,"(%p)->(%p,%p,%p,0x%08lx,%p),stub!\n", this,
         lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx );  

  return DD_OK;
}
 
static HRESULT WINAPI IDirectDrawSurface4_UpdateOverlayDisplay(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_UpdateOverlayZOrder(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSReference )
{
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n", this, dwFlags, lpDDSReference);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetDDInterface(
        LPDIRECTDRAWSURFACE4 this,
        LPVOID* lplpDD )
{
  FIXME(ddraw,"(%p)->(%p),stub!\n", this, lplpDD);

  /* Not sure about that... */
  *lplpDD = (void *) this->s.ddraw;
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_PageLock(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_PageUnlock(
        LPDIRECTDRAWSURFACE4 this,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", this, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_SetSurfaceDesc(
        LPDIRECTDRAWSURFACE4 this,
        LPDDSURFACEDESC lpDDSD,
        DWORD dwFlags )
{
  FIXME(ddraw,"(%p)->(%p,0x%08lx),stub!\n", this, lpDDSD, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_SetPrivateData(LPDIRECTDRAWSURFACE4 this,
							 REFGUID guidTag,
							 LPVOID lpData,
							 DWORD cbSize,
							 DWORD dwFlags) {
  FIXME(ddraw, "(%p)->(%p,%p,%ld,%08lx\n", this, guidTag, lpData, cbSize, dwFlags);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetPrivateData(LPDIRECTDRAWSURFACE4 this,
							 REFGUID guidTag,
							 LPVOID lpBuffer,
							 LPDWORD lpcbBufferSize) {
  FIXME(ddraw, "(%p)->(%p,%p,%p)\n", this, guidTag, lpBuffer, lpcbBufferSize);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_FreePrivateData(LPDIRECTDRAWSURFACE4 this,
							  REFGUID guidTag)  {
  FIXME(ddraw, "(%p)->(%p)\n", this, guidTag);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_GetUniquenessValue(LPDIRECTDRAWSURFACE4 this,
							     LPDWORD lpValue)  {
  FIXME(ddraw, "(%p)->(%p)\n", this, lpValue);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4_ChangeUniquenessValue(LPDIRECTDRAWSURFACE4 this) {
  FIXME(ddraw, "(%p)\n", this);
  
  return DD_OK;
}

static struct IDirectDrawSurface4_VTable dga_dds4vt = {
	IDirectDrawSurface4_QueryInterface,
	IDirectDrawSurface4_AddRef,
	DGA_IDirectDrawSurface4_Release,
	IDirectDrawSurface4_AddAttachedSurface,
	IDirectDrawSurface4_AddOverlayDirtyRect,
	IDirectDrawSurface4_Blt,
	IDirectDrawSurface4_BltBatch,
	IDirectDrawSurface4_BltFast,
	IDirectDrawSurface4_DeleteAttachedSurface,
	IDirectDrawSurface4_EnumAttachedSurfaces,
	IDirectDrawSurface4_EnumOverlayZOrders,
	DGA_IDirectDrawSurface4_Flip,
	IDirectDrawSurface4_GetAttachedSurface,
	IDirectDrawSurface4_GetBltStatus,
	IDirectDrawSurface4_GetCaps,
	IDirectDrawSurface4_GetClipper,
	IDirectDrawSurface4_GetColorKey,
	IDirectDrawSurface4_GetDC,
	IDirectDrawSurface4_GetFlipStatus,
	IDirectDrawSurface4_GetOverlayPosition,
	IDirectDrawSurface4_GetPalette,
	IDirectDrawSurface4_GetPixelFormat,
	IDirectDrawSurface4_GetSurfaceDesc,
	IDirectDrawSurface4_Initialize,
	IDirectDrawSurface4_IsLost,
	IDirectDrawSurface4_Lock,
	IDirectDrawSurface4_ReleaseDC,
	IDirectDrawSurface4_Restore,
	IDirectDrawSurface4_SetClipper,
	IDirectDrawSurface4_SetColorKey,
	IDirectDrawSurface4_SetOverlayPosition,
	DGA_IDirectDrawSurface4_SetPalette,
	DGA_IDirectDrawSurface4_Unlock,
	IDirectDrawSurface4_UpdateOverlay,
	IDirectDrawSurface4_UpdateOverlayDisplay,
	IDirectDrawSurface4_UpdateOverlayZOrder,
	IDirectDrawSurface4_GetDDInterface,
	IDirectDrawSurface4_PageLock,
	IDirectDrawSurface4_PageUnlock,
	IDirectDrawSurface4_SetSurfaceDesc,
	IDirectDrawSurface4_SetPrivateData,
	IDirectDrawSurface4_GetPrivateData,
	IDirectDrawSurface4_FreePrivateData,
	IDirectDrawSurface4_GetUniquenessValue,
	IDirectDrawSurface4_ChangeUniquenessValue
};

static struct IDirectDrawSurface4_VTable xlib_dds4vt = {
	IDirectDrawSurface4_QueryInterface,
	IDirectDrawSurface4_AddRef,
	Xlib_IDirectDrawSurface4_Release,
	IDirectDrawSurface4_AddAttachedSurface,
	IDirectDrawSurface4_AddOverlayDirtyRect,
	IDirectDrawSurface4_Blt,
	IDirectDrawSurface4_BltBatch,
	IDirectDrawSurface4_BltFast,
	IDirectDrawSurface4_DeleteAttachedSurface,
	IDirectDrawSurface4_EnumAttachedSurfaces,
	IDirectDrawSurface4_EnumOverlayZOrders,
	Xlib_IDirectDrawSurface4_Flip,
	IDirectDrawSurface4_GetAttachedSurface,
	IDirectDrawSurface4_GetBltStatus,
	IDirectDrawSurface4_GetCaps,
	IDirectDrawSurface4_GetClipper,
	IDirectDrawSurface4_GetColorKey,
	IDirectDrawSurface4_GetDC,
	IDirectDrawSurface4_GetFlipStatus,
	IDirectDrawSurface4_GetOverlayPosition,
	IDirectDrawSurface4_GetPalette,
	IDirectDrawSurface4_GetPixelFormat,
	IDirectDrawSurface4_GetSurfaceDesc,
	IDirectDrawSurface4_Initialize,
	IDirectDrawSurface4_IsLost,
	IDirectDrawSurface4_Lock,
	IDirectDrawSurface4_ReleaseDC,
	IDirectDrawSurface4_Restore,
	IDirectDrawSurface4_SetClipper,
	IDirectDrawSurface4_SetColorKey,
	IDirectDrawSurface4_SetOverlayPosition,
	Xlib_IDirectDrawSurface4_SetPalette,
	Xlib_IDirectDrawSurface4_Unlock,
	IDirectDrawSurface4_UpdateOverlay,
	IDirectDrawSurface4_UpdateOverlayDisplay,
	IDirectDrawSurface4_UpdateOverlayZOrder,
	IDirectDrawSurface4_GetDDInterface,
	IDirectDrawSurface4_PageLock,
	IDirectDrawSurface4_PageUnlock,
	IDirectDrawSurface4_SetSurfaceDesc,
	IDirectDrawSurface4_SetPrivateData,
	IDirectDrawSurface4_GetPrivateData,
	IDirectDrawSurface4_FreePrivateData,
	IDirectDrawSurface4_GetUniquenessValue,
	IDirectDrawSurface4_ChangeUniquenessValue
};

/******************************************************************************
 *			DirectDrawCreateClipper (DDRAW.7)
 */
HRESULT WINAPI DirectDrawCreateClipper( DWORD dwFlags,
				        LPDIRECTDRAWCLIPPER *lplpDDClipper,
				        LPUNKNOWN pUnkOuter)
{
  TRACE(ddraw, "(%08lx,%p,%p)\n", dwFlags, lplpDDClipper, pUnkOuter);

  *lplpDDClipper = (LPDIRECTDRAWCLIPPER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipper));
  (*lplpDDClipper)->lpvtbl = &ddclipvt;
  (*lplpDDClipper)->ref = 1;

  return DD_OK;
}

/******************************************************************************
 *			IDirectDrawClipper
 */
static HRESULT WINAPI IDirectDrawClipper_SetHwnd(
	LPDIRECTDRAWCLIPPER this,DWORD x,HWND32 hwnd
) {
	FIXME(ddraw,"(%p)->SetHwnd(0x%08lx,0x%08lx),stub!\n",this,x,(DWORD)hwnd);
	return DD_OK;
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
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipper_SetClipList(
	LPDIRECTDRAWCLIPPER this,LPRGNDATA lprgn,DWORD hmm
) {
	FIXME(ddraw,"(%p,%p,%ld),stub!\n",this,lprgn,hmm);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipper_QueryInterface(
         LPDIRECTDRAWCLIPPER this,
         REFIID riid,
         LPVOID* ppvObj )
{
   FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,riid,ppvObj);
   return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirectDrawClipper_AddRef( LPDIRECTDRAWCLIPPER this )
{
  TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );
  return ++(this->ref);
}

static HRESULT WINAPI IDirectDrawClipper_GetHWnd(
         LPDIRECTDRAWCLIPPER this,
         HWND32* HWndPtr )
{
   FIXME(ddraw,"(%p)->(%p),stub!\n",this,HWndPtr);
   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipper_Initialize(
         LPDIRECTDRAWCLIPPER this,
         LPDIRECTDRAW lpDD,
         DWORD dwFlags )
{
   FIXME(ddraw,"(%p)->(%p,0x%08lx),stub!\n",this,lpDD,dwFlags);
   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipper_IsClipListChanged(
         LPDIRECTDRAWCLIPPER this,
         BOOL32* lpbChanged )
{
   FIXME(ddraw,"(%p)->(%p),stub!\n",this,lpbChanged);
   return DD_OK;
}

static struct IDirectDrawClipper_VTable ddclipvt = {
        IDirectDrawClipper_QueryInterface,
        IDirectDrawClipper_AddRef,
        IDirectDrawClipper_Release,
        IDirectDrawClipper_GetClipList,
        IDirectDrawClipper_GetHWnd,
        IDirectDrawClipper_Initialize,
        IDirectDrawClipper_IsClipListChanged,
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

	TRACE(ddraw,"(%p)->GetEntries(%08lx,%ld,%ld,%p)\n",
	      this,x,start,count,palent);

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
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
	XColor		xc;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,count,palent
	);
	for (i=0;i<count;i++) {
		xc.red = palent[i].peRed<<8;
		xc.blue = palent[i].peBlue<<8;
		xc.green = palent[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = start+i;

		if (this->cm)
		    TSXStoreColor(display,this->cm,&xc);

		this->palents[start+i].peRed = palent[i].peRed;
		this->palents[start+i].peBlue = palent[i].peBlue;
		this->palents[start+i].peGreen = palent[i].peGreen;
                this->palents[start+i].peFlags = palent[i].peFlags;
	}

	/* Now, if we are in 'depth conversion mode', update the screen palette */
	if (this->ddraw->d.depth != this->ddraw->d.screen_depth) {
	  int i;
	  
	  switch (this->ddraw->d.screen_depth) {
	  case 16: {
	    unsigned short *screen_palette = (unsigned short *) this->screen_palents;
	    
	    for (i = 0; i < count; i++) {
	      screen_palette[start + i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 8) |
					   ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
					   ((((unsigned short) palent[i].peGreen) & 0xFC) << 3));
	    }
	  } break;
	    
	  default:
	    ERR(ddraw, "Memory corruption !\n");
	    break;
	  }
	}
	
	if (!this->cm) /* should not happen */ {
	}
	return DD_OK;
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
	return DD_OK;
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
        TRACE(ddraw,"(%p)->(%p,%ld,%p)\n", this, ddraw, x, palent);

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

/*******************************************************************************
 *				IDirect3D
 */
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

		TRACE(ddraw, "  Creating IUnknown interface (%p)\n", *obj);
		
                return S_OK;
        }
        if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
                LPDIRECT3D      d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = (LPDIRECTDRAW)this;
                this->lpvtbl->fnAddRef(this);
                d3d->lpvtbl = &d3dvt;
                *obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);

                return S_OK;
        }
        if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D2))) {
                LPDIRECT3D2     d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = (LPDIRECTDRAW)this;
                this->lpvtbl->fnAddRef(this);
                d3d->lpvtbl = &d3d2vt;
                *obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);

                return S_OK;
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

static HRESULT WINAPI IDirect3D_EnumDevices(LPDIRECT3D this,
					    LPD3DENUMDEVICESCALLBACK cb,
					    LPVOID context) {
  FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,cb,context);

  /* Call functions defined in d3ddevices.c */
  if (d3d_OpenGL_dx3(cb, context))
    return DD_OK;

  return DD_OK;
}

static HRESULT WINAPI IDirect3D_CreateLight(LPDIRECT3D this,
					    LPDIRECT3DLIGHT *lplight,
					    IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lplight, lpunk);
  
  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create_dx3(this);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D_CreateMaterial(LPDIRECT3D this,
					       LPDIRECT3DMATERIAL *lpmaterial,
					       IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial_create(this);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D_CreateViewport(LPDIRECT3D this,
					       LPDIRECT3DVIEWPORT *lpviewport,
					       IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport_create(this);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D_FindDevice(LPDIRECT3D this,
					   LPD3DFINDDEVICESEARCH lpfinddevsrc,
					   LPD3DFINDDEVICERESULT lpfinddevrst)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpfinddevsrc, lpfinddevrst);
  
  return DD_OK;
}

static struct IDirect3D_VTable d3dvt = {
        IDirect3D_QueryInterface,
        IDirect3D_AddRef,
        IDirect3D_Release,
        IDirect3D_Initialize,
        IDirect3D_EnumDevices,
        IDirect3D_CreateLight,
        IDirect3D_CreateMaterial,
        IDirect3D_CreateViewport,
	IDirect3D_FindDevice
};

/*******************************************************************************
 *				IDirect3D2
 */
static HRESULT WINAPI IDirect3D2_QueryInterface(
        LPDIRECT3D2 this,REFIID refiid,LPVOID *obj) {
  /* For the moment, we use the same function as in IDirect3D */
  TRACE(ddraw, "Calling IDirect3D enumerating function.\n");
  
  return IDirect3D_QueryInterface((LPDIRECT3D) this, refiid, obj);
}

static ULONG WINAPI IDirect3D2_AddRef(LPDIRECT3D2 this) {
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", this, this->ref );

        return ++(this->ref);
}

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
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,cb,context);

	/* Call functions defined in d3ddevices.c */
	if (d3d_OpenGL(cb, context))
	  return DD_OK;

	return DD_OK;
}

static HRESULT WINAPI IDirect3D2_CreateLight(LPDIRECT3D2 this,
					     LPDIRECT3DLIGHT *lplight,
					     IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lplight, lpunk);

  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create(this);
  
	return DD_OK;
}

static HRESULT WINAPI IDirect3D2_CreateMaterial(LPDIRECT3D2 this,
						LPDIRECT3DMATERIAL2 *lpmaterial,
						IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial2_create(this);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2_CreateViewport(LPDIRECT3D2 this,
						LPDIRECT3DVIEWPORT2 *lpviewport,
						IUnknown *lpunk)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport2_create(this);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D2_FindDevice(LPDIRECT3D2 this,
					    LPD3DFINDDEVICESEARCH lpfinddevsrc,
					    LPD3DFINDDEVICERESULT lpfinddevrst)
{
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", this, lpfinddevsrc, lpfinddevrst);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2_CreateDevice(LPDIRECT3D2 this,
					      REFCLSID rguid,
					      LPDIRECTDRAWSURFACE surface,
					      LPDIRECT3DDEVICE2 *device)
{
  char	xbuf[50];
  
  WINE_StringFromCLSID(rguid,xbuf);
  FIXME(ddraw,"(%p)->(%s,%p,%p): stub\n",this,xbuf,surface,device);

  if (is_OpenGL(rguid, surface, device, this)) {
    this->lpvtbl->fnAddRef(this);
    return DD_OK;
}

  return DDERR_INVALIDPARAMS;
}

static struct IDirect3D2_VTable d3d2vt = {
	IDirect3D2_QueryInterface,
	IDirect3D2_AddRef,
        IDirect3D2_Release,
        IDirect3D2_EnumDevices,
	IDirect3D2_CreateLight,
	IDirect3D2_CreateMaterial,
	IDirect3D2_CreateViewport,
	IDirect3D2_FindDevice,
	IDirect3D2_CreateDevice
};

/*******************************************************************************
 *				IDirectDraw
 */

/* Used in conjunction with cbWndExtra for storage of the this ptr for the window.
 * Please adjust allocation in Xlib_DirectDrawCreate if you store more data here.
 */
static INT32 ddrawXlibThisOffset = 0;

static HRESULT common_off_screen_CreateSurface(LPDIRECTDRAW2 this,
					       LPDDSURFACEDESC lpddsd,
					       LPDIRECTDRAWSURFACE lpdsf)
{
  int bpp;
  
  /* The surface was already allocated when entering in this function */
  TRACE(ddraw,"using system memory for a surface (%p)\n", lpdsf);

  if (lpddsd->dwFlags & DDSD_ZBUFFERBITDEPTH) {
    /* This is a Z Buffer */
    TRACE(ddraw, "Creating Z-Buffer of %ld bit depth\n", lpddsd->x.dwZBufferBitDepth);
    bpp = lpddsd->x.dwZBufferBitDepth / 8;
  } else {
    /* This is a standard image */
  if (!(lpddsd->dwFlags & DDSD_PIXELFORMAT)) {
    /* No pixel format => use DirectDraw's format */
    _getpixelformat(this,&(lpddsd->ddpfPixelFormat));
    lpddsd->dwFlags |= DDSD_PIXELFORMAT;
  }  else {
    /* To check what the program wants */
    if (TRACE_ON(ddraw)) {
      _dump_pixelformat(&(lpddsd->ddpfPixelFormat));
    }
  }

  if (lpddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
    bpp = 1;
  } else {
  bpp = lpddsd->ddpfPixelFormat.x.dwRGBBitCount / 8;
  }
  }

  /* Copy the surface description */
  lpdsf->s.surface_desc = *lpddsd;
  
  lpdsf->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE;
  lpdsf->s.surface_desc.y.lpSurface = (LPBYTE)HeapAlloc(GetProcessHeap(),0,lpddsd->dwWidth * lpddsd->dwHeight * bpp);
  lpdsf->s.surface_desc.lPitch = lpddsd->dwWidth * bpp;
  
  return DD_OK;
}

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
	(*lpdsf)->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dga_dds4vt;
	(*lpdsf)->s.ddraw = this;
	(*lpdsf)->s.palette = NULL;
	(*lpdsf)->t.dga.fb_height = -1; /* This is to have non-on screen surfaces freed */

	if (!(lpddsd->dwFlags & DDSD_WIDTH))
	  lpddsd->dwWidth  = this->d.width;
	if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	  lpddsd->dwHeight = this->d.height;
	
	/* Check if this a 'primary surface' or not */
	if ((lpddsd->dwFlags & DDSD_CAPS) &&
	    (lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {

	  /* This is THE primary surface => there is DGA-specific code */
	  /* First, store the surface description */
	  (*lpdsf)->s.surface_desc = *lpddsd;
	  
	  /* Find a viewport */
		for (i=0;i<32;i++)
			if (!(this->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for a primary surface\n",i);
		/* if i == 32 or maximum ... return error */
		this->e.dga.vpmask|=(1<<i);
	  (*lpdsf)->s.surface_desc.y.lpSurface =
	    this->e.dga.fb_addr+((i*this->e.dga.fb_height)*this->e.dga.fb_width*this->d.depth/8);
		(*lpdsf)->t.dga.fb_height = i*this->e.dga.fb_height;
	  (*lpdsf)->s.surface_desc.lPitch = this->e.dga.fb_width*this->d.depth/8;
	  lpddsd->lPitch = (*lpdsf)->s.surface_desc.lPitch;

	  /* Add flags if there were not present */
	  (*lpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE;
	  (*lpdsf)->s.surface_desc.dwWidth = this->d.width;
	  (*lpdsf)->s.surface_desc.dwHeight = this->d.height;
	  TRACE(ddraw,"primary surface: dwWidth=%ld, dwHeight=%ld, lPitch=%ld\n",this->d.width,this->d.height,lpddsd->lPitch);
	  /* We put our surface always in video memory */
	  (*lpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	  _getpixelformat(this,&((*lpdsf)->s.surface_desc.ddpfPixelFormat));
	(*lpdsf)->s.backbuffer = NULL;
	  
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE4	back;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

	    (*lpdsf)->s.backbuffer = back =
	      (LPDIRECTDRAWSURFACE4)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface4));
		this->lpvtbl->fnAddRef(this);
		back->ref = 1;
		back->lpvtbl = (LPDIRECTDRAWSURFACE4_VTABLE)&dga_dds4vt;
		for (i=0;i<32;i++)
			if (!(this->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		this->e.dga.vpmask|=(1<<i);
		back->t.dga.fb_height = i*this->e.dga.fb_height;

	    /* Copy the surface description from the front buffer */
	    back->s.surface_desc = (*lpdsf)->s.surface_desc;
	    /* Change the parameters that are not the same */
	    back->s.surface_desc.y.lpSurface = this->e.dga.fb_addr+
	      ((i*this->e.dga.fb_height)*this->e.dga.fb_width*this->d.depth/8);
		back->s.ddraw = this;
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */

	    /* Add relevant info to front and back buffers */
	    (*lpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
	    back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	    back->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	  }
	} else {
	  /* There is no DGA-specific code here...
	     Go to the common surface creation function */
	  return common_off_screen_CreateSurface(this, lpddsd, *lpdsf);
	}
	
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

#ifdef HAVE_LIBXXSHM
/* Error handlers for Image creation */
static int XShmErrorHandler(Display *dpy, XErrorEvent *event) {
  XShmErrorFlag = 1;
  return 0;
}

static XImage *create_xshmimage(LPDIRECTDRAW2 this, LPDIRECTDRAWSURFACE4 lpdsf) {
  XImage *img;
  int (*WineXHandler)(Display *, XErrorEvent *);

    img = TSXShmCreateImage(display,
			    DefaultVisualOfScreen(X11DRV_GetXScreen()),
			    this->d.screen_depth,
			    ZPixmap,
			    NULL,
			    &(lpdsf->t.xlib.shminfo),
			    lpdsf->s.surface_desc.dwWidth,
			    lpdsf->s.surface_desc.dwHeight);
    
  if (img == NULL) {
    ERR(ddraw, "Error creating XShm image. Reverting to standard X images !\n");
    this->e.xlib.xshm_active = 0;
      return NULL;
  }

    lpdsf->t.xlib.shminfo.shmid = shmget( IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT|0777 );
    if (lpdsf->t.xlib.shminfo.shmid < 0) {
    ERR(ddraw, "Error creating shared memory segment. Reverting to standard X images !\n");
    this->e.xlib.xshm_active = 0;
      TSXDestroyImage(img);
      return NULL;
    }

    lpdsf->t.xlib.shminfo.shmaddr = img->data = (char*)shmat(lpdsf->t.xlib.shminfo.shmid, 0, 0);
      
    if (img->data == (char *) -1) {
    ERR(ddraw, "Error attaching shared memory segment. Reverting to standard X images !\n");
    this->e.xlib.xshm_active = 0;
      TSXDestroyImage(img);
      shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);
      return NULL;
    }
    lpdsf->t.xlib.shminfo.readOnly = False;
      
  /* This is where things start to get trickier....
     First, we flush the current X connections to be sure to catch all non-XShm related
     errors */
    TSXSync(display, False);
  /* Then we enter in the non-thread safe part of the tests */
  EnterCriticalSection( &X11DRV_CritSection );
  
  /* Reset the error flag, sets our new error handler and try to attach the surface */
  XShmErrorFlag = 0;
  WineXHandler = XSetErrorHandler(XShmErrorHandler);
  XShmAttach(display, &(lpdsf->t.xlib.shminfo));
  XSync(display, False);

  /* Check the error flag */
  if (XShmErrorFlag) {
    /* An error occured */
    XFlush(display);
    XShmErrorFlag = 0;
    XDestroyImage(img);
    shmdt(lpdsf->t.xlib.shminfo.shmaddr);
    shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);
    XSetErrorHandler(WineXHandler);

    ERR(ddraw, "Error attaching shared memory segment to X server. Reverting to standard X images !\n");
    this->e.xlib.xshm_active = 0;
    
    /* Leave the critical section */
    LeaveCriticalSection( &X11DRV_CritSection );

    return NULL;
  }

  /* Here, to be REALLY sure, I should do a XShmPutImage to check if this works,
     but it may be a bit overkill.... */
  XSetErrorHandler(WineXHandler);
  LeaveCriticalSection( &X11DRV_CritSection );
  
    shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);

    if (this->d.depth != this->d.screen_depth) {
      lpdsf->s.surface_desc.y.lpSurface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
						    lpdsf->s.surface_desc.dwWidth *
						    lpdsf->s.surface_desc.dwHeight *
						    (this->d.depth / 8));
    } else {
    lpdsf->s.surface_desc.y.lpSurface = img->data;
    }

  return img;
}
#endif /* HAVE_LIBXXSHM */

static XImage *create_ximage(LPDIRECTDRAW2 this, LPDIRECTDRAWSURFACE4 lpdsf) {
  XImage *img = NULL;
  void *img_data;

#ifdef HAVE_LIBXXSHM
  if (this->e.xlib.xshm_active) {
    img = create_xshmimage(this, lpdsf);
  }
    
  if (img == NULL) {
#endif
    /* Allocate surface memory */
    lpdsf->s.surface_desc.y.lpSurface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
						  lpdsf->s.surface_desc.dwWidth *
						  lpdsf->s.surface_desc.dwHeight *
						  (this->d.depth / 8));
    
    if (this->d.depth != this->d.screen_depth) {
      img_data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			   lpdsf->s.surface_desc.dwWidth *
			   lpdsf->s.surface_desc.dwHeight *
			   (this->d.screen_depth / 8));
    } else {
      img_data = lpdsf->s.surface_desc.y.lpSurface;
    }
      
    /* In this case, create an XImage */
    img =
      TSXCreateImage(display,
		     DefaultVisualOfScreen(X11DRV_GetXScreen()),
		     this->d.screen_depth,
		     ZPixmap,
		     0,
		     img_data,
		     lpdsf->s.surface_desc.dwWidth,
		     lpdsf->s.surface_desc.dwHeight,
		     32,
		     lpdsf->s.surface_desc.dwWidth * (this->d.screen_depth / 8)
		     );
    
#ifdef HAVE_LIBXXSHM
  }
#endif
  if (this->d.depth != this->d.screen_depth) {
    lpdsf->s.surface_desc.lPitch = (this->d.depth / 8) * lpdsf->s.surface_desc.dwWidth;
  } else {
  lpdsf->s.surface_desc.lPitch = img->bytes_per_line;
  }
  
  return img;
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
	(*lpdsf)->lpvtbl              = (LPDIRECTDRAWSURFACE_VTABLE)&xlib_dds4vt;
	(*lpdsf)->s.palette = NULL;
	(*lpdsf)->t.xlib.image = NULL; /* This is for off-screen buffers */

	if (!(lpddsd->dwFlags & DDSD_WIDTH))
		lpddsd->dwWidth	 = this->d.width;
	if (!(lpddsd->dwFlags & DDSD_HEIGHT))
		lpddsd->dwHeight = this->d.height;

	/* Check if this a 'primary surface' or not */
  if ((lpddsd->dwFlags & DDSD_CAPS) && 
	    (lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
    XImage *img;
      
    TRACE(ddraw,"using standard XImage for a primary surface (%p)\n", *lpdsf);

	  /* First, store the surface description */
	  (*lpdsf)->s.surface_desc = *lpddsd;
	  
    /* Create the XImage */
    img = create_ximage(this, (LPDIRECTDRAWSURFACE4) *lpdsf);
    if (img == NULL)
      return DDERR_OUTOFMEMORY;
    (*lpdsf)->t.xlib.image = img;

	  /* Add flags if there were not present */
	  (*lpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE;
	  (*lpdsf)->s.surface_desc.dwWidth = this->d.width;
	  (*lpdsf)->s.surface_desc.dwHeight = this->d.height;
	  (*lpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	  _getpixelformat(this,&((*lpdsf)->s.surface_desc.ddpfPixelFormat));
	  (*lpdsf)->s.backbuffer = NULL;
    
    /* Check for backbuffers */
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE4	back;
      XImage *img;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

      (*lpdsf)->s.backbuffer = back =
	(LPDIRECTDRAWSURFACE4)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface4));

      TRACE(ddraw,"allocated back-buffer (%p)\n", back);
      
		this->lpvtbl->fnAddRef(this);
		back->s.ddraw = this;

		back->ref = 1;
		back->lpvtbl = (LPDIRECTDRAWSURFACE4_VTABLE)&xlib_dds4vt;
	    /* Copy the surface description from the front buffer */
	    back->s.surface_desc = (*lpdsf)->s.surface_desc;

      /* Create the XImage */
      img = create_ximage(this, back);
      if (img == NULL)
	return DDERR_OUTOFMEMORY;
      back->t.xlib.image = img;
      
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */

	    /* Add relevant info to front and back buffers */
	    (*lpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
	    back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	    back->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	}
	} else {
	  /* There is no Xlib-specific code here...
	     Go to the common surface creation function */
	  return common_off_screen_CreateSurface(this, lpddsd, *lpdsf);
  }
  
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_DuplicateSurface(
	LPDIRECTDRAW2 this,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
	FIXME(ddraw,"(%p)->(%p,%p) simply copies\n",this,src,dst);
	*dst = src; /* FIXME */
	return DD_OK;
}

/* 
 * The Xlib Implementation tries to use the passed hwnd as drawing window,
 * even when the approbiate bitmasks are not specified.
 */
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

	/* This will be overwritten in the case of Full Screen mode.
	   Windowed games could work with that :-) */
	if (hwnd)
	this->d.drawable  = X11DRV_WND_GetXWindow(WIN_FindWndPtr(hwnd));

	return DD_OK;
}

/* Small helper to either use the cooperative window or create a new 
 * one (for mouse and keyboard input) and drawing in the Xlib implementation.
 */
static void _common_IDirectDraw_SetDisplayMode(LPDIRECTDRAW this) {
	RECT32	rect;

	/* Do not destroy the application supplied cooperative window */
	if (this->d.window && this->d.window != this->d.mainWindow) {
		DestroyWindow32(this->d.window);
		this->d.window = 0;
	}
	/* Sanity check cooperative window before assigning it to drawing. */
	if (	IsWindow32(this->d.mainWindow) &&
		IsWindowVisible32(this->d.mainWindow)
	) {
		GetWindowRect32(this->d.mainWindow,&rect);
		if (((rect.right-rect.left) >= this->d.width)	&&
		    ((rect.bottom-rect.top) >= this->d.height)
		)
			this->d.window = this->d.mainWindow;
	}
	/* ... failed, create new one. */
	if (!this->d.window) {
	    this->d.window = CreateWindowEx32A(
		    0,
		    "WINE_DirectDraw",
		    "WINE_DirectDraw",
		    WS_VISIBLE|WS_SYSMENU|WS_THICKFRAME,
		    0,0,
		    this->d.width,
		    this->d.height,
		    0,
		    0,
		    0,
		    NULL
	    );
	    /*Store THIS with the window. We'll use it in the window procedure*/
	    SetWindowLong32A(this->d.window,ddrawXlibThisOffset,(LONG)this);
	    ShowWindow32(this->d.window,TRUE);
	    UpdateWindow32(this->d.window);
	}
	SetFocus32(this->d.window);
}

static HRESULT WINAPI DGA_IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
#ifdef HAVE_LIBXXF86DGA
        int	i,*depths,depcount,mode_count;

	TRACE(ddraw, "(%p)->(%ld,%ld,%ld)\n", this, width, height, depth);

	/* We hope getting the asked for depth */
	this->d.screen_depth = depth;

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
	this->d.depth	= depth;

	/* adjust fb_height, so we don't overlap */
	if (this->e.dga.fb_height < height)
		this->e.dga.fb_height = height;
	_common_IDirectDraw_SetDisplayMode(this);

#ifdef HAVE_LIBXXF86VM
        {
            XF86VidModeModeInfo **all_modes, *vidmode = NULL;
	    XF86VidModeModeLine mod_tmp;
	    /* int dotclock_tmp; */

            /* save original video mode and set fullscreen if available*/
	    orig_mode = (XF86VidModeModeInfo *) malloc (sizeof(XF86VidModeModeInfo));  
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
            for (i=0;i<mode_count;i++)
            {
                if (all_modes[i]->hdisplay == width && all_modes[i]->vdisplay == height)
                {
                    vidmode = (XF86VidModeModeInfo *)malloc(sizeof(XF86VidModeModeInfo));
                    *vidmode = *(all_modes[i]);
                    break;
                } else
                    TSXFree(all_modes[i]->private);
            }
	    for (i++;i<mode_count;i++) TSXFree(all_modes[i]->private);
            TSXFree(all_modes);

            if (!vidmode)
                WARN(ddraw, "Fullscreen mode not available!\n");

            if (vidmode)
	      {
	        TRACE(ddraw,"SwitchToMode(%dx%d)\n",vidmode->hdisplay,vidmode->vdisplay);
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
#ifdef DIABLO_HACK
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,this->e.dga.fb_height);
#else
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
#endif

#ifdef RESTORE_SIGNALS
	if (SIGNAL_Reinit) SIGNAL_Reinit();
#endif
	return DD_OK;
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

	/* We hope getting the asked for depth */
	this->d.screen_depth = depth;

	depths = TSXListDepths(display,DefaultScreen(display),&depcount);

	for (i=0;i<depcount;i++)
		if (depths[i]==depth)
			break;
	if (i==depcount) {/* not found */
	  for (i=0;i<depcount;i++)
	    if (depths[i]==16)
	      break;

	  if (i==depcount) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
	    TSXFree(depths);
		return DDERR_UNSUPPORTEDMODE;
	  } else {
	    WARN(ddraw, "Warning : running in depth-convertion mode. Should run using a %ld depth for optimal performances.\n", depth);
	    this->d.screen_depth = 16;
	  }
	}
	TSXFree(depths);

	this->d.width	= width;
	this->d.height	= height;
	this->d.depth	= depth;

	_common_IDirectDraw_SetDisplayMode(this);

	this->d.paintable = 1;
        this->d.drawable  = ((X11DRV_WND_DATA *) WIN_FindWndPtr(this->d.window)->pDriverData)->window;  
        /* We don't have a context for this window. Host off the desktop */
        if( !this->d.drawable )
           this->d.drawable = ((X11DRV_WND_DATA *) WIN_GetDesktop()->pDriverData)->window;
	return DD_OK;
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
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static void fill_caps(LPDDCAPS caps) {
  /* This function tries to fill the capabilities of Wine's DDraw implementation.
     Need to be fixed, though.. */
  if (caps == NULL)
    return;

  caps->dwSize = sizeof(*caps);
  caps->dwCaps = DDCAPS_3D | DDCAPS_ALPHA | DDCAPS_BLT | DDCAPS_BLTSTRETCH | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL |
    DDCAPS_CANBLTSYSMEM |  DDCAPS_COLORKEY | DDCAPS_PALETTE | DDCAPS_ZBLTS;
  caps->dwCaps2 = DDCAPS2_CERTIFIED | DDCAPS2_NO2DDURING3DSCENE | DDCAPS2_NOPAGELOCKREQUIRED |
    DDCAPS2_WIDESURFACES;
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
  caps->ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
    DDSCAPS_FRONTBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_MIPMAP | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN |
      DDSCAPS_OVERLAY | DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE |
	DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE | DDSCAPS_ZBUFFER;
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",this,caps1,caps2);

	/* Put the same caps for the two capabilities */
	fill_caps(caps1);
	fill_caps(caps2);

	return DD_OK;
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
	return DD_OK;
}

static HRESULT WINAPI common_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk,int *psize
) {
	int size = 0;
	  
	if (TRACE_ON(ddraw))
	  _dump_paletteformat(dwFlags);
	
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	if (*lpddpal == NULL) return E_OUTOFMEMORY;
	(*lpddpal)->ref = 1;
	(*lpddpal)->ddraw = (LPDIRECTDRAW)this;
	(*lpddpal)->installed = 0;

	  if (dwFlags & DDPCAPS_1BIT)
	    size = 2;
	  else if (dwFlags & DDPCAPS_2BIT)
	    size = 4;
	  else if (dwFlags & DDPCAPS_4BIT)
	    size = 16;
	  else if (dwFlags & DDPCAPS_8BIT)
	    size = 256;
	  else
	    ERR(ddraw, "unhandled palette format\n");
	*psize = size;
	
        if (palent)
        {
	  /* Now, if we are in 'depth conversion mode', create the screen palette */
	  if (this->d.depth != this->d.screen_depth) {
	    int i;
	  
	    switch (this->d.screen_depth) {
	    case 16: {
	      unsigned short *screen_palette = (unsigned short *) (*lpddpal)->screen_palents;
	      
	      for (i = 0; i < size; i++) {
		screen_palette[i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 8) |
				     ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
				     ((((unsigned short) palent[i].peGreen) & 0xFC) << 3));
	      }
	    } break;

	    default:
	      ERR(ddraw, "Memory corruption ! (depth=%ld, screen_depth=%ld)\n",this->d.depth,this->d.screen_depth);
	      break;
	    }
	  }
	  
	  memcpy((*lpddpal)->palents, palent, size * sizeof(PALETTEENTRY));
        } else if (this->d.depth != this->d.screen_depth) {
	  int i;
	  
	  switch (this->d.screen_depth) {
	  case 16: {
	    unsigned short *screen_palette = (unsigned short *) (*lpddpal)->screen_palents;
	    
	    for (i = 0; i < size; i++) {
	      screen_palette[i] = 0xFFFF;
        }
	  } break;
	    
	  default:
	    ERR(ddraw, "Memory corruption !\n");
	    break;
	  }
	}
	
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	HRESULT res;
	int xsize = 0,i;

	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",this,dwFlags,palent,lpddpal,lpunk);
	res = common_IDirectDraw2_CreatePalette(this,dwFlags,palent,lpddpal,lpunk,&xsize);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &dga_ddpalvt;
	if (this->d.depth<=8) {
		(*lpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);
	} else {
		FIXME(ddraw,"why are we doing CreatePalette in hi/truecolor?\n");
		(*lpddpal)->cm = 0;
	}
	if (((*lpddpal)->cm)&&xsize) {
	  for (i=0;i<xsize;i++) {
		  XColor xc;

		  xc.red = (*lpddpal)->palents[i].peRed<<8;
		  xc.blue = (*lpddpal)->palents[i].peBlue<<8;
		  xc.green = (*lpddpal)->palents[i].peGreen<<8;
		  xc.flags = DoRed|DoBlue|DoGreen;
		  xc.pixel = i;
		  TSXStoreColor(display,(*lpddpal)->cm,&xc);
	  }
	}
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	int xsize;
	HRESULT res;

	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",this,dwFlags,palent,lpddpal,lpunk);
	res = common_IDirectDraw2_CreatePalette(this,dwFlags,palent,lpddpal,lpunk,&xsize);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &xlib_ddpalvt;
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw, "(%p)->()\n",this);
	Sleep(1000);
	TSXF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
	if (SIGNAL_Reinit) SIGNAL_Reinit();
#endif
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif
}

static HRESULT WINAPI Xlib_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	TRACE(ddraw, "(%p)->RestoreDisplayMode()\n", this);
	Sleep(1000);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_WaitForVerticalBlank(
	LPDIRECTDRAW2 this,DWORD x,HANDLE32 h
) {
	TRACE(ddraw,"(%p)->(0x%08lx,0x%08x)\n",this,x,h);
	return DD_OK;
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

#ifdef HAVE_LIBXXF86VM
		if (orig_mode) {
			TSXF86VidModeSwitchToMode(
				display,
				DefaultScreen(display),
				orig_mode);
			if (orig_mode->privsize)
				TSXFree(orig_mode->private);		
			free(orig_mode);
			orig_mode = NULL;
		}
#endif
		
#ifdef RESTORE_SIGNALS
		if (SIGNAL_Reinit) SIGNAL_Reinit();
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

		TRACE(ddraw, "  Creating IUnknown interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&dga_ddvt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&dga_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw2 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&dga_dd4vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw4 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D2))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);
		
		return S_OK;
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

		TRACE(ddraw, "  Creating IUnknown interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&xlib_ddvt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&xlib_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw2 interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4))) {
		this->lpvtbl = (LPDIRECTDRAW2_VTABLE)&xlib_dd4vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;

		TRACE(ddraw, "  Creating IDirectDraw4 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (LPDIRECTDRAW)this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);

		return S_OK;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw2_GetVerticalBlankStatus(
	LPDIRECTDRAW2 this,BOOL32 *status
) {
        TRACE(ddraw,"(%p)->(%p)\n",this,status);
	*status = TRUE;
	return DD_OK;
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
		/* FIXME: those masks would have to be set in depth > 8 */
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
		    ddsfd.ddpfPixelFormat.y.dwRBitMask = 0xF800;
		    ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x07E0;
		    ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x001F;
		    break;
		  case 24:
		    ddsfd.ddpfPixelFormat.y.dwRBitMask = 0x00FF0000;
		    ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x0000FF00;
		    ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x000000FF;
		    break;
		  case 32:
		    ddsfd.ddpfPixelFormat.y.dwRBitMask = 0x00FF0000;
		    ddsfd.ddpfPixelFormat.z.dwGBitMask = 0x0000FF00;
		    ddsfd.ddpfPixelFormat.xx.dwBBitMask= 0x000000FF;
		    break;
		  }
		}

		ddsfd.dwWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
		ddsfd.dwHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
		TRACE(ddraw," enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
		if (!modescb(&ddsfd,context)) return DD_OK;

		for (j=0;j<sizeof(modes)/sizeof(modes[0]);j++) {
			ddsfd.dwWidth	= modes[j].w;
			ddsfd.dwHeight	= modes[j].h;
			TRACE(ddraw," enumerating (%ldx%ldx%d)\n",ddsfd.dwWidth,ddsfd.dwHeight,depths[i]);
			if (!modescb(&ddsfd,context)) return DD_OK;
		}

		if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
			/* modeX is not standard VGA */

			ddsfd.dwHeight = 200;
			ddsfd.dwWidth = 320;
			TRACE(ddraw," enumerating (320x200x%d)\n",depths[i]);
			if (!modescb(&ddsfd,context)) return DD_OK;
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
	lpddsfd->dwHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	lpddsfd->dwWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
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
	lpddsfd->dwHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	lpddsfd->dwWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
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
	return DD_OK;
}

/* what can we directly decompress? */
static HRESULT WINAPI IDirectDraw2_GetFourCCCodes(
	LPDIRECTDRAW2 this,LPDWORD x,LPDWORD y
) {
	FIXME(ddraw,"(%p,%p,%p), stub\n",this,x,y);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_EnumSurfaces(
	LPDIRECTDRAW2 this,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
  FIXME(ddraw,"(%p)->(0x%08lx,%p,%p,%p),stub!\n",this,x,ddsfd,context,ddsfcb);
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_Compact(
          LPDIRECTDRAW2 this )
{
  FIXME(ddraw,"(%p)->()\n", this );
 
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_GetGDISurface(LPDIRECTDRAW2 this,
						 LPDIRECTDRAWSURFACE *lplpGDIDDSSurface) {
  FIXME(ddraw,"(%p)->(%p)\n", this, lplpGDIDDSSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_GetScanLine(LPDIRECTDRAW2 this,
					       LPDWORD lpdwScanLine) {
  FIXME(ddraw,"(%p)->(%p)\n", this, lpdwScanLine);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2_Initialize(LPDIRECTDRAW2 this,
					      GUID *lpGUID) {
  FIXME(ddraw,"(%p)->(%p)\n", this, lpGUID);
  
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
	XCAST(GetGDISurface)IDirectDraw2_GetGDISurface,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)IDirectDraw2_GetScanLine,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)IDirectDraw2_Initialize,
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
	XCAST(GetGDISurface)IDirectDraw2_GetGDISurface,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)IDirectDraw2_GetScanLine,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)IDirectDraw2_Initialize,
	XCAST(RestoreDisplayMode)Xlib_IDirectDraw2_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2_SetCooperativeLevel,
	Xlib_IDirectDraw_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2_WaitForVerticalBlank,
};

#undef XCAST

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
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetAvailableVidMem(
	LPDIRECTDRAW2 this,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		this,ddscaps,total,free
	);
	if (total) *total = 2048 * 1024;
	if (free) *free = 2048 * 1024;
	return DD_OK;
}

static IDirectDraw2_VTable dga_dd2vt = {
	DGA_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	DGA_IDirectDraw2_Release,
	IDirectDraw2_Compact,
	IDirectDraw2_CreateClipper,
	DGA_IDirectDraw2_CreatePalette,
	DGA_IDirectDraw2_CreateSurface,
	IDirectDraw2_DuplicateSurface,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	IDirectDraw2_FlipToGDISurface,
	DGA_IDirectDraw2_GetCaps,
	DGA_IDirectDraw2_GetDisplayMode,
	IDirectDraw2_GetFourCCCodes,
	IDirectDraw2_GetGDISurface,
	IDirectDraw2_GetMonitorFrequency,
	IDirectDraw2_GetScanLine,
	IDirectDraw2_GetVerticalBlankStatus,
	IDirectDraw2_Initialize,
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
	IDirectDraw2_DuplicateSurface,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	IDirectDraw2_FlipToGDISurface,
	Xlib_IDirectDraw2_GetCaps,
	Xlib_IDirectDraw2_GetDisplayMode,
	IDirectDraw2_GetFourCCCodes,
	IDirectDraw2_GetGDISurface,
	IDirectDraw2_GetMonitorFrequency,
	IDirectDraw2_GetScanLine,
	IDirectDraw2_GetVerticalBlankStatus,
	IDirectDraw2_Initialize,
	Xlib_IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	Xlib_IDirectDraw2_SetDisplayMode,
	IDirectDraw2_WaitForVerticalBlank,
	Xlib_IDirectDraw2_GetAvailableVidMem	
};

/*****************************************************************************
 * 	IDirectDraw4
 *
 */

static HRESULT WINAPI IDirectDraw4_GetSurfaceFromDC(LPDIRECTDRAW4 this,
						    HDC32 hdc,
						    LPDIRECTDRAWSURFACE *lpDDS) {
  FIXME(ddraw, "(%p)->(%08ld,%p)\n", this, (DWORD) hdc, lpDDS);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4_RestoreAllSurfaces(LPDIRECTDRAW4 this) {
  FIXME(ddraw, "(%p)->()\n", this);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4_TestCooperativeLevel(LPDIRECTDRAW4 this) {
  FIXME(ddraw, "(%p)->()\n", this);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4_GetDeviceIdentifier(LPDIRECTDRAW4 this,
						       LPDDDEVICEIDENTIFIER lpdddi,
						       DWORD dwFlags) {
  FIXME(ddraw, "(%p)->(%p,%08lx)\n", this, lpdddi, dwFlags);
  
  return DD_OK;
}

#ifdef __GNUC__
# define XCAST(fun)	(typeof(dga_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif


static struct IDirectDraw4_VTable dga_dd4vt = {
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
	XCAST(GetGDISurface)IDirectDraw2_GetGDISurface,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)IDirectDraw2_GetScanLine,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)IDirectDraw2_Initialize,
	XCAST(RestoreDisplayMode)DGA_IDirectDraw2_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2_SetCooperativeLevel,
	XCAST(SetDisplayMode)DGA_IDirectDraw_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2_WaitForVerticalBlank,
	XCAST(GetAvailableVidMem)DGA_IDirectDraw2_GetAvailableVidMem,
	IDirectDraw4_GetSurfaceFromDC,
	IDirectDraw4_RestoreAllSurfaces,
	IDirectDraw4_TestCooperativeLevel,
	IDirectDraw4_GetDeviceIdentifier
};

static struct IDirectDraw4_VTable xlib_dd4vt = {
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
	XCAST(GetGDISurface)IDirectDraw2_GetGDISurface,
	XCAST(GetMonitorFrequency)IDirectDraw2_GetMonitorFrequency,
	XCAST(GetScanLine)IDirectDraw2_GetScanLine,
	XCAST(GetVerticalBlankStatus)IDirectDraw2_GetVerticalBlankStatus,
	XCAST(Initialize)IDirectDraw2_Initialize,
	XCAST(RestoreDisplayMode)Xlib_IDirectDraw2_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2_SetCooperativeLevel,
	XCAST(SetDisplayMode)Xlib_IDirectDraw_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2_WaitForVerticalBlank,
	XCAST(GetAvailableVidMem)Xlib_IDirectDraw2_GetAvailableVidMem,
	IDirectDraw4_GetSurfaceFromDC,
	IDirectDraw4_RestoreAllSurfaces,
	IDirectDraw4_TestCooperativeLevel,
	IDirectDraw4_GetDeviceIdentifier
};

#undef XCAST

/******************************************************************************
 * 				DirectDrawCreate
 */

LRESULT WINAPI Xlib_DDWndProc(HWND32 hwnd,UINT32 msg,WPARAM32 wParam,LPARAM lParam)
{
   LRESULT ret;
   LPDIRECTDRAW ddraw = NULL;
   DWORD lastError;

   /* FIXME(ddraw,"(0x%04x,%s,0x%08lx,0x%08lx),stub!\n",(int)hwnd,SPY_GetMsgName(msg),(long)wParam,(long)lParam); */

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
        ddraw->d.paintable = 1;

      /* Now let the application deal with the rest of this */
      if( ddraw->d.mainWindow )
      {
    
        /* Don't think that we actually need to call this but... 
           might as well be on the safe side of things... */

	/* I changed hwnd to ddraw->d.mainWindow as I did not see why
	   it should be the procedures of our fake window that gets called
	   instead of those of the window provided by the application.
	   And with this patch, mouse clicks work with Monkey Island III
	     - Lionel */
	ret = DefWindowProc32A( ddraw->d.mainWindow, msg, wParam, lParam );

        if( !ret )
        {
          /* We didn't handle the message - give it to the application */
	  if (ddraw && ddraw->d.mainWindow && WIN_FindWndPtr(ddraw->d.mainWindow)) {
          	ret = CallWindowProc32A( WIN_FindWndPtr( ddraw->d.mainWindow )->winproc,
               	                   ddraw->d.mainWindow, msg, wParam, lParam );
	  }
        }
        
      } else {
        ret = DefWindowProc32A(hwnd, msg, wParam, lParam );
      } 

    }
    else
    {
	ret = DefWindowProc32A(hwnd,msg,wParam,lParam);
    }

    return ret;
}

HRESULT WINAPI DGA_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
#ifdef HAVE_LIBXXF86DGA
	int	memsize,banksize,width,major,minor,flags,height;
	char	*addr;
	int     fd;             	

        /* Must be able to access /dev/mem for DGA extensions to work, root is not neccessary. --stephenc */
        if ((fd = open("/dev/mem", O_RDWR)) != -1)
	  close(fd);
	
	if (fd  == -1) {
	  MSG("Must be able to access /dev/mem to use XF86DGA!\n");
	  MessageBox32A(0,"Using the XF86DGA extension requires access to /dev/mem.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
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
	(*lplpDD)->e.dga.fb_height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
#ifdef DIABLO_HACK
	(*lplpDD)->e.dga.vpmask = 1;
#else
	(*lplpDD)->e.dga.vpmask = 0;
#endif

	/* just assume the default depth is the DGA depth too */
	(*lplpDD)->d.screen_depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	(*lplpDD)->d.depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
#ifdef RESTORE_SIGNALS
	if (SIGNAL_Reinit) SIGNAL_Reinit();
#endif

	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return DDERR_INVALIDDIRECTDRAWGUID;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

BOOL32
DDRAW_XSHM_Available(void)
   {
#ifdef HAVE_LIBXXSHM
  if (TSXShmQueryExtension(display))
      {
      int major, minor;
      Bool shpix;

      if (TSXShmQueryVersion(display, &major, &minor, &shpix))
	return 1;
      else
	return 0;
    }
    else
    return 0;
#else
  return 0;
#endif
}

HRESULT WINAPI Xlib_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {

	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &xlib_ddvt;
	(*lplpDD)->ref = 1;
	(*lplpDD)->d.drawable = 0; /* in SetDisplayMode */

	/* At DirectDraw creation, the depth is the default depth */
	(*lplpDD)->d.depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	(*lplpDD)->d.screen_depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	(*lplpDD)->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	(*lplpDD)->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);

#ifdef HAVE_LIBXXSHM
	/* Test if XShm is available. */
	if (((*lplpDD)->e.xlib.xshm_active = DDRAW_XSHM_Available()))
	  TRACE(ddraw, "Using XShm extension.\n");
#endif
	
	return DD_OK;
}

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	char	xclsid[50];
	WNDCLASS32A	wc;
        WND*            pParentWindow; 
	HRESULT ret;

	if (HIWORD(lpGUID))
		WINE_StringFromCLSID(lpGUID,xclsid);
	else {
		sprintf(xclsid,"<guid-0x%08x>",(int)lpGUID);
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
	RegisterClass32A(&wc);

	if (!memcmp(lpGUID, &DGA_DirectDraw_GUID, sizeof(GUID)))
		ret = DGA_DirectDrawCreate(lplpDD, pUnkOuter);
	else if (!memcmp(lpGUID, &XLIB_DirectDraw_GUID, sizeof(GUID)))
		ret = Xlib_DirectDrawCreate(lplpDD, pUnkOuter);
	else
	  goto err;


	(*lplpDD)->d.winclass = RegisterClass32A(&wc);
	return ret;

      err:
	fprintf(stderr,"DirectDrawCreate(%s,%p,%p): did not recognize requested GUID\n",xclsid,lplpDD,pUnkOuter);
	return DDERR_INVALIDDIRECTDRAWGUID;
}
