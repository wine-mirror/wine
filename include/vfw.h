#ifndef __WINE_VFW_H
#define __WINE_VFW_H

#include "wintypes.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "wine/obj_base.h"

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

/* Installable Compressor M? */

/* HIC32 struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
	DWORD		magic;		/* 00: 'Smag' */
	HANDLE32	curthread;	/* 04: */
	DWORD		type;		/* 08: */
	DWORD		handler;	/* 0C: */
	HDRVR32		hdrv;		/* 10: */
	DWORD		private;	/* 14:(handled by SendDriverMessage32)*/
	FARPROC32	driverproc;	/* 18:(handled by SendDriverMessage32)*/
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

DWORD VFWAPIV ICCompress32(
	HIC32 hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiOutput,LPVOID lpData,
	LPBITMAPINFOHEADER lpbiInput,LPVOID lpBits,LPDWORD lpckid,
	LPDWORD lpdwFlags,LONG lFrameNum,DWORD dwFrameSize,DWORD dwQuality,
	LPBITMAPINFOHEADER lpbiPrev,LPVOID lpPrev
);

#define ICCompressGetFormat32(hic, lpbiInput, lpbiOutput) 		\
	ICSendMessage32(						\
	    hic,ICM_COMPRESS_GET_FORMAT,(DWORD)(LPVOID)(lpbiInput),	\
	    (DWORD)(LPVOID)(lpbiOutput)					\
	)
#define ICCompressGetFormat WINELIB_NAME(ICCompressGetFormat)

#define ICCompressGetFormatSize32(hic,lpbi) ICCompressGetFormat32(hic,lpbi,NULL)
#define ICCompressGetFormatSize WINELIB_NAME(ICCompressGetFormatSize)

#define ICCompressBegin32(hic, lpbiInput, lpbiOutput) 			\
    ICSendMessage32(							\
    	hic, ICM_COMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput),		\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )
#define ICCompressBegin WINELIB_NAME(ICCompressBegin)

#define ICCompressGetSize32(hic, lpbiInput, lpbiOutput) 		\
    ICSendMessage32(							\
    	hic, ICM_COMPRESS_GET_SIZE, (DWORD)(LPVOID)(lpbiInput), 	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )
#define ICCompressGetSize WINELIB_NAME(ICCompressGetSize)

#define ICCompressQuery32(hic, lpbiInput, lpbiOutput)		\
    ICSendMessage32(						\
    	hic, ICM_COMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)				\
    )
#define ICCompressQuery WINELIB_NAME(ICCompressQuery)


#define ICCompressEnd32(hic) ICSendMessage32(hic, ICM_COMPRESS_END, 0, 0)
#define ICCompressEnd WINELIB_NAME(ICCompressEnd)

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
} ICINFO32;
DECL_WINELIB_TYPE(ICINFO)

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

#define ICQueryAbout32(hic) \
	(ICSendMessage32(hic,ICM_ABOUT,(DWORD)-1,ICMF_ABOUT_QUERY)==ICERR_OK)
#define ICQueryAbout WINELIB_NAME(ICQueryAbout)

#define ICAbout32(hic, hwnd) ICSendMessage32(hic,ICM_ABOUT,(DWORD)(UINT)(hwnd),0)
#define ICAbout WINELIB_NAME(ICAbout)

/* ICM_CONFIGURE */
#define ICMF_CONFIGURE_QUERY	0x00000001
#define ICQueryConfigure32(hic) \
	(ICSendMessage32(hic,ICM_CONFIGURE,(DWORD)-1,ICMF_CONFIGURE_QUERY)==ICERR_OK)
#define ICQueryConfigure WINELIB_NAME(ICQueryConfigure)

#define ICConfigure32(hic,hwnd) \
	ICSendMessage32(hic,ICM_CONFIGURE,(DWORD)(UINT)(hwnd),0)
#define ICConfigure WINELIB_NAME(ICConfigure)

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

DWORD VFWAPIV ICDecompress32(HIC32 hic,DWORD dwFlags,LPBITMAPINFOHEADER lpbiFormat,LPVOID lpData,LPBITMAPINFOHEADER lpbi,LPVOID lpBits);
#define ICDecompress WINELIB_NAME(ICDecompress)


#define ICDecompressBegin32(hic, lpbiInput, lpbiOutput) 	\
    ICSendMessage32(						\
    	hic, ICM_DECOMPRESS_BEGIN, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)				\
    )
#define ICDecompressBegin WINELIB_NAME(ICDecompressBegin)

#define ICDecompressQuery32(hic, lpbiInput, lpbiOutput) 	\
    ICSendMessage32(						\
    	hic,ICM_DECOMPRESS_QUERY, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD) (LPVOID)(lpbiOutput)				\
    )
#define ICDecompressQuery WINELIB_NAME(ICDecompressQuery)

#define ICDecompressGetFormat32(hic, lpbiInput, lpbiOutput)		\
    ((LONG)ICSendMessage32(						\
    	hic,ICM_DECOMPRESS_GET_FORMAT, (DWORD)(LPVOID)(lpbiInput),	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    ))
#define ICDecompressGetFormat WINELIB_NAME(ICDecompressGetFormat)

#define ICDecompressGetFormatSize32(hic, lpbi) 				\
	ICDecompressGetFormat32(hic, lpbi, NULL)
#define ICDecompressGetFormatSize WINELIB_NAME(ICDecompressGetFormatSize)

#define ICDecompressGetPalette32(hic, lpbiInput, lpbiOutput)		\
    ICSendMessage32(							\
    	hic, ICM_DECOMPRESS_GET_PALETTE, (DWORD)(LPVOID)(lpbiInput), 	\
	(DWORD)(LPVOID)(lpbiOutput)					\
    )
#define ICDecompressGetPalette WINELIB_NAME(ICDecompressGetPalette)

#define ICDecompressSetPalette32(hic,lpbiPalette)	\
        ICSendMessage32(				\
		hic,ICM_DECOMPRESS_SET_PALETTE,		\
		(DWORD)(LPVOID)(lpbiPalette),0		\
	)
#define ICDecompressSetPalette WINELIB_NAME(ICDecompressSetPalette)

#define ICDecompressEnd32(hic) ICSendMessage32(hic, ICM_DECOMPRESS_END, 0, 0)
#define ICDecompressEnd WINELIB_NAME(ICDecompressEnd)


#define ICDRAW_QUERY        0x00000001L   /* test for support */
#define ICDRAW_FULLSCREEN   0x00000002L   /* draw to full screen */
#define ICDRAW_HDC          0x00000004L   /* draw to a HDC/HWND */

BOOL32	VFWAPI	ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO32 * lpicinfo);
#define ICInfo WINELIB_NAME(ICInfo)
LRESULT	VFWAPI	ICGetInfo32(HIC32 hic,ICINFO32 *picinfo, DWORD cb);
#define ICGetInfo WINELIB_NAME(ICGetInfo)
HIC32	VFWAPI	ICOpen32(DWORD fccType, DWORD fccHandler, UINT32 wMode);
#define ICOpen WINELIB_NAME(ICOpen)
LRESULT VFWAPI ICClose32(HIC32 hic);
#define ICClose WINELIB_NAME(ICClose)
LRESULT	VFWAPI	ICSendMessage32(HIC32 hic, UINT32 msg, DWORD dw1, DWORD dw2);
#define ICSendMessage WINELIB_NAME(ICSendMessage)
HIC32	VFWAPI ICLocate32(DWORD fccType, DWORD fccHandler, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut, WORD wFlags);
#define ICLocate WINELIB_NAME(ICLocate)

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
        DWORD			dwScale
);
#define ICDrawBegin WINELIB_NAME(ICDrawBegin)

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

typedef struct _AVISTREAMINFO32A {
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
    RECT32	rcFrame;
    DWORD	dwEditCount;
    DWORD	dwFormatChangeCount;
    CHAR	szName[64];
} AVISTREAMINFO32A, * LPAVISTREAMINFO32A, *PAVISTREAMINFO32A;

typedef struct _AVISTREAMINFO32W {
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
    RECT32	rcFrame;
    DWORD	dwEditCount;
    DWORD	dwFormatChangeCount;
    WCHAR	szName[64];
} AVISTREAMINFO32W, * LPAVISTREAMINFO32W, *PAVISTREAMINFO32W;
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

typedef struct _AVIFILEINFO32W {
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
} AVIFILEINFO32W, * LPAVIFILEINFO32W, *PAVIFILEINFO32W;
typedef struct _AVIFILEINFO32A {
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
} AVIFILEINFO32A, * LPAVIFILEINFO32A, *PAVIFILEINFO32A;
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


/* IAVIStream32 interface. */
#define ICOM_INTERFACE IAVIStream32
typedef struct IAVIStream32 IAVIStream32,*PAVISTREAM32;
ICOM_BEGIN(IAVIStream32, IUnknown)
    ICOM_METHOD2(HRESULT,Create,LPARAM,,LPARAM,);
    ICOM_METHOD2(HRESULT,Info,AVISTREAMINFO32W*,,LONG,);
    ICOM_METHOD2(LONG,FindSample,LONG,,LONG,);
    ICOM_METHOD3(HRESULT,ReadFormat,LONG,,LPVOID,,LONG*,);
    ICOM_METHOD3(HRESULT,SetFormat,LONG,,LPVOID,,LONG,);
    ICOM_METHOD6(HRESULT,Read,LONG,,LONG,,LPVOID,,LONG,,LONG*,,LONG*,);
    ICOM_METHOD7(HRESULT,Write,LONG,,LONG,,LPVOID,,LONG,,DWORD,,LONG*,,LONG*,);
    ICOM_METHOD2(HRESULT,Delete,LONG,,LONG,);
    ICOM_METHOD3(HRESULT,ReadData,DWORD,,LPVOID,,LONG*,);
    ICOM_METHOD3(HRESULT,WriteData,DWORD,,LPVOID,,LONG,);
    ICOM_METHOD2(HRESULT,SetInfo,AVISTREAMINFO32W*,,LONG,);
ICOM_END(IAVIStream32)
#undef ICOM_INTERFACE

DECL_WINELIB_TYPE(IAVIStream)
DECL_WINELIB_TYPE(PAVISTREAM)

/* IAVIFile32 interface. In Win32 this interface uses UNICODE only */
#define ICOM_INTERFACE IAVIFile32
typedef struct IAVIFile32 IAVIFile32,*PAVIFILE32;
ICOM_BEGIN(IAVIFile32,IUnknown)
	ICOM_METHOD2(HRESULT,Info,AVIFILEINFO32W*,,LONG,);
	ICOM_METHOD3(HRESULT,GetStream,PAVISTREAM32*,,DWORD,,LONG,);
	ICOM_METHOD2(HRESULT,CreateStream,PAVISTREAM32*,,AVISTREAMINFO32W*,);
	ICOM_METHOD3(HRESULT,WriteData,DWORD,,LPVOID,,LONG,);
	ICOM_METHOD3(HRESULT,ReadData,DWORD,,LPVOID,,LONG*,);
	ICOM_METHOD (HRESULT,EndRecord);
	ICOM_METHOD2(HRESULT,DeleteStream,DWORD,,LONG,);
ICOM_END(IAVIFile32)
#undef ICOM_INTERFACE

DECL_WINELIB_TYPE(IAVIFile)
DECL_WINELIB_TYPE(PAVIFILE)

/* IGetFrame32 interface */
#define ICOM_INTERFACE IGetFrame32
typedef struct IGetFrame32 IGetFrame32,*PGETFRAME32;
ICOM_BEGIN(IGetFrame32,IUnknown)
	ICOM_METHOD1(LPVOID,GetFrame,LONG,);
	ICOM_METHOD3(HRESULT,Begin,LONG,,LONG,,LONG,);
	ICOM_METHOD (HRESULT,End);
	ICOM_METHOD6(HRESULT,SetFormat,LPBITMAPINFOHEADER,,LPVOID,,INT32,,INT32,,INT32,,INT32,);
ICOM_END(IGetFrame32)
#undef ICOM_INTERFACE

DECL_WINELIB_TYPE(IGetFrame)
DECL_WINELIB_TYPE(PGETFRAME)

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

HRESULT WINAPI AVIMakeCompressedStream32(PAVISTREAM32*ppsCompressed,PAVISTREAM32 ppsSource,AVICOMPRESSOPTIONS *lpOptions,CLSID*pclsidHandler); 
#define AVIMakeCompressedStream WINELIB_NAME_AW(AVIMakeCompressedStream)
HRESULT WINAPI AVIStreamSetFormat32(PAVISTREAM32 iface,LONG pos,LPVOID format,LONG formatsize);
#define AVIStreamSetFormat WINELIB_NAME(AVIStreamSetFormat)
HRESULT WINAPI AVIStreamWrite32(PAVISTREAM32 iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten);
#define AVIStreamWrite WINELIB_NAME(AVIStreamWrite)
ULONG WINAPI AVIStreamRelease32(PAVISTREAM32 iface);
#define AVIStreamRelease WINELIB_NAME(AVIStreamRelease)
LONG WINAPI AVIStreamStart32(PAVISTREAM32 iface);
#define AVIStreamStart WINELIB_NAME(AVIStreamStart)
LONG WINAPI AVIStreamLength32(PAVISTREAM32 iface);
#define AVIStreamLength WINELIB_NAME(AVIStreamLength)
HRESULT WINAPI AVIStreamReadFormat32(PAVISTREAM32 iface,LONG pos,LPVOID format,LONG *formatsize);
#define AVIStreamReadFormat WINELIB_NAME(AVIStreamReadFormat)
HRESULT WINAPI AVIStreamWrite32(PAVISTREAM32 iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,DWORD flags,LONG *sampwritten,LONG *byteswritten);
#define AVIStreamWrite WINELIB_NAME(AVIStreamWrite)
HRESULT WINAPI AVIStreamRead32(PAVISTREAM32 iface,LONG start,LONG samples,LPVOID buffer,LONG buffersize,LONG *bytesread,LONG *samplesread);
#define AVIStreamRead WINELIB_NAME(AVIStreamRead)
HRESULT WINAPI AVIStreamWriteData32(PAVISTREAM32 iface,DWORD fcc,LPVOID lp,LONG size);
#define AVIStreamWriteData WINELIB_NAME(AVIStreamWriteData)
HRESULT WINAPI AVIStreamReadData32(PAVISTREAM32 iface,DWORD fcc,LPVOID lp,LONG *lpread);
#define AVIStreamReadData WINELIB_NAME(AVIStreamReadData)
HRESULT WINAPI AVIStreamInfo32A(PAVISTREAM32 iface,AVISTREAMINFO32A *asi,LONG size);
HRESULT WINAPI AVIStreamInfo32W(PAVISTREAM32 iface,AVISTREAMINFO32W *asi,LONG size);
#define AVIStreamInfo WINELIB_NAME_AW(AVIStreamInfo)
PGETFRAME32 WINAPI AVIStreamGetFrameOpen32(PAVISTREAM32 pavi,LPBITMAPINFOHEADER lpbiWanted);
#define AVIStreamGetFrameOpen WINELIB_NAME(AVIStreamGetFrameOpen)
HRESULT WINAPI AVIStreamGetFrameClose32(PGETFRAME32 pg);
#define AVIStreamGetFrameClose WINELIB_NAME(AVIStreamGetFrameClose)
PGETFRAME32 WINAPI AVIStreamGetFrameOpen32(PAVISTREAM32 pavi,LPBITMAPINFOHEADER lpbiWanted);
#define AVIStreamGetFrameOpen WINELIB_NAME(AVIStreamGetFrameOpen)
LPVOID WINAPI AVIStreamGetFrame32(PGETFRAME32 pg,LONG pos);
#define AVIStreamGetFrame WINELIB_NAME(AVIStreamGetFrame)

void WINAPI AVIFileInit32(void);
#define AVIFileInit WINELIB_NAME(AVIFileInit)
HRESULT WINAPI AVIFileOpen32A(PAVIFILE32 * ppfile,LPCSTR szFile,UINT32 uMode,LPCLSID lpHandler);
#define AVIFileOpen WINELIB_NAME_AW(AVIFileOpen)
HRESULT WINAPI AVIFileCreateStream32A(PAVIFILE32 pfile,PAVISTREAM32 *ppavi,AVISTREAMINFO32A * psi);
HRESULT WINAPI AVIFileCreateStream32W(PAVIFILE32 pfile,PAVISTREAM32 *ppavi,AVISTREAMINFO32W * psi);
#define AVIFileCreateStream WINELIB_NAME_AW(AVIFileCreateStream)
ULONG WINAPI AVIFileRelease32(PAVIFILE32 iface);
#define AVIFileRelease WINELIB_NAME(AVIFileRelease)
HRESULT WINAPI AVIFileInfo32A(PAVIFILE32 pfile,PAVIFILEINFO32A,LONG);
HRESULT WINAPI AVIFileInfo32W(PAVIFILE32 pfile,PAVIFILEINFO32W,LONG);
#define AVIFileInfo WINELIB_NAME_AW(AVIFileInfo)
HRESULT WINAPI AVIFileGetStream32(PAVIFILE32 pfile,PAVISTREAM32*avis,DWORD fccType,LONG lParam);
#define AVIFileGetStream WINELIB_NAME(AVIFileGetStream)
void WINAPI AVIFileExit32(void);
#define AVIFileExit WINELIB_NAME(AVIFileExit)

#endif
