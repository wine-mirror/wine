#ifndef __WINE_D3D_H
#define __WINE_D3D_H

#include "ddraw.h"

/* This is needed for GL_LIGHT */
#include "wine_gl.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IDirect3D,		0x3BBA0080,0x2421,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID(IID_IDirect3D2,		0x6aae1ec1,0x662a,0x11d0,0x88,0x9d,0x00,0xaa,0x00,0xbb,0xb7,0x6a);

DEFINE_GUID(IID_IDirect3DRampDevice,	0xF2086B20,0x259F,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID(IID_IDirect3DRGBDevice,	0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID(IID_IDirect3DHALDevice,	0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID(IID_IDirect3DMMXDevice,	0x881949a1,0xd6f3,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29 );

DEFINE_GUID(IID_IDirect3DDevice,	0x64108800,0x957d,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29 );
DEFINE_GUID(IID_IDirect3DDevice2,	0x93281501,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DTexture,	0x2CDCD9E0,0x25A0,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirect3DTexture2,	0x93281502,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DLight,		0x4417C142,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DMaterial,	0x4417C144,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DMaterial2,	0x93281503,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DExecuteBuffer,	0x4417C145,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DViewport,	0x4417C146,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DViewport2,	0x93281500,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);

typedef struct IDirect3D	IDirect3D ,*LPDIRECT3D ;
typedef struct IDirect3D2	IDirect3D2,*LPDIRECT3D2;
typedef struct IDirect3DLight	IDirect3DLight,*LPDIRECT3DLIGHT;
typedef struct IDirect3DDevice        IDirect3DDevice, *LPDIRECT3DDEVICE;
typedef struct IDirect3DDevice2       IDirect3DDevice2, *LPDIRECT3DDEVICE2;
typedef struct IDirect3DViewport2     IDirect3DViewport, IDirect3DViewport2, *LPDIRECT3DVIEWPORT2, *LPDIRECT3DVIEWPORT;
typedef struct IDirect3DMaterial2     IDirect3DMaterial, *LPDIRECT3DMATERIAL, IDirect3DMaterial2, *LPDIRECT3DMATERIAL2;
typedef struct IDirect3DTexture2      IDirect3DTexture, *LPDIRECT3DTEXTURE, IDirect3DTexture2,  *LPDIRECT3DTEXTURE2;
typedef struct IDirect3DExecuteBuffer IDirect3DExecuteBuffer, *LPDIRECT3DEXECUTEBUFFER;


/* ********************************************************************
   Error Codes
   ******************************************************************** */
#define D3D_OK                          DD_OK
#define D3DERR_BADMAJORVERSION          MAKE_DDHRESULT(700)
#define D3DERR_BADMINORVERSION          MAKE_DDHRESULT(701)
#define D3DERR_INVALID_DEVICE           MAKE_DDHRESULT(705)
#define D3DERR_INITFAILED               MAKE_DDHRESULT(706)
#define D3DERR_DEVICEAGGREGATED         MAKE_DDHRESULT(707)
#define D3DERR_EXECUTE_CREATE_FAILED    MAKE_DDHRESULT(710)
#define D3DERR_EXECUTE_DESTROY_FAILED   MAKE_DDHRESULT(711)
#define D3DERR_EXECUTE_LOCK_FAILED      MAKE_DDHRESULT(712)
#define D3DERR_EXECUTE_UNLOCK_FAILED    MAKE_DDHRESULT(713)
#define D3DERR_EXECUTE_LOCKED           MAKE_DDHRESULT(714)
#define D3DERR_EXECUTE_NOT_LOCKED       MAKE_DDHRESULT(715)
#define D3DERR_EXECUTE_FAILED           MAKE_DDHRESULT(716)
#define D3DERR_EXECUTE_CLIPPED_FAILED   MAKE_DDHRESULT(717)
#define D3DERR_TEXTURE_NO_SUPPORT       MAKE_DDHRESULT(720)
#define D3DERR_TEXTURE_CREATE_FAILED    MAKE_DDHRESULT(721)
#define D3DERR_TEXTURE_DESTROY_FAILED   MAKE_DDHRESULT(722)
#define D3DERR_TEXTURE_LOCK_FAILED      MAKE_DDHRESULT(723)
#define D3DERR_TEXTURE_UNLOCK_FAILED    MAKE_DDHRESULT(724)
#define D3DERR_TEXTURE_LOAD_FAILED      MAKE_DDHRESULT(725)
#define D3DERR_TEXTURE_SWAP_FAILED      MAKE_DDHRESULT(726)
#define D3DERR_TEXTURE_LOCKED           MAKE_DDHRESULT(727)
#define D3DERR_TEXTURE_NOT_LOCKED       MAKE_DDHRESULT(728)
#define D3DERR_TEXTURE_GETSURF_FAILED   MAKE_DDHRESULT(729)
#define D3DERR_MATRIX_CREATE_FAILED     MAKE_DDHRESULT(730)
#define D3DERR_MATRIX_DESTROY_FAILED    MAKE_DDHRESULT(731)
#define D3DERR_MATRIX_SETDATA_FAILED    MAKE_DDHRESULT(732)
#define D3DERR_MATRIX_GETDATA_FAILED    MAKE_DDHRESULT(733)
#define D3DERR_SETVIEWPORTDATA_FAILED   MAKE_DDHRESULT(734)
#define D3DERR_INVALIDCURRENTVIEWPORT   MAKE_DDHRESULT(735)
#define D3DERR_INVALIDPRIMITIVETYPE     MAKE_DDHRESULT(736)
#define D3DERR_INVALIDVERTEXTYPE        MAKE_DDHRESULT(737)
#define D3DERR_TEXTURE_BADSIZE          MAKE_DDHRESULT(738)
#define D3DERR_INVALIDRAMPTEXTURE       MAKE_DDHRESULT(739)
#define D3DERR_MATERIAL_CREATE_FAILED   MAKE_DDHRESULT(740)
#define D3DERR_MATERIAL_DESTROY_FAILED  MAKE_DDHRESULT(741)
#define D3DERR_MATERIAL_SETDATA_FAILED  MAKE_DDHRESULT(742)
#define D3DERR_MATERIAL_GETDATA_FAILED  MAKE_DDHRESULT(743)
#define D3DERR_INVALIDPALETTE           MAKE_DDHRESULT(744)
#define D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY MAKE_DDHRESULT(745)
#define D3DERR_ZBUFF_NEEDS_VIDEOMEMORY  MAKE_DDHRESULT(746)
#define D3DERR_SURFACENOTINVIDMEM       MAKE_DDHRESULT(747)
#define D3DERR_LIGHT_SET_FAILED         MAKE_DDHRESULT(750)
#define D3DERR_LIGHTHASVIEWPORT         MAKE_DDHRESULT(751)
#define D3DERR_LIGHTNOTINTHISVIEWPORT   MAKE_DDHRESULT(752)
#define D3DERR_SCENE_IN_SCENE           MAKE_DDHRESULT(760)
#define D3DERR_SCENE_NOT_IN_SCENE       MAKE_DDHRESULT(761)
#define D3DERR_SCENE_BEGIN_FAILED       MAKE_DDHRESULT(762)
#define D3DERR_SCENE_END_FAILED         MAKE_DDHRESULT(763)
#define D3DERR_INBEGIN                  MAKE_DDHRESULT(770)
#define D3DERR_NOTINBEGIN               MAKE_DDHRESULT(771)
#define D3DERR_NOVIEWPORTS              MAKE_DDHRESULT(772)
#define D3DERR_VIEWPORTDATANOTSET       MAKE_DDHRESULT(773)
#define D3DERR_VIEWPORTHASNODEVICE      MAKE_DDHRESULT(774)
#define D3DERR_NOCURRENTVIEWPORT        MAKE_DDHRESULT(775)

/* ********************************************************************
   Enums
   ******************************************************************** */
#define D3DNEXT_NEXT 0x01l
#define D3DNEXT_HEAD 0x02l
#define D3DNEXT_TAIL 0x04l

typedef enum {
  D3DLIGHT_POINT          = 1,
  D3DLIGHT_SPOT           = 2,
  D3DLIGHT_DIRECTIONAL    = 3,
  D3DLIGHT_PARALLELPOINT  = 4,
  D3DLIGHT_FORCE_DWORD    = 0x7fffffff
} D3DLIGHTTYPE;

typedef enum {
  D3DPT_POINTLIST     = 1,
  D3DPT_LINELIST      = 2,
  D3DPT_LINESTRIP     = 3,
  D3DPT_TRIANGLELIST  = 4,
  D3DPT_TRIANGLESTRIP = 5,
  D3DPT_TRIANGLEFAN   = 6,
  D3DPT_FORCE_DWORD   = 0x7fffffff
} D3DPRIMITIVETYPE;

typedef enum { 
  D3DFILL_POINT         = 1, 
  D3DFILL_WIREFRAME     = 2, 
  D3DFILL_SOLID         = 3,
  D3DFILL_FORCE_DWORD   = 0x7fffffff
} D3DFILLMODE; 

typedef enum { 
  D3DSHADE_FLAT         = 1, 
  D3DSHADE_GOURAUD      = 2, 
  D3DSHADE_PHONG        = 3, 
  D3DSHADE_FORCE_DWORD  = 0x7fffffff
} D3DSHADEMODE;

typedef enum { 
  D3DCULL_NONE        = 1, 
  D3DCULL_CW          = 2, 
  D3DCULL_CCW         = 3, 
  D3DCULL_FORCE_DWORD = 0x7fffffff 
} D3DCULL; 

typedef enum { 
  D3DBLEND_ZERO            = 1, 
  D3DBLEND_ONE             = 2, 
  D3DBLEND_SRCCOLOR        = 3, 
  D3DBLEND_INVSRCCOLOR     = 4, 
  D3DBLEND_SRCALPHA        = 5, 
  D3DBLEND_INVSRCALPHA     = 6, 
  D3DBLEND_DESTALPHA       = 7, 
  D3DBLEND_INVDESTALPHA    = 8, 
  D3DBLEND_DESTCOLOR       = 9, 
  D3DBLEND_INVDESTCOLOR    = 10, 
  D3DBLEND_SRCALPHASAT     = 11, 
  D3DBLEND_BOTHSRCALPHA    = 12, 
  D3DBLEND_BOTHINVSRCALPHA = 13, 
  D3DBLEND_FORCE_DWORD     = 0x7fffffff
} D3DBLEND; 

typedef enum { 
  D3DTBLEND_DECAL         = 1, 
  D3DTBLEND_MODULATE      = 2, 
  D3DTBLEND_DECALALPHA    = 3, 
  D3DTBLEND_MODULATEALPHA = 4, 
  D3DTBLEND_DECALMASK     = 5, 
  D3DTBLEND_MODULATEMASK  = 6, 
  D3DTBLEND_COPY          = 7, 
  D3DTBLEND_ADD           = 8, 
  D3DTBLEND_FORCE_DWORD   = 0x7fffffff
} D3DTEXTUREBLEND;

typedef enum { 
  D3DFILTER_NEAREST          = 1, 
  D3DFILTER_LINEAR           = 2, 
  D3DFILTER_MIPNEAREST       = 3, 
  D3DFILTER_MIPLINEAR        = 4, 
  D3DFILTER_LINEARMIPNEAREST = 5, 
  D3DFILTER_LINEARMIPLINEAR  = 6, 
  D3DFILTER_FORCE_DWORD      = 0x7fffffff
} D3DTEXTUREFILTER;

typedef enum {
  D3DRENDERSTATE_TEXTUREHANDLE      = 1,    /* Texture handle */
  D3DRENDERSTATE_ANTIALIAS          = 2,    /* D3DANTIALIASMODE */
  D3DRENDERSTATE_TEXTUREADDRESS     = 3,    /* D3DTEXTUREADDRESS      */
  D3DRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
  D3DRENDERSTATE_WRAPU              = 5,    /* TRUE for wrapping in u */
  D3DRENDERSTATE_WRAPV              = 6,    /* TRUE for wrapping in v */
  D3DRENDERSTATE_ZENABLE            = 7,    /* TRUE to enable z test */
  D3DRENDERSTATE_FILLMODE           = 8,    /* D3DFILL_MODE            */
  D3DRENDERSTATE_SHADEMODE          = 9,    /* D3DSHADEMODE */
  D3DRENDERSTATE_LINEPATTERN        = 10,   /* D3DLINEPATTERN */
  D3DRENDERSTATE_MONOENABLE         = 11,   /* TRUE to enable mono rasterization */
  D3DRENDERSTATE_ROP2               = 12,   /* ROP2 */
  D3DRENDERSTATE_PLANEMASK          = 13,   /* DWORD physical plane mask */
  D3DRENDERSTATE_ZWRITEENABLE       = 14,   /* TRUE to enable z writes */
  D3DRENDERSTATE_ALPHATESTENABLE    = 15,   /* TRUE to enable alpha tests */
  D3DRENDERSTATE_LASTPIXEL          = 16,   /* TRUE for last-pixel on lines */
  D3DRENDERSTATE_TEXTUREMAG         = 17,   /* D3DTEXTUREFILTER */
  D3DRENDERSTATE_TEXTUREMIN         = 18,   /* D3DTEXTUREFILTER */
  D3DRENDERSTATE_SRCBLEND           = 19,   /* D3DBLEND */
  D3DRENDERSTATE_DESTBLEND          = 20,   /* D3DBLEND */
  D3DRENDERSTATE_TEXTUREMAPBLEND    = 21,   /* D3DTEXTUREBLEND */
  D3DRENDERSTATE_CULLMODE           = 22,   /* D3DCULL */
  D3DRENDERSTATE_ZFUNC              = 23,   /* D3DCMPFUNC */
  D3DRENDERSTATE_ALPHAREF           = 24,   /* D3DFIXED */
  D3DRENDERSTATE_ALPHAFUNC          = 25,   /* D3DCMPFUNC */
  D3DRENDERSTATE_DITHERENABLE       = 26,   /* TRUE to enable dithering */
  D3DRENDERSTATE_ALPHABLENDENABLE   = 27,   /* TRUE to enable alpha blending */
  D3DRENDERSTATE_FOGENABLE          = 28,   /* TRUE to enable fog */
  D3DRENDERSTATE_SPECULARENABLE     = 29,   /* TRUE to enable specular */
  D3DRENDERSTATE_ZVISIBLE           = 30,   /* TRUE to enable z checking */
  D3DRENDERSTATE_SUBPIXEL           = 31,   /* TRUE to enable subpixel correction */
  D3DRENDERSTATE_SUBPIXELX          = 32,   /* TRUE to enable correction in X only */
  D3DRENDERSTATE_STIPPLEDALPHA      = 33,   /* TRUE to enable stippled alpha */
  D3DRENDERSTATE_FOGCOLOR           = 34,   /* D3DCOLOR */
  D3DRENDERSTATE_FOGTABLEMODE       = 35,   /* D3DFOGMODE */
  D3DRENDERSTATE_FOGTABLESTART      = 36,   /* Fog table start        */
  D3DRENDERSTATE_FOGTABLEEND        = 37,   /* Fog table end          */
  D3DRENDERSTATE_FOGTABLEDENSITY    = 38,   /* Fog table density      */
  D3DRENDERSTATE_STIPPLEENABLE      = 39,   /* TRUE to enable stippling */
  D3DRENDERSTATE_EDGEANTIALIAS      = 40,   /* TRUE to enable edge antialiasing */
  D3DRENDERSTATE_COLORKEYENABLE     = 41,   /* TRUE to enable source colorkeyed textures */
  D3DRENDERSTATE_BORDERCOLOR        = 43,   /* Border color for texturing w/border */
  D3DRENDERSTATE_TEXTUREADDRESSU    = 44,   /* Texture addressing mode for U coordinate */
  D3DRENDERSTATE_TEXTUREADDRESSV    = 45,   /* Texture addressing mode for V coordinate */
  D3DRENDERSTATE_MIPMAPLODBIAS      = 46,   /* D3DVALUE Mipmap LOD bias */
  D3DRENDERSTATE_ZBIAS              = 47,   /* LONG Z bias */
  D3DRENDERSTATE_RANGEFOGENABLE     = 48,   /* Enables range-based fog */
  D3DRENDERSTATE_ANISOTROPY         = 49,   /* Max. anisotropy. 1 = no anisotropy */
  D3DRENDERSTATE_FLUSHBATCH         = 50,   /* Explicit flush for DP batching (DX5 Only) */
  D3DRENDERSTATE_STIPPLEPATTERN00   = 64,   /* Stipple pattern 01...  */
  D3DRENDERSTATE_STIPPLEPATTERN01   = 65,
  D3DRENDERSTATE_STIPPLEPATTERN02   = 66,
  D3DRENDERSTATE_STIPPLEPATTERN03   = 67,
  D3DRENDERSTATE_STIPPLEPATTERN04   = 68,
  D3DRENDERSTATE_STIPPLEPATTERN05   = 69,
  D3DRENDERSTATE_STIPPLEPATTERN06   = 70,
  D3DRENDERSTATE_STIPPLEPATTERN07   = 71,
  D3DRENDERSTATE_STIPPLEPATTERN08   = 72,
  D3DRENDERSTATE_STIPPLEPATTERN09   = 73,
  D3DRENDERSTATE_STIPPLEPATTERN10   = 74,
  D3DRENDERSTATE_STIPPLEPATTERN11   = 75,
  D3DRENDERSTATE_STIPPLEPATTERN12   = 76,
  D3DRENDERSTATE_STIPPLEPATTERN13   = 77,
  D3DRENDERSTATE_STIPPLEPATTERN14   = 78,
  D3DRENDERSTATE_STIPPLEPATTERN15   = 79,
  D3DRENDERSTATE_STIPPLEPATTERN16   = 80,
  D3DRENDERSTATE_STIPPLEPATTERN17   = 81,
  D3DRENDERSTATE_STIPPLEPATTERN18   = 82,
  D3DRENDERSTATE_STIPPLEPATTERN19   = 83,
  D3DRENDERSTATE_STIPPLEPATTERN20   = 84,
  D3DRENDERSTATE_STIPPLEPATTERN21   = 85,
  D3DRENDERSTATE_STIPPLEPATTERN22   = 86,
  D3DRENDERSTATE_STIPPLEPATTERN23   = 87,
  D3DRENDERSTATE_STIPPLEPATTERN24   = 88,
  D3DRENDERSTATE_STIPPLEPATTERN25   = 89,
  D3DRENDERSTATE_STIPPLEPATTERN26   = 90,
  D3DRENDERSTATE_STIPPLEPATTERN27   = 91,
  D3DRENDERSTATE_STIPPLEPATTERN28   = 92,
  D3DRENDERSTATE_STIPPLEPATTERN29   = 93,
  D3DRENDERSTATE_STIPPLEPATTERN30   = 94,
  D3DRENDERSTATE_STIPPLEPATTERN31   = 95,
  D3DRENDERSTATE_FORCE_DWORD        = 0x7fffffff, /* force 32-bit size enum */
} D3DRENDERSTATETYPE;

typedef enum { 
  D3DCMP_NEVER        = 1, 
  D3DCMP_LESS         = 2, 
  D3DCMP_EQUAL        = 3, 
  D3DCMP_LESSEQUAL    = 4, 
  D3DCMP_GREATER      = 5, 
  D3DCMP_NOTEQUAL     = 6, 
  D3DCMP_GREATEREQUAL = 7, 
  D3DCMP_ALWAYS       = 8, 
  D3DCMP_FORCE_DWORD  = 0x7fffffff
} D3DCMPFUNC; 

typedef enum {
  D3DLIGHTSTATE_MATERIAL      = 1,
  D3DLIGHTSTATE_AMBIENT       = 2,
  D3DLIGHTSTATE_COLORMODEL    = 3,
  D3DLIGHTSTATE_FOGMODE       = 4,
  D3DLIGHTSTATE_FOGSTART      = 5,
  D3DLIGHTSTATE_FOGEND        = 6,
  D3DLIGHTSTATE_FOGDENSITY    = 7,
  D3DLIGHTSTATE_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DLIGHTSTATETYPE;

typedef enum {
  D3DVT_VERTEX        = 1,
  D3DVT_LVERTEX       = 2,
  D3DVT_TLVERTEX      = 3,
  D3DVT_FORCE_DWORD   = 0x7fffffff, /* force 32-bit size enum */
} D3DVERTEXTYPE;

typedef enum {
  D3DTRANSFORMSTATE_WORLD           = 1,
  D3DTRANSFORMSTATE_VIEW            = 2,
  D3DTRANSFORMSTATE_PROJECTION      = 3,
  D3DTRANSFORMSTATE_FORCE_DWORD     = 0x7fffffff, /* force 32-bit size enum */
} D3DTRANSFORMSTATETYPE;

/* ********************************************************************
   Types and structures
   ******************************************************************** */
typedef DWORD D3DMATERIALHANDLE, *LPD3DMATERIALHANDLE;
typedef DWORD D3DTEXTUREHANDLE,  *LPD3DTEXTUREHANDLE;
typedef DWORD D3DVIEWPORTHANDLE, *LPD3DVIEWPORTHANDLE;
typedef DWORD D3DMATRIXHANDLE,   *LPD3DMATRIXHANDLE;

typedef DWORD D3DCOLOR, *LPD3DCOLOR;

typedef struct {
	DWORD	dwSize;
	DWORD	dwCaps;
} D3DTRANSFORMCAPS,*LPD3DTRANSFORMCAPS;

#define D3DTRANSFORMCAPS_CLIP	0x00000001

typedef struct {
	DWORD	dwSize;
	DWORD	dwCaps;
	DWORD	dwLightingModel;
	DWORD	dwNumLights;
} D3DLIGHTINGCAPS, *LPD3DLIGHTINGCAPS;

#define D3DLIGHTINGMODEL_RGB		0x00000001
#define D3DLIGHTINGMODEL_MONO		0x00000002

#define D3DLIGHTCAPS_POINT		0x00000001
#define D3DLIGHTCAPS_SPOT		0x00000002
#define D3DLIGHTCAPS_DIRECTIONAL	0x00000004
#define D3DLIGHTCAPS_PARALLELPOINT	0x00000008


#define D3DCOLOR_MONO	1
#define D3DCOLOR_RGB	2

typedef DWORD D3DCOLORMODEL;

typedef struct {
    DWORD dwSize;
    DWORD dwMiscCaps;                 /* Capability flags */
    DWORD dwRasterCaps;
    DWORD dwZCmpCaps;
    DWORD dwSrcBlendCaps;
    DWORD dwDestBlendCaps;
    DWORD dwAlphaCmpCaps;
    DWORD dwShadeCaps;
    DWORD dwTextureCaps;
    DWORD dwTextureFilterCaps;
    DWORD dwTextureBlendCaps;
    DWORD dwTextureAddressCaps;
    DWORD dwStippleWidth;             /* maximum width and height of */
    DWORD dwStippleHeight;            /* of supported stipple (up to 32x32) */
} D3DPRIMCAPS, *LPD3DPRIMCAPS;

/* D3DPRIMCAPS.dwMiscCaps */
#define D3DPMISCCAPS_MASKPLANES		0x00000001
#define D3DPMISCCAPS_MASKZ		0x00000002
#define D3DPMISCCAPS_LINEPATTERNREP	0x00000004
#define D3DPMISCCAPS_CONFORMANT		0x00000008
#define D3DPMISCCAPS_CULLNONE		0x00000010
#define D3DPMISCCAPS_CULLCW		0x00000020
#define D3DPMISCCAPS_CULLCCW		0x00000040

/* D3DPRIMCAPS.dwRasterCaps */
#define D3DPRASTERCAPS_DITHER			0x00000001
#define D3DPRASTERCAPS_ROP2			0x00000002
#define D3DPRASTERCAPS_XOR			0x00000004
#define D3DPRASTERCAPS_PAT			0x00000008
#define D3DPRASTERCAPS_ZTEST			0x00000010
#define D3DPRASTERCAPS_SUBPIXEL			0x00000020
#define D3DPRASTERCAPS_SUBPIXELX		0x00000040
#define D3DPRASTERCAPS_FOGVERTEX		0x00000080
#define D3DPRASTERCAPS_FOGTABLE			0x00000100
#define D3DPRASTERCAPS_STIPPLE			0x00000200
#define D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT	0x00000400
#define D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT	0x00000800
#define D3DPRASTERCAPS_ANTIALIASEDGES		0x00001000
#define D3DPRASTERCAPS_MIPMAPLODBIAS		0x00002000
#define D3DPRASTERCAPS_ZBIAS			0x00004000
#define D3DPRASTERCAPS_ZBUFFERLESSHSR		0x00008000
#define D3DPRASTERCAPS_FOGRANGE			0x00010000
#define D3DPRASTERCAPS_ANISOTROPY		0x00020000

/* D3DPRIMCAPS.dwZCmpCaps and dwAlphaCmpCaps */
#define D3DPCMPCAPS_NEVER		0x00000001
#define D3DPCMPCAPS_LESS		0x00000002
#define D3DPCMPCAPS_EQUAL		0x00000004
#define D3DPCMPCAPS_LESSEQUAL		0x00000008
#define D3DPCMPCAPS_GREATER		0x00000010
#define D3DPCMPCAPS_NOTEQUAL		0x00000020
#define D3DPCMPCAPS_GREATEREQUAL	0x00000040
#define D3DPCMPCAPS_ALWAYS		0x00000080

/* D3DPRIMCAPS.dwSourceBlendCaps, dwDestBlendCaps */
#define D3DPBLENDCAPS_ZERO		0x00000001
#define D3DPBLENDCAPS_ONE		0x00000002
#define D3DPBLENDCAPS_SRCCOLOR		0x00000004
#define D3DPBLENDCAPS_INVSRCCOLOR	0x00000008
#define D3DPBLENDCAPS_SRCALPHA		0x00000010
#define D3DPBLENDCAPS_INVSRCALPHA	0x00000020
#define D3DPBLENDCAPS_DESTALPHA		0x00000040
#define D3DPBLENDCAPS_INVDESTALPHA	0x00000080
#define D3DPBLENDCAPS_DESTCOLOR		0x00000100
#define D3DPBLENDCAPS_INVDESTCOLOR	0x00000200
#define D3DPBLENDCAPS_SRCALPHASAT	0x00000400
#define D3DPBLENDCAPS_BOTHSRCALPHA	0x00000800
#define D3DPBLENDCAPS_BOTHINVSRCALPHA	0x00001000

/* D3DPRIMCAPS.dwShadeCaps */
#define D3DPSHADECAPS_COLORFLATMONO	0x00000001
#define D3DPSHADECAPS_COLORFLATRGB	0x00000002
#define D3DPSHADECAPS_COLORGOURAUDMONO	0x00000004
#define D3DPSHADECAPS_COLORGOURAUDRGB	0x00000008
#define D3DPSHADECAPS_COLORPHONGMONO	0x00000010
#define D3DPSHADECAPS_COLORPHONGRGB	0x00000020

#define D3DPSHADECAPS_SPECULARFLATMONO	0x00000040
#define D3DPSHADECAPS_SPECULARFLATRGB	0x00000080
#define D3DPSHADECAPS_SPECULARGOURAUDMONO	0x00000100
#define D3DPSHADECAPS_SPECULARGOURAUDRGB	0x00000200
#define D3DPSHADECAPS_SPECULARPHONGMONO	0x00000400
#define D3DPSHADECAPS_SPECULARPHONGRGB	0x00000800

#define D3DPSHADECAPS_ALPHAFLATBLEND	0x00001000
#define D3DPSHADECAPS_ALPHAFLATSTIPPLED	0x00002000
#define D3DPSHADECAPS_ALPHAGOURAUDBLEND	0x00004000
#define D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED	0x00008000
#define D3DPSHADECAPS_ALPHAPHONGBLEND	0x00010000
#define D3DPSHADECAPS_ALPHAPHONGSTIPPLED	0x00020000

#define D3DPSHADECAPS_FOGFLAT		0x00040000
#define D3DPSHADECAPS_FOGGOURAUD	0x00080000
#define D3DPSHADECAPS_FOGPHONG		0x00100000

/* D3DPRIMCAPS.dwTextureCaps */
#define D3DPTEXTURECAPS_PERSPECTIVE	0x00000001
#define D3DPTEXTURECAPS_POW2		0x00000002
#define D3DPTEXTURECAPS_ALPHA		0x00000004
#define D3DPTEXTURECAPS_TRANSPARENCY	0x00000008
#define D3DPTEXTURECAPS_BORDER		0x00000010
#define D3DPTEXTURECAPS_SQUAREONLY	0x00000020

/* D3DPRIMCAPS.dwTextureFilterCaps */
#define D3DPTFILTERCAPS_NEAREST		0x00000001
#define D3DPTFILTERCAPS_LINEAR		0x00000002
#define D3DPTFILTERCAPS_MIPNEAREST	0x00000004
#define D3DPTFILTERCAPS_MIPLINEAR	0x00000008
#define D3DPTFILTERCAPS_LINEARMIPNEAREST	0x00000010
#define D3DPTFILTERCAPS_LINEARMIPLINEAR	0x00000020

/* D3DPRIMCAPS.dwTextureBlendCaps */
#define D3DPTBLENDCAPS_DECAL		0x00000001
#define D3DPTBLENDCAPS_MODULATE		0x00000002
#define D3DPTBLENDCAPS_DECALALPHA	0x00000004
#define D3DPTBLENDCAPS_MODULATEALPHA	0x00000008
#define D3DPTBLENDCAPS_DECALMASK	0x00000010
#define D3DPTBLENDCAPS_MODULATEMASK	0x00000020
#define D3DPTBLENDCAPS_COPY		0x00000040
#define D3DPTBLENDCAPS_ADD		0x00000080

/* D3DPRIMCAPS.dwTextureAddressCaps */
#define D3DPTADDRESSCAPS_WRAP		0x00000001
#define D3DPTADDRESSCAPS_MIRROR		0x00000002
#define D3DPTADDRESSCAPS_CLAMP		0x00000004
#define D3DPTADDRESSCAPS_BORDER		0x00000008
#define D3DPTADDRESSCAPS_INDEPENDENTUV	0x00000010


/* D3DDEVICEDESC.dwFlags */
#define D3DDD_COLORMODEL		0x00000001
#define D3DDD_DEVCAPS			0x00000002
#define D3DDD_TRANSFORMCAPS		0x00000004
#define D3DDD_LIGHTINGCAPS		0x00000008
#define D3DDD_BCLIPPING			0x00000010
#define D3DDD_LINECAPS			0x00000020
#define D3DDD_TRICAPS			0x00000040
#define D3DDD_DEVICERENDERBITDEPTH	0x00000080
#define D3DDD_DEVICEZBUFFERBITDEPTH	0x00000100
#define D3DDD_MAXBUFFERSIZE		0x00000200
#define D3DDD_MAXVERTEXCOUNT		0x00000400

/* D3DDEVICEDESC.dwDevCaps */
#define D3DDEVCAPS_FLOATTLVERTEX        0x00000001
#define D3DDEVCAPS_SORTINCREASINGZ      0x00000002
#define D3DDEVCAPS_SORTDECREASINGZ      0X00000004
#define D3DDEVCAPS_SORTEXACT            0x00000008
#define D3DDEVCAPS_EXECUTESYSTEMMEMORY  0x00000010
#define D3DDEVCAPS_EXECUTEVIDEOMEMORY   0x00000020
#define D3DDEVCAPS_TLVERTEXSYSTEMMEMORY 0x00000040
#define D3DDEVCAPS_TLVERTEXVIDEOMEMORY  0x00000080
#define D3DDEVCAPS_TEXTURESYSTEMMEMORY  0x00000100
#define D3DDEVCAPS_TEXTUREVIDEOMEMORY   0x00000200
#define D3DDEVCAPS_DRAWPRIMTLVERTEX     0x00000400
#define D3DDEVCAPS_CANRENDERAFTERFLIP   0x00000800
#define D3DDEVCAPS_TEXTURENONLOCALVIDMEM 0x00001000

typedef struct _D3DDeviceDesc {
	DWORD		dwSize;
	DWORD		dwFlags;
	D3DCOLORMODEL	dcmColorModel;
	DWORD		dwDevCaps;
	D3DTRANSFORMCAPS dtcTransformCaps;
	BOOL		bClipping;
	D3DLIGHTINGCAPS	dlcLightingCaps;
	D3DPRIMCAPS	dpcLineCaps;
	D3DPRIMCAPS	dpcTriCaps;
	DWORD		dwDeviceRenderBitDepth;
	DWORD		dwDeviceZBufferBitDepth;
	DWORD		dwMaxBufferSize;
	DWORD		dwMaxVertexCount;
	/* *** New fields for DX5 *** */
	DWORD		dwMinTextureWidth,dwMinTextureHeight;
	DWORD		dwMaxTextureWidth,dwMaxTextureHeight;
	DWORD		dwMinStippleWidth,dwMaxStippleWidth;
	DWORD		dwMinStippleHeight,dwMaxStippleHeight;
} D3DDEVICEDESC,*LPD3DDEVICEDESC;
 
typedef HRESULT (CALLBACK * LPD3DENUMDEVICESCALLBACK)(LPGUID lpGuid,LPSTR lpDeviceDescription,LPSTR lpDeviceName,LPD3DDEVICEDESC,LPD3DDEVICEDESC,LPVOID);
typedef HRESULT (CALLBACK* LPD3DVALIDATECALLBACK)(LPVOID lpUserArg, DWORD dwOffset);


/* dwflags for FindDevice */
#define D3DFDS_COLORMODEL		0x00000001
#define D3DFDS_GUID			0x00000002
#define D3DFDS_HARDWARE			0x00000004
#define D3DFDS_TRIANGLES		0x00000008
#define D3DFDS_LINES			0x00000010
#define D3DFDS_MISCCAPS			0x00000020
#define D3DFDS_RASTERCAPS		0x00000040
#define D3DFDS_ZCMPCAPS			0x00000080
#define D3DFDS_ALPHACMPCAPS		0x00000100
#define D3DFDS_DSTBLENDCAPS		0x00000400
#define D3DFDS_SHADECAPS		0x00000800
#define D3DFDS_TEXTURECAPS		0x00001000
#define D3DFDS_TEXTUREFILTERCAPS	0x00002000
#define D3DFDS_TEXTUREBLENDCAPS		0x00004000
#define D3DFDS_TEXTUREADDRESSCAPS	0x00008000

typedef struct {
    DWORD		dwSize;
    DWORD		dwFlags;
    BOOL		bHardware;
    D3DCOLORMODEL	dcmColorModel;
    GUID		guid;
    DWORD		dwCaps;
    D3DPRIMCAPS		dpcPrimCaps;
} D3DFINDDEVICESEARCH,*LPD3DFINDDEVICESEARCH;

typedef struct {
    DWORD		dwSize;
    GUID		guid;
    D3DDEVICEDESC	ddHwDesc;
    D3DDEVICEDESC	ddSwDesc;
} D3DFINDDEVICERESULT,*LPD3DFINDDEVICERESULT;

#define D3DVALP(val, prec)	((float)(val))
#define D3DVAL(val)		((float)(val))
typedef float D3DVALUE,*LPD3DVALUE;
#define D3DDivide(a, b)		(float)((double) (a) / (double) (b))
#define D3DMultiply(a, b)	((a) * (b))

typedef struct {
  DWORD         dwFlags;        /* Homogeneous clipping flags */
	union {
    D3DVALUE    hx;
    D3DVALUE    dvHX;
  } x;
  union {
    D3DVALUE    hy;
    D3DVALUE    dvHY;
  } y;
  union {
    D3DVALUE    hz;
    D3DVALUE    dvHZ;
  } z;
} D3DHVERTEX, *LPD3DHVERTEX;
/*
 * Transformed/lit vertices
 */
typedef struct {
  union {
    D3DVALUE    sx;             /* Screen coordinates */
    D3DVALUE    dvSX;
  } x;
  union {
    D3DVALUE    sy;
    D3DVALUE    dvSY;
  } y;
  union {
    D3DVALUE    sz;
    D3DVALUE    dvSZ;
  } z;
  union {
    D3DVALUE    rhw;            /* Reciprocal of homogeneous w */
    D3DVALUE    dvRHW;
  } r;
  union {
    D3DCOLOR    color;          /* Vertex color */
    D3DCOLOR    dcColor;
  } c;
  union {
    D3DCOLOR    specular;       /* Specular component of vertex */
    D3DCOLOR    dcSpecular;
  } s;
  union {
    D3DVALUE    tu;             /* Texture coordinates */
    D3DVALUE    dvTU;
  } u;
  union {
    D3DVALUE    tv;
    D3DVALUE    dvTV;
  } v;
} D3DTLVERTEX, *LPD3DTLVERTEX;
typedef struct {
  union {
    D3DVALUE     x;             /* Homogeneous coordinates */
		D3DVALUE dvX;
	} x;
	union {
		D3DVALUE y;
		D3DVALUE dvY;
	} y;
	union {
		D3DVALUE z;
		D3DVALUE dvZ;
	} z;
  DWORD            dwReserved;
  union {
    D3DCOLOR     color;         /* Vertex color */
    D3DCOLOR     dcColor;
  } c;
  union {
    D3DCOLOR     specular;      /* Specular component of vertex */
    D3DCOLOR     dcSpecular;
  } s;
  union {
    D3DVALUE     tu;            /* Texture coordinates */
    D3DVALUE     dvTU;
  } u;
  union {
    D3DVALUE     tv;
    D3DVALUE     dvTV;
  } v;
} D3DLVERTEX, *LPD3DLVERTEX;
typedef struct {
  union {
    D3DVALUE     x;             /* Homogeneous coordinates */
    D3DVALUE     dvX;
  } x;
  union {
    D3DVALUE     y;
    D3DVALUE     dvY;
  } y;
  union {
    D3DVALUE     z;
    D3DVALUE     dvZ;
  } z;
  union {
    D3DVALUE     nx;            /* Normal */
    D3DVALUE     dvNX;
  } nx;
  union {
    D3DVALUE     ny;
    D3DVALUE     dvNY;
  } ny;
  union {
    D3DVALUE     nz;
    D3DVALUE     dvNZ;
  } nz;
  union {
    D3DVALUE     tu;            /* Texture coordinates */
    D3DVALUE     dvTU;
  } u;
  union {
    D3DVALUE     tv;
    D3DVALUE     dvTV;
  } v;
} D3DVERTEX, *LPD3DVERTEX;

typedef struct {
  union {
    LONG x1;
    LONG lX1;
  } x1;
  union {
    LONG y1;
    LONG lY1;
  } y1;
  union {
    LONG x2;
    LONG lX2;
  } x2;
  union {
    LONG y2;
    LONG lY2;
  } y2;
} D3DRECT, *LPD3DRECT;

typedef struct {
  union {
	D3DVALUE	x;
    D3DVALUE dvX;
  } x;
  union {
	D3DVALUE	y;
    D3DVALUE dvY;
  } y;
  union {
	D3DVALUE	z;
    D3DVALUE dvZ;
  } z;
  /* the c++ variant has operator overloads etc. too */
} D3DVECTOR,*LPD3DVECTOR;

typedef struct {
  D3DVALUE        _11, _12, _13, _14;
  D3DVALUE        _21, _22, _23, _24;
  D3DVALUE        _31, _32, _33, _34;
  D3DVALUE        _41, _42, _43, _44;
} D3DMATRIX, *LPD3DMATRIX;

typedef struct _D3DCOLORVALUE {
	union {
		D3DVALUE r;
		D3DVALUE dvR;
	} r;
	union {
		D3DVALUE g;
		D3DVALUE dvG;
	} g;
	union {
		D3DVALUE b;
		D3DVALUE dvB;
	} b;
	union {
		D3DVALUE a;
		D3DVALUE dvA;
	} a;
} D3DCOLORVALUE,*LPD3DCOLORVALUE;

typedef struct {
    DWORD           dwSize;
    D3DLIGHTTYPE    dltType;
    D3DCOLORVALUE   dcvColor;
    D3DVECTOR       dvPosition;		/* Position in world space */
    D3DVECTOR       dvDirection;	/* Direction in world space */
    D3DVALUE        dvRange;		/* Cutoff range */
    D3DVALUE        dvFalloff;		/* Falloff */
    D3DVALUE        dvAttenuation0;	/* Constant attenuation */
    D3DVALUE        dvAttenuation1;	/* Linear attenuation */
    D3DVALUE        dvAttenuation2;	/* Quadratic attenuation */
    D3DVALUE        dvTheta;		/* Inner angle of spotlight cone */
    D3DVALUE        dvPhi;		/* Outer angle of spotlight cone */
} D3DLIGHT,*LPD3DLIGHT;

/* flags bits */
#define D3DLIGHT_ACTIVE		0x00000001
#define D3DLIGHT_NO_SPECULAR	0x00000002

/* Textures */
typedef HRESULT (CALLBACK* LPD3DENUMTEXTUREFORMATSCALLBACK)(LPDDSURFACEDESC lpDdsd, LPVOID lpContext);


/* Statistics structure */
typedef struct {
  DWORD        dwSize;
  DWORD        dwTrianglesDrawn;
  DWORD        dwLinesDrawn;
  DWORD        dwPointsDrawn;
  DWORD        dwSpansDrawn;
  DWORD        dwVerticesProcessed;
} D3DSTATS, *LPD3DSTATS;

/* Clipping */
typedef struct _D3DCLIPSTATUS {
  DWORD dwFlags; /* Do we set 2d extents, 3D extents or status */
  DWORD dwStatus; /* Clip status */
  float minx, maxx; /* X extents */
  float miny, maxy; /* Y extents */
  float minz, maxz; /* Z extents */
} D3DCLIPSTATUS, *LPD3DCLIPSTATUS;

typedef struct {
  DWORD               dwSize;
  union {
    D3DCOLORVALUE   diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   dcvDiffuse;
  } a;
  union {
    D3DCOLORVALUE   ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   dcvAmbient;
  } b;
  union {
    D3DCOLORVALUE   specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   dcvSpecular;
  } c;
  union {
    D3DCOLORVALUE   emissive;       /* Emissive color RGB */
    D3DCOLORVALUE   dcvEmissive;
  } d;
  union {
    D3DVALUE        power;          /* Sharpness if specular highlight */
    D3DVALUE        dvPower;
  } e;
  D3DTEXTUREHANDLE    hTexture;       /* Handle to texture map */
  DWORD               dwRampSize;
} D3DMATERIAL, *LPD3DMATERIAL;

typedef struct {
  D3DVECTOR dvPosition;  /* Lightable point in model space */
  D3DVECTOR dvNormal;    /* Normalised unit vector */
} D3DLIGHTINGELEMENT, *LPD3DLIGHTINGELEMENT;

typedef struct {
  DWORD       dwSize;
  DWORD       dwX;
  DWORD       dwY;            /* Top left */
  DWORD       dwWidth;
  DWORD       dwHeight;       /* Dimensions */
  D3DVALUE    dvScaleX;       /* Scale homogeneous to screen */
  D3DVALUE    dvScaleY;       /* Scale homogeneous to screen */
  D3DVALUE    dvMaxX;         /* Min/max homogeneous x coord */
  D3DVALUE    dvMaxY;         /* Min/max homogeneous y coord */
  D3DVALUE    dvMinZ;
  D3DVALUE    dvMaxZ;         /* Min/max homogeneous z coord */
} D3DVIEWPORT, *LPD3DVIEWPORT;

typedef struct {
  DWORD       dwSize;
  DWORD       dwX;
  DWORD       dwY;            /* Viewport Top left */
  DWORD       dwWidth;
  DWORD       dwHeight;       /* Viewport Dimensions */
  D3DVALUE    dvClipX;        /* Top left of clip volume */
  D3DVALUE    dvClipY;
  D3DVALUE    dvClipWidth;    /* Clip Volume Dimensions */
  D3DVALUE    dvClipHeight;
  D3DVALUE    dvMinZ;         /* Min/max of clip Volume */
  D3DVALUE    dvMaxZ;
} D3DVIEWPORT2, *LPD3DVIEWPORT2;

#define D3DTRANSFORM_CLIPPED       0x00000001l
#define D3DTRANSFORM_UNCLIPPED     0x00000002l

typedef struct {
  DWORD           dwSize;
  LPVOID          lpIn;           /* Input vertices */
  DWORD           dwInSize;       /* Stride of input vertices */
  LPVOID          lpOut;          /* Output vertices */
  DWORD           dwOutSize;      /* Stride of output vertices */
  LPD3DHVERTEX    lpHOut;         /* Output homogeneous vertices */
  DWORD           dwClip;         /* Clipping hint */
  DWORD           dwClipIntersection;
  DWORD           dwClipUnion;    /* Union of all clip flags */
  D3DRECT         drExtent;       /* Extent of transformed vertices */
} D3DTRANSFORMDATA, *LPD3DTRANSFORMDATA;

/* flags bits */
#define D3DLIGHT_ACTIVE         0x00000001
#define D3DLIGHT_NO_SPECULAR    0x00000002

/* maximum valid light range */
#define D3DLIGHT_RANGE_MAX              ((float)sqrt(FLT_MAX))

typedef struct _D3DLIGHT2 {
  DWORD           dwSize;
  D3DLIGHTTYPE    dltType;            /* Type of light source */
  D3DCOLORVALUE   dcvColor;           /* Color of light */
  D3DVECTOR       dvPosition;         /* Position in world space */
  D3DVECTOR       dvDirection;        /* Direction in world space */
  D3DVALUE        dvRange;            /* Cutoff range */
  D3DVALUE        dvFalloff;          /* Falloff */
  D3DVALUE        dvAttenuation0;     /* Constant attenuation */
  D3DVALUE        dvAttenuation1;     /* Linear attenuation */
  D3DVALUE        dvAttenuation2;     /* Quadratic attenuation */
  D3DVALUE        dvTheta;            /* Inner angle of spotlight cone */
  D3DVALUE        dvPhi;              /* Outer angle of spotlight cone */
  DWORD           dwFlags;
} D3DLIGHT2, *LPD3DLIGHT2;

typedef struct _D3DLIGHTDATA {
  DWORD                dwSize;
  LPD3DLIGHTINGELEMENT lpIn;          /* Input positions and normals */
  DWORD                dwInSize;      /* Stride of input elements */
  LPD3DTLVERTEX        lpOut;         /* Output colors */
  DWORD                dwOutSize;     /* Stride of output colors */
} D3DLIGHTDATA, *LPD3DLIGHTDATA;

typedef struct _D3DPICKRECORD {
  BYTE     bOpcode;
  BYTE     bPad;
  DWORD    dwOffset;
  D3DVALUE dvZ;
} D3DPICKRECORD, *LPD3DPICKRECORD;


typedef struct _D3DExecuteBufferDesc { 
  DWORD  dwSize; 
  DWORD  dwFlags; 
  DWORD  dwCaps; 
  DWORD  dwBufferSize; 
  LPVOID lpData; 
} D3DEXECUTEBUFFERDESC; 
typedef D3DEXECUTEBUFFERDESC *LPD3DEXECUTEBUFFERDESC; 

#define D3DDEB_BUFSIZE          0x00000001l     /* buffer size valid */
#define D3DDEB_CAPS             0x00000002l     /* caps valid */
#define D3DDEB_LPDATA           0x00000004l     /* lpData valid */

#define D3DDEBCAPS_SYSTEMMEMORY 0x00000001l     /* buffer in system memory */
#define D3DDEBCAPS_VIDEOMEMORY  0x00000002l     /* buffer in device memory */
#define D3DDEBCAPS_MEM (D3DDEBCAPS_SYSTEMMEMORY|D3DDEBCAPS_VIDEOMEMORY)

/*
 * Values for d3d status.
 */
#define D3DSTATUS_CLIPUNIONLEFT                 D3DCLIP_LEFT
#define D3DSTATUS_CLIPUNIONRIGHT                D3DCLIP_RIGHT
#define D3DSTATUS_CLIPUNIONTOP                  D3DCLIP_TOP
#define D3DSTATUS_CLIPUNIONBOTTOM               D3DCLIP_BOTTOM
#define D3DSTATUS_CLIPUNIONFRONT                D3DCLIP_FRONT
#define D3DSTATUS_CLIPUNIONBACK                 D3DCLIP_BACK
#define D3DSTATUS_CLIPUNIONGEN0                 D3DCLIP_GEN0
#define D3DSTATUS_CLIPUNIONGEN1                 D3DCLIP_GEN1
#define D3DSTATUS_CLIPUNIONGEN2                 D3DCLIP_GEN2
#define D3DSTATUS_CLIPUNIONGEN3                 D3DCLIP_GEN3
#define D3DSTATUS_CLIPUNIONGEN4                 D3DCLIP_GEN4
#define D3DSTATUS_CLIPUNIONGEN5                 D3DCLIP_GEN5

#define D3DSTATUS_CLIPINTERSECTIONLEFT          0x00001000L
#define D3DSTATUS_CLIPINTERSECTIONRIGHT         0x00002000L
#define D3DSTATUS_CLIPINTERSECTIONTOP           0x00004000L
#define D3DSTATUS_CLIPINTERSECTIONBOTTOM        0x00008000L
#define D3DSTATUS_CLIPINTERSECTIONFRONT         0x00010000L
#define D3DSTATUS_CLIPINTERSECTIONBACK          0x00020000L
#define D3DSTATUS_CLIPINTERSECTIONGEN0          0x00040000L
#define D3DSTATUS_CLIPINTERSECTIONGEN1          0x00080000L
#define D3DSTATUS_CLIPINTERSECTIONGEN2          0x00100000L
#define D3DSTATUS_CLIPINTERSECTIONGEN3          0x00200000L
#define D3DSTATUS_CLIPINTERSECTIONGEN4          0x00400000L
#define D3DSTATUS_CLIPINTERSECTIONGEN5          0x00800000L
#define D3DSTATUS_ZNOTVISIBLE                   0x01000000L

#define D3DSTATUS_CLIPUNIONALL  (               \
            D3DSTATUS_CLIPUNIONLEFT     |       \
            D3DSTATUS_CLIPUNIONRIGHT    |       \
            D3DSTATUS_CLIPUNIONTOP      |       \
            D3DSTATUS_CLIPUNIONBOTTOM   |       \
            D3DSTATUS_CLIPUNIONFRONT    |       \
            D3DSTATUS_CLIPUNIONBACK     |       \
            D3DSTATUS_CLIPUNIONGEN0     |       \
            D3DSTATUS_CLIPUNIONGEN1     |       \
            D3DSTATUS_CLIPUNIONGEN2     |       \
            D3DSTATUS_CLIPUNIONGEN3     |       \
            D3DSTATUS_CLIPUNIONGEN4     |       \
            D3DSTATUS_CLIPUNIONGEN5             \
            )

#define D3DSTATUS_CLIPINTERSECTIONALL   (               \
            D3DSTATUS_CLIPINTERSECTIONLEFT      |       \
            D3DSTATUS_CLIPINTERSECTIONRIGHT     |       \
            D3DSTATUS_CLIPINTERSECTIONTOP       |       \
            D3DSTATUS_CLIPINTERSECTIONBOTTOM    |       \
            D3DSTATUS_CLIPINTERSECTIONFRONT     |       \
            D3DSTATUS_CLIPINTERSECTIONBACK      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN0      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN1      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN2      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN3      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN4      |       \
            D3DSTATUS_CLIPINTERSECTIONGEN5              \
            )

#define D3DSTATUS_DEFAULT       (                       \
            D3DSTATUS_CLIPINTERSECTIONALL       |       \
            D3DSTATUS_ZNOTVISIBLE)


typedef struct _D3DSTATUS { 
  DWORD   dwFlags; 
  DWORD   dwStatus; 
  D3DRECT drExtent; 
} D3DSTATUS, *LPD3DSTATUS; 
 

typedef struct _D3DEXECUTEDATA { 
  DWORD     dwSize; 
  DWORD     dwVertexOffset; 
  DWORD     dwVertexCount; 
  DWORD     dwInstructionOffset; 
  DWORD     dwInstructionLength; 
  DWORD     dwHVertexOffset; 
  D3DSTATUS dsStatus; 
} D3DEXECUTEDATA, *LPD3DEXECUTEDATA; 

typedef enum _D3DOPCODE { 
  D3DOP_POINT           = 1, 
  D3DOP_LINE            = 2, 
  D3DOP_TRIANGLE        = 3, 
  D3DOP_MATRIXLOAD      = 4, 
  D3DOP_MATRIXMULTIPLY  = 5, 
  D3DOP_STATETRANSFORM  = 6, 
  D3DOP_STATELIGHT      = 7, 
  D3DOP_STATERENDER     = 8, 
  D3DOP_PROCESSVERTICES = 9, 
  D3DOP_TEXTURELOAD     = 10, 
  D3DOP_EXIT            = 11, 
  D3DOP_BRANCHFORWARD   = 12, 
  D3DOP_SPAN            = 13, 
  D3DOP_SETSTATUS       = 14, 
  
  D3DOP_FORCE_DWORD     = 0x7fffffff, 
} D3DOPCODE; 

typedef struct _D3DPOINT { 
  WORD wCount; 
  WORD wFirst; 
} D3DPOINT, *LPD3DPOINT; 

typedef struct _D3DLINE { 
  union { 
    WORD v1; 
    WORD wV1; 
  } v1; 
  union { 
    WORD v2; 
    WORD wV2; 
  } v2; 
} D3DLINE, *LPD3DLINE; 

#define D3DTRIFLAG_START                        0x00000000L
#define D3DTRIFLAG_STARTFLAT(len) (len)         /* 0 < len < 30 */
#define D3DTRIFLAG_ODD                          0x0000001eL
#define D3DTRIFLAG_EVEN                         0x0000001fL

#define D3DTRIFLAG_EDGEENABLE1                  0x00000100L /* v0-v1 edge */
#define D3DTRIFLAG_EDGEENABLE2                  0x00000200L /* v1-v2 edge */
#define D3DTRIFLAG_EDGEENABLE3                  0x00000400L /* v2-v0 edge */
#define D3DTRIFLAG_EDGEENABLETRIANGLE \
        (D3DTRIFLAG_EDGEENABLE1 | D3DTRIFLAG_EDGEENABLE2 | D3DTRIFLAG_EDGEENABLE3)

typedef struct _D3DTRIANGLE { 
  union { 
    WORD v1; 
    WORD wV1; 
  } v1; 
  union { 
    WORD v2; 
    WORD wV2; 
  } v2; 
  union { 
    WORD v3; 
    WORD wV3; 
  } v3; 
  WORD     wFlags; 
} D3DTRIANGLE, *LPD3DTRIANGLE; 

typedef struct _D3DMATRIXLOAD { 
  D3DMATRIXHANDLE hDestMatrix; 
  D3DMATRIXHANDLE hSrcMatrix; 
} D3DMATRIXLOAD, *LPD3DMATRIXLOAD; 

typedef struct _D3DMATRIXMULTIPLY { 
  D3DMATRIXHANDLE hDestMatrix; 
  D3DMATRIXHANDLE hSrcMatrix1; 
  D3DMATRIXHANDLE hSrcMatrix2; 
} D3DMATRIXMULTIPLY, *LPD3DMATRIXMULTIPLY; 

typedef struct _D3DSTATE { 
  union { 
    D3DTRANSFORMSTATETYPE dtstTransformStateType; 
    D3DLIGHTSTATETYPE     dlstLightStateType; 
    D3DRENDERSTATETYPE    drstRenderStateType; 
  } t; 
  union { 
    DWORD                 dwArg[1]; 
    D3DVALUE              dvArg[1]; 
  } v; 
} D3DSTATE, *LPD3DSTATE; 

#define D3DPROCESSVERTICES_TRANSFORMLIGHT       0x00000000L
#define D3DPROCESSVERTICES_TRANSFORM            0x00000001L
#define D3DPROCESSVERTICES_COPY                 0x00000002L
#define D3DPROCESSVERTICES_OPMASK               0x00000007L

#define D3DPROCESSVERTICES_UPDATEEXTENTS        0x00000008L
#define D3DPROCESSVERTICES_NOCOLOR              0x00000010L

typedef struct _D3DPROCESSVERTICES { 
  DWORD dwFlags; 
  WORD  wStart; 
  WORD  wDest; 
  DWORD dwCount; 
  DWORD dwReserved; 
} D3DPROCESSVERTICES, *LPD3DPROCESSVERTICES; 

typedef struct _D3DTEXTURELOAD { 
  D3DTEXTUREHANDLE hDestTexture; 
  D3DTEXTUREHANDLE hSrcTexture; 
} D3DTEXTURELOAD, *LPD3DTEXTURELOAD; 

typedef struct _D3DBRANCH { 
  DWORD dwMask; 
  DWORD dwValue; 
  BOOL  bNegate; 
  DWORD dwOffset; 
} D3DBRANCH, *LPD3DBRANCH; 

typedef struct _D3DSPAN { 
  WORD wCount; 
  WORD wFirst; 
} D3DSPAN, *LPD3DSPAN; 

typedef struct _D3DINSTRUCTION { 
  BYTE bOpcode; 
  BYTE bSize; 
  WORD wCount; 
} D3DINSTRUCTION, *LPD3DINSTRUCTION; 


/*****************************************************************************
 * IDirect3D interface
 */
#define ICOM_INTERFACE IDirect3D
#define IDirect3D_METHODS \
    ICOM_METHOD1(HRESULT,Initialize,     REFIID,riid) \
    ICOM_METHOD2(HRESULT,EnumDevices,    LPD3DENUMDEVICESCALLBACK,lpEnumDevicesCallback, LPVOID,lpUserArg) \
    ICOM_METHOD2(HRESULT,CreateLight,    LPDIRECT3DLIGHT*,lplpDirect3DLight, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateMaterial, LPDIRECT3DMATERIAL*,lplpDirect3DMaterial, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateViewport, LPDIRECT3DVIEWPORT*,lplpD3DViewport, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,FindDevice,     LPD3DFINDDEVICESEARCH,lpD3DDFS, LPD3DFINDDEVICERESULT,lplpD3DDevice)
#define IDirect3D_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3D_METHODS
ICOM_DEFINE(IDirect3D,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
	/*** IUnknown methods ***/
#define IDirect3D_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3D_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3D_Release(p)            ICOM_CALL (Release,p)
	/*** IDirect3D methods ***/
#define IDirect3D_Initialize(p,a)       ICOM_CALL2(Initialize,p,a)
#define IDirect3D_EnumDevices(p,a,b)    ICOM_CALL2(EnumDevices,p,a,b)
#define IDirect3D_CreateLight(p,a,b)    ICOM_CALL2(CreateLight,p,a,b)
#define IDirect3D_CreateMaterial(p,a,b) ICOM_CALL2(CreateMaterial,p,a,b)
#define IDirect3D_CreateViewport(p,a,b) ICOM_CALL2(CreateViewport,p,a,b)
#define IDirect3D_FindDevice(p,a,b)     ICOM_CALL2(FindDevice,p,a,b)
#endif


/*****************************************************************************
 * IDirect3D2 interface
 */
#define ICOM_INTERFACE IDirect3D2
#define IDirect3D2_METHODS \
    ICOM_METHOD2(HRESULT,EnumDevices,    LPD3DENUMDEVICESCALLBACK,lpEnumDevicesCallback, LPVOID,lpUserArg) \
    ICOM_METHOD2(HRESULT,CreateLight,    LPDIRECT3DLIGHT*,lplpDirect3DLight, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateMaterial, LPDIRECT3DMATERIAL2*,lplpDirect3DMaterial2, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateViewport, LPDIRECT3DVIEWPORT2*,lplpD3DViewport2, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,FindDevice,     LPD3DFINDDEVICESEARCH,lpD3DDFS, LPD3DFINDDEVICERESULT,lpD3DFDR) \
    ICOM_METHOD3(HRESULT,CreateDevice,   REFCLSID,rclsid, LPDIRECTDRAWSURFACE,lpDDS, LPDIRECT3DDEVICE2*,lplpD3DDevice2)
#define IDirect3D2_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3D2_METHODS
ICOM_DEFINE(IDirect3D2,IUnknown)
#undef ICOM_INTERFACE
  
#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirect3D2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3D2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3D2_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3D2 methods ***/
#define IDirect3D2_EnumDevices(p,a,b)    ICOM_CALL2(EnumDevices,p,a,b)
#define IDirect3D2_CreateLight(p,a,b)    ICOM_CALL2(CreateLight,p,a,b)
#define IDirect3D2_CreateMaterial(p,a,b) ICOM_CALL2(CreateMaterial,p,a,b)
#define IDirect3D2_CreateViewport(p,a,b) ICOM_CALL2(CreateViewport,p,a,b)
#define IDirect3D2_FindDevice(p,a,b)     ICOM_CALL2(FindDevice,p,a,b)
#define IDirect3D2_CreateDevice(p,a,b,c) ICOM_CALL3(CreateDevice,p,a,b,c)
#endif


/*****************************************************************************
 * IDirect3DLight interface
 */
#define ICOM_INTERFACE IDirect3DLight
#define IDirect3DLight_METHODS \
    ICOM_METHOD1(HRESULT,Initialize, LPDIRECT3D,lpDirect3D) \
    ICOM_METHOD1(HRESULT,SetLight,   LPD3DLIGHT,lpLight) \
    ICOM_METHOD1(HRESULT,GetLight,   LPD3DLIGHT,lpLight)
#define IDirect3DLight_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DLight_METHODS
ICOM_DEFINE(IDirect3DLight,IUnknown)
#undef ICOM_INTERFACE
  
#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDirect3DLight_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DLight_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DLight_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3DLight methods ***/
#define IDirect3DLight_Initialize(p,a) ICOM_CALL1(Initialize,p,a)
#define IDirect3DLight_SetLight(p,a)   ICOM_CALL1(SetLight,p,a)
#define IDirect3DLight_GetLight(p,a)   ICOM_CALL1(GetLight,p,a)
#endif


/*****************************************************************************
 * IDirect3DMaterial interface
 */
#define ICOM_INTERFACE IDirect3DMaterial
#define IDirect3DMaterial_METHODS \
    ICOM_METHOD1(HRESULT,Initialize,  LPDIRECT3D,lpDirect3D) \
    ICOM_METHOD1(HRESULT,SetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD1(HRESULT,GetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD2(HRESULT,GetHandle,   LPDIRECT3DDEVICE2,lpDirect3DDevice2, LPD3DMATERIALHANDLE,lpHandle) \
    ICOM_METHOD (HRESULT,Reserve) \
    ICOM_METHOD (HRESULT,Unreserve)
#define IDirect3DMaterial_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DMaterial_METHODS
ICOM_DEFINE(IDirect3DMaterial,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DMaterial_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DMaterial_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DMaterial_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3DMaterial methods ***/
#define IDirect3DMaterial_Initialize(p,a)  ICOM_CALL1(Initialize,p,a)
#define IDirect3DMaterial_SetMaterial(p,a) ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DMaterial_GetMaterial(p,a) ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DMaterial_GetHandle(p,a,b) ICOM_CALL2(GetHandle,p,a,b)
#define IDirect3DMaterial_Reserve(p)       ICOM_CALL (Reserve,p)
#define IDirect3DMaterial_Unreserve(p)     ICOM_CALL (Unreserve,p)
#endif


/*****************************************************************************
 * IDirect3DMaterial2 interface
 */
#define ICOM_INTERFACE IDirect3DMaterial2
#define IDirect3DMaterial2_METHODS \
    ICOM_METHOD1(HRESULT,SetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD1(HRESULT,GetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD2(HRESULT,GetHandle,   LPDIRECT3DDEVICE2,lpDirect3DDevice2, LPD3DMATERIALHANDLE,lpHandle)
#define IDirect3DMaterial2_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DMaterial2_METHODS
ICOM_DEFINE(IDirect3DMaterial2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DMaterial2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DMaterial2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DMaterial2_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DMaterial2 methods ***/
#define IDirect3DMaterial2_SetMaterial(p,a) ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DMaterial2_GetMaterial(p,a) ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DMaterial2_GetHandle(p,a,b) ICOM_CALL2(GetHandle,p,a,b)
#endif
  

/*****************************************************************************
 * IDirect3DTexture interface
 */
#define ICOM_INTERFACE IDirect3DTexture
#define IDirect3DTexture_METHODS \
    ICOM_METHOD2(HRESULT,Initialize,     LPDIRECT3DDEVICE,lpDirect3DDevice, LPDIRECTDRAWSURFACE,) \
    ICOM_METHOD2(HRESULT,GetHandle,      LPDIRECT3DDEVICE,lpDirect3DDevice, LPD3DTEXTUREHANDLE,) \
    ICOM_METHOD2(HRESULT,PaletteChanged, DWORD,dwStart, DWORD,dwCount) \
    ICOM_METHOD1(HRESULT,Load,           LPDIRECT3DTEXTURE,lpD3DTexture) \
    ICOM_METHOD (HRESULT,Unload)
#define IDirect3DTexture_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DTexture_METHODS
ICOM_DEFINE(IDirect3DTexture,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DTexture_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DTexture_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DTexture_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DTexture methods ***/
#define IDirect3DTexture_Initialize(p,a,b,c) ICOM_CALL(Initialize,p,a,b,c)
#define IDirect3DTexture_GetHandle(p,a,b,c) ICOM_CALL(GetHandle,p,a,b,c)
#define IDirect3DTexture_PaletteChanged(p,a,b,c) ICOM_CALL(PaletteChanged,p,a,b,c)
#define IDirect3DTexture_Load(p,a,b,c) ICOM_CALL(Load,p,a,b,c)
#define IDirect3DTexture_Unload(p,a,b,c) ICOM_CALL(Unload,p,a,b,c)
#endif


/*****************************************************************************
 * IDirect3DTexture2 interface
 */
#define ICOM_INTERFACE IDirect3DTexture2
#define IDirect3DTexture2_METHODS \
    ICOM_METHOD2(HRESULT,GetHandle,      LPDIRECT3DDEVICE2,lpDirect3DDevice2, LPD3DTEXTUREHANDLE,lpHandle) \
    ICOM_METHOD2(HRESULT,PaletteChanged, DWORD,dwStart, DWORD,dwCount) \
    ICOM_METHOD1(HRESULT,Load,           LPDIRECT3DTEXTURE2,lpD3DTexture2)
#define IDirect3DTexture2_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DTexture2_METHODS
ICOM_DEFINE(IDirect3DTexture2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DTexture2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DTexture2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DTexture2_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DTexture2 methods ***/
#define IDirect3DTexture2_GetHandle(p,a,b)      ICOM_CALL2(GetHandle,p,a,b)
#define IDirect3DTexture2_PaletteChanged(p,a,b) ICOM_CALL2(PaletteChanged,p,a,b)
#define IDirect3DTexture2_Load(p,a)             ICOM_CALL1(Load,p,a)
#endif  


/*****************************************************************************
 * IDirect3DViewport interface
 */
#define ICOM_INTERFACE IDirect3DViewport
#define IDirect3DViewport_METHODS \
    ICOM_METHOD1(HRESULT,Initialize,         LPDIRECT3D,lpDirect3D) \
    ICOM_METHOD1(HRESULT,GetViewport,        LPD3DVIEWPORT,lpData) \
    ICOM_METHOD1(HRESULT,SetViewport,        LPD3DVIEWPORT,lpData) \
    ICOM_METHOD4(HRESULT,TransformVertices,  DWORD,dwVertexCount, LPD3DTRANSFORMDATA,lpData, DWORD,dwFlags, LPDWORD,lpOffScreen) \
    ICOM_METHOD2(HRESULT,LightElements,      DWORD,dwElementCount, LPD3DLIGHTDATA,lpData) \
    ICOM_METHOD1(HRESULT,SetBackground,      D3DMATERIALHANDLE,hMat) \
    ICOM_METHOD2(HRESULT,GetBackground,      LPD3DMATERIALHANDLE,, LPBOOL,) \
    ICOM_METHOD1(HRESULT,SetBackgroundDepth, LPDIRECTDRAWSURFACE,lpDDSurface) \
    ICOM_METHOD2(HRESULT,GetBackgroundDepth, LPDIRECTDRAWSURFACE*,lplpDDSurface, LPBOOL,lpValid) \
    ICOM_METHOD3(HRESULT,Clear,              DWORD,dwCount, LPD3DRECT,lpRects, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,AddLight,           LPDIRECT3DLIGHT,lpDirect3DLight) \
    ICOM_METHOD1(HRESULT,DeleteLight,        LPDIRECT3DLIGHT,lpDirect3DLight) \
    ICOM_METHOD3(HRESULT,NextLight,          LPDIRECT3DLIGHT,lpDirect3DLight, LPDIRECT3DLIGHT*,lplpDirect3DLight, DWORD,dwFlags)
#define IDirect3DViewport_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DViewport_METHODS
ICOM_DEFINE(IDirect3DViewport,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DViewport_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DViewport_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DViewport_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DViewport methods ***/
#define IDirect3DViewport_Initialize(p,a)              ICOM_CALL1(Initialize,p,a)
#define IDirect3DViewport_GetViewport(p,a)             ICOM_CALL1(GetViewport,p,a)
#define IDirect3DViewport_SetViewport(p,a)             ICOM_CALL1(SetViewport,p,a)
#define IDirect3DViewport_TransformVertices(p,a,b,c,d) ICOM_CALL4(TransformVertices,p,a,b,c,d)
#define IDirect3DViewport_LightElements(p,a,b)         ICOM_CALL2(LightElements,p,a,b)
#define IDirect3DViewport_SetBackground(p,a)           ICOM_CALL1(SetBackground,p,a)
#define IDirect3DViewport_GetBackground(p,a,b)         ICOM_CALL2(GetBackground,p,a,b)
#define IDirect3DViewport_SetBackgroundDepth(p,a)      ICOM_CALL1(SetBackgroundDepth,p,a)
#define IDirect3DViewport_GetBackgroundDepth(p,a,b)    ICOM_CALL2(GetBackgroundDepth,p,a,b)
#define IDirect3DViewport_Clear(p,a,b,c)               ICOM_CALL3(Clear,p,a,b,c)
#define IDirect3DViewport_AddLight(p,a)                ICOM_CALL1(AddLight,p,a)
#define IDirect3DViewport_DeleteLight(p,a)             ICOM_CALL1(DeleteLight,p,a)
#define IDirect3DViewport_NextLight(p,a,b,c)           ICOM_CALL3(NextLight,p,a,b,c)
#endif


/*****************************************************************************
 * IDirect3DViewport2 interface
 */
#define ICOM_INTERFACE IDirect3DViewport2
#define IDirect3DViewport2_METHODS \
    ICOM_METHOD1(HRESULT,GetViewport2, LPD3DVIEWPORT2,lpData) \
    ICOM_METHOD1(HRESULT,SetViewport2, LPD3DVIEWPORT2,lpData)
#define IDirect3DViewport2_IMETHODS \
    IDirect3DViewport_IMETHODS \
    IDirect3DViewport2_METHODS
ICOM_DEFINE(IDirect3DViewport2,IDirect3DViewport)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DViewport2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DViewport2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DViewport2_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3Viewport methods ***/
#define IDirect3DViewport2_Initialize(p,a)              ICOM_CALL1(Initialize,p,a)
#define IDirect3DViewport2_GetViewport(p,a)             ICOM_CALL1(GetViewport,p,a)
#define IDirect3DViewport2_SetViewport(p,a)             ICOM_CALL1(SetViewport,p,a)
#define IDirect3DViewport2_TransformVertices(p,a,b,c,d) ICOM_CALL4(TransformVertices,p,a,b,c,d)
#define IDirect3DViewport2_LightElements(p,a,b)         ICOM_CALL2(LightElements,p,a,b)
#define IDirect3DViewport2_SetBackground(p,a)           ICOM_CALL1(SetBackground,p,a)
#define IDirect3DViewport2_GetBackground(p,a,b)         ICOM_CALL2(GetBackground,p,a,b)
#define IDirect3DViewport2_SetBackgroundDepth(p,a)      ICOM_CALL1(SetBackgroundDepth,p,a)
#define IDirect3DViewport2_GetBackgroundDepth(p,a,b)    ICOM_CALL2(GetBackgroundDepth,p,a,b)
#define IDirect3DViewport2_Clear(p,a,b,c)               ICOM_CALL3(Clear,p,a,b,c)
#define IDirect3DViewport2_AddLight(p,a)                ICOM_CALL1(AddLight,p,a)
#define IDirect3DViewport2_DeleteLight(p,a)             ICOM_CALL1(DeleteLight,p,a)
#define IDirect3DViewport2_NextLight(p,a,b,c)           ICOM_CALL3(NextLight,p,a,b,c)
  /*** IDirect3DViewport2 methods ***/
#define IDirect3DViewport2_GetViewport2(p,a) ICOM_CALL1(GetViewport2,p,a)
#define IDirect3DViewport2_SetViewport2(p,a) ICOM_CALL1(SetViewport2,p,a)
#endif


/*****************************************************************************
 * IDirect3DExecuteBuffer interface
 */
#define ICOM_INTERFACE IDirect3DExecuteBuffer
#define IDirect3DExecuteBuffer_METHODS \
    ICOM_METHOD2(HRESULT,Initialize,     LPDIRECT3DDEVICE,lpDirect3DDevice, LPD3DEXECUTEBUFFERDESC,lpDesc) \
    ICOM_METHOD1(HRESULT,Lock,           LPD3DEXECUTEBUFFERDESC,lpDesc) \
    ICOM_METHOD (HRESULT,Unlock) \
    ICOM_METHOD1(HRESULT,SetExecuteData, LPD3DEXECUTEDATA,lpData) \
    ICOM_METHOD1(HRESULT,GetExecuteData, LPD3DEXECUTEDATA,lpData) \
    ICOM_METHOD4(HRESULT,Validate,       LPDWORD,lpdwOffset, LPD3DVALIDATECALLBACK,lpFunc, LPVOID,lpUserArg, DWORD,dwReserved) \
    ICOM_METHOD1(HRESULT,Optimize,       DWORD,)
#define IDirect3DExecuteBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DExecuteBuffer_METHODS
ICOM_DEFINE(IDirect3DExecuteBuffer,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DExecuteBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DExecuteBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DExecuteBuffer_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DExecuteBuffer methods ***/
#define IDirect3DExecuteBuffer_Initialize(p,a,b)   ICOM_CALL2(Initialize,p,a,b)
#define IDirect3DExecuteBuffer_Lock(p,a)           ICOM_CALL1(Lock,p,a)
#define IDirect3DExecuteBuffer_Unlock(p)           ICOM_CALL (Unlock,p)
#define IDirect3DExecuteBuffer_SetExecuteData(p,a) ICOM_CALL1(SetExecuteData,p,a)
#define IDirect3DExecuteBuffer_GetExecuteData(p,a) ICOM_CALL1(GetExecuteData,p,a)
#define IDirect3DExecuteBuffer_Validate(p,a,b,c,d) ICOM_CALL4(Validate,p,a,b,c,d)
#define IDirect3DExecuteBuffer_Optimize(p,a)       ICOM_CALL1(Optimize,p,a)
#endif


/*****************************************************************************
 * IDirect3DDevice interface
 */
#define ICOM_INTERFACE IDirect3DDevice
#define IDirect3DDevice_METHODS \
    ICOM_METHOD3(HRESULT,Initialize,          LPDIRECT3D,lpDirect3D, LPGUID,lpGUID, LPD3DDEVICEDESC,lpD3DDVDesc) \
    ICOM_METHOD2(HRESULT,GetCaps,             LPD3DDEVICEDESC,lpD3DHWDevDesc, LPD3DDEVICEDESC,lpD3DHELDevDesc) \
    ICOM_METHOD2(HRESULT,SwapTextureHandles,  LPDIRECT3DTEXTURE,lpD3Dtex1, LPDIRECT3DTEXTURE,lpD3DTex2) \
    ICOM_METHOD3(HRESULT,CreateExecuteBuffer, LPD3DEXECUTEBUFFERDESC,lpDesc, LPDIRECT3DEXECUTEBUFFER*,lplpDirect3DExecuteBuffer, IUnknown*,pUnkOuter) \
    ICOM_METHOD1(HRESULT,GetStats,            LPD3DSTATS,lpD3DStats) \
    ICOM_METHOD3(HRESULT,Execute,             LPDIRECT3DEXECUTEBUFFER,lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT,lpDirect3DViewport, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,AddViewport,         LPDIRECT3DVIEWPORT,lpDirect3DViewport) \
    ICOM_METHOD1(HRESULT,DeleteViewport,      LPDIRECT3DVIEWPORT,lpDirect3DViewport) \
    ICOM_METHOD3(HRESULT,NextViewport,        LPDIRECT3DVIEWPORT,lpDirect3DViewport, LPDIRECT3DVIEWPORT*,lplpDirect3DViewport, DWORD,dwFlags) \
    ICOM_METHOD4(HRESULT,Pick,                LPDIRECT3DEXECUTEBUFFER,lpDirect3DExecuteBuffer, LPDIRECT3DVIEWPORT,lpDirect3DViewport, DWORD,dwFlags, LPD3DRECT,lpRect) \
    ICOM_METHOD2(HRESULT,GetPickRecords,      LPDWORD,lpCount, LPD3DPICKRECORD,lpD3DPickRec) \
    ICOM_METHOD2(HRESULT,EnumTextureFormats,  LPD3DENUMTEXTUREFORMATSCALLBACK,lpD3DEnumTextureProc, LPVOID,lpArg) \
    ICOM_METHOD1(HRESULT,CreateMatrix,        LPD3DMATRIXHANDLE,lpD3DMatHandle) \
    ICOM_METHOD2(HRESULT,SetMatrix,           D3DMATRIXHANDLE,D3DMatHandle, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD2(HRESULT,GetMatrix,           D3DMATRIXHANDLE,D3DMatHandle, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD1(HRESULT,DeleteMatrix,        D3DMATRIXHANDLE,D3DMatHandle) \
    ICOM_METHOD (HRESULT,BeginScene) \
    ICOM_METHOD (HRESULT,EndScene) \
    ICOM_METHOD1(HRESULT,GetDirect3D,         LPDIRECT3D*,lplpDirect3D)
#define IDirect3DDevice_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DDevice_METHODS
ICOM_DEFINE(IDirect3DDevice,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DDevice_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DDevice_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DDevice_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DDevice methods ***/
#define IDirect3DDevice_Initialize(p,a,b,c)          ICOM_CALL3(Initialize,p,a,b,c)
#define IDirect3DDevice_GetCaps(p,a,b)               ICOM_CALL2(GetCaps,p,a,b)
#define IDirect3DDevice_SwapTextureHandles(p,a,b)    ICOM_CALL2(SwapTextureHandles,p,a,b)
#define IDirect3DDevice_CreateExecuteBuffer(p,a,b,c) ICOM_CALL3(CreateExecuteBuffer,p,a,b,c)
#define IDirect3DDevice_GetStats(p,a)                ICOM_CALL1(GetStats,p,a)
#define IDirect3DDevice_Execute(p,a,b,c)             ICOM_CALL3(Execute,p,a,b,c)
#define IDirect3DDevice_AddViewport(p,a)             ICOM_CALL1(AddViewport,p,a)
#define IDirect3DDevice_DeleteViewport(p,a)          ICOM_CALL1(DeleteViewport,p,a)
#define IDirect3DDevice_NextViewport(p,a,b,c)        ICOM_CALL3(NextViewport,p,a,b,c)
#define IDirect3DDevice_Pick(p,a,b,c,d)              ICOM_CALL4(Pick,p,a,b,c,d)
#define IDirect3DDevice_GetPickRecords(p,a,b)        ICOM_CALL2(GetPickRecords,p,a,b)
#define IDirect3DDevice_EnumTextureFormats(p,a,b)    ICOM_CALL2(EnumTextureFormats,p,a,b)
#define IDirect3DDevice_CreateMatrix(p,a)            ICOM_CALL1(CreateMatrix,p,a)
#define IDirect3DDevice_SetMatrix(p,a,b)             ICOM_CALL2(SetMatrix,p,a,b)
#define IDirect3DDevice_GetMatrix(p,a,b)             ICOM_CALL2(GetMatrix,p,a,b)
#define IDirect3DDevice_DeleteMatrix(p,a)            ICOM_CALL1(DeleteMatrix,p,a)
#define IDirect3DDevice_BeginScene(p)                ICOM_CALL (BeginScene,p)
#define IDirect3DDevice_EndScene(p)                  ICOM_CALL (EndScene,p)
#define IDirect3DDevice_GetDirect3D(p,a)             ICOM_CALL1(GetDirect3D,p,a)
#endif


/*****************************************************************************
 * IDirect3DDevice2 interface
 */
#define ICOM_INTERFACE IDirect3DDevice2
#define IDirect3DDevice2_METHODS \
    ICOM_METHOD2(HRESULT,GetCaps,              LPD3DDEVICEDESC,lpD3DHWDevDesc, LPD3DDEVICEDESC,lpD3DHELDevDesc) \
    ICOM_METHOD2(HRESULT,SwapTextureHandles,   LPDIRECT3DTEXTURE2,lpD3DTex1, LPDIRECT3DTEXTURE2,lpD3DTex2) \
    ICOM_METHOD1(HRESULT,GetStats,             LPD3DSTATS,lpD3DStats) \
    ICOM_METHOD1(HRESULT,AddViewport,          LPDIRECT3DVIEWPORT2,lpDirect3DViewport2) \
    ICOM_METHOD1(HRESULT,DeleteViewport,       LPDIRECT3DVIEWPORT2,lpDirect3DViewport2) \
    ICOM_METHOD3(HRESULT,NextViewport,         LPDIRECT3DVIEWPORT2,lpDirect3DViewport2, LPDIRECT3DVIEWPORT2*,lplpDirect3DViewport2, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,EnumTextureFormats,   LPD3DENUMTEXTUREFORMATSCALLBACK,lpD3DEnumTextureProc, LPVOID,lpArg) \
    ICOM_METHOD (HRESULT,BeginScene) \
    ICOM_METHOD (HRESULT,EndScene) \
    ICOM_METHOD1(HRESULT,GetDirect3D,          LPDIRECT3D2*,lplpDirect3D2) \
    /*** DrawPrimitive API ***/ \
    ICOM_METHOD1(HRESULT,SetCurrentViewport,   LPDIRECT3DVIEWPORT2,lpDirect3DViewport2) \
    ICOM_METHOD1(HRESULT,GetCurrentViewport,   LPDIRECT3DVIEWPORT2*,lplpDirect3DViewport2) \
    ICOM_METHOD2(HRESULT,SetRenderTarget,      LPDIRECTDRAWSURFACE,lpNewRenderTarget, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetRenderTarget,      LPDIRECTDRAWSURFACE*,lplpRenderTarget) \
    ICOM_METHOD3(HRESULT,Begin,                D3DPRIMITIVETYPE,, D3DVERTEXTYPE,, DWORD,) \
    ICOM_METHOD5(HRESULT,BeginIndexed,         D3DPRIMITIVETYPE,d3dptPrimitiveType, D3DVERTEXTYPE,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwNumVertices, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,Vertex,               LPVOID,lpVertexType) \
    ICOM_METHOD1(HRESULT,Index,                WORD,wVertexIndex) \
    ICOM_METHOD1(HRESULT,End,                  DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetRenderState,       D3DRENDERSTATETYPE,dwRenderStateType, LPDWORD,lpdwRenderState) \
    ICOM_METHOD2(HRESULT,SetRenderState,       D3DRENDERSTATETYPE,dwRenderStateType, DWORD,dwRenderState) \
    ICOM_METHOD2(HRESULT,GetLightState,        D3DLIGHTSTATETYPE,dwLightStateType, LPDWORD,lpdwLightState) \
    ICOM_METHOD2(HRESULT,SetLightState,        D3DLIGHTSTATETYPE,dwLightStateType, DWORD,dwLightState) \
    ICOM_METHOD2(HRESULT,SetTransform,         D3DTRANSFORMSTATETYPE,dtstTransformStateType, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD2(HRESULT,GetTransform,         D3DTRANSFORMSTATETYPE,dtstTransformStateType, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD2(HRESULT,MultiplyTransform,    D3DTRANSFORMSTATETYPE,dtstTransformStateType, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD5(HRESULT,DrawPrimitive,        D3DPRIMITIVETYPE,d3dptPrimitiveType, D3DVERTEXTYPE,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitive, D3DPRIMITIVETYPE,d3dptPrimitiveType, D3DVERTEXTYPE,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, LPWORD,dwIndices, DWORD,dwIndexCount, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,SetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus) \
    ICOM_METHOD1(HRESULT,GetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus)
#define IDirect3DDevice2_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DDevice2_METHODS
ICOM_DEFINE(IDirect3DDevice2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
  /*** IUnknown methods ***/
#define IDirect3DDevice2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DDevice2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DDevice2_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DDevice2 methods ***/
#define IDirect3DDevice2_GetCaps(p,a,b)                        ICOM_CALL2(GetCaps,p,a,b)
#define IDirect3DDevice2_SwapTextureHandles(p,a,b)             ICOM_CALL2(SwapTextureHandles,p,a,b)
#define IDirect3DDevice2_GetStats(p,a)                         ICOM_CALL1(GetStats,p,a)
#define IDirect3DDevice2_AddViewport(p,a)                      ICOM_CALL1(AddViewport,p,a)
#define IDirect3DDevice2_DeleteViewport(p,a)                   ICOM_CALL1(DeleteViewport,p,a)
#define IDirect3DDevice2_NextViewport(p,a,b,c)                 ICOM_CALL3(NextViewport,p,a,b,c)
#define IDirect3DDevice2_EnumTextureFormats(p,a,b)             ICOM_CALL2(EnumTextureFormats,p,a,b)
#define IDirect3DDevice2_BeginScene(p)                         ICOM_CALL (BeginScene,p)
#define IDirect3DDevice2_EndScene(p)                           ICOM_CALL (EndScene,p)
#define IDirect3DDevice2_GetDirect3D(p,a)                      ICOM_CALL1(GetDirect3D,p,a)
#define IDirect3DDevice2_SetCurrentViewport(p,a)               ICOM_CALL1(SetCurrentViewport,p,a)
#define IDirect3DDevice2_GetCurrentViewport(p,a)               ICOM_CALL1(GetCurrentViewport,p,a)
#define IDirect3DDevice2_SetRenderTarget(p,a,b)                ICOM_CALL2(SetRenderTarget,p,a,b)
#define IDirect3DDevice2_GetRenderTarget(p,a)                  ICOM_CALL1(GetRenderTarget,p,a)
#define IDirect3DDevice2_Begin(p,a,b,c)                        ICOM_CALL3(Begin,p,a,b,c)
#define IDirect3DDevice2_BeginIndexed(p,a,b,c,d,e)             ICOM_CALL5(BeginIndexed,p,a,b,c,d,e)
#define IDirect3DDevice2_Vertex(p,a)                           ICOM_CALL1(Vertex,p,a)
#define IDirect3DDevice2_Index(p,a)                            ICOM_CALL1(Index,p,a)
#define IDirect3DDevice2_End(p,a)                              ICOM_CALL1(End,p,a)
#define IDirect3DDevice2_GetRenderState(p,a,b)                 ICOM_CALL2(GetRenderState,p,a,b)
#define IDirect3DDevice2_SetRenderState(p,a,b)                 ICOM_CALL2(SetRenderState,p,a,b)
#define IDirect3DDevice2_GetLightState(p,a,b)                  ICOM_CALL2(GetLightState,p,a,b)
#define IDirect3DDevice2_SetLightState(p,a,b)                  ICOM_CALL2(SetLightState,p,a,b)
#define IDirect3DDevice2_SetTransform(p,a,b)                   ICOM_CALL2(SetTransform,p,a,b)
#define IDirect3DDevice2_GetTransform(p,a,b)                   ICOM_CALL2(GetTransform,p,a,b)
#define IDirect3DDevice2_MultiplyTransform(p,a,b)              ICOM_CALL2(MultiplyTransform,p,a,b)
#define IDirect3DDevice2_DrawPrimitive(p,a,b,c,d,e)            ICOM_CALL5(DrawPrimitive,p,a,b,c,d,e)
#define IDirect3DDevice2_DrawIndexedPrimitive(p,a,b,c,d,e,f,g) ICOM_CALL7(DrawIndexedPrimitive,p,a,b,c,d,e,f,g)
#define IDirect3DDevice2_SetClipStatus(p,a)                    ICOM_CALL1(SetClipStatus,p,a)
#define IDirect3DDevice2_GetClipStatus(p,a)                    ICOM_CALL1(GetClipStatus,p,a)
#endif


#endif /* __WINE_D3D_H */
