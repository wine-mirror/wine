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

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* trace: */
#if 1
# define VTRACE(A) FIXME A
#else 
# define VTRACE(A) 
#endif


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
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    FIXME("(%p) : stub\n", This);    return D3D_OK;
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
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    /* fixme: should we really lock as such? */

    if (FALSE == This->lockable) {
      ERR("trying to lock unlockable surf@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }

    TRACE("(%p) : rect=%p, output prect=%p, allMem=%p\n", This, pRect, pLockedRect, This->allocatedMemory);

    pLockedRect->Pitch = This->bytesPerPixel * This->myDesc.Width;  /* Bytes / row */    
    
    if (!pRect) {
      pLockedRect->pBits = This->allocatedMemory;
      This->lockedRect.left = 0;
      This->lockedRect.top = 0;
      This->lockedRect.right = This->myDesc.Width;
      This->lockedRect.bottom = This->myDesc.Height;
    } else {
      TRACE("Lock Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);
      pLockedRect->pBits = This->allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * This->bytesPerPixel);
      This->lockedRect.left = pRect->left;
      This->lockedRect.top = pRect->top;
      This->lockedRect.right = pRect->right;
      This->lockedRect.bottom = pRect->bottom;
    }



    if (0 == This->myDesc.Usage) { /* classic surface */

      /* Nothing to do ;) */

    } else if (D3DUSAGE_RENDERTARGET & This->myDesc.Usage) { /* render surfaces */
      
      if (This == This->Device->backBuffer || This == This->Device->frontBuffer) {
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
	} else if (This == This->Device->frontBuffer) {
	  glReadBuffer(GL_FRONT);
	}
	vcheckGLcall("glReadBuffer");

	switch (This->myDesc.Format) { 
	case D3DFMT_R5G6B5:
	  {
	    glReadPixels(This->lockedRect.left, This->lockedRect.top, 
			 This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, pLockedRect->pBits);
	    vcheckGLcall("glReadPixels");
	  }
	  break;
	case D3DFMT_R8G8B8:
	  {
	    glReadPixels(This->lockedRect.left, This->lockedRect.top, 
			 This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_RGB, GL_UNSIGNED_BYTE, pLockedRect->pBits);
	    vcheckGLcall("glReadPixels");
	  }
	  break;
	case D3DFMT_A8R8G8B8:
	  {
	    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
	    vcheckGLcall("glPixelStorei");
	    glReadPixels(This->lockedRect.left, This->lockedRect.top, 
			 This->lockedRect.right - This->lockedRect.left, This->lockedRect.bottom - This->lockedRect.top,
			 GL_BGRA, GL_UNSIGNED_BYTE, pLockedRect->pBits);
	    vcheckGLcall("glReadPixels");
	    glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
	    vcheckGLcall("glPixelStorei");
	  }
	  break;
	default:
	  FIXME("Unsupported Format %u in locking func\n", This->myDesc.Format);
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

    TRACE("returning pBits=%p, pitch=%d\n", pLockedRect->pBits, pLockedRect->Pitch);

    This->locked = TRUE;
    return D3D_OK;
}
HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    HRESULT hr;
    ICOM_THIS(IDirect3DSurface8Impl,iface);
    
    if (FALSE == This->locked) {
      ERR("trying to lock unlocked surf@%p\n", This);  
      return D3DERR_INVALIDCALL;
    }

    TRACE("(%p) : stub\n", This);

    if (0 == This->myDesc.Usage) { /* classic surface */
      if (This->Container) {
	IDirect3DBaseTexture8* cont = NULL;
	hr = IUnknown_QueryInterface(This->Container, &IID_IDirect3DBaseTexture8, (void**) &cont);
	
	if (SUCCEEDED(hr) && NULL != cont) {
	  /* Now setup the texture appropraitly */
	  int containerType = IDirect3DBaseTexture8Impl_GetType(cont);
	  if (containerType == D3DRTYPE_TEXTURE) {
            IDirect3DTexture8Impl *pTexture = (IDirect3DTexture8Impl *)cont;
            pTexture->Dirty = TRUE;
	  } else if (containerType == D3DRTYPE_CUBETEXTURE) {
            IDirect3DCubeTexture8Impl *pTexture = (IDirect3DCubeTexture8Impl *)cont;
            pTexture->Dirty = TRUE;
	  } else {
            FIXME("Set dirty on container type %d\n", containerType);
	  }
	  IDirect3DBaseTexture8_Release(cont);
	  cont = NULL;
	}
      }
    } else if (D3DUSAGE_RENDERTARGET & This->myDesc.Usage) { /* render surfaces */

      if (This == This->Device->backBuffer || This == This->Device->frontBuffer) {
	GLint  prev_store;
	GLenum prev_draw;
	
	ENTER_GL();
	
	glFlush();
	vcheckGLcall("glFlush");
	glGetIntegerv(GL_DRAW_BUFFER, &prev_draw);
	vcheckGLcall("glIntegerv");
	glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
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
	LEAVE_GL();
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
    This->locked = FALSE;
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
