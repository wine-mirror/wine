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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "controls.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(menu);


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
            HMENU hSubMenu = NtUserCreatePopupMenu();
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
	    mii.hSubMenu = NtUserCreatePopupMenu();
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
 *           PopupMenuWndProcA
 */
LRESULT WINAPI PopupMenuWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
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
                                  NULL, NtUserPopupMenuWndProc, TRUE );

    default:
        return DefWindowProcA( hwnd, message, wParam, lParam );
    }
    return 0;
}


/***********************************************************************
 *           PopupMenuWndProcW
 */
LRESULT WINAPI PopupMenuWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
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
INT WINAPI GetMenuStringA( HMENU menu, UINT item, char *str, INT count, UINT flags )
{
    MENUITEMINFOA info;
    int ret;

    TRACE( "menu=%p item=%04x ptr=%p len=%d flags=%04x\n", menu, item, str, count, flags );

    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING;
    info.dwTypeData = str;
    info.cch = count;
    ret = NtUserThunkedMenuItemInfo( menu, item, flags, NtUserGetMenuItemInfoA,
                                     (MENUITEMINFOW *)&info, NULL );
    if (ret) ret = info.cch;
    TRACE( "returning %s %d\n", debugstr_a( str ), ret );
    return ret;
}


/*******************************************************************
 *         GetMenuStringW    (USER32.@)
 */
INT WINAPI GetMenuStringW( HMENU menu, UINT item, WCHAR *str, INT count, UINT flags )
{
    MENUITEMINFOW info;
    int ret;

    TRACE( "menu=%p item=%04x ptr=%p len=%d flags=%04x\n", menu, item, str, count, flags );

    info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING;
    info.dwTypeData = str;
    info.cch = count;
    ret = NtUserThunkedMenuItemInfo( menu, item, flags, NtUserGetMenuItemInfoW, &info, NULL );
    if (ret) ret = info.cch;
    TRACE( "returning %s %d\n", debugstr_w( str ), ret );
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
UINT WINAPI GetMenuItemID( HMENU menu, INT pos )
{
    return NtUserThunkedMenuItemInfo( menu, pos, MF_BYPOSITION, NtUserGetMenuItemID, NULL, NULL );
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
    if ((flags & MF_POPUP) && IsMenu( UlongToHandle( id )))
    {
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
BOOL WINAPI SetMenuItemBitmaps( HMENU menu, UINT pos, UINT flags, HBITMAP uncheck, HBITMAP check )
{
    MENUITEMINFOW info;

    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    if (!NtUserThunkedMenuItemInfo( menu, pos, flags, NtUserGetMenuItemInfoW, &info, NULL ))
        return FALSE;

    info.fMask = MIIM_STATE | MIIM_CHECKMARKS;
    info.hbmpChecked = check;
    info.hbmpUnchecked = uncheck;
    if (check || uncheck) info.fState |= MF_USECHECKBITMAPS;
    else info.fState &= ~MF_USECHECKBITMAPS;
    return NtUserThunkedMenuItemInfo( menu, pos, flags, NtUserSetMenuItemInfo, &info, NULL );
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
HMENU WINAPI GetSubMenu( HMENU menu, INT pos )
{
    UINT ret = NtUserThunkedMenuItemInfo( menu, pos, MF_BYPOSITION, NtUserGetSubMenu, NULL, NULL );
    return UlongToHandle( ret );
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
	if (!(hMenu = NtUserCreateMenu())) return 0;
	if (!MENU_ParseResource( p, hMenu ))
	  {
            NtUserDestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      case 1: /* extended format is version of 1 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = NtUserCreateMenu())) return 0;
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
    ret = NtUserThunkedMenuItemInfo( hmenu, item, bypos ? MF_BYPOSITION : 0,
                                     NtUserGetMenuItemInfoA, (MENUITEMINFOW *)&mii, NULL );
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize );
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
    ret = NtUserThunkedMenuItemInfo( hmenu, item, bypos ? MF_BYPOSITION : 0,
                                     NtUserGetMenuItemInfoW, &mii, NULL );
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize );
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
UINT WINAPI GetMenuDefaultItem( HMENU menu, UINT bypos, UINT flags )
{
    return NtUserThunkedMenuItemInfo( menu, bypos, flags, NtUserGetMenuDefaultItem, NULL, NULL );
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
BOOL WINAPI CheckMenuRadioItem( HMENU menu, UINT first, UINT last, UINT check, UINT flags )
{
    MENUITEMINFOW info; /* abuse to pass last and check */
    info.cch = last;
    info.fMask = check;
    return NtUserThunkedMenuItemInfo( menu, first, flags, NtUserCheckMenuRadioItem, &info, NULL );
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
