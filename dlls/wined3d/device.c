/*
 * IWineD3DDevice implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);
#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

/**********************************************************
 * Global variable / Constants follow
 **********************************************************/
const float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};  /* When needed for comparisons */

/**********************************************************
 * Utility functions follow
 **********************************************************/

/**********************************************************
 * IWineD3DDevice implementation follows
 **********************************************************/
HRESULT WINAPI IWineD3DDeviceImpl_GetParent(IWineD3DDevice *iface, IUnknown **pParent) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    *pParent = This->parent;
    IUnknown_AddRef(This->parent);
    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateVertexBuffer(IWineD3DDevice *iface, UINT Size, DWORD Usage, 
                             DWORD FVF, D3DPOOL Pool, IWineD3DVertexBuffer** ppVertexBuffer, HANDLE *sharedHandle,
                             IUnknown *parent) {

    IWineD3DVertexBufferImpl *object;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DVertexBufferImpl));
    object->lpVtbl                = &IWineD3DVertexBuffer_Vtbl;
    object->resource.wineD3DDevice= iface;
    IWineD3DDevice_AddRef(iface);
    object->resource.parent       = parent;
    object->resource.resourceType = D3DRTYPE_VERTEXBUFFER;
    object->resource.ref          = 1;
    object->allocatedMemory       = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    object->currentDesc.Usage     = Usage;
    object->currentDesc.Pool      = Pool;
    object->currentDesc.FVF       = FVF;
    object->currentDesc.Size      = Size;

    TRACE("(%p) : Size=%d, Usage=%ld, FVF=%lx, Pool=%d - Memory@%p, Iface@%p\n", This, Size, Usage, FVF, Pool, object->allocatedMemory, object);
    *ppVertexBuffer = (IWineD3DVertexBuffer *)object;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateIndexBuffer(IWineD3DDevice *iface, UINT Length, DWORD Usage, 
                                                    D3DFORMAT Format, D3DPOOL Pool, IWineD3DIndexBuffer** ppIndexBuffer,
                                                    HANDLE *sharedHandle, IUnknown *parent) {
    IWineD3DIndexBufferImpl *object;
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DIndexBufferImpl));
    object->lpVtbl = &IWineD3DIndexBuffer_Vtbl;
    object->resource.wineD3DDevice = iface;
    object->resource.resourceType  = D3DRTYPE_INDEXBUFFER;
    object->resource.parent        = parent;
    IWineD3DDevice_AddRef(iface);
    object->resource.ref = 1;
    object->allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);
    object->currentDesc.Usage = Usage;
    object->currentDesc.Pool  = Pool;
    object->currentDesc.Format= Format;
    object->currentDesc.Size  = Length;

    TRACE("(%p) : Len=%d, Use=%lx, Format=(%u,%s), Pool=%d - Memory@%p, Iface@%p\n", This, Length, Usage, Format, 
                           debug_d3dformat(Format), Pool, object, object->allocatedMemory);
    *ppIndexBuffer = (IWineD3DIndexBuffer *) object;

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_CreateStateBlock(IWineD3DDevice* iface, D3DSTATEBLOCKTYPE Type, IWineD3DStateBlock** ppStateBlock, IUnknown *parent) {
  
    IWineD3DDeviceImpl     *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DStateBlockImpl *object;
  
    /* Allocate Storage for the state block */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IWineD3DStateBlockImpl));
    object->lpVtbl        = &IWineD3DStateBlock_Vtbl;
    object->wineD3DDevice = iface;
    IWineD3DDevice_AddRef(iface);
    object->parent        = parent;
    object->ref           = 1;
    object->blockType     = Type;
    *ppStateBlock         = (IWineD3DStateBlock *)object;

    /* Special case - Used during initialization to produce a placeholder stateblock
          so other functions called can update a state block                         */
    if (Type == (D3DSTATEBLOCKTYPE) 0) {
        /* Don't bother increasing the reference count otherwise a device will never
           be freed due to circular dependencies                                   */
        return D3D_OK;
    }

    /* Otherwise, might as well set the whole state block to the appropriate values */
    IWineD3DDevice_AddRef(iface);
    memcpy(object, This->stateBlock, sizeof(IWineD3DStateBlockImpl));
    FIXME("unfinished - needs to set up changed and set attributes\n");
    return D3D_OK;
}

/*****
 * Get / Set FVF
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetFVF(IWineD3DDevice *iface, DWORD fvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Update the current statte block */
    This->updateStateBlock->fvf              = fvf;
    This->updateStateBlock->changed.fvf      = TRUE;
    This->updateStateBlock->set.fvf          = TRUE;

    TRACE("(%p) : FVF Shader FVF set to %lx\n", This, fvf);
    
    /* No difference if recording or not */
    return D3D_OK;
}
HRESULT WINAPI IWineD3DDeviceImpl_GetFVF(IWineD3DDevice *iface, DWORD *pfvf) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : GetFVF returning %lx\n", This, This->stateBlock->fvf);
    *pfvf = This->stateBlock->fvf;
    return D3D_OK;
}

/*****
 * Get / Set Stream Source
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_SetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride) {
    IWineD3DDeviceImpl       *This = (IWineD3DDeviceImpl *)iface;
    IWineD3DVertexBuffer     *oldSrc;

    oldSrc = This->stateBlock->stream_source[StreamNumber];
    TRACE("(%p) : StreamNo: %d, OldStream (%p), NewStream (%p), NewStride %d\n", This, StreamNumber, oldSrc, pStreamData, Stride);

    This->updateStateBlock->changed.stream_source[StreamNumber] = TRUE;
    This->updateStateBlock->set.stream_source[StreamNumber]     = TRUE;
    This->updateStateBlock->stream_stride[StreamNumber]         = Stride;
    This->updateStateBlock->stream_source[StreamNumber]         = pStreamData;
    This->updateStateBlock->stream_offset[StreamNumber]         = OffsetInBytes;

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        return D3D_OK;
    }

    /* Not recording... */
    if (oldSrc != NULL) IWineD3DVertexBuffer_Release(oldSrc);
    if (pStreamData != NULL) IWineD3DVertexBuffer_AddRef(pStreamData);

    return D3D_OK;
}

HRESULT WINAPI IWineD3DDeviceImpl_GetStreamSource(IWineD3DDevice *iface, UINT StreamNumber,IWineD3DVertexBuffer** pStream, UINT *pOffset, UINT* pStride) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    TRACE("(%p) : StreamNo: %d, Stream (%p), Stride %d\n", This, StreamNumber, This->stateBlock->stream_source[StreamNumber], This->stateBlock->stream_stride[StreamNumber]);
    *pStream = This->stateBlock->stream_source[StreamNumber];
    *pStride = This->stateBlock->stream_stride[StreamNumber];
    *pOffset = This->stateBlock->stream_offset[StreamNumber];
    IWineD3DVertexBuffer_AddRef(*pStream); /* We have created a new reference to the VB */
    return D3D_OK;
}

/*****
 * Get / Set Transform
 *****/
HRESULT  WINAPI  IWineD3DDeviceImpl_SetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE d3dts, CONST D3DMATRIX* lpmatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    /* Most of this routine, comments included copied from ddraw tree initially: */
    TRACE("(%p) : Transform State=%d\n", This, d3dts);

    /* Handle recording of state blocks */
    if (This->isRecordingState) {
        TRACE("Recording... not performing anything\n");
        This->updateStateBlock->changed.transform[d3dts] = TRUE;
        This->updateStateBlock->set.transform[d3dts]     = TRUE;
        memcpy(&This->updateStateBlock->transforms[d3dts], lpmatrix, sizeof(D3DMATRIX));
        return D3D_OK;
    }

    /*
     * If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters.
     */
    if (!memcmp(&This->stateBlock->transforms[d3dts].u.m[0][0], lpmatrix, sizeof(D3DMATRIX))) {
        TRACE("The app is setting the same matrix over again\n");
        return D3D_OK;
    } else {
        conv_mat(lpmatrix, &This->stateBlock->transforms[d3dts].u.m[0][0]);
    }

    /*
       ScreenCoord = ProjectionMat * ViewMat * WorldMat * ObjectCoord
       where ViewMat = Camera space, WorldMat = world space.

       In OpenGL, camera and world space is combined into GL_MODELVIEW
       matrix.  The Projection matrix stay projection matrix. 
     */

    /* Capture the times we can just ignore the change for now */
    if (d3dts == D3DTS_WORLDMATRIX(0)) {
        This->modelview_valid = FALSE;
        return D3D_OK;

    } else if (d3dts == D3DTS_PROJECTION) {
        This->proj_valid = FALSE;
        return D3D_OK;

    } else if (d3dts >= D3DTS_WORLDMATRIX(1) && d3dts <= D3DTS_WORLDMATRIX(255)) { 
        /* Indexed Vertex Blending Matrices 256 -> 511  */
        /* Use arb_vertex_blend or NV_VERTEX_WEIGHTING? */
        FIXME("D3DTS_WORLDMATRIX(1..255) not handled\n");
        return D3D_OK;
    } 
    
    /* Now we really are going to have to change a matrix */
    ENTER_GL();

    if (d3dts >= D3DTS_TEXTURE0 && d3dts <= D3DTS_TEXTURE7) { /* handle texture matrices */
        if (d3dts < GL_LIMITS(textures)) {
            int tex = d3dts - D3DTS_TEXTURE0;
            GL_ACTIVETEXTURE(tex);
#if 0 /* TODO: */
            set_texture_matrix((float *)lpmatrix, 
                               This->updateStateBlock->texture_state[tex][D3DTSS_TEXTURETRANSFORMFLAGS]);
#endif
        }

    } else if (d3dts == D3DTS_VIEW) { /* handle the VIEW matrice */

#if 0 /* TODO: */
        unsigned int k;
#endif

        /* If we are changing the View matrix, reset the light and clipping planes to the new view   
         * NOTE: We have to reset the positions even if the light/plane is not currently
         *       enabled, since the call to enable it will not reset the position.                 
         * NOTE2: Apparently texture transforms do NOT need reapplying
         */
        
        This->modelview_valid = FALSE;
        This->view_ident = !memcmp(lpmatrix, identity, 16*sizeof(float));
#if 0 /* TODO: */
        PLIGHTINFOEL *lightChain = NULL;
#endif
        glMatrixMode(GL_MODELVIEW);
        checkGLcall("glMatrixMode(GL_MODELVIEW)");
        glPushMatrix();
        glLoadMatrixf((float *)lpmatrix);
        checkGLcall("glLoadMatrixf(...)");

#if 0 /* TODO: */
        /* Reset lights */
        lightChain = This->StateBlock->lights;
        while (lightChain && lightChain->glIndex != -1) {
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_POSITION, lightChain->lightPosn);
            checkGLcall("glLightfv posn");
            glLightfv(GL_LIGHT0 + lightChain->glIndex, GL_SPOT_DIRECTION, lightChain->lightDirn);
            checkGLcall("glLightfv dirn");
            lightChain = lightChain->next;
        }
        /* Reset Clipping Planes if clipping is enabled */
        for (k = 0; k < GL_LIMITS(clipplanes); k++) {
            glClipPlane(GL_CLIP_PLANE0 + k, This->StateBlock->clipplane[k]);
            checkGLcall("glClipPlane");
        }
#endif
        glPopMatrix();

    } else { /* What was requested!?? */
        WARN("invalid matrix specified: %i\n", d3dts);
    }

    /* Release lock, all done */
    LEAVE_GL();
    return D3D_OK;

}
HRESULT WINAPI IWineD3DDeviceImpl_GetTransform(IWineD3DDevice *iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : for Transform State %d\n", This, State);
    memcpy(pMatrix, &This->stateBlock->transforms[State], sizeof(D3DMATRIX));
    return D3D_OK;
}

/*****
 * Scene related functions
 *****/
HRESULT WINAPI IWineD3DDeviceImpl_BeginScene(IWineD3DDevice *iface) {
    /* At the moment we have no need for any functionality at the beginning
       of a scene                                                          */
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : stub\n", This);
    return D3D_OK;
}

/**********************************************************
 * IUnknown parts follows
 **********************************************************/

HRESULT WINAPI IWineD3DDeviceImpl_QueryInterface(IWineD3DDevice *iface,REFIID riid,LPVOID *ppobj)
{
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DDeviceImpl_AddRef(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    TRACE("(%p) : AddRef increasing from %ld\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI IWineD3DDeviceImpl_Release(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %ld\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        IWineD3DStateBlock_Release((IWineD3DStateBlock *)This->stateBlock);
        IWineD3D_Release(This->wineD3D);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/**********************************************************
 * IWineD3DDevice VTbl follows
 **********************************************************/

IWineD3DDeviceVtbl IWineD3DDevice_Vtbl =
{
    IWineD3DDeviceImpl_QueryInterface,
    IWineD3DDeviceImpl_AddRef,
    IWineD3DDeviceImpl_Release,
    IWineD3DDeviceImpl_GetParent,
    IWineD3DDeviceImpl_CreateVertexBuffer,
    IWineD3DDeviceImpl_CreateIndexBuffer,
    IWineD3DDeviceImpl_CreateStateBlock,
    IWineD3DDeviceImpl_SetFVF,
    IWineD3DDeviceImpl_GetFVF,
    IWineD3DDeviceImpl_SetStreamSource,
    IWineD3DDeviceImpl_GetStreamSource,
    IWineD3DDeviceImpl_SetTransform,
    IWineD3DDeviceImpl_GetTransform,
    IWineD3DDeviceImpl_BeginScene
};
