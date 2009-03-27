/*
 * vertex declaration implementation
 *
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2004 Christian Costa
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_decl);

#define GLINFO_LOCATION This->wineD3DDevice->adapter->gl_info

static void dump_wined3dvertexelement(const WINED3DVERTEXELEMENT *element) {
    TRACE("     Stream: %d\n", element->Stream);
    TRACE("     Offset: %d\n", element->Offset);
    TRACE("       Type: %s (%#x)\n", debug_d3ddecltype(element->Type), element->Type);
    TRACE("     Method: %s (%#x)\n", debug_d3ddeclmethod(element->Method), element->Method);
    TRACE("      Usage: %s (%#x)\n", debug_d3ddeclusage(element->Usage), element->Usage);
    TRACE("Usage index: %d\n", element->UsageIndex);
    TRACE("   Register: %d\n", element->Reg);
}

/* *******************************************
   IWineD3DVertexDeclaration IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexDeclarationImpl_QueryInterface(IWineD3DVertexDeclaration *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DVertexDeclaration)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexDeclarationImpl_AddRef(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IWineD3DVertexDeclarationImpl_Release(IWineD3DVertexDeclaration *iface) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->ref);
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
        if(iface == This->wineD3DDevice->stateBlock->vertexDecl) {
            /* See comment in PixelShader::Release */
            IWineD3DDeviceImpl_MarkStateDirty(This->wineD3DDevice, STATE_VDECL);
        }

        HeapFree(GetProcessHeap(), 0, This->elements);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* *******************************************
   IWineD3DVertexDeclaration parts follow
   ******************************************* */

static HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetParent(IWineD3DVertexDeclaration *iface, IUnknown** parent){
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;

    *parent= This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexDeclarationImpl_GetDevice(IWineD3DVertexDeclaration *iface, IWineD3DDevice** ppDevice) {
    IWineD3DVertexDeclarationImpl *This = (IWineD3DVertexDeclarationImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->wineD3DDevice);

    *ppDevice = (IWineD3DDevice *) This->wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);

    return WINED3D_OK;
}

static BOOL declaration_element_valid_ffp(const WINED3DVERTEXELEMENT *element)
{
    switch(element->Usage)
    {
        case WINED3DDECLUSAGE_POSITION:
        case WINED3DDECLUSAGE_POSITIONT:
            switch(element->Type)
            {
                case WINED3DDECLTYPE_FLOAT2:
                case WINED3DDECLTYPE_FLOAT3:
                case WINED3DDECLTYPE_FLOAT4:
                case WINED3DDECLTYPE_SHORT2:
                case WINED3DDECLTYPE_SHORT4:
                case WINED3DDECLTYPE_FLOAT16_2:
                case WINED3DDECLTYPE_FLOAT16_4:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_BLENDWEIGHT:
            switch(element->Type)
            {
                case WINED3DDECLTYPE_FLOAT1:
                case WINED3DDECLTYPE_FLOAT2:
                case WINED3DDECLTYPE_FLOAT3:
                case WINED3DDECLTYPE_FLOAT4:
                case WINED3DDECLTYPE_D3DCOLOR:
                case WINED3DDECLTYPE_UBYTE4:
                case WINED3DDECLTYPE_SHORT2:
                case WINED3DDECLTYPE_SHORT4:
                case WINED3DDECLTYPE_FLOAT16_2:
                case WINED3DDECLTYPE_FLOAT16_4:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_NORMAL:
            switch(element->Type)
            {
                case WINED3DDECLTYPE_FLOAT3:
                case WINED3DDECLTYPE_FLOAT4:
                case WINED3DDECLTYPE_SHORT4:
                case WINED3DDECLTYPE_FLOAT16_4:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_TEXCOORD:
            switch(element->Type)
            {
                case WINED3DDECLTYPE_FLOAT1:
                case WINED3DDECLTYPE_FLOAT2:
                case WINED3DDECLTYPE_FLOAT3:
                case WINED3DDECLTYPE_FLOAT4:
                case WINED3DDECLTYPE_SHORT2:
                case WINED3DDECLTYPE_SHORT4:
                case WINED3DDECLTYPE_FLOAT16_2:
                case WINED3DDECLTYPE_FLOAT16_4:
                    return TRUE;
                default:
                    return FALSE;
            }

        case WINED3DDECLUSAGE_COLOR:
            switch(element->Type)
            {
                case WINED3DDECLTYPE_FLOAT3:
                case WINED3DDECLTYPE_FLOAT4:
                case WINED3DDECLTYPE_D3DCOLOR:
                case WINED3DDECLTYPE_UBYTE4:
                case WINED3DDECLTYPE_SHORT4:
                case WINED3DDECLTYPE_UBYTE4N:
                case WINED3DDECLTYPE_SHORT4N:
                case WINED3DDECLTYPE_USHORT4N:
                case WINED3DDECLTYPE_FLOAT16_4:
                    return TRUE;
                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

HRESULT vertexdeclaration_init(IWineD3DVertexDeclarationImpl *This,
        const WINED3DVERTEXELEMENT *elements, UINT element_count)
{
    HRESULT hr = WINED3D_OK;
    unsigned int i;
    char isPreLoaded[MAX_STREAMS];

    TRACE("(%p) : d3d version %d\n", This, ((IWineD3DImpl *)This->wineD3DDevice->wineD3D)->dxVersion);
    memset(isPreLoaded, 0, sizeof(isPreLoaded));

    if (TRACE_ON(d3d_decl)) {
        for (i = 0; i < element_count; ++i) {
            dump_wined3dvertexelement(elements+i);
        }
    }

    /* Skip the END element. */
    --element_count;

    This->element_count = element_count;
    This->elements = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->elements) * element_count);
    if (!This->elements)
    {
        ERR("Memory allocation failed\n");
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    /* Do some static analysis on the elements to make reading the declaration more comfortable
     * for the drawing code
     */
    This->num_streams = 0;
    This->position_transformed = FALSE;
    for (i = 0; i < element_count; ++i) {
        struct wined3d_vertex_declaration_element *e = &This->elements[i];

        e->type = elements[i].Type;
        e->ffp_valid = declaration_element_valid_ffp(&elements[i]);
        e->input_slot = elements[i].Stream;
        e->offset = elements[i].Offset;
        e->output_slot = elements[i].Reg;
        e->method = elements[i].Method;
        e->usage = elements[i].Usage;
        e->usage_idx = elements[i].UsageIndex;

        if (e->usage == WINED3DDECLUSAGE_POSITIONT) This->position_transformed = TRUE;

        /* Find the Streams used in the declaration. The vertex buffers have to be loaded
         * when drawing, but filter tesselation pseudo streams
         */
        if (e->input_slot >= MAX_STREAMS) continue;

        if (e->type == WINED3DDECLTYPE_UNUSED)
        {
            WARN("The application tries to use WINED3DDECLTYPE_UNUSED, returning E_FAIL\n");
            /* The caller will release the vdecl, which will free This->elements */
            return E_FAIL;
        }

        if (e->offset & 0x3)
        {
            WARN("Declaration element %u is not 4 byte aligned(%u), returning E_FAIL\n", i, e->offset);
            return E_FAIL;
        }

        if (!isPreLoaded[e->input_slot])
        {
            This->streams[This->num_streams] = e->input_slot;
            This->num_streams++;
            isPreLoaded[e->input_slot] = 1;
        }

        if (e->type == WINED3DDECLTYPE_FLOAT16_2 || e->type == WINED3DDECLTYPE_FLOAT16_4)
        {
            if (!GL_SUPPORT(NV_HALF_FLOAT)) This->half_float_conv_needed = TRUE;
        }
    }

    TRACE("Returning\n");
    return hr;
}

const IWineD3DVertexDeclarationVtbl IWineD3DVertexDeclaration_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexDeclarationImpl_QueryInterface,
    IWineD3DVertexDeclarationImpl_AddRef,
    IWineD3DVertexDeclarationImpl_Release,
    /* IWineD3DVertexDeclaration */
    IWineD3DVertexDeclarationImpl_GetParent,
    IWineD3DVertexDeclarationImpl_GetDevice,
};
