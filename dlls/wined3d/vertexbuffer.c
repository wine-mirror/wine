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
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

#define VB_MAXDECLCHANGES     100     /* After that number we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the changecount after that number of draws */

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
    TRACE("(%p) : AddRef increasing from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {

        if(This->vbo) {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
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

static inline void fixup_d3dcolor(DWORD *pos) {
    DWORD srcColor = *pos;

    /* Color conversion like in drawStridedSlow. watch out for little endianity
     * If we want that stuff to work on big endian machines too we have to consider more things
     *
     * 0xff000000: Alpha mask
     * 0x00ff0000: Blue mask
     * 0x0000ff00: Green mask
     * 0x000000ff: Red mask
     */
    *pos = 0;
    *pos |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
    *pos |= (srcColor & 0x00ff0000) >> 16;   /* Red */
    *pos |= (srcColor & 0x000000ff) << 16;   /* Blue */
}

static inline void fixup_transformed_pos(float *p) {
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
    p[0] = x;
    p[1] = y;
    p[2] = z;
    p[3] = w;
}

DWORD *find_conversion_shift(IWineD3DVertexBufferImpl *This, WineDirect3DVertexStridedData *strided, DWORD stride) {
    DWORD *ret, i, shift, j, type;
    DWORD orig_type_size;

    if(!stride) {
        TRACE("No shift\n");
        return NULL;
    }

    This->conv_stride = stride;
    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * stride);
    for(i = 0; i < MAX_ATTRIBS; i++) {
        type = strided->u.input[i].dwType;
        if(type == WINED3DDECLTYPE_FLOAT16_2) {
            shift = 4;
        } else if(type == WINED3DDECLTYPE_FLOAT16_4) {
            shift = 8;
        } else {
            shift = 0;
        }
        This->conv_stride += shift;

        if(shift) {
            orig_type_size = WINED3D_ATR_TYPESIZE(type) * WINED3D_ATR_SIZE(type);
            for(j = (DWORD_PTR) strided->u.input[i].lpData + orig_type_size; j < stride; j++) {
                ret[j] += shift;
            }
        }
    }

    if(TRACE_ON(d3d)) {
        TRACE("Dumping conversion shift:\n");
        for(i = 0; i < stride; i++) {
            TRACE("[%d]", ret[i]);
        }
        TRACE("\n");
    }
    return ret;
}

inline BOOL WINAPI IWineD3DVertexBufferImpl_FindDecl(IWineD3DVertexBufferImpl *This)
{
    WineDirect3DVertexStridedData strided;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL ret = FALSE;
    DWORD type_old, type_new, stride;
    int i;

    /* In d3d7 the vertex buffer declaration NEVER changes because it is stored in the d3d7 vertex buffer.
     * Once we have our declaration there is no need to look it up again.
     */
    if(((IWineD3DImpl *)device->wineD3D)->dxVersion == 7 && This->Flags & VBFLAG_HASDESC) {
        return FALSE;
    }

    /* There are certain vertex data types that need to be fixed up. The Vertex Buffers FVF doesn't
     * help finding them, only the vertex declaration or the device FVF can determine that at drawPrim
     * time. Rules are as follows:
     *
     * -> Fix up position1 and position 2 if they are XYZRHW
     * -> Fix up types that are D3DDECLTYPE_D3DCOLOR if no shader is used
     * -> TODO: Convert FLOAT16 to FLOAT32 if not supported natively by gl
     *
     * The Declaration is only known at drawing time, and it can change from draw to draw. If any converted values
     * are changed, the whole buffer has to be reconverted and reloaded. (Converting is SLOW, so if this happens too
     * often PreLoad stops converting entirely and falls back to drawStridedSlow).
     *
     * Reconvert if:
     * -> New semantics that have to be converted appear
     * -> The position of semantics that have to be converted changes
     * -> The stride of the vertex changed AND there is stuff that needs conversion
     * -> (If a vertex shader is bound and in use assume that nothing that needs conversion is there)
     *
     * Return values:
     *  TRUE: Reload is needed
     *  FALSE: otherwise
     */

    if (use_vs(device)) {
        if(!This->last_was_vshader && This->last_was_converted) {
            /* Reload if we're switching from converted fixed function to vertex shaders.
             * This isn't strictly needed, e.g. a FLOAT16 attribute could stay at the same
             * place. It is always needed for the FLOAT4 and D3DCOLOR conversions however, and
             * it is unlikely that any game uses FLOAT16s with fixed function vertex processing.
             */
            TRACE("Reconverting because switching from converted fixed function drawing to shaders\n");
            ret = TRUE;
        }
        This->last_was_vshader = TRUE;

        /* The vertex declaration is so nice to carry a flag for this. It takes the gl info into account,
         * but check that separately to avoid the memcmp over the structure
         */
        if(GL_SUPPORT(NV_HALF_FLOAT)) {
            memset(&strided, 0, sizeof(strided));
            ret = FALSE;
        } else if(((IWineD3DVertexDeclarationImpl *)device->stateBlock->vertexDecl)->half_float_conv_needed) {
            memcpy(&strided, &device->strided_streams, sizeof(strided));

            stride = 0;
            for(i = 0; i < MAX_ATTRIBS; i++) {
                if(strided.u.input[i].VBO != This->vbo) {
                    /* Ignore attributes from a different vbo */
                    memset(&strided.u.input[i], 0, sizeof(strided.u.input[i]));
                    continue;
                };

                type_old = This->strided.u.input[i].dwType;
                type_new = strided.u.input[i].dwType;
                if(type_old != type_new) {
                    if(type_old == WINED3DDECLTYPE_FLOAT16_2 || type_new == WINED3DDECLTYPE_FLOAT16_2 ||
                       type_old == WINED3DDECLTYPE_FLOAT16_4 || type_new == WINED3DDECLTYPE_FLOAT16_4) {
                        TRACE("Reloading because attribute %i was %s before and is %s now\n", i,
                              debug_d3ddecltype(type_old), debug_d3ddecltype(type_new));
                        stride = strided.u.input[i].dwStride;
                        ret = TRUE;
                    }
                } else if(type_new == WINED3DDECLTYPE_FLOAT16_2 || type_new == WINED3DDECLTYPE_FLOAT16_4) {
                    if(This->strided.u.input[i].lpData != strided.u.input[i].lpData) {
                        TRACE("Reconverting buffer because attribute %d has type %s and moved from offset %p to %p\n",
                              i, debug_d3ddecltype(type_new), This->strided.u.s.position.lpData, strided.u.s.position.lpData);
                        memcpy(&This->strided, &strided, sizeof(strided));
                        ret = TRUE;
                        stride = strided.u.input[i].dwStride;
                    }
                }

                if(!(type_new == WINED3DDECLTYPE_FLOAT16_2 || type_new == WINED3DDECLTYPE_FLOAT16_4)) {
                    /* Nuke unconverted attributes */
                    memset(&strided.u.input[i], 0, sizeof(strided.u.input[i]));
                }
            }
            if(ret) {
                memcpy(&This->strided, &strided, sizeof(strided));
                HeapFree(GetProcessHeap(), 0, This->conv_shift);
                This->conv_shift = find_conversion_shift(This, &This->strided, stride);
            }
        } else {
            /* No conversion*/
            memset(&strided, 0, sizeof(strided));
            ret = (memcmp(&strided, &This->strided, sizeof(strided)) != 0);
            if(ret) {
                HeapFree(GetProcessHeap(), 0, This->conv_shift);
                This->conv_shift = NULL;
            }
        }
    } else {
        /* This will need modifications of the loading code as well - not handled right now */
        if(((IWineD3DVertexDeclarationImpl *)device->stateBlock->vertexDecl)->half_float_conv_needed) {
            FIXME("Implement half float fixup with fixed function vertex processing\n");
        }

        /* we need a copy because we modify some params */
        memcpy(&strided, &device->strided_streams, sizeof(strided));

        /* Kill things that does not exist in our fixed function pipeline implementation */
        memset(&strided.u.s.blendWeights, 0, sizeof(strided.u.s.blendWeights));
        memset(&strided.u.s.blendMatrixIndices, 0, sizeof(strided.u.s.blendMatrixIndices));
        memset(&strided.u.s.pSize, 0, sizeof(strided.u.s.pSize));
        memset(&strided.u.s.position2, 0, sizeof(strided.u.s.position2));
        memset(&strided.u.s.normal2, 0, sizeof(strided.u.s.normal2));
        memset(&strided.u.s.tangent, 0, sizeof(strided.u.s.tangent));
        memset(&strided.u.s.binormal, 0, sizeof(strided.u.s.binormal));
        memset(&strided.u.s.tessFactor, 0, sizeof(strided.u.s.tessFactor));
        memset(&strided.u.s.fog, 0, sizeof(strided.u.s.fog));
        memset(&strided.u.s.depth, 0, sizeof(strided.u.s.depth));
        memset(&strided.u.s.sample, 0, sizeof(strided.u.s.sample));

        /* Filter out data that does not come from this VBO. No need to repeat that on
         * the attributes filtered above.
         */
#define FILTER_OTHER_VBO(name) if(strided.u.s.name.VBO != This->vbo) {memset(&strided.u.s.name, 0, sizeof(strided.u.s.name));}
        FILTER_OTHER_VBO(position);
        /*FILTER_OTHER_VBO(blendWeights);*/
        /*FILTER_OTHER_VBO(blendMatrixIndices);*/
        FILTER_OTHER_VBO(normal);
        /*FILTER_OTHER_VBO(pSize);*/
        FILTER_OTHER_VBO(diffuse);
        FILTER_OTHER_VBO(specular);
        FILTER_OTHER_VBO(texCoords[0]);
        FILTER_OTHER_VBO(texCoords[1]);
        FILTER_OTHER_VBO(texCoords[2]);
        FILTER_OTHER_VBO(texCoords[3]);
        FILTER_OTHER_VBO(texCoords[4]);
        FILTER_OTHER_VBO(texCoords[5]);
        FILTER_OTHER_VBO(texCoords[6]);
        FILTER_OTHER_VBO(texCoords[7]);
        /*FILTER_OTHER_VBO(position2);*/
        /*FILTER_OTHER_VBO(normal2);*/
        /*FILTER_OTHER_VBO(tangent);*/
        /*FILTER_OTHER_VBO(binormal);*/
        /*FILTER_OTHER_VBO(tessFactor);*/
        /*FILTER_OTHER_VBO(fog);*/
        /*FILTER_OTHER_VBO(depth);*/
        /*FILTER_OTHER_VBO(sample);*/
#undef FILTER_OTHER_VBO

        /* Now find out if anything has changed. compared to the last run. Keep a few things in mind:
         *
         * 1) We do not mind if types change that do not need conversion
         *
         * 2) If some data exists cannot be found out by the data field alone. Data can appear at offset 0,
         * so the main identification is the type. Note that D3DDECLTYPE_FLOAT1 is defined to 0, watch out
         * for this if any semantic needs conversion if it is declared with that
         *
         * 3) If the type is the same, and it needs conversion, make sure it stayed at the same place. Moving
         * converted bytes requires a reconversion of the whole buffer
         *
         * 4) The stride is not a reliable indicator for existance, as it can be 0 if an attribute stays static
         *
         * 5) If we used a vertex buffer before, and we aren't using one now(wouldn't be in this codepath
         * otherwise) then we cannot compare the strided structures. Vertex shaders use numbered attributes,
         * fixed function pipeline uses named once. For example, a vertex shader could have used converted
         * tessFactor data which the code below ignores entirely. So if we used a vertex shader before, and
         * used conversion before, assume a decl change
         */
        if(This->last_was_vshader && This->last_was_converted) {
            TRACE("Reconverting because a vertex shader with conversion was used before\n");
            ret = TRUE;
            /* Still have to run through the code below to find the fixed function attribs that need
             * conversion
             */
        }
        do {
            /* Position: We have to convert FLOAT4 data because opengl divides the vertex by 1 / w, or
             * of course if the position has type D3DCOLOR
             */
            type_old = This->strided.u.s.position.dwType;
            type_new = strided.u.s.position.dwType;
            if(type_old != type_new) {
                if(type_old == WINED3DDECLTYPE_FLOAT4   || type_new == WINED3DDECLTYPE_FLOAT4 ||
                   type_old == WINED3DDECLTYPE_D3DCOLOR || type_new == WINED3DDECLTYPE_D3DCOLOR) {
                    TRACE("Reconverting buffer because POSITION type changed from %s to %s\n",
                          debug_d3ddecltype(type_old), debug_d3ddecltype(type_new));
                    memcpy(&This->strided, &strided, sizeof(strided));
                    ret = TRUE;
                    break;
                }
            } else if(type_new == WINED3DDECLTYPE_D3DCOLOR || type_new == WINED3DDECLTYPE_FLOAT4) {
                if(This->strided.u.s.position.lpData != strided.u.s.position.lpData) {
                    TRACE("Reconverting buffer because POSITION has type %s and moved from offset %p to %p\n",
                          debug_d3ddecltype(type_new), This->strided.u.s.position.lpData, strided.u.s.position.lpData);
                    memcpy(&This->strided, &strided, sizeof(strided));
                    ret = TRUE;
                    break;
                }
            }

            /* Others: D3DCOLOR needs conversion */
#define CHECK_D3DCOLOR_CONV(name) \
            type_old = This->strided.u.s.name.dwType; \
            type_new = strided.u.s.name.dwType; \
            if(type_old != type_new) { \
                if(type_old == WINED3DDECLTYPE_D3DCOLOR || type_new == WINED3DDECLTYPE_D3DCOLOR) { \
                    TRACE("Reconverting buffer because %s type changed from %s to %s\n", \
                           #name, debug_d3ddecltype(type_old), debug_d3ddecltype(type_new)); \
                    memcpy(&This->strided, &strided, sizeof(strided)); \
                    ret = TRUE; \
                    break; \
                } \
            } else if(type_new == WINED3DDECLTYPE_D3DCOLOR) { \
                if(This->strided.u.s.name.lpData != strided.u.s.name.lpData) { \
                    TRACE("Reconverting buffer because DIFFUSE has type %s and moved from offset %p to %p\n", \
                          debug_d3ddecltype(type_new), This->strided.u.s.name.lpData, strided.u.s.name.lpData); \
                    memcpy(&This->strided, &strided, sizeof(strided)); \
                    ret = TRUE; \
                    break; \
                } \
            }
            CHECK_D3DCOLOR_CONV(normal);
            CHECK_D3DCOLOR_CONV(diffuse);
            CHECK_D3DCOLOR_CONV(specular);
            CHECK_D3DCOLOR_CONV(texCoords[0]);
            CHECK_D3DCOLOR_CONV(texCoords[1]);
            CHECK_D3DCOLOR_CONV(texCoords[2]);
            CHECK_D3DCOLOR_CONV(texCoords[3]);
            CHECK_D3DCOLOR_CONV(texCoords[4]);
            CHECK_D3DCOLOR_CONV(texCoords[5]);
            CHECK_D3DCOLOR_CONV(texCoords[6]);
            CHECK_D3DCOLOR_CONV(texCoords[7]);
#undef CHECK_D3DCOLOR_CONV

            /* No conversion */
        } while(0);
        This->last_was_vshader = FALSE;
    }
    /* We have a declaration now in the buffer */
    This->Flags |= VBFLAG_HASDESC;

    return ret;
}

static void     WINAPI IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BYTE *data;
    UINT start = 0, end = 0, stride = 0, vertices;
    BOOL declChanged = FALSE;
    int i, j;
    TRACE("(%p)->()\n", This);

    if(This->Flags & VBFLAG_LOAD) {
        return; /* Already doing that stuff */
    }

    if(!This->vbo) {
        /* TODO: Make converting independent from VBOs */
        return; /* Not doing any conversion */
    }

    /* Reading the declaration makes only sense if the stateblock is finalized and the buffer bound to a stream */
    if(device->isInDraw && This->bindCount > 0) {
        declChanged = IWineD3DVertexBufferImpl_FindDecl(This);
    } else if(This->Flags & VBFLAG_HASDESC) {
        /* Reuse the declaration stored in the buffer. It will most likely not change, and if it does
         * the stream source state handler will call PreLoad again and the change will be cought
         */
    } else {
        /* Cannot get a declaration, and no declaration is stored in the buffer. It is pointless to preload
         * now. When the buffer is used, PreLoad will be called by the stream source state handler and a valid
         * declaration for the buffer can be found
         */
        return;
    }

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too much
     * of them(and thus stop converting)
     */
    if(declChanged) {
        WineDirect3DVertexStridedData zero;
        This->declChanges++;
        This->draws = 0;

        memset(&zero, 0, sizeof(zero));
        zero.u.s.position_transformed = This->strided.u.s.position_transformed;
        if(memcmp(&zero, &This->strided, sizeof(zero)) == 0) {
            This->last_was_converted = FALSE;
        } else {
            This->last_was_converted = TRUE;
        }

        if(This->declChanges > VB_MAXDECLCHANGES) {
            FIXME("Too much declaration changes, stopping converting\n");
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
            This->vbo = 0;
            HeapFree(GetProcessHeap(), 0, This->conv_shift);

            /* The stream source state handler might have read the memory of the vertex buffer already
             * and got the memory in the vbo which is not valid any longer. Dirtify the stream source
             * to force a reload. This happens only once per changed vertexbuffer and should occur rather
             * rarely
             */
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);

            return;
        }
    } else {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        This->draws++;
        if(This->draws > VB_RESETDECLCHANGE) This->declChanges = 0;
    }

    if(declChanged) {
        /* The declaration changed, reload the whole buffer */
        WARN("Reloading buffer because of decl change\n");
        start = 0;
        end = This->resource.size;
    } else if(This->Flags & VBFLAG_DIRTY) {
        /* No decl change, but dirty data, reload the changed stuff */
        start = This->dirtystart;
        end = This->dirtyend;
    } else {
        /* Desc not changed, buffer not dirty, nothing to do :-) */
        return;
    }

    /* Mark the buffer clean */
    This->Flags &= ~VBFLAG_DIRTY;
    This->dirtystart = 0;
    This->dirtyend = 0;

    if(!This->last_was_converted) {
        /* That means that there is nothing to fixup. Just upload from This->resource.allocatedMemory
         * directly into the vbo. Do not free the system memory copy because drawPrimitive may need it if
         * the stride is 0, for instancing emulation, vertex blending emulation or shader emulation.
         */
        TRACE("No conversion needed\n");

        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end-start, This->resource.allocatedMemory + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
        return;
    }

    /* OK, we have the original data from the app, the description of the buffer and the dirty area.
     * so convert the stuff
     */
    if(This->last_was_vshader) {
        TRACE("vertex-shadered conversion\n");
        /* TODO: Improve that */
        for(i = 0; i < MAX_ATTRIBS; i++) {
            if(This->strided.u.input[i].dwStride) {
                stride = This->strided.u.input[i].dwStride;
            }
        }
        TRACE("Found input stride %d, output stride %d\n", stride, This->conv_stride);
        /* For now reconvert the entire buffer */
        start = 0;
        end = This->resource.size;

        vertices = This->resource.size / stride;
        TRACE("%d vertices in buffer\n", vertices);
        if(This->vbo_size != vertices * This->conv_stride) {
            TRACE("Old size %d, creating new size %d\n", This->vbo_size, vertices * This->conv_stride);
            ENTER_GL();
            GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
            checkGLcall("glBindBufferARB");
            GL_EXTCALL(glBufferDataARB(GL_ARRAY_BUFFER_ARB, vertices * This->conv_stride, NULL, This->vbo_usage));
            This->vbo_size = vertices * This->conv_stride;
            checkGLcall("glBufferDataARB");
            LEAVE_GL();
        }
        data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, vertices * This->conv_stride);
        if(!data) {
            ERR("Out of memory\n");
            return;
        }

        /* Now for each vertex in the buffer */
        for(i = 0; i < vertices; i++) {
            /* Copy the vertex over, taking the shifts into account */
            for(j = 0; j < stride; j++) {
                data[This->conv_stride * i + j + This->conv_shift[j]] = This->resource.allocatedMemory[i * stride + j];
            }
            /* And convert FLOAT16s */
            for(j = 0; j < MAX_ATTRIBS; j++) {
                DWORD_PTR offset = (DWORD_PTR) This->strided.u.input[j].lpData;
                float *dest = (float *) (&data[This->conv_stride * i + offset + This->conv_shift[offset]]);
                WORD *in = (WORD *) (&This->resource.allocatedMemory[i * stride + offset]);

                switch(This->strided.u.input[j].dwType) {
                    case WINED3DDECLTYPE_FLOAT16_4:
                        dest[3] = float_16_to_32(in + 3);
                        dest[2] = float_16_to_32(in + 2);
                        /* drop through */
                    case WINED3DDECLTYPE_FLOAT16_2:
                        dest[1] = float_16_to_32(in + 1);
                        dest[0] = float_16_to_32(in + 0);
                        break;
                    default:
                        break;
                }
            }
        }
        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, This->vbo_size, data));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
    } else {
        data = HeapAlloc(GetProcessHeap(), 0, end-start);
        if(!data) {
            ERR("Out of memory\n");
            return;
        }
        memcpy(data, This->resource.allocatedMemory + start, end - start);

        if     (This->strided.u.s.position.dwStride)        stride = This->strided.u.s.position.dwStride;
        else if(This->strided.u.s.specular.dwStride)        stride = This->strided.u.s.specular.dwStride;
        else if(This->strided.u.s.diffuse.dwStride)         stride = This->strided.u.s.diffuse.dwStride;
        else if(This->strided.u.s.normal.dwStride)          stride = This->strided.u.s.normal.dwStride;
        else if(This->strided.u.s.texCoords[0].dwStride)    stride = This->strided.u.s.texCoords[0].dwStride;
        else if(This->strided.u.s.texCoords[1].dwStride)    stride = This->strided.u.s.texCoords[1].dwStride;
        else if(This->strided.u.s.texCoords[2].dwStride)    stride = This->strided.u.s.texCoords[2].dwStride;
        else if(This->strided.u.s.texCoords[3].dwStride)    stride = This->strided.u.s.texCoords[3].dwStride;
        else if(This->strided.u.s.texCoords[4].dwStride)    stride = This->strided.u.s.texCoords[4].dwStride;
        else if(This->strided.u.s.texCoords[5].dwStride)    stride = This->strided.u.s.texCoords[5].dwStride;
        else if(This->strided.u.s.texCoords[6].dwStride)    stride = This->strided.u.s.texCoords[6].dwStride;
        else if(This->strided.u.s.texCoords[7].dwStride)    stride = This->strided.u.s.texCoords[7].dwStride;

        for(i = (( end - start) / stride) - 1; i >= 0; i--) {
            if(This->strided.u.s.position.dwType == WINED3DDECLTYPE_FLOAT4) {
                fixup_transformed_pos((float *)((DWORD_PTR) data + ((DWORD_PTR) This->strided.u.s.position.lpData) + i * stride));
            } else if(This->strided.u.s.position.dwType == WINED3DDECLTYPE_D3DCOLOR) {
                FIXME("Write a test for D3DCOLOR position\n");
            }
#define CONVERT_D3DCOLOR_ATTRIB(name) \
                if(This->strided.u.s.name.dwType == WINED3DDECLTYPE_D3DCOLOR) { \
                    fixup_d3dcolor((DWORD *)((DWORD_PTR) data + ((DWORD_PTR) This->strided.u.s.name.lpData) + i * stride)); \
                }
            CONVERT_D3DCOLOR_ATTRIB(position);
            CONVERT_D3DCOLOR_ATTRIB(specular);
            CONVERT_D3DCOLOR_ATTRIB(diffuse);
            CONVERT_D3DCOLOR_ATTRIB(normal);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[0]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[1]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[2]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[3]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[4]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[5]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[6]);
            CONVERT_D3DCOLOR_ATTRIB(texCoords[7]);
#undef CONVERT_D3DCOLOR_ATTRIB
        }
        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end - start, data));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
    }

    HeapFree(GetProcessHeap(), 0, data);
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
    TRACE("(%p)->%d, %d, %p, %08x\n", This, OffsetToLock, SizeToLock, ppbData, Flags);

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
            This->dirtyend = This->resource.size;
    }

    data = This->resource.allocatedMemory;
    This->Flags |= VBFLAG_DIRTY;
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
        return WINED3D_OK;
    }

    if(This->Flags & VBFLAG_HASDESC) {
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
