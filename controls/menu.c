/*
 * Menu functions
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 */

/*
 * Note: the style MF_MOUSESELECT is used to mark popup items that
 * have been selected, i.e. their popup menu is currently displayed.
 * This is probably not the meaning this style has in MS-Windows.
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "syscolor.h"
#include "sysmetrics.h"
#include "task.h"
#include "win.h"
#include "heap.h"
#include "menu.h"
#include "module.h"
#include "neexe.h"
#include "nonclient.h"
#include "user.h"
#include "message.h"
#include "graphics.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"

/* Menu item structure */
typedef struct
{
    UINT32      item_flags;    /* Item flags */
    UINT32      item_id;       /* Item or popup id */
    RECT32      rect;          /* Item area (relative to menu window) */
    UINT32      xTab;          /* X position of text after Tab */
    HBITMAP32   hCheckBit;     /* Bitmap for checked item */
    HBITMAP32   hUnCheckBit;   /* Bitmap for unchecked item */
    LPSTR       text;          /* Item text or bitmap handle */
} MENUITEM;

/* Popup menu structure */
typedef struct
{
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        wMagic;       /* Magic number */
    HQUEUE16    hTaskQ;       /* Task queue for this menu */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    WORD	nItems;       /* Number of items in the menu */
    HWND32      hWnd;         /* Window containing the menu */
    MENUITEM   *items;        /* Array of menu items */
    UINT32      FocusedItem;  /* Currently focused item */
} POPUPMENU, *LPPOPUPMENU;

#define MENU_MAGIC   0x554d  /* 'MU' */

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

  /* Dimension of the menu bitmaps */
static WORD check_bitmap_width = 0, check_bitmap_height = 0;
static WORD arrow_bitmap_width = 0, arrow_bitmap_height = 0;

  /* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL32 fEndMenuCalled = FALSE;

  /* Space between 2 menu bar items */
#define MENU_BAR_ITEMS_SPACE  16

  /* Minimum width of a tab character */
#define MENU_TAB_SPACE        8

  /* Height of a separator item */
#define SEPARATOR_HEIGHT      5

  /* Values for menu->FocusedItem */
  /* (other values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff
#define SYSMENU_SELECTED  0xfffe  /* Only valid on menu-bars */

#define IS_STRING_ITEM(flags) \
    (!((flags) & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)))

static HBITMAP32 hStdCheck = 0;
static HBITMAP32 hStdMnArrow = 0;
static HMENU32 MENU_DefSysMenu = 0;  /* Default system menu */


/* we _can_ use global popup window because there's no way 2 menues can
 * be tracked at the same time.
 */ 

static WND* pTopPWnd   = 0;
static UINT32 uSubPWndLevel = 0;


/**********************************************************************
 *           MENU_CopySysMenu
 *
 * Load a copy of the system menu.
 */
static HMENU32 MENU_CopySysMenu(void)
{
    HMENU32 hMenu;
    POPUPMENU *menu;

    if (!(hMenu = LoadMenuIndirect32A( SYSRES_GetResPtr(SYSRES_MENU_SYSMENU))))
    {
	dprintf_menu(stddeb,"No SYSMENU\n");
	return 0;
    }
    menu = (POPUPMENU*) USER_HEAP_LIN_ADDR(hMenu);
    menu->wFlags |= MF_SYSMENU | MF_POPUP;
    dprintf_menu(stddeb,"CopySysMenu hMenu=%04x !\n", hMenu);
    return hMenu;
}


/***********************************************************************
 *           MENU_Init
 *
 * Menus initialisation.
 */
BOOL32 MENU_Init()
{
    BITMAP32 bm;

      /* Load bitmaps */

    if (!(hStdCheck = LoadBitmap32A( 0, (LPSTR)MAKEINTRESOURCE(OBM_CHECK) )))
	return FALSE;
    GetObject32A( hStdCheck, sizeof(bm), &bm );
    check_bitmap_width = bm.bmWidth;
    check_bitmap_height = bm.bmHeight;
    if (!(hStdMnArrow = LoadBitmap32A(0,(LPSTR)MAKEINTRESOURCE(OBM_MNARROW))))
	return FALSE;
    GetObject32A( hStdMnArrow, sizeof(bm), &bm );
    arrow_bitmap_width = bm.bmWidth;
    arrow_bitmap_height = bm.bmHeight;

    if (!(MENU_DefSysMenu = MENU_CopySysMenu()))
    {
        fprintf( stderr, "Unable to create default system menu\n" );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_GetDefSysMenu
 *
 * Return the default system menu.
 */
HMENU32 MENU_GetDefSysMenu(void)
{
    return MENU_DefSysMenu;
}


/***********************************************************************
 *           MENU_HasSysMenu
 *
 * Check whether the window owning the menu bar has a system menu.
 */
static BOOL32 MENU_HasSysMenu( POPUPMENU *menu )
{
    WND *wndPtr;

    if (menu->wFlags & MF_POPUP) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return FALSE;
    return (wndPtr->dwStyle & WS_SYSMENU) != 0;
}


/***********************************************************************
 *           MENU_IsInSysMenu
 *
 * Check whether the point (in screen coords) is in the system menu
 * of the window owning the given menu.
 */
static BOOL32 MENU_IsInSysMenu( POPUPMENU *menu, POINT32 pt )
{
    WND *wndPtr;

    if (menu->wFlags & MF_POPUP) return FALSE;
    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return FALSE;
    if (!(wndPtr->dwStyle & WS_SYSMENU)) return FALSE;
    if ((pt.x < wndPtr->rectClient.left) ||
	(pt.x >= wndPtr->rectClient.left+SYSMETRICS_CXSIZE+SYSMETRICS_CXBORDER))
	return FALSE;
    if ((pt.y >= wndPtr->rectClient.top - menu->Height) ||
	(pt.y < wndPtr->rectClient.top - menu->Height -
	              SYSMETRICS_CYSIZE - SYSMETRICS_CYBORDER)) return FALSE;
    return TRUE;
}


/***********************************************************************
 *           MENU_InitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
void MENU_InitSysMenuPopup( HMENU32 hmenu, DWORD style, DWORD clsStyle )
{
    BOOL32 gray;

    gray = !(style & WS_THICKFRAME) || (style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem32( hmenu, SC_SIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = ((style & WS_MAXIMIZE) != 0);
    EnableMenuItem32( hmenu, SC_MOVE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MINIMIZEBOX) || (style & WS_MINIMIZE);
    EnableMenuItem32( hmenu, SC_MINIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MAXIMIZEBOX) || (style & WS_MAXIMIZE);
    EnableMenuItem32( hmenu, SC_MAXIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem32( hmenu, SC_RESTORE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = (clsStyle & CS_NOCLOSE) != 0;
    EnableMenuItem32( hmenu, SC_CLOSE, (gray ? MF_GRAYED : MF_ENABLED) );
}


/***********************************************************************
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
static MENUITEM *MENU_FindItem( HMENU32 *hmenu, UINT32 *nPos, UINT32 wFlags )
{
    POPUPMENU *menu;
    UINT32 i;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR(*hmenu))) return NULL;
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->nItems) return NULL;
	return &menu->items[*nPos];
    }
    else
    {
        MENUITEM *item = menu->items;
	for (i = 0; i < menu->nItems; i++, item++)
	{
	    if (item->item_id == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	    else if (item->item_flags & MF_POPUP)
	    {
		HMENU32 hsubmenu = (HMENU32)item->item_id;
		MENUITEM *subitem = MENU_FindItem( &hsubmenu, nPos, wFlags );
		if (subitem)
		{
		    *hmenu = hsubmenu;
		    return subitem;
		}
	    }
	}
    }
    return NULL;
}


/***********************************************************************
 *           MENU_FindItemByCoords
 *
 * Find the item at the specified coordinates (screen coords).
 */
static MENUITEM *MENU_FindItemByCoords( POPUPMENU *menu, INT32 x, INT32 y,
                                        UINT32 *pos )
{
    MENUITEM *item;
    WND *wndPtr;
    UINT32 i;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return NULL;
    x -= wndPtr->rectWindow.left;
    y -= wndPtr->rectWindow.top;
    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
	if ((x >= item->rect.left) && (x < item->rect.right) &&
	    (y >= item->rect.top) && (y < item->rect.bottom))
	{
	    if (pos) *pos = i;
	    return item;
	}
    }
    return NULL;
}


/***********************************************************************
 *           MENU_FindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT32 MENU_FindItemByKey( HWND32 hwndOwner, HMENU32 hmenu, UINT32 key )
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT32 i;
    LONG menuchar;

    if (!IsMenu32( hmenu )) hmenu = WIN_FindWndPtr(hwndOwner)->hSysMenu;
    if (!hmenu) return -1;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    item = menu->items;
    key = toupper(key);
    for (i = 0; i < menu->nItems; i++, item++)
    {
	if (IS_STRING_ITEM(item->item_flags))
	{
	    char *p = strchr( item->text, '&' );
	    if (p && (p[1] != '&') && (toupper(p[1]) == key)) return i;
	}
    }
    menuchar = SendMessage32A( hwndOwner, WM_MENUCHAR, 
                               MAKEWPARAM( key, menu->wFlags ), hmenu );
    if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
    if (HIWORD(menuchar) == 1) return -2;
    return -1;
}


/***********************************************************************
 *           MENU_CalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void MENU_CalcItemSize( HDC32 hdc, MENUITEM *lpitem, HWND32 hwndOwner,
			       INT32 orgX, INT32 orgY, BOOL32 menuBar )
{
    DWORD dwSize;
    char *p;

    SetRect32( &lpitem->rect, orgX, orgY, orgX, orgY );

    if (lpitem->item_flags & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT32 mis;
        mis.CtlType    = ODT_MENU;
        mis.itemID     = lpitem->item_id;
        mis.itemData   = (DWORD)lpitem->text;
        mis.itemHeight = 16;
        mis.itemWidth  = 30;
        SendMessage32A( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        lpitem->rect.bottom += mis.itemHeight;
        lpitem->rect.right  += mis.itemWidth;
        dprintf_menu( stddeb, "DrawMenuItem: MeasureItem %04x %dx%d!\n",
                      lpitem->item_id, mis.itemWidth, mis.itemHeight );
        return;
    } 

    if (lpitem->item_flags & MF_SEPARATOR)
    {
	lpitem->rect.bottom += SEPARATOR_HEIGHT;
	return;
    }

    if (!menuBar)
    {
	lpitem->rect.right += 2 * check_bitmap_width;
	if (lpitem->item_flags & MF_POPUP)
	    lpitem->rect.right += arrow_bitmap_width;
    }

    if (lpitem->item_flags & MF_BITMAP)
    {
	BITMAP32 bm;
        if (GetObject32A( (HBITMAP32)lpitem->text, sizeof(bm), &bm ))
        {
            lpitem->rect.right  += bm.bmWidth;
            lpitem->rect.bottom += bm.bmHeight;
        }
	return;
    }
    
    /* If we get here, then it must be a text item */

    if (IS_STRING_ITEM( lpitem->item_flags ))
    {
        dwSize = GetTextExtent( hdc, lpitem->text, strlen(lpitem->text) );
        lpitem->rect.right  += LOWORD(dwSize);
        lpitem->rect.bottom += MAX( HIWORD(dwSize), SYSMETRICS_CYMENU );
        lpitem->xTab = 0;

        if (menuBar) lpitem->rect.right += MENU_BAR_ITEMS_SPACE;
        else if ((p = strchr( lpitem->text, '\t' )) != NULL)
        {
            /* Item contains a tab (only meaningful in popup menus) */
            lpitem->xTab = check_bitmap_width + MENU_TAB_SPACE + 
                LOWORD( GetTextExtent( hdc, lpitem->text,
                                       (int)(p - lpitem->text) ));
            lpitem->rect.right += MENU_TAB_SPACE;
        }
        else
        {
            if (strchr( lpitem->text, '\b' ))
                lpitem->rect.right += MENU_TAB_SPACE;
            lpitem->xTab = lpitem->rect.right - check_bitmap_width 
                           - arrow_bitmap_width;
        }
    }
}


/***********************************************************************
 *           MENU_PopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void MENU_PopupMenuCalcSize( LPPOPUPMENU lppop, HWND32 hwndOwner )
{
    MENUITEM *lpitem;
    HDC32 hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth;

    lppop->Width = lppop->Height = 0;
    if (lppop->nItems == 0) return;
    hdc = GetDC32( 0 );
    maxX = start = 0;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = maxX;
	orgY = 0;
	maxTab = maxTabWidth = 0;

	  /* Parse items until column break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((i != start) &&
		(lpitem->item_flags & (MF_MENUBREAK | MF_MENUBARBREAK))) break;
	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, FALSE );
            if (lpitem->item_flags & MF_MENUBARBREAK) orgX++;
	    maxX = MAX( maxX, lpitem->rect.right );
	    orgY = lpitem->rect.bottom;
	    if (IS_STRING_ITEM(lpitem->item_flags) && lpitem->xTab)
	    {
		maxTab = MAX( maxTab, lpitem->xTab );
		maxTabWidth = MAX(maxTabWidth,lpitem->rect.right-lpitem->xTab);
	    }
	}

	  /* Finish the column (set all items to the largest width found) */
	maxX = MAX( maxX, maxTab + maxTabWidth );
	for (lpitem = &lppop->items[start]; start < i; start++, lpitem++)
	{
	    lpitem->rect.right = maxX;
	    if (IS_STRING_ITEM(lpitem->item_flags) && lpitem->xTab)
                lpitem->xTab = maxTab;
	}
	lppop->Height = MAX( lppop->Height, orgY );
    }

    lppop->Width  = maxX;
    ReleaseDC32( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * Calculate the size of the menu bar.
 */
static void MENU_MenuBarCalcSize( HDC32 hdc, LPRECT32 lprect,
                                  LPPOPUPMENU lppop, HWND32 hwndOwner )
{
    MENUITEM *lpitem;
    int start, i, orgX, orgY, maxY, helpPos;

    if ((lprect == NULL) || (lppop == NULL)) return;
    if (lppop->nItems == 0) return;
    dprintf_menu(stddeb,"MENU_MenuBarCalcSize left=%d top=%d right=%d bottom=%d\n", 
                 lprect->left, lprect->top, lprect->right, lprect->bottom);
    lppop->Width  = lprect->right - lprect->left;
    lppop->Height = 0;
    maxY = lprect->top;
    start = 0;
    helpPos = -1;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = lprect->left;
	orgY = maxY;

	  /* Parse items until line break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((helpPos == -1) && (lpitem->item_flags & MF_HELP)) helpPos = i;
	    if ((i != start) &&
		(lpitem->item_flags & (MF_MENUBREAK | MF_MENUBARBREAK))) break;
	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, TRUE );
	    if (lpitem->rect.right > lprect->right)
	    {
		if (i != start) break;
		else lpitem->rect.right = lprect->right;
	    }
	    maxY = MAX( maxY, lpitem->rect.bottom );
	    orgX = lpitem->rect.right;
	}

	  /* Finish the line (set all items to the largest height found) */
	while (start < i) lppop->items[start++].rect.bottom = maxY;
    }

    lprect->bottom = maxY;
    lppop->Height = lprect->bottom - lprect->top;

      /* Flush right all items between the MF_HELP and the last item */
      /* (if several lines, only move the last line) */
    if (helpPos != -1)
    {
	lpitem = &lppop->items[lppop->nItems-1];
	orgY = lpitem->rect.top;
	orgX = lprect->right;
	for (i = lppop->nItems - 1; i >= helpPos; i--, lpitem--)
	{
	    if (lpitem->rect.top != orgY) break;    /* Other line */
	    if (lpitem->rect.right >= orgX) break;  /* Too far right already */
	    lpitem->rect.left += orgX - lpitem->rect.right;
	    lpitem->rect.right = orgX;
	    orgX = lpitem->rect.left;
	}
    }
}


/***********************************************************************
 *           MENU_DrawMenuItem
 *
 * Draw a single menu item.
 */
static void MENU_DrawMenuItem( HWND32 hwnd, HDC32 hdc, MENUITEM *lpitem,
			       UINT32 height, BOOL32 menuBar )
{
    RECT32 rect;

    if (lpitem->item_flags & MF_OWNERDRAW)
    {
        DRAWITEMSTRUCT32 dis;

        dprintf_menu( stddeb, "DrawMenuItem: Ownerdraw!\n" );
        dis.CtlType   = ODT_MENU;
        dis.itemID    = lpitem->item_id;
        dis.itemData  = (DWORD)lpitem->text;
        dis.itemState = 0;
        if (lpitem->item_flags & MF_CHECKED) dis.itemState |= ODS_CHECKED;
        if (lpitem->item_flags & MF_GRAYED)  dis.itemState |= ODS_GRAYED;
        if (lpitem->item_flags & MF_HILITE)  dis.itemState |= ODS_SELECTED;
        dis.itemAction = ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS;
        dis.hwndItem   = hwnd;
        dis.hDC        = hdc;
        dis.rcItem     = lpitem->rect;
        SendMessage32A( hwnd, WM_DRAWITEM, 0, (LPARAM)&dis );
        return;
    }

    if (menuBar && (lpitem->item_flags & MF_SEPARATOR)) return;
    rect = lpitem->rect;

      /* Draw the background */

    if (lpitem->item_flags & MF_HILITE)
	FillRect32( hdc, &rect, sysColorObjects.hbrushHighlight );
    else FillRect32( hdc, &rect, sysColorObjects.hbrushMenu );
    SetBkMode32( hdc, TRANSPARENT );

      /* Draw the separator bar (if any) */

    if (!menuBar && (lpitem->item_flags & MF_MENUBARBREAK))
    {
	SelectObject32( hdc, sysColorObjects.hpenWindowFrame );
	MoveTo( hdc, rect.left, 0 );
	LineTo32( hdc, rect.left, height );
    }
    if (lpitem->item_flags & MF_SEPARATOR)
    {
	SelectObject32( hdc, sysColorObjects.hpenWindowFrame );
	MoveTo( hdc, rect.left, rect.top + SEPARATOR_HEIGHT/2 );
	LineTo32( hdc, rect.right, rect.top + SEPARATOR_HEIGHT/2 );
	return;
    }

      /* Setup colors */

    if (lpitem->item_flags & MF_HILITE)
    {
	if (lpitem->item_flags & MF_GRAYED)
	    SetTextColor32( hdc, GetSysColor32( COLOR_GRAYTEXT ) );
	else
	    SetTextColor32( hdc, GetSysColor32( COLOR_HIGHLIGHTTEXT ) );
	SetBkColor32( hdc, GetSysColor32( COLOR_HIGHLIGHT ) );
    }
    else
    {
	if (lpitem->item_flags & MF_GRAYED)
	    SetTextColor32( hdc, GetSysColor32( COLOR_GRAYTEXT ) );
	else
	    SetTextColor32( hdc, GetSysColor32( COLOR_MENUTEXT ) );
	SetBkColor32( hdc, GetSysColor32( COLOR_MENU ) );
    }

    if (!menuBar)
    {
	  /* Draw the check mark */

	if (lpitem->item_flags & MF_CHECKED)
	{
	    GRAPH_DrawBitmap(hdc, lpitem->hCheckBit ? lpitem->hCheckBit :
			     hStdCheck, rect.left,
			     (rect.top+rect.bottom-check_bitmap_height) / 2,
			     0, 0, check_bitmap_width, check_bitmap_height );
	}
	else if (lpitem->hUnCheckBit != 0)  /* Not checked */
	{
	    GRAPH_DrawBitmap(hdc, lpitem->hUnCheckBit, rect.left,
			     (rect.top+rect.bottom-check_bitmap_height) / 2,
			     0, 0, check_bitmap_width, check_bitmap_height );
	}

	  /* Draw the popup-menu arrow */

	if (lpitem->item_flags & MF_POPUP)
	{
	    GRAPH_DrawBitmap( hdc, hStdMnArrow,
			      rect.right-arrow_bitmap_width-1,
			      (rect.top+rect.bottom-arrow_bitmap_height) / 2,
                              0, 0, arrow_bitmap_width, arrow_bitmap_height );
	}

	rect.left += check_bitmap_width;
	rect.right -= arrow_bitmap_width;
    }

      /* Draw the item text or bitmap */

    if (lpitem->item_flags & MF_BITMAP)
    {
	GRAPH_DrawBitmap( hdc, (HBITMAP32)lpitem->text,
                          rect.left, rect.top, 0, 0,
                          rect.right-rect.left, rect.bottom-rect.top );
	return;
    }
    /* No bitmap - process text if present */
    else if (IS_STRING_ITEM(lpitem->item_flags))
    {
	register int i;

	if (menuBar)
	{
	    rect.left += MENU_BAR_ITEMS_SPACE / 2;
	    rect.right -= MENU_BAR_ITEMS_SPACE / 2;
	    i = strlen( lpitem->text );
	}
	else
	{
	    for (i = 0; lpitem->text[i]; i++)
                if ((lpitem->text[i] == '\t') || (lpitem->text[i] == '\b'))
                    break;
	}
	
	DrawText32A( hdc, lpitem->text, i, &rect,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE );

	if (lpitem->text[i])  /* There's a tab or flush-right char */
	{
	    if (lpitem->text[i] == '\t')
	    {
		rect.left = lpitem->xTab;
		DrawText32A( hdc, lpitem->text + i + 1, -1, &rect,
                             DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	    }
	    else DrawText32A( hdc, lpitem->text + i + 1, -1, &rect,
                              DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
	}
    }
}


/***********************************************************************
 *           MENU_DrawPopupMenu
 *
 * Paint a popup menu.
 */
static void MENU_DrawPopupMenu( HWND32 hwnd, HDC32 hdc, HMENU32 hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;
    RECT32 rect;
    UINT32 i;

    GetClientRect32( hwnd, &rect );
    FillRect32( hdc, &rect, sysColorObjects.hbrushMenu );
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu || !menu->nItems) return;
    for (i = menu->nItems, item = menu->items; i > 0; i--, item++)
	MENU_DrawMenuItem( hwnd, hdc, item, menu->Height, FALSE );
}


/***********************************************************************
 *           MENU_DrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 */
UINT32 MENU_DrawMenuBar( HDC32 hDC, LPRECT32 lprect, HWND32 hwnd,
                         BOOL32 suppress_draw)
{
    LPPOPUPMENU lppop;
    UINT32 i;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR( (HMENU16)wndPtr->wIDmenu );
    if (lppop == NULL || lprect == NULL) return SYSMETRICS_CYMENU;
    dprintf_menu(stddeb,"MENU_DrawMenuBar(%04x, %p, %p); !\n", 
		 hDC, lprect, lppop);
    if (lppop->Height == 0) MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);
    lprect->bottom = lprect->top + lppop->Height;
    if (suppress_draw) return lppop->Height;
    
    FillRect32(hDC, lprect, sysColorObjects.hbrushMenu );
    SelectObject32( hDC, sysColorObjects.hpenWindowFrame );
    MoveTo( hDC, lprect->left, lprect->bottom );
    LineTo32( hDC, lprect->right, lprect->bottom );

    if (lppop->nItems == 0) return SYSMETRICS_CYMENU;
    for (i = 0; i < lppop->nItems; i++)
    {
	MENU_DrawMenuItem( hwnd, hDC, &lppop->items[i], lppop->Height, TRUE );
    }
    return lppop->Height;
} 


/***********************************************************************
 *	     MENU_SwitchTPWndTo
 */
BOOL32 MENU_SwitchTPWndTo( HTASK16 hTask )
{
  /* This is supposed to be called when popup is hidden. 
   * AppExit() calls with hTask == 0, so we get the next to current.
   */

  TDB* task;

  if( !pTopPWnd ) return 0;

  if( !hTask )
  {
    task = (TDB*)GlobalLock16( (hTask = GetCurrentTask()) );
    if( task && task->hQueue == pTopPWnd->hmemTaskQ )
	hTask = TASK_GetNextTask(hTask); 
    else return 0;
  }

  task = (TDB*)GlobalLock16(hTask);
  if( !task ) return 0;

  /* if this task got as far as menu tracking it must have a queue */

  pTopPWnd->hInstance = task->hInstance;
  pTopPWnd->hmemTaskQ = task->hQueue;
  return 1;
}

/***********************************************************************
 *           MENU_ShowPopup
 *
 * Display a popup menu.
 */
static BOOL32 MENU_ShowPopup( HWND32 hwndOwner, HMENU32 hmenu, UINT32 id,
                              INT32 x, INT32 y, INT32 xanchor, INT32 yanchor )
{
    POPUPMENU 	*menu;
    WND 	*wndPtr = NULL;
    BOOL32	 skip_init = 0;
    UINT32	 width, height;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	menu->items[menu->FocusedItem].item_flags &= ~(MF_HILITE|MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }
    SendMessage16( hwndOwner, WM_INITMENUPOPUP, (WPARAM16)hmenu,
                   MAKELONG( id, (menu->wFlags & MF_SYSMENU) ? 1 : 0 ));
    MENU_PopupMenuCalcSize( menu, hwndOwner );

    /* adjust popup menu pos so that it fits within the desktop */

    width = menu->Width + 2*SYSMETRICS_CXBORDER;
    height = menu->Height + 2*SYSMETRICS_CYBORDER; 

    if( x + width > SYSMETRICS_CXSCREEN )
    {
	if( xanchor )
            x -= width - xanchor;
        if( x + width > SYSMETRICS_CXSCREEN)
	    x = SYSMETRICS_CXSCREEN - width;
    }
    if( x < 0 )
         x = 0;

    if( y + height > SYSMETRICS_CYSCREEN )
    { 
	if( yanchor )
	    y -= height + yanchor;
	if( y + height > SYSMETRICS_CYSCREEN )
	    y = SYSMETRICS_CYSCREEN - height;
    }
    if( y < 0 )
	y = 0;

    wndPtr = WIN_FindWndPtr( hwndOwner );
    if (!wndPtr) return FALSE;

    if (!pTopPWnd)
    {
	pTopPWnd = WIN_FindWndPtr(CreateWindow32A( POPUPMENU_CLASS_ATOM, NULL,
                                          WS_POPUP | WS_BORDER, x, y,
                                          width, height,
                                          hwndOwner, 0, wndPtr->hInstance,
                                          (LPVOID)hmenu ));
	if (!pTopPWnd) return FALSE;
	skip_init = TRUE;
    }

    if( uSubPWndLevel )
    {
	/* create new window for the submenu */
	HWND32 hWnd = CreateWindow32A( POPUPMENU_CLASS_ATOM, NULL,
                                      WS_POPUP | WS_BORDER, x, y,
                                      width, height,
                                      menu->hWnd, 0, wndPtr->hInstance,
                                      (LPVOID)hmenu );
	if( !hWnd ) return FALSE;
	menu->hWnd = hWnd;
    }
    else 
    {
	if( !skip_init )
	  {
            MENU_SwitchTPWndTo(GetCurrentTask());
	    SendMessage16( pTopPWnd->hwndSelf, WM_USER, (WPARAM16)hmenu, 0L);
	  }
	menu->hWnd = pTopPWnd->hwndSelf;
    }

    uSubPWndLevel++;

    wndPtr = WIN_FindWndPtr( menu->hWnd );

    SetWindowPos32(menu->hWnd, 0, x, y, width, height,
		  		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
      /* Display the window */

    SetWindowPos32( menu->hWnd, HWND_TOP, 0, 0, 0, 0,
		    SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    UpdateWindow32( menu->hWnd );
    return TRUE;
}


/***********************************************************************
 *           MENU_SelectItem
 */
static void MENU_SelectItem( HWND32 hwndOwner, HMENU32 hmenu, UINT32 wIndex,
                             BOOL32 sendMenuSelect )
{
    LPPOPUPMENU lppop;
    HDC32 hdc;

    lppop = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!lppop->nItems) return;
    if ((wIndex != NO_SELECTED_ITEM) && 
	(wIndex != SYSMENU_SELECTED) &&
	(lppop->items[wIndex].item_flags & MF_SEPARATOR))
	wIndex = NO_SELECTED_ITEM;
    if (lppop->FocusedItem == wIndex) return;
    if (lppop->wFlags & MF_POPUP) hdc = GetDC32( lppop->hWnd );
    else hdc = GetDCEx32( lppop->hWnd, 0, DCX_CACHE | DCX_WINDOW);

      /* Clear previous highlighted item */
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	if (lppop->FocusedItem == SYSMENU_SELECTED)
	    NC_DrawSysButton( lppop->hWnd, hdc, FALSE );
	else
	{
	    lppop->items[lppop->FocusedItem].item_flags &=~(MF_HILITE|MF_MOUSESELECT);
	    MENU_DrawMenuItem(lppop->hWnd,hdc,&lppop->items[lppop->FocusedItem],
                              lppop->Height, !(lppop->wFlags & MF_POPUP) );
	}
    }

      /* Highlight new item (if any) */
    lppop->FocusedItem = wIndex;
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	if (lppop->FocusedItem == SYSMENU_SELECTED)
        {
	    NC_DrawSysButton( lppop->hWnd, hdc, TRUE );
            if (sendMenuSelect)
                SendMessage16( hwndOwner, WM_MENUSELECT,
                             WIN_FindWndPtr(lppop->hWnd)->hSysMenu,
                             MAKELONG(lppop->wFlags | MF_MOUSESELECT, hmenu));
        }
	else
	{
	    lppop->items[lppop->FocusedItem].item_flags |= MF_HILITE;
	    MENU_DrawMenuItem( lppop->hWnd, hdc, &lppop->items[lppop->FocusedItem],
                               lppop->Height, !(lppop->wFlags & MF_POPUP) );
            if (sendMenuSelect)
	        SendMessage16( hwndOwner, WM_MENUSELECT,
                             lppop->items[lppop->FocusedItem].item_id,
                             MAKELONG( lppop->items[lppop->FocusedItem].item_flags | MF_MOUSESELECT, hmenu));
	}
    }
    else if (sendMenuSelect)
        SendMessage16( hwndOwner, WM_MENUSELECT, hmenu,
                       MAKELONG( lppop->wFlags | MF_MOUSESELECT, hmenu ) );

    ReleaseDC32( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_SelectItemRel
 *
 */
static void MENU_SelectItemRel( HWND32 hwndOwner, HMENU32 hmenu, INT32 offset )
{
    INT32 i, min = 0;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu->items) return;
    if ((menu->FocusedItem != NO_SELECTED_ITEM) &&
	(menu->FocusedItem != SYSMENU_SELECTED))
    {
	for (i = menu->FocusedItem + offset ; i >= 0 && i < menu->nItems 
					    ; i += offset)
	{
	    if (!(menu->items[i].item_flags & MF_SEPARATOR))
	    {
		MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
		return;
	    }
	}

	if (MENU_HasSysMenu( menu ))
	{
	    MENU_SelectItem( hwndOwner, hmenu, SYSMENU_SELECTED, TRUE );
	    return;
	}
    }

    if( offset > 0 ) { i = 0; min = -1; }
    else i = menu->nItems - 1;
     
    for ( ; i > min && i < menu->nItems ; i += offset)
    {
	if (!(menu->items[i].item_flags & MF_SEPARATOR))
	{
	    MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
	    return;
	}
    }
    if (MENU_HasSysMenu( menu ))
        MENU_SelectItem( hwndOwner, hmenu, SYSMENU_SELECTED, TRUE );
}


/**********************************************************************
 *         MENU_SetItemData
 *
 * Set an item flags, id and text ptr.
 */
static BOOL32 MENU_SetItemData( MENUITEM *item, UINT32 flags, UINT32 id,
                                LPCSTR str )
{
    LPSTR prevText = IS_STRING_ITEM(item->item_flags) ? item->text : NULL;

    if (IS_STRING_ITEM(flags))
    {
        if (!str)
        {
            flags |= MF_SEPARATOR;
            item->text = NULL;
        }
        else
        {
            LPSTR text;
            /* Item beginning with a backspace is a help item */
            if (*str == '\b')
            {
                flags |= MF_HELP;
                str++;
            }
            if (!(text = HEAP_strdupA( SystemHeap, 0, str ))) return FALSE;
            item->text = text;
        }
    }
    else if (flags & MF_BITMAP) item->text = (LPSTR)(HBITMAP32)LOWORD(str);
    else if (flags & MF_OWNERDRAW) item->text = (LPSTR)str;
    else item->text = NULL;

    item->item_flags = flags & ~(MF_HILITE | MF_MOUSESELECT);
    item->item_id    = id;
    SetRectEmpty32( &item->rect );
    if (prevText) HeapFree( SystemHeap, 0, prevText );
    return TRUE;
}


/**********************************************************************
 *         MENU_InsertItem
 *
 * Insert a new item into a menu.
 */
static MENUITEM *MENU_InsertItem( HMENU32 hMenu, UINT32 pos, UINT32 flags )
{
    MENUITEM *newItems;
    POPUPMENU *menu;

    if (!(menu = (POPUPMENU *)USER_HEAP_LIN_ADDR(hMenu))) 
    {
        dprintf_menu( stddeb, "MENU_InsertItem: %04x not a menu handle\n",
                      hMenu );
        return NULL;
    }

    /* Find where to insert new item */

    if ((flags & MF_BYPOSITION) &&
        ((pos == (UINT32)-1) || (pos == menu->nItems)))
    {
        /* Special case: append to menu */
        /* Some programs specify the menu length to do that */
        pos = menu->nItems;
    }
    else
    {
        if (!MENU_FindItem( &hMenu, &pos, flags )) 
        {
            dprintf_menu( stddeb, "MENU_InsertItem: item %x not found\n",
                          pos );
            return NULL;
        }
        if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu)))
        {
            dprintf_menu(stddeb,"MENU_InsertItem: %04x not a menu handle\n",
                         hMenu);
            return NULL;
        }
    }

    /* Create new items array */

    newItems = HeapAlloc( SystemHeap, 0, sizeof(MENUITEM) * (menu->nItems+1) );
    if (!newItems)
    {
        dprintf_menu( stddeb, "MENU_InsertItem: allocation failed\n" );
        return NULL;
    }
    if (menu->nItems > 0)
    {
	  /* Copy the old array into the new */
	if (pos > 0) memcpy( newItems, menu->items, pos * sizeof(MENUITEM) );
	if (pos < menu->nItems) memcpy( &newItems[pos+1], &menu->items[pos],
					(menu->nItems-pos)*sizeof(MENUITEM) );
        HeapFree( SystemHeap, 0, menu->items );
    }
    menu->items = newItems;
    menu->nItems++;
    memset( &newItems[pos], 0, sizeof(*newItems) );
    return &newItems[pos];
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU32 hMenu, BOOL32 unicode )
{
    WORD flags, id = 0;
    LPCSTR str;

    do
    {
        flags = GET_WORD(res);
        res += sizeof(WORD);
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        if (!IS_STRING_ITEM(flags))
            fprintf( stderr, "MENU_ParseResource: not a string item %04x\n",
                     flags );
        str = res;
        if (!unicode) res += strlen(str) + 1;
        else res += (lstrlen32W((LPCWSTR)str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU32 hSubMenu = CreatePopupMenu32();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu, unicode )))
                return NULL;
            if (!unicode) AppendMenu32A( hMenu, flags, (UINT32)hSubMenu, str );
            else AppendMenu32W( hMenu, flags, (UINT32)hSubMenu, (LPCWSTR)str );
        }
        else  /* Not a popup */
        {
            if (!unicode) AppendMenu32A( hMenu, flags, id, *str ? str : NULL );
            else AppendMenu32W( hMenu, flags, id,
                                *(LPCWSTR)str ? (LPCWSTR)str : NULL );
        }
    } while (!(flags & MF_END));
    return res;
}


/***********************************************************************
 *           MENU_GetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static HMENU32 MENU_GetSubPopup( HMENU32 hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (menu->FocusedItem == NO_SELECTED_ITEM) return 0;
    else if (menu->FocusedItem == SYSMENU_SELECTED)
	return WIN_FindWndPtr(menu->hWnd)->hSysMenu;

    item = &menu->items[menu->FocusedItem];
    if (!(item->item_flags & MF_POPUP) || !(item->item_flags & MF_MOUSESELECT))
	return 0;
    return (HMENU32)item->item_id;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void MENU_HideSubPopups( HWND32 hwndOwner, HMENU32 hmenu,
                                BOOL32 sendMenuSelect )
{
    MENUITEM *item;
    POPUPMENU *menu, *submenu;
    HMENU32 hsubmenu;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return;
    if (menu->FocusedItem == SYSMENU_SELECTED)
    {
	hsubmenu = WIN_FindWndPtr(menu->hWnd)->hSysMenu;
    }
    else
    {
	item = &menu->items[menu->FocusedItem];
	if (!(item->item_flags & MF_POPUP) ||
	    !(item->item_flags & MF_MOUSESELECT)) return;
	item->item_flags &= ~MF_MOUSESELECT;
	hsubmenu = (HMENU32)item->item_id;
    }
    submenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hsubmenu );
    MENU_HideSubPopups( hwndOwner, hsubmenu, FALSE );
    MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, sendMenuSelect );
    if (submenu->hWnd == pTopPWnd->hwndSelf ) 
    {
	ShowWindow32( submenu->hWnd, SW_HIDE );
	uSubPWndLevel = 0;
    }
    else
    {
	DestroyWindow32( submenu->hWnd );
	submenu->hWnd = 0;
    }
}


/***********************************************************************
 *           MENU_ShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or hmenu if no submenu to display.
 */
static HMENU32 MENU_ShowSubPopup( HWND32 hwndOwner, HMENU32 hmenu,
                                  BOOL32 selectFirst )
{
    POPUPMENU *menu;
    MENUITEM *item;
    WND *wndPtr;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return hmenu;
    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return hmenu;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return hmenu;
    if (menu->FocusedItem == SYSMENU_SELECTED)
    {
	MENU_InitSysMenuPopup(wndPtr->hSysMenu, wndPtr->dwStyle,
				wndPtr->class->style);
	MENU_ShowPopup(hwndOwner, wndPtr->hSysMenu, 0, wndPtr->rectClient.left,
		wndPtr->rectClient.top - menu->Height - 2*SYSMETRICS_CYBORDER,
		SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE );
	if (selectFirst)
            MENU_SelectItemRel( hwndOwner, wndPtr->hSysMenu, ITEM_NEXT );
	return wndPtr->hSysMenu;
    }
    item = &menu->items[menu->FocusedItem];
    if (!(item->item_flags & MF_POPUP) ||
	(item->item_flags & (MF_GRAYED | MF_DISABLED))) return hmenu;
    item->item_flags |= MF_MOUSESELECT;
    if (menu->wFlags & MF_POPUP)
    {
	MENU_ShowPopup( hwndOwner, (HMENU16)item->item_id, menu->FocusedItem,
		 wndPtr->rectWindow.left + item->rect.right-arrow_bitmap_width,
		 wndPtr->rectWindow.top + item->rect.top,
		 item->rect.left - item->rect.right + 2*arrow_bitmap_width, 
		 item->rect.top - item->rect.bottom );
    }
    else
    {
	MENU_ShowPopup( hwndOwner, (HMENU16)item->item_id, menu->FocusedItem,
		        wndPtr->rectWindow.left + item->rect.left,
		        wndPtr->rectWindow.top + item->rect.bottom,
			item->rect.right - item->rect.left,
                        item->rect.bottom - item->rect.top );
    }
    if (selectFirst)
        MENU_SelectItemRel( hwndOwner, (HMENU32)item->item_id, ITEM_NEXT );
    return (HMENU32)item->item_id;
}


/***********************************************************************
 *           MENU_FindMenuByCoords
 *
 * Find the menu containing a given point (in screen coords).
 */
static HMENU32 MENU_FindMenuByCoords( HMENU32 hmenu, POINT32 pt )
{
    POPUPMENU *menu;
    HWND32 hwnd;

    if (!(hwnd = WindowFromPoint32( pt ))) return 0;
    while (hmenu)
    {
	menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
	if (menu->hWnd == hwnd)
	{
	    if (!(menu->wFlags & MF_POPUP))
	    {
		  /* Make sure it's in the menu bar (or in system menu) */
		WND *wndPtr = WIN_FindWndPtr( menu->hWnd );
		if ((pt.x < wndPtr->rectClient.left) ||
		    (pt.x >= wndPtr->rectClient.right) ||
		    (pt.y >= wndPtr->rectClient.top)) return 0;
		if (pt.y < wndPtr->rectClient.top - menu->Height)
		{
		    if (!MENU_IsInSysMenu( menu, pt )) return 0;
		}
		/* else it's in the menu bar */
	    }
	    return hmenu;
	}
	hmenu = MENU_GetSubPopup( hmenu );
    }
    return 0;
}


/***********************************************************************
 *           MENU_ExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ExecFocusedItem( HWND32 hwndOwner, HMENU32 hmenu,
                                    HMENU32 *hmenuCurrent )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu || !menu->nItems || (menu->FocusedItem == NO_SELECTED_ITEM) ||
	(menu->FocusedItem == SYSMENU_SELECTED)) return TRUE;
    item = &menu->items[menu->FocusedItem];
    if (!(item->item_flags & MF_POPUP))
    {
	if (!(item->item_flags & (MF_GRAYED | MF_DISABLED)))
	{
	    PostMessage16( hwndOwner, (menu->wFlags & MF_SYSMENU) ? 
                           WM_SYSCOMMAND : WM_COMMAND, item->item_id, 0 );
	    return FALSE;
	}
	else return TRUE;
    }
    else
    {
	*hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, TRUE );
	return TRUE;
    }
}


/***********************************************************************
 *           MENU_ButtonDown
 *
 * Handle a button-down event in a menu. Point is in screen coords.
 * hmenuCurrent is the top-most visible popup.
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ButtonDown( HWND32 hwndOwner, HMENU32 hmenu,
                               HMENU32 *hmenuCurrent, POINT32 pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT32 id;

    if (!hmenu) return FALSE;  /* Outside all menus */
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    item = MENU_FindItemByCoords( menu, pt.x, pt.y, &id );
    if (!item)  /* Maybe in system menu */
    {
	if (!MENU_IsInSysMenu( menu, pt )) return FALSE;
	id = SYSMENU_SELECTED;
    }	

    if (menu->FocusedItem == id)
    {
	if (id == SYSMENU_SELECTED) return FALSE;
	if (item->item_flags & MF_POPUP)
	{
	    if (item->item_flags & MF_MOUSESELECT)
	    {
		if (menu->wFlags & MF_POPUP)
		{
		    MENU_HideSubPopups( hwndOwner, hmenu, TRUE );
		    *hmenuCurrent = hmenu;
		}
		else return FALSE;
	    }
	    else *hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, FALSE );
	}
    }
    else
    {
	MENU_HideSubPopups( hwndOwner, hmenu, FALSE );
	MENU_SelectItem( hwndOwner, hmenu, id, TRUE );
	*hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_ButtonUp
 *
 * Handle a button-up event in a menu. Point is in screen coords.
 * hmenuCurrent is the top-most visible popup.
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ButtonUp( HWND32 hwndOwner, HMENU32 hmenu,
                             HMENU32 *hmenuCurrent, POINT32 pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    HMENU32 hsubmenu = 0;
    UINT32 id;

    if (!hmenu) return FALSE;  /* Outside all menus */
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    item = MENU_FindItemByCoords( menu, pt.x, pt.y, &id );
    if (!item)  /* Maybe in system menu */
    {
	if (!MENU_IsInSysMenu( menu, pt )) return FALSE;
	id = SYSMENU_SELECTED;
	hsubmenu = WIN_FindWndPtr(menu->hWnd)->hSysMenu;
    }	

    if (menu->FocusedItem != id) return FALSE;

    if (id != SYSMENU_SELECTED)
    {
	if (!(item->item_flags & MF_POPUP))
	{
	    return MENU_ExecFocusedItem( hwndOwner, hmenu, hmenuCurrent );
	}
	hsubmenu = (HMENU32)item->item_id;
    }
      /* Select first item of sub-popup */
    MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, FALSE );
    MENU_SelectItemRel( hwndOwner, hsubmenu, ITEM_NEXT );
    return TRUE;
}


/***********************************************************************
 *           MENU_MouseMove
 *
 * Handle a motion event in a menu. Point is in screen coords.
 * hmenuCurrent is the top-most visible popup.
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_MouseMove( HWND32 hwndOwner, HMENU32 hmenu,
                              HMENU32 *hmenuCurrent, POINT32 pt )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    UINT32 id = NO_SELECTED_ITEM;

    if (hmenu)
    {
	item = MENU_FindItemByCoords( menu, pt.x, pt.y, &id );
	if (!item)  /* Maybe in system menu */
	{
	    if (!MENU_IsInSysMenu( menu, pt ))
		id = NO_SELECTED_ITEM;  /* Outside all items */
	    else id = SYSMENU_SELECTED;
	}
    }	
    if (id == NO_SELECTED_ITEM)
    {
	MENU_SelectItem( hwndOwner, *hmenuCurrent, NO_SELECTED_ITEM, TRUE );
    }
    else if (menu->FocusedItem != id)
    {
	MENU_HideSubPopups( hwndOwner, hmenu, FALSE );
	MENU_SelectItem( hwndOwner, hmenu, id, TRUE );
	*hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_DoNextMenu
 */
static LRESULT MENU_DoNextMenu( HWND32* hwndOwner, HMENU32* hmenu,
                                HMENU32 *hmenuCurrent, UINT32 vk )
{
  POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( *hmenu );
  UINT32     id = 0;

  if(    (vk == VK_LEFT && !menu->FocusedItem)
      || (vk == VK_RIGHT && menu->FocusedItem == menu->nItems - 1) 
      || menu->FocusedItem == SYSMENU_SELECTED 
      || ((menu->wFlags & (MF_POPUP | MF_SYSMENU)) == (MF_POPUP | MF_SYSMENU)))
  {
    LRESULT l = SendMessage16( *hwndOwner, WM_NEXTMENU, (WPARAM16)vk, 
                               (LPARAM)((menu->FocusedItem == SYSMENU_SELECTED)
                                        ? GetSystemMenu32( *hwndOwner, 0) 
                                        : *hmenu));

    if( l == 0 || !IsMenu32(LOWORD(l)) || !IsWindow32(HIWORD(l)) ) return 0;

    /* shutdown current menu -
     * all these checks for system popup window are needed
     * only because Wine system menu tracking is unsuitable
     * for a lot of things (esp. when we do not have wIDmenu to fall back on). 
     */

    MENU_SelectItem( *hwndOwner, *hmenu, NO_SELECTED_ITEM, FALSE );

    if( (menu->wFlags & (MF_POPUP | MF_SYSMENU)) == (MF_POPUP | MF_SYSMENU) )
	{
	  ShowWindow32( menu->hWnd, SW_HIDE );
	  uSubPWndLevel = 0;

	  if( !IsIconic32( *hwndOwner ) )
	  { 
	    HDC32 hdc = GetDCEx32( *hwndOwner, 0, DCX_CACHE | DCX_WINDOW);
	    NC_DrawSysButton( *hwndOwner, hdc, FALSE );
	    ReleaseDC32( *hwndOwner, hdc );
	  }
	}

    ReleaseCapture(); 
   *hwndOwner = HIWORD(l);
   *hmenu = LOWORD(l);
    SetCapture32( *hwndOwner );

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( *hmenu );

    /* init next menu */

    if( (menu->wFlags & (MF_POPUP | MF_SYSMENU)) == (MF_POPUP | MF_SYSMENU) )
    {
        RECT32 rect;
        WND*   wndPtr = WIN_FindWndPtr( *hwndOwner );

        /* stupid kludge, see above */

        if( wndPtr->wIDmenu && !(wndPtr->dwStyle & WS_CHILD) )
        {
            *hmenu = wndPtr->wIDmenu;
            id = SYSMENU_SELECTED;
        }
        else
        { 
            if( NC_GetSysPopupPos( wndPtr, &rect ) )
                MENU_ShowPopup( *hwndOwner, *hmenu, 0, rect.left, rect.bottom,
                                SYSMETRICS_CXSIZE, SYSMETRICS_CYSIZE );
            
            if( !IsIconic32( *hwndOwner ) )
            {
                HDC32 hdc =  GetDCEx32( *hwndOwner, 0, DCX_CACHE | DCX_WINDOW);
                NC_DrawSysButton( *hwndOwner, hdc, TRUE );
                ReleaseDC32( *hwndOwner, hdc );
            }
        }
    }

    MENU_SelectItem( *hwndOwner, *hmenu, id, TRUE ); 
    return l;
  }
  return 0;
}

/***********************************************************************
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 * hmenuCurrent is the top-most visible popup.
 */
static void MENU_KeyLeft( HWND32* hwndOwner, HMENU32* hmenu,
                          HMENU32 *hmenuCurrent )
{
    POPUPMENU *menu;
    HMENU32 hmenutmp, hmenuprev;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( *hmenu );
    hmenuprev = hmenutmp = *hmenu;
    while (hmenutmp != *hmenuCurrent)
    {
	hmenutmp = MENU_GetSubPopup( hmenuprev );
	if (hmenutmp != *hmenuCurrent) hmenuprev = hmenutmp;
    }
    MENU_HideSubPopups( *hwndOwner, hmenuprev, TRUE );
    hmenutmp = *hmenu;

    if ( (hmenuprev == *hmenu) && 
         ((menu->wFlags & MF_SYSMENU) || !(menu->wFlags & MF_POPUP)) )
    {
	/* send WM_NEXTMENU */

	if( !MENU_DoNextMenu( hwndOwner, hmenu, hmenuCurrent, VK_LEFT) )
	     MENU_SelectItemRel( *hwndOwner, *hmenu, ITEM_PREV );
	else *hmenuCurrent = *hmenu;

	if (*hmenuCurrent != hmenutmp)
	{
	      /* A sublevel menu was displayed -> display the next one */
	    *hmenuCurrent = MENU_ShowSubPopup( *hwndOwner, *hmenu, TRUE );
	}
    }
    else *hmenuCurrent = hmenuprev;
}


/***********************************************************************
 *           MENU_KeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 * hmenuCurrent is the top-most visible popup.
 */
static void MENU_KeyRight( HWND32* hwndOwner, HMENU32* hmenu,
                           HMENU32 *hmenuCurrent )
{
    POPUPMENU *menu;
    HMENU32 hmenutmp;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( *hmenu );

    if ((menu->wFlags & MF_POPUP) || (*hmenuCurrent != *hmenu))
    {
	  /* If already displaying a popup, try to display sub-popup */
	hmenutmp = MENU_ShowSubPopup( *hwndOwner, *hmenuCurrent, TRUE );
	if (hmenutmp != *hmenuCurrent)  /* Sub-popup displayed */
	{
	    *hmenuCurrent = hmenutmp;
	    return;
	}
    }

      /* If menu-bar tracking, go to next item */

    if (!(menu->wFlags & MF_POPUP) || (menu->wFlags & MF_SYSMENU))
    {
	MENU_HideSubPopups( *hwndOwner, *hmenu, FALSE );
	hmenutmp = *hmenu;

        /* Send WM_NEXTMENU */

	if( !MENU_DoNextMenu( hwndOwner, hmenu, hmenuCurrent, VK_RIGHT) )
	     MENU_SelectItemRel( *hwndOwner, *hmenu, ITEM_NEXT );
	else *hmenuCurrent = *hmenu;

	if (*hmenuCurrent != hmenutmp)
	{
	      /* A sublevel menu was displayed -> display the next one */
	    *hmenuCurrent = MENU_ShowSubPopup( *hwndOwner, *hmenu, TRUE );
	}
    }
    else if (*hmenuCurrent != *hmenu)  /* Hide last level popup */
    {
	HMENU16 hmenuprev;
	hmenuprev = hmenutmp = *hmenu;
	while (hmenutmp != *hmenuCurrent)
	{
	    hmenutmp = MENU_GetSubPopup( hmenuprev );
	    if (hmenutmp != *hmenuCurrent) hmenuprev = hmenutmp;
	}
	MENU_HideSubPopups( *hwndOwner, hmenuprev, TRUE );
	*hmenuCurrent = hmenuprev;
    }
}


/***********************************************************************
 *           MENU_TrackMenu
 *
 * Menu tracking code.
 * If 'x' and 'y' are not 0, we simulate a button-down event at (x,y)
 * before beginning tracking. This is to help menu-bar tracking.
 */
static BOOL32 MENU_TrackMenu( HMENU32 hmenu, UINT32 wFlags, INT32 x, INT32 y,
                              HWND32 hwnd, const RECT32 *lprect )
{
    MSG16 msg;
    POPUPMENU *menu;
    HMENU32 hmenuCurrent = hmenu;
    BOOL32 fClosed = FALSE, fRemove;
    UINT32 pos;
    POINT32 pt;

    fEndMenuCalled = FALSE;
    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    if (x && y)
    {
	pt.x = x;
        pt.y = y;
	MENU_ButtonDown( hwnd, hmenu, &hmenuCurrent, pt );
    }

    EVENT_Capture( hwnd, HTMENU );

    while (!fClosed)
    {
	/* we have to keep the message in the queue until it's
	 * clear that menu loop is not over yet.
	 */

	if (!MSG_InternalGetMessage( &msg, 0, hwnd, MSGF_MENU, 
					      PM_NOREMOVE, TRUE ))
	    break;

        TranslateMessage16( &msg );
        CONV_POINT16TO32( &msg.pt, &pt );

        fRemove = FALSE;
	if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
	{
	      /* Find the sub-popup for this mouse event (if any) */

	    HMENU32 hsubmenu = MENU_FindMenuByCoords( hmenu, pt );

	    switch(msg.message)
	    {
		/* no WM_NC... messages in captured state */

	    case WM_RBUTTONDBLCLK:
	    case WM_RBUTTONDOWN:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */

	    case WM_LBUTTONDBLCLK:
	    case WM_LBUTTONDOWN:
		fClosed = !MENU_ButtonDown( hwnd, hsubmenu,
					    &hmenuCurrent, pt );
		break;
		
	    case WM_RBUTTONUP:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */

	    case WM_LBUTTONUP:
		  /* If outside all menus but inside lprect, ignore it */
		if (!hsubmenu && lprect && PtInRect32(lprect, pt)) break;
		fClosed = !MENU_ButtonUp( hwnd, hsubmenu,
					  &hmenuCurrent, pt );
                fRemove = TRUE;  /* Remove event even if outside menu */
		break;
		
	    case WM_MOUSEMOVE:
		if ((msg.wParam & MK_LBUTTON) ||
		    ((wFlags & TPM_RIGHTBUTTON) && (msg.wParam & MK_RBUTTON)))
		{
		    fClosed = !MENU_MouseMove( hwnd, hsubmenu,
					       &hmenuCurrent, pt );
		}
		break;
	    }
	}
	else if ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
	{
            fRemove = TRUE;  /* Keyboard messages are always removed */
	    switch(msg.message)
	    {
	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_HOME:
		case VK_END:
		    MENU_SelectItem( hwnd, hmenuCurrent, NO_SELECTED_ITEM, FALSE );

		/* fall through */
		case VK_UP:
		    MENU_SelectItemRel( hwnd, hmenuCurrent, 
				       (msg.wParam == VK_HOME)? ITEM_NEXT : ITEM_PREV );
		    break;

		case VK_DOWN: /* If on menu bar, pull-down the menu */

		    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
		    if (!(menu->wFlags & MF_POPUP) && (hmenuCurrent == hmenu))
			hmenuCurrent = MENU_ShowSubPopup( hwnd, hmenu, TRUE );
		    else
			MENU_SelectItemRel( hwnd, hmenuCurrent, ITEM_NEXT );
		    break;

		case VK_LEFT:
		    MENU_KeyLeft( &hwnd, &hmenu, &hmenuCurrent );
		    break;
		    
		case VK_RIGHT:
		    MENU_KeyRight( &hwnd, &hmenu, &hmenuCurrent );
		    break;
		    
		case VK_SPACE:
		case VK_RETURN:
		    fClosed = !MENU_ExecFocusedItem( hwnd, hmenuCurrent,
						     &hmenuCurrent );
		    break;

		case VK_ESCAPE:
		    fClosed = TRUE;
		    break;

		default:
		    break;
		}
		break;  /* WM_KEYDOWN */

	    case WM_SYSKEYDOWN:
		switch(msg.wParam)
		{
		case VK_MENU:
		    fClosed = TRUE;
		    break;
		    
		}
		break;  /* WM_SYSKEYDOWN */

	    case WM_CHAR:
		{
		      /* Hack to avoid control chars. */
		      /* We will find a better way real soon... */
		    if ((msg.wParam <= 32) || (msg.wParam >= 127)) break;
		    pos = MENU_FindItemByKey( hwnd, hmenuCurrent, msg.wParam );
		    if (pos == (UINT32)-2) fClosed = TRUE;
		    else if (pos == (UINT32)-1) MessageBeep32(0);
		    else
		    {
			MENU_SelectItem( hwnd, hmenuCurrent, pos, TRUE );
			fClosed = !MENU_ExecFocusedItem( hwnd, hmenuCurrent,
							 &hmenuCurrent );
			
		    }
		}		    
		break;  /* WM_CHAR */
	    }  /* switch(msg.message) */
	}
	else
	{
	    DispatchMessage16( &msg );
	}
	if (fEndMenuCalled) fClosed = TRUE;
	if (!fClosed) fRemove = TRUE;

        if (fRemove)  /* Remove the message from the queue */
	    PeekMessage16( &msg, 0, msg.message, msg.message, PM_REMOVE );
    }

    ReleaseCapture();
    MENU_HideSubPopups( hwnd, hmenu, FALSE );
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (menu && menu->wFlags & MF_POPUP) 
    {
         ShowWindow32( menu->hWnd, SW_HIDE );
	 uSubPWndLevel = 0;
    }
    MENU_SelectItem( hwnd, hmenu, NO_SELECTED_ITEM, FALSE );
    SendMessage16( hwnd, WM_MENUSELECT, 0, MAKELONG( 0xffff, 0 ) );
    fEndMenuCalled = FALSE;
    return TRUE;
}

/***********************************************************************
 *           MENU_TrackSysPopup
 */
static void MENU_TrackSysPopup( WND* pWnd )
{
    RECT32  rect;
    HMENU32 hMenu = pWnd->hSysMenu;
    HDC32   hDC = 0;

    /* track the system menu like a normal popup menu */

    if(IsMenu32(hMenu))
    {
	HWND32 hWnd = pWnd->hwndSelf;
	if (!(pWnd->dwStyle & WS_MINIMIZE))
	{
	    hDC = GetWindowDC32( hWnd );
	    NC_DrawSysButton( hWnd, hDC, TRUE );
	}
	NC_GetSysPopupPos( pWnd, &rect );
	MENU_InitSysMenuPopup( hMenu, pWnd->dwStyle,
				      pWnd->class->style);
	TrackPopupMenu32( hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
                          rect.left, rect.bottom, 0, hWnd, &rect );
	if (!(pWnd->dwStyle & WS_MINIMIZE))
	{
             NC_DrawSysButton( hWnd, hDC, FALSE );
             ReleaseDC32( hWnd, hDC );
	}
    }
}

/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( WND* wndPtr, INT32 ht, POINT32 pt )
{
    BOOL32  bTrackSys = ((ht == HTSYSMENU && !wndPtr->wIDmenu) ||
                         (wndPtr->dwStyle & (WS_MINIMIZE | WS_CHILD)));
    HWND32  hWnd = wndPtr->hwndSelf;
    HMENU32 hMenu = (bTrackSys) ? wndPtr->hSysMenu : wndPtr->wIDmenu;

    if (IsMenu32(hMenu))
    {
	HideCaret32(0);
	SendMessage16( hWnd, WM_ENTERMENULOOP, 0, 0 );
	SendMessage16( hWnd, WM_INITMENU, hMenu, 0 );
	if( bTrackSys )
	    MENU_TrackSysPopup( wndPtr );
	else
	    MENU_TrackMenu( hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
			    pt.x, pt.y, hWnd, NULL );
	SendMessage16( hWnd, WM_EXITMENULOOP, 0, 0 );
	ShowCaret32(0);
    }
}


/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackKbdMenuBar( WND* wndPtr, UINT32 wParam, INT32 vkey)
{
   INT32 htMenu;
   UINT32 uItem = NO_SELECTED_ITEM;
   HMENU32 hTrackMenu; 

    /* find window that has a menu */
 
    while( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwStyle & WS_SYSMENU) )
	if( !(wndPtr = wndPtr->parent) ) return;
          
    if( !wndPtr->wIDmenu && !(wndPtr->dwStyle & WS_SYSMENU) ) return;

    htMenu = ((wndPtr->dwStyle & (WS_CHILD | WS_MINIMIZE)) || 
	      !wndPtr->wIDmenu) ? HTSYSMENU : HTMENU;
    hTrackMenu = ( htMenu == HTSYSMENU ) ? wndPtr->hSysMenu : wndPtr->wIDmenu;

    if (IsMenu32( hTrackMenu ))
    {
        HideCaret32(0);
        SendMessage16( wndPtr->hwndSelf, WM_ENTERMENULOOP, 0, 0 );
        SendMessage16( wndPtr->hwndSelf, WM_INITMENU, hTrackMenu, 0 );

        /* find suitable menu entry */

        if( vkey == VK_SPACE )
            uItem = SYSMENU_SELECTED;
        else if( vkey )
        {
            uItem = ( htMenu == HTSYSMENU )
		    ? 0xFFFE	/* only VK_SPACE in this case */
		    : MENU_FindItemByKey( wndPtr->hwndSelf, wndPtr->wIDmenu, vkey );
	    if( uItem >= 0xFFFE )
	    {
	        if( uItem == 0xFFFF ) MessageBeep32(0);
		htMenu = 0;
	    }
        }

	switch( htMenu )
	{
	    case HTMENU:
 		MENU_SelectItem( wndPtr->hwndSelf, hTrackMenu, uItem, TRUE );
		if( uItem == NO_SELECTED_ITEM )
		    MENU_SelectItemRel( wndPtr->hwndSelf, hTrackMenu, ITEM_NEXT );
		else
		    PostMessage16( wndPtr->hwndSelf, WM_KEYDOWN, VK_DOWN, 0L );

		MENU_TrackMenu( hTrackMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
				0, 0, wndPtr->hwndSelf, NULL );
		break;

	    case HTSYSMENU:
		MENU_TrackSysPopup( wndPtr );
	}

        SendMessage16( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
        ShowCaret32(0);
    }
}


/**********************************************************************
 *           TrackPopupMenu16   (USER.416)
 */
BOOL16 TrackPopupMenu16( HMENU16 hMenu, UINT16 wFlags, INT16 x, INT16 y,
                         INT16 nReserved, HWND16 hWnd, const RECT16 *lpRect )
{
    RECT32 r;
    if (lpRect)
   	 CONV_RECT16TO32( lpRect, &r );
    return TrackPopupMenu32( hMenu, wFlags, x, y, nReserved, hWnd,
                             lpRect ? &r : NULL );
}


/**********************************************************************
 *           TrackPopupMenu32   (USER32.548)
 */
BOOL32 TrackPopupMenu32( HMENU32 hMenu, UINT32 wFlags, INT32 x, INT32 y,
                         INT32 nReserved, HWND32 hWnd, const RECT32 *lpRect )
{
    BOOL32 ret = FALSE;

    HideCaret32(0);
    if (MENU_ShowPopup( hWnd, hMenu, 0, x, y, 0, 0 )) 
	ret = MENU_TrackMenu( hMenu, wFlags, 0, 0, hWnd, lpRect );
    ShowCaret32(0);
    return ret;
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.549)
 */
BOOL32 TrackPopupMenuEx( HMENU32 hMenu, UINT32 wFlags, INT32 x, INT32 y,
			 HWND32 hWnd, LPTPMPARAMS lpTpm )
{
    fprintf( stderr, "TrackPopupMenuEx: not fully implemented\n" );
    return TrackPopupMenu32( hMenu, wFlags, x, y, 0, hWnd,
                             lpTpm ? &lpTpm->rcExclude : NULL );
}

/***********************************************************************
 *           PopupMenuWndProc
 */
LRESULT PopupMenuWndProc( HWND32 hwnd, UINT32 message, WPARAM32 wParam,
                          LPARAM lParam )
{    
    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCT32A *cs = (CREATESTRUCT32A*)lParam;
	    SetWindowLong32A( hwnd, 0, (LONG)cs->lpCreateParams );
	    return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT32 ps;
	    BeginPaint32( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc,
                                (HMENU32)GetWindowLong32A( hwnd, 0 ) );
	    EndPaint32( hwnd, &ps );
	    return 0;
	}

    case WM_DESTROY:
	    /* zero out global pointer in case system popup
	     * was destroyed by AppExit 
	     */

	    if( hwnd == pTopPWnd->hwndSelf )
	    {	pTopPWnd = NULL; uSubPWndLevel = 0; }
	    else
		uSubPWndLevel--;
	    break;

    case WM_USER:
	if (wParam) SetWindowLong32A( hwnd, 0, (HMENU16)wParam );
        break;
    default:
	return DefWindowProc32A( hwnd, message, wParam, lParam );
    }
    return 0;
}


/***********************************************************************
 *           MENU_GetMenuBarHeight
 *
 * Compute the size of the menu bar height. Used by NC_HandleNCCalcSize().
 */
UINT32 MENU_GetMenuBarHeight( HWND32 hwnd, UINT32 menubarWidth,
                              INT32 orgX, INT32 orgY )
{
    HDC32 hdc;
    RECT32 rectBar;
    WND *wndPtr;
    LPPOPUPMENU lppop;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(lppop = (LPPOPUPMENU)USER_HEAP_LIN_ADDR((HMENU16)wndPtr->wIDmenu)))
      return 0;
    hdc = GetDCEx32( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    SetRect32(&rectBar, orgX, orgY, orgX+menubarWidth, orgY+SYSMETRICS_CYMENU);
    MENU_MenuBarCalcSize( hdc, &rectBar, lppop, hwnd );
    ReleaseDC32( hwnd, hdc );
    return lppop->Height;
}


/*******************************************************************
 *         ChangeMenu16    (USER.153)
 */
BOOL16 ChangeMenu16( HMENU16 hMenu, UINT16 pos, SEGPTR data,
                     UINT16 id, UINT16 flags )
{
    dprintf_menu( stddeb,"ChangeMenu16: menu=%04x pos=%d data=%08lx id=%04x flags=%04x\n",
                  hMenu, pos, (DWORD)data, id, flags );
    if (flags & MF_APPEND) return AppendMenu16( hMenu, flags & ~MF_APPEND,
                                                id, data );
    /* FIXME: Word passes the item id in 'pos' and 0 or 0xffff as id */
    /* for MF_DELETE. We should check the parameters for all others */
    /* MF_* actions also (anybody got a doc on ChangeMenu?). */
    if (flags & MF_DELETE) return DeleteMenu16(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenu16(hMenu, pos, flags & ~MF_CHANGE,
                                               id, data );
    if (flags & MF_REMOVE) return RemoveMenu16(hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu16( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         ChangeMenu32A    (USER32.22)
 */
BOOL32 ChangeMenu32A( HMENU32 hMenu, UINT32 pos, LPCSTR data,
                      UINT32 id, UINT32 flags )
{
    dprintf_menu( stddeb,"ChangeMenu32A: menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
                  hMenu, pos, (DWORD)data, id, flags );
    if (flags & MF_APPEND) return AppendMenu32A( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu32(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenu32A(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu32( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu32A( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         ChangeMenu32W    (USER32.23)
 */
BOOL32 ChangeMenu32W( HMENU32 hMenu, UINT32 pos, LPCWSTR data,
                      UINT32 id, UINT32 flags )
{
    dprintf_menu( stddeb,"ChangeMenu32W: menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
                  hMenu, pos, (DWORD)data, id, flags );
    if (flags & MF_APPEND) return AppendMenu32W( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu32(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenu32W(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu32( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu32W( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         CheckMenuItem16    (USER.154)
 */
BOOL16 CheckMenuItem16( HMENU16 hMenu, UINT16 id, UINT16 flags )
{
    return (BOOL16)CheckMenuItem32( hMenu, id, flags );
}


/*******************************************************************
 *         CheckMenuItem32    (USER32.45)
 */
DWORD CheckMenuItem32( HMENU32 hMenu, UINT32 id, UINT32 flags )
{
    MENUITEM *item;
    DWORD ret;

    dprintf_menu( stddeb,"CheckMenuItem: %04x %04x %04x\n", hMenu, id, flags );
    if (!(item = MENU_FindItem( &hMenu, &id, flags ))) return -1;
    ret = item->item_flags & MF_CHECKED;
    if (flags & MF_CHECKED) item->item_flags |= MF_CHECKED;
    else item->item_flags &= ~MF_CHECKED;
    return ret;
}


/**********************************************************************
 *         EnableMenuItem16    (USER.155)
 */
BOOL16 EnableMenuItem16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return EnableMenuItem32( hMenu, wItemID, wFlags );
}


/**********************************************************************
 *         EnableMenuItem32    (USER32.169)
 */
BOOL32 EnableMenuItem32( HMENU32 hMenu, UINT32 wItemID, UINT32 wFlags )
{
    MENUITEM *item;
    dprintf_menu(stddeb,"EnableMenuItem (%04x, %04X, %04X) !\n", 
		 hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return FALSE;

      /* We can't have MF_GRAYED and MF_DISABLED together */
    if (wFlags & MF_GRAYED)
    {
	item->item_flags = (item->item_flags & ~MF_DISABLED) | MF_GRAYED;
    }
    else if (wFlags & MF_DISABLED)
    {
	item->item_flags = (item->item_flags & ~MF_GRAYED) | MF_DISABLED;
    }
    else   /* MF_ENABLED */
    {
	item->item_flags &= ~(MF_GRAYED | MF_DISABLED);
    }
    return TRUE;
}


/*******************************************************************
 *         GetMenuString16    (USER.161)
 */
INT16 GetMenuString16( HMENU16 hMenu, UINT16 wItemID,
                       LPSTR str, INT16 nMaxSiz, UINT16 wFlags )
{
    return GetMenuString32A( hMenu, wItemID, str, nMaxSiz, wFlags );
}


/*******************************************************************
 *         GetMenuString32A    (USER32.267)
 */
INT32 GetMenuString32A( HMENU32 hMenu, UINT32 wItemID,
                        LPSTR str, INT32 nMaxSiz, UINT32 wFlags )
{
    MENUITEM *item;

    dprintf_menu( stddeb, "GetMenuString32A: menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",
                 hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return 0;
    if (!IS_STRING_ITEM(item->item_flags)) return 0;
    lstrcpyn32A( str, item->text, nMaxSiz );
    dprintf_menu( stddeb, "GetMenuString32A: returning '%s'\n", str );
    return strlen(str);
}


/*******************************************************************
 *         GetMenuString32W    (USER32.268)
 */
INT32 GetMenuString32W( HMENU32 hMenu, UINT32 wItemID,
                        LPWSTR str, INT32 nMaxSiz, UINT32 wFlags )
{
    MENUITEM *item;

    dprintf_menu( stddeb, "GetMenuString32W: menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",
                 hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return 0;
    if (!IS_STRING_ITEM(item->item_flags)) return 0;
    lstrcpynAtoW( str, item->text, nMaxSiz );
    return lstrlen32W(str);
}


/**********************************************************************
 *         HiliteMenuItem16    (USER.162)
 */
BOOL16 HiliteMenuItem16( HWND16 hWnd, HMENU16 hMenu, UINT16 wItemID,
                         UINT16 wHilite )
{
    return HiliteMenuItem32( hWnd, hMenu, wItemID, wHilite );
}


/**********************************************************************
 *         HiliteMenuItem32    (USER32.317)
 */
BOOL32 HiliteMenuItem32( HWND32 hWnd, HMENU32 hMenu, UINT32 wItemID,
                         UINT32 wHilite )
{
    LPPOPUPMENU menu;
    dprintf_menu(stddeb,"HiliteMenuItem(%04x, %04x, %04x, %04x);\n", 
                 hWnd, hMenu, wItemID, wHilite);
    if (!MENU_FindItem( &hMenu, &wItemID, wHilite )) return FALSE;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return FALSE;
    if (menu->FocusedItem == wItemID) return TRUE;
    MENU_HideSubPopups( hWnd, hMenu, FALSE );
    MENU_SelectItem( hWnd, hMenu, wItemID, TRUE );
    return TRUE;
}


/**********************************************************************
 *         GetMenuState16    (USER.250)
 */
UINT16 GetMenuState16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return GetMenuState32( hMenu, wItemID, wFlags );
}


/**********************************************************************
 *         GetMenuState32    (USER32.266)
 */
UINT32 GetMenuState32( HMENU32 hMenu, UINT32 wItemID, UINT32 wFlags )
{
    MENUITEM *item;
    dprintf_menu(stddeb,"GetMenuState(%04x, %04x, %04x);\n", 
		 hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    if (item->item_flags & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( (HMENU16)item->item_id );
	if (!menu) return -1;
	else return (menu->nItems << 8) | (menu->wFlags & 0xff);
    }
    else return item->item_flags;
}


/**********************************************************************
 *         GetMenuItemCount16    (USER.263)
 */
INT16 GetMenuItemCount16( HMENU16 hMenu )
{
    LPPOPUPMENU	menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!menu || (menu->wMagic != MENU_MAGIC)) return -1;
    dprintf_menu( stddeb,"GetMenuItemCount16(%04x) returning %d\n", 
                  hMenu, menu->nItems );
    return menu->nItems;
}


/**********************************************************************
 *         GetMenuItemCount32    (USER32.261)
 */
INT32 GetMenuItemCount32( HMENU32 hMenu )
{
    LPPOPUPMENU	menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!menu || (menu->wMagic != MENU_MAGIC)) return -1;
    dprintf_menu( stddeb,"GetMenuItemCount32(%04x) returning %d\n", 
                  hMenu, menu->nItems );
    return menu->nItems;
}


/**********************************************************************
 *         GetMenuItemID16    (USER.264)
 */
UINT16 GetMenuItemID16( HMENU16 hMenu, INT16 nPos )
{
    LPPOPUPMENU	menu;

    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return -1;
    if ((nPos < 0) || (nPos >= menu->nItems)) return -1;
    if (menu->items[nPos].item_flags & MF_POPUP) return -1;
    return menu->items[nPos].item_id;
}


/**********************************************************************
 *         GetMenuItemID32    (USER32.262)
 */
UINT32 GetMenuItemID32( HMENU32 hMenu, INT32 nPos )
{
    LPPOPUPMENU	menu;

    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return -1;
    if ((nPos < 0) || (nPos >= menu->nItems)) return -1;
    if (menu->items[nPos].item_flags & MF_POPUP) return -1;
    return menu->items[nPos].item_id;
}


/*******************************************************************
 *         InsertMenu16    (USER.410)
 */
BOOL16 InsertMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                     UINT16 id, SEGPTR data )
{
    UINT32 pos32 = (UINT32)pos;
    if ((pos == (UINT16)-1) && (flags & MF_BYPOSITION)) pos32 = (UINT32)-1;
    if (IS_STRING_ITEM(flags) && data)
        return InsertMenu32A( hMenu, pos32, flags, id,
                              (LPSTR)PTR_SEG_TO_LIN(data) );
    return InsertMenu32A( hMenu, pos32, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         InsertMenu32A    (USER32.321)
 */
BOOL32 InsertMenu32A( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                      UINT32 id, LPCSTR str )
{
    MENUITEM *item;

    if (IS_STRING_ITEM(flags) && str)
        dprintf_menu( stddeb, "InsertMenu: %04x %d %04x %04x '%s'\n",
                      hMenu, pos, flags, id, str );
    else dprintf_menu( stddeb, "InsertMenu: %04x %d %04x %04x %08lx\n",
                       hMenu, pos, flags, id, (DWORD)str );

    if (!(item = MENU_InsertItem( hMenu, pos, flags ))) return FALSE;

    if (!(MENU_SetItemData( item, flags, id, str )))
    {
        RemoveMenu32( hMenu, pos, flags );
        return FALSE;
    }

    if (flags & MF_POPUP)  /* Set the MF_POPUP flag on the popup-menu */
	((POPUPMENU *)USER_HEAP_LIN_ADDR((HMENU16)id))->wFlags |= MF_POPUP;

    item->hCheckBit   = hStdCheck;
    item->hUnCheckBit = 0;
    return TRUE;
}


/*******************************************************************
 *         InsertMenu32W    (USER32.324)
 */
BOOL32 InsertMenu32W( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                      UINT32 id, LPCWSTR str )
{
    BOOL32 ret;

    if (IS_STRING_ITEM(flags) && str)
    {
        LPSTR newstr = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
        ret = InsertMenu32A( hMenu, pos, flags, id, newstr );
        HeapFree( GetProcessHeap(), 0, newstr );
        return ret;
    }
    else return InsertMenu32A( hMenu, pos, flags, id, (LPCSTR)str );
}


/*******************************************************************
 *         AppendMenu16    (USER.411)
 */
BOOL16 AppendMenu16( HMENU16 hMenu, UINT16 flags, UINT16 id, SEGPTR data )
{
    return InsertMenu16( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenu32A    (USER32.4)
 */
BOOL32 AppendMenu32A( HMENU32 hMenu, UINT32 flags, UINT32 id, LPCSTR data )
{
    return InsertMenu32A( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenu32W    (USER32.5)
 */
BOOL32 AppendMenu32W( HMENU32 hMenu, UINT32 flags, UINT32 id, LPCWSTR data )
{
    return InsertMenu32W( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/**********************************************************************
 *         RemoveMenu16    (USER.412)
 */
BOOL16 RemoveMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return RemoveMenu32( hMenu, nPos, wFlags );
}


/**********************************************************************
 *         RemoveMenu32    (USER32.440)
 */
BOOL32 RemoveMenu32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags )
{
    LPPOPUPMENU	menu;
    MENUITEM *item;

    dprintf_menu(stddeb,"RemoveMenu (%04x, %04x, %04x)\n",hMenu, nPos, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return FALSE;
    
      /* Remove item */

    if (IS_STRING_ITEM(item->item_flags) && item->text)
        HeapFree( SystemHeap, 0, item->text );
    if (--menu->nItems == 0)
    {
        HeapFree( SystemHeap, 0, menu->items );
        menu->items = NULL;
    }
    else
    {
	while(nPos < menu->nItems)
	{
	    *item = *(item+1);
	    item++;
	    nPos++;
	}
        menu->items = HeapReAlloc( SystemHeap, 0, menu->items,
                                   menu->nItems * sizeof(MENUITEM) );
    }
    return TRUE;
}


/**********************************************************************
 *         DeleteMenu16    (USER.413)
 */
BOOL16 DeleteMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return DeleteMenu32( hMenu, nPos, wFlags );
}


/**********************************************************************
 *         DeleteMenu32    (USER32.128)
 */
BOOL32 DeleteMenu32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags )
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) return FALSE;
    if (item->item_flags & MF_POPUP) DestroyMenu32( (HMENU32)item->item_id );
      /* nPos is now the position of the item */
    RemoveMenu32( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}


/*******************************************************************
 *         ModifyMenu16    (USER.414)
 */
BOOL16 ModifyMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                     UINT16 id, SEGPTR data )
{
    if (IS_STRING_ITEM(flags))
        return ModifyMenu32A( hMenu, pos, flags, id,
                              (LPSTR)PTR_SEG_TO_LIN(data) );
    return ModifyMenu32A( hMenu, pos, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         ModifyMenu32A    (USER32.396)
 */
BOOL32 ModifyMenu32A( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                      UINT32 id, LPCSTR str )
{
    MENUITEM *item;

    if (IS_STRING_ITEM(flags))
    {
	dprintf_menu( stddeb, "ModifyMenu: %04x %d %04x %04x '%s'\n",
                      hMenu, pos, flags, id, str ? str : "#NULL#" );
        if (!str) return FALSE;
    }
    else
    {
	dprintf_menu( stddeb, "ModifyMenu: %04x %d %04x %04x %08lx\n",
                      hMenu, pos, flags, id, (DWORD)str );
    }

    if (!(item = MENU_FindItem( &hMenu, &pos, flags ))) return FALSE;
    return MENU_SetItemData( item, flags, id, str );
}


/*******************************************************************
 *         ModifyMenu32W    (USER32.397)
 */
BOOL32 ModifyMenu32W( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                      UINT32 id, LPCWSTR str )
{
    BOOL32 ret;

    if (IS_STRING_ITEM(flags) && str)
    {
        LPSTR newstr = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
        ret = ModifyMenu32A( hMenu, pos, flags, id, newstr );
        HeapFree( GetProcessHeap(), 0, newstr );
        return ret;
    }
    else return ModifyMenu32A( hMenu, pos, flags, id, (LPCSTR)str );
}


/**********************************************************************
 *         CreatePopupMenu16    (USER.415)
 */
HMENU16 CreatePopupMenu16(void)
{
    return CreatePopupMenu32();
}


/**********************************************************************
 *         CreatePopupMenu32    (USER32.81)
 */
HMENU32 CreatePopupMenu32(void)
{
    HMENU32 hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu32())) return 0;
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    menu->wFlags |= MF_POPUP;
    return hmenu;
}


/**********************************************************************
 *         GetMenuCheckMarkDimensions    (USER.417) (USER32.257)
 */
DWORD GetMenuCheckMarkDimensions(void)
{
    return MAKELONG( check_bitmap_width, check_bitmap_height );
}


/**********************************************************************
 *         SetMenuItemBitmaps16    (USER.418)
 */
BOOL16 SetMenuItemBitmaps16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags,
                             HBITMAP16 hNewUnCheck, HBITMAP16 hNewCheck )
{
    return SetMenuItemBitmaps32( hMenu, nPos, wFlags, hNewUnCheck, hNewCheck );
}


/**********************************************************************
 *         SetMenuItemBitmaps32    (USER32.489)
 */
BOOL32 SetMenuItemBitmaps32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags,
                             HBITMAP32 hNewUnCheck, HBITMAP32 hNewCheck )
{
    MENUITEM *item;
    dprintf_menu(stddeb,"SetMenuItemBitmaps(%04x, %04x, %04x, %04x, %04x)\n",
                 hMenu, nPos, wFlags, hNewCheck, hNewUnCheck);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;

    if (!hNewCheck && !hNewUnCheck)
    {
	  /* If both are NULL, restore default bitmaps */
	item->hCheckBit   = hStdCheck;
	item->hUnCheckBit = 0;
	item->item_flags &= ~MF_USECHECKBITMAPS;
    }
    else  /* Install new bitmaps */
    {
	item->hCheckBit   = hNewCheck;
	item->hUnCheckBit = hNewUnCheck;
	item->item_flags |= MF_USECHECKBITMAPS;
    }
    return TRUE;
}


/**********************************************************************
 *         CreateMenu16    (USER.151)
 */
HMENU16 CreateMenu16(void)
{
    return CreateMenu32();
}


/**********************************************************************
 *         CreateMenu32    (USER32.80)
 */
HMENU32 CreateMenu32(void)
{
    HMENU32 hMenu;
    LPPOPUPMENU menu;
    if (!(hMenu = USER_HEAP_ALLOC( sizeof(POPUPMENU) ))) return 0;
    menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    menu->wFlags = 0;
    menu->wMagic = MENU_MAGIC;
    menu->hTaskQ = 0;
    menu->Width  = 0;
    menu->Height = 0;
    menu->nItems = 0;
    menu->hWnd   = 0;
    menu->items  = NULL;
    menu->FocusedItem = NO_SELECTED_ITEM;
    dprintf_menu( stddeb, "CreateMenu: return %04x\n", hMenu );
    return hMenu;
}


/**********************************************************************
 *         DestroyMenu16    (USER.152)
 */
BOOL16 DestroyMenu16( HMENU16 hMenu )
{
    return DestroyMenu32( hMenu );
}


/**********************************************************************
 *         DestroyMenu32    (USER32.133)
 */
BOOL32 DestroyMenu32( HMENU32 hMenu )
{
    LPPOPUPMENU lppop;
    dprintf_menu(stddeb,"DestroyMenu(%04x)\n", hMenu);

    if (hMenu == 0) return FALSE;
    /* Silently ignore attempts to destroy default system menu */
    if (hMenu == MENU_DefSysMenu) return TRUE;
    lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!lppop || (lppop->wMagic != MENU_MAGIC)) return FALSE;
    lppop->wMagic = 0;  /* Mark it as destroyed */
    if ((lppop->wFlags & MF_POPUP) &&
        lppop->hWnd &&
        (!pTopPWnd || (lppop->hWnd != pTopPWnd->hwndSelf)))
        DestroyWindow32( lppop->hWnd );

    if (lppop->items)
    {
        int i;
        MENUITEM *item = lppop->items;
        for (i = lppop->nItems; i > 0; i--, item++)
        {
            if (item->item_flags & MF_POPUP)
                DestroyMenu32( (HMENU32)item->item_id );
	    if (IS_STRING_ITEM(item->item_flags) && item->text)
                HeapFree( SystemHeap, 0, item->text );
        }
        HeapFree( SystemHeap, 0, lppop->items );
    }
    USER_HEAP_FREE( hMenu );
    return TRUE;
}


/**********************************************************************
 *         GetSystemMenu16    (USER.156)
 */
HMENU16 GetSystemMenu16( HWND16 hWnd, BOOL16 bRevert )
{
    return GetSystemMenu32( hWnd, bRevert );
}


/**********************************************************************
 *         GetSystemMenu32    (USER32.290)
 */
HMENU32 GetSystemMenu32( HWND32 hWnd, BOOL32 bRevert )
{
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    if (!wndPtr) return 0;

    if (!wndPtr->hSysMenu || (wndPtr->hSysMenu == MENU_DefSysMenu))
    {
        wndPtr->hSysMenu = MENU_CopySysMenu();
        return wndPtr->hSysMenu;
    }
    if (!bRevert) return wndPtr->hSysMenu;
    if (wndPtr->hSysMenu) DestroyMenu32(wndPtr->hSysMenu);
    wndPtr->hSysMenu = MENU_CopySysMenu();
    return wndPtr->hSysMenu;
}


/*******************************************************************
 *         SetSystemMenu16    (USER.280)
 */
BOOL16 SetSystemMenu16( HWND16 hwnd, HMENU16 hMenu )
{
    return SetSystemMenu32( hwnd, hMenu );
}


/*******************************************************************
 *         SetSystemMenu32    (USER32.507)
 */
BOOL32 SetSystemMenu32( HWND32 hwnd, HMENU32 hMenu )
{
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;
    if (wndPtr->hSysMenu && (wndPtr->hSysMenu != MENU_DefSysMenu))
        DestroyMenu32( wndPtr->hSysMenu );
    wndPtr->hSysMenu = hMenu;
    return TRUE;
}


/**********************************************************************
 *         GetMenu16    (USER.157)
 */
HMENU16 GetMenu16( HWND16 hWnd ) 
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr) return (HMENU16)wndPtr->wIDmenu;
    return 0;
}


/**********************************************************************
 *         GetMenu32    (USER32.256)
 */
HMENU32 GetMenu32( HWND32 hWnd ) 
{ 
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr) return (HMENU32)wndPtr->wIDmenu;
    return 0;
}


/**********************************************************************
 *         SetMenu16    (USER.158)
 */
BOOL16 SetMenu16( HWND16 hWnd, HMENU16 hMenu )
{
    return SetMenu32( hWnd, hMenu );
}


/**********************************************************************
 *         SetMenu32    (USER32.486)
 */
BOOL32 SetMenu32( HWND32 hWnd, HMENU32 hMenu )
{
    LPPOPUPMENU lpmenu;
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (!wndPtr) return FALSE;
    dprintf_menu(stddeb,"SetMenu(%04x, %04x);\n", hWnd, hMenu);
    if (GetCapture32() == hWnd) ReleaseCapture();
    wndPtr->wIDmenu = (UINT32)hMenu;
    if (hMenu != 0)
    {
        if (!(lpmenu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return FALSE;
        lpmenu->hWnd = hWnd;
        lpmenu->wFlags &= ~MF_POPUP;  /* Can't be a popup */
        lpmenu->Height = 0;  /* Make sure we recalculate the size */
    }
    if (IsWindowVisible32(hWnd))
        SetWindowPos32( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    return TRUE;
}



/**********************************************************************
 *         GetSubMenu16    (USER.159)
 */
HMENU16 GetSubMenu16( HMENU16 hMenu, INT16 nPos )
{
    return GetSubMenu32( hMenu, nPos );
}


/**********************************************************************
 *         GetSubMenu32    (USER32.287)
 */
HMENU32 GetSubMenu32( HMENU32 hMenu, INT32 nPos )
{
    LPPOPUPMENU lppop;

    if (!(lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return 0;
    if ((UINT32)nPos >= lppop->nItems) return 0;
    if (!(lppop->items[nPos].item_flags & MF_POPUP)) return 0;
    return (HMENU32)lppop->items[nPos].item_id;
}


/**********************************************************************
 *         DrawMenuBar16    (USER.160)
 */
void DrawMenuBar16( HWND16 hWnd )
{
    DrawMenuBar32( hWnd );
}


/**********************************************************************
 *         DrawMenuBar32    (USER32.160)
 */
BOOL32 DrawMenuBar32( HWND32 hWnd )
{
    LPPOPUPMENU lppop;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD) && wndPtr->wIDmenu)
    {
        lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR((HMENU16)wndPtr->wIDmenu);
        if (lppop == NULL) return FALSE;

        lppop->Height = 0; /* Make sure we call MENU_MenuBarCalcSize */
        SetWindowPos32( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                        SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           EndMenu   (USER.187) (USER32.174)
 */
void EndMenu(void)
{
    /* FIXME: this won't work when we have multiple tasks... */
    fEndMenuCalled = TRUE;
}


/***********************************************************************
 *           LookupMenuHandle   (USER.217)
 */
HMENU16 LookupMenuHandle( HMENU16 hmenu, INT16 id )
{
    HMENU32 hmenu32 = hmenu;
    INT32 id32 = id;
    if (!MENU_FindItem( &hmenu32, &id32, MF_BYCOMMAND )) return 0;
    else return hmenu32;
}


/**********************************************************************
 *	    LoadMenu16    (USER.150)
 */
HMENU16 LoadMenu16( HINSTANCE16 instance, SEGPTR name )
{
    HRSRC16 hRsrc;
    HGLOBAL16 handle;
    HMENU16 hMenu;

    if (HIWORD(name))
    {
        char *str = (char *)PTR_SEG_TO_LIN( name );
        dprintf_menu( stddeb, "LoadMenu(%04x,'%s')\n", instance, str );
        if (str[0] == '#') name = (SEGPTR)atoi( str + 1 );
    }
    else
        dprintf_resource(stddeb,"LoadMenu(%04x,%04x)\n",instance,LOWORD(name));

    if (!name) return 0;
    
    /* check for Win32 module */
    instance = GetExePtr( instance );
    if (MODULE_GetPtr(instance)->flags & NE_FFLAGS_WIN32)
        return LoadMenu32A(instance,PTR_SEG_TO_LIN(name));

    if (!(hRsrc = FindResource16( instance, name, RT_MENU ))) return 0;
    if (!(handle = LoadResource16( instance, hRsrc ))) return 0;
    hMenu = LoadMenuIndirect16(LockResource16(handle));
    FreeResource16( handle );
    return hMenu;
}


/*****************************************************************
 *        LoadMenu32A   (USER32.370)
 */
HMENU32 LoadMenu32A( HINSTANCE32 instance, LPCSTR name )
{
    HRSRC32 hrsrc = FindResource32A( instance, name, (LPSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirect32A( (LPCVOID)LoadResource32( instance, hrsrc ));
}


/*****************************************************************
 *        LoadMenu32W   (USER32.372)
 */
HMENU32 LoadMenu32W( HINSTANCE32 instance, LPCWSTR name )
{
    HRSRC32 hrsrc = FindResource32W( instance, name, (LPWSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirect32W( (LPCVOID)LoadResource32( instance, hrsrc ));
}


/**********************************************************************
 *	    LoadMenuIndirect16    (USER.220)
 */
HMENU16 LoadMenuIndirect16( LPCVOID template )
{
    HMENU16 hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    dprintf_menu(stddeb,"LoadMenuIndirect16: %p\n", template );
    version = GET_WORD(p);
    p += sizeof(WORD);
    if (version)
    {
        fprintf( stderr, "LoadMenuIndirect16: version must be 0 for Win16\n" );
        return 0;
    }
    offset = GET_WORD(p);
    p += sizeof(WORD) + offset;
    if (!(hMenu = CreateMenu32())) return 0;
    if (!MENU_ParseResource( p, hMenu, FALSE ))
    {
        DestroyMenu32( hMenu );
        return 0;
    }
    return hMenu;
}


/**********************************************************************
 *	    LoadMenuIndirect32A    (USER32.370)
 */
HMENU32 LoadMenuIndirect32A( LPCVOID template )
{
    HMENU16 hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    dprintf_menu(stddeb,"LoadMenuIndirect16: %p\n", template );
    version = GET_WORD(p);
    p += sizeof(WORD);
    if (version)
    {
        fprintf( stderr, "LoadMenuIndirect32A: version %d not supported.\n",
                 version );
        return 0;
    }
    offset = GET_WORD(p);
    p += sizeof(WORD) + offset;
    if (!(hMenu = CreateMenu32())) return 0;
    if (!MENU_ParseResource( p, hMenu, TRUE ))
    {
        DestroyMenu32( hMenu );
        return 0;
    }
    return hMenu;
}


/**********************************************************************
 *	    LoadMenuIndirect32W    (USER32.371)
 */
HMENU32 LoadMenuIndirect32W( LPCVOID template )
{
    /* FIXME: is there anything different between A and W? */
    return LoadMenuIndirect32A( template );
}


/**********************************************************************
 *		IsMenu16    (USER.358)
 */
BOOL16 IsMenu16( HMENU16 hmenu )
{
    LPPOPUPMENU menu;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    return (menu->wMagic == MENU_MAGIC);
}


/**********************************************************************
 *		IsMenu32    (USER32.345)
 */
BOOL32 IsMenu32( HMENU32 hmenu )
{
    LPPOPUPMENU menu;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    return (menu->wMagic == MENU_MAGIC);
}
