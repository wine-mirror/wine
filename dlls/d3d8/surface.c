/*
 * IDirect3DSurface8 implementation
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"
#include <stdio.h>

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);

/* IDirect3DVolume IUnknown parts follow: */
HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface,REFIID riid,LPVOID *ppobj)
{
    ICOM_THIS(IDirect3DSurface8Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        IDirect3DSurface8Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    ULONG ref = --This->ref;
    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
      HeapFree(GetProcessHeap(), 0, This->allocatedMemory);
      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DSurface8: */
HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8** ppDevice) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    TRACE("(%p) : returning %p\n", This, This->Device);
    *ppDevice = (LPDIRECT3DDEVICE8) This->Device;
    /**
     * Note  Calling this method will increase the internal reference count 
     * on the IDirect3DDevice8 interface. 
     */
    IDirect3DDevice8Impl_AddRef(*ppDevice);
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid, void** ppContainer) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    HRESULT res;
    res = IUnknown_QueryInterface(This->Container, riid, ppContainer);
    if (E_NOINTERFACE == res) { 
      /**
       * If the surface is created using CreateImageSurface, CreateRenderTarget, 
       * or CreateDepthStencilSurface, the surface is considered stand alone. In this case, 
       * GetContainer will return the Direct3D device used to create the surface. 
       */
      res = IUnknown_QueryInterface(This->Container, &IID_IDirect3DDevice8, ppContainer);
    }
    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}
HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);

    TRACE("(%p) : copying into %p\n", This, pDesc);
    memcpy(pDesc, &This->myDesc, sizeof(D3DSURFACE_DESC));
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DSurface8Impl,iface);
  
    /* fixme: should we really lock as such? */

    if (FALSE == This->lockable) {
      ERR("trying to lock unlockable surf@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }

    if (This == This->Device->backBuffer || This == This->Device->renderTarget || This == This->Device->frontBuffer || This->Device->depthStencilBuffer) {
      if (This == This->Device->backBuffer) {
	TRACE("(%p, backBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->allocatedMemory);
      } else if (This == This->Device->frontBuffer) {
	TRACE("(%p, frontBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->allocatedMemory);
      } else if (This == This->Device->renderTarget) {
	TRACE("(%p, renderTarget) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->allocatedMemory);
      } else if (This == This->Device->depthStencilBuffer) {
	TRACE("(%p, stencilBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->allocatedMemory);
      }
    } else {
      TRACE("(%p) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->allocatedMemory);
    }

    pLockedRect->Pitch = This->bytesPerPixel * This->myDesc.Width;  /* Bytes / row */    
    
    if (NULL == pRect) {
      pLockedRect->pBits = This->allocatedMemory;
      This->lockedRect.left   = 0;
      This->lockedRect.top    = 0;
      This->lockedRect.right  = This->myDesc.Width;
      This->lockedRect.bottom = This->myDesc.Height;
      TRACE("Locked Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", &This->lockedRect, This->lockedRect.left, This->lockedRect.top, This->lockedRect.right, This->lockedRect.bottom);
    } else {
      TRACE("Lock Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);
      pLockedRect->pBits = This->allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * This->bytesPerPixel);
      This->lockedRect.left   = pRect->left;
      This->lockedRect.top    = pRect->top;
      This->lockedRect.right  = pRect->right;
      This->lockedRect.bottom = pRect->bottom;
    }


    if (0 == This->myDesc.Usage) { /* classic surface */

      /* Nothing to do ;) */

    } else if (D3DUSAGE_RENDERTARGET & This->myDesc.Usage) { /* render surfaces */
      
      if (This == This->Device->backBuffer || This == This->Device->renderTarget || This == This->Device->frontBuffer) {
	GLint  prev_store;
	GLenum prev_read;
	
	ENTER_GL();

	/**
	 * for render->surface copy begin to begin of allocatedMemory
	 * unlock can be more easy
	 */
	pLockedRect->pBits = This->allocatedMemory;
	
	glFlush();
	vcheckGLcall("glFlush");
	glGetIntegerv(GL_READ_BUFFER, &prev_read);
	vcheckGLcall("glIntegerv");
	glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
	vcheckGLcall("glIntegerv");

	if (This == This->Device->backBuffer) {
	  glReadBuffer(GL_BACK);
	} else if (This == This->Device->frontBuffer || This == This->Device->renderTarget) {
	  glReadBuffer(GL_FRONT);
	} else if (This == This->Device->depthStencilBuffer) {
	  ERR("Stencil Buffer lock unsupported for now\n");
	}
	vcheckGLcall("glReadBuffer");

	{
	  long j;
          GLenum format = D3DFmt2GLFmt(This->Device, This->myDesc.Format);
          GLenum type   = D3DFmt2GLType(This->Device, This->myDesc.Format);
	  for (j = This->lockedRect.top; j < This->lockedRect.bottom - This->lockedRect.top; ++j) {
	    glReadPixels(This->lockedRect.left, 
			 This->lockedRect.bottom - j - 1, 
			 This->lockedRect.right - This->lockedRect.left, 
			 1,
			 format, 
                         type, 
                         (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-This->lockedRect.top)));
	    vcheckGLcall("glReadPixels");
	  }
	}

	glReadBuffer(prev_read);
	vcheckGLcall("glReadBuffer");
	LEAVE_GL();
      } else {
	FIXME("unsupported locking to Rendering surface surf@%p usage(%lu)\n", This, This->myDesc.Usage);
      }

    } else if (D3DUSAGE_DEPTHSTENCIL & This->myDesc.Usage) { /* stencil surfaces */

      FIXME("TODO stencil depth surface locking surf@%p usage(%lu)\n", This, This->myDesc.Usage);

    } else {
      FIXME("unsupported locking to surface surf@%p usage(%lu)\n", This, This->myDesc.Usage);
    }

    if (Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY)) {
      /* Dont dirtify */
    } else {
      /**
       * Dirtify on lock
       * as seen in msdn docs
       */
      IDirect3DSurface8Impl_AddDirtyRect(iface, &This->lockedRect);

      /** Dirtify Container if needed */
      if (NULL != This->Container) {
	IDirect3DBaseTexture8* cont = NULL;
	hr = IUnknown_QueryInterface(This->Container, &IID_IDirect3DBaseTexture8, (void**) &cont);
	
	if (SUCCEEDED(hr) && NULL != cont) {
	  IDirect3DBaseTexture8Impl_SetDirty(cont, TRUE);
	  IDirect3DBaseTexture8_Release(cont);
	  cont = NULL;
	}
      }
    }

    TRACE("returning memory@%p, pitch(%d) dirtyfied(%d)\n", pLockedRect->pBits, pLockedRect->Pitch, This->Dirty);

    This->locked = TRUE;
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    
    if (FALSE == This->locked) {
      ERR("trying to lock unlocked surf@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }

    if (This == This->Device->backBuffer || This == This->Device->frontBuffer || This->Device->depthStencilBuffer) {
      if (This == This->Device->backBuffer) {
	TRACE("(%p, backBuffer) : dirtyfied(%d)\n", This, This->Dirty);
      } else if (This == This->Device->frontBuffer) {
	TRACE("(%p, frontBuffer) : dirtyfied(%d)\n", This, This->Dirty);
      } else if (This == This->Device->depthStencilBuffer) {
	TRACE("(%p, stencilBuffer) : dirtyfied(%d)\n", This, This->Dirty);
      }
    } else {
      TRACE("(%p) : dirtyfied(%d)\n", This, This->Dirty);
    }
    /*TRACE("(%p) see if behavior is correct\n", This);*/

    if (FALSE == This->Dirty) {
      TRACE("(%p) : Not Dirtified so nothing to do, return now\n", This);
      goto unlock_end;
    }

    if (0 == This->myDesc.Usage) { /* classic surface */
      /**
       * nothing to do
       * waiting to reload the surface via IDirect3DDevice8::UpdateTexture
       */
    } else if (D3DUSAGE_RENDERTARGET & This->myDesc.Usage) { /* render surfaces */

      if (This == This->Device->backBuffer || This == This->Device->frontBuffer) {
	GLint  prev_store;
	GLenum prev_draw;
	GLint  prev_rasterpos[4];

	ENTER_GL();
	
	glFlush();
	vcheckGLcall("glFlush");
	glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
	vcheckGLcall("glIntegerv");
	glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
	vcheckGLcall("glIntegerv");
	glGetIntegerv(GL_CURRENT_RASTER_POSITION, &prev_rasterpos[0]);
	vcheckGLcall("glIntegerv");

	if (This == This->Device->backBuffer) {
	  glDrawBuffer(GL_BACK);
	} else if (This == This->Device->frontBuffer) {
	  glDrawBuffer(GL_FRONT);
	}
	vcheckGLcall("glDrawBuffer");

	glRasterPos2i(This->lockedRect.left, This->lockedRect.top);
	vcheckGLcall("glRasterPos2f");
	switch (This->myDesc.Format) {
	case D3DFMT_R5G6B5:
	  {
	    glDrawPixels(This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, This->allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	  }
	  break;
	case D3DFMT_R8G8B8:
	  {
	    glDrawPixels(This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_RGB, GL_UNSIGNED_BYTE, This->allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	  }
	  break;
	case D3DFMT_A8R8G8B8:
	  {
	    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
	    vcheckGLcall("glPixelStorei");
	    glDrawPixels(This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_BGRA, GL_UNSIGNED_BYTE, This->allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	    glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
	    vcheckGLcall("glPixelStorei");
	  }
	  break;
	default:
	  FIXME("Unsupported Format %u in locking func\n", This->myDesc.Format);
	}

	glDrawBuffer(prev_draw);
	vcheckGLcall("glDrawBuffer");
	glRasterPos3iv(&prev_rasterpos[0]);
	vcheckGLcall("glRasterPos3iv");
	LEAVE_GL();

	/** restore clean dirty state */
	IDirect3DSurface8Impl_CleanDirtyRect(iface);

      } else {
	FIXME("unsupported unlocking to Rendering surface surf@%p usage(%lu)\n", This, This->myDesc.Usage);
      }

    } else if (D3DUSAGE_DEPTHSTENCIL & This->myDesc.Usage) { /* stencil surfaces */
    
      if (This == This->Device->depthStencilBuffer) {
	FIXME("TODO stencil depth surface unlocking surf@%p usage(%lu)\n", This, This->myDesc.Usage);
      } else {
	FIXME("unsupported unlocking to StencilDepth surface surf@%p usage(%lu)\n", This, This->myDesc.Usage);
      }

    } else {
      FIXME("unsupported unlocking to surface surf@%p usage(%lu)\n", This, This->myDesc.Usage);
    }

unlock_end:
    This->locked = FALSE;
    memset(&This->lockedRect, 0, sizeof(RECT));
    return D3D_OK;
}

ICOM_VTABLE(IDirect3DSurface8) Direct3DSurface8_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DSurface8Impl_QueryInterface,
    IDirect3DSurface8Impl_AddRef,
    IDirect3DSurface8Impl_Release,
    IDirect3DSurface8Impl_GetDevice,
    IDirect3DSurface8Impl_SetPrivateData,
    IDirect3DSurface8Impl_GetPrivateData,
    IDirect3DSurface8Impl_FreePrivateData,
    IDirect3DSurface8Impl_GetContainer,
    IDirect3DSurface8Impl_GetDesc,
    IDirect3DSurface8Impl_LockRect,
    IDirect3DSurface8Impl_UnlockRect,
};

HRESULT WINAPI IDirect3DSurface8Impl_LoadTexture(LPDIRECT3DSURFACE8 iface, GLenum gl_target, GLenum gl_level) {
  ICOM_THIS(IDirect3DSurface8Impl,iface);

  if ((This->myDesc.Format == D3DFMT_P8 || This->myDesc.Format == D3DFMT_A8P8) 
#if defined(GL_EXT_paletted_texture)
      && !GL_SUPPORT_DEV(EXT_PALETTED_TEXTURE, This->Device)
#endif
      ) {
    /**
     * wanted a paletted texture and not really support it in HW 
     * so software emulation code begin
     */
    UINT i;
    PALETTEENTRY* pal = This->Device->palettes[This->Device->currentPalette];
    VOID* surface = (VOID*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->myDesc.Width * This->myDesc.Height * sizeof(DWORD));
    BYTE* dst = (BYTE*) surface;
    BYTE* src = (BYTE*) This->allocatedMemory;
          
    for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
      BYTE color = *src++;
      *dst++ = pal[color].peRed;
      *dst++ = pal[color].peGreen;
      *dst++ = pal[color].peBlue;
      if (This->myDesc.Format == D3DFMT_A8P8)
	*dst++ = pal[color].peFlags; 
      else
	*dst++ = 0xFF; 
    }
    
    TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	  gl_target,
	  gl_level, 
	  GL_RGBA,
	  This->myDesc.Width, 
	  This->myDesc.Height, 
	  0, 
	  GL_RGBA,
	  GL_UNSIGNED_BYTE,
	  surface);
    glTexImage2D(gl_target,
		 gl_level, 
		 GL_RGBA,
		 This->myDesc.Width,
		 This->myDesc.Height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 surface);
    checkGLcall("glTexImage2D");
    HeapFree(GetProcessHeap(), 0, surface);

    return D3D_OK;    
  }

  if (This->myDesc.Format == D3DFMT_DXT1 || 
      This->myDesc.Format == D3DFMT_DXT3 || 
      This->myDesc.Format == D3DFMT_DXT5) {
#if defined(GL_EXT_texture_compression_s3tc)
    if (GL_SUPPORT_DEV(EXT_TEXTURE_COMPRESSION_S3TC, This->Device)) {
      TRACE("Calling glCompressedTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, sz=%d, Mem=%p\n",
	    gl_target, 
	    gl_level, 
	    D3DFmt2GLIntFmt(This->Device, This->myDesc.Format), 
	    This->myDesc.Width, 
	    This->myDesc.Height, 
	    0, 
	    This->myDesc.Size,
	    This->allocatedMemory);
      glCompressedTexImage2DARB(gl_target, 
				gl_level, 
				D3DFmt2GLIntFmt(This->Device, This->myDesc.Format),
				This->myDesc.Width,
				This->myDesc.Height,
				0,
				This->myDesc.Size,
				This->allocatedMemory);
      checkGLcall("glCommpressedTexTexImage2D");
    }
#endif
  } else {
    TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	  gl_target, 
	  gl_level, 
	  D3DFmt2GLIntFmt(This->Device, This->myDesc.Format),
	  This->myDesc.Width, 
	  This->myDesc.Height, 
	  0, 
	  D3DFmt2GLFmt(This->Device, This->myDesc.Format), 
	  D3DFmt2GLType(This->Device, This->myDesc.Format),
	  This->allocatedMemory);
    glTexImage2D(gl_target, 
		 gl_level,
		 D3DFmt2GLIntFmt(This->Device, This->myDesc.Format),
		 This->myDesc.Width,
		 This->myDesc.Height,
		 0,
		 D3DFmt2GLFmt(This->Device, This->myDesc.Format),
		 D3DFmt2GLType(This->Device, This->myDesc.Format),
		 This->allocatedMemory);
    checkGLcall("glTexImage2D");

#if 0
    {
      static unsigned int gen = 0;
      char buffer[4096];
      ++gen;
      if ((gen % 10) == 0) {
	snprintf(buffer, sizeof(buffer), "/tmp/surface%u_level%u_%u.ppm", gl_target, gl_level, gen);
	IDirect3DSurface8Impl_SaveSnapshot((LPDIRECT3DSURFACE8) This, buffer);
      }
    }
#endif
  }

  return D3D_OK;
}

#include <errno.h>
HRESULT WINAPI IDirect3DSurface8Impl_SaveSnapshot(LPDIRECT3DSURFACE8 iface, const char* filename) {
  FILE* f = NULL;
  ULONG i;
  ICOM_THIS(IDirect3DSurface8Impl,iface);

  f = fopen(filename, "w+");
  if (NULL == f) {
    ERR("opening of %s failed with: %s\n", filename, strerror(errno));
    return D3DERR_INVALIDCALL;
  }

  TRACE("opened %s with format %s\n", filename, debug_d3dformat(This->myDesc.Format));

  fprintf(f, "P6\n%u %u\n255\n", This->myDesc.Width, This->myDesc.Height);
  switch (This->myDesc.Format) {
  case D3DFMT_X8R8G8B8:
  case D3DFMT_A8R8G8B8:
    {
      DWORD color;
      for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
	color = ((DWORD*) This->allocatedMemory)[i];
	fputc((color >> 16) & 0xFF, f);
	fputc((color >>  8) & 0xFF, f);
	fputc((color >>  0) & 0xFF, f);
      }
    }
    break;
  case D3DFMT_R8G8B8:
    {
      BYTE* color;
      for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
	color = ((BYTE*) This->allocatedMemory) + (3 * i);
	fputc((color[0]) & 0xFF, f);
	fputc((color[1]) & 0xFF, f);
	fputc((color[2]) & 0xFF, f);
      }
    }
    break;
  case D3DFMT_A1R5G5B5: 
    {
      WORD color;
      for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
	color = ((WORD*) This->allocatedMemory)[i];
	fputc(((color >> 10) & 0x1F) * 255 / 31, f);
	fputc(((color >>  5) & 0x1F) * 255 / 31, f);
	fputc(((color >>  0) & 0x1F) * 255 / 31, f);
      }
    }
    break;
  case D3DFMT_A4R4G4B4:
    {
      WORD color;
      for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
	color = ((WORD*) This->allocatedMemory)[i];
	fputc(((color >>  8) & 0x0F) * 255 / 15, f);
	fputc(((color >>  4) & 0x0F) * 255 / 15, f);
	fputc(((color >>  0) & 0x0F) * 255 / 15, f);
      }
    }
    break;

  case D3DFMT_R5G6B5: 
    {
      WORD color;
      for (i = 0; i < This->myDesc.Width * This->myDesc.Height; i++) {
	color = ((WORD*) This->allocatedMemory)[i];
	fputc(((color >> 11) & 0x1F) * 255 / 31, f);
	fputc(((color >>  5) & 0x3F) * 255 / 63, f);
	fputc(((color >>  0) & 0x1F) * 255 / 31, f);
      }
    }
    break;
  default: 
    FIXME("Unimplemented dump mode format(%u,%s)\n", This->myDesc.Format, debug_d3dformat(This->myDesc.Format));
  }
  fclose(f);
  return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface8Impl_CleanDirtyRect(LPDIRECT3DSURFACE8 iface) {
  ICOM_THIS(IDirect3DSurface8Impl,iface);
  This->Dirty = FALSE;
  This->dirtyRect.left   = This->myDesc.Width;
  This->dirtyRect.top    = This->myDesc.Height;
  This->dirtyRect.right  = 0;
  This->dirtyRect.bottom = 0;
  return D3D_OK;
}

/**
 * Raphael:
 *   very stupid way to handle multiple dirty rects but it works :)
 */
extern HRESULT WINAPI IDirect3DSurface8Impl_AddDirtyRect(LPDIRECT3DSURFACE8 iface, CONST RECT* pDirtyRect) {
  ICOM_THIS(IDirect3DSurface8Impl,iface);
  This->Dirty = TRUE;
  This->dirtyRect.left   = min(This->dirtyRect.left,   pDirtyRect->left);
  This->dirtyRect.top    = min(This->dirtyRect.top,    pDirtyRect->top);
  This->dirtyRect.right  = max(This->dirtyRect.right,  pDirtyRect->right);
  This->dirtyRect.bottom = max(This->dirtyRect.bottom, pDirtyRect->bottom);
  return D3D_OK;
}
