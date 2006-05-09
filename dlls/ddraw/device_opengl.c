/*
 * Direct3D Device
 *
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
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
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include <math.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "d3d_private.h"
#include "opengl_private.h"

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

const float id_mat[16] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

/* This is filled at DLL loading time */
static D3DDEVICEDESC7 opengl_device_caps;
GL_EXTENSIONS_LIST GL_extensions;

static void draw_primitive_strided(IDirect3DDeviceImpl *This,
				   D3DPRIMITIVETYPE d3dptPrimitiveType,
				   DWORD d3dvtVertexType,
				   LPD3DDRAWPRIMITIVESTRIDEDDATA lpD3DDrawPrimStrideData,
				   DWORD dwVertexCount,
				   LPWORD dwIndices,
				   DWORD dwIndexCount,
				   DWORD dwFlags) ;

static DWORD draw_primitive_handle_textures(IDirect3DDeviceImpl *This);

/* retrieve the X display to use on a given DC */
inline static Display *get_display( HDC hdc )
{
    Display *display;
    enum x11drv_escape_codes escape = X11DRV_GET_DISPLAY;

    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(display), (LPSTR)&display )) display = NULL;

    return display;
}

#define UNLOCK_TEX_SIZE 256

#define DEPTH_RANGE_BIT (0x00000001 << 0)
#define VIEWPORT_BIT    (0x00000001 << 1)

static DWORD d3ddevice_set_state_for_flush(IDirect3DDeviceImpl *d3d_dev, LPCRECT pRect, BOOLEAN use_alpha, BOOLEAN *initial) {
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
    DWORD opt_bitmap = 0x00000000;
    
    if ((gl_d3d_dev->current_bound_texture[1] != NULL) &&
	((d3d_dev->state_block.texture_stage_state[1][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE))) {
	if (gl_d3d_dev->current_active_tex_unit != GL_TEXTURE1_WINE) {
	    GL_extensions.glActiveTexture(GL_TEXTURE1_WINE);
	    gl_d3d_dev->current_active_tex_unit = GL_TEXTURE1_WINE;
	}
	/* Disable multi-texturing for level 1 to disable all others */
	glDisable(GL_TEXTURE_2D);
    }
    if (gl_d3d_dev->current_active_tex_unit != GL_TEXTURE0_WINE) {
	GL_extensions.glActiveTexture(GL_TEXTURE0_WINE);
	gl_d3d_dev->current_active_tex_unit = GL_TEXTURE0_WINE;
    }
    if ((gl_d3d_dev->current_bound_texture[0] == NULL) ||
	(d3d_dev->state_block.texture_stage_state[0][D3DTSS_COLOROP - 1] == D3DTOP_DISABLE))
	glEnable(GL_TEXTURE_2D);
    if (gl_d3d_dev->unlock_tex == 0) {
        glGenTextures(1, &gl_d3d_dev->unlock_tex);
	glBindTexture(GL_TEXTURE_2D, gl_d3d_dev->unlock_tex);
	*initial = TRUE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    } else {
        glBindTexture(GL_TEXTURE_2D, gl_d3d_dev->unlock_tex);
	*initial = FALSE;
    }
    if (d3d_dev->tex_mat_is_identity[0] == FALSE) {
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
    }
    
    if (gl_d3d_dev->transform_state != GL_TRANSFORM_ORTHO) {
	gl_d3d_dev->transform_state = GL_TRANSFORM_ORTHO;
	d3ddevice_set_ortho(d3d_dev);
    }
    
    if (gl_d3d_dev->depth_test != FALSE) glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    if ((d3d_dev->active_viewport.dvMinZ != 0.0) ||
	(d3d_dev->active_viewport.dvMaxZ != 1.0)) {
	glDepthRange(0.0, 1.0);
	opt_bitmap |= DEPTH_RANGE_BIT;
    }
    if ((d3d_dev->active_viewport.dwX != 0) ||
	(d3d_dev->active_viewport.dwY != 0) ||
	(d3d_dev->active_viewport.dwWidth != d3d_dev->surface->surface_desc.dwWidth) ||
	(d3d_dev->active_viewport.dwHeight != d3d_dev->surface->surface_desc.dwHeight)) {
	glViewport(0, 0, d3d_dev->surface->surface_desc.dwWidth, d3d_dev->surface->surface_desc.dwHeight);
	opt_bitmap |= VIEWPORT_BIT;
    }
    glScissor(pRect->left, d3d_dev->surface->surface_desc.dwHeight - pRect->bottom,
	      pRect->right - pRect->left, pRect->bottom - pRect->top);
    if (gl_d3d_dev->lighting != FALSE) glDisable(GL_LIGHTING);
    if (gl_d3d_dev->cull_face != FALSE) glDisable(GL_CULL_FACE);
    if (use_alpha) {
	if (gl_d3d_dev->alpha_test == FALSE) glEnable(GL_ALPHA_TEST);
	if ((gl_d3d_dev->current_alpha_test_func != GL_NOTEQUAL) || (gl_d3d_dev->current_alpha_test_ref != 0.0))
	    glAlphaFunc(GL_NOTEQUAL, 0.0);
    } else {
	if (gl_d3d_dev->alpha_test != FALSE) glDisable(GL_ALPHA_TEST);
    }
    if (gl_d3d_dev->stencil_test != FALSE) glDisable(GL_STENCIL_TEST);
    if (gl_d3d_dev->blending != FALSE) glDisable(GL_BLEND);
    if (gl_d3d_dev->fogging != FALSE) glDisable(GL_FOG);
    if (gl_d3d_dev->current_tex_env != GL_REPLACE)
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    return opt_bitmap;
}

static void d3ddevice_restore_state_after_flush(IDirect3DDeviceImpl *d3d_dev, DWORD opt_bitmap, BOOLEAN use_alpha) {
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;

    /* And restore all the various states modified by this code */
    if (gl_d3d_dev->depth_test != 0) glEnable(GL_DEPTH_TEST);
    if (gl_d3d_dev->lighting != 0) glEnable(GL_LIGHTING);
    if ((gl_d3d_dev->alpha_test != 0) && (use_alpha == 0))
	glEnable(GL_ALPHA_TEST);
    else if ((gl_d3d_dev->alpha_test == 0) && (use_alpha != 0))
	glDisable(GL_ALPHA_TEST);
    if (use_alpha) {
	if ((gl_d3d_dev->current_alpha_test_func != GL_NOTEQUAL) || (gl_d3d_dev->current_alpha_test_ref != 0.0))
	    glAlphaFunc(gl_d3d_dev->current_alpha_test_func, gl_d3d_dev->current_alpha_test_ref);
    }
    if (gl_d3d_dev->stencil_test != 0) glEnable(GL_STENCIL_TEST);
    if (gl_d3d_dev->cull_face != 0) glEnable(GL_CULL_FACE);
    if (gl_d3d_dev->blending != 0) glEnable(GL_BLEND);
    if (gl_d3d_dev->fogging != 0) glEnable(GL_FOG);
    glDisable(GL_SCISSOR_TEST);
    if (opt_bitmap & DEPTH_RANGE_BIT) {
	glDepthRange(d3d_dev->active_viewport.dvMinZ, d3d_dev->active_viewport.dvMaxZ);
    }
    if (opt_bitmap & VIEWPORT_BIT) {
	glViewport(d3d_dev->active_viewport.dwX,
		   d3d_dev->surface->surface_desc.dwHeight - (d3d_dev->active_viewport.dwHeight + d3d_dev->active_viewport.dwY),
		   d3d_dev->active_viewport.dwWidth, d3d_dev->active_viewport.dwHeight);
    }
    if (d3d_dev->tex_mat_is_identity[0] == FALSE) {
	d3d_dev->matrices_updated(d3d_dev, TEXMAT0_CHANGED);
    }
    
    if (gl_d3d_dev->current_active_tex_unit != GL_TEXTURE0_WINE) {
	GL_extensions.glActiveTexture(GL_TEXTURE0_WINE);
	gl_d3d_dev->current_active_tex_unit = GL_TEXTURE0_WINE;
    }
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, gl_d3d_dev->current_tex_env);
    /* Note that here we could directly re-bind the previous texture... But it would in some case be a spurious
       bind if ever the game changes the texture just after.

       So choose 0x00000001 to postpone the binding to the next time we draw something on screen. */
    gl_d3d_dev->current_bound_texture[0] = (IDirectDrawSurfaceImpl *) 0x00000001;
    if (d3d_dev->state_block.texture_stage_state[0][D3DTSS_COLOROP - 1] == D3DTOP_DISABLE) glDisable(GL_TEXTURE_2D);

    /* And re-enabled if needed texture level 1 */
    if ((gl_d3d_dev->current_bound_texture[1] != NULL) &&
	(d3d_dev->state_block.texture_stage_state[1][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE)) {
	if (gl_d3d_dev->current_active_tex_unit != GL_TEXTURE1_WINE) {
	    GL_extensions.glActiveTexture(GL_TEXTURE1_WINE);
	    gl_d3d_dev->current_active_tex_unit = GL_TEXTURE1_WINE;
	}
	glEnable(GL_TEXTURE_2D);
    }
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
    if (gl_d3d_dev->state[WINE_GL_BUFFER_BACK] == SURFACE_MEMORY_DIRTY) {
        d3d_dev->flush_to_framebuffer(d3d_dev, &(gl_d3d_dev->lock_rect[WINE_GL_BUFFER_BACK]), gl_d3d_dev->lock_surf[WINE_GL_BUFFER_BACK]);
    }
    gl_d3d_dev->state[WINE_GL_BUFFER_BACK] = SURFACE_GL;
    gl_d3d_dev->state[WINE_GL_BUFFER_FRONT] = SURFACE_GL;
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
   
    TRACE("glxMakeCurrent %p, %ld, %p\n",glThis->display,glThis->drawable, glThis->gl_context);
    ENTER_GL();
    if (glXMakeCurrent(glThis->display, glThis->drawable, glThis->gl_context) == False) {
	ERR("Error in setting current context (context %p drawable %ld)!\n",
	    glThis->gl_context, glThis->drawable);
    }
    LEAVE_GL();
}

static void fill_opengl_caps(D3DDEVICEDESC *d1)
{
    d1->dwSize  = sizeof(*d1);
    d1->dwFlags = D3DDD_COLORMODEL | D3DDD_DEVCAPS | D3DDD_TRANSFORMCAPS | D3DDD_BCLIPPING | D3DDD_LIGHTINGCAPS |
	D3DDD_LINECAPS | D3DDD_TRICAPS | D3DDD_DEVICERENDERBITDEPTH | D3DDD_DEVICEZBUFFERBITDEPTH |
	D3DDD_MAXBUFFERSIZE | D3DDD_MAXVERTEXCOUNT;
    d1->dcmColorModel = D3DCOLOR_RGB;
    d1->dwDevCaps = opengl_device_caps.dwDevCaps;
    d1->dtcTransformCaps.dwSize = sizeof(D3DTRANSFORMCAPS);
    d1->dtcTransformCaps.dwCaps = D3DTRANSFORMCAPS_CLIP;
    d1->bClipping = TRUE;
    d1->dlcLightingCaps.dwSize = sizeof(D3DLIGHTINGCAPS);
    d1->dlcLightingCaps.dwCaps = D3DLIGHTCAPS_DIRECTIONAL | D3DLIGHTCAPS_PARALLELPOINT | D3DLIGHTCAPS_POINT | D3DLIGHTCAPS_SPOT;
    d1->dlcLightingCaps.dwLightingModel = D3DLIGHTINGMODEL_RGB;
    d1->dlcLightingCaps.dwNumLights = opengl_device_caps.dwMaxActiveLights;
    d1->dpcLineCaps = opengl_device_caps.dpcLineCaps;
    d1->dpcTriCaps = opengl_device_caps.dpcTriCaps;
    d1->dwDeviceRenderBitDepth  = opengl_device_caps.dwDeviceRenderBitDepth;
    d1->dwDeviceZBufferBitDepth = opengl_device_caps.dwDeviceZBufferBitDepth;
    d1->dwMaxBufferSize = 0;
    d1->dwMaxVertexCount = 65536;
    d1->dwMinTextureWidth  = opengl_device_caps.dwMinTextureWidth;
    d1->dwMinTextureHeight = opengl_device_caps.dwMinTextureHeight;
    d1->dwMaxTextureWidth  = opengl_device_caps.dwMaxTextureWidth;
    d1->dwMaxTextureHeight = opengl_device_caps.dwMaxTextureHeight;
    d1->dwMinStippleWidth  = 1;
    d1->dwMinStippleHeight = 1;
    d1->dwMaxStippleWidth  = 32;
    d1->dwMaxStippleHeight = 32;
    d1->dwMaxTextureRepeat = opengl_device_caps.dwMaxTextureRepeat;
    d1->dwMaxTextureAspectRatio = opengl_device_caps.dwMaxTextureAspectRatio;
    d1->dwMaxAnisotropy = opengl_device_caps.dwMaxAnisotropy;
    d1->dvGuardBandLeft = opengl_device_caps.dvGuardBandLeft;
    d1->dvGuardBandRight = opengl_device_caps.dvGuardBandRight;
    d1->dvGuardBandTop = opengl_device_caps.dvGuardBandTop;
    d1->dvGuardBandBottom = opengl_device_caps.dvGuardBandBottom;
    d1->dvExtentsAdjust = opengl_device_caps.dvExtentsAdjust;
    d1->dwStencilCaps = opengl_device_caps.dwStencilCaps;
    d1->dwFVFCaps = opengl_device_caps.dwFVFCaps;
    d1->dwTextureOpCaps = opengl_device_caps.dwTextureOpCaps;
    d1->wMaxTextureBlendStages = opengl_device_caps.wMaxTextureBlendStages;
    d1->wMaxSimultaneousTextures = opengl_device_caps.wMaxSimultaneousTextures;
}

static void fill_opengl_caps_7(D3DDEVICEDESC7 *d)
{
    *d = opengl_device_caps;
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
	char interface_name[] = "WINE Reference Direct3DX using OpenGL";
        TRACE(" enumerating OpenGL D3DDevice interface using reference IID (IID %s).\n", debugstr_guid(&IID_IDirect3DRefDevice));
	d1 = dref;
	d2 = dref;
	ret_value = cb((LPIID) &IID_IDirect3DRefDevice, interface_name, device_name, &d1, &d2, context);
	if (ret_value != D3DENUMRET_OK)
	    return ret_value;
    }

    {
	char interface_name[] = "WINE Direct3DX using OpenGL";
	TRACE(" enumerating OpenGL D3DDevice interface (IID %s).\n", debugstr_guid(&IID_D3DDEVICE_OpenGL));
	d1 = dref;
	d2 = dref;
	ret_value = cb((LPIID) &IID_D3DDEVICE_OpenGL, interface_name, device_name, &d1, &d2, context);
	if (ret_value != D3DENUMRET_OK)
	    return ret_value;
    }

    return D3DENUMRET_OK;
}

HRESULT d3ddevice_enumerate7(LPD3DENUMDEVICESCALLBACK7 cb, LPVOID context)
{
    D3DDEVICEDESC7 ddesc;
    char interface_name[] = "WINE Direct3D7 using OpenGL";
    char device_name[] = "Wine D3D7 device";

    fill_opengl_caps_7(&ddesc);
    
    TRACE(" enumerating OpenGL D3DDevice7 interface.\n");
    
    return cb(interface_name, device_name, &ddesc, context);
}

static ULONG WINAPI
GL_IDirect3DDeviceImpl_7_3T_2T_1T_Release(LPDIRECT3DDEVICE7 iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    ULONG ref = InterlockedDecrement(&This->ref);
    
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, ref + 1);

    if (!ref) {
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
	This->d3d->d3d_removed_device(This->d3d, This);

        /* Free light arrays */
        HeapFree(GetProcessHeap(), 0, This->light_parameters);
        HeapFree(GetProcessHeap(), 0, This->active_lights);

	HeapFree(GetProcessHeap(), 0, This->world_mat);
	HeapFree(GetProcessHeap(), 0, This->view_mat);
	HeapFree(GetProcessHeap(), 0, This->proj_mat);

	HeapFree(GetProcessHeap(), 0, glThis->surface_ptr);

	DeleteCriticalSection(&(This->crit));
	
	ENTER_GL();
	if (glThis->unlock_tex)
	    glDeleteTextures(1, &(glThis->unlock_tex));
	glXDestroyContext(glThis->display, glThis->gl_context);
	LEAVE_GL();
	HeapFree(GetProcessHeap(), 0, This->clipping_planes);
	HeapFree(GetProcessHeap(), 0, This->vertex_buffer);

	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
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
					  LPVOID context, int version)
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
    pformat->u2.dwRBitMask =        0x00FF0000;
    pformat->u3.dwGBitMask =        0x0000FF00;
    pformat->u4.dwBBitMask =        0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0xFF000000;
    if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
    if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

    TRACE("Enumerating GL_RGB unpacked (32)\n");
    pformat->dwFlags = DDPF_RGB;
    pformat->u1.dwRGBBitCount = 32;
    pformat->u2.dwRBitMask =        0x00FF0000;
    pformat->u3.dwGBitMask =        0x0000FF00;
    pformat->u4.dwBBitMask =        0x000000FF;
    pformat->u5.dwRGBAlphaBitMask = 0x00000000;
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

    /* DXT textures only exist for devices created from IDirect3D3 and above */
    if ((version >= 3) && GL_extensions.s3tc_compressed_texture) {
	TRACE("Enumerating DXT1\n");
	pformat->dwFlags = DDPF_FOURCC;
        pformat->dwFourCC = MAKE_FOURCC('D','X','T','1');
	if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
	if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

	TRACE("Enumerating DXT3\n");
	pformat->dwFlags = DDPF_FOURCC;
        pformat->dwFourCC = MAKE_FOURCC('D','X','T','3');
	if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
	if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;

	TRACE("Enumerating DXT5\n");
	pformat->dwFlags = DDPF_FOURCC;
        pformat->dwFourCC = MAKE_FOURCC('D','X','T','5');
	if (cb_1) if (cb_1(&sdesc , context) == 0) return DD_OK;
	if (cb_2) if (cb_2(pformat, context) == 0) return DD_OK;
    }

    TRACE("End of enumeration\n");
    return DD_OK;
}


HRESULT
d3ddevice_find(IDirectDrawImpl *d3d,
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
    return enum_texture_format_OpenGL(lpD3DEnumTextureProc, NULL, lpArg, This->version);
}

static HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_EnumTextureFormats(LPDIRECT3DDEVICE7 iface,
					       LPD3DENUMPIXELFORMATSCALLBACK lpD3DEnumPixelProc,
					       LPVOID lpArg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpD3DEnumPixelProc, lpArg);
    return enum_texture_format_OpenGL(NULL, lpD3DEnumPixelProc, lpArg, This->version);
}

static HRESULT WINAPI
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

static HRESULT WINAPI
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

static HRESULT WINAPI
GL_IDirect3DDeviceImpl_3_2T_GetLightState(LPDIRECT3DDEVICE3 iface,
					  D3DLIGHTSTATETYPE dwLightStateType,
					  LPDWORD lpdwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    
    TRACE("(%p/%p)->(%08x,%p)\n", This, iface, dwLightStateType, lpdwLightState);

    if (!dwLightStateType && (dwLightStateType > D3DLIGHTSTATE_COLORVERTEX)) {
	TRACE("Unexpected Light State Type\n");
	return DDERR_INVALIDPARAMS;
    }
	
    if (dwLightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */) {
	*lpdwLightState = This->material;
    } else if (dwLightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */) {
	*lpdwLightState = D3DCOLOR_RGB;
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
		ERR("Unknown D3DLIGHTSTATETYPE %d.\n", dwLightStateType);
		return DDERR_INVALIDPARAMS;
	}

	IDirect3DDevice7_GetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
	                   		rs,lpdwLightState);
    }

    return DD_OK;
}

static HRESULT WINAPI
GL_IDirect3DDeviceImpl_3_2T_SetLightState(LPDIRECT3DDEVICE3 iface,
					  D3DLIGHTSTATETYPE dwLightStateType,
					  DWORD dwLightState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    
    TRACE("(%p/%p)->(%08x,%08lx)\n", This, iface, dwLightStateType, dwLightState);

    if (!dwLightStateType && (dwLightStateType > D3DLIGHTSTATE_COLORVERTEX)) {
	TRACE("Unexpected Light State Type\n");
	return DDERR_INVALIDPARAMS;
    }
	
    if (dwLightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */) {
	IDirect3DMaterialImpl *mat = (IDirect3DMaterialImpl *) dwLightState;

	if (mat != NULL) {
	    TRACE(" activating material %p.\n", mat);
	    mat->activate(mat);
	} else {
	    FIXME(" D3DLIGHTSTATE_MATERIAL called with NULL material !!!\n");
	}
	This->material = dwLightState;
    } else if (dwLightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */) {
	switch (dwLightState) {
	    case D3DCOLOR_MONO:
	       ERR("DDCOLOR_MONO should not happen!\n");
	       break;
	    case D3DCOLOR_RGB:
	       /* We are already in this mode */
	       TRACE("Setting color model to RGB (no-op).\n");
	       break;
	    default:
	       ERR("Unknown color model!\n");
	       return DDERR_INVALIDPARAMS;
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
		ERR("Unknown D3DLIGHTSTATETYPE %d.\n", dwLightStateType);
		return DDERR_INVALIDPARAMS;
	}

	IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
	                   		rs,dwLightState);
    }

    return DD_OK;
}

static GLenum convert_D3D_ptype_to_GL(D3DPRIMITIVETYPE d3dpt)
{
    switch (d3dpt) {
        case D3DPT_POINTLIST:
            TRACE(" primitive type is POINTS\n");
	    return GL_POINTS;

	case D3DPT_LINELIST:
	    TRACE(" primitive type is LINES\n");
	    return GL_LINES;
		
	case D3DPT_LINESTRIP:
	    TRACE(" primitive type is LINE_STRIP\n");
	    return GL_LINE_STRIP;
	    
	case D3DPT_TRIANGLELIST:
	    TRACE(" primitive type is TRIANGLES\n");
	    return GL_TRIANGLES;
	    
	case D3DPT_TRIANGLESTRIP:
	    TRACE(" primitive type is TRIANGLE_STRIP\n");
	    return GL_TRIANGLE_STRIP;
	    
	case D3DPT_TRIANGLEFAN:
	    TRACE(" primitive type is TRIANGLE_FAN\n");
	    return GL_TRIANGLE_FAN;
	    
	default:
	    FIXME("Unhandled primitive %08x\n", d3dpt);
	    return GL_POINTS;
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

    } else if (vertex_transformed &&
	       (glThis->transform_state != GL_TRANSFORM_ORTHO)) {
        /* Set our orthographic projection */
	if (glThis->transform_state != GL_TRANSFORM_ORTHO) {
	    glThis->transform_state = GL_TRANSFORM_ORTHO;
	    d3ddevice_set_ortho(This);
	}
    }

    /* TODO: optimize this to not always reset all the fog stuff on all DrawPrimitive call
             if no fogging state change occurred */
    if (This->state_block.render_state[D3DRENDERSTATE_FOGENABLE - 1]) {
        if (vertex_transformed) {
	    if (glThis->fogging != 0) {
		glDisable(GL_FOG);
		glThis->fogging = 0;
	    }
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
		if (glThis->fogging == 0) {
		    glEnable(GL_FOG);
		    glThis->fogging = 1;
		}
	    } else {
		if (glThis->fogging != 0) {
		    glDisable(GL_FOG);
		    glThis->fogging = 0;
		}
	    }
        }
    } else {
	if (glThis->fogging != 0) {
	    glDisable(GL_FOG);
	    glThis->fogging = 0;
	}
    }
    
    /* Handle the 'no-normal' case */
    if ((vertex_lit == FALSE) && This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1]) {
	if (glThis->lighting == 0) {
	    glEnable(GL_LIGHTING);
	    glThis->lighting = 1;
	}
    } else {
	if (glThis->lighting != 0) {
	    glDisable(GL_LIGHTING);
	    glThis->lighting = 0;
	}
    }

    /* Handle the code for pre-vertex material properties */
    if (vertex_transformed == FALSE) {
        if (This->state_block.render_state[D3DRENDERSTATE_LIGHTING - 1] &&
	    This->state_block.render_state[D3DRENDERSTATE_COLORVERTEX - 1]) {
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

static void flush_zbuffer_to_GL(IDirect3DDeviceImpl *d3d_dev, LPCRECT pRect, IDirectDrawSurfaceImpl *surf) {
    static BOOLEAN first = TRUE;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
    unsigned int row;
    GLenum type;
    
    if (first) {
	MESSAGE("Warning : application does direct locking of ZBuffer - expect slowdowns on many GL implementations :-)\n");
	first = FALSE;
    }
    
    TRACE("flushing ZBuffer back to GL\n");
    
    if (gl_d3d_dev->transform_state != GL_TRANSFORM_ORTHO) {
	gl_d3d_dev->transform_state = GL_TRANSFORM_ORTHO;
	d3ddevice_set_ortho(d3d_dev);
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (gl_d3d_dev->depth_test == 0) glEnable(GL_DEPTH_TEST);
    if (d3d_dev->state_block.render_state[D3DRENDERSTATE_ZFUNC - 1] != D3DCMP_ALWAYS) glDepthFunc(GL_ALWAYS);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 

    /* This loop here is to prevent using PixelZoom that may be unoptimized for the 1.0 / -1.0 case
       in some drivers...
    */
    switch (surf->surface_desc.u4.ddpfPixelFormat.u1.dwZBufferBitDepth) {
        case 16: type = GL_UNSIGNED_SHORT; break;
	case 32: type = GL_UNSIGNED_INT; break;
	default: FIXME("Unhandled ZBuffer format !\n"); goto restore_state;
    }
	
    for (row = 0; row < surf->surface_desc.dwHeight; row++) {
	/* glRasterPos3d(0.0, row + 1.0, 0.5); */
	glRasterPos2i(0, row + 1);
	glDrawPixels(surf->surface_desc.dwWidth, 1, GL_DEPTH_COMPONENT, type,
		     ((unsigned char *) surf->surface_desc.lpSurface) + (row * surf->surface_desc.u1.lPitch));
    }

  restore_state:
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (d3d_dev->state_block.render_state[D3DRENDERSTATE_ZFUNC - 1] != D3DCMP_ALWAYS)
	glDepthFunc(convert_D3D_compare_to_GL(d3d_dev->state_block.render_state[D3DRENDERSTATE_ZFUNC - 1]));
    if (gl_d3d_dev->depth_test == 0) glDisable(GL_DEPTH_TEST);
}

/* These are the various handler used in the generic path */
inline static void handle_xyz(D3DVALUE *coords) {
    glVertex3fv(coords);
}
inline static void handle_xyzrhw(D3DVALUE *coords) {
    if ((coords[3] < 1e-8) && (coords[3] > -1e-8))
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
    if (sb->render_state[D3DRENDERSTATE_ALPHATESTENABLE - 1] ||
	sb->render_state[D3DRENDERSTATE_ALPHABLENDENABLE - 1]) {
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
	sb->render_state[D3DRENDERSTATE_LIGHTING - 1] &&
	sb->render_state[D3DRENDERSTATE_COLORVERTEX - 1]) {
        if (sb->render_state[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE - 1] == D3DMCS_COLOR1) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	    handle_diffuse_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_AMBIENTMATERIALSOURCE - 1] == D3DMCS_COLOR1) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
	    handle_diffuse_base(sb, color);
	}
	if ((sb->render_state[D3DRENDERSTATE_SPECULARMATERIALSOURCE - 1] == D3DMCS_COLOR1) &&
	    sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1]) {
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
	sb->render_state[D3DRENDERSTATE_LIGHTING - 1] &&
	sb->render_state[D3DRENDERSTATE_COLORVERTEX - 1]) {
        if (sb->render_state[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE - 1] == D3DMCS_COLOR2) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	    handle_specular_base(sb, color);
	}
	if (sb->render_state[D3DRENDERSTATE_AMBIENTMATERIALSOURCE - 1] == D3DMCS_COLOR2) {
	    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT);
	    handle_specular_base(sb, color);
	}
	if ((sb->render_state[D3DRENDERSTATE_SPECULARMATERIALSOURCE - 1] == D3DMCS_COLOR2) &&
	    sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1]) {
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
    if (lighted) {
        DWORD color = *color_d;
        if (sb->render_state[D3DRENDERSTATE_FOGENABLE - 1]) {
	    /* Special case where the specular value is used to do fogging */
	    BYTE fog_intensity = *color_s >> 24; /* The alpha value of the specular component is the fog 'intensity' for this vertex */
	    color &= 0xFF000000; /* Only keep the alpha component */
	    color |= fog_table[((*color_d >>  0) & 0xFF) << 8 | fog_intensity] <<  0;
	    color |= fog_table[((*color_d >>  8) & 0xFF) << 8 | fog_intensity] <<  8;
	    color |= fog_table[((*color_d >> 16) & 0xFF) << 8 | fog_intensity] << 16;
	}
	if (sb->render_state[D3DRENDERSTATE_SPECULARENABLE - 1]) {
	    /* Standard specular value in transformed mode. TODO */
	}
	handle_diffuse_base(sb, &color);
    } else {
        if (sb->render_state[D3DRENDERSTATE_LIGHTING - 1]) {
	    handle_diffuse(sb, color_d, FALSE);
	    handle_specular(sb, color_s, FALSE);
	} else {
	    /* In that case, only put the diffuse color... */
	    handle_diffuse_base(sb, color_d);
	}
    }
}

static void handle_texture(DWORD size, const D3DVALUE *coords) {
    switch (size) {
        case 1: glTexCoord1fv(coords); break;
	case 2: glTexCoord2fv(coords); break;
	case 3: glTexCoord3fv(coords); break;
	case 4: glTexCoord4fv(coords); break;
    }
}

inline static void handle_textures(DWORD size, const D3DVALUE *coords, int tex_stage) {
    if (GL_extensions.max_texture_units > 0) {
	GL_extensions.glMultiTexCoord[size - 1](GL_TEXTURE0_WINE + tex_stage, coords);
    } else {
	if (tex_stage == 0) handle_texture(size, coords);
    }
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
    int num_tex_index = GET_TEXCOUNT_FROM_FVF(d3dvtVertexType);
    BOOL reenable_depth_test = FALSE;
    
    /* I put the trace before the various locks... So as to better understand where locks occur :-) */
    if (TRACE_ON(ddraw)) {
        TRACE(" Vertex format : "); dump_flexible_vertex(d3dvtVertexType);
    }

    /* This is to prevent 'thread contention' between a thread locking the device and another
       doing 3D display on it... */
    EnterCriticalSection(&(This->crit));   
    
    ENTER_GL();
    if (glThis->state[WINE_GL_BUFFER_BACK] == SURFACE_MEMORY_DIRTY) {
        This->flush_to_framebuffer(This, &(glThis->lock_rect[WINE_GL_BUFFER_BACK]), glThis->lock_surf[WINE_GL_BUFFER_BACK]);
    }
    glThis->state[WINE_GL_BUFFER_BACK] = SURFACE_GL;

    if (This->current_zbuffer == NULL) {
	/* Search for an attached ZBuffer */
	static const DDSCAPS2 zbuf_caps = { DDSCAPS_ZBUFFER, 0, 0, 0 };
	LPDIRECTDRAWSURFACE7 zbuf;
	HRESULT hr;
	
	hr = IDirectDrawSurface7_GetAttachedSurface(ICOM_INTERFACE(This->surface, IDirectDrawSurface7),
						    (DDSCAPS2 *) &zbuf_caps, &zbuf);
	if (SUCCEEDED(hr)) {
	    This->current_zbuffer = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, zbuf);
	    IDirectDrawSurface7_Release(zbuf);
	} else if (glThis->depth_test) {
	    glDisable(GL_DEPTH_TEST);
	    reenable_depth_test = TRUE;
	}
    }
    if (This->current_zbuffer != NULL) {
	if (This->current_zbuffer->get_dirty_status(This->current_zbuffer, NULL)) {
	    flush_zbuffer_to_GL(This, NULL, This->current_zbuffer);
	}
    }
    
    if ( ((d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ) ||
         ((d3dvtVertexType & D3DFVF_NORMAL) == 0) )
        vertex_lighted = TRUE;
    
    /* Compute the number of active texture stages and set the various texture parameters */
    num_active_stages = draw_primitive_handle_textures(This);

    /* And restore to handle '0' in the case we use glTexCoord calls */
    if (glThis->current_active_tex_unit != GL_TEXTURE0_WINE) {
	GL_extensions.glActiveTexture(GL_TEXTURE0_WINE);
	glThis->current_active_tex_unit = GL_TEXTURE0_WINE;
    }

    draw_primitive_handle_GL_state(This,
				   (d3dvtVertexType & D3DFVF_POSITION_MASK) != D3DFVF_XYZ,
				   vertex_lighted);

    /* First, see if we can use the OpenGL vertex arrays... This is very limited
       for now to some 'special' cases where we can do a direct mapping between D3D
       types and GL types.

       Note: in the future all calls will go through vertex arrays but the arrays
             will be generated by this function.

       Note2: colours cannot be mapped directly because they are stored as BGRA in memory
              (ie not as an array of R, G, B, A as OpenGL does it but as a LWORD 0xAARRGGBB
	      which, as we are little indian, gives a B, G, R, A storage in memory.
    */
    if (((d3dvtVertexType & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) && /* Standard XYZ vertices */
	((d3dvtVertexType & (D3DFVF_DIFFUSE|D3DFVF_SPECULAR)) == 0)) {
	int tex_stage;
	TRACE(" using GL vertex arrays for performance !\n");
	/* First, the vertices (we are sure we have some :-) */
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, lpD3DDrawPrimStrideData->position.dwStride, lpD3DDrawPrimStrideData->position.lpvData);
	/* Then the normals */
	if (d3dvtVertexType & D3DFVF_NORMAL) {
	    glEnableClientState(GL_NORMAL_ARRAY);
	    glNormalPointer(GL_FLOAT, lpD3DDrawPrimStrideData->normal.dwStride, lpD3DDrawPrimStrideData->normal.lpvData);
	}
	/* Then the diffuse colour */
	if (d3dvtVertexType & D3DFVF_DIFFUSE) {
	    glEnableClientState(GL_COLOR_ARRAY);
	    glColorPointer(3, GL_UNSIGNED_BYTE, lpD3DDrawPrimStrideData->normal.dwStride,
			   ((char *) lpD3DDrawPrimStrideData->diffuse.lpvData));
	}
	/* Then the various textures */
	for (tex_stage = 0; tex_stage < num_active_stages; tex_stage++) {
	    int tex_index = This->state_block.texture_stage_state[tex_stage][D3DTSS_TEXCOORDINDEX - 1] & 0x0000FFFF;
	    if (tex_index >= num_tex_index) {
		WARN("Default texture coordinate not handled in the vertex array path !!!\n");
		tex_index = num_tex_index - 1;
	    }
	    if (GL_extensions.glClientActiveTexture) {
		GL_extensions.glClientActiveTexture(GL_TEXTURE0_WINE + tex_stage);
	    }
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    glTexCoordPointer(GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_index), GL_FLOAT, lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride,
			      lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData);
	}
	if (dwIndices != NULL) {
	    glDrawElements(convert_D3D_ptype_to_GL(d3dptPrimitiveType), dwIndexCount, GL_UNSIGNED_SHORT, dwIndices);
	} else {
	    glDrawArrays(convert_D3D_ptype_to_GL(d3dptPrimitiveType), 0, dwIndexCount);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	if (d3dvtVertexType & D3DFVF_NORMAL) {
	    glDisableClientState(GL_NORMAL_ARRAY);
	}
	if (d3dvtVertexType & D3DFVF_DIFFUSE) {
	    glDisableClientState(GL_COLOR_ARRAY);
	}
	for (tex_stage = 0; tex_stage < num_active_stages; tex_stage++) {
	    if (GL_extensions.glClientActiveTexture) {
		GL_extensions.glClientActiveTexture(GL_TEXTURE0_WINE + tex_stage);
	    }
	    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
    } else {
	glBegin(convert_D3D_ptype_to_GL(d3dptPrimitiveType));
	
	/* Some fast paths first before the generic case.... */
	if ((d3dvtVertexType == D3DFVF_VERTEX) && (num_active_stages <= 1)) {
	    unsigned int index;
	    
	    for (index = 0; index < dwIndexCount; index++) {
		int i = (dwIndices == NULL) ? index : dwIndices[index];
		D3DVALUE *normal = 
		    (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->normal.lpvData) + i * lpD3DDrawPrimStrideData->normal.dwStride);
		D3DVALUE *tex_coord =
		    (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[0].lpvData) + i * lpD3DDrawPrimStrideData->textureCoords[0].dwStride);
		D3DVALUE *position =
		    (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->position.lpvData) + i * lpD3DDrawPrimStrideData->position.dwStride);
		
		handle_normal(normal);
		handle_texture(2, tex_coord);
		handle_xyz(position);
		
		TRACE_(ddraw_geom)(" %f %f %f / %f %f %f (%f %f)\n",
				   position[0], position[1], position[2],
				   normal[0], normal[1], normal[2],
				   tex_coord[0], tex_coord[1]);
	    }
	} else if ((d3dvtVertexType == D3DFVF_TLVERTEX) && (num_active_stages <= 1)) {
	    unsigned int index;
	    
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
		handle_texture(2, tex_coord);
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
	    unsigned int index;
	    /* static const D3DVALUE no_index[] = { 0.0, 0.0, 0.0, 0.0 }; */
	    
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
		    int tex_index = This->state_block.texture_stage_state[tex_stage][D3DTSS_TEXCOORDINDEX - 1] & 0x0000FFFF;
		    D3DVALUE *tex_coord;
		    
		    if (tex_index >= num_tex_index) {
			/* This will have to be checked on Windows. RealMYST uses this feature and I would find it more
			 * logical to re-use the index of the previous stage than a default index of '0'.
			 */
			
			/* handle_textures((const D3DVALUE *) no_index, tex_stage); */
			tex_index = num_tex_index - 1;
		    }
		    tex_coord = (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
					      i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
		    handle_textures(GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_index), tex_coord, tex_stage);
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
		    unsigned int tex_index;
		    
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
		    for (tex_index = 0; tex_index < GET_TEXCOUNT_FROM_FVF(d3dvtVertexType); tex_index++) {
			D3DVALUE *tex_coord =
			    (D3DVALUE *) (((char *) lpD3DDrawPrimStrideData->textureCoords[tex_index].lpvData) + 
					  i * lpD3DDrawPrimStrideData->textureCoords[tex_index].dwStride);
			switch (GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_index)) {
			    case 1: TRACE_(ddraw_geom)(" / %f", tex_coord[0]); break;
			    case 2: TRACE_(ddraw_geom)(" / %f %f", tex_coord[0], tex_coord[1]); break;
			    case 3: TRACE_(ddraw_geom)(" / %f %f %f", tex_coord[0], tex_coord[1], tex_coord[2]); break;
			    case 4: TRACE_(ddraw_geom)(" / %f %f %f %f", tex_coord[0], tex_coord[1], tex_coord[2], tex_coord[3]); break;
			    default: TRACE_(ddraw_geom)("Invalid texture size (%ld) !!!", GET_TEXCOORD_SIZE_FROM_FVF(d3dvtVertexType, tex_index)); break;
			}
		    }
		    TRACE_(ddraw_geom)("\n");
		}
	    }
	} else {
	    ERR(" matrix weighting not handled yet....\n");
	}
	
	glEnd();
    }

    /* Whatever the case, disable the color material stuff */
    glDisable(GL_COLOR_MATERIAL);

    if (reenable_depth_test)
	glEnable(GL_DEPTH_TEST);

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

    if (vb_impl->processed) {
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

    if (vb_impl->processed) {
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
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    const char *type;
    DWORD prev_state;
    GLenum unit;
    
    TRACE("(%p/%p)->(%08lx,%08x,%08lx)\n", This, iface, dwStage, d3dTexStageStateType, dwState);

    if (((GL_extensions.max_texture_units == 0) && (dwStage > 0)) ||
	((GL_extensions.max_texture_units != 0) && (dwStage >= GL_extensions.max_texture_units))) {
	return DD_OK;
    }

    unit = GL_TEXTURE0_WINE + dwStage;
    if (unit != glThis->current_active_tex_unit) {
	GL_extensions.glActiveTexture(unit);
	glThis->current_active_tex_unit = unit;
    }
    
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
	    break;
	    
        case D3DTSS_MAGFILTER:
	    if (TRACE_ON(ddraw)) {
	        switch ((D3DTEXTUREMAGFILTER) dwState) {
	            case D3DTFG_POINT:  TRACE(" Stage type is : D3DTSS_MAGFILTER => D3DTFG_POINT\n"); break;
		    case D3DTFG_LINEAR: TRACE(" Stage type is : D3DTSS_MAGFILTER => D3DTFG_LINEAR\n"); break;
		    default: FIXME(" Unhandled stage type : D3DTSS_MAGFILTER => %08lx\n", dwState); break;
		}
	    }
            break;

        case D3DTSS_ADDRESS:
        case D3DTSS_ADDRESSU:
        case D3DTSS_ADDRESSV: {
	    switch ((D3DTEXTUREADDRESS) dwState) {
	        case D3DTADDRESS_WRAP:   TRACE(" Stage type is : %s => D3DTADDRESS_WRAP\n", type); break;
	        case D3DTADDRESS_CLAMP:  TRACE(" Stage type is : %s => D3DTADDRESS_CLAMP\n", type); break;
	        case D3DTADDRESS_BORDER: TRACE(" Stage type is : %s => D3DTADDRESS_BORDER\n", type); break;
		case D3DTADDRESS_MIRROR:
		    if (GL_extensions.mirrored_repeat) {
			TRACE(" Stage type is : %s => D3DTADDRESS_MIRROR\n", type);
		    } else {
			FIXME(" Stage type is : %s => D3DTADDRESS_MIRROR - not supported by GL !\n", type);
		    }
		    break;
	        default: FIXME(" Unhandled stage type : %s => %08lx\n", type, dwState); break;
	    }
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

            if ((d3dTexStageStateType == D3DTSS_COLOROP) && (dwState == D3DTOP_DISABLE)) {
                glDisable(GL_TEXTURE_2D);
		TRACE(" disabling 2D texturing.\n");
            } else {
	        /* Re-enable texturing only if COLOROP was not already disabled... */
	        if ((glThis->current_bound_texture[dwStage] != NULL) &&
		    (This->state_block.texture_stage_state[dwStage][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE)) {
		    glEnable(GL_TEXTURE_2D);
		    TRACE(" enabling 2D texturing.\n");
		}
		
                /* Re-Enable GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT */
                if ((dwState != D3DTOP_DISABLE) &&
		    (This->state_block.texture_stage_state[dwStage][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE)) {
		    if (glThis->current_tex_env != GL_COMBINE_EXT) {
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
			glThis->current_tex_env = GL_COMBINE_EXT;
		    }
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
	    
	    if ((value != 0.0) && (GL_extensions.mipmap_lodbias == FALSE))
	        handled = FALSE;

	    if (handled) {
	        TRACE(" Stage type : D3DTSS_MIPMAPLODBIAS => %f\n", value);
		glTexEnvf(GL_TEXTURE_FILTER_CONTROL_WINE, GL_TEXTURE_LOD_BIAS_WINE, value);
	    } else {
	        FIXME(" Unhandled stage type : D3DTSS_MIPMAPLODBIAS => %f\n", value);
	    }
	} break;

	case D3DTSS_MAXMIPLEVEL: 
	    TRACE(" Stage type : D3DTSS_MAXMIPLEVEL => %ld\n", dwState);
	    break;

	case D3DTSS_BORDERCOLOR:
	    TRACE(" Stage type : D3DTSS_BORDERCOLOR => %02lx %02lx %02lx %02lx (RGBA)\n",
		  ((dwState >> 16) & 0xFF),
		  ((dwState >>  8) & 0xFF),
		  ((dwState >>  0) & 0xFF),
		  ((dwState >> 24) & 0xFF));
	    break;
	    
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

	    if (handled) {
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

static DWORD
draw_primitive_handle_textures(IDirect3DDeviceImpl *This)
{
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    DWORD stage;
    BOOLEAN enable_colorkey = FALSE;
    
    for (stage = 0; stage < MAX_TEXTURES; stage++) {
	IDirectDrawSurfaceImpl *surf_ptr = This->current_texture[stage];
	GLenum unit;

	/* If this stage is disabled, no need to go further... */
	if (This->state_block.texture_stage_state[stage][D3DTSS_COLOROP - 1] == D3DTOP_DISABLE)
	    break;
	
	/* First check if we need to bind any other texture for this stage */
	if (This->current_texture[stage] != glThis->current_bound_texture[stage]) {
	    if (This->current_texture[stage] == NULL) {
		TRACE(" disabling 2D texturing for stage %ld.\n", stage);
		
		unit = GL_TEXTURE0_WINE + stage;
		if (unit != glThis->current_active_tex_unit) {
		    GL_extensions.glActiveTexture(unit);
		    glThis->current_active_tex_unit = unit;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	    } else {
		GLenum tex_name = gltex_get_tex_name(surf_ptr);
		
		unit = GL_TEXTURE0_WINE + stage;
		if (unit != glThis->current_active_tex_unit) {
		    GL_extensions.glActiveTexture(unit);
		    glThis->current_active_tex_unit = unit;
		}

		if (glThis->current_bound_texture[stage] == NULL) {
		    if (This->state_block.texture_stage_state[stage][D3DTSS_COLOROP - 1] != D3DTOP_DISABLE) {
			TRACE(" enabling 2D texturing and");
			glEnable(GL_TEXTURE_2D);
		    }
		}
		TRACE(" activating OpenGL texture id %d for stage %ld.\n", tex_name, stage);
		glBindTexture(GL_TEXTURE_2D, tex_name);
	    }

	    glThis->current_bound_texture[stage] = This->current_texture[stage];
	} else {
	    if (glThis->current_bound_texture[stage] == NULL) {
		TRACE(" displaying without texturing activated for stage %ld.\n", stage);
	    } else {
		TRACE(" using already bound texture id %d for stage %ld.\n",
		      ((IDirect3DTextureGLImpl *) (glThis->current_bound_texture[stage])->tex_private)->tex_name, stage);
	    }
	}

	/* If no texure valid for this stage, go out of the loop */
	if (This->current_texture[stage] == NULL) break;

	/* Then check if we need to flush this texture to GL or not (ie did it change) ?.
	   This will also update the various texture parameters if needed.
	*/
	gltex_upload_texture(surf_ptr, This, stage);

	/* And finally check for color-keying (only on first stage) */
	if (This->current_texture[stage]->surface_desc.dwFlags & DDSD_CKSRCBLT) {
	    if (stage == 0) {
		enable_colorkey = TRUE;
	    } else {
		static BOOL warn = FALSE;
		if (warn == FALSE) {
		    warn = TRUE;
		    WARN(" Surface has color keying on stage different from 0 (%ld) !", stage);
		}
	    }
	} else {
	    if (stage == 0) {
		enable_colorkey = FALSE;
	    }
	}
    }

    /* Apparently, whatever the state of BLEND, color keying is always activated for 'old' D3D versions */
    if (((This->state_block.render_state[D3DRENDERSTATE_COLORKEYENABLE - 1]) ||
	 (glThis->parent.version == 1)) &&
	(enable_colorkey)) {
	TRACE(" colorkey activated.\n");
	
	if (glThis->alpha_test == FALSE) {
	    glEnable(GL_ALPHA_TEST);
	    glThis->alpha_test = TRUE;
	}
	if ((glThis->current_alpha_test_func != GL_NOTEQUAL) || (glThis->current_alpha_test_ref != 0.0)) {
	    if (This->state_block.render_state[D3DRENDERSTATE_ALPHATESTENABLE - 1]) {
		static BOOL warn = FALSE;
		if (warn == FALSE) {
		    warn = TRUE;
		    WARN(" Overriding application-given alpha test values - some graphical glitches may appear !\n");
		}
	    }
	    glThis->current_alpha_test_func = GL_NOTEQUAL;
	    glThis->current_alpha_test_ref = 0.0;
	    glAlphaFunc(GL_NOTEQUAL, 0.0);
	}
	/* Some sanity checks should be added here if a game mixes alphatest + color keying...
	   Only one has been found for now, and the ALPHAFUNC is 'Always' so it works :-) */
    } else {
	if (This->state_block.render_state[D3DRENDERSTATE_ALPHATESTENABLE - 1] == FALSE) {
	    glDisable(GL_ALPHA_TEST);
	    glThis->alpha_test = FALSE;
	}
	/* Maybe we should restore here the application-given alpha test states ? */
    }
    
    return stage;
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_3T_SetTexture(LPDIRECT3DDEVICE7 iface,
				       DWORD dwStage,
				       LPDIRECTDRAWSURFACE7 lpTexture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    
    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwStage, lpTexture2);

    if (((GL_extensions.max_texture_units == 0) && (dwStage > 0)) ||
	((GL_extensions.max_texture_units != 0) && (dwStage >= GL_extensions.max_texture_units))) {
	if (lpTexture2 != NULL) {
	    WARN(" setting a texture to a non-supported texture stage !\n");
	}
	return DD_OK;
    }

    if (This->current_texture[dwStage] != NULL) {
	IDirectDrawSurface7_Release(ICOM_INTERFACE(This->current_texture[dwStage], IDirectDrawSurface7));
    }
    
    if (lpTexture2 == NULL) {
	This->current_texture[dwStage] = NULL;
    } else {
        IDirectDrawSurfaceImpl *tex_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, lpTexture2);
	IDirectDrawSurface7_AddRef(ICOM_INTERFACE(tex_impl, IDirectDrawSurface7));
	This->current_texture[dwStage] = tex_impl;
    }
    
    return DD_OK;
}

static HRESULT WINAPI
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
        TRACE(" material is :\n");
	dump_D3DMATERIAL7(lpMat);
    }
    
    This->current_material = *lpMat;

    ENTER_GL();
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
    LEAVE_GL();

    return DD_OK;
}

static LPD3DLIGHT7 get_light(IDirect3DDeviceImpl *This, DWORD dwLightIndex)
{
    if (dwLightIndex >= This->num_set_lights)
    {
        /* Extend, or allocate the light parameters array. */
        DWORD newlightnum = dwLightIndex + 1;
        LPD3DLIGHT7 newarrayptr = NULL;

        if (This->light_parameters)
            newarrayptr = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                This->light_parameters, newlightnum * sizeof(D3DLIGHT7));
        else
            newarrayptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                newlightnum * sizeof(D3DLIGHT7));

        if (!newarrayptr)
            return NULL;

        This->light_parameters = newarrayptr;
        This->num_set_lights = newlightnum;
    }

    return &This->light_parameters[dwLightIndex];
}

HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_SetLight(LPDIRECT3DDEVICE7 iface,
				  DWORD dwLightIndex,
				  LPD3DLIGHT7 lpLight)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    LPD3DLIGHT7 lpdestlight = get_light( This, dwLightIndex );

    TRACE("(%p/%p)->(%08lx,%p)\n", This, iface, dwLightIndex, lpLight);
    
    if (TRACE_ON(ddraw)) {
        TRACE(" setting light :\n");
	dump_D3DLIGHT7(lpLight);
    }
    
    /* DirectX7 documentation states that this function can return
       DDERR_OUTOFMEMORY, so we do just that in case of an allocation
       error (which is the only reason why get_light() can fail). */
    if( !lpdestlight )
        return DDERR_OUTOFMEMORY;

    *lpdestlight = *lpLight;

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
    int lightslot = -1, i;
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    LPD3DLIGHT7 lpdestlight = get_light(This, dwLightIndex);

    TRACE("(%p/%p)->(%08lx,%d)\n", This, iface, dwLightIndex, bEnable);

    /* The DirectX doc isn't as explicit as for SetLight as whether we can
       return this from this function, but it doesn't state otherwise. */
    if (!lpdestlight)
        return DDERR_OUTOFMEMORY;

    /* If this light hasn't been set, initialise it with default values. */
    if (lpdestlight->dltType == 0)
    {
        TRACE("setting default light parameters\n");

        /* We always use HEAP_ZERO_MEMORY when allocating the light_parameters
           array, so we only have to setup anything that shoud not be zero. */
        lpdestlight->dltType = D3DLIGHT_DIRECTIONAL;
        lpdestlight->dcvDiffuse.u1.r = 1.f;
        lpdestlight->dcvDiffuse.u2.g = 1.f;
        lpdestlight->dcvDiffuse.u3.b = 1.f;
        lpdestlight->dvDirection.u3.z = 1.f;
    }

    /* Look for this light in the active lights array. */
    for (i = 0; i < This->max_active_lights; i++)
        if (This->active_lights[i] == dwLightIndex)
        {
            lightslot = i;
            break;
        }

    /* If we didn't find it, let's find the first available slot, if any. */
    if (lightslot == -1)
        for (i = 0; i < This->max_active_lights; i++)
            if (This->active_lights[i] == ~0)
            {
                lightslot = i;
                break;
            }

    ENTER_GL();
    if (bEnable) {
        if (lightslot == -1)
        {
            /* This means that the app is trying to enable more lights than
               the maximum possible indicated in the caps.

               Windows actually let you do this, and disable one of the
               previously enabled lights to let you enable this one.

               It's not documented and I'm not sure how windows pick which light
               to disable to make room for this one. */
            FIXME("Enabling more light than the maximum is not supported yet.");
            return D3D_OK;
        }

        glEnable(GL_LIGHT0 + lightslot);


        if (This->active_lights[lightslot] == ~0)
        {
	    /* This light gets active... Need to update its parameters to GL before the next drawing */
	    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;

            This->active_lights[lightslot] = dwLightIndex;
	    glThis->transform_state = GL_TRANSFORM_NONE;
	}
    } else {
        glDisable(GL_LIGHT0 + lightslot);
        This->active_lights[lightslot] = ~0;
    }

    LEAVE_GL();
    return DD_OK;
}

HRESULT  WINAPI  
GL_IDirect3DDeviceImpl_7_SetClipPlane(LPDIRECT3DDEVICE7 iface, DWORD dwIndex, D3DVALUE* pPlaneEquation) 
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
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

static HRESULT WINAPI
GL_IDirect3DDeviceImpl_7_SetViewport(LPDIRECT3DDEVICE7 iface,
				     LPD3DVIEWPORT7 lpData)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    if (TRACE_ON(ddraw)) {
        TRACE(" viewport is :\n");
	TRACE("    - dwX = %ld   dwY = %ld\n",
	      lpData->dwX, lpData->dwY);
	TRACE("    - dwWidth = %ld   dwHeight = %ld\n",
	      lpData->dwWidth, lpData->dwHeight);
	TRACE("    - dvMinZ = %f   dvMaxZ = %f\n",
	      lpData->dvMinZ, lpData->dvMaxZ);
    }
    ENTER_GL();
    
    /* Set the viewport */
    if ((lpData->dvMinZ != This->active_viewport.dvMinZ) ||
	(lpData->dvMaxZ != This->active_viewport.dvMaxZ)) {
	glDepthRange(lpData->dvMinZ, lpData->dvMaxZ);
    }
    if ((lpData->dwX != This->active_viewport.dwX) ||
	(lpData->dwY != This->active_viewport.dwY) ||
	(lpData->dwWidth != This->active_viewport.dwWidth) ||
	(lpData->dwHeight != This->active_viewport.dwHeight)) {
	glViewport(lpData->dwX,
		   This->surface->surface_desc.dwHeight - (lpData->dwHeight + lpData->dwY),
		   lpData->dwWidth, lpData->dwHeight);
    }

    LEAVE_GL();

    This->active_viewport = *lpData;
    
    return DD_OK;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DDevice7.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DDevice7Vtbl VTABLE_IDirect3DDevice7 =
{
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

static const IDirect3DDevice3Vtbl VTABLE_IDirect3DDevice3 =
{
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
    XCAST(GetLightState) GL_IDirect3DDeviceImpl_3_2T_GetLightState,
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

static const IDirect3DDevice2Vtbl VTABLE_IDirect3DDevice2 =
{
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

static const IDirect3DDeviceVtbl VTABLE_IDirect3DDevice =
{
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
			       WINE_GL_BUFFER_TYPE buffer_type,
			       DWORD dwCount,
			       LPD3DRECT lpRects,
			       DWORD dwFlags,
			       DWORD dwColor,
			       D3DVALUE dvZ,
			       DWORD dwStencil)
{
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    GLbitfield bitfield = 0;
    D3DRECT rect;
    unsigned int i;
    
    TRACE("(%p)->(%08lx,%p,%08lx,%08lx,%f,%08lx)\n", This, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
    if (TRACE_ON(ddraw)) {
	if (dwCount > 0) {
	    unsigned int i;
	    TRACE(" rectangles :\n");
	    for (i = 0; i < dwCount; i++) {
	        TRACE("  - %ld x %ld     %ld x %ld\n", lpRects[i].u1.x1, lpRects[i].u2.y1, lpRects[i].u3.x2, lpRects[i].u4.y2);
	    }
	}
    }

    if (dwCount == 0) {
        dwCount = 1;
	rect.u1.x1 = 0;
	rect.u2.y1 = 0;
	rect.u3.x2 = This->surface->surface_desc.dwWidth;
	rect.u4.y2 = This->surface->surface_desc.dwHeight;
	lpRects = &rect;
    }
    
    /* Clears the screen */
    ENTER_GL();

    if (dwFlags & D3DCLEAR_TARGET) {
	if (glThis->state[buffer_type] == SURFACE_MEMORY_DIRTY) {
	    /* TODO: optimize here the case where Clear changes all the screen... */
	    This->flush_to_framebuffer(This, &(glThis->lock_rect[buffer_type]), glThis->lock_surf[buffer_type]);
	}
	glThis->state[buffer_type] = SURFACE_GL;
    }

    if (dwFlags & D3DCLEAR_ZBUFFER) {
	bitfield |= GL_DEPTH_BUFFER_BIT;
	if (glThis->depth_mask == FALSE) {
	    glDepthMask(GL_TRUE); /* Enables Z writing to be sure to delete also the Z buffer */
	}
	if (dvZ != glThis->prev_clear_Z) {
	    glClearDepth(dvZ);
	    glThis->prev_clear_Z = dvZ;
	}
	TRACE(" depth value : %f\n", dvZ);
    }
    if (dwFlags & D3DCLEAR_STENCIL) {
        bitfield |= GL_STENCIL_BUFFER_BIT;
	if (dwStencil != glThis->prev_clear_stencil) {
	    glClearStencil(dwStencil);
	    glThis->prev_clear_stencil = dwStencil;
	}
	TRACE(" stencil value : %ld\n", dwStencil);
    }    
    if (dwFlags & D3DCLEAR_TARGET) {
        bitfield |= GL_COLOR_BUFFER_BIT;
	if (dwColor != glThis->prev_clear_color) {
	    glClearColor(((dwColor >> 16) & 0xFF) / 255.0,
			 ((dwColor >>  8) & 0xFF) / 255.0,
			 ((dwColor >>  0) & 0xFF) / 255.0,
			 ((dwColor >> 24) & 0xFF) / 255.0);
	    glThis->prev_clear_color = dwColor;
	}
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
	if (glThis->depth_mask == FALSE) glDepthMask(GL_FALSE);
    }
    
    LEAVE_GL();
    
    return DD_OK;
}

static HRESULT d3ddevice_clear_back(IDirect3DDeviceImpl *This,
				    DWORD dwCount,
				    LPD3DRECT lpRects,
				    DWORD dwFlags,
				    DWORD dwColor,
				    D3DVALUE dvZ,
				    DWORD dwStencil)
{
    return d3ddevice_clear(This, WINE_GL_BUFFER_BACK, dwCount, lpRects, dwFlags, dwColor, dvZ, dwStencil);
}

static HRESULT
setup_rect_and_surface_for_blt(IDirectDrawSurfaceImpl *This,
			       WINE_GL_BUFFER_TYPE *buffer_type_p, D3DRECT *rect)
{
    IDirect3DDeviceGLImpl *gl_d3d_dev = (IDirect3DDeviceGLImpl *) This->d3ddevice;
    WINE_GL_BUFFER_TYPE buffer_type;
    
    /* First check if we BLT to the backbuffer... */
    if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) != 0) {
	buffer_type = WINE_GL_BUFFER_BACK;
    } else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
	buffer_type = WINE_GL_BUFFER_FRONT;
    } else {
	ERR("Only BLT override to front or back-buffer is supported for now !\n");
	return DDERR_INVALIDPARAMS;
    }
            
    if ((gl_d3d_dev->state[buffer_type] == SURFACE_MEMORY_DIRTY) &&
	(rect->u1.x1 >= gl_d3d_dev->lock_rect[buffer_type].left) &&
	(rect->u2.y1 >= gl_d3d_dev->lock_rect[buffer_type].top) &&
	(rect->u3.x2 <= gl_d3d_dev->lock_rect[buffer_type].right) &&
	(rect->u4.y2 <= gl_d3d_dev->lock_rect[buffer_type].bottom)) {
	/* If the memory zone is already dirty, use the standard 'in memory' blit operations and not
	 * GL to do it.
	 */
	return DDERR_INVALIDPARAMS;
    }
    *buffer_type_p = buffer_type;
    
    return DD_OK;
}

HRESULT
d3ddevice_blt(IDirectDrawSurfaceImpl *This, LPRECT rdst,
	      LPDIRECTDRAWSURFACE7 src, LPRECT rsrc,
	      DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    WINE_GL_BUFFER_TYPE buffer_type;
    D3DRECT rect;

    if (rdst) {
	rect.u1.x1 = rdst->left;
	rect.u2.y1 = rdst->top;
	rect.u3.x2 = rdst->right;
	rect.u4.y2 = rdst->bottom;
    } else {
	rect.u1.x1 = 0;
	rect.u2.y1 = 0;
	rect.u3.x2 = This->surface_desc.dwWidth;
	rect.u4.y2 = This->surface_desc.dwHeight;
    }
    
    if (setup_rect_and_surface_for_blt(This, &buffer_type, &rect) != DD_OK) return DDERR_INVALIDPARAMS;

    if (dwFlags & DDBLT_COLORFILL) {
        /* This is easy to handle for the D3D Device... */
        DWORD color;
        GLint prev_draw;
        
        /* The color as given in the Blt function is in the format of the frame-buffer...
         * 'clear' expect it in ARGB format => we need to do some conversion :-)
         */
        if (This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            if (This->palette) {
                color = ((0xFF000000) |
                         (This->palette->palents[lpbltfx->u5.dwFillColor].peRed << 16) |
                         (This->palette->palents[lpbltfx->u5.dwFillColor].peGreen << 8) |
                         (This->palette->palents[lpbltfx->u5.dwFillColor].peBlue));
            } else {
                color = 0xFF000000;
            }
        } else if ((This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_RGB) &&
                   (((This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) == 0) ||
                    (This->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000))) {
            if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) &&
                (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask == 0xF800) &&
                (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask == 0x07E0) &&
                (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask == 0x001F)) {
                if (lpbltfx->u5.dwFillColor == 0xFFFF) {
                    color = 0xFFFFFFFF;
                } else {
                    color = ((0xFF000000) |
                             ((lpbltfx->u5.dwFillColor & 0xF800) << 8) |
                             ((lpbltfx->u5.dwFillColor & 0x07E0) << 5) |
                             ((lpbltfx->u5.dwFillColor & 0x001F) << 3));
                }
            } else if (((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) ||
                        (This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 24)) &&
                       (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask == 0x00FF0000) &&
                       (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask == 0x0000FF00) &&
                       (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask == 0x000000FF)) {
                color = 0xFF000000 | lpbltfx->u5.dwFillColor;
            } else {
                ERR("Wrong surface type for BLT override (unknown RGB format) !\n");
                return DDERR_INVALIDPARAMS;
            }
        } else {
            ERR("Wrong surface type for BLT override !\n");
            return DDERR_INVALIDPARAMS;
        }
        
        TRACE(" executing D3D Device override.\n");
	
        ENTER_GL();

        glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
        if (buffer_type == WINE_GL_BUFFER_FRONT)
            glDrawBuffer(GL_FRONT);
        else
            glDrawBuffer(GL_BACK);
        
        d3ddevice_clear(This->d3ddevice, buffer_type, 1, &rect, D3DCLEAR_TARGET, color, 0.0, 0x00000000);
	
        if (((buffer_type == WINE_GL_BUFFER_FRONT) && (prev_draw == GL_BACK)) ||
            ((buffer_type == WINE_GL_BUFFER_BACK)  && (prev_draw == GL_FRONT)))
            glDrawBuffer(prev_draw);
	
        LEAVE_GL();
        
        return DD_OK;
    } else if ((dwFlags & (~(DDBLT_KEYSRC|DDBLT_WAIT|DDBLT_ASYNC))) == 0) {
	/* Normal blit without any special case... */
	if (src != NULL) {
	    /* And which has a SRC surface */
	    IDirectDrawSurfaceImpl *src_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, src);
	    
	    if ((src_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE) &&
		(src_impl->d3ddevice == This->d3ddevice) &&
		((dwFlags & DDBLT_KEYSRC) == 0)) {
		/* Both are 3D devices and using the same GL device and the Blt is without color-keying */
		D3DRECT src_rect;
		int width, height;
		GLint prev_draw;
		WINE_GL_BUFFER_TYPE src_buffer_type;
		IDirect3DDeviceGLImpl *gl_d3d_dev = (IDirect3DDeviceGLImpl *) This->d3ddevice;
		BOOLEAN initial;
		DWORD opt_bitmap;
		int x, y;
	
		if (rsrc) {
		    src_rect.u1.x1 = rsrc->left;
		    src_rect.u2.y1 = rsrc->top;
		    src_rect.u3.x2 = rsrc->right;
		    src_rect.u4.y2 = rsrc->bottom;
		} else {
		    src_rect.u1.x1 = 0;
		    src_rect.u2.y1 = 0;
		    src_rect.u3.x2 = src_impl->surface_desc.dwWidth;
		    src_rect.u4.y2 = src_impl->surface_desc.dwHeight;
		}

		width = src_rect.u3.x2 - src_rect.u1.x1;
		height = src_rect.u4.y2 - src_rect.u2.y1;

		if ((width != (rect.u3.x2 - rect.u1.x1)) ||
		    (height != (rect.u4.y2 - rect.u2.y1))) {
		    FIXME(" buffer to buffer copy not supported with stretching yet !\n");
		    return DDERR_INVALIDPARAMS;
		}

		/* First check if we BLT from the backbuffer... */
		if ((src_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) != 0) {
		    src_buffer_type = WINE_GL_BUFFER_BACK;
		} else if ((src_impl->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
		    src_buffer_type = WINE_GL_BUFFER_FRONT;
		} else {
		    ERR("Unexpected case in direct buffer to buffer copy !\n");
		    return DDERR_INVALIDPARAMS;
		}
		
		TRACE(" using direct buffer to buffer copy.\n");

		ENTER_GL();

		opt_bitmap = d3ddevice_set_state_for_flush(This->d3ddevice, (LPCRECT) &rect, FALSE, &initial);
		
		if (upload_surface_to_tex_memory_init(This, 0, &gl_d3d_dev->current_internal_format,
						      initial, FALSE, UNLOCK_TEX_SIZE, UNLOCK_TEX_SIZE) != DD_OK) {
		    ERR(" unsupported pixel format at direct buffer to buffer copy.\n");
		    LEAVE_GL();
		    return DDERR_INVALIDPARAMS;
		}
		
		glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
		if (buffer_type == WINE_GL_BUFFER_FRONT)
		    glDrawBuffer(GL_FRONT);
		else
		    glDrawBuffer(GL_BACK);

		if (src_buffer_type == WINE_GL_BUFFER_FRONT)
		    glReadBuffer(GL_FRONT);
		else
		    glReadBuffer(GL_BACK);

		/* Now the serious stuff happens. Basically, we copy from the source buffer to the texture memory.
		   And directly re-draws this on the destination buffer. */
		for (y = 0; y < height; y += UNLOCK_TEX_SIZE) {
		    int get_height;
		    
		    if ((src_rect.u2.y1 + y + UNLOCK_TEX_SIZE) > src_impl->surface_desc.dwHeight)
			get_height = src_impl->surface_desc.dwHeight - (src_rect.u2.y1 + y);
		    else
			get_height = UNLOCK_TEX_SIZE;
		    
		    for (x = 0; x < width; x += UNLOCK_TEX_SIZE) {
			int get_width;
			
			if ((src_rect.u1.x1 + x + UNLOCK_TEX_SIZE) > src_impl->surface_desc.dwWidth)
			    get_width = src_impl->surface_desc.dwWidth - (src_rect.u1.x1 + x);
			else
			    get_width = UNLOCK_TEX_SIZE;
			
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
					    0, UNLOCK_TEX_SIZE - get_height,
					    src_rect.u1.x1 + x, src_impl->surface_desc.dwHeight - (src_rect.u2.y1 + y + get_height),
					    get_width, get_height);
			
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0);
			glVertex3d(rect.u1.x1 + x,
				   rect.u2.y1 + y + UNLOCK_TEX_SIZE,
				   0.5);
			glTexCoord2f(1.0, 0.0);
			glVertex3d(rect.u1.x1 + x + UNLOCK_TEX_SIZE,
				   rect.u2.y1 + y + UNLOCK_TEX_SIZE,
				   0.5);
			glTexCoord2f(1.0, 1.0);
			glVertex3d(rect.u1.x1 + x + UNLOCK_TEX_SIZE,
				   rect.u2.y1 + y,
				   0.5);
			glTexCoord2f(0.0, 1.0);
			glVertex3d(rect.u1.x1 + x,
				   rect.u2.y1 + y,
				   0.5);
			glEnd();
		    }
		}
		
		upload_surface_to_tex_memory_release();
		d3ddevice_restore_state_after_flush(This->d3ddevice, opt_bitmap, FALSE);
		
		if (((buffer_type == WINE_GL_BUFFER_FRONT) && (prev_draw == GL_BACK)) ||
		    ((buffer_type == WINE_GL_BUFFER_BACK)  && (prev_draw == GL_FRONT)))
		    glDrawBuffer(prev_draw);
		
		LEAVE_GL();

		return DD_OK;
	    } else {
		/* This is the normal 'with source' Blit. Use the texture engine to do the Blt for us
		   (this prevents calling glReadPixels) */
		D3DRECT src_rect;
		int width, height;
		GLint prev_draw;
		IDirect3DDeviceGLImpl *gl_d3d_dev = (IDirect3DDeviceGLImpl *) This->d3ddevice;
		BOOLEAN initial;
		DWORD opt_bitmap;
		int x, y;
		double x_stretch, y_stretch;
		
		if (rsrc) {
		    src_rect.u1.x1 = rsrc->left;
		    src_rect.u2.y1 = rsrc->top;
		    src_rect.u3.x2 = rsrc->right;
		    src_rect.u4.y2 = rsrc->bottom;
		} else {
		    src_rect.u1.x1 = 0;
		    src_rect.u2.y1 = 0;
		    src_rect.u3.x2 = src_impl->surface_desc.dwWidth;
		    src_rect.u4.y2 = src_impl->surface_desc.dwHeight;
		}

		width = src_rect.u3.x2 - src_rect.u1.x1;
		height = src_rect.u4.y2 - src_rect.u2.y1;

		x_stretch = (double) (rect.u3.x2 - rect.u1.x1) / (double) width;
		y_stretch = (double) (rect.u4.y2 - rect.u2.y1) / (double) height;

		TRACE(" using memory to buffer Blt override.\n");

		ENTER_GL();

		opt_bitmap = d3ddevice_set_state_for_flush(This->d3ddevice, (LPCRECT) &rect, ((dwFlags & DDBLT_KEYSRC) != 0), &initial);
		
		if (upload_surface_to_tex_memory_init(src_impl, 0, &gl_d3d_dev->current_internal_format,
						      initial, ((dwFlags & DDBLT_KEYSRC) != 0), UNLOCK_TEX_SIZE, UNLOCK_TEX_SIZE) != DD_OK) {
		    ERR(" unsupported pixel format at memory to buffer Blt override.\n");
		    LEAVE_GL();
		    return DDERR_INVALIDPARAMS;
		}
		
		glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
		if (buffer_type == WINE_GL_BUFFER_FRONT)
		    glDrawBuffer(GL_FRONT);
		else
		    glDrawBuffer(GL_BACK);

		/* Now the serious stuff happens. This is basically the same code as for the memory
		   flush to frame buffer ... with stretching and different rectangles added :-) */
		for (y = 0; y < height; y += UNLOCK_TEX_SIZE) {
		    RECT flush_rect;

		    flush_rect.top    = src_rect.u2.y1 + y;
		    flush_rect.bottom = ((src_rect.u2.y1 + y + UNLOCK_TEX_SIZE > src_rect.u4.y2) ?
					 src_rect.u4.y2 :
					 (src_rect.u2.y1 + y + UNLOCK_TEX_SIZE));
		    
		    for (x = 0; x < width; x += UNLOCK_TEX_SIZE) {
			flush_rect.left  = src_rect.u1.x1 + x;
			flush_rect.right = ((src_rect.u1.x1 + x + UNLOCK_TEX_SIZE > src_rect.u3.x2) ?
					    src_rect.u3.x2 :
					    (src_rect.u1.x1 + x + UNLOCK_TEX_SIZE));
			
			upload_surface_to_tex_memory(&flush_rect, 0, 0, &(gl_d3d_dev->surface_ptr));
			
			glBegin(GL_QUADS);
			glTexCoord2f(0.0, 0.0);
			glVertex3d(rect.u1.x1 + (x * x_stretch),
				   rect.u2.y1 + (y * y_stretch),
				   0.5);
			glTexCoord2f(1.0, 0.0);
			glVertex3d(rect.u1.x1 + ((x + UNLOCK_TEX_SIZE) * x_stretch),
				   rect.u2.y1 + (y * y_stretch),
				   0.5);
			glTexCoord2f(1.0, 1.0);
			glVertex3d(rect.u1.x1 + ((x + UNLOCK_TEX_SIZE) * x_stretch),
				   rect.u2.y1 + ((y + UNLOCK_TEX_SIZE) * y_stretch),
				   0.5);
			glTexCoord2f(0.0, 1.0);
			glVertex3d(rect.u1.x1 + (x * x_stretch),
				   rect.u2.y1 + ((y + UNLOCK_TEX_SIZE) * y_stretch),
				   0.5);
			glEnd();
		    }
		}
		
		upload_surface_to_tex_memory_release();
		d3ddevice_restore_state_after_flush(This->d3ddevice, opt_bitmap, ((dwFlags & DDBLT_KEYSRC) != 0));
		
		if (((buffer_type == WINE_GL_BUFFER_FRONT) && (prev_draw == GL_BACK)) ||
		    ((buffer_type == WINE_GL_BUFFER_BACK)  && (prev_draw == GL_FRONT)))
		    glDrawBuffer(prev_draw);
		
		LEAVE_GL();

		return DD_OK;		
	    }
	}
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT
d3ddevice_bltfast(IDirectDrawSurfaceImpl *This, DWORD dstx,
		  DWORD dsty, LPDIRECTDRAWSURFACE7 src,
		  LPRECT rsrc, DWORD trans)
{
    RECT rsrc2;
    RECT rdst;
    IDirectDrawSurfaceImpl *src_impl = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, src);
    IDirect3DDeviceGLImpl *gl_d3d_dev = (IDirect3DDeviceGLImpl *) This->d3ddevice;
    WINE_GL_BUFFER_TYPE buffer_type;
    GLint prev_draw;
    DWORD opt_bitmap;
    BOOLEAN initial;
    int width, height, x, y;
    
    /* Cannot support DSTCOLORKEY blitting... */
    if ((trans & DDBLTFAST_DESTCOLORKEY) != 0) return DDERR_INVALIDPARAMS;

    if (rsrc == NULL) {
	WARN("rsrc is NULL - getting the whole surface !!\n");
	rsrc = &rsrc2;
	rsrc->left = rsrc->top = 0;
	rsrc->right = src_impl->surface_desc.dwWidth;
	rsrc->bottom = src_impl->surface_desc.dwHeight;
    } else {
	rsrc2 = *rsrc;
	rsrc = &rsrc2;
    }

    rdst.left = dstx;
    rdst.top = dsty;
    rdst.right = dstx + (rsrc->right - rsrc->left);
    if (rdst.right > This->surface_desc.dwWidth) {
	rsrc->right -= (This->surface_desc.dwWidth - rdst.right);
	rdst.right = This->surface_desc.dwWidth;
    }
    rdst.bottom = dsty + (rsrc->bottom - rsrc->top);
    if (rdst.bottom > This->surface_desc.dwHeight) {
	rsrc->bottom -= (This->surface_desc.dwHeight - rdst.bottom);
	rdst.bottom = This->surface_desc.dwHeight;
    }

    width = rsrc->right - rsrc->left;
    height = rsrc->bottom - rsrc->top;
    
    if (setup_rect_and_surface_for_blt(This, &buffer_type, (D3DRECT *) &rdst) != DD_OK) return DDERR_INVALIDPARAMS;

    TRACE(" using BltFast memory to frame buffer override.\n");
    
    ENTER_GL();
    
    opt_bitmap = d3ddevice_set_state_for_flush(This->d3ddevice, &rdst, (trans & DDBLTFAST_SRCCOLORKEY) != 0, &initial);
    
    if (upload_surface_to_tex_memory_init(src_impl, 0, &gl_d3d_dev->current_internal_format,
					  initial, (trans & DDBLTFAST_SRCCOLORKEY) != 0,
					  UNLOCK_TEX_SIZE, UNLOCK_TEX_SIZE) != DD_OK) {
	ERR(" unsupported pixel format at memory to buffer Blt override.\n");
	LEAVE_GL();
	return DDERR_INVALIDPARAMS;
    }
    
    glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
    if (buffer_type == WINE_GL_BUFFER_FRONT)
	glDrawBuffer(GL_FRONT);
    else
	glDrawBuffer(GL_BACK);
    
    /* Now the serious stuff happens. This is basically the same code that for the memory
       flush to frame buffer but with different rectangles for source and destination :-) */
    for (y = 0; y < height; y += UNLOCK_TEX_SIZE) {
	RECT flush_rect;
	
	flush_rect.top    = rsrc->top + y;
	flush_rect.bottom = ((rsrc->top + y + UNLOCK_TEX_SIZE > rsrc->bottom) ?
			     rsrc->bottom :
			     (rsrc->top + y + UNLOCK_TEX_SIZE));
	
	for (x = 0; x < width; x += UNLOCK_TEX_SIZE) {
	    flush_rect.left  = rsrc->left + x;
	    flush_rect.right = ((rsrc->left + x + UNLOCK_TEX_SIZE > rsrc->right) ?
				rsrc->right :
				(rsrc->left + x + UNLOCK_TEX_SIZE));
	    
	    upload_surface_to_tex_memory(&flush_rect, 0, 0, &(gl_d3d_dev->surface_ptr));
	    
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.0, 0.0);
	    glVertex3d(rdst.left + x,
		       rdst.top + y,
		       0.5);
	    glTexCoord2f(1.0, 0.0);
	    glVertex3d(rdst.left + (x + UNLOCK_TEX_SIZE),
		       rdst.top + y,
		       0.5);
	    glTexCoord2f(1.0, 1.0);
	    glVertex3d(rdst.left + (x + UNLOCK_TEX_SIZE),
		       rdst.top + (y + UNLOCK_TEX_SIZE),
		       0.5);
	    glTexCoord2f(0.0, 1.0);
	    glVertex3d(rdst.left + x,
		       rdst.top + (y + UNLOCK_TEX_SIZE),
		       0.5);
	    glEnd();
	}
    }
    
    upload_surface_to_tex_memory_release();
    d3ddevice_restore_state_after_flush(This->d3ddevice, opt_bitmap, (trans & DDBLTFAST_SRCCOLORKEY) != 0);
    
    if (((buffer_type == WINE_GL_BUFFER_FRONT) && (prev_draw == GL_BACK)) ||
	((buffer_type == WINE_GL_BUFFER_BACK)  && (prev_draw == GL_FRONT)))
	glDrawBuffer(prev_draw);
    
    LEAVE_GL();
    
    return DD_OK;
}

void
d3ddevice_set_ortho(IDirect3DDeviceImpl *This)
{
    GLfloat height, width;
    GLfloat trans_mat[16];

    TRACE("(%p)\n", This);
    
    width = This->surface->surface_desc.dwWidth;
    height = This->surface->surface_desc.dwHeight;
    
    /* The X axis is straighforward.. For the Y axis, we need to convert 'D3D' screen coordinates
     * to OpenGL screen coordinates (ie the upper left corner is not the same).
     */
    trans_mat[ 0] = 2.0 / width;  trans_mat[ 4] = 0.0;           trans_mat[ 8] = 0.0;    trans_mat[12] = -1.0;
    trans_mat[ 1] = 0.0;          trans_mat[ 5] = -2.0 / height; trans_mat[ 9] = 0.0;    trans_mat[13] =  1.0;
#if 0
    /* It has been checked that in Messiah, which mixes XYZ and XYZRHZ vertex format in the same scene,
     * that the Z coordinate needs to be given to GL unchanged.
     */
    trans_mat[ 2] = 0.0;          trans_mat[ 6] = 0.0;           trans_mat[10] = 2.0;    trans_mat[14] = -1.0;
#endif
    trans_mat[ 2] = 0.0;          trans_mat[ 6] = 0.0;           trans_mat[10] = 1.0;    trans_mat[14] =  0.0;
    trans_mat[ 3] = 0.0;          trans_mat[ 7] = 0.0;           trans_mat[11] = 0.0;    trans_mat[15] =  1.0;
    
    ENTER_GL();
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
    LEAVE_GL();
}

void
d3ddevice_set_matrices(IDirect3DDeviceImpl *This, DWORD matrices,
		       D3DMATRIX *world_mat, D3DMATRIX *view_mat, D3DMATRIX *proj_mat)
{
    TRACE("(%p,%08lx,%p,%p,%p)\n", This, matrices, world_mat, view_mat, proj_mat);
    
    ENTER_GL();
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

            for (i = 0; i < This->max_active_lights; i++ )
            {
                DWORD dwLightIndex = This->active_lights[i];
                if (dwLightIndex != ~0)
                {
                    LPD3DLIGHT7 pLight = &This->light_parameters[dwLightIndex];
                    switch (pLight->dltType)
                    {
		        case D3DLIGHT_DIRECTIONAL: {
			    float direction[4];
			    float cut_off = 180.0;
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &pLight->dcvAmbient);
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &pLight->dcvDiffuse);
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &pLight->dcvSpecular);
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			    
			    direction[0] = pLight->dvDirection.u1.x;
			    direction[1] = pLight->dvDirection.u2.y;
			    direction[2] = pLight->dvDirection.u3.z;
			    direction[3] = 0.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) direction);
			} break;

			case D3DLIGHT_POINT: {
			    float position[4];
			    float cut_off = 180.0;
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &pLight->dcvAmbient);
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &pLight->dcvDiffuse);
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &pLight->dcvSpecular);
			    position[0] = pLight->dvPosition.u1.x;
			    position[1] = pLight->dvPosition.u2.y;
			    position[2] = pLight->dvPosition.u3.z;
			    position[3] = 1.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) position);
			    glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, &pLight->dvAttenuation0);
			    glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, &pLight->dvAttenuation1);
			    glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, &pLight->dvAttenuation2);
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			} break;

			case D3DLIGHT_SPOT: {
			    float direction[4];
			    float position[4];
			    float cut_off = 90.0 * (This->light_parameters[i].dvPhi / M_PI);
			    
			    glLightfv(GL_LIGHT0 + i, GL_AMBIENT,  (float *) &pLight->dcvAmbient);
			    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE,  (float *) &pLight->dcvDiffuse);
			    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, (float *) &pLight->dcvSpecular);
			    
			    direction[0] = pLight->dvDirection.u1.x;
			    direction[1] = pLight->dvDirection.u2.y;
			    direction[2] = pLight->dvDirection.u3.z;
			    direction[3] = 0.0;
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, (float *) direction);
			    position[0] = pLight->dvPosition.u1.x;
			    position[1] = pLight->dvPosition.u2.y;
			    position[2] = pLight->dvPosition.u3.z;
			    position[3] = 1.0;
			    glLightfv(GL_LIGHT0 + i, GL_POSITION, (float *) position);
			    glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, &pLight->dvAttenuation0);
			    glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, &pLight->dvAttenuation1);
			    glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, &pLight->dvAttenuation2);
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &cut_off);
			    glLightfv(GL_LIGHT0 + i, GL_SPOT_EXPONENT, &pLight->dvFalloff);
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
    LEAVE_GL();
}

void
d3ddevice_matrices_updated(IDirect3DDeviceImpl *This, DWORD matrices)
{
    IDirect3DDeviceGLImpl *glThis = (IDirect3DDeviceGLImpl *) This;
    DWORD tex_mat, tex_stage;

    TRACE("(%p,%08lx)\n", This, matrices);
    
    if (matrices & (VIEWMAT_CHANGED|WORLDMAT_CHANGED|PROJMAT_CHANGED)) {
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
	    GLenum unit = GL_TEXTURE0_WINE + tex_stage;
	    if (matrices & tex_mat) {
	        if (This->state_block.texture_stage_state[tex_stage][D3DTSS_TEXTURETRANSFORMFLAGS - 1] != D3DTTFF_DISABLE) {
		    int is_identity = (memcmp(This->tex_mat[tex_stage], id_mat, 16 * sizeof(D3DVALUE)) != 0);

		    if (This->tex_mat_is_identity[tex_stage] != is_identity) {
			if (glThis->current_active_tex_unit != unit) {
			    GL_extensions.glActiveTexture(unit);
			    glThis->current_active_tex_unit = unit;
			}
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf((float *) This->tex_mat[tex_stage]);
		    }
		    This->tex_mat_is_identity[tex_stage] = is_identity;
		} else {
		    if (This->tex_mat_is_identity[tex_stage] == FALSE) {
			if (glThis->current_active_tex_unit != unit) {
			    GL_extensions.glActiveTexture(unit);
			    glThis->current_active_tex_unit = unit;
			}
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			This->tex_mat_is_identity[tex_stage] = TRUE;
		    }
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
    WINE_GL_BUFFER_TYPE buffer_type;
    RECT loc_rect;
    
    if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
        buffer_type = WINE_GL_BUFFER_FRONT;
	if ((gl_d3d_dev->state[WINE_GL_BUFFER_FRONT] != SURFACE_GL) &&
	    (gl_d3d_dev->lock_surf[WINE_GL_BUFFER_FRONT] != This)) {
	    ERR("Change of front buffer.. Expect graphic corruptions !\n");
	}
	gl_d3d_dev->lock_surf[WINE_GL_BUFFER_FRONT] = This;
    } else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
        buffer_type = WINE_GL_BUFFER_BACK;
	if ((gl_d3d_dev->state[WINE_GL_BUFFER_BACK] != SURFACE_GL) &&
	    (gl_d3d_dev->lock_surf[WINE_GL_BUFFER_BACK] != This)) {
	    ERR("Change of back buffer.. Expect graphic corruptions !\n");
	}
	gl_d3d_dev->lock_surf[WINE_GL_BUFFER_BACK] = This;
    } else {
        ERR("Wrong surface type for locking !\n");
	return;
    }

    if (pRect == NULL) {
	loc_rect.top = 0;
	loc_rect.left = 0;
	loc_rect.bottom = This->surface_desc.dwHeight;
	loc_rect.right = This->surface_desc.dwWidth;
	pRect = &loc_rect;
    }
    
    /* Try to acquire the device critical section */
    EnterCriticalSection(&(d3d_dev->crit));
    
    if (gl_d3d_dev->lock_rect_valid[buffer_type]) {
	ERR("Two consecutive locks on %s buffer... Expect problems !\n",
	    (buffer_type == WINE_GL_BUFFER_BACK ? "back" : "front"));
    }
    gl_d3d_dev->lock_rect_valid[buffer_type] = TRUE;
    
    if (gl_d3d_dev->state[buffer_type] != SURFACE_GL) {
	/* Check if the new rectangle is in the previous one or not.
	   If it is not, flush first the previous locks on screen.
	*/
	if ((pRect->top    < gl_d3d_dev->lock_rect[buffer_type].top) ||
	    (pRect->left   < gl_d3d_dev->lock_rect[buffer_type].left) ||
	    (pRect->right  > gl_d3d_dev->lock_rect[buffer_type].right) ||
	    (pRect->bottom > gl_d3d_dev->lock_rect[buffer_type].bottom)) {
	    if (gl_d3d_dev->state[buffer_type] == SURFACE_MEMORY_DIRTY) {
		TRACE(" flushing back to %s buffer as new rect : (%ldx%ld) - (%ldx%ld) not included in old rect : (%ldx%ld) - (%ldx%ld)\n",
		      (buffer_type == WINE_GL_BUFFER_BACK ? "back" : "front"),
		      pRect->left, pRect->top, pRect->right, pRect->bottom,
		      gl_d3d_dev->lock_rect[buffer_type].left, gl_d3d_dev->lock_rect[buffer_type].top,
		      gl_d3d_dev->lock_rect[buffer_type].right, gl_d3d_dev->lock_rect[buffer_type].bottom);
		d3d_dev->flush_to_framebuffer(d3d_dev, &(gl_d3d_dev->lock_rect[buffer_type]), gl_d3d_dev->lock_surf[buffer_type]);
	    }
	    gl_d3d_dev->state[buffer_type] = SURFACE_GL;
	    gl_d3d_dev->lock_rect[buffer_type] = *pRect;
	}
	/* In the other case, do not upgrade the locking rectangle as it's no need... */
    } else {
	gl_d3d_dev->lock_rect[buffer_type] = *pRect;
    }
    
    if (gl_d3d_dev->state[buffer_type] == SURFACE_GL) {
        /* If the surface is already in memory, no need to do anything here... */
        GLenum buffer_format;
	GLenum buffer_color;
	int y;
	char *dst;

	TRACE(" copying %s buffer to main memory with rectangle (%ldx%ld) - (%ldx%ld).\n", (buffer_type == WINE_GL_BUFFER_BACK ? "back" : "front"),
	      pRect->left, pRect->top, pRect->right, pRect->bottom);
	
	/* Note that here we cannot do 'optmizations' about the WriteOnly flag... Indeed, a game
	   may only write to the device... But when we will blit it back to the screen, we need
	   also to blit correctly the parts the application did not overwrite... */

	if (((This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_RGB) != 0) &&
	    (((This->surface_desc.u4.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) == 0) ||
	     (This->surface_desc.u4.ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000))) {
	    if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 16) &&
		(This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask == 0xF800) &&
		(This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask == 0x07E0) &&
		(This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask == 0x001F)) {
		buffer_format = GL_UNSIGNED_SHORT_5_6_5;
		buffer_color = GL_RGB;
	    } else if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 24) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask == 0xFF0000) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask == 0x00FF00) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask == 0x0000FF)) {
		buffer_format = GL_UNSIGNED_BYTE;
		buffer_color = GL_RGB;
	    } else if ((This->surface_desc.u4.ddpfPixelFormat.u1.dwRGBBitCount == 32) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u2.dwRBitMask == 0x00FF0000) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u3.dwGBitMask == 0x0000FF00) &&
		       (This->surface_desc.u4.ddpfPixelFormat.u4.dwBBitMask == 0x000000FF)) {
		buffer_format = GL_UNSIGNED_INT_8_8_8_8_REV;
		buffer_color = GL_BGRA;
	    } else {
		ERR(" unsupported pixel format at device locking.\n");
		return;
	    }
	} else {
	    ERR(" unsupported pixel format at device locking - alpha on frame buffer.\n");
	    return;
	}

	ENTER_GL();
	
	if (buffer_type == WINE_GL_BUFFER_FRONT)
	    /* Application wants to lock the front buffer */
	    glReadBuffer(GL_FRONT);
	else 
	    /* Application wants to lock the back buffer */
	    glReadBuffer(GL_BACK);

	dst = ((char *)This->surface_desc.lpSurface) +
	  (pRect->top * This->surface_desc.u1.lPitch) + (pRect->left * GET_BPP(This->surface_desc));

	if (This->surface_desc.u1.lPitch != (GET_BPP(This->surface_desc) * This->surface_desc.dwWidth)) {
	    /* Slow-path in case of 'odd' surfaces. This could be fixed using some GL options, but I
	     * could not be bothered considering the rare cases where it may be useful :-)
	     */
	    for (y = (This->surface_desc.dwHeight - pRect->top - 1);
		 y >= ((int) This->surface_desc.dwHeight - (int) pRect->bottom);
		 y--) {
		glReadPixels(pRect->left, y,
			     pRect->right - pRect->left, 1,
			     buffer_color, buffer_format, dst);
		dst += This->surface_desc.u1.lPitch;
	    }
	} else {
	    /* Faster path for surface copy. Note that I can use static variables here as I am
	     * protected by the OpenGL critical section so this function won't be called by
	     * two threads at the same time.
	     */
	    static char *buffer = NULL;
	    static int buffer_width = 0;
	    char *dst2 = dst + ((pRect->bottom - pRect->top) - 1) * This->surface_desc.u1.lPitch;
	    int current_width = (pRect->right - pRect->left) * GET_BPP(This->surface_desc);
	    
	    glReadPixels(pRect->left, ((int) This->surface_desc.dwHeight - (int) pRect->bottom),
			 pRect->right - pRect->left, pRect->bottom - pRect->top,
			 buffer_color, buffer_format, dst);

	    if (current_width > buffer_width) {
                HeapFree(GetProcessHeap(), 0, buffer);
		buffer_width = current_width;
		buffer = HeapAlloc(GetProcessHeap(), 0, buffer_width);
	    }
	    for (y = 0; y < ((pRect->bottom - pRect->top) / 2); y++) {
		memcpy(buffer, dst, current_width);
		memcpy(dst, dst2, current_width);
		memcpy(dst2, buffer, current_width);
		dst  += This->surface_desc.u1.lPitch;
		dst2 -= This->surface_desc.u1.lPitch;
	    }
	}

	gl_d3d_dev->state[buffer_type] = SURFACE_MEMORY;
	
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

static void d3ddevice_flush_to_frame_buffer(IDirect3DDeviceImpl *d3d_dev, LPCRECT pRect, IDirectDrawSurfaceImpl *surf) {
    RECT loc_rect;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
    int x, y;
    BOOLEAN initial;
    DWORD opt_bitmap;
    
    /* Note : no need here to lock the 'device critical section' as we are already protected by
       the GL critical section. */

    if (pRect == NULL) {
	loc_rect.top = 0;
	loc_rect.left = 0;
	loc_rect.bottom = d3d_dev->surface->surface_desc.dwHeight;
	loc_rect.right = d3d_dev->surface->surface_desc.dwWidth;
	pRect = &loc_rect;
    }
    
    TRACE(" flushing memory back to screen memory (%ld,%ld) x (%ld,%ld).\n", pRect->top, pRect->left, pRect->right, pRect->bottom);

    opt_bitmap = d3ddevice_set_state_for_flush(d3d_dev, pRect, FALSE, &initial);
    
    if (upload_surface_to_tex_memory_init(surf, 0, &gl_d3d_dev->current_internal_format,
					  initial, FALSE, UNLOCK_TEX_SIZE, UNLOCK_TEX_SIZE) != DD_OK) {
        ERR(" unsupported pixel format at frame buffer flush.\n");
	return;
    }
	
    for (y = pRect->top; y < pRect->bottom; y += UNLOCK_TEX_SIZE) {
	RECT flush_rect;
	
	flush_rect.top = y;
	flush_rect.bottom = (y + UNLOCK_TEX_SIZE > pRect->bottom) ? pRect->bottom : (y + UNLOCK_TEX_SIZE);

	for (x = pRect->left; x < pRect->right; x += UNLOCK_TEX_SIZE) {
	    /* First, upload the texture... */
	    flush_rect.left = x;
	    flush_rect.right = (x + UNLOCK_TEX_SIZE > pRect->right)  ? pRect->right  : (x + UNLOCK_TEX_SIZE);

	    upload_surface_to_tex_memory(&flush_rect, 0, 0, &(gl_d3d_dev->surface_ptr));

	    glBegin(GL_QUADS);
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
    
    upload_surface_to_tex_memory_release();
    d3ddevice_restore_state_after_flush(d3d_dev, opt_bitmap, FALSE);
    
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
    WINE_GL_BUFFER_TYPE buffer_type;
    IDirect3DDeviceImpl *d3d_dev = This->d3ddevice;
    IDirect3DDeviceGLImpl* gl_d3d_dev = (IDirect3DDeviceGLImpl*) d3d_dev;
  
    if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0) {
        buffer_type = WINE_GL_BUFFER_FRONT;
    } else if ((This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER)) {
	buffer_type = WINE_GL_BUFFER_BACK;
    } else {
        ERR("Wrong surface type for locking !\n");
	return;
    }

    if (gl_d3d_dev->lock_rect_valid[buffer_type] == FALSE) {
	ERR("Unlock without prior lock on %s buffer... Expect problems !\n",
	    (buffer_type == WINE_GL_BUFFER_BACK ? "back" : "front"));
    }
    gl_d3d_dev->lock_rect_valid[buffer_type] = FALSE;
    
    /* First, check if we need to do anything. For the backbuffer, flushing is done at the next 3D activity. */
    if ((This->lastlocktype & DDLOCK_READONLY) == 0) {
        if (buffer_type == WINE_GL_BUFFER_FRONT) {
	    GLint prev_draw;

	    TRACE(" flushing front buffer immediately on screen.\n");
	    
	    ENTER_GL();
	    glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
	    glDrawBuffer(GL_FRONT);
	    /* Note: we do not use the application provided lock rectangle but our own stored at
	             lock time. This is because in old D3D versions, the 'lock' parameter did not
		     exist.
	    */
	    d3d_dev->flush_to_framebuffer(d3d_dev, &(gl_d3d_dev->lock_rect[WINE_GL_BUFFER_FRONT]), gl_d3d_dev->lock_surf[WINE_GL_BUFFER_FRONT]);
	    glDrawBuffer(prev_draw);
	    LEAVE_GL();
	} else {
	    gl_d3d_dev->state[WINE_GL_BUFFER_BACK] = SURFACE_MEMORY_DIRTY;
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
	   if (This->state_block.set_flags.texture_stage_state[stage][state]) {
	       IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
						     stage, state + 1, This->state_block.texture_stage_state[stage][state]);
	   }
       }
    }
}     

HRESULT
d3ddevice_create(IDirect3DDeviceImpl **obj, IDirectDrawImpl *d3d, IDirectDrawSurfaceImpl *surface, int version)
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
    
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDeviceGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    gl_object = (IDirect3DDeviceGLImpl *) object;
    
    object->ref = 1;
    object->d3d = d3d;
    object->surface = surface;
    object->set_context = set_context;
    object->clear = d3ddevice_clear_back;
    object->set_matrices = d3ddevice_set_matrices;
    object->matrices_updated = d3ddevice_matrices_updated;
    object->flush_to_framebuffer = d3ddevice_flush_to_frame_buffer;
    object->version = version;
    
    TRACE(" creating OpenGL device for surface = %p, d3d = %p\n", surface, d3d);

    InitializeCriticalSection(&(object->crit));

    TRACE(" device critical section : %p\n", &(object->crit));

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
		
		TRACE(" overriding direct surface access.\n");
	    } else {
	        TRACE(" no override.\n");
	    }
	    surf2->d3ddevice = object;
	}
    }

    /* Set the various light parameters */
    object->num_set_lights = 0;
    object->max_active_lights = opengl_device_caps.dwMaxActiveLights;
    object->light_parameters = NULL;
    object->active_lights = HeapAlloc(GetProcessHeap(), 0,
        object->max_active_lights * sizeof(object->active_lights[0]));
    /* Fill the active light array with ~0, which is used to indicate an
       invalid light index. We don't use 0, because it's a valid light index. */
    for (light=0; light < object->max_active_lights; light++)
        object->active_lights[light] = ~0;

    
    /* Allocate memory for the matrices */
    object->world_mat = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    object->view_mat  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    object->proj_mat  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
    memcpy(object->world_mat, id_mat, 16 * sizeof(float));
    memcpy(object->view_mat , id_mat, 16 * sizeof(float));
    memcpy(object->proj_mat , id_mat, 16 * sizeof(float));
    for (tex_num = 0; tex_num < MAX_TEXTURES; tex_num++) {
        object->tex_mat[tex_num] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 16 * sizeof(float));
	memcpy(object->tex_mat[tex_num], id_mat, 16 * sizeof(float));
	object->tex_mat_is_identity[tex_num] = TRUE;
    }
    
    /* Initialisation */
    TRACE(" setting current context\n");
    object->set_context(object);
    TRACE(" current context set\n");

    /* allocate the clipping planes */
    object->max_clipping_planes = opengl_device_caps.wMaxUserClipPlanes;
    object->clipping_planes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->max_clipping_planes * sizeof(d3d7clippingplane));

    glHint(GL_FOG_HINT,GL_NICEST);

    /* Initialize the various GL contexts to be in sync with what we store locally */
    glClearDepth(0.0);
    glClearStencil(0);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glDepthMask(GL_TRUE);
    gl_object->depth_mask = TRUE;
    glEnable(GL_DEPTH_TEST);
    gl_object->depth_test = TRUE;
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDisable(GL_FOG);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    gl_object->current_tex_env = GL_REPLACE;
    gl_object->current_active_tex_unit = GL_TEXTURE0_WINE;
    if (GL_extensions.glActiveTexture != NULL) {
	GL_extensions.glActiveTexture(GL_TEXTURE0_WINE);
    }
    gl_object->current_alpha_test_ref = 0.0;
    gl_object->current_alpha_test_func = GL_ALWAYS;
    glAlphaFunc(GL_ALWAYS, 0.0);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glDrawBuffer(buffer);
    glReadBuffer(buffer);
    /* glDisable(GL_DEPTH_TEST); Need here to check for the presence of a ZBuffer and to reenable it when the ZBuffer is attached */
    LEAVE_GL();

    gl_object->state[WINE_GL_BUFFER_BACK] = SURFACE_GL;
    gl_object->state[WINE_GL_BUFFER_FRONT] = SURFACE_GL;
    
    /* fill_device_capabilities(d3d->ddraw); */    
    
    ICOM_INIT_INTERFACE(object, IDirect3DDevice,  VTABLE_IDirect3DDevice);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice2, VTABLE_IDirect3DDevice2);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice3, VTABLE_IDirect3DDevice3);
    ICOM_INIT_INTERFACE(object, IDirect3DDevice7, VTABLE_IDirect3DDevice7);

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);

    /* And finally warn D3D that this device is now present */
    object->d3d->d3d_added_device(object->d3d, object);

    InitDefaultStateBlock(&object->state_block, object->version);
    /* Apply default render state and texture stage state values */
    apply_render_state(object, &object->state_block);
    apply_texture_state(object);

    /* And fill the fog table with the default fog value */
    build_fog_table(gl_object->fog_table, object->state_block.render_state[D3DRENDERSTATE_FOGCOLOR - 1]);
    
    return DD_OK;
}

static void fill_opengl_primcaps(D3DPRIMCAPS *pc)
{
    pc->dwSize = sizeof(*pc);
    pc->dwMiscCaps = D3DPMISCCAPS_CONFORMANT | D3DPMISCCAPS_CULLCCW | D3DPMISCCAPS_CULLCW |
      D3DPMISCCAPS_LINEPATTERNREP | D3DPMISCCAPS_MASKPLANES | D3DPMISCCAPS_MASKZ;
    pc->dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_FOGTABLE |
      D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_STIPPLE | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_SUBPIXEL |
	  D3DPRASTERCAPS_ZFOG;
    if (GL_extensions.mipmap_lodbias) {
	pc->dwRasterCaps |= D3DPRASTERCAPS_MIPMAPLODBIAS;
    }
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
      D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_MIPNEAREST | D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_MAGFLINEAR |
	  D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MIPFLINEAR |
	      D3DPTFILTERCAPS_MIPFPOINT;
    pc->dwTextureBlendCaps = D3DPTBLENDCAPS_ADD | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_DECAL | D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_DECALMASK |
      D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA | D3DPTBLENDCAPS_MODULATEMASK;
    pc->dwTextureAddressCaps = D3DPTADDRESSCAPS_BORDER | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_INDEPENDENTUV;
    if (GL_extensions.mirrored_repeat) {
	pc->dwTextureAddressCaps |= D3DPTADDRESSCAPS_MIRROR;
    }
    pc->dwStippleWidth = 32;
    pc->dwStippleHeight = 32;
}

static void fill_caps(void)
{
    GLint max_clip_planes;
    GLint depth_bits;
    
    /* Fill first all the fields with default values which will be overriden later on with
       correct ones from the GL code
    */
    opengl_device_caps.dwDevCaps = D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY |
      D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_TEXTURESYSTEMMEMORY |
      D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY | D3DDEVCAPS_TLVERTEXVIDEOMEMORY |
      /* D3D 7 capabilities */
      D3DDEVCAPS_DRAWPRIMITIVES2 /*| D3DDEVCAPS_HWTRANSFORMANDLIGHT*/ | D3DDEVCAPS_HWRASTERIZATION | D3DDEVCAPS_DRAWPRIMITIVES2EX;
    fill_opengl_primcaps(&(opengl_device_caps.dpcLineCaps));
    fill_opengl_primcaps(&(opengl_device_caps.dpcTriCaps));
    opengl_device_caps.dwDeviceRenderBitDepth  = DDBD_16|DDBD_24|DDBD_32;
    opengl_device_caps.dwMinTextureWidth  = 1;
    opengl_device_caps.dwMinTextureHeight = 1;
    opengl_device_caps.dwMaxTextureWidth  = 1024;
    opengl_device_caps.dwMaxTextureHeight = 1024;
    opengl_device_caps.dwMaxTextureRepeat = 16;
    opengl_device_caps.dwMaxTextureAspectRatio = 1024;
    opengl_device_caps.dwMaxAnisotropy = 0;
    opengl_device_caps.dvGuardBandLeft = 0.0;
    opengl_device_caps.dvGuardBandRight = 0.0;
    opengl_device_caps.dvGuardBandTop = 0.0;
    opengl_device_caps.dvGuardBandBottom = 0.0;
    opengl_device_caps.dvExtentsAdjust = 0.0;
    opengl_device_caps.dwStencilCaps = D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INCRSAT | D3DSTENCILCAPS_INVERT | D3DSTENCILCAPS_KEEP |
      D3DSTENCILCAPS_REPLACE | D3DSTENCILCAPS_ZERO;
    opengl_device_caps.dwTextureOpCaps = D3DTEXOPCAPS_DISABLE | D3DTEXOPCAPS_SELECTARG1 | D3DTEXOPCAPS_SELECTARG2 | D3DTEXOPCAPS_MODULATE4X |
	D3DTEXOPCAPS_MODULATE2X | D3DTEXOPCAPS_MODULATE | D3DTEXOPCAPS_ADD | D3DTEXOPCAPS_ADDSIGNED2X | D3DTEXOPCAPS_ADDSIGNED |
	    D3DTEXOPCAPS_BLENDDIFFUSEALPHA | D3DTEXOPCAPS_BLENDTEXTUREALPHA | D3DTEXOPCAPS_BLENDFACTORALPHA | D3DTEXOPCAPS_BLENDCURRENTALPHA;
    if (GL_extensions.max_texture_units != 0) {
	opengl_device_caps.wMaxTextureBlendStages = GL_extensions.max_texture_units;
	opengl_device_caps.wMaxSimultaneousTextures = GL_extensions.max_texture_units;
	opengl_device_caps.dwFVFCaps = D3DFVFCAPS_DONOTSTRIPELEMENTS | GL_extensions.max_texture_units;
    } else {
	opengl_device_caps.wMaxTextureBlendStages = 1;
	opengl_device_caps.wMaxSimultaneousTextures = 1;
	opengl_device_caps.dwFVFCaps = D3DFVFCAPS_DONOTSTRIPELEMENTS | 1;
    }
    opengl_device_caps.dwMaxActiveLights = 16;
    opengl_device_caps.dvMaxVertexW = 100000000.0; /* No idea exactly what to put here... */
    opengl_device_caps.deviceGUID = IID_IDirect3DTnLHalDevice;
    opengl_device_caps.wMaxUserClipPlanes = 1;
    opengl_device_caps.wMaxVertexBlendMatrices = 0;
    opengl_device_caps.dwVertexProcessingCaps = D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7 | D3DVTXPCAPS_VERTEXFOG |
	D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER;
    opengl_device_caps.dwReserved1 = 0;
    opengl_device_caps.dwReserved2 = 0;
    opengl_device_caps.dwReserved3 = 0;
    opengl_device_caps.dwReserved4 = 0;

    /* And now some GL overrides :-) */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *) &opengl_device_caps.dwMaxTextureWidth);
    opengl_device_caps.dwMaxTextureHeight = opengl_device_caps.dwMaxTextureWidth;
    opengl_device_caps.dwMaxTextureAspectRatio = opengl_device_caps.dwMaxTextureWidth;
    TRACE(": max texture size = %ld\n", opengl_device_caps.dwMaxTextureWidth);
    
    glGetIntegerv(GL_MAX_LIGHTS, (GLint *) &opengl_device_caps.dwMaxActiveLights);
    TRACE(": max active lights = %ld\n", opengl_device_caps.dwMaxActiveLights);

    glGetIntegerv(GL_MAX_CLIP_PLANES, &max_clip_planes);
    opengl_device_caps.wMaxUserClipPlanes = max_clip_planes;
    TRACE(": max clipping planes = %d\n", opengl_device_caps.wMaxUserClipPlanes);

    glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
    TRACE(": Z bits = %d\n", depth_bits);
    switch (depth_bits) {
        case 16: opengl_device_caps.dwDeviceZBufferBitDepth = DDBD_16; break;
        case 24: opengl_device_caps.dwDeviceZBufferBitDepth = DDBD_16|DDBD_24; break;
        case 32:
        default: opengl_device_caps.dwDeviceZBufferBitDepth = DDBD_16|DDBD_24|DDBD_32; break;
    }
}

BOOL
d3ddevice_init_at_startup(void *gl_handle)
{
    XVisualInfo template;
    XVisualInfo *vis;
    HDC device_context;
    Display *display;
    Visual *visual;
    Drawable drawable = (Drawable) GetPropA(GetDesktopWindow(), "__wine_x11_whole_window");
    XWindowAttributes win_attr;
    GLXContext gl_context;
    int num;
    const char *glExtensions;
    const char *glVersion;
    const char *glXExtensions = NULL;
    const void *(*pglXGetProcAddressARB)(const GLubyte *) = NULL;
    int major, minor, patch, num_parsed;
    
    TRACE("Initializing GL...\n");

    if (!drawable)
    {
        WARN("x11drv not loaded - D3D support disabled!\n");
        return FALSE;
    }
    
    /* Get a default rendering context to have the 'caps' function query some info from GL */    
    device_context = GetDC(0);
    display = get_display(device_context);
    ReleaseDC(0, device_context);

    ENTER_GL();
    if (XGetWindowAttributes(display, drawable, &win_attr)) {
	visual = win_attr.visual;
    } else {
	visual = DefaultVisual(display, DefaultScreen(display));
    }
    template.visualid = XVisualIDFromVisual(visual);
    vis = XGetVisualInfo(display, VisualIDMask, &template, &num);
    if (vis == NULL) {
	LEAVE_GL();
	WARN("Error creating visual info for capabilities initialization - D3D support disabled !\n");
	return FALSE;
    }
    gl_context = glXCreateContext(display, vis, NULL, GL_TRUE);
    XFree(vis);

    if (gl_context == NULL) {
	LEAVE_GL();
	WARN("Error creating default context for capabilities initialization - D3D support disabled !\n");
	return FALSE;
    }
    if (glXMakeCurrent(display, drawable, gl_context) == False) {
	glXDestroyContext(display, gl_context);
	LEAVE_GL();
	WARN("Error setting default context as current for capabilities initialization - D3D support disabled !\n");
	return FALSE;	
    }
    
    /* Then, query all extensions */
    glXExtensions = glXQueryExtensionsString(display, DefaultScreen(display)); /* Note: not used right now but will for PBuffers */
    glExtensions = (const char *) glGetString(GL_EXTENSIONS);
    glVersion = (const char *) glGetString(GL_VERSION);
    if (gl_handle != NULL) {
	pglXGetProcAddressARB = wine_dlsym(gl_handle, "glXGetProcAddressARB", NULL, 0);
    }
    
    /* Parse the GL version string */
    num_parsed = sscanf(glVersion, "%d.%d.%d", &major, &minor, &patch);
    if (num_parsed == 1) {
	minor = 0;
	patch = 0;
    } else if (num_parsed == 2) {
	patch = 0;
    }
    TRACE("GL version %d.%d.%d\n", major, minor, patch);

    /* And starts to fill the extension context properly */
    memset(&GL_extensions, 0, sizeof(GL_extensions));
    TRACE("GL supports following extensions used by Wine :\n");
    
    /* Mirrored Repeat extension :
        - GL_ARB_texture_mirrored_repeat
	- GL_IBM_texture_mirrored_repeat
	- GL >= 1.4
    */
    if ((strstr(glExtensions, "GL_ARB_texture_mirrored_repeat")) ||
	(strstr(glExtensions, "GL_IBM_texture_mirrored_repeat")) ||
	(major > 1) ||
	((major == 1) && (minor >= 4))) {
	TRACE(" - mirrored repeat\n");
	GL_extensions.mirrored_repeat = TRUE;
    }

    /* Texture LOD Bias :
        - GL_EXT_texture_lod_bias
    */
    if (strstr(glExtensions, "GL_EXT_texture_lod_bias")) {
	TRACE(" - texture lod bias\n");
	GL_extensions.mipmap_lodbias = TRUE;
    }

    /* For all subsequent extensions, we need glXGetProcAddress */
    if (pglXGetProcAddressARB != NULL) {
	/* Multi-texturing :
	    - GL_ARB_multitexture
	    - GL >= 1.2.1
	*/
	if ((strstr(glExtensions, "GL_ARB_multitexture")) ||
	    (major > 1) ||
	    ((major == 1) && (minor > 2)) ||
	    ((major == 1) && (minor == 2) && (patch >= 1))) {
	    glGetIntegerv(GL_MAX_TEXTURE_UNITS_WINE, &(GL_extensions.max_texture_units));
	    TRACE(" - multi-texturing (%d stages)\n", GL_extensions.max_texture_units);
	    /* We query the ARB version to be the most portable we can... */
	    GL_extensions.glActiveTexture = pglXGetProcAddressARB( (const GLubyte *) "glActiveTextureARB");
	    GL_extensions.glMultiTexCoord[0] = pglXGetProcAddressARB( (const GLubyte *) "glMultiTexCoord1fvARB");
	    GL_extensions.glMultiTexCoord[1] = pglXGetProcAddressARB( (const GLubyte *) "glMultiTexCoord2fvARB");
	    GL_extensions.glMultiTexCoord[2] = pglXGetProcAddressARB( (const GLubyte *) "glMultiTexCoord3fvARB");
	    GL_extensions.glMultiTexCoord[3] = pglXGetProcAddressARB( (const GLubyte *) "glMultiTexCoord4fvARB");
	    GL_extensions.glClientActiveTexture = pglXGetProcAddressARB( (const GLubyte *) "glClientActiveTextureARB");
	} else {
	    GL_extensions.max_texture_units = 0;
	}

	if (strstr(glExtensions, "GL_EXT_texture_compression_s3tc")) {
	    TRACE(" - S3TC compression supported\n");
	    GL_extensions.s3tc_compressed_texture = TRUE;
	    GL_extensions.glCompressedTexImage2D = pglXGetProcAddressARB( (const GLubyte *) "glCompressedTexImage2DARB");
	    GL_extensions.glCompressedTexSubImage2D = pglXGetProcAddressARB( (const GLubyte *) "glCompressedTexSubImage2DARB");
	}
    }
    
    /* Fill the D3D capabilities according to what GL tells us... */
    fill_caps();

    /* And frees this now-useless context */
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, gl_context);
    LEAVE_GL();
    
    return TRUE;
}
