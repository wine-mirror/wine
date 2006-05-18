/*
 * DirectDraw helper functions
 *
 * Copyright 1997-2000 Marcus Meissner
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* *************************************
      16 / 15 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_16_to_8(void *src, void *dst, DWORD width, DWORD height, LONG pitch, IDirectDrawPaletteImpl* palette) {
    unsigned char  *c_src = (unsigned char  *) src;
    unsigned short *c_dst = (unsigned short *) dst;
    int y;

    if (palette != NULL) {
	const unsigned int * pal = (unsigned int *) palette->screen_palents;

	for (y = height; y--; ) {
#if defined(__i386__) && defined(__GNUC__)
	    /* gcc generates slightly inefficient code for the the copy/lookup,
	     * it generates one excess memory access (to pal) per pixel. Since
	     * we know that pal is not modified by the memory write we can
	     * put it into a register and reduce the number of memory accesses
	     * from 4 to 3 pp. There are two xor eax,eax to avoid pipeline
	     * stalls. (This is not guaranteed to be the fastest method.)
	     */
	    __asm__ __volatile__(
	    "xor %%eax,%%eax\n"
	    "1:\n"
	    "    lodsb\n"
	    "    movw (%%edx,%%eax,4),%%ax\n"
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
	FIXME("No palette set...\n");
	memset(dst, 0, width * height * 2);
    }
}
static void palette_convert_16_to_8(
	LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count
) {
    unsigned int i;
    unsigned int *pal = (unsigned int *) screen_palette;

    for (i = 0; i < count; i++)
	pal[start + i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 8) |
			  ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
			  ((((unsigned short) palent[i].peGreen) & 0xFC) << 3));
}

static void palette_convert_15_to_8(
	LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count
) {
    unsigned int i;
    unsigned int *pal = (unsigned int *) screen_palette;

    for (i = 0; i < count; i++)
	pal[start + i] = (((((unsigned short) palent[i].peRed) & 0xF8) << 7) |
			  ((((unsigned short) palent[i].peBlue) & 0xF8) >> 3) |
			  ((((unsigned short) palent[i].peGreen) & 0xF8) << 2));
}

/* *************************************
      24 to palettized 8 bpp
   ************************************* */
static void pixel_convert_24_to_8(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
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
	FIXME("No palette set...\n");
	memset(dst, 0, width * height * 3);
    }
}

/* *************************************
      32 bpp to palettized 8 bpp
   ************************************* */
static void pixel_convert_32_to_8(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
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
	FIXME("No palette set...\n");
	memset(dst, 0, width * height * 4);
    }
}

static void palette_convert_24_to_8(
	LPPALETTEENTRY palent, void *screen_palette, DWORD start, DWORD count
) {
    unsigned int i;
    unsigned int *pal = (unsigned int *) screen_palette;

    for (i = 0; i < count; i++)
	pal[start + i] = ((((unsigned int) palent[i].peRed) << 16) |
			  (((unsigned int) palent[i].peGreen) << 8) |
			   ((unsigned int) palent[i].peBlue));
}

/* *************************************
      16 bpp to 15 bpp
   ************************************* */
static void pixel_convert_15_to_16(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
    unsigned short *c_src = (unsigned short *) src;
    unsigned short *c_dst = (unsigned short *) dst;
    int y;

    for (y = height; y--; ) {
	unsigned short * srclineend = c_src+width;
	while (c_src < srclineend ) {
	    unsigned short val = *c_src++;
	    *c_dst++=((val&0xFFC0)>>1)|(val&0x001f);
	}
	c_src+=((pitch/2)-width);
    }
}

/* *************************************
      32 bpp to 16 bpp
   ************************************* */
static void pixel_convert_32_to_16(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
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

/* *************************************
      32 bpp to 24 bpp
   ************************************* */
static void pixel_convert_32_to_24(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
    unsigned char *c_src = (unsigned char *) src;
    unsigned int *c_dst = (unsigned int *) dst;
    int y;

    for (y = height; y--; ) {
	unsigned char * srclineend = c_src+width*3;
	while (c_src < srclineend ) {
	    /* FIXME: wrong for big endian */
	    memcpy(c_dst,c_src,3);
	    c_src+=3;
	    c_dst++;
	}
	c_src+=pitch-width*3;
    }
}

/* *************************************
      16 bpp to 32 bpp
   ************************************* */
static void pixel_convert_16_to_32(
	void *src, void *dst, DWORD width, DWORD height, LONG pitch,
	IDirectDrawPaletteImpl* palette
) {
    unsigned int *c_src = (unsigned int *) src;
    unsigned short *c_dst = (unsigned short *) dst;
    int y;

    for (y = height; y--; ) {
	unsigned int * srclineend = c_src+width;
	while (c_src < srclineend ) {
	    *c_dst++ = (((*c_src & 0xF80000) >> 8) |
			((*c_src & 0x00FC00) >> 5) |
			((*c_src & 0x0000F8) >> 3));
	    c_src++;
	}
	c_src+=((pitch/4)-width);
    }
}

Convert ModeEmulations[8] = {
  { { 32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, {  24, 24, 0xFF0000, 0x00FF00, 0x0000FF }, { pixel_convert_32_to_24, NULL } },
  { { 32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, {  16, 16, 0xF800, 0x07E0, 0x001F }, { pixel_convert_32_to_16, NULL } },
  { { 32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_32_to_8,  palette_convert_24_to_8 } },
  { { 24, 24,   0xFF0000,   0x00FF00,   0x0000FF }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_24_to_8,  palette_convert_24_to_8 } },
  { { 16, 15,     0x7C00,     0x03E0,     0x001F }, {  16,16, 0xf800, 0x07e0, 0x001f }, { pixel_convert_15_to_16,  NULL } },
  { { 16, 16,     0xF800,     0x07E0,     0x001F }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_16_to_8,  palette_convert_16_to_8 } },
  { { 16, 15,     0x7C00,     0x03E0,     0x001F }, {  8,  8, 0x00, 0x00, 0x00 }, { pixel_convert_16_to_8,  palette_convert_15_to_8 } },
  { { 16, 16,     0xF800,     0x07E0,     0x001F }, {  32, 24, 0x00FF0000, 0x0000FF00, 0x000000FF }, { pixel_convert_16_to_32, NULL } }
};

void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS* pIn, DDSCAPS2* pOut)
{
    /* 2 adds three additional caps fields to the end. Both versions
     * are unversioned. */
    pOut->dwCaps = pIn->dwCaps;
    pOut->dwCaps2 = 0;
    pOut->dwCaps3 = 0;
    pOut->dwCaps4 = 0;
}

void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn,
					     DDDEVICEIDENTIFIER* pOut)
{
    /* 2 adds a dwWHQLLevel field to the end. Both structures are
     * unversioned. */
    memcpy(pOut, pIn, sizeof(*pOut));
}

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
	    FE(DDBLT_DEPTHFILL),
	    FE(DDBLT_DONOTWAIT)
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
	const char *cmd;
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
    DPRINTF("%p", *((const void * const*) in));
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
	    ME(DDSD_ZBUFFERBITDEPTH, DDRAW_dump_DWORD, u2.dwMipMapCount), /* This is for 'old-style' D3D */
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

    if (NULL == lpddsd) {
        DPRINTF("(null)\n");
    } else {
      if (lpddsd->dwSize >= sizeof(DDSURFACEDESC2)) {
          DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps2, 1);
      } else {
          DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members_caps, 1);
      }
      DDRAW_dump_members(lpddsd->dwFlags, lpddsd, members,
			 sizeof(members)/sizeof(members[0]));
    }
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

void DDRAW_dump_DDENUMSURFACES(DWORD flagmask)
{
    static const flag_info flags[] =
	{
	    FE(DDENUMSURFACES_ALL),
	    FE(DDENUMSURFACES_MATCH),
	    FE(DDENUMSURFACES_NOMATCH),
	    FE(DDENUMSURFACES_CANBECREATED),
	    FE(DDENUMSURFACES_DOESEXIST)
	};
    DDRAW_dump_flags(flagmask, flags, sizeof(flags)/sizeof(flags[0]));
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

void DDRAW_dump_surface_to_disk(IDirectDrawSurfaceImpl *surface, FILE *f, int scale)
{
    int rwidth, rheight, x, y;
    static char *output = NULL;
    static int size = 0;

    rwidth  = (surface->surface_desc.dwWidth  + scale - 1) / scale;
    rheight = (surface->surface_desc.dwHeight + scale - 1) / scale;

    if (rwidth > size) {
	output = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, rwidth * 3);
	size = rwidth;
    }
    
    fprintf(f, "P6\n%d %d\n255\n", rwidth, rheight);

    if (surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        unsigned char table[256][3];
	int i;
	
	if (surface->palette == NULL) {
	    fclose(f);
	    return;
	}
	for (i = 0; i < 256; i++) {
	    table[i][0] = surface->palette->palents[i].peRed;
	    table[i][1] = surface->palette->palents[i].peGreen;
	    table[i][2] = surface->palette->palents[i].peBlue;
	}
	for (y = 0; y < rheight; y++) {
	    unsigned char *src = (unsigned char *) surface->surface_desc.lpSurface + (y * scale * surface->surface_desc.u1.lPitch);
	    for (x = 0; x < rwidth; x++) {
		unsigned char color = *src;
		src += scale;

		output[3 * x + 0] = table[color][0];
		output[3 * x + 1] = table[color][1];
		output[3 * x + 2] = table[color][2];
	    }
	    fwrite(output, 3 * rwidth, 1, f);
	}
    } else if (surface->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_RGB) {
        int red_shift, green_shift, blue_shift, pix_width;
	
	if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 8) {
	    pix_width = 1;
	} else if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	    pix_width = 2;
	} else if (surface->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) {
	    pix_width = 4;
	} else {
	    pix_width = 3;
	}
	
	red_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask);
	green_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask);
	blue_shift = get_shift(surface->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask);

	for (y = 0; y < rheight; y++) {
	    unsigned char *src = (unsigned char *) surface->surface_desc.lpSurface + (y * scale * surface->surface_desc.u1.lPitch);
	    for (x = 0; x < rwidth; x++) {	    
		unsigned int color;
		unsigned int comp;
		int i;

		color = 0;
		for (i = 0; i < pix_width; i++) {
		    color |= src[i] << (8 * i);
		}
		src += scale * pix_width;
		
		comp = color & surface->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask;
		output[3 * x + 0] = red_shift > 0 ? comp >> red_shift : comp << -red_shift;
		comp = color & surface->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask;
		output[3 * x + 1] = green_shift > 0 ? comp >> green_shift : comp << -green_shift;
		comp = color & surface->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask;
		output[3 * x + 2] = blue_shift > 0 ? comp >> blue_shift : comp << -blue_shift;
	    }
	    fwrite(output, 3 * rwidth, 1, f);
	}
    }
    fclose(f);
}
