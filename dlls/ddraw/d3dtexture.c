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

#include "windef.h"
#include "winerror.h"
#include "wine/obj_base.h"
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

#define SNOOP_PALETTED() 									\
      {												\
	FILE *f;										\
	char buf[32];										\
	int x, y;										\
												\
	sprintf(buf, "%ld.pnm", dtpriv->tex_name);						\
	f = fopen(buf, "wb");									\
	fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);			\
	for (y = 0; y < src_d->dwHeight; y++) {							\
	  for (x = 0; x < src_d->dwWidth; x++) {						\
	    unsigned char c = ((unsigned char *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
	    fputc(table[c][0], f);								\
	    fputc(table[c][1], f);								\
	    fputc(table[c][2], f);								\
	  }											\
	}											\
	fclose(f);										\
      }

#define SNOOP_5650()											\
	  {												\
	    FILE *f;											\
	    char buf[32];										\
	    int x, y;											\
	    												\
	    sprintf(buf, "%ld.pnm", dtpriv->tex_name);							\
	    f = fopen(buf, "wb");									\
	    fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);				\
	    for (y = 0; y < src_d->dwHeight; y++) {							\
	      for (x = 0; x < src_d->dwWidth; x++) {							\
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
		fputc((c & 0xF800) >> 8, f);								\
		fputc((c & 0x07E0) >> 3, f);								\
		fputc((c & 0x001F) << 3, f);								\
	      }												\
	    }												\
	    fclose(f);											\
	  }

#define SNOOP_5551()											\
	  {												\
	    FILE *f;											\
	    char buf[32];										\
	    int x, y;											\
	    												\
	    sprintf(buf, "%ld.pnm", dtpriv->tex_name);							\
	    f = fopen(buf, "wb");									\
	    fprintf(f, "P6\n%ld %ld\n255\n", src_d->dwWidth, src_d->dwHeight);				\
	    for (y = 0; y < src_d->dwHeight; y++) {							\
	      for (x = 0; x < src_d->dwWidth; x++) {							\
		unsigned short c = ((unsigned short *) src_d->y.lpSurface)[y * src_d->dwWidth + x];	\
		fputc((c & 0xF800) >> 8, f);								\
		fputc((c & 0x07C0) >> 3, f);								\
		fputc((c & 0x003E) << 2, f);								\
	      }												\
	    }												\
	    fclose(f);											\
	  }
#else
#define SNOOP_PALETTED()
#define SNOOP_5650()
#define SNOOP_5551()
#endif

/*******************************************************************************
 *			   IDirectSurface callback methods
 */
HRESULT WINAPI SetColorKey_cb(IDirect3DTextureImpl *texture, DWORD dwFlags, LPDDCOLORKEY ckey )
{
    DDSURFACEDESC *tex_d;
    GLuint current_texture;
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) texture;
    
    TRACE("(%p) : colorkey callback\n", texture);

    /* Get the texture description */
    tex_d = (DDSURFACEDESC *)&(texture->surface->surface_desc);

    /* Now, save the current texture */
    ENTER_GL();
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

    /* If the GetHandle was not done yet, it's an error */
    if (glThis->tex_name == 0) {
        ERR("Unloaded texture !\n");
	LEAVE_GL();
	return DD_OK;
    }
    glBindTexture(GL_TEXTURE_2D, glThis->tex_name);

    if (tex_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        FIXME("Todo Paletted\n");
    } else if (tex_d->ddpfPixelFormat.dwFlags & DDPF_RGB) {
        if (tex_d->ddpfPixelFormat.u1.dwRGBBitCount == 8) {
	    FIXME("Todo 3_3_2_0\n");
	} else if (tex_d->ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	    if (tex_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000) {
	        /* Now transform the 5_6_5 into a 5_5_5_1 surface to support color keying */
	        unsigned short *dest = (unsigned short *) HeapAlloc(GetProcessHeap(),
								    HEAP_ZERO_MEMORY,
								    tex_d->u1.lPitch * tex_d->dwHeight);
		unsigned short *src = (unsigned short *) tex_d->lpSurface;
		int x, y;
		
		for (y = 0; y < tex_d->dwHeight; y++) {
		    for (x = 0; x < tex_d->dwWidth; x++) {
		        unsigned short cpixel = src[x + y * tex_d->dwWidth];
			
			if ((dwFlags & DDCKEY_SRCBLT) &&
			    (cpixel >= ckey->dwColorSpaceLowValue) &&
			    (cpixel <= ckey->dwColorSpaceHighValue)) /* No alpha bit => this pixel is transparent */
			    dest[x + y * tex_d->dwWidth] = (cpixel & ~0x003F) | ((cpixel & 0x001F) << 1) | 0x0000;
			else                                         /* Alpha bit is set => this pixel will be seen */
			    dest[x + y * tex_d->dwWidth] = (cpixel & ~0x003F) | ((cpixel & 0x001F) << 1) | 0x0001;
		    }
		}

		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RGBA,
			     tex_d->dwWidth, tex_d->dwHeight,
			     0,
			     GL_RGBA,
			     GL_UNSIGNED_SHORT_5_5_5_1,
			     dest);
		
		/* Frees the temporary surface */
		HeapFree(GetProcessHeap(),0,dest);
	    } else if (tex_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000001) {
	        FIXME("Todo 5_5_5_1\n");
	    } else if (tex_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000000F) {
	        FIXME("Todo 4_4_4_4\n");
	    } else {
	        ERR("Unhandled texture format (bad Aplha channel for a 16 bit texture)\n");
	    }
	} else if (tex_d->ddpfPixelFormat.u1.dwRGBBitCount == 24) {
	    FIXME("Todo 8_8_8_0\n");
	} else if (tex_d->ddpfPixelFormat.u1.dwRGBBitCount == 32) {
	    FIXME("Todo 8_8_8_8\n");
	} else {
	    ERR("Unhandled texture format (bad RGB count)\n");
	}
    } else {
        ERR("Unhandled texture format (neither RGB nor INDEX)\n");
    }
    LEAVE_GL();

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_QueryInterface(LPDIRECT3DTEXTURE2 iface,
                                              REFIID riid,
                                              LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    TRACE("(%p/%p)->(%s,%p): stub!\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DTexture2_AddRef(ICOM_INTERFACE(This, IDirect3DTexture2));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DTexture, riid ) ) {
        IDirect3DTexture2_AddRef(ICOM_INTERFACE(This, IDirect3DTexture2));
        *obp = ICOM_INTERFACE(This, IDirect3DTexture);
	TRACE("  Creating IDirect3DTexture interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DTexture2, riid ) ) {
        IDirect3DTexture2_AddRef(ICOM_INTERFACE(This, IDirect3DTexture2));
        *obp = ICOM_INTERFACE(This, IDirect3DTexture2);
	TRACE("  Creating IDirect3DTexture2 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DTextureImpl_2_1T_AddRef(LPDIRECT3DTEXTURE2 iface)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->() incrementing from %lu.\n", This, iface, This->ref);
    return ++(This->ref);
}

ULONG WINAPI
Main_IDirect3DTextureImpl_2_1T_Release(LPDIRECT3DTEXTURE2 iface)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    
    if (!--(This->ref)) {
        /* Release surface */
        IDirectDrawSurface3_Release(ICOM_INTERFACE(This->surface, IDirectDrawSurface3));

	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }

    return This->ref;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_1_Initialize(LPDIRECT3DTEXTURE iface,
                                       LPDIRECT3DDEVICE lpDirect3DDevice,
                                       LPDIRECTDRAWSURFACE lpDDSurface)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture, iface);
    FIXME("(%p/%p)->(%p,%p) no-op...\n", This, iface, lpDirect3DDevice, lpDDSurface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_PaletteChanged(LPDIRECT3DTEXTURE2 iface,
                                              DWORD dwStart,
                                              DWORD dwCount)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->(%08lx,%08lx): stub!\n", This, iface, dwStart, dwCount);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_1_Unload(LPDIRECT3DTEXTURE iface)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture, iface);
    FIXME("(%p/%p)->(): stub!\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_GetHandle(LPDIRECT3DTEXTURE2 iface,
					 LPDIRECT3DDEVICE2 lpDirect3DDevice2,
					 LPD3DTEXTUREHANDLE lpHandle)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->(%p,%p): stub!\n", This, iface, lpDirect3DDevice2, lpHandle);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DTextureImpl_2_1T_Load(LPDIRECT3DTEXTURE2 iface,
				    LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    FIXME("(%p/%p)->(%p): stub!\n", This, iface, lpD3DTexture2);
    return DD_OK;
}

ULONG WINAPI
GL_IDirect3DTextureImpl_2_1T_Release(LPDIRECT3DTEXTURE2 iface)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This;
    FIXME("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    
    if (!--(This->ref)) {
        /* Release surface */
        IDirectDrawSurface3_Release(ICOM_INTERFACE(This->surface, IDirectDrawSurface3));

	/* And delete texture handle */
	ENTER_GL();
	glDeleteTextures(1, &(glThis->tex_name));
	LEAVE_GL();	

	/* And if this texture was the current one, remove it at the device level */
	if (This->d3ddevice != NULL)
	    if (This->d3ddevice->current_texture == This)
	        This->d3ddevice->current_texture = NULL;
	
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }

    return This->ref;
}

HRESULT WINAPI
GL_IDirect3DTextureImpl_2_1T_GetHandle(LPDIRECT3DTEXTURE2 iface,
				       LPDIRECT3DDEVICE2 lpDirect3DDevice2,
				       LPD3DTEXTUREHANDLE lpHandle)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This;
    IDirect3DDeviceImpl *lpDeviceImpl = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice2, lpDirect3DDevice2);
    
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, lpDirect3DDevice2, lpHandle);

    /* The handle is simply the pointer to the implementation structure */
    *lpHandle = (D3DTEXTUREHANDLE) This;

    TRACE(" returning handle %08lx.\n", *lpHandle);
    
    /* Now, bind a new texture */
    This->d3ddevice = lpDeviceImpl;
    ENTER_GL();
    if (glThis->tex_name == 0)
	glGenTextures(1, &(glThis->tex_name));
    LEAVE_GL();

    /* Associate the texture with the device and perform the appropriate AddRef/Release */
    /* FIXME: Is there only one or several textures associated with the device ? */
    if (lpDeviceImpl->current_texture != NULL)
        IDirect3DTexture2_Release(ICOM_INTERFACE(lpDeviceImpl->current_texture, IDirect3DTexture2));           
    IDirect3DTexture2_AddRef(ICOM_INTERFACE(This, IDirect3DTexture2));
    lpDeviceImpl->current_texture = This;

    TRACE("OpenGL texture handle is : %d\n", glThis->tex_name);

    return D3D_OK;
}

/* NOTE : if you experience crashes in this function, you must have a buggy
          version of Mesa. See the file d3dtexture.c for a cure */
HRESULT WINAPI
GL_IDirect3DTextureImpl_2_1T_Load(LPDIRECT3DTEXTURE2 iface,
				  LPDIRECT3DTEXTURE2 lpD3DTexture2)
{
    ICOM_THIS_FROM(IDirect3DTextureImpl, IDirect3DTexture2, iface);
    IDirect3DTextureGLImpl *glThis = (IDirect3DTextureGLImpl *) This;
    IDirect3DTextureImpl *lpD3DTextureImpl = ICOM_OBJECT(IDirect3DTextureImpl, IDirect3DTexture2, lpD3DTexture2);
    DDSURFACEDESC	*src_d, *dst_d;
    static void (*ptr_ColorTableEXT) (GLenum target, GLenum internalformat,
				      GLsizei width, GLenum format, GLenum type, const GLvoid *table) = NULL;
#if 0
    static BOOL color_table_queried = FALSE;
#endif
    
    TRACE("(%p/%p)->(%p)\n", This, iface, lpD3DTexture2);
    TRACE("Copied surface %p to surface %p\n", lpD3DTextureImpl->surface, This->surface);

    if ( This->surface->surface_desc.ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD )
        /* If the surface is not allocated and its location is not yet specified,
	   force it to video memory */ 
        if ( !(This->surface->surface_desc.ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)) )
	    This->surface->surface_desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

    /* Suppress the ALLOCONLOAD flag */
    This->surface->surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_ALLOCONLOAD;

    /* Copy one surface on the other */
    dst_d = (DDSURFACEDESC *)&(This->surface->surface_desc);
    src_d = (DDSURFACEDESC *)&(lpD3DTextureImpl->surface->surface_desc);

    /* Install the callbacks to the destination surface */
    This->surface->texture = This;
    This->surface->SetColorKey_cb = SetColorKey_cb;

    if ((src_d->dwWidth != dst_d->dwWidth) || (src_d->dwHeight != dst_d->dwHeight)) {
        /* Should also check for same pixel format, u1.lPitch, ... */
        ERR("Error in surface sizes\n");
	return D3DERR_TEXTURE_LOAD_FAILED;
    } else {
        /* LPDIRECT3DDEVICE2 d3dd = (LPDIRECT3DDEVICE2) This->D3Ddevice; */
        /* I should put a macro for the calculus of bpp */
	GLuint current_texture;

	/* Copy the main memry texture into the surface that corresponds to the OpenGL
	   texture object. */
	memcpy(dst_d->lpSurface, src_d->lpSurface, src_d->u1.lPitch * src_d->dwHeight);

	ENTER_GL();

	/* Now, load the texture */
	/* d3dd->set_context(d3dd); We need to set the context somehow.... */
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);

	/* If the GetHandle was not done, get the texture name here */
	if (glThis->tex_name == 0)
	    glGenTextures(1, &(glThis->tex_name));
	glBindTexture(GL_TEXTURE_2D, glThis->tex_name);

	if (src_d->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
	  /* ****************
	     Paletted Texture
	     **************** */
	    IDirectDrawPaletteImpl* pal = lpD3DTextureImpl->surface->palette;
	    BYTE table[256][4];
	    int i;

#if 0
	    if (color_table_queried == FALSE) {
	        ptr_ColorTableEXT =
		  ((Mesa_DeviceCapabilities *) ((x11_dd_private *) This->surface->s.ddraw->d->private)->device_capabilities)->ptr_ColorTableEXT;
	    }
#endif

	    if (pal == NULL) {
	        ERR("Palettized texture Loading with a NULL palette !\n");
		LEAVE_GL();
		return D3DERR_TEXTURE_LOAD_FAILED;
	    }

	    /* Get the surface's palette */
	    for (i = 0; i < 256; i++) {
	        table[i][0] = pal->palents[i].peRed;
		table[i][1] = pal->palents[i].peGreen;
		table[i][2] = pal->palents[i].peBlue;
		if ((This->surface->surface_desc.dwFlags & DDSD_CKSRCBLT) &&
		    (i >= This->surface->surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue) &&
		    (i <= This->surface->surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue))
		    table[i][3] = 0x00;
		else
		    table[i][3] = 0xFF;
	    }

	    /* Texture snooping */
	    SNOOP_PALETTED();

	    if (ptr_ColorTableEXT != NULL) {
	        /* use Paletted Texture Extension */
	        ptr_ColorTableEXT(GL_TEXTURE_2D,    /* target */
				  GL_RGBA,          /* internal format */
				  256,              /* table size */
				  GL_RGBA,          /* table format */
				  GL_UNSIGNED_BYTE, /* table type */
				  table);           /* the color table */
		
		glTexImage2D(GL_TEXTURE_2D,       /* target */
			     0,                   /* level */
			     GL_COLOR_INDEX8_EXT, /* internal format */
			     src_d->dwWidth, src_d->dwHeight, /* width, height */
			     0,                   /* border */
			     GL_COLOR_INDEX,      /* texture format */
			     GL_UNSIGNED_BYTE,    /* texture type */
			     src_d->lpSurface); /* the texture */
	    } else {
	        DWORD *surface = (DWORD *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_d->dwWidth * src_d->dwHeight * sizeof(DWORD));
		DWORD i;
		BYTE *src = (BYTE *) src_d->lpSurface, *dst = (BYTE *) surface;
		
		for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
		    BYTE color = *src++;
		    *dst++ = table[color][0];
		    *dst++ = table[color][1];
		    *dst++ = table[color][2];
		    *dst++ = table[color][3];
		}
		
		glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RGBA,
			     src_d->dwWidth, src_d->dwHeight,
			     0,
			     GL_RGBA,
			     GL_UNSIGNED_BYTE,
			     surface);
		
		HeapFree(GetProcessHeap(), 0, surface);
	    }
	} else if (src_d->ddpfPixelFormat.dwFlags & DDPF_RGB) {
	    /* ************
	       RGB Textures
	       ************ */
	    if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 8) {
	        /* **********************
		   GL_UNSIGNED_BYTE_3_3_2
		   ********************** */
	        glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RGB,
			     src_d->dwWidth, src_d->dwHeight,
			     0,
			     GL_RGB,
			     GL_UNSIGNED_BYTE_3_3_2,
			     src_d->lpSurface);
	    } else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 16) {
	        if (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000000) {
		  
		    /* Texture snooping */
		    SNOOP_5650();
		    
		    glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGB,
				 src_d->dwWidth, src_d->dwHeight,
				 0,
				 GL_RGB,
				 GL_UNSIGNED_SHORT_5_6_5,
				 src_d->lpSurface);
		} else if (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00000001) {
		    /* Texture snooping */
		    SNOOP_5551();
		    
		    glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 src_d->dwWidth, src_d->dwHeight,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_SHORT_5_5_5_1,
				 src_d->lpSurface);
		} else if (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x0000000F) {
		    glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 src_d->dwWidth, src_d->dwHeight,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_SHORT_4_4_4_4,
				 src_d->lpSurface);
		} else if (src_d->ddpfPixelFormat.u5.dwRGBAlphaBitMask == 0x00008000) {
		    /* Converting the 1555 format in 5551 packed */
		    WORD *surface = (WORD *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, src_d->dwWidth * src_d->dwHeight * sizeof(WORD));
		    DWORD i;
		    WORD *src = (WORD *) src_d->lpSurface, *dst = surface;
		    
		    for (i = 0; i < src_d->dwHeight * src_d->dwWidth; i++) {
		        *dst++ = (((*src & 0x8000) >> 15) |
				  ((*src & 0x7FFF) <<  1));
			src++;
		    }
		    
		    glTexImage2D(GL_TEXTURE_2D,
				 0,
				 GL_RGBA,
				 src_d->dwWidth, src_d->dwHeight,
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_SHORT_5_5_5_1,
				 surface);
		    
		    HeapFree(GetProcessHeap(), 0, surface);
		} else {
		    ERR("Unhandled texture format (bad Aplha channel for a 16 bit texture)\n");
		}
	    } else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 24) {
	        glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RGB,
			     src_d->dwWidth, src_d->dwHeight,
			     0,
			     GL_RGB,
			     GL_UNSIGNED_BYTE,
			     src_d->lpSurface);
	    } else if (src_d->ddpfPixelFormat.u1.dwRGBBitCount == 32) {
	        glTexImage2D(GL_TEXTURE_2D,
			     0,
			     GL_RGBA,
			     src_d->dwWidth, src_d->dwHeight,
			     0,
			     GL_RGBA,
			     GL_UNSIGNED_BYTE,
			     src_d->lpSurface);
	    } else {
	        ERR("Unhandled texture format (bad RGB count)\n");
	    }
	} else {
	    ERR("Unhandled texture format (neither RGB nor INDEX)\n");
	}
	
	glBindTexture(GL_TEXTURE_2D, current_texture);
	
	LEAVE_GL();
    }

    return D3D_OK;
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_QueryInterface(LPDIRECT3DTEXTURE iface,
                                            REFIID riid,
                                            LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DTexture2 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DTexture2_QueryInterface(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface),
                                            riid,
                                            obp);
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_AddRef(LPDIRECT3DTEXTURE iface)
{
    TRACE("(%p)->() thunking to IDirect3DTexture2 interface.\n", iface);
    return IDirect3DTexture2_AddRef(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface));
}

ULONG WINAPI
Thunk_IDirect3DTextureImpl_1_Release(LPDIRECT3DTEXTURE iface)
{
    TRACE("(%p)->() thunking to IDirect3DTexture2 interface.\n", iface);
    return IDirect3DTexture2_Release(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface));
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_PaletteChanged(LPDIRECT3DTEXTURE iface,
                                            DWORD dwStart,
                                            DWORD dwCount)
{
    TRACE("(%p)->(%08lx,%08lx) thunking to IDirect3DTexture2 interface.\n", iface, dwStart, dwCount);
    return IDirect3DTexture2_PaletteChanged(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface),
                                            dwStart,
                                            dwCount);
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_GetHandle(LPDIRECT3DTEXTURE iface,
				       LPDIRECT3DDEVICE lpDirect3DDevice,
				       LPD3DTEXTUREHANDLE lpHandle)
{
    TRACE("(%p)->(%p,%p) thunking to IDirect3DTexture2 interface.\n", iface, lpDirect3DDevice, lpHandle);
    return IDirect3DTexture2_GetHandle(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface),
				       COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice2, lpDirect3DDevice),
				       lpHandle);
}

HRESULT WINAPI
Thunk_IDirect3DTextureImpl_1_Load(LPDIRECT3DTEXTURE iface,
				  LPDIRECT3DTEXTURE lpD3DTexture)
{
    TRACE("(%p)->(%p) thunking to IDirect3DTexture2 interface.\n", iface, lpD3DTexture);
    return IDirect3DTexture2_Load(COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, iface),
				  COM_INTERFACE_CAST(IDirect3DTextureImpl, IDirect3DTexture, IDirect3DTexture2, lpD3DTexture));
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DTexture2.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DTexture2) VTABLE_IDirect3DTexture2 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DTextureImpl_2_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DTextureImpl_2_1T_AddRef,
    XCAST(Release) GL_IDirect3DTextureImpl_2_1T_Release,
    XCAST(GetHandle) GL_IDirect3DTextureImpl_2_1T_GetHandle,
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




HRESULT d3dtexture_create(IDirect3DTextureImpl **obj, IDirect3DImpl *d3d, IDirectDrawSurfaceImpl *surf)
{
    IDirect3DTextureImpl *object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTextureGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->d3d = d3d;
    object->surface = surf;
    
    ICOM_INIT_INTERFACE(object, IDirect3DTexture,  VTABLE_IDirect3DTexture);
    ICOM_INIT_INTERFACE(object, IDirect3DTexture2, VTABLE_IDirect3DTexture2);

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);
    
    return D3D_OK;
}
