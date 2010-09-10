/*
 * Copyright 2008 Luis Busquets
 * Copyright 2009 Matteo Bruni
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
    const ID3DXIncludeVtbl *lpVtbl;
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
        includefromfile.lpVtbl = &D3DXInclude_Vtbl;
        include = (LPD3DXINCLUDE)&includefromfile;
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
        includefromfile.lpVtbl = &D3DXInclude_Vtbl;
        include = (LPD3DXINCLUDE)&includefromfile;
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
        includefromfile.lpVtbl = &D3DXInclude_Vtbl;
        include = (LPD3DXINCLUDE)&includefromfile;
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

typedef struct ctab_constant {
    D3DXCONSTANT_DESC desc;
    struct ctab_constant *members;
} ctab_constant;

static const struct ID3DXConstantTableVtbl ID3DXConstantTable_Vtbl;

typedef struct ID3DXConstantTableImpl {
    const ID3DXConstantTableVtbl *lpVtbl;
    LONG ref;
    char *ctab;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;
    ctab_constant *constants;
} ID3DXConstantTableImpl;

/*** IUnknown methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_QueryInterface(ID3DXConstantTable* iface, REFIID riid, void** ppvObject)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ID3DXBuffer) ||
        IsEqualGUID(riid, &IID_ID3DXConstantTable))
    {
        ID3DXConstantTable_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }

    WARN("Interface %s not found.\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXConstantTableImpl_AddRef(ID3DXConstantTable* iface)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->(): AddRef from %d\n", This, This->ref);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ID3DXConstantTableImpl_Release(ID3DXConstantTable* iface)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): Release from %d\n", This, ref + 1);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->constants);
        HeapFree(GetProcessHeap(), 0, This->ctab);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ID3DXBuffer methods ***/
static LPVOID WINAPI ID3DXConstantTableImpl_GetBufferPointer(ID3DXConstantTable* iface)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->()\n", This);

    return This->ctab;
}

static DWORD WINAPI ID3DXConstantTableImpl_GetBufferSize(ID3DXConstantTable* iface)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->()\n", This);

    return This->size;
}

/*** ID3DXConstantTable methods ***/
static HRESULT WINAPI ID3DXConstantTableImpl_GetDesc(ID3DXConstantTable* iface, D3DXCONSTANTTABLE_DESC *desc)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->(%p)\n", This, desc);

    if (!desc)
        return D3DERR_INVALIDCALL;

    memcpy(desc, &This->desc, sizeof(This->desc));

    return D3D_OK;
}

static HRESULT WINAPI ID3DXConstantTableImpl_GetConstantDesc(ID3DXConstantTable* iface, D3DXHANDLE constant,
                                                             D3DXCONSTANT_DESC *desc, UINT *count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;
    ctab_constant *constant_info;

    TRACE("(%p)->(%p, %p, %p)\n", This, constant, desc, count);

    if (!constant)
        return D3DERR_INVALIDCALL;

    /* Applications can pass the name of the constant in place of the handle */
    if (!((UINT_PTR)constant >> 16))
        constant_info = &This->constants[(UINT_PTR)constant - 1];
    else
    {
        D3DXHANDLE c = ID3DXConstantTable_GetConstantByName(iface, NULL, constant);
        if (!c)
            return D3DERR_INVALIDCALL;

        constant_info = &This->constants[(UINT_PTR)c - 1];
    }

    if (desc)
        *desc = constant_info->desc;
    if (count)
        *count = 1;

    return D3D_OK;
}

static UINT WINAPI ID3DXConstantTableImpl_GetSamplerIndex(LPD3DXCONSTANTTABLE iface, D3DXHANDLE constant)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, constant);

    return (UINT)-1;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstant(ID3DXConstantTable* iface, D3DXHANDLE constant, UINT index)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    TRACE("(%p)->(%p, %d)\n", This, constant, index);

    if (constant)
    {
        FIXME("Only top level constants supported\n");
        return NULL;
    }

    if (index >= This->desc.Constants)
        return NULL;

    return (D3DXHANDLE)(DWORD_PTR)(index + 1);
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantByName(ID3DXConstantTable* iface, D3DXHANDLE constant, LPCSTR name)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;
    UINT i;

    TRACE("(%p)->(%p, %s)\n", This, constant, name);

    if (!name)
        return NULL;

    if (constant)
    {
        FIXME("Only top level constants supported\n");
        return NULL;
    }

    for (i = 0; i < This->desc.Constants; i++)
        if (!strcmp(This->constants[i].desc.Name, name))
            return (D3DXHANDLE)(DWORD_PTR)(i + 1);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantElement(ID3DXConstantTable* iface, D3DXHANDLE constant, UINT index)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %d): stub\n", This, constant, index);

    return NULL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetDefaults(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p): stub\n", This, device);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetValue(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                      D3DXHANDLE constant, LPCVOID data, UINT bytes)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, data, bytes);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBool(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                     D3DXHANDLE constant, BOOL b)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %d): stub\n", This, device, constant, b);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetBoolArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                          D3DXHANDLE constant, CONST BOOL* b, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, b, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetInt(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device, D3DXHANDLE constant, INT n)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %d): stub\n", This, device, constant, n);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetIntArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                         D3DXHANDLE constant, CONST INT* n, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, n, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloat(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                      D3DXHANDLE constant, FLOAT f)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %f): stub\n", This, device, constant, f);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetFloatArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                           D3DXHANDLE constant, CONST FLOAT* f, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, f, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVector(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                       D3DXHANDLE constant, CONST D3DXVECTOR4* vector)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p): stub\n", This, device, constant, vector);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetVectorArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                            D3DXHANDLE constant, CONST D3DXVECTOR4* vector, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, vector, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrix(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                       D3DXHANDLE constant, CONST D3DXMATRIX* matrix)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p): stub\n", This, device, constant, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                            D3DXHANDLE constant, CONST D3DXMATRIX* matrix, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixPointerArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                                   D3DXHANDLE constant, CONST D3DXMATRIX** matrix, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTranspose(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                                D3DXHANDLE constant, CONST D3DXMATRIX* matrix)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p): stub\n", This, device, constant, matrix);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposeArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                                     D3DXHANDLE constant, CONST D3DXMATRIX* matrix, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, matrix, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXConstantTableImpl_SetMatrixTransposePointerArray(ID3DXConstantTable* iface, LPDIRECT3DDEVICE9 device,
                                                                            D3DXHANDLE constant, CONST D3DXMATRIX** matrix, UINT count)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %p, %p, %d): stub\n", This, device, constant, matrix, count);

    return E_NOTIMPL;
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

static HRESULT parse_ctab_constant_type(const D3DXSHADER_TYPEINFO *type, ctab_constant *constant)
{
    constant->desc.Class = type->Class;
    constant->desc.Type = type->Type;
    constant->desc.Rows = type->Rows;
    constant->desc.Columns = type->Columns;
    constant->desc.StructMembers = type->StructMembers;

    TRACE("class = %d, type = %d, rows = %d, columns = %d, struct_members = %d\n",
          constant->desc.Class, constant->desc.Type,
          constant->desc.Rows, constant->desc.Columns, constant->desc.StructMembers);

    if ((constant->desc.Class == D3DXPC_STRUCT) && constant->desc.StructMembers)
    {
        FIXME("Struct not supported yet\n");
        return E_NOTIMPL;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXGetShaderConstantTableEx(CONST DWORD* byte_code,
                                            DWORD flags,
                                            LPD3DXCONSTANTTABLE* constant_table)
{
    ID3DXConstantTableImpl* object = NULL;
    HRESULT hr;
    LPCVOID data;
    UINT size;
    const D3DXSHADER_CONSTANTTABLE* ctab_header;
    D3DXSHADER_CONSTANTINFO* constant_info;
    DWORD i;

    TRACE("(%p, %x, %p)\n", byte_code, flags, constant_table);

    if (!byte_code || !constant_table)
        return D3DERR_INVALIDCALL;

    hr = D3DXFindShaderComment(byte_code, MAKEFOURCC('C','T','A','B'), &data, &size);
    if (hr != D3D_OK)
        return D3DXERR_INVALIDDATA;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ID3DXConstantTableImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl = &ID3DXConstantTable_Vtbl;
    object->ref = 1;

    if (size < sizeof(D3DXSHADER_CONSTANTTABLE))
    {
        hr = D3DXERR_INVALIDDATA;
        goto error;
    }

    object->ctab = HeapAlloc(GetProcessHeap(), 0, size);
    if (!object->ctab)
    {
        ERR("Out of memory\n");
        hr = E_OUTOFMEMORY;
        goto error;
    }
    object->size = size;
    memcpy(object->ctab, data, object->size);

    ctab_header = (const D3DXSHADER_CONSTANTTABLE*)data;
    if (ctab_header->Size != sizeof(D3DXSHADER_CONSTANTTABLE))
    {
        hr = D3DXERR_INVALIDDATA;
        goto error;
    }
    object->desc.Creator = ctab_header->Creator ? object->ctab + ctab_header->Creator : NULL;
    object->desc.Version = ctab_header->Version;
    object->desc.Constants = ctab_header->Constants;
    if (object->desc.Creator)
        TRACE("Creator = %s\n", object->desc.Creator);
    TRACE("Version = %x\n", object->desc.Version);
    TRACE("Constants = %d\n", ctab_header->Constants);
    if (ctab_header->Target)
        TRACE("Target = %s\n", object->ctab + ctab_header->Target);

    if (object->desc.Constants > 65535)
    {
        FIXME("Too many constants (%u)\n", object->desc.Constants);
        hr = E_NOTIMPL;
        goto error;
    }

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
        TRACE("name = %s\n", object->ctab + constant_info[i].Name);
        object->constants[i].desc.Name = object->ctab + constant_info[i].Name;
        object->constants[i].desc.RegisterSet = constant_info[i].RegisterSet;
        object->constants[i].desc.RegisterIndex = constant_info[i].RegisterIndex;
        object->constants[i].desc.RegisterCount = 0;
        hr = parse_ctab_constant_type((LPD3DXSHADER_TYPEINFO)(object->ctab + constant_info[i].TypeInfo),
             &object->constants[i]);
        if (hr != D3D_OK)
            goto error;
    }

    *constant_table = (LPD3DXCONSTANTTABLE)object;

    return D3D_OK;

error:

    HeapFree(GetProcessHeap(), 0, object->constants);
    HeapFree(GetProcessHeap(), 0, object->ctab);
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}

HRESULT WINAPI D3DXGetShaderConstantTable(CONST DWORD* byte_code,
                                          LPD3DXCONSTANTTABLE* constant_table)
{
    TRACE("(%p, %p): Forwarded to D3DXGetShaderConstantTableEx\n", byte_code, constant_table);

    return D3DXGetShaderConstantTableEx(byte_code, 0, constant_table);
}
