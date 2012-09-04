/*
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Matteo Bruni
 * Copyright 2011 Travis Athougies
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "wine/debug.h"
#include "wine/unicode.h"
#include "windef.h"
#include "wingdi.h"
#include "objbase.h"
#include "d3dcommon.h"
#include "d3dcompiler.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

/* This function is not declared in the SDK headers yet */
HRESULT WINAPI D3DAssemble(LPCVOID data, SIZE_T datasize, LPCSTR filename,
                           const D3D_SHADER_MACRO *defines, ID3DInclude *include,
                           UINT flags,
                           ID3DBlob **shader, ID3DBlob **error_messages);

LPCSTR WINAPI D3DXGetPixelShaderProfile(LPDIRECT3DDEVICE9 device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.PixelShaderVersion)
    {
    case D3DPS_VERSION(1, 1):
        return "ps_1_1";

    case D3DPS_VERSION(1, 2):
        return "ps_1_2";

    case D3DPS_VERSION(1, 3):
        return "ps_1_3";

    case D3DPS_VERSION(1, 4):
        return "ps_1_4";

    case D3DPS_VERSION(2, 0):
        if ((caps.PS20Caps.NumTemps>=22)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_ARBITRARYSWIZZLE)     &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_GRADIENTINSTRUCTIONS) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_PREDICATION)          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NODEPENDENTREADLIMIT) &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_a";
        }
        if ((caps.PS20Caps.NumTemps>=32)                          &&
            (caps.PS20Caps.Caps&D3DPS20CAPS_NOTEXINSTRUCTIONLIMIT))
        {
            return "ps_2_b";
        }
        return "ps_2_0";

    case D3DPS_VERSION(3, 0):
        return "ps_3_0";
    }

    return NULL;
}

UINT WINAPI D3DXGetShaderSize(const DWORD *byte_code)
{
    const DWORD *ptr = byte_code;

    TRACE("byte_code %p\n", byte_code);

    if (!ptr) return 0;

    /* Look for the END token, skipping the VERSION token */
    while (*++ptr != D3DSIO_END)
    {
        /* Skip comments */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            ptr += ((*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT);
        }
    }
    ++ptr;

    /* Return the shader size in bytes */
    return (ptr - byte_code) * sizeof(*ptr);
}

DWORD WINAPI D3DXGetShaderVersion(const DWORD *byte_code)
{
    TRACE("byte_code %p\n", byte_code);

    return byte_code ? *byte_code : 0;
}

LPCSTR WINAPI D3DXGetVertexShaderProfile(LPDIRECT3DDEVICE9 device)
{
    D3DCAPS9 caps;

    TRACE("device %p\n", device);

    if (!device) return NULL;

    IDirect3DDevice9_GetDeviceCaps(device,&caps);

    switch (caps.VertexShaderVersion)
    {
    case D3DVS_VERSION(1, 1):
        return "vs_1_1";
    case D3DVS_VERSION(2, 0):
        if ((caps.VS20Caps.NumTemps>=13) &&
            (caps.VS20Caps.DynamicFlowControlDepth==24) &&
            (caps.VS20Caps.Caps&D3DPS20CAPS_PREDICATION))
        {
            return "vs_2_a";
        }
        return "vs_2_0";
    case D3DVS_VERSION(3, 0):
        return "vs_3_0";
    }

    return NULL;
}

HRESULT WINAPI D3DXFindShaderComment(CONST DWORD* byte_code, DWORD fourcc, LPCVOID* data, UINT* size)
{
    CONST DWORD *ptr = byte_code;

    TRACE("(%p, %x, %p, %p)\n", byte_code, fourcc, data, size);

    if (data) *data = NULL;
    if (size) *size = 0;

    if (!byte_code)
        return D3DERR_INVALIDCALL;

    while (*++ptr != D3DSIO_END)
    {
        /* Check if it is a comment */
        if ((*ptr & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT)
        {
            DWORD comment_size = (*ptr & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;

            /* Check if this is the comment we are looking for */
            if (*(ptr + 1) == fourcc)
            {
                UINT ctab_size = (comment_size - 1) * sizeof(DWORD);
                LPCVOID ctab_data = ptr + 2;
                if (size)
                    *size = ctab_size;
                if (data)
                    *data = ctab_data;
                TRACE("Returning comment data at %p with size %d\n", ctab_data, ctab_size);
                return D3D_OK;
            }
            ptr += comment_size;
        }
    }

    return S_FALSE;
}

HRESULT WINAPI D3DXAssembleShader(LPCSTR data,
                                  UINT data_len,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  DWORD flags,
                                  LPD3DXBUFFER* shader,
                                  LPD3DXBUFFER* error_messages)
{
    /* Forward to d3dcompiler: the parameter types aren't really different,
       the actual data types are equivalent */
    HRESULT hr = D3DAssemble(data, data_len, NULL, (D3D_SHADER_MACRO *)defines,
                             (ID3DInclude *)include, flags, (ID3DBlob **)shader,
                             (ID3DBlob **)error_messages);

    if(hr == E_FAIL) hr = D3DXERR_INVALIDDATA;
    return hr;
}

/* D3DXInclude private implementation, used to implement
   D3DXAssembleShaderFromFile from D3DXAssembleShader */
/* To be able to correctly resolve include search paths we have to store
   the pathname of each include file. We store the pathname pointer right
   before the file data. */
static HRESULT WINAPI d3dincludefromfile_open(ID3DXInclude *iface,
                                              D3DXINCLUDE_TYPE include_type,
                                              LPCSTR filename, LPCVOID parent_data,
                                              LPCVOID *data, UINT *bytes) {
    const char *p, *parent_name = "";
    char *pathname = NULL;
    char **buffer = NULL;
    HANDLE file;
    UINT size;

    if(parent_data != NULL)
        parent_name = *((const char **)parent_data - 1);

    TRACE("Looking up for include file %s, parent %s\n", debugstr_a(filename), debugstr_a(parent_name));

    if ((p = strrchr(parent_name, '\\')) || (p = strrchr(parent_name, '/'))) p++;
    else p = parent_name;
    pathname = HeapAlloc(GetProcessHeap(), 0, (p - parent_name) + strlen(filename) + 1);
    if(!pathname)
        return HRESULT_FROM_WIN32(GetLastError());

    memcpy(pathname, parent_name, p - parent_name);
    strcpy(pathname + (p - parent_name), filename);

    file = CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(file == INVALID_HANDLE_VALUE)
        goto error;

    TRACE("Include file found at pathname = %s\n", debugstr_a(pathname));

    size = GetFileSize(file, NULL);
    if(size == INVALID_FILE_SIZE)
        goto error;

    buffer = HeapAlloc(GetProcessHeap(), 0, size + sizeof(char *));
    if(!buffer)
        goto error;
    *buffer = pathname;
    if(!ReadFile(file, buffer + 1, size, bytes, NULL))
        goto error;

    *data = buffer + 1;

    CloseHandle(file);
    return S_OK;

error:
    CloseHandle(file);
    HeapFree(GetProcessHeap(), 0, pathname);
    HeapFree(GetProcessHeap(), 0, buffer);
    return HRESULT_FROM_WIN32(GetLastError());
}

static HRESULT WINAPI d3dincludefromfile_close(ID3DXInclude *iface, LPCVOID data) {
    HeapFree(GetProcessHeap(), 0, *((char **)data - 1));
    HeapFree(GetProcessHeap(), 0, (char **)data - 1);
    return S_OK;
}

static const struct ID3DXIncludeVtbl D3DXInclude_Vtbl = {
    d3dincludefromfile_open,
    d3dincludefromfile_close
};

struct D3DXIncludeImpl {
    ID3DXInclude ID3DXInclude_iface;
};

HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages)
{
    LPWSTR filename_w = NULL;
    DWORD len;
    HRESULT ret;

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXAssembleShaderFromFileW(filename_w, defines, include, flags, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXAssembleShaderFromFileW(LPCWSTR filename,
                                           CONST D3DXMACRO* defines,
                                           LPD3DXINCLUDE include,
                                           DWORD flags,
                                           LPD3DXBUFFER* shader,
                                           LPD3DXBUFFER* error_messages)
{
    void *buffer;
    DWORD len;
    HRESULT hr;
    struct D3DXIncludeImpl includefromfile;

    if(FAILED(map_view_of_file(filename, &buffer, &len)))
        return D3DXERR_INVALIDDATA;

    if(!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    hr = D3DXAssembleShader(buffer, len, defines, include, flags,
                            shader, error_messages);

    UnmapViewOfFile(buffer);
    return hr;
}

HRESULT WINAPI D3DXAssembleShaderFromResourceA(HMODULE module,
                                               LPCSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceA(module, resource, (LPCSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}

HRESULT WINAPI D3DXAssembleShaderFromResourceW(HMODULE module,
                                               LPCWSTR resource,
                                               CONST D3DXMACRO* defines,
                                               LPD3DXINCLUDE include,
                                               DWORD flags,
                                               LPD3DXBUFFER* shader,
                                               LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceW(module, resource, (LPCWSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXAssembleShader(buffer, len, defines, include, flags,
                              shader, error_messages);
}

HRESULT WINAPI D3DXCompileShader(LPCSTR pSrcData,
                                 UINT srcDataLen,
                                 CONST D3DXMACRO* pDefines,
                                 LPD3DXINCLUDE pInclude,
                                 LPCSTR pFunctionName,
                                 LPCSTR pProfile,
                                 DWORD Flags,
                                 LPD3DXBUFFER* ppShader,
                                 LPD3DXBUFFER* ppErrorMsgs,
                                 LPD3DXCONSTANTTABLE * ppConstantTable)
{
    HRESULT hr = D3DCompile(pSrcData, srcDataLen, NULL,
                            (D3D_SHADER_MACRO *)pDefines, (ID3DInclude *)pInclude,
                            pFunctionName, pProfile, Flags, 0,
                            (ID3DBlob **)ppShader, (ID3DBlob **)ppErrorMsgs);

    if(SUCCEEDED(hr) && ppConstantTable)
        return D3DXGetShaderConstantTable(ID3DXBuffer_GetBufferPointer(*ppShader),
                                          ppConstantTable);
    return hr;
}

HRESULT WINAPI D3DXCompileShaderFromFileA(LPCSTR filename,
                                          CONST D3DXMACRO* defines,
                                          LPD3DXINCLUDE include,
                                          LPCSTR entrypoint,
                                          LPCSTR profile,
                                          DWORD flags,
                                          LPD3DXBUFFER* shader,
                                          LPD3DXBUFFER* error_messages,
                                          LPD3DXCONSTANTTABLE* constant_table)
{
    LPWSTR filename_w = NULL;
    DWORD len;
    HRESULT ret;

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXCompileShaderFromFileW(filename_w, defines, include,
                                     entrypoint, profile, flags,
                                     shader, error_messages, constant_table);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXCompileShaderFromFileW(LPCWSTR filename,
                                          CONST D3DXMACRO* defines,
                                          LPD3DXINCLUDE include,
                                          LPCSTR entrypoint,
                                          LPCSTR profile,
                                          DWORD flags,
                                          LPD3DXBUFFER* shader,
                                          LPD3DXBUFFER* error_messages,
                                          LPD3DXCONSTANTTABLE* constant_table)
{
    void *buffer;
    DWORD len, filename_len;
    HRESULT hr;
    struct D3DXIncludeImpl includefromfile;
    char *filename_a;

    if (FAILED(map_view_of_file(filename, &buffer, &len)))
        return D3DXERR_INVALIDDATA;

    if (!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    filename_len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    filename_a = HeapAlloc(GetProcessHeap(), 0, filename_len * sizeof(char));
    if (!filename_a)
    {
        UnmapViewOfFile(buffer);
        return E_OUTOFMEMORY;
    }
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, filename_len, NULL, NULL);

    hr = D3DCompile(buffer, len, filename_a, (const D3D_SHADER_MACRO *)defines,
                    (ID3DInclude *)include, entrypoint, profile, flags, 0,
                    (ID3DBlob **)shader, (ID3DBlob **)error_messages);

    if (SUCCEEDED(hr) && constant_table)
        hr = D3DXGetShaderConstantTable(ID3DXBuffer_GetBufferPointer(*shader),
                                        constant_table);

    HeapFree(GetProcessHeap(), 0, filename_a);
    UnmapViewOfFile(buffer);
    return hr;
}

HRESULT WINAPI D3DXCompileShaderFromResourceA(HMODULE module,
                                              LPCSTR resource,
                                              CONST D3DXMACRO* defines,
                                              LPD3DXINCLUDE include,
                                              LPCSTR entrypoint,
                                              LPCSTR profile,
                                              DWORD flags,
                                              LPD3DXBUFFER* shader,
                                              LPD3DXBUFFER* error_messages,
                                              LPD3DXCONSTANTTABLE* constant_table)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceA(module, resource, (LPCSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXCompileShader(buffer, len, defines, include, entrypoint, profile,
                             flags, shader, error_messages, constant_table);
}

HRESULT WINAPI D3DXCompileShaderFromResourceW(HMODULE module,
                                              LPCWSTR resource,
                                              CONST D3DXMACRO* defines,
                                              LPD3DXINCLUDE include,
                                              LPCSTR entrypoint,
                                              LPCSTR profile,
                                              DWORD flags,
                                              LPD3DXBUFFER* shader,
                                              LPD3DXBUFFER* error_messages,
                                              LPD3DXCONSTANTTABLE* constant_table)
{
    HRSRC res;
    LPCSTR buffer;
    DWORD len;

    if (!(res = FindResourceW(module, resource, (LPCWSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXCompileShader(buffer, len, defines, include, entrypoint, profile,
                             flags, shader, error_messages, constant_table);
}

HRESULT WINAPI D3DXPreprocessShader(LPCSTR data,
                                    UINT data_len,
                                    CONST D3DXMACRO* defines,
                                    LPD3DXINCLUDE include,
                                    LPD3DXBUFFER* shader,
                                    LPD3DXBUFFER* error_messages)
{
    TRACE("Forward to D3DPreprocess\n");
    return D3DPreprocess(data, data_len, NULL,
                         (const D3D_SHADER_MACRO *)defines, (ID3DInclude *)include,
                         (ID3DBlob **)shader, (ID3DBlob **)error_messages);
}

HRESULT WINAPI D3DXPreprocessShaderFromFileA(LPCSTR filename,
                                             CONST D3DXMACRO* defines,
                                             LPD3DXINCLUDE include,
                                             LPD3DXBUFFER* shader,
                                             LPD3DXBUFFER* error_messages)
{
    WCHAR *filename_w = NULL;
    DWORD len;
    HRESULT ret;

    if (!filename) return D3DXERR_INVALIDDATA;

    len = MultiByteToWideChar(CP_ACP, 0, filename, -1, NULL, 0);
    filename_w = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!filename_w) return E_OUTOFMEMORY;
    MultiByteToWideChar(CP_ACP, 0, filename, -1, filename_w, len);

    ret = D3DXPreprocessShaderFromFileW(filename_w, defines, include, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, filename_w);
    return ret;
}

HRESULT WINAPI D3DXPreprocessShaderFromFileW(LPCWSTR filename,
                                             CONST D3DXMACRO* defines,
                                             LPD3DXINCLUDE include,
                                             LPD3DXBUFFER* shader,
                                             LPD3DXBUFFER* error_messages)
{
    void *buffer;
    DWORD len;
    HRESULT hr;
    struct D3DXIncludeImpl includefromfile;

    if (FAILED(map_view_of_file(filename, &buffer, &len)))
        return D3DXERR_INVALIDDATA;

    if (!include)
    {
        includefromfile.ID3DXInclude_iface.lpVtbl = &D3DXInclude_Vtbl;
        include = &includefromfile.ID3DXInclude_iface;
    }

    hr = D3DPreprocess(buffer, len, NULL,
                       (const D3D_SHADER_MACRO *)defines,
                       (ID3DInclude *) include,
                       (ID3DBlob **)shader, (ID3DBlob **)error_messages);

    UnmapViewOfFile(buffer);
    return hr;
}

HRESULT WINAPI D3DXPreprocessShaderFromResourceA(HMODULE module,
                                                 LPCSTR resource,
                                                 CONST D3DXMACRO* defines,
                                                 LPD3DXINCLUDE include,
                                                 LPD3DXBUFFER* shader,
                                                 LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    const char *buffer;
    DWORD len;

    if (!(res = FindResourceA(module, resource, (LPCSTR)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (LPVOID *)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXPreprocessShader(buffer, len, defines, include,
                                shader, error_messages);
}

HRESULT WINAPI D3DXPreprocessShaderFromResourceW(HMODULE module,
                                                 LPCWSTR resource,
                                                 CONST D3DXMACRO* defines,
                                                 LPD3DXINCLUDE include,
                                                 LPD3DXBUFFER* shader,
                                                 LPD3DXBUFFER* error_messages)
{
    HRSRC res;
    const char *buffer;
    DWORD len;

    if (!(res = FindResourceW(module, resource, (const WCHAR *)RT_RCDATA)))
        return D3DXERR_INVALIDDATA;
    if (FAILED(load_resource_into_memory(module, res, (void **)&buffer, &len)))
        return D3DXERR_INVALIDDATA;
    return D3DXPreprocessShader(buffer, len, defines, include,
                                shader, error_messages);

}

struct ctab_constant {
    D3DXCONSTANT_DESC desc;
    struct ctab_constant *constants;
};

static const struct ID3DXConstantTableVtbl ID3DXConstantTable_Vtbl;

struct ID3DXConstantTableImpl {
    ID3DXConstantTable ID3DXConstantTable_iface;
    LONG ref;
    char *ctab;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;
    struct ctab_constant *constants;
};

static void free_constant(struct ctab_constant *constant)
{
    if (constant->constants)
    {
        UINT i, count = constant->desc.Elements > 1 ? constant->desc.Elements : constant->desc.StructMembers;

        for (i = 0; i < count; ++i)
        {
            free_constant(&constant->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, constant->constants);
    }
}

static void free_constant_table(struct ID3DXConstantTableImpl *table)
{
    if (table->constants)
    {
        UINT i;

        for (i = 0; i < table->desc.Constants; ++i)
        {
            free_constant(&table->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, table->constants);
    }
    HeapFree(GetProcessHeap(), 0, table->ctab);
}

static inline struct ID3DXConstantTableImpl *impl_from_ID3DXConstantTable(ID3DXConstantTable *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXConstantTableImpl, ID3DXConstantTable_iface);
}

static inline int is_vertex_shader(DWORD version)
{
    return (version & 0xFFFF0000) == 0xFFFE0000;
}

static DWORD calc_bytes(D3DXCONSTANT_DESC *desc)
{
    if (desc->RegisterSet != D3DXRS_FLOAT4 && desc->RegisterSet != D3DXRS_SAMPLER)
        FIXME("Don't know how to calculate Bytes for constants of type %s\n",
                debug_d3dxparameter_registerset(desc->RegisterSet));

    return 4 * desc->Elements * desc->Rows * desc->Columns;
}

static inline struct ctab_constant *constant_from_handle(D3DXHANDLE handle)
{
    return (struct ctab_constant *)handle;
}

static inline D3DXHANDLE handle_from_constant(struct ctab_constant *constant)
{
    return (D3DXHANDLE)constant;
}

static struct ctab_constant *get_constant_by_name(struct ID3DXConstantTableImpl *, struct ctab_constant *, LPCSTR);

static struct ctab_constant *get_constant_element_by_name(struct ctab_constant *constant, LPCSTR name)
{
    UINT element;
    LPCSTR part;

    TRACE("constant %p, name %s\n", constant, debugstr_a(name));

    if (!name || !*name) return NULL;

    element = atoi(name);
    part = strchr(name, ']') + 1;

    if (constant->desc.Elements > element)
    {
        struct ctab_constant *c = constant->constants ? &constant->constants[element] : constant;

        switch (*part++)
        {
            case '.':
                return get_constant_by_name(NULL, c, part);

            case '[':
                return get_constant_element_by_name(c, part);

            case '\0':
                TRACE("Returning parameter %p\n", c);
                return c;

            default:
                FIXME("Unhandled case \"%c\"\n", *--part);
                break;
        }
    }

    TRACE("Constant not found\n");
    return NULL;
}

static struct ctab_constant *get_constant_by_name(struct ID3DXConstantTableImpl *table,
        struct ctab_constant *constant, LPCSTR name)
{
    UINT i, count, length;
    struct ctab_constant *handles;
    LPCSTR part;

    TRACE("table %p, constant %p, name %s\n", table, constant, debugstr_a(name));

    if (!name || !*name) return NULL;

    if (!constant)
    {
        count = table->desc.Constants;
        handles = table->constants;
    }
    else
    {
        count = constant->desc.StructMembers;
        handles = constant->constants;
    }

    length = strcspn(name, "[.");
    part = name + length;

    for (i = 0; i < count; i++)
    {
        if (strlen(handles[i].desc.Name) == length && !strncmp(handles[i].desc.Name, name, length))
        {
            switch (*part++)
            {
                case '.':
                    return get_constant_by_name(NULL, &handles[i], part);

                case '[':
                    return get_constant_element_by_name(&handles[i], part);

                default:
                    TRACE("Returning parameter %p\n", &handles[i]);
                    return &handles[i];
            }
        }
    }

    TRACE("Constant not found\n");
    return NULL;
}

static struct ctab_constant *is_valid_sub_constant(struct ctab_constant *parent, struct ctab_constant *constant)
{
    UINT i, count;

    /* all variable have at least elements = 1, but no elements */
    if (!parent->constants) return NULL;

    if (parent->desc.Elements > 1) count = parent->desc.Elements;
    else count = parent->desc.StructMembers;

    for (i = 0; i < count; ++i)
    {
        if (&parent->constants[i] == constant)
            return constant;

        if (is_valid_sub_constant(&parent->constants[i], constant))
            return constant;
    }

    return NULL;
}

static inline struct ctab_constant *is_valid_constant(struct ID3DXConstantTableImpl *table, D3DXHANDLE handle)
{
    struct ctab_constant *c = constant_from_handle(handle);
    UINT i;

    if (!c) return NULL;

    for (i = 0; i < table->desc.Constants; ++i)
    {
        if (&table->constants[i] == c)
            return c;

        if (is_valid_sub_constant(&table->constants[i], c))
            return c;
    }

    return NULL;
}

static inline struct ctab_constant *get_valid_constant(struct ID3DXConstantTableImpl *table, D3DXHANDLE handle)
{
    struct ctab_constant *constant = is_valid_constant(table, handle);

    if (!constant) constant = get_constant_by_name(table, NULL, handle);

    return constant;
}

static inline void set_float_shader_constant(struct ID3DXConstantTableImpl *table, IDirect3DDevice9 *device,
                                             UINT register_index, const FLOAT *data, UINT count)
{
    if (is_vertex_shader(table->desc.Version))
        IDirect3DDevice9_SetVertexShaderConstantF(device, register_index, data, count);
    else
        IDirect3DDevice9_SetPixelShaderConstantF(device, register_index, data, count);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_QueryInterface(ID3DXConstantTable *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBuffer) ||
        IsEqualGUID(riid, &IID_ID3DXConstantTable))
    {
        ID3DXConstantTable_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXConstantTableImpl_AddRef(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(): AddRef from %d\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXConstantTableImpl_Release(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %d\n", This, ref + 1);

    if (!ref)
    {
        free_constant_table(This);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBuffer methods ***/
static LPVOID WINAPI ID3DXConstantTableImpl_GetBufferPointer(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->()\n", This);

    return This->ctab;
}

static DWORD WINAPI ID3DXConstantTableImpl_GetBufferSize(ID3DXConstantTable *iface)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->()\n", This);

    return This->size;
}

/*** ID3DXConstantTable methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_GetDesc(ID3DXConstantTable *iface, D3DXCONSTANTTABLE_DESC *desc)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p)\n", This, desc);

    if (!desc)
        return D3DERR_INVALIDCALL;

    *desc = This->desc;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXConstantTableImpl_GetConstantDesc(ID3DXConstantTable *iface, D3DXHANDLE constant,
                                                             D3DXCONSTANT_DESC *desc, UINT *count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p, %p, %p)\n", This, constant, desc, count);

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }

    if (desc) *desc = c->desc;
    if (count) *count = 1;

    return D3D_OK;
}

static UINT WINAPI ID3DXConstantTableImpl_GetSamplerIndex(ID3DXConstantTable *iface, D3DXHANDLE constant)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p)\n", This, constant);

    if (!c || c->desc.RegisterSet != D3DXRS_SAMPLER)
    {
        WARN("Invalid argument specified\n");
        return (UINT)-1;
    }

    TRACE("Returning RegisterIndex %u\n", c->desc.RegisterIndex);
    return c->desc.RegisterIndex;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstant(ID3DXConstantTable *iface, D3DXHANDLE constant, UINT index)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c;

    TRACE("(%p)->(%p, %d)\n", This, constant, index);

    if (constant)
    {
        c = get_valid_constant(This, constant);
        if (c && index < c->desc.StructMembers)
        {
            c = &c->constants[index];
            TRACE("Returning constant %p\n", c);
            return handle_from_constant(c);
        }
    }
    else
    {
        if (index < This->desc.Constants)
        {
            c = &This->constants[index];
            TRACE("Returning constant %p\n", c);
            return handle_from_constant(c);
        }
    }

    WARN("Index out of range\n");
    return NULL;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantByName(ID3DXConstantTable *iface, D3DXHANDLE constant, LPCSTR name)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p, %s)\n", This, constant, name);

    c = get_constant_by_name(This, c, name);
    TRACE("Returning constant %p\n", c);

    return handle_from_constant(c);
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantElement(ID3DXConstantTable *iface, D3DXHANDLE constant, UINT index)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);

    TRACE("(%p)->(%p, %d)\n", This, constant, index);

    if (c && index < c->desc.Elements)
    {
        if (c->desc.Elements > 1) c = &c->constants[index];
        TRACE("Returning constant %p\n", c);
        return handle_from_constant(c);
    }

    WARN("Invalid argument specified\n");
    return NULL;
}

static HRESULT set_scalar_array(ID3DXConstantTable *iface, IDirect3DDevice9 *device, D3DXHANDLE constant, const void *data,
                               UINT count, D3DXPARAMETER_TYPE type)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    D3DXCONSTANT_DESC desc;
    HRESULT hr;
    UINT i, desc_count = 1;
    float row[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    hr = ID3DXConstantTable_GetConstantDesc(iface, constant, &desc, &desc_count);
    if (FAILED(hr))
    {
        TRACE("ID3DXConstantTable_GetConstantDesc failed: %08x\n", hr);
        return D3DERR_INVALIDCALL;
    }

    if (desc.Class != D3DXPC_SCALAR)
        return D3D_OK;

    switch (desc.RegisterSet)
    {
        case D3DXRS_FLOAT4:
            for (i = 0; i < min(count, desc.RegisterCount); i++)
            {
                /* We need the for loop since each IDirect3DDevice9_Set*ShaderConstantF expects a float4 */
                switch(type)
                {
                    case D3DXPT_FLOAT:
                        row[0] = ((float *)data)[i];
                        break;
                    case D3DXPT_INT:
                        row[0] = (float)((int *)data)[i];
                        break;
                    case D3DXPT_BOOL:
                        row[0] = ((BOOL *)data)[i] ? 1.0f : 0.0f;
                        break;
                    default:
                        FIXME("Unhandled type %s\n", debug_d3dxparameter_type(type));
                        return D3DERR_INVALIDCALL;
                }
                set_float_shader_constant(This, device, desc.RegisterIndex + i, row, 1);
            }
            break;
        default:
            FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc.RegisterSet));
            return E_NOTIMPL;
    }

    return D3D_OK;
}

static HRESULT set_vector_array(ID3DXConstantTable *iface, IDirect3DDevice9 *device, D3DXHANDLE constant, const void *data,
                                UINT count, D3DXPARAMETER_TYPE type)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    D3DXCONSTANT_DESC desc;
    HRESULT hr;
    UINT i, j, desc_count = 1;
    float vec[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    hr = ID3DXConstantTable_GetConstantDesc(iface, constant, &desc, &desc_count);
    if (FAILED(hr))
    {
        TRACE("ID3DXConstantTable_GetConstantDesc failed: %08x\n", hr);
        return D3DERR_INVALIDCALL;
    }

    if (desc.Class == D3DXPC_MATRIX_ROWS || desc.Class == D3DXPC_MATRIX_COLUMNS)
        return D3D_OK;

    switch (desc.RegisterSet)
    {
        case D3DXRS_FLOAT4:
            for (i = 0; i < min(count, desc.RegisterCount); i++)
            {
                switch (type)
                {
                    case D3DXPT_FLOAT:
                        memcpy(vec, ((float *)data) + i * desc.Columns, desc.Columns * sizeof(float));
                        break;
                    case D3DXPT_INT:
                        for (j = 0; j < desc.Columns; j++)
                            vec[j] = (float)((int *)data)[i * desc.Columns + j];
                        break;
                    case D3DXPT_BOOL:
                        for (j = 0; j < desc.Columns; j++)
                            vec[j] = ((BOOL *)data)[i * desc.Columns + j] ? 1.0f : 0.0f;
                        break;
                    default:
                        FIXME("Unhandled type %s\n", debug_d3dxparameter_type(type));
                        return D3DERR_INVALIDCALL;
                }

                set_float_shader_constant(This, device, desc.RegisterIndex + i, vec, 1);
            }
            break;
        default:
            FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc.RegisterSet));
            return E_NOTIMPL;
    }

    return D3D_OK;
}

static HRESULT set_float_matrix(FLOAT *matrix, const D3DXCONSTANT_DESC *desc,
                                UINT row_offset, UINT column_offset, UINT rows, UINT columns,
                                const void *data, D3DXPARAMETER_TYPE type, UINT src_columns)
{
    UINT i, j;

    switch (type)
    {
        case D3DXPT_FLOAT:
            for (i = 0; i < rows; i++)
            {
                for (j = 0; j < columns; j++)
                    matrix[i * row_offset + j * column_offset] = ((FLOAT *)data)[i * src_columns + j];
            }
            break;
        case D3DXPT_INT:
            for (i = 0; i < rows; i++)
            {
                for (j = 0; j < columns; j++)
                    matrix[i * row_offset + j * column_offset] = ((INT *)data)[i * src_columns + j];
            }
            break;
        default:
            FIXME("Unhandled type %s\n", debug_d3dxparameter_type(type));
            return D3DERR_INVALIDCALL;
    }

    return D3D_OK;
}

static HRESULT set_matrix_array(ID3DXConstantTable *iface, IDirect3DDevice9 *device, D3DXHANDLE constant, const void *data,
                                UINT count, D3DXPARAMETER_CLASS class, D3DXPARAMETER_TYPE type, UINT rows, UINT columns)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    struct ctab_constant *c = get_valid_constant(This, constant);
    D3DXCONSTANT_DESC *desc;
    UINT registers_per_matrix, num_rows, num_columns, i;
    UINT row_offset = 1, column_offset = 1;
    const DWORD *data_ptr;
    FLOAT matrix[16] = {0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f};

    if (!c)
    {
        WARN("Invalid argument specified\n");
        return D3DERR_INVALIDCALL;
    }
    desc = &c->desc;

    if (desc->Class == D3DXPC_MATRIX_ROWS
        || desc->Class == D3DXPC_MATRIX_COLUMNS
        || desc->Class == D3DXPC_VECTOR
        || desc->Class == D3DXPC_SCALAR)
    {
        if (desc->Class == class) row_offset = 4;
        else column_offset = 4;

        if (class == D3DXPC_MATRIX_ROWS)
        {
            if (desc->Class == D3DXPC_VECTOR) return D3D_OK;

            num_rows = desc->Rows;
            num_columns = desc->Columns;
        }
        else
        {
            num_rows = desc->Columns;
            num_columns = desc->Rows;
        }

        registers_per_matrix = (desc->Class == D3DXPC_MATRIX_COLUMNS) ? desc->Columns : desc->Rows;
    }
    else
    {
        FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc->Class));
        return E_NOTIMPL;
    }

    switch (desc->RegisterSet)
    {
        case D3DXRS_FLOAT4:
            data_ptr = data;
            for (i = 0; i < count; i++)
            {
                HRESULT hr;

                if (registers_per_matrix * (i + 1) > desc->RegisterCount)
                    break;

                hr = set_float_matrix(matrix, desc, row_offset, column_offset, num_rows, num_columns, data_ptr, type, columns);
                if (FAILED(hr)) return hr;

                set_float_shader_constant(This, device, desc->RegisterIndex + i * registers_per_matrix, matrix, registers_per_matrix);

                data_ptr += rows * columns;
            }
            break;
        default:
            FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc->RegisterSet));
            return E_NOTIMPL;
    }

    return D3D_OK;
}

static HRESULT set_matrix_pointer_array(ID3DXConstantTable *iface, IDirect3DDevice9 *device, D3DXHANDLE constant,
                                const D3DXMATRIX **data, UINT count, D3DXPARAMETER_CLASS class)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    D3DXCONSTANT_DESC desc;
    HRESULT hr;
    UINT registers_per_matrix;
    UINT i, desc_count = 1;
    UINT num_rows, num_columns;
    UINT row_offset, column_offset;
    FLOAT matrix[16] = {0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 0.0f};

    hr = ID3DXConstantTable_GetConstantDesc(iface, constant, &desc, &desc_count);
    if (FAILED(hr))
    {
        TRACE("ID3DXConstantTable_GetConstantDesc failed: %08x\n", hr);
        return D3DERR_INVALIDCALL;
    }

    if (desc.Class == D3DXPC_MATRIX_ROWS || desc.Class == D3DXPC_MATRIX_COLUMNS)
    {
        if (desc.Class == class)
        {
            column_offset = 1;
            row_offset = 4;
        }
        else
        {
            column_offset = 4;
            row_offset = 1;
        }

        if (class == D3DXPC_MATRIX_ROWS)
        {
            num_rows = desc.Rows;
            num_columns = desc.Columns;
        }
        else
        {
            num_rows = desc.Columns;
            num_columns = desc.Rows;
        }

        registers_per_matrix = (desc.Class == D3DXPC_MATRIX_ROWS) ? desc.Rows : desc.Columns;
    }
    else if (desc.Class == D3DXPC_SCALAR)
    {
        registers_per_matrix = 1;
        column_offset = 1;
        row_offset = 1;
        num_rows = desc.Rows;
        num_columns = desc.Columns;
    }
    else if (desc.Class == D3DXPC_VECTOR)
    {
        registers_per_matrix = 1;

        if (class == D3DXPC_MATRIX_ROWS)
        {
            column_offset = 1;
            row_offset = 4;
            num_rows = desc.Rows;
            num_columns = desc.Columns;
        }
        else
        {
            column_offset = 4;
            row_offset = 1;
            num_rows = desc.Columns;
            num_columns = desc.Rows;
        }
    }
    else
    {
        FIXME("Unhandled variable class %s\n", debug_d3dxparameter_class(desc.Class));
        return D3D_OK;
    }

    switch (desc.RegisterSet)
    {
        case D3DXRS_FLOAT4:
            for (i = 0; i < count; i++)
            {
                if (registers_per_matrix * (i + 1) > desc.RegisterCount)
                    break;

                hr = set_float_matrix(matrix, &desc, row_offset, column_offset, num_rows, num_columns, *data, D3DXPT_FLOAT, 4);
                if (FAILED(hr)) return hr;

                set_float_shader_constant(This, device, desc.RegisterIndex + i * registers_per_matrix, matrix, registers_per_matrix);

                data++;
            }
            break;
        default:
            FIXME("Unhandled register set %s\n", debug_d3dxparameter_registerset(desc.RegisterSet));
            return E_NOTIMPL;
    }

    return D3D_OK;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetDefaults(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    UINT i;

    TRACE("(%p)->(%p)\n", This, device);

    if (!device)
        return D3DERR_INVALIDCALL;

    for (i = 0; i < This->desc.Constants; i++)
    {
        D3DXCONSTANT_DESC *desc = &This->constants[i].desc;

        if (!desc->DefaultValue)
            continue;

        set_float_shader_constant(This, device, desc->RegisterIndex, desc->DefaultValue, desc->RegisterCount);
    }

    return D3D_OK;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetValue(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                      D3DXHANDLE constant, LPCVOID data, UINT bytes)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);
    HRESULT hr;
    UINT elements;
    UINT count = 1;
    D3DXCONSTANT_DESC desc;

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, data, bytes);

    if (!device || !constant || !data)
        return D3DERR_INVALIDCALL;

    hr = ID3DXConstantTable_GetConstantDesc(iface, constant, &desc, &count);
    if (FAILED(hr))
        return hr;

    elements = bytes / (desc.Bytes / desc.Elements);

    switch (desc.Class)
    {
        case D3DXPC_SCALAR:
            return set_scalar_array(iface, device, constant, data, elements, desc.Type);
        case D3DXPC_VECTOR:
            return set_vector_array(iface, device, constant, data, elements, desc.Type);
        case D3DXPC_MATRIX_ROWS:
        case D3DXPC_MATRIX_COLUMNS:
            return set_matrix_array(iface, device, constant, data, elements,
                    D3DXPC_MATRIX_ROWS, desc.Type, desc.Rows, desc.Columns);
        default:
            FIXME("Unhandled parameter class %s\n", debug_d3dxparameter_class(desc.Class));
            return D3DERR_INVALIDCALL;
    }
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBool(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                     D3DXHANDLE constant, BOOL b)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %d)\n", This, device, constant, b);

    return set_scalar_array(iface, device, constant, &b, 1, D3DXPT_BOOL);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBoolArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                          D3DXHANDLE constant, CONST BOOL* b, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, b, count);

    return set_scalar_array(iface, device, constant, b, count, D3DXPT_BOOL);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetInt(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device, D3DXHANDLE constant, INT n)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %d)\n", This, device, constant, n);

    return set_scalar_array(iface, device, constant, &n, 1, D3DXPT_INT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetIntArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                         D3DXHANDLE constant, CONST INT* n, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, n, count);

    return set_scalar_array(iface, device, constant, n, count, D3DXPT_INT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloat(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                      D3DXHANDLE constant, FLOAT f)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %f)\n", This, device, constant, f);

    return set_scalar_array(iface, device, constant, &f, 1, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloatArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                           D3DXHANDLE constant, CONST FLOAT *f, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, f, count);

    return set_scalar_array(iface, device, constant, f, count, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVector(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                       D3DXHANDLE constant, CONST D3DXVECTOR4 *vector)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p)\n", This, device, constant, vector);

    return set_vector_array(iface, device, constant, vector, 1, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVectorArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                            D3DXHANDLE constant, CONST D3DXVECTOR4 *vector, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, vector, count);

    return set_vector_array(iface, device, constant, vector, count, D3DXPT_FLOAT);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrix(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                       D3DXHANDLE constant, CONST D3DXMATRIX *matrix)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p)\n", This, device, constant, matrix);

    return set_matrix_array(iface, device, constant, matrix, 1, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 4);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                            D3DXHANDLE constant, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, matrix, count);

    return set_matrix_array(iface, device, constant, matrix, count, D3DXPC_MATRIX_ROWS, D3DXPT_FLOAT, 4, 4);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixPointerArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                                   D3DXHANDLE constant, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, matrix, count);

    return set_matrix_pointer_array(iface, device, constant, matrix, count, D3DXPC_MATRIX_ROWS);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTranspose(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                                D3DXHANDLE constant, CONST D3DXMATRIX *matrix)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p)\n", This, device, constant, matrix);

    return set_matrix_array(iface, device, constant, matrix, 1, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposeArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                                     D3DXHANDLE constant, CONST D3DXMATRIX *matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, matrix, count);

    return set_matrix_array(iface, device, constant, matrix, count, D3DXPC_MATRIX_COLUMNS, D3DXPT_FLOAT, 4, 4);
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposePointerArray(ID3DXConstantTable *iface, LPDIRECT3DDEVICE9 device,
                                                                            D3DXHANDLE constant, CONST D3DXMATRIX **matrix, UINT count)
{
    struct ID3DXConstantTableImpl *This = impl_from_ID3DXConstantTable(iface);

    TRACE("(%p)->(%p, %p, %p, %d)\n", This, device, constant, matrix, count);

    return set_matrix_pointer_array(iface, device, constant, matrix, count, D3DXPC_MATRIX_COLUMNS);
}

static const struct ID3DXConstantTableVtbl ID3DXConstantTable_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXConstantTableImpl_QueryInterface,
    ID3DXConstantTableImpl_AddRef,
    ID3DXConstantTableImpl_Release,
    /*** ID3DXBuffer methods ***/
    ID3DXConstantTableImpl_GetBufferPointer,
    ID3DXConstantTableImpl_GetBufferSize,
    /*** ID3DXConstantTable methods ***/
    ID3DXConstantTableImpl_GetDesc,
    ID3DXConstantTableImpl_GetConstantDesc,
    ID3DXConstantTableImpl_GetSamplerIndex,
    ID3DXConstantTableImpl_GetConstant,
    ID3DXConstantTableImpl_GetConstantByName,
    ID3DXConstantTableImpl_GetConstantElement,
    ID3DXConstantTableImpl_SetDefaults,
    ID3DXConstantTableImpl_SetValue,
    ID3DXConstantTableImpl_SetBool,
    ID3DXConstantTableImpl_SetBoolArray,
    ID3DXConstantTableImpl_SetInt,
    ID3DXConstantTableImpl_SetIntArray,
    ID3DXConstantTableImpl_SetFloat,
    ID3DXConstantTableImpl_SetFloatArray,
    ID3DXConstantTableImpl_SetVector,
    ID3DXConstantTableImpl_SetVectorArray,
    ID3DXConstantTableImpl_SetMatrix,
    ID3DXConstantTableImpl_SetMatrixArray,
    ID3DXConstantTableImpl_SetMatrixPointerArray,
    ID3DXConstantTableImpl_SetMatrixTranspose,
    ID3DXConstantTableImpl_SetMatrixTransposeArray,
    ID3DXConstantTableImpl_SetMatrixTransposePointerArray
};

static HRESULT parse_ctab_constant_type(const char *ctab, DWORD typeoffset, struct ctab_constant *constant,
        BOOL is_element, WORD index, WORD max, DWORD *offset, DWORD nameoffset, UINT regset)
{
    const D3DXSHADER_TYPEINFO *type = (LPD3DXSHADER_TYPEINFO)(ctab + typeoffset);
    const D3DXSHADER_STRUCTMEMBERINFO *memberinfo = NULL;
    HRESULT hr = D3D_OK;
    UINT i, count = 0;
    WORD size = 0;

    constant->desc.DefaultValue = offset ? ctab + *offset : NULL;
    constant->desc.Class = type->Class;
    constant->desc.Type = type->Type;
    constant->desc.Rows = type->Rows;
    constant->desc.Columns = type->Columns;
    constant->desc.Elements = is_element ? 1 : type->Elements;
    constant->desc.StructMembers = type->StructMembers;
    constant->desc.Name = ctab + nameoffset;
    constant->desc.RegisterSet = regset;
    constant->desc.RegisterIndex = index;

    TRACE("name %s, elements %u, index %u, defaultvalue %p, regset %s\n", constant->desc.Name,
            constant->desc.Elements, index, constant->desc.DefaultValue,
            debug_d3dxparameter_registerset(regset));
    TRACE("class %s, type %s, rows %d, columns %d, elements %d, struct_members %d\n",
            debug_d3dxparameter_class(type->Class), debug_d3dxparameter_type(type->Type),
            type->Rows, type->Columns, type->Elements, type->StructMembers);

    if (type->Elements > 1 && !is_element)
    {
        count = type->Elements;
    }
    else if ((type->Class == D3DXPC_STRUCT) && type->StructMembers)
    {
        memberinfo = (D3DXSHADER_STRUCTMEMBERINFO*)(ctab + type->StructMemberInfo);
        count = type->StructMembers;
    }

    if (count)
    {
        constant->constants = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*constant->constants) * count);
        if (!constant->constants)
        {
             ERR("Out of memory\n");
             hr = E_OUTOFMEMORY;
             goto error;
        }

        for (i = 0; i < count; ++i)
        {
            hr = parse_ctab_constant_type(ctab, memberinfo ? memberinfo[i].TypeInfo : typeoffset,
                    &constant->constants[i], memberinfo == NULL, index + size, max, offset,
                    memberinfo ? memberinfo[i].Name : nameoffset, regset);
            if (hr != D3D_OK)
                goto error;

            size += constant->constants[i].desc.RegisterCount;
        }
    }
    else
    {
        WORD offsetdiff = 0;

        switch (type->Class)
        {
            case D3DXPC_SCALAR:
            case D3DXPC_VECTOR:
                offsetdiff = 1;
                size = 1;
                break;

            case D3DXPC_MATRIX_ROWS:
                size = is_element ? type->Rows : max(type->Rows, type->Columns);
                offsetdiff = type->Rows;
                break;

            case D3DXPC_MATRIX_COLUMNS:
                size = type->Columns;
                offsetdiff = type->Columns;
                break;

            case D3DXPC_OBJECT:
                size = 1;
                break;

            default:
                FIXME("Unhandled type class %s\n", debug_d3dxparameter_class(type->Class));
                break;
        }

        /* offset in bytes => offsetdiff * components(4) * sizeof(DWORD) */
        if (offset) *offset += offsetdiff * 4 * 4;
    }

    constant->desc.RegisterCount = max(0, min(max - index, size));
    constant->desc.Bytes = calc_bytes(&constant->desc);

    return D3D_OK;

error:
    if (constant->constants)
    {
        for (i = 0; i < count; ++i)
        {
            free_constant(&constant->constants[i]);
        }
        HeapFree(GetProcessHeap(), 0, constant->constants);
        constant->constants = NULL;
    }

    return hr;
}

HRESULT WINAPI D3DXGetShaderConstantTableEx(CONST DWORD *byte_code,
                                            DWORD flags,
                                            LPD3DXCONSTANTTABLE *constant_table)
{
    struct ID3DXConstantTableImpl *object = NULL;
    HRESULT hr;
    LPCVOID data;
    UINT size;
    const D3DXSHADER_CONSTANTTABLE* ctab_header;
    D3DXSHADER_CONSTANTINFO* constant_info;
    DWORD i;

    TRACE("(%p, %x, %p)\n", byte_code, flags, constant_table);

    if (!byte_code || !constant_table)
    {
        WARN("Invalid argument specified.\n");
        return D3DERR_INVALIDCALL;
    }

    if (flags) FIXME("Flags (%#x) are not handled, yet!\n", flags);

    hr = D3DXFindShaderComment(byte_code, MAKEFOURCC('C','T','A','B'), &data, &size);
    if (hr != D3D_OK)
    {
        WARN("CTAB not found.\n");
        return D3DXERR_INVALIDDATA;
    }

    if (size < sizeof(D3DXSHADER_CONSTANTTABLE))
    {
        WARN("Invalid CTAB size.\n");
        return D3DXERR_INVALIDDATA;
    }

    ctab_header = (const D3DXSHADER_CONSTANTTABLE *)data;
    if (ctab_header->Size != sizeof(D3DXSHADER_CONSTANTTABLE))
    {
        WARN("Invalid D3DXSHADER_CONSTANTTABLE size.\n");
        return D3DXERR_INVALIDDATA;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->ID3DXConstantTable_iface.lpVtbl = &ID3DXConstantTable_Vtbl;
    object->ref = 1;

    object->ctab = HeapAlloc(GetProcessHeap(), 0, size);
    if (!object->ctab)
    {
        ERR("Out of memory\n");
        HeapFree(GetProcessHeap(), 0, object);
        return E_OUTOFMEMORY;
    }
    object->size = size;
    memcpy(object->ctab, data, object->size);

    object->desc.Creator = ctab_header->Creator ? object->ctab + ctab_header->Creator : NULL;
    object->desc.Version = ctab_header->Version;
    object->desc.Constants = ctab_header->Constants;
    TRACE("Creator %s, Version %x, Constants %u, Target %s\n",
            debugstr_a(object->desc.Creator), object->desc.Version, object->desc.Constants,
            debugstr_a(ctab_header->Target ? object->ctab + ctab_header->Target : NULL));

    object->constants = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  sizeof(*object->constants) * object->desc.Constants);
    if (!object->constants)
    {
         ERR("Out of memory\n");
         hr = E_OUTOFMEMORY;
         goto error;
    }

    constant_info = (LPD3DXSHADER_CONSTANTINFO)(object->ctab + ctab_header->ConstantInfo);
    for (i = 0; i < ctab_header->Constants; i++)
    {
        DWORD offset = constant_info[i].DefaultValue;

        hr = parse_ctab_constant_type(object->ctab, constant_info[i].TypeInfo,
                &object->constants[i], FALSE, constant_info[i].RegisterIndex,
                constant_info[i].RegisterIndex + constant_info[i].RegisterCount,
                offset ? &offset : NULL, constant_info[i].Name, constant_info[i].RegisterSet);
        if (hr != D3D_OK)
            goto error;
    }

    *constant_table = &object->ID3DXConstantTable_iface;

    return D3D_OK;

error:
    free_constant_table(object);
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}

HRESULT WINAPI D3DXGetShaderConstantTable(CONST DWORD* byte_code,
                                          LPD3DXCONSTANTTABLE* constant_table)
{
    TRACE("(%p, %p): Forwarded to D3DXGetShaderConstantTableEx\n", byte_code, constant_table);

    return D3DXGetShaderConstantTableEx(byte_code, 0, constant_table);
}

HRESULT WINAPI D3DXGetShaderSamplers(CONST DWORD *byte_code, LPCSTR *samplers, UINT *count)
{
    HRESULT hr;
    UINT i, sampler_count = 0;
    UINT size;
    LPCSTR data;
    const D3DXSHADER_CONSTANTTABLE *ctab_header;
    const D3DXSHADER_CONSTANTINFO *constant_info;

    TRACE("byte_code %p, samplers %p, count %p\n", byte_code, samplers, count);

    if (count) *count = 0;

    hr = D3DXFindShaderComment(byte_code, MAKEFOURCC('C','T','A','B'), (LPCVOID *)&data, &size);
    if (hr != D3D_OK) return D3D_OK;

    if (size < sizeof(D3DXSHADER_CONSTANTTABLE)) return D3D_OK;

    ctab_header = (const D3DXSHADER_CONSTANTTABLE *)data;
    if (ctab_header->Size != sizeof(*ctab_header)) return D3D_OK;

    constant_info = (D3DXSHADER_CONSTANTINFO *)(data + ctab_header->ConstantInfo);
    for (i = 0; i < ctab_header->Constants; i++)
    {
        const D3DXSHADER_TYPEINFO *type;

        TRACE("name = %s\n", data + constant_info[i].Name);

        type = (D3DXSHADER_TYPEINFO *)(data + constant_info[i].TypeInfo);

        if (type->Type == D3DXPT_SAMPLER
                || type->Type == D3DXPT_SAMPLER1D
                || type->Type == D3DXPT_SAMPLER2D
                || type->Type == D3DXPT_SAMPLER3D
                || type->Type == D3DXPT_SAMPLERCUBE)
        {
            if (samplers) samplers[sampler_count] = data + constant_info[i].Name;

            ++sampler_count;
        }
    }

    TRACE("Found %u samplers\n", sampler_count);

    if (count) *count = sampler_count;

    return D3D_OK;
}
