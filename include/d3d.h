#ifndef __WINE_D3D_H
#define __WINE_D3D_H

#include "ddraw.h"
#include "d3dtypes.h" /* must precede d3dcaps.h */
#include "d3dcaps.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID(IID_IDirect3D,              0x3BBA0080,0x2421,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirect3D2,             0x6aae1ec1,0x662a,0x11d0,0x88,0x9d,0x00,0xaa,0x00,0xbb,0xb7,0x6a);
DEFINE_GUID(IID_IDirect3D3,             0xbb223240,0xe72b,0x11d0,0xa9,0xb4,0x00,0xaa,0x00,0xc0,0x99,0x3e);
DEFINE_GUID(IID_IDirect3D7,             0xf5049e77,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);

DEFINE_GUID(IID_IDirect3DRampDevice,	0xF2086B20,0x259F,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirect3DRGBDevice,	0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirect3DHALDevice,	0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DMMXDevice,	0x881949a1,0xd6f3,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DRefDevice,     0x50936643,0x13e9,0x11d1,0x89,0xaa,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DTnLHalDevice,  0xf5049e78,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);

DEFINE_GUID(IID_IDirect3DDevice,	0x64108800,0x957d,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DDevice2,	0x93281501,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DDevice3,       0xb0ab3b60,0x33d7,0x11d1,0xa9,0x81,0x00,0xc0,0x4f,0xd7,0xb1,0x74);
DEFINE_GUID(IID_IDirect3DDevice7,       0xf5049e79,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);

DEFINE_GUID(IID_IDirect3DTexture,	0x2CDCD9E0,0x25A0,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_GUID(IID_IDirect3DTexture2,	0x93281502,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);

DEFINE_GUID(IID_IDirect3DLight,		0x4417C142,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);

DEFINE_GUID(IID_IDirect3DMaterial,	0x4417C144,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DMaterial2,	0x93281503,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DMaterial3,     0xca9c46f4,0xd3c5,0x11d1,0xb7,0x5a,0x00,0x60,0x08,0x52,0xb3,0x12);

DEFINE_GUID(IID_IDirect3DExecuteBuffer,	0x4417C145,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);

DEFINE_GUID(IID_IDirect3DViewport,	0x4417C146,0x33AD,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_GUID(IID_IDirect3DViewport2,	0x93281500,0x8CF8,0x11D0,0x89,0xAB,0x00,0xA0,0xC9,0x05,0x41,0x29);
DEFINE_GUID(IID_IDirect3DViewport3,     0xb0ab3b61,0x33d7,0x11d1,0xa9,0x81,0x00,0xc0,0x4f,0xd7,0xb1,0x74);

DEFINE_GUID(IID_IDirect3DVertexBuffer,  0x7a503555,0x4a83,0x11d1,0xa5,0xdb,0x00,0xa0,0xc9,0x03,0x67,0xf8);
DEFINE_GUID(IID_IDirect3DVertexBuffer7, 0xf5049e7d,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);


typedef struct IDirect3D              IDirect3D ,*LPDIRECT3D;
typedef struct IDirect3D2             IDirect3D2,*LPDIRECT3D2;
typedef struct IDirect3D3             IDirect3D3,*LPDIRECT3D3;
typedef struct IDirect3D7             IDirect3D7,*LPDIRECT3D7;

typedef struct IDirect3DLight         IDirect3DLight,*LPDIRECT3DLIGHT;

typedef struct IDirect3DDevice        IDirect3DDevice, *LPDIRECT3DDEVICE;
typedef struct IDirect3DDevice2       IDirect3DDevice2, *LPDIRECT3DDEVICE2;
typedef struct IDirect3DDevice3       IDirect3DDevice3, *LPDIRECT3DDEVICE3;
typedef struct IDirect3DDevice7       IDirect3DDevice7, *LPDIRECT3DDEVICE7;

typedef struct IDirect3DViewport      IDirect3DViewport, *LPDIRECT3DVIEWPORT;
typedef struct IDirect3DViewport2     IDirect3DViewport2, *LPDIRECT3DVIEWPORT2;
typedef struct IDirect3DViewport3     IDirect3DViewport3, *LPDIRECT3DVIEWPORT3;

typedef struct IDirect3DMaterial      IDirect3DMaterial, *LPDIRECT3DMATERIAL;
typedef struct IDirect3DMaterial2     IDirect3DMaterial2, *LPDIRECT3DMATERIAL2;
typedef struct IDirect3DMaterial3     IDirect3DMaterial3, *LPDIRECT3DMATERIAL3;

typedef struct IDirect3DTexture       IDirect3DTexture, *LPDIRECT3DTEXTURE;
typedef struct IDirect3DTexture2      IDirect3DTexture2,  *LPDIRECT3DTEXTURE2;

typedef struct IDirect3DExecuteBuffer IDirect3DExecuteBuffer, *LPDIRECT3DEXECUTEBUFFER;

typedef struct IDirect3DVertexBuffer  IDirect3DVertexBuffer, *LPDIRECT3DVERTEXBUFFER;
typedef struct IDirect3DVertexBuffer7 IDirect3DVertexBuffer7, *LPDIRECT3DVERTEXBUFFER7;

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

/* ********************************************************************
   Types and structures
   ******************************************************************** */
typedef DWORD D3DVIEWPORTHANDLE, *LPD3DVIEWPORTHANDLE;


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


/*****************************************************************************
 * IDirect3D3 interface
 */
#define ICOM_INTERFACE IDirect3D3
#define IDirect3D3_METHODS \
    ICOM_METHOD2(HRESULT,EnumDevices,       LPD3DENUMDEVICESCALLBACK,lpEnumDevicesCallback, LPVOID,lpUserArg) \
    ICOM_METHOD2(HRESULT,CreateLight,       LPDIRECT3DLIGHT*,lplpDirect3DLight, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateMaterial,    LPDIRECT3DMATERIAL3*,lplpDirect3DMaterial3, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,CreateViewport,    LPDIRECT3DVIEWPORT3*,lplpD3DViewport3, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,FindDevice,        LPD3DFINDDEVICESEARCH,lpD3DDFS, LPD3DFINDDEVICERESULT,lpD3DFDR) \
    ICOM_METHOD4(HRESULT,CreateDevice,      REFCLSID,rclsid,LPDIRECTDRAWSURFACE4,lpDDS, LPDIRECT3DDEVICE3*,lplpD3DDevice3,LPUNKNOWN,lpUnk) \
    ICOM_METHOD4(HRESULT,CreateVertexBuffer,LPD3DVERTEXBUFFERDESC,lpD3DVertBufDesc,LPDIRECT3DVERTEXBUFFER*,lplpD3DVertBuf,DWORD,dwFlags,LPUNKNOWN,lpUnk) \
    ICOM_METHOD3(HRESULT,EnumZBufferFormats,REFCLSID,riidDevice,LPD3DENUMPIXELFORMATSCALLBACK,lpEnumCallback,LPVOID,lpContext) \
    ICOM_METHOD (HRESULT,EvictManagedTextures)
#define IDirect3D3_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3D3_METHODS
ICOM_DEFINE(IDirect3D3,IUnknown)
#undef ICOM_INTERFACE
  
/*** IUnknown methods ***/
#define IDirect3D3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3D3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3D3_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3D3 methods ***/
#define IDirect3D3_EnumDevices(p,a,b)            ICOM_CALL2(EnumDevices,p,a,b)
#define IDirect3D3_CreateLight(p,a,b)            ICOM_CALL2(CreateLight,p,a,b)
#define IDirect3D3_CreateMaterial(p,a,b)         ICOM_CALL2(CreateMaterial,p,a,b)
#define IDirect3D3_CreateViewport(p,a,b)         ICOM_CALL2(CreateViewport,p,a,b)
#define IDirect3D3_FindDevice(p,a,b)             ICOM_CALL2(FindDevice,p,a,b)
#define IDirect3D3_CreateDevice(p,a,b,c,d)       ICOM_CALL4(CreateDevice,p,a,b,c,d)
#define IDirect3D3_CreateVertexBuffer(p,a,b,c,d) ICOM_CALL4(CreateVertexBuffer,p,a,b,c,d)
#define IDirect3D3_EnumZBufferFormats(p,a,b,c)   ICOM_CALL3(EnumZBufferFormats,p,a,b,c)
#define IDirect3D3_EvictManagedTextures(p)       ICOM_CALL0(EvictManagedTextures,p)

/*****************************************************************************
 * IDirect3D7 interface
 */
#define ICOM_INTERFACE IDirect3D7
#define IDirect3D7_METHODS \
    ICOM_METHOD2(HRESULT,EnumDevices,       LPD3DENUMDEVICESCALLBACK7,lpEnumDevicesCallback, LPVOID,lpUserArg) \
    ICOM_METHOD3(HRESULT,CreateDevice,      REFCLSID,rclsid,LPDIRECTDRAWSURFACE7,lpDDS, LPDIRECT3DDEVICE7*,lplpD3DDevice) \
    ICOM_METHOD3(HRESULT,CreateVertexBuffer,LPD3DVERTEXBUFFERDESC,lpD3DVertBufDesc,LPDIRECT3DVERTEXBUFFER7*,lplpD3DVertBuf,DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,EnumZBufferFormats,REFCLSID,riidDevice,LPD3DENUMPIXELFORMATSCALLBACK,lpEnumCallback,LPVOID,lpContext) \
    ICOM_METHOD (HRESULT,EvictManagedTextures)
#define IDirect3D7_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3D7_METHODS
ICOM_DEFINE(IDirect3D7,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirect3D7_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3D7_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3D7_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3D3 methods ***/
#define IDirect3D7_EnumDevices(p,a,b)            ICOM_CALL2(EnumDevices,p,a,b)
#define IDirect3D7_CreateDevice(p,a,b,c)         ICOM_CALL3(CreateDevice,p,a,b,c)
#define IDirect3D7_CreateVertexBuffer(p,a,b,c)   ICOM_CALL4(CreateVertexBuffer,p,a,b,c)
#define IDirect3D7_EnumZBufferFormats(p,a,b,c)   ICOM_CALL3(EnumZBufferFormats,p,a,b,c)
#define IDirect3D7_EvictManagedTextures(p)       ICOM_CALL0(EvictManagedTextures,p)


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
  
/*** IUnknown methods ***/
#define IDirect3DLight_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DLight_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DLight_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3DLight methods ***/
#define IDirect3DLight_Initialize(p,a) ICOM_CALL1(Initialize,p,a)
#define IDirect3DLight_SetLight(p,a)   ICOM_CALL1(SetLight,p,a)
#define IDirect3DLight_GetLight(p,a)   ICOM_CALL1(GetLight,p,a)


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

  /*** IUnknown methods ***/
#define IDirect3DMaterial2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DMaterial2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DMaterial2_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DMaterial2 methods ***/
#define IDirect3DMaterial2_SetMaterial(p,a) ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DMaterial2_GetMaterial(p,a) ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DMaterial2_GetHandle(p,a,b) ICOM_CALL2(GetHandle,p,a,b)
  

/*****************************************************************************
 * IDirect3DMaterial3 interface
 */
#define ICOM_INTERFACE IDirect3DMaterial3
#define IDirect3DMaterial3_METHODS \
    ICOM_METHOD1(HRESULT,SetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD1(HRESULT,GetMaterial, LPD3DMATERIAL,lpMat) \
    ICOM_METHOD2(HRESULT,GetHandle,   LPDIRECT3DDEVICE3,lpDirect3DDevice2, LPD3DMATERIALHANDLE,lpHandle)
#define IDirect3DMaterial3_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DMaterial3_METHODS
ICOM_DEFINE(IDirect3DMaterial3,IUnknown)
#undef ICOM_INTERFACE

  /*** IUnknown methods ***/
#define IDirect3DMaterial3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DMaterial3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DMaterial3_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DMaterial3 methods ***/
#define IDirect3DMaterial3_SetMaterial(p,a) ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DMaterial3_GetMaterial(p,a) ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DMaterial3_GetHandle(p,a,b) ICOM_CALL2(GetHandle,p,a,b)
  

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

  /*** IUnknown methods ***/
#define IDirect3DTexture2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DTexture2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DTexture2_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DTexture2 methods ***/
#define IDirect3DTexture2_GetHandle(p,a,b)      ICOM_CALL2(GetHandle,p,a,b)
#define IDirect3DTexture2_PaletteChanged(p,a,b) ICOM_CALL2(PaletteChanged,p,a,b)
#define IDirect3DTexture2_Load(p,a)             ICOM_CALL1(Load,p,a)


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

/*****************************************************************************
 * IDirect3DViewport3 interface
 */
#define ICOM_INTERFACE IDirect3DViewport3
#define IDirect3DViewport3_METHODS \
    ICOM_METHOD1(HRESULT,SetBackgroundDepth2,LPDIRECTDRAWSURFACE4,lpDDS) \
    ICOM_METHOD2(HRESULT,GetBackgroundDepth2,LPDIRECTDRAWSURFACE4*,lplpDDS,LPBOOL,lpValid) \
    ICOM_METHOD6(HRESULT,Clear2,DWORD,dwCount,LPD3DRECT,lpRects,DWORD,dwFlags,DWORD,dwColor,D3DVALUE,dvZ,DWORD,dwStencil)
#define IDirect3DViewport3_IMETHODS \
    IDirect3DViewport2_IMETHODS \
    IDirect3DViewport3_METHODS
ICOM_DEFINE(IDirect3DViewport3,IDirect3DViewport)
#undef ICOM_INTERFACE

  /*** IUnknown methods ***/
#define IDirect3DViewport3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DViewport3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DViewport3_Release(p)            ICOM_CALL (Release,p)
/*** IDirect3Viewport methods ***/
#define IDirect3DViewport3_Initialize(p,a)              ICOM_CALL1(Initialize,p,a)
#define IDirect3DViewport3_GetViewport(p,a)             ICOM_CALL1(GetViewport,p,a)
#define IDirect3DViewport3_SetViewport(p,a)             ICOM_CALL1(SetViewport,p,a)
#define IDirect3DViewport3_TransformVertices(p,a,b,c,d) ICOM_CALL4(TransformVertices,p,a,b,c,d)
#define IDirect3DViewport3_LightElements(p,a,b)         ICOM_CALL2(LightElements,p,a,b)
#define IDirect3DViewport3_SetBackground(p,a)           ICOM_CALL1(SetBackground,p,a)
#define IDirect3DViewport3_GetBackground(p,a,b)         ICOM_CALL2(GetBackground,p,a,b)
#define IDirect3DViewport3_SetBackgroundDepth(p,a)      ICOM_CALL1(SetBackgroundDepth,p,a)
#define IDirect3DViewport3_GetBackgroundDepth(p,a,b)    ICOM_CALL2(GetBackgroundDepth,p,a,b)
#define IDirect3DViewport3_Clear(p,a,b,c)               ICOM_CALL3(Clear,p,a,b,c)
#define IDirect3DViewport3_AddLight(p,a)                ICOM_CALL1(AddLight,p,a)
#define IDirect3DViewport3_DeleteLight(p,a)             ICOM_CALL1(DeleteLight,p,a)
#define IDirect3DViewport3_NextLight(p,a,b,c)           ICOM_CALL3(NextLight,p,a,b,c)
  /*** IDirect3DViewport2 methods ***/
#define IDirect3DViewport3_GetViewport3(p,a) ICOM_CALL1(GetViewport2,p,a)
#define IDirect3DViewport3_SetViewport3(p,a) ICOM_CALL1(SetViewport2,p,a)
  /*** IDirect3DViewport3 methods ***/
#define IDirect3DViewport3_SetBackgroundDepth2(p,a)   ICOM_CALL1(SetBackgroundDepth2,p,a)
#define IDirect3DViewport3_GetBackgroundDepth2(p,a,b) ICOM_CALL2(GetBackgroundDepth2,p,a,b)
#define IDirect3DViewport3_Clear2(p,a,b,c,d,e,f)      ICOM_CALL7(Clear2,p,a,b,c,d,e,f)



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

/*****************************************************************************
 * IDirect3DDevice3 interface
 */
#define ICOM_INTERFACE IDirect3DDevice3
#define IDirect3DDevice3_METHODS \
    ICOM_METHOD2(HRESULT,GetCaps,              LPD3DDEVICEDESC,lpD3DHWDevDesc, LPD3DDEVICEDESC,lpD3DHELDevDesc) \
    ICOM_METHOD1(HRESULT,GetStats,             LPD3DSTATS,lpD3DStats) \
    ICOM_METHOD1(HRESULT,AddViewport,          LPDIRECT3DVIEWPORT3,lpDirect3DViewport3) \
    ICOM_METHOD1(HRESULT,DeleteViewport,       LPDIRECT3DVIEWPORT3,lpDirect3DViewport3) \
    ICOM_METHOD3(HRESULT,NextViewport,         LPDIRECT3DVIEWPORT3,lpDirect3DViewport3, LPDIRECT3DVIEWPORT3*,lplpDirect3DViewport3, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,EnumTextureFormats,   LPD3DENUMPIXELFORMATSCALLBACK,lpD3DEnumPixelProc, LPVOID,lpArg) \
    ICOM_METHOD (HRESULT,BeginScene) \
    ICOM_METHOD (HRESULT,EndScene) \
    ICOM_METHOD1(HRESULT,GetDirect3D,          LPDIRECT3D3*,lplpDirect3D3) \
    /*** DrawPrimitive API ***/ \
    ICOM_METHOD1(HRESULT,SetCurrentViewport,   LPDIRECT3DVIEWPORT3,lpDirect3DViewport3) \
    ICOM_METHOD1(HRESULT,GetCurrentViewport,   LPDIRECT3DVIEWPORT3*,lplpDirect3DViewport3) \
    ICOM_METHOD2(HRESULT,SetRenderTarget,      LPDIRECTDRAWSURFACE,lpNewRenderTarget, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetRenderTarget,      LPDIRECTDRAWSURFACE*,lplpRenderTarget) \
    ICOM_METHOD3(HRESULT,Begin,                D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,dwVertexTypeDesc, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,BeginIndexed,         D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwNumVertices, DWORD,dwFlags) \
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
    ICOM_METHOD5(HRESULT,DrawPrimitive,        D3DPRIMITIVETYPE,d3dptPrimitiveType, DWORD,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitive, D3DPRIMITIVETYPE,d3dptPrimitiveType, DWORD,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, LPWORD,dwIndices, DWORD,dwIndexCount, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,SetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus) \
    ICOM_METHOD1(HRESULT,GetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus) \
    ICOM_METHOD5(HRESULT,DrawPrimitiveStrided, D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,dwVertexType,LPD3DDRAWPRIMITIVESTRIDEDDATA,lpD3DDrawPrimStrideData,DWORD,dwVertexCount,DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitiveStrided, D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,dwVertexType,LPD3DDRAWPRIMITIVESTRIDEDDATA,lpD3DDrawPrimStrideData,DWORD,dwVertexCount,LPWORD,lpIndex,DWORD,dwIndexCount,DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,DrawPrimitiveVB,      D3DPRIMITIVETYPE,d3dptPrimitiveType,LPDIRECT3DVERTEXBUFFER,lpD3DVertexBuf,DWORD,dwStartVertex,DWORD,dwNumVertices,DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,DrawIndexedPrimitiveVB,      D3DPRIMITIVETYPE,d3dptPrimitiveType,LPDIRECT3DVERTEXBUFFER,lpD3DVertexBuf,LPWORD,lpwIndices,DWORD,dwIndexCount,DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,ComputeSphereVisibility,     LPD3DVECTOR,lpCenters,LPD3DVALUE,lpRadii,DWORD,dwNumSpheres,DWORD,dwFlags,LPDWORD,lpdwReturnValues) \
    ICOM_METHOD2(HRESULT,GetTexture,           DWORD,dwStage,LPDIRECT3DTEXTURE2*,lplpTexture2) \
    ICOM_METHOD2(HRESULT,SetTexture,           DWORD,dwStage,LPDIRECT3DTEXTURE2,lpTexture2) \
    ICOM_METHOD3(HRESULT,GetTextureStageState, DWORD,dwStage,D3DTEXTURESTAGESTATETYPE,d3dTexStageStateType,LPDWORD,lpdwState) \
    ICOM_METHOD3(HRESULT,SetTextureStageState, DWORD,dwStage,D3DTEXTURESTAGESTATETYPE,d3dTexStageStateType,DWORD,dwState) \
    ICOM_METHOD1(HRESULT,ValidateDevice,       LPDWORD,lpdwPasses)
#define IDirect3DDevice3_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DDevice3_METHODS
ICOM_DEFINE(IDirect3DDevice3,IUnknown)
#undef ICOM_INTERFACE

  /*** IUnknown methods ***/
#define IDirect3DDevice3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DDevice3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DDevice3_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DDevice3 methods ***/
#define IDirect3DDevice3_GetCaps(p,a,b)                        ICOM_CALL2(GetCaps,p,a,b)
#define IDirect3DDevice3_GetStats(p,a)                         ICOM_CALL1(GetStats,p,a)
#define IDirect3DDevice3_AddViewport(p,a)                      ICOM_CALL1(AddViewport,p,a)
#define IDirect3DDevice3_DeleteViewport(p,a)                   ICOM_CALL1(DeleteViewport,p,a)
#define IDirect3DDevice3_NextViewport(p,a,b,c)                 ICOM_CALL3(NextViewport,p,a,b,c)
#define IDirect3DDevice3_EnumTextureFormats(p,a,b)             ICOM_CALL2(EnumTextureFormats,p,a,b)
#define IDirect3DDevice3_BeginScene(p)                         ICOM_CALL (BeginScene,p)
#define IDirect3DDevice3_EndScene(p)                           ICOM_CALL (EndScene,p)
#define IDirect3DDevice3_GetDirect3D(p,a)                      ICOM_CALL1(GetDirect3D,p,a)
#define IDirect3DDevice3_SetCurrentViewport(p,a)               ICOM_CALL1(SetCurrentViewport,p,a)
#define IDirect3DDevice3_GetCurrentViewport(p,a)               ICOM_CALL1(GetCurrentViewport,p,a)
#define IDirect3DDevice3_SetRenderTarget(p,a,b)                ICOM_CALL2(SetRenderTarget,p,a,b)
#define IDirect3DDevice3_GetRenderTarget(p,a)                  ICOM_CALL1(GetRenderTarget,p,a)
#define IDirect3DDevice3_Begin(p,a,b,c)                        ICOM_CALL3(Begin,p,a,b,c)
#define IDirect3DDevice3_BeginIndexed(p,a,b,c,d,e)             ICOM_CALL5(BeginIndexed,p,a,b,c,d,e)
#define IDirect3DDevice3_Vertex(p,a)                           ICOM_CALL1(Vertex,p,a)
#define IDirect3DDevice3_Index(p,a)                            ICOM_CALL1(Index,p,a)
#define IDirect3DDevice3_End(p,a)                              ICOM_CALL1(End,p,a)
#define IDirect3DDevice3_GetRenderState(p,a,b)                 ICOM_CALL2(GetRenderState,p,a,b)
#define IDirect3DDevice3_SetRenderState(p,a,b)                 ICOM_CALL2(SetRenderState,p,a,b)
#define IDirect3DDevice3_GetLightState(p,a,b)                  ICOM_CALL2(GetLightState,p,a,b)
#define IDirect3DDevice3_SetLightState(p,a,b)                  ICOM_CALL2(SetLightState,p,a,b)
#define IDirect3DDevice3_SetTransform(p,a,b)                   ICOM_CALL2(SetTransform,p,a,b)
#define IDirect3DDevice3_GetTransform(p,a,b)                   ICOM_CALL2(GetTransform,p,a,b)
#define IDirect3DDevice3_MultiplyTransform(p,a,b)              ICOM_CALL2(MultiplyTransform,p,a,b)
#define IDirect3DDevice3_DrawPrimitive(p,a,b,c,d,e)            ICOM_CALL5(DrawPrimitive,p,a,b,c,d,e)
#define IDirect3DDevice3_DrawIndexedPrimitive(p,a,b,c,d,e,f,g) ICOM_CALL7(DrawIndexedPrimitive,p,a,b,c,d,e,f,g)
#define IDirect3DDevice3_SetClipStatus(p,a)                    ICOM_CALL1(SetClipStatus,p,a)
#define IDirect3DDevice3_GetClipStatus(p,a)                    ICOM_CALL1(GetClipStatus,p,a)
#define IDirect3DDevice3_DrawPrimitiveStrided(p,a,b,c,d,e)     ICOM_CALL5(DrawPrimitiveStrided,p,a,b,c,d,e)
#define IDirect3DDevice3_DrawIndexedPrimitiveStrided(p,a,b,c,d,e,f,g) ICOM_CALL7(DrawIndexedPrimitiveStrided,p,a,b,c,d,e,f,g)
#define IDirect3DDevice3_DrawPrimitiveVB(p,a,b,c,d,e)          ICOM_CALL5(DrawPrimitiveVB,p,a,b,c,d,e)
#define IDirect3DDevice3_DrawIndexedPrimitiveVB(p,a,b,c,d,e)   ICOM_CALL5(DrawIndexedPrimitiveVB,p,a,b,c,d,e)
#define IDirect3DDevice3_ComputeSphereVisibility(p,a,b,c,d,e)  ICOM_CALL5(ComputeSphereVisibility,p,a,b,c,d,e)
#define IDirect3DDevice3_GetTexture(p,a,b)                     ICOM_CALL2(GetTexture,p,a,b)
#define IDirect3DDevice3_SetTexture(p,a,b)                     ICOM_CALL2(SetTexture,p,a,b)
#define IDirect3DDevice3_GetTextureStageState(p,a,b,c)         ICOM_CALL3(GetTextureStageState,p,a,b,c)
#define IDirect3DDevice3_SetTextureStageState(p,a,b,c)         ICOM_CALL3(SetTextureStageState,p,a,b,c)
#define IDirect3DDevice3_ValidateDevice(p,a)                   ICOM_CALL1(ValidateDevice,p,a)

/*****************************************************************************
 * IDirect3DDevice7 interface
 */
#define ICOM_INTERFACE IDirect3DDevice7
#define IDirect3DDevice7_METHODS \
    ICOM_METHOD1(HRESULT,GetCaps,              LPD3DDEVICEDESC7,lpD3DHELDevDesc) \
    ICOM_METHOD2(HRESULT,EnumTextureFormats,   LPD3DENUMPIXELFORMATSCALLBACK,lpD3DEnumPixelProc, LPVOID,lpArg) \
    ICOM_METHOD (HRESULT,BeginScene) \
    ICOM_METHOD (HRESULT,EndScene) \
    ICOM_METHOD1(HRESULT,GetDirect3D,          LPDIRECT3D7*,lplpDirect3D3) \
    ICOM_METHOD2(HRESULT,SetRenderTarget,      LPDIRECTDRAWSURFACE7,lpNewRenderTarget,DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetRenderTarget,      LPDIRECTDRAWSURFACE7*,lplpRenderTarget) \
    ICOM_METHOD6(HRESULT,Clear,                DWORD,,LPD3DRECT,,DWORD,,D3DCOLOR,,D3DVALUE,,DWORD,) \
    ICOM_METHOD2(HRESULT,SetTransform,         D3DTRANSFORMSTATETYPE,dtstTransformStateType, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD2(HRESULT,GetTransform,         D3DTRANSFORMSTATETYPE,dtstTransformStateType, LPD3DMATRIX,lpD3DMatrix) \
    ICOM_METHOD2(HRESULT,MultiplyTransform,    D3DTRANSFORMSTATETYPE,,LPD3DMATRIX,) \
    ICOM_METHOD1(HRESULT,GetViewport,          LPD3DVIEWPORT7,lpData) \
    ICOM_METHOD1(HRESULT,SetViewport,          LPD3DVIEWPORT7,lpData) \
    ICOM_METHOD1(HRESULT,SetMaterial,          LPD3DMATERIAL7,lpMat) \
    ICOM_METHOD1(HRESULT,GetMaterial,          LPD3DMATERIAL7,lpMat) \
    ICOM_METHOD2(HRESULT,SetLight,             DWORD,,LPD3DLIGHT7,lpLight) \
    ICOM_METHOD2(HRESULT,GetLight,             DWORD,,LPD3DLIGHT7,lpLight) \
    ICOM_METHOD2(HRESULT,SetRenderState,       D3DRENDERSTATETYPE,dwRenderStateType, DWORD,dwRenderState) \
    ICOM_METHOD2(HRESULT,GetRenderState,       D3DRENDERSTATETYPE,dwRenderStateType, LPDWORD,lpdwRenderState) \
    ICOM_METHOD (HRESULT,BeginStateBlock) \
    ICOM_METHOD1(HRESULT,EndStateBlock,        LPDWORD,) \
    ICOM_METHOD1(HRESULT,PreLoad,              LPDIRECTDRAWSURFACE7,) \
    ICOM_METHOD5(HRESULT,DrawPrimitive,        D3DPRIMITIVETYPE,d3dptPrimitiveType, DWORD,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitive, D3DPRIMITIVETYPE,d3dptPrimitiveType, DWORD,d3dvtVertexType, LPVOID,lpvVertices, DWORD,dwVertexCount, LPWORD,dwIndices, DWORD,dwIndexCount, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,SetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus) \
    ICOM_METHOD1(HRESULT,GetClipStatus,        LPD3DCLIPSTATUS,lpD3DClipStatus) \
    ICOM_METHOD5(HRESULT,DrawPrimitiveStrided, D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,dwVertexType,LPD3DDRAWPRIMITIVESTRIDEDDATA,lpD3DDrawPrimStrideData,DWORD,dwVertexCount,DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitiveStrided, D3DPRIMITIVETYPE,d3dptPrimitiveType,DWORD,dwVertexType,LPD3DDRAWPRIMITIVESTRIDEDDATA,lpD3DDrawPrimStrideData,DWORD,dwVertexCount,LPWORD,lpIndex,DWORD,dwIndexCount,DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,DrawPrimitiveVB,      D3DPRIMITIVETYPE,d3dptPrimitiveType,LPDIRECT3DVERTEXBUFFER7,lpD3DVertexBuf,DWORD,dwStartVertex,DWORD,dwNumVertices,DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,DrawIndexedPrimitiveVB, D3DPRIMITIVETYPE,d3dptPrimitiveType,LPDIRECT3DVERTEXBUFFER7,lpD3DVertexBuf,DWORD,,DWORD,,LPWORD,lpwIndices,DWORD,dwIndexCount,DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,ComputeSphereVisibility,     LPD3DVECTOR,lpCenters,LPD3DVALUE,lpRadii,DWORD,dwNumSpheres,DWORD,dwFlags,LPDWORD,lpdwReturnValues) \
    ICOM_METHOD2(HRESULT,GetTexture,           DWORD,dwStage,LPDIRECTDRAWSURFACE7*,) \
    ICOM_METHOD2(HRESULT,SetTexture,           DWORD,dwStage,LPDIRECTDRAWSURFACE7,) \
    ICOM_METHOD3(HRESULT,GetTextureStageState, DWORD,dwStage,D3DTEXTURESTAGESTATETYPE,d3dTexStageStateType,LPDWORD,lpdwState) \
    ICOM_METHOD3(HRESULT,SetTextureStageState, DWORD,dwStage,D3DTEXTURESTAGESTATETYPE,d3dTexStageStateType,DWORD,dwState) \
    ICOM_METHOD1(HRESULT,ValidateDevice,       LPDWORD,lpdwPasses) \
    ICOM_METHOD1(HRESULT,ApplyStateBlock,      DWORD,) \
    ICOM_METHOD1(HRESULT,CaptureStateBlock,    DWORD,) \
    ICOM_METHOD1(HRESULT,DeleteStateBlock,     DWORD,) \
    ICOM_METHOD2(HRESULT,CreateStateBlock,     D3DSTATEBLOCKTYPE,,LPDWORD,) \
    ICOM_METHOD5(HRESULT,Load,                 LPDIRECTDRAWSURFACE7,,LPPOINT,,LPDIRECTDRAWSURFACE7,,LPRECT,,DWORD,) \
    ICOM_METHOD2(HRESULT,LightEnable,          DWORD,,BOOL,) \
    ICOM_METHOD2(HRESULT,GetLightEnable,       DWORD,,BOOL*,) \
    ICOM_METHOD2(HRESULT,SetClipPlane,         DWORD,,D3DVALUE*,) \
    ICOM_METHOD2(HRESULT,GetClipPlane,         DWORD,,D3DVALUE*,) \
    ICOM_METHOD3(HRESULT,GetInfo,              DWORD,,LPVOID,,DWORD, )
#define IDirect3DDevice7_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DDevice7_METHODS
ICOM_DEFINE(IDirect3DDevice7,IUnknown)
#undef ICOM_INTERFACE

#define IDirect3DDevice7_QueryInterface(p,a,b)                        ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DDevice7_AddRef(p)                                    ICOM_CALL (AddRef,p)
#define IDirect3DDevice7_Release(p)                                   ICOM_CALL (Release,p)
#define IDirect3DDevice7_GetCaps(p,a)                                 ICOM_CALL1(GetCaps,p,a)
#define IDirect3DDevice7_EnumTextureFormats(p,a,b)                    ICOM_CALL2(EnumTextureFormats,p,a,b)
#define IDirect3DDevice7_BeginScene(p)                                ICOM_CALL (BeginScene,p)
#define IDirect3DDevice7_EndScene(p)                                  ICOM_CALL (EndScene,p)
#define IDirect3DDevice7_GetDirect3D(p,a)                             ICOM_CALL1(GetDirect3D,p,a)
#define IDirect3DDevice7_SetRenderTarget(p,a,b)                       ICOM_CALL2(SetRenderTarget,p,a,b)
#define IDirect3DDevice7_GetRenderTarget(p,a)                         ICOM_CALL1(GetRenderTarget,p,a)
#define IDirect3DDevice7_Clear(p,a,b,c,d,e,f)                         ICOM_CALL6(Clear,p,a,b,c,d,e,f)
#define IDirect3DDevice7_SetTransform(p,a,b)                          ICOM_CALL2(SetTransform,p,a,b)
#define IDirect3DDevice7_GetTransform(p,a,b)                          ICOM_CALL2(GetTransform,p,a,b)
#define IDirect3DDevice7_SetViewport(p,a)                             ICOM_CALL1(SetViewport,p,a)
#define IDirect3DDevice7_MultiplyTransform(p,a,b)                     ICOM_CALL2(MultiplyTransform,p,a,b)
#define IDirect3DDevice7_GetViewport(p,a)                             ICOM_CALL1(GetViewport,p,a)
#define IDirect3DDevice7_SetMaterial(p,a)                             ICOM_CALL1(SetMaterial,p,a)
#define IDirect3DDevice7_GetMaterial(p,a)                             ICOM_CALL1(GetMaterial,p,a)
#define IDirect3DDevice7_SetLight(p,a,b)                              ICOM_CALL2(SetLight,p,a,b)
#define IDirect3DDevice7_GetLight(p,a,b)                              ICOM_CALL2(GetLight,p,a,b)
#define IDirect3DDevice7_SetRenderState(p,a,b)                        ICOM_CALL2(SetRenderState,p,a,b)
#define IDirect3DDevice7_GetRenderState(p,a,b)                        ICOM_CALL2(GetRenderState,p,a,b)
#define IDirect3DDevice7_BeginStateBlock(p)                           ICOM_CALL (BeginStateBlock,p)
#define IDirect3DDevice7_EndStateBlock(p,a)                           ICOM_CALL1(EndStateBlock,p,a)
#define IDirect3DDevice7_PreLoad(p,a)                                 ICOM_CALL1(PreLoad,p,a)
#define IDirect3DDevice7_DrawPrimitive(p,a,b,c,d,e)                   ICOM_CALL5(DrawPrimitive,p,a,b,c,d,e)
#define IDirect3DDevice7_DrawIndexedPrimitive(p,a,b,c,d,e,f,g)        ICOM_CALL7(DrawIndexedPrimitive,p,a,b,c,d,e,f,g)
#define IDirect3DDevice7_SetClipStatus(p,a)                           ICOM_CALL1(SetClipStatus,p,a)
#define IDirect3DDevice7_GetClipStatus(p,a)                           ICOM_CALL1(GetClipStatus,p,a)
#define IDirect3DDevice7_DrawPrimitiveStrided(p,a,b,c,d,e)            ICOM_CALL5(DrawPrimitiveStrided,p,a,b,c,d,e)
#define IDirect3DDevice7_DrawIndexedPrimitiveStrided(p,a,b,c,d,e,f,g) ICOM_CALL7(DrawIndexedPrimitiveStrided,p,a,b,c,d,e,f,g)
#define IDirect3DDevice7_DrawPrimitiveVB(p,a,b,c,d,e)                 ICOM_CALL5(DrawPrimitiveVB,p,a,b,c,d,e)
#define IDirect3DDevice7_DrawIndexedPrimitiveVB(p,a,b,c,d,e,f,g)      ICOM_CALL7(DrawIndexedPrimitiveVB,p,a,b,c,d,e,f,g)
#define IDirect3DDevice7_ComputeSphereVisibility(p,a,b,c,d,e)         ICOM_CALL5(ComputeSphereVisibility,p,a,b,c,d,e)
#define IDirect3DDevice7_GetTexture(p,a,b)                            ICOM_CALL2(GetTexture,p,a,b)
#define IDirect3DDevice7_SetTexture(p,a,b)                            ICOM_CALL2(SetTexture,p,a,b)
#define IDirect3DDevice7_GetTextureStageState(p,a,b,c)                ICOM_CALL3(GetTextureStageState,p,a,b,c)
#define IDirect3DDevice7_SetTextureStageState(p,a,b,c)                ICOM_CALL3(SetTextureStageState,p,a,b,c)
#define IDirect3DDevice7_ValidateDevice(p,a)                          ICOM_CALL1(ValidateDevice,p,a)
#define IDirect3DDevice7_ApplyStateBlock(p,a)                         ICOM_CALL1(ApplyStateBlock,p,a)
#define IDirect3DDevice7_CaptureStateBlock(p,a)                       ICOM_CALL1(CaptureStateBlock,p,a)
#define IDirect3DDevice7_DeleteStateBlock(p,a)                        ICOM_CALL1(DeleteStateBlock,p,a)
#define IDirect3DDevice7_CreateStateBlock(p,a,b)                      ICOM_CALL2(CreateStateBlock,p,a,b)
#define IDirect3DDevice7_Load(p,a,b,c,d,e)                            ICOM_CALL5(Load,p,a,b,c,d,e)
#define IDirect3DDevice7_LightEnable(p,a,b)                           ICOM_CALL2(LightEnable,p,a,b)
#define IDirect3DDevice7_GetLightEnable(p,a,b)                        ICOM_CALL2(GetLightEnable,p,a,b)
#define IDirect3DDevice7_SetClipPlane(p,a,b)                          ICOM_CALL2(SetClipPlane,p,a,b)
#define IDirect3DDevice7_GetClipPlane(p,a,b)                          ICOM_CALL2(GetClipPlane,p,a,b)
#define IDirect3DDevice7_GetInfo(p,a,b,c)                             ICOM_CALL3(GetInfo,p,a,b,c)



/*****************************************************************************
 * IDirect3DVertexBuffer interface
 */
#define ICOM_INTERFACE IDirect3DVertexBuffer
#define IDirect3DVertexBuffer_METHODS \
    ICOM_METHOD3(HRESULT,Lock,                DWORD,dwFlags,LPVOID*,lplpData,LPDWORD,lpdwSize) \
    ICOM_METHOD (HRESULT,Unlock) \
    ICOM_METHOD7(HRESULT,ProcessVertices,     DWORD,dwVertexOp,DWORD,dwDestIndex,DWORD,dwCount,LPDIRECT3DVERTEXBUFFER,lpSrcBuffer,DWORD,dwSrcIndex,LPDIRECT3DDEVICE3,lpD3DDevice,DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetVertexBufferDesc, LPD3DVERTEXBUFFERDESC,lpD3DVertexBufferDesc) \
    ICOM_METHOD2(HRESULT,Optimize,            LPDIRECT3DDEVICE3, lpD3DDevice,DWORD,dwFlags)
#define IDirect3DVertexBuffer_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DVertexBuffer_METHODS
ICOM_DEFINE(IDirect3DVertexBuffer,IUnknown)
#undef ICOM_INTERFACE

  /*** IUnknown methods ***/
#define IDirect3DVertexBuffer_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DVertexBuffer_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirect3DVertexBuffer_Release(p)            ICOM_CALL (Release,p)
  /*** IDirect3DVertexBuffer methods ***/
#define IDirect3DVertexBuffer_Lock(p,a,b,c)                    ICOM_CALL3(Lock,p,a,b,c)
#define IDirect3DVertexBuffer_Unlock(p)                        ICOM_CALL (Unlock,p)
#define IDirect3DVertexBuffer_ProcessVertices(p,a,b,c,d,e,f,g) ICOM_CALL7(ProcessVertices,p,a,b,c,d,e,f,g)
#define IDirect3DVertexBuffer_GetVertexBufferDesc(p,a)         ICOM_CALL1(GetVertexBufferDesc,p,a)
#define IDirect3DVertexBuffer_Optimize(p,a,b)                  ICOM_CALL2(Optimize,p,a,b)
  
/*****************************************************************************
 * IDirect3DVertexBuffer7 interface
 */
#define ICOM_INTERFACE IDirect3DVertexBuffer7
#define IDirect3DVertexBuffer7_METHODS \
    ICOM_METHOD3(HRESULT,Lock,                   DWORD,dwFlags,LPVOID*,lplpData,LPDWORD,lpdwSize) \
    ICOM_METHOD (HRESULT,Unlock) \
    ICOM_METHOD7(HRESULT,ProcessVertices,        DWORD,dwVertexOp,DWORD,dwDestIndex,DWORD,dwCount,LPDIRECT3DVERTEXBUFFER7,lpSrcBuffer,DWORD,dwSrcIndex,LPDIRECT3DDEVICE7,lpD3DDevice,DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetVertexBufferDesc,    LPD3DVERTEXBUFFERDESC,lpD3DVertexBufferDesc) \
    ICOM_METHOD2(HRESULT,Optimize,               LPDIRECT3DDEVICE7, lpD3DDevice,DWORD,dwFlags) \
    ICOM_METHOD7(HRESULT,ProcessVerticesStrided, DWORD,,DWORD,,DWORD,,LPD3DDRAWPRIMITIVESTRIDEDDATA,,DWORD,,LPDIRECT3DDEVICE7,,DWORD,)
#define IDirect3DVertexBuffer7_IMETHODS \
    IUnknown_IMETHODS \
    IDirect3DVertexBuffer7_METHODS
ICOM_DEFINE(IDirect3DVertexBuffer7,IUnknown)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IDirect3DVertexBuffer7_QueryInterface(p,a,b)                   ICOM_CALL2(QueryInterface,p,a,b)
#define IDirect3DVertexBuffer7_AddRef(p)                               ICOM_CALL (AddRef,p)
#define IDirect3DVertexBuffer7_Release(p)                              ICOM_CALL (Release,p)

/*** IDirect3DVertexBuffer7 methods ***/
#define IDirect3DVertexBuffer7_Lock(p,a,b,c)                           ICOM_CALL3(Lock,p,a,b,c)
#define IDirect3DVertexBuffer7_Unlock(p)                               ICOM_CALL (Unlock,p)
#define IDirect3DVertexBuffer7_ProcessVertices(p,a,b,c,d,e,f,g)        ICOM_CALL7(ProcessVertices,p,a,b,c,d,e,f,g)
#define IDirect3DVertexBuffer7_GetVertexBufferDesc(p,a)                ICOM_CALL1(GetVertexBufferDesc,p,a)
#define IDirect3DVertexBuffer7_Optimize(p,a,b)                         ICOM_CALL2(Optimize,p,a,b)
#define IDirect3DVertexBuffer7_ProcessVerticesStrided(p,a,b,c,d,e,f,g) ICOM_CALL7(ProcessVerticesStrided,p,a,b,c,d,e,f,g)


#endif /* __WINE_D3D_H */
