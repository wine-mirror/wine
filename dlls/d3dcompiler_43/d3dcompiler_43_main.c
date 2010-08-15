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
