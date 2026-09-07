/*
 * Copyright 2021 Nikolay Sivov for CodeWeavers
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
 *
 */

#define COBJMACROS
#include "d3dx10.h"
#include "d3dcompiler.h"
#include <stdbool.h>
#include <stdint.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c
#define D3DX10_SPRITE_READY 0x10000000

struct vertex
{
    D3DXVECTOR4 pos;
    D3DXVECTOR4 texcoord;
    D3DXVECTOR4 color;
};

struct d3dx10_sprite
{
    ID3DX10Sprite ID3DX10Sprite_iface;
    LONG refcount;

    struct
    {
        D3DX10_SPRITE *sprites;
        size_t count;
        size_t capacity;
    } buffer;
    D3DXMATRIX projection;
    D3DXMATRIX view;
    ID3D10Device *device;
    ID3D10StateBlock *state_block;
    struct vertex *vertex_data;
    unsigned int batch_size;
    ID3D10InputLayout *input_layout;
    ID3D10PixelShader *pixel_shader;
    ID3D10VertexShader *vertex_shader;
    ID3D10SamplerState *sampler;
    ID3D10Buffer *ib;
    ID3D10Buffer *vb;
    ID3D10Buffer *vs_cb;
    unsigned int flags;
};

static bool d3dx_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / size;
    if (count > max_capacity)
        return false;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;
    return true;
}

static void d3dx10_sprite_clear_batch(struct d3dx10_sprite *sprite)
{
    if (sprite->flags & D3DX10_SPRITE_ADDREF_TEXTURES)
    {
        for (size_t i = 0; i < sprite->buffer.count; ++i)
        {
            if (sprite->buffer.sprites[i].pTexture)
                ID3D10ShaderResourceView_Release(sprite->buffer.sprites[i].pTexture);
        }
    }

    sprite->buffer.count = 0;
}

static void d3dx10_sprite_cleanup(struct d3dx10_sprite *sprite)
{
    if (sprite->input_layout)
        ID3D10InputLayout_Release(sprite->input_layout);
    if (sprite->pixel_shader)
        ID3D10PixelShader_Release(sprite->pixel_shader);
    if (sprite->vertex_shader)
        ID3D10VertexShader_Release(sprite->vertex_shader);
    if (sprite->sampler)
        ID3D10SamplerState_Release(sprite->sampler);
    if (sprite->device)
        ID3D10Device_Release(sprite->device);
    if (sprite->state_block)
        IUnknown_Release(sprite->state_block);
    if (sprite->ib)
        ID3D10Buffer_Release(sprite->ib);
    if (sprite->vb)
        ID3D10Buffer_Release(sprite->vb);
    if (sprite->vs_cb)
        ID3D10Buffer_Release(sprite->vs_cb);
    d3dx10_sprite_clear_batch(sprite);
    free(sprite->buffer.sprites);
    free(sprite->vertex_data);
}

static D3DX10_SPRITE * d3dx10_get_sprite_ptr(D3DX10_SPRITE *sprites, unsigned int index,
        unsigned int stride)
{
    return (D3DX10_SPRITE *)((char *)sprites + index * stride);
}

static D3DX10_SPRITE * d3dx10_sprite_draw_batch(struct d3dx10_sprite *sprite,
        D3DX10_SPRITE *sprites, unsigned int count, unsigned int stride)
{
    struct vertex *v = sprite->vertex_data;
    static const D3DXVECTOR4 quad[] =
    {
        {-0.5f, -0.5f, 0.0f, 1.0f},
        {-0.5f,  0.5f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.0f, 1.0f},
        { 0.5f,  0.5f, 0.0f, 1.0f},
    };
    D3DX10_SPRITE *ptr, *start_sprite;
    unsigned int i, start;

    for (i = 0; i < count; ++i, v += 4)
    {
        ptr = d3dx10_get_sprite_ptr(sprites, i, stride);

        memcpy(&v->color, &ptr->ColorModulate, sizeof(v->color));
        v[0].texcoord.z = ptr->TextureIndex;
        v[1] = v[2] = v[3] = *v;
        D3DXVec4TransformArray(&v->pos, sizeof(*v), quad, sizeof(*quad), &ptr->matWorld, 4);
        v[0].texcoord.x = ptr->TexCoord.x;
        v[0].texcoord.y = ptr->TexCoord.y + ptr->TexSize.y;
        v[1].texcoord.x = ptr->TexCoord.x;
        v[1].texcoord.y = ptr->TexCoord.y;
        v[2].texcoord.x = ptr->TexCoord.x + ptr->TexSize.x;
        v[2].texcoord.y = ptr->TexCoord.y + ptr->TexSize.y;
        v[3].texcoord.x = ptr->TexCoord.x + ptr->TexSize.x;
        v[3].texcoord.y = ptr->TexCoord.y;
    }

    ID3D10Device_UpdateSubresource(sprite->device, (ID3D10Resource *)sprite->vb, 0, NULL,
            sprite->vertex_data, 0, 0);

    start_sprite = sprites;
    start = 0;

    for (i = 0; i < count; ++i)
    {
        ptr = d3dx10_get_sprite_ptr(sprites, i, stride);

        if (ptr->pTexture != start_sprite->pTexture || i == count - 1)
        {
            ID3D10Device_PSSetShaderResources(sprite->device, 0, 1, &start_sprite->pTexture);
            ID3D10Device_DrawIndexed(sprite->device, (i - start + 1) * 6, start * 6, 0);
            start_sprite = ptr;
            start = i;
        }
    }

    return d3dx10_get_sprite_ptr(sprites, count, stride);
}

static void d3dx10_sprite_draw(struct d3dx10_sprite *sprite, D3DX10_SPRITE *sprites,
        size_t count, unsigned int stride)
{
    unsigned int i, vb_stride, offset;
    D3DXMATRIX m;

    if (!count) return;

    if (!stride) stride = sizeof(*sprites);

    ID3D10Device_IASetInputLayout(sprite->device, sprite->input_layout);
    ID3D10Device_IASetPrimitiveTopology(sprite->device, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    vb_stride = sizeof(*sprite->vertex_data);
    offset = 0;
    ID3D10Device_IASetVertexBuffers(sprite->device, 0, 1, &sprite->vb, &vb_stride, &offset);
    ID3D10Device_IASetIndexBuffer(sprite->device, sprite->ib, DXGI_FORMAT_R16_UINT, 0);
    ID3D10Device_VSSetShader(sprite->device, sprite->vertex_shader);
    ID3D10Device_VSSetConstantBuffers(sprite->device, 0, 1, &sprite->vs_cb);
    ID3D10Device_PSSetShader(sprite->device, sprite->pixel_shader);
    ID3D10Device_PSSetConstantBuffers(sprite->device, 0, 0, NULL);
    ID3D10Device_PSSetSamplers(sprite->device, 0, 1, &sprite->sampler);

    D3DXMatrixMultiply(&m, &sprite->projection, &sprite->view);
    ID3D10Device_UpdateSubresource(sprite->device, (ID3D10Resource *)sprite->vs_cb, 0, NULL,
            &m, 0, 0);

    for (i = 0; i < count / sprite->batch_size; ++i)
        sprites = d3dx10_sprite_draw_batch(sprite, sprites, sprite->batch_size, stride);

    d3dx10_sprite_draw_batch(sprite, sprites, count % sprite->batch_size, stride);
}

static void d3dx10_sprite_flush(struct d3dx10_sprite *sprite)
{
    d3dx10_sprite_draw(sprite, sprite->buffer.sprites, sprite->buffer.count, 0);
    d3dx10_sprite_clear_batch(sprite);
}

static inline struct d3dx10_sprite *impl_from_ID3DX10Sprite(ID3DX10Sprite *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx10_sprite, ID3DX10Sprite_iface);
}

static HRESULT WINAPI d3dx10_sprite_QueryInterface(ID3DX10Sprite *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DX10Sprite)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3dx10_sprite_AddRef(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedIncrement(&sprite->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3dx10_sprite_Release(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);
    ULONG refcount = InterlockedDecrement(&sprite->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        d3dx10_sprite_cleanup(sprite);
        free(sprite);
    }

    return refcount;
}

static HRESULT WINAPI d3dx10_sprite_Begin(ID3DX10Sprite *iface, UINT flags)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, flags %#x.\n", iface, flags);

    if (sprite->flags & D3DX10_SPRITE_READY)
        return E_FAIL;

    if (flags &
            ( D3DX10_SPRITE_SORT_TEXTURE
            | D3DX10_SPRITE_SORT_DEPTH_BACK_TO_FRONT
            | D3DX10_SPRITE_SORT_DEPTH_FRONT_TO_BACK))
    {
        FIXME("Sorting options are not implemented.\n");
    }

    sprite->flags = flags | D3DX10_SPRITE_READY;
    if (sprite->flags & D3DX10_SPRITE_SAVE_STATE)
        sprite->state_block->lpVtbl->Capture(sprite->state_block);

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesBuffered(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, sprites %p, count %u.\n", iface, sprites, count);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    if (!d3dx_array_reserve((void **)&sprite->buffer.sprites, &sprite->buffer.capacity,
            sprite->buffer.count + count, sizeof(*sprite->buffer.sprites)))
    {
        return E_OUTOFMEMORY;
    }

    for (unsigned int i = 0; i < count; ++i)
    {
        sprite->buffer.sprites[sprite->buffer.count++] = sprites[i];
        if (sprite->flags & D3DX10_SPRITE_ADDREF_TEXTURES)
            ID3D10ShaderResourceView_AddRef(sprites[i].pTexture);
    }

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_Flush(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p.\n", iface);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    d3dx10_sprite_flush(sprite);

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_DrawSpritesImmediate(ID3DX10Sprite *iface,
        D3DX10_SPRITE *sprites, UINT count, UINT size, UINT flags)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, sprites %p, count %u, size %u, flags %#x.\n",
            iface, sprites, count, size, flags);

    d3dx10_sprite_draw(sprite, sprites, count, size);

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_End(ID3DX10Sprite *iface)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p.\n", iface);

    if (!(sprite->flags & D3DX10_SPRITE_READY))
        return E_FAIL;

    d3dx10_sprite_flush(sprite);

    if (sprite->flags & D3DX10_SPRITE_SAVE_STATE)
        sprite->state_block->lpVtbl->Apply(sprite->state_block);
    sprite->flags = 0;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    *transform = sprite->view;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_SetViewTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    sprite->view = *transform;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetProjectionTransform(ID3DX10Sprite *iface,
        D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    *transform = sprite->projection;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_SetProjectionTransform(ID3DX10Sprite *iface, D3DXMATRIX *transform)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (!transform)
        return E_FAIL;

    sprite->projection = *transform;

    return S_OK;
}

static HRESULT WINAPI d3dx10_sprite_GetDevice(ID3DX10Sprite *iface, ID3D10Device **device)
{
    struct d3dx10_sprite *sprite = impl_from_ID3DX10Sprite(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (!device)
        return E_FAIL;

    *device = sprite->device;
    ID3D10Device_AddRef(*device);

    return S_OK;
}

static const ID3DX10SpriteVtbl d3dx10_sprite_vtbl =
{
    d3dx10_sprite_QueryInterface,
    d3dx10_sprite_AddRef,
    d3dx10_sprite_Release,
    d3dx10_sprite_Begin,
    d3dx10_sprite_DrawSpritesBuffered,
    d3dx10_sprite_Flush,
    d3dx10_sprite_DrawSpritesImmediate,
    d3dx10_sprite_End,
    d3dx10_sprite_GetViewTransform,
    d3dx10_sprite_SetViewTransform,
    d3dx10_sprite_GetProjectionTransform,
    d3dx10_sprite_SetProjectionTransform,
    d3dx10_sprite_GetDevice,
};

static HRESULT d3dx10_sprite_create_index_buffer(ID3D10Device *device, unsigned int batch_size,
        ID3D10Buffer **ib)
{
    D3D10_SUBRESOURCE_DATA resource_data;
    D3D10_BUFFER_DESC buffer_desc;
    uint16_t *data;
    size_t size;
    HRESULT hr;

    size = batch_size * 6 * sizeof(*data);
    if (!(data = malloc(size)))
        return E_OUTOFMEMORY;

    for (int i = 0; i < batch_size; ++i)
    {
        data[6 * i]     = 4 * i;
        data[6 * i + 1] = 4 * i + 1;
        data[6 * i + 2] = 4 * i + 2;
        data[6 * i + 3] = 4 * i + 1;
        data[6 * i + 4] = 4 * i + 3;
        data[6 * i + 5] = 4 * i + 2;
    }

    buffer_desc.ByteWidth = size;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    resource_data.pSysMem = data;
    resource_data.SysMemPitch = 0;
    resource_data.SysMemSlicePitch = 0;

    hr = ID3D10Device_CreateBuffer(device, &buffer_desc, &resource_data, ib);

    free(data);

    return hr;
}

static HRESULT d3dx10_sprite_init(struct d3dx10_sprite *sprite, ID3D10Device *device, UINT size)
{
    static const D3D10_INPUT_ELEMENT_DESC il_desc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

    static const char vs_code[] =
        "float4x4 transform;\n"
        "\n"
        "struct vertex\n"
        "{\n"
        "    float4 p : SV_POSITION;\n"
        "    float4 t : TEXCOORD;\n"
        "    float4 color : COLOR;\n"
        "};\n"
        "\n"
        "void main(float4 p : POSITION, float4 t : TEXCOORD, float4 c : COLOR, out struct vertex o)\n"
        "{\n"
        "    o.p = mul(transform, p);\n"
        "    o.t = t;\n"
        "    o.color = c;\n"
        "}";

    static const char ps_code[] =
        "Texture2D t;\n"
        "SamplerState s;\n"
        "\n"
        "struct vertex\n"
        "{\n"
        "    float4 p : SV_POSITION;\n"
        "    float4 t : TEXCOORD;\n"
        "    float4 color : COLOR;\n"
        "};\n"
        "\n"
        "float4 main(struct vertex v) : SV_Target\n"
        "{\n"
        "    return t.Sample(s, float2(v.t.x, v.t.y)) * v.color;\n"
        "}";

    D3D10_SAMPLER_DESC sampler_desc = { 0 };
    const unsigned int max_size = 4096;
    ID3D10Blob *vs = NULL, *ps = NULL;
    D3D10_BUFFER_DESC buffer_desc;
    D3D10_STATE_BLOCK_MASK mask;
    unsigned int vb_size;
    HRESULT hr;

    sprite->ID3DX10Sprite_iface.lpVtbl = &d3dx10_sprite_vtbl;
    sprite->refcount = 1;
    sprite->device = device;
    ID3D10Device_AddRef(device);
    sprite->projection._11 = 1.0f;
    sprite->projection._22 = 1.0f;
    sprite->projection._33 = 1.0f;
    sprite->projection._44 = 1.0f;
    sprite->view = sprite->projection;

    /* TODO: we shouldn't be capturing entire state */
    D3D10StateBlockMaskEnableAll(&mask);
    if (FAILED(hr = D3D10CreateStateBlock(device, &mask, &sprite->state_block)))
        goto err;

    if (FAILED(hr = D3DCompile(vs_code, sizeof(vs_code) - 1, "vs_sprite", NULL, NULL,
            "main", "vs_4_0", 0, 0, &vs, NULL)))
    {
        WARN("Failed to compile the vertex shader, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = D3DCompile(ps_code, sizeof(ps_code) - 1, "ps_sprite", NULL, NULL,
            "main", "ps_4_0", 0, 0, &ps, NULL)))
    {
        WARN("Failed to compile the pixel shader, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreateInputLayout(device, il_desc, ARRAY_SIZE(il_desc),
            ID3D10Blob_GetBufferPointer(vs), ID3D10Blob_GetBufferSize(vs), &sprite->input_layout)))
    {
        WARN("Failed to create input layout, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vs),
            ID3D10Blob_GetBufferSize(vs), &sprite->vertex_shader)))
    {
        WARN("Failed to create vertex shader, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D10Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(ps),
            ID3D10Blob_GetBufferSize(ps), &sprite->pixel_shader)))
    {
        WARN("Failed to create pixel shader, hr %#lx.\n", hr);
        goto err;
    }

    ID3D10Blob_Release(vs);
    vs = NULL;
    ID3D10Blob_Release(ps);
    ps = NULL;

    sampler_desc.Filter = D3D10_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler_desc.AddressU = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D10_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
    if (FAILED(hr = ID3D10Device_CreateSamplerState(device, &sampler_desc, &sprite->sampler)))
    {
        WARN("Failed to create a sampler state, hr %#lx.\n", hr);
        goto err;
    }

    sprite->batch_size = size ? min(size, max_size) : max_size;

    buffer_desc.ByteWidth = vb_size = sprite->batch_size * 4 * sizeof(*sprite->vertex_data);
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &sprite->vb)))
    {
        WARN("Failed to create a vertex buffer, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = d3dx10_sprite_create_index_buffer(device, sprite->batch_size, &sprite->ib)))
    {
        WARN("Failed to create an index buffer, hr %#lx.\n", hr);
        goto err;
    }

    buffer_desc.ByteWidth = sizeof(D3DXMATRIX);
    buffer_desc.Usage = D3D10_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

    if (FAILED(hr = ID3D10Device_CreateBuffer(device, &buffer_desc, NULL, &sprite->vs_cb)))
    {
        WARN("Failed to create a constant buffer, hr %#lx.\n", hr);
        goto err;
    }

    if (!(sprite->vertex_data = malloc(vb_size)))
    {
        hr = E_OUTOFMEMORY;
        goto err;
    }

    return S_OK;

err:
    d3dx10_sprite_cleanup(sprite);
    if (vs)
        ID3D10Blob_Release(vs);
    if (ps)
        ID3D10Blob_Release(ps);

    return hr;
}

HRESULT WINAPI D3DX10CreateSprite(ID3D10Device *device, UINT size, ID3DX10Sprite **sprite)
{
    struct d3dx10_sprite *object;
    HRESULT hr;

    TRACE("device %p, size %u, sprite %p.\n", device, size, sprite);

    if (!device || !sprite)
        return D3DERR_INVALIDCALL;

    *sprite = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3dx10_sprite_init(object, device, size)))
    {
        free(object);
        return hr;
    }

    *sprite = &object->ID3DX10Sprite_iface;

    return S_OK;
}
