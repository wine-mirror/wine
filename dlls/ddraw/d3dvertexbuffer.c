/* Direct3D Viewport
 * Copyright (c) 2002 Lionel ULMER
 *
 * This file contains the implementation of Direct3DVertexBuffer COM object
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
#include "winerror.h"
#include "objbase.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"
#include "mesa_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_QueryInterface(LPDIRECT3DVERTEXBUFFER7 iface,
                                                   REFIID riid,
                                                   LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    /* By default, set the object pointer to NULL */
    *obp = NULL;
      
    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DVertexBuffer7_AddRef(ICOM_INTERFACE(This,IDirect3DVertexBuffer7));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer, riid ) ) {
        IDirect3DVertexBuffer7_AddRef(ICOM_INTERFACE(This,IDirect3DVertexBuffer7));
        *obp = ICOM_INTERFACE(This, IDirect3DVertexBuffer);
	TRACE("  Creating IDirect3DVertexBuffer interface %p\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer7, riid ) ) {
        IDirect3DVertexBuffer7_AddRef(ICOM_INTERFACE(This,IDirect3DVertexBuffer7));
        *obp = ICOM_INTERFACE(This, IDirect3DVertexBuffer7);
	TRACE("  Creating IDirect3DVertexBuffer7 interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_AddRef(LPDIRECT3DVERTEXBUFFER7 iface)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, This->ref);
    return ++(This->ref);
}

ULONG WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_Release(LPDIRECT3DVERTEXBUFFER7 iface)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, This->ref);
    if (--(This->ref) == 0) {
        HeapFree(GetProcessHeap(), 0, This->vertices);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return This->ref;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_Lock(LPDIRECT3DVERTEXBUFFER7 iface,
                                         DWORD dwFlags,
                                         LPVOID* lplpData,
                                         LPDWORD lpdwSize)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->(%08lx,%p,%p)\n", This, iface, dwFlags, lplpData, lpdwSize);

    if (TRACE_ON(ddraw)) {
        TRACE(" lock flags : ");
	DDRAW_dump_lockflag(dwFlags);
    }
    
    if (lpdwSize != NULL) *lpdwSize = This->vertex_buffer_size;
    *lplpData = This->vertices;
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_Unlock(LPDIRECT3DVERTEXBUFFER7 iface)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->()\n", This, iface);
    /* Nothing to do here for now. Maybe some optimizations if ever we want to do some :-) */
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_ProcessVertices(LPDIRECT3DVERTEXBUFFER7 iface,
                                                 DWORD dwVertexOp,
                                                 DWORD dwDestIndex,
                                                 DWORD dwCount,
                                                 LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer,
                                                 DWORD dwSrcIndex,
                                                 LPDIRECT3DDEVICE7 lpD3DDevice,
                                                 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    FIXME("(%p/%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx): stub!\n", This, iface, dwVertexOp, dwDestIndex, dwCount, lpSrcBuffer, dwSrcIndex, lpD3DDevice, dwFlags);    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_GetVertexBufferDesc(LPDIRECT3DVERTEXBUFFER7 iface,
                                                        LPD3DVERTEXBUFFERDESC lpD3DVertexBufferDesc)
{
    DWORD size;
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    TRACE("(%p/%p)->(%p)\n", This, iface, lpD3DVertexBufferDesc);
    size = (lpD3DVertexBufferDesc->dwSize < This->desc.dwSize) ? lpD3DVertexBufferDesc->dwSize : This->desc.dwSize;
    memcpy(lpD3DVertexBufferDesc,&This->desc,size);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_Optimize(LPDIRECT3DVERTEXBUFFER7 iface,
                                          LPDIRECT3DDEVICE7 lpD3DDevice,
                                          DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    FIXME("(%p/%p)->(%p,%08lx): stub!\n", This, iface, lpD3DDevice, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_ProcessVerticesStrided(LPDIRECT3DVERTEXBUFFER7 iface,
                                                        DWORD dwVertexOp,
                                                        DWORD dwDestIndex,
                                                        DWORD dwCount,
                                                        LPD3DDRAWPRIMITIVESTRIDEDDATA lpStrideData,
                                                        DWORD dwVertexTypeDesc,
                                                        LPDIRECT3DDEVICE7 lpD3DDevice,
                                                        DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    FIXME("(%p/%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx): stub!\n", This, iface, dwVertexOp, dwDestIndex, dwCount, lpStrideData, dwVertexTypeDesc, lpD3DDevice, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_1_ProcessVertices(LPDIRECT3DVERTEXBUFFER iface,
                                                 DWORD dwVertexOp,
                                                 DWORD dwDestIndex,
                                                 DWORD dwCount,
                                                 LPDIRECT3DVERTEXBUFFER lpSrcBuffer,
                                                 DWORD dwSrcIndex,
                                                 LPDIRECT3DDEVICE3 lpD3DDevice,
                                                 DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, iface);
    FIXME("(%p/%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx): stub!\n", This, iface, dwVertexOp, dwDestIndex, dwCount, lpSrcBuffer, dwSrcIndex, lpD3DDevice, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_1_Optimize(LPDIRECT3DVERTEXBUFFER iface,
                                          LPDIRECT3DDEVICE3 lpD3DDevice,
                                          DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, iface);
    FIXME("(%p/%p)->(%p,%08lx): stub!\n", This, iface, lpD3DDevice, dwFlags);
    return DD_OK;
}

HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_QueryInterface(LPDIRECT3DVERTEXBUFFER iface,
                                                 REFIID riid,
                                                 LPVOID* obp)
{
    TRACE("(%p)->(%s,%p) thunking to IDirect3DVertexBuffer7 interface.\n", iface, debugstr_guid(riid), obp);
    return IDirect3DVertexBuffer7_QueryInterface(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface),
                                                 riid,
                                                 obp);
}

ULONG WINAPI
Thunk_IDirect3DVertexBufferImpl_1_AddRef(LPDIRECT3DVERTEXBUFFER iface)
{
    TRACE("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", iface);
    return IDirect3DVertexBuffer7_AddRef(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface));
}

ULONG WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Release(LPDIRECT3DVERTEXBUFFER iface)
{
    TRACE("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", iface);
    return IDirect3DVertexBuffer7_Release(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Lock(LPDIRECT3DVERTEXBUFFER iface,
                                       DWORD dwFlags,
                                       LPVOID* lplpData,
                                       LPDWORD lpdwSize)
{
    TRACE("(%p)->(%08lx,%p,%p) thunking to IDirect3DVertexBuffer7 interface.\n", iface, dwFlags, lplpData, lpdwSize);
    return IDirect3DVertexBuffer7_Lock(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface),
                                       dwFlags,
                                       lplpData,
                                       lpdwSize);
}

HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Unlock(LPDIRECT3DVERTEXBUFFER iface)
{
    TRACE("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", iface);
    return IDirect3DVertexBuffer7_Unlock(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface));
}

HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_GetVertexBufferDesc(LPDIRECT3DVERTEXBUFFER iface,
                                                      LPD3DVERTEXBUFFERDESC lpD3DVertexBufferDesc)
{
    TRACE("(%p)->(%p) thunking to IDirect3DVertexBuffer7 interface.\n", iface, lpD3DVertexBufferDesc);
    return IDirect3DVertexBuffer7_GetVertexBufferDesc(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface),
                                                      lpD3DVertexBufferDesc);
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DVertexBuffer7.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DVertexBuffer7) VTABLE_IDirect3DVertexBuffer7 =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Main_IDirect3DVertexBufferImpl_7_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DVertexBufferImpl_7_1T_AddRef,
    XCAST(Release) Main_IDirect3DVertexBufferImpl_7_1T_Release,
    XCAST(Lock) Main_IDirect3DVertexBufferImpl_7_1T_Lock,
    XCAST(Unlock) Main_IDirect3DVertexBufferImpl_7_1T_Unlock,
    XCAST(ProcessVertices) Main_IDirect3DVertexBufferImpl_7_ProcessVertices,
    XCAST(GetVertexBufferDesc) Main_IDirect3DVertexBufferImpl_7_1T_GetVertexBufferDesc,
    XCAST(Optimize) Main_IDirect3DVertexBufferImpl_7_Optimize,
    XCAST(ProcessVerticesStrided) Main_IDirect3DVertexBufferImpl_7_ProcessVerticesStrided
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DVertexBuffer.fun))
#else
# define XCAST(fun)     (void*)
#endif

ICOM_VTABLE(IDirect3DVertexBuffer) VTABLE_IDirect3DVertexBuffer =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    XCAST(QueryInterface) Thunk_IDirect3DVertexBufferImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DVertexBufferImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DVertexBufferImpl_1_Release,
    XCAST(Lock) Thunk_IDirect3DVertexBufferImpl_1_Lock,
    XCAST(Unlock) Thunk_IDirect3DVertexBufferImpl_1_Unlock,
    XCAST(ProcessVertices) Main_IDirect3DVertexBufferImpl_1_ProcessVertices,
    XCAST(GetVertexBufferDesc) Thunk_IDirect3DVertexBufferImpl_1_GetVertexBufferDesc,
    XCAST(Optimize) Main_IDirect3DVertexBufferImpl_1_Optimize
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

HRESULT d3dvertexbuffer_create(IDirect3DVertexBufferImpl **obj, IDirect3DImpl *d3d, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc, DWORD dwFlags)
{
    IDirect3DVertexBufferImpl *object;
    static const flag_info flags[] = {
        FE(D3DVBCAPS_OPTIMIZED),
	FE(D3DVBCAPS_SYSTEMMEMORY),
	FE(D3DVBCAPS_WRITEONLY)
    };

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBufferImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->d3d = d3d;
    object->desc = *lpD3DVertBufDesc;
    object->vertex_buffer_size = get_flexible_vertex_size(lpD3DVertBufDesc->dwFVF, NULL) * lpD3DVertBufDesc->dwNumVertices;
    object->vertices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->vertex_buffer_size);
    
    ICOM_INIT_INTERFACE(object, IDirect3DVertexBuffer,  VTABLE_IDirect3DVertexBuffer);
    ICOM_INIT_INTERFACE(object, IDirect3DVertexBuffer7, VTABLE_IDirect3DVertexBuffer7);

    *obj = object;

    if (TRACE_ON(ddraw)) {
        TRACE(" creating implementation at %p with description : \n", *obj);
	TRACE("  flags        : "); DDRAW_dump_flags_(lpD3DVertBufDesc->dwCaps, flags, sizeof(flags)/sizeof(flags[0]), TRUE);
	TRACE("  vertex type  : "); dump_flexible_vertex(lpD3DVertBufDesc->dwFVF);
	TRACE("  num vertices : %ld\n", lpD3DVertBufDesc->dwNumVertices);
    }
    
    
    return D3D_OK;
}
