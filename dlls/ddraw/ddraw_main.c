/*		DirectDraw using DGA or Xlib(XSHM)
 *
 * Copyright 1997-1999 Marcus Meissner
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
#include "winerror.h"

#ifndef X_DISPLAY_MISSING 

#include "ts_xlib.h"
#include "ts_xutil.h"

#ifdef HAVE_LIBXXSHM
# include <sys/types.h>
# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif
# include "ts_xshm.h"
#endif /* defined(HAVE_LIBXXSHM) */

#ifdef HAVE_LIBXXF86DGA
#include "ts_xf86dga.h"
#endif /* defined(HAVE_LIBXXF86DGA) */

#ifdef HAVE_LIBXXF86DGA2
#include "ts_xf86dga2.h"
#endif /* defined(HAVE_LIBXXF86DGA2) */

#ifdef HAVE_LIBXXF86VM
#include "ts_xf86vmode.h"
#endif /* defined(HAVE_LIBXXF86VM) */

#include "x11drv.h"

#include <unistd.h>
#include <assert.h>
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "gdi.h"
#include "heap.h"
#include "dc.h"
#include "win.h"
#include "wine/exception.h"
#include "ddraw.h"
#include "d3d.h"
#include "debugtools.h"
#include "spy.h"
#include "message.h"
#include "options.h"
#include "monitor.h"

static char *ddProp = "WINE_DDRAW_Property";

/* This for all the enumeration and creation of D3D-related objects */
#include "ddraw_private.h"
#include "d3d_private.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/* Restore signal handlers overwritten by XF86DGA 
 */
#define RESTORE_SIGNALS

/* Get DDSCAPS of surface (shortcutmacro) */
#define SDDSCAPS(iface) ((iface)->s.surface_desc.ddsCaps.dwCaps)

/* Get the number of bytes per pixel for a given surface */
#define PFGET_BPP(pf) (pf.dwFlags&DDPF_PALETTEINDEXED8?1:(pf.u.dwRGBBitCount/8))

#define GET_BPP(desc) PFGET_BPP(desc.ddpfPixelFormat)

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

#ifdef HAVE_LIBXXF86DGA
static struct ICOM_VTABLE(IDirectDrawSurface4)	dga_dds4vt;
static struct ICOM_VTABLE(IDirectDraw)		dga_ddvt;
static struct ICOM_VTABLE(IDirectDraw2)		dga_dd2vt;
static struct ICOM_VTABLE(IDirectDraw4)		dga_dd4vt;
static struct ICOM_VTABLE(IDirectDrawPalette)	dga_ddpalvt;
#endif /* defined(HAVE_LIBXXF86DGA) */

#ifdef HAVE_LIBXXF86DGA2
static struct ICOM_VTABLE(IDirectDrawSurface4) dga2_dds4vt;
#endif /* defined(HAVE_LIBXXF86DGA2) */

static struct ICOM_VTABLE(IDirectDrawSurface4)	xlib_dds4vt;
static struct ICOM_VTABLE(IDirectDraw)		xlib_ddvt;
static struct ICOM_VTABLE(IDirectDraw2)		xlib_dd2vt;
static struct ICOM_VTABLE(IDirectDraw4)		xlib_dd4vt;
static struct ICOM_VTABLE(IDirectDrawPalette)	xlib_ddpalvt;

static struct ICOM_VTABLE(IDirectDrawClipper)	ddclipvt;
static struct ICOM_VTABLE(IDirect3D)		d3dvt;
static struct ICOM_VTABLE(IDirect3D2)		d3d2vt;

/* This is for mode-emulation */

static void pixel_convert_16_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) ;
static void palette_convert_16_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) ;
static void palette_convert_15_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) ;
static void pixel_convert_24_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) ;
static void pixel_convert_32_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) ;
static void palette_convert_24_to_8(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count) ;
static void pixel_convert_32_to_16(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) ;

typedef struct {
  unsigned short bpp;
  unsigned short depth;
  unsigned int rmask;
  unsigned int gmask;
  unsigned int bmask;
} ConvertMode;

typedef struct {
  void (*pixel_convert)(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette);
  void (*palette_convert)(LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count);
} ConvertFuncs;

typedef struct {
  ConvertMode screen, dest;
  ConvertFuncs funcs;
} Convert;

static Convert ModeEmulations[] = {
  { { 32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_32_to_8,  palette_convert_24_to_8 } },
  { { 32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, {  16, 16, 0xF800, 0x07E0, 0x001F }, { pixel_convert_32_to_16, NULL } },
  { { 24, 24,   0xFF0000,   0x00FF00,   0x0000FF }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_24_to_8,  palette_convert_24_to_8 } },
  { { 16, 16,     0xF800,     0x07E0,     0x001F }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_16_to_8,  palette_convert_16_to_8 } },
  { { 16, 15,     0x7C00,     0x03E0,     0x001F }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_16_to_8,  palette_convert_15_to_8 } },
};

#ifdef HAVE_LIBXXF86VM
static XF86VidModeModeInfo *orig_mode = NULL;
#endif

#ifdef HAVE_LIBXXSHM
static int XShmErrorFlag = 0;
#endif

static inline BOOL get_option( const char *name, BOOL def )
{
    return PROFILE_GetWineIniBool( "x11drv", name, def );
}

static BYTE
DDRAW_DGA_Available(void)
{
#ifdef HAVE_LIBXXF86DGA
	int fd, evbase, evret, majver, minver;
	static BYTE return_value = 0xFF;

	/* This prevents from probing X times for DGA */
	if (return_value != 0xFF)
	  return return_value;
	
   	if (!get_option( "UseDGA", 1 )) {
	  return_value = 0;
     	  return 0;
	}

	/* First, query the extenstion and its version */
	if (!TSXF86DGAQueryExtension(display,&evbase,&evret)) {
	  return_value = 0;
	  return 0;
	}

	if (!TSXF86DGAQueryVersion(display,&majver,&minver)) {
	  return_value = 0;
	  return 0;
	}

#ifdef HAVE_LIBXXF86DGA2
	if (majver >= 2) {
	  /* We have DGA 2.0 available ! */
	  if (TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    return_value = 2;
	  } else {
	    return_value = 0;
	  }

	  return return_value;
	} else {
#endif /* defined(HAVE_LIBXXF86DGA2) */
	
	  /* You don't have to be root to use DGA extensions. Simply having access to /dev/mem will do the trick */
	  /* This can be achieved by adding the user to the "kmem" group on Debian 2.x systems, don't know about */
	  /* others. --stephenc */
	  if ((fd = open("/dev/mem", O_RDWR)) != -1)
	    close(fd);

	  if (fd != -1)
	    return_value = 1;
	  else
	    return_value = 0;

	  return return_value;
#ifdef HAVE_LIBXXF86DGA2  
	}
#endif /* defined(HAVE_LIBXXF86DGA2) */
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
  TRACE("(%p,%p, %08lx)\n", lpCallback, lpContext, dwFlags);
  
  if (TRACE_ON(ddraw)) {
    DPRINTF("  Flags : ");
    if (dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES)
      DPRINTF("DDENUM_ATTACHEDSECONDARYDEVICES ");
    if (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES)
      DPRINTF("DDENUM_DETACHEDSECONDARYDEVICES ");
    if (dwFlags & DDENUM_NONDISPLAYDEVICES)
      DPRINTF("DDENUM_NONDISPLAYDEVICES ");
    DPRINTF("\n");
  }

  if (dwFlags == DDENUM_NONDISPLAYDEVICES) {
    /* For the moment, Wine does not support any 3D only accelerators */
    return DD_OK;
  }
  if (dwFlags == DDENUM_NONDISPLAYDEVICES) {
    /* For the moment, Wine does not support any attached secondary devices */
    return DD_OK;
  }
  
  if (DDRAW_DGA_Available()) {
    TRACE("Enumerating DGA interface\n");
    if (!lpCallback(&DGA_DirectDraw_GUID, "WINE with XFree86 DGA", "display", lpContext, 0))
      return DD_OK;
  }

  TRACE("Enumerating Xlib interface\n");
  if (!lpCallback(&XLIB_DirectDraw_GUID, "WINE with Xlib", "display", lpContext, 0))
    return DD_OK;
  
  TRACE("Enumerating Default interface\n");
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
	FIXME("(0x%08lx,0x%08lx,0x%08lx),stub!\n",x,y,z);
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
#undef FE
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
	   if (flags[i].mask & flagmask) {
	      DPRINTF("%s ",flags[i].name);
	      
	   };
	DPRINTF("\n");
	
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
#undef FE
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DPRINTF("%s ",flags[i].name);
	DPRINTF("\n");
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
#undef FE
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DPRINTF("%s ",flags[i].name);
	DPRINTF("\n");
}

static void _dump_DDSCAPS(void *in) {
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
#undef FE
	};
	DWORD flagmask = *((DWORD *) in);
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DPRINTF("%s ",flags[i].name);
}

static void _dump_pixelformat_flag(DWORD flagmask) {
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
#undef FE
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & flagmask)
			DPRINTF("%s ",flags[i].name);
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
#undef FE
  };
  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & dwFlags)
      DPRINTF("%s ",flags[i].name);
  DPRINTF("\n");
}

static void _dump_pixelformat(void *in) {
  LPDDPIXELFORMAT pf = (LPDDPIXELFORMAT) in;
  char *cmd;
  
  DPRINTF("( ");
  _dump_pixelformat_flag(pf->dwFlags);
  if (pf->dwFlags & DDPF_FOURCC) {
    DPRINTF(", dwFourCC : %ld", pf->dwFourCC);
  }
  if (pf->dwFlags & DDPF_RGB) {
    DPRINTF(", RGB bits: %ld, ", pf->u.dwRGBBitCount);
    switch (pf->u.dwRGBBitCount) {
    case 4:
      cmd = "%1lx";
      break;
    case 8:
      cmd = "%02lx";
      break;
    case 16:
      cmd = "%04lx";
      break;
    case 24:
      cmd = "%06lx";
      break;
    case 32:
      cmd = "%08lx";
      break;
    default:
      ERR("Unexpected bit depth !\n");
      cmd = "%d";
    }
    DPRINTF(" R "); DPRINTF(cmd, pf->u1.dwRBitMask);
    DPRINTF(" G "); DPRINTF(cmd, pf->u2.dwGBitMask);
    DPRINTF(" B "); DPRINTF(cmd, pf->u3.dwBBitMask);
    if (pf->dwFlags & DDPF_ALPHAPIXELS) {
      DPRINTF(" A "); DPRINTF(cmd, pf->u4.dwRGBAlphaBitMask);
    }
    if (pf->dwFlags & DDPF_ZPIXELS) {
      DPRINTF(" Z "); DPRINTF(cmd, pf->u4.dwRGBZBitMask);
}
  }
  if (pf->dwFlags & DDPF_ZBUFFER) {
    DPRINTF(", Z bits : %ld", pf->u.dwZBufferBitDepth);
  }
  if (pf->dwFlags & DDPF_ALPHA) {
    DPRINTF(", Alpha bits : %ld", pf->u.dwAlphaBitDepth);
  }
  DPRINTF(")");
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
#undef FE
	};
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
		if (flags[i].mask & ck)
			DPRINTF("%s ",flags[i].name);
}

static void _dump_DWORD(void *in) {
  DPRINTF("%ld", *((DWORD *) in));
}
static void _dump_PTR(void *in) {
  DPRINTF("%p", *((void **) in));
}
static void _dump_DDCOLORKEY(void *in) {
  DDCOLORKEY *ddck = (DDCOLORKEY *) in;

  DPRINTF(" Low : %ld  - High : %ld", ddck->dwColorSpaceLowValue, ddck->dwColorSpaceHighValue);
}

static void _dump_surface_desc(DDSURFACEDESC *lpddsd) {
  int	i;
  struct {
    DWORD	mask;
    char	*name;
    void (*func)(void *);
    void        *elt;
  } flags[16], *fe = flags;
#define FE(x,f,e) do { fe->mask = x;  fe->name = #x; fe->func = f; fe->elt = (void *) &(lpddsd->e); fe++; } while(0)
    FE(DDSD_CAPS, _dump_DDSCAPS, ddsCaps);
    FE(DDSD_HEIGHT, _dump_DWORD, dwHeight);
    FE(DDSD_WIDTH, _dump_DWORD, dwWidth);
    FE(DDSD_PITCH, _dump_DWORD, lPitch);
    FE(DDSD_BACKBUFFERCOUNT, _dump_DWORD, dwBackBufferCount);
    FE(DDSD_ZBUFFERBITDEPTH, _dump_DWORD, u.dwZBufferBitDepth);
    FE(DDSD_ALPHABITDEPTH, _dump_DWORD, dwAlphaBitDepth);
    FE(DDSD_PIXELFORMAT, _dump_pixelformat, ddpfPixelFormat);
    FE(DDSD_CKDESTOVERLAY, _dump_DDCOLORKEY, ddckCKDestOverlay);
    FE(DDSD_CKDESTBLT, _dump_DDCOLORKEY, ddckCKDestBlt);
    FE(DDSD_CKSRCOVERLAY, _dump_DDCOLORKEY, ddckCKSrcOverlay);
    FE(DDSD_CKSRCBLT, _dump_DDCOLORKEY, ddckCKSrcBlt);
    FE(DDSD_MIPMAPCOUNT, _dump_DWORD, u.dwMipMapCount);
    FE(DDSD_REFRESHRATE, _dump_DWORD, u.dwRefreshRate);
    FE(DDSD_LINEARSIZE, _dump_DWORD, u1.dwLinearSize);
    FE(DDSD_LPSURFACE, _dump_PTR, u1.lpSurface);
#undef FE

  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & lpddsd->dwFlags) {
      DPRINTF(" - %s : ",flags[i].name);
      flags[i].func(flags[i].elt);
	DPRINTF("\n");  
}
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
        TRACE("(%p)->Lock(%p,%p,%08lx,%08lx)\n",
		This,lprect,lpddsd,flags,(DWORD)hnd);
	if (flags & ~(DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_WRITEONLY))
	    WARN("(%p)->Lock(%p,%p,%08lx,%08lx)\n",
			 This,lprect,lpddsd,flags,(DWORD)hnd);

	/* First, copy the Surface description */
	*lpddsd = This->s.surface_desc;
	TRACE("locked surface: height=%ld, width=%ld, pitch=%ld\n",
	      lpddsd->dwHeight,lpddsd->dwWidth,lpddsd->lPitch);

	/* If asked only for a part, change the surface pointer */
	if (lprect) {
		TRACE("	lprect: %dx%d-%dx%d\n",
			lprect->top,lprect->left,lprect->bottom,lprect->right
		);
		if ((lprect->top < 0) ||
		    (lprect->left < 0) ||
		    (lprect->bottom < 0) ||
		    (lprect->right < 0)) {
		  ERR(" Negative values in LPRECT !!!\n");
		  return DDERR_INVALIDPARAMS;
               }
               
		lpddsd->u1.lpSurface = (LPVOID) ((char *) This->s.surface_desc.u1.lpSurface +
			(lprect->top*This->s.surface_desc.lPitch) +
			lprect->left*GET_BPP(This->s.surface_desc));
	} else {
		assert(This->s.surface_desc.u1.lpSurface);
	}

	/* wait for any previous operations to complete */
#ifdef HAVE_LIBXXSHM
        if (This->t.xlib.image && (SDDSCAPS(This) & DDSCAPS_VISIBLE) &&
	    This->s.ddraw->e.xlib.xshm_active) {
/*
	  int compl = InterlockedExchange( &This->s.ddraw->e.xlib.xshm_compl, 0 );
          if (compl) X11DRV_EVENT_WaitShmCompletion( compl );
*/
          X11DRV_EVENT_WaitShmCompletions( This->s.ddraw->d.drawable );
	}
#endif
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_Unlock(
	LPDIRECTDRAWSURFACE4 iface,LPVOID surface
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->Unlock(%p)\n",This,surface);
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static void Xlib_copy_surface_on_screen(IDirectDrawSurface4Impl* This) {
  if (This->s.ddraw->d.pixel_convert != NULL)
    This->s.ddraw->d.pixel_convert(This->s.surface_desc.u1.lpSurface,
				   This->t.xlib.image->data,
				   This->s.surface_desc.dwWidth,
				   This->s.surface_desc.dwHeight,
				   This->s.surface_desc.lPitch,
				   This->s.palette);

#ifdef HAVE_LIBXXSHM
    if (This->s.ddraw->e.xlib.xshm_active) {
/*
      X11DRV_EVENT_WaitReplaceShmCompletion( &This->s.ddraw->e.xlib.xshm_compl, This->s.ddraw->d.drawable );
*/
      /* let WaitShmCompletions track 'em for now */
      /* (you may want to track it again whenever you implement DX7's partial surface locking,
          where threads have concurrent access) */
      X11DRV_EVENT_PrepareShmCompletion( This->s.ddraw->d.drawable );
      TSXShmPutImage(display,
		     This->s.ddraw->d.drawable,
		     DefaultGCOfScreen(X11DRV_GetXScreen()),
		     This->t.xlib.image,
		     0, 0, 0, 0,
		     This->t.xlib.image->width,
		     This->t.xlib.image->height,
		     True);
      /* make sure the image is transferred ASAP */
      TSXFlush(display);
    }
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
    TRACE("(%p)->Unlock(%p)\n",This,surface);

    if (!This->s.ddraw->d.paintable)
	return DD_OK;

    /* Only redraw the screen when unlocking the buffer that is on screen */
    if (This->t.xlib.image && (SDDSCAPS(This) & DDSCAPS_VISIBLE)) {
	Xlib_copy_surface_on_screen(This);

	if (This->s.palette && This->s.palette->cm)
	    TSXSetWindowColormap(display,This->s.ddraw->d.drawable,This->s.palette->cm);
    }
    return DD_OK;
}

static IDirectDrawSurface4Impl* _common_find_flipto(
	IDirectDrawSurface4Impl* This,IDirectDrawSurface4Impl* flipto
) {
    int	i,j,flipable=0;
    struct _surface_chain	*chain = This->s.chain;

    /* if there was no override flipto, look for current backbuffer */
    if (!flipto) {
	/* walk the flip chain looking for backbuffer */
	for (i=0;i<chain->nrofsurfaces;i++) {
	    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FLIP)
	    	flipable++;
	    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_BACKBUFFER)
		flipto = chain->surfaces[i];
	}
	/* sanity checks ... */
	if (!flipto) {
	    if (flipable>1) {
		for (i=0;i<chain->nrofsurfaces;i++)
		    if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FRONTBUFFER)
		    	break;
		if (i==chain->nrofsurfaces) {
		    /* we do not have a frontbuffer either */
		    for (i=0;i<chain->nrofsurfaces;i++)
			if (SDDSCAPS(chain->surfaces[i]) & DDSCAPS_FLIP) {
			    SDDSCAPS(chain->surfaces[i])|=DDSCAPS_FRONTBUFFER;
			    break;
			}
		    for (j=i+1;j<i+chain->nrofsurfaces+1;j++) {
		    	int k = j % chain->nrofsurfaces;
			if (SDDSCAPS(chain->surfaces[k]) & DDSCAPS_FLIP) {
			    SDDSCAPS(chain->surfaces[k])|=DDSCAPS_BACKBUFFER;
			    flipto = chain->surfaces[k];
			    break;
			}
		    }
		}
	    }
	    if (!flipto)
		flipto = This;
	}
	TRACE("flipping to %p\n",flipto);
    }
    return flipto;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_Flip(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;
    DWORD	xheight;
    LPBYTE	surf;

    TRACE("(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);
    iflipto = _common_find_flipto(This,iflipto);

    /* and flip! */
    TSXF86DGASetViewPort(display,DefaultScreen(display),0,iflipto->t.dga.fb_height);
    if (iflipto->s.palette && iflipto->s.palette->cm)
	    TSXF86DGAInstallColormap(display,DefaultScreen(display),iflipto->s.palette->cm);
    while (!TSXF86DGAViewPortChanged(display,DefaultScreen(display),2)) {

    }
    /* We need to switch the lowlevel surfaces, for DGA this is: */

    /* The height within the framebuffer */
    xheight			= This->t.dga.fb_height;
    This->t.dga.fb_height	= iflipto->t.dga.fb_height;
    iflipto->t.dga.fb_height	= xheight;

    /* And the assciated surface pointer */
    surf				= This->s.surface_desc.u1.lpSurface;
    This->s.surface_desc.u1.lpSurface	= iflipto->s.surface_desc.u1.lpSurface;
    iflipto->s.surface_desc.u1.lpSurface	= surf;

    return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

#ifdef HAVE_LIBXXF86DGA2
static HRESULT WINAPI DGA2_IDirectDrawSurface4Impl_Flip(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;
    DWORD	xheight;
    LPBYTE	surf;

    TRACE("(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);
    iflipto = _common_find_flipto(This,iflipto);

    /* and flip! */
    TSXDGASetViewport(display,DefaultScreen(display),0,iflipto->t.dga.fb_height, XDGAFlipRetrace);
    TSXDGASync(display,DefaultScreen(display));
    TSXFlush(display);
    if (iflipto->s.palette && iflipto->s.palette->cm)
	    TSXDGAInstallColormap(display,DefaultScreen(display),iflipto->s.palette->cm);
    /* We need to switch the lowlevel surfaces, for DGA this is: */

    /* The height within the framebuffer */
    xheight			= This->t.dga.fb_height;
    This->t.dga.fb_height	= iflipto->t.dga.fb_height;
    iflipto->t.dga.fb_height	= xheight;

    /* And the assciated surface pointer */
    surf	                         = This->s.surface_desc.u1.lpSurface;
    This->s.surface_desc.u1.lpSurface    = iflipto->s.surface_desc.u1.lpSurface;
    iflipto->s.surface_desc.u1.lpSurface = surf;

    return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA2) */

static HRESULT WINAPI Xlib_IDirectDrawSurface4Impl_Flip(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 flipto,DWORD dwFlags
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    XImage	*image;
    LPBYTE	surf;
    IDirectDrawSurface4Impl* iflipto=(IDirectDrawSurface4Impl*)flipto;

    TRACE("(%p)->Flip(%p,%08lx)\n",This,iflipto,dwFlags);
    iflipto = _common_find_flipto(This,iflipto);

#if defined(HAVE_MESAGL) && 0 /* does not work */
	if (This->s.d3d_device || (iflipto && iflipto->s.d3d_device)) {
	  TRACE(" - OpenGL flip\n");
	  ENTER_GL();
	  glXSwapBuffers(display, This->s.ddraw->d.drawable);
	  LEAVE_GL();

	  return DD_OK;
	}
#endif /* defined(HAVE_MESAGL) */
	
    if (!This->s.ddraw->d.paintable)
	    return DD_OK;

    /* We need to switch the lowlevel surfaces, for xlib this is: */
    /* The surface pointer */
    surf				= This->s.surface_desc.u1.lpSurface;
    This->s.surface_desc.u1.lpSurface	= iflipto->s.surface_desc.u1.lpSurface;
    iflipto->s.surface_desc.u1.lpSurface	= surf;
    /* the associated ximage */
    image				= This->t.xlib.image;
    This->t.xlib.image			= iflipto->t.xlib.image;
    iflipto->t.xlib.image		= image;

#ifdef HAVE_LIBXXSHM
    if (This->s.ddraw->e.xlib.xshm_active) {
/*
      int compl = InterlockedExchange( &This->s.ddraw->e.xlib.xshm_compl, 0 );
      if (compl) X11DRV_EVENT_WaitShmCompletion( compl );
*/
      X11DRV_EVENT_WaitShmCompletions( This->s.ddraw->d.drawable );
    }
#endif
    Xlib_copy_surface_on_screen(This);

    if (iflipto->s.palette && iflipto->s.palette->cm)
	TSXSetWindowColormap(display,This->s.ddraw->d.drawable,iflipto->s.palette->cm);
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
	TRACE("(%p)->(%p)\n",This,ipal);

	if (ipal == NULL) {
          if( This->s.palette != NULL )
            IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);
	  This->s.palette = ipal;

	  return DD_OK;
	}
	
	if( !(ipal->cm) && (This->s.ddraw->d.screen_pixelformat.u.dwRGBBitCount<=8)) 
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
          /* Perform the refresh */
          TSXSetWindowColormap(display,This->s.ddraw->d.drawable,This->s.palette->cm);
        }
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDrawSurface4Impl_SetPalette(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWPALETTE pal
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        IDirectDrawPaletteImpl* ipal=(IDirectDrawPaletteImpl*)pal;
	TRACE("(%p)->(%p)\n",This,ipal);

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
#ifdef HAVE_LIBXXF86DGA2
	  if (This->s.ddraw->e.dga.version == 2)
	    TSXDGAInstallColormap(display,DefaultScreen(display),This->s.palette->cm);
	  else
#endif /* defined(HAVE_LIBXXF86DGA2) */
	    TSXF86DGAInstallColormap(display,DefaultScreen(display),This->s.palette->cm);
        }
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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
	FIXME("Color fill not implemented for bpp %d!\n", bpp*8);
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
  
  TRACE("(%p)->(%p,%p,%p,%08lx,%p)\n", This,rdst,src,rsrc,dwFlags,lpbltfx);
  
  if (src) IDirectDrawSurface4_Lock(src, NULL, &sdesc, 0, 0);
  IDirectDrawSurface4_Lock(iface,NULL,&ddesc,0,0);
  
  if (TRACE_ON(ddraw)) {
    if (rdst) TRACE("\tdestrect :%dx%d-%dx%d\n",rdst->left,rdst->top,rdst->right,rdst->bottom);
    if (rsrc) TRACE("\tsrcrect  :%dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
    TRACE("\tflags: ");
    _dump_DDBLT(dwFlags);
    if (dwFlags & DDBLT_DDFX) {
      TRACE("\tblitfx: ");
      _dump_DDBLTFX(lpbltfx->dwDDFX);
    }
  }
  
  if (rdst) {
    if ((rdst->top < 0) ||
        (rdst->left < 0) ||
        (rdst->bottom < 0) ||
        (rdst->right < 0)) {
      ERR(" Negative values in LPRECT !!!\n");
      goto release;
    }
    memcpy(&xdst,rdst,sizeof(xdst));
  } else {
    xdst.top	= 0;
    xdst.bottom	= ddesc.dwHeight;
    xdst.left	= 0;
    xdst.right	= ddesc.dwWidth;
  }
  
  if (rsrc) {
    if ((rsrc->top < 0) ||
        (rsrc->left < 0) ||
        (rsrc->bottom < 0) ||
        (rsrc->right < 0)) {
      ERR(" Negative values in LPRECT !!!\n");
      goto release;
    }
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
  
  bpp = GET_BPP(ddesc);
  srcheight = xsrc.bottom - xsrc.top;
  srcwidth = xsrc.right - xsrc.left;
  dstheight = xdst.bottom - xdst.top;
  dstwidth = xdst.right - xdst.left;
  width = (xdst.right - xdst.left) * bpp;
  dbuf = (BYTE *) ddesc.u1.lpSurface + (xdst.top * ddesc.lPitch) + (xdst.left * bpp);
  
  dwFlags &= ~(DDBLT_WAIT|DDBLT_ASYNC);/* FIXME: can't handle right now */
  
  /* First, all the 'source-less' blits */
  if (dwFlags & DDBLT_COLORFILL) {
    ret = _Blt_ColorFill(dbuf, dstwidth, dstheight, bpp,
			 ddesc.lPitch, lpbltfx->u4.dwFillColor);
    dwFlags &= ~DDBLT_COLORFILL;
  }
  
  if (dwFlags & DDBLT_DEPTHFILL) {
#ifdef HAVE_MESAGL
    GLboolean ztest;
    
    /* Clears the screen */
    TRACE("	Filling depth buffer with %ld\n", lpbltfx->u4.dwFillDepth);
    glClearDepth(lpbltfx->u4.dwFillDepth / 65535.0); /* We suppose a 16 bit Z Buffer */
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
      FIXME("Unsupported raster op: %08lx  Pattern: %p\n", lpbltfx->dwROP, lpbltfx->u4.lpDDSPattern);
      goto error;
    }
    dwFlags &= ~DDBLT_ROP;
  }

  if (dwFlags & DDBLT_DDROPS) {
    FIXME("\tDdraw Raster Ops: %08lx  Pattern: %p\n", lpbltfx->dwDDROP, lpbltfx->u4.lpDDSPattern);
  }
  
  /* Now the 'with source' blits */
  if (src) {
    LPBYTE sbase;
    int sx, xinc, sy, yinc;
    
    sbase = (BYTE *) sdesc.u1.lpSurface + (xsrc.top * sdesc.lPitch) + xsrc.left * bpp;
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
	break; }
	      
	    switch(bpp) {
	    case 1: STRETCH_ROW(BYTE)
	    case 2: STRETCH_ROW(WORD)
	    case 4: STRETCH_ROW(DWORD)
	    case 3: {
		LPBYTE s,d;
		for (x = sx = 0; x < dstwidth; x++, sx+= xinc) {
			DWORD pixel;

			s = sbuf+3*(sx>>16);
			d = dbuf+3*x;
			pixel = (s[0]<<16)|(s[1]<<8)|s[2];
			d[0] = (pixel>>16)&0xff;
			d[1] = (pixel>> 8)&0xff;
			d[2] = (pixel    )&0xff;
		}
		break;
	    }
	    default:
	      FIXME("Stretched blit not implemented for bpp %d!\n", bpp*8);
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
	FIXME("DDBLT_KEYDEST not fully supported yet.\n");
	keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
	keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
      }
      
		
      for (y = sy = 0; y < dstheight; y++, sy += yinc) {
	sbuf = sbase + (sy >> 16) * sdesc.lPitch;
	
#define COPYROW_COLORKEY(type) { \
	type *s = (type *) sbuf, *d = (type *) dbuf, tmp; \
	for (x = sx = 0; x < dstwidth; x++, sx += xinc) { \
		tmp = s[sx >> 16]; \
		if (tmp < keylow || tmp > keyhigh) d[x] = tmp; \
	} \
	break; }
	  
	switch (bpp) {
	case 1: COPYROW_COLORKEY(BYTE)
	case 2: COPYROW_COLORKEY(WORD)
	case 4: COPYROW_COLORKEY(DWORD)
	default:
	  FIXME("%s color-keyed blit not implemented for bpp %d!\n",
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
    FIXME("\tUnsupported flags: ");
    _dump_DDBLT(dwFlags);
  }
  release:
  
  IDirectDrawSurface4_Unlock(iface,ddesc.u1.lpSurface);
  if (src) IDirectDrawSurface4_Unlock(src,sdesc.u1.lpSurface);
  
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
	RECT                            rsrc2;


	if (TRACE_ON(ddraw)) {
	    FIXME("(%p)->(%ld,%ld,%p,%p,%08lx)\n",
		    This,dstx,dsty,src,rsrc,trans
	    );
	    FIXME("	trans:");
	    if (FIXME_ON(ddraw))
	      _dump_DDBLTFAST(trans);
	    if (rsrc)
	      FIXME("        srcrect: %dx%d-%dx%d\n",rsrc->left,rsrc->top,rsrc->right,rsrc->bottom);
	    else
	      FIXME(" srcrect: NULL\n");
	}
	
	/* We need to lock the surfaces, or we won't get refreshes when done. */
	IDirectDrawSurface4_Lock(src, NULL,&sdesc,DDLOCK_READONLY, 0);
	IDirectDrawSurface4_Lock(iface,NULL,&ddesc,DDLOCK_WRITEONLY,0);

       if (!rsrc) {
               WARN("rsrc is NULL!\n");
               rsrc = &rsrc2;
               rsrc->left = rsrc->top = 0;
               rsrc->right = sdesc.dwWidth;
               rsrc->bottom = sdesc.dwHeight;
       }

	bpp = GET_BPP(This->s.surface_desc);
	sbuf = (BYTE *) sdesc.u1.lpSurface + (rsrc->top * sdesc.lPitch) + rsrc->left * bpp;
	dbuf = (BYTE *) ddesc.u1.lpSurface + (dsty      * ddesc.lPitch) + dstx       * bpp;


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
			FIXME("DDBLTFAST_DESTCOLORKEY not fully supported yet.\n");
			keylow  = ddesc.ddckCKDestBlt.dwColorSpaceLowValue;
			keyhigh = ddesc.ddckCKDestBlt.dwColorSpaceHighValue;
		}

#define COPYBOX_COLORKEY(type) { \
	type *d = (type *)dbuf, *s = (type *)sbuf, tmp; \
	s = (type *) ((BYTE *) sdesc.u1.lpSurface + (rsrc->top * sdesc.lPitch) + rsrc->left * bpp); \
	d = (type *) ((BYTE *) ddesc.u1.lpSurface + (dsty * ddesc.lPitch) + dstx * bpp); \
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
				FIXME("Source color key blitting not supported for bpp %d\n", bpp*8);
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

	IDirectDrawSurface4_Unlock(iface,ddesc.u1.lpSurface);
	IDirectDrawSurface4_Unlock(src,sdesc.u1.lpSurface);
	return ret;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_BltBatch(
	LPDIRECTDRAWSURFACE4 iface,LPDDBLTBATCH ddbltbatch,DWORD x,DWORD y
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME("(%p)->BltBatch(%p,%08lx,%08lx),stub!\n",
		This,ddbltbatch,x,y
	);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetCaps(
	LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS caps
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->GetCaps(%p)\n",This,caps);
	caps->dwCaps = DDSCAPS_PALETTE; /* probably more */
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetSurfaceDesc(
	LPDIRECTDRAWSURFACE4 iface,LPDDSURFACEDESC ddsd
) { 
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->GetSurfaceDesc(%p)\n", This,ddsd);
  
    /* Simply copy the surface description stored in the object */
    *ddsd = This->s.surface_desc;
  
    if (TRACE_ON(ddraw)) { _dump_surface_desc(ddsd); }

    return DD_OK;
}

static ULONG WINAPI IDirectDrawSurface4Impl_AddRef(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
    return ++(This->ref);
}

#ifdef HAVE_LIBXXF86DGA
static ULONG WINAPI DGA_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

    if (--(This->ref))
    	return This->ref;

    IDirectDraw2_Release((IDirectDraw2*)This->s.ddraw);
    /* clear out of surface list */
    if (This->t.dga.fb_height == -1)
	HeapFree(GetProcessHeap(),0,This->s.surface_desc.u1.lpSurface);
    else
	This->s.ddraw->e.dga.vpmask &= ~(1<<(This->t.dga.fb_height/This->s.ddraw->e.dga.fb_height));

    /* Free the DIBSection (if any) */
    if (This->s.hdc != 0) {
      SelectObject(This->s.hdc, This->s.holdbitmap);
      DeleteDC(This->s.hdc);
      DeleteObject(This->s.DIBsection);
    }

    /* Free the clipper if attached to this surface */
    if( This->s.lpClipper )
      IDirectDrawClipper_Release(This->s.lpClipper);
    
    HeapFree(GetProcessHeap(),0,This);
    return S_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static ULONG WINAPI Xlib_IDirectDrawSurface4Impl_Release(LPDIRECTDRAWSURFACE4 iface) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE( "(%p)->() decrementing from %lu.\n", This, This->ref );

    if (--(This->ref))
    	return This->ref;

    IDirectDraw2_Release((IDirectDraw2*)This->s.ddraw);

    if (This->t.xlib.image != NULL) {
	if (This->s.ddraw->d.pixel_convert != NULL) {
	    /* In pixel conversion mode, there are 2 buffers to release. */
	    HeapFree(GetProcessHeap(),0,This->s.surface_desc.u1.lpSurface);

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
		HeapFree(GetProcessHeap(),0,This->s.surface_desc.u1.lpSurface);
		TSXDestroyImage(This->t.xlib.image);
#ifdef HAVE_LIBXXSHM	
	    }
#endif
	}
	This->t.xlib.image = 0;
    } else {
	HeapFree(GetProcessHeap(),0,This->s.surface_desc.u1.lpSurface);
    }

    if (This->s.palette)
	IDirectDrawPalette_Release((IDirectDrawPalette*)This->s.palette);

    /* Free the DIBSection (if any) */
    if (This->s.hdc != 0) {
	SelectObject(This->s.hdc, This->s.holdbitmap);
	DeleteDC(This->s.hdc);
	DeleteObject(This->s.DIBsection);
    }

    /* Free the clipper if present */
    if( This->s.lpClipper )
      IDirectDrawClipper_Release(This->s.lpClipper);

    HeapFree(GetProcessHeap(),0,This);
    return S_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetAttachedSurface(
	LPDIRECTDRAWSURFACE4 iface,LPDDSCAPS lpddsd,LPDIRECTDRAWSURFACE4 *lpdsf
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int	i,found = 0,xstart;
    struct _surface_chain	*chain;

    TRACE("(%p)->GetAttachedSurface(%p,%p)\n", This, lpddsd, lpdsf);
    if (TRACE_ON(ddraw)) {
	TRACE("	caps ");_dump_DDSCAPS((void *) &(lpddsd->dwCaps));
	DPRINTF("\n");
    }
    chain = This->s.chain;
    if (!chain)
    	return DDERR_NOTFOUND;

    for (i=0;i<chain->nrofsurfaces;i++)
    	if (chain->surfaces[i] == This)
	    break;

    xstart = i;
    for (i=0;i<chain->nrofsurfaces;i++) {
	if ((SDDSCAPS(chain->surfaces[(xstart+i)%chain->nrofsurfaces])&lpddsd->dwCaps) == lpddsd->dwCaps) {
#if 0
	    if (found) /* may not find the same caps twice, (doc) */
		return DDERR_INVALIDPARAMS;/*FIXME: correct? */
#endif
	    found = (i+1)+xstart;
	}
    }
    if (!found)
	return DDERR_NOTFOUND;
    *lpdsf = (LPDIRECTDRAWSURFACE4)chain->surfaces[found-1-xstart];
    /* FIXME: AddRef? */
    TRACE("found %p\n",*lpdsf);
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Initialize(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAW ddraw,LPDDSURFACEDESC lpdsfd
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->(%p, %p)\n",This,ddraw,lpdsfd);

	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPixelFormat(
	LPDIRECTDRAWSURFACE4 iface,LPDDPIXELFORMAT pf
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->(%p)\n",This,pf);

	*pf = This->s.surface_desc.ddpfPixelFormat;
	if (TRACE_ON(ddraw)) { _dump_pixelformat(pf); DPRINTF("\n"); }
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetBltStatus(LPDIRECTDRAWSURFACE4 iface,DWORD dwFlags) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME("(%p)->(0x%08lx),stub!\n",This,dwFlags);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetOverlayPosition(
	LPDIRECTDRAWSURFACE4 iface,LPLONG x1,LPLONG x2
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME("(%p)->(%p,%p),stub!\n",This,x1,x2);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetClipper(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWCLIPPER lpClipper
) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->(%p)!\n",This,lpClipper);

	if (This->s.lpClipper) IDirectDrawClipper_Release( This->s.lpClipper );
	This->s.lpClipper = lpClipper;
	if (lpClipper) IDirectDrawClipper_AddRef( lpClipper );
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddAttachedSurface(
	LPDIRECTDRAWSURFACE4 iface,LPDIRECTDRAWSURFACE4 surf
) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    IDirectDrawSurface4Impl*isurf = (IDirectDrawSurface4Impl*)surf;
    int i;
    struct _surface_chain *chain;

    FIXME("(%p)->(%p)\n",This,surf);
    chain = This->s.chain;

    /* IDirectDrawSurface4_AddRef(surf); */
    
    if (chain) {
	for (i=0;i<chain->nrofsurfaces;i++)
	    if (chain->surfaces[i] == isurf)
		FIXME("attaching already attached surface %p to %p!\n",iface,isurf);
    } else {
    	chain = HeapAlloc(GetProcessHeap(),0,sizeof(*chain));
	chain->nrofsurfaces = 1;
	chain->surfaces = HeapAlloc(GetProcessHeap(),0,sizeof(chain->surfaces[0]));
	chain->surfaces[0] = This;
	This->s.chain = chain;
    }

    if (chain->surfaces)
	chain->surfaces = HeapReAlloc(
	    GetProcessHeap(),
	    0,
	    chain->surfaces,
	    sizeof(chain->surfaces[0])*(chain->nrofsurfaces+1)
	);
    else
	chain->surfaces = HeapAlloc(GetProcessHeap(),0,sizeof(chain->surfaces[0]));
    isurf->s.chain = chain;
    chain->surfaces[chain->nrofsurfaces++] = isurf;
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDC(LPDIRECTDRAWSURFACE4 iface,HDC* lphdc) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    DDSURFACEDESC desc;
    BITMAPINFO *b_info;
    UINT usage;

    FIXME("(%p)->GetDC(%p)\n",This,lphdc);

    /* Creates a DIB Section of the same size / format as the surface */
    IDirectDrawSurface4_Lock(iface,NULL,&desc,0,0);

    if (This->s.hdc == 0) {
	switch (desc.ddpfPixelFormat.u.dwRGBBitCount) {
	case 16:
	case 32:
#if 0 /* This should be filled if Wine's DIBSection did understand BI_BITFIELDS */
	    b_info = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
	    break;
#endif

	case 24:
	    b_info = (BITMAPINFO *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER));
	    break;

	default:
	    b_info = (BITMAPINFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		    sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (2 << desc.ddpfPixelFormat.u.dwRGBBitCount));
	    break;
	}

	b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	b_info->bmiHeader.biWidth = desc.dwWidth;
	b_info->bmiHeader.biHeight = desc.dwHeight;
	b_info->bmiHeader.biPlanes = 1;
	b_info->bmiHeader.biBitCount = desc.ddpfPixelFormat.u.dwRGBBitCount;
#if 0
	if ((desc.ddpfPixelFormat.u.dwRGBBitCount != 16) &&
	    (desc.ddpfPixelFormat.u.dwRGBBitCount != 32))
#endif
	b_info->bmiHeader.biCompression = BI_RGB;
#if 0
	else
	b_info->bmiHeader.biCompression = BI_BITFIELDS;
#endif
	b_info->bmiHeader.biSizeImage = (desc.ddpfPixelFormat.u.dwRGBBitCount / 8) * desc.dwWidth * desc.dwHeight;
	b_info->bmiHeader.biXPelsPerMeter = 0;
	b_info->bmiHeader.biYPelsPerMeter = 0;
	b_info->bmiHeader.biClrUsed = 0;
	b_info->bmiHeader.biClrImportant = 0;

	switch (desc.ddpfPixelFormat.u.dwRGBBitCount) {
	case 16:
	case 32:
#if 0
	    {
		DWORD *masks = (DWORD *) &(b_info->bmiColors);

		usage = 0;
		masks[0] = desc.ddpfPixelFormat.u1.dwRBitMask;
		masks[1] = desc.ddpfPixelFormat.u2.dwGBitMask;
		masks[2] = desc.ddpfPixelFormat.u3.dwBBitMask;
	    }
	    break;
#endif
	case 24:
	    /* Nothing to do */
	    usage = DIB_RGB_COLORS;
	    break;

	default: {
		int i;

		/* Fill the palette */
		usage = DIB_RGB_COLORS;

		if (This->s.palette == NULL) {
		    ERR("Bad palette !!!\n");
		} else {
		    RGBQUAD *rgb = (RGBQUAD *) &(b_info->bmiColors);
		    PALETTEENTRY *pent = (PALETTEENTRY *)&(This->s.palette->palents);

		    for (i=0;i<(1<<desc.ddpfPixelFormat.u.dwRGBBitCount);i++) {
			rgb[i].rgbBlue = pent[i].peBlue;
			rgb[i].rgbRed = pent[i].peRed;
			rgb[i].rgbGreen = pent[i].peGreen; 
		    }
		}
	    }
	    break;
	}
	This->s.DIBsection = CreateDIBSection(BeginPaint(This->s.ddraw->d.mainWindow,&This->s.ddraw->d.ps),
	    b_info,
	    usage,
	    &(This->s.bitmap_data),
	    0,
	    0
	);
	EndPaint(This->s.ddraw->d.mainWindow,&This->s.ddraw->d.ps);
	TRACE("DIBSection at : %p\n", This->s.bitmap_data);

	/* b_info is not useful anymore */
	HeapFree(GetProcessHeap(), 0, b_info);

	/* Create the DC */
	This->s.hdc = CreateCompatibleDC(0);
	This->s.holdbitmap = SelectObject(This->s.hdc, This->s.DIBsection);
    }

    /* Copy our surface in the DIB section */
    if ((GET_BPP(desc) * desc.dwWidth) == desc.lPitch)
	memcpy(This->s.bitmap_data,desc.u1.lpSurface,desc.lPitch*desc.dwHeight);
    else
	/* TODO */
	FIXME("This case has to be done :/\n");

    TRACE("HDC : %08lx\n", (DWORD) This->s.hdc);
    *lphdc = This->s.hdc;

    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ReleaseDC(LPDIRECTDRAWSURFACE4 iface,HDC hdc) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    FIXME("(%p)->(0x%08lx),stub!\n",This,(long)hdc);
    TRACE( "Copying DIBSection at : %p\n", This->s.bitmap_data);
    /* Copy the DIB section to our surface */
    if ((GET_BPP(This->s.surface_desc) * This->s.surface_desc.dwWidth) == This->s.surface_desc.lPitch) {
	memcpy(This->s.surface_desc.u1.lpSurface, This->s.bitmap_data, This->s.surface_desc.lPitch * This->s.surface_desc.dwHeight);
    } else {
	/* TODO */
	FIXME("This case has to be done :/\n");
    }
    /* Unlock the surface */
    IDirectDrawSurface4_Unlock(iface,This->s.surface_desc.u1.lpSurface);
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_QueryInterface(LPDIRECTDRAWSURFACE4 iface,REFIID refiid,LPVOID *obj) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);

    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
    
    /* All DirectDrawSurface versions (1, 2, 3 and 4) use
     * the same interface. And IUnknown does that too of course.
     */
    if ( IsEqualGUID( &IID_IDirectDrawSurface4, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface3, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface2, refiid )	||
	 IsEqualGUID( &IID_IDirectDrawSurface,  refiid )	||
	 IsEqualGUID( &IID_IUnknown,            refiid )
    ) {
	    *obj = This;
	    IDirectDrawSurface4_AddRef(iface);

	    TRACE("  Creating IDirectDrawSurface interface (%p)\n", *obj);
	    return S_OK;
    }
    else if ( IsEqualGUID( &IID_IDirect3DTexture2, refiid ) )
      {
	/* Texture interface */
	*obj = d3dtexture2_create(This);
	IDirectDrawSurface4_AddRef(iface);
	TRACE("  Creating IDirect3DTexture2 interface (%p)\n", *obj);
	return S_OK;
      }
    else if ( IsEqualGUID( &IID_IDirect3DTexture, refiid ) )
      {
	/* Texture interface */
	*obj = d3dtexture_create(This);
	IDirectDrawSurface4_AddRef(iface);
	
	TRACE("  Creating IDirect3DTexture interface (%p)\n", *obj);
	
	return S_OK;
      }
    else if (is_OpenGL_dx3(refiid, (IDirectDrawSurfaceImpl*)This, (IDirect3DDeviceImpl**) obj)) {
	/* It is the OpenGL Direct3D Device */
	IDirectDrawSurface4_AddRef(iface);
	TRACE("  Creating IDirect3DDevice interface (%p)\n", *obj);
	return S_OK;
    }
    
    FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
    return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_IsLost(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	TRACE("(%p)->(), stub!\n",This);
	return DD_OK; /* hmm */
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE4 iface,LPVOID context,LPDDENUMSURFACESCALLBACK esfcb) {
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int i;
    struct _surface_chain *chain = This->s.chain;

    TRACE("(%p)->(%p,%p)\n",This,context,esfcb);
    for (i=0;i<chain->nrofsurfaces;i++) {
      TRACE( "Enumerating attached surface (%p)\n", chain->surfaces[i]);
      if (esfcb((LPDIRECTDRAWSURFACE) chain->surfaces[i], &(chain->surfaces[i]->s.surface_desc), context) == DDENUMRET_CANCEL)
	return DD_OK; /* FIXME: return value correct? */
    }
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_Restore(LPDIRECTDRAWSURFACE4 iface) {
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
	FIXME("(%p)->(),stub!\n",This);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetColorKey(
	LPDIRECTDRAWSURFACE4 iface, DWORD dwFlags, LPDDCOLORKEY ckey ) 
{
        ICOM_THIS(IDirectDrawSurface4Impl,iface);
        TRACE("(%p)->(0x%08lx,%p)\n",This,dwFlags,ckey);
	if (TRACE_ON(ddraw)) {
	  _dump_colorkeyflag(dwFlags);
	  DPRINTF(" : ");
	  _dump_DDCOLORKEY((void *) ckey);
	  DPRINTF("\n");
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
          FIXME("unhandled dwFlags: 0x%08lx\n", dwFlags );
        }

        return DD_OK;

}

static HRESULT WINAPI IDirectDrawSurface4Impl_AddOverlayDirtyRect(
        LPDIRECTDRAWSURFACE4 iface, 
        LPRECT lpRect )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p),stub!\n",This,lpRect); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_DeleteAttachedSurface(
        LPDIRECTDRAWSURFACE4 iface, 
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSAttachedSurface )
{
    ICOM_THIS(IDirectDrawSurface4Impl,iface);
    int i;
    struct _surface_chain *chain;

    TRACE("(%p)->(0x%08lx,%p)\n",This,dwFlags,lpDDSAttachedSurface);
    chain = This->s.chain;
    for (i=0;i<chain->nrofsurfaces;i++) {
	if ((IDirectDrawSurface4Impl*)lpDDSAttachedSurface==chain->surfaces[i]){
	    IDirectDrawSurface4_Release(lpDDSAttachedSurface);

	    chain->surfaces[i]->s.chain = NULL;
	    memcpy( chain->surfaces+i,
		    chain->surfaces+(i+1),
		    (chain->nrofsurfaces-i-1)*sizeof(chain->surfaces[i])
	    );
	    chain->surfaces = HeapReAlloc(
		GetProcessHeap(),
		0,
		chain->surfaces,
		sizeof(chain->surfaces[i])*(chain->nrofsurfaces-1)
	    );
	    chain->nrofsurfaces--;
	    return DD_OK;
	}
    }
    return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_EnumOverlayZOrders(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPVOID lpContext,
        LPDDENUMSURFACESCALLBACK lpfnCallback )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p),stub!\n", This,dwFlags,
          lpContext, lpfnCallback );

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetClipper(
        LPDIRECTDRAWSURFACE4 iface,
        LPDIRECTDRAWCLIPPER* lplpDDClipper )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p),stub!\n", This, lplpDDClipper);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetColorKey(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPDDCOLORKEY lpDDColorKey )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  TRACE("(%p)->(0x%08lx,%p)\n", This, dwFlags, lpDDColorKey);

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
    FIXME("unhandled dwFlags: 0x%08lx\n", dwFlags );
  }

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetFlipStatus(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags ) 
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPalette(
        LPDIRECTDRAWSURFACE4 iface,
        LPDIRECTDRAWPALETTE* lplpDDPalette )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  TRACE("(%p)->(%p),stub!\n", This, lplpDDPalette);

  if (This->s.palette != NULL) {
    IDirectDrawPalette_AddRef( (IDirectDrawPalette*) This->s.palette );

    *lplpDDPalette = (IDirectDrawPalette*) This->s.palette;
    return DD_OK;
  } else {
    return DDERR_NOPALETTEATTACHED;
  }
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetOverlayPosition(
        LPDIRECTDRAWSURFACE4 iface,
        LONG lX,
        LONG lY)
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%ld,%ld),stub!\n", This, lX, lY);

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
  FIXME("(%p)->(%p,%p,%p,0x%08lx,%p),stub!\n", This,
         lpSrcRect, lpDDDestSurface, lpDestRect, dwFlags, lpDDOverlayFx );  

  return DD_OK;
}
 
static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayDisplay(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags); 

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_UpdateOverlayZOrder(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags,
        LPDIRECTDRAWSURFACE4 lpDDSReference )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx,%p),stub!\n", This, dwFlags, lpDDSReference);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetDDInterface(
        LPDIRECTDRAWSURFACE4 iface,
        LPVOID* lplpDD )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p),stub!\n", This, lplpDD);

  /* Not sure about that... */
  *lplpDD = (void *) This->s.ddraw;
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageLock(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_PageUnlock(
        LPDIRECTDRAWSURFACE4 iface,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(0x%08lx),stub!\n", This, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetSurfaceDesc(
        LPDIRECTDRAWSURFACE4 iface,
        LPDDSURFACEDESC lpDDSD,
        DWORD dwFlags )
{
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p,0x%08lx),stub!\n", This, lpDDSD, dwFlags);

  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_SetPrivateData(LPDIRECTDRAWSURFACE4 iface,
							 REFGUID guidTag,
							 LPVOID lpData,
							 DWORD cbSize,
							 DWORD dwFlags) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p,%p,%ld,%08lx\n", This, guidTag, lpData, cbSize, dwFlags);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetPrivateData(LPDIRECTDRAWSURFACE4 iface,
							 REFGUID guidTag,
							 LPVOID lpBuffer,
							 LPDWORD lpcbBufferSize) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p,%p,%p)\n", This, guidTag, lpBuffer, lpcbBufferSize);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_FreePrivateData(LPDIRECTDRAWSURFACE4 iface,
							  REFGUID guidTag)  {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p)\n", This, guidTag);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_GetUniquenessValue(LPDIRECTDRAWSURFACE4 iface,
							     LPDWORD lpValue)  {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)->(%p)\n", This, lpValue);
  
  return DD_OK;
}

static HRESULT WINAPI IDirectDrawSurface4Impl_ChangeUniquenessValue(LPDIRECTDRAWSURFACE4 iface) {
  ICOM_THIS(IDirectDrawSurface4Impl,iface);
  FIXME("(%p)\n", This);
  
  return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static ICOM_VTABLE(IDirectDrawSurface4) dga_dds4vt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
#endif /* defined(HAVE_LIBXXF86DGA) */

#ifdef HAVE_LIBXXF86DGA2
static ICOM_VTABLE(IDirectDrawSurface4) dga2_dds4vt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	DGA2_IDirectDrawSurface4Impl_Flip,
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
#endif /* defined(HAVE_LIBXXF86DGA2) */

static ICOM_VTABLE(IDirectDrawSurface4) xlib_dds4vt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
  TRACE("(%08lx,%p,%p)\n", dwFlags, ilplpDDClipper, pUnkOuter);

  *ilplpDDClipper = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
  ICOM_VTBL(*ilplpDDClipper) = &ddclipvt;
  (*ilplpDDClipper)->ref = 1;

  (*ilplpDDClipper)->hWnd = 0; 

  return DD_OK;
}

/******************************************************************************
 *			IDirectDrawClipper
 */
static HRESULT WINAPI IDirectDrawClipperImpl_SetHwnd(
	LPDIRECTDRAWCLIPPER iface, DWORD dwFlags, HWND hWnd
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);

	TRACE("(%p)->SetHwnd(0x%08lx,0x%08lx)\n",This,dwFlags,(DWORD)hWnd);
        if( dwFlags ) {
	  FIXME("dwFlags = 0x%08lx, not supported.\n",dwFlags);
          return DDERR_INVALIDPARAMS; 
        }

        This->hWnd = hWnd;
	return DD_OK;
}

static ULONG WINAPI IDirectDrawClipperImpl_Release(LPDIRECTDRAWCLIPPER iface) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
        TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return S_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetClipList(
	LPDIRECTDRAWCLIPPER iface,LPRECT rects,LPRGNDATA lprgn,LPDWORD hmm
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
	FIXME("(%p,%p,%p,%p),stub!\n",This,rects,lprgn,hmm);
	if (hmm) *hmm=0;
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_SetClipList(
	LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD hmm
) {
        ICOM_THIS(IDirectDrawClipperImpl,iface);
	FIXME("(%p,%p,%ld),stub!\n",This,lprgn,hmm);
	return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_QueryInterface(
         LPDIRECTDRAWCLIPPER iface,
         REFIID riid,
         LPVOID* ppvObj )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME("(%p)->(%p,%p),stub!\n",This,riid,ppvObj);
   return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirectDrawClipperImpl_AddRef( LPDIRECTDRAWCLIPPER iface )
{
  ICOM_THIS(IDirectDrawClipperImpl,iface);
  TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
  return ++(This->ref);
}

static HRESULT WINAPI IDirectDrawClipperImpl_GetHWnd(
         LPDIRECTDRAWCLIPPER iface,
         HWND* hWndPtr )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME("(%p)->(%p),stub!\n",This,hWndPtr);

   *hWndPtr = This->hWnd; 

   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_Initialize(
         LPDIRECTDRAWCLIPPER iface,
         LPDIRECTDRAW lpDD,
         DWORD dwFlags )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME("(%p)->(%p,0x%08lx),stub!\n",This,lpDD,dwFlags);
   return DD_OK;
}

static HRESULT WINAPI IDirectDrawClipperImpl_IsClipListChanged(
         LPDIRECTDRAWCLIPPER iface,
         BOOL* lpbChanged )
{
   ICOM_THIS(IDirectDrawClipperImpl,iface);
   FIXME("(%p)->(%p),stub!\n",This,lpbChanged);
   return DD_OK;
}

static ICOM_VTABLE(IDirectDrawClipper) ddclipvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

	TRACE("(%p)->GetEntries(%08lx,%ld,%ld,%p)\n",
	      This,x,start,count,palent);

	/* No palette created and not in depth-convertion mode -> BUG ! */
	if ((This->cm == None) &&
	    (This->ddraw->d.palette_convert == NULL))
	{
		FIXME("app tried to read colormap for non-palettized mode\n");
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

	TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
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
	/* FIXME: we need to update the image or we won't get palette fading. */
	if (This->ddraw->d.palette_convert != NULL)
	  This->ddraw->d.palette_convert(palent, This->screen_palents, start, count);
	  
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDrawPaletteImpl_SetEntries(
	LPDIRECTDRAWPALETTE iface,DWORD x,DWORD start,DWORD count,LPPALETTEENTRY palent
) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
	XColor		xc;
	Colormap	cm;
	int		i;

	TRACE("(%p)->SetEntries(%08lx,%ld,%ld,%p)\n",
		This,x,start,count,palent
	);
	if (!This->cm) /* should not happen */ {
		FIXME("app tried to set colormap in non-palettized mode\n");
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
#ifdef HAVE_LIBXXF86DGA2
	if (This->ddraw->e.dga.version == 2)
	  TSXDGAInstallColormap(display,DefaultScreen(display),This->cm);
	else
#endif /* defined(HAVE_LIBXXF86DGA2) */
	  TSXF86DGAInstallColormap(display,DefaultScreen(display),This->cm);
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static ULONG WINAPI IDirectDrawPaletteImpl_Release(LPDIRECTDRAWPALETTE iface) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
        TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );
	if (!--(This->ref)) {
		if (This->cm) {
			TSXFreeColormap(display,This->cm);
			This->cm = 0;
		}
		HeapFree(GetProcessHeap(),0,This);
		return S_OK;
	}
	return This->ref;
}

static ULONG WINAPI IDirectDrawPaletteImpl_AddRef(LPDIRECTDRAWPALETTE iface) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);

        TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );
	return ++(This->ref);
}

static HRESULT WINAPI IDirectDrawPaletteImpl_Initialize(
	LPDIRECTDRAWPALETTE iface,LPDIRECTDRAW ddraw,DWORD x,LPPALETTEENTRY palent
) {
        ICOM_THIS(IDirectDrawPaletteImpl,iface);
        TRACE("(%p)->(%p,%ld,%p)\n", This, ddraw, x, palent);

	return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectDrawPaletteImpl_GetCaps(
         LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps )
{
   ICOM_THIS(IDirectDrawPaletteImpl,iface);
   FIXME("(%p)->(%p) stub.\n", This, lpdwCaps );
   return DD_OK;
} 

static HRESULT WINAPI IDirectDrawPaletteImpl_QueryInterface(
        LPDIRECTDRAWPALETTE iface,REFIID refiid,LPVOID *obj ) 
{
  ICOM_THIS(IDirectDrawPaletteImpl,iface);

  FIXME("(%p)->(%s,%p) stub.\n",This,debugstr_guid(refiid),obj);

  return S_OK;
}

#ifdef HAVE_LIBXXF86DGA
static ICOM_VTABLE(IDirectDrawPalette) dga_ddpalvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectDrawPaletteImpl_QueryInterface,
	IDirectDrawPaletteImpl_AddRef,
	IDirectDrawPaletteImpl_Release,
	IDirectDrawPaletteImpl_GetCaps,
	IDirectDrawPaletteImpl_GetEntries,
	IDirectDrawPaletteImpl_Initialize,
	DGA_IDirectDrawPaletteImpl_SetEntries
};
#endif /* defined(HAVE_LIBXXF86DGA) */

static ICOM_VTABLE(IDirectDrawPalette) xlib_ddpalvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

        TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
        if ( ( IsEqualGUID( &IID_IDirectDraw,  refiid ) ) ||
	     ( IsEqualGUID (&IID_IDirectDraw2, refiid ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw4, refiid ) ) ) {
                *obj = This->ddraw;
                IDirect3D_AddRef(iface);

		TRACE("  Creating IDirectDrawX interface (%p)\n", *obj);
		
                return S_OK;
        }
        if ( ( IsEqualGUID( &IID_IDirect3D, refiid ) ) ||
	     ( IsEqualGUID( &IID_IUnknown,  refiid ) ) ) {
                *obj = This;
                IDirect3D_AddRef(iface);

		TRACE("  Creating IDirect3D interface (%p)\n", *obj);

                return S_OK;
        }
        if ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) {
                IDirect3D2Impl*  d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = This->ddraw;
                IDirect3D_AddRef(iface);
                ICOM_VTBL(d3d) = &d3d2vt;
                *obj = d3d;

		TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);

                return S_OK;
        }
        FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
        return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirect3DImpl_AddRef(LPDIRECT3D iface) {
        ICOM_THIS(IDirect3DImpl,iface);
        TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

        return ++(This->ref);
}

static ULONG WINAPI IDirect3DImpl_Release(LPDIRECT3D iface)
{
        ICOM_THIS(IDirect3DImpl,iface);
        TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

        if (!--(This->ref)) {
                IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
                HeapFree(GetProcessHeap(),0,This);
                return S_OK;
        }
        return This->ref;
}

static HRESULT WINAPI IDirect3DImpl_Initialize(
         LPDIRECT3D iface, REFIID refiid )
{
  ICOM_THIS(IDirect3DImpl,iface);
  /* FIXME: Not sure if this is correct */

  FIXME("(%p)->(%s):stub.\n",This,debugstr_guid(refiid));
  
  return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirect3DImpl_EnumDevices(LPDIRECT3D iface,
					    LPD3DENUMDEVICESCALLBACK cb,
					    LPVOID context) {
  ICOM_THIS(IDirect3DImpl,iface);
  FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);

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
  TRACE("(%p)->(%p,%p): stub\n", This, lplight, lpunk);
  
  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create_dx3(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_CreateMaterial(LPDIRECT3D iface,
					       LPDIRECT3DMATERIAL *lpmaterial,
					       IUnknown *lpunk)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_CreateViewport(LPDIRECT3D iface,
					       LPDIRECT3DVIEWPORT *lpviewport,
					       IUnknown *lpunk)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3DImpl_FindDevice(LPDIRECT3D iface,
					   LPD3DFINDDEVICESEARCH lpfinddevsrc,
					   LPD3DFINDDEVICERESULT lpfinddevrst)
{
  ICOM_THIS(IDirect3DImpl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);
  
  return DD_OK;
}

static ICOM_VTABLE(IDirect3D) d3dvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

        TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);
        if ( ( IsEqualGUID( &IID_IDirectDraw,  refiid ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw2, refiid ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw4, refiid ) ) ) {
                *obj = This->ddraw;
                IDirect3D2_AddRef(iface);

		TRACE("  Creating IDirectDrawX interface (%p)\n", *obj);
		
                return S_OK;
        }
        if ( ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) ||
	     ( IsEqualGUID( &IID_IUnknown,   refiid ) ) ) {
                *obj = This;
                IDirect3D2_AddRef(iface);

		TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);

                return S_OK;
        }
        if ( IsEqualGUID( &IID_IDirect3D, refiid ) ) {
                IDirect3DImpl*  d3d;

                d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
                d3d->ref = 1;
                d3d->ddraw = This->ddraw;
                IDirect3D2_AddRef(iface);
                ICOM_VTBL(d3d) = &d3dvt;
                *obj = d3d;

		TRACE("  Creating IDirect3D interface (%p)\n", *obj);

                return S_OK;
        }
        FIXME("(%p):interface for IID %s NOT found!\n",This,debugstr_guid(refiid));
        return OLE_E_ENUM_NOMORE;
}

static ULONG WINAPI IDirect3D2Impl_AddRef(LPDIRECT3D2 iface) {
        ICOM_THIS(IDirect3D2Impl,iface);
        TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

        return ++(This->ref);
}

static ULONG WINAPI IDirect3D2Impl_Release(LPDIRECT3D2 iface) {
        ICOM_THIS(IDirect3D2Impl,iface);
        TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

	if (!--(This->ref)) {
		IDirectDraw2_Release((IDirectDraw2*)This->ddraw);
		HeapFree(GetProcessHeap(),0,This);
		return S_OK;
	}
	return This->ref;
}

static HRESULT WINAPI IDirect3D2Impl_EnumDevices(
	LPDIRECT3D2 iface,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
) {
        ICOM_THIS(IDirect3D2Impl,iface);
	FIXME("(%p)->(%p,%p),stub!\n",This,cb,context);

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
  TRACE("(%p)->(%p,%p): stub\n", This, lplight, lpunk);

  /* Call the creation function that is located in d3dlight.c */
  *lplight = d3dlight_create(This);
  
	return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateMaterial(LPDIRECT3D2 iface,
						LPDIRECT3DMATERIAL2 *lpmaterial,
						IUnknown *lpunk)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpmaterial, lpunk);

  /* Call the creation function that is located in d3dviewport.c */
  *lpmaterial = d3dmaterial2_create(This);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateViewport(LPDIRECT3D2 iface,
						LPDIRECT3DVIEWPORT2 *lpviewport,
						IUnknown *lpunk)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpviewport, lpunk);
  
  /* Call the creation function that is located in d3dviewport.c */
  *lpviewport = d3dviewport2_create(This);
  
  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_FindDevice(LPDIRECT3D2 iface,
					    LPD3DFINDDEVICESEARCH lpfinddevsrc,
					    LPD3DFINDDEVICERESULT lpfinddevrst)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  TRACE("(%p)->(%p,%p): stub\n", This, lpfinddevsrc, lpfinddevrst);

  return DD_OK;
}

static HRESULT WINAPI IDirect3D2Impl_CreateDevice(LPDIRECT3D2 iface,
					      REFCLSID rguid,
					      LPDIRECTDRAWSURFACE surface,
					      LPDIRECT3DDEVICE2 *device)
{
  ICOM_THIS(IDirect3D2Impl,iface);
  
  FIXME("(%p)->(%s,%p,%p): stub\n",This,debugstr_guid(rguid),surface,device);

  if (is_OpenGL(rguid, (IDirectDrawSurfaceImpl*)surface, (IDirect3DDevice2Impl**)device, This)) {
    IDirect3D2_AddRef(iface);
    return DD_OK;
}

  return DDERR_INVALIDPARAMS;
}

static ICOM_VTABLE(IDirect3D2) d3d2vt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
static HRESULT common_off_screen_CreateSurface(IDirectDraw2Impl* This,
					       IDirectDrawSurfaceImpl* lpdsf)
{
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

  if (lpdsf->s.surface_desc.dwFlags & DDSD_LPSURFACE) {
    /* The surface was preallocated : seems that we have nothing to do :-) */
    ERR("Creates a surface that is already allocated : assuming this is an application bug !\n");
  }

  assert(bpp);
  FIXME("using w=%ld, h=%ld, bpp=%d\n",lpdsf->s.surface_desc.dwWidth,lpdsf->s.surface_desc.dwHeight,bpp);
  
  lpdsf->s.surface_desc.dwFlags |= DDSD_PITCH|DDSD_LPSURFACE;
  lpdsf->s.surface_desc.u1.lpSurface =
    (LPBYTE)HeapAlloc(GetProcessHeap(),0,lpdsf->s.surface_desc.dwWidth * lpdsf->s.surface_desc.dwHeight * bpp);
  lpdsf->s.surface_desc.lPitch = lpdsf->s.surface_desc.dwWidth * bpp;
  
  return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_CreateSurface(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawSurfaceImpl** ilpdsf=(IDirectDrawSurfaceImpl**)lpdsf;
    int	i, fbheight = This->e.dga.fb_height;

    TRACE("(%p)->(%p,%p,%p)\n",This,lpddsd,ilpdsf,lpunk);
    if (TRACE_ON(ddraw)) { _dump_surface_desc(lpddsd); }

    *ilpdsf = (IDirectDrawSurfaceImpl*)HeapAlloc(
    	GetProcessHeap(),
	HEAP_ZERO_MEMORY,
	sizeof(IDirectDrawSurfaceImpl)
    );
    IDirectDraw2_AddRef(iface);

    (*ilpdsf)->ref = 1;
#ifdef HAVE_LIBXXF86DGA2
    if (This->e.dga.version == 2)
      ICOM_VTBL(*ilpdsf) = (ICOM_VTABLE(IDirectDrawSurface)*)&dga2_dds4vt;
    else
#endif /* defined(HAVE_LIBXXF86DGA2) */
      ICOM_VTBL(*ilpdsf) = (ICOM_VTABLE(IDirectDrawSurface)*)&dga_dds4vt;
    (*ilpdsf)->s.ddraw = This;
    (*ilpdsf)->s.palette = NULL;
    (*ilpdsf)->t.dga.fb_height = -1; /* This is to have non-on screen surfaces freed */
    (*ilpdsf)->s.lpClipper = NULL;

    /* Copy the surface description */
    (*ilpdsf)->s.surface_desc = *lpddsd;

    if (!(lpddsd->dwFlags & DDSD_WIDTH))
	(*ilpdsf)->s.surface_desc.dwWidth  = This->d.width;
    if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	(*ilpdsf)->s.surface_desc.dwHeight = This->d.height;

    (*ilpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT;

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
	TRACE("using viewport %d for a primary surface\n",i);
	/* if i == 32 or maximum ... return error */
	This->e.dga.vpmask|=(1<<i);
	lpddsd->lPitch = (*ilpdsf)->s.surface_desc.lPitch = 
		This->e.dga.fb_width*PFGET_BPP(This->d.directdraw_pixelformat);

	(*ilpdsf)->s.surface_desc.u1.lpSurface =
	    This->e.dga.fb_addr + i*fbheight*lpddsd->lPitch;

	(*ilpdsf)->t.dga.fb_height = i*fbheight;

	/* Add flags if there were not present */
	(*ilpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT|DDSD_PITCH|DDSD_LPSURFACE|DDSD_PIXELFORMAT;
	(*ilpdsf)->s.surface_desc.dwWidth = This->d.width;
	(*ilpdsf)->s.surface_desc.dwHeight = This->d.height;
	TRACE("primary surface: dwWidth=%ld, dwHeight=%ld, lPitch=%ld\n",This->d.width,This->d.height,lpddsd->lPitch);
	/* We put our surface always in video memory */
	SDDSCAPS((*ilpdsf)) |= DDSCAPS_VISIBLE|DDSCAPS_VIDEOMEMORY;
	(*ilpdsf)->s.surface_desc.ddpfPixelFormat = This->d.directdraw_pixelformat;
	(*ilpdsf)->s.chain = NULL;

	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
	    IDirectDrawSurface4Impl*	back;
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
		ICOM_VTBL(back) = (ICOM_VTABLE(IDirectDrawSurface4)*)&dga_dds4vt;
		for (i=0;i<32;i++)
		    if (!(This->e.dga.vpmask & (1<<i)))
			break;
		TRACE("using viewport %d for backbuffer %d\n",i, bbc);
		/* if i == 32 or maximum ... return error */
		This->e.dga.vpmask|=(1<<i);
		back->t.dga.fb_height = i*fbheight;
		/* Copy the surface description from the front buffer */
		back->s.surface_desc = (*ilpdsf)->s.surface_desc;
		/* Change the parameters that are not the same */
		back->s.surface_desc.u1.lpSurface =
		    This->e.dga.fb_addr + i*fbheight*lpddsd->lPitch;

		back->s.ddraw = This;
		/* Add relevant info to front and back buffers */
		/* FIXME: backbuffer/frontbuffer handling broken here, but
		 * will be fixed up in _Flip().
		 */
		SDDSCAPS((*ilpdsf)) |= DDSCAPS_FRONTBUFFER;
		SDDSCAPS(back) |= DDSCAPS_FLIP|DDSCAPS_BACKBUFFER|DDSCAPS_VIDEOMEMORY;
		back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
		SDDSCAPS(back) &= ~DDSCAPS_VISIBLE;
		IDirectDrawSurface4_AddAttachedSurface((LPDIRECTDRAWSURFACE4)(*ilpdsf),(LPDIRECTDRAWSURFACE4)back);
	    }
	}
    } else {
	/* There is no DGA-specific code here...
	Go to the common surface creation function */
	return common_off_screen_CreateSurface(This, *ilpdsf);
    }
    return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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
	lpdsf->s.surface_desc.dwHeight
    );

    if (img == NULL) {
	FIXME("Couldn't create XShm image (due to X11 remote display or failure).\nReverting to standard X images !\n");
	This->e.xlib.xshm_active = 0;
	return NULL;
    }

    lpdsf->t.xlib.shminfo.shmid = shmget( IPC_PRIVATE, img->bytes_per_line * img->height, IPC_CREAT|0777 );
    if (lpdsf->t.xlib.shminfo.shmid < 0) {
	FIXME("Couldn't create shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
	This->e.xlib.xshm_active = 0;
	TSXDestroyImage(img);
	return NULL;
    }

    lpdsf->t.xlib.shminfo.shmaddr = img->data = (char*)shmat(lpdsf->t.xlib.shminfo.shmid, 0, 0);

    if (img->data == (char *) -1) {
	FIXME("Couldn't attach shared memory segment (due to X11 remote display or failure).\nReverting to standard X images !\n");
	This->e.xlib.xshm_active = 0;
	TSXDestroyImage(img);
	shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);
	return NULL;
    }
    lpdsf->t.xlib.shminfo.readOnly = False;

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

	FIXME("Couldn't attach shared memory segment to X server (due to X11 remote display or failure).\nReverting to standard X images !\n");
	This->e.xlib.xshm_active = 0;

	/* Leave the critical section */
	LeaveCriticalSection( &X11DRV_CritSection );
	return NULL;
    }
    /* Here, to be REALLY sure, I should do a XShmPutImage to check if
     * this works, but it may be a bit overkill....
     */
    XSetErrorHandler(WineXHandler);
    LeaveCriticalSection( &X11DRV_CritSection );

    shmctl(lpdsf->t.xlib.shminfo.shmid, IPC_RMID, 0);

    if (This->d.pixel_convert != NULL) {
	lpdsf->s.surface_desc.u1.lpSurface = HeapAlloc(
	    GetProcessHeap(),
	    HEAP_ZERO_MEMORY,
	    lpdsf->s.surface_desc.dwWidth *
	    lpdsf->s.surface_desc.dwHeight *
	    PFGET_BPP(This->d.directdraw_pixelformat)
	);
    } else {
	lpdsf->s.surface_desc.u1.lpSurface = img->data;
    }
    return img;
}
#endif /* HAVE_LIBXXSHM */

static XImage *create_ximage(IDirectDraw2Impl* This, IDirectDrawSurface4Impl* lpdsf) {
    XImage *img = NULL;
    void *img_data;

#ifdef HAVE_LIBXXSHM
    if (This->e.xlib.xshm_active)
	img = create_xshmimage(This, lpdsf);

    if (img == NULL) {
#endif
    /* Allocate surface memory */
	lpdsf->s.surface_desc.u1.lpSurface = HeapAlloc(
	    GetProcessHeap(),HEAP_ZERO_MEMORY,
	    lpdsf->s.surface_desc.dwWidth *
	    lpdsf->s.surface_desc.dwHeight *
	    PFGET_BPP(This->d.directdraw_pixelformat)
	);

	if (This->d.pixel_convert != NULL) {
	    img_data = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
		lpdsf->s.surface_desc.dwWidth *
		lpdsf->s.surface_desc.dwHeight *
		PFGET_BPP(This->d.screen_pixelformat)
	    );
	} else {
	    img_data = lpdsf->s.surface_desc.u1.lpSurface;
	}

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
	    lpdsf->s.surface_desc.dwWidth* PFGET_BPP(This->d.screen_pixelformat)
	);
#ifdef HAVE_LIBXXSHM
    }
#endif
    if (This->d.pixel_convert != NULL)
	lpdsf->s.surface_desc.lPitch = PFGET_BPP(This->d.directdraw_pixelformat) * lpdsf->s.surface_desc.dwWidth;
    else
	lpdsf->s.surface_desc.lPitch = img->bytes_per_line;
    return img;
}

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreateSurface(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsd,LPDIRECTDRAWSURFACE *lpdsf,IUnknown *lpunk
) {
    ICOM_THIS(IDirectDraw2Impl,iface);
    IDirectDrawSurfaceImpl** ilpdsf=(IDirectDrawSurfaceImpl**)lpdsf;

    TRACE("(%p)->CreateSurface(%p,%p,%p)\n", This,lpddsd,ilpdsf,lpunk);

    if (TRACE_ON(ddraw)) { _dump_surface_desc(lpddsd); }

    *ilpdsf = (IDirectDrawSurfaceImpl*)HeapAlloc(
    	GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(IDirectDrawSurfaceImpl)
    );

    IDirectDraw2_AddRef(iface);

    (*ilpdsf)->s.ddraw             = This;
    (*ilpdsf)->ref                 = 1;
    ICOM_VTBL(*ilpdsf)             = (ICOM_VTABLE(IDirectDrawSurface)*)&xlib_dds4vt;
    (*ilpdsf)->s.palette = NULL;
    (*ilpdsf)->t.xlib.image = NULL; /* This is for off-screen buffers */
    (*ilpdsf)->s.lpClipper = NULL;

    /* Copy the surface description */
    (*ilpdsf)->s.surface_desc = *lpddsd;

    if (!(lpddsd->dwFlags & DDSD_WIDTH))
	(*ilpdsf)->s.surface_desc.dwWidth  = This->d.width;
    if (!(lpddsd->dwFlags & DDSD_HEIGHT))
	(*ilpdsf)->s.surface_desc.dwHeight = This->d.height;
    (*ilpdsf)->s.surface_desc.dwFlags |= DDSD_WIDTH|DDSD_HEIGHT;

    /* Check if this a 'primary surface' or not */
    if ((lpddsd->dwFlags & DDSD_CAPS) && 
	(lpddsd->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)) {
	XImage *img;

	TRACE("using standard XImage for a primary surface (%p)\n", *ilpdsf);
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

	/* Check for backbuffers */
	if (lpddsd->dwFlags & DDSD_BACKBUFFERCOUNT) {
	    IDirectDrawSurface4Impl*	back;
	    XImage *img;
	    int	i;

	    for (i=lpddsd->dwBackBufferCount;i--;) {
		back = (IDirectDrawSurface4Impl*)HeapAlloc(
		    GetProcessHeap(),HEAP_ZERO_MEMORY,
		    sizeof(IDirectDrawSurface4Impl)
		);

		TRACE("allocated back-buffer (%p)\n", back);

		IDirectDraw2_AddRef(iface);
		back->s.ddraw = This;

		back->ref = 1;
		ICOM_VTBL(back) = (ICOM_VTABLE(IDirectDrawSurface4)*)&xlib_dds4vt;
		/* Copy the surface description from the front buffer */
		back->s.surface_desc = (*ilpdsf)->s.surface_desc;

		/* Create the XImage */
		img = create_ximage(This, back);
		if (img == NULL)
		    return DDERR_OUTOFMEMORY;
		back->t.xlib.image = img;

		/* Add relevant info to front and back buffers */
		/* FIXME: backbuffer/frontbuffer handling broken here, but
		 * will be fixed up in _Flip().
		 */
		SDDSCAPS((*ilpdsf)) |= DDSCAPS_FRONTBUFFER;
		SDDSCAPS(back) |= DDSCAPS_BACKBUFFER|DDSCAPS_VIDEOMEMORY|DDSCAPS_FLIP;
		back->s.surface_desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
		SDDSCAPS(back) &= ~DDSCAPS_VISIBLE;
		IDirectDrawSurface4_AddAttachedSurface((LPDIRECTDRAWSURFACE4)(*ilpdsf),(LPDIRECTDRAWSURFACE4)back);
	    }
	}
    } else {
	/* There is no Xlib-specific code here...
	Go to the common surface creation function */
	return common_off_screen_CreateSurface(This, *ilpdsf);
    }
    return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_DuplicateSurface(
	LPDIRECTDRAW2 iface,LPDIRECTDRAWSURFACE src,LPDIRECTDRAWSURFACE *dst
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME("(%p)->(%p,%p) simply copies\n",This,src,dst);
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
	} flags[] = {
#define FE(x) { x, #x},
		FE(DDSCL_FULLSCREEN)
		FE(DDSCL_ALLOWREBOOT)
		FE(DDSCL_NOWINDOWCHANGES)
		FE(DDSCL_NORMAL)
		FE(DDSCL_ALLOWMODEX)
		FE(DDSCL_EXCLUSIVE)
		FE(DDSCL_SETFOCUSWINDOW)
		FE(DDSCL_SETDEVICEWINDOW)
		FE(DDSCL_CREATEDEVICEWINDOW)
#undef FE
	};

	FIXME("(%p)->(%08lx,%08lx)\n",This,(DWORD)hwnd,cooplevel);
	if (TRACE_ON(ddraw)) {
	  DPRINTF(" - ");
	  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++) {
	    if (flags[i].mask & cooplevel) {
	      DPRINTF("%s ",flags[i].name);
	    }
	  }
	  DPRINTF("\n");
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
	    TRACE("Setting drawable to %ld\n", This->d.drawable);
        }

	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA2
static HRESULT WINAPI DGA_IDirectDraw2Impl_SetCooperativeLevel(
	LPDIRECTDRAW2 iface,HWND hwnd,DWORD cooplevel
) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  HRESULT ret;
  int evbase, erbase;
  
  ret = IDirectDraw2Impl_SetCooperativeLevel(iface, hwnd, cooplevel);

  if (This->e.dga.version != 2) {
    return ret;
  } else {
    if (ret != DD_OK)
      return ret;
    
    TSXDGAQueryExtension(display, &evbase, &erbase);
    
    /* Now, start handling of DGA events giving the handle to the DDraw window
       as the window for which the event will be reported */
    TSXDGASelectInput(display, DefaultScreen(display),
		      KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask );
    X11DRV_EVENT_SetDGAStatus(hwnd, evbase);
    
    return DD_OK;
  }
}
#endif

/* Small helper to either use the cooperative window or create a new 
 * one (for mouse and keyboard input) and drawing in the Xlib implementation.
 */
static void _common_IDirectDrawImpl_SetDisplayMode(IDirectDrawImpl* This) {
	RECT	rect;

	/* Do destroy only our window */
	if (This->d.window && GetPropA(This->d.window,ddProp)) {
		DestroyWindow(This->d.window);
		This->d.window = 0;
	}
	/* Sanity check cooperative window before assigning it to drawing. */
	if (	IsWindow(This->d.mainWindow) &&
		IsWindowVisible(This->d.mainWindow)
	) {
		/* if it does not fit, resize the cooperative window.
		 * and hope the app likes it 
		 */
		GetWindowRect(This->d.mainWindow,&rect);
		if ((((rect.right-rect.left) >= This->d.width)	&&
		     ((rect.bottom-rect.top) >= This->d.height))
		) {
		    This->d.window = This->d.mainWindow;
		    /* SetWindowPos(This->d.mainWindow,HWND_TOPMOST,0,0,This->d.width,This->d.height,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER); */
		    This->d.paintable = 1;
		}
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
	    SetPropA(This->d.window,ddProp,(LONG)This);
	    ShowWindow(This->d.window,TRUE);
	    UpdateWindow(This->d.window);
	}
	SetFocus(This->d.window);
}

static int _common_depth_to_pixelformat(DWORD depth, 
					DDPIXELFORMAT *pixelformat, 
					DDPIXELFORMAT *screen_pixelformat, 
					int *pix_depth) {
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
	  
	  if (pix_depth != NULL)
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

	    if (pix_depth != NULL)
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

#ifdef HAVE_LIBXXF86DGA2
static void _DGA_Initialize_FrameBuffer(IDirectDrawImpl *This, int mode) {
  DDPIXELFORMAT *pf = &(This->d.directdraw_pixelformat);

  /* Now, get the device / mode description */
  This->e.dga.dev = TSXDGASetMode(display, DefaultScreen(display), mode);
  
  This->e.dga.fb_width = This->e.dga.dev->mode.imageWidth;
  TSXDGASetViewport(display,DefaultScreen(display),0,0, XDGAFlipImmediate);
  This->e.dga.fb_height = This->e.dga.dev->mode.viewportHeight;
  TRACE("video framebuffer: begin %p, width %d, memsize %d\n",
	This->e.dga.dev->data,
	This->e.dga.dev->mode.imageWidth,
	(This->e.dga.dev->mode.imageWidth *
	 This->e.dga.dev->mode.imageHeight *
	 (This->e.dga.dev->mode.bitsPerPixel / 8))
	);
  TRACE("viewport height: %d\n", This->e.dga.dev->mode.viewportHeight);
  /* Get the screen dimensions as seen by Wine.
     In that case, it may be better to ignore the -desktop mode and return the
     real screen size => print a warning */
  This->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
  This->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
  This->e.dga.fb_addr = This->e.dga.dev->data;
  This->e.dga.fb_memsize = (This->e.dga.dev->mode.imageWidth *
				  This->e.dga.dev->mode.imageHeight *
				  (This->e.dga.dev->mode.bitsPerPixel / 8));
  This->e.dga.vpmask = 0;
  
  /* Fill the screen pixelformat */
  pf->dwSize = sizeof(DDPIXELFORMAT);
  pf->dwFourCC = 0;
  pf->u.dwRGBBitCount = This->e.dga.dev->mode.bitsPerPixel;
  if (This->e.dga.dev->mode.depth == 8) {
    pf->dwFlags = DDPF_PALETTEINDEXED8|DDPF_RGB;
    pf->u1.dwRBitMask = 0;
    pf->u2.dwGBitMask = 0;
    pf->u3.dwBBitMask = 0;
  } else {
    pf->dwFlags = DDPF_RGB;
    pf->u1.dwRBitMask = This->e.dga.dev->mode.redMask;
    pf->u2.dwGBitMask = This->e.dga.dev->mode.greenMask;
    pf->u3.dwBBitMask = This->e.dga.dev->mode.blueMask;
  }
  pf->u4.dwRGBAlphaBitMask= 0;
  
  This->d.screen_pixelformat = *pf; 
}
#endif /* defined(HAVE_LIBXXF86DGA2) */

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
        ICOM_THIS(IDirectDrawImpl,iface);
        int	i,mode_count;

	TRACE("(%p)->(%ld,%ld,%ld)\n", This, width, height, depth);

#ifdef HAVE_LIBXXF86DGA2
	if (This->e.dga.version == 2) {
	  XDGAMode *modes = This->e.dga.modes;
	  int mode_to_use = -1;
	  
	  /* Search in the list a display mode that corresponds to what is requested */
	  for (i = 0; i < This->e.dga.num_modes; i++) {
	    if ((height == modes[i].viewportHeight) &&
		(width == modes[i].viewportWidth) &&
		(depth == modes[i].depth)) {
	      mode_to_use = modes[i].num;
	    }
	  }

	  if (mode_to_use < 0) {
	    ERR("Could not find matching mode !!!\n");
	    return DDERR_UNSUPPORTEDMODE;
	  } else {
	    TRACE("Using mode number %d\n", mode_to_use);
	    
	    TSXDGACloseFramebuffer(display, DefaultScreen(display));
	    
	    if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	      ERR("Error opening the frame buffer !!!\n");

	      return DDERR_GENERIC;
	    }
	    
	    /* Initialize the frame buffer */
	    _DGA_Initialize_FrameBuffer(This, mode_to_use);

	    /* Re-get (if necessary) the DGA events */
	    TSXDGASelectInput(display, DefaultScreen(display),
			      KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask );
	  }
	  
	  return DD_OK;
	}
#endif /* defined(HAVE_LIBXXF86DGA2) */

	/* We hope getting the asked for depth */
	if (_common_depth_to_pixelformat(depth, &(This->d.directdraw_pixelformat), &(This->d.screen_pixelformat), NULL) != -1) {
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
	if (This->e.dga.fb_height < height)
		This->e.dga.fb_height = height;
	_common_IDirectDrawImpl_SetDisplayMode(This);

#ifdef HAVE_LIBXXF86VM
#ifdef HAVE_LIBXXF86DGA2
	if (This->e.dga.version == 1) /* Only for DGA 1.0, it crashes with DGA 2.0 */
#endif /* defined(HAVE_LIBXXF86DGA2) */
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
                WARN("Fullscreen mode not available!\n");

            if (vidmode)
	      {
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
#endif /* defined(HAVE_LIBXXF86DGA) */

/* *************************************
      16 / 15 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_16_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned char  *c_src = (unsigned char  *) src;
  unsigned short *c_dst = (unsigned short *) dst;
  int y;

  if (palette != NULL) {
    const unsigned short * pal = (unsigned short *) palette->screen_palents;

    for (y = height; y--; ) {
#if defined(__i386__) && defined(__GNUC__)
      /* gcc generates slightly inefficient code for the the copy / lookup,
       * it generates one excess memory access (to pal) per pixel. Since
       * we know that pal is not modified by the memory write we can
       * put it into a register and reduce the number of memory accesses 
       * from 4 to 3 pp. There are two xor eax,eax to avoid pipeline stalls.
       * (This is not guaranteed to be the fastest method.)
       */
      __asm__ __volatile__(
      "xor %%eax,%%eax\n"
      "1:\n"
      "    lodsb\n"
      "    movw (%%edx,%%eax,2),%%ax\n"
      "    stosw\n"
      "	   xor %%eax,%%eax\n"
      "    loop 1b\n"
      : "=S" (c_src), "=D" (c_dst)
      : "S" (c_src), "D" (c_dst) , "c" (width), "d" (pal)
      : "eax", "cc", "memory"
      );
      c_src+=(pitch-width);
#else
      unsigned char * srclineend = c_src+width;
      while (c_src < srclineend)
        *c_dst++ = pal[*c_src++];
      c_src+=(pitch-width);
#endif
    }
  } else {
    WARN("No palette set...\n");
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
      24 to palettized 8 bpp
   ************************************* */
static void pixel_convert_24_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned char  *c_src = (unsigned char  *) src;
  unsigned char *c_dst = (unsigned char *) dst;
  int y;

  if (palette != NULL) {
    const unsigned int *pal = (unsigned int *) palette->screen_palents;
    
    for (y = height; y--; ) {
      unsigned char * srclineend = c_src+width;
      while (c_src < srclineend ) {
	register long pixel = pal[*c_src++];
	*c_dst++ = pixel;
	*c_dst++ = pixel>>8;
	*c_dst++ = pixel>>16;
      }
      c_src+=(pitch-width);
    }
  } else {
    WARN("No palette set...\n");
    memset(dst, 0, width * height * 4);
  }
}
/* *************************************
      32 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_32_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned char  *c_src = (unsigned char  *) src;
  unsigned int *c_dst = (unsigned int *) dst;
  int y;

  if (palette != NULL) {
    const unsigned int *pal = (unsigned int *) palette->screen_palents;
    
    for (y = height; y--; ) {
#if defined(__i386__) && defined(__GNUC__)
      /* See comment in pixel_convert_16_to_8 */
      __asm__ __volatile__(
      "xor %%eax,%%eax\n"
      "1:\n"
      "    lodsb\n"
      "    movl (%%edx,%%eax,4),%%eax\n"
      "    stosl\n"
      "	   xor %%eax,%%eax\n"
      "    loop 1b\n"
      : "=S" (c_src), "=D" (c_dst)
      : "S" (c_src), "D" (c_dst) , "c" (width), "d" (pal)
      : "eax", "cc", "memory"
      );
      c_src+=(pitch-width);
#else
      unsigned char * srclineend = c_src+width;
      while (c_src < srclineend )
	*c_dst++ = pal[*c_src++];
      c_src+=(pitch-width);
#endif
    }
  } else {
    WARN("No palette set...\n");
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

/* *************************************
      32 bpp to 16 bpp
   ************************************* */
static void pixel_convert_32_to_16(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
  unsigned short *c_src = (unsigned short *) src;
  unsigned int *c_dst = (unsigned int *) dst;
  int y;

  for (y = height; y--; ) {
    unsigned short * srclineend = c_src+width;
    while (c_src < srclineend ) {
      *c_dst++ = (((*c_src & 0xF800) << 8) |
		  ((*c_src & 0x07E0) << 5) |
		  ((*c_src & 0x001F) << 3));
      c_src++;
    }
    c_src+=((pitch/2)-width);
  }
}


static HRESULT WINAPI Xlib_IDirectDrawImpl_SetDisplayMode(
	LPDIRECTDRAW iface,DWORD width,DWORD height,DWORD depth
) {
        ICOM_THIS(IDirectDrawImpl,iface);
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
	  WARN("Warning : running in depth-convertion mode. Should run using a %ld depth for optimal performances.\n", depth);
	  
	  /* Set the depth convertion routines */
	  This->d.pixel_convert = ModeEmulations[c].funcs.pixel_convert;
	  This->d.palette_convert = ModeEmulations[c].funcs.palette_convert;
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
	TRACE("Setting drawable to %ld\n", This->d.drawable);

	if (get_option( "DXGrab", 0 )) {
	    /* Confine cursor movement (risky, but the user asked for it) */
	    TSXGrabPointer(display, This->d.drawable, True, 0, GrabModeAsync, GrabModeAsync, This->d.drawable, None, CurrentTime);
	}

	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_GetCaps(
	LPDIRECTDRAW2 iface,LPDDCAPS caps1,LPDDCAPS caps2
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);
	if (!caps1 && !caps2)
	   return DDERR_INVALIDPARAMS;
	if (caps1) {
		caps1->dwVidMemTotal = This->e.dga.fb_memsize;
		caps1->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps1->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
	if (caps2) {
		caps2->dwVidMemTotal = This->e.dga.fb_memsize;
		caps2->dwCaps = 0xffffffff&~(DDCAPS_BANKSWITCHED);		/* we can do anything */
		caps2->ddsCaps.dwCaps = 0xffffffff;	/* we can do anything */
	}
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static void fill_caps(LPDDCAPS caps) {
  /* This function tries to fill the capabilities of Wine's DDraw implementation.
     Need to be fixed, though.. */
  if (caps == NULL)
    return;

  caps->dwSize = sizeof(*caps);
  caps->dwCaps = DDCAPS_ALPHA | DDCAPS_BLT | DDCAPS_BLTSTRETCH | DDCAPS_BLTCOLORFILL | DDCAPS_BLTDEPTHFILL | DDCAPS_CANBLTSYSMEM |  DDCAPS_COLORKEY | DDCAPS_PALETTE | DDCAPS_NOHARDWARE;
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
	TRACE("(%p)->GetCaps(%p,%p)\n",This,caps1,caps2);

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
	FIXME("(%p)->(%08lx,%p,%p),stub!\n",
		This,x,ilpddclip,lpunk
	);
	*ilpddclip = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawClipperImpl));
	(*ilpddclip)->ref = 1;
	ICOM_VTBL(*ilpddclip) = &ddclipvt;
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
	    ERR("unhandled palette format\n");
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

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_CreatePalette(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
	HRESULT res;
	int xsize = 0,i;

	TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
	res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
	if (res != 0) return res;
	ICOM_VTBL(*ilpddpal) = &dga_ddpalvt;
	if (This->d.directdraw_pixelformat.u.dwRGBBitCount<=8) {
		(*ilpddpal)->cm = TSXCreateColormap(display,DefaultRootWindow(display),DefaultVisualOfScreen(X11DRV_GetXScreen()),AllocAll);
	} else {
		FIXME("why are we doing CreatePalette in hi/truecolor?\n");
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
#endif /* defined(HAVE_LIBXXF86DGA) */

static HRESULT WINAPI Xlib_IDirectDraw2Impl_CreatePalette(
	LPDIRECTDRAW2 iface,DWORD dwFlags,LPPALETTEENTRY palent,LPDIRECTDRAWPALETTE *lpddpal,LPUNKNOWN lpunk
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        IDirectDrawPaletteImpl** ilpddpal=(IDirectDrawPaletteImpl**)lpddpal;
	int xsize;
	HRESULT res;

	TRACE("(%p)->(%08lx,%p,%p,%p)\n",This,dwFlags,palent,ilpddpal,lpunk);
	res = common_IDirectDraw2Impl_CreatePalette(This,dwFlags,palent,ilpddpal,lpunk,&xsize);
	if (res != 0) return res;
	ICOM_VTBL(*ilpddpal) = &xlib_ddpalvt;
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
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
#endif /* defined(HAVE_LIBXXF86DGA) */

static HRESULT WINAPI Xlib_IDirectDraw2Impl_RestoreDisplayMode(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->RestoreDisplayMode()\n", This);
	Sleep(1000);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_WaitForVerticalBlank(
	LPDIRECTDRAW2 iface,DWORD x,HANDLE h
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->(0x%08lx,0x%08x)\n",This,x,h);
	return DD_OK;
}

static ULONG WINAPI IDirectDraw2Impl_AddRef(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE("(%p)->() incrementing from %lu.\n", This, This->ref );

	return ++(This->ref);
}

#ifdef HAVE_LIBXXF86DGA
static ULONG WINAPI DGA_IDirectDraw2Impl_Release(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE("(%p)->() decrementing from %lu.\n", This, This->ref );

	if (!--(This->ref)) {
#ifdef HAVE_LIBXXF86DGA2
               if (This->e.dga.version == 2) {
		 TRACE("Closing access to the FrameBuffer\n");
                 TSXDGACloseFramebuffer(display, DefaultScreen(display));
		 TRACE("Going back to normal X mode of operation\n");
		 TSXDGASetMode(display, DefaultScreen(display), 0);

                 /* Set the input handling back to absolute */
                 X11DRV_EVENT_SetInputMehod(X11DRV_INPUT_ABSOLUTE);

		 /* Remove the handling of DGA2 events */
		 X11DRV_EVENT_SetDGAStatus(0, -1);

		 /* Free the modes list */
		 TSXFree(This->e.dga.modes);
               } else
#endif /* defined(HAVE_LIBXXF86DGA2) */
		 TSXF86DGADirectVideo(display,DefaultScreen(display),0);
		if (This->d.window && GetPropA(This->d.window,ddProp))
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
		SIGNAL_Init();
#endif
		HeapFree(GetProcessHeap(),0,This);
		return S_OK;
	}
	return This->ref;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_QueryInterface(
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
	if ( IsEqualGUID( &IID_IDirect3D, refiid ) ) {
		IDirect3DImpl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		ICOM_VTBL(d3d) = &d3dvt;
		*obj = d3d;

		TRACE("  Creating IDirect3D interface (%p)\n", *obj);
		
		return S_OK;
	}
	if ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) {
		IDirect3D2Impl*	d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		ICOM_VTBL(d3d) = &d3d2vt;
		*obj = d3d;

		TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);
		
		return S_OK;
	}
	WARN("(%p):interface for IID %s _NOT_ found!\n",This,debugstr_guid(refiid));
        return OLE_E_ENUM_NOMORE;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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
	if ( IsEqualGUID( &IID_IDirect3D, refiid ) ) {
		IDirect3DImpl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		ICOM_VTBL(d3d) = &d3dvt;
		*obj = d3d;

		TRACE("  Creating IDirect3D interface (%p)\n", *obj);

		return S_OK;
	}
	if ( IsEqualGUID( &IID_IDirect3D2, refiid ) ) {
		IDirect3D2Impl* d3d;

		d3d = HeapAlloc(GetProcessHeap(),0,sizeof(*d3d));
		d3d->ref = 1;
		d3d->ddraw = (IDirectDrawImpl*)This;
		IDirectDraw2_AddRef(iface);
		ICOM_VTBL(d3d) = &d3d2vt;
		*obj = d3d;

		TRACE("  Creating IDirect3D2 interface (%p)\n", *obj);

		return S_OK;
	}
	WARN("(%p):interface for IID %s _NOT_ found!\n",This,debugstr_guid(refiid));
        return OLE_E_ENUM_NOMORE;
}

static HRESULT WINAPI IDirectDraw2Impl_GetVerticalBlankStatus(
	LPDIRECTDRAW2 iface,BOOL *status
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
        TRACE("(%p)->(%p)\n",This,status);
	*status = TRUE;
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
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

	TRACE("(%p)->(0x%08lx,%p,%p,%p)\n",This,dwFlags,lpddsfd,context,modescb);
	ddsfd.dwSize = sizeof(ddsfd);
	ddsfd.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	if (dwFlags & DDEDM_REFRESHRATES) {
		ddsfd.dwFlags |= DDSD_REFRESHRATE;
		ddsfd.u.dwRefreshRate = 60;
	}
	ddsfd.ddsCaps.dwCaps = 0;
	ddsfd.dwBackBufferCount = 1;

#ifdef HAVE_LIBXXF86DGA2
	if (This->e.dga.version == 2) {
	  XDGAMode *modes = This->e.dga.modes;

	  ddsfd.dwFlags |= DDSD_PITCH;
	  for (i = 0; i < This->e.dga.num_modes; i++) {
	    if (TRACE_ON(ddraw)) {
	      DPRINTF("  Enumerating mode %d : %s (FB: %dx%d / VP: %dx%d) - depth %d -",
		      modes[i].num,
		      modes[i].name, modes[i].imageWidth, modes[i].imageHeight,
		      modes[i].viewportWidth, modes[i].viewportHeight,
		      modes[i].depth);
	      if (modes[i].flags & XDGAConcurrentAccess) DPRINTF(" XDGAConcurrentAccess ");
	      if (modes[i].flags & XDGASolidFillRect) DPRINTF(" XDGASolidFillRect ");
	      if (modes[i].flags & XDGABlitRect) DPRINTF(" XDGABlitRect ");
	      if (modes[i].flags & XDGABlitTransRect) DPRINTF(" XDGABlitTransRect ");
	      if (modes[i].flags & XDGAPixmap) DPRINTF(" XDGAPixmap ");
	      DPRINTF("\n");
	    }
	    /* Fill the pixel format */
	    ddsfd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	    ddsfd.ddpfPixelFormat.dwFourCC = 0;
	    ddsfd.ddpfPixelFormat.u4.dwRGBAlphaBitMask= 0;
	    ddsfd.ddpfPixelFormat.u.dwRGBBitCount = modes[i].bitsPerPixel;
	    if (modes[i].depth == 8) {
	      ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB|DDPF_PALETTEINDEXED8;
	      ddsfd.ddpfPixelFormat.u1.dwRBitMask = 0;
	      ddsfd.ddpfPixelFormat.u2.dwGBitMask = 0;
	      ddsfd.ddpfPixelFormat.u3.dwBBitMask = 0;
	      ddsfd.ddsCaps.dwCaps = DDSCAPS_PALETTE;
	    } else {
	      ddsfd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	      ddsfd.ddpfPixelFormat.u1.dwRBitMask = modes[i].redMask;
	      ddsfd.ddpfPixelFormat.u2.dwGBitMask = modes[i].greenMask;
	      ddsfd.ddpfPixelFormat.u3.dwBBitMask = modes[i].blueMask;
	    }
	    
	    ddsfd.dwWidth = modes[i].viewportWidth;
	    ddsfd.dwHeight = modes[i].viewportHeight;
	    ddsfd.lPitch = modes[i].imageWidth;
	      
	    /* Send mode to the application */
	    if (!modescb(&ddsfd,context)) return DD_OK;
	  }
	} else {
#endif
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
#ifdef HAVE_LIBXXF86DGA2
      }
#endif
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_GetDisplayMode(
	LPDIRECTDRAW2 iface,LPDDSURFACEDESC lpddsfd
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->(%p)\n",This,lpddsfd);
	lpddsfd->dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PITCH|DDSD_BACKBUFFERCOUNT|DDSD_PIXELFORMAT|DDSD_CAPS;
	lpddsfd->dwHeight = This->d.height;
	lpddsfd->dwWidth = This->d.width;
	lpddsfd->lPitch = This->e.dga.fb_width*PFGET_BPP(This->d.directdraw_pixelformat);
	lpddsfd->dwBackBufferCount = 2;
	lpddsfd->u.dwRefreshRate = 60;
	lpddsfd->ddsCaps.dwCaps = DDSCAPS_PALETTE;
	lpddsfd->ddpfPixelFormat = This->d.directdraw_pixelformat;
	if (TRACE_ON(ddraw)) {
	  _dump_surface_desc(lpddsfd);
	}
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

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
	if (TRACE_ON(ddraw)) {
	  _dump_surface_desc(lpddsfd);
	}
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_FlipToGDISurface(LPDIRECTDRAW2 iface) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->()\n",This);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetMonitorFrequency(
	LPDIRECTDRAW2 iface,LPDWORD freq
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME("(%p)->(%p) returns 60 Hz always\n",This,freq);
	*freq = 60*100; /* 60 Hz */
	return DD_OK;
}

/* what can we directly decompress? */
static HRESULT WINAPI IDirectDraw2Impl_GetFourCCCodes(
	LPDIRECTDRAW2 iface,LPDWORD x,LPDWORD y
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	FIXME("(%p,%p,%p), stub\n",This,x,y);
	return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_EnumSurfaces(
	LPDIRECTDRAW2 iface,DWORD x,LPDDSURFACEDESC ddsfd,LPVOID context,LPDDENUMSURFACESCALLBACK ddsfcb
) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME("(%p)->(0x%08lx,%p,%p,%p),stub!\n",This,x,ddsfd,context,ddsfcb);
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_Compact(
          LPDIRECTDRAW2 iface )
{
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME("(%p)->()\n", This );
 
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetGDISurface(LPDIRECTDRAW2 iface,
						 LPDIRECTDRAWSURFACE *lplpGDIDDSSurface) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME("(%p)->(%p)\n", This, lplpGDIDDSSurface);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_GetScanLine(LPDIRECTDRAW2 iface,
					       LPDWORD lpdwScanLine) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME("(%p)->(%p)\n", This, lpdwScanLine);

  if (lpdwScanLine)
	  *lpdwScanLine = 0;
  return DD_OK;
}

static HRESULT WINAPI IDirectDraw2Impl_Initialize(LPDIRECTDRAW2 iface,
					      GUID *lpGUID) {
  ICOM_THIS(IDirectDraw2Impl,iface);
  FIXME("(%p)->(%p)\n", This, lpGUID);
  
  return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

static ICOM_VTABLE(IDirectDraw) dga_ddvt = 
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
#ifdef HAVE_LIBXXF86DGA2
	XCAST(SetCooperativeLevel)DGA_IDirectDraw2Impl_SetCooperativeLevel,
#else
	XCAST(SetCooperativeLevel)IDirectDraw2Impl_SetCooperativeLevel,
#endif
	DGA_IDirectDrawImpl_SetDisplayMode,
	XCAST(WaitForVerticalBlank)IDirectDraw2Impl_WaitForVerticalBlank,
};

#undef XCAST

#endif /* defined(HAVE_LIBXXF86DGA) */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(xlib_ddvt.fn##fun))
#else
# define XCAST(fun)	(void *)
#endif

static ICOM_VTABLE(IDirectDraw) xlib_ddvt = 
{
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

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_SetDisplayMode(
	LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD dwRefreshRate, DWORD dwFlags
) {
	FIXME( "Ignored parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
	return DGA_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static HRESULT WINAPI Xlib_IDirectDraw2Impl_SetDisplayMode(
	LPDIRECTDRAW2 iface,DWORD width,DWORD height,DWORD depth,DWORD dwRefreshRate,DWORD dwFlags
) {
	FIXME( "Ignored parameters (0x%08lx,0x%08lx)\n", dwRefreshRate, dwFlags ); 
	return Xlib_IDirectDrawImpl_SetDisplayMode((LPDIRECTDRAW)iface,width,height,depth);
}

#ifdef HAVE_LIBXXF86DGA
static HRESULT WINAPI DGA_IDirectDraw2Impl_GetAvailableVidMem(
	LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->(%p,%p,%p)\n",
		This,ddscaps,total,free
	);
	if (total) *total = This->e.dga.fb_memsize * 1024;
	if (free) *free = This->e.dga.fb_memsize * 1024;
	return DD_OK;
}
#endif /* defined(HAVE_LIBXXF86DGA) */

static HRESULT WINAPI Xlib_IDirectDraw2Impl_GetAvailableVidMem(
	LPDIRECTDRAW2 iface,LPDDSCAPS ddscaps,LPDWORD total,LPDWORD free
) {
        ICOM_THIS(IDirectDraw2Impl,iface);
	TRACE("(%p)->(%p,%p,%p)\n",
		This,ddscaps,total,free
	);
	if (total) *total = 2048 * 1024;
	if (free) *free = 2048 * 1024;
	return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA
static ICOM_VTABLE(IDirectDraw2) dga_dd2vt = 
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
#endif /* defined(HAVE_LIBXXF86DGA) */

static ICOM_VTABLE(IDirectDraw2) xlib_dd2vt = 
{
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
  FIXME("(%p)->(%08ld,%p)\n", This, (DWORD) hdc, lpDDS);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_RestoreAllSurfaces(LPDIRECTDRAW4 iface) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME("(%p)->()\n", This);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_TestCooperativeLevel(LPDIRECTDRAW4 iface) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME("(%p)->()\n", This);

  return DD_OK;
}

static HRESULT WINAPI IDirectDraw4Impl_GetDeviceIdentifier(LPDIRECTDRAW4 iface,
						       LPDDDEVICEIDENTIFIER lpdddi,
						       DWORD dwFlags) {
  ICOM_THIS(IDirectDraw4Impl,iface);
  FIXME("(%p)->(%p,%08lx)\n", This, lpdddi, dwFlags);
  
  return DD_OK;
}

#ifdef HAVE_LIBXXF86DGA

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(dga_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

static ICOM_VTABLE(IDirectDraw4) dga_dd4vt = 
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

#endif /* defined(HAVE_LIBXXF86DGA) */

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(xlib_dd4vt.fn##fun))
#else
# define XCAST(fun)	(void*)
#endif

static ICOM_VTABLE(IDirectDraw4) xlib_dd4vt = 
{
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

static LRESULT WINAPI Xlib_DDWndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   LRESULT ret;
   IDirectDrawImpl* ddraw = NULL;
   DWORD lastError;

   /* FIXME(ddraw,"(0x%04x,%s,0x%08lx,0x%08lx),stub!\n",(int)hwnd,SPY_GetMsgName(msg),(long)wParam,(long)lParam); */

   SetLastError( ERROR_SUCCESS );
   ddraw  = (IDirectDrawImpl*)GetPropA( hwnd, ddProp );
   if( (!ddraw)  && ( ( lastError = GetLastError() ) != ERROR_SUCCESS )) 
   {
     ERR("Unable to retrieve this ptr from window. Error %08lx\n", lastError );
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

static HRESULT WINAPI DGA_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
#ifdef HAVE_LIBXXF86DGA
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	int  memsize,banksize,major,minor,flags;
	char *addr;
	int  depth;
	int  dga_version;
	int  width, height;
	  
	/* Get DGA availability / version */
	dga_version = DDRAW_DGA_Available();
	
	if (dga_version == 0) {
	  MessageBoxA(0,"Unable to initialize DGA.","WINE DirectDraw",MB_OK|MB_ICONSTOP);
	  return DDERR_GENERIC;
	}

	*ilplpDD = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
	(*ilplpDD)->ref = 1;
	ICOM_VTBL(*ilplpDD) = &dga_ddvt;
#ifdef HAVE_LIBXXF86DGA2
	if (dga_version == 1) {
	  (*ilplpDD)->e.dga.version = 1;
#endif /* defined(HAVE_LIBXXF86DGA2) */
	  TSXF86DGAQueryVersion(display,&major,&minor);
	  TRACE("XF86DGA is version %d.%d\n",major,minor);
	  TSXF86DGAQueryDirectVideo(display,DefaultScreen(display),&flags);
	  if (!(flags & XF86DGADirectPresent))
	    MESSAGE("direct video is NOT PRESENT.\n");
	  TSXF86DGAGetVideo(display,DefaultScreen(display),&addr,&width,&banksize,&memsize);
	  (*ilplpDD)->e.dga.fb_width = width;
	  TSXF86DGAGetViewPortSize(display,DefaultScreen(display),&width,&height);
	  TSXF86DGASetViewPort(display,DefaultScreen(display),0,0);
	  (*ilplpDD)->e.dga.fb_height = height;
	  TRACE("video framebuffer: begin %p, width %d,banksize %d,memsize %d\n",
		addr,width,banksize,memsize
		);
	  TRACE("viewport height: %d\n",height);
	  /* Get the screen dimensions as seen by Wine.
	     In that case, it may be better to ignore the -desktop mode and return the
	     real screen size => print a warning */
	  (*ilplpDD)->d.height = MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
	  (*ilplpDD)->d.width = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
	  if (((*ilplpDD)->d.height != height) ||
	      ((*ilplpDD)->d.width != width))
	    WARN("You seem to be running in -desktop mode. This may prove dangerous in DGA mode...\n");
	  (*ilplpDD)->e.dga.fb_addr = addr;
	  (*ilplpDD)->e.dga.fb_memsize = memsize;
	  (*ilplpDD)->e.dga.vpmask = 0;
	  
	  /* just assume the default depth is the DGA depth too */
	  depth = DefaultDepthOfScreen(X11DRV_GetXScreen());
	  _common_depth_to_pixelformat(depth, &((*ilplpDD)->d.directdraw_pixelformat), &((*ilplpDD)->d.screen_pixelformat), NULL);
#ifdef RESTORE_SIGNALS
	  SIGNAL_Init();
#endif
#ifdef HAVE_LIBXXF86DGA2
	} else {
	  XDGAMode *modes;
	  int i, num_modes;
	  int mode_to_use = 0;
	  
	  (*ilplpDD)->e.dga.version = 2;

	  TSXDGAQueryVersion(display,&major,&minor);
	  TRACE("XDGA is version %d.%d\n",major,minor);

	  TRACE("Opening the frame buffer.\n");
	  if (!TSXDGAOpenFramebuffer(display, DefaultScreen(display))) {
	    ERR("Error opening the frame buffer !!!\n");

	    return DDERR_GENERIC;
	  }

	  /* List all available modes */
	  modes = TSXDGAQueryModes(display, DefaultScreen(display), &num_modes);
	  (*ilplpDD)->e.dga.modes = modes;
	  (*ilplpDD)->e.dga.num_modes = num_modes;
	  if (TRACE_ON(ddraw)) {
	    TRACE("Available modes :\n");
	    for (i = 0; i < num_modes; i++) {
	      DPRINTF("   %d) - %s (FB: %dx%d / VP: %dx%d) - depth %d -",
		      modes[i].num,
		      modes[i].name, modes[i].imageWidth, modes[i].imageHeight,
		      modes[i].viewportWidth, modes[i].viewportHeight,
		      modes[i].depth);
	      if (modes[i].flags & XDGAConcurrentAccess) DPRINTF(" XDGAConcurrentAccess ");
	      if (modes[i].flags & XDGASolidFillRect) DPRINTF(" XDGASolidFillRect ");
	      if (modes[i].flags & XDGABlitRect) DPRINTF(" XDGABlitRect ");
	      if (modes[i].flags & XDGABlitTransRect) DPRINTF(" XDGABlitTransRect ");
	      if (modes[i].flags & XDGAPixmap) DPRINTF(" XDGAPixmap ");
	      DPRINTF("\n");

	      if ((MONITOR_GetHeight(&MONITOR_PrimaryMonitor) == modes[i].viewportHeight) &&
		  (MONITOR_GetWidth(&MONITOR_PrimaryMonitor) == modes[i].viewportWidth) &&
		  (MONITOR_GetDepth(&MONITOR_PrimaryMonitor) == modes[i].depth)) {
		mode_to_use = modes[i].num;
	      }
	    }
	  }
	  if (mode_to_use == 0) {
	    ERR("Could not find mode !\n");
	    mode_to_use = 1;
	  } else {
	    DPRINTF("Using mode number %d\n", mode_to_use);
	  }

	  /* Initialize the frame buffer */
	  _DGA_Initialize_FrameBuffer(*ilplpDD, mode_to_use);
	  /* Set the input handling for relative mouse movements */
	  X11DRV_EVENT_SetInputMehod(X11DRV_INPUT_RELATIVE);
	}
#endif /* defined(HAVE_LIBXXF86DGA2) */
	return DD_OK;
#else /* defined(HAVE_LIBXXF86DGA) */
	return DDERR_INVALIDDIRECTDRAWGUID;
#endif /* defined(HAVE_LIBXXF86DGA) */
}

static BOOL
DDRAW_XSHM_Available(void)
{
#ifdef HAVE_LIBXXSHM
    if (get_option( "UseXShm", 1 ))
    {
        if (TSXShmQueryExtension(display))
        {
            int major, minor;
            Bool shpix;
            if (TSXShmQueryVersion(display, &major, &minor, &shpix)) return TRUE;
        }
    }
#endif
    return FALSE;
}

static HRESULT WINAPI Xlib_DirectDrawCreate( LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) {
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	int depth;

	*ilplpDD = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectDrawImpl));
	ICOM_VTBL(*ilplpDD) = &xlib_ddvt;
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
	if (((*ilplpDD)->e.xlib.xshm_active = DDRAW_XSHM_Available())) {
	  (*ilplpDD)->e.xlib.xshm_compl = 0;
	  TRACE("Using XShm extension.\n");
	}
#endif
	
	return DD_OK;
}

HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter ) {
        IDirectDrawImpl** ilplpDD=(IDirectDrawImpl**)lplpDD;
	WNDCLASSA	wc;
        /* WND*            pParentWindow; */
	HRESULT ret;

	if (!HIWORD(lpGUID)) lpGUID = NULL;

	TRACE("(%s,%p,%p)\n",debugstr_guid(lpGUID),ilplpDD,pUnkOuter);

	if ( ( !lpGUID ) ||
	     ( IsEqualGUID( &IID_IDirectDraw,  lpGUID ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw2, lpGUID ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw4, lpGUID ) ) ) {
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
	wc.cbWndExtra	= 0;

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

	if ( IsEqualGUID( &DGA_DirectDraw_GUID, lpGUID ) ) {
		ret = DGA_DirectDrawCreate(lplpDD, pUnkOuter);
	}
	else if ( IsEqualGUID( &XLIB_DirectDraw_GUID, &XLIB_DirectDraw_GUID ) ) {
		ret = Xlib_DirectDrawCreate(lplpDD, pUnkOuter);
	}
	else {
	  goto err;
	}


	(*ilplpDD)->d.winclass = RegisterClassA(&wc);
	return ret;

      err:
	ERR("DirectDrawCreate(%s,%p,%p): did not recognize requested GUID\n",
            debugstr_guid(lpGUID),lplpDD,pUnkOuter);
	return DDERR_INVALIDDIRECTDRAWGUID;
}

/*******************************************************************************
 * DirectDraw ClassFactory
 *
 *  Heavily inspired (well, can you say completely copied :-) ) from DirectSound
 *
 */
typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI 
DDCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	ICOM_THIS(IClassFactoryImpl,iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
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

	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
	if ( ( IsEqualGUID( &IID_IDirectDraw,  riid ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw2, riid ) ) ||
	     ( IsEqualGUID( &IID_IDirectDraw4, riid ) ) ) {
		/* FIXME: reuse already created DirectDraw if present? */
		return DirectDrawCreate((LPGUID) riid,(LPDIRECTDRAW*)ppobj,pOuter);
	}
	return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI DDCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	ICOM_THIS(IClassFactoryImpl,iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static ICOM_VTABLE(IClassFactory) DDCF_Vtbl = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
    TRACE("(%p,%p,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
    	*ppv = (LPVOID)&DDRAW_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }
    FIXME("(%p,%p,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
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
    FIXME("(void): stub\n");
    return S_FALSE;
}

#else /* !defined(X_DISPLAY_MISSING) */

#include "windef.h"
#include "wtypes.h"

#define DD_OK 0

typedef void *LPUNKNOWN;
typedef void *LPDIRECTDRAW;
typedef void *LPDIRECTDRAWCLIPPER;
typedef void *LPDDENUMCALLBACKA;
typedef void *LPDDENUMCALLBACKEXA;
typedef void *LPDDENUMCALLBACKEXW;
typedef void *LPDDENUMCALLBACKW;

/***********************************************************************
 *		DSoundHelp
 */
HRESULT WINAPI DSoundHelp(DWORD x, DWORD y, DWORD z) 
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawCreate
 */
HRESULT WINAPI DirectDrawCreate(
  LPGUID lpGUID, LPDIRECTDRAW *lplpDD, LPUNKNOWN pUnkOuter) 
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawCreateClipper
 */
HRESULT WINAPI DirectDrawCreateClipper(
  DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, LPUNKNOWN pUnkOuter)
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateA
 */
HRESULT WINAPI DirectDrawEnumerateA(
  LPDDENUMCALLBACKA lpCallback, LPVOID lpContext) 
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateExA
 */
HRESULT WINAPI DirectDrawEnumerateExA(
  LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateExW
 */
HRESULT WINAPI DirectDrawEnumerateExW(
  LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags)
{
  return DD_OK;
}

/***********************************************************************
 *		DirectDrawEnumerateW
 */
HRESULT WINAPI DirectDrawEnumerateW(
  LPDDENUMCALLBACKW lpCallback, LPVOID lpContext) 
{
  return DD_OK;
}

/***********************************************************************
 *		DDRAW_DllGetClassObject
 */
DWORD WINAPI DDRAW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DDRAW_DllCanUnloadNow
 */
DWORD WINAPI DDRAW_DllCanUnloadNow(void)
{
    return DD_OK;
}

#endif /* !defined(X_DISPLAY_MISSING) */




