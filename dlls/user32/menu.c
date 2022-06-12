/*
 * Menu functions
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 * Copyright 2005 Maxime Bellengé
 * Copyright 2006 Phil Krylov
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

/*
 * Note: the style MF_MOUSESELECT is used to mark popup items that
 * have been selected, i.e. their popup menu is currently displayed.
 * This is probably not the meaning this style has in MS-Windows.
 *
 * Note 2: where there is a difference, these menu API's are according
 * the behavior of Windows 2k and Windows XP. Known differences with
 * Windows 9x/ME are documented in the comments, in case an application
 * is found to depend on the old behavior.
 * 
 * TODO:
 *    implements styles :
 *        - MNS_AUTODISMISS
 *        - MNS_DRAGDROP
 *        - MNS_MODELESS
 */

#include <stdarg.h>
#include <string.h>

#define OEMRESOURCE

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/server.h"
#include "wine/exception.h"
#include "win.h"
#include "controls.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(menu);


  /* Space between 2 columns */
#define MENU_COL_SPACE 4

  /* Margins for popup menus */
#define MENU_MARGIN 3

  /* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)
#define IS_MAGIC_BITMAP(id)     ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define MENUITEMINFO_TYPE_MASK \
		(MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
		MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
		MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )
#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)
#define STATE_MASK (~TYPE_MASK)
#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

/*********************************************************************
 * menu class descriptor
 */
const struct builtin_class_descr MENU_builtin_class =
{
    (LPCWSTR)POPUPMENU_CLASS_ATOM,  /* name */
    CS_DROPSHADOW | CS_SAVEBITS | CS_DBLCLKS,  /* style */
    WINPROC_MENU,                  /* proc */
    sizeof(HMENU),                 /* extra */
    IDC_ARROW,                     /* cursor */
    (HBRUSH)(COLOR_MENU+1)         /* brush */
};


/***********************************************************************
 *           debug_print_menuitem
 *
 * Print a menuitem in readable form.
 */

#define debug_print_menuitem(pre, mp, post) \
    do { if (TRACE_ON(menu)) do_debug_print_menuitem(pre, mp, post); } while (0)

#define MENUOUT(text) \
  TRACE("%s%s", (count++ ? "," : ""), (text))

#define MENUFLAG(bit,text) \
  do { \
    if (flags & (bit)) { flags &= ~(bit); MENUOUT ((text)); } \
  } while (0)

static void do_debug_print_menuitem(const char *prefix, const MENUITEM *mp,
				    const char *postfix)
{
    static const char * const hbmmenus[] = { "HBMMENU_CALLBACK", "", "HBMMENU_SYSTEM",
    "HBMMENU_MBAR_RESTORE", "HBMMENU_MBAR_MINIMIZE", "UNKNOWN BITMAP", "HBMMENU_MBAR_CLOSE",
    "HBMMENU_MBAR_CLOSE_D", "HBMMENU_MBAR_MINIMIZE_D", "HBMMENU_POPUP_CLOSE",
    "HBMMENU_POPUP_RESTORE", "HBMMENU_POPUP_MAXIMIZE", "HBMMENU_POPUP_MINIMIZE"};
    TRACE("%s ", prefix);
    if (mp) {
        UINT flags = mp->fType;
        TRACE( "{ ID=0x%Ix", mp->wID);
        if ( mp->hSubMenu)
            TRACE( ", Sub=%p", mp->hSubMenu);
        if (flags) {
            int count = 0;
            TRACE( ", fType=");
            MENUFLAG( MFT_SEPARATOR, "sep");
            MENUFLAG( MFT_OWNERDRAW, "own");
            MENUFLAG( MFT_BITMAP, "bit");
            MENUFLAG(MF_POPUP, "pop");
            MENUFLAG(MFT_MENUBARBREAK, "barbrk");
            MENUFLAG(MFT_MENUBREAK, "brk");
            MENUFLAG(MFT_RADIOCHECK, "radio");
            MENUFLAG(MFT_RIGHTORDER, "rorder");
            MENUFLAG(MF_SYSMENU, "sys");
            MENUFLAG(MFT_RIGHTJUSTIFY, "right");  /* same as MF_HELP */
            if (flags)
                TRACE( "+0x%x", flags);
        }
        flags = mp->fState;
        if (flags) {
            int count = 0;
            TRACE( ", State=");
            MENUFLAG(MFS_GRAYED, "grey");
            MENUFLAG(MFS_DEFAULT, "default");
            MENUFLAG(MFS_DISABLED, "dis");
            MENUFLAG(MFS_CHECKED, "check");
            MENUFLAG(MFS_HILITE, "hi");
            MENUFLAG(MF_USECHECKBITMAPS, "usebit");
            MENUFLAG(MF_MOUSESELECT, "mouse");
            if (flags)
                TRACE( "+0x%x", flags);
        }
        if (mp->hCheckBit)
            TRACE( ", Chk=%p", mp->hCheckBit);
        if (mp->hUnCheckBit)
            TRACE( ", Unc=%p", mp->hUnCheckBit);
        if (mp->text)
            TRACE( ", Text=%s", debugstr_w(mp->text));
        if (mp->dwItemData)
            TRACE( ", ItemData=0x%08Ix", mp->dwItemData);
        if (mp->hbmpItem)
        {
            if( IS_MAGIC_BITMAP(mp->hbmpItem))
                TRACE( ", hbitmap=%s", hbmmenus[ (INT_PTR)mp->hbmpItem + 1]);
            else
                TRACE( ", hbitmap=%p", mp->hbmpItem);
        }
        TRACE( " }");
    } else
        TRACE( "NULL");
    TRACE(" %s\n", postfix);
}

#undef MENUOUT
#undef MENUFLAG


/***********************************************************************
 *           MENU_GetMenu
 *
 * Validate the given menu handle and returns the menu structure pointer.
 */
static POPUPMENU *MENU_GetMenu(HMENU hMenu)
{
    POPUPMENU *menu = get_user_handle_ptr( hMenu, NTUSER_OBJ_MENU );

    if (menu == OBJ_OTHER_PROCESS)
    {
        WARN( "other process menu %p?\n", hMenu);
        return NULL;
    }
    if (menu) release_user_handle_ptr( menu );  /* FIXME! */
    else WARN("invalid menu handle=%p\n", hMenu);
    return menu;
}

static POPUPMENU *grab_menu_ptr(HMENU hMenu)
{
    POPUPMENU *menu = get_user_handle_ptr( hMenu, NTUSER_OBJ_MENU );

    if (menu == OBJ_OTHER_PROCESS)
    {
        WARN("other process menu %p?\n", hMenu);
        return NULL;
    }

    if (menu)
        menu->refcount++;
    else
        WARN("invalid menu handle=%p\n", hMenu);
    return menu;
}

static void release_menu_ptr(POPUPMENU *menu)
{
    if (menu)
    {
        menu->refcount--;
        release_user_handle_ptr(menu);
    }
}

/***********************************************************************
 *           MENU_CopySysPopup
 *
 * Return the default system menu.
 */
static HMENU MENU_CopySysPopup(BOOL mdi)
{
    HMENU hMenu = LoadMenuW(user32_module, mdi ? L"SYSMENUMDI" : L"SYSMENU");

    if( hMenu ) {
        MENUINFO minfo;
        MENUITEMINFOW miteminfo;
        POPUPMENU* menu = MENU_GetMenu(hMenu);
        menu->wFlags |= MF_SYSMENU | MF_POPUP;
        /* decorate the menu with bitmaps */
        minfo.cbSize = sizeof( MENUINFO);
        minfo.dwStyle = MNS_CHECKORBMP;
        minfo.fMask = MIM_STYLE;
        SetMenuInfo( hMenu, &minfo);
        miteminfo.cbSize = sizeof( MENUITEMINFOW);
        miteminfo.fMask = MIIM_BITMAP;
        miteminfo.hbmpItem = HBMMENU_POPUP_CLOSE;
        SetMenuItemInfoW( hMenu, SC_CLOSE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_RESTORE;
        SetMenuItemInfoW( hMenu, SC_RESTORE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
        SetMenuItemInfoW( hMenu, SC_MAXIMIZE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_MINIMIZE;
        SetMenuItemInfoW( hMenu, SC_MINIMIZE, FALSE, &miteminfo);
        NtUserSetMenuDefaultItem( hMenu, SC_CLOSE, FALSE );
    }
    else
	ERR("Unable to load default system menu\n" );

    TRACE("returning %p (mdi=%d).\n", hMenu, mdi );

    return hMenu;
}


/**********************************************************************
 *           MENU_GetSysMenu
 *
 * Create a copy of the system menu. System menu in Windows is
 * a special menu bar with the single entry - system menu popup.
 * This popup is presented to the outside world as a "system menu".
 * However, the real system menu handle is sometimes seen in the
 * WM_MENUSELECT parameters (and Word 6 likes it this way).
 */
HMENU MENU_GetSysMenu( HWND hWnd, HMENU hPopupMenu )
{
    HMENU hMenu;

    TRACE("loading system menu, hWnd %p, hPopupMenu %p\n", hWnd, hPopupMenu);
    if ((hMenu = CreateMenu()))
    {
	POPUPMENU *menu = MENU_GetMenu(hMenu);
	menu->wFlags = MF_SYSMENU;
	menu->hWnd = WIN_GetFullHandle( hWnd );
	TRACE("hWnd %p (hMenu %p)\n", menu->hWnd, hMenu);

	if (!hPopupMenu)
        {
            if (GetWindowLongW(hWnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
	        hPopupMenu = MENU_CopySysPopup(TRUE);
            else
	        hPopupMenu = MENU_CopySysPopup(FALSE);
        }

	if (hPopupMenu)
	{
            if (GetClassLongW(hWnd, GCL_STYLE) & CS_NOCLOSE)
                NtUserDeleteMenu( hPopupMenu, SC_CLOSE, MF_BYCOMMAND );

	    InsertMenuW( hMenu, -1, MF_SYSMENU | MF_POPUP | MF_BYPOSITION,
                         (UINT_PTR)hPopupMenu, NULL );

            menu->items[0].fType = MF_SYSMENU | MF_POPUP;
            menu->items[0].fState = 0;
            if ((menu = MENU_GetMenu(hPopupMenu))) menu->wFlags |= MF_SYSMENU;

	    TRACE("hMenu=%p (hPopup %p)\n", hMenu, hPopupMenu );
	    return hMenu;
	}
	NtUserDestroyMenu( hMenu );
    }
    ERR("failed to load system menu!\n");
    return 0;
}


static POPUPMENU *find_menu_item(HMENU hmenu, UINT id, UINT flags, UINT *pos)
{
    UINT fallback_pos = ~0u, i;
    POPUPMENU *menu;

    menu = grab_menu_ptr(hmenu);
    if (!menu)
        return NULL;

    if (flags & MF_BYPOSITION)
    {
        if (id >= menu->nItems)
        {
            release_menu_ptr(menu);
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
                POPUPMENU *submenu = find_menu_item(item->hSubMenu, id, flags, pos);

                if (submenu)
                {
                    release_menu_ptr(menu);
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
        release_menu_ptr(menu);
        menu = NULL;
    }

    return menu;
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu )
{
    WORD flags, id = 0;
    LPCWSTR str;
    BOOL end_flag;

    do
    {
        flags = GET_WORD(res);
        end_flag = flags & MF_END;
        /* Remove MF_END because it has the same value as MF_HILITE */
        flags &= ~MF_END;
        res += sizeof(WORD);
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        str = (LPCWSTR)res;
        res += (lstrlenW(str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu ))) return NULL;
            AppendMenuW( hMenu, flags, (UINT_PTR)hSubMenu, str );
        }
        else  /* Not a popup */
        {
            AppendMenuW( hMenu, flags, id, *str ? str : NULL );
        }
    } while (!end_flag);
    return res;
}


/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU hMenu)
{
    WORD resinfo;
    do {
	MENUITEMINFOW mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
	mii.fType = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.fState = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.wID = GET_DWORD(res);
        res += sizeof(DWORD);
	resinfo = GET_WORD(res); /* FIXME: for 16-bit apps this is a byte.  */
        res += sizeof(WORD);
	/* Align the text on a word boundary.  */
	res += (~((UINT_PTR)res - 1)) & 1;
	mii.dwTypeData = (LPWSTR) res;
	res += (1 + lstrlenW(mii.dwTypeData)) * sizeof(WCHAR);
	/* Align the following fields on a dword boundary.  */
	res += (~((UINT_PTR)res - 1)) & 3;

        TRACE("Menu item: [%08x,%08x,%04x,%04x,%s]\n",
              mii.fType, mii.fState, mii.wID, resinfo, debugstr_w(mii.dwTypeData));

	if (resinfo & 1) {	/* Pop-up? */
	    /* DWORD helpid = GET_DWORD(res); FIXME: use this.  */
	    res += sizeof(DWORD);
	    mii.hSubMenu = CreatePopupMenu();
	    if (!mii.hSubMenu)
		return NULL;
	    if (!(res = MENUEX_ParseResource(res, mii.hSubMenu))) {
		NtUserDestroyMenu( mii.hSubMenu );
                return NULL;
	    }
	    mii.fMask |= MIIM_SUBMENU;
	    mii.fType |= MF_POPUP;
        }
	else if(!*mii.dwTypeData && !(mii.fType & MF_SEPARATOR))
	{
	    WARN("Converting NULL menu item %04x, type %04x to SEPARATOR\n",
		mii.wID, mii.fType);
	    mii.fType |= MF_SEPARATOR;
	}
	InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
    return res;
}


/**********************************************************************
 *           TrackPopupMenu   (USER32.@)
 *
 * Like the win32 API, the function return the command ID only if the
 * flag TPM_RETURNCMD is on.
 *
 */
BOOL WINAPI TrackPopupMenu( HMENU hMenu, UINT wFlags, INT x, INT y,
                            INT nReserved, HWND hWnd, const RECT *lpRect )
{
    return NtUserTrackPopupMenuEx( hMenu, wFlags, x, y, hWnd, NULL );
}

/***********************************************************************
 *           PopupMenuWndProc
 *
 * NOTE: Windows has totally different (and undocumented) popup wndproc.
 */
LRESULT WINAPI PopupMenuWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch(message)
    {
    case WM_DESTROY:
    case WM_CREATE:
    case WM_MOUSEACTIVATE:
    case WM_PAINT:
    case WM_PRINTCLIENT:
    case WM_ERASEBKGND:
    case WM_SHOWWINDOW:
    case MN_GETHMENU:
        return NtUserMessageCall( hwnd, message, wParam, lParam,
                                  NULL, NtUserPopupMenuWndProc, FALSE );

    default:
        return DefWindowProcW( hwnd, message, wParam, lParam );
    }
    return 0;
}


/*******************************************************************
 *         ChangeMenuA    (USER32.@)
 */
BOOL WINAPI ChangeMenuA( HMENU hMenu, UINT pos, LPCSTR data,
                             UINT id, UINT flags )
{
    TRACE("menu=%p pos=%d data=%p id=%08x flags=%08x\n", hMenu, pos, data, id, flags );
    if (flags & MF_APPEND) return AppendMenuA( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return NtUserDeleteMenu( hMenu, pos, flags & ~MF_DELETE );
    if (flags & MF_CHANGE) return ModifyMenuA(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return NtUserRemoveMenu( hMenu,
                                                    flags & MF_BYPOSITION ? pos : id,
                                                    flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuA( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         ChangeMenuW    (USER32.@)
 */
BOOL WINAPI ChangeMenuW( HMENU hMenu, UINT pos, LPCWSTR data,
                             UINT id, UINT flags )
{
    TRACE("menu=%p pos=%d data=%p id=%08x flags=%08x\n", hMenu, pos, data, id, flags );
    if (flags & MF_APPEND) return AppendMenuW( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return NtUserDeleteMenu( hMenu, pos, flags & ~MF_DELETE );
    if (flags & MF_CHANGE) return ModifyMenuW(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return NtUserRemoveMenu( hMenu,
                                                    flags & MF_BYPOSITION ? pos : id,
                                                    flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuW( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         GetMenuStringA    (USER32.@)
 */
INT WINAPI GetMenuStringA(
	HMENU hMenu,	/* [in] menuhandle */
	UINT wItemID,	/* [in] menu item (dep. on wFlags) */
	LPSTR str,	/* [out] outbuffer. If NULL, func returns entry length*/
	INT nMaxSiz,	/* [in] length of buffer. if 0, func returns entry len*/
	UINT wFlags	/* [in] MF_ flags */
)
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT pos;
    INT ret;

    TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, wItemID, str, nMaxSiz, wFlags );
    if (str && nMaxSiz) str[0] = '\0';

    if (!(menu = find_menu_item(hMenu, wItemID, wFlags, &pos)))
    {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return 0;
    }
    item = &menu->items[pos];

    if (!item->text)
        ret = 0;
    else if (!str || !nMaxSiz)
        ret = WideCharToMultiByte( CP_ACP, 0, item->text, -1, NULL, 0, NULL, NULL );
    else
    {
        if (!WideCharToMultiByte( CP_ACP, 0, item->text, -1, str, nMaxSiz, NULL, NULL ))
            str[nMaxSiz-1] = 0;
        ret = strlen(str);
    }
    release_menu_ptr(menu);

    TRACE("returning %s\n", debugstr_a(str));
    return ret;
}


/*******************************************************************
 *         GetMenuStringW    (USER32.@)
 */
INT WINAPI GetMenuStringW( HMENU hMenu, UINT wItemID,
                               LPWSTR str, INT nMaxSiz, UINT wFlags )
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT pos;
    INT ret;

    TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, wItemID, str, nMaxSiz, wFlags );
    if (str && nMaxSiz) str[0] = '\0';

    if (!(menu = find_menu_item(hMenu, wItemID, wFlags, &pos)))
    {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return 0;
    }
    item = &menu->items[pos];

    if (!str || !nMaxSiz)
        ret = item->text ? lstrlenW(item->text) : 0;
    else if (!item->text)
    {
        str[0] = 0;
        ret = 0;
    }
    else
    {
        lstrcpynW( str, item->text, nMaxSiz );
        ret = lstrlenW(str);
    }
    release_menu_ptr(menu);

    TRACE("returning %s\n", debugstr_w(str));
    return ret;
}


/**********************************************************************
 *         GetMenuState    (USER32.@)
 */
UINT WINAPI GetMenuState( HMENU menu, UINT item, UINT flags )
{
    return NtUserThunkedMenuItemInfo( menu, item, flags, NtUserGetMenuState, NULL, NULL );
}


/**********************************************************************
 *         GetMenuItemCount    (USER32.@)
 */
INT WINAPI GetMenuItemCount( HMENU menu )
{
    return NtUserGetMenuItemCount( menu );
}


/**********************************************************************
 *         GetMenuItemID    (USER32.@)
 */
UINT WINAPI GetMenuItemID( HMENU hMenu, INT nPos )
{
    POPUPMENU *menu;
    UINT id, pos;

    if (!(menu = find_menu_item(hMenu, nPos, MF_BYPOSITION, &pos)))
        return -1;

    id = menu->items[pos].fType & MF_POPUP ? -1 : menu->items[pos].wID;
    release_menu_ptr(menu);
    return id;
}


/**********************************************************************
 *         MENU_mnu2mnuii
 *
 * Uses flags, id and text ptr, passed by InsertMenu() and
 * ModifyMenu() to setup a MenuItemInfo structure.
 */
static void MENU_mnu2mnuii( UINT flags, UINT_PTR id, LPCWSTR str,
        LPMENUITEMINFOW pmii)
{
    ZeroMemory( pmii, sizeof( MENUITEMINFOW));
    pmii->cbSize = sizeof( MENUITEMINFOW);
    pmii->fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
    /* setting bitmap clears text and vice versa */
    if( IS_STRING_ITEM(flags)) {
        pmii->fMask |= MIIM_STRING | MIIM_BITMAP;
        if( !str)
            flags |= MF_SEPARATOR;
        /* Item beginning with a backspace is a help item */
        /* FIXME: wrong place, this is only true in win16 */
        else if( *str == '\b') {
            flags |= MF_HELP;
            str++;
        }
        pmii->dwTypeData = (LPWSTR)str;
    } else if( flags & MFT_BITMAP){
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->hbmpItem = (HBITMAP)str;
    }
    if( flags & MF_OWNERDRAW){
        pmii->fMask |= MIIM_DATA;
        pmii->dwItemData = (ULONG_PTR) str;
    }
    if( flags & MF_POPUP && MENU_GetMenu((HMENU)id)) {
        pmii->fMask |= MIIM_SUBMENU;
        pmii->hSubMenu = (HMENU)id;
    }
    if( flags & MF_SEPARATOR) flags |= MF_GRAYED | MF_DISABLED;
    pmii->fState = flags & MENUITEMINFO_STATE_MASK & ~MFS_DEFAULT;
    pmii->fType = flags & MENUITEMINFO_TYPE_MASK;
    pmii->wID = (UINT)id;
}


/*******************************************************************
 *         InsertMenuW    (USER32.@)
 */
BOOL WINAPI InsertMenuW( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCWSTR str )
{
    MENUITEMINFOW mii;

    if (IS_STRING_ITEM(flags) && str)
        TRACE("hMenu %p, pos %d, flags %08x, id %04Ix, str %s\n",
              hMenu, pos, flags, id, debugstr_w(str) );
    else TRACE("hMenu %p, pos %d, flags %08x, id %04Ix, str %p (not a string)\n",
               hMenu, pos, flags, id, str );

    MENU_mnu2mnuii( flags, id, str, &mii);
    mii.fMask |= MIIM_CHECKMARKS;
    return NtUserThunkedMenuItemInfo( hMenu, pos, flags, NtUserInsertMenuItem, &mii, NULL );
}


/*******************************************************************
 *         InsertMenuA    (USER32.@)
 */
BOOL WINAPI InsertMenuA( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCSTR str )
{
    BOOL ret = FALSE;

    if (IS_STRING_ITEM(flags) && str)
    {
        INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        LPWSTR newstr = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (newstr)
        {
            MultiByteToWideChar( CP_ACP, 0, str, -1, newstr, len );
            ret = InsertMenuW( hMenu, pos, flags, id, newstr );
            HeapFree( GetProcessHeap(), 0, newstr );
        }
        return ret;
    }
    else return InsertMenuW( hMenu, pos, flags, id, (LPCWSTR)str );
}


/*******************************************************************
 *         AppendMenuA    (USER32.@)
 */
BOOL WINAPI AppendMenuA( HMENU hMenu, UINT flags,
                         UINT_PTR id, LPCSTR data )
{
    return InsertMenuA( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenuW    (USER32.@)
 */
BOOL WINAPI AppendMenuW( HMENU hMenu, UINT flags,
                         UINT_PTR id, LPCWSTR data )
{
    return InsertMenuW( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         ModifyMenuW    (USER32.@)
 */
BOOL WINAPI ModifyMenuW( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCWSTR str )
{
    MENUITEMINFOW mii;

    if (IS_STRING_ITEM(flags))
        TRACE("%p %d %04x %04Ix %s\n", hMenu, pos, flags, id, debugstr_w(str) );
    else
        TRACE("%p %d %04x %04Ix %p\n", hMenu, pos, flags, id, str );

    MENU_mnu2mnuii( flags, id, str, &mii);
    return NtUserThunkedMenuItemInfo( hMenu, pos, flags, NtUserSetMenuItemInfo, &mii, NULL );
}


/*******************************************************************
 *         ModifyMenuA    (USER32.@)
 */
BOOL WINAPI ModifyMenuA( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCSTR str )
{
    BOOL ret = FALSE;

    if (IS_STRING_ITEM(flags) && str)
    {
        INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        LPWSTR newstr = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (newstr)
        {
            MultiByteToWideChar( CP_ACP, 0, str, -1, newstr, len );
            ret = ModifyMenuW( hMenu, pos, flags, id, newstr );
            HeapFree( GetProcessHeap(), 0, newstr );
        }
        return ret;
    }
    else return ModifyMenuW( hMenu, pos, flags, id, (LPCWSTR)str );
}


/**********************************************************************
 *         CreatePopupMenu    (USER32.@)
 */
HMENU WINAPI CreatePopupMenu(void)
{
    return NtUserCreateMenu( TRUE );
}


/**********************************************************************
 *         GetMenuCheckMarkDimensions    (USER.417)
 *         GetMenuCheckMarkDimensions    (USER32.@)
 */
DWORD WINAPI GetMenuCheckMarkDimensions(void)
{
    return MAKELONG( GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK) );
}


/**********************************************************************
 *         SetMenuItemBitmaps    (USER32.@)
 */
BOOL WINAPI SetMenuItemBitmaps( HMENU hMenu, UINT nPos, UINT wFlags,
                                    HBITMAP hNewUnCheck, HBITMAP hNewCheck)
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT pos;

    if (!(menu = find_menu_item(hMenu, nPos, wFlags, &pos)))
        return FALSE;

    item = &menu->items[pos];
    if (!hNewCheck && !hNewUnCheck)
    {
        item->fState &= ~MF_USECHECKBITMAPS;
    }
    else  /* Install new bitmaps */
    {
        item->hCheckBit = hNewCheck;
        item->hUnCheckBit = hNewUnCheck;
        item->fState |= MF_USECHECKBITMAPS;
    }
    release_menu_ptr(menu);

    return TRUE;
}


/**********************************************************************
 *         CreateMenu    (USER32.@)
 */
HMENU WINAPI CreateMenu(void)
{
    return NtUserCreateMenu( FALSE );
}


/**********************************************************************
 *         GetMenu    (USER32.@)
 */
HMENU WINAPI GetMenu( HWND hWnd )
{
    HMENU retvalue = (HMENU)GetWindowLongPtrW( hWnd, GWLP_ID );
    TRACE("for %p returning %p\n", hWnd, retvalue);
    return retvalue;
}


/**********************************************************************
 *         GetSubMenu    (USER32.@)
 */
HMENU WINAPI GetSubMenu( HMENU hMenu, INT nPos )
{
    POPUPMENU *menu;
    HMENU submenu;
    UINT pos;

    if (!(menu = find_menu_item(hMenu, nPos, MF_BYPOSITION, &pos)))
        return 0;

    if (menu->items[pos].fType & MF_POPUP)
        submenu = menu->items[pos].hSubMenu;
    else
        submenu = 0;

    release_menu_ptr(menu);
    return submenu;
}


/**********************************************************************
 *         DrawMenuBar    (USER32.@)
 */
BOOL WINAPI DrawMenuBar( HWND hwnd )
{
    return NtUserDrawMenuBar( hwnd );
}


/*****************************************************************
 *        LoadMenuA   (USER32.@)
 */
HMENU WINAPI LoadMenuA( HINSTANCE instance, LPCSTR name )
{
    HRSRC hrsrc = FindResourceA( instance, name, (LPSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectA( LoadResource( instance, hrsrc ));
}


/*****************************************************************
 *        LoadMenuW   (USER32.@)
 */
HMENU WINAPI LoadMenuW( HINSTANCE instance, LPCWSTR name )
{
    HRSRC hrsrc = FindResourceW( instance, name, (LPWSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectW( LoadResource( instance, hrsrc ));
}


/**********************************************************************
 *	    LoadMenuIndirectW    (USER32.@)
 */
HMENU WINAPI LoadMenuIndirectW( LPCVOID template )
{
    HMENU hMenu;
    WORD version, offset;
    LPCSTR p = template;

    version = GET_WORD(p);
    p += sizeof(WORD);
    TRACE("%p, ver %d\n", template, version );
    switch (version)
    {
      case 0: /* standard format is version of 0 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu())) return 0;
	if (!MENU_ParseResource( p, hMenu ))
	  {
            NtUserDestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      case 1: /* extended format is version of 1 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu())) return 0;
	if (!MENUEX_ParseResource( p, hMenu))
	  {
	    NtUserDestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      default:
        ERR("version %d not supported.\n", version);
        return 0;
    }
}


/**********************************************************************
 *	    LoadMenuIndirectA    (USER32.@)
 */
HMENU WINAPI LoadMenuIndirectA( LPCVOID template )
{
    return LoadMenuIndirectW( template );
}


/**********************************************************************
 *		IsMenu    (USER32.@)
 */
BOOL WINAPI IsMenu( HMENU menu )
{
    MENUINFO info;

    info.cbSize = sizeof(info);
    info.fMask = 0;
    if (GetMenuInfo( menu, &info )) return TRUE;

    SetLastError(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
}

/**********************************************************************
 *		GetMenuItemInfo_common
 */

static BOOL GetMenuItemInfo_common ( HMENU hmenu, UINT id, BOOL bypos,
					LPMENUITEMINFOW lpmii, BOOL unicode)
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT pos;

    menu = find_menu_item(hmenu, id, bypos ? MF_BYPOSITION : 0, &pos);

    item = menu ? &menu->items[pos] : NULL;

    debug_print_menuitem("GetMenuItemInfo_common: ", item, "");

    if (!menu)
    {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return FALSE;
    }

    if( lpmii->fMask & MIIM_TYPE) {
        if( lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP)) {
            release_menu_ptr(menu);
            WARN("invalid combination of fMask bits used\n");
            /* this does not happen on Win9x/ME */
            SetLastError( ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        lpmii->fType = item->fType & MENUITEMINFO_TYPE_MASK;
        if (item->hbmpItem && !IS_MAGIC_BITMAP(item->hbmpItem))
            lpmii->fType |= MFT_BITMAP;
        lpmii->hbmpItem = item->hbmpItem; /* not on Win9x/ME */
        if( lpmii->fType & MFT_BITMAP) {
	    lpmii->dwTypeData = (LPWSTR) item->hbmpItem;
	    lpmii->cch = 0;
        } else if( lpmii->fType & (MFT_OWNERDRAW | MFT_SEPARATOR)) {
            /* this does not happen on Win9x/ME */
	    lpmii->dwTypeData = 0;
	    lpmii->cch = 0;
        }
    }

    /* copy the text string */
    if ((lpmii->fMask & (MIIM_TYPE|MIIM_STRING))) {
         if (!item->text) {
                if(lpmii->dwTypeData && lpmii->cch) {
                    if( unicode)
                        *((WCHAR *)lpmii->dwTypeData) = 0;
                    else
                        *((CHAR *)lpmii->dwTypeData) = 0;
                }
                lpmii->cch = 0;
         } else {
            int len;
            if (unicode)
            {
                len = lstrlenW(item->text);
                if(lpmii->dwTypeData && lpmii->cch)
                    lstrcpynW(lpmii->dwTypeData, item->text, lpmii->cch);
            }
            else
            {
                len = WideCharToMultiByte( CP_ACP, 0, item->text, -1, NULL,
                        0, NULL, NULL ) - 1;
                if(lpmii->dwTypeData && lpmii->cch)
                    if (!WideCharToMultiByte( CP_ACP, 0, item->text, -1,
                            (LPSTR)lpmii->dwTypeData, lpmii->cch, NULL, NULL ))
                        ((LPSTR)lpmii->dwTypeData)[lpmii->cch - 1] = 0;
            }
            /* if we've copied a substring we return its length */
            if(lpmii->dwTypeData && lpmii->cch)
                if (lpmii->cch <= len + 1)
                    lpmii->cch--;
                else
                    lpmii->cch = len;
            else {
                /* return length of string */
                /* not on Win9x/ME if fType & MFT_BITMAP */
                lpmii->cch = len;
            }
        }
    }

    if (lpmii->fMask & MIIM_FTYPE)
        lpmii->fType = item->fType & MENUITEMINFO_TYPE_MASK;

    if (lpmii->fMask & MIIM_BITMAP)
        lpmii->hbmpItem = item->hbmpItem;

    if (lpmii->fMask & MIIM_STATE)
        lpmii->fState = item->fState & MENUITEMINFO_STATE_MASK;

    if (lpmii->fMask & MIIM_ID)
        lpmii->wID = item->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
        lpmii->hSubMenu = item->hSubMenu;
    else {
        /* hSubMenu is always cleared 
         * (not on Win9x/ME ) */
        lpmii->hSubMenu = 0;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS) {
        lpmii->hbmpChecked = item->hCheckBit;
        lpmii->hbmpUnchecked = item->hUnCheckBit;
    }
    if (lpmii->fMask & MIIM_DATA)
        lpmii->dwItemData = item->dwItemData;

    release_menu_ptr(menu);
    return TRUE;
}

/**********************************************************************
 *		GetMenuItemInfoA    (USER32.@)
 */
BOOL WINAPI GetMenuItemInfoA( HMENU hmenu, UINT item, BOOL bypos,
                                  LPMENUITEMINFOA lpmii)
{
    BOOL ret;
    MENUITEMINFOA mii;
    if( lpmii->cbSize != sizeof( mii) &&
            lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu, item, bypos,
                                    (LPMENUITEMINFOW)&mii, FALSE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}

/**********************************************************************
 *		GetMenuItemInfoW    (USER32.@)
 */
BOOL WINAPI GetMenuItemInfoW( HMENU hmenu, UINT item, BOOL bypos,
                                  LPMENUITEMINFOW lpmii)
{
    BOOL ret;
    MENUITEMINFOW mii;
    if( lpmii->cbSize != sizeof( mii) &&
            lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu, item, bypos, &mii, TRUE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}


/**********************************************************************
 *		MENU_NormalizeMenuItemInfoStruct
 *
 * Helper for SetMenuItemInfo and InsertMenuItemInfo:
 * check, copy and extend the MENUITEMINFO struct from the version that the application
 * supplied to the version used by wine source. */
static BOOL MENU_NormalizeMenuItemInfoStruct( const MENUITEMINFOW *pmii_in,
                                              MENUITEMINFOW *pmii_out )
{
    /* do we recognize the size? */
    if( !pmii_in || (pmii_in->cbSize != sizeof( MENUITEMINFOW) &&
            pmii_in->cbSize != sizeof( MENUITEMINFOW) - sizeof( pmii_in->hbmpItem)) ) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* copy the fields that we have */
    memcpy( pmii_out, pmii_in, pmii_in->cbSize);
    /* if the hbmpItem member is missing then extend */
    if( pmii_in->cbSize != sizeof( MENUITEMINFOW)) {
        pmii_out->cbSize = sizeof( MENUITEMINFOW);
        pmii_out->hbmpItem = NULL;
    }
    /* test for invalid bit combinations */
    if( (pmii_out->fMask & MIIM_TYPE &&
         pmii_out->fMask & (MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP)) ||
        (pmii_out->fMask & MIIM_FTYPE && pmii_out->fType & MFT_BITMAP)) {
        WARN("invalid combination of fMask bits used\n");
        /* this does not happen on Win9x/ME */
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* convert old style (MIIM_TYPE) to the new */
    if( pmii_out->fMask & MIIM_TYPE){
        pmii_out->fMask |= MIIM_FTYPE;
        if( IS_STRING_ITEM(pmii_out->fType)){
            pmii_out->fMask |= MIIM_STRING;
        } else if( (pmii_out->fType) & MFT_BITMAP){
            pmii_out->fMask |= MIIM_BITMAP;
            pmii_out->hbmpItem = UlongToHandle(LOWORD(pmii_out->dwTypeData));
        }
    }
    return TRUE;
}

/**********************************************************************
 *		SetMenuItemInfoA    (USER32.@)
 */
BOOL WINAPI SetMenuItemInfoA(HMENU hmenu, UINT item, BOOL bypos,
                                 const MENUITEMINFOA *lpmii)
{
    WCHAR *strW = NULL;
    MENUITEMINFOW mii;
    BOOL ret;

    TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

    if ((mii.fMask & MIIM_STRING) && mii.dwTypeData)
    {
        const char *str = (const char *)mii.dwTypeData;
        UINT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        mii.dwTypeData = strW;
    }

    ret = NtUserThunkedMenuItemInfo( hmenu, item, bypos ? MF_BYPOSITION : 0,
                                     NtUserSetMenuItemInfo, &mii, NULL );

    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}

/**********************************************************************
 *		SetMenuItemInfoW    (USER32.@)
 */
BOOL WINAPI SetMenuItemInfoW(HMENU hmenu, UINT item, BOOL bypos,
                                 const MENUITEMINFOW *lpmii)
{
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( lpmii, &mii )) return FALSE;

    return NtUserThunkedMenuItemInfo( hmenu, item, bypos ? MF_BYPOSITION : 0,
                                      NtUserSetMenuItemInfo, &mii, NULL );
}

/**********************************************************************
 *		GetMenuDefaultItem    (USER32.@)
 */
UINT WINAPI GetMenuDefaultItem(HMENU hmenu, UINT bypos, UINT flags)
{
	POPUPMENU *menu;
	MENUITEM * item;
	UINT i = 0;

	TRACE("(%p,%d,%d)\n", hmenu, bypos, flags);

	if (!(menu = MENU_GetMenu(hmenu))) return -1;

	/* find default item */
	item = menu->items;

	/* empty menu */
	if (! item) return -1;

	while ( !( item->fState & MFS_DEFAULT ) )
	{
	    i++; item++;
	    if  (i >= menu->nItems ) return -1;
	}

	/* default: don't return disabled items */
	if ( (!(GMDI_USEDISABLED & flags)) && (item->fState & MFS_DISABLED )) return -1;

	/* search rekursiv when needed */
	if ( (item->fType & MF_POPUP) &&  (flags & GMDI_GOINTOPOPUPS) )
	{
	    UINT ret;
	    ret = GetMenuDefaultItem( item->hSubMenu, bypos, flags );
	    if ( -1 != ret ) return ret;

	    /* when item not found in submenu, return the popup item */
	}
	return ( bypos ) ? i : item->wID;

}


/**********************************************************************
 *		InsertMenuItemA    (USER32.@)
 */
BOOL WINAPI InsertMenuItemA(HMENU hMenu, UINT uItem, BOOL bypos,
                                const MENUITEMINFOA *lpmii)
{
    WCHAR *strW = NULL;
    MENUITEMINFOW mii;
    BOOL ret;

    TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

    if ((mii.fMask & MIIM_STRING) && mii.dwTypeData)
    {
        const char *str = (const char *)mii.dwTypeData;
        UINT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        mii.dwTypeData = strW;
    }

    ret = NtUserThunkedMenuItemInfo( hMenu, uItem, bypos ? MF_BYPOSITION : 0,
                                     NtUserInsertMenuItem, &mii, NULL );

    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/**********************************************************************
 *		InsertMenuItemW    (USER32.@)
 */
BOOL WINAPI InsertMenuItemW(HMENU hMenu, UINT uItem, BOOL bypos,
                                const MENUITEMINFOW *lpmii)
{
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( lpmii, &mii )) return FALSE;

    return NtUserThunkedMenuItemInfo( hMenu, uItem, bypos ? MF_BYPOSITION : 0,
                                      NtUserInsertMenuItem, &mii, NULL );
}

/**********************************************************************
 *		CheckMenuRadioItem    (USER32.@)
 */

BOOL WINAPI CheckMenuRadioItem(HMENU hMenu, UINT first, UINT last,
    UINT check, UINT flags)
{
    POPUPMENU *first_menu = NULL, *check_menu;
    UINT i, check_pos;
    BOOL done = FALSE;

    for (i = first; i <= last; i++)
    {
        MENUITEM *item;

        if (!(check_menu = find_menu_item(hMenu, i, flags, &check_pos)))
            continue;

        if (!first_menu)
            first_menu = grab_menu_ptr(check_menu->obj.handle);

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
                /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
                item->fState &= ~MFS_CHECKED;
            }
        }

        release_menu_ptr(check_menu);
    }
    release_menu_ptr(first_menu);

    return done;
}


/**********************************************************************
 *		SetMenuInfo    (USER32.@)
 */
BOOL WINAPI SetMenuInfo( HMENU menu, const MENUINFO *info )
{
    TRACE( "(%p %p)\n", menu, info );

    if (!info || info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return NtUserThunkedMenuInfo( menu, info );
}

/**********************************************************************
 *           GetMenuInfo    (USER32.@)
 */
BOOL WINAPI GetMenuInfo( HMENU menu, MENUINFO *info )
{
    return NtUserGetMenuInfo( menu, info );
}


/**********************************************************************
 *         GetMenuContextHelpId    (USER32.@)
 */
DWORD WINAPI GetMenuContextHelpId( HMENU menu )
{
    MENUINFO info;
    TRACE( "(%p)\n", menu );
    info.cbSize = sizeof(info);
    info.fMask = MIM_HELPID;
    return GetMenuInfo( menu, &info ) ? info.dwContextHelpID : 0;
}

/**********************************************************************
 *         MenuItemFromPoint    (USER32.@)
 */
INT WINAPI MenuItemFromPoint( HWND hwnd, HMENU menu, POINT pt )
{
    return NtUserMenuItemFromPoint( hwnd, menu, pt.x, pt.y );
}


/**********************************************************************
 *      CalcMenuBar     (USER32.@)
 */
DWORD WINAPI CalcMenuBar(HWND hwnd, DWORD left, DWORD right, DWORD top, RECT *rect)
{
    FIXME("(%p, %ld, %ld, %ld, %p): stub\n", hwnd, left, right, top, rect);
    return 0;
}


/**********************************************************************
 *      TranslateAcceleratorA     (USER32.@)
 *      TranslateAccelerator      (USER32.@)
 */
INT WINAPI TranslateAcceleratorA( HWND hWnd, HACCEL hAccel, LPMSG msg )
{
    switch (msg->message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        return NtUserTranslateAccelerator( hWnd, hAccel, msg );

    case WM_CHAR:
    case WM_SYSCHAR:
        {
            MSG msgW = *msg;
            char ch = LOWORD(msg->wParam);
            WCHAR wch;
            MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
            msgW.wParam = MAKEWPARAM(wch, HIWORD(msg->wParam));
            return NtUserTranslateAccelerator( hWnd, hAccel, &msgW );
        }

    default:
        return 0;
    }
}
