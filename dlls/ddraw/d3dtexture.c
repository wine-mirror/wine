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
gltex_upload_texture(IDirectDrawSurfaceImpl *This) {
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This->tex_private;
#if 0
    static BOOL color_table_queried = FALSE;
#endif
    static void (*ptr_ColorTableEXT) (GLenum target, GLenum internalformat,
				      GLsizei width, GLenum format, GLenum type, const GLvoid *table) = NULL;
    IDirectDrawSurfaceImpl *surf_ptr;
    GLuint tex_name = glThis->tex_name;
    
    TRACE(" activating OpenGL texture id %d.\n", tex_name);
    glBindTexture(GL_TEXTURE_2D, tex_name);

    if (This->mipmap_level != 0) {
        WARN(" application activating a sub-level of the mipmapping chain (level %d) !\n", This->mipmap_level);
    }
    
    surf_ptr = This;
    while (surf_ptr != NULL) {
        GLenum internal_format = GL_RGBA, format = GL_RGBA, pixel_format = GL_UNSIGNED_BYTE; /* This is only to prevent warnings.. */
	VOID *surface = NULL;
	DDSURFACEDESC *src_d = (DDSURFACEDESC *)&(surf_ptr->surface_desc);
	IDirect3DTextureGLImpl *gl_surf_ptr = (IDirect3DTextureGLImpl *) surf_ptr->tex_private;
	BOOL upload_done = FALSE;
	BOOL error = FALSE;
	
	if (gl_surf_ptr->dirty_flag == FALSE) {
	    TRACE("   - level %d already uploaded.\n", surf_ptr->mipmap_level);
	} else {
	    TRACE("   - uploading texture level %d (initial done = %d).\n",
		  surf_ptr->mipmap_level, gl_surf_ptr->initial_upload_done);

	    /* Texture snooping for the curious :-) */
	    snoop_texture(surf_ptr);
    
	    if (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
	        /* ****************
		   Paletted Texture
		   **************** */
	        IDirectDrawPaletteImpl* pal = surf_ptr->palette;
		BYTE table[256][4];
		int i;
		
#if 0
		if (color_table_queried == FALSE) {
		    ptr_ColorTableEXT =
		      ((Mesa_DeviceCapabilities *) ((x11_dd_private *) surf_ptr->surface->s.ddraw->d->private)->device_capabilities)->ptr_ColorTableEXT;
		}
#endif
		
		if (pal == NULL) {
		    /* Upload a black texture. The real one will be uploaded on palette change */
		    WARN("Palettized texture Loading with a NULL palette !\n");
		    memset(table, 0, 256 * 4);
		} else {
		    /* Get the surface's palette */
		    for (i = 0; i < 256; i++) {
		        table[i][0] = pal->palents[i].peRed;
			table[i][1] = pal->palents[i].peGreen;
			table[i][2] = pal->palents[i].peBlue;
			if ((src_d->dwFlags & DDSD_CKSRCBLT) &&
			    (i >= src_d->ddckCKSrcBlt.dwColorSpaceLowValue) &&
			    (i <= src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
			    /* We should maybe here put a more 'neutral' color than the standard bright purple
			       one often used by application to prevent the nice purple borders when bi-linear
			       filtering is on */
			    table[i][3] = 0x00;
			else
			    table[i][3] = 0xFF;
		    }
		}

		if (ptr_ColorTableEXT != NULL) {
		    /* use Paletted Texture Extension */
		    ptr_ColorTableEXT(GL_TEXTURE_2D,    /* target */
				      GL_RGBA,          /* internal format */
				      256,              /* table size */
				      GL_RGBA,          /* table format */
				      GL_UNSIGNED_BYTE, /* table type */
				      table);           /* the color table */
		    
		    glTexImage2D(GL_TEXTURE_2D,       /* target */
				 surf_ptr->mipmap_level, /* level */
				 GL_COLOR_INDEX8_EXT, /* internal format */
				 src_d->dwWidth, src_d->dwHeight, /* width, height */
				 0,                   /* border */
				 GL_COLOR_INDEX,      /* texture format */
				 GL_UNSIGNED_BYTE,    /* texture type */
				 src_d->lpSurface); /* the texture */
		    
		    upload_done = TRUE;
		} else {
		    DWORD i;
		    BYTE *src = (BYTE *) src_d->lpSurface, *dst;

		    if (glThis->surface_ptr != NULL)
		        surface = glThis->surface_ptr;
		    else
		        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
		    dst = (BYTE *) surface;
		    
		    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
		        BYTE color = *src++;
			*dst++ = table[color][0];
			*dst++ = table[color][1];
			*dst++ = table[color][2];
			*dst++ = table[color][3];
		    }
		    
		    format = GL_RGBA;
		    internal_format = GL_RGBA;
		    pixel_format = GL_UNSIGNED_BYTE;
		}
	    } else if (src_d->ddpfPixelFormat.dwFlags & DDPF_RGB) {
	        /* ************
		   RGB Textures
		   ************ */
	        if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 8) {
		    if ((src_d->ddpfPixelFormat.u2.dwRBitMask == 0xE0) &&
			(src_d->ddpfPixelFormat.u3.dwGBitMask == 0x1C) &&
			(src_d->ddpfPixelFormat.u4.dwBBitMask == 0x03)) {
		        /* **********************
			   GL_UNSIGNED_BYTE_3_3_2
			   ********************** */
		        if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* This texture format will never be used.. So do not care about color keying
			       up until the point in time it will be needed :-) */
			    error = TRUE;
			} else {
			    format = GL_RGB;
			    internal_format = GL_RGB;
			    pixel_format = GL_UNSIGNED_BYTE_3_3_2;
			}
		    } else {
		        error = TRUE;
		    }
		} else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 16) {
		    if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0xF800) &&
			(src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x07E0) &&
			(src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x001F) &&
			(src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000)) {
		        if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* Converting the 565 format in 5551 packed to emulate color-keying.
			       
			       Note : in all these conversion, it would be best to average the averaging
			              pixels to get the color of the pixel that will be color-keyed to
				      prevent 'color bleeding'. This will be done later on if ever it is
				      too visible.
				      
			       Note2: when using color-keying + alpha, are the alpha bits part of the
			              color-space or not ?
			    */
			    DWORD i;
			    WORD *src = (WORD *) src_d->lpSurface, *dst;
			    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			    dst = (WORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = ((color & 0xFFC0) | ((color & 0x1F) << 1));
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= 0x0001;
				dst++;
			    }
			    
			    format = GL_RGBA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
			} else {
			    format = GL_RGB;
			    internal_format = GL_RGB;
			    pixel_format = GL_UNSIGNED_SHORT_5_6_5;
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0xF800) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x07C0) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x003E) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0001)) {
		        format = GL_RGBA;
			internal_format = GL_RGBA;
			pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* Change the alpha value of the color-keyed pixels to emulate color-keying. */
			    DWORD i;
			    WORD *src = (WORD *) src_d->lpSurface, *dst;
			    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			    dst = (WORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = color & 0xFFFE;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= color & 0x0001;
				dst++;
			    }
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0xF000) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x0F00) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x00F0) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x000F)) {
		        format = GL_RGBA;
			internal_format = GL_RGBA;
			pixel_format = GL_UNSIGNED_SHORT_4_4_4_4;
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* Change the alpha value of the color-keyed pixels to emulate color-keying. */
			    DWORD i;
			    WORD *src = (WORD *) src_d->lpSurface, *dst;
		    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			    dst = (WORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = color & 0xFFF0;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= color & 0x000F;
				dst++;
			    }
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x0F00) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x00F0) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x000F) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0xF000)) {
		        /* Move the four Alpha bits... */
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    DWORD i;
			    WORD *src = (WORD *) src_d->lpSurface, *dst;
			
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			    dst = surface;
			    
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = (color & 0x0FFF) << 4;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= (color & 0xF000) >> 12;
				dst++;
			    }
			    format = GL_RGBA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_SHORT_4_4_4_4;
			} else {
			    format = GL_BGRA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_SHORT_4_4_4_4_REV;
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x7C00) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x03E0) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x001F) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x8000)) {
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    DWORD i;
			    WORD *src = (WORD *) src_d->lpSurface, *dst;
			
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			    dst = (WORD *) surface;
			
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = (color & 0x7FFF) << 1;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= (color & 0x8000) >> 15;
				dst++;
			    }
			    format = GL_RGBA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
			} else {
			    format = GL_BGRA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x7C00) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x03E0) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x001F) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000)) {
		        /* Converting the 0555 format in 5551 packed */
		        DWORD i;
			WORD *src = (WORD *) src_d->lpSurface, *dst;
			
			if (glThis->surface_ptr != NULL)
			    surface = glThis->surface_ptr;
			else
			    surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
			dst = (WORD *) surface;
			
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst = (color & 0x7FFF) << 1;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= 0x0001;
				dst++;
			    }
			} else {
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        WORD color = *src++;
				*dst++ = ((color & 0x7FFF) << 1) | 0x0001;
			    }
			}
			
			format = GL_RGBA;
			internal_format = GL_RGBA;
			pixel_format = GL_UNSIGNED_SHORT_5_5_5_1;
		    } else {
		        error = TRUE;
		    }
		} else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 24) {
		    if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x00FF0000) &&
			(src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x0000FF00) &&
			(src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x000000FF) &&
			(src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000)) {
		        if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* This is a pain :-) */
			    DWORD i;
			    BYTE *src = (BYTE *) src_d->lpSurface;
			    DWORD *dst;
			    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
			    dst = (DWORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        DWORD color = *((DWORD *) src) & 0x00FFFFFF;
				src += 3;
				*dst = *src++ << 8;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= 0xFF;
				dst++;
			    }
			    format = GL_RGBA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_INT_8_8_8_8;
			} else {
			    format = GL_BGR;
			    internal_format = GL_RGB;
			    pixel_format = GL_UNSIGNED_BYTE;
			}
		    } else {
		        error = TRUE;
		    }
		} else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 32) {
		    if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0xFF000000) &&
			(src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x00FF0000) &&
			(src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x0000FF00) &&
			(src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x000000FF)) {
		        if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    /* Just use the alpha component to handle color-keying... */
			    DWORD i;
			    DWORD *src = (DWORD *) src_d->lpSurface, *dst;
			    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
						    src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
			    
			    dst = (DWORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        DWORD color = *src++;
				*dst = color & 0xFFFFFF00;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= color & 0x000000FF;
				dst++;
			    }
			}
			format = GL_RGBA;
			internal_format = GL_RGBA;
			pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x00FF0000) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x0000FF00) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x000000FF) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0xFF000000)) {
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    DWORD i;
			    DWORD *src = (DWORD *) src_d->lpSurface, *dst;
			    
			    if (glThis->surface_ptr != NULL)
			        surface = glThis->surface_ptr;
			    else
			        surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
			
			    dst = (DWORD *) surface;
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        DWORD color = *src++;
				*dst = (color & 0x00FFFFFF) << 8;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= (color & 0xFF000000) >> 24;
				dst++;
			    }
			    format = GL_RGBA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_INT_8_8_8_8;
			} else {
			    format = GL_BGRA;
			    internal_format = GL_RGBA;
			    pixel_format = GL_UNSIGNED_INT_8_8_8_8_REV;
			}
		    } else if ((src_d->ddpfPixelFormat.u2.dwRBitMask ==        0x00FF0000) &&
			       (src_d->ddpfPixelFormat.u3.dwGBitMask ==        0x0000FF00) &&
			       (src_d->ddpfPixelFormat.u4.dwBBitMask ==        0x000000FF) &&
			       (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000)) {
		        /* Just add an alpha component and handle color-keying... */
		        DWORD i;
			DWORD *src = (DWORD *) src_d->lpSurface, *dst;
			
			if (glThis->surface_ptr != NULL)
			    surface = glThis->surface_ptr;
			else
			    surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
			dst = (DWORD *) surface;
			
			if (src_d->dwFlags & DDSD_CKSRCBLT) {
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        DWORD color = *src++;
				*dst = color << 8;
				if ((color < src_d->ddckCKSrcBlt.dwColorSpaceLowValue) ||
				    (color > src_d->ddckCKSrcBlt.dwColorSpaceHighValue))
				    *dst |= 0xFF;
				dst++;
			    }
			} else {
			    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
			        *dst++ = (*src++ << 8) | 0xFF;
			    }
			}
			format = GL_RGBA;
			internal_format = GL_RGBA;
			pixel_format = GL_UNSIGNED_INT_8_8_8_8;
		    } else {
		        error = TRUE;
		    }
		} else {
		    error = TRUE;
		}
	    } else {
	        error = TRUE;
	    } 
	    
	    if ((upload_done == FALSE) && (error == FALSE)) {
	        if (gl_surf_ptr->initial_upload_done == FALSE) {
		    glTexImage2D(GL_TEXTURE_2D,
				 surf_ptr->mipmap_level,
				 internal_format,
				 src_d->dwWidth, src_d->dwHeight,
				 0,
				 format,
				 pixel_format,
				 surface == NULL ? src_d->lpSurface : surface);
		    gl_surf_ptr->initial_upload_done = TRUE;
		} else {
		    glTexSubImage2D(GL_TEXTURE_2D,
				    surf_ptr->mipmap_level,
				    0, 0,
				    src_d->dwWidth, src_d->dwHeight,
				    format,
				    pixel_format,
				    surface == NULL ? src_d->lpSurface : surface);
		}
		gl_surf_ptr->dirty_flag = FALSE;
		
		/* And store the surface pointer for future reuse.. */
		if (surface)
		    glThis->surface_ptr = surface;
	    } else if (error == TRUE) {
	        if (ERR_ON(ddraw)) {
		    ERR("  unsupported pixel format for textures : \n");
		    DDRAW_dump_pixelformat(&src_d->ddpfPixelFormat);
		}
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

    glThis->dirty_flag = TRUE;
    /* TODO: check color-keying on mipmapped surfaces... */
    
    return DD_OK;
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
    
    /* First call the previous set_palette function */
    glThis->set_palette(This, pal);
    
    /* And set the dirty flag */
    glThis->dirty_flag = TRUE;

    /* TODO: check palette on mipmapped surfaces... */
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
        glThis->dirty_flag = TRUE;
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
		gl_dst_ptr->dirty_flag = TRUE;
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

HRESULT d3dtexture_create(IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *surf, BOOLEAN at_creation, 
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
	private->dirty_flag = (at_creation == FALSE);
	private->initial_upload_done = FALSE;
    }

    return D3D_OK;
}
