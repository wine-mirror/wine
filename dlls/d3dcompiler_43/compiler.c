/*
 * Copyright 2009 Matteo Bruni
 * Copyright 2010 Matteo Bruni for CodeWeavers
 * Copyright 2016,2018 Józef Kucia for CodeWeavers
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
#include <stdarg.h>
#include <time.h>
#include "wine/debug.h"

#include "d3dcompiler_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

static HRESULT hresult_from_vkd3d_result(int vkd3d_result)
{
    switch (vkd3d_result)
    {
        case VKD3D_OK:
            return S_OK;
        case VKD3D_ERROR_INVALID_SHADER:
            WARN("Invalid shader bytecode.\n");
            /* fall-through */
        case VKD3D_ERROR:
            return E_FAIL;
        case VKD3D_ERROR_OUT_OF_MEMORY:
            return E_OUTOFMEMORY;
        case VKD3D_ERROR_INVALID_ARGUMENT:
            return E_INVALIDARG;
        case VKD3D_ERROR_NOT_IMPLEMENTED:
            return E_NOTIMPL;
        default:
            FIXME("Unhandled vkd3d result %d.\n", vkd3d_result);
            return E_FAIL;
    }
}

#define D3DXERR_INVALIDDATA                      0x88760b59

/* Mutex used to guarantee a single invocation of the D3DAssemble function at
 * a time. This is needed as the assembler isn't thread-safe. */
static CRITICAL_SECTION wpp_mutex;
static CRITICAL_SECTION_DEBUG wpp_mutex_debug =
{
    0, 0, &wpp_mutex,
    { &wpp_mutex_debug.ProcessLocksList,
      &wpp_mutex_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wpp_mutex") }
};
static CRITICAL_SECTION wpp_mutex = { &wpp_mutex_debug, -1, 0, 0, 0, 0 };

struct d3dcompiler_include_from_file
{
    ID3DInclude ID3DInclude_iface;
    const char *initial_filename;
};

static inline struct d3dcompiler_include_from_file *impl_from_ID3DInclude(ID3DInclude *iface)
{
    return CONTAINING_RECORD(iface, struct d3dcompiler_include_from_file, ID3DInclude_iface);
}

static HRESULT WINAPI d3dcompiler_include_from_file_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    struct d3dcompiler_include_from_file *include = impl_from_ID3DInclude(iface);
    char *fullpath, *buffer = NULL, current_dir[MAX_PATH + 1];
    const char *initial_dir;
    SIZE_T size;
    HANDLE file;
    ULONG read;
    DWORD len;

    if ((initial_dir = strrchr(include->initial_filename, '\\')))
    {
        len = initial_dir - include->initial_filename + 1;
        initial_dir = include->initial_filename;
    }
    else
    {
        len = GetCurrentDirectoryA(MAX_PATH, current_dir);
        current_dir[len] = '\\';
        len++;
        initial_dir = current_dir;
    }
    fullpath = malloc(len + strlen(filename) + 1);
    if (!fullpath)
        return E_OUTOFMEMORY;
    memcpy(fullpath, initial_dir, len);
    strcpy(fullpath + len, filename);

    file = CreateFileA(fullpath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
        goto error;

    TRACE("Include file found at %s.\n", debugstr_a(fullpath));

    size = GetFileSize(file, NULL);
    if (size == INVALID_FILE_SIZE)
        goto error;
    buffer = malloc(size);
    if (!buffer)
        goto error;
    if (!ReadFile(file, buffer, size, &read, NULL) || read != size)
        goto error;

    *bytes = size;
    *data = buffer;

    free(fullpath);
    CloseHandle(file);
    return S_OK;

error:
    free(fullpath);
    free(buffer);
    CloseHandle(file);
    WARN("Returning E_FAIL.\n");
    return E_FAIL;
}

static HRESULT WINAPI d3dcompiler_include_from_file_close(ID3DInclude *iface, const void *data)
{
    free((void *)data);
    return S_OK;
}

const struct ID3DIncludeVtbl d3dcompiler_include_from_file_vtbl =
{
    d3dcompiler_include_from_file_open,
    d3dcompiler_include_from_file_close
};

static const char *get_line(const char **ptr)
{
    const char *p, *q;

    p = *ptr;
    if (!(q = strstr(p, "\n")))
    {
        if (!*p)
            return NULL;
        *ptr += strlen(p);
        return p;
    }
    *ptr = q + 1;

    return p;
}

HRESULT WINAPI vkd3d_D3DPreprocess(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include,
        ID3DBlob **preprocessed_blob, ID3DBlob **messages_blob);

static HRESULT preprocess_shader(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, ID3DBlob **shader_blob,
        ID3DBlob **messages_blob)
{
    struct d3dcompiler_include_from_file include_from_file;

    if (include == D3D_COMPILE_STANDARD_FILE_INCLUDE)
    {
        include_from_file.ID3DInclude_iface.lpVtbl = &d3dcompiler_include_from_file_vtbl;
        include_from_file.initial_filename = filename ? filename : "";
        include = &include_from_file.ID3DInclude_iface;
    }

    return vkd3d_D3DPreprocess(data, data_size, filename, defines, include, shader_blob, messages_blob);
}

static HRESULT assemble_shader(const char *preproc_shader, ID3DBlob **shader_blob, ID3DBlob **error_messages)
{
    struct bwriter_shader *shader;
    char *messages = NULL;
    uint32_t *res, size;
    ID3DBlob *buffer;
    HRESULT hr;
    char *pos;

    shader = SlAssembleShader(preproc_shader, &messages);

    if (messages)
    {
        TRACE("Assembler messages:\n");
        TRACE("%s\n", debugstr_a(messages));

        TRACE("Shader source:\n");
        TRACE("%s\n", debugstr_a(preproc_shader));

        if (error_messages)
        {
            const char *preproc_messages = *error_messages ? ID3D10Blob_GetBufferPointer(*error_messages) : NULL;

            size = strlen(messages) + (preproc_messages ? strlen(preproc_messages) : 0) + 1;
            hr = D3DCreateBlob(size, &buffer);
            if (FAILED(hr))
            {
                free(messages);
                if (shader) SlDeleteShader(shader);
                return hr;
            }
            pos = ID3D10Blob_GetBufferPointer(buffer);
            if (preproc_messages)
            {
                CopyMemory(pos, preproc_messages, strlen(preproc_messages) + 1);
                pos += strlen(preproc_messages);
            }
            CopyMemory(pos, messages, strlen(messages) + 1);

            if (*error_messages) ID3D10Blob_Release(*error_messages);
            *error_messages = buffer;
        }
        free(messages);
    }

    if (shader == NULL)
    {
        ERR("Asm reading failed\n");
        return D3DXERR_INVALIDDATA;
    }

    hr = shader_write_bytecode(shader, &res, &size);
    SlDeleteShader(shader);
    if (FAILED(hr))
    {
        ERR("Failed to write bytecode, hr %#lx.\n", hr);
        return D3DXERR_INVALIDDATA;
    }

    if (shader_blob)
    {
        hr = D3DCreateBlob(size, &buffer);
        if (FAILED(hr))
        {
            free(res);
            return hr;
        }
        CopyMemory(ID3D10Blob_GetBufferPointer(buffer), res, size);
        *shader_blob = buffer;
    }

    free(res);

    return S_OK;
}

HRESULT WINAPI D3DAssemble(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages)
{
    unsigned int preproc_size;
    ID3DBlob *preproc_shader;
    char *preproc_terminated;
    HRESULT hr;

    TRACE("data %p, datasize %Iu, filename %s, defines %p, include %p, sflags %#x, "
            "shader %p, error_messages %p.\n",
            data, datasize, debugstr_a(filename), defines, include, flags, shader, error_messages);

    EnterCriticalSection(&wpp_mutex);

    /* TODO: flags */
    if (flags) FIXME("flags %x\n", flags);

    if (shader) *shader = NULL;

    hr = preprocess_shader(data, datasize, filename, defines, include, &preproc_shader, error_messages);
    if (SUCCEEDED(hr))
    {
        preproc_size = ID3D10Blob_GetBufferSize(preproc_shader);
        if ((preproc_terminated = malloc(preproc_size + 1)))
        {
            memcpy(preproc_terminated, ID3D10Blob_GetBufferPointer(preproc_shader), preproc_size);
            ID3D10Blob_Release(preproc_shader);
            preproc_terminated[preproc_size] = 0;

            hr = assemble_shader(preproc_terminated, shader, error_messages);
            free(preproc_terminated);
        }
    }
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

HRESULT WINAPI vkd3d_D3DCompile2VKD3D(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader,
        ID3DBlob **error_messages, unsigned int compiler_version);

HRESULT WINAPI D3DCompile2(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *macros, ID3DInclude *include, const char *entry_point,
        const char *profile, UINT flags, UINT effect_flags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader_blob,
        ID3DBlob **messages_blob)
{
    struct d3dcompiler_include_from_file include_from_file;
    ID3DBlob *dummy_blob;
    HRESULT hr;

    TRACE("data %p, data_size %Iu, filename %s, macros %p, include %p, entry_point %s, "
            "profile %s, flags %#x, effect_flags %#x, secondary_flags %#x, secondary_data %p, "
            "secondary_data_size %Iu, shader_blob %p, messages_blob %p.\n",
            data, data_size, debugstr_a(filename), macros, include, debugstr_a(entry_point),
            debugstr_a(profile), flags, effect_flags, secondary_flags, secondary_data,
            secondary_data_size, shader_blob, messages_blob);

    if (include == D3D_COMPILE_STANDARD_FILE_INCLUDE)
    {
        include_from_file.ID3DInclude_iface.lpVtbl = &d3dcompiler_include_from_file_vtbl;
        include_from_file.initial_filename = filename ? filename : "";
        include = &include_from_file.ID3DInclude_iface;
    }

    if (shader_blob)
        *shader_blob = NULL;
    else
        shader_blob = &dummy_blob;

    hr = vkd3d_D3DCompile2VKD3D(data, data_size, filename, macros, include, entry_point, profile, flags, effect_flags,
            secondary_flags, secondary_data, secondary_data_size, shader_blob, messages_blob, D3D_COMPILER_VERSION);
    if (SUCCEEDED(hr) && shader_blob == &dummy_blob)
        ID3D10Blob_Release(dummy_blob);
    return hr;
}

HRESULT WINAPI D3DCompile(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, ID3DBlob **shader, ID3DBlob **error_messages)
{
    TRACE("data %p, data_size %Iu, filename %s, defines %p, include %p, entrypoint %s, "
            "target %s, sflags %#x, eflags %#x, shader %p, error_messages %p.\n",
            data, data_size, debugstr_a(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(target), sflags, eflags, shader, error_messages);

    return D3DCompile2(data, data_size, filename, defines, include, entrypoint, target, sflags,
            eflags, 0, NULL, 0, shader, error_messages);
}

HRESULT WINAPI D3DPreprocess(const void *data, SIZE_T size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        ID3DBlob **shader, ID3DBlob **error_messages)
{
    TRACE("data %p, size %Iu, filename %s, defines %p, include %p, shader %p, error_messages %p.\n",
          data, size, debugstr_a(filename), defines, include, shader, error_messages);

    if (!data || !shader)
        return E_INVALIDARG;

    *shader = NULL;

    return preprocess_shader(data, size, filename, defines, include, shader, error_messages);
}

HRESULT WINAPI D3DDisassemble(const void *data, SIZE_T size, UINT flags, const char *comments,
        ID3DBlob **disassembly)
{
    struct vkd3d_shader_compile_info compile_info;
    enum vkd3d_shader_source_type source_type;
    struct vkd3d_shader_code asm_code;
    const char *ptr = data;
    char *messages;
    HRESULT hr;
    int ret;

    TRACE("data %p, size %Iu, flags %#x, comments %p, disassembly %p.\n",
            data, size, flags, comments, disassembly);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    if (comments)
        FIXME("Ignoring comments %s.\n", debugstr_a(comments));

#if D3D_COMPILER_VERSION >= 46
    if (!size)
        return E_INVALIDARG;
#endif

    if (size >= 4 && read_u32(&ptr) == TAG_DXBC)
        source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
    else
        source_type = VKD3D_SHADER_SOURCE_D3D_BYTECODE;

    compile_info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    compile_info.next = NULL;
    compile_info.source.code = data;
    compile_info.source.size = size;
    compile_info.source_type = source_type;
    compile_info.target_type = VKD3D_SHADER_TARGET_D3D_ASM;
    compile_info.options = NULL;
    compile_info.option_count = 0;
    compile_info.log_level = VKD3D_SHADER_LOG_INFO;
    compile_info.source_name = NULL;

    ret = vkd3d_shader_compile(&compile_info, &asm_code, &messages);

    if (ret)
        ERR("Failed to disassemble shader, vkd3d result %d.\n", ret);

    if (messages)
    {
        if (*messages && ERR_ON(d3dcompiler))
        {
            const char *ptr = messages;
            const char *line;

            ERR("Shader log:\n");
            while ((line = get_line(&ptr)))
            {
                ERR("    %.*s", (int)(ptr - line), line);
            }
            ERR("\n");
        }

        vkd3d_shader_free_messages(messages);
    }

    if (ret)
        return hresult_from_vkd3d_result(ret);

    if (SUCCEEDED(hr = D3DCreateBlob(asm_code.size, disassembly)))
        memcpy(ID3D10Blob_GetBufferPointer(*disassembly), asm_code.code, asm_code.size);

    vkd3d_shader_free_shader_code(&asm_code);

    return hr;
}

HRESULT WINAPI D3DCompileFromFile(const WCHAR *filename, const D3D_SHADER_MACRO *defines, ID3DInclude *include,
        const char *entrypoint, const char *target, UINT flags1, UINT flags2, ID3DBlob **code, ID3DBlob **errors)
{
    char filename_a[MAX_PATH], *source = NULL;
    DWORD source_size, read_size;
    HANDLE file;
    HRESULT hr;

    TRACE("filename %s, defines %p, include %p, entrypoint %s, target %s, flags1 %#x, flags2 %#x, "
            "code %p, errors %p.\n", debugstr_w(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(target), flags1, flags2, code, errors);

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    source_size = GetFileSize(file, NULL);
    if (source_size == INVALID_FILE_SIZE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    if (!(source = malloc(source_size)))
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    if (!ReadFile(file, source, source_size, &read_size, NULL) || read_size != source_size)
    {
        WARN("Failed to read file contents.\n");
        hr = E_FAIL;
        goto end;
    }

    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, sizeof(filename_a), NULL, NULL);

    hr = D3DCompile(source, source_size, filename_a, defines, include, entrypoint, target,
            flags1, flags2, code, errors);

end:
    free(source);
    CloseHandle(file);
    return hr;
}

HRESULT WINAPI D3DLoadModule(const void *data, SIZE_T size, ID3D11Module **module)
{
    FIXME("data %p, size %Iu, module %p stub!\n", data, size, module);
    return E_NOTIMPL;
}
