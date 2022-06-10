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

#define OEMRESOURCE
#include "ntgdi_private.h"
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

/* Space between 2 columns */
#define MENU_COL_SPACE 4

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


static SIZE menucharsize;
static UINT od_item_hight; /* default owner drawn item height */

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

/*
 * Validate the given menu handle and returns the menu structure pointer.
 * FIXME: this is unsafe, we should use a better mechanism instead.
 */
static POPUPMENU *unsafe_menu_ptr( HMENU handle )
{
    POPUPMENU *menu = grab_menu_ptr( handle );
    if (menu) release_menu_ptr( menu );
    return menu;
}

/* see IsMenu */
BOOL is_menu( HMENU handle )
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
UINT get_menu_state( HMENU handle, UINT item_id, UINT flags )
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

static HFONT get_menu_font( BOOL bold )
{
    static HFONT menu_font, menu_font_bold;

    HFONT ret = bold ? menu_font_bold : menu_font;

    if (!ret)
    {
        NONCLIENTMETRICSW ncm;
        HFONT prev;

        ncm.cbSize = sizeof(NONCLIENTMETRICSW);
        NtUserSystemParametersInfo( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0 );

        if (bold)
        {
            ncm.lfMenuFont.lfWeight += 300;
            if (ncm.lfMenuFont.lfWeight > 1000) ncm.lfMenuFont.lfWeight = 1000;
        }
        if (!(ret = NtGdiHfontCreate( &ncm.lfMenuFont, sizeof(ncm.lfMenuFont), 0, 0, NULL )))
            return 0;
        prev = InterlockedCompareExchangePointer( (void **)(bold ? &menu_font_bold : &menu_font),
                                                  ret, NULL );
        if (prev)
        {
            /* another thread beat us to it */
            NtGdiDeleteObjectApp( ret );
            ret = prev;
        }
    }
    return ret;
}

static HBITMAP get_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap)
        arrow_bitmap = LoadImageW( 0, MAKEINTRESOURCEW(OBM_MNARROW), IMAGE_BITMAP, 0, 0, 0 );
    return arrow_bitmap;
}

/* Get the size of a bitmap item */
static void get_bitmap_item_size( MENUITEM *item, SIZE *size, HWND owner )
{
    HBITMAP bmp = item->hbmpItem;
    BITMAP bm;

    size->cx = size->cy = 0;

    /* check if there is a magic menu item associated with this item */
    switch ((INT_PTR)bmp)
    {
    case (INT_PTR)HBMMENU_CALLBACK:
        {
            MEASUREITEMSTRUCT meas_item;
            meas_item.CtlType = ODT_MENU;
            meas_item.CtlID = 0;
            meas_item.itemID = item->wID;
            meas_item.itemWidth = item->rect.right - item->rect.left;
            meas_item.itemHeight = item->rect.bottom - item->rect.top;
            meas_item.itemData = item->dwItemData;
            send_message( owner, WM_MEASUREITEM, 0, (LPARAM)&meas_item );
            size->cx = meas_item.itemWidth;
            size->cy = meas_item.itemHeight;
            return;
        }
        break;
    case (INT_PTR)HBMMENU_SYSTEM:
        if (item->dwItemData)
        {
            bmp = (HBITMAP)item->dwItemData;
            break;
        }
        /* fall through */
    case (INT_PTR)HBMMENU_MBAR_RESTORE:
    case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
    case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
    case (INT_PTR)HBMMENU_MBAR_CLOSE:
    case (INT_PTR)HBMMENU_MBAR_CLOSE_D:
        size->cx = get_system_metrics( SM_CYMENU ) - 4;
        size->cy = size->cx;
        return;
    case (INT_PTR)HBMMENU_POPUP_CLOSE:
    case (INT_PTR)HBMMENU_POPUP_RESTORE:
    case (INT_PTR)HBMMENU_POPUP_MAXIMIZE:
    case (INT_PTR)HBMMENU_POPUP_MINIMIZE:
        size->cx = get_system_metrics( SM_CXMENUSIZE );
        size->cy = get_system_metrics( SM_CYMENUSIZE );
        return;
    }
    if (NtGdiExtGetObjectW( bmp, sizeof(bm), &bm ))
    {
        size->cx = bm.bmWidth;
        size->cy = bm.bmHeight;
    }
}

/* Calculate the size of the menu item and store it in item->rect */
static void calc_menu_item_size( HDC hdc, MENUITEM *item, HWND owner, INT org_x, INT org_y,
                                 BOOL menu_bar, POPUPMENU *menu )
{
    UINT check_bitmap_width = get_system_metrics( SM_CXMENUCHECK );
    UINT arrow_bitmap_width;
    INT item_height;
    BITMAP bm;
    WCHAR *p;

    TRACE( "dc=%p owner=%p (%d,%d) item %s\n", hdc, owner, org_x, org_y, debugstr_menuitem( item ));

    NtGdiExtGetObjectW( get_arrow_bitmap(), sizeof(bm), &bm );
    arrow_bitmap_width = bm.bmWidth;

    if (!menucharsize.cx)
    {
        menucharsize.cx = get_char_dimensions( hdc, NULL, &menucharsize.cy );
        /* Win95/98/ME will use menucharsize.cy here. Testing is possible
         * but it is unlikely an application will depend on that */
        od_item_hight = HIWORD( get_dialog_base_units() );
    }

    SetRect( &item->rect, org_x, org_y, org_x, org_y );

    if (item->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.CtlID      = 0;
        mis.itemID     = item->wID;
        mis.itemData   = item->dwItemData;
        mis.itemHeight = od_item_hight;
        mis.itemWidth  = 0;
        send_message( owner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        /* Tests reveal that Windows ( Win95 through WinXP) adds twice the average
         * width of a menufont character to the width of an owner-drawn menu. */
        item->rect.right += mis.itemWidth + 2 * menucharsize.cx;
        if (menu_bar)
        {
            /* Under at least win95 you seem to be given a standard
             * height for the menu and the height value is ignored. */
            item->rect.bottom += get_system_metrics( SM_CYMENUSIZE );
        }
        else
            item->rect.bottom += mis.itemHeight;

        TRACE( "id=%04lx size=%dx%d\n", item->wID, item->rect.right-item->rect.left,
               item->rect.bottom-item->rect.top );
        return;
    }

    if (item->fType & MF_SEPARATOR)
    {
        item->rect.bottom += get_system_metrics( SM_CYMENUSIZE ) / 2;
        if (!menu_bar) item->rect.right += arrow_bitmap_width + menucharsize.cx;
        return;
    }

    item_height = 0;
    item->xTab = 0;

    if (!menu_bar)
    {
        if (item->hbmpItem)
        {
            SIZE size;

            get_bitmap_item_size( item, &size, owner );
            /* Keep the size of the bitmap in callback mode to be able
             * to draw it correctly */
            item->bmpsize = size;
            menu->textOffset = max( menu->textOffset, size.cx );
            item->rect.right += size.cx + 2;
            item_height = size.cy + 2;
        }
        if (!(menu->dwStyle & MNS_NOCHECK)) item->rect.right += check_bitmap_width;
        item->rect.right += 4 + menucharsize.cx;
        item->xTab = item->rect.right;
        item->rect.right += arrow_bitmap_width;
    }
    else if (item->hbmpItem) /* menu_bar */
    {
        SIZE size;

        get_bitmap_item_size( item, &size, owner );
        item->bmpsize = size;
        item->rect.right  += size.cx;
        if (item->text) item->rect.right += 2;
        item_height = size.cy;
    }

    /* it must be a text item - unless it's the system menu */
    if (!(item->fType & MF_SYSMENU) && item->text)
    {
        LONG txt_height, txt_width;
        HFONT prev_font = NULL;
        RECT rc = item->rect;

        if (item->fState & MFS_DEFAULT)
             prev_font = NtGdiSelectFont( hdc, get_menu_font(TRUE) );

        if (menu_bar)
        {
            txt_height = DrawTextW( hdc, item->text, -1, &rc, DT_SINGLELINE | DT_CALCRECT );
            item->rect.right  += rc.right - rc.left;
            item_height = max( max( item_height, txt_height ),
                               get_system_metrics( SM_CYMENU ) - 1 );
            item->rect.right +=  2 * menucharsize.cx;
        }
        else
        {
            if ((p = wcschr( item->text, '\t' )))
            {
                RECT r = rc;
                int h, n = (int)(p - item->text);

                /* Item contains a tab (only meaningful in popup menus) */
                /* get text size before the tab */
                txt_height = DrawTextW( hdc, item->text, n, &rc, DT_SINGLELINE | DT_CALCRECT );
                txt_width = rc.right - rc.left;
                p += 1; /* advance past the Tab */
                /* get text size after the tab */
                h = DrawTextW( hdc, p, -1, &r, DT_SINGLELINE | DT_CALCRECT );
                item->xTab += txt_width;
                txt_height = max( txt_height, h );
                /* space for the tab and the short cut */
                txt_width += menucharsize.cx + r.right - r.left;
            }
            else
            {
                txt_height = DrawTextW( hdc, item->text, -1, &rc, DT_SINGLELINE | DT_CALCRECT );
                txt_width = rc.right - rc.left;
                item->xTab += txt_width;
            }
            item->rect.right += 2 + txt_width;
            item_height = max( item_height, max( txt_height + 2, menucharsize.cy + 4 ));
        }
        if (prev_font) NtGdiSelectFont( hdc, prev_font );
    }
    else if (menu_bar)
    {
        item_height = max( item_height, get_system_metrics( SM_CYMENU ) - 1 );
    }
    item->rect.bottom += item_height;
    TRACE( "%s\n", wine_dbgstr_rect( &item->rect ));
}

/* Calculate the size of the menu bar */
static void calc_menu_bar_size( HDC hdc, RECT *rect, POPUPMENU *menu, HWND owner )
{
    UINT start, i, help_pos;
    int org_x, org_y;
    MENUITEM *item;

    if (!rect || !menu || !menu->nItems) return;

    TRACE( "rect %p %s\n", rect, wine_dbgstr_rect( rect ));
    /* Start with a 1 pixel top border.
       This corresponds to the difference between SM_CYMENU and SM_CYMENUSIZE. */
    SetRect( &menu->items_rect, 0, 0, rect->right - rect->left, 1 );
    start = 0;
    help_pos = ~0u;
    menu->textOffset = 0;
    while (start < menu->nItems)
    {
        item = &menu->items[start];
        org_x = menu->items_rect.left;
        org_y = menu->items_rect.bottom;

        /* Parse items until line break or end of menu */
        for (i = start; i < menu->nItems; i++, item++)
        {
            if (help_pos == ~0u && (item->fType & MF_RIGHTJUSTIFY)) help_pos = i;
            if (i != start && (item->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

            TRACE("item org=(%d, %d) %s\n", org_x, org_y, debugstr_menuitem( item ));
            calc_menu_item_size( hdc, item, owner, org_x, org_y, TRUE, menu );

            if (item->rect.right > menu->items_rect.right)
            {
                if (i != start) break;
                else item->rect.right = menu->items_rect.right;
            }
            menu->items_rect.bottom = max( menu->items_rect.bottom, item->rect.bottom );
            org_x = item->rect.right;
        }

        /* Finish the line (set all items to the largest height found) */
        while (start < i) menu->items[start++].rect.bottom = menu->items_rect.bottom;
    }

    OffsetRect( &menu->items_rect, rect->left, rect->top );
    menu->Width = menu->items_rect.right - menu->items_rect.left;
    menu->Height = menu->items_rect.bottom - menu->items_rect.top;
    rect->bottom = menu->items_rect.bottom;

    /* Flush right all items between the MF_RIGHTJUSTIFY and */
    /* the last item (if several lines, only move the last line) */
    if (help_pos == ~0u) return;
    item = &menu->items[menu->nItems-1];
    org_y = item->rect.top;
    org_x = rect->right - rect->left;
    for (i = menu->nItems - 1; i >= help_pos; i--, item--)
    {
        if (item->rect.top != org_y) break;    /* other line */
        if (item->rect.right >= org_x) break;  /* too far right already */
        item->rect.left += org_x - item->rect.right;
        item->rect.right = org_x;
        org_x = item->rect.left;
    }
}

UINT get_menu_bar_height( HWND hwnd, UINT width, INT org_x, INT org_y )
{
    POPUPMENU *menu;
    RECT rect_bar;
    HDC hdc;

    TRACE( "hwnd %p, width %d, at (%d, %d).\n", hwnd, width, org_x, org_y );

    if (!(menu = unsafe_menu_ptr( get_menu( hwnd )))) return 0;

    hdc = NtUserGetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    NtGdiSelectFont( hdc, get_menu_font(FALSE));
    SetRect( &rect_bar, org_x, org_y, org_x + width, org_y + get_system_metrics( SM_CYMENU ));
    calc_menu_bar_size( hdc, &rect_bar, menu, hwnd );
    NtUserReleaseDC( hwnd, hdc );
    return menu->Height;
}

static void draw_popup_arrow( HDC hdc, RECT rect, UINT arrow_width, UINT arrow_height )
{
    HDC mem_hdc = NtGdiCreateCompatibleDC( hdc );
    HBITMAP prev_bitmap;

    prev_bitmap = NtGdiSelectBitmap( mem_hdc, get_arrow_bitmap() );
    NtGdiBitBlt( hdc, rect.right - arrow_width - 1,
                 (rect.top + rect.bottom - arrow_height) / 2,
                 arrow_width, arrow_height, mem_hdc, 0, 0, SRCCOPY, 0, 0 );
    NtGdiSelectBitmap( mem_hdc, prev_bitmap );
    NtGdiDeleteObjectApp( mem_hdc );
}

static void draw_bitmap_item( HDC hdc, MENUITEM *item, const RECT *rect,
                              POPUPMENU *menu, HWND owner, UINT odaction )
{
    int w = rect->right - rect->left;
    int h = rect->bottom - rect->top;
    int bmp_xoffset = 0, left, top;
    HBITMAP bmp_to_draw = item->hbmpItem;
    HBITMAP bmp = bmp_to_draw;
    BITMAP bm;
    DWORD rop;
    HDC mem_hdc;

    /* Check if there is a magic menu item associated with this item */
    if (IS_MAGIC_BITMAP( bmp_to_draw ))
    {
        UINT flags = 0;
        WCHAR bmchr = 0;
        RECT r;

        switch ((INT_PTR)bmp_to_draw)
        {
        case (INT_PTR)HBMMENU_SYSTEM:
            if (item->dwItemData)
            {
                bmp = (HBITMAP)item->dwItemData;
                if (!NtGdiExtGetObjectW( bmp, sizeof(bm), &bm )) return;
            }
            else
            {
                static HBITMAP sys_menu_bmp;

                if (!sys_menu_bmp)
                    sys_menu_bmp = LoadImageW( 0, MAKEINTRESOURCEW(OBM_CLOSE), IMAGE_BITMAP, 0, 0, 0 );
                bmp = sys_menu_bmp;
                if (!NtGdiExtGetObjectW( bmp, sizeof(bm), &bm )) return;
                /* only use right half of the bitmap */
                bmp_xoffset = bm.bmWidth / 2;
                bm.bmWidth -= bmp_xoffset;
            }
            goto got_bitmap;
        case (INT_PTR)HBMMENU_MBAR_RESTORE:
            flags = DFCS_CAPTIONRESTORE;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
            flags = DFCS_CAPTIONMIN;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
            flags = DFCS_CAPTIONMIN | DFCS_INACTIVE;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE:
            flags = DFCS_CAPTIONCLOSE;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE_D:
            flags = DFCS_CAPTIONCLOSE | DFCS_INACTIVE;
            break;
        case (INT_PTR)HBMMENU_CALLBACK:
            {
                DRAWITEMSTRUCT drawItem;
                drawItem.CtlType = ODT_MENU;
                drawItem.CtlID = 0;
                drawItem.itemID = item->wID;
                drawItem.itemAction = odaction;
                drawItem.itemState = 0;
                if (item->fState & MF_CHECKED)  drawItem.itemState |= ODS_CHECKED;
                if (item->fState & MF_DEFAULT)  drawItem.itemState |= ODS_DEFAULT;
                if (item->fState & MF_DISABLED) drawItem.itemState |= ODS_DISABLED;
                if (item->fState & MF_GRAYED)   drawItem.itemState |= ODS_GRAYED|ODS_DISABLED;
                if (item->fState & MF_HILITE)   drawItem.itemState |= ODS_SELECTED;
                drawItem.hwndItem = (HWND)menu->obj.handle;
                drawItem.hDC = hdc;
                drawItem.itemData = item->dwItemData;
                drawItem.rcItem = *rect;
                send_message( owner, WM_DRAWITEM, 0, (LPARAM)&drawItem );
                return;
            }
            break;
        case (INT_PTR)HBMMENU_POPUP_CLOSE:
            bmchr = 0x72;
            break;
        case (INT_PTR)HBMMENU_POPUP_RESTORE:
            bmchr = 0x32;
            break;
        case (INT_PTR)HBMMENU_POPUP_MAXIMIZE:
            bmchr = 0x31;
            break;
        case (INT_PTR)HBMMENU_POPUP_MINIMIZE:
            bmchr = 0x30;
            break;
        default:
            FIXME( "Magic %p not implemented\n", bmp_to_draw );
            return;
        }

        if (bmchr)
        {
            /* draw the magic bitmaps using marlett font characters */
            /* FIXME: fontsize and the position (x,y) could probably be better */
            HFONT hfont, prev_font;
            LOGFONTW logfont = { 0, 0, 0, 0, FW_NORMAL, 0, 0, 0, SYMBOL_CHARSET, 0, 0, 0, 0,
                                 {'M','a','r','l','e','t','t'}};
            logfont.lfHeight =  min( h, w) - 5 ;
            TRACE( " height %d rect %s\n", logfont.lfHeight, wine_dbgstr_rect( rect ));
            hfont = NtGdiHfontCreate( &logfont, sizeof(logfont), 0, 0, NULL );
            prev_font = NtGdiSelectFont( hdc, hfont );
            NtGdiExtTextOutW( hdc, rect->left, rect->top + 2, 0, NULL, &bmchr, 1, NULL, 0 );
            NtGdiSelectFont( hdc, prev_font );
            NtGdiDeleteObjectApp( hfont );
        }
        else
        {
            r = *rect;
            InflateRect( &r, -1, -1 );
            if (item->fState & MF_HILITE) flags |= DFCS_PUSHED;
            draw_frame_caption( hdc, &r, flags );
        }
        return;
    }

    if (!bmp || !NtGdiExtGetObjectW( bmp, sizeof(bm), &bm )) return;

got_bitmap:
    mem_hdc = NtGdiCreateCompatibleDC( hdc );
    NtGdiSelectBitmap( mem_hdc, bmp );

    /* handle fontsize > bitmap_height */
    top = (h>bm.bmHeight) ? rect->top + (h - bm.bmHeight) / 2 : rect->top;
    left=rect->left;
    rop= ((item->fState & MF_HILITE) && !IS_MAGIC_BITMAP(bmp_to_draw)) ? NOTSRCCOPY : SRCCOPY;
    if ((item->fState & MF_HILITE) && item->hbmpItem)
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color( COLOR_HIGHLIGHT ), NULL );
    NtGdiBitBlt( hdc, left, top, w, h, mem_hdc, bmp_xoffset, 0, rop, 0, 0 );
    NtGdiDeleteObjectApp( mem_hdc );
}

/* Adjust menu item rectangle according to scrolling state */
static void adjust_menu_item_rect( const POPUPMENU *menu, RECT *rect )
{
    INT scroll_offset = menu->bScrolling ? menu->nScrollPos : 0;
    OffsetRect( rect, menu->items_rect.left, menu->items_rect.top - scroll_offset );
}

/* Draw a single menu item */
static void draw_menu_item( HWND hwnd, POPUPMENU *menu, HWND owner, HDC hdc,
                            MENUITEM *item, BOOL menu_bar, UINT odaction )
{
    UINT arrow_width = 0, arrow_height = 0;
    HRGN old_clip = NULL, clip;
    BOOL flat_menu = FALSE;
    RECT rect, bmprc;
    int bkgnd;

    TRACE( "%s\n", debugstr_menuitem( item ));

    if (!menu_bar)
    {
        BITMAP bmp;
        NtGdiExtGetObjectW( get_arrow_bitmap(), sizeof(bmp), &bmp );
        arrow_width = bmp.bmWidth;
        arrow_height = bmp.bmHeight;
    }

    if (item->fType & MF_SYSMENU)
    {
        if (!is_iconic( hwnd ))
            draw_nc_sys_button( hwnd, hdc, item->fState & (MF_HILITE | MF_MOUSESELECT) );
        return;
    }

    TRACE( "rect=%s\n", wine_dbgstr_rect( &item->rect ));
    rect = item->rect;
    adjust_menu_item_rect( menu, &rect );
    if (!intersect_rect( &bmprc, &rect, &menu->items_rect )) /* bmprc is used as a dummy */
        return;

    NtUserSystemParametersInfo( SPI_GETFLATMENU, 0, &flat_menu, 0 );
    bkgnd = (menu_bar && flat_menu) ? COLOR_MENUBAR : COLOR_MENU;

    /* Setup colors */
    if (item->fState & MF_HILITE)
    {
        if (menu_bar && !flat_menu)
        {
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color(COLOR_MENUTEXT), NULL );
            NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color(COLOR_MENU), NULL );
        }
        else
        {
            if (item->fState & MF_GRAYED)
                NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_GRAYTEXT ), NULL );
            else
                NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_HIGHLIGHTTEXT ), NULL );
            NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color( COLOR_HIGHLIGHT ), NULL );
        }
    }
    else
    {
        if (item->fState & MF_GRAYED)
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_GRAYTEXT ), NULL );
        else
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, get_sys_color( COLOR_MENUTEXT ), NULL );
        NtGdiGetAndSetDCDword( hdc, NtGdiSetBkColor, get_sys_color( bkgnd ), NULL );
    }

    old_clip = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    if (NtGdiGetRandomRgn( hdc, old_clip, NTGDI_RGN_MIRROR_RTL | 1 ) <= 0)
    {
        NtGdiDeleteObjectApp( old_clip );
        old_clip = NULL;
    }
    clip = NtGdiCreateRectRgn( menu->items_rect.left, menu->items_rect.top,
                               menu->items_rect.right, menu->items_rect.bottom );
    NtGdiExtSelectClipRgn( hdc, clip, RGN_AND );
    NtGdiDeleteObjectApp( clip );

    if (item->fType & MF_OWNERDRAW)
    {
        /*
         * Experimentation under Windows reveals that an owner-drawn
         * menu is given the rectangle which includes the space it requested
         * in its response to WM_MEASUREITEM _plus_ width for a checkmark
         * and a popup-menu arrow.  This is the value of item->rect.
         * Windows will leave all drawing to the application except for
         * the popup-menu arrow.  Windows always draws that itself, after
         * the menu owner has finished drawing.
         */
        DRAWITEMSTRUCT dis;
        DWORD old_bk, old_text;

        dis.CtlType   = ODT_MENU;
        dis.CtlID     = 0;
        dis.itemID    = item->wID;
        dis.itemData  = item->dwItemData;
        dis.itemState = 0;
        if (item->fState & MF_CHECKED) dis.itemState |= ODS_CHECKED;
        if (item->fState & MF_GRAYED)  dis.itemState |= ODS_GRAYED|ODS_DISABLED;
        if (item->fState & MF_HILITE)  dis.itemState |= ODS_SELECTED;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = (HWND)menu->obj.handle;
        dis.hDC        = hdc;
        dis.rcItem     = rect;
        TRACE( "Ownerdraw: owner=%p itemID=%d, itemState=%d, itemAction=%d, "
               "hwndItem=%p, hdc=%p, rcItem=%s\n", owner,
               dis.itemID, dis.itemState, dis.itemAction, dis.hwndItem,
               dis.hDC, wine_dbgstr_rect( &dis.rcItem ));
        NtGdiGetDCDword( hdc, NtGdiGetBkColor, &old_bk );
        NtGdiGetDCDword( hdc, NtGdiGetTextColor, &old_text );
        send_message( owner, WM_DRAWITEM, 0, (LPARAM)&dis );
        /* Draw the popup-menu arrow */
        NtGdiGetAndSetDCDword( hdc, NtGdiGetBkColor, old_bk, NULL );
        NtGdiGetAndSetDCDword( hdc, NtGdiGetTextColor, old_text, NULL );
        if (item->fType & MF_POPUP)
            draw_popup_arrow( hdc, rect, arrow_width, arrow_height );
        goto done;
    }

    if (menu_bar && (item->fType & MF_SEPARATOR)) goto done;

    if (item->fState & MF_HILITE)
    {
        if (flat_menu)
        {
            InflateRect (&rect, -1, -1);
            fill_rect( hdc, &rect, get_sys_color_brush( COLOR_MENUHILIGHT ));
            InflateRect (&rect, 1, 1);
            fill_rect( hdc, &rect, get_sys_color_brush( COLOR_HIGHLIGHT ));
        }
        else
        {
            if (menu_bar)
                draw_rect_edge( hdc, &rect, BDR_SUNKENOUTER, BF_RECT, 1 );
            else
                fill_rect( hdc, &rect, get_sys_color_brush( COLOR_HIGHLIGHT ));
        }
    }
    else
        fill_rect( hdc, &rect, get_sys_color_brush(bkgnd) );

    NtGdiGetAndSetDCDword( hdc, NtGdiSetBkMode, TRANSPARENT, NULL );

    /* vertical separator */
    if (!menu_bar && (item->fType & MF_MENUBARBREAK))
    {
        HPEN oldPen;
        RECT rc = rect;

        rc.left -= MENU_COL_SPACE / 2 + 1;
        rc.top = 3;
        rc.bottom = menu->Height - 3;
        if (flat_menu)
        {
            oldPen = NtGdiSelectPen( hdc, get_sys_color_pen( COLOR_BTNSHADOW ));
            NtGdiMoveTo( hdc, rc.left, rc.top, NULL );
            NtGdiLineTo( hdc, rc.left, rc.bottom );
            NtGdiSelectPen( hdc, oldPen );
        }
        else
            draw_rect_edge( hdc, &rc, EDGE_ETCHED, BF_LEFT, 1 );
    }

    /* horizontal separator */
    if (item->fType & MF_SEPARATOR)
    {
        HPEN oldPen;
        RECT rc = rect;

        InflateRect( &rc, -1, 0 );
        rc.top = ( rc.top + rc.bottom) / 2;
        if (flat_menu)
        {
            oldPen = NtGdiSelectPen( hdc, get_sys_color_pen( COLOR_BTNSHADOW ));
            NtGdiMoveTo( hdc, rc.left, rc.top, NULL );
            NtGdiLineTo( hdc, rc.right, rc.top );
            NtGdiSelectPen( hdc, oldPen );
        }
        else
            draw_rect_edge( hdc, &rc, EDGE_ETCHED, BF_TOP, 1 );
        goto done;
    }

    if (item->hbmpItem)
    {
        /* calculate the bitmap rectangle in coordinates relative
         * to the item rectangle */
        if (menu_bar)
        {
            if (item->hbmpItem == HBMMENU_CALLBACK)
                bmprc.left = 3;
            else
                bmprc.left = item->text ? menucharsize.cx : 0;
        }
        else if (menu->dwStyle & MNS_NOCHECK)
            bmprc.left = 4;
        else if (menu->dwStyle & MNS_CHECKORBMP)
            bmprc.left = 2;
        else
            bmprc.left = 4 + get_system_metrics( SM_CXMENUCHECK );
        bmprc.right =  bmprc.left + item->bmpsize.cx;
        if (menu_bar && !(item->hbmpItem == HBMMENU_CALLBACK))
            bmprc.top = 0;
        else
            bmprc.top = (rect.bottom - rect.top - item->bmpsize.cy) / 2;
        bmprc.bottom = bmprc.top + item->bmpsize.cy;
    }

    if (!menu_bar)
    {
        HBITMAP bm;
        INT y = rect.top + rect.bottom;
        BOOL checked = FALSE;
        UINT check_bitmap_width = get_system_metrics( SM_CXMENUCHECK );
        UINT check_bitmap_height = get_system_metrics( SM_CYMENUCHECK );

        /* Draw the check mark */
        if (!(menu->dwStyle & MNS_NOCHECK))
        {
            bm = (item->fState & MF_CHECKED) ? item->hCheckBit :
                item->hUnCheckBit;
            if (bm)  /* we have a custom bitmap */
            {
                HDC mem_hdc = NtGdiCreateCompatibleDC( hdc );

                NtGdiSelectBitmap( mem_hdc, bm );
                NtGdiBitBlt( hdc, rect.left, (y - check_bitmap_height) / 2,
                             check_bitmap_width, check_bitmap_height,
                             mem_hdc, 0, 0, SRCCOPY, 0, 0 );
                NtGdiDeleteObjectApp( mem_hdc );
                checked = TRUE;
            }
            else if (item->fState & MF_CHECKED) /* standard bitmaps */
            {
                RECT r;
                HBITMAP bm = NtGdiCreateBitmap( check_bitmap_width,
                        check_bitmap_height, 1, 1, NULL );
                HDC mem_hdc = NtGdiCreateCompatibleDC( hdc );

                NtGdiSelectBitmap( mem_hdc, bm );
                SetRect( &r, 0, 0, check_bitmap_width, check_bitmap_height);
                draw_frame_menu( mem_hdc, &r,
                                 (item->fType & MFT_RADIOCHECK) ? DFCS_MENUBULLET : DFCS_MENUCHECK );
                NtGdiBitBlt( hdc, rect.left, (y - r.bottom) / 2, r.right, r.bottom,
                             mem_hdc, 0, 0, SRCCOPY, 0, 0 );
                NtGdiDeleteObjectApp( mem_hdc );
                NtGdiDeleteObjectApp( bm );
                checked = TRUE;
            }
        }
        if (item->hbmpItem && !(checked && (menu->dwStyle & MNS_CHECKORBMP)))
        {
            POINT origorg;
            /* some applications make this assumption on the DC's origin */
            set_viewport_org( hdc, rect.left, rect.top, &origorg );
            draw_bitmap_item( hdc, item, &bmprc, menu, owner, odaction );
            set_viewport_org( hdc, origorg.x, origorg.y, NULL );
        }
        /* Draw the popup-menu arrow */
        if (item->fType & MF_POPUP)
            draw_popup_arrow( hdc, rect, arrow_width, arrow_height);
        rect.left += 4;
        if (!(menu->dwStyle & MNS_NOCHECK))
            rect.left += check_bitmap_width;
        rect.right -= arrow_width;
    }
    else if (item->hbmpItem)
    {   /* Draw the bitmap */
        POINT origorg;

        set_viewport_org( hdc, rect.left, rect.top, &origorg);
        draw_bitmap_item( hdc, item, &bmprc, menu, owner, odaction );
        set_viewport_org( hdc, origorg.x, origorg.y, NULL);
    }
    /* process text if present */
    if (item->text)
    {
        int i;
        HFONT prev_font = 0;
        UINT format = menu_bar ?
            DT_CENTER | DT_VCENTER | DT_SINGLELINE :
            DT_LEFT | DT_VCENTER | DT_SINGLELINE;

        if (!(menu->dwStyle & MNS_CHECKORBMP))
            rect.left += menu->textOffset;

        if (item->fState & MFS_DEFAULT)
        {
             prev_font = NtGdiSelectFont(hdc, get_menu_font( TRUE ));
        }

        if (menu_bar)
        {
            if (item->hbmpItem)
                rect.left += item->bmpsize.cx;
            if (item->hbmpItem != HBMMENU_CALLBACK)
                rect.left += menucharsize.cx;
            rect.right -= menucharsize.cx;
        }

        for (i = 0; item->text[i]; i++)
            if ((item->text[i] == '\t') || (item->text[i] == '\b'))
                break;

        if (item->fState & MF_GRAYED)
        {
            if (!(item->fState & MF_HILITE) )
            {
                ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
                NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, RGB(0xff, 0xff, 0xff), NULL );
                DrawTextW( hdc, item->text, i, &rect, format );
                --rect.left; --rect.top; --rect.right; --rect.bottom;
            }
            NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, RGB(0x80, 0x80, 0x80), NULL );
        }

        DrawTextW( hdc, item->text, i, &rect, format );

        /* paint the shortcut text */
        if (!menu_bar && item->text[i])  /* There's a tab or flush-right char */
        {
            if (item->text[i] == '\t')
            {
                rect.left = item->xTab;
                format = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
            }
            else
            {
                rect.right = item->xTab;
                format = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
            }

            if (item->fState & MF_GRAYED)
            {
                if (!(item->fState & MF_HILITE) )
                {
                    ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
                    NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, RGB(0xff, 0xff, 0xff), NULL );
                    DrawTextW( hdc, item->text + i + 1, -1, &rect, format );
                    --rect.left; --rect.top; --rect.right; --rect.bottom;
                }
                NtGdiGetAndSetDCDword( hdc, NtGdiSetTextColor, RGB(0x80, 0x80, 0x80), NULL );
            }
            DrawTextW( hdc, item->text + i + 1, -1, &rect, format );
        }

        if (prev_font) NtGdiSelectFont( hdc, prev_font );
    }

done:
    NtGdiExtSelectClipRgn( hdc, old_clip, RGN_COPY );
    if (old_clip) NtGdiDeleteObjectApp( old_clip );
}

/***********************************************************************
 *           NtUserDrawMenuBarTemp   (win32u.@)
 */
DWORD WINAPI NtUserDrawMenuBarTemp( HWND hwnd, HDC hdc, RECT *rect, HMENU handle, HFONT font )
{
    BOOL flat_menu = FALSE;
    HFONT prev_font = 0;
    POPUPMENU *menu;
    UINT i, retvalue;

    NtUserSystemParametersInfo( SPI_GETFLATMENU, 0, &flat_menu, 0 );

    if (!handle) handle = get_menu( hwnd );
    if (!font) font = get_menu_font(FALSE);

    menu = unsafe_menu_ptr( handle );
    if (!menu || !rect) return get_system_metrics( SM_CYMENU );

    TRACE( "(%p, %p, %p, %p, %p)\n", hwnd, hdc, rect, handle, font );

    prev_font = NtGdiSelectFont( hdc, font );

    if (!menu->Height) calc_menu_bar_size( hdc, rect, menu, hwnd );

    rect->bottom = rect->top + menu->Height;

    fill_rect( hdc, rect, get_sys_color_brush( flat_menu ? COLOR_MENUBAR : COLOR_MENU ));

    NtGdiSelectPen( hdc, get_sys_color_pen( COLOR_3DFACE ));
    NtGdiMoveTo( hdc, rect->left, rect->bottom, NULL );
    NtGdiLineTo( hdc, rect->right, rect->bottom );

    if (menu->nItems)
    {
        for (i = 0; i < menu->nItems; i++)
            draw_menu_item( hwnd, menu, hwnd, hdc, &menu->items[i], TRUE, ODA_DRAWENTIRE );

        retvalue = menu->Height;
    }
    else
    {
        retvalue = get_system_metrics( SM_CYMENU );
    }

    if (prev_font) NtGdiSelectFont( hdc, prev_font );
    return retvalue;
}

static UINT get_scroll_arrow_height( const POPUPMENU *menu )
{
    return menucharsize.cy + 4;
}

static void draw_scroll_arrow( HDC hdc, int x, int top, int height, BOOL up, BOOL enabled )
{
    RECT rect, light_rect;
    HBRUSH brush = get_sys_color_brush( enabled ? COLOR_BTNTEXT : COLOR_BTNSHADOW );
    HBRUSH light = get_sys_color_brush( COLOR_3DLIGHT );

    if (!up)
    {
        top = top + height;
        if (!enabled)
        {
            SetRect( &rect, x + 1, top, x + 2, top + 1);
            fill_rect( hdc, &rect, light );
        }
        top--;
    }

    SetRect( &rect, x, top, x + 1, top + 1);
    while (height--)
    {
        fill_rect( hdc, &rect, brush );
        if (!enabled && !up && height)
        {
            SetRect( &light_rect, rect.right, rect.top, rect.right + 2, rect.bottom );
            fill_rect( hdc, &light_rect, light );
        }
        InflateRect( &rect, 1, 0 );
        OffsetRect( &rect, 0, up ? 1 : -1 );
    }

    if (!enabled && up)
    {
        rect.left += 2;
        fill_rect( hdc, &rect, light );
    }
}

static void draw_scroll_arrows( const POPUPMENU *menu, HDC hdc )
{
    UINT full_height = get_scroll_arrow_height( menu );
    UINT arrow_height = full_height / 3;
    BOOL at_end = menu->nScrollPos + menu->items_rect.bottom - menu->items_rect.top == menu->nTotalHeight;

    draw_scroll_arrow( hdc, menu->Width / 3, arrow_height, arrow_height,
                       TRUE, menu->nScrollPos != 0);
    draw_scroll_arrow( hdc, menu->Width / 3, menu->Height - 2 * arrow_height, arrow_height,
                       FALSE, !at_end );
}

static int frame_rect( HDC hdc, const RECT *rect, HBRUSH hbrush )
{
    HBRUSH prev_brush;
    RECT r = *rect;

    if (IsRectEmpty(&r)) return 0;
    if (!(prev_brush = NtGdiSelectBrush( hdc, hbrush ))) return 0;

    NtGdiPatBlt( hdc, r.left, r.top, 1, r.bottom - r.top, PATCOPY );
    NtGdiPatBlt( hdc, r.right - 1, r.top, 1, r.bottom - r.top, PATCOPY );
    NtGdiPatBlt( hdc, r.left, r.top, r.right - r.left, 1, PATCOPY );
    NtGdiPatBlt( hdc, r.left, r.bottom - 1, r.right - r.left, 1, PATCOPY );

    NtGdiSelectBrush( hdc, prev_brush );
    return TRUE;
}

static void draw_popup_menu( HWND hwnd, HDC hdc, HMENU hmenu )
{
    HBRUSH prev_hrush, brush = get_sys_color_brush( COLOR_MENU );
    POPUPMENU *menu = unsafe_menu_ptr( hmenu );
    RECT rect;

    TRACE( "wnd=%p dc=%p menu=%p\n", hwnd, hdc, hmenu );

    get_client_rect( hwnd, &rect );

    if (menu && menu->hbrBack) brush = menu->hbrBack;
    if ((prev_hrush = NtGdiSelectBrush( hdc, brush ))
        && NtGdiSelectFont( hdc, get_menu_font( FALSE )))
    {
        HPEN prev_pen;

        NtGdiRectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );

        prev_pen = NtGdiSelectPen( hdc, GetStockObject( NULL_PEN ));
        if (prev_pen)
        {
            BOOL flat_menu = FALSE;

            NtUserSystemParametersInfo( SPI_GETFLATMENU, 0, &flat_menu, 0 );
            if (flat_menu)
                frame_rect( hdc, &rect, get_sys_color_brush( COLOR_BTNSHADOW ));
            else
                draw_rect_edge( hdc, &rect, EDGE_RAISED, BF_RECT, 1 );

            if (menu)
            {
                TRACE( "hmenu %p Style %08x\n", hmenu, menu->dwStyle );
                /* draw menu items */
                if (menu->nItems)
                {
                    MENUITEM *item;
                    UINT u;

                    item = menu->items;
                    for (u = menu->nItems; u > 0; u--, item++)
                        draw_menu_item( hwnd, menu, menu->hwndOwner, hdc,
                                        item, FALSE, ODA_DRAWENTIRE );
                }
                if (menu->bScrolling) draw_scroll_arrows( menu, hdc );
            }
        }
        else
        {
            NtGdiSelectBrush( hdc, prev_hrush );
        }
    }
}

LRESULT popup_menu_window_proc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam )
{
    TRACE( "hwnd=%p msg=0x%04x wp=0x%04lx lp=0x%08lx\n", hwnd, message, wparam, lparam );

    switch(message)
    {
    case WM_CREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
            NtUserSetWindowLongPtr( hwnd, 0, (LONG_PTR)cs->lpCreateParams, FALSE );
            return 0;
        }

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
        return MA_NOACTIVATE;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            NtUserBeginPaint( hwnd, &ps );
            draw_popup_menu( hwnd, ps.hdc, (HMENU)get_window_long_ptr( hwnd, 0, FALSE ));
            NtUserEndPaint( hwnd, &ps );
            return 0;
        }

    case WM_PRINTCLIENT:
        {
            draw_popup_menu( hwnd, (HDC)wparam, (HMENU)get_window_long_ptr( hwnd, 0, FALSE ));
            return 0;
        }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        break;

    case WM_SHOWWINDOW:
        if (wparam)
        {
            if (!get_window_long_ptr( hwnd, 0, FALSE )) ERR( "no menu to display\n" );
        }
        else
            NtUserSetWindowLongPtr( hwnd, 0, 0, FALSE );
        break;

    case MN_GETHMENU:
        return get_window_long_ptr( hwnd, 0, FALSE );

    default:
        return default_window_proc( hwnd, message, wparam, lparam, FALSE );
    }
    return 0;
}
