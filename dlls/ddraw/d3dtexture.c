/* Direct3D Texture
 * Copyright (c) 1998 Lionel ULMER
 *
 * This file contains the implementation of interface Direct3DTexture2.
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

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/* Define this if you want to save to a file all the textures used by a game
   (can be funny to see how they managed to cram all the pictures in
   texture memory) */
#undef TEXTURE_SNOOP

#ifdef TEXTURE_SNOOP
#include <stdio.h>

static void 
snoop_texture(IDirectDrawSurfaceImpl *This) {
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;
    char buf[128];
    FILE *f;
    
    sprintf(buf, "tex_%05d_%02d.pnm", glThis->tex_name, This->mipmap_level);
    f = fopen(buf, "wb");
    DDRAW_dump_surface_to_disk(This, f);
}

#else

#define snoop_texture(a)

#endif

static IDirectDrawSurfaceImpl *
get_sub_mimaplevel(IDirectDrawSurfaceImpl *tex_ptr)
{
    /* Now go down the mipmap chain to the next surface */
    static const DDSCAPS2 mipmap_caps = { DDSCAPS_MIPMAP | DDSCAPS_TEXTURE, 0, 0, 0 };
    LPDIRECTDRAWSURFACE7 next_level;
    IDirectDrawSurfaceImpl *surf_ptr;
    HRESULT hr;
    
    hr = IDirectDrawSurface7_GetAttachedSurface(ICOM_INTERFACE(tex_ptr, IDirectDrawSurface7),
						(DDSCAPS2 *) &mipmap_caps, &next_level);
    if (FAILED(hr)) return NULL;
    
    surf_ptr = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, next_level);
    IDirectDrawSurface7_Release(next_level);
    
    return surf_ptr;
}

/*******************************************************************************
 *			   IDirectSurface callback methods
 */

HRESULT
gltex_download_texture(IDirectDrawSurfaceImpl *surf_ptr) {
    IDirect3DTextureGLImpl *gl_surf_ptr = (IDirect3DTextureGLImpl *) surf_ptr->tex_private;

    FIXME("This is not supported yet... Expect some graphical glitches !!!\n");

    /* GL and memory are in sync again ... */
    gl_surf_ptr->dirty_flag = SURFACE_MEMORY;
    
    return DD_OK;
}

HRESULT
gltex_upload_texture(IDirectDrawSurfaceImpl *surf_ptr) {
    IDirect3DTextureGLImpl *gl_surf_ptr = (IDirect3DTextureGLImpl *) surf_ptr->tex_private;
    GLuint tex_name = gl_surf_ptr->tex_name;
    
    TRACE(" activating OpenGL texture id %d.\n", tex_name);
    glBindTexture(GL_TEXTURE_2D, tex_name);

    if (surf_ptr->mipmap_level != 0) {
        WARN(" application activating a sub-level of the mipmapping chain (level %d) !\n", surf_ptr->mipmap_level);
    }
    
    while (surf_ptr != NULL) {
        IDirect3DTextureGLImpl *gl_surf_ptr = (IDirect3DTextureGLImpl *) surf_ptr->tex_private;
	
	if (gl_surf_ptr->dirty_flag != SURFACE_MEMORY_DIRTY) {
            TRACE("   - level %d already uploaded.\n", surf_ptr->mipmap_level);
	} else {
	    TRACE("   - uploading texture level %d (initial done = %d).\n",
		  surf_ptr->mipmap_level, gl_surf_ptr->initial_upload_done);
	    
	    /* Texture snooping for the curious :-) */
	    snoop_texture(surf_ptr);

	    if (upload_surface_to_tex_memory_init(surf_ptr, surf_ptr->mipmap_level, &(gl_surf_ptr->current_internal_format),
						  gl_surf_ptr->initial_upload_done == FALSE, TRUE, 0, 0) == D3D_OK) {
	        upload_surface_to_tex_memory(NULL, 0, 0, &(gl_surf_ptr->surface_ptr));
		upload_surface_to_tex_memory_release();
		gl_surf_ptr->dirty_flag = SURFACE_MEMORY;
	    }
	}
	
	if (surf_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) {
	    surf_ptr = get_sub_mimaplevel(surf_ptr);
	} else {
	    surf_ptr = NULL;
	}
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_1_Initialize(LPDIRECT3DTEXTURE iface,
                                       LPDIRECT3DDEVICE lpDirect3DDevice,
                                       LPDIRECTDRAWSURFACE lpDDSurface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture, iface);
    FIXME("(%p/%p)->(%p,%p) no-op...\n", This, iface, lpDirect3DDevice, lpDDSurface);
    return DD_OK;
}

static HRESULT
gltex_setcolorkey_cb(IDirectDrawSurfaceImpl *This, DWORD dwFlags, LPDDCOLORKEY ckey )
{
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;

    if (glThis->dirty_flag == SURFACE_GL) {
	GLuint cur_tex;
	
	ENTER_GL();
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &cur_tex);
	glBindTexture(GL_TEXTURE_2D, glThis->tex_name);
	gltex_download_texture(This);
	glBindTexture(GL_TEXTURE_2D, cur_tex);
	LEAVE_GL();
    }

    glThis->dirty_flag = SURFACE_MEMORY_DIRTY;
    /* TODO: check color-keying on mipmapped surfaces... */
    
    return DD_OK;
}

HRESULT
gltex_blt(IDirectDrawSurfaceImpl *This, LPRECT rdst,
	  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc,
	  DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    if (src != NULL) {
	IDirectDrawSurfaceImpl *src_ptr = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, src);
	if (src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE) {
	    FIXME("Blt from framebuffer to texture - unsupported now, please report !\n");
	}
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT
gltex_bltfast(IDirectDrawSurfaceImpl *surf_ptr, DWORD dstx,
	      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
	      LPRECT rsrc, DWORD trans)
{
    if (src != NULL) {
	IDirectDrawSurfaceImpl *src_ptr = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, src);
	
	if ((src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE) &&
	    ((trans & (DDBLTFAST_SRCCOLORKEY | DDBLTFAST_DESTCOLORKEY)) == 0)) {
	    /* This is a blt without color keying... We can use the direct copy. */
	    RECT rsrc2;
	    DWORD width, height;
	    GLuint cur_tex;
	    IDirect3DTextureGLImpl *gl_surf_ptr = surf_ptr->tex_private;
	    int y;
	    
	    if (rsrc == NULL) {
		WARN("rsrc is NULL\n");
		rsrc = &rsrc2;
		
		rsrc->left = 0;
		rsrc->top = 0;
		rsrc->right = src_ptr->surface_desc.dwWidth;
		rsrc->bottom = src_ptr->surface_desc.dwHeight;
	    }
	    
	    width = rsrc->right - rsrc->left;
	    height = rsrc->bottom - rsrc->top;
	    
	    if (((dstx + width)  > surf_ptr->surface_desc.dwWidth) ||
		((dsty + height) > surf_ptr->surface_desc.dwHeight)) {
		FIXME("Does not handle clipping yet in FB => Texture blits !\n");
		return DDERR_INVALIDPARAMS;
	    }

	    TRACE(" direct frame buffer => texture BltFast override.\n");
	    
	    ENTER_GL();
	    
	    glGetIntegerv(GL_TEXTURE_BINDING_2D, &cur_tex);
	    glBindTexture(GL_TEXTURE_2D, gl_surf_ptr->tex_name);
	    
	    if ((gl_surf_ptr->dirty_flag == SURFACE_MEMORY_DIRTY) &&
		!((dstx == 0) && (dsty == 0) &&
		  (width == surf_ptr->surface_desc.dwWidth) && (height == surf_ptr->surface_desc.dwHeight))) {
		/* If not 'full size' and the surface is dirty, first flush it to GL before doing the copy. */
	        if (upload_surface_to_tex_memory_init(surf_ptr, surf_ptr->mipmap_level, &(gl_surf_ptr->current_internal_format),
						      gl_surf_ptr->initial_upload_done == FALSE, TRUE, 0, 0) != D3D_OK) {
		    upload_surface_to_tex_memory(NULL, 0, 0, &(gl_surf_ptr->surface_ptr));
		    upload_surface_to_tex_memory_release();
		    gl_surf_ptr->dirty_flag = SURFACE_MEMORY;
		} else {
		    return DDERR_INVALIDPARAMS;
		}
	    }

	    /* This is a hack and would need some clean-up :-) */
	    if (gl_surf_ptr->initial_upload_done == FALSE) {
		gl_surf_ptr->dirty_flag = SURFACE_MEMORY_DIRTY;
		if (upload_surface_to_tex_memory_init(surf_ptr, surf_ptr->mipmap_level, &(gl_surf_ptr->current_internal_format),
						       gl_surf_ptr->initial_upload_done == FALSE, TRUE, 0, 0) != D3D_OK) {
		    upload_surface_to_tex_memory(NULL, 0, 0, &(gl_surf_ptr->surface_ptr));
		    upload_surface_to_tex_memory_release();
		    gl_surf_ptr->dirty_flag = SURFACE_MEMORY;
		} else {
		    return DDERR_INVALIDPARAMS;
		}
	    }
	    
	    if ((src_ptr->surface_desc.ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER|DDSCAPS_PRIMARYSURFACE)) != 0)
		glReadBuffer(GL_FRONT);
	    else if ((src_ptr->surface_desc.ddsCaps.dwCaps & (DDSCAPS_BACKBUFFER)) == (DDSCAPS_BACKBUFFER))
		glReadBuffer(GL_BACK);
	    else {
		ERR("Wrong surface type for locking !\n");
		glBindTexture(GL_TEXTURE_2D, cur_tex);
		LEAVE_GL();
		return DDERR_INVALIDPARAMS;
	    }
	    
	    for (y = (src_ptr->surface_desc.dwHeight - rsrc->top - 1);
		 y >= (src_ptr->surface_desc.dwHeight - (rsrc->top + height));
		 y--) {
		glCopyTexSubImage2D(GL_TEXTURE_2D, surf_ptr->mipmap_level,
				    dstx, dsty,
				    rsrc->left, y,
				    width, 1);
		dsty++;
	    }
	    
	    glBindTexture(GL_TEXTURE_2D, cur_tex);
	    LEAVE_GL();
	    
	    gl_surf_ptr->dirty_flag = SURFACE_GL;
	    
	    return DD_OK;
	}
    }
    return DDERR_INVALIDPARAMS;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_PaletteChanged(LPDIRECT3DTEXTURE2 iface,
                                              DWORD dwStart,
                                              DWORD dwCount)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->(%08lx,%08lx): stub!\n", This, iface, dwStart, dwCount);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_1_Unload(LPDIRECT3DTEXTURE iface)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture, iface);
    FIXME("(%p/%p)->(): stub!\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_GetHandle(LPDIRECT3DTEXTURE2 iface,
					 LPDIRECT3DDEVICE2 lpDirect3DDevice2,
					 LPD3DTEXTUREHANDLE lpHandle)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture2, iface);
    IDirect3DDeviceImpl *lpDeviceImpl = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice2, lpDirect3DDevice2);
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpDirect3DDevice2, lpHandle);

    /* The handle is simply the pointer to the implementation structure */
    *lpHandle = (D3DTEXTUREHANDLE) This;

    TRACE(" returning handle %08lx.\n", *lpHandle);
    
    /* Now set the device for this texture */
    This->d3ddevice = lpDeviceImpl;

    return D3D_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_Load(LPDIRECT3DTEXTURE2 iface,
				    LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DTexture2);
    return DD_OK;
}

static void gltex_set_palette(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* pal)
{
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;

    if (glThis->dirty_flag == SURFACE_GL) {
	GLuint cur_tex;
	
	ENTER_GL();
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &cur_tex);
	glBindTexture(GL_TEXTURE_2D, glThis->tex_name);
	gltex_download_texture(This);
	glBindTexture(GL_TEXTURE_2D, cur_tex);
	LEAVE_GL();
    }
    
    /* First call the previous set_palette function */
    glThis->set_palette(This, pal);
    
    /* And set the dirty flag */
    glThis->dirty_flag = SURFACE_MEMORY_DIRTY;
    
    /* TODO: check palette on mipmapped surfaces...
       TODO: do we need to re-upload in case of usage of the paletted texture extension ? */
}

static void
gltex_final_release(IDirectDrawSurfaceImpl *This)
{
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;
    DWORD mem_used;
    int i;

    TRACE(" deleting texture with GL id %d.\n", glThis->tex_name);

    /* And delete texture handle */
    ENTER_GL();
    if (glThis->tex_name != 0)
        glDeleteTextures(1, &(glThis->tex_name));
    LEAVE_GL();	

    if (glThis->surface_ptr != NULL)
        HeapFree(GetProcessHeap(), 0, glThis->surface_ptr);

    /* And if this texture was the current one, remove it at the device level */
    if (This->d3ddevice != NULL)
        for (i = 0; i < MAX_TEXTURES; i++)
	    if (This->d3ddevice->current_texture[i] == This)
	        This->d3ddevice->current_texture[i] = NULL;

    /* All this should be part of main surface management not just a hack for texture.. */
    if (glThis->loaded) {
        mem_used = This->surface_desc.dwHeight *
	           This->surface_desc.u1.lPitch;
	This->ddraw_owner->free_memory(This->ddraw_owner, mem_used);
    }

    glThis->final_release(This);
}

static void
gltex_lock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect, DWORD dwFlags)
{
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;
    
    glThis->lock_update(This, pRect, dwFlags);
}

static void
gltex_unlock_update(IDirectDrawSurfaceImpl* This, LPCRECT pRect)
{
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;

    glThis->unlock_update(This, pRect);
    
    /* Set the dirty flag according to the lock type */
    if ((This->lastlocktype & DDLOCK_READONLY) == 0)
        glThis->dirty_flag = SURFACE_MEMORY_DIRTY;
}

HRESULT WINAPI
GL_IDirect3DTextureImpl_2_1T_Load(LPDIRECT3DTEXTURE2 iface,
				  LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirect3DTexture2, iface);
    IDirectDrawSurfaceImpl *src_ptr = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture2, lpD3DTexture2);
    IDirectDrawSurfaceImpl *dst_ptr = This;
    HRESULT ret_value = D3D_OK;
   
    TRACE("(%p/%p)->(%p)\n", This, iface, lpD3DTexture2);

    if (((src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) != (dst_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP)) ||
	(src_ptr->surface_desc.u2.dwMipMapCount != dst_ptr->surface_desc.u2.dwMipMapCount)) {
        ERR("Trying to load surfaces with different mip-map counts !\n");
    }

    /* Now loop through all mipmap levels and load all of them... */
    while (1) {
        IDirect3DTextureGLImpl *gl_dst_ptr = (IDirect3DTextureGLImpl *) dst_ptr->tex_private;
	DDSURFACEDESC *src_d, *dst_d;
	
	if (gl_dst_ptr != NULL) {
	    if (gl_dst_ptr->loaded == FALSE) {
	        /* Only check memory for not already loaded texture... */
	        DWORD mem_used = dst_ptr->surface_desc.dwHeight * dst_ptr->surface_desc.u1.lPitch;
		if (This->ddraw_owner->allocate_memory(This->ddraw_owner, mem_used) < 0) {
		    TRACE(" out of virtual memory... Warning application.\n");
		    return D3DERR_TEXTURE_LOAD_FAILED;
		}
	    }
	    gl_dst_ptr->loaded = TRUE;
	}
    
	TRACE(" copying surface %p to surface %p (mipmap level %d)\n", src_ptr, dst_ptr, src_ptr->mipmap_level);

	if ( dst_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD )
	    /* If the surface is not allocated and its location is not yet specified,
	       force it to video memory */ 
	    if ( !(dst_ptr->surface_desc.ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)) )
	        dst_ptr->surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

	/* Suppress the ALLOCONLOAD flag */
	dst_ptr->surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;
    
	/* After seeing some logs, not sure at all about this... */
	if (dst_ptr->palette == NULL) {
	    dst_ptr->palette = src_ptr->palette;
	    if (src_ptr->palette != NULL) IDirectDrawPalette_AddRef(ICOM_INTERFACE(src_ptr->palette, IDirectDrawPalette));
	} else {
	    if (src_ptr->palette != NULL) {
	        PALETTEENTRY palent[256];
		IDirectDrawPalette_GetEntries(ICOM_INTERFACE(src_ptr->palette, IDirectDrawPalette),
					      0, 0, 256, palent);
		IDirectDrawPalette_SetEntries(ICOM_INTERFACE(dst_ptr->palette, IDirectDrawPalette),
					      0, 0, 256, palent);
	    }
	}
	
	/* Copy one surface on the other */
	dst_d = (DDSURFACEDESC *)&(dst_ptr->surface_desc);
	src_d = (DDSURFACEDESC *)&(src_ptr->surface_desc);

	if ((src_d->dwWidth != dst_d->dwWidth) || (src_d->dwHeight != dst_d->dwHeight)) {
	    /* Should also check for same pixel format, u1.lPitch, ... */
	    ERR("Error in surface sizes\n");
	    return D3DERR_TEXTURE_LOAD_FAILED;
	} else {
	    /* LPDIRECT3DDEVICE2 d3dd = (LPDIRECT3DDEVICE2) This->D3Ddevice; */
	    /* I should put a macro for the calculus of bpp */
	  
	    /* Copy also the ColorKeying stuff */
	    if (src_d->dwFlags & DDSD_CKSRCBLT) {
	        dst_d->dwFlags |= DDSD_CKSRCBLT;
		dst_d->ddckCKSrcBlt.dwColorSpaceLowValue = src_d->ddckCKSrcBlt.dwColorSpaceLowValue;
		dst_d->ddckCKSrcBlt.dwColorSpaceHighValue = src_d->ddckCKSrcBlt.dwColorSpaceHighValue;
	    }

	    /* Copy the main memory texture into the surface that corresponds to the OpenGL
	       texture object. */
	    memcpy(dst_d->lpSurface, src_d->lpSurface, src_d->u1.lPitch * src_d->dwHeight);

	    if (gl_dst_ptr != NULL) {
	        /* If the GetHandle was not done, it is an error... */
	        if (gl_dst_ptr->tex_name == 0) ERR("Unbound GL texture !!!\n");

		/* Set this texture as dirty */
		gl_dst_ptr->dirty_flag = SURFACE_MEMORY_DIRTY;
	    }
	}

	if (src_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) {
	    src_ptr = get_sub_mimaplevel(src_ptr);
	} else {
	    src_ptr = NULL;
	}
	if (dst_ptr->surface_desc.ddsCaps.dwCaps & DDSCAPS_MIPMAP) {
	    dst_ptr = get_sub_mimaplevel(dst_ptr);
	} else {
	    dst_ptr = NULL;
	}

	if ((src_ptr == NULL) || (dst_ptr == NULL)) {
	    if (src_ptr != dst_ptr) {
	        ERR(" Loading surface with different mipmap structure !!!\n");
	    }
	    break;
	}
    }

    return ret_value;
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_2_QueryInterface(LPDIRECT3DTEXTURE2 iface,
                                            REFIID riid,
                                            LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDrawSurface7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDrawSurface7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture2, IDirectDrawSurface7, iface),
					      riid,
					      obp);
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_2_AddRef(LPDIRECT3DTEXTURE2 iface)
{
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", iface);
    return IDirectDrawSurface7_AddRef(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture2, IDirectDrawSurface7, iface));
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_2_Release(LPDIRECT3DTEXTURE2 iface)
{
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", iface);
    return IDirectDrawSurface7_Release(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture2, IDirectDrawSurface7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_QueryInterface(LPDIRECT3DTEXTURE iface,
                                            REFIID riid,
                                            LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirectDrawSurface7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirectDrawSurface7_QueryInterface(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirectDrawSurface7, iface),
					      riid,
					      obp);
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_AddRef(LPDIRECT3DTEXTURE iface)
{
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", iface);
    return IDirectDrawSurface7_AddRef(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirectDrawSurface7, iface));
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_Release(LPDIRECT3DTEXTURE iface)
{
    TRACE("(%p)->() thunking to IDirectDrawSurface7 interface.\n", iface);
    return IDirectDrawSurface7_Release(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirectDrawSurface7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_PaletteChanged(LPDIRECT3DTEXTURE iface,
                                            DWORD dwStart,
                                            DWORD dwCount)
{
    TRACE("(%p)->(%08lx,%08lx) thunking to IDirect3DTexture2 interface.\n", iface, dwStart, dwCount);
    return IDirect3DTexture2_PaletteChanged(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, iface),
                                            dwStart,
                                            dwCount);
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_GetHandle(LPDIRECT3DTEXTURE iface,
				       LPDIRECT3DDEVICE lpDirect3DDevice,
				       LPD3DTEXTUREHANDLE lpHandle)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DTexture2 interface.\n", iface, lpDirect3DDevice, lpHandle);
    return IDirect3DTexture2_GetHandle(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, iface),
				       COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice2, lpDirect3DDevice),
				       lpHandle);
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_Load(LPDIRECT3DTEXTURE iface,
				  LPDIRECT3DTEXTURE lpD3DTexture)
{
    TRACE("(%p)->(%p) thunking to IDirect3DTexture2 interface.\n", iface, lpD3DTexture);
    return IDirect3DTexture2_Load(COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, iface),
				  COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirect3DTexture, IDirect3DTexture2, lpD3DTexture));
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DTexture2.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DTexture2) VTABLE_IDirect3DTexture2 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DTextureImpl_2_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DTextureImpl_2_AddRef,
    XCAST(Release) Thunk_IDirect3DTextureImpl_2_Release,
    XCAST(GetHandle) Main_IDirect3DTextureImpl_2_1T_GetHandle,
    XCAST(PaletteChanged) Main_IDirect3DTextureImpl_2_1T_PaletteChanged,
    XCAST(Load) GL_IDirect3DTextureImpl_2_1T_Load,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DTexture.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DTexture) VTABLE_IDirect3DTexture =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DTextureImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DTextureImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DTextureImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DTextureImpl_1_Initialize,
    XCAST(GetHandle) Thunk_IDirect3DTextureImpl_1_GetHandle,
    XCAST(PaletteChanged) Thunk_IDirect3DTextureImpl_1_PaletteChanged,
    XCAST(Load) Thunk_IDirect3DTextureImpl_1_Load,
    XCAST(Unload) Main_IDirect3DTextureImpl_1_Unload,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

HRESULT d3dtexture_create(IDirectDrawImpl *d3d, IDirectDrawSurfaceImpl *surf, BOOLEAN at_creation, 
			  IDirectDrawSurfaceImpl *main)
{
    /* First, initialize the texture vtables... */
    ICOM_INIT_INTERFACE(surf, IDirect3DTexture,  VTABLE_IDirect3DTexture);
    ICOM_INIT_INTERFACE(surf, IDirect3DTexture2, VTABLE_IDirect3DTexture2);
	
    /* Only create all the private stuff if we actually have an OpenGL context.. */
    if (d3d->current_device != NULL) {
        IDirect3DTextureGLImpl *private;

        private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTextureGLImpl));
	if (private == NULL) return DDERR_OUTOFMEMORY;

	private->final_release = surf->final_release;
	private->lock_update = surf->lock_update;
	private->unlock_update = surf->unlock_update;
	private->set_palette = surf->set_palette;
	
	/* If at creation, we can optimize stuff and wait the first 'unlock' to upload a valid stuff to OpenGL.
	   Otherwise, it will be uploaded here (and may be invalid). */
	surf->final_release = gltex_final_release;
	surf->lock_update = gltex_lock_update;
	surf->unlock_update = gltex_unlock_update;
	surf->tex_private = private;
	surf->aux_setcolorkey_cb = gltex_setcolorkey_cb;
	surf->set_palette = gltex_set_palette;

	/* We are the only one to use the aux_blt and aux_bltfast overides, so no need
	   to save those... */
	surf->aux_blt = gltex_blt;
	surf->aux_bltfast = gltex_bltfast;
	
	ENTER_GL();
	if (surf->mipmap_level == 0) {
	    glGenTextures(1, &(private->tex_name));
	    if (private->tex_name == 0) ERR("Error at creation of OpenGL texture ID !\n");
	    TRACE(" GL texture created for surface %p (private data at %p and GL id %d).\n", surf, private, private->tex_name);
	} else {
	    private->tex_name = ((IDirect3DTextureGLImpl *) (main->tex_private))->tex_name;
	    TRACE(" GL texture created for surface %p (private data at %p and GL id reusing id %d from surface %p (%p)).\n",
		  surf, private, private->tex_name, main, main->tex_private);
	}
	LEAVE_GL();

	/* And set the dirty flag accordingly */
	private->dirty_flag = (at_creation == FALSE ? SURFACE_MEMORY_DIRTY : SURFACE_MEMORY);
	private->initial_upload_done = FALSE;
    }

    return D3D_OK;
}
