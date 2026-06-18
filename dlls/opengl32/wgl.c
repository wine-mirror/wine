/* Window-specific OpenGL functions implementation.
 *
 * Copyright (c) 1999 Lionel Ulmer
 * Copyright (c) 2005 Raphael Junqueira
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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntuser.h"
#include "ntgdi.h"
#include "malloc.h"

#include "unixlib.h"
#include "private.h"

#include "wine/glu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);
WINE_DECLARE_DEBUG_CHANNEL(fps);

static const MAT2 identity = { {0,1},{0,0},{0,0},{0,1} };

#define WINE_GL_RESERVED_FORMATS_HDC      2
#define WINE_GL_RESERVED_FORMATS_PTR      3
#define WINE_GL_RESERVED_FORMATS_NUM      4
#define WINE_GL_RESERVED_FORMATS_ONSCREEN 5

static CRITICAL_SECTION wgl_cs;
static CRITICAL_SECTION_DEBUG wgl_cs_debug = {
    0, 0, &wgl_cs,
    { &wgl_cs_debug.ProcessLocksList,
      &wgl_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": wgl_cs") }
};
static CRITICAL_SECTION wgl_cs = { &wgl_cs_debug, -1, 0, 0, 0, 0 };
static char *wgl_extensions;

#define USE_GL_EXT(x) #x,
static const char *extension_names[] = { ALL_GL_EXTS ALL_WGL_EXTS };
#undef USE_GL_EXT

#ifndef _WIN64

static char **wow64_strings;
static SIZE_T wow64_strings_count;

static CRITICAL_SECTION wow64_cs;
static CRITICAL_SECTION_DEBUG wow64_cs_debug =
{
    0, 0, &wow64_cs,
    { &wow64_cs_debug.ProcessLocksList, &wow64_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wow64_cs") }
};
static CRITICAL_SECTION wow64_cs = { &wow64_cs_debug, -1, 0, 0, 0, 0 };

static void append_wow64_string( char *str )
{
    char **tmp;

    EnterCriticalSection( &wow64_cs );

    if (!(tmp = realloc( wow64_strings, (wow64_strings_count + 1) * sizeof(*wow64_strings) )))
        ERR( "Failed to allocate memory for wow64 strings\n" );
    else
    {
        wow64_strings = tmp;
        wow64_strings[wow64_strings_count] = str;
        wow64_strings_count += 1;
    }

    LeaveCriticalSection( &wow64_cs );
}

static void cleanup_wow64_strings(void)
{
    while (wow64_strings_count--) free( wow64_strings[wow64_strings_count] );
    free( wow64_strings );
}

#endif

static const char *debugstr_object_type( enum object_type type )
{
    switch (type)
    {
    case OBJ_TYPE_BUFFER: return "buffer";
    case OBJ_TYPE_DISPLAY_LIST: return "display list";
    case OBJ_TYPE_FRAMEBUFFER: return "framebuffer";
    case OBJ_TYPE_MEMORY: return "memory";
    case OBJ_TYPE_PATH: return "path";
    case OBJ_TYPE_PROGRAM: return "program";
    case OBJ_TYPE_RENDERBUFFER: return "renderbuffer";
    case OBJ_TYPE_SEMAPHORE: return "semaphore";
    case OBJ_TYPE_SHADER: return "shader";
    case OBJ_TYPE_SAMPLER: return "sampler";
    case OBJ_TYPE_SHADER_ATI: return "fragment shader";
    case OBJ_TYPE_SHADER_EXT: return "vertex shader";
    case OBJ_TYPE_TEXTURE: return "texture";
    case OBJ_TYPE_COUNT: break;
    }
    return wine_dbg_sprintf( "object (type %u)", type );
}

static void init_wgl_extensions( const BOOLEAN extensions[GL_EXTENSION_COUNT] )
{
    UINT pos = 0, len = 0, ext;
    char *str;

    for (ext = WGL_FIRST_EXTENSION; ext < GL_EXTENSION_COUNT; ext++)
        if (extensions[ext]) len += strlen( extension_names[ext] ) + 1;

    if (!(str = malloc( len + 1 ))) return;

    for (ext = WGL_FIRST_EXTENSION; ext < GL_EXTENSION_COUNT; ext++)
        if (extensions[ext]) pos += sprintf( str + pos, "%s ", extension_names[ext] );
    str[pos - 1] = 0;

    wgl_extensions = str;
}

struct handle_entry
{
    UINT handle;
    union
    {
        struct opengl_client_context *context;
        struct opengl_client_pbuffer *pbuffer;
        struct handle_entry *next_free;
        void *user_data;
    };
};

struct handle_table
{
    SRWLOCK              lock;
    struct handle_entry  handles[1024];
    struct handle_entry *next_free;
    UINT                 count;
};

static struct handle_table pbuffers;
static struct handle_table contexts;

static struct handle_entry *alloc_handle( struct handle_table *table, void *user_data )
{
    struct handle_entry *ptr = NULL;
    WORD generation;

    AcquireSRWLockExclusive( &table->lock );
    if ((ptr = table->next_free)) table->next_free = ptr->next_free;
    else if (table->count < ARRAY_SIZE(table->handles)) ptr = table->handles + table->count++;
    else ptr = NULL;

    if (ptr)
    {
        if (!(generation = HIWORD( ptr->handle ) + 1)) generation++;
        ptr->handle = MAKELONG( ptr - table->handles + 1, generation );
        ptr->user_data = user_data;
    }

    if (!ptr) RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
    ReleaseSRWLockExclusive( &table->lock );
    return ptr;
}

static void free_handle( struct handle_table *table, struct handle_entry *ptr )
{
    AcquireSRWLockExclusive( &table->lock );
    ptr->handle |= 0xffff;
    ptr->next_free = table->next_free;
    table->next_free = ptr;
    ReleaseSRWLockExclusive( &table->lock );
}

static struct handle_entry *get_handle_ptr( struct handle_table *table, HANDLE handle )
{
    WORD index = LOWORD( handle ) - 1;
    struct handle_entry *ptr = table->handles + index;

    AcquireSRWLockShared( &table->lock );
    if (index >= table->count || ULongToHandle( ptr->handle ) != handle)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        ptr = NULL;
    }
    ReleaseSRWLockShared( &table->lock );

    return ptr;
}

static struct opengl_client_pbuffer *pbuffer_from_handle( HPBUFFERARB handle )
{
    struct handle_entry *ptr;
    if (!(ptr = get_handle_ptr( &pbuffers, handle ))) return NULL;
    return ptr->pbuffer;
}

BOOL get_pbuffer_from_handle( HPBUFFERARB handle, HPBUFFERARB *obj )
{
    struct opengl_client_pbuffer *pbuffer = pbuffer_from_handle( handle );
    *obj = pbuffer ? &pbuffer->obj : NULL;
    return pbuffer || !handle;
}

static struct handle_entry *alloc_client_pbuffer(void)
{
    struct opengl_client_pbuffer *pbuffer;
    struct handle_entry *ptr;

    if (!(pbuffer = calloc( 1, sizeof(*pbuffer) ))) return NULL;
    if (!(ptr = alloc_handle( &pbuffers, pbuffer ))) free( pbuffer );
    return ptr;
}

static void free_client_pbuffer( struct handle_entry *ptr )
{
    struct opengl_client_pbuffer *pbuffer = ptr->pbuffer;
    free_handle( &pbuffers, ptr );
    free( pbuffer );
}

HPBUFFERARB WINAPI wglCreatePbufferARB( HDC hdc, int format, int width, int height, const int *attribs )
{
    struct wglCreatePbufferARB_params args = { .teb = NtCurrentTeb(), .hDC = hdc, .iPixelFormat = format, .iWidth = width, .iHeight = height, .piAttribList = attribs };
    struct handle_entry *ptr;
    NTSTATUS status;

    TRACE( "hdc %p, format %d, width %d, height %d, attribs %p\n", hdc, format, width, height, attribs );

    if (!(ptr = alloc_client_pbuffer())) return 0;
    args.ret = &ptr->pbuffer->obj;

    if ((status = UNIX_CALL( wglCreatePbufferARB, &args ))) WARN( "wglCreatePbufferARB returned %#lx\n", status );
    assert( args.ret == &ptr->pbuffer->obj || !args.ret );

    if (!status && args.ret) return UlongToHandle( ptr->handle );
    free_client_pbuffer( ptr );
    return NULL;
}

BOOL WINAPI wglDestroyPbufferARB( HPBUFFERARB handle )
{
    struct wglDestroyPbufferARB_params args = { .teb = NtCurrentTeb() };
    struct handle_entry *ptr;
    NTSTATUS status;

    TRACE( "handle %p\n", handle );

    if (!(ptr = get_handle_ptr( &pbuffers, handle ))) return FALSE;
    args.hPbuffer = &ptr->pbuffer->obj;

    if ((status = UNIX_CALL( wglDestroyPbufferARB, &args ))) WARN( "wglDestroyPbufferARB returned %#lx\n", status );
    if (args.ret) free_client_pbuffer( ptr );

    return args.ret;
}

#define L1_COUNT   0x80
#define L2_COUNT  0x400
#define L3_COUNT 0x8000

struct object_table
{
    enum object_type    type;                   /* object type of the id table */
    SRWLOCK             lock;                   /* lock for accessing the table */
    GLuint            **host_ids[L1_COUNT];     /* client -> host id mapping sparse array */
    GLuint            **client_ids[L1_COUNT];   /* host -> client id mapping sparse array */
    GLuint              min_free;               /* id to start looking for a free slot */
    BOOL                implicit;               /* table allows implicit allocation */
};

static GLuint *find_object_id( GLuint **ids[L1_COUNT], GLuint client_id )
{
    GLuint i = client_id / L3_COUNT / L2_COUNT, j = (client_id / L3_COUNT) % L2_COUNT, k = client_id % L3_COUNT;
    return ids[i] ? ids[i][j] ? ids[i][j] + k : NULL : NULL;
}

static GLuint *alloc_object_ids( GLuint **ids[L1_COUNT], GLuint client_id )
{
    GLuint i = client_id / L3_COUNT / L2_COUNT, j = (client_id / L3_COUNT) % L2_COUNT;
    GLuint **ptr;

    if (!(ptr = ids[i]) && !(ptr = ids[i] = calloc( L2_COUNT, sizeof(*ptr) ))) return NULL;
    if (!ptr[j] && !(ptr[j] = calloc( L3_COUNT, sizeof(*ptr[j]) ))) return NULL;
    return ptr[j];
}

static void free_object_ids( struct object_table *table, GLuint **ids[L1_COUNT],
                             void (*callback)(struct object_table *, GLuint, GLuint) )
{
    GLuint id, **l1_block, *l2_block;

    for (int i = 0; i < L1_COUNT; i++)
    {
        if (!(l1_block = ids[i])) continue;
        for (int j = 0; j < L2_COUNT; j++)
        {
            if (!(l2_block = l1_block[j])) continue;
            for (int k = 0; callback && k < L3_COUNT; k++)
            {
                if (!(id = l2_block[k])) continue;
                callback( table, id, (i * L2_COUNT + j) * L3_COUNT + k );
            }
            free( l2_block );
        }
        free( l1_block );
    }
}

static GLuint alloc_client_id( struct object_table *table, GLuint host_id, UINT range )
{
    /* if we don't need implicit allocations, use the host allocated ids directly */
    if (!table->implicit) return host_id;

    /* otherwise we need to allocate client id ourselves, lookup for a free id */
    for (GLuint id = table->min_free + 1, n = range, *ids; id != 0; id++)
    {
        if (!(ids = alloc_object_ids( table->host_ids, id ))) return 0;
        if (ids[id % L3_COUNT]) n = range;
        else if (!n--) return table->min_free = id - range;
    }

    return 0;
}

static GLuint set_object( struct object_table *table, GLuint client_id, GLuint host_id )
{
    GLuint *ids;

    if (!(ids = alloc_object_ids( table->host_ids, client_id ))) goto failed;
    ids[client_id % L3_COUNT] = host_id;

    if (table->implicit || table->type == OBJ_TYPE_SHADER /* for destruction check */)
    {
        if (!(ids = alloc_object_ids( table->client_ids, host_id ))) goto failed;
        ids[host_id % L3_COUNT] = client_id;
    }

    TRACE( "Inserted %s client %#x, host %#x\n", debugstr_object_type(table->type), client_id, host_id );
    return client_id;

failed:
    ERR( "Failed to allocate object id block\n" );
    return -1;
}

static GLuint del_object( struct object_table *table, GLuint client_id )
{
    GLuint *object, host_id = 0;

    if (!client_id || !(object = find_object_id( table->host_ids, client_id ))) return -1;
    table->min_free = min( table->min_free, client_id - 1 );
    host_id = *object;
    if (table->type != OBJ_TYPE_SHADER) *object = 0; /* shader objects may outlive their deletion */
    if (host_id && (object = find_object_id( table->client_ids, host_id ))) *object = 0;

    TRACE( "Deleting %s client %#x, host %#x\n", debugstr_object_type( table->type ), client_id, host_id );
    return host_id ? host_id : -1;
}

static GLuint get_object( struct object_table *table, GLuint client_id, BOOL check )
{
    const GLuint *object = find_object_id( table->host_ids, client_id );
    GLuint host_id = object ? *object : 0;

    TRACE( "Found %s client %#x, host %#x\n", debugstr_object_type( table->type ), client_id, host_id );
    return check || host_id ? host_id : -1;
}

#define MAKE_OBJECT_CALL( func, ... )                                                              \
    struct func##_params args = { .teb = NtCurrentTeb(), __VA_ARGS__ };                            \
    UNIX_CALL( func, &args )

static GLuint create_object( enum object_type type )
{
    GLuint object;

    switch (type)
    {
    case OBJ_TYPE_BUFFER: { MAKE_OBJECT_CALL( glGenBuffers, .n = 1, .buffers = &object ); return object; }
    case OBJ_TYPE_DISPLAY_LIST: { MAKE_OBJECT_CALL( glGenLists, .range = 1 ); return args.ret; }
    case OBJ_TYPE_FRAMEBUFFER: { MAKE_OBJECT_CALL( glGenFramebuffers, .n = 1, .framebuffers = &object ); return object; }
    case OBJ_TYPE_MEMORY: assert( 0 ); return 0;
    case OBJ_TYPE_PATH: { MAKE_OBJECT_CALL( glGenPathsNV, .range = 1 ); return args.ret; }
    case OBJ_TYPE_PROGRAM: { MAKE_OBJECT_CALL( glGenProgramsARB, .n = 1, .programs = &object ); return object; }
    case OBJ_TYPE_RENDERBUFFER: { MAKE_OBJECT_CALL( glGenRenderbuffers, .n = 1, .renderbuffers = &object ); return object; }
    case OBJ_TYPE_SAMPLER: { MAKE_OBJECT_CALL( glGenSamplers, .count = 1, .samplers = &object ); return object; }
    case OBJ_TYPE_SEMAPHORE: { MAKE_OBJECT_CALL( glGenSemaphoresEXT, .n = 1, .semaphores = &object ); return object; }
    case OBJ_TYPE_SHADER: assert( 0 ); return 0;
    case OBJ_TYPE_SHADER_ATI: { MAKE_OBJECT_CALL( glGenFragmentShadersATI, .range = 1 ); return args.ret; }
    case OBJ_TYPE_SHADER_EXT: { MAKE_OBJECT_CALL( glGenVertexShadersEXT, .range = 1 ); return args.ret; }
    case OBJ_TYPE_TEXTURE: { MAKE_OBJECT_CALL( glGenTextures, .n = 1, .textures = &object ); return object; }
    case OBJ_TYPE_COUNT: break;
    }

    return 0;
}

static void destroy_object( enum object_type type, GLuint object )
{
    switch (type)
    {
    case OBJ_TYPE_BUFFER: { MAKE_OBJECT_CALL( glDeleteBuffers, .n = 1, .buffers = &object ); return; }
    case OBJ_TYPE_DISPLAY_LIST: { MAKE_OBJECT_CALL( glDeleteLists, .range = 1, .list = object ); return; }
    case OBJ_TYPE_FRAMEBUFFER: { MAKE_OBJECT_CALL( glDeleteFramebuffers, .n = 1, .framebuffers = &object ); return; }
    case OBJ_TYPE_MEMORY: { MAKE_OBJECT_CALL( glDeleteMemoryObjectsEXT, .n = 1, .memoryObjects = &object ); return; }
    case OBJ_TYPE_PATH: { MAKE_OBJECT_CALL( glDeletePathsNV, .range = 1, .path = object ); return; }
    case OBJ_TYPE_PROGRAM: { MAKE_OBJECT_CALL( glDeleteProgramsARB, .n = 1, .programs = &object ); return; }
    case OBJ_TYPE_RENDERBUFFER: { MAKE_OBJECT_CALL( glDeleteRenderbuffers, .n = 1, .renderbuffers = &object ); return; }
    case OBJ_TYPE_SAMPLER: { MAKE_OBJECT_CALL( glDeleteSamplers, .count = 1, .samplers = &object ); return; }
    case OBJ_TYPE_SEMAPHORE: { MAKE_OBJECT_CALL( glDeleteSemaphoresEXT, .n = 1, .semaphores = &object ); return; }
    case OBJ_TYPE_SHADER: { MAKE_OBJECT_CALL( glDeleteObjectARB, .obj = object ); return; }
    case OBJ_TYPE_SHADER_ATI: { MAKE_OBJECT_CALL( glDeleteFragmentShaderATI, .id = object ); return; }
    case OBJ_TYPE_SHADER_EXT: { MAKE_OBJECT_CALL( glDeleteVertexShaderEXT, .id = object ); return; }
    case OBJ_TYPE_TEXTURE: { MAKE_OBJECT_CALL( glDeleteTextures, .n = 1, .textures = &object ); return; }
    case OBJ_TYPE_COUNT: return;
    }
}

#undef MAKE_OBJECT_CALL

static void destroy_host_object( struct object_table *table, GLuint host_id, GLuint client_id )
{
    WARN( "Destroying %s client %#x, host %#x\n", debugstr_object_type( table->type ), client_id, host_id );
    destroy_object( table->type, host_id );
}

static void destroy_host_shader( struct object_table *table, GLuint host_id, GLuint client_id )
{
    GLuint *object;
    if (!(object = find_object_id( table->client_ids, host_id )) || !(client_id = *object)) return;
    WARN( "Destroying %s client %#x, host %#x\n", debugstr_object_type( table->type ), client_id, host_id );
    destroy_object( table->type, host_id );
}

static void free_object_table( struct object_table *table )
{
    if (table->type == OBJ_TYPE_SHADER) free_object_ids( table, table->host_ids, destroy_host_shader );
    else free_object_ids( table, table->host_ids, destroy_host_object );
    free_object_ids( table, table->client_ids, NULL );
}

static void init_object_table( struct object_table *table, enum object_type type )
{
    InitializeSRWLock( &table->lock );
    table->type = type;
}

struct display_lists
{
    LONG                refcount;
    LONG                modified;
    struct object_table tables[OBJ_TYPE_COUNT];
    struct handle_table syncs;
};

static struct display_lists *display_lists_create(void)
{
    struct display_lists *lists;

    if (!(lists = calloc( 1, sizeof(*lists) ))) return NULL;
    lists->refcount = 1;

    for (UINT i = 0; i < OBJ_TYPE_COUNT; i++)
        init_object_table( lists->tables + i, i );
    InitializeSRWLock( &lists->syncs.lock );

    return lists;
}

static struct display_lists *display_lists_acquire( struct display_lists *lists )
{
    InterlockedIncrement( &lists->refcount );
    return lists;
}

static void display_lists_release( struct display_lists *lists )
{
    struct glDeleteSync_params delete_sync = { .teb = NtCurrentTeb() };

    if (InterlockedDecrement( &lists->refcount )) return;

    for (UINT i = 0; i < OBJ_TYPE_COUNT; i++)
        free_object_table( lists->tables + i );

    for (int i = 0; i < lists->syncs.count; i++)
    {
        struct handle_entry *entry = lists->syncs.handles + i;
        if (LOWORD(entry->handle) == 0xffff) continue;
        WARN( "Leaking sync client %#x, host %p\n", entry->handle, entry->user_data );
        delete_sync.sync = entry->user_data;
        UNIX_CALL( glDeleteSync, &delete_sync );
        free( entry->user_data );
    }

    free( lists );
}

struct context
{
    struct opengl_client_context base;
    struct display_lists *lists;
};

static struct context *context_from_opengl_client_context( struct opengl_client_context *base )
{
    return CONTAINING_RECORD( base, struct context, base );
}

static struct opengl_client_context *opengl_client_context_from_handle( HGLRC handle )
{
    struct handle_entry *ptr;
    if (!(ptr = get_handle_ptr( &contexts, handle ))) return NULL;
    return ptr->context;
}

static struct context *context_from_handle( HGLRC handle )
{
    return context_from_opengl_client_context( opengl_client_context_from_handle( handle ) );
}

BOOL get_context_from_handle( HGLRC handle, HGLRC *obj )
{
    struct context *context = context_from_handle( handle );
    *obj = context ? &context->base.obj : NULL;
    return context || !handle;
}

static struct handle_entry *alloc_client_context( struct context *share )
{
    struct context *context;
    struct handle_entry *ptr;

    if (!(context = calloc( 1, sizeof(*context) ))) return NULL;
    if (!(context->lists = share ? display_lists_acquire( share->lists ) : display_lists_create())) goto failed;
    if ((ptr = alloc_handle( &contexts, context ))) return ptr;

    display_lists_release( context->lists );
failed:
    free( context );
    return NULL;
}

static void free_client_context( struct handle_entry *ptr )
{
    struct context *context = context_from_opengl_client_context( ptr->context );

    display_lists_release( context->lists );

    free_handle( &contexts, ptr );
    free( context );
}

static struct context *get_current_context(void)
{
    HGLRC current = NtCurrentTeb()->glCurrentRC;
    return current ? context_from_handle( current ) : NULL;
}

void set_gl_error( GLenum error )
{
    struct opengl_client_context *context;
    if (!(context = opengl_client_context_from_handle( NtCurrentTeb()->glCurrentRC ))) return;
    if (!context->last_error && !(context->last_error = glGetError())) context->last_error = error;
}

static struct object_table *get_object_table( struct context *ctx, enum object_type type, BOOL write )
{
    if (write) InterlockedExchange( &ctx->lists->modified, 1 );
    return type < OBJ_TYPE_COUNT ? ctx->lists->tables + type : NULL;
}

void put_context_objects( enum object_type type, UINT n, GLuint *handles )
{
    struct object_table *table;
    struct context *ctx;

    if (!(ctx = get_current_context())) return;
    if (!(table = get_object_table( ctx, type, TRUE ))) return;

    AcquireSRWLockExclusive( &table->lock );
    for (UINT i = 0; i < n; i++) handles[i] = handles[i] ? set_object( table, alloc_client_id( table, handles[i], 0 ), handles[i] ) : 0;
    ReleaseSRWLockExclusive( &table->lock );
}

GLuint put_context_object_range( enum object_type type, UINT range, GLuint base )
{
    struct object_table *table;
    struct context *ctx;
    GLuint first;

    if (!(ctx = get_current_context())) return base;
    if (!(table = get_object_table( ctx, type, TRUE ))) return base;

    AcquireSRWLockExclusive( &table->lock );
    first = alloc_client_id( table, base, range );
    for (UINT i = 0; i < range; i++) set_object( table, first + i, base + i );
    ReleaseSRWLockExclusive( &table->lock );

    return first;
}

static void alloc_client_objects( struct context *ctx, enum object_type type, UINT n, const GLuint *handles )
{
    struct object_table *table;

    if (!(table = get_object_table( ctx, type, TRUE ))) return;

    AcquireSRWLockExclusive( &table->lock );
    for (UINT i = 0; i < n; i++)
    {
        if (!handles[i] || get_object( table, handles[i], TRUE )) continue;
        WARN( "Creating implicit %s client %#x\n", debugstr_object_type( type ), handles[i] );
        set_object( table, handles[i], create_object( table->type ) );
        table->implicit = TRUE; /* from now on we cannot rely on host-allocated ids */
    }
    ReleaseSRWLockExclusive( &table->lock );
}

BOOL alloc_context_objects( enum object_type type, UINT n, const GLuint *handles, BOOL extension )
{
    BOOL alloc_client, needs_client = FALSE;
    struct object_table *table;
    struct context *ctx;

    if (!(ctx = get_current_context())) return TRUE;
    if (!(table = get_object_table( ctx, type, FALSE ))) return TRUE;

    /* only allow explicit allocation in some cases, use host allocated ids directly in that case */
    switch (type)
    {
    case OBJ_TYPE_DISPLAY_LIST:
        if (ctx->base.profile_mask & WGL_CONTEXT_CORE_PROFILE_BIT_ARB) return FALSE;
        alloc_client = TRUE;
        break;
    case OBJ_TYPE_FRAMEBUFFER:
    case OBJ_TYPE_RENDERBUFFER:
    case OBJ_TYPE_PROGRAM:
    case OBJ_TYPE_SHADER_EXT:
    case OBJ_TYPE_SHADER_ATI:
    case OBJ_TYPE_SEMAPHORE:
    case OBJ_TYPE_PATH:
        alloc_client = extension;
        break;
    case OBJ_TYPE_SAMPLER:
    case OBJ_TYPE_MEMORY:
    case OBJ_TYPE_SHADER:
        alloc_client = FALSE;
        break;
    default:
        alloc_client = !(ctx->base.profile_mask & WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
        break;
    }

    AcquireSRWLockShared( &table->lock );
    for (UINT i = 0; i < n && !needs_client; i++)
        needs_client = handles[i] && !get_object( table, handles[i], TRUE );
    ReleaseSRWLockShared( &table->lock );
    if (!needs_client) return TRUE;

    if (alloc_client) alloc_client_objects( ctx, type, n, handles );
    else set_gl_error( GL_INVALID_OPERATION );

    return alloc_client;
}

GLuint *del_context_objects( enum object_type type, UINT n, GLuint *handles )
{
    struct object_table *table;
    struct context *ctx;

    if (!(ctx = get_current_context())) return handles;
    if (!(table = get_object_table( ctx, type, FALSE ))) return handles;

    AcquireSRWLockExclusive( &table->lock );
    for (UINT i = 0; i < n; i++) handles[i] = del_object( table, handles[i] );
    ReleaseSRWLockExclusive( &table->lock );

    return handles;
}

GLuint *map_context_objects( enum object_type type, UINT n, GLuint *handles )
{
    struct object_table *table;
    struct context *ctx;

    if (!(ctx = get_current_context())) return handles;
    if (!(table = get_object_table( ctx, type, FALSE ))) return handles;

    AcquireSRWLockShared( &table->lock );
    while (n--) handles[n] = handles[n] ? get_object( table, handles[n], FALSE ) : 0;
    ReleaseSRWLockShared( &table->lock );

    return handles;
}

static GLuint get_pname_object_type( GLenum pname )
{
    switch (pname)
    {
    case GL_ARRAY_BUFFER_BINDING:
    case GL_ATOMIC_COUNTER_BUFFER_BINDING:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
    case GL_COPY_READ_BUFFER_BINDING:
    case GL_COPY_WRITE_BUFFER_BINDING:
    case GL_DISPATCH_INDIRECT_BUFFER_BINDING:
    case GL_DRAW_INDIRECT_BUFFER_BINDING:
    case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
    case GL_FOG_COORD_ARRAY_BUFFER_BINDING:
    case GL_INDEX_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_PARAMETER_BUFFER_BINDING:
    case GL_PIXEL_PACK_BUFFER_BINDING:
    case GL_PIXEL_UNPACK_BUFFER_BINDING:
    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
    case GL_QUERY_BUFFER_BINDING:
    case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING:
    case GL_SHADER_STORAGE_BUFFER_BINDING:
    case GL_TEXTURE_BUFFER_DATA_STORE_BINDING:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
    case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
    case GL_UNIFORM_BLOCK_BINDING:
    case GL_UNIFORM_BUFFER_BINDING:
    case GL_UNIFORM_BUFFER_BINDING_EXT:
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_BINDING_BUFFER:
    case GL_VIDEO_BUFFER_BINDING_NV:
    case GL_WEIGHT_ARRAY_BUFFER_BINDING:
    case GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
        return OBJ_TYPE_BUFFER;
    case GL_READ_FRAMEBUFFER_BINDING:
    case GL_DRAW_FRAMEBUFFER_BINDING:
        return OBJ_TYPE_FRAMEBUFFER;
    case GL_RENDERBUFFER_BINDING:
    case GL_TEXTURE_RENDERBUFFER_DATA_STORE_BINDING_NV:
        return OBJ_TYPE_RENDERBUFFER;
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_1D_ARRAY:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_2D_ARRAY:
    case GL_TEXTURE_BINDING_2D_MULTISAMPLE:
    case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
    case GL_TEXTURE_BINDING_3D:
    case GL_TEXTURE_BINDING_BUFFER:
    case GL_TEXTURE_BINDING_CUBE_MAP:
    case GL_TEXTURE_BINDING_CUBE_MAP_ARRAY:
    case GL_TEXTURE_BINDING_EXTERNAL_OES:
    case GL_TEXTURE_BINDING_RECTANGLE:
    case GL_TEXTURE_BUFFER_BINDING:
    case GL_TEXTURE_BINDING_RENDERBUFFER_NV:
    case GL_TEXTURE_1D_STACK_BINDING_MESAX:
    case GL_TEXTURE_2D_STACK_BINDING_MESAX:
    case GL_TEXTURE_4D_BINDING_SGIS:
    case GL_DETAIL_TEXTURE_2D_BINDING_SGIS:
    case GL_SHADING_RATE_IMAGE_BINDING_NV:
    case GL_IMAGE_BINDING_NAME:
        return OBJ_TYPE_TEXTURE;
    case GL_SAMPLER_BINDING:
        return OBJ_TYPE_SAMPLER;
    case GL_LIST_INDEX:
        return OBJ_TYPE_DISPLAY_LIST;
    case GL_PROGRAM_BINDING_ARB:
    case GL_VERTEX_PROGRAM_BINDING_NV:
    case GL_FRAGMENT_PROGRAM_BINDING_NV:
        return OBJ_TYPE_PROGRAM;
    case GL_COMPUTE_SHADER:
    case GL_CURRENT_PROGRAM:
    case GL_FRAGMENT_SHADER:
    case GL_GEOMETRY_SHADER:
    case GL_TESS_CONTROL_SHADER:
    case GL_TESS_EVALUATION_SHADER:
    case GL_VERTEX_SHADER:
    case GL_ACTIVE_PROGRAM:
        return OBJ_TYPE_SHADER;
    case GL_VERTEX_SHADER_BINDING_EXT:
        return OBJ_TYPE_SHADER_EXT;
    }

    return OBJ_TYPE_COUNT;
}

static BOOL map_client_objects( enum object_type type, GLuint host_id, GLuint *ret )
{
    GLuint *object, client_id = host_id;
    struct object_table *table;
    struct context *ctx;

    if (!host_id || type == OBJ_TYPE_COUNT) return FALSE;
    if (!(ctx = get_current_context())) return FALSE;
    if (!(table = get_object_table( ctx, type, FALSE ))) return FALSE;

    AcquireSRWLockShared( &table->lock );
    if ((object = find_object_id( table->client_ids, host_id ))) client_id = *object;
    ReleaseSRWLockShared( &table->lock );

    *ret = client_id;
    return TRUE;
}

static void map_framebuffer_attachment_param( GLenum target, GLenum attachment, GLenum pname, GLint *params )
{
    GLint type, value;

    if (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME) return;
    glGetFramebufferAttachmentParameteriv( target, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type );
    if (type == GL_RENDERBUFFER && map_client_objects( OBJ_TYPE_RENDERBUFFER, *params, (GLuint *)&value )) *params = value;
    if (type == GL_TEXTURE && map_client_objects( OBJ_TYPE_TEXTURE, *params, (GLuint *)&value )) *params = value;
}

void WINAPI glGetFramebufferAttachmentParameteriv( GLenum target, GLenum attachment, GLenum pname, GLint *params )
{
    struct glGetFramebufferAttachmentParameteriv_params args = { .teb = NtCurrentTeb(), .target = target, .attachment = attachment, .pname = pname, .params = params };
    NTSTATUS status;

    TRACE( "target %d, attachment %d, pname %d, params %p\n", target, attachment, pname, params );

    if ((status = UNIX_CALL( glGetFramebufferAttachmentParameteriv, &args ))) WARN( "glGetFramebufferAttachmentParameteriv returned %#lx\n", status );
    map_framebuffer_attachment_param( target, attachment, pname, params );
}

void WINAPI glGetFramebufferAttachmentParameterivEXT( GLenum target, GLenum attachment, GLenum pname, GLint *params )
{
    struct glGetFramebufferAttachmentParameterivEXT_params args = { .teb = NtCurrentTeb(), .target = target, .attachment = attachment, .pname = pname, .params = params };
    NTSTATUS status;

    TRACE( "target %d, attachment %d, pname %d, params %p\n", target, attachment, pname, params );

    if ((status = UNIX_CALL( glGetFramebufferAttachmentParameterivEXT, &args ))) WARN( "glGetFramebufferAttachmentParameterivEXT returned %#lx\n", status );
    map_framebuffer_attachment_param( target, attachment, pname, params );
}

static void map_named_framebuffer_attachment_param( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params )
{
    GLint type, value;

    if (pname != GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME) return;
    glGetNamedFramebufferAttachmentParameteriv( framebuffer, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type );
    if (type == GL_RENDERBUFFER && map_client_objects( OBJ_TYPE_RENDERBUFFER, *params, (GLuint *)&value )) *params = value;
    if (type == GL_TEXTURE && map_client_objects( OBJ_TYPE_TEXTURE, *params, (GLuint *)&value )) *params = value;
}

void WINAPI glGetNamedFramebufferAttachmentParameteriv( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params )
{
    struct glGetNamedFramebufferAttachmentParameteriv_params args = { .teb = NtCurrentTeb(), .attachment = attachment, .pname = pname, .params = params };
    GLuint host_framebuffer = framebuffer;
    NTSTATUS status;

    TRACE( "framebuffer %d, attachment %d, pname %d, params %p\n", framebuffer, attachment, pname, params );

    args.framebuffer = *map_context_objects( OBJ_TYPE_FRAMEBUFFER, 1, &host_framebuffer );
    if ((status = UNIX_CALL( glGetNamedFramebufferAttachmentParameteriv, &args ))) WARN( "glGetNamedFramebufferAttachmentParameteriv returned %#lx\n", status );
    map_named_framebuffer_attachment_param( framebuffer, attachment, pname, params );
}

void WINAPI glGetNamedFramebufferAttachmentParameterivEXT( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params )
{
    struct glGetNamedFramebufferAttachmentParameterivEXT_params args = { .teb = NtCurrentTeb(), .attachment = attachment, .pname = pname, .params = params };
    GLuint host_framebuffer = framebuffer;
    NTSTATUS status;

    TRACE( "framebuffer %d, attachment %d, pname %d, params %p\n", framebuffer, attachment, pname, params );

    if (!alloc_context_objects( OBJ_TYPE_FRAMEBUFFER, 1, &host_framebuffer, TRUE )) return;
    args.framebuffer = *map_context_objects( OBJ_TYPE_FRAMEBUFFER, 1, &host_framebuffer );
    if ((status = UNIX_CALL( glGetNamedFramebufferAttachmentParameterivEXT, &args ))) WARN( "glGetNamedFramebufferAttachmentParameterivEXT returned %#lx\n", status );
    map_named_framebuffer_attachment_param( framebuffer, attachment, pname, params );
}

HGLRC WINAPI wglCreateContext( HDC hdc )
{
    static const int attribs[] = { WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, 0, 0 };
    TRACE( "hdc %p\n", hdc );
    return wglCreateContextAttribsARB( hdc, NULL, attribs );
}

HGLRC WINAPI wglCreateContextAttribsARB( HDC hdc, HGLRC share, const int *attribs )
{
    struct wglCreateContextAttribsARB_params args = { .teb = NtCurrentTeb(), .hDC = hdc, .attribList = attribs };
    struct context *share_context = NULL;
    struct handle_entry *ptr;
    NTSTATUS status;

    TRACE( "hdc %p, share %p, attribs %p\n", hdc, share, attribs );

    if (share && !(share_context = context_from_handle( share )))
    {
        SetLastError( ERROR_INVALID_OPERATION );
        return NULL;
    }
    if (share) args.hShareContext = &share_context->base.obj;

    if (!(ptr = alloc_client_context( share_context ))) return NULL;
    args.ret = &ptr->context->obj;

    if ((status = UNIX_CALL( wglCreateContextAttribsARB, &args ))) WARN( "wglCreateContextAttribsARB returned %#lx\n", status );
    assert( args.ret == &ptr->context->obj || !args.ret );

    if (!status && args.ret) return UlongToHandle( ptr->handle );
    free_client_context( ptr );
    return NULL;
}

BOOL WINAPI wglDeleteContext( HGLRC handle )
{
    TEB *teb = NtCurrentTeb();
    struct wglDeleteContext_params args = {.teb = teb};
    struct handle_entry *ptr;
    NTSTATUS status;

    TRACE( "handle %p\n", handle );

    if (!(ptr = get_handle_ptr( &contexts, handle ))) return FALSE;
    args.oldContext = &ptr->context->obj;

    if (handle == teb->glCurrentRC) wglMakeCurrent( NULL, NULL );
    if (ptr->context->current_tid)
    {
        SetLastError( ERROR_BUSY );
        return FALSE;
    }

    if ((status = UNIX_CALL( wglDeleteContext, &args ))) WARN( "wglDeleteContext returned %#lx\n", status );
    if (status || !args.ret) return FALSE;

    /* make sure there's a (dummy) context before releasing and destroying display list objects */
    if (!teb->glCurrentRC) wglMakeContextCurrentARB( NULL, NULL, NULL );
    free_client_context( ptr );
    return TRUE;
}

BOOL WINAPI wglMakeCurrent( HDC hdc, HGLRC handle )
{
    TRACE( "hdc %p, handle %p\n", hdc, handle );
    if (!hdc && !handle && !NtCurrentTeb()->glCurrentRC)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return wglMakeContextCurrentARB( hdc, hdc, handle );
}

BOOL WINAPI wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, HGLRC handle )
{
    TEB *teb = NtCurrentTeb();
    struct wglMakeContextCurrentARB_params args = { .teb = teb, .hDrawDC = draw_hdc, .hReadDC = read_hdc };
    struct opengl_client_context *context = NULL, *previous = opengl_client_context_from_handle( teb->glCurrentRC );
    NTSTATUS status;

    TRACE( "draw_hdc %p, read_hdc %p, handle %p\n", draw_hdc, read_hdc, handle );

    if (!get_context_from_handle( handle, &args.hglrc )) return FALSE;
    if ((context = opengl_client_context_from_handle( handle )) &&
        context->current_tid && context->current_tid != GetCurrentThreadId())
    {
        SetLastError( ERROR_BUSY );
        return FALSE;
    }

    if ((status = UNIX_CALL( wglMakeContextCurrentARB, &args ))) WARN( "wglMakeContextCurrentARB returned %#lx\n", status );
    if (status || !args.ret) return FALSE;

    if (context) context->current_tid = GetCurrentThreadId();
    if (previous) previous->current_tid = 0;
    teb->glCurrentRC = handle;
    teb->glReserved1[0] = draw_hdc;
    teb->glReserved1[1] = read_hdc;
    return TRUE;
}

/***********************************************************************
 *      wglShareLists
 */
BOOL WINAPI wglShareLists( HGLRC src_handle, HGLRC dst_handle )
{
    struct context *src_context, *dst_context;
    struct display_lists *lists;

    TRACE( "src_handle %p, dst_handle %p\n", src_handle, dst_handle );

    if (!(src_context = context_from_handle( src_handle ))) return FALSE;
    if (!(dst_context = context_from_handle( dst_handle ))) return FALSE;
    if (ReadNoFence( &dst_context->lists->modified )) return FALSE;

    lists = display_lists_acquire( src_context->lists );
    lists = InterlockedExchangePointer( (void *)&dst_context->lists, lists );
    display_lists_release( lists );

    return TRUE;
}

/***********************************************************************
 *		wglGetCurrentReadDCARB
 *
 * Provided by the WGL_ARB_make_current_read extension.
 */
HDC WINAPI wglGetCurrentReadDCARB(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return 0;
    return NtCurrentTeb()->glReserved1[1];
}

/***********************************************************************
 *		wglGetCurrentDC (OPENGL32.@)
 */
HDC WINAPI wglGetCurrentDC(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return 0;
    return NtCurrentTeb()->glReserved1[0];
}

/***********************************************************************
 *		wglGetCurrentContext (OPENGL32.@)
 */
HGLRC WINAPI wglGetCurrentContext(void)
{
    return NtCurrentTeb()->glCurrentRC;
}

/***********************************************************************
 *		wglChoosePixelFormat (OPENGL32.@)
 */
INT WINAPI wglChoosePixelFormat(HDC hdc, const PIXELFORMATDESCRIPTOR* ppfd)
{
    PIXELFORMATDESCRIPTOR format, best;
    int i, count, best_format;
    int bestDBuffer = -1, bestStereo = -1;
    DWORD is_memdc;

    TRACE( "%p %p: size %u version %u flags %lu type %u color %u %u,%u,%u,%u "
           "accum %u depth %u stencil %u aux %u\n",
           hdc, ppfd, ppfd->nSize, ppfd->nVersion, ppfd->dwFlags, ppfd->iPixelType,
           ppfd->cColorBits, ppfd->cRedBits, ppfd->cGreenBits, ppfd->cBlueBits, ppfd->cAlphaBits,
           ppfd->cAccumBits, ppfd->cDepthBits, ppfd->cStencilBits, ppfd->cAuxBuffers );

    count = wglDescribePixelFormat( hdc, 0, 0, NULL );
    if (!count) return 0;

    if (!NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc )) is_memdc = 0;

    best_format = 0;
    best.dwFlags = 0;
    best.cAlphaBits = -1;
    best.cColorBits = -1;
    best.cDepthBits = -1;
    best.cStencilBits = -1;
    best.cAuxBuffers = -1;
    best.cAccumBits = -1;

    for (i = 1; i <= count; i++)
    {
        if (!wglDescribePixelFormat( hdc, i, sizeof(format), &format )) continue;

        if ((ppfd->iPixelType == PFD_TYPE_COLORINDEX) != (format.iPixelType == PFD_TYPE_COLORINDEX))
        {
            TRACE( "pixel type mismatch for iPixelFormat=%d\n", i );
            continue;
        }

        if ((ppfd->dwFlags & PFD_DRAW_TO_BITMAP) && !(format.dwFlags & PFD_DRAW_TO_BITMAP))
        {
            TRACE( "PFD_DRAW_TO_BITMAP required but not found for iPixelFormat=%d\n", i );
            continue;
        }
        if ((ppfd->dwFlags & PFD_DRAW_TO_WINDOW) && !(format.dwFlags & PFD_DRAW_TO_WINDOW))
        {
            TRACE( "PFD_DRAW_TO_WINDOW required but not found for iPixelFormat=%d\n", i );
            continue;
        }

        if ((ppfd->dwFlags & PFD_SUPPORT_GDI) && !(format.dwFlags & PFD_SUPPORT_GDI))
        {
            TRACE( "PFD_SUPPORT_GDI required but not found for iPixelFormat=%d\n", i );
            continue;
        }
        if ((ppfd->dwFlags & PFD_SUPPORT_OPENGL) && !(format.dwFlags & PFD_SUPPORT_OPENGL))
        {
            TRACE( "PFD_SUPPORT_OPENGL required but not found for iPixelFormat=%d\n", i );
            continue;
        }
        if (is_memdc && ppfd->cColorBits == 16 && ppfd->cAlphaBits != 1)
        {
            TRACE( "Ignoring iPixelFormat=%d RGB565 format on memory DC\n", i );
            continue;
        }

        /* The behavior of PDF_STEREO/PFD_STEREO_DONTCARE and PFD_DOUBLEBUFFER / PFD_DOUBLEBUFFER_DONTCARE
         * is not very clear on MSDN. They specify that ChoosePixelFormat tries to match pixel formats
         * with the flag (PFD_STEREO / PFD_DOUBLEBUFFERING) set. Otherwise it says that it tries to match
         * formats without the given flag set.
         * A test on Windows using a Radeon 9500pro on WinXP (the driver doesn't support Stereo)
         * has indicated that a format without stereo is returned when stereo is unavailable.
         * So in case PFD_STEREO is set, formats that support it should have priority above formats
         * without. In case PFD_STEREO_DONTCARE is set, stereo is ignored.
         *
         * To summarize the following is most likely the correct behavior:
         * stereo not set -> prefer non-stereo formats, but also accept stereo formats
         * stereo set -> prefer stereo formats, but also accept non-stereo formats
         * stereo don't care -> it doesn't matter whether we get stereo or not
         *
         * In Wine we will treat non-stereo the same way as don't care because it makes
         * format selection even more complicated and second drivers with Stereo advertise
         * each format twice anyway.
         */

        /* Doublebuffer, see the comments above */
        if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) &&
                ((format.dwFlags & PFD_DOUBLEBUFFER) == (ppfd->dwFlags & PFD_DOUBLEBUFFER)))
                goto found;

            if (bestDBuffer != -1 && (format.dwFlags & PFD_DOUBLEBUFFER) != bestDBuffer) continue;
        }
        else if (!best_format)
            goto found;

        /* Stereo, see the comments above. */
        if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE))
        {
            if (((ppfd->dwFlags & PFD_STEREO) != bestStereo) &&
                ((format.dwFlags & PFD_STEREO) == (ppfd->dwFlags & PFD_STEREO)))
                goto found;

            if (bestStereo != -1 && (format.dwFlags & PFD_STEREO) != bestStereo) continue;
        }
        else if (!best_format)
            goto found;

        /* Below we will do a number of checks to select the 'best' pixelformat.
         * We assume the precedence cColorBits > cAlphaBits > cDepthBits > cStencilBits -> cAuxBuffers.
         * The code works by trying to match the most important options as close as possible.
         * When a reasonable format is found, we will try to match more options.
         * It appears (see the opengl32 test) that Windows opengl drivers ignore options
         * like cColorBits, cAlphaBits and friends if they are set to 0, so they are considered
         * as DONTCARE. At least Serious Sam TSE relies on this behavior. */

        if (ppfd->cColorBits)
        {
            if (((ppfd->cColorBits > best.cColorBits) && (format.cColorBits > best.cColorBits)) ||
                ((format.cColorBits >= ppfd->cColorBits) && (format.cColorBits < best.cColorBits)))
                goto found;

            if (best.cColorBits != format.cColorBits)  /* Do further checks if the format is compatible */
            {
                TRACE( "color mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAlphaBits)
        {
            if (((ppfd->cAlphaBits > best.cAlphaBits) && (format.cAlphaBits > best.cAlphaBits)) ||
                ((format.cAlphaBits >= ppfd->cAlphaBits) && (format.cAlphaBits < best.cAlphaBits)))
                goto found;

            if (best.cAlphaBits != format.cAlphaBits)
            {
                TRACE( "alpha mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cStencilBits)
        {
            if (((ppfd->cStencilBits > best.cStencilBits) && (format.cStencilBits > best.cStencilBits)) ||
                ((format.cStencilBits >= ppfd->cStencilBits) && (format.cStencilBits < best.cStencilBits)))
                goto found;

            if (best.cStencilBits != format.cStencilBits)
            {
                TRACE( "stencil mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cDepthBits && !(ppfd->dwFlags & PFD_DEPTH_DONTCARE))
        {
            if (((ppfd->cDepthBits > best.cDepthBits) && (format.cDepthBits > best.cDepthBits)) ||
                ((format.cDepthBits >= ppfd->cDepthBits) && (format.cDepthBits < best.cDepthBits)))
                goto found;

            if (best.cDepthBits != format.cDepthBits)
            {
                TRACE( "depth mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAuxBuffers)
        {
            if (((ppfd->cAuxBuffers > best.cAuxBuffers) && (format.cAuxBuffers > best.cAuxBuffers)) ||
                ((format.cAuxBuffers >= ppfd->cAuxBuffers) && (format.cAuxBuffers < best.cAuxBuffers)))
                goto found;

            if (best.cAuxBuffers != format.cAuxBuffers)
            {
                TRACE( "aux mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->cAccumBits)
        {
            if (((ppfd->cAccumBits > best.cAccumBits) && (format.cAccumBits > best.cAccumBits)) ||
                ((format.cAccumBits >= ppfd->cAccumBits) && (format.cAccumBits < best.cAccumBits)))
                goto found;

            if (best.cAccumBits != format.cAccumBits)
            {
                TRACE( "cAccumBits mismatch for iPixelFormat=%d\n", i );
                continue;
            }
        }
        if (ppfd->dwFlags & PFD_DEPTH_DONTCARE)
        {
            if (format.cDepthBits < best.cDepthBits)
                goto found;
            continue;
        }

        if (!ppfd->cDepthBits && format.cDepthBits > best.cDepthBits)
            goto found;

        continue;

    found:
        best_format = i;
        best = format;
        bestDBuffer = format.dwFlags & PFD_DOUBLEBUFFER;
        bestStereo = format.dwFlags & PFD_STEREO;
    }

    TRACE( "returning %u\n", best_format );
    return best_format;
}

static struct wgl_pixel_format *get_pixel_formats( HDC hdc, UINT *num_formats,
                                                   UINT *num_onscreen_formats )
{
    struct get_pixel_formats_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    PVOID *glReserved = NtCurrentTeb()->glReserved1;
    NTSTATUS status;
    DWORD is_memdc;

    *num_formats = *num_onscreen_formats = 0;
    if (!NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc ))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }

    if (glReserved[WINE_GL_RESERVED_FORMATS_HDC] == hdc)
    {
        *num_formats = PtrToUlong( glReserved[WINE_GL_RESERVED_FORMATS_NUM] );
        *num_onscreen_formats = PtrToUlong( glReserved[WINE_GL_RESERVED_FORMATS_ONSCREEN] );
        return glReserved[WINE_GL_RESERVED_FORMATS_PTR];
    }

    if ((status = UNIX_CALL( get_pixel_formats, &args ))) goto error;
    /* Clear formats memory since not all drivers deal with all wgl_pixel_format
     * fields at the moment. */
    if (!(args.formats = calloc( args.num_formats, sizeof(*args.formats) ))) goto error;
    args.max_formats = args.num_formats;
    if ((status = UNIX_CALL( get_pixel_formats, &args ))) goto error;

    if (is_memdc) args.num_onscreen_formats = args.num_formats;

    *num_formats = args.num_formats;
    *num_onscreen_formats = args.num_onscreen_formats;

    free( glReserved[WINE_GL_RESERVED_FORMATS_PTR] );
    glReserved[WINE_GL_RESERVED_FORMATS_HDC] = hdc;
    glReserved[WINE_GL_RESERVED_FORMATS_PTR] = args.formats;
    glReserved[WINE_GL_RESERVED_FORMATS_NUM] = ULongToPtr( args.num_formats );
    glReserved[WINE_GL_RESERVED_FORMATS_ONSCREEN] = ULongToPtr( args.num_onscreen_formats );

    return args.formats;

error:
    *num_formats = *num_onscreen_formats = 0;
    free( args.formats );
    return NULL;
}

static BOOL wgl_attrib_uses_layer( int attrib )
{
    switch (attrib)
    {
    case WGL_ACCELERATION_ARB:
    case WGL_TRANSPARENT_ARB:
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
    case WGL_TRANSPARENT_RED_VALUE_ARB:
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
    case WGL_SUPPORT_GDI_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
    case WGL_DOUBLE_BUFFER_ARB:
    case WGL_STEREO_ARB:
    case WGL_PIXEL_TYPE_ARB:
    case WGL_COLOR_BITS_ARB:
    case WGL_RED_BITS_ARB:
    case WGL_RED_SHIFT_ARB:
    case WGL_GREEN_BITS_ARB:
    case WGL_GREEN_SHIFT_ARB:
    case WGL_BLUE_BITS_ARB:
    case WGL_BLUE_SHIFT_ARB:
    case WGL_ALPHA_BITS_ARB:
    case WGL_ALPHA_SHIFT_ARB:
    case WGL_ACCUM_BITS_ARB:
    case WGL_ACCUM_RED_BITS_ARB:
    case WGL_ACCUM_GREEN_BITS_ARB:
    case WGL_ACCUM_BLUE_BITS_ARB:
    case WGL_ACCUM_ALPHA_BITS_ARB:
    case WGL_DEPTH_BITS_ARB:
    case WGL_STENCIL_BITS_ARB:
    case WGL_AUX_BUFFERS_ARB:
    case WGL_SAMPLE_BUFFERS_ARB:
    case WGL_SAMPLES_ARB:
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB:
    case WGL_FLOAT_COMPONENTS_NV:
        return TRUE;
    default:
        return FALSE;
    }
}

static BOOL wgl_pixel_format_get_attrib( const struct wgl_pixel_format *fmt, int attrib, int *value )
{
    int val = 0;
    int valid = -1;

    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB: val = !!(fmt->pfd.dwFlags & PFD_DRAW_TO_WINDOW); break;
    case WGL_DRAW_TO_BITMAP_ARB: val = !!(fmt->pfd.dwFlags & PFD_DRAW_TO_BITMAP); break;
    case WGL_ACCELERATION_ARB:
        if (fmt->pfd.dwFlags & PFD_GENERIC_ACCELERATED)
            val = WGL_GENERIC_ACCELERATION_ARB;
        else if (fmt->pfd.dwFlags & PFD_GENERIC_FORMAT)
            val = WGL_NO_ACCELERATION_ARB;
        else
            val = WGL_FULL_ACCELERATION_ARB;
        break;
    case WGL_NEED_PALETTE_ARB: val = !!(fmt->pfd.dwFlags & PFD_NEED_PALETTE); break;
    case WGL_NEED_SYSTEM_PALETTE_ARB: val = !!(fmt->pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE); break;
    case WGL_SWAP_LAYER_BUFFERS_ARB: val = !!(fmt->pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS); break;
    case WGL_SWAP_METHOD_ARB: val = fmt->swap_method; break;
    case WGL_NUMBER_OVERLAYS_ARB:
    case WGL_NUMBER_UNDERLAYS_ARB:
        /* We don't support any overlays/underlays. */
        val = 0;
        break;
    case WGL_TRANSPARENT_ARB: val = fmt->transparent; break;
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
        /* We support only a main plane at the moment which by definition
         * shares the depth/stencil/accum buffers with itself. */
        val = GL_TRUE;
        break;
    case WGL_SUPPORT_GDI_ARB: val = !!(fmt->pfd.dwFlags & PFD_SUPPORT_GDI); break;
    case WGL_SUPPORT_OPENGL_ARB: val = !!(fmt->pfd.dwFlags & PFD_SUPPORT_OPENGL); break;
    case WGL_DOUBLE_BUFFER_ARB: val = !!(fmt->pfd.dwFlags & PFD_DOUBLEBUFFER); break;
    case WGL_STEREO_ARB: val = !!(fmt->pfd.dwFlags & PFD_STEREO); break;
    case WGL_PIXEL_TYPE_ARB: val = fmt->pixel_type; break;
    case WGL_COLOR_BITS_ARB: val = fmt->pfd.cColorBits; break;
    case WGL_RED_BITS_ARB: val = fmt->pfd.cRedBits; break;
    case WGL_RED_SHIFT_ARB: val = fmt->pfd.cRedShift; break;
    case WGL_GREEN_BITS_ARB: val = fmt->pfd.cGreenBits; break;
    case WGL_GREEN_SHIFT_ARB: val = fmt->pfd.cGreenShift; break;
    case WGL_BLUE_BITS_ARB: val = fmt->pfd.cBlueBits; break;
    case WGL_BLUE_SHIFT_ARB: val = fmt->pfd.cBlueShift; break;
    case WGL_ALPHA_BITS_ARB: val = fmt->pfd.cAlphaBits; break;
    case WGL_ALPHA_SHIFT_ARB: val = fmt->pfd.cAlphaShift; break;
    case WGL_ACCUM_BITS_ARB: val = fmt->pfd.cAccumBits; break;
    case WGL_ACCUM_RED_BITS_ARB: val = fmt->pfd.cAccumRedBits; break;
    case WGL_ACCUM_GREEN_BITS_ARB: val = fmt->pfd.cAccumGreenBits; break;
    case WGL_ACCUM_BLUE_BITS_ARB: val = fmt->pfd.cAccumBlueBits; break;
    case WGL_ACCUM_ALPHA_BITS_ARB: val = fmt->pfd.cAccumAlphaBits; break;
    case WGL_DEPTH_BITS_ARB: val = fmt->pfd.cDepthBits; break;
    case WGL_STENCIL_BITS_ARB: val = fmt->pfd.cStencilBits; break;
    case WGL_AUX_BUFFERS_ARB: val = fmt->pfd.cAuxBuffers; break;
    case WGL_DRAW_TO_PBUFFER_ARB: val = fmt->draw_to_pbuffer; break;
    case WGL_MAX_PBUFFER_PIXELS_ARB: val = fmt->max_pbuffer_pixels; break;
    case WGL_MAX_PBUFFER_WIDTH_ARB: val = fmt->max_pbuffer_width; break;
    case WGL_MAX_PBUFFER_HEIGHT_ARB: val = fmt->max_pbuffer_height; break;
    case WGL_TRANSPARENT_RED_VALUE_ARB:
        val = fmt->transparent_red_value;
        valid = !!fmt->transparent_red_value_valid;
        break;
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
        val = fmt->transparent_green_value;
        valid = !!fmt->transparent_green_value_valid;
        break;
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
        val = fmt->transparent_blue_value;
        valid = !!fmt->transparent_blue_value_valid;
        break;
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
        val = fmt->transparent_alpha_value;
        valid = !!fmt->transparent_alpha_value_valid;
        break;
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
        val = fmt->transparent_index_value;
        valid = !!fmt->transparent_index_value_valid;
        break;
    case WGL_SAMPLE_BUFFERS_ARB: val = fmt->sample_buffers; break;
    case WGL_SAMPLES_ARB: val = fmt->samples; break;
    case WGL_BIND_TO_TEXTURE_RGB_ARB: val = fmt->bind_to_texture_rgb; break;
    case WGL_BIND_TO_TEXTURE_RGBA_ARB: val = fmt->bind_to_texture_rgba; break;
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV: val = fmt->bind_to_texture_rectangle_rgb; break;
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV: val = fmt->bind_to_texture_rectangle_rgba; break;
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB: val = fmt->framebuffer_srgb_capable; break;
    case WGL_FLOAT_COMPONENTS_NV: val = fmt->float_components; break;
    default:
        FIXME( "unsupported 0x%x WGL attribute\n", attrib );
        valid = 0;
        break;
    }

    /* If we haven't already determined validity, use the default check */
    if (valid == -1) valid = val != -1;
    if (valid) *value = val;

    return valid;
}

enum attrib_match
{
    ATTRIB_MATCH_INVALID = -1,
    ATTRIB_MATCH_IGNORE,
    ATTRIB_MATCH_EXACT,
    ATTRIB_MATCH_MINIMUM,
};

static enum attrib_match wgl_attrib_match_criteria( int attrib )
{
    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB:
    case WGL_DRAW_TO_BITMAP_ARB:
    case WGL_ACCELERATION_ARB:
    case WGL_NEED_PALETTE_ARB:
    case WGL_NEED_SYSTEM_PALETTE_ARB:
    case WGL_SWAP_LAYER_BUFFERS_ARB:
    case WGL_SHARE_DEPTH_ARB:
    case WGL_SHARE_STENCIL_ARB:
    case WGL_SHARE_ACCUM_ARB:
    case WGL_SUPPORT_GDI_ARB:
    case WGL_SUPPORT_OPENGL_ARB:
    case WGL_DOUBLE_BUFFER_ARB:
    case WGL_STEREO_ARB:
    case WGL_PIXEL_TYPE_ARB:
    case WGL_DRAW_TO_PBUFFER_ARB:
    case WGL_BIND_TO_TEXTURE_RGB_ARB:
    case WGL_BIND_TO_TEXTURE_RGBA_ARB:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV:
    case WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV:
    case WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB:
    case WGL_FLOAT_COMPONENTS_NV:
        return ATTRIB_MATCH_EXACT;
    case WGL_NUMBER_OVERLAYS_ARB:
    case WGL_NUMBER_UNDERLAYS_ARB:
    case WGL_COLOR_BITS_ARB:
    case WGL_RED_BITS_ARB:
    case WGL_GREEN_BITS_ARB:
    case WGL_BLUE_BITS_ARB:
    case WGL_ALPHA_BITS_ARB:
    case WGL_ACCUM_BITS_ARB:
    case WGL_ACCUM_RED_BITS_ARB:
    case WGL_ACCUM_GREEN_BITS_ARB:
    case WGL_ACCUM_BLUE_BITS_ARB:
    case WGL_ACCUM_ALPHA_BITS_ARB:
    case WGL_DEPTH_BITS_ARB:
    case WGL_STENCIL_BITS_ARB:
    case WGL_AUX_BUFFERS_ARB:
    case WGL_SAMPLE_BUFFERS_ARB:
    case WGL_SAMPLES_ARB:
        return ATTRIB_MATCH_MINIMUM;
    case WGL_NUMBER_PIXEL_FORMATS_ARB:
    case WGL_RED_SHIFT_ARB:
    case WGL_GREEN_SHIFT_ARB:
    case WGL_BLUE_SHIFT_ARB:
    case WGL_ALPHA_SHIFT_ARB:
    case WGL_TRANSPARENT_ARB:
    case WGL_TRANSPARENT_RED_VALUE_ARB:
    case WGL_TRANSPARENT_GREEN_VALUE_ARB:
    case WGL_TRANSPARENT_BLUE_VALUE_ARB:
    case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
    case WGL_TRANSPARENT_INDEX_VALUE_ARB:
    case WGL_SWAP_METHOD_ARB:
        return ATTRIB_MATCH_IGNORE;
    default:
        return ATTRIB_MATCH_INVALID;
    }
}

static void filter_format_array( const struct wgl_pixel_format **array,
                                 UINT num_formats, int attrib, int value )
{
    enum attrib_match match = wgl_attrib_match_criteria( attrib );
    int fmt_value;
    UINT i;

    assert(match != ATTRIB_MATCH_INVALID);

    if (match == ATTRIB_MATCH_IGNORE && attrib != WGL_SWAP_METHOD_ARB) return;

    for (i = 0; i < num_formats; ++i)
    {
        if (!array[i]) continue;
        if (!wgl_pixel_format_get_attrib( array[i], attrib, &fmt_value ) ||
            (match == ATTRIB_MATCH_EXACT && fmt_value != value) ||
            (match == ATTRIB_MATCH_MINIMUM && fmt_value < value) ||
            (attrib == WGL_SWAP_METHOD_ARB && ((fmt_value == WGL_SWAP_COPY_ARB) ^ (value == WGL_SWAP_COPY_ARB))))
        {
            array[i] = NULL;
        }
    }
}

static int wgl_attrib_sort_priority( int attrib )
{
    switch (attrib)
    {
    case WGL_DRAW_TO_WINDOW_ARB: return 1;
    case WGL_DRAW_TO_BITMAP_ARB: return 2;
    case WGL_ACCELERATION_ARB: return 3;
    case WGL_COLOR_BITS_ARB: return 4;
    case WGL_ACCUM_BITS_ARB: return 5;
    case WGL_PIXEL_TYPE_ARB: return 6;
    case WGL_ALPHA_BITS_ARB: return 7;
    case WGL_AUX_BUFFERS_ARB: return 8;
    case WGL_DEPTH_BITS_ARB: return 9;
    case WGL_STENCIL_BITS_ARB: return 10;
    case WGL_DOUBLE_BUFFER_ARB: return 11;
    case WGL_SWAP_METHOD_ARB: return 12;
    default: return 100;
    }
}

static int compare_attribs( const void *a, const void *b )
{
    return wgl_attrib_sort_priority( *(int *)a ) - wgl_attrib_sort_priority( *(int *)b );
}

static int wgl_attrib_value_priority( int value )
{
    switch (value)
    {
    case WGL_SWAP_UNDEFINED_ARB: return 1;
    case WGL_SWAP_EXCHANGE_ARB: return 2;
    case WGL_SWAP_COPY_ARB: return 3;

    case WGL_FULL_ACCELERATION_ARB: return 1;
    case WGL_GENERIC_ACCELERATION_ARB: return 2;
    case WGL_NO_ACCELERATION_ARB: return 3;

    case WGL_TYPE_RGBA_ARB: return 1;
    case WGL_TYPE_RGBA_FLOAT_ATI: return 2;
    case WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT: return 3;
    case WGL_TYPE_COLORINDEX_ARB: return 4;

    default: return 100;
    }
}

struct compare_formats_ctx
{
    int attribs[256];
    UINT num_attribs;
};

static int compare_formats( void *arg, const void *a, const void *b )
{
    const struct wgl_pixel_format *fmt_a = *(void **)a, *fmt_b = *(void **)b;
    struct compare_formats_ctx *ctx = arg;
    int attrib, val_a, val_b;
    UINT i;

    if (!fmt_a) return 1;
    if (!fmt_b) return -1;

    for (i = 0; i < ctx->num_attribs; ++i)
    {
        attrib = ctx->attribs[2 * i];
        if (wgl_pixel_format_get_attrib( fmt_a, attrib, &val_a ) &&
            wgl_pixel_format_get_attrib( fmt_b, attrib, &val_b ) &&
            val_a != val_b)
        {
            switch (attrib)
            {
            case WGL_ACCELERATION_ARB:
            case WGL_SWAP_METHOD_ARB:
            case WGL_PIXEL_TYPE_ARB:
                return wgl_attrib_value_priority( val_a ) -
                       wgl_attrib_value_priority( val_b );
            case WGL_COLOR_BITS_ARB:
                /* Prefer 32bpp over other values */
                if (val_a >= 32 && val_b >= 32) return val_a - val_b;
                else return val_b - val_a;
            default:
                /* Smaller values first */
                return val_a - val_b;
            }
        }
    }

    /* Maintain pixel format id order */
    return fmt_a - fmt_b;
}

static void compare_formats_ctx_set_attrib( struct compare_formats_ctx *ctx,
                                            int attrib, int value )
{
    UINT i;

    /* Overwrite attribute if it exists already */
    for (i = 0; i < ctx->num_attribs; ++i)
        if (ctx->attribs[2 * i] == attrib) break;

    assert(i < ARRAY_SIZE(ctx->attribs) / 2);

    ctx->attribs[2 * i] = attrib;
    ctx->attribs[2 * i + 1] = value;
    if (i == ctx->num_attribs) ++ctx->num_attribs;
}

/***********************************************************************
 *		wglChoosePixelFormatARB (OPENGL32.@)
 */
BOOL WINAPI wglChoosePixelFormatARB( HDC hdc, const int *attribs_int, const FLOAT *attribs_float,
                                     UINT max_formats, int *formats, UINT *num_formats )
{
    struct wgl_pixel_format *wgl_formats;
    UINT i, num_wgl_formats, num_wgl_onscreen_formats;
    const struct wgl_pixel_format **format_array;
    struct compare_formats_ctx ctx = { 0 };
    DWORD is_memdc;

    TRACE( "hdc %p, attribs_int %p, attribs_float %p, max_formats %u, formats %p, num_formats %p\n",
           hdc, attribs_int, attribs_float, max_formats, formats, num_formats );

    if (!NtGdiGetDCDword( hdc, NtGdiIsMemDC, &is_memdc )) is_memdc = 0;

    wgl_formats = get_pixel_formats( hdc, &num_wgl_formats, &num_wgl_onscreen_formats );

    /* Gather, validate and deduplicate all attributes */
    for (i = 0; attribs_int && attribs_int[i]; i += 2)
    {
        if (wgl_attrib_match_criteria( attribs_int[i] ) == ATTRIB_MATCH_INVALID) return FALSE;
        compare_formats_ctx_set_attrib( &ctx, attribs_int[i], attribs_int[i + 1] );
    }
    for (i = 0; attribs_float && attribs_float[i]; i += 2)
    {
        if (wgl_attrib_match_criteria( attribs_float[i] ) == ATTRIB_MATCH_INVALID) return FALSE;
        compare_formats_ctx_set_attrib( &ctx, attribs_float[i], attribs_float[i + 1] );
    }

    /* Initialize the format_array with (pointers to) all wgl formats */
    format_array = malloc( num_wgl_formats * sizeof(*format_array) );
    if (!format_array) return FALSE;
    for (i = 0; i < num_wgl_formats; ++i)
    {
        struct wgl_pixel_format *format = wgl_formats + i;
        if (is_memdc && format->pfd.cColorBits == 16 && format->pfd.cAlphaBits != 1) format_array[i] = NULL;
        else format_array[i] = &wgl_formats[i];
    }

    /* Remove formats that are not acceptable */
    for (i = 0; i < ctx.num_attribs; ++i)
        filter_format_array( format_array, num_wgl_formats, ctx.attribs[2 * i],
                             ctx.attribs[2 * i + 1] );

    /* Some attributes we always want to sort by (values don't matter for sorting) */
    compare_formats_ctx_set_attrib( &ctx, WGL_ACCELERATION_ARB, 0 );
    compare_formats_ctx_set_attrib( &ctx, WGL_COLOR_BITS_ARB, 0 );
    compare_formats_ctx_set_attrib( &ctx, WGL_ACCUM_BITS_ARB, 0 );

    /* Arrange attributes in the order which we want to check them */
    qsort( ctx.attribs, ctx.num_attribs, 2 * sizeof(*ctx.attribs), compare_attribs );

    /* Sort pixel formats based on the specified attributes */
    qsort_s( format_array, num_wgl_formats, sizeof(*format_array), compare_formats, &ctx );

    /* Return the best max_formats format ids */
    *num_formats = 0;
    for (i = 0; i < num_wgl_formats && i < max_formats && format_array[i]; ++i)
    {
        ++*num_formats;
        formats[i] = format_array[i] - wgl_formats + 1;
    }

    free( format_array );
    return TRUE;
}

INT WINAPI wglDescribePixelFormat( HDC hdc, int index, UINT size, PIXELFORMATDESCRIPTOR *ppfd )
{
    struct wgl_pixel_format *formats;
    UINT num_formats, num_onscreen_formats;

    TRACE( "hdc %p, index %d, size %u, ppfd %p\n", hdc, index, size, ppfd );

    if (!(formats = get_pixel_formats( hdc, &num_formats, &num_onscreen_formats ))) return 0;
    if (!ppfd) return num_onscreen_formats;
    if (size < sizeof(*ppfd)) return 0;
    if (index <= 0 || index > num_onscreen_formats) return 0;

    *ppfd = formats[index - 1].pfd;

    return num_onscreen_formats;
}

/***********************************************************************
 *		wglGetPixelFormatAttribivARB (OPENGL32.@)
 */
BOOL WINAPI wglGetPixelFormatAttribivARB( HDC hdc, int index, int plane, UINT count,
                                          const int *attributes, int *values )
{
    static const DWORD invalid_data_error = 0xC007000D;
    struct wgl_pixel_format *formats;
    UINT i, num_formats, num_onscreen_formats;

    TRACE( "hdc %p, index %d, plane %d, count %u, attributes %p, values %p\n",
           hdc, index, plane, count, attributes, values );

    formats = get_pixel_formats( hdc, &num_formats, &num_onscreen_formats );

    if (!count) return TRUE;
    if (count == 1 && attributes[0] == WGL_NUMBER_PIXEL_FORMATS_ARB)
    {
        values[0] = num_formats;
        return TRUE;
    }
    if (index <= 0 || index > num_formats)
    {
        SetLastError( invalid_data_error );
        return FALSE;
    }

    for (i = 0; i < count; ++i)
    {
        int attrib = attributes[i];

        if (attrib == WGL_NUMBER_PIXEL_FORMATS_ARB)
        {
            values[i] = num_formats;
        }
        else if ((plane != 0 && wgl_attrib_uses_layer( attrib )) ||
                 !wgl_pixel_format_get_attrib( &formats[index - 1], attrib, &values[i] ))
        {
            SetLastError( invalid_data_error );
            return FALSE;
        }
    }

    return TRUE;
}

/***********************************************************************
 *		wglGetPixelFormatAttribfvARB (OPENGL32.@)
 */
BOOL WINAPI wglGetPixelFormatAttribfvARB( HDC hdc, int index, int plane, UINT count,
                                          const int *attributes, FLOAT *values )
{
    int *ivalues;
    BOOL ret;
    UINT i;

    TRACE( "hdc %p, index %d, plane %d, count %u, attributes %p, values %p\n",
           hdc, index, plane, count, attributes, values );

    if (!(ivalues = malloc( count * sizeof(int) ))) return FALSE;

    /* For now we can piggy-back on wglGetPixelFormatAttribivARB, since we don't support
     * any non-integer attributes. */
    ret = wglGetPixelFormatAttribivARB( hdc, index, plane, count, attributes, ivalues );
    if (ret)
    {
        for (i = 0; i < count; i++)
            values[i] = ivalues[i];
    }

    free( ivalues );
    return ret;
}

/***********************************************************************
 *		wglGetPixelFormat (OPENGL32.@)
 */
INT WINAPI wglGetPixelFormat(HDC hdc)
{
    struct wglGetPixelFormat_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    NTSTATUS status;

    TRACE( "hdc %p\n", hdc );

    if ((status = UNIX_CALL( wglGetPixelFormat, &args )))
    {
        WARN( "wglGetPixelFormat returned %#lx\n", status );
        SetLastError( ERROR_INVALID_PIXEL_FORMAT );
    }

    return args.ret;
}

/***********************************************************************
 *              wglSwapBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapBuffers( HDC hdc )
{
    struct wglSwapBuffers_params args = { .teb = NtCurrentTeb(), .hdc = hdc };
    NTSTATUS status;

    if ((status = UNIX_CALL( wglSwapBuffers, &args ))) WARN( "wglSwapBuffers returned %#lx\n", status );
    else if (TRACE_ON(fps))
    {
        static long prev_time, start_time;
        static unsigned long frames, frames_total;

        DWORD time = GetTickCount();
        frames++;
        frames_total++;
        /* every 1.5 seconds */
        if (time - prev_time > 1500)
        {
            TRACE_(fps)("@ approx %.2ffps, total %.2ffps\n",
                        1000.0*frames/(time - prev_time), 1000.0*frames_total/(time - start_time));
            prev_time = time;
            frames = 0;
            if (start_time == 0) start_time = time;
        }
    }

    return args.ret;
}

/***********************************************************************
 *		wglCreateLayerContext (OPENGL32.@)
 */
HGLRC WINAPI wglCreateLayerContext( HDC hdc, int iLayerPlane )
{
    TRACE("(%p,%d)\n", hdc, iLayerPlane);

    if (iLayerPlane == 0) return wglCreateContext( hdc );

    FIXME("no handler for layer %d\n", iLayerPlane);
    return NULL;
}

/***********************************************************************
 *		wglDescribeLayerPlane (OPENGL32.@)
 */
BOOL WINAPI wglDescribeLayerPlane(HDC hdc,
				  int iPixelFormat,
				  int iLayerPlane,
				  UINT nBytes,
				  LPLAYERPLANEDESCRIPTOR plpd) {
  FIXME("(%p,%d,%d,%d,%p)\n", hdc, iPixelFormat, iLayerPlane, nBytes, plpd);

  return FALSE;
}

/***********************************************************************
 *		wglGetLayerPaletteEntries (OPENGL32.@)
 */
int WINAPI wglGetLayerPaletteEntries( HDC hdc, int plane, int start, int count, COLORREF *colors )
{
    FIXME( "hdc %p, plane %d, start %d, count %d, colors %p, stub!\n", hdc, plane, start, count, colors );
    return 0;
}

/***********************************************************************
 *		wglGetProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetProcAddress( LPCSTR name )
{
    const struct registry_entry *func;
    const enum opengl_extension *ext;
    struct context *ctx;

    if (!(ctx = get_current_context())) return NULL;

    if (!(func = get_function_entry( name )))
    {
        WARN( "Function %s unknown\n", name );
        return NULL;
    }

    if (!strncmp( name, "wglGetExtensionsString", 22 ))
    {
        EnterCriticalSection( &wgl_cs );
        if (!wgl_extensions) init_wgl_extensions( ctx->base.extensions );
        LeaveCriticalSection( &wgl_cs );
    }

    if (func->major && (ctx->base.major_version > func->major
                        || (ctx->base.major_version == func->major && ctx->base.minor_version >= func->minor)))
        return func->func;

    for (ext = func->extensions; *ext != GL_EXTENSION_COUNT; ext++)
    {
        if (ctx->base.extensions[*ext]) return func->func;
    }

    WARN( "Extensions required for %s not supported\n", name );
    return NULL;
}

/***********************************************************************
 *		wglRealizeLayerPalette (OPENGL32.@)
 */
BOOL WINAPI wglRealizeLayerPalette(HDC hdc,
				   int iLayerPlane,
				   BOOL bRealize) {
  FIXME("()\n");

  return FALSE;
}

/***********************************************************************
 *		wglSetLayerPaletteEntries (OPENGL32.@)
 */
int WINAPI wglSetLayerPaletteEntries(HDC hdc,
				     int iLayerPlane,
				     int iStart,
				     int cEntries,
				     const COLORREF *pcr) {
  FIXME("(): stub!\n");

  return 0;
}

/***********************************************************************
 *		wglGetDefaultProcAddress (OPENGL32.@)
 */
PROC WINAPI wglGetDefaultProcAddress( LPCSTR name )
{
    FIXME( "%s: stub\n", debugstr_a(name));
    return NULL;
}

/***********************************************************************
 *		wglSwapLayerBuffers (OPENGL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH wglSwapLayerBuffers(HDC hdc, UINT fuPlanes)
{
  TRACE("(%p, %08x)\n", hdc, fuPlanes);

  if (fuPlanes & WGL_SWAP_MAIN_PLANE) {
    if (!wglSwapBuffers( hdc )) return FALSE;
    fuPlanes &= ~WGL_SWAP_MAIN_PLANE;
  }

  if (fuPlanes) {
    WARN("Following layers unhandled: %08x\n", fuPlanes);
  }

  return TRUE;
}

/***********************************************************************
 *		wglUseFontBitmaps_common
 */
static BOOL wglUseFontBitmaps_common( HDC hdc, DWORD first, DWORD count, DWORD listBase, BOOL unicode )
{
     GLYPHMETRICS gm;
     unsigned int glyph, size = 0;
     void *bitmap = NULL, *gl_bitmap = NULL;
     int org_alignment;
     BOOL ret = TRUE;

     glGetIntegerv( GL_UNPACK_ALIGNMENT, &org_alignment );
     glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );

     for (glyph = first; glyph < first + count; glyph++) {
         unsigned int needed_size, height, width, width_int;

         if (unicode)
             needed_size = GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);
         else
             needed_size = GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm, 0, NULL, &identity);

         TRACE("Glyph: %3d / List: %ld size %d\n", glyph, listBase, needed_size);
         if (needed_size == GDI_ERROR) {
             ret = FALSE;
             break;
         }

         if (needed_size > size) {
             size = needed_size;
             free( bitmap );
             free( gl_bitmap );
             bitmap = calloc( 1, size );
             gl_bitmap = calloc( 1, size );
         }
         if (needed_size != 0) {
             if (unicode)
                 ret = (GetGlyphOutlineW(hdc, glyph, GGO_BITMAP, &gm,
                                         size, bitmap, &identity) != GDI_ERROR);
             else
                 ret = (GetGlyphOutlineA(hdc, glyph, GGO_BITMAP, &gm,
                                         size, bitmap, &identity) != GDI_ERROR);
             if (!ret) break;
         }

         if (TRACE_ON(opengl))
         {
             unsigned int bitmask;
             unsigned char *bitmap_ = bitmap;

             TRACE("  - bbox: %d x %d\n", gm.gmBlackBoxX, gm.gmBlackBoxY);
             TRACE("  - origin: (%ld, %ld)\n", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y);
             TRACE("  - increment: %d - %d\n", gm.gmCellIncX, gm.gmCellIncY);
             if (needed_size != 0) {
                 TRACE("  - bitmap:\n");
                 for (height = 0; height < gm.gmBlackBoxY; height++) {
                     TRACE("      ");
                     for (width = 0, bitmask = 0x80; width < gm.gmBlackBoxX; width++, bitmask >>= 1) {
                         if (bitmask == 0) {
                             bitmap_ += 1;
                             bitmask = 0x80;
                         }
                         if (*bitmap_ & bitmask)
                             TRACE("*");
                         else
                             TRACE(" ");
                     }
                     bitmap_ += (4 - ((UINT_PTR)bitmap_ & 0x03));
                     TRACE("\n");
                 }
             }
         }

         /* In OpenGL, the bitmap is drawn from the bottom to the top... So we need to invert the
         * glyph for it to be drawn properly.
         */
         if (needed_size != 0) {
             width_int = (gm.gmBlackBoxX + 31) / 32;
             for (height = 0; height < gm.gmBlackBoxY; height++) {
                 for (width = 0; width < width_int; width++) {
                     ((int *) gl_bitmap)[(gm.gmBlackBoxY - height - 1) * width_int + width] =
                     ((int *) bitmap)[height * width_int + width];
                 }
             }
         }

         glNewList( listBase++, GL_COMPILE );
         if (needed_size != 0) {
             glBitmap( gm.gmBlackBoxX, gm.gmBlackBoxY, 0 - gm.gmptGlyphOrigin.x,
                       (int)gm.gmBlackBoxY - gm.gmptGlyphOrigin.y, gm.gmCellIncX, gm.gmCellIncY, gl_bitmap );
         } else {
             /* This is the case of 'empty' glyphs like the space character */
             glBitmap( 0, 0, 0, 0, gm.gmCellIncX, gm.gmCellIncY, NULL );
         }
         glEndList();
     }

     glPixelStorei( GL_UNPACK_ALIGNMENT, org_alignment );
     free( bitmap );
     free( gl_bitmap );
     return ret;
}

/***********************************************************************
 *		wglUseFontBitmapsA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, FALSE );
}

/***********************************************************************
 *		wglUseFontBitmapsW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglUseFontBitmaps_common( hdc, first, count, listBase, TRUE );
}

static void fixed_to_double(POINTFX fixed, UINT em_size, GLdouble vertex[3])
{
    vertex[0] = (fixed.x.value + (GLdouble)fixed.x.fract / (1 << 16)) / em_size;
    vertex[1] = (fixed.y.value + (GLdouble)fixed.y.fract / (1 << 16)) / em_size;
    vertex[2] = 0.0;
}

typedef struct _bezier_vector {
    GLdouble x;
    GLdouble y;
} bezier_vector;

static BOOL bezier_fits_deviation(const bezier_vector *p, FLOAT max_deviation)
{
    bezier_vector deviation;
    bezier_vector vertex;
    bezier_vector base;
    double base_length;
    double dot;

    max_deviation *= max_deviation;

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4 - p[0].x;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4 - p[0].y;

    base.x = p[2].x - p[0].x;
    base.y = p[2].y - p[0].y;

    base_length = base.x * base.x + base.y * base.y;
    if (base_length <= max_deviation)
    {
        base.x = 0.0;
        base.y = 0.0;
    }
    else
    {
        base_length = sqrt(base_length);
        base.x /= base_length;
        base.y /= base_length;

        dot = base.x*vertex.x + base.y*vertex.y;
        dot = min(max(dot, 0.0), base_length);
        base.x *= dot;
        base.y *= dot;
    }

    deviation.x = vertex.x-base.x;
    deviation.y = vertex.y-base.y;

    return deviation.x*deviation.x + deviation.y*deviation.y <= max_deviation;
}

static int bezier_approximate(const bezier_vector *p, bezier_vector *points, FLOAT deviation)
{
    bezier_vector first_curve[3];
    bezier_vector second_curve[3];
    bezier_vector vertex;
    int total_vertices;

    if (bezier_fits_deviation(p, deviation))
    {
        if(points)
            *points = p[2];
        return 1;
    }

    vertex.x = (p[0].x + p[1].x*2 + p[2].x)/4;
    vertex.y = (p[0].y + p[1].y*2 + p[2].y)/4;

    first_curve[0] = p[0];
    first_curve[1].x = (p[0].x + p[1].x)/2;
    first_curve[1].y = (p[0].y + p[1].y)/2;
    first_curve[2] = vertex;

    second_curve[0] = vertex;
    second_curve[1].x = (p[2].x + p[1].x)/2;
    second_curve[1].y = (p[2].y + p[1].y)/2;
    second_curve[2] = p[2];

    total_vertices = bezier_approximate(first_curve, points, deviation);
    if(points)
        points += total_vertices;
    total_vertices += bezier_approximate(second_curve, points, deviation);
    return total_vertices;
}

/***********************************************************************
 *		wglUseFontOutlines_common
 */
static BOOL wglUseFontOutlines_common(HDC hdc,
                                      DWORD first,
                                      DWORD count,
                                      DWORD listBase,
                                      FLOAT deviation,
                                      FLOAT extrusion,
                                      int format,
                                      LPGLYPHMETRICSFLOAT lpgmf,
                                      BOOL unicode)
{
    UINT glyph;
    GLUtesselator *tess = NULL;
    LOGFONTW lf;
    HFONT old_font, unscaled_font;
    UINT em_size = 1024;
    RECT rc;

    TRACE("(%p, %ld, %ld, %ld, %f, %f, %d, %p, %s)\n", hdc, first, count,
          listBase, deviation, extrusion, format, lpgmf, unicode ? "W" : "A");

    if(deviation <= 0.0)
        deviation = 1.0/em_size;

    if(format == WGL_FONT_POLYGONS)
    {
        tess = gluNewTess();
        if(!tess)
        {
            ERR("glu32 is required for this function but isn't available\n");
            return FALSE;
        }
        gluTessCallback( tess, GLU_TESS_VERTEX, (void *)glVertex3dv );
        gluTessCallback( tess, GLU_TESS_BEGIN, (void *)glBegin );
        gluTessCallback( tess, GLU_TESS_END, glEnd );
    }

    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);
    rc.left = rc.right = rc.bottom = 0;
    rc.top = em_size;
    DPtoLP(hdc, (POINT*)&rc, 2);
    lf.lfHeight = -abs(rc.top - rc.bottom);
    lf.lfOrientation = lf.lfEscapement = 0;
    unscaled_font = CreateFontIndirectW(&lf);
    old_font = SelectObject(hdc, unscaled_font);

    for (glyph = first; glyph < first + count; glyph++)
    {
        DWORD needed;
        GLYPHMETRICS gm;
        BYTE *buf;
        TTPOLYGONHEADER *pph;
        TTPOLYCURVE *ppc;
        GLdouble *vertices = NULL;
        int vertex_total = -1;

        if(unicode)
            needed = GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);
        else
            needed = GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, 0, NULL, &identity);

        if(needed == GDI_ERROR)
            goto error;

        buf = malloc( needed );

        if(unicode)
            GetGlyphOutlineW(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);
        else
            GetGlyphOutlineA(hdc, glyph, GGO_NATIVE, &gm, needed, buf, &identity);

        TRACE("glyph %d\n", glyph);

        if(lpgmf)
        {
            lpgmf->gmfBlackBoxX = (float)gm.gmBlackBoxX / em_size;
            lpgmf->gmfBlackBoxY = (float)gm.gmBlackBoxY / em_size;
            lpgmf->gmfptGlyphOrigin.x = (float)gm.gmptGlyphOrigin.x / em_size;
            lpgmf->gmfptGlyphOrigin.y = (float)gm.gmptGlyphOrigin.y / em_size;
            lpgmf->gmfCellIncX = (float)gm.gmCellIncX / em_size;
            lpgmf->gmfCellIncY = (float)gm.gmCellIncY / em_size;

            TRACE("%fx%f at %f,%f inc %f,%f\n", lpgmf->gmfBlackBoxX, lpgmf->gmfBlackBoxY,
                  lpgmf->gmfptGlyphOrigin.x, lpgmf->gmfptGlyphOrigin.y, lpgmf->gmfCellIncX, lpgmf->gmfCellIncY);
            lpgmf++;
        }

        glNewList( listBase++, GL_COMPILE );
        glFrontFace( GL_CCW );
        if(format == WGL_FONT_POLYGONS)
        {
            glNormal3d( 0.0, 0.0, 1.0 );
            gluTessNormal(tess, 0, 0, 1);
            gluTessBeginPolygon(tess, NULL);
        }

        while(!vertices)
        {
            if (vertex_total != -1) vertices = malloc( vertex_total * 3 * sizeof(GLdouble) );
            vertex_total = 0;

            pph = (TTPOLYGONHEADER*)buf;
            while((BYTE*)pph < buf + needed)
            {
                GLdouble previous[3];
                fixed_to_double(pph->pfxStart, em_size, previous);

                if(vertices)
                    TRACE("\tstart %d, %d\n", pph->pfxStart.x.value, pph->pfxStart.y.value);

                if (format == WGL_FONT_POLYGONS) gluTessBeginContour( tess );
                else glBegin( GL_LINE_LOOP );

                if(vertices)
                {
                    fixed_to_double(pph->pfxStart, em_size, vertices);
                    if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                    else glVertex3d( vertices[0], vertices[1], vertices[2] );
                    vertices += 3;
                }
                vertex_total++;

                ppc = (TTPOLYCURVE*)((char*)pph + sizeof(*pph));
                while((char*)ppc < (char*)pph + pph->cb)
                {
                    int i, j;
                    int num;

                    switch(ppc->wType) {
                    case TT_PRIM_LINE:
                        for(i = 0; i < ppc->cpfx; i++)
                        {
                            if(vertices)
                            {
                                TRACE("\t\tline to %d, %d\n",
                                      ppc->apfx[i].x.value, ppc->apfx[i].y.value);
                                fixed_to_double(ppc->apfx[i], em_size, vertices);
                                if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                                else glVertex3d( vertices[0], vertices[1], vertices[2] );
                                vertices += 3;
                            }
                            fixed_to_double(ppc->apfx[i], em_size, previous);
                            vertex_total++;
                        }
                        break;

                    case TT_PRIM_QSPLINE:
                        for(i = 0; i < ppc->cpfx-1; i++)
                        {
                            bezier_vector curve[3];
                            bezier_vector *points;
                            GLdouble curve_vertex[3];

                            if(vertices)
                                TRACE("\t\tcurve  %d,%d %d,%d\n",
                                      ppc->apfx[i].x.value,     ppc->apfx[i].y.value,
                                      ppc->apfx[i + 1].x.value, ppc->apfx[i + 1].y.value);

                            curve[0].x = previous[0];
                            curve[0].y = previous[1];
                            fixed_to_double(ppc->apfx[i], em_size, curve_vertex);
                            curve[1].x = curve_vertex[0];
                            curve[1].y = curve_vertex[1];
                            fixed_to_double(ppc->apfx[i + 1], em_size, curve_vertex);
                            curve[2].x = curve_vertex[0];
                            curve[2].y = curve_vertex[1];
                            if(i < ppc->cpfx-2)
                            {
                                curve[2].x = (curve[1].x + curve[2].x)/2;
                                curve[2].y = (curve[1].y + curve[2].y)/2;
                            }
                            num = bezier_approximate(curve, NULL, deviation);
                            points = malloc( num * sizeof(bezier_vector) );
                            num = bezier_approximate(curve, points, deviation);
                            vertex_total += num;
                            if(vertices)
                            {
                                for(j=0; j<num; j++)
                                {
                                    TRACE("\t\t\tvertex at %f,%f\n", points[j].x, points[j].y);
                                    vertices[0] = points[j].x;
                                    vertices[1] = points[j].y;
                                    vertices[2] = 0.0;
                                    if (format == WGL_FONT_POLYGONS) gluTessVertex( tess, vertices, vertices );
                                    else glVertex3d( vertices[0], vertices[1], vertices[2] );
                                    vertices += 3;
                                }
                            }
                            free( points );
                            previous[0] = curve[2].x;
                            previous[1] = curve[2].y;
                        }
                        break;
                    default:
                        ERR("\t\tcurve type = %d\n", ppc->wType);
                        if (format == WGL_FONT_POLYGONS) gluTessEndContour( tess );
                        else glEnd();
                        goto error_in_list;
                    }

                    ppc = (TTPOLYCURVE*)((char*)ppc + sizeof(*ppc) +
                                         (ppc->cpfx - 1) * sizeof(POINTFX));
                }
                if (format == WGL_FONT_POLYGONS) gluTessEndContour( tess );
                else glEnd();
                pph = (TTPOLYGONHEADER*)((char*)pph + pph->cb);
            }
        }

error_in_list:
        if (format == WGL_FONT_POLYGONS) gluTessEndPolygon( tess );
        glTranslated( (GLdouble)gm.gmCellIncX / em_size, (GLdouble)gm.gmCellIncY / em_size, 0.0 );
        glEndList();
        free( buf );
        free( vertices );
    }

 error:
    DeleteObject(SelectObject(hdc, old_font));
    if(format == WGL_FONT_POLYGONS)
        gluDeleteTess(tess);
    return TRUE;

}

/***********************************************************************
 *		wglUseFontOutlinesA (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesA(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, FALSE);
}

/***********************************************************************
 *		wglUseFontOutlinesW (OPENGL32.@)
 */
BOOL WINAPI wglUseFontOutlinesW(HDC hdc,
				DWORD first,
				DWORD count,
				DWORD listBase,
				FLOAT deviation,
				FLOAT extrusion,
				int format,
				LPGLYPHMETRICSFLOAT lpgmf)
{
    return wglUseFontOutlines_common(hdc, first, count, listBase, deviation, extrusion, format, lpgmf, TRUE);
}

GLhandleARB WINAPI glGetHandleARB( GLenum pname )
{
    struct glGetHandleARB_params args = { .teb = NtCurrentTeb(), .pname = pname };
    GLuint *object, client_id = 0;
    struct object_table *table;
    struct context *ctx;
    NTSTATUS status;

    TRACE( "pname %d\n", pname );

    if ((status = UNIX_CALL( glGetHandleARB, &args ))) WARN( "glGetHandleARB returned %#lx\n", status );
    if (!args.ret) return args.ret;

    if (!(ctx = context_from_handle( args.teb->glCurrentRC ))) return 0;
    if (!(table = get_object_table( ctx, OBJ_TYPE_SHADER, FALSE ))) return 0;

    AcquireSRWLockShared( &table->lock );
    if ((object = find_object_id( table->client_ids, args.ret ))) client_id = *object;
    ReleaseSRWLockShared( &table->lock );

    return client_id;
}

void WINAPI glGetAttachedObjectsARB( GLhandleARB container, GLsizei max_count, GLsizei *count, GLhandleARB *obj )
{
    struct glGetAttachedObjectsARB_params args = { .teb = NtCurrentTeb(), .maxCount = max_count, .count = count };
    struct object_table *table;
    struct context *ctx;
    NTSTATUS status;
    GLuint *object;

    TRACE( "container %d, max_count %d, count %p, obj %p\n", container, max_count, count, obj );

    args.containerObj = *map_context_objects( OBJ_TYPE_SHADER, 1, &container );
    if ((status = UNIX_CALL( glGetAttachedObjectsARB, &args ))) WARN( "glGetAttachedObjectsARB returned %#lx\n", status );

    if (!(ctx = context_from_handle( args.teb->glCurrentRC ))) return;
    if (!(table = get_object_table( ctx, OBJ_TYPE_SHADER, FALSE ))) return;

    AcquireSRWLockShared( &table->lock );
    for (UINT i = 0; i < max_count; ++i)
    {
        if (!obj[i] || !(object = find_object_id( table->client_ids, obj[i] ))) continue;
        obj[i] = *object;
    }
    ReleaseSRWLockShared( &table->lock );
}

void WINAPI glGetAttachedShaders( GLuint program, GLsizei max_count, GLsizei *count, GLuint *shaders )
{
    struct glGetAttachedShaders_params args = { .teb = NtCurrentTeb(), .maxCount = max_count, .count = count };
    struct object_table *table;
    struct context *ctx;
    NTSTATUS status;
    GLuint *object;

    TRACE( "program %d, max_count %d, count %p, shaders %p\n", program, max_count, count, shaders );

    args.program = *map_context_objects( OBJ_TYPE_SHADER, 1, &program );
    if ((status = UNIX_CALL( glGetAttachedShaders, &args ))) WARN( "glGetAttachedShaders returned %#lx\n", status );

    if (!(ctx = context_from_handle( args.teb->glCurrentRC ))) return;
    if (!(table = get_object_table( ctx, OBJ_TYPE_SHADER, FALSE ))) return;

    AcquireSRWLockShared( &table->lock );
    for (UINT i = 0; i < max_count; ++i)
    {
        if (!shaders[i] || !(object = find_object_id( table->client_ids, shaders[i] ))) continue;
        shaders[i] = *object;
    }
    ReleaseSRWLockShared( &table->lock );
}

/***********************************************************************
 *              glDebugEntry (OPENGL32.@)
 */
GLint WINAPI glDebugEntry( GLint unknown1, GLint unknown2 )
{
    return 0;
}

static GLsync sync_from_handle( GLsync handle )
{
    struct handle_entry *ptr;
    struct context *ctx;

    if (!(ctx = get_current_context())) return NULL;
    if (!(ptr = get_handle_ptr( &ctx->lists->syncs, handle ))) return NULL;
    return ptr->user_data;
}

BOOL get_sync_from_handle( GLsync handle, GLsync *obj )
{
    *obj = sync_from_handle( handle );
    return *obj || !handle;
}

static struct handle_entry *alloc_client_sync( struct context *ctx )
{
    struct handle_entry *ptr;
    GLsync sync;

    if (!(sync = calloc( 1, sizeof(*sync) ))) return NULL;
    if (!(ptr = alloc_handle( &ctx->lists->syncs, sync ))) free( sync );
    else InterlockedExchange( &ctx->lists->modified, 1 );
    return ptr;
}

static void free_client_sync( struct context *ctx, struct handle_entry *ptr )
{
    GLsync sync = ptr->user_data;
    InterlockedExchange( &ctx->lists->modified, 1 );
    free_handle( &ctx->lists->syncs, ptr );
    free( sync );
}

GLsync WINAPI glCreateSyncFromCLeventARB( struct _cl_context *context, struct _cl_event *event, GLbitfield flags )
{
    TEB *teb = NtCurrentTeb();
    struct glCreateSyncFromCLeventARB_params args = { .teb = teb, .context = context, .event = event, .flags = flags };
    struct handle_entry *ptr;
    struct context *ctx;
    NTSTATUS status;

    TRACE( "context %p, event %p, flags %d\n", context, event, flags );

    if (!(ctx = context_from_handle( teb->glCurrentRC ))) return NULL;
    if (!(ptr = alloc_client_sync( ctx ))) return NULL;
    args.ret = ptr->user_data;

    if ((status = UNIX_CALL( glCreateSyncFromCLeventARB, &args ))) WARN( "glCreateSyncFromCLeventARB returned %#lx\n", status );
    assert( args.ret == ptr->user_data || !args.ret );

    if (!status && args.ret) return UlongToHandle( ptr->handle );
    free_client_sync( ctx, ptr );
    return NULL;
}

void WINAPI glDeleteSync( GLsync sync )
{
    TEB *teb = NtCurrentTeb();
    struct glDeleteSync_params args = { .teb = teb };
    struct handle_entry *ptr;
    struct context *ctx;
    NTSTATUS status;

    TRACE( "sync %p\n", sync );

    if (!(ctx = context_from_handle( teb->glCurrentRC ))) return;
    if (!(ptr = get_handle_ptr( &ctx->lists->syncs, sync ))) return set_gl_error( GL_INVALID_VALUE );
    args.sync = ptr->user_data;

    if ((status = UNIX_CALL( glDeleteSync, &args ))) WARN( "glDeleteSync returned %#lx\n", status );
    if (!status) free_client_sync( ctx, ptr );
}

GLsync WINAPI glFenceSync( GLenum condition, GLbitfield flags )
{
    TEB *teb = NtCurrentTeb();
    struct glFenceSync_params args = { .teb = teb, .condition = condition, .flags = flags };
    struct handle_entry *ptr;
    struct context *ctx;
    NTSTATUS status;

    TRACE( "condition %d, flags %d\n", condition, flags );

    if (!(ctx = context_from_handle( teb->glCurrentRC ))) return NULL;
    if (!(ptr = alloc_client_sync( ctx ))) return NULL;
    args.ret = ptr->user_data;

    if ((status = UNIX_CALL( glFenceSync, &args ))) WARN( "glFenceSync returned %#lx\n", status );
    assert( args.ret == ptr->user_data || !args.ret );

    if (!status && args.ret) return UlongToHandle( ptr->handle );
    free_client_sync( ctx, ptr );
    return NULL;
}

GLsync WINAPI glImportSyncEXT( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags )
{
    TEB *teb = NtCurrentTeb();
    struct glImportSyncEXT_params args = { .teb = teb, .external_sync_type = external_sync_type, .external_sync = external_sync, .flags = flags };
    struct handle_entry *ptr;
    struct context *ctx;
    NTSTATUS status;

    TRACE( "external_sync_type %d, external_sync %Id, flags %d\n", external_sync_type, external_sync, flags );

    if (!(ctx = context_from_handle( teb->glCurrentRC ))) return NULL;
    if (!(ptr = alloc_client_sync( ctx ))) return NULL;
    args.ret = ptr->user_data;

    if ((status = UNIX_CALL( glImportSyncEXT, &args ))) WARN( "glImportSyncEXT returned %#lx\n", status );
    assert( args.ret == ptr->user_data || !args.ret );

    if (!status && args.ret) return UlongToHandle( ptr->handle );
    free_client_sync( ctx, ptr );
    return NULL;
}

BOOL get_integer( GLenum name, GLuint index, GLint value, GLint *data )
{
    struct context *ctx;

    if (!(ctx = get_current_context())) return FALSE;

    switch (name)
    {
    case GL_CONTEXT_PROFILE_MASK:
        *data = ctx->base.profile_mask;
        return TRUE;
    case GL_MAJOR_VERSION:
        *data = ctx->base.major_version;
        return TRUE;
    case GL_MINOR_VERSION:
        *data = ctx->base.minor_version;
        return TRUE;
    case GL_NUM_EXTENSIONS:
        *data = ctx->base.extension_count;
        return TRUE;
    }

    return map_client_objects( get_pname_object_type( name ), value, (GLuint *)data );
}

const GLubyte * WINAPI glGetStringi( GLenum name, GLuint index )
{
    struct glGetStringi_params args =
    {
        .teb = NtCurrentTeb(),
        .name = name,
        .index = index,
    };
    NTSTATUS status;
    struct context *ctx;
#ifndef _WIN64
    GLubyte *wow64_str = NULL;
#endif

    TRACE( "name %d, index %d\n", name, index );

    if (!(ctx = get_current_context())) return NULL;

    switch (name)
    {
    case GL_EXTENSIONS:
        if (index < ctx->base.extension_count)
            return (const GLubyte *)extension_names[ctx->base.extension_array[index]];
        set_gl_error( GL_INVALID_VALUE );
        return NULL;
    }

#ifndef _WIN64
    if (UNIX_CALL( glGetStringi, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( glGetStringi, &args ))) WARN( "glGetStringi returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( (char *)args.ret );
#endif
    return args.ret;
}

/***********************************************************************
 *              glGetString (OPENGL32.@)
 */
const GLubyte * WINAPI glGetString( GLenum name )
{
    struct glGetString_params args = { .teb = NtCurrentTeb(), .name = name };
    NTSTATUS status;
#ifndef _WIN64
    GLubyte *wow64_str = NULL;
#endif

    TRACE( "name %d\n", name );

#ifndef _WIN64
    if (UNIX_CALL( glGetString, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( glGetString, &args ))) WARN( "glGetString returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( (char *)args.ret );
#endif
    return args.ret;
}

const char * WINAPI wglGetExtensionsStringARB( HDC hdc )
{
    TRACE( "hdc %p\n", hdc );
    return wgl_extensions;
}

const char * WINAPI wglGetExtensionsStringEXT(void)
{
    TRACE( "\n" );
    return wgl_extensions;
}

const GLchar * WINAPI wglQueryCurrentRendererStringWINE( GLenum attribute )
{
    struct wglQueryCurrentRendererStringWINE_params args = { .teb = NtCurrentTeb(), .attribute = attribute };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "attribute %d\n", attribute );

#ifndef _WIN64
    if (UNIX_CALL( wglQueryCurrentRendererStringWINE, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglQueryCurrentRendererStringWINE, &args ))) WARN( "wglQueryCurrentRendererStringWINE returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

const GLchar * WINAPI wglQueryRendererStringWINE( HDC dc, GLint renderer, GLenum attribute )
{
    struct wglQueryRendererStringWINE_params args =
    {
        .teb = NtCurrentTeb(),
        .dc = dc,
        .renderer = renderer,
        .attribute = attribute,
    };
    NTSTATUS status;
#ifndef _WIN64
    char *wow64_str = NULL;
#endif

    TRACE( "dc %p, renderer %d, attribute %d\n", dc, renderer, attribute );

#ifndef _WIN64
    if (UNIX_CALL( wglQueryRendererStringWINE, &args ) == STATUS_BUFFER_TOO_SMALL) args.ret = wow64_str = malloc( (size_t)args.ret );
#endif
    if ((status = UNIX_CALL( wglQueryRendererStringWINE, &args ))) WARN( "wglQueryRendererStringWINE returned %#lx\n", status );
#ifndef _WIN64
    if (args.ret != wow64_str) free( wow64_str );
    else if (args.ret) append_wow64_string( wow64_str );
#endif
    return args.ret;
}

typedef void (WINAPI *gl_debug_message)(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void *);

static NTSTATUS WINAPI call_gl_debug_message_callback( void *args, ULONG size )
{
    struct gl_debug_message_callback_params *params = args;
    gl_debug_message callback = (void *)(UINT_PTR)params->debug_callback;
    const void *user = (void *)(UINT_PTR)params->debug_user;
    callback( params->source, params->type, params->id, params->severity,
              params->length, params->message, user );
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           OpenGL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    struct process_attach_params params =
    {
        .call_gl_debug_message_callback = (UINT_PTR)call_gl_debug_message_callback,
    };
    struct context *context;
    NTSTATUS status;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if ((status = __wine_init_unix_call()) ||
            (status = UNIX_CALL( process_attach, &params )))
        {
            ERR( "Failed to load unixlib, status %#lx\n", status );
            return FALSE;
        }

        /* fallthrough */
    case DLL_THREAD_ATTACH:
        if ((status = UNIX_CALL( thread_attach, NtCurrentTeb() )))
        {
            WARN( "Failed to initialize thread, status %#lx\n", status );
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        UNIX_CALL( process_detach, NULL );
#ifndef _WIN64
        cleanup_wow64_strings();
#endif
        /* fallthrough */
    case DLL_THREAD_DETACH:
        if ((context = get_current_context())) context->base.current_tid = 0;
        free( NtCurrentTeb()->glReserved1[WINE_GL_RESERVED_FORMATS_PTR] );
        return TRUE;
    }
    return TRUE;
}
