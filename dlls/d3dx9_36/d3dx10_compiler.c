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

#ifdef D3DX10_SDK_VERSION

#include "wine/debug.h"

#define COBJMACROS

#include <stdint.h>
#include "d3dx_helpers.h"
#include "d3d10_1.h"
#include "d3dx10.h"
#include "d3dcompiler.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

#define D3DERR_INVALIDCALL 0x8876086c

HRESULT WINAPI D3DCompileFromMemory(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages);

HRESULT WINAPI D3DPreprocessFromMemory(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, ID3DBlob **shader, ID3DBlob **error_messages);

static HRESULT create_effect(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile,
        UINT shader_flags, UINT effect_flags, ID3D10Device *device, ID3D10EffectPool *effect_pool,
        ID3D10Effect **effect, ID3D10Blob **errors)
{
    const char dxbc[] = {'D','X','B','C'};
    ID3D10Blob *code = NULL;
    void *buffer;
    SIZE_T size;
    HRESULT hr;

    if (!data || !device)
        return E_FAIL;

    if (errors)
        *errors = NULL;

    buffer = (void *)data;
    size = datasize;

    /* Effect is not compiled. */
    if (datasize < sizeof(dxbc) || memcmp(dxbc, data, sizeof(dxbc)))
    {
        if (!profile)
            return D3DERR_INVALIDCALL;
#if D3DX10_SDK_VERSION < 40
        if (FAILED(hr = D3DCompileFromMemory(data, datasize, filename, (D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
                        "main", profile, shader_flags, effect_flags, &code, (ID3DBlob **)errors)))
#else
        if (FAILED(hr = D3DCompile(data, datasize, filename, (D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
                        "main", profile, shader_flags, effect_flags, &code, (ID3DBlob **)errors)))
#endif
        {
            WARN("Effect compilation failed, hr %#lx.\n", hr);
            return hr;
        }
        buffer = ID3D10Blob_GetBufferPointer(code);
        size = ID3D10Blob_GetBufferSize(code);
    }

    hr = D3D10CreateEffectFromMemory(buffer, size, effect_flags, device, effect_pool, effect);

    if (code)
        ID3D10Blob_Release(code);
    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromMemory(const void *data, SIZE_T datasize, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *profile,
        UINT shader_flags, UINT effect_flags, ID3D10Device *device, ID3D10EffectPool *effect_pool,
        ID3DX10ThreadPump *pump, ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    TRACE("data %p, datasize %Iu, filename %s, defines %p, include %p, profile %s, shader_flags %#x,"
            "effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            data, datasize, debugstr_a(filename), defines, include, debugstr_a(profile),
            shader_flags, effect_flags, device, effect_pool, pump, effect, errors, hresult);

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    return create_effect(data, datasize, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
}

HRESULT WINAPI D3DX10CreateEffectFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT shader_flags, UINT effect_flags,
        ID3D10Device *device, ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump,
        ID3D10Effect **effect, ID3D10Blob **errors, HRESULT *hresult)
{
    char filename_a[MAX_PATH];
    void *buffer = NULL;
    uint32_t size = 0;
    HRESULT hr;

    TRACE("filename %s, defines %p, include %p, profile %s, shader_flags %#x, effect_flags %#x, "
            "device %p, effect_pool %p, pump %p, effect %p, errors %p, hresult %p.\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    if (!filename)
        return E_FAIL;

    if (pump)
        FIXME("Asynchronous mode is not supported.\n");

    if (!include)
        include = D3D_COMPILE_STANDARD_FILE_INCLUDE;

    if ((hr = d3dx_load_file(filename, &buffer, &size)) == ERROR_FILE_NOT_FOUND)
        return D3D10_ERROR_FILE_NOT_FOUND;

    if (SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, sizeof(filename_a), NULL, NULL);
        hr = create_effect(buffer, size, filename_a, defines, include, profile,
                shader_flags, effect_flags, device, effect_pool, effect, errors);
        free(buffer);
    }

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
        return E_FAIL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    if (!(filenameW = malloc(len * sizeof(*filenameW))))
        return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filenameW, len);

    hr = D3DX10CreateEffectFromFileW(filenameW, defines, include, profile, shader_flags,
            effect_flags, device, effect_pool, pump, effect, errors, hresult);
    free(filenameW);

    return hr;
}

HRESULT WINAPI D3DX10CreateEffectFromResourceA(HMODULE module, const char *resource_name,
        const char *filename, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult)
{
    uint32_t size;
    void *data;
    HRESULT hr;

    TRACE("module %p, resource_name %s, filename %s, defines %p, include %p, profile %s, "
            "shader_flags %#x, effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, "
            "errors %p, hresult %p.\n", module, debugstr_a(resource_name), debugstr_a(filename),
            defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    hr = d3dx_load_resource_a(module, resource_name, &data, &size);
    if (FAILED(hr))
        return hr;

    return create_effect(data, size, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
}

HRESULT WINAPI D3DX10CreateEffectFromResourceW(HMODULE module, const WCHAR *resource_name,
        const WCHAR *filenameW, const D3D10_SHADER_MACRO *defines, ID3D10Include *include,
        const char *profile, UINT shader_flags, UINT effect_flags, ID3D10Device *device,
        ID3D10EffectPool *effect_pool, ID3DX10ThreadPump *pump, ID3D10Effect **effect,
        ID3D10Blob **errors, HRESULT *hresult)
{
    char *filename = NULL;
    uint32_t size;
    void *data;
    HRESULT hr;
    int len;

    TRACE("module %p, resource_name %s, filename %s, defines %p, include %p, profile %s, "
            "shader_flags %#x, effect_flags %#x, device %p, effect_pool %p, pump %p, effect %p, "
            "errors %p, hresult %p.\n", module, debugstr_w(resource_name), debugstr_w(filenameW),
            defines, include, debugstr_a(profile), shader_flags, effect_flags,
            device, effect_pool, pump, effect, errors, hresult);

    hr = d3dx_load_resource_w(module, resource_name, &data, &size);
    if (FAILED(hr))
        return hr;

    if (filenameW)
    {
        len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);
        if (!(filename = malloc(len)))
            return E_OUTOFMEMORY;
        WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, len, NULL, NULL);
    }

    hr = create_effect(data, size, filename, defines, include, profile,
            shader_flags, effect_flags, device, effect_pool, effect, errors);
    free(filename);
    return hr;
}

HRESULT WINAPI D3DX10PreprocessShaderFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3DInclude *include, ID3DX10ThreadPump *pump, ID3D10Blob **shader_text,
        ID3D10Blob **errors, HRESULT *hresult)
{
    HRESULT hr;

    TRACE("data %s, data_size %Iu, filename %s, defines %p, include %p, pump %p, shader_text %p, "
            "errors %p, hresult %p.\n",
            debugstr_an(data, data_size), data_size, debugstr_a(filename), defines, include, pump,
            shader_text, errors, hresult);

    if (!data)
        return E_FAIL;

    if (pump)
        FIXME("Unimplemented ID3DX10ThreadPump handling.\n");

#if D3DX10_SDK_VERSION < 40
    hr = D3DPreprocessFromMemory(data, data_size, filename,
        (const D3D_SHADER_MACRO *)defines, include, (ID3DBlob **)shader_text, (ID3DBlob **)errors);
#else
    hr = D3DPreprocess(data, data_size, filename,
        (const D3D_SHADER_MACRO *)defines, include, (ID3DBlob **)shader_text, (ID3DBlob **)errors);
#endif

    if (hresult)
        *hresult = hr;
    return hr;
}

HRESULT WINAPI D3DX10CompileFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entry_point,
        const char *target, UINT sflags, UINT eflags, ID3DX10ThreadPump *pump, ID3D10Blob **shader,
        ID3D10Blob **error_messages, HRESULT *hresult)
{
    TRACE("data %s, data_size %Iu, filename %s, defines %p, include %p, entry_point %s, target %s, "
            "sflags %#x, eflags %#x, pump %p, shader %p, error_messages %p, hresult %p.\n",
            debugstr_an(data, data_size), data_size, debugstr_a(filename), defines, include,
            debugstr_a(entry_point), debugstr_a(target), sflags, eflags, pump, shader,
            error_messages, hresult);

    if (pump)
        FIXME("Unimplemented ID3DX10ThreadPump handling.\n");

#if D3DX10_SDK_VERSION < 40
    return D3DCompileFromMemory(data, data_size, NULL, (D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
            entry_point, target, sflags, eflags, (ID3DBlob **)shader, (ID3DBlob **)error_messages);
#else
    return D3DCompile(data, data_size, NULL, (D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
            entry_point, target, sflags, eflags, (ID3DBlob **)shader, (ID3DBlob **)error_messages);
#endif
}

#endif
