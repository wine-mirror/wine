/*		DirectDraw
 *
 * Copyright 1997 Marcus Meissner
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <X11/Xlib.h>

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
static HRESULT WINAPI IDirectDraw_CreateSurface( LPDIRECTDRAW this,LPDDSURFACEDESC *lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk);
static struct IDirectDrawSurface2_VTable dds2vt;
static struct IDirectDrawSurface_VTable ddsvt;


HRESULT WINAPI
DirectDrawEnumerate32A(LPDDENUMCALLBACK32A ddenumproc,LPVOID data) {
	fprintf(stderr,"DirectDrawEnumerateA(%p,%p).\n",ddenumproc,data);
	ddenumproc(0,"WINE Display","display",data);
	ddenumproc(&IID_IDirectDraw,"WINE DirectDraw","directdraw",data);
	ddenumproc(&IID_IDirectDrawPalette,"WINE DirectPalette","directpalette",data);
	return 0;
}

HRESULT WINAPI 
DSoundHelp(DWORD x,DWORD y,DWORD z) {
	fprintf(stderr,"DSoundHelp(0x%08lx,0x%08lx,0x%08lx),stub!\n",x,y,z);
	return 0;
}

#ifdef HAVE_LIBXXF86DGA

static int _getpixelformat(LPDIRECTDRAW ddraw,LPDDPIXELFORMAT pf) {
	pf->dwFourCC = mmioFOURCC('R','G','B',' ');
	if (ddraw->d.depth==8) {
		pf->dwFlags 		= DDPF_RGB|DDPF_PALETTEINDEXEDTO8;
		pf->x.dwRGBBitCount	= 8;
		pf->y.dwRBitMask  	= 0;
		pf->z.dwGBitMask  	= 0;
		pf->xx.dwBBitMask 	= 0;
		return 0;
	}
	if (ddraw->d.depth==16) {
		pf->dwFlags       = DDPF_RGB;
		pf->x.dwRGBBitCount  = 16;
		pf->y.dwRBitMask  = 0x0000f800;
		pf->z.dwGBitMask  = 0x000007e0;
		pf->xx.dwBBitMask = 0x0000001f;
		return 0;
	}
	fprintf(stderr,"_getpixelformat:oops?\n");
	return DDERR_GENERIC;
}


static HRESULT WINAPI IDirectDrawSurface_Lock(
    LPDIRECTDRAWSURFACE this,LPRECT32 lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE32 hnd
) {
	dprintf_relay(stddeb,"IDirectDrawSurface(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd
	);
	fprintf(stderr,".");
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
	dprintf_relay(stddeb,"IDirectDrawSurface2(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		this,lprect,lpddsd,flags,(DWORD)hnd
	);
	fprintf(stderr,".");
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
	dprintf_relay(stddeb,"IDirectDrawSurface(%p)->Unlock(%p)\n",this,surface);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Flip(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAWSURFACE flipto,DWORD dwFlags
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->Flip(%p,%08lx),STUB\n",this,flipto,dwFlags);
	if (flipto) {
		XF86DGASetViewPort(display,DefaultScreen(display),0,flipto->fb_height);
	} else {
		/* FIXME: flip through attached surfaces */
		XF86DGASetViewPort(display,DefaultScreen(display),0,this->fb_height);
	}
	while (!XF86DGAViewPortChanged(display,DefaultScreen(display),1)) {
	} 
	fprintf(stderr,"flipped to new height %ld\n",flipto->fb_height);
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface2_Unlock(
	LPDIRECTDRAWSURFACE2 this,LPVOID surface
) {
	dprintf_relay(stddeb,"IDirectDrawSurface2(%p)->Unlock(%p)\n",this,surface);
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
	fprintf(stderr,"IDirectDrawSurface(%p)->GetCaps(%p),stub!\n",this,caps);
	caps->dwCaps = 0; /* we cannot do anything */
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE this,LPDDSURFACEDESC ddsd
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->GetSurfaceDesc(%p)\n",this,ddsd);
	if (ddsd->dwFlags & DDSD_CAPS)
		ddsd->ddsCaps.dwCaps = 0;
	if (ddsd->dwFlags & DDSD_BACKBUFFERCOUNT)
		ddsd->dwBackBufferCount = 1;
	if (ddsd->dwFlags & DDSD_HEIGHT)
		ddsd->dwHeight = this->height;
	if (ddsd->dwFlags & DDSD_WIDTH)
		ddsd->dwHeight = this->width;
	ddsd->dwFlags &= ~(DDSD_CAPS|DDSD_BACKBUFFERCOUNT|DDSD_HEIGHT|DDSD_WIDTH);
	if (ddsd->dwFlags)
		fprintf(stderr,"	ddsd->flags is 0x%08lx\n",ddsd->dwFlags);
	return 0;
}

static ULONG WINAPI IDirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE this) {
	dprintf_relay(stddeb,"IDirectDrawSurface(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDrawSurface_Release(LPDIRECTDRAWSURFACE this) {
	dprintf_relay(stddeb,"IDirectDrawSurface(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->ddraw->lpvtbl->fnRelease(this->ddraw);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static ULONG WINAPI IDirectDrawSurface2_AddRef(LPDIRECTDRAWSURFACE2 this) {
	dprintf_relay(stddeb,"IDirectDrawSurface2(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDrawSurface2_Release(LPDIRECTDRAWSURFACE2 this) {
	dprintf_relay(stddeb,"IDirectDrawSurface2(%p)->Release()\n",this);
	if (!--(this->ref)) {
		this->ddraw->lpvtbl->fnRelease(this->ddraw);
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectDrawSurface2_GetAttachedSurface(
	LPDIRECTDRAWSURFACE2 this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE2 *lpdsf
) {
	DDSURFACEDESC	ddsfd;
	IUnknown	unk;

	/* DOES NOT CREATE THEM, but uses the ones already attached to this
	 * surface 
	 */
	fprintf(stderr,"IDirectDrawSurface2(%p)->GetAttachedSurface(%p,%p)\n",this,lpddsd,lpdsf);
	/* FIXME: not correct */
	IDirectDraw2_CreateSurface((LPDIRECTDRAW2)this->ddraw,&ddsfd,(LPDIRECTDRAWSURFACE*)lpdsf,&unk);

	lpddsd->dwCaps = 0;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_GetAttachedSurface(
	LPDIRECTDRAWSURFACE this,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE *lpdsf
) {
	LPDDSURFACEDESC	lpddsfd;
	IUnknown	unk;

	fprintf(stderr,"IDirectDrawSurface(%p)->GetAttachedSurface(%p,%p)\n",this,lpddsd,lpdsf);
	/* FIXME: not correct */
	IDirectDraw_CreateSurface(this->ddraw,&lpddsfd,lpdsf,&unk);
	lpddsd->dwCaps = 0;
	return 0;
}

static HRESULT WINAPI IDirectDrawSurface_Initialize(
	LPDIRECTDRAWSURFACE this,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
	fprintf(stderr,"IDirectDrawSurface(%p)->Initialize(%p,%p)\n",
		this,ddraw,lpdsfd
	);
	fprintf(stderr,"	dwFlags is %08lx\n",lpdsfd->dwFlags);
	return 0;
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
	1/*IDirectDrawSurface2_QueryInterface*/,
	IDirectDrawSurface2_AddRef,
	IDirectDrawSurface2_Release,
	4,
	5,
	6/*IDirectDrawSurface_Blt*/,
	7/*IDirectDrawSurface_BltBatch*/,
	8,
	9,
	IDirectDrawSurface2_EnumAttachedSurfaces,
	11,
	12,
	IDirectDrawSurface2_GetAttachedSurface,
	14,
	15/*IDirectDrawSurface_GetCaps*/,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23/*IDirectDrawSurface_GetSurfaceDesc*/,
	24,
	25,
	IDirectDrawSurface2_Lock,
	27,
	28,
	29,
	30,
	31,
	IDirectDrawSurface2_SetPalette,
	IDirectDrawSurface2_Unlock,
	34,
	35,
	36,
	37,
	38,
	39,
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
	4,
	5,
	IDirectDrawSurface_Blt,
	IDirectDrawSurface_BltBatch,
	IDirectDrawSurface_BltFast,
	9,
	10,
	11,
	IDirectDrawSurface_Flip,
	IDirectDrawSurface_GetAttachedSurface,
	IDirectDrawSurface_GetBltStatus,
	IDirectDrawSurface_GetCaps,
	16,
	17,
	IDirectDrawSurface_GetDC,
	19,
	IDirectDrawSurface_GetOverlayPosition,
	21,
	IDirectDrawSurface_GetPixelFormat,
	IDirectDrawSurface_GetSurfaceDesc,
	IDirectDrawSurface_Initialize,
	25,
	IDirectDrawSurface_Lock,
	27,
	28,
	29,
	30,
	31,
	IDirectDrawSurface_SetPalette,
	IDirectDrawSurface_Unlock,
	34,
	35,
	36,
};

static HRESULT WINAPI IDirectDraw_CreateSurface(
	LPDIRECTDRAW this,LPDDSURFACEDESC *lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
	fprintf(stderr,"IDirectDraw(%p)->CreateSurface(%p,%p,%p)\n",this,lpddsd,lpdsf,lpunk);
	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &ddsvt;
	(*lpdsf)->surface = this->d.fb_addr+(this->d.current_height*this->d.fb_width*this->d.depth/8);
	(*lpdsf)->fb_height = this->d.current_height; /* for setviewport */
	this->d.current_height += this->d.fb_height;
	(*lpdsf)->width = this->d.width;
	(*lpdsf)->height = this->d.height;
	(*lpdsf)->ddraw = this;
	(*lpdsf)->lpitch = this->d.fb_width*this->d.depth/8;
	*lpddsd = (LPDDSURFACEDESC)HeapAlloc(GetProcessHeap(),0,sizeof(DDSURFACEDESC));
	(*lpddsd)->dwWidth = this->d.width;
	(*lpddsd)->dwHeight = this->d.height;
	(*lpddsd)->lPitch = this->d.fb_width*this->d.depth/8;
	(*lpddsd)->ddsCaps.dwCaps = 0;
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
	fprintf(stderr,"IDirectDraw2(%p)->CreateSurface(%p,%p,%p)\n",this,lpddsd,lpdsf,lpunk);
	*lpdsf = (LPDIRECTDRAWSURFACE)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectDrawSurface));
	this->lpvtbl->fnAddRef(this);
	(*lpdsf)->ref = 1;
	(*lpdsf)->lpvtbl = &ddsvt;
	(*lpdsf)->surface = this->d.fb_addr+(this->d.current_height*this->d.fb_width*this->d.depth/8);
	(*lpdsf)->width = this->d.width;
	(*lpdsf)->height = this->d.height;
	(*lpdsf)->ddraw = (LPDIRECTDRAW)this;
	(*lpdsf)->fb_height = this->d.current_height;
	(*lpdsf)->lpitch = this->d.fb_width*this->d.depth/8;
	this->d.current_height += this->d.fb_height;
	lpddsd->dwWidth = this->d.width;
	lpddsd->dwHeight = this->d.height;
	lpddsd->lPitch = this->d.fb_width*this->d.depth/8;
	lpddsd->ddsCaps.dwCaps = 0;
	return 0;
}

static HRESULT WINAPI IDirectDraw_SetCooperativeLevel(
	LPDIRECTDRAW this,HWND32 hwnd,DWORD x
) {
	fprintf(stderr,"IDirectDraw(%p)->SetCooperativeLevel(%08lx,%08lx),stub!\n",
		this,(DWORD)hwnd,x
	);
	this->d.mainwindow = hwnd;
	return 0;
}

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
	XF86DGADirectVideo(display,DefaultScreen(display),XF86DGADirectGraphics);
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
	1,2,3,4,5,6,0x10007,8,9
};

static HRESULT WINAPI IDirectDraw_CreateClipper(
	LPDIRECTDRAW this,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectDraw(%p)->CreateClipper(%08lx,%p,%p),stub!\n",
		this,x,lpddclip,lpunk
	);
	*lpddclip = (LPDIRECTDRAWCLIPPER)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectDrawClipper));
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

	fprintf(stderr,"IDirectDrawPalette(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		this,x,start,end,palent
	);
	if (!this->cm) /* should not happen */ {
		fprintf(stderr,"no colormap in SetEntries???\n");
		return DDERR_GENERIC;
	}
	XFreeColormap(display,this->cm);
	this->cm = XCreateColormap(display,DefaultRootWindow(display),DefaultVisual(display,DefaultScreen(display)),AllocAll);
	xc.red = xc.blue = xc.green = 0; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 0; XStoreColor(display,this->cm,&xc);
	xc.red = xc.blue = xc.green = 0xffff; xc.flags = DoRed|DoGreen|DoBlue; xc.pixel = 255; XStoreColor(display,this->cm,&xc);
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

static struct IDirectDrawPalette_VTable ddpalvt = {
	1,
	IDirectDrawPalette_AddRef,
	IDirectDrawPalette_Release,
	4,
	IDirectDrawPalette_GetEntries,
	6,
	IDirectDrawPalette_SetEntries
};

static HRESULT WINAPI IDirectDraw_CreatePalette(
	LPDIRECTDRAW this,DWORD x,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectDraw(%p)->CreatePalette(%08lx,%p,%p,%p),stub!\n",
		this,x,palent,lpddpal,lpunk
	);
	*lpddpal = (LPDIRECTDRAWPALETTE)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectDrawPalette));
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
	dprintf_relay(stddeb,"IDirectDraw(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectDraw_Release(LPDIRECTDRAW this) {
	dprintf_relay(stddeb,"IDirectDraw(%p)->Release()\n",this);
	if (!--(this->ref)) {
		fprintf(stderr,"IDirectDraw::Release:freeing IDirectDraw(%p)\n",this);
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
	4,
	5/*IDirectDraw_CreateClipper*/,
	IDirectDraw2_CreatePalette,
	IDirectDraw2_CreateSurface,
	8,
	9,
	IDirectDraw2_EnumSurfaces,
	11,
	IDirectDraw2_GetCaps,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	IDirectDraw2_RestoreDisplayMode,
	IDirectDraw2_SetCooperativeLevel,
	IDirectDraw2_SetDisplayMode,
	23/*IDirectDraw_WaitForVerticalBlank*/,
	24
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
	ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_CAPS|DDSD_BACKBUFFERCOUNT|DDSD_REFRESHRATE;
	ddsfd.dwHeight = 480;
	ddsfd.dwWidth = 640;
	ddsfd.lPitch = 640;
	ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE|DDSCAPS_FRONTBUFFER|DDSCAPS_BACKBUFFER|DDSCAPS_FLIP|DDSCAPS_PRIMARYSURFACE|DDSCAPS_VIDEOMEMORY|DDSCAPS_ZBUFFER;
	ddsfd.dwBackBufferCount = 1;
	ddsfd.x.dwRefreshRate = 60;
	_getpixelformat(this,&(ddsfd.ddpfPixelFormat));
	fprintf(stderr,"modescb returned: 0x%lx\n",(DWORD)modescb(&ddsfd,context));
	return 0;
}


static IDirectDraw_VTable ddvt = {
	IDirectDraw_QueryInterface,
	IDirectDraw_AddRef,
	IDirectDraw_Release,
	4,
	IDirectDraw_CreateClipper,
	IDirectDraw_CreatePalette,
	IDirectDraw_CreateSurface,
	IDirectDraw_DuplicateSurface,
	IDirectDraw_EnumDisplayModes,
	10,
	11,
	IDirectDraw_GetCaps,
	13,
	14,
	15,
	16,
	17,
	IDirectDraw_GetVerticalBlankStatus,
	19,
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
	*lplpDD = (LPDIRECTDRAW)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectDraw));
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
	while (!XF86DGAViewPortChanged(display,DefaultScreen(display),1)) {
		fprintf(stderr,".");
	}
	XF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	(*lplpDD)->d.vp_width = width;
	(*lplpDD)->d.vp_height = height;
	(*lplpDD)->d.fb_height = height; /* FIXME: can we find out the virtual
	 				* size somehow else ?
					*/
	(*lplpDD)->d.current_height = 0;
	return 0;
}
#else

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
	MessageBox32A(0,"WINE DirectDraw needs the XF86DGA extensions compiled in. (libXxf86dga.a).","WINE DirectDraw",MB_OK|MB_ICONSTOP);
	return E_OUTOFMEMORY;
}
#endif
