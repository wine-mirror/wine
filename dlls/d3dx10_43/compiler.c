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

#include "wine/debug.h"
#include "wine/heap.h"

#define COBJMACROS

#include "d3d10_1.h"
#include "d3dx10.h"
#include "d3dcompiler.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);


HRESULT WINAPI D3DX10CreateEffectFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile,
        UINT shader_flags, UINT effect_flags, ID3D10Device *device, ID3D10EffectPool *effect_pool,
        ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    ID3D10Blob *code;
    HRESULT hr;

    TRACE("data %p, datasize %lu, filename %s, defines %p, include %p, profile %s, shader_flags %#x,"
            "effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile),
            shader_flags, effect_flags, device, effect_pool, pump, effect, errors, hresult);

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    if (FAILED(hr = D3DCompile(data, datasize, filename, defines, include, "main", profile,
            shader_flags, effect_flags, &code, errors)))
    {
        WARN("Effect compilation failed, hr %#x.\n", hr);
        return hr;
    }

    hr = D3D10CreateEffectFromMemory(ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code),
            effect_flags, device, effect_pool, effect);
    ID3D10Blob_Release(code);

    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT shader_flags, UINT effect_flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    ID3D10Blob *code;
    HRESULT hr;

    TRACE("filename %s, defines %p, include %p, profile %s, shader_flags %#x, effect_flags %#x, "
            "device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    if (FAILED(hr = D3DCompileFromFile(filename, defines, include, "main", profile, shader_flags,
            effect_flags, &code, errors)))
    {
        WARN("Effect compilation failed, hr %#x.\n", hr);
        return hr;
    }

    hr = D3D10CreateEffectFromMemory(ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code),
            effect_flags, device, effect_pool, effect);
    ID3D10Blob_Release(code);

    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT shader_flags, UINT effect_flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    WCHAR *filenameW;
    HRESULT hr;
    int len;

    TRACE("filename %s, defines %p, include %p, profile %s, shader_flags %#x, effect_flags %#x, "
            "device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            debugstr_a(filename), defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    if (!filename)
        return E_INVALIDARG;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    if (!(filenameW = heap_alloc(len * sizeof(*filenameW))))
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = D3DX10CreateEffectFromFileW(filenameW, defines, include, profile, shader_flags,
            effect_flags, device, effect_pool, pump, effect, errors, hresult);
    heap_free(filenameW);

    return hr;
}
