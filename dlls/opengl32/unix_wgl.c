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

struct color_buffer_state
{
    GLfloat clear_color[4];
};

struct hint_state
{
    GLenum perspective_correction;
};

struct opengl_context
{
    HDC hdc;                     /* context creation DC */
    HGLRC share;                 /* context to be shared with */
    int *attribs;                /* creation attributes */
    DWORD tid;                   /* thread that the context is current in */
    UINT64 debug_callback;       /* client pointer */
    UINT64 debug_user;           /* client pointer */
    GLubyte *extensions;         /* extension string */
    GLuint *disabled_exts;       /* indices of disabled extensions */
    struct wgl_context *drv_ctx; /* driver context */
    GLubyte *wow64_version;      /* wow64 GL version override */

    /* semi-stub state tracker for wglCopyContext */
    GLbitfield used;                            /* context state used bits */
    struct lighting_state lighting;             /* GL_LIGHTING_BIT */
    struct depth_buffer_state depth_buffer;     /* GL_DEPTH_BUFFER_BIT */
    struct viewport_state viewport;             /* GL_VIEWPORT_BIT */
    struct enable_state enable;                 /* GL_ENABLE_BIT */
    struct color_buffer_state color_buffer;     /* GL_COLOR_BUFFER_BIT */
    struct hint_state hint;                     /* GL_HINT_BIT */
};

struct wgl_handle
{
    UINT handle;
    const struct opengl_funcs *funcs;
    union
    {
        struct opengl_context *context; /* for HANDLE_CONTEXT */
        struct wgl_pbuffer *pbuffer;    /* for HANDLE_PBUFFER */
        GLsync sync;                    /* for HANDLE_GLSYNC */
        struct wgl_handle *next;        /* for free handles */
    } u;
};

#define MAX_WGL_HANDLES 1024
static struct wgl_handle wgl_handles[MAX_WGL_HANDLES];
static struct wgl_handle *next_free;
static unsigned int handle_count;

/* the current context is assumed valid and doesn't need locking */
static struct opengl_context *get_current_context( TEB *teb )
{
    if (!teb->glCurrentRC) return NULL;
    return wgl_handles[LOWORD(teb->glCurrentRC) & ~HANDLE_TYPE_MASK].u.context;
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
#define CONTEXT_ATTRIBUTE_DESC(bit, name, field) [name] = { bit, offsetof(struct opengl_context, field), sizeof(((struct opengl_context *)0)->field) }
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
    struct opengl_context *ctx = get_current_context( teb );
    GLbitfield bit;

    if (!(ctx = get_current_context( teb ))) return;

    if (name >= ARRAY_SIZE(context_attributes) || !(bit = context_attributes[name].bit)) bit = -1 /* unsupported */;
    else if (size && size != context_attributes[name].size) ERR( "Invalid state attrib %#x parameter size %#zx\n", name, size );
    else memcpy( (char *)ctx + context_attributes[name].offset, value, context_attributes[name].size );

    if (bit == -1 && ctx->used != -1) WARN( "Unsupported attribute on context %p/%p\n", teb->glCurrentRC, ctx );
    ctx->used |= bit;
}

static BOOL copy_context_attributes( TEB *teb, const struct opengl_funcs *funcs, HGLRC dst_handle, struct opengl_context *dst,
                                     HGLRC src_handle, struct opengl_context *src, GLbitfield mask )
{
    HDC draw_hdc = teb->glReserved1[0], read_hdc = teb->glReserved1[1];
    struct opengl_context *old_ctx = get_current_context( teb );
    const struct opengl_funcs *old_funcs = teb->glTable;

    if (dst == old_ctx)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (!mask) return TRUE;
    if (src->used == -1) FIXME( "Unsupported attributes on context %p/%p\n", src_handle, src );
    if (src != dst && dst->used == -1) FIXME( "Unsupported attributes on context %p/%p\n", dst_handle, dst );

    funcs->p_wglMakeCurrent( dst->hdc, dst->drv_ctx );

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
    else if (!old_funcs->p_wglMakeContextCurrentARB) old_funcs->p_wglMakeCurrent( draw_hdc, old_ctx->drv_ctx );
    else old_funcs->p_wglMakeContextCurrentARB( draw_hdc, read_hdc, old_ctx->drv_ctx );

    return dst->used != -1 && src->used != -1;
}

static struct opengl_context *opengl_context_from_handle( TEB *teb, HGLRC handle, const struct opengl_funcs **funcs );

/* update handle context if it has been re-shared with another one */
static void update_handle_context( TEB *teb, HGLRC handle, struct wgl_handle *ptr )
{
    struct opengl_context *ctx = ptr->u.context, *shared;
    const struct opengl_funcs *funcs = ptr->funcs, *share_funcs;
    struct wgl_context *(*func)( HDC hDC, struct wgl_context * hShareContext, const int *attribList );
    struct wgl_context *prev = ctx->drv_ctx, *drv_ctx;

    if (ctx->tid) return; /* currently in use */
    if (ctx->share == (HGLRC)-1) return; /* not re-shared */
    if (!(func = funcs->p_wglCreateContextAttribsARB)) func = (void *)funcs->p_wglGetProcAddress( "wglCreateContextAttribsARB" );
    if (!func) return; /* not supported */

    shared = ctx->share ? opengl_context_from_handle( teb, ctx->share, &share_funcs ) : NULL;
    if (!(drv_ctx = func( ctx->hdc, shared ? shared->drv_ctx : NULL, ctx->attribs )))
    {
        WARN( "Failed to re-create context for wglShareLists\n" );
        return;
    }
    ctx->share = (HGLRC)-1; /* initial shared context */
    ctx->drv_ctx = drv_ctx;
    copy_context_attributes( teb, funcs, handle, ctx, handle, ctx, ctx->used );
    funcs->p_wglDeleteContext( prev );
}

static struct opengl_context *opengl_context_from_handle( TEB *teb, HGLRC handle, const struct opengl_funcs **funcs )
{
    struct wgl_handle *entry;
    if (!(entry = get_handle_ptr( handle ))) return NULL;
    update_handle_context( teb, handle, entry );
    *funcs = entry->funcs;
    return entry->u.context;
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

static GLubyte *filter_extensions_list( const char *extensions, const char *disabled, const char *enabled )
{
    const char *end, **extra;
    char *p, *str;
    size_t size, len;

    size = strlen( extensions ) + 2;
    for (extra = legacy_extensions; *extra; extra++) size += strlen( *extra ) + 1;
    if (!(p = str = malloc( size ))) return NULL;

    TRACE( "GL_EXTENSIONS:\n" );

    for (;;)
    {
        while (*extensions == ' ') extensions++;
        if (!*extensions) break;

        if (!(end = strchr( extensions, ' ' ))) end = extensions + strlen( extensions );
        memcpy( p, extensions, end - extensions );
        len = end - extensions;
        p[len] = 0;

        /* We do not support GL_MAP_PERSISTENT_BIT, and hence
         * ARB_buffer_storage, on wow64. */
        if (is_win64 && is_wow64() && (!strcmp( p, "GL_ARB_buffer_storage" ) || !strcmp( p, "GL_EXT_buffer_storage" )))
        {
            TRACE( "-- %s (disabled due to wow64)\n", p );
        }
        else if (!has_extension( disabled, p, len ) && (!*enabled || has_extension( enabled, p, len )))
        {
            TRACE( "++ %s\n", p );
            p += end - extensions;
            *p++ = ' ';
        }
        else
        {
            TRACE( "-- %s (disabled by config)\n", p );
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

static GLuint *filter_extensions_index( TEB *teb, const char *disabled, const char *enabled )
{
    const struct opengl_funcs *funcs = teb->glTable;
    const char *ext, *version;
    GLuint *disabled_index;
    GLint extensions_count;
    unsigned int i = 0, j, len;
    int major, minor;

    if (!funcs->p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->p_glGetStringi;
        *func_ptr = funcs->p_wglGetProcAddress( "glGetStringi" );
        if (!funcs->p_glGetStringi) return NULL;
    }

    version = (const char *)funcs->p_glGetString( GL_VERSION );
    parse_gl_version( version, &major, &minor );
    if (major < 3)
        return NULL;

    funcs->p_glGetIntegerv( GL_NUM_EXTENSIONS, &extensions_count );
    disabled_index = malloc( extensions_count * sizeof(*disabled_index) );
    if (!disabled_index) return NULL;

    TRACE( "GL_EXTENSIONS:\n" );

    for (j = 0; j < extensions_count; ++j)
    {
        ext = (const char *)funcs->p_glGetStringi( GL_EXTENSIONS, j );
        len = strlen( ext );

        /* We do not support GL_MAP_PERSISTENT_BIT, and hence
         * ARB_buffer_storage, on wow64. */
        if (is_win64 && is_wow64() && (!strcmp( ext, "GL_ARB_buffer_storage" ) || !strcmp( ext, "GL_EXT_buffer_storage" )))
        {
            TRACE( "-- %s (disabled due to wow64)\n", ext );
            disabled_index[i++] = j;
        }
        else if (!has_extension( disabled, ext, len ) && (!*enabled || has_extension( enabled, ext, len )))
        {
            TRACE( "++ %s\n", ext );
        }
        else
        {
            TRACE( "-- %s (disabled by config)\n", ext );
            disabled_index[i++] = j;
        }
    }

    disabled_index[i] = ~0u;
    return disabled_index;
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

/* build the extension string by filtering out the disabled extensions */
static BOOL filter_extensions( TEB * teb, const char *extensions, GLubyte **exts_list, GLuint **disabled_exts )
{
    static const char *disabled, *enabled;
    char *str;

    if (!disabled)
    {
        if (!(str = query_opengl_option( "DisabledExtensions" ))) disabled = "";
        else if (InterlockedCompareExchangePointer( (void **)&disabled, str, NULL )) free( str );
    }
    if (!enabled)
    {
        if (!(str = query_opengl_option( "EnabledExtensions" ))) enabled = "";
        else if (InterlockedCompareExchangePointer( (void **)&enabled, str, NULL )) free( str );
    }

    if (extensions && !*exts_list) *exts_list = filter_extensions_list( extensions, disabled, enabled );
    if (!*disabled_exts) *disabled_exts = filter_extensions_index( teb, disabled, enabled );
    return (exts_list && *exts_list) || *disabled_exts;
}

static const GLuint *disabled_extensions_index( TEB *teb )
{
    struct opengl_context *ctx = get_current_context( teb );
    GLuint **disabled = &ctx->disabled_exts;
    if (*disabled || filter_extensions( teb, NULL, NULL, disabled )) return *disabled;
    return NULL;
}

/* Check if a GL extension is supported */
static BOOL check_extension_support( TEB *teb, const char *extension, const char *available_extensions )
{
    const struct opengl_funcs *funcs = teb->glTable;
    size_t len;

    TRACE( "Checking for extension '%s'\n", extension );

    /* We use the GetProcAddress function from the display driver to retrieve function pointers
     * for OpenGL and WGL extensions. In case of winex11.drv the OpenGL extension lookup is done
     * using glXGetProcAddress. This function is quite unreliable in the sense that its specs don't
     * require the function to return NULL when an extension isn't found. For this reason we check
     * if the OpenGL extension required for the function we are looking up is supported. */

    while ((len = strcspn( extension, " " )))
    {
        /* Check if the extension is part of the GL extension string to see if it is supported. */
        if (has_extension( available_extensions, extension, len )) return TRUE;

        /* In general an OpenGL function starts as an ARB/EXT extension and at some stage
         * it becomes part of the core OpenGL library and can be reached without the ARB/EXT
         * suffix as well. In the extension table, these functions contain GL_VERSION_major_minor.
         * Check if we are searching for a core GL function */
        if (!strncmp( extension, "GL_VERSION_", 11 ))
        {
            const GLubyte *gl_version = funcs->p_glGetString( GL_VERSION );
            const char *version = extension + 11; /* Move past 'GL_VERSION_' */

            if (!gl_version)
            {
                ERR( "No OpenGL version found!\n" );
                return FALSE;
            }

            /* Compare the major/minor version numbers of the native OpenGL library and what is required by the function.
             * The gl_version string is guaranteed to have at least a major/minor and sometimes it has a release number as well. */
            if ((gl_version[0] > version[0]) || ((gl_version[0] == version[0]) && (gl_version[2] >= version[2]))) return TRUE;

            WARN( "The function requires OpenGL version '%c.%c' while your drivers only provide '%c.%c'\n",
                  version[0], version[2], gl_version[0], gl_version[2] );
        }

        if (extension[len] == ' ') len++;
        extension += len;
    }

    return FALSE;
}

static BOOL get_integer( TEB *teb, GLenum pname, GLint *data )
{
    const struct opengl_funcs *funcs = teb->glTable;
    const GLuint *disabled;

    if (pname == GL_NUM_EXTENSIONS)
    {
        funcs->p_glGetIntegerv( pname, data );
        if ((disabled = disabled_extensions_index( teb )))
            while (*disabled++ != ~0u) (*data)--;
        *data += ARRAY_SIZE(legacy_extensions) - 1;
        return TRUE;
    }

    if (is_win64 && is_wow64())
    {
        /* 4.4 depends on ARB_buffer_storage, which we don't support on wow64. */
        if (pname == GL_MAJOR_VERSION)
        {
            funcs->p_glGetIntegerv( pname, data );
            if (*data > 4) *data = 4;
            return TRUE;
        }
        if (pname == GL_MINOR_VERSION)
        {
            GLint major;
            funcs->p_glGetIntegerv( GL_MAJOR_VERSION, &major );
            funcs->p_glGetIntegerv( pname, data );
            if (major == 4 && *data > 3) *data = 3;
            return TRUE;
        }
    }

    return FALSE;
}

const GLubyte *wrap_glGetString( TEB *teb, GLenum name )
{
    const struct opengl_funcs *funcs = teb->glTable;
    const GLubyte *ret;

    if ((ret = funcs->p_glGetString( name )))
    {
        if (name == GL_EXTENSIONS)
        {
            struct opengl_context *ctx = get_current_context( teb );
            GLubyte **extensions = &ctx->extensions;
            GLuint **disabled = &ctx->disabled_exts;
            if (*extensions || filter_extensions( teb, (const char *)ret, extensions, disabled )) return *extensions;
        }
        else if (name == GL_VERSION && is_win64 && is_wow64())
        {
            struct opengl_context *ctx = get_current_context( teb );
            GLubyte **str = &ctx->wow64_version;
            int major, minor;

            if (!*str)
            {
                const char *rest = parse_gl_version( (const char *)ret, &major, &minor );
                /* 4.4 depends on ARB_buffer_storage, which we don't support on wow64. */
                if (major > 4 || (major == 4 && minor >= 4)) asprintf( (char **)str, "4.3%s", rest );
                else *str = (GLubyte *)strdup( (char *)ret );
            }

            return *str;
        }
    }

    return ret;
}

const GLubyte *wrap_glGetStringi( TEB *teb, GLenum name, GLuint index )
{
    const struct opengl_funcs *funcs = teb->glTable;
    const GLuint *disabled;
    GLint count;

    if (!funcs->p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->p_glGetStringi;
        *func_ptr = funcs->p_wglGetProcAddress( "glGetStringi" );
    }

    if (name == GL_EXTENSIONS)
    {
        funcs->p_glGetIntegerv( GL_NUM_EXTENSIONS, &count );

        if ((disabled = disabled_extensions_index( teb )))
            while (index >= *disabled++) index++;

        if (index >= count && index - count < ARRAY_SIZE(legacy_extensions))
            return (const GLubyte *)legacy_extensions[index - count];
    }

    return funcs->p_glGetStringi( name, index );
}

static char *build_extension_list( TEB *teb )
{
    GLint len = 0, capacity, i, extensions_count;
    char *extension, *tmp, *available_extensions;

    get_integer( teb, GL_NUM_EXTENSIONS, &extensions_count );
    capacity = 128 * extensions_count;

    if (!(available_extensions = malloc( capacity ))) return NULL;
    for (i = 0; i < extensions_count; ++i)
    {
        extension = (char *)wrap_glGetStringi( teb, GL_EXTENSIONS, i );
        capacity = max( capacity, len + strlen( extension ) + 2 );
        if (!(tmp = realloc( available_extensions, capacity ))) break;
        available_extensions = tmp;
        len += snprintf( available_extensions + len, capacity - len, "%s ", extension );
    }
    if (len) available_extensions[len - 1] = 0;

    return available_extensions;
}

static UINT get_context_major_version( TEB *teb )
{
    struct opengl_context *ctx;

    if (!(ctx = get_current_context( teb ))) return TRUE;
    for (const int *attr = ctx->attribs; attr && attr[0]; attr += 2)
        if (attr[0] == WGL_CONTEXT_MAJOR_VERSION_ARB) return attr[1];

    return 1;
}

/* Check if a GL extension is supported */
static BOOL is_extension_supported( TEB *teb, const char *extension )
{
    char *available_extensions = NULL;
    BOOL ret = FALSE;

    if (get_context_major_version( teb ) < 3) available_extensions = strdup( (const char *)wrap_glGetString( teb, GL_EXTENSIONS ) );
    if (!available_extensions) available_extensions = build_extension_list( teb );

    if (!available_extensions) ERR( "No OpenGL extensions found, check if your OpenGL setup is correct!\n" );
    else ret = check_extension_support( teb, extension, available_extensions );

    free( available_extensions );
    return ret;
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

    /* Without an active context opengl32 doesn't know to what
     * driver it has to dispatch wglGetProcAddress.
     */
    if (!get_current_context( teb ))
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

        if (!is_extension_supported( teb, found->extension ))
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
    struct opengl_context *src, *dst;
    BOOL ret = FALSE;

    if (!(src = opengl_context_from_handle( teb, hglrcSrc, &src_funcs ))) return FALSE;
    if (!(dst = opengl_context_from_handle( teb, hglrcDst, &dst_funcs ))) return FALSE;
    else ret = copy_context_attributes( teb, dst_funcs, hglrcDst, dst, hglrcSrc, src, mask );

    return ret;
}

HGLRC wrap_wglCreateContext( TEB *teb, HDC hdc )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct opengl_context *context;
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (!(drv_ctx = funcs->p_wglCreateContext( hdc ))) return 0;
    if ((context = calloc( 1, sizeof(*context) )))
    {
        context->hdc = hdc;
        context->share = (HGLRC)-1; /* initial shared context */
        context->drv_ctx = drv_ctx;
        if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context ))) free( context );
    }
    if (!ret) funcs->p_wglDeleteContext( drv_ctx );
    return ret;
}

BOOL wrap_wglMakeCurrent( TEB *teb, HDC hdc, HGLRC hglrc )
{
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);
    struct opengl_context *ctx, *prev = get_current_context( teb );
    const struct opengl_funcs *funcs = teb->glTable;

    if (hglrc)
    {
        if (!(ctx = opengl_context_from_handle( teb, hglrc, &funcs ))) return FALSE;
        if (ctx->tid && ctx->tid != tid)
        {
            RtlSetLastWin32Error( ERROR_BUSY );
            return FALSE;
        }

        if (!funcs->p_wglMakeCurrent( hdc, ctx->drv_ctx )) return FALSE;
        if (prev) prev->tid = 0;
        ctx->tid = tid;
        teb->glReserved1[0] = hdc;
        teb->glReserved1[1] = hdc;
        teb->glCurrentRC = hglrc;
        teb->glTable = (void *)funcs;
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

BOOL wrap_wglDeleteContext( TEB *teb, HGLRC hglrc )
{
    struct wgl_handle *ptr;
    struct opengl_context *ctx;
    DWORD tid = HandleToULong(teb->ClientId.UniqueThread);

    if (!(ptr = get_handle_ptr( hglrc ))) return FALSE;
    ctx = ptr->u.context;
    if (ctx->tid && ctx->tid != tid)
    {
        RtlSetLastWin32Error( ERROR_BUSY );
        return FALSE;
    }
    if (hglrc == teb->glCurrentRC) wrap_wglMakeCurrent( teb, 0, 0 );
    ptr->funcs->p_wglDeleteContext( ctx->drv_ctx );
    free( ctx->wow64_version );
    free( ctx->disabled_exts );
    free( ctx->extensions );
    free( ctx->attribs );
    free( ctx );
    free_handle_ptr( ptr );
    return TRUE;
}

static void flush_context( TEB *teb, void (*flush)(void) )
{
    struct opengl_context *ctx = get_current_context( teb );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!ctx || !funcs->p_wgl_context_flush( ctx->drv_ctx, flush ))
    {
        /* default implementation: call the functions directly */
        if (flush) flush();
    }
}

void wrap_glFinish( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, funcs->p_glFinish );
}

void wrap_glFlush( TEB *teb )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, funcs->p_glFlush );
}

void wrap_glClear( TEB *teb, GLbitfield mask )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glClear( mask );
}

void wrap_glDrawPixels( TEB *teb, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels )
{
    const struct opengl_funcs *funcs = teb->glTable;
    flush_context( teb, NULL );
    funcs->p_glDrawPixels( width, height, format, type, pixels );
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
}

BOOL wrap_wglSwapBuffers( TEB *teb, HDC hdc )
{
    const struct opengl_funcs *funcs = get_dc_funcs( hdc );
    BOOL ret;

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
    struct opengl_context *src, *dst;
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
    struct wgl_context *drv_ctx;
    struct opengl_context *context, *share_ctx = NULL;
    const struct opengl_funcs *funcs = get_dc_funcs( hdc ), *share_funcs;

    if (!funcs)
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return 0;
    }
    if (!funcs->p_wglCreateContextAttribsARB) return 0;
    if (share && !(share_ctx = opengl_context_from_handle( teb, share, &share_funcs )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_OPERATION );
        return 0;
    }
    if ((drv_ctx = funcs->p_wglCreateContextAttribsARB( hdc, share_ctx ? share_ctx->drv_ctx : NULL, attribs )))
    {
        if ((context = calloc( 1, sizeof(*context) )))
        {
            context->hdc = hdc;
            context->share = (HGLRC)-1; /* initial shared context */
            context->attribs = memdup_attribs( attribs );
            context->drv_ctx = drv_ctx;
            if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context ))) free( context );
        }
        if (!ret) funcs->p_wglDeleteContext( drv_ctx );
    }

    return ret;
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
    struct opengl_context *ctx, *prev = get_current_context( teb );
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
        if (!funcs->p_wglMakeContextCurrentARB( draw_hdc, read_hdc, ctx->drv_ctx )) return FALSE;
        if (prev) prev->tid = 0;
        ctx->tid = tid;
        teb->glReserved1[0] = draw_hdc;
        teb->glReserved1[1] = read_hdc;
        teb->glCurrentRC = hglrc;
        teb->glTable = (void *)funcs;
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
    struct opengl_context *ctx = (struct opengl_context *)user;
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
    struct opengl_context *ctx = get_current_context( teb );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallback) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallback( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
}

void wrap_glDebugMessageCallbackAMD( TEB *teb, GLDEBUGPROCAMD callback, void *user )
{
    struct opengl_context *ctx = get_current_context( teb );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallbackAMD) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallbackAMD( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
}

void wrap_glDebugMessageCallbackARB( TEB *teb, GLDEBUGPROCARB callback, const void *user )
{
    struct opengl_context *ctx = get_current_context( teb );
    const struct opengl_funcs *funcs = teb->glTable;

    if (!funcs->p_glDebugMessageCallbackARB) return;

    ctx->debug_callback = (UINT_PTR)callback;
    ctx->debug_user     = (UINT_PTR)user;
    funcs->p_glDebugMessageCallbackARB( gl_debug_message_callback, ctx );
    set_context_attribute( teb, -1 /* unsupported */, NULL, 0 );
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

NTSTATUS process_attach( void *args )
{
    struct process_attach_params *params = args;
    call_gl_debug_message_callback = params->call_gl_debug_message_callback;
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

GLenum wow64_glClientWaitSync( TEB *teb, GLsync sync, GLbitfield flags, GLuint64 timeout )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if (!(handle = get_handle_ptr( sync ))) return GL_INVALID_VALUE;
    return funcs->p_glClientWaitSync( handle->u.sync, flags, timeout );
}

void wow64_glDeleteSync( TEB *teb, GLsync sync )
{
    const struct opengl_funcs *funcs = teb->glTable;
    struct wgl_handle *handle;

    if ((handle = get_handle_ptr( sync )))
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

    if ((handle = get_handle_ptr( sync ))) funcs->p_glGetSynciv( handle->u.sync, pname, count, length, values );
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

    if ((handle = get_handle_ptr( sync ))) funcs->p_glWaitSync( handle->u.sync, flags, timeout );
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

static void *get_buffer_pointer( TEB *teb, GLenum target )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glGetBufferPointerv) *func;
    void *ptr = NULL;
    if (!(func = funcs->p_glGetBufferPointerv)) func = (void *)funcs->p_wglGetProcAddress( "glGetBufferPointerv" );
    if (func) func( target, GL_BUFFER_MAP_POINTER, &ptr );
    return ptr;
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

static void *get_named_buffer_pointer( TEB *teb, GLint buffer )
{
    const struct opengl_funcs *funcs = teb->glTable;
    typeof(*funcs->p_glGetNamedBufferPointerv) *func;
    void *ptr = NULL;
    if (!(func = funcs->p_glGetNamedBufferPointerv)) func = (void *)funcs->p_wglGetProcAddress( "glGetNamedBufferPointerv" );
    if (func) func( buffer, GL_BUFFER_MAP_POINTER, &ptr );
    return ptr;
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

static NTSTATUS wow64_map_buffer( TEB *teb, GLint buffer, GLenum target, void *ptr, SIZE_T size,
                                  GLbitfield access, PTR32 *ret )
{
    static unsigned int once;

    if (*ret)  /* wow64 pointer provided, map buffer to it */
    {
        if (!(access & (GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)))
        {
            if (!once++)
                FIXME( "Doing a copy of a mapped buffer (expect performance issues)\n" );

            TRACE( "Copying %#zx from buffer at %p to wow64 buffer %p\n", size, ptr, UlongToPtr(*ret) );
            memcpy( UlongToPtr(*ret), ptr, size );
        }

        /* save the wow64 pointer in the buffer data, we'll overwrite it on unmap */
        *(PTR32 *)ptr = (UINT_PTR)*ret;
        return STATUS_SUCCESS;
    }

    if (ULongToPtr(*ret = PtrToUlong(ptr)) == ptr) return STATUS_SUCCESS;  /* we're lucky */
    if (access & GL_MAP_PERSISTENT_BIT)
    {
        FIXME( "GL_MAP_PERSISTENT_BIT not supported!\n" );
        return STATUS_NOT_SUPPORTED;
    }

    if (!size) size = buffer ? get_named_buffer_param( teb, buffer, GL_BUFFER_SIZE ) : get_buffer_param( teb, target, GL_BUFFER_SIZE );
    if ((PTR32)size != size) return STATUS_NO_MEMORY;  /* overflow */
    if (size < sizeof(PTR32))
    {
        FIXME( "Buffer too small for metadata!\n" );
        return STATUS_BUFFER_TOO_SMALL;
    }

    *ret = size;
    return STATUS_INVALID_ADDRESS;
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

static PTR32 wow64_unmap_buffer( void *ptr, SIZE_T size, GLbitfield access )
{
    void *wow_ptr;

    if (ULongToPtr(PtrToUlong(ptr)) == ptr) return 0;  /* we're lucky */

    wow_ptr = UlongToPtr(*(PTR32 *)ptr);
    if (access & GL_MAP_WRITE_BIT)
    {
        TRACE( "Copying %#zx from wow64 buffer %p to buffer %p\n", size, wow_ptr, ptr );
        memcpy( ptr, wow_ptr, size );
    }

    return PtrToUlong( wow_ptr );
}

static void wow64_gl_get_buffer_pointer_v( GLenum pname, PTR32 *ptr, PTR32 *wow_ptr )
{
    if (pname != GL_BUFFER_MAP_POINTER) return;
    if (ULongToPtr(*wow_ptr = PtrToUlong(ptr)) == ptr) return;  /* we're lucky */
    *wow_ptr = ptr[0];
}

void wow64_glGetBufferPointerv( TEB *teb, GLenum target, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    funcs->p_glGetBufferPointerv( target, pname, &ptr );
    return wow64_gl_get_buffer_pointer_v( pname, ptr, params );
}

void wow64_glGetBufferPointervARB( TEB *teb, GLenum target, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    funcs->p_glGetBufferPointervARB( target, pname, &ptr );
    return wow64_gl_get_buffer_pointer_v( pname, ptr, params );
}

void wow64_glGetNamedBufferPointerv( TEB *teb, GLuint buffer, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    funcs->p_glGetNamedBufferPointerv( buffer, pname, &ptr );
    return wow64_gl_get_buffer_pointer_v( pname, ptr, params );
}

void wow64_glGetNamedBufferPointervEXT( TEB *teb, GLuint buffer, GLenum pname, PTR32 *params )
{
    const struct opengl_funcs *funcs = teb->glTable;
    void *ptr;
    funcs->p_glGetNamedBufferPointervEXT( buffer, pname, &ptr );
    return wow64_gl_get_buffer_pointer_v( pname, ptr, params );
}

static PTR32 wow64_gl_map_buffer( TEB *teb, GLenum target, GLenum access, PTR32 *client_ptr,
                                  PFN_glMapBuffer gl_map_buffer64 )
{
    NTSTATUS status;
    void *ptr;

    /* if *ret, we're being called again with a wow64 pointer */
    ptr = *client_ptr ? get_buffer_pointer( teb, target ) : gl_map_buffer64( target, access );

    status = wow64_map_buffer( teb, 0, target, ptr, 0, map_range_flags_from_map_flags( access ), client_ptr );
    if (!status) return *client_ptr;
    if (status != STATUS_INVALID_ADDRESS) unmap_buffer( teb, target );
    return 0;
}

PTR32 wow64_glMapBuffer( TEB *teb, GLenum target, GLenum access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_buffer( teb, target, access, client_ptr, funcs->p_glMapBuffer );
}

PTR32 wow64_glMapBufferARB( TEB *teb, GLenum target, GLenum access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_buffer( teb, target, access, client_ptr, funcs->p_glMapBufferARB );
}

PTR32 wow64_glMapBufferRange( TEB *teb, GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    NTSTATUS status;
    void *ptr;

    /* already mapped, we're being called again with a wow64 pointer */
    if (*client_ptr) ptr = (char *)get_buffer_pointer( teb, target );
    else ptr = funcs->p_glMapBufferRange( target, offset, length, access );

    status = wow64_map_buffer( teb, 0, target, ptr, length, access, client_ptr );
    if (!status) return *client_ptr;
    if (status != STATUS_INVALID_ADDRESS) unmap_buffer( teb, target );
    return 0;
}

static PTR32 wow64_gl_map_named_buffer( TEB *teb, GLuint buffer, GLenum access, PTR32 *client_ptr,
                                        PFN_glMapNamedBuffer gl_map_named_buffer64 )
{
    NTSTATUS status;
    void *ptr;

    /* already mapped, we're being called again with a wow64 pointer */
    if (*client_ptr) ptr = get_named_buffer_pointer( teb, buffer );
    else ptr = gl_map_named_buffer64( buffer, access );

    status = wow64_map_buffer( teb, buffer, 0, ptr, 0, map_range_flags_from_map_flags( access ), client_ptr );
    if (!status) return *client_ptr;
    if (status != STATUS_INVALID_ADDRESS) unmap_named_buffer( teb, buffer );
    return 0;
}

PTR32 wow64_glMapNamedBuffer( TEB *teb, GLuint buffer, GLenum access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer( teb, buffer, access, client_ptr, funcs->p_glMapNamedBuffer );
}

PTR32 wow64_glMapNamedBufferEXT( TEB *teb, GLuint buffer, GLenum access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer( teb, buffer, access, client_ptr, funcs->p_glMapNamedBufferEXT );
}

static NTSTATUS wow64_gl_map_named_buffer_range( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access,
                                                 PTR32 *client_ptr, PFN_glMapNamedBufferRange gl_map_named_buffer_range64 )
{
    NTSTATUS status;
    void *ptr;

    /* already mapped, we're being called again with a wow64 pointer */
    if (*client_ptr) ptr = get_named_buffer_pointer( teb, buffer );
    else ptr = gl_map_named_buffer_range64( buffer, offset, length, access );

    status = wow64_map_buffer( teb, buffer, 0, ptr, length, access, client_ptr );
    if (!status) return *client_ptr;
    if (status != STATUS_INVALID_ADDRESS)  unmap_named_buffer( teb, buffer );
    return 0;
}

PTR32 wow64_glMapNamedBufferRange( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer_range( teb, buffer, offset, length, access, client_ptr, funcs->p_glMapNamedBufferRange );
}

PTR32 wow64_glMapNamedBufferRangeEXT( TEB *teb, GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    return wow64_gl_map_named_buffer_range( teb, buffer, offset, length, access, client_ptr, funcs->p_glMapNamedBufferRangeEXT );
}

static PTR32 wow64_unmap_client_buffer( TEB *teb, GLenum target )
{
    PTR32 *ptr;

    if (!(ptr = get_buffer_pointer( teb, target ))) return 0;
    return wow64_unmap_buffer( ptr, get_buffer_param( teb, target, GL_BUFFER_MAP_LENGTH ),
                               get_buffer_param( teb, target, GL_BUFFER_ACCESS_FLAGS ) );
}

GLboolean wow64_glUnmapBuffer( TEB *teb, GLenum target, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    *client_ptr = wow64_unmap_client_buffer( teb, target );
    return funcs->p_glUnmapBuffer( target );
}

GLboolean wow64_glUnmapBufferARB( TEB *teb, GLenum target, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    *client_ptr = wow64_unmap_client_buffer( teb, target );
    return funcs->p_glUnmapBuffer( target );
}

static PTR32 wow64_gl_unmap_named_buffer( TEB *teb, GLuint buffer )
{
    PTR32 *ptr;

    if (!(ptr = get_named_buffer_pointer( teb, buffer ))) return 0;
    return wow64_unmap_buffer( ptr, get_named_buffer_param( teb, buffer, GL_BUFFER_MAP_LENGTH ),
                               get_named_buffer_param( teb, buffer, GL_BUFFER_ACCESS_FLAGS ) );
}

GLboolean wow64_glUnmapNamedBuffer( TEB *teb, GLuint buffer, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    *client_ptr = wow64_gl_unmap_named_buffer( teb, buffer );
    return funcs->p_glUnmapNamedBuffer( buffer );
}

GLboolean wow64_glUnmapNamedBufferEXT( TEB *teb, GLuint buffer, PTR32 *client_ptr )
{
    const struct opengl_funcs *funcs = teb->glTable;
    *client_ptr = wow64_gl_unmap_named_buffer( teb, buffer );
    return funcs->p_glUnmapNamedBufferEXT( buffer );
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
