/*		DirectDraw
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
 * - Diablo:
 *   The movies play. The game doesn't yet. No sound. (Needs clone())
 *
 * - WingCommander 4 (not 5!) / Win95 Patch:
 *   The intromovie plays, in 8 bit mode (to reconfigure wc4, run wine
 *   "wc4w.exe -I"). The 16bit mode looks broken, but I think this is due to
 *   my Matrox Mystique which uses 565 (rgb) colorweight instead of the usual
 *   555. Specifying it in DDPIXELFORMAT didn't help.
 *   Requires to be run in 640x480xdepth mode (doesn't seem to heed
 *   DDSURFACEDESC.lPitch).
 *
 * - Monkey Island 3:
 *   Goes to the easy/hard selection screen, then hangs due to not MT safe
 *   XLibs.
 * 
 * - Dark Angel Demo (Some shoot and run game):
 *   The graphics stuff works fine, you can play it.
 *
 * - XvT:
 *   Doesn't work, I am still unsure why not.
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <sys/signal.h>

#include "windows.h"
#include "winerror.h"
#include "interfaces.h"
#include "gdi.h"
#include "heap.h"
#include "ldt.h"
#include "dc.h"
#include "win.h"
#include "debug.h"
#include "stddebug.h"
#include "miscemu.h"
#include "mmsystem.h"
#include "ddraw.h"

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

static HRESULT WINAPI IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE,REFIID,LPVOID*);
static HRESULT WINAPI IDirectDraw_QueryInterface(LPDIRECTDRAW this,REFIID refiid,LPVOID *obj);
static HRESULT WINAPI IDirectDraw2_CreateSurface( LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk);
static HRESULT WINAPI IDirectDraw_CreateSurface( LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk);
static struct IDirectDrawSurface2_VTable dds2vt;
static struct IDirectDrawSurface_VTable ddsvt;


HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	fprintf(stderr,"DirectDrawEnumerateA(%p,%p).\n",ddenumproc,data);
	ddenumproc(0,"WINE Display","display",data);
	ddenumproc((void*)&IID_IDirectDraw,"WINE DirectDraw","directdraw",data);
	ddenumproc((void*)&IID_IDirectDrawPalette,"WINE DirectPalette","directpalette",data);
	return 0;
}

HRESULT WINAPI 
DSoundHelp(DWORD x,DWORD y,DWORD z) {
	fprintf(stderr,"DSoundHelp(0x%08lx,0x%08lx,0x%08lx),stub!\n",x,y,z);
	return 0;
}

#ifdef HAVE_LIBXXF86DGA

static void _dump_DDSCAPS(DWORD flagmask) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDSCAPS_3D)
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
		FE(DDSCAPS_ALLOCONLOAD)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			fprintf(stderr,"%s ",flags[i].name);
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
			fprintf(stderr,"%s ",flags[i].name);
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
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			fprintf(stderr,"%s ",flags[i].name);
}

static int _getpixelformat(LPDIRECTDRAW ddraw,LPDDPIXELFORMAT pf) {
	static XVisualInfo	*vi;
	XVisualInfo		vt;
	int			nitems;

	if (!vi)
		vi = XGetVisualInfo(display,VisualNoMask,&vt,&nitems);

	pf->dwFourCC = mmioFOURCC('R','G','B',' ');
	if (ddraw->d.depth==8) {
		pf->dwFlags 		= DDPF_RGB|DDPF_PALETTEINDEXED8;
		pf->x.dwRGBBitCount	= 8;
		pf->y.dwRBitMask  	= 0;
		pf->z.dwGBitMask  	= 0;
		pf->xx.dwBBitMask 	= 0;
		return 0;
	}
	if (ddraw->d.depth==16) {
		pf->dwFlags       	= DDPF_RGB;
		pf->x.dwRGBBitCount	= 16;
		pf->y.dwRBitMask	= vi[0].red_mask;
		pf->z.dwGBitMask	= vi[0].green_mask;
		pf->xx.dwBBitMask	= vi[0].blue_mask;
		return 0;
	}
	fprintf(stderr,"_getpixelformat:oops?\n");
	return DDERR_GENERIC;
}


static HRESULT WINAPI IDirectDrawSurface_Lock(
    LPDIRECTDRAWSURFACE this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	/*
	fprintf(stderr,"IDirectDrawSurface(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd
	);
	fprintf(stderr,".");
	*/
	if (lprect) {
		/*
		fprintf(stderr,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		 */
		lpddsd->lpSurface =	this->surface+
					(lprect->top*this->lpitch)+
					(lprect->left*(this->ddraw->d.depth/8));
	} else
		lpddsd->lpSurface = this->surface;
	lpddsd->dwWidth		= this->width;
	lpddsd->dwHeight	= this->height;
	lpddsd->lPitch		= this->lpitch;
	_getpixelformat(this->ddraw,&(lpddsd->ddpfPixelFormat));
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_Lock(
    LPDIRECTDRAWSURFACE2 this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	fprintf(stderr,"IDirectDrawSurface2(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd
	);
	/*fprintf(stderr,".");*/
	if (lprect) {
		/*
		fprintf(stderr,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		 */
		lpddsd->lpSurface =	this->surface+
					(lprect->top*this->lpitch)+
					(lprect->left*(this->ddraw->d.depth/8));
	} else
		lpddsd->lpSurface = this->surface;
	lpddsd->dwWidth		= this->width;
	lpddsd->dwHeight	= this->height;
	lpddsd->lPitch		= this->lpitch;
	_getpixelformat(this->ddraw,&(lpddsd->ddpfPixelFormat));
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Unlock(
	LPDIRECTDRAWSURFACE this,LPVOID surface
) {
	dprintf_relay(stderr,"IDirectDrawSurface(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
/*	fprintf(stderr,"IDirectDrawSurface(%p)->Flip(%p,%08lx),STUB\n",this,flipto,dwFlags);*/
	if (!flipto) {
		if (this->backbuffer)
			flipto = this->backbuffer;
		else
			flipto = this;
	}
/*	fprintf(stderr,"f>%ld",flipto->fb_height);*/
	XF86DGASetViewPort(display,DefaultScreen(display),0,flipto->fb_height);
	if (flipto->palette && flipto->palette->cm)
		XF86DGAInstallColormap(display,DefaultScreen(display),flipto->palette->cm);
	while (!XF86DGAViewPortChanged(display,DefaultScreen(display),2)) {
		fprintf(stderr,"w");
	}
	/* is this a good idea ? */
	if (flipto!=this) {
		int	tmp;
		LPVOID	ptmp;

		tmp = this->fb_height;
		this->fb_height = flipto->fb_height;
		flipto->fb_height = tmp;

		ptmp = this->surface;
		this->surface = flipto->surface;
		flipto->surface = ptmp;
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	dprintf_relay(stderr,"IDirectDrawSurface2(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_SetPalette(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWPALETTE pal
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->SetPalette(%p)\n",this,pal);
	this->palette = pal;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_SetPalette(
	LPDIRECTDRAWSURFACE2 this,LPDIRECTDRAWPALETTE pal
) {
	fprintf(stderr,"IDirectDrawSurface2(%p)->SetPalette(%p)\n",this,pal);
	this->palette = pal;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Blt(
	LPDIRECTDRAWSURFACE this,LPRECT32 rdst,LPDIRECTDRAWSURFACE src,LPRECT32 rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->Blt(%p,%p,%p,%08lx,%p),stub!\n",
		this,rdst,src,rsrc,dwFlags,lpbltfx
	);
	if (rdst) fprintf(stderr,"	destrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	if (rsrc) fprintf(stderr,"	srcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	fprintf(stderr,"	blitfx: 0x%08lx\n",lpbltfx->dwDDFX);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_BltFast(
	LPDIRECTDRAWSURFACE this,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE src,LPRECT32 rsrc,DWORD trans
) {
	int	i,bpp;
	fprintf(stderr,"IDirectDrawSurface(%p)->BltFast(%ld,%ld,%p,%p,%08lx),stub!\n",
		this,dstx,dsty,src,rsrc,trans
	);
	fprintf(stderr,"	srcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	bpp = this->ddraw->d.depth/8;
	for (i=0;i<rsrc->bottom-rsrc->top;i++) {
		memcpy(	this->surface+((i+dsty)*this->width*bpp)+dstx*bpp,
			src->surface +(rsrc->top+i)*src->width*bpp+rsrc->left*bpp,
			(rsrc->right-rsrc->left)*bpp
		);
	}
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_BltBatch(
	LPDIRECTDRAWSURFACE this,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		this,ddbltbatch,x,y
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetCaps(
	LPDIRECTDRAWSURFACE this,LPDDSCAPS caps
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetCaps(%p)\n",this,caps);
	caps->dwCaps = DDCAPS_PALETTE; /* probably more */
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE this,LPDDSURFACEDESC ddsd
) { 
	fprintf(stderr,"IDirectDrawSurface(%p)->GetSurfaceDesc(%p)\n",this,ddsd);
	fprintf(stderr,"	flags: ");
	_dump_DDSD(ddsd->dwFlags);
	fprintf(stderr,"\n");

	ddsd->dwFlags |= DDSD_PIXELFORMAT|DDSD_CAPS|DDSD_BACKBUFFERCOUNT|DDSD_HEIGHT|DDSD_WIDTH;
	ddsd->ddsCaps.dwCaps	= DDSCAPS_PALETTE;
	ddsd->dwBackBufferCount	= 1;
	ddsd->dwHeight		= this->height;
	ddsd->dwWidth		= this->width;
	ddsd->lPitch		= this->lpitch;
	if (this->backbuffer)
		ddsd->ddsCaps.dwCaps |= DDSCAPS_PRIMARYSURFACE|DDSCAPS_FLIP;
	_getpixelformat(this->ddraw,&(ddsd->ddpfPixelFormat));
	
	return 0;
}

static ULONG WINAPI IDirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE this) {
	dprintf_relay(stderr,"IDirectDrawSurface(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	dprintf_relay(stderr,"IDirectDrawSurface(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->ddraw->lpvtbl->fnRelease(this->ddraw);
		/* clear out of surface list */
		this->ddraw->d.vpmask &= ~(1<<(this->fb_height/this->ddraw->d.fb_height));
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static ULONG WINAPI IDirectDrawSurface2_AddRef(LPDIRECTDRAWSURFACE2 this) {
	dprintf_relay(stderr,"IDirectDrawSurface2(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	dprintf_relay(stderr,"IDirectDrawSurface2(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->ddraw->lpvtbl->fnRelease(this->ddraw);
		this->ddraw->d.vpmask &= ~(1<<(this->fb_height/this->ddraw->d.fb_height));
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	fprintf(stderr,"IDirectDrawSurface2(%p)->GetAttachedSurface(%p,%p)\n",this,lpddsd,lpdsf);
	fprintf(stderr,"	caps ");_dump_DDSCAPS(lpddsd->dwCaps);fprintf(stderr,"\n");
	if (!(lpddsd->dwCaps & DDSCAPS_BACKBUFFER)) {
		fprintf(stderr,"whoops, can only handle backbuffers for now\n");
		return E_FAIL;
	}
	/* FIXME: should handle more than one backbuffer */
	*lpdsf = this->backbuffer;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetAttachedSurface(
	LPDIRECTDRAWSURFACE this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE *lpdsf
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetAttachedSurface(%p,%p)\n",this,lpddsd,lpdsf);
	fprintf(stderr,"	caps ");_dump_DDSCAPS(lpddsd->dwCaps);fprintf(stderr,"\n");
	if (!(lpddsd->dwCaps & DDSCAPS_BACKBUFFER)) {
		fprintf(stderr,"whoops, can only handle backbuffers for now\n");
		return E_FAIL;
	}
	/* FIXME: should handle more than one backbuffer */
	*lpdsf = this->backbuffer;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Initialize(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->Initialize(%p,%p)\n",
		this,ddraw,lpdsfd
	);
	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface_GetPixelFormat(
	LPDIRECTDRAWSURFACE this,LPDDPIXELFORMAT pf
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetPixelFormat(%p)\n",this,pf);
	return _getpixelformat(this->ddraw,pf);
}

static HRESULT WINAPI IDirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE this,DWORD dwFlags) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetBltStatus(0x%08lx),stub!\n",
		this,dwFlags
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetOverlayPosition(
	LPDIRECTDRAWSURFACE this,LPLONG x1,LPLONG x2
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetOverlayPosition(%p,%p),stub!\n",
		this,x1,x2
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE this,HDC32* lphdc) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetDC(%p),stub!\n",this,lphdc);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE2 this,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
	/* none yet? */
	return 0;
}

static struct IDirectDrawSurface2_VTable dds2vt = {
	(void*)1/*IDirectDrawSurface2_QueryInterface*/,
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
	(void*)25,
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


static HRESULT WINAPI IDirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE this,REFIID refiid,LPVOID *obj) {
        char    xrefiid[50];

        StringFromCLSID((LPCLSID)refiid,xrefiid);
        fprintf(stderr,"IDirectDrawSurface(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);
	
	/* thats version 2 */
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
    	return OLE_E_ENUM_NOMORE;
}

static struct IDirectDrawSurface_VTable ddsvt = {
	IDirectDrawSurface_QueryInterface,
	IDirectDrawSurface_AddRef,
	IDirectDrawSurface_Release,
	(void*)4,
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
	(void*)25,
	IDirectDrawSurface_Lock,
	(void*)27,
	(void*)28,
	(void*)29,
	(void*)30,
	(void*)31,
	IDirectDrawSurface_SetPalette,
	IDirectDrawSurface_Unlock,
	(void*)34,
	(void*)35,
	(void*)36,
};

static HRESULT WINAPI IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	int	i;

	fprintf(stderr,"IDirectDraw(%p)->CreateSurface(%p,%p,%p)\n",this,lpddsd,lpdsf,lpunk);
	fprintf(stderr," [w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
	_dump_DDSD(lpddsd->dwFlags);
	fprintf(stderr,"caps ");_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
	fprintf(stderr,"]\n");

	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &ddsvt;
	for (i=0;i<32;i++)
		if (!(this->d.vpmask & (1<<i)))
			break;
	fprintf(stderr,"using viewport %d for a primary surface\n",i);
	/* if i == 32 or maximum ... return error */
	this->d.vpmask|=(1<<i);
	(*lpdsf)->surface = this->d.fb_addr+((i*this->d.vp_height)*this->d.fb_width*this->d.depth/8);
	(*lpdsf)->fb_height = i*this->d.fb_height;

	(*lpdsf)->width = this->d.width;
	(*lpdsf)->height = this->d.height;
	(*lpdsf)->ddraw = this;
	(*lpdsf)->lpitch = this->d.fb_width*this->d.depth/8;
	(*lpdsf)->backbuffer = NULL;
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE	back;

		if (lpddsd->dwBackBufferCount>1)
			fprintf(stderr,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

		(*lpdsf)->backbuffer = back = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
		this->lpvtbl->fnAddRef(this);
		back->ref = 1;
		back->lpvtbl = &ddsvt;
		for (i=0;i<32;i++)
			if (!(this->d.vpmask & (1<<i)))
				break;
		fprintf(stderr,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		this->d.vpmask|=(1<<i);
		back->surface = this->d.fb_addr+((i*this->d.vp_height)*this->d.fb_width*this->d.depth/8);
		back->fb_height = i*this->d.fb_height;

		back->width = this->d.width;
		back->height = this->d.height;
		back->ddraw = this;
		back->lpitch = this->d.fb_width*this->d.depth/8;
		back->backbuffer = NULL; /* does not have a backbuffer, it is
					  * one! */
	}
	return 0;
}

static HRESULT WINAPI IDirectDraw_DuplicateSurface(
	LPDIRECTDRAW this,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
	fprintf(stderr,"IDirectDraw(%p)->DuplicateSurface(%p,%p)\n",this,src,dst);
	*dst = src; /* FIXME */
	return 0;
}

static HRESULT WINAPI IDirectDraw2_CreateSurface(
	LPDIRECTDRAW2 this,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	int	i;

	fprintf(stderr,"IDirectDraw2(%p)->CreateSurface(%p,%p,%p)\n",this,lpddsd,lpdsf,lpunk);
	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &ddsvt;

	for (i=0;i<32;i++)
		if (!(this->d.vpmask & (1<<i)))
			break;
	fprintf(stderr,"using viewport %d for primary\n",i);
	/* if i == 32 or maximum ... return error */
	this->d.vpmask|=(1<<i);
	(*lpdsf)->surface = this->d.fb_addr+((i*this->d.fb_height)*this->d.fb_width*this->d.depth/8);
	(*lpdsf)->width = this->d.width;
	(*lpdsf)->height = this->d.height;
	(*lpdsf)->ddraw = (LPDIRECTDRAW)this;
	(*lpdsf)->fb_height = i*this->d.fb_height;
	(*lpdsf)->lpitch = this->d.fb_width*this->d.depth/8;
	(*lpdsf)->backbuffer = NULL;
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		LPDIRECTDRAWSURFACE	back;

		if (lpddsd->dwBackBufferCount>1)
			fprintf(stderr,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

		(*lpdsf)->backbuffer = back = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface2));
		this->lpvtbl->fnAddRef(this);
		back->ref = 1;
		back->lpvtbl = &ddsvt;
		for (i=0;i<32;i++)
			if (!(this->d.vpmask & (1<<i)))
				break;
		fprintf(stderr,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		this->d.vpmask|=(1<<i);
		back->surface = this->d.fb_addr+((i*this->d.vp_height)*this->d.fb_width*this->d.depth/8);
		back->fb_height = i*this->d.fb_height;

		back->width = this->d.width;
		back->height = this->d.height;
		back->ddraw = (LPDIRECTDRAW)this;
		back->lpitch = this->d.fb_width*this->d.depth/8;
		back->backbuffer = NULL; /* does not have a backbuffer, it is
					  * one! */
	}
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
	};
	fprintf(stderr,"IDirectDraw(%p)->SetCooperativeLevel(%08lx,%08lx),stub!\n",
		this,(DWORD)hwnd,cooplevel
	);
	fprintf(stderr,"	cooperative level ");
	for (i=0;i<sizeof(flagmap)/sizeof(flagmap[0]);i++)
		if (flagmap[i].mask & cooplevel)
			fprintf(stderr,"%s ",flagmap[i].name);
	fprintf(stderr,"\n");
	this->d.mainwindow = hwnd;
	return 0;
}

extern BOOL32 SIGNAL_InitEmulator(void);

static HRESULT WINAPI IDirectDraw_SetDisplayMode(
	LPDIRECTDRAW this,DWORD width,DWORD height,DWORD depth
) {
	int	i,*depths,depcount;
	char	buf[200];

	fprintf(stderr,"IDirectDraw(%p)->SetDisplayMode(%ld,%ld,%ld),stub!\n",this,width,height,depth);

	depths = XListDepths(display,DefaultScreen(display),&depcount);
	for (i=0;i<depcount;i++)
		if (depths[i]==depth)
			break;
	XFree(depths);
	if (i==depcount) {/* not found */
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	}
	if (this->d.fb_width < width) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld",width,height,depth,width,this->d.fb_width);
		MessageBox32A(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
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
/* FIXME:  can't call this in winelib... so comment only in for debugging.
	SIGNAL_InitEmulator();
 */
	return 0;
}

static HRESULT WINAPI IDirectDraw_GetCaps(
	LPDIRECTDRAW this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	fprintf(stderr,"IDirectDraw(%p)->GetCaps(%p,%p),stub!\n",this,caps1,caps2);
	caps1->dwVidMemTotal = this->d.fb_memsize;
	caps1->dwCaps = 0; /* we cannot do anything */
	caps1->ddsCaps.dwCaps = 0; /* we cannot do anything */
	return 0;
}

static HRESULT WINAPI IDirectDraw2_GetCaps(
	LPDIRECTDRAW2 this,LPDDCAPS caps1,LPDDCAPS caps2
)  {
	fprintf(stderr,"IDirectDraw2(%p)->GetCaps(%p,%p),stub!\n",this,caps1,caps2);
	caps1->dwVidMemTotal = this->d.fb_memsize;
	caps1->dwCaps = 0; /* we cannot do anything */
	caps1->ddsCaps.dwCaps = 0;
	return 0;
}


static struct IDirectDrawClipper_VTable ddclipvt = {
	(void*)1,
	(void*)2,(void*)3,(void*)4,(void*)5,
	(void*)6,
	(void*)7,(void*)8,(void*)9
};

static HRESULT WINAPI IDirectDraw_CreateClipper(
	LPDIRECTDRAW this,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectDraw(%p)->CreateClipper(%08lx,%p,%p),stub!\n",
		this,x,lpddclip,lpunk
	);
	*lpddclip = (LPDIRECTDRAWCLIPPER)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipper));
	(*lpddclip)->ref = 1;
	(*lpddclip)->lpvtbl = &ddclipvt;
	return 0;
}

static HRESULT WINAPI IDirectDrawPalette_GetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD y,DWORD z,LPPALETTEENTRY palent
) {
	fprintf(stderr,"IDirectDrawPalette(%p)->GetEntries(%08lx,%08lx,%08lx,%p),stub!\n",
		this,x,y,z,palent
	);
	return 0;
}

static HRESULT WINAPI IDirectDrawPalette_SetEntries(
	LPDIRECTDRAWPALETTE this,DWORD x,DWORD start,DWORD end,LPPALETTEENTRY palent
) {
	XColor		xc;
	int		i;

/*
	fprintf(stderr,"IDirectDrawPalette(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,end,palent
	);
 */
	if (!this->cm) /* should not happen */ {
		fprintf(stderr,"no colormap in SetEntries???\n");
		return DDERR_GENERIC;
	}
/* FIXME: free colorcells instead of freeing whole map */
	XFreeColormap(display,this->cm);
	this->cm = XCreateColormap(display,DefaultRootWindow(display),DefaultVisual(display,DefaultScreen(display)),AllocAll);
	if (start>0) {
		xc.red = xc.blue = xc.green = 0; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 0; XStoreColor(display,this->cm,&xc);
	}
	if (end<256) {
		xc.red = xc.blue = xc.green = 0xffff; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 255; XStoreColor(display,this->cm,&xc);
	}
	for (i=start;i<end;i++) {
		xc.red = palent[i-start].peRed<<8;
		xc.blue = palent[i-start].peBlue<<8;
		xc.green = palent[i-start].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i;
		XStoreColor(display,this->cm,&xc);
	}
	XF86DGAInstallColormap(display,DefaultScreen(display),this->cm);
	return 0;
}

static ULONG WINAPI IDirectDrawPalette_Release(LPDIRECTDRAWPALETTE this) {
	fprintf(stderr,"IDirectDrawPalette(%p)->Release()\n",this);
	if (!--(this->ref)) {
		fprintf(stderr,"IDirectDrawPalette(%p) freed!\n",this);
		if (this->cm) {
			XFreeColormap(display,this->cm);
			this->cm = 0;
		}
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static ULONG WINAPI IDirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE this) {
	fprintf(stderr,"IDirectDrawPalette(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static HRESULT WINAPI IDirectDrawPalette_Initialize(
	LPDIRECTDRAWPALETTE this,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
	fprintf(stderr,"IDirectDrawPalette(%p)->Initialize(%p,0x%08lx,%p)\n",this,ddraw,x,palent);
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

static HRESULT WINAPI IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectDraw(%p)->CreatePalette(%08lx,%p,%p,%p),stub!\n",
		this,x,palent,lpddpal,lpunk
	);
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPalette));
	(*lpddpal)->ref = 1;
	(*lpddpal)->lpvtbl = &ddpalvt;
	(*lpddpal)->ddraw = this;
	if (this->d.depth<=8) {
		(*lpddpal)->cm = XCreateColormap(display,DefaultRootWindow(display),DefaultVisual(display,DefaultScreen(display)),AllocAll);
		fprintf(stderr,"created colormap...\n");
	} else /* we don't want palettes in hicolor or truecolor */
		(*lpddpal)->cm = 0;

	return 0;
}

static HRESULT WINAPI IDirectDraw2_CreatePalette(
	LPDIRECTDRAW2 this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	return IDirectDraw_CreatePalette((LPDIRECTDRAW)this,x,palent,lpddpal,lpunk);
}

static HRESULT WINAPI IDirectDraw_WaitForVerticalBlank(
	LPDIRECTDRAW this,DWORD x,HANDLE32 h
) {
	fprintf(stderr,"IDirectDraw(%p)->WaitForVerticalBlank(0x%08lx,0x%08x),stub!\n",this,x,h);
	return 0;
}

static ULONG WINAPI IDirectDraw_AddRef(LPDIRECTDRAW this) {
	dprintf_relay(stderr,"IDirectDraw(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDraw_Release(LPDIRECTDRAW this) {
	dprintf_relay(stderr,"IDirectDraw(%p)->Release()\n",this);
	if (!--(this->ref)) {
		fprintf(stderr,"IDirectDraw::Release:freeing IDirectDraw(%p)\n",this);
		XF86DGADirectVideo(display,DefaultScreen(display),0);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
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

static HRESULT WINAPI IDirectDraw2_SetCooperativeLevel(
	LPDIRECTDRAW2 this,HWND32 hwnd,DWORD x
) {
	fprintf(stderr,"IDirectDraw2(%p)->SetCooperativeLevel(%08lx,%08lx),stub!\n",
		this,(DWORD)hwnd,x
	);
	this->d.mainwindow = hwnd;
	return 0;
}

static HRESULT WINAPI IDirectDraw2_SetDisplayMode(
	LPDIRECTDRAW2 this,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	fprintf(stderr,"IDirectDraw2(%p)->SetDisplayMode(%ld,%ld,%ld,%08lx,%08lx),stub!\n",this,width,height,depth,xx,yy);

	return IDirectDraw_SetDisplayMode((LPDIRECTDRAW)this,width,height,depth);
}

static HRESULT WINAPI IDirectDraw2_RestoreDisplayMode(LPDIRECTDRAW2 this) {
	fprintf(stderr,"IDirectDraw2(%p)->RestoreDisplayMode(),stub!\n",this);
	XF86DGADirectVideo(display,DefaultScreen(display),0);
	return 0;
}

static HRESULT WINAPI IDirectDraw_RestoreDisplayMode(LPDIRECTDRAW this) {
	fprintf(stderr,"IDirectDraw(%p)->RestoreDisplayMode(),stub!\n",this);
	XF86DGADirectVideo(display,DefaultScreen(display),0);
	return 0;
}

static HRESULT WINAPI IDirectDraw2_EnumSurfaces(
	LPDIRECTDRAW2 this,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
	fprintf(stderr,"IDirectDraw2(%p)->EnumSurfaces(0x%08lx,%p,%p,%p),stub!\n",this,x,ddsfd,context,ddsfcb);
	return 0;
}

static IDirectDraw2_VTable dd2vt = {
	IDirectDraw2_QueryInterface,
	IDirectDraw2_AddRef,
	IDirectDraw2_Release,
	(void*)4,
	(void*)5/*IDirectDraw_CreateClipper*/,
	IDirectDraw2_CreatePalette,
	IDirectDraw2_CreateSurface,
	(void*)8,
	(void*)9,
	IDirectDraw2_EnumSurfaces,
	(void*)11,
	IDirectDraw2_GetCaps,
	(void*)13,
	(void*)14,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
	(void*)19,
	IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	IDirectDraw2_SetDisplayMode,
	(void*)23/*IDirectDraw_WaitForVerticalBlank*/,
	(void*)24
};

static HRESULT WINAPI IDirectDraw_QueryInterface(
	LPDIRECTDRAW this,REFIID refiid,LPVOID *obj
) {
        char    xrefiid[50];

        StringFromCLSID((LPCLSID)refiid,xrefiid);
        fprintf(stderr,"IDirectDraw(%p)->QueryInterface(%s,%p)\n",this,xrefiid,obj);
        if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
                *obj = this;
		this->lpvtbl->fnAddRef(this);
                return 0;
        }
        if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		/* FIXME FIXME FIXME */
		this->lpvtbl = (LPDIRECTDRAW_VTABLE)&dd2vt;
		this->lpvtbl->fnAddRef(this);
                *obj = this;
                return 0;
        }
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw_GetVerticalBlankStatus(
	LPDIRECTDRAW this,BOOL32 *status
) {
        fprintf(stderr,"IDirectDraw(%p)->GetVerticalBlankSatus(%p)\n",this,status);
	*status = TRUE;
	return 0;
}

static HRESULT WINAPI IDirectDraw_EnumDisplayModes(
	LPDIRECTDRAW this,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
	DDSURFACEDESC	ddsfd;

	fprintf(stderr,"IDirectDraw(%p)->EnumDisplayModes(0x%08lx,%p,%p,%p),stub!\n",this,dwFlags,lpddsfd,context,modescb);
	ddsfd.dwSize = sizeof(ddsfd);
	fprintf(stderr,"size is %d\n",sizeof(ddsfd));
	ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	ddsfd.dwHeight = 480;
	ddsfd.dwWidth = 640;
	ddsfd.lPitch = 640;
	ddsfd.dwBackBufferCount = 1;
	ddsfd.x.dwRefreshRate = 60;
	ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;
	this->d.depth = 8;
	_getpixelformat(this,&(ddsfd.ddpfPixelFormat));
	fprintf(stderr,"modescb returned: 0x%lx\n",(DWORD)modescb(&ddsfd,context));
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw_GetDisplayMode(
	LPDIRECTDRAW this,LPDDSURFACEDESC lpddsfd
) {
	fprintf(stderr,"IDirectDraw(%p)->GetDisplayMode(%p),stub!\n",this,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = this->d.vp_height;
	lpddsfd->dwWidth = this->d.vp_width;
	lpddsfd->lPitch = this->d.fb_width*this->d.depth/8;
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	_getpixelformat(this,&(lpddsfd->ddpfPixelFormat));
	return DD_OK;
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
	(void*)11,
	IDirectDraw_GetCaps,
	IDirectDraw_GetDisplayMode,
	(void*)14,
	(void*)15,
	(void*)16,
	(void*)17,
	IDirectDraw_GetVerticalBlankStatus,
	(void*)19,
	IDirectDraw_RestoreDisplayMode,
	IDirectDraw_SetCooperativeLevel,
	IDirectDraw_SetDisplayMode,
	IDirectDraw_WaitForVerticalBlank,
};


HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {

	char	xclsid[50];
	int	memsize,banksize,width,evbase,evret,major,minor,flags,height;
	char	*addr;
	

	if (lpGUID)
		StringFromCLSID(lpGUID,xclsid);
	else
		strcpy(xclsid,"<null>");

	fprintf(stderr,"DirectDrawCreate(%s,%p,%p)\n",xclsid,lplpDD,pUnkOuter);
	if (getuid()) {
		MessageBox32A(0,"Using the XF86DGA extensions requires the program to be run using UID 0.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return E_UNEXPECTED;
	}
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDraw));
	(*lplpDD)->lpvtbl = &ddvt;
	(*lplpDD)->ref = 1;
	if (!XF86DGAQueryExtension(display,&evbase,&evret)) {
		fprintf(stderr,"No XF86DGA detected.\n");
		return 0;
	}
	XF86DGAQueryVersion(display,&major,&minor);
	fprintf(stderr,"XF86DGA is version %d.%d\n",major,minor);
	XF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
		fprintf(stderr,"direct video is NOT ENABLED.\n");
	XF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	fprintf(stderr,"video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
	);
	(*lplpDD)->d.fb_width = width;
	(*lplpDD)->d.fb_addr = addr;
	(*lplpDD)->d.fb_memsize = memsize;
	(*lplpDD)->d.fb_banksize = banksize;

	XF86DGASetViewPort(display,DefaultScreen(display),0,0);
	XF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	(*lplpDD)->d.vp_width = width;
	(*lplpDD)->d.vp_height = height;
	(*lplpDD)->d.fb_height = height;
	(*lplpDD)->d.vpmask = 0;
	return 0;
}
#else

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	MessageBox32A(0,"WINE DirectDraw needs the XF86DGA extensions compiled in. (libXxf86dga.a).","WINE DirectDraw",MB_OK|MB_ICONSTOP);
	return E_OUTOFMEMORY;
}
#endif
