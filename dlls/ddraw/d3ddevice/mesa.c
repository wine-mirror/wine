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
#include <math.h>

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

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_geom);

/* x11drv GDI escapes */
#define X11DRV_ESCAPE 6789
enum x11drv_escape_codes
{
    X11DRV_GET_DISPLAY,   /* get X11 display for a DC */
    X11DRV_GET_DRAWABLE,  /* get current drawable for a DC */
    X11DRV_GET_FONT,      /* get current X font for a DC */
};

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


static BOOL opengl_flip( LPVOID dev, LPVOID drawable)
{
    IDirect3DDeviceImpl *d3d_dev = (IDirect3DDeviceImpl *) dev;
    IDirect3DDeviceGLImpl *gl_d3d_dev = (IDirect3DDeviceGLImpl *) dev;

    TRACE("(%p, %ld)\n", gl_d3d_dev->display,(Drawable)drawable);
    ENTER_GL();
    if (gl_d3d_dev->state == SURFACE_MEMORY_DIRTY) {
        d3d_dev->flush_to_framebuffer(d3d_dev, NULL, gl_d3d_dev->lock_surf);
    }
    gl_d3d_dev->state = SURFACE_GL;
    gl_d3d_dev->front_state = SURFACE_GL;
    glXSwapBuffers(gl_d3d_dev->display, (Drawable)drawable);
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

HRESULT d3ddevice_enumerate(LPD3DENUMDEVICESCALLBACK cb, LPVOID context, DWORD version)
{
    D3DDEVICEDESC dref, d1, d2;
    HRESULT ret_value;

    /* Some games (Motoracer 2 demo) have the bad idea to modify the device name string.
       Let's put the string in a sufficiently sized array in writable memory. */
    char device_name[50];
    strcpy(device_name,"direct3d");

    fill_opengl_caps(&dref);

    if (version > 1) {
        /* It seems that enumerating the reference IID on Direct3D 1 games (AvP / Motoracer2) breaks them */
        TRACE(" enumerating OpenGL D3DDevice interface using reference IID (IID %s).\n", debugstr_guid(&IID_IDirect3DRefDevice));
	d1 = dref;
	d2 = dref;
	ret_value = cb((LPIID) &IID_IDirect3DRefDevice, "WINE Reference Direct3DX using OpenGL", device_name, &d1, &d2, context);
	if (ret_value != D3DENUMRET_OK)
	    return ret_value;
    }
    
    TRACE(" enumerating OpenGL D3DDevice interface (IID %s).\n", debugstr_guid(&IID_D3DDEVICE_OpenGL));
    d1 = dref;
    d2 = dref;
    ret_value = cb((LPIID) &IID_D3DDEVICE_OpenGL, "WINE Direct3DX using OpenGL", device_name, &d1, &d2, context);
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
	IDirectDrawSurfaceImpl *surface = This->surface, *surf;
	
	/* Release texture associated with the device */ 
	for (i = 0; i < MAX_TEXTURES; i++) {
	    if (This->current_texture[i] != NULL)
	        IDirectDrawSurface7_Release(ICOM_INTERFACE(This->current_texture[i], IDirectDrawSurface7));
	    HeapFree(GetProcessHeap(), 0, This->tex_mat[i]);
	}

	/* Look for the front buffer and override its surface's Flip method (if in double buffering) */
	for (surf = surface; surf != NULL; surf = surf->surface_owner) {
	    if ((surf->surface_desc.ddsCaps.dwCaps&(DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER)) == (DDSCAPS_FLIP|DDSCAPS_FRONTBUFFER)) {
	        surf->aux_ctx  = NULL;
		surf->aux_data = NULL;
		surf->aux_flip = NULL;
		break;
	    }
	}
	for (surf = surface; surf != NULL; surf = surf->surface_owner) {
	    IDirectDrawSurfaceImpl *surf2;
	    for (surf2 = surf; surf2->prev_attached != NULL; surf2 = surf2->prev_attached) ;
	    for (; surf2 != NULL; surf2 = surf2->next_attached) {
	        if (((surf2->surface_desc.ddsCaps.dwCaps & (DDSCAPS_3DDEVICE)) == (DDSCAPS_3DDEVICE)) &&
		    ((surf2->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER)) != (DDSCAPS_ZBUFFER))) {
		    /* Override the Lock / Unlock function for all these surfaces */
		    surf2->lock_update = surf2->lock_update_prev;
		    surf2->unlock_update = surf2->unlock_update_prev;
		    /* And install also the blt / bltfast overrides */
		    surf2->aux_blt = NULL;
		    surf2->aux_bltfast = NULL;
		}
		surf2->d3ddevice = NULL;
	    }
	}
	
	/* And warn the D3D object that this device is no longer active... */
	This->d3d->removed_device(This->d3d, This);

	HeapFree(GetProcessHeap(), 0, This->world_mat);
	HeapFree(GetProcessHeap(), 0, This->view_mat);
	HeapFree(GetProcessHeap(), 0, This->proj_mat);

	DeleteCriticalSection(&(This->crit));
	
	ENTER_GL();
	if (glThis->unlock_tex)
	    glDeleteTextures(1, &(glThis->unlock_tex));
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

#if 0
    /* See argument about the RGBA format for 'packed' texture formats */
    TRACE("Enumerating GL_RGBA unpacked (32)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 32;
    pformat->u2.dwRBitMask =        0xFF000000;
    pformat->u3.dwGBitMask =        0x00FF0000;
    pformat->u4.dwBBitMask =        0x0000FF00;
    pformat->u5.dwRGBAlphaBitMask = 0x000000FF;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;
#endif
    
    TRACE("Enumerating GL_RGBA unpacked (32)\n");
    pformat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    pformat->u1.dwRGBBitCount = 32;
    pformat->u2.dwRBitMask =        0x00FF0000;
    pformat->u3.dwGBitMask =        0x0000FF00;
    pformat->u4.dwBBitMask =        0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0xFF000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

#if 0 /* Enabling this breaks Tomb Raider 3, need to investigate... */
    TRACE("Enumerating GL_RGB unpacked (32)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 32;
    pformat->u2.dwRBitMask =        0x00FF0000;
    pformat->u3.dwGBitMask =        0x0000FF00;
    pformat->u4.dwBBitMask =        0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;
#endif
    
    TRACE("Enumerating GL_RGB unpacked (24)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 24;
    pformat->u2.dwRBitMask = 0x00FF0000;
    pformat->u3.dwGBitMask = 0x0000FF00;
    pformat->u4.dwBBitMask = 0x000000FF;
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

    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_6_5 (16)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask = 0x0000F800;
    pformat->u3.dwGBitMask = 0x000007E0;
    pformat->u4.dwBBitMask = 0x0000001F;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB packed GL_UNSIGNED_SHORT_5_5_5 (16)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 16;
    pformat->u2.dwRBitMask = 0x00007C00;
    pformat->u3.dwGBitMask = 0x000003E0;
    pformat->u4.dwBBitMask = 0x0000001F;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
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

    if (!dwLightStateType && (dwLightStateType > D3DLIGHTSTATE_COLORVERTEX))
	TRACE("Unexpected Light State Type\n");
	return DDERR_INVALIDPARAMS;
	
    if (dwLightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */) {
	IDirect3DMaterialImpl *mat = (IDirect3DMaterialImpl *) dwLightState;

	if (mat != NULL) {
	    ENTER_GL();
	    mat->activate(mat);
	    LEAVE_GL();
	} else {
	    ERR(" D3DLIGHTSTATE_MATERIAL called with NULL material !!!\n");
	}
    } else if (dwLightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */) {
	switch (dwLightState) {
	    case D3DCOLOR_MONO:
	       ERR("DDCOLOR_MONO should not happen!\n");
	       break;
	    case D3DCOLOR_RGB:
	       /* We are already in this mode */
	       break;
	    default:
	       ERR("Unknown color model!\n");
	       break;
	}
    } else {
        D3DRENDERSTATETYPE rs;
	switch (dwLightStateType) {

	    case D3DLIGHTSTATE_AMBIENT:       /* 2 */
		rs = D3DRENDERSTATE_AMBIENT;
		break;		
	    case D3DLIGHTSTATE_FOGMODE:       /* 4 */
		rs = D3DRENDERSTATE_FOGVERTEXMODE;
		break;
	    case D3DLIGHTSTATE_FOGSTART:      /* 5 */
		rs = D3DRENDERSTATE_FOGSTART;
		break;
	    case D3DLIGHTSTATE_FOGEND:        /* 6 */
		rs = D3DRENDERSTATE_FOGEND;
		break;
	    case D3DLIGHTSTATE_FOGDENSITY:    /* 7 */
		rs = D3DRENDERSTATE_FOGDENSITY;
		break;
	    case D3DLIGHTSTATE_COLORVERTEX:   /* 8 */
		rs = D3DRENDERSTATE_COLORVERTEX;
		break;
	    default:
		break;
	}

	IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
	                   		rs,dwLightState);
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
	    FIXME("Unhandled primitive %08x\n", d3dpt);
	    break;
    }
}

/* This function calculate the Z coordinate from Zproj */ 
static float ZfromZproj(IDirect3DDeviceImpl *This, D3DVALUE Zproj)
{
    float a,b,c,d;
    /* Assume that X = Y = 0 and W = 1 */
    a = This->proj_mat->_33;
    b = This->proj_mat->_34;
    c = This->proj_mat->_43;
    d = This->proj_mat->_44;
    /* We have in homogenous coordinates Z' = a * Z + c and W' = b * Z + d
     * So in non homogenous coordinates we have Zproj = (a * Z + c) / (b * Z + d)
     * And finally Z = (d * Zproj - c) / (a - b * Zproj)
     */
    return (d*Zproj - c) / (a - b*Zproj);
}

static void build_fog_table(BYTE *fog_table, DWORD fog_color) {
    int i;
    
    TRACE(" rebuilding fog table (%06lx)...\n", fog_color & 0x00FFFFFF);
    
    for (i = 0; i < 3; i++) {
        BYTE fog_color_component = (fog_color >> (8 * i)) & 0xFF;
	DWORD elt;
	for (elt = 0; elt < 0x10000; elt++) {
	    /* We apply the fog transformation and cache the result */
	    DWORD fog_intensity = elt & 0xFF;
	    DWORD vertex_color = (elt >> 8) & 0xFF;
	    fog_table[(i * 0x10000) + elt] = ((fog_intensity * vertex_color) + ((0xFF - fog_intensity) * fog_color_component)) / 0xFF;
	}
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

    } else if ((vertex_transformed == TRUE) &&
	       (glThis->transform_state != GL_TRANSFORM_ORTHO)) {
        /* Set our orthographic projection */
        glThis->transform_state = GL_TRANSFORM_ORTHO;
	d3ddevice_set_ortho(This);
    }

    /* TODO: optimize this to not always reset all the fog stuff on all DrawPrimitive call
             if no fogging state change occured */
    if (This->state_block.render_state[D3DRENDERSTATE_FOGENABLE - 1] == TRUE) {
        if (vertex_transformed == TRUE) {
	    glDisable(GL_FOG);
	    /* Now check if our fog_table still corresponds to the current vertex color.
	       Element '0x..00' is always the fog color as it corresponds to maximum fog intensity */
	    if ((glThis->fog_table[0 * 0x10000 + 0x0000] != ((This->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1] >>  0) & 0xFF)) ||
		(glThis->fog_table[1 * 0x10000 + 0x0000] != ((This->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1] >>  8) & 0xFF)) ||
		(glThis->fog_table[2 * 0x10000 + 0x0000] != ((This->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1] >> 16) & 0xFF))) {
	        /* We need to rebuild our fog table.... */
		build_fog_table(glThis->fog_table, This->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1]);
	    }
	} else {
	    if (This->state_block.render_state[D3DRENDERSTATE_FOGTABLEMODE - 1] != D3DFOG_NONE) {
	        switch (This->state_block.render_state[D3DRENDERSTATE_FOGTABLEMODE - 1]) {
                    case D3DFOG_LINEAR: glFogi(GL_FOG_MODE, GL_LINEAR); break; 
		    case D3DFOG_EXP:    glFogi(GL_FOG_MODE, GL_EXP); break; 
		    case D3DFOG_EXP2:   glFogi(GL_FOG_MODE, GL_EXP2); break;
		}
		if (vertex_lit == FALSE) {
		    glFogf(GL_FOG_START, *(float*)&This->state_block.render_state[D3DRENDERSTATE_FOGSTART - 1]);
		    glFogf(GL_FOG_END, *(float*)&This->state_block.render_state[D3DRENDERSTATE_FOGEND - 1]);
		} else {
		    /* Special case of 'pixel fog' */
		    glFogf(GL_FOG_START, ZfromZproj(This, *(float*)&This->state_block.render_state[D3DRENDERSTATE_FOGSTART - 1]));
		    glFogf(GL_FOG_END, ZfromZproj(This, *(float*)&This->state_block.render_state[D3DRENDERSTATE_FOGEND - 1]));
		}
		glEnable(GL_FOG);
	    } else {
                glDisable(GL_FOG);
	    }
        }
    } else {
	glDisable(GL_FOG);
    }
    
    /* Handle the 'no-normal' case */
    if ((vertex_lit == FALSE) && (This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE))
        glEnable(GL_LIGHTING);
    else 
	glDisable(GL_LIGHTING);

    /* Handle the code for pre-vertex material properties */
    if (vertex_transformed == FALSE) {
        if ((This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE) &&
	    (This->state_block.render_state[D3DRENDERSTATE_COLORVERTEX - 1] == TRUE)) {
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
	    draw_primitive_strided(This, d3dpt, D3DFVF_VERTEX, &strided, 0 /* Unused */, index, maxvert, 0 /* Unused */);
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
	    draw_primitive_strided(This, d3dpt, D3DFVF_LVERTEX, &strided, 0 /* Unused */, index, maxvert, 0 /* Unused */);
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
	    draw_primitive_strided(This, d3dpt, D3DFVF_TLVERTEX, &strided, 0 /* Unused */, index, maxvert, 0 /* Unused */);
	} break;

        default:
	    FIXME("Unhandled vertex type %08x\n", d3dvt);
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
	(sb->render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE) &&
	(sb->render_state[D3DRENDERSTATE_COLORVERTEX - 1] == TRUE)) {
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
	(sb->render_state[D3DRENDERSTATE_LIGHTING - 1] == TRUE) &&
	(sb->render_state[D3DRENDERSTATE_COLORVERTEX - 1] == TRUE)) {
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

inline static void handle_diffuse_and_specular(STATEBLOCK *sb, BYTE *fog_table, DWORD *color_d, DWORD *color_s, BOOLEAN lighted) {
    if (lighted == TRUE) {
        DWORD color = *color_d;
        if (sb->render_state[D3DRENDERSTATE_FOGENABLE - 1] == TRUE) {
	    /* Special case where the specular value is used to do fogging */
	    BYTE fog_intensity = *color_s >> 24; /* The alpha value of the specular component is the fog 'intensity' for this vertex */
	    color &= 0xFF000000; /* Only keep the alpha component */
	    color |= fog_table[((*color_d >>  0) & 0xFF) << 8 | fog_intensity] <<  0;
	    color |= fog_table[((*color_d >>  8) & 0xFF) << 8 | fog_intensity] <<  8;
	    color |= fog_table[((*color_d >> 16) & 0xFF) << 8 | fog_intensity] << 16;
	}
	if (sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1] == TRUE) {
	    /* Standard specular value in transformed mode. TODO */
	}
	handle_diffuse_base(sb, &color);
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
				   DWORD dwVertexCount,
				   LPWORD dwIndices,
				   DWORD dwIndexCount,
				   DWORD dwFlags)
{
    BOOLEAN vertex_lighted = FALSE;
    IDirect3DDeviceGLImpl* glThis = (IDirect3DDeviceGLImpl*) This;
    int num_active_stages = 0;

    /* This is to prevent 'thread contention' between a thread locking the device and another
       doing 3D display on it... */
    EnterCriticalSection(&(This->crit));   
    
    ENTER_GL();
    if (glThis->state == SURFACE_MEMORY_DIRTY) {
        This->flush_to_framebuffer(This, NULL, glThis->lock_surf);
    }

    glThis->state = SURFACE_GL;
    
    /* Compute the number of active texture stages */
    while (This->current_texture[num_active_stages] != NULL) num_active_stages++;

    if (TRACE_ON(ddraw)) {
        TRACE(" Vertex format : "); dump_flexible_vertex(d3dvtVertexType);
    }

    /* Just a hack for now.. Will have to find better algorithm :-/ */
    if ((d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ) {
        vertex_lighted = TRUE;
    } else {
        if ((d3dvtVertexType & D3DFVF_NORMAL) == 0) glNormal3f(0.0, 0.0, 0.0);
    }
    
    draw_primitive_handle_GL_state(This,
				   (d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ,
				   vertex_lighted);
    draw_primitive_start_GL(d3dptPrimitiveType);

    /* Some fast paths first before the generic case.... */
    if ((d3dvtVertexType == D3DFVF_VERTEX) && (num_active_stages <= 1)) {
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
	    
	    TRACE_(ddraw_geom)(" %f %f %f / %f %f %f (%f %f)\n",
			       position[0], position[1], position[2],
			       normal[0], normal[1], normal[2],
			       tex_coord[0], tex_coord[1]);
	}
    } else if ((d3dvtVertexType == D3DFVF_TLVERTEX) && (num_active_stages <= 1)) {
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

	    handle_diffuse_and_specular(&(This->state_block), glThis->fog_table, color_d, color_s, TRUE);
	    handle_texture(tex_coord);
	    handle_xyzrhw(position);

	    TRACE_(ddraw_geom)(" %f %f %f %f / %02lx %02lx %02lx %02lx - %02lx %02lx %02lx %02lx (%f %f)\n",
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
	int num_tex_index = ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT);
	static const D3DVALUE no_index[] = { 0.0, 0.0, 0.0, 0.0 };
	  
	for (index = 0; index < dwIndexCount; index++) {
	    int i = (dwIndices == NULL) ? index : dwIndices[index];
	    int tex_stage;

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
		handle_diffuse_and_specular(&(This->state_block), glThis->fog_table, color_d, color_s, vertex_lighted);
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

	    for (tex_stage = 0; tex_stage < num_active_stages; tex_stage++) {
	        int tex_index = This->state_block.texture_stage_state[tex_stage][D3DTSS_TEXCOORDINDEX - 1] & 0xFFFF000;
		if (tex_index >= num_tex_index) {
		    handle_textures((D3DVALUE *) no_index, tex_stage);
		 } else {
		     D3DVALUE *tex_coord =
		       (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
				     i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
		     handle_textures(tex_coord, tex_stage);
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

	    if (TRACE_ON(ddraw_geom)) {
	        int tex_index;

		if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
		    D3DVALUE *position =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		    TRACE_(ddraw_geom)(" %f %f %f", position[0], position[1], position[2]);
		} else if ((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) {
		    D3DVALUE *position =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		    TRACE_(ddraw_geom)(" %f %f %f %f", position[0], position[1], position[2], position[3]);
		}
	        if (d3dvtVertexType & D3DFVF_NORMAL) { 
		    D3DVALUE *normal = 
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->normal.lpvData) + i * lpD3DDrawPrimStrideData->normal.dwStride);	    
		    TRACE_(ddraw_geom)(" / %f %f %f", normal[0], normal[1], normal[2]);
		}
		if (d3dvtVertexType & D3DFVF_DIFFUSE) {
		    DWORD *color_d = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->diffuse.lpvData) + i * lpD3DDrawPrimStrideData->diffuse.dwStride);
		    TRACE_(ddraw_geom)(" / %02lx %02lx %02lx %02lx",
				       (*color_d >> 16) & 0xFF,
				       (*color_d >>  8) & 0xFF,
				       (*color_d >>  0) & 0xFF,
				       (*color_d >> 24) & 0xFF);
		}
	        if (d3dvtVertexType & D3DFVF_SPECULAR) { 
		    DWORD *color_s = 
		      (DWORD *) (((char *) lpD3DDrawPrimStrideData->specular.lpvData) + i * lpD3DDrawPrimStrideData->specular.dwStride);
		    TRACE_(ddraw_geom)(" / %02lx %02lx %02lx %02lx",
				       (*color_s >> 16) & 0xFF,
				       (*color_s >>  8) & 0xFF,
				       (*color_s >>  0) & 0xFF,
				       (*color_s >> 24) & 0xFF);
		}
		for (tex_index = 0; tex_index < ((d3dvtVertexType & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT); tex_index++) {
                    D3DVALUE *tex_coord =
		      (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
				    i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
		    TRACE_(ddraw_geom)(" / %f %f", tex_coord[0], tex_coord[1]);
		}
		TRACE_(ddraw_geom)("\n");
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

    LeaveCriticalSection(&(This->crit));
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

    convert_FVF_to_strided_data(d3dvtVertexType, lpvVertices, &strided, 0);
    draw_primitive_strided(This, d3dptPrimitiveType, d3dvtVertexType, &strided, dwVertexCount, NULL, dwVertexCount, dwFlags);
    
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

    convert_FVF_to_strided_data(d3dvtVertexType, lpvVertices, &strided, 0);
    draw_primitive_strided(This, d3dptPrimitiveType, d3dvtVertexType, &strided, dwVertexCount, dwIndices, dwIndexCount, dwFlags);
    
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
    draw_primitive_strided(This, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, NULL, dwVertexCount, dwFlags);

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

    draw_primitive_strided(This, d3dptPrimitiveType, dwVertexType, lpD3DDrawPrimStrideData, dwVertexCount, lpIndex, dwIndexCount, dwFlags);

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

	convert_FVF_to_strided_data(vb_glimp->dwVertexTypeDesc, vb_glimp->vertices, &strided, dwStartVertex);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_glimp->dwVertexTypeDesc, &strided, dwNumVertices, NULL, dwNumVertices, dwFlags);

    } else {
        convert_FVF_to_strided_data(vb_impl->desc.dwFVF, vb_impl->vertices, &strided, dwStartVertex);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_impl->desc.dwFVF, &strided, dwNumVertices, NULL, dwNumVertices, dwFlags);
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

	convert_FVF_to_strided_data(vb_glimp->dwVertexTypeDesc, vb_glimp->vertices, &strided, dwStartVertex);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_glimp->dwVertexTypeDesc, &strided, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);

    } else {
        convert_FVF_to_strided_data(vb_impl->desc.dwFVF, vb_impl->vertices, &strided, dwStartVertex);
	draw_primitive_strided(This, d3dptPrimitiveType, vb_impl->desc.dwFVF, &strided, dwNumVertices, lpwIndices, dwIndexCount, dwFlags);
    }

    return DD_OK;
}

static GLenum
convert_min_filter_to_GL(D3DTEXTUREMINFILTER dwMinState, D3DTEXTUREMIPFILTER dwMipState)
{
    GLenum gl_state;

    if (dwMipState == D3DTFP_NONE) {
        switch (dwMinState) {
            case D3DTFN_POINT:  gl_state = GL_NEAREST; break;
	    case D3DTFN_LINEAR: gl_state = GL_LINEAR;  break;
	    default:            gl_state = GL_LINEAR;  break;
	}
    } else if (dwMipState == D3DTFP_POINT) {
        switch (dwMinState) {
            case D3DTFN_POINT:  gl_state = GL_NEAREST_MIPMAP_NEAREST; break;
	    case D3DTFN_LINEAR: gl_state = GL_LINEAR_MIPMAP_NEAREST;  break;
	    default:            gl_state = GL_LINEAR_MIPMAP_NEAREST;  break;
	}
    } else {
        switch (dwMinState) {
            case D3DTFN_POINT:  gl_state = GL_NEAREST_MIPMAP_LINEAR; break;
	    case D3DTFN_LINEAR: gl_state = GL_LINEAR_MIPMAP_LINEAR;  break;
	    default:            gl_state = GL_LINEAR_MIPMAP_LINEAR;  break;
	}
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

static GLenum
convert_tex_address_to_GL(D3DTEXTUREADDRESS dwState)
{
    GLenum gl_state;
    switch (dwState) {
        case D3DTADDRESS_WRAP:   gl_state = GL_REPEAT; break;
	case D3DTADDRESS_CLAMP:  gl_state = GL_CLAMP; break;
	case D3DTADDRESS_BORDER: gl_state = GL_CLAMP_TO_EDGE; break;
#if defined(GL_VERSION_1_4)
	case D3DTADDRESS_MIRROR: gl_state = GL_MIRRORED_REPEAT; break;
#elif defined(GL_ARB_texture_mirrored_repeat)
	case D3DTADDRESS_MIRROR: gl_state = GL_MIRRORED_REPEAT_ARB; break;
#endif
	default:                 gl_state = GL_REPEAT; break;
    }
    return gl_state;
}

/* We need a static function for that to handle the 'special' case of 'SELECT_ARG2' */
static BOOLEAN
handle_color_alpha_args(IDirect3DDeviceImpl *This, DWORD dwStage, D3DTEXTURESTAGESTATETYPE d3dTexStageStateType, DWORD dwState, D3DTEXTUREOP tex_op)
{
    BOOLEAN is_complement = FALSE;
    BOOLEAN is_alpha_replicate = FALSE;
    BOOLEAN handled = TRUE;
    GLenum src;
    BOOLEAN is_color = ((d3dTexStageStateType == D3DTSS_COLORARG1) || (d3dTexStageStateType == D3DTSS_COLORARG2));
    int num;
    
    if (is_color) {
        if (d3dTexStageStateType == D3DTSS_COLORARG1) num = 0;
	else if (d3dTexStageStateType == D3DTSS_COLORARG2) num = 1;
	else {
	    handled = FALSE;
	    num = 0;
	}
	if (tex_op == D3DTOP_SELECTARG2) {
	    num = 1 - num;
	}
    } else {
        if (d3dTexStageStateType == D3DTSS_ALPHAARG1) num = 0;
	else if (d3dTexStageStateType == D3DTSS_ALPHAARG2) num = 1;
	else {
	    handled = FALSE;
	    num = 0;
	}
	if (tex_op == D3DTOP_SELECTARG2) {
	    num = 1 - num;
	}
    }
    
    if (dwState & D3DTA_COMPLEMENT) {
        is_complement = TRUE;
    }
    if (dwState & D3DTA_ALPHAREPLICATE) {
	is_alpha_replicate = TRUE;
    }
    dwState &= D3DTA_SELECTMASK;
    if ((dwStage == 0) && (dwState == D3DTA_CURRENT)) {
        dwState = D3DTA_DIFFUSE;
    }

    switch (dwState) {
        case D3DTA_CURRENT: src = GL_PREVIOUS_EXT; break;
	case D3DTA_DIFFUSE: src = GL_PRIMARY_COLOR_EXT; break;
	case D3DTA_TEXTURE: src = GL_TEXTURE; break;
	case D3DTA_TFACTOR: {
	    /* Get the constant value from the current rendering state */
	    GLfloat color[4];
	    DWORD col = This->state_block.render_state[D3DRENDERSTATE_TEXTUREFACTOR - 1];
	    
	    color[0] = ((col >> 16) & 0xFF) / 255.0f;
	    color[1] = ((col >>  8) & 0xFF) / 255.0f;
	    color[2] = ((col >>  0) & 0xFF) / 255.0f;
	    color[3] = ((col >> 24) & 0xFF) / 255.0f;
	    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
	    
	    src = GL_CONSTANT_EXT;
	} break;
	default: src = GL_TEXTURE; handled = FALSE; break;
    }

    if (is_color) {
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT + num, src);
	if (is_alpha_replicate) {
	    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT + num, is_complement ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA);
	} else {
	    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT + num, is_complement ? GL_ONE_MINUS_SRC_COLOR : GL_SRC_COLOR);
	}
    } else {
        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT + num, src);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_EXT + num, is_complement ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA);
    }

    return handled;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_SetTextureStageState(LPDIRECT3DDEVICE7 iface,
						 DWORD dwStage,
						 D3DTEXTURESTAGESTATETYPE d3dTexStageStateType,
						 DWORD dwState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    const char *type;
    DWORD prev_state;
    
    TRACE("(%p/%p)->(%08lx,%08x,%08lx)\n", This, iface, dwStage, d3dTexStageStateType, dwState);

    if (dwStage > 0) return DD_OK; /* We nothing in this case for now */

    switch (d3dTexStageStateType) {
#define GEN_CASE(a) case a: type = #a; break
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
        default: type = "UNKNOWN";
    }

    /* Store the values in the state array */
    prev_state = This->state_block.texture_stage_state[dwStage][d3dTexStageStateType - 1];
    This->state_block.texture_stage_state[dwStage][d3dTexStageStateType - 1] = dwState;
    /* Some special cases when one state modifies more than one... */
    if (d3dTexStageStateType == D3DTSS_ADDRESS) {
        This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSU - 1] = dwState;
	This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSV - 1] = dwState;
    }

    ENTER_GL();
    
    switch (d3dTexStageStateType) {
        case D3DTSS_MINFILTER:
        case D3DTSS_MIPFILTER:
	    if (TRACE_ON(ddraw)) {
	        if (d3dTexStageStateType == D3DTSS_MINFILTER) {
		    switch ((D3DTEXTUREMINFILTER) dwState) {
	                case D3DTFN_POINT:  TRACE(" Stage type is : D3DTSS_MINFILTER => D3DTFN_POINT\n"); break;
			case D3DTFN_LINEAR: TRACE(" Stage type is : D3DTSS_MINFILTER => D3DTFN_LINEAR\n"); break;
			default: FIXME(" Unhandled stage type : D3DTSS_MINFILTER => %08lx\n", dwState); break;
		    }
		} else {
		    switch ((D3DTEXTUREMIPFILTER) dwState) {
	                case D3DTFP_NONE:   TRACE(" Stage type is : D3DTSS_MIPFILTER => D3DTFP_NONE\n"); break;
			case D3DTFP_POINT:  TRACE(" Stage type is : D3DTSS_MIPFILTER => D3DTFP_POINT\n"); break;
			case D3DTFP_LINEAR: TRACE(" Stage type is : D3DTSS_MIPFILTER => D3DTFP_LINEAR\n"); break;
			default: FIXME(" Unhandled stage type : D3DTSS_MIPFILTER => %08lx\n", dwState); break;
		    }
		}
	    }

	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			    convert_min_filter_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_MINFILTER - 1],
						     This->state_block.texture_stage_state[dwStage][D3DTSS_MIPFILTER - 1]));
	    break;
	    
        case D3DTSS_MAGFILTER:
	    if (TRACE_ON(ddraw)) {
	        switch ((D3DTEXTUREMAGFILTER) dwState) {
	            case D3DTFG_POINT:  TRACE(" Stage type is : D3DTSS_MAGFILTER => D3DTFN_POINT\n"); break;
		    case D3DTFG_LINEAR: TRACE(" Stage type is : D3DTSS_MAGFILTER => D3DTFN_LINEAR\n"); break;
		    default: FIXME(" Unhandled stage type : D3DTSS_MAGFILTER => %08lx\n", dwState); break;
		}
	    }
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, convert_mag_filter_to_GL(dwState));
            break;

        case D3DTSS_ADDRESS:
        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV: {
	    GLenum arg = convert_tex_address_to_GL(dwState);
	    
	    switch ((D3DTEXTUREADDRESS) dwState) {
	        case D3DTADDRESS_WRAP:   TRACE(" Stage type is : %s => D3DTADDRESS_WRAP\n", type); break;
	        case D3DTADDRESS_CLAMP:  TRACE(" Stage type is : %s => D3DTADDRESS_CLAMP\n", type); break;
	        case D3DTADDRESS_BORDER: TRACE(" Stage type is : %s => D3DTADDRESS_BORDER\n", type); break;
#if defined(GL_VERSION_1_4)
		case D3DTADDRESS_MIRROR: TRACE(" Stage type is : %s => D3DTADDRESS_MIRROR\n", type); break;
#elif defined(GL_ARB_texture_mirrored_repeat)
		case D3DTADDRESS_MIRROR: TRACE(" Stage type is : %s => D3DTADDRESS_MIRROR\n", type); break;
#endif
	        default: FIXME(" Unhandled stage type : %s => %08lx\n", type, dwState); break;
	    }
	    
	    if ((d3dTexStageStateType == D3DTSS_ADDRESS) ||
		(d3dTexStageStateType == D3DTSS_ADDRESSU))
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, arg);
	    if ((d3dTexStageStateType == D3DTSS_ADDRESS) ||
		(d3dTexStageStateType == D3DTSS_ADDRESSV))
	        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, arg);
        } break;

	case D3DTSS_ALPHAOP:
	case D3DTSS_COLOROP: {
            int scale = 1;
            GLenum parm = (d3dTexStageStateType == D3DTSS_ALPHAOP) ? GL_COMBINE_ALPHA_EXT : GL_COMBINE_RGB_EXT;
	    const char *value;
	    int handled = 1;
	    
	    switch (dwState) {
#define GEN_CASE(a) case a: value = #a; break
	        GEN_CASE(D3DTOP_DISABLE);
		GEN_CASE(D3DTOP_SELECTARG1);
		GEN_CASE(D3DTOP_SELECTARG2);
		GEN_CASE(D3DTOP_MODULATE);
		GEN_CASE(D3DTOP_MODULATE2X);
		GEN_CASE(D3DTOP_MODULATE4X);
		GEN_CASE(D3DTOP_ADD);
		GEN_CASE(D3DTOP_ADDSIGNED);
		GEN_CASE(D3DTOP_ADDSIGNED2X);
		GEN_CASE(D3DTOP_SUBTRACT);
		GEN_CASE(D3DTOP_ADDSMOOTH);
		GEN_CASE(D3DTOP_BLENDDIFFUSEALPHA);
		GEN_CASE(D3DTOP_BLENDTEXTUREALPHA);
		GEN_CASE(D3DTOP_BLENDFACTORALPHA);
		GEN_CASE(D3DTOP_BLENDTEXTUREALPHAPM);
		GEN_CASE(D3DTOP_BLENDCURRENTALPHA);
		GEN_CASE(D3DTOP_PREMODULATE);
		GEN_CASE(D3DTOP_MODULATEALPHA_ADDCOLOR);
		GEN_CASE(D3DTOP_MODULATECOLOR_ADDALPHA);
		GEN_CASE(D3DTOP_MODULATEINVALPHA_ADDCOLOR);
		GEN_CASE(D3DTOP_MODULATEINVCOLOR_ADDALPHA);
		GEN_CASE(D3DTOP_BUMPENVMAP);
		GEN_CASE(D3DTOP_BUMPENVMAPLUMINANCE);
		GEN_CASE(D3DTOP_DOTPRODUCT3);
		GEN_CASE(D3DTOP_FORCE_DWORD);
#undef GEN_CASE
	        default: value = "UNKNOWN";
	    }

            if ((d3dTexStageStateType == D3DTSS_COLOROP) && (dwState == D3DTOP_DISABLE) && (dwStage == 0)) {
                glDisable(GL_TEXTURE_2D);
		TRACE(" disabling 2D texturing.\n");
            } else {
	        /* Re-enable texturing */
	        if ((dwStage == 0) && (This->current_texture[0] != NULL)) {
		    glEnable(GL_TEXTURE_2D);
		    TRACE(" enabling 2D texturing.\n");
		}
		
                /* Re-Enable GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT */
                if (dwState != D3DTOP_DISABLE) {
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                }

                /* Now set up the operand correctly */
                switch (dwState) {
                    case D3DTOP_DISABLE:
		        /* Contrary to the docs, alpha can be disabled when colorop is enabled
			   and it works, so ignore this op */
		        TRACE(" Note : disable ALPHAOP but COLOROP enabled!\n");
			break;

		    case D3DTOP_SELECTARG1:
		    case D3DTOP_SELECTARG2:
			glTexEnvi(GL_TEXTURE_ENV, parm, GL_REPLACE);
			break;
			
		    case D3DTOP_MODULATE4X:
			scale = scale * 2;  /* Drop through */
		    case D3DTOP_MODULATE2X:
			scale = scale * 2;  /* Drop through */
		    case D3DTOP_MODULATE:
			glTexEnvi(GL_TEXTURE_ENV, parm, GL_MODULATE);
			break;

		    case D3DTOP_ADD:
			glTexEnvi(GL_TEXTURE_ENV, parm, GL_ADD);
			break;

		    case D3DTOP_ADDSIGNED2X:
			scale = scale * 2;  /* Drop through */
		    case D3DTOP_ADDSIGNED:
			glTexEnvi(GL_TEXTURE_ENV, parm, GL_ADD_SIGNED_EXT);
			break;

			/* For the four blending modes, use the Arg2 parameter */
		    case D3DTOP_BLENDDIFFUSEALPHA:
		    case D3DTOP_BLENDTEXTUREALPHA:
		    case D3DTOP_BLENDFACTORALPHA:
		    case D3DTOP_BLENDCURRENTALPHA: {
		        GLenum src = GL_PRIMARY_COLOR_EXT; /* Just to prevent a compiler warning.. */

			switch (dwState) {
			    case D3DTOP_BLENDDIFFUSEALPHA: src = GL_PRIMARY_COLOR_EXT;
			    case D3DTOP_BLENDTEXTUREALPHA: src = GL_TEXTURE;
			    case D3DTOP_BLENDFACTORALPHA:  src = GL_CONSTANT_EXT;
			    case D3DTOP_BLENDCURRENTALPHA: src = GL_PREVIOUS_EXT;
			}
			
			glTexEnvi(GL_TEXTURE_ENV, parm, GL_INTERPOLATE_EXT);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, src);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_EXT, GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, src);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_EXT, GL_SRC_ALPHA);
		    } break;
			
		    default:
			handled = FALSE;
			break;
                }
            }

	    if (((prev_state == D3DTOP_SELECTARG2) && (dwState != D3DTOP_SELECTARG2)) ||
		((dwState == D3DTOP_SELECTARG2) && (prev_state != D3DTOP_SELECTARG2))) {
	        /* Switch the arguments if needed... */
	        if (d3dTexStageStateType == D3DTSS_COLOROP) {
		    handle_color_alpha_args(This, dwStage, D3DTSS_COLORARG1,
					    This->state_block.texture_stage_state[dwStage][D3DTSS_COLORARG1 - 1],
					    dwState);
		    handle_color_alpha_args(This, dwStage, D3DTSS_COLORARG2,
					    This->state_block.texture_stage_state[dwStage][D3DTSS_COLORARG2 - 1],
					    dwState);
		} else {
		    handle_color_alpha_args(This, dwStage, D3DTSS_ALPHAARG1,
					    This->state_block.texture_stage_state[dwStage][D3DTSS_ALPHAARG1 - 1],
					    dwState);
		    handle_color_alpha_args(This, dwStage, D3DTSS_ALPHAARG2,
					    This->state_block.texture_stage_state[dwStage][D3DTSS_ALPHAARG2 - 1],
					    dwState);
		}
	    }
	    
	    if (handled) {
	        if (d3dTexStageStateType == D3DTSS_ALPHAOP) {
		    glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, scale);
		} else {
		    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, scale);
		}			
		TRACE(" Stage type is : %s => %s\n", type, value);
	    } else {
	        FIXME(" Unhandled stage type is : %s => %s\n", type, value);
	    }
        } break;

	case D3DTSS_COLORARG1:
	case D3DTSS_COLORARG2:
	case D3DTSS_ALPHAARG1:
	case D3DTSS_ALPHAARG2: {
	    const char *value, *value_comp = "", *value_alpha = "";
	    BOOLEAN handled;
	    D3DTEXTUREOP tex_op;
	    
	    switch (dwState & D3DTA_SELECTMASK) {
#define GEN_CASE(a) case a: value = #a; break
	        GEN_CASE(D3DTA_DIFFUSE);
		GEN_CASE(D3DTA_CURRENT);
		GEN_CASE(D3DTA_TEXTURE);
		GEN_CASE(D3DTA_TFACTOR);
	        GEN_CASE(D3DTA_SPECULAR);
#undef GEN_CASE
	        default: value = "UNKNOWN";
	    }
	    if (dwState & D3DTA_COMPLEMENT) {
	        value_comp = " | D3DTA_COMPLEMENT";
	    }
	    if (dwState & D3DTA_ALPHAREPLICATE) {
	        value_alpha = " | D3DTA_ALPHAREPLICATE";
	    }

	    if ((d3dTexStageStateType == D3DTSS_COLORARG1) || (d3dTexStageStateType == D3DTSS_COLORARG2)) {
	        tex_op = This->state_block.texture_stage_state[dwStage][D3DTSS_COLOROP - 1];
	    } else {
	        tex_op = This->state_block.texture_stage_state[dwStage][D3DTSS_ALPHAOP - 1];
	    }
	    
	    handled = handle_color_alpha_args(This, dwStage, d3dTexStageStateType, dwState, tex_op);
	    
	    if (handled) {
	        TRACE(" Stage type : %s => %s%s%s\n", type, value, value_comp, value_alpha);
	    } else {
	        FIXME(" Unhandled stage type : %s => %s%s%s\n", type, value, value_comp, value_alpha);
	    }
	} break;

	case D3DTSS_MIPMAPLODBIAS: {
	    D3DVALUE value = *((D3DVALUE *) &dwState);
	    BOOLEAN handled = TRUE;
	    
	    if (value != 0.0)
	        handled = FALSE;

	    if (handled) {
	        TRACE(" Stage type : D3DTSS_MIPMAPLODBIAS => %f\n", value);
	    } else {
	        FIXME(" Unhandled stage type : D3DTSS_MIPMAPLODBIAS => %f\n", value);
	    }
	} break;

	case D3DTSS_MAXMIPLEVEL: 
	    if (dwState == 0) {
	        TRACE(" Stage type : D3DTSS_MAXMIPLEVEL => 0 (disabled) \n");
	    } else {
	        FIXME(" Unhandled stage type : D3DTSS_MAXMIPLEVEL => %ld\n", dwState);
	    }
	    break;

	case D3DTSS_BORDERCOLOR: {
	    GLfloat color[4];

	    color[0] = ((dwState >> 16) & 0xFF) / 255.0;
	    color[1] = ((dwState >>  8) & 0xFF) / 255.0;
	    color[2] = ((dwState >>  0) & 0xFF) / 255.0;
	    color[3] = ((dwState >> 24) & 0xFF) / 255.0;

	    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

	    TRACE(" Stage type : D3DTSS_BORDERCOLOR => %02lx %02lx %02lx %02lx (RGBA)\n",
		  ((dwState >> 16) & 0xFF),
		  ((dwState >>  8) & 0xFF),
		  ((dwState >>  0) & 0xFF),
		  ((dwState >> 24) & 0xFF));
	} break;
	    
	case D3DTSS_TEXCOORDINDEX: {
	    BOOLEAN handled = TRUE;
	    const char *value;
	    
	    switch (dwState & 0xFFFF0000) {
#define GEN_CASE(a) case a: value = #a; break
	        GEN_CASE(D3DTSS_TCI_PASSTHRU);
		GEN_CASE(D3DTSS_TCI_CAMERASPACENORMAL);
		GEN_CASE(D3DTSS_TCI_CAMERASPACEPOSITION);
		GEN_CASE(D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
#undef GEN_CASE
		default: value = "UNKNOWN";
	    }
	    if ((dwState & 0xFFFF0000) != D3DTSS_TCI_PASSTHRU)
	        handled = FALSE;

	    if (handled) {
	        TRACE(" Stage type : D3DTSS_TEXCOORDINDEX => %ld | %s\n", dwState & 0x0000FFFF, value);
	    } else {
	        FIXME(" Unhandled stage type : D3DTSS_TEXCOORDINDEX => %ld | %s\n", dwState & 0x0000FFFF, value);
	    }
	} break;
	    
	case D3DTSS_TEXTURETRANSFORMFLAGS: {
	    const char *projected = "", *value;
	    BOOLEAN handled = TRUE;
	    switch (dwState & 0xFF) {
#define GEN_CASE(a) case a: value = #a; break
	        GEN_CASE(D3DTTFF_DISABLE);
		GEN_CASE(D3DTTFF_COUNT1);
		GEN_CASE(D3DTTFF_COUNT2);
		GEN_CASE(D3DTTFF_COUNT3);
		GEN_CASE(D3DTTFF_COUNT4);
#undef GEN_CASE
		default: value = "UNKNOWN";
	    }
	    if (dwState & D3DTTFF_PROJECTED) {
	        projected = " | D3DTTFF_PROJECTED";
		handled = FALSE;
	    }

	    if ((dwState & 0xFF) != D3DTTFF_DISABLE) {
	        This->matrices_updated(This, TEXMAT0_CHANGED << dwStage);
	    }

	    if (handled == TRUE) {
	        TRACE(" Stage type : D3DTSS_TEXTURETRANSFORMFLAGS => %s%s\n", value, projected);
	    } else {
	        FIXME(" Unhandled stage type : D3DTSS_TEXTURETRANSFORMFLAGS => %s%s\n", value, projected);
	    }
	} break;
	    
	default:
	    FIXME(" Unhandled stage type : %s => %08lx\n", type, dwState);
	    break;
    }

    LEAVE_GL();
    
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
	GLint max_mip_level;
	GLfloat color[4];
	
	IDirectDrawSurface7_AddRef(ICOM_INTERFACE(tex_impl, IDirectDrawSurface7)); /* Not sure about this either */

	if (This->current_texture[dwStage] == tex_impl) {
	    /* No need to do anything as the texture did not change. */
	    return DD_OK;
	}
	This->current_texture[dwStage] = tex_impl;
	
	if (This->state_block.texture_stage_state[dwStage][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE) {
	    /* Do not re-enable texturing if it was disabled due to the COLOROP code */
	    glEnable(GL_TEXTURE_2D);
	    TRACE(" enabling 2D texturing.\n");
	}
	gltex_upload_texture(tex_impl);

	if ((tex_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) == 0) {
	    max_mip_level = 0;
	} else {
	    max_mip_level = tex_impl->surface_desc.u2.dwMipMapCount - 1;
	}

	/* Now we need to reset all glTexParameter calls for this particular texture... */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			convert_mag_filter_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_MAGFILTER - 1]));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			convert_min_filter_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_MINFILTER - 1],
						  This->state_block.texture_stage_state[dwStage][D3DTSS_MIPFILTER - 1]));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
			convert_tex_address_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSU - 1]));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
			convert_tex_address_to_GL(This->state_block.texture_stage_state[dwStage][D3DTSS_ADDRESSV - 1]));	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_mip_level);
	color[0] = ((This->state_block.texture_stage_state[dwStage][D3DTSS_BORDERCOLOR - 1] >> 16) & 0xFF) / 255.0;
	color[1] = ((This->state_block.texture_stage_state[dwStage][D3DTSS_BORDERCOLOR - 1] >>  8) & 0xFF) / 255.0;
	color[2] = ((This->state_block.texture_stage_state[dwStage][D3DTSS_BORDERCOLOR - 1] >>  0) & 0xFF) / 255.0;
	color[3] = ((This->state_block.texture_stage_state[dwStage][D3DTSS_BORDERCOLOR - 1] >> 24) & 0xFF) / 255.0;
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
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
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwLightIndex, lpLight);
    
    if (TRACE_ON(ddraw)) {
        TRACE(" setting light : \n");
	dump_D3DLIGHT7(lpLight);
    }
    
    if (dwLightIndex >= MAX_LIGHTS) return DDERR_INVALIDPARAMS;
    This->set_lights |= 0x00000001 << dwLightIndex;
    This->light_parameters[dwLightIndex] = *lpLight;

    /* Some checks to print out nice warnings :-) */
    switch (lpLight->dltType) {
        case D3DLIGHT_DIRECTIONAL:
        case D3DLIGHT_POINT:
            /* These are handled properly... */
            break;
	    
        case D3DLIGHT_SPOT:
            if ((lpLight->dvTheta != 0.0) ||
		(lpLight->dvTheta != lpLight->dvPhi)) {
	        ERR("dvTheta not fully supported yet !\n");
	    }
	    break;

	default:
	    ERR("Light type not handled yet : %08x !\n", lpLight->dltType);
    }
    
    /* This will force the Light setting on next drawing of primitives */
    glThis->transform_state = GL_TRANSFORM_NONE;
    
    return DD_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_LightEnable(LPDIRECT3DDEVICE7 iface,
				     DWORD dwLightIndex,
				     BOOL bEnable)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%08lx,%d)\n", This, iface, dwLightIndex, bEnable);
    
    if (dwLightIndex >= MAX_LIGHTS) return DDERR_INVALIDPARAMS;

    if (bEnable) {
        if (((0x00000001 << dwLightIndex) & This->set_lights) == 0) {
	    /* Set the default parameters.. */
	    TRACE(" setting default light parameters...\n");
	    GL_IDirect3DDeviceImpl_7_SetLight(iface, dwLightIndex, &(This->light_parameters[dwLightIndex]));
	}
	glEnable(GL_LIGHT0 + dwLightIndex);
	if ((This->active_lights & (0x00000001 << dwLightIndex)) == 0) {
	    /* This light gets active... Need to update its parameters to GL before the next drawing */
	    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
	    
	    This->active_lights |= 0x00000001 << dwLightIndex;
	    glThis->transform_state = GL_TRANSFORM_NONE;
	}
    } else {
        glDisable(GL_LIGHT0 + dwLightIndex);
	This->active_lights &= ~(0x00000001 << dwLightIndex);
    }

    return DD_OK;
}

HRESULT  WINAPI  
GL_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface, DWORD dwIndex, CONST D3DVALUE* pPlaneEquation) 
{
    ICOM_THIS(IDirect3DDeviceImpl,iface);
    IDirect3DDeviceGLImpl* glThis = (IDirect3DDeviceGLImpl*) This;

    TRACE("(%p)->(%ld,%p)\n", This, dwIndex, pPlaneEquation);

    if (dwIndex >= This->max_clipping_planes) {
	return DDERR_INVALIDPARAMS;
    }

    TRACE(" clip plane %ld : %f %f %f %f\n", dwIndex, pPlaneEquation[0], pPlaneEquation[1], pPlaneEquation[2], pPlaneEquation[3] );

    memcpy(This->clipping_planes[dwIndex].plane, pPlaneEquation, sizeof(D3DVALUE[4]));
    
    /* This is to force the reset of the transformation matrices on the next drawing.
     * This is needed to use the correct matrices for the various clipping planes.
     */
    glThis->transform_state = GL_TRANSFORM_NONE;
    
    return D3D_OK;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_SetViewport(LPDIRECT3DDEVICE7 iface,
				     LPD3DVIEWPORT7 lpData)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(ddraw)) {
        TRACE(" viewport is : \n");
	TRACE("    - dwX = %ld   dwY = %ld\n",
	      lpData->dwX, lpData->dwY);
	TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	      lpData->dwWidth, lpData->dwHeight);
	TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	      lpData->dvMinZ, lpData->dvMaxZ);
    }
    This->active_viewport = *lpData;

    ENTER_GL();
    
    /* Set the viewport */
    glDepthRange(lpData->dvMinZ, lpData->dvMaxZ);
    glViewport(lpData->dwX,
	       This->surface->surface_desc.dwHeight - (lpData->dwHeight + lpData->dwY),
	       lpData->dwWidth, lpData->dwHeight);

    LEAVE_GL();
    
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
    XCAST(SetTransform) Main_IDirect3DDeviceImpl_7_3T_2T_SetTransform,
    XCAST(GetTransform) Main_IDirect3DDeviceImpl_7_3T_2T_GetTransform,
    XCAST(SetViewport) GL_IDirect3DDeviceImpl_7_SetViewport,
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
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    GLboolean ztest;
    GLfloat old_z_clear_value;
    GLbitfield bitfield = 0;
    GLint old_stencil_clear_value;
    GLfloat old_color_clear_value[4];
    D3DRECT rect;
    int i;
    
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

    if (dwCount == 0) {
        /* Not sure if this is really needed... */
        dwCount = 1;
	rect.u1.x1 = 0;
	rect.u2.y1 = 0;
	rect.u3.x2 = This->surface->surface_desc.dwWidth;
	rect.u4.y2 = This->surface->surface_desc.dwHeight;
	lpRects = &rect;
    }
    
    /* Clears the screen */
    ENTER_GL();

    if (glThis->state == SURFACE_MEMORY_DIRTY) {
        /* TODO: optimize here the case where Clear changes all the screen... */
        This->flush_to_framebuffer(This, NULL, glThis->lock_surf);
    }
    glThis->state = SURFACE_GL;

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

    glEnable(GL_SCISSOR_TEST); 
    for (i = 0; i < dwCount; i++) {
        glScissor(lpRects[i].u1.x1, This->surface->surface_desc.dwHeight - lpRects[i].u4.y2,
		  lpRects[i].u3.x2 - lpRects[i].u1.x1, lpRects[i].u4.y2 - lpRects[i].u2.y1);
        glClear(bitfield);
    }
    glDisable(GL_SCISSOR_TEST); 
    
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
	D3DRECT rect;
	
        TRACE(" executing D3D Device override.\n");

	if (rdst) {
	    rect.u1.x1 = rdst->left;
	    rect.u2.y1 = rdst->top;
	    rect.u3.x2 = rdst->right;
	    rect.u4.y2 = rdst->bottom;
	}
	d3ddevice_clear(This->d3ddevice, rdst != NULL ? 1 : 0, &rect, D3DCLEAR_TARGET, color, 0.0, 0x00000000);
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
    /* See the OpenGL Red Book for an explanation of the following translation (in the OpenGL
       Correctness Tips section).
       
       Basically, from what I understood, if the game does not filter the font texture,
       as the 'real' pixel will lie at the middle of the two texels, OpenGL may choose the wrong
       one and we will have strange artifacts (as the rounding and stuff may give different results
       for different pixels, ie sometimes take the left pixel, sometimes the right).
    */
    glTranslatef(0.375, 0.375, 0);
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

	/* Now also re-loads all the Lights and Clipping Planes using the new matrices */
	if (This->state_block.render_state[D3DRENDERSTATE_CLIPPING - 1] != FALSE) {
	    GLint i;
	    DWORD runner;
	    for (i = 0, runner = 0x00000001; i < This->max_clipping_planes; i++, runner <<= 1) {
	        if (runner & This->state_block.render_state[D3DRENDERSTATE_CLIPPLANEENABLE - 1]) {
		    GLdouble plane[4];

		    plane[0] = This->clipping_planes[i].plane[0];
		    plane[1] = This->clipping_planes[i].plane[1];
		    plane[2] = This->clipping_planes[i].plane[2];
		    plane[3] = This->clipping_planes[i].plane[3];
		    
		    glClipPlane( GL_CLIP_PLANE0 + i, (const GLdouble*) (&plane) );
		}
	    }
	}
	if (This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] != FALSE) {
	    GLint i;
	    DWORD runner;

	    for (i = 0, runner = 0x00000001; i < MAX_LIGHTS; i++, runner <<= 1) {
	        if (runner & This->active_lights) {
		    switch (This->light_parameters[i].dltType) {
		        case D3DLIGHT_DIRECTIONAL: {
			    float direction[4];
			    float cut_off = 180.0;
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &(This->light_parameters[i].dcvAmbient));
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &(This->light_parameters[i].dcvDiffuse));
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &(This->light_parameters[i].dcvSpecular));
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			    
			    direction[0] = This->light_parameters[i].dvDirection.u1.x;
			    direction[1] = This->light_parameters[i].dvDirection.u2.y;
			    direction[2] = This->light_parameters[i].dvDirection.u3.z;
			    direction[3] = 0.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) direction);
			} break;

			case D3DLIGHT_POINT: {
			    float position[4];
			    float cut_off = 180.0;
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &(This->light_parameters[i].dcvAmbient));
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &(This->light_parameters[i].dcvDiffuse));
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &(This->light_parameters[i].dcvSpecular));
			    position[0] = This->light_parameters[i].dvPosition.u1.x;
			    position[1] = This->light_parameters[i].dvPosition.u2.y;
			    position[2] = This->light_parameters[i].dvPosition.u3.z;
			    position[3] = 1.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) position);
			    glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, &(This->light_parameters[i].dvAttenuation0));
			    glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, &(This->light_parameters[i].dvAttenuation1));
			    glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, &(This->light_parameters[i].dvAttenuation2));
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			} break;

			case D3DLIGHT_SPOT: {
			    float direction[4];
			    float position[4];
			    float cut_off = 90.0 * (This->light_parameters[i].dvPhi / M_PI);
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &(This->light_parameters[i].dcvAmbient));
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &(This->light_parameters[i].dcvDiffuse));
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &(This->light_parameters[i].dcvSpecular));
			    
			    direction[0] = This->light_parameters[i].dvDirection.u1.x;
			    direction[1] = This->light_parameters[i].dvDirection.u2.y;
			    direction[2] = This->light_parameters[i].dvDirection.u3.z;
			    direction[3] = 0.0;
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, (float *) direction);
			    position[0] = This->light_parameters[i].dvPosition.u1.x;
			    position[1] = This->light_parameters[i].dvPosition.u2.y;
			    position[2] = This->light_parameters[i].dvPosition.u3.z;
			    position[3] = 1.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) position);
			    glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, &(This->light_parameters[i].dvAttenuation0));
			    glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, &(This->light_parameters[i].dvAttenuation1));
			    glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, &(This->light_parameters[i].dvAttenuation2));
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_EXPONENT, &(This->light_parameters[i].dvFalloff));
			} break;

			default:
			    /* No warning here as it's already done at light setting */
			    break;
		    }
		}
	    }
	}
	
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
    DWORD tex_mat, tex_stage;
    if ((matrices & (VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED)) != 0) {
        if (glThis->transform_state == GL_TRANSFORM_NORMAL) {
	    /* This will force an update of the transform state at the next drawing. */
	    glThis->transform_state = GL_TRANSFORM_NONE;
	}
    }
    if (matrices & (TEXMAT0_CHANGED|TEXMAT1_CHANGED|TEXMAT2_CHANGED|TEXMAT3_CHANGED|
		    TEXMAT4_CHANGED|TEXMAT5_CHANGED|TEXMAT6_CHANGED|TEXMAT7_CHANGED))
    {
        ENTER_GL();
	for (tex_mat = TEXMAT0_CHANGED, tex_stage = 0; tex_mat <= TEXMAT7_CHANGED; tex_mat <<= 1, tex_stage++) {
	    if (matrices & tex_mat) {
	        if (This->state_block.texture_stage_state[tex_stage][D3DTSS_TEXTURETRANSFORMFLAGS - 1] != D3DTTFF_DISABLE) {
		    if (tex_stage == 0) {
		        /* No multi-texturing support for now ... */
		        glMatrixMode(GL_TEXTURE);
			glLoadMatrixf((float *) This->tex_mat[tex_stage]);
		    }
		} else {
		    glMatrixMode(GL_TEXTURE);
		    glLoadIdentity();
		}
	    }
	}
	LEAVE_GL();
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
    IDirect3DDeviceImpl *d3d_dev = This->d3ddevice;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
    BOOLEAN is_front;
    
    if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
        is_front = TRUE;
	if ((gl_d3d_dev->front_state != SURFACE_GL) &&
	    (gl_d3d_dev->front_lock_surf != This)) {
	    ERR("Change of front buffer.. Expect graphic corruptions !\n");
	}
	gl_d3d_dev->front_lock_surf = This;
    } else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
        is_front = FALSE;
	if ((gl_d3d_dev->state != SURFACE_GL) &&
	    (gl_d3d_dev->lock_surf != This)) {
	    ERR("Change of back buffer.. Expect graphic corruptions !\n");
	}
	gl_d3d_dev->lock_surf = This;
    } else {
        ERR("Wrong surface type for locking !\n");
	return;
    }

    /* Try to acquire the device critical section */
    EnterCriticalSection(&(d3d_dev->crit));
    
    if (((is_front == TRUE)  && (gl_d3d_dev->front_state == SURFACE_GL)) ||
	((is_front == FALSE) && (gl_d3d_dev->state       == SURFACE_GL))) {
        /* If the surface is already in memory, no need to do anything here... */
        GLenum buffer_type;
	GLenum buffer_color;
	RECT loc_rect;
	int y;
	char *dst;

	TRACE(" copying frame buffer to main memory.\n");
	
	/* Note that here we cannot do 'optmizations' about the WriteOnly flag... Indeed, a game
	   may only write to the device... But when we will blit it back to the screen, we need
	   also to blit correctly the parts the application did not overwrite... */
	
	if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) &&
	    (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask ==        0xF800) &&
	    (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask ==        0x07E0) &&
	    (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask ==        0x001F) &&
	    (This->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000)) {
	    buffer_type = GL_UNSIGNED_SHORT_5_6_5;
	    buffer_color = GL_RGB;
	} else if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) &&
		   (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask ==        0x00FF0000) &&
		   (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask ==        0x0000FF00) &&
		   (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask ==        0x000000FF) &&
		   (This->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000)) {
	    buffer_type = GL_UNSIGNED_BYTE;
	    buffer_color = GL_BGRA;
	    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
	} else {
	    ERR(" unsupported pixel format at device locking.\n");
	    return;
	}

	ENTER_GL();
	
	if (is_front == TRUE)
	    /* Application wants to lock the front buffer */
	    glReadBuffer(GL_FRONT);
	else 
	    /* Application wants to lock the back buffer */
	    glReadBuffer(GL_BACK);

	/* Just a hack while waiting for proper rectangle support */
	pRect = NULL;
	if (pRect == NULL) {
	    loc_rect.top = 0;
	    loc_rect.left = 0;
	    loc_rect.bottom = This->surface_desc.dwHeight;
	    loc_rect.right = This->surface_desc.dwWidth;
	} else {
	    loc_rect = *pRect;
	}
	
	dst = ((char *)This->surface_desc.lpSurface) +
	  (loc_rect.top * This->surface_desc.u1.lPitch) + (loc_rect.left * GET_BPP(This->surface_desc));
	for (y = (This->surface_desc.dwHeight - loc_rect.top - 1);
	     y >= ((int) This->surface_desc.dwHeight - (int) loc_rect.bottom);
	     y--) {
	    glReadPixels(loc_rect.left, y,
			 loc_rect.right - loc_rect.left, 1,
			 buffer_color, buffer_type, dst);
	    dst += This->surface_desc.u1.lPitch;
	}

	glPixelStorei(GL_PACK_SWAP_BYTES, FALSE);
	
	if (is_front)
	    gl_d3d_dev->front_state = SURFACE_MEMORY;
	else
	    gl_d3d_dev->state = SURFACE_MEMORY;
	
#if 0
	/* I keep this code here as it's very useful to debug :-) */
	{
	    static int flush_count = 0;
	    char buf[128];
	    FILE *f;
	    
	    if ((++flush_count % 50) == 0) {
	        sprintf(buf, "lock_%06d.pnm", flush_count);
		f = fopen(buf, "wb");
		DDRAW_dump_surface_to_disk(This, f);
	    }
	}
#endif
	
	LEAVE_GL();
    }
}

#define UNLOCK_TEX_SIZE 256

static void d3ddevice_flush_to_frame_buffer(IDirect3DDeviceImpl *d3d_dev, LPCRECT pRect, IDirectDrawSurfaceImpl *surf) {
    GLenum buffer_type, buffer_color;
    RECT loc_rect;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
    GLint depth_test, alpha_test, cull_face, lighting, min_tex, max_tex, tex_env, blend, stencil_test, fog;
    GLuint initial_texture;
    GLint tex_state;
    int x, y;

    /* Note : no need here to lock the 'device critical section' as we are already protected by
       the GL critical section. */
    
    loc_rect.top = 0;
    loc_rect.left = 0;
    loc_rect.bottom = surf->surface_desc.dwHeight;
    loc_rect.right = surf->surface_desc.dwWidth;

    TRACE(" flushing memory back to the frame-buffer (%ld,%ld) x (%ld,%ld).\n", loc_rect.top, loc_rect.left, loc_rect.right, loc_rect.bottom);
	
    glGetIntegerv(GL_DEPTH_TEST, &depth_test);
    glGetIntegerv(GL_ALPHA_TEST, &alpha_test);
    glGetIntegerv(GL_STENCIL_TEST, &stencil_test);
    glGetIntegerv(GL_CULL_FACE, &cull_face);
    glGetIntegerv(GL_LIGHTING, &lighting);
    glGetIntegerv(GL_BLEND, &blend);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &initial_texture);
    glGetIntegerv(GL_TEXTURE_2D, &tex_state);
    glGetIntegerv(GL_FOG, &fog);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &max_tex);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_tex);
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &tex_env);
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    /* TODO: scissor test if ever we use it ! */
    
    if ((surf->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) &&
	(surf->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask ==        0xF800) &&
	(surf->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask ==        0x07E0) &&
	(surf->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask ==        0x001F) &&
	(surf->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000)) {
        buffer_type = GL_UNSIGNED_SHORT_5_6_5;
	buffer_color = GL_RGB;
    } else if ((surf->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) &&
	       (surf->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask ==        0x00FF0000) &&
	       (surf->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask ==        0x0000FF00) &&
	       (surf->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask ==        0x000000FF) &&
	       (surf->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000)) {
        buffer_type = GL_UNSIGNED_BYTE;
	buffer_color = GL_BGRA;
	glPixelStorei(GL_UNPACK_SWAP_BYTES, TRUE);
    } else {
        ERR(" unsupported pixel format at frame buffer flush.\n");
	return;
    }

    gl_d3d_dev->transform_state = GL_TRANSFORM_ORTHO;
    d3ddevice_set_ortho(d3d_dev);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_SCISSOR_TEST); 
    glDepthRange(0.0, 1.0);
    glViewport(0, 0, d3d_dev->surface->surface_desc.dwWidth, d3d_dev->surface->surface_desc.dwHeight);
    glScissor(loc_rect.left, surf->surface_desc.dwHeight - loc_rect.bottom,
	      loc_rect.right - loc_rect.left, loc_rect.bottom - loc_rect.top);
    
    if (gl_d3d_dev->unlock_tex == 0) {
        glGenTextures(1, &gl_d3d_dev->unlock_tex);
	glBindTexture(GL_TEXTURE_2D, gl_d3d_dev->unlock_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
		     UNLOCK_TEX_SIZE, UNLOCK_TEX_SIZE, 0,
		     GL_RGB, buffer_type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glBindTexture(GL_TEXTURE_2D, gl_d3d_dev->unlock_tex);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, surf->surface_desc.dwWidth);
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);

    for (x = loc_rect.left; x < loc_rect.right; x += UNLOCK_TEX_SIZE) {
        for (y = loc_rect.top; y < loc_rect.bottom; y += UNLOCK_TEX_SIZE) {
	    /* First, upload the texture... */
	    int w = (x + UNLOCK_TEX_SIZE > surf->surface_desc.dwWidth)  ? (surf->surface_desc.dwWidth - x)  : UNLOCK_TEX_SIZE;
	    int h = (y + UNLOCK_TEX_SIZE > surf->surface_desc.dwHeight) ? (surf->surface_desc.dwHeight - y) : UNLOCK_TEX_SIZE;
	    glTexSubImage2D(GL_TEXTURE_2D,
			    0,
			    0, 0,
			    w, h,
			    buffer_color,
			    buffer_type,
			    ((char *) surf->surface_desc.lpSurface) + (x * GET_BPP(surf->surface_desc)) + (y * surf->surface_desc.u1.lPitch));
	    glBegin(GL_QUADS);
	    glColor3ub(0xFF, 0xFF, 0xFF);
	    glTexCoord2f(0.0, 0.0);
	    glVertex3d(x, y, 0.5);
	    glTexCoord2f(1.0, 0.0);
	    glVertex3d(x + UNLOCK_TEX_SIZE, y, 0.5);
	    glTexCoord2f(1.0, 1.0);
	    glVertex3d(x + UNLOCK_TEX_SIZE, y + UNLOCK_TEX_SIZE, 0.5);
	    glTexCoord2f(0.0, 1.0);
	    glVertex3d(x, y + UNLOCK_TEX_SIZE, 0.5);
	    glEnd();
	}
    }
    
    
    /* And restore all the various states modified by this code */
    if (depth_test != 0) glEnable(GL_DEPTH_TEST);
    if (lighting != 0) glEnable(GL_LIGHTING);
    if (alpha_test != 0) glEnable(GL_ALPHA_TEST);
    if (stencil_test != 0) glEnable(GL_STENCIL_TEST);
    if (cull_face != 0) glEnable(GL_CULL_FACE);
    if (blend != 0) glEnable(GL_BLEND);
    if (fog != 0) glEnable(GL_FOG);
    glBindTexture(GL_TEXTURE_2D, initial_texture);
    if (tex_state == 0) glDisable(GL_TEXTURE_2D);
    glDisable(GL_SCISSOR_TEST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, FALSE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, tex_env);
    glDepthRange(d3d_dev->active_viewport.dvMinZ, d3d_dev->active_viewport.dvMaxZ);
    glViewport(d3d_dev->active_viewport.dwX,
	       d3d_dev->surface->surface_desc.dwHeight - (d3d_dev->active_viewport.dwHeight + d3d_dev->active_viewport.dwY),
	       d3d_dev->active_viewport.dwWidth, d3d_dev->active_viewport.dwHeight);
    d3d_dev->matrices_updated(d3d_dev, TEXMAT0_CHANGED);
#if 0
    /* I keep this code here as it's very useful to debug :-) */
    {
        static int flush_count = 0;
	char buf[128];
	FILE *f;

	if ((++flush_count % 50) == 0) {
	    sprintf(buf, "flush_%06d.pnm", flush_count);
	    f = fopen(buf, "wb");
	    DDRAW_dump_surface_to_disk(surf, f);
	}
    }
#endif
}

static void d3ddevice_unlock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect)
{
    BOOLEAN is_front;
    IDirect3DDeviceImpl *d3d_dev = This->d3ddevice;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
  
    if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
        is_front = TRUE;
    } else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
        is_front = FALSE;
    } else {
        ERR("Wrong surface type for locking !\n");
	return;
    }
    /* First, check if we need to do anything. For the backbuffer, flushing is done at the next 3D activity. */
    if ((This->lastlocktype & DDLOCK_READONLY) == 0) {
        if (is_front == TRUE) {
	    GLenum prev_draw;
	    ENTER_GL();
	    glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
	    glDrawBuffer(GL_FRONT);
	    d3d_dev->flush_to_framebuffer(d3d_dev, pRect, gl_d3d_dev->front_lock_surf);
	    glDrawBuffer(prev_draw);
	    LEAVE_GL();
	} else {
	    gl_d3d_dev->state = SURFACE_MEMORY_DIRTY;
	}
    }

    /* And 'frees' the device critical section */
    LeaveCriticalSection(&(d3d_dev->crit));
}

static void
apply_texture_state(IDirect3DDeviceImpl *This)
{
    int stage, state;
    
    /* Initialize texture stages states */
    for (stage = 0; stage < MAX_TEXTURES; stage++) {
       for (state = 0; state < HIGHEST_TEXTURE_STAGE_STATE; state += 1) {
	   if (This->state_block.set_flags.texture_stage_state[stage][state] == TRUE) {
	       IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
						     stage, state + 1, This->state_block.texture_stage_state[stage][state]);
	   }
       }
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
    int tex_num;
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
    object->flush_to_framebuffer = d3ddevice_flush_to_frame_buffer;
    
    InitializeCriticalSection(&(object->crit));
    
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
            surf->aux_ctx  = (LPVOID) object;
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
    
    for (surf = surface; surf != NULL; surf = surf->surface_owner) {
        IDirectDrawSurfaceImpl *surf2;
	for (surf2 = surf; surf2->prev_attached != NULL; surf2 = surf2->prev_attached) ;
	for (; surf2 != NULL; surf2 = surf2->next_attached) {
	    TRACE(" checking surface %p :", surf2);
	    if (((surf2->surface_desc.ddsCaps.dwCaps & (DDSCAPS_3DDEVICE)) == (DDSCAPS_3DDEVICE)) &&
		((surf2->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER)) != (DDSCAPS_ZBUFFER))) {
	        /* Override the Lock / Unlock function for all these surfaces */
	        surf2->lock_update_prev = surf2->lock_update;
	        surf2->lock_update = d3ddevice_lock_update;
		surf2->unlock_update_prev = surf2->unlock_update;
		surf2->unlock_update = d3ddevice_unlock_update;
		/* And install also the blt / bltfast overrides */
		surf2->aux_blt = d3ddevice_blt;
		surf2->aux_bltfast = d3ddevice_bltfast;
		
		TRACE(" overiding direct surface access.\n");
	    } else {
	        TRACE(" no overide.\n");
	    }
	    surf2->d3ddevice = object;
	}
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
    for (tex_num = 0; tex_num < MAX_TEXTURES; tex_num++) {
        object->tex_mat[tex_num] = (D3DMATRIX *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
	memcpy(object->tex_mat[tex_num], id_mat, 16 * sizeof(float));
    }
    
    /* Initialisation */
    TRACE(" setting current context\n");
    object->set_context(object);
    TRACE(" current context set\n");

    /* allocate the clipping planes */
    glGetIntegerv(GL_MAX_CLIP_PLANES,&max_clipping_planes);
    if (max_clipping_planes>32) {
	object->max_clipping_planes=32;
    } else {
	object->max_clipping_planes = max_clipping_planes;
    }
    TRACE(" capable of %d clipping planes\n", (int)object->max_clipping_planes );
    object->clipping_planes = (d3d7clippingplane*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->max_clipping_planes * sizeof(d3d7clippingplane));

    glHint(GL_FOG_HINT,GL_NICEST);
    
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawBuffer(buffer);
    glReadBuffer(buffer);
    /* glDisable(GL_DEPTH_TEST); Need here to check for the presence of a ZBuffer and to reenable it when the ZBuffer is attached */
    LEAVE_GL();

    gl_object->state = SURFACE_GL;
    
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
    /* Apply default render state and texture stage state values */
    apply_render_state(object, &object->state_block);
    apply_texture_state(object);

    /* And fill the fog table with the default fog value */
    build_fog_table(gl_object->fog_table, object->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1]);
    
    return DD_OK;
}
