/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "user_private.h"
#include "winnls.h"
#include "winver.h"
#include "wine/server.h"
#include "wine/asm.h"
#include "win.h"
#include "controls.h"
#include "winerror.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define NB_USER_HANDLES  ((LAST_USER_HANDLE - FIRST_USER_HANDLE + 1) >> 1)
#define USER_HANDLE_TO_INDEX(hwnd) ((LOWORD(hwnd) - FIRST_USER_HANDLE) >> 1)

static DWORD process_layout = ~0u;

static struct list window_surfaces = LIST_INIT( window_surfaces );

static CRITICAL_SECTION surfaces_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &surfaces_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": surfaces_section") }
};
static CRITICAL_SECTION surfaces_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/**********************************************************************/

/* helper for Get/SetWindowLong */
static inline LONG_PTR get_win_data( const void *ptr, UINT size )
{
    if (size == sizeof(WORD))
    {
        WORD ret;
        memcpy( &ret, ptr, sizeof(ret) );
        return ret;
    }
    else if (size == sizeof(DWORD))
    {
        DWORD ret;
        memcpy( &ret, ptr, sizeof(ret) );
        return ret;
    }
    else
    {
        LONG_PTR ret;
        memcpy( &ret, ptr, sizeof(ret) );
        return ret;
    }
}

/* helper for Get/SetWindowLong */
static inline void set_win_data( void *ptr, LONG_PTR val, UINT size )
{
    if (size == sizeof(WORD))
    {
        WORD newval = val;
        memcpy( ptr, &newval, sizeof(newval) );
    }
    else if (size == sizeof(DWORD))
    {
        DWORD newval = val;
        memcpy( ptr, &newval, sizeof(newval) );
    }
    else
    {
        memcpy( ptr, &val, sizeof(val) );
    }
}


static void *user_handles[NB_USER_HANDLES];

/***********************************************************************
 *           alloc_user_handle
 */
HANDLE alloc_user_handle( struct user_object *ptr, enum user_obj_type type )
{
    HANDLE handle = 0;

    SERVER_START_REQ( alloc_user_handle )
    {
        if (!wine_server_call_err( req )) handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (handle)
    {
        UINT index = USER_HANDLE_TO_INDEX( handle );

        assert( index < NB_USER_HANDLES );
        ptr->handle = handle;
        ptr->type = type;
        InterlockedExchangePointer( &user_handles[index], ptr );
    }
    return handle;
}


/***********************************************************************
 *           get_user_handle_ptr
 */
void *get_user_handle_ptr( HANDLE handle, enum user_obj_type type )
{
    struct user_object *ptr;
    WORD index = USER_HANDLE_TO_INDEX( handle );

    if (index >= NB_USER_HANDLES) return NULL;

    USER_Lock();
    if ((ptr = user_handles[index]))
    {
        if (ptr->type == type &&
            ((UINT)(UINT_PTR)ptr->handle == (UINT)(UINT_PTR)handle ||
             !HIWORD(handle) || HIWORD(handle) == 0xffff))
            return ptr;
        ptr = NULL;
    }
    else ptr = OBJ_OTHER_PROCESS;
    USER_Unlock();
    return ptr;
}


/***********************************************************************
 *           release_user_handle_ptr
 */
void release_user_handle_ptr( void *ptr )
{
    assert( ptr && ptr != OBJ_OTHER_PROCESS );
    USER_Unlock();
}


/***********************************************************************
 *           free_user_handle
 */
void *free_user_handle( HANDLE handle, enum user_obj_type type )
{
    struct user_object *ptr;
    WORD index = USER_HANDLE_TO_INDEX( handle );

    if ((ptr = get_user_handle_ptr( handle, type )) && ptr != OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( free_user_handle )
        {
            req->handle = wine_server_user_handle( handle );
            if (wine_server_call( req )) ptr = NULL;
            else InterlockedCompareExchangePointer( &user_handles[index], NULL, ptr );
        }
        SERVER_END_REQ;
        USER_Unlock();
    }
    return ptr;
}


/***********************************************************************
 *           create_window_handle
 *
 * Create a window handle with the server.
 */
static WND *create_window_handle( HWND parent, HWND owner, LPCWSTR name,
                                  HINSTANCE instance, BOOL unicode,
                                  DWORD style, DWORD ex_style )
{
    WORD index;
    WND *win;
    HWND handle = 0, full_parent = 0, full_owner = 0;
    struct tagCLASS *class = NULL;
    int extra_bytes = 0;
    DPI_AWARENESS awareness = GetAwarenessFromDpiAwarenessContext( GetThreadDpiAwarenessContext() );
    UINT dpi = 0;

    SERVER_START_REQ( create_window )
    {
        req->parent   = wine_server_user_handle( parent );
        req->owner    = wine_server_user_handle( owner );
        req->instance = wine_server_client_ptr( instance );
        req->dpi      = GetDpiForSystem();
        req->awareness = awareness;
        req->style    = style;
        req->ex_style = ex_style;
        if (!(req->atom = get_int_atom_value( name )) && name)
            wine_server_add_data( req, name, lstrlenW(name)*sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            handle      = wine_server_ptr_handle( reply->handle );
            full_parent = wine_server_ptr_handle( reply->parent );
            full_owner  = wine_server_ptr_handle( reply->owner );
            extra_bytes = reply->extra;
            dpi         = reply->dpi;
            awareness   = reply->awareness;
            class       = wine_server_get_ptr( reply->class_ptr );
        }
    }
    SERVER_END_REQ;

    if (!handle)
    {
        WARN( "error %d creating window\n", GetLastError() );
        return NULL;
    }

    if (!(win = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                           sizeof(WND) + extra_bytes - sizeof(win->wExtra) )))
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = wine_server_user_handle( handle );
            wine_server_call( req );
        }
        SERVER_END_REQ;
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    if (!parent)  /* if parent is 0 we don't have a desktop window yet */
    {
        struct user_thread_info *thread_info = get_user_thread_info();

        if (name == (LPCWSTR)DESKTOP_CLASS_ATOM)
        {
            if (!thread_info->top_window) thread_info->top_window = full_parent ? full_parent : handle;
            else assert( full_parent == thread_info->top_window );
            if (full_parent && !USER_Driver->pCreateDesktopWindow( thread_info->top_window ))
                ERR( "failed to create desktop window\n" );
            register_builtin_classes();
        }
        else  /* HWND_MESSAGE parent */
        {
            if (!thread_info->msg_window && !full_parent) thread_info->msg_window = handle;
        }
    }

    USER_Lock();

    index = USER_HANDLE_TO_INDEX(handle);
    assert( index < NB_USER_HANDLES );
    win->obj.handle = handle;
    win->obj.type   = USER_WINDOW;
    win->parent     = full_parent;
    win->owner      = full_owner;
    win->class      = class;
    win->winproc    = get_class_winproc( class );
    win->cbWndExtra = extra_bytes;
    win->dpi        = dpi;
    win->dpi_awareness = awareness;
    InterlockedExchangePointer( &user_handles[index], win );
    if (WINPROC_IsUnicode( win->winproc, unicode )) win->flags |= WIN_ISUNICODE;
    return win;
}


/***********************************************************************
 *           free_window_handle
 *
 * Free a window handle.
 */
static void free_window_handle( HWND hwnd )
{
    struct user_object *ptr;
    WORD index = USER_HANDLE_TO_INDEX(hwnd);

    if ((ptr = get_user_handle_ptr( hwnd, USER_WINDOW )) && ptr != OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = wine_server_user_handle( hwnd );
            wine_server_call( req );
            InterlockedCompareExchangePointer( &user_handles[index], NULL, ptr );
        }
        SERVER_END_REQ;
        USER_Unlock();
        HeapFree( GetProcessHeap(), 0, ptr );
    }
}


/*******************************************************************
 *           list_window_children
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
static HWND *list_window_children( HDESK desktop, HWND hwnd, LPCWSTR class, DWORD tid )
{
    HWND *list;
    int i, size = 128;
    ATOM atom = get_int_atom_value( class );

    /* empty class is not the same as NULL class */
    if (!atom && class && !class[0]) return NULL;

    for (;;)
    {
        int count = 0;

        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) break;

        SERVER_START_REQ( get_window_children )
        {
            req->desktop = wine_server_obj_handle( desktop );
            req->parent = wine_server_user_handle( hwnd );
            req->tid = tid;
            req->atom = atom;
            if (!atom && class) wine_server_add_data( req, class, lstrlenW(class)*sizeof(WCHAR) );
            wine_server_set_reply( req, list, (size-1) * sizeof(user_handle_t) );
            if (!wine_server_call( req )) count = reply->count;
        }
        SERVER_END_REQ;
        if (count && count < size)
        {
            /* start from the end since HWND is potentially larger than user_handle_t */
            for (i = count - 1; i >= 0; i--)
                list[i] = wine_server_ptr_handle( ((user_handle_t *)list)[i] );
            list[count] = 0;
            return list;
        }
        HeapFree( GetProcessHeap(), 0, list );
        if (!count) break;
        size = count + 1;  /* restart with a large enough buffer */
    }
    return NULL;
}


/*******************************************************************
 *           list_window_parents
 *
 * Build an array of all parents of a given window, starting with
 * the immediate parent. The array must be freed with HeapFree.
 */
static HWND *list_window_parents( HWND hwnd )
{
    WND *win;
    HWND current, *list;
    int i, pos = 0, size = 16, count;

    if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return NULL;

    current = hwnd;
    for (;;)
    {
        if (!(win = WIN_GetPtr( current ))) goto empty;
        if (win == WND_OTHER_PROCESS) break;  /* need to do it the hard way */
        if (win == WND_DESKTOP)
        {
            if (!pos) goto empty;
            list[pos] = 0;
            return list;
        }
        list[pos] = current = win->parent;
        WIN_ReleasePtr( win );
        if (!current) return list;
        if (++pos == size - 1)
        {
            /* need to grow the list */
            HWND *new_list = HeapReAlloc( GetProcessHeap(), 0, list, (size+16) * sizeof(HWND) );
            if (!new_list) goto empty;
            list = new_list;
            size += 16;
        }
    }

    /* at least one parent belongs to another process, have to query the server */

    for (;;)
    {
        count = 0;
        SERVER_START_REQ( get_window_parents )
        {
            req->handle = wine_server_user_handle( hwnd );
            wine_server_set_reply( req, list, (size-1) * sizeof(user_handle_t) );
            if (!wine_server_call( req )) count = reply->count;
        }
        SERVER_END_REQ;
        if (!count) goto empty;
        if (size > count)
        {
            /* start from the end since HWND is potentially larger than user_handle_t */
            for (i = count - 1; i >= 0; i--)
                list[i] = wine_server_ptr_handle( ((user_handle_t *)list)[i] );
            list[count] = 0;
            return list;
        }
        HeapFree( GetProcessHeap(), 0, list );
        size = count + 1;
        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return NULL;
    }

 empty:
    HeapFree( GetProcessHeap(), 0, list );
    return NULL;
}


/*******************************************************************
 *           send_parent_notify
 */
static void send_parent_notify( HWND hwnd, UINT msg )
{
    if ((GetWindowLongW( hwnd, GWL_STYLE ) & (WS_CHILD | WS_POPUP)) == WS_CHILD &&
        !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY))
    {
        HWND parent = GetParent(hwnd);
        if (parent && parent != GetDesktopWindow())
            SendMessageW( parent, WM_PARENTNOTIFY,
                          MAKEWPARAM( msg, GetWindowLongPtrW( hwnd, GWLP_ID )), (LPARAM)hwnd );
    }
}


/*******************************************************************
 *		update_window_state
 *
 * Trigger an update of the window's driver state and surface.
 */
void update_window_state( HWND hwnd )
{
    DPI_AWARENESS_CONTEXT context;
    RECT window_rect, client_rect, valid_rects[2];

    if (!WIN_IsCurrentThread( hwnd ))
    {
        PostMessageW( hwnd, WM_WINE_UPDATEWINDOWSTATE, 0, 0 );
        return;
    }

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));
    WIN_GetRectangles( hwnd, COORDS_PARENT, &window_rect, &client_rect );
    valid_rects[0] = valid_rects[1] = client_rect;
    set_window_pos( hwnd, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE |
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW,
                    &window_rect, &client_rect, valid_rects );
    SetThreadDpiAwarenessContext( context );
}


/*******************************************************************
 *		get_server_window_text
 *
 * Retrieve the window text from the server.
 */
static data_size_t get_server_window_text( HWND hwnd, WCHAR *text, data_size_t count )
{
    data_size_t len = 0, needed = 0;

    SERVER_START_REQ( get_window_text )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (count) wine_server_set_reply( req, text, (count - 1) * sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            needed = reply->length;
            len = wine_server_reply_size(reply);
        }
    }
    SERVER_END_REQ;
    if (text) text[len / sizeof(WCHAR)] = 0;
    return needed;
}


/*******************************************************************
 *           get_hwnd_message_parent
 *
 * Return the parent for HWND_MESSAGE windows.
 */
HWND get_hwnd_message_parent(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (!thread_info->msg_window) GetDesktopWindow();  /* trigger creation */
    return thread_info->msg_window;
}


/*******************************************************************
 *           is_desktop_window
 *
 * Check if window is the desktop or the HWND_MESSAGE top parent.
 */
BOOL is_desktop_window( HWND hwnd )
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (!hwnd) return FALSE;
    if (hwnd == thread_info->top_window) return TRUE;
    if (hwnd == thread_info->msg_window) return TRUE;

    if (!HIWORD(hwnd) || HIWORD(hwnd) == 0xffff)
    {
        if (LOWORD(thread_info->top_window) == LOWORD(hwnd)) return TRUE;
        if (LOWORD(thread_info->msg_window) == LOWORD(hwnd)) return TRUE;
    }
    return FALSE;
}


/*******************************************************************
 * Dummy window surface for windows that shouldn't get painted.
 */

static void CDECL dummy_surface_lock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void CDECL dummy_surface_unlock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void *CDECL dummy_surface_get_bitmap_info( struct window_surface *window_surface, BITMAPINFO *info )
{
    static DWORD dummy_data;

    info->bmiHeader.biSize          = sizeof( info->bmiHeader );
    info->bmiHeader.biWidth         = dummy_surface.rect.right;
    info->bmiHeader.biHeight        = dummy_surface.rect.bottom;
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = 32;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biSizeImage     = 0;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    return &dummy_data;
}

static RECT *CDECL dummy_surface_get_bounds( struct window_surface *window_surface )
{
    static RECT dummy_bounds;
    return &dummy_bounds;
}

static void CDECL dummy_surface_set_region( struct window_surface *window_surface, HRGN region )
{
    /* nothing to do */
}

static void CDECL dummy_surface_flush( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void CDECL dummy_surface_destroy( struct window_surface *window_surface )
{
    /* nothing to do */
}

static const struct window_surface_funcs dummy_surface_funcs =
{
    dummy_surface_lock,
    dummy_surface_unlock,
    dummy_surface_get_bitmap_info,
    dummy_surface_get_bounds,
    dummy_surface_set_region,
    dummy_surface_flush,
    dummy_surface_destroy
};

struct window_surface dummy_surface = { &dummy_surface_funcs, { NULL, NULL }, 1, { 0, 0, 1, 1 } };

/*******************************************************************
 * Off-screen window surface.
 */

struct offscreen_window_surface
{
    struct window_surface header;
    CRITICAL_SECTION cs;
    RECT bounds;
    char *bits;
    BITMAPINFO info;
};

static const struct window_surface_funcs offscreen_window_surface_funcs;

static inline void reset_bounds( RECT *bounds )
{
    bounds->left = bounds->top = INT_MAX;
    bounds->right = bounds->bottom = INT_MIN;
}

static struct offscreen_window_surface *impl_from_window_surface( struct window_surface *base )
{
    if (!base || base->funcs != &offscreen_window_surface_funcs) return NULL;
    return CONTAINING_RECORD( base, struct offscreen_window_surface, header );
}

static void CDECL offscreen_window_surface_lock( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    EnterCriticalSection( &impl->cs );
}

static void CDECL offscreen_window_surface_unlock( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    LeaveCriticalSection( &impl->cs );
}

static RECT *CDECL offscreen_window_surface_get_bounds( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    return &impl->bounds;
}

static void *CDECL offscreen_window_surface_get_bitmap_info( struct window_surface *base, BITMAPINFO *info )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    memcpy( info, &impl->info, offsetof( BITMAPINFO, bmiColors[0] ) );
    return impl->bits;
}

static void CDECL offscreen_window_surface_set_region( struct window_surface *base, HRGN region )
{
}

static void CDECL offscreen_window_surface_flush( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    base->funcs->lock( base );
    reset_bounds( &impl->bounds );
    base->funcs->unlock( base );
}

static void CDECL offscreen_window_surface_destroy( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    impl->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection( &impl->cs );
    free( impl );
}

static const struct window_surface_funcs offscreen_window_surface_funcs =
{
    offscreen_window_surface_lock,
    offscreen_window_surface_unlock,
    offscreen_window_surface_get_bitmap_info,
    offscreen_window_surface_get_bounds,
    offscreen_window_surface_set_region,
    offscreen_window_surface_flush,
    offscreen_window_surface_destroy
};

void create_offscreen_window_surface( const RECT *visible_rect, struct window_surface **surface )
{
    struct offscreen_window_surface *impl;
    SIZE_T size;
    RECT surface_rect = *visible_rect;

    TRACE( "visible_rect %s, surface %p.\n", wine_dbgstr_rect( visible_rect ), surface );

    OffsetRect( &surface_rect, -surface_rect.left, -surface_rect.top );
    surface_rect.right  = (surface_rect.right + 0x1f) & ~0x1f;
    surface_rect.bottom = (surface_rect.bottom + 0x1f) & ~0x1f;

    /* check that old surface is an offscreen_window_surface, or release it */
    if ((impl = impl_from_window_surface( *surface )))
    {
        /* if the rect didn't change, keep the same surface */
        if (EqualRect( &surface_rect, &impl->header.rect )) return;
        window_surface_release( &impl->header );
    }
    else if (*surface) window_surface_release( *surface );

    /* create a new window surface */
    *surface = NULL;
    size = surface_rect.right * surface_rect.bottom * 4;
    if (!(impl = calloc(1, offsetof( struct offscreen_window_surface, info.bmiColors[0] ) + size))) return;

    impl->header.funcs = &offscreen_window_surface_funcs;
    impl->header.ref = 1;
    impl->header.rect = surface_rect;

    InitializeCriticalSection( &impl->cs );
    impl->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": surface");
    reset_bounds( &impl->bounds );

    impl->bits = (char *)&impl->info.bmiColors[0];
    impl->info.bmiHeader.biSize        = sizeof( impl->info );
    impl->info.bmiHeader.biWidth       = surface_rect.right;
    impl->info.bmiHeader.biHeight      = surface_rect.bottom;
    impl->info.bmiHeader.biPlanes      = 1;
    impl->info.bmiHeader.biBitCount    = 32;
    impl->info.bmiHeader.biCompression = BI_RGB;
    impl->info.bmiHeader.biSizeImage   = size;

    TRACE( "created window surface %p\n", &impl->header );

    *surface = &impl->header;
}

/*******************************************************************
 *           register_window_surface
 *
 * Register a window surface in the global list, possibly replacing another one.
 */
void register_window_surface( struct window_surface *old, struct window_surface *new )
{
    if (old == new) return;
    EnterCriticalSection( &surfaces_section );
    if (old && old != &dummy_surface) list_remove( &old->entry );
    if (new && new != &dummy_surface) list_add_tail( &window_surfaces, &new->entry );
    LeaveCriticalSection( &surfaces_section );
}


/*******************************************************************
 *           flush_window_surfaces
 *
 * Flush pending output from all window surfaces.
 */
void flush_window_surfaces( BOOL idle )
{
    static DWORD last_idle;
    DWORD now;
    struct window_surface *surface;

    EnterCriticalSection( &surfaces_section );
    now = GetTickCount();
    if (idle) last_idle = now;
    /* if not idle, we only flush if there's evidence that the app never goes idle */
    else if ((int)(now - last_idle) < 50) goto done;

    LIST_FOR_EACH_ENTRY( surface, &window_surfaces, struct window_surface, entry )
        surface->funcs->flush( surface );
done:
    LeaveCriticalSection( &surfaces_section );
}


/***********************************************************************
 *           WIN_GetPtr
 *
 * Return a pointer to the WND structure if local to the process,
 * or WND_OTHER_PROCESS if handle may be valid in other process.
 * If ret value is a valid pointer, it must be released with WIN_ReleasePtr.
 */
WND *WIN_GetPtr( HWND hwnd )
{
    WND *ptr;

    if ((ptr = get_user_handle_ptr( hwnd, USER_WINDOW )) == WND_OTHER_PROCESS)
    {
        if (is_desktop_window( hwnd )) ptr = WND_DESKTOP;
    }
    return ptr;
}


/***********************************************************************
 *           WIN_IsCurrentProcess
 *
 * Check whether a given window belongs to the current process (and return the full handle).
 */
HWND WIN_IsCurrentProcess( HWND hwnd )
{
    WND *ptr;
    HWND ret;

    if (!(ptr = WIN_GetPtr( hwnd )) || ptr == WND_OTHER_PROCESS || ptr == WND_DESKTOP) return 0;
    ret = ptr->obj.handle;
    WIN_ReleasePtr( ptr );
    return ret;
}


/***********************************************************************
 *           WIN_IsCurrentThread
 *
 * Check whether a given window belongs to the current thread (and return the full handle).
 */
HWND WIN_IsCurrentThread( HWND hwnd )
{
    WND *ptr;
    HWND ret = 0;

    if (!(ptr = WIN_GetPtr( hwnd )) || ptr == WND_OTHER_PROCESS || ptr == WND_DESKTOP) return 0;
    if (ptr->tid == GetCurrentThreadId()) ret = ptr->obj.handle;
    WIN_ReleasePtr( ptr );
    return ret;
}


/***********************************************************************
 *           win_set_flags
 *
 * Set the flags of a window and return the previous value.
 */
UINT win_set_flags( HWND hwnd, UINT set_mask, UINT clear_mask )
{
    UINT ret;
    WND *ptr = WIN_GetPtr( hwnd );

    if (!ptr || ptr == WND_OTHER_PROCESS || ptr == WND_DESKTOP) return 0;
    ret = ptr->flags;
    ptr->flags = (ret & ~clear_mask) | set_mask;
    WIN_ReleasePtr( ptr );
    return ret;
}


/***********************************************************************
 *           WIN_GetFullHandle
 *
 * Convert a possibly truncated window handle to a full 32-bit handle.
 */
HWND WIN_GetFullHandle( HWND hwnd )
{
    WND *ptr;

    if (!hwnd || (ULONG_PTR)hwnd >> 16) return hwnd;
    if (LOWORD(hwnd) <= 1 || LOWORD(hwnd) == 0xffff) return hwnd;
    /* do sign extension for -2 and -3 */
    if (LOWORD(hwnd) >= (WORD)-3) return (HWND)(LONG_PTR)(INT16)LOWORD(hwnd);

    if (!(ptr = WIN_GetPtr( hwnd ))) return hwnd;

    if (ptr == WND_DESKTOP)
    {
        if (LOWORD(hwnd) == LOWORD(GetDesktopWindow())) return GetDesktopWindow();
        else return get_hwnd_message_parent();
    }

    if (ptr != WND_OTHER_PROCESS)
    {
        hwnd = ptr->obj.handle;
        WIN_ReleasePtr( ptr );
    }
    else  /* may belong to another process */
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            if (!wine_server_call_err( req )) hwnd = wine_server_ptr_handle( reply->full_handle );
        }
        SERVER_END_REQ;
    }
    return hwnd;
}


/***********************************************************************
 *           WIN_SetOwner
 *
 * Change the owner of a window.
 */
static HWND WIN_SetOwner( HWND hwnd, HWND owner )
{
    WND *win = WIN_GetPtr( hwnd );
    HWND ret = 0;

    if (!win || win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd)) ERR( "cannot set owner %p on other process window %p\n", owner, hwnd );
        return 0;
    }
    SERVER_START_REQ( set_window_owner )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->owner  = wine_server_user_handle( owner );
        if (!wine_server_call( req ))
        {
            win->owner = wine_server_ptr_handle( reply->full_owner );
            ret = wine_server_ptr_handle( reply->prev_owner );
        }
    }
    SERVER_END_REQ;
    WIN_ReleasePtr( win );
    return ret;
}


/***********************************************************************
 *           WIN_SetStyle
 *
 * Change the style of a window.
 */
ULONG WIN_SetStyle( HWND hwnd, ULONG set_bits, ULONG clear_bits )
{
    BOOL ok, made_visible = FALSE;
    STYLESTRUCT style;
    WND *win = WIN_GetPtr( hwnd );

    if (!win || win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (IsWindow(hwnd))
            return SendMessageW(hwnd, WM_WINE_SETSTYLE, set_bits, clear_bits);
        return 0;
    }
    style.styleOld = win->dwStyle;
    style.styleNew = (win->dwStyle | set_bits) & ~clear_bits;
    if (style.styleNew == style.styleOld)
    {
        WIN_ReleasePtr( win );
        return style.styleNew;
    }
    SERVER_START_REQ( set_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->flags  = SET_WIN_STYLE;
        req->style  = style.styleNew;
        req->extra_offset = -1;
        if ((ok = !wine_server_call( req )))
        {
            style.styleOld = reply->old_style;
            win->dwStyle = style.styleNew;
        }
    }
    SERVER_END_REQ;

    if (ok && ((style.styleOld ^ style.styleNew) & WS_VISIBLE))
    {
        made_visible = (style.styleNew & WS_VISIBLE) != 0;
        invalidate_dce( win, NULL );
    }
    WIN_ReleasePtr( win );

    if (!ok) return 0;

    USER_Driver->pSetWindowStyle( hwnd, GWL_STYLE, &style );
    if (made_visible) update_window_state( hwnd );

    return style.styleOld;
}


/***********************************************************************
 *           WIN_GetRectangles
 *
 * Get the window and client rectangles.
 */
BOOL WIN_GetRectangles( HWND hwnd, enum coords_relative relative, RECT *rectWindow, RECT *rectClient )
{
    WND *win = WIN_GetPtr( hwnd );
    BOOL ret = TRUE;

    if (!win)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    if (win == WND_DESKTOP)
    {
        RECT rect;
        rect.left = rect.top = 0;
        if (hwnd == get_hwnd_message_parent())
        {
            rect.right  = 100;
            rect.bottom = 100;
            rect = rect_win_to_thread_dpi( hwnd, rect );
        }
        else
        {
            rect = get_primary_monitor_rect();
        }
        if (rectWindow) *rectWindow = rect;
        if (rectClient) *rectClient = rect;
        return TRUE;
    }
    if (win != WND_OTHER_PROCESS)
    {
        RECT window_rect = win->window_rect, client_rect = win->client_rect;

        switch (relative)
        {
        case COORDS_CLIENT:
            OffsetRect( &window_rect, -win->client_rect.left, -win->client_rect.top );
            OffsetRect( &client_rect, -win->client_rect.left, -win->client_rect.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
                mirror_rect( &win->client_rect, &window_rect );
            break;
        case COORDS_WINDOW:
            OffsetRect( &window_rect, -win->window_rect.left, -win->window_rect.top );
            OffsetRect( &client_rect, -win->window_rect.left, -win->window_rect.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
                mirror_rect( &win->window_rect, &client_rect );
            break;
        case COORDS_PARENT:
            if (win->parent)
            {
                WND *parent = WIN_GetPtr( win->parent );
                if (parent == WND_DESKTOP) break;
                if (!parent || parent == WND_OTHER_PROCESS)
                {
                    WIN_ReleasePtr( win );
                    goto other_process;
                }
                if (parent->flags & WIN_CHILDREN_MOVED)
                {
                    WIN_ReleasePtr( parent );
                    WIN_ReleasePtr( win );
                    goto other_process;
                }
                if (parent->dwExStyle & WS_EX_LAYOUTRTL)
                {
                    mirror_rect( &parent->client_rect, &window_rect );
                    mirror_rect( &parent->client_rect, &client_rect );
                }
                WIN_ReleasePtr( parent );
            }
            break;
        case COORDS_SCREEN:
            while (win->parent)
            {
                WND *parent = WIN_GetPtr( win->parent );
                if (parent == WND_DESKTOP) break;
                if (!parent || parent == WND_OTHER_PROCESS)
                {
                    WIN_ReleasePtr( win );
                    goto other_process;
                }
                WIN_ReleasePtr( win );
                if (parent->flags & WIN_CHILDREN_MOVED)
                {
                    WIN_ReleasePtr( parent );
                    goto other_process;
                }
                win = parent;
                if (win->parent)
                {
                    OffsetRect( &window_rect, win->client_rect.left, win->client_rect.top );
                    OffsetRect( &client_rect, win->client_rect.left, win->client_rect.top );
                }
            }
            break;
        }
        if (rectWindow) *rectWindow = rect_win_to_thread_dpi( hwnd, window_rect );
        if (rectClient) *rectClient = rect_win_to_thread_dpi( hwnd, client_rect );
        WIN_ReleasePtr( win );
        return TRUE;
    }

other_process:
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->relative = relative;
        req->dpi = get_thread_dpi();
        if ((ret = !wine_server_call_err( req )))
        {
            if (rectWindow)
            {
                rectWindow->left   = reply->window.left;
                rectWindow->top    = reply->window.top;
                rectWindow->right  = reply->window.right;
                rectWindow->bottom = reply->window.bottom;
            }
            if (rectClient)
            {
                rectClient->left   = reply->client.left;
                rectClient->top    = reply->client.top;
                rectClient->right  = reply->client.right;
                rectClient->bottom = reply->client.bottom;
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           WIN_DestroyWindow
 *
 * Destroy storage associated to a window. "Internals" p.358
 */
LRESULT WIN_DestroyWindow( HWND hwnd )
{
    WND *wndPtr;
    HWND *list;
    HMENU menu = 0, sys_menu;
    struct window_surface *surface;

    TRACE("%p\n", hwnd );

    /* destroy default IME window */
    if (win_set_flags( hwnd, 0, WIN_HAS_IME_WIN ) & WIN_HAS_IME_WIN)
    {
        TRACE("unregister IME window for %p\n", hwnd);
        imm_unregister_window( hwnd );
    }

    /* free child windows */
    if ((list = WIN_ListChildren( hwnd )))
    {
        int i;
        for (i = 0; list[i]; i++)
        {
            if (WIN_IsCurrentThread( list[i] )) WIN_DestroyWindow( list[i] );
            else SendNotifyMessageW( list[i], WM_WINE_DESTROYWINDOW, 0, 0 );
        }
        HeapFree( GetProcessHeap(), 0, list );
    }

    /* Unlink now so we won't bother with the children later on */
    SERVER_START_REQ( set_parent )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->parent = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /*
     * Send the WM_NCDESTROY to the window being destroyed.
     */
    SendMessageW( hwnd, WM_NCDESTROY, 0, 0 );

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    /* free resources associated with the window */

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
    if ((wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
        menu = (HMENU)wndPtr->wIDmenu;
    sys_menu = wndPtr->hSysMenu;
    free_dce( wndPtr->dce, hwnd );
    wndPtr->dce = NULL;
    HeapFree( GetProcessHeap(), 0, wndPtr->text );
    wndPtr->text = NULL;
    HeapFree( GetProcessHeap(), 0, wndPtr->pScroll );
    wndPtr->pScroll = NULL;
    DestroyIcon( wndPtr->hIconSmall2 );
    surface = wndPtr->surface;
    wndPtr->surface = NULL;
    WIN_ReleasePtr( wndPtr );

    if (menu) DestroyMenu( menu );
    if (sys_menu) DestroyMenu( sys_menu );
    if (surface)
    {
        register_window_surface( surface, NULL );
        window_surface_release( surface );
    }

    USER_Driver->pDestroyWindow( hwnd );

    free_window_handle( hwnd );
    return 0;
}


/***********************************************************************
 *           next_thread_window
 */
static WND *next_thread_window( HWND *hwnd )
{
    struct user_object *ptr;
    WND *win;
    WORD index = *hwnd ? USER_HANDLE_TO_INDEX( *hwnd ) + 1 : 0;

    USER_Lock();
    while (index < NB_USER_HANDLES)
    {
        if (!(ptr = user_handles[index++])) continue;
        if (ptr->type != USER_WINDOW) continue;
        win = (WND *)ptr;
        if (win->tid != GetCurrentThreadId()) continue;
        *hwnd = ptr->handle;
        return win;
    }
    USER_Unlock();
    return NULL;
}


/***********************************************************************
 *		destroy_thread_windows
 *
 * Destroy all window owned by the current thread.
 */
void destroy_thread_windows(void)
{
    WND *wndPtr;
    HWND hwnd = 0;
    HMENU menu, sys_menu;
    struct window_surface *surface;

    while ((wndPtr = next_thread_window( &hwnd )))
    {
        /* destroy the client-side storage */

        menu = ((wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD) ? (HMENU)wndPtr->wIDmenu : 0;
        sys_menu = wndPtr->hSysMenu;
        free_dce( wndPtr->dce, hwnd );
        surface = wndPtr->surface;
        InterlockedCompareExchangePointer( &user_handles[USER_HANDLE_TO_INDEX(hwnd)], NULL, wndPtr );
        WIN_ReleasePtr( wndPtr );
        HeapFree( GetProcessHeap(), 0, wndPtr );
        if (menu) DestroyMenu( menu );
        if (sys_menu) DestroyMenu( sys_menu );
        if (surface)
        {
            register_window_surface( surface, NULL );
            window_surface_release( surface );
        }
    }
}


/***********************************************************************
 *           WIN_FixCoordinates
 *
 * Fix the coordinates - Helper for WIN_CreateWindowEx.
 * returns default show mode in sw.
 */
static void WIN_FixCoordinates( CREATESTRUCTW *cs, INT *sw)
{
#define IS_DEFAULT(x)  ((x) == CW_USEDEFAULT || (x) == (SHORT)0x8000)
    POINT pos[2];

    if (cs->dwExStyle & WS_EX_MDICHILD)
    {
        UINT id = 0;

        MDI_CalcDefaultChildPos(cs->hwndParent, -1, pos, 0, &id);
        if (!(cs->style & WS_POPUP)) cs->hMenu = ULongToHandle(id);

        TRACE("MDI child id %04x\n", id);
    }

    if (cs->style & (WS_CHILD | WS_POPUP))
    {
        if (cs->dwExStyle & WS_EX_MDICHILD)
        {
            if (IS_DEFAULT(cs->x))
            {
                cs->x = pos[0].x;
                cs->y = pos[0].y;
            }
            if (IS_DEFAULT(cs->cx) || !cs->cx) cs->cx = pos[1].x;
            if (IS_DEFAULT(cs->cy) || !cs->cy) cs->cy = pos[1].y;
        }
        else
        {
            if (IS_DEFAULT(cs->x)) cs->x = cs->y = 0;
            if (IS_DEFAULT(cs->cx)) cs->cx = cs->cy = 0;
        }
    }
    else  /* overlapped window */
    {
        HMONITOR monitor;
        MONITORINFO mon_info;
        STARTUPINFOW info;

        if (!IS_DEFAULT(cs->x) && !IS_DEFAULT(cs->cx) && !IS_DEFAULT(cs->cy)) return;

        monitor = MonitorFromWindow( cs->hwndParent, MONITOR_DEFAULTTOPRIMARY );
        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );
        GetStartupInfoW( &info );

        if (IS_DEFAULT(cs->x))
        {
            if (!IS_DEFAULT(cs->y)) *sw = cs->y;
            cs->x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : mon_info.rcWork.left;
            cs->y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : mon_info.rcWork.top;
        }

        if (IS_DEFAULT(cs->cx))
        {
            if (info.dwFlags & STARTF_USESIZE)
            {
                cs->cx = info.dwXSize;
                cs->cy = info.dwYSize;
            }
            else
            {
                cs->cx = (mon_info.rcWork.right - mon_info.rcWork.left) * 3 / 4 - cs->x;
                cs->cy = (mon_info.rcWork.bottom - mon_info.rcWork.top) * 3 / 4 - cs->y;
            }
        }
        /* neither x nor cx are default. Check the y values .
         * In the trace we see Outlook and Outlook Express using
         * cy set to CW_USEDEFAULT when opening the address book.
         */
        else if (IS_DEFAULT(cs->cy))
        {
            FIXME("Strange use of CW_USEDEFAULT in nHeight\n");
            cs->cy = (mon_info.rcWork.bottom - mon_info.rcWork.top) * 3 / 4 - cs->y;
        }
    }
#undef IS_DEFAULT
}

/***********************************************************************
 *           dump_window_styles
 */
static void dump_window_styles( DWORD style, DWORD exstyle )
{
    TRACE( "style:" );
    if(style & WS_POPUP) TRACE(" WS_POPUP");
    if(style & WS_CHILD) TRACE(" WS_CHILD");
    if(style & WS_MINIMIZE) TRACE(" WS_MINIMIZE");
    if(style & WS_VISIBLE) TRACE(" WS_VISIBLE");
    if(style & WS_DISABLED) TRACE(" WS_DISABLED");
    if(style & WS_CLIPSIBLINGS) TRACE(" WS_CLIPSIBLINGS");
    if(style & WS_CLIPCHILDREN) TRACE(" WS_CLIPCHILDREN");
    if(style & WS_MAXIMIZE) TRACE(" WS_MAXIMIZE");
    if((style & WS_CAPTION) == WS_CAPTION) TRACE(" WS_CAPTION");
    else
    {
        if(style & WS_BORDER) TRACE(" WS_BORDER");
        if(style & WS_DLGFRAME) TRACE(" WS_DLGFRAME");
    }
    if(style & WS_VSCROLL) TRACE(" WS_VSCROLL");
    if(style & WS_HSCROLL) TRACE(" WS_HSCROLL");
    if(style & WS_SYSMENU) TRACE(" WS_SYSMENU");
    if(style & WS_THICKFRAME) TRACE(" WS_THICKFRAME");
    if (style & WS_CHILD)
    {
        if(style & WS_GROUP) TRACE(" WS_GROUP");
        if(style & WS_TABSTOP) TRACE(" WS_TABSTOP");
    }
    else
    {
        if(style & WS_MINIMIZEBOX) TRACE(" WS_MINIMIZEBOX");
        if(style & WS_MAXIMIZEBOX) TRACE(" WS_MAXIMIZEBOX");
    }

    /* FIXME: Add dumping of BS_/ES_/SBS_/LBS_/CBS_/DS_/etc. styles */
#define DUMPED_STYLES \
    ((DWORD)(WS_POPUP | \
     WS_CHILD | \
     WS_MINIMIZE | \
     WS_VISIBLE | \
     WS_DISABLED | \
     WS_CLIPSIBLINGS | \
     WS_CLIPCHILDREN | \
     WS_MAXIMIZE | \
     WS_BORDER | \
     WS_DLGFRAME | \
     WS_VSCROLL | \
     WS_HSCROLL | \
     WS_SYSMENU | \
     WS_THICKFRAME | \
     WS_GROUP | \
     WS_TABSTOP | \
     WS_MINIMIZEBOX | \
     WS_MAXIMIZEBOX))

    if(style & ~DUMPED_STYLES) TRACE(" %08x", style & ~DUMPED_STYLES);
    TRACE("\n");
#undef DUMPED_STYLES

    TRACE( "exstyle:" );
    if(exstyle & WS_EX_DLGMODALFRAME) TRACE(" WS_EX_DLGMODALFRAME");
    if(exstyle & WS_EX_DRAGDETECT) TRACE(" WS_EX_DRAGDETECT");
    if(exstyle & WS_EX_NOPARENTNOTIFY) TRACE(" WS_EX_NOPARENTNOTIFY");
    if(exstyle & WS_EX_TOPMOST) TRACE(" WS_EX_TOPMOST");
    if(exstyle & WS_EX_ACCEPTFILES) TRACE(" WS_EX_ACCEPTFILES");
    if(exstyle & WS_EX_TRANSPARENT) TRACE(" WS_EX_TRANSPARENT");
    if(exstyle & WS_EX_MDICHILD) TRACE(" WS_EX_MDICHILD");
    if(exstyle & WS_EX_TOOLWINDOW) TRACE(" WS_EX_TOOLWINDOW");
    if(exstyle & WS_EX_WINDOWEDGE) TRACE(" WS_EX_WINDOWEDGE");
    if(exstyle & WS_EX_CLIENTEDGE) TRACE(" WS_EX_CLIENTEDGE");
    if(exstyle & WS_EX_CONTEXTHELP) TRACE(" WS_EX_CONTEXTHELP");
    if(exstyle & WS_EX_RIGHT) TRACE(" WS_EX_RIGHT");
    if(exstyle & WS_EX_RTLREADING) TRACE(" WS_EX_RTLREADING");
    if(exstyle & WS_EX_LEFTSCROLLBAR) TRACE(" WS_EX_LEFTSCROLLBAR");
    if(exstyle & WS_EX_CONTROLPARENT) TRACE(" WS_EX_CONTROLPARENT");
    if(exstyle & WS_EX_STATICEDGE) TRACE(" WS_EX_STATICEDGE");
    if(exstyle & WS_EX_APPWINDOW) TRACE(" WS_EX_APPWINDOW");
    if(exstyle & WS_EX_LAYERED) TRACE(" WS_EX_LAYERED");
    if(exstyle & WS_EX_NOINHERITLAYOUT) TRACE(" WS_EX_NOINHERITLAYOUT");
    if(exstyle & WS_EX_LAYOUTRTL) TRACE(" WS_EX_LAYOUTRTL");
    if(exstyle & WS_EX_COMPOSITED) TRACE(" WS_EX_COMPOSITED");
    if(exstyle & WS_EX_NOACTIVATE) TRACE(" WS_EX_NOACTIVATE");

#define DUMPED_EX_STYLES \
    ((DWORD)(WS_EX_DLGMODALFRAME | \
     WS_EX_DRAGDETECT | \
     WS_EX_NOPARENTNOTIFY | \
     WS_EX_TOPMOST | \
     WS_EX_ACCEPTFILES | \
     WS_EX_TRANSPARENT | \
     WS_EX_MDICHILD | \
     WS_EX_TOOLWINDOW | \
     WS_EX_WINDOWEDGE | \
     WS_EX_CLIENTEDGE | \
     WS_EX_CONTEXTHELP | \
     WS_EX_RIGHT | \
     WS_EX_RTLREADING | \
     WS_EX_LEFTSCROLLBAR | \
     WS_EX_CONTROLPARENT | \
     WS_EX_STATICEDGE | \
     WS_EX_APPWINDOW | \
     WS_EX_LAYERED | \
     WS_EX_NOINHERITLAYOUT | \
     WS_EX_LAYOUTRTL | \
     WS_EX_COMPOSITED |\
     WS_EX_NOACTIVATE))

    if(exstyle & ~DUMPED_EX_STYLES) TRACE(" %08x", exstyle & ~DUMPED_EX_STYLES);
    TRACE("\n");
#undef DUMPED_EX_STYLES
}

/***********************************************************************
 *           map_dpi_create_struct
 */
static void map_dpi_create_struct( CREATESTRUCTW *cs, UINT dpi_from, UINT dpi_to )
{
    if (!dpi_from && !dpi_to) return;
    if (!dpi_from || !dpi_to)
    {
        POINT pt = { cs->x, cs->y };
        UINT mon_dpi = get_monitor_dpi( MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST ));
        if (!dpi_from) dpi_from = mon_dpi;
        else dpi_to = mon_dpi;
    }
    if (dpi_from == dpi_to) return;
    cs->x = MulDiv( cs->x, dpi_to, dpi_from );
    cs->y = MulDiv( cs->y, dpi_to, dpi_from );
    cs->cx = MulDiv( cs->cx, dpi_to, dpi_from );
    cs->cy = MulDiv( cs->cy, dpi_to, dpi_from );
}

/***********************************************************************
 *           fix_exstyle
 */
static DWORD fix_exstyle( DWORD style, DWORD exstyle )
{
    if ((exstyle & WS_EX_DLGMODALFRAME) ||
            (!(exstyle & WS_EX_STATICEDGE) &&
             (style & (WS_DLGFRAME | WS_THICKFRAME))))
        exstyle |= WS_EX_WINDOWEDGE;
    else
        exstyle &= ~WS_EX_WINDOWEDGE;
    return exstyle;
}

/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
HWND WIN_CreateWindowEx( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE module, BOOL unicode )
{
    INT cx, cy, style, ex_style, sw = SW_SHOW;
    LRESULT result;
    RECT rect;
    WND *wndPtr;
    HWND hwnd, parent, owner, top_child = 0;
    const WCHAR *p = className;
    UINT win_dpi, thread_dpi = get_thread_dpi();
    DPI_AWARENESS_CONTEXT context;
    MDICREATESTRUCTW mdi_cs;
    CBT_CREATEWNDW cbtc;
    CREATESTRUCTW cbcs;

    className = CLASS_GetVersionedName(className, NULL, NULL, TRUE);

    TRACE("%s %s%s%s ex=%08x style=%08x %d,%d %dx%d parent=%p menu=%p inst=%p params=%p\n",
          unicode ? debugstr_w(cs->lpszName) : debugstr_a((LPCSTR)cs->lpszName),
          debugstr_w(p), p != className ? "->" : "", p != className ? debugstr_w(className) : "",
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );
    if(TRACE_ON(win)) dump_window_styles( cs->style, cs->dwExStyle );

    /* Fix the styles for MDI children */
    if (cs->dwExStyle & WS_EX_MDICHILD)
    {
        if (!(win_get_flags( cs->hwndParent ) & WIN_ISMDICLIENT))
        {
            WARN("WS_EX_MDICHILD, but parent %p is not MDIClient\n", cs->hwndParent);
            return 0;
        }

        /* cs->lpCreateParams of WM_[NC]CREATE is different for MDI children.
         * MDICREATESTRUCT members have the originally passed values.
         *
         * Note: we rely on the fact that MDICREATESTRUCTA and MDICREATESTRUCTW
         * have the same layout.
         */
        mdi_cs.szClass = cs->lpszClass;
        mdi_cs.szTitle = cs->lpszName;
        mdi_cs.hOwner = cs->hInstance;
        mdi_cs.x = cs->x;
        mdi_cs.y = cs->y;
        mdi_cs.cx = cs->cx;
        mdi_cs.cy = cs->cy;
        mdi_cs.style = cs->style;
        mdi_cs.lParam = (LPARAM)cs->lpCreateParams;

        cs->lpCreateParams = &mdi_cs;

        if (GetWindowLongW(cs->hwndParent, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
        {
            if (cs->style & WS_POPUP)
            {
                TRACE("WS_POPUP with MDIS_ALLCHILDSTYLES is not allowed\n");
                return 0;
            }
            cs->style |= WS_CHILD | WS_CLIPSIBLINGS;
        }
        else
        {
            cs->style &= ~WS_POPUP;
            cs->style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        }

        top_child = GetWindow(cs->hwndParent, GW_CHILD);

        if (top_child)
        {
            /* Restore current maximized child */
            if((cs->style & WS_VISIBLE) && IsZoomed(top_child))
            {
                TRACE("Restoring current maximized child %p\n", top_child);
                if (cs->style & WS_MAXIMIZE)
                {
                    /* if the new window is maximized don't bother repainting */
                    SendMessageW( top_child, WM_SETREDRAW, FALSE, 0 );
                    ShowWindow( top_child, SW_SHOWNORMAL );
                    SendMessageW( top_child, WM_SETREDRAW, TRUE, 0 );
                }
                else ShowWindow( top_child, SW_SHOWNORMAL );
            }
        }
    }

    /* Find the parent window */

    parent = cs->hwndParent;
    owner = 0;

    if (cs->hwndParent == HWND_MESSAGE)
    {
        cs->hwndParent = parent = get_hwnd_message_parent();
    }
    else if (cs->hwndParent)
    {
        if ((cs->style & (WS_CHILD|WS_POPUP)) != WS_CHILD)
        {
            parent = GetDesktopWindow();
            owner = cs->hwndParent;
        }
        else
        {
            DWORD parent_style = GetWindowLongW( parent, GWL_EXSTYLE );
            if ((parent_style & WS_EX_LAYOUTRTL) && !(parent_style & WS_EX_NOINHERITLAYOUT))
                cs->dwExStyle |= WS_EX_LAYOUTRTL;
        }
    }
    else
    {
        if ((cs->style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
        {
            WARN("No parent for child window\n" );
            SetLastError(ERROR_TLW_WITH_WSCHILD);
            return 0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
        }

        /* are we creating the desktop or HWND_MESSAGE parent itself? */
        if (className != (LPCWSTR)DESKTOP_CLASS_ATOM &&
            (IS_INTRESOURCE(className) || wcsicmp( className, L"Message" )))
        {
            DWORD layout;
            GetProcessDefaultLayout( &layout );
            if (layout & LAYOUT_RTL) cs->dwExStyle |= WS_EX_LAYOUTRTL;
            parent = GetDesktopWindow();
        }
    }

    WIN_FixCoordinates(cs, &sw); /* fix default coordinates */
    cs->dwExStyle = fix_exstyle(cs->style, cs->dwExStyle);

    /* Create the window structure */

    style = cs->style & ~WS_VISIBLE;
    ex_style = cs->dwExStyle & ~WS_EX_LAYERED;
    if (!(wndPtr = create_window_handle( parent, owner, className, module,
                    unicode, style, ex_style )))
    {
        WNDCLASSW wc;
        /* if it's a comctl32 class, GetClassInfo will load it, then we can retry */
        if (GetLastError() != ERROR_INVALID_HANDLE ||
            !GetClassInfoW( 0, className, &wc ) ||
            !(wndPtr = create_window_handle( parent, owner, className, module,
                    unicode, style, ex_style )))
            return 0;
    }
    hwnd = wndPtr->obj.handle;

    /* Fill the window structure */

    wndPtr->tid            = GetCurrentThreadId();
    wndPtr->hInstance      = cs->hInstance;
    wndPtr->text           = NULL;
    wndPtr->dwStyle        = style;
    wndPtr->dwExStyle      = ex_style;
    wndPtr->wIDmenu        = 0;
    wndPtr->helpContext    = 0;
    wndPtr->pScroll        = NULL;
    wndPtr->userdata       = 0;
    wndPtr->hIcon          = 0;
    wndPtr->hIconSmall     = 0;
    wndPtr->hIconSmall2    = 0;
    wndPtr->hSysMenu       = 0;

    wndPtr->min_pos.x = wndPtr->min_pos.y = -1;
    wndPtr->max_pos.x = wndPtr->max_pos.y = -1;
    SetRect( &wndPtr->normal_rect, cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy );

    if (wndPtr->dwStyle & WS_SYSMENU) SetSystemMenu( hwnd, 0 );

    /* call the WH_CBT hook */

    WIN_ReleasePtr( wndPtr );
    cbcs = *cs;
    cbtc.lpcs = &cbcs;
    cbtc.hwndInsertAfter = HWND_TOP;
    if (HOOK_CallHooks( WH_CBT, HCBT_CREATEWND, (WPARAM)hwnd, (LPARAM)&cbtc, unicode ) ||
            !(wndPtr = WIN_GetPtr( hwnd )))
    {
        free_window_handle( hwnd );
        return 0;
    }

    /*
     * Correct the window styles.
     *
     * It affects only the style loaded into the WIN structure.
     */

    if ((wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
    {
        wndPtr->dwStyle |= WS_CLIPSIBLINGS;
        if (!(wndPtr->dwStyle & WS_POPUP))
            wndPtr->dwStyle |= WS_CAPTION;
    }

    wndPtr->dwExStyle = cs->dwExStyle;
    /* WS_EX_WINDOWEDGE depends on some other styles */
    if ((wndPtr->dwStyle & (WS_DLGFRAME | WS_THICKFRAME)) &&
            !(wndPtr->dwStyle & (WS_CHILD | WS_POPUP)))
        wndPtr->dwExStyle |= WS_EX_WINDOWEDGE;

    if (!(wndPtr->dwStyle & (WS_CHILD | WS_POPUP)))
        wndPtr->flags |= WIN_NEED_SIZE;

    SERVER_START_REQ( set_window_info )
    {
        req->handle    = wine_server_user_handle( hwnd );
        req->flags     = SET_WIN_STYLE | SET_WIN_EXSTYLE | SET_WIN_INSTANCE | SET_WIN_UNICODE;
        req->style     = wndPtr->dwStyle;
        req->ex_style  = wndPtr->dwExStyle;
        req->instance  = wine_server_client_ptr( wndPtr->hInstance );
        req->is_unicode = (wndPtr->flags & WIN_ISUNICODE) != 0;
        req->extra_offset = -1;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* Set the window menu */

    if ((wndPtr->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
    {
        if (cs->hMenu)
        {
            if (!MENU_SetMenu(hwnd, cs->hMenu))
            {
                WIN_ReleasePtr( wndPtr );
                free_window_handle( hwnd );
                return 0;
            }
        }
        else
        {
            LPCWSTR menuName = (LPCWSTR)GetClassLongPtrW( hwnd, GCLP_MENUNAME );
            if (menuName)
            {
                cs->hMenu = LoadMenuW( cs->hInstance, menuName );
                if (cs->hMenu) MENU_SetMenu( hwnd, cs->hMenu );
            }
        }
    }
    else SetWindowLongPtrW( hwnd, GWLP_ID, (ULONG_PTR)cs->hMenu );

    win_dpi = wndPtr->dpi;
    WIN_ReleasePtr( wndPtr );

    if (parent) map_dpi_create_struct( cs, thread_dpi, win_dpi );

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));

    /* send the WM_GETMINMAXINFO message and fix the size if needed */

    cx = cs->cx;
    cy = cs->cy;
    if ((cs->style & WS_THICKFRAME) || !(cs->style & (WS_POPUP | WS_CHILD)))
    {
        MINMAXINFO info = WINPOS_GetMinMaxInfo( hwnd );
        cx = max( min( cx, info.ptMaxTrackSize.x ), info.ptMinTrackSize.x );
        cy = max( min( cy, info.ptMaxTrackSize.y ), info.ptMinTrackSize.y );
    }

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    SetRect( &rect, cs->x, cs->y, cs->x + cx, cs->y + cy );
    /* check for wraparound */
    if (cs->x > 0x7fffffff - cx) rect.right = 0x7fffffff;
    if (cs->y > 0x7fffffff - cy) rect.bottom = 0x7fffffff;
    if (!set_window_pos( hwnd, 0, SWP_NOZORDER | SWP_NOACTIVATE, &rect, &rect, NULL )) goto failed;

    /* send WM_NCCREATE */

    TRACE( "hwnd %p cs %d,%d %dx%d %s\n", hwnd, cs->x, cs->y, cs->cx, cs->cy, wine_dbgstr_rect(&rect) );
    if (unicode)
        result = SendMessageW( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
    else
        result = SendMessageA( hwnd, WM_NCCREATE, 0, (LPARAM)cs );
    if (!result)
    {
        WARN( "%p: aborted by WM_NCCREATE\n", hwnd );
        goto failed;
    }

    /* create default IME window */

    if (imm_register_window && !is_desktop_window( hwnd ) &&
        parent != get_hwnd_message_parent() && imm_register_window( hwnd ))
    {
        TRACE("register IME window for %p\n", hwnd);
        win_set_flags( hwnd, WIN_HAS_IME_WIN, 0 );
    }

    /* send WM_NCCALCSIZE */

    if (WIN_GetRectangles( hwnd, COORDS_PARENT, &rect, NULL ))
    {
        /* yes, even if the CBT hook was called with HWND_TOP */
        HWND insert_after = (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) ? HWND_BOTTOM : HWND_TOP;
        RECT client_rect = rect;

        /* the rectangle is in screen coords for WM_NCCALCSIZE when wparam is FALSE */
        MapWindowPoints( parent, 0, (POINT *)&client_rect, 2 );
        SendMessageW( hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&client_rect );
        MapWindowPoints( 0, parent, (POINT *)&client_rect, 2 );
        set_window_pos( hwnd, insert_after, SWP_NOACTIVATE, &rect, &client_rect, NULL );
    }
    else goto failed;

    /* send WM_CREATE */

    if (unicode)
        result = SendMessageW( hwnd, WM_CREATE, 0, (LPARAM)cs );
    else
        result = SendMessageA( hwnd, WM_CREATE, 0, (LPARAM)cs );
    if (result == -1) goto failed;

    /* call the driver */

    if (!USER_Driver->pCreateWindow( hwnd )) goto failed;

    NotifyWinEvent(EVENT_OBJECT_CREATE, hwnd, OBJID_WINDOW, 0);

    /* send the size messages */

    if (!(win_get_flags( hwnd ) & WIN_NEED_SIZE))
    {
        WIN_GetRectangles( hwnd, COORDS_PARENT, NULL, &rect );
        SendMessageW( hwnd, WM_SIZE, SIZE_RESTORED,
                      MAKELONG(rect.right-rect.left, rect.bottom-rect.top));
        SendMessageW( hwnd, WM_MOVE, 0, MAKELONG( rect.left, rect.top ) );
    }

    /* Show the window, maximizing or minimizing if needed */

    style = WIN_SetStyle( hwnd, 0, WS_MAXIMIZE | WS_MINIMIZE );
    if (style & (WS_MINIMIZE | WS_MAXIMIZE))
    {
        RECT newPos;
        UINT swFlag = (style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;

        swFlag = WINPOS_MinMaximize( hwnd, swFlag, &newPos );
        swFlag |= SWP_FRAMECHANGED; /* Frame always gets changed */
        if (!(style & WS_VISIBLE) || (style & WS_CHILD) || GetActiveWindow()) swFlag |= SWP_NOACTIVATE;
        SetWindowPos( hwnd, 0, newPos.left, newPos.top, newPos.right - newPos.left,
                      newPos.bottom - newPos.top, swFlag );
    }

    /* Notify the parent window only */

    send_parent_notify( hwnd, WM_CREATE );
    if (!IsWindow( hwnd ))
    {
        SetThreadDpiAwarenessContext( context );
        return 0;
    }

    if (parent == GetDesktopWindow())
        PostMessageW( parent, WM_PARENTNOTIFY, WM_CREATE, (LPARAM)hwnd );

    if (cs->style & WS_VISIBLE)
    {
        if (cs->style & WS_MAXIMIZE)
            sw = SW_SHOW;
        else if (cs->style & WS_MINIMIZE)
            sw = SW_SHOWMINIMIZED;

        ShowWindow( hwnd, sw );
        if (cs->dwExStyle & WS_EX_MDICHILD)
        {
            SendMessageW(cs->hwndParent, WM_MDIREFRESHMENU, 0, 0);
            /* ShowWindow won't activate child windows */
            SetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE );
        }
    }

    /* Call WH_SHELL hook */

    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) && !GetWindow( hwnd, GW_OWNER ))
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWCREATED, (WPARAM)hwnd, 0, TRUE );

    TRACE("created window %p\n", hwnd);
    SetThreadDpiAwarenessContext( context );
    return hwnd;

failed:
    WIN_DestroyWindow( hwnd );
    SetThreadDpiAwarenessContext( context );
    return 0;
}


/***********************************************************************
 *		CreateWindowExA (USER32.@)
 */
HWND WINAPI DECLSPEC_HOTPATCH CreateWindowExA( DWORD exStyle, LPCSTR className,
                                 LPCSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    CREATESTRUCTA cs;

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    if (!IS_INTRESOURCE(className))
    {
        WCHAR bufferW[256];
        if (!MultiByteToWideChar( CP_ACP, 0, className, -1, bufferW, ARRAY_SIZE( bufferW )))
            return 0;
        return wow_handlers.create_window( (CREATESTRUCTW *)&cs, bufferW, instance, FALSE );
    }
    /* Note: we rely on the fact that CREATESTRUCTA and */
    /* CREATESTRUCTW have the same layout. */
    return wow_handlers.create_window( (CREATESTRUCTW *)&cs, (LPCWSTR)className, instance, FALSE );
}


/***********************************************************************
 *		CreateWindowExW (USER32.@)
 */
HWND WINAPI DECLSPEC_HOTPATCH CreateWindowExW( DWORD exStyle, LPCWSTR className,
                                 LPCWSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    CREATESTRUCTW cs;

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    return wow_handlers.create_window( &cs, className, instance, TRUE );
}


/***********************************************************************
 *           WIN_SendDestroyMsg
 */
static void WIN_SendDestroyMsg( HWND hwnd )
{
    GUITHREADINFO info;

    info.cbSize = sizeof(info);
    if (GetGUIThreadInfo( GetCurrentThreadId(), &info ))
    {
        if (hwnd == info.hwndCaret) DestroyCaret();
        if (hwnd == info.hwndActive) WINPOS_ActivateOtherWindow( hwnd );
    }

    if (hwnd == NtUserGetClipboardOwner()) CLIPBOARD_ReleaseOwner( hwnd );

    /*
     * Send the WM_DESTROY to the window.
     */
    SendMessageW( hwnd, WM_DESTROY, 0, 0);

    /*
     * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
     * make sure that the window still exists when we come back.
     */
    if (IsWindow(hwnd))
    {
        HWND* pWndArray;
        int i;

        if (!(pWndArray = WIN_ListChildren( hwnd ))) return;

        for (i = 0; pWndArray[i]; i++)
        {
            if (IsWindow( pWndArray[i] )) WIN_SendDestroyMsg( pWndArray[i] );
        }
        HeapFree( GetProcessHeap(), 0, pWndArray );
    }
    else
      WARN("\tdestroyed itself while in WM_DESTROY!\n");
}


/***********************************************************************
 *		DestroyWindow (USER32.@)
 */
BOOL WINAPI DestroyWindow( HWND hwnd )
{
    BOOL is_child;

    if (!(hwnd = WIN_IsCurrentThread( hwnd )) || is_desktop_window( hwnd ))
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    TRACE("(%p)\n", hwnd);

      /* Call hooks */

    if (HOOK_CallHooks( WH_CBT, HCBT_DESTROYWND, (WPARAM)hwnd, 0, TRUE )) return FALSE;

    if (MENU_IsMenuActive() == hwnd)
        EndMenu();

    is_child = (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) != 0;

    if (is_child)
    {
        if (!USER_IsExitingThread( GetCurrentThreadId() ))
            send_parent_notify( hwnd, WM_DESTROY );
    }
    else if (!GetWindow( hwnd, GW_OWNER ))
    {
        HOOK_CallHooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0L, TRUE );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if (!IsWindow(hwnd)) return TRUE;

      /* Hide the window */
    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE)
    {
        /* Only child windows receive WM_SHOWWINDOW in DestroyWindow() */
        if (is_child)
            ShowWindow( hwnd, SW_HIDE );
        else
            SetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                          SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW );
    }

    if (!IsWindow(hwnd)) return TRUE;

      /* Recursively destroy owned windows */

    if (!is_child)
    {
        for (;;)
        {
            int i;
            BOOL got_one = FALSE;
            HWND *list = WIN_ListChildren( GetDesktopWindow() );
            if (list)
            {
                for (i = 0; list[i]; i++)
                {
                    if (GetWindow( list[i], GW_OWNER ) != hwnd) continue;
                    if (WIN_IsCurrentThread( list[i] ))
                    {
                        DestroyWindow( list[i] );
                        got_one = TRUE;
                        continue;
                    }
                    WIN_SetOwner( list[i], 0 );
                }
                HeapFree( GetProcessHeap(), 0, list );
            }
            if (!got_one) break;
        }
    }

      /* Send destroy messages */

    WIN_SendDestroyMsg( hwnd );
    if (!IsWindow( hwnd )) return TRUE;

      /* Destroy the window storage */

    WIN_DestroyWindow( hwnd );
    return TRUE;
}


/***********************************************************************
 *		CloseWindow (USER32.@)
 */
BOOL WINAPI CloseWindow( HWND hwnd )
{
    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) return FALSE;
    ShowWindow( hwnd, SW_MINIMIZE );
    return TRUE;
}


/***********************************************************************
 *		OpenIcon (USER32.@)
 */
BOOL WINAPI OpenIcon( HWND hwnd )
{
    if (!IsIconic( hwnd )) return FALSE;
    ShowWindow( hwnd, SW_SHOWNORMAL );
    return TRUE;
}


/***********************************************************************
 *		FindWindowExW (USER32.@)
 */
HWND WINAPI FindWindowExW( HWND parent, HWND child, LPCWSTR className, LPCWSTR title )
{
    HWND *list;
    HWND retvalue = 0;
    int i = 0, len = 0;
    WCHAR *buffer = NULL;

    if (!parent && child) parent = GetDesktopWindow();
    else if (parent == HWND_MESSAGE) parent = get_hwnd_message_parent();

    if (title)
    {
        len = lstrlenW(title) + 1;  /* one extra char to check for chars beyond the end */
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return 0;
    }

    if (!(list = list_window_children( 0, parent, className, 0 ))) goto done;

    if (child)
    {
        child = WIN_GetFullHandle( child );
        while (list[i] && list[i] != child) i++;
        if (!list[i]) goto done;
        i++;  /* start from next window */
    }

    if (title)
    {
        while (list[i])
        {
            if (InternalGetWindowText( list[i], buffer, len + 1 ))
            {
                if (!wcsicmp( buffer, title )) break;
            }
            else
            {
                if (!title[0]) break;
            }
            i++;
        }
    }
    retvalue = list[i];

 done:
    HeapFree( GetProcessHeap(), 0, list );
    HeapFree( GetProcessHeap(), 0, buffer );
    return retvalue;
}



/***********************************************************************
 *		FindWindowA (USER32.@)
 */
HWND WINAPI FindWindowA( LPCSTR className, LPCSTR title )
{
    HWND ret = FindWindowExA( 0, 0, className, title );
    if (!ret) SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
    return ret;
}


/***********************************************************************
 *		FindWindowExA (USER32.@)
 */
HWND WINAPI FindWindowExA( HWND parent, HWND child, LPCSTR className, LPCSTR title )
{
    LPWSTR titleW = NULL;
    HWND hwnd = 0;

    if (title)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, title, -1, NULL, 0 );
        if (!(titleW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return 0;
        MultiByteToWideChar( CP_ACP, 0, title, -1, titleW, len );
    }

    if (!IS_INTRESOURCE(className))
    {
        WCHAR classW[256];
        if (MultiByteToWideChar( CP_ACP, 0, className, -1, classW, ARRAY_SIZE( classW )))
            hwnd = FindWindowExW( parent, child, classW, titleW );
    }
    else
    {
        hwnd = FindWindowExW( parent, child, (LPCWSTR)className, titleW );
    }

    HeapFree( GetProcessHeap(), 0, titleW );
    return hwnd;
}


/***********************************************************************
 *		FindWindowW (USER32.@)
 */
HWND WINAPI FindWindowW( LPCWSTR className, LPCWSTR title )
{
    return FindWindowExW( 0, 0, className, title );
}


/**********************************************************************
 *		GetDesktopWindow (USER32.@)
 */
HWND WINAPI GetDesktopWindow(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (thread_info->top_window) return thread_info->top_window;

    SERVER_START_REQ( get_desktop_window )
    {
        req->force = 0;
        if (!wine_server_call( req ))
        {
            thread_info->top_window = wine_server_ptr_handle( reply->top_window );
            thread_info->msg_window = wine_server_ptr_handle( reply->msg_window );
        }
    }
    SERVER_END_REQ;

    if (!thread_info->top_window)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        WCHAR windir[MAX_PATH];
        WCHAR app[MAX_PATH + ARRAY_SIZE( L"\\explorer.exe" )];
        WCHAR cmdline[MAX_PATH + ARRAY_SIZE( L"\\explorer.exe /desktop" )];
        WCHAR desktop[MAX_PATH];
        void *redir;

        SERVER_START_REQ( set_user_object_info )
        {
            req->handle = wine_server_obj_handle( NtUserGetThreadDesktop(GetCurrentThreadId()) );
            req->flags  = SET_USER_OBJECT_GET_FULL_NAME;
            wine_server_set_reply( req, desktop, sizeof(desktop) - sizeof(WCHAR) );
            if (!wine_server_call( req ))
            {
                size_t size = wine_server_reply_size( reply );
                desktop[size / sizeof(WCHAR)] = 0;
                TRACE( "starting explorer for desktop %s\n", debugstr_w(desktop) );
            }
            else
                desktop[0] = 0;
        }
        SERVER_END_REQ;

        memset( &si, 0, sizeof(si) );
        si.cb = sizeof(si);
        si.lpDesktop = *desktop ? desktop : NULL;
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput  = 0;
        si.hStdOutput = 0;
        si.hStdError  = GetStdHandle( STD_ERROR_HANDLE );

        GetSystemDirectoryW( windir, MAX_PATH );
        lstrcpyW( app, windir );
        lstrcatW( app, L"\\explorer.exe" );
        lstrcpyW( cmdline, app );
        lstrcatW( cmdline, L" /desktop" );

        Wow64DisableWow64FsRedirection( &redir );
        if (CreateProcessW( app, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS,
                            NULL, windir, &si, &pi ))
        {
            TRACE( "started explorer pid %04x tid %04x\n", pi.dwProcessId, pi.dwThreadId );
            WaitForInputIdle( pi.hProcess, 10000 );
            CloseHandle( pi.hThread );
            CloseHandle( pi.hProcess );
        }
        else WARN( "failed to start explorer, err %d\n", GetLastError() );
        Wow64RevertWow64FsRedirection( redir );

        SERVER_START_REQ( get_desktop_window )
        {
            req->force = 1;
            if (!wine_server_call( req ))
            {
                thread_info->top_window = wine_server_ptr_handle( reply->top_window );
                thread_info->msg_window = wine_server_ptr_handle( reply->msg_window );
            }
        }
        SERVER_END_REQ;
    }

    if (!thread_info->top_window || !USER_Driver->pCreateDesktopWindow( thread_info->top_window ))
        ERR( "failed to create desktop window\n" );

    register_builtin_classes();
    return thread_info->top_window;
}


/*******************************************************************
 *		EnableWindow (USER32.@)
 */
BOOL WINAPI EnableWindow( HWND hwnd, BOOL enable )
{
    BOOL retvalue;

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE("( %p, %d )\n", hwnd, enable);

    if (enable)
    {
        retvalue = (WIN_SetStyle( hwnd, 0, WS_DISABLED ) & WS_DISABLED) != 0;
        if (retvalue) SendMessageW( hwnd, WM_ENABLE, TRUE, 0 );
    }
    else
    {
        SendMessageW( hwnd, WM_CANCELMODE, 0, 0 );

        retvalue = (WIN_SetStyle( hwnd, WS_DISABLED, 0 ) & WS_DISABLED) != 0;
        if (!retvalue)
        {
            if (hwnd == GetFocus())
                SetFocus( 0 ); /* A disabled window can't have the focus */

            SendMessageW( hwnd, WM_ENABLE, FALSE, 0 );
        }
    }
    return retvalue;
}


/***********************************************************************
 *		IsWindowEnabled (USER32.@)
 */
BOOL WINAPI IsWindowEnabled(HWND hWnd)
{
    LONG ret;

    SetLastError(NO_ERROR);
    ret = GetWindowLongW( hWnd, GWL_STYLE );
    if (!ret && GetLastError() != NO_ERROR) return FALSE;
    return !(ret & WS_DISABLED);
}

/***********************************************************************
 *		IsWindowUnicode (USER32.@)
 */
BOOL WINAPI IsWindowUnicode( HWND hwnd )
{
    WND * wndPtr;
    BOOL retvalue = FALSE;

    if (!(wndPtr = WIN_GetPtr(hwnd))) return FALSE;

    if (wndPtr == WND_DESKTOP) return TRUE;

    if (wndPtr != WND_OTHER_PROCESS)
    {
        retvalue = (wndPtr->flags & WIN_ISUNICODE) != 0;
        WIN_ReleasePtr( wndPtr );
    }
    else
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            if (!wine_server_call_err( req )) retvalue = reply->is_unicode;
        }
        SERVER_END_REQ;
    }
    return retvalue;
}


/***********************************************************************
 *		GetWindowDpiAwarenessContext  (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext( HWND hwnd )
{
    WND *win;
    DPI_AWARENESS_CONTEXT ret = 0;

    if (!(win = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP) return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
    if (win != WND_OTHER_PROCESS)
    {
        ret = ULongToHandle( win->dpi_awareness | 0x10 );
        WIN_ReleasePtr( win );
    }
    else
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            if (!wine_server_call_err( req )) ret = ULongToHandle( reply->awareness | 0x10 );
        }
        SERVER_END_REQ;
    }
    return ret;
}


/***********************************************************************
 *              GetDpiForWindow   (USER32.@)
 */
UINT WINAPI GetDpiForWindow( HWND hwnd )
{
    WND *win;
    UINT ret = 0;

    if (!(win = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP)
    {
        POINT pt = { 0, 0 };
        return get_monitor_dpi( MonitorFromPoint( pt, MONITOR_DEFAULTTOPRIMARY ));
    }
    if (win != WND_OTHER_PROCESS)
    {
        ret = win->dpi;
        if (!ret) ret = get_win_monitor_dpi( hwnd );
        WIN_ReleasePtr( win );
    }
    else
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            if (!wine_server_call_err( req )) ret = reply->dpi;
        }
        SERVER_END_REQ;
    }
    return ret;
}


/**********************************************************************
 *	     WIN_GetWindowLong
 *
 * Helper function for GetWindowLong().
 */
static LONG_PTR WIN_GetWindowLong( HWND hwnd, INT offset, UINT size, BOOL unicode )
{
    LONG_PTR retvalue = 0;
    WND *wndPtr;

    if (offset == GWLP_HWNDPARENT)
    {
        HWND parent = GetAncestor( hwnd, GA_PARENT );
        if (parent == GetDesktopWindow()) parent = GetWindow( hwnd, GW_OWNER );
        return (ULONG_PTR)parent;
    }

    if (!(wndPtr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (wndPtr == WND_DESKTOP)
    {
        switch (offset)
        {
        case GWL_STYLE:
            retvalue = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN; /* message parent is not visible */
            if (WIN_GetFullHandle( hwnd ) == GetDesktopWindow())
                retvalue |= WS_VISIBLE;
            return retvalue;
        case GWL_EXSTYLE:
        case GWLP_USERDATA:
        case GWLP_ID:
        case GWLP_HINSTANCE:
            return 0;
        case GWLP_WNDPROC:
            SetLastError( ERROR_ACCESS_DENIED );
            return 0;
        }
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
    }

    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (offset == GWLP_WNDPROC)
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return 0;
        }
        SERVER_START_REQ( set_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            req->flags  = 0;  /* don't set anything, just retrieve */
            req->extra_offset = (offset >= 0) ? offset : -1;
            req->extra_size = (offset >= 0) ? size : 0;
            if (!wine_server_call_err( req ))
            {
                switch(offset)
                {
                case GWL_STYLE:      retvalue = reply->old_style; break;
                case GWL_EXSTYLE:    retvalue = reply->old_ex_style; break;
                case GWLP_ID:        retvalue = reply->old_id; break;
                case GWLP_HINSTANCE: retvalue = (ULONG_PTR)wine_server_get_ptr( reply->old_instance ); break;
                case GWLP_USERDATA:  retvalue = reply->old_user_data; break;
                default:
                    if (offset >= 0) retvalue = get_win_data( &reply->old_extra_value, size );
                    else SetLastError( ERROR_INVALID_INDEX );
                    break;
                }
            }
        }
        SERVER_END_REQ;
        return retvalue;
    }

    /* now we have a valid wndPtr */

    if (offset >= 0)
    {
        if (offset > (int)(wndPtr->cbWndExtra - size))
        {
            WARN("Invalid offset %d\n", offset );
            WIN_ReleasePtr( wndPtr );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        retvalue = get_win_data( (char *)wndPtr->wExtra + offset, size );

        /* Special case for dialog window procedure */
        if ((offset == DWLP_DLGPROC) && (size == sizeof(LONG_PTR)) && wndPtr->dlgInfo)
            retvalue = (LONG_PTR)WINPROC_GetProc( (WNDPROC)retvalue, unicode );
        WIN_ReleasePtr( wndPtr );
        return retvalue;
    }

    switch(offset)
    {
    case GWLP_USERDATA:  retvalue = wndPtr->userdata; break;
    case GWL_STYLE:      retvalue = wndPtr->dwStyle; break;
    case GWL_EXSTYLE:    retvalue = wndPtr->dwExStyle; break;
    case GWLP_ID:        retvalue = wndPtr->wIDmenu; break;
    case GWLP_HINSTANCE: retvalue = (ULONG_PTR)wndPtr->hInstance; break;
    case GWLP_WNDPROC:
        /* This looks like a hack only for the edit control (see tests). This makes these controls
         * more tolerant to A/W mismatches. The lack of W->A->W conversion for such a mismatch suggests
         * that the hack is in GetWindowLongPtr[AW], not in winprocs.
         */
        if (wndPtr->winproc == BUILTIN_WINPROC(WINPROC_EDIT) && (!unicode != !(wndPtr->flags & WIN_ISUNICODE)))
            retvalue = (ULONG_PTR)wndPtr->winproc;
        else
            retvalue = (ULONG_PTR)WINPROC_GetProc( wndPtr->winproc, unicode );
        break;
    default:
        WARN("Unknown offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    WIN_ReleasePtr(wndPtr);
    return retvalue;
}


/**********************************************************************
 *	     WIN_SetWindowLong
 *
 * Helper function for SetWindowLong().
 *
 * 0 is the failure code. However, in the case of failure SetLastError
 * must be set to distinguish between a 0 return value and a failure.
 */
LONG_PTR WIN_SetWindowLong( HWND hwnd, INT offset, UINT size, LONG_PTR newval, BOOL unicode )
{
    STYLESTRUCT style;
    BOOL ok, made_visible = FALSE;
    LONG_PTR retval = 0;
    WND *wndPtr;

    TRACE( "%p %d %lx %c\n", hwnd, offset, newval, unicode ? 'W' : 'A' );

    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(wndPtr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (wndPtr == WND_DESKTOP)
    {
        /* can't change anything on the desktop window */
        SetLastError( ERROR_ACCESS_DENIED );
        return 0;
    }
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (offset == GWLP_WNDPROC)
        {
            SetLastError( ERROR_ACCESS_DENIED );
            return 0;
        }
        if (offset > 32767 || offset < -32767)
        {
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        return SendMessageW( hwnd, WM_WINE_SETWINDOWLONG, MAKEWPARAM( offset, size ), newval );
    }

    /* first some special cases */
    switch( offset )
    {
    case GWL_STYLE:
        style.styleOld = wndPtr->dwStyle;
        style.styleNew = newval;
        WIN_ReleasePtr( wndPtr );
        SendMessageW( hwnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM)&style );
        if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
        newval = style.styleNew;
        /* WS_CLIPSIBLINGS can't be reset on top-level windows */
        if (wndPtr->parent == GetDesktopWindow()) newval |= WS_CLIPSIBLINGS;
        /* WS_MINIMIZE can't be reset */
        if (wndPtr->dwStyle & WS_MINIMIZE) newval |= WS_MINIMIZE;
        break;
    case GWL_EXSTYLE:
        style.styleOld = wndPtr->dwExStyle;
        style.styleNew = newval;
        WIN_ReleasePtr( wndPtr );
        SendMessageW( hwnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM)&style );
        if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return 0;
        /* WS_EX_TOPMOST can only be changed through SetWindowPos */
        newval = (style.styleNew & ~WS_EX_TOPMOST) | (wndPtr->dwExStyle & WS_EX_TOPMOST);
        newval = fix_exstyle(wndPtr->dwStyle, newval);
        break;
    case GWLP_HWNDPARENT:
        if (wndPtr->parent == GetDesktopWindow())
        {
            WIN_ReleasePtr( wndPtr );
            return (ULONG_PTR)WIN_SetOwner( hwnd, (HWND)newval );
        }
        else
        {
            WIN_ReleasePtr( wndPtr );
            return (ULONG_PTR)SetParent( hwnd, (HWND)newval );
        }
    case GWLP_WNDPROC:
    {
        WNDPROC proc;
        UINT old_flags = wndPtr->flags;
        retval = WIN_GetWindowLong( hwnd, offset, size, unicode );
        proc = WINPROC_AllocProc( (WNDPROC)newval, unicode );
        if (proc) wndPtr->winproc = proc;
        if (WINPROC_IsUnicode( proc, unicode )) wndPtr->flags |= WIN_ISUNICODE;
        else wndPtr->flags &= ~WIN_ISUNICODE;
        if (!((old_flags ^ wndPtr->flags) & WIN_ISUNICODE))
        {
            WIN_ReleasePtr( wndPtr );
            return retval;
        }
        /* update is_unicode flag on the server side */
        break;
    }
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_USERDATA:
        break;
    case DWLP_DLGPROC:
        if ((wndPtr->cbWndExtra - sizeof(LONG_PTR) >= DWLP_DLGPROC) &&
            (size == sizeof(LONG_PTR)) && wndPtr->dlgInfo)
        {
            WNDPROC *ptr = (WNDPROC *)((char *)wndPtr->wExtra + DWLP_DLGPROC);
            retval = (ULONG_PTR)WINPROC_GetProc( *ptr, unicode );
            *ptr = WINPROC_AllocProc( (WNDPROC)newval, unicode );
            WIN_ReleasePtr( wndPtr );
            return retval;
        }
        /* fall through */
    default:
        if (offset < 0 || offset > (int)(wndPtr->cbWndExtra - size))
        {
            WARN("Invalid offset %d\n", offset );
            WIN_ReleasePtr( wndPtr );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        else if (get_win_data( (char *)wndPtr->wExtra + offset, size ) == newval)
        {
            /* already set to the same value */
            WIN_ReleasePtr( wndPtr );
            return newval;
        }
        break;
    }

    SERVER_START_REQ( set_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->extra_offset = -1;
        switch(offset)
        {
        case GWL_STYLE:
            req->flags = SET_WIN_STYLE | SET_WIN_EXSTYLE;
            req->style = newval;
            req->ex_style = fix_exstyle(newval, wndPtr->dwExStyle);
            break;
        case GWL_EXSTYLE:
            req->flags = SET_WIN_EXSTYLE;
            req->ex_style = newval;
            break;
        case GWLP_ID:
            req->flags = SET_WIN_ID;
            req->id = newval;
            break;
        case GWLP_HINSTANCE:
            req->flags = SET_WIN_INSTANCE;
            req->instance = wine_server_client_ptr( (void *)newval );
            break;
        case GWLP_WNDPROC:
            req->flags = SET_WIN_UNICODE;
            req->is_unicode = (wndPtr->flags & WIN_ISUNICODE) != 0;
            break;
        case GWLP_USERDATA:
            req->flags = SET_WIN_USERDATA;
            req->user_data = newval;
            break;
        default:
            req->flags = SET_WIN_EXTRA;
            req->extra_offset = offset;
            req->extra_size = size;
            set_win_data( &req->extra_value, newval, size );
        }
        if ((ok = !wine_server_call_err( req )))
        {
            switch(offset)
            {
            case GWL_STYLE:
                wndPtr->dwStyle = newval;
                wndPtr->dwExStyle = fix_exstyle(wndPtr->dwStyle, wndPtr->dwExStyle);
                retval = reply->old_style;
                break;
            case GWL_EXSTYLE:
                wndPtr->dwExStyle = newval;
                retval = reply->old_ex_style;
                break;
            case GWLP_ID:
                wndPtr->wIDmenu = newval;
                retval = reply->old_id;
                break;
            case GWLP_HINSTANCE:
                wndPtr->hInstance = (HINSTANCE)newval;
                retval = (ULONG_PTR)wine_server_get_ptr( reply->old_instance );
                break;
            case GWLP_WNDPROC:
                break;
            case GWLP_USERDATA:
                wndPtr->userdata = newval;
                retval = reply->old_user_data;
                break;
            default:
                retval = get_win_data( (char *)wndPtr->wExtra + offset, size );
                set_win_data( (char *)wndPtr->wExtra + offset, newval, size );
                break;
            }
        }
    }
    SERVER_END_REQ;

    if ((offset == GWL_STYLE && ((style.styleOld ^ style.styleNew) & WS_VISIBLE)) ||
        (offset == GWL_EXSTYLE && ((style.styleOld ^ style.styleNew) & WS_EX_LAYERED)))
    {
        made_visible = (wndPtr->dwStyle & WS_VISIBLE) != 0;
        invalidate_dce( wndPtr, NULL );
    }
    WIN_ReleasePtr( wndPtr );

    if (!ok) return 0;

    if (offset == GWL_STYLE || offset == GWL_EXSTYLE)
    {
        style.styleOld = retval;
        style.styleNew = newval;
        USER_Driver->pSetWindowStyle( hwnd, offset, &style );
        if (made_visible) update_window_state( hwnd );
        SendMessageW( hwnd, WM_STYLECHANGED, offset, (LPARAM)&style );
    }

    return retval;
}


/**********************************************************************
 *		GetWindowWord (USER32.@)
 */
WORD WINAPI GetWindowWord( HWND hwnd, INT offset )
{
    switch(offset)
    {
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        break;
    default:
        if (offset < 0)
        {
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        break;
    }
    return WIN_GetWindowLong( hwnd, offset, sizeof(WORD), FALSE );
}


/**********************************************************************
 *		GetWindowLongA (USER32.@)
 */
LONG WINAPI GetWindowLongA( HWND hwnd, INT offset )
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        return WIN_GetWindowLong( hwnd, offset, sizeof(LONG), FALSE );
    }
}


/**********************************************************************
 *		GetWindowLongW (USER32.@)
 */
LONG WINAPI GetWindowLongW( HWND hwnd, INT offset )
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        return WIN_GetWindowLong( hwnd, offset, sizeof(LONG), TRUE );
    }
}


/**********************************************************************
 *		SetWindowWord (USER32.@)
 */
WORD WINAPI SetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    switch(offset)
    {
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        break;
    default:
        if (offset < 0)
        {
            WARN("Invalid offset %d\n", offset );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        break;
    }
    return WIN_SetWindowLong( hwnd, offset, sizeof(WORD), newval, FALSE );
}


/**********************************************************************
 *		SetWindowLongA (USER32.@)
 *
 * See SetWindowLongW.
 */
LONG WINAPI DECLSPEC_HOTPATCH SetWindowLongA( HWND hwnd, INT offset, LONG newval )
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        return WIN_SetWindowLong( hwnd, offset, sizeof(LONG), newval, FALSE );
    }
}


/**********************************************************************
 *		SetWindowLongW (USER32.@) Set window attribute
 *
 * SetWindowLong() alters one of a window's attributes or sets a 32-bit (long)
 * value in a window's extra memory.
 *
 * The _hwnd_ parameter specifies the handle to a window that
 * has extra memory. The _newval_ parameter contains the new
 * attribute or extra memory value.  If positive, the _offset_
 * parameter is the byte-addressed location in the window's extra
 * memory to set.  If negative, _offset_ specifies the window
 * attribute to set, and should be one of the following values:
 *
 * GWL_EXSTYLE      The window's extended window style
 *
 * GWL_STYLE        The window's window style.
 *
 * GWLP_WNDPROC     Pointer to the window's window procedure.
 *
 * GWLP_HINSTANCE   The window's application instance handle.
 *
 * GWLP_ID          The window's identifier.
 *
 * GWLP_USERDATA    The window's user-specified data.
 *
 * If the window is a dialog box, the _offset_ parameter can be one of
 * the following values:
 *
 * DWLP_DLGPROC     The address of the window's dialog box procedure.
 *
 * DWLP_MSGRESULT   The return value of a message
 *                  that the dialog box procedure processed.
 *
 * DWLP_USER        Application specific information.
 *
 * RETURNS
 *
 * If successful, returns the previous value located at _offset_. Otherwise,
 * returns 0.
 *
 * NOTES
 *
 * Extra memory for a window class is specified by a nonzero cbWndExtra
 * parameter of the WNDCLASS structure passed to RegisterClass() at the
 * time of class creation.
 *
 * Using GWL_WNDPROC to set a new window procedure effectively creates
 * a window subclass. Use CallWindowProc() in the new windows procedure
 * to pass messages to the superclass's window procedure.
 *
 * The user data is reserved for use by the application which created
 * the window.
 *
 * Do not use GWL_STYLE to change the window's WS_DISABLED style;
 * instead, call the EnableWindow() function to change the window's
 * disabled state.
 *
 * Do not use GWL_HWNDPARENT to reset the window's parent, use
 * SetParent() instead.
 *
 * Win95:
 * When offset is GWL_STYLE and the calling app's ver is 4.0,
 * it sends WM_STYLECHANGING before changing the settings
 * and WM_STYLECHANGED afterwards.
 * App ver 4.0 can't use SetWindowLong to change WS_EX_TOPMOST.
 */
LONG WINAPI DECLSPEC_HOTPATCH SetWindowLongW(
    HWND hwnd,  /* [in] window to alter */
    INT offset, /* [in] offset, in bytes, of location to alter */
    LONG newval /* [in] new value of location */
)
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN("Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        return WIN_SetWindowLong( hwnd, offset, sizeof(LONG), newval, TRUE );
    }
}


/*******************************************************************
 *		GetWindowTextA (USER32.@)
 */
INT WINAPI GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    WCHAR *buffer;
    int ret = 0;

    if (!lpString || nMaxCount <= 0) return 0;

    __TRY
    {
        lpString[0] = 0;

        if (WIN_IsCurrentProcess( hwnd ))
        {
            ret = (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
        }
        else if ((buffer = HeapAlloc( GetProcessHeap(), 0, nMaxCount * sizeof(WCHAR) )))
        {
            /* when window belongs to other process, don't send a message */
            get_server_window_text( hwnd, buffer, nMaxCount );
            if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, lpString, nMaxCount, NULL, NULL ))
                lpString[nMaxCount-1] = 0;
            HeapFree( GetProcessHeap(), 0, buffer );
            ret = strlen(lpString);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = 0;
    }
    __ENDTRY

    return ret;
}


/*******************************************************************
 *		InternalGetWindowText (USER32.@)
 */
INT WINAPI InternalGetWindowText(HWND hwnd,LPWSTR lpString,INT nMaxCount )
{
    WND *win;

    if (nMaxCount <= 0) return 0;
    if (!(win = WIN_GetPtr( hwnd ))) return 0;
    if (win == WND_DESKTOP) lpString[0] = 0;
    else if (win != WND_OTHER_PROCESS)
    {
        if (win->text) lstrcpynW( lpString, win->text, nMaxCount );
        else lpString[0] = 0;
        WIN_ReleasePtr( win );
    }
    else
    {
        get_server_window_text( hwnd, lpString, nMaxCount );
    }
    return lstrlenW(lpString);
}


/*******************************************************************
 *		GetWindowTextW (USER32.@)
 */
INT WINAPI GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    int ret;

    if (!lpString || nMaxCount <= 0) return 0;

    __TRY
    {
        lpString[0] = 0;

        if (WIN_IsCurrentProcess( hwnd ))
        {
            ret = (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
        }
        else
        {
            /* when window belongs to other process, don't send a message */
            get_server_window_text( hwnd, lpString, nMaxCount );
            ret = lstrlenW(lpString);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = 0;
    }
    __ENDTRY

    return ret;
}


/*******************************************************************
 *		SetWindowTextA (USER32.@)
 *		SetWindowText  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
        WARN( "setting text %s of other process window %p should not use SendMessage\n",
               debugstr_a(lpString), hwnd );
    return (BOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowTextW (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
        WARN( "setting text %s of other process window %p should not use SendMessage\n",
               debugstr_w(lpString), hwnd );
    return (BOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		GetWindowTextLengthA (USER32.@)
 */
INT WINAPI GetWindowTextLengthA( HWND hwnd )
{
    CPINFO info;

    if (WIN_IsCurrentProcess( hwnd )) return SendMessageA( hwnd, WM_GETTEXTLENGTH, 0, 0 );

    /* when window belongs to other process, don't send a message */
    GetCPInfo( CP_ACP, &info );
    return get_server_window_text( hwnd, NULL, 0 ) * info.MaxCharSize;
}

/*******************************************************************
 *		GetWindowTextLengthW (USER32.@)
 */
INT WINAPI GetWindowTextLengthW( HWND hwnd )
{
    if (WIN_IsCurrentProcess( hwnd )) return SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 );

    /* when window belongs to other process, don't send a message */
    return get_server_window_text( hwnd, NULL, 0 );
}


/*******************************************************************
 *		IsWindow (USER32.@)
 */
BOOL WINAPI IsWindow( HWND hwnd )
{
    WND *ptr;
    BOOL ret;

    if (!(ptr = WIN_GetPtr( hwnd ))) return FALSE;
    if (ptr == WND_DESKTOP) return TRUE;

    if (ptr != WND_OTHER_PROCESS)
    {
        WIN_ReleasePtr( ptr );
        return TRUE;
    }

    /* check other processes */
    SERVER_START_REQ( get_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		GetWindowThreadProcessId (USER32.@)
 */
DWORD WINAPI GetWindowThreadProcessId( HWND hwnd, LPDWORD process )
{
    WND *ptr;
    DWORD tid = 0;

    if (!(ptr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE);
        return 0;
    }

    if (ptr != WND_OTHER_PROCESS && ptr != WND_DESKTOP)
    {
        /* got a valid window */
        tid = ptr->tid;
        if (process) *process = GetCurrentProcessId();
        WIN_ReleasePtr( ptr );
        return tid;
    }

    /* check other processes */
    SERVER_START_REQ( get_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (!wine_server_call_err( req ))
        {
            tid = (DWORD)reply->tid;
            if (process) *process = (DWORD)reply->pid;
        }
    }
    SERVER_END_REQ;
    return tid;
}


/*****************************************************************
 *		GetParent (USER32.@)
 */
HWND WINAPI GetParent( HWND hwnd )
{
    WND *wndPtr;
    HWND retvalue = 0;

    if (!(wndPtr = WIN_GetPtr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (wndPtr == WND_DESKTOP) return 0;
    if (wndPtr == WND_OTHER_PROCESS)
    {
        LONG style = GetWindowLongW( hwnd, GWL_STYLE );
        if (style & (WS_POPUP | WS_CHILD))
        {
            SERVER_START_REQ( get_window_tree )
            {
                req->handle = wine_server_user_handle( hwnd );
                if (!wine_server_call_err( req ))
                {
                    if (style & WS_POPUP) retvalue = wine_server_ptr_handle( reply->owner );
                    else if (style & WS_CHILD) retvalue = wine_server_ptr_handle( reply->parent );
                }
            }
            SERVER_END_REQ;
        }
    }
    else
    {
        if (wndPtr->dwStyle & WS_POPUP) retvalue = wndPtr->owner;
        else if (wndPtr->dwStyle & WS_CHILD) retvalue = wndPtr->parent;
        WIN_ReleasePtr( wndPtr );
    }
    return retvalue;
}


/*****************************************************************
 *		GetAncestor (USER32.@)
 */
HWND WINAPI GetAncestor( HWND hwnd, UINT type )
{
    WND *win;
    HWND *list, ret = 0;

    switch(type)
    {
    case GA_PARENT:
        if (!(win = WIN_GetPtr( hwnd )))
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (win == WND_DESKTOP) return 0;
        if (win != WND_OTHER_PROCESS)
        {
            ret = win->parent;
            WIN_ReleasePtr( win );
        }
        else /* need to query the server */
        {
            SERVER_START_REQ( get_window_tree )
            {
                req->handle = wine_server_user_handle( hwnd );
                if (!wine_server_call_err( req )) ret = wine_server_ptr_handle( reply->parent );
            }
            SERVER_END_REQ;
        }
        break;

    case GA_ROOT:
        if (!(list = list_window_parents( hwnd ))) return 0;

        if (!list[0] || !list[1]) ret = WIN_GetFullHandle( hwnd );  /* top-level window */
        else
        {
            int count = 2;
            while (list[count]) count++;
            ret = list[count - 2];  /* get the one before the desktop */
        }
        HeapFree( GetProcessHeap(), 0, list );
        break;

    case GA_ROOTOWNER:
        if (is_desktop_window( hwnd )) return 0;
        ret = WIN_GetFullHandle( hwnd );
        for (;;)
        {
            HWND parent = GetParent( ret );
            if (!parent) break;
            ret = parent;
        }
        break;
    }
    return ret;
}


/*****************************************************************
 *		SetParent (USER32.@)
 */
HWND WINAPI SetParent( HWND hwnd, HWND parent )
{
    WINDOWPOS winpos;
    HWND full_handle;
    HWND old_parent = 0;
    BOOL was_visible;
    WND *wndPtr;
    BOOL ret;
    DPI_AWARENESS_CONTEXT context;
    RECT window_rect, old_screen_rect, new_screen_rect;

    TRACE("(%p %p)\n", hwnd, parent);

    if (is_broadcast(hwnd) || is_broadcast(parent))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!parent) parent = GetDesktopWindow();
    else if (parent == HWND_MESSAGE) parent = get_hwnd_message_parent();
    else parent = WIN_GetFullHandle( parent );

    if (!IsWindow( parent ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    /* Some applications try to set a child as a parent */
    if (IsChild(hwnd, parent))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(full_handle = WIN_IsCurrentThread( hwnd )))
        return (HWND)SendMessageW( hwnd, WM_WINE_SETPARENT, (WPARAM)parent, 0 );

    if (full_handle == parent)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* Windows hides the window first, then shows it again
     * including the WM_SHOWWINDOW messages and all */
    was_visible = ShowWindow( hwnd, SW_HIDE );

    wndPtr = WIN_GetPtr( hwnd );
    if (!wndPtr || wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return 0;

    context = SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));
    WIN_GetRectangles( hwnd, COORDS_PARENT, &window_rect, NULL );
    SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    WIN_GetRectangles( hwnd, COORDS_SCREEN, &old_screen_rect, NULL );
    SetThreadDpiAwarenessContext( context );

    SERVER_START_REQ( set_parent )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->parent = wine_server_user_handle( parent );
        if ((ret = !wine_server_call( req )))
        {
            old_parent = wine_server_ptr_handle( reply->old_parent );
            wndPtr->parent = parent = wine_server_ptr_handle( reply->full_parent );
            wndPtr->dpi = reply->dpi;
            wndPtr->dpi_awareness = reply->awareness;
        }

    }
    SERVER_END_REQ;
    WIN_ReleasePtr( wndPtr );
    if (!ret) return 0;

    context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    WIN_GetRectangles( hwnd, COORDS_SCREEN, &new_screen_rect, NULL );
    SetThreadDpiAwarenessContext( GetWindowDpiAwarenessContext( hwnd ));

    USER_Driver->pSetParent( full_handle, parent, old_parent );

    winpos.hwnd = hwnd;
    winpos.hwndInsertAfter = HWND_TOP;
    winpos.x = window_rect.left;
    winpos.y = window_rect.top;
    winpos.cx = 0;
    winpos.cy = 0;
    winpos.flags = SWP_NOSIZE;

    USER_SetWindowPos( &winpos, new_screen_rect.left - old_screen_rect.left,
                       new_screen_rect.top - old_screen_rect.top );

    if (was_visible) ShowWindow( hwnd, SW_SHOW );

    SetThreadDpiAwarenessContext( context );
    return old_parent;
}


/*******************************************************************
 *		IsChild (USER32.@)
 */
BOOL WINAPI IsChild( HWND parent, HWND child )
{
    HWND *list;
    int i;
    BOOL ret = FALSE;

    if (!(GetWindowLongW( child, GWL_STYLE ) & WS_CHILD)) return FALSE;
    if (!(list = list_window_parents( child ))) return FALSE;
    parent = WIN_GetFullHandle( parent );
    for (i = 0; list[i]; i++)
    {
        if (list[i] == parent)
        {
            ret = list[i] && list[i+1];
            break;
        }
        if (!(GetWindowLongW( list[i], GWL_STYLE ) & WS_CHILD)) break;
    }
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/***********************************************************************
 *		IsWindowVisible (USER32.@)
 */
BOOL WINAPI IsWindowVisible( HWND hwnd )
{
    HWND *list;
    BOOL retval = TRUE;
    int i;

    if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_VISIBLE)) return FALSE;
    if (!(list = list_window_parents( hwnd ))) return TRUE;
    if (list[0])
    {
        for (i = 0; list[i+1]; i++)
            if (!(GetWindowLongW( list[i], GWL_STYLE ) & WS_VISIBLE)) break;
        retval = !list[i+1] && (list[i] == GetDesktopWindow());  /* top message window isn't visible */
    }
    HeapFree( GetProcessHeap(), 0, list );
    return retval;
}


/***********************************************************************
 *           WIN_IsWindowDrawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
BOOL WIN_IsWindowDrawable( HWND hwnd, BOOL icon )
{
    HWND *list;
    BOOL retval = TRUE;
    int i;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & WS_MINIMIZE) && icon && GetClassLongPtrW( hwnd, GCLP_HICON ))  return FALSE;

    if (!(list = list_window_parents( hwnd ))) return TRUE;
    if (list[0])
    {
        for (i = 0; list[i+1]; i++)
            if ((GetWindowLongW( list[i], GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != WS_VISIBLE)
                break;
        retval = !list[i+1] && (list[i] == GetDesktopWindow());  /* top message window isn't visible */
    }
    HeapFree( GetProcessHeap(), 0, list );
    return retval;
}


/*******************************************************************
 *		GetTopWindow (USER32.@)
 */
HWND WINAPI GetTopWindow( HWND hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow();
    return GetWindow( hwnd, GW_CHILD );
}


/*******************************************************************
 *		GetWindow (USER32.@)
 */
HWND WINAPI GetWindow( HWND hwnd, UINT rel )
{
    HWND retval = 0;

    if (rel == GW_OWNER)  /* this one may be available locally */
    {
        WND *wndPtr = WIN_GetPtr( hwnd );
        if (!wndPtr)
        {
            SetLastError( ERROR_INVALID_HANDLE );
            return 0;
        }
        if (wndPtr == WND_DESKTOP) return 0;
        if (wndPtr != WND_OTHER_PROCESS)
        {
            retval = wndPtr->owner;
            WIN_ReleasePtr( wndPtr );
            return retval;
        }
        /* else fall through to server call */
    }

    SERVER_START_REQ( get_window_tree )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (!wine_server_call_err( req ))
        {
            switch(rel)
            {
            case GW_HWNDFIRST:
                retval = wine_server_ptr_handle( reply->first_sibling );
                break;
            case GW_HWNDLAST:
                retval = wine_server_ptr_handle( reply->last_sibling );
                break;
            case GW_HWNDNEXT:
                retval = wine_server_ptr_handle( reply->next_sibling );
                break;
            case GW_HWNDPREV:
                retval = wine_server_ptr_handle( reply->prev_sibling );
                break;
            case GW_OWNER:
                retval = wine_server_ptr_handle( reply->owner );
                break;
            case GW_CHILD:
                retval = wine_server_ptr_handle( reply->first_child );
                break;
            }
        }
    }
    SERVER_END_REQ;
    return retval;
}


/*******************************************************************
 *		ShowOwnedPopups (USER32.@)
 */
BOOL WINAPI ShowOwnedPopups( HWND owner, BOOL fShow )
{
    int count = 0;
    HWND *win_array = WIN_ListChildren( GetDesktopWindow() );

    if (!win_array) return TRUE;

    while (win_array[count]) count++;
    while (--count >= 0)
    {
        if (GetWindow( win_array[count], GW_OWNER ) != owner) continue;
        if (fShow)
        {
            if (win_get_flags( win_array[count] ) & WIN_NEEDS_SHOW_OWNEDPOPUP)
                /* In Windows, ShowOwnedPopups(TRUE) generates
                 * WM_SHOWWINDOW messages with SW_PARENTOPENING,
                 * regardless of the state of the owner
                 */
                SendMessageW(win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING);
        }
        else
        {
            if (GetWindowLongW( win_array[count], GWL_STYLE ) & WS_VISIBLE)
                /* In Windows, ShowOwnedPopups(FALSE) generates
                 * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
                 * regardless of the state of the owner
                 */
                SendMessageW(win_array[count], WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING);
        }
    }
    HeapFree( GetProcessHeap(), 0, win_array );
    return TRUE;
}


/*******************************************************************
 *		GetLastActivePopup (USER32.@)
 */
HWND WINAPI GetLastActivePopup( HWND hwnd )
{
    HWND retval = hwnd;

    SERVER_START_REQ( get_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        if (!wine_server_call_err( req )) retval = wine_server_ptr_handle( reply->last_active );
    }
    SERVER_END_REQ;
    return retval;
}


/*******************************************************************
 *           WIN_ListChildren
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
HWND *WIN_ListChildren( HWND hwnd )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return NULL;
    }
    return list_window_children( 0, hwnd, NULL, 0 );
}


/*******************************************************************
 *		EnumWindows (USER32.@)
 */
BOOL WINAPI EnumWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    HWND *list;
    BOOL ret = TRUE;
    int i;

    USER_CheckNotLock();

    /* We have to build a list of all windows first, to avoid */
    /* unpleasant side-effects, for instance if the callback */
    /* function changes the Z-order of the windows.          */

    if (!(list = WIN_ListChildren( GetDesktopWindow() ))) return TRUE;

    /* Now call the callback function for every window */

    for (i = 0; list[i]; i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( list[i] )) continue;
        if (!(ret = lpEnumFunc( list[i], lParam ))) break;
    }
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/**********************************************************************
 *		EnumThreadWindows (USER32.@)
 */
BOOL WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC func, LPARAM lParam )
{
    HWND *list;
    int i;
    BOOL ret = TRUE;

    USER_CheckNotLock();

    if (!(list = list_window_children( 0, GetDesktopWindow(), NULL, id ))) return TRUE;

    /* Now call the callback function for every window */

    for (i = 0; list[i]; i++)
        if (!(ret = func( list[i], lParam ))) break;
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/***********************************************************************
 *              EnumDesktopWindows   (USER32.@)
 */
BOOL WINAPI EnumDesktopWindows( HDESK desktop, WNDENUMPROC func, LPARAM lparam )
{
    HWND *list;
    int i;

    USER_CheckNotLock();

    if (!(list = list_window_children( desktop, 0, NULL, 0 ))) return TRUE;

    for (i = 0; list[i]; i++)
        if (!func( list[i], lparam )) break;
    HeapFree( GetProcessHeap(), 0, list );
    return TRUE;
}


#ifdef __i386__
/* Some apps pass a non-stdcall proc to EnumChildWindows,
 * so we need a small assembly wrapper to call the proc.
 */
extern LRESULT enum_callback_wrapper( WNDENUMPROC proc, HWND hwnd, LPARAM lparam );
__ASM_GLOBAL_FUNC( enum_callback_wrapper,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "pushl 16(%ebp)\n\t"
    "pushl 12(%ebp)\n\t"
    "call *8(%ebp)\n\t"
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#else
static inline LRESULT enum_callback_wrapper( WNDENUMPROC proc, HWND hwnd, LPARAM lparam )
{
    return proc( hwnd, lparam );
}
#endif /* __i386__ */

/**********************************************************************
 *           WIN_EnumChildWindows
 *
 * Helper function for EnumChildWindows().
 */
static BOOL WIN_EnumChildWindows( HWND *list, WNDENUMPROC func, LPARAM lParam )
{
    HWND *childList;
    BOOL ret = FALSE;

    for ( ; *list; list++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( *list )) continue;
        /* Build children list first */
        childList = WIN_ListChildren( *list );

        ret = enum_callback_wrapper( func, *list, lParam );

        if (childList)
        {
            if (ret) ret = WIN_EnumChildWindows( childList, func, lParam );
            HeapFree( GetProcessHeap(), 0, childList );
        }
        if (!ret) return FALSE;
    }
    return TRUE;
}


/**********************************************************************
 *		EnumChildWindows (USER32.@)
 */
BOOL WINAPI EnumChildWindows( HWND parent, WNDENUMPROC func, LPARAM lParam )
{
    HWND *list;
    BOOL ret;

    USER_CheckNotLock();

    if (!(list = WIN_ListChildren( parent ))) return FALSE;
    ret = WIN_EnumChildWindows( list, func, lParam );
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/*******************************************************************
 *		AnyPopup (USER32.@)
 */
BOOL WINAPI AnyPopup(void)
{
    int i;
    BOOL retvalue;
    HWND *list = WIN_ListChildren( GetDesktopWindow() );

    if (!list) return FALSE;
    for (i = 0; list[i]; i++)
    {
        if (IsWindowVisible( list[i] ) && GetWindow( list[i], GW_OWNER )) break;
    }
    retvalue = (list[i] != 0);
    HeapFree( GetProcessHeap(), 0, list );
    return retvalue;
}


/*******************************************************************
 *		FlashWindow (USER32.@)
 */
BOOL WINAPI FlashWindow( HWND hWnd, BOOL bInvert )
{
    FLASHWINFO finfo;

    finfo.cbSize = sizeof(FLASHWINFO);
    finfo.dwFlags = bInvert ? FLASHW_ALL : FLASHW_STOP;
    finfo.uCount = 1;
    finfo.dwTimeout = 0;
    finfo.hwnd = hWnd;
    return FlashWindowEx( &finfo );
}

/*******************************************************************
 *		FlashWindowEx (USER32.@)
 */
BOOL WINAPI FlashWindowEx( PFLASHWINFO pfinfo )
{
    WND *wndPtr;

    TRACE( "%p\n", pfinfo );

    if (!pfinfo)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!pfinfo->hwnd || pfinfo->cbSize != sizeof(FLASHWINFO) || !IsWindow( pfinfo->hwnd ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    FIXME( "%p - semi-stub\n", pfinfo );

    if (IsIconic( pfinfo->hwnd ))
    {
        RedrawWindow( pfinfo->hwnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME );

        wndPtr = WIN_GetPtr( pfinfo->hwnd );
        if (!wndPtr || wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return FALSE;
        if (pfinfo->dwFlags && !(wndPtr->flags & WIN_NCACTIVATED))
        {
            wndPtr->flags |= WIN_NCACTIVATED;
        }
        else
        {
            wndPtr->flags &= ~WIN_NCACTIVATED;
        }
        WIN_ReleasePtr( wndPtr );
        USER_Driver->pFlashWindowEx( pfinfo );
        return TRUE;
    }
    else
    {
        WPARAM wparam;
        HWND hwnd = pfinfo->hwnd;

        wndPtr = WIN_GetPtr( hwnd );
        if (!wndPtr || wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return FALSE;
        hwnd = wndPtr->obj.handle;  /* make it a full handle */

        if (pfinfo->dwFlags) wparam = !(wndPtr->flags & WIN_NCACTIVATED);
        else wparam = (hwnd == GetForegroundWindow());

        WIN_ReleasePtr( wndPtr );
        SendMessageW( hwnd, WM_NCACTIVATE, wparam, 0 );
        USER_Driver->pFlashWindowEx( pfinfo );
        return wparam;
    }
}

/*******************************************************************
 *		GetWindowContextHelpId (USER32.@)
 */
DWORD WINAPI GetWindowContextHelpId( HWND hwnd )
{
    DWORD retval;
    WND *wnd = WIN_GetPtr( hwnd );
    if (!wnd || wnd == WND_DESKTOP) return 0;
    if (wnd == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return 0;
    }
    retval = wnd->helpContext;
    WIN_ReleasePtr( wnd );
    return retval;
}


/*******************************************************************
 *		SetWindowContextHelpId (USER32.@)
 */
BOOL WINAPI SetWindowContextHelpId( HWND hwnd, DWORD id )
{
    WND *wnd = WIN_GetPtr( hwnd );
    if (!wnd || wnd == WND_DESKTOP) return FALSE;
    if (wnd == WND_OTHER_PROCESS)
    {
        if (IsWindow( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return FALSE;
    }
    wnd->helpContext = id;
    WIN_ReleasePtr( wnd );
    return TRUE;
}


/*******************************************************************
 *		DragDetect (USER32.@)
 */
BOOL WINAPI DragDetect( HWND hWnd, POINT pt )
{
    MSG msg;
    RECT rect;
    WORD wDragWidth, wDragHeight;

    TRACE( "%p,%s\n", hWnd, wine_dbgstr_point( &pt ) );

    if (!(NtUserGetKeyState( VK_LBUTTON ) & 0x8000))
        return FALSE;

    wDragWidth = GetSystemMetrics(SM_CXDRAG);
    wDragHeight= GetSystemMetrics(SM_CYDRAG);
    SetRect(&rect, pt.x - wDragWidth, pt.y - wDragHeight, pt.x + wDragWidth, pt.y + wDragHeight);

    SetCapture(hWnd);

    while(1)
    {
        while (PeekMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ))
        {
            if( msg.message == WM_LBUTTONUP )
            {
                ReleaseCapture();
                return FALSE;
            }
            if( msg.message == WM_MOUSEMOVE )
            {
                POINT tmp;
                tmp.x = (short)LOWORD(msg.lParam);
                tmp.y = (short)HIWORD(msg.lParam);
                if( !PtInRect( &rect, tmp ))
                {
                    ReleaseCapture();
                    return TRUE;
                }
            }
        }
        WaitMessage();
    }
    return FALSE;
}

/******************************************************************************
 *		GetWindowModuleFileNameA (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameA( HWND hwnd, LPSTR module, UINT size )
{
    WND *win;
    HINSTANCE hinst;

    TRACE( "%p, %p, %u\n", hwnd, module, size );

    win = WIN_GetPtr( hwnd );
    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    hinst = win->hInstance;
    WIN_ReleasePtr( win );

    return GetModuleFileNameA( hinst, module, size );
}

/******************************************************************************
 *		GetWindowModuleFileNameW (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameW( HWND hwnd, LPWSTR module, UINT size )
{
    WND *win;
    HINSTANCE hinst;

    TRACE( "%p, %p, %u\n", hwnd, module, size );

    win = WIN_GetPtr( hwnd );
    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    hinst = win->hInstance;
    WIN_ReleasePtr( win );

    return GetModuleFileNameW( hinst, module, size );
}

/******************************************************************************
 *              GetWindowInfo (USER32.@)
 *
 * Note: tests show that Windows doesn't check cbSize of the structure.
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetWindowInfo( HWND hwnd, PWINDOWINFO pwi)
{
    RECT rcWindow, rcClient;

    if (!WIN_GetRectangles( hwnd, COORDS_SCREEN, &rcWindow, &rcClient )) return FALSE;
    if (!pwi) return FALSE;

    pwi->rcWindow = rcWindow;
    pwi->rcClient = rcClient;
    pwi->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    pwi->dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    pwi->dwWindowStatus = ((GetActiveWindow() == hwnd) ? WS_ACTIVECAPTION : 0);

    pwi->cxWindowBorders = pwi->rcClient.left - pwi->rcWindow.left;
    pwi->cyWindowBorders = pwi->rcWindow.bottom - pwi->rcClient.bottom;

    pwi->atomWindowType = GetClassLongW( hwnd, GCW_ATOM );
    pwi->wCreatorVersion = 0x0400;

    return TRUE;
}

/******************************************************************************
 *              SwitchDesktop (USER32.@)
 *
 * NOTES: Sets the current input or interactive desktop.
 */
BOOL WINAPI SwitchDesktop( HDESK hDesktop)
{
    FIXME("(hwnd %p) stub!\n", hDesktop);
    return TRUE;
}


/***********************************************************************
 *           __wine_set_pixel_format
 */
BOOL CDECL __wine_set_pixel_format( HWND hwnd, int format )
{
    WND *win = WIN_GetPtr( hwnd );

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        WARN( "setting format %d on win %p not supported\n", format, hwnd );
        return FALSE;
    }
    win->pixel_format = format;
    WIN_ReleasePtr( win );

    update_window_state( hwnd );
    return TRUE;
}


/*****************************************************************************
 *              SetLayeredWindowAttributes (USER32.@)
 */
BOOL WINAPI SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    BOOL ret;

    TRACE("(%p,%08x,%d,%x)\n", hwnd, key, alpha, flags);

    SERVER_START_REQ( set_window_layered_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->color_key = key;
        req->alpha = alpha;
        req->flags = flags;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (ret)
    {
        USER_Driver->pSetLayeredWindowAttributes( hwnd, key, alpha, flags );
        update_window_state( hwnd );
    }

    return ret;
}


/*****************************************************************************
 *              UpdateLayeredWindowIndirect  (USER32.@)
 */
BOOL WINAPI UpdateLayeredWindowIndirect( HWND hwnd, const UPDATELAYEREDWINDOWINFO *info )
{
    DWORD flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
    RECT window_rect, client_rect;
    SIZE offset;

    if (!info ||
        info->cbSize != sizeof(*info) ||
        info->dwFlags & ~(ULW_COLORKEY | ULW_ALPHA | ULW_OPAQUE | ULW_EX_NORESIZE) ||
        !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED) ||
        NtUserGetLayeredWindowAttributes( hwnd, NULL, NULL, NULL ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    WIN_GetRectangles( hwnd, COORDS_PARENT, &window_rect, &client_rect );

    if (info->pptDst)
    {
        offset.cx = info->pptDst->x - window_rect.left;
        offset.cy = info->pptDst->y - window_rect.top;
        OffsetRect( &client_rect, offset.cx, offset.cy );
        OffsetRect( &window_rect, offset.cx, offset.cy );
        flags &= ~SWP_NOMOVE;
    }
    if (info->psize)
    {
        offset.cx = info->psize->cx - (window_rect.right - window_rect.left);
        offset.cy = info->psize->cy - (window_rect.bottom - window_rect.top);
        if (info->psize->cx <= 0 || info->psize->cy <= 0)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if ((info->dwFlags & ULW_EX_NORESIZE) && (offset.cx || offset.cy))
        {
            SetLastError( ERROR_INCORRECT_SIZE );
            return FALSE;
        }
        client_rect.right  += offset.cx;
        client_rect.bottom += offset.cy;
        window_rect.right  += offset.cx;
        window_rect.bottom += offset.cy;
        flags &= ~SWP_NOSIZE;
    }

    TRACE( "window %p win %s client %s\n", hwnd,
           wine_dbgstr_rect(&window_rect), wine_dbgstr_rect(&client_rect) );

    set_window_pos( hwnd, 0, flags, &window_rect, &client_rect, NULL );
    return USER_Driver->pUpdateLayeredWindow( hwnd, info, &window_rect );
}


/*****************************************************************************
 *              UpdateLayeredWindow (USER32.@)
 */
BOOL WINAPI UpdateLayeredWindow( HWND hwnd, HDC hdcDst, POINT *pptDst, SIZE *psize,
                                 HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend,
                                 DWORD flags)
{
    UPDATELAYEREDWINDOWINFO info;

    if (flags & ULW_EX_NORESIZE)  /* only valid for UpdateLayeredWindowIndirect */
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    info.cbSize   = sizeof(info);
    info.hdcDst   = hdcDst;
    info.pptDst   = pptDst;
    info.psize    = psize;
    info.hdcSrc   = hdcSrc;
    info.pptSrc   = pptSrc;
    info.crKey    = crKey;
    info.pblend   = pblend;
    info.dwFlags  = flags;
    info.prcDirty = NULL;
    return UpdateLayeredWindowIndirect( hwnd, &info );
}


/******************************************************************************
 *                    GetProcessDefaultLayout [USER32.@]
 *
 * Gets the default layout for parentless windows.
 */
BOOL WINAPI GetProcessDefaultLayout( DWORD *layout )
{
    if (!layout)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    if (process_layout == ~0u)
    {
        WCHAR *str, buffer[MAX_PATH];
        DWORD i, version_layout = 0;
        UINT len;
        DWORD user_lang = GetUserDefaultLangID();
        DWORD *languages;
        void *data = NULL;

        GetModuleFileNameW( 0, buffer, MAX_PATH );
        if (!(len = GetFileVersionInfoSizeW( buffer, NULL ))) goto done;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, len ))) goto done;
        if (!GetFileVersionInfoW( buffer, 0, len, data )) goto done;
        if (!VerQueryValueW( data, L"\\VarFileInfo\\Translation", (void **)&languages, &len ) || !len) goto done;

        len /= sizeof(DWORD);
        for (i = 0; i < len; i++) if (LOWORD(languages[i]) == user_lang) break;
        if (i == len)  /* try neutral language */
            for (i = 0; i < len; i++)
                if (LOWORD(languages[i]) == MAKELANGID( PRIMARYLANGID(user_lang), SUBLANG_NEUTRAL )) break;
        if (i == len) i = 0;  /* default to the first one */

        swprintf( buffer, ARRAY_SIZE(buffer), L"\\StringFileInfo\\%04x%04x\\FileDescription",
                  LOWORD(languages[i]), HIWORD(languages[i]) );
        if (!VerQueryValueW( data, buffer, (void **)&str, &len )) goto done;
        TRACE( "found description %s\n", debugstr_w( str ));
        if (str[0] == 0x200e && str[1] == 0x200e) version_layout = LAYOUT_RTL;

    done:
        HeapFree( GetProcessHeap(), 0, data );
        process_layout = version_layout;
    }
    *layout = process_layout;
    return TRUE;
}


/******************************************************************************
 *                    SetProcessDefaultLayout [USER32.@]
 *
 * Sets the default layout for parentless windows.
 */
BOOL WINAPI SetProcessDefaultLayout( DWORD layout )
{
    process_layout = layout;
    return TRUE;
}


/* 64bit versions */

#ifdef GetWindowLongPtrW
#undef GetWindowLongPtrW
#endif

#ifdef GetWindowLongPtrA
#undef GetWindowLongPtrA
#endif

#ifdef SetWindowLongPtrW
#undef SetWindowLongPtrW
#endif

#ifdef SetWindowLongPtrA
#undef SetWindowLongPtrA
#endif

/*****************************************************************************
 *              GetWindowLongPtrW (USER32.@)
 */
LONG_PTR WINAPI GetWindowLongPtrW( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, sizeof(LONG_PTR), TRUE );
}

/*****************************************************************************
 *              GetWindowLongPtrA (USER32.@)
 */
LONG_PTR WINAPI GetWindowLongPtrA( HWND hwnd, INT offset )
{
    return WIN_GetWindowLong( hwnd, offset, sizeof(LONG_PTR), FALSE );
}

/*****************************************************************************
 *              SetWindowLongPtrW (USER32.@)
 */
LONG_PTR WINAPI SetWindowLongPtrW( HWND hwnd, INT offset, LONG_PTR newval )
{
    return WIN_SetWindowLong( hwnd, offset, sizeof(LONG_PTR), newval, TRUE );
}

/*****************************************************************************
 *              SetWindowLongPtrA (USER32.@)
 */
LONG_PTR WINAPI SetWindowLongPtrA( HWND hwnd, INT offset, LONG_PTR newval )
{
    return WIN_SetWindowLong( hwnd, offset, sizeof(LONG_PTR), newval, FALSE );
}

/*****************************************************************************
 *              RegisterTouchWindow (USER32.@)
 */
BOOL WINAPI RegisterTouchWindow(HWND hwnd, ULONG flags)
{
    FIXME("(%p %08x): stub\n", hwnd, flags);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *              UnregisterTouchWindow (USER32.@)
 */
BOOL WINAPI UnregisterTouchWindow(HWND hwnd)
{
    FIXME("(%p): stub\n", hwnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *              CloseTouchInputHandle (USER32.@)
 */
BOOL WINAPI CloseTouchInputHandle(HTOUCHINPUT handle)
{
    FIXME("(%p): stub\n", handle);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *              GetTouchInputInfo (USER32.@)
 */
BOOL WINAPI GetTouchInputInfo(HTOUCHINPUT handle, UINT count, TOUCHINPUT *ptr, int size)
{
    FIXME("(%p %u %p %u): stub\n", handle, count, ptr, size);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *              GetGestureInfo (USER32.@)
 */
BOOL WINAPI GetGestureInfo(HGESTUREINFO handle, PGESTUREINFO ptr)
{
    FIXME("(%p %p): stub\n", handle, ptr);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************************************
 *              GetWindowDisplayAffinity (USER32.@)
 */
BOOL WINAPI GetWindowDisplayAffinity(HWND hwnd, DWORD *affinity)
{
    FIXME("(%p, %p): stub\n", hwnd, affinity);

    if (!hwnd || !affinity)
    {
        SetLastError(hwnd ? ERROR_NOACCESS : ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    *affinity = WDA_NONE;
    return TRUE;
}

/*****************************************************************************
 *              SetWindowDisplayAffinity (USER32.@)
 */
BOOL WINAPI SetWindowDisplayAffinity(HWND hwnd, DWORD affinity)
{
    FIXME("(%p, %u): stub\n", hwnd, affinity);

    if (!hwnd)
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

/**********************************************************************
 *              SetWindowCompositionAttribute (USER32.@)
 */
BOOL WINAPI SetWindowCompositionAttribute(HWND hwnd, void *data)
{
    FIXME("(%p, %p): stub\n", hwnd, data);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              InternalGetWindowIcon   (USER32.@)
 */
HICON WINAPI InternalGetWindowIcon( HWND hwnd, UINT type )
{
    WND *win = WIN_GetPtr( hwnd );
    HICON ret;

    TRACE( "hwnd %p, type %#x\n", hwnd, type );

    if (!win)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        if (IsWindow( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return 0;
    }

    switch (type)
    {
        case ICON_BIG:
            ret = win->hIcon;
            if (!ret) ret = (HICON)GetClassLongPtrW( hwnd, GCLP_HICON );
            break;

        case ICON_SMALL:
        case ICON_SMALL2:
            ret = win->hIconSmall ? win->hIconSmall : win->hIconSmall2;
            if (!ret) ret = (HICON)GetClassLongPtrW( hwnd, GCLP_HICONSM );
            if (!ret) ret = (HICON)GetClassLongPtrW( hwnd, GCLP_HICON );
            break;

        default:
            SetLastError( ERROR_INVALID_PARAMETER );
            WIN_ReleasePtr( win );
            return 0;
    }

    if (!ret) ret = LoadIconW( 0, (const WCHAR *)IDI_APPLICATION );

    WIN_ReleasePtr( win );
    return CopyIcon( ret );
}
