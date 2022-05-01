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

/* maximum allowed depth of any branch in the menu tree.
 * This value is slightly larger than in windows (25) to
 * stay on the safe side. */
#define MAXMENUDEPTH 30

/* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)
#define IS_MAGIC_BITMAP(id)     ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define MENUITEMINFO_TYPE_MASK                                          \
    (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR |          \
     MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK |                \
     MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )
#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)
#define STATE_MASK (~TYPE_MASK)
#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

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

#define MENUFLAG(bit,text) \
  do { \
      if (flags & (bit)) { flags &= ~(bit); strcat(buf, (text)); } \
  } while (0)

static const char *debugstr_menuitem( const MENUITEM *item )
{
    static const char *const hbmmenus[] = { "HBMMENU_CALLBACK", "", "HBMMENU_SYSTEM",
        "HBMMENU_MBAR_RESTORE", "HBMMENU_MBAR_MINIMIZE", "UNKNOWN BITMAP", "HBMMENU_MBAR_CLOSE",
        "HBMMENU_MBAR_CLOSE_D", "HBMMENU_MBAR_MINIMIZE_D", "HBMMENU_POPUP_CLOSE",
        "HBMMENU_POPUP_RESTORE", "HBMMENU_POPUP_MAXIMIZE", "HBMMENU_POPUP_MINIMIZE" };
    char buf[256];
    UINT flags;

    if (!item) return "NULL";

    sprintf( buf, "{ ID=0x%lx", item->wID );
    if (item->hSubMenu) sprintf( buf + strlen(buf), ", Sub=%p", item->hSubMenu );

    flags = item->fType;
    if (flags)
    {
        strcat( buf, ", fType=" );
        MENUFLAG( MFT_SEPARATOR, "sep" );
        MENUFLAG( MFT_OWNERDRAW, "own" );
        MENUFLAG( MFT_BITMAP, "bit" );
        MENUFLAG( MF_POPUP, "pop" );
        MENUFLAG( MFT_MENUBARBREAK, "barbrk" );
        MENUFLAG( MFT_MENUBREAK, "brk");
        MENUFLAG( MFT_RADIOCHECK, "radio" );
        MENUFLAG( MFT_RIGHTORDER, "rorder" );
        MENUFLAG( MF_SYSMENU, "sys" );
        MENUFLAG( MFT_RIGHTJUSTIFY, "right" );  /* same as MF_HELP */
        if (flags) sprintf( buf + strlen(buf), "+0x%x", flags );
    }

    flags = item->fState;
    if (flags)
    {
        strcat( buf, ", State=" );
        MENUFLAG( MFS_GRAYED, "grey" );
        MENUFLAG( MFS_DEFAULT, "default" );
        MENUFLAG( MFS_DISABLED, "dis" );
        MENUFLAG( MFS_CHECKED, "check" );
        MENUFLAG( MFS_HILITE, "hi" );
        MENUFLAG( MF_USECHECKBITMAPS, "usebit" );
        MENUFLAG( MF_MOUSESELECT, "mouse" );
        if (flags) sprintf( buf + strlen(buf), "+0x%x", flags );
    }

    if (item->hCheckBit)   sprintf( buf + strlen(buf), ", Chk=%p", item->hCheckBit );
    if (item->hUnCheckBit) sprintf( buf + strlen(buf), ", Unc=%p", item->hUnCheckBit );
    if (item->text)        sprintf( buf + strlen(buf), ", Text=%s", debugstr_w(item->text) );
    if (item->dwItemData)  sprintf( buf + strlen(buf), ", ItemData=0x%08lx", item->dwItemData );

    if (item->hbmpItem)
    {
        if (IS_MAGIC_BITMAP( item->hbmpItem ))
            sprintf( buf + strlen(buf), ", hbitmap=%s", hbmmenus[(INT_PTR)item->hbmpItem + 1] );
        else
            sprintf( buf + strlen(buf), ", hbitmap=%p", item->hbmpItem );
    }
    return wine_dbg_sprintf( "%s  }", buf );
}

#undef MENUFLAG

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

/***********************************************************************
 *           get_win_sys_menu
 *
 * Get the system menu of a window
 */
static HMENU get_win_sys_menu( HWND hwnd )
{
    HMENU ret = 0;
    WND *win = get_win_ptr( hwnd );
    if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
    {
        ret = win->hSysMenu;
        release_win_ptr( win );
    }
    return ret;
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

static POPUPMENU *insert_menu_item( HMENU handle, UINT id, UINT flags, UINT *ret_pos )
{
    MENUITEM *new_items;
    POPUPMENU *menu;
    UINT pos = id;

    /* Find where to insert new item */
    if (!(menu = find_menu_item(handle, id, flags, &pos)))
    {
        if (!(menu = grab_menu_ptr(handle)))
            return NULL;
        pos = menu->nItems;
    }

    /* Make sure that MDI system buttons stay on the right side.
     * Note: XP treats only bitmap handles 1 - 6 as "magic" ones
     * regardless of their id.
     */
    while (pos > 0 && (INT_PTR)menu->items[pos - 1].hbmpItem >= (INT_PTR)HBMMENU_SYSTEM &&
           (INT_PTR)menu->items[pos - 1].hbmpItem <= (INT_PTR)HBMMENU_MBAR_CLOSE_D)
        pos--;

    TRACE( "inserting at %u flags %x\n", pos, flags );

    new_items = malloc( sizeof(MENUITEM) * (menu->nItems + 1) );
    if (!new_items)
    {
        release_menu_ptr( menu );
        return NULL;
    }
    if (menu->nItems > 0)
    {
        /* Copy the old array into the new one */
        if (pos > 0) memcpy( new_items, menu->items, pos * sizeof(MENUITEM) );
        if (pos < menu->nItems) memcpy( &new_items[pos + 1], &menu->items[pos],
                                        (menu->nItems - pos) * sizeof(MENUITEM) );
        free( menu->items );
    }
    menu->items = new_items;
    menu->nItems++;
    memset( &new_items[pos], 0, sizeof(*new_items) );
    menu->Height = 0; /* force size recalculate */

    *ret_pos = pos;
    return menu;
}

static BOOL is_win_menu_disallowed( HWND hwnd )
{
    return (get_window_long(hwnd, GWL_STYLE) & (WS_CHILD | WS_POPUP)) == WS_CHILD;
}

/***********************************************************************
 *           find_submenu
 *
 * Find a Sub menu. Return the position of the submenu, and modifies
 * *hmenu in case it is found in another sub-menu.
 * If the submenu cannot be found, NO_SELECTED_ITEM is returned.
 */
static UINT find_submenu( HMENU *handle_ptr, HMENU target )
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT i;

    if (*handle_ptr == (HMENU)0xffff || !(menu = grab_menu_ptr( *handle_ptr )))
        return NO_SELECTED_ITEM;

    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
        if(!(item->fType & MF_POPUP)) continue;
        if (item->hSubMenu == target)
        {
            release_menu_ptr( menu );
            return i;
        }
        else
        {
            HMENU hsubmenu = item->hSubMenu;
            UINT pos = find_submenu( &hsubmenu, target );
            if (pos != NO_SELECTED_ITEM)
            {
                *handle_ptr = hsubmenu;
                release_menu_ptr( menu );
                return pos;
            }
        }
    }

    release_menu_ptr( menu );
    return NO_SELECTED_ITEM;
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

    /* recursively destroy submenus */
    if (menu->items)
    {
        MENUITEM *item = menu->items;
        int i;

        for (i = menu->nItems; i > 0; i--, item++)
        {
            if (item->fType & MF_POPUP) NtUserDestroyMenu( item->hSubMenu );
            free( item->text );
        }
        free( menu->items );
    }

    free( menu );
    return TRUE;
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
 *           menu_depth
 *
 * detect if there are loops in the menu tree (or the depth is too large)
 */
static int menu_depth( POPUPMENU *pmenu, int depth)
{
    int i, subdepth;
    MENUITEM *item;

    if (++depth > MAXMENUDEPTH) return depth;
    item = pmenu->items;
    subdepth = depth;
    for (i = 0; i < pmenu->nItems && subdepth <= MAXMENUDEPTH; i++, item++)
    {
        POPUPMENU *submenu = item->hSubMenu ? grab_menu_ptr( item->hSubMenu ) : NULL;
        if (submenu)
        {
            int bdepth = menu_depth( submenu, depth);
            if (bdepth > subdepth) subdepth = bdepth;
            release_menu_ptr( submenu );
        }
        if (subdepth > MAXMENUDEPTH)
            TRACE( "<- hmenu %p\n", item->hSubMenu );
    }

    return subdepth;
}

static BOOL set_menu_item_info( MENUITEM *menu, const MENUITEMINFOW *info )
{
    if (!menu) return FALSE;

    TRACE( "%s\n", debugstr_menuitem( menu ));

    if (info->fMask & MIIM_FTYPE )
    {
        menu->fType &= ~MENUITEMINFO_TYPE_MASK;
        menu->fType |= info->fType & MENUITEMINFO_TYPE_MASK;
    }
    if (info->fMask & MIIM_STRING )
    {
        const WCHAR *text = info->dwTypeData;
        /* free the string when used */
        free( menu->text );
        if (!text)
            menu->text = NULL;
        else if ((menu->text = malloc( (lstrlenW(text) + 1) * sizeof(WCHAR) )))
            lstrcpyW( menu->text, text );
    }

    if (info->fMask & MIIM_STATE)
         /* Other menu items having MFS_DEFAULT are not converted
           to normal items */
         menu->fState = info->fState & MENUITEMINFO_STATE_MASK;

    if (info->fMask & MIIM_ID)
        menu->wID = info->wID;

    if (info->fMask & MIIM_SUBMENU)
    {
        menu->hSubMenu = info->hSubMenu;
        if (menu->hSubMenu)
        {
            POPUPMENU *submenu = grab_menu_ptr( menu->hSubMenu );
            if (!submenu)
            {
                SetLastError( ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            if (menu_depth( submenu, 0 ) > MAXMENUDEPTH)
            {
                ERR( "Loop detected in menu hierarchy or maximum menu depth exceeded\n" );
                menu->hSubMenu = 0;
                release_menu_ptr( submenu );
                return FALSE;
            }
            submenu->wFlags |= MF_POPUP;
            menu->fType |= MF_POPUP;
            release_menu_ptr( submenu );
        }
        else
            menu->fType &= ~MF_POPUP;
    }

    if (info->fMask & MIIM_CHECKMARKS)
    {
        menu->hCheckBit = info->hbmpChecked;
        menu->hUnCheckBit = info->hbmpUnchecked;
    }
    if (info->fMask & MIIM_DATA)
        menu->dwItemData = info->dwItemData;

    if (info->fMask & MIIM_BITMAP)
        menu->hbmpItem = info->hbmpItem;

    if (!menu->text && !(menu->fType & MFT_OWNERDRAW) && !menu->hbmpItem)
        menu->fType |= MFT_SEPARATOR;

    TRACE( "to: %s\n", debugstr_menuitem( menu ));
    return TRUE;
}

/* see GetMenuState */
static UINT get_menu_state( HMENU handle, UINT item_id, UINT flags )
{
    POPUPMENU *menu;
    UINT state, pos;
    MENUITEM *item;

    TRACE( "(menu=%p, id=%04x, flags=%04x);\n", handle, item_id, flags );

    if (!(menu = find_menu_item( handle, item_id, flags, &pos )))
        return -1;

    item = &menu->items[pos];
    TRACE( "  item: %s\n", debugstr_menuitem( item ));
    if (item->fType & MF_POPUP)
    {
        POPUPMENU *submenu = grab_menu_ptr( item->hSubMenu );
        if (submenu)
            state = (submenu->nItems << 8) | ((item->fState | item->fType) & 0xff);
        else
            state = -1;
        release_menu_ptr( submenu );
    }
    else
    {
        state = item->fType | item->fState;
    }
    release_menu_ptr(menu);
    return state;
}

/**********************************************************************
 *           NtUserThunkedMenuItemInfo    (win32u.@)
 */
UINT WINAPI NtUserThunkedMenuItemInfo( HMENU handle, UINT pos, UINT flags, UINT method,
                                       MENUITEMINFOW *info, UNICODE_STRING *str )
{
    POPUPMENU *menu;
    UINT i;
    BOOL ret;

    switch (method)
    {
    case NtUserInsertMenuItem:
        if (!info || info->cbSize != sizeof(*info))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if (!(menu = insert_menu_item( handle, pos, flags, &i )))
        {
            /* workaround for Word 95: pretend that SC_TASKLIST item exists */
            if (pos == SC_TASKLIST && !(flags & MF_BYPOSITION)) return TRUE;
            return FALSE;
        }

        ret = set_menu_item_info( &menu->items[i], info );
        if (!ret) NtUserRemoveMenu( handle, pos, flags );
        release_menu_ptr(menu);
        break;

    case NtUserSetMenuItemInfo:
        if (!info || info->cbSize != sizeof(*info))
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if (!(menu = find_menu_item( handle, pos, flags, &i )))
        {
            /* workaround for Word 95: pretend that SC_TASKLIST item exists */
            if (pos == SC_TASKLIST && !(flags & MF_BYPOSITION)) return TRUE;
            return FALSE;
        }

        ret = set_menu_item_info( &menu->items[i], info );
        if (ret) menu->Height = 0; /* force size recalculate */
        release_menu_ptr(menu);
        break;

    case NtUserGetMenuState:
        return get_menu_state( handle, pos, flags );

    default:
        FIXME( "unsupported method %u\n", method );
        return FALSE;
    }

    return ret;
}

/* see GetMenuItemCount */
INT get_menu_item_count( HMENU handle )
{
    POPUPMENU *menu;
    INT count;

    if (!(menu = grab_menu_ptr( handle ))) return -1;
    count = menu->nItems;
    release_menu_ptr(menu);

    TRACE( "(%p) returning %d\n", handle, count );
    return count;
}

/**********************************************************************
 *           NtUserRemoveMenu    (win32u.@)
 */
BOOL WINAPI NtUserRemoveMenu( HMENU handle, UINT id, UINT flags )
{
    POPUPMENU *menu;
    UINT pos;

    TRACE( "(menu=%p id=%#x flags=%04x)\n", handle, id, flags );

    if (!(menu = find_menu_item( handle, id, flags, &pos )))
        return FALSE;

    /* Remove item */
    free( menu->items[pos].text );

    if (--menu->nItems == 0)
    {
        free( menu->items );
        menu->items = NULL;
    }
    else
    {
        MENUITEM *new_items, *item = &menu->items[pos];

        while (pos < menu->nItems)
        {
            *item = item[1];
            item++;
            pos++;
        }
        new_items = realloc( menu->items, menu->nItems * sizeof(MENUITEM) );
        if (new_items) menu->items = new_items;
    }

    release_menu_ptr(menu);
    return TRUE;
}

/**********************************************************************
 *         NtUserDeleteMenu    (win32u.@)
 */
BOOL WINAPI NtUserDeleteMenu( HMENU handle, UINT id, UINT flags )
{
    POPUPMENU *menu;
    UINT pos;

    if (!(menu = find_menu_item( handle, id, flags, &pos )))
        return FALSE;

    if (menu->items[pos].fType & MF_POPUP)
        NtUserDestroyMenu( menu->items[pos].hSubMenu );

    NtUserRemoveMenu( menu->obj.handle, pos, flags | MF_BYPOSITION );
    release_menu_ptr( menu );
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

/* see GetSubMenu */
static HMENU get_sub_menu( HMENU handle, INT pos )
{
    POPUPMENU *menu;
    HMENU submenu;
    UINT i;

    if (!(menu = find_menu_item( handle, pos, MF_BYPOSITION, &i )))
        return 0;

    if (menu->items[i].fType & MF_POPUP)
        submenu = menu->items[i].hSubMenu;
    else
        submenu = 0;

    release_menu_ptr(menu);
    return submenu;
}

/**********************************************************************
 *           NtUserGetSystemMenu    (win32u.@)
 */
HMENU WINAPI NtUserGetSystemMenu( HWND hwnd, BOOL revert )
{
    WND *win = get_win_ptr( hwnd );
    HMENU retvalue = 0;

    if (win == WND_DESKTOP || !win) return 0;
    if (win == WND_OTHER_PROCESS)
    {
        if (is_window( hwnd )) FIXME( "not supported on other process window %p\n", hwnd );
        return 0;
    }

    if (win->hSysMenu && revert)
    {
        NtUserDestroyMenu( win->hSysMenu );
        win->hSysMenu = 0;
    }

    if (!win->hSysMenu && (win->dwStyle & WS_SYSMENU) && user_callbacks)
        win->hSysMenu = user_callbacks->get_sys_menu( hwnd, 0 );

    if (win->hSysMenu)
    {
        POPUPMENU *menu;
        retvalue = get_sub_menu( win->hSysMenu, 0 );

        /* Store the dummy sysmenu handle to facilitate the refresh */
        /* of the close button if the SC_CLOSE item change */
        menu = grab_menu_ptr( retvalue );
        if (menu)
        {
            menu->hSysMenuOwner = win->hSysMenu;
            release_menu_ptr( menu );
        }
    }

    release_win_ptr( win );
    return revert ? 0 : retvalue;
}

/**********************************************************************
 *           NtUserSetSystemMenu    (win32u.@)
 */
BOOL WINAPI NtUserSetSystemMenu( HWND hwnd, HMENU menu )
{
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return FALSE;

    if (win->hSysMenu) NtUserDestroyMenu( win->hSysMenu );
    win->hSysMenu = user_callbacks ? user_callbacks->get_sys_menu( hwnd, menu ) : NULL;
    release_win_ptr( win );
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

/**********************************************************************
 *           translate_accelerator
 */
static BOOL translate_accelerator( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam,
                                   BYTE virt, WORD key, WORD cmd )
{
    INT mask = 0;
    UINT msg = 0;

    if (wparam != key) return FALSE;

    if (NtUserGetKeyState( VK_CONTROL ) & 0x8000) mask |= FCONTROL;
    if (NtUserGetKeyState( VK_MENU ) & 0x8000)    mask |= FALT;
    if (NtUserGetKeyState( VK_SHIFT ) & 0x8000)   mask |= FSHIFT;

    if (message == WM_CHAR || message == WM_SYSCHAR)
    {
        if (!(virt & FVIRTKEY) && (mask & FALT) == (virt & FALT))
        {
            TRACE_(accel)( "found accel for WM_CHAR: ('%c')\n", LOWORD(wparam) & 0xff );
            goto found;
        }
    }
    else
    {
        if (virt & FVIRTKEY)
        {
            TRACE_(accel)( "found accel for virt_key %04lx (scan %04x)\n",
                           wparam, 0xff & HIWORD(lparam) );

            if (mask == (virt & (FSHIFT | FCONTROL | FALT))) goto found;
            TRACE_(accel)( ", but incorrect SHIFT/CTRL/ALT-state\n" );
        }
        else
        {
            if (!(lparam & 0x01000000))  /* no special_key */
            {
                if ((virt & FALT) && (lparam & 0x20000000)) /* ALT pressed */
                {
                    TRACE_(accel)( "found accel for Alt-%c\n", LOWORD(wparam) & 0xff );
                    goto found;
                }
            }
        }
    }
    return FALSE;

found:
    if (message == WM_KEYUP || message == WM_SYSKEYUP)
        msg = 1;
    else
    {
        HMENU menu_handle, submenu, sys_menu;
        UINT sys_stat = ~0u, stat = ~0u, pos;
        POPUPMENU *menu;

        menu_handle = (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) ? 0 : get_menu(hwnd);
        sys_menu = get_win_sys_menu( hwnd );

        /* find menu item and ask application to initialize it */
        /* 1. in the system menu */
        if ((menu = find_menu_item( sys_menu, cmd, MF_BYCOMMAND, NULL )))
        {
            submenu = menu->obj.handle;
            release_menu_ptr( menu );

            if (get_capture())
                msg = 2;
            if (!is_window_enabled( hwnd ))
                msg = 3;
            else
            {
                send_message( hwnd, WM_INITMENU, (WPARAM)sys_menu, 0 );
                if (submenu != sys_menu)
                {
                    pos = find_submenu( &sys_menu, submenu );
                    TRACE_(accel)( "sys_menu = %p, submenu = %p, pos = %d\n",
                                   sys_menu, submenu, pos );
                    send_message( hwnd, WM_INITMENUPOPUP, (WPARAM)submenu, MAKELPARAM(pos, TRUE) );
                }
                sys_stat = get_menu_state( get_sub_menu( sys_menu, 0 ), cmd, MF_BYCOMMAND );
            }
        }
        else /* 2. in the window's menu */
        {
            if ((menu = find_menu_item( menu_handle, cmd, MF_BYCOMMAND, NULL )))
            {
                submenu = menu->obj.handle;
                release_menu_ptr( menu );

                if (get_capture())
                    msg = 2;
                if (!is_window_enabled( hwnd ))
                    msg = 3;
                else
                {
                    send_message( hwnd, WM_INITMENU, (WPARAM)menu_handle, 0 );
                    if(submenu != menu_handle)
                    {
                        pos = find_submenu( &menu_handle, submenu );
                        TRACE_(accel)( "menu_handle = %p, submenu = %p, pos = %d\n",
                                       menu_handle, submenu, pos );
                        send_message( hwnd, WM_INITMENUPOPUP, (WPARAM)submenu,
                                      MAKELPARAM(pos, FALSE) );
                    }
                    stat = get_menu_state( menu_handle, cmd, MF_BYCOMMAND );
                }
            }
        }

        if (msg == 0)
        {
            if (sys_stat != ~0u)
            {
                if (sys_stat & (MF_DISABLED|MF_GRAYED))
                    msg = 4;
                else
                    msg = WM_SYSCOMMAND;
            }
            else
            {
                if (stat != ~0u)
                {
                    if (is_iconic( hwnd ))
                        msg = 5;
                    else
                    {
                        if (stat & (MF_DISABLED|MF_GRAYED))
                            msg = 6;
                        else
                            msg = WM_COMMAND;
                    }
                }
                else
                    msg = WM_COMMAND;
            }
        }
    }

    if (msg == WM_COMMAND)
    {
        TRACE_(accel)( ", sending WM_COMMAND, wparam=%0x\n", 0x10000 | cmd );
        send_message( hwnd, msg, 0x10000 | cmd, 0 );
    }
    else if (msg == WM_SYSCOMMAND)
    {
        TRACE_(accel)( ", sending WM_SYSCOMMAND, wparam=%0x\n", cmd );
        send_message( hwnd, msg, cmd, 0x00010000 );
    }
    else
    {
        /*  some reasons for NOT sending the WM_{SYS}COMMAND message:
         *   #0: unknown (please report!)
         *   #1: for WM_KEYUP,WM_SYSKEYUP
         *   #2: mouse is captured
         *   #3: window is disabled
         *   #4: it's a disabled system menu option
         *   #5: it's a menu option, but window is iconic
         *   #6: it's a menu option, but disabled
         */
        TRACE_(accel)( ", but won't send WM_{SYS}COMMAND, reason is #%d\n", msg );
        if (!msg) ERR_(accel)( " unknown reason\n" );
    }
    return TRUE;
}

/**********************************************************************
 *           NtUserTranslateAccelerator     (win32u.@)
 */
INT WINAPI NtUserTranslateAccelerator( HWND hwnd, HACCEL accel, MSG *msg )
{
    ACCEL data[32], *ptr = data;
    int i, count;

    if (!hwnd) return 0;

    if (msg->message != WM_KEYDOWN &&
        msg->message != WM_SYSKEYDOWN &&
        msg->message != WM_CHAR &&
        msg->message != WM_SYSCHAR)
        return 0;

    TRACE_(accel)("accel %p, hwnd %p, msg->hwnd %p, msg->message %04x, wParam %08lx, lParam %08lx\n",
                  accel,hwnd,msg->hwnd,msg->message,msg->wParam,msg->lParam);

    if (!(count = NtUserCopyAcceleratorTable( accel, NULL, 0 ))) return 0;
    if (count > ARRAY_SIZE( data ))
    {
        if (!(ptr = malloc( count * sizeof(*ptr) ))) return 0;
    }
    count = NtUserCopyAcceleratorTable( accel, ptr, count );
    for (i = 0; i < count; i++)
    {
        if (translate_accelerator( hwnd, msg->message, msg->wParam, msg->lParam,
                                   ptr[i].fVirt, ptr[i].key, ptr[i].cmd))
            break;
    }
    if (ptr != data) free( ptr );
    return (i < count);
}
