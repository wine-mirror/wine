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

/* Define this variable if you have an unpatched Mesa 3.0 (patches are available
   on Mesa's home page) or version 3.1b.

   Version 3.1b2 should correct this bug */
#undef HAVE_BUGGY_MESAGL

#ifndef HAVE_GLEXT_PROTOTYPES
/* This is for non-OpenGL ABI compliant glext.h headers :-) */
typedef void (* PFNGLCOLORTABLEEXTPROC) (GLenum target, GLenum internalFormat,
					 GLsizei width, GLenum format, GLenum type,
					 const GLvoid *table);
#endif

static const float id_mat[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
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
      D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_STIPPLE | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ZTEST;
    pc->dwZCmpCaps = 0xFFFFFFFF; /* All Z test can be done */
    pc->dwSrcBlendCaps  = 0xFFFFFFFF; /* FIXME: need REAL values */
    pc->dwDestBlendCaps = 0xFFFFFFFF; /* FIXME: need REAL values */
    pc->dwAlphaCmpCaps  = 0xFFFFFFFF; /* FIXME: need REAL values */
    pc->dwShadeCaps = 0xFFFFFFFF;     /* FIXME: need REAL values */
    pc->dwTextureCaps = D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_BORDER | D3DPTEXTURECAPS_PERSPECTIVE |
      D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_TRANSPARENCY;
    pc->dwTextureFilterCaps = D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_LINEARMIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST |
      D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_MIPNEAREST | D3DPTFILTERCAPS_NEAREST;
    pc->dwTextureBlendCaps = 0xFFFFFFFF; /* FIXME: need REAL values */
    pc->dwTextureAddressCaps = D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_WRAP;
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
      D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY;
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
    d1->dwDeviceZBufferBitDepth = DDBD_16;
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
    d1->dwStencilCaps = 0; /* TODO add proper caps according to what OpenGL can do */
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
    D3DDEVICEDESC d1, d2;

    fill_opengl_caps(&d1);
    d2 = d1;

    TRACE(" enumerating OpenGL D3DDevice interface (IID %s).\n", debugstr_guid(&IID_D3DDEVICE_OpenGL));
    return cb((LPIID) &IID_D3DDEVICE_OpenGL, "WINE Direct3DX using OpenGL", "direct3d", &d1, &d2, context);
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
	/* Release texture associated with the device */ 
	if (This->current_texture[0] != NULL)
	    IDirect3DTexture2_Release(ICOM_INTERFACE(This->current_texture[0], IDirect3DTexture2));
	    	  
	ENTER_GL();
	glXDestroyContext(glThis->display, glThis->gl_context);
	LEAVE_GL();

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
    pformat->u2.dwRBitMask =         0xFF000000;
    pformat->u3.dwGBitMask =         0x00FF0000;
    pformat->u4.dwBBitMask =        0x0000FF00;
    pformat->u5.dwRGBAlphaBitMask = 0x000000FF;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB unpacked (24)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 24;
    pformat->u2.dwRBitMask =  0x00FF0000;
    pformat->u3.dwGBitMask =  0x0000FF00;
    pformat->u4.dwBBitMask = 0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

#ifndef HAVE_BUGGY_MESAGL
    /* The packed texture format are buggy in Mesa. The bug was reported and corrected,
       so that future version will work great. */
    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =  0x0000F800;
    pformat->u3.dwGBitMask =  0x000007E0;
    pformat->u4.dwBBitMask = 0x0000001F;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_5_5_5_1 (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =         0x0000F800;
    pformat->u3.dwGBitMask =         0x000007C0;
    pformat->u4.dwBBitMask =        0x0000003E;
    pformat->u5.dwRGBAlphaBitMask = 0x00000001;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGBA packed GL_UNSIGNED_SHORT_4_4_4_4 (16)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =         0x0000F000;
    pformat->u3.dwGBitMask =         0x00000F00;
    pformat->u4.dwBBitMask =        0x000000F0;
    pformat->u5.dwRGBAlphaBitMask = 0x0000000F;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_BYTE_3_3_2 (8)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 8;
    pformat->u2.dwRBitMask =        0x000000E0;
    pformat->u3.dwGBitMask =        0x0000001C;
    pformat->u4.dwBBitMask =        0x00000003;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;
#endif

    TRACE("Enumerating GL_ARGB (no direct OpenGL equivalent - conversion needed)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask =         0x00007C00;
    pformat->u3.dwGBitMask =         0x000003E0;
    pformat->u4.dwBBitMask =         0x0000001F;
    pformat->u5.dwRGBAlphaBitMask =  0x00008000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating Paletted (8)\n");
    pformat->dwFlags = DDPF_PALETTEINDEXED8;
    pformat->u1.dwRGBBitCount = 8;
    pformat->u2.dwRBitMask =  0x00000000;
    pformat->u3.dwGBitMask =  0x00000000;
    pformat->u4.dwBBitMask = 0x00000000;
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
    DWORD dwSize;
    D3DDEVICEDESC desc;
  
    if ((lpD3DDFS->dwFlags & D3DFDS_COLORMODEL) &&
	(lpD3DDFS->dcmColorModel != D3DCOLOR_RGB)) {
        TRACE(" trying to request a non-RGB D3D color model. Not supported.\n");
	return DDERR_INVALIDPARAMS; /* No real idea what to return here :-) */
    }
    if (lpD3DDFS->dwFlags & D3DFDS_GUID) {
        TRACE(" trying to match guid %s.\n", debugstr_guid(&(lpD3DDFS->guid)));
	if ((IsEqualGUID( &IID_D3DDEVICE_OpenGL, &(lpD3DDFS->guid)) == 0) &&
	    (IsEqualGUID(&IID_IDirect3DHALDevice, &(lpD3DDFS->guid)) == 0)) {
	    TRACE(" no match for this GUID.\n");
	    return DDERR_INVALIDPARAMS;
	}
    }

    /* Now return our own GUID */
    lplpD3DDevice->guid = IID_D3DDEVICE_OpenGL;
    fill_opengl_caps(&desc);
    dwSize = lplpD3DDevice->ddHwDesc.dwSize;
    memset(&(lplpD3DDevice->ddHwDesc), 0, dwSize);
    memcpy(&(lplpD3DDevice->ddHwDesc), &desc, (dwSize <= desc.dwSize ? dwSize : desc.dwSize));
    dwSize = lplpD3DDevice->ddSwDesc.dwSize;
    memset(&(lplpD3DDevice->ddSwDesc), 0, dwSize);
    memcpy(&(lplpD3DDevice->ddSwDesc), &desc, (dwSize <= desc.dwSize ? dwSize : desc.dwSize));
    
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
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    TRACE("(%p/%p)->(%08x,%08lx)\n", This, iface, dwRenderStateType, dwRenderState);

    /* Call the render state functions */
    set_render_state(dwRenderStateType, dwRenderState, &(glThis->render_state));

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

	case D3DLIGHTSTATE_AMBIENT: {   /* 2 */
	    float light[4];

	    light[0] = ((dwLightState >> 16) & 0xFF) / 255.0;
	    light[1] = ((dwLightState >>  8) & 0xFF) / 255.0;
	    light[2] = ((dwLightState >>  0) & 0xFF) / 255.0;
	    light[3] = 1.0;
	    ENTER_GL();
	    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float *) light);
	    LEAVE_GL();
	} break;

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

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_2T_SetTransform(LPDIRECT3DDEVICE7 iface,
                                            D3DTRANSFORMSTATETYPE dtstTransformStateType,
                                            LPD3DMATRIX lpD3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;

    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dtstTransformStateType, lpD3DMatrix);

    ENTER_GL();

    /* Using a trial and failure approach, I found that the order of
       Direct3D transformations that works best is :

       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord

       As OpenGL uses only two matrices, I combined PROJECTION and VIEW into
       OpenGL's GL_PROJECTION matrix and the WORLD into GL_MODELVIEW.

       If anyone has a good explanation of the three different matrices in
       the SDK online documentation, feel free to point it to me. For example,
       which matrices transform lights ? In OpenGL only the PROJECTION matrix
       transform the lights, not the MODELVIEW. Using the matrix names, I
       supposed that PROJECTION and VIEW (all 'camera' related names) do
       transform lights, but WORLD do not. It may be wrong though... */

    /* After reading through both OpenGL and Direct3D documentations, I
       thought that D3D matrices were written in 'line major mode' transposed
       from OpenGL's 'column major mode'. But I found out that a simple memcpy
       works fine to transfer one matrix format to the other (it did not work
       when transposing)....

       So :
         1) are the documentations wrong
	 2) does the matrix work even if they are not read correctly
	 3) is Mesa's implementation of OpenGL not compliant regarding Matrix
            loading using glLoadMatrix ?

       Anyway, I always use 'conv_mat' to transfer the matrices from one format
       to the other so that if I ever find out that I need to transpose them, I
       will able to do it quickly, only by changing the macro conv_mat. */

    switch (dtstTransformStateType) {
        case D3DTRANSFORMSTATE_WORLD: {
	    TRACE(" D3DTRANSFORMSTATE_WORLD :\n");
	    conv_mat(lpD3DMatrix, glThis->world_mat);
	    glMatrixMode(GL_MODELVIEW);
	    glLoadMatrixf((float *) glThis->view_mat);
	    glMultMatrixf((float *) glThis->world_mat);
	} break;

	case D3DTRANSFORMSTATE_VIEW: {
	    TRACE(" D3DTRANSFORMSTATE_VIEW :\n");
	    conv_mat(lpD3DMatrix, glThis->view_mat);
	    glMatrixMode(GL_MODELVIEW);
	    glLoadMatrixf((float *) glThis->view_mat);
	    glMultMatrixf((float *) glThis->world_mat);
	} break;

	case D3DTRANSFORMSTATE_PROJECTION: {
	    TRACE(" D3DTRANSFORMSTATE_PROJECTION :\n");
	    conv_mat(lpD3DMatrix, glThis->proj_mat);
	    glMatrixMode(GL_PROJECTION);
	    glLoadMatrixf((float *) glThis->proj_mat);
	} break;

	default:
	    ERR("Unknown trasnform type %08x !!!\n", dtstTransformStateType);
	    break;
    }
    LEAVE_GL();

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

static void draw_primitive_handle_GL_state(IDirect3DDeviceGLImpl *glThis,
					   BOOLEAN vertex_transformed,
					   BOOLEAN vertex_lit) {
    /* Puts GL in the correct lighting / transformation mode */
    if ((glThis->last_vertices_transformed == TRUE) && (vertex_transformed == FALSE)) {
        /* Need to put the correct transformation again if we go from Transformed
	   vertices to non-transformed ones.
	*/
        glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float *) glThis->view_mat);
	glMultMatrixf((float *) glThis->world_mat);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float *) glThis->proj_mat);
    } else if ((glThis->last_vertices_transformed == FALSE) && (vertex_transformed == TRUE)) {
        GLdouble height, width, minZ, maxZ;
      
        glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	if (glThis->parent.current_viewport == NULL) {
	    ERR("No current viewport !\n");
	    /* Using standard values */
	    height = 640.0;
	    width = 480.0;
	    minZ = -10.0;
	    maxZ = 10.0;
	} else {
	    if (glThis->parent.current_viewport->use_vp2 == 1) {
	        height = (GLdouble) glThis->parent.current_viewport->viewports.vp2.dwHeight;
		width  = (GLdouble) glThis->parent.current_viewport->viewports.vp2.dwWidth;
		minZ   = (GLdouble) glThis->parent.current_viewport->viewports.vp2.dvMinZ;
		maxZ   = (GLdouble) glThis->parent.current_viewport->viewports.vp2.dvMaxZ;
	    } else {
	        height = (GLdouble) glThis->parent.current_viewport->viewports.vp1.dwHeight;
		width  = (GLdouble) glThis->parent.current_viewport->viewports.vp1.dwWidth;
		minZ   = (GLdouble) glThis->parent.current_viewport->viewports.vp1.dvMinZ;
		maxZ   = (GLdouble) glThis->parent.current_viewport->viewports.vp1.dvMaxZ;
	    }
	}
	glOrtho(0.0, width, height, 0.0, -minZ, -maxZ);
    }
    
    if ((glThis->last_vertices_lit == TRUE) && (vertex_lit == FALSE)) {
        glEnable(GL_LIGHTING);
    } else if ((glThis->last_vertices_lit == TRUE) && (vertex_lit == TRUE)) {
        glDisable(GL_LIGHTING);
    }

    /* And save the current state */
    glThis->last_vertices_transformed = vertex_transformed;
    glThis->last_vertices_lit = vertex_lit;
}


inline static void draw_primitive(IDirect3DDeviceGLImpl *glThis, DWORD maxvert, WORD *index,
				  D3DVERTEXTYPE d3dvt, D3DPRIMITIVETYPE d3dpt, void *lpvertex)
{
    DWORD vx_index;
  
    draw_primitive_handle_GL_state(glThis, d3dvt == D3DVT_TLVERTEX, d3dvt != D3DVT_VERTEX);
    draw_primitive_start_GL(d3dpt);
    
    /* Draw the primitives */
    for (vx_index = 0; vx_index < maxvert; vx_index++) {
        switch (d3dvt) {
	    case D3DVT_VERTEX: {
	        D3DVERTEX *vx = ((D3DVERTEX *) lpvertex) + (index == 0 ? vx_index : index[vx_index]);

		glNormal3f(vx->u4.nx, vx->u5.ny, vx->u6.nz);
		glVertex3f(vx->u1.x, vx->u2.y, vx->u3.z);
		TRACE("   V: %f %f %f\n", vx->u1.x, vx->u2.y, vx->u3.z);
	    } break;

	    case D3DVT_LVERTEX: {
	        D3DLVERTEX *vx = ((D3DLVERTEX *) lpvertex) + (index == 0 ? vx_index : index[vx_index]);
		DWORD col = vx->u4.color;

		glColor3f(((col >> 16) & 0xFF) / 255.0,
			  ((col >>  8) & 0xFF) / 255.0,
			  ((col >>  0) & 0xFF) / 255.0);
		glVertex3f(vx->u1.x, vx->u2.y, vx->u3.z);
		TRACE("  LV: %f %f %f (%02lx %02lx %02lx)\n",
		      vx->u1.x, vx->u2.y, vx->u3.z,
		      ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF));
	    } break;

	    case D3DVT_TLVERTEX: {
	        D3DTLVERTEX *vx = ((D3DTLVERTEX *) lpvertex) + (index == 0 ? vx_index : index[vx_index]);
		DWORD col = vx->u5.color;

		glColor4ub((col >> 24) & 0xFF,
			   (col >> 16) & 0xFF,
			   (col >>  8) & 0xFF,
			   (col >>  0) & 0xFF);
		glTexCoord2f(vx->u7.tu, vx->u8.tv);
		if (vx->u4.rhw < 0.01)
		    glVertex3f(vx->u1.sx,
			       vx->u2.sy,
			       vx->u3.sz);
		else
		    glVertex4f(vx->u1.sx / vx->u4.rhw,
			       vx->u2.sy / vx->u4.rhw,
			       vx->u3.sz / vx->u4.rhw,
			       1.0 / vx->u4.rhw);
		TRACE(" TLV: %f %f %f (%02lx %02lx %02lx) (%f %f) (%f)\n",
		      vx->u1.sx, vx->u2.sy, vx->u3.sz,
		      ((col >> 16) & 0xFF), ((col >>  8) & 0xFF), ((col >>  0) & 0xFF),
		      vx->u7.tu, vx->u8.tv, vx->u4.rhw);
	    } break;

	    default:
	        FIXME("Unhandled vertex type\n");
	        break;
        }
    }

    glEnd();
    TRACE("End\n");
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

    ENTER_GL();
    draw_primitive((IDirect3DDeviceGLImpl *) This, dwVertexCount, NULL, d3dvtVertexType, d3dptPrimitiveType, lpvVertices);
    LEAVE_GL();
		   
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

    ENTER_GL();
    draw_primitive((IDirect3DDeviceGLImpl *) This, dwIndexCount, dwIndices, d3dvtVertexType, d3dptPrimitiveType, lpvVertices);
    LEAVE_GL();
    
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

DWORD get_flexible_vertex_size(DWORD d3dvtVertexType)
{
    DWORD size = 0;
    
    if (d3dvtVertexType & D3DFVF_NORMAL) size += 3 * sizeof(D3DVALUE);
    if (d3dvtVertexType & D3DFVF_DIFFUSE) size += sizeof(DWORD);
    if (d3dvtVertexType & D3DFVF_SPECULAR) size += sizeof(DWORD);
    switch (d3dvtVertexType & D3DFVF_POSITION_MASK) {
        case D3DFVF_XYZ: size += 3 * sizeof(D3DVALUE); break;
        case D3DFVF_XYZRHW: size += 4 * sizeof(D3DVALUE); break;
	default: TRACE(" matrix weighting not handled yet...\n");
    }
    size += 2 * sizeof(D3DVALUE) * ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);

    return size;
}

void dump_flexible_vertex(DWORD d3dvtVertexType)
{
    static const flag_info flags[] = {
        FE(D3DFVF_NORMAL),
	FE(D3DFVF_RESERVED1),
	FE(D3DFVF_DIFFUSE),
	FE(D3DFVF_SPECULAR)
    };
    if (d3dvtVertexType & D3DFVF_RESERVED0) DPRINTF("D3DFVF_RESERVED0 ");
    switch (d3dvtVertexType & D3DFVF_POSITION_MASK) {
#define GEN_CASE(a) case a: DPRINTF(#a " "); break
        GEN_CASE(D3DFVF_XYZ);
	GEN_CASE(D3DFVF_XYZRHW);
	GEN_CASE(D3DFVF_XYZB1);
	GEN_CASE(D3DFVF_XYZB2);
	GEN_CASE(D3DFVF_XYZB3);
	GEN_CASE(D3DFVF_XYZB4);
	GEN_CASE(D3DFVF_XYZB5);
    }
    DDRAW_dump_flags_(d3dvtVertexType, flags, sizeof(flags)/sizeof(flags[0]), FALSE);
    switch (d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) {
        GEN_CASE(D3DFVF_TEX0);
	GEN_CASE(D3DFVF_TEX1);
	GEN_CASE(D3DFVF_TEX2);
	GEN_CASE(D3DFVF_TEX3);
	GEN_CASE(D3DFVF_TEX4);
	GEN_CASE(D3DFVF_TEX5);
	GEN_CASE(D3DFVF_TEX6);
	GEN_CASE(D3DFVF_TEX7);
	GEN_CASE(D3DFVF_TEX8);
    }
#undef GEN_CASE
    DPRINTF("\n");
}

/* Some types used by the fast paths... */
typedef struct {
    float x, y, z;
    float nx, ny, nz;
    float tu1, tv1;
} D3DFVF_VERTEX_1;

typedef struct {
    float x, y, z, rhw;
    DWORD diffuse;
    DWORD specular;
    float tu1, tv1;
} D3DFVF_TLVERTEX_1;

static void draw_primitive_7(IDirect3DDeviceImpl *This,
			     D3DPRIMITIVETYPE d3dptPrimitiveType,
			     DWORD d3dvtVertexType,
			     LPVOID lpvVertices,
			     DWORD dwVertexCount,
			     LPWORD dwIndices,
			     DWORD dwIndexCount,
			     DWORD dwFlags)
{
    IDirect3DDeviceGLImpl* glThis = (IDirect3DDeviceGLImpl*) This;
    if (TRACE_ON(ddraw)) {
        TRACE(" Vertex format : "); dump_flexible_vertex(d3dvtVertexType);
    }

    ENTER_GL();
    draw_primitive_handle_GL_state(glThis,
				   (d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ,
				   (d3dvtVertexType & D3DFVF_NORMAL) == 0);
    draw_primitive_start_GL(d3dptPrimitiveType);

    /* Some fast paths first before the generic case.... */
    if (d3dvtVertexType == D3DFVF_VERTEX) {
        D3DFVF_VERTEX_1 *vertices = (D3DFVF_VERTEX_1 *) lpvVertices;
	int index;
	
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];
	  
	    glNormal3f(vertices[i].nx, vertices[i].ny, vertices[i].nz);
	    glTexCoord2f(vertices[i].tu1, vertices[i].tv1);
	    glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
	    TRACE(" %f %f %f / %f %f %f (%f %f)\n",
		  vertices[i].x, vertices[i].y, vertices[i].z,
		  vertices[i].nx, vertices[i].ny, vertices[i].nz,
		  vertices[i].tu1, vertices[i].tv1);
	}
    } else if (d3dvtVertexType == D3DFVF_TLVERTEX) {
        D3DFVF_TLVERTEX_1 *vertices = (D3DFVF_TLVERTEX_1 *) lpvVertices;
	int index;
	
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];
	    
	    glColor4ub((vertices[i].diffuse >> 24) & 0xFF,
		       (vertices[i].diffuse >> 16) & 0xFF,
		       (vertices[i].diffuse >>  8) & 0xFF,
		       (vertices[i].diffuse >>  0) & 0xFF);
	    /* Todo : handle specular... */
	    glTexCoord2f(vertices[i].tu1, vertices[i].tv1);
	    if (vertices[i].rhw < 0.00001)
	        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
	    else
	        glVertex4f(vertices[i].x / vertices[i].rhw,
			   vertices[i].y / vertices[i].rhw,
			   vertices[i].z / vertices[i].rhw,
			   1.0 / vertices[i].rhw);
	    TRACE(" %f %f %f %f / %02lx %02lx %02lx %02lx - %02lx %02lx %02lx %02lx (%f %f)\n",
		  vertices[i].x, vertices[i].y, vertices[i].z, vertices[i].rhw,
		  (vertices[i].diffuse >> 24) & 0xFF,
		  (vertices[i].diffuse >> 16) & 0xFF,
		  (vertices[i].diffuse >>  8) & 0xFF,
		  (vertices[i].diffuse >>  0) & 0xFF,
		  (vertices[i].specular >> 24) & 0xFF,
		  (vertices[i].specular >> 16) & 0xFF,
		  (vertices[i].specular >>  8) & 0xFF,
		  (vertices[i].specular >>  0) & 0xFF,
		  vertices[i].tu1, vertices[i].tv1);
	} 
    }
    
    glEnd();
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
    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwFlags);

    draw_primitive_7(This, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, NULL, dwVertexCount, dwFlags);
    
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
    TRACE("(%p/%p)->(%08x,%08lx,%p,%08lx,%p,%08lx,%08lx)\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);

    draw_primitive_7(This, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
						 DWORD dwStage,
						 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
						 DWORD dwState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    GLenum gl_state;
    
    TRACE("(%p/%p)->(%08lx,%08x,%08lx)\n", This, iface, dwStage, d3dTexStageStateType, dwState);

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
            switch ((D3DTEXTUREMINFILTER) dwState) {
	        case D3DTFN_POINT:
	            if (TRACE_ON(ddraw)) DPRINTF("D3DTFN_POINT\n");
		    gl_state = GL_NEAREST;
		    break;
		case D3DTFN_LINEAR:
	            if (TRACE_ON(ddraw)) DPRINTF("D3DTFN_LINEAR\n");
		    gl_state = GL_LINEAR;
		    break;
		default:
		    if (TRACE_ON(ddraw)) DPRINTF(" state unhandled.\n");
		    gl_state = GL_LINEAR;
		    break;
	    }
	    glThis->render_state.min = gl_state;
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_state);
            break;
	    
        case D3DTSS_MAGFILTER:
            switch ((D3DTEXTUREMAGFILTER) dwState) {
	        case D3DTFG_POINT:
	            if (TRACE_ON(ddraw)) DPRINTF("D3DTFG_POINT\n");
		    gl_state = GL_NEAREST;
		    break;
		case D3DTFG_LINEAR:
	            if (TRACE_ON(ddraw)) DPRINTF("D3DTFG_LINEAR\n");
		    gl_state = GL_LINEAR;
		    break;
		default:
		    if (TRACE_ON(ddraw)) DPRINTF(" state unhandled.\n");
		    gl_state = GL_LINEAR;
		    break;
	    }
	    glThis->render_state.mag = gl_state;
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_state);
            break;
	    
	default:
	    if (TRACE_ON(ddraw)) DPRINTF(" unhandled.\n");
    }
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_3_SetTexture(LPDIRECT3DDEVICE3 iface,
				    DWORD dwStage,
				    LPDIRECT3DTEXTURE2 lpTexture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwStage, lpTexture2);
    
    if (This->current_texture[dwStage] != NULL) {
        /* Seems that this is not right... Need to test in real Windows
	   IDirect3DTexture2_Release(ICOM_INTERFACE(This->current_texture[dwStage], IDirect3DTexture2)); */
    }
    
    ENTER_GL();
    if (lpTexture2 == NULL) {
        TRACE(" disabling 2D texturing.\n");
	glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    } else {
        IDirect3DTextureImpl *tex_impl = ICOM_OBJECT(IDirect3DTextureImpl, IDirect3DTexture2, lpTexture2);
	IDirect3DTextureGLImpl *tex_glimpl = (IDirect3DTextureGLImpl *) tex_impl;
	
	This->current_texture[dwStage] = tex_impl;
	IDirect3DTexture2_AddRef(lpTexture2);

	TRACE(" activating OpenGL texture %d.\n", tex_glimpl->tex_name);
	
        glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_glimpl->tex_name);
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
    XCAST(SetTransform) GL_IDirect3DDeviceImpl_7_3T_2T_SetTransform,
    XCAST(GetTransform) Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform,
    XCAST(SetViewport) Main_IDirect3DDeviceImpl_7_SetViewport,
    XCAST(MultiplyTransform) Main_IDirect3DDeviceImpl_7_3T_2T_MultiplyTransform,
    XCAST(GetViewport) Main_IDirect3DDeviceImpl_7_GetViewport,
    XCAST(SetMaterial) Main_IDirect3DDeviceImpl_7_SetMaterial,
    XCAST(GetMaterial) Main_IDirect3DDeviceImpl_7_GetMaterial,
    XCAST(SetLight) Main_IDirect3DDeviceImpl_7_SetLight,
    XCAST(GetLight) Main_IDirect3DDeviceImpl_7_GetLight,
    XCAST(SetRenderState) GL_IDirect3DDeviceImpl_7_3T_2T_SetRenderState,
    XCAST(GetRenderState) Main_IDirect3DDeviceImpl_7_3T_2T_GetRenderState,
    XCAST(BeginStateBlock) Main_IDirect3DDeviceImpl_7_BeginStateBlock,
    XCAST(EndStateBlock) Main_IDirect3DDeviceImpl_7_EndStateBlock,
    XCAST(PreLoad) Main_IDirect3DDeviceImpl_7_PreLoad,
    XCAST(DrawPrimitive) GL_IDirect3DDeviceImpl_7_3T_DrawPrimitive,
    XCAST(DrawIndexedPrimitive) GL_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitive,
    XCAST(SetClipStatus) Main_IDirect3DDeviceImpl_7_3T_2T_SetClipStatus,
    XCAST(GetClipStatus) Main_IDirect3DDeviceImpl_7_3T_2T_GetClipStatus,
    XCAST(DrawPrimitiveStrided) Main_IDirect3DDeviceImpl_7_3T_DrawPrimitiveStrided,
    XCAST(DrawIndexedPrimitiveStrided) Main_IDirect3DDeviceImpl_7_3T_DrawIndexedPrimitiveStrided,
    XCAST(DrawPrimitiveVB) Main_IDirect3DDeviceImpl_7_DrawPrimitiveVB,
    XCAST(DrawIndexedPrimitiveVB) Main_IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB,
    XCAST(ComputeSphereVisibility) Main_IDirect3DDeviceImpl_7_3T_ComputeSphereVisibility,
    XCAST(GetTexture) Main_IDirect3DDeviceImpl_7_GetTexture,
    XCAST(SetTexture) Main_IDirect3DDeviceImpl_7_SetTexture,
    XCAST(GetTextureStageState) Main_IDirect3DDeviceImpl_7_3T_GetTextureStageState,
    XCAST(SetTextureStageState) GL_IDirect3DDeviceImpl_7_3T_SetTextureStageState,
    XCAST(ValidateDevice) Main_IDirect3DDeviceImpl_7_3T_ValidateDevice,
    XCAST(ApplyStateBlock) Main_IDirect3DDeviceImpl_7_ApplyStateBlock,
    XCAST(CaptureStateBlock) Main_IDirect3DDeviceImpl_7_CaptureStateBlock,
    XCAST(DeleteStateBlock) Main_IDirect3DDeviceImpl_7_DeleteStateBlock,
    XCAST(CreateStateBlock) Main_IDirect3DDeviceImpl_7_CreateStateBlock,
    XCAST(Load) Main_IDirect3DDeviceImpl_7_Load,
    XCAST(LightEnable) Main_IDirect3DDeviceImpl_7_LightEnable,
    XCAST(GetLightEnable) Main_IDirect3DDeviceImpl_7_GetLightEnable,
    XCAST(SetClipPlane) Main_IDirect3DDeviceImpl_7_SetClipPlane,
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
    XCAST(DrawPrimitiveVB) Main_IDirect3DDeviceImpl_3_DrawPrimitiveVB,
    XCAST(DrawIndexedPrimitiveVB) Main_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB,
    XCAST(ComputeSphereVisibility) Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility,
    XCAST(GetTexture) Main_IDirect3DDeviceImpl_3_GetTexture,
    XCAST(SetTexture) GL_IDirect3DDeviceImpl_3_SetTexture,
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
    XCAST(SwapTextureHandles) Main_IDirect3DDeviceImpl_2_SwapTextureHandles,
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
    XCAST(SwapTextureHandles) Main_IDirect3DDeviceImpl_1_SwapTextureHandles,
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
    GLenum buffer;
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDeviceGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    gl_object = (IDirect3DDeviceGLImpl *) object;
    
    object->ref = 1;
    object->d3d = d3d;
    object->surface = surface;
    object->set_context = set_context;

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
	}
	surf->d3ddevice = object;
    }
    
    gl_object->render_state.src = GL_ONE;
    gl_object->render_state.dst = GL_ZERO;
    gl_object->render_state.mag = GL_NEAREST;
    gl_object->render_state.min = GL_NEAREST;

    /* Allocate memory for the matrices */
    gl_object->world_mat = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    gl_object->view_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    gl_object->proj_mat  = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));

    memcpy(gl_object->world_mat, id_mat, 16 * sizeof(float));
    memcpy(gl_object->view_mat , id_mat, 16 * sizeof(float));
    memcpy(gl_object->proj_mat , id_mat, 16 * sizeof(float));

    /* Initialisation */
    TRACE(" setting current context\n");
    LEAVE_GL();
    object->set_context(object);
    ENTER_GL();
    TRACE(" current context set\n");
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);
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
    
    return DD_OK;
}
