/* Direct3D Device
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains the MESA implementation of all the D3D devices that
 * Wine supports.
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

#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winerror.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "mesa_private.h"
#include "main.h"

#include "x11drv.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* They are non-static as they are used by Direct3D in the creation function */
const GUID IID_D3DDEVICE_OpenGL = {
  0x31416d44,
  0x86ae,
  0x11d2,
  { 0x82,0x2d,0xa8,0xd5,0x31,0x87,0xca,0xfa }
};

#ifndef HAVE_GLEXT_PROTOTYPES
/* This is for non-OpenGL ABI compliant glext.h headers :-) */
typedef void (* PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat,
					 GLsizei width, GLenum format, GLenum type,
					 const GLvoid *table);
#endif

const float id_mat[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

static void draw_primitive_strided(IDirect3DDeviceImpl *This,
				   D3DPRIMITIVETYPE d3dptPrimitiveType,
				   DWORD d3dvtVertexType,
				   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
				   DWORD dwStartVertex,
				   DWORD dwVertexCount,
				   LPWORD dwIndices,
				   DWORD dwIndexCount,
				   DWORD dwFlags) ;

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;

    return display;
}


/* retrieve the X drawable to use on a given DC */
inline static Drawable get_drawable( HDC hdc )
{
    Drawable drawable;
    enum x11drv_escape_codes escape = X11DRV_GET_DRAWABLE;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(drawable), (LPSTR)&drawable )) drawable = 0;

    return drawable;
}


static BOOL opengl_flip( LPVOID display, LPVOID drawable)
{
    TRACE("(%p, %ld)\n",(Display*)display,(Drawable)drawable);
    ENTER_GL();
    glXSwapBuffers((Display*)display,(Drawable)drawable);
    LEAVE_GL();
    return TRUE;
}


/*******************************************************************************
 *				OpenGL static functions
 */
static void set_context(IDirect3DDeviceImpl* This)
{
    IDirect3DDeviceGLImpl* glThis = (IDirect3DDeviceGLImpl*) This;
   
    ENTER_GL();
    TRACE("glxMakeCurrent %p, %ld, %p\n",glThis->display,glThis->drawable, glThis->gl_context);
    if (glXMakeCurrent(glThis->display, glThis->drawable, glThis->gl_context) == False) {
	ERR("Error in setting current context (context %p drawable %ld)!\n",
	    glThis->gl_context, glThis->drawable);
    }
    LEAVE_GL();
}

static void fill_opengl_primcaps(D3DPRIMCAPS *pc)
{
    pc->dwSize = sizeof(*pc);
    pc->dwMiscCaps = D3DPMISCCAPS_CONFORMANT | D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW |
      D3DPMISCCAPS_LINEPATTERNREP | D3DPMISCCAPS_MASKZ;
    pc->dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_FOGTABLE |
      D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_STIPPLE | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_SUBPIXEL;
    pc->dwZCmpCaps = D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
      D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL;
    pc->dwSrcBlendCaps  = D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_DESTCOLOR | D3DPBLENDCAPS_INVDESTCOLOR |
      D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_SRCALPHASAT |
	D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA;
    pc->dwDestBlendCaps = D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR | D3DPBLENDCAPS_INVSRCCOLOR |
      D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_SRCALPHASAT |
	D3DPBLENDCAPS_BOTHSRCALPHA | D3DPBLENDCAPS_BOTHINVSRCALPHA;
    pc->dwAlphaCmpCaps  = D3DPCMPCAPS_ALWAYS | D3DPCMPCAPS_EQUAL | D3DPCMPCAPS_GREATER | D3DPCMPCAPS_GREATEREQUAL |
      D3DPCMPCAPS_LESS | D3DPCMPCAPS_LESSEQUAL | D3DPCMPCAPS_NEVER | D3DPCMPCAPS_NOTEQUAL;
    pc->dwShadeCaps = D3DPSHADECAPS_ALPHAFLATBLEND | D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_COLORFLATRGB | D3DPSHADECAPS_COLORGOURAUDRGB |
      D3DPSHADECAPS_FOGFLAT | D3DPSHADECAPS_FOGGOURAUD | D3DPSHADECAPS_SPECULARFLATRGB | D3DPSHADECAPS_SPECULARGOURAUDRGB;
    pc->dwTextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_ALPHAPALETTE | D3DPTEXTURECAPS_BORDER | D3DPTEXTURECAPS_PERSPECTIVE |
      D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_TRANSPARENCY;
    pc->dwTextureFilterCaps = D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_LINEARMIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST |
      D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_MIPNEAREST | D3DPTFILTERCAPS_NEAREST;
    pc->dwTextureBlendCaps = D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_DECALMASK |
      D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA | D3DPTBLENDCAPS_MODULATEMASK;
    pc->dwTextureAddressCaps = D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_INDEPENDENTUV;
    pc->dwStippleWidth = 32;
    pc->dwStippleHeight = 32;
}

static void fill_opengl_caps(D3DDEVICEDESC *d1)
{
    /* GLint maxlight; */

    d1->dwSize  = sizeof(*d1);
    d1->dwFlags = D3DDD_DEVCAPS | D3DDD_BCLIPPING | D3DDD_COLORMODEL | D3DDD_DEVICERENDERBITDEPTH | D3DDD_DEVICEZBUFFERBITDEPTH
      | D3DDD_LIGHTINGCAPS | D3DDD_LINECAPS | D3DDD_MAXBUFFERSIZE | D3DDD_MAXVERTEXCOUNT | D3DDD_TRANSFORMCAPS | D3DDD_TRICAPS;
    d1->dcmColorModel = D3DCOLOR_RGB;
    d1->dwDevCaps = D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY |
      D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
      D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
      /* D3D 7 capabilities */
      D3DDEVCAPS_DRAWPRIMITIVES2 | D3DDEVCAPS_HWTRANSFORMANDLIGHT | D3DDEVCAPS_HWRASTERIZATION;
    d1->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    d1->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    d1->bClipping = TRUE;
    d1->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    d1->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_PARALLELPOINT | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;
    d1->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    d1->dlcLightingCaps.dwNumLights = 16; /* glGetIntegerv(GL_MAX_LIGHTS, &maxlight); d1->dlcLightingCaps.dwNumLights = maxlight; */
    fill_opengl_primcaps(&(d1->dpcLineCaps));
    fill_opengl_primcaps(&(d1->dpcTriCaps));
    d1->dwDeviceRenderBitDepth  = DDBD_16|DDBD_24|DDBD_32;
    d1->dwDeviceZBufferBitDepth = DDBD_16|DDBD_24|DDBD_32;
    d1->dwMaxBufferSize = 0;
    d1->dwMaxVertexCount = 65536;
    d1->dwMinTextureWidth  = 1;
    d1->dwMinTextureHeight = 1;
    d1->dwMaxTextureWidth  = 1024;
    d1->dwMaxTextureHeight = 1024;
    d1->dwMinStippleWidth  = 1;
    d1->dwMinStippleHeight = 1;
    d1->dwMaxStippleWidth  = 32;
    d1->dwMaxStippleHeight = 32;
    d1->dwMaxTextureRepeat = 16;
    d1->dwMaxTextureAspectRatio = 1024;
    d1->dwMaxAnisotropy = 0;
    d1->dvGuardBandLeft = 0.0;
    d1->dvGuardBandRight = 0.0;
    d1->dvGuardBandTop = 0.0;
    d1->dvGuardBandBottom = 0.0;
    d1->dvExtentsAdjust = 0.0;
    d1->dwStencilCaps = D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INCRSAT | D3DSTENCILCAPS_INVERT | D3DSTENCILCAPS_KEEP |
      D3DSTENCILCAPS_REPLACE | D3DSTENCILCAPS_ZERO;
    d1->dwFVFCaps = D3DFVFCAPS_DONOTSTRIPELEMENTS | 1;
    d1->dwTextureOpCaps = 0; /* TODO add proper caps according to OpenGL multi-texture stuff */
    d1->wMaxTextureBlendStages = 1;  /* TODO add proper caps according to OpenGL multi-texture stuff */
    d1->wMaxSimultaneousTextures = 1;  /* TODO add proper caps according to OpenGL multi-texture stuff */
}

static void fill_opengl_caps_7(D3DDEVICEDESC7 *d)
{
    D3DDEVICEDESC d1;

    /* Copy first D3D1/2/3 capabilities */
    fill_opengl_caps(&d1);

    /* And fill the D3D7 one with it */
    d->dwDevCaps = d1.dwDevCaps;
    d->dpcLineCaps = d1.dpcLineCaps;
    d->dpcTriCaps = d1.dpcTriCaps;
    d->dwDeviceRenderBitDepth = d1.dwDeviceRenderBitDepth;
    d->dwDeviceZBufferBitDepth = d1.dwDeviceZBufferBitDepth;
    d->dwMinTextureWidth = d1.dwMinTextureWidth;
    d->dwMinTextureHeight = d1.dwMinTextureHeight;
    d->dwMaxTextureWidth = d1.dwMaxTextureWidth;
    d->dwMaxTextureHeight = d1.dwMaxTextureHeight;
    d->dwMaxTextureRepeat = d1.dwMaxTextureRepeat;
    d->dwMaxTextureAspectRatio = d1.dwMaxTextureAspectRatio;
    d->dwMaxAnisotropy = d1.dwMaxAnisotropy;
    d->dvGuardBandLeft = d1.dvGuardBandLeft;
    d->dvGuardBandTop = d1.dvGuardBandTop;
    d->dvGuardBandRight = d1.dvGuardBandRight;
    d->dvGuardBandBottom = d1.dvGuardBandBottom;
    d->dvExtentsAdjust = d1.dvExtentsAdjust;
    d->dwStencilCaps = d1.dwStencilCaps;
    d->dwFVFCaps = d1.dwFVFCaps;
    d->dwTextureOpCaps = d1.dwTextureOpCaps;
    d->wMaxTextureBlendStages = d1.wMaxTextureBlendStages;
    d->wMaxSimultaneousTextures = d1.wMaxSimultaneousTextures;
    d->dwMaxActiveLights = d1.dlcLightingCaps.dwNumLights;
    d->dvMaxVertexW = 100000000.0; /* No idea exactly what to put here... */
    d->deviceGUID = IID_IDirect3DTnLHalDevice;
    d->wMaxUserClipPlanes = 1;
    d->wMaxVertexBlendMatrices = 0;
    d->dwVertexProcessingCaps = D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7 | D3DVTXPCAPS_VERTEXFOG | D3DVTXPCAPS_DIRECTIONALLIGHTS |
      D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER;
    d->dwReserved1 = 0;
    d->dwReserved2 = 0;
    d->dwReserved3 = 0;
    d->dwReserved4 = 0;
}

#if 0 /* TODO : fix this and add multitexturing and other needed stuff */
static void fill_device_capabilities(IDirectDrawImpl* ddraw)
{
    x11_dd_private *private = (x11_dd_private *) ddraw->d->private;
    const char *ext_string;
    Mesa_DeviceCapabilities *devcap;

    private->device_capabilities = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(Mesa_DeviceCapabilities));
    devcap = (Mesa_DeviceCapabilities *) private->device_capabilities;

    ENTER_GL();
    ext_string = glGetString(GL_EXTENSIONS);
    /* Query for the ColorTable Extension */
    if (strstr(ext_string, "GL_EXT_paletted_texture")) {
        devcap->ptr_ColorTableEXT = (PFNGLCOLORTABLEEXTPROC) glXGetProcAddressARB("glColorTableEXT");
	TRACE("Color table extension supported (function at %p)\n", devcap->ptr_ColorTableEXT);
    } else {
        TRACE("Color table extension not found.\n");
    }
    LEAVE_GL();
}
#endif



HRESULT d3ddevice_enumerate(LPD3DENUMDEVICESCALLBACK cb, LPVOID context)
{
    D3DDEVICEDESC dref, d1, d2;
    HRESULT ret_value;
    
    fill_opengl_caps(&dref);

    TRACE(" enumerating OpenGL D3DDevice interface using reference IID (IID %s).\n", debugstr_guid(&IID_IDirect3DRefDevice));
    d1 = dref;
    d2 = dref;
    ret_value = cb((LPIID) &IID_IDirect3DRefDevice, "WINE Reference Direct3DX using OpenGL", "direct3d", &d1, &d2, context);
    if (ret_value != D3DENUMRET_OK)
        return ret_value;

    TRACE(" enumerating OpenGL D3DDevice interface (IID %s).\n", debugstr_guid(&IID_D3DDEVICE_OpenGL));
    d1 = dref;
    d2 = dref;
    ret_value = cb((LPIID) &IID_D3DDEVICE_OpenGL, "WINE Direct3DX using OpenGL", "direct3d", &d1, &d2, context);
    if (ret_value != D3DENUMRET_OK)
        return ret_value;

    return D3DENUMRET_OK;
}

HRESULT d3ddevice_enumerate7(LPD3DENUMDEVICESCALLBACK7 cb, LPVOID context)
{
    D3DDEVICEDESC7 ddesc;

    fill_opengl_caps_7(&ddesc);
    
    TRACE(" enumerating OpenGL D3DDevice7 interface.\n");
    
    return cb("WINE Direct3D7 using OpenGL", "Wine D3D7 device", &ddesc, context);
}

ULONG WINAPI
GL_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    if (!--(This->ref)) {
        int i;
	/* Release texture associated with the device */ 
	for (i = 0; i < MAX_TEXTURES; i++) 
	    if (This->current_texture[i] != NULL)
	        IDirectDrawSurface7_Release(ICOM_INTERFACE(This->current_texture[i], IDirectDrawSurface7));

	/* And warn the D3D object that this device is no longer active... */
	This->d3d->removed_device(This->d3d, This);

	HeapFree(GetProcessHeap(), 0, This->world_mat);
	HeapFree(GetProcessHeap(), 0, This->view_mat);
	HeapFree(GetProcessHeap(), 0, This->proj_mat);

	ENTER_GL();
	glXDestroyContext(glThis->display, glThis->gl_context);
	LEAVE_GL();
	HeapFree(GetProcessHeap(), 0, This->clipping_planes);

	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_3_2T_1T_GetCaps(LPDIRECT3DDEVICE3 iface,
				       LPD3DDEVICEDESC lpD3DHWDevDesc,
				       LPD3DDEVICEDESC lpD3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    D3DDEVICEDESC desc;
    DWORD dwSize;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DHWDevDesc, lpD3DHELDevDesc);

    fill_opengl_caps(&desc);
    dwSize = lpD3DHWDevDesc->dwSize;
    memset(lpD3DHWDevDesc, 0, dwSize);
    memcpy(lpD3DHWDevDesc, &desc, (dwSize <= desc.dwSize ? dwSize : desc.dwSize));

    dwSize = lpD3DHELDevDesc->dwSize;
    memset(lpD3DHELDevDesc, 0, dwSize);
    memcpy(lpD3DHELDevDesc, &desc, (dwSize <= desc.dwSize ? dwSize : desc.dwSize));

    TRACE(" returning caps : (no dump function yet)\n");

    return DD_OK;
}

static HRESULT enum_texture_format_OpenGL(LPD3DENUMTEXTUREFORMATSCALLBACK cb_1,
					  LPD3DENUMPIXELFORMATSCALLBACK cb_2,
					  LPVOID context)
{
    DDSURFACEDESC sdesc;
    LPDDPIXELFORMAT pformat;

    /* Do the texture enumeration */
    sdesc.dwSize = sizeof(DDSURFACEDESC);
    sdesc.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
    sdesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    pformat = &(sdesc.ddpfPixelFormat);
    pformat->dwSize = sizeof(DDPIXELFORMAT);
    pformat->dwFourCC = 0;

    TRACE("Enumerating GL_RGBA unpacked (32)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 32;
    pformat->u2.dwRBitMask =        0xFF000000;
    pformat->u3.dwGBitMask =        0x00FF0000;
    pformat->u4.dwBBitMask =        0x0000FF00;
    pformat->u5.dwRGBAlphaBitMask = 0x000000FF;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB unpacked (24)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 24;
    pformat->u2.dwRBitMask = 0x00FF0000;
    pformat->u3.dwGBitMask = 0x0000FF00;
    pformat->u4.dwBBitMask = 0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask = 0x0000F800;
    pformat->u3.dwGBitMask = 0x000007E0;
    pformat->u4.dwBBitMask = 0x0000001F;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    /* Note : even if this is an 'emulated' texture format, it needs to be first
              as some dumb applications seem to rely on that. */
    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_1_5_5_5 (ARGB) (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =        0x00007C00;
    pformat->u3.dwGBitMask =        0x000003E0;
    pformat->u4.dwBBitMask =        0x0000001F;
    pformat->u5.dwRGBAlphaBitMask = 0x00008000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (ARGB) (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =        0x00000F00;
    pformat->u3.dwGBitMask =        0x000000F0;
    pformat->u4.dwBBitMask =        0x0000000F;
    pformat->u5.dwRGBAlphaBitMask = 0x0000F000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

#if 0
    /* This is a compromise : some games choose the first 16 bit texture format with alpha they
       find enumerated, others the last one. And both want to have the ARGB one.
       
       So basically, forget our OpenGL roots and do not even enumerate our RGBA ones.
    */
    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =        0x0000F000;
    pformat->u3.dwGBitMask =        0x00000F00;
    pformat->u4.dwBBitMask =        0x000000F0;
    pformat->u5.dwRGBAlphaBitMask = 0x0000000F;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_5_5_5_1 (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =        0x0000F800;
    pformat->u3.dwGBitMask =        0x000007C0;
    pformat->u4.dwBBitMask =        0x0000003E;
    pformat->u5.dwRGBAlphaBitMask = 0x00000001;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;
#endif

    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_BYTE_3_3_2 (8)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 8;
    pformat->u2.dwRBitMask =        0x000000E0;
    pformat->u3.dwGBitMask =        0x0000001C;
    pformat->u4.dwBBitMask =        0x00000003;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating Paletted (8)\n");
    pformat->dwFlags = DDPF_PALETTEINDEXED8;
    pformat->u1.dwRGBBitCount = 8;
    pformat->u2.dwRBitMask =        0x00000000;
    pformat->u3.dwGBitMask =        0x00000000;
    pformat->u4.dwBBitMask =        0x00000000;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("End of enumeration\n");
    return DD_OK;
}


HRESULT
d3ddevice_find(IDirect3DImpl *d3d,
	       LPD3DFINDDEVICESEARCH lpD3DDFS,
	       LPD3DFINDDEVICERESULT lplpD3DDevice)
{
    D3DDEVICEDESC desc;
  
    if ((lpD3DDFS->dwFlags & D3DFDS_COLORMODEL) &&
	(lpD3DDFS->dcmColorModel != D3DCOLOR_RGB)) {
        TRACE(" trying to request a non-RGB D3D color model. Not supported.\n");
	return DDERR_INVALIDPARAMS; /* No real idea what to return here :-) */
    }
    if (lpD3DDFS->dwFlags & D3DFDS_GUID) {
        TRACE(" trying to match guid %s.\n", debugstr_guid(&(lpD3DDFS->guid)));
	if ((IsEqualGUID(&IID_D3DDEVICE_OpenGL, &(lpD3DDFS->guid)) == 0) &&
	    (IsEqualGUID(&IID_IDirect3DHALDevice, &(lpD3DDFS->guid)) == 0) &&
	    (IsEqualGUID(&IID_IDirect3DRefDevice, &(lpD3DDFS->guid)) == 0)) {
	    TRACE(" no match for this GUID.\n");
	    return DDERR_INVALIDPARAMS;
	}
    }

    /* Now return our own GUID */
    lplpD3DDevice->guid = IID_D3DDEVICE_OpenGL;
    fill_opengl_caps(&desc);
    lplpD3DDevice->ddHwDesc = desc;
    lplpD3DDevice->ddSwDesc = desc;

    TRACE(" returning Wine's OpenGL device with (undumped) capabilities\n");
    
    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_2_1T_EnumTextureFormats(LPDIRECT3DDEVICE2 iface,
					       LPD3DENUMTEXTUREFORMATSCALLBACK lpD3DEnumTextureProc,
					       LPVOID lpArg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DEnumTextureProc, lpArg);
    return enum_texture_format_OpenGL(lpD3DEnumTextureProc, NULL, lpArg);
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_EnumTextureFormats(LPDIRECT3DDEVICE7 iface,
					       LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
					       LPVOID lpArg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DEnumPixelProc, lpArg);
    return enum_texture_format_OpenGL(NULL, lpD3DEnumPixelProc, lpArg);
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_2T_SetRenderState(LPDIRECT3DDEVICE7 iface,
					      D3DRENDERSTATETYPE dwRenderStateType,
					      DWORD dwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08x,%08lx)\n", This, iface, dwRenderStateType, dwRenderState);

    /* Call the render state functions */
    store_render_state(This, dwRenderStateType, dwRenderState, &This->state_block);
    set_render_state(This, dwRenderStateType, &This->state_block);

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_2T_GetRenderState(LPDIRECT3DDEVICE7 iface,
					      D3DRENDERSTATETYPE dwRenderStateType,
					      LPDWORD lpdwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dwRenderStateType, lpdwRenderState);

    /* Call the render state functions */
    get_render_state(This, dwRenderStateType, lpdwRenderState, &This->state_block);

    TRACE(" - asked for rendering state : %s, returning value %08lx.\n", _get_renderstate(dwRenderStateType), *lpdwRenderState);

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_3_2T_SetLightState(LPDIRECT3DDEVICE3 iface,
					  D3DLIGHTSTATETYPE dwLightStateType,
					  DWORD dwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);

    TRACE("(%p/%p)->(%08x,%08lx)\n", This, iface, dwLightStateType, dwLightState);

    switch (dwLightStateType) {
        case D3DLIGHTSTATE_MATERIAL: {  /* 1 */
	    IDirect3DMaterialImpl *mat = (IDirect3DMaterialImpl *) dwLightState;

	    if (mat != NULL) {
	        ENTER_GL();
		mat->activate(mat);
		LEAVE_GL();
	    } else {
	        ERR(" D3DLIGHTSTATE_MATERIAL called with NULL material !!!\n");
	    }
	} break;

	case D3DLIGHTSTATE_AMBIENT:     /* 2 */
	    IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
					    D3DRENDERSTATE_AMBIENT,
					    dwLightState);
	    break;

#define UNSUP(x) case D3DLIGHTSTATE_##x: FIXME("unsupported D3DLIGHTSTATE_" #x "!\n");break;
	UNSUP(COLORMODEL);
	UNSUP(FOGMODE);
	UNSUP(FOGSTART);
	UNSUP(FOGEND);
	UNSUP(FOGDENSITY);
	UNSUP(COLORVERTEX);
#undef UNSUP

	default:
	    TRACE("Unexpected Light State Type\n");
	    return DDERR_INVALIDPARAMS;
    }
    
    return DD_OK;
}

static void draw_primitive_start_GL(D3DPRIMITIVETYPE d3dpt)
{
    switch (d3dpt) {
        case D3DPT_POINTLIST:
            TRACE("Start POINTS\n");
	    glBegin(GL_POINTS);
	    break;

	case D3DPT_LINELIST:
	    TRACE("Start LINES\n");
	    glBegin(GL_LINES);
	    break;

	case D3DPT_LINESTRIP:
	    TRACE("Start LINE_STRIP\n");
	    glBegin(GL_LINE_STRIP);
	    break;

	case D3DPT_TRIANGLELIST:
	    TRACE("Start TRIANGLES\n");
	    glBegin(GL_TRIANGLES);
	    break;

	case D3DPT_TRIANGLESTRIP:
	    TRACE("Start TRIANGLE_STRIP\n");
	    glBegin(GL_TRIANGLE_STRIP);
	    break;

	case D3DPT_TRIANGLEFAN:
	    TRACE("Start TRIANGLE_FAN\n");
	    glBegin(GL_TRIANGLE_FAN);
	    break;

	default:
	    TRACE("Unhandled primitive\n");
	    break;
    }
}

static void draw_primitive_handle_GL_state(IDirect3DDeviceImpl *This,
					   BOOLEAN vertex_transformed,
					   BOOLEAN vertex_lit) {
    IDirect3DDeviceGLImpl* glThis = (IDirect3DDeviceGLImpl*) This;
  
    /* Puts GL in the correct lighting / transformation mode */
    if ((vertex_transformed == FALSE) && 
	(glThis->transform_state != GL_TRANSFORM_NORMAL)) {
        /* Need to put the correct transformation again if we go from Transformed
	   vertices to non-transformed ones.
	*/
        This->set_matrices(This, VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED,
			   This->world_mat, This->view_mat, This->proj_mat);
	glThis->transform_state = GL_TRANSFORM_NORMAL;

	if (This->state_block.render_state[D3DRENDERSTATE_FOGENABLE - 1] == TRUE)
	    glEnable(GL_FOG);
    } else if ((vertex_transformed == TRUE) &&
	       (glThis->transform_state != GL_TRANSFORM_ORTHO)) {
        /* Set our orthographic projection */
        glThis->transform_state = GL_TRANSFORM_ORTHO;
	d3ddevice_set_ortho(This);
    
	/* Remove also fogging... */
	glDisable(GL_FOG);
    }

    /* Handle the 'no-normal' case */
    if (vertex_lit == TRUE)
        glDisable(GL_LIGHTING);
    else if (This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE)
        glEnable(GL_LIGHTING);

    /* Handle the code for pre-vertex material properties */
    if (vertex_transformed == FALSE) {
        if (This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE) {
	    if ((This->state_block.render_state[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE - 1] != D3DMCS_MATERIAL) ||
		(This->state_block.render_state[D3DRENDERSTATE_AMBIENTMATERIALSOURCE - 1] != D3DMCS_MATERIAL) ||
		(This->state_block.render_state[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE - 1] != D3DMCS_MATERIAL) ||
		(This->state_block.render_state[D3DRENDERSTATE_SPECULARMATERIALSOURCE - 1] != D3DMCS_MATERIAL)) {
	        glEnable(GL_COLOR_MATERIAL);
	    }
	}
    }
}


inline static void draw_primitive(IDirect3DDeviceImpl *This, DWORD maxvert, WORD *index,
				  D3DVERTEXTYPE d3dvt, D3DPRIMITIVETYPE d3dpt, void *lpvertex)
{
    D3DDRAWPRIMITIVESTRIDEDDATA strided;

    switch (d3dvt) {
        case D3DVT_VERTEX: {
	    strided.position.lpvData = &((D3DVERTEX *) lpvertex)->u1.x;
	    strided.position.dwStride = sizeof(D3DVERTEX);
	    strided.normal.lpvData = &((D3DVERTEX *) lpvertex)->u4.nx;
	    strided.normal.dwStride = sizeof(D3DVERTEX);
	    strided.textureCoords[0].lpvData = &((D3DVERTEX *) lpvertex)->u7.tu;
	    strided.textureCoords[0].dwStride = sizeof(D3DVERTEX);
	    draw_primitive_strided(This, d3dpt, D3DFVF_VERTEX, &strided, 0, 0 /* Unused */, index, maxvert, 0 /* Unused */);
	} break;

        case D3DVT_LVERTEX: {
	    strided.position.lpvData = &((D3DLVERTEX *) lpvertex)->u1.x;
	    strided.position.dwStride = sizeof(D3DLVERTEX);
	    strided.diffuse.lpvData = &((D3DLVERTEX *) lpvertex)->u4.color;
	    strided.diffuse.dwStride = sizeof(D3DLVERTEX);
	    strided.specular.lpvData = &((D3DLVERTEX *) lpvertex)->u5.specular;
	    strided.specular.dwStride = sizeof(D3DLVERTEX);
	    strided.textureCoords[0].lpvData = &((D3DLVERTEX *) lpvertex)->u6.tu;
	    strided.textureCoords[0].dwStride = sizeof(D3DLVERTEX);
	    draw_primitive_strided(This, d3dpt, D3DFVF_LVERTEX, &strided, 0, 0 /* Unused */, index, maxvert, 0 /* Unused */);
	} break;

        case D3DVT_TLVERTEX: {
	    strided.position.lpvData = &((D3DTLVERTEX *) lpvertex)->u1.sx;
	    strided.position.dwStride = sizeof(D3DTLVERTEX);
	    strided.diffuse.lpvData = &((D3DTLVERTEX *) lpvertex)->u5.color;
	    strided.diffuse.dwStride = sizeof(D3DTLVERTEX);
	    strided.specular.lpvData = &((D3DTLVERTEX *) lpvertex)->u6.specular;
	    strided.specular.dwStride = sizeof(D3DTLVERTEX);
	    strided.textureCoords[0].lpvData = &((D3DTLVERTEX *) lpvertex)->u7.tu;
	    strided.textureCoords[0].dwStride = sizeof(D3DTLVERTEX);
	    draw_primitive_strided(This, d3dpt, D3DFVF_TLVERTEX, &strided, 0, 0 /* Unused */, index, maxvert, 0 /* Unused */);
	} break;

        default:
	    FIXME("Unhandled vertex type\n");
	    break;
    }
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_2_DrawPrimitive(LPDIRECT3DDEVICE2 iface,
				       D3DPRIMITIVETYPE d3dptPrimitiveType,
				       D3DVERTEXTYPE d3dvtVertexType,
				       LPVOID lpvVertices,
				       DWORD dwVertexCount,
				       DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);

    TRACE("(%p/%p)->(%08x,%08x,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    draw_primitive(This, dwVertexCount, NULL, d3dvtVertexType, d3dptPrimitiveType, lpvVertices);
		   
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_2_DrawIndexedPrimitive(LPDIRECT3DDEVICE2 iface,
					      D3DPRIMITIVETYPE d3dptPrimitiveType,
					      D3DVERTEXTYPE d3dvtVertexType,
					      LPVOID lpvVertices,
					      DWORD dwVertexCount,
					      LPWORD dwIndices,
					      DWORD dwIndexCount,
					      DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE("(%p/%p)->(%08x,%08x,%p,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    draw_primitive(This, dwIndexCount, dwIndices, d3dvtVertexType, d3dptPrimitiveType, lpvVertices);
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_1_CreateExecuteBuffer(LPDIRECT3DDEVICE iface,
					     LPD3DEXECUTEBUFFERDESC lpDesc,
					     LPDIRECT3DEXECUTEBUFFER* lplpDirect3DExecuteBuffer,
					     IUnknown* pUnkOuter)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DExecuteBufferImpl *ret;
    HRESULT ret_value;
    
    TRACE("(%p/%p)->(%p,%p,%p)\n", This, iface, lpDesc, lplpDirect3DExecuteBuffer, pUnkOuter);

    ret_value = d3dexecutebuffer_create(&ret, This->d3d, This, lpDesc);
    *lplpDirect3DExecuteBuffer = ICOM_INTERFACE(ret, IDirect3DExecuteBuffer);

    TRACE(" returning %p.\n", *lplpDirect3DExecuteBuffer);
    
    return ret_value;
}

/* These are the various handler used in the generic path */
inline static void handle_xyz(D3DVALUE *coords) {
    glVertex3fv(coords);
}
inline static void handle_xyzrhw(D3DVALUE *coords) {
    if (coords[3] < 1e-8)
        glVertex3fv(coords);
    else {
        GLfloat w = 1.0 / coords[3];
	
        glVertex4f(coords[0] * w,
		   coords[1] * w,
		   coords[2] * w,
		   w);
    }
}
inline static void handle_normal(D3DVALUE *coords) {
    glNormal3fv(coords);
}

inline static void handle_diffuse_base(STATEBLOCK *sb, DWORD *color) {
    if ((sb->render_state[D3DRENDERSTATE_ALPHATESTENABLE - 1] == TRUE) ||
	(sb->render_state[D3DRENDERSTATE_ALPHABLENDENABLE - 1] == TRUE)) {
        glColor4ub((*color >> 16) & 0xFF,
		   (*color >>  8) & 0xFF,
		   (*color >>  0) & 0xFF,
		   (*color >> 24) & 0xFF);
    } else {
        glColor3ub((*color >> 16) & 0xFF,
		   (*color >>  8) & 0xFF,
		   (*color >>  0) & 0xFF);    
    }
}

inline static void handle_specular_base(STATEBLOCK *sb, DWORD *color) {
    glColor4ub((*color >> 16) & 0xFF,
	       (*color >>  8) & 0xFF,
	       (*color >>  0) & 0xFF,
	       (*color >> 24) & 0xFF); /* No idea if the alpha field is really used.. */
}

inline static void handle_diffuse(STATEBLOCK *sb, DWORD *color, BOOLEAN lighted) {
    if ((lighted == FALSE) &&
	(sb->render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE)) {
        if (sb->render_state[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE - 1] == D3DMCS_COLOR1) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	    handle_diffuse_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_AMBIENTMATERIALSOURCE - 1] == D3DMCS_COLOR1) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
	    handle_diffuse_base(sb, color);
	}
	if ((sb->render_state[D3DRENDERSTATE_SPECULARMATERIALSOURCE - 1] == D3DMCS_COLOR1) &&
	    (sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1] == TRUE)) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);
	    handle_diffuse_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE - 1] == D3DMCS_COLOR1) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
	    handle_diffuse_base(sb, color);
	}
    } else {
        handle_diffuse_base(sb, color);
    }    
}

inline static void handle_specular(STATEBLOCK *sb, DWORD *color, BOOLEAN lighted) {
    if ((lighted == FALSE) &&
	(sb->render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE)) {
        if (sb->render_state[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE - 1] == D3DMCS_COLOR2) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	    handle_specular_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_AMBIENTMATERIALSOURCE - 1] == D3DMCS_COLOR2) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
	    handle_specular_base(sb, color);
	}
	if ((sb->render_state[D3DRENDERSTATE_SPECULARMATERIALSOURCE - 1] == D3DMCS_COLOR2) &&
	    (sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1] == TRUE)) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_SPECULAR);
	    handle_specular_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE - 1] == D3DMCS_COLOR2) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
	    handle_specular_base(sb, color);
	}
    }
    /* No else here as we do not know how to handle 'specular' on its own in any case.. */
}

inline static void handle_diffuse_and_specular(STATEBLOCK *sb, DWORD *color_d, DWORD *color_s, BOOLEAN lighted) {
    if (lighted == TRUE) {
        if (sb->render_state[D3DRENDERSTATE_FOGENABLE - 1] == TRUE) {
	    /* Special case where the specular value is used to do fogging. TODO */
	}
	if (sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1] == TRUE) {
	    /* Standard specular value in transformed mode. TODO */
	}
	handle_diffuse_base(sb, color_d);
    } else {
        if (sb->render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE) {
	    handle_diffuse(sb, color_d, FALSE);
	    handle_specular(sb, color_s, FALSE);
	} else {
	    /* In that case, only put the diffuse color... */
	    handle_diffuse_base(sb, color_d);
	}
    }
}

inline static void handle_texture(D3DVALUE *coords) {
    glTexCoord2fv(coords);
}
inline static void handle_textures(D3DVALUE *coords, int tex_index) {
    /* For the moment, draw only the first texture.. */
    if (tex_index == 0) glTexCoord2fv(coords);
}

static void draw_primitive_strided(IDirect3DDeviceImpl *This,
				   D3DPRIMITIVETYPE d3dptPrimitiveType,
				   DWORD d3dvtVertexType,
				   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
				   DWORD dwStartVertex,
				   DWORD dwVertexCount,
				   LPWORD dwIndices,
				   DWORD dwIndexCount,
				   DWORD dwFlags)
{
    BOOLEAN vertex_lighted = (d3dvtVertexType & D3DFVF_NORMAL) == 0;
  
    if (TRACE_ON(ddraw)) {
        TRACE(" Vertex format : "); dump_flexible_vertex(d3dvtVertexType);
    }

    ENTER_GL();
    draw_primitive_handle_GL_state(This,
				   (d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ,
				   vertex_lighted);
    draw_primitive_start_GL(d3dptPrimitiveType);

    /* Some fast paths first before the generic case.... */
    if (d3dvtVertexType == D3DFVF_VERTEX) {
	int index;
	
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];
	    D3DVALUE *normal = 
	      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->normal.lpvData) + i * lpD3DDrawPrimStrideData->normal.dwStride);
	    D3DVALUE *tex_coord =
	      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[0].lpvData) + i * lpD3DDrawPrimStrideData->textureCoords[0].dwStride);
	    D3DVALUE *position =
	      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
	    
	    handle_normal(normal);
	    handle_texture(tex_coord);
	    handle_xyz(position);
	    
	    TRACE(" %f %f %f / %f %f %f (%f %f)\n",
		  position[0], position[1], position[2],
		  normal[0], normal[1], normal[2],
		  tex_coord[0], tex_coord[1]);
	}
    } else if (d3dvtVertexType == D3DFVF_TLVERTEX) {
	int index;
	
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];
	    DWORD *color_d = 
	      (DWORD *) (((char *) lpD3DDrawPrimStrideData->diffuse.lpvData) + i * lpD3DDrawPrimStrideData->diffuse.dwStride);
	    DWORD *color_s = 
	      (DWORD *) (((char *) lpD3DDrawPrimStrideData->specular.lpvData) + i * lpD3DDrawPrimStrideData->specular.dwStride);
	    D3DVALUE *tex_coord =
	      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[0].lpvData) + i * lpD3DDrawPrimStrideData->textureCoords[0].dwStride);
	    D3DVALUE *position =
	      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);

	    handle_diffuse_and_specular(&(This->state_block), color_d, color_s, TRUE);
	    handle_texture(tex_coord);
	    handle_xyzrhw(position);

	    TRACE(" %f %f %f %f / %02lx %02lx %02lx %02lx - %02lx %02lx %02lx %02lx (%f %f)\n",
		  position[0], position[1], position[2], position[3], 
		  (*color_d >> 16) & 0xFF,
		  (*color_d >>  8) & 0xFF,
		  (*color_d >>  0) & 0xFF,
		  (*color_d >> 24) & 0xFF,
		  (*color_s >> 16) & 0xFF,
		  (*color_s >>  8) & 0xFF,
		  (*color_s >>  0) & 0xFF,
		  (*color_s >> 24) & 0xFF,
		  tex_coord[0], tex_coord[1]);
	} 
    } else if (((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) ||
	       ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW)) {
        /* This is the 'slow path' but that should support all possible vertex formats out there...
	   Note that people should write a fast path for all vertex formats out there...
	*/  
	int index;
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];	    

	    if (d3dvtVertexType & D3DFVF_NORMAL) { 
	        D3DVALUE *normal = 
		  (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->normal.lpvData) + i * lpD3DDrawPrimStrideData->normal.dwStride);	    
		handle_normal(normal);
	    }
	    if ((d3dvtVertexType & (D3DFVF_DIFFUSE|D3DFVF_SPECULAR)) == (D3DFVF_DIFFUSE|D3DFVF_SPECULAR)) {
	        DWORD *color_d = 
		  (DWORD *) (((char *) lpD3DDrawPrimStrideData->diffuse.lpvData) + i * lpD3DDrawPrimStrideData->diffuse.dwStride);
		DWORD *color_s = 
		  (DWORD *) (((char *) lpD3DDrawPrimStrideData->specular.lpvData) + i * lpD3DDrawPrimStrideData->specular.dwStride);
		handle_diffuse_and_specular(&(This->state_block), color_d, color_s, vertex_lighted);
	    } else {
	        if (d3dvtVertexType & D3DFVF_SPECULAR) { 
		    DWORD *color_s = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->specular.lpvData) + i * lpD3DDrawPrimStrideData->specular.dwStride);
		    handle_specular(&(This->state_block), color_s, vertex_lighted);
		} else if (d3dvtVertexType & D3DFVF_DIFFUSE) {
		    DWORD *color_d = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->diffuse.lpvData) + i * lpD3DDrawPrimStrideData->diffuse.dwStride);
		    handle_diffuse(&(This->state_block), color_d, vertex_lighted);
		}
	    }
		
	    if (((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) == 1) {
                /* Special case for single texture... */
	        D3DVALUE *tex_coord =
		  (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[0].lpvData) + i * lpD3DDrawPrimStrideData->textureCoords[0].dwStride);
	        handle_texture(tex_coord);
	    } else {
	        int tex_index;
		for (tex_index = 0; tex_index < ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT); tex_index++) {
                    D3DVALUE *tex_coord =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
				    i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
		    handle_textures(tex_coord, tex_index);
		}
	    }
	    if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
	        D3DVALUE *position =
		  (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		handle_xyz(position);
	    } else if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) {
	        D3DVALUE *position =
		  (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		handle_xyzrhw(position);
	    }

	    if (TRACE_ON(ddraw)) {
	        int tex_index;

		if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
		    D3DVALUE *position =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		    TRACE(" %f %f %f", position[0], position[1], position[2]);
		} else if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) {
		    D3DVALUE *position =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		    TRACE(" %f %f %f %f", position[0], position[1], position[2], position[3]);
		}
	        if (d3dvtVertexType & D3DFVF_NORMAL) { 
		    D3DVALUE *normal = 
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->normal.lpvData) + i * lpD3DDrawPrimStrideData->normal.dwStride);	    
		    DPRINTF(" / %f %f %f", normal[0], normal[1], normal[2]);
		}
		if (d3dvtVertexType & D3DFVF_DIFFUSE) {
		    DWORD *color_d = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->diffuse.lpvData) + i * lpD3DDrawPrimStrideData->diffuse.dwStride);
		    DPRINTF(" / %02lx %02lx %02lx %02lx",
			    (*color_d >> 16) & 0xFF,
			    (*color_d >>  8) & 0xFF,
			    (*color_d >>  0) & 0xFF,
			    (*color_d >> 24) & 0xFF);
		}
	        if (d3dvtVertexType & D3DFVF_SPECULAR) { 
		    DWORD *color_s = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->specular.lpvData) + i * lpD3DDrawPrimStrideData->specular.dwStride);
		    DPRINTF(" / %02lx %02lx %02lx %02lx",
			    (*color_s >> 16) & 0xFF,
			    (*color_s >>  8) & 0xFF,
			    (*color_s >>  0) & 0xFF,
			    (*color_s >> 24) & 0xFF);
		}
		for (tex_index = 0; tex_index < ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT); tex_index++) {
                    D3DVALUE *tex_coord =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
				    i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
		    DPRINTF(" / %f %f", tex_coord[0], tex_coord[1]);
		}
		DPRINTF("\n");
	    }
	}
    } else {
        ERR(" matrix weighting not handled yet....\n");
    }
    
    glEnd();

    /* Whatever the case, disable the color material stuff */
    glDisable(GL_COLOR_MATERIAL);

    LEAVE_GL();
    TRACE("End\n");    
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawPrimitive(LPDIRECT3DDEVICE7 iface,
					  D3DPRIMITIVETYPE d3dptPrimitiveType,
					  DWORD d3dvtVertexType,
					  LPVOID lpvVertices,
					  DWORD dwVertexCount,
					  DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    D3DDRAWPRIMITIVESTRIDEDDATA strided;

    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    convert_FVF_to_strided_data(d3dvtVertexType, lpvVertices, &strided);
    draw_primitive_strided(This, d3dptPrimitiveType, d3dvtVertexType, &strided, 0, dwVertexCount, NULL, dwVertexCount, dwFlags);
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive(LPDIRECT3DDEVICE7 iface,
						 D3DPRIMITIVETYPE d3dptPrimitiveType,
						 DWORD d3dvtVertexType,
						 LPVOID lpvVertices,
						 DWORD dwVertexCount,
						 LPWORD dwIndices,
						 DWORD dwIndexCount,
						 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    D3DDRAWPRIMITIVESTRIDEDDATA strided;

    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    convert_FVF_to_strided_data(d3dvtVertexType, lpvVertices, &strided);
    draw_primitive_strided(This, d3dptPrimitiveType, d3dvtVertexType, &strided, 0, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                   D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                   DWORD dwVertexType,
                                                   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                   DWORD dwVertexCount,
                                                   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);

    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }
    draw_primitive_strided(This, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, 0, dwVertexCount, NULL, dwVertexCount, dwFlags);

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided(LPDIRECT3DDEVICE7 iface,
                                                          D3DPRIMITIVETYPE d3dptPrimitiveType,
                                                          DWORD dwVertexType,
                                                          LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
                                                          DWORD dwVertexCount,
                                                          LPWORD lpIndex,
                                                          DWORD dwIndexCount,
                                                          DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);

    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, lpIndex, dwIndexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    draw_primitive_strided(This, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, 0, dwVertexCount, lpIndex, dwIndexCount, dwFlags);

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawPrimitiveVB(LPDIRECT3DDEVICE7 iface,
					    D3DPRIMITIVETYPE d3dptPrimitiveType,
					    LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
					    DWORD dwStartVertex,
					    DWORD dwNumVertices,
					    DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DVertexBufferImpl *vb_impl = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, lpD3DVertexBuf);
    D3DDRAWPRIMITIVESTRIDEDDATA strided;

    TRACE("(%p/%p)->(%08x,%p,%08lx,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    if (vb_impl->processed == TRUE) {
        IDirect3DVertexBufferGLImpl *vb_glimp = (IDirect3DVertexBufferGLImpl *) vb_impl;
	IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;

	glThis->transform_state = GL_TRANSFORM_VERTEXBUFFER;
	This->set_matrices(This, VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED,
			   &(vb_glimp->world_mat), &(vb_glimp->view_mat), &(vb_glimp->proj_mat));

	convert_FVF_to_strided_data(vb_glimp->dwVertexTypeDesc, vb_glimp->vertices, &strided);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_glimp->dwVertexTypeDesc, &strided, dwStartVertex, dwNumVertices, NULL, dwNumVertices, dwFlags);

    } else {
        convert_FVF_to_strided_data(vb_impl->desc.dwFVF, vb_impl->vertices, &strided);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_impl->desc.dwFVF, &strided, dwStartVertex, dwNumVertices, NULL, dwNumVertices, dwFlags);
    }

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveVB(LPDIRECT3DDEVICE7 iface,
						   D3DPRIMITIVETYPE d3dptPrimitiveType,
						   LPDIRECT3DVERTEXBUFFER7 lpD3DVertexBuf,
						   DWORD dwStartVertex,
						   DWORD dwNumVertices,
						   LPWORD lpwIndices,
						   DWORD dwIndexCount,
						   DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DVertexBufferImpl *vb_impl = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, lpD3DVertexBuf);
    D3DDRAWPRIMITIVESTRIDEDDATA strided;
    
    TRACE("(%p/%p)->(%08x,%p,%08lx,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, lpD3DVertexBuf, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - flags : "); dump_DPFLAGS(dwFlags);
    }

    if (vb_impl->processed == TRUE) {
        IDirect3DVertexBufferGLImpl *vb_glimp = (IDirect3DVertexBufferGLImpl *) vb_impl;
	IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;

	glThis->transform_state = GL_TRANSFORM_VERTEXBUFFER;
	This->set_matrices(This, VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED,
			   &(vb_glimp->world_mat), &(vb_glimp->view_mat), &(vb_glimp->proj_mat));

	convert_FVF_to_strided_data(vb_glimp->dwVertexTypeDesc, vb_glimp->vertices, &strided);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_glimp->dwVertexTypeDesc, &strided, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);

    } else {
        convert_FVF_to_strided_data(vb_impl->desc.dwFVF, vb_impl->vertices, &strided);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_impl->desc.dwFVF, &strided, dwStartVertex, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
    }

    return DD_OK;
}

static GLenum
convert_min_filter_to_GL(D3DTEXTUREMINFILTER dwState)
{
    GLenum gl_state;

    switch (dwState) {
        case D3DTFN_POINT:
	    gl_state = GL_NEAREST;
	    break;
        case D3DTFN_LINEAR:
	    gl_state = GL_LINEAR;
	    break;
        default:
	    gl_state = GL_LINEAR;
	    break;
    }
    return gl_state;
}

static GLenum
convert_mag_filter_to_GL(D3DTEXTUREMAGFILTER dwState)
{
    GLenum gl_state;

    switch (dwState) {
        case D3DTFG_POINT:
	    gl_state = GL_NEAREST;
	    break;
        case D3DTFG_LINEAR:
	    gl_state = GL_LINEAR;
	    break;
        default:
	    gl_state = GL_LINEAR;
	    break;
    }
    return gl_state;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
						 DWORD dwStage,
						 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
						 DWORD dwState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    
    TRACE("(%p/%p)->(%08lx,%08x,%08lx)\n", This, iface, dwStage, d3dTexStageStateType, dwState);

    if (dwStage > 0) return DD_OK; /* We nothing in this case for now */

    if (TRACE_ON(ddraw)) {
        TRACE(" Stage type is : ");
	switch (d3dTexStageStateType) {
#define GEN_CASE(a) case a: DPRINTF(#a " "); break
	    GEN_CASE(D3DTSS_COLOROP);
	    GEN_CASE(D3DTSS_COLORARG1);
	    GEN_CASE(D3DTSS_COLORARG2);
	    GEN_CASE(D3DTSS_ALPHAOP);
	    GEN_CASE(D3DTSS_ALPHAARG1);
	    GEN_CASE(D3DTSS_ALPHAARG2);
	    GEN_CASE(D3DTSS_BUMPENVMAT00);
	    GEN_CASE(D3DTSS_BUMPENVMAT01);
	    GEN_CASE(D3DTSS_BUMPENVMAT10);
	    GEN_CASE(D3DTSS_BUMPENVMAT11);
	    GEN_CASE(D3DTSS_TEXCOORDINDEX);
	    GEN_CASE(D3DTSS_ADDRESS);
	    GEN_CASE(D3DTSS_ADDRESSU);
	    GEN_CASE(D3DTSS_ADDRESSV);
	    GEN_CASE(D3DTSS_BORDERCOLOR);
	    GEN_CASE(D3DTSS_MAGFILTER);
	    GEN_CASE(D3DTSS_MINFILTER);
	    GEN_CASE(D3DTSS_MIPFILTER);
	    GEN_CASE(D3DTSS_MIPMAPLODBIAS);
	    GEN_CASE(D3DTSS_MAXMIPLEVEL);
	    GEN_CASE(D3DTSS_MAXANISOTROPY);
	    GEN_CASE(D3DTSS_BUMPENVLSCALE);
	    GEN_CASE(D3DTSS_BUMPENVLOFFSET);
	    GEN_CASE(D3DTSS_TEXTURETRANSFORMFLAGS);
#undef GEN_CASE
	    default: DPRINTF("UNKNOWN !!!");
	}
	DPRINTF(" => ");
    }

    switch (d3dTexStageStateType) {
        case D3DTSS_MINFILTER:
	    if (TRACE_ON(ddraw)) {
	        switch ((D3DTEXTUREMINFILTER) dwState) {
	            case D3DTFN_POINT:  DPRINTF("D3DTFN_POINT\n"); break;
		    case D3DTFN_LINEAR: DPRINTF("D3DTFN_LINEAR\n"); break;
		    default: DPRINTF(" state unhandled (%ld).\n", dwState); break;
		}
	    }
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, convert_min_filter_to_GL(dwState));
            break;
	    
        case D3DTSS_MAGFILTER:
	    if (TRACE_ON(ddraw)) {
	        switch ((D3DTEXTUREMAGFILTER) dwState) {
	            case D3DTFG_POINT:  DPRINTF("D3DTFN_POINT\n"); break;
		    case D3DTFG_LINEAR: DPRINTF("D3DTFN_LINEAR\n"); break;
		    default: DPRINTF(" state unhandled (%ld).\n", dwState); break;
		}
	    }
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, convert_mag_filter_to_GL(dwState));
            break;

        case D3DTSS_ADDRESS:
        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV: {
	    GLenum arg = GL_REPEAT; /* Default value */
	    switch ((D3DTEXTUREADDRESS) dwState) {
	        case D3DTADDRESS_WRAP:   if (TRACE_ON(ddraw)) DPRINTF("D3DTADDRESS_WRAP\n"); arg = GL_REPEAT; break;
	        case D3DTADDRESS_CLAMP:  if (TRACE_ON(ddraw)) DPRINTF("D3DTADDRESS_CLAMP\n"); arg = GL_CLAMP; break;
	        case D3DTADDRESS_BORDER: if (TRACE_ON(ddraw)) DPRINTF("D3DTADDRESS_BORDER\n"); arg = GL_CLAMP_TO_EDGE; break;
	        default: DPRINTF(" state unhandled (%ld).\n", dwState);
	    }
	    if ((d3dTexStageStateType == D3DTSS_ADDRESS) ||
		(d3dTexStageStateType == D3DTSS_ADDRESSU))
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, arg);
	    if ((d3dTexStageStateType == D3DTSS_ADDRESS) ||
		(d3dTexStageStateType == D3DTSS_ADDRESSV))
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, arg);
        } break;
	    
	default:
	    if (TRACE_ON(ddraw)) DPRINTF(" unhandled.\n");
    }
   
    This->state_block.texture_stage_state[dwStage][d3dTexStageStateType - 1] = dwState;
    /* Some special cases when one state modifies more than one... */
    if (d3dTexStageStateType == D3DTSS_ADDRESS) {
        This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSU - 1] = dwState;
	This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSV - 1] = dwState;
    }

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_SetTexture(LPDIRECT3DDEVICE7 iface,
				       DWORD dwStage,
				       LPDIRECTDRAWSURFACE7 lpTexture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwStage, lpTexture2);
    
    if (dwStage > 0) return DD_OK;

    if (This->current_texture[dwStage] != NULL) {
	IDirectDrawSurface7_Release(ICOM_INTERFACE(This->current_texture[dwStage], IDirectDrawSurface7));
    }
    
    ENTER_GL();
    if (lpTexture2 == NULL) {
	This->current_texture[dwStage] = NULL;

        TRACE(" disabling 2D texturing.\n");
	glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        IDirectDrawSurfaceImpl *tex_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpTexture2);
	
	This->current_texture[dwStage] = tex_impl;
	IDirectDrawSurface7_AddRef(ICOM_INTERFACE(tex_impl, IDirectDrawSurface7)); /* Not sure about this either */
	
        glEnable(GL_TEXTURE_2D);
	gltex_upload_texture(tex_impl);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			convert_mag_filter_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_MAGFILTER - 1]));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			convert_min_filter_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_MINFILTER - 1]));
    }
    LEAVE_GL();
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_GetCaps(LPDIRECT3DDEVICE7 iface,
				 LPD3DDEVICEDESC7 lpD3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpD3DHELDevDesc);

    fill_opengl_caps_7(lpD3DHELDevDesc);

    TRACE(" returning caps : no dump function yet.\n");

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_SetMaterial(LPDIRECT3DDEVICE7 iface,
				     LPD3DMATERIAL7 lpMat)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpMat);
    
    if (TRACE_ON(ddraw)) {
        TRACE(" material is : \n");
	dump_D3DMATERIAL7(lpMat);
    }
    
    This->current_material = *lpMat;

    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_DIFFUSE,
		 (float *) &(This->current_material.u.diffuse));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_AMBIENT,
		 (float *) &(This->current_material.u1.ambient));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_SPECULAR,
		 (float *) &(This->current_material.u2.specular));
    glMaterialfv(GL_FRONT_AND_BACK,
		 GL_EMISSION,
		 (float *) &(This->current_material.u3.emissive));
    glMaterialf(GL_FRONT_AND_BACK,
		GL_SHININESS,
		This->current_material.u4.power); /* Not sure about this... */

    return DD_OK;
}


HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_SetLight(LPDIRECT3DDEVICE7 iface,
				  DWORD dwLightIndex,
				  LPD3DLIGHT7 lpLight)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwLightIndex, lpLight);
    
    if (TRACE_ON(ddraw)) {
        TRACE(" setting light : \n");
	dump_D3DLIGHT7(lpLight);
    }
    
    if (dwLightIndex > MAX_LIGHTS) return DDERR_INVALIDPARAMS;
    This->set_lights |= 0x00000001 << dwLightIndex;
    This->light_parameters[dwLightIndex] = *lpLight;

    switch (lpLight->dltType) {
	case D3DLIGHT_DIRECTIONAL: {
	    float direction[4];
	    float cut_off = 180.0;
	    
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_AMBIENT,  (float *) &(lpLight->dcvAmbient));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_DIFFUSE,  (float *) &(lpLight->dcvDiffuse));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPECULAR, (float *) &(lpLight->dcvSpecular));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPOT_CUTOFF, &cut_off);

	    direction[0] = lpLight->dvDirection.u1.x;
	    direction[1] = lpLight->dvDirection.u2.y;
	    direction[2] = lpLight->dvDirection.u3.z;
	    direction[3] = 0.0;
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_POSITION, (float *) direction);
	} break;

        case D3DLIGHT_POINT: {
	    float position[4];
	    float cut_off = 180.0;

	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_AMBIENT,  (float *) &(lpLight->dcvAmbient));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_DIFFUSE,  (float *) &(lpLight->dcvDiffuse));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPECULAR, (float *) &(lpLight->dcvSpecular));
	    position[0] = lpLight->dvPosition.u1.x;
	    position[1] = lpLight->dvPosition.u2.y;
	    position[2] = lpLight->dvPosition.u3.z;
	    position[3] = 1.0;
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_POSITION, (float *) position);
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_CONSTANT_ATTENUATION, &(lpLight->dvAttenuation0));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_LINEAR_ATTENUATION, &(lpLight->dvAttenuation1));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_QUADRATIC_ATTENUATION, &(lpLight->dvAttenuation2));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPOT_CUTOFF, &cut_off);
	} break;

        case D3DLIGHT_SPOT: {
	    float direction[4];
	    float position[4];
	    float cut_off = 90.0 * (lpLight->dvPhi / M_PI);

	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_AMBIENT,  (float *) &(lpLight->dcvAmbient));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_DIFFUSE,  (float *) &(lpLight->dcvDiffuse));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPECULAR, (float *) &(lpLight->dcvSpecular));

	    direction[0] = lpLight->dvDirection.u1.x;
	    direction[1] = lpLight->dvDirection.u2.y;
	    direction[2] = lpLight->dvDirection.u3.z;
	    direction[3] = 0.0;
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPOT_DIRECTION, (float *) direction);
	    position[0] = lpLight->dvPosition.u1.x;
	    position[1] = lpLight->dvPosition.u2.y;
	    position[2] = lpLight->dvPosition.u3.z;
	    position[3] = 1.0;
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_POSITION, (float *) position);
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_CONSTANT_ATTENUATION, &(lpLight->dvAttenuation0));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_LINEAR_ATTENUATION, &(lpLight->dvAttenuation1));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_QUADRATIC_ATTENUATION, &(lpLight->dvAttenuation2));
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPOT_CUTOFF, &cut_off);
	    glLightfv(GL_LIGHT0 + dwLightIndex, GL_SPOT_EXPONENT, &(lpLight->dvFalloff));
	    if ((lpLight->dvTheta != 0.0) || (lpLight->dvTheta != lpLight->dvPhi)) {
	        WARN("dvTheta not fully supported yet !\n");
	    }
	} break;

        default: WARN(" light type not handled yet...\n");
    }

    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_LightEnable(LPDIRECT3DDEVICE7 iface,
				     DWORD dwLightIndex,
				     BOOL bEnable)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%d)\n", This, iface, dwLightIndex, bEnable);
    
    if (dwLightIndex > MAX_LIGHTS) return DDERR_INVALIDPARAMS;

    if (bEnable) {
        if (((0x00000001 << dwLightIndex) & This->set_lights) == 0) {
	    /* Set the default parameters.. */
	    TRACE(" setting default light parameters...\n");
	    GL_IDirect3DDeviceImpl_7_SetLight(iface, dwLightIndex, &(This->light_parameters[dwLightIndex]));
	}
	glEnable(GL_LIGHT0 + dwLightIndex);
    } else {
        glDisable(GL_LIGHT0 + dwLightIndex);
    }

    return DD_OK;
}

HRESULT  WINAPI  
GL_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface, DWORD dwIndex, CONST D3DVALUE* pPlaneEquation) 
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    GLdouble plane[4];

    TRACE("(%p)->(%ld,%p)\n", This, dwIndex, pPlaneEquation);

    if (dwIndex>=This->max_clipping_planes) {
	return DDERR_INVALIDPARAMS;
    }

    TRACE(" clip plane %ld : %f %f %f %f\n", dwIndex, pPlaneEquation[0], pPlaneEquation[1], pPlaneEquation[2], pPlaneEquation[3] );

    memcpy( This->clipping_planes[dwIndex].plane, pPlaneEquation, sizeof(D3DVALUE[4]));
    plane[0] = pPlaneEquation[0];
    plane[1] = pPlaneEquation[1];
    plane[2] = pPlaneEquation[2];
    plane[3] = pPlaneEquation[3];

    /* XXX: is here also code needed to handle the transformation of the world? */
    glClipPlane( GL_CLIP_PLANE0+dwIndex, (const GLdouble*)(&plane) );

    return D3D_OK;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DDevice7.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DDevice7) VTABLE_IDirect3DDevice7 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DDeviceImpl_7_3T_2T_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DDeviceImpl_7_3T_2T_1T_AddRef,
    XCAST(Release) GL_IDirect3DDeviceImpl_7_3T_2T_1T_Release,
    XCAST(GetCaps) GL_IDirect3DDeviceImpl_7_GetCaps,
    XCAST(EnumTextureFormats) GL_IDirect3DDeviceImpl_7_3T_EnumTextureFormats,
    XCAST(BeginScene) Main_IDirect3DDeviceImpl_7_3T_2T_1T_BeginScene,
    XCAST(EndScene) Main_IDirect3DDeviceImpl_7_3T_2T_1T_EndScene,
    XCAST(GetDirect3D) Main_IDirect3DDeviceImpl_7_3T_2T_1T_GetDirect3D,
    XCAST(SetRenderTarget) Main_IDirect3DDeviceImpl_7_3T_2T_SetRenderTarget,
    XCAST(GetRenderTarget) Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderTarget,
    XCAST(Clear) Main_IDirect3DDeviceImpl_7_Clear,
    XCAST(SetTransform) Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform,
    XCAST(GetTransform) Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform,
    XCAST(SetViewport) Main_IDirect3DDeviceImpl_7_SetViewport,
    XCAST(MultiplyTransform) Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform,
    XCAST(GetViewport) Main_IDirect3DDeviceImpl_7_GetViewport,
    XCAST(SetMaterial) GL_IDirect3DDeviceImpl_7_SetMaterial,
    XCAST(GetMaterial) Main_IDirect3DDeviceImpl_7_GetMaterial,
    XCAST(SetLight) GL_IDirect3DDeviceImpl_7_SetLight,
    XCAST(GetLight) Main_IDirect3DDeviceImpl_7_GetLight,
    XCAST(SetRenderState) GL_IDirect3DDeviceImpl_7_3T_2T_SetRenderState,
    XCAST(GetRenderState) GL_IDirect3DDeviceImpl_7_3T_2T_GetRenderState,
    XCAST(BeginStateBlock) Main_IDirect3DDeviceImpl_7_BeginStateBlock,
    XCAST(EndStateBlock) Main_IDirect3DDeviceImpl_7_EndStateBlock,
    XCAST(PreLoad) Main_IDirect3DDeviceImpl_7_PreLoad,
    XCAST(DrawPrimitive) GL_IDirect3DDeviceImpl_7_3T_DrawPrimitive,
    XCAST(DrawIndexedPrimitive) GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive,
    XCAST(SetClipStatus) Main_IDirect3DDeviceImpl_7_3T_2T_SetClipStatus,
    XCAST(GetClipStatus) Main_IDirect3DDeviceImpl_7_3T_2T_GetClipStatus,
    XCAST(DrawPrimitiveStrided) GL_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided,
    XCAST(DrawIndexedPrimitiveStrided) GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided,
    XCAST(DrawPrimitiveVB) GL_IDirect3DDeviceImpl_7_3T_DrawPrimitiveVB,
    XCAST(DrawIndexedPrimitiveVB) GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveVB,
    XCAST(ComputeSphereVisibility) Main_IDirect3DDeviceImpl_7_3T_ComputeSphereVisibility,
    XCAST(GetTexture) Main_IDirect3DDeviceImpl_7_3T_GetTexture,
    XCAST(SetTexture) GL_IDirect3DDeviceImpl_7_3T_SetTexture,
    XCAST(GetTextureStageState) Main_IDirect3DDeviceImpl_7_3T_GetTextureStageState,
    XCAST(SetTextureStageState) GL_IDirect3DDeviceImpl_7_3T_SetTextureStageState,
    XCAST(ValidateDevice) Main_IDirect3DDeviceImpl_7_3T_ValidateDevice,
    XCAST(ApplyStateBlock) Main_IDirect3DDeviceImpl_7_ApplyStateBlock,
    XCAST(CaptureStateBlock) Main_IDirect3DDeviceImpl_7_CaptureStateBlock,
    XCAST(DeleteStateBlock) Main_IDirect3DDeviceImpl_7_DeleteStateBlock,
    XCAST(CreateStateBlock) Main_IDirect3DDeviceImpl_7_CreateStateBlock,
    XCAST(Load) Main_IDirect3DDeviceImpl_7_Load,
    XCAST(LightEnable) GL_IDirect3DDeviceImpl_7_LightEnable,
    XCAST(GetLightEnable) Main_IDirect3DDeviceImpl_7_GetLightEnable,
    XCAST(SetClipPlane) GL_IDirect3DDeviceImpl_7_SetClipPlane,
    XCAST(GetClipPlane) Main_IDirect3DDeviceImpl_7_GetClipPlane,
    XCAST(GetInfo) Main_IDirect3DDeviceImpl_7_GetInfo,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DDevice3.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DDevice3) VTABLE_IDirect3DDevice3 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DDeviceImpl_3_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DDeviceImpl_3_AddRef,
    XCAST(Release) Thunk_IDirect3DDeviceImpl_3_Release,
    XCAST(GetCaps) GL_IDirect3DDeviceImpl_3_2T_1T_GetCaps,
    XCAST(GetStats) Main_IDirect3DDeviceImpl_3_2T_1T_GetStats,
    XCAST(AddViewport) Main_IDirect3DDeviceImpl_3_2T_1T_AddViewport,
    XCAST(DeleteViewport) Main_IDirect3DDeviceImpl_3_2T_1T_DeleteViewport,
    XCAST(NextViewport) Main_IDirect3DDeviceImpl_3_2T_1T_NextViewport,
    XCAST(EnumTextureFormats) Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats,
    XCAST(BeginScene) Thunk_IDirect3DDeviceImpl_3_BeginScene,
    XCAST(EndScene) Thunk_IDirect3DDeviceImpl_3_EndScene,
    XCAST(GetDirect3D) Thunk_IDirect3DDeviceImpl_3_GetDirect3D,
    XCAST(SetCurrentViewport) Main_IDirect3DDeviceImpl_3_2T_SetCurrentViewport,
    XCAST(GetCurrentViewport) Main_IDirect3DDeviceImpl_3_2T_GetCurrentViewport,
    XCAST(SetRenderTarget) Thunk_IDirect3DDeviceImpl_3_SetRenderTarget,
    XCAST(GetRenderTarget) Thunk_IDirect3DDeviceImpl_3_GetRenderTarget,
    XCAST(Begin) Main_IDirect3DDeviceImpl_3_Begin,
    XCAST(BeginIndexed) Main_IDirect3DDeviceImpl_3_BeginIndexed,
    XCAST(Vertex) Main_IDirect3DDeviceImpl_3_2T_Vertex,
    XCAST(Index) Main_IDirect3DDeviceImpl_3_2T_Index,
    XCAST(End) Main_IDirect3DDeviceImpl_3_2T_End,
    XCAST(GetRenderState) Thunk_IDirect3DDeviceImpl_3_GetRenderState,
    XCAST(SetRenderState) Thunk_IDirect3DDeviceImpl_3_SetRenderState,
    XCAST(GetLightState) Main_IDirect3DDeviceImpl_3_2T_GetLightState,
    XCAST(SetLightState) GL_IDirect3DDeviceImpl_3_2T_SetLightState,
    XCAST(SetTransform) Thunk_IDirect3DDeviceImpl_3_SetTransform,
    XCAST(GetTransform) Thunk_IDirect3DDeviceImpl_3_GetTransform,
    XCAST(MultiplyTransform) Thunk_IDirect3DDeviceImpl_3_MultiplyTransform,
    XCAST(DrawPrimitive) Thunk_IDirect3DDeviceImpl_3_DrawPrimitive,
    XCAST(DrawIndexedPrimitive) Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive,
    XCAST(SetClipStatus) Thunk_IDirect3DDeviceImpl_3_SetClipStatus,
    XCAST(GetClipStatus) Thunk_IDirect3DDeviceImpl_3_GetClipStatus,
    XCAST(DrawPrimitiveStrided) Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided,
    XCAST(DrawIndexedPrimitiveStrided) Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided,
    XCAST(DrawPrimitiveVB) Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB,
    XCAST(DrawIndexedPrimitiveVB) Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB,
    XCAST(ComputeSphereVisibility) Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility,
    XCAST(GetTexture) Thunk_IDirect3DDeviceImpl_3_GetTexture,
    XCAST(SetTexture) Thunk_IDirect3DDeviceImpl_3_SetTexture,
    XCAST(GetTextureStageState) Thunk_IDirect3DDeviceImpl_3_GetTextureStageState,
    XCAST(SetTextureStageState) Thunk_IDirect3DDeviceImpl_3_SetTextureStageState,
    XCAST(ValidateDevice) Thunk_IDirect3DDeviceImpl_3_ValidateDevice,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DDevice2.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DDevice2) VTABLE_IDirect3DDevice2 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DDeviceImpl_2_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DDeviceImpl_2_AddRef,
    XCAST(Release) Thunk_IDirect3DDeviceImpl_2_Release,
    XCAST(GetCaps) Thunk_IDirect3DDeviceImpl_2_GetCaps,
    XCAST(SwapTextureHandles) Main_IDirect3DDeviceImpl_2_1T_SwapTextureHandles,
    XCAST(GetStats) Thunk_IDirect3DDeviceImpl_2_GetStats,
    XCAST(AddViewport) Thunk_IDirect3DDeviceImpl_2_AddViewport,
    XCAST(DeleteViewport) Thunk_IDirect3DDeviceImpl_2_DeleteViewport,
    XCAST(NextViewport) Thunk_IDirect3DDeviceImpl_2_NextViewport,
    XCAST(EnumTextureFormats) GL_IDirect3DDeviceImpl_2_1T_EnumTextureFormats,
    XCAST(BeginScene) Thunk_IDirect3DDeviceImpl_2_BeginScene,
    XCAST(EndScene) Thunk_IDirect3DDeviceImpl_2_EndScene,
    XCAST(GetDirect3D) Thunk_IDirect3DDeviceImpl_2_GetDirect3D,
    XCAST(SetCurrentViewport) Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport,
    XCAST(GetCurrentViewport) Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport,
    XCAST(SetRenderTarget) Thunk_IDirect3DDeviceImpl_2_SetRenderTarget,
    XCAST(GetRenderTarget) Thunk_IDirect3DDeviceImpl_2_GetRenderTarget,
    XCAST(Begin) Main_IDirect3DDeviceImpl_2_Begin,
    XCAST(BeginIndexed) Main_IDirect3DDeviceImpl_2_BeginIndexed,
    XCAST(Vertex) Thunk_IDirect3DDeviceImpl_2_Vertex,
    XCAST(Index) Thunk_IDirect3DDeviceImpl_2_Index,
    XCAST(End) Thunk_IDirect3DDeviceImpl_2_End,
    XCAST(GetRenderState) Thunk_IDirect3DDeviceImpl_2_GetRenderState,
    XCAST(SetRenderState) Thunk_IDirect3DDeviceImpl_2_SetRenderState,
    XCAST(GetLightState) Thunk_IDirect3DDeviceImpl_2_GetLightState,
    XCAST(SetLightState) Thunk_IDirect3DDeviceImpl_2_SetLightState,
    XCAST(SetTransform) Thunk_IDirect3DDeviceImpl_2_SetTransform,
    XCAST(GetTransform) Thunk_IDirect3DDeviceImpl_2_GetTransform,
    XCAST(MultiplyTransform) Thunk_IDirect3DDeviceImpl_2_MultiplyTransform,
    XCAST(DrawPrimitive) GL_IDirect3DDeviceImpl_2_DrawPrimitive,
    XCAST(DrawIndexedPrimitive) GL_IDirect3DDeviceImpl_2_DrawIndexedPrimitive,
    XCAST(SetClipStatus) Thunk_IDirect3DDeviceImpl_2_SetClipStatus,
    XCAST(GetClipStatus) Thunk_IDirect3DDeviceImpl_2_GetClipStatus,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DDevice.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DDevice) VTABLE_IDirect3DDevice =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DDeviceImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DDeviceImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DDeviceImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DDeviceImpl_1_Initialize,
    XCAST(GetCaps) Thunk_IDirect3DDeviceImpl_1_GetCaps,
    XCAST(SwapTextureHandles) Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles,
    XCAST(CreateExecuteBuffer) GL_IDirect3DDeviceImpl_1_CreateExecuteBuffer,
    XCAST(GetStats) Thunk_IDirect3DDeviceImpl_1_GetStats,
    XCAST(Execute) Main_IDirect3DDeviceImpl_1_Execute,
    XCAST(AddViewport) Thunk_IDirect3DDeviceImpl_1_AddViewport,
    XCAST(DeleteViewport) Thunk_IDirect3DDeviceImpl_1_DeleteViewport,
    XCAST(NextViewport) Thunk_IDirect3DDeviceImpl_1_NextViewport,
    XCAST(Pick) Main_IDirect3DDeviceImpl_1_Pick,
    XCAST(GetPickRecords) Main_IDirect3DDeviceImpl_1_GetPickRecords,
    XCAST(EnumTextureFormats) Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats,
    XCAST(CreateMatrix) Main_IDirect3DDeviceImpl_1_CreateMatrix,
    XCAST(SetMatrix) Main_IDirect3DDeviceImpl_1_SetMatrix,
    XCAST(GetMatrix) Main_IDirect3DDeviceImpl_1_GetMatrix,
    XCAST(DeleteMatrix) Main_IDirect3DDeviceImpl_1_DeleteMatrix,
    XCAST(BeginScene) Thunk_IDirect3DDeviceImpl_1_BeginScene,
    XCAST(EndScene) Thunk_IDirect3DDeviceImpl_1_EndScene,
    XCAST(GetDirect3D) Thunk_IDirect3DDeviceImpl_1_GetDirect3D,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

static HRESULT d3ddevice_clear(IDirect3DDeviceImpl *This,
			       DWORD dwCount,
			       LPD3DRECT lpRects,
			       DWORD dwFlags,
			       DWORD dwColor,
			       D3DVALUE dvZ,
			       DWORD dwStencil)
{
    GLboolean ztest;
    GLfloat old_z_clear_value;
    GLbitfield bitfield = 0;
    GLint old_stencil_clear_value;
    GLfloat old_color_clear_value[4];
    
    TRACE("(%p)->(%08lx,%p,%08lx,%08lx,%f,%08lx)\n", This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
    if (TRACE_ON(ddraw)) {
	if (dwCount > 0) {
	    int i;
	    TRACE(" rectangles : \n");
	    for (i = 0; i < dwCount; i++) {
	        TRACE("  - %ld x %ld     %ld x %ld\n", lpRects[i].u1.x1, lpRects[i].u2.y1, lpRects[i].u3.x2, lpRects[i].u4.y2);
	    }
	}
    }

    if (dwCount > 1) {
        WARN("  Warning, this function only for now clears the whole screen...\n");
    }

    /* Clears the screen */
    ENTER_GL();
    if (dwFlags & D3DCLEAR_ZBUFFER) {
	bitfield |= GL_DEPTH_BUFFER_BIT;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &ztest);
	glDepthMask(GL_TRUE); /* Enables Z writing to be sure to delete also the Z buffer */
	glGetFloatv(GL_DEPTH_CLEAR_VALUE, &old_z_clear_value);
	glClearDepth(dvZ);
	TRACE(" depth value : %f\n", dvZ);
    }
    if (dwFlags & D3DCLEAR_STENCIL) {
        bitfield |= GL_STENCIL_BUFFER_BIT;
	glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &old_stencil_clear_value);
	glClearStencil(dwStencil);
	TRACE(" stencil value : %ld\n", dwStencil);
    }    
    if (dwFlags & D3DCLEAR_TARGET) {
        bitfield |= GL_COLOR_BUFFER_BIT;
	glGetFloatv(GL_COLOR_CLEAR_VALUE, old_color_clear_value);
	glClearColor(((dwColor >> 16) & 0xFF) / 255.0,
		     ((dwColor >>  8) & 0xFF) / 255.0,
		     ((dwColor >>  0) & 0xFF) / 255.0,
		     ((dwColor >> 24) & 0xFF) / 255.0);
	TRACE(" color value (ARGB) : %08lx\n", dwColor);
    }
    
    glClear(bitfield);
    
    if (dwFlags & D3DCLEAR_ZBUFFER) {
        glDepthMask(ztest);
	glClearDepth(old_z_clear_value);
    }
     if (dwFlags & D3DCLEAR_STENCIL) {
        bitfield |= GL_STENCIL_BUFFER_BIT;
	glClearStencil(old_stencil_clear_value);
    }    
    if (dwFlags & D3DCLEAR_TARGET) {
        bitfield |= GL_COLOR_BUFFER_BIT;
	glClearColor(old_color_clear_value[0],
		     old_color_clear_value[1],
		     old_color_clear_value[2],
		     old_color_clear_value[3]);
    }
    
    LEAVE_GL();
    
    return DD_OK;
}

HRESULT
d3ddevice_blt(IDirectDrawSurfaceImpl *This, LPRECT rdst,
	      LPDIRECTDRAWSURFACE7 src, LPRECT rsrc,
	      DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    if (dwFlags & DDBLT_COLORFILL) {
        /* This is easy to handle for the D3D Device... */
        DWORD color = lpbltfx->u5.dwFillColor;
        TRACE(" executing D3D Device override.\n");
	d3ddevice_clear(This->d3ddevice, 0, NULL, D3DCLEAR_TARGET, color, 0.0, 0x00000000);
	return DD_OK;
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT
d3ddevice_bltfast(IDirectDrawSurfaceImpl *This, DWORD dstx,
		  DWORD dsty, LPDIRECTDRAWSURFACE7 src,
		  LPRECT rsrc, DWORD trans)
{
     return DDERR_INVALIDPARAMS;
}

void
d3ddevice_set_ortho(IDirect3DDeviceImpl *This)
{
    GLfloat height, width;
    GLfloat trans_mat[16];
    
    width = This->surface->surface_desc.dwWidth;
    height = This->surface->surface_desc.dwHeight;
    
    /* The X axis is straighforward.. For the Y axis, we need to convert 'D3D' screen coordinates
       to OpenGL screen coordinates (ie the upper left corner is not the same).
       For Z, the mystery is what should it be mapped to ? Ie should the resulting range be between
       -1.0 and 1.0 (as the X and Y coordinates) or between 0.0 and 1.0 ? */
    trans_mat[ 0] = 2.0 / width;  trans_mat[ 4] = 0.0;  trans_mat[ 8] = 0.0; trans_mat[12] = -1.0;
    trans_mat[ 1] = 0.0; trans_mat[ 5] = -2.0 / height; trans_mat[ 9] = 0.0; trans_mat[13] =  1.0;
    trans_mat[ 2] = 0.0; trans_mat[ 6] = 0.0; trans_mat[10] = 1.0;           trans_mat[14] = -1.0;
    trans_mat[ 3] = 0.0; trans_mat[ 7] = 0.0; trans_mat[11] = 0.0;           trans_mat[15] =  1.0;
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(trans_mat);
}

void
d3ddevice_set_matrices(IDirect3DDeviceImpl *This, DWORD matrices,
		       D3DMATRIX *world_mat, D3DMATRIX *view_mat, D3DMATRIX *proj_mat)
{
    if ((matrices & (VIEWMAT_CHANGED|WORLDMAT_CHANGED)) != 0) {
        glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float *) view_mat);
	glMultMatrixf((float *) world_mat);
    }
    if ((matrices & PROJMAT_CHANGED) != 0) {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float *) proj_mat);
    }
}

void
d3ddevice_matrices_updated(IDirect3DDeviceImpl *This, DWORD matrices)
{
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    if (glThis->transform_state == GL_TRANSFORM_NORMAL) {
        /* This will force an update of the transform state at the next drawing. */
        glThis->transform_state = GL_TRANSFORM_NONE;
    }
}

/* TODO for both these functions :
    - change / restore OpenGL parameters for pictures transfers in case they are ever modified
      by other OpenGL code in D3D
    - handle the case where no 'Begin / EndScene' was done between two locks
    - handle the rectangles in the unlock too
    - handle pitch correctly...
*/
static void d3ddevice_lock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags)
{
    /* First, check if we need to do anything */
    if ((This->lastlocktype & DDLOCK_WRITEONLY) == 0) {
        GLenum buffer_type;
	GLenum prev_read;
	RECT loc_rect;

	ENTER_GL();

	glGetIntegerv(GL_READ_BUFFER, &prev_read);
	glFlush();
	
        WARN(" application does a lock on a 3D surface - expect slow downs.\n");
	if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
	    /* Application wants to lock the front buffer */
	    glReadBuffer(GL_FRONT);
	} else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
	    /* Application wants to lock the back buffer */
	    glReadBuffer(GL_BACK);
	} else {
	    WARN(" do not support 3D surface locking for this surface type - trying to use default buffer.\n");
	}

	if (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	    buffer_type = GL_UNSIGNED_SHORT_5_6_5;
	} else {
	    WARN(" unsupported pixel format.\n");
	    LEAVE_GL();
	    return;
	}
	if (pRect == NULL) {
	    loc_rect.top = 0;
	    loc_rect.left = 0;
	    loc_rect.bottom = This->surface_desc.dwHeight;
	    loc_rect.right = This->surface_desc.dwWidth;
	} else {
	    loc_rect = *pRect;
	}
	glReadPixels(loc_rect.left, loc_rect.top, loc_rect.right, loc_rect.bottom,
		     GL_RGB, buffer_type, ((char *)This->surface_desc.lpSurface
					   + loc_rect.top * This->surface_desc.u1.lPitch
					   + loc_rect.left * GET_BPP(This->surface_desc)));
	glReadBuffer(prev_read);
	LEAVE_GL();
    }
}

static void d3ddevice_unlock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect)
{
    /* First, check if we need to do anything */
    if ((This->lastlocktype & DDLOCK_READONLY) == 0) {
        GLenum buffer_type;
	GLenum prev_draw;

	ENTER_GL();

	glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);

        WARN(" application does an unlock on a 3D surface - expect slow downs.\n");
	if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
	    /* Application wants to lock the front buffer */
	    glDrawBuffer(GL_FRONT);
	} else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
	    /* Application wants to lock the back buffer */
	    glDrawBuffer(GL_BACK);
	} else {
	    WARN(" do not support 3D surface unlocking for this surface type - trying to use default buffer.\n");
	}

	if (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	    buffer_type = GL_UNSIGNED_SHORT_5_6_5;
	} else {
	    WARN(" unsupported pixel format.\n");
	    LEAVE_GL();
	    return;
	}
	glRasterPos2f(0.0, 0.0);
	glDrawPixels(This->surface_desc.dwWidth, This->surface_desc.dwHeight, 
		     GL_RGB, buffer_type, This->surface_desc.lpSurface);
	glDrawBuffer(prev_draw);

	LEAVE_GL();
   }
}

HRESULT
d3ddevice_create(IDirect3DDeviceImpl **obj, IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *surface)
{
    IDirect3DDeviceImpl *object;
    IDirect3DDeviceGLImpl *gl_object;
    IDirectDrawSurfaceImpl *surf;
    HDC device_context;
    XVisualInfo *vis;
    int num;
    XVisualInfo template;
    GLenum buffer = GL_FRONT;
    int light;
    GLint max_clipping_planes = 0;
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDeviceGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    gl_object = (IDirect3DDeviceGLImpl *) object;
    
    object->ref = 1;
    object->d3d = d3d;
    object->surface = surface;
    object->set_context = set_context;
    object->clear = d3ddevice_clear;
    object->set_matrices = d3ddevice_set_matrices;
    object->matrices_updated = d3ddevice_matrices_updated;

    TRACE(" creating OpenGL device for surface = %p, d3d = %p\n", surface, d3d);

    device_context = GetDC(surface->ddraw_owner->window);
    gl_object->display = get_display(device_context);
    gl_object->drawable = get_drawable(device_context);
    ReleaseDC(surface->ddraw_owner->window,device_context);

    ENTER_GL();
    template.visualid = (VisualID)GetPropA( GetDesktopWindow(), "__wine_x11_visual_id" );
    vis = XGetVisualInfo(gl_object->display, VisualIDMask, &template, &num);
    if (vis == NULL) {
        HeapFree(GetProcessHeap(), 0, object);
        ERR("No visual found !\n");
	LEAVE_GL();
	return DDERR_INVALIDPARAMS;
    } else {
        TRACE(" visual found\n");
    }

    gl_object->gl_context = glXCreateContext(gl_object->display, vis,
					     NULL, GL_TRUE);

    if (gl_object->gl_context == NULL) {
        HeapFree(GetProcessHeap(), 0, object);
        ERR("Error in context creation !\n");
	LEAVE_GL();
	return DDERR_INVALIDPARAMS;
    } else {
        TRACE(" context created (%p)\n", gl_object->gl_context);
    }
    
    /* Look for the front buffer and override its surface's Flip method (if in double buffering) */
    for (surf = surface; surf != NULL; surf = surf->surface_owner) {
        if ((surf->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER)) == (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER)) {
            surf->aux_ctx  = (LPVOID) gl_object->display;
            surf->aux_data = (LPVOID) gl_object->drawable;
            surf->aux_flip = opengl_flip;
	    buffer =  GL_BACK;
            break;
        }
    }
    /* We are not doing any double buffering.. Then force OpenGL to draw on the front buffer */
    if (surf == NULL) {
        TRACE(" no double buffering : drawing on the front buffer\n");
        buffer = GL_FRONT;
    }
    
    for (surf = surface; surf->prev_attached != NULL; surf = surf->prev_attached) ;
    for (; surf != NULL; surf = surf->next_attached) {
        if (((surf->surface_desc.ddsCaps.dwCaps & (DDSCAPS_3DDEVICE)) == (DDSCAPS_3DDEVICE)) &&
	    ((surf->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER)) != (DDSCAPS_ZBUFFER))) {
	    /* Override the Lock / Unlock function for all these surfaces */
	    surf->lock_update = d3ddevice_lock_update;
	    surf->unlock_update = d3ddevice_unlock_update;
	    /* And install also the blt / bltfast overrides */
	    surf->aux_blt = d3ddevice_blt;
	    surf->aux_bltfast = d3ddevice_bltfast;
	}
	surf->d3ddevice = object;
    }

    /* Set the various light parameters */
    for (light = 0; light < MAX_LIGHTS; light++) {
        /* Only set the fields that are not zero-created */
        object->light_parameters[light].dltType = D3DLIGHT_DIRECTIONAL;
	object->light_parameters[light].dcvDiffuse.u1.r = 1.0;
	object->light_parameters[light].dcvDiffuse.u2.g = 1.0;
	object->light_parameters[light].dcvDiffuse.u3.b = 1.0;
	object->light_parameters[light].dvDirection.u3.z = 1.0;
    }
    
    /* Allocate memory for the matrices */
    object->world_mat = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    object->view_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    object->proj_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    memcpy(object->world_mat, id_mat, 16 * sizeof(float));
    memcpy(object->view_mat , id_mat, 16 * sizeof(float));
    memcpy(object->proj_mat , id_mat, 16 * sizeof(float));

    /* allocate the clipping planes */
    glGetIntegerv(GL_MAX_CLIP_PLANES,&max_clipping_planes);
    if (max_clipping_planes>32) {
	object->max_clipping_planes=32;
    } else {
	object->max_clipping_planes = max_clipping_planes;
    }
    TRACE(" capable of %d clipping planes\n", (int)object->max_clipping_planes );
    object->clipping_planes = (d3d7clippingplane*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->max_clipping_planes * sizeof(d3d7clippingplane));

    /* Initialisation */
    TRACE(" setting current context\n");
    LEAVE_GL();
    object->set_context(object);
    ENTER_GL();
    TRACE(" current context set\n");

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawBuffer(buffer);
    glReadBuffer(buffer);
    /* glDisable(GL_DEPTH_TEST); Need here to check for the presence of a ZBuffer and to reenable it when the ZBuffer is attached */
    LEAVE_GL();

    /* fill_device_capabilities(d3d->ddraw); */    
    
    ICOM_INIT_INTERFACE(object, IDirect3DDevice,  VTABLE_IDirect3DDevice);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice2, VTABLE_IDirect3DDevice2);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice3, VTABLE_IDirect3DDevice3);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice7, VTABLE_IDirect3DDevice7);

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);

    /* And finally warn D3D that this device is now present */
    object->d3d->added_device(object->d3d, object);

    /* FIXME: Should handle other versions than just 7 */
    InitDefaultStateBlock(&object->state_block, 7);
    /* Apply default render state values */
    apply_render_state(object, &object->state_block);
    /* FIXME: do something similar for ligh_state and texture_stage_state */

    return DD_OK;
}
