/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DBaseTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DBaseTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    /* FIXME: This needs to extend an IWineD3DBaseObject */
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DBaseTextureImpl_AddRef(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);

    TRACE("(%p) : AddRef increasing from %ld\n", This,ref - 1);
    IUnknown_AddRef(This->resource.parent);
    return ref;
}

ULONG WINAPI IWineD3DBaseTextureImpl_Release(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %ld\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DBaseTextureImpl_CleanUp(iface);
        HeapFree(GetProcessHeap(), 0, This);
    } else {
        IUnknown_Release(This->resource.parent);  /* Released the reference to the d3dx object */
    }
    return ref;
}

/* class static */
void IWineD3DBaseTextureImpl_CleanUp(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : textureName(%d)\n", This, This->baseTexture.textureName);
    if (This->baseTexture.textureName != 0) {
        ENTER_GL();
        TRACE("(%p) : Deleting texture %d\n", This, This->baseTexture.textureName);
        glDeleteTextures(1, &This->baseTexture.textureName);
        LEAVE_GL();
    }
    IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
}

/* ****************************************************
   IWineD3DBaseTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DBaseTextureImpl_GetDevice(IWineD3DBaseTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_GetPriority(IWineD3DBaseTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void     WINAPI        IWineD3DBaseTextureImpl_PreLoad(IWineD3DBaseTexture *iface) {
    return IWineD3DResourceImpl_PreLoad((IWineD3DResource *)iface);
}

D3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetParent(IWineD3DBaseTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DBaseTexture IWineD3DBaseTexture parts follow
   ****************************************************** */

/* There is no OpenGL equivilent of setLOD, getLOD, all they do it priortise testure loading
 * so just pretend that they work unless something really needs a failure. */
DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    if (This->resource.pool != D3DPOOL_MANAGED) {
        return  D3DERR_INVALIDCALL;
    }

    if(LODNew >= This->baseTexture.levels)
        LODNew = This->baseTexture.levels - 1;
     This->baseTexture.LOD = LODNew;

    TRACE("(%p) : set bogus LOD to %d \n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    if (This->resource.pool != D3DPOOL_MANAGED) {
        return  D3DERR_INVALIDCALL;
    }

    TRACE("(%p) : returning %d \n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->baseTexture.levels);
    return This->baseTexture.levels;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, D3DTEXTUREFILTERTYPE FilterType) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

  if (!(This->baseTexture.usage & D3DUSAGE_AUTOGENMIPMAP)) {
      TRACE("(%p) : returning invalid call\n", This);
      return D3DERR_INVALIDCALL;
  }
  This->baseTexture.filterType = FilterType;
  TRACE("(%p) : \n", This);
  return D3D_OK;
}

D3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  if (!(This->baseTexture.usage & D3DUSAGE_AUTOGENMIPMAP)) {
     return D3DTEXF_NONE;
  }
  return This->baseTexture.filterType;
  return D3DTEXF_LINEAR; /* default */
}

void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  /* TODO: implement filters using GL_SGI_generate_mipmaps http://oss.sgi.com/projects/ogl-sample/registry/SGIS/generate_mipmap.txt */
  FIXME("(%p) : stub\n", This);
  return ;
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL dirty) {
    BOOL old;
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    old = This->baseTexture.dirty;
    This->baseTexture.dirty = dirty;
    return old;
}

BOOL WINAPI IWineD3DBaseTextureImpl_GetDirty(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    return This->baseTexture.dirty;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_BindTexture(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    HRESULT hr = D3D_OK;
    UINT textureDimensions;
    TRACE("(%p) : About to bind texture\n", This);

    textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    ENTER_GL();
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PushState(This->contextManager, textureDimensions, ENABLED, NOW /* make sure the state is applied now */);
#else
    glEnable(textureDimensions);
#endif

    /* Generate a texture name if we don't already have one */
    if (This->baseTexture.textureName == 0) {
        glGenTextures(1, &This->baseTexture.textureName);
        checkGLcall("glGenTextures");
        TRACE("Generated texture %d\n", This->baseTexture.textureName);
         if (This->resource.pool == D3DPOOL_DEFAULT) {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            glPrioritizeTextures(1, &This->baseTexture.textureName, &tmp);
         }
        IWineD3DBaseTexture_SetDirty(iface, TRUE);
    }

    /* Bind the texture */
    if (This->baseTexture.textureName != 0) {
        glBindTexture(textureDimensions, This->baseTexture.textureName);
        checkGLcall("glBindTexture");
    } else { /* this only happened if we've run out of openGL textures */
        WARN("This texture doesn't have an openGL texture assigned to it\n");
        hr =  D3DERR_INVALIDCALL;
    }

    if (hr == D3D_OK) {
        /* Always need to reset the number of mipmap levels when rebinding as it is
        a property of the active texture unit, and another texture may have set it
        to a different value                                                       */
        TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->baseTexture.levels - 1);
        glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels - 1);
        checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels)");
    }
    LEAVE_GL();
    return hr;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_UnBindTexture(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    UINT textureDimensions;

    TRACE("(%p) : About to bind texture\n", This);
    textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);

    ENTER_GL();

    glBindTexture(textureDimensions, 0);
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PopState(This->contextManager, textureDimensions, ENABLED, NOW /* make sure the state is applied now */);
#else
    glDisable(textureDimensions);
#endif

    LEAVE_GL();
    return D3D_OK;
}

UINT WINAPI IWineD3DBaseTextureImpl_GetTextureDimensions(IWineD3DBaseTexture *iface){
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    FIXME("(%p) : This shouldn't be called\n", This);
    return D3D_OK;
}

static const IWineD3DBaseTextureVtbl IWineD3DBaseTexture_Vtbl =
{
    IWineD3DBaseTextureImpl_QueryInterface,
    IWineD3DBaseTextureImpl_AddRef,
    IWineD3DBaseTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DBaseTextureImpl_GetParent,
    IWineD3DBaseTextureImpl_GetDevice,
    IWineD3DBaseTextureImpl_SetPrivateData,
    IWineD3DBaseTextureImpl_GetPrivateData,
    IWineD3DBaseTextureImpl_FreePrivateData,
    IWineD3DBaseTextureImpl_SetPriority,
    IWineD3DBaseTextureImpl_GetPriority,
    IWineD3DBaseTextureImpl_PreLoad,
    IWineD3DBaseTextureImpl_GetType,
    /*IWineD3DBaseTexture*/
    IWineD3DBaseTextureImpl_SetLOD,
    IWineD3DBaseTextureImpl_GetLOD,
    IWineD3DBaseTextureImpl_GetLevelCount,
    IWineD3DBaseTextureImpl_SetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GenerateMipSubLevels,
    IWineD3DBaseTextureImpl_SetDirty,
    IWineD3DBaseTextureImpl_GetDirty,
    /* internal */
    IWineD3DBaseTextureImpl_BindTexture,
    IWineD3DBaseTextureImpl_UnBindTexture,
    IWineD3DBaseTextureImpl_GetTextureDimensions

};
