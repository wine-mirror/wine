/*
 * Direct3D shader compiler main file
 *
 * Copyright 2010 Matteo Bruni for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE; /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(inst);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

HRESULT WINAPI D3DCreateBlob(SIZE_T data_size, ID3DBlob **blob)
{
    struct d3dcompiler_blob *object;
    HRESULT hr;

    TRACE("data_size %lu, blob %p\n", data_size, blob);

    if (!blob)
    {
        WARN("Invalid blob specified.\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate D3D blob object memory\n");
        return E_OUTOFMEMORY;
    }

    hr = d3dcompiler_blob_init(object, data_size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize blob, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *blob = (ID3DBlob *)object;

    TRACE("Created ID3DBlob %p\n", object);

    return S_OK;
}

HRESULT WINAPI D3DGetBlobPart(const void *data, SIZE_T data_size, D3D_BLOB_PART part, UINT flags, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, part %s, flags %#x, blob %p\n", data,
           data_size, debug_d3dcompiler_d3d_blob_part(part), flags, blob);

    return d3dcompiler_get_blob_part(data, data_size, part, flags, blob);
}

HRESULT WINAPI D3DGetInputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetInputAndOutputSignatureBlob(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_INPUT_AND_OUTPUT_SIGNATURE_BLOB, 0, blob);
}

HRESULT WINAPI D3DGetDebugInfo(const void *data, SIZE_T data_size, ID3DBlob **blob)
{
    TRACE("data %p, data_size %lu, blob %p\n", data, data_size, blob);

    return d3dcompiler_get_blob_part(data, data_size, D3D_BLOB_DEBUG_INFO, 0, blob);
}

HRESULT WINAPI D3DStripShader(const void *data, SIZE_T data_size, UINT flags, ID3D10Blob **blob)
{
    TRACE("data %p, data_size %lu, flags %#x, blob %p\n", data, data_size, flags, blob);

    return d3dcompiler_strip_shader(data, data_size, flags, blob);
}

HRESULT WINAPI D3DReflect(const void *data, SIZE_T data_size, REFIID riid, void **reflector)
{
    struct d3dcompiler_shader_reflection *object;

    FIXME("data %p, data_size %lu, riid %s, blob %p stub!\n", data, data_size, debugstr_guid(riid), reflector);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate D3D compiler shader reflection object memory\n");
        return E_OUTOFMEMORY;
    }

    object->vtbl = &d3dcompiler_shader_reflection_vtbl;
    object->refcount = 1;

    *reflector = (ID3D11ShaderReflection *)object;

    TRACE("Created ID3D11ShaderReflection %p\n", object);

    return S_OK;
}
