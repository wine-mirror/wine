/*		DirectDraw IDirectDraw interface (generic)
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 1998-2000 Lionel Ulmer (most of Direct3D stuff)
 */

/*
 * This file contains all the interface functions that are shared between
 * all interfaces. Or better, it is a "common stub" library for the IDirectDraw*
 * objects
 */

#include "config.h"

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include "winerror.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

#include "ddraw_private.h"

HRESULT WINAPI IDirectDraw2Impl_DuplicateSurface(
    LPDIRECTDRAW2 iface,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(%p,%p) simply copies\n",This,src,dst);
    *dst = src; /* FIXME */
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_SetCooperativeLevel(
    LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
) {
    ICOM_THIS(IDirectDraw2Impl,iface);

    FIXME("(%p)->(%08lx,%08lx)\n",This,(DWORD)hwnd,cooplevel);
    _dump_cooperativelevel(cooplevel);
    This->d.mainWindow = hwnd;
    return DD_OK;
}

/*
 * Small helper to either use the cooperative window or create a new 
 * one (for mouse and keyboard input) and drawing in the Xlib implementation.
 * 
 * Note this just uses USER calls, so it is safe in here.
 */
void _common_IDirectDrawImpl_SetDisplayMode(IDirectDrawImpl* This) {
    RECT	rect;

    /* Do destroy only our window */
    if (This->d.window && GetPropA(This->d.window,ddProp)) {
	DestroyWindow(This->d.window);
	This->d.window = 0;
    }
    /* Sanity check cooperative window before assigning it to drawing. */
    if (IsWindow(This->d.mainWindow) &&
	IsWindowVisible(This->d.mainWindow)
    ) {
	GetWindowRect(This->d.mainWindow,&rect);
	if ((((rect.right-rect.left) >= This->d.width)	&&
	     ((rect.bottom-rect.top) >= This->d.height))
	) {
	    This->d.window = This->d.mainWindow;
	    /* FIXME: resizing is not windows compatible behaviour, need test */
	    /* SetWindowPos(This->d.mainWindow,HWND_TOPMOST,0,0,This->d.width,This->d.height,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER); */
	    This->d.paintable = 1; /* don't wait for WM_PAINT */
	}
    }
    /* ... failed, create new one. */
    if (!This->d.window) {
	This->d.window = CreateWindowExA(
	    0,
	    "WINE_DirectDraw",
	    "WINE_DirectDraw",
	    WS_POPUP,
	    0,0,
	    This->d.width,
	    This->d.height,
	    0,
	    0,
	    0,
	    NULL
	);
	/*Store THIS with the window. We'll use it in the window procedure*/
	SetPropA(This->d.window,ddProp,(LONG)This);
	ShowWindow(This->d.window,TRUE);
	UpdateWindow(This->d.window);
    }
    SetFocus(This->d.window);
}

HRESULT WINAPI IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
        ICOM_THIS(IDirectDrawImpl,iface);

	FIXME("(%p)->SetDisplayMode(%ld,%ld,%ld), needs to be implemented for your display adapter!\n",This,width,height,depth);
	This->d.width	= width;
	This->d.height	= height;
	_common_IDirectDrawImpl_SetDisplayMode(This);
	return DD_OK;
}

static void fill_caps(LPDDCAPS caps) {
    /* This function tries to fill the capabilities of Wines DDraw
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
     * memory to put textures in video memory. BTW, is this only frame buffer
     * memory or also texture memory (for Voodoo boards for example) ?
     */
    caps->dwVidMemTotal = 8192 * 1024;
    caps->dwVidMemFree = 8192 * 1024;
    /* These are all the supported capabilities of the surfaces */
    caps->ddsCaps.dwCaps = DDSCAPS_ALPHA | DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP |
    DDSCAPS_FRONTBUFFER | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_OFFSCREENPLAIN |
      /*DDSCAPS_OVERLAY |*/ DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
	DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;
}

HRESULT WINAPI IDirectDraw2Impl_GetCaps(
    LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
)  {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);

    /* Put the same caps for the two capabilities */
    fill_caps(caps1);
    fill_caps(caps2);

    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_CreateClipper(
    LPDIRECTDRAW2 iface,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawClipperImpl** ilpddclip=(IDirectDrawClipperImpl**)lpddclip;
    FIXME("(%p)->(%08lx,%p,%p),stub!\n", This,x,ilpddclip,lpunk);
    *ilpddclip = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
    (*ilpddclip)->ref = 1;
    ICOM_VTBL(*ilpddclip) = &ddclipvt;
    return DD_OK;
}

HRESULT WINAPI common_IDirectDraw2Impl_CreatePalette(
    IDirectDraw2Impl* This,DWORD dwFlags,LPPALETTEENTRY palent,
    IDirectDrawPaletteImpl **lpddpal,LPUNKNOWN lpunk,int *psize
) {
    int size = 0;
	  
    if (TRACE_ON(ddraw))
	_dump_paletteformat(dwFlags);
	
    *lpddpal = (IDirectDrawPaletteImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPaletteImpl));
    if (*lpddpal == NULL) return E_OUTOFMEMORY;
    (*lpddpal)->ref = 1;
    (*lpddpal)->ddraw = (IDirectDrawImpl*)This;

    if (dwFlags & DDPCAPS_1BIT)
	size = 2;
    else if (dwFlags & DDPCAPS_2BIT)
	size = 4;
    else if (dwFlags & DDPCAPS_4BIT)
	size = 16;
    else if (dwFlags & DDPCAPS_8BIT)
	size = 256;
    else
	ERR("unhandled palette format\n");

    *psize = size;
    if (This->d.palette_convert == NULL) {
	/* No depth conversion - create 8<->8 identity map */
	int ent;
	for (ent=0; ent<256; ent++)
	    (*lpddpal)->screen_palents[ent] = ent;
    }
    if (palent) {
	/* Now, if we are in depth conversion mode, create the screen palette */
	if (This->d.palette_convert != NULL)	    
	    This->d.palette_convert(palent,(*lpddpal)->screen_palents,0,size);

	memcpy((*lpddpal)->palents, palent, size * sizeof(PALETTEENTRY));
    } else if (This->d.palette_convert != NULL) {
	/* In that case, put all 0xFF */
	memset((*lpddpal)->screen_palents, 0xFF, 256 * sizeof(int));
    }
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_CreatePalette(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
    int xsize;
    HRESULT res;

    TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
    res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
    if (res != 0) return res;
    ICOM_VTBL(*ilpddpal) = &ddraw_ddpalvt;
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->RestoreDisplayMode()\n", This);
    Sleep(1000);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_WaitForVerticalBlank(
    LPDIRECTDRAW2 iface,DWORD x,HANDLE h
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(flags=0x%08lx,handle=0x%08x)\n",This,x,h);
    return DD_OK;
}

ULONG WINAPI IDirectDraw2Impl_AddRef(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

    return ++(This->ref);
}

ULONG WINAPI IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (!--(This->ref)) {
	if (This->d.window && GetPropA(This->d.window,ddProp))
	    DestroyWindow(This->d.window);
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
    }
    return This->ref;
}

HRESULT WINAPI IDirectDraw2Impl_QueryInterface(
    LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj
) {
    ICOM_THIS(IDirectDraw2Impl,iface);

    if ( IsEqualGUID( &IID_IUnknown, refiid ) ) {
	*obj = This;
	IDirectDraw2_AddRef(iface);
	TRACE("  Creating IUnknown interface (%p)\n", *obj);
	return S_OK;
    }
    ERR("(%p)->(%s,%p), must be implemented by display interface!\n",This,debugstr_guid(refiid),obj);
    return OLE_E_ENUM_NOMORE;
}

HRESULT WINAPI IDirectDraw2Impl_GetVerticalBlankStatus(
    LPDIRECTDRAW2 iface,BOOL *status
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->(%p)\n",This,status);
    *status = TRUE;
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_EnumDisplayModes(
    LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,
    LPDDENUMMODESCALLBACK modescb
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
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

HRESULT WINAPI IDirectDraw2Impl_GetDisplayMode(
    LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->GetDisplayMode(%p)\n",This,lpddsfd);
    lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
    lpddsfd->dwHeight = This->d.height;
    lpddsfd->dwWidth = This->d.width;
    lpddsfd->lPitch =lpddsfd->dwWidth*PFGET_BPP(This->d.directdraw_pixelformat);
    lpddsfd->dwBackBufferCount = 2;
    lpddsfd->u.dwRefreshRate = 60;
    lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
    lpddsfd->ddpfPixelFormat = This->d.directdraw_pixelformat;
    if (TRACE_ON(ddraw))
	_dump_surface_desc(lpddsfd);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_FlipToGDISurface(LPDIRECTDRAW2 iface) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->()\n",This);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_GetMonitorFrequency(
    LPDIRECTDRAW2 iface,LPDWORD freq
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(%p) returns 60 Hz always\n",This,freq);
    *freq = 60*100; /* 60 Hz */
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_GetFourCCCodes(
    LPDIRECTDRAW2 iface,LPDWORD x,LPDWORD y
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p,%p,%p), stub\n",This,x,y);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_EnumSurfaces(
    LPDIRECTDRAW2 iface,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,
    LPDDENUMSURFACESCALLBACK ddsfcb
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(0x%08lx,%p,%p,%p),stub!\n",This,x,ddsfd,context,ddsfcb);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_Compact( LPDIRECTDRAW2 iface ) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->()\n", This );
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_GetGDISurface(
    LPDIRECTDRAW2 iface, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(%p)\n", This, lplpGDIDDSSurface);
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_GetScanLine(
    LPDIRECTDRAW2 iface, LPDWORD lpdwScanLine
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(%p)\n", This, lpdwScanLine);

    if (lpdwScanLine)
	*lpdwScanLine = 0;
    return DD_OK;
}

HRESULT WINAPI IDirectDraw2Impl_Initialize(LPDIRECTDRAW2 iface, GUID *lpGUID) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    FIXME("(%p)->(%p)\n", This, lpGUID);
    return DD_OK;
}

/*****************************************************************************
 * 	IDirectDraw2
 *
 * We only need to list the changed/new functions.
 */
HRESULT WINAPI IDirectDraw2Impl_SetDisplayMode(
    LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,
    DWORD dwRefreshRate, DWORD dwFlags
) {
    FIXME("Ignoring parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
    return IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

HRESULT WINAPI IDirectDraw2Impl_GetAvailableVidMem(
    LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    TRACE("(%p)->(%p,%p,%p)\n", This,ddscaps,total,free);

    /* We have 16 MB videomemory */
    if (total)	*total= 16*1024*1024;
    if (free)	*free = 16*1024*1024;
    return DD_OK;
}

/*****************************************************************************
 * 	IDirectDraw4
 *
 */

HRESULT WINAPI IDirectDraw4Impl_GetSurfaceFromDC(
    LPDIRECTDRAW4 iface, HDC hdc, LPDIRECTDRAWSURFACE *lpDDS
) {
    ICOM_THIS(IDirectDraw4Impl,iface);
    FIXME("(%p)->(%08ld,%p)\n", This, (DWORD) hdc, lpDDS);

    return DD_OK;
}

HRESULT WINAPI IDirectDraw4Impl_RestoreAllSurfaces(LPDIRECTDRAW4 iface) {
    ICOM_THIS(IDirectDraw4Impl,iface);
    FIXME("(%p)->()\n", This);

    return DD_OK;
}

HRESULT WINAPI IDirectDraw4Impl_TestCooperativeLevel(LPDIRECTDRAW4 iface) {
    ICOM_THIS(IDirectDraw4Impl,iface);
    FIXME("(%p)->()\n", This);

    return DD_OK;
}

HRESULT WINAPI IDirectDraw4Impl_GetDeviceIdentifier(
    LPDIRECTDRAW4 iface,LPDDDEVICEIDENTIFIER lpdddi,DWORD dwFlags
) {
    ICOM_THIS(IDirectDraw4Impl,iface);
    FIXME("(%p)->(%p,%08lx)\n", This, lpdddi, dwFlags);

    /* just guessing values */
    strcpy(lpdddi->szDriver,"directdraw");
    strcpy(lpdddi->szDescription,"WINE DirectDraw");
    lpdddi->liDriverVersion.s.HighPart = 7;
    lpdddi->liDriverVersion.s.LowPart = 0;
    /* Do I smell PCI ids here ? -MM */
    lpdddi->dwVendorId			= 0;
    lpdddi->dwDeviceId			= 0;
    lpdddi->dwSubSysId			= 0;
    lpdddi->dwRevision			= 1;
    memset(&(lpdddi->guidDeviceIdentifier),0,sizeof(lpdddi->guidDeviceIdentifier));
    return DD_OK;
}

HRESULT common_off_screen_CreateSurface(
    IDirectDraw2Impl* This,IDirectDrawSurfaceImpl* lpdsf
) {
    int bpp;

    /* The surface was already allocated when entering in this function */
    TRACE("using system memory for a surface (%p) \n", lpdsf);

    if (lpdsf->s.surface_desc.dwFlags & DDSD_ZBUFFERBITDEPTH) {
	/* This is a Z Buffer */
	TRACE("Creating Z-Buffer of %ld bit depth\n", lpdsf->s.surface_desc.u.dwZBufferBitDepth);
	bpp = lpdsf->s.surface_desc.u.dwZBufferBitDepth / 8;
    } else {
	/* This is a standard image */
	if (!(lpdsf->s.surface_desc.dwFlags & DDSD_PIXELFORMAT)) {
	    /* No pixel format => use DirectDraw's format */
	    lpdsf->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;
	    lpdsf->s.surface_desc.dwFlags |= DDSD_PIXELFORMAT;
	}
	bpp = GET_BPP(lpdsf->s.surface_desc);
    }

    if (lpdsf->s.surface_desc.dwFlags & DDSD_LPSURFACE)
	ERR("Creates a surface that is already allocated : assuming this is an application bug !\n");

    lpdsf->s.surface_desc.dwFlags |= DDSD_PITCH|DDSD_LPSURFACE;
    lpdsf->s.surface_desc.u1.lpSurface =(LPBYTE)VirtualAlloc(
	NULL,
	lpdsf->s.surface_desc.dwWidth * lpdsf->s.surface_desc.dwHeight * bpp,
	MEM_RESERVE | MEM_COMMIT,
	PAGE_READWRITE
    );
    lpdsf->s.surface_desc.lPitch = lpdsf->s.surface_desc.dwWidth * bpp;
    return DD_OK;
}
