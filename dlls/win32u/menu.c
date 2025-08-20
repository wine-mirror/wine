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


/* menu item structure */
struct menu_item
{
    UINT      fType;          /* item type */
    UINT      fState;         /* item state */
    UINT_PTR  wID;            /* item id  */
    HMENU     hSubMenu;       /* pop-up menu */
    HBITMAP   hCheckBit;      /* bitmap when checked */
    HBITMAP   hUnCheckBit;    /* bitmap when unchecked */
    LPWSTR    text;           /* item text */
    ULONG_PTR dwItemData;     /* application defined */
    LPWSTR    dwTypeData;     /* depends on fMask */
    HBITMAP   hbmpItem;       /* bitmap */
    RECT      rect;           /* item area (relative to the items_rect), see adjust_menu_item_rect */
    UINT      xTab;           /* X position of text after Tab */
    SIZE      bmpsize;        /* size needed for the HBMMENU_CALLBACK bitmap */
};

/* menu user object */
struct menu
{
    HMENU       handle;         /* menu full handle */
    struct menu_item  *items;   /* array of menu items */
    WORD        wFlags;         /* menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        Width;          /* width of the whole menu */
    WORD        Height;         /* height of the whole menu */
    UINT        nItems;         /* number of items in the menu */
    HWND        hWnd;           /* window containing the menu */
    UINT        FocusedItem;    /* currently focused item */
    HWND        hwndOwner;      /* window receiving the messages for ownerdraw */
    BOOL        bScrolling;     /* scroll arrows are active */
    UINT        nScrollPos;     /* current scroll position */
    UINT        nTotalHeight;   /* total height of menu items inside menu */
    RECT        items_rect;     /* rectangle within which the items lie, excludes margins and scroll arrows */
    LONG        refcount;
    UINT        dwStyle;        /* extended menu style */
    UINT        cyMax;          /* max height of the whole menu, 0 is screen height */
    HBRUSH      hbrBack;        /* brush for menu background */
    DWORD       dwContextHelpID;
    ULONG_PTR   dwMenuData;     /* application defined value */
    HMENU       hSysMenuOwner;  /* handle to the dummy sys menu holder */
    WORD        textOffset;     /* offset of text when items have both bitmaps and text */
};

/* the accelerator user object */
struct accelerator
{
    unsigned int       count;
    ACCEL              table[1];
};

enum hittest
{
    ht_nowhere,     /* outside the menu */
    ht_border,      /* anywhere that's not an item or a scroll arrow */
    ht_item,        /* a menu item */
    ht_scroll_up,   /* scroll up arrow */
    ht_scroll_down  /* scroll down arrow */
};

typedef struct
{
    UINT   trackFlags;
    HMENU  hCurrentMenu; /* current submenu (can be equal to hTopMenu)*/
    HMENU  hTopMenu;     /* initial menu */
    HWND   hOwnerWnd;    /* where notifications are sent */
    POINT  pt;
} MTRACKER;

/* maximum allowed depth of any branch in the menu tree.
 * This value is slightly larger than in windows (25) to
 * stay on the safe side. */
#define MAXMENUDEPTH 30

/* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

/* internal flags for menu tracking */
#define TF_ENDMENU       0x10000
#define TF_SUSPENDPOPUP  0x20000
#define TF_SKIPREMOVE    0x40000
#define TF_RCVD_BTN_UP   0x80000

/* Internal track_menu() flags */
#define TPM_INTERNAL    0xf0000000
#define TPM_BUTTONDOWN  0x40000000  /* menu was clicked before tracking */
#define TPM_POPUPMENU   0x20000000  /* menu is a popup menu */

/* Space between 2 columns */
#define MENU_COL_SPACE  4

/* Margins for popup menus */
#define MENU_MARGIN  3

#define ITEM_PREV  -1
#define ITEM_NEXT  1

#define MENU_ITEM_TYPE(flags) \
    ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)
#define IS_MAGIC_BITMAP(id) ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define IS_SYSTEM_MENU(menu)  \
    (!((menu)->wFlags & MF_POPUP) && ((menu)->wFlags & MF_SYSMENU))

#define MENUITEMINFO_TYPE_MASK                                          \
    (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR |          \
     MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK |                \
     MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )
#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)
#define STATE_MASK (~TYPE_MASK)
#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))


/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */
static HWND top_popup;
static HMENU top_popup_hmenu;

/* Flag set by NtUserEndMenu() to force an exit from menu tracking */
static BOOL exit_menu = FALSE;

static SIZE menucharsize;
static UINT od_item_height; /* default owner drawn item height */

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
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return 0;
    }
    accel = malloc( FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    memcpy( accel->table, table, count * sizeof(*table) );

    if (!(handle = alloc_user_handle( accel, NTUSER_OBJ_ACCEL ))) free( accel );
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
      if (flags & (bit)) { flags &= ~(bit); len += snprintf(buf + len, sizeof(buf) - len, (text)); } \
  } while (0)

static const char *debugstr_menuitem( const struct menu_item *item )
{
    static const char *const hbmmenus[] = { "HBMMENU_CALLBACK", "", "HBMMENU_SYSTEM",
        "HBMMENU_MBAR_RESTORE", "HBMMENU_MBAR_MINIMIZE", "UNKNOWN BITMAP", "HBMMENU_MBAR_CLOSE",
        "HBMMENU_MBAR_CLOSE_D", "HBMMENU_MBAR_MINIMIZE_D", "HBMMENU_POPUP_CLOSE",
        "HBMMENU_POPUP_RESTORE", "HBMMENU_POPUP_MAXIMIZE", "HBMMENU_POPUP_MINIMIZE" };
    char buf[256];
    UINT flags;
    int len;

    if (!item) return "NULL";

    len = snprintf( buf, sizeof(buf), "{ ID=0x%lx", (long)item->wID );
    if (item->hSubMenu) len += snprintf( buf + len, sizeof(buf) - len, ", Sub=%p", item->hSubMenu );

    flags = item->fType;
    if (flags)
    {
        len += snprintf( buf + len, sizeof(buf) - len, ", fType=" );
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
        if (flags) len += snprintf( buf + len, sizeof(buf) - len, "+0x%x", flags );
    }

    flags = item->fState;
    if (flags)
    {
        len += snprintf( buf + len, sizeof(buf) - len, ", State=" );
        MENUFLAG( MFS_GRAYED, "grey" );
        MENUFLAG( MFS_DEFAULT, "default" );
        MENUFLAG( MFS_DISABLED, "dis" );
        MENUFLAG( MFS_CHECKED, "check" );
        MENUFLAG( MFS_HILITE, "hi" );
        MENUFLAG( MF_USECHECKBITMAPS, "usebit" );
        MENUFLAG( MF_MOUSESELECT, "mouse" );
        if (flags) len += snprintf( buf + len, sizeof(buf) - len, "+0x%x", flags );
    }

    if (item->hCheckBit)   len += snprintf( buf + len, sizeof(buf) - len, ", Chk=%p", item->hCheckBit );
    if (item->hUnCheckBit) len += snprintf( buf + len, sizeof(buf) - len, ", Unc=%p", item->hUnCheckBit );
    if (item->text)        len += snprintf( buf + len, sizeof(buf) - len, ", Text=%s", debugstr_w(item->text) );
    if (item->dwItemData)  len += snprintf( buf + len, sizeof(buf) - len, ", ItemData=0x%08lx", item->dwItemData );

    if (item->hbmpItem)
    {
        if (IS_MAGIC_BITMAP( item->hbmpItem ))
            len += snprintf( buf + len, sizeof(buf) - len, ", hbitmap=%s", hbmmenus[(INT_PTR)item->hbmpItem + 1] );
        else
            len += snprintf( buf + len, sizeof(buf) - len, ", hbitmap=%p", item->hbmpItem );
    }
    return wine_dbg_sprintf( "%s  }", buf );
}

#undef MENUFLAG

static struct menu *grab_menu_ptr( HMENU handle )
{
    struct menu *menu = get_user_handle_ptr( handle, NTUSER_OBJ_MENU );

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

static void release_menu_ptr( struct menu *menu )
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
static struct menu *unsafe_menu_ptr( HMENU handle )
{
    struct menu *menu = grab_menu_ptr( handle );
    if (menu) release_menu_ptr( menu );
    return menu;
}

/* see IsMenu */
BOOL is_menu( HMENU handle )
{
    struct menu *menu;
    BOOL is_menu;

    menu = grab_menu_ptr( handle );
    is_menu = menu != NULL;
    release_menu_ptr( menu );

    if (!is_menu) RtlSetLastWin32Error( ERROR_INVALID_MENU_HANDLE );
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

static struct menu *find_menu_item( HMENU handle, UINT id, UINT flags, UINT *pos )
{
    UINT fallback_pos = ~0u, i;
    struct menu *menu;

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
        struct menu_item *item = menu->items;
        for (i = 0; i < menu->nItems; i++, item++)
        {
            if (item->fType & MF_POPUP)
            {
                struct menu *submenu = find_menu_item( item->hSubMenu, id, flags, pos );

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

static struct menu *insert_menu_item( HMENU handle, UINT id, UINT flags, UINT *ret_pos )
{
    struct menu_item *new_items;
    struct menu *menu;
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

    new_items = malloc( sizeof(*new_items) * (menu->nItems + 1) );
    if (!new_items)
    {
        release_menu_ptr( menu );
        return NULL;
    }
    if (menu->nItems > 0)
    {
        /* Copy the old array into the new one */
        if (pos > 0) memcpy( new_items, menu->items, pos * sizeof(*new_items) );
        if (pos < menu->nItems) memcpy( &new_items[pos + 1], &menu->items[pos],
                                        (menu->nItems - pos) * sizeof(*new_items) );
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
    struct menu *menu;
    struct menu_item *item;
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

/* Adjust menu item rectangle according to scrolling state */
static void adjust_menu_item_rect( const struct menu *menu, RECT *rect )
{
    INT scroll_offset = menu->bScrolling ? menu->nScrollPos : 0;
    OffsetRect( rect, menu->items_rect.left, menu->items_rect.top - scroll_offset );
}

/***********************************************************************
 *           find_item_by_coords
 *
 * Find the item at the specified coordinates (screen coords). Does
 * not work for child windows and therefore should not be called for
 * an arbitrary system menu.
 *
 * Returns a hittest code.  *pos will contain the position of the
 * item or NO_SELECTED_ITEM.  If the hittest code is ht_scroll_up
 * or ht_scroll_down then *pos will contain the position of the
 * item that's just outside the items_rect - ie, the one that would
 * be scrolled completely into view.
 */
static enum hittest find_item_by_coords( const struct menu *menu, POINT pt, UINT *pos )
{
    enum hittest ht = ht_border;
    struct menu_item *item;
    RECT rect;
    UINT i;

    *pos = NO_SELECTED_ITEM;

    if (!get_window_rect( menu->hWnd, &rect, get_thread_dpi() ) || !PtInRect( &rect, pt ))
        return ht_nowhere;

    if (get_window_long( menu->hWnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) pt.x = rect.right - 1 - pt.x;
    else pt.x -= rect.left;
    pt.y -= rect.top;

    if (!PtInRect( &menu->items_rect, pt ))
    {
        if (!menu->bScrolling || pt.x < menu->items_rect.left || pt.x >= menu->items_rect.right)
            return ht_border;

        /* On a scroll arrow. Update pt so that it points to the item just outside items_rect */
        if (pt.y < menu->items_rect.top)
        {
            ht = ht_scroll_up;
            pt.y = menu->items_rect.top - 1;
        }
        else
        {
            ht = ht_scroll_down;
            pt.y = menu->items_rect.bottom;
        }
    }

    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
        rect = item->rect;
        adjust_menu_item_rect( menu, &rect );
        if (PtInRect( &rect, pt ))
        {
            *pos = i;
            if (ht != ht_scroll_up && ht != ht_scroll_down) ht = ht_item;
            break;
        }
    }

    return ht;
}

/* see GetMenu */
HMENU get_menu( HWND hwnd )
{
    return UlongToHandle( get_window_long( hwnd, GWLP_ID ));
}

/* see CreateMenu and CreatePopupMenu */
static HMENU create_menu( BOOL is_popup )
{
    struct menu *menu;
    HMENU handle;

    if (!(menu = calloc( 1, sizeof(*menu) ))) return 0;
    menu->FocusedItem = NO_SELECTED_ITEM;
    menu->refcount = 1;
    if (is_popup) menu->wFlags |= MF_POPUP;

    if (!(handle = alloc_user_handle( menu, NTUSER_OBJ_MENU ))) free( menu );
    else menu->handle = handle;

    TRACE( "return %p\n", handle );
    return handle;
}

/**********************************************************************
 *         NtUserCreateMenu   (win32u.@)
 */
HMENU WINAPI NtUserCreateMenu(void)
{
    return create_menu( FALSE );
}

/**********************************************************************
 *         NtUserCreatePopupMenu   (win32u.@)
 */
HMENU WINAPI NtUserCreatePopupMenu(void)
{
    return create_menu( TRUE );
}

/**********************************************************************
 *         NtUserDestroyMenu   (win32u.@)
 */
BOOL WINAPI NtUserDestroyMenu( HMENU handle )
{
    struct menu *menu;

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
        struct menu_item *item = menu->items;
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
        struct menu *menu;

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
    struct menu_item *item;
    struct menu *menu;
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
    struct menu *menu;
    struct menu_item *item;

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
        struct menu *parent_menu;
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
        get_window_rect_rel( hwnd, COORDS_CLIENT, &rc, get_thread_dpi() );
        rc.bottom = 0;
        NtUserRedrawWindow( hwnd, &rc, 0, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN );
    }
    else
        release_menu_ptr( menu );

    return oldflags;
}

/**********************************************************************
 *           NtUserDrawMenuBar    (win32u.@)
 */
BOOL WINAPI NtUserDrawMenuBar( HWND hwnd )
{
    HMENU handle;

    if (!is_window( hwnd )) return FALSE;
    if (is_win_menu_disallowed( hwnd )) return TRUE;

    if ((handle = get_menu( hwnd )))
    {
        struct menu *menu = grab_menu_ptr( handle );
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
    struct menu *menu;
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
    struct menu *menu;

    if (!(menu = grab_menu_ptr( handle ))) return FALSE;

    if (info->fMask & MIM_BACKGROUND) menu->hbrBack = info->hbrBack;
    if (info->fMask & MIM_HELPID)     menu->dwContextHelpID = info->dwContextHelpID;
    if (info->fMask & MIM_MAXHEIGHT)  menu->cyMax = info->cyMax;
    if (info->fMask & MIM_MENUDATA)   menu->dwMenuData = info->dwMenuData;
    if (info->fMask & MIM_STYLE)      menu->dwStyle = info->dwStyle;

    if (info->fMask & MIM_APPLYTOSUBMENUS)
    {
        int i;
        struct menu_item *item = menu->items;
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
        RtlSetLastWin32Error( ERROR_NOACCESS );
        return FALSE;
    }

    if (!set_menu_info( menu, info ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_MENU_HANDLE );
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
    struct menu *menu;

    TRACE( "(%p %p)\n", handle, info );

    if (!info || info->cbSize != sizeof(MENUINFO) || !(menu = grab_menu_ptr( handle )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER);
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
static int menu_depth( struct menu *pmenu, int depth)
{
    int i, subdepth;
    struct menu_item *item;

    if (++depth > MAXMENUDEPTH) return depth;
    item = pmenu->items;
    subdepth = depth;
    for (i = 0; i < pmenu->nItems && subdepth <= MAXMENUDEPTH; i++, item++)
    {
        struct menu *submenu = item->hSubMenu ? grab_menu_ptr( item->hSubMenu ) : NULL;
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

static BOOL set_menu_item_info( struct menu_item *menu, const MENUITEMINFOW *info )
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
            struct menu *submenu = grab_menu_ptr( menu->hSubMenu );
            if (!submenu)
            {
                RtlSetLastWin32Error( ERROR_INVALID_PARAMETER);
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
    struct menu *menu;
    UINT state, pos;
    struct menu_item *item;

    TRACE( "(menu=%p, id=%04x, flags=%04x);\n", handle, item_id, flags );

    if (!(menu = find_menu_item( handle, item_id, flags, &pos )))
        return -1;

    item = &menu->items[pos];
    TRACE( "  item: %s\n", debugstr_menuitem( item ));
    if (item->fType & MF_POPUP)
    {
        struct menu *submenu = grab_menu_ptr( item->hSubMenu );
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

static BOOL get_menu_item_info( HMENU handle, UINT id, UINT flags, MENUITEMINFOW *info, BOOL ansi )
{
    struct menu *menu;
    struct menu_item *item;
    UINT pos;

    if (!info || info->cbSize != sizeof(*info))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    menu = find_menu_item( handle, id, flags, &pos );
    item = menu ? &menu->items[pos] : NULL;
    TRACE( "%s\n", debugstr_menuitem( item ));
    if (!menu)
    {
        RtlSetLastWin32Error( ERROR_MENU_ITEM_NOT_FOUND);
        return FALSE;
    }

    if (info->fMask & MIIM_TYPE)
    {
        if (info->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP))
        {
            release_menu_ptr( menu );
            WARN( "invalid combination of fMask bits used\n" );
            /* this does not happen on Win9x/ME */
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        info->fType = item->fType & MENUITEMINFO_TYPE_MASK;
        if (item->hbmpItem && !IS_MAGIC_BITMAP(item->hbmpItem))
            info->fType |= MFT_BITMAP;
        info->hbmpItem = item->hbmpItem; /* not on Win9x/ME */
        if (info->fType & MFT_BITMAP)
        {
            info->dwTypeData = (WCHAR *)item->hbmpItem;
            info->cch = 0;
        }
        else if (info->fType & (MFT_OWNERDRAW | MFT_SEPARATOR))
        {
            /* this does not happen on Win9x/ME */
            info->dwTypeData = 0;
            info->cch = 0;
        }
    }

    /* copy the text string */
    if ((info->fMask & (MIIM_TYPE|MIIM_STRING)))
    {
        if (!item->text)
        {
            if (info->dwTypeData && info->cch)
            {
                if (ansi)
                    *((char *)info->dwTypeData) = 0;
                else
                    *((WCHAR *)info->dwTypeData) = 0;
            }
            info->cch = 0;
        }
        else
        {
            DWORD len, text_len;
            if (ansi)
            {
                text_len = wcslen( item->text );
                len = win32u_wctomb_size( &ansi_cp, item->text, text_len );
                if (info->dwTypeData && info->cch)
                    if (!win32u_wctomb( &ansi_cp, (char *)info->dwTypeData, info->cch,
                                        item->text, text_len + 1 ))
                        ((char *)info->dwTypeData)[info->cch - 1] = 0;
            }
            else
            {
                len = lstrlenW( item->text );
                if (info->dwTypeData && info->cch)
                    lstrcpynW( info->dwTypeData, item->text, info->cch );
            }

            if (info->dwTypeData && info->cch)
            {
                /* if we've copied a substring we return its length */
                if (info->cch <= len + 1)
                    info->cch--;
                else
                    info->cch = len;
            }
            else
            {
                /* return length of string, not on Win9x/ME if fType & MFT_BITMAP */
                info->cch = len;
            }
        }
    }

    if (info->fMask & MIIM_FTYPE)  info->fType = item->fType & MENUITEMINFO_TYPE_MASK;
    if (info->fMask & MIIM_BITMAP) info->hbmpItem = item->hbmpItem;
    if (info->fMask & MIIM_STATE)  info->fState = item->fState & MENUITEMINFO_STATE_MASK;
    if (info->fMask & MIIM_ID)     info->wID = item->wID;
    if (info->fMask & MIIM_DATA)   info->dwItemData = item->dwItemData;

    if (info->fMask & MIIM_SUBMENU) info->hSubMenu = item->hSubMenu;
    else info->hSubMenu = 0; /* hSubMenu is always cleared (not on Win9x/ME ) */

    if (info->fMask & MIIM_CHECKMARKS)
    {
        info->hbmpChecked = item->hCheckBit;
        info->hbmpUnchecked = item->hUnCheckBit;
    }

    release_menu_ptr( menu );
    return TRUE;
}

static BOOL check_menu_radio_item( HMENU handle, UINT first, UINT last, UINT check, UINT flags )
{
    struct menu *first_menu = NULL, *check_menu;
    UINT i, check_pos;
    BOOL done = FALSE;

    for (i = first; i <= last; i++)
    {
        struct menu_item *item;

        if (!(check_menu = find_menu_item( handle, i, flags, &check_pos ))) continue;
        if (!first_menu) first_menu = grab_menu_ptr( check_menu->handle );

        if (first_menu != check_menu)
        {
            release_menu_ptr(check_menu);
            continue;
        }

        item = &check_menu->items[check_pos];
        if (item->fType != MFT_SEPARATOR)
        {
            if (i == check)
            {
                item->fType |= MFT_RADIOCHECK;
                item->fState |= MFS_CHECKED;
                done = TRUE;
            }
            else
            {
                /* Windows does not remove MFT_RADIOCHECK */
                item->fState &= ~MFS_CHECKED;
            }
        }

        release_menu_ptr( check_menu );
    }

    release_menu_ptr( first_menu );
    return done;
}

/* see GetSubMenu */
static HMENU get_sub_menu( HMENU handle, INT pos )
{
    struct menu *menu;
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

/* see GetMenuDefaultItem */
static UINT get_menu_default_item( HMENU handle, UINT bypos, UINT flags )
{
    struct menu_item *item = NULL;
    struct menu *menu;
    UINT i;

    TRACE( "(%p,%d,%d)\n", handle, bypos, flags );

    if (!(menu = grab_menu_ptr( handle ))) return -1;

    for (i = 0; i < menu->nItems; i++)
    {
        if (!(menu->items[i].fState & MFS_DEFAULT)) continue;
        item = &menu->items[i];
        break;
    }

    /* default: don't return disabled items */
    if (item && (!(GMDI_USEDISABLED & flags)) && (item->fState & MFS_DISABLED)) item = NULL;

    /* search submenu when needed */
    if (item && (item->fType & MF_POPUP) && (flags & GMDI_GOINTOPOPUPS))
    {
        UINT ret = get_menu_default_item( item->hSubMenu, bypos, flags );
        if (ret != -1)
        {
            release_menu_ptr( menu );
            return ret;
        }
        /* when item not found in submenu, return the popup item */
    }

    if (!item) i = -1;
    else if (!bypos) i = item->wID;
    release_menu_ptr( menu );
    return i;
}

/**********************************************************************
 *           NtUserThunkedMenuItemInfo    (win32u.@)
 */
UINT WINAPI NtUserThunkedMenuItemInfo( HMENU handle, UINT pos, UINT flags, UINT method,
                                       MENUITEMINFOW *info, UNICODE_STRING *str )
{
    struct menu *menu;
    UINT i;
    BOOL ret;

    switch (method)
    {
    case NtUserCheckMenuRadioItem:
        return check_menu_radio_item( handle, pos, info->cch, info->fMask, flags );

    case NtUserGetMenuDefaultItem:
        return get_menu_default_item( handle, pos, flags );

    case NtUserGetMenuItemID:
        if (!(menu = find_menu_item( handle, pos, flags, &i ))) return -1;
        ret = menu->items[i].fType & MF_POPUP ? -1 : menu->items[i].wID;
        release_menu_ptr( menu );
        break;

    case NtUserGetMenuItemInfoA:
        return get_menu_item_info( handle, pos, flags, info, TRUE );

    case NtUserGetMenuItemInfoW:
        return get_menu_item_info( handle, pos, flags, info, FALSE );

    case NtUserGetSubMenu:
        return HandleToUlong( get_sub_menu( handle, pos ));

    case NtUserInsertMenuItem:
        if (!info || info->cbSize != sizeof(*info))
        {
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
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
            RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
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
    struct menu *menu;
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
    struct menu *menu;
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
        struct menu_item *new_items, *item = &menu->items[pos];

        while (pos < menu->nItems)
        {
            *item = item[1];
            item++;
            pos++;
        }
        new_items = realloc( menu->items, menu->nItems * sizeof(*item) );
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
    struct menu *menu;
    UINT pos;

    if (!(menu = find_menu_item( handle, id, flags, &pos )))
        return FALSE;

    if (menu->items[pos].fType & MF_POPUP)
        NtUserDestroyMenu( menu->items[pos].hSubMenu );

    NtUserRemoveMenu( menu->handle, pos, flags | MF_BYPOSITION );
    release_menu_ptr( menu );
    return TRUE;
}

/**********************************************************************
 *           NtUserSetMenuContextHelpId    (win32u.@)
 */
BOOL WINAPI NtUserSetMenuContextHelpId( HMENU handle, DWORD id )
{
    struct menu *menu;

    TRACE( "(%p 0x%08x)\n", handle, id );

    if (!(menu = grab_menu_ptr( handle ))) return FALSE;
    menu->dwContextHelpID = id;
    release_menu_ptr( menu );
    return TRUE;
}

/***********************************************************************
 *           copy_sys_popup
 *
 * Return the default system menu.
 */
static HMENU copy_sys_popup( BOOL mdi )
{
    struct load_sys_menu_params params;
    MENUITEMINFOW item_info;
    MENUINFO menu_info;
    struct menu *menu;
    void *ret_ptr;
    ULONG ret_len;
    NTSTATUS status;
    HMENU handle = 0;

    params.mdi = mdi;
    status = KeUserModeCallback( NtUserLoadSysMenu, &params, sizeof(params), &ret_ptr, &ret_len );
    if (!status && ret_len == sizeof(HMENU)) handle = *(HMENU *)ret_ptr;
    if (!handle || !(menu = grab_menu_ptr( handle )))
    {
        ERR("Unable to load default system menu\n" );
        return 0;
    }

    menu->wFlags |= MF_SYSMENU | MF_POPUP;
    release_menu_ptr( menu );

    /* decorate the menu with bitmaps */
    menu_info.cbSize = sizeof(MENUINFO);
    menu_info.dwStyle = MNS_CHECKORBMP;
    menu_info.fMask = MIM_STYLE;
    NtUserThunkedMenuInfo( handle, &menu_info );
    item_info.cbSize = sizeof(MENUITEMINFOW);
    item_info.fMask = MIIM_BITMAP;
    item_info.hbmpItem = HBMMENU_POPUP_CLOSE;
    NtUserThunkedMenuItemInfo( handle, SC_CLOSE, 0, NtUserSetMenuItemInfo, &item_info, NULL );
    item_info.hbmpItem = HBMMENU_POPUP_RESTORE;
    NtUserThunkedMenuItemInfo( handle, SC_RESTORE, 0, NtUserSetMenuItemInfo, &item_info, NULL );
    item_info.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
    NtUserThunkedMenuItemInfo( handle, SC_MAXIMIZE, 0, NtUserSetMenuItemInfo, &item_info, NULL );
    item_info.hbmpItem = HBMMENU_POPUP_MINIMIZE;
    NtUserThunkedMenuItemInfo( handle, SC_MINIMIZE, 0, NtUserSetMenuItemInfo, &item_info, NULL );
    NtUserSetMenuDefaultItem( handle, SC_CLOSE, FALSE );

    TRACE( "returning %p (mdi=%d).\n", handle, mdi );
    return handle;
}

/**********************************************************************
 *           get_sys_menu
 *
 * Create a copy of the system menu. System menu in Windows is
 * a special menu bar with the single entry - system menu popup.
 * This popup is presented to the outside world as a "system menu".
 * However, the real system menu handle is sometimes seen in the
 * WM_MENUSELECT parameters (and Word 6 likes it this way).
 */
static HMENU get_sys_menu( HWND hwnd, HMENU popup_menu )
{
    MENUITEMINFOW info;
    struct menu *menu;
    HMENU handle;

    TRACE("loading system menu, hwnd %p, popup_menu %p\n", hwnd, popup_menu);
    if (!(handle = NtUserCreateMenu()))
    {
        ERR("failed to load system menu!\n");
        return 0;
    }

    if (!(menu = grab_menu_ptr( handle )))
    {
        NtUserDestroyMenu( handle );
        return 0;
    }
    menu->wFlags = MF_SYSMENU;
    menu->hWnd = get_full_window_handle( hwnd );
    release_menu_ptr( menu );
    TRACE("hwnd %p (handle %p)\n", menu->hWnd, handle);

    if (!popup_menu)
    {
        if (get_window_long(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
            popup_menu = copy_sys_popup(TRUE);
        else
            popup_menu = copy_sys_popup(FALSE);
    }
    if (!popup_menu)
    {
        NtUserDestroyMenu( handle );
        return 0;
    }

    if (get_class_long( hwnd, GCL_STYLE, FALSE ) & CS_NOCLOSE)
        NtUserDeleteMenu(popup_menu, SC_CLOSE, MF_BYCOMMAND);

    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE | MIIM_SUBMENU;
    info.fState = 0;
    info.fType = MF_SYSMENU | MF_POPUP;
    info.wID = (UINT_PTR)popup_menu;
    info.hSubMenu = popup_menu;

    NtUserThunkedMenuItemInfo( handle, -1, MF_SYSMENU | MF_POPUP | MF_BYPOSITION,
                               NtUserInsertMenuItem, &info, NULL );

    if ((menu = grab_menu_ptr( handle )))
    {
        menu->items[0].fType = MF_SYSMENU | MF_POPUP;
        menu->items[0].fState = 0;
        release_menu_ptr( menu );
    }
    if ((menu = grab_menu_ptr(popup_menu)))
    {
        menu->wFlags |= MF_SYSMENU;
        release_menu_ptr( menu );
    }

    TRACE("handle=%p (hPopup %p)\n", handle, popup_menu );
    return handle;
}

/**********************************************************************
 *           NtUserMenuItemFromPoint    (win32u.@)
 */
INT WINAPI NtUserMenuItemFromPoint( HWND hwnd, HMENU handle, int x, int y )
{
    POINT pt = { .x = x, .y = y };
    struct menu *menu;
    UINT pos;

    if (!(menu = grab_menu_ptr(handle))) return -1;
    if (find_item_by_coords( menu, pt, &pos ) != ht_item) pos = -1;
    release_menu_ptr(menu);
    return pos;
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

    if (!win->hSysMenu && (win->dwStyle & WS_SYSMENU))
        win->hSysMenu = get_sys_menu( hwnd, 0 );

    if (win->hSysMenu)
    {
        struct menu *menu;
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
    win->hSysMenu = get_sys_menu( hwnd, menu );
    release_win_ptr( win );
    return TRUE;
}

HMENU get_window_sys_sub_menu( HWND hwnd )
{
    WND *win;
    HMENU ret;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return 0;
    ret = win->hSysMenu;
    release_win_ptr( win );
    return get_sub_menu( ret, 0 );
}

/**********************************************************************
 *           NtUserSetMenuDefaultItem    (win32u.@)
 */
BOOL WINAPI NtUserSetMenuDefaultItem( HMENU handle, UINT item, UINT bypos )
{
    struct menu_item *menu_item;
    struct menu *menu;
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
            TRACE_(accel)( "found accel for virt_key %04x (scan %04x)\n",
                           key, 0xff & HIWORD(lparam) );

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
        struct menu *menu;

        menu_handle = (get_window_long( hwnd, GWL_STYLE ) & WS_CHILD) ? 0 : get_menu(hwnd);
        sys_menu = get_win_sys_menu( hwnd );

        /* find menu item and ask application to initialize it */
        /* 1. in the system menu */
        if ((menu = find_menu_item( sys_menu, cmd, MF_BYCOMMAND, NULL )))
        {
            submenu = menu->handle;
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
                submenu = menu->handle;
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
                  accel, hwnd, msg->hwnd, msg->message, (long)msg->wParam, msg->lParam);

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
static void get_bitmap_item_size( struct menu_item *item, SIZE *size, HWND owner )
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
static void calc_menu_item_size( HDC hdc, struct menu_item *item, HWND owner, INT org_x, INT org_y,
                                 BOOL menu_bar, struct menu *menu )
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
        int height;
        menucharsize.cx = get_char_dimensions( hdc, NULL, &height );
        menucharsize.cy = height;
        /* Win95/98/ME will use menucharsize.cy here. Testing is possible
         * but it is unlikely an application will depend on that */
        od_item_height = HIWORD( get_dialog_base_units() );
    }

    SetRect( &item->rect, org_x, org_y, org_x, org_y );

    if (item->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.CtlID      = 0;
        mis.itemID     = item->wID;
        mis.itemData   = item->dwItemData;
        mis.itemHeight = od_item_height;
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

        TRACE( "id=%04lx size=%dx%d\n", (long)item->wID, item->rect.right - item->rect.left,
               item->rect.bottom - item->rect.top );
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
static void calc_menu_bar_size( HDC hdc, RECT *rect, struct menu *menu, HWND owner )
{
    UINT start, i, help_pos;
    int org_x, org_y;
    struct menu_item *item;

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
    struct menu *menu;
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

static void draw_bitmap_item( HWND hwnd, HDC hdc, struct menu_item *item, const RECT *rect,
                              struct menu *menu, HWND owner, UINT odaction )
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
        BOOL down = FALSE, grayed = FALSE;
        enum NONCLIENT_BUTTON_TYPE type;
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
            type = MENU_RESTORE_BUTTON;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
            type = MENU_MIN_BUTTON;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
            type = MENU_MIN_BUTTON;
            grayed = TRUE;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE:
            type = MENU_CLOSE_BUTTON;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE_D:
            type = MENU_CLOSE_BUTTON;
            grayed = TRUE;
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
                drawItem.hwndItem = (HWND)menu->handle;
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
            if (item->fState & MF_HILITE) down = TRUE;
            draw_menu_button( hwnd, hdc, &r, type, down, grayed );
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

/* Draw a single menu item */
static void draw_menu_item( HWND hwnd, struct menu *menu, HWND owner, HDC hdc,
                            struct menu_item *item, BOOL menu_bar, UINT odaction )
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
        dis.hwndItem   = (HWND)menu->handle;
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
            draw_bitmap_item( hwnd, hdc, item, &bmprc, menu, owner, odaction );
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
        draw_bitmap_item( hwnd, hdc, item, &bmprc, menu, owner, odaction );
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
    struct menu *menu;
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

static UINT get_scroll_arrow_height( const struct menu *menu )
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

static void draw_scroll_arrows( const struct menu *menu, HDC hdc )
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
    struct menu *menu = unsafe_menu_ptr( hmenu );
    RECT rect;

    TRACE( "wnd=%p dc=%p menu=%p\n", hwnd, hdc, hmenu );

    get_client_rect( hwnd, &rect, get_thread_dpi() );

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
                    struct menu_item *item;
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

LRESULT popup_menu_window_proc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, BOOL ansi )
{
    TRACE( "hwnd=%p msg=0x%04x wp=0x%04lx lp=0x%08lx\n", hwnd, message, (long)wparam, lparam );

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
        /* zero out global pointer in case resident popup window was destroyed. */
        if (hwnd == top_popup)
        {
            top_popup = 0;
            top_popup_hmenu = NULL;
        }
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
        return default_window_proc( hwnd, message, wparam, lparam, ansi );
    }
    return 0;
}

HWND is_menu_active(void)
{
    return top_popup;
}

/* Calculate the size of a popup menu */
static void calc_popup_menu_size( struct menu *menu, UINT max_height )
{
    BOOL textandbmp = FALSE, multi_col = FALSE;
    int org_x, org_y, max_tab, max_tab_width;
    struct menu_item *item;
    UINT start, i;
    HDC hdc;

    menu->Width = menu->Height = 0;
    SetRectEmpty( &menu->items_rect );

    if (menu->nItems == 0) return;
    hdc = NtUserGetDC( 0 );

    NtGdiSelectFont( hdc, get_menu_font( FALSE ));

    start = 0;
    menu->textOffset = 0;

    while (start < menu->nItems)
    {
        item = &menu->items[start];
        org_x = menu->items_rect.right;
        if (item->fType & (MF_MENUBREAK | MF_MENUBARBREAK))
            org_x += MENU_COL_SPACE;
        org_y = menu->items_rect.top;

        max_tab = max_tab_width = 0;
        /* Parse items until column break or end of menu */
        for (i = start; i < menu->nItems; i++, item++)
        {
            if (item->fType & (MF_MENUBREAK | MF_MENUBARBREAK))
            {
                multi_col = TRUE;
                if (i != start) break;
            }

            calc_menu_item_size( hdc, item, menu->hwndOwner, org_x, org_y, FALSE, menu );
            menu->items_rect.right = max( menu->items_rect.right, item->rect.right );
            org_y = item->rect.bottom;
            if (IS_STRING_ITEM( item->fType ) && item->xTab)
            {
                max_tab = max( max_tab, item->xTab );
                max_tab_width = max( max_tab_width, item->rect.right-item->xTab );
            }
            if (item->text && item->hbmpItem) textandbmp = TRUE;
        }

        /* Finish the column (set all items to the largest width found) */
        menu->items_rect.right = max( menu->items_rect.right, max_tab + max_tab_width );
        for (item = &menu->items[start]; start < i; start++, item++)
        {
            item->rect.right = menu->items_rect.right;
            if (IS_STRING_ITEM( item->fType ) && item->xTab)
                item->xTab = max_tab;
        }
        menu->items_rect.bottom = max( menu->items_rect.bottom, org_y );
    }

    /* If none of the items have both text and bitmap then
     * the text and bitmaps are all aligned on the left. If there is at
     * least one item with both text and bitmap then bitmaps are
     * on the left and texts left aligned with the right hand side
     * of the bitmaps */
    if (!textandbmp) menu->textOffset = 0;

    menu->nTotalHeight = menu->items_rect.bottom;

    /* space for the border */
    OffsetRect( &menu->items_rect, MENU_MARGIN, MENU_MARGIN );
    menu->Height = menu->items_rect.bottom + MENU_MARGIN;
    menu->Width = menu->items_rect.right + MENU_MARGIN;

    /* Adjust popup height if it exceeds maximum */
    if (menu->Height >= max_height)
    {
        menu->Height = max_height;
        menu->bScrolling = !multi_col;
        /* When the scroll arrows are present, don't add the top/bottom margin as well */
        if (menu->bScrolling)
        {
            menu->items_rect.top = get_scroll_arrow_height( menu );
            menu->items_rect.bottom = menu->Height - get_scroll_arrow_height( menu );
        }
    }
    else
    {
        menu->bScrolling = FALSE;
    }

    NtUserReleaseDC( 0, hdc );
}

static BOOL show_popup( HWND owner, HMENU hmenu, UINT id, UINT flags,
                        int x, int y, INT xanchor, INT yanchor )
{
    struct menu *menu;
    MONITORINFO info;
    UINT max_height;
    RECT rect;

    TRACE( "owner=%p hmenu=%p id=0x%04x x=0x%04x y=0x%04x xa=0x%04x ya=0x%04x\n",
           owner, hmenu, id, x, y, xanchor, yanchor );

    if (!(menu = unsafe_menu_ptr( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
        menu->items[menu->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
        menu->FocusedItem = NO_SELECTED_ITEM;
    }

    menu->nScrollPos = 0;

    /* FIXME: should use item rect */
    SetRect( &rect, x, y, x, y );
    info = monitor_info_from_rect( rect, get_thread_dpi() );

    max_height = info.rcWork.bottom - info.rcWork.top;
    if (menu->cyMax) max_height = min( max_height, menu->cyMax );
    calc_popup_menu_size( menu, max_height );

    /* adjust popup menu pos so that it fits within the desktop */
    if (flags & TPM_LAYOUTRTL) flags ^= TPM_RIGHTALIGN;

    if (flags & TPM_RIGHTALIGN) x -= menu->Width;
    if (flags & TPM_CENTERALIGN) x -= menu->Width / 2;

    if (flags & TPM_BOTTOMALIGN) y -= menu->Height;
    if (flags & TPM_VCENTERALIGN) y -= menu->Height / 2;

    if (x + menu->Width > info.rcWork.right)
    {
        if (xanchor && x >= menu->Width - xanchor) x -= menu->Width - xanchor;
        if (x + menu->Width > info.rcWork.right) x = info.rcWork.right - menu->Width;
    }
    if (x < info.rcWork.left) x = info.rcWork.left;

    if (y + menu->Height > info.rcWork.bottom)
    {
        if (yanchor && y >= menu->Height + yanchor) y -= menu->Height + yanchor;
        if (y + menu->Height > info.rcWork.bottom) y = info.rcWork.bottom - menu->Height;
    }
    if (y < info.rcWork.top) y = info.rcWork.top;

    if (!top_popup)
    {
        top_popup = menu->hWnd;
        top_popup_hmenu = hmenu;
    }

    /* Display the window */
    NtUserSetWindowPos( menu->hWnd, HWND_TOPMOST, x, y, menu->Width, menu->Height,
                        SWP_SHOWWINDOW | SWP_NOACTIVATE );
    NtUserRedrawWindow( menu->hWnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}

static void ensure_menu_item_visible( struct menu *menu, UINT index, HDC hdc )
{
    if (menu->bScrolling)
    {
        struct menu_item *item = &menu->items[index];
        UINT prev_pos = menu->nScrollPos;
        const RECT *rc = &menu->items_rect;
        UINT scroll_height = rc->bottom - rc->top;

        if (item->rect.bottom > menu->nScrollPos + scroll_height)
        {
            menu->nScrollPos = item->rect.bottom - scroll_height;
            NtUserScrollWindowEx( menu->hWnd, 0, prev_pos - menu->nScrollPos, rc, rc, 0, NULL,
                                  SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN | SW_NODCCACHE );
        }
        else if (item->rect.top < menu->nScrollPos)
        {
            menu->nScrollPos = item->rect.top;
            NtUserScrollWindowEx( menu->hWnd, 0, prev_pos - menu->nScrollPos, rc, rc, 0, NULL,
                                  SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN | SW_NODCCACHE );
        }

        /* Invalidate the scroll arrows if necessary */
        if (prev_pos != menu->nScrollPos)
        {
            RECT arrow_rect = menu->items_rect;
            if (prev_pos == 0 || menu->nScrollPos == 0)
            {
                arrow_rect.top = 0;
                arrow_rect.bottom = menu->items_rect.top;
                NtUserInvalidateRect( menu->hWnd, &arrow_rect, FALSE );
            }
            if (prev_pos + scroll_height == menu->nTotalHeight ||
                menu->nScrollPos + scroll_height == menu->nTotalHeight)
            {
                arrow_rect.top = menu->items_rect.bottom;
                arrow_rect.bottom = menu->Height;
                NtUserInvalidateRect( menu->hWnd, &arrow_rect, FALSE );
            }
        }
    }
}

static void select_item( HWND owner, HMENU hmenu, UINT index, BOOL send_select, HMENU topmenu )
{
    struct menu *menu;
    HDC hdc;

    TRACE( "owner %p menu %p index 0x%04x select 0x%04x\n", owner, hmenu, index, send_select );

    menu = unsafe_menu_ptr( hmenu );
    if (!menu || !menu->nItems || !menu->hWnd) return;

    if (menu->FocusedItem == index) return;
    if (menu->wFlags & MF_POPUP) hdc = NtUserGetDC( menu->hWnd );
    else hdc = NtUserGetDCEx( menu->hWnd, 0, DCX_CACHE | DCX_WINDOW);
    if (!top_popup)
    {
        top_popup = menu->hWnd;
        top_popup_hmenu = hmenu;
    }

    NtGdiSelectFont( hdc, get_menu_font( FALSE ));

    /* Clear previous highlighted item */
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
        menu->items[menu->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
        draw_menu_item( menu->hWnd, menu, owner, hdc, &menu->items[menu->FocusedItem],
                        !(menu->wFlags & MF_POPUP), ODA_SELECT );
    }

    /* Highlight new item (if any) */
    menu->FocusedItem = index;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
        if (!(menu->items[index].fType & MF_SEPARATOR))
        {
            menu->items[index].fState |= MF_HILITE;
            ensure_menu_item_visible( menu, index, hdc );
            draw_menu_item( menu->hWnd, menu, owner, hdc, &menu->items[index],
                            !(menu->wFlags & MF_POPUP), ODA_SELECT );
        }
        if (send_select)
        {
            struct menu_item *ip = &menu->items[menu->FocusedItem];
            send_message( owner, WM_MENUSELECT,
                          MAKEWPARAM( ip->fType & MF_POPUP ? index: ip->wID,
                                      ip->fType | ip->fState | (menu->wFlags & MF_SYSMENU) ),
                          (LPARAM)hmenu );
        }
    }
    else if (send_select)
    {
        if (topmenu)
        {
            int pos = find_submenu( &topmenu, hmenu );
            if (pos != NO_SELECTED_ITEM)
            {
                struct menu *ptm = unsafe_menu_ptr( topmenu );
                struct menu_item *ip = &ptm->items[pos];
                send_message( owner, WM_MENUSELECT,
                              MAKEWPARAM( pos, ip->fType | ip->fState | (ptm->wFlags & MF_SYSMENU) ),
                              (LPARAM)topmenu );
            }
        }
    }
    NtUserReleaseDC( menu->hWnd, hdc );
}

/***********************************************************************
 *           move_selection
 *
 * Moves currently selected item according to the offset parameter.
 * If there is no selection then it should select the last item if
 * offset is ITEM_PREV or the first item if offset is ITEM_NEXT.
 */
static void move_selection( HWND owner, HMENU hmenu, INT offset )
{
    struct menu *menu;
    int i;

    TRACE( "hwnd %p hmenu %p off 0x%04x\n", owner, hmenu, offset );

    menu = unsafe_menu_ptr( hmenu );
    if (!menu || !menu->items) return;

    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
        if (menu->nItems == 1) return;
        for (i = menu->FocusedItem + offset; i >= 0 && i < menu->nItems; i += offset)
        {
            if (menu->items[i].fType & MF_SEPARATOR) continue;
            select_item( owner, hmenu, i, TRUE, 0 );
            return;
        }
    }

    for (i = (offset > 0) ? 0 : menu->nItems - 1; i >= 0 && i < menu->nItems; i += offset)
    {
        if (menu->items[i].fType & MF_SEPARATOR) continue;
        select_item( owner, hmenu, i, TRUE, 0 );
        return;
    }
}

static void hide_sub_popups( HWND owner, HMENU hmenu, BOOL send_select, UINT flags )
{
    struct menu *menu = unsafe_menu_ptr( hmenu );

    TRACE( "owner=%p hmenu=%p 0x%04x\n", owner, hmenu, send_select );

    if (menu && top_popup)
    {
        struct menu *submenu;
        struct menu_item *item;
        HMENU hsubmenu;

        if (menu->FocusedItem == NO_SELECTED_ITEM) return;

        item = &menu->items[menu->FocusedItem];
        if (!(item->fType & MF_POPUP) || !(item->fState & MF_MOUSESELECT)) return;
        item->fState &= ~MF_MOUSESELECT;
        hsubmenu = item->hSubMenu;

        if (!(submenu = unsafe_menu_ptr( hsubmenu ))) return;
        hide_sub_popups( owner, hsubmenu, FALSE, flags );
        select_item( owner, hsubmenu, NO_SELECTED_ITEM, send_select, 0 );
        NtUserDestroyWindow( submenu->hWnd );
        submenu->hWnd = 0;

        if (!(flags & TPM_NONOTIFY))
           send_message( owner, WM_UNINITMENUPOPUP, (WPARAM)hsubmenu,
                         MAKELPARAM( 0, IS_SYSTEM_MENU( submenu )));
    }
}

static void init_sys_menu_popup( HMENU hmenu, DWORD style, DWORD class_style )
{
    BOOL gray;

    /* Grey the appropriate items in System menu */
    gray = !(style & WS_THICKFRAME) || (style & (WS_MAXIMIZE | WS_MINIMIZE));
    NtUserEnableMenuItem( hmenu, SC_SIZE, gray ? MF_GRAYED : MF_ENABLED );
    gray = ((style & WS_MAXIMIZE) != 0);
    NtUserEnableMenuItem( hmenu, SC_MOVE, gray ? MF_GRAYED : MF_ENABLED );
    gray = !(style & WS_MINIMIZEBOX) || (style & WS_MINIMIZE);
    NtUserEnableMenuItem( hmenu, SC_MINIMIZE, gray ? MF_GRAYED : MF_ENABLED );
    gray = !(style & WS_MAXIMIZEBOX) || (style & WS_MAXIMIZE);
    NtUserEnableMenuItem( hmenu, SC_MAXIMIZE, gray ? MF_GRAYED : MF_ENABLED );
    gray = !(style & (WS_MAXIMIZE | WS_MINIMIZE));
    NtUserEnableMenuItem( hmenu, SC_RESTORE, gray ? MF_GRAYED : MF_ENABLED );
    gray = (class_style & CS_NOCLOSE) != 0;

    /* The menu item must keep its state if it's disabled */
    if (gray) NtUserEnableMenuItem( hmenu, SC_CLOSE, MF_GRAYED );
}

static BOOL init_popup( HWND owner, HMENU hmenu, UINT flags )
{
    UNICODE_STRING class_name = { .Buffer = MAKEINTRESOURCEW( POPUPMENU_CLASS_ATOM ) };
    DWORD ex_style = 0;
    struct menu *menu;

    TRACE( "owner %p hmenu %p\n", owner, hmenu );

    if (!(menu = unsafe_menu_ptr( hmenu ))) return FALSE;

    /* store the owner for DrawItem */
    if (!is_window( owner ))
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    menu->hwndOwner = owner;

    if (flags & TPM_LAYOUTRTL) ex_style = WS_EX_LAYOUTRTL;

    /* NOTE: In Windows, top menu popup is not owned. */
    menu->hWnd = NtUserCreateWindowEx( ex_style, &class_name, NULL, NULL,
                                       WS_POPUP, 0, 0, 0, 0, owner, 0,
                                       (HINSTANCE)get_window_long_ptr( owner, GWLP_HINSTANCE, FALSE ),
                                       (void *)hmenu, 0, NULL, 0, FALSE );
    return !!menu->hWnd;
}


/***********************************************************************
 *           show_sub_popup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or hmenu if no submenu to display.
 */
static HMENU show_sub_popup( HWND owner, HMENU hmenu, BOOL select_first, UINT flags )
{
    struct menu *menu;
    struct menu_item *item;
    RECT rect;
    HDC hdc;

    TRACE( "owner %p hmenu %p 0x%04x\n", owner, hmenu, select_first );

    if (!(menu = unsafe_menu_ptr( hmenu ))) return hmenu;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return hmenu;

    item = &menu->items[menu->FocusedItem];
    if (!(item->fType & MF_POPUP) || (item->fState & (MF_GRAYED | MF_DISABLED)))
        return hmenu;

    /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
    if (!(flags & TPM_NONOTIFY))
       send_message( owner, WM_INITMENUPOPUP, (WPARAM)item->hSubMenu,
                     MAKELPARAM( menu->FocusedItem, IS_SYSTEM_MENU( menu )));

    item = &menu->items[menu->FocusedItem];
    rect = item->rect;

    /* correct item if modified as a reaction to WM_INITMENUPOPUP message */
    if (!(item->fState & MF_HILITE))
    {
        if (menu->wFlags & MF_POPUP) hdc = NtUserGetDC( menu->hWnd );
        else hdc = NtUserGetDCEx( menu->hWnd, 0, DCX_CACHE | DCX_WINDOW );

        NtGdiSelectFont( hdc, get_menu_font( FALSE ));

        item->fState |= MF_HILITE;
        draw_menu_item( menu->hWnd, menu, owner, hdc, item, !(menu->wFlags & MF_POPUP), ODA_DRAWENTIRE );
        NtUserReleaseDC( menu->hWnd, hdc );
    }
    if (!item->rect.top && !item->rect.left && !item->rect.bottom && !item->rect.right)
        item->rect = rect;

    item->fState |= MF_MOUSESELECT;

    if (IS_SYSTEM_MENU( menu ))
    {
        init_sys_menu_popup( item->hSubMenu,
                             get_window_long( menu->hWnd, GWL_STYLE ),
                             get_class_long( menu->hWnd, GCL_STYLE, FALSE ));

        get_sys_popup_pos( menu->hWnd, &rect );
        if (flags & TPM_LAYOUTRTL) rect.left = rect.right;
        rect.top = rect.bottom;
        rect.right = get_system_metrics( SM_CXSIZE );
        rect.bottom = get_system_metrics( SM_CYSIZE );
    }
    else
    {
        RECT item_rect = item->rect;

        adjust_menu_item_rect( menu, &item_rect );
        get_window_rect( menu->hWnd, &rect, get_thread_dpi() );

        if (menu->wFlags & MF_POPUP)
        {
            /* The first item in the popup menu has to be at the
               same y position as the focused menu item */
            if (flags & TPM_LAYOUTRTL)
                rect.left += get_system_metrics( SM_CXBORDER );
            else
                rect.left += item_rect.right - get_system_metrics( SM_CXBORDER );
            rect.top += item_rect.top - MENU_MARGIN;
            rect.right = item_rect.left - item_rect.right + get_system_metrics( SM_CXBORDER );
            rect.bottom = item_rect.top - item_rect.bottom - 2 * MENU_MARGIN;
        }
        else
        {
            if (flags & TPM_LAYOUTRTL)
                rect.left = rect.right - item_rect.left;
            else
                rect.left += item_rect.left;
            rect.top += item_rect.bottom;
            rect.right = item_rect.right - item_rect.left;
            rect.bottom = item_rect.bottom - item_rect.top;
        }
    }

    /* use default alignment for submenus */
    flags &= ~(TPM_CENTERALIGN | TPM_RIGHTALIGN | TPM_VCENTERALIGN | TPM_BOTTOMALIGN);
    init_popup( owner, item->hSubMenu, flags );
    show_popup( owner, item->hSubMenu, menu->FocusedItem, flags,
                rect.left, rect.top, rect.right, rect.bottom );
    if (select_first) move_selection( owner, item->hSubMenu, ITEM_NEXT );
    return item->hSubMenu;
}

/***********************************************************************
 *           exec_focused_item
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return the wID of the executed item. Otherwise, -1 indicating
 * that no menu item was executed, -2 if a popup is shown;
 * Have to receive the flags for the NtUserTrackPopupMenuEx options to avoid
 * sending unwanted message.
 */
static INT exec_focused_item( MTRACKER *pmt, HMENU handle, UINT flags )
{
    struct menu_item *item;
    struct menu *menu = unsafe_menu_ptr( handle );

    TRACE( "%p hmenu=%p\n", pmt, handle );

    if (!menu || !menu->nItems || menu->FocusedItem == NO_SELECTED_ITEM) return -1;
    item = &menu->items[menu->FocusedItem];

    TRACE( "handle %p ID %08lx submenu %p type %04x\n", handle, (long)item->wID,
           item->hSubMenu, item->fType );

    if ((item->fType & MF_POPUP))
    {
        pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, handle, TRUE, flags );
        return -2;
    }

    if ((item->fState & (MF_GRAYED | MF_DISABLED)) || (item->fType & MF_SEPARATOR))
        return -1;

    /* If TPM_RETURNCMD is set you return the id, but
       do not send a message to the owner */
    if (!(flags & TPM_RETURNCMD))
    {
        if (menu->wFlags & MF_SYSMENU)
            NtUserPostMessage( pmt->hOwnerWnd, WM_SYSCOMMAND, item->wID,
                               MAKELPARAM( (INT16)pmt->pt.x, (INT16)pmt->pt.y ));
        else
        {
            struct menu *topmenu = unsafe_menu_ptr( pmt->hTopMenu );
            DWORD style = menu->dwStyle | (topmenu ? topmenu->dwStyle : 0);

            if (style & MNS_NOTIFYBYPOS)
                NtUserPostMessage( pmt->hOwnerWnd, WM_MENUCOMMAND, menu->FocusedItem,
                                   (LPARAM)handle);
            else
                NtUserPostMessage( pmt->hOwnerWnd, WM_COMMAND, item->wID, 0 );
        }
    }

    return item->wID;
}

/***********************************************************************
 *           switch_tracking
 *
 * Helper function for menu navigation routines.
 */
static void switch_tracking( MTRACKER *pmt, HMENU pt_menu, UINT id, UINT flags )
{
    struct menu *ptmenu = unsafe_menu_ptr( pt_menu );
    struct menu *topmenu = unsafe_menu_ptr( pmt->hTopMenu );

    TRACE( "%p hmenu=%p 0x%04x\n", pmt, pt_menu, id );

    if (pmt->hTopMenu != pt_menu && !((ptmenu->wFlags | topmenu->wFlags) & MF_POPUP))
    {
        /* both are top level menus (system and menu-bar) */
        hide_sub_popups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, flags );
        select_item( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE, 0 );
        pmt->hTopMenu = pt_menu;
    }
    else hide_sub_popups( pmt->hOwnerWnd, pt_menu, FALSE, flags );
    select_item( pmt->hOwnerWnd, pt_menu, id, TRUE, 0 );
}

/***********************************************************************
 *           menu_button_down
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL menu_button_down( MTRACKER *pmt, UINT message, HMENU pt_menu, UINT flags )
{
    TRACE( "%p pt_menu=%p\n", pmt, pt_menu );

    if (pt_menu)
    {
        struct menu *ptmenu = unsafe_menu_ptr( pt_menu );
        enum hittest ht = ht_item;
        UINT pos;

        if (IS_SYSTEM_MENU( ptmenu ))
        {
            if (message == WM_LBUTTONDBLCLK) return FALSE;
            pos = 0;
        }
        else
            ht = find_item_by_coords( ptmenu, pmt->pt, &pos );

        if (pos != NO_SELECTED_ITEM)
        {
            if (ptmenu->FocusedItem != pos)
                switch_tracking( pmt, pt_menu, pos, flags );

            /* If the popup menu is not already "popped" */
            if (!(ptmenu->items[pos].fState & MF_MOUSESELECT))
                pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, pt_menu, FALSE, flags );
        }

        /* A click on an item or anywhere on a popup keeps tracking going */
        if (ht == ht_item || ((ptmenu->wFlags & MF_POPUP) && ht != ht_nowhere))
            return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           menu_button_up
 *
 * Return the value of exec_focused_item if
 * the selected item was not a popup. Else open the popup.
 * A -1 return value indicates that we go on with menu tracking.
 *
 */
static INT menu_button_up( MTRACKER *pmt, HMENU pt_menu, UINT flags )
{
    TRACE( "%p hmenu=%p\n", pmt, pt_menu );

    if (pt_menu)
    {
        struct menu *ptmenu = unsafe_menu_ptr( pt_menu );
        UINT pos;

        if (IS_SYSTEM_MENU( ptmenu ))
            pos = 0;
        else if (find_item_by_coords( ptmenu, pmt->pt, &pos ) != ht_item)
            pos = NO_SELECTED_ITEM;

        if (pos != NO_SELECTED_ITEM && (ptmenu->FocusedItem == pos))
        {
            TRACE( "%s\n", debugstr_menuitem( &ptmenu->items[pos] ));

            if (!(ptmenu->items[pos].fType & MF_POPUP))
            {
                INT executedMenuId = exec_focused_item( pmt, pt_menu, flags );
                if (executedMenuId == -1 || executedMenuId == -2) return -1;
                return executedMenuId;
            }

            /* If we are dealing with the menu bar and this is a click on an
             * already "popped" item: Stop the menu tracking and close the
             * opened submenus */
            if(((pmt->hTopMenu == pt_menu) || IS_SYSTEM_MENU( ptmenu )) &&
               (pmt->trackFlags & TF_RCVD_BTN_UP))
                return 0;
        }

        if (get_menu( ptmenu->hWnd ) == pt_menu || IS_SYSTEM_MENU( ptmenu ))
        {
            if (pos == NO_SELECTED_ITEM) return 0;
            pmt->trackFlags |= TF_RCVD_BTN_UP;
        }
    }
    return -1;
}

/***********************************************************************
 *           end_menu
 *
 * Call NtUserEndMenu() if the hwnd parameter belongs to the menu owner.
 */
void end_menu( HWND hwnd )
{
    struct menu *menu;
    BOOL call_end = FALSE;
    if (top_popup_hmenu && (menu = grab_menu_ptr( top_popup_hmenu )))
    {
        call_end = hwnd == menu->hWnd || hwnd == menu->hwndOwner;
        release_menu_ptr( menu );
    }
    if (call_end) NtUserEndMenu();
}

/***********************************************************************
 *           menu_mouse_move
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL menu_mouse_move( MTRACKER* pmt, HMENU pt_menu, UINT flags )
{
    UINT id = NO_SELECTED_ITEM;
    struct menu *ptmenu = NULL;

    if (pt_menu)
    {
        ptmenu = unsafe_menu_ptr( pt_menu );
        if (IS_SYSTEM_MENU( ptmenu ))
            id = 0;
        else if (find_item_by_coords( ptmenu, pmt->pt, &id ) != ht_item)
            id = NO_SELECTED_ITEM;
    }

    if (id == NO_SELECTED_ITEM)
    {
        select_item( pmt->hOwnerWnd, pmt->hCurrentMenu, NO_SELECTED_ITEM, TRUE, pmt->hTopMenu );
    }
    else if (ptmenu->FocusedItem != id)
    {
        switch_tracking( pmt, pt_menu, id, flags );
        pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, pt_menu, FALSE, flags );
    }
    return TRUE;
}

static LRESULT do_next_menu( MTRACKER *pmt, UINT vk, UINT flags )
{
    struct menu *menu = unsafe_menu_ptr( pmt->hTopMenu );
    BOOL at_end = FALSE;

    if (vk == VK_LEFT && menu->FocusedItem == 0)
    {
        /* When skipping left, we need to do something special after the  first menu */
        at_end = TRUE;
    }
    else if (vk == VK_RIGHT && !IS_SYSTEM_MENU( menu ))
    {
        /* When skipping right, for the non-system menu, we need to
         * handle the last non-special menu item (ie skip any window
         * icons such as MDI maximize, restore or close) */
        UINT i = menu->FocusedItem + 1;
        while (i < menu->nItems)
        {
            if (menu->items[i].wID < SC_SIZE || menu->items[i].wID > SC_RESTORE) break;
            i++;
        }
        if (i == menu->nItems) at_end = TRUE;
    }
    else if (vk == VK_RIGHT && IS_SYSTEM_MENU( menu ))
    {
        /* When skipping right, we need to cater for the system menu */
        if (menu->FocusedItem == menu->nItems - 1) at_end = TRUE;
    }

    if (at_end)
    {
        MDINEXTMENU next_menu;
        HMENU new_menu;
        HWND new_hwnd;
        UINT id = 0;

        next_menu.hmenuIn = (IS_SYSTEM_MENU( menu )) ? get_sub_menu( pmt->hTopMenu, 0 ) : pmt->hTopMenu;
        next_menu.hmenuNext = 0;
        next_menu.hwndNext = 0;
        send_message( pmt->hOwnerWnd, WM_NEXTMENU, vk, (LPARAM)&next_menu );

        TRACE( "%p [%p] -> %p [%p]\n", pmt->hCurrentMenu, pmt->hOwnerWnd, next_menu.hmenuNext,
               next_menu.hwndNext );

        if (!next_menu.hmenuNext || !next_menu.hwndNext)
        {
            DWORD style = get_window_long( pmt->hOwnerWnd, GWL_STYLE );
            new_hwnd = pmt->hOwnerWnd;
            if (IS_SYSTEM_MENU( menu ))
            {
                /* switch to the menu bar */
                if ((style & WS_CHILD) || !(new_menu = get_menu( new_hwnd ))) return FALSE;

                if (vk == VK_LEFT)
                {
                    menu = unsafe_menu_ptr( new_menu );
                    id = menu->nItems - 1;

                    /* Skip backwards over any system predefined icons,
                     * eg. MDI close, restore etc icons */
                    while (id > 0 &&
                           menu->items[id].wID >= SC_SIZE && menu->items[id].wID <= SC_RESTORE)
                        id--;
                }
            }
            else if (style & WS_SYSMENU)
            {
                /* switch to the system menu */
                new_menu = get_win_sys_menu( new_hwnd );
            }
            else return FALSE;
        }
        else /* application returned a new menu to switch to */
        {
            new_menu = next_menu.hmenuNext;
            new_hwnd = get_full_window_handle( next_menu.hwndNext );

            if (is_menu( new_menu ) && is_window( new_hwnd ))
            {
                DWORD style = get_window_long( new_hwnd, GWL_STYLE );

                if (style & WS_SYSMENU && get_sub_menu(get_win_sys_menu( new_hwnd ), 0) == new_menu)
                {
                    /* get the real system menu */
                    new_menu = get_win_sys_menu( new_hwnd );
                }
                else if (style & WS_CHILD || get_menu( new_hwnd ) != new_menu )
                {
                    /* FIXME: what should we do? */
                    TRACE( " -- got confused.\n" );
                    return FALSE;
                }
            }
            else return FALSE;
        }

        if (new_menu != pmt->hTopMenu)
        {
            select_item( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE, 0 );
            if (pmt->hCurrentMenu != pmt->hTopMenu)
                hide_sub_popups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, flags );
        }

        if (new_hwnd != pmt->hOwnerWnd)
        {
            pmt->hOwnerWnd = new_hwnd;
            set_capture_window( pmt->hOwnerWnd, GUI_INMENUMODE, NULL );
        }

        pmt->hTopMenu = pmt->hCurrentMenu = new_menu; /* all subpopups are hidden */
        select_item( pmt->hOwnerWnd, pmt->hTopMenu, id, TRUE, 0 );
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 *           get_sub_popup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static HMENU get_sub_popup( HMENU hmenu )
{
    struct menu *menu;
    struct menu_item *item;

    menu = unsafe_menu_ptr( hmenu );

    if (!menu || menu->FocusedItem == NO_SELECTED_ITEM) return 0;

    item = &menu->items[menu->FocusedItem];
    if ((item->fType & MF_POPUP) && (item->fState & MF_MOUSESELECT))
        return item->hSubMenu;
    return 0;
}

/***********************************************************************
 *           menu_key_escape
 *
 * Handle a VK_ESCAPE key event in a menu.
 */
static BOOL menu_key_escape( MTRACKER *pmt, UINT flags )
{
    BOOL ret = TRUE;

    if (pmt->hCurrentMenu != pmt->hTopMenu)
    {
        struct menu *menu = unsafe_menu_ptr( pmt->hCurrentMenu );

        if (menu->wFlags & MF_POPUP)
        {
            HMENU top, prev_menu;

            prev_menu = top = pmt->hTopMenu;

            /* close topmost popup */
            while (top != pmt->hCurrentMenu)
            {
                prev_menu = top;
                top = get_sub_popup( prev_menu );
            }

            hide_sub_popups( pmt->hOwnerWnd, prev_menu, TRUE, flags );
            pmt->hCurrentMenu = prev_menu;
            ret = FALSE;
        }
    }

    return ret;
}

static UINT get_start_of_next_column( HMENU handle )
{
    struct menu *menu = unsafe_menu_ptr( handle );
    UINT i;

    if (!menu) return NO_SELECTED_ITEM;

    i = menu->FocusedItem + 1;
    if (i == NO_SELECTED_ITEM) return i;

    while (i < menu->nItems)
    {
        if (menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
            return i;
        i++;
    }

    return NO_SELECTED_ITEM;
}

static UINT get_start_of_prev_column( HMENU handle )
{
    struct menu *menu = unsafe_menu_ptr( handle );
    UINT i;

    if (!menu) return NO_SELECTED_ITEM;

    if (menu->FocusedItem == 0 || menu->FocusedItem == NO_SELECTED_ITEM)
        return NO_SELECTED_ITEM;

    /* Find the start of the column */
    i = menu->FocusedItem;
    while (i != 0 && !(menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))) i--;
    if (i == 0) return NO_SELECTED_ITEM;

    for (--i; i != 0; --i)
    {
        if (menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
            break;
    }

    TRACE( "ret %d.\n", i );
    return i;
}

/***********************************************************************
 *           suspend_popup
 *
 * Avoid showing the popup if the next input message is going to hide it anyway.
 */
static BOOL suspend_popup( MTRACKER *pmt, UINT message )
{
    MSG msg;

    msg.hwnd = pmt->hOwnerWnd;
    NtUserPeekMessage( &msg, 0, message, message, PM_NOYIELD | PM_REMOVE );
    pmt->trackFlags |= TF_SKIPREMOVE;

    switch (message)
    {
    case WM_KEYDOWN:
        NtUserPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE );
        if (msg.message == WM_KEYUP || msg.message == WM_PAINT)
        {
            NtUserPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE );
            NtUserPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE );
            if (msg.message == WM_KEYDOWN && (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
            {
                pmt->trackFlags |= TF_SUSPENDPOPUP;
                return TRUE;
            }
        }
        break;
    }

    /* failures go through this */
    pmt->trackFlags &= ~TF_SUSPENDPOPUP;
    return FALSE;
}

static void menu_key_left( MTRACKER *pmt, UINT flags, UINT msg )
{
    struct menu *menu;
    HMENU tmp_menu, prev_menu;
    UINT  prevcol;

    prev_menu = tmp_menu = pmt->hTopMenu;
    menu = unsafe_menu_ptr( tmp_menu );

    /* Try to move 1 column left (if possible) */
    if ((prevcol = get_start_of_prev_column( pmt->hCurrentMenu )) != NO_SELECTED_ITEM)
    {
        select_item( pmt->hOwnerWnd, pmt->hCurrentMenu, prevcol, TRUE, 0 );
        return;
    }

    /* close topmost popup */
    while (tmp_menu != pmt->hCurrentMenu)
    {
        prev_menu = tmp_menu;
        tmp_menu = get_sub_popup( prev_menu );
    }

    hide_sub_popups( pmt->hOwnerWnd, prev_menu, TRUE, flags );
    pmt->hCurrentMenu = prev_menu;

    if ((prev_menu == pmt->hTopMenu) && !(menu->wFlags & MF_POPUP))
    {
        /* move menu bar selection if no more popups are left */
        if (!do_next_menu( pmt, VK_LEFT, flags ))
            move_selection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_PREV );

        if (prev_menu != tmp_menu || pmt->trackFlags & TF_SUSPENDPOPUP)
        {
           /* A sublevel menu was displayed - display the next one
            * unless there is another displacement coming up */
            if (!suspend_popup( pmt, msg ))
                pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, pmt->hTopMenu, TRUE, flags );
        }
    }
}

static void menu_right_key( MTRACKER *pmt, UINT flags, UINT msg )
{
    struct menu *menu = unsafe_menu_ptr( pmt->hTopMenu );
    HMENU tmp_menu;
    UINT  nextcol;

    TRACE( "menu_right_key called, cur %p (%s), top %p (%s).\n",
           pmt->hCurrentMenu, debugstr_w(unsafe_menu_ptr( pmt->hCurrentMenu )->items[0].text ),
           pmt->hTopMenu, debugstr_w( menu->items[0].text ));

    if ((menu->wFlags & MF_POPUP) || (pmt->hCurrentMenu != pmt->hTopMenu))
    {
        /* If already displaying a popup, try to display sub-popup */
        tmp_menu = pmt->hCurrentMenu;
        pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, tmp_menu, TRUE, flags );

        /* if subpopup was displayed then we are done */
        if (tmp_menu != pmt->hCurrentMenu) return;
    }

    /* Check to see if there's another column */
    if ((nextcol = get_start_of_next_column( pmt->hCurrentMenu )) != NO_SELECTED_ITEM)
    {
        TRACE( "Going to %d.\n", nextcol );
        select_item( pmt->hOwnerWnd, pmt->hCurrentMenu, nextcol, TRUE, 0 );
        return;
    }

    if (!(menu->wFlags & MF_POPUP))  /* menu bar tracking */
    {
        if (pmt->hCurrentMenu != pmt->hTopMenu)
        {
            hide_sub_popups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, flags );
            tmp_menu = pmt->hCurrentMenu = pmt->hTopMenu;
        }
        else tmp_menu = 0;

        /* try to move to the next item */
        if (!do_next_menu( pmt, VK_RIGHT, flags ))
            move_selection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_NEXT );

        if (tmp_menu || pmt->trackFlags & TF_SUSPENDPOPUP)
            if (!suspend_popup( pmt, msg ))
                pmt->hCurrentMenu = show_sub_popup( pmt->hOwnerWnd, pmt->hTopMenu, TRUE, flags );
    }
}

/***********************************************************************
 *           menu_from_point
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static HMENU menu_from_point( HMENU handle, POINT pt )
{
   struct menu *menu = unsafe_menu_ptr( handle );
   UINT item = menu->FocusedItem;
   HMENU ret = 0;

   /* try subpopup first (if any) */
   if (item != NO_SELECTED_ITEM && (menu->items[item].fType & MF_POPUP) &&
       (menu->items[item].fState & MF_MOUSESELECT))
       ret = menu_from_point( menu->items[item].hSubMenu, pt );

   if (!ret)  /* check the current window (avoiding WM_HITTEST) */
   {
       INT ht = handle_nc_hit_test( menu->hWnd, pt );
       if (menu->wFlags & MF_POPUP)
       {
           if (ht != HTNOWHERE && ht != HTERROR) ret = handle;
       }
       else if (ht == HTSYSMENU)
           ret = get_win_sys_menu( menu->hWnd );
       else if (ht == HTMENU)
           ret = get_menu( menu->hWnd );
   }
   return ret;
}

/***********************************************************************
 *           find_item_by_key
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT find_item_by_key( HWND owner, HMENU hmenu, WCHAR key, BOOL force_menu_char )
{
    TRACE( "\tlooking for '%c' (0x%02x) in [%p]\n", (char)key, key, hmenu );

    if (!is_menu( hmenu )) hmenu = get_sub_menu( get_win_sys_menu( owner ), 0 );

    if (hmenu)
    {
        struct menu *menu = unsafe_menu_ptr( hmenu );
        struct menu_item *item = menu->items;
        LRESULT menuchar;

        if (!force_menu_char)
        {
             BOOL cjk = get_system_metrics( SM_DBCSENABLED );
             UINT i;

             for (i = 0; i < menu->nItems; i++, item++)
             {
                if (item->text)
                {
                    const WCHAR *p = item->text - 2;
                    do
                    {
                        const WCHAR *q = p + 2;
                        p = wcschr( q, '&' );
                        if (!p && cjk) p = wcschr( q, '\036' ); /* Japanese Win16 */
                    }
                    while (p && p[1] == '&');
                    if (p && !wcsnicmp( &p[1], &key, 1)) return i;
                }
             }
        }
        menuchar = send_message( owner, WM_MENUCHAR,
                                 MAKEWPARAM( key, menu->wFlags ), (LPARAM)hmenu );
        if (HIWORD(menuchar) == MNC_EXECUTE) return LOWORD( menuchar );
        if (HIWORD(menuchar) == MNC_CLOSE) return (UINT)-2;
    }
    return -1;
}

static BOOL track_menu( HMENU hmenu, UINT flags, int x, int y, HWND hwnd, const RECT *rect )
{
    BOOL enter_idle_sent = FALSE;
    int executed_menu_id = -1;
    HWND capture_win;
    struct menu *menu;
    BOOL remove;
    MTRACKER mt;
    MSG msg;

    mt.trackFlags = 0;
    mt.hCurrentMenu = hmenu;
    mt.hTopMenu = hmenu;
    mt.hOwnerWnd = get_full_window_handle( hwnd );
    mt.pt.x = x;
    mt.pt.y = y;

    TRACE( "hmenu=%p flags=0x%08x (%d,%d) hwnd=%p %s\n",
           hmenu, flags, x, y, hwnd, wine_dbgstr_rect( rect ));

    if (!(menu = unsafe_menu_ptr( hmenu )))
    {
        WARN( "Invalid menu handle %p\n", hmenu );
        RtlSetLastWin32Error( ERROR_INVALID_MENU_HANDLE );
        return FALSE;
    }

    if (flags & TPM_BUTTONDOWN)
    {
        /* Get the result in order to start the tracking or not */
        remove = menu_button_down( &mt, WM_LBUTTONDOWN, hmenu, flags );
        exit_menu = !remove;
    }

    if (flags & TF_ENDMENU) exit_menu = TRUE;

    /* owner may not be visible when tracking a popup, so use the menu itself */
    capture_win = (flags & TPM_POPUPMENU) ? menu->hWnd : mt.hOwnerWnd;
    set_capture_window( capture_win, GUI_INMENUMODE, NULL );

    if ((flags & TPM_POPUPMENU) && menu->nItems == 0)
        return FALSE;

    while (!exit_menu)
    {
        if (!(menu = unsafe_menu_ptr( mt.hCurrentMenu ))) break;

        /* we have to keep the message in the queue until it's
         * clear that menu loop is not over yet. */
        for (;;)
        {
            if (NtUserPeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ))
            {
                if (!NtUserCallMsgFilter( &msg, MSGF_MENU )) break;
                /* remove the message from the queue */
                NtUserPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE );
            }
            else
            {
                if (!enter_idle_sent)
                {
                    HWND win = (menu->wFlags & MF_POPUP) ? menu->hWnd : 0;
                    enter_idle_sent = TRUE;
                    send_message( mt.hOwnerWnd, WM_ENTERIDLE, MSGF_MENU, (LPARAM)win );
                }
                NtUserMsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 );
            }
        }

        /* check if NtUserEndMenu() tried to cancel us, by posting this message */
        if (msg.message == WM_CANCELMODE)
        {
            exit_menu = TRUE;
            /* remove the message from the queue */
            NtUserPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE );
            break;
        }

        mt.pt = msg.pt;
        if (msg.hwnd == menu->hWnd || msg.message != WM_TIMER) enter_idle_sent = FALSE;

        remove = FALSE;
        if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
        {
            /*
             * Use the mouse coordinates in lParam instead of those in the MSG
             * struct to properly handle synthetic messages. They are already
             * in screen coordinates.
             */
            mt.pt.x = (short)LOWORD( msg.lParam );
            mt.pt.y = (short)HIWORD( msg.lParam );

            /* Find a menu for this mouse event */
            hmenu = menu_from_point( mt.hTopMenu, mt.pt );

            switch (msg.message)
            {
                /* no WM_NC... messages in captured state */
                case WM_RBUTTONDBLCLK:
                case WM_RBUTTONDOWN:
                    if (!(flags & TPM_RIGHTBUTTON)) break;
                    /* fall through */
                case WM_LBUTTONDBLCLK:
                case WM_LBUTTONDOWN:
                    /* If the message belongs to the menu, removes it from the queue
                     * Else, end menu tracking */
                    remove = menu_button_down( &mt, msg.message, hmenu, flags );
                    exit_menu = !remove;
                    break;

                case WM_RBUTTONUP:
                    if (!(flags & TPM_RIGHTBUTTON)) break;
                    /* fall through */
                case WM_LBUTTONUP:
                    /* Check if a menu was selected by the mouse */
                    if (hmenu)
                    {
                        executed_menu_id = menu_button_up( &mt, hmenu, flags);
                        TRACE( "executed_menu_id %d\n", executed_menu_id );

                        /* End the loop if executed_menu_id is an item ID
                         * or if the job was done (executed_menu_id = 0). */
                        exit_menu = remove = executed_menu_id != -1;
                    }
                    else
                        /* No menu was selected by the mouse. If the function was called by
                         * NtUserTrackPopupMenuEx, continue with the menu tracking. */
                        exit_menu = !(flags & TPM_POPUPMENU);

                    break;

                case WM_MOUSEMOVE:
                    /* the selected menu item must be changed every time the mouse moves. */
                    if (hmenu) exit_menu |= !menu_mouse_move( &mt, hmenu, flags );
                    break;
            }
        }
        else if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
        {
            remove = TRUE;  /* Keyboard messages are always removed */
            switch (msg.message)
            {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                switch(msg.wParam)
                {
                case VK_MENU:
                case VK_F10:
                    exit_menu = TRUE;
                    break;

                case VK_HOME:
                case VK_END:
                    select_item( mt.hOwnerWnd, mt.hCurrentMenu, NO_SELECTED_ITEM, FALSE, 0 );
                    move_selection( mt.hOwnerWnd, mt.hCurrentMenu,
                                    msg.wParam == VK_HOME ? ITEM_NEXT : ITEM_PREV );
                    break;

                case VK_UP:
                case VK_DOWN: /* If on menu bar, pull-down the menu */
                    menu = unsafe_menu_ptr( mt.hCurrentMenu );
                    if (!(menu->wFlags & MF_POPUP))
                        mt.hCurrentMenu = show_sub_popup( mt.hOwnerWnd, mt.hTopMenu, TRUE, flags );
                    else  /* otherwise try to move selection */
                        move_selection( mt.hOwnerWnd, mt.hCurrentMenu,
                                        msg.wParam == VK_UP ? ITEM_PREV : ITEM_NEXT );
                    break;

                case VK_LEFT:
                    menu_key_left( &mt, flags, msg.message );
                    break;

                case VK_RIGHT:
                    menu_right_key( &mt, flags, msg.message );
                    break;

                case VK_ESCAPE:
                    exit_menu = menu_key_escape( &mt, flags );
                    break;

                case VK_F1:
                    {
                        HELPINFO hi;
                        hi.cbSize = sizeof(HELPINFO);
                        hi.iContextType = HELPINFO_MENUITEM;
                        if (menu->FocusedItem == NO_SELECTED_ITEM)
                            hi.iCtrlId = 0;
                        else
                            hi.iCtrlId = menu->items[menu->FocusedItem].wID;
                        hi.hItemHandle = hmenu;
                        hi.dwContextId = menu->dwContextHelpID;
                        hi.MousePos = msg.pt;
                        send_message( hwnd, WM_HELP, 0, (LPARAM)&hi );
                        break;
                    }

                default:
                    NtUserTranslateMessage( &msg, 0 );
                    break;
                }
                break;  /* WM_KEYDOWN */

            case WM_CHAR:
            case WM_SYSCHAR:
                {
                    UINT pos;

                    if (msg.wParam == '\r' || msg.wParam == ' ')
                    {
                        executed_menu_id = exec_focused_item( &mt, mt.hCurrentMenu, flags );
                        exit_menu = executed_menu_id != -2;
                        break;
                    }

                    /* Hack to avoid control chars... */
                    if (msg.wParam < 32) break;

                    pos = find_item_by_key( mt.hOwnerWnd, mt.hCurrentMenu,
                                            LOWORD( msg.wParam ), FALSE );
                    if (pos == -2) exit_menu = TRUE;
                    else if (pos == -1) NtUserMessageBeep( 0 );
                    else
                    {
                        select_item( mt.hOwnerWnd, mt.hCurrentMenu, pos, TRUE, 0 );
                        executed_menu_id = exec_focused_item( &mt,mt.hCurrentMenu, flags );
                        exit_menu = executed_menu_id != -2;
                    }
                }
                break;
            }
        }
        else
        {
            NtUserPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE );
            NtUserDispatchMessage( &msg );
            continue;
        }

        if (!exit_menu) remove = TRUE;

        /* finally remove message from the queue */
        if (remove && !(mt.trackFlags & TF_SKIPREMOVE))
            NtUserPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE );
        else mt.trackFlags &= ~TF_SKIPREMOVE;
    }

    set_capture_window( 0, GUI_INMENUMODE, NULL );

    /* If dropdown is still painted and the close box is clicked on
     * then the menu will be destroyed as part of the DispatchMessage above.
     * This will then invalidate the menu handle in mt.hTopMenu. We should
     * check for this first.  */
    if (is_menu( mt.hTopMenu ))
    {
        menu = unsafe_menu_ptr( mt.hTopMenu );

        if (is_window( mt.hOwnerWnd ))
        {
            hide_sub_popups( mt.hOwnerWnd, mt.hTopMenu, FALSE, flags );

            if (menu && (menu->wFlags & MF_POPUP))
            {
                NtUserDestroyWindow( menu->hWnd );
                menu->hWnd = 0;

                if (!(flags & TPM_NONOTIFY))
                   send_message( mt.hOwnerWnd, WM_UNINITMENUPOPUP, (WPARAM)mt.hTopMenu,
                                 MAKELPARAM( 0, IS_SYSTEM_MENU( menu )));
            }
            select_item( mt.hOwnerWnd, mt.hTopMenu, NO_SELECTED_ITEM, FALSE, 0 );
            send_message( mt.hOwnerWnd, WM_MENUSELECT, MAKEWPARAM( 0, 0xffff ), 0 );
        }
    }

    RtlSetLastWin32Error( ERROR_SUCCESS );
    /* The return value is only used by NtUserTrackPopupMenuEx */
    if (!(flags & TPM_RETURNCMD)) return TRUE;
    if (executed_menu_id == -1) executed_menu_id = 0;
    return executed_menu_id;
}

static BOOL init_tracking( HWND hwnd, HMENU handle, BOOL is_popup, UINT flags )
{
    struct menu *menu;

    TRACE( "hwnd=%p hmenu=%p\n", hwnd, handle );

    NtUserHideCaret( 0 );

    if (!(menu = unsafe_menu_ptr( handle ))) return FALSE;

    /* This makes the menus of applications built with Delphi work.
     * It also enables menus to be displayed in more than one window,
     * but there are some bugs left that need to be fixed in this case.
     */
    if (!is_popup) menu->hWnd = hwnd;
    if (!top_popup)
    {
        top_popup = menu->hWnd;
        top_popup_hmenu = handle;
    }

    exit_menu = FALSE;

    /* Send WM_ENTERMENULOOP and WM_INITMENU message only if TPM_NONOTIFY flag is not specified */
    if (!(flags & TPM_NONOTIFY))
       send_message( hwnd, WM_ENTERMENULOOP, is_popup, 0 );

    send_message( hwnd, WM_SETCURSOR, (WPARAM)hwnd, HTCAPTION );

    if (!(flags & TPM_NONOTIFY))
    {
       send_message( hwnd, WM_INITMENU, (WPARAM)handle, 0 );
       /* If an app changed/recreated menu bar entries in WM_INITMENU
        * menu sizes will be recalculated once the menu created/shown. */
    }

    return TRUE;
}

static BOOL exit_tracking( HWND hwnd, BOOL is_popup )
{
    TRACE( "hwnd=%p\n", hwnd );

    send_message( hwnd, WM_EXITMENULOOP, is_popup, 0 );
    NtUserShowCaret( 0 );
    top_popup = 0;
    top_popup_hmenu = NULL;
    return TRUE;
}

void track_mouse_menu_bar( HWND hwnd, INT ht, int x, int y )
{
    HMENU handle = ht == HTSYSMENU ? get_win_sys_menu( hwnd ) : get_menu( hwnd );
    UINT flags = TPM_BUTTONDOWN | TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE( "wnd=%p ht=0x%04x %d,%d\n", hwnd, ht, x, y );

    if (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) flags |= TPM_LAYOUTRTL;
    if (is_menu( handle ))
    {
        init_tracking( hwnd, handle, FALSE, flags );

        /* fetch the window menu again, it may have changed */
        handle = ht == HTSYSMENU ? get_win_sys_menu( hwnd ) : get_menu( hwnd );
        track_menu( handle, flags, x, y, hwnd, NULL );
        exit_tracking( hwnd, FALSE );
    }
}

void track_keyboard_menu_bar( HWND hwnd, UINT wparam, WCHAR ch )
{
    UINT flags = TPM_LEFTALIGN | TPM_LEFTBUTTON;
    UINT item = NO_SELECTED_ITEM;
    HMENU menu;

    TRACE( "hwnd %p wparam 0x%04x ch 0x%04x\n", hwnd, wparam, ch );

    /* find window that has a menu */
    while (is_win_menu_disallowed( hwnd ))
        if (!(hwnd = NtUserGetAncestor( hwnd, GA_PARENT ))) return;

    /* check if we have to track a system menu */
    menu = get_menu( hwnd );
    if (!menu || is_iconic( hwnd ) || ch == ' ')
    {
        if (!(get_window_long( hwnd, GWL_STYLE ) & WS_SYSMENU)) return;
        menu = get_win_sys_menu( hwnd );
        item = 0;
        wparam |= HTSYSMENU; /* prevent item lookup */
    }
    if (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) flags |= TPM_LAYOUTRTL;

    if (!is_menu( menu )) return;

    init_tracking( hwnd, menu, FALSE, flags );

    /* fetch the window menu again, it may have changed */
    menu = (wparam & HTSYSMENU) ? get_win_sys_menu( hwnd ) : get_menu( hwnd );

    if (ch && ch != ' ')
    {
        item = find_item_by_key( hwnd, menu, ch, wparam & HTSYSMENU );
        if (item >= -2)
        {
            if (item == -1) NtUserMessageBeep( 0 );
            /* schedule end of menu tracking */
            flags |= TF_ENDMENU;
            goto track_menu;
        }
    }

    select_item( hwnd, menu, item, TRUE, 0 );

    if (!(wparam & HTSYSMENU) || ch == ' ')
    {
        if( item == NO_SELECTED_ITEM )
            move_selection( hwnd, menu, ITEM_NEXT );
        else
            NtUserPostMessage( hwnd, WM_KEYDOWN, VK_RETURN, 0 );
    }

track_menu:
    track_menu( menu, flags, 0, 0, hwnd, NULL );
    exit_tracking( hwnd, FALSE );
}

/**********************************************************************
 *           NtUserTrackPopupMenuEx   (win32u.@)
 */
BOOL WINAPI NtUserTrackPopupMenuEx( HMENU handle, UINT flags, INT x, INT y, HWND hwnd,
                                    TPMPARAMS *params )
{
    struct menu *menu;
    BOOL ret = FALSE;

    TRACE( "hmenu %p flags %04x (%d,%d) hwnd %p params %p rect %s\n",
           handle, flags, x, y, hwnd, params,
           params ? wine_dbgstr_rect( &params->rcExclude ) : "-" );

    if (!(menu = unsafe_menu_ptr( handle )))
    {
        RtlSetLastWin32Error( ERROR_INVALID_MENU_HANDLE );
        return FALSE;
    }

    if (is_window(menu->hWnd))
    {
        RtlSetLastWin32Error( ERROR_POPUP_ALREADY_ACTIVE );
        return FALSE;
    }

    if (init_popup( hwnd, handle, flags ))
    {
        init_tracking( hwnd, handle, TRUE, flags );

        /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
        if (!(flags & TPM_NONOTIFY))
            send_message( hwnd, WM_INITMENUPOPUP, (WPARAM)handle, 0 );

        if (menu->wFlags & MF_SYSMENU)
            init_sys_menu_popup( handle, get_window_long( hwnd, GWL_STYLE ),
                                 get_class_long( hwnd, GCL_STYLE, FALSE ));

        if (show_popup( hwnd, handle, 0, flags, x, y, 0, 0 ))
            ret = track_menu( handle, flags | TPM_POPUPMENU, 0, 0, hwnd,
                              params ? &params->rcExclude : NULL );
        exit_tracking( hwnd, TRUE );

        if (menu->hWnd)
        {
            NtUserDestroyWindow( menu->hWnd );
            menu->hWnd = 0;

            if (!(flags & TPM_NONOTIFY))
                send_message( hwnd, WM_UNINITMENUPOPUP, (WPARAM)handle,
                              MAKELPARAM( 0, IS_SYSTEM_MENU( menu )));
        }
        RtlSetLastWin32Error( 0 );
    }

    return ret;
}

/**********************************************************************
 *           NtUserHiliteMenuItem    (win32u.@)
 */
BOOL WINAPI NtUserHiliteMenuItem( HWND hwnd, HMENU handle, UINT item, UINT hilite )
{
    HMENU handle_menu;
    UINT focused_item;
    struct menu *menu;
    UINT pos;

    TRACE( "(%p, %p, %04x, %04x);\n", hwnd, handle, item, hilite );

    if (!(menu = find_menu_item(handle, item, hilite, &pos))) return FALSE;
    handle_menu = menu->handle;
    focused_item = menu->FocusedItem;
    release_menu_ptr(menu);

    if (focused_item != pos)
    {
        hide_sub_popups( hwnd, handle_menu, FALSE, 0 );
        select_item( hwnd, handle_menu, pos, TRUE, 0 );
    }

    return TRUE;
}

/**********************************************************************
 *           NtUserGetMenuBarInfo    (win32u.@)
 */
BOOL WINAPI NtUserGetMenuBarInfo( HWND hwnd, LONG id, LONG item, MENUBARINFO *info )
{
    HMENU hmenu = NULL;
    struct menu *menu;
    ATOM class_atom;

    TRACE( "(%p,0x%08x,0x%08x,%p)\n", hwnd, id, item, info );

    switch (id)
    {
    case OBJID_CLIENT:
        class_atom = get_class_long( hwnd, GCW_ATOM, FALSE );
        if (!class_atom)
            return FALSE;
        if (class_atom != POPUPMENU_CLASS_ATOM)
        {
            WARN("called on invalid window: %d\n", class_atom);
            RtlSetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
            return FALSE;
        }

        hmenu = (HMENU)get_window_long_ptr( hwnd, 0, FALSE );
        break;
    case OBJID_MENU:
        hmenu = get_menu( hwnd );
        break;
    case OBJID_SYSMENU:
        hmenu = NtUserGetSystemMenu( hwnd, FALSE );
        break;
    default:
        return FALSE;
    }

    if (!hmenu)
        return FALSE;

    if (info->cbSize != sizeof(MENUBARINFO))
    {
        RtlSetLastWin32Error( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!(menu = grab_menu_ptr( hmenu ))) return FALSE;
    if (item < 0 || item > menu->nItems)
    {
        release_menu_ptr( menu );
        return FALSE;
    }

    if (!menu->Height)
    {
        SetRectEmpty( &info->rcBar );
    }
    else if (item == 0)
    {
        NtUserGetMenuItemRect( hwnd, hmenu, 0, &info->rcBar );
        info->rcBar.right = info->rcBar.left + menu->Width;
        info->rcBar.bottom = info->rcBar.top + menu->Height;
    }
    else
    {
        NtUserGetMenuItemRect( hwnd, hmenu, item - 1, &info->rcBar );
    }

    info->hMenu = hmenu;
    info->hwndMenu = NULL;
    info->fBarFocused = top_popup_hmenu == hmenu;
    if (item)
    {
        info->fFocused = menu->FocusedItem == item - 1;
        if (info->fFocused && (menu->items[item - 1].fType & MF_POPUP))
        {
            struct menu *hwnd_menu = grab_menu_ptr( menu->items[item - 1].hSubMenu );
            if (hwnd_menu)
            {
                info->hwndMenu = hwnd_menu->hWnd;
                release_menu_ptr( hwnd_menu );
            }
        }
    }
    else
    {
        info->fFocused = info->fBarFocused;
    }

    release_menu_ptr( menu );
    return TRUE;
}

/***********************************************************************
 *           NtUserEndMenu   (win32u.@)
 */
BOOL WINAPI NtUserEndMenu(void)
{
    /* if we are in the menu code, and it is active, terminate the menu handling code */
    if (!exit_menu && top_popup)
    {
        exit_menu = TRUE;

        /* needs to be posted to wakeup the internal menu handler
         * which will now terminate the menu, in the event that
         * the main window was minimized, or lost focus, so we
         * don't end up with an orphaned menu */
        NtUserPostMessage( top_popup, WM_CANCELMODE, 0, 0 );
    }
    return exit_menu;
}
