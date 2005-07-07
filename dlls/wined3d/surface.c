/*
 * IWineD3DSurface Implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DSurface IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DSurfaceImpl_QueryInterface(IWineD3DSurface *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    /* Warn ,but be nice about things */
    TRACE("(%p)->(%s,%p) \n", This,debugstr_guid(riid),ppobj);
    if (riid == NULL) {
        ERR("Probably FIXME: Calling query interface with NULL riid\n");
    }
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DSurface)) {
        IUnknown_AddRef((IUnknown*)iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DSurfaceImpl_AddRef(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %ld\n", This,ref - 1);
    return ref;
}

ULONG WINAPI IWineD3DSurfaceImpl_Release(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %ld\n", This, ref + 1);
    if (ref == 0) {
        if (This->textureName != 0) { /* release the openGL texture.. */
            ENTER_GL();
            TRACE("Deleting texture %d\n", This->textureName);
            glDeleteTextures(1, &This->textureName);
            LEAVE_GL();
        }
        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);

    }
    return ref;
}

/* ****************************************************
   IWineD3DSurface IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DSurfaceImpl_GetDevice(IWineD3DSurface *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD   WINAPI IWineD3DSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD   WINAPI IWineD3DSurfaceImpl_GetPriority(IWineD3DSurface *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void    WINAPI IWineD3DSurfaceImpl_PreLoad(IWineD3DSurface *iface) {
    /* TODO: re-write the way textures and managed,
    *  use a 'opengl context manager' to manage RenderTarget surfaces
    ** *********************************************************/
    
    /* TODO: check for locks */
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;
    TRACE("(%p)Checking to see if the container is a base textuer\n", This);
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == D3D_OK) {
        TRACE("Passing to conatiner\n");
        IWineD3DBaseTexture_PreLoad(baseTexture);
        IWineD3DBaseTexture_Release(baseTexture);
    } else{
    TRACE("(%p) : About to load surface\n", This);
    ENTER_GL();
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PushState(This->contextManager, GL_TEXTURE_2D, ENABLED, NOW /* make sure the state is applied now */);
#endif
    glEnable(GL_TEXTURE_2D); /* make sure texture support is enabled in this context */
    if (This->currentDesc.Level == 0 &&  This->textureName == 0) {
          glGenTextures(1, &This->textureName);
          checkGLcall("glGenTextures");
          TRACE("Surface %p given name %d\n", This, This->textureName);
          glBindTexture(GL_TEXTURE_2D, This->textureName);
          checkGLcall("glBindTexture");
          IWineD3DSurface_LoadTexture((IWineD3DSurface *) This, GL_TEXTURE_2D, This->currentDesc.Level);
          /* This is where we should be reducing the amount of GLMemoryUsed */
    }else {
        if (This->currentDesc.Level == 0) {
          glBindTexture(GL_TEXTURE_2D, This->textureName);
          checkGLcall("glBindTexture");
          IWineD3DSurface_LoadTexture((IWineD3DSurface *) This, GL_TEXTURE_2D, This->currentDesc.Level);
        } else  if (This->textureName != 0) { /* NOTE: the level 0 surface of a mpmapped texture must be loaded first! */
            /* assume this is a coding error not a real error for now */
            FIXME("Mipmap surface has a glTexture bound to it!\n");
        }
    }
    if (This->resource.pool == D3DPOOL_DEFAULT) {
       /* Tell opengl to try and keep this texture in video ram (well mostly) */
       GLclampf tmp;
       tmp = 0.9f;
        glPrioritizeTextures(1, &This->textureName, &tmp);
    }
    /* TODO: disable texture support, if it wastn't enabled when we entered. */
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PopState(This->contextManager, GL_TEXTURE_2D, DISABLED,DELAYED
              /* we don't care when the state is disabled(if atall) */);
#endif
    LEAVE_GL();
    }
    return;
}

D3DRESOURCETYPE WINAPI IWineD3DSurfaceImpl_GetType(IWineD3DSurface *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

HRESULT WINAPI IWineD3DSurfaceImpl_GetContainer(IWineD3DSurface* iface, REFIID riid, void** ppContainer) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    HRESULT hr;
    if (ppContainer == NULL) {
            ERR("Get container called witout a null ppContainer\n");
        return E_NOINTERFACE;
    }
    TRACE("(%p) : Relaying to queryInterface %p %p\n", This, ppContainer, *ppContainer);
    /** From MSDN:
     * If the surface is created using CreateImageSurface/CreateOffscreenPlainSurface, CreateRenderTarget,
     * or CreateDepthStencilSurface, the surface is considered stand alone. In this case,
     * GetContainer will return the Direct3D device used to create the surface.
     */
    hr = IUnknown_QueryInterface(This->container, riid, ppContainer);
    return hr;
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : copying into %p\n", This, pDesc);
    *(pDesc->Format)             = This->resource.format;
    *(pDesc->Type)               = This->resource.resourceType;
    *(pDesc->Usage)              = This->resource.usage;
    *(pDesc->Pool)               = This->resource.pool;
    *(pDesc->Size)               = This->resource.size;   /* dx8 only */
    *(pDesc->MultiSampleType)    = This->currentDesc.MultiSampleType;
    *(pDesc->MultiSampleQuality) = This->currentDesc.MultiSampleQuality;
    *(pDesc->Width)              = This->currentDesc.Width;
    *(pDesc->Height)             = This->currentDesc.Height;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_LockRect(IWineD3DSurface *iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain = NULL;
    static UINT messages = 0; /* holds flags to disable fixme messages */

    /* fixme: should we really lock as such? */
    if (This->inTexture && This->inPBuffer) {
        FIXME("Warning: Surface is in texture memory or pbuffer\n");
        This->inTexture = 0;
        This->inPBuffer = 0;
    }

    if (FALSE == This->lockable) {
        /* Note: UpdateTextures calls CopyRects which calls this routine to populate the 
              texture regions, and since the destination is an unlockable region we need
              to tolerate this                                                           */
        TRACE("Warning: trying to lock unlockable surf@%p\n", This);  
        /*return D3DERR_INVALIDCALL; */
    }

    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);

    if (swapchain != NULL ||  iface == myDevice->renderTarget || iface == myDevice->depthStencilBuffer) {
        if (swapchain != NULL && iface ==  swapchain->backBuffer) {
            TRACE("(%p, backBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
        } else if (swapchain != NULL && iface ==  swapchain->frontBuffer) {
            TRACE("(%p, frontBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
        } else if (iface == myDevice->renderTarget) {
            TRACE("(%p, renderTarget) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
        } else if (iface == myDevice->depthStencilBuffer) {
            TRACE("(%p, stencilBuffer) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
        }

        if (NULL != swapchain) {
            IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
        }
        swapchain = NULL;
    } else {
        TRACE("(%p) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
    }

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
         where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt3/5)      
          ie pitch = (width/4) * bytes per block                                  */
    if (This->resource.format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        pLockedRect->Pitch = (This->currentDesc.Width >> 2) << 3;
    else if (This->resource.format == WINED3DFMT_DXT3 || This->resource.format == WINED3DFMT_DXT5) /* DXT3/5 is 16 bytes per block */
        pLockedRect->Pitch = (This->currentDesc.Width >> 2) << 4;
    else
        pLockedRect->Pitch = This->bytesPerPixel * This->currentDesc.Width;  /* Bytes / row */

    if (NULL == pRect) {
        pLockedRect->pBits = This->resource.allocatedMemory;
        This->lockedRect.left   = 0;
        This->lockedRect.top    = 0;
        This->lockedRect.right  = This->currentDesc.Width;
        This->lockedRect.bottom = This->currentDesc.Height;
        TRACE("Locked Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", &This->lockedRect, This->lockedRect.left, This->lockedRect.top, This->lockedRect.right, This->lockedRect.bottom);
    } else {
        TRACE("Lock Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);

        if (This->resource.format == WINED3DFMT_DXT1) { /* DXT1 is half byte per pixel */
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + ((pRect->left * This->bytesPerPixel/2));
        } else {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * This->bytesPerPixel);
        }
        This->lockedRect.left   = pRect->left;
        This->lockedRect.top    = pRect->top;
        This->lockedRect.right  = pRect->right;
        This->lockedRect.bottom = pRect->bottom;
    }


    if (0 == This->resource.usage) { /* classic surface */

        /* Nothing to do ;) */

    } else if (D3DUSAGE_RENDERTARGET & This->resource.usage && !(Flags&D3DLOCK_DISCARD)) { /* render surfaces */

        GLint  prev_store;
        GLenum prev_read;
        BOOL notInContext = FALSE;
        IWineD3DSwapChainImpl *targetSwapChain = NULL;


        ENTER_GL();

            /**
             * for render->surface copy begin to begin of allocatedMemory
             * unlock can be more easy
             */

        TRACE("locking a render target\n");

        if (This->resource.allocatedMemory == NULL)
                This->resource.allocatedMemory = HeapAlloc(GetProcessHeap() ,0 ,This->resource.size);

        pLockedRect->pBits = This->resource.allocatedMemory;

        glFlush();
        vcheckGLcall("glFlush");
        glGetIntegerv(GL_READ_BUFFER, &prev_read);
        vcheckGLcall("glIntegerv");
        glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
        vcheckGLcall("glIntegerv");

 /* Here's what we have to do:
            See if the swapchain has the same context as the renderTarget or the surface is the render target.
            Otherwise, see if were sharing a context with the implicite swapchain (because were using a shared context model!)
            and use the fron back buffer as required.
            if not, we need to switch contexts and then switchback at the end.
         */
        IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);
        IWineD3DSurface_GetContainer(myDevice->renderTarget, &IID_IWineD3DSwapChain, (void **)&targetSwapChain);

        /* NOTE: In a shared context environment the renderTarget will use the same context as the implicite swapchain (we're not in a shared environment yet! */
        if ((swapchain == targetSwapChain && targetSwapChain != NULL) || iface == myDevice->renderTarget) {
                if (iface == myDevice->renderTarget || iface == swapchain->backBuffer) {
                    TRACE("locking back buffer\n");
                   glReadBuffer(GL_BACK);
                }else if (iface == swapchain->frontBuffer) {
                   TRACE("locking front\n");
                   glReadBuffer(GL_FRONT);
                }else if (iface == myDevice->depthStencilBuffer) {
                    FIXME("Stencil Buffer lock unsupported for now\n");
                } else{
                   FIXME("(%p) Shouldn't have got here!\n", This);
                   glReadBuffer(GL_BACK);
                }
        }else if (swapchain != NULL) {
            IWineD3DSwapChainImpl *implSwapChain;
            IWineD3DDevice_GetSwapChain((IWineD3DDevice *)myDevice, 0, (IWineD3DSwapChain **)&implSwapChain);
            if (swapchain->glCtx == implSwapChain->render_ctx && swapchain->drawable == implSwapChain->win) {
                    /* This will fail for the implicite swapchain, which is why there needs to be a context manager */
                    if (iface == swapchain->backBuffer) {
                        glReadBuffer(GL_BACK);
                    }else if (iface == swapchain->frontBuffer) {
                        glReadBuffer(GL_FRONT);
                    } else if (iface == myDevice->depthStencilBuffer) {
                        FIXME("Stencil Buffer lock unsupported for now\n");
                    } else{
                        FIXME("Should have got here!\n");
                        glReadBuffer(GL_BACK);
                    }
            }else{
                /* We need to switch contexts to be able to read the buffer!!! */
                FIXME("The buffer requested isn't in the current openGL context\n");
                notInContext = TRUE;
                /* TODO: check the contexts, to see if were shared with the current context */
            }
            IWineD3DSwapChain_Release((IWineD3DSwapChain *)implSwapChain);
        }
        if (swapchain != NULL)       IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
        if (targetSwapChain != NULL) IWineD3DSwapChain_Release((IWineD3DSwapChain *)targetSwapChain);


        /** the depth stencil in openGL has a format of GL_FLOAT
        * which should be good for WINED3DFMT_D16_LOCKABLE
        * and WINED3DFMT_D16
        * it is unclear what format the stencil buffer is in except.
        * 'Each index is converted to fixed point...
        * If GL_MAP_STENCIL is GL_TRUE, indices are replaced by their
        * mappings in the table GL_PIXEL_MAP_S_TO_S.
        * glReadPixels(This->lockedRect.left,
        *             This->lockedRect.bottom - j - 1,
        *             This->lockedRect.right - This->lockedRect.left,
        *             1,
        *             GL_DEPTH_COMPONENT,
        *             type,
        *             (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-This->lockedRect.top)));
            *****************************************/
        if (!notInContext) { /* Only read the buffer if it's in the current context */
            long j;
            GLenum format = D3DFmt2GLFmt(myDevice, This->resource.format);
            GLenum type   = D3DFmt2GLType(myDevice, This->resource.format);
#if 0
            /* Bizarly it's takes 120 millseconds to get an 800x600 region a line at a time, but only 10 to get the whole lot every time,
            *  This is on an ATI9600, and may be format dependant, anyhow this hack makes this demo dx9_2d_demo_game
            *  run ten times faster!
            * ************************************/
            BOOL ati_performance_hack = FALSE;
            ati_performance_hack = (This->lockedRect.bottom - This->lockedRect.top > 10) || (This->lockedRect.right - This->lockedRect.left > 10)? TRUE: FALSE;
#endif
            if ((This->lockedRect.left ==0 &&  This->lockedRect.top ==0 &&
                This->lockedRect.right == This->currentDesc.Width
                && This->lockedRect.bottom ==  This->currentDesc.Height)) {
                    glReadPixels(0, 0,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    format,
                    type,
                    (char *)pLockedRect->pBits);
            }else if (This->lockedRect.left ==0 &&  This->lockedRect.right == This->currentDesc.Width) {
                    glReadPixels(0,
                    This->lockedRect.top,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    format,
                    type,
                    (char *)pLockedRect->pBits);
            } else{

                for (j = This->lockedRect.top; j < This->lockedRect.bottom - This->lockedRect.top; ++j) {
                    glReadPixels(This->lockedRect.left, 
                                 This->lockedRect.bottom - j - 1, 
                                 This->lockedRect.right - This->lockedRect.left, 
                                 1,
                                 format, 
                                 type, 
                                 (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-This->lockedRect.top)));

                }
            }
            vcheckGLcall("glReadPixels");
            TRACE("Resetting buffer\n");
            glReadBuffer(prev_read);
            vcheckGLcall("glReadBuffer");
        }
        LEAVE_GL();

    } else if (D3DUSAGE_DEPTHSTENCIL & This->resource.usage) { /* stencil surfaces */

        if(!messages & 1){
            FIXME("TODO stencil depth surface locking surf%p usage(%lu)\n", This, This->resource.usage);
            /*

            glReadPixels(This->lockedRect.left,
            This->lockedRect.bottom - j - 1, 
            This->lockedRect.right - This->lockedRect.left,
            1,
            GL_STENCIL_INDEX or GL_DEPTH_COMPONENT

            )
            */
            messages |=1;
        }
    } else {
        FIXME("unsupported locking to surface surf@%p usage(%lu)\n", This, This->resource.usage);
    }

    if (Flags & (D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_READONLY)) {
        /* Don't dirtify */
    } else {
        IWineD3DBaseTexture *pBaseTexture;
        /**
         * Dirtify on lock
         * as seen in msdn docs
         */
        IWineD3DSurface_AddDirtyRect(iface, &This->lockedRect);

        /** Dirtify Container if needed */
        if (D3D_OK == IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&pBaseTexture) && pBaseTexture != NULL) {
            TRACE("Making container dirty\n");
            IWineD3DBaseTexture_SetDirty(pBaseTexture, TRUE);
            IWineD3DBaseTexture_Release(pBaseTexture);
        }else{
            TRACE("Surface is standalone, no need to dirty the container\n");
        }
    }

    TRACE("returning memory@%p, pitch(%d) dirtyfied(%d)\n", pLockedRect->pBits, pLockedRect->Pitch, This->Dirty);

    This->locked = TRUE;
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_UnlockRect(IWineD3DSurface *iface) {
    GLint skipBytes = 0;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    const char *buffername = "";
    IWineD3DSwapChainImpl *swapchain = NULL;

    if (FALSE == This->locked) {
        WARN("trying to Unlock an unlocked surf@%p\n", This);
        return D3DERR_INVALIDCALL;
    }

    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);

    if ((swapchain != NULL) &&  iface ==  swapchain->backBuffer) {
            buffername = "backBuffer";
    } else if ((swapchain != NULL) && iface ==  swapchain->frontBuffer) {
            buffername = "frontBuffer";
    } else if (iface == myDevice->depthStencilBuffer) {
            buffername = "depthStencilBuffer";
    } else if (iface == myDevice->renderTarget) {
            buffername = "renderTarget";
    }

    if (swapchain != NULL) {
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)swapchain);
    }

    TRACE("(%p %s) : dirtyfied(%d)\n", This, buffername, This->Dirty);

    if (FALSE == This->Dirty) {
        TRACE("(%p) : Not Dirtified so nothing to do, return now\n", This);
        goto unlock_end;
    }

    if (0 == This->resource.usage) { /* classic surface */
        /**
         * nothing to do
         * waiting to reload the surface via IDirect3DDevice8::UpdateTexture
         */
    } else if (D3DUSAGE_RENDERTARGET & This->resource.usage) { /* render surfaces */

        /****************************
        * TODO: Render targets are 'special' and
        * ?some? locking needs to be passed onto the context manager
        * so that it becomes possible to use auxilary buffers, pbuffers
        * render-to-texture, shared, cached contexts etc...
        * ****************************/
        IWineD3DSwapChainImpl *implSwapChain;
        IWineD3DDevice_GetSwapChain((IWineD3DDevice *)myDevice, 0, (IWineD3DSwapChain **)&implSwapChain);

        if (iface ==  implSwapChain->backBuffer || iface ==  implSwapChain->frontBuffer || iface == myDevice->renderTarget) {
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
            glPixelZoom(1.0, -1.0);
            vcheckGLcall("glPixelZoom");

            /* glDrawPixels transforms the raster position as though it was a vertex -
               we want to draw at screen position 0,0 - Set up ortho (rhw) mode as   
               per drawprim (and leave set - it will sort itself out due to last_was_rhw */
            if (!myDevice->last_was_rhw) {

                double X, Y, height, width, minZ, maxZ;
                myDevice->last_was_rhw = TRUE;

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
                X      = myDevice->stateBlock->viewport.X;
                Y      = myDevice->stateBlock->viewport.Y;
                height = myDevice->stateBlock->viewport.Height;
                width  = myDevice->stateBlock->viewport.Width;
                minZ   = myDevice->stateBlock->viewport.MinZ;
                maxZ   = myDevice->stateBlock->viewport.MaxZ;
                TRACE("Calling glOrtho with %f, %f, %f, %f\n", width, height, -minZ, -maxZ);
                glOrtho(X, X + width, Y + height, Y, -minZ, -maxZ);
                checkGLcall("glOrtho");

                /* Window Coord 0 is the middle of the first pixel, so translate by half
                   a pixel (See comment above glTranslate below)                         */
                glTranslatef(0.5, 0.5, 0);
                checkGLcall("glTranslatef(0.5, 0.5, 0)");
            }

            if (iface ==  implSwapChain->backBuffer || iface == myDevice->renderTarget) {
                glDrawBuffer(GL_BACK);
            } else if (iface ==  implSwapChain->frontBuffer) {
                glDrawBuffer(GL_FRONT);
            }

            vcheckGLcall("glDrawBuffer");

            /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &skipBytes);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, This->currentDesc.Width);

            /* And back buffers are not blended */
            glDisable(GL_BLEND);

            glRasterPos3i(This->lockedRect.left, This->lockedRect.top, 1);
            vcheckGLcall("glRasterPos2f");
            switch (This->resource.format) {
            case WINED3DFMT_R5G6B5:
                {
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, This->resource.allocatedMemory);
                    vcheckGLcall("glDrawPixels");
                }
                break;
            case WINED3DFMT_R8G8B8:
                {
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_RGB, GL_UNSIGNED_BYTE, This->resource.allocatedMemory);
                    vcheckGLcall("glDrawPixels");
                }
                break;
            case WINED3DFMT_X8R8G8B8: /* FIXME: there's no alpha change with D3DFMT_X8R8G8B8 but were using GL_BGRA */
            case WINED3DFMT_A8R8G8B8:
                {
                    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
                    vcheckGLcall("glPixelStorei");
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_BGRA, GL_UNSIGNED_BYTE, This->resource.allocatedMemory);
                    vcheckGLcall("glDrawPixels");
                    glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
                    vcheckGLcall("glPixelStorei");
                }
                break;
            default:
                FIXME("Unsupported Format %u in locking func\n", This->resource.format);
            }

            glPixelZoom(1.0,1.0);
            vcheckGLcall("glPixelZoom");
            glDrawBuffer(prev_draw);
            vcheckGLcall("glDrawBuffer");
            glRasterPos3iv(&prev_rasterpos[0]);
            vcheckGLcall("glRasterPos3iv");

            /* Reset to previous pack row length / blending state */
            glPixelStorei(GL_UNPACK_ROW_LENGTH, skipBytes);
            if (myDevice->stateBlock->renderState[D3DRS_ALPHABLENDENABLE]) glEnable(GL_BLEND);

            LEAVE_GL();

            /** restore clean dirty state */
            IWineD3DSurface_CleanDirtyRect(iface);

        } else {
            FIXME("unsupported unlocking to Rendering surface surf@%p usage(%lu)\n", This, This->resource.usage);
        }
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)implSwapChain);

    } else if (D3DUSAGE_DEPTHSTENCIL & This->resource.usage) { /* stencil surfaces */

        if (iface == myDevice->depthStencilBuffer) {
            FIXME("TODO stencil depth surface unlocking surf@%p usage(%lu)\n", This, This->resource.usage);
        } else {
            FIXME("unsupported unlocking to StencilDepth surface surf@%p usage(%lu)\n", This, This->resource.usage);
        }

    } else {
        FIXME("unsupported unlocking to surface surf@%p usage(%lu)\n", This, This->resource.usage);
    }

    unlock_end:
    This->locked = FALSE;
    memset(&This->lockedRect, 0, sizeof(RECT));
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetDC(IWineD3DSurface *iface, HDC *pHDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    FIXME("No support for GetDC yet for surface %p\n", This);
    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI IWineD3DSurfaceImpl_ReleaseDC(IWineD3DSurface *iface, HDC hDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    FIXME("No support for ReleaseDC yet for surface %p\n", This);
    return D3DERR_INVALIDCALL;
}

/* ******************************************************
   IWineD3DSurface Internal (No mapping to directx api) parts follow
   ****************************************************** */
HRESULT WINAPI IWineD3DSurfaceImpl_LoadTexture(IWineD3DSurface *iface, GLenum gl_target, GLenum gl_level) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if (This->inTexture)
        return D3D_OK;

    if (This->inPBuffer) {
        ENTER_GL();

        if (gl_level != 0)
            FIXME("Surface in texture is only supported for level 0\n");
        else if (This->resource.format == WINED3DFMT_P8 || This->resource.format == WINED3DFMT_A8P8 ||
                 This->resource.format == WINED3DFMT_DXT1 || This->resource.format == WINED3DFMT_DXT3 ||
                 This->resource.format == WINED3DFMT_DXT5)
            FIXME("Format %d not supported\n", This->resource.format);
        else {
            glCopyTexImage2D(gl_target,
                             0,
                             D3DFmt2GLIntFmt(This->resource.wineD3DDevice,
                                             This->resource.format),
                             0,
                             0,
                             This->currentDesc.Width,
                             This->currentDesc.Height,
                             0);
            TRACE("Updating target %d\n", gl_target);
            This->inTexture = TRUE;
        }
        LEAVE_GL();
        return D3D_OK;
    }

    if ((This->resource.format == WINED3DFMT_P8 || This->resource.format == WINED3DFMT_A8P8) &&
        !GL_SUPPORT(EXT_PALETTED_TEXTURE)) {
        /**
         * wanted a paletted texture and not really support it in HW 
         * so software emulation code begin
         */
        UINT i;
        PALETTEENTRY* pal = This->resource.wineD3DDevice->palettes[This->resource.wineD3DDevice->currentPalette];
        VOID* surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->currentDesc.Width * This->currentDesc.Height * sizeof(DWORD));
        BYTE* dst = (BYTE*) surface;
        BYTE* src = (BYTE*) This->resource.allocatedMemory;

        for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
            BYTE color = *src++;
            *dst++ = pal[color].peRed;
            *dst++ = pal[color].peGreen;
            *dst++ = pal[color].peBlue;
            if (This->resource.format == WINED3DFMT_A8P8)
                *dst++ = pal[color].peFlags;
            else
                *dst++ = 0xFF; 
        }

        ENTER_GL();

        TRACE("Calling glTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
              gl_target,
              gl_level, 
              GL_RGBA,
              This->currentDesc.Width, 
              This->currentDesc.Height, 
              0, 
              GL_RGBA,
              GL_UNSIGNED_BYTE,
              surface);
        glTexImage2D(gl_target,
                     gl_level, 
                     GL_RGBA,
                     This->currentDesc.Width,
                     This->currentDesc.Height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     surface);
        checkGLcall("glTexImage2D");
        HeapFree(GetProcessHeap(), 0, surface);

        LEAVE_GL();

        return D3D_OK;    
    }

    if (This->resource.format == WINED3DFMT_DXT1 ||
        This->resource.format == WINED3DFMT_DXT3 ||
        This->resource.format == WINED3DFMT_DXT5) {
        if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
            TRACE("Calling glCompressedTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, sz=%d, Mem=%p\n",
                  gl_target, 
                  gl_level, 
                  D3DFmt2GLIntFmt(This->resource.wineD3DDevice, This->resource.format), 
                  This->currentDesc.Width, 
                  This->currentDesc.Height, 
                  0, 
                  This->resource.size,
                  This->resource.allocatedMemory);

            ENTER_GL();

            GL_EXTCALL(glCompressedTexImage2DARB)(gl_target, 
                                                  gl_level, 
                                                  D3DFmt2GLIntFmt(This->resource.wineD3DDevice, This->resource.format),
                                                  This->currentDesc.Width,
                                                  This->currentDesc.Height,
                                                  0,
                                                  This->resource.size,
                                                  This->resource.allocatedMemory);
            checkGLcall("glCommpressedTexTexImage2D");

            LEAVE_GL();
        } else {
            FIXME("Using DXT1/3/5 without advertized support\n");
        }
    } else {

        TRACE("Calling glTexImage2D %x i=%d, d3dfmt=%s, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
              gl_target, 
              gl_level, 
              debug_d3dformat(This->resource.format),
              D3DFmt2GLIntFmt(This->resource.wineD3DDevice, This->resource.format), 
              This->currentDesc.Width, 
              This->currentDesc.Height, 
              0, 
              D3DFmt2GLFmt(This->resource.wineD3DDevice, This->resource.format), 
              D3DFmt2GLType(This->resource.wineD3DDevice, This->resource.format),
              This->resource.allocatedMemory);

        ENTER_GL();

        glTexImage2D(gl_target, 
                     gl_level,
                     D3DFmt2GLIntFmt(This->resource.wineD3DDevice, This->resource.format),
                     This->currentDesc.Width,
                     This->currentDesc.Height,
                     0,
                     D3DFmt2GLFmt(This->resource.wineD3DDevice, This->resource.format),
                     D3DFmt2GLType(This->resource.wineD3DDevice, This->resource.format),
                     This->resource.allocatedMemory);
        checkGLcall("glTexImage2D");

        LEAVE_GL();

#if 0
        {
            static unsigned int gen = 0;
            char buffer[4096];
            ++gen;
            if ((gen % 10) == 0) {
                snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, gl_target, gl_level, gen);
                IWineD3DSurfaceImpl_SaveSnapshot((IWineD3DSurface *) This, buffer);
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

#include <errno.h>
#include <stdio.h>
HRESULT WINAPI IWineD3DSurfaceImpl_SaveSnapshot(IWineD3DSurface *iface, const char* filename) {
    FILE* f = NULL;
    ULONG i;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    f = fopen(filename, "w+");
    if (NULL == f) {
        ERR("opening of %s failed with: %s\n", filename, strerror(errno));
        return D3DERR_INVALIDCALL;
    }

    TRACE("opened %s with format %s\n", filename, debug_d3dformat(This->resource.format));

    fprintf(f, "P6\n%u %u\n255\n", This->currentDesc.Width, This->currentDesc.Height);
    switch (This->resource.format) {
    case WINED3DFMT_X8R8G8B8:
    case WINED3DFMT_A8R8G8B8:
        {
            DWORD color;
            for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
                color = ((DWORD*) This->resource.allocatedMemory)[i];
                fputc((color >> 16) & 0xFF, f);
                fputc((color >>  8) & 0xFF, f);
                fputc((color >>  0) & 0xFF, f);
            }
        }
        break;
    case WINED3DFMT_R8G8B8:
        {
            BYTE* color;
            for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
                color = ((BYTE*) This->resource.allocatedMemory) + (3 * i);
                fputc((color[0]) & 0xFF, f);
                fputc((color[1]) & 0xFF, f);
                fputc((color[2]) & 0xFF, f);
            }
        }
        break;
    case WINED3DFMT_A1R5G5B5:
        {
            WORD color;
            for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
                color = ((WORD*) This->resource.allocatedMemory)[i];
                fputc(((color >> 10) & 0x1F) * 255 / 31, f);
                fputc(((color >>  5) & 0x1F) * 255 / 31, f);
                fputc(((color >>  0) & 0x1F) * 255 / 31, f);
            }
        }
        break;
    case WINED3DFMT_A4R4G4B4:
        {
            WORD color;
            for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
                color = ((WORD*) This->resource.allocatedMemory)[i];
                fputc(((color >>  8) & 0x0F) * 255 / 15, f);
                fputc(((color >>  4) & 0x0F) * 255 / 15, f);
                fputc(((color >>  0) & 0x0F) * 255 / 15, f);
            }
        }
        break;

    case WINED3DFMT_R5G6B5:
        {
            WORD color;
            for (i = 0; i < This->currentDesc.Width * This->currentDesc.Height; i++) {
                color = ((WORD*) This->resource.allocatedMemory)[i];
                fputc(((color >> 11) & 0x1F) * 255 / 31, f);
                fputc(((color >>  5) & 0x3F) * 255 / 63, f);
                fputc(((color >>  0) & 0x1F) * 255 / 31, f);
            }
        }
        break;
    default: 
        FIXME("Unimplemented dump mode format(%u,%s)\n", This->resource.format, debug_d3dformat(This->resource.format));
    }
    fclose(f);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_CleanDirtyRect(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    This->Dirty = FALSE;
    This->dirtyRect.left   = This->currentDesc.Width;
    This->dirtyRect.top    = This->currentDesc.Height;
    This->dirtyRect.right  = 0;
    This->dirtyRect.bottom = 0;
    TRACE("(%p) : Dirty?%d, Rect:(%ld,%ld,%ld,%ld)\n", This, This->Dirty, This->dirtyRect.left, 
          This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);
    return D3D_OK;
}

/**
 *   Slightly inefficient way to handle multiple dirty rects but it works :)
 */
extern HRESULT WINAPI IWineD3DSurfaceImpl_AddDirtyRect(IWineD3DSurface *iface, CONST RECT* pDirtyRect) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    This->Dirty = TRUE;
    if (NULL != pDirtyRect) {
        This->dirtyRect.left   = min(This->dirtyRect.left,   pDirtyRect->left);
        This->dirtyRect.top    = min(This->dirtyRect.top,    pDirtyRect->top);
        This->dirtyRect.right  = max(This->dirtyRect.right,  pDirtyRect->right);
        This->dirtyRect.bottom = max(This->dirtyRect.bottom, pDirtyRect->bottom);
    } else {
        This->dirtyRect.left   = 0;
        This->dirtyRect.top    = 0;
        This->dirtyRect.right  = This->currentDesc.Width;
        This->dirtyRect.bottom = This->currentDesc.Height;
    }
    TRACE("(%p) : Dirty?%d, Rect:(%ld,%ld,%ld,%ld)\n", This, This->Dirty, This->dirtyRect.left, 
          This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetContainer(IWineD3DSurface *iface, IUnknown *container) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;
    return D3D_OK;    
}

/* TODO: replace this function with context management routines */
HRESULT WINAPI IWineD3DSurfaceImpl_SetPBufferState(IWineD3DSurface *iface, BOOL inPBuffer, BOOL  inTexture) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    
    This->inPBuffer = inPBuffer;
    This->inTexture = inTexture;
    return D3D_OK;
}

const IWineD3DSurfaceVtbl IWineD3DSurface_Vtbl =
{
    /* IUnknown */
    IWineD3DSurfaceImpl_QueryInterface,
    IWineD3DSurfaceImpl_AddRef,
    IWineD3DSurfaceImpl_Release,
    /* IWineD3DResource */
    IWineD3DSurfaceImpl_GetParent,
    IWineD3DSurfaceImpl_GetDevice,
    IWineD3DSurfaceImpl_SetPrivateData,
    IWineD3DSurfaceImpl_GetPrivateData,
    IWineD3DSurfaceImpl_FreePrivateData,
    IWineD3DSurfaceImpl_SetPriority,
    IWineD3DSurfaceImpl_GetPriority,
    IWineD3DSurfaceImpl_PreLoad,
    IWineD3DSurfaceImpl_GetType,
    /* IWineD3DSurface */    
    IWineD3DSurfaceImpl_GetContainer,
    IWineD3DSurfaceImpl_GetDesc,
    IWineD3DSurfaceImpl_LockRect,
    IWineD3DSurfaceImpl_UnlockRect,
    IWineD3DSurfaceImpl_GetDC,
    IWineD3DSurfaceImpl_ReleaseDC,
    /* Internal use: */
    IWineD3DSurfaceImpl_CleanDirtyRect,
    IWineD3DSurfaceImpl_AddDirtyRect,
    IWineD3DSurfaceImpl_LoadTexture,
    IWineD3DSurfaceImpl_SaveSnapshot,
    IWineD3DSurfaceImpl_SetContainer,
    IWineD3DSurfaceImpl_SetPBufferState    
};
