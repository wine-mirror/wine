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
#include "wine/unicode.h"

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

#define NUM_MODES 20
static const int modes[NUM_MODES][4] = {
    {640, 480, 85, 16},
    {640, 480, 85, 32},

    {800, 600, 85, 16},
    {800, 600, 85, 32},

    {1024, 768, 85, 16},
    {1024, 768, 85, 32},

    {1152, 864, 85, 16},
    {1152, 864, 85, 32},

    {1280, 768, 85, 16},
    {1280, 768, 85, 32},

    {1280, 960, 85, 16},
    {1280, 960, 85, 32},

    {1280, 1024, 85, 16},
    {1280, 1024, 85, 32},

    {1600, 900, 85, 16},
    {1600, 900, 85, 32},

    {1600, 1024, 85, 16},
    {1600, 1024, 85, 32},

    {1600, 1200, 85, 16},
    {1600, 1200, 85, 32}
};

#define NUM_FORMATS 7
static const D3DFORMAT device_formats[NUM_FORMATS] = {
  D3DFMT_P8,
  D3DFMT_R3G3B2,
  D3DFMT_R5G6B5, 
  D3DFMT_X1R5G5B5,
  D3DFMT_X4R4G4B4,
  D3DFMT_R8G8B8,
  D3DFMT_X8R8G8B8
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
        if (Flags & D3DENUM_NO_WHQL_LEVEL) {
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

        for (i = 0; i < NUM_MODES; i++) {
            if (modes[i][0] > maxWidth || modes[i][1] > maxHeight) {
  	        TRACE("(%p}->(Adapter: %d) => %d\n", This, Adapter, i + 1);
                return i + 1;
            }
        }
	TRACE("(%p}->(Adapter: %d) => %d\n", This, Adapter, NUM_MODES);
        return NUM_MODES + 1;
    } else {
        FIXME("Adapter not primary display\n");
    }

    return 0;
}

HRESULT  WINAPI  IDirect3D8Impl_EnumAdapterModes           (LPDIRECT3D8 iface,
                                                            UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    ICOM_THIS(IDirect3D8Impl,iface);

    TRACE("(%p}->(Adapter:%d, mode:%d, pMode:%p)\n", This, Adapter, Mode, pMode);

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
	    bpp                 = 32;
        } else if (Mode < (NUM_MODES + 1)) {
            pMode->Width        = modes[Mode - 1][0];
            pMode->Height       = modes[Mode - 1][1];
            pMode->RefreshRate  = modes[Mode - 1][2];
	    bpp                 = modes[Mode - 1][3];
        } else {
            TRACE("Requested mode out of range %d\n", Mode);
            return D3DERR_INVALIDCALL;
        }

        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = min(GetDeviceCaps(hdc, BITSPIXEL), bpp);
        DeleteDC(hdc);

        switch (bpp) {
        case  8: pMode->Format = D3DFMT_R3G3B2;   break;
        case 16: pMode->Format = D3DFMT_R5G6B5;   break;
        case 24: /* pMode->Format = D3DFMT_R5G6B5;   break;*/ /* Make 24bit appear as 32 bit */
        case 32: pMode->Format = D3DFMT_A8R8G8B8; break;
        default: pMode->Format = D3DFMT_UNKNOWN;
        }
        TRACE("W %d H %d rr %d fmt (%x,%s) bpp %u\n", pMode->Width, pMode->Height, pMode->RefreshRate, pMode->Format, debug_d3dformat(pMode->Format), bpp);

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
        case 24: /*pMode->Format       = D3DFMT_R5G6B5;   break;*/ /* Make 24bit appear as 32 bit */
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
    FIXME("(%p)->(Adptr:%d, CheckType:(%x,%s), DispFmt:(%x,%s), BackBuf:(%x,%s), Win?%d): stub\n", 
	  This, 
	  Adapter, 
	  CheckType, debug_d3ddevicetype(CheckType),
          DisplayFormat, debug_d3dformat(DisplayFormat),
	  BackBufferFormat, debug_d3dformat(BackBufferFormat),
	  Windowed);
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat          (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType:(%u,%s), AdptFmt:(%u,%s), Use:(%lu,%s), ResTyp:(%x,%s), CheckFmt:(%u,%s))\n", 
          This, 
	  Adapter, 
	  DeviceType, debug_d3ddevicetype(DeviceType), 
	  AdapterFormat, debug_d3dformat(AdapterFormat), 
	  Usage, debug_d3dusage(Usage),
	  RType, debug_d3dressourcetype(RType), 
	  CheckFormat, debug_d3dformat(CheckFormat));

    switch (CheckFormat) {
    case D3DFMT_UYVY:
    case D3DFMT_YUY2:
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
    case D3DFMT_X8L8V8U8:
    case D3DFMT_L6V5U5:
    case D3DFMT_V8U8:
      /* Since we do not support these formats right now, don't pretend to. */
      return D3DERR_NOTAVAILABLE;
    default:
      break;
    }

    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceMultiSampleType(LPDIRECT3D8 iface,
							   UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
							   BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType:(%x,%s), SurfFmt:(%x,%s), Win?%d, MultiSamp:%x)\n", 
	  This, 
	  Adapter, 
	  DeviceType, debug_d3ddevicetype(DeviceType),
          SurfaceFormat, debug_d3dformat(SurfaceFormat),
	  Windowed, 
	  MultiSampleType);
  
    if (D3DMULTISAMPLE_NONE == MultiSampleType)
      return D3D_OK;
    return D3DERR_NOTAVAILABLE;
}

HRESULT  WINAPI  IDirect3D8Impl_CheckDepthStencilMatch(LPDIRECT3D8 iface, 
						       UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d, DevType:(%x,%s), AdptFmt:(%x,%s), RendrTgtFmt:(%x,%s), DepthStencilFmt:(%x,%s))\n", 
	  This, 
	  Adapter, 
	  DeviceType, debug_d3ddevicetype(DeviceType),
          AdapterFormat, debug_d3dformat(AdapterFormat),
	  RenderTargetFormat, debug_d3dformat(RenderTargetFormat), 
	  DepthStencilFormat, debug_d3dformat(DepthStencilFormat));

#if 0
    switch (DepthStencilFormat) {
    case D3DFMT_D24X4S4:
    case D3DFMT_D24X8: 
    case D3DFMT_D24S8: 
    case D3DFMT_D32:
      /**
       * as i don't know how to really check hard caps of graphics cards
       * i prefer to not permit 32bit zbuffers enumeration (as few cards can do it)
       */
      return D3DERR_NOTAVAILABLE;
    default:
      break;
    }
#endif
    return D3D_OK;
}

HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    ICOM_THIS(IDirect3D8Impl,iface);
    TRACE("(%p)->(Adptr:%d, DevType: %x, pCaps: %p)\n", This, Adapter, DeviceType, pCaps);

    pCaps->DeviceType = (DeviceType == D3DDEVTYPE_HAL) ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;  /* Not quite true, but use h/w supported by opengl I suppose */
    pCaps->AdapterOrdinal = Adapter;

    pCaps->Caps = 0;
    pCaps->Caps2 = D3DCAPS2_CANRENDERWINDOWED;
    pCaps->Caps3 = D3DDEVCAPS_HWTRANSFORMANDLIGHT;
    pCaps->PresentationIntervals = D3DPRESENT_INTERVAL_IMMEDIATE;

    pCaps->CursorCaps = 0;

    pCaps->DevCaps = D3DDEVCAPS_DRAWPRIMTLVERTEX    | 
                     D3DDEVCAPS_HWTRANSFORMANDLIGHT |
                     D3DDEVCAPS_PUREDEVICE;

    pCaps->PrimitiveMiscCaps = D3DPMISCCAPS_CULLCCW               | 
                               D3DPMISCCAPS_CULLCW                | 
                               D3DPMISCCAPS_COLORWRITEENABLE      |
                               D3DPMISCCAPS_CLIPTLVERTS           |
                               D3DPMISCCAPS_CLIPPLANESCALEDPOINTS | 
                               D3DPMISCCAPS_MASKZ; 
                               /*NOT: D3DPMISCCAPS_TSSARGTEMP*/

    pCaps->RasterCaps = D3DPRASTERCAPS_DITHER   | 
                        D3DPRASTERCAPS_PAT      | 
                        D3DPRASTERCAPS_FOGRANGE;
                        /* FIXME Add:
			   D3DPRASTERCAPS_FOGVERTEX
			   D3DPRASTERCAPS_FOGTABLE
			   D3DPRASTERCAPS_MIPMAPLODBIAS
			   D3DPRASTERCAPS_ZBIAS
			   D3DPRASTERCAPS_ANISOTROPY
			   D3DPRASTERCAPS_WFOG
			   D3DPRASTERCAPS_ZFOG 
			   D3DPRASTERCAPS_COLORPERSPECTIVE
			   D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE
			   D3DPRASTERCAPS_ANTIALIASEDGES
			   D3DPRASTERCAPS_ZBUFFERLESSHSR
			   D3DPRASTERCAPS_WBUFFER */

    pCaps->ZCmpCaps = D3DPCMPCAPS_ALWAYS       | 
                      D3DPCMPCAPS_EQUAL        | 
                      D3DPCMPCAPS_GREATER      | 
                      D3DPCMPCAPS_GREATEREQUAL |
                      D3DPCMPCAPS_LESS         | 
                      D3DPCMPCAPS_LESSEQUAL    | 
                      D3DPCMPCAPS_NEVER        |
                      D3DPCMPCAPS_NOTEQUAL;

    pCaps->SrcBlendCaps  = 0xFFFFFFFF;   /*FIXME: Tidy up later */
    pCaps->DestBlendCaps = 0xFFFFFFFF;   /*FIXME: Tidy up later */
    pCaps->AlphaCmpCaps  = 0xFFFFFFFF;   /*FIXME: Tidy up later */

    pCaps->ShadeCaps = D3DPSHADECAPS_SPECULARGOURAUDRGB | 
                       D3DPSHADECAPS_COLORGOURAUDRGB;

    pCaps->TextureCaps =  D3DPTEXTURECAPS_ALPHA        | 
                          D3DPTEXTURECAPS_ALPHAPALETTE | 
                          D3DPTEXTURECAPS_POW2         | 
                          D3DPTEXTURECAPS_VOLUMEMAP    | 
                          D3DPTEXTURECAPS_MIPMAP;
#if defined(GL_VERSION_1_3) || defined(GL_ARB_texture_cube_map)
    pCaps->TextureCaps |= D3DPTEXTURECAPS_CUBEMAP      | 
                          D3DPTEXTURECAPS_MIPCUBEMAP   | 
                          D3DPTEXTURECAPS_CUBEMAP_POW2;
#endif

    pCaps->TextureFilterCaps = D3DPTFILTERCAPS_MAGFLINEAR | 
                               D3DPTFILTERCAPS_MAGFPOINT  | 
                               D3DPTFILTERCAPS_MINFLINEAR | 
                               D3DPTFILTERCAPS_MINFPOINT  |
                               D3DPTFILTERCAPS_MIPFLINEAR | 
                               D3DPTFILTERCAPS_MIPFPOINT;

    pCaps->CubeTextureFilterCaps = 0;
    pCaps->VolumeTextureFilterCaps = 0;

    pCaps->TextureAddressCaps =  D3DPTADDRESSCAPS_BORDER | 
                                 D3DPTADDRESSCAPS_CLAMP  | 
                                 D3DPTADDRESSCAPS_WRAP;
#if defined(GL_VERSION_1_3)
    pCaps->TextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
#endif
                                 /* FIXME: Add 
				    D3DPTADDRESSCAPS_BORDER
				    D3DPTADDRESSCAPS_MIRRORONCE */

    pCaps->VolumeTextureAddressCaps = 0;

    pCaps->LineCaps = D3DLINECAPS_TEXTURE | 
                      D3DLINECAPS_ZTEST;
                      /* FIXME: Add 
			 D3DLINECAPS_BLEND
			 D3DLINECAPS_ALPHACMP
			 D3DLINECAPS_FOG */

    {
      GLint gl_tex_size;    
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_tex_size);
      pCaps->MaxTextureWidth = gl_tex_size;
      pCaps->MaxTextureHeight = gl_tex_size;
    }

    pCaps->MaxVolumeExtent = 0;

    pCaps->MaxTextureRepeat = 32768;
    pCaps->MaxTextureAspectRatio = 32768;
    pCaps->MaxVertexW = 1.0;

    pCaps->GuardBandLeft = 0;
    pCaps->GuardBandTop = 0;
    pCaps->GuardBandRight = 0;
    pCaps->GuardBandBottom = 0;

    pCaps->ExtentsAdjust = 0;

    pCaps->StencilCaps =  D3DSTENCILCAPS_DECRSAT | 
                          D3DSTENCILCAPS_INCRSAT | 
                          D3DSTENCILCAPS_INVERT  | 
                          D3DSTENCILCAPS_KEEP    | 
                          D3DSTENCILCAPS_REPLACE | 
                          D3DSTENCILCAPS_ZERO;
#if defined(GL_VERSION_1_4) || defined(GL_EXT_stencil_wrap)
    pCaps->StencilCaps |= D3DSTENCILCAPS_DECR    | 
                          D3DSTENCILCAPS_INCR;
#endif

    pCaps->FVFCaps = D3DFVFCAPS_PSIZE | 0x80000;

    pCaps->TextureOpCaps =  D3DTEXOPCAPS_ADD         | 
                            D3DTEXOPCAPS_ADDSIGNED   | 
                            D3DTEXOPCAPS_ADDSIGNED2X |
                            D3DTEXOPCAPS_MODULATE    | 
                            D3DTEXOPCAPS_MODULATE2X  | 
                            D3DTEXOPCAPS_MODULATE4X  |
                            D3DTEXOPCAPS_SELECTARG1  | 
                            D3DTEXOPCAPS_SELECTARG2  | 
                            D3DTEXOPCAPS_DISABLE;
#if defined(GL_VERSION_1_3)
    pCaps->TextureOpCaps |= D3DTEXOPCAPS_DOTPRODUCT3 | 
                            D3DTEXOPCAPS_SUBTRACT;
#endif
                            /* FIXME: Add 
			      D3DTEXOPCAPS_ADDSMOOTH
			      D3DTEXOPCAPS_BLENDCURRENTALPHA 
			      D3DTEXOPCAPS_BLENDDIFFUSEALPHA 
			      D3DTEXOPCAPS_BLENDFACTORALPHA 
			      D3DTEXOPCAPS_BLENDTEXTUREALPHA 
			      D3DTEXOPCAPS_BLENDTEXTUREALPHAPM 
			      D3DTEXOPCAPS_BUMPENVMAP
			      D3DTEXOPCAPS_BUMPENVMAPLUMINANCE 
			      D3DTEXOPCAPS_LERP 
			      D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR 
			      D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA 
			      D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR 
			      D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA
			      D3DTEXOPCAPS_MULTIPLYADD 
			      D3DTEXOPCAPS_PREMODULATE */

    {
        GLint gl_max;
	GLfloat gl_float;

#if defined(GL_VERSION_1_3)
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gl_max);
#else
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
#endif
        TRACE("GLCaps: GL_MAX_TEXTURE_UNITS_ARB=%d\n", gl_max);
        pCaps->MaxTextureBlendStages = min(8, gl_max);
        pCaps->MaxSimultaneousTextures = min(8, gl_max);

        glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
        pCaps->MaxUserClipPlanes = min(MAX_CLIPPLANES, gl_max);
        TRACE("GLCaps: GL_MAX_CLIP_PLANES=%ld\n", pCaps->MaxUserClipPlanes);

        glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
        pCaps->MaxActiveLights = min(MAX_ACTIVE_LIGHTS, gl_max);
        TRACE("GLCaps: GL_MAX_LIGHTS=%ld\n", pCaps->MaxActiveLights);

#if defined(GL_ARB_vertex_blend)
	glGetIntegerv(GL_MAX_VERTEX_UNITS_ARB, &gl_max);
	pCaps->MaxVertexBlendMatrices = gl_max;
	pCaps->MaxVertexBlendMatrixIndex = 1;
#else
	pCaps->MaxVertexBlendMatrices = 0;
	pCaps->MaxVertexBlendMatrixIndex = 1;
#endif

#if defined(GL_EXT_texture_filter_anisotropic)
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_max);
        pCaps->MaxAnisotropy = gl_max;
#else
        pCaps->MaxAnisotropy = 0;
#endif

	glGetFloatv(GL_POINT_SIZE_RANGE, &gl_float);
	pCaps->MaxPointSize = gl_float;
    }

    pCaps->VertexProcessingCaps = D3DVTXPCAPS_DIRECTIONALLIGHTS | 
                                  D3DVTXPCAPS_MATERIALSOURCE7   | 
                                  D3DVTXPCAPS_POSITIONALLIGHTS  | 
                                  D3DVTXPCAPS_TEXGEN;
                                  /* FIXME: Add 
				     D3DVTXPCAPS_LOCALVIEWER 
				     D3DVTXPCAPS_TWEENING */

    pCaps->MaxPrimitiveCount = 0xFFFFFFFF;
    pCaps->MaxVertexIndex = 0xFFFFFFFF;
    pCaps->MaxStreams = MAX_STREAMS;
    pCaps->MaxStreamStride = 1024;

#if 1
    pCaps->VertexShaderVersion = D3DVS_VERSION(1,1);
    pCaps->MaxVertexShaderConst = D3D8_VSHADER_MAX_CONSTANTS;
#else
    pCaps->VertexShaderVersion = 0;
    pCaps->MaxVertexShaderConst = D3D8_VSHADER_MAX_CONSTANTS;
#endif

#if 0
    pCaps->PixelShaderVersion = D3DPS_VERSION(1,1);
    pCaps->MaxPixelShaderValue = 1.0;
#else
    pCaps->PixelShaderVersion = 0;
    pCaps->MaxPixelShaderValue = 0.0;
#endif

    return D3D_OK;
}

HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter) {
    ICOM_THIS(IDirect3D8Impl,iface);
    FIXME("(%p)->(Adptr:%d)\n", This, Adapter);
    return D3D_OK;
}


static void IDirect3D8Impl_FillGLCaps(LPDIRECT3D8 iface, Display* display) {
    const char *GL_Extensions = NULL;
    const char *GLX_Extensions = NULL;
    GLint gl_max;
    ICOM_THIS(IDirect3D8Impl,iface);

    /* 
     * Initialize openGL extension related variables
     *  with Default values 
     */
    memset(&This->gl_info.supported, 0, sizeof(This->gl_info.supported));
    This->gl_info.max_textures   = 1;
    This->gl_info.ps_arb_version = PS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_arb_version = VS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_nv_version  = VS_VERSION_NOT_SUPPORTED;
    This->gl_info.vs_ati_version = VS_VERSION_NOT_SUPPORTED;

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = NULL;
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

    /* Retrieve opengl defaults */
    glGetIntegerv(GL_MAX_CLIP_PLANES, &gl_max);
    This->gl_info.max_clipplanes = min(MAX_CLIPPLANES, gl_max);
    TRACE("ClipPlanes support - num Planes=%d\n", gl_max);

    glGetIntegerv(GL_MAX_LIGHTS, &gl_max);
    This->gl_info.max_lights = min(MAX_ACTIVE_LIGHTS, gl_max);
    TRACE("Lights support - max lights=%d\n", gl_max);

    /* Parse the gl supported features, in theory enabling parts of our code appropriately */
    GL_Extensions = glGetString(GL_EXTENSIONS);
    FIXME("GL_Extensions reported:\n");  
    
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
        FIXME("- %s\n", ThisExtn);

	/**
	 * ARB 
	 */
	if (strcmp(ThisExtn, "GL_ARB_fragment_program") == 0) {
	  This->gl_info.ps_arb_version = PS_VERSION_11;
	  FIXME(" FOUND: ARB Pixel Shader support - version=%02x\n", This->gl_info.ps_arb_version);
	  This->gl_info.supported[ARB_FRAGMENT_PROGRAM] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_multisample") == 0) {
	  FIXME(" FOUND: ARB Multisample support\n");
	  This->gl_info.supported[ARB_MULTISAMPLE] = TRUE;
	} else if (strcmp(ThisExtn, "GL_ARB_multitexture") == 0) {
	  glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_max);
	  FIXME(" FOUND: ARB Multitexture support - GL_MAX_TEXTURE_UNITS_ARB=%u\n", gl_max);
	  This->gl_info.supported[ARB_MULTITEXTURE] = TRUE;
	  This->gl_info.max_textures = min(8, gl_max);
        } else if (strcmp(ThisExtn, "GL_ARB_texture_cube_map") == 0) {
	  FIXME(" FOUND: ARB Texture Cube Map support\n");
	  This->gl_info.supported[ARB_TEXTURE_CUBE_MAP] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_compression") == 0) {
	  FIXME(" FOUND: ARB Texture Compression support\n");
	  This->gl_info.supported[ARB_TEXTURE_COMPRESSION] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_env_combine") == 0) {
	  FIXME(" FOUND: EXT Texture Env combine support\n");
	  This->gl_info.supported[ARB_TEXTURE_ENV_COMBINE] = TRUE;
        } else if (strcmp(ThisExtn, "GL_ARB_texture_env_dot3") == 0) {
	  if (FALSE == This->gl_info.supported[ARB_TEXTURE_ENV_DOT3]) {
	    FIXME(" FOUND: EXT Dot3 support\n");
	    This->gl_info.supported[ARB_TEXTURE_ENV_DOT3] = TRUE;
	  }
	} else if (strstr(ThisExtn, "GL_ARB_vertex_program")) {
	  This->gl_info.vs_arb_version = VS_VERSION_11;
	  FIXME(" FOUND: ARB Vertex Shader support - version=%02x\n", This->gl_info.vs_arb_version);
	  This->gl_info.supported[ARB_VERTEX_PROGRAM] = TRUE;

	/**
	 * EXT
	 */
        } else if (strcmp(ThisExtn, "GL_EXT_fog_coord") == 0) {
	  FIXME(" FOUND: EXT Fog coord support\n");
	  This->gl_info.supported[EXT_FOG_COORD] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_paletted_texture") == 0) { /* handle paletted texture extensions */
	  FIXME(" FOUND: EXT Paletted texture support\n");
	  This->gl_info.supported[EXT_PALETTED_TEXTURE] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_point_parameters") == 0) {
	  FIXME(" FOUND: EXT Point parameters support\n");
	  This->gl_info.supported[EXT_POINT_PARAMETERS] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_secondary_color") == 0) {
	  FIXME(" FOUND: EXT Secondary coord support\n");
	  This->gl_info.supported[EXT_SECONDARY_COLOR] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_compression_s3tc") == 0) {
	  FIXME(" FOUND: EXT Texture S3TC compression support\n");
	  This->gl_info.supported[EXT_TEXTURE_COMPRESSION_S3TC] = TRUE;
        } else if (strcmp(ThisExtn, "GL_EXT_texture_env_dot3") == 0) {
	  if (FALSE == This->gl_info.supported[ARB_TEXTURE_ENV_DOT3]) {
	    FIXME(" FOUND: EXT Dot3 support\n");
	    This->gl_info.supported[ARB_TEXTURE_ENV_DOT3] = TRUE;
	  }
	} else if (strcmp(ThisExtn, "GL_EXT_texture_filter_anisotropic") == 0) {
	  FIXME(" FOUND: EXT Texture Anisotropic filter support\n");
	  This->gl_info.supported[EXT_TEXTURE_FILTER_ANISOTROPIC] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_lod") == 0) {
	  FIXME(" FOUND: EXT Texture LOD support\n");
	  This->gl_info.supported[EXT_TEXTURE_LOD] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_texture_lod_bias") == 0) {
	  FIXME(" FOUND: EXT Texture LOD bias support\n");
	  This->gl_info.supported[EXT_TEXTURE_LOD_BIAS] = TRUE;
	} else if (strcmp(ThisExtn, "GL_EXT_vertex_weighting") == 0) {
	  FIXME(" FOUND: EXT Vertex weighting support\n");
	  This->gl_info.supported[EXT_VERTEX_WEIGHTING] = TRUE;

	/**
	 * NVIDIA 
	 */
	} else if (strstr(ThisExtn, "GL_NV_fragment_program")) {
	  This->gl_info.ps_nv_version = PS_VERSION_11;
	  FIXME(" FOUND: NVIDIA (NV) Pixel Shader support - version=%02x\n", This->gl_info.ps_nv_version);
	  This->gl_info.supported[NV_FRAGMENT_PROGRAM] = TRUE;
	} else if (strstr(ThisExtn, "GL_NV_vertex_program")) {
	  This->gl_info.vs_nv_version = max(This->gl_info.vs_nv_version, (0 == strcmp(ThisExtn, "GL_NV_vertex_program1_1")) ? VS_VERSION_11 : VS_VERSION_10);
	  This->gl_info.vs_nv_version = max(This->gl_info.vs_nv_version, (0 == strcmp(ThisExtn, "GL_NV_vertex_program2"))   ? VS_VERSION_20 : VS_VERSION_10);
	  FIXME(" FOUND: NVIDIA (NV) Vertex Shader support - version=%02x\n", This->gl_info.vs_nv_version);
	  This->gl_info.supported[NV_VERTEX_PROGRAM] = TRUE;

	/**
	 * ATI
	 */
	/** TODO */
	} else if (strcmp(ThisExtn, "GL_EXT_vertex_shader") == 0) {
	  This->gl_info.vs_ati_version = VS_VERSION_11;
	  FIXME(" FOUND: ATI (EXT) Vertex Shader support - version=%02x\n", This->gl_info.vs_ati_version);
	  This->gl_info.supported[EXT_VERTEX_SHADER] = TRUE;
	}


        if (*GL_Extensions == ' ') GL_Extensions++;
      }
    }

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = (type) glXGetProcAddressARB(#pfn);
    GL_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

    GLX_Extensions = glXQueryExtensionsString(display, DefaultScreen(display));
    FIXME("GLX_Extensions reported:\n");  
    
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
        FIXME("- %s\n", ThisExtn);
        if (*GLX_Extensions == ' ') GLX_Extensions++;
      }
    }

#define USE_GL_FUNC(type, pfn) This->gl_info.pfn = (type) glXGetProcAddressARB(#pfn);
    GLX_EXT_FUNCS_GEN;
#undef USE_GL_FUNC

}

HRESULT  WINAPI  IDirect3D8Impl_CreateDevice               (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                                            DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                            IDirect3DDevice8** ppReturnedDeviceInterface) {
    IDirect3DDevice8Impl *object;
    HWND whichHWND;
    int num;
    XVisualInfo template;

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

    *ppReturnedDeviceInterface = (LPDIRECT3DDEVICE8) object;

    /* Initialize settings */
    object->PresentParms.BackBufferCount = 1; /* Opengl only supports one? */
    object->adapterNo = Adapter;
    object->devType = DeviceType;

    /* Initialize openGl */
    {
        HDC hDc;
	/*int dblBuf[] = {GLX_STENCIL_SIZE,8,GLX_RGBA,GLX_DEPTH_SIZE,16,GLX_DOUBLEBUFFER,None};*/
        int dblBuf[] = {GLX_RGBA, 
			GLX_STENCIL_SIZE, 8, /*  2 */
			GLX_DEPTH_SIZE,  16, /*  4 */
			GLX_DOUBLEBUFFER, None};
        /* FIXME: Handle stencil appropriately via EnableAutoDepthStencil / AutoDepthStencilFormat */

        /* Which hwnd are we using? */
/*      if (pPresentationParameters->Windowed) { */
           whichHWND = pPresentationParameters->hDeviceWindow;
           if (!whichHWND) {
               whichHWND = hFocusWindow;
           }
	   object->win_handle = whichHWND;
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


	TRACE("(%p)->(DepthStencil:(%u,%s), BackBufferFormat:(%u,%s))\n", This, 
	      pPresentationParameters->AutoDepthStencilFormat, debug_d3dformat(pPresentationParameters->AutoDepthStencilFormat),
	      pPresentationParameters->BackBufferFormat, debug_d3dformat(pPresentationParameters->BackBufferFormat));

	
#if 0
	if (TRUE == pPresentationParameters->EnableAutoDepthStencil) {
	  switch (pPresentationParameters->AutoDepthStencilFormat) {
	  case D3DFMT_D16_LOCKABLE: dblBuf[2] =  8; dblBuf[4] = 16; break;
	  case D3DFMT_D16:          dblBuf[2] =  8; dblBuf[4] = 16; break;
	  case D3DFMT_D15S1:        dblBuf[2] =  1; dblBuf[4] = 16; break;
	  case D3DFMT_D24X4S4:      dblBuf[2] =  4; dblBuf[4] = 24; break;
	  case D3DFMT_D24S8:        dblBuf[2] =  8; dblBuf[4] = 24; break;
	  case D3DFMT_D24X8:        dblBuf[2] =  8; dblBuf[4] = 24; break;
	  case D3DFMT_D32:          dblBuf[2] =  8; dblBuf[4] = 32; break;
	  default:                  dblBuf[2] =  8; dblBuf[4] = 16; break;
	  }
	}
	
	switch (pPresentationParameters->BackBufferFormat) {
	case D3DFMT_R3G3B2:   dblBuf[6] = 3; dblBuf[8] = 3; dblBuf[10] = 2; dblBuf[12] = 0; break;
	case D3DFMT_R5G6B5:   dblBuf[6] = 5; dblBuf[8] = 6; dblBuf[10] = 5; dblBuf[12] = 0; break;
	case D3DFMT_X1R5G5B5: dblBuf[6] = 5; dblBuf[8] = 5; dblBuf[10] = 5; dblBuf[12] = 0; break;
	case D3DFMT_A1R5G5B5: dblBuf[6] = 5; dblBuf[8] = 5; dblBuf[10] = 5; dblBuf[12] = 1; break;
	case D3DFMT_X4R4G4B4: dblBuf[6] = 4; dblBuf[8] = 4; dblBuf[10] = 4; dblBuf[12] = 0; break;
	case D3DFMT_R8G8B8:   dblBuf[6] = 8; dblBuf[8] = 8; dblBuf[10] = 8; dblBuf[12] = 0; break;
	case D3DFMT_X8R8G8B8: dblBuf[6] = 8; dblBuf[8] = 8; dblBuf[10] = 8; dblBuf[12] = 0; break;
	case D3DFMT_A8R8G8B8: dblBuf[6] = 8; dblBuf[8] = 8; dblBuf[10] = 8; dblBuf[12] = 8; break;
	default:              dblBuf[6] = 5; dblBuf[8] = 6; dblBuf[10] = 5; dblBuf[12] = 0; break;
	}
#endif

        ENTER_GL();
	object->visInfo = glXChooseVisual(object->display, DefaultScreen(object->display), dblBuf);
	if (NULL == object->visInfo) {
	  FIXME("cannot choose needed glxVisual with Stencil Buffer\n"); 

	  /**
	   * second try using wine initialized visual ...
	   * must be fixed reworking wine-glx init
	   */
	  template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
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
#if 0
	DEVMODEW devmode;
	HDC hdc;
        int bpp = 0;
	memset(&devmode, 0, sizeof(DEVMODEW));
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT; 
	MultiByteToWideChar(CP_ACP, 0, "Gamers CG", -1, devmode.dmDeviceName, CCHDEVICENAME);
        hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        bpp = GetDeviceCaps(hdc, BITSPIXEL);
        DeleteDC(hdc);
	devmode.dmBitsPerPel = (bpp >= 24) ? 32 : bpp;/*Stupid XVidMode cannot change bpp D3DFmtGetBpp(object, pPresentationParameters->BackBufferFormat);*/
	devmode.dmPelsWidth  = pPresentationParameters->BackBufferWidth;
	devmode.dmPelsHeight = pPresentationParameters->BackBufferHeight;
	ChangeDisplaySettingsExW(devmode.dmDeviceName, &devmode, object->win_handle, CDS_FULLSCREEN, NULL);
#else
        FIXME("Requested full screen support not implemented, expect windowed operation\n");
#endif
        SetWindowPos(object->win_handle, HWND_TOP, 0, 0, 
		     pPresentationParameters->BackBufferWidth,
                     pPresentationParameters->BackBufferHeight, SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    }

    TRACE("Creating back buffer\n");
    /* MSDN: If Windowed is TRUE and either of the BackBufferWidth/Height values is zero,
       then the corresponding dimension of the client area of the hDeviceWindow
       (or the focus window, if hDeviceWindow is NULL) is taken. */
    if (pPresentationParameters->Windowed && ((pPresentationParameters->BackBufferWidth  == 0) ||
                                              (pPresentationParameters->BackBufferHeight == 0))) {
        RECT Rect;

        GetClientRect(whichHWND, &Rect);

        if (pPresentationParameters->BackBufferWidth == 0) {
           pPresentationParameters->BackBufferWidth = Rect.right;
           TRACE("Updating width to %d\n", pPresentationParameters->BackBufferWidth);
        }
        if (pPresentationParameters->BackBufferHeight == 0) {
           pPresentationParameters->BackBufferHeight = Rect.bottom;
           TRACE("Updating height to %d\n", pPresentationParameters->BackBufferHeight);
        }
    }

    /* Save the presentation parms now filled in correctly */
    memcpy(&object->PresentParms, pPresentationParameters, sizeof(D3DPRESENT_PARAMETERS));


    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->frontBuffer);

    IDirect3DDevice8Impl_CreateRenderTarget((LPDIRECT3DDEVICE8) object,
                                            pPresentationParameters->BackBufferWidth,
                                            pPresentationParameters->BackBufferHeight,
                                            pPresentationParameters->BackBufferFormat,
					    pPresentationParameters->MultiSampleType,
					    TRUE,
                                            (LPDIRECT3DSURFACE8*) &object->backBuffer);

    if (pPresentationParameters->EnableAutoDepthStencil) {
       IDirect3DDevice8Impl_CreateDepthStencilSurface((LPDIRECT3DDEVICE8) object,
	  					      pPresentationParameters->BackBufferWidth,
						      pPresentationParameters->BackBufferHeight,
						      pPresentationParameters->AutoDepthStencilFormat,
						      D3DMULTISAMPLE_NONE,
						      (LPDIRECT3DSURFACE8*) &object->depthStencilBuffer);
    } else {
      object->depthStencilBuffer = NULL;
    }

    /* init the default renderTarget management */
    object->drawable = object->win;
    object->render_ctx = object->glCtx;
    object->renderTarget = object->frontBuffer;
    IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->renderTarget);
    object->stencilBufferTarget = object->depthStencilBuffer;
    if (NULL != object->stencilBufferTarget) {
      IDirect3DSurface8Impl_AddRef((LPDIRECT3DSURFACE8) object->stencilBufferTarget);
    }

    ENTER_GL();

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
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    /* 
     * Initialize openGL extension related variables
     *  with Default values 
     */
    IDirect3D8Impl_FillGLCaps(iface, object->display);

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
