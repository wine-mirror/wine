/*				   
 * Copyright 1998 Marcus Meissner
 */
#include <stdio.h>
#include <strings.h>

#include "windows.h"
#include "driver.h"
#include "mmsystem.h"
#include "ole2.h"
#include "vfw.h"
#include "debug.h"

/****************************************************************************
 *		VideoForWindowsVersion		[MSVFW32.2][MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI
VideoForWindowsVersion(void) {
	return 0x040003B6; /* 4.950 */
}

/* system.ini: [drivers32] */

/**************************************************************************
 *		ICInfo				[MSVFW32.33]
 * Get information about an installable compressor. Return TRUE if there
 * is one.
 */
BOOL32 WINAPI
ICInfo32(
	DWORD fccType,		/* [in] type of compressor ('vidc') */
	DWORD fccHandler,	/* [in] <n>th compressor */
	ICINFO32 *lpicinfo	/* [out] information about compressor */
) {
	char	type[5],buf[2000];

	memcpy(type,&fccType,4);type[4]=0;
	TRACE(msvideo,"(%s,%ld,%p).\n",type,fccHandler,lpicinfo);
	/* does OpenDriver/CloseDriver */
	lpicinfo->dwSize = sizeof(ICINFO32);
	lpicinfo->fccType = fccType;
	lpicinfo->dwFlags = 0;
	if (GetPrivateProfileString32A("drivers32",NULL,NULL,buf,2000,"system.ini")) {
		char *s = buf;
		while (*s) {
			if (!lstrncmpi32A(type,s,4)) {
				if(!fccHandler--) {
					lpicinfo->fccHandler = mmioStringToFOURCC32A(s+5,0);
					return TRUE;
				}
			}
			s=s+lstrlen32A(s)+1; /* either next char or \0 */
		}
	}
	return FALSE;
}

/**************************************************************************
 *		ICOpen				[MSVFW32.37]
 * Opens an installable compressor. Return special handle.
 */
HIC32 WINAPI
ICOpen32(DWORD fccType,DWORD fccHandler,UINT32 wMode) {
	char		type[5],handler[5],codecname[20];
	ICOPEN		icopen;
	HDRVR32		hdrv;
	WINE_HIC	*whic;

	memcpy(type,&fccType,4);type[4]=0;
	memcpy(handler,&fccHandler,4);handler[4]=0;
	TRACE(msvideo,"(%s,%s,0x%08lx)\n",type,handler,(DWORD)wMode);
	sprintf(codecname,"%s.%s",type,handler);

	/* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the 
	 * same layout as ICOPEN
	 */
	icopen.fccType		= fccType;
	icopen.fccHandler	= fccHandler;
	icopen.dwSize		= sizeof(ICOPEN);
	icopen.dwFlags		= wMode;
	/* FIXME: do we need to fill out the rest too? */
	hdrv=OpenDriver32A(codecname,"drivers32",(LPARAM)&icopen);
	if (!hdrv) {
	    if (!strcasecmp(type,"vids")) {
		sprintf(codecname,"vidc.%s",handler);
		fccType = mmioFOURCC('v','i','d','c');
	    }
	    hdrv=OpenDriver32A(codecname,"drivers32",(LPARAM)&icopen);
	    return 0;
	}
	whic = HeapAlloc(GetProcessHeap(),0,sizeof(WINE_HIC));
	whic->hdrv	= hdrv;
#if 0
	whic->driverproc= GetProcAddress32(GetDriverModuleHandle32(hdrv),"DriverProc");
	whic->private	= whic->driverproc(0,hdrv,DRV_OPEN,0,&icopen);
#endif
	return (HIC32)whic;
}

LRESULT WINAPI
ICGetInfo32(HIC32 hic,ICINFO32 *picinfo,DWORD cb) {
	LRESULT		ret;

	TRACE(msvideo,"(0x%08lx,%p,%ld)\n",(DWORD)hic,picinfo,cb);
	ret = ICSendMessage32(hic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE(msvideo,"	-> 0x%08lx\n",ret);
	return ret;
}

HIC32  VFWAPI
ICLocate32(
	DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
	LPBITMAPINFOHEADER lpbiOut, WORD wMode
) {
	char	type[5],handler[5];
	HIC32	hic;
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
		FIXME(msvideo,"Unknown mode (%d)\n",wMode);
		return 0;
	}

	/* Easy case: handler/type match, we just fire a query and return */
	hic = ICOpen32(fccType,fccHandler,wMode);
	if (hic) {
		if (!ICSendMessage32(hic,querymsg,(DWORD)lpbiIn,(DWORD)lpbiOut))
			return hic;
		ICClose32(hic);
	}
	type[4]='\0';memcpy(type,&fccType,4);
	handler[4]='\0';memcpy(handler,&fccHandler,4);
	FIXME(msvideo,"(%s,%s,%p,%p,0x%04x),unhandled!\n",type,handler,lpbiIn,lpbiOut,wMode);
	return 0;
}

DWORD VFWAPIV
ICCompress32(
	HIC32 hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
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
	return ICSendMessage32(hic,ICM_COMPRESS,(LPARAM)&iccmp,sizeof(iccmp));
}

DWORD VFWAPIV 
ICDecompress32(HIC32 hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,LPVOID lpData,LPBITMAPINFOHEADER  lpbi,LPVOID lpBits) {
	ICDECOMPRESS	icd;

	icd.dwFlags	= dwFlags;
	icd.lpbiInput	= lpbiFormat;
	icd.lpInput	= lpData;

	icd.lpbiOutput	= lpbi;
	icd.lpOutput	= lpBits;
	/* 
	icd.ckid	= ??? ckid from AVI file? how do we get it? ;
	 */
	return ICSendMessage32(hic,ICM_DECOMPRESS,(LPARAM)&icd,sizeof(icd));
}

LRESULT VFWAPI
ICSendMessage32(HIC32 hic,UINT32 msg,DWORD lParam1,DWORD lParam2) {
	LRESULT		ret;
	WINE_HIC	*whic = (WINE_HIC*)hic;

#define XX(x) case x: TRACE(msvideo,"(0x%08lx,"#x",0x%08lx,0x%08lx)\n",(DWORD)hic,lParam1,lParam2);break;

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
		FIXME(msvideo,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",(DWORD)hic,(DWORD)msg,lParam1,lParam2);
	}
	ret = SendDriverMessage32(whic->hdrv,msg,lParam1,lParam2);
/*	ret = whic->driverproc(whic->private,whic->hdrv,msg,lParam1,lParam2);*/
	TRACE(msvideo,"	-> 0x%08lx\n",ret);
	return ret;
}

DWORD	VFWAPIV	ICDrawBegin32(
        HIC32			hic,
        DWORD			dwFlags,/* flags */
        HPALETTE32		hpal,	/* palette to draw with */
        HWND32			hwnd,	/* window to draw to */
        HDC32			hdc,	/* HDC to draw to */
        INT32			xDst,	/* destination rectangle */
        INT32			yDst,
        INT32			dxDst,
        INT32			dyDst,
        LPBITMAPINFOHEADER	lpbi,	/* format of frame to draw */
        INT32			xSrc,	/* source rectangle */
        INT32			ySrc,
        INT32			dxSrc,
        INT32			dySrc,
        DWORD			dwRate,	/* frames/second = (dwRate/dwScale) */
        DWORD			dwScale) {
		return 0;
}

LRESULT WINAPI ICClose32(HIC32 hic) {
	WINE_HIC	*whic = (WINE_HIC*)hic;
	TRACE(msvideo,"(%d).\n",hic);
	/* FIXME: correct? */
	CloseDriver32(whic->hdrv,0,0);
	HeapFree(GetProcessHeap(),0,whic);
	return 0;
}

HANDLE32 /* HDRAWDIB */ WINAPI
DrawDibOpen32( void ) {
	FIXME(msvideo,"stub!\n");
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32 (HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPVOID szFile)
{	FIXME(msvideo,"%x %x %lx %p\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32A(HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPCSTR szFile)
{	FIXME(msvideo,"%x %x %lx %s\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32W(HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPCWSTR szFile)
{	FIXME(msvideo,"%x %x %lx %s\n",hwndParent, hInstance, dwStyle, debugstr_w(szFile));
	return 0;
}
