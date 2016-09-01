/*
 * Copyright 2016 Andrey Gusev
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
#include "wine/port.h"
#include "d3d10_1.h"
#include "d3dx10.h"
#include "d3dx10core.h"
#include "d3dcompiler.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

HRESULT WINAPI D3DX10CompileFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3D10Include *include, const char *entry_point,
        const char *target, UINT sflags, UINT eflags, ID3DX10ThreadPump *pump, ID3D10Blob **shader,
        ID3D10Blob **error_messages, HRESULT *hresult)
{
    TRACE("data %s, data_size %lu, filename %s, defines %p, include %p, entry_point %s, target %s, "
            "sflags %#x, eflags %#x, pump %p, shader %p, error_messages %p, hresult %p.\n",
            debugstr_a(data), data_size, debugstr_a(filename), defines, include, debugstr_a(entry_point),
            debugstr_a(target), sflags, eflags, pump, shader, error_messages, hresult);

    if (pump)
        FIXME("Unimplemented ID3DX10ThreadPump handling.\n");

    return D3DCompile(data, data_size, filename, defines, include, entry_point, target,
            sflags, eflags, shader, error_messages);
}

HRESULT WINAPI D3DX10CreateEffectPoolFromFileA(const char *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool, ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, device %p, "
            "pump %p, effectpool %p, errors %p, hresult %p, stub!\n",
            debugstr_a(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            pump, effectpool, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateEffectPoolFromFileW(const WCHAR *filename, const D3D10_SHADER_MACRO *defines,
        ID3D10Include *include, const char *profile, UINT hlslflags, UINT fxflags, ID3D10Device *device,
        ID3DX10ThreadPump *pump, ID3D10EffectPool **effectpool, ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("filename %s, defines %p, include %p, profile %s, hlslflags %#x, fxflags %#x, device %p, "
            "pump %p, effectpool %p, errors %p, hresult %p, stub!\n",
            debugstr_w(filename), defines, include, debugstr_a(profile), hlslflags, fxflags, device,
            pump, effectpool, errors, hresult);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateAsyncMemoryLoader(const void *data, SIZE_T data_size, ID3DX10DataLoader **loader)
{
    FIXME("data %p, data_size %lu, loader %p stub!\n", data, data_size, loader);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateAsyncFileLoaderA(const char *filename, ID3DX10DataLoader **loader)
{
    FIXME("filename %s, loader %p stub!\n", debugstr_a(filename), loader);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateAsyncFileLoaderW(const WCHAR *filename, ID3DX10DataLoader **loader)
{
    FIXME("filename %s, loader %p stub!\n", debugstr_w(filename), loader);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateAsyncResourceLoaderA(HMODULE module, const char *resource, ID3DX10DataLoader **loader)
{
    FIXME("module %p, resource %s, loader %p stub!\n", module, debugstr_a(resource), loader);

    return E_NOTIMPL;
}

HRESULT WINAPI D3DX10CreateAsyncResourceLoaderW(HMODULE module, const WCHAR *resource, ID3DX10DataLoader **loader)
{
    FIXME("module %p, resource %s, loader %p stub!\n", module, debugstr_w(resource), loader);

    return E_NOTIMPL;
}
