
/*		DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include "config.h"

#include <stddef.h>

#include "d3d.h"
#include "ddraw.h"
#include "winerror.h"

#include "wine/exception.h"
#include "ddraw_private.h"
#include "heap.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *		debug output functions
 */
typedef struct
{
    DWORD val;
    const char* name;
} flag_info;

#define FE(x) { x, #x }

typedef struct
{
    DWORD val;
    const char* name;
    void (*func)(const void *);
    ptrdiff_t offset;
} member_info;

#define ME(x,f,e) { x, #x, (void (*)(const void *))(f), offsetof(STRUCT, e) }

static void DDRAW_dump_flags(DWORD flags, const flag_info* names,
			     size_t num_names)
{
    int	i;

    for (i=0; i < num_names; i++)
	if (names[i].val & flags)
	    DPRINTF("%s ", names[i].name);

    DPRINTF("\n");
}

static void DDRAW_dump_members(DWORD flags, const void* data,
			       const member_info* mems, size_t num_mems)
{
    int i;

    for (i=0; i < sizeof(mems)/sizeof(mems[0]); i++)
    {
	if (mems[i].val & flags)
	{
	    DPRINTF(" - %s : ", mems[i].name);
	    mems[i].func((const char *)data + mems[i].offset);
	    DPRINTF("\n");
	}
    }
}

void DDRAW_dump_DDBLTFX(DWORD flagmask)
{
    static const flag_info flags[] =
	{
	    FE(DDBLTFX_ARITHSTRETCHY),
	    FE(DDBLTFX_MIRRORLEFTRIGHT),
	    FE(DDBLTFX_MIRRORUPDOWN),
	    FE(DDBLTFX_NOTEARING),
	    FE(DDBLTFX_ROTATE180),
	    FE(DDBLTFX_ROTATE270),
	    FE(DDBLTFX_ROTATE90),
	    FE(DDBLTFX_ZBUFFERRANGE),
	    FE(DDBLTFX_ZBUFFERBASEDEST)
	};

    DDRAW_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_DDBLTFAST(DWORD flagmask)
{
    static const flag_info flags[] =
	{
	    FE(DDBLTFAST_NOCOLORKEY),
	    FE(DDBLTFAST_SRCCOLORKEY),
	    FE(DDBLTFAST_DESTCOLORKEY),
	    FE(DDBLTFAST_WAIT)
	};

    DDRAW_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_DDBLT(DWORD flagmask)
{
    static const flag_info flags[] =
	{
	    FE(DDBLT_ALPHADEST),
	    FE(DDBLT_ALPHADESTCONSTOVERRIDE),
	    FE(DDBLT_ALPHADESTNEG),
	    FE(DDBLT_ALPHADESTSURFACEOVERRIDE),
	    FE(DDBLT_ALPHAEDGEBLEND),
	    FE(DDBLT_ALPHASRC),
	    FE(DDBLT_ALPHASRCCONSTOVERRIDE),
	    FE(DDBLT_ALPHASRCNEG),
	    FE(DDBLT_ALPHASRCSURFACEOVERRIDE),
	    FE(DDBLT_ASYNC),
	    FE(DDBLT_COLORFILL),
	    FE(DDBLT_DDFX),
	    FE(DDBLT_DDROPS),
	    FE(DDBLT_KEYDEST),
	    FE(DDBLT_KEYDESTOVERRIDE),
	    FE(DDBLT_KEYSRC),
	    FE(DDBLT_KEYSRCOVERRIDE),
	    FE(DDBLT_ROP),
	    FE(DDBLT_ROTATIONANGLE),
	    FE(DDBLT_ZBUFFER),
	    FE(DDBLT_ZBUFFERDESTCONSTOVERRIDE),
	    FE(DDBLT_ZBUFFERDESTOVERRIDE),
	    FE(DDBLT_ZBUFFERSRCCONSTOVERRIDE),
	    FE(DDBLT_ZBUFFERSRCOVERRIDE),
	    FE(DDBLT_WAIT),
	    FE(DDBLT_DEPTHFILL)
    };

    DDRAW_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_DDSCAPS(const DDSCAPS2 *in)
{
    static const flag_info flags[] = 
	{
	    FE(DDSCAPS_RESERVED1),
	    FE(DDSCAPS_ALPHA),
	    FE(DDSCAPS_BACKBUFFER),
	    FE(DDSCAPS_COMPLEX),
	    FE(DDSCAPS_FLIP),
	    FE(DDSCAPS_FRONTBUFFER),
	    FE(DDSCAPS_OFFSCREENPLAIN),
	    FE(DDSCAPS_OVERLAY),
	    FE(DDSCAPS_PALETTE),
	    FE(DDSCAPS_PRIMARYSURFACE),
	    FE(DDSCAPS_PRIMARYSURFACELEFT),
	    FE(DDSCAPS_SYSTEMMEMORY),
	    FE(DDSCAPS_TEXTURE),
	    FE(DDSCAPS_3DDEVICE),
	    FE(DDSCAPS_VIDEOMEMORY),
	    FE(DDSCAPS_VISIBLE),
	    FE(DDSCAPS_WRITEONLY),
	    FE(DDSCAPS_ZBUFFER),
	    FE(DDSCAPS_OWNDC),
	    FE(DDSCAPS_LIVEVIDEO),
	    FE(DDSCAPS_HWCODEC),
	    FE(DDSCAPS_MODEX),
	    FE(DDSCAPS_MIPMAP),
	    FE(DDSCAPS_RESERVED2),
	    FE(DDSCAPS_ALLOCONLOAD),
	    FE(DDSCAPS_VIDEOPORT),
	    FE(DDSCAPS_LOCALVIDMEM),
	    FE(DDSCAPS_NONLOCALVIDMEM),
	    FE(DDSCAPS_STANDARDVGAMODE),
	    FE(DDSCAPS_OPTIMIZED)
    };

    DDRAW_dump_flags(in->dwCaps, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_pixelformat_flag(DWORD flagmask)
{
    static const flag_info flags[] =
	{
	    FE(DDPF_ALPHAPIXELS),
	    FE(DDPF_ALPHA),
	    FE(DDPF_FOURCC),
	    FE(DDPF_PALETTEINDEXED4),
	    FE(DDPF_PALETTEINDEXEDTO8),
	    FE(DDPF_PALETTEINDEXED8),
	    FE(DDPF_RGB),
	    FE(DDPF_COMPRESSED),
	    FE(DDPF_RGBTOYUV),
	    FE(DDPF_YUV),
	    FE(DDPF_ZBUFFER),
	    FE(DDPF_PALETTEINDEXED1),
	    FE(DDPF_PALETTEINDEXED2),
	    FE(DDPF_ZPIXELS)
    };

    DDRAW_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_paletteformat(DWORD dwFlags)
{
    static const flag_info flags[] =
	{
	    FE(DDPCAPS_4BIT),
	    FE(DDPCAPS_8BITENTRIES),
	    FE(DDPCAPS_8BIT),
	    FE(DDPCAPS_INITIALIZE),
	    FE(DDPCAPS_PRIMARYSURFACE),
	    FE(DDPCAPS_PRIMARYSURFACELEFT),
	    FE(DDPCAPS_ALLOW256),
	    FE(DDPCAPS_VSYNC),
	    FE(DDPCAPS_1BIT),
	    FE(DDPCAPS_2BIT),
	    FE(DDPCAPS_ALPHA)
    };

    DDRAW_dump_flags(dwFlags, flags, sizeof(flags)/sizeof(flags[0]));
}

void DDRAW_dump_pixelformat(const DDPIXELFORMAT *pf) {
    DPRINTF("( ");
    DDRAW_dump_pixelformat_flag(pf->dwFlags);
    if (pf->dwFlags & DDPF_FOURCC) {
	DPRINTF(", dwFourCC code '%c%c%c%c' (0x%08lx) - %ld bits per pixel",
		(unsigned char)( pf->dwFourCC     &0xff),
		(unsigned char)((pf->dwFourCC>> 8)&0xff),
		(unsigned char)((pf->dwFourCC>>16)&0xff),
		(unsigned char)((pf->dwFourCC>>24)&0xff),
		pf->dwFourCC,
		pf->u1.dwYUVBitCount
	);
    }
    if (pf->dwFlags & DDPF_RGB) {
	char *cmd;
	DPRINTF(", RGB bits: %ld, ", pf->u1.dwRGBBitCount);
	switch (pf->u1.dwRGBBitCount) {
	case 4: cmd = "%1lx"; break;
	case 8: cmd = "%02lx"; break;
	case 16: cmd = "%04lx"; break;
	case 24: cmd = "%06lx"; break;
	case 32: cmd = "%08lx"; break;
	default: ERR("Unexpected bit depth !\n"); cmd = "%d"; break;
	}
	DPRINTF(" R "); DPRINTF(cmd, pf->u2.dwRBitMask);
	DPRINTF(" G "); DPRINTF(cmd, pf->u3.dwGBitMask);
	DPRINTF(" B "); DPRINTF(cmd, pf->u4.dwBBitMask);
	if (pf->dwFlags & DDPF_ALPHAPIXELS) {
	    DPRINTF(" A "); DPRINTF(cmd, pf->u5.dwRGBAlphaBitMask);
	}
	if (pf->dwFlags & DDPF_ZPIXELS) {
	    DPRINTF(" Z "); DPRINTF(cmd, pf->u5.dwRGBZBitMask);
	}
    }
    if (pf->dwFlags & DDPF_ZBUFFER) {
	DPRINTF(", Z bits : %ld", pf->u1.dwZBufferBitDepth);
    }
    if (pf->dwFlags & DDPF_ALPHA) {
	DPRINTF(", Alpha bits : %ld", pf->u1.dwAlphaBitDepth);
    }
    DPRINTF(")");
}

void DDRAW_dump_colorkeyflag(DWORD ck)
{
    static const flag_info flags[] =
	{
	    FE(DDCKEY_COLORSPACE),
	    FE(DDCKEY_DESTBLT),
	    FE(DDCKEY_DESTOVERLAY),
	    FE(DDCKEY_SRCBLT),
	    FE(DDCKEY_SRCOVERLAY)
    };

    DDRAW_dump_flags(ck, flags, sizeof(flags)/sizeof(flags[0]));
}

static void DDRAW_dump_DWORD(const void *in) {
    DPRINTF("%ld", *((const DWORD *) in));
}
static void DDRAW_dump_PTR(const void *in) {
    DPRINTF("%p", *((const void **) in));
}
void DDRAW_dump_DDCOLORKEY(const DDCOLORKEY *ddck) {
    DPRINTF(" Low : %ld  - High : %ld", ddck->dwColorSpaceLowValue, ddck->dwColorSpaceHighValue);
}

void DDRAW_dump_surface_desc(const DDSURFACEDESC2 *lpddsd)
{
#define STRUCT DDSURFACEDESC2
    static const member_info members[] =
	{
	    ME(DDSD_CAPS, DDRAW_dump_DDSCAPS, ddsCaps),
	    ME(DDSD_HEIGHT, DDRAW_dump_DWORD, dwHeight),
	    ME(DDSD_WIDTH, DDRAW_dump_DWORD, dwWidth),
	    ME(DDSD_PITCH, DDRAW_dump_DWORD, u1.lPitch),
	    ME(DDSD_LINEARSIZE, DDRAW_dump_DWORD, u1.dwLinearSize),
	    ME(DDSD_BACKBUFFERCOUNT, DDRAW_dump_DWORD, dwBackBufferCount),
	    ME(DDSD_MIPMAPCOUNT, DDRAW_dump_DWORD, u2.dwMipMapCount),
	    ME(DDSD_REFRESHRATE, DDRAW_dump_DWORD, u2.dwRefreshRate),
	    ME(DDSD_ALPHABITDEPTH, DDRAW_dump_DWORD, dwAlphaBitDepth),
	    ME(DDSD_LPSURFACE, DDRAW_dump_PTR, lpSurface),
	    ME(DDSD_CKDESTOVERLAY, DDRAW_dump_DDCOLORKEY, u3.ddckCKDestOverlay),
	    ME(DDSD_CKDESTBLT, DDRAW_dump_DDCOLORKEY, ddckCKDestBlt),
	    ME(DDSD_CKSRCOVERLAY, DDRAW_dump_DDCOLORKEY, ddckCKSrcOverlay),
	    ME(DDSD_CKSRCBLT, DDRAW_dump_DDCOLORKEY, ddckCKSrcBlt),
	    ME(DDSD_PIXELFORMAT, DDRAW_dump_pixelformat, u4.ddpfPixelFormat)
	};

    DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members,
		       sizeof(members)/sizeof(members[0]));
}

void DDRAW_dump_cooperativelevel(DWORD cooplevel)
{
    static const flag_info flags[] =
	{
	    FE(DDSCL_FULLSCREEN),
	    FE(DDSCL_ALLOWREBOOT),
	    FE(DDSCL_NOWINDOWCHANGES),
	    FE(DDSCL_NORMAL),
	    FE(DDSCL_ALLOWMODEX),
	    FE(DDSCL_EXCLUSIVE),
	    FE(DDSCL_SETFOCUSWINDOW),
	    FE(DDSCL_SETDEVICEWINDOW),
	    FE(DDSCL_CREATEDEVICEWINDOW)
    };

    if (TRACE_ON(ddraw))
    {
	DPRINTF(" - ");
	DDRAW_dump_flags(cooplevel, flags, sizeof(flags)/sizeof(flags[0]));
    }
}
