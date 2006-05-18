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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "d3d.h"
#include "wine/debug.h"

#include "d3d_private.h"
#include "opengl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_geom);

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
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() incrementing from %lu.\n", This, iface, ref - 1);

    return ref;
}

ULONG WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_Release(LPDIRECT3DVERTEXBUFFER7 iface)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() decrementing from %lu.\n", This, iface, ref + 1);

    if (ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->vertices);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
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
    
    if (This->processed) {
        WARN(" application does a Lock on a vertex buffer resulting from a ProcessVertices call. Expect problems !\n");
    }

    if (This->desc.dwCaps & D3DVBCAPS_OPTIMIZED) return D3DERR_VERTEXBUFFEROPTIMIZED;

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
Main_IDirect3DVertexBufferImpl_7_1T_ProcessVertices(LPDIRECT3DVERTEXBUFFER7 iface,
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
    
    size = lpD3DVertexBufferDesc->dwSize;
    memset(lpD3DVertexBufferDesc, 0, size);
    memcpy(lpD3DVertexBufferDesc, &This->desc, 
	   (size < This->desc.dwSize) ? size : This->desc.dwSize);
    
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DVertexBufferImpl_7_1T_Optimize(LPDIRECT3DVERTEXBUFFER7 iface,
					     LPDIRECT3DDEVICE7 lpD3DDevice,
					     DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    FIXME("(%p/%p)->(%p,%08lx): stub!\n", This, iface, lpD3DDevice, dwFlags);

    This->desc.dwCaps |= D3DVBCAPS_OPTIMIZED;

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
Thunk_IDirect3DVertexBufferImpl_1_ProcessVertices(LPDIRECT3DVERTEXBUFFER iface,
						  DWORD dwVertexOp,
						  DWORD dwDestIndex,
						  DWORD dwCount,
						  LPDIRECT3DVERTEXBUFFER lpSrcBuffer,
						  DWORD dwSrcIndex,
						  LPDIRECT3DDEVICE3 lpD3DDevice,
						  DWORD dwFlags)
{
    TRACE("(%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx) thunking to IDirect3DVertexBuffer7 interface.\n", iface,
	  dwVertexOp, dwDestIndex, dwCount, lpSrcBuffer, dwSrcIndex, lpD3DDevice, dwFlags);
    return IDirect3DVertexBuffer7_ProcessVertices(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface),
						  dwVertexOp,
						  dwDestIndex,
						  dwCount,
						  COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, lpSrcBuffer),
						  dwSrcIndex,
						  COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, lpD3DDevice),
						  dwFlags);
}

HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Optimize(LPDIRECT3DVERTEXBUFFER iface,
					   LPDIRECT3DDEVICE3 lpD3DDevice,
					   DWORD dwFlags)
{
    TRACE("(%p)->(%p,%08lx) thunking to IDirect3DVertexBuffer7 interface.\n", iface, lpD3DDevice, dwFlags);
    return IDirect3DVertexBuffer7_Optimize(COM_INTERFACE_CAST(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, IDirect3DVertexBuffer7, iface),
					   COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice3, IDirect3DDevice7, lpD3DDevice),
					   dwFlags);
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

#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)

static HRESULT
process_vertices_strided(IDirect3DVertexBufferImpl *This,
			 DWORD dwVertexOp,
			 DWORD dwDestIndex,
			 DWORD dwCount,
			 LPD3DDRAWPRIMITIVESTRIDEDDATA lpStrideData,
			 DWORD dwVertexTypeDesc,
			 IDirect3DDeviceImpl *device_impl,
			 DWORD dwFlags)
{
    IDirect3DVertexBufferGLImpl *glThis = (IDirect3DVertexBufferGLImpl *) This;
    DWORD size = get_flexible_vertex_size(dwVertexTypeDesc);
    char *dest_ptr;
    unsigned int i;

    This->processed = TRUE;

    /* For the moment, the trick is to save the transform and lighting state at process
       time to restore them at drawing time.
       
       The BIG problem with this method is nothing prevents D3D to do dirty tricks like
       processing two different sets of vertices with two different rendering parameters
       and then to display them using the same DrawPrimitive call.

       It would be nice to check for such code here (but well, even this is not trivial
       to do).

       This is exactly what the TWIST.EXE demo does but using the same kind of ugly stuff
       in the D3DExecuteBuffer code. I really wonder why Microsoft went back in time when
       implementing this mostly useless (IMHO) API.
    */
    glThis->dwVertexTypeDesc = dwVertexTypeDesc;

    if (dwVertexTypeDesc & D3DFVF_NORMAL) {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (glThis->vertices == NULL) {
        glThis->vertices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size * This->desc.dwNumVertices);
    }
    dest_ptr = ((char *) glThis->vertices) + dwDestIndex * size;
    
    memcpy(&(glThis->world_mat), device_impl->world_mat, sizeof(D3DMATRIX));
    memcpy(&(glThis->view_mat), device_impl->view_mat, sizeof(D3DMATRIX));
    memcpy(&(glThis->proj_mat), device_impl->proj_mat, sizeof(D3DMATRIX));

    for (i = 0; i < dwCount; i++) {
        unsigned int tex_index;

	if ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
	    D3DVALUE *position =
	      (D3DVALUE *) (((char *) lpStrideData->position.lpvData) + i * lpStrideData->position.dwStride);
	    copy_and_next(dest_ptr, position, 3 * sizeof(D3DVALUE));
	} else if ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) {
	    D3DVALUE *position =
	      (D3DVALUE *) (((char *) lpStrideData->position.lpvData) + i * lpStrideData->position.dwStride);
	    copy_and_next(dest_ptr, position, 4 * sizeof(D3DVALUE));
	}
	if (dwVertexTypeDesc & D3DFVF_RESERVED1) {
	    dest_ptr += sizeof(DWORD);
	}
	if (dwVertexTypeDesc & D3DFVF_NORMAL) { 
	    D3DVALUE *normal = 
	      (D3DVALUE *) (((char *) lpStrideData->normal.lpvData) + i * lpStrideData->normal.dwStride);	    
	    copy_and_next(dest_ptr, normal, 3 * sizeof(D3DVALUE));
	}
	if (dwVertexTypeDesc & D3DFVF_DIFFUSE) {
	    DWORD *color_d = 
	      (DWORD *) (((char *) lpStrideData->diffuse.lpvData) + i * lpStrideData->diffuse.dwStride);
	    copy_and_next(dest_ptr, color_d, sizeof(DWORD));
	}
	if (dwVertexTypeDesc & D3DFVF_SPECULAR) { 
	    DWORD *color_s = 
	      (DWORD *) (((char *) lpStrideData->specular.lpvData) + i * lpStrideData->specular.dwStride);
	    copy_and_next(dest_ptr, color_s, sizeof(DWORD));
	}
	for (tex_index = 0; tex_index < GET_TEXCOUNT_FROM_FVF(dwVertexTypeDesc); tex_index++) {
	    D3DVALUE *tex_coord =
	      (D3DVALUE *) (((char *) lpStrideData->textureCoords[tex_index].lpvData) + 
			    i * lpStrideData->textureCoords[tex_index].dwStride);
	    copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(dwVertexTypeDesc, tex_index) * sizeof(D3DVALUE));
	}

	if (TRACE_ON(ddraw_geom)) {
	    if ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) == D3DFVF_XYZ) {
	        D3DVALUE *position =
		  (D3DVALUE *) (((char *) lpStrideData->position.lpvData) + i * lpStrideData->position.dwStride);
		TRACE_(ddraw_geom)(" %f %f %f", position[0], position[1], position[2]);
	    } else if ((dwVertexTypeDesc & D3DFVF_POSITION_MASK) == D3DFVF_XYZRHW) {
	        D3DVALUE *position =
		  (D3DVALUE *) (((char *) lpStrideData->position.lpvData) + i * lpStrideData->position.dwStride);
		TRACE_(ddraw_geom)(" %f %f %f %f", position[0], position[1], position[2], position[3]);
	    }
	    if (dwVertexTypeDesc & D3DFVF_NORMAL) { 
	        D3DVALUE *normal = 
		  (D3DVALUE *) (((char *) lpStrideData->normal.lpvData) + i * lpStrideData->normal.dwStride);	    
		TRACE_(ddraw_geom)(" / %f %f %f", normal[0], normal[1], normal[2]);
	    }
	    if (dwVertexTypeDesc & D3DFVF_DIFFUSE) {
	        DWORD *color_d = 
		  (DWORD *) (((char *) lpStrideData->diffuse.lpvData) + i * lpStrideData->diffuse.dwStride);
		TRACE_(ddraw_geom)(" / %02lx %02lx %02lx %02lx",
				   (*color_d >> 16) & 0xFF,
				   (*color_d >>  8) & 0xFF,
				   (*color_d >>  0) & 0xFF,
				   (*color_d >> 24) & 0xFF);
	    }
	    if (dwVertexTypeDesc & D3DFVF_SPECULAR) { 
	        DWORD *color_s = 
		  (DWORD *) (((char *) lpStrideData->specular.lpvData) + i * lpStrideData->specular.dwStride);
		TRACE_(ddraw_geom)(" / %02lx %02lx %02lx %02lx",
				   (*color_s >> 16) & 0xFF,
				   (*color_s >>  8) & 0xFF,
				   (*color_s >>  0) & 0xFF,
				   (*color_s >> 24) & 0xFF);
	    }
	    for (tex_index = 0; tex_index < GET_TEXCOUNT_FROM_FVF(dwVertexTypeDesc); tex_index++) {
	        D3DVALUE *tex_coord =
		  (D3DVALUE *) (((char *) lpStrideData->textureCoords[tex_index].lpvData) + 
				i * lpStrideData->textureCoords[tex_index].dwStride);
		switch (GET_TEXCOORD_SIZE_FROM_FVF(dwVertexTypeDesc, tex_index)) {
		    case 1: TRACE_(ddraw_geom)(" / %f", tex_coord[0]); break;
		    case 2: TRACE_(ddraw_geom)(" / %f %f", tex_coord[0], tex_coord[1]); break;
		    case 3: TRACE_(ddraw_geom)(" / %f %f %f", tex_coord[0], tex_coord[1], tex_coord[2]); break;
		    case 4: TRACE_(ddraw_geom)(" / %f %f %f %f", tex_coord[0], tex_coord[1], tex_coord[2], tex_coord[3]); break;
		    default: TRACE_(ddraw_geom)("Invalid texture size (%ld) !!!", GET_TEXCOORD_SIZE_FROM_FVF(dwVertexTypeDesc, tex_index)); break;
		}
	    }
	    TRACE_(ddraw_geom)("\n");
	}
    }

    return DD_OK;
}

#undef copy_and_next

HRESULT WINAPI
GL_IDirect3DVertexBufferImpl_7_1T_ProcessVertices(LPDIRECT3DVERTEXBUFFER7 iface,
						  DWORD dwVertexOp,
						  DWORD dwDestIndex,
						  DWORD dwCount,
						  LPDIRECT3DVERTEXBUFFER7 lpSrcBuffer,
						  DWORD dwSrcIndex,
						  LPDIRECT3DDEVICE7 lpD3DDevice,
						  DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    IDirect3DVertexBufferImpl *src_impl = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, lpSrcBuffer);
    IDirect3DDeviceImpl *device_impl = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice7, lpD3DDevice);
    D3DDRAWPRIMITIVESTRIDEDDATA strided;
    DWORD size;

    TRACE("(%p/%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx)\n", This, iface, dwVertexOp, dwDestIndex, dwCount, lpSrcBuffer, dwSrcIndex, lpD3DDevice, dwFlags);    

    if (TRACE_ON(ddraw)) {
        TRACE(" - vertex operations : "); dump_D3DVOP(dwVertexOp);
	TRACE(" - flags             : "); dump_D3DPV(dwFlags);
    }

    if ((dwVertexOp & D3DVOP_TRANSFORM) == 0) return DDERR_INVALIDPARAMS;

    size = get_flexible_vertex_size(src_impl->desc.dwFVF);
    convert_FVF_to_strided_data(src_impl->desc.dwFVF, ((char *) src_impl->vertices) + dwSrcIndex * size, &strided, 0);

    return process_vertices_strided(This, dwVertexOp, dwDestIndex, dwCount, &strided, src_impl->desc.dwFVF, device_impl, dwFlags);
}

HRESULT WINAPI
GL_IDirect3DVertexBufferImpl_7_ProcessVerticesStrided(LPDIRECT3DVERTEXBUFFER7 iface,
						      DWORD dwVertexOp,
						      DWORD dwDestIndex,
						      DWORD dwCount,
						      LPD3DDRAWPRIMITIVESTRIDEDDATA lpStrideData,
						      DWORD dwVertexTypeDesc,
						      LPDIRECT3DDEVICE7 lpD3DDevice,
						      DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, iface);
    IDirect3DDeviceImpl *device_impl = ICOM_OBJECT(IDirect3DDeviceImpl, IDirect3DDevice7, lpD3DDevice);

    TRACE("(%p/%p)->(%08lx,%08lx,%08lx,%p,%08lx,%p,%08lx)\n", This, iface, dwVertexOp, dwDestIndex, dwCount, lpStrideData, dwVertexTypeDesc, lpD3DDevice, dwFlags);
    if (TRACE_ON(ddraw)) {
        TRACE(" - vertex operations : "); dump_D3DVOP(dwVertexOp);
	TRACE(" - flags             : "); dump_D3DPV(dwFlags);
	TRACE(" - vertex format     : "); dump_flexible_vertex(dwVertexTypeDesc);
    }

    if ((dwVertexOp & D3DVOP_TRANSFORM) == 0) return DDERR_INVALIDPARAMS;

    return process_vertices_strided(This, dwVertexOp, dwDestIndex, dwCount, lpStrideData, dwVertexTypeDesc, device_impl, dwFlags);
}



#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DVertexBuffer7.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DVertexBuffer7Vtbl VTABLE_IDirect3DVertexBuffer7 =
{
    XCAST(QueryInterface) Main_IDirect3DVertexBufferImpl_7_1T_QueryInterface,
    XCAST(AddRef) Main_IDirect3DVertexBufferImpl_7_1T_AddRef,
    XCAST(Release) Main_IDirect3DVertexBufferImpl_7_1T_Release,
    XCAST(Lock) Main_IDirect3DVertexBufferImpl_7_1T_Lock,
    XCAST(Unlock) Main_IDirect3DVertexBufferImpl_7_1T_Unlock,
    XCAST(ProcessVertices) GL_IDirect3DVertexBufferImpl_7_1T_ProcessVertices,
    XCAST(GetVertexBufferDesc) Main_IDirect3DVertexBufferImpl_7_1T_GetVertexBufferDesc,
    XCAST(Optimize) Main_IDirect3DVertexBufferImpl_7_1T_Optimize,
    XCAST(ProcessVerticesStrided) GL_IDirect3DVertexBufferImpl_7_ProcessVerticesStrided
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DVertexBuffer.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DVertexBufferVtbl VTABLE_IDirect3DVertexBuffer =
{
    XCAST(QueryInterface) Thunk_IDirect3DVertexBufferImpl_1_QueryInterface,
    XCAST(AddRef) Thunk_IDirect3DVertexBufferImpl_1_AddRef,
    XCAST(Release) Thunk_IDirect3DVertexBufferImpl_1_Release,
    XCAST(Lock) Thunk_IDirect3DVertexBufferImpl_1_Lock,
    XCAST(Unlock) Thunk_IDirect3DVertexBufferImpl_1_Unlock,
    XCAST(ProcessVertices) Thunk_IDirect3DVertexBufferImpl_1_ProcessVertices,
    XCAST(GetVertexBufferDesc) Thunk_IDirect3DVertexBufferImpl_1_GetVertexBufferDesc,
    XCAST(Optimize) Thunk_IDirect3DVertexBufferImpl_1_Optimize
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif

HRESULT d3dvertexbuffer_create(IDirect3DVertexBufferImpl **obj, IDirectDrawImpl *d3d, LPD3DVERTEXBUFFERDESC lpD3DVertBufDesc, DWORD dwFlags)
{
    IDirect3DVertexBufferImpl *object;
    static const flag_info flags[] = {
        FE(D3DVBCAPS_DONOTCLIP),
        FE(D3DVBCAPS_OPTIMIZED),
	FE(D3DVBCAPS_SYSTEMMEMORY),
	FE(D3DVBCAPS_WRITEONLY)
    };

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBufferGLImpl));
    if (object == NULL) return DDERR_OUTOFMEMORY;

    object->ref = 1;
    object->d3d = d3d;
    object->desc = *lpD3DVertBufDesc;
    object->vertex_buffer_size = get_flexible_vertex_size(lpD3DVertBufDesc->dwFVF) * lpD3DVertBufDesc->dwNumVertices;
    object->vertices = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, object->vertex_buffer_size);
    
    ICOM_INIT_INTERFACE(object, IDirect3DVertexBuffer,  VTABLE_IDirect3DVertexBuffer);
    ICOM_INIT_INTERFACE(object, IDirect3DVertexBuffer7, VTABLE_IDirect3DVertexBuffer7);

    *obj = object;

    if (TRACE_ON(ddraw)) {
        TRACE(" creating implementation at %p with description :\n", *obj);
	TRACE("  flags        : "); DDRAW_dump_flags_(lpD3DVertBufDesc->dwCaps, flags, sizeof(flags)/sizeof(flags[0]), TRUE);
	TRACE("  vertex type  : "); dump_flexible_vertex(lpD3DVertBufDesc->dwFVF);
	TRACE("  num vertices : %ld\n", lpD3DVertBufDesc->dwNumVertices);
    }
    
    
    return D3D_OK;
}
