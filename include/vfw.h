#ifndef __WINE_VFW_H
#define __WINE_VFW_H

#include <wintypes.h>
#include <driver.h>

#define VFWAPI	WINAPI
#define VFWAPIV	WINAPIV

DWORD VFWAPI VideoForWindowsVersion(void);

#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

#define ICTYPE_VIDEO	mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO	mmioFOURCC('a', 'u', 'd', 'c')


/* Installable Compressor M? */

/* HIC32 struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
	DWORD		magic;		/* 00: 'Smag' */
	HANDLE32	curthread;	/* 04: */
	DWORD		type;		/* 08: */
	DWORD		handler;	/* 0C: */
	HDRVR32		hdrv;		/* 10: */
	DWORD		private;	/* 14: private data passed to drv */
	FARPROC32	driverproc;	/* 18: */
	DWORD		x1;		/* 1c: name? */
	WORD		x2;		/* 20: */
	DWORD		x3;		/* 22: */
					/* 26: */
} WINE_HIC;

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

/* Values for dwFlags of ICOpen() */
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
} ICINFO32;


/* Flags for the <dwFlags> field of the <ICINFO> structure. */
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

#define ICAbout(hic, hwnd) \
	ICSendMessage(hic,ICM_ABOUT,(DWORD)(UINT)(hwnd),0)

/* ICM_CONFIGURE */
#define ICMF_CONFIGURE_QUERY	0x00000001
#define ICQueryConfigure(hic) \
	(ICSendMessage(hic,ICM_CONFIGURE,(DWORD)-1,ICMF_CONFIGURE_QUERY)==ICERR_OK)

#define ICConfigure(hic,hwnd) \
	ICSendMessage(hic,ICM_CONFIGURE,(DWORD)(UINT)(hwnd),0)

/* Decompression stuff */
#define ICDECOMPRESS_HURRYUP		0x80000000	/* don't draw just buffer (hurry up!) */
#define ICDECOMPRESS_UPDATE		0x40000000	/* don't draw just update screen */
#define ICDECOMPRESS_PREROL		0x20000000	/* this frame is before real start */
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
    INT32		xDst;       /* destination rectangle */
    INT32		yDst;
    INT32		dxDst;
    INT32		dyDst;

    INT32		xSrc;       /* source rectangle */
    INT32		ySrc;
    INT32		dxSrc;
    INT32		dySrc;
} ICDECOMPRESSEX;

#define ICDRAW_QUERY        0x00000001L   // test for support
#define ICDRAW_FULLSCREEN   0x00000002L   // draw to full screen
#define ICDRAW_HDC          0x00000004L   // draw to a HDC/HWND


BOOL32	VFWAPI	ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO32 * lpicinfo);
LRESULT	VFWAPI	ICGetInfo32(HIC32 hic,ICINFO32 *picinfo, DWORD cb);
HIC32	VFWAPI	ICOpen32(DWORD fccType, DWORD fccHandler, UINT32 wMode);
LRESULT	VFWAPI	ICSendMessage32(HIC32 hic, UINT32 msg, DWORD dw1, DWORD dw2);
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
        DWORD			dwScale);

#endif
