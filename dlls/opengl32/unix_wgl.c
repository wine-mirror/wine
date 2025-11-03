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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ntuser.h"

#include "unixlib.h"
#include "unix_private.h"

#include "wine/debug.h"
#include "wine/rbtree.h"
#include "wine/vulkan_driver.h"

WINE_DEFAULT_DEBUG_CHANNEL(opengl);

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static BOOL is_wow64(void)
{
    return !!NtCurrentTeb()->WowTebOffset;
}

static UINT64 call_gl_debug_message_callback;
pthread_mutex_t wgl_lock = PTHREAD_MUTEX_INITIALIZER;

/* handle management */

enum wgl_handle_type
{
    HANDLE_PBUFFER = 0 << 12,
    HANDLE_CONTEXT = 1 << 12,
    HANDLE_GLSYNC = 2 << 12,
    HANDLE_TYPE_MASK = 15 << 12,
};

/* context state management */

struct pixel_mode_state
{
    GLenum read_buffer;
};

struct light_model_state
{
    GLfloat ambient[4];
    GLint two_side;
};

struct lighting_state
{
    struct light_model_state model;
    GLenum shade_model;
};

struct depth_buffer_state
{
    GLenum depth_func;
};

struct viewport_state
{
    GLint x;
    GLint y;
    GLsizei w;
    GLsizei h;
};

struct enable_state
{
    GLboolean cull_face;
    GLboolean depth_test;
    GLboolean dither;
    GLboolean fog;
    GLboolean lighting;
    GLboolean normalize;
};

#define MAX_DRAW_BUFFERS 16

struct color_buffer_state
{
    GLfloat clear_color[4];
    GLenum  draw_buffers[MAX_DRAW_BUFFERS];
};

struct hint_state
{
    GLenum perspective_correction;
};

struct buffers
{
    unsigned int ref;
    struct rb_tree map;
    struct vk_device *vk_device;
};

struct context
{
    struct wgl_context base;

    HDC hdc;                       /* context creation DC */
    HGLRC share;                   /* context to be shared with */
    int *attribs;                  /* creation attributes */
    DWORD tid;                     /* thread that the context is current in */
    int major_version;             /* major GL version */
    int minor_version;             /* minor GL version */
    UINT64 debug_callback;         /* client pointer */
    UINT64 debug_user;             /* client pointer */
    GLubyte *extensions;           /* extension string */
    char *wow64_version;           /* wow64 GL version override */
    struct buffers *buffers;       /* wow64 buffers map */
    GLenum gl_error;               /* wrapped GL error */
    const char **extension_array;  /* array of supported extensions */
    size_t extension_count;        /* size of supported extensions */

    /* semi-stub state tracker for wglCopyContext */
    GLbitfield used;                            /* context state used bits */
    struct pixel_mode_state pixel_mode;         /* GL_PIXEL_MODE_BIT */
    struct lighting_state lighting;             /* GL_LIGHTING_BIT */
    struct depth_buffer_state depth_buffer;     /* GL_DEPTH_BUFFER_BIT */
    struct viewport_state viewport;             /* GL_VIEWPORT_BIT */
    struct enable_state enable;                 /* GL_ENABLE_BIT */
    struct color_buffer_state color_buffer;     /* GL_COLOR_BUFFER_BIT */
    struct hint_state hint;                     /* GL_HINT_BIT */
    GLuint draw_fbo;                            /* currently bound draw FBO name */
    GLuint read_fbo;                            /* currently bound read FBO name */
    GLboolean has_viewport;                     /* whether viewport has been initialized */
};

struct buffer
{
    struct rb_entry entry;
    GLuint name;
    size_t size;
    void *host_ptr;
    void *map_ptr;
    size_t copy_length;
    void *vm_ptr;
    SIZE_T vm_size;

    /* members of Vulkan-backed buffer storages */
    struct vk_device *vk_device;
    VkDeviceMemory vk_memory;
    GLuint gl_memory;
    GLbitfield flags;
};

struct vk_device
{
    struct rb_entry entry;
    VkDevice vk_device;
    GUID uuid;
    VkPhysicalDeviceMemoryProperties memory_properties;

    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkDestroyDevice p_vkDestroyDevice;
    PFN_vkFlushMappedMemoryRanges p_vkFlushMappedMemoryRanges;
    PFN_vkFreeMemory p_vkFreeMemory;
    PFN_vkGetMemoryFdKHR p_vkGetMemoryFdKHR;
    PFN_vkMapMemory2KHR p_vkMapMemory2KHR;
    PFN_vkUnmapMemory2KHR p_vkUnmapMemory2KHR;
};

struct wgl_handle
{
    UINT handle;
    const struct opengl_funcs *funcs;
    union
    {
        struct wgl_context *context;    /* for HANDLE_CONTEXT */
        struct wgl_pbuffer *pbuffer;    /* for HANDLE_PBUFFER */
        GLsync sync;                    /* for HANDLE_GLSYNC */
        struct wgl_handle *next;        /* for free handles */
    } u;
};

#define MAX_WGL_HANDLES 1024
static struct wgl_handle wgl_handles[MAX_WGL_HANDLES];
static struct wgl_handle *next_free;
static unsigned int handle_count;

static ULONG_PTR zero_bits;

static const struct vulkan_funcs *vk_funcs;
static VkInstance vk_instance;
static PFN_vkDestroyInstance p_vkDestroyInstance;

static int vk_device_cmp( const void *key, const struct rb_entry *entry )
{
    struct vk_device *vk_device = RB_ENTRY_VALUE( entry, struct vk_device, entry );
    return memcmp( key, &vk_device->uuid, sizeof(vk_device->uuid) );
}

struct rb_tree vk_devices = { vk_device_cmp };

static struct context *context_from_wgl_context( struct wgl_context *context )
{
    return CONTAINING_RECORD( context, struct context, base );
}

/* the current context is assumed valid and doesn't need locking */
static struct context *get_current_context( TEB *teb, struct opengl_drawable **draw, struct opengl_drawable **read )
{
    struct wgl_context *context;
    if (!teb->glCurrentRC) return NULL;
    if (!(context = wgl_handles[LOWORD(teb->glCurrentRC) & ~HANDLE_TYPE_MASK].u.context)) return NULL;
    if (draw) *draw = context->draw;
    if (read) *read = context->read;
    return context_from_wgl_context( context );
}

static inline HANDLE next_handle( struct wgl_handle *ptr, enum wgl_handle_type type )
{
    WORD generation = HIWORD( ptr->handle ) + 1;
    if (!generation) generation++;
    ptr->handle = MAKELONG( ptr - wgl_handles, generation ) | type;
    return ULongToHandle( ptr->handle );
}

static struct wgl_handle *get_handle_ptr( HANDLE handle )
{
    unsigned int index = LOWORD( handle ) & ~HANDLE_TYPE_MASK;

    if (index < handle_count && ULongToHandle(wgl_handles[index].handle) == handle)
        return &wgl_handles[index];

    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
    return NULL;
}

struct context_attribute_desc
{
    GLbitfield bit;
    unsigned short offset;
    unsigned short size;
};

static struct context_attribute_desc context_attributes[] =
{
#define CONTEXT_ATTRIBUTE_DESC(bit, name, field) [name] = { bit, offsetof(struct context, field), sizeof(((struct context *)0)->field) }
    CONTEXT_ATTRIBUTE_DESC( GL_COLOR_BUFFER_BIT, GL_COLOR_CLEAR_VALUE, color_buffer.clear_color ),
    CONTEXT_ATTRIBUTE_DESC( GL_DEPTH_BUFFER_BIT, GL_DEPTH_FUNC, depth_buffer.depth_func ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_CULL_FACE, enable.cull_face ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_DEPTH_TEST, enable.depth_test ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_DITHER, enable.dither ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_FOG, enable.fog ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_LIGHTING, enable.lighting ),
    CONTEXT_ATTRIBUTE_DESC( GL_ENABLE_BIT, GL_NORMALIZE, enable.normalize ),
    CONTEXT_ATTRIBUTE_DESC( GL_HINT_BIT, GL_PERSPECTIVE_CORRECTION_HINT, hint.perspective_correction ),
    CONTEXT_ATTRIBUTE_DESC( GL_LIGHTING_BIT, GL_LIGHT_MODEL_AMBIENT, lighting.model.ambient ),
    CONTEXT_ATTRIBUTE_DESC( GL_LIGHTING_BIT, GL_LIGHT_MODEL_TWO_SIDE, lighting.model.two_side ),
    CONTEXT_ATTRIBUTE_DESC( GL_LIGHTING_BIT, GL_SHADE_MODEL, lighting.shade_model ),
    CONTEXT_ATTRIBUTE_DESC( GL_VIEWPORT_BIT, GL_VIEWPORT, viewport ),
#undef CONTEXT_ATTRIBUTE_DESC
};

/* GL constants used as indexes should be small, make sure size is reasonable */
C_ASSERT( sizeof(context_attributes) <= 64 * 1024 );

void set_context_attribute( TEB *teb, GLenum name, const void *value, size_t size )
{
    struct context *ctx;
    GLbitfield bit;

    if (!(ctx = get_current_context( teb, NULL, NULL ))) return;

    if (name >= ARRAY_SIZE(context_attributes) || !(bit = context_attributes[name].bit)) bit = -1 /* unsupported */;
    else if (size && size != context_attributes[name].size) ERR( "Invalid state attrib %#x parameter size %#zx\n", name, size );
    else memcpy( (char *)ctx + context_attributes[name].offset, value, context_attributes[name].size );

    if (bit == -1 && ctx->used != -1) WARN( "Unsupported attribute on context %p/%p\n", teb->glCurrentRC, ctx );
    ctx->used |= bit;
}

static BOOL copy_context_attributes( TEB *teb, const struct opengl_funcs *funcs, HGLRC dst_handle, struct context *dst,
                                     HGLRC src_handle, struct context *src, GLbitfield mask )
{
    HDC draw_hdc = teb->glReserved1[0], read_hdc = teb->glReserved1[1];
    struct context *old_ctx = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *old_funcs = teb->glTable;

    if (dst == old_ctx)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!mask) return TRUE;
    if (src->used == -1) FIXME( "Unsupported attributes on context %p/%p\n", src_handle, src );
    if (src != dst && dst->used == -1) FIXME( "Unsupported attributes on context %p/%p\n", dst_handle, dst );

    funcs->p_wglMakeCurrent( dst->hdc, &dst->base );

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        const GLfloat *floats = src->color_buffer.clear_color;
        funcs->p_glClearColor( floats[0], floats[1], floats[2], floats[3] );
        dst->color_buffer = src->color_buffer;
    }
    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        funcs->p_glDepthFunc( src->depth_buffer.depth_func );
        dst->depth_buffer = src->depth_buffer;
    }
    if (mask & GL_ENABLE_BIT)
    {
        if (src->enable.cull_face) funcs->p_glEnable( GL_CULL_FACE );
        else funcs->p_glDisable( GL_CULL_FACE );
        if (src->enable.depth_test) funcs->p_glEnable( GL_DEPTH_TEST );
        else funcs->p_glDisable( GL_DEPTH_TEST );
        if (src->enable.dither) funcs->p_glEnable( GL_DITHER );
        else funcs->p_glDisable( GL_DITHER );
        if (src->enable.fog) funcs->p_glEnable( GL_FOG );
        else funcs->p_glDisable( GL_FOG );
        if (src->enable.lighting) funcs->p_glEnable( GL_LIGHTING );
        else funcs->p_glDisable( GL_LIGHTING );
        if (src->enable.normalize) funcs->p_glEnable( GL_NORMALIZE );
        else funcs->p_glDisable( GL_NORMALIZE );
        dst->enable = src->enable;
    }
    if (mask & GL_HINT_BIT)
    {
        funcs->p_glHint( GL_PERSPECTIVE_CORRECTION_HINT, src->hint.perspective_correction );
        dst->hint = src->hint;
    }
    if (mask & GL_LIGHTING_BIT)
    {
        funcs->p_glLightModelfv( GL_LIGHT_MODEL_AMBIENT, src->lighting.model.ambient );
        funcs->p_glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, src->lighting.model.two_side );
        funcs->p_glShadeModel( src->lighting.shade_model );
        dst->lighting = src->lighting;
    }
    if (mask & GL_VIEWPORT_BIT)
    {
        funcs->p_glViewport( src->viewport.x, src->viewport.y, src->viewport.w, src->viewport.h );
        dst->viewport = src->viewport;
    }
    dst->used |= (src->used & mask);

    if (!old_ctx) funcs->p_wglMakeCurrent( NULL, NULL );
    else if (!old_funcs->p_wglMakeContextCurrentARB) old_funcs->p_wglMakeCurrent( draw_hdc, &old_ctx->base );
    else old_funcs->p_wglMakeContextCurrentARB( draw_hdc, read_hdc, &old_ctx->base );

    return dst->used != -1 && src->used != -1;
}

static void unmap_vk_buffer( struct buffer *buffer )
{
    VkMemoryUnmapInfoKHR unmap_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR,
        .memory = buffer->vk_memory,
        .flags = VK_MEMORY_UNMAP_RESERVE_BIT_EXT,
    };
    VkResult vr;

    vr = buffer->vk_device->p_vkUnmapMemory2KHR( buffer->vk_device->vk_device, &unmap_info );
    if (vr) ERR( "VkMemoryUnmapInfoKHR failed: %x\n", vr);
}

static int compare_buffer_name( const void *key, const struct rb_entry *entry )
{
    struct buffer *buffer = RB_ENTRY_VALUE( entry, struct buffer, entry );
    return memcmp( key, &buffer->name, sizeof(buffer->name) );
}

static void free_buffer( const struct opengl_funcs *funcs, struct buffer *buffer )
{
    if (buffer->vk_memory)
    {
        if (buffer->map_ptr) unmap_vk_buffer( buffer );
        buffer->vk_device->p_vkFreeMemory( buffer->vk_device->vk_device, buffer->vk_memory, NULL );
    }
    if (buffer->gl_memory) funcs->p_glDeleteMemoryObjectsEXT( 1, &buffer->gl_memory );
    if (buffer->vm_ptr) NtFreeVirtualMemory( GetCurrentProcess(), &buffer->vm_ptr, &buffer->vm_size, MEM_RELEASE );
    free( buffer );
}

static void release_buffers( const struct opengl_funcs *funcs, struct buffers *buffers )
{
    struct buffer *buffer, *next;

    if (--buffers->ref) return;

    RB_FOR_EACH_ENTRY_DESTRUCTOR( buffer, next, &buffers->map, struct buffer, entry )
        free_buffer( funcs, buffer );
    free( buffers );
}

static struct context *opengl_context_from_handle( TEB *teb, HGLRC handle, const struct opengl_funcs **funcs );

/* update handle context if it has been re-shared with another one */
static void update_handle_context( TEB *teb, HGLRC handle, struct wgl_handle *ptr )
{
    struct context *ctx = context_from_wgl_context( ptr->u.context ), *shared;
    const struct opengl_funcs *funcs = ptr->funcs, *share_funcs;

    if (ctx->tid) return; /* currently in use */
    if (ctx->share == (HGLRC)-1) return; /* not re-shared */

    shared = ctx->share ? opengl_context_from_handle( teb, ctx->share, &share_funcs ) : NULL;
    if (!funcs->p_wgl_context_reset( &ctx->base, ctx->hdc, shared ? &shared->base : NULL, ctx->attribs ))
    {
        WARN( "Failed to re-create context for wglShareLists\n" );
        return;
    }
    if (shared && shared->buffers)
    {
        release_buffers( funcs, ctx->buffers );
        ctx->buffers = shared->buffers;
        ctx->buffers->ref++;
    }
    ctx->share = (HGLRC)-1; /* initial shared context */
    copy_context_attributes( teb, funcs, handle, ctx, handle, ctx, ctx->used );
}

static struct context *opengl_context_from_handle( TEB *teb, HGLRC handle, const struct opengl_funcs **funcs )
{
    struct wgl_handle *entry;
    if (!(entry = get_handle_ptr( handle ))) return NULL;
    update_handle_context( teb, handle, entry );
    *funcs = entry->funcs;
    return context_from_wgl_context( entry->u.context );
}

static struct wgl_pbuffer *wgl_pbuffer_from_handle( HPBUFFERARB handle, const struct opengl_funcs **funcs )
{
    struct wgl_handle *entry;
    if (!(entry = get_handle_ptr( handle ))) return NULL;
    *funcs = entry->funcs;
    return entry->u.pbuffer;
}

static HANDLE alloc_handle( enum wgl_handle_type type, const struct opengl_funcs *funcs, void *user_ptr )
{
    HANDLE handle = 0;
    struct wgl_handle *ptr = NULL;

    if ((ptr = next_free))
        next_free = next_free->u.next;
    else if (handle_count < MAX_WGL_HANDLES)
        ptr = &wgl_handles[handle_count++];

    if (ptr)
    {
        ptr->funcs = funcs;
        ptr->u.context = user_ptr;
        handle = next_handle( ptr, type );
    }
    else RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
    return handle;
}

static void free_handle_ptr( struct wgl_handle *ptr )
{
    ptr->handle |= 0xffff;
    ptr->u.next = next_free;
    ptr->funcs = NULL;
    next_free = ptr;
}

static void update_teb32_context( TEB *teb )
{
#ifdef _WIN64
    TEB32 *teb32;

    if (!teb->WowTebOffset) return;
    teb32 = (TEB32 *)((char *)teb + teb->WowTebOffset);

    teb32->glCurrentRC = (UINT_PTR)teb->glCurrentRC;
    teb32->glReserved1[0] = (UINT_PTR)teb->glReserved1[0];
    teb32->glReserved1[1] = (UINT_PTR)teb->glReserved1[1];
#endif
}

static int *memdup_attribs( const int *attribs )
{
    const int *attr;
    size_t size;
    int *copy;

    if (!(attr = attribs)) return NULL;
    while (*attr++) { /* nothing */ }

    size = (attr - attribs) * sizeof(*attr);
    if (!(copy = malloc( size ))) return NULL;
    memcpy( copy, attribs, size );
    return copy;
}

/* check if the extension is present in the list */
static BOOL has_extension( const char *list, const char *ext, size_t len )
{
    while (list)
    {
        while (*list == ' ') list++;
        if (!strncmp( list, ext, len ) && (!list[len] || list[len] == ' ')) return TRUE;
        list = strchr( list, ' ' );
    }
    return FALSE;
}

static const char *legacy_extensions[] =
{
    "WGL_EXT_extensions_string",
    "WGL_EXT_swap_control",
    NULL,
};

static const char *parse_gl_version( const char *gl_version, int *major, int *minor )
{
    const char *ptr = gl_version;

    *major = atoi( ptr );
    if (*major <= 0)
        ERR( "Invalid OpenGL major version %d.\n", *major );

    while (isdigit( *ptr )) ++ptr;
    if (*ptr++ != '.')
        ERR( "Invalid OpenGL version string %s.\n", debugstr_a(gl_version) );

    *minor = atoi( ptr );

    while (isdigit( *ptr )) ++ptr;
    return ptr;
}

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline UINT asciiz_to_unicode( WCHAR *dst, const char *src )
{
    WCHAR *p = dst;
    while ((*p++ = *src++));
    return (p - dst) * sizeof(WCHAR);
}

static inline void unicode_to_ascii( char *dst, const WCHAR *src, size_t len )
{
    while (len--) *dst++ = *src++;
}

static HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len )
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    return NtOpenKeyEx( &ret, MAXIMUM_ALLOWED, &attr, 0 ) ? 0 : ret;
}

static HKEY open_hkcu_key( const char *name )
{
    WCHAR bufferW[256];
    static HKEY hkcu;

    if (!hkcu)
    {
        char buffer[256];
        DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
        DWORD i, len = sizeof(sid_data);
        SID *sid;

        if (NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len ))
            return 0;

        sid = ((TOKEN_USER *)sid_data)->User.Sid;
        len = snprintf( buffer, sizeof(buffer), "\\Registry\\User\\S-%u-%u", sid->Revision,
                        MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5],
                                            sid->IdentifierAuthority.Value[4] ),
                                  MAKEWORD( sid->IdentifierAuthority.Value[3],
                                            sid->IdentifierAuthority.Value[2] )));
        for (i = 0; i < sid->SubAuthorityCount; i++)
            len += snprintf( buffer + len, sizeof(buffer) - len, "-%u", sid->SubAuthority[i] );

        ascii_to_unicode( bufferW, buffer, len );
        hkcu = reg_open_key( NULL, bufferW, len * sizeof(WCHAR) );
    }

    return reg_open_key( hkcu, bufferW, asciiz_to_unicode( bufferW, name ) - sizeof(WCHAR) );
}

static ULONG query_reg_value( HKEY hkey, const WCHAR *name, KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size )
{
    unsigned int name_size = name ? lstrlenW( name ) * sizeof(WCHAR) : 0;
    UNICODE_STRING nameW = { name_size, name_size, (WCHAR *)name };

    if (NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                         info, size, &size ))
        return 0;

    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}

static ULONG query_reg_ascii_value( HKEY hkey, const char *name, KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size )
{
    WCHAR nameW[64];
    asciiz_to_unicode( nameW, name );
    return query_reg_value( hkey, nameW, info, size );
}

static DWORD get_ascii_config_key( HKEY defkey, HKEY appkey, const char *name,
                                   char *buffer, DWORD size )
{
    char buf[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[4096])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (void *)buf;

    if (appkey && query_reg_ascii_value( appkey, name, info, sizeof(buf) ))
    {
        size = min( info->DataLength, size - sizeof(WCHAR) ) / sizeof(WCHAR);
        unicode_to_ascii( buffer, (WCHAR *)info->Data, size );
        buffer[size] = 0;
        return 0;
    }

    if (defkey && query_reg_ascii_value( defkey, name, info, sizeof(buf) ))
    {
        size = min( info->DataLength, size - sizeof(WCHAR) ) / sizeof(WCHAR);
        unicode_to_ascii( buffer, (WCHAR *)info->Data, size );
        buffer[size] = 0;
        return 0;
    }

    return ERROR_FILE_NOT_FOUND;
}

static char *query_opengl_option( const char *name )
{
    WCHAR bufferW[MAX_PATH + 16], *p, *appname;
    HKEY defkey, appkey = 0;
    char buffer[4096];
    char *str = NULL;
    DWORD len;

    /* @@ Wine registry key: HKCU\Software\Wine\OpenGL */
    defkey = open_hkcu_key( "Software\\Wine\\OpenGL" );

    /* open the app-specific key */
    appname = NtCurrentTeb()->Peb->ProcessParameters->ImagePathName.Buffer;
    if ((p = wcsrchr( appname, '/' ))) appname = p + 1;
    if ((p = wcsrchr( appname, '\\' ))) appname = p + 1;
    len = lstrlenW( appname );

    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        int i;

        for (i = 0; appname[i]; i++) bufferW[i] = RtlDowncaseUnicodeChar( appname[i] );
        bufferW[i] = 0;
        appname = bufferW;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\OpenGL */
        if ((tmpkey = open_hkcu_key( "Software\\Wine\\AppDefaults" )))
        {
            static const WCHAR openglW[] = {'\\','O','p','e','n','G','L',0};
            memcpy( appname + i, openglW, sizeof(openglW) );
            appkey = reg_open_key( tmpkey, appname, lstrlenW( appname ) * sizeof(WCHAR) );
            NtClose( tmpkey );
        }
    }

    if (!get_ascii_config_key( defkey, appkey, name, buffer, sizeof(buffer) ))
        str = strdup( buffer );

    if (appkey) NtClose( appkey );
    if (defkey) NtClose( defkey );
    return str;
}

static int string_array_cmp( const void *p1, const void *p2 )
{
    const char *const *s1 = p1;
    const char *const *s2 = p2;
    return strcmp( *s1, *s2 );
}

/* Check if a GL extension is supported */
static BOOL is_extension_supported( struct context *ctx, const char *extension )
{
    return bsearch( &extension, ctx->extension_array, ctx->extension_count,
                    sizeof(ctx->extension_array[0]), string_array_cmp ) != NULL;
}

/* build the extension string by filtering out the disabled extensions */
static GLubyte *filter_extensions( struct context *ctx, const char *extensions )
{
    const char *end, **extra;
    size_t size, len;
    char *p, *str;

    size = strlen( extensions ) + 2;
    for (extra = legacy_extensions; *extra; extra++) size += strlen( *extra ) + 1;
    if (!(p = str = malloc( size ))) return NULL;

    TRACE( "GL_EXTENSIONS:\n" );

    for (;;)
    {
        while (*extensions == ' ') extensions++;
        if (!*extensions) break;
        len = (end = strchr( extensions, ' ' )) ? end - extensions : strlen( extensions );
        memcpy( p, extensions, len );
        p[len] = 0;
        if (is_extension_supported( ctx, p ))
        {
            TRACE( "++ %s\n", p );
            p += len;
            *p++ = ' ';
        }
        else
        {
            TRACE( "-- %s (disabled in context)\n", p );
        }
        extensions = end;
    }

    for (extra = legacy_extensions; *extra; extra++)
    {
        size = strlen( *extra );
        memcpy( p, *extra, size );
        p += size;
        *p++ = ' ';
    }

    if (p != str) --p;
    *p = 0;
    return (GLubyte *)str;
}

/* Check if any GL extension from the list is supported */
static BOOL is_any_extension_supported( struct context *ctx, const char *extension )
{
    size_t len;

    /* We use the GetProcAddress function from the display driver to retrieve function pointers
     * for OpenGL and WGL extensions. In case of winex11.drv the OpenGL extension lookup is done
     * using glXGetProcAddress. This function is quite unreliable in the sense that its specs don't
     * require the function to return NULL when an extension isn't found. For this reason we check
     * if the OpenGL extension required for the function we are looking up is supported. */

    while ((len = strlen( extension )))
    {
        TRACE( "Checking for extension '%s'\n", extension );

        /* Check if the extension is part of the GL extension string to see if it is supported. */
        if (is_extension_supported( ctx, extension )) return TRUE;

        /* In general an OpenGL function starts as an ARB/EXT extension and at some stage
         * it becomes part of the core OpenGL library and can be reached without the ARB/EXT
         * suffix as well. In the extension table, these functions contain GL_VERSION_major_minor.
         * Check if we are searching for a core GL function */
        if (!strncmp( extension, "GL_VERSION_", 11 ))
        {
            int major = extension[11] - '0'; /* Move past 'GL_VERSION_' */
            int minor = extension[13] - '0';

            /* Compare the major/minor version numbers of the native OpenGL library and what is required by the function.
             * The gl_version string is guaranteed to have at least a major/minor and sometimes it has a release number as well. */
            if (ctx->major_version > major || (ctx->major_version == major && ctx->minor_version >= minor)) return TRUE;

            WARN( "The function requires OpenGL version '%d.%d' while your drivers only provide '%d.%d'\n",
                  major, minor, ctx->major_version, ctx->minor_version );
        }

        extension += len + 1;
    }

    return FALSE;
}

static BOOL get_default_fbo_integer( struct context *ctx, struct opengl_drawable *draw, struct opengl_drawable *read,
                                     GLenum pname, GLint *data )
{
    if (pname == GL_READ_BUFFER && !ctx->read_fbo && read->read_fbo)
    {
        if (ctx->pixel_mode.read_buffer) *data = ctx->pixel_mode.read_buffer;
        else *data = read->doublebuffer ? GL_BACK : GL_FRONT;
        return TRUE;
    }
    if ((pname == GL_DRAW_BUFFER || pname == GL_DRAW_BUFFER0) && !ctx->draw_fbo && draw->draw_fbo)
    {
        if (ctx->color_buffer.draw_buffers[0]) *data = ctx->color_buffer.draw_buffers[0];
        else *data = draw->doublebuffer ? GL_BACK : GL_FRONT;
        return TRUE;
    }
    if (pname >= GL_DRAW_BUFFER1 && pname <= GL_DRAW_BUFFER15 && !ctx->draw_fbo && draw->draw_fbo)
    {
        *data = ctx->color_buffer.draw_buffers[pname - GL_DRAW_BUFFER0];
        return TRUE;
    }
    if (pname == GL_DOUBLEBUFFER && draw->draw_fbo)
    {
        *data = draw->doublebuffer;
        return TRUE;
    }
    if (pname == GL_STEREO && draw->draw_fbo)
    {
        *data = draw->stereo;
        return TRUE;
    }

    return FALSE;
}

static BOOL get_integer( TEB *teb, GLenum pname, GLint *data )
{
    struct opengl_drawable *draw, *read;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, &draw, &read ))) return FALSE;

    switch (pname)
    {
    case GL_MAJOR_VERSION:
        *data = ctx->major_version;
        return TRUE;
    case GL_MINOR_VERSION:
        *data = ctx->minor_version;
        return TRUE;
    case GL_NUM_EXTENSIONS:
        *data = ctx->extension_count;
        return TRUE;
    case GL_DRAW_FRAMEBUFFER_BINDING:
        if (!draw->draw_fbo) break;
        *data = ctx->draw_fbo;
        return TRUE;
    case GL_READ_FRAMEBUFFER_BINDING:
        if (!read->read_fbo) break;
        *data = ctx->read_fbo;
        return TRUE;
    }

    return get_default_fbo_integer( ctx, draw, read, pname, data );
}

const GLubyte *wrap_glGetString( TEB *teb, GLenum name )
{
    const struct opengl_funcs *funcs = teb->glTable;
    const GLubyte *ret;

    if ((ret = funcs->p_glGetString( name )))
    {
        if (name == GL_EXTENSIONS)
        {
            struct context *ctx = get_current_context( teb, NULL, NULL );
            GLubyte **extensions = &ctx->extensions;
            if (*extensions || (*extensions = filter_extensions( ctx, (const char *)ret ))) return *extensions;
        }
        else if (name == GL_VERSION)
        {
            struct context *ctx = get_current_context( teb, NULL, NULL );
            if (ctx->wow64_version) return (const GLubyte *)ctx->wow64_version;
        }
    }

    return ret;
}

const GLubyte *wrap_glGetStringi( TEB *teb, GLenum name, GLuint index )
{
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->p_glGetStringi;
        *func_ptr = funcs->p_wglGetProcAddress( "glGetStringi" );
    }

    if (name == GL_EXTENSIONS)
    {
        struct context *ctx = get_current_context( teb, NULL, NULL );
        if (index < ctx->extension_count) return (const GLubyte *)ctx->extension_array[index];
        index = -1;
    }

    return funcs->p_glGetStringi( name, index );
}

static int registry_entry_cmp( const void *a, const void *b )
{
    const struct registry_entry *entry_a = a, *entry_b = b;
    return strcmp( entry_a->name, entry_b->name );
}

PROC wrap_wglGetProcAddress( TEB *teb, LPCSTR name )
{
    const struct registry_entry entry = {.name = name}, *found;
    struct opengl_funcs *funcs = teb->glTable;
    const void **func_ptr;
    struct context *ctx;

    /* Without an active context opengl32 doesn't know to what
     * driver it has to dispatch wglGetProcAddress.
     */
    if (!(ctx = get_current_context( teb, NULL, NULL )))
    {
        WARN( "No active WGL context found\n" );
        return (void *)-1;
    }

    if (!(found = bsearch( &entry, extension_registry, extension_registry_size, sizeof(entry), registry_entry_cmp )))
    {
        WARN( "Function %s unknown\n", name );
        return (void *)-1;
    }

    func_ptr = (const void **)((char *)funcs + found->offset);
    if (!*func_ptr)
    {
        void *driver_func = funcs->p_wglGetProcAddress( name );

        if (!is_any_extension_supported( ctx, found->extension ))
        {
            unsigned int i;
            static const struct { const char *name, *alt; } alternatives[] =
            {
                { "glCopyTexSubImage3DEXT", "glCopyTexSubImage3D" },     /* needed by RuneScape */
                { "glVertexAttribDivisor", "glVertexAttribDivisorARB"},  /* needed by Caffeine */
                { "glCompressedTexImage2DARB", "glCompressedTexImage2D" }, /* needed by Grim Fandango Remastered */
            };

            for (i = 0; i < ARRAY_SIZE(alternatives); i++)
            {
                if (strcmp( name, alternatives[i].name )) continue;
                WARN( "Extension %s required for %s not supported, trying %s\n", found->extension,
                      name, alternatives[i].alt );
                return wrap_wglGetProcAddress( teb, alternatives[i].alt );
            }

            WARN( "Extension %s required for %s not supported\n", found->extension, name );
            return (void *)-1;
        }

        if (driver_func == NULL)
        {
            WARN( "Function %s not supported by driver\n", name );
            return (void *)-1;
        }

        *func_ptr = driver_func;
    }

    /* Return the index into the extension registry instead of a useless
     * function pointer, PE side will returns its own function pointers.
     */
    return (void *)(UINT_PTR)(found - extension_registry);
}

BOOL wrap_wglCopyContext( TEB *teb, HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask )
{
    const struct opengl_funcs *src_funcs, *dst_funcs;
    struct context *src, *dst;
    BOOL ret = FALSE;

    if (!(src = opengl_context_from_handle( teb, hglrcSrc, &src_funcs ))) return FALSE;
    if (!(dst = opengl_context_from_handle( teb, hglrcDst, &dst_funcs ))) return FALSE;
    else ret = copy_context_attributes( teb, dst_funcs, hglrcDst, dst, hglrcSrc, src, mask );

    return ret;
}

static BOOL initialize_vk_device( TEB *teb, struct context *ctx )
{
    struct opengl_funcs *funcs = teb->glTable;
    VkPhysicalDevice *vk_physical_devices = NULL;
    struct vk_device *vk_device = NULL;
    GLint uuid_count = 0, i, j;
    VkResult vr;

    static PFN_vkCreateDevice p_vkCreateDevice;
    static PFN_vkEnumeratePhysicalDevices p_vkEnumeratePhysicalDevices;
    static PFN_vkGetPhysicalDeviceFeatures2KHR p_vkGetPhysicalDeviceFeatures2KHR;
    static PFN_vkGetPhysicalDeviceMemoryProperties p_vkGetPhysicalDeviceMemoryProperties;
    static PFN_vkGetPhysicalDeviceProperties2KHR p_vkGetPhysicalDeviceProperties2KHR;

    if (ctx->buffers->vk_device) return TRUE; /* already initialized */
    if (!is_extension_supported( ctx, "GL_EXT_memory_object_fd" ))
    {
        TRACE( "GL_EXT_memory_object_fd is not supported\n" );
        return FALSE;
    }

    if (!vk_funcs)
    {
        PFN_vkCreateInstance p_vkCreateInstance;
        static const char *instance_extensions[] =
        {
            "VK_KHR_get_physical_device_properties2",
        };
        VkApplicationInfo app_info =
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pEngineName = "Wine WGL",
            .engineVersion = 1,
        };
        VkInstanceCreateInfo create_info =
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
            .ppEnabledExtensionNames = instance_extensions,
            .enabledExtensionCount = ARRAYSIZE(instance_extensions),
        };

        if (!(vk_funcs = __wine_get_vulkan_driver( WINE_VULKAN_DRIVER_VERSION ))) return FALSE;
        if (!vk_funcs->p_vkGetInstanceProcAddr
            || !(p_vkCreateInstance = (void *)vk_funcs->p_vkGetInstanceProcAddr( NULL, "vkCreateInstance" )))
            return FALSE;
        if ((vr = p_vkCreateInstance( &create_info, NULL, &vk_instance )))
        {
            WARN( "Failed to create a Vulkan instance, vr %d.\n", vr );
            return FALSE;
        }
#define GET_VK_FUNC(name) p_##name = (void *)vk_funcs->p_vkGetInstanceProcAddr( vk_instance, #name )
        GET_VK_FUNC( vkCreateDevice );
        GET_VK_FUNC( vkDestroyInstance );
        GET_VK_FUNC( vkEnumeratePhysicalDevices );
        GET_VK_FUNC( vkGetPhysicalDeviceFeatures2KHR );
        GET_VK_FUNC( vkGetPhysicalDeviceMemoryProperties );
        GET_VK_FUNC( vkGetPhysicalDeviceProperties2KHR );
#undef GET_VK_FUNC
    }
    if (!vk_instance) return FALSE;

#define GET_GL_FUNC(name) if (!funcs->p_##name) funcs->p_##name = (void *)funcs->p_wglGetProcAddress( #name )
    GET_GL_FUNC( glBufferStorageMemEXT );
    GET_GL_FUNC( glCreateMemoryObjectsEXT );
    GET_GL_FUNC( glDeleteMemoryObjectsEXT );
    GET_GL_FUNC( glGetUnsignedBytei_vEXT );
    GET_GL_FUNC( glImportMemoryFdEXT );
    GET_GL_FUNC( glNamedBufferStorageMemEXT );
#undef GET_GL_FUNC

    funcs->p_glGetIntegerv( GL_NUM_DEVICE_UUIDS_EXT, &uuid_count );
    for (i = 0; i < uuid_count; i++)
    {
        GLubyte uuid[GL_UUID_SIZE_EXT];
        struct rb_entry *entry;
        uint32_t count = 0;

        funcs->p_glGetUnsignedBytei_vEXT( GL_DEVICE_UUID_EXT, i, uuid );
        if ((entry = rb_get( &vk_devices, uuid )))
        {
            vk_device = RB_ENTRY_VALUE( entry, struct vk_device, entry );
            if (!vk_device->vk_device) continue; /* known incompatible device */
            TRACE( "Found existing device %p for uuid %s\n", vk_device, debugstr_guid(&vk_device->uuid) );
            ctx->buffers->vk_device = vk_device;
            free( vk_physical_devices );
            return TRUE;
        }

        if (!(vk_device = calloc( 1, sizeof(*vk_device) ))) return FALSE;
        memcpy( &vk_device->uuid, uuid, sizeof(uuid) );
        rb_put( &vk_devices, uuid, &vk_device->entry );

        if (!vk_physical_devices)
        {
            vr = p_vkEnumeratePhysicalDevices( vk_instance, &count, NULL );
            if (!vr && count)
            {
                vk_physical_devices = calloc( count, sizeof(*vk_physical_devices) );
                vr = p_vkEnumeratePhysicalDevices( vk_instance, &count, vk_physical_devices );
            }
            if (vr || !count)
            {
                WARN( "Could not get vulkan physical devices: %d\n", vr );
                continue;
            }
        }

        for (j = 0; j < count; j++)
        {
            static const char *device_extensions[] =
            {
                "VK_KHR_external_memory",
                "VK_KHR_external_memory_fd",
                "VK_EXT_map_memory_placed",
                "VK_KHR_map_memory2",
            };
            VkPhysicalDeviceIDProperties id_props =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
            };
            VkPhysicalDeviceProperties2 props =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &id_props,
            };
            float priority = 0.0f;
            VkDeviceQueueCreateInfo queue_info =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueCount = 1,
                .pQueuePriorities = &priority,
            };
            VkDeviceCreateInfo device_info =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queue_info,
                .enabledExtensionCount = ARRAYSIZE(device_extensions),
                .ppEnabledExtensionNames = device_extensions,
            };
            VkPhysicalDeviceMapMemoryPlacedFeaturesEXT map_placed_feature =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_FEATURES_EXT,
            };
            VkPhysicalDeviceFeatures2 features =
            {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &map_placed_feature,
            };

            p_vkGetPhysicalDeviceProperties2KHR( vk_physical_devices[j], &props );
            if (memcmp( id_props.deviceUUID, uuid, sizeof(uuid) )) continue;

            p_vkGetPhysicalDeviceFeatures2KHR( vk_physical_devices[j], &features );
            if (!map_placed_feature.memoryMapPlaced || !map_placed_feature.memoryUnmapReserve)
            {
                WARN( "Device %s does not support placed memory mapping\n", debugstr_guid(&vk_device->uuid) );
                continue;
            }

            p_vkGetPhysicalDeviceMemoryProperties( vk_physical_devices[j], &vk_device->memory_properties );
            vr = p_vkCreateDevice( vk_physical_devices[j], &device_info, NULL, &vk_device->vk_device );
            if (vr)
            {
                WARN( "Could not create vulkan device: %d\n", vr );
                continue;
            }

#define GET_VK_FUNC(name) vk_device->p_##name = (void *)vk_funcs->p_vkGetDeviceProcAddr( vk_device->vk_device, #name )
            GET_VK_FUNC( vkAllocateMemory );
            GET_VK_FUNC( vkDestroyDevice );
            GET_VK_FUNC( vkFreeMemory );
            GET_VK_FUNC( vkGetMemoryFdKHR );
            GET_VK_FUNC( vkMapMemory2KHR );
            GET_VK_FUNC( vkUnmapMemory2KHR );
            GET_VK_FUNC( vkFlushMappedMemoryRanges );
#undef GET_VK_FUNC
            TRACE( "Created %p device for uuid %s\n", vk_device, debugstr_guid(&vk_device->uuid) );
            free( vk_physical_devices );
            return TRUE;
        }
    }

    free( vk_physical_devices );
    WARN( "Could not find compatible Vulkan device\n" );
    return FALSE;
}

static void make_context_current( TEB *teb, const struct opengl_funcs *funcs, HDC draw_hdc, HDC read_hdc,
                                  HGLRC hglrc, struct context *ctx )
{
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);
    size_t size = ARRAYSIZE(legacy_extensions) - 1, count = 0;
    const char *version, *rest = "", **extensions;
    int i, j;

    static const char *disabled, *enabled;

    ctx->tid = tid;
    teb->glReserved1[0] = draw_hdc;
    teb->glReserved1[1] = read_hdc;
    teb->glCurrentRC = hglrc;
    teb->glTable = (void *)funcs;
    pop_default_fbo( teb );

    if (ctx->major_version) return; /* already synced */

    version = (const char *)funcs->p_glGetString( GL_VERSION );
    if (version) rest = parse_gl_version( version, &ctx->major_version, &ctx->minor_version );
    if (!ctx->major_version) ctx->major_version = 1;
    TRACE( "context %p version %d.%d\n", ctx, ctx->major_version, ctx->minor_version );

    if (ctx->major_version >= 3)
    {
        GLint extensions_count;

        if (!funcs->p_glGetStringi)
        {
            void **func_ptr = (void **)&funcs->p_glGetStringi;
            *func_ptr = funcs->p_wglGetProcAddress( "glGetStringi" );
        }

        funcs->p_glGetIntegerv( GL_NUM_EXTENSIONS, &extensions_count );
        size += extensions_count;
        if (!(extensions = malloc( size * sizeof(*extensions) ))) return;
        for (i = 0; i < extensions_count; i++) extensions[count++] = (const char *)funcs->p_glGetStringi( GL_EXTENSIONS, i );
    }
    else
    {
        const char *str = (const char *)funcs->p_glGetString( GL_EXTENSIONS );
        size_t len = strlen( str );
        const char *p;
        char *ext;
        if (!str) str = "";
        if ((len = strlen( str )) && str[len - 1] == ' ') len--;
        if (*str) size++;
        for (p = str; p < str + len; p++) if (*p == ' ') size++;
        if (!(extensions = malloc( size * sizeof(*extensions) + len + 1 ))) return;
        ext = (char *)&extensions[size];
        memcpy( ext, str, len );
        ext[len] = 0;
        if (*ext) extensions[count++] = ext;
        while (*ext)
        {
            if (*ext == ' ')
            {
                *ext = 0;
                extensions[count++] = ext + 1;
            }
            ext++;
        }
        assert( count + ARRAYSIZE(legacy_extensions) - 1 == size );
    }

    if (!disabled && !(disabled = query_opengl_option( "DisabledExtensions" ))) disabled = "";
    if (!enabled && !(enabled = query_opengl_option( "EnabledExtensions" ))) enabled = "";
    if (*enabled || *disabled)
    {
        for (i = 0, j = 0; i < count; i++)
        {
            size_t len = strlen( extensions[i] );
            if (!has_extension( disabled, extensions[i], len ) && (!*enabled || has_extension( enabled, extensions[i], len )))
                extensions[j++] = extensions[i];
            else
                TRACE( "-- %s (disabled by config)\n", extensions[i] );
        }
        count = j;
    }

    for (i = 0; legacy_extensions[i]; i++) extensions[count++] = legacy_extensions[i];
    qsort( extensions, count, sizeof(*extensions), string_array_cmp );
    ctx->extension_array = extensions;
    ctx->extension_count = count;

    if (is_win64 && ctx->buffers && !initialize_vk_device( teb, ctx ))
    {
        if (ctx->major_version > 4 || (ctx->major_version == 4 && ctx->minor_version > 3))
        {
            FIXME( "GL version %d.%d is not supported on wow64, using 4.3\n", ctx->major_version, ctx->minor_version );
            ctx->major_version = 4;
            ctx->minor_version = 3;
            asprintf( &ctx->wow64_version, "4.3%s", rest );
        }
        for (i = 0, j = 0; i < count; i++)
        {
            const char *ext = extensions[i];
            if (!strcmp( ext, "GL_ARB_buffer_storage" ) || !strcmp( ext, "GL_ARB_buffer_storage" ))
            {
                FIXME( "Disabling %s extension on wow64\n", ext );
                continue;
            }
            extensions[j++] = ext;
        }
        ctx->extension_count = j;
    }

    if (TRACE_ON(opengl)) for (i = 0; i < count; i++) TRACE( "++ %s\n", extensions[i] );
}

BOOL wrap_wglMakeCurrent( TEB *teb, HDC hdc, HGLRC hglrc )
{
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);
    struct context *ctx, *prev = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *funcs = teb->glTable;

    if (hglrc)
    {
        if (!(ctx = opengl_context_from_handle( teb, hglrc, &funcs ))) return FALSE;
        if (ctx->tid && ctx->tid != tid)
        {
            RtlSetLastWin32Error( ERROR_BUSY );
            return FALSE;
        }

        if (!funcs->p_wglMakeCurrent( hdc, &ctx->base )) return FALSE;
        if (prev) prev->tid = 0;
        make_context_current( teb, funcs, hdc, hdc, hglrc, ctx );
    }
    else if (prev)
    {
        if (!funcs->p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->tid = 0;
        teb->glCurrentRC = 0;
        teb->glTable = &null_opengl_funcs;
    }
    else if (!hdc)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    update_teb32_context( teb );
    return TRUE;
}

static void free_context( struct context *ctx )
{
    free( ctx->wow64_version );
    free( ctx->extension_array );
    free( ctx->extensions );
    free( ctx->attribs );
    free( ctx );
}

BOOL wrap_wglDeleteContext( TEB *teb, HGLRC hglrc )
{
    struct wgl_handle *ptr;
    struct context *ctx;
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);

    if (!(ptr = get_handle_ptr( hglrc ))) return FALSE;
    ctx = context_from_wgl_context( ptr->u.context );
    if (ctx->tid && ctx->tid != tid)
    {
        RtlSetLastWin32Error( ERROR_BUSY );
        return FALSE;
    }
    if (hglrc == teb->glCurrentRC) wrap_wglMakeCurrent( teb, 0, 0 );
    ptr->funcs->p_wgl_context_reset( &ctx->base, NULL, NULL, NULL );
    free_context( ctx );
    free_handle_ptr( ptr );
    return TRUE;
}

static GLenum drawable_buffer_from_buffer( struct opengl_drawable *drawable, GLenum buffer )
{
    if (buffer == GL_NONE) return GL_NONE;
    if (buffer < GL_FRONT_LEFT || buffer > GL_AUX3) return GL_NONE;
    return drawable->buffer_map[buffer - GL_FRONT_LEFT];
}

static BOOL context_draws_back( struct context *ctx )
{
    for (int i = 0; i < ARRAY_SIZE(ctx->color_buffer.draw_buffers); i++)
    {
        switch (ctx->color_buffer.draw_buffers[i])
        {
        case GL_LEFT:
        case GL_RIGHT:
        case GL_BACK:
        case GL_FRONT_AND_BACK:
        case GL_BACK_LEFT:
        case GL_BACK_RIGHT:
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL context_draws_front( struct context *ctx )
{
    for (int i = 0; i < ARRAY_SIZE(ctx->color_buffer.draw_buffers); i++)
    {
        switch (ctx->color_buffer.draw_buffers[i])
        {
        case GL_LEFT:
        case GL_RIGHT:
        case GL_FRONT:
        case GL_FRONT_AND_BACK:
        case GL_FRONT_LEFT:
        case GL_FRONT_RIGHT:
            return TRUE;
        }
    }

    return FALSE;
}

static void flush_context( TEB *teb, void (*flush)(void) )
{
    struct opengl_drawable *read, *draw;
    struct context *ctx = get_current_context( teb, &read, &draw );
    const struct opengl_funcs *funcs = teb->glTable;
    BOOL force_swap = flush && !ctx->draw_fbo && context_draws_front( ctx ) &&
                      draw->buffer_map[0] == GL_BACK_LEFT && draw->client;

    if (!ctx || !funcs->p_wgl_context_flush( &ctx->base, flush, force_swap ))
    {
        /* default implementation: call the functions directly */
        if (flush) flush();
    }

    if (force_swap)
    {
        GLenum mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        RECT rect;

        WARN( "Front buffer rendering emulation, copying front buffer back\n" );

        NtUserGetClientRect( draw->client->hwnd, &rect, NtUserGetDpiForWindow( draw->client->hwnd ) );
        if (ctx->read_fbo) funcs->p_glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
        funcs->p_glReadBuffer( GL_FRONT_LEFT );
        funcs->p_glBlitFramebuffer( 0, 0, 0, 0, rect.right, rect.bottom, rect.right, rect.bottom, mask, GL_NEAREST );
        if (ctx->read_fbo) funcs->p_glBindFramebuffer( GL_READ_FRAMEBUFFER, ctx->read_fbo );
        else funcs->p_glReadBuffer( drawable_buffer_from_buffer( read, ctx->pixel_mode.read_buffer ) );
    }
}

void wrap_glFinish( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, funcs->p_glFinish );
    resolve_default_fbo( teb, FALSE );
}

void wrap_glFlush( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, funcs->p_glFlush );
    resolve_default_fbo( teb, FALSE );
}

void wrap_glClear( TEB *teb, GLbitfield mask )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glClear( mask );
    resolve_default_fbo( teb, FALSE );
}

void wrap_glDrawPixels( TEB *teb, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glDrawPixels( width, height, format, type, pixels );
    resolve_default_fbo( teb, FALSE );
}

void wrap_glReadPixels( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glReadPixels( x, y, width, height, format, type, pixels );
}

void wrap_glViewport( TEB *teb, GLint x, GLint y, GLsizei width, GLsizei height )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glViewport( x, y, width, height );
    resolve_default_fbo( teb, FALSE );
}

BOOL wrap_wglSwapBuffers( TEB *teb, HDC hdc )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );
    BOOL ret;

    resolve_default_fbo( teb, FALSE );

    if (!(ret = funcs->p_wglSwapBuffers( hdc )))
    {
        /* default implementation: implicitly flush the context */
        flush_context( teb, funcs->p_glFlush );
    }

    return ret;
}

BOOL wrap_wglShareLists( TEB *teb, HGLRC hglrcSrc, HGLRC hglrcDst )
{
    const struct opengl_funcs *src_funcs, *dst_funcs;
    struct context *src, *dst;
    BOOL ret = FALSE;

    if (!(src = opengl_context_from_handle( teb, hglrcSrc, &src_funcs ))) return FALSE;
    if (!(dst = opengl_context_from_handle( teb, hglrcDst, &dst_funcs ))) return FALSE;
    if (src_funcs != dst_funcs) RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
    else if ((ret = dst->used != -1)) dst->share = hglrcSrc;
    else FIXME( "Unsupported attributes on context %p/%p\n", hglrcDst, dst );

    return ret;
}

BOOL wrap_wglBindTexImageARB( TEB *teb, HPBUFFERARB handle, int buffer )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return FALSE;
    return funcs->p_wglBindTexImageARB( pbuffer, buffer );
}

HGLRC wrap_wglCreateContextAttribsARB( TEB *teb, HDC hdc, HGLRC share, const int *attribs )
{
    HGLRC ret = 0;
    struct context *context, *share_ctx = NULL;
    const struct opengl_funcs *funcs = get_dc_funcs( hdc ), *share_funcs;

    if (!funcs)
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return 0;
    }
    if (share && !(share_ctx = opengl_context_from_handle( teb, share, &share_funcs )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_OPERATION );
        return 0;
    }
    if ((context = calloc( 1, sizeof(*context) )))
    {
        context->hdc = hdc;
        context->share = (HGLRC)-1; /* initial shared context */
        context->attribs = memdup_attribs( attribs );
        if (is_win64 && is_wow64())
        {
            if (share_ctx)
            {
                context->buffers = share_ctx->buffers;
                context->buffers->ref++;
            }
            else if (!(context->buffers = malloc( sizeof(*context->buffers ))))
            {
                free_context( context );
                return 0;
            }
            else
            {
                context->buffers->ref = 1;
                context->buffers->vk_device = NULL;
                rb_init( &context->buffers->map, compare_buffer_name );
            }
        }
        if (!(funcs->p_wgl_context_reset( &context->base, hdc, share_ctx ? &share_ctx->base : NULL, attribs ))) free_context( context );
        else if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context )))
        {
            funcs->p_wgl_context_reset( &context->base, NULL, NULL, NULL );
            free_context( context );
        }
    }
    return ret;
}

HGLRC wrap_wglCreateContext( TEB *teb, HDC hdc )
{
    return wrap_wglCreateContextAttribsARB( teb, hdc, NULL, NULL );
}

HPBUFFERARB wrap_wglCreatePbufferARB( TEB *teb, HDC hdc, int format, int width, int height, const int *attribs )
{
    HPBUFFERARB ret;
    struct wgl_pbuffer *pbuffer;
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->p_wglCreatePbufferARB) return 0;
    if (!(pbuffer = funcs->p_wglCreatePbufferARB( hdc, format, width, height, attribs ))) return 0;
    ret = alloc_handle( HANDLE_PBUFFER, funcs, pbuffer );
    if (!ret) funcs->p_wglDestroyPbufferARB( pbuffer );
    return ret;
}

BOOL wrap_wglDestroyPbufferARB( TEB *teb, HPBUFFERARB handle )
{
    struct wgl_pbuffer *pbuffer;
    struct wgl_handle *ptr;

    if (!(ptr = get_handle_ptr( handle ))) return FALSE;
    pbuffer = ptr->u.pbuffer;
    ptr->funcs->p_wglDestroyPbufferARB( pbuffer );
    free_handle_ptr( ptr );
    return TRUE;
}

HDC wrap_wglGetPbufferDCARB( TEB *teb, HPBUFFERARB handle )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return 0;
    return funcs->p_wglGetPbufferDCARB( pbuffer );
}

BOOL wrap_wglMakeContextCurrentARB( TEB *teb, HDC draw_hdc, HDC read_hdc, HGLRC hglrc )
{
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);
    struct context *ctx, *prev = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *funcs = teb->glTable;

    if (hglrc)
    {
        if (!(ctx = opengl_context_from_handle( teb, hglrc, &funcs ))) return FALSE;
        if (ctx->tid && ctx->tid != tid)
        {
            RtlSetLastWin32Error( ERROR_BUSY );
            return FALSE;
        }

        if (!funcs->p_wglMakeContextCurrentARB) return FALSE;
        if (!funcs->p_wglMakeContextCurrentARB( draw_hdc, read_hdc, &ctx->base )) return FALSE;
        if (prev) prev->tid = 0;
        make_context_current( teb, funcs, draw_hdc, read_hdc, hglrc, ctx );
    }
    else if (prev)
    {
        if (!funcs->p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->tid = 0;
        teb->glCurrentRC = 0;
        teb->glTable = &null_opengl_funcs;
    }
    update_teb32_context( teb );
    return TRUE;
}

BOOL wrap_wglQueryPbufferARB( TEB *teb, HPBUFFERARB handle, int attrib, int *value )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return FALSE;
    return funcs->p_wglQueryPbufferARB( pbuffer, attrib, value );
}

int wrap_wglReleasePbufferDCARB( TEB *teb, HPBUFFERARB handle, HDC hdc )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return FALSE;
    return funcs->p_wglReleasePbufferDCARB( pbuffer, hdc );
}

BOOL wrap_wglReleaseTexImageARB( TEB *teb, HPBUFFERARB handle, int buffer )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return FALSE;
    return funcs->p_wglReleaseTexImageARB( pbuffer, buffer );
}

BOOL wrap_wglSetPbufferAttribARB( TEB *teb, HPBUFFERARB handle, const int *attribs )
{
    const struct opengl_funcs *funcs;
    struct wgl_pbuffer *pbuffer;
    if (!(pbuffer = wgl_pbuffer_from_handle( handle, &funcs ))) return FALSE;
    return funcs->p_wglSetPbufferAttribARB( pbuffer, attribs );
}

static void gl_debug_message_callback( GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar *message, const void *user )
{
    struct gl_debug_message_callback_params *params;
    void *ret_ptr;
    ULONG ret_len;
    struct context *ctx = (struct context *)user;
    UINT len = strlen( message ) + 1, size;

    if (!ctx->debug_callback) return;
    if (!NtCurrentTeb())
    {
        fprintf( stderr, "msg:gl_debug_message_callback called from native thread, severity %#x, message \"%.*s\".\n",
                 severity, length, message );
        return;
    }

    size = offsetof(struct gl_debug_message_callback_params, message[len] );
    if (!(params = malloc( size ))) return;
    params->dispatch.callback = call_gl_debug_message_callback;
    params->debug_callback = ctx->debug_callback;
    params->debug_user = ctx->debug_user;
    params->source = source;
    params->type = type;
    params->id = id;
    params->severity = severity;
    params->length = length;
    memcpy( params->message, message, len );

    KeUserDispatchCallback( &params->dispatch, size, &ret_ptr, &ret_len );
    free( params );
}

void wrap_glDebugMessageCallback( TEB *teb, GLDEBUGPROC callback, const void *user )
{
    struct context *ctx = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallback) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallback( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
}

void wrap_glDebugMessageCallbackAMD( TEB *teb, GLDEBUGPROCAMD callback, void *user )
{
    struct context *ctx = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallbackAMD) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallbackAMD( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
}

void wrap_glDebugMessageCallbackARB( TEB *teb, GLDEBUGPROCARB callback, const void *user )
{
    struct context *ctx = get_current_context( teb, NULL, NULL );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallbackARB) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallbackARB( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
}

void set_current_fbo( TEB *teb, GLenum target, GLuint fbo )
{
    struct context *ctx;

    if (!(ctx = get_current_context( teb, NULL, NULL ))) return;
    if (target == GL_FRAMEBUFFER) ctx->draw_fbo = ctx->read_fbo = fbo;
    if (target == GL_DRAW_FRAMEBUFFER) ctx->draw_fbo = fbo;
    if (target == GL_READ_FRAMEBUFFER) ctx->read_fbo = fbo;
}

GLuint get_default_fbo( TEB *teb, GLenum target )
{
    struct opengl_drawable *draw, *read;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, &draw, &read ))) return 0;
    if (target == GL_FRAMEBUFFER) return draw->draw_fbo;
    if (target == GL_DRAW_FRAMEBUFFER) return draw->draw_fbo;
    if (target == GL_READ_FRAMEBUFFER) return read->read_fbo;
    return 0;
}

void push_default_fbo( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw, *read;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, &draw, &read ))) return;
    if (!ctx->draw_fbo && draw->draw_fbo) funcs->p_glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
    if (!ctx->read_fbo && read->read_fbo) funcs->p_glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
}

void pop_default_fbo( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw, *read;
    struct context *ctx;
    RECT rect;

    if (!(ctx = get_current_context( teb, &draw, &read ))) return;
    if (!ctx->draw_fbo && draw->draw_fbo) funcs->p_glBindFramebuffer( GL_DRAW_FRAMEBUFFER, draw->draw_fbo );
    if (!ctx->read_fbo && read->read_fbo) funcs->p_glBindFramebuffer( GL_READ_FRAMEBUFFER, read->read_fbo );
    if (!ctx->has_viewport && draw->draw_fbo && draw->client)
    {
        NtUserGetClientRect( draw->client->hwnd, &rect, NtUserGetDpiForWindow( draw->client->hwnd ) );
        funcs->p_glViewport( 0, 0, rect.right, rect.bottom );
        ctx->has_viewport = GL_TRUE;
    }
}

void resolve_default_fbo( TEB *teb, BOOL read )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *drawable;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, read ? NULL : &drawable, read ? &drawable : NULL )) || !drawable) return;

    if (drawable->draw_fbo && drawable->read_fbo && drawable->draw_fbo != drawable->read_fbo)
    {
        GLenum mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        RECT rect;

        NtUserGetClientRect( drawable->client->hwnd, &rect, NtUserGetDpiForWindow( drawable->client->hwnd ) );

        if (context_draws_front( ctx ))
        {
            funcs->p_glNamedFramebufferReadBuffer( drawable->draw_fbo, GL_COLOR_ATTACHMENT0 );
            funcs->p_glNamedFramebufferDrawBuffer( drawable->read_fbo, GL_COLOR_ATTACHMENT0 );
            funcs->p_glBlitNamedFramebuffer( drawable->draw_fbo, drawable->read_fbo, 0, 0, 0, 0, rect.right, rect.bottom,
                                             rect.right, rect.bottom, mask, GL_NEAREST );
            mask &= ~(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        if ((drawable->doublebuffer && context_draws_back( ctx )) || (!drawable->doublebuffer && drawable->stereo && context_draws_front( ctx )))
        {
            funcs->p_glNamedFramebufferReadBuffer( drawable->draw_fbo, GL_COLOR_ATTACHMENT1 );
            funcs->p_glNamedFramebufferDrawBuffer( drawable->read_fbo, GL_COLOR_ATTACHMENT1 );
            funcs->p_glBlitNamedFramebuffer( drawable->draw_fbo, drawable->read_fbo, 0, 0, 0, 0, rect.right, rect.bottom,
                                             rect.right, rect.bottom, mask, GL_NEAREST );
            mask &= ~(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        if (drawable->doublebuffer && drawable->stereo && context_draws_front( ctx ))
        {
            funcs->p_glNamedFramebufferReadBuffer( drawable->draw_fbo, GL_COLOR_ATTACHMENT2 );
            funcs->p_glNamedFramebufferDrawBuffer( drawable->read_fbo, GL_COLOR_ATTACHMENT2 );
            funcs->p_glBlitNamedFramebuffer( drawable->draw_fbo, drawable->read_fbo, 0, 0, 0, 0, rect.right, rect.bottom,
                                             rect.right, rect.bottom, mask, GL_NEAREST );
            mask &= ~(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }

        if (drawable->doublebuffer && drawable->stereo && context_draws_back( ctx ))
        {
            funcs->p_glNamedFramebufferReadBuffer( drawable->draw_fbo, GL_COLOR_ATTACHMENT3 );
            funcs->p_glNamedFramebufferDrawBuffer( drawable->read_fbo, GL_COLOR_ATTACHMENT3 );
            funcs->p_glBlitNamedFramebuffer( drawable->draw_fbo, drawable->read_fbo, 0, 0, 0, 0, rect.right, rect.bottom,
                                             rect.right, rect.bottom, mask, GL_NEAREST );
            mask &= ~(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    }
}

static GLenum *set_default_fbo_draw_buffers( struct context *ctx, struct opengl_drawable *draw,
                                             GLsizei count, const GLenum *src, GLenum *dst )
{
    memset( ctx->color_buffer.draw_buffers, 0, sizeof(ctx->color_buffer.draw_buffers) );

    for (GLsizei i = 0; i < count; i++)
    {
        dst[i] = drawable_buffer_from_buffer( draw, src[i] );
        if (src[i] && !dst[i]) WARN( "Invalid draw buffer #%d %#x for context %p\n", i, src[i], ctx );

        if (i >= MAX_DRAW_BUFFERS) FIXME( "Needs %u draw buffers\n", i );
        else ctx->color_buffer.draw_buffers[i] = src[i];
    }

    return dst;
}

void wrap_glDrawBuffers( TEB *teb, GLsizei n, const GLenum *bufs )
{
    struct opengl_funcs *funcs = teb->glTable;
    GLenum buffer[MAX_DRAW_BUFFERS];
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !ctx->draw_fbo)
        bufs = set_default_fbo_draw_buffers( ctx, draw, n, bufs, buffer );

    funcs->p_glDrawBuffers( n, bufs );
}

void wrap_glFramebufferDrawBuffersEXT( TEB *teb, GLuint fbo, GLsizei n, const GLenum *bufs )
{
    struct opengl_funcs *funcs = teb->glTable;
    GLenum buffer[MAX_DRAW_BUFFERS];
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !fbo)
        bufs = set_default_fbo_draw_buffers( ctx, draw, n, bufs, buffer );

    funcs->p_glFramebufferDrawBuffersEXT( fbo, n, bufs );
}

void wrap_glNamedFramebufferDrawBuffers( TEB *teb, GLuint fbo, GLsizei n, const GLenum *bufs )
{
    struct opengl_funcs *funcs = teb->glTable;
    GLenum buffer[MAX_DRAW_BUFFERS];
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !fbo)
        bufs = set_default_fbo_draw_buffers( ctx, draw, n, bufs, buffer );

    funcs->p_glNamedFramebufferDrawBuffers( fbo, n, bufs );
}

static GLenum set_default_fbo_draw_buffer( struct context *ctx, struct opengl_drawable *draw, GLint src )
{
    GLenum dst = drawable_buffer_from_buffer( draw, src );
    if (src && !dst) WARN( "Invalid draw buffer %#x for context %p\n", src, ctx );
    memset( ctx->color_buffer.draw_buffers, 0, sizeof(ctx->color_buffer.draw_buffers) );
    ctx->color_buffer.draw_buffers[0] = src;
    return dst;
}

void wrap_glDrawBuffer( TEB *teb, GLenum buf )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !ctx->draw_fbo)
        buf = set_default_fbo_draw_buffer( ctx, draw, buf );

    funcs->p_glDrawBuffer( buf );
}

void wrap_glFramebufferDrawBufferEXT( TEB *teb, GLuint fbo, GLenum mode )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !fbo)
        mode = set_default_fbo_draw_buffer( ctx, draw, mode );

    funcs->p_glFramebufferDrawBufferEXT( fbo, mode );
}

void wrap_glNamedFramebufferDrawBuffer( TEB *teb, GLuint fbo, GLenum buf )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, NULL )) && !fbo)
        buf = set_default_fbo_draw_buffer( ctx, draw, buf );

    funcs->p_glNamedFramebufferDrawBuffer( fbo, buf );
}

static GLenum set_default_fbo_read_buffer( struct context *ctx, struct opengl_drawable *read, GLint src )
{
    GLenum dst = drawable_buffer_from_buffer( read, src );
    if (src && !dst) WARN( "Invalid read buffer %#x for context %p\n", src, ctx );
    ctx->pixel_mode.read_buffer = src;
    return dst;
}

void wrap_glReadBuffer( TEB *teb, GLenum src )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *read;
    struct context *ctx;

    if ((ctx = get_current_context( teb, NULL, &read )) && !ctx->read_fbo)
        src = set_default_fbo_read_buffer( ctx, read, src );

    funcs->p_glReadBuffer( src );
}

void wrap_glFramebufferReadBufferEXT( TEB *teb, GLuint fbo, GLenum mode )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *read;
    struct context *ctx;

    if ((ctx = get_current_context( teb, NULL, &read )) && !fbo)
        mode = set_default_fbo_read_buffer( ctx, read, mode );

    funcs->p_glFramebufferReadBufferEXT( fbo, mode );
}

void wrap_glNamedFramebufferReadBuffer( TEB *teb, GLuint fbo, GLenum src )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *read;
    struct context *ctx;

    if ((ctx = get_current_context( teb, NULL, &read )) && !fbo)
        src = set_default_fbo_read_buffer( ctx, read, src );

    funcs->p_glNamedFramebufferReadBuffer( fbo, src );
}

void wrap_glGetIntegerv( TEB *teb, GLenum pname, GLint *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    if (get_integer( teb, pname, data )) return;
    funcs->p_glGetIntegerv( pname, data );
}

void wrap_glGetBooleanv( TEB *teb, GLenum pname, GLboolean *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLint value;
    if (get_integer( teb, pname, &value )) *data = !!value;
    else funcs->p_glGetBooleanv( pname, data );
}

void wrap_glGetDoublev( TEB *teb, GLenum pname, GLdouble *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLint value;
    if (get_integer( teb, pname, &value )) *data = value;
    funcs->p_glGetDoublev( pname, data );
}

void wrap_glGetFloatv( TEB *teb, GLenum pname, GLfloat *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLint value;
    if (get_integer( teb, pname, &value )) *data = value;
    else funcs->p_glGetFloatv( pname, data );
}

void wrap_glGetInteger64v( TEB *teb, GLenum pname, GLint64 *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLint value;
    if (get_integer( teb, pname, &value )) *data = value;
    else funcs->p_glGetInteger64v( pname, data );
}

void wrap_glGetFramebufferParameterivEXT( TEB *teb, GLuint fbo, GLenum pname, GLint *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct opengl_drawable *draw, *read;
    struct context *ctx;

    if ((ctx = get_current_context( teb, &draw, &read )) && !fbo && (fbo = draw->draw_fbo))
        if (get_default_fbo_integer( ctx, draw, read, pname, params )) return;

    funcs->p_glGetFramebufferParameterivEXT( fbo, pname, params );
}

NTSTATUS process_attach( void *args )
{
    struct process_attach_params *params = args;
    call_gl_debug_message_callback = params->call_gl_debug_message_callback;
    if (is_win64 && is_wow64())
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation( SystemEmulationBasicInformation, &info, sizeof(info), NULL );
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
    return STATUS_SUCCESS;
}

NTSTATUS thread_attach( void *args )
{
    TEB *teb = args;
    teb->glTable = &null_opengl_funcs;
    return STATUS_SUCCESS;
}

NTSTATUS process_detach( void *args )
{
    struct vk_device *vk_device, *next;

    RB_FOR_EACH_ENTRY_DESTRUCTOR( vk_device, next, &vk_devices, struct vk_device, entry )
    {
        if (vk_device->vk_device) vk_device->p_vkDestroyDevice( vk_device->vk_device, NULL );
        free( vk_device );
    }
    rb_destroy( &vk_devices, NULL, NULL );
    if (vk_instance)
    {
        p_vkDestroyInstance( vk_instance, NULL );
        vk_instance = NULL;
    }
    vk_funcs = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS get_pixel_formats( void *args )
{
    struct get_pixel_formats_params *params = args;
    const struct opengl_funcs *funcs = get_dc_funcs( params->hdc );
    if (!funcs || !funcs->p_get_pixel_formats) return STATUS_NOT_IMPLEMENTED;
    funcs->p_get_pixel_formats( params->formats, params->max_formats,
                                    &params->num_formats, &params->num_onscreen_formats );
    return STATUS_SUCCESS;
}

#ifdef _WIN64

struct wow64_string_entry
{
    const char *str;
    PTR32 wow64_str;
};
static struct wow64_string_entry *wow64_strings;
static SIZE_T wow64_strings_count;

NTSTATUS return_wow64_string( const void *str, PTR32 *wow64_str )
{
    void *tmp;
    SIZE_T i;

    if (!str)
    {
        *wow64_str = 0;
        return STATUS_SUCCESS;
    }

    pthread_mutex_lock( &wgl_lock );

    for (i = 0; i < wow64_strings_count; i++) if (wow64_strings[i].str == str) break;
    if (i == wow64_strings_count && (tmp = realloc( wow64_strings, (i + 1) * sizeof(*wow64_strings) )))
    {
        wow64_strings = tmp;
        wow64_strings[i].str = str;
        wow64_strings[i].wow64_str = 0;
        wow64_strings_count += 1;
    }

    if (i == wow64_strings_count) ERR( "Failed to allocate memory for wow64 strings\n" );
    else if (wow64_strings[i].wow64_str) *wow64_str = wow64_strings[i].wow64_str;
    else if (*wow64_str)
    {
        strcpy( UlongToPtr(*wow64_str), str );
        wow64_strings[i].wow64_str = *wow64_str;
    }

    pthread_mutex_unlock( &wgl_lock );

    if (*wow64_str) return STATUS_SUCCESS;
    *wow64_str = strlen( str ) + 1;
    return STATUS_BUFFER_TOO_SMALL;
}

GLenum wow64_glGetError( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLenum gl_err, prev_err;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, NULL, NULL ))) return GL_INVALID_OPERATION;
    gl_err = funcs->p_glGetError();
    prev_err = ctx->gl_error;
    ctx->gl_error = GL_NO_ERROR;
    return prev_err ? prev_err : gl_err;
}

static void set_gl_error( TEB *teb, GLenum gl_error )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct context *ctx;

    if (!(ctx = get_current_context( teb, NULL, NULL )) || ctx->gl_error) return;
    if (!(ctx->gl_error = funcs->p_glGetError())) ctx->gl_error = gl_error;
}

static struct wgl_handle *get_sync_ptr( TEB *teb, GLsync sync )
{
    struct wgl_handle *handle = get_handle_ptr( sync );
    if (!handle) set_gl_error( teb, GL_INVALID_VALUE );
    return handle;
}

GLenum wow64_glClientWaitSync( TEB *teb, GLsync sync, GLbitfield flags, GLuint64 timeout )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if (!(handle = get_sync_ptr( teb, sync ))) return GL_INVALID_VALUE;
    return funcs->p_glClientWaitSync( handle->u.sync, flags, timeout );
}

void wow64_glDeleteSync( TEB *teb, GLsync sync )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if ((handle = get_sync_ptr( teb, sync )))
    {
        funcs->p_glDeleteSync( handle->u.sync );
        free_handle_ptr( handle );
    }
}

GLsync wow64_glFenceSync( TEB *teb, GLenum condition, GLbitfield flags )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLsync sync, handle;

    if (!(sync = funcs->p_glFenceSync( condition, flags ))) return NULL;

    pthread_mutex_lock( &wgl_lock );
    if (!(handle = alloc_handle( HANDLE_GLSYNC, NULL, sync ))) funcs->p_glDeleteSync( sync );
    pthread_mutex_unlock( &wgl_lock );
    return handle;
}

void wow64_glGetSynciv( TEB *teb, GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if ((handle = get_sync_ptr( teb, sync ))) funcs->p_glGetSynciv( handle->u.sync, pname, count, length, values );
}

GLboolean wow64_glIsSync( TEB *teb, GLsync sync )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if (!(handle = get_handle_ptr( sync ))) return FALSE;
    return funcs->p_glIsSync( handle->u.sync );
}

void wow64_glWaitSync( TEB *teb, GLsync sync, GLbitfield flags, GLuint64 timeout )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if ((handle = get_sync_ptr( teb, sync ))) funcs->p_glWaitSync( handle->u.sync, flags, timeout );
}

static GLint get_buffer_param( TEB *teb, GLenum target, GLenum param )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glGetBufferParameteriv) *func;
    GLint size = 0;
    if (!(func = funcs->p_glGetBufferParameteriv)) func = (void *)funcs->p_wglGetProcAddress( "glGetBufferParameteriv" );
    if (func) func( target, param, &size );
    return size;
}

static GLint get_named_buffer_param( TEB *teb, GLint buffer, GLenum param )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glGetNamedBufferParameteriv) *func;
    GLint size = 0;
    if (!(func = funcs->p_glGetNamedBufferParameteriv)) func = (void *)funcs->p_wglGetProcAddress( "glGetNamedBufferParameteriv" );
    if (func) func( buffer, param, &size );
    return size;
}

static void unmap_buffer( TEB *teb, GLenum target )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glUnmapBuffer) *func;
    if (!(func = funcs->p_glUnmapBuffer)) func = (void *)funcs->p_wglGetProcAddress( "glUnmapBuffer" );
    if (func) func( target );
}

static void unmap_named_buffer( TEB *teb, GLint buffer )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glUnmapNamedBuffer) *func;
    if (!(func = funcs->p_glUnmapNamedBuffer)) func = (void *)funcs->p_wglGetProcAddress( "glUnmapNamedBuffer" );
    if (func) func( buffer );
}

static GLuint get_target_name( TEB *teb, GLenum target )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLenum binding_name;
    GLint name = 0;

    switch (target)
    {
    case GL_ARRAY_BUFFER: binding_name = GL_ARRAY_BUFFER_BINDING; break;
    case GL_ATOMIC_COUNTER_BUFFER: binding_name = GL_ATOMIC_COUNTER_BUFFER_BINDING; break;
    case GL_COPY_READ_BUFFER: binding_name = GL_COPY_READ_BUFFER_BINDING; break;
    case GL_COPY_WRITE_BUFFER: binding_name = GL_COPY_WRITE_BUFFER_BINDING; break;
    case GL_DISPATCH_INDIRECT_BUFFER: binding_name = GL_DISPATCH_INDIRECT_BUFFER_BINDING; break;
    case GL_DRAW_INDIRECT_BUFFER: binding_name = GL_DRAW_INDIRECT_BUFFER_BINDING; break;
    case GL_ELEMENT_ARRAY_BUFFER: binding_name = GL_ELEMENT_ARRAY_BUFFER_BINDING; break;
    case GL_PIXEL_PACK_BUFFER: binding_name = GL_PIXEL_PACK_BUFFER_BINDING; break;
    case GL_PIXEL_UNPACK_BUFFER: binding_name = GL_PIXEL_UNPACK_BUFFER_BINDING; break;
    case GL_QUERY_BUFFER: binding_name = GL_QUERY_BUFFER_BINDING; break;
    case GL_SHADER_STORAGE_BUFFER: binding_name = GL_SHADER_STORAGE_BUFFER_BINDING; break;
    case GL_TEXTURE_BUFFER: binding_name = GL_TEXTURE_BUFFER_BINDING; break;
    case GL_TRANSFORM_FEEDBACK_BUFFER: binding_name = GL_TRANSFORM_FEEDBACK_BUFFER_BINDING; break;
    case GL_UNIFORM_BUFFER: binding_name = GL_UNIFORM_BUFFER_BINDING; break;
    default:
        FIXME( "unknown target %x\n", target );
        return 0;
    };

    funcs->p_glGetIntegerv( binding_name, &name );
    return name;
}

static struct buffer *get_named_buffer( TEB *teb, GLuint name )
{
    struct context *ctx = get_current_context( teb, NULL, NULL );
    struct rb_entry *entry;

    if (ctx && (entry = rb_get( &ctx->buffers->map, &name )))
        return RB_ENTRY_VALUE( entry, struct buffer, entry );
    return NULL;
}

void invalidate_buffer_name( TEB *teb, GLuint name )
{
    struct buffer *buffer = get_named_buffer( teb, name );
    struct context *ctx;

    if (!buffer || !(ctx = get_current_context( teb, NULL, NULL ))) return;
    rb_remove( &ctx->buffers->map, &buffer->entry );
    free_buffer( teb->glTable, buffer );
}

void invalidate_buffer_target( TEB *teb, GLenum target )
{
    GLuint name = get_target_name( teb, target );
    if (name) invalidate_buffer_name( teb, name );
}

static struct buffer *get_target_buffer( TEB *teb, GLenum target )
{
    GLuint name = get_target_name( teb, target );
    return name ? get_named_buffer( teb, name ) : NULL;
}

static BOOL buffer_vm_alloc( TEB *teb, struct buffer *buffer, SIZE_T size )
{
    if (buffer->vm_size >= size) return TRUE;
    if (buffer->vm_ptr)
    {
        NtFreeVirtualMemory( GetCurrentProcess(), &buffer->vm_ptr, &buffer->vm_size, MEM_RELEASE );
        buffer->vm_ptr = NULL;
        buffer->vm_size = 0;
    }
    if (NtAllocateVirtualMemory( GetCurrentProcess(), &buffer->vm_ptr, zero_bits, &size,
                                 MEM_COMMIT, PAGE_READWRITE ))
    {
        ERR("NtAllocateVirtualMemory failed\n");
        set_gl_error( teb, GL_OUT_OF_MEMORY );
        return FALSE;
    }
    buffer->vm_size = size;
    return TRUE;
}

static void flush_buffer( TEB *teb, struct buffer *buffer, size_t offset, size_t length )
{
    VkMappedMemoryRange memory_range =
    {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = buffer->vk_memory,
        .offset = (char *)buffer->map_ptr - (char *)buffer->host_ptr + offset,
        .size = length,
    };
    VkResult vr;

    if (!buffer->vk_memory) return;

    vr = buffer->vk_device->p_vkFlushMappedMemoryRanges( buffer->vk_device->vk_device, 1, &memory_range );
    if (vr) ERR( "vkFlushMappedMemoryRanges failed: %x\n", vr );
}

static int find_vk_memory_type( struct vk_device *vk_device, uint32_t flags, uint32_t mask )
{
    uint32_t i;
    flags &= mask;
    for (i = 0; i < vk_device->memory_properties.memoryTypeCount; i++)
    {
        if ((vk_device->memory_properties.memoryTypes[i].propertyFlags & mask) == flags) return i;
    }
    return -1;
}

static struct buffer *create_buffer_storage( TEB *teb, GLenum target, GLuint name, size_t size, const void *data, GLbitfield flags )
{
    VkExportMemoryAllocateInfo export_alloc =
    {
        .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
        .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    VkMemoryAllocateInfo alloc_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &export_alloc,
        .allocationSize = size,
    };
    VkMemoryGetFdInfoKHR fd_info =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
    };
    struct opengl_funcs *funcs = teb->glTable;
    GLuint buffer_name = name ? name : get_target_name( teb, target );
    uint32_t type_mask = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t desired_type = type_mask;
    struct context *ctx = get_current_context( teb, NULL, NULL );
    struct vk_device *vk_device;
    struct buffer *buffer;
    int fd, memory_type;
    VkResult vr;

    if (!(vk_device = ctx->buffers->vk_device) || !vk_device->vk_device) return NULL;

    if (flags & GL_CLIENT_STORAGE_BIT) desired_type &= ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    memory_type = find_vk_memory_type( vk_device, desired_type, type_mask );
    if (memory_type == -1) /* if we can’t find a matching type, try ignoring GL_CLIENT_STORAGE_BIT */
        memory_type = find_vk_memory_type( vk_device, desired_type, type_mask & ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    if (memory_type == -1)
    {
        WARN( "Could not find memory type\n" );
        return NULL;
    }
    alloc_info.memoryTypeIndex = memory_type;

    if (!(buffer = calloc( 1, sizeof(*buffer) ))) return NULL;
    buffer->name = buffer_name;
    buffer->flags = flags;
    buffer->size = size;
    buffer->vk_device = vk_device;

    vr = vk_device->p_vkAllocateMemory( vk_device->vk_device, &alloc_info, NULL, &buffer->vk_memory );
    if (vr)
    {
        ERR( "vkAllocateMemory failed: %d\n", vr );
        free_buffer( funcs, buffer );
        return NULL;
    }

    if (data)
    {
        VkMemoryMapInfoKHR map_info =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
            .memory = buffer->vk_memory,
            .size = VK_WHOLE_SIZE,
        };
        VkMemoryUnmapInfoKHR unmap_info =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR,
            .memory = buffer->vk_memory,
        };
        void *ptr;

        vr = vk_device->p_vkMapMemory2KHR( vk_device->vk_device, &map_info, &ptr );
        if (vr)
        {
            ERR( "vkMapMemory2KHR failed: %d\n", vr );
            free_buffer( funcs, buffer );
            return NULL;
        }

        memcpy( ptr, data, size );
        vk_device->p_vkUnmapMemory2KHR( vk_device->vk_device, &unmap_info );
    }

    fd_info.memory = buffer->vk_memory;
    vr = vk_device->p_vkGetMemoryFdKHR( vk_device->vk_device, &fd_info, &fd );
    if (vr)
    {
        ERR( "vkGetMemoryFdKHR failed: %d\n", vr );
        free_buffer( funcs, buffer );
        return NULL;
    }

    funcs->p_glCreateMemoryObjectsEXT( 1, &buffer->gl_memory );
    funcs->p_glImportMemoryFdEXT( buffer->gl_memory, size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd );
    if (name)
        funcs->p_glNamedBufferStorageMemEXT( buffer->name, size, buffer->gl_memory, 0 );
    else
        funcs->p_glBufferStorageMemEXT( target, size, buffer->gl_memory, 0 );
    rb_put( &ctx->buffers->map, &buffer->name, &buffer->entry );
    TRACE( "created buffer_storage %p\n", buffer );
    return buffer;
}

static PTR32 wow64_map_buffer( TEB *teb, struct buffer *buffer, GLenum target, GLuint name, GLintptr offset,
                               size_t length, GLbitfield access, void *ptr )
{
    if (buffer && buffer->map_ptr)
    {
        set_gl_error( teb, GL_INVALID_OPERATION );
        return 0;
    }

    if (buffer && buffer->vk_memory)
    {
        VkMemoryMapPlacedInfoEXT placed_info =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_PLACED_INFO_EXT,
        };
        VkMemoryMapInfoKHR map_info =
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
            .flags = VK_MEMORY_MAP_PLACED_BIT_EXT,
            .pNext = &placed_info,
            .memory = buffer->vk_memory,
            .size = VK_WHOLE_SIZE,
        };
        VkResult vr;
        struct context *ctx = get_current_context( teb, NULL, NULL );
        struct vk_device *vk_device = ctx->buffers->vk_device;

        if (!buffer_vm_alloc( teb, buffer, buffer->size )) return 0;
        placed_info.pPlacedAddress = buffer->vm_ptr;
        vr = vk_device->p_vkMapMemory2KHR( vk_device->vk_device, &map_info, &buffer->host_ptr );
        if (vr)
        {
            ERR( "vkMapMemory2KHR failed: %d\n", vr );
            return 0;
        }
        assert( buffer->host_ptr == buffer->vm_ptr );
        buffer->map_ptr = (char *)buffer->vm_ptr + offset;
        TRACE( "returning vk mapping %p\n", buffer->map_ptr );
        return PtrToUlong( buffer->map_ptr );
    }

    if (!ptr) return 0;

    if (!buffer)
    {
        struct context *ctx = get_current_context( teb, NULL, NULL );

        if (!(buffer = calloc( 1, sizeof(*buffer) ))) return 0;
        buffer->name = name ? name : get_target_name( teb, target );
        buffer->size = name ? get_named_buffer_param( teb, name, GL_BUFFER_SIZE ) : get_buffer_param( teb, target, GL_BUFFER_SIZE );
        rb_put( &ctx->buffers->map, &buffer->name, &buffer->entry );
        TRACE( "allocated buffer %p for %u\n", buffer, buffer->name );
    }

    buffer->host_ptr = ptr;
    if (!offset && !length) length = buffer->size;
    if (ULongToPtr(PtrToUlong(ptr)) == ptr) /* we're lucky */
    {
        buffer->map_ptr = ptr;
        TRACE( "returning %p\n", buffer->map_ptr );
        return PtrToUlong( buffer->map_ptr );
    }

    if (access & GL_MAP_PERSISTENT_BIT)
    {
        FIXME( "GL_MAP_PERSISTENT_BIT not supported!\n" );
        goto unmap;
    }

    if (!buffer_vm_alloc( teb, buffer, length + (offset & 0xf) )) goto unmap;
    buffer->map_ptr = (char *)buffer->vm_ptr + (offset & 0xf);
    buffer->copy_length = (access & GL_MAP_WRITE_BIT) ? length : 0;
    if (!(access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)))
    {
        static int once;
        if (!once++)
            FIXME( "Doing a copy of a mapped buffer (expect performance issues)\n" );

        TRACE( "Copying %#zx from buffer at %p to wow64 buffer %p\n", length, buffer->host_ptr,
               buffer->map_ptr );
        memcpy( buffer->map_ptr, buffer->host_ptr, length );
    }
    TRACE( "returning copy buffer %p\n", buffer->map_ptr );
    return PtrToUlong( buffer->map_ptr );

unmap:
    if (name)
        unmap_named_buffer( teb, name );
    else
        unmap_buffer( teb, target );
    return 0;
}

static GLbitfield map_range_flags_from_map_flags( GLenum flags )
{
    switch (flags)
    {
        case GL_READ_ONLY: return GL_MAP_READ_BIT;
        case GL_WRITE_ONLY: return GL_MAP_WRITE_BIT;
        case GL_READ_WRITE: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        default:
            ERR( "invalid map flags %#x\n", flags );
            return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    }
}

void wow64_glDeleteBuffers( TEB *teb, GLsizei n, const GLuint *buffers )
{
    const struct opengl_funcs *funcs = teb->glTable;
    GLsizei i;

    pthread_mutex_lock( &wgl_lock );

    funcs->p_glDeleteBuffers( n, buffers );
    for (i = 0; i < n; i++) invalidate_buffer_name( teb, buffers[i] );

    pthread_mutex_unlock( &wgl_lock );
}

void wow64_glBufferStorage( TEB *teb, GLenum target, GLsizeiptr size, const void *data, GLbitfield flags )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer = NULL;

    if (flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))
        buffer = create_buffer_storage( teb, target, 0, size, data, flags );

    if (!buffer) funcs->p_glBufferStorage( target, size, data, flags );
}

void wow64_glNamedBufferStorage( TEB *teb, GLuint name, GLsizeiptr size, const void *data, GLbitfield flags )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer = NULL;

    if (flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))
        buffer = create_buffer_storage( teb, 0, name, size, data, flags );

    if (!buffer) funcs->p_glNamedBufferStorage( name, size, data, flags );
}

void wow64_glNamedBufferStorageEXT( TEB *teb, GLuint name, GLsizeiptr size, const void *data, GLbitfield flags )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer = NULL;

    if (flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))
        buffer = create_buffer_storage( teb, 0, name, size, data, flags );

    if (!buffer) funcs->p_glNamedBufferStorageEXT( name, size, data, flags );
}

static BOOL wow64_gl_get_buffer_pointer_v( TEB *teb, GLenum target, GLuint name, GLenum pname, PTR32 *wow_ptr )
{
    struct buffer *buffer;
    BOOL ret = FALSE;

    if (pname != GL_BUFFER_MAP_POINTER) return FALSE;

    pthread_mutex_lock( &wgl_lock );
    buffer = name ? get_named_buffer( teb, name ) : get_target_buffer( teb, target );
    if (buffer)
    {
        *wow_ptr = PtrToUlong( buffer->map_ptr );
        ret = TRUE;
    }
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

void wow64_glGetBufferPointerv( TEB *teb, GLenum target, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    if (wow64_gl_get_buffer_pointer_v( teb, target, 0, pname, params )) return;
    funcs->p_glGetBufferPointerv( target, pname, &ptr );
    *params = PtrToUlong(ptr);
}

void wow64_glGetBufferPointervARB( TEB *teb, GLenum target, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    if (wow64_gl_get_buffer_pointer_v( teb, target, 0, pname, params )) return;
    funcs->p_glGetBufferPointerv( target, pname, &ptr );
    *params = PtrToUlong(ptr);
}

void wow64_glGetNamedBufferPointerv( TEB *teb, GLuint buffer, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    if (buffer && wow64_gl_get_buffer_pointer_v( teb, 0, buffer, pname, params )) return;
    funcs->p_glGetNamedBufferPointerv( buffer, pname, &ptr );
    *params = PtrToUlong(ptr);
}

void wow64_glGetNamedBufferPointervEXT( TEB *teb, GLuint buffer, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    if (buffer && wow64_gl_get_buffer_pointer_v( teb, 0, buffer, pname, params )) return;
    funcs->p_glGetNamedBufferPointervEXT( buffer, pname, &ptr );
    *params = PtrToUlong(ptr);
}

static PTR32 wow64_gl_map_buffer( TEB *teb, GLenum target, GLenum access, PFN_glMapBuffer gl_map_buffer64 )
{
    GLbitfield range_access = map_range_flags_from_map_flags( access );
    struct buffer *buffer;
    void *ptr = NULL;
    PTR32 ret ;

    pthread_mutex_lock( &wgl_lock );
    buffer = get_target_buffer( teb, target );
    if (!buffer || !buffer->vk_memory) ptr = gl_map_buffer64( target, access );
    ret = wow64_map_buffer( teb, buffer, target, 0, 0, 0, range_access, ptr );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

PTR32 wow64_glMapBuffer( TEB *teb, GLenum target, GLenum access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_buffer( teb, target, access, funcs->p_glMapBuffer );
}

PTR32 wow64_glMapBufferARB( TEB *teb, GLenum target, GLenum access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_buffer( teb, target, access, funcs->p_glMapBufferARB );
}

PTR32 wow64_glMapBufferRange( TEB *teb, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer;
    void *ptr = NULL;
    PTR32 ret;

    pthread_mutex_lock( &wgl_lock );
    buffer = get_target_buffer( teb, target );
    if (!buffer || !buffer->vk_memory) ptr = funcs->p_glMapBufferRange( target, offset, length, access );
    ret = wow64_map_buffer( teb, buffer, target, 0, offset, length, access, ptr );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

static PTR32 wow64_gl_map_named_buffer( TEB *teb, GLuint name, GLenum access, PFN_glMapNamedBuffer gl_map_named_buffer64 )
{
    GLbitfield range_access = map_range_flags_from_map_flags( access );
    struct buffer *buffer;
    void *ptr = NULL;
    PTR32 ret;

    pthread_mutex_lock( &wgl_lock );
    buffer = get_named_buffer( teb, name );
    if (!buffer || !buffer->vk_memory) ptr = gl_map_named_buffer64( name, access );
    ret = wow64_map_buffer( teb, buffer, 0, name, 0, 0, range_access, ptr );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

PTR32 wow64_glMapNamedBuffer( TEB *teb, GLuint buffer, GLenum access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer( teb, buffer, access, funcs->p_glMapNamedBuffer );
}

PTR32 wow64_glMapNamedBufferEXT( TEB *teb, GLuint buffer, GLenum access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer( teb, buffer, access, funcs->p_glMapNamedBufferEXT );
}

static NTSTATUS wow64_gl_map_named_buffer_range( TEB *teb, GLuint name, GLintptr offset, GLsizeiptr length, GLbitfield access,
                                                 PFN_glMapNamedBufferRange gl_map_named_buffer_range64 )
{
    struct buffer *buffer;
    void *ptr = NULL;
    PTR32 ret;

    pthread_mutex_lock( &wgl_lock );
    buffer = get_named_buffer( teb, name );
    if (!buffer || !buffer->vk_memory) ptr = gl_map_named_buffer_range64( name, offset, length, access );
    ret = wow64_map_buffer( teb, buffer, 0, name, offset, length, access, ptr );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

PTR32 wow64_glMapNamedBufferRange( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer_range( teb, buffer, offset, length, access, funcs->p_glMapNamedBufferRange );
}

PTR32 wow64_glMapNamedBufferRangeEXT( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer_range( teb, buffer, offset, length, access, funcs->p_glMapNamedBufferRangeEXT );
}

static BOOL wow64_unmap_buffer( TEB *teb, struct buffer *buffer )
{
    if (buffer->vk_memory)
    {
        if (!buffer->map_ptr)
        {
            set_gl_error( teb, GL_INVALID_OPERATION );
            return FALSE;
        }
        unmap_vk_buffer( buffer );
    }

    if (buffer->host_ptr != buffer->map_ptr)
    {
        if (buffer->copy_length)
        {
            TRACE( "Copying %#zx from wow64 buffer %p to buffer %p\n", buffer->copy_length,
                   buffer->map_ptr, buffer->host_ptr );
            memcpy( buffer->host_ptr, buffer->map_ptr, buffer->copy_length );
        }
    }

    buffer->host_ptr = buffer->map_ptr = NULL;
    return TRUE;
}

static GLboolean wow64_unmap_target_buffer( TEB *teb, GLenum target, PFN_glUnmapBuffer gl_unmap )
{
    struct buffer *buffer;
    GLboolean ret;

    pthread_mutex_lock( &wgl_lock );
    if ((buffer = get_target_buffer( teb, target ))) ret = wow64_unmap_buffer( teb, buffer );
    if (!buffer || !buffer->vk_memory) ret = gl_unmap( target );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

GLboolean wow64_glUnmapBuffer( TEB *teb, GLenum target )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_unmap_target_buffer( teb, target, funcs->p_glUnmapBuffer );
}

GLboolean wow64_glUnmapBufferARB( TEB *teb, GLenum target )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_unmap_target_buffer( teb, target, funcs->p_glUnmapBufferARB );
}

static GLboolean wow64_gl_unmap_named_buffer( TEB *teb, GLuint name, PFN_glUnmapBuffer gl_unmap )
{
    struct buffer *buffer;
    GLboolean ret;

    pthread_mutex_lock( &wgl_lock );
    if ((buffer = get_named_buffer( teb, name ))) ret = wow64_unmap_buffer( teb, buffer );
    if (!buffer || !buffer->vk_memory) ret = gl_unmap( name );
    pthread_mutex_unlock( &wgl_lock );
    return ret;
}

GLboolean wow64_glUnmapNamedBuffer( TEB *teb, GLuint buffer )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_unmap_named_buffer( teb, buffer, funcs->p_glUnmapNamedBuffer );
}

GLboolean wow64_glUnmapNamedBufferEXT( TEB *teb, GLuint buffer )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_unmap_named_buffer( teb, buffer, funcs->p_glUnmapNamedBufferEXT );
}

void wow64_glFlushMappedBufferRange( TEB *teb, GLenum target, GLintptr offset, GLsizeiptr length )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer;

    pthread_mutex_lock( &wgl_lock );
    if ((buffer = get_target_buffer( teb, target ))) flush_buffer( teb, buffer, offset, length );
    if (!buffer || !buffer->vk_memory) funcs->p_glFlushMappedBufferRange( target, offset, length );
    pthread_mutex_unlock( &wgl_lock );
}

void wow64_glFlushMappedNamedBufferRange( TEB *teb, GLuint name, GLintptr offset, GLsizeiptr length )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer;

    pthread_mutex_lock( &wgl_lock );
    if ((buffer = get_named_buffer( teb, name ))) flush_buffer( teb, buffer, offset, length );
    if (!buffer || !buffer->vk_memory) funcs->p_glFlushMappedNamedBufferRange( name, offset, length );
    pthread_mutex_unlock( &wgl_lock );
}

void wow64_glFlushMappedNamedBufferRangeEXT( TEB *teb, GLuint name, GLintptr offset, GLsizeiptr length )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct buffer *buffer;

    pthread_mutex_lock( &wgl_lock );
    if ((buffer = get_named_buffer( teb, name ))) flush_buffer( teb, buffer, offset, length );
    if (!buffer || !buffer->vk_memory) funcs->p_glFlushMappedNamedBufferRangeEXT( name, offset, length );
    pthread_mutex_unlock( &wgl_lock );
}

NTSTATUS wow64_thread_attach( void *args )
{
    return thread_attach( get_teb64( (ULONG_PTR)args ));
}

NTSTATUS wow64_process_detach( void *args )
{
    NTSTATUS status;

    if ((status = process_detach( NULL ))) return status;

    free( wow64_strings );
    wow64_strings = NULL;
    wow64_strings_count = 0;

    return STATUS_SUCCESS;
}

NTSTATUS wow64_get_pixel_formats( void *args )
{
    struct
    {
        PTR32 teb;
        PTR32 hdc;
        PTR32 formats;
        UINT max_formats;
        UINT num_formats;
        UINT num_onscreen_formats;
    } *params32 = args;
    struct get_pixel_formats_params params =
    {
        .teb = get_teb64(params32->teb),
        .hdc = ULongToPtr(params32->hdc),
        .formats = ULongToPtr(params32->formats),
        .max_formats = params32->max_formats,
    };
    NTSTATUS status;
    status = get_pixel_formats( &params );
    params32->num_formats = params.num_formats;
    params32->num_onscreen_formats = params.num_onscreen_formats;
    return status;
}

#endif
