/*
 * IWineD3DSurface Implementation
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Stefan Dösinger for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

typedef enum {
    NO_CONVERSION,
    CONVERT_PALETTED,
    CONVERT_PALETTED_CK,
    CONVERT_CK_565,
    CONVERT_CK_5551,
    CONVERT_CK_4444,
    CONVERT_CK_4444_ARGB,
    CONVERT_CK_1555,
    CONVERT_555,
    CONVERT_CK_RGB24,
    CONVERT_CK_8888,
    CONVERT_CK_8888_ARGB,
    CONVERT_RGB32_888
} CONVERT_TYPES;

HRESULT d3dfmt_convert_surface(BYTE *src, BYTE *dst, unsigned long len, CONVERT_TYPES convert, IWineD3DSurfaceImpl *surf);

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
        return S_OK;
    }
    *ppobj = NULL;
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
        IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->resource.wineD3DDevice;
        TRACE("(%p) : cleaning up\n", This);
        if (This->glDescription.textureName != 0) { /* release the openGL texture.. */
            ENTER_GL();
            TRACE("Deleting texture %d\n", This->glDescription.textureName);
            glDeleteTextures(1, &This->glDescription.textureName);
            LEAVE_GL();
        }

        if(This->Flags & SFLAG_DIBSECTION) {
            /* Release the DC */
            SelectObject(This->hDC, This->dib.holdbitmap);
            DeleteDC(This->hDC);
            /* Release the DIB section */
            DeleteObject(This->dib.DIBsection);
            This->dib.bitmap_data = NULL;
            This->resource.allocatedMemory = NULL;
        }

        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        if(iface == device->ddraw_primary)
            device->ddraw_primary = NULL;

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

void WINAPI IWineD3DSurfaceImpl_PreLoad(IWineD3DSurface *iface) {
    /* TODO: re-write the way textures and managed,
    *  use a 'opengl context manager' to manage RenderTarget surfaces
    ** *********************************************************/

    /* TODO: check for locks */
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;
    TRACE("(%p)Checking to see if the container is a base texture\n", This);
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == WINED3D_OK) {
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

    return WINED3D_OK;
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
    return WINED3D_OK;
}

void WINAPI IWineD3DSurfaceImpl_SetGlTextureDesc(IWineD3DSurface *iface, UINT textureName, int target) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p) : setting textureName %u, target %i\n", This, textureName, target);
    if (This->glDescription.textureName == 0 && textureName != 0) {
        This->Flags |= SFLAG_DIRTY;
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

static void read_from_framebuffer(IWineD3DSurfaceImpl *This, CONST RECT *rect, void *dest, UINT pitch) {
    long j;
    void *mem;
    GLint fmt;
    GLint type;

    switch(This->resource.format)
    {
        case WINED3DFMT_P8:
        {
            /* GL can't return palettized data, so read ARGB pixels into a
             * seperate block of memory and convert them into palettized format
             * in software. Slow, but if the app means to use palettized render
             * targets and lock it...
             *
             * Use GL_RGB, GL_UNSIGNED_BYTE to read the surface for performance reasons
             * Don't use GL_BGR as in the WINED3DFMT_R8G8B8 case, instead watch out
             * for the color channels when palettizing the colors.
             */
            fmt = GL_RGB;
            type = GL_UNSIGNED_BYTE;
            pitch *= 3;
            mem = HeapAlloc(GetProcessHeap(), 0, (rect->bottom - rect->top) * pitch);
            if(!mem) {
                ERR("Out of memory\n");
                return;
            }
        }
        break;

        default:
            mem = dest;
            fmt = This->glDescription.glFormat;
            type = This->glDescription.glType;
    }

    if (rect->left == 0 &&
        rect->right == This->currentDesc.Width ) {
        BYTE *row, *top, *bottom;
        int i;

        glReadPixels(0, rect->top,
                     This->currentDesc.Width,
                     rect->bottom - rect->top,
                     fmt,
                     type,
                     mem);

       /* glReadPixels returns the image upside down, and there is no way to prevent this.
          Flip the lines in software */
        row = HeapAlloc(GetProcessHeap(), 0, pitch);
        if(!row) {
            ERR("Out of memory\n");
            return;
        }
        top = mem;
        bottom = ((BYTE *) mem) + pitch * ( rect->bottom - rect->top - 1);
        for(i = 0; i < (rect->bottom - rect->top) / 2; i++) {
            memcpy(row, top, pitch);
            memcpy(top, bottom, pitch);
            memcpy(bottom, row, pitch);
            top += pitch;
            bottom -= pitch;
        }
        HeapFree(GetProcessHeap(), 0, row);

        if(This->lockedRect.top == 0 && This->lockedRect.bottom ==  This->currentDesc.Height) {
            This->Flags &= ~SFLAG_GLDIRTY;
        }
    } else {
        for (j = This->lockedRect.top; j < This->lockedRect.bottom - This->lockedRect.top; ++j) {
            glReadPixels(rect->left,
                         rect->bottom - j - 1,
                         rect->right - rect->left,
                         1,
                         fmt,
                         type,
                         (char *)mem + (pitch * (j-rect->top)));
        }
    }

    vcheckGLcall("glReadPixels");

    if(This->resource.format == WINED3DFMT_P8) {
        PALETTEENTRY *pal;
        DWORD width = pitch / 3;
        int x, y, c;
        if(This->palette) {
            pal = This->palette->palents;
        } else {
            pal = This->resource.wineD3DDevice->palettes[This->resource.wineD3DDevice->currentPalette];
        }

        for(y = rect->top; y < rect->bottom; y++) {
            for(x = rect->left; x < rect->right; x++) {
                /*                      start              lines            pixels      */
                BYTE *blue =  (BYTE *) ((BYTE *) mem) + y * pitch + x * (sizeof(BYTE) * 3);
                BYTE *green = blue  + 1;
                BYTE *red =   green + 1;

                for(c = 0; c < 256; c++) {
                    if(*red   == pal[c].peRed   &&
                       *green == pal[c].peGreen &&
                       *blue  == pal[c].peBlue)
                    {
                        *((BYTE *) dest + y * width + x) = c;
                        break;
                    }
                }
            }
        }
        HeapFree(GetProcessHeap(), 0, mem);
    }
}

static HRESULT WINAPI IWineD3DSurfaceImpl_LockRect(IWineD3DSurface *iface, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain = NULL;
    static UINT messages = 0; /* holds flags to disable fixme messages */
    BOOL backbuf = FALSE;

    /* fixme: should we really lock as such? */
    if((This->Flags & (SFLAG_INTEXTURE | SFLAG_INPBUFFER)) ==
                      (SFLAG_INTEXTURE | SFLAG_INPBUFFER) ) {
        FIXME("Warning: Surface is in texture memory or pbuffer\n");
        This->Flags &= ~(SFLAG_INTEXTURE | SFLAG_INPBUFFER);
    }

    if (!(This->Flags & SFLAG_LOCKABLE)) {
        /* Note: UpdateTextures calls CopyRects which calls this routine to populate the
              texture regions, and since the destination is an unlockable region we need
              to tolerate this                                                           */
        TRACE("Warning: trying to lock unlockable surf@%p\n", This);
        /*return WINED3DERR_INVALIDCALL; */
    }

    if (This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
        IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);

        if (swapchain != NULL ||  iface == myDevice->renderTarget || iface == myDevice->depthStencilBuffer) {
            if(swapchain != NULL) {
                int i;
                for(i = 0; i < swapchain->presentParms.BackBufferCount; i++) {
                    if(iface == swapchain->backBuffer[i]) {
                        backbuf = TRUE;
                        break;
                    }
                }
            }
            if (backbuf) {
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

    pLockedRect->Pitch = IWineD3DSurface_GetPitch(iface);

    if (NULL == pRect) {
        pLockedRect->pBits = This->resource.allocatedMemory;
        This->lockedRect.left   = 0;
        This->lockedRect.top    = 0;
        This->lockedRect.right  = This->currentDesc.Width;
        This->lockedRect.bottom = This->currentDesc.Height;
        TRACE("Locked Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", &This->lockedRect, This->lockedRect.left, This->lockedRect.top, This->lockedRect.right, This->lockedRect.bottom);
    } else {
        TRACE("Lock Rect (%p) = l %ld, t %ld, r %ld, b %ld\n", pRect, pRect->left, pRect->top, pRect->right, pRect->bottom);

        if ((pRect->top < 0) ||
             (pRect->left < 0) ||
             (pRect->left >= pRect->right) ||
             (pRect->top >= pRect->bottom) ||
             (pRect->right > This->currentDesc.Width) ||
             (pRect->bottom > This->currentDesc.Height))
        {
            WARN(" Invalid values in pRect !!!\n");
            return D3DERR_INVALIDCALL;
        }

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

    if (This->Flags & SFLAG_NONPOW2) {
        TRACE("Locking non-power 2 texture\n");
    }

    if (0 == This->resource.usage || This->resource.usage & WINED3DUSAGE_DYNAMIC) {
        /* classic surface  TODO: non 2d surfaces?
        These resources may be POOL_SYSTEMMEM, so they must not access the device */
        TRACE("locking an ordinarary surface\n");
        /* Check to see if memory has already been allocated from the surface*/
        if ((NULL == This->resource.allocatedMemory) ||
            (This->Flags & SFLAG_GLDIRTY) ){ /* TODO: check to see if an update has been performed on the surface (an update could just clobber allocatedMemory */
            /* Non-system memory surfaces */

            This->Flags &= ~SFLAG_GLDIRTY;

            /*Surface has no memory currently allocated to it!*/
            TRACE("(%p) Locking rect\n" , This);
            if(!This->resource.allocatedMemory) {
                This->resource.allocatedMemory = HeapAlloc(GetProcessHeap() ,0 , This->pow2Size);
            }
            if (0 != This->glDescription.textureName) {
                /* Now I have to copy thing bits back */
                This->Flags |= SFLAG_ACTIVELOCK; /* When this flag is set to true, loading the surface again won't free THis->resource.allocatedMemory */
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
                        if (This->Flags & SFLAG_NONPOW2) {
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

    } else if (WINED3DUSAGE_RENDERTARGET & This->resource.usage ){ /* render surfaces */
        if((!(Flags&WINED3DLOCK_DISCARD) && (This->Flags & SFLAG_GLDIRTY)) || !This->resource.allocatedMemory) {
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

            This->Flags |= SFLAG_ACTIVELOCK; /*When this flag is set to true, loading the surface again won't free THis->resource.allocatedMemory*/
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
                    if (iface == swapchain->frontBuffer) {
                        TRACE("locking front\n");
                        glReadBuffer(GL_FRONT);
                    }
                    else if (iface == myDevice->renderTarget || backbuf) {
                        TRACE("locking back buffer\n");
                        glReadBuffer(GL_BACK);
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
                        if (backbuf) {
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
                switch(wined3d_settings.rendertargetlock_mode) {
                    case RTL_AUTO:
                    case RTL_READDRAW:
                    case RTL_READTEX:
                        read_from_framebuffer(This, &This->lockedRect, pLockedRect->pBits, pLockedRect->Pitch);
                        break;

                    case RTL_TEXDRAW:
                    case RTL_TEXTEX:
                        ERR("Reading from render target with a texture isn't implemented yet\n");
                        break;

                    case RTL_DISABLE:
                    {
                        static BOOL warned = FALSE;
                        if(!warned) {
                            ERR("Application tries to lock the render target, but render target locking is disabled\n");
                            warned = TRUE;
                        }
                    }
                    break;
                }
            }
            TRACE("Resetting buffer\n");

            glReadBuffer(prev_read);
            vcheckGLcall("glReadBuffer");

            LEAVE_GL();
        }
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
        if (WINED3D_OK == IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&pBaseTexture) && pBaseTexture != NULL) {
            TRACE("Making container dirty\n");
            IWineD3DBaseTexture_SetDirty(pBaseTexture, TRUE);
            IWineD3DBaseTexture_Release(pBaseTexture);
        } else {
            TRACE("Surface is standalone, no need to dirty the container\n");
        }
    }

    TRACE("returning memory@%p, pitch(%d) dirtyfied(%d)\n", pLockedRect->pBits, pLockedRect->Pitch, This->Flags & SFLAG_DIRTY ? 0 : 1);

    This->Flags |= SFLAG_LOCKED;
    return WINED3D_OK;
}

static void flush_to_framebuffer_drawpixels(IWineD3DSurfaceImpl *This) {
    GLint  prev_store;
    GLint  prev_rasterpos[4];
    GLint skipBytes = 0;
    BOOL storechanged = FALSE;
    GLint fmt, type;
    void *mem;

    glDisable(GL_TEXTURE_2D);
    vcheckGLcall("glDisable(GL_TEXTURE_2D)");
    glDisable(GL_TEXTURE_1D);
    vcheckGLcall("glDisable(GL_TEXTURE_1D)");

    glFlush();
    vcheckGLcall("glFlush");
    glGetIntegerv(GL_PACK_SWAP_BYTES, &prev_store);
    vcheckGLcall("glIntegerv");
    glGetIntegerv(GL_CURRENT_RASTER_POSITION, &prev_rasterpos[0]);
    vcheckGLcall("glIntegerv");
    glPixelZoom(1.0, -1.0);
    vcheckGLcall("glPixelZoom");

    /* If not fullscreen, we need to skip a number of bytes to find the next row of data */
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &skipBytes);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, This->currentDesc.Width);

    glRasterPos3i(This->lockedRect.left, This->lockedRect.top, 1);
    vcheckGLcall("glRasterPos2f");

    /* Some drivers(radeon dri, others?) don't like exceptions during
     * glDrawPixels. If the surface is a DIB section, it might be in GDIMode
     * after ReleaseDC. Reading it will cause an exception, which x11drv will
     * catch to put the dib section in InSync mode, which leads to a crash
     * and a blocked x server on my radeon card.
     *
     * The following lines read the dib section so it is put in inSync mode
     * before glDrawPixels is called and the crash is prevented. There won't
     * be any interfering gdi accesses, because UnlockRect is called from
     * ReleaseDC, and the app won't use the dc any more afterwards.
     */
    if(This->Flags & SFLAG_DIBSECTION) {
        volatile BYTE read;
        read = This->resource.allocatedMemory[0];
    }

    switch (This->resource.format) {
        /* No special care needed */
        case WINED3DFMT_A4R4G4B4:
        case WINED3DFMT_R5G6B5:
        case WINED3DFMT_A1R5G5B5:
        case WINED3DFMT_R8G8B8:
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
            break;

        case WINED3DFMT_X4R4G4B4:
        {
#if 0       /* Do we still need that? Those pixel formats have no alpha channel in gl any more */
            int size;
            unsigned short *data;
            data = (unsigned short *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0xF000;
                data++;
                size--;
            }
#endif
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
        }
        break;

        case WINED3DFMT_X1R5G5B5:
        {
#if 0       /* Do we still need that? Those pixel formats have no alpha channel in gl any more */
            int size;
            unsigned short *data;
            data = (unsigned short *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0x8000;
                data++;
                size--;
            }
#endif
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
        }
        break;

        case WINED3DFMT_X8R8G8B8:
        {
#if 0       /* Do we still need that? Those pixel formats have no alpha channel in gl any more */
            /* make sure the X byte is set to alpha on, since it 
               could be any random value this fixes the intro move in Pirates! */
            int size;
            unsigned int *data;
            data = (unsigned int *)This->resource.allocatedMemory;
            size = (This->lockedRect.bottom - This->lockedRect.top) * (This->lockedRect.right - This->lockedRect.left);
            while(size > 0) {
                *data |= 0xFF000000;
                data++;
                size--;
            }
#endif
        }

        case WINED3DFMT_A8R8G8B8:
        {
            glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
            vcheckGLcall("glPixelStorei");
            storechanged = TRUE;
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
        }
        break;

        case WINED3DFMT_A2R10G10B10:
        {
            glPixelStorei(GL_PACK_SWAP_BYTES, TRUE);
            vcheckGLcall("glPixelStorei");
            storechanged = TRUE;
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
        }
        break;

        case WINED3DFMT_P8:
        {
            UINT pitch = IWineD3DSurface_GetPitch((IWineD3DSurface *) This);    /* target is argb, 4 byte */
            int row;
            type = GL_UNSIGNED_BYTE;
            fmt = GL_RGBA;

            mem = HeapAlloc(GetProcessHeap(), 0, This->resource.size * sizeof(DWORD));
            if(!mem) {
                ERR("Out of memory\n");
                return;
            }
            for(row = This->dirtyRect.top; row < This->dirtyRect.bottom; row++) {
                d3dfmt_convert_surface(This->resource.allocatedMemory + row * pitch + This->lockedRect.left,
                                       (BYTE *) mem + row * pitch * 4 + This->lockedRect.left * 4,
                                       This->lockedRect.right - This->lockedRect.left,
                                       CONVERT_PALETTED, This);
            }
        }
        break;

        default:
            FIXME("Unsupported Format %u in locking func\n", This->resource.format);

            /* Give it a try */
            type = This->glDescription.glType;
            fmt = This->glDescription.glFormat;
            mem = This->resource.allocatedMemory;
    }

    glDrawPixels(This->lockedRect.right - This->lockedRect.left,
                 (This->lockedRect.bottom - This->lockedRect.top)-1,
                 fmt, type,
                 mem);
    checkGLcall("glDrawPixels");
    glPixelZoom(1.0,1.0);
    vcheckGLcall("glPixelZoom");

    glRasterPos3iv(&prev_rasterpos[0]);
    vcheckGLcall("glRasterPos3iv");

    /* Reset to previous pack row length */
    glPixelStorei(GL_UNPACK_ROW_LENGTH, skipBytes);
    vcheckGLcall("glPixelStorei GL_UNPACK_ROW_LENGTH");
    if(storechanged) {
        glPixelStorei(GL_PACK_SWAP_BYTES, prev_store);
        vcheckGLcall("glPixelStorei GL_PACK_SWAP_BYTES");
    }

    if(mem != This->resource.allocatedMemory) HeapFree(GetProcessHeap(), 0, mem);
    return;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_UnlockRect(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDeviceImpl  *myDevice = This->resource.wineD3DDevice;
    const char *buffername = "";
    IWineD3DSwapChainImpl *swapchain = NULL;
    BOOL backbuf = FALSE;

    if (!(This->Flags & SFLAG_LOCKED)) {
        WARN("trying to Unlock an unlocked surf@%p\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    if (WINED3DUSAGE_RENDERTARGET & This->resource.usage) {
        IWineD3DSurface_GetContainer(iface, &IID_IWineD3DSwapChain, (void **)&swapchain);

        if(swapchain) {
            int i;
            for(i = 0; i < swapchain->presentParms.BackBufferCount; i++) {
                if(iface == swapchain->backBuffer[i]) {
                    backbuf = TRUE;
                    break;
                }
            }
        }

        if (backbuf) {
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

    TRACE("(%p %s) : dirtyfied(%d)\n", This, buffername, This->Flags & SFLAG_DIRTY ? 1 : 0);

    if (!(This->Flags & SFLAG_DIRTY)) {
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

        if (backbuf || iface ==  implSwapChain->frontBuffer || iface == myDevice->renderTarget) {
            int tex;

            ENTER_GL();

            /* glDrawPixels transforms the raster position as though it was a vertex -
               we want to draw at screen position 0,0 - Set up ortho (rhw) mode as
               per drawprim (and leave set - it will sort itself out due to last_was_rhw */
            d3ddevice_set_ortho(This->resource.wineD3DDevice);

            if (iface ==  implSwapChain->frontBuffer) {
                glDrawBuffer(GL_FRONT);
                checkGLcall("glDrawBuffer GL_FRONT");
            } else if (backbuf || iface == myDevice->renderTarget) {
                glDrawBuffer(GL_BACK);
                checkGLcall("glDrawBuffer GL_BACK");
            }

            /* Disable higher textures before calling glDrawPixels */
            for(tex = 1; tex < GL_LIMITS(sampler_stages); tex++) {
                if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + tex));
                    checkGLcall("glActiveTextureARB");
                }
                glDisable(GL_TEXTURE_2D);
                checkGLcall("glDisable GL_TEXTURE_2D");
                glDisable(GL_TEXTURE_1D);
                checkGLcall("glDisable GL_TEXTURE_1D");
            }
            /* Activate texture 0, but don't disable it necessarilly */
            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
                checkGLcall("glActiveTextureARB");
            }

            /* And back buffers are not blended. Disable the depth test, 
               that helps performance */
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);

            switch(wined3d_settings.rendertargetlock_mode) {
                case RTL_AUTO:
                case RTL_READDRAW:
                case RTL_TEXDRAW:
                    flush_to_framebuffer_drawpixels(This);
                    break;

                case RTL_READTEX:
                case RTL_TEXTEX:
                    ERR("Writing to the render target with textures is not implemented yet\n");
                    break;

                case RTL_DISABLE:
                {
                    static BOOL warned = FALSE;
                    if(!warned) {
                        ERR("The application tries to write to the render target, but render target locking is disabled\n");
                        warned = TRUE;
                    }
                }
                break;
            }

            if(implSwapChain->backBuffer && implSwapChain->backBuffer[0]) {
                glDrawBuffer(GL_BACK);
                vcheckGLcall("glDrawBuffer");
            }
            if(myDevice->stateBlock->renderState[D3DRS_ZENABLE] == D3DZB_TRUE ||
               myDevice->stateBlock->renderState[D3DRS_ZENABLE] == D3DZB_USEW) glEnable(GL_DEPTH_TEST);
            if (myDevice->stateBlock->renderState[D3DRS_ALPHABLENDENABLE]) glEnable(GL_BLEND);

            LEAVE_GL();

            /** restore clean dirty state */
            IWineD3DSurface_CleanDirtyRect(iface);

        } else {
            FIXME("unsupported unlocking to Rendering surface surf@%p usage(%s)\n", This, debug_d3dusage(This->resource.usage));
        }
        IWineD3DSwapChain_Release((IWineD3DSwapChain *)implSwapChain);

    } else if (WINED3DUSAGE_DEPTHSTENCIL & This->resource.usage) { /* stencil surfaces */

        if (iface == myDevice->depthStencilBuffer) {
            FIXME("TODO stencil depth surface unlocking surf@%p usage(%lu)\n", This, This->resource.usage);
        } else {
            FIXME("unsupported unlocking to StencilDepth surface surf@%p usage(%lu)\n", This, This->resource.usage);
        }

    } else {
        FIXME("unsupported unlocking to surface surf@%p usage(%s)\n", This, debug_d3dusage(This->resource.usage));
    }

    unlock_end:
    This->Flags &= ~SFLAG_LOCKED;
    memset(&This->lockedRect, 0, sizeof(RECT));
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetDC(IWineD3DSurface *iface, HDC *pHDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    WINED3DLOCKED_RECT lock;
    UINT usage;
    BITMAPINFO* b_info;
    HDC ddc;
    DWORD *masks;
    HRESULT hr;
    RGBQUAD col[256];
    const PixelFormatDesc *formatEntry = getFormatDescEntry(This->resource.format);

    TRACE("(%p)->(%p)\n",This,pHDC);

    /* Give more detailed info for ddraw */
    if (This->Flags & SFLAG_DCINUSE)
        return DDERR_DCALREADYCREATED;

    /* Can't GetDC if the surface is locked */
    if (This->Flags & SFLAG_LOCKED)
        return WINED3DERR_INVALIDCALL;

    memset(&lock, 0, sizeof(lock)); /* To be sure */

    /* Create a DIB section if there isn't a hdc yet */
    if(!This->hDC) {
        int extraline = 0;
        SYSTEM_INFO sysInfo;

        if(This->Flags & SFLAG_ACTIVELOCK) {
            ERR("Creating a DIB section while a lock is active. Uncertain consequences\n");
        }

        switch (This->bytesPerPixel) {
            case 2:
            case 4:
                /* Allocate extra space to store the RGB bit masks. */
                b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 3 * sizeof(DWORD));
                break;

            case 3:
                b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER));
                break;

            default:
                /* Allocate extra space for a palette. */
                b_info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  sizeof(BITMAPINFOHEADER)
                                  + sizeof(RGBQUAD)
                                  * (1 << (This->bytesPerPixel * 8)));
                break;
        }

        /* Some apps access the surface in via DWORDs, and do not take the necessary care at the end of the
         * surface. So we need at least extra 4 bytes at the end of the surface. Check against the page size,
         * if the last page used for the surface has at least 4 spare bytes we're safe, otherwise
         * add an extra line to the dib section
         */
        GetSystemInfo(&sysInfo);
        if( ((This->resource.size + 3) % sysInfo.dwPageSize) < 4) {
            extraline = 1;
            TRACE("Adding an extra line to the dib section\n");
        }

        b_info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        if( (NP2_REPACK == wined3d_settings.nonpower2_mode || This->resource.usage & WINED3DUSAGE_RENDERTARGET)) {
            b_info->bmiHeader.biWidth = This->currentDesc.Width;
            b_info->bmiHeader.biHeight = -This->currentDesc.Height -extraline;
            b_info->bmiHeader.biSizeImage = This->currentDesc.Width * This->currentDesc.Height * This->bytesPerPixel;
            /* Use the full pow2 image size(assigned below) because LockRect
             * will need it for a full glGetTexImage call
             */
        } else {
            b_info->bmiHeader.biWidth = This->pow2Width;
            b_info->bmiHeader.biHeight = -This->pow2Height -extraline;
            b_info->bmiHeader.biSizeImage = This->resource.size;
        }
        b_info->bmiHeader.biPlanes = 1;
        b_info->bmiHeader.biBitCount = This->bytesPerPixel * 8;

        b_info->bmiHeader.biXPelsPerMeter = 0;
        b_info->bmiHeader.biYPelsPerMeter = 0;
        b_info->bmiHeader.biClrUsed = 0;
        b_info->bmiHeader.biClrImportant = 0;

        /* Get the bit masks */
        masks = (DWORD *) &(b_info->bmiColors);
        switch (This->resource.format) {
            case WINED3DFMT_R8G8B8:
                usage = DIB_RGB_COLORS;
                b_info->bmiHeader.biCompression = BI_RGB;
                break;

            case WINED3DFMT_X1R5G5B5:
            case WINED3DFMT_A1R5G5B5:
            case WINED3DFMT_A4R4G4B4:
            case WINED3DFMT_X4R4G4B4:
            case WINED3DFMT_R3G3B2:
            case WINED3DFMT_A8R3G3B2:
            case WINED3DFMT_A2B10G10R10:
            case WINED3DFMT_A8B8G8R8:
            case WINED3DFMT_X8B8G8R8:
            case WINED3DFMT_A2R10G10B10:
            case WINED3DFMT_R5G6B5:
            case WINED3DFMT_A16B16G16R16:
                usage = 0;
                b_info->bmiHeader.biCompression = BI_BITFIELDS;
                masks[0] = formatEntry->redMask;
                masks[1] = formatEntry->greenMask;
                masks[2] = formatEntry->blueMask;
                break;

            default:
                /* Don't know palette */
                b_info->bmiHeader.biCompression = BI_RGB;
                usage = 0;
                break;
        }

        ddc = CreateDCA("DISPLAY", NULL, NULL, NULL);
        if (ddc == 0) {
            HeapFree(GetProcessHeap(), 0, b_info);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        TRACE("Creating a DIB section with size %ldx%ldx%d, size=%ld\n", b_info->bmiHeader.biWidth, b_info->bmiHeader.biHeight, b_info->bmiHeader.biBitCount, b_info->bmiHeader.biSizeImage);
        This->dib.DIBsection = CreateDIBSection(ddc, b_info, usage, &This->dib.bitmap_data, 0 /* Handle */, 0 /* Offset */);
        DeleteDC(ddc);

        if (!This->dib.DIBsection) {
            ERR("CreateDIBSection failed!\n");
            return HRESULT_FROM_WIN32(GetLastError());
        }
        HeapFree(GetProcessHeap(), 0, b_info);

        TRACE("DIBSection at : %p\n", This->dib.bitmap_data);

        /* copy the existing surface to the dib section */
        if(This->resource.allocatedMemory) {
            memcpy(This->dib.bitmap_data, This->resource.allocatedMemory, b_info->bmiHeader.biSizeImage);
            /* We won't need that any more */
            HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
        } else {
            /* This is to make LockRect read the gl Texture although memory is allocated */
            This->Flags |= SFLAG_GLDIRTY;
        }

        /* Use the dib section from now on */
        This->resource.allocatedMemory = This->dib.bitmap_data;

        /* Now allocate a HDC */
        This->hDC = CreateCompatibleDC(0);
        This->dib.holdbitmap = SelectObject(This->hDC, This->dib.DIBsection);
        TRACE("using wined3d palette %p\n", This->palette);
        SelectPalette(This->hDC,
                      This->palette ? This->palette->hpal : 0,
                      FALSE);

        This->Flags |= SFLAG_DIBSECTION;
    }

    /* Lock the surface */
    hr = IWineD3DSurface_LockRect(iface,
                                  &lock,
                                  NULL,
                                  0);
    if(FAILED(hr)) {
        ERR("IWineD3DSurface_LockRect failed with hr = %08lx\n", hr);
        /* keep the dib section */
        return hr;
    }

    if(This->resource.format == WINED3DFMT_P8 ||
        This->resource.format == WINED3DFMT_A8P8) {
        unsigned int n;
        if(This->palette) {
            PALETTEENTRY ent[256];

            GetPaletteEntries(This->palette->hpal, 0, 256, ent);
            for (n=0; n<256; n++) {
                col[n].rgbRed   = ent[n].peRed;
                col[n].rgbGreen = ent[n].peGreen;
                col[n].rgbBlue  = ent[n].peBlue;
                col[n].rgbReserved = 0;
            }
        } else {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            for (n=0; n<256; n++) {
                col[n].rgbRed   = device->palettes[device->currentPalette][n].peRed;
                col[n].rgbGreen = device->palettes[device->currentPalette][n].peGreen;
                col[n].rgbBlue  = device->palettes[device->currentPalette][n].peBlue;
                col[n].rgbReserved = 0;
            }

        }
        SetDIBColorTable(This->hDC, 0, 256, col);
    }

    *pHDC = This->hDC;
    TRACE("returning %p\n",*pHDC);
    This->Flags |= SFLAG_DCINUSE;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_ReleaseDC(IWineD3DSurface *iface, HDC hDC) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("(%p)->(%p)\n",This,hDC);

    if (!(This->Flags & SFLAG_DCINUSE))
        return D3DERR_INVALIDCALL;

    /* we locked first, so unlock now */
    IWineD3DSurface_UnlockRect(iface);

    This->Flags &= ~SFLAG_DCINUSE;

    return WINED3D_OK;
}

/* ******************************************************
   IWineD3DSurface Internal (No mapping to directx api) parts follow
   ****************************************************** */

HRESULT d3dfmt_get_conv(IWineD3DSurfaceImpl *This, BOOL need_alpha_ck, GLenum *format, GLenum *internal, GLenum *type, CONVERT_TYPES *convert, int *target_bpp) {
    BOOL colorkey_active = need_alpha_ck && (This->CKeyFlags & DDSD_CKSRCBLT);
    const PixelFormatDesc *formatEntry = getFormatDescEntry(This->resource.format);

    /* Default values: From the surface */
    *format = formatEntry->glFormat;
    *internal = formatEntry->glInternal;
    *type = formatEntry->glType;
    *convert = NO_CONVERSION;
    *target_bpp = This->bytesPerPixel;

    /* Ok, now look if we have to do any conversion */
    switch(This->resource.format) {
        case WINED3DFMT_P8:
            /* ****************
                Paletted Texture
                **************** */
            if(!GL_SUPPORT(EXT_PALETTED_TEXTURE) || colorkey_active) {
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_BYTE;
                *target_bpp = 4;
                if(colorkey_active) {
                    *convert = CONVERT_PALETTED;
                } else {
                    *convert = CONVERT_PALETTED_CK;
                }
            }

            break;

        case WINED3DFMT_R3G3B2:
            /* **********************
                GL_UNSIGNED_BYTE_3_3_2
                ********************** */
            if (colorkey_active) {
                /* This texture format will never be used.. So do not care about color keying
                    up until the point in time it will be needed :-) */
                FIXME(" ColorKeying not supported in the RGB 332 format !\n");
            }
            break;

        case WINED3DFMT_R5G6B5:
            if (colorkey_active) {
                *convert = CONVERT_CK_565;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_SHORT_5_5_5_1;
            }
            break;

        case WINED3DFMT_R8G8B8:
            if (colorkey_active) {
                *convert = CONVERT_CK_RGB24;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_INT_8_8_8_8;
                *target_bpp = 4;
            }
            break;

        case WINED3DFMT_X8R8G8B8:
            if (colorkey_active) {
                *convert = CONVERT_RGB32_888;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_INT_8_8_8_8;
            }
            break;
#if 0
        /* Not sure if we should do color keying on Alpha-Enabled surfaces */
        case WINED3DFMT_A4R4G4B4:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_4444_ARGB;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_SHORT_4_4_4_4;
            }
            break;

        case WINED3DFMT_A1R5G5B5:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_1555;
            }

        case WINED3DFMT_A8R8G8B8:
            if (colorkey_active)
            {
                *convert = CONVERT_CK_8888_ARGB;
                *format = GL_RGBA;
                *internal = GL_RGBA;
                *type = GL_UNSIGNED_INT_8_8_8_8;
            }
            break;
#endif
        default:
            break;
    }

    return WINED3D_OK;
}

HRESULT d3dfmt_convert_surface(BYTE *src, BYTE *dst, unsigned long len, CONVERT_TYPES convert, IWineD3DSurfaceImpl *surf) {
    TRACE("(%p)->(%p),(%ld,%d,%p)\n", src, dst, len, convert, surf);

    switch (convert) {
        case NO_CONVERSION:
        {
            memcpy(dst, src, len * surf->bytesPerPixel);
            break;
        }
        case CONVERT_PALETTED:
        case CONVERT_PALETTED_CK:
        {
            IWineD3DPaletteImpl* pal = surf->palette;
            BYTE table[256][4];
            unsigned int i;
            unsigned int x;

            if( pal == NULL) {
                /* TODO: If we are a sublevel, try to get the palette from level 0 */
            }

            if (pal == NULL) {
                /* Still no palette? Use the device's palette */
                /* Get the surface's palette */
                for (i = 0; i < 256; i++) {
                    IWineD3DDeviceImpl *device = surf->resource.wineD3DDevice;

                    table[i][0] = device->palettes[device->currentPalette][i].peRed;
                    table[i][1] = device->palettes[device->currentPalette][i].peGreen;
                    table[i][2] = device->palettes[device->currentPalette][i].peBlue;
                    if ((convert == CONVERT_PALETTED_CK) &&
                        (i >= surf->SrcBltCKey.dwColorSpaceLowValue) &&
                        (i <= surf->SrcBltCKey.dwColorSpaceHighValue)) {
                        /* We should maybe here put a more 'neutral' color than the standard bright purple
                          one often used by application to prevent the nice purple borders when bi-linear
                          filtering is on */
                        table[i][3] = 0x00;
                    } else {
                        table[i][3] = 0xFF;
                    }
                }
            } else {
                TRACE("Using surface palette %p\n", pal);
                /* Get the surface's palette */
                for (i = 0; i < 256; i++) {
                    table[i][0] = pal->palents[i].peRed;
                    table[i][1] = pal->palents[i].peGreen;
                    table[i][2] = pal->palents[i].peBlue;
                    if ((convert == CONVERT_PALETTED_CK) &&
                        (i >= surf->SrcBltCKey.dwColorSpaceLowValue) &&
                        (i <= surf->SrcBltCKey.dwColorSpaceHighValue)) {
                        /* We should maybe here put a more 'neutral' color than the standard bright purple
                          one often used by application to prevent the nice purple borders when bi-linear
                          filtering is on */
                        table[i][3] = 0x00;
                    } else {
                        table[i][3] = 0xFF;
                    }
                }
            }

            for (x = 0; x < len; x++) {
                BYTE color = *src++;
                *dst++ = table[color][0];
                *dst++ = table[color][1];
                *dst++ = table[color][2];
                *dst++ = table[color][3];
            }
        }
        break;

        case CONVERT_CK_565:
        {
            /* Converting the 565 format in 5551 packed to emulate color-keying.

              Note : in all these conversion, it would be best to average the averaging
                      pixels to get the color of the pixel that will be color-keyed to
                      prevent 'color bleeding'. This will be done later on if ever it is
                      too visible.

              Note2: when using color-keying + alpha, are the alpha bits part of the
                      color-space or not ?
            */
            unsigned int x;
            WORD *Source = (WORD *) src;
            WORD *Dest = (WORD *) dst;

            TRACE("Color keyed 565\n");

            for (x = 0; x < len; x++ ) {
                WORD color = *Source++;
                *Dest = ((color & 0xFFC0) | ((color & 0x1F) << 1));
                if ((color < surf->SrcBltCKey.dwColorSpaceLowValue) ||
                    (color > surf->SrcBltCKey.dwColorSpaceHighValue)) {
                    *Dest |= 0x0001;
                }
                Dest++;
            }
        }
        break;

        case CONVERT_CK_1555:
        {
            unsigned int x;
            WORD *Source = (WORD *) src;
            WORD *Dest = (WORD *) dst;

            for (x = 0; x < len * sizeof(WORD); x+=sizeof(WORD)) {
                WORD color = *Source++;
                *Dest = (color & 0x7FFF);
                if ((color < surf->SrcBltCKey.dwColorSpaceLowValue) ||
                    (color > surf->SrcBltCKey.dwColorSpaceHighValue))
                    *Dest |= (color & 0x8000);
                Dest++;
            }
        }
        break;

        case CONVERT_CK_4444_ARGB:
        {
            /* Move the four Alpha bits... */
            unsigned int x;
            WORD *Source = (WORD *) src;
            WORD *Dest = (WORD *) dst;

            for (x = 0; x < len; x++) {
                WORD color = *Source++;
                *dst = (color & 0x0FFF) << 4;
                if ((color < surf->SrcBltCKey.dwColorSpaceLowValue) ||
                    (color > surf->SrcBltCKey.dwColorSpaceHighValue))
                    *Dest |= (color & 0xF000) >> 12;
                Dest++;
            }
        } break;

        default:
            ERR("Unsupported conversation type %d\n", convert);
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_LoadTexture(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if (This->Flags & SFLAG_INTEXTURE) {
        TRACE("Surface already in texture\n");
        return WINED3D_OK;
    }
    if (!(This->Flags & SFLAG_DIRTY)) {
        TRACE("surface isn't dirty\n");
        return WINED3D_OK;
    }

    This->Flags &= ~SFLAG_DIRTY;

    /* Resources are placed in system RAM and do not need to be recreated when a device is lost.
    *  These resources are not bound by device size or format restrictions. Because of this,
    *  these resources cannot be accessed by the Direct3D device nor set as textures or render targets.
    *  However, these resources can always be created, locked, and copied.
    */
    if (This->resource.pool == WINED3DPOOL_SCRATCH)
    {
        FIXME("(%p) Operation not supported for scratch textures\n",This);
        return WINED3DERR_INVALIDCALL;
    }

    if (This->Flags & SFLAG_INPBUFFER) {
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
            This->Flags |= SFLAG_INTEXTURE;
        }
        LEAVE_GL();
        return WINED3D_OK;
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

            if(!(This->Flags & SFLAG_DONOTFREE)){
                HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
                This->resource.allocatedMemory = NULL;
            }
        }
    } else {
        GLenum format, internal, type;
        CONVERT_TYPES convert;
        int bpp;
        BYTE *mem;

        d3dfmt_get_conv(This, TRUE /* We need color keying */, &format, &internal, &type, &convert, &bpp);

        if((convert != NO_CONVERSION) &&
	   This->resource.allocatedMemory) {
            int width = This->glRect.right - This->glRect.left;
            int height = This->glRect.bottom - This->glRect.top;
            int row;

            mem = HeapAlloc(GetProcessHeap(), 0, width * height * bpp);
            if(!mem) {
                ERR("Out of memory %d, %d!\n", width, height);
                return WINED3DERR_OUTOFVIDEOMEMORY;
            }

            for(row = This->glRect.top; row < This->glRect.bottom; row++) {
                BYTE *cur = This->resource.allocatedMemory + row * This->pow2Width * This->bytesPerPixel;
                d3dfmt_convert_surface(cur + This->glRect.left * This->bytesPerPixel,
                                       mem + row * width * bpp,
                                       width,
                                       convert,
                                       This);
            }
            This->Flags |= SFLAG_CONVERTED;
        } else {
            This->Flags &= ~SFLAG_CONVERTED;
            mem = This->resource.allocatedMemory;
        }

       /* TODO: possibly use texture rectangle (though we are probably more compatible without it) */
        if (NP2_REPACK == wined3d_settings.nonpower2_mode && (This->Flags & SFLAG_NONPOW2) && !(This->Flags & SFLAG_OVERSIZE) ) {


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

            TRACE("Calling 2 glTexImage2D %x i=%d, d3dfmt=%s, intfmt=%x, w=%ld, h=%ld,0=%d, glFmt=%x, glType=%x, Mem=%p\n",
                This->glDescription.target,
                This->glDescription.level,
                debug_d3dformat(This->resource.format),
                This->glDescription.glFormatInternal,
                This->glRect.right - This->glRect.left,
                This->glRect.bottom - This->glRect.top,
                0,
                This->glDescription.glFormat,
                This->glDescription.glType,
                mem);

            ENTER_GL();

            /* OK, create the texture */
            glTexImage2D(This->glDescription.target,
                        This->glDescription.level,
                        internal,
                        This->glRect.right - This->glRect.left,
                        This->glRect.bottom - This->glRect.top,
                        0 /* border */,
                        format,
                        type,
                        mem);

            checkGLcall("glTexImage2D");

            LEAVE_GL();
        }
        if(mem != This->resource.allocatedMemory)
            HeapFree(GetProcessHeap(), 0, mem);

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
        if(!(This->Flags & SFLAG_DONOTFREE)){
            HeapFree(GetProcessHeap(),0,This->resource.allocatedMemory);
            This->resource.allocatedMemory = NULL;
        }

    }

    return WINED3D_OK;
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

    if (swapChain || (This->Flags & SFLAG_INPBUFFER)) { /* if were not a real texture then read the back buffer into a real texture*/
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
        return WINED3DERR_INVALIDCALL;
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
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_CleanDirtyRect(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    This->Flags &= ~SFLAG_DIRTY;
    This->dirtyRect.left   = This->currentDesc.Width;
    This->dirtyRect.top    = This->currentDesc.Height;
    This->dirtyRect.right  = 0;
    This->dirtyRect.bottom = 0;
    TRACE("(%p) : Dirty?%d, Rect:(%ld,%ld,%ld,%ld)\n", This, This->Flags & SFLAG_DIRTY ? 1 : 0, This->dirtyRect.left,
          This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);
    return WINED3D_OK;
}

/**
 *   Slightly inefficient way to handle multiple dirty rects but it works :)
 */
extern HRESULT WINAPI IWineD3DSurfaceImpl_AddDirtyRect(IWineD3DSurface *iface, CONST RECT* pDirtyRect) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DBaseTexture *baseTexture = NULL;
    This->Flags |= SFLAG_DIRTY;
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
    TRACE("(%p) : Dirty?%d, Rect:(%ld,%ld,%ld,%ld)\n", This, This->Flags & SFLAG_DIRTY, This->dirtyRect.left,
          This->dirtyRect.top, This->dirtyRect.right, This->dirtyRect.bottom);
    /* if the container is a basetexture then mark it dirty. */
    if (IWineD3DSurface_GetContainer(iface, &IID_IWineD3DBaseTexture, (void **)&baseTexture) == WINED3D_OK) {
        TRACE("Passing to conatiner\n");
        IWineD3DBaseTexture_SetDirty(baseTexture, TRUE);
        IWineD3DBaseTexture_Release(baseTexture);
    }
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("This %p, container %p\n", This, container);

    /* We can't keep a reference to the container, since the container already keeps a reference to us. */

    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    const PixelFormatDesc *formatEntry = getFormatDescEntry(format);

    if (This->resource.format != WINED3DFMT_UNKNOWN) {
        FIXME("(%p) : The foramt of the surface must be WINED3DFORMAT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting texture foramt to (%d,%s)\n", This, format, debug_d3dformat(format));
    if (format == WINED3DFMT_UNKNOWN) {
        This->resource.size = 0;
    } else if (format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4)) >> 1;

    } else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
               format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) {
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4));
    } else {
        This->resource.size = (This->pow2Width * formatEntry->bpp) * This->pow2Height;
    }


    /* Setup some glformat defaults */
    This->glDescription.glFormat         = formatEntry->glFormat;
    This->glDescription.glFormatInternal = formatEntry->glInternal;
    This->glDescription.glType           = formatEntry->glType;

    if (format != WINED3DFMT_UNKNOWN) {
        This->bytesPerPixel = formatEntry->bpp;
        This->pow2Size      = (This->pow2Width * This->bytesPerPixel) * This->pow2Height;
    } else {
        This->bytesPerPixel = 0;
        This->pow2Size      = 0;
    }

    This->Flags |= (WINED3DFMT_D16_LOCKABLE == format) ? SFLAG_LOCKABLE : 0;

    This->resource.format = format;

    TRACE("(%p) : Size %d, pow2Size %d, bytesPerPixel %d, glFormat %d, glFotmatInternal %d, glType %d\n", This, This->resource.size, This->pow2Size, This->bytesPerPixel, This->glDescription.glFormat, This->glDescription.glFormatInternal, This->glDescription.glType);

    return WINED3D_OK;
}

/* TODO: replace this function with context management routines */
HRESULT WINAPI IWineD3DSurfaceImpl_SetPBufferState(IWineD3DSurface *iface, BOOL inPBuffer, BOOL  inTexture) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    if(inPBuffer) {
        This->Flags |= SFLAG_INPBUFFER;
    } else {
        This->Flags &= ~SFLAG_INPBUFFER;
    }

    if(inTexture) {
        This->Flags |= SFLAG_INTEXTURE;
    } else {
        This->Flags &= ~SFLAG_INTEXTURE;
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Flip(IWineD3DSurface *iface, IWineD3DSurface *override, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DDevice *D3D = (IWineD3DDevice *) This->resource.wineD3DDevice;
    TRACE("(%p)->(%p,%lx)\n", This, override, Flags);

    /* Flipping is only supported on RenderTargets */
    if( !(This->resource.usage & WINED3DUSAGE_RENDERTARGET) ) return DDERR_NOTFLIPPABLE;

    if(override) {
        /* DDraw sets this for the X11 surfaces, so don't confuse the user 
         * FIXME("(%p) Target override is not supported by now\n", This);
         * Additionally, it isn't really possible to support triple-buffering
         * properly on opengl at all
         */
    }

    /* Flipping a OpenGL surface -> Use WineD3DDevice::Present */
    return IWineD3DDevice_Present(D3D, NULL, NULL, 0, NULL);
}

/* Not called from the VTable */
static HRESULT IWineD3DSurfaceImpl_BltOverride(IWineD3DSurfaceImpl *This, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, DDBLTFX *DDBltFx) {
    D3DRECT rect;
    IWineD3DDeviceImpl *myDevice = This->resource.wineD3DDevice;
    IWineD3DSwapChainImpl *swapchain = NULL;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    BOOL SrcOK = TRUE;

    TRACE("(%p)->(%p,%p,%p,%08lx,%p)\n", This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);

    /* Get the swapchain. One of the surfaces has to be a primary surface */
    IWineD3DSurface_GetContainer( (IWineD3DSurface *) This, &IID_IWineD3DSwapChain, (void **)&swapchain);
    if(swapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
    else if(Src) {
        IWineD3DSurface_GetContainer( (IWineD3DSurface *) Src, &IID_IWineD3DSwapChain, (void **)&swapchain);
        if(swapchain) IWineD3DSwapChain_Release((IWineD3DSwapChain *) swapchain);
        else return WINED3DERR_INVALIDCALL;
    } else {
        swapchain = NULL;
    }

    if (DestRect) {
        rect.x1 = DestRect->left;
        rect.y1 = DestRect->top;
        rect.x2 = DestRect->right;
        rect.y2 = DestRect->bottom;
    } else {
        rect.x1 = 0;
        rect.y1 = 0;
        rect.x2 = This->currentDesc.Width;
        rect.y2 = This->currentDesc.Height;
    }

    /* Half-life does a Blt from the back buffer to the front buffer,
     * Full surface size, no flags... Use present instead
     */
    if(Src)
    {
        /* First, check if we can do a Flip */

        /* Check rects - IWineD3DDevice_Present doesn't handle them */
        if( SrcRect ) {
            if( (SrcRect->left == 0) && (SrcRect->top == 0) &&
                (SrcRect->right == Src->currentDesc.Width) && (SrcRect->bottom == Src->currentDesc.Height) ) {
                SrcOK = TRUE;
            }
        } else {
            SrcOK = TRUE;
        }

        /* Check the Destination rect and the surface sizes */
        if(SrcOK &&
           (rect.x1 == 0) && (rect.y1 == 0) &&
           (rect.x2 ==  This->currentDesc.Width) && (rect.y2 == This->currentDesc.Height) &&
           (This->currentDesc.Width == Src->currentDesc.Width) &&
           (This->currentDesc.Height == Src->currentDesc.Height)) {
            /* These flags are unimportant for the flag check, remove them */

            if((Flags & ~(DDBLT_DONOTWAIT | DDBLT_WAIT)) == 0) {
                if( swapchain->backBuffer && ((IWineD3DSurface *) This == swapchain->frontBuffer) && ((IWineD3DSurface *) Src == swapchain->backBuffer[0]) ) {

                    D3DSWAPEFFECT orig_swap = swapchain->presentParms.SwapEffect;

                    /* The idea behind this is that a glReadPixels and a glDrawPixels call
                     * take very long, while a flip is fast.
                     * This applies to Half-Life, which does such Blts every time it finished
                     * a frame, and to Prince of Persia 3D, which uses this to draw at least the main
                     * menu. This is also used by all apps when they do windowed rendering
                     *
                     * The problem is that flipping is not really the same as copying. After a
                     * Blt the front buffer is a copy of the back buffer, and the back buffer is
                     * untouched. Therefore it's necessary to override the swap effect
                     * and to set it back after the flip.
                     */

                    swapchain->presentParms.SwapEffect = WINED3DSWAPEFFECT_COPY;

                    TRACE("Full screen back buffer -> front buffer blt, performing a flip instead\n");
                    IWineD3DDevice_Present((IWineD3DDevice *) This->resource.wineD3DDevice,
                                            NULL, NULL, 0, NULL);

                    swapchain->presentParms.SwapEffect = orig_swap;

                    return WINED3D_OK;
                }
            }
        }

        /* Blt from texture to rendertarget? */
        if( ( ( (IWineD3DSurface *) This == swapchain->frontBuffer) ||
              ( swapchain->backBuffer && (IWineD3DSurface *) This == swapchain->backBuffer[0]) )
              &&
              ( ( (IWineD3DSurface *) Src != swapchain->frontBuffer) &&
                ( swapchain->backBuffer && (IWineD3DSurface *) Src != swapchain->backBuffer[0]) ) ) {
            float glTexCoord[4];
            DWORD oldCKey;
            GLint oldLight, oldFog, oldDepth, oldBlend, oldCull, oldAlpha;
            GLint alphafunc;
            GLclampf alpharef;
            GLint oldStencil;
            RECT SourceRectangle;
            GLint oldDraw;

            TRACE("Blt from surface %p to rendertarget %p\n", Src, This);

            if(SrcRect) {
                SourceRectangle.left = SrcRect->left;
                SourceRectangle.right = SrcRect->right;
                SourceRectangle.top = SrcRect->top;
                SourceRectangle.bottom = SrcRect->bottom;
            } else {
                SourceRectangle.left = 0;
                SourceRectangle.right = Src->currentDesc.Width;
                SourceRectangle.top = 0;
                SourceRectangle.bottom = Src->currentDesc.Height;
            }

            if(!CalculateTexRect(Src, &SourceRectangle, glTexCoord)) {
                /* Fall back to software */
                WARN("(%p) Source texture area (%ld,%ld)-(%ld,%ld) is too big\n", Src,
                     SourceRectangle.left, SourceRectangle.top,
                     SourceRectangle.right, SourceRectangle.bottom);
                return WINED3DERR_INVALIDCALL;
            }

            /* Color keying: Check if we have to do a color keyed blt,
             * and if not check if a color key is activated.
             */
            oldCKey = Src->CKeyFlags;
            if(!(Flags & DDBLT_KEYSRC) && 
               Src->CKeyFlags & DDSD_CKSRCBLT) {
                /* Ok, the surface has a color key, but we shall not use it - 
                 * Deactivate it for now and dirtify the surface to reload it
                 */
                Src->CKeyFlags &= ~DDSD_CKSRCBLT;
                Src->Flags |= SFLAG_DIRTY;
            }

            /* Now load the surface */
            IWineD3DSurface_PreLoad((IWineD3DSurface *) Src);

            ENTER_GL();

            /* Save all the old stuff until we have a proper opengl state manager */
            oldLight = glIsEnabled(GL_LIGHTING);
            oldFog = glIsEnabled(GL_FOG);
            oldDepth = glIsEnabled(GL_DEPTH_TEST);
            oldBlend = glIsEnabled(GL_BLEND);
            oldCull = glIsEnabled(GL_CULL_FACE);
            oldAlpha = glIsEnabled(GL_ALPHA_TEST);
            oldStencil = glIsEnabled(GL_STENCIL_TEST);

            glGetIntegerv(GL_ALPHA_TEST_FUNC, &alphafunc);
            checkGLcall("glGetFloatv GL_ALPHA_TEST_FUNC");
            glGetFloatv(GL_ALPHA_TEST_REF, &alpharef);
            checkGLcall("glGetFloatv GL_ALPHA_TEST_REF");

            glGetIntegerv(GL_DRAW_BUFFER, &oldDraw);
            if(This == (IWineD3DSurfaceImpl *) swapchain->frontBuffer) {
                TRACE("Drawing to front buffer\n");
                glDrawBuffer(GL_FRONT);
                checkGLcall("glDrawBuffer GL_FRONT");
            }

            /* Unbind the old texture */
            glBindTexture(GL_TEXTURE_2D, 0);

            if (GL_SUPPORT(ARB_MULTITEXTURE)) {
            /* We use texture unit 0 for blts */
                GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
                checkGLcall("glActiveTextureARB");
            } else {
                WARN("Multi-texturing is unsupported in the local OpenGL implementation\n");
            }

            /* Disable some fancy graphics effects */
            glDisable(GL_LIGHTING);
            checkGLcall("glDisable GL_LIGHTING");
            glDisable(GL_DEPTH_TEST);
            checkGLcall("glDisable GL_DEPTH_TEST");
            glDisable(GL_FOG);
            checkGLcall("glDisable GL_FOG");
            glDisable(GL_BLEND);
            checkGLcall("glDisable GL_BLEND");
            glDisable(GL_CULL_FACE);
            checkGLcall("glDisable GL_CULL_FACE");
            glDisable(GL_STENCIL_TEST);
            checkGLcall("glDisable GL_STENCIL_TEST");

            /* Ok, we need 2d textures, but not 1D or 3D */
            glDisable(GL_TEXTURE_1D);
            checkGLcall("glDisable GL_TEXTURE_1D");
            glEnable(GL_TEXTURE_2D);
            checkGLcall("glEnable GL_TEXTURE_2D");
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable GL_TEXTURE_3D");

            /* Bind the texture */
            glBindTexture(GL_TEXTURE_2D, Src->glDescription.textureName);
            checkGLcall("glBindTexture");

            glEnable(GL_SCISSOR_TEST);

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

            /* No filtering for blts */
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                            GL_NEAREST);
            checkGLcall("glTexParameteri");
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                            GL_NEAREST);
            checkGLcall("glTexParameteri");
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            checkGLcall("glTexEnvi");

            /* This is for color keying */
            if(Flags & DDBLT_KEYSRC) {
                glEnable(GL_ALPHA_TEST);
                checkGLcall("glEnable GL_ALPHA_TEST");
                glAlphaFunc(GL_NOTEQUAL, 0.0);
                checkGLcall("glAlphaFunc\n");
            } else {
                glDisable(GL_ALPHA_TEST);
                checkGLcall("glDisable GL_ALPHA_TEST");
            }

            /* Draw a textured quad
             */
            d3ddevice_set_ortho(This->resource.wineD3DDevice);

            glBegin(GL_QUADS);

            glColor3d(1.0f, 1.0f, 1.0f);
            glTexCoord2f(glTexCoord[0], glTexCoord[2]);
            glVertex3f(rect.x1,
                       rect.y1,
                       0.0);

            glTexCoord2f(glTexCoord[0], glTexCoord[3]);
            glVertex3f(rect.x1, rect.y2, 0.0);

            glTexCoord2f(glTexCoord[1], glTexCoord[3]);
            glVertex3f(rect.x2,
                       rect.y2,
                       0.0);

            glTexCoord2f(glTexCoord[1], glTexCoord[2]);
            glVertex3f(rect.x2,
                       rect.y1,
                       0.0);
            glEnd();
            checkGLcall("glEnd");

            /* Unbind the texture */
            glBindTexture(GL_TEXTURE_2D, 0);
            checkGLcall("glEnable glBindTexture");

            /* Restore the old settings */
            if(oldLight) {
                glEnable(GL_LIGHTING);
                checkGLcall("glEnable GL_LIGHTING");
            }
            if(oldFog) {
                glEnable(GL_FOG);
                checkGLcall("glEnable GL_FOG");
            }
            if(oldDepth) {
                glEnable(GL_DEPTH_TEST);
                checkGLcall("glEnable GL_DEPTH_TEST");
            }
            if(oldBlend) {
                glEnable(GL_BLEND);
                checkGLcall("glEnable GL_BLEND");
            }
            if(oldCull) {
                glEnable(GL_CULL_FACE);
                checkGLcall("glEnable GL_CULL_FACE");
            }
            if(oldStencil) {
                glEnable(GL_STENCIL_TEST);
                checkGLcall("glEnable GL_STENCIL_TEST");
            }
            if(!oldAlpha) {
                glDisable(GL_ALPHA_TEST);
                checkGLcall("glDisable GL_ALPHA_TEST");
            } else {
                glEnable(GL_ALPHA_TEST);
                checkGLcall("glEnable GL_ALPHA_TEST");
            }

            glAlphaFunc(alphafunc, alpharef);
            checkGLcall("glAlphaFunc\n");

            if(This == (IWineD3DSurfaceImpl *) swapchain->frontBuffer && oldDraw == GL_BACK) {
                glDrawBuffer(oldDraw);
            }

            /* Restore the color key */
            if(oldCKey != Src->CKeyFlags) {
                Src->CKeyFlags = oldCKey;
                Src->Flags |= SFLAG_DIRTY;
            }

            LEAVE_GL();

            /* TODO: If the surface is locked often, perform the Blt in software on the memory instead */
            This->Flags |= SFLAG_GLDIRTY;

            return WINED3D_OK;
        }


        /* Blt from rendertarget to texture? */
        if( (SrcSurface == swapchain->frontBuffer) ||
            (swapchain->backBuffer && SrcSurface == swapchain->backBuffer[0]) ) {
            if( ( (IWineD3DSurface *) This != swapchain->frontBuffer) &&
                ( swapchain->backBuffer && (IWineD3DSurface *) This != swapchain->backBuffer[0]) ) {
                UINT row;
                D3DRECT srect;
                float xrel, yrel;

                TRACE("Blt from rendertarget to texture\n");

                /* Call preload for the surface to make sure it isn't dirty */
                IWineD3DSurface_PreLoad((IWineD3DSurface *) This);

                if(SrcRect) {
                    srect.x1 = SrcRect->left;
                    srect.y1 = SrcRect->top;
                    srect.x2 = SrcRect->right;
                    srect.y2 = SrcRect->bottom;
                } else {
                    srect.x1 = 0;
                    srect.y1 = 0;
                    srect.x2 = Src->currentDesc.Width;
                    srect.y2 = Src->currentDesc.Height;
                }

                ENTER_GL();

                /* Bind the target texture */
                glBindTexture(GL_TEXTURE_2D, This->glDescription.textureName);
                checkGLcall("glBindTexture");
                if(swapchain->backBuffer && SrcSurface == swapchain->backBuffer[0]) {
                    glReadBuffer(GL_BACK);
                } else {
                    glReadBuffer(GL_FRONT);
                }
                checkGLcall("glReadBuffer");

                xrel = (float) (srect.x2 - srect.x1) / (float) (rect.x2 - rect.x1);
                yrel = (float) (srect.y2 - srect.y1) / (float) (rect.y2 - rect.y1);

                /* I have to process this row by row to swap the image,
                 * otherwise it would be upside down, so streching in y direction
                 * doesn't cost extra time
                 *
                 * However, streching in x direction can be avoided if not necessary
                 */
                for(row = rect.y1; row < rect.y2; row++) {
                    if( (xrel - 1.0 < -eps) || (xrel - 1.0 > eps)) {
                        /* Well, that stuff works, but it's very slow.
                         * find a better way instead
                         */
                        UINT col;
                        for(col = rect.x1; col < rect.x2; col++) {
                            glCopyTexSubImage2D(GL_TEXTURE_2D,
                                                0, /* level */
                                                rect.x1 + col, This->currentDesc.Height - row - 1, /* xoffset, yoffset */
                                                srect.x1 + col * xrel, Src->currentDesc.Height - srect.y2 + row * yrel,
                                                1, 1);
                        }
                    } else {
                        glCopyTexSubImage2D(GL_TEXTURE_2D,
                                            0, /* level */
                                            rect.x1, This->currentDesc.Height - row - 1, /* xoffset, yoffset */
                                            srect.x1, Src->currentDesc.Height - srect.y2 + row * yrel,
                                            rect.x2, 1);
                    }
                }

                vcheckGLcall("glCopyTexSubImage2D");
                LEAVE_GL();

                if(!(This->Flags & SFLAG_DONOTFREE)) {
                    HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
                    This->resource.allocatedMemory = NULL;
                } else {
                    This->Flags |= SFLAG_GLDIRTY;
                }

                return WINED3D_OK;
            }
        }
    }

    if (Flags & DDBLT_COLORFILL) {
        /* This is easy to handle for the D3D Device... */
        DWORD color;
        IWineD3DSwapChainImpl *implSwapChain;

        TRACE("Colorfill\n");

        /* The color as given in the Blt function is in the format of the frame-buffer...
         * 'clear' expect it in ARGB format => we need to do some conversion :-)
         */
        if (This->resource.format == WINED3DFMT_P8) {
            if (This->palette) {
                color = ((0xFF000000) |
                          (This->palette->palents[DDBltFx->u5.dwFillColor].peRed << 16) |
                          (This->palette->palents[DDBltFx->u5.dwFillColor].peGreen << 8) |
                          (This->palette->palents[DDBltFx->u5.dwFillColor].peBlue));
            } else {
                color = 0xFF000000;
            }
        }
        else if (This->resource.format == WINED3DFMT_R5G6B5) {
            if (DDBltFx->u5.dwFillColor == 0xFFFF) {
                color = 0xFFFFFFFF;
            } else {
                color = ((0xFF000000) |
                          ((DDBltFx->u5.dwFillColor & 0xF800) << 8) |
                          ((DDBltFx->u5.dwFillColor & 0x07E0) << 5) |
                          ((DDBltFx->u5.dwFillColor & 0x001F) << 3));
            }
        }
        else if ((This->resource.format == WINED3DFMT_R8G8B8) ||
                  (This->resource.format == WINED3DFMT_X8R8G8B8) ) {
            color = 0xFF000000 | DDBltFx->u5.dwFillColor;
        }
        else if (This->resource.format == WINED3DFMT_A8R8G8B8) {
            color = DDBltFx->u5.dwFillColor;
        }
        else {
            ERR("Wrong surface type for BLT override(Format doesn't match) !\n");
            return WINED3DERR_INVALIDCALL;
        }

        TRACE("Calling GetSwapChain with mydevice = %p\n", myDevice);
        IWineD3DDevice_GetSwapChain((IWineD3DDevice *)myDevice, 0, (IWineD3DSwapChain **)&implSwapChain);
        IWineD3DSwapChain_Release( (IWineD3DSwapChain *) implSwapChain );
        if(implSwapChain->backBuffer && This == (IWineD3DSurfaceImpl*) implSwapChain->backBuffer[0]) {
            glDrawBuffer(GL_BACK);
            checkGLcall("glDrawBuffer(GL_BACK)");
        }
        else if (This == (IWineD3DSurfaceImpl*) implSwapChain->frontBuffer) {
            glDrawBuffer(GL_FRONT);
            checkGLcall("glDrawBuffer(GL_FRONT)");
        }
        else {
            ERR("Wrong surface type for BLT override(not on swapchain) !\n");
            return WINED3DERR_INVALIDCALL;
        }

        TRACE("(%p) executing Render Target override, color = %lx\n", This, color);

        IWineD3DDevice_Clear( (IWineD3DDevice *) myDevice,
                              1 /* Number of rectangles */,
                              &rect,
                              D3DCLEAR_TARGET,
                              color,
                              0.0 /* Z */,
                              0 /* Stencil */);

        /* Restore the original draw buffer */
        if(implSwapChain->backBuffer && implSwapChain->backBuffer[0]) {
            glDrawBuffer(GL_BACK);
            vcheckGLcall("glDrawBuffer");
        }

        return WINED3D_OK;
    }

    /* Default: Fall back to the generic blt */
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_Blt(IWineD3DSurface *iface, RECT *DestRect, IWineD3DSurface *SrcSurface, RECT *SrcRect, DWORD Flags, DDBLTFX *DDBltFx) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    IWineD3DSurfaceImpl *Src = (IWineD3DSurfaceImpl *) SrcSurface;
    TRACE("(%p)->(%p,%p,%p,%lx,%p)\n", This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);
    TRACE("(%p): Usage is %08lx\n", This, This->resource.usage);

    /* Special cases for RenderTargets */
    if( (This->resource.usage & WINED3DUSAGE_RENDERTARGET) ||
        ( Src && (Src->resource.usage & WINED3DUSAGE_RENDERTARGET) )) {
        if(IWineD3DSurfaceImpl_BltOverride(This, DestRect, SrcSurface, SrcRect, Flags, DDBltFx) == WINED3D_OK) return WINED3D_OK;
    }

    /* For the rest call the X11 surface implementation.
     * For RenderTargets this should be implemented OpenGL accelerated in BltOverride,
     * other Blts are rather rare
     */
    return IWineGDISurfaceImpl_Blt(iface, DestRect, SrcSurface, SrcRect, Flags, DDBltFx);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetBltStatus(IWineD3DSurface *iface, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p)->(%lx)\n", This, Flags);

    switch (Flags)
    {
    case DDGBS_CANBLT:
    case DDGBS_ISBLTDONE:
        return DD_OK;

    default:
        return DDERR_INVALIDPARAMS;
    }
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetFlipStatus(IWineD3DSurface *iface, DWORD Flags) {
    /* XXX: DDERR_INVALIDSURFACETYPE */

    TRACE("(%p)->(%08lx)\n",iface,Flags);
    switch (Flags) {
    case DDGFS_CANFLIP:
    case DDGFS_ISFLIPDONE:
        return DD_OK;

    default:
        return DDERR_INVALIDPARAMS;
    }
}

HRESULT WINAPI IWineD3DSurfaceImpl_IsLost(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    return This->Flags & SFLAG_LOST ? DDERR_SURFACELOST : WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_Restore(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    /* So far we don't lose anything :) */
    This->Flags &= ~SFLAG_LOST;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_BltFast(IWineD3DSurface *iface, DWORD dstx, DWORD dsty, IWineD3DSurface *Source, RECT *rsrc, DWORD trans) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *srcImpl = (IWineD3DSurfaceImpl *) Source;
    TRACE("(%p)->(%ld, %ld, %p, %p, %08lx\n", iface, dstx, dsty, Source, rsrc, trans);

    /* Special cases for RenderTargets */
    if( (This->resource.usage & WINED3DUSAGE_RENDERTARGET) ||
        ( srcImpl && (srcImpl->resource.usage & WINED3DUSAGE_RENDERTARGET) )) {

        RECT SrcRect, DstRect;

        if(rsrc) {
            SrcRect.left = rsrc->left;
            SrcRect.top= rsrc->top;
            SrcRect.bottom = rsrc->bottom;
            SrcRect.right = rsrc->right;
        } else {
            SrcRect.left = 0;
            SrcRect.top = 0;
            SrcRect.right = srcImpl->currentDesc.Width;
            SrcRect.bottom = srcImpl->currentDesc.Height;
        }

        DstRect.left = dstx;
        DstRect.top=dsty;
        DstRect.right = dstx + SrcRect.right - SrcRect.left;
        DstRect.bottom = dsty + SrcRect.bottom - SrcRect.top;

        if(IWineD3DSurfaceImpl_BltOverride(This, &DstRect, Source, &SrcRect, 0, NULL) == WINED3D_OK) return WINED3D_OK;
    }


    return IWineGDISurfaceImpl_BltFast(iface, dstx, dsty, Source, rsrc, trans);
}

HRESULT WINAPI IWineD3DSurfaceImpl_GetPalette(IWineD3DSurface *iface, IWineD3DPalette **Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, Pal);

    *Pal = (IWineD3DPalette *) This->palette;
    return DD_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_RealizePalette(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    RGBQUAD col[256];
    IWineD3DPaletteImpl *pal = This->palette;
    unsigned int n;
    TRACE("(%p)\n", This);

    if(This->resource.format == WINED3DFMT_P8 ||
       This->resource.format == WINED3DFMT_A8P8)
    {
        TRACE("Dirtifying surface\n");
        This->Flags |= SFLAG_DIRTY;
    }

    if(This->Flags & SFLAG_DIBSECTION) {
        TRACE("(%p): Updating the hdc's palette\n", This);
        for (n=0; n<256; n++) {
            if(pal) {
                col[n].rgbRed   = pal->palents[n].peRed;
                col[n].rgbGreen = pal->palents[n].peGreen;
                col[n].rgbBlue  = pal->palents[n].peBlue;
            } else {
                IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
                /* Use the default device palette */
                col[n].rgbRed   = device->palettes[device->currentPalette][n].peRed;
                col[n].rgbGreen = device->palettes[device->currentPalette][n].peGreen;
                col[n].rgbBlue  = device->palettes[device->currentPalette][n].peBlue;
            }
            col[n].rgbReserved = 0;
        }
        SetDIBColorTable(This->hDC, 0, 256, col);
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetPalette(IWineD3DSurface *iface, IWineD3DPalette *Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DPaletteImpl *PalImpl = (IWineD3DPaletteImpl *) Pal;
    TRACE("(%p)->(%p)\n", This, Pal);

    if(This->palette != NULL) 
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
            This->palette->Flags &= ~DDPCAPS_PRIMARYSURFACE;

    if(PalImpl != NULL) {
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            /* Set the device's main palette if the palette
             * wasn't a primary palette before
             */
            if(!(PalImpl->Flags & DDPCAPS_PRIMARYSURFACE)) {
                IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
                unsigned int i;

                for(i=0; i < 256; i++) {
                    device->palettes[device->currentPalette][i] = PalImpl->palents[i];
                }
            }

            (PalImpl)->Flags |= DDPCAPS_PRIMARYSURFACE;
        }
    }
    This->palette = PalImpl;

    return IWineD3DSurface_RealizePalette(iface);
}

HRESULT WINAPI IWineD3DSurfaceImpl_SetColorKey(IWineD3DSurface *iface, DWORD Flags, DDCOLORKEY *CKey) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    BOOL dirtify = FALSE;
    TRACE("(%p)->(%08lx,%p)\n", This, Flags, CKey);

    if ((Flags & DDCKEY_COLORSPACE) != 0) {
        FIXME(" colorkey value not supported (%08lx) !\n", Flags);
        return DDERR_INVALIDPARAMS;
    }

    /* Dirtify the surface, but only if a key was changed */
    if(CKey) {
        switch (Flags & ~DDCKEY_COLORSPACE) {
            case DDCKEY_DESTBLT:
                if(!(This->CKeyFlags & DDSD_CKDESTBLT)) {
                    dirtify = TRUE;
                } else {
                    dirtify = memcmp(&This->DestBltCKey, CKey, sizeof(*CKey) ) != 0;
                }
                This->DestBltCKey = *CKey;
                This->CKeyFlags |= DDSD_CKDESTBLT;
                break;

            case DDCKEY_DESTOVERLAY:
                if(!(This->CKeyFlags & DDSD_CKDESTOVERLAY)) {
                    dirtify = TRUE;
                } else {
                    dirtify = memcmp(&This->DestOverlayCKey, CKey, sizeof(*CKey)) != 0;
                }
                This->DestOverlayCKey = *CKey;
                This->CKeyFlags |= DDSD_CKDESTOVERLAY;
                break;

            case DDCKEY_SRCOVERLAY:
                if(!(This->CKeyFlags & DDSD_CKSRCOVERLAY)) {
                    dirtify = TRUE;
                } else {
                    dirtify = memcmp(&This->SrcOverlayCKey, CKey, sizeof(*CKey)) != 0;
                }
                This->SrcOverlayCKey = *CKey;
                This->CKeyFlags |= DDSD_CKSRCOVERLAY;
                break;

            case DDCKEY_SRCBLT:
                if(!(This->CKeyFlags & DDSD_CKSRCBLT)) {
                    dirtify = TRUE;
                } else {
                    dirtify = memcmp(&This->SrcBltCKey, CKey, sizeof(*CKey)) != 0;
                }
                This->SrcBltCKey = *CKey;
                This->CKeyFlags |= DDSD_CKSRCBLT;
                break;
        }
    }
    else {
        switch (Flags & ~DDCKEY_COLORSPACE) {
            case DDCKEY_DESTBLT:
                dirtify = This->CKeyFlags & DDSD_CKDESTBLT;
                This->CKeyFlags &= ~DDSD_CKDESTBLT;
                break;

            case DDCKEY_DESTOVERLAY:
                dirtify = This->CKeyFlags & DDSD_CKDESTOVERLAY;
                This->CKeyFlags &= ~DDSD_CKDESTOVERLAY;
                break;

            case DDCKEY_SRCOVERLAY:
                dirtify = This->CKeyFlags & DDSD_CKSRCOVERLAY;
                This->CKeyFlags &= ~DDSD_CKSRCOVERLAY;
                break;

            case DDCKEY_SRCBLT:
                dirtify = This->CKeyFlags & DDSD_CKSRCBLT;
                This->CKeyFlags &= ~DDSD_CKSRCBLT;
                break;
        }
    }

    if(dirtify) {
        TRACE("Color key changed, dirtifying surface\n");
        This->Flags |= SFLAG_DIRTY;
    }

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSurfaceImpl_PrivateSetup(IWineD3DSurface *iface) {
    /** Check against the maximum texture sizes supported by the video card **/
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    TRACE("%p\n", This);
    if ((This->pow2Width > GL_LIMITS(texture_size) || This->pow2Height > GL_LIMITS(texture_size)) && !(This->resource.usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL))) {
        /* one of three options
        1: Do the same as we do with nonpow 2 and scale the texture, (any texture ops would require the texture to be scaled which is potentially slow)
        2: Set the texture to the maxium size (bad idea)
        3:    WARN and return WINED3DERR_NOTAVAILABLE;
        4: Create the surface, but allow it to be used only for DirectDraw Blts. Some apps(e.g. Swat 3) create textures with a Height of 16 and a Width > 3000 and blt 16x16 letter areas from them to the render target.
        */
        WARN("(%p) Creating an oversized surface\n", This);
        This->Flags |= SFLAG_OVERSIZE;

        /* This will be initialized on the first blt */
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = 0;
        This->glRect.bottom = 0;
    } else {
        /* No oversize, gl rect is the full texture size */
        This->Flags &= ~SFLAG_OVERSIZE;
        This->glRect.left = 0;
        This->glRect.top = 0;
        This->glRect.right = This->pow2Width;
        This->glRect.bottom = This->pow2Height;
    }

    return WINED3D_OK;
}

DWORD WINAPI IWineD3DSurfaceImpl_GetPitch(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    DWORD ret;
    TRACE("(%p)\n", This);

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
         where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)
          ie pitch = (width/4) * bytes per block                                  */
    if (This->resource.format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        ret = (This->currentDesc.Width >> 2) << 3;
    else if (This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
             This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        ret = (This->currentDesc.Width >> 2) << 4;
    else {
        if (NP2_REPACK == wined3d_settings.nonpower2_mode || This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            /* Front and back buffers are always lockes/unlocked on currentDesc.Width */
            ret = This->bytesPerPixel * This->currentDesc.Width;  /* Bytes / row */
        } else {
            ret = This->bytesPerPixel * This->pow2Width;
        }
    }
    TRACE("(%p) Returning %ld\n", This, ret);
    return ret;
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
    IWineD3DSurfaceImpl_Flip,
    IWineD3DSurfaceImpl_Blt,
    IWineD3DSurfaceImpl_GetBltStatus,
    IWineD3DSurfaceImpl_GetFlipStatus,
    IWineD3DSurfaceImpl_IsLost,
    IWineD3DSurfaceImpl_Restore,
    IWineD3DSurfaceImpl_BltFast,
    IWineD3DSurfaceImpl_GetPalette,
    IWineD3DSurfaceImpl_SetPalette,
    IWineD3DSurfaceImpl_RealizePalette,
    IWineD3DSurfaceImpl_SetColorKey,
    IWineD3DSurfaceImpl_GetPitch,
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
    IWineD3DSurfaceImpl_SetFormat,
    IWineD3DSurfaceImpl_PrivateSetup
};
