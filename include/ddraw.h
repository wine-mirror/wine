#ifndef __WINE_DDRAW_H
#define __WINE_DDRAW_H

#include "ts_xlib.h"

#ifndef	DIRECTDRAW_VERSION
#define	DIRECTDRAW_VERSION	0x0500
#endif /* DIRECTDRAW_VERSION */


DEFINE_GUID( CLSID_DirectDraw,		0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( CLSID_DirectDrawClipper,	0x593817A0,0x7DB3,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xb9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw,		0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDraw2,		0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDrawSurface,	0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawSurface2,	0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27 );
DEFINE_GUID( IID_IDirectDrawSurface3,	0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB );
DEFINE_GUID( IID_IDirectDrawPalette,	0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawClipper,	0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawColorControl,0x4B9F0EE0,0x0D7E,0x11D0,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8 );

typedef struct IDirectDraw IDirectDraw,*LPDIRECTDRAW;
typedef struct IDirectDraw2 IDirectDraw2,*LPDIRECTDRAW2;
typedef struct IDirectDrawClipper IDirectDrawClipper,*LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawPalette IDirectDrawPalette,*LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawSurface IDirectDrawSurface,*LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2 IDirectDrawSurface2,*LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface3 IDirectDrawSurface3,*LPDIRECTDRAWSURFACE3;
typedef struct IDirectDrawColorControl IDirectDrawColorControl,*LPDIRECTDRAWCOLORCONTROL;

#define DDENUMRET_CANCEL	0
#define DDENUMRET_OK		1

#define DD_OK			0


#define _FACDD		0x876
#define MAKE_DDHRESULT( code )  MAKE_HRESULT( 1, _FACDD, code )

#define DDERR_ALREADYINITIALIZED		MAKE_DDHRESULT( 5 )
#define DDERR_CANNOTATTACHSURFACE		MAKE_DDHRESULT( 10 )
#define DDERR_CANNOTDETACHSURFACE		MAKE_DDHRESULT( 20 )
#define DDERR_CURRENTLYNOTAVAIL			MAKE_DDHRESULT( 40 )
#define DDERR_EXCEPTION				MAKE_DDHRESULT( 55 )
#define DDERR_GENERIC				E_FAIL
#define DDERR_HEIGHTALIGN			MAKE_DDHRESULT( 90 )
#define DDERR_INCOMPATIBLEPRIMARY		MAKE_DDHRESULT( 95 )
#define DDERR_INVALIDCAPS			MAKE_DDHRESULT( 100 )
#define DDERR_INVALIDCLIPLIST			MAKE_DDHRESULT( 110 )
#define DDERR_INVALIDMODE			MAKE_DDHRESULT( 120 )
#define DDERR_INVALIDOBJECT			MAKE_DDHRESULT( 130 )
#define DDERR_INVALIDPARAMS			E_INVALIDARG
#define DDERR_INVALIDPIXELFORMAT		MAKE_DDHRESULT( 145 )
#define DDERR_INVALIDRECT			MAKE_DDHRESULT( 150 )
#define DDERR_LOCKEDSURFACES			MAKE_DDHRESULT( 160 )
#define DDERR_NO3D				MAKE_DDHRESULT( 170 )
#define DDERR_NOALPHAHW				MAKE_DDHRESULT( 180 )
#define DDERR_NOCLIPLIST			MAKE_DDHRESULT( 205 )
#define DDERR_NOCOLORCONVHW			MAKE_DDHRESULT( 210 )
#define DDERR_NOCOOPERATIVELEVELSET		MAKE_DDHRESULT( 212 )
#define DDERR_NOCOLORKEY			MAKE_DDHRESULT( 215 )
#define DDERR_NOCOLORKEYHW			MAKE_DDHRESULT( 220 )
#define DDERR_NODIRECTDRAWSUPPORT		MAKE_DDHRESULT( 222 )
#define DDERR_NOEXCLUSIVEMODE			MAKE_DDHRESULT( 225 )
#define DDERR_NOFLIPHW				MAKE_DDHRESULT( 230 )
#define DDERR_NOGDI				MAKE_DDHRESULT( 240 )
#define DDERR_NOMIRRORHW			MAKE_DDHRESULT( 250 )
#define DDERR_NOTFOUND				MAKE_DDHRESULT( 255 )
#define DDERR_NOOVERLAYHW			MAKE_DDHRESULT( 260 )
#define DDERR_NORASTEROPHW			MAKE_DDHRESULT( 280 )
#define DDERR_NOROTATIONHW			MAKE_DDHRESULT( 290 )
#define DDERR_NOSTRETCHHW			MAKE_DDHRESULT( 310 )
#define DDERR_NOT4BITCOLOR			MAKE_DDHRESULT( 316 )
#define DDERR_NOT4BITCOLORINDEX			MAKE_DDHRESULT( 317 )
#define DDERR_NOT8BITCOLOR			MAKE_DDHRESULT( 320 )
#define DDERR_NOTEXTUREHW			MAKE_DDHRESULT( 330 )
#define DDERR_NOVSYNCHW				MAKE_DDHRESULT( 335 )
#define DDERR_NOZBUFFERHW			MAKE_DDHRESULT( 340 )
#define DDERR_NOZOVERLAYHW			MAKE_DDHRESULT( 350 )
#define DDERR_OUTOFCAPS				MAKE_DDHRESULT( 360 )
#define DDERR_OUTOFMEMORY			E_OUTOFMEMORY
#define DDERR_OUTOFVIDEOMEMORY			MAKE_DDHRESULT( 380 )
#define DDERR_OVERLAYCANTCLIP			MAKE_DDHRESULT( 382 )
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE	MAKE_DDHRESULT( 384 )
#define DDERR_PALETTEBUSY			MAKE_DDHRESULT( 387 )
#define DDERR_COLORKEYNOTSET			MAKE_DDHRESULT( 400 )
#define DDERR_SURFACEALREADYATTACHED		MAKE_DDHRESULT( 410 )
#define DDERR_SURFACEALREADYDEPENDENT		MAKE_DDHRESULT( 420 )
#define DDERR_SURFACEBUSY			MAKE_DDHRESULT( 430 )
#define DDERR_CANTLOCKSURFACE			MAKE_DDHRESULT( 435 )
#define DDERR_SURFACEISOBSCURED			MAKE_DDHRESULT( 440 )
#define DDERR_SURFACELOST			MAKE_DDHRESULT( 450 )
#define DDERR_SURFACENOTATTACHED		MAKE_DDHRESULT( 460 )
#define DDERR_TOOBIGHEIGHT			MAKE_DDHRESULT( 470 )
#define DDERR_TOOBIGSIZE			MAKE_DDHRESULT( 480 )
#define DDERR_TOOBIGWIDTH			MAKE_DDHRESULT( 490 )
#define DDERR_UNSUPPORTED			E_NOTIMPL
#define DDERR_UNSUPPORTEDFORMAT			MAKE_DDHRESULT( 510 )
#define DDERR_UNSUPPORTEDMASK			MAKE_DDHRESULT( 520 )
#define DDERR_VERTICALBLANKINPROGRESS		MAKE_DDHRESULT( 537 )
#define DDERR_WASSTILLDRAWING			MAKE_DDHRESULT( 540 )
#define DDERR_XALIGN				MAKE_DDHRESULT( 560 )
#define DDERR_INVALIDDIRECTDRAWGUID		MAKE_DDHRESULT( 561 )
#define DDERR_DIRECTDRAWALREADYCREATED		MAKE_DDHRESULT( 562 )
#define DDERR_NODIRECTDRAWHW			MAKE_DDHRESULT( 563 )
#define DDERR_PRIMARYSURFACEALREADYEXISTS	MAKE_DDHRESULT( 564 )
#define DDERR_NOEMULATION			MAKE_DDHRESULT( 565 )
#define DDERR_REGIONTOOSMALL			MAKE_DDHRESULT( 566 )
#define DDERR_CLIPPERISUSINGHWND		MAKE_DDHRESULT( 567 )
#define DDERR_NOCLIPPERATTACHED			MAKE_DDHRESULT( 568 )
#define DDERR_NOHWND				MAKE_DDHRESULT( 569 )
#define DDERR_HWNDSUBCLASSED			MAKE_DDHRESULT( 570 )
#define DDERR_HWNDALREADYSET			MAKE_DDHRESULT( 571 )
#define DDERR_NOPALETTEATTACHED			MAKE_DDHRESULT( 572 )
#define DDERR_NOPALETTEHW			MAKE_DDHRESULT( 573 )
#define DDERR_BLTFASTCANTCLIP			MAKE_DDHRESULT( 574 )
#define DDERR_NOBLTHW				MAKE_DDHRESULT( 575 )
#define DDERR_NODDROPSHW			MAKE_DDHRESULT( 576 )
#define DDERR_OVERLAYNOTVISIBLE			MAKE_DDHRESULT( 577 )
#define DDERR_NOOVERLAYDEST			MAKE_DDHRESULT( 578 )
#define DDERR_INVALIDPOSITION			MAKE_DDHRESULT( 579 )
#define DDERR_NOTAOVERLAYSURFACE		MAKE_DDHRESULT( 580 )
#define DDERR_EXCLUSIVEMODEALREADYSET		MAKE_DDHRESULT( 581 )
#define DDERR_NOTFLIPPABLE			MAKE_DDHRESULT( 582 )
#define DDERR_CANTDUPLICATE			MAKE_DDHRESULT( 583 )
#define DDERR_NOTLOCKED				MAKE_DDHRESULT( 584 )
#define DDERR_CANTCREATEDC			MAKE_DDHRESULT( 585 )
#define DDERR_NODC				MAKE_DDHRESULT( 586 )
#define DDERR_WRONGMODE				MAKE_DDHRESULT( 587 )
#define DDERR_IMPLICITLYCREATED			MAKE_DDHRESULT( 588 )
#define DDERR_NOTPALETTIZED			MAKE_DDHRESULT( 589 )
#define DDERR_UNSUPPORTEDMODE			MAKE_DDHRESULT( 590 )
#define DDERR_NOMIPMAPHW			MAKE_DDHRESULT( 591 )
#define DDERR_INVALIDSURFACETYPE		MAKE_DDHRESULT( 592 )
#define DDERR_NOOPTIMIZEHW			MAKE_DDHRESULT( 600 )
#define DDERR_NOTLOADED				MAKE_DDHRESULT( 601 )
#define DDERR_NOFOCUSWINDOW			MAKE_DDHRESULT( 602 )
#define DDERR_DCALREADYCREATED			MAKE_DDHRESULT( 620 )
#define DDERR_NONONLOCALVIDMEM			MAKE_DDHRESULT( 630 )
#define DDERR_CANTPAGELOCK			MAKE_DDHRESULT( 640 )
#define DDERR_CANTPAGEUNLOCK			MAKE_DDHRESULT( 660 )
#define DDERR_NOTPAGELOCKED			MAKE_DDHRESULT( 680 )
#define DDERR_MOREDATA				MAKE_DDHRESULT( 690 )
#define DDERR_VIDEONOTACTIVE			MAKE_DDHRESULT( 695 )
#define DDERR_DEVICEDOESNTOWNSURFACE		MAKE_DDHRESULT( 699 )
#define DDERR_NOTINITIALIZED			CO_E_NOTINITIALIZED

/* dwFlags for Blt* */
#define DDBLT_ALPHADEST				0x00000001
#define DDBLT_ALPHADESTCONSTOVERRIDE		0x00000002
#define DDBLT_ALPHADESTNEG			0x00000004
#define DDBLT_ALPHADESTSURFACEOVERRIDE		0x00000008
#define DDBLT_ALPHAEDGEBLEND			0x00000010
#define DDBLT_ALPHASRC				0x00000020
#define DDBLT_ALPHASRCCONSTOVERRIDE		0x00000040
#define DDBLT_ALPHASRCNEG			0x00000080
#define DDBLT_ALPHASRCSURFACEOVERRIDE		0x00000100
#define DDBLT_ASYNC				0x00000200
#define DDBLT_COLORFILL				0x00000400
#define DDBLT_DDFX				0x00000800
#define DDBLT_DDROPS				0x00001000
#define DDBLT_KEYDEST				0x00002000
#define DDBLT_KEYDESTOVERRIDE			0x00004000
#define DDBLT_KEYSRC				0x00008000
#define DDBLT_KEYSRCOVERRIDE			0x00010000
#define DDBLT_ROP				0x00020000
#define DDBLT_ROTATIONANGLE			0x00040000
#define DDBLT_ZBUFFER				0x00080000
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE		0x00100000
#define DDBLT_ZBUFFERDESTOVERRIDE		0x00200000
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE		0x00400000
#define DDBLT_ZBUFFERSRCOVERRIDE		0x00800000
#define DDBLT_WAIT				0x01000000
#define DDBLT_DEPTHFILL				0x02000000

/* dwTrans for BltFast */
#define DDBLTFAST_NOCOLORKEY			0x00000000
#define DDBLTFAST_SRCCOLORKEY			0x00000001
#define DDBLTFAST_DESTCOLORKEY			0x00000002
#define DDBLTFAST_WAIT				0x00000010

/* dwFlags for Flip */
#define DDFLIP_WAIT	0x00000001
#define DDFLIP_EVEN	0x00000002 /* only valid for overlay */
#define DDFLIP_ODD	0x00000004 /* only valid for overlay */

/* dwFlags for GetBltStatus */
#define DDGBS_CANBLT				0x00000001
#define DDGBS_ISBLTDONE				0x00000002

/* DDSCAPS.dwCaps */
/* reserved1, was 3d capable */
#define DDSCAPS_RESERVED1		0x00000001
/* surface contains alpha information */
#define DDSCAPS_ALPHA			0x00000002
/* this surface is a backbuffer */
#define DDSCAPS_BACKBUFFER		0x00000004
/* complex surface structure */
#define DDSCAPS_COMPLEX			0x00000008
/* part of surface flipping structure */
#define DDSCAPS_FLIP			0x00000010
/* this surface is the frontbuffer surface */
#define DDSCAPS_FRONTBUFFER		0x00000020
/* this is a plain offscreen surface */
#define DDSCAPS_OFFSCREENPLAIN		0x00000040
/* overlay */
#define DDSCAPS_OVERLAY			0x00000080
/* palette objects can be created and attached to us */
#define DDSCAPS_PALETTE			0x00000100
/* primary surface (the one the user looks at currently)(right eye)*/
#define DDSCAPS_PRIMARYSURFACE		0x00000200
/* primary surface for left eye */
#define DDSCAPS_PRIMARYSURFACELEFT	0x00000400
/* surface exists in systemmemory */
#define DDSCAPS_SYSTEMMEMORY		0x00000800
/* surface can be used as a texture */
#define DDSCAPS_TEXTURE		        0x00001000
/* surface may be destination for 3d rendering */
#define DDSCAPS_3DDEVICE		0x00002000
/* surface exists in videomemory */
#define DDSCAPS_VIDEOMEMORY		0x00004000
/* surface changes immediately visible */
#define DDSCAPS_VISIBLE			0x00008000
/* write only surface */
#define DDSCAPS_WRITEONLY		0x00010000
/* zbuffer surface */
#define DDSCAPS_ZBUFFER			0x00020000
/* has its own DC */
#define DDSCAPS_OWNDC			0x00040000
/* surface should be able to receive live video */
#define DDSCAPS_LIVEVIDEO		0x00080000
/* should be able to have a hw codec decompress stuff into it */
#define DDSCAPS_HWCODEC			0x00100000
/* mode X (320x200 or 320x240) surface */
#define DDSCAPS_MODEX			0x00200000
/* one mipmap surface (1 level) */
#define DDSCAPS_MIPMAP			0x00400000
#define DDSCAPS_RESERVED2		0x00800000
/* memory allocation delayed until Load() */
#define DDSCAPS_ALLOCONLOAD		0x04000000
/* Indicates that the surface will recieve data from a video port */
#define DDSCAPS_VIDEOPORT		0x08000000
/* surface is in local videomemory */
#define DDSCAPS_LOCALVIDMEM		0x10000000
/* surface is in nonlocal videomemory */
#define DDSCAPS_NONLOCALVIDMEM		0x20000000
/* surface is a standard VGA mode surface (NOT ModeX) */
#define DDSCAPS_STANDARDVGAMODE		0x40000000
/* optimized? surface */
#define DDSCAPS_OPTIMIZED		0x80000000

typedef struct _DDSCAPS {
	DWORD	dwCaps;	/* capabilities of surface wanted */
} DDSCAPS,*LPDDSCAPS;

#define	DD_ROP_SPACE	(256/32)	/* space required to store ROP array */

typedef struct _DDCAPS_DX3
{
    DWORD	dwSize;                 /* size of the DDDRIVERCAPS structure */
    DWORD	dwCaps;                 /* driver specific capabilities */
    DWORD	dwCaps2;                /* more driver specific capabilites */
    DWORD	dwCKeyCaps;             /* color key capabilities of the surface */
    DWORD	dwFXCaps;               /* driver specific stretching and effects capabilites */
    DWORD	dwFXAlphaCaps;          /* alpha driver specific capabilities */
    DWORD	dwPalCaps;              /* palette capabilities */
    DWORD	dwSVCaps;               /* stereo vision capabilities */
    DWORD	dwAlphaBltConstBitDepths;       /* DDBD_2,4,8 */
    DWORD	dwAlphaBltPixelBitDepths;       /* DDBD_1,2,4,8 */
    DWORD	dwAlphaBltSurfaceBitDepths;     /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlayConstBitDepths;   /* DDBD_2,4,8 */
    DWORD	dwAlphaOverlayPixelBitDepths;   /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlaySurfaceBitDepths; /* DDBD_1,2,4,8 */
    DWORD	dwZBufferBitDepths;             /* DDBD_8,16,24,32 */
    DWORD	dwVidMemTotal;          /* total amount of video memory */
    DWORD	dwVidMemFree;           /* amount of free video memory */
    DWORD	dwMaxVisibleOverlays;   /* maximum number of visible overlays */
    DWORD	dwCurrVisibleOverlays;  /* current number of visible overlays */
    DWORD	dwNumFourCCCodes;       /* number of four cc codes */
    DWORD	dwAlignBoundarySrc;     /* source rectangle alignment */
    DWORD	dwAlignSizeSrc;         /* source rectangle byte size */
    DWORD	dwAlignBoundaryDest;    /* dest rectangle alignment */
    DWORD	dwAlignSizeDest;        /* dest rectangle byte size */
    DWORD	dwAlignStrideAlign;     /* stride alignment */
    DWORD	dwRops[DD_ROP_SPACE];   /* ROPS supported */
    DDSCAPS	ddsCaps;                /* DDSCAPS structure has all the general capabilities */
    DWORD	dwMinOverlayStretch;    /* minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxOverlayStretch;    /* maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinLiveVideoStretch;  /* minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxLiveVideoStretch;  /* maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinHwCodecStretch;    /* minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxHwCodecStretch;    /* maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwReserved1;
    DWORD	dwReserved2;
    DWORD	dwReserved3;
    DWORD	dwSVBCaps;              /* driver specific capabilities for System->Vmem blts */
    DWORD	dwSVBCKeyCaps;          /* driver color key capabilities for System->Vmem blts */
    DWORD	dwSVBFXCaps;            /* driver FX capabilities for System->Vmem blts */
    DWORD	dwSVBRops[DD_ROP_SPACE];/* ROPS supported for System->Vmem blts */
    DWORD	dwVSBCaps;              /* driver specific capabilities for Vmem->System blts */
    DWORD	dwVSBCKeyCaps;          /* driver color key capabilities for Vmem->System blts */
    DWORD	dwVSBFXCaps;            /* driver FX capabilities for Vmem->System blts */
    DWORD	dwVSBRops[DD_ROP_SPACE];/* ROPS supported for Vmem->System blts */
    DWORD	dwSSBCaps;              /* driver specific capabilities for System->System blts */
    DWORD	dwSSBCKeyCaps;          /* driver color key capabilities for System->System blts */
    DWORD	dwSSBFXCaps;            /* driver FX capabilities for System->System blts */
    DWORD	dwSSBRops[DD_ROP_SPACE];/* ROPS supported for System->System blts */
    DWORD	dwReserved4;
    DWORD	dwReserved5;
    DWORD	dwReserved6;
} DDCAPS_DX3,*LPDDCAPS_DX3;

typedef struct _DDCAPS
{
/*  0*/ DWORD  dwSize;			/* size of the DDDRIVERCAPS structure */
/*  4*/ DWORD  dwCaps;			/* driver specific capabilities */
/*  8*/ DWORD  dwCaps2;			/* more driver specific capabilites */
/*  c*/ DWORD  dwCKeyCaps;		/* color key capabilities of the surface */
/* 10*/ DWORD  dwFXCaps;		/* driver specific stretching and effects capabilites */
/* 14*/ DWORD  dwFXAlphaCaps;		/* alpha driver specific capabilities */
/* 18*/ DWORD  dwPalCaps;		/* palette capabilities */
/* 1c*/ DWORD  dwSVCaps;		/* stereo vision capabilities */
/* 20*/ DWORD  dwAlphaBltConstBitDepths;	/* DDBD_2,4,8 */
/* 24*/ DWORD  dwAlphaBltPixelBitDepths;	/* DDBD_1,2,4,8 */
/* 28*/ DWORD  dwAlphaBltSurfaceBitDepths;	/* DDBD_1,2,4,8 */
/* 2c*/ DWORD  dwAlphaOverlayConstBitDepths;	/* DDBD_2,4,8 */
/* 30*/ DWORD  dwAlphaOverlayPixelBitDepths;	/* DDBD_1,2,4,8 */
/* 34*/ DWORD  dwAlphaOverlaySurfaceBitDepths;	/* DDBD_1,2,4,8 */
/* 38*/ DWORD  dwZBufferBitDepths;		/* DDBD_8,16,24,32 */
/* 3c*/ DWORD  dwVidMemTotal;		/* total amount of video memory */
/* 40*/ DWORD  dwVidMemFree;		/* amount of free video memory */
/* 44*/ DWORD  dwMaxVisibleOverlays;	/* maximum number of visible overlays */
/* 48*/ DWORD  dwCurrVisibleOverlays;	/* current number of visible overlays */
/* 4c*/ DWORD  dwNumFourCCCodes;	/* number of four cc codes */
/* 50*/ DWORD  dwAlignBoundarySrc;	/* source rectangle alignment */
/* 54*/ DWORD  dwAlignSizeSrc;		/* source rectangle byte size */
/* 58*/ DWORD  dwAlignBoundaryDest;	/* dest rectangle alignment */
/* 5c*/ DWORD  dwAlignSizeDest;		/* dest rectangle byte size */
/* 60*/ DWORD  dwAlignStrideAlign;	/* stride alignment */
/* 64*/ DWORD  dwRops[DD_ROP_SPACE];	/* ROPS supported */
/* 84*/ DDSCAPS ddsCaps;		/* DDSCAPS structure has all the general capabilities */
/* 88*/ DWORD  dwMinOverlayStretch;	/* minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 8c*/ DWORD  dwMaxOverlayStretch;	/* maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 90*/ DWORD  dwMinLiveVideoStretch;	/* minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 94*/ DWORD  dwMaxLiveVideoStretch;	/* maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 98*/ DWORD  dwMinHwCodecStretch;	/* minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 9c*/ DWORD  dwMaxHwCodecStretch;	/* maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* a0*/ DWORD  dwReserved1;
/* a4*/ DWORD  dwReserved2;
/* a8*/ DWORD  dwReserved3;
/* ac*/ DWORD  dwSVBCaps;	/* driver specific capabilities for System->Vmem blts */
/* b0*/ DWORD  dwSVBCKeyCaps;	/* driver color key capabilities for System->Vmem blts */
/* b4*/ DWORD  dwSVBFXCaps;	/* driver FX capabilities for System->Vmem blts */
/* b8*/ DWORD  dwSVBRops[DD_ROP_SPACE];/* ROPS supported for System->Vmem blts */
/* d8*/ DWORD  dwVSBCaps;	/* driver specific capabilities for Vmem->System blts */
/* dc*/ DWORD  dwVSBCKeyCaps;	/* driver color key capabilities for Vmem->System blts */
/* e0*/ DWORD  dwVSBFXCaps;	/* driver FX capabilities for Vmem->System blts */
/* e4*/ DWORD  dwVSBRops[DD_ROP_SPACE];/* ROPS supported for Vmem->System blts */
/*104*/ DWORD  dwSSBCaps;	/* driver specific capabilities for System->System blts */
/*108*/ DWORD  dwSSBCKeyCaps;	/* driver color key capabilities for System->System blts */
/*10c*/ DWORD  dwSSBFXCaps;	/* driver FX capabilities for System->System blts */
/*110*/ DWORD  dwSSBRops[DD_ROP_SPACE];/* ROPS supported for System->System blts */
#if       DIRECTDRAW_VERSION >= 0x0500
/*130*/ DWORD  dwMaxVideoPorts;	/* maximum number of usable video ports */
/*134*/ DWORD  dwCurrVideoPorts;/* current number of video ports used */
/*138*/ DWORD  dwSVBCaps2;	/* more driver specific capabilities for System->Vmem blts */
/*13c*/ DWORD  dwNLVBCaps;	/* driver specific capabilities for non-local->local vidmem blts */
/*140*/ DWORD  dwNLVBCaps2;	/* more driver specific capabilities non-local->local vidmem blts */
/*144*/ DWORD  dwNLVBCKeyCaps;	/* driver color key capabilities for non-local->local vidmem blts */
/*148*/ DWORD  dwNLVBFXCaps;	/* driver FX capabilities for non-local->local blts */
/*14c*/ DWORD  dwNLVBRops[DD_ROP_SPACE];/* ROPS supported for non-local->local blts */
#else  /* DIRECTDRAW_VERSION >= 0x0500 */
/*130*/ DWORD  dwReserved4;
/*134*/ DWORD  dwReserved5;
/*138*/ DWORD  dwReserved6;
#endif /* DIRECTDRAW_VERSION >= 0x0500 */
} DDCAPS,*LPDDCAPS;


/* DDCAPS.dwCaps */
#define DDCAPS_3D			0x00000001
#define DDCAPS_ALIGNBOUNDARYDEST	0x00000002
#define DDCAPS_ALIGNSIZEDEST		0x00000004
#define DDCAPS_ALIGNBOUNDARYSRC		0x00000008
#define DDCAPS_ALIGNSIZESRC		0x00000010
#define DDCAPS_ALIGNSTRIDE		0x00000020
#define DDCAPS_BLT			0x00000040
#define DDCAPS_BLTQUEUE			0x00000080
#define DDCAPS_BLTFOURCC		0x00000100
#define DDCAPS_BLTSTRETCH		0x00000200
#define DDCAPS_GDI			0x00000400
#define DDCAPS_OVERLAY			0x00000800
#define DDCAPS_OVERLAYCANTCLIP		0x00001000
#define DDCAPS_OVERLAYFOURCC		0x00002000
#define DDCAPS_OVERLAYSTRETCH		0x00004000
#define DDCAPS_PALETTE			0x00008000
#define DDCAPS_PALETTEVSYNC		0x00010000
#define DDCAPS_READSCANLINE		0x00020000
#define DDCAPS_STEREOVIEW		0x00040000
#define DDCAPS_VBI			0x00080000
#define DDCAPS_ZBLTS			0x00100000
#define DDCAPS_ZOVERLAYS		0x00200000
#define DDCAPS_COLORKEY			0x00400000
#define DDCAPS_ALPHA			0x00800000
#define DDCAPS_COLORKEYHWASSIST		0x01000000
#define DDCAPS_NOHARDWARE		0x02000000
#define DDCAPS_BLTCOLORFILL		0x04000000
#define DDCAPS_BANKSWITCHED		0x08000000
#define DDCAPS_BLTDEPTHFILL		0x10000000
#define DDCAPS_CANCLIP			0x20000000
#define DDCAPS_CANCLIPSTRETCHED		0x40000000
#define DDCAPS_CANBLTSYSMEM		0x80000000

/* DDCAPS.dwCaps2 */
#define DDCAPS2_CERTIFIED		0x00000001
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002
#define DDCAPS2_VIDEOPORT		0x00000004
#define DDCAPS2_AUTOFLIPOVERLAY		0x00000008
#define DDCAPS2_CANBOBINTERLEAVED	0x00000010
#define DDCAPS2_CANBOBNONINTERLEAVED	0x00000020
#define DDCAPS2_COLORCONTROLOVERLAY	0x00000040
#define DDCAPS2_COLORCONTROLPRIMARY	0x00000080
#define DDCAPS2_CANDROPZ16BIT		0x00000100
#define DDCAPS2_NONLOCALVIDMEM		0x00000200
#define DDCAPS2_NONLOCALVIDMEMCAPS	0x00000400
#define DDCAPS2_NOPAGELOCKREQUIRED	0x00000800
#define DDCAPS2_WIDESURFACES		0x00001000
#define DDCAPS2_CANFLIPODDEVEN		0x00002000
#define DDCAPS2_CANBOBHARDWARE		0x00004000

typedef struct _DDCOLORKEY
{
	DWORD	dwColorSpaceLowValue;/* low boundary of color space that is to
                                      * be treated as Color Key, inclusive
				      */
	DWORD	dwColorSpaceHighValue;/* high boundary of color space that is
                                       * to be treated as Color Key, inclusive
				       */
} DDCOLORKEY,*LPDDCOLORKEY;

/* ddCKEYCAPS bits */
#define DDCKEYCAPS_DESTBLT			0x00000001
#define DDCKEYCAPS_DESTBLTCLRSPACE		0x00000002
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV		0x00000004
#define DDCKEYCAPS_DESTBLTYUV			0x00000008
#define DDCKEYCAPS_DESTOVERLAY			0x00000010
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE		0x00000020
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV	0x00000040
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE		0x00000080
#define DDCKEYCAPS_DESTOVERLAYYUV		0x00000100
#define DDCKEYCAPS_SRCBLT			0x00000200
#define DDCKEYCAPS_SRCBLTCLRSPACE		0x00000400
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV		0x00000800
#define DDCKEYCAPS_SRCBLTYUV			0x00001000
#define DDCKEYCAPS_SRCOVERLAY			0x00002000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE		0x00004000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV	0x00008000
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE		0x00010000
#define DDCKEYCAPS_SRCOVERLAYYUV		0x00020000
#define DDCKEYCAPS_NOCOSTOVERLAY		0x00040000

typedef struct _DDPIXELFORMAT {
    DWORD	dwSize;                 /* 0: size of structure */
    DWORD	dwFlags;                /* 4: pixel format flags */
    DWORD	dwFourCC;               /* 8: (FOURCC code) */
    union {
	DWORD	dwRGBBitCount;          /* C: how many bits per pixel */
	DWORD	dwYUVBitCount;          /* C: how many bits per pixel */
	DWORD	dwZBufferBitDepth;      /* C: how many bits for z buffers */
	DWORD	dwAlphaBitDepth;        /* C: how many bits for alpha channels*/
    } x;
    union {
	DWORD	dwRBitMask;             /* 10: mask for red bit*/
	DWORD	dwYBitMask;             /* 10: mask for Y bits*/
    } y;
    union {
	DWORD	dwGBitMask;             /* 14: mask for green bits*/
	DWORD	dwUBitMask;             /* 14: mask for U bits*/
    } z;
    union {
	DWORD   dwBBitMask;             /* 18: mask for blue bits*/
	DWORD   dwVBitMask;             /* 18: mask for V bits*/
    } xx;
    union {
    	DWORD	dwRGBAlphaBitMask;	/* 1C: mask for alpha channel */
    	DWORD	dwYUVAlphaBitMask;	/* 1C: mask for alpha channel */
	DWORD	dwRGBZBitMask;		/* 1C: mask for Z channel */
	DWORD	dwYUVZBitMask;		/* 1C: mask for Z channel */
    } xy;
    					/* 20: next structure */
} DDPIXELFORMAT,*LPDDPIXELFORMAT;

/* DDCAPS.dwFXCaps */
#define DDFXCAPS_BLTARITHSTRETCHY	0x00000020
#define DDFXCAPS_BLTARITHSTRETCHYN	0x00000010
#define DDFXCAPS_BLTMIRRORLEFTRIGHT	0x00000040
#define DDFXCAPS_BLTMIRRORUPDOWN	0x00000080
#define DDFXCAPS_BLTROTATION		0x00000100
#define DDFXCAPS_BLTROTATION90		0x00000200
#define DDFXCAPS_BLTSHRINKX		0x00000400
#define DDFXCAPS_BLTSHRINKXN		0x00000800
#define DDFXCAPS_BLTSHRINKY		0x00001000
#define DDFXCAPS_BLTSHRINKYN		0x00002000
#define DDFXCAPS_BLTSTRETCHX		0x00004000
#define DDFXCAPS_BLTSTRETCHXN		0x00008000
#define DDFXCAPS_BLTSTRETCHY		0x00010000
#define DDFXCAPS_BLTSTRETCHYN		0x00020000
#define DDFXCAPS_OVERLAYARITHSTRETCHY	0x00040000
#define DDFXCAPS_OVERLAYARITHSTRETCHYN	0x00000008
#define DDFXCAPS_OVERLAYSHRINKX		0x00080000
#define DDFXCAPS_OVERLAYSHRINKXN	0x00100000
#define DDFXCAPS_OVERLAYSHRINKY		0x00200000
#define DDFXCAPS_OVERLAYSHRINKYN	0x00400000
#define DDFXCAPS_OVERLAYSTRETCHX	0x00800000
#define DDFXCAPS_OVERLAYSTRETCHXN	0x01000000
#define DDFXCAPS_OVERLAYSTRETCHY	0x02000000
#define DDFXCAPS_OVERLAYSTRETCHYN	0x04000000
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT	0x08000000
#define DDFXCAPS_OVERLAYMIRRORUPDOWN	0x10000000

/* DDCAPS.dwFXAlphaCaps */
#define DDFXALPHACAPS_BLTALPHAEDGEBLEND		0x00000001
#define DDFXALPHACAPS_BLTALPHAPIXELS		0x00000002
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG		0x00000004
#define DDFXALPHACAPS_BLTALPHASURFACES		0x00000008
#define DDFXALPHACAPS_BLTALPHASURFACESNEG	0x00000010
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND	0x00000020
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS	0x00000040
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG	0x00000080
#define DDFXALPHACAPS_OVERLAYALPHASURFACES	0x00000100
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG	0x00000200

/* DDCAPS.dwPalCaps */
#define DDPCAPS_4BIT			0x00000001
#define DDPCAPS_8BITENTRIES		0x00000002
#define DDPCAPS_8BIT			0x00000004
#define DDPCAPS_INITIALIZE		0x00000008
#define DDPCAPS_PRIMARYSURFACE		0x00000010
#define DDPCAPS_PRIMARYSURFACELEFT	0x00000020
#define DDPCAPS_ALLOW256		0x00000040
#define DDPCAPS_VSYNC			0x00000080
#define DDPCAPS_1BIT			0x00000100
#define DDPCAPS_2BIT			0x00000200

/* DDCAPS.dwSVCaps */
#define DDSVCAPS_ENIGMA			0x00000001l
#define DDSVCAPS_FLICKER		0x00000002l
#define DDSVCAPS_REDBLUE		0x00000004l
#define DDSVCAPS_SPLIT			0x00000008l

/* BitDepths */
#define DDBD_1				0x00004000
#define DDBD_2				0x00002000
#define DDBD_4				0x00001000
#define DDBD_8				0x00000800
#define DDBD_16				0x00000400
#define DDBD_24				0x00000200
#define DDBD_32				0x00000100

/* DDOVERLAYFX.dwDDFX */
#define DDOVERFX_ARITHSTRETCHY		0x00000001
#define DDOVERFX_MIRRORLEFTRIGHT	0x00000002
#define DDOVERFX_MIRRORUPDOWN		0x00000004

/* DDCOLORKEY.dwFlags */
#define DDPF_ALPHAPIXELS		0x00000001
#define DDPF_ALPHA			0x00000002
#define DDPF_FOURCC			0x00000004
#define DDPF_PALETTEINDEXED4		0x00000008
#define DDPF_PALETTEINDEXEDTO8		0x00000010
#define DDPF_PALETTEINDEXED8		0x00000020
#define DDPF_RGB			0x00000040
#define DDPF_COMPRESSED			0x00000080
#define DDPF_RGBTOYUV			0x00000100
#define DDPF_YUV			0x00000200
#define DDPF_ZBUFFER			0x00000400
#define DDPF_PALETTEINDEXED1		0x00000800
#define DDPF_PALETTEINDEXED2		0x00001000
#define DDPF_ZPIXELS			0x00002000

/* SetCooperativeLevel dwFlags */
#define DDSCL_FULLSCREEN		0x00000001
#define DDSCL_ALLOWREBOOT		0x00000002
#define DDSCL_NOWINDOWCHANGES		0x00000004
#define DDSCL_NORMAL			0x00000008
#define DDSCL_EXCLUSIVE			0x00000010
#define DDSCL_ALLOWMODEX		0x00000040
#define DDSCL_SETFOCUSWINDOW		0x00000080
#define DDSCL_SETDEVICEWINDOW		0x00000100
#define DDSCL_CREATEDEVICEWINDOW	0x00000200


/* DDSURFACEDESC.dwFlags */
#define	DDSD_CAPS		0x00000001
#define	DDSD_HEIGHT		0x00000002
#define	DDSD_WIDTH		0x00000004
#define	DDSD_PITCH		0x00000008
#define	DDSD_BACKBUFFERCOUNT	0x00000020
#define	DDSD_ZBUFFERBITDEPTH	0x00000040
#define	DDSD_ALPHABITDEPTH	0x00000080
#define	DDSD_LPSURFACE		0x00000800
#define	DDSD_PIXELFORMAT	0x00001000
#define	DDSD_CKDESTOVERLAY	0x00002000
#define	DDSD_CKDESTBLT		0x00004000
#define	DDSD_CKSRCOVERLAY	0x00008000
#define	DDSD_CKSRCBLT		0x00010000
#define	DDSD_MIPMAPCOUNT	0x00020000
#define	DDSD_REFRESHRATE	0x00040000
#define	DDSD_LINEARSIZE		0x00080000
#define	DDSD_ALL		0x000ff9ee

/* SetDisplayMode flags */
#define DDSDM_STANDARDVGAMODE	0x00000001

/* EnumDisplayModes flags */
#define DDEDM_REFRESHRATES	0x00000001
#define DDEDM_STANDARDVGAMODES	0x00000002


typedef struct _DDSURFACEDESC
{
	DWORD	dwSize;		/* 0: size of the DDSURFACEDESC structure*/
	DWORD	dwFlags;	/* 4: determines what fields are valid*/
	DWORD	dwHeight;	/* 8: height of surface to be created*/
	DWORD	dwWidth;	/* C: width of input surface*/
	LONG	lPitch;		/*10: distance to start of next line (return value only)*/
	DWORD	dwBackBufferCount;/* 14: number of back buffers requested*/
	union {
		DWORD	dwMipMapCount;/* 18:number of mip-map levels requested*/
		DWORD	dwZBufferBitDepth;/*18: depth of Z buffer requested*/
		DWORD	dwRefreshRate;/* 18:refresh rate (used when display mode is described)*/
	} x;		
	DWORD	dwAlphaBitDepth;/* 1C:depth of alpha buffer requested*/
	DWORD	dwReserved;	/* 20:reserved*/
	union {
		LPVOID	lpSurface;	/* 24:pointer to the associated surface memory*/
		DWORD	dwLinearSize;	/* 24:Formless late-allocated optimized surface size*/
	} y;
	DDCOLORKEY	ddckCKDestOverlay;/* 28: CK for dest overlay use*/
	DDCOLORKEY	ddckCKDestBlt;	/* 30: CK for destination blt use*/
	DDCOLORKEY	ddckCKSrcOverlay;/* 38: CK for source overlay use*/
	DDCOLORKEY	ddckCKSrcBlt;	/* 40: CK for source blt use*/
	DDPIXELFORMAT	ddpfPixelFormat;/* 48: pixel format description of the surface*/
	DDSCAPS		ddsCaps;	/* 68: direct draw surface caps */
} DDSURFACEDESC,*LPDDSURFACEDESC;

/* DDCOLORCONTROL.dwFlags */
#define DDCOLOR_BRIGHTNESS	0x00000001
#define DDCOLOR_CONTRAST	0x00000002
#define DDCOLOR_HUE		0x00000004
#define DDCOLOR_SATURATION	0x00000008
#define DDCOLOR_SHARPNESS	0x00000010
#define DDCOLOR_GAMMA		0x00000020
#define DDCOLOR_COLORENABLE	0x00000040

typedef struct {
	DWORD	dwSize;
	DWORD	dwFlags;
	LONG	lBrightness;
	LONG	lContrast;
	LONG	lHue;
	LONG	lSaturation;
	LONG	lSharpness;
	LONG	lGamma;
	LONG	lColorEnable;
	DWORD	dwReserved1;
} DDCOLORCONTROL,*LPDDCOLORCONTROL;

typedef BOOL32 (CALLBACK * LPDDENUMCALLBACK32A)(GUID *, LPSTR, LPSTR, LPVOID);
typedef BOOL32 (CALLBACK * LPDDENUMCALLBACK32W)(GUID *, LPWSTR, LPWSTR, LPVOID);
DECL_WINELIB_TYPE_AW(LPDDENUMCALLBACK);

typedef HRESULT (CALLBACK * LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);
typedef HRESULT (CALLBACK * LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);

typedef HANDLE32 HMONITOR;
typedef BOOL32 (CALLBACK * LPDDENUMCALLBACKEX32A)(GUID *, LPSTR, LPSTR, LPVOID, HMONITOR);
typedef BOOL32 (CALLBACK * LPDDENUMCALLBACKEX32W)(GUID *, LPWSTR, LPWSTR, LPVOID, HMONITOR);
DECL_WINELIB_TYPE_AW(LPDDENUMCALLBACKEX);

HRESULT WINAPI DirectDrawEnumerateExA( LPDDENUMCALLBACKEX32A lpCallback, LPVOID lpContext, DWORD dwFlags);
HRESULT WINAPI DirectDrawEnumerateExW( LPDDENUMCALLBACKEX32W lpCallback, LPVOID lpContext, DWORD dwFlags);

/* flags for DirectDrawEnumerateEx */
#define DDENUM_ATTACHEDSECONDARYDEVICES	0x00000001
#define DDENUM_DETACHEDSECONDARYDEVICES	0x00000002
#define DDENUM_NONDISPLAYDEVICES	0x00000004

typedef struct _DDBLTFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwDDFX;                         /* FX operations */
    DWORD       dwROP;                          /* Win32 raster operations */
    DWORD       dwDDROP;                        /* Raster operations new for DirectDraw */
    DWORD       dwRotationAngle;                /* Rotation angle for blt */
    DWORD       dwZBufferOpCode;                /* ZBuffer compares */
    DWORD       dwZBufferLow;                   /* Low limit of Z buffer */
    DWORD       dwZBufferHigh;                  /* High limit of Z buffer */
    DWORD       dwZBufferBaseDest;              /* Destination base value */
    DWORD       dwZDestConstBitDepth;           /* Bit depth used to specify Z constant for destination */
    union
    {
        DWORD   dwZDestConst;                   /* Constant to use as Z buffer for dest */
        LPDIRECTDRAWSURFACE lpDDSZBufferDest;   /* Surface to use as Z buffer for dest */
    } x;
    DWORD       dwZSrcConstBitDepth;            /* Bit depth used to specify Z constant for source */
    union
    {
        DWORD   dwZSrcConst;                    /* Constant to use as Z buffer for src */
        LPDIRECTDRAWSURFACE lpDDSZBufferSrc;    /* Surface to use as Z buffer for src */
    } y;
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Alpha for edge blending */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as Alpha Channel */
    } z;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as Alpha Channel */
    } a;
    union
    {
        DWORD   dwFillColor;                    /* color in RGB or Palettized */
        DWORD   dwFillDepth;                    /* depth value for z-buffer */
	DWORD   dwFillPixel;			/* pixel val for RGBA or RGBZ */
        LPDIRECTDRAWSURFACE lpDDSPattern;       /* Surface to use as pattern */
    } b;
    DDCOLORKEY  ddckDestColorkey;               /* DestColorkey override */
    DDCOLORKEY  ddckSrcColorkey;                /* SrcColorkey override */
} DDBLTFX,*LPDDBLTFX;

/* dwDDFX */
/* arithmetic stretching along y axis */
#define DDBLTFX_ARITHSTRETCHY			0x00000001
/* mirror on y axis */
#define DDBLTFX_MIRRORLEFTRIGHT			0x00000002
/* mirror on x axis */
#define DDBLTFX_MIRRORUPDOWN			0x00000004
/* do not tear */
#define DDBLTFX_NOTEARING			0x00000008
/* 180 degrees clockwise rotation */
#define DDBLTFX_ROTATE180			0x00000010
/* 270 degrees clockwise rotation */
#define DDBLTFX_ROTATE270			0x00000020
/* 90 degrees clockwise rotation */
#define DDBLTFX_ROTATE90			0x00000040
/* dwZBufferLow and dwZBufferHigh specify limits to the copied Z values */
#define DDBLTFX_ZBUFFERRANGE			0x00000080
/* add dwZBufferBaseDest to every source z value before compare */
#define DDBLTFX_ZBUFFERBASEDEST			0x00000100

typedef struct _DDOVERLAYFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Constant to use as alpha for edge blend */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as alpha channel for dest */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as alpha channel for dest */
    } x;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as alpha channel for src */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as alpha channel for src */
    } y;
    DDCOLORKEY  dckDestColorkey;                /* DestColorkey override */
    DDCOLORKEY  dckSrcColorkey;                 /* DestColorkey override */
    DWORD       dwDDFX;                         /* Overlay FX */
    DWORD       dwFlags;                        /* flags */
} DDOVERLAYFX,*LPDDOVERLAYFX;

typedef struct _DDBLTBATCH
{
    LPRECT32		lprDest;
    LPDIRECTDRAWSURFACE	lpDDSSrc;
    LPRECT32		lprSrc;
    DWORD		dwFlags;
    LPDDBLTFX		lpDDBltFx;
} DDBLTBATCH,*LPDDBLTBATCH;

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS ,

#define THIS LPDIRECTDRAWPALETTE this

typedef struct IDirectDrawPalette_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawPalette methods ***/
    STDMETHOD(GetCaps)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD, LPPALETTEENTRY) PURE;
    STDMETHOD(SetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
} *LPDIRECTDRAWPALETTE_VTABLE,IDirectDrawPalette_VTable;

struct IDirectDrawPalette {
    LPDIRECTDRAWPALETTE_VTABLE	lpvtbl;
    DWORD			ref;
    LPDIRECTDRAW		ddraw;
    Colormap			cm;
    PALETTEENTRY		palents[256];
};
#undef THIS

#define THIS LPDIRECTDRAWCLIPPER this
typedef struct IDirectDrawClipper_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawClipper methods ***/
    STDMETHOD(GetClipList)(THIS_ LPRECT32, LPRGNDATA, LPDWORD) PURE;
    STDMETHOD(GetHWnd)(THIS_ HWND32 FAR *) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD) PURE;
    STDMETHOD(IsClipListChanged)(THIS_ BOOL32 FAR *) PURE;
    STDMETHOD(SetClipList)(THIS_ LPRGNDATA,DWORD) PURE;
    STDMETHOD(SetHWnd)(THIS_ DWORD, HWND32 ) PURE;
} *LPDIRECTDRAWCLIPPER_VTABLE,IDirectDrawClipper_VTable;

struct IDirectDrawClipper {
   LPDIRECTDRAWCLIPPER_VTABLE lpvtbl;
   DWORD ref;
};
#undef THIS

#define THIS LPDIRECTDRAW this
typedef struct IDirectDraw_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *,
IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE
FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ BOOL32* ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND32, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD , DWORD ,DWORD ) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE32 ) PURE;
} *LPDIRECTDRAW_VTABLE,IDirectDraw_VTable;

struct _directdrawdata {
    DWORD			depth;
    DWORD			vp_width,vp_height; /* viewport dimension */
    DWORD			height,width;	/* SetDisplayMode */
    DWORD			fb_width,fb_height,fb_banksize,fb_memsize;
    HWND32			mainwindow;
    void			*fb_addr;
    unsigned int		vpmask;
};


struct IDirectDraw {
	LPDIRECTDRAW_VTABLE	lpvtbl;
	DWORD			ref;
	struct _directdrawdata	d;
};
#undef THIS

/* flags for Lock() */
#define DDLOCK_SURFACEMEMORYPTR	0x00000000
#define DDLOCK_WAIT		0x00000001
#define DDLOCK_EVENT		0x00000002
#define DDLOCK_READONLY		0x00000010
#define DDLOCK_WRITEONLY	0x00000020
#define DDLOCK_NOSYSLOCK	0x00000800


#define THIS LPDIRECTDRAW2 this
typedef struct IDirectDraw2_VTable
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *,
IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE
FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ BOOL32* ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND32, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD, DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE32 ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
} IDirectDraw2_VTable,*LPDIRECTDRAW2_VTABLE;
/* MUST HAVE THE SAME LAYOUT AS struct IDirectDraw */

struct IDirectDraw2 {
    LPDIRECTDRAW2_VTABLE	lpvtbl;
    DWORD			ref;
    struct _directdrawdata	d;
};
#undef THIS

#define THIS LPDIRECTDRAWSURFACE this
struct _directdrawsurface {
    LPVOID		surface;
    LPDIRECTDRAWPALETTE	palette;
    DWORD		fb_height,lpitch,width,height;
    LPDIRECTDRAW	ddraw;
    LPDIRECTDRAWSURFACE	backbuffer;
};

typedef struct IDirectDrawSurface_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT32) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT32,LPDIRECTDRAWSURFACE, LPRECT32,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT32,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC32 FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT32,LPDDSURFACEDESC,DWORD flags,HANDLE32) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC32) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT32, LPDIRECTDRAWSURFACE,LPRECT32,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE) PURE;
} *LPDIRECTDRAWSURFACE_VTABLE,IDirectDrawSurface_VTable;

struct IDirectDrawSurface {
    LPDIRECTDRAWSURFACE_VTABLE	lpvtbl;
    DWORD			ref;
    struct _directdrawsurface	s;
};
#undef THIS
#define THIS LPDIRECTDRAWSURFACE2 this

typedef struct IDirectDrawSurface2_VTable {
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT32) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT32,LPDIRECTDRAWSURFACE2, LPRECT32,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE2, LPRECT32,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE2, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE2 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC32 FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT32,LPDDSURFACEDESC,DWORD,HANDLE32) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC32) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT32, LPDIRECTDRAWSURFACE2,LPRECT32,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE2) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
} *LPDIRECTDRAWSURFACE2_VTABLE,IDirectDrawSurface2_VTable;

struct IDirectDrawSurface2 {
    LPDIRECTDRAWSURFACE2_VTABLE	lpvtbl;
    DWORD			ref;
    struct _directdrawsurface	s;
};
#undef THIS
#define THIS LPDIRECTDRAWSURFACE3 this

typedef struct IDirectDrawSurface3_VTable {
    /*** IUnknown methods ***/
/*00*/STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
/*04*/STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
/*08*/STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
/*0c*/STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE3) PURE;
/*10*/STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT32) PURE;
/*14*/STDMETHOD(Blt)(THIS_ LPRECT32,LPDIRECTDRAWSURFACE3, LPRECT32,DWORD, LPDDBLTFX) PURE;
/*18*/STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
/*1c*/STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE3, LPRECT32,DWORD) PURE;
/*20*/STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE3) PURE;
/*24*/STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;    
/*28*/STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
/*2c*/STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE3, DWORD) PURE;
/*30*/STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE3 FAR *) PURE;
/*34*/STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
/*38*/STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
/*3c*/STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
/*40*/STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
/*44*/STDMETHOD(GetDC)(THIS_ HDC32 FAR *) PURE;
/*48*/STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
/*4c*/STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
/*50*/STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
/*54*/STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
/*58*/STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
/*5c*/STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
/*60*/STDMETHOD(IsLost)(THIS) PURE;
/*64*/STDMETHOD(Lock)(THIS_ LPRECT32,LPDDSURFACEDESC,DWORD,HANDLE32) PURE;
/*68*/STDMETHOD(ReleaseDC)(THIS_ HDC32) PURE;
/*6c*/STDMETHOD(Restore)(THIS) PURE;
/*70*/STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
/*74*/STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
/*78*/STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
/*7c*/STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
/*80*/STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
/*84*/STDMETHOD(UpdateOverlay)(THIS_ LPRECT32, LPDIRECTDRAWSURFACE3,LPRECT32,DWORD, LPDDOVERLAYFX) PURE;
/*88*/STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
/*8c*/STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE3) PURE;
    /*** Added in the v2 interface ***/
/*90*/STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
/*94*/STDMETHOD(PageLock)(THIS_ DWORD) PURE;
/*98*/STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
    /*** Added in the V3 interface ***/
/*9c*/STDMETHOD(SetSurfaceDesc)(THIS_ LPDDSURFACEDESC, DWORD) PURE;
} *LPDIRECTDRAWSURFACE3_VTABLE,IDirectDrawSurface3_VTable;

struct IDirectDrawSurface3 {
    LPDIRECTDRAWSURFACE3_VTABLE	lpvtbl;
    DWORD			ref;
    struct _directdrawsurface	s;
};
#undef THIS

#define THIS LPDIRECTDRAWCOLORCONTROL this
typedef struct IDirectDrawColorControl_VTable {
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;
	/*** IDirectDrawColorControl methods ***/
	STDMETHOD(GetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
	STDMETHOD(SetColorControls)(THIS_ LPDDCOLORCONTROL) PURE;
} IDirectDrawColorControl_VTable,*LPDIRECTDRAWCOLORCONTROL_VTABLE;

struct IDirectDrawColorControl  {
	LPDIRECTDRAWCOLORCONTROL_VTABLE	lpvtbl;
	DWORD	ref;
};
#undef THIS

#undef THIS_
#undef PURE
#undef FAR
#undef STDMETHOD
#undef STDMETHOD_

extern HRESULT WINAPI DirectDrawCreate( LPGUID lpGUID,LPDIRECTDRAW *lplpDD,LPUNKNOWN pUnkOuter );
#endif
