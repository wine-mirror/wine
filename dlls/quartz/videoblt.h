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
