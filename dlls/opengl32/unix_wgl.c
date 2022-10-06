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

WINE_DEFAULT_DEBUG_CHANNEL(wgl);

static pthread_mutex_t wgl_lock = PTHREAD_MUTEX_INITIALIZER;

/* handle management */

enum wgl_handle_type
{
    HANDLE_PBUFFER = 0 << 12,
    HANDLE_CONTEXT = 1 << 12,
    HANDLE_CONTEXT_V3 = 3 << 12,
    HANDLE_TYPE_MASK = 15 << 12
};

struct opengl_context
{
    DWORD tid;                   /* thread that the context is current in */
    void (CALLBACK *debug_callback)( GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *, const void * ); /* debug callback */
    const void *debug_user;      /* debug user parameter */
    GLubyte *extensions;         /* extension string */
    GLuint *disabled_exts;       /* indices of disabled extensions */
    struct wgl_context *drv_ctx; /* driver context */
};

struct wgl_handle
{
    UINT handle;
    struct opengl_funcs *funcs;
    union
    {
        struct opengl_context *context; /* for HANDLE_CONTEXT */
        struct wgl_pbuffer *pbuffer;    /* for HANDLE_PBUFFER */
        struct wgl_handle *next;        /* for free handles */
    } u;
};

#define MAX_WGL_HANDLES 1024
static struct wgl_handle wgl_handles[MAX_WGL_HANDLES];
static struct wgl_handle *next_free;
static unsigned int handle_count;

/* the current context is assumed valid and doesn't need locking */
static inline struct wgl_handle *get_current_context_ptr(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return NULL;
    return &wgl_handles[LOWORD(NtCurrentTeb()->glCurrentRC) & ~HANDLE_TYPE_MASK];
}

static inline HANDLE next_handle( struct wgl_handle *ptr, enum wgl_handle_type type )
{
    WORD generation = HIWORD( ptr->handle ) + 1;
    if (!generation) generation++;
    ptr->handle = MAKELONG( ptr - wgl_handles, generation ) | type;
    return ULongToHandle( ptr->handle );
}

static struct wgl_handle *get_handle_ptr( HANDLE handle, enum wgl_handle_type type )
{
    unsigned int index = LOWORD( handle ) & ~HANDLE_TYPE_MASK;

    if (index < handle_count && ULongToHandle(wgl_handles[index].handle) == handle)
        return &wgl_handles[index];

    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
    return NULL;
}

static HANDLE alloc_handle( enum wgl_handle_type type, struct opengl_funcs *funcs, void *user_ptr )
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

static GLubyte *filter_extensions_list( const char *extensions, const char *disabled )
{
    const char *end;
    char *p, *str;

    p = str = malloc( strlen( extensions ) + 2 );
    if (!str) return NULL;

    TRACE( "GL_EXTENSIONS:\n" );

    for (;;)
    {
        while (*extensions == ' ') extensions++;
        if (!*extensions) break;

        if (!(end = strchr( extensions, ' ' ))) end = extensions + strlen( extensions );
        memcpy( p, extensions, end - extensions );
        p[end - extensions] = 0;

        if (!has_extension( disabled, p, strlen( p ) ))
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
    *p = 0;
    return (GLubyte *)str;
}

static GLuint *filter_extensions_index( const char *disabled )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    GLuint *disabled_index;
    GLint extensions_count;
    unsigned int i = 0, j;
    const char *ext;

    if (!funcs->ext.p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->ext.p_glGetStringi;
        *func_ptr = funcs->wgl.p_wglGetProcAddress( "glGetStringi" );
        if (!funcs->ext.p_glGetStringi) return NULL;
    }

    funcs->gl.p_glGetIntegerv( GL_NUM_EXTENSIONS, &extensions_count );
    disabled_index = malloc( extensions_count * sizeof(*disabled_index) );
    if (!disabled_index) return NULL;

    TRACE( "GL_EXTENSIONS:\n" );

    for (j = 0; j < extensions_count; ++j)
    {
        ext = (const char *)funcs->ext.p_glGetStringi( GL_EXTENSIONS, j );
        if (!has_extension( disabled, ext, strlen( ext ) ))
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
        len = sprintf( buffer, "\\Registry\\User\\S-%u-%u", sid->Revision,
                       MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5],
                                           sid->IdentifierAuthority.Value[4] ),
                                 MAKEWORD( sid->IdentifierAuthority.Value[3],
                                           sid->IdentifierAuthority.Value[2] )));
        for (i = 0; i < sid->SubAuthorityCount; i++)
            len += sprintf( buffer + len, "-%u", sid->SubAuthority[i] );

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

/* build the extension string by filtering out the disabled extensions */
static BOOL filter_extensions( const char *extensions, GLubyte **exts_list, GLuint **disabled_exts )
{
    static const char *disabled;

    if (!disabled)
    {
        char *str = NULL;
        HKEY hkey;

        /* @@ Wine registry key: HKCU\Software\Wine\OpenGL */
        if ((hkey = open_hkcu_key( "Software\\Wine\\OpenGL" )))
        {
            char buffer[4096];
            KEY_VALUE_PARTIAL_INFORMATION *value = (void *)buffer;
            static WCHAR disabled_extensionsW[] = {'D','i','s','a','b','l','e','d','E','x','t','e','n','s','i','o','n','s',0};

            if (query_reg_value( hkey, disabled_extensionsW, value, sizeof(buffer) )) str = strdup( buffer );
            NtClose( hkey );
        }
        if (str)
        {
            if (InterlockedCompareExchangePointer( (void **)&disabled, str, NULL )) free( str );
        }
        else disabled = "";
    }

    if (!disabled[0]) return FALSE;
    if (extensions && !*exts_list) *exts_list = filter_extensions_list( extensions, disabled );
    if (!*disabled_exts) *disabled_exts = filter_extensions_index( disabled );
    return (exts_list && *exts_list) || *disabled_exts;
}

static const GLuint *disabled_extensions_index(void)
{
    struct wgl_handle *ptr = get_current_context_ptr();
    GLuint **disabled = &ptr->u.context->disabled_exts;
    if (*disabled || filter_extensions( NULL, NULL, disabled )) return *disabled;
    return NULL;
}

/* Check if a GL extension is supported */
static BOOL check_extension_support( const char *extension, const char *available_extensions )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
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
            const GLubyte *gl_version = funcs->gl.p_glGetString( GL_VERSION );
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

static void WINAPI wrap_glGetIntegerv( GLenum pname, GLint *data )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const GLuint *disabled;

    funcs->gl.p_glGetIntegerv( pname, data );

    if (pname == GL_NUM_EXTENSIONS && (disabled = disabled_extensions_index()))
        while (*disabled++ != ~0u) (*data)--;
}

static const GLubyte * WINAPI wrap_glGetString( GLenum name )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const GLubyte *ret;

    if ((ret = funcs->gl.p_glGetString( name )) && name == GL_EXTENSIONS)
    {
        struct wgl_handle *ptr = get_current_context_ptr();
        GLubyte **extensions = &ptr->u.context->extensions;
        GLuint **disabled = &ptr->u.context->disabled_exts;
        if (*extensions || filter_extensions( (const char *)ret, extensions, disabled )) return *extensions;
    }

    return ret;
}

static const GLubyte * WINAPI wrap_glGetStringi( GLenum name, GLuint index )
{
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const GLuint *disabled;

    if (!funcs->ext.p_glGetStringi)
    {
        void **func_ptr = (void **)&funcs->ext.p_glGetStringi;
        *func_ptr = funcs->wgl.p_wglGetProcAddress( "glGetStringi" );
    }

    if (name == GL_EXTENSIONS && (disabled = disabled_extensions_index()))
        while (index >= *disabled++) index++;

    return funcs->ext.p_glGetStringi( name, index );
}

static char *build_extension_list(void)
{
    GLint len = 0, capacity, i, extensions_count;
    char *extension, *tmp, *available_extensions;

    wrap_glGetIntegerv( GL_NUM_EXTENSIONS, &extensions_count );
    capacity = 128 * extensions_count;

    if (!(available_extensions = malloc( capacity ))) return NULL;
    for (i = 0; i < extensions_count; ++i)
    {
        extension = (char *)wrap_glGetStringi( GL_EXTENSIONS, i );
        capacity = max( capacity, len + strlen( extension ) + 2 );
        if (!(tmp = realloc( available_extensions, capacity ))) break;
        available_extensions = tmp;
        len += sprintf( available_extensions + len, "%s ", extension );
    }
    if (len) available_extensions[len - 1] = 0;

    return available_extensions;
}

static inline enum wgl_handle_type get_current_context_type(void)
{
    if (!NtCurrentTeb()->glCurrentRC) return HANDLE_CONTEXT;
    return LOWORD(NtCurrentTeb()->glCurrentRC) & HANDLE_TYPE_MASK;
}

/* Check if a GL extension is supported */
static BOOL is_extension_supported( const char *extension )
{
    enum wgl_handle_type type = get_current_context_type();
    char *available_extensions = NULL;
    BOOL ret = FALSE;

    if (type == HANDLE_CONTEXT) available_extensions = strdup( (const char *)wrap_glGetString( GL_EXTENSIONS ) );
    if (!available_extensions) available_extensions = build_extension_list();

    if (!available_extensions) ERR( "No OpenGL extensions found, check if your OpenGL setup is correct!\n" );
    else ret = check_extension_support( extension, available_extensions );

    free( available_extensions );
    return ret;
}

static int registry_entry_cmp( const void *a, const void *b )
{
    const struct registry_entry *entry_a = a, *entry_b = b;
    return strcmp( entry_a->name, entry_b->name );
}

static PROC WINAPI wrap_wglGetProcAddress( LPCSTR name )
{
    const struct registry_entry entry = {.name = name}, *found;
    struct opengl_funcs *funcs = NtCurrentTeb()->glTable;
    const void **func_ptr;

    /* Without an active context opengl32 doesn't know to what
     * driver it has to dispatch wglGetProcAddress.
     */
    if (!get_current_context_ptr())
    {
        WARN( "No active WGL context found\n" );
        return (void *)-1;
    }

    if (!(found = bsearch( &entry, extension_registry, extension_registry_size, sizeof(entry), registry_entry_cmp )))
    {
        WARN( "Function %s unknown\n", name );
        return (void *)-1;
    }

    func_ptr = (const void **)&funcs->ext + (found - extension_registry);
    if (!*func_ptr)
    {
        void *driver_func = funcs->wgl.p_wglGetProcAddress( name );

        if (!is_extension_supported( found->extension ))
        {
            unsigned int i;
            static const struct { const char *name, *alt; } alternatives[] =
            {
                { "glCopyTexSubImage3DEXT", "glCopyTexSubImage3D" },     /* needed by RuneScape */
                { "glVertexAttribDivisor", "glVertexAttribDivisorARB"},  /* needed by Caffeine */
            };

            for (i = 0; i < ARRAY_SIZE(alternatives); i++)
            {
                if (strcmp( name, alternatives[i].name )) continue;
                WARN( "Extension %s required for %s not supported, trying %s\n", found->extension,
                      name, alternatives[i].alt );
                return wrap_wglGetProcAddress( alternatives[i].alt );
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

    return (void *)(UINT_PTR)(found - extension_registry);
}

static BOOL wrap_wglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask )
{
    struct wgl_handle *src, *dst;
    BOOL ret = FALSE;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglCopyContext( src->u.context->drv_ctx, dst->u.context->drv_ctx, mask );
    }
    return ret;
}

static HGLRC wrap_wglCreateContext( HDC hdc )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs) return 0;
    if (!(drv_ctx = funcs->wgl.p_wglCreateContext( hdc ))) return 0;
    if ((context = calloc( 1, sizeof(*context) )))
    {
        context->drv_ctx = drv_ctx;
        if (!(ret = alloc_handle( HANDLE_CONTEXT, funcs, context ))) free( context );
    }
    if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
    return ret;
}

static BOOL wrap_wglMakeCurrent( HDC hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = ptr->funcs->wgl.p_wglMakeCurrent( hdc, ptr->u.context->drv_ctx );
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                NtCurrentTeb()->glReserved1[0] = hdc;
                NtCurrentTeb()->glReserved1[1] = hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
            }
        }
        else
        {
            RtlSetLastWin32Error( ERROR_BUSY );
            ret = FALSE;
        }
    }
    else if (prev)
    {
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    else if (!hdc)
    {
        RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        ret = FALSE;
    }
    return ret;
}

static BOOL wrap_wglDeleteContext( HGLRC hglrc )
{
    struct wgl_handle *ptr;

    if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
    if (ptr->u.context->tid && ptr->u.context->tid != GetCurrentThreadId())
    {
        RtlSetLastWin32Error( ERROR_BUSY );
        return FALSE;
    }
    if (hglrc == NtCurrentTeb()->glCurrentRC) wrap_wglMakeCurrent( 0, 0 );
    ptr->funcs->wgl.p_wglDeleteContext( ptr->u.context->drv_ctx );
    free( ptr->u.context->disabled_exts );
    free( ptr->u.context->extensions );
    free( ptr->u.context );
    free_handle_ptr( ptr );
    return TRUE;
}

static BOOL wrap_wglShareLists( HGLRC hglrcSrc, HGLRC hglrcDst )
{
    BOOL ret = FALSE;
    struct wgl_handle *src, *dst;

    if (!(src = get_handle_ptr( hglrcSrc, HANDLE_CONTEXT ))) return FALSE;
    if ((dst = get_handle_ptr( hglrcDst, HANDLE_CONTEXT )))
    {
        if (src->funcs != dst->funcs) RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
        else ret = src->funcs->wgl.p_wglShareLists( src->u.context->drv_ctx, dst->u.context->drv_ctx );
    }
    return ret;
}

static BOOL wrap_wglBindTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    return ptr->funcs->ext.p_wglBindTexImageARB( ptr->u.pbuffer, buffer );
}

static HGLRC wrap_wglCreateContextAttribsARB( HDC hdc, HGLRC share, const int *attribs )
{
    HGLRC ret = 0;
    struct wgl_context *drv_ctx;
    struct wgl_handle *share_ptr = NULL;
    struct opengl_context *context;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs)
    {
        RtlSetLastWin32Error( ERROR_DC_NOT_FOUND );
        return 0;
    }
    if (!funcs->ext.p_wglCreateContextAttribsARB) return 0;
    if (share && !(share_ptr = get_handle_ptr( share, HANDLE_CONTEXT )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_OPERATION );
        return 0;
    }
    if ((drv_ctx = funcs->ext.p_wglCreateContextAttribsARB( hdc, share_ptr ? share_ptr->u.context->drv_ctx : NULL, attribs )))
    {
        if ((context = calloc( 1, sizeof(*context) )))
        {
            enum wgl_handle_type type = HANDLE_CONTEXT;

            if (attribs)
            {
                while (*attribs)
                {
                    if (attribs[0] == WGL_CONTEXT_MAJOR_VERSION_ARB)
                    {
                        if (attribs[1] >= 3) type = HANDLE_CONTEXT_V3;
                        break;
                    }
                    attribs += 2;
                }
            }

            context->drv_ctx = drv_ctx;
            if (!(ret = alloc_handle( type, funcs, context ))) free( context );
        }
        if (!ret) funcs->wgl.p_wglDeleteContext( drv_ctx );
    }

    return ret;
}

static HPBUFFERARB wrap_wglCreatePbufferARB( HDC hdc, int format, int width, int height, const int *attribs )
{
    HPBUFFERARB ret;
    struct wgl_pbuffer *pbuffer;
    struct opengl_funcs *funcs = get_dc_funcs( hdc );

    if (!funcs || !funcs->ext.p_wglCreatePbufferARB) return 0;
    if (!(pbuffer = funcs->ext.p_wglCreatePbufferARB( hdc, format, width, height, attribs ))) return 0;
    ret = alloc_handle( HANDLE_PBUFFER, funcs, pbuffer );
    if (!ret) funcs->ext.p_wglDestroyPbufferARB( pbuffer );
    return ret;
}

static BOOL wrap_wglDestroyPbufferARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr;

    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    ptr->funcs->ext.p_wglDestroyPbufferARB( ptr->u.pbuffer );
    free_handle_ptr( ptr );
    return TRUE;
}

static HDC wrap_wglGetPbufferDCARB( HPBUFFERARB handle )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return 0;
    return ptr->funcs->ext.p_wglGetPbufferDCARB( ptr->u.pbuffer );
}

static BOOL wrap_wglMakeContextCurrentARB( HDC draw_hdc, HDC read_hdc, HGLRC hglrc )
{
    BOOL ret = TRUE;
    struct wgl_handle *ptr, *prev = get_current_context_ptr();

    if (hglrc)
    {
        if (!(ptr = get_handle_ptr( hglrc, HANDLE_CONTEXT ))) return FALSE;
        if (!ptr->u.context->tid || ptr->u.context->tid == GetCurrentThreadId())
        {
            ret = (ptr->funcs->ext.p_wglMakeContextCurrentARB &&
                   ptr->funcs->ext.p_wglMakeContextCurrentARB( draw_hdc, read_hdc, ptr->u.context->drv_ctx ));
            if (ret)
            {
                if (prev) prev->u.context->tid = 0;
                ptr->u.context->tid = GetCurrentThreadId();
                NtCurrentTeb()->glReserved1[0] = draw_hdc;
                NtCurrentTeb()->glReserved1[1] = read_hdc;
                NtCurrentTeb()->glCurrentRC = hglrc;
                NtCurrentTeb()->glTable = ptr->funcs;
            }
        }
        else
        {
            RtlSetLastWin32Error( ERROR_BUSY );
            ret = FALSE;
        }
    }
    else if (prev)
    {
        if (!prev->funcs->wgl.p_wglMakeCurrent( 0, NULL )) return FALSE;
        prev->u.context->tid = 0;
        NtCurrentTeb()->glCurrentRC = 0;
        NtCurrentTeb()->glTable = &null_opengl_funcs;
    }
    return ret;
}

static BOOL wrap_wglQueryPbufferARB( HPBUFFERARB handle, int attrib, int *value )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    return ptr->funcs->ext.p_wglQueryPbufferARB( ptr->u.pbuffer, attrib, value );
}

static int wrap_wglReleasePbufferDCARB( HPBUFFERARB handle, HDC hdc )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    return ptr->funcs->ext.p_wglReleasePbufferDCARB( ptr->u.pbuffer, hdc );
}

static BOOL wrap_wglReleaseTexImageARB( HPBUFFERARB handle, int buffer )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    return ptr->funcs->ext.p_wglReleaseTexImageARB( ptr->u.pbuffer, buffer );
}

static BOOL wrap_wglSetPbufferAttribARB( HPBUFFERARB handle, const int *attribs )
{
    struct wgl_handle *ptr;
    if (!(ptr = get_handle_ptr( handle, HANDLE_PBUFFER ))) return FALSE;
    return ptr->funcs->ext.p_wglSetPbufferAttribARB( ptr->u.pbuffer, attribs );
}

static void gl_debug_message_callback( GLenum source, GLenum type, GLuint id, GLenum severity,
                                       GLsizei length, const GLchar *message, const void *userParam )
{
    struct wine_gl_debug_message_params params =
    {
        .source = source,
        .type = type,
        .id = id,
        .severity = severity,
        .length = length,
        .message = message,
    };
    void *ret_ptr;
    ULONG ret_len;

    struct wgl_handle *ptr = (struct wgl_handle *)userParam;
    if (!(params.user_callback = ptr->u.context->debug_callback)) return;
    params.user_data = ptr->u.context->debug_user;

    KeUserModeCallback( NtUserCallOpenGLDebugMessageCallback, &params, sizeof(params),
                        &ret_ptr, &ret_len );
}

static void WINAPI wrap_glDebugMessageCallback( GLDEBUGPROC callback, const void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallback) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallback( gl_debug_message_callback, ptr );
}

static void WINAPI wrap_glDebugMessageCallbackAMD( GLDEBUGPROCAMD callback, void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallbackAMD) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallbackAMD( gl_debug_message_callback, ptr );
}

static void WINAPI wrap_glDebugMessageCallbackARB( GLDEBUGPROCARB callback, const void *userParam )
{
    struct wgl_handle *ptr = get_current_context_ptr();
    const struct opengl_funcs *funcs = NtCurrentTeb()->glTable;

    if (!funcs->ext.p_glDebugMessageCallbackARB) return;

    ptr->u.context->debug_callback = callback;
    ptr->u.context->debug_user     = userParam;
    funcs->ext.p_glDebugMessageCallbackARB( gl_debug_message_callback, ptr );
}

NTSTATUS wgl_wglCopyContext( void *args )
{
    struct wglCopyContext_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglCopyContext( params->hglrcSrc, params->hglrcDst, params->mask );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglCreateContext( void *args )
{
    struct wglCreateContext_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglCreateContext( params->hDc );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglDeleteContext( void *args )
{
    struct wglDeleteContext_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglDeleteContext( params->oldContext );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglGetProcAddress( void *args )
{
    struct wglGetProcAddress_params *params = args;
    params->ret = wrap_wglGetProcAddress( params->lpszProc );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglMakeCurrent( void *args )
{
    struct wglMakeCurrent_params *params = args;
    if (params->newContext) pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglMakeCurrent( params->hDc, params->newContext );
    if (params->newContext) pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS wgl_wglShareLists( void *args )
{
    struct wglShareLists_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglShareLists( params->hrcSrvShare, params->hrcSrvSource );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS gl_glGetIntegerv( void *args )
{
    struct glGetIntegerv_params *params = args;
    wrap_glGetIntegerv( params->pname, params->data );
    return STATUS_SUCCESS;
}

NTSTATUS gl_glGetString( void *args )
{
    struct glGetString_params *params = args;
    params->ret = wrap_glGetString( params->name );
    return STATUS_SUCCESS;
}

NTSTATUS ext_glDebugMessageCallback( void *args )
{
    struct glDebugMessageCallback_params *params = args;
    wrap_glDebugMessageCallback( params->callback, params->userParam );
    return STATUS_SUCCESS;
}

NTSTATUS ext_glDebugMessageCallbackAMD( void *args )
{
    struct glDebugMessageCallbackAMD_params *params = args;
    wrap_glDebugMessageCallbackAMD( params->callback, params->userParam );
    return STATUS_SUCCESS;
}

NTSTATUS ext_glDebugMessageCallbackARB( void *args )
{
    struct glDebugMessageCallbackARB_params *params = args;
    wrap_glDebugMessageCallbackARB( params->callback, params->userParam );
    return STATUS_SUCCESS;
}

NTSTATUS ext_glGetStringi( void *args )
{
    struct glGetStringi_params *params = args;
    params->ret = wrap_glGetStringi( params->name, params->index );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglBindTexImageARB( void *args )
{
    struct wglBindTexImageARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglBindTexImageARB( params->hPbuffer, params->iBuffer );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglCreateContextAttribsARB( void *args )
{
    struct wglCreateContextAttribsARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglCreateContextAttribsARB( params->hDC, params->hShareContext, params->attribList );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglCreatePbufferARB( void *args )
{
    struct wglCreatePbufferARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglCreatePbufferARB( params->hDC, params->iPixelFormat, params->iWidth, params->iHeight, params->piAttribList );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglDestroyPbufferARB( void *args )
{
    struct wglDestroyPbufferARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglDestroyPbufferARB( params->hPbuffer );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglGetPbufferDCARB( void *args )
{
    struct wglGetPbufferDCARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglGetPbufferDCARB( params->hPbuffer );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglMakeContextCurrentARB( void *args )
{
    struct wglMakeContextCurrentARB_params *params = args;
    if (params->hglrc) pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglMakeContextCurrentARB( params->hDrawDC, params->hReadDC, params->hglrc );
    if (params->hglrc) pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglQueryPbufferARB( void *args )
{
    struct wglQueryPbufferARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglQueryPbufferARB( params->hPbuffer, params->iAttribute, params->piValue );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglReleasePbufferDCARB( void *args )
{
    struct wglReleasePbufferDCARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglReleasePbufferDCARB( params->hPbuffer, params->hDC );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglReleaseTexImageARB( void *args )
{
    struct wglReleaseTexImageARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglReleaseTexImageARB( params->hPbuffer, params->iBuffer );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS ext_wglSetPbufferAttribARB( void *args )
{
    struct wglSetPbufferAttribARB_params *params = args;
    pthread_mutex_lock( &wgl_lock );
    params->ret = wrap_wglSetPbufferAttribARB( params->hPbuffer, params->piAttribList );
    pthread_mutex_unlock( &wgl_lock );
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI thread_attach( void *args )
{
    NtCurrentTeb()->glTable = &null_opengl_funcs;
    return STATUS_SUCCESS;
}
