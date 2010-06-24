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
#include "wine/wpp.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

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

#define BUFFER_INITIAL_CAPACITY 256

struct mem_file_desc
{
    const char *buffer;
    unsigned int size;
    unsigned int pos;
};

struct mem_file_desc current_shader;
LPD3DXINCLUDE current_include;
char *wpp_output;
int wpp_output_capacity, wpp_output_size;

char *wpp_messages;
int wpp_messages_capacity, wpp_messages_size;

/* Mutex used to guarantee a single invocation
   of the D3DXAssembleShader function (or its variants) at a time.
   This is needed as wpp isn't thread-safe */
static CRITICAL_SECTION wpp_mutex;
static CRITICAL_SECTION_DEBUG wpp_mutex_debug =
{
    0, 0, &wpp_mutex,
    { &wpp_mutex_debug.ProcessLocksList,
      &wpp_mutex_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wpp_mutex") }
};
static CRITICAL_SECTION wpp_mutex = { &wpp_mutex_debug, -1, 0, 0, 0, 0 };

/* Preprocessor error reporting functions */
static void wpp_write_message(const char *fmt, va_list args)
{
    char* newbuffer;
    int rc, newsize;

    if(wpp_messages_capacity == 0)
    {
        wpp_messages = HeapAlloc(GetProcessHeap(), 0, MESSAGEBUFFER_INITIAL_SIZE);
        if(wpp_messages == NULL)
        {
            ERR("Error allocating memory for parser messages\n");
            return;
        }
        wpp_messages_capacity = MESSAGEBUFFER_INITIAL_SIZE;
    }

    while(1)
    {
        rc = vsnprintf(wpp_messages + wpp_messages_size,
                       wpp_messages_capacity - wpp_messages_size, fmt, args);

        if (rc < 0 ||                                           /* C89 */
            rc >= wpp_messages_capacity - wpp_messages_size) {  /* C99 */
            /* Resize the buffer */
            newsize = wpp_messages_capacity * 2;
            newbuffer = HeapReAlloc(GetProcessHeap(), 0, wpp_messages, newsize);
            if(newbuffer == NULL)
            {
                ERR("Error reallocating memory for parser messages\n");
                return;
            }
            wpp_messages = newbuffer;
            wpp_messages_capacity = newsize;
        }
        else
        {
            wpp_messages_size += rc;
            return;
        }
    }
}

static void PRINTF_ATTR(1,2) wpp_write_message_var(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    wpp_write_message(fmt, args);
    va_end(args);
}

static void wpp_error(const char *file, int line, int col, const char *near,
                      const char *msg, va_list ap)
{
    wpp_write_message_var("%s:%d:%d: %s: ", file ? file : "'main file'",
                          line, col, "Error");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
}

static void wpp_warning(const char *file, int line, int col, const char *near,
                        const char *msg, va_list ap)
{
    wpp_write_message_var("%s:%d:%d: %s: ", file ? file : "'main file'",
                          line, col, "Warning");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
}

static char *wpp_lookup_mem(const char *filename, const char *parent_name,
                            char **include_path, int include_path_count)
{
    /* Here we return always ok. We will maybe fail on the next wpp_open_mem */
    char *path;

    path = malloc(strlen(filename) + 1);
    if(!path) return NULL;
    memcpy(path, filename, strlen(filename) + 1);
    return path;
}

static void *wpp_open_mem(const char *filename, int type)
{
    struct mem_file_desc *desc;
    HRESULT hr;

    if(filename[0] == '\0') /* "" means to load the initial shader */
    {
        current_shader.pos = 0;
        return &current_shader;
    }

    if(current_include == NULL) return NULL;
    desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*desc));
    if(!desc)
    {
        ERR("Error allocating memory\n");
        return NULL;
    }
    hr = ID3DXInclude_Open(current_include,
                           type ? D3DXINC_SYSTEM : D3DXINC_LOCAL,
                           filename, NULL, (LPCVOID *)&desc->buffer,
                           &desc->size);
    if(FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, desc);
        return NULL;
    }
    desc->pos = 0;
    return desc;
}

static void wpp_close_mem(void *file)
{
    struct mem_file_desc *desc = file;

    if(desc != &current_shader)
    {
        if(current_include)
            ID3DXInclude_Close(current_include, desc->buffer);
        else
            ERR("current_include == NULL, desc == %p, buffer = %s\n",
                desc, desc->buffer);

        HeapFree(GetProcessHeap(), 0, desc);
    }
}

static int wpp_read_mem(void *file, char *buffer, unsigned int len)
{
    struct mem_file_desc *desc = file;

    len = min(len, desc->size - desc->pos);
    memcpy(buffer, desc->buffer + desc->pos, len);
    desc->pos += len;
    return len;
}

static void wpp_write_mem(const char *buffer, unsigned int len)
{
    char *new_wpp_output;

    if(wpp_output_capacity == 0)
    {
        wpp_output = HeapAlloc(GetProcessHeap(), 0, BUFFER_INITIAL_CAPACITY);
        if(!wpp_output)
        {
            ERR("Error allocating memory\n");
            return;
        }
        wpp_output_capacity = BUFFER_INITIAL_CAPACITY;
    }
    if(len > wpp_output_capacity - wpp_output_size)
    {
        while(len > wpp_output_capacity - wpp_output_size)
        {
            wpp_output_capacity *= 2;
        }
        new_wpp_output = HeapReAlloc(GetProcessHeap(), 0, wpp_output,
                                     wpp_output_capacity);
        if(!new_wpp_output)
        {
            ERR("Error allocating memory\n");
            return;
        }
        wpp_output = new_wpp_output;
    }
    memcpy(wpp_output + wpp_output_size, buffer, len);
    wpp_output_size += len;
}

static int wpp_close_output(void)
{
    char *new_wpp_output = HeapReAlloc(GetProcessHeap(), 0, wpp_output,
                                       wpp_output_size + 1);
    if(!new_wpp_output) return 0;
    wpp_output = new_wpp_output;
    wpp_output[wpp_output_size]='\0';
    return 1;
}

static HRESULT assemble_shader(const char *preprocShader, const char *preprocMessages,
                        LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs)
{
    struct bwriter_shader *shader;
    char *messages = NULL;
    HRESULT hr;
    DWORD *res;
    LPD3DXBUFFER buffer;
    int size;
    char *pos;

    shader = SlAssembleShader(preprocShader, &messages);

    if(messages || preprocMessages)
    {
        if(preprocMessages)
        {
            TRACE("Preprocessor messages:\n");
            TRACE("%s", preprocMessages);
        }
        if(messages)
        {
            TRACE("Assembler messages:\n");
            TRACE("%s", messages);
        }

        TRACE("Shader source:\n");
        TRACE("%s\n", debugstr_a(preprocShader));

        if(ppErrorMsgs)
        {
            size = (messages ? strlen(messages) : 0) +
                (preprocMessages ? strlen(preprocMessages) : 0) + 1;
            hr = D3DXCreateBuffer(size, &buffer);
            if(FAILED(hr))
            {
                HeapFree(GetProcessHeap(), 0, messages);
                if(shader) SlDeleteShader(shader);
                return hr;
            }
            pos = ID3DXBuffer_GetBufferPointer(buffer);
            if(preprocMessages)
            {
                CopyMemory(pos, preprocMessages, strlen(preprocMessages) + 1);
                pos += strlen(preprocMessages);
            }
            if(messages)
                CopyMemory(pos, messages, strlen(messages) + 1);

            *ppErrorMsgs = buffer;
        }

        HeapFree(GetProcessHeap(), 0, messages);
    }

    if(shader == NULL)
    {
        ERR("Asm reading failed\n");
        return D3DXERR_INVALIDDATA;
    }

    hr = SlWriteBytecode(shader, 9, &res);
    SlDeleteShader(shader);
    if(FAILED(hr))
    {
        ERR("SlWriteBytecode failed with 0x%08x\n", hr);
        return D3DXERR_INVALIDDATA;
    }

    if(ppShader)
    {
        size = HeapSize(GetProcessHeap(), 0, res);
        hr = D3DXCreateBuffer(size, &buffer);
        if(FAILED(hr))
        {
            HeapFree(GetProcessHeap(), 0, res);
            return hr;
        }
        CopyMemory(ID3DXBuffer_GetBufferPointer(buffer), res, size);
        *ppShader = buffer;
    }

    HeapFree(GetProcessHeap(), 0, res);

    return D3D_OK;
}

HRESULT WINAPI D3DXAssembleShader(LPCSTR data,
                                  UINT data_len,
                                  CONST D3DXMACRO* defines,
                                  LPD3DXINCLUDE include,
                                  DWORD flags,
                                  LPD3DXBUFFER* shader,
                                  LPD3DXBUFFER* error_messages)
{
    int ret;
    HRESULT hr;
    CONST D3DXMACRO* def = defines;

    static const struct wpp_callbacks wpp_callbacks = {
        wpp_lookup_mem,
        wpp_open_mem,
        wpp_close_mem,
        wpp_read_mem,
        wpp_write_mem,
        wpp_error,
        wpp_warning,
    };

    EnterCriticalSection(&wpp_mutex);

    /* TODO: flags */
    if(flags) FIXME("flags: %x\n", flags);

    if(def != NULL)
    {
        while(def->Name != NULL)
        {
            wpp_add_define(def->Name, def->Definition);
            def++;
        }
    }
    current_include = include;

    if(shader) *shader = NULL;
    if(error_messages) *error_messages = NULL;
    wpp_output_size = wpp_output_capacity = 0;
    wpp_output = NULL;

    /* Preprocess shader */
    wpp_set_callbacks(&wpp_callbacks);
    wpp_messages_size = wpp_messages_capacity = 0;
    wpp_messages = NULL;
    current_shader.buffer = data;
    current_shader.size = data_len;

    ret = wpp_parse("", NULL);
    if(!wpp_close_output())
        ret = 1;
    if(ret)
    {
        TRACE("Error during shader preprocessing\n");
        if(wpp_messages)
        {
            int size;
            LPD3DXBUFFER buffer;

            TRACE("Preprocessor messages:\n");
            TRACE("%s", wpp_messages);

            if(error_messages)
            {
                size = strlen(wpp_messages) + 1;
                hr = D3DXCreateBuffer(size, &buffer);
                if(FAILED(hr)) goto cleanup;
                CopyMemory(ID3DXBuffer_GetBufferPointer(buffer), wpp_messages, size);
                *error_messages = buffer;
            }
        }
        if(data)
        {
            TRACE("Shader source:\n");
            TRACE("%s\n", debugstr_an(data, data_len));
        }
        hr = D3DXERR_INVALIDDATA;
        goto cleanup;
    }

    hr = assemble_shader(wpp_output, wpp_messages, shader, error_messages);

cleanup:
    /* Remove the previously added defines */
    if(defines != NULL)
    {
        while(defines->Name != NULL)
        {
            wpp_del_define(defines->Name);
            defines++;
        }
    }
    HeapFree(GetProcessHeap(), 0, wpp_messages);
    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

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
    FIXME("(%s, %p, %p, %x, %p, %p): stub\n", debugstr_w(filename), defines, include, flags, shader, error_messages);
    return D3DERR_INVALIDCALL;
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
    FIXME("(%p, %d, %p, %p, %s, %s, %x, %p, %p, %p): stub\n",
          pSrcData, srcDataLen, pDefines, pInclude, debugstr_a(pFunctionName),
          debugstr_a(pProfile), Flags, ppShader, ppErrorMsgs, ppConstantTable);

    TRACE("Shader source:\n");
    TRACE("%s\n", debugstr_an(pSrcData, srcDataLen));

    return D3DERR_INVALIDCALL;
}

static const struct ID3DXConstantTableVtbl ID3DXConstantTable_Vtbl;

typedef struct ID3DXConstantTableImpl {
    const ID3DXConstantTableVtbl *lpVtbl;
    LONG ref;
    LPVOID ctab;
    DWORD size;
    D3DXCONSTANTTABLE_DESC desc;
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

    FIXME("(%p)->(%p, %p, %p): stub\n", This, constant, desc, count);

    return E_NOTIMPL;
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

    FIXME("(%p)->(%p, %d): stub\n", This, constant, index);

    return NULL;
}

static D3DXHANDLE WINAPI ID3DXConstantTableImpl_GetConstantByName(ID3DXConstantTable* iface, D3DXHANDLE constant, LPCSTR name)
{
    ID3DXConstantTableImpl *This = (ID3DXConstantTableImpl *)iface;

    FIXME("(%p)->(%p, %s): stub\n", This, constant, name);

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

HRESULT WINAPI D3DXGetShaderConstantTableEx(CONST DWORD* byte_code,
                                            DWORD flags,
                                            LPD3DXCONSTANTTABLE* constant_table)
{
    ID3DXConstantTableImpl* object = NULL;
    HRESULT hr;
    LPCVOID data;
    UINT size;
    const D3DXSHADER_CONSTANTTABLE* ctab_header;

    FIXME("(%p, %x, %p): semi-stub\n", byte_code, flags, constant_table);

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
        goto error;

    object->ctab = HeapAlloc(GetProcessHeap(), 0, size);
    if (!object->ctab)
    {
        HeapFree(GetProcessHeap(), 0, object);
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    object->size = size;
    memcpy(object->ctab, data, object->size);

    ctab_header = (const D3DXSHADER_CONSTANTTABLE*)data;
    if (ctab_header->Size != sizeof(D3DXSHADER_CONSTANTTABLE))
        goto error;
    object->desc.Creator = ctab_header->Creator ? (LPCSTR)object->ctab + ctab_header->Creator : NULL;
    object->desc.Version = ctab_header->Version;
    object->desc.Constants = ctab_header->Constants;

    *constant_table = (LPD3DXCONSTANTTABLE)object;

    return D3D_OK;

error:

    HeapFree(GetProcessHeap(), 0, object->ctab);
    HeapFree(GetProcessHeap(), 0, object);

    return D3DXERR_INVALIDDATA;
}

HRESULT WINAPI D3DXGetShaderConstantTable(CONST DWORD* byte_code,
                                          LPD3DXCONSTANTTABLE* constant_table)
{
    TRACE("(%p, %p): Forwarded to D3DXGetShaderConstantTableEx\n", byte_code, constant_table);

    return D3DXGetShaderConstantTableEx(byte_code, 0, constant_table);
}
