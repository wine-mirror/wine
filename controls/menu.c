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
#include "user.h"
#include "message.h"
#include "graphics.h"
#include "resource.h"
#include "string32.h"
#include "stddebug.h"
#include "debug.h"

/* Menu item structure */
typedef struct
{
    WORD	item_flags;    /* Item flags */
    UINT	item_id;       /* Item or popup id */
    RECT16      rect;          /* Item area (relative to menu window) */
    WORD        xTab;          /* X position of text after Tab */
    HBITMAP	hCheckBit;     /* Bitmap for checked item */
    HBITMAP	hUnCheckBit;   /* Bitmap for unchecked item */
    LPSTR       text;          /* Item text or bitmap handle */
} MENUITEM;

/* Popup menu structure */
typedef struct
{
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        wMagic;       /* Magic number */
    HANDLE      hTaskQ;       /* Task queue for this menu */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    WORD	nItems;       /* Number of items in the menu */
    HWND	hWnd;	      /* Window containing the menu */
    MENUITEM   *items;        /* Array of menu items */
    UINT	FocusedItem;  /* Currently focused item */
} POPUPMENU, *LPPOPUPMENU;

#define MENU_MAGIC   0x554d  /* 'MU' */

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
static HMENU MENU_DefSysMenu = 0;  /* Default system menu */


/* we _can_ use global popup window because there's no way 2 menues can
 * be tracked at the same time.
 */ 

static WND* pTopPWnd   = 0;
static UINT uSubPWndLevel = 0;


/**********************************************************************
 *           MENU_CopySysMenu
 *
 * Load a copy of the system menu.
 */
static HMENU MENU_CopySysMenu(void)
{
    HMENU hMenu;
    HGLOBAL handle;
    POPUPMENU *menu;

    if (!(handle = SYSRES_LoadResource( SYSRES_MENU_SYSMENU ))) return 0;
    hMenu = LoadMenuIndirect16( GlobalLock16( handle ) );
    SYSRES_FreeResource( handle );
    if (!hMenu)
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
HMENU MENU_GetDefSysMenu(void)
{
    return MENU_DefSysMenu;
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
static BOOL MENU_IsInSysMenu( POPUPMENU *menu, POINT16 pt )
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
static MENUITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags )
{
    POPUPMENU *menu;
    int i;

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
static MENUITEM *MENU_FindItemByCoords( POPUPMENU *menu, int x, int y, UINT *pos )
{
    MENUITEM *item;
    WND *wndPtr;
    int i;

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
static UINT MENU_FindItemByKey( HWND hwndOwner, HMENU hmenu, UINT key )
{
    POPUPMENU *menu;
    MENUITEM *item;
    int i;
    LONG menuchar;

    if (!IsMenu( hmenu )) hmenu = WIN_FindWndPtr(hwndOwner)->hSysMenu;
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
#ifdef WINELIB32
    menuchar = SendMessage32A( hwndOwner, WM_MENUCHAR, 
                               MAKEWPARAM(key,menu->wFlags), hmenu );
#else
    menuchar = SendMessage16( hwndOwner, WM_MENUCHAR, key,
                              MAKELONG( menu->wFlags, hmenu ) );
#endif
    if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
    if (HIWORD(menuchar) == 1) return -2;
    return -1;
}


/***********************************************************************
 *           MENU_CalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void MENU_CalcItemSize( HDC hdc, MENUITEM *lpitem, HWND hwndOwner,
			       int orgX, int orgY, BOOL menuBar )
{
    DWORD dwSize;
    char *p;

    SetRect16( &lpitem->rect, orgX, orgY, orgX, orgY );

    if (lpitem->item_flags & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT *mis;
        if (!(mis = SEGPTR_NEW(MEASUREITEMSTRUCT))) return;
        mis->CtlType    = ODT_MENU;
        mis->itemID     = lpitem->item_id;
        mis->itemData   = (DWORD)lpitem->text;
        mis->itemHeight = 16;
        mis->itemWidth  = 30;
        SendMessage16( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)SEGPTR_GET(mis) );
        lpitem->rect.bottom += mis->itemHeight;
        lpitem->rect.right  += mis->itemWidth;
        dprintf_menu( stddeb, "DrawMenuItem: MeasureItem %04x %dx%d!\n",
                      lpitem->item_id, mis->itemWidth, mis->itemHeight );
        SEGPTR_FREE(mis);
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
	BITMAP bm;
        if (GetObject( (HBITMAP16)(UINT32)lpitem->text,
                       sizeof(BITMAP), (LPSTR)&bm ))
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
static void MENU_PopupMenuCalcSize( LPPOPUPMENU lppop, HWND hwndOwner )
{
    MENUITEM *lpitem;
    HDC hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth;

    lppop->Width = lppop->Height = 0;
    if (lppop->nItems == 0) return;
    hdc = GetDC( 0 );
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
    ReleaseDC( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * Calculate the size of the menu bar.
 */
static void MENU_MenuBarCalcSize( HDC hdc, LPRECT16 lprect, LPPOPUPMENU lppop,
				  HWND hwndOwner )
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
static void MENU_DrawMenuItem( HWND hwnd, HDC hdc, MENUITEM *lpitem,
			       UINT height, BOOL menuBar )
{
    RECT16 rect;

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
        CONV_RECT16TO32( &lpitem->rect, &dis.rcItem );
        SendMessage32A( hwnd, WM_DRAWITEM, 0, (LPARAM)&dis );
        return;
    }

    if (menuBar && (lpitem->item_flags & MF_SEPARATOR)) return;
    rect = lpitem->rect;

      /* Draw the background */

    if (lpitem->item_flags & MF_HILITE)
	FillRect16( hdc, &rect, sysColorObjects.hbrushHighlight );
    else FillRect16( hdc, &rect, sysColorObjects.hbrushMenu );
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
	GRAPH_DrawBitmap( hdc, (HBITMAP16)(UINT32)lpitem->text,
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
	
	DrawText16( hdc, lpitem->text, i, &rect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE );

	if (lpitem->text[i])  /* There's a tab or flush-right char */
	{
	    if (lpitem->text[i] == '\t')
	    {
		rect.left = lpitem->xTab;
		DrawText16( hdc, lpitem->text + i + 1, -1, &rect,
                            DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	    }
	    else DrawText16( hdc, lpitem->text + i + 1, -1, &rect,
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
    RECT16 rect;
    int i;

    GetClientRect16( hwnd, &rect );
    FillRect16( hdc, &rect, sysColorObjects.hbrushMenu );
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
UINT MENU_DrawMenuBar(HDC hDC, LPRECT16 lprect, HWND hwnd, BOOL suppress_draw)
{
    LPPOPUPMENU lppop;
    int i;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    
    lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR( (HMENU)wndPtr->wIDmenu );
    if (lppop == NULL || lprect == NULL) return SYSMETRICS_CYMENU;
    dprintf_menu(stddeb,"MENU_DrawMenuBar(%04x, %p, %p); !\n", 
		 hDC, lprect, lppop);
    if (lppop->Height == 0) MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);
    lprect->bottom = lprect->top + lppop->Height;
    if (suppress_draw) return lppop->Height;
    
    FillRect16(hDC, lprect, sysColorObjects.hbrushMenu );
    SelectObject( hDC, sysColorObjects.hpenWindowFrame );
    MoveTo( hDC, lprect->left, lprect->bottom );
    LineTo( hDC, lprect->right, lprect->bottom );

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
BOOL MENU_SwitchTPWndTo( HTASK hTask)
{
  /* This is supposed to be called when popup is hidden */

  TDB* task = (TDB*)GlobalLock16(hTask);

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
static BOOL MENU_ShowPopup(HWND hwndOwner, HMENU hmenu, UINT id, int x, int y)
{
    POPUPMENU 	*menu;
    WND 	*wndPtr = NULL;
    BOOL	 skip_init = 0;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	menu->items[menu->FocusedItem].item_flags &= ~(MF_HILITE|MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }
    SendMessage16( hwndOwner, WM_INITMENUPOPUP, (WPARAM)hmenu,
		 MAKELONG( id, (menu->wFlags & MF_SYSMENU) ? 1 : 0 ));
    MENU_PopupMenuCalcSize( menu, hwndOwner );

    wndPtr = WIN_FindWndPtr( hwndOwner );
    if (!wndPtr) return FALSE;

    if (!pTopPWnd)
    {
	pTopPWnd = WIN_FindWndPtr(CreateWindow16( POPUPMENU_CLASS_ATOM, (SEGPTR)0,
				   		WS_POPUP | WS_BORDER, x, y, 
				   		menu->Width + 2*SYSMETRICS_CXBORDER,
				   		menu->Height + 2*SYSMETRICS_CYBORDER,
				   		0, 0, wndPtr->hInstance, (SEGPTR)hmenu ));
	if (!pTopPWnd) return FALSE;
	skip_init = TRUE;
    }

    if( uSubPWndLevel )
    {
	/* create new window for the submenu */
	HWND  hWnd = CreateWindow16( POPUPMENU_CLASS_ATOM, (SEGPTR)0,
                                   WS_POPUP | WS_BORDER, x, y,
                                   menu->Width + 2*SYSMETRICS_CXBORDER,
                                   menu->Height + 2*SYSMETRICS_CYBORDER,
                                   menu->hWnd, 0, wndPtr->hInstance, (SEGPTR)hmenu );
	if( !hWnd ) return FALSE;
	menu->hWnd = hWnd;
    }
    else 
    {
	if( !skip_init )
	  {
            MENU_SwitchTPWndTo(GetCurrentTask());
	    SendMessage16( pTopPWnd->hwndSelf, WM_USER, (WPARAM)hmenu, 0L);
	  }
	menu->hWnd = pTopPWnd->hwndSelf;
    }

    uSubPWndLevel++;

    SetWindowPos(menu->hWnd, 0, x, y, menu->Width + 2*SYSMETRICS_CXBORDER, 
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
static void MENU_SelectItem( HWND hwndOwner, HMENU hmenu, UINT wIndex,
                             BOOL sendMenuSelect )
{
    LPPOPUPMENU lppop;
    HDC hdc;

    lppop = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!lppop->nItems) return;
    if ((wIndex != NO_SELECTED_ITEM) && 
	(wIndex != SYSMENU_SELECTED) &&
	(lppop->items[wIndex].item_flags & MF_SEPARATOR))
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
#ifdef WINELIB32
/* FIX: LostInfo */
                SendMessage32A( hwndOwner, WM_MENUSELECT,
                             MAKEWPARAM( WIN_FindWndPtr(lppop->hWnd)->hSysMenu,
                                         lppop->wFlags | MF_MOUSESELECT ),
                             (LPARAM)hmenu );
#else
                SendMessage16( hwndOwner, WM_MENUSELECT,
                             WIN_FindWndPtr(lppop->hWnd)->hSysMenu,
                             MAKELONG(lppop->wFlags | MF_MOUSESELECT, hmenu));
#endif
        }
	else
	{
	    lppop->items[lppop->FocusedItem].item_flags |= MF_HILITE;
	    MENU_DrawMenuItem( lppop->hWnd, hdc, &lppop->items[lppop->FocusedItem],
                               lppop->Height, !(lppop->wFlags & MF_POPUP) );
            if (sendMenuSelect)
#ifdef WINELIB32
                SendMessage32A( hwndOwner, WM_MENUSELECT,
                             MAKEWPARAM( lppop->items[lppop->FocusedItem].item_id,
                                         lppop->items[lppop->FocusedItem].item_flags| 
                                         MF_MOUSESELECT ),
                             (LPARAM) hmenu );
#else
	        SendMessage16( hwndOwner, WM_MENUSELECT,
                             lppop->items[lppop->FocusedItem].item_id,
                             MAKELONG( lppop->items[lppop->FocusedItem].item_flags | MF_MOUSESELECT, hmenu));
#endif
	}
    }
#ifdef WINELIB32
/* FIX: Lost Info */
    else if (sendMenuSelect)
        SendMessage32A( hwndOwner, WM_MENUSELECT, 
                     MAKEWPARAM( (DWORD)hmenu, lppop->wFlags | MF_MOUSESELECT),
                     hmenu );
#else
    else if (sendMenuSelect)
        SendMessage16( hwndOwner, WM_MENUSELECT, hmenu,
                     MAKELONG( lppop->wFlags | MF_MOUSESELECT, hmenu ) );
#endif

    ReleaseDC( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_SelectNextItem
 */
static void MENU_SelectNextItem( HWND hwndOwner, HMENU hmenu )
{
    int i;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu->items) return;
    if ((menu->FocusedItem != NO_SELECTED_ITEM) &&
	(menu->FocusedItem != SYSMENU_SELECTED))
    {
	for (i = menu->FocusedItem+1; i < menu->nItems; i++)
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
    for (i = 0; i < menu->nItems; i++)
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


/***********************************************************************
 *           MENU_SelectPrevItem
 */
static void MENU_SelectPrevItem( HWND hwndOwner, HMENU hmenu )
{
    int i;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu->items) return;
    if ((menu->FocusedItem != NO_SELECTED_ITEM) &&
	(menu->FocusedItem != SYSMENU_SELECTED))
    {
	for (i = menu->FocusedItem - 1; i >= 0; i--)
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
    for (i = menu->nItems - 1; i > 0; i--)
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
static BOOL MENU_SetItemData( MENUITEM *item, UINT flags, UINT id, LPCSTR str )
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
    else if ((flags & MF_BITMAP) || (flags & MF_OWNERDRAW))
        item->text = (LPSTR)str;
    else item->text = NULL;

    item->item_flags = flags & ~(MF_HILITE | MF_MOUSESELECT);
    item->item_id    = id;
    SetRectEmpty16( &item->rect );
    if (prevText) HeapFree( SystemHeap, 0, prevText );
    return TRUE;
}


/**********************************************************************
 *         MENU_InsertItem
 *
 * Insert a new item into a menu.
 */
static MENUITEM *MENU_InsertItem( HMENU hMenu, UINT pos, UINT flags )
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
        ((pos == (UINT)-1) || (pos == menu->nItems)))
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
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu, BOOL unicode )
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
        else res += (STRING32_lstrlenW((LPCWSTR)str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu, unicode )))
                return NULL;
            if (!unicode) AppendMenu32A( hMenu, flags, (UINT)hSubMenu, str );
            else AppendMenu32W( hMenu, flags, (UINT)hSubMenu, (LPCWSTR)str );
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
static HMENU MENU_GetSubPopup( HMENU hmenu )
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
    return (HMENU)item->item_id;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void MENU_HideSubPopups( HWND hwndOwner, HMENU hmenu,
                                BOOL sendMenuSelect )
{
    MENUITEM *item;
    POPUPMENU *menu, *submenu;
    HMENU hsubmenu;

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
	hsubmenu = (HMENU)item->item_id;
    }
    submenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hsubmenu );
    MENU_HideSubPopups( hwndOwner, hsubmenu, FALSE );
    MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, sendMenuSelect );
    if (submenu->hWnd == pTopPWnd->hwndSelf ) 
    {
	ShowWindow( submenu->hWnd, SW_HIDE );
	uSubPWndLevel = 0;
    }
    else
    {
	DestroyWindow( submenu->hWnd );
	submenu->hWnd = 0;
    }
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

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return hmenu;
    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return hmenu;
    if (menu->FocusedItem == NO_SELECTED_ITEM) return hmenu;
    if (menu->FocusedItem == SYSMENU_SELECTED)
    {
	MENU_ShowPopup(hwndOwner, wndPtr->hSysMenu, 0, wndPtr->rectClient.left,
		wndPtr->rectClient.top - menu->Height - 2*SYSMETRICS_CYBORDER);
	if (selectFirst) MENU_SelectNextItem( hwndOwner, wndPtr->hSysMenu );
	return wndPtr->hSysMenu;
    }
    item = &menu->items[menu->FocusedItem];
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
    if (selectFirst) MENU_SelectNextItem( hwndOwner, (HMENU)item->item_id );
    return (HMENU)item->item_id;
}


/***********************************************************************
 *           MENU_FindMenuByCoords
 *
 * Find the menu containing a given point (in screen coords).
 */
static HMENU MENU_FindMenuByCoords( HMENU hmenu, POINT16 pt )
{
    POPUPMENU *menu;
    HWND hwnd;

    if (!(hwnd = WindowFromPoint16( pt ))) return 0;
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
static BOOL MENU_ExecFocusedItem( HWND hwndOwner, HMENU hmenu,
				  HMENU *hmenuCurrent )
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
			     POINT16 pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    UINT id;

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
static BOOL MENU_ButtonUp( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent,
			   POINT16 pt )
{
    POPUPMENU *menu;
    MENUITEM *item;
    HMENU hsubmenu = 0;
    UINT id;

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
	hsubmenu = (HMENU)item->item_id;
    }
      /* Select first item of sub-popup */
    MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, FALSE );
    MENU_SelectNextItem( hwndOwner, hsubmenu );
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
			    POINT16 pt )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    UINT id = NO_SELECTED_ITEM;

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
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 * hmenuCurrent is the top-most visible popup.
 */
static void MENU_KeyLeft( HWND hwndOwner, HMENU hmenu, HMENU *hmenuCurrent )
{
    POPUPMENU *menu;
    HMENU hmenutmp, hmenuprev;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    hmenuprev = hmenutmp = hmenu;
    while (hmenutmp != *hmenuCurrent)
    {
	hmenutmp = MENU_GetSubPopup( hmenuprev );
	if (hmenutmp != *hmenuCurrent) hmenuprev = hmenutmp;
    }
    MENU_HideSubPopups( hwndOwner, hmenuprev, TRUE );

    if ((hmenuprev == hmenu) && !(menu->wFlags & MF_POPUP))
    {
	  /* Select previous item on the menu bar */
	MENU_SelectPrevItem( hwndOwner, hmenu );
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

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );

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
	MENU_HideSubPopups( hwndOwner, hmenu, FALSE );
	MENU_SelectNextItem( hwndOwner, hmenu );
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
	MENU_HideSubPopups( hwndOwner, hmenuprev, TRUE );
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
static BOOL MENU_TrackMenu( HMENU hmenu, UINT wFlags, int x, int y,
			    HWND hwnd, const RECT16 *lprect )
{
    MSG *msg;
    HLOCAL16 hMsg;
    POPUPMENU *menu;
    HMENU hmenuCurrent = hmenu;
    BOOL fClosed = FALSE, fRemove;
    UINT pos;

    fEndMenuCalled = FALSE;
    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    if (x && y)
    {
	POINT16 pt = { x, y };
	MENU_ButtonDown( hwnd, hmenu, &hmenuCurrent, pt );
    }
    SetCapture( hwnd );
    hMsg = USER_HEAP_ALLOC( sizeof(MSG) );
    msg = (MSG *)USER_HEAP_LIN_ADDR( hMsg );
    while (!fClosed)
    {
	if (!MSG_InternalGetMessage( (SEGPTR)USER_HEAP_SEG_ADDR(hMsg), 0,
                                     hwnd, MSGF_MENU, 0, TRUE ))
	    break;

        fRemove = FALSE;
	if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
	{
	      /* Find the sub-popup for this mouse event (if any) */
	    HMENU hsubmenu = MENU_FindMenuByCoords( hmenu, msg->pt );

	    switch(msg->message)
	    {
	    case WM_RBUTTONDOWN:
	    case WM_NCRBUTTONDOWN:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */
	    case WM_LBUTTONDOWN:
	    case WM_NCLBUTTONDOWN:
		fClosed = !MENU_ButtonDown( hwnd, hsubmenu,
					    &hmenuCurrent, msg->pt );
		break;
		
	    case WM_RBUTTONUP:
	    case WM_NCRBUTTONUP:
		if (!(wFlags & TPM_RIGHTBUTTON)) break;
		/* fall through */
	    case WM_LBUTTONUP:
	    case WM_NCLBUTTONUP:
		  /* If outside all menus but inside lprect, ignore it */
		if (!hsubmenu && lprect && PtInRect16(lprect, msg->pt)) break;
		fClosed = !MENU_ButtonUp( hwnd, hsubmenu,
					  &hmenuCurrent, msg->pt );
                fRemove = TRUE;  /* Remove event even if outside menu */
		break;
		
	    case WM_MOUSEMOVE:
	    case WM_NCMOUSEMOVE:
		if ((msg->wParam & MK_LBUTTON) ||
		    ((wFlags & TPM_RIGHTBUTTON) && (msg->wParam & MK_RBUTTON)))
		{
		    fClosed = !MENU_MouseMove( hwnd, hsubmenu,
					       &hmenuCurrent, msg->pt );
		}
		break;
	    }
	}
	else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
	{
            fRemove = TRUE;  /* Keyboard messages are always removed */
	    switch(msg->message)
	    {
	    case WM_KEYDOWN:
		switch(msg->wParam)
		{
		case VK_HOME:
		    MENU_SelectItem( hwnd, hmenuCurrent, NO_SELECTED_ITEM, FALSE );
		    MENU_SelectNextItem( hwnd, hmenuCurrent );
		    break;

		case VK_END:
		    MENU_SelectItem( hwnd, hmenuCurrent, NO_SELECTED_ITEM, FALSE );
		    MENU_SelectPrevItem( hwnd, hmenuCurrent );
		    break;

		case VK_UP:
		    MENU_SelectPrevItem( hwnd, hmenuCurrent );
		    break;

		case VK_DOWN:
		      /* If on menu bar, pull-down the menu */
		    if (!(menu->wFlags & MF_POPUP) && (hmenuCurrent == hmenu))
			hmenuCurrent = MENU_ShowSubPopup( hwnd, hmenu, TRUE );
		    else
			MENU_SelectNextItem( hwnd, hmenuCurrent );
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
		switch(msg->wParam)
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
		    if ((msg->wParam <= 32) || (msg->wParam >= 127)) break;
		    pos = MENU_FindItemByKey( hwnd, hmenuCurrent, msg->wParam );
		    if (pos == (UINT)-2) fClosed = TRUE;
		    else if (pos == (UINT)-1) MessageBeep(0);
		    else
		    {
			MENU_SelectItem( hwnd, hmenuCurrent, pos, TRUE );
			fClosed = !MENU_ExecFocusedItem( hwnd, hmenuCurrent,
							 &hmenuCurrent );
			
		    }
		}		    
		break;  /* WM_CHAR */
	    }  /* switch(msg->message) */
	}
	else
	{
	    DispatchMessage( msg );
	}
	if (fEndMenuCalled) fClosed = TRUE;
	if (!fClosed) fRemove = TRUE;

        if (fRemove)  /* Remove the message from the queue */
	    PeekMessage( msg, 0, msg->message, msg->message, PM_REMOVE );
    }
    USER_HEAP_FREE( hMsg );
    ReleaseCapture();
    MENU_HideSubPopups( hwnd, hmenu, FALSE );
    if (menu->wFlags & MF_POPUP) 
    {
         ShowWindow( menu->hWnd, SW_HIDE );
	 uSubPWndLevel = 0;
    }
    MENU_SelectItem( hwnd, hmenu, NO_SELECTED_ITEM, FALSE );
    SendMessage16( hwnd, WM_MENUSELECT, 0, MAKELONG( 0xffff, 0 ) );
    fEndMenuCalled = FALSE;
    return TRUE;
}


/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( HWND hwnd, POINT16 pt )
{
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    HideCaret(0);
    SendMessage16( hwnd, WM_ENTERMENULOOP, 0, 0 );
    SendMessage16( hwnd, WM_INITMENU, wndPtr->wIDmenu, 0 );
    MENU_TrackMenu( (HMENU)wndPtr->wIDmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
		    pt.x, pt.y, hwnd, NULL );
    SendMessage16( hwnd, WM_EXITMENULOOP, 0, 0 );
    ShowCaret(0);
}


/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackKbdMenuBar( WND* wndPtr, UINT wParam, INT vkey)
{
    UINT uItem = NO_SELECTED_ITEM;
   HMENU hTrackMenu; 

    /* find window that has a menu 
     */
 
    if( !(wndPtr->dwStyle & WS_CHILD) )
      {
	  wndPtr = WIN_FindWndPtr( GetActiveWindow() );
          if( !wndPtr ) return;
      }
    else
      while( wndPtr->dwStyle & WS_CHILD && 
           !(wndPtr->dwStyle & WS_SYSMENU) )
           if( !(wndPtr = wndPtr->parent) ) return;
          
    if( wndPtr->dwStyle & WS_CHILD || !wndPtr->wIDmenu )
      if( !(wndPtr->dwStyle & WS_SYSMENU) )
	return;

    hTrackMenu = ( IsMenu( wndPtr->wIDmenu ) )? wndPtr->wIDmenu:
                                                wndPtr->hSysMenu;

    HideCaret(0);
    SendMessage16( wndPtr->hwndSelf, WM_ENTERMENULOOP, 0, 0 );
    SendMessage16( wndPtr->hwndSelf, WM_INITMENU, wndPtr->wIDmenu, 0 );

    /* find suitable menu entry 
     */

    if( vkey == VK_SPACE )
        uItem = SYSMENU_SELECTED;
    else if( vkey )
      {
        uItem = MENU_FindItemByKey( wndPtr->hwndSelf, wndPtr->wIDmenu, vkey );
	if( uItem >= 0xFFFE )
	  {
	    if( uItem == 0xFFFF ) 
                MessageBeep(0);
	    SendMessage16( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
            ShowCaret(0);
	    return;
	  }
      }

    MENU_SelectItem( wndPtr->hwndSelf, hTrackMenu, uItem, TRUE );
    if( uItem == NO_SELECTED_ITEM )
      MENU_SelectNextItem( wndPtr->hwndSelf, hTrackMenu );
    else
      PostMessage( wndPtr->hwndSelf, WM_KEYDOWN, VK_DOWN, 0L );

    MENU_TrackMenu( hTrackMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON,
		    0, 0, wndPtr->hwndSelf, NULL );

    SendMessage16( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
    ShowCaret(0);
}


/**********************************************************************
 *           TrackPopupMenu16   (USER.416)
 */
BOOL16 TrackPopupMenu16( HMENU16 hMenu, UINT16 wFlags, INT16 x, INT16 y,
                         INT16 nReserved, HWND16 hWnd, const RECT16 *lpRect )
{
    BOOL ret = FALSE;

    HideCaret(0);
    if (MENU_ShowPopup( hWnd, hMenu, 0, x, y )) 
	ret = MENU_TrackMenu( hMenu, wFlags, 0, 0, hWnd, lpRect );
    ShowCaret(0);
    return ret;
}


/**********************************************************************
 *           TrackPopupMenu32   (USER32.548)
 */
BOOL32 TrackPopupMenu32( HMENU32 hMenu, UINT32 wFlags, INT32 x, INT32 y,
                         INT32 nReserved, HWND32 hWnd, const RECT32 *lpRect )
{
    RECT16 r;
    CONV_RECT32TO16( lpRect, &r );
    return TrackPopupMenu16( hMenu, wFlags, x, y, nReserved, hWnd, &r );
}


/***********************************************************************
 *           PopupMenuWndProc
 */
LRESULT PopupMenuWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{    
    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCT16 *cs = (CREATESTRUCT16*)PTR_SEG_TO_LIN(lParam);
#ifdef WINELIB32
	    HMENU hmenu = (HMENU) (cs->lpCreateParams);
	    SetWindowLong( hwnd, 0, hmenu );
#else
	    HMENU hmenu = (HMENU) ((int)cs->lpCreateParams & 0xffff);
	    SetWindowWord( hwnd, 0, hmenu );
#endif
	    return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
	return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT16 ps;
	    BeginPaint16( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc,
#ifdef WINELIB32
			        (HMENU)GetWindowLong( hwnd, 0 )
#else
			        (HMENU)GetWindowWord( hwnd, 0 )
#endif
 			       );
	    EndPaint16( hwnd, &ps );
	    return 0;
	}

    case WM_DESTROY:
	    /* zero out global pointer in case system popup
	     * was destroyed by AppExit 
	     */

	    if( hwnd == pTopPWnd->hwndSelf )
		pTopPWnd = 0;
	    else
		uSubPWndLevel--;
	    break;

    case WM_USER:
	if( wParam )
#ifdef WINELIB32
            SetWindowLong( hwnd, 0, (HMENU)wParam );
#else
            SetWindowWord( hwnd, 0, (HMENU)wParam );
#endif
        break;
    default:
	return DefWindowProc16(hwnd, message, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 *           MENU_GetMenuBarHeight
 *
 * Compute the size of the menu bar height. Used by NC_HandleNCCalcSize().
 */
UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth, int orgX, int orgY )
{
    HDC hdc;
    RECT16 rectBar;
    WND *wndPtr;
    LPPOPUPMENU lppop;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    if (!(lppop = (LPPOPUPMENU)USER_HEAP_LIN_ADDR((HMENU)wndPtr->wIDmenu)))
      return 0;
    hdc = GetDC( hwnd );
    SetRect16(&rectBar, orgX, orgY, orgX+menubarWidth, orgY+SYSMETRICS_CYMENU);
    MENU_MenuBarCalcSize( hdc, &rectBar, lppop, hwnd );
    ReleaseDC( hwnd, hdc );
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
    if (flags & MF_DELETE) return DeleteMenu( hMenu, pos, flags & ~MF_DELETE );
    if (flags & MF_CHANGE) return ModifyMenu16( hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
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
    if (flags & MF_DELETE) return DeleteMenu( hMenu, pos, flags & ~MF_DELETE );
    if (flags & MF_CHANGE) return ModifyMenu32A(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
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
    if (flags & MF_DELETE) return DeleteMenu( hMenu, pos, flags & ~MF_DELETE );
    if (flags & MF_CHANGE) return ModifyMenu32W(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu32W( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         CheckMenuItem    (USER.154)
 */
INT CheckMenuItem( HMENU hMenu, UINT id, UINT flags )
{
    MENUITEM *item;
    INT ret;

    dprintf_menu( stddeb,"CheckMenuItem: %04x %04x %04x\n", hMenu, id, flags );
    if (!(item = MENU_FindItem( &hMenu, &id, flags ))) return -1;
    ret = item->item_flags & MF_CHECKED;
    if (flags & MF_CHECKED) item->item_flags |= MF_CHECKED;
    else item->item_flags &= ~MF_CHECKED;
    return ret;
}


/**********************************************************************
 *			EnableMenuItem		[USER.155]
 */
BOOL EnableMenuItem(HMENU hMenu, UINT wItemID, UINT wFlags)
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
 *         GetMenuString    (USER.161)
 */
int GetMenuString( HMENU hMenu, UINT wItemID,
                   LPSTR str, short nMaxSiz, UINT wFlags )
{
    MENUITEM *item;

    dprintf_menu( stddeb, "GetMenuString: menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",
                 hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return 0;
    if (!IS_STRING_ITEM(item->item_flags)) return 0;
    lstrcpyn( str, item->text, nMaxSiz );
    dprintf_menu( stddeb, "GetMenuString: returning '%s'\n", str );
    return strlen(str);
}


/**********************************************************************
 *			HiliteMenuItem		[USER.162]
 */
BOOL HiliteMenuItem(HWND hWnd, HMENU hMenu, UINT wItemID, UINT wHilite)
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
 *			GetMenuState		[USER.250]
 */
UINT GetMenuState(HMENU hMenu, UINT wItemID, UINT wFlags)
{
    MENUITEM *item;
    dprintf_menu(stddeb,"GetMenuState(%04x, %04x, %04x);\n", 
		 hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    if (item->item_flags & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( (HMENU)item->item_id );
	if (!menu) return -1;
	else return (menu->nItems << 8) | (menu->wFlags & 0xff);
    }
    else return item->item_flags;
}


/**********************************************************************
 *			GetMenuItemCount		[USER.263]
 */
INT GetMenuItemCount(HMENU hMenu)
{
	LPPOPUPMENU	menu;
	dprintf_menu(stddeb,"GetMenuItemCount(%04x);\n", hMenu);
	menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
	if (menu == NULL) return (UINT)-1;
	dprintf_menu(stddeb,"GetMenuItemCount(%04x) return %d \n", 
		     hMenu, menu->nItems);
	return menu->nItems;
}


/**********************************************************************
 *			GetMenuItemID			[USER.264]
 */
UINT GetMenuItemID(HMENU hMenu, int nPos)
{
    LPPOPUPMENU	menu;

    dprintf_menu(stddeb,"GetMenuItemID(%04x, %d);\n", hMenu, nPos);
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
    if (IS_STRING_ITEM(flags) && data)
        return InsertMenu32A( hMenu, (INT32)(INT16)pos, flags, id,
                              (LPSTR)PTR_SEG_TO_LIN(data) );
    return InsertMenu32A( hMenu, (INT32)(INT16)pos, flags, id, (LPSTR)data );
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
        RemoveMenu( hMenu, pos, flags );
        return FALSE;
    }

    if (flags & MF_POPUP)  /* Set the MF_POPUP flag on the popup-menu */
	((POPUPMENU *)USER_HEAP_LIN_ADDR((HMENU)id))->wFlags |= MF_POPUP;

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
        LPSTR newstr = STRING32_DupUniToAnsi( str );
        ret = InsertMenu32A( hMenu, pos, flags, id, newstr );
        free( newstr );
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
 *			RemoveMenu		[USER.412]
 */
BOOL RemoveMenu(HMENU hMenu, UINT nPos, UINT wFlags)
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
 *			DeleteMenu		[USER.413]
 */
BOOL DeleteMenu(HMENU hMenu, UINT nPos, UINT wFlags)
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) return FALSE;
    if (item->item_flags & MF_POPUP) DestroyMenu( (HMENU)item->item_id );
      /* nPos is now the position of the item */
    RemoveMenu( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}


/*******************************************************************
 *         ModifyMenu16    (USER.414)
 */
BOOL16 ModifyMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                     UINT16 id, SEGPTR data )
{
    if (IS_STRING_ITEM(flags))
        return ModifyMenu32A( hMenu, (INT32)(INT16)pos, flags, id,
                              (LPSTR)PTR_SEG_TO_LIN(data) );
    return ModifyMenu32A( hMenu, (INT32)(INT16)pos, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         ModifyMenu32A    (USER32.396)
 */
BOOL32 ModifyMenu32A( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                      UINT32 id, LPCSTR str )
{
    MENUITEM *item;
    HMENU hMenu16 = hMenu;
    UINT16 pos16 = pos;

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

    if (!(item = MENU_FindItem( &hMenu16, &pos16, flags ))) return FALSE;
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
        LPSTR newstr = STRING32_DupUniToAnsi( str );
        ret = ModifyMenu32A( hMenu, pos, flags, id, newstr );
        free( newstr );
        return ret;
    }
    else return ModifyMenu32A( hMenu, pos, flags, id, (LPCSTR)str );
}


/**********************************************************************
 *			CreatePopupMenu		[USER.415]
 */
HMENU CreatePopupMenu()
{
    HMENU hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu())) return 0;
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
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
BOOL SetMenuItemBitmaps(HMENU hMenu, UINT nPos, UINT wFlags,
                        HBITMAP hNewUnCheck, HBITMAP hNewCheck)
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
 *			CreateMenu		[USER.151]
 */
HMENU CreateMenu()
{
    HMENU hMenu;
    LPPOPUPMENU menu;
    dprintf_menu(stddeb,"CreateMenu !\n");
    if (!(hMenu = USER_HEAP_ALLOC( sizeof(POPUPMENU) )))
	return 0;
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
    dprintf_menu(stddeb,"CreateMenu // return %04x\n", hMenu);
    return hMenu;
}


/**********************************************************************
 *			DestroyMenu		[USER.152]
 */
BOOL DestroyMenu(HMENU hMenu)
{
    LPPOPUPMENU lppop;
    dprintf_menu(stddeb,"DestroyMenu (%04x) !\n", hMenu);

    if (hMenu == 0) return FALSE;
    /* Silently ignore attempts to destroy default system menu */
    if (hMenu == MENU_DefSysMenu) return TRUE;
    lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!lppop || (lppop->wMagic != MENU_MAGIC)) return FALSE;
    lppop->wMagic = 0;  /* Mark it as destroyed */
    if ((lppop->wFlags & MF_POPUP) && lppop->hWnd && lppop->hWnd != pTopPWnd->hwndSelf )
        DestroyWindow( lppop->hWnd );

    if (lppop->items)
    {
        int i;
        MENUITEM *item = lppop->items;
        for (i = lppop->nItems; i > 0; i--, item++)
        {
            if (item->item_flags & MF_POPUP)
                DestroyMenu( (HMENU)item->item_id );
	    if (IS_STRING_ITEM(item->item_flags) && item->text)
                HeapFree( SystemHeap, 0, item->text );
        }
        HeapFree( SystemHeap, 0, lppop->items );
    }
    USER_HEAP_FREE( hMenu );
    dprintf_menu(stddeb,"DestroyMenu (%04x) // End !\n", hMenu);
    return TRUE;
}

/**********************************************************************
 *			GetSystemMenu		[USER.156]
 */
HMENU GetSystemMenu(HWND hWnd, BOOL bRevert)
{
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    if (!wndPtr) return 0;

    if (!wndPtr->hSysMenu || (wndPtr->hSysMenu == MENU_DefSysMenu))
    {
        wndPtr->hSysMenu = MENU_CopySysMenu();
        return wndPtr->hSysMenu;
    }
    if (!bRevert) return wndPtr->hSysMenu;
    if (wndPtr->hSysMenu) DestroyMenu(wndPtr->hSysMenu);
    wndPtr->hSysMenu = MENU_CopySysMenu();
    return wndPtr->hSysMenu;
}


/*******************************************************************
 *         SetSystemMenu    (USER.280)
 */
BOOL SetSystemMenu( HWND hwnd, HMENU hMenu )
{
    WND *wndPtr;

    if (!(wndPtr = WIN_FindWndPtr(hwnd))) return FALSE;
    if (wndPtr->hSysMenu && (wndPtr->hSysMenu != MENU_DefSysMenu))
        DestroyMenu( wndPtr->hSysMenu );
    wndPtr->hSysMenu = hMenu;
    return TRUE;
}


/**********************************************************************
 *			GetMenu		[USER.157]
 */
HMENU GetMenu(HWND hWnd) 
{ 
	WND * wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) return 0;
	return (HMENU)wndPtr->wIDmenu;
}


/**********************************************************************
 * 			SetMenu 	[USER.158]
 */
BOOL SetMenu(HWND hWnd, HMENU hMenu)
{
	LPPOPUPMENU lpmenu;
	WND * wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr == NULL) {
		fprintf(stderr,"SetMenu(%04x, %04x) // Bad window handle !\n",
			hWnd, hMenu);
		return FALSE;
		}
	dprintf_menu(stddeb,"SetMenu(%04x, %04x);\n", hWnd, hMenu);
	if (GetCapture() == hWnd) ReleaseCapture();
	wndPtr->wIDmenu = (UINT)hMenu;
	if (hMenu != 0)
	{
	    lpmenu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
	    if (lpmenu == NULL) {
		fprintf(stderr,"SetMenu(%04x, %04x) // Bad menu handle !\n", 
			hWnd, hMenu);
		return FALSE;
		}
	    lpmenu->hWnd = hWnd;
	    lpmenu->wFlags &= ~MF_POPUP;  /* Can't be a popup */
	    lpmenu->Height = 0;  /* Make sure we recalculate the size */
	}
        if (IsWindowVisible(hWnd))
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

    dprintf_menu(stddeb,"GetSubMenu (%04x, %04X) !\n", hMenu, nPos);
    if (!(lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return 0;
    if ((UINT)nPos >= lppop->nItems) return 0;
    if (!(lppop->items[nPos].item_flags & MF_POPUP)) return 0;
    return (HMENU)lppop->items[nPos].item_id;
}


/**********************************************************************
 *			DrawMenuBar		[USER.160]
 */
void DrawMenuBar(HWND hWnd)
{
	WND		*wndPtr;
	LPPOPUPMENU lppop;
	dprintf_menu(stddeb,"DrawMenuBar (%04x)\n", hWnd);
	wndPtr = WIN_FindWndPtr(hWnd);
	if (wndPtr != NULL && (wndPtr->dwStyle & WS_CHILD) == 0 && 
		wndPtr->wIDmenu != 0) {
		dprintf_menu(stddeb,"DrawMenuBar wIDmenu=%04X \n", 
			     wndPtr->wIDmenu);
		lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR((HMENU)wndPtr->wIDmenu);
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
    /* FIXME: this won't work when we have multiple tasks... */
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
 *	    LoadMenu    (USER.150)
 */
HMENU LoadMenu( HINSTANCE instance, SEGPTR name )
{
    HRSRC hRsrc;
    HGLOBAL handle;
    HMENU hMenu;

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
        return WIN32_LoadMenuA(instance,PTR_SEG_TO_LIN(name));

    if (!(hRsrc = FindResource( instance, name, RT_MENU ))) return 0;
    if (!(handle = LoadResource( instance, hRsrc ))) return 0;
    hMenu = LoadMenuIndirect16( LockResource(handle) );
    FreeResource( handle );
    return hMenu;
}


/**********************************************************************
 *	    LoadMenuIndirect16    (USER.220)
 */
HMENU16 LoadMenuIndirect16( LPCVOID template )
{
    HMENU hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    dprintf_menu(stddeb,"LoadMenuIndirect32A: %p\n", template );
    version = GET_WORD(p);
    p += sizeof(WORD);
    if (version)
    {
        fprintf( stderr, "LoadMenuIndirect16: version must be 0 for Win16\n" );
        return 0;
    }
    offset = GET_WORD(p);
    p += sizeof(WORD) + offset;
    if (!(hMenu = CreateMenu())) return 0;
    if (!MENU_ParseResource( p, hMenu, FALSE ))
    {
        DestroyMenu( hMenu );
        return 0;
    }
    return hMenu;
}


/**********************************************************************
 *	    LoadMenuIndirect32A    (USER32.370)
 */
HMENU32 LoadMenuIndirect32A( LPCVOID template )
{
    HMENU hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    dprintf_menu(stddeb,"LoadMenuIndirect32A: %p\n", template );
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
    if (!(hMenu = CreateMenu())) return 0;
    if (!MENU_ParseResource( p, hMenu, TRUE ))
    {
        DestroyMenu( hMenu );
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
 *		IsMenu    (USER.358)
 */
BOOL IsMenu( HMENU hmenu )
{
    LPPOPUPMENU menu;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    return (menu->wMagic == MENU_MAGIC);
}
