/*
 * Copyright 1998 Marcus Meissner
 * Copyright 2000 Bradley Baetz 
 *
 * FIXME: This all assumes 32 bit codecs
 *		Win95 appears to prefer 32 bit codecs, even from 16 bit code.
 *		There is the ICOpenFunction16 to worry about still, though.
 */

#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "vfw.h"
#include "wine/winestring.h"
#include "debugtools.h"
#include "ldt.h"
#include "heap.h"
#include "stackframe.h"

DEFAULT_DEBUG_CHANNEL(msvideo);

/* ### start build ### */
extern LONG CALLBACK MSVIDEO_CallTo16_long_lwwll(FARPROC16,LONG,WORD,WORD,LONG,LONG);
/* ### stop build ### */

LPVOID MSVIDEO_MapMsg16To32(UINT msg, LPDWORD lParam1, LPDWORD lParam2);
void MSVIDEO_UnmapMsg16To32(UINT msg, LPVOID lpv, LPDWORD lParam1, LPDWORD lParam2);
LRESULT MSVIDEO_SendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2, BOOL bFrom32);

/***********************************************************************
 *		VideoForWindowsVersion		[MSVFW.2][MSVIDEO.2]
 * Returns the version in major.minor form.
 * In Windows95 this returns 0x040003b6 (4.950)
 */
DWORD WINAPI VideoForWindowsVersion(void) {
	return 0x040003B6; /* 4.950 */
}

/***********************************************************************
 *		VideoCapDriverDescAndVer	[MSVIDEO.22]
 */
DWORD WINAPI VideoCapDriverDescAndVer(WORD nr,LPVOID buf1,WORD buf1len,LPVOID buf2,WORD buf2len) {
	FIXME("(%d,%p,%d,%p,%d), stub!\n",nr,buf1,buf1len,buf2,buf2len);
	return 0;
}

/* system.ini: [drivers] */

/***********************************************************************
 *		ICInfo				[MSVFW.33]
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

BOOL16 VFWAPI ICInfo16(DWORD fccType, DWORD fccHandler, ICINFO16* /*SEGPTR*/ lpicinfo) {
	BOOL16 ret;
	LPVOID lpv;
	DWORD lParam = (DWORD)lpicinfo;
	DWORD size = ((ICINFO*)(PTR_SEG_TO_LIN(lpicinfo)))->dwSize;

	/* Use the mapping functions to map the ICINFO structure */
	lpv = MSVIDEO_MapMsg16To32(ICM_GETINFO,&lParam,&size);

	ret = ICInfo(fccType,fccHandler,(ICINFO*)lParam);

	MSVIDEO_UnmapMsg16To32(ICM_GETINFO,lpv,&lParam,&size);
	
	return ret;
}

/***********************************************************************
 *		ICOpen				[MSVFW.37]
 * Opens an installable compressor. Return special handle.
 */
HIC VFWAPI ICOpen(DWORD fccType,DWORD fccHandler,UINT wMode) {
	char		type[5],handler[5],codecname[20];
	ICOPEN		icopen;
	HDRVR		hdrv;
	HIC16		hic;
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
	hic = GlobalAlloc16(GHND,sizeof(WINE_HIC));
	whic = (WINE_HIC*)GlobalLock16(hic);
	whic->hdrv	= hdrv;
	whic->driverproc= NULL;
	whic->private	= 0;
	GlobalUnlock16(hic);
	TRACE("=> 0x%08lx\n",(DWORD)hic);
	return hic;
}

HIC MSVIDEO_OpenFunc(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler, BOOL bFrom32) {
	char	type[5],handler[5],codecname[20];
	HIC16	hic;
	ICOPEN*  icopen = SEGPTR_NEW(ICOPEN);
	WINE_HIC	*whic;

	memcpy(type,&fccType,4);type[4]=0;
	memcpy(handler,&fccHandler,4);handler[4]=0;
	TRACE("(%s,%s,%d,%p,%d)\n",type,handler,wMode,lpfnHandler,bFrom32?32:16);
	
	icopen->fccType		= fccType;
	icopen->fccHandler	= fccHandler;
	icopen->dwSize		= sizeof(ICOPEN);
	icopen->dwFlags		= wMode;
	
	sprintf(codecname,"%s.%s",type,handler);

	hic = GlobalAlloc16(GHND,sizeof(WINE_HIC));
	if (!hic)
		return 0;
	whic = GlobalLock16(hic);
	whic->driverproc = lpfnHandler;
	
	whic->private = bFrom32;
	
	/* Now try opening/loading the driver. Taken from DRIVER_AddToList */
	/* What if the function is used more than once? */
	
	if (MSVIDEO_SendMessage(hic,DRV_LOAD,0L,0L,bFrom32) != DRV_SUCCESS) {
		WARN("DRV_LOAD failed for hic 0x%08lx\n",(DWORD)hic);
		GlobalFree16(hic);
		return 0;
	}
	if (MSVIDEO_SendMessage(hic,DRV_ENABLE,0L,0L,bFrom32) != DRV_SUCCESS) {
		WARN("DRV_ENABLE failed for hic 0x%08lx\n",(DWORD)hic);
		GlobalFree16(hic);
		return 0;
	}
	whic->hdrv = MSVIDEO_SendMessage(hic,DRV_OPEN,0,(LPARAM)(SEGPTR_GET(icopen)),FALSE);
	if (whic->hdrv == 0) {
		WARN("DRV_OPEN failed for hic 0x%08lx\n",(DWORD)hic);
		GlobalFree16(hic);
		return 0;
	}

	GlobalUnlock16(hic);
	TRACE("=> 0x%08lx\n",(DWORD)hic);
	return hic;
}

/***********************************************************************
 *		ICOpenFunction			[MSVFW.38]
 */
HIC VFWAPI ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler) {
	return MSVIDEO_OpenFunc(fccType,fccHandler,wMode,lpfnHandler,TRUE);
}

HIC16 VFWAPI ICOpen16(DWORD fccType, DWORD fccHandler, UINT16 wMode) {
	return (HIC16)ICOpen(fccType, fccHandler, wMode);
}

HIC16 VFWAPI ICOpenFunction16(DWORD fccType, DWORD fccHandler, UINT16 wMode, FARPROC16 lpfnHandler) {
	return MSVIDEO_OpenFunc(fccType, fccHandler, wMode, lpfnHandler,FALSE);
}

/***********************************************************************
 *		ICGetInfo			[MSVFW.30]
 */
LRESULT VFWAPI ICGetInfo(HIC hic,ICINFO *picinfo,DWORD cb) {
	LRESULT		ret;

	TRACE("(0x%08lx,%p,%ld)\n",(DWORD)hic,picinfo,cb);
	ret = ICSendMessage(hic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

LRESULT VFWAPI ICGetInfo16(HIC16 hic, ICINFO16 *picinfo,DWORD cb) {
	LRESULT		ret;

	TRACE("(0x%08lx,%p,%ld)\n",(DWORD)hic,picinfo,cb);
	ret = ICSendMessage16(hic,ICM_GETINFO,(DWORD)picinfo,cb);
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICLocate			[MSVFW.35]
 */
HIC VFWAPI ICLocate(
	DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
	LPBITMAPINFOHEADER lpbiOut, WORD wMode)
{
	char	type[5],handler[5];
	HIC	hic;
	DWORD	querymsg;
	LPSTR pszBuffer;

	TRACE("(0x%08lx,0x%08lx,%p,%p,0x%04x)\n", fccType, fccHandler, lpbiIn, lpbiOut, wMode);

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
			if (!lstrncmpiA(type,s,5)) {
				char *s2 = s;
				while (*s2 != '\0' && *s2 != '.') s2++;
				if (*s2) {
					HIC h;

					*s2++ = '\0';
					h = ICOpen(fccType,*(DWORD*)s2,wMode);
					if (h) {
						if (!ICSendMessage(h,querymsg,(DWORD)lpbiIn,(DWORD)lpbiOut))
							return h;
						ICClose(h);
					}
				}
			}
			s += lstrlenA(s) + 1;
		}
	}
	HeapFree(GetProcessHeap(),0,pszBuffer);
	
	if (fccType==streamtypeVIDEO) {
		hic = ICLocate(ICTYPE_VIDEO,fccHandler,lpbiIn,lpbiOut,wMode);
		if (hic)
			return hic;
	}

	type[4] = handler[4] = '\0';
	FIXME("(%.4s,%.4s,%p,%p,0x%04x),unhandled!\n",type,handler,lpbiIn,lpbiOut,wMode);
	return 0;
}

HIC16 VFWAPI ICLocate16(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn,
						LPBITMAPINFOHEADER lpbiOut, WORD wFlags) {
	return (HIC16)ICLocate(fccType, fccHandler, lpbiIn, lpbiOut, wFlags);
}

/***********************************************************************
 *		ICGetDisplayFormat			[MSVFW.29]
 */
HIC VFWAPI ICGetDisplayFormat(
	HIC hic,LPBITMAPINFOHEADER lpbiIn,LPBITMAPINFOHEADER lpbiOut,
	INT depth,INT dx,INT dy)
{
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
	ICDecompressGetFormat(tmphic,lpbiIn,lpbiOut);
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
	}
	if (lpbiIn->biBitCount == 8)
		depth = 8;
	
	TRACE("=> 0x%08lx\n",(DWORD)tmphic);
	return tmphic;
errout:
	if (hic!=tmphic)
		ICClose(tmphic);

	TRACE("=> 0\n");
	return 0;
}

HIC16 VFWAPI ICGetDisplayFormat16(HIC16 hic, LPBITMAPINFOHEADER lpbiIn,
								  LPBITMAPINFOHEADER lpbiOut, INT16 depth, INT16 dx, INT16 dy) {
	return (HIC16)ICGetDisplayFormat(hic,lpbiIn,lpbiOut,depth,dx,dy);
}

/***********************************************************************
 *		ICCompress			[MSVFW.23]
 */
DWORD VFWAPIV
ICCompress(
	HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev)
{
	ICCOMPRESS	iccmp;

	TRACE("(0x%08lx,%ld,%p,%p,%p,%p,...)\n",(DWORD)hic,dwFlags,lpbiOutput,lpData,lpbiInput,lpBits);

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

DWORD VFWAPIV ICCompress16(HIC16 hic, DWORD dwFlags, LPBITMAPINFOHEADER lpbiOutput, LPVOID lpData,
						   LPBITMAPINFOHEADER lpbiInput, LPVOID lpBits, LPDWORD lpckid,
						   LPDWORD lpdwFlags, LONG lFrameNum, DWORD dwFrameSize, DWORD dwQuality,
						   LPBITMAPINFOHEADER lpbiPrev, LPVOID lpPrev) {

	DWORD ret;
	ICCOMPRESS *iccmp = SEGPTR_NEW(ICCOMPRESS);

	TRACE("(0x%08lx,%ld,%p,%p,%p,%p,...)\n",(DWORD)hic,dwFlags,lpbiOutput,lpData,lpbiInput,lpBits);

	iccmp->dwFlags		= dwFlags;

	iccmp->lpbiOutput	= lpbiOutput;
	iccmp->lpOutput		= lpData;
	iccmp->lpbiInput		= lpbiInput;
	iccmp->lpInput		= lpBits;

	iccmp->lpckid		= lpckid;
	iccmp->lpdwFlags		= lpdwFlags;
	iccmp->lFrameNum		= lFrameNum;
	iccmp->dwFrameSize	= dwFrameSize;
	iccmp->dwQuality		= dwQuality;
	iccmp->lpbiPrev		= lpbiPrev;
	iccmp->lpPrev		= lpPrev;
	ret = ICSendMessage16(hic,ICM_COMPRESS,(DWORD)SEGPTR_GET(iccmp),sizeof(ICCOMPRESS));
	SEGPTR_FREE(iccmp);
	return ret;
}

/***********************************************************************
 *		ICDecompress			[MSVFW.26]
 */
DWORD VFWAPIV  ICDecompress(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,
				LPVOID lpData,LPBITMAPINFOHEADER lpbi,LPVOID lpBits)
{
	ICDECOMPRESS	icd;
	DWORD ret;

	TRACE("(0x%08lx,%ld,%p,%p,%p,%p)\n",(DWORD)hic,dwFlags,lpbiFormat,lpData,lpbi,lpBits);

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

DWORD VFWAPIV ICDecompress16(HIC16 hic, DWORD dwFlags, LPBITMAPINFOHEADER lpbiFormat,
							 LPVOID lpData, LPBITMAPINFOHEADER lpbi, LPVOID lpBits) {

	ICDECOMPRESS *icd = SEGPTR_NEW(ICDECOMPRESS);
	DWORD ret;
	
	TRACE("(0x%08lx,%ld,%p,%p,%p,%p)\n",(DWORD)hic,dwFlags,lpbiFormat,lpData,lpbi,lpBits);

	icd->dwFlags = dwFlags;
	icd->lpbiInput = lpbiFormat;
	icd->lpInput = lpData;
	icd->lpbiOutput = lpbi;
	icd->lpOutput = lpBits;
	icd->ckid = 0;

	ret = ICSendMessage16(hic,ICM_DECOMPRESS,(DWORD)SEGPTR_GET(icd),sizeof(ICDECOMPRESS));

	SEGPTR_FREE(icd);
	return ret;
}

#define COPY(x,y) (##x##->##y = ##x##16->##y);
#define COPYPTR(x,y) (##x##->##y = PTR_SEG_TO_LIN(##x##16->##y));

LPVOID MSVIDEO_MapICDEX16To32(LPDWORD lParam) {
	LPVOID ret;

	ICDECOMPRESSEX *icdx = HeapAlloc(GetProcessHeap(),0,sizeof(ICDECOMPRESSEX));
	ICDECOMPRESSEX16 *icdx16 = (ICDECOMPRESSEX16*)PTR_SEG_TO_LIN(*lParam);
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
		break;
	case DRV_OPEN:
	case ICM_GETDEFAULTQUALITY:
	case ICM_GETQUALITY:
		*lParam1 = (DWORD)PTR_SEG_TO_LIN(*lParam1);
		break;
	case ICM_GETINFO:
		{
			ICINFO *ici = HeapAlloc(GetProcessHeap(),0,sizeof(ICINFO));
			ICINFO16 *ici16;

			ici16 = (ICINFO16*)PTR_SEG_TO_LIN(*lParam1);
			ret = ici16;

			ici->dwSize = sizeof(ICINFO);
			COPY(ici,fccType);
			COPY(ici,fccHandler);
			COPY(ici,dwFlags);
			COPY(ici,dwVersion);
			COPY(ici,dwVersionICM);
			lstrcpynAtoW(ici->szName,ici16->szName,16);
			lstrcpynAtoW(ici->szDescription,ici16->szDescription,128);
			lstrcpynAtoW(ici->szDriver,ici16->szDriver,128);

			*lParam1 = (DWORD)(ici);
			*lParam2 = sizeof(ICINFO);
		}
		break;
	case ICM_COMPRESS:
		{
			ICCOMPRESS *icc = HeapAlloc(GetProcessHeap(),0,sizeof(ICCOMPRESS));
			ICCOMPRESS *icc16;

			icc16 = (ICCOMPRESS*)PTR_SEG_TO_LIN(*lParam1);
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
			
			icd16 = (ICDECOMPRESS*)PTR_SEG_TO_LIN(*lParam1);
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
		*lParam1 = (DWORD)PTR_SEG_TO_LIN(*lParam1);
		*lParam2 = (DWORD)PTR_SEG_TO_LIN(*lParam2);
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
			ICDRAWBEGIN16 *icdb16 = (ICDRAWBEGIN16*)PTR_SEG_TO_LIN(*lParam1);
			ret = icdb16;

			COPY(icdb,dwFlags);
			COPY(icdb,hpal);
			COPY(icdb,hwnd);
			COPY(icdb,hdc);
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
			ICDRAWSUGGEST16 *icds16 = (ICDRAWSUGGEST16*)PTR_SEG_TO_LIN(*lParam1);
			
			ret = icds16;

			COPY(icds,dwFlags);
			COPYPTR(icds,lpbiIn);
			COPYPTR(icds,lpbiSuggest);
			COPY(icds,dxSrc);
			COPY(icds,dySrc);
			COPY(icds,dxDst);
			COPY(icds,dyDst);
			COPY(icds,hicDecompressor);

			*lParam1 = (DWORD)(icds);
			*lParam2 = sizeof(ICDRAWSUGGEST);
		}
		break;
	case ICM_DRAW:
		{
			ICDRAW *icd = HeapAlloc(GetProcessHeap(),0,sizeof(ICDRAW));
			ICDRAW *icd16 = (ICDRAW*)PTR_SEG_TO_LIN(*lParam1);
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
	default:
		FIXME("%d is not yet handled. Expect a crash.\n",msg);
	}
	return ret;
}

#undef COPY
#undef COPYPTR

void MSVIDEO_UnmapMsg16To32(UINT msg, LPVOID data16, LPDWORD lParam1, LPDWORD lParam2) {
	TRACE("Unmapping %d\n",msg);

#define UNCOPY(x,y) (##x##16->##y = ##x##->##y);

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
			lstrcpynWtoA(ici16->szName,ici->szName,16);
			lstrcpynWtoA(ici16->szDescription,ici->szDescription,128);
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
	WINE_HIC	*whic = GlobalLock16(hic);
	LPVOID data16 = 0;
	BOOL bDrv32;

#define XX(x) case x: TRACE("(0x%08lx,"#x",0x%08lx,0x%08lx,%d)\n",(DWORD)hic,lParam1,lParam2,bFrom32?32:16);break;

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
		FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,%i) unknown message\n",(DWORD)hic,(DWORD)msg,lParam1,lParam2,bFrom32?32:16);
	}

#undef XX

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
			ret = MSVIDEO_CallTo16_long_lwwll((FARPROC16)whic->driverproc,whic->hdrv,hic,msg,lParam1,lParam2);
		}
	} else {
		ret = SendDriverMessage(whic->hdrv,msg,lParam1,lParam2);
	}

	if (data16)
		MSVIDEO_UnmapMsg16To32(msg,data16,&lParam1,&lParam2);
 
 out:
	GlobalUnlock16(hic);
	
	TRACE("	-> 0x%08lx\n",ret);
	return ret;
}

/***********************************************************************
 *		ICSendMessage			[MSVFW.40]
 */
LRESULT VFWAPI ICSendMessage(HIC hic, UINT msg, DWORD lParam1, DWORD lParam2) {
	return MSVIDEO_SendMessage(hic,msg,lParam1,lParam2,TRUE);
}

LRESULT VFWAPI ICSendMessage16(HIC16 hic, UINT16 msg, DWORD lParam1, DWORD lParam2) {
	return MSVIDEO_SendMessage(hic,msg,lParam1,lParam2,FALSE);
}

LRESULT VFWAPIV ICMessage16(void) {
	HIC16 hic;
	UINT16 msg;
	UINT16 cb;
	LPWORD lpData;
	LRESULT ret;
	UINT16 i;

	VA_LIST16 valist;
	
	VA_START16(valist);
	hic = VA_ARG16(valist, HIC16);
	msg = VA_ARG16(valist, UINT16);
	cb  = VA_ARG16(valist, UINT16);

	lpData = SEGPTR_ALLOC(cb);

	TRACE("0x%08lx, %u, %u, ...)\n",(DWORD)hic,msg,cb);

	for(i=0;i<cb/sizeof(WORD);i++) {
		lpData[i] = VA_ARG16(valist, WORD);
	}
		
	VA_END16(valist);
	ret = ICSendMessage16(hic, msg, (DWORD)(SEGPTR_GET(lpData)), (DWORD)cb);

	SEGPTR_FREE(lpData);
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

	TRACE("(0x%08lx,%ld,0x%08lx,0x%08lx,0x%08lx,%u,%u,%u,%u,%p,%u,%u,%u,%u,%ld,%ld)\n",
		  (DWORD)hic, dwFlags, (DWORD)hpal, (DWORD)hwnd, (DWORD)hdc, xDst, yDst, dxDst, dyDst,
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

DWORD VFWAPIV ICDrawBegin16(
							HIC16			hic,
							DWORD			dwFlags,/* flags */
							HPALETTE16		hpal,	/* palette to draw with */
							HWND16			hwnd,	/* window to draw to */
							HDC16			hdc,	/* HDC to draw to */
							INT16			xDst,	/* destination rectangle */
							INT16			yDst,
							INT16			dxDst,
							INT16			dyDst,
						 	LPBITMAPINFOHEADER /*SEGPTR*/ lpbi,	/* format of frame to draw */
							INT16			xSrc,	/* source rectangle */
							INT16			ySrc,
							INT16			dxSrc,
							INT16			dySrc,
							DWORD			dwRate,	/* frames/second = (dwRate/dwScale) */
							DWORD			dwScale) {

	DWORD ret;
	ICDRAWBEGIN16* icdb = SEGPTR_NEW(ICDRAWBEGIN16); /* SEGPTR for mapper to deal with */

	TRACE("(0x%08lx,%ld,0x%08lx,0x%08lx,0x%08lx,%u,%u,%u,%u,%p,%u,%u,%u,%u,%ld,%ld)\n",
		  (DWORD)hic, dwFlags, (DWORD)hpal, (DWORD)hwnd, (DWORD)hdc, xDst, yDst, dxDst, dyDst,
		  lpbi, xSrc, ySrc, dxSrc, dySrc, dwRate, dwScale);

	icdb->dwFlags = dwFlags;
	icdb->hpal = hpal;
	icdb->hwnd = hwnd;
	icdb->hdc = hdc;
	icdb->xDst = xDst;
	icdb->yDst = yDst;
	icdb->dxDst = dxDst;
	icdb->dyDst = dyDst;
	icdb->lpbi = lpbi; /* Keep this as SEGPTR for the mapping code to deal with */
	icdb->xSrc = xSrc;
	icdb->ySrc = ySrc;
	icdb->dxSrc = dxSrc;
	icdb->dySrc = dySrc;
	icdb->dwRate = dwRate;
	icdb->dwScale = dwScale;
	
	ret = (DWORD)ICSendMessage16(hic,ICM_DRAW_BEGIN,(DWORD)SEGPTR_GET(icdb),sizeof(ICDRAWBEGIN16));
	SEGPTR_FREE(icdb);
	return ret;
}

/***********************************************************************
 *		ICDraw			[MSVFW.27]
 */
DWORD VFWAPIV ICDraw(HIC hic, DWORD dwFlags, LPVOID lpFormat, LPVOID lpData, DWORD cbData, LONG lTime) {
	ICDRAW	icd;

	TRACE("(0x%09lx,%ld,%p,%p,%ld,%ld)\n",(DWORD)hic,dwFlags,lpFormat,lpData,cbData,lTime);

	icd.dwFlags = dwFlags;
	icd.lpFormat = lpFormat;
	icd.lpData = lpData;
	icd.cbData = cbData;
	icd.lTime = lTime;

	return ICSendMessage(hic,ICM_DRAW,(DWORD)&icd,sizeof(icd));
}

DWORD VFWAPIV ICDraw16(HIC16 hic, DWORD dwFlags, LPVOID /*SEGPTR*/ lpFormat,
					   LPVOID /*SEGPTR*/ lpData, DWORD cbData, LONG lTime) {

	ICDRAW* icd = SEGPTR_NEW(ICDRAW); /* SEGPTR for mapper to deal with */

	TRACE("(0x%08lx,0x%08lx,%p,%p,%ld,%ld)\n",(DWORD)hic,dwFlags,lpFormat,lpData,cbData,lTime);
	icd->dwFlags = dwFlags;
	icd->lpFormat = lpFormat;
	icd->lpData = lpData;
	icd->cbData = cbData;
	icd->lTime = lTime;

	return ICSendMessage16(hic,ICM_DRAW,(DWORD)SEGPTR_GET(icd),sizeof(ICDRAW));
}

/***********************************************************************
 *		ICClose			[MSVFW.22]
 */
LRESULT WINAPI ICClose(HIC hic) {
	WINE_HIC *whic = GlobalLock16(hic);
	TRACE("(0x%08lx)\n",(DWORD)hic);
	if (whic->driverproc) {
		ICSendMessage16(hic,DRV_CLOSE,0,0);
		ICSendMessage16(hic,DRV_DISABLE,0,0);
		ICSendMessage16(hic,DRV_FREE,0,0);
	} else {
		CloseDriver(whic->hdrv,0,0);
}

	GlobalUnlock16(hic);
	GlobalFree16(hic);
	return 0;
}

LRESULT WINAPI ICClose16(HIC16 hic) {
	return ICClose(hic);
}

/***********************************************************************
 *		MCIWndCreateA		[MSVFW.44 & MSVFW.45]
 */
HWND VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle,LPCSTR szFile)
{
	FIXME("%x %x %lx %s\n",hwndParent, hInstance, dwStyle, szFile);
	return 0;
}

/***********************************************************************
 *		MCIWndCreateW		[MSVFW.46]
 */
HWND VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance,
                      DWORD dwStyle,LPCWSTR szFile)
{
	FIXME("%x %x %lx %s\n",hwndParent, hInstance, dwStyle, debugstr_w(szFile));
	return 0;
}
