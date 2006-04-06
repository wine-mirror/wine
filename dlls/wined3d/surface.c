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
#include "wine/port.h"
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
    TRACE("(%p)->(%s,%p)\n", This,debugstr_guid(riid),ppobj);
    if (riid == NULL) {
        ERR("Probably FIXME: Calling query interface with NULL riid\n");
    }
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
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
        TRACE("(%p) : cleaning up\n", This);
        if (This->glDescription.textureName != 0) { /* release the openGL texture.. */
            ENTER_GL();
            TRACE("Deleting texture %d\n", This->glDescription.textureName);
            glDeleteTextures(1, &This->glDescription.textureName);
            LEAVE_GL();
        }
        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);

        TRACE("(%p) Released\n", This);
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
    TRACE("(%p)Checking to see if the container is a base texture\n", This);
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == D3D_OK) {
        TRACE("Passing to conatiner\n");
        IWineD3DBaseTexture_PreLoad(baseTexture);
        IWineD3DBaseTexture_Release(baseTexture);
    } else {
    TRACE("(%p) : About to load surface\n", This);
    ENTER_GL();
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PushState(This->contextManager, GL_TEXTURE_2D, ENABLED, NOW /* make sure the state is applied now */);
#endif
    glEnable(This->glDescription.target);/* make sure texture support is enabled in this context */
    if (This->glDescription.level == 0 &&  This->glDescription.textureName == 0) {
          glGenTextures(1, &This->glDescription.textureName);
          checkGLcall("glGenTextures");
          TRACE("Surface %p given name %d\n", This, This->glDescription.textureName);
          glBindTexture(This->glDescription.target, This->glDescription.textureName);
          checkGLcall("glBindTexture");
          IWineD3DSurface_LoadTexture(iface);
          /* This is where we should be reducing the amount of GLMemoryUsed */
    } else {
        if (This->glDescription.level == 0) {
          glBindTexture(This->glDescription.target, This->glDescription.textureName);
          checkGLcall("glBindTexture");
          IWineD3DSurface_LoadTexture(iface);
        } else  if (This->glDescription.textureName != 0) { /* NOTE: the level 0 surface of a mpmapped texture must be loaded first! */
            /* assume this is a coding error not a real error for now */
            FIXME("Mipmap surface has a glTexture bound to it!\n");
        }
    }
    if (This->resource.pool == WINED3DPOOL_DEFAULT) {
       /* Tell opengl to try and keep this texture in video ram (well mostly) */
       GLclampf tmp;
       tmp = 0.9f;
        glPrioritizeTextures(1, &This->glDescription.textureName, &tmp);
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

WINED3DRESOURCETYPE WINAPI IWineD3DSurfaceImpl_GetType(IWineD3DSurface *iface) {
    TRACE("(%p) : calling resourceimpl_GetType\n", iface);
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) {
    TRACE("(%p) : calling resourceimpl_GetParent\n", iface);
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

HRESULT WINAPI IWineD3DSurfaceImpl_GetContainerParent(IWineD3DSurface* iface, IUnknown **ppContainerParent) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : ppContainerParent %p)\n", This, ppContainerParent);

    if (!ppContainerParent) {
        ERR("(%p) : Called without a valid ppContainerParent.\n", This);
    }

    if (This->container) {
        IWineD3DBase_GetParent(This->container, ppContainerParent);
        if (!ppContainerParent) {
            /* WineD3D objects should always have a parent */
            ERR("(%p) : GetParent returned NULL\n", This);
        }
        IUnknown_Release(*ppContainerParent); /* GetParent adds a reference; we want just the pointer */
    } else {
        *ppContainerParent = NULL;
    }

    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetContainer(IWineD3DSurface* iface, REFIID riid, void** ppContainer) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBase *container = 0;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    /** From MSDN:
     * If the surface is created using CreateImageSurface/CreateOffscreenPlainSurface, CreateRenderTarget,
     * or CreateDepthStencilSurface, the surface is considered stand alone. In this case,
     * GetContainer will return the Direct3D device used to create the surface.
     */
    if (This->container) {
        container = This->container;
    } else {
        container = (IWineD3DBase *)This->resource.wineD3DDevice;
    }

    TRACE("Relaying to QueryInterface\n");
    return IUnknown_QueryInterface(container, riid, ppContainer);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p) : copying into %p\n", This, pDesc);
    if(pDesc->Format != NULL)             *(pDesc->Format) = This->resource.format;
    if(pDesc->Type != NULL)               *(pDesc->Type)   = This->resource.resourceType;
    if(pDesc->Usage != NULL)              *(pDesc->Usage)              = This->resource.usage;
    if(pDesc->Pool != NULL)               *(pDesc->Pool)               = This->resource.pool;
    if(pDesc->Size != NULL)               *(pDesc->Size)               = This->resource.size;   /* dx8 only */
    if(pDesc->MultiSampleType != NULL)    *(pDesc->MultiSampleType)    = This->currentDesc.MultiSampleType;
    if(pDesc->MultiSampleQuality != NULL) *(pDesc->MultiSampleQuality) = This->currentDesc.MultiSampleQuality;
    if(pDesc->Width != NULL)              *(pDesc->Width)              = This->currentDesc.Width;
    if(pDesc->Height != NULL)             *(pDesc->Height)             = This->currentDesc.Height;
    return D3D_OK;
}

void WINAPI IWineD3DSurfaceImpl_SetGlTextureDesc(IWineD3DSurface *iface, UINT textureName, int target) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : setting textureName %u, target %i\n", This, textureName, target);
    if (This->glDescription.textureName == 0 && textureName != 0) {
        This->Dirty = TRUE;
        IWineD3DSurface_AddDirtyRect(iface, NULL);
    }
    This->glDescription.textureName = textureName;
    This->glDescription.target      = target;
}

void WINAPI IWineD3DSurfaceImpl_GetGlDesc(IWineD3DSurface *iface, glDescriptor **glDescription) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, &This->glDescription);
    *glDescription = &This->glDescription;
}

/* TODO: think about moving this down to resource? */
const void *WINAPI IWineD3DSurfaceImpl_GetData(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    /* This should only be called for sysmem textures, it may be a good idea to extend this to all pools at some point in the futture  */
    if (This->resource.pool != WINED3DPOOL_SYSTEMMEM) {
        FIXME(" (%p)Attempting to get system memory for a non-system memory texture\n", iface);
    }
    return (CONST void*)(This->resource.allocatedMemory);
}

HRESULT WINAPI IWineD3DSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
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

    if (This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
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
        }
    } else {
        TRACE("(%p) : rect@%p flags(%08lx), output lockedRect@%p, memory@%p\n", This, pRect, Flags, pLockedRect, This->resource.allocatedMemory);
    }

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
         where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)
          ie pitch = (width/4) * bytes per block                                  */
    if (This->resource.format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        pLockedRect->Pitch = (This->currentDesc.Width >> 2) << 3;
    else if (This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
             This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        pLockedRect->Pitch = (This->currentDesc.Width >> 2) << 4;
    else {
        if (NP2_REPACK == wined3d_settings.nonpower2_mode || This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            /* Front and back buffers are always lockes/unlocked on currentDesc.Width */
            pLockedRect->Pitch = This->bytesPerPixel * This->currentDesc.Width;  /* Bytes / row */
        } else {
            pLockedRect->Pitch = This->bytesPerPixel * This->pow2Width;
        }
    }

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
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + ((pRect->left * This->bytesPerPixel / 2));
        } else {
            pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * This->bytesPerPixel);
        }
        This->lockedRect.left   = pRect->left;
        This->lockedRect.top    = pRect->top;
        This->lockedRect.right  = pRect->right;
        This->lockedRect.bottom = pRect->bottom;
    }

    if (This->nonpow2) {
        TRACE("Locking non-power 2 texture\n");
    }

    if (0 == This->resource.usage || This->resource.usage & WINED3DUSAGE_DYNAMIC) {
        /* classic surface  TODO: non 2d surfaces?
        These resources may be POOL_SYSTEMMEM, so they must not access the device */
        TRACE("locking an ordinarary surface\n");
        /* Check to see if memory has already been allocated from the surface*/
        if (NULL == This->resource.allocatedMemory) { /* TODO: check to see if an update has been performed on the surface (an update could just clobber allocatedMemory */
            /* Non-system memory surfaces */

            /*Surface has no memory currently allocated to it!*/
            TRACE("(%p) Locking rect\n" , This);
            This->resource.allocatedMemory = HeapAlloc(GetProcessHeap() ,0 , This->pow2Size);
            if (0 != This->glDescription.textureName) {
                /* Now I have to copy thing bits back */
                This->activeLock = TRUE; /* When this flag is set to true, loading the surface again won't free THis->resource.allocatedMemory */
                /* TODO: make activeLock a bit more intelligent, maybe implement a method to purge the texture memory. */
                ENTER_GL();
    
                /* Make sure that the texture is loaded */
                IWineD3DSurface_PreLoad(iface); /* Make sure there is a texture to bind! */
    
                TRACE("(%p) glGetTexImage level(%d), fmt(%d), typ(%d), mem(%p)\n" , This, This->glDescription.level,  This->glDescription.glFormat, This->glDescription.glType, This->resource.allocatedMemory);
    
                if (This->resource.format == WINED3DFMT_DXT1 ||
                    This->resource.format == WINED3DFMT_DXT2 ||
                    This->resource.format == WINED3DFMT_DXT3 ||
                    This->resource.format == WINED3DFMT_DXT4 ||
                    This->resource.format == WINED3DFMT_DXT5) {
                    TRACE("Locking a compressed texture\n");
                    if (GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) { /* we can assume this as the texture would not have been created otherwise */
                        GL_EXTCALL(glGetCompressedTexImageARB)(This->glDescription.target,
                                                            This->glDescription.level,
                                                            This->resource.allocatedMemory);
    
                    } else {
                        FIXME("(%p) attempting to lock a compressed texture when texture compression isn't supported by opengl\n", This);
                    }
                } else {
                    glGetTexImage(This->glDescription.target,
                                This->glDescription.level,
                                This->glDescription.glFormat,
                                This->glDescription.glType,
                                This->resource.allocatedMemory);
                    vcheckGLcall("glGetTexImage");
                    if (NP2_REPACK == wined3d_settings.nonpower2_mode) {
                        /* some games (e.g. warhammer 40k) don't work with the odd pitchs properly, preventing
                        the surface pitch from being used to box non-power2 textures. Instead we have to use a hack to
                        repack the texture so that the bpp * width pitch can be used instead of the bpp * pow2width.
        
                        Were doing this...
        
                        instead of boxing the texture :
                        |<-texture width ->|  -->pow2width|   /\
                        |111111111111111111|              |   |
                        |222 Texture 222222| boxed empty  | texture height
                        |3333 Data 33333333|              |   |
                        |444444444444444444|              |   \/
                        -----------------------------------   |
                        |     boxed  empty | boxed empty  | pow2height
                        |                  |              |   \/
                        -----------------------------------
        
        
                        were repacking the data to the expected texture width
        
                        |<-texture width ->|  -->pow2width|   /\
                        |111111111111111111222222222222222|   |
                        |222333333333333333333444444444444| texture height
                        |444444                           |   |
                        |                                 |   \/
                        |                                 |   |
                        |            empty                | pow2height
                        |                                 |   \/
                        -----------------------------------
        
                        == is the same as
        
                        |<-texture width ->|    /\
                        |111111111111111111|
                        |222222222222222222|texture height
                        |333333333333333333|
                        |444444444444444444|    \/
                        --------------------
        
                        this also means that any references to allocatedMemory should work with the data as if were a standard texture with a non-power2 width instead of texture boxed up to be a power2 texture.
        
                        internally the texture is still stored in a boxed format so any references to textureName will get a boxed texture with width pow2width and not a texture of width currentDesc.Width.
                        */
                        if (This->nonpow2) {
                            BYTE* dataa, *datab;
                            int pitcha = 0, pitchb = 0;
                            int y;
                            pitcha = This->bytesPerPixel * This->currentDesc.Width;
                            pitchb = This->bytesPerPixel * This->pow2Width;
                            datab = dataa = This->resource.allocatedMemory;
                            FIXME("(%p) : Repacking the surface data from pitch %d to pitch %d\n", This, pitcha, pitchb);
                            for (y = 1 ; y < This->currentDesc.Height; y++) {
                                dataa += pitcha; /* skip the first row */
                                datab += pitchb;
                                memcpy(dataa, datab, pitcha);
                            }
                        }
                    }
                }
                LEAVE_GL();
            }
        } else { /* Nothing to do */
            TRACE("Memory %p already allocted for texture\n",  This->resource.allocatedMemory);
        }

        if (NULL == pRect) {
            pLockedRect->pBits = This->resource.allocatedMemory;
        }  else{
            if (This->resource.format == D3DFMT_DXT1) { /* DXT1 is half byte per pixel */
                pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + ((pRect->left * This->bytesPerPixel / 2));
            } else {
                pLockedRect->pBits = This->resource.allocatedMemory + (pLockedRect->Pitch * pRect->top) + (pRect->left * This->bytesPerPixel);
            }
        }

    } else if (WINED3DUSAGE_RENDERTARGET & This->resource.usage && !(Flags&WINED3DLOCK_DISCARD)) { /* render surfaces */

        GLint  prev_store;
        GLint  prev_read;
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

        This->activeLock = TRUE; /*When this flag is set to true, loading the surface again won't free THis->resource.allocatedMemory*/
        pLockedRect->pBits = This->resource.allocatedMemory;

        glFlush();
        vcheckGLcall("glFlush");
        glGetIntegerv(GL_READ_BUFFER, &prev_read);
        vcheckGLcall("glIntegerv");
        glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
        vcheckGLcall("glIntegerv");

 /* Here's what we have to do:
            See if the swapchain has the same context as the renderTarget or the surface is the render target.
            Otherwise, see if were sharing a context with the implicit swapchain (because we're using a shared context model!)
            and use the front back buffer as required.
            if not, we need to switch contexts and then switchback at the end.
         */
        IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);
        IWineD3DSurface_GetContainer(myDevice->renderTarget, &IID_IWineD3DSwapChain, (void **)&targetSwapChain);

        /* NOTE: In a shared context environment the renderTarget will use the same context as the implicit swapchain (we're not in a shared environment yet! */
        if ((swapchain == targetSwapChain && targetSwapChain != NULL) || iface == myDevice->renderTarget) {
                if (iface == myDevice->renderTarget || iface == swapchain->backBuffer) {
                    TRACE("locking back buffer\n");
                   glReadBuffer(GL_BACK);
                } else if (iface == swapchain->frontBuffer) {
                   TRACE("locking front\n");
                   glReadBuffer(GL_FRONT);
                } else if (iface == myDevice->depthStencilBuffer) {
                    FIXME("Stencil Buffer lock unsupported for now\n");
                } else {
                   FIXME("(%p) Shouldn't have got here!\n", This);
                   glReadBuffer(GL_BACK);
                }
        } else if (swapchain != NULL) {
            IWineD3DSwapChainImpl *implSwapChain;
            IWineD3DDevice_GetSwapChain((IWineD3DDevice *)myDevice, 0, (IWineD3DSwapChain **)&implSwapChain);
            if (swapchain->glCtx == implSwapChain->render_ctx && swapchain->drawable == implSwapChain->win) {
                    /* This will fail for the implicit swapchain, which is why there needs to be a context manager */
                    if (iface == swapchain->backBuffer) {
                        glReadBuffer(GL_BACK);
                    } else if (iface == swapchain->frontBuffer) {
                        glReadBuffer(GL_FRONT);
                    } else if (iface == myDevice->depthStencilBuffer) {
                        FIXME("Stencil Buffer lock unsupported for now\n");
                    } else {
                        FIXME("Should have got here!\n");
                        glReadBuffer(GL_BACK);
                    }
            } else {
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
#if 0
            /* Bizarly it takes 120 millseconds to get an 800x600 region a line at a time, but only 10 to get the whole lot every time,
            *  This is on an ATI9600, and may be format dependent, anyhow this hack makes this demo dx9_2d_demo_game
            *  run ten times faster!
            * ************************************/
            BOOL ati_performance_hack = FALSE;
            ati_performance_hack = (This->lockedRect.bottom - This->lockedRect.top > 10) || (This->lockedRect.right - This->lockedRect.left > 10)? TRUE: FALSE;
#endif
            if ((This->lockedRect.left == 0 &&  This->lockedRect.top == 0 &&
                This->lockedRect.right == This->currentDesc.Width
                && This->lockedRect.bottom ==  This->currentDesc.Height)) {
                    glReadPixels(0, 0,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    This->glDescription.glFormat,
                    This->glDescription.glType,
                    (char *)pLockedRect->pBits);
            } else if (This->lockedRect.left == 0 &&  This->lockedRect.right == This->currentDesc.Width) {
                    glReadPixels(0,
                    This->lockedRect.top,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    This->glDescription.glFormat,
                    This->glDescription.glType,
                    (char *)pLockedRect->pBits);
            } else{

                for (j = This->lockedRect.top; j < This->lockedRect.bottom - This->lockedRect.top; ++j) {
                    glReadPixels(This->lockedRect.left, 
                                 This->lockedRect.bottom - j - 1, 
                                 This->lockedRect.right - This->lockedRect.left, 
                                 1,
                                 This->glDescription.glFormat,
                                 This->glDescription.glType,
                                 (char *)pLockedRect->pBits + (pLockedRect->Pitch * (j-This->lockedRect.top)));

                }
            }
            vcheckGLcall("glReadPixels");
            TRACE("Resetting buffer\n");
            glReadBuffer(prev_read);
            vcheckGLcall("glReadBuffer");
        }
        LEAVE_GL();

    } else if (WINED3DUSAGE_DEPTHSTENCIL & This->resource.usage) { /* stencil surfaces */

        if (!messages & 1) {
            FIXME("TODO stencil depth surface locking surf%p usage(%lu)\n", This, This->resource.usage);
            /*

            glReadPixels(This->lockedRect.left,
            This->lockedRect.bottom - j - 1,
            This->lockedRect.right - This->lockedRect.left,
            1,
            GL_STENCIL_INDEX or GL_DEPTH_COMPONENT

            )
            */
            messages |= 1;
        }
    } else {
        FIXME("unsupported locking to surface surf@%p usage(%lu)\n", This, This->resource.usage);
    }

    if (Flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)) {
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
        } else {
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

    if (WINED3DUSAGE_RENDERTARGET & This->resource.usage) {
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
    } else if (WINED3DUSAGE_RENDERTARGET & This->resource.usage) { /* render surfaces */

        /****************************
        * TODO: Render targets are 'special' and
        * ?some? locking needs to be passed onto the context manager
        * so that it becomes possible to use auxiliary buffers, pbuffers
        * render-to-texture, shared, cached contexts etc...
        * ****************************/
        IWineD3DSwapChainImpl *implSwapChain;
        IWineD3DDevice_GetSwapChain((IWineD3DDevice *)myDevice, 0, (IWineD3DSwapChain **)&implSwapChain);

        if (iface ==  implSwapChain->backBuffer || iface ==  implSwapChain->frontBuffer || iface == myDevice->renderTarget) {
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
            if ( (!myDevice->last_was_rhw) || (myDevice->viewport_changed) ) {

                double X, Y, height, width, minZ, maxZ;
                myDevice->last_was_rhw = TRUE;
                myDevice->viewport_changed = FALSE;

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
	    case WINED3DFMT_X4R4G4B4:
	        {
		    int size;
                    unsigned short *data;
                    data = (unsigned short *)This->resource.allocatedMemory;
                    size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
                    while(size > 0) {
                            *data |= 0xF000;
                            data++;
                            size--;
                    }
		}
	    case WINED3DFMT_A4R4G4B4:
	        {
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, This->resource.allocatedMemory);
                    vcheckGLcall("glDrawPixels");
	        }
		break;
            case WINED3DFMT_R5G6B5:
                {
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, This->resource.allocatedMemory);
                    vcheckGLcall("glDrawPixels");
                }
                break;
            case WINED3DFMT_X1R5G5B5:
                {
                    int size;
                    unsigned short *data;
                    data = (unsigned short *)This->resource.allocatedMemory;
                    size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
                    while(size > 0) {
                            *data |= 0x8000;
                            data++;
                            size--;
                    }
                }
            case WINED3DFMT_A1R5G5B5:
                {
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, This->resource.allocatedMemory);
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
            case WINED3DFMT_X8R8G8B8: /* make sure the X byte is set to alpha on, since it 
                                        could be any random value this fixes the intro move in Pirates! */
                {
                    int size;
                    unsigned int *data;
                    data = (unsigned int *)This->resource.allocatedMemory;
                    size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
                    while(size > 0) {
                            *data |= 0xFF000000;
                            data++;
                            size--;
                    }
                }
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
            case WINED3DFMT_A2R10G10B10:
                {
                    glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
                    vcheckGLcall("glPixelStorei");
                    glDrawPixels(This->lockedRect.right - This->lockedRect.left, (This->lockedRect.bottom - This->lockedRect.top)-1,
                                 GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, This->resource.allocatedMemory);
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

    } else if (WINED3DUSAGE_DEPTHSTENCIL & This->resource.usage) { /* stencil surfaces */

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
HRESULT WINAPI IWineD3DSurfaceImpl_LoadTexture(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if (This->inTexture) {
        TRACE("Surface already in texture\n");
        return D3D_OK;
    }
    if (This->Dirty == FALSE) {
        TRACE("surface isn't dirty\n");
        return D3D_OK;
    }

    This->Dirty = FALSE;

    /* Resources are placed in system RAM and do not need to be recreated when a device is lost.
    *  These resources are not bound by device size or format restrictions. Because of this,
    *  these resources cannot be accessed by the Direct3D device nor set as textures or render targets.
    *  However, these resources can always be created, locked, and copied.
    *  In general never store scratch or system mem textures in the video ram. However it is allowed
    *  for system memory textures when WINED3DDEVCAPS_TEXTURESYSTEMMEMORY is set but it isn't right now.
    */
    if (This->resource.pool == WINED3DPOOL_SCRATCH || This->resource.pool == WINED3DPOOL_SYSTEMMEM)
    {
        FIXME("(%p) Operation not supported for scratch or SYSTEMMEM textures\n",This);
        return D3DERR_INVALIDCALL;
    }

    if (This->inPBuffer) {
        ENTER_GL();

        if (This->glDescription.level != 0)
            FIXME("Surface in texture is only supported for level 0\n");
        else if (This->resource.format == WINED3DFMT_P8 || This->resource.format == WINED3DFMT_A8P8 ||
                 This->resource.format == WINED3DFMT_DXT1 || This->resource.format == WINED3DFMT_DXT2 ||
                 This->resource.format == WINED3DFMT_DXT3 || This->resource.format == WINED3DFMT_DXT4 ||
                 This->resource.format == WINED3DFMT_DXT5)
            FIXME("Format %d not supported\n", This->resource.format);
        else {
            GLint prevRead;
            glGetIntegerv(GL_READ_BUFFER, &prevRead);
            vcheckGLcall("glGetIntegerv");
            glReadBuffer(GL_BACK);
            vcheckGLcall("glReadBuffer");

            glCopyTexImage2D(This->glDescription.target,
                             This->glDescription.level,
                             This->glDescription.glFormatInternal,
                             0,
                             0,
                             This->currentDesc.Width,
                             This->currentDesc.Height,
                             0);

            checkGLcall("glCopyTexImage2D");
            glReadBuffer(prevRead);
            vcheckGLcall("glReadBuffer");
            TRACE("Updating target %d\n", This->glDescription.target);
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
              This->glDescription.target,
              This->glDescription.level,
              GL_RGBA,
              This->currentDesc.Width,
              This->currentDesc.Height,
              0,
              GL_RGBA,
              GL_UNSIGNED_BYTE,
              surface);
        glTexImage2D(This->glDescription.target,
                     This->glDescription.level,
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

    /* TODO: Compressed non-power 2 support */

    if (This->resource.format == WINED3DFMT_DXT1 ||
        This->resource.format == WINED3DFMT_DXT2 ||
        This->resource.format == WINED3DFMT_DXT3 ||
        This->resource.format == WINED3DFMT_DXT4 ||
        This->resource.format == WINED3DFMT_DXT5) {
        if (!GL_SUPPORT(EXT_TEXTURE_COMPRESSION_S3TC)) {
            FIXME("Using DXT1/3/5 without advertized support\n");
        } else if (This->resource.allocatedMemory) {
            TRACE("Calling glCompressedTexImage2D %x i=%d, intfmt=%x, w=%d, h=%d,0=%d, sz=%d, Mem=%p\n",
                  This->glDescription.target,
                  This->glDescription.level,
                  This->glDescription.glFormatInternal,
                  This->currentDesc.Width,
                  This->currentDesc.Height,
                  0,
                  This->resource.size,
                  This->resource.allocatedMemory);

            ENTER_GL();

            GL_EXTCALL(glCompressedTexImage2DARB)(This->glDescription.target,
                                                  This->glDescription.level,
                                                  This->glDescription.glFormatInternal,
                                                  This->currentDesc.Width,
                                                  This->currentDesc.Height,
                                                  0,
                                                  This->resource.size,
                                                  This->resource.allocatedMemory);
            checkGLcall("glCommpressedTexImage2D");

            LEAVE_GL();

            if(This->activeLock == FALSE){
                HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
                This->resource.allocatedMemory = NULL;
            }
        }
    } else {

       /* TODO: possibly use texture rectangle (though we are probably more compatible without it) */
        if (NP2_REPACK == wined3d_settings.nonpower2_mode && This->nonpow2 == TRUE) {


            TRACE("non power of two support\n");
            ENTER_GL();
            TRACE("(%p) Calling 2 glTexImage2D %x i=%d, d3dfmt=%s, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n", This,
                This->glDescription.target,
                This->glDescription.level,
                debug_d3dformat(This->resource.format),
                This->glDescription.glFormatInternal,
                This->pow2Width,
                This->pow2Height,
                0,
                This->glDescription.glFormat,
                This->glDescription.glType,
                NULL);

            glTexImage2D(This->glDescription.target,
                         This->glDescription.level,
                         This->glDescription.glFormatInternal,
                         This->pow2Width,
                         This->pow2Height,
                         0/*border*/,
                         This->glDescription.glFormat,
                         This->glDescription.glType,
                         NULL);

            checkGLcall("glTexImage2D");
            if (This->resource.allocatedMemory != NULL) {
                TRACE("(%p) Calling glTexSubImage2D w(%d) h(%d) mem(%p)\n", This, This->currentDesc.Width, This->currentDesc.Height, This->resource.allocatedMemory);
                /* And map the non-power two data into the top left corner */
                glTexSubImage2D(
                    This->glDescription.target,
                    This->glDescription.level,
                    0 /* xoffset */,
                    0 /* ysoffset */ ,
                    This->currentDesc.Width,
                    This->currentDesc.Height,
                    This->glDescription.glFormat,
                    This->glDescription.glType,
                    This->resource.allocatedMemory
                );
                checkGLcall("glTexSubImage2D");
            }
            LEAVE_GL();

        } else {

            TRACE("Calling 2 glTexImage2D %x i=%d, d3dfmt=%s, intfmt=%x, w=%d, h=%d,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
                This->glDescription.target,
                This->glDescription.level,
                debug_d3dformat(This->resource.format),
                This->glDescription.glFormatInternal,
                This->pow2Width,
                This->pow2Height,
                0,
                This->glDescription.glFormat,
                This->glDescription.glType,
                This->resource.allocatedMemory);

            ENTER_GL();
            glTexImage2D(This->glDescription.target,
                        This->glDescription.level,
                        This->glDescription.glFormatInternal,
                        This->pow2Width,
                        This->pow2Height,
                        0 /* border */,
                        This->glDescription.glFormat,
                        This->glDescription.glType,
                        This->resource.allocatedMemory);
            checkGLcall("glTexImage2D");
            LEAVE_GL();
        }

#if 0
        {
            static unsigned int gen = 0;
            char buffer[4096];
            ++gen;
            if ((gen % 10) == 0) {
                snprintf(buffer, sizeof(buffer), "/tmp/surface%p_type%u_level%u_%u.ppm", This, This->glDescription.target, This->glDescription.level, gen);
                IWineD3DSurfaceImpl_SaveSnapshot(iface, buffer);
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
        if(This->activeLock == FALSE){
            HeapFree(GetProcessHeap(),0,This->resource.allocatedMemory);
            This->resource.allocatedMemory = NULL;
        }

    }

    return D3D_OK;
}

#include <errno.h>
#include <stdio.h>
HRESULT WINAPI IWineD3DSurfaceImpl_SaveSnapshot(IWineD3DSurface *iface, const char* filename) {
    FILE* f = NULL;
    UINT i, y;
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    char *allocatedMemory;
    char *textureRow;
    IWineD3DSwapChain *swapChain = NULL;
    int width, height;
    GLuint tmpTexture;
    DWORD color;
    /*FIXME:
    Textures my not be stored in ->allocatedgMemory and a GlTexture
    so we should lock the surface before saving a snapshot, or at least check that
    */
    /* TODO: Compressed texture images can be obtained from the GL in uncompressed form
    by calling GetTexImage and in compressed form by calling
    GetCompressedTexImageARB.  Queried compressed images can be saved and
    later reused by calling CompressedTexImage[123]DARB.  Pre-compressed
    texture images do not need to be processed by the GL and should
    significantly improve texture loading performance relative to uncompressed
    images. */

/* Setup the width and height to be the internal texture width and height. */
    width  = This->pow2Width;
    height = This->pow2Height;
/* check to see if were a 'virtual' texture e.g. were not a pbuffer of texture were a back buffer*/
    IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapChain);

    if (swapChain || This->inPBuffer) { /* if were not a real texture then read the back buffer into a real texture*/
/* we don't want to interfere with the back buffer so read the data into a temporary texture and then save the data out of the temporary texture */
        GLint prevRead;
        ENTER_GL();
        FIXME("(%p) This surface needs to be locked before a snapshot can be taken\n", This);
        glEnable(GL_TEXTURE_2D);

        glGenTextures(1, &tmpTexture);
        glBindTexture(GL_TEXTURE_2D, tmpTexture);

        glTexImage2D(GL_TEXTURE_2D,
                        0,
                        GL_RGBA,
                        width,
                        height,
                        0/*border*/,
                        GL_RGBA,
                        GL_UNSIGNED_INT_8_8_8_8_REV,
                        NULL);

        glGetIntegerv(GL_READ_BUFFER, &prevRead);
        vcheckGLcall("glGetIntegerv");
        glReadBuffer(GL_BACK);
        vcheckGLcall("glReadBuffer");
        glCopyTexImage2D(GL_TEXTURE_2D,
                            0,
                            GL_RGBA,
                            0,
                            0,
                            width,
                            height,
                            0);

        checkGLcall("glCopyTexImage2D");
        glReadBuffer(prevRead);
        LEAVE_GL();

    } else { /* bind the real texture */
        IWineD3DSurface_PreLoad(iface);
    }
    allocatedMemory = HeapAlloc(GetProcessHeap(), 0, width  * height * 4);
    ENTER_GL();
    FIXME("Saving texture level %d width %d height %d\n", This->glDescription.level, width, height);
    glGetTexImage(GL_TEXTURE_2D,
                This->glDescription.level,
                GL_RGBA,
                GL_UNSIGNED_INT_8_8_8_8_REV,
                allocatedMemory);
    checkGLcall("glTexImage2D");
    if (tmpTexture) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &tmpTexture);
    }
    LEAVE_GL();

    f = fopen(filename, "w+");
    if (NULL == f) {
        ERR("opening of %s failed with: %s\n", filename, strerror(errno));
        return D3DERR_INVALIDCALL;
    }
/* Save the dat out to a TGA file because 1: it's an easy raw format, 2: it supports an alpha chanel*/
    TRACE("(%p) opened %s with format %s\n", This, filename, debug_d3dformat(This->resource.format));
/* TGA header */
    fputc(0,f);
    fputc(0,f);
    fputc(2,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
    fputc(0,f);
/* short width*/
    fwrite(&width,2,1,f);
/* short height */
    fwrite(&height,2,1,f);
/* format rgba */
    fputc(0x20,f);
    fputc(0x28,f);
/* raw data */
    /* if  the data is upside down if we've fetched it from a back buffer, so it needs flipping again to make it the correct way up*/
    if(swapChain)
        textureRow = allocatedMemory + (width * (height - 1) *4);
    else
        textureRow = allocatedMemory;
    for (y = 0 ; y < height; y++) {
        for (i = 0; i < width;  i++) {
            color = *((DWORD*)textureRow);
            fputc((color >> 16) & 0xFF, f); /* B */
            fputc((color >>  8) & 0xFF, f); /* G */
            fputc((color >>  0) & 0xFF, f); /* R */
            fputc((color >> 24) & 0xFF, f); /* A */
            textureRow += 4;
        }
        /* take two rows of the pointer to the texture memory */
        if(swapChain)
            (textureRow-= width << 3);

    }
    TRACE("Closing file\n");
    fclose(f);

    if(swapChain) {
        IWineD3DSwapChain_Release(swapChain);
    }
    HeapFree(GetProcessHeap(), 0, allocatedMemory);
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
    IWineD3DBaseTexture *baseTexture = NULL;
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
    /* if the container is a basetexture then mark it dirty. */
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == D3D_OK) {
        TRACE("Passing to conatiner\n");
        IWineD3DBaseTexture_SetDirty(baseTexture, TRUE);
        IWineD3DBaseTexture_Release(baseTexture);
    }
    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("This %p, container %p\n", This, container);

    /* We can't keep a reference to the container, since the container already keeps a reference to us. */

    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if (This->resource.format != WINED3DFMT_UNKNOWN) {
        FIXME("(%p) : The foramt of the surface must be WINED3DFORMAT_UNKNOWN\n", This);
        return D3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting texture foramt to (%d,%s)\n", This, format, debug_d3dformat(format));
    if (format == WINED3DFMT_UNKNOWN) {
        This->resource.size = 0;
    } else if (format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        This->resource.size = ((max(This->pow2Width, 4) * D3DFmtGetBpp(This->resource.wineD3DDevice, format)) * max(This->pow2Height, 4)) >> 1;

    } else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
               format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) {
        This->resource.size = ((max(This->pow2Width, 4) * D3DFmtGetBpp(This->resource.wineD3DDevice, format)) * max(This->pow2Height, 4));
    } else {
        This->resource.size = (This->pow2Width * D3DFmtGetBpp(This->resource.wineD3DDevice, format)) * This->pow2Height;
    }


    /* Setup some glformat defaults */
    if (format != WINED3DFMT_UNKNOWN) {
        This->glDescription.glFormat         = D3DFmt2GLFmt(This->resource.wineD3DDevice, format);
        This->glDescription.glFormatInternal = D3DFmt2GLIntFmt(This->resource.wineD3DDevice, format);
        This->glDescription.glType           = D3DFmt2GLType(This->resource.wineD3DDevice, format);
    } else {
        This->glDescription.glFormat         = 0;
        This->glDescription.glFormatInternal = 0;
        This->glDescription.glType           = 0;
    }

    if (format != WINED3DFMT_UNKNOWN) {
        This->bytesPerPixel = D3DFmtGetBpp(This->resource.wineD3DDevice, format);
        This->pow2Size      = (This->pow2Width * This->bytesPerPixel) * This->pow2Height;
    } else {
        This->bytesPerPixel = 0;
        This->pow2Size      = 0;
    }

    This->lockable = (WINED3DFMT_D16_LOCKABLE == format) ? TRUE : This->lockable;

    This->resource.format = format;

    TRACE("(%p) : Size %d, pow2Size %d, bytesPerPixel %d, glFormat %d, glFotmatInternal %d, glType %d\n", This, This->resource.size, This->pow2Size, This->bytesPerPixel, This->glDescription.glFormat, This->glDescription.glFormatInternal, This->glDescription.glType);

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
    IWineD3DSurfaceImpl_GetContainerParent,
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
    IWineD3DSurfaceImpl_SetPBufferState,
    IWineD3DSurfaceImpl_SetGlTextureDesc,
    IWineD3DSurfaceImpl_GetGlDesc,
    IWineD3DSurfaceImpl_GetData,
    IWineD3DSurfaceImpl_SetFormat
};
