/*
 * IDirect3DTexture9 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DTexture9 IUnknown parts follow: */
HRESULT WINAPI IDirect3DTexture9Impl_QueryInterface(LPDIRECT3DTEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
        || IsEqualGUID(riid, &IID_IDirect3DTexture9)) {
        IDirect3DTexture9Impl_AddRef(iface);
        *ppobj = This;
        return D3D_OK;
    }

    WARN("(%p)->(%s,%p) not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

ULONG WINAPI IDirect3DTexture9Impl_AddRef(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    TRACE("(%p) : AddRef from %ld\n", This, This->ref);
    return ++(This->ref);
}

ULONG WINAPI IDirect3DTexture9Impl_Release(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    ULONG ref = --This->ref;
    int i;

    TRACE("(%p) : ReleaseRef to %ld\n", This, This->ref);
    if (ref == 0) {
        for (i = 0; i < This->levels; i++) {
            if (NULL != This->surfaces[i]) {
                TRACE("(%p) : Releasing surface %p\n", This, This->surfaces[i]);
                IDirect3DSurface9Impl_Release((LPDIRECT3DSURFACE9) This->surfaces[i]);
            }
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DTexture9 IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DTexture9Impl_GetDevice(LPDIRECT3DTEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

HRESULT WINAPI IDirect3DTexture9Impl_SetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DTexture9Impl_GetPrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DTexture9Impl_FreePrivateData(LPDIRECT3DTEXTURE9 iface, REFGUID refguid) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

DWORD WINAPI IDirect3DTexture9Impl_SetPriority(LPDIRECT3DTEXTURE9 iface, DWORD PriorityNew) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DResource9Impl_SetPriority((LPDIRECT3DRESOURCE9) This, PriorityNew);
}

DWORD WINAPI IDirect3DTexture9Impl_GetPriority(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DResource9Impl_GetPriority((LPDIRECT3DRESOURCE9) This);
}

void WINAPI IDirect3DTexture9Impl_PreLoad(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return ;
}

D3DRESOURCETYPE WINAPI IDirect3DTexture9Impl_GetType(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DResource9Impl_GetType((LPDIRECT3DRESOURCE9) This);
}

/* IDirect3DTexture9 IDirect3DBaseTexture9 Interface follow: */
DWORD WINAPI IDirect3DTexture9Impl_SetLOD(LPDIRECT3DTEXTURE9 iface, DWORD LODNew) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetLOD((LPDIRECT3DBASETEXTURE9) This, LODNew);
}

DWORD WINAPI IDirect3DTexture9Impl_GetLOD(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLOD((LPDIRECT3DBASETEXTURE9) This);
}

DWORD WINAPI IDirect3DTexture9Impl_GetLevelCount(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetLevelCount((LPDIRECT3DBASETEXTURE9) This);
}

HRESULT WINAPI IDirect3DTexture9Impl_SetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_SetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This, FilterType);
}

D3DTEXTUREFILTERTYPE WINAPI IDirect3DTexture9Impl_GetAutoGenFilterType(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    return IDirect3DBaseTexture9Impl_GetAutoGenFilterType((LPDIRECT3DBASETEXTURE9) This);
}

void WINAPI IDirect3DTexture9Impl_GenerateMipSubLevels(LPDIRECT3DTEXTURE9 iface) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    return ;
}

/* IDirect3DTexture9 Interface follow: */
HRESULT WINAPI IDirect3DTexture9Impl_GetLevelDesc(LPDIRECT3DTEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);

    if (Level < This->levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IDirect3DSurface9Impl_GetDesc((LPDIRECT3DSURFACE9) This->surfaces[Level], pDesc);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
	return D3DERR_INVALIDCALL;
    }
    return D3D_OK;
}

HRESULT WINAPI IDirect3DTexture9Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE9 iface, UINT Level, IDirect3DSurface9** ppSurfaceLevel) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    *ppSurfaceLevel = (LPDIRECT3DSURFACE9) This->surfaces[Level];
    IDirect3DSurface9Impl_AddRef((LPDIRECT3DSURFACE9) This->surfaces[Level]);
    TRACE("(%p) : returning %p for level %d\n", This, *ppSurfaceLevel, Level);
    return D3D_OK;
}

HRESULT WINAPI IDirect3DTexture9Impl_LockRect(LPDIRECT3DTEXTURE9 iface, UINT Level,D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr;
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    if (Level < This->levels) {
        /**
	 * Not dirtified while Surfaces don't notify dirtification
	 * This->impl.parent.Dirty = TRUE;
	 */
        hr = IDirect3DSurface9Impl_LockRect((LPDIRECT3DSURFACE9) This->surfaces[Level], pLockedRect, pRect, Flags);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
	return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IDirect3DTexture9Impl_UnlockRect(LPDIRECT3DTEXTURE9 iface, UINT Level) {
    HRESULT hr;
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    if (Level < This->levels) {
        hr = IDirect3DSurface9Impl_UnlockRect((LPDIRECT3DSURFACE9) This->surfaces[Level]);
        TRACE("(%p) Level (%d) success(%lu)\n", This, Level, hr);
    } else {
        FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->levels);
	return D3DERR_INVALIDCALL;
    }
    return hr;
}

HRESULT WINAPI IDirect3DTexture9Impl_AddDirtyRect(LPDIRECT3DTEXTURE9 iface, CONST RECT* pDirtyRect) {
    ICOM_THIS(IDirect3DTexture9Impl,iface);
    FIXME("(%p) : stub\n", This);
    This->Dirty = TRUE;
    return D3D_OK;
}


ICOM_VTABLE(IDirect3DTexture9) Direct3DTexture9_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    IDirect3DTexture9Impl_QueryInterface,
    IDirect3DTexture9Impl_AddRef,
    IDirect3DTexture9Impl_Release,
    IDirect3DTexture9Impl_GetDevice,
    IDirect3DTexture9Impl_SetPrivateData,
    IDirect3DTexture9Impl_GetPrivateData,
    IDirect3DTexture9Impl_FreePrivateData,
    IDirect3DTexture9Impl_SetPriority,
    IDirect3DTexture9Impl_GetPriority,
    IDirect3DTexture9Impl_PreLoad,
    IDirect3DTexture9Impl_GetType,
    IDirect3DTexture9Impl_SetLOD,
    IDirect3DTexture9Impl_GetLOD,
    IDirect3DTexture9Impl_GetLevelCount,
    IDirect3DTexture9Impl_SetAutoGenFilterType,
    IDirect3DTexture9Impl_GetAutoGenFilterType,
    IDirect3DTexture9Impl_GenerateMipSubLevels,
    IDirect3DTexture9Impl_GetLevelDesc,
    IDirect3DTexture9Impl_GetSurfaceLevel,
    IDirect3DTexture9Impl_LockRect,
    IDirect3DTexture9Impl_UnlockRect,
    IDirect3DTexture9Impl_AddDirtyRect
};


/* IDirect3DDevice9 IDirect3DTexture9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) {
    ICOM_THIS(IDirect3DDevice9Impl,iface);
    FIXME("(%p) : stub\n", This);    
    return D3D_OK;
}
