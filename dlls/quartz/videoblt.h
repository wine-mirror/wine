#ifndef QUARTZ_VIDEOBLT_H
#define QUARTZ_VIDEOBLT_H

typedef void (*pVIDEOBLT_Blt)(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed );


void VIDEOBLT_Blt_888_to_332(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed );
void VIDEOBLT_Blt_888_to_555(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed );
void VIDEOBLT_Blt_888_to_565(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed );
void VIDEOBLT_Blt_888_to_8888(
	BYTE* pDst, LONG pitchDst,
	const BYTE* pSrc, LONG pitchSrc,
	LONG width, LONG height,
	const RGBQUAD* prgbSrc, LONG nClrUsed );


#endif  /* QUARTZ_VIDEOBLT_H */
