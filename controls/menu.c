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
#include "menu.h"
#include "user.h"
#include "win.h"
#include "library.h"
#include "message.h"
#include "graphics.h"
#include "stddebug.h"
/* #define DEBUG_MENU */
/* #define DEBUG_MENUCALC */
/* #define DEBUG_MENUSHORTCUT */
#include "debug.h"


  /* Dimension of the menu bitmaps */
static WORD check_bitmap_width = 0, check_bitmap_height = 0;
static WORD arrow_bitmap_width = 0, arrow_bitmap_height = 0;

  /* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL fEndMenuCalled = FALSE;

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

#define IS_STRING_ITEM(flags) (!((flags) & (MF_BITMAP | MF_OWNERDRAW | \
			     MF_MENUBARBREAK | MF_MENUBREAK | MF_SEPARATOR)))


extern void NC_DrawSysButton(HWND hwnd, HDC hdc, BOOL down);  /* nonclient.c */

static HBITMAP hStdCheck = 0;
static HBITMAP hStdMnArrow = 0;

HMENU CopySysMenu();
WORD * ParseMenuResource(WORD *first_item, int level, HMENU hMenu);


/***********************************************************************
 *           MENU_Init
 *
 * Menus initialisation.
 */
BOOL MENU_Init()
{
    BITMAP bm;

      /* Load bitmaps */

    if (!(hStdCheck = LoadBitmap( 0, MAKEINTRESOURCE(OBM_CHECK) )))
	return FALSE;
    GetObject( hStdCheck, sizeof(BITMAP), (LPSTR)&bm );
    check_bitmap_width = bm.bmWidth;
    check_bitmap_height = bm.bmHeight;
    if (!(hStdMnArrow = LoadBitmap( 0, MAKEINTRESOURCE(OBM_MNARROW) )))
	return FALSE;
    GetObject( hStdMnArrow, sizeof(BITMAP), (LPSTR)&bm );
    arrow_bitmap_width = bm.bmWidth;
    arrow_bitmap_height = bm.bmHeight;

    return TRUE;
}


/***********************************************************************
 *           MENU_HasSysMenu
 *
 * Check whether the window owning the menu bar has a system menu.
 */
static BOOL MENU_HasSysMenu( POPUPMENU *menu )
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
static BOOL MENU_IsInSysMenu( POPUPMENU *menu, POINT pt )
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
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
static MENUITEM *MENU_FindItem( HMENU *hmenu, WORD *nPos, WORD wFlags )
{
    POPUPMENU *menu;
    MENUITEM *item;
    int i;

    if (!(menu = (POPUPMENU *) USER_HEAP_ADDR(*hmenu))) return NULL;
    item = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->nItems) return NULL;
	return &item[*nPos];
    }
    else
    {
	for (i = 0; i < menu->nItems; i++, item++)
	{
	    if (item->item_id == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	    else if (item->item_flags & MF_POPUP)
	    {
		HMENU hsubmenu = (HMENU)item->item_id;
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
static MENUITEM *MENU_FindItemByCoords( POPUPMENU *menu, int x, int y, WORD *pos )
{
    MENUITEM *item;
    WND *wndPtr;
    int i;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return NULL;
    x -= wndPtr->rectWindow.left;
    y -= wndPtr->rectWindow.top;
    item = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
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
static WORD MENU_FindItemByKey( HWND hwndOwner, HMENU hmenu, WORD key )
{
    POPUPMENU *menu;
    LPMENUITEM lpitem;
    int i;
    LONG menuchar;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    lpitem = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    key = toupper(key);
    for (i = 0; i < menu->nItems; i++, lpitem++)
    {
	if (IS_STRING_ITEM(lpitem->item_flags))
	{
	    char *p = strchr( lpitem->item_text, '&' );
	    if (p && (p[1] != '&') && (toupper(p[1]) == key)) return i;
	}
    }
    menuchar = SendMessage( hwndOwner, WM_MENUCHAR, key,
			    MAKELONG( menu->wFlags, hmenu ) );
    if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
    if (HIWORD(menuchar) == 1) return -2;
    return -1;
}


/***********************************************************************
 *           MENU_CalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void MENU_CalcItemSize( HDC hdc, LPMENUITEM lpitem, HWND hwndOwner,
			       int orgX, int orgY, BOOL menuBar )
{
    DWORD dwSize;
    char *p;

    SetRect( &lpitem->rect, orgX, orgY, orgX, orgY );
    lpitem->xTab = 0;

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
	BITMAP bm;
	GetObject( (HBITMAP)lpitem->hText, sizeof(BITMAP), (LPSTR)&bm );
	lpitem->rect.right  += bm.bmWidth;
	lpitem->rect.bottom += bm.bmHeight;
	return;
    }
    
      /* If we get here, then it is a text item */

    dwSize = (lpitem->item_text == NULL) ? 0 : GetTextExtent( hdc, lpitem->item_text, strlen(lpitem->item_text));
    lpitem->rect.right  += LOWORD(dwSize);
    lpitem->rect.bottom += max( HIWORD(dwSize), SYSMETRICS_CYMENU );

    if (menuBar) lpitem->rect.right += MENU_BAR_ITEMS_SPACE;
    else if ((p = strchr( lpitem->item_text, '\t' )) != NULL)
    {
	  /* Item contains a tab (only meaningful in popup menus) */
	lpitem->xTab = check_bitmap_width + MENU_TAB_SPACE + 
	                 LOWORD( GetTextExtent( hdc, lpitem->item_text,
					       (int)(p - lpitem->item_text) ));
	lpitem->rect.right += MENU_TAB_SPACE;
    }
    else
    {
	if (strchr( lpitem->item_text, '\b' ))
	    lpitem->rect.right += MENU_TAB_SPACE;
	lpitem->xTab = lpitem->rect.right - check_bitmap_width 
	                - arrow_bitmap_width;
    }
}


/***********************************************************************
 *           MENU_PopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void MENU_PopupMenuCalcSize( LPPOPUPMENU lppop, HWND hwndOwner )
{
    LPMENUITEM  items, lpitem;
    HDC hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth;

    lppop->Width = lppop->Height = 0;
    if (lppop->nItems == 0) return;
    items = (MENUITEM *)USER_HEAP_ADDR( lppop->hItems );
    hdc = GetDC( 0 );
    maxX = start = 0;
    while (start < lppop->nItems)
    {
	lpitem = &items[start];
	orgX = maxX;
	orgY = 0;
	maxTab = maxTabWidth = 0;

	  /* Parse items until column break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((i != start) &&
		(lpitem->item_flags & (MF_MENUBREAK | MF_MENUBARBREAK))) break;
	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, FALSE );
	    maxX = max( maxX, lpitem->rect.right );
	    orgY = lpitem->rect.bottom;
	    if (lpitem->xTab)
	    {
		maxTab = max( maxTab, lpitem->xTab );
		maxTabWidth = max(maxTabWidth,lpitem->rect.right-lpitem->xTab);
	    }
	}

	  /* Finish the column (set all items to the largest width found) */
	maxX = max( maxX, maxTab + maxTabWidth );
	for (lpitem = &items[start]; start < i; start++, lpitem++)
	{
	    lpitem->rect.right = maxX;
	    if (lpitem->xTab) lpitem->xTab = maxTab;
	}
	lppop->Height = max( lppop->Height, orgY );
    }

    lppop->Width  = maxX;
    ReleaseDC( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * Calculate the size of the menu bar.
 */
static void MENU_MenuBarCalcSize( HDC hdc, LPRECT lprect, LPPOPUPMENU lppop,
				  HWND hwndOwner )
{
    LPMENUITEM lpitem, items;
    int start, i, orgX, orgY, maxY, helpPos;

    if ((lprect == NULL) || (lppop == NULL)) return;
    if (lppop->nItems == 0) return;
	dprintf_menucalc(stddeb,"MenuBarCalcSize left=%d top=%d right=%d bottom=%d !\n", 
		lprect->left, lprect->top, lprect->right, lprect->bottom);
    items = (MENUITEM *)USER_HEAP_ADDR( lppop->hItems );
    lppop->Width  = lprect->right - lprect->left;
    lppop->Height = 0;
    maxY = lprect->top;
    start = 0;
    helpPos = -1;
    while (start < lppop->nItems)
    {
	lpitem = &items[start];
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
	    maxY = max( maxY, lpitem->rect.bottom );
	    orgX = lpitem->rect.right;
	}

	  /* Finish the line (set all items to the largest height found) */
	while (start < i) items[start++].rect.bottom = maxY;
    }

    lprect->bottom = maxY;
    lppop->Height = lprect->bottom - lprect->top;

      /* Flush right all items between the MF_HELP and the last item */
      /* (if several lines, only move the last line) */
    if (helpPos != -1)
    {
	lpitem = &items[lppop->nItems-1];
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
static void MENU_DrawMenuItem( HDC hdc, LPMENUITEM lpitem,
			       WORD height, BOOL menuBar )
{
    RECT rect;

    if (menuBar && (lpitem->item_flags & MF_SEPARATOR)) return;
    rect = lpitem->rect;

      /* Draw the background */

    if (lpitem->item_flags & MF_HILITE)
	FillRect( hdc, &rect, sysColorObjects.hbrushHighlight );
    else FillRect( hdc, &rect, sysColorObjects.hbrushMenu );
    SetBkMode( hdc, TRANSPARENT );

      /* Draw the separator bar (if any) */

    if (!menuBar && (lpitem->item_flags & MF_MENUBARBREAK))
    {
	SelectObject( hdc, sysColorObjects.hpenWindowFrame );
	MoveTo( hdc, rect.left, 0 );
	LineTo( hdc, rect.left, height );
    }
    if (lpitem->item_flags & MF_SEPARATOR)
    {
	SelectObject( hdc, sysColorObjects.hpenWindowFrame );
	MoveTo( hdc, rect.left, rect.top + SEPARATOR_HEIGHT/2 );
	LineTo( hdc, rect.right, rect.top + SEPARATOR_HEIGHT/2 );
	return;
    }

      /* Setup colors */

    if (lpitem->item_flags & MF_HILITE)
    {
	if (lpitem->item_flags & MF_GRAYED)
	    SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
	else
	    SetTextColor( hdc, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
	SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
    }
    else
    {
	if (lpitem->item_flags & MF_GRAYED)
	    SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
	else
	    SetTextColor( hdc, GetSysColor( COLOR_MENUTEXT ) );
	SetBkColor( hdc, GetSysColor( COLOR_MENU ) );
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
	GRAPH_DrawBitmap( hdc, (HBITMAP)lpitem->hText, rect.left, rect.top,
                          0, 0, rect.right-rect.left, rect.bottom-rect.top );
	return;
    }
    /* No bitmap - process text if present */
    else if ((lpitem->item_text) != ((char *) NULL)) 
    {
	register int i;

	if (menuBar)
	{
	    rect.left += MENU_BAR_ITEMS_SPACE / 2;
	    rect.right -= MENU_BAR_ITEMS_SPACE / 2;
	    i = strlen( lpitem->item_text );
	}
	else
	{
	    for (i = 0; lpitem->item_text[i]; i++)
		if ((lpitem->item_text[i] == '\t') || 
		    (lpitem->item_text[i] == '\b')) break;
	}
	
	DrawText( hdc, lpitem->item_text, i, &rect,
		 DT_LEFT | DT_VCENTER | DT_SINGLELINE );

	if (lpitem->item_text[i])  /* There's a tab or flush-right char */
	{
	    if (lpitem->item_text[i] == '\t')
	    {
		rect.left = lpitem->xTab;
		DrawText( hdc, lpitem->item_text + i + 1, -1, &rect,
			  DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	    }
	    else DrawText( hdc, lpitem->item_text + i + 1, -1, &rect,
			   DT_RIGHT | DT_VCENTER | DT_SINGLELINE );
	}
    }
}


/***********************************************************************
 *           MENU_DrawPopupMenu
 *
 * Paint a popup menu.
 */
static void MENU_DrawPopupMenu( HWND hwnd, HDC hdc, HMENU hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;
    RECT rect;
    int i;

    GetClientRect( hwnd, &rect );
    FillRect( hdc, &rect, sysColorObjects.hbrushMenu );
    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (!menu || !menu->nItems) return;
    item = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    for (i = menu->nItems; i > 0; i--, item++)
	MENU_DrawMenuItem( hdc, item, menu->Height, FALSE );
}


/***********************************************************************
 *           MENU_DrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 */
WORD MENU_DrawMenuBar(HDC hDC, LPRECT lprect, HWND hwnd, BOOL suppress_draw)
{
    LPPOPUPMENU lppop;
    LPMENUITEM lpitem;
    int i;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    lppop = (LPPOPUPMENU) USER_HEAP_ADDR( wndPtr->wIDmenu );
    if (lppop == NULL || lprect == NULL) return SYSMETRICS_CYMENU;
    dprintf_menu(stddeb,"MENU_DrawMenuBar(%04X, %p, %p); !\n", 
		 hDC, lprect, lppop);
    if (lppop->Height == 0) MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);
    lprect->bottom = lprect->top + lppop->Height;
    if (suppress_draw) return lppop->Height;
    
    FillRect(hDC, lprect, sysColorObjects.hbrushMenu );
    SelectObject( hDC, sysColorObjects.hpenWindowFrame );
    MoveTo( hDC, lprect->left, lprect->bottom );
    LineTo( hDC, lprect->right, lprect->bottom );

    if (lppop->nItems == 0) return SYSMETRICS_CYMENU;
    lpitem = (MENUITEM *) USER_HEAP_ADDR( lppop->hItems );
    for (i = 0; i < lppop->nItems; i++, lpitem++)
    {
	MENU_DrawMenuItem( hDC, lpitem, lppop->Height, TRUE );
    }
    return lppop->Height;
} 


/***********************************************************************
 *           MENU_ShowPopup
 *
 * Display a popup menu.
 */
static BOOL MENU_ShowPopup(HWND hwndOwner, HMENU hmenu, WORD id, int x, int y)
{
    POPUPMENU *menu;

    if (!(menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	MENUITEM *item = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
	item[menu->FocusedItem].item_flags &= ~(MF_HILITE | MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }
    SendMessage( hwndOwner, WM_INITMENUPOPUP, hmenu,
		 MAKELONG( id, (menu->wFlags & MF_POPUP) ? 1 : 0 ));
    MENU_PopupMenuCalcSize( menu, hwndOwner );
    if (!menu->hWnd)
    {
	WND *wndPtr = WIN_FindWndPtr( hwndOwner );
	if (!wndPtr) return FALSE;
	menu->hWnd = CreateWindow( POPUPMENU_CLASS_NAME, "",
				   WS_POPUP | WS_BORDER, x, y, 
				   menu->Width + 2*SYSMETRICS_CXBORDER,
				   menu->Height + 2*SYSMETRICS_CYBORDER,
				   0, 0, wndPtr->hInstance,
				   (LPSTR)(DWORD)hmenu );
	if (!menu->hWnd) return FALSE;
    }
    else SetWindowPos( menu->hWnd, 0, x, y,
		       menu->Width + 2*SYSMETRICS_CXBORDER,
		       menu->Height + 2*SYSMETRICS_CYBORDER,
		       SWP_NOACTIVATE | SWP_NOZORDER );

      /* Display the window */

    SetWindowPos( menu->hWnd, HWND_TOP, 0, 0, 0, 0,
		  SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
    UpdateWindow( menu->hWnd );
    return TRUE;
}


/***********************************************************************
 *           MENU_SelectItem
 */
static void MENU_SelectItem( HMENU hmenu, WORD wIndex )
{
    MENUITEM *items;
    LPPOPUPMENU lppop;
    HDC hdc;

    lppop = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (!lppop->nItems) return;
    items = (MENUITEM *) USER_HEAP_ADDR( lppop->hItems );
    if ((wIndex != NO_SELECTED_ITEM) && 
	(wIndex != SYSMENU_SELECTED) &&
	(items[wIndex].item_flags & MF_SEPARATOR))
	wIndex = NO_SELECTED_ITEM;
    if (lppop->FocusedItem == wIndex) return;
    if (lppop->wFlags & MF_POPUP) hdc = GetDC( lppop->hWnd );
    else hdc = GetDCEx( lppop->hWnd, 0, DCX_CACHE | DCX_WINDOW);

      /* Clear previous highlighted item */
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	if (lppop->FocusedItem == SYSMENU_SELECTED)
	    NC_DrawSysButton( lppop->hWnd, hdc, FALSE );
	else
	{
	    items[lppop->FocusedItem].item_flags &=~(MF_HILITE|MF_MOUSESELECT);
	    MENU_DrawMenuItem( hdc, &items[lppop->FocusedItem], lppop->Height,
			       !(lppop->wFlags & MF_POPUP) );
	}
    }

      /* Highlight new item (if any) */
    lppop->FocusedItem = wIndex;
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	if (lppop->FocusedItem == SYSMENU_SELECTED)
	    NC_DrawSysButton( lppop->hWnd, hdc, TRUE );
	else
	{
	    items[lppop->FocusedItem].item_flags |= MF_HILITE;
	    MENU_DrawMenuItem( hdc, &items[lppop->FocusedItem], lppop->Height,
			       !(lppop->wFlags & MF_POPUP) );
	    SendMessage(lppop->hWnd, WM_MENUSELECT,
			items[lppop->FocusedItem].item_id, 
		       MAKELONG( hmenu, items[lppop->FocusedItem].item_flags));
	}
    }
    ReleaseDC( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_SelectNextItem
 */
static void MENU_SelectNextItem( HMENU hmenu )
{
    int i;
    MENUITEM *items;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (!menu->nItems) return;
    items = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    if ((menu->FocusedItem != NO_SELECTED_ITEM) &&
	(menu->FocusedItem != SYSMENU_SELECTED))
    {
	for (i = menu->FocusedItem+1; i < menu->nItems; i++)
	{
	    if (!(items[i].item_flags & MF_SEPARATOR))
	    {
		MENU_SelectItem( hmenu, i );
		return;
	    }
	}
	if (MENU_HasSysMenu( menu ))
	{
	    MENU_SelectItem( hmenu, SYSMENU_SELECTED );
	    return;
	}
    }
    for (i = 0; i < menu->nItems; i++)
    {
	if (!(items[i].item_flags & MF_SEPARATOR))
	{
	    MENU_SelectItem( hmenu, i );
	    return;
	}
    }
    if (MENU_HasSysMenu( menu )) MENU_SelectItem( hmenu, SYSMENU_SELECTED );
}


/***********************************************************************
 *           MENU_SelectPrevItem
 */
static void MENU_SelectPrevItem( HMENU hmenu )
{
    int i;
    MENUITEM *items;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (!menu->nItems) return;
    items = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    if ((menu->FocusedItem != NO_SELECTED_ITEM) &&
	(menu->FocusedItem != SYSMENU_SELECTED))
    {
	for (i = menu->FocusedItem - 1; i >= 0; i--)
	{
	    if (!(items[i].item_flags & MF_SEPARATOR))
	    {
		MENU_SelectItem( hmenu, i );
		return;
	    }
	}
	if (MENU_HasSysMenu( menu ))
	{
	    MENU_SelectItem( hmenu, SYSMENU_SELECTED );
	    return;
	}
    }
    for (i = menu->nItems - 1; i > 0; i--)
    {
	if (!(items[i].item_flags & MF_SEPARATOR))
	{
	    MENU_SelectItem( hmenu, i );
	    return;
	}
    }
    if (MENU_HasSysMenu( menu )) MENU_SelectItem( hmenu, SYSMENU_SELECTED );
}


/***********************************************************************
 *           MENU_GetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static HMENU MENU_GetSubPopup( HMENU hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (menu->FocusedItem == NO_SELECTED_ITEM) return 0;
    else if (menu->FocusedItem == SYSMENU_SELECTED)
	return GetSystemMenu( menu->hWnd, FALSE );

    item = ((MENUITEM *)USER_HEAP_ADDR( menu->hItems )) + menu->FocusedItem;
    if (!(item->item_flags & MF_POPUP) || !(item->item_flags & MF_MOUSESELECT))
	return 0;
    return item->item_id;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void MENU_HideSubPopups( HMENU hmenu )
{
    MENUITEM *item;
    POPUPMENU *menu, *submenu;
    HMENU hsubmenu;

    if (!(menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu ))) return;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return;
    if (menu->FocusedItem == SYSMENU_SELECTED)
    {
	hsubmenu = GetSystemMenu( menu->hWnd, FALSE );
    }
    else
    {
	item = ((MENUITEM *)USER_HEAP_ADDR(menu->hItems)) + menu->FocusedItem;
	if (!(item->item_flags & MF_POPUP) ||
	    !(item->item_flags & MF_MOUSESELECT)) return;
	item->item_flags &= ~MF_MOUSESELECT;
	hsubmenu = item->item_id;
    }
    submenu = (POPUPMENU *) USER_HEAP_ADDR( hsubmenu );
    MENU_HideSubPopups( hsubmenu );
    if (submenu->hWnd) ShowWindow( submenu->hWnd, SW_HIDE );
    MENU_SelectItem( hsubmenu, NO_SELECTED_ITEM );
}


/***********************************************************************
 *           MENU_ShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or hmenu if no submenu to display.
 */
static HMENU MENU_ShowSubPopup( HWND hwndOwner, HMENU hmenu, BOOL selectFirst )
{
    POPUPMENU *menu;
    MENUITEM *item;
    WND *wndPtr;

    if (!(menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu ))) return hmenu;
    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return hmenu;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return hmenu;
    if (menu->FocusedItem == SYSMENU_SELECTED)
    {
	MENU_ShowPopup(hwndOwner, wndPtr->hSysMenu, 0, wndPtr->rectClient.left,
		wndPtr->rectClient.top - menu->Height - 2*SYSMETRICS_CYBORDER);
	if (selectFirst) MENU_SelectNextItem( wndPtr->hSysMenu );
	return wndPtr->hSysMenu;
    }
    item = ((MENUITEM *)USER_HEAP_ADDR( menu->hItems )) + menu->FocusedItem;
    if (!(item->item_flags & MF_POPUP) ||
	(item->item_flags & (MF_GRAYED | MF_DISABLED))) return hmenu;
    item->item_flags |= MF_MOUSESELECT;
    if (menu->wFlags & MF_POPUP)
    {
	MENU_ShowPopup( hwndOwner, (HMENU)item->item_id, menu->FocusedItem,
		 wndPtr->rectWindow.left + item->rect.right-arrow_bitmap_width,
		 wndPtr->rectWindow.top + item->rect.top );
    }
    else
    {
	MENU_ShowPopup( hwndOwner, (HMENU)item->item_id, menu->FocusedItem,
		        wndPtr->rectWindow.left + item->rect.left,
		        wndPtr->rectWindow.top + item->rect.bottom );
    }
    if (selectFirst) MENU_SelectNextItem( (HMENU)item->item_id );
    return (HMENU)item->item_id;
}


/***********************************************************************
 *           MENU_FindMenuByCoords
 *
 * Find the menu containing a given point (in screen coords).
 */
static HMENU MENU_FindMenuByCoords( HMENU hmenu, POINT pt )
{
    POPUPMENU *menu;
    HWND hwnd;

    if (!(hwnd = WindowFromPoint( pt ))) return 0;
    while (hmenu)
    {
	menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
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
static BOOL MENU_ExecFocusedItem( HWND hwndOwner, HMENU hmenu,
				  HMENU *hmenuCurrent )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    if (!menu || !menu->nItems || (menu->FocusedItem == NO_SELECTED_ITEM) ||
	(menu->FocusedItem == SYSMENU_SELECTED)) return TRUE;
    item = ((MENUITEM *)USER_HEAP_ADDR( menu->hItems )) + menu->FocusedItem;
    if (!(item->item_flags & MF_POPUP))
    {
	if (!(item->item_flags & (MF_GRAYED | MF_DISABLED)))
	{
	    PostMessage( hwndOwner, (menu->wFlags & MF_SYSMENU) ? 
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
static BOOL MENU_ButtonDown( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent,
			     POINT pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    WORD id;

    if (!hmenu) return FALSE;  /* Outside all menus */
    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
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
		    MENU_HideSubPopups( hmenu );
		    *hmenuCurrent = hmenu;
		}
		else return FALSE;
	    }
	    else *hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, FALSE );
	}
    }
    else
    {
	MENU_HideSubPopups( hmenu );
	MENU_SelectItem( hmenu, id );
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
static BOOL MENU_ButtonUp( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent,
			   POINT pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    HMENU hsubmenu = 0;
    WORD id;

    if (!hmenu) return FALSE;  /* Outside all menus */
    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    item = MENU_FindItemByCoords( menu, pt.x, pt.y, &id );
    if (!item)  /* Maybe in system menu */
    {
	if (!MENU_IsInSysMenu( menu, pt )) return FALSE;
	id = SYSMENU_SELECTED;
	hsubmenu = GetSystemMenu( menu->hWnd, FALSE );
    }	

    if (menu->FocusedItem != id) return FALSE;

    if (id != SYSMENU_SELECTED)
    {
	if (!(item->item_flags & MF_POPUP))
	{
	    return MENU_ExecFocusedItem( hwndOwner, hmenu, hmenuCurrent );
	}
	hsubmenu = item->item_id;
    }
      /* Select first item of sub-popup */
    MENU_SelectItem( hsubmenu, NO_SELECTED_ITEM );
    MENU_SelectNextItem( hsubmenu );
    return TRUE;
}


/***********************************************************************
 *           MENU_MouseMove
 *
 * Handle a motion event in a menu. Point is in screen coords.
 * hmenuCurrent is the top-most visible popup.
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL MENU_MouseMove( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent,
			    POINT pt )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    WORD id = NO_SELECTED_ITEM;

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
	MENU_SelectItem( *hmenuCurrent, NO_SELECTED_ITEM );
    }
    else if (menu->FocusedItem != id)
    {
	MENU_HideSubPopups( hmenu );
	MENU_SelectItem( hmenu, id );
	*hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 * hmenuCurrent is the top-most visible popup.
 */
static void MENU_KeyLeft( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent )
{
    POPUPMENU *menu;
    HMENU hmenutmp, hmenuprev;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    hmenuprev = hmenutmp = hmenu;
    while (hmenutmp != *hmenuCurrent)
    {
	hmenutmp = MENU_GetSubPopup( hmenuprev );
	if (hmenutmp != *hmenuCurrent) hmenuprev = hmenutmp;
    }
    MENU_HideSubPopups( hmenuprev );

    if ((hmenuprev == hmenu) && !(menu->wFlags & MF_POPUP))
    {
	  /* Select previous item on the menu bar */
	MENU_SelectPrevItem( hmenu );
	if (*hmenuCurrent != hmenu)
	{
	      /* A popup menu was displayed -> display the next one */
	    *hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, TRUE );
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
static void MENU_KeyRight( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent )
{
    POPUPMENU *menu;
    HMENU hmenutmp;

    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );

    if ((menu->wFlags & MF_POPUP) || (*hmenuCurrent != hmenu))
    {
	  /* If already displaying a popup, try to display sub-popup */
	hmenutmp = MENU_ShowSubPopup( hwndOwner, *hmenuCurrent, TRUE );
	if (hmenutmp != *hmenuCurrent)  /* Sub-popup displayed */
	{
	    *hmenuCurrent = hmenutmp;
	    return;
	}
    }

      /* If on menu-bar, go to next item */
    if (!(menu->wFlags & MF_POPUP))
    {
	MENU_HideSubPopups( hmenu );
	MENU_SelectNextItem( hmenu );
	if (*hmenuCurrent != hmenu)
	{
	      /* A popup menu was displayed -> display the next one */
	    *hmenuCurrent = MENU_ShowSubPopup( hwndOwner, hmenu, TRUE );
	}
    }
    else if (*hmenuCurrent != hmenu)  /* Hide last level popup */
    {
	HMENU hmenuprev;
	hmenuprev = hmenutmp = hmenu;
	while (hmenutmp != *hmenuCurrent)
	{
	    hmenutmp = MENU_GetSubPopup( hmenuprev );
	    if (hmenutmp != *hmenuCurrent) hmenuprev = hmenutmp;
	}
	MENU_HideSubPopups( hmenuprev );
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
static BOOL MENU_TrackMenu( HMENU hmenu, WORD wFlags, int x, int y,
			    HWND hwnd, LPRECT lprect )
{
    MSG msg;
    POPUPMENU *menu;
    HMENU hmenuCurrent = hmenu;
    BOOL fClosed = FALSE;
    WORD pos;

    fEndMenuCalled = FALSE;
    if (!(menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu ))) return FALSE;
    if (x && y)
    {
	POINT pt = { x, y };
	MENU_ButtonDown( hwnd, hmenu, &hmenuCurrent, pt );
    }
    SetCapture( hwnd );

    while (!fClosed)
    {
	if (!MSG_InternalGetMessage( &msg, 0, hwnd, MSGF_MENU, 0, TRUE ))
	    break;

	if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
	{
	      /* Find the sub-popup for this mouse event (if any) */
	    HMENU hsubmenu = MENU_FindMenuByCoords( hmenu, msg.pt );

	    switch(msg.message)
	    {
	    case WM_RBUTTONDOWN:
	    case WM_NCRBUTTONDOWN:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */
	    case WM_LBUTTONDOWN:
	    case WM_NCLBUTTONDOWN:
		fClosed = !MENU_ButtonDown( hwnd, hsubmenu,
					    &hmenuCurrent, msg.pt );
		break;
		
	    case WM_RBUTTONUP:
	    case WM_NCRBUTTONUP:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */
	    case WM_LBUTTONUP:
	    case WM_NCLBUTTONUP:
		  /* If outside all menus but inside lprect, ignore it */
		if (!hsubmenu && lprect && PtInRect( lprect, msg.pt )) break;
		fClosed = !MENU_ButtonUp( hwnd, hsubmenu,
					  &hmenuCurrent, msg.pt );
		break;
		
	    case WM_MOUSEMOVE:
	    case WM_NCMOUSEMOVE:
		if ((msg.wParam & MK_LBUTTON) ||
		    ((wFlags & TPM_RIGHTBUTTON) && (msg.wParam & MK_RBUTTON)))
		{
		    fClosed = !MENU_MouseMove( hwnd, hsubmenu,
					       &hmenuCurrent, msg.pt );
		}
		break;
	    }
	}
	else if ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
	{
	    switch(msg.message)
	    {
	    case WM_KEYDOWN:
		switch(msg.wParam)
		{
		case VK_HOME:
		    MENU_SelectItem( hmenuCurrent, NO_SELECTED_ITEM );
		    MENU_SelectNextItem( hmenuCurrent );
		    break;

		case VK_END:
		    MENU_SelectItem( hmenuCurrent, NO_SELECTED_ITEM );
		    MENU_SelectPrevItem( hmenuCurrent );
		    break;

		case VK_UP:
		    MENU_SelectPrevItem( hmenuCurrent );
		    break;

		case VK_DOWN:
		      /* If on menu bar, pull-down the menu */
		    if (!(menu->wFlags & MF_POPUP) && (hmenuCurrent == hmenu))
			hmenuCurrent = MENU_ShowSubPopup( hwnd, hmenu, TRUE );
		    else
			MENU_SelectNextItem( hmenuCurrent );
		    break;

		case VK_LEFT:
		    MENU_KeyLeft( hwnd, hmenu, &hmenuCurrent );
		    break;
		    
		case VK_RIGHT:
		    MENU_KeyRight( hwnd, hmenu, &hmenuCurrent );
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
		    if (pos == (WORD)-2) fClosed = TRUE;
		    else if (pos == (WORD)-1) MessageBeep(0);
		    else
		    {
			MENU_SelectItem( hmenuCurrent, pos );
			fClosed = !MENU_ExecFocusedItem( hwnd, hmenuCurrent,
							 &hmenuCurrent );
			
		    }
		}		    
		break;  /* WM_CHAR */
	    }  /* switch(msg.message) */
	}
	else
	{
	    DispatchMessage( &msg );
	}
	if (fEndMenuCalled) fClosed = TRUE;

	if (!fClosed)  /* Remove the message from the queue */
	    PeekMessage( &msg, 0, 0, 0, PM_REMOVE );
    }
    ReleaseCapture();
    MENU_HideSubPopups( hmenu );
    if (menu->wFlags & MF_POPUP) ShowWindow( menu->hWnd, SW_HIDE );
    MENU_SelectItem( hmenu, NO_SELECTED_ITEM );
    fEndMenuCalled = FALSE;
    return TRUE;
}


/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( HWND hwnd, POINT pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SendMessage( hwnd, WM_ENTERMENULOOP, 0, 0 );
    MENU_TrackMenu( (HMENU)wndPtr->wIDmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
		    pt.x, pt.y, hwnd, NULL );
    SendMessage( hwnd, WM_EXITMENULOOP, 0, 0 );
}


/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackKbdMenuBar( HWND hwnd, WORD wParam )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    SendMessage( hwnd, WM_ENTERMENULOOP, 0, 0 );
      /* Select first selectable item */
    MENU_SelectItem( wndPtr->wIDmenu, NO_SELECTED_ITEM );
    MENU_SelectNextItem( (HMENU)wndPtr->wIDmenu );
    MENU_TrackMenu( (HMENU)wndPtr->wIDmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
		    0, 0, hwnd, NULL );
    SendMessage( hwnd, WM_EXITMENULOOP, 0, 0 );
}


/**********************************************************************
 *			TrackPopupMenu		[USER.416]
 */
BOOL TrackPopupMenu( HMENU hMenu, WORD wFlags, short x, short y,
		     short nReserved, HWND hWnd, LPRECT lpRect )
{
    if (!MENU_ShowPopup( hWnd, hMenu, 0, x, y )) return FALSE;
    return MENU_TrackMenu( hMenu, wFlags, 0, 0, hWnd, lpRect );
}


/***********************************************************************
 *           PopupMenuWndProc
 */
LONG PopupMenuWndProc( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{    
    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCT *createStruct = (CREATESTRUCT *)lParam;
	    HMENU hmenu = (HMENU) ((int)createStruct->lpCreateParams & 0xffff);
	    SetWindowWord( hwnd, 0, hmenu );
	    return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    BeginPaint( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc,
			        (HMENU)GetWindowWord( hwnd, 0 ) );
	    EndPaint( hwnd, &ps );
	    return 0;
	}

    default:
	return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 *           MENU_GetMenuBarHeight
 *
 * Compute the size of the menu bar height. Used by NC_HandleNCCalcSize().
 */
WORD MENU_GetMenuBarHeight( HWND hwnd, WORD menubarWidth, int orgX, int orgY )
{
    HDC hdc;
    RECT rectBar;
    WND *wndPtr;
    LPPOPUPMENU lppop;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(lppop = (LPPOPUPMENU)USER_HEAP_ADDR( wndPtr->wIDmenu ))) return 0;
    hdc = GetDC( hwnd );
    SetRect( &rectBar, orgX, orgY, orgX+menubarWidth, orgY+SYSMETRICS_CYMENU );
    MENU_MenuBarCalcSize( hdc, &rectBar, lppop, hwnd );
    ReleaseDC( hwnd, hdc );
    return lppop->Height;
}


/**********************************************************************
 *			ChangeMenu		[USER.153]
 */
BOOL ChangeMenu(HMENU hMenu, WORD nPos, LPSTR lpNewItem, 
			WORD wItemID, WORD wFlags)
{
	if (wFlags & MF_APPEND)
		return AppendMenu(hMenu, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_DELETE)
		return DeleteMenu(hMenu, wItemID, wFlags);
	if (wFlags & MF_INSERT) 
		return InsertMenu(hMenu, nPos, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_CHANGE) 
		return ModifyMenu(hMenu, nPos, wFlags, wItemID, lpNewItem);
	if (wFlags & MF_REMOVE) 
		return RemoveMenu(hMenu, wItemID, wFlags);
	return FALSE;
}


/**********************************************************************
 *			CheckMenuItem		[USER.154]
 */
BOOL CheckMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
	LPMENUITEM 	lpitem;
	dprintf_menu(stddeb,"CheckMenuItem (%04X, %04X, %04X) !\n", 
		     hMenu, wItemID, wFlags);
	if (!(lpitem = MENU_FindItem(&hMenu, &wItemID, wFlags))) return FALSE;
	if (wFlags & MF_CHECKED) lpitem->item_flags |= MF_CHECKED;
	else lpitem->item_flags &= ~MF_CHECKED;
	return TRUE;
}


/**********************************************************************
 *			EnableMenuItem		[USER.155]
 */
BOOL EnableMenuItem(HMENU hMenu, WORD wItemID, WORD wFlags)
{
    LPMENUITEM 	lpitem;
    dprintf_menu(stddeb,"EnableMenuItem (%04X, %04X, %04X) !\n", 
		 hMenu, wItemID, wFlags);
    if (!(lpitem = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return FALSE;

      /* We can't have MF_GRAYED and MF_DISABLED together */
    if (wFlags & MF_GRAYED)
    {
	lpitem->item_flags = (lpitem->item_flags & ~MF_DISABLED) | MF_GRAYED;
    }
    else if (wFlags & MF_DISABLED)
    {
	lpitem->item_flags = (lpitem->item_flags & ~MF_GRAYED) | MF_DISABLED;
    }
    else   /* MF_ENABLED */
    {
	lpitem->item_flags &= ~(MF_GRAYED | MF_DISABLED);
    }
    return TRUE;
}


/**********************************************************************
 *			GetMenuString		[USER.161]
 */
int GetMenuString(HMENU hMenu, WORD wItemID, 
		  LPSTR str, short nMaxSiz, WORD wFlags)
{
	LPMENUITEM 	lpitem;
	int		maxsiz;
	dprintf_menu(stddeb,"GetMenuString(%04X, %04X, %p, %d, %04X);\n",
					hMenu, wItemID, str, nMaxSiz, wFlags);
	if (str == NULL) return FALSE;
	lpitem = MENU_FindItem( &hMenu, &wItemID, wFlags );
	if (lpitem != NULL) {
		if (lpitem->item_text != NULL) {
			maxsiz = min(nMaxSiz - 1, strlen(lpitem->item_text));
			strncpy(str, lpitem->item_text, maxsiz + 1);
			}
		else
			maxsiz = 0;
		dprintf_menu(stddeb,"GetMenuString // Found !\n");
		return maxsiz;
		}
	return 0;
}


/**********************************************************************
 *			HiliteMenuItem		[USER.162]
 */
BOOL HiliteMenuItem(HWND hWnd, HMENU hMenu, WORD wItemID, WORD wHilite)
{
    LPPOPUPMENU menu;
    LPMENUITEM  lpitem;
    dprintf_menu(stddeb,"HiliteMenuItem(%04X, %04X, %04X, %04X);\n", 
						hWnd, hMenu, wItemID, wHilite);
    if (!(lpitem = MENU_FindItem( &hMenu, &wItemID, wHilite ))) return FALSE;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return FALSE;
    if (menu->FocusedItem == wItemID) return TRUE;
    MENU_HideSubPopups( hMenu );
    MENU_SelectItem( hMenu, wItemID );
    return TRUE;
}


/**********************************************************************
 *			GetMenuState		[USER.250]
 */
WORD GetMenuState(HMENU hMenu, WORD wItemID, WORD wFlags)
{
    LPMENUITEM lpitem;
    dprintf_menu(stddeb,"GetMenuState(%04X, %04X, %04X);\n", 
		 hMenu, wItemID, wFlags);
    if (!(lpitem = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    if (lpitem->item_flags & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *) USER_HEAP_ADDR( lpitem->item_id );
	if (!menu) return -1;
	else return (menu->nItems << 8) | (menu->wFlags & 0xff);
    }
    else return lpitem->item_flags;
}


/**********************************************************************
 *			GetMenuItemCount		[USER.263]
 */
WORD GetMenuItemCount(HMENU hMenu)
{
	LPPOPUPMENU	menu;
	dprintf_menu(stddeb,"GetMenuItemCount(%04X);\n", hMenu);
	menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu);
	if (menu == NULL) return (WORD)-1;
	dprintf_menu(stddeb,"GetMenuItemCount(%04X) return %d \n", 
		     hMenu, menu->nItems);
	return menu->nItems;
}


/**********************************************************************
 *			GetMenuItemID			[USER.264]
 */
WORD GetMenuItemID(HMENU hMenu, int nPos)
{
    LPPOPUPMENU	menu;
    MENUITEM *item;

    dprintf_menu(stddeb,"GetMenuItemID(%04X, %d);\n", hMenu, nPos);
    if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return -1;
    if ((nPos < 0) || (nPos >= menu->nItems)) return -1;
    item = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
    if (item[nPos].item_flags & MF_POPUP) return -1;
    return item[nPos].item_id;
}


/**********************************************************************
 *			InsertMenu		[USER.410]
 */
BOOL InsertMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    HANDLE hNewItems;
    MENUITEM *lpitem, *newItems;
    LPPOPUPMENU	menu;
    
    if (IS_STRING_ITEM(wFlags))
	{
	   dprintf_menu(stddeb,"InsertMenu (%04X, %04X, %04X, %04X, '%s') !\n",
				 hMenu, nPos, wFlags, wItemID, lpNewItem);
	}
    else
	   dprintf_menu(stddeb,"InsertMenu (%04X, %04X, %04X, %04X, %p) !\n",
		                  hMenu, nPos, wFlags, wItemID, lpNewItem);

      /* Find where to insert new item */

    if ((wFlags & MF_BYPOSITION) && (nPos == (WORD)-1))
    {
	  /* Special case: append to menu */
	if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return FALSE;
	nPos = menu->nItems;
    }
    else
    {
	if (!MENU_FindItem( &hMenu, &nPos, wFlags )) return FALSE;
	if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return FALSE;
    }

      /* Create new items array */

    hNewItems = USER_HEAP_ALLOC( GMEM_MOVEABLE,
				 sizeof(MENUITEM) * (menu->nItems+1) );
    if (!hNewItems) return FALSE;
    newItems = (MENUITEM *) USER_HEAP_ADDR( hNewItems );
    if (menu->nItems > 0)
    {
	  /* Copy the old array into the new */
	MENUITEM *oldItems = (MENUITEM *) USER_HEAP_ADDR( menu->hItems );
	if (nPos > 0) memcpy( newItems, oldItems, nPos * sizeof(MENUITEM) );
	if (nPos < menu->nItems) memcpy( &newItems[nPos+1], &oldItems[nPos],
					(menu->nItems-nPos)*sizeof(MENUITEM) );

	USER_HEAP_FREE( menu->hItems );
    }
    menu->hItems = hNewItems;
    menu->nItems++;

      /* Store the new item data */

    lpitem = &newItems[nPos];
    lpitem->item_flags = wFlags & ~(MF_HILITE | MF_MOUSESELECT);
    lpitem->item_id    = wItemID;

    if (IS_STRING_ITEM(wFlags))
    {
	  /* Item beginning with a backspace is a help item */
	if (lpNewItem[0] == '\b')
	{
	    lpitem->item_flags |= MF_HELP;
	    lpNewItem++;
	}
	lpitem->hText = USER_HEAP_ALLOC( GMEM_MOVEABLE, strlen(lpNewItem)+1 );
	lpitem->item_text = (char *)USER_HEAP_ADDR( lpitem->hText );
	strcpy( lpitem->item_text, lpNewItem );
    }
    else if (wFlags & MF_BITMAP) lpitem->hText = LOWORD((DWORD)lpNewItem);
    else lpitem->item_text = lpNewItem;

    if (wFlags & MF_POPUP)  /* Set the MF_POPUP flag on the popup-menu */
	((POPUPMENU *)USER_HEAP_ADDR(wItemID))->wFlags |= MF_POPUP;

    SetRectEmpty( &lpitem->rect );
    lpitem->hCheckBit   = hStdCheck;
    lpitem->hUnCheckBit = 0;
    return TRUE;
}


/**********************************************************************
 *			AppendMenu		[USER.411]
 */
BOOL AppendMenu(HMENU hMenu, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    return InsertMenu( hMenu, -1, wFlags | MF_BYPOSITION, wItemID, lpNewItem );
}


/**********************************************************************
 *			RemoveMenu		[USER.412]
 */
BOOL RemoveMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
    LPPOPUPMENU	menu;
    LPMENUITEM 	lpitem;
	dprintf_menu(stddeb,"RemoveMenu (%04X, %04X, %04X) !\n", 
		     hMenu, nPos, wFlags);
    if (!(lpitem = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return FALSE;
    
      /* Remove item */

    if (IS_STRING_ITEM(lpitem->item_flags)) USER_HEAP_FREE( lpitem->hText );
    if (--menu->nItems == 0)
    {
	USER_HEAP_FREE( menu->hItems );
	menu->hItems = 0;
    }
    else
    {
	while(nPos < menu->nItems)
	{
	    *lpitem = *(lpitem+1);
	    lpitem++;
	    nPos++;
	}
	menu->hItems = USER_HEAP_REALLOC( menu->hItems,
					  menu->nItems * sizeof(MENUITEM),
					  GMEM_MOVEABLE );
    }
    return TRUE;
}


/**********************************************************************
 *			DeleteMenu		[USER.413]
 */
BOOL DeleteMenu(HMENU hMenu, WORD nPos, WORD wFlags)
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) return FALSE;
    if (item->item_flags & MF_POPUP) DestroyMenu( item->item_id );
      /* nPos is now the position of the item */
    RemoveMenu( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}


/**********************************************************************
 *			ModifyMenu		[USER.414]
 */
BOOL ModifyMenu(HMENU hMenu, WORD nPos, WORD wFlags, WORD wItemID, LPSTR lpNewItem)
{
    LPMENUITEM 	lpitem;
    if (IS_STRING_ITEM(wFlags))
	dprintf_menu(stddeb,"ModifyMenu (%04X, %04X, %04X, %04X, '%s') !\n",
	       hMenu, nPos, wFlags, wItemID, lpNewItem);
    else
	dprintf_menu(stddeb,"ModifyMenu (%04X, %04X, %04X, %04X, %p) !\n",
	       hMenu, nPos, wFlags, wItemID, lpNewItem);
    if (!(lpitem = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    
    if (IS_STRING_ITEM(lpitem->item_flags)) USER_HEAP_FREE( lpitem->hText );
    lpitem->item_flags = wFlags & ~(MF_HILITE | MF_MOUSESELECT);
    lpitem->item_id    = wItemID;

    if (IS_STRING_ITEM(wFlags))
    {
	lpitem->hText = USER_HEAP_ALLOC( GMEM_MOVEABLE, strlen(lpNewItem)+1 );
	lpitem->item_text = (char *)USER_HEAP_ADDR( lpitem->hText );
	strcpy( lpitem->item_text, lpNewItem );
    }
    else if (wFlags & MF_BITMAP) lpitem->hText = LOWORD((DWORD)lpNewItem);
    else lpitem->item_text = lpNewItem;
    SetRectEmpty( &lpitem->rect );
    return TRUE;
}


/**********************************************************************
 *			CreatePopupMenu		[USER.415]
 */
HMENU CreatePopupMenu()
{
    HMENU hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu())) return 0;
    menu = (POPUPMENU *) USER_HEAP_ADDR( hmenu );
    menu->wFlags |= MF_POPUP;
    return hmenu;
}


/**********************************************************************
 *			GetMenuCheckMarkDimensions	[USER.417]
 */
DWORD GetMenuCheckMarkDimensions()
{
    return MAKELONG( check_bitmap_width, check_bitmap_height );
}


/**********************************************************************
 *			SetMenuItemBitmaps	[USER.418]
 */
BOOL SetMenuItemBitmaps(HMENU hMenu, WORD nPos, WORD wFlags,
		HBITMAP hNewCheck, HBITMAP hNewUnCheck)
{
    LPMENUITEM lpitem;
   dprintf_menu(stddeb,"SetMenuItemBitmaps (%04X, %04X, %04X, %04X, %08X) !\n",
	    hMenu, nPos, wFlags, hNewCheck, hNewUnCheck);
    if (!(lpitem = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;

    if (!hNewCheck && !hNewUnCheck)
    {
	  /* If both are NULL, restore default bitmaps */
	lpitem->hCheckBit   = hStdCheck;
	lpitem->hUnCheckBit = 0;
	lpitem->item_flags &= ~MF_USECHECKBITMAPS;
    }
    else  /* Install new bitmaps */
    {
	lpitem->hCheckBit   = hNewCheck;
	lpitem->hUnCheckBit = hNewUnCheck;
	lpitem->item_flags |= MF_USECHECKBITMAPS;
    }
    return TRUE;
}


/**********************************************************************
 *			CreateMenu		[USER.151]
 */
HMENU CreateMenu()
{
    HMENU hMenu;
    LPPOPUPMENU menu;
    dprintf_menu(stddeb,"CreateMenu !\n");
    if (!(hMenu = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(POPUPMENU) )))
	return 0;
    menu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu);
    menu->hNext  = 0;
    menu->wFlags = 0;
    menu->wMagic = MENU_MAGIC;
    menu->hTaskQ = 0;
    menu->Width  = 0;
    menu->Height = 0;
    menu->nItems = 0;
    menu->hWnd   = 0;
    menu->hItems = 0;
    menu->FocusedItem = NO_SELECTED_ITEM;
    dprintf_menu(stddeb,"CreateMenu // return %04X\n", hMenu);
    return hMenu;
}


/**********************************************************************
 *			DestroyMenu		[USER.152]
 */
BOOL DestroyMenu(HMENU hMenu)
{
	LPPOPUPMENU lppop;
	dprintf_menu(stddeb,"DestroyMenu (%04X) !\n", hMenu);
	if (hMenu == 0) return FALSE;
	lppop = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu);
	if (lppop == NULL) return FALSE;
	if ((lppop->wFlags & MF_POPUP) && lppop->hWnd)
            DestroyWindow( lppop->hWnd );

	if (lppop->hItems)
	{
	    int i;
	    MENUITEM *item = (MENUITEM *) USER_HEAP_ADDR( lppop->hItems );
	    for (i = lppop->nItems; i > 0; i--, item++)
	    {
		if (item->item_flags & MF_POPUP)
		    DestroyMenu( item->item_id );
	    }
	    USER_HEAP_FREE( lppop->hItems );
	}
	USER_HEAP_FREE( hMenu );
	dprintf_menu(stddeb,"DestroyMenu (%04X) // End !\n", hMenu);
	return TRUE;
}

/**********************************************************************
 *			GetSystemMenu		[USER.156]
 */
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert)
{
	WND		*wndPtr;
	wndPtr = WIN_FindWndPtr(hWnd);
	if (!bRevert) {
		return wndPtr->hSysMenu;
		}
	else {
		DestroyMenu(wndPtr->hSysMenu);
		wndPtr->hSysMenu = CopySysMenu();
		}
	return wndPtr->hSysMenu;
}

/**********************************************************************
 *			SetSystemMenu		[USER.280]
 */
BOOL SetSystemMenu(HWND hWnd, HMENU newHmenu)
{
    WND *wndPtr;

    if ((wndPtr = WIN_FindWndPtr(hWnd)) != NULL) wndPtr->hSysMenu = newHmenu;
    return TRUE;
}


/**********************************************************************
 *			GetMenu		[USER.157]
 */
HMENU GetMenu(HWND hWnd) 
{ 
	WND * wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) return 0;
	return wndPtr->wIDmenu;
}


/**********************************************************************
 * 			SetMenu 	[USER.158]
 */
BOOL SetMenu(HWND hWnd, HMENU hMenu)
{
	LPPOPUPMENU lpmenu;
	WND * wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) {
		fprintf(stderr,"SetMenu(%04X, %04X) // Bad window handle !\n",
			hWnd, hMenu);
		return FALSE;
		}
	dprintf_menu(stddeb,"SetMenu(%04X, %04X);\n", hWnd, hMenu);
	if (GetCapture() == hWnd) ReleaseCapture();
	wndPtr->wIDmenu = hMenu;
	if (hMenu != 0)
	{
	    lpmenu = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu);
	    if (lpmenu == NULL) {
		fprintf(stderr,"SetMenu(%04X, %04X) // Bad menu handle !\n", 
			hWnd, hMenu);
		return FALSE;
		}
	    lpmenu->hWnd = hWnd;
	    lpmenu->wFlags &= ~MF_POPUP;  /* Can't be a popup */
	    lpmenu->Height = 0;  /* Make sure we recalculate the size */
	}
	SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
		      SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
	return TRUE;
}



/**********************************************************************
 *			GetSubMenu		[USER.159]
 */
HMENU GetSubMenu(HMENU hMenu, short nPos)
{
    LPPOPUPMENU lppop;
    LPMENUITEM 	lpitem;
    dprintf_menu(stddeb,"GetSubMenu (%04X, %04X) !\n", hMenu, nPos);
    if (!(lppop = (LPPOPUPMENU) USER_HEAP_ADDR(hMenu))) return 0;
    if ((WORD)nPos >= lppop->nItems) return 0;
    lpitem = (MENUITEM *) USER_HEAP_ADDR( lppop->hItems );
    if (!(lpitem[nPos].item_flags & MF_POPUP)) return 0;
    return lpitem[nPos].item_id;
}


/**********************************************************************
 *			DrawMenuBar		[USER.160]
 */
void DrawMenuBar(HWND hWnd)
{
	WND		*wndPtr;
	LPPOPUPMENU lppop;
	dprintf_menu(stddeb,"DrawMenuBar (%04X)\n", hWnd);
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr != NULL && (wndPtr->dwStyle & WS_CHILD) == 0 && 
		wndPtr->wIDmenu != 0) {
		dprintf_menu(stddeb,"DrawMenuBar wIDmenu=%04X \n", 
			     wndPtr->wIDmenu);
		lppop = (LPPOPUPMENU) USER_HEAP_ADDR(wndPtr->wIDmenu);
		if (lppop == NULL) return;

		lppop->Height = 0; /* Make sure we call MENU_MenuBarCalcSize */
		SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
			    SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
	    }
}


/***********************************************************************
 *           EndMenu   (USER.187)
 */
void EndMenu(void)
{
      /* Note: this won't work when we have multiple tasks... */
    fEndMenuCalled = TRUE;
}


/***********************************************************************
 *           LookupMenuHandle   (USER.217)
 */
HMENU LookupMenuHandle( HMENU hmenu, INT id )
{
    if (!MENU_FindItem( &hmenu, &id, MF_BYCOMMAND )) return 0;
    else return hmenu;
}


/**********************************************************************
 *			LoadMenuIndirect	[USER.220]
 */
HMENU LoadMenuIndirect(LPSTR menu_template)
{
	HMENU     		hMenu;
	MENU_HEADER 	*menu_desc;
	dprintf_menu(stddeb,"LoadMenuIndirect: menu_template '%p'\n", 
		     menu_template);
	hMenu = CreateMenu();
	menu_desc = (MENU_HEADER *)menu_template;
	ParseMenuResource((WORD *)(menu_desc + 1), 0, hMenu); 
	return hMenu;
}


/**********************************************************************
 *			CopySysMenu (Internal)
 */
HMENU CopySysMenu()
{
    HMENU hMenu;
    LPPOPUPMENU menu;
    extern unsigned char sysres_MENU_SYSMENU[];

    hMenu=LoadMenuIndirect(sysres_MENU_SYSMENU);
    if(!hMenu){
	dprintf_menu(stddeb,"No SYSMENU\n");
	return 0;
    }
    menu = (POPUPMENU*) USER_HEAP_ADDR(hMenu);
    menu->wFlags |= MF_SYSMENU|MF_POPUP;
    dprintf_menu(stddeb,"CopySysMenu hMenu=%04X !\n", hMenu);
    return hMenu;
}


/**********************************************************************
 *			ParseMenuResource (from Resource or Template)
 */
WORD * ParseMenuResource(WORD *first_item, int level, HMENU hMenu)
{
    WORD 	*item;
    WORD 	*next_item;
    HMENU	hSubMenu;
    int   	i;

    level++;
    next_item = first_item;
    i = 0;
    do {
	i++;
	item = next_item;
	if (*item & MF_POPUP) {
	    MENU_POPUPITEM *popup_item = (MENU_POPUPITEM *) item;
	    next_item = (WORD *) (popup_item->item_text + 
				  strlen(popup_item->item_text) + 1);
	    hSubMenu = CreatePopupMenu();
	    next_item = ParseMenuResource(next_item, level, hSubMenu);
	    AppendMenu(hMenu, popup_item->item_flags, 
	    	hSubMenu, popup_item->item_text);
	    }
	else {
		MENUITEMTEMPLATE *normal_item = (MENUITEMTEMPLATE *) item;
		next_item = (WORD *) (normal_item->item_text + 
		strlen(normal_item->item_text) + 1);
		if (strlen(normal_item->item_text) == 0 && normal_item->item_id == 0) 
			normal_item->item_flags |= MF_SEPARATOR;
		AppendMenu(hMenu, normal_item->item_flags, 
			normal_item->item_id, normal_item->item_text);
	    }
	}
    while (!(*item & MF_END));
    return next_item;
}


/**********************************************************************
 *		IsMenu    (USER.358)
 */
BOOL IsMenu( HMENU hmenu )
{
    LPPOPUPMENU menu;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_ADDR( hmenu ))) return FALSE;
    return (menu->wMagic == MENU_MAGIC);
}
