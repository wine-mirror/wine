#include "config.h"

#include "windef.h"
#include "wingdi.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(quartz);

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

