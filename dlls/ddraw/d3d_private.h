/* Direct3D private include file
   (c) 1998 Lionel ULMER
   
   This files contains all the structure that are not exported
   through d3d.h and all common macros. */

#ifndef __GRAPHICS_WINE_D3D_PRIVATE_H
#define __GRAPHICS_WINE_D3D_PRIVATE_H

/* THIS FILE MUST NOT CONTAIN X11 or MESA DEFINES */

#include "d3d.h"

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirect3DImpl IDirect3DImpl;
typedef struct IDirect3D2Impl IDirect3D2Impl;
typedef struct IDirect3D3Impl IDirect3D3Impl;

typedef struct IDirect3DLightImpl IDirect3DLightImpl;
typedef struct IDirect3DMaterial2Impl IDirect3DMaterial2Impl;
typedef struct IDirect3DTexture2Impl IDirect3DTexture2Impl;
typedef struct IDirect3DViewport2Impl IDirect3DViewport2Impl;
typedef struct IDirect3DExecuteBufferImpl IDirect3DExecuteBufferImpl;
typedef struct IDirect3DDeviceImpl IDirect3DDeviceImpl;
typedef struct IDirect3DDevice2Impl IDirect3DDevice2Impl;

#include "ddraw_private.h"

extern ICOM_VTABLE(IDirect3D)	mesa_d3dvt;
extern ICOM_VTABLE(IDirect3D2)	mesa_d3d2vt;
extern ICOM_VTABLE(IDirect3D3)  mesa_d3d3vt;

/*****************************************************************************
 * IDirect3D implementation structure
 */
struct IDirect3DImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3D);
    DWORD                   ref;
    /* IDirect3D fields */
    IDirectDrawImpl*	ddraw;
    LPVOID		private;
};

/*****************************************************************************
 * IDirect3D2 implementation structure
 */
struct IDirect3D2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3D2);
    DWORD                    ref;
    /* IDirect3D2 fields */
    IDirectDrawImpl*	ddraw;
    LPVOID		private;
};

struct IDirect3D3Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3D3);
    DWORD                    ref;
    /* IDirect3D2 fields */
    IDirectDrawImpl*    ddraw;
    LPVOID              private;
    /* IDirect3D3 fields */
};


extern HRESULT WINAPI IDirect3DImpl_QueryInterface(
    LPDIRECT3D iface,REFIID refiid,LPVOID *obj
);
extern ULONG WINAPI IDirect3DImpl_AddRef(LPDIRECT3D iface);
extern ULONG WINAPI IDirect3DImpl_Release(LPDIRECT3D iface)
;
extern HRESULT WINAPI IDirect3DImpl_Initialize(LPDIRECT3D iface,REFIID refiid);
extern HRESULT WINAPI IDirect3DImpl_EnumDevices(
    LPDIRECT3D iface, LPD3DENUMDEVICESCALLBACK cb, LPVOID context
);
extern HRESULT WINAPI IDirect3DImpl_CreateLight(
    LPDIRECT3D iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3DImpl_CreateMaterial(
    LPDIRECT3D iface, LPDIRECT3DMATERIAL *lpmaterial, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3DImpl_CreateViewport(
    LPDIRECT3D iface, LPDIRECT3DVIEWPORT *lpviewport, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3DImpl_FindDevice(
    LPDIRECT3D iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst)
;
extern HRESULT WINAPI IDirect3D2Impl_QueryInterface(LPDIRECT3D2 iface,REFIID refiid,LPVOID *obj);  
extern ULONG WINAPI IDirect3D2Impl_AddRef(LPDIRECT3D2 iface);
extern ULONG WINAPI IDirect3D2Impl_Release(LPDIRECT3D2 iface);
extern HRESULT WINAPI IDirect3D2Impl_EnumDevices(
    LPDIRECT3D2 iface,LPD3DENUMDEVICESCALLBACK cb, LPVOID context
);
extern HRESULT WINAPI IDirect3D2Impl_CreateLight(
    LPDIRECT3D2 iface, LPDIRECT3DLIGHT *lplight, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3D2Impl_CreateMaterial(
    LPDIRECT3D2 iface, LPDIRECT3DMATERIAL2 *lpmaterial, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3D2Impl_CreateViewport(
    LPDIRECT3D2 iface, LPDIRECT3DVIEWPORT2 *lpviewport, IUnknown *lpunk
);
extern HRESULT WINAPI IDirect3D2Impl_FindDevice(
    LPDIRECT3D2 iface, LPD3DFINDDEVICESEARCH lpfinddevsrc,
    LPD3DFINDDEVICERESULT lpfinddevrst);
extern HRESULT WINAPI IDirect3D2Impl_CreateDevice(
    LPDIRECT3D2 iface, REFCLSID rguid, LPDIRECTDRAWSURFACE surface,
    LPDIRECT3DDEVICE2 *device);

/*****************************************************************************
 * IDirect3DLight implementation structure
 */
struct IDirect3DLightImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DLight);
    DWORD                        ref;
    /* IDirect3DLight fields */
    union {
        IDirect3DImpl*  d3d1;
        IDirect3D2Impl* d3d2;
    } d3d;
    int                 type;
  
    D3DLIGHT2           light;

    /* Chained list used for adding / removing from viewports */
    IDirect3DLightImpl *next, *prev;

    /* Activation function */
    void (*activate)(IDirect3DLightImpl*);
    int                 is_active;
  
    LPVOID		private;
};

/*****************************************************************************
 * IDirect3DMaterial2 implementation structure
 */
struct IDirect3DMaterial2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DMaterial2);
    DWORD                            ref;
    /* IDirect3DMaterial2 fields */
    union {
        IDirect3DImpl*        d3d1;
        IDirect3D2Impl*       d3d2;
    } d3d;
    union {
        IDirect3DDeviceImpl*  active_device1;
        IDirect3DDevice2Impl* active_device2;
    } device;
    int                       use_d3d2;

    D3DMATERIAL               mat;

    void (*activate)(IDirect3DMaterial2Impl* this);
    LPVOID			private;
};

/*****************************************************************************
 * IDirect3DTexture2 implementation structure
 */
struct IDirect3DTexture2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DTexture2);
    DWORD                           ref;
    /* IDirect3DTexture2 fields */
    void*			D3Ddevice; /* (void *) to use the same pointer
					    * for both Direct3D and Direct3D2 */
    IDirectDrawSurface4Impl*	surface;
    LPVOID			private;
};

extern HRESULT WINAPI IDirect3DTexture2Impl_QueryInterface(
    LPDIRECT3DTEXTURE2 iface, REFIID riid, LPVOID* ppvObj
);
extern ULONG WINAPI IDirect3DTexture2Impl_AddRef(LPDIRECT3DTEXTURE2 iface);
extern ULONG WINAPI IDirect3DTexture2Impl_Release(LPDIRECT3DTEXTURE2 iface);
extern HRESULT WINAPI IDirect3DTextureImpl_GetHandle(LPDIRECT3DTEXTURE iface,
						 LPDIRECT3DDEVICE lpD3DDevice,
						 LPD3DTEXTUREHANDLE lpHandle)
;
extern HRESULT WINAPI IDirect3DTextureImpl_Initialize(LPDIRECT3DTEXTURE iface,
					  LPDIRECT3DDEVICE lpD3DDevice,
					  LPDIRECTDRAWSURFACE lpSurface)
;
extern HRESULT WINAPI IDirect3DTextureImpl_Unload(LPDIRECT3DTEXTURE iface);
extern HRESULT WINAPI IDirect3DTexture2Impl_GetHandle(
    LPDIRECT3DTEXTURE2 iface, LPDIRECT3DDEVICE2 lpD3DDevice2,
    LPD3DTEXTUREHANDLE lpHandle
);
extern HRESULT WINAPI IDirect3DTexture2Impl_PaletteChanged(
    LPDIRECT3DTEXTURE2 iface, DWORD dwStart, DWORD dwCount
);
extern HRESULT WINAPI IDirect3DTexture2Impl_Load(
    LPDIRECT3DTEXTURE2 iface, LPDIRECT3DTEXTURE2 lpD3DTexture2
);

/*****************************************************************************
 * IDirect3DViewport2 implementation structure
 */
struct IDirect3DViewport2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DViewport2);
    DWORD                            ref;
    /* IDirect3DViewport2 fields */
    union {
        IDirect3DImpl*        d3d1;
        IDirect3D2Impl*       d3d2;
    } d3d;
    /* If this viewport is active for one device, put the device here */
    union {
        IDirect3DDeviceImpl*	active_device1;
        IDirect3DDevice2Impl*	active_device2;
    } device;
    int				use_d3d2;

    union {
        D3DVIEWPORT		vp1;
        D3DVIEWPORT2		vp2;
    } viewport;
    int				use_vp2;

  /* Activation function */
  void (*activate)(IDirect3DViewport2Impl*);
  
  /* Field used to chain viewports together */
  IDirect3DViewport2Impl*	next;

  /* Lights list */
  IDirect3DLightImpl*		lights;
  
  LPVOID			private;
};

extern HRESULT WINAPI IDirect3DViewport2Impl_QueryInterface(
    LPDIRECT3DVIEWPORT2 iface, REFIID riid, LPVOID* ppvObj
);
extern ULONG WINAPI IDirect3DViewport2Impl_AddRef(LPDIRECT3DVIEWPORT2 iface)
;
extern ULONG WINAPI IDirect3DViewport2Impl_Release(LPDIRECT3DVIEWPORT2 iface)
;
extern HRESULT WINAPI IDirect3DViewport2Impl_Initialize(
    LPDIRECT3DVIEWPORT2 iface, LPDIRECT3D d3d
);
extern HRESULT WINAPI IDirect3DViewport2Impl_GetViewport(
    LPDIRECT3DVIEWPORT2 iface, LPD3DVIEWPORT lpvp
);
extern HRESULT WINAPI IDirect3DViewport2Impl_SetViewport(
    LPDIRECT3DVIEWPORT2 iface,LPD3DVIEWPORT lpvp
);
extern HRESULT WINAPI IDirect3DViewport2Impl_TransformVertices(
    LPDIRECT3DVIEWPORT2 iface,DWORD dwVertexCount,LPD3DTRANSFORMDATA lpData,
    DWORD dwFlags,LPDWORD lpOffScreen
);
extern HRESULT WINAPI IDirect3DViewport2Impl_LightElements(
    LPDIRECT3DVIEWPORT2 iface,DWORD dwElementCount,LPD3DLIGHTDATA lpData
);
extern HRESULT WINAPI IDirect3DViewport2Impl_SetBackground(
    LPDIRECT3DVIEWPORT2 iface, D3DMATERIALHANDLE hMat
);
extern HRESULT WINAPI IDirect3DViewport2Impl_GetBackground(
    LPDIRECT3DVIEWPORT2 iface,LPD3DMATERIALHANDLE lphMat,LPBOOL lpValid
);
extern HRESULT WINAPI IDirect3DViewport2Impl_SetBackgroundDepth(
    LPDIRECT3DVIEWPORT2 iface,LPDIRECTDRAWSURFACE lpDDSurface
);
extern HRESULT WINAPI IDirect3DViewport2Impl_GetBackgroundDepth(
    LPDIRECT3DVIEWPORT2 iface,LPDIRECTDRAWSURFACE* lplpDDSurface,LPBOOL lpValid
);
extern HRESULT WINAPI IDirect3DViewport2Impl_Clear(
    LPDIRECT3DVIEWPORT2 iface, DWORD dwCount, LPD3DRECT lpRects, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DViewport2Impl_AddLight(
    LPDIRECT3DVIEWPORT2 iface,LPDIRECT3DLIGHT lpLight
);
extern HRESULT WINAPI IDirect3DViewport2Impl_DeleteLight(
    LPDIRECT3DVIEWPORT2 iface,LPDIRECT3DLIGHT lpLight
);
extern HRESULT WINAPI IDirect3DViewport2Impl_NextLight(
    LPDIRECT3DVIEWPORT2 iface, LPDIRECT3DLIGHT lpLight,
    LPDIRECT3DLIGHT* lplpLight, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DViewport2Impl_GetViewport2(
    LPDIRECT3DVIEWPORT2 iface, LPD3DVIEWPORT2 lpViewport2
);
extern HRESULT WINAPI IDirect3DViewport2Impl_SetViewport2(
    LPDIRECT3DVIEWPORT2 iface, LPD3DVIEWPORT2 lpViewport2
);

/*****************************************************************************
 * IDirect3DExecuteBuffer implementation structure
 */
struct IDirect3DExecuteBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DExecuteBuffer);
    DWORD                                ref;
    /* IDirect3DExecuteBuffer fields */
    IDirect3DDeviceImpl* d3ddev;

    D3DEXECUTEBUFFERDESC desc;
    D3DEXECUTEDATA data;

    /* This buffer will store the transformed vertices */
    void* vertex_data;
    D3DVERTEXTYPE vertex_type;

    /* This flags is set to TRUE if we allocated ourselves the
       data buffer */
    BOOL need_free;

    void (*execute)(IDirect3DExecuteBuffer* this,
                    IDirect3DDevice* dev,
                    IDirect3DViewport2* vp);
    LPVOID private;
};
extern LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(IDirect3DDeviceImpl* d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);

/*****************************************************************************
 * IDirect3DDevice implementation structure
 */
struct IDirect3DDeviceImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DDevice);
    DWORD                         ref;
    /* IDirect3DDevice fields */
    IDirect3DImpl*          d3d;
    IDirectDrawSurfaceImpl* surface;

    IDirect3DViewport2Impl*  viewport_list;
    IDirect3DViewport2Impl*  current_viewport;

    void (*set_context)(IDirect3DDeviceImpl*);
    LPVOID		private;
};

/*****************************************************************************
 * IDirect3DDevice2 implementation structure
 */
struct IDirect3DDevice2Impl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirect3DDevice2);
    DWORD                          ref;
    /* IDirect3DDevice fields */
    IDirect3D2Impl*         d3d;
    IDirectDrawSurfaceImpl* surface;

    IDirect3DViewport2Impl* viewport_list;
    IDirect3DViewport2Impl* current_viewport;

    void (*set_context)(IDirect3DDevice2Impl*);
    LPVOID		private;
};
extern HRESULT WINAPI IDirect3DDevice2Impl_QueryInterface(
    LPDIRECT3DDEVICE2 iface, REFIID riid, LPVOID* ppvObj
);
extern ULONG WINAPI IDirect3DDevice2Impl_AddRef(LPDIRECT3DDEVICE2 iface);
extern ULONG WINAPI IDirect3DDevice2Impl_Release(LPDIRECT3DDEVICE2 iface)
;
extern HRESULT WINAPI IDirect3DDevice2Impl_GetCaps(
    LPDIRECT3DDEVICE2 iface, LPD3DDEVICEDESC lpdescsoft,
    LPD3DDEVICEDESC lpdeschard
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SwapTextureHandles(
    LPDIRECT3DDEVICE2 iface,LPDIRECT3DTEXTURE2 lptex1,LPDIRECT3DTEXTURE2 lptex2
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetStats(
    LPDIRECT3DDEVICE2 iface, LPD3DSTATS lpstats)
;
extern HRESULT WINAPI IDirect3DDevice2Impl_AddViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp
);
extern HRESULT WINAPI IDirect3DDevice2Impl_DeleteViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp)
;
extern HRESULT WINAPI IDirect3DDevice2Impl_NextViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp,
    LPDIRECT3DVIEWPORT2* lplpvp, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_EnumTextureFormats(
    LPDIRECT3DDEVICE2 iface, LPD3DENUMTEXTUREFORMATSCALLBACK cb, LPVOID context
);
extern HRESULT WINAPI IDirect3DDevice2Impl_BeginScene(LPDIRECT3DDEVICE2 iface);
extern HRESULT WINAPI IDirect3DDevice2Impl_EndScene(LPDIRECT3DDEVICE2 iface);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetDirect3D(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3D2 *lpd3d2
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetCurrentViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 lpvp
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetCurrentViewport(
    LPDIRECT3DDEVICE2 iface, LPDIRECT3DVIEWPORT2 *lplpvp
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetRenderTarget(
    LPDIRECT3DDEVICE2 iface, LPDIRECTDRAWSURFACE lpdds, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetRenderTarget(
    LPDIRECT3DDEVICE2 iface, LPDIRECTDRAWSURFACE *lplpdds
);
extern HRESULT WINAPI IDirect3DDevice2Impl_Begin(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_BeginIndexed(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvert, DWORD numvert, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_Vertex(
    LPDIRECT3DDEVICE2 iface,LPVOID lpvert
);
extern HRESULT WINAPI IDirect3DDevice2Impl_Index(LPDIRECT3DDEVICE2 iface, WORD index);
extern HRESULT WINAPI IDirect3DDevice2Impl_End(LPDIRECT3DDEVICE2 iface,DWORD dwFlags);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetRenderState(
    LPDIRECT3DDEVICE2 iface, D3DRENDERSTATETYPE d3drs, LPDWORD lprstate
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetRenderState(
    LPDIRECT3DDEVICE2 iface, D3DRENDERSTATETYPE dwRenderStateType,
    DWORD dwRenderState
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetLightState(
    LPDIRECT3DDEVICE2 iface, D3DLIGHTSTATETYPE d3dls, LPDWORD lplstate
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetLightState(
    LPDIRECT3DDEVICE2 iface, D3DLIGHTSTATETYPE dwLightStateType,
    DWORD dwLightState
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
);
extern HRESULT WINAPI IDirect3DDevice2Impl_MultiplyTransform(
    LPDIRECT3DDEVICE2 iface, D3DTRANSFORMSTATETYPE d3dts, LPD3DMATRIX lpmatrix
);
extern HRESULT WINAPI IDirect3DDevice2Impl_DrawPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_DrawIndexedPrimitive(
    LPDIRECT3DDEVICE2 iface, D3DPRIMITIVETYPE d3dp, D3DVERTEXTYPE d3dv,
    LPVOID lpvertex, DWORD vertcount, LPWORD lpindexes, DWORD indexcount,
    DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDevice2Impl_SetClipStatus(
    LPDIRECT3DDEVICE2 iface, LPD3DCLIPSTATUS lpcs
);
extern HRESULT WINAPI IDirect3DDevice2Impl_GetClipStatus(
    LPDIRECT3DDEVICE2 iface, LPD3DCLIPSTATUS lpcs
);
extern HRESULT WINAPI IDirect3DDeviceImpl_QueryInterface(
    LPDIRECT3DDEVICE iface, REFIID riid, LPVOID* ppvObj
);
extern ULONG WINAPI IDirect3DDeviceImpl_AddRef(LPDIRECT3DDEVICE iface);
extern ULONG WINAPI IDirect3DDeviceImpl_Release(LPDIRECT3DDEVICE iface);
extern HRESULT WINAPI IDirect3DDeviceImpl_Initialize(
    LPDIRECT3DDEVICE iface, LPDIRECT3D lpd3d, LPGUID lpGUID,
    LPD3DDEVICEDESC lpd3ddvdesc
);
extern HRESULT WINAPI IDirect3DDeviceImpl_GetCaps(
    LPDIRECT3DDEVICE iface, LPD3DDEVICEDESC lpD3DHWDevDesc,
    LPD3DDEVICEDESC lpD3DSWDevDesc
);
extern HRESULT WINAPI IDirect3DDeviceImpl_SwapTextureHandles(
    LPDIRECT3DDEVICE iface, LPDIRECT3DTEXTURE lpD3DTex1,
    LPDIRECT3DTEXTURE lpD3DTex2
);
extern HRESULT WINAPI IDirect3DDeviceImpl_CreateExecuteBuffer(
    LPDIRECT3DDEVICE iface, LPD3DEXECUTEBUFFERDESC lpDesc,
    LPDIRECT3DEXECUTEBUFFER *lplpDirect3DExecuteBuffer, IUnknown *pUnkOuter
);
extern HRESULT WINAPI IDirect3DDeviceImpl_GetStats(
    LPDIRECT3DDEVICE iface, LPD3DSTATS lpD3DStats
);
extern HRESULT WINAPI IDirect3DDeviceImpl_Execute(
    LPDIRECT3DDEVICE iface, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
    LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDeviceImpl_AddViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp
);
extern HRESULT WINAPI IDirect3DDeviceImpl_DeleteViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp
);
extern HRESULT WINAPI IDirect3DDeviceImpl_NextViewport(
    LPDIRECT3DDEVICE iface, LPDIRECT3DVIEWPORT lpvp,
    LPDIRECT3DVIEWPORT* lplpvp, DWORD dwFlags
);
extern HRESULT WINAPI IDirect3DDeviceImpl_Pick(
    LPDIRECT3DDEVICE iface, LPDIRECT3DEXECUTEBUFFER lpDirect3DExecuteBuffer,
    LPDIRECT3DVIEWPORT lpDirect3DViewport, DWORD dwFlags, LPD3DRECT lpRect
);
extern HRESULT WINAPI IDirect3DDeviceImpl_GetPickRecords(
    LPDIRECT3DDEVICE iface, LPDWORD lpCount, LPD3DPICKRECORD lpD3DPickRec
);
extern HRESULT WINAPI IDirect3DDeviceImpl_EnumTextureFormats(
    LPDIRECT3DDEVICE iface,LPD3DENUMTEXTUREFORMATSCALLBACK lpd3dEnumTextureProc,
    LPVOID lpArg
);
extern HRESULT WINAPI IDirect3DDeviceImpl_CreateMatrix(
    LPDIRECT3DDEVICE iface, LPD3DMATRIXHANDLE lpD3DMatHandle
)
;
extern HRESULT WINAPI IDirect3DDeviceImpl_SetMatrix(
    LPDIRECT3DDEVICE iface, D3DMATRIXHANDLE d3dMatHandle,
    const LPD3DMATRIX lpD3DMatrix)
;
extern HRESULT WINAPI IDirect3DDeviceImpl_GetMatrix(
    LPDIRECT3DDEVICE iface,D3DMATRIXHANDLE D3DMatHandle,LPD3DMATRIX lpD3DMatrix
);
extern HRESULT WINAPI IDirect3DDeviceImpl_DeleteMatrix(
    LPDIRECT3DDEVICE iface, D3DMATRIXHANDLE d3dMatHandle
);
extern HRESULT WINAPI IDirect3DDeviceImpl_BeginScene(LPDIRECT3DDEVICE iface)
;
extern HRESULT WINAPI IDirect3DDeviceImpl_EndScene(LPDIRECT3DDEVICE iface)
;
extern HRESULT WINAPI IDirect3DDeviceImpl_GetDirect3D(
    LPDIRECT3DDEVICE iface, LPDIRECT3D *lpDirect3D
);

/* All non-static functions 'exported' by various sub-objects */
extern LPDIRECT3DTEXTURE2 d3dtexture2_create(IDirectDrawSurface4Impl* surf);
extern LPDIRECT3DTEXTURE d3dtexture_create(IDirectDrawSurface4Impl* surf);

extern LPDIRECT3DLIGHT d3dlight_create_dx3(IDirect3DImpl* d3d1);
extern LPDIRECT3DLIGHT d3dlight_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DEXECUTEBUFFER d3dexecutebuffer_create(IDirect3DDeviceImpl* d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc);

extern LPDIRECT3DMATERIAL d3dmaterial_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DMATERIAL2 d3dmaterial2_create(IDirect3D2Impl* d3d2);

extern LPDIRECT3DVIEWPORT d3dviewport_create(IDirect3DImpl* d3d1);
extern LPDIRECT3DVIEWPORT2 d3dviewport2_create(IDirect3D2Impl* d3d2);

extern int is_OpenGL_dx3(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDeviceImpl** device);
extern int d3d_OpenGL_dx3(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int d3d_OpenGL(LPD3DENUMDEVICESCALLBACK cb, LPVOID context) ;
extern int is_OpenGL(REFCLSID rguid, IDirectDrawSurfaceImpl* surface, IDirect3DDevice2Impl** device, IDirect3D2Impl* d3d);


extern void _dump_renderstate(D3DRENDERSTATETYPE type, DWORD value);

#define dump_mat(mat) \
    TRACE("%f %f %f %f\n", (mat)->_11, (mat)->_12, (mat)->_13, (mat)->_14); \
    TRACE("%f %f %f %f\n", (mat)->_21, (mat)->_22, (mat)->_23, (mat)->_24); \
    TRACE("%f %f %f %f\n", (mat)->_31, (mat)->_32, (mat)->_33, (mat)->_34); \
    TRACE("%f %f %f %f\n", (mat)->_41, (mat)->_42, (mat)->_43, (mat)->_44);

#endif /* __GRAPHICS_WINE_D3D_PRIVATE_H */
