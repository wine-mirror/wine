/*
 * IDirect3DSurface8 implementation
 *
 * Copyright 2005 Oliver Stieber
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DSurface8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    InterlockedIncrement(&D3D8_SURFACE(This)->resource.ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DSurface_Release(This->wineD3DSurface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    else
	InterlockedDecrement(&D3D8_SURFACE(This)->resource.ref);
    
    return ref;
}

/* IDirect3DSurface8 IDirect3DResource8 Interface follow: */
HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
}

/* IDirect3DSurface8 Interface follow: */
HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid, void **ppContainer) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT res;
    res = IUnknown_QueryInterface(D3D8_SURFACE(This)->container, riid, ppContainer);
    if (E_NOINTERFACE == res) { 
      /**
       * If the surface is created using CreateImageSurface, CreateRenderTarget, 
       * or CreateDepthStencilSurface, the surface is considered stand alone. In this case, 
       * GetContainer will return the Direct3D device used to create the surface. 
       */
      res = IUnknown_QueryInterface(D3D8_SURFACE(This)->container, &IID_IDirect3DDevice8, ppContainer);
    }
    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}

HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    UINT                   tmpInt = -1;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d8 structures differ, pass in ptrs to where data needs to go */
    memset(&wined3ddesc, 0, sizeof(wined3ddesc));
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = &pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.MultiSampleType     = &pDesc->MultiSampleType;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
}

#define D3D8_SURFACE_GET_DEVICE(a) ((IDirect3DDevice8Impl*)((IDirect3DTexture8Impl*)(D3D8_SURFACE(a)->container))->Device)
HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
  
    /* fixme: should we really lock as such? */
    if (D3D8_SURFACE(This)->inTexture && D3D8_SURFACE(This)->inPBuffer) {
	FIXME("Warning: Surface is in texture memory or pbuffer\n");
	D3D8_SURFACE(This)->inTexture = 0;
	D3D8_SURFACE(This)->inPBuffer = 0;
    }
    
    if (FALSE == D3D8_SURFACE(This)->lockable) {
      /* Note: UpdateTextures calls CopyRects which calls this routine to populate the 
            texture regions, and since the destination is an unlockable region we need
            to tolerate this                                                           */
      TRACE("Warning: trying to lock unlockable surf@%p\n", This);  
      /*return D3DERR_INVALIDCALL; */
    }

    if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget || This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer || D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer) {
      if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer) {
	TRACE("(%p, backBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, D3D8_SURFACE(This)->resource.allocatedMemory);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer) {
	TRACE("(%p, frontBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, D3D8_SURFACE(This)->resource.allocatedMemory);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
	TRACE("(%p, renderTarget) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, D3D8_SURFACE(This)->resource.allocatedMemory);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer) {
	TRACE("(%p, stencilBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, D3D8_SURFACE(This)->resource.allocatedMemory);
      }
    } else {
      TRACE("(%p) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, D3D8_SURFACE(This)->resource.allocatedMemory);
    }

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
         where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)      
          ie pitch = (width/4) * bytes per block                                  */
    if (D3D8_SURFACE(This)->resource.format == D3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        pLockedRect->Pitch = (D3D8_SURFACE(This)->currentDesc.Width/4) * 8;
    else if (D3D8_SURFACE(This)->resource.format == D3DFMT_DXT2 || D3D8_SURFACE(This)->resource.format == D3DFMT_DXT3 ||
             D3D8_SURFACE(This)->resource.format == D3DFMT_DXT4 || D3D8_SURFACE(This)->resource.format == D3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        pLockedRect->Pitch = (D3D8_SURFACE(This)->currentDesc.Width/4) * 16;
    else
        pLockedRect->Pitch = D3D8_SURFACE(This)->bytesPerPixel * D3D8_SURFACE(This)->currentDesc.Width;  /* Bytes / row */    

    if (NULL == pRect) {
      pLockedRect->pBits = D3D8_SURFACE(This)->resource.allocatedMemory;
      D3D8_SURFACE(This)->lockedRect.left   = 0;
      D3D8_SURFACE(This)->lockedRect.top    = 0;
      D3D8_SURFACE(This)->lockedRect.right  = D3D8_SURFACE(This)->currentDesc.Width;
      D3D8_SURFACE(This)->lockedRect.bottom = D3D8_SURFACE(This)->currentDesc.Height;
      TRACE("Locked Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", &D3D8_SURFACE(This)->lockedRect, D3D8_SURFACE(This)->lockedRect.left, D3D8_SURFACE(This)->lockedRect.top, D3D8_SURFACE(This)->lockedRect.right, D3D8_SURFACE(This)->lockedRect.bottom);
    } else {
      TRACE("Lock Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);

      if (D3D8_SURFACE(This)->resource.format == D3DFMT_DXT1) { /* DXT1 is half byte per pixel */
          pLockedRect->pBits = D3D8_SURFACE(This)->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + ((pRect->left * D3D8_SURFACE(This)->bytesPerPixel/2));
      } else {
          pLockedRect->pBits = D3D8_SURFACE(This)->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * D3D8_SURFACE(This)->bytesPerPixel);
      }
      D3D8_SURFACE(This)->lockedRect.left   = pRect->left;
      D3D8_SURFACE(This)->lockedRect.top    = pRect->top;
      D3D8_SURFACE(This)->lockedRect.right  = pRect->right;
      D3D8_SURFACE(This)->lockedRect.bottom = pRect->bottom;
    }


    if (0 == D3D8_SURFACE(This)->resource.usage) { /* classic surface */

      /* Nothing to do ;) */

    } else if (D3DUSAGE_RENDERTARGET & D3D8_SURFACE(This)->resource.usage && !(Flags&D3DLOCK_DISCARD)) { /* render surfaces */
      
      if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget || This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer) {
	GLint  prev_store;
	GLint  prev_read;
	
	ENTER_GL();

	/**
	 * for render->surface copy begin to begin of allocatedMemory
	 * unlock can be more easy
	 */
	pLockedRect->pBits = D3D8_SURFACE(This)->resource.allocatedMemory;
	
	glFlush();
	vcheckGLcall("glFlush");
	glGetIntegerv(GL_READ_BUFFER, &prev_read);
	vcheckGLcall("glIntegerv");
	glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
	vcheckGLcall("glIntegerv");

	if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer) {
	  glReadBuffer(GL_BACK);
	} else if (This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
	  glReadBuffer(GL_FRONT);
	} else if (This == D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer) {
	  ERR("Stencil Buffer lock unsupported for now\n");
	}
	vcheckGLcall("glReadBuffer");

	{
	  long j;
          GLenum format = D3DFmt2GLFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format);
          GLenum type   = D3DFmt2GLType(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format);
	  for (j = D3D8_SURFACE(This)->lockedRect.top; j < D3D8_SURFACE(This)->lockedRect.bottom - D3D8_SURFACE(This)->lockedRect.top; ++j) {
	    glReadPixels(D3D8_SURFACE(This)->lockedRect.left, 
			 D3D8_SURFACE(This)->lockedRect.bottom - j - 1, 
			 D3D8_SURFACE(This)->lockedRect.right - D3D8_SURFACE(This)->lockedRect.left, 
			 1,
			 format, 
                         type, 
                         (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-D3D8_SURFACE(This)->lockedRect.top)));
	    vcheckGLcall("glReadPixels");
	  }
	}

	glReadBuffer(prev_read);
	vcheckGLcall("glReadBuffer");

	LEAVE_GL();

      } else {
	FIXME("unsupported locking to Rendering surface surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
      }

    } else if (D3DUSAGE_DEPTHSTENCIL & D3D8_SURFACE(This)->resource.usage) { /* stencil surfaces */

      FIXME("TODO stencil depth surface locking surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);

    } else {
      FIXME("unsupported locking to surface surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
    }

    if (Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY)) {
      /* Don't dirtify */
    } else {
      /**
       * Dirtify on lock
       * as seen in msdn docs
       */
      IWineD3DSurface_AddDirtyRect(This->wineD3DSurface, &D3D8_SURFACE(This)->lockedRect);

      /** Dirtify Container if needed */
      if (NULL != D3D8_SURFACE(This)->container) {
	IDirect3DBaseTexture8* cont = NULL;
	hr = IUnknown_QueryInterface(D3D8_SURFACE(This)->container, &IID_IDirect3DBaseTexture8, (void**) &cont);
	
	if (SUCCEEDED(hr) && NULL != cont) {
	  IDirect3DBaseTexture8Impl_SetDirty(cont, TRUE);
	  IDirect3DBaseTexture8_Release(cont);
	  cont = NULL;
	}
      }
    }

    TRACE("returning memory@%p, pitch(%d) dirtyfied(%d)\n", pLockedRect->pBits, pLockedRect->Pitch, D3D8_SURFACE(This)->Dirty);

    D3D8_SURFACE(This)->locked = TRUE;
    return D3D_OK;
}

HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    GLint skipBytes = 0;
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    if (FALSE == D3D8_SURFACE(This)->locked) {
      ERR("trying to Unlock an unlocked surf@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }

    if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer || D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
      if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer) {
	TRACE("(%p, backBuffer) : dirtyfied(%d)\n", This, D3D8_SURFACE(This)->Dirty);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer) {
	TRACE("(%p, frontBuffer) : dirtyfied(%d)\n", This, D3D8_SURFACE(This)->Dirty);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer) {
	TRACE("(%p, stencilBuffer) : dirtyfied(%d)\n", This, D3D8_SURFACE(This)->Dirty);
      } else if (This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
	TRACE("(%p, renderTarget) : dirtyfied(%d)\n", This, D3D8_SURFACE(This)->Dirty);
      }
    } else {
      TRACE("(%p) : dirtyfied(%d)\n", This, D3D8_SURFACE(This)->Dirty);
    }

    if (FALSE == D3D8_SURFACE(This)->Dirty) {
      TRACE("(%p) : Not Dirtified so nothing to do, return now\n", This);
      goto unlock_end;
    }

    if (0 == D3D8_SURFACE(This)->resource.usage) { /* classic surface */
      /**
       * nothing to do
       * waiting to reload the surface via IDirect3DDevice8::UpdateTexture
       */
    } else if (D3DUSAGE_RENDERTARGET & D3D8_SURFACE(This)->resource.usage) { /* render surfaces */

      if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
	GLint  prev_store;
	GLint  prev_draw;
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
        glPixelZoom(1.0, -1.0);
        vcheckGLcall("glPixelZoom");

        /* glDrawPixels transforms the raster position as though it was a vertex -
           we want to draw at screen position 0,0 - Set up ortho (rhw) mode as   
           per drawprim (and leave set - it will sort itself out due to last_was_rhw */
        if (!D3D8_SURFACE_GET_DEVICE(This)->last_was_rhw) {

            double X, Y, height, width, minZ, maxZ;
            D3D8_SURFACE_GET_DEVICE(This)->last_was_rhw = TRUE;

            /* Transformed already into viewport coordinates, so we do not need transform
               matrices. Reset all matrices to identity and leave the default matrix in world 
               mode.                                                                         */
            glMatrixMode(GL_MODELVIEW);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            checkGLcall("glLoadIdentity");

            glMatrixMode(GL_PROJECTION);
            checkGLcall("glMatrixMode");
            glLoadIdentity();
            checkGLcall("glLoadIdentity");

            /* Set up the viewport to be full viewport */
            X      = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.X;
            Y      = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.Y;
            height = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.Height;
            width  = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.Width;
            minZ   = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.MinZ;
            maxZ   = D3D8_SURFACE_GET_DEVICE(This)->StateBlock->viewport.MaxZ;
            TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
            glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
            checkGLcall("glOrtho");

            /* Window Coord 0 is the middle of the first pixel, so translate by half
               a pixel (See comment above glTranslate below)                         */
            glTranslatef(0.5, 0.5, 0);
            checkGLcall("glTranslatef(0.5, 0.5, 0)");
        }

	if (This == D3D8_SURFACE_GET_DEVICE(This)->backBuffer) {
	  glDrawBuffer(GL_BACK);
	} else if (This == D3D8_SURFACE_GET_DEVICE(This)->frontBuffer || This == D3D8_SURFACE_GET_DEVICE(This)->renderTarget) {
	  glDrawBuffer(GL_FRONT);
	}
	vcheckGLcall("glDrawBuffer");

    /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &skipBytes);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, D3D8_SURFACE(This)->currentDesc.Width);

    /* And back buffers are not blended */
    glDisable(GL_BLEND);

	glRasterPos3i(D3D8_SURFACE(This)->lockedRect.left, D3D8_SURFACE(This)->lockedRect.top, 1);
	vcheckGLcall("glRasterPos2f");
	switch (D3D8_SURFACE(This)->resource.format) {
	case D3DFMT_R5G6B5:
	  {
	    glDrawPixels(D3D8_SURFACE(This)->lockedRect.right - D3D8_SURFACE(This)->lockedRect.left, (D3D8_SURFACE(This)->lockedRect.bottom - D3D8_SURFACE(This)->lockedRect.top)-1,
			 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, D3D8_SURFACE(This)->resource.allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	  }
	  break;
	case D3DFMT_R8G8B8:
	  {
	    glDrawPixels(D3D8_SURFACE(This)->lockedRect.right - D3D8_SURFACE(This)->lockedRect.left, (D3D8_SURFACE(This)->lockedRect.bottom - D3D8_SURFACE(This)->lockedRect.top)-1,
			 GL_RGB, GL_UNSIGNED_BYTE, D3D8_SURFACE(This)->resource.allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	  }
	  break;
	case D3DFMT_A8R8G8B8:
	  {
	    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
	    vcheckGLcall("glPixelStorei");
	    glDrawPixels(D3D8_SURFACE(This)->lockedRect.right - D3D8_SURFACE(This)->lockedRect.left, (D3D8_SURFACE(This)->lockedRect.bottom - D3D8_SURFACE(This)->lockedRect.top)-1,
			 GL_BGRA, GL_UNSIGNED_BYTE, D3D8_SURFACE(This)->resource.allocatedMemory);
	    vcheckGLcall("glDrawPixels");
	    glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
	    vcheckGLcall("glPixelStorei");
	  }
	  break;
	default:
	  FIXME("Unsupported Format %u in locking func\n", D3D8_SURFACE(This)->resource.format);
	}

        glPixelZoom(1.0,1.0);
        vcheckGLcall("glPixelZoom");
	glDrawBuffer(prev_draw);
	vcheckGLcall("glDrawBuffer");
	glRasterPos3iv(&prev_rasterpos[0]);
	vcheckGLcall("glRasterPos3iv");

    /* Reset to previous pack row length / blending state */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, skipBytes);
    if (D3D8_SURFACE_GET_DEVICE(This)->StateBlock->renderstate[D3DRS_ALPHABLENDENABLE]) glEnable(GL_BLEND);

	LEAVE_GL();

	/** restore clean dirty state */
	IWineD3DSurface_CleanDirtyRect(This->wineD3DSurface);

      } else {
	FIXME("unsupported unlocking to Rendering surface surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
      }

    } else if (D3DUSAGE_DEPTHSTENCIL & D3D8_SURFACE(This)->resource.usage) { /* stencil surfaces */
    
      if (This == D3D8_SURFACE_GET_DEVICE(This)->depthStencilBuffer) {
	FIXME("TODO stencil depth surface unlocking surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
      } else {
	FIXME("unsupported unlocking to StencilDepth surface surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
      }

    } else {
      FIXME("unsupported unlocking to surface surf@%p usage(%lu)\n", This, D3D8_SURFACE(This)->resource.usage);
    }

unlock_end:
    D3D8_SURFACE(This)->locked = FALSE;
    memset(&D3D8_SURFACE(This)->lockedRect, 0, sizeof(RECT));
    return D3D_OK;
}

const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface8Impl_QueryInterface,
    IDirect3DSurface8Impl_AddRef,
    IDirect3DSurface8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DSurface8Impl_GetDevice,
    IDirect3DSurface8Impl_SetPrivateData,
    IDirect3DSurface8Impl_GetPrivateData,
    IDirect3DSurface8Impl_FreePrivateData,
    /* IDirect3DSurface8 */
    IDirect3DSurface8Impl_GetContainer,
    IDirect3DSurface8Impl_GetDesc,
    IDirect3DSurface8Impl_LockRect,
    IDirect3DSurface8Impl_UnlockRect
};

HRESULT WINAPI IDirect3DSurface8Impl_LoadTexture(LPDIRECT3DSURFACE8 iface, UINT gl_target, UINT gl_level) {
  IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
  if (D3D8_SURFACE(This)->inTexture)
    return D3D_OK;
  if (D3D8_SURFACE(This)->inPBuffer) {
    ENTER_GL();
    if (gl_level != 0)
      FIXME("Surface in texture is only supported for level 0\n");
    else if (D3D8_SURFACE(This)->resource.format == D3DFMT_P8 || D3D8_SURFACE(This)->resource.format == D3DFMT_A8P8 ||
        D3D8_SURFACE(This)->resource.format == D3DFMT_DXT1 || D3D8_SURFACE(This)->resource.format == D3DFMT_DXT2 ||
        D3D8_SURFACE(This)->resource.format == D3DFMT_DXT3 || D3D8_SURFACE(This)->resource.format == D3DFMT_DXT4 ||
        D3D8_SURFACE(This)->resource.format == D3DFMT_DXT5)
      FIXME("Format %d not supported\n", D3D8_SURFACE(This)->resource.format);
    else {
	glCopyTexImage2D(gl_target,
			 0,
			 D3DFmt2GLIntFmt(D3D8_SURFACE_GET_DEVICE(This),
		         D3D8_SURFACE(This)->resource.format),
			 0,
			 0,/*This->surfaces[j][i]->myDesc.Height-1,*/
			 D3D8_SURFACE(This)->currentDesc.Width,
			 D3D8_SURFACE(This)->currentDesc.Height,
			 0);
	TRACE("Updating target %d\n", gl_target);
	D3D8_SURFACE(This)->inTexture = TRUE;
    }
    LEAVE_GL();
    return D3D_OK;
  }
  
  if ((D3D8_SURFACE(This)->resource.format == D3DFMT_P8 || D3D8_SURFACE(This)->resource.format == D3DFMT_A8P8) && 
      !GL_SUPPORT_DEV(EXT_PALETTED_TEXTURE, D3D8_SURFACE_GET_DEVICE(This))) {
    /**
     * wanted a paletted texture and not really support it in HW 
     * so software emulation code begin
     */
    UINT i;
    PALETTEENTRY* pal = D3D8_SURFACE_GET_DEVICE(This)->palettes[D3D8_SURFACE_GET_DEVICE(This)->currentPalette];
    VOID* surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, D3D8_SURFACE(This)->currentDesc.Width * D3D8_SURFACE(This)->currentDesc.Height * sizeof(DWORD));
    BYTE* dst = surface;
    BYTE* src = (BYTE*) D3D8_SURFACE(This)->resource.allocatedMemory;
          
    for (i = 0; i < D3D8_SURFACE(This)->currentDesc.Width * D3D8_SURFACE(This)->currentDesc.Height; i++) {
      BYTE color = *src++;
      *dst++ = pal[color].peRed;
      *dst++ = pal[color].peGreen;
      *dst++ = pal[color].peBlue;
      if (D3D8_SURFACE(This)->resource.format == D3DFMT_A8P8)
	*dst++ = pal[color].peFlags; 
      else
	*dst++ = 0xFF; 
    }

    ENTER_GL();
    
    TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	  gl_target,
	  gl_level, 
	  GL_RGBA,
	  D3D8_SURFACE(This)->currentDesc.Width, 
	  D3D8_SURFACE(This)->currentDesc.Height, 
	  0, 
	  GL_RGBA,
	  GL_UNSIGNED_BYTE,
	  surface);
    glTexImage2D(gl_target,
		 gl_level, 
		 GL_RGBA,
		 D3D8_SURFACE(This)->currentDesc.Width,
		 D3D8_SURFACE(This)->currentDesc.Height,
		 0,
		 GL_RGBA,
		 GL_UNSIGNED_BYTE,
		 surface);
    checkGLcall("glTexImage2D");
    HeapFree(GetProcessHeap(), 0, surface);

    LEAVE_GL();

    return D3D_OK;    
  }

  if (D3D8_SURFACE(This)->resource.format == D3DFMT_DXT1 || 
      D3D8_SURFACE(This)->resource.format == D3DFMT_DXT2 || 
      D3D8_SURFACE(This)->resource.format == D3DFMT_DXT3 || 
      D3D8_SURFACE(This)->resource.format == D3DFMT_DXT4 || 
      D3D8_SURFACE(This)->resource.format == D3DFMT_DXT5) {
    if (GL_SUPPORT_DEV(EXT_TEXTURE_COMPRESSION_S3TC, D3D8_SURFACE_GET_DEVICE(This))) {
      TRACE("Calling glCompressedTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, sz=%d, Mem=%p\n",
	    gl_target, 
	    gl_level, 
	    D3DFmt2GLIntFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format), 
	    D3D8_SURFACE(This)->currentDesc.Width, 
	    D3D8_SURFACE(This)->currentDesc.Height, 
	    0, 
	    D3D8_SURFACE(This)->resource.size,
	    D3D8_SURFACE(This)->resource.allocatedMemory);
      
      ENTER_GL();

      GL_EXTCALL_DEV(glCompressedTexImage2DARB, D3D8_SURFACE_GET_DEVICE(This))(gl_target, 
							      gl_level, 
							      D3DFmt2GLIntFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format),
							      D3D8_SURFACE(This)->currentDesc.Width,
							      D3D8_SURFACE(This)->currentDesc.Height,
							      0,
							      D3D8_SURFACE(This)->resource.size,
							      D3D8_SURFACE(This)->resource.allocatedMemory);
      checkGLcall("glCommpressedTexTexImage2D");

      LEAVE_GL();
    } else {
      FIXME("Using DXT1/3/5 without advertized support\n");
    }
  } else {

    TRACE("Calling glTexImage2D %x i=%d, d3dfmt=%s, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
	  gl_target, 
	  gl_level, 
	  debug_d3dformat(D3D8_SURFACE(This)->resource.format),
	  D3DFmt2GLIntFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format), 
	  D3D8_SURFACE(This)->currentDesc.Width, 
	  D3D8_SURFACE(This)->currentDesc.Height, 
	  0, 
	  D3DFmt2GLFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format), 
	  D3DFmt2GLType(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format),
	  D3D8_SURFACE(This)->resource.allocatedMemory);

    ENTER_GL();

    glTexImage2D(gl_target, 
		 gl_level,
		 D3DFmt2GLIntFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format),
		 D3D8_SURFACE(This)->currentDesc.Width,
		 D3D8_SURFACE(This)->currentDesc.Height,
		 0,
		 D3DFmt2GLFmt(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format),
		 D3DFmt2GLType(D3D8_SURFACE_GET_DEVICE(This), D3D8_SURFACE(This)->resource.format),
		 D3D8_SURFACE(This)->resource.allocatedMemory);
    checkGLcall("glTexImage2D");

    LEAVE_GL();

#if 0
    {
      static unsigned int gen = 0;
      char buffer[4096];
      ++gen;
      if ((gen % 10) == 0) {
	snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, gl_target, gl_level, gen);
	IDirect3DSurface8Impl_SaveSnapshot((LPDIRECT3DSURFACE8) This, buffer);
      }
      /*
       * debugging crash code
      if (gen == 250) {
	void** test = NULL;
	*test = 0;
      }
      */
    }
#endif
  }

  return D3D_OK;
}
