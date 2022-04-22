/*
 * Menu functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2009 Alexandre Julliard
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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(menu);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* the accelerator user object */
struct accelerator
{
    struct user_object obj;
    unsigned int       count;
    ACCEL              table[1];
};

/* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

/**********************************************************************
 *           NtUserCopyAcceleratorTable   (win32u.@)
 */
INT WINAPI NtUserCopyAcceleratorTable( HACCEL src, ACCEL *dst, INT count )
{
    struct accelerator *accel;
    int i;

    if (!(accel = get_user_handle_ptr( src, NTUSER_OBJ_ACCEL ))) return 0;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME_(accel)( "other process handle %p?\n", src );
        return 0;
    }
    if (dst)
    {
        if (count > accel->count) count = accel->count;
        for (i = 0; i < count; i++)
        {
            dst[i].fVirt = accel->table[i].fVirt & 0x7f;
            dst[i].key   = accel->table[i].key;
            dst[i].cmd   = accel->table[i].cmd;
        }
    }
    else count = accel->count;
    release_user_handle_ptr( accel );
    return count;
}

/*********************************************************************
 *           NtUserCreateAcceleratorTable   (win32u.@)
 */
HACCEL WINAPI NtUserCreateAcceleratorTable( ACCEL *table, INT count )
{
    struct accelerator *accel;
    HACCEL handle;

    if (count < 1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    accel = malloc( FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    memcpy( accel->table, table, count * sizeof(*table) );

    if (!(handle = alloc_user_handle( &accel->obj, NTUSER_OBJ_ACCEL ))) free( accel );
    TRACE_(accel)("returning %p\n", handle );
    return handle;
}

/******************************************************************************
 *           NtUserDestroyAcceleratorTable   (win32u.@)
 */
BOOL WINAPI NtUserDestroyAcceleratorTable( HACCEL handle )
{
    struct accelerator *accel;

    if (!(accel = free_user_handle( handle, NTUSER_OBJ_ACCEL ))) return FALSE;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME_(accel)( "other process handle %p\n", accel );
        return FALSE;
    }
    free( accel );
    return TRUE;
}

static POPUPMENU *grab_menu_ptr( HMENU handle )
{
    POPUPMENU *menu = get_user_handle_ptr( handle, NTUSER_OBJ_MENU );

    if (menu == OBJ_OTHER_PROCESS)
    {
        WARN( "other process menu %p\n", handle );
        return NULL;
    }

    if (menu)
        menu->refcount++;
    else
        WARN( "invalid menu handle=%p\n", handle );
    return menu;
}

static void release_menu_ptr( POPUPMENU *menu )
{
    if (menu)
    {
        menu->refcount--;
        release_user_handle_ptr( menu );
    }
}

/* see IsMenu */
static BOOL is_menu( HMENU handle )
{
    POPUPMENU *menu;
    BOOL is_menu;

    menu = grab_menu_ptr( handle );
    is_menu = menu != NULL;
    release_menu_ptr( menu );

    if (!is_menu) SetLastError( ERROR_INVALID_MENU_HANDLE );
    return is_menu;
}

static POPUPMENU *find_menu_item( HMENU handle, UINT id, UINT flags, UINT *pos )
{
    UINT fallback_pos = ~0u, i;
    POPUPMENU *menu;

    menu = grab_menu_ptr( handle );
    if (!menu)
        return NULL;

    if (flags & MF_BYPOSITION)
    {
        if (id >= menu->nItems)
        {
            release_menu_ptr( menu );
            return NULL;
        }

        if (pos) *pos = id;
        return menu;
    }
    else
    {
        MENUITEM *item = menu->items;
        for (i = 0; i < menu->nItems; i++, item++)
        {
            if (item->fType & MF_POPUP)
            {
                POPUPMENU *submenu = find_menu_item( item->hSubMenu, id, flags, pos );

                if (submenu)
                {
                    release_menu_ptr( menu );
                    return submenu;
                }
                else if (item->wID == id)
                {
                    /* fallback to this item if nothing else found */
                    fallback_pos = i;
                }
            }
            else if (item->wID == id)
            {
                if (pos) *pos = i;
                return menu;
            }
        }
    }

    if (fallback_pos != ~0u)
        *pos = fallback_pos;
    else
    {
        release_menu_ptr( menu );
        menu = NULL;
    }

    return menu;
}

static BOOL is_win_menu_disallowed( HWND hwnd )
{
    return (get_window_long(hwnd, GWL_STYLE) & (WS_CHILD | WS_POPUP)) == WS_CHILD;
}

/* see GetMenu */
HMENU get_menu( HWND hwnd )
{
    return UlongToHandle( get_window_long( hwnd, GWLP_ID ));
}

/* see CreateMenu and CreatePopupMenu */
HMENU create_menu( BOOL is_popup )
{
    POPUPMENU *menu;
    HMENU handle;

    if (!(menu = calloc( 1, sizeof(*menu) ))) return 0;
    menu->FocusedItem = NO_SELECTED_ITEM;
    menu->refcount = 1;
    if (is_popup) menu->wFlags |= MF_POPUP;

    if (!(handle = alloc_user_handle( &menu->obj, NTUSER_OBJ_MENU ))) free( menu );

    TRACE( "return %p\n", handle );
    return handle;
}

/**********************************************************************
 *         NtUserDestroyMenu   (win32u.@)
 */
BOOL WINAPI NtUserDestroyMenu( HMENU handle )
{
    POPUPMENU *menu;

    TRACE( "(%p)\n", handle );

    if (!(menu = free_user_handle( handle, NTUSER_OBJ_MENU ))) return FALSE;
    if (menu == OBJ_OTHER_PROCESS) return FALSE;

    /* DestroyMenu should not destroy system menu popup owner */
    if ((menu->wFlags & (MF_POPUP | MF_SYSMENU)) == MF_POPUP && menu->hWnd)
    {
        NtUserDestroyWindow( menu->hWnd );
        menu->hWnd = 0;
    }

    if (menu->items && user_callbacks) /* recursively destroy submenus */
        user_callbacks->free_menu_items( menu );

    free( menu );
    return TRUE;
}

/*******************************************************************
 *           NtUserSetSystemMenu    (win32u.@)
 */
BOOL WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    return user_callbacks && user_callbacks->pSetSystemMenu( hwnd, menu );
}

/*******************************************************************
 *           set_window_menu
 *
 * Helper for NtUserSetMenu that does not call NtUserSetWindowPos.
 */
BOOL set_window_menu( HWND hwnd, HMENU handle )
{
    TRACE( "(%p, %p);\n", hwnd, handle );

    if (handle && !is_menu( handle ))
    {
        WARN( "%p is not a menu handle\n", handle );
        return FALSE;
    }

    if (is_win_menu_disallowed( hwnd ))
        return FALSE;

    hwnd = get_full_window_handle( hwnd );
    if (get_capture() == hwnd)
        set_capture_window( 0, GUI_INMENUMODE, NULL );  /* release the capture */

    if (handle)
    {
        POPUPMENU *menu;

        if (!(menu = grab_menu_ptr( handle ))) return FALSE;
        menu->hWnd = hwnd;
        menu->Height = 0;  /* Make sure we recalculate the size */
        release_menu_ptr(menu);
    }

    NtUserSetWindowLong( hwnd, GWLP_ID, (LONG_PTR)handle, FALSE );
    return TRUE;
}

/**********************************************************************
 *           NtUserSetMenu    (win32u.@)
 */
BOOL WINAPI NtUserSetMenu( HWND hwnd, HMENU menu )
{
    if (!set_window_menu( hwnd, menu ))
        return FALSE;

    NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    return TRUE;
}

/*******************************************************************
 *           NtUserCheckMenuItem    (win32u.@)
 */
DWORD WINAPI NtUserCheckMenuItem( HMENU handle, UINT id, UINT flags )
{
    POPUPMENU *menu;
    MENUITEM *item;
    DWORD ret;
    UINT pos;

    if (!(menu = find_menu_item(handle, id, flags, &pos)))
        return -1;
    item = &menu->items[pos];

    ret = item->fState & MF_CHECKED;
    if (flags & MF_CHECKED) item->fState |= MF_CHECKED;
    else item->fState &= ~MF_CHECKED;
    release_menu_ptr(menu);
    return ret;
}

/**********************************************************************
 *           NtUserEnableMenuItem    (win32u.@)
 */
BOOL WINAPI NtUserEnableMenuItem( HMENU handle, UINT id, UINT flags )
{
    UINT oldflags, pos;
    POPUPMENU *menu;
    MENUITEM *item;

    TRACE( "(%p, %04x, %04x)\n", handle, id, flags );

    /* Get the Popupmenu to access the owner menu */
    if (!(menu = find_menu_item( handle, id, flags, &pos )))
	return ~0u;

    item = &menu->items[pos];
    oldflags = item->fState & (MF_GRAYED | MF_DISABLED);
    item->fState ^= (oldflags ^ flags) & (MF_GRAYED | MF_DISABLED);

    /* If the close item in the system menu change update the close button */
    if (item->wID == SC_CLOSE && oldflags != flags && menu->hSysMenuOwner)
    {
        POPUPMENU *parent_menu;
        RECT rc;
        HWND hwnd;

        /* Get the parent menu to access */
        parent_menu = grab_menu_ptr( menu->hSysMenuOwner );
        release_menu_ptr( menu );
        if (!parent_menu)
            return ~0u;

        hwnd = parent_menu->hWnd;
        release_menu_ptr( parent_menu );

        /* Refresh the frame to reflect the change */
        get_window_rects( hwnd, COORDS_CLIENT, &rc, NULL, get_thread_dpi() );
        rc.bottom = 0;
        NtUserRedrawWindow( hwnd, &rc, 0, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN );
    }
    else
        release_menu_ptr( menu );

    return oldflags;
}

/* see DrawMenuBar */
BOOL draw_menu_bar( HWND hwnd )
{
    HMENU handle;

    if (!is_window( hwnd )) return FALSE;
    if (is_win_menu_disallowed( hwnd )) return TRUE;

    if ((handle = get_menu( hwnd )))
    {
        POPUPMENU *menu = grab_menu_ptr( handle );
        if (menu)
        {
            menu->Height = 0; /* Make sure we call MENU_MenuBarCalcSize */
            menu->hwndOwner = hwnd;
            release_menu_ptr( menu );
        }
    }

    return NtUserSetWindowPos( hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
}

/**********************************************************************
 *           NtUserGetMenuItemRect    (win32u.@)
 */
BOOL WINAPI NtUserGetMenuItemRect( HWND hwnd, HMENU handle, UINT item, RECT *rect )
{
    POPUPMENU *menu;
    UINT pos;
    RECT window_rect;

    TRACE( "(%p,%p,%d,%p)\n", hwnd, handle, item, rect );

    if (!rect)
        return FALSE;

    if (!(menu = find_menu_item( handle, item, MF_BYPOSITION, &pos )))
        return FALSE;

    if (!hwnd) hwnd = menu->hWnd;
    if (!hwnd)
    {
        release_menu_ptr( menu );
        return FALSE;
    }

    *rect = menu->items[pos].rect;
    OffsetRect( rect, menu->items_rect.left, menu->items_rect.top );

    /* Popup menu item draws in the client area */
    if (menu->wFlags & MF_POPUP) map_window_points( hwnd, 0, (POINT *)rect, 2, get_thread_dpi() );
    else
    {
        /* Sysmenu draws in the non-client area */
        get_window_rect( hwnd, &window_rect, get_thread_dpi() );
        OffsetRect( rect, window_rect.left, window_rect.top );
    }

    release_menu_ptr(menu);
    return TRUE;
}

static BOOL set_menu_info( HMENU handle, const MENUINFO *info )
{
    POPUPMENU *menu;

    if (!(menu = grab_menu_ptr( handle ))) return FALSE;

    if (info->fMask & MIM_BACKGROUND) menu->hbrBack = info->hbrBack;
    if (info->fMask & MIM_HELPID)     menu->dwContextHelpID = info->dwContextHelpID;
    if (info->fMask & MIM_MAXHEIGHT)  menu->cyMax = info->cyMax;
    if (info->fMask & MIM_MENUDATA)   menu->dwMenuData = info->dwMenuData;
    if (info->fMask & MIM_STYLE)      menu->dwStyle = info->dwStyle;

    if (info->fMask & MIM_APPLYTOSUBMENUS)
    {
        int i;
        MENUITEM *item = menu->items;
        for (i = menu->nItems; i; i--, item++)
            if (item->fType & MF_POPUP)
                set_menu_info( item->hSubMenu, info);
    }

    release_menu_ptr( menu );
    return TRUE;
}

/**********************************************************************
 *           NtUserThunkedMenuInfo    (win32u.@)
 */
BOOL WINAPI NtUserThunkedMenuInfo( HMENU menu, const MENUINFO *info )
{
    TRACE( "(%p %p)\n", menu, info );

    if (!info)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    if (!set_menu_info( menu, info ))
    {
        SetLastError( ERROR_INVALID_MENU_HANDLE );
        return FALSE;
    }

    if (info->fMask & MIM_STYLE)
    {
        if (info->dwStyle & MNS_AUTODISMISS) FIXME("MNS_AUTODISMISS unimplemented\n");
        if (info->dwStyle & MNS_DRAGDROP) FIXME("MNS_DRAGDROP unimplemented\n");
        if (info->dwStyle & MNS_MODELESS) FIXME("MNS_MODELESS unimplemented\n");
    }
    return TRUE;
}

/* see GetMenuInfo */
BOOL get_menu_info( HMENU handle, MENUINFO *info )
{
    POPUPMENU *menu;

    TRACE( "(%p %p)\n", handle, info );

    if (!info || info->cbSize != sizeof(MENUINFO) || !(menu = grab_menu_ptr( handle )))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (info->fMask & MIM_BACKGROUND) info->hbrBack = menu->hbrBack;
    if (info->fMask & MIM_HELPID)     info->dwContextHelpID = menu->dwContextHelpID;
    if (info->fMask & MIM_MAXHEIGHT)  info->cyMax = menu->cyMax;
    if (info->fMask & MIM_MENUDATA)   info->dwMenuData = menu->dwMenuData;
    if (info->fMask & MIM_STYLE)      info->dwStyle = menu->dwStyle;

    release_menu_ptr(menu);
    return TRUE;
}

/**********************************************************************
 *           NtUserSetMenuContextHelpId    (win32u.@)
 */
BOOL WINAPI NtUserSetMenuContextHelpId( HMENU handle, DWORD id )
{
    POPUPMENU *menu;

    TRACE( "(%p 0x%08x)\n", handle, id );

    if (!(menu = grab_menu_ptr( handle ))) return FALSE;
    menu->dwContextHelpID = id;
    release_menu_ptr( menu );
    return TRUE;
}

/**********************************************************************
 *           NtUserSetMenuDefaultItem    (win32u.@)
 */
BOOL WINAPI NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos )
{
    MENUITEM *menu_item;
    POPUPMENU *menu;
    unsigned int i;
    BOOL ret = FALSE;

    TRACE( "(%p,%d,%d)\n", handle, item, bypos );

    if (!(menu = grab_menu_ptr( handle ))) return FALSE;

    /* reset all default-item flags */
    menu_item = menu->items;
    for (i = 0; i < menu->nItems; i++, menu_item++)
    {
        menu_item->fState &= ~MFS_DEFAULT;
    }

    if (item != -1)
    {
        menu_item = menu->items;

        if (bypos)
        {
            ret = item < menu->nItems;
            if (ret) menu->items[item].fState |= MFS_DEFAULT;
        }
        else
        {
            for (i = 0; i < menu->nItems; i++)
            {
                if (menu->items[i].wID == item)
                {
                    menu->items[i].fState |= MFS_DEFAULT;
                    ret = TRUE;
                }
            }
        }
    }
    else ret = TRUE;

    release_menu_ptr( menu );
    return ret;
}
