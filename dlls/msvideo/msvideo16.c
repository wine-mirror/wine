/*
 * msvideo 16-bit functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2000 Bradley Baetz
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

#include "winbase.h"
#include "windef.h"
#include "vfw.h"
#include "vfw16.h"
#include "stackframe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

/* handle16 --> handle conversions */
#define HDRAWDIB_32(h16)	((HDRAWDIB)(ULONG_PTR)(h16))
#define HIC_32(h16)		((HIC)(ULONG_PTR)(h16))

/* handle --> handle16 conversions */
#define HDRAWDIB_16(h32)	(LOWORD(h32))
#define HIC_16(h32)		(LOWORD(h32))


/***********************************************************************
 *		DrawDibOpen		[MSVIDEO.102]
 */
HDRAWDIB16 VFWAPI DrawDibOpen16(void)
{
    return HDRAWDIB_16(DrawDibOpen());
}

/***********************************************************************
 *		DrawDibClose		[MSVIDEO.103]
 */
BOOL16 VFWAPI DrawDibClose16(HDRAWDIB16 hdd)
{
    return DrawDibClose(HDRAWDIB_32(hdd));
}

/************************************************************************
 *		DrawDibBegin		[MSVIDEO.104]
 */
BOOL16 VFWAPI DrawDibBegin16(HDRAWDIB16 hdd, HDC16 hdc, INT16 dxDst,
			     INT16 dyDst, LPBITMAPINFOHEADER lpbi, INT16 dxSrc,
			     INT16 dySrc, UINT16 wFlags)
{
    return DrawDibBegin(HDRAWDIB_32(hdd), HDC_32(hdc), dxDst, dyDst, lpbi,
			dxSrc, dySrc, wFlags);
}

/***********************************************************************
 *		DrawDibEnd		[MSVIDEO.105]
 */
BOOL16 VFWAPI DrawDibEnd16(HDRAWDIB16 hdd)
{
    return DrawDibEnd(HDRAWDIB_32(hdd));
}

/**********************************************************************
 *		DrawDibDraw		[MSVIDEO.106]
 */
BOOL16 VFWAPI DrawDibDraw16(HDRAWDIB16 hdd, HDC16 hdc, INT16 xDst, INT16 yDst,
			    INT16 dxDst, INT16 dyDst, LPBITMAPINFOHEADER lpbi,
			    LPVOID lpBits, INT16 xSrc, INT16 ySrc, INT16 dxSrc,
			    INT16 dySrc, UINT16 wFlags)
{
    return DrawDibDraw(HDRAWDIB_32(hdd), HDC_32(hdc), xDst, yDst, dxDst,
		       dyDst, lpbi, lpBits, xSrc, ySrc, dxSrc, dySrc, wFlags);
}

/***********************************************************************
 *              DrawDibGetPalette       [MSVIDEO.108]
 */
HPALETTE16 VFWAPI DrawDibGetPalette16(HDRAWDIB16 hdd)
{
    return HPALETTE_16(DrawDibGetPalette(HDRAWDIB_32(hdd)));
}

/***********************************************************************
 *              DrawDibSetPalette       [MSVIDEO.110]
 */
BOOL16 VFWAPI DrawDibSetPalette16(HDRAWDIB16 hdd, HPALETTE16 hpal)
{
    return DrawDibSetPalette(HDRAWDIB_32(hdd), HPALETTE_32(hpal));
}

/***********************************************************************
 *              DrawDibRealize          [MSVIDEO.112]
 */
UINT16 VFWAPI DrawDibRealize16(HDRAWDIB16 hdd, HDC16 hdc,
			       BOOL16 fBackground)
{
    return (UINT16) DrawDibRealize(HDRAWDIB_32(hdd), HDC_32(hdc), fBackground);
}

/*************************************************************************
 *		DrawDibStart		[MSVIDEO.118]
 */
BOOL16 VFWAPI DrawDibStart16(HDRAWDIB16 hdd, DWORD rate)
{
    return DrawDibStart(HDRAWDIB_32(hdd), rate);
}

/*************************************************************************
 *		DrawDibStop		[MSVIDEO.119]
 */
BOOL16 DrawDibStop16(HDRAWDIB16 hdd)
{
    return DrawDibStop(HDRAWDIB_32(hdd));
}

/***********************************************************************
 *		ICOpen				[MSVIDEO.203]
 */
HIC16 VFWAPI ICOpen16(DWORD fccType, DWORD fccHandler, UINT16 wMode)
{
    return HIC_16(ICOpen(fccType, fccHandler, wMode));
}

/***********************************************************************
 *		ICClose			[MSVIDEO.204]
 */
LRESULT WINAPI ICClose16(HIC16 hic)
{
    return ICClose(HIC_32(hic));
}

/***********************************************************************
 *		_ICMessage			[MSVIDEO.207]
 */
LRESULT VFWAPIV ICMessage16(void)
{
    HIC16 hic;
    UINT16 msg;
    UINT16 cb;
    LPWORD lpData;
    SEGPTR segData;
    LRESULT ret;
    UINT16 i;

    VA_LIST16 valist;

    VA_START16(valist);
    hic = VA_ARG16(valist, HIC16);
    msg = VA_ARG16(valist, UINT16);
    cb = VA_ARG16(valist, UINT16);

    lpData = HeapAlloc(GetProcessHeap(), 0, cb);

    TRACE("0x%08lx, %u, %u, ...)\n", (DWORD) hic, msg, cb);

    for (i = 0; i < cb / sizeof(WORD); i++) {
	lpData[i] = VA_ARG16(valist, WORD);
    }

    VA_END16(valist);
    segData = MapLS(lpData);
    ret = ICSendMessage16(hic, msg, segData, (DWORD) cb);
    UnMapLS(segData);
    HeapFree(GetProcessHeap(), 0, lpData);
    return ret;
}

/***********************************************************************
 *		ICGetInfo			[MSVIDEO.212]
 */
LRESULT VFWAPI ICGetInfo16(HIC16 hic, ICINFO16 * picinfo, DWORD cb)
{
    LRESULT ret;

    TRACE("(0x%08lx,%p,%ld)\n", (DWORD) hic, picinfo, cb);
    ret = ICSendMessage16(hic, ICM_GETINFO, (DWORD) picinfo, cb);
    TRACE("	-> 0x%08lx\n", ret);
    return ret;
}

/***********************************************************************
 *		ICLocate			[MSVIDEO.213]
 */
HIC16 VFWAPI ICLocate16(DWORD fccType, DWORD fccHandler,
			LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut,
		       	WORD wFlags)
{
    return HIC_16(ICLocate(fccType, fccHandler, lpbiIn, lpbiOut, wFlags));
}

/***********************************************************************
 *		_ICCompress			[MSVIDEO.224]
 */
DWORD VFWAPIV ICCompress16(HIC16 hic, DWORD dwFlags,
			   LPBITMAPINFOHEADER lpbiOutput, LPVOID lpData,
			   LPBITMAPINFOHEADER lpbiInput, LPVOID lpBits,
			   LPDWORD lpckid, LPDWORD lpdwFlags,
			   LONG lFrameNum, DWORD dwFrameSize,
			   DWORD dwQuality, LPBITMAPINFOHEADER lpbiPrev,
			   LPVOID lpPrev)
{

    DWORD ret;
    ICCOMPRESS iccmp;
    SEGPTR seg_iccmp;

    TRACE("(0x%08lx,%ld,%p,%p,%p,%p,...)\n", (DWORD) hic, dwFlags,
	  lpbiOutput, lpData, lpbiInput, lpBits);

    iccmp.dwFlags = dwFlags;

    iccmp.lpbiOutput = lpbiOutput;
    iccmp.lpOutput = lpData;
    iccmp.lpbiInput = lpbiInput;
    iccmp.lpInput = lpBits;

    iccmp.lpckid = lpckid;
    iccmp.lpdwFlags = lpdwFlags;
    iccmp.lFrameNum = lFrameNum;
    iccmp.dwFrameSize = dwFrameSize;
    iccmp.dwQuality = dwQuality;
    iccmp.lpbiPrev = lpbiPrev;
    iccmp.lpPrev = lpPrev;
    seg_iccmp = MapLS(&iccmp);
    ret = ICSendMessage16(hic, ICM_COMPRESS, seg_iccmp, sizeof(ICCOMPRESS));
    UnMapLS(seg_iccmp);
    return ret;
}

/***********************************************************************
 *		_ICDecompress			[MSVIDEO.230]
 */
DWORD VFWAPIV ICDecompress16(HIC16 hic, DWORD dwFlags,
			     LPBITMAPINFOHEADER lpbiFormat, LPVOID lpData,
			     LPBITMAPINFOHEADER lpbi, LPVOID lpBits)
{
    ICDECOMPRESS icd;
    SEGPTR segptr;
    DWORD ret;

    TRACE("(0x%08lx,%ld,%p,%p,%p,%p)\n", (DWORD) hic, dwFlags, lpbiFormat,
	  lpData, lpbi, lpBits);

    icd.dwFlags = dwFlags;
    icd.lpbiInput = lpbiFormat;
    icd.lpInput = lpData;
    icd.lpbiOutput = lpbi;
    icd.lpOutput = lpBits;
    icd.ckid = 0;
    segptr = MapLS(&icd);
    ret = ICSendMessage16(hic, ICM_DECOMPRESS, segptr, sizeof(ICDECOMPRESS));
    UnMapLS(segptr);
    return ret;
}

/***********************************************************************
 *		_ICDrawBegin		[MSVIDEO.232]
 */
DWORD VFWAPIV ICDrawBegin16(HIC16 hic,		/* [in] */
			    DWORD dwFlags,	/* [in] flags */
			    HPALETTE16 hpal,	/* [in] palette to draw with */
			    HWND16 hwnd,	/* [in] window to draw to */
			    HDC16 hdc,		/* [in] HDC to draw to */
			    INT16 xDst,		/* [in] destination rectangle */
			    INT16 yDst,		/* [in] */
			    INT16 dxDst,	/* [in] */
			    INT16 dyDst,	/* [in] */
			    LPBITMAPINFOHEADER lpbi,	/* [in] format of frame to draw NOTE: SEGPTR */
			    INT16 xSrc,		/* [in] source rectangle */
			    INT16 ySrc,		/* [in] */
			    INT16 dxSrc,	/* [in] */
			    INT16 dySrc,	/* [in] */
			    DWORD dwRate,	/* [in] frames/second = (dwRate/dwScale) */
			    DWORD dwScale)	/* [in] */
{
    DWORD ret;
    ICDRAWBEGIN16 icdb;
    SEGPTR seg_icdb;

    TRACE ("(0x%08lx,%ld,0x%08lx,0x%08lx,0x%08lx,%u,%u,%u,%u,%p,%u,%u,%u,%u,%ld,%ld)\n",
	   (DWORD) hic, dwFlags, (DWORD) hpal, (DWORD) hwnd, (DWORD) hdc,
	   xDst, yDst, dxDst, dyDst, lpbi, xSrc, ySrc, dxSrc, dySrc, dwRate,
	   dwScale);

    icdb.dwFlags = dwFlags;
    icdb.hpal = hpal;
    icdb.hwnd = hwnd;
    icdb.hdc = hdc;
    icdb.xDst = xDst;
    icdb.yDst = yDst;
    icdb.dxDst = dxDst;
    icdb.dyDst = dyDst;
    icdb.lpbi = lpbi;		/* Keep this as SEGPTR for the mapping code to deal with */
    icdb.xSrc = xSrc;
    icdb.ySrc = ySrc;
    icdb.dxSrc = dxSrc;
    icdb.dySrc = dySrc;
    icdb.dwRate = dwRate;
    icdb.dwScale = dwScale;
    seg_icdb = MapLS(&icdb);
    ret = (DWORD) ICSendMessage16(hic, ICM_DRAW_BEGIN, seg_icdb,
				  sizeof(ICDRAWBEGIN16));
    UnMapLS(seg_icdb);
    return ret;
}

/***********************************************************************
 *		_ICDraw			[MSVIDEO.234]
 */
DWORD VFWAPIV ICDraw16(HIC16 hic, DWORD dwFlags,
		       LPVOID lpFormat,	/* [???] NOTE: SEGPTR */
		       LPVOID lpData,	/* [???] NOTE: SEGPTR */
		       DWORD cbData, LONG lTime)
{
    DWORD ret;
    ICDRAW icd;
    SEGPTR seg_icd;

    TRACE("(0x%08lx,0x%08lx,%p,%p,%ld,%ld)\n", (DWORD) hic, dwFlags,
	  lpFormat, lpData, cbData, lTime);
    icd.dwFlags = dwFlags;
    icd.lpFormat = lpFormat;
    icd.lpData = lpData;
    icd.cbData = cbData;
    icd.lTime = lTime;
    seg_icd = MapLS(&icd);
    ret = ICSendMessage16(hic, ICM_DRAW, seg_icd, sizeof(ICDRAW));
    UnMapLS(seg_icd);
    return ret;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVIDEO.239]
 */
HIC16 VFWAPI ICGetDisplayFormat16(HIC16 hic, LPBITMAPINFOHEADER lpbiIn,
				  LPBITMAPINFOHEADER lpbiOut, INT16 depth,
				  INT16 dx, INT16 dy)
{
    return HIC_16(ICGetDisplayFormat(HIC_32(hic), lpbiIn, lpbiOut, depth,
				     dx, dy));
}
