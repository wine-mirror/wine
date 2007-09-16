/*
 * IWineD3DSurface Implementation of management(non-rendering) functions
 *
 * Copyright 1998 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2007 Stefan Dösinger for CodeWeavers
 * Copyright 2007 Henri Verbeet
 * Copyright 2006-2007 Roderick Colenbrander
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

/* Do NOT define GLINFO_LOCATION in this file. THIS CODE MUST NOT USE IT */

/* *******************************************
   IWineD3DSurface IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_QueryInterface(IWineD3DSurface *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    /* Warn ,but be nice about things */
    TRACE("(%p)->(%s,%p)\n", This,debugstr_guid(riid),ppobj);

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

ULONG WINAPI IWineD3DBaseSurfaceImpl_AddRef(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This,ref - 1);
    return ref;
}

/* ****************************************************
   IWineD3DSurface IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDevice(IWineD3DSurface *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPrivateData(IWineD3DSurface *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPrivateData(IWineD3DSurface *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_FreePrivateData(IWineD3DSurface *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_SetPriority(IWineD3DSurface *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD   WINAPI IWineD3DBaseSurfaceImpl_GetPriority(IWineD3DSurface *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

WINED3DRESOURCETYPE WINAPI IWineD3DBaseSurfaceImpl_GetType(IWineD3DSurface *iface) {
    TRACE("(%p) : calling resourceimpl_GetType\n", iface);
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetParent(IWineD3DSurface *iface, IUnknown **pParent) {
    TRACE("(%p) : calling resourceimpl_GetParent\n", iface);
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DSurface IWineD3DSurface parts follow
   ****************************************************** */

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetContainer(IWineD3DSurface* iface, REFIID riid, void** ppContainer) {
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

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetDesc(IWineD3DSurface *iface, WINED3DSURFACE_DESC *pDesc) {
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

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetBltStatus(IWineD3DSurface *iface, DWORD Flags) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    TRACE("(%p)->(%x)\n", This, Flags);

    switch (Flags)
    {
        case WINEDDGBS_CANBLT:
        case WINEDDGBS_ISBLTDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetFlipStatus(IWineD3DSurface *iface, DWORD Flags) {
    /* XXX: DDERR_INVALIDSURFACETYPE */

    TRACE("(%p)->(%08x)\n",iface,Flags);
    switch (Flags) {
        case WINEDDGFS_CANFLIP:
        case WINEDDGFS_ISFLIPDONE:
            return WINED3D_OK;

        default:
            return WINED3DERR_INVALIDCALL;
    }
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_IsLost(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    /* D3D8 and 9 loose full devices, ddraw only surfaces */
    return This->Flags & SFLAG_LOST ? WINED3DERR_DEVICELOST : WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_Restore(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)\n", This);

    /* So far we don't lose anything :) */
    This->Flags &= ~SFLAG_LOST;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetPalette(IWineD3DSurface *iface, IWineD3DPalette *Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DPaletteImpl *PalImpl = (IWineD3DPaletteImpl *) Pal;
    TRACE("(%p)->(%p)\n", This, Pal);

    if(This->palette != NULL)
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET)
            This->palette->Flags &= ~WINEDDPCAPS_PRIMARYSURFACE;

    if(PalImpl != NULL) {
        if(This->resource.usage & WINED3DUSAGE_RENDERTARGET) {
            /* Set the device's main palette if the palette
            * wasn't a primary palette before
            */
            if(!(PalImpl->Flags & WINEDDPCAPS_PRIMARYSURFACE)) {
                IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
                unsigned int i;

                for(i=0; i < 256; i++) {
                    device->palettes[device->currentPalette][i] = PalImpl->palents[i];
                }
            }

            (PalImpl)->Flags |= WINEDDPCAPS_PRIMARYSURFACE;
        }
    }
    This->palette = PalImpl;

    return IWineD3DSurface_RealizePalette(iface);
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetColorKey(IWineD3DSurface *iface, DWORD Flags, WINEDDCOLORKEY *CKey) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%08x,%p)\n", This, Flags, CKey);

    if ((Flags & WINEDDCKEY_COLORSPACE) != 0) {
        FIXME(" colorkey value not supported (%08x) !\n", Flags);
        return WINED3DERR_INVALIDCALL;
    }

    /* Dirtify the surface, but only if a key was changed */
    if(CKey) {
        switch (Flags & ~WINEDDCKEY_COLORSPACE) {
            case WINEDDCKEY_DESTBLT:
                This->DestBltCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                This->DestOverlayCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                This->SrcOverlayCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                This->SrcBltCKey = *CKey;
                This->CKeyFlags |= WINEDDSD_CKSRCBLT;
                break;
        }
    }
    else {
        switch (Flags & ~WINEDDCKEY_COLORSPACE) {
            case WINEDDCKEY_DESTBLT:
                This->CKeyFlags &= ~WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                This->CKeyFlags &= ~WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                This->CKeyFlags &= ~WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                This->CKeyFlags &= ~WINEDDSD_CKSRCBLT;
                break;
        }
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetPalette(IWineD3DSurface *iface, IWineD3DPalette **Pal) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, Pal);

    *Pal = (IWineD3DPalette *) This->palette;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_RealizePalette(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    RGBQUAD col[256];
    IWineD3DPaletteImpl *pal = This->palette;
    unsigned int n;
    TRACE("(%p)\n", This);

    if(This->resource.format == WINED3DFMT_P8 ||
       This->resource.format == WINED3DFMT_A8P8)
    {
        if(!This->Flags & SFLAG_INSYSMEM) {
            FIXME("Palette changed with surface that does not have an up to date system memory copy\n");
        }
        TRACE("Dirtifying surface\n");
        This->Flags &= ~(SFLAG_INTEXTURE | SFLAG_INDRAWABLE);
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

DWORD WINAPI IWineD3DBaseSurfaceImpl_GetPitch(IWineD3DSurface *iface) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    DWORD ret;
    TRACE("(%p)\n", This);

    /* DXTn formats don't have exact pitches as they are to the new row of blocks,
    where each block is 4x4 pixels, 8 bytes (dxt1) and 16 bytes (dxt2/3/4/5)
    ie pitch = (width/4) * bytes per block                                  */
    if (This->resource.format == WINED3DFMT_DXT1) /* DXT1 is 8 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 3;
    else if (This->resource.format == WINED3DFMT_DXT2 || This->resource.format == WINED3DFMT_DXT3 ||
             This->resource.format == WINED3DFMT_DXT4 || This->resource.format == WINED3DFMT_DXT5) /* DXT2/3/4/5 is 16 bytes per block */
        ret = ((This->currentDesc.Width + 3) >> 2) << 4;
    else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        ret = This->bytesPerPixel * This->currentDesc.Width;  /* Bytes / row */
        ret = (ret + alignment - 1) & ~(alignment - 1);
    }
    TRACE("(%p) Returning %d\n", This, ret);
    return ret;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetOverlayPosition(IWineD3DSurface *iface, LONG X, LONG Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    FIXME("(%p)->(%d,%d) Stub!\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetOverlayPosition(IWineD3DSurface *iface, LONG *X, LONG *Y) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;

    FIXME("(%p)->(%p,%p) Stub!\n", This, X, Y);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlayZOrder(IWineD3DSurface *iface, DWORD Flags, IWineD3DSurface *Ref) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *RefImpl = (IWineD3DSurfaceImpl *) Ref;

    FIXME("(%p)->(%08x,%p) Stub!\n", This, Flags, RefImpl);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_UpdateOverlay(IWineD3DSurface *iface, RECT *SrcRect, IWineD3DSurface *DstSurface, RECT *DstRect, DWORD Flags, WINEDDOVERLAYFX *FX) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    IWineD3DSurfaceImpl *Dst = (IWineD3DSurfaceImpl *) DstSurface;
    FIXME("(%p)->(%p, %p, %p, %08x, %p)\n", This, SrcRect, Dst, DstRect, Flags, FX);

    if(!(This->resource.usage & WINED3DUSAGE_OVERLAY))
    {
        TRACE("(%p): Not an overlay surface\n", This);
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetClipper(IWineD3DSurface *iface, IWineD3DClipper *clipper)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, clipper);

    This->clipper = clipper;
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_GetClipper(IWineD3DSurface *iface, IWineD3DClipper **clipper)
{
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *) iface;
    TRACE("(%p)->(%p)\n", This, clipper);

    *clipper = This->clipper;
    if(*clipper) {
        IWineD3DClipper_AddRef(*clipper);
    }
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetContainer(IWineD3DSurface *iface, IWineD3DBase *container) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;

    TRACE("This %p, container %p\n", This, container);

    /* We can't keep a reference to the container, since the container already keeps a reference to us. */

    TRACE("Setting container to %p from %p\n", container, This->container);
    This->container = container;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSurfaceImpl_SetFormat(IWineD3DSurface *iface, WINED3DFORMAT format) {
    IWineD3DSurfaceImpl *This = (IWineD3DSurfaceImpl *)iface;
    const StaticPixelFormatDesc *formatEntry = getFormatDescEntry(format, NULL, NULL);

    if (This->resource.format != WINED3DFMT_UNKNOWN) {
        FIXME("(%p) : The format of the surface must be WINED3DFORMAT_UNKNOWN\n", This);
        return WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting texture format to (%d,%s)\n", This, format, debug_d3dformat(format));
    if (format == WINED3DFMT_UNKNOWN) {
        This->resource.size = 0;
    } else if (format == WINED3DFMT_DXT1) {
        /* DXT1 is half byte per pixel */
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4)) >> 1;

    } else if (format == WINED3DFMT_DXT2 || format == WINED3DFMT_DXT3 ||
               format == WINED3DFMT_DXT4 || format == WINED3DFMT_DXT5) {
        This->resource.size = ((max(This->pow2Width, 4) * formatEntry->bpp) * max(This->pow2Height, 4));
    } else {
        unsigned char alignment = This->resource.wineD3DDevice->surface_alignment;
        This->resource.size = ((This->pow2Width * formatEntry->bpp) + alignment - 1) & ~(alignment - 1);
        This->resource.size *= This->pow2Height;
    }

    if (format != WINED3DFMT_UNKNOWN) {
        This->bytesPerPixel = formatEntry->bpp;
    } else {
        This->bytesPerPixel = 0;
    }

    This->Flags |= (WINED3DFMT_D16_LOCKABLE == format) ? SFLAG_LOCKABLE : 0;

    This->resource.format = format;

    TRACE("(%p) : Size %d, bytesPerPixel %d\n", This, This->resource.size, This->bytesPerPixel);

    return WINED3D_OK;
}
