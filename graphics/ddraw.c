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

#ifndef X_DISPLAY_MISSING 

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
typedef int INT32;
#include "ts_xf86vmode.h"
#endif /* defined(HAVE_LIBXXF86VM) */

#include "x11drv.h"

#include <unistd.h>
#include <assert.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "winerror.h"
#include "gdi.h"
#include "heap.h"
#include "dc.h"
#include "win.h"
#include "miscemu.h"
#include "ddraw.h"
#include "d3d.h"
#include "debug.h"
#include "spy.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

/* This for all the enumeration and creation of D3D-related objects */
#include "ddraw_private.h"
#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw)

/* define this if you want to play Diablo using XF86DGA. (bug workaround) */
#undef DIABLO_HACK

/* Restore signal handlers overwritten by XF86DGA 
 */
#define RESTORE_SIGNALS

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

static struct ICOM_VTABLE(IDirectDrawSurface4)	dga_dds4vt, xlib_dds4vt;
static struct ICOM_VTABLE(IDirectDraw)		dga_ddvt, xlib_ddvt;
static struct ICOM_VTABLE(IDirectDraw2)		dga_dd2vt, xlib_dd2vt;
static struct ICOM_VTABLE(IDirectDraw4)		dga_dd4vt, xlib_dd4vt;
static struct ICOM_VTABLE(IDirectDrawClipper)	ddclipvt;
static struct ICOM_VTABLE(IDirectDrawPalette) dga_ddpalvt, xlib_ddpalvt;
static struct ICOM_VTABLE(IDirect3D)			d3dvt;
static struct ICOM_VTABLE(IDirect3D2)			d3d2vt;

#ifdef HAVE_LIBXXF86VM
static XF86VidModeModeInfo *orig_mode = NULL;
#endif

#ifdef HAVE_LIBXXSHM
static int XShmErrorFlag = 0;
#endif

BOOL
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

/**********************************************************************/

typedef struct {
  LPVOID lpCallback;
  LPVOID lpContext; 
} DirectDrawEnumerateProcData;

/***********************************************************************
 *		DirectDrawEnumerateExA (DDRAW.*)
 */
HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  TRACE(ddraw, "(%p,%p, %08lx)\n", lpCallback, lpContext, dwFlags);
  
  if (TRACE_ON(ddraw)) {
    DUMP("  Flags : ");
    if (dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES)
      DUMP("DDENUM_ATTACHEDSECONDARYDEVICES ");
    if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES)
      DUMP("DDENUM_DETACHEDSECONDARYDEVICES ");
    if (dwFlags & DDENUM_NONDISPLAYDEVICES)
      DUMP("DDENUM_NONDISPLAYDEVICES ");
    DUMP("\n");
  }

  if (dwFlags & DDENUM_NONDISPLAYDEVICES) {
    /* For the moment, Wine does not support any 3D only accelerators */
    return DD_OK;
  }
  
  if (DDRAW_DGA_Available()) {
    TRACE(ddraw, "Enumerating DGA interface\n");
    if (!lpCallback(&DGA_DirectDraw_GUID, "WINE with XFree86 DGA", "display", lpContext, 0))
      return DD_OK;
  }

  TRACE(ddraw, "Enumerating Xlib interface\n");
  if (!lpCallback(&XLIB_DirectDraw_GUID, "WINE with Xlib", "display", lpContext, 0))
    return DD_OK;
  
  TRACE(ddraw, "Enumerating Default interface\n");
  if (!lpCallback(NULL,"WINE (default)", "display", lpContext, 0))
    return DD_OK;
  
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateExW (DDRAW.*)
 */

static BOOL CALLBACK DirectDrawEnumerateExProcW(
	GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, 
	LPVOID lpContext, HMONITOR hm)
{
  DirectDrawEnumerateProcData *pEPD =
    (DirectDrawEnumerateProcData *) lpContext;
  LPWSTR lpDriverDescriptionW =
    HEAP_strdupAtoW(GetProcessHeap(), 0, lpDriverDescription);
  LPWSTR lpDriverNameW =
    HEAP_strdupAtoW(GetProcessHeap(), 0, lpDriverName);

  BOOL bResult = (*(LPDDENUMCALLBACKEXW *) pEPD->lpCallback)(
    lpGUID, lpDriverDescriptionW, lpDriverNameW, pEPD->lpContext, hm);

  HeapFree(GetProcessHeap(), 0, lpDriverDescriptionW);
  HeapFree(GetProcessHeap(), 0, lpDriverNameW);

  return bResult;
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  DirectDrawEnumerateProcData epd;
  epd.lpCallback = (LPVOID) lpCallback;
  epd.lpContext = lpContext;

  return DirectDrawEnumerateExA(DirectDrawEnumerateExProcW, 
				  (LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateA (DDRAW.*)
 */

static BOOL CALLBACK DirectDrawEnumerateProcA(
	GUID *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, 
	LPVOID lpContext, HMONITOR hm)
{
  DirectDrawEnumerateProcData *pEPD = 
    (DirectDrawEnumerateProcData *) lpContext;
  
  return ((LPDDENUMCALLBACKA) pEPD->lpCallback)(
    lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) 
{
  DirectDrawEnumerateProcData epd;  
  epd.lpCallback = (LPVOID) lpCallback;
  epd.lpContext = lpContext;

  return DirectDrawEnumerateExA(DirectDrawEnumerateProcA, 
				(LPVOID) &epd, 0);
}

/***********************************************************************
 *		DirectDrawEnumerateW (DDRAW.*)
 */

static BOOL WINAPI DirectDrawEnumerateProcW(
  GUID *lpGUID, LPWSTR lpDriverDescription, LPWSTR lpDriverName, 
  LPVOID lpContext, HMONITOR hm)
{
  DirectDrawEnumerateProcData *pEPD = 
    (DirectDrawEnumerateProcData *) lpContext;
  
  return ((LPDDENUMCALLBACKW) pEPD->lpCallback)(
    lpGUID, lpDriverDescription, lpDriverName, 
    pEPD->lpContext);
}

/**********************************************************************/

HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) 
{
  DirectDrawEnumerateProcData epd;  
  epd.lpCallback = (LPVOID) lpCallback;
  epd.lpContext = lpContext;

  return DirectDrawEnumerateExW(DirectDrawEnumerateProcW, 
				(LPVOID) &epd, 0);
}

/***********************************************************************
 *		DSoundHelp (DDRAW.?)
 */

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

static void _dump_colorkeyflag(DWORD ck) {
	int	i;
	const struct {
		DWORD	mask;
		char	*name;
	} flags[] = {
#define FE(x) { x, #x},
	  FE(DDCKEY_COLORSPACE)
	  FE(DDCKEY_DESTBLT)
	  FE(DDCKEY_DESTOVERLAY)
	  FE(DDCKEY_SRCBLT)
	  FE(DDCKEY_SRCOVERLAY)
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & ck)
			DUMP("%s ",flags[i].name);
	DUMP("\n");  
}

/******************************************************************************
 *		IDirectDrawSurface methods
 *
 * Since DDS3 and DDS2 are supersets of DDS, we implement DDS3 and let
 * DDS and DDS2 use those functions. (Function calls did not change (except
 * using different DirectDrawSurfaceX version), just added flags and functions)
 */
static HRESULT WINAPI IDirectDrawSurface4Impl_Lock(
    LPDIRECTDRAWSURFACE4 iface,LPRECT lprect,LPDDSURFACEDESC lpddsd,DWORD flags, HANDLE hnd
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		This,lprect,lpddsd,flags,(DWORD)hnd);
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	    WARN(ddraw, "(%p)->Lock(%p,%p,%08lx,%08lx)\n",
			 This,lprect,lpddsd,flags,(DWORD)hnd);

	/* First, copy the Surface description */
	*lpddsd = This->s.surface_desc;
	TRACE(ddraw,"locked surface: height=%ld, width=%ld, pitch=%ld\n",
	      lpddsd->dwHeight,lpddsd->dwWidth,lpddsd->lPitch);

	/* If asked only for a part, change the surface pointer */
	if (lprect) {
		FIXME(ddraw,"	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		lpddsd->y.lpSurface = (LPVOID) ((char *) This->s.surface_desc.y.lpSurface +
			(lprect->top*This->s.surface_desc.lPitch) +
						(lprect->left*(This->s.surface_desc.ddpfPixelFormat.x.dwRGBBitCount / 8)));
	} else {
		assert(This->s.surface_desc.y.lpSurface);
	}
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_Unlock(
	LPDIRECTDRAWSURFACE4 iface,LPVOID surface
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->Unlock(%p)\n",This,surface);
	return DD_OK;
}

static void Xlib_copy_surface_on_screen(IDirectDrawSurface4Impl* This) {
  if (This->s.ddraw->d.pixel_convert != NULL)
    This->s.ddraw->d.pixel_convert(This->s.surface_desc.y.lpSurface,
				   This->t.xlib.image->data,
				   This->s.surface_desc.dwWidth,
				   This->s.surface_desc.dwHeight,
				   This->s.surface_desc.lPitch,
				   This->s.palette);

#ifdef HAVE_LIBXXSHM
    if (This->s.ddraw->e.xlib.xshm_active)
      TSXShmPutImage(display,
		     This->s.ddraw->d.drawable,
		     DefaultGCOfScreen(X11DRV_GetXScreen()),
		     This->t.xlib.image,
		     0, 0, 0, 0,
		     This->t.xlib.image->width,
		     This->t.xlib.image->height,
		     False);
    else
#endif
	TSXPutImage(		display,
				This->s.ddraw->d.drawable,
				DefaultGCOfScreen(X11DRV_GetXScreen()),
				This->t.xlib.image,
				0, 0, 0, 0,
				This->t.xlib.image->width,
		  This->t.xlib.image->height);
}

static HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Unlock(
	LPDIRECTDRAWSURFACE4 iface,LPVOID surface)
{
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->Unlock(%p)\n",This,surface);
  
	if (!This->s.ddraw->d.paintable)
		return DD_OK;

  /* Only redraw the screen when unlocking the buffer that is on screen */
	if ((This->t.xlib.image != NULL) &&
	    (This->s.surface_desc.ddsCaps.dwCaps & DDSCAPS_VISIBLE)) {
	  Xlib_copy_surface_on_screen(This);
	  
	if (This->s.palette && This->s.palette->cm)
		TSXSetWindowColormap(display,This->s.ddraw->d.drawable,This->s.palette->cm);
	}

	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_Flip(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;
#ifdef HAVE_LIBXXF86DGA
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);
	if (!iflipto) {
		if (This->s.backbuffer)
			iflipto = This->s.backbuffer;
		else
			iflipto = This;
	}
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,iflipto->t.dga.fb_height);

	if (iflipto->s.palette && iflipto->s.palette->cm) {
		TSXF86DGAInstallColormap(display,DefaultScreen(display),iflipto->s.palette->cm);
	}
	while (!TSXF86DGAViewPortChanged(display,DefaultScreen(display),2)) {
	}
	if (iflipto!=This) {
		int	tmp;
		LPVOID	ptmp;

		tmp = This->t.dga.fb_height;
		This->t.dga.fb_height = iflipto->t.dga.fb_height;
		iflipto->t.dga.fb_height = tmp;

		ptmp = This->s.surface_desc.y.lpSurface;
		This->s.surface_desc.y.lpSurface = iflipto->s.surface_desc.y.lpSurface;
		iflipto->s.surface_desc.y.lpSurface = ptmp;
	}
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Flip(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;
	TRACE(ddraw,"(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);

#ifdef HAVE_MESAGL
	if ((This->s.d3d_device != NULL) ||
	    ((This->s.backbuffer != NULL) && (This->s.backbuffer->s.d3d_device != NULL))) {
	  TRACE(ddraw," - OpenGL flip\n");
	  ENTER_GL();
	  glXSwapBuffers(display,
			 This->s.ddraw->d.drawable);
	  LEAVE_GL();

	  return DD_OK;
	}
#endif /* defined(HAVE_MESAGL) */
	
	if (!This->s.ddraw->d.paintable)
		return DD_OK;

	if (!iflipto) {
		if (This->s.backbuffer)
			iflipto = This->s.backbuffer;
		else
			iflipto = This;
	}
  
	Xlib_copy_surface_on_screen(This);
	
	if (iflipto->s.palette && iflipto->s.palette->cm) {
	  TSXSetWindowColormap(display,This->s.ddraw->d.drawable,iflipto->s.palette->cm);
	}
	if (iflipto!=This) {
		XImage *tmp;
		LPVOID	*surf;
		tmp = This->t.xlib.image;
		This->t.xlib.image = iflipto->t.xlib.image;
		iflipto->t.xlib.image = tmp;
		surf = This->s.surface_desc.y.lpSurface;
		This->s.surface_desc.y.lpSurface = iflipto->s.surface_desc.y.lpSurface;
		iflipto->s.surface_desc.y.lpSurface = surf;
	}
	return DD_OK;
}


/* The IDirectDrawSurface4::SetPalette method attaches the specified
 * DirectDrawPalette object to a surface. The surface uses this palette for all
 * subsequent operations. The palette change takes place immediately.
 */
static HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_SetPalette(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        IDirectDrawPaletteImpl* ipal=(IDirectDrawPaletteImpl*)pal;
	int i;
	TRACE(ddraw,"(%p)->(%p)\n",This,ipal);

	if (ipal == NULL) {
          if( This->s.palette != NULL )
            IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);
	  This->s.palette = ipal;

	  return DD_OK;
	}
	
	if( !(ipal->cm) && (This->s.ddraw->d.screen_pixelformat.x.dwRGBBitCount<=8)) 
	{
		ipal->cm = TSXCreateColormap(display,This->s.ddraw->d.drawable,
					    DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);

    	    	if (!Options.managed)
			TSXInstallColormap(display,ipal->cm);

		for (i=0;i<256;i++) {
			XColor xc;

			xc.red = ipal->palents[i].peRed<<8;
			xc.blue = ipal->palents[i].peBlue<<8;
			xc.green = ipal->palents[i].peGreen<<8;
			xc.flags = DoRed|DoBlue|DoGreen;
			xc.pixel = i;
			TSXStoreColor(display,ipal->cm,&xc);
		}
		TSXInstallColormap(display,ipal->cm);
        }

        /* According to spec, we are only supposed to 
         * AddRef if this is not the same palette.
         */
        if( This->s.palette != ipal )
        {
          if( ipal != NULL )
            IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
          if( This->s.palette != NULL )
            IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.palette );
	  This->s.palette = ipal; 

          /* I think that we need to attach it to all backbuffers...*/
          if( This->s.backbuffer ) {
             if( This->s.backbuffer->s.palette )
               IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.backbuffer->s.palette );
             This->s.backbuffer->s.palette = ipal;
             if( ipal )
                IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
           }
          /* Perform the refresh */
          TSXSetWindowColormap(display,This->s.ddraw->d.drawable,This->s.palette->cm);
        }
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_SetPalette(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        IDirectDrawPaletteImpl* ipal=(IDirectDrawPaletteImpl*)pal;
	TRACE(ddraw,"(%p)->(%p)\n",This,ipal);
#ifdef HAVE_LIBXXF86DGA
        /* According to spec, we are only supposed to 
         * AddRef if this is not the same palette.
         */
        if( This->s.palette != ipal )
        {
          if( ipal != NULL ) 
	  	IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
          if( This->s.palette != NULL )
            IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.palette );
	  This->s.palette = ipal; 

          /* I think that we need to attach it to all backbuffers...*/
          if( This->s.backbuffer ) {
             if( This->s.backbuffer->s.palette )
               IDirectDrawPalette_Release( (IDirectDrawPalette*)This->s.backbuffer->s.palette );
             This->s.backbuffer->s.palette = ipal;
             if ( ipal )
               IDirectDrawPalette_AddRef( (IDirectDrawPalette*)ipal );
          }
	  TSXF86DGAInstallColormap(display,DefaultScreen(display),This->s.palette->cm);
        }
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */


}

static HRESULT _Blt_ColorFill(LPBYTE buf, int width, int height, int bpp, LONG lPitch, DWORD color)
{
	int x, y;
	LPBYTE first;

	/* Do first row */

#define COLORFILL_ROW(type) { \
	type *d = (type *) buf; \
	for (x = 0; x < width; x++) \
		d[x] = (type) color; \
	break; \
}

	switch(bpp) {
	case 1: COLORFILL_ROW(BYTE)
	case 2: COLORFILL_ROW(WORD)
	case 4: COLORFILL_ROW(DWORD)
	default:
	FIXME(ddraw, "Stretched blit not implemented for bpp %d!\n", bpp*8);
	return DDERR_UNSUPPORTED;
	}

#undef COLORFILL_ROW

	/* Now copy first row */
	first = buf;
	for (y = 1; y < height; y++) {
		buf += lPitch;
		memcpy(buf, first, width * bpp);
	}

	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Blt(
	LPDIRECTDRAWSURFACE4 iface,LPRECT rdst,LPDIRECTDRAWSURFACE4 src,LPRECT rsrc,DWORD dwFlags,LPDDBLTFX lpbltfx
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	RECT	xdst,xsrc;
	DDSURFACEDESC	ddesc,sdesc;
	HRESULT			ret = DD_OK;
	int bpp, srcheight, srcwidth, dstheight, dstwidth, width;
	int x, y;
	LPBYTE dbuf, sbuf;

	TRACE(ddraw,"(%p)->(%p,%p,%p,%08lx,%p)\n", This,rdst,src,rsrc,dwFlags,lpbltfx);

	if (src) IDirectDrawSurface4_Lock(src, NULL, &sdesc, 0, 0);
	IDirectDrawSurface4_Lock(iface,NULL,&ddesc,0,0);
	
	if (TRACE_ON(ddraw)) {
	  if (rdst) TRACE(ddraw,"	destrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
	  if (rsrc) TRACE(ddraw,"	srcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	  TRACE(ddraw,"\tflags: ");
	  _dump_DDBLT(dwFlags);
	  if (dwFlags & DDBLT_DDFX) {
	    TRACE(ddraw,"	blitfx: \n");
		_dump_DDBLTFX(lpbltfx->dwDDFX);
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

	bpp = ddesc.ddpfPixelFormat.x.dwRGBBitCount / 8;
	srcheight = xsrc.bottom - xsrc.top;
	srcwidth = xsrc.right - xsrc.left;
	dstheight = xdst.bottom - xdst.top;
	dstwidth = xdst.right - xdst.left;
	width = (xdst.right - xdst.left) * bpp;
	dbuf = (BYTE *) ddesc.y.lpSurface + (xdst.top * ddesc.lPitch) + (xdst.left * bpp);

	dwFlags &= ~(DDBLT_WAIT|DDBLT_ASYNC);/* FIXME: can't handle right now */
	
	/* First, all the 'source-less' blits */
	if (dwFlags & DDBLT_COLORFILL) {
		ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
		                     ddesc.lPitch, lpbltfx->b.dwFillColor);
		dwFlags &= ~DDBLT_COLORFILL;
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
#endif /* defined(HAVE_MESAGL) */
	}
	
	if (dwFlags & DDBLT_ROP) {
		/* Catch some degenerate cases here */
		switch(lpbltfx->dwROP) {
			case BLACKNESS:
				ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, ddesc.lPitch, 0);
				break;
			case 0xAA0029: /* No-op */
				break;
			case WHITENESS:
				ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp, ddesc.lPitch, ~0);
				break;
			default: 
				FIXME(ddraw, "Unsupported raster op: %08lx  Pattern: %p\n", lpbltfx->dwROP, lpbltfx->b.lpDDSPattern);
				goto error;
	    }
		dwFlags &= ~DDBLT_ROP;
	}

    if (dwFlags & DDBLT_DDROPS) {
		FIXME(ddraw, "\tDdraw Raster Ops: %08lx  Pattern: %p\n", lpbltfx->dwDDROP, lpbltfx->b.lpDDSPattern);
	}

	/* Now the 'with source' blits */
	if (src) {
		LPBYTE sbase;
		int sx, xinc, sy, yinc;

		sbase = (BYTE *) sdesc.y.lpSurface + (xsrc.top * sdesc.lPitch) + xsrc.left * bpp;
		xinc = (srcwidth << 16) / dstwidth;
		yinc = (srcheight << 16) / dstheight;

		if (!dwFlags) {

			/* No effects, we can cheat here */
			if (dstwidth == srcwidth) {
				if (dstheight == srcheight) {
					/* No stretching in either direction.  This needs to be as fast as possible */
					sbuf = sbase;
					for (y = 0; y < dstheight; y++) {
						memcpy(dbuf, sbuf, width);
						sbuf += sdesc.lPitch;
						dbuf += ddesc.lPitch;
					}
	} else {
					/* Stretching in Y direction only */
					for (y = sy = 0; y < dstheight; y++, sy += yinc) {
						sbuf = sbase + (sy >> 16) * sdesc.lPitch;
						memcpy(dbuf, sbuf, width);
						dbuf += ddesc.lPitch;
		  }
		}
			} else {
				/* Stretching in X direction */
				int last_sy = -1;
				for (y = sy = 0; y < dstheight; y++, sy += yinc) {
					sbuf = sbase + (sy >> 16) * sdesc.lPitch;

					if ((sy >> 16) == (last_sy >> 16)) {
						/* Same as last row - copy already stretched row */
						memcpy(dbuf, dbuf - ddesc.lPitch, width);
	      } else {

#define STRETCH_ROW(type) { \
	type *s = (type *) sbuf, *d = (type *) dbuf; \
	for (x = sx = 0; x < dstwidth; x++, sx += xinc) \
		d[x] = s[sx >> 16]; \
	break; \
}
		  
						switch(bpp) {
							case 1: STRETCH_ROW(BYTE)
							case 2: STRETCH_ROW(WORD)
							case 4: STRETCH_ROW(DWORD)
							default:
								FIXME(ddraw, "Stretched blit not implemented for bpp %d!\n", bpp*8);
								ret = DDERR_UNSUPPORTED;
								goto error;
		}

#undef STRETCH_ROW

	      }
					last_sy = sy;
					dbuf += ddesc.lPitch;
	      }
	    }
		} else if (dwFlags & (DDBLT_KEYSRC | DDBLT_KEYDEST)) {
			DWORD keylow, keyhigh;

	    if (dwFlags & DDBLT_KEYSRC) {
				keylow  = sdesc.ddckCKSrcBlt.dwColorSpaceLowValue;
				keyhigh = sdesc.ddckCKSrcBlt.dwColorSpaceHighValue;
			} else {
				/* I'm not sure if this is correct */
				FIXME(ddraw, "DDBLT_KEYDEST not fully supported yet.\n");
				keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
				keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
			}

#define COPYROW_COLORKEY(type) { \
	type *s = (type *) sbuf, *d = (type *) dbuf, tmp; \
	for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
		tmp = s[sx >> 16]; \
		if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
	} \
	break; \
		}
		
			for (y = sy = 0; y < dstheight; y++, sy += yinc) {
				sbuf = sbase + (sy >> 16) * sdesc.lPitch;

				switch (bpp) {
					case 1: COPYROW_COLORKEY(BYTE)
					case 2: COPYROW_COLORKEY(WORD)
					case 4: COPYROW_COLORKEY(DWORD)
	      default:
						FIXME(ddraw, "%s color-keyed blit not implemented for bpp %d!\n",
							(dwFlags & DDBLT_KEYSRC) ? "Source" : "Destination", bpp*8);
						ret = DDERR_UNSUPPORTED;
						goto error;
	      }
				dbuf += ddesc.lPitch;
	  }

#undef COPYROW_COLORKEY
			
			dwFlags &= ~(DDBLT_KEYSRC | DDBLT_KEYDEST);

	}
	}
	
error:
	
	if (dwFlags && FIXME_ON(ddraw)) {
	  FIXME(ddraw,"\tUnsupported flags: ");
	  _dump_DDBLT(dwFlags);
	}

	IDirectDrawSurface4_Unlock(iface,ddesc.y.lpSurface);
	if (src) IDirectDrawSurface4_Unlock(src,sdesc.y.lpSurface);

	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_BltFast(
	LPDIRECTDRAWSURFACE4 iface,DWORD dstx,DWORD dsty,LPDIRECTDRAWSURFACE4 src,LPRECT rsrc,DWORD trans
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	int				bpp, w, h, x, y;
	DDSURFACEDESC	ddesc,sdesc;
	HRESULT			ret = DD_OK;
	LPBYTE			sbuf, dbuf;


	if (TRACE_ON(ddraw)) {
	    FIXME(ddraw,"(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		    This,dstx,dsty,src,rsrc,trans
	    );
	    FIXME(ddraw,"	trans:");
	    if (FIXME_ON(ddraw))
	      _dump_DDBLTFAST(trans);
	    FIXME(ddraw,"	srcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	}
	/* We need to lock the surfaces, or we won't get refreshes when done. */
	IDirectDrawSurface4_Lock(src, NULL,&sdesc,DDLOCK_READONLY, 0);
	IDirectDrawSurface4_Lock(iface,NULL,&ddesc,DDLOCK_WRITEONLY,0);

	bpp = This->s.surface_desc.ddpfPixelFormat.x.dwRGBBitCount / 8;
	sbuf = (BYTE *) sdesc.y.lpSurface + (rsrc->top * sdesc.lPitch) + rsrc->left * bpp;
	dbuf = (BYTE *) ddesc.y.lpSurface + (dsty      * ddesc.lPitch) + dstx       * bpp;


	h=rsrc->bottom-rsrc->top;
	if (h>ddesc.dwHeight-dsty) h=ddesc.dwHeight-dsty;
	if (h>sdesc.dwHeight-rsrc->top) h=sdesc.dwHeight-rsrc->top;
	if (h<0) h=0;

	w=rsrc->right-rsrc->left;
	if (w>ddesc.dwWidth-dstx) w=ddesc.dwWidth-dstx;
	if (w>sdesc.dwWidth-rsrc->left) w=sdesc.dwWidth-rsrc->left;
	if (w<0) w=0;

	if (trans & (DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY)) {
		DWORD keylow, keyhigh;
		if (trans & DDBLTFAST_SRCCOLORKEY) {
			keylow  = sdesc.ddckCKSrcBlt.dwColorSpaceLowValue;
			keyhigh = sdesc.ddckCKSrcBlt.dwColorSpaceHighValue;
		} else {
			/* I'm not sure if this is correct */
			FIXME(ddraw, "DDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
			keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
			keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
		}

#define COPYBOX_COLORKEY(type) { \
	type *d = (type *)dbuf, *s = (type *)sbuf, tmp; \
	s = (type *) ((BYTE *) sdesc.y.lpSurface + (rsrc->top * sdesc.lPitch) + rsrc->left * bpp); \
	d = (type *) ((BYTE *) ddesc.y.lpSurface + (dsty * ddesc.lPitch) + dstx * bpp); \
	for (y = 0; y < h; y++) { \
		for (x = 0; x < w; x++) { \
			tmp = s[x]; \
			if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
		} \
		(LPBYTE)s += sdesc.lPitch; \
		(LPBYTE)d += ddesc.lPitch; \
	} \
	break; \
}

		switch (bpp) {
			case 1: COPYBOX_COLORKEY(BYTE)
			case 2: COPYBOX_COLORKEY(WORD)
			case 4: COPYBOX_COLORKEY(DWORD)
			default:
				FIXME(ddraw, "Source color key blitting not supported for bpp %d\n", bpp*8);
				ret = DDERR_UNSUPPORTED;
				goto error;
	}

#undef COPYBOX_COLORKEY

	} else {
		int width = w * bpp;

		for (y = 0; y < h; y++) {
			memcpy(dbuf, sbuf, width);
			sbuf += sdesc.lPitch;
			dbuf += ddesc.lPitch;
		}
	}

error:

	IDirectDrawSurface4_Unlock(iface,ddesc.y.lpSurface);
	IDirectDrawSurface4_Unlock(src,sdesc.y.lpSurface);
	return ret;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_BltBatch(
	LPDIRECTDRAWSURFACE4 iface,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		This,ddbltbatch,x,y
	);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetCaps(
	LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS caps
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->GetCaps(%p)\n",This,caps);
	caps->dwCaps = DDSCAPS_PALETTE; /* probably more */
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE4 iface,LPDDSURFACEDESC ddsd
) { 
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
		TRACE(ddraw, "(%p)->GetSurfaceDesc(%p)\n",
			     This,ddsd);
  
  /* Simply copy the surface description stored in the object */
  *ddsd = This->s.surface_desc;
  
  if (TRACE_ON(ddraw)) {
    DUMP("   flags: ");
		_dump_DDSD(ddsd->dwFlags);
    if (ddsd->dwFlags & DDSD_CAPS) {
      DUMP("   caps:  ");
      _dump_DDSCAPS(ddsd->ddsCaps.dwCaps);
    }
    if (ddsd->dwFlags & DDSD_PIXELFORMAT) {
      DUMP("   pixel format : \n");
      _dump_pixelformat(&(ddsd->ddpfPixelFormat));
    }
	}

	return DD_OK;
}

static ULONG WINAPI IDirectDrawSurface4Impl_AddRef(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );

	return ++(This->ref);
}

static ULONG WINAPI DGA_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

#ifdef HAVE_LIBXXF86DGA
	if (!--(This->ref)) {
		IDirectDraw2_Release((IDirectDraw2*)This->s.ddraw);
		/* clear out of surface list */
		if (This->t.dga.fb_height == -1) {
			HeapFree(GetProcessHeap(),0,This->s.surface_desc.y.lpSurface);
		} else {
			This->s.ddraw->e.dga.vpmask &= ~(1<<(This->t.dga.fb_height/This->s.ddraw->e.dga.fb_height));
		}

		/* Free the backbuffer */
		if (This->s.backbuffer)
			IDirectDrawSurface4_Release((IDirectDrawSurface4*)This->s.backbuffer);

		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return This->ref;
}

static ULONG WINAPI Xlib_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

	if (!--(This->ref)) {
		IDirectDraw2_Release((IDirectDraw2*)This->s.ddraw);

		if( This->s.backbuffer )
		  IDirectDrawSurface4_Release((IDirectDrawSurface4*)This->s.backbuffer);

    if (This->t.xlib.image != NULL) {
      if (This->s.ddraw->d.pixel_convert != NULL) {
	/* In pixel conversion mode, there are two buffers to release... */
	HeapFree(GetProcessHeap(),0,This->s.surface_desc.y.lpSurface);
	
#ifdef HAVE_LIBXXSHM
	if (This->s.ddraw->e.xlib.xshm_active) {
	  TSXShmDetach(display, &(This->t.xlib.shminfo));
	  TSXDestroyImage(This->t.xlib.image);
	  shmdt(This->t.xlib.shminfo.shmaddr);
	} else {
#endif
	  HeapFree(GetProcessHeap(),0,This->t.xlib.image->data);
	  This->t.xlib.image->data = NULL;
	  TSXDestroyImage(This->t.xlib.image);
#ifdef HAVE_LIBXXSHM
	}
#endif
	
      } else {
		This->t.xlib.image->data = NULL;
      
#ifdef HAVE_LIBXXSHM
      if (This->s.ddraw->e.xlib.xshm_active) {
	TSXShmDetach(display, &(This->t.xlib.shminfo));
		TSXDestroyImage(This->t.xlib.image);
	shmdt(This->t.xlib.shminfo.shmaddr);
      } else {
#endif
	      HeapFree(GetProcessHeap(),0,This->s.surface_desc.y.lpSurface);
	TSXDestroyImage(This->t.xlib.image);
#ifdef HAVE_LIBXXSHM	
      }
#endif
      }
      
		This->t.xlib.image = 0;
    } else {
	    HeapFree(GetProcessHeap(),0,This->s.surface_desc.y.lpSurface);
    }

		if (This->s.palette)
                	IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);

		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
  
	return This->ref;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetAttachedSurface(
	LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE4 *lpdsf
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE(ddraw, "(%p)->GetAttachedSurface(%p,%p)\n",
		     This, lpddsd, lpdsf);

	if (TRACE_ON(ddraw)) {
		TRACE(ddraw,"	caps ");
		_dump_DDSCAPS(lpddsd->dwCaps);
	}

	if (!(lpddsd->dwCaps & DDSCAPS_BACKBUFFER)) {
		FIXME(ddraw,"whoops, can only handle backbuffers for now\n");
		return E_FAIL;
	}

	/* FIXME: should handle more than one backbuffer */
	*lpdsf = (LPDIRECTDRAWSURFACE4)This->s.backbuffer;
 
        if( This->s.backbuffer )
          IDirectDrawSurface4_AddRef( (IDirectDrawSurface4*)This->s.backbuffer );

	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Initialize(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->(%p, %p)\n",This,ddraw,lpdsfd);

	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPixelFormat(
	LPDIRECTDRAWSURFACE4 iface,LPDDPIXELFORMAT pf
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->(%p)\n",This,pf);

	*pf = This->s.surface_desc.ddpfPixelFormat;
	
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetBltStatus(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",This,dwFlags);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetOverlayPosition(
	LPDIRECTDRAWSURFACE4 iface,LPLONG x1,LPLONG x2
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",This,x1,x2);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetClipper(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWCLIPPER clipper
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(%p),stub!\n",This,clipper);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddAttachedSurface(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 surf
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(%p),stub!\n",This,surf);

	IDirectDrawSurface4_AddRef(iface);
	
	/* This hack will be enough for the moment */
	if (This->s.backbuffer == NULL)
	  This->s.backbuffer = (IDirectDrawSurface4Impl*)surf;
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->GetDC(%p)\n",This,lphdc);
	*lphdc = BeginPaint(This->s.ddraw->d.window,&This->s.ddraw->d.ps);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ReleaseDC(LPDIRECTDRAWSURFACE4 iface,HDC hdc) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
  	DDSURFACEDESC	desc;
	DWORD x, y;
	
	FIXME(ddraw,"(%p)->(0x%08lx),stub!\n",This,(long)hdc);
	EndPaint(This->s.ddraw->d.window,&This->s.ddraw->d.ps);

	/* Well, as what the application did paint in this DC is NOT saved in the surface,
	   I fill it with 'dummy' values to have something on the screen */
	IDirectDrawSurface4_Lock(iface,NULL,&desc,0,0);
	for (y = 0; y < desc.dwHeight; y++) {
	  for (x = 0; x < desc.dwWidth; x++) {
	    ((unsigned char *) desc.y.lpSurface)[x + y * desc.dwWidth] = (unsigned int) This + x + y;
	  }
	}
	IDirectDrawSurface4_Unlock(iface,NULL);
	
	return DD_OK;
}


static HRESULT WINAPI IDirectDrawSurface4Impl_QueryInterface(LPDIRECTDRAWSURFACE4 iface,REFIID refiid,LPVOID *obj) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",This,xrefiid,obj);
	
	/* All DirectDrawSurface versions (1, 2, 3 and 4) use
	 * the same interface. And IUnknown does that too of course.
	 */
	if (    !memcmp(&IID_IDirectDrawSurface4,refiid,sizeof(IID))	||
	        !memcmp(&IID_IDirectDrawSurface3,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface2,refiid,sizeof(IID))	||
		!memcmp(&IID_IDirectDrawSurface,refiid,sizeof(IID))	||
		!memcmp(&IID_IUnknown,refiid,sizeof(IID))
	) {
		*obj = This;
		IDirectDrawSurface4_AddRef(iface);

		TRACE(ddraw, "  Creating IDirectDrawSurface interface (%p)\n", *obj);
		
		return S_OK;
	}
	else if (!memcmp(&IID_IDirect3DTexture2,refiid,sizeof(IID)))
	  {
	    /* Texture interface */
	    *obj = d3dtexture2_create(This);
	    IDirectDrawSurface4_AddRef(iface);
	    
	    TRACE(ddraw, "  Creating IDirect3DTexture2 interface (%p)\n", *obj);
	    
	    return S_OK;
	  }
	else if (!memcmp(&IID_IDirect3DTexture,refiid,sizeof(IID)))
	  {
	    /* Texture interface */
	    *obj = d3dtexture_create(This);
	    IDirectDrawSurface4_AddRef(iface);
	    
	    TRACE(ddraw, "  Creating IDirect3DTexture interface (%p)\n", *obj);
	    
	    return S_OK;
	  }
	else if (is_OpenGL_dx3(refiid, (IDirectDrawSurfaceImpl*)This, (IDirect3DDeviceImpl**) obj))
	  {
	    /* It is the OpenGL Direct3D Device */
	    IDirectDrawSurface4_AddRef(iface);

	    TRACE(ddraw, "  Creating IDirect3DDevice interface (%p)\n", *obj);
			
		return S_OK;
	}
	
	FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",This,xrefiid);
	return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_IsLost(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE(ddraw,"(%p)->(), stub!\n",This);
	return DD_OK; /* hmm */
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE4 iface,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",This,context,esfcb);

	/* For the moment, only enumerating the back buffer */
	if (This->s.backbuffer != NULL) {
	  TRACE(ddraw, "Enumerating back-buffer (%p)\n", This->s.backbuffer);
	  if (esfcb((LPDIRECTDRAWSURFACE) This->s.backbuffer, &(This->s.backbuffer->s.surface_desc), context) == DDENUMRET_CANCEL)
	    return DD_OK;
	}
	
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Restore(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME(ddraw,"(%p)->(),stub!\n",This);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetColorKey(
	LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPDDCOLORKEY ckey ) 
{
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE(ddraw,"(%p)->(0x%08lx,%p)\n",This,dwFlags,ckey);
	if (TRACE_ON(ddraw)) {
	  DUMP("   (0x%08lx <-> 0x%08lx)  -  ",
	       ckey->dwColorSpaceLowValue,
	       ckey->dwColorSpaceHighValue);
	  _dump_colorkeyflag(dwFlags);
	}

	/* If this surface was loaded as a texture, call also the texture
	   SetColorKey callback */
	if (This->s.texture) {
	  This->s.SetColorKey_cb(This->s.texture, dwFlags, ckey);
	}

        if( dwFlags & DDCKEY_SRCBLT )
        {
           dwFlags &= ~DDCKEY_SRCBLT;
	   This->s.surface_desc.dwFlags |= DDSD_CKSRCBLT;
           memcpy( &(This->s.surface_desc.ddckCKSrcBlt), ckey, sizeof( *ckey ) );
        }

        if( dwFlags & DDCKEY_DESTBLT )
        {
           dwFlags &= ~DDCKEY_DESTBLT;
	   This->s.surface_desc.dwFlags |= DDSD_CKDESTBLT;
           memcpy( &(This->s.surface_desc.ddckCKDestBlt), ckey, sizeof( *ckey ) );
        }

        if( dwFlags & DDCKEY_SRCOVERLAY )
        {
           dwFlags &= ~DDCKEY_SRCOVERLAY;
	   This->s.surface_desc.dwFlags |= DDSD_CKSRCOVERLAY;
           memcpy( &(This->s.surface_desc.ddckCKSrcOverlay), ckey, sizeof( *ckey ) );	   
        }
	
        if( dwFlags & DDCKEY_DESTOVERLAY )
        {
           dwFlags &= ~DDCKEY_DESTOVERLAY;
	   This->s.surface_desc.dwFlags |= DDSD_CKDESTOVERLAY;
           memcpy( &(This->s.surface_desc.ddckCKDestOverlay), ckey, sizeof( *ckey ) );	   
        }

        if( dwFlags )
        {
          FIXME( ddraw, "unhandled dwFlags: 0x%08lx\n", dwFlags );
        }

        return DD_OK;

}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddOverlayDirtyRect(
        LPDIRECTDRAWSURFACE4 iface, 
        LPRECT lpRect )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p),stub!\n",This,lpRect); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_DeleteAttachedSurface(
        LPDIRECTDRAWSURFACE4 iface, 
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n",This,dwFlags,lpDDSAttachedSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumOverlayZOrders(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpfnCallback )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx,%p,%p),stub!\n", This,dwFlags,
          lpContext, lpfnCallback );

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetClipper(
        LPDIRECTDRAWSURFACE4 iface,
        LPDIRECTDRAWCLIPPER* lplpDDClipper )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p),stub!\n", This, lplpDDClipper);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetColorKey(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPDDCOLORKEY lpDDColorKey )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  TRACE(ddraw,"(%p)->(0x%08lx,%p)\n", This, dwFlags, lpDDColorKey);

  if( dwFlags & DDCKEY_SRCBLT )  {
     dwFlags &= ~DDCKEY_SRCBLT;
     memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKSrcBlt), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_DESTBLT )
  {
    dwFlags &= ~DDCKEY_DESTBLT;
    memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKDestBlt), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_SRCOVERLAY )
  {
    dwFlags &= ~DDCKEY_SRCOVERLAY;
    memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKSrcOverlay), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags & DDCKEY_DESTOVERLAY )
  {
    dwFlags &= ~DDCKEY_DESTOVERLAY;
    memcpy( lpDDColorKey, &(This->s.surface_desc.ddckCKDestOverlay), sizeof( *lpDDColorKey ) );
  }

  if( dwFlags )
  {
    FIXME( ddraw, "unhandled dwFlags: 0x%08lx\n", dwFlags );
  }

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetFlipStatus(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags ) 
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPalette(
        LPDIRECTDRAWSURFACE4 iface,
        LPDIRECTDRAWPALETTE* lplpDDPalette )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p),stub!\n", This, lplpDDPalette);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetOverlayPosition(
        LPDIRECTDRAWSURFACE4 iface,
        LONG lX,
        LONG lY)
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%ld,%ld),stub!\n", This, lX, lY);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlay(
        LPDIRECTDRAWSURFACE4 iface,
        LPRECT lpSrcRect,
        LPDIRECTDRAWSURFACE4 lpDDDestSurface,
        LPRECT lpDestRect,
        DWORD dwFlags,
        LPDDOVERLAYFX lpDDOverlayFx )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p,%p,%p,0x%08lx,%p),stub!\n", This,
         lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx );  

  return DD_OK;
}
 
static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayDisplay(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", This, dwFlags); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayZOrder(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSReference )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx,%p),stub!\n", This, dwFlags, lpDDSReference);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDDInterface(
        LPDIRECTDRAWSURFACE4 iface,
        LPVOID* lplpDD )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p),stub!\n", This, lplpDD);

  /* Not sure about that... */
  *lplpDD = (void *) This->s.ddraw;
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageLock(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageUnlock(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetSurfaceDesc(
        LPDIRECTDRAWSURFACE4 iface,
        LPDDSURFACEDESC lpDDSD,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw,"(%p)->(%p,0x%08lx),stub!\n", This, lpDDSD, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetPrivateData(LPDIRECTDRAWSURFACE4 iface,
							 REFGUID guidTag,
							 LPVOID lpData,
							 DWORD cbSize,
							 DWORD dwFlags) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%p,%ld,%08lx\n", This, guidTag, lpData, cbSize, dwFlags);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPrivateData(LPDIRECTDRAWSURFACE4 iface,
							 REFGUID guidTag,
							 LPVOID lpBuffer,
							 LPDWORD lpcbBufferSize) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%p,%p)\n", This, guidTag, lpBuffer, lpcbBufferSize);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_FreePrivateData(LPDIRECTDRAWSURFACE4 iface,
							  REFGUID guidTag)  {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw, "(%p)->(%p)\n", This, guidTag);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetUniquenessValue(LPDIRECTDRAWSURFACE4 iface,
							     LPDWORD lpValue)  {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw, "(%p)->(%p)\n", This, lpValue);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ChangeUniquenessValue(LPDIRECTDRAWSURFACE4 iface) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME(ddraw, "(%p)\n", This);
  
  return DD_OK;
}

static ICOM_VTABLE(IDirectDrawSurface4) dga_dds4vt = {
	IDirectDrawSurface4Impl_QueryInterface,
	IDirectDrawSurface4Impl_AddRef,
	DGA_IDirectDrawSurface4Impl_Release,
	IDirectDrawSurface4Impl_AddAttachedSurface,
	IDirectDrawSurface4Impl_AddOverlayDirtyRect,
	IDirectDrawSurface4Impl_Blt,
	IDirectDrawSurface4Impl_BltBatch,
	IDirectDrawSurface4Impl_BltFast,
	IDirectDrawSurface4Impl_DeleteAttachedSurface,
	IDirectDrawSurface4Impl_EnumAttachedSurfaces,
	IDirectDrawSurface4Impl_EnumOverlayZOrders,
	DGA_IDirectDrawSurface4Impl_Flip,
	IDirectDrawSurface4Impl_GetAttachedSurface,
	IDirectDrawSurface4Impl_GetBltStatus,
	IDirectDrawSurface4Impl_GetCaps,
	IDirectDrawSurface4Impl_GetClipper,
	IDirectDrawSurface4Impl_GetColorKey,
	IDirectDrawSurface4Impl_GetDC,
	IDirectDrawSurface4Impl_GetFlipStatus,
	IDirectDrawSurface4Impl_GetOverlayPosition,
	IDirectDrawSurface4Impl_GetPalette,
	IDirectDrawSurface4Impl_GetPixelFormat,
	IDirectDrawSurface4Impl_GetSurfaceDesc,
	IDirectDrawSurface4Impl_Initialize,
	IDirectDrawSurface4Impl_IsLost,
	IDirectDrawSurface4Impl_Lock,
	IDirectDrawSurface4Impl_ReleaseDC,
	IDirectDrawSurface4Impl_Restore,
	IDirectDrawSurface4Impl_SetClipper,
	IDirectDrawSurface4Impl_SetColorKey,
	IDirectDrawSurface4Impl_SetOverlayPosition,
	DGA_IDirectDrawSurface4Impl_SetPalette,
	DGA_IDirectDrawSurface4Impl_Unlock,
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

static ICOM_VTABLE(IDirectDrawSurface4) xlib_dds4vt = {
	IDirectDrawSurface4Impl_QueryInterface,
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
	IDirectDrawSurface4Impl_GetDC,
	IDirectDrawSurface4Impl_GetFlipStatus,
	IDirectDrawSurface4Impl_GetOverlayPosition,
	IDirectDrawSurface4Impl_GetPalette,
	IDirectDrawSurface4Impl_GetPixelFormat,
	IDirectDrawSurface4Impl_GetSurfaceDesc,
	IDirectDrawSurface4Impl_Initialize,
	IDirectDrawSurface4Impl_IsLost,
	IDirectDrawSurface4Impl_Lock,
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

/******************************************************************************
 *			DirectDrawCreateClipper (DDRAW.7)
 */
HRESULT WINAPI DirectDrawCreateClipper( DWORD dwFlags,
				        LPDIRECTDRAWCLIPPER *lplpDDClipper,
				        LPUNKNOWN pUnkOuter)
{
  IDirectDrawClipperImpl** ilplpDDClipper=(IDirectDrawClipperImpl**)lplpDDClipper;
  TRACE(ddraw, "(%08lx,%p,%p)\n", dwFlags, ilplpDDClipper, pUnkOuter);

  *ilplpDDClipper = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
  (*ilplpDDClipper)->lpvtbl = &ddclipvt;
  (*ilplpDDClipper)->ref = 1;

  return DD_OK;
}

/******************************************************************************
 *			IDirectDrawClipper
 */
static HRESULT WINAPI IDirectDrawClipperImpl_SetHwnd(
	LPDIRECTDRAWCLIPPER iface,DWORD x,HWND hwnd
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
	FIXME(ddraw,"(%p)->SetHwnd(0x%08lx,0x%08lx),stub!\n",This,x,(DWORD)hwnd);
	return DD_OK;
}

static ULONG WINAPI IDirectDrawClipperImpl_Release(LPDIRECTDRAWCLIPPER iface) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetClipList(
	LPDIRECTDRAWCLIPPER iface,LPRECT rects,LPRGNDATA lprgn,LPDWORD hmm
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
	FIXME(ddraw,"(%p,%p,%p,%p),stub!\n",This,rects,lprgn,hmm);
	if (hmm) *hmm=0;
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_SetClipList(
	LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD hmm
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
	FIXME(ddraw,"(%p,%p,%ld),stub!\n",This,lprgn,hmm);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_QueryInterface(
         LPDIRECTDRAWCLIPPER iface,
         REFIID riid,
         LPVOID* ppvObj )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME(ddraw,"(%p)->(%p,%p),stub!\n",This,riid,ppvObj);
   return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirectDrawClipperImpl_AddRef( LPDIRECTDRAWCLIPPER iface )
{
  ICOM_THIS(IDirectDrawClipperImpl,iface);
  TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );
  return ++(This->ref);
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetHWnd(
         LPDIRECTDRAWCLIPPER iface,
         HWND* HWndPtr )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME(ddraw,"(%p)->(%p),stub!\n",This,HWndPtr);
   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_Initialize(
         LPDIRECTDRAWCLIPPER iface,
         LPDIRECTDRAW lpDD,
         DWORD dwFlags )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME(ddraw,"(%p)->(%p,0x%08lx),stub!\n",This,lpDD,dwFlags);
   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_IsClipListChanged(
         LPDIRECTDRAWCLIPPER iface,
         BOOL* lpbChanged )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME(ddraw,"(%p)->(%p),stub!\n",This,lpbChanged);
   return DD_OK;
}

static ICOM_VTABLE(IDirectDrawClipper) ddclipvt = {
        IDirectDrawClipperImpl_QueryInterface,
        IDirectDrawClipperImpl_AddRef,
        IDirectDrawClipperImpl_Release,
        IDirectDrawClipperImpl_GetClipList,
        IDirectDrawClipperImpl_GetHWnd,
        IDirectDrawClipperImpl_Initialize,
        IDirectDrawClipperImpl_IsClipListChanged,
        IDirectDrawClipperImpl_SetClipList,
        IDirectDrawClipperImpl_SetHwnd
};


/******************************************************************************
 *			IDirectDrawPalette
 */
static HRESULT WINAPI IDirectDrawPaletteImpl_GetEntries(
	LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
	int	i;

	TRACE(ddraw,"(%p)->GetEntries(%08lx,%ld,%ld,%p)\n",
	      This,x,start,count,palent);

	if (!This->cm) /* should not happen */ {
		FIXME(ddraw,"app tried to read colormap for non-palettized mode\n");
		return DDERR_GENERIC;
	}
	for (i=0;i<count;i++) {
                palent[i].peRed   = This->palents[start+i].peRed;
                palent[i].peBlue  = This->palents[start+i].peBlue;
                palent[i].peGreen = This->palents[start+i].peGreen;
                palent[i].peFlags = This->palents[start+i].peFlags;

	}
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDrawPaletteImpl_SetEntries(
	LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
	XColor		xc;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		This,x,start,count,palent
	);
	for (i=0;i<count;i++) {
		xc.red = palent[i].peRed<<8;
		xc.blue = palent[i].peBlue<<8;
		xc.green = palent[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = start+i;

		if (This->cm)
		    TSXStoreColor(display,This->cm,&xc);

		This->palents[start+i].peRed = palent[i].peRed;
		This->palents[start+i].peBlue = palent[i].peBlue;
		This->palents[start+i].peGreen = palent[i].peGreen;
                This->palents[start+i].peFlags = palent[i].peFlags;
	}

	/* Now, if we are in 'depth conversion mode', update the screen palette */
	if (This->ddraw->d.palette_convert != NULL)
	  This->ddraw->d.palette_convert(palent, This->screen_palents, start, count);
	  
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDrawPaletteImpl_SetEntries(
	LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
	XColor		xc;
	Colormap	cm;
	int		i;

	TRACE(ddraw,"(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		This,x,start,count,palent
	);
	if (!This->cm) /* should not happen */ {
		FIXME(ddraw,"app tried to set colormap in non-palettized mode\n");
		return DDERR_GENERIC;
	}
	/* FIXME: free colorcells instead of freeing whole map */
	cm = This->cm;
	This->cm = TSXCopyColormapAndFree(display,This->cm);
	TSXFreeColormap(display,cm);

	for (i=0;i<count;i++) {
		xc.red = palent[i].peRed<<8;
		xc.blue = palent[i].peBlue<<8;
		xc.green = palent[i].peGreen<<8;
		xc.flags = DoRed|DoBlue|DoGreen;
		xc.pixel = i+start;

		TSXStoreColor(display,This->cm,&xc);

		This->palents[start+i].peRed = palent[i].peRed;
		This->palents[start+i].peBlue = palent[i].peBlue;
		This->palents[start+i].peGreen = palent[i].peGreen;
                This->palents[start+i].peFlags = palent[i].peFlags;
	}
	TSXF86DGAInstallColormap(display,DefaultScreen(display),This->cm);
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static ULONG WINAPI IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE iface) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );
	if (!--(This->ref)) {
		if (This->cm) {
			TSXFreeColormap(display,This->cm);
			This->cm = 0;
		}
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}

static ULONG WINAPI IDirectDrawPaletteImpl_AddRef(LPDIRECTDRAWPALETTE iface) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);

        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );
	return ++(This->ref);
}

static HRESULT WINAPI IDirectDrawPaletteImpl_Initialize(
	LPDIRECTDRAWPALETTE iface,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
        TRACE(ddraw,"(%p)->(%p,%ld,%p)\n", This, ddraw, x, palent);

	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawPaletteImpl_GetCaps(
         LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps )
{
   ICOM_THIS(IDirectDrawPaletteImpl,iface);
   FIXME( ddraw, "(%p)->(%p) stub.\n", This, lpdwCaps );
   return DD_OK;
} 

static HRESULT WINAPI IDirectDrawPaletteImpl_QueryInterface(
        LPDIRECTDRAWPALETTE iface,REFIID refiid,LPVOID *obj ) 
{
  ICOM_THIS(IDirectDrawPaletteImpl,iface);
  char    xrefiid[50];

  WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
  FIXME(ddraw,"(%p)->(%s,%p) stub.\n",This,xrefiid,obj);

  return S_OK;
}

static ICOM_VTABLE(IDirectDrawPalette) dga_ddpalvt = {
	IDirectDrawPaletteImpl_QueryInterface,
	IDirectDrawPaletteImpl_AddRef,
	IDirectDrawPaletteImpl_Release,
	IDirectDrawPaletteImpl_GetCaps,
	IDirectDrawPaletteImpl_GetEntries,
	IDirectDrawPaletteImpl_Initialize,
	DGA_IDirectDrawPaletteImpl_SetEntries
};

static ICOM_VTABLE(IDirectDrawPalette) xlib_ddpalvt = {
	IDirectDrawPaletteImpl_QueryInterface,
	IDirectDrawPaletteImpl_AddRef,
	IDirectDrawPaletteImpl_Release,
	IDirectDrawPaletteImpl_GetCaps,
	IDirectDrawPaletteImpl_GetEntries,
	IDirectDrawPaletteImpl_Initialize,
	Xlib_IDirectDrawPaletteImpl_SetEntries
};

/*******************************************************************************
 *				IDirect3D
 */
static HRESULT WINAPI IDirect3DImpl_QueryInterface(
        LPDIRECT3D iface,REFIID refiid,LPVOID *obj
) {
        ICOM_THIS(IDirect3DImpl,iface);
	/* FIXME: Not sure if this is correct */
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",This,xrefiid,obj);
        if ((!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) ||
	    (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) ||
	    (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4)))) {
                *obj = This->ddraw;
                IDirect3D_AddRef(iface);

		TRACE(ddraw, "  Creating IDirectDrawX interface (%p)\n", *obj);
		
                return S_OK;
        }
        if ((!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) ||
	    (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)))) {
                *obj = This;
                IDirect3D_AddRef(iface);

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);

                return S_OK;
        }
        if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D2))) {
                IDirect3D2Impl*  d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = This->ddraw;
                IDirect3D_AddRef(iface);
                d3d->lpvtbl = &d3d2vt;
                *obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);

                return S_OK;
        }
        FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",This,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirect3DImpl_AddRef(LPDIRECT3D iface) {
        ICOM_THIS(IDirect3DImpl,iface);
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );

        return ++(This->ref);
}

static ULONG WINAPI IDirect3DImpl_Release(LPDIRECT3D iface)
{
        ICOM_THIS(IDirect3DImpl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

        if (!--(This->ref)) {
                IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
                HeapFree(GetProcessHeap(),0,This);
                return 0;
        }
        return This->ref;
}

static HRESULT WINAPI IDirect3DImpl_Initialize(
         LPDIRECT3D iface, REFIID refiid )
{
  ICOM_THIS(IDirect3DImpl,iface);
  /* FIXME: Not sure if this is correct */
  char    xrefiid[50];

  WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
  FIXME(ddraw,"(%p)->(%s):stub.\n",This,xrefiid);
  
  return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirect3DImpl_EnumDevices(LPDIRECT3D iface,
					    LPD3DENUMDEVICESCALLBACK cb,
					    LPVOID context) {
  ICOM_THIS(IDirect3DImpl,iface);
  FIXME(ddraw,"(%p)->(%p,%p),stub!\n",This,cb,context);

  /* Call functions defined in d3ddevices.c */
  if (!d3d_OpenGL_dx3(cb, context))
    return DD_OK;

  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_CreateLight(LPDIRECT3D iface,
					    LPDIRECT3DLIGHT *lplight,
					    IUnknown *lpunk)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lplight, lpunk);
  
  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create_dx3(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_CreateMaterial(LPDIRECT3D iface,
					       LPDIRECT3DMATERIAL *lpmaterial,
					       IUnknown *lpunk)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_CreateViewport(LPDIRECT3D iface,
					       LPDIRECT3DVIEWPORT *lpviewport,
					       IUnknown *lpunk)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_FindDevice(LPDIRECT3D iface,
					   LPD3DFINDDEVICESEARCH lpfinddevsrc,
					   LPD3DFINDDEVICERESULT lpfinddevrst)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);
  
  return DD_OK;
}

static ICOM_VTABLE(IDirect3D) d3dvt = {
        IDirect3DImpl_QueryInterface,
        IDirect3DImpl_AddRef,
        IDirect3DImpl_Release,
        IDirect3DImpl_Initialize,
        IDirect3DImpl_EnumDevices,
        IDirect3DImpl_CreateLight,
        IDirect3DImpl_CreateMaterial,
        IDirect3DImpl_CreateViewport,
	IDirect3DImpl_FindDevice
};

/*******************************************************************************
 *				IDirect3D2
 */
static HRESULT WINAPI IDirect3D2Impl_QueryInterface(
        LPDIRECT3D2 iface,REFIID refiid,LPVOID *obj) {  
	ICOM_THIS(IDirect3D2Impl,iface);

	/* FIXME: Not sure if this is correct */
        char    xrefiid[50];

        WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
        TRACE(ddraw,"(%p)->(%s,%p)\n",This,xrefiid,obj);
        if ((!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) ||
	    (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) ||
	    (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4)))) {
                *obj = This->ddraw;
                IDirect3D2_AddRef(iface);

		TRACE(ddraw, "  Creating IDirectDrawX interface (%p)\n", *obj);
		
                return S_OK;
        }
        if ((!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D2))) ||
	    (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown)))) {
                *obj = This;
                IDirect3D2_AddRef(iface);

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);

                return S_OK;
        }
        if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
                IDirect3DImpl*  d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = This->ddraw;
                IDirect3D2_AddRef(iface);
                d3d->lpvtbl = &d3dvt;
                *obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);

                return S_OK;
        }
        FIXME(ddraw,"(%p):interface for IID %s NOT found!\n",This,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirect3D2Impl_AddRef(LPDIRECT3D2 iface) {
        ICOM_THIS(IDirect3D2Impl,iface);
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );

        return ++(This->ref);
}

static ULONG WINAPI IDirect3D2Impl_Release(LPDIRECT3D2 iface) {
        ICOM_THIS(IDirect3D2Impl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

	if (!--(This->ref)) {
		IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IDirect3D2Impl_EnumDevices(
	LPDIRECT3D2 iface,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
        ICOM_THIS(IDirect3D2Impl,iface);
	FIXME(ddraw,"(%p)->(%p,%p),stub!\n",This,cb,context);

	/* Call functions defined in d3ddevices.c */
	if (!d3d_OpenGL(cb, context))
	  return DD_OK;

	return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateLight(LPDIRECT3D2 iface,
					     LPDIRECT3DLIGHT *lplight,
					     IUnknown *lpunk)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lplight, lpunk);

  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create(This);
  
	return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateMaterial(LPDIRECT3D2 iface,
						LPDIRECT3DMATERIAL2 *lpmaterial,
						IUnknown *lpunk)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial2_create(This);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateViewport(LPDIRECT3D2 iface,
						LPDIRECT3DVIEWPORT2 *lpviewport,
						IUnknown *lpunk)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport2_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_FindDevice(LPDIRECT3D2 iface,
					    LPD3DFINDDEVICESEARCH lpfinddevsrc,
					    LPD3DFINDDEVICERESULT lpfinddevrst)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE(ddraw, "(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateDevice(LPDIRECT3D2 iface,
					      REFCLSID rguid,
					      LPDIRECTDRAWSURFACE surface,
					      LPDIRECT3DDEVICE2 *device)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  char	xbuf[50];
  
  WINE_StringFromCLSID(rguid,xbuf);
  FIXME(ddraw,"(%p)->(%s,%p,%p): stub\n",This,xbuf,surface,device);

  if (is_OpenGL(rguid, (IDirectDrawSurfaceImpl*)surface, (IDirect3DDevice2Impl**)device, This)) {
    IDirect3D2_AddRef(iface);
    return DD_OK;
}

  return DDERR_INVALIDPARAMS;
}

static ICOM_VTABLE(IDirect3D2) d3d2vt = {
	IDirect3D2Impl_QueryInterface,
	IDirect3D2Impl_AddRef,
        IDirect3D2Impl_Release,
        IDirect3D2Impl_EnumDevices,
	IDirect3D2Impl_CreateLight,
	IDirect3D2Impl_CreateMaterial,
	IDirect3D2Impl_CreateViewport,
	IDirect3D2Impl_FindDevice,
	IDirect3D2Impl_CreateDevice
};

/*******************************************************************************
 *				IDirectDraw
 */

/* Used in conjunction with cbWndExtra for storage of the this ptr for the window.
 * Please adjust allocation in Xlib_DirectDrawCreate if you store more data here.
 */
static INT ddrawXlibThisOffset = 0;

static HRESULT common_off_screen_CreateSurface(IDirectDraw2Impl* This,
					       LPDDSURFACEDESC lpddsd,
					       IDirectDrawSurfaceImpl* lpdsf)
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
    lpddsd->ddpfPixelFormat = This->d.directdraw_pixelformat;
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

static HRESULT WINAPI DGA_IDirectDraw2Impl_CreateSurface(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawSurfaceImpl** ilpdsf=(IDirectDrawSurfaceImpl**)lpdsf;
	int	i;

	TRACE(ddraw, "(%p)->(%p,%p,%p)\n",This,lpddsd,ilpdsf,lpunk);
	if (TRACE_ON(ddraw)) {
		DUMP("   w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
		_dump_DDSD(lpddsd->dwFlags);
		DUMP("   caps ");
		_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
	}

	*ilpdsf = (IDirectDrawSurfaceImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurfaceImpl));
	IDirectDraw2_AddRef(iface);
        
	(*ilpdsf)->ref = 1;
	(*ilpdsf)->lpvtbl = (ICOM_VTABLE(IDirectDrawSurface)*)&dga_dds4vt;
	(*ilpdsf)->s.ddraw = This;
	(*ilpdsf)->s.palette = NULL;
	(*ilpdsf)->t.dga.fb_height = -1; /* This is to have non-on screen surfaces freed */

	if (!(lpddsd->dwFlags & DDSD_WIDTH))
	  lpddsd->dwWidth  = This->d.width;
	if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	  lpddsd->dwHeight = This->d.height;
	
	/* Check if this a 'primary surface' or not */
	if ((lpddsd->dwFlags & DDSD_CAPS) &&
	    (lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {

	  /* This is THE primary surface => there is DGA-specific code */
	  /* First, store the surface description */
	  (*ilpdsf)->s.surface_desc = *lpddsd;
	  
	  /* Find a viewport */
		for (i=0;i<32;i++)
			if (!(This->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for a primary surface\n",i);
		/* if i == 32 or maximum ... return error */
		This->e.dga.vpmask|=(1<<i);
	  (*ilpdsf)->s.surface_desc.y.lpSurface =
	    This->e.dga.fb_addr+((i*This->e.dga.fb_height)*This->e.dga.fb_width*This->d.directdraw_pixelformat.x.dwRGBBitCount/8);
		(*ilpdsf)->t.dga.fb_height = i*This->e.dga.fb_height;
	  (*ilpdsf)->s.surface_desc.lPitch = This->e.dga.fb_width*This->d.directdraw_pixelformat.x.dwRGBBitCount/8;
	  lpddsd->lPitch = (*ilpdsf)->s.surface_desc.lPitch;

	  /* Add flags if there were not present */
	  (*ilpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;
	  (*ilpdsf)->s.surface_desc.dwWidth = This->d.width;
	  (*ilpdsf)->s.surface_desc.dwHeight = This->d.height;
	  TRACE(ddraw,"primary surface: dwWidth=%ld, dwHeight=%ld, lPitch=%ld\n",This->d.width,This->d.height,lpddsd->lPitch);
	  /* We put our surface always in video memory */
	  (*ilpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	  (*ilpdsf)->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;
	(*ilpdsf)->s.backbuffer = NULL;
	  
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		IDirectDrawSurface4Impl*	back;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

	    (*ilpdsf)->s.backbuffer = back =
	      (IDirectDrawSurface4Impl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface4Impl));
		IDirectDraw2_AddRef(iface);
		back->ref = 1;
		back->lpvtbl = (ICOM_VTABLE(IDirectDrawSurface4)*)&dga_dds4vt;
		for (i=0;i<32;i++)
			if (!(This->e.dga.vpmask & (1<<i)))
				break;
		TRACE(ddraw,"using viewport %d for backbuffer\n",i);
		/* if i == 32 or maximum ... return error */
		This->e.dga.vpmask|=(1<<i);
		back->t.dga.fb_height = i*This->e.dga.fb_height;

	    /* Copy the surface description from the front buffer */
	    back->s.surface_desc = (*ilpdsf)->s.surface_desc;
	    /* Change the parameters that are not the same */
	    back->s.surface_desc.y.lpSurface = This->e.dga.fb_addr+
	      ((i*This->e.dga.fb_height)*This->e.dga.fb_width*This->d.directdraw_pixelformat.x.dwRGBBitCount/8);
		back->s.ddraw = This;
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */

	    /* Add relevant info to front and back buffers */
	    (*ilpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
	    back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	    back->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	  }
	} else {
	  /* There is no DGA-specific code here...
	     Go to the common surface creation function */
	  return common_off_screen_CreateSurface(This, lpddsd, *ilpdsf);
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

static XImage *create_xshmimage(IDirectDraw2Impl* This, IDirectDrawSurface4Impl* lpdsf) {
  XImage *img;
  int (*WineXHandler)(Display *, XErrorEvent *);

    img = TSXShmCreateImage(display,
			    DefaultVisualOfScreen(X11DRV_GetXScreen()),
			    This->d.pixmap_depth,
			    ZPixmap,
			    NULL,
			    &(lpdsf->t.xlib.shminfo),
			    lpdsf->s.surface_desc.dwWidth,
			    lpdsf->s.surface_desc.dwHeight);
    
  if (img == NULL) {
    MSG("Couldn't create XShm image (due to X11 remote display or failure).\nReverting to standard X images !\n");
    This->e.xlib.xshm_active = 0;
      return NULL;
  }

    lpdsf->t.xlib.shminfo.shmid = shmget( IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT|0777 );
    if (lpdsf->t.xlib.shminfo.shmid < 0) {
    MSG("Couldn't create shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
    This->e.xlib.xshm_active = 0;
      TSXDestroyImage(img);
      return NULL;
    }

    lpdsf->t.xlib.shminfo.shmaddr = img->data = (char*)shmat(lpdsf->t.xlib.shminfo.shmid, 0, 0);
      
    if (img->data == (char *) -1) {
    MSG("Couldn't attach shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
    This->e.xlib.xshm_active = 0;
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

    MSG("Couldn't attach shared memory segment to X server (due to X11 remote display or failure).\nReverting to standard X images !\n");
    This->e.xlib.xshm_active = 0;
    
    /* Leave the critical section */
    LeaveCriticalSection( &X11DRV_CritSection );

    return NULL;
  }

  /* Here, to be REALLY sure, I should do a XShmPutImage to check if this works,
     but it may be a bit overkill.... */
  XSetErrorHandler(WineXHandler);
  LeaveCriticalSection( &X11DRV_CritSection );
  
    shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);

    if (This->d.pixel_convert != NULL) {
      lpdsf->s.surface_desc.y.lpSurface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
						    lpdsf->s.surface_desc.dwWidth *
						    lpdsf->s.surface_desc.dwHeight *
						    (This->d.directdraw_pixelformat.x.dwRGBBitCount));
    } else {
    lpdsf->s.surface_desc.y.lpSurface = img->data;
    }

  return img;
}
#endif /* HAVE_LIBXXSHM */

static XImage *create_ximage(IDirectDraw2Impl* This, IDirectDrawSurface4Impl* lpdsf) {
  XImage *img = NULL;
  void *img_data;

#ifdef HAVE_LIBXXSHM
  if (This->e.xlib.xshm_active) {
    img = create_xshmimage(This, lpdsf);
  }
    
  if (img == NULL) {
#endif
    /* Allocate surface memory */
    lpdsf->s.surface_desc.y.lpSurface = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
						  lpdsf->s.surface_desc.dwWidth *
						  lpdsf->s.surface_desc.dwHeight *
						  (This->d.directdraw_pixelformat.x.dwRGBBitCount / 8));
    
    if (This->d.pixel_convert != NULL) {
      img_data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
			   lpdsf->s.surface_desc.dwWidth *
			   lpdsf->s.surface_desc.dwHeight *
			   (This->d.screen_pixelformat.x.dwRGBBitCount / 8));
    } else {
      img_data = lpdsf->s.surface_desc.y.lpSurface;
    }
      
    /* In this case, create an XImage */
    img =
      TSXCreateImage(display,
		     DefaultVisualOfScreen(X11DRV_GetXScreen()),
		     This->d.pixmap_depth,
		     ZPixmap,
		     0,
		     img_data,
		     lpdsf->s.surface_desc.dwWidth,
		     lpdsf->s.surface_desc.dwHeight,
		     32,
		     lpdsf->s.surface_desc.dwWidth * (This->d.screen_pixelformat.x.dwRGBBitCount / 8)
		     );
    
#ifdef HAVE_LIBXXSHM
  }
#endif
  if (This->d.pixel_convert != NULL) {
    lpdsf->s.surface_desc.lPitch = (This->d.directdraw_pixelformat.x.dwRGBBitCount / 8) * lpdsf->s.surface_desc.dwWidth;
  } else {
  lpdsf->s.surface_desc.lPitch = img->bytes_per_line;
  }
  
  return img;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreateSurface(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawSurfaceImpl** ilpdsf=(IDirectDrawSurfaceImpl**)lpdsf;
	TRACE(ddraw, "(%p)->CreateSurface(%p,%p,%p)\n",
		     This,lpddsd,ilpdsf,lpunk);

	if (TRACE_ON(ddraw)) {
		DUMP("   w=%ld,h=%ld,flags ",lpddsd->dwWidth,lpddsd->dwHeight);
		_dump_DDSD(lpddsd->dwFlags);
		DUMP("   caps ");
		_dump_DDSCAPS(lpddsd->ddsCaps.dwCaps);
	}

	*ilpdsf = (IDirectDrawSurfaceImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurfaceImpl));

	IDirectDraw2_AddRef(iface);
	(*ilpdsf)->s.ddraw             = This;
	(*ilpdsf)->ref                 = 1;
	(*ilpdsf)->lpvtbl              = (ICOM_VTABLE(IDirectDrawSurface)*)&xlib_dds4vt;
	(*ilpdsf)->s.palette = NULL;
	(*ilpdsf)->t.xlib.image = NULL; /* This is for off-screen buffers */

	if (!(lpddsd->dwFlags & DDSD_WIDTH))
		lpddsd->dwWidth	 = This->d.width;
	if (!(lpddsd->dwFlags & DDSD_HEIGHT))
		lpddsd->dwHeight = This->d.height;

	/* Check if this a 'primary surface' or not */
  if ((lpddsd->dwFlags & DDSD_CAPS) && 
	    (lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
    XImage *img;
      
    TRACE(ddraw,"using standard XImage for a primary surface (%p)\n", *ilpdsf);

	  /* First, store the surface description */
	  (*ilpdsf)->s.surface_desc = *lpddsd;
	  
    /* Create the XImage */
    img = create_ximage(This, (IDirectDrawSurface4Impl*) *ilpdsf);
    if (img == NULL)
      return DDERR_OUTOFMEMORY;
    (*ilpdsf)->t.xlib.image = img;

	  /* Add flags if there were not present */
	  (*ilpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;
	  (*ilpdsf)->s.surface_desc.dwWidth = This->d.width;
	  (*ilpdsf)->s.surface_desc.dwHeight = This->d.height;
	  (*ilpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	  (*ilpdsf)->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;
	  (*ilpdsf)->s.backbuffer = NULL;
    
    /* Check for backbuffers */
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
		IDirectDrawSurface4Impl*	back;
      XImage *img;

		if (lpddsd->dwBackBufferCount>1)
			FIXME(ddraw,"urks, wants to have more than one backbuffer (%ld)!\n",lpddsd->dwBackBufferCount);

      (*ilpdsf)->s.backbuffer = back =
	(IDirectDrawSurface4Impl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawSurface4Impl));

      TRACE(ddraw,"allocated back-buffer (%p)\n", back);
      
		IDirectDraw2_AddRef(iface);
		back->s.ddraw = This;

		back->ref = 1;
		back->lpvtbl = (ICOM_VTABLE(IDirectDrawSurface4)*)&xlib_dds4vt;
	    /* Copy the surface description from the front buffer */
	    back->s.surface_desc = (*ilpdsf)->s.surface_desc;

      /* Create the XImage */
      img = create_ximage(This, back);
      if (img == NULL)
	return DDERR_OUTOFMEMORY;
      back->t.xlib.image = img;
      
		back->s.backbuffer = NULL; /* does not have a backbuffer, it is
					    * one! */

	    /* Add relevant info to front and back buffers */
	    (*ilpdsf)->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_FRONTBUFFER;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;
	    back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
	    back->s.surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	    back->s.surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	}
	} else {
	  /* There is no Xlib-specific code here...
	     Go to the common surface creation function */
	  return common_off_screen_CreateSurface(This, lpddsd, *ilpdsf);
  }
  
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_DuplicateSurface(
	LPDIRECTDRAW2 iface,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME(ddraw,"(%p)->(%p,%p) simply copies\n",This,src,dst);
	*dst = src; /* FIXME */
	return DD_OK;
}

/* 
 * The Xlib Implementation tries to use the passed hwnd as drawing window,
 * even when the approbiate bitmasks are not specified.
 */
static HRESULT WINAPI IDirectDraw2Impl_SetCooperativeLevel(
	LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
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

	FIXME(ddraw,"(%p)->(%08lx,%08lx)\n",This,(DWORD)hwnd,cooplevel);
	if(TRACE_ON(ddraw)){
	  dbg_decl_str(ddraw, 512);
	  for (i=0;i<sizeof(flagmap)/sizeof(flagmap[0]);i++)
	    if (flagmap[i].mask & cooplevel)
	      dsprintf(ddraw, "%s ", flagmap[i].name);
	  TRACE(ddraw,"	cooperative level %s\n", dbg_str(ddraw));
	}
        This->d.mainWindow = hwnd;

	/* This will be overwritten in the case of Full Screen mode.
	   Windowed games could work with that :-) */
	if (hwnd)
        {
            WND *tmpWnd = WIN_FindWndPtr(hwnd);
            This->d.drawable  = X11DRV_WND_GetXWindow(tmpWnd);
            WIN_ReleaseWndPtr(tmpWnd);

	    if( !This->d.drawable ) {
	      This->d.drawable = ((X11DRV_WND_DATA *) WIN_GetDesktop()->pDriverData)->window;
	      WIN_ReleaseDesktop();
        }
	    TRACE(ddraw, "Setting drawable to %ld\n", This->d.drawable);
        }

	return DD_OK;
}

/* Small helper to either use the cooperative window or create a new 
 * one (for mouse and keyboard input) and drawing in the Xlib implementation.
 */
static void _common_IDirectDrawImpl_SetDisplayMode(IDirectDrawImpl* This) {
	RECT	rect;

	/* Do not destroy the application supplied cooperative window */
	if (This->d.window && This->d.window != This->d.mainWindow) {
		DestroyWindow(This->d.window);
		This->d.window = 0;
	}
	/* Sanity check cooperative window before assigning it to drawing. */
	if (	IsWindow(This->d.mainWindow) &&
		IsWindowVisible(This->d.mainWindow)
	) {
		GetWindowRect(This->d.mainWindow,&rect);
		if (((rect.right-rect.left) >= This->d.width)	&&
		    ((rect.bottom-rect.top) >= This->d.height)
		)
			This->d.window = This->d.mainWindow;
	}
	/* ... failed, create new one. */
	if (!This->d.window) {
	    This->d.window = CreateWindowExA(
		    0,
		    "WINE_DirectDraw",
		    "WINE_DirectDraw",
		    WS_VISIBLE|WS_SYSMENU|WS_THICKFRAME,
		    0,0,
		    This->d.width,
		    This->d.height,
		    0,
		    0,
		    0,
		    NULL
	    );
	    /*Store THIS with the window. We'll use it in the window procedure*/
	    SetWindowLongA(This->d.window,ddrawXlibThisOffset,(LONG)This);
	    ShowWindow(This->d.window,TRUE);
	    UpdateWindow(This->d.window);
	}
	SetFocus(This->d.window);
}

static int _common_depth_to_pixelformat(DWORD depth, DDPIXELFORMAT *pixelformat, DDPIXELFORMAT *screen_pixelformat, int *pix_depth) {
  XVisualInfo *vi;
  XPixmapFormatValues *pf;
  XVisualInfo vt;
  int nvisuals, npixmap, i;
  int match = 0;

  vi = TSXGetVisualInfo(display, VisualNoMask, &vt, &nvisuals);
  pf = XListPixmapFormats(display, &npixmap);
  
  for (i = 0; i < npixmap; i++) {
    if (pf[i].bits_per_pixel == depth) {
      int j;
      
      for (j = 0; j < nvisuals; j++) {
	if (vi[j].depth == pf[i].depth) {
	  pixelformat->dwSize = sizeof(*pixelformat);
	  if (depth == 8) {
	    pixelformat->dwFlags = DDPF_PALETTEINDEXED8;
	    pixelformat->y.dwRBitMask = 0;
	    pixelformat->z.dwGBitMask = 0;
	    pixelformat->xx.dwBBitMask = 0;
	  } else {
	    pixelformat->dwFlags = DDPF_RGB;
	    pixelformat->y.dwRBitMask = vi[j].red_mask;
	    pixelformat->z.dwGBitMask = vi[j].green_mask;
	    pixelformat->xx.dwBBitMask = vi[j].blue_mask;
	  }
	  pixelformat->dwFourCC = 0;
	  pixelformat->x.dwRGBBitCount = pf[i].bits_per_pixel;
	  pixelformat->xy.dwRGBAlphaBitMask= 0;

	  *screen_pixelformat = *pixelformat;
	  
	  if (pix_depth != NULL)
	    *pix_depth = vi[j].depth;
	  
	  match = 1;
	  
	  break;
	}
      }

      if (j == nvisuals)
	ERR(ddraw, "No visual corresponding to pixmap format !\n");
    }
  }

  if ((match == 0) && (depth == 8)) {
    pixelformat->dwSize = sizeof(*pixelformat);
    pixelformat->dwFlags = DDPF_PALETTEINDEXED8;
    pixelformat->dwFourCC = 0;
    pixelformat->x.dwRGBBitCount = 8;
    pixelformat->y.dwRBitMask = 0;
    pixelformat->z.dwGBitMask = 0;
    pixelformat->xx.dwBBitMask = 0;
    pixelformat->xy.dwRGBAlphaBitMask= 0;    
    
    /* In that case, find a visual to emulate the 8 bpp format */
    for (i = 0; i < npixmap; i++) {
      if (pf[i].bits_per_pixel >= depth) {
	int j;
	
	for (j = 0; j < nvisuals; j++) {
	  if (vi[j].depth == pf[i].depth) {
	    screen_pixelformat->dwSize = sizeof(*screen_pixelformat);
	    screen_pixelformat->dwFlags = DDPF_RGB;
	    screen_pixelformat->dwFourCC = 0;
	    screen_pixelformat->x.dwRGBBitCount = pf[i].bits_per_pixel;
	    screen_pixelformat->y.dwRBitMask = vi[j].red_mask;
	    screen_pixelformat->z.dwGBitMask = vi[j].green_mask;
	    screen_pixelformat->xx.dwBBitMask = vi[j].blue_mask;
	    screen_pixelformat->xy.dwRGBAlphaBitMask= 0;

	    if (pix_depth != NULL)
	      *pix_depth = vi[j].depth;
	    
	    match = 2;
	    
	    break;
	  }
	}
	
	if (j == nvisuals)
	  ERR(ddraw, "No visual corresponding to pixmap format !\n");
      }
    }
  }
  
  TSXFree(vi);
  TSXFree(pf);

  return match;
}

static HRESULT WINAPI DGA_IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDrawImpl,iface);
        int	i,mode_count;

	TRACE(ddraw, "(%p)->(%ld,%ld,%ld)\n", This, width, height, depth);

	/* We hope getting the asked for depth */
	if (_common_depth_to_pixelformat(depth, &(This->d.directdraw_pixelformat), &(This->d.screen_pixelformat), NULL) != 1) {
	  /* I.e. no visual found or emulated */
		ERR(ddraw,"(w=%ld,h=%ld,d=%ld), unsupported depth!\n",width,height,depth);
		return DDERR_UNSUPPORTEDMODE;
	}
	
	if (This->d.width < width) {
		ERR(ddraw,"SetDisplayMode(w=%ld,h=%ld,d=%ld), width %ld exceeds framebuffer width %ld\n",width,height,depth,width,This->d.width);
		return DDERR_UNSUPPORTEDMODE;
	}
	This->d.width	= width;
	This->d.height	= height;

	/* adjust fb_height, so we don't overlap */
	if (This->e.dga.fb_height < height)
		This->e.dga.fb_height = height;
	_common_IDirectDrawImpl_SetDisplayMode(This);

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
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,This->e.dga.fb_height);
#else
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
#endif

#ifdef RESTORE_SIGNALS
	SIGNAL_InitHandlers();
#endif
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

/* *************************************
      16 / 15 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_16_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned char  *c_src = (unsigned char  *) src;
  unsigned short *c_dst = (unsigned short *) dst;
  int x, y;

  if (palette != NULL) {
    unsigned short *pal = (unsigned short *) palette->screen_palents;

    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	c_dst[x + y * width] = pal[c_src[x + y * pitch]];
      }
    }
  } else {
    WARN(ddraw, "No palette set...\n");
    memset(dst, 0, width * height * 2);
  }
}
static void palette_convert_16_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) {
  int i;
  unsigned short *pal = (unsigned short *) screen_palette;
  
  for (i = 0; i < count; i++)
    pal[start + i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 8) |
		      ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
		      ((((unsigned short) palent[i].peGreen) & 0xFC) << 3));
}
static void palette_convert_15_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) {
  int i;
  unsigned short *pal = (unsigned short *) screen_palette;
  
  for (i = 0; i < count; i++)
    pal[start + i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 7) |
		      ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
		      ((((unsigned short) palent[i].peGreen) & 0xF8) << 2));
}

/* *************************************
      24 / 32 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_32_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned char  *c_src = (unsigned char  *) src;
  unsigned int *c_dst = (unsigned int *) dst;
  int x, y;

  if (palette != NULL) {
    unsigned int *pal = (unsigned int *) palette->screen_palents;
    
    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	c_dst[x + y * width] = pal[c_src[x + y * pitch]];
      }
    }
  } else {
    WARN(ddraw, "No palette set...\n");
    memset(dst, 0, width * height * 4);
  }
}
static void palette_convert_24_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) {
  int i;
  unsigned int *pal = (unsigned int *) screen_palette;
  
  for (i = 0; i < count; i++)
    pal[start + i] = ((((unsigned int) palent[i].peRed) << 16) |
		      (((unsigned int) palent[i].peGreen) << 8) |
		      ((unsigned int) palent[i].peBlue));
}

static HRESULT WINAPI Xlib_IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
        ICOM_THIS(IDirectDrawImpl,iface);
	char	buf[200];
        WND *tmpWnd;

	TRACE(ddraw, "(%p)->SetDisplayMode(%ld,%ld,%ld)\n",
		      This, width, height, depth);

	switch (_common_depth_to_pixelformat(depth,
					     &(This->d.directdraw_pixelformat),
					     &(This->d.screen_pixelformat),
					     &(This->d.pixmap_depth))) {
	case 0:
	  sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
	  MessageBoxA(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
	  return DDERR_UNSUPPORTEDMODE;

	case 1:
	  /* No convertion */
	  This->d.pixel_convert = NULL;
	  This->d.palette_convert = NULL;
	  break;

	case 2: {
	  int found = 0;

	  WARN(ddraw, "Warning : running in depth-convertion mode. Should run using a %ld depth for optimal performances.\n", depth);
	  
	  /* Set the depth convertion routines */
	  switch (This->d.screen_pixelformat.x.dwRGBBitCount) {
	  case 16:
	    if ((This->d.screen_pixelformat.y.dwRBitMask == 0xF800) &&
		(This->d.screen_pixelformat.z.dwGBitMask == 0x07E0) &&
		(This->d.screen_pixelformat.xx.dwBBitMask == 0x001F)) {
	      /* 16 bpp */
	      found = 1;
	      
	      This->d.pixel_convert = pixel_convert_16_to_8;
	      This->d.palette_convert = palette_convert_16_to_8;
	    } else if ((This->d.screen_pixelformat.y.dwRBitMask == 0x7C00) &&
		       (This->d.screen_pixelformat.z.dwGBitMask == 0x03E0) &&
		       (This->d.screen_pixelformat.xx.dwBBitMask == 0x001F)) {
	      /* 15 bpp */
	      found = 1;

	      This->d.pixel_convert = pixel_convert_16_to_8;
	      This->d.palette_convert = palette_convert_15_to_8;
	    }
	    break;
	    
	  case 24:
	    /* Not handled yet :/ */
	    found = 0;
			break;
	    
	  case 32:
	    if ((This->d.screen_pixelformat.y.dwRBitMask ==  0xFF0000) &&
		(This->d.screen_pixelformat.z.dwGBitMask ==  0x00FF00) &&
		(This->d.screen_pixelformat.xx.dwBBitMask == 0x0000FF)) {
	      /* 24 bpp */
	      found = 1;

	      This->d.pixel_convert = pixel_convert_32_to_8;
	      This->d.palette_convert = palette_convert_24_to_8;
	    }
	      break;
	  }

	  if (!found) {
		sprintf(buf,"SetDisplayMode(w=%ld,h=%ld,d=%ld), unsupported depth!",width,height,depth);
		MessageBoxA(0,buf,"WINE DirectDraw",MB_OK|MB_ICONSTOP);
		return DDERR_UNSUPPORTEDMODE;
	  }
	} break;
	}

	This->d.width	= width;
	This->d.height	= height;

	_common_IDirectDrawImpl_SetDisplayMode(This);

        tmpWnd = WIN_FindWndPtr(This->d.window);
	This->d.paintable = 1;
        This->d.drawable  = ((X11DRV_WND_DATA *) tmpWnd->pDriverData)->window;
        WIN_ReleaseWndPtr(tmpWnd);

        /* We don't have a context for this window. Host off the desktop */
        if( !This->d.drawable )
        {
           This->d.drawable = ((X11DRV_WND_DATA *) WIN_GetDesktop()->pDriverData)->window;
            WIN_ReleaseDesktop();
        }
	TRACE(ddraw, "Setting drawable to %ld\n", This->d.drawable);

	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_GetCaps(
	LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);
	caps1->dwVidMemTotal = This->e.dga.fb_memsize;
	caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
	caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	if (caps2) {
		caps2->dwVidMemTotal = This->e.dga.fb_memsize;
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
  caps->dwCaps = DDCAPS_ALPHA | DDCAPS_BLT | DDCAPS_BLTSTRETCH | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL |
    DDCAPS_CANBLTSYSMEM |  DDCAPS_COLORKEY | DDCAPS_PALETTE;
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
      DDSCAPS_OVERLAY | DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY |
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
	TRACE(ddraw,"(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);

	/* Put the same caps for the two capabilities */
	fill_caps(caps1);
	fill_caps(caps2);

	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_CreateClipper(
	LPDIRECTDRAW2 iface,DWORD x,LPDIRECTDRAWCLIPPER *lpddclip,LPUNKNOWN lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawClipperImpl** ilpddclip=(IDirectDrawClipperImpl**)lpddclip;
	FIXME(ddraw,"(%p)->(%08lx,%p,%p),stub!\n",
		This,x,ilpddclip,lpunk
	);
	*ilpddclip = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
	(*ilpddclip)->ref = 1;
	(*ilpddclip)->lpvtbl = &ddclipvt;
	return DD_OK;
}

static HRESULT WINAPI common_IDirectDraw2Impl_CreatePalette(
	IDirectDraw2Impl* This,DWORD dwFlags,LPPALETTEENTRY palent,IDirectDrawPaletteImpl **lpddpal,LPUNKNOWN lpunk,int *psize
) {
	int size = 0;
	  
	if (TRACE_ON(ddraw))
	  _dump_paletteformat(dwFlags);
	
	*lpddpal = (IDirectDrawPaletteImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawPaletteImpl));
	if (*lpddpal == NULL) return E_OUTOFMEMORY;
	(*lpddpal)->ref = 1;
	(*lpddpal)->ddraw = (IDirectDrawImpl*)This;
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
	  if (This->d.palette_convert != NULL)	    
	    This->d.palette_convert(palent, (*lpddpal)->screen_palents, 0, size);

	  memcpy((*lpddpal)->palents, palent, size * sizeof(PALETTEENTRY));
        } else if (This->d.palette_convert != NULL) {
	  /* In that case, put all 0xFF */
	  memset((*lpddpal)->screen_palents, 0xFF, 256 * sizeof(int));
	}
	
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_CreatePalette(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
	HRESULT res;
	int xsize = 0,i;

	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
	res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
	if (res != 0) return res;
	(*ilpddpal)->lpvtbl = &dga_ddpalvt;
	if (This->d.directdraw_pixelformat.x.dwRGBBitCount<=8) {
		(*ilpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);
	} else {
		FIXME(ddraw,"why are we doing CreatePalette in hi/truecolor?\n");
		(*ilpddpal)->cm = 0;
	}
	if (((*ilpddpal)->cm)&&xsize) {
	  for (i=0;i<xsize;i++) {
		  XColor xc;

		  xc.red = (*ilpddpal)->palents[i].peRed<<8;
		  xc.blue = (*ilpddpal)->palents[i].peBlue<<8;
		  xc.green = (*ilpddpal)->palents[i].peGreen<<8;
		  xc.flags = DoRed|DoBlue|DoGreen;
		  xc.pixel = i;
		  TSXStoreColor(display,(*ilpddpal)->cm,&xc);
	  }
	}
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreatePalette(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
	int xsize;
	HRESULT res;

	TRACE(ddraw,"(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
	res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
	if (res != 0) return res;
	(*ilpddpal)->lpvtbl = &xlib_ddpalvt;
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw, "(%p)->()\n",This);
	Sleep(1000);
	TSXF86DGADirectVideo(display,DefaultScreen(display),0);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitHandlers();
#endif
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw, "(%p)->RestoreDisplayMode()\n", This);
	Sleep(1000);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_WaitForVerticalBlank(
	LPDIRECTDRAW2 iface,DWORD x,HANDLE h
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->(0x%08lx,0x%08x)\n",This,x,h);
	return DD_OK;
}

static ULONG WINAPI IDirectDraw2Impl_AddRef(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE( ddraw, "(%p)->() incrementing from %lu.\n", This, This->ref );

	return ++(This->ref);
}

static ULONG WINAPI DGA_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

#ifdef HAVE_LIBXXF86DGA
	if (!--(This->ref)) {
		TSXF86DGADirectVideo(display,DefaultScreen(display),0);
		if (This->d.window && (This->d.mainWindow != This->d.window))
			DestroyWindow(This->d.window);
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
		SIGNAL_InitHandlers();
#endif
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
#endif /* defined(HAVE_LIBXXF86DGA) */
	return This->ref;
}

static ULONG WINAPI Xlib_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE( ddraw, "(%p)->() decrementing from %lu.\n", This, This->ref );

	if (!--(This->ref)) {
		if (This->d.window && (This->d.mainWindow != This->d.window))
			DestroyWindow(This->d.window);
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	/* FIXME: destroy window ... */
	return This->ref;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_QueryInterface(
	LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(ddraw,"(%p)->(%s,%p)\n",This,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		IDirectDraw2_AddRef(iface);

		TRACE(ddraw, "  Creating IUnknown interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&dga_ddvt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&dga_dd2vt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw2 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&dga_dd4vt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw4 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		IDirect3DImpl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D2))) {
		IDirect3D2Impl*	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);
		
		return S_OK;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",This,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_QueryInterface(
	LPDIRECTDRAW2 iface,REFIID refiid,LPVOID *obj
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	char    xrefiid[50];

	WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
	TRACE(ddraw,"(%p)->(%s,%p)\n",This,xrefiid,obj);
	if (!memcmp(&IID_IUnknown,refiid,sizeof(IID_IUnknown))) {
		*obj = This;
		IDirectDraw2_AddRef(iface);

		TRACE(ddraw, "  Creating IUnknown interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw,refiid,sizeof(IID_IDirectDraw))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&xlib_ddvt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw2,refiid,sizeof(IID_IDirectDraw2))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&xlib_dd2vt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw2 interface (%p)\n", *obj);
		
		return S_OK;
	}
	if (!memcmp(&IID_IDirectDraw4,refiid,sizeof(IID_IDirectDraw4))) {
		This->lpvtbl = (ICOM_VTABLE(IDirectDraw2)*)&xlib_dd4vt;
		IDirectDraw2_AddRef(iface);
		*obj = This;

		TRACE(ddraw, "  Creating IDirectDraw4 interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D,refiid,sizeof(IID_IDirect3D))) {
		IDirect3DImpl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		d3d->lpvtbl = &d3dvt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D interface (%p)\n", *obj);

		return S_OK;
	}
	if (!memcmp(&IID_IDirect3D2,refiid,sizeof(IID_IDirect3D))) {
		IDirect3D2Impl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		d3d->lpvtbl = &d3d2vt;
		*obj = d3d;

		TRACE(ddraw, "  Creating IDirect3D2 interface (%p)\n", *obj);

		return S_OK;
	}
	WARN(ddraw,"(%p):interface for IID %s _NOT_ found!\n",This,xrefiid);
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw2Impl_GetVerticalBlankStatus(
	LPDIRECTDRAW2 iface,BOOL *status
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE(ddraw,"(%p)->(%p)\n",This,status);
	*status = TRUE;
	return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_EnumDisplayModes(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
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

	TRACE(ddraw,"(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
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

static HRESULT WINAPI Xlib_IDirectDraw2Impl_EnumDisplayModes(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPDDSURFACEDESC lpddsfd,LPVOID context,LPDDENUMMODESCALLBACK modescb
) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  XVisualInfo *vi;
  XPixmapFormatValues *pf;
  XVisualInfo vt;
  int nvisuals, npixmap, i;
  int send_mode;
  int has_8bpp = 0;
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

  TRACE(ddraw,"(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
  ddsfd.dwSize = sizeof(ddsfd);
  ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_CAPS;
  if (dwFlags & DDEDM_REFRESHRATES) {
    ddsfd.dwFlags |= DDSD_REFRESHRATE;
    ddsfd.x.dwRefreshRate = 60;
  }
  maxWidth = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
  maxHeight = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
  
  vi = TSXGetVisualInfo(display, VisualNoMask, &vt, &nvisuals);
  pf = XListPixmapFormats(display, &npixmap);

  i = 0;
  send_mode = 0;
  while (i < npixmap) {
    if ((has_8bpp == 0) && (pf[i].depth == 8)) {
      /* Special case of a 8bpp depth */
      has_8bpp = 1;
      send_mode = 1;

      ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;
      ddsfd.ddpfPixelFormat.dwSize = sizeof(ddsfd.ddpfPixelFormat);
      ddsfd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8;
      ddsfd.ddpfPixelFormat.dwFourCC = 0;
      ddsfd.ddpfPixelFormat.x.dwRGBBitCount = 8;
      ddsfd.ddpfPixelFormat.y.dwRBitMask = 0;
      ddsfd.ddpfPixelFormat.z.dwGBitMask = 0;
      ddsfd.ddpfPixelFormat.xx.dwBBitMask = 0;
      ddsfd.ddpfPixelFormat.xy.dwRGBAlphaBitMask= 0;
    } else if (pf[i].depth > 8) {
      int j;
      
      /* All the 'true color' depths (15, 16 and 24)
	 First, find the corresponding visual to extract the bit masks */
      for (j = 0; j < nvisuals; j++) {
	if (vi[j].depth == pf[i].depth) {
	  ddsfd.ddsCaps.dwCaps = 0;
	  ddsfd.ddpfPixelFormat.dwSize = sizeof(ddsfd.ddpfPixelFormat);
	  ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	  ddsfd.ddpfPixelFormat.dwFourCC = 0;
	  ddsfd.ddpfPixelFormat.x.dwRGBBitCount = pf[i].bits_per_pixel;
	  ddsfd.ddpfPixelFormat.y.dwRBitMask = vi[j].red_mask;
	  ddsfd.ddpfPixelFormat.z.dwGBitMask = vi[j].green_mask;
	  ddsfd.ddpfPixelFormat.xx.dwBBitMask = vi[j].blue_mask;
	  ddsfd.ddpfPixelFormat.xy.dwRGBAlphaBitMask= 0;

	  send_mode = 1;
	  break;
	}
      }
      
      if (j == nvisuals)
	ERR(ddraw, "Did not find visual corresponding the the pixmap format !\n");
    } else {
      send_mode = 0;
    }

    if (send_mode) {
      int mode;

      if (TRACE_ON(ddraw)) {
	TRACE(ddraw, "Enumerating with pixel format : \n");
	_dump_pixelformat(&(ddsfd.ddpfPixelFormat));
      }
      
      for (mode = 0; mode < sizeof(modes)/sizeof(modes[0]); mode++) {
	/* Do not enumerate modes we cannot handle anyway */
	if ((modes[mode].w > maxWidth) || (modes[mode].h > maxHeight))
	  break;

	ddsfd.dwWidth = modes[mode].w;
	ddsfd.dwHeight = modes[mode].h;
	
	/* Now, send the mode description to the application */
	TRACE(ddraw, " - mode %4ld - %4ld\n", ddsfd.dwWidth, ddsfd.dwHeight);
	if (!modescb(&ddsfd, context))
	  goto exit_enum;
      }

      if (!(dwFlags & DDEDM_STANDARDVGAMODES)) {
	/* modeX is not standard VGA */
	ddsfd.dwWidth = 320;
	ddsfd.dwHeight = 200;
	if (!modescb(&ddsfd, context))
	  goto exit_enum;
      }
    }

    /* Hack to always enumerate a 8bpp depth */
    i++;
    if ((i == npixmap) && (has_8bpp == 0)) {
      i--;
      pf[i].depth = 8;
    }
  }
  
 exit_enum:
  TSXFree(vi);
  TSXFree(pf);

  return DD_OK;
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_GetDisplayMode(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
#ifdef HAVE_LIBXXF86DGA
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->(%p)\n",This,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = This->d.height;
	lpddsfd->dwWidth = This->d.width;
	lpddsfd->lPitch = This->e.dga.fb_width*This->d.directdraw_pixelformat.x.dwRGBBitCount/8;
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	lpddsfd->ddpfPixelFormat = This->d.directdraw_pixelformat;
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return E_UNEXPECTED;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetDisplayMode(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->GetDisplayMode(%p)\n",This,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = This->d.height;
	lpddsfd->dwWidth = This->d.width;
	lpddsfd->lPitch = lpddsfd->dwWidth * This->d.directdraw_pixelformat.x.dwRGBBitCount/8;
	lpddsfd->dwBackBufferCount = 1;
	lpddsfd->x.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	lpddsfd->ddpfPixelFormat = This->d.directdraw_pixelformat;
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_FlipToGDISurface(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->()\n",This);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetMonitorFrequency(
	LPDIRECTDRAW2 iface,LPDWORD freq
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME(ddraw,"(%p)->(%p) returns 60 Hz always\n",This,freq);
	*freq = 60*100; /* 60 Hz */
	return DD_OK;
}

/* what can we directly decompress? */
static HRESULT WINAPI IDirectDraw2Impl_GetFourCCCodes(
	LPDIRECTDRAW2 iface,LPDWORD x,LPDWORD y
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME(ddraw,"(%p,%p,%p), stub\n",This,x,y);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_EnumSurfaces(
	LPDIRECTDRAW2 iface,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME(ddraw,"(%p)->(0x%08lx,%p,%p,%p),stub!\n",This,x,ddsfd,context,ddsfcb);
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_Compact(
          LPDIRECTDRAW2 iface )
{
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME(ddraw,"(%p)->()\n", This );
 
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetGDISurface(LPDIRECTDRAW2 iface,
						 LPDIRECTDRAWSURFACE *lplpGDIDDSSurface) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME(ddraw,"(%p)->(%p)\n", This, lplpGDIDDSSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetScanLine(LPDIRECTDRAW2 iface,
					       LPDWORD lpdwScanLine) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME(ddraw,"(%p)->(%p)\n", This, lpdwScanLine);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_Initialize(LPDIRECTDRAW2 iface,
					      GUID *lpGUID) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME(ddraw,"(%p)->(%p)\n", This, lpGUID);
  
  return DD_OK;
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

static ICOM_VTABLE(IDirectDraw) dga_ddvt = {
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

static ICOM_VTABLE(IDirectDraw) xlib_ddvt = {
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
	XCAST(RestoreDisplayMode)Xlib_IDirectDraw2Impl_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2Impl_SetCooperativeLevel,
	Xlib_IDirectDrawImpl_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
};

#undef XCAST

/*****************************************************************************
 * 	IDirectDraw2
 *
 */


static HRESULT WINAPI DGA_IDirectDraw2Impl_SetDisplayMode(
	LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return DGA_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_SetDisplayMode(
	LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD xx,DWORD yy
) {
	return Xlib_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

static HRESULT WINAPI DGA_IDirectDraw2Impl_GetAvailableVidMem(
	LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		This,ddscaps,total,free
	);
	if (total) *total = This->e.dga.fb_memsize * 1024;
	if (free) *free = This->e.dga.fb_memsize * 1024;
	return DD_OK;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetAvailableVidMem(
	LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE(ddraw,"(%p)->(%p,%p,%p)\n",
		This,ddscaps,total,free
	);
	if (total) *total = 2048 * 1024;
	if (free) *free = 2048 * 1024;
	return DD_OK;
}

static ICOM_VTABLE(IDirectDraw2) dga_dd2vt = {
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

static ICOM_VTABLE(IDirectDraw2) xlib_dd2vt = {
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
	Xlib_IDirectDraw2Impl_RestoreDisplayMode,
	IDirectDraw2Impl_SetCooperativeLevel,
	Xlib_IDirectDraw2Impl_SetDisplayMode,
	IDirectDraw2Impl_WaitForVerticalBlank,
	Xlib_IDirectDraw2Impl_GetAvailableVidMem	
};

/*****************************************************************************
 * 	IDirectDraw4
 *
 */

static HRESULT WINAPI IDirectDraw4Impl_GetSurfaceFromDC(LPDIRECTDRAW4 iface,
						    HDC hdc,
						    LPDIRECTDRAWSURFACE *lpDDS) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME(ddraw, "(%p)->(%08ld,%p)\n", This, (DWORD) hdc, lpDDS);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_RestoreAllSurfaces(LPDIRECTDRAW4 iface) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME(ddraw, "(%p)->()\n", This);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_TestCooperativeLevel(LPDIRECTDRAW4 iface) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME(ddraw, "(%p)->()\n", This);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_GetDeviceIdentifier(LPDIRECTDRAW4 iface,
						       LPDDDEVICEIDENTIFIER lpdddi,
						       DWORD dwFlags) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME(ddraw, "(%p)->(%p,%08lx)\n", This, lpdddi, dwFlags);
  
  return DD_OK;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif


static ICOM_VTABLE(IDirectDraw4) dga_dd4vt = {
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

static ICOM_VTABLE(IDirectDraw4) xlib_dd4vt = {
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
	XCAST(RestoreDisplayMode)Xlib_IDirectDraw2Impl_RestoreDisplayMode,
	XCAST(SetCooperativeLevel)IDirectDraw2Impl_SetCooperativeLevel,
	XCAST(SetDisplayMode)Xlib_IDirectDrawImpl_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
	XCAST(GetAvailableVidMem)Xlib_IDirectDraw2Impl_GetAvailableVidMem,
	IDirectDraw4Impl_GetSurfaceFromDC,
	IDirectDraw4Impl_RestoreAllSurfaces,
	IDirectDraw4Impl_TestCooperativeLevel,
	IDirectDraw4Impl_GetDeviceIdentifier
};

#undef XCAST

/******************************************************************************
 * 				DirectDrawCreate
 */

LRESULT WINAPI Xlib_DDWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   LRESULT ret;
   IDirectDrawImpl* ddraw = NULL;
   DWORD lastError;

   /* FIXME(ddraw,"(0x%04x,%s,0x%08lx,0x%08lx),stub!\n",(int)hwnd,SPY_GetMsgName(msg),(long)wParam,(long)lParam); */

   SetLastError( ERROR_SUCCESS );
   ddraw  = (IDirectDrawImpl*)GetWindowLongA( hwnd, ddrawXlibThisOffset );
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
	ret = DefWindowProcA( ddraw->d.mainWindow, msg, wParam, lParam );

        if( !ret )
        {
          WND *tmpWnd =WIN_FindWndPtr(ddraw->d.mainWindow);
          /* We didn't handle the message - give it to the application */
          if (ddraw && ddraw->d.mainWindow && tmpWnd)
          {
          	ret = CallWindowProcA(tmpWnd->winproc,
               	                   ddraw->d.mainWindow, msg, wParam, lParam );
	  }
          WIN_ReleaseWndPtr(tmpWnd);

        }
        
      } else {
        ret = DefWindowProcA(hwnd, msg, wParam, lParam );
      } 

    }
    else
    {
	ret = DefWindowProcA(hwnd,msg,wParam,lParam);
    }

    return ret;
}

HRESULT WINAPI DGA_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
#ifdef HAVE_LIBXXF86DGA
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	int	memsize,banksize,width,major,minor,flags,height;
	char	*addr;
	int     fd;             	
	int     depth;

        /* Must be able to access /dev/mem for DGA extensions to work, root is not neccessary. --stephenc */
        if ((fd = open("/dev/mem", O_RDWR)) != -1)
	  close(fd);
	
	if (fd  == -1) {
	  MSG("Must be able to access /dev/mem to use XF86DGA!\n");
	  MessageBoxA(0,"Using the XF86DGA extension requires access to /dev/mem.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
	  return E_UNEXPECTED;
	}
	if (!DDRAW_DGA_Available()) {
	        TRACE(ddraw,"No XF86DGA detected.\n");
	        return DDERR_GENERIC;
	}
	*ilplpDD = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
	(*ilplpDD)->lpvtbl = &dga_ddvt;
	(*ilplpDD)->ref = 1;
	TSXF86DGAQueryVersion(display,&major,&minor);
	TRACE(ddraw,"XF86DGA is version %d.%d\n",major,minor);
	TSXF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	if (!(flags & XF86DGADirectPresent))
		MSG("direct video is NOT PRESENT.\n");
	TSXF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	TRACE(ddraw,"video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
	);
	(*ilplpDD)->e.dga.fb_width = width;
	(*ilplpDD)->d.width = width;
	(*ilplpDD)->e.dga.fb_addr = addr;
	(*ilplpDD)->e.dga.fb_memsize = memsize;
	(*ilplpDD)->e.dga.fb_banksize = banksize;

	TSXF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
	(*ilplpDD)->e.dga.fb_height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
#ifdef DIABLO_HACK
	(*ilplpDD)->e.dga.vpmask = 1;
#else
	(*ilplpDD)->e.dga.vpmask = 0;
#endif

	/* just assume the default depth is the DGA depth too */
	depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	_common_depth_to_pixelformat(depth, &((*ilplpDD)->d.directdraw_pixelformat), &((*ilplpDD)->d.screen_pixelformat), NULL);
#ifdef RESTORE_SIGNALS
	SIGNAL_InitHandlers();
#endif

	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return DDERR_INVALIDDIRECTDRAWGUID;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

BOOL
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
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	int depth;

	*ilplpDD = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
	(*ilplpDD)->lpvtbl = &xlib_ddvt;
	(*ilplpDD)->ref = 1;
	(*ilplpDD)->d.drawable = 0; /* in SetDisplayMode */

	/* At DirectDraw creation, the depth is the default depth */
	depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	_common_depth_to_pixelformat(depth,
				     &((*ilplpDD)->d.directdraw_pixelformat),
				     &((*ilplpDD)->d.screen_pixelformat),
				     &((*ilplpDD)->d.pixmap_depth));
	(*ilplpDD)->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	(*ilplpDD)->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);

#ifdef HAVE_LIBXXSHM
	/* Test if XShm is available. */
	if (((*ilplpDD)->e.xlib.xshm_active = DDRAW_XSHM_Available()))
	  TRACE(ddraw, "Using XShm extension.\n");
#endif
	
	return DD_OK;
}

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	char	xclsid[50];
	WNDCLASSA	wc;
        /* WND*            pParentWindow; */
	HRESULT ret;

	if (HIWORD(lpGUID))
		WINE_StringFromCLSID(lpGUID,xclsid);
	else {
		sprintf(xclsid,"<guid-0x%08x>",(int)lpGUID);
		lpGUID = NULL;
	}

	TRACE(ddraw,"(%s,%p,%p)\n",xclsid,ilplpDD,pUnkOuter);

	if ((!lpGUID) ||
	    (!memcmp(lpGUID,&IID_IDirectDraw ,sizeof(IID_IDirectDraw ))) ||
	    (!memcmp(lpGUID,&IID_IDirectDraw2,sizeof(IID_IDirectDraw2))) ||
	    (!memcmp(lpGUID,&IID_IDirectDraw4,sizeof(IID_IDirectDraw4)))) {
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
        /*
         This code is not useful since hInstance is forced to 0 afterward
        pParentWindow   = WIN_GetDesktop();
	wc.hInstance	= pParentWindow ? pParentWindow->hwndSelf : 0;
        */
        wc.hInstance    = 0; 

        
	wc.hIcon	= 0;
	wc.hCursor	= (HCURSOR)IDC_ARROWA;
	wc.hbrBackground= NULL_BRUSH;
	wc.lpszMenuName	= 0;
	wc.lpszClassName= "WINE_DirectDraw";
	RegisterClassA(&wc);

	if (!memcmp(lpGUID, &DGA_DirectDraw_GUID, sizeof(GUID)))
		ret = DGA_DirectDrawCreate(lplpDD, pUnkOuter);
	else if (!memcmp(lpGUID, &XLIB_DirectDraw_GUID, sizeof(GUID)))
		ret = Xlib_DirectDrawCreate(lplpDD, pUnkOuter);
	else
	  goto err;


	(*ilplpDD)->d.winclass = RegisterClassA(&wc);
	return ret;

      err:
	ERR(ddraw, "DirectDrawCreate(%s,%p,%p): did not recognize requested GUID\n",xclsid,lplpDD,pUnkOuter);
	return DDERR_INVALIDDIRECTDRAWGUID;
}


#else /* !defined(X_DISPLAY_MISSING) */

#include "windef.h"

#define DD_OK 0

typedef void *LPGUID;
typedef void *LPUNKNOWN;
typedef void *LPDIRECTDRAW;
typedef void *LPDIRECTDRAWCLIPPER;
typedef void *LPDDENUMCALLBACKA;
typedef void *LPDDENUMCALLBACKEXA;
typedef void *LPDDENUMCALLBACKEXW;
typedef void *LPDDENUMCALLBACKW;

HRESULT WINAPI DSoundHelp(DWORD x, DWORD y, DWORD z) 
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawCreate(
  LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) 
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawCreateClipper(
  DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, LPUNKNOWN pUnkOuter)
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) 
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  return DD_OK;
}

HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) 
{
  return DD_OK;
}

#endif /* !defined(X_DISPLAY_MISSING) */


/*******************************************************************************
 * DirectDraw ClassFactory
 *
 *  Heavily inspired (well, can you say completely copied :-) ) from DirectSound
 *
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IClassFactory)* lpvtbl;
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI 
DDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);
	char buf[80];

	if (HIWORD(riid))
	    WINE_StringFromCLSID(riid,buf);
	else
	    sprintf(buf,"<guid-0x%04x>",LOWORD(riid));
	FIXME(ddraw,"(%p)->(%s,%p),stub!\n",This,buf,ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
DDCF_AddRef(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI DDCF_Release(LPCLASSFACTORY iface) {
	ICOM_THIS(IClassFactoryImpl,iface);
	/* static class, won't be  freed */
	return --(This->ref);
}

static HRESULT WINAPI DDCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IClassFactoryImpl,iface);
	char buf[80];

	WINE_StringFromCLSID(riid,buf);
	TRACE(ddraw,"(%p)->(%p,%s,%p)\n",This,pOuter,buf,ppobj);
	if ((!memcmp(riid,&IID_IDirectDraw ,sizeof(IID_IDirectDraw ))) ||
	    (!memcmp(riid,&IID_IDirectDraw2,sizeof(IID_IDirectDraw2))) ||
	    (!memcmp(riid,&IID_IDirectDraw4,sizeof(IID_IDirectDraw4)))) {
		/* FIXME: reuse already created DirectDraw if present? */
		return DirectDrawCreate((LPGUID) riid,(LPDIRECTDRAW*)ppobj,pOuter);
	}
	return E_NOINTERFACE;
}

static HRESULT WINAPI DDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME(ddraw,"(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DDCF_Vtbl = {
	DDCF_QueryInterface,
	DDCF_AddRef,
	DDCF_Release,
	DDCF_CreateInstance,
	DDCF_LockServer
};
static IClassFactoryImpl DDRAW_CF = {&DDCF_Vtbl, 1 };

/*******************************************************************************
 * DllGetClassObject [DDRAW.13]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI DDRAW_DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
    char buf[80],xbuf[80];

    if (HIWORD(rclsid))
    	WINE_StringFromCLSID(rclsid,xbuf);
    else
    	sprintf(xbuf,"<guid-0x%04x>",LOWORD(rclsid));
    if (HIWORD(riid))
    	WINE_StringFromCLSID(riid,buf);
    else
    	sprintf(buf,"<guid-0x%04x>",LOWORD(riid));
    WINE_StringFromCLSID(riid,xbuf);
    TRACE(ddraw, "(%p,%p,%p)\n", xbuf, buf, ppv);
    if (!memcmp(riid,&IID_IClassFactory,sizeof(IID_IClassFactory))) {
    	*ppv = (LPVOID)&DDRAW_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }
    FIXME(ddraw, "(%p,%p,%p): no interface found.\n", xbuf, buf, ppv);
    return E_NOINTERFACE;
}


/*******************************************************************************
 * DllCanUnloadNow [DDRAW.12]  Determines whether the DLL is in use.
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
DWORD WINAPI DDRAW_DllCanUnloadNow(void)
{
    FIXME(ddraw, "(void): stub\n");
    return S_FALSE;
}
