/*		DirectDraw using DGA
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

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

/* restore signal handlers overwritten by XF86DGA 
 * this is a define, for it will only work in emulator mode
 */
#undef RESTORE_SIGNALS

static struct IDirectDrawSurface3_VTable	dds3vt;
static struct IDirectDrawSurface2_VTable	dds2vt;
static struct IDirectDrawSurface_VTable		ddsvt;
static struct IDirectDraw_VTable		ddvt;
static struct IDirectDraw2_VTable		dd2vt;
static struct IDirect3D_VTable			d3dvt;
static struct IDirect3D2_VTable			d3d2vt;


HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	/* we have just one display driver right now ... */
	ddenumproc(0,"WINE Display","display",data);
	return 0;
}

HRESULT WINAPI 
DSoundHelp(DWORD x,DWORD y,DWORD z) {
	FIXME(ddraw,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x,y,z);
	return 0;
}


#ifdef HAVE_LIBXXF86DGA

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

void _dump_DDCAPS(DWORD flagmask) {
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
		lpddsd->y.lpSurface =	this->s.surface+
					(lprect->top*this->s.lpitch)+
					(lprect->left*(this->s.ddraw->d.depth/8));
	} else
		lpddsd->y.lpSurface = this->s.surface;
	lpddsd->dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_PITCH|DDSD_LPSURFACE;
	lpddsd->dwWidth		= this->s.width;
	lpddsd->dwHeight	= this->s.height;
	lpddsd->lPitch		= this->s.lpitch;
	_getpixelformat(this->s.ddraw,&(lpddsd->ddpfPixelFormat));
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Unlock(
	LPDIRECTDRAWSURFACE this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",this,flipto,dwFlags);
	if (!flipto) {
		if (this->s.backbuffer)
			flipto = this->s.backbuffer;
		else
			flipto = this;
	}
	XF86DGASetViewPort(display,DefaultScreen(display),0,flipto->s.fb_height);
	if (flipto->s.palette && flipto->s.palette->cm)
		XF86DGAInstallColormap(display,DefaultScreen(display),flipto->s.palette->cm);
	while (!XF86DGAViewPortChanged(display,DefaultScreen(display),2)) {
	}
	if (flipto!=this) {
		int	tmp;
		LPVOID	ptmp;

		tmp = this->s.fb_height;
		this->s.fb_height = flipto->s.fb_height;
		flipto->s.fb_height = tmp;

		ptmp = this->s.surface;
		this->s.surface = flipto->s.surface;
		flipto->s.surface = ptmp;
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

	if (rdst)
		memcpy(&xdst,rdst,sizeof(xdst));
	else {
		xdst.top	= 0;
		xdst.bottom	= this->s.height;
		xdst.left	= 0;
		xdst.right	= this->s.width;
	}
	if (rsrc)
		memcpy(&xsrc,rsrc,sizeof(xsrc));
	else if (src) {
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

static ULONG WINAPI IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	TRACE(ddraw,"(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		/* clear out of surface list */
		if (this->s.fb_height == -1) {
			HeapFree(GetProcessHeap(),0,this->s.surface);
		} else {
			this->s.ddraw->d.vpmask &= ~(1<<(this->s.fb_height/this->s.ddraw->d.fb_height));
		}
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

static HRESULT WINAPI IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",this,xrefiid,obj);
	
	/* thats version 3 (DirectX 5) */
	if (	!memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID_IDirectDrawSurface3))) {
		this->lpvtbl->fnAddRef(this);
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dds3vt;
		*obj = this;
		return 0;
	}
	/* thats version 2 (DirectX 3) */
	if (	!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID_IDirectDrawSurface2))) {
		this->lpvtbl->fnAddRef(this);
		this->lpvtbl = (LPDIRECTDRAWSURFACE_VTABLE)&dds2vt;
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

static struct IDirectDrawSurface_VTable ddsvt = {
	IDirectDrawSurface_QueryInterface,
	IDirectDrawSurface_AddRef,
	IDirectDrawSurface_Release,
	IDirectDrawSurface_AddAttachedSurface,
	(void*)5,
	IDirectDrawSurface_Blt,
	IDirectDrawSurface_BltBatch,
	IDirectDrawSurface_BltFast,
	(void*)9,
	(void*)10,
	(void*)11,
	IDirectDrawSurface_Flip,
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
	IDirectDrawSurface_Unlock,
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

static HRESULT WINAPI IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	TRACE(ddraw,"(%p)->Unlock(%p)\n",this,surface);
	return 0;
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

static ULONG WINAPI IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	return IDirectDrawSurface_Release((LPDIRECTDRAWSURFACE)this);
}

static HRESULT WINAPI IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	HRESULT	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret)
		(*lpdsf)->lpvtbl = &dds2vt;
	return ret;
}

static HRESULT WINAPI IDirectDrawSurface2_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE2 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",this,context,esfcb);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_QueryInterface(
	LPDIRECTDRAWSURFACE2 this,REFIID riid,LPVOID *ppobj
) {
	return IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static HRESULT WINAPI IDirectDrawSurface2_IsLost(LPDIRECTDRAWSURFACE2 this) {
	return 0; /* hmm */
}


static struct IDirectDrawSurface2_VTable dds2vt = {
	IDirectDrawSurface2_QueryInterface,
	IDirectDrawSurface2_AddRef,
	IDirectDrawSurface2_Release,
	(void*)4,
	(void*)5,
	(void*)6/*IDirectDrawSurface_Blt*/,
	(void*)7/*IDirectDrawSurface_BltBatch*/,
	(void*)8,
	(void*)9,
	IDirectDrawSurface2_EnumAttachedSurfaces,
	(void*)11,
	(void*)12,
	IDirectDrawSurface2_GetAttachedSurface,
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
	IDirectDrawSurface2_Unlock,
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

static HRESULT WINAPI IDirectDrawSurface3_GetAttachedSurface(
	LPDIRECTDRAWSURFACE3 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE3 *lpdsf
) {
	HRESULT	ret = IDirectDrawSurface_GetAttachedSurface((LPDIRECTDRAWSURFACE)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf);

	if (!ret)
		(*lpdsf)->lpvtbl = &dds3vt;
	return ret;
}

static ULONG WINAPI IDirectDrawSurface3_AddRef(LPDIRECTDRAWSURFACE3 this) {
	TRACE(ddraw,"(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDrawSurface3_Release(LPDIRECTDRAWSURFACE3 this) {
	TRACE(ddraw,"(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->s.ddraw->lpvtbl->fnRelease(this->s.ddraw);
		this->s.ddraw->d.vpmask &= ~(1<<(this->s.fb_height/this->s.ddraw->d.fb_height));
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
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

static HRESULT WINAPI IDirectDrawSurface3_Flip(
	LPDIRECTDRAWSURFACE3 this,LPDIRECTDRAWSURFACE3 flipto,DWORD dwFlags
) {
	return IDirectDrawSurface_Flip((LPDIRECTDRAWSURFACE)this,(LPDIRECTDRAWSURFACE)flipto,dwFlags);
}

static HRESULT WINAPI IDirectDrawSurface3_Lock(
    LPDIRECTDRAWSURFACE3 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	return IDirectDrawSurface_Lock((LPDIRECTDRAWSURFACE)this,lprect,lpddsd,flags,hnd); 
}

static HRESULT WINAPI IDirectDrawSurface3_Unlock(
	LPDIRECTDRAWSURFACE3 this,LPVOID surface
) {
	return IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)this,surface);
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

static HRESULT WINAPI IDirectDrawSurface3_QueryInterface(
	LPDIRECTDRAWSURFACE3 this,REFIID riid,LPVOID *ppobj
) {
	return IDirectDrawSurface_QueryInterface((LPDIRECTDRAWSURFACE)this,riid,ppobj);
}

static struct IDirectDrawSurface3_VTable dds3vt = {
	IDirectDrawSurface3_QueryInterface,
	IDirectDrawSurface3_AddRef,
	IDirectDrawSurface3_Release,
	(void*)4,
	(void*)5,
	IDirectDrawSurface3_Blt,
	(void*)7,
	(void*)8,
	(void*)9,
	IDirectDrawSurface3_EnumAttachedSurfaces,
	(void*)11,
	IDirectDrawSurface3_Flip,
	IDirectDrawSurface3_GetAttachedSurface,
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
	IDirectDrawSurface3_Unlock,
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

static struct IDirectDrawClipper_VTable ddclipvt = {
	(void*)1,
	(void*)2,
	IDirectDrawClipper_Release,
	(void*)4,
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
	int	i;

	FIXME(ddraw,"(%p)->GetEntries(%08lx,%ld,%ld,%p),stub!\n",
		this,x,start,end,palent
	);
	for (i=start;i<end;i++) {
		palent[i-start].peRed = i;
		palent[i-start].peGreen = i;
		palent[i-start].peBlue = i;
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
	XColor		xc;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,end,palent
	);
	if (!this->cm) /* should not happen */ {
		ERR(ddraw,"no colormap in SetEntries???\n");
		return DDERR_GENERIC;
	}
/* FIXME: free colorcells instead of freeing whole map */
	TSXFreeColormap(display,this->cm);
	this->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(screen),AllocAll);
	if (start>0) {
		xc.red = xc.blue = xc.green = 0; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 0; TSXStoreColor(display,this->cm,&xc);
		this->palents[0].peRed = 0;
		this->palents[0].peBlue = 0;
		this->palents[0].peGreen = 0;
	}
	if (end<256) {
		xc.red = xc.blue = xc.green = 0xffff; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 255; TSXStoreColor(display,this->cm,&xc);
		this->palents[255].peRed = 255;
		this->palents[255].peBlue = 255;
		this->palents[255].peGreen = 255;
	}
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

/* Insomnia's (Stea Greene's) Mods Start Here */
/* FIXME: Still should free individual cells, but this fixes loss of */
/*        unchange sections of old palette */

	for (i=0;i<start;i++) {
		xc.red = this->palents[i].peRed<<8;
		xc.blue = this->palents[i].peBlue<<8;
		xc.green = this->palents[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i;
		TSXStoreColor(display,this->cm,&xc);
	}
	for (i=end;i<256;i++) {
		xc.red = this->palents[i].peRed<<8;
		xc.blue = this->palents[i].peBlue<<8;
		xc.green = this->palents[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i;
		TSXStoreColor(display,this->cm,&xc);
	}
/* End Insomnia's Mods */

	XF86DGAInstallColormap(display,DefaultScreen(display),this->cm);
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

static struct IDirectDrawPalette_VTable ddpalvt = {
	(void*)1,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	(void*)4,
	IDirectDrawPalette_GetEntries,
	IDirectDrawPalette_Initialize,
	IDirectDrawPalette_SetEntries
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
static HRESULT WINAPI IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
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
	(*lpdsf)->lpvtbl = &ddsvt;
	if (	(lpddsd->dwFlags & DDSD_CAPS) && 
		(lpddsd->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
	) {
		if (!(lpddsd->dwFlags & DDSD_WIDTH))
			lpddsd->dwWidth = this->d.fb_width;
		if (!(lpddsd->dwFlags & DDSD_HEIGHT))
			lpddsd->dwWidth = this->d.fb_height;
		(*lpdsf)->s.surface = (LPBYTE)HeapAlloc(GetProcessHeap(),0,lpddsd->dwWidth*lpddsd->dwHeight*this->d.depth/8);
		(*lpdsf)->s.fb_height = -1;
		(*lpdsf)->s.lpitch = lpddsd->dwWidth*this->d.depth/8;
		TRACE(ddraw,"using system memory for a primary surface\n");
	} else {
		for (i=0;i<32;i++)
			if (!(this->d.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for a primary surface\n",i);
		/* if i == 32 or maximum ... return error */
		this->d.vpmask|=(1<<i);
		(*lpdsf)->s.surface = this->d.fb_addr+((i*this->d.fb_height)*this->d.fb_width*this->d.depth/8);
		(*lpdsf)->s.fb_height = i*this->d.fb_height;
		(*lpdsf)->s.lpitch = this->d.fb_width*this->d.depth/8;
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
		back->lpvtbl = &ddsvt;
		for (i=0;i<32;i++)
			if (!(this->d.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		this->d.vpmask|=(1<<i);
		back->s.surface = this->d.fb_addr+((i*this->d.fb_height)*this->d.fb_width*this->d.depth/8);
		back->s.fb_height = i*this->d.fb_height;

		back->s.width = this->d.width;
		back->s.height = this->d.height;
		back->s.ddraw = this;
		back->s.lpitch = this->d.fb_width*this->d.depth/8;
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					  * one! */
	}
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


static HRESULT WINAPI IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
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
	if (this->d.fb_width < width) {
		ERR(ddraw,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld\n",width,height,depth,width,this->d.fb_width);
		return DDERR_UNSUPPORTEDMODE;
	}
	this->d.width	= width;
	this->d.height	= height;
	/* adjust fb_height, so we don't overlap */
	if (this->d.fb_height < height)
		this->d.fb_height = height;
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
}

static HRESULT WINAPI IDirectDraw_GetCaps(
	LPDIRECTDRAW this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	TRACE(ddraw,"(%p)->(%p,%p)\n",this,caps1,caps2);
	caps1->dwVidMemTotal = this->d.fb_memsize;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	if (caps2) {
		caps2->dwVidMemTotal = this->d.fb_memsize;
		caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
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

static HRESULT WINAPI IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",
		this,x,palent,lpddpal,lpunk
	);
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	(*lpddpal)->ref = 1;
	(*lpddpal)->lpvtbl = &ddpalvt;
	(*lpddpal)->ddraw = this;
	if (this->d.depth<=8) {
		(*lpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(screen),AllocAll);
	} else /* we don't want palettes in hicolor or truecolor */
		(*lpddpal)->cm = 0;

	return 0;
}

static HRESULT WINAPI IDirectDraw_RestoreDisplayMode(LPDIRECTDRAW this) {
	TRACE(ddraw, "(%p)->()\n", 
		      this);
	Sleep(1000);
	XF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
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

static ULONG WINAPI IDirectDraw_Release(LPDIRECTDRAW this) {
	if (!--(this->ref)) {
		XF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
		SIGNAL_InitEmulator();
#endif
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectDraw_QueryInterface(
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
		this->lpvtbl = (LPDIRECTDRAW_VTABLE)&dd2vt;
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

static HRESULT WINAPI IDirectDraw_GetDisplayMode(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsfd
) {
	TRACE(ddraw,"(%p)->(%p)\n",this,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = screenHeight;
	lpddsfd->dwWidth = screenWidth;
	lpddsfd->lPitch = this->d.fb_width*this->d.depth/8;
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

static IDirectDraw_VTable ddvt = {
	IDirectDraw_QueryInterface,
	IDirectDraw_AddRef,
	IDirectDraw_Release,
	(void*)4,
	IDirectDraw_CreateClipper,
	IDirectDraw_CreatePalette,
	IDirectDraw_CreateSurface,
	IDirectDraw_DuplicateSurface,
	IDirectDraw_EnumDisplayModes,
	(void*)10,
	IDirectDraw_FlipToGDISurface,
	IDirectDraw_GetCaps,
	IDirectDraw_GetDisplayMode,
	(void*)14,
	(void*)15,
	IDirectDraw_GetMonitorFrequency,
	(void*)17,
	IDirectDraw_GetVerticalBlankStatus,
	(void*)19,
	IDirectDraw_RestoreDisplayMode,
	IDirectDraw_SetCooperativeLevel,
	IDirectDraw_SetDisplayMode,
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

static HRESULT WINAPI IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	return IDirectDraw_CreateSurface((LPDIRECTDRAW)this,lpddsd,(LPDIRECTDRAWSURFACE*)lpdsf,lpunk);
}

static HRESULT WINAPI IDirectDraw2_QueryInterface(
	LPDIRECTDRAW2 this,REFIID refiid,LPVOID *obj
) {
	return IDirectDraw_QueryInterface((LPDIRECTDRAW)this,refiid,obj);
}

static ULONG WINAPI IDirectDraw2_AddRef(LPDIRECTDRAW2 this) {
	return IDirectDraw_AddRef((LPDIRECTDRAW)this);
}

static ULONG WINAPI IDirectDraw2_Release(LPDIRECTDRAW2 this) {
	return IDirectDraw_Release((LPDIRECTDRAW)this);
}

static HRESULT WINAPI IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	return IDirectDraw_GetCaps((LPDIRECTDRAW)this,caps1,caps2);
}

static HRESULT WINAPI IDirectDraw2_SetCooperativeLevel(
	LPDIRECTDRAW2 this,HWND32 hwnd,DWORD x
) {
	return IDirectDraw_SetCooperativeLevel((LPDIRECTDRAW)this,hwnd,x);
}

static HRESULT WINAPI IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	return IDirectDraw_CreatePalette((LPDIRECTDRAW)this,x,palent,lpddpal,lpunk);
}


static HRESULT WINAPI IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	TRACE(ddraw,"(%p)->(%ld,%ld,%ld,%08lx,%08lx)\n",
		      this, width, height, depth, xx, yy);

	return IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	return IDirectDraw_RestoreDisplayMode((LPDIRECTDRAW)this);
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

static HRESULT WINAPI IDirectDraw2_GetDisplayMode(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsfd
) {
	return IDirectDraw_GetDisplayMode((LPDIRECTDRAW)this,lpddsfd);
}

static HRESULT WINAPI IDirectDraw2_GetAvailableVidMem(
	LPDIRECTDRAW2 this,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		this,ddscaps,total,free
	);
	if (total) *total = this->d.fb_memsize * 1024;
	if (free) *free = this->d.fb_memsize * 1024;
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

static IDirectDraw2_VTable dd2vt = {
	IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	IDirectDraw2_Release,
	(void*)4,
	IDirectDraw2_CreateClipper,
	IDirectDraw2_CreatePalette,
	IDirectDraw2_CreateSurface,
	(void*)8,
	IDirectDraw2_EnumDisplayModes,
	IDirectDraw2_EnumSurfaces,
	(void*)11,
	IDirectDraw2_GetCaps,
	IDirectDraw2_GetDisplayMode,
	(void*)14,
	(void*)15,
	IDirectDraw2_GetMonitorFrequency,
	(void*)17,
	IDirectDraw2_GetVerticalBlankStatus,
	(void*)19,
	IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	IDirectDraw2_SetDisplayMode,
	IDirectDraw2_WaitForVerticalBlank,
	IDirectDraw2_GetAvailableVidMem
	
};

/******************************************************************************
 * 				DirectDrawCreate
 */

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {

	char	xclsid[50];
	int	memsize,banksize,width,evbase,evret,major,minor,flags,height;
	char	*addr;

	if (lpGUID)
		WINE_StringFromCLSID(lpGUID,xclsid);
	else
		strcpy(xclsid,"<null>");

	TRACE(ddraw,"(%s,%p,%p)\n",xclsid,lplpDD,pUnkOuter);
	if (getuid()) {
		MSG("Must be root to use XF86DGA!\n");
		MessageBox32A(0,"Using the XF86DGA extension requires the program to be run using UID 0.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return E_UNEXPECTED;
	}
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &ddvt;
	(*lplpDD)->ref = 1;
	if (!XF86DGAQueryExtension(display,&evbase,&evret)) {
		MSG("Wine DirectDraw: No XF86DGA detected.\n");
		return 0;
	}
	XF86DGAQueryVersion(display,&major,&minor);
	TRACE(ddraw,"XF86DGA is version %d.%d\n",major,minor);
	XF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
		MSG("direct video is NOT ENABLED.\n");
	XF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	TRACE(ddraw,"video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
	);
	(*lplpDD)->d.fb_width = width;
	(*lplpDD)->d.fb_addr = addr;
	(*lplpDD)->d.fb_memsize = memsize;
	(*lplpDD)->d.fb_banksize = banksize;

	XF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	XF86DGASetViewPort(display,DefaultScreen(display),0,0);
	(*lplpDD)->d.vp_width = width;
	(*lplpDD)->d.vp_height = height;
	(*lplpDD)->d.fb_height = screenHeight;
	(*lplpDD)->d.vpmask = 0;

	/* just assume the default depth is the DGA depth too */
	(*lplpDD)->d.depth = DefaultDepthOfScreen(screen);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitEmulator();
#endif
	return 0;
}
#else

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	MessageBox32A(0,"WINE DirectDraw needs the XF86DGA extensions compiled in. (libXxf86dga.a).","WINE DirectDraw",MB_OK|MB_ICONSTOP);
	return E_OUTOFMEMORY;
}
#endif
