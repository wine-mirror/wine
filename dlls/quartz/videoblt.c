/*
 * Copyright (C) Hidenori TAKESHIMA
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

#include "windef.h"
#include "wingdi.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#include "videoblt.h"


#define QUARTZ_LOBYTE(pix)	((BYTE)((pix)&0xff))
#define QUARTZ_HIBYTE(pix)	((BYTE)((pix)>>8))

void VIDEOBLT_Blt_888_to_332(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed )
{
	LONG x,y;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			*pDst++ = ((pSrc[2]&0xe0)   ) |
					  ((pSrc[1]&0xe0)>>3) |
					  ((pSrc[0]&0xc0)>>6);
			pSrc += 3;
		}
		pDst += pitchDst - width;
		pSrc += pitchSrc - width*3;
	}
}

void VIDEOBLT_Blt_888_to_555(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed )
{
	LONG x,y;
	unsigned pix;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			pix = ((unsigned)(pSrc[2]&0xf8)<<7) |
				  ((unsigned)(pSrc[1]&0xf8)<<2) |
				  ((unsigned)(pSrc[0]&0xf8)>>3);
			*pDst++ = QUARTZ_LOBYTE(pix);
			*pDst++ = QUARTZ_HIBYTE(pix);
			pSrc += 3;
		}
		pDst += pitchDst - width*2;
		pSrc += pitchSrc - width*3;
	}
}

void VIDEOBLT_Blt_888_to_565(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed )
{
	LONG x,y;
	unsigned pix;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			pix = ((unsigned)(pSrc[2]&0xf8)<<8) |
				  ((unsigned)(pSrc[1]&0xfc)<<3) |
				  ((unsigned)(pSrc[0]&0xf8)>>3);
			*pDst++ = QUARTZ_LOBYTE(pix);
			*pDst++ = QUARTZ_HIBYTE(pix);
			pSrc += 3;
		}
		pDst += pitchDst - width*2;
		pSrc += pitchSrc - width*3;
	}
}

void VIDEOBLT_Blt_888_to_8888(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed )
{
	LONG x,y;

	for ( y = 0; y < height; y++ )
	{
		for ( x = 0; x < width; x++ )
		{
			*pDst++ = *pSrc++;
			*pDst++ = *pSrc++;
			*pDst++ = *pSrc++;
			*pDst++ = (BYTE)0xff;
		}
		pDst += pitchDst - width*4;
		pSrc += pitchSrc - width*3;
	}
}

