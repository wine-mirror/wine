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

#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/opengl_driver.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define USER_HANDLE_TO_INDEX(hwnd) ((LOWORD(hwnd) - FIRST_USER_HANDLE) >> 1)
#define USER_HANDLE_FROM_INDEX(index, generation) UlongToHandle( (index << 1) + FIRST_USER_HANDLE + (generation << 16) )

static void *client_objects[MAX_USER_HANDLES];

#define SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOCLIENTSIZE | SWP_NOZORDER)
#define SWP_AGG_NOPOSCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)
#define SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)
#define SWP_AGG_NOCLIENTCHANGE \
        (SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)

#define PLACE_MIN		0x0001
#define PLACE_MAX		0x0002
#define PLACE_RECT		0x0004

/***********************************************************************
 *           alloc_user_handle
 */
HANDLE alloc_user_handle( void *ptr, unsigned short type )
{
    HANDLE handle = 0;

    SERVER_START_REQ( alloc_user_handle )
    {
        req->type = type;
        if (!wine_server_call_err( req )) handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (handle)
    {
        UINT index = USER_HANDLE_TO_INDEX( handle );

        assert( index < MAX_USER_HANDLES );
        InterlockedExchangePointer( &client_objects[index], ptr );
    }
    return handle;
}

static BOOL is_valid_entry_uniq( HANDLE handle, unsigned short type, UINT64 uniq )
{
    if (!HIWORD(handle) || HIWORD(handle) == 0xffff) return LOWORD(uniq) == type;
    return uniq == MAKELONG(type, HIWORD(handle));
}

static BOOL is_valid_entry( HANDLE handle, unsigned short type )
{
    const volatile struct user_entry *entries = shared_session->user_entries;
    WORD index = USER_HANDLE_TO_INDEX( handle );
    if (index >= MAX_USER_HANDLES) return FALSE;
    return is_valid_entry_uniq( handle, type, ReadAcquire64( (LONG64 *)&entries[index].uniq ) );
}

static BOOL read_acquire_user_entry( HANDLE handle, unsigned short type, const volatile struct user_entry *src, struct user_entry *dst )
{
    UINT64 uniq = ReadAcquire64( (LONG64 *)&src->uniq );
    if (!is_valid_entry_uniq( handle, type, uniq )) return FALSE;
    dst->offset = src->offset;
    dst->tid = src->tid;
    dst->pid = src->pid;
    dst->id = src->id;
    __SHARED_READ_FENCE;
    dst->uniq = ReadNoFence64( &src->uniq );
    return dst->uniq == uniq;
}

static BOOL get_user_entry( HANDLE handle, unsigned short type, struct user_entry *entry, HANDLE *full )
{
    const volatile struct user_entry *entries = shared_session->user_entries;
    WORD index = USER_HANDLE_TO_INDEX( handle );

    if (index >= MAX_USER_HANDLES) return FALSE;
    if (!read_acquire_user_entry( handle, type, entries + index, entry )) return FALSE;
    *full = USER_HANDLE_FROM_INDEX( index, entry->generation );
    return TRUE;
}

static BOOL get_user_entry_at( WORD index, unsigned short type, struct user_entry *entry, HANDLE *full )
{
    const volatile struct user_entry *entries = shared_session->user_entries;
    if (index >= MAX_USER_HANDLES) return FALSE;
    if (!read_acquire_user_entry( 0, type, entries + index, entry )) return FALSE;
    *full = USER_HANDLE_FROM_INDEX( index, entry->generation );
    return TRUE;
}

static NTSTATUS get_shared_window( HANDLE handle, struct object_lock *lock, const window_shm_t **window_shm )
{
    const shared_object_t *object;
    struct user_entry entry;

    TRACE( "handle %p, lock %p, window_shm %p\n", handle, lock, window_shm );

    if (!get_user_entry( handle, NTUSER_OBJ_WINDOW, &entry, &handle )) return STATUS_INVALID_HANDLE;

    if (lock->id) object = CONTAINING_RECORD( *window_shm, shared_object_t, shm.window );
    else
    {
        object = find_shared_session_object( entry.id, entry.offset );
        if (!object) return STATUS_INVALID_HANDLE;
    }

    if (!lock->id || !shared_object_release_seqlock( object, lock->seq ))
    {
        shared_object_acquire_seqlock( object, &lock->seq );
        *window_shm = &object->shm.window;
        lock->id = object->id;
        return STATUS_PENDING;
    }

    return STATUS_SUCCESS;
}

/***********************************************************************
 *           get_user_handle_ptr
 */
void *get_user_handle_ptr( HANDLE handle, unsigned short type )
{
    WORD index = USER_HANDLE_TO_INDEX( handle );
    void *ptr = NULL;
    struct user_entry entry;

    if (index >= MAX_USER_HANDLES) return NULL;

    user_lock();

    if (!get_user_entry( handle, type, &entry, &handle )) ptr = NULL;
    else if (entry.pid != GetCurrentProcessId()) ptr = OBJ_OTHER_PROCESS;
    else ptr = client_objects[index];

    if (!ptr || ptr == OBJ_OTHER_PROCESS) user_unlock();
    return ptr;
}

/***********************************************************************
 *           next_thread_user_object
 *
 * user_lock must be held by caller.
 */
void *next_thread_user_object( UINT tid, HANDLE *handle, unsigned short type )
{
    WORD index = *handle ? USER_HANDLE_TO_INDEX( *handle ) + 1 : 0;
    struct user_entry entry;
    UINT i;

    for (i = index; i < MAX_USER_HANDLES; i++)
    {
        if (!get_user_entry_at( i, type, &entry, handle )) continue;
        if (entry.pid != GetCurrentProcessId()) continue;
        if (tid != -1 && entry.tid != tid) continue;
        return client_objects[i];
    }

    return NULL;
}

/***********************************************************************
 *           set_user_handle_ptr
 */
static void set_user_handle_ptr( HANDLE handle, void *ptr )
{
    WORD index = USER_HANDLE_TO_INDEX(handle);
    assert( index < MAX_USER_HANDLES );
    InterlockedExchangePointer( &client_objects[index], ptr );
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
void *free_user_handle( HANDLE handle, unsigned short type )
{
    void *ptr;
    WORD index = USER_HANDLE_TO_INDEX( handle );

    if ((ptr = get_user_handle_ptr( handle, type )) && ptr != OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( free_user_handle )
        {
            req->type = type;
            req->handle = wine_server_user_handle( handle );
            if (wine_server_call( req )) ptr = NULL;
            else InterlockedCompareExchangePointer( &client_objects[index], NULL, ptr );
        }
        SERVER_END_REQ;
        user_unlock();
    }
    return ptr;
}

/*******************************************************************
 *           get_hwnd_message_parent
 *
 * Return the parent for HWND_MESSAGE windows.
 */
HWND get_hwnd_message_parent(void)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

    if (!thread_info->msg_window) get_desktop_window(); /* trigger creation */
    return UlongToHandle( thread_info->msg_window );
}

/***********************************************************************
 *           get_full_window_handle
 *
 * Convert a possibly truncated window handle to a full 32-bit handle.
 */
HWND get_full_window_handle( HWND hwnd )
{
    struct user_entry entry;
    HANDLE handle;

    if (!hwnd || (ULONG_PTR)hwnd >> 16) return hwnd;
    if (LOWORD(hwnd) <= 1 || LOWORD(hwnd) == 0xffff) return hwnd;
    /* do sign extension for -2 and -3 */
    if (LOWORD(hwnd) >= (WORD)-3) return (HWND)(LONG_PTR)(INT16)LOWORD(hwnd);

    if (!get_user_entry( hwnd, NTUSER_OBJ_WINDOW, &entry, &handle )) return hwnd;
    return handle;
}

/*******************************************************************
 *           is_desktop_window
 *
 * Check if window is the desktop or the HWND_MESSAGE top parent.
 */
BOOL is_desktop_window( HWND hwnd )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

    if (!hwnd) return FALSE;
    if (hwnd == UlongToHandle( thread_info->top_window )) return TRUE;
    if (hwnd == UlongToHandle( thread_info->msg_window )) return TRUE;

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
    struct user_entry entry;
    HANDLE handle;

    if (!get_user_entry( hwnd, NTUSER_OBJ_WINDOW, &entry, &handle )) return 0;
    if (entry.tid != GetCurrentThreadId()) return 0;
    return handle;
}

/***********************************************************************
 *           is_current_process_window
 *
 * Check whether a given window belongs to the current process (and return the full handle).
 */
HWND is_current_process_window( HWND hwnd )
{
    struct user_entry entry;
    HANDLE handle;

    if (!get_user_entry( hwnd, NTUSER_OBJ_WINDOW, &entry, &handle )) return 0;
    if (entry.pid != GetCurrentProcessId()) return 0;
    return handle;
}

/* see IsWindow */
BOOL is_window( HWND hwnd )
{
    if (!hwnd) return FALSE;
    if (!is_valid_entry( hwnd, NTUSER_OBJ_WINDOW ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    return TRUE;
}

/* see GetWindowThreadProcessId */
DWORD get_window_thread( HWND hwnd, DWORD *process )
{
    struct user_entry entry;
    HANDLE handle;

    if (!get_user_entry( hwnd, NTUSER_OBJ_WINDOW, &entry, &handle ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    if (process) *process = entry.pid;
    return entry.tid;
}

/* see GetParent */
HWND get_parent( HWND hwnd )
{
    HWND retval = 0;
    WND *win;

    if (!(win = get_win_ptr( hwnd )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        DWORD style = get_window_long( hwnd, GWL_STYLE );
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

/*****************************************************************
 *           NtUserSetParent (win32u.@)
 */
HWND WINAPI NtUserSetParent( HWND hwnd, HWND parent )
{
    RECT window_rect = {0}, old_screen_rect = {0}, new_screen_rect = {0};
    UINT context;
    WINDOWPOS winpos;
    HWND full_handle;
    HWND old_parent = 0;
    BOOL was_visible;
    WND *win;
    BOOL ret;

    TRACE("(%p %p)\n", hwnd, parent);

    if (is_broadcast(hwnd) || is_broadcast(parent))
    {
        RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!parent) parent = get_desktop_window();
    else if (parent == HWND_MESSAGE) parent = get_hwnd_message_parent();
    else parent = get_full_window_handle( parent );

    if (!is_window( parent ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    /* Some applications try to set a child as a parent */
    if (is_child( hwnd, parent ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (!(full_handle = is_current_thread_window( hwnd )))
        return UlongToHandle( send_message( hwnd, WM_WINE_SETPARENT, (WPARAM)parent, 0 ));

    if (full_handle == parent)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* Windows hides the window first, then shows it again
     * including the WM_SHOWWINDOW messages and all */
    was_visible = NtUserShowWindow( hwnd, SW_HIDE );

    win = get_win_ptr( hwnd );
    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return 0;

    get_window_rect_rel( hwnd, COORDS_PARENT, &window_rect, get_dpi_for_window(hwnd) );
    get_window_rect_rel( hwnd, COORDS_SCREEN, &old_screen_rect, 0 );

    SERVER_START_REQ( set_parent )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->parent = wine_server_user_handle( parent );
        if ((ret = !wine_server_call_err( req )))
        {
            old_parent = wine_server_ptr_handle( reply->old_parent );
            win->parent = parent = wine_server_ptr_handle( reply->full_parent );
        }

    }
    SERVER_END_REQ;
    release_win_ptr( win );
    if (!ret) return 0;

    get_window_rect_rel( hwnd, COORDS_SCREEN, &new_screen_rect, 0 );
    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));

    user_driver->pSetParent( full_handle, parent, old_parent );

    winpos.hwnd = hwnd;
    winpos.hwndInsertAfter = HWND_TOP;
    winpos.x = window_rect.left;
    winpos.y = window_rect.top;
    winpos.cx = 0;
    winpos.cy = 0;
    winpos.flags = SWP_NOSIZE;

    set_window_pos( &winpos, new_screen_rect.left - old_screen_rect.left,
                    new_screen_rect.top - old_screen_rect.top );

    if (was_visible) NtUserShowWindow( hwnd, SW_SHOW );

    set_thread_dpi_awareness_context( context );
    return old_parent;
}

/* see GetWindow */
HWND get_window_relative( HWND hwnd, UINT rel )
{
    HWND retval = 0;

    if (rel == GW_OWNER)  /* this one may be available locally */
    {
        WND *win = get_win_ptr( hwnd );
        if (!win)
        {
            RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
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
 * freed with free(). Returns NULL when no windows are found.
 */
HWND *list_window_children( HWND hwnd )
{
    HWND *list;
    ULONG size = 128;
    NTSTATUS status;

    if (hwnd && !(hwnd = get_window_relative( hwnd, GW_CHILD ))) return NULL;

    for (;;)
    {
        if (!(list = malloc( size * sizeof(HWND) ))) return NULL;
        status = NtUserBuildHwndList( 0, hwnd, FALSE, TRUE, 0, size, list, &size );
        if (!status && size) break;
        free( list );
        if (status != STATUS_BUFFER_TOO_SMALL) return NULL;
    }
    list[size - 1] = 0;
    return list;
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
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
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
BOOL is_window_visible( HWND hwnd )
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
BOOL is_window_drawable( HWND hwnd, BOOL icon )
{
    HWND *list;
    BOOL retval = TRUE;
    int i;
    DWORD style = get_window_long( hwnd, GWL_STYLE );

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
BOOL is_window_unicode( HWND hwnd )
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

/*****************************************************************
 *           NtUserEnableWindow (win32u.@)
 */
BOOL WINAPI NtUserEnableWindow( HWND hwnd, BOOL enable )
{
    BOOL ret;

    if (is_broadcast(hwnd))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    TRACE( "( %p, %d )\n", hwnd, enable );

    if (enable)
    {
        ret = (set_window_style_bits( hwnd, 0, WS_DISABLED ) & WS_DISABLED) != 0;
        if (ret)
        {
            NtUserNotifyWinEvent( EVENT_OBJECT_STATECHANGE, hwnd, OBJID_WINDOW, 0 );

            send_message( hwnd, WM_ENABLE, TRUE, 0 );
        }
    }
    else
    {
        send_message( hwnd, WM_CANCELMODE, 0, 0 );

        ret = (set_window_style_bits( hwnd, WS_DISABLED, 0 ) & WS_DISABLED) != 0;
        if (!ret)
        {
            NtUserNotifyWinEvent( EVENT_OBJECT_STATECHANGE, hwnd, OBJID_WINDOW, 0 );

            if (hwnd == get_focus())
                NtUserSetFocus( 0 ); /* A disabled window can't have the focus */

            send_message( hwnd, WM_ENABLE, FALSE, 0 );
        }
    }
    return ret;
}


/* see IsHungAppWindow */
static BOOL is_hung_app_window( HWND hwnd )
{
    BOOL ret;

    SERVER_START_REQ( is_window_hung )
    {
        req->win = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req ) && reply->is_hung;
    }
    SERVER_END_REQ;
    return ret;
}

/* see IsWindowEnabled */
BOOL is_window_enabled( HWND hwnd )
{
    DWORD ret;

    RtlSetLastWin32Error( NO_ERROR );
    ret = get_window_long( hwnd, GWL_STYLE );
    if (!ret && RtlGetLastWin32Error() != NO_ERROR) return FALSE;
    return !(ret & WS_DISABLED);
}

/* see GetWindowDpiAwarenessContext */
UINT get_window_dpi_awareness_context( HWND hwnd )
{
    struct object_lock lock = OBJECT_LOCK_INIT;
    const window_shm_t *window_shm;
    UINT status, ctx = 0;

    while ((status = get_shared_window( hwnd, &lock, &window_shm )) == STATUS_PENDING)
        ctx = window_shm->dpi_context;
    if (status)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    return ctx;
}

/* see GetDpiForWindow */
UINT get_dpi_for_window( HWND hwnd )
{
    UINT raw_dpi, context = get_window_dpi_awareness_context( hwnd );
    if (NTUSER_DPI_CONTEXT_IS_MONITOR_AWARE( context )) return get_win_monitor_dpi( hwnd, &raw_dpi );
    return NTUSER_DPI_CONTEXT_GET_DPI( context );
}

/* see GetLastActivePopup */
static HWND get_last_active_popup( HWND hwnd )
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

/* helper for set_window_long */
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

BOOL is_iconic( HWND hwnd )
{
    return (get_window_long( hwnd, GWL_STYLE ) & WS_MINIMIZE) != 0;
}

BOOL is_zoomed( HWND hwnd )
{
    return (get_window_long( hwnd, GWL_STYLE ) & WS_MAXIMIZE) != 0;
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
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
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
            RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
            return 0;
        }
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        return 0;
    }

    if (win == WND_OTHER_PROCESS)
    {
        if (offset == GWLP_WNDPROC)
        {
            RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
            return 0;
        }
        SERVER_START_REQ( get_window_info )
        {
            req->handle = wine_server_user_handle( hwnd );
            req->offset = offset;
            req->size = size;
            if (!wine_server_call_err( req )) retval = reply->info;
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
            RtlSetLastWin32Error( ERROR_INVALID_INDEX );
            return 0;
        }
        retval = get_win_data( (char *)win->wExtra + offset, size );
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
        if (win->winproc == BUILTIN_WINPROC(NTUSER_WNDPROC_EDIT) && (!!ansi != !(win->flags & WIN_ISUNICODE)))
            retval = (ULONG_PTR)win->winproc;
        else
            retval = (ULONG_PTR)get_winproc( win->winproc, ansi );
        break;
    default:
        WARN("Unknown offset %d\n", offset );
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
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
ULONG_PTR get_window_long_ptr( HWND hwnd, INT offset, BOOL ansi )
{
    return get_window_long_size( hwnd, offset, sizeof(LONG_PTR), ansi );
}

/* see GetWindowWord */
static WORD get_window_word( HWND hwnd, INT offset )
{
    if (offset < 0 && offset != GWLP_USERDATA)
    {
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        return 0;
    }
    return get_window_long_size( hwnd, offset, sizeof(WORD), TRUE );
}

UINT set_window_style_bits( HWND hwnd, UINT set_bits, UINT clear_bits )
{
    BOOL ok, made_visible = FALSE;
    STYLESTRUCT style;
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (is_window(hwnd))
            return send_message( hwnd, WM_WINE_SETSTYLE, set_bits, clear_bits );
        return 0;
    }
    style.styleOld = win->dwStyle;
    style.styleNew = (win->dwStyle | set_bits) & ~clear_bits;
    if (style.styleNew == style.styleOld)
    {
        release_win_ptr( win );
        return style.styleNew;
    }
    SERVER_START_REQ( set_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->offset = GWL_STYLE;
        req->new_info = style.styleNew;
        if ((ok = !wine_server_call( req )))
        {
            style.styleOld = reply->old_info;
            win->dwStyle = style.styleNew;
        }
    }
    SERVER_END_REQ;

    if (ok && ((style.styleOld ^ style.styleNew) & WS_VISIBLE))
    {
        made_visible = (style.styleNew & WS_VISIBLE) != 0;
        invalidate_dce( win, NULL );
    }
    release_win_ptr( win );

    if (!ok) return 0;

    user_driver->pSetWindowStyle( hwnd, GWL_STYLE, &style );
    if (made_visible) update_window_state( hwnd );

    return style.styleOld;
}

/**********************************************************************
 *           NtUserAlterWindowStyle (win32u.@)
 */
ULONG WINAPI NtUserAlterWindowStyle( HWND hwnd, UINT mask, UINT style )
{
    TRACE( "hwnd %p, mask %#x, style %#x\n", hwnd, mask, style );
    /* FIXME: WS_TABSTOP shouldn't be there but we need it for BM_SETCHECK */
    mask &= WS_TABSTOP | WS_VSCROLL | WS_HSCROLL | 0x23f;
    return !!set_window_style_bits( hwnd, style & mask, mask & ~style );
}

static DWORD fix_exstyle( DWORD style, DWORD exstyle )
{
    if ((exstyle & WS_EX_DLGMODALFRAME) ||
        (!(exstyle & WS_EX_STATICEDGE) && (style & (WS_DLGFRAME | WS_THICKFRAME))))
        exstyle |= WS_EX_WINDOWEDGE;
    else
        exstyle &= ~WS_EX_WINDOWEDGE;
    return exstyle;
}

/* Change the owner of a window. */
static HWND set_window_owner( HWND hwnd, HWND owner )
{
    WND *win = get_win_ptr( hwnd );
    HWND ret = 0;

    if (!win || win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (is_window(hwnd)) ERR( "cannot set owner %p on other process window %p\n", owner, hwnd );
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
    release_win_ptr( win );
    return ret;
}

/* Helper function for SetWindowLong(). */
LONG_PTR set_window_long( HWND hwnd, INT offset, UINT size, LONG_PTR newval, BOOL ansi )
{
    BOOL ok, made_visible = FALSE, layered = FALSE;
    LONG_PTR retval = 0;
    STYLESTRUCT style;
    WND *win;

    TRACE( "%p %d %lx %c\n", hwnd, offset, newval, ansi ? 'A' : 'W' );

    if (is_broadcast(hwnd))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(win = get_win_ptr( hwnd )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_DESKTOP)
    {
        /* can't change anything on the desktop window */
        RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
        return 0;
    }
    if (win == WND_OTHER_PROCESS)
    {
        if (offset == GWLP_WNDPROC)
        {
            RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
            return 0;
        }
        if (offset > 32767 || offset < -32767)
        {
            RtlSetLastWin32Error( ERROR_INVALID_INDEX );
            return 0;
        }
        return send_message( hwnd, WM_WINE_SETWINDOWLONG, MAKEWPARAM( offset, size ), newval );
    }

    /* first some special cases */
    switch( offset )
    {
    case GWL_STYLE:
        style.styleOld = win->dwStyle;
        style.styleNew = newval;
        release_win_ptr( win );
        send_message( hwnd, WM_STYLECHANGING, GWL_STYLE, (LPARAM)&style );
        if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS) return 0;
        newval = style.styleNew;
        /* WS_CLIPSIBLINGS can't be reset on top-level windows */
        if (win->parent == get_desktop_window()) newval |= WS_CLIPSIBLINGS;
        /* WS_MINIMIZE can't be reset */
        if (win->dwStyle & WS_MINIMIZE) newval |= WS_MINIMIZE;
        break;
    case GWL_EXSTYLE:
        style.styleOld = win->dwExStyle;
        style.styleNew = newval;
        release_win_ptr( win );
        send_message( hwnd, WM_STYLECHANGING, GWL_EXSTYLE, (LPARAM)&style );
        if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS) return 0;
        /* WS_EX_TOPMOST can only be changed through SetWindowPos */
        newval = (style.styleNew & ~WS_EX_TOPMOST) | (win->dwExStyle & WS_EX_TOPMOST);
        newval = fix_exstyle(win->dwStyle, newval);
        break;
    case GWLP_HWNDPARENT:
        if (win->parent == get_desktop_window())
        {
            release_win_ptr( win );
            return (ULONG_PTR)set_window_owner( hwnd, (HWND)newval );
        }
        else
        {
            release_win_ptr( win );
            return (ULONG_PTR)NtUserSetParent( hwnd, (HWND)newval );
        }
    case GWLP_WNDPROC:
    {
        WNDPROC proc;
        UINT old_flags = win->flags;
        retval = get_window_long_ptr( hwnd, offset, ansi );
        proc = alloc_winproc( (WNDPROC)newval, ansi );
        if (proc) win->winproc = proc;
        if (is_winproc_unicode( proc, !ansi )) win->flags |= WIN_ISUNICODE;
        else win->flags &= ~WIN_ISUNICODE;
        if (!((old_flags ^ win->flags) & WIN_ISUNICODE))
        {
            release_win_ptr( win );
            return retval;
        }
        /* update is_unicode flag on the server side */
        break;
    }
    case GWLP_ID:
    case GWLP_HINSTANCE:
    case GWLP_USERDATA:
        break;
    default:
        if (offset < 0 || offset > (int)(win->cbWndExtra - size))
        {
            WARN("Invalid offset %d\n", offset );
            release_win_ptr( win );
            RtlSetLastWin32Error( ERROR_INVALID_INDEX );
            return 0;
        }
        else if (get_win_data( (char *)win->wExtra + offset, size ) == newval)
        {
            /* already set to the same value */
            release_win_ptr( win );
            return newval;
        }
        break;
    }

    if (offset == GWLP_WNDPROC) newval = !!(win->flags & WIN_ISUNICODE);
    if (offset == GWLP_USERDATA && size == sizeof(WORD)) newval = MAKELONG( newval, win->userdata >> 16 );

    SERVER_START_REQ( set_window_info )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->offset = offset;
        req->new_info = newval;
        req->size = size;
        if ((ok = !wine_server_call_err( req )))
        {
            switch (offset)
            {
            case GWL_STYLE:
                win->dwStyle = newval;
                win->dwExStyle = fix_exstyle(win->dwStyle, win->dwExStyle);
                retval = reply->old_info;
                break;
            case GWL_EXSTYLE:
                win->dwExStyle = newval;
                retval = reply->old_info;
                break;
            case GWLP_ID:
                win->wIDmenu = newval;
                retval = reply->old_info;
                break;
            case GWLP_HINSTANCE:
                win->hInstance = (HINSTANCE)newval;
                retval = reply->old_info;
                break;
            case GWLP_WNDPROC:
                break;
            case GWLP_USERDATA:
                win->userdata = newval;
                retval = reply->old_info;
                break;
            default:
                set_win_data( (char *)win->wExtra + offset, newval, size );
                retval = reply->old_info;
                break;
            }
        }
    }
    SERVER_END_REQ;

    if (offset == GWL_EXSTYLE && ((style.styleOld ^ style.styleNew) & WS_EX_LAYERED)) layered = TRUE;
    if ((offset == GWL_STYLE && ((style.styleOld ^ style.styleNew) & WS_VISIBLE)) || layered)
    {
        made_visible = (win->dwStyle & WS_VISIBLE) != 0;
        invalidate_dce( win, NULL );
    }
    release_win_ptr( win );

    if (!ok) return 0;

    if (offset == GWL_STYLE || offset == GWL_EXSTYLE)
    {
        style.styleOld = retval;
        style.styleNew = newval;
        user_driver->pSetWindowStyle( hwnd, offset, &style );
        if (made_visible || layered) update_window_state( hwnd );
        send_message( hwnd, WM_STYLECHANGED, offset, (LPARAM)&style );
    }

    return retval;
}

/**********************************************************************
 *           NtUserSetWindowWord (win32u.@)
 */
WORD WINAPI NtUserSetWindowWord( HWND hwnd, INT offset, WORD newval )
{
    if (offset < 0 && offset != GWLP_USERDATA)
    {
        RtlSetLastWin32Error( ERROR_INVALID_INDEX );
        return 0;
    }
    return set_window_long( hwnd, offset, sizeof(WORD), newval, TRUE );
}

/**********************************************************************
 *           NtUserSetWindowLong (win32u.@)
 */
LONG WINAPI NtUserSetWindowLong( HWND hwnd, INT offset, LONG newval, BOOL ansi )
{
    return set_window_long( hwnd, offset, sizeof(LONG), newval, ansi );
}

/*****************************************************************************
 *           NtUserSetWindowLongPtr (win32u.@)
 */
LONG_PTR WINAPI NtUserSetWindowLongPtr( HWND hwnd, INT offset, LONG_PTR newval, BOOL ansi )
{
    return set_window_long( hwnd, offset, sizeof(LONG_PTR), newval, ansi );
}

BOOL set_window_pixel_format( HWND hwnd, int format, BOOL internal )
{
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        WARN( "setting format %d on win %p not supported\n", format, hwnd );
        return FALSE;
    }
    if (internal)
        win->internal_pixel_format = format;
    else
        win->pixel_format = format;
    release_win_ptr( win );

    update_window_state( hwnd );
    return TRUE;
}

int get_window_pixel_format( HWND hwnd, BOOL internal )
{
    WND *win = get_win_ptr( hwnd );
    int ret;

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS)
    {
        WARN( "getting format on win %p not supported\n", hwnd );
        return 0;
    }

    ret = internal && win->internal_pixel_format ? win->internal_pixel_format : win->pixel_format;
    release_win_ptr( win );

    return ret;
}

static int window_has_client_surface( HWND hwnd )
{
    WND *win = get_win_ptr( hwnd );
    BOOL ret;

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return FALSE;
    ret = win->pixel_format || win->internal_pixel_format || !list_empty(&win->vulkan_surfaces);
    release_win_ptr( win );

    return ret;
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


/***********************************************************************
 *           NtUserBuildPropList   (win32u.@)
 */
NTSTATUS WINAPI NtUserBuildPropList( HWND hwnd, ULONG count, struct ntuser_property_list *buffer, ULONG *ret_count )
{
    struct property_data *data;
    ULONG i;
    NTSTATUS status;

    if (!buffer || !ret_count) return STATUS_INVALID_PARAMETER;

    if (!(data = malloc( count * sizeof(*data) ))) return STATUS_NO_MEMORY;

    SERVER_START_REQ( get_window_properties )
    {
        req->window = wine_server_user_handle( hwnd );
        wine_server_set_reply( req, data, count * sizeof(*data) );
        if (!(status = wine_server_call( req )))
        {
            for (i = 0; i < wine_server_reply_size(reply) / sizeof(*data); i++)
            {
                buffer[i].data   = data[i].data;
                buffer[i].atom   = data[i].atom;
                buffer[i].string = data[i].string;
            }
            *ret_count = reply->total;
            if (reply->total > count) status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    SERVER_END_REQ;

    free( data );
    return status;
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
BOOL get_window_rects( HWND hwnd, enum coords_relative relative, struct window_rects *rects, UINT dpi )
{
    WND *win = get_win_ptr( hwnd );
    BOOL ret = TRUE;

    if (!win)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
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
        rects->window = rect;
        rects->client = rect;
        rects->visible = rect;
        return TRUE;
    }
    if (win != WND_OTHER_PROCESS)
    {
        UINT window_dpi = get_dpi_for_window( hwnd );
        *rects = win->rects;

        switch (relative)
        {
        case COORDS_CLIENT:
            OffsetRect( &rects->window, -win->rects.client.left, -win->rects.client.top );
            OffsetRect( &rects->client, -win->rects.client.left, -win->rects.client.top );
            OffsetRect( &rects->visible, -win->rects.client.left, -win->rects.client.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_rect( &win->rects.client, &rects->window );
                mirror_rect( &win->rects.client, &rects->visible );
            }
            break;
        case COORDS_WINDOW:
            OffsetRect( &rects->window, -win->rects.window.left, -win->rects.window.top );
            OffsetRect( &rects->client, -win->rects.window.left, -win->rects.window.top );
            OffsetRect( &rects->visible, -win->rects.window.left, -win->rects.window.top );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_rect( &win->rects.window, &rects->client );
                mirror_rect( &win->rects.window, &rects->visible );
            }
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
                    mirror_rect( &parent->rects.client, &rects->window );
                    mirror_rect( &parent->rects.client, &rects->client );
                    mirror_rect( &parent->rects.client, &rects->visible );
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
                    OffsetRect( &rects->window, win->rects.client.left, win->rects.client.top );
                    OffsetRect( &rects->client, win->rects.client.left, win->rects.client.top );
                    OffsetRect( &rects->visible, win->rects.client.left, win->rects.client.top );
                }
            }
            break;
        }
        rects->window = map_dpi_rect( rects->window, window_dpi, dpi );
        rects->client = map_dpi_rect( rects->client, window_dpi, dpi );
        rects->visible = map_dpi_rect( rects->visible, window_dpi, dpi );
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
            rects->window = wine_server_get_rect( reply->window );
            rects->client = wine_server_get_rect( reply->client );
            rects->visible = rects->window;
        }
    }
    SERVER_END_REQ;
    return ret;
}

BOOL get_window_rect_rel( HWND hwnd, enum coords_relative rel, RECT *rect, UINT dpi )
{
    struct window_rects rects;
    BOOL ret = get_window_rects( hwnd, rel, &rects, dpi );
    if (ret) *rect = rects.window;
    return ret;
}

/* see GetWindowRect */
BOOL get_window_rect( HWND hwnd, RECT *rect, UINT dpi )
{
    return get_window_rect_rel( hwnd, COORDS_SCREEN, rect, dpi );
}

BOOL get_client_rect_rel( HWND hwnd, enum coords_relative rel, RECT *rect, UINT dpi )
{
    struct window_rects rects;
    BOOL ret = get_window_rects( hwnd, rel, &rects, dpi );
    if (ret) *rect = rects.client;
    return ret;
}

/* see GetClientRect */
BOOL get_client_rect( HWND hwnd, RECT *rect, UINT dpi )
{
    return get_client_rect_rel( hwnd, COORDS_CLIENT, rect, dpi );
}

/* see GetWindowInfo */
static BOOL get_window_info( HWND hwnd, WINDOWINFO *info )
{
    struct window_rects rects;

    if (!info || !get_window_rects( hwnd, COORDS_SCREEN, &rects, get_thread_dpi() ))
        return FALSE;

    info->rcWindow        = rects.window;
    info->rcClient        = rects.client;
    info->dwStyle         = get_window_long( hwnd, GWL_STYLE );
    info->dwExStyle       = get_window_long( hwnd, GWL_EXSTYLE );
    info->dwWindowStatus  = get_active_window() == hwnd ? WS_ACTIVECAPTION : 0;
    info->cxWindowBorders = info->rcClient.left - info->rcWindow.left;
    info->cyWindowBorders = info->rcWindow.bottom - info->rcClient.bottom;
    info->atomWindowType  = get_class_long( hwnd, GCW_ATOM, FALSE );
    info->wCreatorVersion = 0x0400;
    return TRUE;
}

static NTSTATUS get_window_region( HWND hwnd, BOOL surface, HRGN *region, RECT *visible )
{
    NTSTATUS status;
    RGNDATA *data;
    size_t size = 256;

    *region = 0;
    do
    {
        if (!(data = malloc( FIELD_OFFSET( RGNDATA, Buffer[size] )))) return STATUS_NO_MEMORY;

        SERVER_START_REQ( get_window_region )
        {
            req->window = wine_server_user_handle( hwnd );
            req->surface = surface;
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
                    *region = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                    *visible = wine_server_get_rect( reply->visible_rect );
                }
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;

        free( data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    return status;
}

/***********************************************************************
 *           update_surface_region
 */
static void update_surface_region( HWND hwnd )
{
    WND *win = get_win_ptr( hwnd );
    HRGN region, shape = 0;
    RECT visible;

    if (!win || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return;
    if (!win->surface) goto done;

    if (get_window_region( hwnd, FALSE, &shape, &visible )) goto done;
    if (shape)
    {
        region = NtGdiCreateRectRgn( 0, 0, visible.right - visible.left, visible.bottom - visible.top );
        NtGdiCombineRgn( shape, shape, region, RGN_AND );
        if (win->dwExStyle & WS_EX_LAYOUTRTL) NtUserMirrorRgn( hwnd, shape );
        NtGdiDeleteObjectApp( region );
    }
    window_surface_set_shape( win->surface, shape );

    if (get_window_region( hwnd, TRUE, &region, &visible )) goto done;
    if (!region) window_surface_set_clip( win->surface, shape );
    else
    {
        NtGdiOffsetRgn( region, -visible.left, -visible.top );
        if (shape) NtGdiCombineRgn( region, region, shape, RGN_AND );
        window_surface_set_clip( win->surface, region );
        NtGdiDeleteObjectApp( region );
    }

done:
    if (shape) NtGdiDeleteObjectApp( shape );
    release_win_ptr( win );
}


static RECT get_visible_rect( HWND hwnd, BOOL shaped, UINT style, UINT ex_style, const struct window_rects *rects )
{
    UINT dpi = get_dpi_for_window( hwnd ), style_mask, ex_style_mask;
    RECT visible_rect, rect = {0};

    if (IsRectEmpty( &rects->window ) || EqualRect( &rects->window, &rects->client ) || shaped || !decorated_mode) return rects->window;
    if (!user_driver->pGetWindowStyleMasks( hwnd, style, ex_style, &style_mask, &ex_style_mask )) return rects->window;
    if (!NtUserAdjustWindowRect( &rect, style & style_mask, FALSE, ex_style & ex_style_mask, dpi )) return rects->window;

    visible_rect = rects->window;
    visible_rect.left   -= rect.left;
    visible_rect.right  -= rect.right;
    visible_rect.top    -= rect.top;
    visible_rect.bottom -= rect.bottom;
    if (visible_rect.top >= visible_rect.bottom) visible_rect.bottom = visible_rect.top + 1;
    if (visible_rect.left >= visible_rect.right) visible_rect.right = visible_rect.left + 1;

    TRACE( "hwnd %p, rects %s, style %#x, ex_style %#x -> visible_rect %s\n", hwnd,
           debugstr_window_rects( rects ), style, ex_style, wine_dbgstr_rect( &visible_rect ) );
    return visible_rect;
}


static BOOL get_surface_rect( const RECT *visible_rect, RECT *surface_rect )
{
    RECT virtual_rect = get_virtual_screen_rect( 0, MDT_RAW_DPI );

    *surface_rect = *visible_rect;

    /* crop surfaces which are larger than the virtual screen rect, some applications create huge windows */
    if ((surface_rect->right - surface_rect->left > virtual_rect.right - virtual_rect.left ||
         surface_rect->bottom - surface_rect->top > virtual_rect.bottom - virtual_rect.top) &&
        !intersect_rect( surface_rect, surface_rect, &virtual_rect ))
        return FALSE;
    OffsetRect( surface_rect, -visible_rect->left, -visible_rect->top );

    /* round the surface coordinates to avoid re-creating them too often on resize */
    surface_rect->left &= ~127;
    surface_rect->top  &= ~127;
    surface_rect->right  = max( surface_rect->left + 128, (surface_rect->right + 127) & ~127 );
    surface_rect->bottom = max( surface_rect->top + 128, (surface_rect->bottom + 127) & ~127 );
    return TRUE;
}

static BOOL get_default_window_surface( HWND hwnd, const RECT *surface_rect, struct window_surface **surface )
{
    struct window_surface *previous;
    WND *win;

    TRACE( "hwnd %p, surface_rect %s, surface %p\n", hwnd, wine_dbgstr_rect( surface_rect ), surface );

    if (!(win = get_win_ptr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return FALSE;

    if ((previous = win->surface) && EqualRect( &previous->rect, surface_rect ))
    {
        window_surface_add_ref( (*surface = previous) );
        TRACE( "trying to reuse previous surface %p\n", previous );
    }
    else if (!win->parent || win->parent == get_desktop_window())
    {
        *surface = &dummy_surface;  /* provide a default surface for top-level windows */
        window_surface_add_ref( *surface );
    }
    else
    {
        *surface = NULL; /* use parent surface for child windows */
        TRACE( "using parent window surface\n" );
    }

    release_win_ptr( win );
    return TRUE;
}

static struct window_surface *get_window_surface( HWND hwnd, UINT swp_flags, BOOL create_layered,
                                                  struct window_rects *rects, RECT *surface_rect )
{
    BOOL shaped, needs_surface, create_opaque, is_layered, is_child;
    HWND parent = NtUserGetAncestor( hwnd, GA_PARENT );
    struct window_surface *new_surface;
    struct window_rects monitor_rects;
    UINT raw_dpi, style, ex_style;
    RECT dummy;
    HRGN shape;

    style = NtUserGetWindowLongW( hwnd, GWL_STYLE );
    ex_style = NtUserGetWindowLongW( hwnd, GWL_EXSTYLE );
    is_child = parent && parent != NtUserGetDesktopWindow();

    if (is_child) get_win_monitor_dpi( parent, &raw_dpi );
    else monitor_dpi_from_rect( rects->window, get_thread_dpi(), &raw_dpi );

    if (get_window_region( hwnd, FALSE, &shape, &dummy )) shaped = FALSE;
    else if ((shaped = !!shape)) NtGdiDeleteObjectApp( shape );

    rects->visible = rects->window;
    if (is_child) monitor_rects = map_dpi_window_rects( *rects, get_thread_dpi(), raw_dpi );
    else monitor_rects = map_window_rects_virt_to_raw( *rects, get_thread_dpi() );

    if (!user_driver->pWindowPosChanging( hwnd, swp_flags, shaped, &monitor_rects )) needs_surface = FALSE;
    else if (is_child) needs_surface = FALSE;
    else if (swp_flags & SWP_HIDEWINDOW) needs_surface = FALSE;
    else if (swp_flags & SWP_SHOWWINDOW) needs_surface = TRUE;
    else needs_surface = !!(style & WS_VISIBLE);

    if (!is_child) rects->visible = get_visible_rect( hwnd, shaped, style, ex_style, rects );
    if (!get_surface_rect( &rects->visible, surface_rect )) needs_surface = FALSE;
    if (!get_default_window_surface( hwnd, surface_rect, &new_surface )) return NULL;

    is_layered = new_surface && new_surface->alpha_mask;
    create_opaque = !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED);

    if ((create_opaque && is_layered) || (create_layered && !is_layered))
    {
        if (new_surface) window_surface_release( new_surface );
        window_surface_add_ref( (new_surface = &dummy_surface) );
    }
    else if (!create_opaque && is_layered) create_layered = TRUE;

    if (IsRectEmpty( surface_rect )) needs_surface = FALSE;
    else if (create_layered || is_layered) needs_surface = TRUE;

    if (needs_surface)
        create_window_surface( hwnd, create_layered, surface_rect, raw_dpi, &new_surface );
    else if (new_surface && new_surface != &dummy_surface)
    {
        window_surface_release( new_surface );
        window_surface_add_ref( (new_surface = &dummy_surface) );
    }

    if (new_surface && !is_layered)
    {
        DWORD lwa_flags = 0, alpha_bits = -1;
        COLORREF key;
        BYTE alpha;

        if (!NtUserGetLayeredWindowAttributes( hwnd, &key, &alpha, &lwa_flags )) lwa_flags = 0;
        if (lwa_flags & LWA_ALPHA) alpha_bits = alpha << 24;
        if (!(lwa_flags & LWA_COLORKEY)) key = CLR_INVALID;
        window_surface_set_layered( new_surface, key, alpha_bits, 0 );
    }

    return new_surface;
}

static void update_children_window_state( HWND hwnd )
{
    HWND *children;
    int i;

    if (!(children = list_window_children( hwnd ))) return;

    for (i = 0; children[i]; i++)
    {
        if (!window_has_client_surface( children[i] )) continue;
        update_window_state( children[i] );
    }

    free( children );
}

/***********************************************************************
 *           apply_window_pos
 *
 * Backend implementation of SetWindowPos.
 */
static BOOL apply_window_pos( HWND hwnd, HWND insert_after, UINT swp_flags, struct window_surface *new_surface,
                              const struct window_rects *new_rects, const RECT *valid_rects )
{
    struct window_rects monitor_rects;
    WND *win;
    HWND owner_hint, surface_win = 0, parent = NtUserGetAncestor( hwnd, GA_PARENT );
    BOOL ret, is_fullscreen, is_layered, is_child;
    struct window_rects old_rects;
    RECT extra_rects[3];
    struct window_surface *old_surface;
    UINT raw_dpi, monitor_dpi;

    is_layered = new_surface && new_surface->alpha_mask;
    is_fullscreen = is_window_rect_full_screen( &new_rects->visible, get_thread_dpi() );
    is_child = parent && parent != NtUserGetDesktopWindow();

    if (is_child) monitor_dpi = get_win_monitor_dpi( parent, &raw_dpi );
    else monitor_dpi = monitor_dpi_from_rect( new_rects->window, get_thread_dpi(), &raw_dpi );

    get_window_rects( hwnd, COORDS_PARENT, &old_rects, get_thread_dpi() );
    if (IsRectEmpty( &valid_rects[0] ) || is_layered) valid_rects = NULL;

    if (!(win = get_win_ptr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return FALSE;
    old_surface = win->surface;
    if (old_surface != new_surface) swp_flags |= SWP_FRAMECHANGED;  /* force refreshing non-client area */

    if (new_surface == &dummy_surface) swp_flags |= SWP_NOREDRAW;
    else if (old_surface == &dummy_surface)
    {
        swp_flags |= SWP_NOCOPYBITS;
        valid_rects = NULL;
    }

    if (is_child) monitor_rects = map_dpi_window_rects( *new_rects, get_thread_dpi(), raw_dpi );
    else monitor_rects = map_window_rects_virt_to_raw( *new_rects, get_thread_dpi() );

    SERVER_START_REQ( set_window_pos )
    {
        req->handle        = wine_server_user_handle( hwnd );
        req->previous      = wine_server_user_handle( insert_after );
        req->swp_flags     = swp_flags;
        req->monitor_dpi   = monitor_dpi;
        req->window        = wine_server_rectangle( new_rects->window );
        req->client        = wine_server_rectangle( new_rects->client );
        if (!EqualRect( &new_rects->window, &new_rects->visible ) || new_surface || valid_rects)
        {
            extra_rects[0] = extra_rects[1] = new_rects->visible;
            if (new_surface)
            {
                extra_rects[1] = is_layered ? dummy_surface.rect : new_surface->rect;
                OffsetRect( &extra_rects[1], new_rects->visible.left, new_rects->visible.top );
            }
            if (valid_rects) extra_rects[2] = valid_rects[0];
            else SetRectEmpty( &extra_rects[2] );
            wine_server_add_data( req, extra_rects, sizeof(extra_rects) );
        }
        if (new_surface) req->paint_flags |= SET_WINPOS_PAINT_SURFACE;
        if (is_layered) req->paint_flags |= SET_WINPOS_LAYERED_WINDOW;
        if (win->pixel_format || win->internal_pixel_format)
            req->paint_flags |= SET_WINPOS_PIXEL_FORMAT;

        if ((ret = !wine_server_call( req )))
        {
            win->dwStyle      = reply->new_style;
            win->dwExStyle    = reply->new_ex_style;
            win->rects        = *new_rects;
            if ((win->surface = new_surface)) window_surface_add_ref( win->surface );
            surface_win       = wine_server_ptr_handle( reply->surface_win );
            if (get_window_long( win->parent, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
            {
                RECT client = {0};
                get_client_rect_rel( win->parent, COORDS_CLIENT, &client, get_thread_dpi() );
                mirror_rect( &client, &win->rects.window );
                mirror_rect( &client, &win->rects.client );
                mirror_rect( &client, &win->rects.visible );
            }
            /* if an RTL window is resized the children have moved */
            if (win->dwExStyle & WS_EX_LAYOUTRTL &&
                new_rects->client.right - new_rects->client.left != old_rects.client.right - old_rects.client.left)
                win->flags |= WIN_CHILDREN_MOVED;
        }
    }
    SERVER_END_REQ;

    if (ret)
    {
        update_surface_region( surface_win );
        if (((swp_flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) ||
            (swp_flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW | SWP_STATECHANGED | SWP_FRAMECHANGED)))
            invalidate_dce( win, &old_rects.window );
    }

    release_win_ptr( win );

    if (ret)
    {
        TRACE( "win %p surface %p -> %p\n", hwnd, old_surface, new_surface );
        register_window_surface( old_surface, new_surface );
        if (old_surface)
        {
            if (valid_rects)
            {
                RECT rects[2] = {valid_rects[0], valid_rects[1]};
                valid_rects = rects;

                if (old_surface != new_surface)
                    move_window_bits_surface( hwnd, &new_rects->window, old_surface, &old_rects.visible, valid_rects );
                else
                {
                    OffsetRect( &rects[1], new_rects->visible.left - old_rects.visible.left, new_rects->visible.top - old_rects.visible.top );
                    move_window_bits( hwnd, new_rects, valid_rects );
                }
            }
            window_surface_release( old_surface );
        }
        else if (valid_rects)
        {
            RECT rects[2] = {valid_rects[0], valid_rects[1]};
            int x_offset = old_rects.visible.left - new_rects->visible.left;
            int y_offset = old_rects.visible.top - new_rects->visible.top;
            valid_rects = rects;

            /* if all that happened is that the whole window moved, copy everything */
            if (!(swp_flags & SWP_FRAMECHANGED) &&
                old_rects.visible.right  - new_rects->visible.right  == x_offset &&
                old_rects.visible.bottom - new_rects->visible.bottom == y_offset &&
                old_rects.client.left    - new_rects->client.left   == x_offset &&
                old_rects.client.right   - new_rects->client.right  == x_offset &&
                old_rects.client.top     - new_rects->client.top    == y_offset &&
                old_rects.client.bottom  - new_rects->client.bottom == y_offset &&
                EqualRect( &valid_rects[0], &new_rects->client ))
            {
                rects[0] = new_rects->window;
                rects[1] = old_rects.window;
            }

            if (!surface_win || surface_win == hwnd)
                user_driver->pMoveWindowBits( hwnd, &old_rects, new_rects, valid_rects );
            else
            {
                /* move a child window bits within its parent window surface, the surface itself
                 * didn't move and valid rects are already relative to the surface rect. */
                move_window_bits( hwnd, new_rects, valid_rects );
            }
        }

        owner_hint = NtUserGetWindowRelative(hwnd, GW_OWNER);
        /* fallback to any window that is right below our top left corner */
        if (!owner_hint) owner_hint = NtUserWindowFromPoint(new_rects->window.left - 1, new_rects->window.top - 1);
        if (owner_hint) owner_hint = NtUserGetAncestor(owner_hint, GA_ROOT);

        user_driver->pWindowPosChanged( hwnd, insert_after, owner_hint, swp_flags, is_fullscreen, &monitor_rects,
                                        get_driver_window_surface( new_surface, raw_dpi ) );

        update_children_window_state( hwnd );
    }

    return ret;
}

static BOOL expose_window_surface( HWND hwnd, UINT flags, const RECT *rect, UINT dpi )
{
    struct window_surface *surface;
    struct window_rects rects;
    RECT exposed_rect;
    WND *win;

    if (!(win = get_win_ptr( hwnd )) || win == WND_DESKTOP || win == WND_OTHER_PROCESS) return FALSE;
    if ((surface = win->surface)) window_surface_add_ref( surface );
    rects = win->rects;
    release_win_ptr( win );

    if (rect) exposed_rect = map_dpi_rect( *rect, dpi, get_dpi_for_window( hwnd ) );

    if (!surface || surface == &dummy_surface)
    {
        NtUserRedrawWindow( hwnd, rect ? &exposed_rect : NULL, NULL, flags );
        if (surface) window_surface_release( surface );
        return TRUE;
    }

    window_surface_lock( surface );
    if (!rect) add_bounds_rect( &surface->bounds, &surface->rect );
    else
    {
        OffsetRect( &exposed_rect, rects.client.left - rects.visible.left, rects.client.top - rects.visible.top );
        intersect_rect( &exposed_rect, &exposed_rect, &surface->rect );
        add_bounds_rect( &surface->bounds, &exposed_rect );
    }
    window_surface_unlock( surface );
    if (surface->alpha_mask) window_surface_flush( surface );
    window_surface_release( surface );
    return TRUE;
}


/*******************************************************************
 *           NtUserGetWindowRgnEx (win32u.@)
 */
int WINAPI NtUserGetWindowRgnEx( HWND hwnd, HRGN hrgn, UINT unk )
{
    NTSTATUS status;
    int ret = ERROR;
    HRGN win_rgn;
    RECT visible;

    if ((status = get_window_region( hwnd, FALSE, &win_rgn, &visible )))
    {
        set_ntstatus( status );
        return ERROR;
    }

    if (win_rgn)
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
        UINT raw_dpi;
        HRGN monitor_hrgn;

        if (!redraw) swp_flags |= SWP_NOREDRAW;

        get_win_monitor_dpi( hwnd, &raw_dpi );
        monitor_hrgn = map_dpi_region( hrgn, get_thread_dpi(), raw_dpi );
        user_driver->pSetWindowRgn( hwnd, monitor_hrgn, redraw );
        if (monitor_hrgn) NtGdiDeleteObjectApp( monitor_hrgn );

        NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, swp_flags );
        if (hrgn) NtGdiDeleteObjectApp( hrgn );
    }
    return ret;
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

    TRACE( "(%p,%s,%d,%x)\n", hwnd, debugstr_color(key), alpha, flags );

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
    struct window_rects new_rects;
    struct window_surface *surface;
    RECT surface_rect;
    SIZE offset;
    BOOL ret = FALSE;

    if (flags & ~(ULW_COLORKEY | ULW_ALPHA | ULW_OPAQUE | ULW_EX_NORESIZE) ||
        !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYERED) ||
        NtUserGetLayeredWindowAttributes( hwnd, NULL, NULL, NULL ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    get_window_rects( hwnd, COORDS_PARENT, &new_rects, get_thread_dpi() );

    if (pts_dst)
    {
        offset.cx = pts_dst->x - new_rects.window.left;
        offset.cy = pts_dst->y - new_rects.window.top;
        OffsetRect( &new_rects.client, offset.cx, offset.cy );
        OffsetRect( &new_rects.window, offset.cx, offset.cy );
        OffsetRect( &new_rects.visible, offset.cx, offset.cy );
        swp_flags &= ~SWP_NOMOVE;
    }
    if (size)
    {
        offset.cx = size->cx - (new_rects.window.right - new_rects.window.left);
        offset.cy = size->cy - (new_rects.window.bottom - new_rects.window.top);
        if (size->cx <= 0 || size->cy <= 0)
        {
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        if ((flags & ULW_EX_NORESIZE) && (offset.cx || offset.cy))
        {
            RtlSetLastWin32Error( ERROR_INCORRECT_SIZE );
            return FALSE;
        }
        new_rects.client.right  += offset.cx;
        new_rects.client.bottom += offset.cy;
        new_rects.window.right  += offset.cx;
        new_rects.window.bottom += offset.cy;
        new_rects.visible.right  += offset.cx;
        new_rects.visible.bottom += offset.cy;
        swp_flags &= ~SWP_NOSIZE;
    }

    TRACE( "window %p new_rects %s\n", hwnd, debugstr_window_rects( &new_rects ) );

    surface = get_window_surface( hwnd, swp_flags, TRUE, &new_rects, &surface_rect );
    apply_window_pos( hwnd, 0, swp_flags, surface, &new_rects, NULL );
    if (!surface) return FALSE;

    if (!hdc_src || surface == &dummy_surface)
    {
        user_driver->pUpdateLayeredWindow( hwnd, blend->SourceConstantAlpha, flags );
        ret = TRUE;
    }
    else
    {
        BLENDFUNCTION src_blend = { AC_SRC_OVER, 0, 255, 0 };
        RECT rect = new_rects.window, src_rect;
        HDC hdc = NULL;

        OffsetRect( &rect, -rect.left, -rect.top );
        intersect_rect( &rect, &rect, &surface_rect );

        if (!(hdc = NtGdiCreateCompatibleDC( 0 ))) goto done;
        window_surface_lock( surface );
        NtGdiSelectBitmap( hdc, surface->color_bitmap );

        if (dirty) intersect_rect( &rect, &rect, dirty );
        NtGdiPatBlt( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, BLACKNESS );

        src_rect = rect;
        if (pts_src) OffsetRect( &src_rect, pts_src->x, pts_src->y );
        NtGdiTransformPoints( hdc_src, (POINT *)&src_rect, (POINT *)&src_rect, 2, NtGdiDPtoLP );

        ret = NtGdiAlphaBlend( hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                               hdc_src, src_rect.left, src_rect.top, src_rect.right - src_rect.left, src_rect.bottom - src_rect.top,
                               *(DWORD *)&src_blend, 0 );
        if (ret) add_bounds_rect( &surface->bounds, &rect );

        NtGdiDeleteObjectApp( hdc );
        window_surface_unlock( surface );

        if (!(flags & ULW_COLORKEY)) key = CLR_INVALID;
        window_surface_set_layered( surface, key, -1, 0xff000000 );

        user_driver->pUpdateLayeredWindow( hwnd, blend->SourceConstantAlpha, flags );
        window_surface_flush( surface );
    }

done:
    window_surface_release( surface );
    return ret;
}

/***********************************************************************
 *           list_children_from_point
 *
 * Get the list of children that can contain point from the server.
 * Point is in screen coordinates.
 * Returned list must be freed by caller.
 */
static HWND *list_children_from_point( HWND hwnd, POINT pt, UINT dpi )
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
            req->dpi = dpi;
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
HWND window_from_point( HWND hwnd, POINT pt, INT *hittest )
{
    int i, res;
    HWND ret, *list;
    POINT win_pt;
    UINT dpi, raw_dpi;

    if (!hwnd) hwnd = get_desktop_window();
    if (!(dpi = get_thread_dpi())) dpi = get_win_monitor_dpi( hwnd, &raw_dpi );

    *hittest = HTNOWHERE;

    if (!(list = list_children_from_point( hwnd, pt, dpi ))) return 0;

    /* now determine the hittest */

    for (i = 0; list[i]; i++)
    {
        DWORD style = get_window_long( list[i], GWL_STYLE );

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
        win_pt = map_dpi_point( pt, dpi, get_dpi_for_window( list[i] ));
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
 *           NtUserChildWindowFromPointEx (win32u.@)
 */
HWND WINAPI NtUserChildWindowFromPointEx( HWND parent, LONG x, LONG y, UINT flags )
{
    POINT pt = { .x = x, .y = y }; /* in the client coordinates */
    HWND *list;
    int i;
    RECT rect;
    HWND ret;

    if (!get_client_rect( parent, &rect, get_thread_dpi() )) return 0;
    if (!PtInRect( &rect, pt )) return 0;
    if (!(list = list_window_children( parent ))) return parent;

    for (i = 0; list[i]; i++)
    {
        if (!get_window_rect_rel( list[i], COORDS_PARENT, &rect, get_thread_dpi() )) continue;
        if (!PtInRect( &rect, pt )) continue;
        if (flags & (CWP_SKIPINVISIBLE|CWP_SKIPDISABLED))
        {
            DWORD style = get_window_long( list[i], GWL_STYLE );
            if ((flags & CWP_SKIPINVISIBLE) && !(style & WS_VISIBLE)) continue;
            if ((flags & CWP_SKIPDISABLED) && (style & WS_DISABLED)) continue;
        }
        if (flags & CWP_SKIPTRANSPARENT)
        {
            if (get_window_long( list[i], GWL_EXSTYLE ) & WS_EX_TRANSPARENT) continue;
        }
        break;
    }
    ret = list[i];
    free( list );
    if (!ret) ret = parent;
    return ret;
}

/*******************************************************************
 *           NtUserRealChildWindowFromPoint (win32u.@)
 */
HWND WINAPI NtUserRealChildWindowFromPoint( HWND parent, LONG x, LONG y )
{
    return NtUserChildWindowFromPointEx( parent, x, y, CWP_SKIPTRANSPARENT | CWP_SKIPINVISIBLE );
}

/*******************************************************************
 *           get_maximized_rect
 *
 * Get the area that a maximized window can cover, depending on style.
 */
static BOOL get_maximized_rect( HWND hwnd, RECT *rect )
{
    MONITORINFO mon_info;
    DWORD style;

    mon_info = monitor_info_from_window( hwnd, MONITOR_DEFAULTTOPRIMARY );
    *rect = mon_info.rcMonitor;

    style = get_window_long( hwnd, GWL_STYLE );
    if (style & WS_MAXIMIZEBOX && (style & WS_CAPTION) == WS_CAPTION && !(style & WS_CHILD))
        *rect = mon_info.rcWork;
    return TRUE;
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
static void update_maximized_pos( WND *wnd )
{
    RECT work_rect = { 0 };
    MONITORINFO mon_info;

    if (wnd->parent && wnd->parent != get_desktop_window())
        return;

    if (wnd->dwStyle & WS_MAXIMIZE)
    {
        if (!(wnd->dwStyle & WS_MINIMIZE))
        {
            mon_info = monitor_info_from_window( wnd->handle, MONITOR_DEFAULTTOPRIMARY );
            work_rect = mon_info.rcWork;
        }

        if (wnd->rects.window.left  <= work_rect.left  && wnd->rects.window.top    <= work_rect.top &&
            wnd->rects.window.right >= work_rect.right && wnd->rects.window.bottom >= work_rect.bottom)
            wnd->max_pos.x = wnd->max_pos.y = -1;
    }
    else
        wnd->max_pos.x = wnd->max_pos.y = -1;
}

static BOOL empty_point( POINT pt )
{
    return pt.x == -1 && pt.y == -1;
}

/***********************************************************************
 *           NtUserGetWindowPlacement (win32u.@)
 */
BOOL WINAPI NtUserGetWindowPlacement( HWND hwnd, WINDOWPLACEMENT *placement )
{
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
        win->min_pos.x = win->rects.window.left;
        win->min_pos.y = win->rects.window.top;
    }
    else if (win->dwStyle & WS_MAXIMIZE)
    {
        win->max_pos.x = win->rects.window.left;
        win->max_pos.y = win->rects.window.top;
    }
    else
    {
        win->normal_rect = win->rects.window;
    }
    update_maximized_pos( win );

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

/* make sure the specified rect is visible on screen */
static void make_rect_onscreen( RECT *rect )
{
    MONITORINFO info = monitor_info_from_rect( *rect, get_thread_dpi() );

    /* FIXME: map coordinates from rcWork to rcMonitor */
    if (rect->right <= info.rcWork.left)
    {
        rect->right += info.rcWork.left - rect->left;
        rect->left = info.rcWork.left;
    }
    else if (rect->left >= info.rcWork.right)
    {
        rect->left += info.rcWork.right - rect->right;
        rect->right = info.rcWork.right;
    }
    if (rect->bottom <= info.rcWork.top)
    {
        rect->bottom += info.rcWork.top - rect->top;
        rect->top = info.rcWork.top;
    }
    else if (rect->top >= info.rcWork.bottom)
    {
        rect->top += info.rcWork.bottom - rect->bottom;
        rect->bottom = info.rcWork.bottom;
    }
}

/***********************************************************************
 *           NtUserGetInternalWindowPos (win32u.@)
 */
UINT WINAPI NtUserGetInternalWindowPos( HWND hwnd, RECT *rect, POINT *pt )
{
    WINDOWPLACEMENT placement;

    placement.length = sizeof(placement);
    if (!NtUserGetWindowPlacement( hwnd, &placement )) return 0;
    if (rect) *rect = placement.rcNormalPosition;
    if (pt) *pt = placement.ptMinPosition;
    return placement.showCmd;
}

/* make sure the specified point is visible on screen */
static void make_point_onscreen( POINT *pt )
{
    RECT rect;

    SetRect( &rect, pt->x, pt->y, pt->x + 1, pt->y + 1 );
    make_rect_onscreen( &rect );
    pt->x = rect.left;
    pt->y = rect.top;
}

static BOOL set_window_placement( HWND hwnd, const WINDOWPLACEMENT *wndpl, UINT flags )
{
    WND *win = get_win_ptr( hwnd );
    WINDOWPLACEMENT wp = *wndpl;
    DWORD style;

    if (flags & PLACE_MIN) make_point_onscreen( &wp.ptMinPosition );
    if (flags & PLACE_MAX) make_point_onscreen( &wp.ptMaxPosition );
    if (flags & PLACE_RECT) make_rect_onscreen( &wp.rcNormalPosition );

    TRACE( "%p: setting min %d,%d max %d,%d normal %s flags %x adjusted to min %d,%d max %d,%d normal %s\n",
           hwnd, wndpl->ptMinPosition.x, wndpl->ptMinPosition.y,
           wndpl->ptMaxPosition.x, wndpl->ptMaxPosition.y,
           wine_dbgstr_rect(&wndpl->rcNormalPosition), flags,
           wp.ptMinPosition.x, wp.ptMinPosition.y,
           wp.ptMaxPosition.x, wp.ptMaxPosition.y,
           wine_dbgstr_rect(&wp.rcNormalPosition) );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;

    if (flags & PLACE_MIN) win->min_pos = point_thread_to_win_dpi( hwnd, wp.ptMinPosition );
    if (flags & PLACE_MAX)
    {
        win->max_pos = point_thread_to_win_dpi( hwnd, wp.ptMaxPosition );
        update_maximized_pos( win );
    }
    if (flags & PLACE_RECT) win->normal_rect = rect_thread_to_win_dpi( hwnd, wp.rcNormalPosition );

    style = win->dwStyle;

    release_win_ptr( win );

    if (style & WS_MINIMIZE)
    {
        if (flags & PLACE_MIN)
        {
            NtUserSetWindowPos( hwnd, 0, wp.ptMinPosition.x, wp.ptMinPosition.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    else if (style & WS_MAXIMIZE)
    {
        if (flags & PLACE_MAX)
            NtUserSetWindowPos( hwnd, 0, wp.ptMaxPosition.x, wp.ptMaxPosition.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
    }
    else if (flags & PLACE_RECT)
        NtUserSetWindowPos( hwnd, 0, wp.rcNormalPosition.left, wp.rcNormalPosition.top,
                            wp.rcNormalPosition.right - wp.rcNormalPosition.left,
                            wp.rcNormalPosition.bottom - wp.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE );

    NtUserShowWindow( hwnd, wndpl->showCmd );

    if (is_iconic( hwnd ))
    {
        if (wndpl->flags & WPF_RESTORETOMAXIMIZED)
            win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );
    }
    return TRUE;
}

/***********************************************************************
 *           NtUserSetWindowPlacement (win32u.@)
 */
BOOL WINAPI NtUserSetWindowPlacement( HWND hwnd, const WINDOWPLACEMENT *wpl )
{
    UINT flags = PLACE_MAX | PLACE_RECT;
    if (!wpl) return FALSE;
    if (wpl->length != sizeof(*wpl))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (wpl->flags & WPF_SETMINPOSITION) flags |= PLACE_MIN;
    return set_window_placement( hwnd, wpl, flags );
}

/*****************************************************************************
 *           NtUserBuildHwndList (win32u.@)
 */
NTSTATUS WINAPI NtUserBuildHwndList( HDESK desktop, HWND hwnd, BOOL children, BOOL non_immersive,
                                     ULONG thread_id, ULONG count, HWND *buffer, ULONG *size )
{
    user_handle_t *list = (user_handle_t *)buffer;
    int i;
    NTSTATUS status;

    SERVER_START_REQ( get_window_list )
    {
        req->desktop  = wine_server_obj_handle( desktop );
        req->handle   = wine_server_user_handle( hwnd );
        req->tid      = thread_id;
        req->children = children;
        if (count) wine_server_set_reply( req, list, (count - 1) * sizeof(*list) );
        status = wine_server_call( req );
        if (!status || status == STATUS_BUFFER_TOO_SMALL) *size = reply->count + 1;
    }
    SERVER_END_REQ;

    if (status) return status;

    /* start from the end since HWND is potentially larger than user_handle_t */
    for (i = *size - 2; i >= 0; i--)
        buffer[i] = wine_server_ptr_handle( list[i] );
    buffer[*size - 1] = HWND_BOTTOM;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           NtUserFindWindowEx (USER32.@)
 */
HWND WINAPI NtUserFindWindowEx( HWND parent, HWND child, UNICODE_STRING *class, UNICODE_STRING *title,
                                ULONG unk )
{
    user_handle_t *list;
    HWND retvalue = 0;
    int i = 0, size = 128, title_len;
    ATOM atom = class ? get_int_atom_value( class ) : 0;
    NTSTATUS status;

    /* empty class is not the same as NULL class */
    if (!atom && class && !class->Length) return 0;

    if (parent == HWND_MESSAGE) parent = get_hwnd_message_parent();

    for (;;)
    {
        if (!(list = malloc( size * sizeof(*list) ))) return 0;

        SERVER_START_REQ( get_class_windows )
        {
            req->parent = wine_server_user_handle( parent );
            req->child  = wine_server_user_handle( child );
            req->atom   = atom;
            if (!atom && class) wine_server_add_data( req, class->Buffer, class->Length );
            wine_server_set_reply( req, list, size * sizeof(user_handle_t) );
            status = wine_server_call( req );
            size = reply->count;
        }
        SERVER_END_REQ;

        if (!status && size) break;
        free( list );
        if (status != STATUS_BUFFER_TOO_SMALL) return 0;
    }

    if (title)
    {
        int len = title->Length / sizeof(WCHAR) + 1;  /* one extra char to check for chars beyond the end */
        WCHAR *buffer = malloc( (len + 1) * sizeof(WCHAR) );

        if (!buffer) goto done;
        while (i < size)
        {
            title_len = NtUserInternalGetWindowText( wine_server_ptr_handle(list[i]), buffer, len + 1 );
            if (title_len * sizeof(WCHAR) == title->Length &&
                (!title_len || !wcsnicmp( buffer, title->Buffer, title_len )))
                break;
            i++;
        }
        free( buffer );
    }

    if (i < size) retvalue = wine_server_ptr_handle(list[i]);

 done:
    free( list );
    return retvalue;
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
 *         get_windows_offset
 *
 * Calculate the offset between the origin of the two windows. Used
 * to implement MapWindowPoints.
 */
static BOOL get_windows_offset( HWND hwnd_from, HWND hwnd_to, UINT dpi, BOOL *mirrored, POINT *ret_offset )
{
    WND *win;
    POINT offset;
    BOOL mirror_from, mirror_to, ret;
    HWND hwnd;

    offset.x = offset.y = 0;
    *mirrored = mirror_from = mirror_to = FALSE;

    /* Translate source window origin to screen coords */
    if (hwnd_from)
    {
        if (!(win = get_win_ptr( hwnd_from )))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return FALSE;
        }
        if (win == WND_OTHER_PROCESS) goto other_process;
        if (win != WND_DESKTOP)
        {
            UINT raw_dpi, dpi_from = dpi ? dpi : get_win_monitor_dpi( hwnd_from, &raw_dpi );
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_from = TRUE;
                offset.x += win->rects.client.right - win->rects.client.left;
            }
            while (win->parent)
            {
                offset.x += win->rects.client.left;
                offset.y += win->rects.client.top;
                hwnd = win->parent;
                release_win_ptr( win );
                if (!(win = get_win_ptr( hwnd ))) break;
                if (win == WND_OTHER_PROCESS) goto other_process;
                if (win == WND_DESKTOP) break;
                if (win->flags & WIN_CHILDREN_MOVED)
                {
                    release_win_ptr( win );
                    goto other_process;
                }
            }
            if (win && win != WND_DESKTOP) release_win_ptr( win );
            offset = map_dpi_point( offset, get_dpi_for_window( hwnd_from ), dpi_from );
        }
    }

    /* Translate origin to destination window coords */
    if (hwnd_to)
    {
        if (!(win = get_win_ptr( hwnd_to )))
        {
            RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
            return FALSE;
        }
        if (win == WND_OTHER_PROCESS) goto other_process;
        if (win != WND_DESKTOP)
        {
            UINT raw_dpi, dpi_to = dpi ? dpi : get_win_monitor_dpi( hwnd_to, &raw_dpi );
            POINT pt = { 0, 0 };
            if (win->dwExStyle & WS_EX_LAYOUTRTL)
            {
                mirror_to = TRUE;
                pt.x += win->rects.client.right - win->rects.client.left;
            }
            while (win->parent)
            {
                pt.x += win->rects.client.left;
                pt.y += win->rects.client.top;
                hwnd = win->parent;
                release_win_ptr( win );
                if (!(win = get_win_ptr( hwnd ))) break;
                if (win == WND_OTHER_PROCESS) goto other_process;
                if (win == WND_DESKTOP) break;
                if (win->flags & WIN_CHILDREN_MOVED)
                {
                    release_win_ptr( win );
                    goto other_process;
                }
            }
            if (win && win != WND_DESKTOP) release_win_ptr( win );
            pt = map_dpi_point( pt, get_dpi_for_window( hwnd_to ), dpi_to );
            offset.x -= pt.x;
            offset.y -= pt.y;
        }
    }

    *mirrored = mirror_from ^ mirror_to;
    if (mirror_from) offset.x = -offset.x;
    *ret_offset = offset;
    return TRUE;

other_process:  /* one of the parents may belong to another process, do it the hard way */
    SERVER_START_REQ( get_windows_offset )
    {
        req->from = wine_server_user_handle( hwnd_from );
        req->to   = wine_server_user_handle( hwnd_to );
        req->dpi  = dpi;
        if ((ret = !wine_server_call_err( req )))
        {
            ret_offset->x = reply->x;
            ret_offset->y = reply->y;
            *mirrored = reply->mirror;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/* see ClientToScreen */
BOOL client_to_screen( HWND hwnd, POINT *pt )
{
    POINT offset;
    BOOL mirrored;

    if (!hwnd)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    if (!get_windows_offset( hwnd, 0, get_thread_dpi(), &mirrored, &offset )) return FALSE;
    pt->x += offset.x;
    pt->y += offset.y;
    if (mirrored) pt->x = -pt->x;
    return TRUE;
}

/* see ScreenToClient */
BOOL screen_to_client( HWND hwnd, POINT *pt )
{
    POINT offset;
    BOOL mirrored;

    if (!hwnd)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    if (!get_windows_offset( 0, hwnd, get_thread_dpi(), &mirrored, &offset )) return FALSE;
    pt->x += offset.x;
    pt->y += offset.y;
    if (mirrored) pt->x = -pt->x;
    return TRUE;
}

/* map coordinates of a window region */
void map_window_region( HWND from, HWND to, HRGN hrgn )
{
    BOOL mirrored;
    POINT offset;
    UINT i, size;
    RGNDATA *data;
    HRGN new_rgn;
    RECT *rect;

    if (!get_windows_offset( from, to, get_thread_dpi(), &mirrored, &offset )) return;

    if (!mirrored)
    {
        NtGdiOffsetRgn( hrgn, offset.x, offset.y );
        return;
    }
    if (!(size = NtGdiGetRegionData( hrgn, 0, NULL ))) return;
    if (!(data = malloc( size ))) return;
    NtGdiGetRegionData( hrgn, size, data );
    rect = (RECT *)data->Buffer;
    for (i = 0; i < data->rdh.nCount; i++)
    {
        int tmp = -(rect[i].left + offset.x);
        rect[i].left    = -(rect[i].right + offset.x);
        rect[i].right   = tmp;
        rect[i].top    += offset.y;
        rect[i].bottom += offset.y;
    }
    if ((new_rgn = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data )))
    {
        NtGdiCombineRgn( hrgn, new_rgn, 0, RGN_COPY );
        NtGdiDeleteObjectApp( new_rgn );
    }
    free( data );
}

/* see MapWindowPoints */
int map_window_points( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count, UINT dpi )
{
    BOOL mirrored;
    POINT offset;
    UINT i;

    if (!get_windows_offset( hwnd_from, hwnd_to, dpi, &mirrored, &offset )) return 0;

    for (i = 0; i < count; i++)
    {
        points[i].x += offset.x;
        points[i].y += offset.y;
        if (mirrored) points[i].x = -points[i].x;
    }
    if (mirrored && count == 2)  /* special case for rectangle */
    {
        int tmp = points[0].x;
        points[0].x = points[1].x;
        points[1].x = tmp;
    }
    return MAKELONG( LOWORD(offset.x), LOWORD(offset.y) );
}

/***********************************************************************
 *           dump_winpos_flags
 */
static void dump_winpos_flags( UINT flags )
{
    static const UINT dumped_flags = (SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW |
                                      SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW |
                                      SWP_HIDEWINDOW | SWP_NOCOPYBITS | SWP_NOOWNERZORDER |
                                      SWP_NOSENDCHANGING | SWP_DEFERERASE | SWP_ASYNCWINDOWPOS |
                                      SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_STATECHANGED);
    TRACE( "flags:" );
    if(flags & SWP_NOSIZE) TRACE( " SWP_NOSIZE" );
    if(flags & SWP_NOMOVE) TRACE( " SWP_NOMOVE" );
    if(flags & SWP_NOZORDER) TRACE( " SWP_NOZORDER" );
    if(flags & SWP_NOREDRAW) TRACE( " SWP_NOREDRAW" );
    if(flags & SWP_NOACTIVATE) TRACE( " SWP_NOACTIVATE" );
    if(flags & SWP_FRAMECHANGED) TRACE( " SWP_FRAMECHANGED" );
    if(flags & SWP_SHOWWINDOW) TRACE( " SWP_SHOWWINDOW" );
    if(flags & SWP_HIDEWINDOW) TRACE( " SWP_HIDEWINDOW" );
    if(flags & SWP_NOCOPYBITS) TRACE( " SWP_NOCOPYBITS" );
    if(flags & SWP_NOOWNERZORDER) TRACE( " SWP_NOOWNERZORDER" );
    if(flags & SWP_NOSENDCHANGING) TRACE( " SWP_NOSENDCHANGING" );
    if(flags & SWP_DEFERERASE) TRACE( " SWP_DEFERERASE" );
    if(flags & SWP_ASYNCWINDOWPOS) TRACE( " SWP_ASYNCWINDOWPOS" );
    if(flags & SWP_NOCLIENTSIZE) TRACE( " SWP_NOCLIENTSIZE" );
    if(flags & SWP_NOCLIENTMOVE) TRACE( " SWP_NOCLIENTMOVE" );
    if(flags & SWP_STATECHANGED) TRACE( " SWP_STATECHANGED" );

    if(flags & ~dumped_flags) TRACE( " %08x", flags & ~dumped_flags );
    TRACE( "\n" );
}

/***********************************************************************
 *           map_dpi_winpos
 */
static void map_dpi_winpos( WINDOWPOS *winpos )
{
    RECT rect = {winpos->x, winpos->y, winpos->x + winpos->cx, winpos->y + winpos->cy};
    UINT raw_dpi, dpi_from = get_thread_dpi(), dpi_to = get_dpi_for_window( winpos->hwnd );

    if (!dpi_from) dpi_from = get_win_monitor_dpi( winpos->hwnd, &raw_dpi );
    rect = map_dpi_rect( rect, dpi_from, dpi_to );
    winpos->x = rect.left;
    winpos->y = rect.top;
    winpos->cx = rect.right - rect.left;
    winpos->cy = rect.bottom - rect.top;
}

/***********************************************************************
 *           calc_winpos
 */
static BOOL calc_winpos( WINDOWPOS *winpos, struct window_rects *old_rects, struct window_rects *new_rects )
{
    WND *win;

    /* Send WM_WINDOWPOSCHANGING message */
    if (!(winpos->flags & SWP_NOSENDCHANGING)
           && !((winpos->flags & SWP_AGG_NOCLIENTCHANGE) && (winpos->flags & SWP_SHOWWINDOW)))
        send_message( winpos->hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)winpos );

    if (!(win = get_win_ptr( winpos->hwnd )) ||
        win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;

    /* Calculate new position and size */
    get_window_rects( winpos->hwnd, COORDS_PARENT, old_rects, get_thread_dpi() );
    old_rects->visible = win->rects.visible;
    *new_rects = *old_rects;

    if (!(winpos->flags & SWP_NOSIZE))
    {
        if (win->dwStyle & WS_MINIMIZE)
        {
            new_rects->window.right  = new_rects->window.left + get_system_metrics( SM_CXMINIMIZED );
            new_rects->window.bottom = new_rects->window.top + get_system_metrics( SM_CYMINIMIZED );
        }
        else
        {
            new_rects->window.right  = new_rects->window.left + winpos->cx;
            new_rects->window.bottom = new_rects->window.top + winpos->cy;
        }
    }

    if (!(winpos->flags & SWP_NOMOVE))
    {
        /* If the window is toplevel minimized off-screen, force keep it there */
        if ((win->dwStyle & WS_MINIMIZE) &&
             win->rects.window.left <= -32000 && win->rects.window.top <= -32000 &&
            (!win->parent || win->parent == get_desktop_window()))
        {
            winpos->x = -32000;
            winpos->y = -32000;
        }
        new_rects->window.left    = winpos->x;
        new_rects->window.top     = winpos->y;
        new_rects->window.right  += winpos->x - old_rects->window.left;
        new_rects->window.bottom += winpos->y - old_rects->window.top;

        OffsetRect( &new_rects->client, winpos->x - old_rects->window.left,
                    winpos->y - old_rects->window.top );
        OffsetRect( &new_rects->visible, winpos->x - old_rects->window.left,
                    winpos->y - old_rects->window.top );
    }
    winpos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

    TRACE( "hwnd %p, after %p, swp %d,%d %dx%d flags %08x style %08x old_rects %s new_rects %s\n",
           winpos->hwnd, winpos->hwndInsertAfter, winpos->x, winpos->y, winpos->cx, winpos->cy, winpos->flags, win->dwStyle,
           debugstr_window_rects( old_rects ), debugstr_window_rects( new_rects ) );

    release_win_ptr( win );
    return TRUE;
}

/***********************************************************************
 *           get_valid_rects
 *
 * Compute the valid rects from the old and new client rect and WVR_* flags.
 * Helper for WM_NCCALCSIZE handling.
 */
static inline void get_valid_rects( const RECT *old_client, const RECT *new_client, UINT flags,
                                    RECT *valid )
{
    int cx, cy;

    if (flags & WVR_REDRAW)
    {
        SetRectEmpty( &valid[0] );
        SetRectEmpty( &valid[1] );
        return;
    }

    if (flags & WVR_VALIDRECTS)
    {
        if (!intersect_rect( &valid[0], &valid[0], new_client ) ||
            !intersect_rect( &valid[1], &valid[1], old_client ))
        {
            SetRectEmpty( &valid[0] );
            SetRectEmpty( &valid[1] );
            return;
        }
        flags = WVR_ALIGNLEFT | WVR_ALIGNTOP;
    }
    else
    {
        valid[0] = *new_client;
        valid[1] = *old_client;
    }

    /* make sure the rectangles have the same size */
    cx = min( valid[0].right - valid[0].left, valid[1].right - valid[1].left );
    cy = min( valid[0].bottom - valid[0].top, valid[1].bottom - valid[1].top );

    if (flags & WVR_ALIGNBOTTOM)
    {
        valid[0].top = valid[0].bottom - cy;
        valid[1].top = valid[1].bottom - cy;
    }
    else
    {
        valid[0].bottom = valid[0].top + cy;
        valid[1].bottom = valid[1].top + cy;
    }
    if (flags & WVR_ALIGNRIGHT)
    {
        valid[0].left = valid[0].right - cx;
        valid[1].left = valid[1].right - cx;
    }
    else
    {
        valid[0].right = valid[0].left + cx;
        valid[1].right = valid[1].left + cx;
    }
}

static UINT calc_ncsize( WINDOWPOS *winpos, const struct window_rects *old_rects,
                         struct window_rects *new_rects, RECT *valid_rects,
                         int parent_x, int parent_y )
{
    UINT wvr_flags = 0;

    /* Send WM_NCCALCSIZE message to get new client area */
    if ((winpos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
    {
        NCCALCSIZE_PARAMS params;
        WINDOWPOS winposCopy;
        UINT class_style;

        params.rgrc[0] = new_rects->window;
        params.rgrc[1] = old_rects->window;
        params.rgrc[2] = old_rects->client;
        params.lppos = &winposCopy;
        winposCopy = *winpos;

        if (winpos->flags & SWP_NOMOVE)
        {
            winposCopy.x = old_rects->window.left;
            winposCopy.y = old_rects->window.top;
        }

        if (winpos->flags & SWP_NOSIZE)
        {
            winposCopy.cx = old_rects->window.right - old_rects->window.left;
            winposCopy.cy = old_rects->window.bottom - old_rects->window.top;
        }

        class_style = get_class_long( winpos->hwnd, GCL_STYLE, FALSE );
        if (class_style & CS_VREDRAW) wvr_flags |= WVR_VREDRAW;
        if (class_style & CS_HREDRAW) wvr_flags |= WVR_HREDRAW;

        wvr_flags |= send_message( winpos->hwnd, WM_NCCALCSIZE, TRUE, (LPARAM)&params );

        new_rects->client = params.rgrc[0];

        TRACE( "hwnd %p old win %s old client %s new win %s new client %s\n", winpos->hwnd,
               wine_dbgstr_rect(&old_rects->window), wine_dbgstr_rect(&old_rects->client),
               wine_dbgstr_rect(&new_rects->window), wine_dbgstr_rect(&new_rects->client) );

        if (new_rects->client.left != old_rects->client.left - parent_x ||
            new_rects->client.top != old_rects->client.top - parent_y)
            winpos->flags &= ~SWP_NOCLIENTMOVE;

        if ((new_rects->client.right - new_rects->client.left !=
             old_rects->client.right - old_rects->client.left))
            winpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvr_flags &= ~WVR_HREDRAW;

        if (new_rects->client.bottom - new_rects->client.top !=
            old_rects->client.bottom - old_rects->client.top)
            winpos->flags &= ~SWP_NOCLIENTSIZE;
        else
            wvr_flags &= ~WVR_VREDRAW;

        valid_rects[0] = params.rgrc[1];
        valid_rects[1] = params.rgrc[2];
    }
    else
    {
        if (!(winpos->flags & SWP_NOMOVE) &&
            (new_rects->client.left != old_rects->client.left - parent_x ||
             new_rects->client.top != old_rects->client.top - parent_y))
            winpos->flags &= ~SWP_NOCLIENTMOVE;
    }

    if (winpos->flags & (SWP_NOCOPYBITS | SWP_NOREDRAW | SWP_SHOWWINDOW | SWP_HIDEWINDOW))
    {
        SetRectEmpty( &valid_rects[0] );
        SetRectEmpty( &valid_rects[1] );
    }
    else get_valid_rects( &old_rects->client, &new_rects->client, wvr_flags, valid_rects );

    return wvr_flags;
}

/* fix redundant flags and values in the WINDOWPOS structure */
static BOOL fixup_swp_flags( WINDOWPOS *winpos, const RECT *old_window_rect, int parent_x, int parent_y )
{
    HWND parent;
    WND *win = get_win_ptr( winpos->hwnd );
    BOOL ret = TRUE;

    if (!win || win == WND_OTHER_PROCESS)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    winpos->hwnd = win->handle;  /* make it a full handle */

    /* Finally make sure that all coordinates are valid */
    if (winpos->x < -32768) winpos->x = -32768;
    else if (winpos->x > 32767) winpos->x = 32767;
    if (winpos->y < -32768) winpos->y = -32768;
    else if (winpos->y > 32767) winpos->y = 32767;

    if (winpos->cx < 0) winpos->cx = 0;
    else if (winpos->cx > 32767) winpos->cx = 32767;
    if (winpos->cy < 0) winpos->cy = 0;
    else if (winpos->cy > 32767) winpos->cy = 32767;

    parent = NtUserGetAncestor( winpos->hwnd, GA_PARENT );
    if (!is_window_visible( parent )) winpos->flags |= SWP_NOREDRAW;

    if (win->dwStyle & WS_VISIBLE) winpos->flags &= ~SWP_SHOWWINDOW;
    else
    {
        winpos->flags &= ~SWP_HIDEWINDOW;
        if (!(winpos->flags & SWP_SHOWWINDOW)) winpos->flags |= SWP_NOREDRAW;
    }

    if ((old_window_rect->right - old_window_rect->left == winpos->cx) &&
        (old_window_rect->bottom - old_window_rect->top == winpos->cy))
        winpos->flags |= SWP_NOSIZE;    /* Already the right size */

    if ((old_window_rect->left - parent_x == winpos->x) && (old_window_rect->top - parent_y == winpos->y))
        winpos->flags |= SWP_NOMOVE;    /* Already the right position */

    if ((win->dwStyle & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
        if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW)) && /* Bring to the top when activating */
            (winpos->flags & SWP_NOZORDER ||
             (winpos->hwndInsertAfter != HWND_TOPMOST && winpos->hwndInsertAfter != HWND_NOTOPMOST)))
        {
            winpos->flags &= ~SWP_NOZORDER;
            winpos->hwndInsertAfter = HWND_TOP;
        }
    }

    /* Check hwndInsertAfter */
    if (winpos->flags & SWP_NOZORDER) goto done;

    if (winpos->hwndInsertAfter == HWND_TOP)
    {
        if (get_window_relative( winpos->hwnd, GW_HWNDFIRST ) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_BOTTOM)
    {
        if (!(win->dwExStyle & WS_EX_TOPMOST) &&
            get_window_relative( winpos->hwnd, GW_HWNDLAST ) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_TOPMOST)
    {
        if ((win->dwExStyle & WS_EX_TOPMOST) &&
            get_window_relative( winpos->hwnd, GW_HWNDFIRST ) == winpos->hwnd)
            winpos->flags |= SWP_NOZORDER;
    }
    else if (winpos->hwndInsertAfter == HWND_NOTOPMOST)
    {
        if (!(win->dwExStyle & WS_EX_TOPMOST))
            winpos->flags |= SWP_NOZORDER;
    }
    else
    {
        if ((winpos->hwnd == winpos->hwndInsertAfter) ||
            (winpos->hwnd == get_window_relative( winpos->hwndInsertAfter, GW_HWNDNEXT )))
            winpos->flags |= SWP_NOZORDER;
    }
 done:
    release_win_ptr( win );
    return ret;
}

/***********************************************************************
 *           swp_owner_popups
 *
 * fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 *
 * FIXME: hide/show owned popups when owner visibility changes.
 */
static HWND swp_owner_popups( HWND hwnd, HWND after )
{
    HWND owner, *list = NULL;
    unsigned int i;

    TRACE( "(%p) after = %p\n", hwnd, after );

    if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) return after;

    if ((owner = get_window_relative( hwnd, GW_OWNER )))
    {
        /* make sure this popup stays above the owner */

        if (after != HWND_TOPMOST)
        {
            if (!(list = list_window_children( 0 ))) return after;

            for (i = 0; list[i]; i++)
            {
                BOOL topmost = (get_window_long( list[i], GWL_EXSTYLE ) & WS_EX_TOPMOST) != 0;

                if (list[i] == owner)
                {
                    if (i > 0) after = list[i-1];
                    else after = topmost ? HWND_TOPMOST : HWND_TOP;
                    break;
                }

                if (after == HWND_TOP || after == HWND_NOTOPMOST)
                {
                    if (!topmost) break;
                }
                else if (list[i] == after) break;
            }
        }
    }

    if (after == HWND_BOTTOM) goto done;
    if (!list && !(list = list_window_children( 0 ))) goto done;

    i = 0;
    if (after == HWND_TOP || after == HWND_NOTOPMOST)
    {
        if (after == HWND_NOTOPMOST ||
            !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_TOPMOST))
        {
            /* skip all the topmost windows */
            while (list[i] && (get_window_long( list[i], GWL_EXSTYLE ) & WS_EX_TOPMOST)) i++;
        }
    }
    else if (after != HWND_TOPMOST)
    {
        /* skip windows that are already placed correctly */
        for (i = 0; list[i]; i++)
        {
            if (list[i] == after) break;
            if (list[i] == hwnd) goto done;  /* nothing to do if window is moving backwards in z-order */
        }
    }

    for ( ; list[i]; i++)
    {
        if (list[i] == hwnd) break;
        if (get_window_relative( list[i], GW_OWNER ) != hwnd) continue;
        TRACE( "moving %p owned by %p after %p\n", list[i], hwnd, after );
        NtUserSetWindowPos( list[i], after, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_DEFERERASE );
        after = list[i];
    }

done:
    free( list );
    return after;
}

/* NtUserSetWindowPos implementation */
BOOL set_window_pos( WINDOWPOS *winpos, int parent_x, int parent_y )
{
    RECT valid_rects[2], surface_rect;
    struct window_surface *surface;
    struct window_rects old_rects, new_rects;
    UINT orig_flags, context;
    BOOL ret = FALSE;

    orig_flags = winpos->flags;

    /* First, check z-order arguments.  */
    if (!(winpos->flags & SWP_NOZORDER))
    {
        /* fix sign extension */
        if (winpos->hwndInsertAfter == (HWND)0xffff) winpos->hwndInsertAfter = HWND_TOPMOST;
        else if (winpos->hwndInsertAfter == (HWND)0xfffe) winpos->hwndInsertAfter = HWND_NOTOPMOST;

        if (winpos->hwndInsertAfter == HWND_TOPMOST || winpos->hwndInsertAfter == HWND_NOTOPMOST)
        {
            HWND parent = NtUserGetAncestor( NtUserGetAncestor( winpos->hwnd, GA_ROOT ), GA_PARENT );
            if (parent == get_hwnd_message_parent()) return TRUE;
        }
        else if (winpos->hwndInsertAfter != HWND_TOP && winpos->hwndInsertAfter != HWND_BOTTOM)
        {
            HWND parent = NtUserGetAncestor( winpos->hwnd, GA_PARENT );
            HWND insertafter_parent = NtUserGetAncestor( winpos->hwndInsertAfter, GA_PARENT );

            /* hwndInsertAfter must be a sibling of the window */
            if (!insertafter_parent) return FALSE;
            if (insertafter_parent != parent) return TRUE;
        }
    }

    /* Make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
    if (!(winpos->flags & SWP_NOMOVE))
    {
        if (winpos->x < -32768) winpos->x = -32768;
        else if (winpos->x > 32767) winpos->x = 32767;
        if (winpos->y < -32768) winpos->y = -32768;
        else if (winpos->y > 32767) winpos->y = 32767;
    }
    if (!(winpos->flags & SWP_NOSIZE))
    {
        if (winpos->cx < 0) winpos->cx = 0;
        else if (winpos->cx > 32767) winpos->cx = 32767;
        if (winpos->cy < 0) winpos->cy = 0;
        else if (winpos->cy > 32767) winpos->cy = 32767;
    }

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( winpos->hwnd ));

    if (!calc_winpos( winpos, &old_rects, &new_rects )) goto done;

    /* Fix redundant flags */
    if (!fixup_swp_flags( winpos, &old_rects.window, parent_x, parent_y )) goto done;

    if((winpos->flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) != SWP_NOZORDER)
    {
        if (NtUserGetAncestor( winpos->hwnd, GA_PARENT ) == get_desktop_window())
            winpos->hwndInsertAfter = swp_owner_popups( winpos->hwnd, winpos->hwndInsertAfter );
    }

    /* Common operations */

    calc_ncsize( winpos, &old_rects, &new_rects, valid_rects, parent_x, parent_y );

    surface = get_window_surface( winpos->hwnd, winpos->flags, FALSE, &new_rects, &surface_rect );
    if (!apply_window_pos( winpos->hwnd, winpos->hwndInsertAfter, winpos->flags, surface,
                           &new_rects, valid_rects ))
    {
        if (surface) window_surface_release( surface );
        goto done;
    }
    if (surface) window_surface_release( surface );

    if (winpos->flags & SWP_HIDEWINDOW)
    {
        NtUserNotifyWinEvent( EVENT_OBJECT_HIDE, winpos->hwnd, 0, 0 );

        NtUserHideCaret( winpos->hwnd );
    }
    else if (winpos->flags & SWP_SHOWWINDOW)
    {
        NtUserNotifyWinEvent( EVENT_OBJECT_SHOW, winpos->hwnd, 0, 0 );

        NtUserShowCaret( winpos->hwnd );
    }

    if (!(winpos->flags & (SWP_NOACTIVATE|SWP_HIDEWINDOW)))
    {
        UINT style = get_window_long( winpos->hwnd, GWL_STYLE );
        /* child windows get WM_CHILDACTIVATE message */
        if ((style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
            send_message( winpos->hwnd, WM_CHILDACTIVATE, 0, 0 );
        else if (!(style & WS_MINIMIZE))
            set_foreground_window( winpos->hwnd, FALSE );
    }

    if(!(orig_flags & SWP_DEFERERASE))
    {
        /* erase parent when hiding or resizing child */
        if ((orig_flags & SWP_HIDEWINDOW) ||
         (!(orig_flags & SWP_SHOWWINDOW) &&
          (winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOGEOMETRYCHANGE))
        {
            HWND parent = NtUserGetAncestor( winpos->hwnd, GA_PARENT );
            if (!parent || parent == get_desktop_window()) parent = winpos->hwnd;
            erase_now( parent, 0 );
        }

        /* Give newly shown windows a chance to redraw */
        if(((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
                && !(orig_flags & SWP_AGG_NOCLIENTCHANGE) && (orig_flags & SWP_SHOWWINDOW))
        {
            erase_now(winpos->hwnd, 0);
        }
    }

      /* And last, send the WM_WINDOWPOSCHANGED message */

    TRACE( "\tstatus flags = %04x\n", winpos->flags & SWP_AGG_STATUSFLAGS );

    if (((winpos->flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
            && !((orig_flags & SWP_AGG_NOCLIENTCHANGE) && (orig_flags & SWP_SHOWWINDOW)))
    {
        /* WM_WINDOWPOSCHANGED is sent even if SWP_NOSENDCHANGING is set
           and always contains final window position.
         */
        winpos->x  = new_rects.window.left;
        winpos->y  = new_rects.window.top;
        winpos->cx = new_rects.window.right - new_rects.window.left;
        winpos->cy = new_rects.window.bottom - new_rects.window.top;
        send_message( winpos->hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)winpos );
    }

    if ((winpos->flags & (SWP_NOSIZE|SWP_NOMOVE|SWP_FRAMECHANGED)) != (SWP_NOSIZE|SWP_NOMOVE))
        NtUserNotifyWinEvent( EVENT_OBJECT_LOCATIONCHANGE, winpos->hwnd, OBJID_WINDOW, 0 );

    ret = TRUE;
done:
    set_thread_dpi_awareness_context( context );
    return ret;
}

/*******************************************************************
 *           NtUserSetWindowPos (win32u.@)
 */
BOOL WINAPI NtUserSetWindowPos( HWND hwnd, HWND after, INT x, INT y, INT cx, INT cy, UINT flags )
{
    WINDOWPOS winpos;

    TRACE( "hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n", hwnd, after, x, y, cx, cy, flags );
    if(TRACE_ON(win)) dump_winpos_flags(flags);

    if (is_broadcast( hwnd ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    winpos.hwnd = get_full_window_handle( hwnd );
    winpos.hwndInsertAfter = get_full_window_handle( after );
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    winpos.flags = flags;

    map_dpi_winpos( &winpos );

    if (is_current_thread_window( hwnd ))
        return set_window_pos( &winpos, 0, 0 );

    if (flags & SWP_ASYNCWINDOWPOS)
        return NtUserMessageCall( winpos.hwnd, WM_WINE_SETWINDOWPOS, 0, (LPARAM)&winpos,
                                  0, NtUserSendNotifyMessage, FALSE );
    else
        return send_message( winpos.hwnd, WM_WINE_SETWINDOWPOS, 0, (LPARAM)&winpos );
}

typedef struct
{
    INT        count;
    INT        suggested_count;
    HWND       parent;
    WINDOWPOS *winpos;
} DWP;

/***********************************************************************
 *           NtUserBeginDeferWindowPos (win32u.@)
 */
HDWP WINAPI NtUserBeginDeferWindowPos( INT count )
{
    HDWP handle = 0;
    DWP *dwp;

    TRACE( "%d\n", count );

    if (count < 0)
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    /* Windows allows zero count, in which case it allocates context for 8 moves */
    if (count == 0) count = 8;

    if (!(dwp = malloc( sizeof(DWP) ))) return 0;

    dwp->count  = 0;
    dwp->parent = 0;
    dwp->suggested_count = count;

    if (!(dwp->winpos = malloc( count * sizeof(WINDOWPOS) )) ||
        !(handle = alloc_user_handle( dwp, NTUSER_OBJ_WINPOS )))
    {
        free( dwp->winpos );
        free( dwp );
    }

    TRACE( "returning %p\n", handle );
    return handle;
}

/***********************************************************************
 *           NtUserDeferWindowPosAndBand (win32u.@)
 */
HDWP WINAPI NtUserDeferWindowPosAndBand( HDWP hdwp, HWND hwnd, HWND after,
                                         INT x, INT y, INT cx, INT cy,
                                         UINT flags, UINT unk1, UINT unk2 )
{
    HDWP retvalue = hdwp;
    WINDOWPOS winpos;
    DWP *dwp;
    int i;

    TRACE( "hdwp %p, hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
           hdwp, hwnd, after, x, y, cx, cy, flags );

    winpos.hwnd = get_full_window_handle( hwnd );
    if (is_desktop_window( winpos.hwnd ) || !is_window( winpos.hwnd ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    winpos.hwndInsertAfter = get_full_window_handle( after );
    winpos.flags = flags;
    winpos.x = x;
    winpos.y = y;
    winpos.cx = cx;
    winpos.cy = cy;
    map_dpi_winpos( &winpos );

    if (!(dwp = get_user_handle_ptr( hdwp, NTUSER_OBJ_WINPOS ))) return 0;
    if (dwp == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p\n", hdwp );
        return 0;
    }

    for (i = 0; i < dwp->count; i++)
    {
        if (dwp->winpos[i].hwnd == winpos.hwnd)
        {
              /* Merge with the other changes */
            if (!(flags & SWP_NOZORDER))
            {
                dwp->winpos[i].hwndInsertAfter = winpos.hwndInsertAfter;
            }
            if (!(flags & SWP_NOMOVE))
            {
                dwp->winpos[i].x = winpos.x;
                dwp->winpos[i].y = winpos.y;
            }
            if (!(flags & SWP_NOSIZE))
            {
                dwp->winpos[i].cx = winpos.cx;
                dwp->winpos[i].cy = winpos.cy;
            }
            dwp->winpos[i].flags &= flags | ~(SWP_NOSIZE | SWP_NOMOVE |
                                              SWP_NOZORDER | SWP_NOREDRAW |
                                              SWP_NOACTIVATE | SWP_NOCOPYBITS|
                                              SWP_NOOWNERZORDER);
            dwp->winpos[i].flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                             SWP_FRAMECHANGED);
            goto done;
        }
    }
    if (dwp->count >= dwp->suggested_count)
    {
        WINDOWPOS *newpos = realloc( dwp->winpos, dwp->suggested_count * 2 * sizeof(WINDOWPOS) );
        if (!newpos)
        {
            retvalue = 0;
            goto done;
        }
        dwp->suggested_count *= 2;
        dwp->winpos = newpos;
    }
    dwp->winpos[dwp->count++] = winpos;
done:
    release_user_handle_ptr( dwp );
    return retvalue;
}

/***********************************************************************
 *           NtUserEndDeferWindowPosEx (win32u.@)
 */
BOOL WINAPI NtUserEndDeferWindowPosEx( HDWP hdwp, BOOL async )
{
    WINDOWPOS *winpos;
    DWP *dwp;
    int i;

    TRACE( "%p\n", hdwp );

    if (async) FIXME( "async not supported\n" );

    if (!(dwp = free_user_handle( hdwp, NTUSER_OBJ_WINPOS ))) return FALSE;
    if (dwp == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p\n", hdwp );
        return FALSE;
    }

    for (i = 0, winpos = dwp->winpos; i < dwp->count; i++, winpos++)
    {
        TRACE( "hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
               winpos->hwnd, winpos->hwndInsertAfter, winpos->x, winpos->y,
               winpos->cx, winpos->cy, winpos->flags );

        if (is_current_thread_window( winpos->hwnd ))
            set_window_pos( winpos, 0, 0 );
        else
            send_message( winpos->hwnd, WM_WINE_SETWINDOWPOS, 0, (LPARAM)winpos );
    }
    free( dwp->winpos );
    free( dwp );
    return TRUE;
}

/***********************************************************************
 *           NtUserSetInternalWindowPos (win32u.@)
 */
void WINAPI NtUserSetInternalWindowPos( HWND hwnd, UINT cmd, RECT *rect, POINT *pt )
{
    WINDOWPLACEMENT wndpl;
    UINT flags;

    wndpl.length  = sizeof(wndpl);
    wndpl.showCmd = cmd;
    wndpl.flags = flags = 0;

    if (pt)
    {
        flags |= PLACE_MIN;
        wndpl.flags |= WPF_SETMINPOSITION;
        wndpl.ptMinPosition = *pt;
    }
    if( rect )
    {
        flags |= PLACE_RECT;
        wndpl.rcNormalPosition = *rect;
    }
    set_window_placement( hwnd, &wndpl, flags );
}

/***********************************************************************
 *           win_set_flags
 *
 * Set the flags of a window and return the previous value.
 */
UINT win_set_flags( HWND hwnd, UINT set_mask, UINT clear_mask )
{
    WND *win = get_win_ptr( hwnd );
    UINT ret;

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return 0;
    ret = win->flags;
    win->flags = (ret & ~clear_mask) | set_mask;
    release_win_ptr( win );
    return ret;
}

/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static BOOL can_activate_window( HWND hwnd )
{
    DWORD style;

    if (!hwnd) return FALSE;
    style = get_window_long( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}

/*******************************************************************
 *         activate_other_window
 *
 * Activates window other than hwnd.
 */
static void activate_other_window( HWND hwnd )
{
    HWND hwnd_to, fg;

    if ((get_window_long( hwnd, GWL_STYLE ) & WS_POPUP) &&
        (hwnd_to = get_window_relative( hwnd, GW_OWNER )))
    {
        hwnd_to = NtUserGetAncestor( hwnd_to, GA_ROOT );
        if (can_activate_window( hwnd_to )) goto done;
    }

    hwnd_to = hwnd;
    for (;;)
    {
        if (!(hwnd_to = get_window_relative( hwnd_to, GW_HWNDNEXT ))) break;
        if (can_activate_window( hwnd_to )) goto done;
    }

    hwnd_to = get_window_relative( get_desktop_window(), GW_CHILD );
    for (;;)
    {
        if (hwnd_to == hwnd)
        {
            hwnd_to = 0;
            break;
        }
        if (can_activate_window( hwnd_to )) goto done;
        if (!(hwnd_to = get_window_relative( hwnd_to, GW_HWNDNEXT ))) break;
    }

 done:
    fg = NtUserGetForegroundWindow();
    TRACE( "win = %p fg = %p\n", hwnd_to, fg );
    if (!fg || hwnd == fg)
    {
        if (set_foreground_window( hwnd_to, FALSE )) return;
    }
    if (NtUserSetActiveWindow( hwnd_to )) NtUserSetActiveWindow( 0 );
}

/*******************************************************************
 *           send_parent_notify
 */
static void send_parent_notify( HWND hwnd, UINT msg )
{
    if ((get_window_long( hwnd, GWL_STYLE ) & (WS_CHILD | WS_POPUP)) == WS_CHILD &&
        !(get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY))
    {
        HWND parent = get_parent( hwnd );
        if (parent && parent != get_desktop_window())
            send_message( parent, WM_PARENTNOTIFY,
                          MAKEWPARAM( msg, get_window_long( hwnd, GWLP_ID )), (LPARAM)hwnd );
    }
}

/*******************************************************************
 *           get_min_max_info
 *
 * Get the minimized and maximized information for a window.
 */
MINMAXINFO get_min_max_info( HWND hwnd )
{
    DWORD style = get_window_long( hwnd, GWL_STYLE );
    DWORD exstyle = get_window_long( hwnd, GWL_EXSTYLE );
    UINT context;
    RECT rc_max, rc_primary;
    DWORD adjusted_style;
    MINMAXINFO minmax;
    INT xinc, yinc;
    RECT rc;
    WND *win;

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));

    /* Compute default values */

    get_window_rect( hwnd, &rc, get_thread_dpi() );
    minmax.ptReserved.x = rc.left;
    minmax.ptReserved.y = rc.top;

    if ((style & WS_CAPTION) == WS_CAPTION)
        adjusted_style = style & ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
    else
        adjusted_style = style;

    get_client_rect( NtUserGetAncestor( hwnd, GA_PARENT ), &rc, get_thread_dpi() );
    adjust_window_rect( &rc, adjusted_style, (style & WS_POPUP) && get_menu( hwnd ), exstyle, get_system_dpi() );

    xinc = -rc.left;
    yinc = -rc.top;

    minmax.ptMaxSize.x = rc.right - rc.left;
    minmax.ptMaxSize.y = rc.bottom - rc.top;
    if (style & (WS_DLGFRAME | WS_BORDER))
    {
        minmax.ptMinTrackSize.x = get_system_metrics( SM_CXMINTRACK );
        minmax.ptMinTrackSize.y = get_system_metrics( SM_CYMINTRACK );
    }
    else
    {
        minmax.ptMinTrackSize.x = 2 * xinc;
        minmax.ptMinTrackSize.y = 2 * yinc;
    }
    minmax.ptMaxTrackSize.x = get_system_metrics( SM_CXMAXTRACK );
    minmax.ptMaxTrackSize.y = get_system_metrics( SM_CYMAXTRACK );
    minmax.ptMaxPosition.x = -xinc;
    minmax.ptMaxPosition.y = -yinc;

    if ((win = get_win_ptr( hwnd )) && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        if (!empty_point( win->max_pos )) minmax.ptMaxPosition = win->max_pos;
        release_win_ptr( win );
    }

    send_message( hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&minmax );

    /* if the app didn't change the values, adapt them for the current monitor */

    if (get_maximized_rect( hwnd, &rc_max ))
    {
        rc_primary = get_primary_monitor_rect( get_thread_dpi() );
        if (minmax.ptMaxSize.x == (rc_primary.right - rc_primary.left) + 2 * xinc &&
            minmax.ptMaxSize.y == (rc_primary.bottom - rc_primary.top) + 2 * yinc)
        {
            minmax.ptMaxSize.x = (rc_max.right - rc_max.left) + 2 * xinc;
            minmax.ptMaxSize.y = (rc_max.bottom - rc_max.top) + 2 * yinc;
        }
        if (minmax.ptMaxPosition.x == -xinc && minmax.ptMaxPosition.y == -yinc)
        {
            minmax.ptMaxPosition.x = rc_max.left - xinc;
            minmax.ptMaxPosition.y = rc_max.top - yinc;
        }
    }

    TRACE( "%d %d / %d %d / %d %d / %d %d\n",
           minmax.ptMaxSize.x, minmax.ptMaxSize.y,
           minmax.ptMaxPosition.x, minmax.ptMaxPosition.y,
           minmax.ptMaxTrackSize.x, minmax.ptMaxTrackSize.y,
           minmax.ptMinTrackSize.x, minmax.ptMinTrackSize.y );

    minmax.ptMaxTrackSize.x = max( minmax.ptMaxTrackSize.x, minmax.ptMinTrackSize.x );
    minmax.ptMaxTrackSize.y = max( minmax.ptMaxTrackSize.y, minmax.ptMinTrackSize.y );

    set_thread_dpi_awareness_context( context );
    return minmax;
}

static POINT get_first_minimized_child_pos( const RECT *parent, const MINIMIZEDMETRICS *mm,
                                            int width, int height )
{
    POINT ret;

    if (mm->iArrange & ARW_STARTRIGHT)
        ret.x = parent->right - mm->iHorzGap - width;
    else
        ret.x = parent->left + mm->iHorzGap;
    if (mm->iArrange & ARW_STARTTOP)
        ret.y = parent->top + mm->iVertGap;
    else
        ret.y = parent->bottom - mm->iVertGap - height;

    return ret;
}

static void get_next_minimized_child_pos( const RECT *parent, const MINIMIZEDMETRICS *mm,
                                          int width, int height, POINT *pos )
{
    BOOL next;

    if (mm->iArrange & ARW_UP) /* == ARW_DOWN */
    {
        if (mm->iArrange & ARW_STARTTOP)
        {
            pos->y += height + mm->iVertGap;
            if ((next = pos->y + height > parent->bottom))
                pos->y = parent->top + mm->iVertGap;
        }
        else
        {
            pos->y -= height + mm->iVertGap;
            if ((next = pos->y < parent->top))
                pos->y = parent->bottom - mm->iVertGap - height;
        }

        if (next)
        {
            if (mm->iArrange & ARW_STARTRIGHT)
                pos->x -= width + mm->iHorzGap;
            else
                pos->x += width + mm->iHorzGap;
        }
    }
    else
    {
        if (mm->iArrange & ARW_STARTRIGHT)
        {
            pos->x -= width + mm->iHorzGap;
            if ((next = pos->x < parent->left))
                pos->x = parent->right - mm->iHorzGap - width;
        }
        else
        {
            pos->x += width + mm->iHorzGap;
            if ((next = pos->x + width > parent->right))
                pos->x = parent->left + mm->iHorzGap;
        }

        if (next)
        {
            if (mm->iArrange & ARW_STARTTOP)
                pos->y += height + mm->iVertGap;
            else
                pos->y -= height + mm->iVertGap;
        }
    }
}

static POINT get_minimized_pos( HWND hwnd, POINT pt )
{
    RECT rect, parent_rect = {0};
    HWND parent, child;
    HRGN hrgn, tmp;
    MINIMIZEDMETRICS metrics;
    int width, height;

    parent = NtUserGetAncestor( hwnd, GA_PARENT );
    if (parent == get_desktop_window())
    {
        MONITORINFO mon_info = monitor_info_from_window( hwnd, MONITOR_DEFAULTTOPRIMARY );
        parent_rect = mon_info.rcWork;
    }
    else get_client_rect( parent, &parent_rect, get_thread_dpi() );

    if (pt.x >= parent_rect.left && (pt.x + get_system_metrics( SM_CXMINIMIZED ) < parent_rect.right) &&
        pt.y >= parent_rect.top  && (pt.y + get_system_metrics( SM_CYMINIMIZED ) < parent_rect.bottom))
        return pt;  /* The icon already has a suitable position */

    width  = get_system_metrics( SM_CXMINIMIZED );
    height = get_system_metrics( SM_CYMINIMIZED );

    metrics.cbSize = sizeof(metrics);
    NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );

    /* Check if another icon already occupies this spot */
    /* FIXME: this is completely inefficient */

    hrgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    tmp = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    for (child = get_window_relative( parent, GW_CHILD );
         child;
         child = get_window_relative( child, GW_HWNDNEXT ))
    {
        if (child == hwnd) continue;
        if ((get_window_long( child, GWL_STYLE ) & (WS_VISIBLE|WS_MINIMIZE)) != (WS_VISIBLE|WS_MINIMIZE))
            continue;
        if (get_window_rect_rel( child, COORDS_PARENT, &rect, get_thread_dpi() ))
        {
            NtGdiSetRectRgn( tmp, rect.left, rect.top, rect.right, rect.bottom );
            NtGdiCombineRgn( hrgn, hrgn, tmp, RGN_OR );
        }
    }
    NtGdiDeleteObjectApp( tmp );

    pt = get_first_minimized_child_pos( &parent_rect, &metrics, width, height );
    for (;;)
    {
        SetRect( &rect, pt.x, pt.y, pt.x + width, pt.y + height );
        if (!NtGdiRectInRegion( hrgn, &rect ))
            break;

        get_next_minimized_child_pos( &parent_rect, &metrics, width, height, &pt );
    }

    NtGdiDeleteObjectApp( hrgn );
    return pt;
}

/***********************************************************************
 *           window_min_maximize
 */
static UINT window_min_maximize( HWND hwnd, UINT cmd, RECT *rect )
{
    UINT swp_flags = 0;
    DWORD old_style;
    MINMAXINFO minmax;
    WINDOWPLACEMENT wpl;

    TRACE( "%p %u\n", hwnd, cmd );

    wpl.length = sizeof(wpl);
    NtUserGetWindowPlacement( hwnd, &wpl );

    if (call_hooks( WH_CBT, HCBT_MINMAX, (WPARAM)hwnd, cmd, 0 ))
        return SWP_NOSIZE | SWP_NOMOVE;

    if (is_iconic( hwnd ))
    {
        switch (cmd)
        {
        case SW_SHOWMINNOACTIVE:
        case SW_SHOWMINIMIZED:
        case SW_FORCEMINIMIZE:
        case SW_MINIMIZE:
            wpl.ptMinPosition = get_minimized_pos( hwnd, wpl.ptMinPosition );

            SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                     wpl.ptMinPosition.x + get_system_metrics( SM_CXMINIMIZED ),
                     wpl.ptMinPosition.y + get_system_metrics( SM_CYMINIMIZED ));
            return SWP_NOSIZE | SWP_NOMOVE;
        }
        if (!send_message( hwnd, WM_QUERYOPEN, 0, 0 )) return SWP_NOSIZE | SWP_NOMOVE;
        swp_flags |= SWP_NOCOPYBITS;
    }

    switch( cmd )
    {
    case SW_SHOWMINNOACTIVE:
    case SW_SHOWMINIMIZED:
    case SW_FORCEMINIMIZE:
    case SW_MINIMIZE:
        if (is_zoomed( hwnd )) win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );
        else win_set_flags( hwnd, 0, WIN_RESTORE_MAX );

        if (get_focus() == hwnd)
        {
            if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD)
                NtUserSetFocus( NtUserGetAncestor( hwnd, GA_PARENT ));
            else
                NtUserSetFocus( 0 );
        }

        old_style = set_window_style_bits( hwnd, WS_MINIMIZE, WS_MAXIMIZE );

        wpl.ptMinPosition = get_minimized_pos( hwnd, wpl.ptMinPosition );

        if (!(old_style & WS_MINIMIZE)) swp_flags |= SWP_STATECHANGED;
        SetRect( rect, wpl.ptMinPosition.x, wpl.ptMinPosition.y,
                 wpl.ptMinPosition.x + get_system_metrics(SM_CXMINIMIZED),
                 wpl.ptMinPosition.y + get_system_metrics(SM_CYMINIMIZED) );
        swp_flags |= SWP_NOCOPYBITS;
        break;

    case SW_MAXIMIZE:
        old_style = get_window_long( hwnd, GWL_STYLE );
        if ((old_style & WS_MAXIMIZE) && (old_style & WS_VISIBLE)) return SWP_NOSIZE | SWP_NOMOVE;

        minmax = get_min_max_info( hwnd );

        old_style = set_window_style_bits( hwnd, WS_MAXIMIZE, WS_MINIMIZE );
        if (old_style & WS_MINIMIZE)
            win_set_flags( hwnd, WIN_RESTORE_MAX, 0 );

        if (!(old_style & WS_MAXIMIZE)) swp_flags |= SWP_STATECHANGED;
        SetRect( rect, minmax.ptMaxPosition.x, minmax.ptMaxPosition.y,
                 minmax.ptMaxPosition.x + minmax.ptMaxSize.x,
                 minmax.ptMaxPosition.y + minmax.ptMaxSize.y );
        break;

    case SW_SHOWNOACTIVATE:
        win_set_flags( hwnd, 0, WIN_RESTORE_MAX );
        /* fall through */
    case SW_SHOWNORMAL:
    case SW_RESTORE:
    case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
        old_style = set_window_style_bits( hwnd, 0, WS_MINIMIZE | WS_MAXIMIZE );
        if (old_style & WS_MINIMIZE)
        {
            if (win_get_flags( hwnd ) & WIN_RESTORE_MAX)
            {
                /* Restore to maximized position */
                minmax = get_min_max_info( hwnd );
                set_window_style_bits( hwnd, WS_MAXIMIZE, 0 );
                swp_flags |= SWP_STATECHANGED;
                SetRect( rect, minmax.ptMaxPosition.x, minmax.ptMaxPosition.y,
                         minmax.ptMaxPosition.x + minmax.ptMaxSize.x,
                         minmax.ptMaxPosition.y + minmax.ptMaxSize.y );
                break;
            }
        }
        else if (!(old_style & WS_MAXIMIZE)) break;

        swp_flags |= SWP_STATECHANGED;

        /* Restore to normal position */

        *rect = wpl.rcNormalPosition;
        break;
    }

    return swp_flags;
}

/***********************************************************************
 *           NtUserArrangeIconicWindows (win32u.@)
 */
UINT WINAPI NtUserArrangeIconicWindows( HWND parent )
{
    int width, height, count = 0;
    MINIMIZEDMETRICS metrics;
    RECT parent_rect = {0};
    HWND child;
    POINT pt;

    metrics.cbSize = sizeof(metrics);
    NtUserSystemParametersInfo( SPI_GETMINIMIZEDMETRICS, sizeof(metrics), &metrics, 0 );
    width  = get_system_metrics( SM_CXMINIMIZED );
    height = get_system_metrics( SM_CYMINIMIZED );

    if (parent == get_desktop_window())
    {
        MONITORINFO mon_info = monitor_info_from_window( 0, MONITOR_DEFAULTTOPRIMARY );
        parent_rect = mon_info.rcWork;
    }
    else get_client_rect( parent, &parent_rect, get_thread_dpi() );

    pt = get_first_minimized_child_pos( &parent_rect, &metrics, width, height );

    child = get_window_relative( parent, GW_CHILD );
    while (child)
    {
        if (is_iconic( child ))
        {
            NtUserSetWindowPos( child, 0, pt.x, pt.y, 0, 0,
                                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
            get_next_minimized_child_pos( &parent_rect, &metrics, width, height, &pt );
            count++;
        }
        child = get_window_relative( child, GW_HWNDNEXT );
    }
    return count;
}

/*******************************************************************
 *           update_window_state
 *
 * Trigger an update of the window's driver state and surface.
 */
void update_window_state( HWND hwnd )
{
    static const UINT swp_flags = SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE |
                                  SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW;
    UINT context;
    RECT valid_rects[2], surface_rect;
    struct window_surface *surface;
    struct window_rects new_rects;

    if (!is_current_thread_window( hwnd ))
    {
        NtUserPostMessage( hwnd, WM_WINE_UPDATEWINDOWSTATE, 0, 0 );
        return;
    }

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));
    get_window_rects( hwnd, COORDS_PARENT, &new_rects, get_thread_dpi() );
    valid_rects[0] = valid_rects[1] = new_rects.client;

    surface = get_window_surface( hwnd, swp_flags, FALSE, &new_rects, &surface_rect );
    apply_window_pos( hwnd, 0, swp_flags, surface, &new_rects, valid_rects );
    if (surface) window_surface_release( surface );

    set_thread_dpi_awareness_context( context );
}

/***********************************************************************
 *              show_window
 *
 * Implementation of ShowWindow and ShowWindowAsync.
 */
static BOOL show_window( HWND hwnd, INT cmd )
{
    WND *win;
    HWND parent;
    DWORD style = get_window_long( hwnd, GWL_STYLE ), new_style;
    BOOL was_visible = (style & WS_VISIBLE) != 0;
    BOOL show_flag = TRUE;
    RECT newPos = {0, 0, 0, 0};
    UINT new_swp, swp = 0, context;

    TRACE( "hwnd=%p, cmd=%d, was_visible %d\n", hwnd, cmd, was_visible );

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));

    switch(cmd)
    {
    case SW_HIDE:
        if (!was_visible) goto done;
        show_flag = FALSE;
        swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE;
        if (style & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        break;

    case SW_SHOWMINNOACTIVE:
    case SW_MINIMIZE:
    case SW_FORCEMINIMIZE: /* FIXME: Does not work if thread is hung. */
        swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        /* fall through */
    case SW_SHOWMINIMIZED:
        swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
        swp |= window_min_maximize( hwnd, cmd, &newPos );
        if ((style & WS_MINIMIZE) && was_visible) goto done;
        break;

    case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
        if (!was_visible) swp |= SWP_SHOWWINDOW;
        swp |= SWP_FRAMECHANGED;
        swp |= window_min_maximize( hwnd, SW_MAXIMIZE, &newPos );
        if ((style & WS_MAXIMIZE) && was_visible) goto done;
        break;

    case SW_SHOWNA:
        swp |= SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
        if (style & WS_CHILD) swp |= SWP_NOZORDER;
        break;

    case SW_SHOW:
        if (was_visible) goto done;
        swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
        if (style & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        break;

    case SW_SHOWNOACTIVATE:
        swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        /* fall through */
    case SW_RESTORE:
        /* fall through */
    case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
    case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
        if (!was_visible) swp |= SWP_SHOWWINDOW;
        if (style & (WS_MINIMIZE | WS_MAXIMIZE))
        {
            swp |= SWP_FRAMECHANGED;
            swp |= window_min_maximize( hwnd, cmd, &newPos );
        }
        else
        {
            if (was_visible) goto done;
            swp |= SWP_NOSIZE | SWP_NOMOVE;
        }
        if (style & WS_CHILD && !(swp & SWP_STATECHANGED)) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        break;

    default:
        goto done;
    }

    if ((show_flag != was_visible || cmd == SW_SHOWNA) && cmd != SW_SHOWMAXIMIZED && !(swp & SWP_STATECHANGED))
    {
        send_message( hwnd, WM_SHOWWINDOW, show_flag, 0 );
        if (!is_window( hwnd )) goto done;
    }

    if (IsRectEmpty( &newPos )) new_swp = swp;
    else if ((new_swp = user_driver->pShowWindow( hwnd, cmd, &newPos, swp )) == ~0)
    {
        if (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) new_swp = swp;
        else if (is_iconic( hwnd ) && (newPos.left != -32000 || newPos.top != -32000))
        {
            OffsetRect( &newPos, -32000 - newPos.left, -32000 - newPos.top );
            new_swp = swp & ~(SWP_NOMOVE | SWP_NOCLIENTMOVE);
        }
        else new_swp = swp;
    }
    swp = new_swp;

    parent = NtUserGetAncestor( hwnd, GA_PARENT );
    if (parent && !is_window_visible( parent ) && !(swp & SWP_STATECHANGED))
    {
        /* if parent is not visible simply toggle WS_VISIBLE and return */
        if (show_flag) set_window_style_bits( hwnd, WS_VISIBLE, 0 );
        else set_window_style_bits( hwnd, 0, WS_VISIBLE );
    }
    else
        NtUserSetWindowPos( hwnd, HWND_TOP, newPos.left, newPos.top,
                            newPos.right - newPos.left, newPos.bottom - newPos.top, swp );

    new_style = get_window_long( hwnd, GWL_STYLE );
    if (((style ^ new_style) & WS_MINIMIZE) != 0)
    {
        if ((new_style & WS_MINIMIZE) != 0)
            NtUserNotifyWinEvent( EVENT_SYSTEM_MINIMIZESTART, hwnd, OBJID_WINDOW, 0 );
        else
            NtUserNotifyWinEvent( EVENT_SYSTEM_MINIMIZEEND, hwnd, OBJID_WINDOW, 0 );
    }

    if (cmd == SW_HIDE)
    {
        HWND hFocus;

        /* FIXME: This will cause the window to be activated irrespective
         * of whether it is owned by the same thread. Has to be done
         * asynchronously.
         */

        if (hwnd == get_active_window()) activate_other_window( hwnd );

        /* Revert focus to parent */
        hFocus = get_focus();
        if (hwnd == hFocus)
        {
            HWND parent = NtUserGetAncestor(hwnd, GA_PARENT);
            if (parent == get_desktop_window()) parent = 0;
            NtUserSetFocus(parent);
        }
        goto done;
    }

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS) goto done;

    if (win->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
        int wParam = SIZE_RESTORED;
        RECT client = {0};
        LPARAM lparam;

        get_client_rect_rel( hwnd, COORDS_PARENT, &client, get_thread_dpi() );
        lparam = MAKELONG( client.right - client.left, client.bottom - client.top );
        win->flags &= ~WIN_NEED_SIZE;
        if (win->dwStyle & WS_MAXIMIZE) wParam = SIZE_MAXIMIZED;
        else if (win->dwStyle & WS_MINIMIZE)
        {
            wParam = SIZE_MINIMIZED;
            lparam = 0;
        }
        release_win_ptr( win );

        send_message( hwnd, WM_SIZE, wParam, lparam );
        send_message( hwnd, WM_MOVE, 0, MAKELONG( client.left, client.top ));
    }
    else release_win_ptr( win );

    /* if previous state was minimized Windows sets focus to the window */
    if (style & WS_MINIMIZE)
    {
        NtUserSetFocus( hwnd );
        /* Send a WM_ACTIVATE message for a top level window, even if the window is already active */
        if (NtUserGetAncestor( hwnd, GA_ROOT ) == hwnd && !(swp & SWP_NOACTIVATE))
            send_message( hwnd, WM_ACTIVATE, WA_ACTIVE, 0 );
    }

done:
    set_thread_dpi_awareness_context( context );
    return was_visible;
}

/***********************************************************************
 *           NtUserShowWindowAsync (win32u.@)
 *
 * doesn't wait; returns immediately.
 * used by threads to toggle windows in other (possibly hanging) threads
 */
BOOL WINAPI NtUserShowWindowAsync( HWND hwnd, INT cmd )
{
    HWND full_handle;

    if (is_broadcast(hwnd))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ((full_handle = is_current_thread_window( hwnd )))
        return show_window( full_handle, cmd );

    return NtUserMessageCall( hwnd, WM_WINE_SHOWWINDOW, cmd, 0, 0,
                              NtUserSendNotifyMessage, FALSE );
}

/***********************************************************************
 *           NtUserShowWindow (win32u.@)
 */
BOOL WINAPI NtUserShowWindow( HWND hwnd, INT cmd )
{
    HWND full_handle;

    if (is_broadcast(hwnd))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if ((full_handle = is_current_thread_window( hwnd )))
        return show_window( full_handle, cmd );

    if ((cmd == SW_HIDE) && !(get_window_long( hwnd, GWL_STYLE ) & WS_VISIBLE))
        return FALSE;

    if ((cmd == SW_SHOW) && (get_window_long( hwnd, GWL_STYLE ) & WS_VISIBLE))
        return TRUE;

    return send_message( hwnd, WM_WINE_SHOWWINDOW, cmd, 0 );
}

/***********************************************************************
 *           NtUserShowOwnedPopups (win32u.@)
 */
BOOL WINAPI NtUserShowOwnedPopups( HWND owner, BOOL show )
{
    int count = 0;
    HWND *win_array = list_window_children( 0 );

    if (!win_array) return TRUE;

    while (win_array[count]) count++;
    while (--count >= 0)
    {
        if (get_window_relative( win_array[count], GW_OWNER ) != owner) continue;
        if (show)
        {
            if (win_get_flags( win_array[count] ) & WIN_NEEDS_SHOW_OWNEDPOPUP)
                /* In Windows, ShowOwnedPopups(TRUE) generates
                 * WM_SHOWWINDOW messages with SW_PARENTOPENING,
                 * regardless of the state of the owner
                 */
                send_message( win_array[count], WM_SHOWWINDOW, SW_SHOWNORMAL, SW_PARENTOPENING );
        }
        else
        {
            if (get_window_long( win_array[count], GWL_STYLE ) & WS_VISIBLE)
                /* In Windows, ShowOwnedPopups(FALSE) generates
                 * WM_SHOWWINDOW messages with SW_PARENTCLOSING,
                 * regardless of the state of the owner
                 */
                send_message( win_array[count], WM_SHOWWINDOW, SW_HIDE, SW_PARENTCLOSING );
        }
    }

    free( win_array );
    return TRUE;
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
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    if (!info->hwnd || info->cbSize != sizeof(FLASHWINFO) || !is_window( info->hwnd ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    FIXME( "%p - semi-stub\n", info );

    if (is_iconic( info->hwnd ))
    {
        NtUserRedrawWindow( info->hwnd, 0, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_FRAME );

        win = get_win_ptr( info->hwnd );
        if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
        if (info->dwFlags & FLASHW_CAPTION && !(win->flags & WIN_NCACTIVATED))
        {
            win->flags |= WIN_NCACTIVATED;
        }
        else if (!info->dwFlags)
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
        hwnd = win->handle;  /* make it a full handle */

        if (info->dwFlags) wparam = !(win->flags & WIN_NCACTIVATED);
        else wparam = (hwnd == NtUserGetForegroundWindow());

        release_win_ptr( win );

        if (!info->dwFlags || info->dwFlags & FLASHW_CAPTION)
            send_message( hwnd, WM_NCACTIVATE, wparam, 0 );

        user_driver->pFlashWindowEx( info );
        return wparam;
    }
}

/***********************************************************************
 *           NtUserGetWindowContextHelpId   (win32u.@)
 */
DWORD WINAPI NtUserGetWindowContextHelpId( HWND hwnd )
{
    DWORD retval;
    WND *win = get_win_ptr( hwnd );
    if (!win || win == WND_DESKTOP) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (is_window( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return 0;
    }
    retval = win->helpContext;
    release_win_ptr( win );
    return retval;
}

/***********************************************************************
 *           NtUserSetWindowContextHelpId   (win32u.@)
 */
BOOL WINAPI NtUserSetWindowContextHelpId( HWND hwnd, DWORD id )
{
    WND *win = get_win_ptr( hwnd );
    if (!win || win == WND_DESKTOP) return FALSE;
    if (win == WND_OTHER_PROCESS)
    {
        if (is_window( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return FALSE;
    }
    win->helpContext = id;
    release_win_ptr( win );
    return TRUE;
}

/***********************************************************************
 *           NtUserInternalGetWindowIcon   (win32u.@)
 */
HICON WINAPI NtUserInternalGetWindowIcon( HWND hwnd, UINT type )
{
    WND *win = get_win_ptr( hwnd );
    HICON ret;

    TRACE( "hwnd %p, type %#x\n", hwnd, type );

    if (!win)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        if (is_window( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return 0;
    }

    switch (type)
    {
        case ICON_BIG:
            ret = win->hIcon;
            if (!ret) ret = (HICON)get_class_long_ptr( hwnd, GCLP_HICON, FALSE );
            break;

        case ICON_SMALL:
        case ICON_SMALL2:
            ret = win->hIconSmall ? win->hIconSmall : win->hIconSmall2;
            if (!ret) ret = (HICON)get_class_long_ptr( hwnd, GCLP_HICONSM, FALSE );
            if (!ret) ret = (HICON)get_class_long_ptr( hwnd, GCLP_HICON, FALSE );
            break;

        default:
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            release_win_ptr( win );
            return 0;
    }
    release_win_ptr( win );

    if (!ret) ret = LoadImageW( 0, (const WCHAR *)IDI_APPLICATION, IMAGE_ICON,
                                0, 0, LR_SHARED | LR_DEFAULTSIZE );

    return CopyImage( ret, IMAGE_ICON, 0, 0, 0 );
}

/***********************************************************************
 *           send_destroy_message
 */
static void send_destroy_message( HWND hwnd, BOOL winevent )
{
    GUITHREADINFO info;

    info.cbSize = sizeof(info);
    if (NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info ))
    {
        if (hwnd == info.hwndCaret) NtUserDestroyCaret();
        if (hwnd == info.hwndActive) activate_other_window( hwnd );
    }

    if (hwnd == NtUserGetClipboardOwner()) release_clipboard_owner( hwnd );

    if (winevent)
        NtUserNotifyWinEvent( EVENT_OBJECT_DESTROY, hwnd, OBJID_WINDOW, 0 );

    send_message( hwnd, WM_DESTROY, 0, 0);

    /*
     * This WM_DESTROY message can trigger re-entrant calls to DestroyWindow
     * make sure that the window still exists when we come back.
     */
    if (is_window(hwnd))
    {
        HWND *children;
        int i;

        if (!(children = list_window_children( hwnd ))) return;

        for (i = 0; children[i]; i++)
        {
            if (is_window( children[i] )) send_destroy_message( children[i], FALSE );
        }
        free( children );
    }
    else
        WARN( "\tdestroyed itself while in WM_DESTROY!\n" );
}

/***********************************************************************
 *           free_window_handle
 *
 * Free a window handle.
 */
static void free_window_handle( HWND hwnd )
{
    WND *win;

    TRACE( "\n" );

    if ((win = get_user_handle_ptr( hwnd, NTUSER_OBJ_WINDOW )) && win != OBJ_OTHER_PROCESS)
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = wine_server_user_handle( hwnd );
            wine_server_call( req );
            set_user_handle_ptr( hwnd, NULL );
        }
        SERVER_END_REQ;
        user_unlock();
        free( win->pScroll );
        free( win->text );
        free( win );
    }
}

/***********************************************************************
 *           destroy_window
 */
LRESULT destroy_window( HWND hwnd )
{
    struct list vulkan_surfaces = LIST_INIT(vulkan_surfaces);
    struct window_surface *surface;
    HMENU menu = 0, sys_menu;
    WND *win;
    HWND *children;

    TRACE( "%p\n", hwnd );

    unregister_imm_window( hwnd );

    /* free child windows */
    if ((children = list_window_children( hwnd )))
    {
        int i;
        for (i = 0; children[i]; i++)
        {
            if (is_current_thread_window( children[i] ))
                destroy_window( children[i] );
            else
                NtUserMessageCall( children[i], WM_WINE_DESTROYWINDOW, 0, 0,
                                   0, NtUserSendNotifyMessage, FALSE );
        }
        free( children );
    }

    /* Unlink now so we won't bother with the children later on */
    SERVER_START_REQ( set_parent )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->parent = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    send_message( hwnd, WM_NCDESTROY, 0, 0 );

    /* FIXME: do we need to fake QS_MOUSEMOVE wakebit? */

    /* free resources associated with the window */

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS) return 0;
    if ((win->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
        menu = (HMENU)win->wIDmenu;
    sys_menu = win->hSysMenu;
    free_dce( win->dce, hwnd );
    win->dce = NULL;
    NtUserDestroyCursor( win->hIconSmall2, 0 );
    list_move_tail( &vulkan_surfaces, &win->vulkan_surfaces );
    surface = win->surface;
    win->surface = NULL;
    release_win_ptr( win );

    NtUserDestroyMenu( menu );
    NtUserDestroyMenu( sys_menu );
    if (surface)
    {
        register_window_surface( surface, NULL );
        window_surface_release( surface );
    }

    vulkan_detach_surfaces( &vulkan_surfaces );
    if (win->opengl_drawable) opengl_drawable_release( win->opengl_drawable );
    user_driver->pDestroyWindow( hwnd );

    free_window_handle( hwnd );
    return 0;
}

/***********************************************************************
 *           NtUserDestroyWindow (win32u.@)
 */
static BOOL user_destroy_window( HWND hwnd, BOOL winevent )
{
    BOOL is_child;

    if (!(hwnd = is_current_thread_window( hwnd )) || is_desktop_window( hwnd ))
    {
        RtlSetLastWin32Error( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    TRACE( "(%p)\n", hwnd );

    if (call_hooks( WH_CBT, HCBT_DESTROYWND, (WPARAM)hwnd, 0, 0 )) return FALSE;

    if (is_menu_active() == hwnd) NtUserEndMenu();

    is_child = (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) != 0;

    if (is_child)
    {
        if (!is_exiting_thread( GetCurrentThreadId() ))
            send_parent_notify( hwnd, WM_DESTROY );
    }
    else if (!get_window_relative( hwnd, GW_OWNER ))
    {
        call_hooks( WH_SHELL, HSHELL_WINDOWDESTROYED, (WPARAM)hwnd, 0, 0 );
        /* FIXME: clean up palette - see "Internals" p.352 */
    }

    if (!is_window( hwnd )) return TRUE;

    /* Hide the window */
    if (get_window_long( hwnd, GWL_STYLE ) & WS_VISIBLE)
    {
        /* Only child windows receive WM_SHOWWINDOW in DestroyWindow() */
        if (is_child)
            NtUserShowWindow( hwnd, SW_HIDE );
        else
            NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
                                SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW );
    }

    if (!is_window( hwnd )) return TRUE;

    /* Recursively destroy child windows */
    if (!is_child)
    {
        for (;;)
        {
            BOOL got_one = FALSE;
            HWND *children;
            unsigned int i;

            if (!(children = list_window_children( 0 ))) break;

            for (i = 0; children[i]; i++)
            {
                if (get_window_relative( children[i], GW_OWNER ) != hwnd) continue;
                if (is_current_thread_window( children[i] ))
                {
                    user_destroy_window( children[i], FALSE );
                    got_one = TRUE;
                    continue;
                }
                set_window_owner( children[i], 0 );
            }
            free( children );
            if (!got_one) break;
        }
    }

    send_destroy_message( hwnd, winevent );
    if (!is_window( hwnd )) return TRUE;

    destroy_window( hwnd );
    return TRUE;
}

BOOL WINAPI NtUserDestroyWindow( HWND hwnd )
{
    return user_destroy_window( hwnd, TRUE );
}

/*****************************************************************************
 *           destroy_thread_windows
 *
 * Destroy all window owned by the current thread.
 */
void destroy_thread_windows(void)
{
    struct list vulkan_surfaces = LIST_INIT(vulkan_surfaces);
    struct destroy_entry
    {
        HWND handle;
        HMENU menu;
        HMENU sys_menu;
        struct window_surface *surface;
        struct destroy_entry *next;
    } *entry, *free_list = NULL;
    HANDLE handle = 0;
    WND *win;

    /* recycle WND structs as destroy_entry structs */
    C_ASSERT( sizeof(struct destroy_entry) <= sizeof(WND) );

    user_lock();
    while ((win = next_thread_user_object( GetCurrentThreadId(), &handle, NTUSER_OBJ_WINDOW )))
    {
        BOOL is_child = (win->dwStyle & (WS_CHILD | WS_POPUP)) == WS_CHILD;
        struct destroy_entry tmp = {0};

        free_dce( win->dce, win->handle );
        set_user_handle_ptr( handle, NULL );
        free( win->pScroll );
        free( win->text );

        /* recycle the WND struct as a destroy_entry struct */
        entry = (struct destroy_entry *)win;
        tmp.handle = win->handle;
        list_move_tail( &vulkan_surfaces, &win->vulkan_surfaces );
        if (!is_child) tmp.menu = (HMENU)win->wIDmenu;
        tmp.sys_menu = win->hSysMenu;
        tmp.surface = win->surface;
        *entry = tmp;

        entry->next = free_list;
        free_list = entry;
    }
    if (free_list)
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = 0; /* destroy all thread windows */
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    user_unlock();

    while ((entry = free_list))
    {
        free_list = entry->next;
        TRACE( "destroying %p\n", entry );

        user_driver->pDestroyWindow( entry->handle );
        if (win->opengl_drawable) opengl_drawable_release( win->opengl_drawable );

        NtUserDestroyMenu( entry->menu );
        NtUserDestroyMenu( entry->sys_menu );
        if (entry->surface)
        {
            register_window_surface( entry->surface, NULL );
            window_surface_release( entry->surface );
        }
        free( entry );
    }

    vulkan_detach_surfaces( &vulkan_surfaces );
}

/***********************************************************************
 *           create_window_handle
 *
 * Create a window handle with the server.
 */
static WND *create_window_handle( HWND parent, HWND owner, UNICODE_STRING *name, HINSTANCE class_instance,
                                  HINSTANCE instance, BOOL ansi, DWORD style, DWORD ex_style )
{
    UINT dpi_context = get_thread_dpi_awareness_context();
    HWND handle = 0, full_parent = 0, full_owner = 0;
    struct tagCLASS *class = NULL;
    int extra_bytes = 0;
    WND *win;

    SERVER_START_REQ( create_window )
    {
        req->parent          = wine_server_user_handle( parent );
        req->owner           = wine_server_user_handle( owner );
        req->class_instance  = wine_server_client_ptr( class_instance );
        req->instance        = wine_server_client_ptr( instance );
        req->dpi_context     = dpi_context;
        req->style           = style;
        req->ex_style        = ex_style;
        if (!(req->atom = get_int_atom_value( name )) && name->Length)
            wine_server_add_data( req, name->Buffer, name->Length );
        if (!wine_server_call_err( req ))
        {
            handle      = wine_server_ptr_handle( reply->handle );
            full_parent = wine_server_ptr_handle( reply->parent );
            full_owner  = wine_server_ptr_handle( reply->owner );
            extra_bytes = reply->extra;
            class       = wine_server_get_ptr( reply->class_ptr );
        }
    }
    SERVER_END_REQ;

    if (!handle)
    {
        WARN( "error %d creating window\n", RtlGetLastWin32Error() );
        return NULL;
    }

    if (!(win = calloc( 1, FIELD_OFFSET(WND, wExtra) + extra_bytes )))
    {
        SERVER_START_REQ( destroy_window )
        {
            req->handle = wine_server_user_handle( handle );
            wine_server_call( req );
        }
        SERVER_END_REQ;
        RtlSetLastWin32Error( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    if (!parent)  /* if parent is 0 we don't have a desktop window yet */
    {
        struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

        if (name->Buffer == (const WCHAR *)DESKTOP_CLASS_ATOM)
        {
            if (!thread_info->top_window) thread_info->top_window = HandleToUlong( full_parent ? full_parent : handle );
            else assert( full_parent == UlongToHandle( thread_info->top_window ));
            if (!thread_info->top_window) ERR_(win)( "failed to create desktop window\n" );
            else user_driver->pSetDesktopWindow( UlongToHandle( thread_info->top_window ));
            register_builtin_classes();
        }
        else  /* HWND_MESSAGE parent */
        {
            if (!thread_info->msg_window && !full_parent)
                thread_info->msg_window = HandleToUlong( handle );
        }
    }

    user_lock();

    win->handle     = handle;
    win->parent     = full_parent;
    win->owner      = full_owner;
    win->class      = class;
    win->winproc    = get_class_winproc( class );
    win->hInstance  = instance;
    win->cbWndExtra = extra_bytes;
    list_init( &win->vulkan_surfaces );
    set_user_handle_ptr( handle, win );
    if (is_winproc_unicode( win->winproc, !ansi )) win->flags |= WIN_ISUNICODE;
    return win;
}

static BOOL is_default_coord( int x )
{
    return x == CW_USEDEFAULT || x == 0x8000;
}

/***********************************************************************
 *           fix_cs_coordinates
 *
 * Fix the coordinates and return default show mode in sw.
 */
static void fix_cs_coordinates( CREATESTRUCTW *cs, INT *sw )
{
    if (cs->style & (WS_CHILD | WS_POPUP))
    {
        if (is_default_coord(cs->x)) cs->x = cs->y = 0;
        if (is_default_coord(cs->cx)) cs->cx = cs->cy = 0;
    }
    else  /* overlapped window */
    {
        RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
        MONITORINFO mon_info;

        if (!is_default_coord( cs->x ) && !is_default_coord( cs->cx ) && !is_default_coord( cs->cy ))
            return;

        mon_info = monitor_info_from_window( cs->hwndParent, MONITOR_DEFAULTTOPRIMARY );

        if (is_default_coord( cs->x ))
        {
            if (!is_default_coord( cs->y )) *sw = cs->y;
            cs->x = (params->dwFlags & STARTF_USEPOSITION) ? params->dwX : mon_info.rcWork.left;
            cs->y = (params->dwFlags & STARTF_USEPOSITION) ? params->dwY : mon_info.rcWork.top;
        }

        if (is_default_coord( cs->cx ))
        {
            if (params->dwFlags & STARTF_USESIZE)
            {
                cs->cx = params->dwXSize;
                cs->cy = params->dwYSize;
            }
            else
            {
                cs->cx = (mon_info.rcWork.right - mon_info.rcWork.left) * 3 / 4 - cs->x;
                cs->cy = (mon_info.rcWork.bottom - mon_info.rcWork.top) * 3 / 4 - cs->y;
            }
        }
        /* neither x nor cx are default. Check the y values.
         * In the trace we see Outlook and Outlook Express using
         * cy set to CW_USEDEFAULT when opening the address book.
         */
        else if (is_default_coord( cs->cy ))
        {
            FIXME( "Strange use of CW_USEDEFAULT in cy\n" );
            cs->cy = (mon_info.rcWork.bottom - mon_info.rcWork.top) * 3 / 4 - cs->y;
        }
    }
}

/***********************************************************************
 *           map_dpi_create_struct
 */
static void map_dpi_create_struct( CREATESTRUCTW *cs, UINT dpi_to )
{
    RECT rect = {cs->x, cs->y, cs->x + cs->cx, cs->y + cs->cy};
    UINT dpi_from = get_thread_dpi();

    if (!dpi_from || !dpi_to)
    {
        UINT raw_dpi, mon_dpi = monitor_dpi_from_rect( rect, get_thread_dpi(), &raw_dpi );
        if (!dpi_from) dpi_from = mon_dpi;
        else dpi_to = mon_dpi;
    }

    rect = map_dpi_rect( rect, dpi_from, dpi_to );
    cs->x = rect.left;
    cs->y = rect.top;
    cs->cx = rect.right - rect.left;
    cs->cy = rect.bottom - rect.top;
}

/***********************************************************************
 *           NtUserCreateWindowEx (win32u.@)
 */
HWND WINAPI NtUserCreateWindowEx( DWORD ex_style, UNICODE_STRING *class_name,
                                  UNICODE_STRING *version, UNICODE_STRING *window_name,
                                  DWORD style, INT x, INT y, INT cx, INT cy,
                                  HWND parent, HMENU menu, HINSTANCE class_instance, void *params,
                                  DWORD flags, HINSTANCE instance, DWORD unk, BOOL ansi )
{
    UINT win_dpi, context;
    struct window_surface *surface;
    struct window_rects new_rects;
    CBT_CREATEWNDW cbtc;
    HWND hwnd, owner = 0;
    CREATESTRUCTW cs;
    INT sw = SW_SHOW;
    RECT surface_rect;
    WND *win;

    static const WCHAR messageW[] = {'M','e','s','s','a','g','e'};

    cs.lpCreateParams = params;
    cs.hInstance  = instance ? instance : class_instance;
    cs.hMenu      = menu;
    cs.hwndParent = parent;
    cs.style      = style;
    cs.dwExStyle  = ex_style;
    cs.lpszName   = window_name ? window_name->Buffer : NULL;
    cs.lpszClass  = class_name->Buffer;
    cs.x  = x;
    cs.y  = y;
    cs.cx = cx;
    cs.cy = cy;

    /* Find the parent window */
    if (parent == HWND_MESSAGE)
    {
        cs.hwndParent = parent = get_hwnd_message_parent();
    }
    else if (parent)
    {
        if ((cs.style & (WS_CHILD|WS_POPUP)) != WS_CHILD)
        {
            owner = parent;
            parent = get_desktop_window();
        }
        else
        {
            DWORD parent_style = get_window_long( parent, GWL_EXSTYLE );
            if ((parent_style & WS_EX_LAYOUTRTL) && !(parent_style & WS_EX_NOINHERITLAYOUT))
                cs.dwExStyle |= WS_EX_LAYOUTRTL;
        }
    }
    else
    {
        if ((cs.style & (WS_CHILD|WS_POPUP)) == WS_CHILD)
        {
            WARN( "No parent for child window\n" );
            RtlSetLastWin32Error( ERROR_TLW_WITH_WSCHILD );
            return 0;  /* WS_CHILD needs a parent, but WS_POPUP doesn't */
        }

        /* are we creating the desktop or HWND_MESSAGE parent itself? */
        if (class_name->Buffer != (LPCWSTR)DESKTOP_CLASS_ATOM &&
            (class_name->Length != sizeof(messageW) ||
             wcsnicmp( class_name->Buffer, messageW, ARRAYSIZE(messageW) )))
        {
            if (get_process_layout() & LAYOUT_RTL) cs.dwExStyle |= WS_EX_LAYOUTRTL;
            parent = get_desktop_window();
        }
    }

    fix_cs_coordinates( &cs, &sw );
    cs.dwExStyle = fix_exstyle( cs.style, cs.dwExStyle );

    /* Create the window structure */

    style = cs.style & ~WS_VISIBLE;
    ex_style = cs.dwExStyle & ~WS_EX_LAYERED;
    if (!(win = create_window_handle( parent, owner, class_name, class_instance,
                                      cs.hInstance, ansi, style, ex_style )))
        return 0;
    hwnd = win->handle;

    /* Fill the window structure */

    win->text        = NULL;
    win->dwStyle     = style;
    win->dwExStyle   = ex_style;
    win->wIDmenu     = 0;
    win->helpContext = 0;
    win->pScroll     = NULL;
    win->userdata    = 0;
    win->hIcon       = 0;
    win->hIconSmall  = 0;
    win->hIconSmall2 = 0;
    win->hSysMenu    = 0;
    win->swap_interval = 1;

    win->min_pos.x = win->min_pos.y = -1;
    win->max_pos.x = win->max_pos.y = -1;
    SetRect( &win->normal_rect, cs.x, cs.y, cs.x + cs.cx, cs.y + cs.cy );

    if (win->dwStyle & WS_SYSMENU) NtUserSetSystemMenu( hwnd, 0 );

    win->imc = get_default_input_context();

    /* call the WH_CBT hook */

    release_win_ptr( win );
    cbtc.hwndInsertAfter = HWND_TOP;
    cbtc.lpcs = &cs;
    if (call_hooks( WH_CBT, HCBT_CREATEWND, (WPARAM)hwnd, (LPARAM)&cbtc, sizeof(cbtc) ))
    {
        free_window_handle( hwnd );
        return 0;
    }
    if (!(win = get_win_ptr( hwnd ))) return 0;

    /*
     * Correct the window styles.
     *
     * It affects only the style loaded into the WND structure.
     */

    if ((win->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
    {
        win->dwStyle |= WS_CLIPSIBLINGS;
        if (!(win->dwStyle & WS_POPUP)) win->dwStyle |= WS_CAPTION;
    }

    win->dwExStyle = cs.dwExStyle;
    /* WS_EX_WINDOWEDGE depends on some other styles */
    if ((win->dwStyle & (WS_DLGFRAME | WS_THICKFRAME)) &&
            !(win->dwStyle & (WS_CHILD | WS_POPUP)))
        win->dwExStyle |= WS_EX_WINDOWEDGE;

    if (!(win->dwStyle & (WS_CHILD | WS_POPUP))) win->flags |= WIN_NEED_SIZE;

    SERVER_START_REQ( init_window_info )
    {
        req->handle     = wine_server_user_handle( hwnd );
        req->style      = win->dwStyle;
        req->ex_style   = win->dwExStyle;
        req->is_unicode = (win->flags & WIN_ISUNICODE) != 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* Set the window menu */

    if ((win->dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD)
    {
        if (cs.hMenu && !set_window_menu( hwnd, cs.hMenu ))
        {
            release_win_ptr( win );
            free_window_handle( hwnd );
            return 0;
        }
    }
    else NtUserSetWindowLongPtr( hwnd, GWLP_ID, (ULONG_PTR)cs.hMenu, FALSE );

    release_win_ptr( win );

    win_dpi = get_dpi_for_window( hwnd );
    if (parent) map_dpi_create_struct( &cs, win_dpi );

    context = set_thread_dpi_awareness_context( get_window_dpi_awareness_context( hwnd ));

    /* send the WM_GETMINMAXINFO message and fix the size if needed */

    cx = cs.cx;
    cy = cs.cy;
    if ((cs.style & WS_THICKFRAME) || !(cs.style & (WS_POPUP | WS_CHILD)))
    {
        MINMAXINFO info = get_min_max_info( hwnd );
        cx = max( min( cx, info.ptMaxTrackSize.x ), info.ptMinTrackSize.x );
        cy = max( min( cy, info.ptMaxTrackSize.y ), info.ptMinTrackSize.y );
    }

    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    SetRect( &new_rects.window, cs.x, cs.y, cs.x + cx, cs.y + cy );
    /* check for wraparound */
    if (cs.x > 0x7fffffff - cx) new_rects.window.right = 0x7fffffff;
    if (cs.y > 0x7fffffff - cy) new_rects.window.bottom = 0x7fffffff;
    new_rects.client = new_rects.window;

    surface = get_window_surface( hwnd, SWP_NOZORDER | SWP_NOACTIVATE, FALSE, &new_rects, &surface_rect );
    if (!apply_window_pos( hwnd, 0, SWP_NOZORDER | SWP_NOACTIVATE, surface, &new_rects, NULL ))
    {
        if (surface) window_surface_release( surface );
        goto failed;
    }
    if (surface) window_surface_release( surface );

    /* send WM_NCCREATE */

    TRACE( "hwnd %p cs %d,%d %dx%d %s\n", hwnd, cs.x, cs.y, cs.cx, cs.cy, debugstr_window_rects(&new_rects) );
    if (!send_message_timeout( hwnd, WM_NCCREATE, 0, (LPARAM)&cs, SMTO_NORMAL, 0, ansi ))
    {
        WARN( "%p: aborted by WM_NCCREATE\n", hwnd );
        goto failed;
    }

    /* create default IME window */

    if (!is_desktop_window( hwnd ) && parent != get_hwnd_message_parent() &&
        register_imm_window( hwnd ))
    {
        TRACE( "register IME window for %p\n", hwnd );
        win_set_flags( hwnd, WIN_HAS_IME_WIN, 0 );
    }

    /* send WM_NCCALCSIZE */

    if (get_window_rect_rel( hwnd, COORDS_PARENT, &new_rects.window, win_dpi ))
    {
        /* yes, even if the CBT hook was called with HWND_TOP */
        HWND insert_after = (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) ? HWND_BOTTOM : HWND_TOP;
        new_rects.client = new_rects.window;

        /* the rectangle is in screen coords for WM_NCCALCSIZE when wparam is FALSE */
        map_window_points( parent, 0, (POINT *)&new_rects.client, 2, win_dpi );
        send_message( hwnd, WM_NCCALCSIZE, FALSE, (LPARAM)&new_rects.client );
        map_window_points( 0, parent, (POINT *)&new_rects.client, 2, win_dpi );

        surface = get_window_surface( hwnd, SWP_NOACTIVATE, FALSE, &new_rects, &surface_rect );
        apply_window_pos( hwnd, insert_after, SWP_NOACTIVATE, surface, &new_rects, NULL );
        if (surface) window_surface_release( surface );
    }
    else goto failed;

    /* send WM_CREATE */
    if (send_message_timeout( hwnd, WM_CREATE, 0, (LPARAM)&cs, SMTO_NORMAL, 0, ansi ) == -1)
        goto failed;

    /* call the driver */

    if (!user_driver->pCreateWindow( hwnd )) goto failed;

    NtUserNotifyWinEvent( EVENT_OBJECT_CREATE, hwnd, OBJID_WINDOW, 0 );

    /* send the size messages */

    if (!(win_get_flags( hwnd ) & WIN_NEED_SIZE))
    {
        RECT rect = {0};
        get_client_rect_rel( hwnd, COORDS_PARENT, &rect, win_dpi );
        send_message( hwnd, WM_SIZE, SIZE_RESTORED, MAKELONG( rect.right - rect.left, rect.bottom - rect.top ) );
        send_message( hwnd, WM_MOVE, 0, MAKELONG( rect.left, rect.top ) );
    }

    /* Show the window, maximizing or minimizing if needed */

    style = set_window_style_bits( hwnd, 0, WS_MAXIMIZE | WS_MINIMIZE );
    if (style & (WS_MINIMIZE | WS_MAXIMIZE))
    {
        RECT new_pos;
        UINT sw_flags = (style & WS_MINIMIZE) ? SW_MINIMIZE : SW_MAXIMIZE;

        sw_flags = window_min_maximize( hwnd, sw_flags, &new_pos );
        sw_flags |= SWP_FRAMECHANGED; /* Frame always gets changed */
        if (!(style & WS_VISIBLE) || (style & WS_CHILD) || get_active_window())
            sw_flags |= SWP_NOACTIVATE;
        NtUserSetWindowPos( hwnd, 0, new_pos.left, new_pos.top, new_pos.right - new_pos.left,
                            new_pos.bottom - new_pos.top, sw_flags );
    }

    /* Notify the parent window only */

    send_parent_notify( hwnd, WM_CREATE );
    if (!is_window( hwnd ))
    {
        set_thread_dpi_awareness_context( context );
        return 0;
    }

    if (parent == get_desktop_window())
        NtUserPostMessage( parent, WM_PARENTNOTIFY, WM_CREATE, (LPARAM)hwnd );

    if (cs.style & WS_VISIBLE)
    {
        if (cs.style & WS_MAXIMIZE)
            sw = SW_SHOW;
        else if (cs.style & WS_MINIMIZE)
            sw = SW_SHOWMINIMIZED;

        NtUserShowWindow( hwnd, sw );
        if (cs.dwExStyle & WS_EX_MDICHILD)
        {
            send_message( cs.hwndParent, WM_MDIREFRESHMENU, 0, 0 );
            /* ShowWindow won't activate child windows */
            NtUserSetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE );
        }
    }

    /* Call WH_SHELL hook */

    if (!(get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) && !get_window_relative( hwnd, GW_OWNER ))
        call_hooks( WH_SHELL, HSHELL_WINDOWCREATED, (WPARAM)hwnd, 0, 0 );

    TRACE( "created window %p\n", hwnd );
    set_thread_dpi_awareness_context( context );
    return hwnd;

failed:
    destroy_window( hwnd );
    set_thread_dpi_awareness_context( context );
    return 0;
}

static void *get_dialog_info( HWND hwnd )
{
    WND *win;
    void *ret;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return NULL;
    }

    ret = win->dlgInfo;
    release_win_ptr( win );
    return ret;
}

static BOOL set_dialog_info( HWND hwnd, void *info )
{
    WND *win;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;
    win->dlgInfo = info;
    release_win_ptr( win );
    return TRUE;
}

static BOOL set_raw_window_pos( HWND hwnd, RECT rect, UINT flags, BOOL internal )
{
    TRACE( "hwnd %p, rect %s, flags %#x, internal %u\n", hwnd, wine_dbgstr_rect(&rect), flags, internal );

    rect = map_rect_raw_to_virt( rect, get_thread_dpi() );

    if (internal)
    {
        NtUserSetInternalWindowPos( hwnd, SW_SHOW, &rect, NULL );
        return TRUE;
    }

    return NtUserSetWindowPos( hwnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, flags );
}

/*****************************************************************************
 *           NtUserCallHwnd (win32u.@)
 */
ULONG_PTR WINAPI NtUserCallHwnd( HWND hwnd, DWORD code )
{
    switch (code)
    {
    case NtUserCallHwnd_ActivateOtherWindow:
        activate_other_window( hwnd );
        return 0;

    case NtUserCallHwnd_GetDpiForWindow:
        return get_dpi_for_window( hwnd );

    case NtUserCallHwnd_GetLastActivePopup:
        return (ULONG_PTR)get_last_active_popup( hwnd );

    case NtUserCallHwnd_GetParent:
        return HandleToUlong( get_parent( hwnd ));

    case NtUserCallHwnd_GetDialogInfo:
        return (ULONG_PTR)get_dialog_info( hwnd );

    case NtUserCallHwnd_GetMDIClientInfo:
        if (!(win_get_flags( hwnd ) & WIN_ISMDICLIENT)) return 0;
        return get_window_long_ptr( hwnd, sizeof(void *), FALSE );

    case NtUserCallHwnd_GetWindowDpiAwarenessContext:
        return get_window_dpi_awareness_context( hwnd );

    case NtUserCallHwnd_GetWindowInputContext:
        return HandleToUlong( get_window_input_context( hwnd ));

    case NtUserCallHwnd_GetWindowSysSubMenu:
        return HandleToUlong( get_window_sys_sub_menu( hwnd ));

    case NtUserCallHwnd_GetWindowTextLength:
        return get_server_window_text( hwnd, NULL, 0 );

    case NtUserCallHwnd_IsWindow:
        return is_window( hwnd );

    case NtUserCallHwnd_IsWindowEnabled:
        return is_window_enabled( hwnd );

    case NtUserCallHwnd_IsWindowUnicode:
        return is_window_unicode( hwnd );

    case NtUserCallHwnd_IsWindowVisible:
        return is_window_visible( hwnd );

    /* temporary exports */
    case NtUserGetFullWindowHandle:
        return HandleToUlong( get_full_window_handle( hwnd ));

    case NtUserIsCurrentProcessWindow:
        return HandleToUlong( is_current_process_window( hwnd ));

    case NtUserIsCurrentThreadWindow:
        return HandleToUlong( is_current_thread_window( hwnd ));

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
    case NtUserCallHwndParam_ClientToScreen:
        return client_to_screen( hwnd, (POINT *)param );

    case NtUserCallHwndParam_GetChildRect:
        return get_window_rect_rel( hwnd, COORDS_PARENT, (RECT *)param, get_thread_dpi() );

    case NtUserCallHwndParam_GetClassLongA:
        return get_class_long( hwnd, param, TRUE );

    case NtUserCallHwndParam_GetClassLongW:
        return get_class_long( hwnd, param, FALSE );

    case NtUserCallHwndParam_GetClassLongPtrA:
        return get_class_long_ptr( hwnd, param, TRUE );

    case NtUserCallHwndParam_GetClassLongPtrW:
        return get_class_long_ptr( hwnd, param, FALSE );

    case NtUserCallHwndParam_GetClassWord:
        return get_class_word( hwnd, param );

    case NtUserCallHwndParam_GetScrollInfo:
        {
            struct get_scroll_info_params *params = (void *)param;
            return get_scroll_info( hwnd, params->bar, params->info );
        }

    case NtUserCallHwndParam_GetWindowInfo:
        return get_window_info( hwnd, (WINDOWINFO *)param );

    case NtUserCallHwndParam_GetWindowLongA:
        return get_window_long_size( hwnd, param, sizeof(LONG), TRUE );

    case NtUserCallHwndParam_GetWindowLongW:
        return get_window_long( hwnd, param );

    case NtUserCallHwndParam_GetWindowLongPtrA:
        return get_window_long_ptr( hwnd, param, TRUE );

    case NtUserCallHwndParam_GetWindowLongPtrW:
        return get_window_long_ptr( hwnd, param, FALSE );

    case NtUserCallHwndParam_GetWindowRects:
    {
        struct get_window_rects_params *params = (void *)param;
        return params->client ? get_client_rect( hwnd, params->rect, params->dpi )
                              : get_window_rect( hwnd, params->rect, params->dpi );
    }

    case NtUserCallHwndParam_GetWindowRelative:
        return HandleToUlong( get_window_relative( hwnd, param ));

    case NtUserCallHwndParam_GetWindowThread:
        return get_window_thread( hwnd, (DWORD *)param );

    case NtUserCallHwndParam_GetWindowWord:
        return get_window_word( hwnd, param );

    case NtUserCallHwndParam_IsChild:
        return is_child( hwnd, UlongToHandle(param) );

    case NtUserCallHwndParam_MapWindowPoints:
        {
            struct map_window_points_params *params = (void *)param;
            return map_window_points( hwnd, params->hwnd_to, params->points, params->count, params->dpi );
        }

    case NtUserCallHwndParam_MirrorRgn:
        return mirror_window_region( hwnd, UlongToHandle(param) );

    case NtUserCallHwndParam_MonitorFromWindow:
        return HandleToUlong( monitor_from_window( hwnd, param, get_thread_dpi() ));

    case NtUserCallHwndParam_ScreenToClient:
        return screen_to_client( hwnd, (POINT *)param );

    case NtUserCallHwndParam_SetDialogInfo:
        return set_dialog_info( hwnd, (void *)param );

    case NtUserCallHwndParam_SetMDIClientInfo:
        NtUserSetWindowLongPtr( hwnd, sizeof(void *), param, FALSE );
        return win_set_flags( hwnd, WIN_ISMDICLIENT, 0 );

    case NtUserCallHwndParam_SendHardwareInput:
    {
        struct send_hardware_input_params *params = (void *)param;
        return send_hardware_message( hwnd, params->flags, params->input, params->lparam );
    }

    case NtUserCallHwndParam_ExposeWindowSurface:
    {
        struct expose_window_surface_params *params = (void *)param;
        return expose_window_surface( hwnd, params->flags, params->whole ? NULL : &params->rect, params->dpi );
    }

    case NtUserCallHwndParam_GetWinMonitorDpi:
    {
        UINT raw_dpi, dpi = get_win_monitor_dpi( hwnd, &raw_dpi );
        return param == MDT_EFFECTIVE_DPI ? dpi : raw_dpi;
    }

    case NtUserCallHwndParam_SetRawWindowPos:
    {
        struct set_raw_window_pos_params *params = (void *)param;
        return set_raw_window_pos( hwnd, params->rect, params->flags, params->internal );
    }

    default:
        FIXME( "invalid code %u\n", code );
        return 0;
    }
}

/*******************************************************************
 *           NtUserDragDetect (win32u.@)
 *
 * Note that x and y are in the coordinate relative to the upper-left corner of the client area, not
 * in screen coordinates as MSDN claims.
 */
BOOL WINAPI NtUserDragDetect( HWND hwnd, int x, int y )
{
    WORD width, height;
    RECT rect;
    MSG msg;

    TRACE( "%p (%d,%d)\n", hwnd, x, y );

    if (!(NtUserGetKeyState( VK_LBUTTON ) & 0x8000)) return FALSE;

    width  = get_system_metrics( SM_CXDRAG );
    height = get_system_metrics( SM_CYDRAG );
    SetRect( &rect, x - width, y - height, x + width, y + height );

    NtUserSetCapture( hwnd );

    for (;;)
    {
        while (NtUserPeekMessage( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ))
        {
            if (msg.message == WM_LBUTTONUP)
            {
                NtUserReleaseCapture();
                return FALSE;
            }
            if (msg.message == WM_MOUSEMOVE)
            {
                POINT tmp;
                tmp.x = (short)LOWORD( msg.lParam );
                tmp.y = (short)HIWORD( msg.lParam );
                if (!PtInRect( &rect, tmp ))
                {
                    NtUserReleaseCapture();
                    return TRUE;
                }
            }
        }
        NtUserMsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 );
    }
    return FALSE;
}

/*******************************************************************
 *           NtUserDragObject (win32u.@)
 */
DWORD WINAPI NtUserDragObject( HWND parent, HWND hwnd, UINT fmt, ULONG_PTR data, HCURSOR cursor )
{
    FIXME( "%p, %p, %u, %#lx, %p stub!\n", parent, hwnd, fmt, data, cursor );

    return 0;
}


HWND get_shell_window(void)
{
    HWND hwnd = 0;

    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            hwnd = wine_server_ptr_handle( reply->old_shell_window );
    }
    SERVER_END_REQ;

    return hwnd;
}

/*******************************************************************
 *           NtUserQueryWindow (win32u.@)
 */
HANDLE WINAPI NtUserQueryWindow( HWND hwnd, WINDOWINFOCLASS cls )
{
    DWORD pid;
    GUITHREADINFO info;

    switch (cls)
    {
    case WindowProcess:
    case WindowProcess2:
        if (!get_window_thread( hwnd, &pid )) return NULL;
        return UlongToHandle( pid );

    case WindowThread:
        return UlongToHandle( get_window_thread( hwnd, NULL ));

    case WindowActiveWindow:
        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( get_window_thread( hwnd, NULL ), &info );
        return info.hwndActive;

    case WindowFocusWindow:
        info.cbSize = sizeof(info);
        NtUserGetGUIThreadInfo( get_window_thread( hwnd, NULL ), &info );
        return info.hwndFocus;

    case WindowIsHung:
        return UlongToHandle( is_hung_app_window( hwnd ));

    case WindowIsForegroundThread:
        return UlongToHandle( get_window_thread( NtUserGetForegroundWindow(), NULL ) ==
                              get_window_thread( hwnd, NULL ));

    case WindowDefaultImeWindow:
        return get_default_ime_window( hwnd );

    case WindowDefaultInputContext:
        /* FIXME: should get it for the specified window */
        return get_default_input_context();

    case WindowClientBase:
    default:
        WARN( "unsupported class %u\n", cls );
        return 0;
    }
}

/***********************************************************************
*            NtUserSetShellWindowEx (win32u.@)
*/
BOOL WINAPI NtUserSetShellWindowEx( HWND shell, HWND list_view )
{
    BOOL ret;

    /* shell =     Progman[Program Manager]
     *             |-> SHELLDLL_DefView
     * list_view = |   |-> SysListView32
     *             |   |   |-> tooltips_class32
     *             |   |
     *             |   |-> SysHeader32
     *             |
     *             |-> ProxyTarget
     */

    if (get_shell_window())
        return FALSE;

    if (get_window_long( shell, GWL_EXSTYLE ) & WS_EX_TOPMOST)
        return FALSE;

    if (list_view != shell && (get_window_long( list_view, GWL_EXSTYLE ) & WS_EX_TOPMOST))
        return FALSE;

    if (list_view && list_view != shell)
        NtUserSetWindowPos( list_view, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    NtUserSetWindowPos( shell, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags          = SET_DESKTOP_SHELL_WINDOWS;
        req->shell_window   = wine_server_user_handle( shell );
        req->shell_listview = wine_server_user_handle( list_view );
        ret = !wine_server_call_err(req);
    }
    SERVER_END_REQ;
    return ret;
}

HWND get_progman_window(void)
{
    HWND ret = 0;

    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            ret = wine_server_ptr_handle( reply->old_progman_window );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *            NtUserSetProgmanWindow (win32u.@)
 */
HWND WINAPI NtUserSetProgmanWindow( HWND hwnd )
{
    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags          = SET_DESKTOP_PROGMAN_WINDOW;
        req->progman_window = wine_server_user_handle( hwnd );
        if (wine_server_call_err( req )) hwnd = 0;
    }
    SERVER_END_REQ;
    return hwnd;
}

HWND get_taskman_window(void)
{
    HWND ret = 0;

    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags = 0;
        if (!wine_server_call_err(req))
            ret = wine_server_ptr_handle( reply->old_taskman_window );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *            NtUserSetTaskmanWindow (win32u.@)
 */
HWND WINAPI NtUserSetTaskmanWindow( HWND hwnd )
{
    /* hwnd = MSTaskSwWClass
     *        |-> SysTabControl32
     */
    SERVER_START_REQ(set_desktop_shell_windows)
    {
        req->flags          = SET_DESKTOP_TASKMAN_WINDOW;
        req->taskman_window = wine_server_user_handle( hwnd );
        if (wine_server_call_err( req )) hwnd = 0;
    }
    SERVER_END_REQ;
    return hwnd;
}

/***********************************************************************
 *            NtUserIsChildWindowDpiMessageEnabled (win32u.@)
 */
BOOL WINAPI NtUserIsChildWindowDpiMessageEnabled( HWND hwnd )
{
    FIXME( "%p: stub\n", hwnd );
    return FALSE;
}
