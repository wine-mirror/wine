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
 */

#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winver.h"
#include "vfw.h"
#include "vfw16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "stackframe.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvideo);

/* ### start build ### */
extern LONG CALLBACK MSVIDEO_CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
/* ### stop build ### */

LPVOID MSVIDEO_MapMsg16To32(UINT msg, LPDWORD lParam1, LPDWORD lParam2);
void MSVIDEO_UnmapMsg16To32(UINT msg, LPVOID lpv, LPDWORD lParam1, LPDWORD lParam2);
LRESULT MSVIDEO_SendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2, BOOL bFrom32);

#define HDRVR_16(h32)		(LOWORD(h32))


/***********************************************************************
 *		VideoForWindowsVersion		[MSVFW32.2]
 *		VideoForWindowsVersion		[MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI VideoForWindowsVersion(void) {
	return 0x040003B6; /* 4.950 */
}

/***********************************************************************
 *		VideoCapDriverDescAndVer	[MSVIDEO.22]
 */
DWORD WINAPI VideoCapDriverDescAndVer(
	WORD nr,LPSTR buf1,WORD buf1len,LPSTR buf2,WORD buf2len
) {
	DWORD	verhandle;
	WORD	xnr = nr;
	DWORD	infosize;
	UINT	subblocklen;
	char	*s,buf[2000],fn[260];
	LPBYTE	infobuf;
	LPVOID	subblock;

	TRACE("(%d,%p,%d,%p,%d)\n",nr,buf1,buf1len,buf2,buf2len);
	if (GetPrivateProfileStringA("drivers32",NULL,NULL,buf,sizeof(buf),"system.ini")) {
		s = buf;
		while (*s) {
		        if (!strncasecmp(s,"vid",3)) {
			    if (!xnr)
				break;
			    xnr--;
			}
			s=s+strlen(s)+1; /* either next char or \0 */
		}
	} else
	    return 20; /* hmm, out of entries even if we don't have any */
	if (xnr) {
		FIXME("No more VID* entries found\n");
		return 20;
	}
	GetPrivateProfileStringA("drivers32",s,NULL,fn,sizeof(fn),"system.ini");
	infosize = GetFileVersionInfoSizeA(fn,&verhandle);
	if (!infosize) {
	    TRACE("%s has no fileversioninfo.\n",fn);
	    return 18;
	}
	infobuf = HeapAlloc(GetProcessHeap(),0,infosize);
	if (GetFileVersionInfoA(fn,verhandle,infosize,infobuf)) {
	    char	vbuf[200];
	    /* Yes, two space behind : */
	    /* FIXME: test for buflen */
	    sprintf(vbuf,"Version:  %d.%d.%d.%d\n",
		    ((WORD*)infobuf)[0x0f],
		    ((WORD*)infobuf)[0x0e],
		    ((WORD*)infobuf)[0x11],
		    ((WORD*)infobuf)[0x10]
	    );
	    TRACE("version of %s is %s\n",fn,vbuf);
	    strncpy(buf2,vbuf,buf2len);
	} else {
	    TRACE("GetFileVersionInfoA failed for %s.\n",fn);
	    strncpy(buf2,fn,buf2len); /* msvideo.dll appears to copy fn*/
	}
	/* FIXME: language problem? */
	if (VerQueryValueA(	infobuf,
				"\\StringFileInfo\\040904E4\\FileDescription",
				&subblock,
				&subblocklen
	)) {
	    TRACE("VQA returned %s\n",(LPCSTR)subblock);
	    strncpy(buf1,subblock,buf1len);
	} else {
	    TRACE("VQA did not return on query \\StringFileInfo\\040904E4\\FileDescription?\n");
	    strncpy(buf1,fn,buf1len); /* msvideo.dll appears to copy fn*/
	}
	HeapFree(GetProcessHeap(),0,infobuf);
	return 0;
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
			if (!strncasecmp(type,s,4)) {
				if(!fccHandler--) {
					lpicinfo->fccHandler = mmioStringToFOURCCA(s+5,0);
					return TRUE;
				}
			}
			s=s+strlen(s)+1; /* either next char or \0 */
		}
	}
	return FALSE;
}

/***********************************************************************
 *		ICInfo				[MSVIDEO.200]
 */
BOOL16 VFWAPI ICInfo16(
	DWORD     fccType,    /* [in] */
	DWORD     fccHandler, /* [in] */
	ICINFO16 *lpicinfo)   /* [in/out] NOTE: SEGPTR */
{
	BOOL16 ret;
	LPVOID lpv;
	DWORD lParam = (DWORD)lpicinfo;
	DWORD size = ((ICINFO*)(MapSL((SEGPTR)lpicinfo)))->dwSize;

	/* Use the mapping functions to map the ICINFO structure */
	lpv = MSVIDEO_MapMsg16To32(ICM_GETINFO,&lParam,&size);

	ret = ICInfo(fccType,fccHandler,(ICINFO*)lParam);

	MSVIDEO_UnmapMsg16To32(ICM_GETINFO,lpv,&lParam,&size);

	return ret;
}

/***********************************************************************
 *		ICOpen				[MSVFW32.@]
 * Opens an installable compressor. Return special handle.
 */
HIC VFWAPI ICOpen(DWORD fccType,DWORD fccHandler,UINT wMode) {
	char		type[5],handler[5],codecname[20];
	ICOPEN		icopen;
	HDRVR		hdrv;
	HIC		hic;
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
	/* The handle should be a valid 16-bit handle as well */
	hic = HIC_32(GlobalAlloc16(GHND,sizeof(WINE_HIC)));
	whic = (WINE_HIC*)GlobalLock16(HIC_16(hic));
	whic->hdrv	= hdrv;
	whic->driverproc= NULL;
	whic->private	= 0;
	GlobalUnlock16(HIC_16(hic));
	TRACE("=> %p\n",hic);
	return HIC_32(hic);
}

HIC MSVIDEO_OpenFunc(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler, BOOL bFrom32) {
	char	type[5],handler[5],codecname[20];
	HIC	hic;
        ICOPEN icopen;
        SEGPTR seg_icopen;
	WINE_HIC	*whic;

	memcpy(type,&fccType,4);type[4]=0;
	memcpy(handler,&fccHandler,4);handler[4]=0;
	TRACE("(%s,%s,%d,%p,%d)\n",type,handler,wMode,lpfnHandler,bFrom32?32:16);

        icopen.fccType    = fccType;
        icopen.fccHandler = fccHandler;
        icopen.dwSize     = sizeof(ICOPEN);
        icopen.dwFlags    = wMode;

        sprintf(codecname,"%s.%s",type,handler);

	hic = HIC_32(GlobalAlloc16(GHND,sizeof(WINE_HIC)));
	if (!hic)
		return 0;
	whic = GlobalLock16(HIC_16(hic));
	whic->driverproc = lpfnHandler;

	whic->private = bFrom32;

	/* Now try opening/loading the driver. Taken from DRIVER_AddToList */
	/* What if the function is used more than once? */

	if (MSVIDEO_SendMessage(hic,DRV_LOAD,0L,0L,bFrom32) != DRV_SUCCESS) {
		WARN("DRV_LOAD failed for hic %p\n", hic);
		GlobalFree16(HIC_16(hic));
		return 0;
	}
	/* return value is not checked */
	MSVIDEO_SendMessage(hic,DRV_ENABLE,0L,0L,bFrom32);

        seg_icopen = MapLS( &icopen );
        whic->hdrv = (HDRVR)MSVIDEO_SendMessage(hic,DRV_OPEN,0,seg_icopen,FALSE);
        UnMapLS( seg_icopen );
	if (whic->hdrv == 0) {
		WARN("DRV_OPEN failed for hic %p\n",hic);
		GlobalFree16(HIC_16(hic));
		return 0;
	}

	GlobalUnlock16(HIC_16(hic));
	TRACE("=> %p\n",hic);
	return hic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW32.@]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler) {
	return MSVIDEO_OpenFunc(fccType,fccHandler,wMode,lpfnHandler,TRUE);
}

/***********************************************************************
 *		ICOpenFunction			[MSVIDEO.206]
 */
HIC16 VFWAPI ICOpenFunction16(DWORD fccType, DWORD fccHandler, UINT16 wMode, FARPROC16 lpfnHandler)
{
	return HIC_16(MSVIDEO_OpenFunc(fccType, fccHandler, wMode, (FARPROC)lpfnHandler,FALSE));
}

/***********************************************************************
 *		ICGetInfo			[MSVFW32.@]
 */
LRESULT VFWAPI ICGetInfo(HIC hic,ICINFO *picinfo,DWORD cb) {
	LRESULT		ret;

	TRACE("(%p,%p,%ld)\n",hic,picinfo,cb);
	ret = ICSendMessage(hic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICLocate			[MSVFW32.@]
 */
HIC VFWAPI ICLocate(
	DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
	LPBITMAPINFOHEADER lpbiOut, WORD wMode)
{
	char	type[5],handler[5];
	HIC	hic;
	DWORD	querymsg;
	LPSTR pszBuffer;

	type[4]=0;memcpy(type,&fccType,4);
	handler[4]=0;memcpy(handler,&fccHandler,4);

	TRACE("(%s,%s,%p,%p,0x%04x)\n", type, handler, lpbiIn, lpbiOut, wMode);

	switch (wMode) {
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
		WARN("Unknown mode (%d)\n",wMode);
		return 0;
	}

	/* Easy case: handler/type match, we just fire a query and return */
	hic = ICOpen(fccType,fccHandler,wMode);
	if (hic) {
		if (!ICSendMessage(hic,querymsg,(DWORD)lpbiIn,(DWORD)lpbiOut))
			return hic;
		ICClose(hic);
	}

	type[4]='.';memcpy(type,&fccType,4);
	handler[4]='.';memcpy(handler,&fccHandler,4);

	/* Now try each driver in turn. 32 bit codecs only. */
	/* FIXME: Move this to an init routine? */

	pszBuffer = (LPSTR)HeapAlloc(GetProcessHeap(),0,1024);
	if (GetPrivateProfileSectionA("drivers32",pszBuffer,1024,"system.ini")) {
		char* s = pszBuffer;
		while (*s) {
			if (!strncasecmp(type,s,5)) {
				char *s2 = s;
				while (*s2 != '\0' && *s2 != '.') s2++;
				if (*s2++) {
					HIC h;

					h = ICOpen(fccType,*(DWORD*)s2,wMode);
					if (h) {
						if (!ICSendMessage(h,querymsg,(DWORD)lpbiIn,(DWORD)lpbiOut))
							return h;
						ICClose(h);
					}
				}
			}
			s += strlen(s) + 1;
		}
	}
	HeapFree(GetProcessHeap(),0,pszBuffer);

	if (fccType==streamtypeVIDEO) {
		hic = ICLocate(ICTYPE_VIDEO,fccHandler,lpbiIn,lpbiOut,wMode);
		if (hic)
			return hic;
	}

	type[4] = handler[4] = '\0';
	WARN("(%.4s,%.4s,%p,%p,0x%04x) not found!\n",type,handler,lpbiIn,lpbiOut,wMode);
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

#define COPY(x,y) (x->y = x##16->y);
#define COPYPTR(x,y) (x->y = MapSL((SEGPTR)x##16->y));

LPVOID MSVIDEO_MapICDEX16To32(LPDWORD lParam) {
	LPVOID ret;

	ICDECOMPRESSEX *icdx = HeapAlloc(GetProcessHeap(),0,sizeof(ICDECOMPRESSEX));
	ICDECOMPRESSEX16 *icdx16 = MapSL(*lParam);
	ret = icdx16;

	COPY(icdx,dwFlags);
	COPYPTR(icdx,lpbiSrc);
	COPYPTR(icdx,lpSrc);
	COPYPTR(icdx,lpbiDst);
	COPYPTR(icdx,lpDst);
	COPY(icdx,xDst);
	COPY(icdx,yDst);
	COPY(icdx,dxDst);
	COPY(icdx,dyDst);
	COPY(icdx,xSrc);
	COPY(icdx,ySrc);
	COPY(icdx,dxSrc);
	COPY(icdx,dySrc);

	*lParam = (DWORD)(icdx);
	return ret;
}

LPVOID MSVIDEO_MapMsg16To32(UINT msg, LPDWORD lParam1, LPDWORD lParam2) {
	LPVOID ret = 0;

	TRACE("Mapping %d\n",msg);

	switch (msg) {
	case DRV_LOAD:
	case DRV_ENABLE:
	case DRV_CLOSE:
	case DRV_DISABLE:
	case DRV_FREE:
	case ICM_ABOUT:
	case ICM_CONFIGURE:
	case ICM_COMPRESS_END:
	case ICM_DECOMPRESS_END:
	case ICM_DECOMPRESSEX_END:
	case ICM_SETQUALITY:
	case ICM_DRAW_START_PLAY:
	case ICM_DRAW_STOP_PLAY:
	case ICM_DRAW_REALIZE:
	case ICM_DRAW_RENDERBUFFER:
	case ICM_DRAW_END:
		break;
	case DRV_OPEN:
	case ICM_GETDEFAULTQUALITY:
	case ICM_GETQUALITY:
	case ICM_SETSTATE:
	case ICM_DRAW_WINDOW:
	case ICM_GETBUFFERSWANTED:
		*lParam1 = (DWORD)MapSL(*lParam1);
		break;
	case ICM_GETINFO:
		{
			ICINFO *ici = HeapAlloc(GetProcessHeap(),0,sizeof(ICINFO));
			ICINFO16 *ici16;

			ici16 = MapSL(*lParam1);
			ret = ici16;

			ici->dwSize = sizeof(ICINFO);
			COPY(ici,fccType);
			COPY(ici,fccHandler);
			COPY(ici,dwFlags);
			COPY(ici,dwVersion);
			COPY(ici,dwVersionICM);
                        MultiByteToWideChar( CP_ACP, 0, ici16->szName, -1, ici->szName, 16 );
                        MultiByteToWideChar( CP_ACP, 0, ici16->szDescription, -1, ici->szDescription, 128 );
                        MultiByteToWideChar( CP_ACP, 0, ici16->szDriver, -1, ici->szDriver, 128 );
			*lParam1 = (DWORD)(ici);
			*lParam2 = sizeof(ICINFO);
		}
		break;
	case ICM_COMPRESS:
		{
			ICCOMPRESS *icc = HeapAlloc(GetProcessHeap(),0,sizeof(ICCOMPRESS));
			ICCOMPRESS *icc16;

			icc16 = MapSL(*lParam1);
			ret = icc16;

			COPY(icc,dwFlags);
			COPYPTR(icc,lpbiOutput);
			COPYPTR(icc,lpOutput);
			COPYPTR(icc,lpbiInput);
			COPYPTR(icc,lpInput);
			COPYPTR(icc,lpckid);
			COPYPTR(icc,lpdwFlags);
			COPY(icc,lFrameNum);
			COPY(icc,dwFrameSize);
			COPY(icc,dwQuality);
			COPYPTR(icc,lpbiPrev);
			COPYPTR(icc,lpPrev);

			*lParam1 = (DWORD)(icc);
			*lParam2 = sizeof(ICCOMPRESS);
		}
		break;
	case ICM_DECOMPRESS:
		{
			ICDECOMPRESS *icd = HeapAlloc(GetProcessHeap(),0,sizeof(ICDECOMPRESS));
			ICDECOMPRESS *icd16; /* Same structure except for the pointers */

			icd16 = MapSL(*lParam1);
			ret = icd16;

			COPY(icd,dwFlags);
			COPYPTR(icd,lpbiInput);
			COPYPTR(icd,lpInput);
			COPYPTR(icd,lpbiOutput);
			COPYPTR(icd,lpOutput);
			COPY(icd,ckid);

			*lParam1 = (DWORD)(icd);
			*lParam2 = sizeof(ICDECOMPRESS);
		}
		break;
	case ICM_COMPRESS_BEGIN:
	case ICM_COMPRESS_GET_FORMAT:
	case ICM_COMPRESS_GET_SIZE:
	case ICM_COMPRESS_QUERY:
	case ICM_DECOMPRESS_GET_FORMAT:
	case ICM_DECOMPRESS_QUERY:
	case ICM_DECOMPRESS_BEGIN:
	case ICM_DECOMPRESS_SET_PALETTE:
	case ICM_DECOMPRESS_GET_PALETTE:
		*lParam1 = (DWORD)MapSL(*lParam1);
		*lParam2 = (DWORD)MapSL(*lParam2);
		break;
	case ICM_DECOMPRESSEX_QUERY:
		if ((*lParam2 != sizeof(ICDECOMPRESSEX16)) && (*lParam2 != 0))
			WARN("*lParam2 has unknown value %p\n",(ICDECOMPRESSEX16*)*lParam2);
		/* FIXME: *lParm2 is meant to be 0 or an ICDECOMPRESSEX16*, but is sizeof(ICDECOMRPESSEX16)
		 * This is because of ICMessage(). Special case it?
		 {
		 LPVOID* addr = HeapAlloc(GetProcessHeap(),0,2*sizeof(LPVOID));
		 addr[0] = MSVIDEO_MapICDEX16To32(lParam1);
		 if (*lParam2)
		 addr[1] = MSVIDEO_MapICDEX16To32(lParam2);
		 else
		 addr[1] = 0;

		 ret = addr;
		 }
		 break;*/
	case ICM_DECOMPRESSEX_BEGIN:
	case ICM_DECOMPRESSEX:
		ret = MSVIDEO_MapICDEX16To32(lParam1);
		*lParam2 = sizeof(ICDECOMPRESSEX);
		break;
	case ICM_DRAW_BEGIN:
		{
			ICDRAWBEGIN *icdb = HeapAlloc(GetProcessHeap(),0,sizeof(ICDRAWBEGIN));
			ICDRAWBEGIN16 *icdb16 = MapSL(*lParam1);
			ret = icdb16;

			COPY(icdb,dwFlags);
			icdb->hpal = HPALETTE_32(icdb16->hpal);
			icdb->hwnd = HWND_32(icdb16->hwnd);
			icdb->hdc = HDC_32(icdb16->hdc);
			COPY(icdb,xDst);
			COPY(icdb,yDst);
			COPY(icdb,dxDst);
			COPY(icdb,dyDst);
			COPYPTR(icdb,lpbi);
			COPY(icdb,xSrc);
			COPY(icdb,ySrc);
			COPY(icdb,dxSrc);
			COPY(icdb,dySrc);
			COPY(icdb,dwRate);
			COPY(icdb,dwScale);

			*lParam1 = (DWORD)(icdb);
			*lParam2 = sizeof(ICDRAWBEGIN);
		}
		break;
	case ICM_DRAW_SUGGESTFORMAT:
		{
			ICDRAWSUGGEST *icds = HeapAlloc(GetProcessHeap(),0,sizeof(ICDRAWSUGGEST));
			ICDRAWSUGGEST16 *icds16 = MapSL(*lParam1);

			ret = icds16;

			COPY(icds,dwFlags);
			COPYPTR(icds,lpbiIn);
			COPYPTR(icds,lpbiSuggest);
			COPY(icds,dxSrc);
			COPY(icds,dySrc);
			COPY(icds,dxDst);
			COPY(icds,dyDst);
			icds->hicDecompressor = HIC_32(icds16->hicDecompressor);

			*lParam1 = (DWORD)(icds);
			*lParam2 = sizeof(ICDRAWSUGGEST);
		}
		break;
	case ICM_DRAW:
		{
			ICDRAW *icd = HeapAlloc(GetProcessHeap(),0,sizeof(ICDRAW));
			ICDRAW *icd16 = MapSL(*lParam1);
			ret = icd16;

			COPY(icd,dwFlags);
			COPYPTR(icd,lpFormat);
			COPYPTR(icd,lpData);
			COPY(icd,cbData);
			COPY(icd,lTime);

			*lParam1 = (DWORD)(icd);
			*lParam2 = sizeof(ICDRAW);
		}
		break;
	case ICM_DRAW_START:
	case ICM_DRAW_STOP:
		break;
	default:
		FIXME("%d is not yet handled. Expect a crash.\n",msg);
	}
	return ret;
}

#undef COPY
#undef COPYPTR

void MSVIDEO_UnmapMsg16To32(UINT msg, LPVOID data16, LPDWORD lParam1, LPDWORD lParam2) {
	TRACE("Unmapping %d\n",msg);

#define UNCOPY(x,y) (x##16->y = x->y);

	switch (msg) {
	case ICM_GETINFO:
		{
			ICINFO *ici = (ICINFO*)(*lParam1);
			ICINFO16 *ici16 = (ICINFO16*)data16;

			UNCOPY(ici,fccType);
			UNCOPY(ici,fccHandler);
			UNCOPY(ici,dwFlags);
			UNCOPY(ici,dwVersion);
			UNCOPY(ici,dwVersionICM);
                        WideCharToMultiByte( CP_ACP, 0, ici->szName, -1, ici16->szName,
                                             sizeof(ici16->szName), NULL, NULL );
                        ici16->szName[sizeof(ici16->szName)-1] = 0;
                        WideCharToMultiByte( CP_ACP, 0, ici->szDescription, -1, ici16->szDescription,
                                             sizeof(ici16->szDescription), NULL, NULL );
                        ici16->szDescription[sizeof(ici16->szDescription)-1] = 0;
			/* This just gives garbage for some reason - BB
			   lstrcpynWtoA(ici16->szDriver,ici->szDriver,128);*/

			HeapFree(GetProcessHeap(),0,ici);
		}
		break;
	case ICM_DECOMPRESS_QUERY:
		/*{
		  LPVOID* x = data16;
		  HeapFree(GetProcessHeap(),0,x[0]);
		  if (x[1])
		  HeapFree(GetProcessHeap(),0,x[1]);
		  }
		  break;*/
	case ICM_COMPRESS:
	case ICM_DECOMPRESS:
	case ICM_DECOMPRESSEX_QUERY:
	case ICM_DECOMPRESSEX_BEGIN:
	case ICM_DECOMPRESSEX:
	case ICM_DRAW_BEGIN:
	case ICM_DRAW_SUGGESTFORMAT:
	case ICM_DRAW:
		HeapFree(GetProcessHeap(),0,data16);
		break;
	default:
		ERR("Unmapping unmapped msg %d\n",msg);
	}
#undef UNCOPY
}

LRESULT MSVIDEO_SendMessage(HIC hic,UINT msg,DWORD lParam1,DWORD lParam2, BOOL bFrom32) {
	LRESULT		ret;
	WINE_HIC	*whic = GlobalLock16(HIC_16(hic));
	LPVOID data16 = 0;
	BOOL bDrv32;

#define XX(x) case x: TRACE("(%p,"#x",0x%08lx,0x%08lx,%d)\n",hic,lParam1,lParam2,bFrom32?32:16);break;

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
		FIXME("(%p,0x%08lx,0x%08lx,0x%08lx,%i) unknown message\n",hic,(DWORD)msg,lParam1,lParam2,bFrom32?32:16);
	}

#undef XX

	if (!whic) return ICERR_BADHANDLE;

	if (whic->driverproc) { /* IC is a function */
		bDrv32 = whic->private;
	} else {
		bDrv32 = ((GetDriverFlags(whic->hdrv) & (WINE_GDF_EXIST|WINE_GDF_16BIT)) == WINE_GDF_EXIST);
	}

	if (!bFrom32) {
		if (bDrv32)
			data16 = MSVIDEO_MapMsg16To32(msg,&lParam1,&lParam2);
	} else {
		if (!bDrv32) {
			ERR("Can't do 32->16 mappings\n");
			ret = -1;
			goto out;
		}
	}

	if (whic->driverproc) {
		if (bDrv32) {
			ret = whic->driverproc(whic->hdrv,hic,msg,lParam1,lParam2);
		} else {
			ret = MSVIDEO_CallTo16_long_lwwll((FARPROC16)whic->driverproc,(LONG)whic->hdrv,HIC_16(hic),msg,lParam1,lParam2);
		}
	} else {
		ret = SendDriverMessage(whic->hdrv,msg,lParam1,lParam2);
	}

	if (data16)
		MSVIDEO_UnmapMsg16To32(msg,data16,&lParam1,&lParam2);

 out:
	GlobalUnlock16(HIC_16(hic));

	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICSendMessage			[MSVFW32.@]
 */
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2) {
	return MSVIDEO_SendMessage(hic,msg,lParam1,lParam2,TRUE);
}

/***********************************************************************
 *		ICSendMessage			[MSVIDEO.205]
 */
LRESULT VFWAPI ICSendMessage16(HIC16 hic, UINT16 msg, DWORD lParam1, DWORD lParam2) {
	return MSVIDEO_SendMessage(HIC_32(hic),msg,lParam1,lParam2,FALSE);
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
LRESULT WINAPI ICClose(HIC hic) {
	WINE_HIC *whic = GlobalLock16(HIC_16(hic));
	TRACE("(%p)\n",hic);
	if (whic->driverproc) {
		ICSendMessage(hic,DRV_CLOSE,0,0);
		ICSendMessage(hic,DRV_DISABLE,0,0);
		ICSendMessage(hic,DRV_FREE,0,0);
	} else {
		CloseDriver(whic->hdrv,0,0);
	}

	GlobalUnlock16(HIC_16(hic));
	GlobalFree16(HIC_16(hic));
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

	return (HANDLE)NULL;
}

/***********************************************************************
 *		ICImageDecompress	[MSVFW32.@]
 */

HANDLE VFWAPI ICImageDecompress(
	HIC hic, UINT uiFlags, LPBITMAPINFO lpbiIn,
	LPVOID lpBits, LPBITMAPINFO lpbiOut)
{
	HGLOBAL	hMem = (HGLOBAL)NULL;
	BYTE*	pMem = NULL;
	BOOL	bReleaseIC = FALSE;
	BYTE*	pHdr = NULL;
	LONG	cbHdr = 0;
	BOOL	bSucceeded = FALSE;
	BOOL	bInDecompress = FALSE;
	DWORD	biSizeImage;

	TRACE("(%p,%08x,%p,%p,%p)\n",
		hic, uiFlags, lpbiIn, lpBits, lpbiOut);

	if ( hic == (HIC)NULL )
	{
		hic = ICDecompressOpen( mmioFOURCC('V','I','D','C'), 0, &lpbiIn->bmiHeader, (lpbiOut != NULL) ? &lpbiOut->bmiHeader : NULL );
		if ( hic == (HIC)NULL )
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
	if ( hMem == (HGLOBAL)NULL )
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
	if ( !bSucceeded && hMem != (HGLOBAL)NULL )
	{
		GlobalFree(hMem); hMem = (HGLOBAL)NULL;
	}

	return (HANDLE)hMem;
}
