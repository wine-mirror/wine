/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_VFW_H
#define __WINE_VFW_H

#include "windef.h"
#include "mmsystem.h"
#include "wingdi.h"

#define VFWAPI	WINAPI
#define VFWAPIV	WINAPIV

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef HANDLE HDRAWDIB;

DWORD       VFWAPI  VideoForWindowsVersion(void);
LONG 	    VFWAPI  InitVFW(void);
LONG	    VFWAPI  TermVFW(void);

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IAVIStream IAVIStream,*PAVISTREAM;
typedef struct IAVIFile IAVIFile,*PAVIFILE;
typedef struct IGetFrame IGetFrame,*PGETFRAME;

/* Installable Compressor Manager */

DECLARE_HANDLE(HIC);

/* error return codes */
#define	ICERR_OK		0
#define	ICERR_DONTDRAW		1
#define	ICERR_NEWPALETTE	2
#define	ICERR_GOTOKEYFRAME	3
#define	ICERR_STOPDRAWING	4

#define	ICERR_UNSUPPORTED	-1
#define	ICERR_BADFORMAT		-2
#define	ICERR_MEMORY		-3
#define	ICERR_INTERNAL		-4
#define	ICERR_BADFLAGS		-5
#define	ICERR_BADPARAM		-6
#define	ICERR_BADSIZE		-7
#define	ICERR_BADHANDLE		-8
#define	ICERR_CANTUPDATE	-9
#define	ICERR_ABORT		-10
#define	ICERR_ERROR		-100
#define	ICERR_BADBITDEPTH	-200
#define	ICERR_BADIMAGESIZE	-201

#define	ICERR_CUSTOM		-400

/* ICM Messages */
#define	ICM_USER		(DRV_USER+0x0000)

/* ICM driver message range */
#define	ICM_RESERVED_LOW	(DRV_USER+0x1000)
#define	ICM_RESERVED_HIGH	(DRV_USER+0x2000)
#define	ICM_RESERVED		ICM_RESERVED_LOW

#define	ICM_GETSTATE		(ICM_RESERVED+0)
#define	ICM_SETSTATE		(ICM_RESERVED+1)
#define	ICM_GETINFO		(ICM_RESERVED+2)

#define	ICM_CONFIGURE		(ICM_RESERVED+10)
#define	ICM_ABOUT		(ICM_RESERVED+11)
/* */

#define	ICM_GETDEFAULTQUALITY	(ICM_RESERVED+30)
#define	ICM_GETQUALITY		(ICM_RESERVED+31)
#define	ICM_SETQUALITY		(ICM_RESERVED+32)

#define	ICM_SET			(ICM_RESERVED+40)
#define	ICM_GET			(ICM_RESERVED+41)

/* 2 constant FOURCC codes */
#define ICM_FRAMERATE		mmioFOURCC('F','r','m','R')
#define ICM_KEYFRAMERATE	mmioFOURCC('K','e','y','R')

#define	ICM_COMPRESS_GET_FORMAT		(ICM_USER+4)
#define	ICM_COMPRESS_GET_SIZE		(ICM_USER+5)
#define	ICM_COMPRESS_QUERY		(ICM_USER+6)
#define	ICM_COMPRESS_BEGIN		(ICM_USER+7)
#define	ICM_COMPRESS			(ICM_USER+8)
#define	ICM_COMPRESS_END		(ICM_USER+9)

#define	ICM_DECOMPRESS_GET_FORMAT	(ICM_USER+10)
#define	ICM_DECOMPRESS_QUERY		(ICM_USER+11)
#define	ICM_DECOMPRESS_BEGIN		(ICM_USER+12)
#define	ICM_DECOMPRESS			(ICM_USER+13)
#define	ICM_DECOMPRESS_END		(ICM_USER+14)
#define	ICM_DECOMPRESS_SET_PALETTE	(ICM_USER+29)
#define	ICM_DECOMPRESS_GET_PALETTE	(ICM_USER+30)

#define	ICM_DRAW_QUERY			(ICM_USER+31)
#define	ICM_DRAW_BEGIN			(ICM_USER+15)
#define	ICM_DRAW_GET_PALETTE		(ICM_USER+16)
#define	ICM_DRAW_START			(ICM_USER+18)
#define	ICM_DRAW_STOP			(ICM_USER+19)
#define	ICM_DRAW_END			(ICM_USER+21)
#define	ICM_DRAW_GETTIME		(ICM_USER+32)
#define	ICM_DRAW			(ICM_USER+33)
#define	ICM_DRAW_WINDOW			(ICM_USER+34)
#define	ICM_DRAW_SETTIME		(ICM_USER+35)
#define	ICM_DRAW_REALIZE		(ICM_USER+36)
#define	ICM_DRAW_FLUSH			(ICM_USER+37)
#define	ICM_DRAW_RENDERBUFFER		(ICM_USER+38)

#define	ICM_DRAW_START_PLAY		(ICM_USER+39)
#define	ICM_DRAW_STOP_PLAY		(ICM_USER+40)

#define	ICM_DRAW_SUGGESTFORMAT		(ICM_USER+50)
#define	ICM_DRAW_CHANGEPALETTE		(ICM_USER+51)

#define	ICM_GETBUFFERSWANTED		(ICM_USER+41)

#define	ICM_GETDEFAULTKEYFRAMERATE	(ICM_USER+42)

#define	ICM_DECOMPRESSEX_BEGIN		(ICM_USER+60)
#define	ICM_DECOMPRESSEX_QUERY		(ICM_USER+61)
#define	ICM_DECOMPRESSEX		(ICM_USER+62)
#define	ICM_DECOMPRESSEX_END		(ICM_USER+63)

#define	ICM_COMPRESS_FRAMES_INFO	(ICM_USER+70)
#define	ICM_SET_STATUS_PROC		(ICM_USER+72)

/* structs */

/* NOTE: Only the 16 bit structs are packed. Structs that are packed anyway
 * have not been changed. If a structure is later extended, you may need to create
 * two versions of it.
 */

typedef struct {
	DWORD	dwSize;		/* 00: size */
	DWORD	fccType;	/* 04: type 'vidc' usually */
	DWORD	fccHandler;	/* 08: */
	DWORD	dwVersion;	/* 0c: version of compman opening you */
	DWORD	dwFlags;	/* 10: LOWORD is type specific */
	LRESULT	dwError;	/* 14: */
	LPVOID	pV1Reserved;	/* 18: */
	LPVOID	pV2Reserved;	/* 1c: */
	DWORD	dnDevNode;	/* 20: */
				/* 24: */
} ICOPEN,*LPICOPEN;

#define ICCOMPRESS_KEYFRAME     0x00000001L

typedef struct {
    DWORD		dwFlags;
    LPBITMAPINFOHEADER	lpbiOutput;
    LPVOID		lpOutput;
    LPBITMAPINFOHEADER	lpbiInput;
    LPVOID		lpInput;
    LPDWORD		lpckid;
    LPDWORD		lpdwFlags;
    LONG		lFrameNum;
    DWORD		dwFrameSize;
    DWORD		dwQuality;
    LPBITMAPINFOHEADER	lpbiPrev;
    LPVOID		lpPrev;
} ICCOMPRESS;

DWORD VFWAPIV ICCompress(
	HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev
);

#define ICCompressGetFormat(hic, lpbiInput, lpbiOutput) 		\
	ICSendMessage(							\
	    hic,ICM_COMPRESS_GET_FORMAT,(DWORD)(LPVOID)(lpbiInput),	\
	    (DWORD)(LPVOID)(lpbiOutput)					\
	)

#define ICCompressGetFormatSize(hic,lpbi) ICCompressGetFormat(hic,lpbi,NULL)

#define ICCompressBegin(hic, lpbiInput, lpbiOutput) 			\
    ICSendMessage(							\
    	hic, ICM_COMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput),		\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )

#define ICCompressGetSize(hic, lpbiInput, lpbiOutput) 			\
    ICSendMessage(							\
    	hic, ICM_COMPRESS_GET_SIZE, (DWORD)(LPVOID)(lpbiInput), 	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )

#define ICCompressQuery(hic, lpbiInput, lpbiOutput)		\
    ICSendMessage(						\
    	hic, ICM_COMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)				\
    )

#define ICCompressEnd(hic) ICSendMessage(hic, ICM_COMPRESS_END, 0, 0)

/* ICCOMPRESSFRAMES.dwFlags */
#define ICCOMPRESSFRAMES_PADDING        0x00000001
typedef struct {
    DWORD               dwFlags;
    LPBITMAPINFOHEADER  lpbiOutput;
    LPARAM              lOutput;
    LPBITMAPINFOHEADER  lpbiInput;
    LPARAM              lInput;
    LONG                lStartFrame;
    LONG                lFrameCount;
    LONG                lQuality;
    LONG                lDataRate;
    LONG                lKeyRate;
    DWORD               dwRate;
    DWORD               dwScale;
    DWORD               dwOverheadPerFrame;
    DWORD               dwReserved2;
    LONG (CALLBACK *GetData)(LPARAM lInput,LONG lFrame,LPVOID lpBits,LONG len);
    LONG (CALLBACK *PutData)(LPARAM lOutput,LONG lFrame,LPVOID lpBits,LONG len);
} ICCOMPRESSFRAMES;

typedef struct {
    DWORD		dwFlags;
    LPARAM		lParam;
   /* messages for Status callback */
#define ICSTATUS_START	    0
#define ICSTATUS_STATUS	    1
#define ICSTATUS_END	    2
#define ICSTATUS_ERROR	    3
#define ICSTATUS_YIELD	    4
    /* FIXME: some X11 libs define Status as int... */
    /* LONG (CALLBACK *zStatus)(LPARAM lParam, UINT message, LONG l); */
    LONG (CALLBACK *zStatus)(LPARAM lParam, UINT message, LONG l);
} ICSETSTATUSPROC;

/* Values for wMode of ICOpen() */
#define	ICMODE_COMPRESS		1
#define	ICMODE_DECOMPRESS	2
#define	ICMODE_FASTDECOMPRESS	3
#define	ICMODE_QUERY		4
#define	ICMODE_FASTCOMPRESS	5
#define	ICMODE_DRAW		8

/* quality flags */
#define ICQUALITY_LOW       0
#define ICQUALITY_HIGH      10000
#define ICQUALITY_DEFAULT   -1

typedef struct {
	DWORD	dwSize;		/* 00: */
	DWORD	fccType;	/* 04:compressor type     'vidc' 'audc' */
	DWORD	fccHandler;	/* 08:compressor sub-type 'rle ' 'jpeg' 'pcm '*/
	DWORD	dwFlags;	/* 0c:flags LOWORD is type specific */
	DWORD	dwVersion;	/* 10:version of the driver */
	DWORD	dwVersionICM;	/* 14:version of the ICM used */
	/*
	 * under Win32, the driver always returns UNICODE strings.
	 */
	WCHAR	szName[16];		/* 18:short name */
	WCHAR	szDescription[128];	/* 38:long name */
	WCHAR	szDriver[128];		/* 138:driver that contains compressor*/
					/* 238: */
} ICINFO;

/* ICINFO.dwFlags */
#define	VIDCF_QUALITY		0x0001  /* supports quality */
#define	VIDCF_CRUNCH		0x0002  /* supports crunching to a frame size */
#define	VIDCF_TEMPORAL		0x0004  /* supports inter-frame compress */
#define	VIDCF_COMPRESSFRAMES	0x0008  /* wants the compress all frames message */
#define	VIDCF_DRAW		0x0010  /* supports drawing */
#define	VIDCF_FASTTEMPORALC	0x0020  /* does not need prev frame on compress */
#define	VIDCF_FASTTEMPORALD	0x0080  /* does not need prev frame on decompress */
#define	VIDCF_QUALITYTIME	0x0040  /* supports temporal quality */

#define	VIDCF_FASTTEMPORAL	(VIDCF_FASTTEMPORALC|VIDCF_FASTTEMPORALD)


/* function shortcuts */
/* ICM_ABOUT */
#define ICMF_ABOUT_QUERY         0x00000001

#define ICQueryAbout(hic) \
	(ICSendMessage(hic,ICM_ABOUT,(DWORD)-1,ICMF_ABOUT_QUERY)==ICERR_OK)

#define ICAbout(hic, hwnd) ICSendMessage(hic,ICM_ABOUT,(DWORD)(UINT)(hwnd),0)

/* ICM_CONFIGURE */
#define ICMF_CONFIGURE_QUERY	0x00000001
#define ICQueryConfigure(hic) \
	(ICSendMessage(hic,ICM_CONFIGURE,(DWORD)-1,ICMF_CONFIGURE_QUERY)==ICERR_OK)

#define ICConfigure(hic,hwnd) \
	ICSendMessage(hic,ICM_CONFIGURE,(DWORD)(UINT)(hwnd),0)

/* Decompression stuff */
#define ICDECOMPRESS_HURRYUP		0x80000000	/* don't draw just buffer (hurry up!) */
#define ICDECOMPRESS_UPDATE		0x40000000	/* don't draw just update screen */
#define ICDECOMPRESS_PREROLL		0x20000000	/* this frame is before real start */
#define ICDECOMPRESS_NULLFRAME		0x10000000	/* repeat last frame */
#define ICDECOMPRESS_NOTKEYFRAME	0x08000000	/* this frame is not a key frame */

typedef struct {
    DWORD		dwFlags;	/* flags (from AVI index...) */
    LPBITMAPINFOHEADER	lpbiInput;	/* BITMAPINFO of compressed data */
    LPVOID		lpInput;	/* compressed data */
    LPBITMAPINFOHEADER	lpbiOutput;	/* DIB to decompress to */
    LPVOID		lpOutput;
    DWORD		ckid;		/* ckid from AVI file */
} ICDECOMPRESS;

typedef struct {
    DWORD		dwFlags;
    LPBITMAPINFOHEADER	lpbiSrc;
    LPVOID		lpSrc;
    LPBITMAPINFOHEADER	lpbiDst;
    LPVOID		lpDst;

    /* changed for ICM_DECOMPRESSEX */
    INT			xDst;       /* destination rectangle */
    INT			yDst;
    INT			dxDst;
    INT			dyDst;

    INT			xSrc;       /* source rectangle */
    INT			ySrc;
    INT			dxSrc;
    INT			dySrc;
} ICDECOMPRESSEX;

DWORD VFWAPIV ICDecompress(HIC hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,LPVOID lpData,LPBITMAPINFOHEADER lpbi,LPVOID lpBits);

#define ICDecompressBegin(hic, lpbiInput, lpbiOutput) 	\
    ICSendMessage(						\
    	hic, ICM_DECOMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)				\
    )

#define ICDecompressQuery(hic, lpbiInput, lpbiOutput) 	\
    ICSendMessage(						\
    	hic,ICM_DECOMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD) (LPVOID)(lpbiOutput)				\
    )

#define ICDecompressGetFormat(hic, lpbiInput, lpbiOutput)		\
    ((LONG)ICSendMessage(						\
    	hic,ICM_DECOMPRESS_GET_FORMAT, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    ))

#define ICDecompressGetFormatSize(hic, lpbi) 				\
	ICDecompressGetFormat(hic, lpbi, NULL)

#define ICDecompressGetPalette(hic, lpbiInput, lpbiOutput)		\
    ICSendMessage(							\
    	hic, ICM_DECOMPRESS_GET_PALETTE, (DWORD)(LPVOID)(lpbiInput), 	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )

#define ICDecompressSetPalette(hic,lpbiPalette)	\
        ICSendMessage(				\
		hic,ICM_DECOMPRESS_SET_PALETTE,		\
		(DWORD)(LPVOID)(lpbiPalette),0		\
	)

#define ICDecompressEnd(hic) ICSendMessage(hic, ICM_DECOMPRESS_END, 0, 0)

LRESULT	VFWAPI	ICSendMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2);

inline static LRESULT VFWAPI ICDecompressEx(HIC hic, DWORD dwFlags,
					    LPBITMAPINFOHEADER lpbiSrc, LPVOID lpSrc,
					    int xSrc, int ySrc, int dxSrc, int dySrc,
					    LPBITMAPINFOHEADER lpbiDst, LPVOID lpDst,
					    int xDst, int yDst, int dxDst, int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;
    return ICSendMessage(hic, ICM_DECOMPRESSEX, (DWORD)&ic, sizeof(ic));
}

inline static LRESULT VFWAPI ICDecompressExBegin(HIC hic, DWORD dwFlags,
						 LPBITMAPINFOHEADER lpbiSrc,
						 LPVOID lpSrc,
						 int xSrc, int ySrc, int dxSrc, int dySrc,
						 LPBITMAPINFOHEADER lpbiDst,
						 LPVOID lpDst,
						 int xDst,
						 int yDst,
						 int dxDst,
						 int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;
    return ICSendMessage(hic, ICM_DECOMPRESSEX_BEGIN, (DWORD)&ic, sizeof(ic));
}
inline static LRESULT VFWAPI ICDecompressExQuery(HIC hic, DWORD dwFlags,
						 LPBITMAPINFOHEADER lpbiSrc,
						 LPVOID lpSrc,
						 int xSrc, int ySrc, int dxSrc, int dySrc,
						 LPBITMAPINFOHEADER lpbiDst,
						 LPVOID lpDst,
						 int xDst,
						 int yDst,
						 int dxDst,
						 int dyDst)
{
    ICDECOMPRESSEX ic;

    ic.dwFlags = dwFlags;
    ic.lpbiSrc = lpbiSrc;
    ic.lpSrc = lpSrc;
    ic.xSrc = xSrc;
    ic.ySrc = ySrc;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.lpbiDst = lpbiDst;
    ic.lpDst = lpDst;
    ic.xDst = xDst;
    ic.yDst = yDst;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;
    return ICSendMessage(hic, ICM_DECOMPRESSEX_QUERY, (DWORD)&ic, sizeof(ic));
}

#define ICDecompressExEnd(hic) \
    ICSendMessage(hic, ICM_DECOMPRESSEX_END, 0, 0)

#define ICDRAW_QUERY        0x00000001L   /* test for support */
#define ICDRAW_FULLSCREEN   0x00000002L   /* draw to full screen */
#define ICDRAW_HDC          0x00000004L   /* draw to a HDC/HWND */
#define ICDRAW_ANIMATE	    0x00000008L	  /* expect palette animation */
#define ICDRAW_CONTINUE	    0x00000010L	  /* draw is a continuation of previous draw */
#define ICDRAW_MEMORYDC	    0x00000020L	  /* DC is offscreen, by the way */
#define ICDRAW_UPDATING	    0x00000040L	  /* We're updating, as opposed to playing */
#define ICDRAW_RENDER       0x00000080L   /* used to render data not draw it */
#define ICDRAW_BUFFER       0x00000100L   /* buffer data offscreen, we will need to update it */

#define ICDecompressOpen(fccType, fccHandler, lpbiIn, lpbiOut) \
    ICLocate(fccType, fccHandler, lpbiIn, lpbiOut, ICMODE_DECOMPRESS)

#define ICDrawOpen(fccType, fccHandler, lpbiIn) \
    ICLocate(fccType, fccHandler, lpbiIn, NULL, ICMODE_DRAW)

HANDLE VFWAPI ICImageCompress(HIC hic, UINT uiFlags, LPBITMAPINFO lpbiIn,
			      LPVOID lpBits, LPBITMAPINFO lpbiOut, LONG lQuality,
			      LONG* plSize);

HANDLE VFWAPI ICImageDecompress(HIC hic, UINT uiFlags, LPBITMAPINFO lpbiIn,
				LPVOID lpBits, LPBITMAPINFO lpbiOut);

BOOL	VFWAPI	ICInfo(DWORD fccType, DWORD fccHandler, ICINFO * lpicinfo);
BOOL    VFWAPI  ICInstall(DWORD fccType, DWORD fccHandler, LPARAM lParam, LPSTR szDesc, UINT wFlags);
BOOL    VFWAPI  ICRemove(DWORD fccType, DWORD fccHandler, UINT wFlags);
LRESULT	VFWAPI	ICGetInfo(HIC hic,ICINFO *picinfo, DWORD cb);
HIC	VFWAPI	ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode);
HIC	VFWAPI	ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler);

LRESULT VFWAPI	ICClose(HIC hic);
HIC	VFWAPI	ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);
HIC	VFWAPI	ICGetDisplayFormat(HIC hic, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, int BitDepth, int dx, int dy);

/* Values for wFlags of ICInstall() */
#define ICINSTALL_UNICODE       0x8000
#define ICINSTALL_FUNCTION      0x0001
#define ICINSTALL_DRIVER        0x0002
#define ICINSTALL_HDRV          0x0004
#define ICINSTALL_DRIVERW       0x8002

#define ICGetState(hic, pv, cb) \
    ICSendMessage(hic, ICM_GETSTATE, (DWORD)(LPVOID)(pv), (DWORD)(cb))
#define ICSetState(hic, pv, cb) \
    ICSendMessage(hic, ICM_SETSTATE, (DWORD)(LPVOID)(pv), (DWORD)(cb))
#define ICGetStateSize(hic) \
    ICGetState(hic, NULL, 0)

inline static DWORD ICGetDefaultQuality(HIC hic)
{
   DWORD dwICValue;
   ICSendMessage(hic, ICM_GETDEFAULTQUALITY, (DWORD)(LPVOID)&dwICValue, sizeof(DWORD));
   return dwICValue;
}

inline static DWORD ICGetDefaultKeyFrameRate(HIC hic)
{
   DWORD dwICValue;
   ICSendMessage(hic, ICM_GETDEFAULTKEYFRAMERATE, (DWORD)(LPVOID)&dwICValue, sizeof(DWORD));
   return dwICValue;
}

#define ICDrawWindow(hic, prc) \
    ICSendMessage(hic, ICM_DRAW_WINDOW, (DWORD)(LPVOID)(prc), sizeof(RECT))

/* As passed to ICM_DRAW_SUGGESTFORMAT */
typedef struct {
	DWORD dwFlags;
	LPBITMAPINFOHEADER lpbiIn;
	LPBITMAPINFOHEADER lpbiSuggest;
	INT dxSrc;
	INT dySrc;
	INT dxDst;
	INT dyDst;
	HIC hicDecompressor;
} ICDRAWSUGGEST;

typedef struct {
    DWORD               dwFlags;
    int                 iStart;
    int                 iLen;
    LPPALETTEENTRY      lppe;
} ICPALETTE;

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
        DWORD			dwScale
);

/* as passed to ICM_DRAW_BEGIN */
typedef struct {
	DWORD		dwFlags;
	HPALETTE	hpal;
	HWND		hwnd;
	HDC		hdc;
	INT		xDst;
	INT		yDst;
	INT		dxDst;
	INT		dyDst;
	LPBITMAPINFOHEADER	lpbi;
	INT		xSrc;
	INT		ySrc;
	INT		dxSrc;
	INT		dySrc;
	DWORD		dwRate;
	DWORD		dwScale;
} ICDRAWBEGIN;

#define ICDRAW_HURRYUP      0x80000000L   /* don't draw just buffer (hurry up!) */
#define ICDRAW_UPDATE       0x40000000L   /* don't draw just update screen */
#define ICDRAW_PREROLL      0x20000000L   /* this frame is before real start */
#define ICDRAW_NULLFRAME    0x10000000L   /* repeat last frame */
#define ICDRAW_NOTKEYFRAME  0x08000000L   /* this frame is not a key frame */

typedef struct {
	DWORD	dwFlags;
	LPVOID	lpFormat;
	LPVOID	lpData;
	DWORD	cbData;
	LONG	lTime;
} ICDRAW;

DWORD VFWAPIV ICDraw(HIC hic,DWORD dwFlags,LPVOID lpFormat,LPVOID lpData,DWORD cbData,LONG lTime);

inline static LRESULT VFWAPI ICDrawSuggestFormat(HIC hic, LPBITMAPINFOHEADER lpbiIn,
						 LPBITMAPINFOHEADER lpbiOut,
						 int dxSrc, int dySrc,
						 int dxDst, int dyDst,
						 HIC hicDecomp)
{
    ICDRAWSUGGEST ic;

    ic.lpbiIn = lpbiIn;
    ic.lpbiSuggest = lpbiOut;
    ic.dxSrc = dxSrc;
    ic.dySrc = dySrc;
    ic.dxDst = dxDst;
    ic.dyDst = dyDst;
    ic.hicDecompressor = hicDecomp;
    return ICSendMessage(hic, ICM_DRAW_SUGGESTFORMAT, (DWORD)&ic, sizeof(ic));
}

#define ICDrawQuery(hic, lpbiInput) \
    ICSendMessage(hic, ICM_DRAW_QUERY, (DWORD)(LPVOID)(lpbiInput), 0L)

#define ICDrawChangePalette(hic, lpbiInput) \
    ICSendMessage(hic, ICM_DRAW_CHANGEPALETTE, (DWORD)(LPVOID)(lpbiInput), 0L)

#define ICGetBuffersWanted(hic, lpdwBuffers) \
    ICSendMessage(hic, ICM_GETBUFFERSWANTED, (DWORD)(LPVOID)(lpdwBuffers), 0)

#define ICDrawEnd(hic) \
    ICSendMessage(hic, ICM_DRAW_END, 0, 0)

#define ICDrawStart(hic) \
    ICSendMessage(hic, ICM_DRAW_START, 0, 0)

#define ICDrawStartPlay(hic, lFrom, lTo) \
    ICSendMessage(hic, ICM_DRAW_START_PLAY, (DWORD)(lFrom), (DWORD)(lTo))

#define ICDrawStop(hic) \
    ICSendMessage(hic, ICM_DRAW_STOP, 0, 0)

#define ICDrawStopPlay(hic) \
    ICSendMessage(hic, ICM_DRAW_STOP_PLAY, 0, 0)

#define ICDrawGetTime(hic, lplTime) \
    ICSendMessage(hic, ICM_DRAW_GETTIME, (DWORD)(LPVOID)(lplTime), 0)

#define ICDrawSetTime(hic, lTime) \
    ICSendMessage(hic, ICM_DRAW_SETTIME, (DWORD)lTime, 0)

#define ICDrawRealize(hic, hdc, fBackground) \
    ICSendMessage(hic, ICM_DRAW_REALIZE, (DWORD)(UINT)(HDC)(hdc), (DWORD)(BOOL)(fBackground))

#define ICDrawFlush(hic) \
    ICSendMessage(hic, ICM_DRAW_FLUSH, 0, 0)

#define ICDrawRenderBuffer(hic) \
    ICSendMessage(hic, ICM_DRAW_RENDERBUFFER, 0, 0)

inline static LRESULT VFWAPI ICSetStatusProc(HIC hic, DWORD dwFlags, LRESULT lParam,
					     LONG (CALLBACK *fpfnStatus)(LPARAM, UINT, LONG))
{
    ICSETSTATUSPROC ic;

    ic.dwFlags = dwFlags;
    ic.lParam = lParam;
    /* FIXME: see comment in ICSETSTATUSPROC definition */
    ic.zStatus = fpfnStatus;

    return ICSendMessage(hic, ICM_SET_STATUS_PROC, (DWORD)&ic, sizeof(ic));
}

typedef struct {
    LONG		cbSize;
    DWORD		dwFlags;
    HIC			hic;
    DWORD               fccType;
    DWORD               fccHandler;
    LPBITMAPINFO	lpbiIn;
    LPBITMAPINFO	lpbiOut;
    LPVOID		lpBitsOut;
    LPVOID		lpBitsPrev;
    LONG		lFrame;
    LONG		lKey;
    LONG		lDataRate;
    LONG		lQ;
    LONG		lKeyCount;
    LPVOID		lpState;
    LONG		cbState;
} COMPVARS, *PCOMPVARS;

#define ICMF_COMPVARS_VALID	0x00000001

BOOL VFWAPI ICCompressorChoose(HWND hwnd, UINT uiFlags, LPVOID pvIn, LPVOID lpData,
			       PCOMPVARS pc, LPSTR lpszTitle);

#define ICMF_CHOOSE_KEYFRAME		0x0001
#define ICMF_CHOOSE_DATARATE		0x0002
#define ICMF_CHOOSE_PREVIEW		0x0004
#define ICMF_CHOOSE_ALLCOMPRESSORS	0x0008

BOOL VFWAPI ICSeqCompressFrameStart(PCOMPVARS pc, LPBITMAPINFO lpbiIn);
void VFWAPI ICSeqCompressFrameEnd(PCOMPVARS pc);

LPVOID VFWAPI ICSeqCompressFrame(PCOMPVARS pc, UINT uiFlags, LPVOID lpBits,
				 BOOL *pfKey, LONG *plSize);
void VFWAPI ICCompressorFree(PCOMPVARS pc);

/********************* AVIFILE function declarations *************************/

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
	( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |		\
	( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

typedef WORD TWOCC;

#define ICTYPE_VIDEO		mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO		mmioFOURCC('a', 'u', 'd', 'c')

#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME		mmioFOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI          mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')

#define FromHex(n)		(((n) >= 'A') ? ((n) + 10 - 'A') : ((n) - '0'))
#define StreamFromFOURCC(fcc)	((WORD)((FromHex(LOBYTE(LOWORD(fcc))) << 4) + \
					(FromHex(HIBYTE(LOWORD(fcc))))))
#define TWOCCFromFOURCC(fcc)    HIWORD(fcc)
#define ToHex(n)		((BYTE)(((n) > 9) ? ((n) - 10 + 'A') : ((n) + '0')))
#define MAKEAVICKID(tcc, stream) \
                                MAKELONG((ToHex((stream) & 0x0f) << 8) | \
			                 (ToHex(((stream) & 0xf0) >> 4)), tcc)

/* AVIFileHdr.dwFlags */
#define AVIF_HASINDEX		0x00000010	/* Index at end of file? */
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_TRUSTCKTYPE	0x00000800	/* Use CKType to find key frames*/
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

#define AVI_HEADERSIZE	2048

typedef BOOL (CALLBACK *AVISAVECALLBACK)(INT);

typedef struct _MainAVIHeader
{
    DWORD	dwMicroSecPerFrame;
    DWORD	dwMaxBytesPerSec;
    DWORD	dwPaddingGranularity;
    DWORD	dwFlags;
    DWORD	dwTotalFrames;
    DWORD	dwInitialFrames;
    DWORD	dwStreams;
    DWORD	dwSuggestedBufferSize;
    DWORD	dwWidth;
    DWORD	dwHeight;
    DWORD	dwReserved[4];
} MainAVIHeader;

/* AVIStreamHeader.dwFlags */
#define AVISF_DISABLED                  0x00000001
#define AVISF_VIDEO_PALCHANGES          0x00010000

typedef struct {
    FOURCC	fccType;
    FOURCC	fccHandler;
    DWORD	dwFlags;        /* AVISF_* */
    WORD	wPriority;
    WORD	wLanguage;
    DWORD	dwInitialFrames;
    DWORD	dwScale;
    DWORD	dwRate; /* dwRate / dwScale == samples/second */
    DWORD	dwStart;
    DWORD	dwLength; /* In units above... */
    DWORD	dwSuggestedBufferSize;
    DWORD	dwQuality;
    DWORD	dwSampleSize;
    struct { SHORT left, top, right, bottom; } rcFrame; /* word.word - word.word in file */
} AVIStreamHeader;

/* AVIINDEXENTRY.dwFlags */
#define AVIIF_LIST	0x00000001	/* chunk is a 'LIST' */
#define AVIIF_TWOCC	0x00000002
#define AVIIF_KEYFRAME	0x00000010	/* this frame is a key frame. */

#define AVIIF_NOTIME	0x00000100	/* this frame doesn't take any time */
#define AVIIF_COMPUSE	0x0FFF0000

typedef struct _AVIINDEXENTRY {
    DWORD	ckid;
    DWORD	dwFlags;
    DWORD	dwChunkOffset;
    DWORD	dwChunkLength;
} AVIINDEXENTRY;

typedef struct _AVIPALCHANGE {
    BYTE		bFirstEntry;
    BYTE		bNumEntries;
    WORD		wFlags;		/* pad */
    PALETTEENTRY	peNew[1];
} AVIPALCHANGE;

#define AVIIF_KEYFRAME	0x00000010	/* this frame is a key frame. */

#define	AVIGETFRAMEF_BESTDISPLAYFMT	1

typedef struct _AVISTREAMINFOA {
    DWORD	fccType;
    DWORD	fccHandler;
    DWORD	dwFlags;        /* AVIIF_* */
    DWORD	dwCaps;
    WORD	wPriority;
    WORD	wLanguage;
    DWORD	dwScale;
    DWORD	dwRate;		/* dwRate / dwScale == samples/second */
    DWORD	dwStart;
    DWORD	dwLength;	/* In units above... */
    DWORD	dwInitialFrames;
    DWORD	dwSuggestedBufferSize;
    DWORD	dwQuality;
    DWORD	dwSampleSize;
    RECT	rcFrame;
    DWORD	dwEditCount;
    DWORD	dwFormatChangeCount;
    CHAR	szName[64];
} AVISTREAMINFOA, * LPAVISTREAMINFOA, *PAVISTREAMINFOA;

typedef struct _AVISTREAMINFOW {
    DWORD	fccType;
    DWORD	fccHandler;
    DWORD	dwFlags;
    DWORD	dwCaps;
    WORD	wPriority;
    WORD	wLanguage;
    DWORD	dwScale;
    DWORD	dwRate;		/* dwRate / dwScale == samples/second */
    DWORD	dwStart;
    DWORD	dwLength;	/* In units above... */
    DWORD	dwInitialFrames;
    DWORD	dwSuggestedBufferSize;
    DWORD	dwQuality;
    DWORD	dwSampleSize;
    RECT	rcFrame;
    DWORD	dwEditCount;
    DWORD	dwFormatChangeCount;
    WCHAR	szName[64];
} AVISTREAMINFOW, * LPAVISTREAMINFOW, *PAVISTREAMINFOW;
DECL_WINELIB_TYPE_AW(AVISTREAMINFO)
DECL_WINELIB_TYPE_AW(LPAVISTREAMINFO)
DECL_WINELIB_TYPE_AW(PAVISTREAMINFO)

#define AVISTREAMINFO_DISABLED		0x00000001
#define AVISTREAMINFO_FORMATCHANGES	0x00010000

/* AVIFILEINFO.dwFlags */
#define AVIFILEINFO_HASINDEX		0x00000010
#define AVIFILEINFO_MUSTUSEINDEX	0x00000020
#define AVIFILEINFO_ISINTERLEAVED	0x00000100
#define AVIFILEINFO_TRUSTCKTYPE         0x00000800
#define AVIFILEINFO_WASCAPTUREFILE	0x00010000
#define AVIFILEINFO_COPYRIGHTED		0x00020000

/* AVIFILEINFO.dwCaps */
#define AVIFILECAPS_CANREAD		0x00000001
#define AVIFILECAPS_CANWRITE		0x00000002
#define AVIFILECAPS_ALLKEYFRAMES	0x00000010
#define AVIFILECAPS_NOCOMPRESSION	0x00000020

typedef struct _AVIFILEINFOW {
    DWORD               dwMaxBytesPerSec;
    DWORD               dwFlags;
    DWORD               dwCaps;
    DWORD               dwStreams;
    DWORD               dwSuggestedBufferSize;
    DWORD               dwWidth;
    DWORD               dwHeight;
    DWORD               dwScale;
    DWORD               dwRate;
    DWORD               dwLength;
    DWORD               dwEditCount;
    WCHAR               szFileType[64];
} AVIFILEINFOW, * LPAVIFILEINFOW, *PAVIFILEINFOW;
typedef struct _AVIFILEINFOA {
    DWORD               dwMaxBytesPerSec;
    DWORD               dwFlags;
    DWORD               dwCaps;
    DWORD               dwStreams;
    DWORD               dwSuggestedBufferSize;
    DWORD               dwWidth;
    DWORD               dwHeight;
    DWORD               dwScale;
    DWORD               dwRate;
    DWORD               dwLength;
    DWORD               dwEditCount;
    CHAR		szFileType[64];
} AVIFILEINFOA, * LPAVIFILEINFOA, *PAVIFILEINFOA;
DECL_WINELIB_TYPE_AW(AVIFILEINFO)
DECL_WINELIB_TYPE_AW(PAVIFILEINFO)
DECL_WINELIB_TYPE_AW(LPAVIFILEINFO)

/* AVICOMPRESSOPTIONS.dwFlags. determines presence of fields in below struct */
#define AVICOMPRESSF_INTERLEAVE	0x00000001
#define AVICOMPRESSF_DATARATE	0x00000002
#define AVICOMPRESSF_KEYFRAMES	0x00000004
#define AVICOMPRESSF_VALID	0x00000008

typedef struct {
    DWORD	fccType;		/* stream type, for consistency */
    DWORD	fccHandler;		/* compressor */
    DWORD	dwKeyFrameEvery;	/* keyframe rate */
    DWORD	dwQuality;		/* compress quality 0-10,000 */
    DWORD	dwBytesPerSecond;	/* bytes per second */
    DWORD	dwFlags;		/* flags... see below */
    LPVOID	lpFormat;		/* save format */
    DWORD	cbFormat;
    LPVOID	lpParms;		/* compressor options */
    DWORD	cbParms;
    DWORD	dwInterleaveEvery;	/* for non-video streams only */
} AVICOMPRESSOPTIONS, *LPAVICOMPRESSOPTIONS,*PAVICOMPRESSOPTIONS;

#define FIND_DIR        0x0000000FL     /* direction mask */
#define FIND_NEXT       0x00000001L     /* search forward */
#define FIND_PREV       0x00000004L     /* search backward */
#define FIND_FROM_START 0x00000008L     /* start at the logical beginning */

#define FIND_TYPE       0x000000F0L     /* type mask */
#define FIND_KEY        0x00000010L     /* find a key frame */
#define FIND_ANY        0x00000020L     /* find any (non-empty) sample */
#define FIND_FORMAT     0x00000040L     /* find a formatchange */

#define FIND_RET        0x0000F000L     /* return mask */
#define FIND_POS        0x00000000L     /* return logical position */
#define FIND_LENGTH     0x00001000L     /* return logical size */
#define FIND_OFFSET     0x00002000L     /* return physical position */
#define FIND_SIZE       0x00003000L     /* return physical size */
#define FIND_INDEX      0x00004000L     /* return physical index position */

#include "ole2.h"

#define DEFINE_AVIGUID(name, l, w1, w2) \
    DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

DEFINE_AVIGUID(IID_IAVIFile,            0x00020020, 0, 0);
DEFINE_AVIGUID(IID_IAVIStream,          0x00020021, 0, 0);
DEFINE_AVIGUID(IID_IAVIStreaming,       0x00020022, 0, 0);
DEFINE_AVIGUID(IID_IGetFrame,           0x00020023, 0, 0);
DEFINE_AVIGUID(IID_IAVIEditStream,      0x00020024, 0, 0);

DEFINE_AVIGUID(CLSID_AVIFile,           0x00020000, 0, 0);

/*****************************************************************************
 * IAVIStream interface
 */
#define INTERFACE IAVIStream
#define IAVIStream_METHODS						\
    STDMETHOD(Create)(THIS_ LPARAM lParam1, LPARAM lParam2) PURE; \
    STDMETHOD(Info)(THIS_ AVISTREAMINFOW *psi, LONG lSize) PURE; \
    STDMETHOD_(LONG,FindSample)(THIS_ LONG lPos, LONG lFlags) PURE; \
    STDMETHOD(ReadFormat)(THIS_ LONG lPos, LPVOID lpFormat, LONG *lpcbFormat) PURE; \
    STDMETHOD(SetFormat)(THIS_ LONG lPos, LPVOID lpFormat, LONG cbFormat) PURE; \
    STDMETHOD(Read)(THIS_ LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG *plBytes, LONG *plSamples) PURE; \
    STDMETHOD(Write)(THIS_ LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG *plSampWritten, LONG *plBytesWritten) PURE; \
    STDMETHOD(Delete)(THIS_ LONG lStart, LONG lSamples) PURE; \
    STDMETHOD(ReadData)(THIS_ DWORD fcc, LPVOID lpBuffer, LONG *lpcbBuffer) PURE; \
    STDMETHOD(WriteData)(THIS_ DWORD fcc, LPVOID lpBuffer, LONG cbBuffer) PURE; \
    STDMETHOD(SetInfo)(THIS_ AVISTREAMINFOW *plInfo, LONG cbInfo) PURE;
#define IAVIStream_IMETHODS	\
	IUnknown_IMETHODS	\
	IAVIStream_METHODS
ICOM_DEFINE(IAVIStream, IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IAVIStream_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IAVIStream_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IAVIStream_Release(p)            (p)->lpVtbl->Release(p)
/*** IAVIStream methods ***/
#define IAVIStream_Create(p,a,b)          (p)->lpVtbl->Create(p,a,b)
#define IAVIStream_Info(p,a,b)            (p)->lpVtbl->Info(p,a,b)
#define IAVIStream_FindSample(p,a,b)      (p)->lpVtbl->FindSample(p,a,b)
#define IAVIStream_ReadFormat(p,a,b,c)    (p)->lpVtbl->ReadFormat(p,a,b,c)
#define IAVIStream_SetFormat(p,a,b,c)     (p)->lpVtbl->SetFormat(p,a,b,c)
#define IAVIStream_Read(p,a,b,c,d,e,f)    (p)->lpVtbl->Read(p,a,b,c,d,e,f)
#define IAVIStream_Write(p,a,b,c,d,e,f,g) (p)->lpVtbl->Write(p,a,b,c,d,e,f,g)
#define IAVIStream_Delete(p,a,b)          (p)->lpVtbl->Delete(p,a,b)
#define IAVIStream_ReadData(p,a,b,c)      (p)->lpVtbl->ReadData(p,a,b,c)
#define IAVIStream_WriteData(p,a,b,c)     (p)->lpVtbl->WriteData(p,a,b,c)
#define IAVIStream_SetInfo(p,a,b)         (p)->lpVtbl->SetInfo(p,a,b)
#endif

ULONG WINAPI AVIStreamAddRef(PAVISTREAM iface);
ULONG WINAPI AVIStreamRelease(PAVISTREAM iface);
HRESULT WINAPI AVIStreamCreate(PAVISTREAM*,LONG,LONG,CLSID*);
HRESULT WINAPI AVIStreamInfoA(PAVISTREAM iface,AVISTREAMINFOA *asi,LONG size);
HRESULT WINAPI AVIStreamInfoW(PAVISTREAM iface,AVISTREAMINFOW *asi,LONG size);
#define AVIStreamInfo WINELIB_NAME_AW(AVIStreamInfo)
HRESULT WINAPI AVIStreamFindSample(PAVISTREAM pstream, LONG pos, DWORD flags);
HRESULT WINAPI AVIStreamReadFormat(PAVISTREAM iface,LONG pos,LPVOID format,LONG *formatsize);
HRESULT WINAPI AVIStreamSetFormat(PAVISTREAM iface,LONG pos,LPVOID format,LONG formatsize);
HRESULT WINAPI AVIStreamRead(PAVISTREAM iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread);
HRESULT WINAPI AVIStreamWrite(PAVISTREAM iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten);
HRESULT WINAPI AVIStreamReadData(PAVISTREAM iface,DWORD fcc,LPVOID lp,LONG *lpread);
HRESULT WINAPI AVIStreamWriteData(PAVISTREAM iface,DWORD fcc,LPVOID lp,LONG size);

PGETFRAME WINAPI AVIStreamGetFrameOpen(PAVISTREAM pavi,LPBITMAPINFOHEADER lpbiWanted);
LPVOID  WINAPI AVIStreamGetFrame(PGETFRAME pg,LONG pos);
HRESULT WINAPI AVIStreamGetFrameClose(PGETFRAME pg);

HRESULT WINAPI AVIMakeCompressedStream(PAVISTREAM*ppsCompressed,PAVISTREAM ppsSource,AVICOMPRESSOPTIONS *lpOptions,CLSID*pclsidHandler);

HRESULT WINAPI AVIStreamOpenFromFileA(PAVISTREAM *ppavi, LPCSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, CLSID *pclsidHandler);
HRESULT WINAPI AVIStreamOpenFromFileW(PAVISTREAM *ppavi, LPCWSTR szFile,
				      DWORD fccType, LONG lParam,
				      UINT mode, CLSID *pclsidHandler);
#define AVIStreamOpenFromFile WINELIB_NAME_AW(AVIStreamOpenFromFile)

HRESULT WINAPI AVIBuildFilterA(LPSTR szFilter, LONG cbFilter, BOOL fSaving);
HRESULT WINAPI AVIBuildFilterW(LPWSTR szFilter, LONG cbFilter, BOOL fSaving);
#define AVIBuildFilter WINELIB_NAME_AW(AVIBuildFilter)

BOOL WINAPI AVISaveOptions(HWND hWnd,UINT uFlags,INT nStream,
			   PAVISTREAM *ppavi,LPAVICOMPRESSOPTIONS *ppOptions);
HRESULT WINAPI AVISaveOptionsFree(INT nStreams,LPAVICOMPRESSOPTIONS*ppOptions);

LONG WINAPI AVIStreamStart(PAVISTREAM iface);
LONG WINAPI AVIStreamLength(PAVISTREAM iface);
LONG WINAPI AVIStreamSampleToTime(PAVISTREAM pstream, LONG lSample);
LONG WINAPI AVIStreamTimeToSample(PAVISTREAM pstream, LONG lTime);

#define AVIFileClose(pavi) \
    AVIFileRelease(pavi)
#define AVIStreamClose(pavi) \
    AVIStreamRelease(pavi);
#define AVIStreamEnd(pavi) \
    (AVIStreamStart(pavi) + AVIStreamLength(pavi))
#define AVIStreamEndTime(pavi) \
    AVIStreamSampleToTime(pavi, AVIStreamEnd(pavi))
#define AVIStreamFormatSize(pavi, lPos, plSize) \
    AVIStreamReadFormat(pavi, lPos, NULL, plSize)
#define AVIStreamLengthTime(pavi) \
    AVIStreamSampleToTime(pavi, AVIStreamLength(pavi))
#define AVIStreamSampleSize(pavi,pos,psize) \
    AVIStreamRead(pavi,pos,1,NULL,0,psize,NULL)
#define AVIStreamSampleToSample(pavi1, pavi2, samp2) \
    AVIStreamTimeToSample(pavi1, AVIStreamSampleToTime(pavi2, samp2))
#define AVIStreamStartTime(pavi) \
    AVIStreamSampleToTime(pavi, AVIStreamStart(pavi))

/*****************************************************************************
 * IAVIFile interface
 */
/* In Win32 this interface uses UNICODE only */
#define INTERFACE IAVIFile
#define IAVIFile_METHODS 						\
    STDMETHOD(Info)(THIS_ AVIFILEINFOW *pfi, LONG lSize) PURE; \
    STDMETHOD(GetStream)(THIS_ PAVISTREAM *ppStream, DWORD fccType, LONG lParam) PURE; \
    STDMETHOD(CreateStream)(THIS_ PAVISTREAM *ppStream, AVISTREAMINFOW *psi) PURE; \
    STDMETHOD(WriteData)(THIS_ DWORD fcc, LPVOID lpBuffer, LONG cbBuffer) PURE; \
    STDMETHOD(ReadData)(THIS_ DWORD fcc, LPVOID lpBuffer, LONG *lpcbBuffer) PURE; \
    STDMETHOD(EndRecord)(THIS) PURE; \
    STDMETHOD(DeleteStream)(THIS_ DWORD fccType, LONG lParam) PURE;
#define IAVIFile_IMETHODS 	\
	IUnknown_IMETHODS	\
	IAVIFile_METHODS
ICOM_DEFINE(IAVIFile,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IAVIFile_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IAVIFile_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IAVIFile_Release(p)            (p)->lpVtbl->Release(p)
/*** IAVIFile methods ***/
#define IAVIFile_Info(p,a,b)         (p)->lpVtbl->Info(p,a,b)
#define IAVIFile_GetStream(p,a,b,c)  (p)->lpVtbl->GetStream(p,a,b,c)
#define IAVIFile_CreateStream(p,a,b) (p)->lpVtbl->CreateStream(p,a,b)
#define IAVIFile_WriteData(p,a,b,c)  (p)->lpVtbl->WriteData(p,a,b,c)
#define IAVIFile_ReadData(p,a,b,c)   (p)->lpVtbl->ReadData(p,a,b,c)
#define IAVIFile_EndRecord(p)        (p)->lpVtbl->EndRecord(p)
#define IAVIFile_DeleteStream(p,a,b) (p)->lpVtbl->DeleteStream(p,a,b)
#endif

void WINAPI AVIFileInit(void);
void WINAPI AVIFileExit(void);

HRESULT WINAPI AVIFileOpenA(PAVIFILE* ppfile,LPCSTR szFile,UINT uMode,LPCLSID lpHandler);
HRESULT WINAPI AVIFileOpenW(PAVIFILE* ppfile,LPCWSTR szFile,UINT uMode,LPCLSID lpHandler);
#define AVIFileOpen WINELIB_NAME_AW(AVIFileOpen)

ULONG   WINAPI AVIFileAddRef(PAVIFILE pfile);
ULONG   WINAPI AVIFileRelease(PAVIFILE pfile);
HRESULT WINAPI AVIFileInfoA(PAVIFILE pfile,PAVIFILEINFOA pfi,LONG lSize);
HRESULT WINAPI AVIFileInfoW(PAVIFILE pfile,PAVIFILEINFOW pfi,LONG lSize);
#define AVIFileInfo WINELIB_NAME_AW(AVIFileInfo)
HRESULT WINAPI AVIFileGetStream(PAVIFILE pfile,PAVISTREAM* avis,DWORD fccType,LONG lParam);
HRESULT WINAPI AVIFileCreateStreamA(PAVIFILE pfile,PAVISTREAM* ppavi,AVISTREAMINFOA* psi);
HRESULT WINAPI AVIFileCreateStreamW(PAVIFILE pfile,PAVISTREAM* ppavi,AVISTREAMINFOW* psi);
#define AVIFileCreateStream WINELIB_NAME_AW(AVIFileCreateStream)
HRESULT WINAPI AVIFileWriteData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LONG size);
HRESULT WINAPI AVIFileReadData(PAVIFILE pfile,DWORD fcc,LPVOID lp,LPLONG size);
HRESULT WINAPI AVIFileEndRecord(PAVIFILE pfile);

/*****************************************************************************
 * IGetFrame interface
 */
#define INTERFACE IGetFrame
#define IGetFrame_METHODS					\
    STDMETHOD_(LPVOID,GetFrame)(THIS_ LONG lPos) PURE; \
    STDMETHOD(Begin)(THIS_ LONG lStart, LONG lEnd, LONG lRate) PURE; \
    STDMETHOD(End)(THIS) PURE; \
    STDMETHOD(SetFormat)(THIS_ LPBITMAPINFOHEADER lpbi, LPVOID lpBits, INT x, INT y, INT dx, INT dy) PURE;
#define IGetFrame_IMETHODS	\
	IUnknown_IMETHODS	\
	IGetFrame_METHODS
ICOM_DEFINE(IGetFrame,IUnknown)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IGetFrame_QueryInterface(p,a,b) (p)->lpVtbl->QueryInterface(p,a,b)
#define IGetFrame_AddRef(p)             (p)->lpVtbl->AddRef(p)
#define IGetFrame_Release(p)            (p)->lpVtbl->Release(p)
/*** IGetFrame methods ***/
#define IGetFrame_GetFrame(p,a)            (p)->lpVtbl->GetFrame(p,a)
#define IGetFrame_Begin(p,a,b,c)           (p)->lpVtbl->Begin(p,a,b,c)
#define IGetFrame_End(p)                   (p)->lpVtbl->End(p)
#define IGetFrame_SetFormat(p,a,b,c,d,e,f) (p)->lpVtbl->SetFormat(p,a,b,c,d,e,f)
#endif

#define AVIERR_OK		0
#define MAKE_AVIERR(error)	MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x4000+error)

#define AVIERR_UNSUPPORTED	MAKE_AVIERR(101)
#define AVIERR_BADFORMAT	MAKE_AVIERR(102)
#define AVIERR_MEMORY		MAKE_AVIERR(103)
#define AVIERR_INTERNAL		MAKE_AVIERR(104)
#define AVIERR_BADFLAGS		MAKE_AVIERR(105)
#define AVIERR_BADPARAM		MAKE_AVIERR(106)
#define AVIERR_BADSIZE		MAKE_AVIERR(107)
#define AVIERR_BADHANDLE	MAKE_AVIERR(108)
#define AVIERR_FILEREAD		MAKE_AVIERR(109)
#define AVIERR_FILEWRITE	MAKE_AVIERR(110)
#define AVIERR_FILEOPEN		MAKE_AVIERR(111)
#define AVIERR_COMPRESSOR	MAKE_AVIERR(112)
#define AVIERR_NOCOMPRESSOR	MAKE_AVIERR(113)
#define AVIERR_READONLY		MAKE_AVIERR(114)
#define AVIERR_NODATA		MAKE_AVIERR(115)
#define AVIERR_BUFFERTOOSMALL	MAKE_AVIERR(116)
#define AVIERR_CANTCOMPRESS	MAKE_AVIERR(117)
#define AVIERR_USERABORT	MAKE_AVIERR(198)
#define AVIERR_ERROR		MAKE_AVIERR(199)

HWND        VFWAPIV MCIWndCreateA(HWND hwndParent, HINSTANCE hInstance, DWORD dwStyle, LPCSTR szFile);
HWND        VFWAPIV MCIWndCreateW(HWND hwndParent, HINSTANCE hInstance, DWORD dwStyle, LPCWSTR szFile);
#define     MCIWndCreate WINELIB_NAME_AW(MCIWndCreate)

#define MCIWNDOPENF_NEW			0x0001

#define MCIWNDF_NOAUTOSIZEWINDOW	0x0001
#define MCIWNDF_NOPLAYBAR		0x0002
#define MCIWNDF_NOAUTOSIZEMOVIE		0x0004
#define MCIWNDF_NOMENU			0x0008
#define MCIWNDF_SHOWNAME		0x0010
#define MCIWNDF_SHOWPOS			0x0020
#define MCIWNDF_SHOWMODE		0x0040
#define MCIWNDF_SHOWALL			0x0070

#define MCIWNDF_NOTIFYMODE		0x0100
#define MCIWNDF_NOTIFYPOS		0x0200
#define MCIWNDF_NOTIFYSIZE		0x0400
#define MCIWNDF_NOTIFYERROR		0x1000
#define MCIWNDF_NOTIFYALL		0x1F00

#define MCIWNDF_NOTIFYANSI		0x0080

#define MCIWNDF_NOTIFYMEDIAA		0x0880
#define MCIWNDF_NOTIFYMEDIAW		0x0800
#define MCIWNDF_NOTIFYMEDIA WINELIB_NAME_AW(MCIWNDF_NOTIFYMEDIA)

#define MCIWNDF_RECORD			0x2000
#define MCIWNDF_NOERRORDLG		0x4000
#define MCIWNDF_NOOPEN			0x8000

#ifdef __cplusplus
#define MCIWndSM ::SendMessage
#else
#define MCIWndSM SendMessage
#endif

#define MCIWndCanPlay(hWnd)         (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_PLAY,0,0)
#define MCIWndCanRecord(hWnd)       (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_RECORD,0,0)
#define MCIWndCanSave(hWnd)         (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_SAVE,0,0)
#define MCIWndCanWindow(hWnd)       (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_WINDOW,0,0)
#define MCIWndCanEject(hWnd)        (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_EJECT,0,0)
#define MCIWndCanConfig(hWnd)       (BOOL)MCIWndSM(hWnd,MCIWNDM_CAN_CONFIG,0,0)
#define MCIWndPaletteKick(hWnd)     (BOOL)MCIWndSM(hWnd,MCIWNDM_PALETTEKICK,0,0)

#define MCIWndSave(hWnd,szFile)	    (LONG)MCIWndSM(hWnd,MCI_SAVE,0,(LPARAM)(LPVOID)(szFile))
#define MCIWndSaveDialog(hWnd)      MCIWndSave(hWnd,-1)

#define MCIWndNew(hWnd,lp)          (LONG)MCIWndSM(hWnd,MCIWNDM_NEW,0,(LPARAM)(LPVOID)(lp))

#define MCIWndRecord(hWnd)          (LONG)MCIWndSM(hWnd,MCI_RECORD,0,0)
#define MCIWndOpen(hWnd,sz,f)       (LONG)MCIWndSM(hWnd,MCIWNDM_OPEN,(WPARAM)(UINT)(f),(LPARAM)(LPVOID)(sz))
#define MCIWndOpenDialog(hWnd)      MCIWndOpen(hWnd,-1,0)
#define MCIWndClose(hWnd)           (LONG)MCIWndSM(hWnd,MCI_CLOSE,0,0)
#define MCIWndPlay(hWnd)            (LONG)MCIWndSM(hWnd,MCI_PLAY,0,0)
#define MCIWndStop(hWnd)            (LONG)MCIWndSM(hWnd,MCI_STOP,0,0)
#define MCIWndPause(hWnd)           (LONG)MCIWndSM(hWnd,MCI_PAUSE,0,0)
#define MCIWndResume(hWnd)          (LONG)MCIWndSM(hWnd,MCI_RESUME,0,0)
#define MCIWndSeek(hWnd,lPos)       (LONG)MCIWndSM(hWnd,MCI_SEEK,0,(LPARAM)(LONG)(lPos))
#define MCIWndEject(hWnd)           (LONG)MCIWndSM(hWnd,MCIWNDM_EJECT,0,0)

#define MCIWndHome(hWnd)            MCIWndSeek(hWnd,MCIWND_START)
#define MCIWndEnd(hWnd)             MCIWndSeek(hWnd,MCIWND_END)

#define MCIWndGetSource(hWnd,prc)   (LONG)MCIWndSM(hWnd,MCIWNDM_GET_SOURCE,0,(LPARAM)(LPRECT)(prc))
#define MCIWndPutSource(hWnd,prc)   (LONG)MCIWndSM(hWnd,MCIWNDM_PUT_SOURCE,0,(LPARAM)(LPRECT)(prc))

#define MCIWndGetDest(hWnd,prc)     (LONG)MCIWndSM(hWnd,MCIWNDM_GET_DEST,0,(LPARAM)(LPRECT)(prc))
#define MCIWndPutDest(hWnd,prc)     (LONG)MCIWndSM(hWnd,MCIWNDM_PUT_DEST,0,(LPARAM)(LPRECT)(prc))

#define MCIWndPlayReverse(hWnd)     (LONG)MCIWndSM(hWnd,MCIWNDM_PLAYREVERSE,0,0)
#define MCIWndPlayFrom(hWnd,lPos)   (LONG)MCIWndSM(hWnd,MCIWNDM_PLAYFROM,0,(LPARAM)(LONG)(lPos))
#define MCIWndPlayTo(hWnd,lPos)     (LONG)MCIWndSM(hWnd,MCIWNDM_PLAYTO,  0,(LPARAM)(LONG)(lPos))
#define MCIWndPlayFromTo(hWnd,lStart,lEnd) (MCIWndSeek(hWnd,lStart),MCIWndPlayTo(hWnd,lEnd))

#define MCIWndGetDeviceID(hWnd)     (UINT)MCIWndSM(hWnd,MCIWNDM_GETDEVICEID,0,0)
#define MCIWndGetAlias(hWnd)        (UINT)MCIWndSM(hWnd,MCIWNDM_GETALIAS,0,0)
#define MCIWndGetMode(hWnd,lp,len)  (LONG)MCIWndSM(hWnd,MCIWNDM_GETMODE,(WPARAM)(UINT)(len),(LPARAM)(LPTSTR)(lp))
#define MCIWndGetPosition(hWnd)     (LONG)MCIWndSM(hWnd,MCIWNDM_GETPOSITION,0,0)
#define MCIWndGetPositionString(hWnd,lp,len) (LONG)MCIWndSM(hWnd,MCIWNDM_GETPOSITION,(WPARAM)(UINT)(len),(LPARAM)(LPTSTR)(lp))
#define MCIWndGetStart(hWnd)        (LONG)MCIWndSM(hWnd,MCIWNDM_GETSTART,0,0)
#define MCIWndGetLength(hWnd)       (LONG)MCIWndSM(hWnd,MCIWNDM_GETLENGTH,0,0)
#define MCIWndGetEnd(hWnd)          (LONG)MCIWndSM(hWnd,MCIWNDM_GETEND,0,0)

#define MCIWndStep(hWnd,n)          (LONG)MCIWndSM(hWnd,MCI_STEP,0,(LPARAM)(long)(n))

#define MCIWndDestroy(hWnd)         (VOID)MCIWndSM(hWnd,WM_CLOSE,0,0)
#define MCIWndSetZoom(hWnd,iZoom)   (VOID)MCIWndSM(hWnd,MCIWNDM_SETZOOM,0,(LPARAM)(UINT)(iZoom))
#define MCIWndGetZoom(hWnd)         (UINT)MCIWndSM(hWnd,MCIWNDM_GETZOOM,0,0)
#define MCIWndSetVolume(hWnd,iVol)  (LONG)MCIWndSM(hWnd,MCIWNDM_SETVOLUME,0,(LPARAM)(UINT)(iVol))
#define MCIWndGetVolume(hWnd)       (LONG)MCIWndSM(hWnd,MCIWNDM_GETVOLUME,0,0)
#define MCIWndSetSpeed(hWnd,iSpeed) (LONG)MCIWndSM(hWnd,MCIWNDM_SETSPEED,0,(LPARAM)(UINT)(iSpeed))
#define MCIWndGetSpeed(hWnd)        (LONG)MCIWndSM(hWnd,MCIWNDM_GETSPEED,0,0)
#define MCIWndSetTimeFormat(hWnd,lp) (LONG)MCIWndSM(hWnd,MCIWNDM_SETTIMEFORMAT,0,(LPARAM)(LPTSTR)(lp))
#define MCIWndGetTimeFormat(hWnd,lp,len) (LONG)MCIWndSM(hWnd,MCIWNDM_GETTIMEFORMAT,(WPARAM)(UINT)(len),(LPARAM)(LPTSTR)(lp))
#define MCIWndValidateMedia(hWnd)   (VOID)MCIWndSM(hWnd,MCIWNDM_VALIDATEMEDIA,0,0)

#define MCIWndSetRepeat(hWnd,f)     (void)MCIWndSM(hWnd,MCIWNDM_SETREPEAT,0,(LPARAM)(BOOL)(f))
#define MCIWndGetRepeat(hWnd)       (BOOL)MCIWndSM(hWnd,MCIWNDM_GETREPEAT,0,0)

#define MCIWndUseFrames(hWnd)       MCIWndSetTimeFormat(hWnd,TEXT("frames"))
#define MCIWndUseTime(hWnd)         MCIWndSetTimeFormat(hWnd,TEXT("ms"))

#define MCIWndSetActiveTimer(hWnd,active)				\
	(VOID)MCIWndSM(hWnd,MCIWNDM_SETACTIVETIMER,			\
	(WPARAM)(UINT)(active),0L)
#define MCIWndSetInactiveTimer(hWnd,inactive)				\
	(VOID)MCIWndSM(hWnd,MCIWNDM_SETINACTIVETIMER,			\
	(WPARAM)(UINT)(inactive),0L)
#define MCIWndSetTimers(hWnd,active,inactive)				\
	    (VOID)MCIWndSM(hWnd,MCIWNDM_SETTIMERS,(WPARAM)(UINT)(active),\
	    (LPARAM)(UINT)(inactive))
#define MCIWndGetActiveTimer(hWnd)					\
	(UINT)MCIWndSM(hWnd,MCIWNDM_GETACTIVETIMER,0,0L);
#define MCIWndGetInactiveTimer(hWnd)					\
	(UINT)MCIWndSM(hWnd,MCIWNDM_GETINACTIVETIMER,0,0L);

#define MCIWndRealize(hWnd,fBkgnd) (LONG)MCIWndSM(hWnd,MCIWNDM_REALIZE,(WPARAM)(BOOL)(fBkgnd),0)

#define MCIWndSendString(hWnd,sz)  (LONG)MCIWndSM(hWnd,MCIWNDM_SENDSTRING,0,(LPARAM)(LPTSTR)(sz))
#define MCIWndReturnString(hWnd,lp,len)  (LONG)MCIWndSM(hWnd,MCIWNDM_RETURNSTRING,(WPARAM)(UINT)(len),(LPARAM)(LPVOID)(lp))
#define MCIWndGetError(hWnd,lp,len) (LONG)MCIWndSM(hWnd,MCIWNDM_GETERROR,(WPARAM)(UINT)(len),(LPARAM)(LPVOID)(lp))

#define MCIWndGetPalette(hWnd)      (HPALETTE)MCIWndSM(hWnd,MCIWNDM_GETPALETTE,0,0)
#define MCIWndSetPalette(hWnd,hpal) (LONG)MCIWndSM(hWnd,MCIWNDM_SETPALETTE,(WPARAM)(HPALETTE)(hpal),0)

#define MCIWndGetFileName(hWnd,lp,len) (LONG)MCIWndSM(hWnd,MCIWNDM_GETFILENAME,(WPARAM)(UINT)(len),(LPARAM)(LPVOID)(lp))
#define MCIWndGetDevice(hWnd,lp,len)   (LONG)MCIWndSM(hWnd,MCIWNDM_GETDEVICE,(WPARAM)(UINT)(len),(LPARAM)(LPVOID)(lp))

#define MCIWndGetStyles(hWnd) (UINT)MCIWndSM(hWnd,MCIWNDM_GETSTYLES,0,0L)
#define MCIWndChangeStyles(hWnd,mask,value) (LONG)MCIWndSM(hWnd,MCIWNDM_CHANGESTYLES,(WPARAM)(UINT)(mask),(LPARAM)(LONG)(value))

#define MCIWndOpenInterface(hWnd,pUnk)  (LONG)MCIWndSM(hWnd,MCIWNDM_OPENINTERFACE,0,(LPARAM)(LPUNKNOWN)(pUnk))

#define MCIWndSetOwner(hWnd,hWndP)  (LONG)MCIWndSM(hWnd,MCIWNDM_SETOWNER,(WPARAM)(hWndP),0)

#define MCIWNDM_GETDEVICEID	(WM_USER + 100)
#define MCIWNDM_GETSTART	(WM_USER + 103)
#define MCIWNDM_GETLENGTH	(WM_USER + 104)
#define MCIWNDM_GETEND		(WM_USER + 105)
#define MCIWNDM_EJECT		(WM_USER + 107)
#define MCIWNDM_SETZOOM		(WM_USER + 108)
#define MCIWNDM_GETZOOM         (WM_USER + 109)
#define MCIWNDM_SETVOLUME	(WM_USER + 110)
#define MCIWNDM_GETVOLUME	(WM_USER + 111)
#define MCIWNDM_SETSPEED	(WM_USER + 112)
#define MCIWNDM_GETSPEED	(WM_USER + 113)
#define MCIWNDM_SETREPEAT	(WM_USER + 114)
#define MCIWNDM_GETREPEAT	(WM_USER + 115)
#define MCIWNDM_REALIZE         (WM_USER + 118)
#define MCIWNDM_VALIDATEMEDIA   (WM_USER + 121)
#define MCIWNDM_PLAYFROM	(WM_USER + 122)
#define MCIWNDM_PLAYTO          (WM_USER + 123)
#define MCIWNDM_GETPALETTE      (WM_USER + 126)
#define MCIWNDM_SETPALETTE      (WM_USER + 127)
#define MCIWNDM_SETTIMERS	(WM_USER + 129)
#define MCIWNDM_SETACTIVETIMER	(WM_USER + 130)
#define MCIWNDM_SETINACTIVETIMER (WM_USER + 131)
#define MCIWNDM_GETACTIVETIMER	(WM_USER + 132)
#define MCIWNDM_GETINACTIVETIMER (WM_USER + 133)
#define MCIWNDM_CHANGESTYLES	(WM_USER + 135)
#define MCIWNDM_GETSTYLES	(WM_USER + 136)
#define MCIWNDM_GETALIAS	(WM_USER + 137)
#define MCIWNDM_PLAYREVERSE	(WM_USER + 139)
#define MCIWNDM_GET_SOURCE      (WM_USER + 140)
#define MCIWNDM_PUT_SOURCE      (WM_USER + 141)
#define MCIWNDM_GET_DEST        (WM_USER + 142)
#define MCIWNDM_PUT_DEST        (WM_USER + 143)
#define MCIWNDM_CAN_PLAY        (WM_USER + 144)
#define MCIWNDM_CAN_WINDOW      (WM_USER + 145)
#define MCIWNDM_CAN_RECORD      (WM_USER + 146)
#define MCIWNDM_CAN_SAVE        (WM_USER + 147)
#define MCIWNDM_CAN_EJECT       (WM_USER + 148)
#define MCIWNDM_CAN_CONFIG      (WM_USER + 149)
#define MCIWNDM_PALETTEKICK     (WM_USER + 150)
#define MCIWNDM_OPENINTERFACE	(WM_USER + 151)
#define MCIWNDM_SETOWNER	(WM_USER + 152)

#define MCIWNDM_SENDSTRINGA	(WM_USER + 101)
#define MCIWNDM_GETPOSITIONA	(WM_USER + 102)
#define MCIWNDM_GETMODEA	(WM_USER + 106)
#define MCIWNDM_SETTIMEFORMATA  (WM_USER + 119)
#define MCIWNDM_GETTIMEFORMATA  (WM_USER + 120)
#define MCIWNDM_GETFILENAMEA    (WM_USER + 124)
#define MCIWNDM_GETDEVICEA      (WM_USER + 125)
#define MCIWNDM_GETERRORA       (WM_USER + 128)
#define MCIWNDM_NEWA		(WM_USER + 134)
#define MCIWNDM_RETURNSTRINGA	(WM_USER + 138)
#define MCIWNDM_OPENA		(WM_USER + 153)

#define MCIWNDM_SENDSTRINGW	(WM_USER + 201)
#define MCIWNDM_GETPOSITIONW	(WM_USER + 202)
#define MCIWNDM_GETMODEW	(WM_USER + 206)
#define MCIWNDM_SETTIMEFORMATW  (WM_USER + 219)
#define MCIWNDM_GETTIMEFORMATW  (WM_USER + 220)
#define MCIWNDM_GETFILENAMEW    (WM_USER + 224)
#define MCIWNDM_GETDEVICEW      (WM_USER + 225)
#define MCIWNDM_GETERRORW       (WM_USER + 228)
#define MCIWNDM_NEWW		(WM_USER + 234)
#define MCIWNDM_RETURNSTRINGW	(WM_USER + 238)
#define MCIWNDM_OPENW		(WM_USER + 252)

#define MCIWNDM_SENDSTRING	WINELIB_NAME_AW(MCIWNDM_SENDSTRING)
#define MCIWNDM_GETPOSITION	WINELIB_NAME_AW(MCIWNDM_GETPOSITION)
#define MCIWNDM_GETMODE		WINELIB_NAME_AW(MCIWNDM_GETMODE)
#define MCIWNDM_SETTIMEFORMAT	WINELIB_NAME_AW(MCIWNDM_SETTIMEFORMAT)
#define MCIWNDM_GETTIMEFORMAT	WINELIB_NAME_AW(MCIWNDM_GETTIMEFORMAT)
#define MCIWNDM_GETFILENAME	WINELIB_NAME_AW(MCIWNDM_GETFILENAME)
#define MCIWNDM_GETDEVICE	WINELIB_NAME_AW(MCIWNDM_GETDEVICE)
#define MCIWNDM_GETERROR	WINELIB_NAME_AW(MCIWNDM_GETERROR)
#define MCIWNDM_NEW		WINELIB_NAME_AW(MCIWNDM_NEW)
#define MCIWNDM_RETURNSTRING	WINELIB_NAME_AW(MCIWNDM_RETURNSTRING)
#define MCIWNDM_OPEN		WINELIB_NAME_AW(MCIWNDM_OPEN)

#define MCIWNDM_NOTIFYMODE      (WM_USER + 200)
#define MCIWNDM_NOTIFYPOS	(WM_USER + 201)
#define MCIWNDM_NOTIFYSIZE	(WM_USER + 202)
#define MCIWNDM_NOTIFYMEDIA     (WM_USER + 203)
#define MCIWNDM_NOTIFYERROR     (WM_USER + 205)

#define MCIWND_START                -1
#define MCIWND_END                  -2

/********************************************
 * DrawDib declarations
 */

HDRAWDIB VFWAPI DrawDibOpen( void );
UINT VFWAPI DrawDibRealize(HDRAWDIB hdd, HDC hdc, BOOL fBackground);

BOOL VFWAPI DrawDibBegin(HDRAWDIB hdd, HDC hdc, INT dxDst, INT dyDst,
			 LPBITMAPINFOHEADER lpbi, INT dxSrc, INT dySrc, UINT wFlags);

BOOL VFWAPI DrawDibDraw(HDRAWDIB hdd, HDC hdc, INT xDst, INT yDst, INT dxDst, INT dyDst,
			LPBITMAPINFOHEADER lpbi, LPVOID lpBits,
			INT xSrc, INT ySrc, INT dxSrc, INT dySrc, UINT wFlags);

/* DrawDibDraw flags */

#define DDF_UPDATE			0x0002
#define DDF_SAME_HDC			0x0004
#define DDF_SAME_DRAW			0x0008
#define DDF_DONTDRAW			0x0010
#define DDF_ANIMATE			0x0020
#define DDF_BUFFER			0x0040
#define DDF_JUSTDRAWIT			0x0080
#define DDF_FULLSCREEN			0x0100
#define DDF_BACKGROUNDPAL		0x0200
#define DDF_NOTKEYFRAME			0x0400
#define DDF_HURRYUP			0x0800
#define DDF_HALFTONE			0x1000

#define DDF_PREROLL         		DDF_DONTDRAW
#define DDF_SAME_DIB        		DDF_SAME_DRAW
#define DDF_SAME_SIZE       		DDF_SAME_DRAW

BOOL VFWAPI DrawDibSetPalette(HDRAWDIB hdd, HPALETTE hpal);
HPALETTE VFWAPI DrawDibGetPalette(HDRAWDIB hdd);
BOOL VFWAPI DrawDibChangePalette(HDRAWDIB hdd, int iStart, int iLen, LPPALETTEENTRY lppe);
LPVOID VFWAPI DrawDibGetBuffer(HDRAWDIB hdd, LPBITMAPINFOHEADER lpbi, DWORD dwSize, DWORD dwFlags);

BOOL VFWAPI DrawDibStart(HDRAWDIB hdd, DWORD rate);
BOOL VFWAPI DrawDibStop(HDRAWDIB hdd);
#define DrawDibUpdate(hdd, hdc, x, y) \
        DrawDibDraw(hdd, hdc, x, y, 0, 0, NULL, NULL, 0, 0, 0, 0, DDF_UPDATE)

BOOL VFWAPI DrawDibEnd(HDRAWDIB hdd);
BOOL VFWAPI DrawDibClose(HDRAWDIB hdd);

/* display profiling */
#define PD_CAN_DRAW_DIB         0x0001
#define PD_CAN_STRETCHDIB       0x0002
#define PD_STRETCHDIB_1_1_OK    0x0004
#define PD_STRETCHDIB_1_2_OK    0x0008
#define PD_STRETCHDIB_1_N_OK    0x0010

DWORD VFWAPI DrawDibProfileDisplay(LPBITMAPINFOHEADER lpbi);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __WINE_VFW_H */
