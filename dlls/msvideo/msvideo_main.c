/*
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
 *
 * FIXME: This all assumes 32 bit codecs
 *		Win95 appears to prefer 32 bit codecs, even from 16 bit code.
 *		There is the ICOpenFunction16 to worry about still, though.
 *      
 * TODO
 *      - no thread safety
 *      - the four CC comparisons are wrong on big endian machines
 */

#include <stdio.h>
#include <string.h>

#include "msvideo_private.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

LRESULT (CALLBACK *pFnCallTo16)(HDRVR, HIC, UINT, LPARAM, LPARAM) = NULL;

static WINE_HIC*        MSVIDEO_FirstHic /* = NULL */;

/******************************************************************
 *		MSVIDEO_GetHicPtr
 *
 *
 */
WINE_HIC*   MSVIDEO_GetHicPtr(HIC hic)
{
    WINE_HIC*   whic;

    for (whic = MSVIDEO_FirstHic; whic && whic->hic != hic; whic = whic->next);
    return whic;
}

/***********************************************************************
 *		VideoForWindowsVersion		[MSVFW32.2]
 *		VideoForWindowsVersion		[MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI VideoForWindowsVersion(void) 
{
    return 0x040003B6; /* 4.950 */
}

/* system.ini: [drivers] */

/***********************************************************************
 *		ICInfo				[MSVFW32.@]
 * Get information about an installable compressor. Return TRUE if there
 * is one.
 */
BOOL VFWAPI ICInfo(
	DWORD fccType,		/* [in] type of compressor ('vidc') */
	DWORD fccHandler,	/* [in] <n>th compressor */
	ICINFO *lpicinfo)	/* [out] information about compressor */
{
    char	buf[2000];

    TRACE("(%.4s,%.4s,%p)\n", (char*)&fccType, (char*)&fccHandler, lpicinfo);

    if (GetPrivateProfileStringA("drivers32", NULL, NULL, buf, sizeof(buf), "system.ini")) 
    {
        char *s = buf;
        while (*s) 
        {
	    /* (WS) I'm commenting out this test because GetPrivateProfileString
	     * will return only a list of keys without their values. I'm curious
	     * to understand how the codecs ever got listed since it seems
	     * obvious they can't be found this way.
	     */
            if (!strncasecmp((char*)&fccType, s, 4) && s[4] == '.' /* && s[9] == '=' */ )
            {
                if (!fccHandler--) 
                {
                    HIC hic;

                    lpicinfo->fccHandler = mmioStringToFOURCCA(s + 5, 0);
                    hic = ICOpen(fccType, lpicinfo->fccHandler, ICMODE_QUERY);
                    if (hic)
                    {
			/* (WS) Some incompatible codecs can make wine crash 
			 * right here. It would be nice if we could protect
			 * wine and simply ignore such codecs.
			 */
                        ICGetInfo(hic, lpicinfo, lpicinfo->dwSize);
                        ICClose(hic);
                        return TRUE;
                    }
		    /* (WS) I'm removing this return because I think it's
		     * better to keep going down the list of codecs rather
		     * than stopping short at the first one that will not
		     * open.
		     
                     * return FALSE; */
		    fccHandler++;
                }
            }
            s += strlen(s) + 1; /* either next char or \0 */
        }
    }
    return FALSE;
}

static DWORD IC_HandleRef = 1;

/***********************************************************************
 *		ICOpen				[MSVFW32.@]
 * Opens an installable compressor. Return special handle.
 */
HIC VFWAPI ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode) 
{
    char		codecname[20];
    ICOPEN		icopen;
    HDRVR		hdrv;
    WINE_HIC*           whic;
    BOOL                bIs16;

    TRACE("(%.4s,%.4s,0x%08lx)\n", (char*)&fccType, (char*)&fccHandler, (DWORD)wMode);

    sprintf(codecname, "%.4s.%.4s", (char*)&fccType, (char*)&fccHandler);

    /* Well, lParam2 is in fact a LPVIDEO_OPEN_PARMS, but it has the
     * same layout as ICOPEN
     */
    icopen.dwSize		= sizeof(ICOPEN);
    icopen.fccType		= fccType;
    icopen.fccHandler	        = fccHandler;
    icopen.dwVersion            = 0x00001000; /* FIXME */
    icopen.dwFlags		= wMode;
    icopen.dwError              = 0;
    icopen.pV1Reserved          = NULL;
    icopen.pV2Reserved          = NULL;
    icopen.dnDevNode            = 0; /* FIXME */
        
    hdrv = OpenDriverA(codecname, "drivers32", (LPARAM)&icopen);
    if (!hdrv) 
    {
        if (fccType == streamtypeVIDEO) 
        {
            sprintf(codecname, "vidc.%.4s", (char*)&fccHandler);
            fccType = ICTYPE_VIDEO;
            hdrv = OpenDriverA(codecname, "drivers32", (LPARAM)&icopen);
        }
        if (!hdrv)
            return 0;
    }
    bIs16 = GetDriverFlags(hdrv) & WINE_GDF_16BIT;

    if (bIs16 && !pFnCallTo16)
    {
        FIXME("Got a 16 bit driver, but no 16 bit support in msvfw\n");
        return 0;
    }
    whic = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_HIC));
    if (!whic)
    {
        CloseDriver(hdrv, 0, 0);
        return FALSE;
    }
    whic->hdrv          = hdrv;
    /* FIXME: is the signature the real one ? */
    whic->driverproc    = bIs16 ? (DRIVERPROC)pFnCallTo16 : NULL;
    whic->driverproc16  = 0;
    whic->type          = fccType;
    whic->handler       = fccHandler;
    while (MSVIDEO_GetHicPtr(HIC_32(IC_HandleRef)) != NULL) IC_HandleRef++;
    whic->hic           = HIC_32(IC_HandleRef++);
    whic->next          = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		MSVIDEO_OpenFunction
 */
HIC MSVIDEO_OpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, 
                         DRIVERPROC lpfnHandler, DWORD lpfnHandler16) 
{
    ICOPEN      icopen;
    WINE_HIC*   whic;

    TRACE("(%.4s,%.4s,%d,%p,%08lx)\n", 
          (char*)&fccType, (char*)&fccHandler, wMode, lpfnHandler, lpfnHandler16);

    icopen.dwSize		= sizeof(ICOPEN);
    icopen.fccType		= fccType;
    icopen.fccHandler	        = fccHandler;
    icopen.dwVersion            = 0x00001000; /* FIXME */
    icopen.dwFlags		= wMode;
    icopen.dwError              = 0;
    icopen.pV1Reserved          = NULL;
    icopen.pV2Reserved          = NULL;
    icopen.dnDevNode            = 0; /* FIXME */

    whic = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_HIC));
    if (!whic) return 0;

    whic->driverproc   = lpfnHandler;
    whic->driverproc16 = lpfnHandler16;
    while (MSVIDEO_GetHicPtr(HIC_32(IC_HandleRef)) != NULL) IC_HandleRef++;
    whic->hic          = HIC_32(IC_HandleRef++);
    whic->next         = MSVIDEO_FirstHic;
    MSVIDEO_FirstHic = whic;

    /* Now try opening/loading the driver. Taken from DRIVER_AddToList */
    /* What if the function is used more than once? */

    if (MSVIDEO_SendMessage(whic->hic, DRV_LOAD, 0L, 0L) != DRV_SUCCESS) 
    {
        WARN("DRV_LOAD failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        HeapFree(GetProcessHeap(), 0, whic);
        return 0;
    }
    /* return value is not checked */
    MSVIDEO_SendMessage(whic->hic, DRV_ENABLE, 0L, 0L);

    whic->hdrv = (HDRVR)MSVIDEO_SendMessage(whic->hic, DRV_OPEN, 0, (DWORD)&icopen);

    if (whic->hdrv == 0) 
    {
        WARN("DRV_OPEN failed for hic %p\n", whic->hic);
        MSVIDEO_FirstHic = whic->next;
        HeapFree(GetProcessHeap(), 0, whic);
        return 0;
    }

    TRACE("=> %p\n", whic->hic);
    return whic->hic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW32.@]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler) 
{
    return MSVIDEO_OpenFunction(fccType, fccHandler, wMode, (DRIVERPROC)lpfnHandler, 0);
}

/***********************************************************************
 *		ICGetInfo			[MSVFW32.@]
 */
LRESULT VFWAPI ICGetInfo(HIC hic,ICINFO *picinfo,DWORD cb) {
	LRESULT		ret;
	char    codecname[10];
	char	szDriver[128];

	TRACE("(%p,%p,%ld)\n",hic,picinfo,cb);

	/* (WS) The field szDriver should be initialized because the driver 
	 * is not obliged and often will not do it. Some applications, like
	 * VirtualDub, rely on this field and will occasionally crash if it
	 * goes unitialized.
	 */
	if (picinfo && cb >= sizeof(ICINFO)) 
	   szDriver[0] = 0; /* At first, set it to an empty string */

	ret = ICSendMessage(hic,ICM_GETINFO,(DWORD)picinfo,cb);

	/* (WS) When szDriver was not supplied by the driver itself, apparently 
	 * Windows will set its value equal to the driver file name. This can
	 * be obtained from the registry as we do here.
	 */
	if (picinfo && cb >= sizeof(ICINFO))
	   if (szDriver[0] == 0) { /* was szDriver not supplied? */
              sprintf(codecname, "vidc.%.4s", (char*)&(picinfo->fccHandler));
              GetPrivateProfileStringA("drivers32", codecname, "", szDriver, sizeof(szDriver), "system.ini");
	      MultiByteToWideChar(CP_ACP, 0, szDriver, -1, picinfo->szDriver, sizeof(picinfo->szDriver)/sizeof(WCHAR));
	   }

	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICLocate			[MSVFW32.@]
 */
HIC VFWAPI ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
                    LPBITMAPINFOHEADER lpbiOut, WORD wMode)
{
    HIC         hic;
    DWORD	querymsg;
    LPSTR       pszBuffer;

    TRACE("(%.4s,%.4s,%p,%p,0x%04x)\n", 
          (char*)&fccType, (char*)&fccHandler, lpbiIn, lpbiOut, wMode);

    switch (wMode) 
    {
    case ICMODE_FASTCOMPRESS:
    case ICMODE_COMPRESS:
        querymsg = ICM_COMPRESS_QUERY;
        break;
    case ICMODE_FASTDECOMPRESS:
    case ICMODE_DECOMPRESS:
        querymsg = ICM_DECOMPRESS_QUERY;
        break;
    case ICMODE_DRAW:
        querymsg = ICM_DRAW_QUERY;
        break;
    default:
        WARN("Unknown mode (%d)\n", wMode);
        return 0;
    }

    /* Easy case: handler/type match, we just fire a query and return */
    hic = ICOpen(fccType, fccHandler, wMode);
    if (hic) 
    {
        if (!ICSendMessage(hic, querymsg, (DWORD)lpbiIn, (DWORD)lpbiOut))
        {
            TRACE("=> %p\n", hic);
            return hic;
        }
        ICClose(hic);
    }

    /* Now try each driver in turn. 32 bit codecs only. */
    /* FIXME: Move this to an init routine? */
    
    pszBuffer = (LPSTR)HeapAlloc(GetProcessHeap(), 0, 1024);
    if (GetPrivateProfileSectionA("drivers32", pszBuffer, 1024, "system.ini")) 
    {
        char* s = pszBuffer;
        while (*s) 
        {
            if (!strncasecmp((char*)&fccType, s, 4) && s[4] == '.' && s[9] == '=')
            {
                char *s2 = s;
                while (*s2 != '\0' && *s2 != '.') s2++;
                if (*s2++) 
                {
                    hic = ICOpen(fccType, *(DWORD*)s2, wMode);
                    if (hic) 
                    {
                        if (!ICSendMessage(hic, querymsg, (DWORD)lpbiIn, (DWORD)lpbiOut))
                        {
                            HeapFree(GetProcessHeap(), 0, pszBuffer);
                            TRACE("=> %p\n", hic);
                            return hic;
                        }
                        ICClose(hic);
                    }
                }
            }
            s += strlen(s) + 1;
        }
    }
    HeapFree(GetProcessHeap(), 0, pszBuffer);

    if (fccType == streamtypeVIDEO) 
        return ICLocate(ICTYPE_VIDEO, fccHandler, lpbiIn, lpbiOut, wMode);
    
    WARN("(%.4s,%.4s,%p,%p,0x%04x) not found!\n",
         (char*)&fccType, (char*)&fccHandler, lpbiIn, lpbiOut, wMode);
    return 0;
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVFW32.@]
 */
HIC VFWAPI ICGetDisplayFormat(
	HIC hic,LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut,
	INT depth,INT dx,INT dy)
{
	HIC	tmphic = hic;

	FIXME("(%p,%p,%p,%d,%d,%d),stub!\n",hic,lpbiIn,lpbiOut,depth,dx,dy);
	if (!tmphic) {
		tmphic=ICLocate(ICTYPE_VIDEO,0,lpbiIn,NULL,ICMODE_DECOMPRESS);
		if (!tmphic)
			return tmphic;
	}
	if ((dy == lpbiIn->biHeight) && (dx == lpbiIn->biWidth))
		dy = dx = 0; /* no resize needed */

	/* Can we decompress it ? */
	if (ICDecompressQuery(tmphic,lpbiIn,NULL) != 0)
		goto errout; /* no, sorry */

	ICDecompressGetFormat(tmphic,lpbiIn,lpbiOut);

	if (lpbiOut->biCompression != 0) {
	   FIXME("Ooch, how come decompressor outputs compressed data (%ld)??\n",
			 lpbiOut->biCompression);
	}
	if (lpbiOut->biSize < sizeof(*lpbiOut)) {
	   FIXME("Ooch, size of output BIH is too small (%ld)\n",
			 lpbiOut->biSize);
	   lpbiOut->biSize = sizeof(*lpbiOut);
	}
	if (!depth) {
		HDC	hdc;

		hdc = GetDC(0);
		depth = GetDeviceCaps(hdc,BITSPIXEL)*GetDeviceCaps(hdc,PLANES);
		ReleaseDC(0,hdc);
		if (depth==15)	depth = 16;
		if (depth<8)	depth =  8;
	}
	if (lpbiIn->biBitCount == 8)
		depth = 8;

	TRACE("=> %p\n", tmphic);
	return tmphic;
errout:
	if (hic!=tmphic)
		ICClose(tmphic);

	TRACE("=> 0\n");
	return 0;
}

/***********************************************************************
 *		ICCompress			[MSVFW32.@]
 */
DWORD VFWAPIV
ICCompress(
	HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev)
{
	ICCOMPRESS	iccmp;

	TRACE("(%p,%ld,%p,%p,%p,%p,...)\n",hic,dwFlags,lpbiOutput,lpData,lpbiInput,lpBits);

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
	return ICSendMessage(hic,ICM_COMPRESS,(DWORD)&iccmp,sizeof(iccmp));
}

/***********************************************************************
 *		ICDecompress			[MSVFW32.@]
 */
DWORD VFWAPIV  ICDecompress(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,
				LPVOID lpData,LPBITMAPINFOHEADER lpbi,LPVOID lpBits)
{
	ICDECOMPRESS	icd;
	DWORD ret;

	TRACE("(%p,%ld,%p,%p,%p,%p)\n",hic,dwFlags,lpbiFormat,lpData,lpbi,lpBits);

	TRACE("lpBits[0] == %ld\n",((LPDWORD)lpBits)[0]);

	icd.dwFlags	= dwFlags;
	icd.lpbiInput	= lpbiFormat;
	icd.lpInput	= lpData;

	icd.lpbiOutput	= lpbi;
	icd.lpOutput	= lpBits;
	icd.ckid	= 0;
	ret = ICSendMessage(hic,ICM_DECOMPRESS,(DWORD)&icd,sizeof(ICDECOMPRESS));

	TRACE("lpBits[0] == %ld\n",((LPDWORD)lpBits)[0]);

	TRACE("-> %ld\n",ret);

	return ret;
}


/***********************************************************************
 *		ICCompressorChoose   [MSVFW32.@]
 */
BOOL VFWAPI ICCompressorChoose(HWND hwnd, UINT uiFlags, LPVOID pvIn, LPVOID lpData,
                               PCOMPVARS pc, LPSTR lpszTitle)
{
    FIXME("stub\n");
    return FALSE;
}


/***********************************************************************
 *		ICCompressorFree   [MSVFW32.@]
 */
void VFWAPI ICCompressorFree(PCOMPVARS pc)
{
    FIXME("stub\n");
}


/******************************************************************
 *		MSVIDEO_SendMessage
 *
 *
 */
LRESULT MSVIDEO_SendMessage(HIC hic,UINT msg,DWORD lParam1,DWORD lParam2)
{
    LRESULT     ret;
    WINE_HIC*   whic = MSVIDEO_GetHicPtr(hic);
    
#define XX(x) case x: TRACE("(%p,"#x",0x%08lx,0x%08lx)\n",hic,lParam1,lParam2);break;
    
    switch (msg) {
        /* DRV_* */
        XX(DRV_LOAD);
        XX(DRV_ENABLE);
        XX(DRV_OPEN);
        XX(DRV_CLOSE);
        XX(DRV_DISABLE);
        XX(DRV_FREE);
        /* ICM_RESERVED+X */
        XX(ICM_ABOUT);
        XX(ICM_CONFIGURE);
        XX(ICM_GET);
        XX(ICM_GETINFO);
        XX(ICM_GETDEFAULTQUALITY);
        XX(ICM_GETQUALITY);
        XX(ICM_GETSTATE);
        XX(ICM_SETQUALITY);
        XX(ICM_SET);
        XX(ICM_SETSTATE);
        /* ICM_USER+X */
        XX(ICM_COMPRESS_FRAMES_INFO);
        XX(ICM_COMPRESS_GET_FORMAT);
        XX(ICM_COMPRESS_GET_SIZE);
        XX(ICM_COMPRESS_QUERY);
        XX(ICM_COMPRESS_BEGIN);
        XX(ICM_COMPRESS);
        XX(ICM_COMPRESS_END);
        XX(ICM_DECOMPRESS_GET_FORMAT);
        XX(ICM_DECOMPRESS_QUERY);
        XX(ICM_DECOMPRESS_BEGIN);
        XX(ICM_DECOMPRESS);
        XX(ICM_DECOMPRESS_END);
        XX(ICM_DECOMPRESS_SET_PALETTE);
        XX(ICM_DECOMPRESS_GET_PALETTE);
        XX(ICM_DRAW_QUERY);
        XX(ICM_DRAW_BEGIN);
        XX(ICM_DRAW_GET_PALETTE);
        XX(ICM_DRAW_START);
        XX(ICM_DRAW_STOP);
        XX(ICM_DRAW_END);
        XX(ICM_DRAW_GETTIME);
        XX(ICM_DRAW);
        XX(ICM_DRAW_WINDOW);
        XX(ICM_DRAW_SETTIME);
        XX(ICM_DRAW_REALIZE);
        XX(ICM_DRAW_FLUSH);
        XX(ICM_DRAW_RENDERBUFFER);
        XX(ICM_DRAW_START_PLAY);
        XX(ICM_DRAW_STOP_PLAY);
        XX(ICM_DRAW_SUGGESTFORMAT);
        XX(ICM_DRAW_CHANGEPALETTE);
        XX(ICM_GETBUFFERSWANTED);
        XX(ICM_GETDEFAULTKEYFRAMERATE);
        XX(ICM_DECOMPRESSEX_BEGIN);
        XX(ICM_DECOMPRESSEX_QUERY);
        XX(ICM_DECOMPRESSEX);
        XX(ICM_DECOMPRESSEX_END);
        XX(ICM_SET_STATUS_PROC);
    default:
        FIXME("(%p,0x%08lx,0x%08lx,0x%08lx) unknown message\n",hic,(DWORD)msg,lParam1,lParam2);
    }
    
#undef XX
    
    if (!whic) return ICERR_BADHANDLE;
    
    if (whic->driverproc) {
        ret = whic->driverproc((DWORD)hic, whic->hdrv, msg, lParam1, lParam2);
    } else {
        ret = SendDriverMessage(whic->hdrv, msg, lParam1, lParam2);
    }

    TRACE("	-> 0x%08lx\n", ret);
    return ret;
}

/***********************************************************************
 *		ICSendMessage			[MSVFW32.@]
 */
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2) {
	return MSVIDEO_SendMessage(hic,msg,lParam1,lParam2);
}

/***********************************************************************
 *		ICDrawBegin		[MSVFW32.@]
 */
DWORD VFWAPIV ICDrawBegin(
	HIC                hic,     /* [in] */
	DWORD              dwFlags, /* [in] flags */
	HPALETTE           hpal,    /* [in] palette to draw with */
	HWND               hwnd,    /* [in] window to draw to */
	HDC                hdc,     /* [in] HDC to draw to */
	INT                xDst,    /* [in] destination rectangle */
	INT                yDst,    /* [in] */
	INT                dxDst,   /* [in] */
	INT                dyDst,   /* [in] */
	LPBITMAPINFOHEADER lpbi,    /* [in] format of frame to draw */
	INT                xSrc,    /* [in] source rectangle */
	INT                ySrc,    /* [in] */
	INT                dxSrc,   /* [in] */
	INT                dySrc,   /* [in] */
	DWORD              dwRate,  /* [in] frames/second = (dwRate/dwScale) */
	DWORD              dwScale) /* [in] */
{

	ICDRAWBEGIN	icdb;

	TRACE("(%p,%ld,%p,%p,%p,%u,%u,%u,%u,%p,%u,%u,%u,%u,%ld,%ld)\n",
		  hic, dwFlags, hpal, hwnd, hdc, xDst, yDst, dxDst, dyDst,
		  lpbi, xSrc, ySrc, dxSrc, dySrc, dwRate, dwScale);

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
	return ICSendMessage(hic,ICM_DRAW_BEGIN,(DWORD)&icdb,sizeof(icdb));
}

/***********************************************************************
 *		ICDraw			[MSVFW32.@]
 */
DWORD VFWAPIV ICDraw(HIC hic, DWORD dwFlags, LPVOID lpFormat, LPVOID lpData, DWORD cbData, LONG lTime) {
	ICDRAW	icd;

	TRACE("(%p,%ld,%p,%p,%ld,%ld)\n",hic,dwFlags,lpFormat,lpData,cbData,lTime);

	icd.dwFlags = dwFlags;
	icd.lpFormat = lpFormat;
	icd.lpData = lpData;
	icd.cbData = cbData;
	icd.lTime = lTime;

	return ICSendMessage(hic,ICM_DRAW,(DWORD)&icd,sizeof(icd));
}

/***********************************************************************
 *		ICClose			[MSVFW32.@]
 */
LRESULT WINAPI ICClose(HIC hic)
{
    WINE_HIC* whic = MSVIDEO_GetHicPtr(hic);
    WINE_HIC** p;

    TRACE("(%p)\n",hic);

    if (!whic) return ICERR_BADHANDLE;

    if (whic->driverproc) 
    {
        MSVIDEO_SendMessage(hic, DRV_CLOSE, 0, 0);
        MSVIDEO_SendMessage(hic, DRV_DISABLE, 0, 0);
        MSVIDEO_SendMessage(hic, DRV_FREE, 0, 0);
    }
    else
    {
        CloseDriver(whic->hdrv, 0, 0);
    }

    /* remove whic from list */
    for (p = &MSVIDEO_FirstHic; *p != NULL; p = &((*p)->next))
    {
        if ((*p) == whic)
        {
            *p = whic->next;
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, whic);
    return 0;
}



/***********************************************************************
 *		ICImageCompress	[MSVFW32.@]
 */
HANDLE VFWAPI ICImageCompress(
	HIC hic, UINT uiFlags,
	LPBITMAPINFO lpbiIn, LPVOID lpBits,
	LPBITMAPINFO lpbiOut, LONG lQuality,
	LONG* plSize)
{
	FIXME("(%p,%08x,%p,%p,%p,%ld,%p)\n",
		hic, uiFlags, lpbiIn, lpBits, lpbiOut, lQuality, plSize);

	return NULL;
}

/***********************************************************************
 *		ICImageDecompress	[MSVFW32.@]
 */

HANDLE VFWAPI ICImageDecompress(
	HIC hic, UINT uiFlags, LPBITMAPINFO lpbiIn,
	LPVOID lpBits, LPBITMAPINFO lpbiOut)
{
	HGLOBAL	hMem = NULL;
	BYTE*	pMem = NULL;
	BOOL	bReleaseIC = FALSE;
	BYTE*	pHdr = NULL;
	LONG	cbHdr = 0;
	BOOL	bSucceeded = FALSE;
	BOOL	bInDecompress = FALSE;
	DWORD	biSizeImage;

	TRACE("(%p,%08x,%p,%p,%p)\n",
		hic, uiFlags, lpbiIn, lpBits, lpbiOut);

	if ( hic == NULL )
	{
		hic = ICDecompressOpen( mmioFOURCC('V','I','D','C'), 0, &lpbiIn->bmiHeader, (lpbiOut != NULL) ? &lpbiOut->bmiHeader : NULL );
		if ( hic == NULL )
		{
			WARN("no handler\n" );
			goto err;
		}
		bReleaseIC = TRUE;
	}
	if ( uiFlags != 0 )
	{
		FIXME( "unknown flag %08x\n", uiFlags );
		goto err;
	}
	if ( lpbiIn == NULL || lpBits == NULL )
	{
		WARN("invalid argument\n");
		goto err;
	}

	if ( lpbiOut != NULL )
	{
		if ( lpbiOut->bmiHeader.biSize != sizeof(BITMAPINFOHEADER) )
			goto err;
		cbHdr = sizeof(BITMAPINFOHEADER);
		if ( lpbiOut->bmiHeader.biCompression == 3 )
			cbHdr += sizeof(DWORD)*3;
		else
		if ( lpbiOut->bmiHeader.biBitCount <= 8 )
		{
			if ( lpbiOut->bmiHeader.biClrUsed == 0 )
				cbHdr += sizeof(RGBQUAD) * (1<<lpbiOut->bmiHeader.biBitCount);
			else
				cbHdr += sizeof(RGBQUAD) * lpbiOut->bmiHeader.biClrUsed;
		}
	}
	else
	{
		TRACE( "get format\n" );

		cbHdr = ICDecompressGetFormatSize(hic,lpbiIn);
		if ( cbHdr < sizeof(BITMAPINFOHEADER) )
			goto err;
		pHdr = (BYTE*)HeapAlloc(GetProcessHeap(),0,cbHdr+sizeof(RGBQUAD)*256);
		if ( pHdr == NULL )
			goto err;
		ZeroMemory( pHdr, cbHdr+sizeof(RGBQUAD)*256 );
		if ( ICDecompressGetFormat( hic, lpbiIn, (BITMAPINFO*)pHdr ) != ICERR_OK )
			goto err;
		lpbiOut = (BITMAPINFO*)pHdr;
		if ( lpbiOut->bmiHeader.biBitCount <= 8 &&
			 ICDecompressGetPalette( hic, lpbiIn, lpbiOut ) != ICERR_OK &&
			 lpbiIn->bmiHeader.biBitCount == lpbiOut->bmiHeader.biBitCount )
		{
			if ( lpbiIn->bmiHeader.biClrUsed == 0 )
				memcpy( lpbiOut->bmiColors, lpbiIn->bmiColors, sizeof(RGBQUAD)*(1<<lpbiOut->bmiHeader.biBitCount) );
			else
				memcpy( lpbiOut->bmiColors, lpbiIn->bmiColors, sizeof(RGBQUAD)*lpbiIn->bmiHeader.biClrUsed );
		}
		if ( lpbiOut->bmiHeader.biBitCount <= 8 &&
			 lpbiOut->bmiHeader.biClrUsed == 0 )
			lpbiOut->bmiHeader.biClrUsed = 1<<lpbiOut->bmiHeader.biBitCount;

		lpbiOut->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		cbHdr = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)*lpbiOut->bmiHeader.biClrUsed;
	}

	biSizeImage = lpbiOut->bmiHeader.biSizeImage;
	if ( biSizeImage == 0 )
		biSizeImage = ((((lpbiOut->bmiHeader.biWidth * lpbiOut->bmiHeader.biBitCount + 7) >> 3) + 3) & (~3)) * abs(lpbiOut->bmiHeader.biHeight);

	TRACE( "call ICDecompressBegin\n" );

	if ( ICDecompressBegin( hic, lpbiIn, lpbiOut ) != ICERR_OK )
		goto err;
	bInDecompress = TRUE;

	TRACE( "cbHdr %ld, biSizeImage %ld\n", cbHdr, biSizeImage );

	hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_ZEROINIT, cbHdr + biSizeImage );
	if ( hMem == NULL )
	{
		WARN( "out of memory\n" );
		goto err;
	}
	pMem = (BYTE*)GlobalLock( hMem );
	if ( pMem == NULL )
		goto err;
	memcpy( pMem, lpbiOut, cbHdr );

	TRACE( "call ICDecompress\n" );
	if ( ICDecompress( hic, 0, &lpbiIn->bmiHeader, lpBits, &lpbiOut->bmiHeader, pMem+cbHdr ) != ICERR_OK )
		goto err;

	bSucceeded = TRUE;
err:
	if ( bInDecompress )
		ICDecompressEnd( hic );
	if ( bReleaseIC )
		ICClose(hic);
	if ( pHdr != NULL )
		HeapFree(GetProcessHeap(),0,pHdr);
	if ( pMem != NULL )
		GlobalUnlock( hMem );
	if ( !bSucceeded && hMem != NULL )
	{
		GlobalFree(hMem); hMem = NULL;
	}

	return (HANDLE)hMem;
}
