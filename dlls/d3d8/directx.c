/*
 * IDirect3D8 implementation
 *
 * Copyright 2002 Jason Edmeades
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

#include "config.h"

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

#define NUM_MODES 10
static const int modes[NUM_MODES][3] = {
    {640, 480, 85},
    {800, 600, 85},
    {1024, 768, 85},
    {1152, 864, 85},
    {1280, 768, 85},
    {1280, 960, 85},
    {1280, 1024, 85},
    {1600, 900, 85},
    {1600, 1024, 85},
    {1600, 1200, 85}
};

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;
    return display;
}


/* IDirect3D IUnknown parts follow: */
HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3D8Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D8)) {
        IDirect3D8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface) {
    ICOM_THIS(IDirect3D8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface) {
    ICOM_THIS(IDirect3D8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

/* IDirect3D Interface follow: */
HRESULT  WINAPI  IDirect3D8Impl_RegisterSoftwareDevice     (LPDIRECT3D8 iface, void* pInitializeFunction) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(%p): stub\n", This, pInitializeFunction);
    return D3D_OK;
}

UINT     WINAPI  IDirect3D8Impl_GetAdapterCount            (LPDIRECT3D8 iface) {
    ICOM_THIS(IDirect3D8Impl,iface);
    /* FIXME: Set to one for now to imply the display */
    TRACE("(%p): Mostly stub, only returns primary display\n", This);
    return 1;
}

HRESULT  WINAPI  IDirect3D8Impl_GetAdapterIdentifier       (LPDIRECT3D8 iface,
                                                            UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8* pIdentifier) {
    ICOM_THIS(IDirect3D8Impl,iface);

    TRACE("(%p}->(Adapter: %d, Flags: %lx, pId=%p)\n", This, Adapter, Flags, pIdentifier);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        strcpy(pIdentifier->Driver, "Display");
        strcpy(pIdentifier->Description, "Direct3D Display");
        pIdentifier->DriverVersion.s.HighPart = 1;
        pIdentifier->DriverVersion.s.LowPart = 0;
        pIdentifier->VendorId = 0;
        pIdentifier->DeviceId = 0;
        pIdentifier->SubSysId = 0;
        pIdentifier->Revision = 0;
        /*FIXME: memcpy(&pIdentifier->DeviceIdentifier, ??, sizeof(??GUID)); */
        if (Flags & D3DENUM_NO_WHQL_LEVEL ) {
            pIdentifier->WHQLLevel = 0;
        } else {
            pIdentifier->WHQLLevel = 1;
        }
    } else {
        FIXME("Adapter not primary display\n");
    }

    return D3D_OK;
}

UINT     WINAPI  IDirect3D8Impl_GetAdapterModeCount        (LPDIRECT3D8 iface,
                                                            UINT Adapter) {
    ICOM_THIS(IDirect3D8Impl,iface);

    TRACE("(%p}->(Adapter: %d)\n", This, Adapter);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        int maxWidth        = GetSystemMetrics(SM_CXSCREEN);
        int maxHeight       = GetSystemMetrics(SM_CYSCREEN);
        int i;

        for (i=0; i<NUM_MODES; i++) {
            if (modes[i][0] > maxWidth || modes[i][1] > maxHeight) {
                return i+1;
            }
        }
        return NUM_MODES+1;
    } else {
        FIXME("Adapter not primary display\n");
    }

    return 0;
}

HRESULT  WINAPI  IDirect3D8Impl_EnumAdapterModes           (LPDIRECT3D8 iface,
                                                            UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    ICOM_THIS(IDirect3D8Impl,iface);

    TRACE("(%p}->(Adapter: %d, mode: %d, pMode=%p)\n", This, Adapter, Mode, pMode);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        HDC hdc;
        int bpp = 0;

        if (Mode == 0) {
            pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
            pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
            pMode->RefreshRate  = 85; /*FIXME: How to identify? */
        } else if (Mode < (NUM_MODES+1)) {
            pMode->Width        = modes[Mode-1][0];
            pMode->Height       = modes[Mode-1][1];
            pMode->RefreshRate  = modes[Mode-1][2];
        } else {
            TRACE("Requested mode out of range %d\n", Mode);
            return D3DERR_INVALIDCALL;
        }

        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);

        switch (bpp) {
        case  8: pMode->Format       = D3DFMT_R3G3B2;   break;
        case 16: pMode->Format       = D3DFMT_R5G6B5;   break;
        case 24: pMode->Format       = D3DFMT_R5G6B5;   break; /* Make 24bit appear as 16 bit */
        case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
        default: pMode->Format       = D3DFMT_UNKNOWN;
        }
        TRACE("W %d H %d rr %d fmt %x\n", pMode->Width, pMode->Height, pMode->RefreshRate, pMode->Format);

    } else {
        FIXME("Adapter not primary display\n");
    }

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_GetAdapterDisplayMode      (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDISPLAYMODE* pMode) {
    ICOM_THIS(IDirect3D8Impl,iface);
    TRACE("(%p}->(Adapter: %d, pMode: %p)\n", This, Adapter, pMode);

    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        return D3DERR_INVALIDCALL;
    }

    if (Adapter == 0) { /* Display */
        HDC hdc;
        int bpp = 0;

        pMode->Width        = GetSystemMetrics(SM_CXSCREEN);
        pMode->Height       = GetSystemMetrics(SM_CYSCREEN);
        pMode->RefreshRate  = 85; /*FIXME: How to identify? */

        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);

        switch (bpp) {
        case  8: pMode->Format       = D3DFMT_R3G3B2;   break;
        case 16: pMode->Format       = D3DFMT_R5G6B5;   break;
        case 24: pMode->Format       = D3DFMT_R5G6B5;   break; /* Make 24bit appear as 16 bit */
        case 32: pMode->Format       = D3DFMT_A8R8G8B8; break;
        default: pMode->Format       = D3DFMT_UNKNOWN;
        }

    } else {
        FIXME("Adapter not primary display\n");
    }

    TRACE("returning w:%d, h:%d, ref:%d, fmt:%x\n", pMode->Width,
          pMode->Height, pMode->RefreshRate, pMode->Format);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceType            (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
                                                            D3DFORMAT BackBufferFormat, BOOL Windowed) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, CheckType:%x, DispFmt:%x, BackBuf:%x, Win? %d): stub\n", This, Adapter, CheckType,
          DisplayFormat, BackBufferFormat, Windowed);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat          (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType: %x, AdptFmt: %d, Use: %ld, ResTyp: %x, CheckFmt: %d)\n", This, Adapter, DeviceType,
          AdapterFormat, Usage, RType, CheckFormat);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceMultiSampleType (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
                                                            BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType: %x, SurfFmt: %x, Win? %d, MultiSamp: %x)\n", This, Adapter, DeviceType,
          SurfaceFormat, Windowed, MultiSampleType);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDepthStencilMatch     (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType: %x, AdptFmt: %x, RendrTgtFmt: %x, DepthStencilFmt: %x)\n", This, Adapter, DeviceType,
          AdapterFormat, RenderTargetFormat, DepthStencilFormat);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps              (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    ICOM_THIS(IDirect3D8Impl,iface);
    TRACE("(%p)->(Adptr:%d, DevType: %x, pCaps: %p)\n", This, Adapter, DeviceType, pCaps);


    /* NOTE: Most of the values here are complete garbage for now */
    pCaps->DeviceType = (DeviceType == D3DDEVTYPE_HAL) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;  /* Not quite true, but use h/w supported by opengl I suppose */
    pCaps->AdapterOrdinal = Adapter;

    pCaps->Caps = 0;
    pCaps->Caps2 = D3DCAPS2_CANRENDERWINDOWED;
    pCaps->Caps3 = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
    pCaps->PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE;

    pCaps->CursorCaps = 0;

    pCaps->DevCaps = D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_PUREDEVICE;

    pCaps->PrimitiveMiscCaps = D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_COLORWRITEENABLE | D3DPMISCCAPS_CLIPTLVERTS  |
                               D3DPMISCCAPS_CLIPPLANESCALEDPOINTS | D3DPMISCCAPS_MASKZ; /*NOT: D3DPMISCCAPS_TSSARGTEMP*/
    pCaps->RasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_PAT;
    pCaps->ZCmpCaps = D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
                      D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL;

    pCaps->SrcBlendCaps  = 0xFFFFFFFF;   /*FIXME: Tidy up later */
    pCaps->DestBlendCaps = 0xFFFFFFFF;   /*FIXME: Tidy up later */
    pCaps->AlphaCmpCaps  = 0xFFFFFFFF;   /*FIXME: Tidy up later */
    pCaps->ShadeCaps = D3DPSHADECAPS_SPECULARGOURAUDRGB | D3DPSHADECAPS_COLORGOURAUDRGB ;
    pCaps->TextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_ALPHAPALETTE | D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_VOLUMEMAP | D3DPTEXTURECAPS_MIPMAP;
    pCaps->TextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR  | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MINFPOINT |
                               D3DPTFILTERCAPS_MIPFLINEAR  | D3DPTFILTERCAPS_MIPFPOINT ;
    pCaps->CubeTextureFilterCaps = 0;
    pCaps->VolumeTextureFilterCaps = 0;
    pCaps->TextureAddressCaps = D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_WRAP;
#if defined(GL_VERSION_1_3)
    pCaps->TextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
#endif
    pCaps->VolumeTextureAddressCaps = 0;

    pCaps->LineCaps = D3DLINECAPS_TEXTURE | D3DLINECAPS_ZTEST;

    pCaps->MaxTextureWidth = 16384;
    pCaps->MaxTextureHeight = 16384;
    pCaps->MaxVolumeExtent = 0;

    pCaps->MaxTextureRepeat = 32768;
    pCaps->MaxTextureAspectRatio = 32768;
    pCaps->MaxAnisotropy = 0;
    pCaps->MaxVertexW = 1.0;

    pCaps->GuardBandLeft = 0;
    pCaps->GuardBandTop = 0;
    pCaps->GuardBandRight = 0;
    pCaps->GuardBandBottom = 0;

    pCaps->ExtentsAdjust = 0;

    pCaps->StencilCaps = D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INCRSAT | 
                         D3DSTENCILCAPS_INVERT  | D3DSTENCILCAPS_KEEP    | 
                         D3DSTENCILCAPS_REPLACE | D3DSTENCILCAPS_ZERO /* | D3DSTENCILCAPS_DECR | D3DSTENCILCAPS_INCR */;

    pCaps->FVFCaps = D3DFVFCAPS_PSIZE | 0x80000;

    pCaps->TextureOpCaps = D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_ADDSIGNED | D3DTEXOPCAPS_ADDSIGNED2X |
                           D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE4X |
                           D3DTEXOPCAPS_SELECTARG1  | D3DTEXOPCAPS_DISABLE;
#if defined(GL_VERSION_1_3)
    pCaps->TextureOpCaps |= D3DTEXOPCAPS_DOTPRODUCT3;
#endif
              /* FIXME: Add D3DTEXOPCAPS_ADDSMOOTH D3DTEXOPCAPS_BLENDCURRENTALPHA D3DTEXOPCAPS_BLENDDIFFUSEALPHA D3DTEXOPCAPS_BLENDFACTORALPHA 
                            D3DTEXOPCAPS_BLENDTEXTUREALPHA D3DTEXOPCAPS_BLENDTEXTUREALPHAPM D3DTEXOPCAPS_BUMPENVMAP D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 
                            D3DTEXOPCAPS_LERP D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA 
                            D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA D3DTEXOPCAPS_MULTIPLYADD 
                            D3DTEXOPCAPS_PREMODULATE D3DTEXOPCAPS_SELECTARG2 D3DTEXOPCAPS_SUBTRACT */

    {
        GLint gl_max;

        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
        pCaps->MaxTextureBlendStages = min(8, gl_max);
        pCaps->MaxSimultaneousTextures = min(8, gl_max);
        TRACE("GLCaps: GL_MAX_TEXTURE_UNITS_ARB=%ld\n", pCaps->MaxTextureBlendStages);

        glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
        pCaps->MaxUserClipPlanes = min(MAX_CLIPPLANES, gl_max);
        TRACE("GLCaps: GL_MAX_CLIP_PLANES=%ld\n", pCaps->MaxUserClipPlanes);

        glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
        pCaps->MaxActiveLights = min(MAX_ACTIVE_LIGHTS, gl_max);
        TRACE("GLCaps: GL_MAX_LIGHTS=%ld\n", pCaps->MaxActiveLights);
    }

    pCaps->VertexProcessingCaps = D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_MATERIALSOURCE7 | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_TEXGEN;

    pCaps->MaxVertexBlendMatrices = 1;
    pCaps->MaxVertexBlendMatrixIndex = 1;

    pCaps->MaxPointSize = 128.0;

    pCaps->MaxPrimitiveCount = 0xFFFFFFFF;
    pCaps->MaxVertexIndex = 0xFFFFFFFF;
    pCaps->MaxStreams = 2; /* HACK: Some games want at least 2 */ 
    pCaps->MaxStreamStride = 1024;

    pCaps->VertexShaderVersion = D3DVS_VERSION(1,1);
    pCaps->MaxVertexShaderConst = D3D8_VSHADER_MAX_CONSTANTS;

    pCaps->PixelShaderVersion = D3DPS_VERSION(1,1);
    pCaps->MaxPixelShaderValue = 1.0;

    return D3D_OK;
}

HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor          (LPDIRECT3D8 iface,
                                                            UINT Adapter) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d)\n", This, Adapter);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CreateDevice               (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                            DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                            IDirect3DDevice8** ppReturnedDeviceInterface) {
    IDirect3DDevice8Impl *object;
    HWND whichHWND;
    int num;
    XVisualInfo template;
    const char *GL_Extensions = NULL;
    const char *GLX_Extensions = NULL;
    GLint gl_max;

    ICOM_THIS(IDirect3D8Impl,iface);
    TRACE("(%p)->(Adptr:%d, DevType: %x, FocusHwnd: %p, BehFlags: %lx, PresParms: %p, RetDevInt: %p)\n", This, Adapter, DeviceType,
          hFocusWindow, BehaviourFlags, pPresentationParameters, ppReturnedDeviceInterface);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice8Impl));
    if (NULL == object) {
      return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->lpVtbl = &Direct3DDevice8_Vtbl;
    object->ref = 1;
    object->direct3d8 = This;
    /** The device AddRef the direct3d8 Interface else crash in propers clients codes */
    IDirect3D8_AddRef((LPDIRECT3D8) object->direct3d8);

    /** use StateBlock Factory here, for creating the startup stateBlock */
    object->StateBlock = NULL;
    IDirect3DDeviceImpl_CreateStateBlock(object, D3DSBT_ALL, NULL);
    object->UpdateStateBlock = object->StateBlock;

    /* Save the creation parameters */
    object->CreateParms.AdapterOrdinal = Adapter;
    object->CreateParms.DeviceType = DeviceType;
    object->CreateParms.hFocusWindow = hFocusWindow;
    object->CreateParms.BehaviorFlags = BehaviourFlags;

    *ppReturnedDeviceInterface = (LPDIRECT3DDEVICE8)object;

    /* Initialize settings */
    memcpy(&object->PresentParms, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));

    object->PresentParms.BackBufferCount = 1; /* Opengl only supports one? */
    pPresentationParameters->BackBufferCount = 1;

    object->adapterNo = Adapter;
    object->devType = DeviceType;

    /* Initialize openGl */
    {
        HDC hDc;
        int          dblBuf[]={GLX_STENCIL_SIZE,8,GLX_RGBA,GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None};
        /* FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat */
	/*int          dblBuf[]={GLX_RGBA,GLX_RED_SIZE,4,GLX_GREEN_SIZE,4,GLX_BLUE_SIZE,4,GLX_DOUBLEBUFFER,None}; */

        /* Which hwnd are we using? */
/*      if (pPresentationParameters->Windowed) { */
           whichHWND = pPresentationParameters->hDeviceWindow;
           if (!whichHWND) {
               whichHWND = hFocusWindow;
           }
           object->win     = (Window)GetPropA( whichHWND, "__wine_x11_client_window" );
/*
 *      } else {
 *           whichHWND       = (HWND) GetDesktopWindow();
 *           object->win     = (Window)GetPropA(whichHWND, "__wine_x11_whole_window" );
 *	   root_window
 *        }
 */

        hDc = GetDC(whichHWND);
        object->display = get_display(hDc);

        ENTER_GL();
	object->visInfo = glXChooseVisual(object->display, DefaultScreen(object->display), dblBuf);
	if (NULL == object->visInfo) {
	  FIXME("cannot choose needed glxVisual with Stencil Buffer\n"); 

	  /**
	   * second try using wine initialized visual ...
	   * must be fixed reworking wine-glx init
	   */
	  template.visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
	  object->visInfo = XGetVisualInfo(object->display, VisualIDMask, &template, &num);
	  if (NULL == object->visInfo) {
	    ERR("cannot really get XVisual\n"); 
	    LEAVE_GL();
	    return D3DERR_NOTAVAILABLE;
	  }
	}
        object->glCtx = glXCreateContext(object->display, object->visInfo, NULL, GL_TRUE);
	if (NULL == object->glCtx) {
	  ERR("cannot create glxContext\n"); 
	  LEAVE_GL();
	  return D3DERR_NOTAVAILABLE;
	}
	LEAVE_GL();

        ReleaseDC(whichHWND, hDc);
    }

    if (object->glCtx == NULL) {
        ERR("Error in context creation !\n");
        return D3DERR_INVALIDCALL;
    } else {
        TRACE("Context created (HWND=%p, glContext=%p, Window=%ld, VisInfo=%p)\n",
			whichHWND, object->glCtx, object->win, object->visInfo);
    }

    /* If not windowed, need to go fullscreen, and resize the HWND to the appropriate  */
    /*        dimensions                                                               */
    if (!pPresentationParameters->Windowed) {
        FIXME("Requested full screen support not implemented, expect windowed operation\n");
        SetWindowPos(whichHWND, HWND_TOP, 0, 0, pPresentationParameters->BackBufferWidth,
            pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW);
    }

    TRACE("Creating back buffer\n");
    /* MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
       then the corresponding dimension of the client area of the hDeviceWindow
       (or the focus window, if hDeviceWindow is NULL) is taken. */
    if (pPresentationParameters->Windowed && ((pPresentationParameters->BackBufferWidth  == 0) ||
                                              (pPresentationParameters->BackBufferHeight  == 0))) {
        RECT Rect;

        GetClientRect(whichHWND, &Rect);

        if (pPresentationParameters->BackBufferWidth  == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight  == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
    }

    IDirect3DDevice8Impl_CreateImageSurface((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
                                            (LPDIRECT3DSURFACE8*) &object->backBuffer);

    if (pPresentationParameters->EnableAutoDepthStencil)
        IDirect3DDevice8Impl_CreateImageSurface((LPDIRECT3DDEVICE8) object,
                                                pPresentationParameters->BackBufferWidth,
                                                pPresentationParameters->BackBufferHeight,
                                                pPresentationParameters->AutoDepthStencilFormat,
                                                (LPDIRECT3DSURFACE8*) &object->depthStencilBuffer);
    
    /* Now override the surface's Flip method (if in double buffering) ?COPIED from DDRAW!?
    ((x11_ds_private *) surface->private)->opengl_flip = TRUE;
    {
    int i;
    struct _surface_chain *chain = surface->s.chain;
    for (i=0;i<chain->nrofsurfaces;i++)
      if (chain->surfaces[i]->s.surface_desc.ddsCaps.dwCaps & DDSCAPS_FLIP)
          ((x11_ds_private *) chain->surfaces[i]->private)->opengl_flip = TRUE;
    }
    */

    ENTER_GL();

    /*TRACE("hereeee. %x %x %x\n", object->display, object->win, object->glCtx);*/
    if (glXMakeCurrent(object->display, object->win, object->glCtx) == False) {
      ERR("Error in setting current context (context %p drawable %ld)!\n", object->glCtx, object->win);
    }
    checkGLcall("glXMakeCurrent");

    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
    glColor3f(1.0, 1.0, 1.0);
    checkGLcall("glColor3f");

    glEnable(GL_LIGHTING);
    checkGLcall("glEnable");

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    /* Initialize openGL extension related variables */
    object->isMultiTexture = FALSE;
    object->isDot3         = FALSE;
    object->TextureUnits   = 1;

    /* Retrieve opengl defaults */
    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    object->clipPlanes   = min(MAX_CLIPPLANES, gl_max);
    TRACE("ClipPlanes support - num Planes=%d\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    object->maxLights = min(MAX_ACTIVE_LIGHTS, gl_max);
    TRACE("Lights support - max lights=%d\n", gl_max);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately */
    GL_Extensions = glGetString(GL_EXTENSIONS);
    TRACE("GL_Extensions reported:\n");  
    
    if (NULL == GL_Extensions) {
      ERR("   GL_Extensions returns NULL\n");      
    } else {
      while (*GL_Extensions != 0x00) {
        const char *Start = GL_Extensions;
        char ThisExtn[256];

        memset(ThisExtn, 0x00, sizeof(ThisExtn));
        while (*GL_Extensions != ' ' && *GL_Extensions != 0x00) {
	  GL_Extensions++;
        }
        memcpy(ThisExtn, Start, (GL_Extensions - Start));
        TRACE ("   %s\n", ThisExtn);

        if (strcmp(ThisExtn, "GL_ARB_multitexture") == 0) {
            glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
            object->isMultiTexture = TRUE;
            object->TextureUnits   = min(8, gl_max);
            TRACE("FOUND: Multitexture support - GL_MAX_TEXTURE_UNITS_ARB=%d\n", gl_max);
        } else if (strcmp(ThisExtn, "GL_EXT_texture_env_dot3") == 0) {
            object->isDot3 = TRUE;
            TRACE("FOUND: Dot3 support\n");
        }

        if (*GL_Extensions == ' ') GL_Extensions++;
      }
    }

    GLX_Extensions = glXQueryExtensionsString(object->display, DefaultScreen(object->display));
    TRACE("GLX_Extensions reported:\n");  
    
    if (NULL == GLX_Extensions) {
      ERR("   GLX_Extensions returns NULL\n");      
    } else {
      while (*GLX_Extensions != 0x00) {
        const char *Start = GLX_Extensions;
        char ThisExtn[256];
	
        memset(ThisExtn, 0x00, sizeof(ThisExtn));
        while (*GLX_Extensions != ' ' && *GLX_Extensions != 0x00) {
	  GLX_Extensions++;
        }
        memcpy(ThisExtn, Start, (GLX_Extensions - Start));
        TRACE ("   %s\n", ThisExtn);
        if (*GLX_Extensions == ' ') GLX_Extensions++;
      }
    }

    /* Setup all the devices defaults */
    IDirect3DDeviceImpl_InitStartupStateBlock(object);

    LEAVE_GL();

    { /* Set a default viewport */
       D3DVIEWPORT8 vp;
       vp.X      = 0;
       vp.Y      = 0;
       vp.Width  = pPresentationParameters->BackBufferWidth;
       vp.Height = pPresentationParameters->BackBufferHeight;
       vp.MinZ   = 0.0f;
       vp.MaxZ   = 1.0f;
       IDirect3DDevice8Impl_SetViewport((LPDIRECT3DDEVICE8) object, &vp);
    }

    TRACE("(%p,%d) All defaults now set up, leaving CreateDevice\n", This, Adapter);
    return D3D_OK;
}

ICOM_VTABLE(IDirect3D8) Direct3D8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3D8Impl_QueryInterface,
    IDirect3D8Impl_AddRef,
    IDirect3D8Impl_Release,
    IDirect3D8Impl_RegisterSoftwareDevice,
    IDirect3D8Impl_GetAdapterCount,
    IDirect3D8Impl_GetAdapterIdentifier,
    IDirect3D8Impl_GetAdapterModeCount,
    IDirect3D8Impl_EnumAdapterModes,
    IDirect3D8Impl_GetAdapterDisplayMode,
    IDirect3D8Impl_CheckDeviceType,
    IDirect3D8Impl_CheckDeviceFormat,
    IDirect3D8Impl_CheckDeviceMultiSampleType,
    IDirect3D8Impl_CheckDepthStencilMatch,
    IDirect3D8Impl_GetDeviceCaps,
    IDirect3D8Impl_GetAdapterMonitor,
    IDirect3D8Impl_CreateDevice
};
