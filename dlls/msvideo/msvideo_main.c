/*
 * Copyright 1998 Marcus Meissner
 */
#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"
#include "wine/winestring.h"
#include "driver.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msvideo)

/***********************************************************************
 *		VideoForWindowsVersion		[MSVFW.2][MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI
VideoForWindowsVersion(void) {
	return 0x040003B6; /* 4.950 */
}

/***********************************************************************
 *		VideoCapDriverDescAndVer	[MSVIDEO.22]
 */
DWORD WINAPI
VideoCapDriverDescAndVer(WORD nr,LPVOID buf1,WORD buf1len,LPVOID buf2,WORD buf2len) {
	FIXME("(%d,%p,%d,%p,%d), stub!\n",nr,buf1,buf1len,buf2,buf2len);
	return 0;
}

/* system.ini: [drivers] */

/***********************************************************************
 *		ICInfo				[MSVFW.33]
 * Get information about an installable compressor. Return TRUE if there
 * is one.
 */
BOOL WINAPI
ICInfo(
	DWORD fccType,		/* [in] type of compressor ('vidc') */
	DWORD fccHandler,	/* [in] <n>th compressor */
	ICINFO *lpicinfo	/* [out] information about compressor */
) {
	char	type[5],buf[2000];

	memcpy(type,&fccType,4);type[4]=0;
	TRACE("(%s,%ld,%p).\n",type,fccHandler,lpicinfo);
	/* does OpenDriver/CloseDriver */
	lpicinfo->dwSize = sizeof(ICINFO);
	lpicinfo->fccType = fccType;
	lpicinfo->dwFlags = 0;
	if (GetPrivateProfileStringA("drivers32",NULL,NULL,buf,2000,"system.ini")) {
		char *s = buf;
		while (*s) {
			if (!lstrncmpiA(type,s,4)) {
				if(!fccHandler--) {
					lpicinfo->fccHandler = mmioStringToFOURCCA(s+5,0);
					return TRUE;
				}
			}
			s=s+lstrlenA(s)+1; /* either next char or \0 */
		}
	}
	return FALSE;
}

/***********************************************************************
 *		ICOpen				[MSVFW.37]
 * Opens an installable compressor. Return special handle.
 */
HIC WINAPI
ICOpen(DWORD fccType,DWORD fccHandler,UINT wMode) {
	char		type[5],handler[5],codecname[20];
	ICOPEN		icopen;
	HDRVR		hdrv;
	WINE_HIC	*whic;

	memcpy(type,&fccType,4);type[4]=0;
	memcpy(handler,&fccHandler,4);handler[4]=0;
	TRACE("(%s,%s,0x%08lx)\n",type,handler,(DWORD)wMode);
	sprintf(codecname,"%s.%s",type,handler);

	/* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the 
	 * same layout as ICOPEN
	 */
	icopen.fccType		= fccType;
	icopen.fccHandler	= fccHandler;
	icopen.dwSize		= sizeof(ICOPEN);
	icopen.dwFlags		= wMode;
	/* FIXME: do we need to fill out the rest too? */
	hdrv=OpenDriverA(codecname,"drivers32",(LPARAM)&icopen);
	if (!hdrv) {
	    if (!strcasecmp(type,"vids")) {
		sprintf(codecname,"vidc.%s",handler);
		fccType = mmioFOURCC('v','i','d','c');
	    }
	    hdrv=OpenDriverA(codecname,"drivers32",(LPARAM)&icopen);
	    if (!hdrv)
		    return 0;
	}
	whic = HeapAlloc(GetProcessHeap(),0,sizeof(WINE_HIC));
	whic->hdrv	= hdrv;
	whic->driverproc= NULL;
	whic->private	= ICSendMessage((HIC)whic,DRV_OPEN,0,(LPARAM)&icopen);
	return (HIC)whic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW.38]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode,
FARPROC lpfnHandler) {
	char		type[5],handler[5];
	HIC		hic;
	WINE_HIC	*whic;

	memcpy(type,&fccType,4);type[4]=0;
	memcpy(handler,&fccHandler,4);handler[4]=0;
	FIXME("(%s,%s,%d,%p), stub!\n",type,handler,wMode,lpfnHandler);
	hic = ICOpen(fccType,fccHandler,wMode);
	if (!hic)
		return hic;
	whic = (WINE_HIC*)hic;
	whic->driverproc = lpfnHandler;
	return hic;
}


/***********************************************************************
 *		ICGetInfo			[MSVFW.30]
 */
LRESULT WINAPI
ICGetInfo(HIC hic,ICINFO *picinfo,DWORD cb) {
	LRESULT		ret;

	TRACE("(0x%08lx,%p,%ld)\n",(DWORD)hic,picinfo,cb);
	ret = ICSendMessage(hic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICLocate			[MSVFW.35]
 */
HIC  VFWAPI
ICLocate(
	DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
	LPBITMAPINFOHEADER lpbiOut, WORD wMode
) {
	char	type[5],handler[5];
	HIC	hic;
	DWORD	querymsg;

	switch (wMode) {
	case ICMODE_FASTCOMPRESS:
	case ICMODE_COMPRESS: 
		querymsg = ICM_COMPRESS_QUERY;
		break;
	case ICMODE_DECOMPRESS:
	case ICMODE_FASTDECOMPRESS:
		querymsg = ICM_DECOMPRESS_QUERY;
		break;
	case ICMODE_DRAW:
		querymsg = ICM_DRAW_QUERY;
		break;
	default:
		FIXME("Unknown mode (%d)\n",wMode);
		return 0;
	}

	/* Easy case: handler/type match, we just fire a query and return */
	hic = ICOpen(fccType,fccHandler,wMode);
	if (hic) {
		if (!ICSendMessage(hic,querymsg,(DWORD)lpbiIn,(DWORD)lpbiOut))
			return hic;
		ICClose(hic);
	}
	type[4]='\0';memcpy(type,&fccType,4);
	handler[4]='\0';memcpy(handler,&fccHandler,4);
	if (fccType==streamtypeVIDEO) {
		hic = ICLocate(ICTYPE_VIDEO,fccHandler,lpbiIn,lpbiOut,wMode);
		if (hic)
			return hic;
	}
	FIXME("(%s,%s,%p,%p,0x%04x),unhandled!\n",type,handler,lpbiIn,lpbiOut,wMode);
	return 0;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVFW.29]
 */
HIC VFWAPI ICGetDisplayFormat(
	HIC hic,LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut,
	INT depth,INT dx,INT dy
) {
	HIC	tmphic = hic; 
	LRESULT	lres;

	FIXME("(0x%08lx,%p,%p,%d,%d,%d),stub!\n",(DWORD)hic,lpbiIn,lpbiOut,depth,dx,dy);
	if (!tmphic) {
		tmphic=ICLocate(ICTYPE_VIDEO,0,lpbiIn,NULL,ICMODE_DECOMPRESS);
		if (!tmphic)
			return tmphic;
	}
	if ((dy == lpbiIn->biHeight) || (dx == lpbiIn->biWidth))
		dy = dx = 0; /* no resize needed */
	/* Can we decompress it ? */
	lres = ICDecompressQuery(tmphic,lpbiIn,NULL);
	if (lres)
		goto errout; /* no, sorry */
	ICDecompressGetFormat(hic,lpbiIn,lpbiOut);
	*lpbiOut=*lpbiIn;
	lpbiOut->biCompression = 0;
	lpbiOut->biSize = sizeof(*lpbiOut);
	if (!depth) {
		HDC	hdc;

		hdc = GetDC(0);
		depth = GetDeviceCaps(hdc,12)*GetDeviceCaps(hdc,14);
		ReleaseDC(0,hdc);
		if (depth==15)	depth = 16;
		if (depth<8)	depth =  8;
		/* more constraints and tests */
	}
	if (lpbiIn->biBitCount == 8)
		depth = 8;
	
	return hic;
errout:
	if (hic!=tmphic)
		ICClose(tmphic);
	return 0;
}

/***********************************************************************
 *		ICCompress			[MSVFW.23]
 */
DWORD VFWAPIV
ICCompress(
	HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev
) {
	ICCOMPRESS	iccmp;

	iccmp.dwFlags		= dwFlags;

	iccmp.lpbiOutput	= lpbiOutput;
	iccmp.lpOutput		= lpData;
	iccmp.lpbiInput		= lpbiInput;
	iccmp.lpInput		= lpBits;

	iccmp.lpckid		= lpckid;
	iccmp.lpdwFlags		= lpdwFlags;
	iccmp.lFrameNum		= lFrameNum;
	iccmp.dwFrameSize	= dwFrameSize;
	iccmp.dwQuality		= dwQuality;
	iccmp.lpbiPrev		= lpbiPrev;
	iccmp.lpPrev		= lpPrev;
	return ICSendMessage(hic,ICM_COMPRESS,(LPARAM)&iccmp,sizeof(iccmp));
}

/***********************************************************************
 *		ICDecompress			[MSVFW.26]
 */
DWORD VFWAPIV 
ICDecompress(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,LPVOID lpData,LPBITMAPINFOHEADER  lpbi,LPVOID lpBits) {
	ICDECOMPRESS	icd;

	icd.dwFlags	= dwFlags;
	icd.lpbiInput	= lpbiFormat;
	icd.lpInput	= lpData;

	icd.lpbiOutput	= lpbi;
	icd.lpOutput	= lpBits;
	icd.ckid	= 0;
	return ICSendMessage(hic,ICM_DECOMPRESS,(LPARAM)&icd,sizeof(icd));
}

/***********************************************************************
 *		ICSendMessage			[MSVFW.40]
 */
LRESULT VFWAPI
ICSendMessage(HIC hic,UINT msg,DWORD lParam1,DWORD lParam2) {
	LRESULT		ret;
	WINE_HIC	*whic = (WINE_HIC*)hic;

#define XX(x) case x: TRACE("(0x%08lx,"#x",0x%08lx,0x%08lx)\n",(DWORD)hic,lParam1,lParam2);break;

	switch (msg) {
	XX(ICM_ABOUT)
	XX(ICM_GETINFO)
	XX(ICM_COMPRESS_FRAMES_INFO)
	XX(ICM_COMPRESS_GET_FORMAT)
	XX(ICM_COMPRESS_GET_SIZE)
	XX(ICM_COMPRESS_QUERY)
	XX(ICM_COMPRESS_BEGIN)
	XX(ICM_COMPRESS)
	XX(ICM_COMPRESS_END)
	XX(ICM_DECOMPRESS_GET_FORMAT)
	XX(ICM_DECOMPRESS_QUERY)
	XX(ICM_DECOMPRESS_BEGIN)
	XX(ICM_DECOMPRESS)
	XX(ICM_DECOMPRESS_END)
	XX(ICM_DECOMPRESS_SET_PALETTE)
	XX(ICM_DECOMPRESS_GET_PALETTE)
	XX(ICM_DRAW_QUERY)
	XX(ICM_DRAW_BEGIN)
	XX(ICM_DRAW_GET_PALETTE)
	XX(ICM_DRAW_START)
	XX(ICM_DRAW_STOP)
	XX(ICM_DRAW_END)
	XX(ICM_DRAW_GETTIME)
	XX(ICM_DRAW)
	XX(ICM_DRAW_WINDOW)
	XX(ICM_DRAW_SETTIME)
	XX(ICM_DRAW_REALIZE)
	XX(ICM_DRAW_FLUSH)
	XX(ICM_DRAW_RENDERBUFFER)
	XX(ICM_DRAW_START_PLAY)
	XX(ICM_DRAW_STOP_PLAY)
	XX(ICM_DRAW_SUGGESTFORMAT)
	XX(ICM_DRAW_CHANGEPALETTE)
	XX(ICM_GETBUFFERSWANTED)
	XX(ICM_GETDEFAULTKEYFRAMERATE)
	XX(ICM_DECOMPRESSEX_BEGIN)
	XX(ICM_DECOMPRESSEX_QUERY)
	XX(ICM_DECOMPRESSEX)
	XX(ICM_DECOMPRESSEX_END)
	XX(ICM_SET_STATUS_PROC)
	default:
		FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",(DWORD)hic,(DWORD)msg,lParam1,lParam2);
	}
#if 0
	if (whic->driverproc) {
		FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx), calling %p\n",(DWORD)hic,(DWORD)msg,lParam1,lParam2,whic->driverproc);
		ret = whic->driverproc(whic->hdrv,1,msg,lParam1,lParam2);
	} else
#endif
		ret = SendDriverMessage(whic->hdrv,msg,lParam1,lParam2);
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICDrawBegin		[MSVFW.28]
 */
DWORD	VFWAPIV	ICDrawBegin(
        HIC			hic,
        DWORD			dwFlags,/* flags */
        HPALETTE		hpal,	/* palette to draw with */
        HWND			hwnd,	/* window to draw to */
        HDC			hdc,	/* HDC to draw to */
        INT			xDst,	/* destination rectangle */
        INT			yDst,
        INT			dxDst,
        INT			dyDst,
        LPBITMAPINFOHEADER	lpbi,	/* format of frame to draw */
        INT			xSrc,	/* source rectangle */
        INT			ySrc,
        INT			dxSrc,
        INT			dySrc,
        DWORD			dwRate,	/* frames/second = (dwRate/dwScale) */
        DWORD			dwScale) {
	ICDRAWBEGIN	icdb;

	icdb.dwFlags = dwFlags;
	icdb.hpal = hpal;
	icdb.hwnd = hwnd;
	icdb.hdc = hdc;
	icdb.xDst = xDst;
	icdb.yDst = yDst;
	icdb.dxDst = dxDst;
	icdb.dyDst = dyDst;
	icdb.lpbi = lpbi;
	icdb.xSrc = xSrc;
	icdb.ySrc = ySrc;
	icdb.dxSrc = dxSrc;
	icdb.dySrc = dySrc;
	icdb.dwRate = dwRate;
	icdb.dwScale = dwScale;
	return ICSendMessage(hic,ICM_DRAW_BEGIN,(LPARAM)&icdb,sizeof(icdb));
}

/***********************************************************************
 *		ICDraw			[MSVFW.27]
 */
DWORD VFWAPIV ICDraw(
	HIC hic,DWORD dwFlags,LPVOID lpFormat,LPVOID lpData,DWORD cbData,
	LONG lTime
) {
	ICDRAW	icd;

	icd.dwFlags = dwFlags;
	icd.lpFormat = lpFormat;
	icd.lpData = lpData;
	icd.cbData = cbData;
	icd.lTime = lTime;
	return ICSendMessage(hic,ICM_DRAW,(LPARAM)&icd,sizeof(icd));
}

/***********************************************************************
 *		ICClose			[MSVFW.22]
 */
LRESULT WINAPI ICClose(HIC hic) {
	WINE_HIC	*whic = (WINE_HIC*)hic;
	TRACE("(%d).\n",hic);
	/* FIXME: correct? */
	CloseDriver(whic->hdrv,0,0);
	HeapFree(GetProcessHeap(),0,whic);
	return 0;
}

/***********************************************************************
 *		DrawDibOpen		[MSVFW.10]
 */
HDRAWDIB VFWAPI
DrawDibOpen( void ) {
	FIXME("stub!\n");
	return 0xdead;
}

/***********************************************************************
 *		DrawDibClose		[MSVFW.5]
 */
BOOL VFWAPI
DrawDibClose( HDRAWDIB hDib ) {
       FIXME("stub!\n");
       return TRUE;
}

/***********************************************************************
 *		DrawDibBegin		[MSVFW.3]
 */
BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd,
                                    HDC      hdc,
                                    INT      dxDst,
                                    INT      dyDst,
                                    LPBITMAPINFOHEADER lpbi,
                                    INT      dxSrc,
                                    INT      dySrc,
                                    UINT     wFlags) {
	FIXME("(%d,0x%lx,%d,%d,%p,%d,%d,0x%08lx), stub!\n",
		hdd,(DWORD)hdc,dxDst,dyDst,lpbi,dxSrc,dySrc,(DWORD)wFlags
	);
	return TRUE;
}


/***********************************************************************
 *		DrawDibSetPalette	[MSVFW.13]
 */
BOOL VFWAPI
DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal) {
	FIXME("(%d,%d),stub!\n",hdd,hpal);
	return TRUE;
}

/***********************************************************************
 *		DrawDibRealize		[MSVFW.12]
 */
UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground) {
	FIXME("(0x%08lx,0x%08lx,%d),stub!\n",(DWORD)hdd,(DWORD)hdc,fBackground);
	return 0;
}

/***********************************************************************
 *		MCIWndCreate		[MSVFW.44]
 */
HWND VFWAPIV MCIWndCreate (HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle,LPVOID szFile)
{	FIXME("%x %x %lx %p\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}

/***********************************************************************
 *		MCIWndCreateA		[MSVFW.45]
 */
HWND VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle,LPCSTR szFile)
{	FIXME("%x %x %lx %s\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}

/***********************************************************************
 *		MCIWndCreateW		[MSVFW.46]
 */
HWND VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle,LPCWSTR szFile)
{	FIXME("%x %x %lx %s\n",hwndParent, hInstance, dwStyle, debugstr_w(szFile));
	return 0;
}
