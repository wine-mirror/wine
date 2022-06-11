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

#define COBJMACROS
#include "d3d10_1.h"
#include "d3dx10.h"
#include "d3dcompiler.h"
#include "dxhelpers.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct asyncdataloader
{
    ID3DX10DataLoader ID3DX10DataLoader_iface;

    union
    {
        struct
        {
            WCHAR *path;
        } file;
        struct
        {
            HMODULE module;
            HRSRC rsrc;
        } resource;
    } u;
    void *data;
    DWORD size;
};

static inline struct asyncdataloader *impl_from_ID3DX10DataLoader(ID3DX10DataLoader *iface)
{
    return CONTAINING_RECORD(iface, struct asyncdataloader, ID3DX10DataLoader_iface);
}

static HRESULT WINAPI memorydataloader_Load(ID3DX10DataLoader *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static HRESULT WINAPI memorydataloader_Decompress(ID3DX10DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI memorydataloader_Destroy(ID3DX10DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader);
    return S_OK;
}

static const ID3DX10DataLoaderVtbl memorydataloadervtbl =
{
    memorydataloader_Load,
    memorydataloader_Decompress,
    memorydataloader_Destroy
};

HRESULT load_file(const WCHAR *path, void **data, DWORD *size)
{
    DWORD read_len;
    HANDLE file;
    BOOL ret;

    file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return D3D10_ERROR_FILE_NOT_FOUND;

    *size = GetFileSize(file, NULL);
    *data = malloc(*size);
    if (!*data)
    {
        CloseHandle(file);
        return E_OUTOFMEMORY;
    }

    ret = ReadFile(file, *data, *size, &read_len, NULL);
    CloseHandle(file);
    if (!ret || read_len != *size)
    {
        WARN("Failed to read file contents.\n");
        free(*data);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI filedataloader_Load(ID3DX10DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);
    void *data;
    DWORD size;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    /* Always buffer file contents, even if Load() was already called. */
    if (FAILED((hr = load_file(loader->u.file.path, &data, &size))))
        return hr;

    free(loader->data);
    loader->data = data;
    loader->size = size;

    return S_OK;
}

static HRESULT WINAPI filedataloader_Decompress(ID3DX10DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    if (!loader->data)
        return E_FAIL;

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI filedataloader_Destroy(ID3DX10DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader->u.file.path);
    free(loader->data);
    free(loader);

    return S_OK;
}

static const ID3DX10DataLoaderVtbl filedataloadervtbl =
{
    filedataloader_Load,
    filedataloader_Decompress,
    filedataloader_Destroy
};

static HRESULT load_resource_initA(HMODULE module, const char *resource, HRSRC *rsrc)
{
    if (!(*rsrc = FindResourceA(module, resource, (const char *)RT_RCDATA)))
        *rsrc = FindResourceA(module, resource, (const char *)RT_BITMAP);
    if (!*rsrc)
    {
        WARN("Failed to find resource.\n");
        return D3DX10_ERR_INVALID_DATA;
    }
    return S_OK;
}

static HRESULT load_resource_initW(HMODULE module, const WCHAR *resource, HRSRC *rsrc)
{
    if (!(*rsrc = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
        *rsrc = FindResourceW(module, resource, (const WCHAR *)RT_BITMAP);
    if (!*rsrc)
    {
        WARN("Failed to find resource.\n");
        return D3DX10_ERR_INVALID_DATA;
    }
    return S_OK;
}

static HRESULT load_resource(HMODULE module, HRSRC rsrc, void **data, DWORD *size)
{
    HGLOBAL hglobal;

    if (!(*size = SizeofResource(module, rsrc)))
        return D3DX10_ERR_INVALID_DATA;
    if (!(hglobal = LoadResource(module, rsrc)))
        return D3DX10_ERR_INVALID_DATA;
    if (!(*data = LockResource(hglobal)))
        return D3DX10_ERR_INVALID_DATA;
    return S_OK;
}

HRESULT load_resourceA(HMODULE module, const char *resource, void **data, DWORD *size)
{
    HRESULT hr;
    HRSRC rsrc;

    if (FAILED((hr = load_resource_initA(module, resource, &rsrc))))
        return hr;
    return load_resource(module, rsrc, data, size);
}

HRESULT load_resourceW(HMODULE module, const WCHAR *resource, void **data, DWORD *size)
{
    HRESULT hr;
    HRSRC rsrc;

    if ((FAILED(hr = load_resource_initW(module, resource, &rsrc))))
        return hr;
    return load_resource(module, rsrc, data, size);
}

static HRESULT WINAPI resourcedataloader_Load(ID3DX10DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p.\n", iface);

    if (loader->data)
        return S_OK;

    return load_resource(loader->u.resource.module, loader->u.resource.rsrc,
            &loader->data, &loader->size);
}

static HRESULT WINAPI resourcedataloader_Decompress(ID3DX10DataLoader *iface, void **data, SIZE_T *size)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p, data %p, size %p.\n", iface, data, size);

    if (!loader->data)
        return E_FAIL;

    *data = loader->data;
    *size = loader->size;

    return S_OK;
}

static HRESULT WINAPI resourcedataloader_Destroy(ID3DX10DataLoader *iface)
{
    struct asyncdataloader *loader = impl_from_ID3DX10DataLoader(iface);

    TRACE("iface %p.\n", iface);

    free(loader);

    return S_OK;
}

static const ID3DX10DataLoaderVtbl resourcedataloadervtbl =
{
    resourcedataloader_Load,
    resourcedataloader_Decompress,
    resourcedataloader_Destroy
};

struct texture_info_processor
{
    ID3DX10DataProcessor ID3DX10DataProcessor_iface;
    D3DX10_IMAGE_INFO *info;
};

static inline struct texture_info_processor *impl_from_ID3DX10DataProcessor(ID3DX10DataProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct texture_info_processor, ID3DX10DataProcessor_iface);
}

static HRESULT WINAPI texture_info_processor_Process(ID3DX10DataProcessor *iface, void *data, SIZE_T size)
{
    struct texture_info_processor *processor = impl_from_ID3DX10DataProcessor(iface);

    TRACE("iface %p, data %p, size %Iu.\n", iface, data, size);
    return get_image_info(data, size, processor->info);
}

static HRESULT WINAPI texture_info_processor_CreateDeviceObject(ID3DX10DataProcessor *iface, void **object)
{
    TRACE("iface %p, object %p.\n", iface, object);
    return S_OK;
}

static HRESULT WINAPI texture_info_processor_Destroy(ID3DX10DataProcessor *iface)
{
    struct texture_info_processor *processor = impl_from_ID3DX10DataProcessor(iface);

    TRACE("iface %p.\n", iface);

    free(processor);
    return S_OK;
}

static ID3DX10DataProcessorVtbl texture_info_processor_vtbl =
{
    texture_info_processor_Process,
    texture_info_processor_CreateDeviceObject,
    texture_info_processor_Destroy
};

struct texture_processor
{
    ID3DX10DataProcessor ID3DX10DataProcessor_iface;
    ID3D10Device *device;
    D3DX10_IMAGE_LOAD_INFO load_info;
    D3D10_SUBRESOURCE_DATA *resource_data;
};

static inline struct texture_processor *texture_processor_from_ID3DX10DataProcessor(ID3DX10DataProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct texture_processor, ID3DX10DataProcessor_iface);
}

static HRESULT WINAPI texture_processor_Process(ID3DX10DataProcessor *iface, void *data, SIZE_T size)
{
    struct texture_processor *processor = texture_processor_from_ID3DX10DataProcessor(iface);

    TRACE("iface %p, data %p, size %Iu.\n", iface, data, size);

    if (processor->resource_data)
    {
        WARN("Called multiple times.\n");
        free(processor->resource_data);
        processor->resource_data = NULL;
    }
    return load_texture_data(data, size, &processor->load_info, &processor->resource_data);
}

static HRESULT WINAPI texture_processor_CreateDeviceObject(ID3DX10DataProcessor *iface, void **object)
{
    struct texture_processor *processor = texture_processor_from_ID3DX10DataProcessor(iface);

    TRACE("iface %p, object %p.\n", iface, object);

    if (!processor->resource_data)
        return E_FAIL;

    return create_d3d_texture(processor->device, &processor->load_info,
            processor->resource_data, (ID3D10Resource **)object);
}

static HRESULT WINAPI texture_processor_Destroy(ID3DX10DataProcessor *iface)
{
    struct texture_processor *processor = texture_processor_from_ID3DX10DataProcessor(iface);

    TRACE("iface %p.\n", iface);

    ID3D10Device_Release(processor->device);
    free(processor->resource_data);
    free(processor);
    return S_OK;
}

static ID3DX10DataProcessorVtbl texture_processor_vtbl =
{
    texture_processor_Process,
    texture_processor_CreateDeviceObject,
    texture_processor_Destroy
};

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
    struct asyncdataloader *object;

    TRACE("data %p, data_size %Iu, loader %p.\n", data, data_size, loader);

    if (!data || !loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX10DataLoader_iface.lpVtbl = &memorydataloadervtbl;
    object->data = (void *)data;
    object->size = data_size;

    *loader = &object->ID3DX10DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX10CreateAsyncFileLoaderA(const char *filename, ID3DX10DataLoader **loader)
{
    WCHAR *filename_w;
    HRESULT hr;
    int len;

    TRACE("filename %s, loader %p.\n", debugstr_a(filename), loader);

    if (!filename || !loader)
        return E_FAIL;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = malloc(len * sizeof(*filename_w));
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    hr = D3DX10CreateAsyncFileLoaderW(filename_w, loader);

    free(filename_w);

    return hr;
}

HRESULT WINAPI D3DX10CreateAsyncFileLoaderW(const WCHAR *filename, ID3DX10DataLoader **loader)
{
    struct asyncdataloader *object;

    TRACE("filename %s, loader %p.\n", debugstr_w(filename), loader);

    if (!filename || !loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX10DataLoader_iface.lpVtbl = &filedataloadervtbl;
    object->u.file.path = malloc((lstrlenW(filename) + 1) * sizeof(WCHAR));
    if (!object->u.file.path)
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    lstrcpyW(object->u.file.path, filename);
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX10DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX10CreateAsyncResourceLoaderA(HMODULE module, const char *resource, ID3DX10DataLoader **loader)
{
    struct asyncdataloader *object;
    HRSRC rsrc;
    HRESULT hr;

    TRACE("module %p, resource %s, loader %p.\n", module, debugstr_a(resource), loader);

    if (!loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED((hr = load_resource_initA(module, resource, &rsrc))))
    {
        free(object);
        return hr;
    }

    object->ID3DX10DataLoader_iface.lpVtbl = &resourcedataloadervtbl;
    object->u.resource.module = module;
    object->u.resource.rsrc = rsrc;
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX10DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX10CreateAsyncResourceLoaderW(HMODULE module, const WCHAR *resource, ID3DX10DataLoader **loader)
{
    struct asyncdataloader *object;
    HRSRC rsrc;
    HRESULT hr;

    TRACE("module %p, resource %s, loader %p.\n", module, debugstr_w(resource), loader);

    if (!loader)
        return E_FAIL;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED((hr = load_resource_initW(module, resource, &rsrc))))
    {
        free(object);
        return hr;
    }

    object->ID3DX10DataLoader_iface.lpVtbl = &resourcedataloadervtbl;
    object->u.resource.module = module;
    object->u.resource.rsrc = rsrc;
    object->data = NULL;
    object->size = 0;

    *loader = &object->ID3DX10DataLoader_iface;

    return S_OK;
}

HRESULT WINAPI D3DX10CreateAsyncTextureInfoProcessor(D3DX10_IMAGE_INFO *info, ID3DX10DataProcessor **processor)
{
    struct texture_info_processor *object;

    TRACE("info %p, processor %p.\n", info, processor);

    if (!processor)
        return E_INVALIDARG;

    object = malloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX10DataProcessor_iface.lpVtbl = &texture_info_processor_vtbl;
    object->info = info;

    *processor = &object->ID3DX10DataProcessor_iface;
    return S_OK;
}

HRESULT WINAPI D3DX10CreateAsyncTextureProcessor(ID3D10Device *device,
        D3DX10_IMAGE_LOAD_INFO *load_info, ID3DX10DataProcessor **processor)
{
    struct texture_processor *object;

    TRACE("device %p, load_info %p, processor %p.\n", device, load_info, processor);

    if (!device || !processor)
        return E_INVALIDARG;

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->ID3DX10DataProcessor_iface.lpVtbl = &texture_processor_vtbl;
    object->device = device;
    ID3D10Device_AddRef(device);
    init_load_info(load_info, &object->load_info);

    *processor = &object->ID3DX10DataProcessor_iface;
    return S_OK;
}

HRESULT WINAPI D3DX10PreprocessShaderFromMemory(const char *data, SIZE_T data_size, const char *filename,
        const D3D10_SHADER_MACRO *defines, ID3DInclude *include, ID3DX10ThreadPump *pump, ID3D10Blob **shader_text,
        ID3D10Blob **errors, HRESULT *hresult)
{
    FIXME("data %s, data_size %Iu, filename %s, defines %p, include %p, pump %p, shader_text %p, "
            "errors %p, hresult %p stub!\n",
            debugstr_an(data, data_size), data_size, debugstr_a(filename), defines, include, pump,
            shader_text, errors, hresult);

    return E_NOTIMPL;
}
