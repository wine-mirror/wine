
/*		DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 */

#include "config.h"

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "winerror.h"
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

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *		debug output functions
 */
void _dump_DDBLTFX(DWORD flagmask) {
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
	if (flags[i].mask & flagmask)
	    DPRINTF("%s ",flags[i].name);
    DPRINTF("\n");
}

void _dump_DDBLTFAST(DWORD flagmask) {
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

void _dump_DDBLT(DWORD flagmask) {
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

void _dump_DDSCAPS(void *in) {
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

void _dump_pixelformat_flag(DWORD flagmask) {
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

void _dump_paletteformat(DWORD dwFlags) {
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

void _dump_pixelformat(void *in) {
    LPDDPIXELFORMAT pf = (LPDDPIXELFORMAT) in;

    DPRINTF("( ");
    _dump_pixelformat_flag(pf->dwFlags);
    if (pf->dwFlags & DDPF_FOURCC)
	DPRINTF(", dwFourCC : %ld", pf->dwFourCC);
    if (pf->dwFlags & DDPF_RGB) {
	char *cmd;
	DPRINTF(", RGB bits: %ld, ", pf->u.dwRGBBitCount);
	switch (pf->u.dwRGBBitCount) {
	case 4: cmd = "%1lx"; break;
	case 8: cmd = "%02lx"; break;
	case 16: cmd = "%04lx"; break;
	case 24: cmd = "%06lx"; break;
	case 32: cmd = "%08lx"; break;
	default: ERR("Unexpected bit depth !\n"); cmd = "%d"; break;
	}
	DPRINTF(" R "); DPRINTF(cmd, pf->u1.dwRBitMask);
	DPRINTF(" G "); DPRINTF(cmd, pf->u2.dwGBitMask);
	DPRINTF(" B "); DPRINTF(cmd, pf->u3.dwBBitMask);
	if (pf->dwFlags & DDPF_ALPHAPIXELS)
	    DPRINTF(" A "); DPRINTF(cmd, pf->u4.dwRGBAlphaBitMask);
	if (pf->dwFlags & DDPF_ZPIXELS)
	    DPRINTF(" Z "); DPRINTF(cmd, pf->u4.dwRGBZBitMask);
    }
    if (pf->dwFlags & DDPF_ZBUFFER)
	DPRINTF(", Z bits : %ld", pf->u.dwZBufferBitDepth);
    if (pf->dwFlags & DDPF_ALPHA)
	DPRINTF(", Alpha bits : %ld", pf->u.dwAlphaBitDepth);
    DPRINTF(")");
}

void _dump_colorkeyflag(DWORD ck) {
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
void _dump_DDCOLORKEY(void *in) {
    DDCOLORKEY *ddck = (DDCOLORKEY *) in;

    DPRINTF(" Low : %ld  - High : %ld", ddck->dwColorSpaceLowValue, ddck->dwColorSpaceHighValue);
}

void _dump_surface_desc(DDSURFACEDESC *lpddsd) {
    int	i;
    struct {
	DWORD	mask;
	char	*name;
	void (*func)(void *);
	void	*elt;
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
void _dump_cooperativelevel(DWORD cooplevel) {
    int i;
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

    if (TRACE_ON(ddraw)) {
	DPRINTF(" - ");
	for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
	    if (flags[i].mask & cooplevel)
		DPRINTF("%s ",flags[i].name);
	DPRINTF("\n");
    }
}
