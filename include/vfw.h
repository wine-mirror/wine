#ifndef __WINE_VFW_H
#define __WINE_VFW_H

#include "windef.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "wine/obj_base.h"
#include "unknwn.h"

#define VFWAPI	WINAPI
#define VFWAPIV	WINAPIV

DWORD VFWAPI VideoForWindowsVersion(void);

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
	( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |		\
	( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((WORD)(BYTE)(ch0) | ((WORD)(BYTE)(ch1) << 8))
#endif

#define ICTYPE_VIDEO	mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO	mmioFOURCC('a', 'u', 'd', 'c')

/*****************************************************************************
 * Predeclare the interfaces
 */
typedef struct IAVIStream IAVIStream,*PAVISTREAM;
typedef struct IAVIFile IAVIFile,*PAVIFILE;
typedef struct IGetFrame IGetFrame,*PGETFRAME;

/* Installable Compressor M? */

DECLARE_HANDLE(HIC);

#ifdef __WINE__
/* HIC struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
	DWORD		magic;		/* 00: 'Smag' */
	HANDLE	curthread;	/* 04: */
	DWORD		type;		/* 08: */
	DWORD		handler;	/* 0C: */
	HDRVR		hdrv;		/* 10: */
	DWORD		private;	/* 14:(handled by SendDriverMessage)*/
	FARPROC	driverproc;	/* 18:(handled by SendDriverMessage)*/
	DWORD		x1;		/* 1c: name? */
	WORD		x2;		/* 20: */
	DWORD		x3;		/* 22: */
					/* 26: */
} WINE_HIC;
#endif

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
	ICSendMessage(						\
	    hic,ICM_COMPRESS_GET_FORMAT,(DWORD)(LPVOID)(lpbiInput),	\
	    (DWORD)(LPVOID)(lpbiOutput)					\
	)

#define ICCompressGetFormatSize(hic,lpbi) ICCompressGetFormat(hic,lpbi,NULL)

#define ICCompressBegin(hic, lpbiInput, lpbiOutput) 			\
    ICSendMessage(							\
    	hic, ICM_COMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput),		\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )

#define ICCompressGetSize(hic, lpbiInput, lpbiOutput) 		\
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
    LONG CALLBACK (*GetData)(LPARAM lInput,LONG lFrame,LPVOID lpBits,LONG len);
    LONG CALLBACK (*PutData)(LPARAM lOutput,LONG lFrame,LPVOID lpBits,LONG len);
} ICCOMPRESSFRAMES;

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
    INT		xDst;       /* destination rectangle */
    INT		yDst;
    INT		dxDst;
    INT		dyDst;

    INT		xSrc;       /* source rectangle */
    INT		ySrc;
    INT		dxSrc;
    INT		dySrc;
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


#define ICDRAW_QUERY        0x00000001L   /* test for support */
#define ICDRAW_FULLSCREEN   0x00000002L   /* draw to full screen */
#define ICDRAW_HDC          0x00000004L   /* draw to a HDC/HWND */

BOOL	VFWAPI	ICInfo(DWORD fccType, DWORD fccHandler, ICINFO * lpicinfo);
LRESULT	VFWAPI	ICGetInfo(HIC hic,ICINFO *picinfo, DWORD cb);
HIC	VFWAPI	ICOpen(DWORD fccType, DWORD fccHandler, UINT wMode);
HIC	VFWAPI	ICOpenFunction(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler);

LRESULT VFWAPI ICClose(HIC hic);
LRESULT	VFWAPI	ICSendMessage(HIC hic, UINT msg, DWORD dw1, DWORD dw2);
HIC	VFWAPI ICLocate(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);

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

/* as passed to ICM_DRAW_BEGIN (FIXME: correct only for Win32?)  */
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

/********************* AVIFILE function declarations *************************/

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

/* AVIFileHdr.dwFlags */
#define AVIF_HASINDEX		0x00000010	/* Index at end of file? */
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_TRUSTCKTYPE	0x00000800	/* Use CKType to find key frames*/
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

#define AVI_HEADERSIZE	2048

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
    RECT16	rcFrame;	/* word.word - word.word in file */
} AVIStreamHeader;

/* AVIINDEXENTRY.dwFlags */
#define AVIIF_LIST	0x00000001	/* chunk is a 'LIST' */
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
#define ICOM_INTERFACE IAVIStream
#define IAVIStream_METHODS						\
    ICOM_METHOD2(HRESULT,Create,     LPARAM,lParam1, LPARAM,lParam2) \
    ICOM_METHOD2(HRESULT,Info,       AVISTREAMINFOW*,psi, LONG,lSize) \
    ICOM_METHOD2(LONG,   FindSample, LONG,lPos, LONG,lFlags) \
    ICOM_METHOD3(HRESULT,ReadFormat, LONG,lPos, LPVOID,lpFormat, LONG*,lpcbFormat) \
    ICOM_METHOD3(HRESULT,SetFormat,  LONG,lPos, LPVOID,lpFormat, LONG,cbFormat) \
    ICOM_METHOD6(HRESULT,Read,       LONG,lStart, LONG,lSamples, LPVOID,lpBuffer, LONG,cbBuffer, LONG*,plBytes, LONG*,plSamples) \
    ICOM_METHOD7(HRESULT,Write,      LONG,lStart, LONG,lSamples, LPVOID,lpBuffer, LONG,cbBuffer, DWORD,dwFlags, LONG*,plSampWritten, LONG*,plBytesWritten) \
    ICOM_METHOD2(HRESULT,Delete,     LONG,lStart, LONG,lSamples) \
    ICOM_METHOD3(HRESULT,ReadData,   DWORD,fcc, LPVOID,lpBuffer, LONG*,lpcbBuffer) \
    ICOM_METHOD3(HRESULT,WriteData,  DWORD,fcc, LPVOID,lpBuffer, LONG,cbBuffer) \
    ICOM_METHOD2(HRESULT,SetInfo,    AVISTREAMINFOW*,plInfo, LONG,cbInfo)
#define IAVIStream_IMETHODS	\
	IUnknown_IMETHODS	\
	IAVIStream_METHODS
ICOM_DEFINE(IAVIStream, IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IAVIStream_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IAVIStream_AddRef(p)             ICOM_CALL (AddRef,p)
#define IAVIStream_Release(p)            ICOM_CALL (Release,p)
/*** IAVIStream methods ***/
#define IAVIStream_Create(p,a,b)          ICOM_CALL2(Create,p,a,b)
#define IAVIStream_Info(p,a,b)            ICOM_CALL2(Info,p,a,b)
#define IAVIStream_FindSample(p,a,b)      ICOM_CALL2(FindSample,p,a,b)
#define IAVIStream_ReadFormat(p,a,b,c)    ICOM_CALL3(ReadFormat,p,a,b,c)
#define IAVIStream_SetFormat(p,a,b,c)     ICOM_CALL3(SetFormat,p,a,b,c)
#define IAVIStream_Read(p,a,b,c,d,e,f)    ICOM_CALL6(Read,p,a,b,c,d,e,f)
#define IAVIStream_Write(p,a,b,c,d,e,f,g) ICOM_CALL7(Write,p,a,b,c,d,e,f,g)
#define IAVIStream_Delete(p,a,b)          ICOM_CALL2(Delete,p,a,b)
#define IAVIStream_ReadData(p,a,b,c)      ICOM_CALL3(ReadData,p,a,b,c)
#define IAVIStream_WriteData(p,a,b,c)     ICOM_CALL3(WriteData,p,a,b,c)
#define IAVIStream_SetInfo(p,a,b)         ICOM_CALL2(SetInfo,p,a,b)

HRESULT WINAPI AVIMakeCompressedStream(PAVISTREAM*ppsCompressed,PAVISTREAM ppsSource,AVICOMPRESSOPTIONS *lpOptions,CLSID*pclsidHandler); 

HRESULT WINAPI AVIStreamInfoA(PAVISTREAM iface,AVISTREAMINFOA *asi,LONG size);
HRESULT WINAPI AVIStreamInfoW(PAVISTREAM iface,AVISTREAMINFOW *asi,LONG size);
#define AVIStreamInfo WINELIB_NAME_AW(AVIStreamInfo)
LPVOID WINAPI AVIStreamGetFrame(PGETFRAME pg,LONG pos);
HRESULT WINAPI AVIStreamGetFrameClose(PGETFRAME pg);
PGETFRAME WINAPI AVIStreamGetFrameOpen(PAVISTREAM pavi,LPBITMAPINFOHEADER lpbiWanted);
LONG WINAPI AVIStreamLength(PAVISTREAM iface);
HRESULT WINAPI AVIStreamRead(PAVISTREAM iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread);
HRESULT WINAPI AVIStreamReadData(PAVISTREAM iface,DWORD fcc,LPVOID lp,LONG *lpread);
HRESULT WINAPI AVIStreamReadFormat(PAVISTREAM iface,LONG pos,LPVOID format,LONG *formatsize);
ULONG WINAPI AVIStreamRelease(PAVISTREAM iface);
HRESULT WINAPI AVIStreamSetFormat(PAVISTREAM iface,LONG pos,LPVOID format,LONG formatsize);
LONG WINAPI AVIStreamStart(PAVISTREAM iface);
HRESULT WINAPI AVIStreamWrite(PAVISTREAM iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten);
HRESULT WINAPI AVIStreamWriteData(PAVISTREAM iface,DWORD fcc,LPVOID lp,LONG size);


/*****************************************************************************
 * IAVIFile interface
 */
/* In Win32 this interface uses UNICODE only */
#define ICOM_INTERFACE IAVIFile
#define IAVIFile_METHODS 						\
    ICOM_METHOD2(HRESULT,Info,         AVIFILEINFOW*,pfi, LONG,lSize) \
    ICOM_METHOD3(HRESULT,GetStream,    PAVISTREAM*,ppStream, DWORD,fccType, LONG,lParam) \
    ICOM_METHOD2(HRESULT,CreateStream, PAVISTREAM*,ppStream, AVISTREAMINFOW*,psi) \
    ICOM_METHOD3(HRESULT,WriteData,    DWORD,fcc, LPVOID,lpBuffer, LONG,cbBuffer) \
    ICOM_METHOD3(HRESULT,ReadData,     DWORD,fcc, LPVOID,lpBuffer, LONG*,lpcbBuffer) \
	ICOM_METHOD (HRESULT,EndRecord)					\
    ICOM_METHOD2(HRESULT,DeleteStream, DWORD,fccType, LONG,lParam)
#define IAVIFile_IMETHODS 	\
	IUnknown_IMETHODS	\
	IAVIFile_METHODS
ICOM_DEFINE(IAVIFile,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IAVIFile_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IAVIFile_AddRef(p)             ICOM_CALL (AddRef,p)
#define IAVIFile_Release(p)            ICOM_CALL (Release,p)
/*** IAVIFile methods ***/
#define IAVIFile_Info(p,a,b)         ICOM_CALL2(Info,p,a,b)
#define IAVIFile_GetStream(p,a,b,c)  ICOM_CALL3(GetStream,p,a,b,c)
#define IAVIFile_CreateStream(p,a,b) ICOM_CALL2(CreateStream,p,a,b)
#define IAVIFile_WriteData(p,a,b,c)  ICOM_CALL3(WriteData,p,a,b,c)
#define IAVIFile_ReadData(p,a,b,c)   ICOM_CALL3(ReadData,p,a,b,c)
#define IAVIFile_EndRecord(p)        ICOM_CALL (EndRecord,p)
#define IAVIFile_DeleteStream(p,a,b) ICOM_CALL2(DeleteStream,p,a,b)

HRESULT WINAPI AVIFileCreateStreamA(PAVIFILE pfile,PAVISTREAM* ppavi,AVISTREAMINFOA* psi);
HRESULT WINAPI AVIFileCreateStreamW(PAVIFILE pfile,PAVISTREAM* ppavi,AVISTREAMINFOW* psi);
#define AVIFileCreateStream WINELIB_NAME_AW(AVIFileCreateStream)
void WINAPI AVIFileExit(void);
HRESULT WINAPI AVIFileGetStream(PAVIFILE pfile,PAVISTREAM* avis,DWORD fccType,LONG lParam);
HRESULT WINAPI AVIFileInfoA(PAVIFILE pfile,PAVIFILEINFOA pfi,LONG lSize);
HRESULT WINAPI AVIFileInfoW(PAVIFILE pfile,PAVIFILEINFOW pfi,LONG lSize);
#define AVIFileInfo WINELIB_NAME_AW(AVIFileInfo)
void WINAPI AVIFileInit(void);
HRESULT WINAPI AVIFileOpenA(PAVIFILE* ppfile,LPCSTR szFile,UINT uMode,LPCLSID lpHandler);
HRESULT WINAPI AVIFileOpenW(PAVIFILE* ppfile,LPCWSTR szFile,UINT uMode,LPCLSID lpHandler);
#define AVIFileOpen WINELIB_NAME_AW(AVIFileOpen)
ULONG WINAPI AVIFileRelease(PAVIFILE iface);


/*****************************************************************************
 * IGetFrame interface
 */
#define ICOM_INTERFACE IGetFrame
#define IGetFrame_METHODS					\
    ICOM_METHOD1(LPVOID, GetFrame,  LONG,lPos) \
    ICOM_METHOD3(HRESULT,Begin,     LONG,lStart, LONG,lEnd, LONG,lRate) \
	ICOM_METHOD (HRESULT,End)				\
    ICOM_METHOD6(HRESULT,SetFormat, LPBITMAPINFOHEADER,lpbi, LPVOID,lpBits, INT,x, INT,y, INT,dx, INT,dy)
#define IGetFrame_IMETHODS	\
	IUnknown_IMETHODS	\
	IGetFrame_METHODS
ICOM_DEFINE(IGetFrame,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IGetFrame_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IGetFrame_AddRef(p)             ICOM_CALL (AddRef,p)
#define IGetFrame_Release(p)            ICOM_CALL (Release,p)
/*** IGetFrame methods ***/
#define IGetFrame_GetFrame(p,a)            ICOM_CALL1(GetFrame,p,a)
#define IGetFrame_Begin(p,a,b,c)           ICOM_CALL3(Begin,p,a,b,c)
#define IGetFrame_End(p)                   ICOM_CALL (End,p)
#define IGetFrame_SetFormat(p,a,b,c,d,e,f) ICOM_CALL6(SetFormat,p,a,b,c,d,e,f)

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


#endif /* __WINE_VFW_H */
