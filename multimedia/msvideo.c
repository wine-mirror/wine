/*				   
 * Copyright 1998 Marcus Meissner
 */
#include <stdio.h>

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
	TRACE(mmsys,"(%s,%ld,%p).\n",type,fccHandler,lpicinfo);
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
	TRACE(mmsys,"(%s,%s,0x%08lx)\n",type,handler,(DWORD)wMode);
	sprintf(codecname,"%s.%s",type,handler);
	hdrv=OpenDriver32A(codecname,"drivers32",0);
	if (!hdrv)
		return 0;
	whic = HeapAlloc(GetProcessHeap(),0,sizeof(WINE_HIC));
	whic->hdrv	= hdrv;
	whic->driverproc= GetProcAddress32(GetDriverModuleHandle32(hdrv),"DriverProc");
	/* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the 
	 * same layout as ICOPEN
	 */
	icopen.fccType		= fccType;
	icopen.fccHandler	= fccHandler;
	icopen.dwSize		= sizeof(ICOPEN);
	/* FIXME: fill out rest too... */
	whic->private	= whic->driverproc(0,hdrv,DRV_OPEN,0,&icopen);
	return (HIC32)whic;
}

LRESULT WINAPI
ICGetInfo32(HIC32 hic,ICINFO32 *picinfo,DWORD cb) {
	LRESULT		ret;
	WINE_HIC	*whic = (WINE_HIC*)hic;

	TRACE(mmsys,"(0x%08lx,%p,%ld)\n",(DWORD)hic,picinfo,cb);
	ret = ICSendMessage32(whic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE(mmsys,"	-> 0x%08lx\n",ret);
	return ret;
}


HIC32 WINAPI
ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
	 LPBITMAPINFOHEADER lpbiOut, WORD wFlags
) {
	FIXME(mmsys,"stub!\n");
	return 0;
}

LRESULT VFWAPI
ICSendMessage32(HIC32 hic,UINT32 msg,DWORD lParam1,DWORD lParam2) {
	LRESULT		ret;
	WINE_HIC	*whic = (WINE_HIC*)hic;

	switch (msg) {
	case ICM_GETINFO:
		FIXME(mmsys,"(0x%08lx,ICM_GETINFO,0x%08lx,0x%08lx)\n",(DWORD)hic,lParam1,lParam2);
		break;
	default:
		FIXME(mmsys,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",(DWORD)hic,(DWORD)msg,lParam1,lParam2);
	}
	ret = whic->driverproc(whic->private,whic->hdrv,msg,lParam1,lParam2);
	FIXME(mmsys,"	-> 0x%08lx\n",ret);
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

HANDLE32 /* HDRAWDIB */ WINAPI
DrawDibOpen32( void ) {
	FIXME(mmsys,"stub!\n");
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32 (HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPVOID szFile)
{	FIXME(mmsys,"%x %x %lx %p\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32A(HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPCSTR szFile)
{	FIXME(mmsys,"%x %x %lx %s\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}
HWND32 VFWAPIV MCIWndCreate32W(HWND32 hwndParent, HINSTANCE32 hInstance,
                      DWORD dwStyle,LPCWSTR szFile)
{	FIXME(mmsys,"%x %x %lx %s\n",hwndParent, hInstance, dwStyle, debugstr_w(szFile));
	return 0;
}
