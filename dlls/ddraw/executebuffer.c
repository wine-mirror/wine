/* Direct3D ExecuteBuffer
 * Copyright (c) 1998-2004 Lionel ULMER
 * Copyright (c) 2002-2004 Christian Costa
 *
 * This file contains the implementation of Direct3DExecuteBuffer.
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
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

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

static void _dump_d3dstatus(LPD3DSTATUS lpStatus) {

}

static void _dump_executedata(LPD3DEXECUTEDATA lpData) {
    DPRINTF("dwSize : %ld\n", lpData->dwSize);
    DPRINTF("Vertex      Offset : %ld  Count  : %ld\n", lpData->dwVertexOffset, lpData->dwVertexCount);
    DPRINTF("Instruction Offset : %ld  Length : %ld\n", lpData->dwInstructionOffset, lpData->dwInstructionLength);
    DPRINTF("HVertex     Offset : %ld\n", lpData->dwHVertexOffset);
    _dump_d3dstatus(&(lpData->dsStatus));
}

static void _dump_D3DEXECUTEBUFFERDESC(LPD3DEXECUTEBUFFERDESC lpDesc) {
    DPRINTF("dwSize       : %ld\n", lpDesc->dwSize);
    DPRINTF("dwFlags      : %lx\n", lpDesc->dwFlags);
    DPRINTF("dwCaps       : %lx\n", lpDesc->dwCaps);
    DPRINTF("dwBufferSize : %ld\n", lpDesc->dwBufferSize);
    DPRINTF("lpData       : %p\n", lpDesc->lpData);
}

static void execute(IDirect3DExecuteBufferImpl *This,
		    IDirect3DDeviceImpl *lpDevice,
		    IDirect3DViewportImpl *lpViewport)
{
    /* DWORD bs = This->desc.dwBufferSize; */
    DWORD vs = This->data.dwVertexOffset;
    /* DWORD vc = This->data.dwVertexCount; */
    DWORD is = This->data.dwInstructionOffset;
    /* DWORD il = This->data.dwInstructionLength; */

    char *instr = (char *)This->desc.lpData + is;

    /* Should check if the viewport was added or not to the device */

    /* Activate the viewport */
    lpViewport->active_device = lpDevice;
    lpViewport->activate(lpViewport);

    TRACE("ExecuteData :\n");
    if (TRACE_ON(ddraw))
      _dump_executedata(&(This->data));

    while (1) {
        LPD3DINSTRUCTION current = (LPD3DINSTRUCTION) instr;
	BYTE size;
	WORD count;
	
	count = current->wCount;
	size = current->bSize;
	instr += sizeof(D3DINSTRUCTION);
	
	switch (current->bOpcode) {
	    case D3DOP_POINT: {
	        WARN("POINT-s          (%d)\n", count);
		instr += count * size;
	    } break;

	    case D3DOP_LINE: {
	        WARN("LINE-s           (%d)\n", count);
		instr += count * size;
	    } break;

	    case D3DOP_TRIANGLE: {
	        int i;
		D3DTLVERTEX *tl_vx = (D3DTLVERTEX *) This->vertex_data;
		TRACE("TRIANGLE         (%d)\n", count);
		
		if (count*3>This->nb_indices) {
		    This->nb_indices = count * 3;
                    HeapFree(GetProcessHeap(),0,This->indices);
		    This->indices = HeapAlloc(GetProcessHeap(),0,sizeof(WORD)*This->nb_indices);
		}
			
		for (i = 0; i < count; i++) {
                    LPD3DTRIANGLE ci = (LPD3DTRIANGLE) instr;
		    TRACE_(ddraw_geom)("  v1: %d  v2: %d  v3: %d\n",ci->u1.v1, ci->u2.v2, ci->u3.v3);
		    TRACE_(ddraw_geom)("  Flags : ");
		    if (TRACE_ON(ddraw)) {
			/* Wireframe */
			if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)
	        	    TRACE_(ddraw_geom)("EDGEENABLE1 ");
	    		if (ci->wFlags & D3DTRIFLAG_EDGEENABLE2)
	        	    TRACE_(ddraw_geom)("EDGEENABLE2 ");
	    		if (ci->wFlags & D3DTRIFLAG_EDGEENABLE1)
	        	    TRACE_(ddraw_geom)("EDGEENABLE3 ");
	    		/* Strips / Fans */
	    		if (ci->wFlags == D3DTRIFLAG_EVEN)
	        	    TRACE_(ddraw_geom)("EVEN ");
	    		if (ci->wFlags == D3DTRIFLAG_ODD)
	        	    TRACE_(ddraw_geom)("ODD ");
	    		if (ci->wFlags == D3DTRIFLAG_START)
	        	    TRACE_(ddraw_geom)("START ");
	    		if ((ci->wFlags > 0) && (ci->wFlags < 30))
	       		    TRACE_(ddraw_geom)("STARTFLAT(%d) ", ci->wFlags);
	    		TRACE_(ddraw_geom)("\n");
        	    }
		    This->indices[(i * 3)    ] = ci->u1.v1;
		    This->indices[(i * 3) + 1] = ci->u2.v2;
		    This->indices[(i * 3) + 2] = ci->u3.v3;
                    instr += size;
		}
                IDirect3DDevice7_DrawIndexedPrimitive(ICOM_INTERFACE(lpDevice,IDirect3DDevice7),
				                      D3DPT_TRIANGLELIST,D3DFVF_TLVERTEX,tl_vx,0,This->indices,count*3,0);
	    } break;

	    case D3DOP_MATRIXLOAD:
	        WARN("MATRIXLOAD-s     (%d)\n", count);
	        instr += count * size;
	        break;

	    case D3DOP_MATRIXMULTIPLY: {
	        int i;
		TRACE("MATRIXMULTIPLY   (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DMATRIXMULTIPLY ci = (LPD3DMATRIXMULTIPLY) instr;
		    LPD3DMATRIX a = (LPD3DMATRIX) ci->hDestMatrix;
		    LPD3DMATRIX b = (LPD3DMATRIX) ci->hSrcMatrix1;
		    LPD3DMATRIX c = (LPD3DMATRIX) ci->hSrcMatrix2;
		    
		    TRACE("  Dest : %08lx  Src1 : %08lx  Src2 : %08lx\n",
			  ci->hDestMatrix, ci->hSrcMatrix1, ci->hSrcMatrix2);
		    
                    multiply_matrix(a,c,b);

		    instr += size;
		}
	    } break;

	    case D3DOP_STATETRANSFORM: {
	        int i;
		TRACE("STATETRANSFORM   (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;

		    IDirect3DDevice7_SetTransform(ICOM_INTERFACE(lpDevice, IDirect3DDevice7),
						  ci->u1.drstRenderStateType, (LPD3DMATRIX)ci->u2.dwArg[0]);
		    
		    instr += size;
		}
	    } break;

	    case D3DOP_STATELIGHT: {
	        int i;
		TRACE("STATELIGHT       (%d)\n", count);
		
		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;

		    TRACE("(%08x,%08lx)\n",ci->u1.dlstLightStateType, ci->u2.dwArg[0]);

		    if (!ci->u1.dlstLightStateType && (ci->u1.dlstLightStateType > D3DLIGHTSTATE_COLORVERTEX))
			ERR("Unexpected Light State Type\n");
		    else if (ci->u1.dlstLightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */) {
			IDirect3DMaterialImpl *mat = (IDirect3DMaterialImpl *) ci->u2.dwArg[0];

			if (mat != NULL) {
			    mat->activate(mat);
			} else {
			    FIXME(" D3DLIGHTSTATE_MATERIAL called with NULL material !!!\n");
			}
		    }
		   else if (ci->u1.dlstLightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */) {
			switch (ci->u2.dwArg[0]) {
			    case D3DCOLOR_MONO:
			       ERR("DDCOLOR_MONO should not happen!\n");
			       break;
			    case D3DCOLOR_RGB:
			       /* We are already in this mode */
			       break;
			    default:
		               ERR("Unknown color model!\n");
			}
		    } else {
		    	D3DRENDERSTATETYPE rs = 0;
		    	switch (ci->u1.dlstLightStateType) {

		    	    case D3DLIGHTSTATE_AMBIENT:       /* 2 */
				rs = D3DRENDERSTATE_AMBIENT;
				break;		
			    case D3DLIGHTSTATE_FOGMODE:       /* 4 */
				rs = D3DRENDERSTATE_FOGVERTEXMODE;
				break;
			    case D3DLIGHTSTATE_FOGSTART:      /* 5 */
				rs = D3DRENDERSTATE_FOGSTART;
				break;
			    case D3DLIGHTSTATE_FOGEND:        /* 6 */
				rs = D3DRENDERSTATE_FOGEND;
				break;
			    case D3DLIGHTSTATE_FOGDENSITY:    /* 7 */
				rs = D3DRENDERSTATE_FOGDENSITY;
				break;
			    case D3DLIGHTSTATE_COLORVERTEX:   /* 8 */
				rs = D3DRENDERSTATE_COLORVERTEX;
				break;
			    default:
				break;
			}
			
		        IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(lpDevice, IDirect3DDevice7),
	                                		rs,ci->u2.dwArg[0]);
		   }

		   instr += size;
		}
	    } break;

	    case D3DOP_STATERENDER: {
	        int i;
		TRACE("STATERENDER      (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DSTATE ci = (LPD3DSTATE) instr;
		    
		    IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(lpDevice, IDirect3DDevice7),
						    ci->u1.drstRenderStateType, ci->u2.dwArg[0]);

		    instr += size;
		}
	    } break;

	    case D3DOP_PROCESSVERTICES: {
	        int i;
		TRACE("PROCESSVERTICES  (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DPROCESSVERTICES ci = (LPD3DPROCESSVERTICES) instr;

		    TRACE("  Start : %d Dest : %d Count : %ld\n",
			  ci->wStart, ci->wDest, ci->dwCount);
		    TRACE("  Flags : ");
		    if (TRACE_ON(ddraw)) {
		        if (ci->dwFlags & D3DPROCESSVERTICES_COPY)
			    TRACE("COPY ");
			if (ci->dwFlags & D3DPROCESSVERTICES_NOCOLOR)
			    TRACE("NOCOLOR ");
			if (ci->dwFlags == D3DPROCESSVERTICES_OPMASK)
			    TRACE("OPMASK ");
			if (ci->dwFlags & D3DPROCESSVERTICES_TRANSFORM)
			    TRACE("TRANSFORM ");
			if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT)
			    TRACE("TRANSFORMLIGHT ");
			if (ci->dwFlags & D3DPROCESSVERTICES_UPDATEEXTENTS)
			    TRACE("UPDATEEXTENTS ");
			TRACE("\n");
		    }
		    
		    /* This is where doing Direct3D on top on OpenGL is quite difficult.
		       This method transforms a set of vertices using the CURRENT state
		       (lighting, projection, ...) but does not rasterize them.
		       They will only be put on screen later (with the POINT / LINE and
		       TRIANGLE op-codes). The problem is that you can have a triangle
		       with each point having been transformed using another state...
		       
		       In this implementation, I will emulate only ONE thing : each
		       vertex can have its own "WORLD" transformation (this is used in the
		       TWIST.EXE demo of the 5.2 SDK). I suppose that all vertices of the
		       execute buffer use the same state.
		       
		       If I find applications that change other states, I will try to do a
		       more 'fine-tuned' state emulation (but I may become quite tricky if
		       it changes a light position in the middle of a triangle).
		       
		       In this case, a 'direct' approach (i.e. without using OpenGL, but
		       writing our own 3D rasterizer) would be easier. */
		    
		    /* The current method (with the hypothesis that only the WORLD matrix
		       will change between two points) is like this :
		       - I transform 'manually' all the vertices with the current WORLD
		         matrix and store them in the vertex buffer
		       - during the rasterization phase, the WORLD matrix will be set to
		         the Identity matrix */
		    
		    /* Enough for the moment */
		    if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORMLIGHT) {
		        unsigned int nb;
			D3DVERTEX  *src = ((LPD3DVERTEX)  ((char *)This->desc.lpData + vs)) + ci->wStart;
			D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (This->vertex_data)) + ci->wDest;
			D3DMATRIX *mat2 = lpDevice->world_mat;
			D3DMATRIX mat;
			D3DVALUE nx,ny,nz;
			D3DVIEWPORT* Viewport = &lpViewport->viewports.vp1;
			
			if (TRACE_ON(ddraw)) {
			    TRACE("  Projection Matrix : (%p)\n", lpDevice->proj_mat);
			    dump_D3DMATRIX(lpDevice->proj_mat);
			    TRACE("  View       Matrix : (%p)\n", lpDevice->view_mat);
			    dump_D3DMATRIX(lpDevice->view_mat);
			    TRACE("  World Matrix : (%p)\n", lpDevice->world_mat);
			    dump_D3DMATRIX(lpDevice->world_mat);
			}

			multiply_matrix(&mat,lpDevice->view_mat,lpDevice->world_mat);
			multiply_matrix(&mat,lpDevice->proj_mat,&mat);

			for (nb = 0; nb < ci->dwCount; nb++) {
			    /* Normals transformation */
			    nx = (src->u4.nx * mat2->_11) + (src->u5.ny * mat2->_21) + (src->u6.nz * mat2->_31);
			    ny = (src->u4.nx * mat2->_12) + (src->u5.ny * mat2->_22) + (src->u6.nz * mat2->_32);
			    nz = (src->u4.nx * mat2->_13) + (src->u5.ny * mat2->_23) + (src->u6.nz * mat2->_33);
			    
			    /* No lighting yet */
			    dst->u5.color = 0xFFFFFFFF; /* Opaque white */
			    dst->u6.specular = 0xFF000000; /* No specular and no fog factor */
			    
			    dst->u7.tu  = src->u7.tu;
			    dst->u8.tv  = src->u8.tv;
			    
			    /* Now, the matrix multiplication */
			    dst->u1.sx = (src->u1.x * mat._11) + (src->u2.y * mat._21) + (src->u3.z * mat._31) + (1.0 * mat._41);
			    dst->u2.sy = (src->u1.x * mat._12) + (src->u2.y * mat._22) + (src->u3.z * mat._32) + (1.0 * mat._42);
			    dst->u3.sz = (src->u1.x * mat._13) + (src->u2.y * mat._23) + (src->u3.z * mat._33) + (1.0 * mat._43);
			    dst->u4.rhw = (src->u1.x * mat._14) + (src->u2.y * mat._24) + (src->u3.z * mat._34) + (1.0 * mat._44);

			    dst->u1.sx = dst->u1.sx / dst->u4.rhw * Viewport->dwWidth / 2
				       + Viewport->dwX + Viewport->dwWidth / 2;
			    dst->u2.sy = dst->u2.sy / dst->u4.rhw * Viewport->dwHeight / 2
				       + Viewport->dwY + Viewport->dwHeight / 2;
			    dst->u3.sz /= dst->u4.rhw;
			    dst->u4.rhw = 1 / dst->u4.rhw;

			    src++;
			    dst++;
			    
			}
		    } else if (ci->dwFlags == D3DPROCESSVERTICES_TRANSFORM) {
		        unsigned int nb;
			D3DLVERTEX *src  = ((LPD3DLVERTEX) ((char *)This->desc.lpData + vs)) + ci->wStart;
			D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (This->vertex_data)) + ci->wDest;
			D3DMATRIX mat;
			D3DVIEWPORT* Viewport = &lpViewport->viewports.vp1;
			
			if (TRACE_ON(ddraw)) {
			    TRACE("  Projection Matrix : (%p)\n", lpDevice->proj_mat);
			    dump_D3DMATRIX(lpDevice->proj_mat);
			    TRACE("  View       Matrix : (%p)\n", lpDevice->view_mat);
			    dump_D3DMATRIX(lpDevice->view_mat);
			    TRACE("  World Matrix : (%p)\n", &mat);
			    dump_D3DMATRIX(&mat);
			}

			multiply_matrix(&mat,lpDevice->view_mat,lpDevice->world_mat);
			multiply_matrix(&mat,lpDevice->proj_mat,&mat);

			for (nb = 0; nb < ci->dwCount; nb++) {
			    dst->u5.color = src->u4.color;
			    dst->u6.specular = src->u5.specular;
			    dst->u7.tu = src->u6.tu;
			    dst->u8.tv = src->u7.tv;
			    
			    /* Now, the matrix multiplication */
			    dst->u1.sx = (src->u1.x * mat._11) + (src->u2.y * mat._21) + (src->u3.z * mat._31) + (1.0 * mat._41);
			    dst->u2.sy = (src->u1.x * mat._12) + (src->u2.y * mat._22) + (src->u3.z * mat._32) + (1.0 * mat._42);
			    dst->u3.sz = (src->u1.x * mat._13) + (src->u2.y * mat._23) + (src->u3.z * mat._33) + (1.0 * mat._43);
			    dst->u4.rhw = (src->u1.x * mat._14) + (src->u2.y * mat._24) + (src->u3.z * mat._34) + (1.0 * mat._44);

			    dst->u1.sx /= dst->u4.rhw * Viewport->dvScaleX * Viewport->dwWidth / 2 + Viewport->dwX;
			    dst->u2.sy /= dst->u4.rhw * Viewport->dvScaleY * Viewport->dwHeight / 2 + Viewport->dwY;
			    dst->u3.sz /= dst->u4.rhw;
			    dst->u4.rhw = 1 / dst->u4.rhw;

			    src++;
			    dst++;
			}
		    } else if (ci->dwFlags == D3DPROCESSVERTICES_COPY) {
		        D3DTLVERTEX *src = ((LPD3DTLVERTEX) ((char *)This->desc.lpData + vs)) + ci->wStart;
			D3DTLVERTEX *dst = ((LPD3DTLVERTEX) (This->vertex_data)) + ci->wDest;
			
			memcpy(dst, src, ci->dwCount * sizeof(D3DTLVERTEX));
		    } else {
		        ERR("Unhandled vertex processing !\n");
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_TEXTURELOAD: {
	        WARN("TEXTURELOAD-s    (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_EXIT: {
	        TRACE("EXIT             (%d)\n", count);
		/* We did this instruction */
		instr += size;
		/* Exit this loop */
		goto end_of_buffer;
	    } break;

	    case D3DOP_BRANCHFORWARD: {
	        int i;
		TRACE("BRANCHFORWARD    (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DBRANCH ci = (LPD3DBRANCH) instr;

		    if ((This->data.dsStatus.dwStatus & ci->dwMask) == ci->dwValue) {
		        if (!ci->bNegate) {
			    TRACE(" Branch to %ld\n", ci->dwOffset);
			    instr = (char*)current + ci->dwOffset;
			    break;
			}
		    } else {
		        if (ci->bNegate) {
			    TRACE(" Branch to %ld\n", ci->dwOffset);
			    instr = (char*)current + ci->dwOffset;
			    break;
			}
		    }

		    instr += size;
		}
	    } break;

	    case D3DOP_SPAN: {
	        WARN("SPAN-s           (%d)\n", count);

		instr += count * size;
	    } break;

	    case D3DOP_SETSTATUS: {
	        int i;
		TRACE("SETSTATUS        (%d)\n", count);

		for (i = 0; i < count; i++) {
		    LPD3DSTATUS ci = (LPD3DSTATUS) instr;
		    
		    This->data.dsStatus = *ci;

		    instr += size;
		}
	    } break;

	    default:
	        ERR("Unhandled OpCode %d !!!\n",current->bOpcode);
	        /* Try to save ... */
	        instr += count * size;
	        break;
	}
    }

end_of_buffer:
    ;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_QueryInterface(LPDIRECT3DEXECUTEBUFFER iface,
                                                 REFIID riid,
                                                 LPVOID* obp)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(riid), obp);

    *obp = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) ) {
        IDirect3DExecuteBuffer_AddRef(ICOM_INTERFACE(This, IDirect3DExecuteBuffer));
	*obp = iface;
	TRACE("  Creating IUnknown interface at %p.\n", *obp);
	return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DMaterial, riid ) ) {
        IDirect3DExecuteBuffer_AddRef(ICOM_INTERFACE(This, IDirect3DExecuteBuffer));
        *obp = ICOM_INTERFACE(This, IDirect3DExecuteBuffer);
	TRACE("  Creating IDirect3DExecuteBuffer interface %p\n", *obp);
	return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return OLE_E_ENUM_NOMORE;
}

ULONG WINAPI
Main_IDirect3DExecuteBufferImpl_1_AddRef(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    FIXME("(%p/%p)->()incrementing from %lu.\n", This, iface, ref - 1);

    return ref;
}

ULONG WINAPI
Main_IDirect3DExecuteBufferImpl_1_Release(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->()decrementing from %lu.\n", This, iface, ref + 1);

    if (!ref) {
        if (This->need_free)
	    HeapFree(GetProcessHeap(),0,This->desc.lpData);
        HeapFree(GetProcessHeap(),0,This->vertex_data);
        HeapFree(GetProcessHeap(),0,This->indices);
	HeapFree(GetProcessHeap(),0,This);
	return 0;
    }

    return ref;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Initialize(LPDIRECT3DEXECUTEBUFFER iface,
                                             LPDIRECT3DDEVICE lpDirect3DDevice,
                                             LPD3DEXECUTEBUFFERDESC lpDesc)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%p,%p) no-op....\n", This, iface, lpDirect3DDevice, lpDesc);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Lock(LPDIRECT3DEXECUTEBUFFER iface,
                                       LPD3DEXECUTEBUFFERDESC lpDesc)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpDesc);

    dwSize = lpDesc->dwSize;
    memset(lpDesc, 0, dwSize);
    memcpy(lpDesc, &This->desc, dwSize);
    
    if (TRACE_ON(ddraw)) {
        TRACE("  Returning description :\n");
	_dump_D3DEXECUTEBUFFERDESC(lpDesc);
    }
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Unlock(LPDIRECT3DEXECUTEBUFFER iface)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->() no-op...\n", This, iface);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_SetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
                                                 LPD3DEXECUTEDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD nbvert;
    TRACE("(%p/%p)->(%p)\n", This, iface, lpData);

    memcpy(&This->data, lpData, lpData->dwSize);

    /* Get the number of vertices in the execute buffer */
    nbvert = This->data.dwVertexCount;
    
    /* Prepares the transformed vertex buffer */
    HeapFree(GetProcessHeap(), 0, This->vertex_data);
    This->vertex_data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nbvert * sizeof(D3DTLVERTEX));

    if (TRACE_ON(ddraw)) {
        _dump_executedata(lpData);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_GetExecuteData(LPDIRECT3DEXECUTEBUFFER iface,
                                                 LPD3DEXECUTEDATA lpData)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    DWORD dwSize;
    TRACE("(%p/%p)->(%p): stub!\n", This, iface, lpData);

    dwSize = lpData->dwSize;
    memset(lpData, 0, dwSize);
    memcpy(lpData, &This->data, dwSize);

    if (TRACE_ON(ddraw)) {
        TRACE("Returning data :\n");
	_dump_executedata(lpData);
    }

    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Validate(LPDIRECT3DEXECUTEBUFFER iface,
                                           LPDWORD lpdwOffset,
                                           LPD3DVALIDATECALLBACK lpFunc,
                                           LPVOID lpUserArg,
                                           DWORD dwReserved)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    FIXME("(%p/%p)->(%p,%p,%p,%08lx): stub!\n", This, iface, lpdwOffset, lpFunc, lpUserArg, dwReserved);
    return DD_OK;
}

HRESULT WINAPI
Main_IDirect3DExecuteBufferImpl_1_Optimize(LPDIRECT3DEXECUTEBUFFER iface,
                                           DWORD dwDummy)
{
    ICOM_THIS_FROM(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, iface);
    TRACE("(%p/%p)->(%08lx) no-op...\n", This, iface, dwDummy);
    return DD_OK;
}

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(VTABLE_IDirect3DExecuteBuffer.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirect3DExecuteBufferVtbl VTABLE_IDirect3DExecuteBuffer =
{
    XCAST(QueryInterface) Main_IDirect3DExecuteBufferImpl_1_QueryInterface,
    XCAST(AddRef) Main_IDirect3DExecuteBufferImpl_1_AddRef,
    XCAST(Release) Main_IDirect3DExecuteBufferImpl_1_Release,
    XCAST(Initialize) Main_IDirect3DExecuteBufferImpl_1_Initialize,
    XCAST(Lock) Main_IDirect3DExecuteBufferImpl_1_Lock,
    XCAST(Unlock) Main_IDirect3DExecuteBufferImpl_1_Unlock,
    XCAST(SetExecuteData) Main_IDirect3DExecuteBufferImpl_1_SetExecuteData,
    XCAST(GetExecuteData) Main_IDirect3DExecuteBufferImpl_1_GetExecuteData,
    XCAST(Validate) Main_IDirect3DExecuteBufferImpl_1_Validate,
    XCAST(Optimize) Main_IDirect3DExecuteBufferImpl_1_Optimize,
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
#undef XCAST
#endif


HRESULT d3dexecutebuffer_create(IDirect3DExecuteBufferImpl **obj, IDirectDrawImpl *d3d, IDirect3DDeviceImpl *d3ddev, LPD3DEXECUTEBUFFERDESC lpDesc)
{
    IDirect3DExecuteBufferImpl* object;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DExecuteBufferImpl));

    ICOM_INIT_INTERFACE(object, IDirect3DExecuteBuffer, VTABLE_IDirect3DExecuteBuffer);
    
    object->ref = 1;
    object->d3d = d3d;
    object->d3ddev = d3ddev;

    /* Initializes memory */
    memcpy(&object->desc, lpDesc, lpDesc->dwSize);

    /* No buffer given */
    if ((object->desc.dwFlags & D3DDEB_LPDATA) == 0)
        object->desc.lpData = NULL;

    /* No buffer size given */
    if ((lpDesc->dwFlags & D3DDEB_BUFSIZE) == 0)
        object->desc.dwBufferSize = 0;

    /* Create buffer if asked */
    if ((object->desc.lpData == NULL) && (object->desc.dwBufferSize > 0)) {
        object->need_free = TRUE;
	object->desc.lpData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,object->desc.dwBufferSize);
    } else {
        object->need_free = FALSE;
    }

    /* No vertices for the moment */
    object->vertex_data = NULL;

    object->desc.dwFlags |= D3DDEB_LPDATA;

    object->execute = execute;

    object->indices = NULL;
    object->nb_indices = 0;

    *obj = object;

    TRACE(" creating implementation at %p.\n", *obj);

    return DD_OK;
}
