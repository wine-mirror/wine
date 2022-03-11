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


#if 0
#pragma makedep unix
#endif

#include <pthread.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define NB_USER_HANDLES  ((LAST_USER_HANDLE - FIRST_USER_HANDLE + 1) >> 1)
#define USER_HANDLE_TO_INDEX(hwnd) ((LOWORD(hwnd) - FIRST_USER_HANDLE) >> 1)

static void *user_handles[NB_USER_HANDLES];

static struct list window_surfaces = LIST_INIT( window_surfaces );
static pthread_mutex_t surfaces_lock = PTHREAD_MUTEX_INITIALIZER;

/***********************************************************************
 *           alloc_user_handle
 */
HANDLE alloc_user_handle( struct user_object *ptr, unsigned int type )
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
void *get_user_handle_ptr( HANDLE handle, unsigned int type )
{
    struct user_object *ptr;
    WORD index = USER_HANDLE_TO_INDEX( handle );

    if (index >= NB_USER_HANDLES) return NULL;

    user_lock();
    if ((ptr = user_handles[index]))
    {
        if (ptr->type == type &&
            ((UINT)(UINT_PTR)ptr->handle == (UINT)(UINT_PTR)handle ||
             !HIWORD(handle) || HIWORD(handle) == 0xffff))
            return ptr;
        ptr = NULL;
    }
    else ptr = OBJ_OTHER_PROCESS;
    user_unlock();
    return ptr;
}

/***********************************************************************
 *           set_user_handle_ptr
 */
void set_user_handle_ptr( HANDLE handle, struct user_object *ptr )
{
    WORD index = USER_HANDLE_TO_INDEX(handle);
    assert( index < NB_USER_HANDLES );
    InterlockedExchangePointer( &user_handles[index], ptr );
}

/***********************************************************************
 *           release_user_handle_ptr
 */
void release_user_handle_ptr( void *ptr )
{
    assert( ptr && ptr != OBJ_OTHER_PROCESS );
    user_unlock();
}

/***********************************************************************
 *           free_user_handle
 */
void *free_user_handle( HANDLE handle, unsigned int type )
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
        user_unlock();
    }
    return ptr;
}

/***********************************************************************
 *           next_thread_window
 */
WND *next_thread_window_ptr( HWND *hwnd )
{
    struct user_object *ptr;
    WND *win;
    WORD index = *hwnd ? USER_HANDLE_TO_INDEX( *hwnd ) + 1 : 0;

    while (index < NB_USER_HANDLES)
    {
        if (!(ptr = user_handles[index++])) continue;
        if (ptr->type != NTUSER_OBJ_WINDOW) continue;
        win = (WND *)ptr;
        if (win->tid != GetCurrentThreadId()) continue;
        *hwnd = ptr->handle;
        return win;
    }
    return NULL;
}

/*******************************************************************
 *           get_hwnd_message_parent
 *
 * Return the parent for HWND_MESSAGE windows.
 */
HWND get_hwnd_message_parent(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (!thread_info->msg_window) get_desktop_window(); /* trigger creation */
    return thread_info->msg_window;
}

/***********************************************************************
 *           get_full_window_handle
 *
 * Convert a possibly truncated window handle to a full 32-bit handle.
 */
HWND get_full_window_handle( HWND hwnd )
{
    WND *win;

    if (!hwnd || (ULONG_PTR)hwnd >> 16) return hwnd;
    if (LOWORD(hwnd) <= 1 || LOWORD(hwnd) == 0xffff) return hwnd;
    /* do sign extension for -2 and -3 */
    if (LOWORD(hwnd) >= (WORD)-3) return (HWND)(LONG_PTR)(INT16)LOWORD(hwnd);

    if (!(win = get_win_ptr( hwnd ))) return hwnd;

    if (win == WND_DESKTOP)
    {
        if (LOWORD(hwnd) == LOWORD(get_desktop_window())) return get_desktop_window();
        else return get_hwnd_message_parent();
    }

    if (win != WND_OTHER_PROCESS)
    {
        hwnd = win->obj.handle;
        release_win_ptr( win );
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

/*******************************************************************
 *           register_window_surface
 *
 * Register a window surface in the global list, possibly replacing another one.
 */
void register_window_surface( struct window_surface *old, struct window_surface *new )
{
    if (old == new) return;
    pthread_mutex_lock( &surfaces_lock );
    if (old) list_remove( &old->entry );
    if (new) list_add_tail( &window_surfaces, &new->entry );
    pthread_mutex_unlock( &surfaces_lock );
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

    pthread_mutex_lock( &surfaces_lock );
    now = NtGetTickCount();
    if (idle) last_idle = now;
    /* if not idle, we only flush if there's evidence that the app never goes idle */
    else if ((int)(now - last_idle) < 50) goto done;

    LIST_FOR_EACH_ENTRY( surface, &window_surfaces, struct window_surface, entry )
        surface->funcs->flush( surface );
done:
    pthread_mutex_unlock( &surfaces_lock );
}

/*******************************************************************
 *           is_desktop_window
 *
 * Check if window is the desktop or the HWND_MESSAGE top parent.
 */
static BOOL is_desktop_window( HWND hwnd )
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

/***********************************************************************
 *           win_get_ptr
 *
 * Return a pointer to the WND structure if local to the process,
 * or WND_OTHER_PROCESS if handle may be valid in other process.
 * If ret value is a valid pointer, it must be released with WIN_ReleasePtr.
 */
WND *get_win_ptr( HWND hwnd )
{
    WND *win;

    if ((win = get_user_handle_ptr( hwnd, NTUSER_OBJ_WINDOW )) == WND_OTHER_PROCESS)
    {
        if (is_desktop_window( hwnd )) win = WND_DESKTOP;
    }
    return win;
}

/***********************************************************************
 *           is_current_thread_window
 *
 * Check whether a given window belongs to the current process (and return the full handle).
 */
HWND is_current_thread_window( HWND hwnd )
{
    WND *win;
    HWND ret = 0;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
        return 0;
    if (win->tid == GetCurrentThreadId()) ret = win->obj.handle;
    release_win_ptr( win );
    return ret;
}

/* see IsWindow */
BOOL is_window( HWND hwnd )
{
    WND *win;
    BOOL ret;

    if (!(win = get_win_ptr( hwnd ))) return FALSE;
    if (win == WND_DESKTOP) return TRUE;

    if (win != WND_OTHER_PROCESS)
    {
        release_win_ptr( win );
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

/* see GetWindowThreadProcessId */
DWORD get_window_thread( HWND hwnd, DWORD *process )
{
    WND *ptr;
    DWORD tid = 0;

    if (!(ptr = get_win_ptr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE);
        return 0;
    }

    if (ptr != WND_OTHER_PROCESS && ptr != WND_DESKTOP)
    {
        /* got a valid window */
        tid = ptr->tid;
        if (process) *process = GetCurrentProcessId();
        release_win_ptr( ptr );
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

/* see GetParent */
static HWND get_parent( HWND hwnd )
{
    HWND retval = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        LONG style = get_window_long( hwnd, GWL_STYLE );
        if (style & (WS_POPUP | WS_CHILD))
        {
            SERVER_START_REQ( get_window_tree )
            {
                req->handle = wine_server_user_handle( hwnd );
                if (!wine_server_call_err( req ))
                {
                    if (style & WS_POPUP) retval = wine_server_ptr_handle( reply->owner );
                    else if (style & WS_CHILD) retval = wine_server_ptr_handle( reply->parent );
                }
            }
            SERVER_END_REQ;
        }
    }
    else
    {
        if (win->dwStyle & WS_POPUP) retval = win->owner;
        else if (win->dwStyle & WS_CHILD) retval = win->parent;
        release_win_ptr( win );
    }
    return retval;
}

/* see GetWindow */
static HWND get_window_relative( HWND hwnd, UINT rel )
{
    HWND retval = 0;

    if (rel == GW_OWNER)  /* this one may be available locally */
    {
        WND *win = get_win_ptr( hwnd );
        if (!win)
        {
            SetLastError( ERROR_INVALID_HANDLE );
            return 0;
        }
        if (win == WND_DESKTOP) return 0;
        if (win != WND_OTHER_PROCESS)
        {
            retval = win->owner;
            release_win_ptr( win );
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
 *           list_window_parents
 *
 * Build an array of all parents of a given window, starting with
 * the immediate parent. The array must be freed with free().
 */
static HWND *list_window_parents( HWND hwnd )
{
    WND *win;
    HWND current, *list;
    int i, pos = 0, size = 16, count;

    if (!(list = malloc( size * sizeof(HWND) ))) return NULL;

    current = hwnd;
    for (;;)
    {
        if (!(win = get_win_ptr( current ))) goto empty;
        if (win == WND_OTHER_PROCESS) break;  /* need to do it the hard way */
        if (win == WND_DESKTOP)
        {
            if (!pos) goto empty;
            list[pos] = 0;
            return list;
        }
        list[pos] = current = win->parent;
        release_win_ptr( win );
        if (!current) return list;
        if (++pos == size - 1)
        {
            /* need to grow the list */
            HWND *new_list = realloc( list, (size + 16) * sizeof(HWND) );
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
        free( list );
        size = count + 1;
        if (!(list = malloc( size * sizeof(HWND) ))) return NULL;
    }

empty:
    free( list );
    return NULL;
}

/*******************************************************************
 *           list_window_children
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
HWND *list_window_children( HDESK desktop, HWND hwnd, UNICODE_STRING *class, DWORD tid )
{
    HWND *list;
    int i, size = 128;
    ATOM atom = class ? get_int_atom_value( class ) : 0;

    /* empty class is not the same as NULL class */
    if (!atom && class && !class->Length) return NULL;

    for (;;)
    {
        int count = 0;

        if (!(list = malloc( size * sizeof(HWND) ))) break;

        SERVER_START_REQ( get_window_children )
        {
            req->desktop = wine_server_obj_handle( desktop );
            req->parent = wine_server_user_handle( hwnd );
            req->tid = tid;
            req->atom = atom;
            if (!atom && class) wine_server_add_data( req, class->Buffer, class->Length );
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
        free( list );
        if (!count) break;
        size = count + 1;  /* restart with a large enough buffer */
    }
    return NULL;
}

/*****************************************************************
 *           NtUserGetAncestor (win32u.@)
 */
HWND WINAPI NtUserGetAncestor( HWND hwnd, UINT type )
{
    HWND *list, ret = 0;
    WND *win;

    switch(type)
    {
    case GA_PARENT:
        if (!(win = get_win_ptr( hwnd )))
        {
            SetLastError( ERROR_INVALID_WINDOW_HANDLE );
            return 0;
        }
        if (win == WND_DESKTOP) return 0;
        if (win != WND_OTHER_PROCESS)
        {
            ret = win->parent;
            release_win_ptr( win );
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

        if (!list[0] || !list[1]) ret = get_full_window_handle( hwnd );  /* top-level window */
        else
        {
            int count = 2;
            while (list[count]) count++;
            ret = list[count - 2];  /* get the one before the desktop */
        }
        free( list );
        break;

    case GA_ROOTOWNER:
        if (is_desktop_window( hwnd )) return 0;
        ret = get_full_window_handle( hwnd );
        for (;;)
        {
            HWND parent = get_parent( ret );
            if (!parent) break;
            ret = parent;
        }
        break;
    }
    return ret;
}

/* see IsChild */
BOOL is_child( HWND parent, HWND child )
{
    HWND *list;
    int i;
    BOOL ret = FALSE;

    if (!(get_window_long( child, GWL_STYLE ) & WS_CHILD)) return FALSE;
    if (!(list = list_window_parents( child ))) return FALSE;
    parent = get_full_window_handle( parent );
    for (i = 0; list[i]; i++)
    {
        if (list[i] == parent)
        {
            ret = list[i] && list[i+1];
            break;
        }
        if (!(get_window_long( list[i], GWL_STYLE ) & WS_CHILD)) break;
    }
    free( list );
    return ret;
}

/* see IsWindowVisible */
static BOOL is_window_visible( HWND hwnd )
{
    HWND *list;
    BOOL retval = TRUE;
    int i;

    if (!(get_window_long( hwnd, GWL_STYLE ) & WS_VISIBLE)) return FALSE;
    if (!(list = list_window_parents( hwnd ))) return TRUE;
    if (list[0])
    {
        for (i = 0; list[i+1]; i++)
            if (!(get_window_long( list[i], GWL_STYLE ) & WS_VISIBLE)) break;
        retval = !list[i+1] && (list[i] == get_desktop_window());  /* top message window isn't visible */
    }
    free( list );
    return retval;
}

/***********************************************************************
 *           is_window_drawable
 *
 * hwnd is drawable when it is visible, all parents are not
 * minimized, and it is itself not minimized unless we are
 * trying to draw its default class icon.
 */
static BOOL is_window_drawable( HWND hwnd, BOOL icon )
{
    HWND *list;
    BOOL retval = TRUE;
    int i;
    LONG style = get_window_long( hwnd, GWL_STYLE );

    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & WS_MINIMIZE) && icon && get_class_long_ptr( hwnd, GCLP_HICON, FALSE ))  return FALSE;

    if (!(list = list_window_parents( hwnd ))) return TRUE;
    if (list[0])
    {
        for (i = 0; list[i+1]; i++)
            if ((get_window_long( list[i], GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != WS_VISIBLE)
                break;
        retval = !list[i+1] && (list[i] == get_desktop_window());  /* top message window isn't visible */
    }
    free( list );
    return retval;
}

/* see IsWindowUnicode */
static BOOL is_window_unicode( HWND hwnd )
{
    WND *win;
    BOOL ret = FALSE;

    if (!(win = get_win_ptr(hwnd))) return FALSE;

    if (win == WND_DESKTOP) return TRUE;

    if (win != WND_OTHER_PROCESS)
    {
        ret = (win->flags & WIN_ISUNICODE) != 0;
        release_win_ptr( win );
    }
    else
    {
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            if (!wine_server_call_err( req )) ret = reply->is_unicode;
        }
        SERVER_END_REQ;
    }
    return ret;
}

/* see GetWindowDpiAwarenessContext */
static DPI_AWARENESS_CONTEXT get_window_dpi_awareness_context( HWND hwnd )
{
    DPI_AWARENESS_CONTEXT ret = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP) return DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE;
    if (win != WND_OTHER_PROCESS)
    {
        ret = ULongToHandle( win->dpi_awareness | 0x10 );
        release_win_ptr( win );
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

/* see GetDpiForWindow */
static UINT get_dpi_for_window( HWND hwnd )
{
    WND *win;
    UINT ret = 0;

    if (!(win = get_win_ptr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP)
    {
        POINT pt = { 0, 0 };
        return get_monitor_dpi( monitor_from_point( pt, MONITOR_DEFAULTTOPRIMARY, 0 ));
    }
    if (win != WND_OTHER_PROCESS)
    {
        ret = win->dpi;
        if (!ret) ret = get_win_monitor_dpi( hwnd );
        release_win_ptr( win );
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

static LONG_PTR get_win_data( const void *ptr, UINT size )
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

BOOL is_iconic( HWND hwnd )
{
    return (get_window_long( hwnd, GWL_STYLE ) & WS_MINIMIZE) != 0;
}

static LONG_PTR get_window_long_size( HWND hwnd, INT offset, UINT size, BOOL ansi )
{
    LONG_PTR retval = 0;
    WND *win;

    if (offset == GWLP_HWNDPARENT)
    {
        HWND parent = NtUserGetAncestor( hwnd, GA_PARENT );
        if (parent == get_desktop_window())
            parent = get_window_relative( hwnd, GW_OWNER );
        return (ULONG_PTR)parent;
    }

    if (!(win = get_win_ptr( hwnd )))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (win == WND_DESKTOP)
    {
        switch (offset)
        {
        case GWL_STYLE:
            retval = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN; /* message parent is not visible */
            if (get_full_window_handle( hwnd ) == get_desktop_window())
                retval |= WS_VISIBLE;
            return retval;
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

    if (win == WND_OTHER_PROCESS)
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
                case GWL_STYLE:      retval = reply->old_style; break;
                case GWL_EXSTYLE:    retval = reply->old_ex_style; break;
                case GWLP_ID:        retval = reply->old_id; break;
                case GWLP_HINSTANCE: retval = (ULONG_PTR)wine_server_get_ptr( reply->old_instance ); break;
                case GWLP_USERDATA:  retval = reply->old_user_data; break;
                default:
                    if (offset >= 0) retval = get_win_data( &reply->old_extra_value, size );
                    else SetLastError( ERROR_INVALID_INDEX );
                    break;
                }
            }
        }
        SERVER_END_REQ;
        return retval;
    }

    /* now we have a valid win */

    if (offset >= 0)
    {
        if (offset > (int)(win->cbWndExtra - size))
        {
            WARN("Invalid offset %d\n", offset );
            release_win_ptr( win );
            SetLastError( ERROR_INVALID_INDEX );
            return 0;
        }
        retval = get_win_data( (char *)win->wExtra + offset, size );

        /* Special case for dialog window procedure */
        if ((offset == DWLP_DLGPROC) && (size == sizeof(LONG_PTR)) && win->dlgInfo)
            retval = (LONG_PTR)get_winproc( (WNDPROC)retval, ansi );
        release_win_ptr( win );
        return retval;
    }

    switch(offset)
    {
    case GWLP_USERDATA:  retval = win->userdata; break;
    case GWL_STYLE:      retval = win->dwStyle; break;
    case GWL_EXSTYLE:    retval = win->dwExStyle; break;
    case GWLP_ID:        retval = win->wIDmenu; break;
    case GWLP_HINSTANCE: retval = (ULONG_PTR)win->hInstance; break;
    case GWLP_WNDPROC:
        /* This looks like a hack only for the edit control (see tests). This makes these controls
         * more tolerant to A/W mismatches. The lack of W->A->W conversion for such a mismatch suggests
         * that the hack is in GetWindowLongPtr[AW], not in winprocs.
         */
        if (win->winproc == BUILTIN_WINPROC(WINPROC_EDIT) && (!!ansi != !(win->flags & WIN_ISUNICODE)))
            retval = (ULONG_PTR)win->winproc;
        else
            retval = (ULONG_PTR)get_winproc( win->winproc, ansi );
        break;
    default:
        WARN("Unknown offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        break;
    }
    release_win_ptr( win );
    return retval;
}

/* see GetWindowLongW */
DWORD get_window_long( HWND hwnd, INT offset )
{
    return get_window_long_size( hwnd, offset, sizeof(LONG), FALSE );
}

/* see GetWindowLongPtr */
static ULONG_PTR get_window_long_ptr( HWND hwnd, INT offset, BOOL ansi )
{
    return get_window_long_size( hwnd, offset, sizeof(LONG_PTR), ansi );
}

/* see GetWindowWord */
static WORD get_window_word( HWND hwnd, INT offset )
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
    return get_window_long_size( hwnd, offset, sizeof(WORD), TRUE );
}

/***********************************************************************
 *           NtUserGetProp   (win32u.@)
 *
 * NOTE Native allows only ATOMs as the second argument. We allow strings
 *      to save extra server call in GetPropW.
 */
HANDLE WINAPI NtUserGetProp( HWND hwnd, const WCHAR *str )
{
    ULONG_PTR ret = 0;

    SERVER_START_REQ( get_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, lstrlenW(str) * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) ret = reply->data;
    }
    SERVER_END_REQ;
    return (HANDLE)ret;
}

/*****************************************************************************
 *           NtUserSetProp    (win32u.@)
 *
 * NOTE Native allows only ATOMs as the second argument. We allow strings
 *      to save extra server call in SetPropW.
 */
BOOL WINAPI NtUserSetProp( HWND hwnd, const WCHAR *str, HANDLE handle )
{
    BOOL ret;

    SERVER_START_REQ( set_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        req->data   = (ULONG_PTR)handle;
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, lstrlenW(str) * sizeof(WCHAR) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           NtUserRemoveProp   (win32u.@)
 *
 * NOTE Native allows only ATOMs as the second argument. We allow strings
 *      to save extra server call in RemovePropW.
 */
HANDLE WINAPI NtUserRemoveProp( HWND hwnd, const WCHAR *str )
{
    ULONG_PTR ret = 0;

    SERVER_START_REQ( remove_window_property )
    {
        req->window = wine_server_user_handle( hwnd );
        if (IS_INTRESOURCE(str)) req->atom = LOWORD(str);
        else wine_server_add_data( req, str, lstrlenW(str) * sizeof(WCHAR) );
        if (!wine_server_call_err( req )) ret = reply->data;
    }
    SERVER_END_REQ;

    return (HANDLE)ret;
}

static void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/***********************************************************************
 *           get_window_rects
 *
 * Get the window and client rectangles.
 */
BOOL get_window_rects( HWND hwnd, enum coords_relative relative, RECT *window_rect,
                       RECT *client_rect, UINT dpi )
{
    WND *win = get_win_ptr( hwnd );
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
            rect = map_dpi_rect( rect, get_dpi_for_window( hwnd ), dpi );
        }
        else
        {
            rect = get_primary_monitor_rect( dpi );
        }
        if (window_rect) *window_rect = rect;
        if (client_rect) *client_rect = rect;
        return TRUE;
    }
    if (win != WND_OTHER_PROCESS)
    {
        UINT window_dpi = get_dpi_for_window( hwnd );
        RECT window = win->window_rect;
        RECT client = win->client_rect;

        switch (relative)
        {
        case COORDS_CLIENT:
            OffsetRect( &window, -win->client_rect.left, -win->client_rect.top );
            OffsetRect( &client, -win->client_rect.left, -win->client_rect.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
                mirror_rect( &win->client_rect, &window );
            break;
        case COORDS_WINDOW:
            OffsetRect( &window, -win->window_rect.left, -win->window_rect.top );
            OffsetRect( &client, -win->window_rect.left, -win->window_rect.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
                mirror_rect( &win->window_rect, &client );
            break;
        case COORDS_PARENT:
            if (win->parent)
            {
                WND *parent = get_win_ptr( win->parent );
                if (parent == WND_DESKTOP) break;
                if (!parent || parent == WND_OTHER_PROCESS)
                {
                    release_win_ptr( win );
                    goto other_process;
                }
                if (parent->flags & WIN_CHILDREN_MOVED)
                {
                    release_win_ptr( parent );
                    release_win_ptr( win );
                    goto other_process;
                }
                if (parent->dwExStyle & WS_EX_LAYOUTRTL)
                {
                    mirror_rect( &parent->client_rect, &window );
                    mirror_rect( &parent->client_rect, &client );
                }
                release_win_ptr( parent );
            }
            break;
        case COORDS_SCREEN:
            while (win->parent)
            {
                WND *parent = get_win_ptr( win->parent );
                if (parent == WND_DESKTOP) break;
                if (!parent || parent == WND_OTHER_PROCESS)
                {
                    release_win_ptr( win );
                    goto other_process;
                }
                release_win_ptr( win );
                if (parent->flags & WIN_CHILDREN_MOVED)
                {
                    release_win_ptr( parent );
                    goto other_process;
                }
                win = parent;
                if (win->parent)
                {
                    offset_rect( &window, win->client_rect.left, win->client_rect.top );
                    offset_rect( &client, win->client_rect.left, win->client_rect.top );
                }
            }
            break;
        }
        if (window_rect) *window_rect = map_dpi_rect( window, window_dpi, dpi );
        if (client_rect) *client_rect = map_dpi_rect( client, window_dpi, dpi );
        release_win_ptr( win );
        return TRUE;
    }

other_process:
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->relative = relative;
        req->dpi = dpi;
        if ((ret = !wine_server_call_err( req )))
        {
            if (window_rect)
            {
                window_rect->left   = reply->window.left;
                window_rect->top    = reply->window.top;
                window_rect->right  = reply->window.right;
                window_rect->bottom = reply->window.bottom;
            }
            if (client_rect)
            {
                client_rect->left   = reply->client.left;
                client_rect->top    = reply->client.top;
                client_rect->right  = reply->client.right;
                client_rect->bottom = reply->client.bottom;
            }
        }
    }
    SERVER_END_REQ;
    return ret;
}

/* see GetWindowRect */
BOOL get_window_rect( HWND hwnd, RECT *rect, UINT dpi )
{
    return get_window_rects( hwnd, COORDS_SCREEN, rect, NULL, dpi );
}

/* see GetClientRect */
static BOOL get_client_rect( HWND hwnd, RECT *rect )
{
    return get_window_rects( hwnd, COORDS_CLIENT, NULL, rect, get_thread_dpi() );
}

/* see GetWindowInfo */
static BOOL get_window_info( HWND hwnd, WINDOWINFO *info )
{

    if (!info || !get_window_rects( hwnd, COORDS_SCREEN, &info->rcWindow,
                                    &info->rcClient, get_thread_dpi() ))
        return FALSE;

    info->dwStyle         = get_window_long( hwnd, GWL_STYLE );
    info->dwExStyle       = get_window_long( hwnd, GWL_EXSTYLE );
    info->dwWindowStatus  = get_active_window() == hwnd ? WS_ACTIVECAPTION : 0;
    info->cxWindowBorders = info->rcClient.left - info->rcWindow.left;
    info->cyWindowBorders = info->rcWindow.bottom - info->rcClient.bottom;
    info->atomWindowType  = get_class_long( hwnd, GCW_ATOM, FALSE );
    info->wCreatorVersion = 0x0400;
    return TRUE;
}

/*******************************************************************
 *           NtUserGetWindowRgnEx (win32u.@)
 */
int WINAPI NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk )
{
    NTSTATUS status;
    HRGN win_rgn = 0;
    RGNDATA *data;
    size_t size = 256;
    int ret = ERROR;

    do
    {
        if (!(data = malloc( sizeof(*data) + size - 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return ERROR;
        }
        SERVER_START_REQ( get_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                if (reply_size)
                {
                    data->rdh.dwSize   = sizeof(data->rdh);
                    data->rdh.iType    = RDH_RECTANGLES;
                    data->rdh.nCount   = reply_size / sizeof(RECT);
                    data->rdh.nRgnSize = reply_size;
                    win_rgn = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                }
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        free( data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (set_ntstatus( status ) && win_rgn)
    {
        ret = NtGdiCombineRgn( hrgn, win_rgn, 0, RGN_COPY );
        NtGdiDeleteObjectApp( win_rgn );
    }
    return ret;
}

/***********************************************************************
 *           NtUserSetWindowRgn (win32u.@)
 */
int WINAPI NtUserSetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    static const RECT empty_rect;
    BOOL ret;

    if (hrgn)
    {
        RGNDATA *data;
        DWORD size;

        if (!(size = NtGdiGetRegionData( hrgn, 0, NULL ))) return FALSE;
        if (!(data = malloc( size ))) return FALSE;
        if (!NtGdiGetRegionData( hrgn, size, data ))
        {
            free( data );
            return FALSE;
        }
        SERVER_START_REQ( set_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            req->redraw = redraw != 0;
            if (data->rdh.nCount)
                wine_server_add_data( req, data->Buffer, data->rdh.nCount * sizeof(RECT) );
            else
                wine_server_add_data( req, &empty_rect, sizeof(empty_rect) );
            ret = !wine_server_call_err( req );
        }
        SERVER_END_REQ;
        free( data );
    }
    else  /* clear existing region */
    {
        SERVER_START_REQ( set_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            req->redraw = redraw != 0;
            ret = !wine_server_call_err( req );
        }
        SERVER_END_REQ;
    }

    if (ret)
    {
        UINT swp_flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED |
            SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE;
        if (!redraw) swp_flags |= SWP_NOREDRAW;
        user_driver->pSetWindowRgn( hwnd, hrgn, redraw );
        NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, swp_flags );
        if (hrgn) NtGdiDeleteObjectApp( hrgn );
    }
    return ret;
}

/*******************************************************************
 *           NtUserSetWindowPos (win32u.@)
 */
BOOL WINAPI NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags )
{
    /* FIXME: move implementation from user32 */
    return user_callbacks && user_callbacks->pSetWindowPos( hwnd, after, x, y, cx, cy, flags );
}

/***********************************************************************
 *           NtUserMoveWindow (win32u.@)
 */
BOOL WINAPI NtUserMoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy, BOOL repaint )
{
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
    TRACE( "%p %d,%d %dx%d %d\n", hwnd, x, y, cx, cy, repaint );
    return NtUserSetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}

/*****************************************************************************
 *           NtUserGetLayeredWindowAttributes (win32u.@)
 */
BOOL WINAPI NtUserGetLayeredWindowAttributes( HWND hwnd, COLORREF *key, BYTE *alpha, DWORD *flags )
{
    BOOL ret;

    SERVER_START_REQ( get_window_layered_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
        {
            if (key) *key = reply->color_key;
            if (alpha) *alpha = reply->alpha;
            if (flags) *flags = reply->flags;
        }
    }
    SERVER_END_REQ;

    return ret;
}

/*****************************************************************************
 *           NtUserSetLayeredWindowAttributes (win32u.@)
 */
BOOL WINAPI NtUserSetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    BOOL ret;

    TRACE( "(%p,%08x,%d,%x)\n", hwnd, key, alpha, flags );

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
        user_driver->pSetLayeredWindowAttributes( hwnd, key, alpha, flags );
        update_window_state( hwnd );
    }

    return ret;
}

/*****************************************************************************
 *           UpdateLayeredWindow (win32u.@)
 */
BOOL WINAPI NtUserUpdateLayeredWindow( HWND hwnd, HDC hdc_dst, const POINT *pts_dst, const SIZE *size,
                                       HDC hdc_src, const POINT *pts_src, COLORREF key,
                                       const BLENDFUNCTION *blend, DWORD flags, const RECT *dirty )
{
    DWORD swp_flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
    RECT window_rect, client_rect;
    UPDATELAYEREDWINDOWINFO info;
    SIZE offset;

    if (flags & ~(ULW_COLORKEY | ULW_ALPHA | ULW_OPAQUE | ULW_EX_NORESIZE) ||
        !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED) ||
        NtUserGetLayeredWindowAttributes( hwnd, NULL, NULL, NULL ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    get_window_rects( hwnd, COORDS_PARENT, &window_rect, &client_rect, get_thread_dpi() );

    if (pts_dst)
    {
        offset.cx = pts_dst->x - window_rect.left;
        offset.cy = pts_dst->y - window_rect.top;
        OffsetRect( &client_rect, offset.cx, offset.cy );
        OffsetRect( &window_rect, offset.cx, offset.cy );
        swp_flags &= ~SWP_NOMOVE;
    }
    if (size)
    {
        offset.cx = size->cx - (window_rect.right - window_rect.left);
        offset.cy = size->cy - (window_rect.bottom - window_rect.top);
        if (size->cx <= 0 || size->cy <= 0)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if ((flags & ULW_EX_NORESIZE) && (offset.cx || offset.cy))
        {
            SetLastError( ERROR_INCORRECT_SIZE );
            return FALSE;
        }
        client_rect.right  += offset.cx;
        client_rect.bottom += offset.cy;
        window_rect.right  += offset.cx;
        window_rect.bottom += offset.cy;
        swp_flags &= ~SWP_NOSIZE;
    }

    TRACE( "window %p win %s client %s\n", hwnd,
           wine_dbgstr_rect(&window_rect), wine_dbgstr_rect(&client_rect) );

    if (user_callbacks)
        user_callbacks->set_window_pos( hwnd, 0, swp_flags, &window_rect, &client_rect, NULL );

    info.cbSize   = sizeof(info);
    info.hdcDst   = hdc_dst;
    info.pptDst   = pts_dst;
    info.psize    = size;
    info.hdcSrc   = hdc_src;
    info.pptSrc   = pts_src;
    info.crKey    = key;
    info.pblend   = blend;
    info.dwFlags  = flags;
    info.prcDirty = dirty;
    return user_driver->pUpdateLayeredWindow( hwnd, &info, &window_rect );
}

/***********************************************************************
 *           list_children_from_point
 *
 * Get the list of children that can contain point from the server.
 * Point is in screen coordinates.
 * Returned list must be freed by caller.
 */
static HWND *list_children_from_point( HWND hwnd, POINT pt )
{
    int i, size = 128;
    HWND *list;

    for (;;)
    {
        int count = 0;

        if (!(list = malloc( size * sizeof(HWND) ))) break;

        SERVER_START_REQ( get_window_children_from_point )
        {
            req->parent = wine_server_user_handle( hwnd );
            req->x = pt.x;
            req->y = pt.y;
            req->dpi = get_thread_dpi();
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
        free( list );
        if (!count) break;
        size = count + 1;  /* restart with a large enough buffer */
    }
    return NULL;
}

/***********************************************************************
 *           window_from_point
 *
 * Find the window and hittest for a given point.
 */
static HWND window_from_point( HWND hwnd, POINT pt, INT *hittest )
{
    int i, res;
    HWND ret, *list;
    POINT win_pt;

    if (!hwnd) hwnd = get_desktop_window();

    *hittest = HTNOWHERE;

    if (!(list = list_children_from_point( hwnd, pt ))) return 0;

    /* now determine the hittest */

    for (i = 0; list[i]; i++)
    {
        LONG style = get_window_long( list[i], GWL_STYLE );

        /* If window is minimized or disabled, return at once */
        if (style & WS_DISABLED)
        {
            *hittest = HTERROR;
            break;
        }
        /* Send WM_NCCHITTEST (if same thread) */
        if (!is_current_thread_window( list[i] ))
        {
            *hittest = HTCLIENT;
            break;
        }
        win_pt = map_dpi_point( pt, get_thread_dpi(), get_dpi_for_window( list[i] ));
        res = send_message( list[i], WM_NCHITTEST, 0, MAKELPARAM( win_pt.x, win_pt.y ));
        if (res != HTTRANSPARENT)
        {
            *hittest = res;  /* Found the window */
            break;
        }
        /* continue search with next window in z-order */
    }
    ret = list[i];
    free( list );
    TRACE( "scope %p (%d,%d) returning %p\n", hwnd, pt.x, pt.y, ret );
    return ret;
}

/*******************************************************************
 *           NtUserWindowFromPoint (win32u.@)
 */
HWND WINAPI NtUserWindowFromPoint( LONG x, LONG y )
{
    POINT pt = { .x = x, .y = y };
    INT hittest;
    return window_from_point( 0, pt, &hittest );
}

/*******************************************************************
 *           get_work_rect
 *
 * Get the work area that a maximized window can cover, depending on style.
 */
static BOOL get_work_rect( HWND hwnd, RECT *rect )
{
    HMONITOR monitor = monitor_from_window( hwnd, MONITOR_DEFAULTTOPRIMARY, get_thread_dpi() );
    MONITORINFO mon_info;
    DWORD style;

    if (!monitor) return FALSE;

    mon_info.cbSize = sizeof(mon_info);
    get_monitor_info( monitor, &mon_info );
    *rect = mon_info.rcMonitor;

    style = get_window_long( hwnd, GWL_STYLE );
    if (style & WS_MAXIMIZEBOX)
    {
        if ((style & WS_CAPTION) == WS_CAPTION || !(style & (WS_CHILD | WS_POPUP)))
            *rect = mon_info.rcWork;
    }
    return TRUE;
}

static RECT get_maximized_work_rect( HWND hwnd )
{
    RECT work_rect = { 0 };

    if ((get_window_long( hwnd, GWL_STYLE ) & (WS_MINIMIZE | WS_MAXIMIZE)) == WS_MAXIMIZE)
    {
        if (!get_work_rect( hwnd, &work_rect ))
            work_rect = get_primary_monitor_rect( get_thread_dpi() );
    }
    return work_rect;
}

/*******************************************************************
 *           update_maximized_pos
 *
 * For top level windows covering the work area, we might have to
 * "forget" the maximized position. Windows presumably does this
 * to avoid situations where the border style changes, which would
 * lead the window to be outside the screen, or the window gets
 * reloaded on a different screen, and the "saved" position no
 * longer applies to it (despite being maximized).
 *
 * Some applications (e.g. Imperiums: Greek Wars) depend on this.
 */
static void update_maximized_pos( WND *wnd, RECT *work_rect )
{
    if (wnd->parent && wnd->parent != get_desktop_window())
        return;

    if (wnd->dwStyle & WS_MAXIMIZE)
    {
        if (wnd->window_rect.left  <= work_rect->left  && wnd->window_rect.top    <= work_rect->top &&
            wnd->window_rect.right >= work_rect->right && wnd->window_rect.bottom >= work_rect->bottom)
            wnd->max_pos.x = wnd->max_pos.y = -1;
    }
    else
        wnd->max_pos.x = wnd->max_pos.y = -1;
}

static BOOL empty_point( POINT pt )
{
    return pt.x == -1 && pt.y == -1;
}

/* see GetWindowPlacement */
BOOL get_window_placement( HWND hwnd, WINDOWPLACEMENT *placement )
{
    RECT work_rect = get_maximized_work_rect( hwnd );
    WND *win = get_win_ptr( hwnd );
    UINT win_dpi;

    if (!win) return FALSE;

    if (win == WND_DESKTOP)
    {
        placement->length  = sizeof(*placement);
        placement->showCmd = SW_SHOWNORMAL;
        placement->flags = 0;
        placement->ptMinPosition.x = -1;
        placement->ptMinPosition.y = -1;
        placement->ptMaxPosition.x = -1;
        placement->ptMaxPosition.y = -1;
        get_window_rect( hwnd, &placement->rcNormalPosition, get_thread_dpi() );
        return TRUE;
    }
    if (win == WND_OTHER_PROCESS)
    {
        RECT normal_position;
        DWORD style;

        if (!get_window_rect( hwnd, &normal_position, get_thread_dpi() ))
            return FALSE;

        FIXME("not fully supported on other process window %p.\n", hwnd);

        placement->length  = sizeof(*placement);
        style = get_window_long( hwnd, GWL_STYLE );
        if (style & WS_MINIMIZE)
            placement->showCmd = SW_SHOWMINIMIZED;
        else
            placement->showCmd = (style & WS_MAXIMIZE) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
        /* provide some dummy information */
        placement->flags = 0;
        placement->ptMinPosition.x = -1;
        placement->ptMinPosition.y = -1;
        placement->ptMaxPosition.x = -1;
        placement->ptMaxPosition.y = -1;
        placement->rcNormalPosition = normal_position;
        return TRUE;
    }

    /* update the placement according to the current style */
    if (win->dwStyle & WS_MINIMIZE)
    {
        win->min_pos.x = win->window_rect.left;
        win->min_pos.y = win->window_rect.top;
    }
    else if (win->dwStyle & WS_MAXIMIZE)
    {
        win->max_pos.x = win->window_rect.left;
        win->max_pos.y = win->window_rect.top;
    }
    else
    {
        win->normal_rect = win->window_rect;
    }
    update_maximized_pos( win, &work_rect );

    placement->length  = sizeof(*placement);
    if (win->dwStyle & WS_MINIMIZE)
        placement->showCmd = SW_SHOWMINIMIZED;
    else
        placement->showCmd = ( win->dwStyle & WS_MAXIMIZE ) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL ;
    if (win->flags & WIN_RESTORE_MAX)
        placement->flags = WPF_RESTORETOMAXIMIZED;
    else
        placement->flags = 0;
    win_dpi = get_dpi_for_window( hwnd );
    placement->ptMinPosition = empty_point(win->min_pos) ? win->min_pos
        : map_dpi_point( win->min_pos, win_dpi, get_thread_dpi() );
    placement->ptMaxPosition = empty_point(win->max_pos) ? win->max_pos
        : map_dpi_point( win->max_pos, win_dpi, get_thread_dpi() );
    placement->rcNormalPosition = map_dpi_rect( win->normal_rect, win_dpi, get_thread_dpi() );
    release_win_ptr( win );

    TRACE( "%p: returning min %d,%d max %d,%d normal %s\n",
           hwnd, placement->ptMinPosition.x, placement->ptMinPosition.y,
           placement->ptMaxPosition.x, placement->ptMaxPosition.y,
           wine_dbgstr_rect(&placement->rcNormalPosition) );
    return TRUE;
}

/*****************************************************************************
 *           NtUserBuildHwndList (win32u.@)
 */
NTSTATUS WINAPI NtUserBuildHwndList( HDESK desktop, ULONG unk2, ULONG unk3, ULONG unk4,
                                     ULONG thread_id, ULONG count, HWND *buffer, ULONG *size )
{
    user_handle_t *list = (user_handle_t *)buffer;
    int i;
    NTSTATUS status;

    SERVER_START_REQ( get_window_children )
    {
        req->desktop = wine_server_obj_handle( desktop );
        req->tid = thread_id;
        if (count) wine_server_set_reply( req, list, (count - 1) * sizeof(user_handle_t) );
        status = wine_server_call( req );
        if (status && status != STATUS_BUFFER_TOO_SMALL) return status;
        *size = reply->count + 1;
    }
    SERVER_END_REQ;
    if (*size > count) return STATUS_BUFFER_TOO_SMALL;

    /* start from the end since HWND is potentially larger than user_handle_t */
    for (i = *size - 2; i >= 0; i--)
        buffer[i] = wine_server_ptr_handle( list[i] );
    buffer[*size - 1] = HWND_BOTTOM;
    return STATUS_SUCCESS;
}

/* Retrieve the window text from the server. */
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
 *           NtUserInternalGetWindowText (win32u.@)
 */
INT WINAPI NtUserInternalGetWindowText( HWND hwnd, WCHAR *text, INT count )
{
    WND *win;

    if (count <= 0) return 0;
    if (!(win = get_win_ptr( hwnd ))) return 0;
    if (win == WND_DESKTOP) text[0] = 0;
    else if (win != WND_OTHER_PROCESS)
    {
        if (win->text) lstrcpynW( text, win->text, count );
        else text[0] = 0;
        release_win_ptr( win );
    }
    else
    {
        get_server_window_text( hwnd, text, count );
    }
    return lstrlenW(text);
}

/*******************************************************************
 *           update_window_state
 *
 * Trigger an update of the window's driver state and surface.
 */
void update_window_state( HWND hwnd )
{
    DPI_AWARENESS_CONTEXT context;
    RECT window_rect, client_rect, valid_rects[2];

    if (!is_current_thread_window( hwnd ))
    {
        post_message( hwnd, WM_WINE_UPDATEWINDOWSTATE, 0, 0 );
        return;
    }

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));
    get_window_rects( hwnd, COORDS_PARENT, &window_rect, &client_rect, get_thread_dpi() );
    valid_rects[0] = valid_rects[1] = client_rect;
    if (user_callbacks)
        user_callbacks->set_window_pos( hwnd, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE |
                                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW,
                                        &window_rect, &client_rect, valid_rects );
    set_thread_dpi_awareness_context( context );
}

/*******************************************************************
 *           NtUserFlashWindowEx (win32u.@)
 */
BOOL WINAPI NtUserFlashWindowEx( FLASHWINFO *info )
{
    WND *win;

    TRACE( "%p\n", info );

    if (!info)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!info->hwnd || info->cbSize != sizeof(FLASHWINFO) || !is_window( info->hwnd ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    FIXME( "%p - semi-stub\n", info );

    if (is_iconic( info->hwnd ))
    {
        user_callbacks->pRedrawWindow( info->hwnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME );

        win = get_win_ptr( info->hwnd );
        if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
        if (info->dwFlags && !(win->flags & WIN_NCACTIVATED))
        {
            win->flags |= WIN_NCACTIVATED;
        }
        else
        {
            win->flags &= ~WIN_NCACTIVATED;
        }
        release_win_ptr( win );
        user_driver->pFlashWindowEx( info );
        return TRUE;
    }
    else
    {
        WPARAM wparam;
        HWND hwnd = info->hwnd;

        win = get_win_ptr( hwnd );
        if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
        hwnd = win->obj.handle;  /* make it a full handle */

        if (info->dwFlags) wparam = !(win->flags & WIN_NCACTIVATED);
        else wparam = (hwnd == NtUserGetForegroundWindow());

        release_win_ptr( win );
        send_message( hwnd, WM_NCACTIVATE, wparam, 0 );
        user_driver->pFlashWindowEx( info );
        return wparam;
    }
}

/*****************************************************************************
 *           NtUserCallHwnd (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallHwnd( HWND hwnd, DWORD code )
{
    switch (code)
    {
    case NtUserGetDpiForWindow:
        return get_dpi_for_window( hwnd );
    case NtUserGetParent:
        return HandleToUlong( get_parent( hwnd ));
    case NtUserGetWindowDpiAwarenessContext:
        return (ULONG_PTR)get_window_dpi_awareness_context( hwnd );
    case NtUserGetWindowTextLength:
        return get_server_window_text( hwnd, NULL, 0 );
    case NtUserIsWindow:
        return is_window( hwnd );
    case NtUserIsWindowUnicode:
        return is_window_unicode( hwnd );
    case NtUserIsWindowVisible:
        return is_window_visible( hwnd );
    /* temporary exports */
    case NtUserCreateDesktopWindow:
        return user_driver->pCreateDesktopWindow( hwnd );
    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}

/*****************************************************************************
 *           NtUserCallHwndParam (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallHwndParam( HWND hwnd, DWORD_PTR param, DWORD code )
{
    switch (code)
    {
    case NtUserGetClassLongA:
        return get_class_long( hwnd, param, TRUE );
    case NtUserGetClassLongW:
        return get_class_long( hwnd, param, FALSE );
    case NtUserGetClassLongPtrA:
        return get_class_long_ptr( hwnd, param, TRUE );
    case NtUserGetClassLongPtrW:
        return get_class_long_ptr( hwnd, param, FALSE );
    case NtUserGetClassWord:
        return get_class_word( hwnd, param );
    case NtUserGetClientRect:
        return get_client_rect( hwnd, (RECT *)param );
    case NtUserGetWindowInfo:
        return get_window_info( hwnd, (WINDOWINFO *)param );
    case NtUserGetWindowLongA:
        return get_window_long_size( hwnd, param, sizeof(LONG), TRUE );
    case NtUserGetWindowLongW:
        return get_window_long( hwnd, param );
    case NtUserGetWindowLongPtrA:
        return get_window_long_ptr( hwnd, param, TRUE );
    case NtUserGetWindowLongPtrW:
        return get_window_long_ptr( hwnd, param, FALSE );
    case NtUserGetWindowPlacement:
        return get_window_placement( hwnd, (WINDOWPLACEMENT *)param );
    case NtUserGetWindowRect:
        return get_window_rect( hwnd, (RECT *)param, get_thread_dpi() );
    case NtUserGetWindowRelative:
        return HandleToUlong( get_window_relative( hwnd, param ));
    case NtUserGetWindowThread:
        return get_window_thread( hwnd, (DWORD *)param );
    case NtUserGetWindowWord:
        return get_window_word( hwnd, param );
    case NtUserIsChild:
        return is_child( hwnd, UlongToHandle(param) );
    case NtUserMonitorFromWindow:
        return HandleToUlong( monitor_from_window( hwnd, param, NtUserMonitorFromWindow ));
    case NtUserSetCaptureWindow:
        return set_capture_window( hwnd, param, NULL );
    case NtUserSetForegroundWindow:
        return set_foreground_window( hwnd, param );
    /* temporary exports */
    case NtUserIsWindowDrawable:
        return is_window_drawable( hwnd, param );
    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}
