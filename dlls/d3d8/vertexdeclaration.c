/*
 * IDirect3DVertexDeclaration8 implementation
 *
 * Copyright 2007 Henri Verbeet
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

/* IDirect3DVertexDeclaration8 is internal to our implementation.
 * It's not visible in the API. */

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IUnknown */
static HRESULT WINAPI IDirect3DVertexDeclaration8Impl_QueryInterface(IDirect3DVertexDeclaration8 *iface, REFIID riid, void **obj_ptr)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), obj_ptr);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IDirect3DVertexDeclaration8))
    {
        IUnknown_AddRef(iface);
        *obj_ptr = iface;
        return S_OK;
    }

    *obj_ptr = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_AddRef(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedIncrement(&This->ref_count);
    TRACE("(%p) : AddRef increasing to %d\n", This, ref_count);

    return ref_count;
}

static ULONG WINAPI IDirect3DVertexDeclaration8Impl_Release(IDirect3DVertexDeclaration8 *iface)
{
    IDirect3DVertexDeclaration8Impl *This = (IDirect3DVertexDeclaration8Impl *)iface;

    ULONG ref_count = InterlockedDecrement(&This->ref_count);
    TRACE("(%p) : Releasing to %d\n", This, ref_count);

    if (!ref_count) {
        IWineD3DVertexDeclaration_Release(This->wined3d_vertex_declaration);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref_count;
}

static const char *debug_d3dvsdt_type(D3DVSDT_TYPE d3dvsdt_type)
{
    switch (d3dvsdt_type)
    {
#define D3DVSDT_TYPE_TO_STR(u) case u: return #u
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT1);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT2);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT3);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_FLOAT4);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_D3DCOLOR);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_UBYTE4);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_SHORT2);
        D3DVSDT_TYPE_TO_STR(D3DVSDT_SHORT4);
#undef D3DVSDT_TYPE_TO_STR
        default:
            FIXME("Unrecognized D3DVSDT_TYPE %#x\n", d3dvsdt_type);
            return "unrecognized";
    }
}

static const char *debug_d3dvsde_register(D3DVSDE_REGISTER d3dvsde_register)
{
    switch (d3dvsde_register)
    {
#define D3DVSDE_REGISTER_TO_STR(u) case u: return #u
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_POSITION);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_BLENDWEIGHT);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_BLENDINDICES);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_NORMAL);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_PSIZE);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_DIFFUSE);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_SPECULAR);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD0);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD1);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD2);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD3);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD4);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD5);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD6);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_TEXCOORD7);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_POSITION2);
        D3DVSDE_REGISTER_TO_STR(D3DVSDE_NORMAL2);
#undef D3DVSDE_REGISTER_TO_STR
        default:
            FIXME("Unrecognized D3DVSDE_REGISTER %#x\n", d3dvsde_register);
            return "unrecognized";
    }
}

static size_t parse_token(const DWORD* pToken)
{
    const DWORD token = *pToken;
    size_t tokenlen = 1;

    switch ((token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT) { /* maybe a macro to inverse ... */
        case D3DVSD_TOKEN_NOP:
            TRACE(" 0x%08x NOP()\n", token);
            break;

        case D3DVSD_TOKEN_STREAM:
            if (token & D3DVSD_STREAMTESSMASK)
            {
                TRACE(" 0x%08x STREAM_TESS()\n", token);
            } else {
                TRACE(" 0x%08x STREAM(%u)\n", token, ((token & D3DVSD_STREAMNUMBERMASK) >> D3DVSD_STREAMNUMBERSHIFT));
            }
            break;

        case D3DVSD_TOKEN_STREAMDATA:
            if (token & 0x10000000)
            {
                TRACE(" 0x%08x SKIP(%u)\n", token, ((token & D3DVSD_SKIPCOUNTMASK) >> D3DVSD_SKIPCOUNTSHIFT));
            } else {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
                DWORD reg = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
                TRACE(" 0x%08x REG(%s, %s)\n", token, debug_d3dvsde_register(reg), debug_d3dvsdt_type(type));
            }
            break;

        case D3DVSD_TOKEN_TESSELLATOR:
            if (token & 0x10000000)
            {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)  >> D3DVSD_DATATYPESHIFT);
                DWORD reg = ((token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);
                TRACE(" 0x%08x TESSUV(%s) as %s\n", token, debug_d3dvsde_register(reg), debug_d3dvsdt_type(type));
            } else {
                DWORD type = ((token & D3DVSD_DATATYPEMASK)    >> D3DVSD_DATATYPESHIFT);
                DWORD regout = ((token & D3DVSD_VERTEXREGMASK)   >> D3DVSD_VERTEXREGSHIFT);
                DWORD regin = ((token & D3DVSD_VERTEXREGINMASK) >> D3DVSD_VERTEXREGINSHIFT);
                TRACE(" 0x%08x TESSNORMAL(%s, %s) as %s\n", token, debug_d3dvsde_register(regin),
                        debug_d3dvsde_register(regout), debug_d3dvsdt_type(type));
            }
            break;

        case D3DVSD_TOKEN_CONSTMEM:
            {
                DWORD count = ((token & D3DVSD_CONSTCOUNTMASK)   >> D3DVSD_CONSTCOUNTSHIFT);
                tokenlen = (4 * count) + 1;
            }
            break;

        case D3DVSD_TOKEN_EXT:
            {
                DWORD count = ((token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT);
                DWORD extinfo = ((token & D3DVSD_EXTINFOMASK)    >> D3DVSD_EXTINFOSHIFT);
                TRACE(" 0x%08x EXT(%u, %u)\n", token, count, extinfo);
                /* todo ... print extension */
                tokenlen = count + 1;
            }
            break;

        case D3DVSD_TOKEN_END:
            TRACE(" 0x%08x END()\n", token);
            break;

        default:
            TRACE(" 0x%08x UNKNOWN\n", token);
            /* argg error */
    }

    return tokenlen;
}

void load_local_constants(const DWORD *d3d8_elements, IWineD3DVertexShader *wined3d_vertex_shader)
{
    const DWORD *token = d3d8_elements;

    while (*token != D3DVSD_END())
    {
        if (((*token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT) == D3DVSD_TOKEN_CONSTMEM)
        {
            DWORD count = ((*token & D3DVSD_CONSTCOUNTMASK) >> D3DVSD_CONSTCOUNTSHIFT);
            DWORD constant_idx = ((*token & D3DVSD_CONSTADDRESSMASK) >> D3DVSD_CONSTADDRESSSHIFT);
            HRESULT hr;

            if (TRACE_ON(d3d8))
            {
                DWORD i;
                for (i = 0; i < count; ++i)
                {
                    TRACE("c[%u] = (%8f, %8f, %8f, %8f)\n",
                            constant_idx,
                            *(const float *)(token + i * 4 + 1),
                            *(const float *)(token + i * 4 + 2),
                            *(const float *)(token + i * 4 + 3),
                            *(const float *)(token + i * 4 + 4));
                }
            }
            hr = IWineD3DVertexShader_SetLocalConstantsF(wined3d_vertex_shader, constant_idx, (const float *)token+1, count);
            if (FAILED(hr)) ERR("Failed setting shader constants\n");
        }

        token += parse_token(token);
    }
}

const IDirect3DVertexDeclaration8Vtbl Direct3DVertexDeclaration8_Vtbl =
{
    IDirect3DVertexDeclaration8Impl_QueryInterface,
    IDirect3DVertexDeclaration8Impl_AddRef,
    IDirect3DVertexDeclaration8Impl_Release
};
