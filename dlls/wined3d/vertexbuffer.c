/*
 * IWineD3DVertexBuffer Implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* *******************************************
   IWineD3DVertexBuffer IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexBufferImpl_QueryInterface(IWineD3DVertexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DVertexBuffer)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_AddRef(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %ld\n", This, ref + 1);
    if (ref == 0) {

        if(This->vbo) {
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
        }

        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVertexBuffer IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DVertexBufferImpl_GetDevice(IWineD3DVertexBuffer *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_SetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_FreePrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_SetPriority(IWineD3DVertexBuffer *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_GetPriority(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

static void     WINAPI IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    TRACE("(%p)->()\n", This);

    if(This->Flags & VBFLAG_LOAD) {
        return; /* Already doing that stuff */
    }

    if(!This->resource.allocatedMemory) {
        TRACE("Locking directly into VBO, nothing to do\n");
        return; /* Direct lock into the VBO */
    }

    if(This->vbo) {
        WineDirect3DVertexStridedData strided;
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
        BOOL useVertexShaderFunction = FALSE, fixup = FALSE;
        BYTE *data;
        UINT i;
        DWORD declFVF;  /* Not interested */
        UINT start = 0, end = 0, stride = 0;

        if(This->Flags & VBFLAG_DIRTY) {
            /* Update the old buffer on unlock, use the old desc */
            start = This->dirtystart;
            end = This->dirtyend;
            memcpy(&strided, &This->strided, sizeof(strided));

            if     (strided.u.s.position.dwStride) stride = strided.u.s.position.dwStride;
            else if(strided.u.s.specular.dwStride) stride = strided.u.s.specular.dwStride;
            else if(strided.u.s.diffuse.dwStride)  stride = strided.u.s.diffuse.dwStride;
            else {
                /* That means that there is nothing to fixup, just override previously modified data */
                fixup = FALSE;
            }
            if(stride) fixup = TRUE;
        } else {
            /* Keep this in sync with drawPrimitive in drawprim.c */
            if (device->stateBlock->vertexShader != NULL && wined3d_settings.vs_mode != VS_NONE 
                    &&((IWineD3DVertexShaderImpl *)device->stateBlock->vertexShader)->baseShader.function != NULL
                    && GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
                /* Using shaders? No conversion needed, the shaders handle this */
                TRACE("Using vertex shaders, not doing any vertex conversion\n");
                ENTER_GL();
                GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
                checkGLcall("glBindBufferARB");
                GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, This->resource.size, This->resource.allocatedMemory));
                checkGLcall("glBufferSubDataARB");
                LEAVE_GL();
                /* Lock directly into the VBO in the future */
                HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
                This->resource.allocatedMemory = NULL;
                This->Flags &= ~VBFLAG_DIRTY;
                return;
            }

            /* The code below reads the FVF / Vertex Declaration to find out which bits we have to convert
            * Basically I can't see any reason why it can't change from DrawPrimitive to DrawPrimitive call
            * from the DX api, but I think no sane game will do that. Reading the vertex declaration is quite
            * complex, and we should save as much CPU time as possible. So read it only once ans assume that
            * it doesn't change silently. I expect Windows D3D drivers to depend on that too
            */
            if(This->Flags & VBFLAG_HASDESC) return;

            /* Check against updated declarations */
            memset(&strided, 0, sizeof(strided));

            if(device->stateBlock->vertexDecl != NULL) {
                /* Check against the stream offset and make sure it is 0 */

                This->Flags |= VBFLAG_LOAD;
                primitiveDeclarationConvertToStridedData((IWineD3DDevice *) device,
                                                        useVertexShaderFunction,
                                                        &strided,
                                                        0,
                                                        &declFVF,
                                                        &fixup);
                This->Flags &= ~VBFLAG_LOAD;

                /* Only take care for stuff that is in this buffer, well, only the stuff that is interesting */
                if(strided.u.s.position.VBO != This->vbo)    memset(&strided.u.s.position, 0, sizeof(strided.u.s.position));
                if(strided.u.s.diffuse.VBO != This->vbo)     memset(&strided.u.s.diffuse, 0, sizeof(strided.u.s.diffuse));
                if(strided.u.s.specular.VBO != This->vbo)    memset(&strided.u.s.specular, 0, sizeof(strided.u.s.specular));
                if(strided.u.s.position2.VBO != This->vbo)   memset(&strided.u.s.position2, 0, sizeof(strided.u.s.position2));
            } else {
                if(!(This->Flags & VBFLAG_STREAM) ) {
                    TRACE("No vertex decl used and buffer is not bound to a stream, nothing to do\n");
                    return;
                }

                This->Flags |= VBFLAG_LOAD;
                primitiveConvertFVFtoOffset(device->stateBlock->fvf,
                                            device->stateBlock->streamStride[This->stream],
                                            NULL,
                                            &strided,
                                            This->vbo);
                This->Flags &= ~VBFLAG_LOAD;
            }

            /* If any data that needs conversion has changed we have to reload the whole buffer */
            if( ( (This->strided.u.s.position.dwType != WINED3DDECLTYPE_FLOAT4 || strided.u.s.position.dwType != WINED3DDECLTYPE_FLOAT4) &&
                  This->strided.u.s.position.lpData != strided.u.s.position.lpData) ||
                !(This->strided.u.s.diffuse.lpData == strided.u.s.diffuse.lpData || strided.u.s.diffuse.VBO != This->vbo)   ||
                !(This->strided.u.s.specular.lpData == strided.u.s.specular.lpData || strided.u.s.specular.VBO != This->vbo) ) {

                start = 0;
                end = This->resource.size;
                fixup = TRUE;

                if     (strided.u.s.position.dwStride) stride = strided.u.s.position.dwStride;
                else if(strided.u.s.specular.dwStride) stride = strided.u.s.specular.dwStride;
                else if(strided.u.s.diffuse.dwStride)  stride = strided.u.s.diffuse.dwStride;
                else {
                    /* That means that there is nothing to fixup, just override previously modified data */
                    fixup = FALSE;
                }

                memcpy(&This->strided, &strided, sizeof(strided));
            } else {
                TRACE("No declaration change\n");
                /* nothing to do - the old data is correct*/
                return;
            }
            This->Flags |= VBFLAG_HASDESC;
        }

        if(end == 0) {
            TRACE("Buffer not dirty, nothing to do\n");
            This->Flags &= ~VBFLAG_DIRTY;
            return;
        }

        TRACE("Loading buffer\n");
        if(fixup) {
            data = HeapAlloc(GetProcessHeap(), 0, end-start);
            if(!data) {
                ERR("Out of memory\n");
                return;
            }
            memcpy(data, This->resource.allocatedMemory + start, end - start);

            for(i = 0; i < ( end - start) / stride; i++) {
                if(strided.u.s.position.dwType == WINED3DDECLTYPE_FLOAT4 ) {
                    float *p = (float *) (((int) This->resource.allocatedMemory + (int) strided.u.s.position.lpData) + start + i * stride);
                    float x, y, z, w;

                    /* rhw conversion like in drawStridedSlow */
                    if(p[3] == 1.0 || ((p[3] < eps) && (p[3] > -eps))) {
                        x = p[0];
                        y = p[1];
                        z = p[2];
                        w = 1.0;
                    } else {
                        w = 1.0 / p[3];
                        x = p[0] * w;
                        y = p[1] * w;
                        z = p[2] * w;
                    }
                    p = (float *) ((int) data + i * stride + (int) strided.u.s.position.lpData);
                    p[0] = x;
                    p[1] = y;
                    p[2] = z;
                    p[3] = w;
                }
                if(strided.u.s.diffuse.dwType == WINED3DDECLTYPE_SHORT4 || strided.u.s.diffuse.dwType == WINED3DDECLTYPE_D3DCOLOR) {
                    DWORD srcColor, *dstColor = (DWORD *) (data + i * stride + (int) strided.u.s.diffuse.lpData);
                    srcColor = * (DWORD *) ( ((int) This->resource.allocatedMemory + (int) strided.u.s.diffuse.lpData) + start + i * stride);

                    /* Color conversion like in drawStridedSlow. watch out for little endianity
                     * If we want that stuff to work on big endian machines too we have to consider more things
                     *
                     * 0xff000000: Alpha mask
                     * 0x00ff0000: Blue mask
                     * 0x0000ff00: Green mask
                     * 0x000000ff: Red mask
                     */

                    *dstColor = 0;
                    *dstColor |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
                    *dstColor |= (srcColor & 0x00ff0000) >> 16;   /* Red */
                    *dstColor |= (srcColor & 0x000000ff) << 16;   /* Blue */
                } else if (strided.u.s.diffuse.lpData != NULL) {
                    FIXME("Type is %ld\n", strided.u.s.diffuse.dwType);
                }
                if(strided.u.s.specular.dwType == WINED3DDECLTYPE_SHORT4 || strided.u.s.specular.dwType == WINED3DDECLTYPE_D3DCOLOR) {
                    DWORD srcColor, *dstColor = (DWORD *) (data + i * stride + (int) strided.u.s.specular.lpData);
                    srcColor = * (DWORD *) ( ((int) This->resource.allocatedMemory + (int) strided.u.s.specular.lpData) + start + i * stride);

                    /* Color conversion like in drawStridedSlow. watch out for little endianity
                     * If we want that stuff to work on big endian machines too we have to consider more things
                     */
                    *dstColor = 0;
                    *dstColor |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
                    *dstColor |= (srcColor & 0x00ff0000) >> 16;   /* Red */
                    *dstColor |= (srcColor & 0x000000ff) << 16;   /* Blue */
                }
            }
        } else {
            data = This->resource.allocatedMemory + start;
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end - start, data));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();

        if(fixup) {
            HeapFree(GetProcessHeap(), 0, data);
        } else if(This->Flags & VBFLAG_HASDESC) {
            /* Free the allocated memory, then Lock will directly lock into the
             * VBO the next time :-)
             */
            HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
            This->resource.allocatedMemory = NULL;
        }
    }
    This->Flags &= ~VBFLAG_DIRTY;
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVertexBufferImpl_GetType(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetParent(IWineD3DVertexBuffer *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVertexBuffer IWineD3DVertexBuffer parts follow
   ****************************************************** */
static HRESULT  WINAPI IWineD3DVertexBufferImpl_Lock(IWineD3DVertexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    BYTE *data;
    TRACE("(%p)->%d, %d, %p, %08lx\n", This, OffsetToLock, SizeToLock, ppbData, Flags);

    InterlockedIncrement(&This->lockcount);

    if(This->Flags & VBFLAG_DIRTY) {
        if(This->dirtystart > OffsetToLock) This->dirtystart = OffsetToLock;
        if(SizeToLock) {
            if(This->dirtyend < OffsetToLock + SizeToLock) This->dirtyend = OffsetToLock + SizeToLock;
        } else {
            This->dirtyend = This->resource.size;
        }
    } else {
        This->dirtystart = OffsetToLock;
        if(SizeToLock)
            This->dirtyend = OffsetToLock + SizeToLock;
        else
            This->dirtyend = OffsetToLock + This->resource.size;
    }

    if(This->resource.allocatedMemory) {
          data = This->resource.allocatedMemory;
          This->Flags |= VBFLAG_DIRTY;
    } else {
        GLenum mode = GL_READ_WRITE_ARB;
        /* Return data to the VBO */

        TRACE("Locking directly into the buffer\n");

        if((This->resource.usage & WINED3DUSAGE_WRITEONLY) || ( Flags & D3DLOCK_DISCARD) ) {
            mode = GL_WRITE_ONLY_ARB;
        } else if( Flags & (D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE) ) {
            mode = GL_READ_ONLY_ARB;
        }

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        data = GL_EXTCALL(glMapBufferARB(GL_ARRAY_BUFFER_ARB, mode));
        LEAVE_GL();
        if(!data) {
            ERR("glMapBuffer failed\n");
            return WINED3DERR_INVALIDCALL;
        }
    }
    *ppbData = data + OffsetToLock;

    TRACE("(%p) : returning memory of %p (base:%p,offset:%u)\n", This, data + OffsetToLock, data, OffsetToLock);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */
    return WINED3D_OK;
}
HRESULT  WINAPI IWineD3DVertexBufferImpl_Unlock(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    LONG lockcount;
    TRACE("(%p)\n", This);

    lockcount = InterlockedDecrement(&This->lockcount);
    if(lockcount > 0) {
        /* Delay loading the buffer until everything is unlocked */
        TRACE("Ignoring the unlock\n");
        return D3D_OK;
    }

    if(!This->resource.allocatedMemory) {
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glUnmapBufferARB(GL_ARRAY_BUFFER_ARB));
        checkGLcall("glUnmapBufferARB");
        LEAVE_GL();
    } else {
        IWineD3DVertexBufferImpl_PreLoad(iface);
    }
    return WINED3D_OK;
}
static HRESULT  WINAPI IWineD3DVertexBufferImpl_GetDesc(IWineD3DVertexBuffer *iface, WINED3DVERTEXBUFFER_DESC *pDesc) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->resource.format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->resource.usage;
    pDesc->Pool   = This->resource.pool;
    pDesc->Size   = This->resource.size;
    pDesc->FVF    = This->fvf;
    return WINED3D_OK;
}

const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexBufferImpl_QueryInterface,
    IWineD3DVertexBufferImpl_AddRef,
    IWineD3DVertexBufferImpl_Release,
    /* IWineD3DResource */
    IWineD3DVertexBufferImpl_GetParent,
    IWineD3DVertexBufferImpl_GetDevice,
    IWineD3DVertexBufferImpl_SetPrivateData,
    IWineD3DVertexBufferImpl_GetPrivateData,
    IWineD3DVertexBufferImpl_FreePrivateData,
    IWineD3DVertexBufferImpl_SetPriority,
    IWineD3DVertexBufferImpl_GetPriority,
    IWineD3DVertexBufferImpl_PreLoad,
    IWineD3DVertexBufferImpl_GetType,
    /* IWineD3DVertexBuffer */
    IWineD3DVertexBufferImpl_Lock,
    IWineD3DVertexBufferImpl_Unlock,
    IWineD3DVertexBufferImpl_GetDesc
};

BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset, GLint *vbo) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    *vbo = This->vbo;
    if(This->vbo == 0) {
        return This->resource.allocatedMemory + iOffset;
    } else {
        return (BYTE *) iOffset;
    }
}

HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface) {
  return WINED3D_OK;
}
