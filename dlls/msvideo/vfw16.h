#ifndef __WINE_VFW16_H
#define __WINE_VFW16_H

#include "vfw.h"
#include "wine/windef16.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef HANDLE16 HDRAWDIB16;

#include "pshpack1.h"

typedef struct {
	DWORD dwSize;
	DWORD fccType;
	DWORD fccHandler;
	DWORD dwFlags;
	DWORD dwVersion;
	DWORD dwVersionICM;
	/*
	 * under Win16, normal chars are used
	 */
	CHAR szName[16];
	CHAR szDescription[128];
	CHAR szDriver[128];
} ICINFO16;

typedef struct {
    DWORD		dwFlags;
    LPBITMAPINFOHEADER	lpbiSrc;
    LPVOID		lpSrc;
    LPBITMAPINFOHEADER	lpbiDst;
    LPVOID		lpDst;

    INT16  		xDst;       /* destination rectangle */
    INT16		yDst;
    INT16  		dxDst;
    INT16  		dyDst;

    INT16		xSrc;       /* source rectangle */
    INT16  		ySrc;
    INT16		dxSrc;
    INT16  		dySrc;
} ICDECOMPRESSEX16;

typedef struct {
	DWORD		dwFlags;
	HPALETTE16	hpal;
	HWND16		hwnd;
	HDC16		hdc;
	INT16		xDst;
	INT16		yDst;
	INT16		dxDst;
	INT16		dyDst;
	LPBITMAPINFOHEADER	lpbi;
	INT16		xSrc;
	INT16		ySrc;
	INT16		dxSrc;
	INT16		dySrc;
	DWORD		dwRate;
	DWORD		dwScale;
} ICDRAWBEGIN16;

#include "poppack.h"


LRESULT VFWAPI	ICSendMessage16(HIC16 hic, UINT16 msg, DWORD dw1, DWORD dw2);
HIC16   VFWAPI	ICOpen16(DWORD fccType, DWORD fccHangler, UINT16 wMode);
HIC16	VFWAPI	ICLocate16(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);

typedef struct {
	DWORD dwFlags;
	LPBITMAPINFOHEADER lpbiIn;
	LPBITMAPINFOHEADER lpbiSuggest;
	INT16 dxSrc;
	INT16 dySrc;
	INT16 dxDst;
	INT16 dyDst;
	HIC16 hicDecompressor;
} ICDRAWSUGGEST16;

DWORD	VFWAPIV	ICDrawBegin16(
        HIC16			hic,
        DWORD			dwFlags,/* flags */
        HPALETTE16		hpal,	/* palette to draw with */
        HWND16			hwnd,	/* window to draw to */
        HDC16			hdc,	/* HDC to draw to */
	INT16			xDst,	/* destination rectangle */
        INT16			yDst,
        INT16			dxDst,
        INT16			dyDst,
        LPBITMAPINFOHEADER	lpbi,	/* format of frame to draw */
        INT16			xSrc,	/* source rectangle */
        INT16			ySrc,
        INT16			dxSrc,
        INT16			dySrc,
        DWORD			dwRate,	/* frames/second = (dwRate/dwScale) */
        DWORD			dwScale
);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __WINE_VFW16_H */
