/* 
 * Copyright 2000 Bradley Baetz
 *
 * Fixme: Some flags are ignored
 * Should be doing buffering when requested
 * Handle palettes
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winbase.h"
#include "debugtools.h"
#include "driver.h"
#include "vfw.h"
#include "windef.h"

DEFAULT_DEBUG_CHANNEL(msvideo);
typedef struct {
	HDC hdc;
	INT dxDst;
	INT dyDst;
	LPBITMAPINFOHEADER lpbi;
	INT dxSrc;
	INT dySrc;
	HPALETTE hpal;				/* Palette to use for the DIB */
	BOOL begun;					/* DrawDibBegin has been called */
	LPBITMAPINFOHEADER lpbiOut;	/* Output format */
	HIC hic;					/* HIC for decompression */
	HDC hMemDC;					/* DC for buffering */
	HBITMAP hOldDib;			/* Original Dib */
	HBITMAP hDib;				/* DibSection */
	LPVOID lpvbits;				/* Buffer for holding decompressed dib */
} WINE_HDD;

/***********************************************************************
 *		DrawDibOpen		[MSVFW32.10]
 */
HDRAWDIB VFWAPI DrawDibOpen(void) {
	HDRAWDIB hdd;

	TRACE("(void)\n");
	hdd = GlobalAlloc16(GHND,sizeof(WINE_HDD));
	TRACE("=> %d\n",hdd);
	return hdd;
}

/***********************************************************************
 *		DrawDibOpen		[MSVIDEO.102]
 */
HDRAWDIB16 VFWAPI DrawDibOpen16(void) {
	return (HDRAWDIB16)DrawDibOpen();
}

/***********************************************************************
 *		DrawDibClose		[MSVFW32.5]
 */
BOOL VFWAPI DrawDibClose(HDRAWDIB hdd) {
	WINE_HDD *whdd = GlobalLock16(hdd);

	TRACE("(0x%08lx)\n",(DWORD)hdd);

	if (!whdd)
		return FALSE;

	if (whdd->begun)
		DrawDibEnd(hdd);

	GlobalUnlock16(hdd);
	GlobalFree16(hdd);
	return TRUE;
}

/***********************************************************************
 *		DrawDibClose		[MSVIDEO.103]
 */
BOOL16 VFWAPI DrawDibClose16(HDRAWDIB16 hdd) {
	return DrawDibClose(hdd);
}

/***********************************************************************
 *		DrawDibEnd		[MSVFW32.7]
 */
BOOL VFWAPI DrawDibEnd(HDRAWDIB hdd) {
	BOOL ret = TRUE;
	WINE_HDD *whdd = GlobalLock16(hdd);
	
	TRACE("(0x%08lx)\n",(DWORD)hdd);

	whdd->hpal = 0; /* Do not free this */
	whdd->hdc = 0;
	if (whdd->lpbi) {
		HeapFree(GetProcessHeap(),0,whdd->lpbi);
		whdd->lpbi = NULL;
	}
	if (whdd->lpbiOut) {
		HeapFree(GetProcessHeap(),0,whdd->lpbiOut);
		whdd->lpbiOut = NULL;
	}

	whdd->begun = FALSE;

	/*if (whdd->lpvbits)
	  HeapFree(GetProcessHeap(),0,whdd->lpvbuf);*/

	if (whdd->hMemDC) {
		SelectObject(whdd->hMemDC,whdd->hOldDib);
		DeleteDC(whdd->hMemDC);
	}

	if (whdd->hDib)
		DeleteObject(whdd->hDib);
	
	if (whdd->hic) {
		ICDecompressEnd(whdd->hic);
		ICClose(whdd->hic);
	}

	whdd->lpvbits = NULL;

	GlobalUnlock16(hdd);
	return ret;
}

/***********************************************************************
 *		DrawDibEnd		[MSVIDEO.105]
 */
BOOL16 VFWAPI DrawDibEnd16(HDRAWDIB16 hdd) {
	return DrawDibEnd(hdd);
}

/***********************************************************************
 *              DrawDibBegin            [MSVFW32.3]
 */
BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd,
						 HDC      hdc,
						 INT      dxDst,
						 INT      dyDst,
						 LPBITMAPINFOHEADER lpbi,
						 INT      dxSrc,
						 INT      dySrc,
						 UINT     wFlags) {
	BOOL ret = TRUE;
	WINE_HDD *whdd;

	TRACE("(%d,0x%lx,%d,%d,%p,%d,%d,0x%08lx)\n",
		hdd,(DWORD)hdc,dxDst,dyDst,lpbi,dxSrc,dySrc,(DWORD)wFlags
	);

	if (wFlags)
		FIXME("wFlags == 0x%08lx not handled\n",(DWORD)wFlags);

	whdd = (WINE_HDD*)GlobalLock16(hdd);
	
	if (whdd->begun)
		DrawDibEnd(hdd);

	if (lpbi->biCompression) {
		DWORD size;

		whdd->hic = ICOpen(ICTYPE_VIDEO,lpbi->biCompression,ICMODE_DECOMPRESS);
		if (!whdd->hic) {
			ERR("Could not open IC. biCompression == 0x%08lx\n",lpbi->biCompression);
			ret = FALSE;
		}

		if (ret) {
			size = ICDecompressGetFormat(whdd->hic,lpbi,NULL);
			if (size == ICERR_UNSUPPORTED) {
				FIXME("Codec doesn't support GetFormat, giving up.\n");
				ret = FALSE;
			}
		}

		if (ret) {
			whdd->lpbiOut = HeapAlloc(GetProcessHeap(),0,size);

			if (ICDecompressGetFormat(whdd->hic,lpbi,whdd->lpbiOut) != ICERR_OK)
				ret = FALSE;
		}

		if (ret) {
			/* FIXME: Use Ex functions if available? */
			if (ICDecompressBegin(whdd->hic,lpbi,whdd->lpbiOut) != ICERR_OK)
				ret = FALSE;

			TRACE("biSizeImage == %ld\n",whdd->lpbiOut->biSizeImage);
			TRACE("biCompression == %ld\n",whdd->lpbiOut->biCompression);
			TRACE("biBitCount == %d\n",whdd->lpbiOut->biBitCount);
		}
	} else {
		/* No compression */
		TRACE("Not compressed!\n");
		whdd->lpbiOut = HeapAlloc(GetProcessHeap(),0,lpbi->biSize);
		memcpy(whdd->lpbiOut,lpbi,lpbi->biSize);
	}

	if (ret) {
		/*whdd->lpvbuf = HeapAlloc(GetProcessHeap(),0,whdd->lpbiOut->biSizeImage);*/
		
		whdd->hMemDC = CreateCompatibleDC(hdc);
		TRACE("Creating: %ld,%p\n",whdd->lpbiOut->biSize,whdd->lpvbits);
		whdd->hDib = CreateDIBSection(whdd->hMemDC,(BITMAPINFO *)whdd->lpbiOut,DIB_RGB_COLORS,&(whdd->lpvbits),0,0);
		if (!whdd->hDib) {
			TRACE("Error: %ld\n",GetLastError());
		}
		TRACE("Created: %d,%p\n",whdd->hDib,whdd->lpvbits);
		whdd->hOldDib = SelectObject(whdd->hMemDC,whdd->hDib);
	}

	if (ret) {
		whdd->hdc = hdc;
		whdd->dxDst = dxDst;
		whdd->dyDst = dyDst;
		whdd->lpbi = HeapAlloc(GetProcessHeap(),0,lpbi->biSize);
		memcpy(whdd->lpbi,lpbi,lpbi->biSize);
		whdd->dxSrc = dxSrc;
		whdd->dySrc = dySrc;
		whdd->begun = TRUE;
		whdd->hpal = 0;
	} else {
		if (whdd->hic)
			ICClose(whdd->hic);
		if (whdd->lpbiOut) {
			HeapFree(GetProcessHeap(),0,whdd->lpbiOut);
			whdd->lpbiOut = NULL;
		}
	}

	GlobalUnlock16(hdd);

	return ret;
}

/************************************************************************
 *		DrawDibBegin		[MSVIDEO.104]
 */
BOOL16 VFWAPI DrawDibBegin16(HDRAWDIB16 hdd,
						   HDC16      hdc,
						   INT16      dxDst,
						   INT16      dyDst,
						   LPBITMAPINFOHEADER lpbi,
						   INT16      dxSrc,
						   INT16      dySrc,
						   UINT16     wFlags) {
	return DrawDibBegin(hdd,hdc,dxDst,dyDst,lpbi,dxSrc,dySrc,wFlags);
}

/**********************************************************************
 *		DrawDibDraw		[MSVFW32.6]
 */
BOOL VFWAPI DrawDibDraw(HDRAWDIB hdd,             
						HDC hdc,
						INT xDst,
						INT yDst,
						INT dxDst,
						INT dyDst,
						LPBITMAPINFOHEADER lpbi,
						LPVOID lpBits,
						INT xSrc,
						INT ySrc,
						INT dxSrc,
						INT dySrc,
						UINT wFlags) {
	WINE_HDD *whdd;
	BOOL ret = TRUE;

	TRACE("(%d,0x%lx,%d,%d,%d,%d,%p,%p,%d,%d,%d,%d,0x%08lx)\n",
		  hdd,(DWORD)hdc,xDst,yDst,dxDst,dyDst,lpbi,lpBits,xSrc,ySrc,dxSrc,dySrc,(DWORD)wFlags
	);

	if (wFlags & ~(DDF_SAME_HDC | DDF_SAME_DRAW | DDF_NOTKEYFRAME))
		FIXME("wFlags == 0x%08lx not handled\n",(DWORD)wFlags);

	if (!lpBits) {
		/* Undocumented? */
		lpBits = (LPSTR)lpbi + (WORD)(lpbi->biSize) + (WORD)(lpbi->biClrUsed*sizeof(RGBQUAD));
	}

	whdd = GlobalLock16(hdd);

#define CHANGED(x) (whdd->##x != ##x)

	if ((!whdd->begun) || (!(wFlags & DDF_SAME_HDC) && CHANGED(hdc)) || (!(wFlags & DDF_SAME_DRAW) &&
		 (CHANGED(lpbi) || CHANGED(dxSrc) || CHANGED(dySrc) || CHANGED(dxDst) || CHANGED(dyDst)))) {
		TRACE("Something changed!\n");
		ret = DrawDibBegin(hdd,hdc,dxDst,dyDst,lpbi,dxSrc,dySrc,0);
	}

#undef CHANGED

    if ((dxDst == -1) && (dyDst == -1)) {
		dxDst = dxSrc;
		dyDst = dySrc;
	}

	if (lpbi->biCompression) {
		DWORD flags = 0;
		
		TRACE("Compression == 0x%08lx\n",lpbi->biCompression);
		
		if (wFlags & DDF_NOTKEYFRAME)
		  flags |= ICDECOMPRESS_NOTKEYFRAME;
		
		ICDecompress(whdd->hic,flags,lpbi,lpBits,whdd->lpbiOut,whdd->lpvbits);
	} else {
		memcpy(whdd->lpvbits,lpBits,lpbi->biSizeImage);
	}

	SelectPalette(hdc,whdd->hpal,FALSE);

	StretchBlt(whdd->hdc,xDst,yDst,dxDst,dyDst,whdd->hMemDC,xSrc,ySrc,dxSrc,dySrc,SRCCOPY);

	GlobalUnlock16(hdd);
	return ret;
}

/**********************************************************************
 *		DrawDibDraw		[MSVIDEO.106]
 */
BOOL16 VFWAPI DrawDibDraw16(HDRAWDIB16 hdd,             
						  HDC16 hdc,
						  INT16 xDst,
						  INT16 yDst,
						  INT16 dxDst,
						  INT16 dyDst,
						  LPBITMAPINFOHEADER lpbi,
						  LPVOID lpBits,
						  INT16 xSrc,
						  INT16 ySrc,
						  INT16 dxSrc,
						  INT16 dySrc,
						  UINT16 wFlags) {
	return DrawDibDraw(hdd,hdc,xDst,yDst,dxDst,dyDst,lpbi,lpBits,xSrc,ySrc,dxSrc,dySrc,wFlags);
}

/*************************************************************************
 *		DrawDibStart		[MSVFW32.14]
 */
BOOL VFWAPI DrawDibStart(HDRAWDIB hdd, DWORD rate) {
	FIXME("(0x%08lx,%ld), stub\n",(DWORD)hdd,rate);
	return TRUE;
}

/*************************************************************************
 *		DrawDibStart		[MSVIDEO.118]
 */
BOOL16 VFWAPI DrawDibStart16(HDRAWDIB16 hdd, DWORD rate) {
	return DrawDibStart(hdd,rate);
}

/*************************************************************************
 *		DrawDibStop		[MSVFW32.15]
 */
BOOL VFWAPI DrawDibStop(HDRAWDIB hdd) {
	FIXME("(0x%08lx), stub\n",(DWORD)hdd);
	return TRUE;
}

/*************************************************************************
 *		DrawDibStop		[MSVIDEO.119]
 */
BOOL16 DrawDibStop16(HDRAWDIB16 hdd) {
	return DrawDibStop(hdd);
}

/***********************************************************************
 *              DrawDibSetPalette       [MSVFW32.13]   
 */
BOOL VFWAPI DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal) {
	WINE_HDD *whdd;

	TRACE("(0x%08lx,0x%08lx)\n",(DWORD)hdd,(DWORD)hpal);

	whdd = GlobalLock16(hdd);
	whdd->hpal = hpal;
	
	if (whdd->begun) {
		SelectPalette(whdd->hdc,hpal,0);
		RealizePalette(whdd->hdc);
	}
	GlobalUnlock16(hdd);
	return TRUE;
}

/***********************************************************************
 *              DrawDibSetPalette       [MSVIDEO.110]
 */
BOOL16 VFWAPI DrawDibSetPalette16(HDRAWDIB16 hdd, HPALETTE16 hpal) {
	return DrawDibSetPalette(hdd,hpal);
}

/***********************************************************************
 *              DrawDibGetPalette       [MSVFW32.9]   
 */
HPALETTE VFWAPI DrawDibGetPalette(HDRAWDIB hdd) {
	WINE_HDD *whdd;
	HPALETTE ret;

	TRACE("(0x%08lx)\n",(DWORD)hdd);

	whdd = GlobalLock16(hdd);
	ret = whdd->hpal;
	GlobalUnlock16(hdd);
	return ret;
}

/***********************************************************************
 *              DrawDibGetPalette       [MSVIDEO.108]]   
 */
HPALETTE16 VFWAPI DrawDibGetPalette16(HDRAWDIB16 hdd) {
	return (HPALETTE16)DrawDibGetPalette(hdd);
}

/***********************************************************************
 *              DrawDibRealize          [MSVFW32.12]
 */
UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground) {
	WINE_HDD *whdd;
	HPALETTE oldPal;
	UINT ret = 0;

	FIXME("(%d,0x%08lx,%d), stub\n",hdd,(DWORD)hdc,fBackground);
	
	whdd = GlobalLock16(hdd);

	if (!whdd || !(whdd->begun)) {
		ret = 0;
		goto out;
	}
	
	if (!whdd->hpal)
		whdd->hpal = CreateHalftonePalette(hdc);

	oldPal = SelectPalette(hdc,whdd->hpal,fBackground);
	ret = RealizePalette(hdc);
	
 out:
	GlobalUnlock16(hdd);

	TRACE("=> %u\n",ret);
	return ret;
}

/***********************************************************************
 *              DrawDibRealize          [MSVIDEO.112]
 */
UINT16 VFWAPI DrawDibRealize16(HDRAWDIB16 hdd, HDC16 hdc, BOOL16 fBackground) {
	return (UINT16)DrawDibRealize(hdd,hdc,fBackground);
}
