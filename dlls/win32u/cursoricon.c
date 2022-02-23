/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Martin Von Loewis
 * Copyright 1997 Alex Korobka
 * Copyright 1998 Turchanov Sergey
 * Copyright 2007 Henri Verbeet
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2016 Dmitry Timoshkov
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
#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);

static struct list icon_cache = LIST_INIT( icon_cache );

static struct cursoricon_object *get_icon_ptr( HICON handle )
{
    struct cursoricon_object *obj = get_user_handle_ptr( handle, NTUSER_OBJ_ICON );
    if (obj == OBJ_OTHER_PROCESS)
    {
        WARN( "icon handle %p from other process\n", handle );
        obj = NULL;
    }
    return obj;
}

/***********************************************************************
 *	     NtUserShowCursor    (win32u.@)
 */
INT WINAPI NtUserShowCursor( BOOL show )
{
    HCURSOR cursor;
    int increment = show ? 1 : -1;
    int count;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_COUNT;
        req->show_count = increment;
        wine_server_call( req );
        cursor = wine_server_ptr_handle( reply->prev_handle );
        count = reply->prev_count + increment;
    }
    SERVER_END_REQ;

    TRACE("%d, count=%d\n", show, count );

    if (show && !count) user_driver->pSetCursor( cursor );
    else if (!show && count == -1) user_driver->pSetCursor( 0 );

    return count;
}

/***********************************************************************
 *	     NtUserSetCursor (win32u.@)
 */
HCURSOR WINAPI NtUserSetCursor( HCURSOR cursor )
{
    struct cursoricon_object *obj;
    HCURSOR old_cursor;
    int show_count;
    BOOL ret;

    TRACE( "%p\n", cursor );

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_HANDLE;
        req->handle = wine_server_user_handle( cursor );
        if ((ret = !wine_server_call_err( req )))
        {
            old_cursor = wine_server_ptr_handle( reply->prev_handle );
            show_count = reply->prev_count;
        }
    }
    SERVER_END_REQ;
    if (!ret) return 0;

    user_driver->pSetCursor( show_count >= 0 ? cursor : 0 );

    if (!(obj = get_icon_ptr( old_cursor ))) return 0;
    release_user_handle_ptr( obj );
    return old_cursor;
}

/***********************************************************************
 *	     NtUserGetCursor (win32u.@)
 */
HCURSOR WINAPI NtUserGetCursor(void)
{
    HCURSOR ret;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = 0;
        wine_server_call( req );
        ret = wine_server_ptr_handle( reply->prev_handle );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *	     NtUserClipCursor (win32u.@)
 */
BOOL WINAPI NtUserClipCursor( const RECT *rect )
{
    UINT dpi;
    BOOL ret;
    RECT new_rect;

    TRACE( "Clipping to %s\n", wine_dbgstr_rect(rect) );

    if (rect)
    {
        if (rect->left > rect->right || rect->top > rect->bottom) return FALSE;
        if ((dpi = get_thread_dpi()))
        {
            HMONITOR monitor = monitor_from_rect( rect, MONITOR_DEFAULTTOPRIMARY, dpi );
            new_rect = map_dpi_rect( *rect, dpi, get_monitor_dpi( monitor ));
            rect = &new_rect;
        }
    }

    SERVER_START_REQ( set_cursor )
    {
        req->clip_msg = WM_WINE_CLIPCURSOR;
        if (rect)
        {
            req->flags       = SET_CURSOR_CLIP;
            req->clip.left   = rect->left;
            req->clip.top    = rect->top;
            req->clip.right  = rect->right;
            req->clip.bottom = rect->bottom;
        }
        else req->flags = SET_CURSOR_NOCLIP;

        if ((ret = !wine_server_call( req )))
        {
            new_rect.left   = reply->new_clip.left;
            new_rect.top    = reply->new_clip.top;
            new_rect.right  = reply->new_clip.right;
            new_rect.bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;
    if (ret) user_driver->pClipCursor( &new_rect );
    return ret;
}

BOOL get_clip_cursor( RECT *rect )
{
    UINT dpi;
    BOOL ret;

    if (!rect) return FALSE;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = 0;
        if ((ret = !wine_server_call( req )))
        {
            rect->left   = reply->new_clip.left;
            rect->top    = reply->new_clip.top;
            rect->right  = reply->new_clip.right;
            rect->bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;

    if (ret && (dpi = get_thread_dpi()))
    {
        HMONITOR monitor = monitor_from_rect( rect, MONITOR_DEFAULTTOPRIMARY, 0 );
        *rect = map_dpi_rect( *rect, get_monitor_dpi( monitor ), dpi );
    }
    return ret;
}

HICON alloc_cursoricon_handle( BOOL is_icon )
{
    struct cursoricon_object *obj;
    HICON handle;

    if (!(obj = calloc( 1, sizeof(*obj) ))) return NULL;
    obj->is_icon = is_icon;
    if (!(handle = alloc_user_handle( &obj->obj, NTUSER_OBJ_ICON ))) free( obj );
    return handle;
}

static BOOL free_icon_handle( HICON handle )
{
    struct cursoricon_object *obj = free_user_handle( handle, NTUSER_OBJ_ICON );

    if (obj == OBJ_OTHER_PROCESS) WARN( "icon handle %p from other process\n", handle );
    else if (obj)
    {
        ULONG param = obj->param;
        void *ret_ptr;
        ULONG ret_len;
        UINT i;

        assert( !obj->rsrc );  /* shared icons can't be freed */

        if (!obj->is_ani)
        {
            if (obj->frame.alpha) NtGdiDeleteObjectApp( obj->frame.alpha );
            if (obj->frame.color) NtGdiDeleteObjectApp( obj->frame.color );
            if (obj->frame.mask)  NtGdiDeleteObjectApp( obj->frame.mask );
        }
        else
        {
            for (i = 0; i < obj->ani.num_steps; i++)
            {
                HICON hFrame = obj->ani.frames[i];

                if (hFrame)
                {
                    UINT j;

                    free_icon_handle( obj->ani.frames[i] );
                    for (j = 0; j < obj->ani.num_steps; j++)
                    {
                        if (obj->ani.frames[j] == hFrame) obj->ani.frames[j] = 0;
                    }
                }
            }
            free( obj->ani.frames );
        }
        if (!IS_INTRESOURCE( obj->resname )) free( obj->resname );
        free( obj );
        if (param) KeUserModeCallback( NtUserCallFreeIcon, &param, sizeof(param), &ret_ptr, &ret_len );
        user_driver->pDestroyCursorIcon( handle );
        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *	     NtUserDestroyCursor (win32u.@)
 */
BOOL WINAPI NtUserDestroyCursor( HCURSOR cursor, ULONG arg )
{
    struct cursoricon_object *obj;
    BOOL shared, ret;

    TRACE( "%p\n", cursor );

    if (!(obj = get_icon_ptr( cursor ))) return FALSE;
    shared = obj->is_shared;
    release_user_handle_ptr( obj );
    ret = NtUserGetCursor() != cursor;
    if (!shared) free_icon_handle( cursor );
    return ret;
}

/***********************************************************************
 *	     NtUserSetCursorIconData (win32u.@)
 */
BOOL WINAPI NtUserSetCursorIconData( HCURSOR cursor, UNICODE_STRING *module, UNICODE_STRING *res_name,
                                     struct cursoricon_desc *desc )
{
    struct cursoricon_object *obj;
    UINT i, j;

    if (!(obj = get_icon_ptr( cursor ))) return FALSE;

    if (obj->is_ani || obj->frame.width)
    {
        /* already initialized */
        release_user_handle_ptr( obj );
        SetLastError( ERROR_INVALID_CURSOR_HANDLE );
        return FALSE;
    }

    obj->delay = desc->delay;

    if (desc->num_steps)
    {
        if (!(obj->ani.frames = calloc( desc->num_steps, sizeof(*obj->ani.frames) )))
        {
            release_user_handle_ptr( obj );
            return FALSE;
        }
        obj->is_ani = TRUE;
        obj->ani.num_steps  = desc->num_steps;
        obj->ani.num_frames = desc->num_frames;
    }
    else obj->frame = desc->frames[0];

    if (!res_name)
        obj->resname = NULL;
    else if (res_name->Length)
    {
        obj->resname = malloc( res_name->Length + sizeof(WCHAR) );
        if (obj->resname)
        {
            memcpy( obj->resname, res_name->Buffer, res_name->Length );
            obj->resname[res_name->Length / sizeof(WCHAR)] = 0;
        }
    }
    else
        obj->resname = MAKEINTRESOURCEW( LOWORD(res_name->Buffer) );

    if (module && module->Length && (obj->module.Buffer = malloc( module->Length )))
    {
        memcpy( obj->module.Buffer, module->Buffer, module->Length );
        obj->module.Length = module->Length;
    }

    if (obj->is_ani)
    {
        /* Setup the animated frames in the correct sequence */
        for (i = 0; i < desc->num_steps; i++)
        {
            struct cursoricon_desc frame_desc;
            DWORD frame_id;

            if (obj->ani.frames[i]) continue; /* already set */

            frame_id = desc->frame_seq ? desc->frame_seq[i] : i;
            if (frame_id >= obj->ani.num_frames)
            {
                frame_id = obj->ani.num_frames - 1;
                ERR_(cursor)( "Sequence indicates frame past end of list, corrupt?\n" );
            }
            memset( &frame_desc, 0, sizeof(frame_desc) );
            frame_desc.delay  = desc->frame_rates ? desc->frame_rates[i] : desc->delay;
            frame_desc.frames = &desc->frames[frame_id];
            if (!(obj->ani.frames[i] = alloc_cursoricon_handle( obj->is_icon )) ||
                !NtUserSetCursorIconData( obj->ani.frames[i], NULL, NULL, &frame_desc ))
            {
                release_user_handle_ptr( obj );
                return 0;
            }

            if (desc->frame_seq)
            {
                for (j = i + 1; j < obj->ani.num_steps; j++)
                {
                    if (desc->frame_seq[j] == frame_id) obj->ani.frames[j] = obj->ani.frames[i];
                }
            }
        }
    }

    if (desc->flags & LR_SHARED)
    {
        obj->is_shared = TRUE;
        if (obj->module.Length)
        {
            obj->rsrc = desc->rsrc;
            list_add_head( &icon_cache, &obj->entry );
        }
    }

    release_user_handle_ptr( obj );
    return TRUE;
}

/***********************************************************************
 *	     NtUserFindExistingCursorIcon (win32u.@)
 */
HICON WINAPI NtUserFindExistingCursorIcon( UNICODE_STRING *module, UNICODE_STRING *res_name, void *desc )
{
    struct cursoricon_object *ptr;
    HICON ret = 0;

    user_lock();
    LIST_FOR_EACH_ENTRY( ptr, &icon_cache, struct cursoricon_object, entry )
    {
        if (ptr->module.Length != module->Length) continue;
        if (memcmp( ptr->module.Buffer, module->Buffer, module->Length )) continue;
        /* We pass rsrc as desc argument, this is not compatible with Windows */
        if (ptr->rsrc != desc) continue;
        ret = ptr->obj.handle;
        break;
    }
    user_unlock();
    return ret;
}
