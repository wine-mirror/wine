/*
 * IDirect3DTexture8 implementation
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

#define D3D8_TEXTURE(a) ((IWineD3DTextureImpl*)(a->wineD3DTexture))
#define D3D8_TEXTURE_GET_SURFACE(a) ((IWineD3DSurfaceImpl*)(D3D8_TEXTURE(a)->surfaces[i]))
#define D3D8_BASETEXTURE(a) (((IWineD3DTextureImpl*)(a->wineD3DTexture))->baseTexture)

/* IDirect3DTexture8 IUnknown parts follow: */
HRESULT WINAPI IDirect3DTexture8Impl_QueryInterface(LPDIRECT3DTEXTURE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
        || IsEqualGUID(riid, &IID_IDirect3DTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p) not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DTexture8Impl_AddRef(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %ld\n", This, ref - 1);

    return ref;
}

ULONG WINAPI IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %ld\n", This, ref);

    if (ref == 0) {
        IWineD3DTexture_Release(This->wineD3DTexture);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DTexture8 IDirect3DResource8 Interface follow: */
HRESULT WINAPI IDirect3DTexture8Impl_GetDevice(LPDIRECT3DTEXTURE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

HRESULT WINAPI IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetPrivateData(This->wineD3DTexture, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetPrivateData(This->wineD3DTexture, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_FreePrivateData(This->wineD3DTexture, refguid);
}

DWORD WINAPI IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetPriority(This->wineD3DTexture, PriorityNew);
}

DWORD WINAPI IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetPriority(This->wineD3DTexture);
}

void     WINAPI        IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface) {
    unsigned int i;
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) : About to load texture\n", This);

    ENTER_GL();

    for (i = 0; i < D3D8_BASETEXTURE(This).levels; i++) {
      if (i == 0 && D3D8_TEXTURE_GET_SURFACE(This)->textureName != 0 && D3D8_BASETEXTURE(This).dirty == FALSE) {
	glBindTexture(GL_TEXTURE_2D, D3D8_TEXTURE_GET_SURFACE(This)->textureName);
	checkGLcall("glBindTexture");
	TRACE("Texture %p (level %d) given name %d\n", D3D8_TEXTURE_GET_SURFACE(This), i, D3D8_TEXTURE_GET_SURFACE(This)->textureName);
	/* No need to walk through all mip-map levels, since already all assigned */
        i = D3D8_BASETEXTURE(This).levels;

      } else {
	if (i == 0) {
	  if (D3D8_TEXTURE_GET_SURFACE(This)->textureName == 0) {
	    glGenTextures(1, &(D3D8_TEXTURE_GET_SURFACE(This)->textureName));
	    checkGLcall("glGenTextures");
	    TRACE("Texture %p (level %d) given name %d\n", D3D8_TEXTURE_GET_SURFACE(This), i, D3D8_TEXTURE_GET_SURFACE(This)->textureName);
	  }
	  
	  glBindTexture(GL_TEXTURE_2D, D3D8_TEXTURE_GET_SURFACE(This)->textureName);
	  checkGLcall("glBindTexture");
	}
	IWineD3DSurface_LoadTexture((IWineD3DSurface*)D3D8_TEXTURE_GET_SURFACE(This));
/*	IDirect3DSurface8Impl_LoadTexture((LPDIRECT3DSURFACE8) D3D8_TEXTURE_GET_SURFACE(This), GL_TEXTURE_2D, i); */
      }
    }

    /* No longer dirty */
    D3D8_BASETEXTURE(This).dirty = FALSE;

    /* Always need to reset the number of mipmap levels when rebinding as it is
       a property of the active texture unit, and another texture may have set it
       to a different value                                                       */
    TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", D3D8_BASETEXTURE(This).levels - 1);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, D3D8_BASETEXTURE(This).levels - 1);
    checkGLcall("glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, This->levels)");

    LEAVE_GL();

    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetType(This->wineD3DTexture);
}

/* IDirect3DTexture8 IDirect3DBaseTexture8 Interface follow: */
DWORD WINAPI IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetLOD(This->wineD3DTexture, LODNew);
}

DWORD WINAPI IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetLOD(This->wineD3DTexture);
}

DWORD WINAPI IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetLevelCount(This->wineD3DTexture);
}

/* IDirect3DTexture8 Interface follow: */
HRESULT WINAPI IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DSURFACE_DESC *pDesc) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    WINED3DSURFACE_DESC    wined3ddesc;
    UINT                   tmpInt = -1;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d8 structures differ, pass in ptrs to where data needs to go */
    memset(&wined3ddesc, 0, sizeof(wined3ddesc));
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = &pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt; /* required for d3d8 */
    wined3ddesc.MultiSampleType     = &pDesc->MultiSampleType;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DTexture_GetLevelDesc(This->wineD3DTexture, Level, &wined3ddesc);
}

HRESULT WINAPI IDirect3DTexture8Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE8 iface, UINT Level, IDirect3DSurface8 **ppSurfaceLevel) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);
    hrc = IWineD3DTexture_GetSurfaceLevel(This->wineD3DTexture, Level, &mySurface);
    if (hrc == D3D_OK && NULL != ppSurfaceLevel) {
       IWineD3DSurface_GetParent(mySurface, (IUnknown **)ppSurfaceLevel);
       IWineD3DSurface_Release(mySurface);
    }
    return hrc;
}

HRESULT WINAPI IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_LockRect(This->wineD3DTexture, Level, pLockedRect, pRect, Flags);
}

HRESULT WINAPI IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_UnlockRect(This->wineD3DTexture, Level);
}

HRESULT WINAPI IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT *pDirtyRect) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_AddDirtyRect(This->wineD3DTexture, pDirtyRect);
}

const IDirect3DTexture8Vtbl Direct3DTexture8_Vtbl =
{
    /* IUnknown */
    IDirect3DTexture8Impl_QueryInterface,
    IDirect3DTexture8Impl_AddRef,
    IDirect3DTexture8Impl_Release,
     /* IDirect3DResource8 */
    IDirect3DTexture8Impl_GetDevice,
    IDirect3DTexture8Impl_SetPrivateData,
    IDirect3DTexture8Impl_GetPrivateData,
    IDirect3DTexture8Impl_FreePrivateData,
    IDirect3DTexture8Impl_SetPriority,
    IDirect3DTexture8Impl_GetPriority,
    IDirect3DTexture8Impl_PreLoad,
    IDirect3DTexture8Impl_GetType,
    /* IDirect3dBaseTexture8 */
    IDirect3DTexture8Impl_SetLOD,
    IDirect3DTexture8Impl_GetLOD,
    IDirect3DTexture8Impl_GetLevelCount,
    /* IDirect3DTexture8 */
    IDirect3DTexture8Impl_GetLevelDesc,
    IDirect3DTexture8Impl_GetSurfaceLevel,
    IDirect3DTexture8Impl_LockRect,
    IDirect3DTexture8Impl_UnlockRect,
    IDirect3DTexture8Impl_AddDirtyRect
};
