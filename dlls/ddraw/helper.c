
/*		DirectDraw Base Functions
 *
 * Copyright 1997-1999 Marcus Meissner
 * Copyright 1998 Lionel Ulmer (most of Direct3D stuff)
 * Copyright 2000 TransGaming Technologies Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stddef.h>
#include <stdio.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "d3d.h"
#include "ddraw.h"
#include "winerror.h"

#include "wine/exception.h"
#include "ddraw_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/******************************************************************************
 *		debug output functions
 */
void DDRAW_dump_flags_(DWORD flags, const flag_info* names,
		       size_t num_names, int newline)
{
    unsigned int	i;

    for (i=0; i < num_names; i++)
        if ((flags & names[i].val) ||      /* standard flag value */
	    ((!flags) && (!names[i].val))) /* zero value only */
	    DPRINTF("%s ", names[i].name);

    if (newline)
        DPRINTF("\n");
}

void DDRAW_dump_members(DWORD flags, const void* data,
			const member_info* mems, size_t num_mems)
{
    unsigned int i;

    for (i=0; i < num_mems; i++)
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

void DDRAW_dump_DDSCAPS2(const DDSCAPS2 *in)
{
    static const flag_info flags[] = {
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
    static const flag_info flags2[] = {
        FE(DDSCAPS2_HARDWAREDEINTERLACE),
        FE(DDSCAPS2_HINTDYNAMIC),
        FE(DDSCAPS2_HINTSTATIC),
        FE(DDSCAPS2_TEXTUREMANAGE),
        FE(DDSCAPS2_RESERVED1),
        FE(DDSCAPS2_RESERVED2),
        FE(DDSCAPS2_OPAQUE),
        FE(DDSCAPS2_HINTANTIALIASING),
        FE(DDSCAPS2_CUBEMAP),
        FE(DDSCAPS2_CUBEMAP_POSITIVEX),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEX),
        FE(DDSCAPS2_CUBEMAP_POSITIVEY),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEY),
        FE(DDSCAPS2_CUBEMAP_POSITIVEZ),
        FE(DDSCAPS2_CUBEMAP_NEGATIVEZ),
        FE(DDSCAPS2_MIPMAPSUBLEVEL),
        FE(DDSCAPS2_D3DTEXTUREMANAGE),
        FE(DDSCAPS2_DONOTPERSIST),
        FE(DDSCAPS2_STEREOSURFACELEFT)
    };
 
    DDRAW_dump_flags_(in->dwCaps, flags, sizeof(flags)/sizeof(flags[0]), 0);
    DDRAW_dump_flags_(in->dwCaps2, flags2, sizeof(flags2)/sizeof(flags2[0]), 0);
}

void DDRAW_dump_DDSCAPS(const DDSCAPS *in) {
    DDSCAPS2 in_bis;

    in_bis.dwCaps = in->dwCaps;
    in_bis.dwCaps2 = 0;
    in_bis.dwCaps3 = 0;
    in_bis.dwCaps4 = 0;

    DDRAW_dump_DDSCAPS2(&in_bis);
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

    DDRAW_dump_flags_(flagmask, flags, sizeof(flags)/sizeof(flags[0]), 0);
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

void DDRAW_dump_lockflag(DWORD lockflag)
{
    static const flag_info flags[] =
	{
	    FE(DDLOCK_SURFACEMEMORYPTR),
	    FE(DDLOCK_WAIT),
	    FE(DDLOCK_EVENT),
	    FE(DDLOCK_READONLY),
	    FE(DDLOCK_WRITEONLY),
	    FE(DDLOCK_NOSYSLOCK),
	    FE(DDLOCK_DISCARDCONTENTS),
	    FE(DDLOCK_NOOVERWRITE)
	};

    DDRAW_dump_flags(lockflag, flags, sizeof(flags)/sizeof(flags[0]));
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
    static const member_info members_caps[] =
        {
            ME(DDSD_CAPS, DDRAW_dump_DDSCAPS, ddsCaps)
        };
    static const member_info members_caps2[] =
        {
            ME(DDSD_CAPS, DDRAW_dump_DDSCAPS2, ddsCaps)
        };
#undef STRUCT

    if (lpddsd->dwSize >= sizeof(DDSURFACEDESC2)) {
        DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps2, 1);
    } else {
        DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps, 1);
    }
                                                  
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

void DDRAW_dump_DDCAPS(const DDCAPS *lpcaps) {
    static const flag_info flags1[] = {
      FE(DDCAPS_3D),
      FE(DDCAPS_ALIGNBOUNDARYDEST),
      FE(DDCAPS_ALIGNSIZEDEST),
      FE(DDCAPS_ALIGNBOUNDARYSRC),
      FE(DDCAPS_ALIGNSIZESRC),
      FE(DDCAPS_ALIGNSTRIDE),
      FE(DDCAPS_BLT),
      FE(DDCAPS_BLTQUEUE),
      FE(DDCAPS_BLTFOURCC),
      FE(DDCAPS_BLTSTRETCH),
      FE(DDCAPS_GDI),
      FE(DDCAPS_OVERLAY),
      FE(DDCAPS_OVERLAYCANTCLIP),
      FE(DDCAPS_OVERLAYFOURCC),
      FE(DDCAPS_OVERLAYSTRETCH),
      FE(DDCAPS_PALETTE),
      FE(DDCAPS_PALETTEVSYNC),
      FE(DDCAPS_READSCANLINE),
      FE(DDCAPS_STEREOVIEW),
      FE(DDCAPS_VBI),
      FE(DDCAPS_ZBLTS),
      FE(DDCAPS_ZOVERLAYS),
      FE(DDCAPS_COLORKEY),
      FE(DDCAPS_ALPHA),
      FE(DDCAPS_COLORKEYHWASSIST),
      FE(DDCAPS_NOHARDWARE),
      FE(DDCAPS_BLTCOLORFILL),
      FE(DDCAPS_BANKSWITCHED),
      FE(DDCAPS_BLTDEPTHFILL),
      FE(DDCAPS_CANCLIP),
      FE(DDCAPS_CANCLIPSTRETCHED),
      FE(DDCAPS_CANBLTSYSMEM)
    };
    static const flag_info flags2[] = {
      FE(DDCAPS2_CERTIFIED),
      FE(DDCAPS2_NO2DDURING3DSCENE),
      FE(DDCAPS2_VIDEOPORT),
      FE(DDCAPS2_AUTOFLIPOVERLAY),
      FE(DDCAPS2_CANBOBINTERLEAVED),
      FE(DDCAPS2_CANBOBNONINTERLEAVED),
      FE(DDCAPS2_COLORCONTROLOVERLAY),
      FE(DDCAPS2_COLORCONTROLPRIMARY),
      FE(DDCAPS2_CANDROPZ16BIT),
      FE(DDCAPS2_NONLOCALVIDMEM),
      FE(DDCAPS2_NONLOCALVIDMEMCAPS),
      FE(DDCAPS2_NOPAGELOCKREQUIRED),
      FE(DDCAPS2_WIDESURFACES),
      FE(DDCAPS2_CANFLIPODDEVEN),
      FE(DDCAPS2_CANBOBHARDWARE),
      FE(DDCAPS2_COPYFOURCC),
      FE(DDCAPS2_PRIMARYGAMMA),
      FE(DDCAPS2_CANRENDERWINDOWED),
      FE(DDCAPS2_CANCALIBRATEGAMMA),
      FE(DDCAPS2_FLIPINTERVAL),
      FE(DDCAPS2_FLIPNOVSYNC),
      FE(DDCAPS2_CANMANAGETEXTURE),
      FE(DDCAPS2_TEXMANINNONLOCALVIDMEM),
      FE(DDCAPS2_STEREO),
      FE(DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL)
    };
    static const flag_info flags3[] = {
      FE(DDCKEYCAPS_DESTBLT),
      FE(DDCKEYCAPS_DESTBLTCLRSPACE),
      FE(DDCKEYCAPS_DESTBLTCLRSPACEYUV),
      FE(DDCKEYCAPS_DESTBLTYUV),
      FE(DDCKEYCAPS_DESTOVERLAY),
      FE(DDCKEYCAPS_DESTOVERLAYCLRSPACE),
      FE(DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV),
      FE(DDCKEYCAPS_DESTOVERLAYONEACTIVE),
      FE(DDCKEYCAPS_DESTOVERLAYYUV),
      FE(DDCKEYCAPS_SRCBLT),
      FE(DDCKEYCAPS_SRCBLTCLRSPACE),
      FE(DDCKEYCAPS_SRCBLTCLRSPACEYUV),
      FE(DDCKEYCAPS_SRCBLTYUV),
      FE(DDCKEYCAPS_SRCOVERLAY),
      FE(DDCKEYCAPS_SRCOVERLAYCLRSPACE),
      FE(DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV),
      FE(DDCKEYCAPS_SRCOVERLAYONEACTIVE),
      FE(DDCKEYCAPS_SRCOVERLAYYUV),
      FE(DDCKEYCAPS_NOCOSTOVERLAY)
    };
    static const flag_info flags4[] = {
      FE(DDFXCAPS_BLTALPHA),
      FE(DDFXCAPS_OVERLAYALPHA),
      FE(DDFXCAPS_BLTARITHSTRETCHYN),
      FE(DDFXCAPS_BLTARITHSTRETCHY),
      FE(DDFXCAPS_BLTMIRRORLEFTRIGHT),
      FE(DDFXCAPS_BLTMIRRORUPDOWN),
      FE(DDFXCAPS_BLTROTATION),
      FE(DDFXCAPS_BLTROTATION90),
      FE(DDFXCAPS_BLTSHRINKX),
      FE(DDFXCAPS_BLTSHRINKXN),
      FE(DDFXCAPS_BLTSHRINKY),
      FE(DDFXCAPS_BLTSHRINKYN),
      FE(DDFXCAPS_BLTSTRETCHX),
      FE(DDFXCAPS_BLTSTRETCHXN),
      FE(DDFXCAPS_BLTSTRETCHY),
      FE(DDFXCAPS_BLTSTRETCHYN),
      FE(DDFXCAPS_OVERLAYARITHSTRETCHY),
      FE(DDFXCAPS_OVERLAYARITHSTRETCHYN),
      FE(DDFXCAPS_OVERLAYSHRINKX),
      FE(DDFXCAPS_OVERLAYSHRINKXN),
      FE(DDFXCAPS_OVERLAYSHRINKY),
      FE(DDFXCAPS_OVERLAYSHRINKYN),
      FE(DDFXCAPS_OVERLAYSTRETCHX),
      FE(DDFXCAPS_OVERLAYSTRETCHXN),
      FE(DDFXCAPS_OVERLAYSTRETCHY),
      FE(DDFXCAPS_OVERLAYSTRETCHYN),
      FE(DDFXCAPS_OVERLAYMIRRORLEFTRIGHT),
      FE(DDFXCAPS_OVERLAYMIRRORUPDOWN)
    };
    static const flag_info flags5[] = {
      FE(DDFXALPHACAPS_BLTALPHAEDGEBLEND),
      FE(DDFXALPHACAPS_BLTALPHAPIXELS),
      FE(DDFXALPHACAPS_BLTALPHAPIXELSNEG),
      FE(DDFXALPHACAPS_BLTALPHASURFACES),
      FE(DDFXALPHACAPS_BLTALPHASURFACESNEG),
      FE(DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND),
      FE(DDFXALPHACAPS_OVERLAYALPHAPIXELS),
      FE(DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG),
      FE(DDFXALPHACAPS_OVERLAYALPHASURFACES),
      FE(DDFXALPHACAPS_OVERLAYALPHASURFACESNEG)
    };
    static const flag_info flags6[] = {
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
      FE(DDPCAPS_ALPHA),
    };
    static const flag_info flags7[] = {
      FE(DDSVCAPS_RESERVED1),
      FE(DDSVCAPS_RESERVED2),
      FE(DDSVCAPS_RESERVED3),
      FE(DDSVCAPS_RESERVED4),
      FE(DDSVCAPS_STEREOSEQUENTIAL),
    };

    DPRINTF(" - dwSize : %ld\n", lpcaps->dwSize);
    DPRINTF(" - dwCaps : "); DDRAW_dump_flags(lpcaps->dwCaps, flags1, sizeof(flags1)/sizeof(flags1[0]));
    DPRINTF(" - dwCaps2 : "); DDRAW_dump_flags(lpcaps->dwCaps2, flags2, sizeof(flags2)/sizeof(flags2[0]));
    DPRINTF(" - dwCKeyCaps : "); DDRAW_dump_flags(lpcaps->dwCKeyCaps, flags3, sizeof(flags3)/sizeof(flags3[0]));
    DPRINTF(" - dwFXCaps : "); DDRAW_dump_flags(lpcaps->dwFXCaps, flags4, sizeof(flags4)/sizeof(flags4[0]));
    DPRINTF(" - dwFXAlphaCaps : "); DDRAW_dump_flags(lpcaps->dwFXAlphaCaps, flags5, sizeof(flags5)/sizeof(flags5[0]));
    DPRINTF(" - dwPalCaps : "); DDRAW_dump_flags(lpcaps->dwPalCaps, flags6, sizeof(flags6)/sizeof(flags6[0]));
    DPRINTF(" - dwSVCaps : "); DDRAW_dump_flags(lpcaps->dwSVCaps, flags7, sizeof(flags7)/sizeof(flags7[0]));
    DPRINTF("...\n");
    DPRINTF(" - dwNumFourCCCodes : %ld\n", lpcaps->dwNumFourCCCodes);
    DPRINTF(" - dwCurrVisibleOverlays : %ld\n", lpcaps->dwCurrVisibleOverlays);
    DPRINTF(" - dwMinOverlayStretch : %ld\n", lpcaps->dwMinOverlayStretch);
    DPRINTF(" - dwMaxOverlayStretch : %ld\n", lpcaps->dwMaxOverlayStretch);
    DPRINTF("...\n");
    DPRINTF(" - ddsCaps : "); DDRAW_dump_DDSCAPS2(&lpcaps->ddsCaps); DPRINTF("\n");
}

/* Debug function that can be helpful to debug various surface-related problems */
static int get_shift(DWORD color_mask) {
    int shift = 0;
    while (color_mask > 0xFF) {
        color_mask >>= 1;
	shift += 1;
    }
    while ((color_mask & 0x80) == 0) {
        color_mask <<= 1;
	shift -= 1;
    }
    return shift;
}

void DDRAW_dump_surface_to_disk(IDirectDrawSurfaceImpl *surface, FILE *f)
{
    int i;

    fprintf(f, "P6\n%ld %ld\n255\n", surface->surface_desc.dwWidth, surface->surface_desc.dwHeight);

    if (surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        unsigned char table[256][3];
	unsigned char *src = (unsigned char *) surface->surface_desc.lpSurface;
	if (surface->palette == NULL) {
	    fclose(f);
	    return;
	}
	for (i = 0; i < 256; i++) {
	    table[i][0] = surface->palette->palents[i].peRed;
	    table[i][1] = surface->palette->palents[i].peGreen;
	    table[i][2] = surface->palette->palents[i].peBlue;
	}
	for (i = 0; i < surface->surface_desc.dwHeight * surface->surface_desc.dwWidth; i++) {
	    unsigned char color = *src++;
	    fputc(table[color][0], f);
	    fputc(table[color][1], f);
	    fputc(table[color][2], f);
	}
    } else if (surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_RGB) {
        int red_shift, green_shift, blue_shift;
	red_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask);
	green_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask);
	blue_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask);

	for (i = 0; i < surface->surface_desc.dwHeight * surface->surface_desc.dwWidth; i++) {
	    int color;
	    int comp;
	    
	    if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 8) {
	        color = ((unsigned char *) surface->surface_desc.lpSurface)[i];
	    } else if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	        color = ((unsigned short *) surface->surface_desc.lpSurface)[i];
	    } else if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) {
	        color = ((unsigned int *) surface->surface_desc.lpSurface)[i];
	    } else {
	        /* Well, this won't work on platforms without support for non-aligned memory accesses or big endian :-) */
	        color = *((unsigned int *) (((char *) surface->surface_desc.lpSurface) + (3 * i)));
	    }
	    comp = color & surface->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask;
	    fputc(red_shift > 0 ? comp >> red_shift : comp << -red_shift, f);
	    comp = color & surface->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask;
	    fputc(green_shift > 0 ? comp >> green_shift : comp << -green_shift, f);
	    comp = color & surface->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask;
	    fputc(blue_shift > 0 ? comp >> blue_shift : comp << -blue_shift, f);
	}
    }
    fclose(f);
}
