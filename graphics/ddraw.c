/*		DirectDraw using DGA, XShm, or Xlib
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
 */
/* Progress on following programs:
 *
 * - Diablo [640x480x8]:
 *   The movies play. The game doesn't work, it somehow tries to write
 *   into 2 lines _BEFORE_ the start of the surface. Don't know why.
 *
 * - WingCommander 4 / Win95 Patch [640x480x8]:
 *   The intromovie plays, in 8 bit mode (to reconfigure wc4, run wine
 *   "wc4w.exe -I"). The 16bit mode looks broken, but I think this is due to
 *   my Matrox Mystique which uses 565 (rgb) colorweight instead of the usual
 *   555. Specifying it in DDPIXELFORMAT didn't help.
 *   Requires to be run in 640x480xdepth mode (doesn't seem to heed
 *   DDSURFACEDESC.lPitch). You can fly the first mission with Maniac, but
 *   it crashes as soon as you arrive at Blue Point Station...
 *
 * - Monkey Island 3 [640x480x8]:
 *   Goes to the easy/hard selection screen, then hangs due to MT problems.
 * 
 * - DiscWorld 2 [640x480x8]:
 *   [Crashes with 'cli' in released version. Yes. Privileged instructions
 *    in 32bit code. Will they ever learn...]
 *   Plays through nearly all intro movies. Sound and animation skip a lot of
 *   stuff (possible DirectSound problem).
 * 
 * - XvT [640x480x16]:
 *   Shows the splash screen, then fails with missing Joystick.
 *
 * - Tomb Raider 2 Demo (using 8 bit renderer) [640x480x8]:
 *   Playable. Sound is weird.
 *
 * - WingCommander Prophecy Demo (using software renderer) [640x480x16]:
 *   [Crashes with an invalid opcode (outb) in the release version.]
 *   Plays intromovie, hangs in selection screen (no keyboard input, probably
 *   DirectInput problem).
 */

#include "config.h"
#include <unistd.h>
#include <assert.h>
#include "ts_xlib.h"
#ifdef HAVE_LIBXXSHM
#include "ts_xshm.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
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

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

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
static GUID XSHM_DirectDraw_GUID = { /* 2e494ff0-dc61-11d1-8407-860cf3f59f7a */
    0x2e494ff0,
    0xdc61,
    0x11d1,
    {0x84, 0x07, 0x86, 0x0c, 0xf3, 0xf5, 0x9f, 0x7a}
};
static GUID XLIB_DirectDraw_GUID = { /* 1574a740-dc61-11d1-8407-f7875a7d1879 */
    0x1574a740,
    0xdc61,
    0x11d1,
    {0x84, 0x07, 0xf7, 0x87, 0x5a, 0x7d, 0x18, 0x79}
};

static struct IDirectDrawSurface3_VTable	dga_dds3vt, xshm_dds3vt, xlib_dds3vt;
static struct IDirectDrawSurface2_VTable	dga_dds2vt, xshm_dds2vt, xlib_dds2vt;
static struct IDirectDrawSurface_VTable		dga_ddsvt, xshm_ddsvt, xlib_ddsvt;
static struct IDirectDraw_VTable		dga_ddvt, xshm_ddvt, xlib_ddvt;
static struct IDirectDraw2_VTable		dga_dd2vt, xshm_dd2vt, xlib_dd2vt;
static struct IDirectDrawClipper_VTable	ddclipvt;
static struct IDirectDrawPalette_VTable dga_ddpalvt, xshm_ddpalvt, xlib_ddpalvt;
static struct IDirect3D_VTable			d3dvt;
static struct IDirect3D2_VTable			d3d2vt;

BOOL32
DDRAW_DGA_Available()
{
#ifdef HAVE_LIBXXF86DGA
	int evbase, evret;
	return (getuid() == 0) && XF86DGAQueryExtension(display,&evbase,&evret);
#else /* defined(HAVE_LIBXXF86DGA) */
	return 0;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

BOOL32
DDRAW_XShm_Available()
{
#ifdef HAVE_LIBXXSHM
	return TSXShmQueryExtension(display);
#else /* defined(HAVE_LIBXXSHM) */
	return 0;
#endif /* defined(HAVE_LIBXXSHM) */
}

HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	if (DDRAW_DGA_Available()) {
		ddenumproc(&DGA_DirectDraw_GUID,"WINE with XFree86 DGA","wine-dga",data);
	}
	if (DDRAW_XShm_Available()) {
		ddenumproc(&XSHM_DirectDraw_GUID,"WINE with MIT XShm","wine-xshm",data);
	}
	ddenumproc(&XLIB_DirectDraw_GUID,"WINE with Xlib","wine-xlib",data);
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

static void _dump_DDCAPS(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
} flags[] = {
#define FE(x) { x, #x},
		FE(DDCAPS_3D)
		FE(DDCAPS_ALIGNBOUNDARYDEST)
		FE(DDCAPS_ALIGNSIZEDEST)
		FE(DDCAPS_ALIGNBOUNDARYSRC)
		FE(DDCAPS_ALIGNSIZESRC)
		FE(DDCAPS_ALIGNSTRIDE)
		FE(DDCAPS_BLT)
		FE(DDCAPS_BLTQUEUE)
		FE(DDCAPS_BLTFOURCC)
		FE(DDCAPS_BLTSTRETCH)
		FE(DDCAPS_GDI)
		FE(DDCAPS_OVERLAY)
		FE(DDCAPS_OVERLAYCANTCLIP)
		FE(DDCAPS_OVERLAYFOURCC)
		FE(DDCAPS_OVERLAYSTRETCH)
		FE(DDCAPS_PALETTE)
		FE(DDCAPS_PALETTEVSYNC)
		FE(DDCAPS_READSCANLINE)
		FE(DDCAPS_STEREOVIEW)
		FE(DDCAPS_VBI)
		FE(DDCAPS_ZBLTS)
		FE(DDCAPS_ZOVERLAYS)
		FE(DDCAPS_COLORKEY)
		FE(DDCAPS_ALPHA)
		FE(DDCAPS_COLORKEYHWASSIST)
		FE(DDCAPS_NOHARDWARE)
		FE(DDCAPS_BLTCOLORFILL)
		FE(DDCAPS_BANKSWITCHED)
		FE(DDCAPS_BLTDEPTHFILL)
		FE(DDCAPS_CANCLIP)
		FE(DDCAPS_CANCLIPSTRETCHED)
		FE(DDCAPS_CANBLTSYSMEM)
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

static int _getpixelformat(LPDIRECTDRAW ddraw,LPDDPIXELFORMAT pf) {
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
	FIXME(ddraw,"_getpixelformat:oops?\n");
	return DDERR_GENERIC;
}

/******************************************************************************
 *			IDirectDrawSurface
 */

static HRESULT WINAPI IDirectDrawSurface_Lock(
    LPDIRECTDRAWSURFACE this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
        TRACE(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd);
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	    WARN(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
			 this,lprect,lpddsd,flags,(DWORD)hnd);

	if (lprect) {
		TRACE(ddraw,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		lpddsd->y.lpSurface = this->s.surface +
			(lprect->top*this->s.lpitch) +
			(lprect->left*(this->s.ddraw->d.depth/8));
	} else {
		lpddsd->y.lpSurface = this->s.surface;
	}
	lpddsd->dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_LPSURFACE;
	lpddsd->dwWidth		= this->s.width;
	lpddsd->dwHeight	= this->s.height;
	lpddsd->lPitch		= this->s.lpitch;
	_getpixelformat(this->s.ddraw,&(lpddsd->ddpfPixelFormat));
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface_Unlock(
	LPDIRECTDRAWSURFACE this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI XShm_IDirectDrawSurface_Unlock(
	LPDIRECTDRAWSURFACE this,LPVOID surface
) {
#ifdef HAVE_LIBXXSHM
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	if (!this->t.xshm.surface_is_image_data) {
		FIXME(ddraw,"(%p)->Unlock(%p) needs an image copy!\n",this,surface);
	}
	/* FIXME: is it really right to display the image on unlock?
	 * or should it wait for a Flip()? */
	TSXShmPutImage(display,
				   this->s.ddraw->e.xshm.drawable,
				   DefaultGCOfScreen(screen),
				   this->t.xshm.image,
				   0, 0, 0, 0,
				   this->t.xshm.image->width,
				   this->t.xshm.image->height,
				   False);
	if (this->s.palette && this->s.palette->cm) {
		TSXInstallColormap(display,this->s.palette->cm);
	}
	TSXSync(display,False);
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
}

static HRESULT WINAPI Xlib_IDirectDrawSurface_Unlock(
	LPDIRECTDRAWSURFACE this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	if (!this->t.xshm.surface_is_image_data) {
		FIXME(ddraw,"(%p)->Unlock(%p) needs an image copy!\n",this,surface);
	}
	/* FIXME: is it really right to display the image on unlock?
	 * or should it wait for a Flip()? */
	TSXPutImage(display,
				this->s.ddraw->e.xlib.drawable,
				DefaultGCOfScreen(screen),
				this->t.xlib.image,
				0, 0, 0, 0,
				this->t.xlib.image->width,
				this->t.xlib.image->width);
	if (this->s.palette && this->s.palette->cm) {
		TSXInstallColormap(display,this->s.palette->cm);
	}
	TSXSync(display,False);
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
	XF86DGASetViewPort(display,DefaultScreen(display),0,flipto->t.dga.fb_height);
	if (flipto->s.palette && flipto->s.palette->cm) {
		XF86DGAInstallColormap(display,DefaultScreen(display),flipto->s.palette->cm);
	}
	while (!XF86DGAViewPortChanged(display,DefaultScreen(display),2)) {
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

static HRESULT WINAPI XShm_IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
#ifdef HAVE_LIBXXSHM
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
	TSXShmPutImage(display,
				   this->s.ddraw->e.xshm.drawable,
				   DefaultGCOfScreen(screen),
				   flipto->t.xshm.image,
				   0, 0, 0, 0,
				   flipto->t.xshm.image->width,
				   flipto->t.xshm.image->height,
				   False);
	if (flipto->s.palette && flipto->s.palette->cm) {
		TSXInstallColormap(display,flipto->s.palette->cm);
	}
	TSXSync(display,False);
	if (flipto!=this) {
		XImage *tmp;
		tmp = this->t.xshm.image;
		this->t.xshm.image = flipto->t.xshm.image;
		flipto->t.xshm.image = tmp;
	}
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
}

static HRESULT WINAPI Xlib_IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
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
				flipto->t.xlib.image->width);
	if (flipto->s.palette && flipto->s.palette->cm) {
		TSXInstallColormap(display,flipto->s.palette->cm);
	}
	TSXSync(display,False);
	if (flipto!=this) {
		XImage *tmp;
		tmp = this->t.xshm.image;
		this->t.xshm.image = flipto->t.xshm.image;
		flipto->t.xshm.image = tmp;
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_SetPalette(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWPALETTE pal
) {
	TRACE(ddraw,"(%p)->SetPalette(%p)\n",this,pal);
	this->s.palette = pal; /* probably addref it too */
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Blt(
	LPDIRECTDRAWSURFACE this,LPRECT32 rdst,LPDIRECTDRAWSURFACE src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	RECT32	xdst,xsrc;
	int	i,j;

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
	}
	if (dwFlags) {
		FIXME(ddraw,"(%p)->(%p,%p,%p,%08lx,%p),stub!\n",
			this,rdst,src,rsrc,dwFlags,lpbltfx
		);
		if (rdst) TRACE(ddraw,"	destrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
		if (rsrc) TRACE(ddraw,"	srcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
		TRACE(ddraw,"\tflags: ");_dump_DDBLT(dwFlags);fprintf(stderr,"\n");
	}
	if (dwFlags & DDBLT_DDFX) {
		TRACE(ddraw,"	blitfx: \n");_dump_DDBLTFX(lpbltfx->dwDDFX);
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_BltFast(
	LPDIRECTDRAWSURFACE this,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE src,LPRECT32 rsrc,DWORD trans
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

static HRESULT WINAPI IDirectDrawSurface_BltBatch(
	LPDIRECTDRAWSURFACE this,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
	TRACE(ddraw,"(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		this,ddbltbatch,x,y
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetCaps(
	LPDIRECTDRAWSURFACE this,LPDDSCAPS caps
) {
	TRACE(ddraw,"(%p)->GetCaps(%p)\n",this,caps);
	caps->dwCaps = DDCAPS_PALETTE; /* probably more */
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE this,LPDDSURFACEDESC ddsd
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

static ULONG WINAPI IDirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE this) {
	TRACE(ddraw,"(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	TRACE(ddraw,"(%p)->Release()\n",this);
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

static ULONG WINAPI XShm_IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	TRACE(ddraw,"(%p)->Release()\n",this);
#ifdef HAVE_LIBXXSHM
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		if (!this->t.xshm.surface_is_image_data) {
			HeapFree(GetProcessHeap(),0,this->s.surface);
		}
		XShmDetach(display,&this->t.xshm.shminfo);
		XDestroyImage(this->t.xshm.image);
		shmdt(this->t.xshm.shminfo.shmaddr);
		shmctl(this->t.xshm.shminfo.shmid, IPC_RMID, 0);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXSHM) */
	return this->ref;
}

static ULONG WINAPI Xlib_IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	TRACE(ddraw,"(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		if (!this->t.xshm.surface_is_image_data) {
			HeapFree(GetProcessHeap(),0,this->s.surface);
		}
		XDestroyImage(this->t.xshm.image);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectDrawSurface_GetAttachedSurface(
	LPDIRECTDRAWSURFACE this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE *lpdsf
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
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Initialize(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface_GetPixelFormat(
	LPDIRECTDRAWSURFACE this,LPDDPIXELFORMAT pf
) {
	return _getpixelformat(this->s.ddraw,pf);
}

static HRESULT WINAPI IDirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE this,DWORD dwFlags) {
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",
		this,dwFlags
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetOverlayPosition(
	LPDIRECTDRAWSURFACE this,LPLONG x1,LPLONG x2
) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",
		this,x1,x2
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_SetClipper(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWCLIPPER clipper
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,clipper);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_AddAttachedSurface(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE surf
) {
	FIXME(ddraw,"(%p)->(%p),stub!\n",this,surf);
	this->s.backbuffer = surf;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE this,HDC32* lphdc) {
	FIXME(ddraw,"(%p)->GetDC(%p),stub!\n",this,lphdc);
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* thats version 3 (DirectX 5) */
	if (!memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID_IDirectDrawSurface3))) {
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dga_dds3vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	/* thats version 2 (DirectX 3) */
	if (!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID_IDirectDrawSurface2))) {
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dga_dds2vt;
		*obj = this;
		return 0;
	}
	/* thats us */
	if (!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID_IDirectDrawSurface))) {
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI XShm_IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* thats version 3 (DirectX 5) */
	if (	!memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID_IDirectDrawSurface3))) {
		this->lpvtbl->fnAddRef(this);
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&xshm_dds3vt;
		*obj = this;
		return 0;
	}
	/* thats version 2 (DirectX 3) */
	if (!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID_IDirectDrawSurface2))) {
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&xshm_dds2vt;
		*obj = this;
		return 0;
	}
	/* thats us */
	if (!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID_IDirectDrawSurface))) {
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI Xlib_IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* thats version 3 (DirectX 5) */
	if (!memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID_IDirectDrawSurface3))) {
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&xlib_dds3vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	/* thats version 2 (DirectX 3) */
	if (!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID_IDirectDrawSurface2))) {
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&xlib_dds2vt;
		*obj = this;
		return 0;
	}
	/* thats us */
	if (!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID_IDirectDrawSurface))) {
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
    	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE this) {
	return 0; /* hmm */
}

static struct IDirectDrawSurface_VTable dga_ddsvt = {
	DGA_IDirectDrawSurface_QueryInterface,
	IDirectDrawSurface_AddRef,
	DGA_IDirectDrawSurface_Release,
	IDirectDrawSurface_AddAttachedSurface,
	(void*)5,
	IDirectDrawSurface_Blt,
	IDirectDrawSurface_BltBatch,
	IDirectDrawSurface_BltFast,
	(void*)9,
	(void*)10,
	(void*)11,
	DGA_IDirectDrawSurface_Flip,
	IDirectDrawSurface_GetAttachedSurface,
	IDirectDrawSurface_GetBltStatus,
	IDirectDrawSurface_GetCaps,
	(void*)16,
	(void*)17,
	IDirectDrawSurface_GetDC,
	(void*)19,
	IDirectDrawSurface_GetOverlayPosition,
	(void*)21,
	IDirectDrawSurface_GetPixelFormat,
	IDirectDrawSurface_GetSurfaceDesc,
	IDirectDrawSurface_Initialize,
	IDirectDrawSurface_IsLost,
	IDirectDrawSurface_Lock,
	(void*)27,
	(void*)28,
	IDirectDrawSurface_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface_SetPalette,
	DGA_IDirectDrawSurface_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
};

static struct IDirectDrawSurface_VTable xshm_ddsvt = {
	XShm_IDirectDrawSurface_QueryInterface,
	IDirectDrawSurface_AddRef,
	XShm_IDirectDrawSurface_Release,
	IDirectDrawSurface_AddAttachedSurface,
	(void*)5,
	IDirectDrawSurface_Blt,
	IDirectDrawSurface_BltBatch,
	IDirectDrawSurface_BltFast,
	(void*)9,
	(void*)10,
	(void*)11,
	XShm_IDirectDrawSurface_Flip,
	IDirectDrawSurface_GetAttachedSurface,
	IDirectDrawSurface_GetBltStatus,
	IDirectDrawSurface_GetCaps,
	(void*)16,
	(void*)17,
	IDirectDrawSurface_GetDC,
	(void*)19,
	IDirectDrawSurface_GetOverlayPosition,
	(void*)21,
	IDirectDrawSurface_GetPixelFormat,
	IDirectDrawSurface_GetSurfaceDesc,
	IDirectDrawSurface_Initialize,
	IDirectDrawSurface_IsLost,
	IDirectDrawSurface_Lock,
	(void*)27,
	(void*)28,
	IDirectDrawSurface_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface_SetPalette,
	XShm_IDirectDrawSurface_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
};

static struct IDirectDrawSurface_VTable xlib_ddsvt = {
	Xlib_IDirectDrawSurface_QueryInterface,
	IDirectDrawSurface_AddRef,
	Xlib_IDirectDrawSurface_Release,
	IDirectDrawSurface_AddAttachedSurface,
	(void*)5,
	IDirectDrawSurface_Blt,
	IDirectDrawSurface_BltBatch,
	IDirectDrawSurface_BltFast,
	(void*)9,
	(void*)10,
	(void*)11,
	Xlib_IDirectDrawSurface_Flip,
	IDirectDrawSurface_GetAttachedSurface,
	IDirectDrawSurface_GetBltStatus,
	IDirectDrawSurface_GetCaps,
	(void*)16,
	(void*)17,
	IDirectDrawSurface_GetDC,
	(void*)19,
	IDirectDrawSurface_GetOverlayPosition,
	(void*)21,
	IDirectDrawSurface_GetPixelFormat,
	IDirectDrawSurface_GetSurfaceDesc,
	IDirectDrawSurface_Initialize,
	IDirectDrawSurface_IsLost,
	IDirectDrawSurface_Lock,
	(void*)27,
	(void*)28,
	IDirectDrawSurface_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface_SetPalette,
	Xlib_IDirectDrawSurface_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
};

/******************************************************************************
 *			IDirectDrawSurface2
 *
 * calls IDirectDrawSurface methods where possible
 */
static HRESULT WINAPI IDirectDrawSurface2_Lock(
    LPDIRECTDRAWSURFACE2 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	return IDirectDrawSurface_Lock((LPDIRECTDRAWSURFACE)this,lprect,lpddsd,flags,hnd);
}

static HRESULT WINAPI DGA_IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	return DGA_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI XShm_IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	return DGA_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	return DGA_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI IDirectDrawSurface2_SetPalette(
	LPDIRECTDRAWSURFACE2 this,LPDIRECTDRAWPALETTE pal
) {
	return IDirectDrawSurface_SetPalette((LPDIRECTDRAWSURFACE)this,pal);
}

static ULONG WINAPI IDirectDrawSurface2_AddRef(LPDIRECTDRAWSURFACE2 this) {
	TRACE(ddraw,"(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	return DGA_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static ULONG WINAPI XShm_IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	return XShm_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static ULONG WINAPI Xlib_IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	return Xlib_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static HRESULT WINAPI IDirectDrawSurface2_Blt(
	LPDIRECTDRAWSURFACE2 this,LPRECT32 rdst,LPDIRECTDRAWSURFACE2 src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	return IDirectDrawSurface_Blt((LPDIRECTDRAWSURFACE)this, rdst, (LPDIRECTDRAWSURFACE)src, rsrc, dwFlags,lpbltfx);
}

static HRESULT WINAPI IDirectDrawSurface2_BltFast(
	LPDIRECTDRAWSURFACE2 this,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE2 src,LPRECT32 rsrc,DWORD trans
) {
	return IDirectDrawSurface_BltFast((LPDIRECTDRAWSURFACE)this,dstx,dsty,(LPDIRECTDRAWSURFACE)src,rsrc,trans);
}

static HRESULT WINAPI IDirectDrawSurface2_BltBatch(
	LPDIRECTDRAWSURFACE2 this,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
	return IDirectDrawSurface_BltBatch((LPDIRECTDRAWSURFACE)this,ddbltbatch,x,y);
}

static HRESULT WINAPI DGA_IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &dga_dds2vt;
	}
	return ret;
}

static HRESULT WINAPI XShm_IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &xshm_dds2vt;
	}
	return ret;
}

static HRESULT WINAPI Xlib_IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &xlib_dds2vt;
	}
	return ret;
}

static HRESULT WINAPI DGA_IDirectDrawSurface2_Flip(
	LPDIRECTDRAWSURFACE2 this,LPDIRECTDRAWSURFACE2 flipto,DWORD dwFlags
) {
	return DGA_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI XShm_IDirectDrawSurface2_Flip(
	LPDIRECTDRAWSURFACE2 this,LPDIRECTDRAWSURFACE2 flipto,DWORD dwFlags
) {
	return XShm_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface2_Flip(
	LPDIRECTDRAWSURFACE2 this,LPDIRECTDRAWSURFACE2 flipto,DWORD dwFlags
) {
	return Xlib_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI IDirectDrawSurface2_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE2 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,context,esfcb);
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawSurface2_QueryInterface(
	LPDIRECTDRAWSURFACE2 this,REFIID riid,LPVOID *ppobj
) {
	return DGA_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI XShm_IDirectDrawSurface2_QueryInterface(
	LPDIRECTDRAWSURFACE2 this,REFIID riid,LPVOID *ppobj
) {
	return XShm_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface2_QueryInterface(
	LPDIRECTDRAWSURFACE2 this,REFIID riid,LPVOID *ppobj
) {
	return Xlib_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI IDirectDrawSurface2_IsLost(LPDIRECTDRAWSURFACE2 this) {
	return 0; /* hmm */
}

static struct IDirectDrawSurface2_VTable dga_dds2vt = {
	DGA_IDirectDrawSurface2_QueryInterface,
	IDirectDrawSurface2_AddRef,
	DGA_IDirectDrawSurface2_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface2_Blt,
	IDirectDrawSurface2_BltBatch,
	IDirectDrawSurface2_BltFast,
	(void*)9,
	IDirectDrawSurface2_EnumAttachedSurfaces,
	(void*)11,
	DGA_IDirectDrawSurface2_Flip,
	DGA_IDirectDrawSurface2_GetAttachedSurface,
	(void*)14,
	(void*)15/*IDirectDrawSurface_GetCaps*/,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	(void*)22,
	(void*)23/*IDirectDrawSurface_GetSurfaceDesc*/,
	(void*)24,
	IDirectDrawSurface2_IsLost,
	IDirectDrawSurface2_Lock,
	(void*)27,
	(void*)28,
	(void*)29,
	(void*)30,
	(void*)31,
	IDirectDrawSurface2_SetPalette,
	DGA_IDirectDrawSurface2_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
};

static struct IDirectDrawSurface2_VTable xshm_dds2vt = {
	XShm_IDirectDrawSurface2_QueryInterface,
	IDirectDrawSurface2_AddRef,
	XShm_IDirectDrawSurface2_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface2_Blt,
	IDirectDrawSurface2_BltBatch,
	IDirectDrawSurface2_BltFast,
	(void*)9,
	IDirectDrawSurface2_EnumAttachedSurfaces,
	(void*)11,
	XShm_IDirectDrawSurface2_Flip,
	XShm_IDirectDrawSurface2_GetAttachedSurface,
	(void*)14,
	(void*)15/*IDirectDrawSurface_GetCaps*/,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	(void*)22,
	(void*)23/*IDirectDrawSurface_GetSurfaceDesc*/,
	(void*)24,
	IDirectDrawSurface2_IsLost,
	IDirectDrawSurface2_Lock,
	(void*)27,
	(void*)28,
	(void*)29,
	(void*)30,
	(void*)31,
	IDirectDrawSurface2_SetPalette,
	XShm_IDirectDrawSurface2_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
};

static struct IDirectDrawSurface2_VTable xlib_dds2vt = {
	Xlib_IDirectDrawSurface2_QueryInterface,
	IDirectDrawSurface2_AddRef,
	Xlib_IDirectDrawSurface2_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface2_Blt,
	IDirectDrawSurface2_BltBatch,
	IDirectDrawSurface2_BltFast,
	(void*)9,
	IDirectDrawSurface2_EnumAttachedSurfaces,
	(void*)11,
	Xlib_IDirectDrawSurface2_Flip,
	Xlib_IDirectDrawSurface2_GetAttachedSurface,
	(void*)14,
	(void*)15/*IDirectDrawSurface_GetCaps*/,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	(void*)22,
	(void*)23/*IDirectDrawSurface_GetSurfaceDesc*/,
	(void*)24,
	IDirectDrawSurface2_IsLost,
	IDirectDrawSurface2_Lock,
	(void*)27,
	(void*)28,
	(void*)29,
	(void*)30,
	(void*)31,
	IDirectDrawSurface2_SetPalette,
	Xlib_IDirectDrawSurface2_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
};

/******************************************************************************
 *			IDirectDrawSurface3
 */
static HRESULT WINAPI IDirectDrawSurface3_SetPalette(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWPALETTE pal
) {
	return IDirectDrawSurface_SetPalette((LPDIRECTDRAWSURFACE)this,pal);
}

static HRESULT WINAPI IDirectDrawSurface3_GetPixelFormat(
	LPDIRECTDRAWSURFACE3 this,LPDDPIXELFORMAT pf
) {
	return _getpixelformat(this->s.ddraw,pf);
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_GetAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE3 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &dga_dds3vt;
	}
	return ret;
}

static HRESULT WINAPI XShm_IDirectDrawSurface3_GetAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE3 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &xshm_dds3vt;
	}
	return ret;
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_GetAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE3 *lpdsf
) {
	HRESULT	ret;

	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret) {
		(*lpdsf)->lpvtbl = &xlib_dds3vt;
	}
	return ret;
}

static ULONG WINAPI IDirectDrawSurface3_AddRef(LPDIRECTDRAWSURFACE3 this) {
	TRACE(ddraw,"(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
	return DGA_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static ULONG WINAPI XShm_IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
	return XShm_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static ULONG WINAPI Xlib_IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
	return Xlib_IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static HRESULT WINAPI IDirectDrawSurface3_Blt(
        LPDIRECTDRAWSURFACE3 this,LPRECT32 rdst,LPDIRECTDRAWSURFACE3 src,
	LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	return IDirectDrawSurface_Blt((LPDIRECTDRAWSURFACE)this,rdst,(LPDIRECTDRAWSURFACE)src,rsrc,dwFlags,lpbltfx);
}

static HRESULT WINAPI IDirectDrawSurface3_IsLost(LPDIRECTDRAWSURFACE3 this) {
	return 0; /* hmm */
}

static HRESULT WINAPI IDirectDrawSurface3_Restore(LPDIRECTDRAWSURFACE3 this) {
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_GetBltStatus(
	LPDIRECTDRAWSURFACE3 this,DWORD dwflags
) {
	return IDirectDrawSurface_GetBltStatus((LPDIRECTDRAWSURFACE)this,dwflags);
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
	return DGA_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI XShm_IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
	return XShm_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
	return Xlib_IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI IDirectDrawSurface3_Lock(
    LPDIRECTDRAWSURFACE3 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	return IDirectDrawSurface_Lock((LPDIRECTDRAWSURFACE)this,lprect,lpddsd,flags,hnd); 
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface
) {
	return DGA_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI XShm_IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface
) {
	return XShm_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface
) {
	return Xlib_IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
}

static HRESULT WINAPI IDirectDrawSurface3_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE3 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,context,esfcb);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface3_SetClipper(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWCLIPPER clipper
) {
	return IDirectDrawSurface_SetClipper((LPDIRECTDRAWSURFACE)this,clipper);
}

static HRESULT WINAPI DGA_IDirectDrawSurface3_QueryInterface(
	LPDIRECTDRAWSURFACE3 this,REFIID riid,LPVOID *ppobj
) {
	return DGA_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI XShm_IDirectDrawSurface3_QueryInterface(
	LPDIRECTDRAWSURFACE3 this,REFIID riid,LPVOID *ppobj
) {
	return XShm_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface3_QueryInterface(
	LPDIRECTDRAWSURFACE3 this,REFIID riid,LPVOID *ppobj
) {
	return Xlib_IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static struct IDirectDrawSurface3_VTable dga_dds3vt = {
	DGA_IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	DGA_IDirectDrawSurface3_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface3_Blt,
	(void*)7,
	(void*)8,
	(void*)9,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	(void*)11,
	DGA_IDirectDrawSurface3_Flip,
	DGA_IDirectDrawSurface3_GetAttachedSurface,
	IDirectDrawSurface3_GetBltStatus,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	IDirectDrawSurface3_GetPixelFormat,
	(void*)23,
	(void*)24,
	IDirectDrawSurface3_IsLost,
	IDirectDrawSurface3_Lock,
	(void*)27,
	IDirectDrawSurface3_Restore,
	IDirectDrawSurface3_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface3_SetPalette,
	DGA_IDirectDrawSurface3_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
	(void*)40,
};

static struct IDirectDrawSurface3_VTable xshm_dds3vt = {
	XShm_IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	XShm_IDirectDrawSurface3_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface3_Blt,
	(void*)7,
	(void*)8,
	(void*)9,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	(void*)11,
	XShm_IDirectDrawSurface3_Flip,
	XShm_IDirectDrawSurface3_GetAttachedSurface,
	IDirectDrawSurface3_GetBltStatus,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	IDirectDrawSurface3_GetPixelFormat,
	(void*)23,
	(void*)24,
	IDirectDrawSurface3_IsLost,
	IDirectDrawSurface3_Lock,
	(void*)27,
	IDirectDrawSurface3_Restore,
	IDirectDrawSurface3_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface3_SetPalette,
	XShm_IDirectDrawSurface3_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
	(void*)40,
};

static struct IDirectDrawSurface3_VTable xlib_dds3vt = {
	Xlib_IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	Xlib_IDirectDrawSurface3_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface3_Blt,
	(void*)7,
	(void*)8,
	(void*)9,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	(void*)11,
	Xlib_IDirectDrawSurface3_Flip,
	Xlib_IDirectDrawSurface3_GetAttachedSurface,
	IDirectDrawSurface3_GetBltStatus,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	(void*)20,
	(void*)21,
	IDirectDrawSurface3_GetPixelFormat,
	(void*)23,
	(void*)24,
	IDirectDrawSurface3_IsLost,
	IDirectDrawSurface3_Lock,
	(void*)27,
	IDirectDrawSurface3_Restore,
	IDirectDrawSurface3_SetClipper,
	(void*)30,
	(void*)31,
	IDirectDrawSurface3_SetPalette,
	Xlib_IDirectDrawSurface3_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
	(void*)37,
	(void*)38,
	(void*)39,
	(void*)40,
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

static struct IDirectDrawClipper_VTable ddclipvt = {
	(void*)1,
	(void*)2,
	IDirectDrawClipper_Release,
	IDirectDrawClipper_GetClipList,
	(void*)5,
	(void*)6,
	(void*)7,
	(void*)8,
	IDirectDrawClipper_SetHwnd
};

/******************************************************************************
 *			IDirectDrawPalette
 */
static HRESULT WINAPI IDirectDrawPalette_GetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
	XColor xc;
	int	i;

	if (!this->cm) /* should not happen */ {
		TRACE(ddraw,"app tried to read colormap for non-palettized mode\n");
		return DDERR_GENERIC;
	}
	for (i=start;i<end;i++) {
		xc.pixel = i;
		TSXQueryColor(display,this->cm,&xc);
		palent[i-start].peRed = xc.red>>8;
		palent[i-start].peGreen = xc.green>>8;
		palent[i-start].peBlue = xc.blue>>8;
	}
	return 0;
}

static HRESULT WINAPI common_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
	XColor		xc;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,end,palent
	);
	if (!this->cm) /* should not happen */ {
		TRACE(ddraw,"app tried to set colormap in non-palettized mode\n");
		return DDERR_GENERIC;
	}
	/* FIXME: free colorcells instead of freeing whole map */
	this->cm = TSXCopyColormapAndFree(display,this->cm);
	for (i=start;i<end;i++) {
		xc.red = palent[i-start].peRed<<8;
		xc.blue = palent[i-start].peBlue<<8;
		xc.green = palent[i-start].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i;
		TSXStoreColor(display,this->cm,&xc);
		this->palents[i].peRed = palent[i-start].peRed;
		this->palents[i].peBlue = palent[i-start].peBlue;
		this->palents[i].peGreen = palent[i-start].peGreen;
	}
	return 0;
}

static HRESULT WINAPI DGA_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
#ifdef HAVE_LIBXXF86DGA
	HRESULT hres;
	hres = common_IDirectDrawPalette_SetEntries(this,x,start,end,palent);
	if (hres != 0) return hres;
	XF86DGAInstallColormap(display,DefaultScreen(display),this->cm);
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
	HRESULT hres;
	hres = common_IDirectDrawPalette_SetEntries(this,x,start,end,palent);
	if (hres != 0) return hres;
	TSXInstallColormap(display,this->cm);
	return 0;
}

static ULONG WINAPI IDirectDrawPalette_Release(LPDIRECTDRAWPALETTE this) {
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
	return ++(this->ref);
}

static HRESULT WINAPI IDirectDrawPalette_Initialize(
	LPDIRECTDRAWPALETTE this,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
	return DDERR_ALREADYINITIALIZED;
}

static struct IDirectDrawPalette_VTable dga_ddpalvt = {
	(void*)1,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	(void*)4,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	DGA_IDirectDrawPalette_SetEntries
};

static struct IDirectDrawPalette_VTable xshm_ddpalvt = {
	(void*)1,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	(void*)4,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	Xlib_IDirectDrawPalette_SetEntries
};

static struct IDirectDrawPalette_VTable xlib_ddpalvt = {
	(void*)1,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	(void*)4,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	Xlib_IDirectDrawPalette_SetEntries
};

/*******************************************************************************
 *				IDirect3D
 */
static struct IDirect3D_VTable d3dvt = {
	(void*)1,
	(void*)2,
	(void*)3,
	(void*)4,
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
	this->ref--;
	if (!this->ref) {
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
	d1.dwSize	= sizeof(d1);
	d1.dwFlags	= 0;

	d2.dwSize	= sizeof(d2);
	d2.dwFlags	= 0;
	cb(&IID_IDirect3DHALDevice,"WINE Direct3D HAL","direct3d",&d1,&d2,context);
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
static HRESULT WINAPI DGA_IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
#ifdef HAVE_LIBXXF86DGA
	int	i;

	TRACE(ddraw, "(%p)->(%p,%p,%p)\n",
		     this,lpddsd,lpdsf,lpunk);
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
	(*lpdsf)->lpvtbl = &dga_ddsvt;
	if (	(lpddsd->dwFlags & DDSD_CAPS) && 
		(lpddsd->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
	) {
		if (!(lpddsd->dwFlags & DDSD_WIDTH))
			lpddsd->dwWidth = this->e.dga.fb_width;
		if (!(lpddsd->dwFlags & DDSD_HEIGHT))
			lpddsd->dwWidth = this->e.dga.fb_height;
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
		LPDIRECTDRAWSURFACE	back;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

		(*lpdsf)->s.backbuffer = back = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
		this->lpvtbl->fnAddRef(this);
		back->ref = 1;
		back->lpvtbl = &dga_ddsvt;
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

static HRESULT WINAPI XShm_IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
#ifdef HAVE_LIBXXSHM
	XImage *img;
	int shmid;
	TRACE(ddraw, "(%p)->CreateSurface(%p,%p,%p)\n",
		     this,lpddsd,lpdsf,lpunk);
	if (TRACE_ON(ddraw)) {
		fprintf(stderr,"[w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
		_dump_DDSD(lpddsd->dwFlags);
		fprintf(stderr,"caps ");
		_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
		fprintf(stderr,"]\n");
	}

	TRACE(ddraw,"using shared XImage for a primary surface\n");
	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &xshm_ddsvt;
	(*lpdsf)->t.xshm.image = img =
		XShmCreateImage(display, /*FIXME:visual*/0, /*FIXME:depth*/8, ZPixmap,
						NULL, &(*lpdsf)->t.xshm.shminfo,
						/*FIXME:width*/640, /*FIXME:height*/480);
	(*lpdsf)->t.xshm.shminfo.shmid = shmid =
		shmget(IPC_PRIVATE, img->bytes_per_line*img->height, IPC_CREAT|0777);
	(*lpdsf)->t.xshm.shminfo.shmaddr = img->data = shmat(shmid, 0, 0);
	XShmAttach(display, &(*lpdsf)->t.xshm.shminfo);
	/* POOLE FIXME: XShm: this will easily break */
	(*lpdsf)->t.xshm.surface_is_image_data = TRUE;
	(*lpdsf)->s.surface = img->data;
	/* END FIXME: XShm */
	(*lpdsf)->s.lpitch = img->bytes_per_line;
	(*lpdsf)->s.width = img->width;
	(*lpdsf)->s.height = img->height;
	(*lpdsf)->s.ddraw = this;
	(*lpdsf)->s.backbuffer = NULL;
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
}

static HRESULT WINAPI Xlib_IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	XImage *img;
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
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &xlib_ddsvt;
	TRACE(ddraw,"using standard XImage for a primary surface\n");
	/* POOLE FIXME: Xlib: this will easily break */
	(*lpdsf)->t.xshm.surface_is_image_data = TRUE;
	(*lpdsf)->s.surface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,640*480);
	(*lpdsf)->t.xlib.image = img =
		XCreateImage(display, /*FIXME:visual*/0, /*FIXME: depth*/8, ZPixmap,
					 0, (*lpdsf)->s.surface,
					 /*FIXME:width*/640, /*FIXME:height*/480, 0, 640*1);
	/* END FIXME: Xlib */
	(*lpdsf)->s.lpitch = img->bytes_per_line;
	(*lpdsf)->s.width = img->width;
	(*lpdsf)->s.height = img->height;
	(*lpdsf)->s.ddraw = this;
	(*lpdsf)->s.backbuffer = NULL;
	return 0;
}

static HRESULT WINAPI IDirectDraw_DuplicateSurface(
	LPDIRECTDRAW this,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
	FIXME(ddraw,"(%p)->(%p,%p) simply copies\n",this,src,dst);
	*dst = src; /* FIXME */
	return 0;
}

static HRESULT WINAPI IDirectDraw_SetCooperativeLevel(
	LPDIRECTDRAW this,HWND32 hwnd,DWORD cooplevel
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

	TRACE(ddraw,"(%p)->(%08lx,%08lx)\n",
		this,(DWORD)hwnd,cooplevel
	);
	if(TRACE_ON(ddraw)){
	  dbg_decl_str(ddraw, 512);
	  for (i=0;i<sizeof(flagmap)/sizeof(flagmap[0]);i++)
	    if (flagmap[i].mask & cooplevel)
	      dsprintf(ddraw, "%s ", flagmap[i].name);
	  TRACE(ddraw,"	cooperative level %s\n", dbg_str(ddraw));
	}
	this->d.mainwindow = hwnd;
	return 0;
}


static HRESULT WINAPI DGA_IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
#ifdef HAVE_LIBXXF86DGA
	int	i,*depths,depcount;

	TRACE(ddraw, "(%p)->(%ld,%ld,%ld)\n",
		      this, width, height, depth);

	depths = TSXListDepths(display,DefaultScreen(display),&depcount);
	for (i=0;i<depcount;i++)
		if (depths[i]==depth)
			break;
	TSXFree(depths);
	if (i==depcount) {/* not found */
		ERR(ddraw,"(w=%ld,h=%ld,d=%ld), unsupported depth!\n",width,height,depth);
		return DDERR_UNSUPPORTEDMODE;
	}
	if (this->e.dga.fb_width < width) {
		ERR(ddraw,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld\n",width,height,depth,width,this->e.dga.fb_width);
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
	XF86DGADirectVideo(display,DefaultScreen(display),XF86DGADirectGraphics);
/*
	XF86DGASetViewPort(display,DefaultScreen(display),0,this->d.fb_height);
 */

#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI XShm_IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
#ifdef HAVE_LIBXXSHM
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
	/* POOLE FIXME: XShm */
	if (this->e.dga.fb_width < width) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld",width,height,depth,width,this->e.dga.fb_width);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	}
	this->d.width	= width;
	this->d.height	= height;
	/* adjust fb_height, so we don't overlap */
	if (this->e.dga.fb_height < height)
		this->e.dga.fb_height = height;
	this->d.depth	= depth;
	/* END FIXME: XShm */
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
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
	/* POOLE FIXME: Xlib */
	if (this->e.dga.fb_width < width) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld",width,height,depth,width,this->e.dga.fb_width);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	}
	this->d.width	= width;
	this->d.height	= height;
	/* adjust fb_height, so we don't overlap */
	if (this->e.dga.fb_height < height)
		this->e.dga.fb_height = height;
	this->d.depth	= depth;
	/* END FIXME: Xlib */
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw_GetCaps(
	LPDIRECTDRAW this,LPDDCAPS caps1,LPDDCAPS caps2
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

static HRESULT WINAPI XShm_IDirectDraw_GetCaps(
	LPDIRECTDRAW this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
#ifdef HAVE_LIBXXSHM
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",this,caps1,caps2);
	/* FIXME: XShm */
	caps1->dwVidMemTotal = 2048*1024;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	if (caps2) {
		caps2->dwVidMemTotal = 2048*1024;
		caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
	/* END FIXME: XShm */
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
}

static HRESULT WINAPI Xlib_IDirectDraw_GetCaps(
	LPDIRECTDRAW this,LPDDCAPS caps1,LPDDCAPS caps2
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

static HRESULT WINAPI IDirectDraw_CreateClipper(
	LPDIRECTDRAW this,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
	FIXME(ddraw,"(%p)->(%08lx,%p,%p),stub!\n",
		this,x,lpddclip,lpunk
	);
	*lpddclip = (LPDIRECTDRAWCLIPPER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipper));
	(*lpddclip)->ref = 1;
	(*lpddclip)->lpvtbl = &ddclipvt;
	return 0;
}

static HRESULT WINAPI common_IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	if (*lpddpal == NULL) return E_OUTOFMEMORY;
	(*lpddpal)->ref = 1;
	(*lpddpal)->ddraw = this;
	if (this->d.depth<=8) {
		(*lpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(screen),AllocAll);
	} else {
		/* we don't want palettes in hicolor or truecolor */
		(*lpddpal)->cm = 0;
	}
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	HRESULT res;
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",
		this,x,palent,lpddpal,lpunk
	);
	res = common_IDirectDraw_CreatePalette(this,x,palent,lpddpal,lpunk);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &dga_ddpalvt;
	return 0;
}

static HRESULT WINAPI XShm_IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	HRESULT res;
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",
		this,x,palent,lpddpal,lpunk
	);
	res = common_IDirectDraw_CreatePalette(this,x,palent,lpddpal,lpunk);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &xshm_ddpalvt;
	return 0;
}

static HRESULT WINAPI Xlib_IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	HRESULT res;
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",
		this,x,palent,lpddpal,lpunk
	);
	res = common_IDirectDraw_CreatePalette(this,x,palent,lpddpal,lpunk);
	if (res != 0) return res;
	(*lpddpal)->lpvtbl = &xlib_ddpalvt;
	return 0;
}

static HRESULT WINAPI DGA_IDirectDraw_RestoreDisplayMode(LPDIRECTDRAW this) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw, "(%p)->()\n", 
		      this);
	Sleep(1000);
	XF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif
}

static HRESULT WINAPI XShm_IDirectDraw_RestoreDisplayMode(LPDIRECTDRAW this) {
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw, "(%p)->RestoreDisplayMode()\n", 
		      this);
	Sleep(1000);
	return 0;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif
}

static HRESULT WINAPI Xlib_IDirectDraw_RestoreDisplayMode(LPDIRECTDRAW this) {
	TRACE(ddraw, "(%p)->RestoreDisplayMode()\n", 
		      this);
	return 0;
}

static HRESULT WINAPI IDirectDraw_WaitForVerticalBlank(
	LPDIRECTDRAW this,DWORD x,HANDLE32 h
) {
	TRACE(ddraw,"(%p)->(0x%08lx,0x%08x)\n",this,x,h);
	return 0;
}

static ULONG WINAPI IDirectDraw_AddRef(LPDIRECTDRAW this) {
	return ++(this->ref);
}

static ULONG WINAPI DGA_IDirectDraw_Release(LPDIRECTDRAW this) {
#ifdef HAVE_LIBXXF86DGA
	if (!--(this->ref)) {
		XF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
		SIGNAL_InitEmulator();
#endif
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return this->ref;
}

static ULONG WINAPI XShm_IDirectDraw_Release(LPDIRECTDRAW this) {
#ifdef HAVE_LIBXXSHM
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
#endif /* defined(HAVE_LIBXXSHM) */
	return this->ref;
}

static ULONG WINAPI Xlib_IDirectDraw_Release(LPDIRECTDRAW this) {
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI DGA_IDirectDraw_QueryInterface(
	LPDIRECTDRAW this,REFIID refiid,LPVOID *obj
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
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW_VTABLE)&dga_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI XShm_IDirectDraw_QueryInterface(
	LPDIRECTDRAW this,REFIID refiid,LPVOID *obj
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
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW_VTABLE)&xshm_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI Xlib_IDirectDraw_QueryInterface(
	LPDIRECTDRAW this,REFIID refiid,LPVOID *obj
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
		*obj = this;
		this->lpvtbl->fnAddRef(this);
		return 0;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		this->lpvtbl = (LPDIRECTDRAW_VTABLE)&xlib_dd2vt;
		this->lpvtbl->fnAddRef(this);
		*obj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;
		return 0;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		LPDIRECT3D2	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = this;
		this->lpvtbl->fnAddRef(this);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;
		return 0;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",this,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw_GetVerticalBlankStatus(
	LPDIRECTDRAW this,BOOL32 *status
) {
        TRACE(ddraw,"(%p)->(%p)\n",this,status);
	*status = TRUE;
	return 0;
}

static HRESULT WINAPI IDirectDraw_EnumDisplayModes(
	LPDIRECTDRAW this,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
	DDSURFACEDESC	ddsfd;

	TRACE(ddraw,"(%p)->(0x%08lx,%p,%p,%p)\n",this,dwFlags,lpddsfd,context,modescb);

	_getpixelformat(this,&(ddsfd.ddpfPixelFormat));
	ddsfd.dwSize = sizeof(ddsfd);
	ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	if (dwFlags & DDEDM_REFRESHRATES) {
		ddsfd.dwFlags |= DDSD_REFRESHRATE;
		ddsfd.x.dwRefreshRate = 60;
	}

	ddsfd.dwWidth = 640;
	ddsfd.dwHeight = 480;
	ddsfd.dwBackBufferCount = 1;
	ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;

	if (!modescb(&ddsfd,context)) return 0;

	ddsfd.dwWidth = 800;
	ddsfd.dwHeight = 600;
	if (!modescb(&ddsfd,context)) return 0;

	if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
		/* modeX is not standard VGA */

		ddsfd.dwHeight = 200;
		ddsfd.dwWidth = 320;
		if (!modescb(&ddsfd,context)) return 0;
	}
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw_GetDisplayMode(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsfd
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

static HRESULT WINAPI XShm_IDirectDraw_GetDisplayMode(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsfd
) {
#ifdef HAVE_LIBXXSM
	TRACE(ddraw,"(%p)->GetDisplayMode(%p)\n",this,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = screenHeight;
	lpddsfd->dwWidth = screenWidth;
	/* POOLE FIXME: XShm */
	lpddsfd->lPitch = this->e.dga.fb_width*this->d.depth/8;
	/* END FIXME: XShm */
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	_getpixelformat(this,&(lpddsfd->ddpfPixelFormat));
	return DD_OK;
#else /* defined(HAVE_LIBXXSHM) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXSHM) */
}

static HRESULT WINAPI Xlib_IDirectDraw_GetDisplayMode(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsfd
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

static HRESULT WINAPI IDirectDraw_FlipToGDISurface(LPDIRECTDRAW this) {
	TRACE(ddraw,"(%p)->()\n",this);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw_GetMonitorFrequency(
	LPDIRECTDRAW this,LPDWORD freq
) {
	FIXME(ddraw,"(%p)->(%p) returns 60 Hz always\n",this,freq);
	*freq = 60*100; /* 60 Hz */
	return 0;
}

/* what can we directly decompress? */
static HRESULT WINAPI IDirectDraw_GetFourCCCodes(
	LPDIRECTDRAW this,LPDWORD x,LPDWORD y
) {
	FIXME(ddraw,"(%p,%p,%p), stub\n",this,x,y);
	return 0;
}

static struct IDirectDraw_VTable dga_ddvt = {
	DGA_IDirectDraw_QueryInterface,
	IDirectDraw_AddRef,
	DGA_IDirectDraw_Release,
	(void*)4,
	IDirectDraw_CreateClipper,
	DGA_IDirectDraw_CreatePalette,
	DGA_IDirectDraw_CreateSurface,
	IDirectDraw_DuplicateSurface,
	IDirectDraw_EnumDisplayModes,
	(void*)10,
	IDirectDraw_FlipToGDISurface,
	DGA_IDirectDraw_GetCaps,
	DGA_IDirectDraw_GetDisplayMode,
	IDirectDraw_GetFourCCCodes,
	(void*)15,
	IDirectDraw_GetMonitorFrequency,
	(void*)17,
	IDirectDraw_GetVerticalBlankStatus,
	(void*)19,
	DGA_IDirectDraw_RestoreDisplayMode,
	IDirectDraw_SetCooperativeLevel,
	DGA_IDirectDraw_SetDisplayMode,
	IDirectDraw_WaitForVerticalBlank,
};

static struct IDirectDraw_VTable xshm_ddvt = {
	XShm_IDirectDraw_QueryInterface,
	IDirectDraw_AddRef,
	XShm_IDirectDraw_Release,
	(void*)4,
	IDirectDraw_CreateClipper,
	XShm_IDirectDraw_CreatePalette,
	XShm_IDirectDraw_CreateSurface,
	IDirectDraw_DuplicateSurface,
	IDirectDraw_EnumDisplayModes,
	(void*)10,
	IDirectDraw_FlipToGDISurface,
	XShm_IDirectDraw_GetCaps,
	XShm_IDirectDraw_GetDisplayMode,
	IDirectDraw_GetFourCCCodes,
	(void*)15,
	IDirectDraw_GetMonitorFrequency,
	(void*)17,
	IDirectDraw_GetVerticalBlankStatus,
	(void*)19,
	XShm_IDirectDraw_RestoreDisplayMode,
	IDirectDraw_SetCooperativeLevel,
	XShm_IDirectDraw_SetDisplayMode,
	IDirectDraw_WaitForVerticalBlank,
};

static struct IDirectDraw_VTable xlib_ddvt = {
	Xlib_IDirectDraw_QueryInterface,
	IDirectDraw_AddRef,
	Xlib_IDirectDraw_Release,
	(void*)4,
	IDirectDraw_CreateClipper,
	Xlib_IDirectDraw_CreatePalette,
	Xlib_IDirectDraw_CreateSurface,
	IDirectDraw_DuplicateSurface,
	IDirectDraw_EnumDisplayModes,
	(void*)10,
	IDirectDraw_FlipToGDISurface,
	Xlib_IDirectDraw_GetCaps,
	Xlib_IDirectDraw_GetDisplayMode,
	IDirectDraw_GetFourCCCodes,
	(void*)15,
	IDirectDraw_GetMonitorFrequency,
	(void*)17,
	IDirectDraw_GetVerticalBlankStatus,
	(void*)19,
	Xlib_IDirectDraw_RestoreDisplayMode,
	IDirectDraw_SetCooperativeLevel,
	Xlib_IDirectDraw_SetDisplayMode,
	IDirectDraw_WaitForVerticalBlank,
};

/*****************************************************************************
 * 	IDirectDraw2
 *
 */
static HRESULT WINAPI IDirectDraw2_CreateClipper(
	LPDIRECTDRAW2 this,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
	return IDirectDraw_CreateClipper((LPDIRECTDRAW)this,x,lpddclip,lpunk);
}

static HRESULT WINAPI DGA_IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	return DGA_IDirectDraw_CreateSurface((LPDIRECTDRAW)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf,lpunk);
}

static HRESULT WINAPI XShm_IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	return XShm_IDirectDraw_CreateSurface((LPDIRECTDRAW)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf,lpunk);
}

static HRESULT WINAPI Xlib_IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	return Xlib_IDirectDraw_CreateSurface((LPDIRECTDRAW)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf,lpunk);
}

static HRESULT WINAPI DGA_IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	return DGA_IDirectDraw_QueryInterface((LPDIRECTDRAW)this,refiid,obj);
}

static HRESULT WINAPI XShm_IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	return XShm_IDirectDraw_QueryInterface((LPDIRECTDRAW)this,refiid,obj);
}

static HRESULT WINAPI Xlib_IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	return Xlib_IDirectDraw_QueryInterface((LPDIRECTDRAW)this,refiid,obj);
}

static ULONG WINAPI IDirectDraw2_AddRef(LPDIRECTDRAW2 this) {
	return IDirectDraw_AddRef((LPDIRECTDRAW)this);
}

static ULONG WINAPI DGA_IDirectDraw2_Release(LPDIRECTDRAW2 this) {
	return DGA_IDirectDraw_Release((LPDIRECTDRAW)this);
}

static ULONG WINAPI XShm_IDirectDraw2_Release(LPDIRECTDRAW2 this) {
	return XShm_IDirectDraw_Release((LPDIRECTDRAW)this);
}

static ULONG WINAPI Xlib_IDirectDraw2_Release(LPDIRECTDRAW2 this) {
	return Xlib_IDirectDraw_Release((LPDIRECTDRAW)this);
}

static HRESULT WINAPI DGA_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	return DGA_IDirectDraw_GetCaps((LPDIRECTDRAW)this,caps1,caps2);
}

static HRESULT WINAPI XShm_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	return XShm_IDirectDraw_GetCaps((LPDIRECTDRAW)this,caps1,caps2);
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	return Xlib_IDirectDraw_GetCaps((LPDIRECTDRAW)this,caps1,caps2);
}

static HRESULT WINAPI IDirectDraw2_SetCooperativeLevel(
	LPDIRECTDRAW2 this,HWND32 hwnd,DWORD x
) {
	return IDirectDraw_SetCooperativeLevel((LPDIRECTDRAW)this,hwnd,x);
}

static HRESULT WINAPI DGA_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	return DGA_IDirectDraw_CreatePalette((LPDIRECTDRAW)this,x,palent,lpddpal,lpunk);
}

static HRESULT WINAPI XShm_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	return XShm_IDirectDraw_CreatePalette((LPDIRECTDRAW)this,x,palent,lpddpal,lpunk);
}

static HRESULT WINAPI Xlib_IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	return Xlib_IDirectDraw_CreatePalette((LPDIRECTDRAW)this,x,palent,lpddpal,lpunk);
}

static HRESULT WINAPI DGA_IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return DGA_IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI XShm_IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return XShm_IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI Xlib_IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return Xlib_IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI DGA_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	return DGA_IDirectDraw_RestoreDisplayMode((LPDIRECTDRAW)this);
}

static HRESULT WINAPI XShm_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	return XShm_IDirectDraw_RestoreDisplayMode((LPDIRECTDRAW)this);
}

static HRESULT WINAPI Xlib_IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	return Xlib_IDirectDraw_RestoreDisplayMode((LPDIRECTDRAW)this);
}

static HRESULT WINAPI IDirectDraw2_EnumSurfaces(
	LPDIRECTDRAW2 this,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
	FIXME(ddraw,"(%p)->(0x%08lx,%p,%p,%p),stub!\n",this,x,ddsfd,context,ddsfcb);
	return 0;
}

static HRESULT WINAPI IDirectDraw2_EnumDisplayModes(
	LPDIRECTDRAW2 this,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
	return IDirectDraw_EnumDisplayModes((LPDIRECTDRAW)this,dwFlags,lpddsfd,context,modescb);
}

static HRESULT WINAPI DGA_IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
	return DGA_IDirectDraw_GetDisplayMode((LPDIRECTDRAW)this,lpddsfd);
}

static HRESULT WINAPI XShm_IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
	return XShm_IDirectDraw_GetDisplayMode((LPDIRECTDRAW)this,lpddsfd);
}

static HRESULT WINAPI Xlib_IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
	return Xlib_IDirectDraw_GetDisplayMode((LPDIRECTDRAW)this,lpddsfd);
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

static HRESULT WINAPI IDirectDraw2_GetMonitorFrequency(
	LPDIRECTDRAW2 this,LPDWORD freq
) {
	return IDirectDraw_GetMonitorFrequency((LPDIRECTDRAW)this,freq);
}

static HRESULT WINAPI IDirectDraw2_GetVerticalBlankStatus(
	LPDIRECTDRAW2 this,BOOL32 *status
) {
	return IDirectDraw_GetVerticalBlankStatus((LPDIRECTDRAW)this,status);
}

static HRESULT WINAPI IDirectDraw2_WaitForVerticalBlank(
	LPDIRECTDRAW2 this,DWORD x,HANDLE32 h
) {
	return IDirectDraw_WaitForVerticalBlank((LPDIRECTDRAW)this,x,h);
}

static IDirectDraw2_VTable dga_dd2vt = {
	DGA_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	DGA_IDirectDraw2_Release,
	(void*)4,
	IDirectDraw2_CreateClipper,
	DGA_IDirectDraw2_CreatePalette,
	DGA_IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	(void*)11,
	DGA_IDirectDraw2_GetCaps,
	DGA_IDirectDraw2_GetDisplayMode,
	(void*)14,
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

static IDirectDraw2_VTable xshm_dd2vt = {
	XShm_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	XShm_IDirectDraw2_Release,
	(void*)4,
	IDirectDraw2_CreateClipper,
	XShm_IDirectDraw2_CreatePalette,
	XShm_IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	(void*)11,
	XShm_IDirectDraw2_GetCaps,
	XShm_IDirectDraw2_GetDisplayMode,
	(void*)14,
	(void*)15,
	IDirectDraw2_GetMonitorFrequency,
	(void*)17,
	IDirectDraw2_GetVerticalBlankStatus,
	(void*)19,
	XShm_IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	XShm_IDirectDraw2_SetDisplayMode,
	IDirectDraw2_WaitForVerticalBlank,
	Xlib_IDirectDraw2_GetAvailableVidMem
};

static struct IDirectDraw2_VTable xlib_dd2vt = {
	Xlib_IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	Xlib_IDirectDraw2_Release,
	(void*)4,
	IDirectDraw2_CreateClipper,
	Xlib_IDirectDraw2_CreatePalette,
	Xlib_IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	(void*)11,
	Xlib_IDirectDraw2_GetCaps,
	Xlib_IDirectDraw2_GetDisplayMode,
	(void*)14,
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
	XF86DGAQueryVersion(display,&major,&minor);
	TRACE(ddraw,"XF86DGA is version %d.%d\n",major,minor);
	XF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
		MSG("direct video is NOT ENABLED.\n");
	XF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	TRACE(ddraw,"video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
	);
	(*lplpDD)->e.dga.fb_width = width;
	(*lplpDD)->e.dga.fb_addr = addr;
	(*lplpDD)->e.dga.fb_memsize = memsize;
	(*lplpDD)->e.dga.fb_banksize = banksize;

	XF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	XF86DGASetViewPort(display,DefaultScreen(display),0,0);
	(*lplpDD)->e.dga.fb_height = screenHeight;
	(*lplpDD)->e.dga.vpmask = 0;

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

HRESULT WINAPI XShm_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
#ifdef HAVE_LIBXXSHM
	if (!DDRAW_XShm_Available()) {
		fprintf(stderr,"No XShm detected.\n");
		return DDERR_GENERIC;
	}
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &xshm_ddvt;
	(*lplpDD)->ref = 1;
	(*lplpDD)->e.xshm.drawable = 0; /* FIXME: make a window */
	(*lplpDD)->d.depth = DefaultDepthOfScreen(screen);
	(*lplpDD)->d.height = (*lplpDD)->d.width = 0; /* FIXME */
	return 0;
#else /* defined(HAVE_LIBXXSHM) */
	return DDERR_INVALIDDIRECTDRAWGUID;
#endif /* defined(HAVE_LIBXXSHM) */
}

HRESULT WINAPI Xlib_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &xlib_ddvt;
	(*lplpDD)->ref = 1;
	(*lplpDD)->e.xshm.drawable = 0; /* FIXME: make a window */
	(*lplpDD)->d.depth = DefaultDepthOfScreen(screen);
	(*lplpDD)->d.height = (*lplpDD)->d.width = 0; /* FIXME */
	return 0;
}

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	char	xclsid[50];

	if (lpGUID)
		WINE_StringFromCLSID(lpGUID,xclsid);
	else
		strcpy(xclsid,"<null>");

	TRACE(ddraw,"(%s,%p,%p)\n",xclsid,lplpDD,pUnkOuter);

	if (!lpGUID) {
		/* if they didn't request a particular interface, use the best
		 * supported one */
		if (DDRAW_DGA_Available()) {
			lpGUID = &DGA_DirectDraw_GUID;
		} else if (DDRAW_XShm_Available()) {
			lpGUID = &XSHM_DirectDraw_GUID;
		} else {
			lpGUID = &XLIB_DirectDraw_GUID;
		}
	}

	if (!memcmp(lpGUID, &DGA_DirectDraw_GUID, sizeof(GUID))) {
		return DGA_DirectDrawCreate(lplpDD, pUnkOuter);
	} else if (!memcmp(lpGUID, &XSHM_DirectDraw_GUID, sizeof(GUID))) {
		return XShm_DirectDrawCreate(lplpDD, pUnkOuter);
	} else if (!memcmp(lpGUID, &XLIB_DirectDraw_GUID, sizeof(GUID))) {
		return Xlib_DirectDrawCreate(lplpDD, pUnkOuter);
	}

	fprintf(stderr,"DirectDrawCreate(%s,%p,%p): did not recognize requested GUID\n",xclsid,lplpDD,pUnkOuter);
	return DDERR_INVALIDDIRECTDRAWGUID;
}
