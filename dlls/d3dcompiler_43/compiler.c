/*
 * Copyright 2009 Matteo Bruni
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
 */

#define COBJMACROS
#include <stdarg.h>
#include <time.h>
#include "wine/debug.h"

#include "d3dcompiler_private.h"
#include "wpp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dcompiler);

#define D3DXERR_INVALIDDATA                      0x88760b59

#define BUFFER_INITIAL_CAPACITY 256

struct mem_file_desc
{
    const char *buffer;
    unsigned int size;
    unsigned int pos;
};

static struct mem_file_desc current_shader;
static ID3DInclude *current_include;
static const char *initial_filename;

#define INCLUDES_INITIAL_CAPACITY 4

struct loaded_include
{
    const char *name;
    const char *data;
};

static struct loaded_include *includes;
static int includes_capacity, includes_size;
static const char *parent_include;

static char *wpp_output;
static int wpp_output_capacity, wpp_output_size;

static char *wpp_messages;
static int wpp_messages_capacity, wpp_messages_size;

struct define
{
    struct define *next;
    char          *name;
    char          *value;
};

static struct define *cmdline_defines;

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
            return;

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

static void WINAPIV PRINTF_ATTR(1,2) wpp_write_message_var(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    wpp_write_message(fmt, args);
    va_end(args);
}

int WINAPIV ppy_error(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    wpp_write_message_var("%s:%d:%d: %s: ",
                          pp_status.input ? pp_status.input : "'main file'",
                          pp_status.line_number, pp_status.char_number, "Error");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
    va_end(ap);
    pp_status.state = 1;
    return 1;
}

int WINAPIV ppy_warning(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    wpp_write_message_var("%s:%d:%d: %s: ",
                          pp_status.input ? pp_status.input : "'main file'",
                          pp_status.line_number, pp_status.char_number, "Warning");
    wpp_write_message(msg, ap);
    wpp_write_message_var("\n");
    va_end(ap);
    return 0;
}

char *wpp_lookup(const char *filename, int type, const char *parent_name)
{
    /* We don't check for file existence here. We will potentially fail on
     * the following wpp_open_mem(). */
    char *path;
    int i;

    TRACE("Looking for include %s, parent %s.\n", debugstr_a(filename), debugstr_a(parent_name));

    parent_include = NULL;
    if (strcmp(parent_name, initial_filename))
    {
        for(i = 0; i < includes_size; i++)
        {
            if(!strcmp(parent_name, includes[i].name))
            {
                parent_include = includes[i].data;
                break;
            }
        }
        if(parent_include == NULL)
        {
            ERR("Parent include %s missing.\n", debugstr_a(parent_name));
            return NULL;
        }
    }

    path = malloc(strlen(filename) + 1);
    if(path)
        memcpy(path, filename, strlen(filename) + 1);
    return path;
}

void *wpp_open(const char *filename, int type)
{
    struct mem_file_desc *desc;
    HRESULT hr;

    TRACE("Opening include %s.\n", debugstr_a(filename));

    if(!strcmp(filename, initial_filename))
    {
        current_shader.pos = 0;
        return &current_shader;
    }

    if(current_include == NULL) return NULL;
    desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*desc));
    if(!desc)
        return NULL;

    if (FAILED(hr = ID3DInclude_Open(current_include, type ? D3D_INCLUDE_LOCAL : D3D_INCLUDE_SYSTEM,
            filename, parent_include, (const void **)&desc->buffer, &desc->size)))
    {
        HeapFree(GetProcessHeap(), 0, desc);
        return NULL;
    }

    if(includes_capacity == includes_size)
    {
        if(includes_capacity == 0)
        {
            includes = HeapAlloc(GetProcessHeap(), 0, INCLUDES_INITIAL_CAPACITY * sizeof(*includes));
            if(includes == NULL)
            {
                ERR("Error allocating memory for the loaded includes structure\n");
                goto error;
            }
            includes_capacity = INCLUDES_INITIAL_CAPACITY * sizeof(*includes);
        }
        else
        {
            int newcapacity = includes_capacity * 2;
            struct loaded_include *newincludes =
                HeapReAlloc(GetProcessHeap(), 0, includes, newcapacity);
            if(newincludes == NULL)
            {
                ERR("Error reallocating memory for the loaded includes structure\n");
                goto error;
            }
            includes = newincludes;
            includes_capacity = newcapacity;
        }
    }
    includes[includes_size].name = filename;
    includes[includes_size++].data = desc->buffer;

    desc->pos = 0;
    return desc;

error:
    ID3DInclude_Close(current_include, desc->buffer);
    HeapFree(GetProcessHeap(), 0, desc);
    return NULL;
}

void wpp_close(void *file)
{
    struct mem_file_desc *desc = file;

    if(desc != &current_shader)
    {
        if(current_include)
            ID3DInclude_Close(current_include, desc->buffer);
        else
            ERR("current_include == NULL, desc == %p, buffer = %s\n",
                desc, desc->buffer);

        HeapFree(GetProcessHeap(), 0, desc);
    }
}

int wpp_read(void *file, char *buffer, unsigned int len)
{
    struct mem_file_desc *desc = file;

    len = min(len, desc->size - desc->pos);
    memcpy(buffer, desc->buffer + desc->pos, len);
    desc->pos += len;
    return len;
}

void wpp_write(const char *buffer, unsigned int len)
{
    char *new_wpp_output;

    if(wpp_output_capacity == 0)
    {
        wpp_output = HeapAlloc(GetProcessHeap(), 0, BUFFER_INITIAL_CAPACITY);
        if(!wpp_output)
            return;

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
    wpp_output_size++;
    return 1;
}

static void add_cmdline_defines(void)
{
    struct define *def;

    for (def = cmdline_defines; def; def = def->next)
    {
        if (def->value) pp_add_define( def->name, def->value );
    }
}

static void del_cmdline_defines(void)
{
    struct define *def;

    for (def = cmdline_defines; def; def = def->next)
    {
        if (def->value) pp_del_define( def->name );
    }
}

static void add_special_defines(void)
{
    time_t now = time(NULL);
    pp_entry_t *ppp;
    char buf[32];

    strftime(buf, sizeof(buf), "\"%b %d %Y\"", localtime(&now));
    pp_add_define( "__DATE__", buf );

    strftime(buf, sizeof(buf), "\"%H:%M:%S\"", localtime(&now));
    pp_add_define( "__TIME__", buf );

    ppp = pp_add_define( "__FILE__", "" );
    if(ppp)
        ppp->type = def_special;

    ppp = pp_add_define( "__LINE__", "" );
    if(ppp)
        ppp->type = def_special;
}

static void del_special_defines(void)
{
    pp_del_define( "__DATE__" );
    pp_del_define( "__TIME__" );
    pp_del_define( "__FILE__" );
    pp_del_define( "__LINE__" );
}


/* add a define to the preprocessor list */
int wpp_add_define( const char *name, const char *value )
{
    struct define *def;

    if (!value) value = "";

    for (def = cmdline_defines; def; def = def->next)
    {
        if (!strcmp( def->name, name ))
        {
            char *new_value = pp_xstrdup(value);
            if(!new_value)
                return 1;
            free( def->value );
            def->value = new_value;

            return 0;
        }
    }

    def = pp_xmalloc( sizeof(*def) );
    if(!def)
        return 1;
    def->next  = cmdline_defines;
    def->name  = pp_xstrdup(name);
    if(!def->name)
    {
        free(def);
        return 1;
    }
    def->value = pp_xstrdup(value);
    if(!def->value)
    {
        free(def->name);
        free(def);
        return 1;
    }
    cmdline_defines = def;
    return 0;
}


/* undefine a previously added definition */
void wpp_del_define( const char *name )
{
    struct define *def;

    for (def = cmdline_defines; def; def = def->next)
    {
        if (!strcmp( def->name, name ))
        {
            free( def->value );
            def->value = NULL;
            return;
        }
    }
}


/* the main preprocessor parsing loop */
int wpp_parse( const char *input, FILE *output )
{
    int ret;

    pp_status.input = NULL;
    pp_status.line_number = 1;
    pp_status.char_number = 1;
    pp_status.state = 0;

    ret = pp_push_define_state();
    if(ret)
        return ret;
    add_cmdline_defines();
    add_special_defines();

    if (!input) pp_status.file = stdin;
    else if (!(pp_status.file = wpp_open(input, 1)))
    {
        ppy_error("Could not open %s\n", input);
        del_special_defines();
        del_cmdline_defines();
        pp_pop_define_state();
        return 2;
    }

    pp_status.input = input ? pp_xstrdup(input) : NULL;

    ppy_out = output;
    pp_writestring("# 1 \"%s\" 1\n", input ? input : "");

    ret = ppy_parse();
    /* If there were errors during processing, return an error code */
    if (!ret && pp_status.state) ret = pp_status.state;

    if (input)
    {
	wpp_close(pp_status.file);
	free(pp_status.input);
    }
    /* Clean if_stack, it could remain dirty on errors */
    while (pp_get_if_depth()) pp_pop_if();
    ppy_lex_destroy();
    del_special_defines();
    del_cmdline_defines();
    pp_pop_define_state();
    return ret;
}

static HRESULT WINAPI d3dcompiler_include_from_file_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    char *fullpath, *buffer = NULL, current_dir[MAX_PATH + 1];
    const char *initial_dir;
    SIZE_T size;
    HANDLE file;
    ULONG read;
    DWORD len;

    if ((initial_dir = strrchr(initial_filename, '\\')))
    {
        len = initial_dir - initial_filename + 1;
        initial_dir = initial_filename;
    }
    else
    {
        len = GetCurrentDirectoryA(MAX_PATH, current_dir);
        current_dir[len] = '\\';
        len++;
        initial_dir = current_dir;
    }
    fullpath = heap_alloc(len + strlen(filename) + 1);
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
    buffer = heap_alloc(size);
    if (!buffer)
        goto error;
    if (!ReadFile(file, buffer, size, &read, NULL) || read != size)
        goto error;

    *bytes = size;
    *data = buffer;

    heap_free(fullpath);
    CloseHandle(file);
    return S_OK;

error:
    heap_free(fullpath);
    heap_free(buffer);
    CloseHandle(file);
    WARN("Returning E_FAIL.\n");
    return E_FAIL;
}

static HRESULT WINAPI d3dcompiler_include_from_file_close(ID3DInclude *iface, const void *data)
{
    heap_free((void *)data);
    return S_OK;
}

const struct ID3DIncludeVtbl d3dcompiler_include_from_file_vtbl =
{
    d3dcompiler_include_from_file_open,
    d3dcompiler_include_from_file_close
};

struct d3dcompiler_include_from_file
{
    ID3DInclude ID3DInclude_iface;
};

static HRESULT preprocess_shader(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, ID3DBlob **error_messages)
{
    struct d3dcompiler_include_from_file include_from_file;
    int ret;
    HRESULT hr = S_OK;
    const D3D_SHADER_MACRO *def = defines;

    if (include == D3D_COMPILE_STANDARD_FILE_INCLUDE)
    {
        include_from_file.ID3DInclude_iface.lpVtbl = &d3dcompiler_include_from_file_vtbl;
        include = &include_from_file.ID3DInclude_iface;
    }

    if (def != NULL)
    {
        while (def->Name != NULL)
        {
            wpp_add_define(def->Name, def->Definition);
            def++;
        }
    }
    current_include = include;
    includes_size = 0;

    wpp_output_size = wpp_output_capacity = 0;
    wpp_output = NULL;

    wpp_messages_size = wpp_messages_capacity = 0;
    wpp_messages = NULL;
    current_shader.buffer = data;
    current_shader.size = data_size;
    initial_filename = filename ? filename : "";

    ret = wpp_parse(initial_filename, NULL);
    if (!wpp_close_output())
        ret = 1;
    if (ret)
    {
        TRACE("Error during shader preprocessing\n");
        if (wpp_messages)
        {
            int size;
            ID3DBlob *buffer;

            TRACE("Preprocessor messages:\n%s\n", debugstr_a(wpp_messages));

            if (error_messages)
            {
                size = strlen(wpp_messages) + 1;
                hr = D3DCreateBlob(size, &buffer);
                if (FAILED(hr))
                    goto cleanup;
                CopyMemory(ID3D10Blob_GetBufferPointer(buffer), wpp_messages, size);
                *error_messages = buffer;
            }
        }
        if (data)
            TRACE("Shader source:\n%s\n", debugstr_an(data, data_size));
        hr = E_FAIL;
    }

cleanup:
    /* Remove the previously added defines */
    if (defines != NULL)
    {
        while (defines->Name != NULL)
        {
            wpp_del_define(defines->Name);
            defines++;
        }
    }
    HeapFree(GetProcessHeap(), 0, wpp_messages);
    return hr;
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
                HeapFree(GetProcessHeap(), 0, messages);
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
        HeapFree(GetProcessHeap(), 0, messages);
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
            HeapFree(GetProcessHeap(), 0, res);
            return hr;
        }
        CopyMemory(ID3D10Blob_GetBufferPointer(buffer), res, size);
        *shader_blob = buffer;
    }

    HeapFree(GetProcessHeap(), 0, res);

    return S_OK;
}

HRESULT WINAPI D3DAssemble(const void *data, SIZE_T datasize, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, UINT flags,
        ID3DBlob **shader, ID3DBlob **error_messages)
{
    HRESULT hr;

    TRACE("data %p, datasize %Iu, filename %s, defines %p, include %p, sflags %#x, "
            "shader %p, error_messages %p.\n",
            data, datasize, debugstr_a(filename), defines, include, flags, shader, error_messages);

    EnterCriticalSection(&wpp_mutex);

    /* TODO: flags */
    if (flags) FIXME("flags %x\n", flags);

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    hr = preprocess_shader(data, datasize, filename, defines, include, error_messages);
    if (SUCCEEDED(hr))
        hr = assemble_shader(wpp_output, shader, error_messages);

    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

HRESULT WINAPI D3DCompile2(const void *data, SIZE_T data_size, const char *filename,
        const D3D_SHADER_MACRO *defines, ID3DInclude *include, const char *entrypoint,
        const char *target, UINT sflags, UINT eflags, UINT secondary_flags,
        const void *secondary_data, SIZE_T secondary_data_size, ID3DBlob **shader,
        ID3DBlob **error_messages)
{
    HRESULT hr;

    TRACE("data %p, data_size %Iu, filename %s, defines %p, include %p, entrypoint %s, "
            "target %s, sflags %#x, eflags %#x, secondary_flags %#x, secondary_data %p, "
            "secondary_data_size %Iu, shader %p, error_messages %p.\n",
            data, data_size, debugstr_a(filename), defines, include, debugstr_a(entrypoint),
            debugstr_a(target), sflags, eflags, secondary_flags, secondary_data,
            secondary_data_size, shader, error_messages);

    if (secondary_data)
        FIXME("secondary data not implemented yet\n");

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    EnterCriticalSection(&wpp_mutex);

    hr = preprocess_shader(data, data_size, filename, defines, include, error_messages);
    if (SUCCEEDED(hr))
    {
        FIXME("HLSL shader compilation is not yet implemented.\n");
        hr = E_NOTIMPL;
    }

    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
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
    HRESULT hr;
    ID3DBlob *buffer;

    TRACE("data %p, size %Iu, filename %s, defines %p, include %p, shader %p, error_messages %p.\n",
          data, size, debugstr_a(filename), defines, include, shader, error_messages);

    if (!data)
        return E_INVALIDARG;

    EnterCriticalSection(&wpp_mutex);

    if (shader) *shader = NULL;
    if (error_messages) *error_messages = NULL;

    hr = preprocess_shader(data, size, filename, defines, include, error_messages);

    if (SUCCEEDED(hr))
    {
        if (shader)
        {
            hr = D3DCreateBlob(wpp_output_size, &buffer);
            if (FAILED(hr))
                goto cleanup;
            CopyMemory(ID3D10Blob_GetBufferPointer(buffer), wpp_output, wpp_output_size);
            *shader = buffer;
        }
        else
            hr = E_INVALIDARG;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, wpp_output);
    LeaveCriticalSection(&wpp_mutex);
    return hr;
}

HRESULT WINAPI D3DDisassemble(const void *data, SIZE_T size, UINT flags, const char *comments, ID3DBlob **disassembly)
{
    FIXME("data %p, size %Iu, flags %#x, comments %p, disassembly %p stub!\n",
            data, size, flags, comments, disassembly);
    return E_NOTIMPL;
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

    if (!(source = heap_alloc(source_size)))
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
    heap_free(source);
    CloseHandle(file);
    return hr;
}

HRESULT WINAPI D3DLoadModule(const void *data, SIZE_T size, ID3D11Module **module)
{
    FIXME("data %p, size %Iu, module %p stub!\n", data, size, module);
    return E_NOTIMPL;
}
