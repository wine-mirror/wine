/*
 * Menu functions
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 */

/*
 * Note: the style MF_MOUSESELECT is used to mark popup items that
 * have been selected, i.e. their popup menu is currently displayed.
 * This is probably not the meaning this style has in MS-Windows.
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "bitmap.h"
#include "gdi.h"
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
#include "tweak.h"
#include "debug.h"


UINT32  MENU_BarItemTopNudge;
UINT32  MENU_BarItemLeftNudge;
UINT32  MENU_ItemTopNudge;
UINT32  MENU_ItemLeftNudge;
UINT32  MENU_HighlightTopNudge;
UINT32  MENU_HighlightLeftNudge;
UINT32  MENU_HighlightBottomNudge;
UINT32  MENU_HighlightRightNudge;

/* internal popup menu window messages */

#define MM_SETMENUHANDLE	(WM_USER + 0)
#define MM_GETMENUHANDLE	(WM_USER + 1)

/* Menu item structure */
typedef struct {
    /* ----------- MENUITEMINFO Stuff ----------- */
    UINT32 fType;		/* Item type. */
    UINT32 fState;		/* Item state.  */
    UINT32 wID;			/* Item id.  */
    HMENU32 hSubMenu;		/* Pop-up menu.  */
    HBITMAP32 hCheckBit;	/* Bitmap when checked.  */
    HBITMAP32 hUnCheckBit;	/* Bitmap when unchecked.  */
    LPSTR text;			/* Item text or bitmap handle.  */
    DWORD dwItemData;		/* Application defined.  */
    /* ----------- Wine stuff ----------- */
    RECT32      rect;          /* Item area (relative to menu window) */
    UINT32      xTab;          /* X position of text after Tab */
} MENUITEM;

/* Popup menu structure */
typedef struct {
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD        wMagic;       /* Magic number */
    HQUEUE16    hTaskQ;       /* Task queue for this menu */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    WORD	nItems;       /* Number of items in the menu */
    HWND32      hWnd;         /* Window containing the menu */
    MENUITEM   *items;        /* Array of menu items */
    UINT32      FocusedItem;  /* Currently focused item */
    WORD	defitem;      /* default item position. Unused (except for set/get)*/
} POPUPMENU, *LPPOPUPMENU;

/* internal flags for menu tracking */

#define TF_ENDMENU              0x0001
#define TF_SUSPENDPOPUP         0x0002
#define TF_SKIPREMOVE		0x0004

typedef struct
{
    UINT32	trackFlags;
    HMENU32	hCurrentMenu; /* current submenu (can be equal to hTopMenu)*/
    HMENU32	hTopMenu;     /* initial menu */
    HWND32	hOwnerWnd;    /* where notifications are sent */
    POINT32	pt;
} MTRACKER;

#define MENU_MAGIC   0x554d  /* 'MU' */
#define IS_A_MENU(pmenu) ((pmenu) && (pmenu)->wMagic == MENU_MAGIC)

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

  /* Internal MENU_TrackMenu() flags */
#define TPM_INTERNAL		0xF0000000
#define TPM_ENTERIDLEEX	 	0x80000000		/* set owner window for WM_ENTERIDLE */
#define TPM_BUTTONDOWN		0x40000000		/* menu was clicked before tracking */

  /* popup menu shade thickness */
#define POPUP_XSHADE		4
#define POPUP_YSHADE		4

  /* Space between 2 menu bar items */
#define MENU_BAR_ITEMS_SPACE 12

  /* Minimum width of a tab character */
#define MENU_TAB_SPACE 8

  /* Height of a separator item */
#define SEPARATOR_HEIGHT 5

  /* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)

#define IS_SYSTEM_MENU(menu)  \
	(!((menu)->wFlags & MF_POPUP) && (menu)->wFlags & MF_SYSMENU)
#define IS_SYSTEM_POPUP(menu) \
	((menu)->wFlags & MF_POPUP && (menu)->wFlags & MF_SYSMENU)

#define TYPE_MASK (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
		   MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
		   MFT_RIGHTORDER | MFT_RIGHTJUSTIFY | \
		   MF_POPUP | MF_SYSMENU | MF_HELP)
#define STATE_MASK (~TYPE_MASK)

  /* Dimension of the menu bitmaps */
static WORD check_bitmap_width = 0, check_bitmap_height = 0;
static WORD arrow_bitmap_width = 0, arrow_bitmap_height = 0;

static HBITMAP32 hStdRadioCheck = 0;
static HBITMAP32 hStdCheck = 0;
static HBITMAP32 hStdMnArrow = 0;
static HBRUSH32 hShadeBrush = 0;
static HMENU32 MENU_DefSysPopup = 0;  /* Default system menu popup */

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */ 

static WND* pTopPopupWnd   = 0;
static UINT32 uSubPWndLevel = 0;

  /* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL32 fEndMenu = FALSE;


/***********************************************************************
 *           debug_print_menuitem
 *
 * Print a menuitem in readable form.
 */

#define debug_print_menuitem(pre, mp, post) \
  if(!TRACE_ON(menu)) ; else do_debug_print_menuitem(pre, mp, post)

#define MENUOUT(text) \
  dsprintf(menu, "%s%s", (count++ ? "," : ""), (text))

#define MENUFLAG(bit,text) \
  do { \
    if (flags & (bit)) { flags &= ~(bit); MENUOUT ((text)); } \
  } while (0)

static void do_debug_print_menuitem(const char *prefix, MENUITEM * mp, 
				    const char *postfix)
{
    dbg_decl_str(menu, 256);

    if (mp) {
	UINT32 flags = mp->fType;
	int typ = MENU_ITEM_TYPE(flags);
	dsprintf(menu, "{ ID=0x%x", mp->wID);
	if (flags & MF_POPUP)
	    dsprintf(menu, ", Sub=0x%x", mp->hSubMenu);
	if (flags) {
	    int count = 0;
	    dsprintf(menu, ", Typ=");
	    if (typ == MFT_STRING)
		/* Nothing */ ;
	    else if (typ == MFT_SEPARATOR)
		MENUOUT("sep");
	    else if (typ == MFT_OWNERDRAW)
		MENUOUT("own");
	    else if (typ == MFT_BITMAP)
		MENUOUT("bit");
	    else
		MENUOUT("???");
	    flags -= typ;

	    MENUFLAG(MF_POPUP, "pop");
	    MENUFLAG(MFT_MENUBARBREAK, "barbrk");
	    MENUFLAG(MFT_MENUBREAK, "brk");
	    MENUFLAG(MFT_RADIOCHECK, "radio");
	    MENUFLAG(MFT_RIGHTORDER, "rorder");
	    MENUFLAG(MF_SYSMENU, "sys");
	    MENUFLAG(MFT_RIGHTJUSTIFY, "right");

	    if (flags)
		dsprintf(menu, "+0x%x", flags);
	}
	flags = mp->fState;
	if (flags) {
	    int count = 0;
	    dsprintf(menu, ", State=");
	    MENUFLAG(MFS_GRAYED, "grey");
	    MENUFLAG(MFS_DISABLED, "dis");
	    MENUFLAG(MFS_CHECKED, "check");
	    MENUFLAG(MFS_HILITE, "hi");
	    MENUFLAG(MF_USECHECKBITMAPS, "usebit");
	    MENUFLAG(MF_MOUSESELECT, "mouse");
	    if (flags)
		dsprintf(menu, "+0x%x", flags);
	}
	if (mp->hCheckBit)
	    dsprintf(menu, ", Chk=0x%x", mp->hCheckBit);
	if (mp->hUnCheckBit)
	    dsprintf(menu, ", Unc=0x%x", mp->hUnCheckBit);

	if (typ == MFT_STRING) {
	    if (mp->text)
		dsprintf(menu, ", Text=\"%s\"", mp->text);
	    else
		dsprintf(menu, ", Text=Null");
	} else if (mp->text == NULL)
	    /* Nothing */ ;
	else
	    dsprintf(menu, ", Text=%p", mp->text);
	dsprintf(menu, " }");
    } else {
	dsprintf(menu, "NULL");
    }

    TRACE(menu, "%s %s %s\n", prefix, dbg_str(menu), postfix);
}

#undef MENUOUT
#undef MENUFLAG

/***********************************************************************
 *           MENU_CopySysPopup
 *
 * Return the default system menu.
 */
static HMENU32 MENU_CopySysPopup(void)
{
    HMENU32 hMenu = LoadMenuIndirect32A(SYSRES_GetResPtr(SYSRES_MENU_SYSMENU));

    if( hMenu ) {
        POPUPMENU* menu = (POPUPMENU *) USER_HEAP_LIN_ADDR(hMenu);
        menu->wFlags |= MF_SYSMENU | MF_POPUP;
    }
    else {
	hMenu = 0;
	ERR(menu, "Unable to load default system menu\n" );
    }

    TRACE(menu, "returning %x.\n", hMenu );

    return hMenu;
}


/**********************************************************************
 *           MENU_GetSysMenu
 *
 * Create a copy of the system menu. System menu in Windows is
 * a special menu-bar with the single entry - system menu popup.
 * This popup is presented to the outside world as a "system menu". 
 * However, the real system menu handle is sometimes seen in the 
 * WM_MENUSELECT paramemters (and Word 6 likes it this way).
 */
HMENU32 MENU_GetSysMenu( HWND32 hWnd, HMENU32 hPopupMenu )
{
    HMENU32 hMenu;

    if ((hMenu = CreateMenu32()))
    {
	POPUPMENU *menu = (POPUPMENU*) USER_HEAP_LIN_ADDR(hMenu);
	menu->wFlags = MF_SYSMENU;
	menu->hWnd = hWnd;

	if (hPopupMenu == (HMENU32)(-1))
	    hPopupMenu = MENU_CopySysPopup();
	else if( !hPopupMenu ) hPopupMenu = MENU_DefSysPopup; 

	if (hPopupMenu)
	{
	    InsertMenu32A( hMenu, -1, MF_SYSMENU | MF_POPUP | MF_BYPOSITION, hPopupMenu, NULL );

            menu->items[0].fType = MF_SYSMENU | MF_POPUP;
            menu->items[0].fState = 0;
	    menu = (POPUPMENU*) USER_HEAP_LIN_ADDR(hPopupMenu);
	    menu->wFlags |= MF_SYSMENU;

	    TRACE(menu,"GetSysMenu hMenu=%04x (%04x)\n", hMenu, hPopupMenu );
	    return hMenu;
	}
	DestroyMenu32( hMenu );
    }
    ERR(menu, "failed to load system menu!\n");
    return 0;
}


/***********************************************************************
 *           MENU_Init
 *
 * Menus initialisation.
 */
BOOL32 MENU_Init()
{
    HBITMAP32 hBitmap;
    static unsigned char shade_bits[16] = { 0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0,
					    0x55, 0, 0xAA, 0 };

    /* Load menu bitmaps */
    hStdCheck = LoadBitmap32A(0, MAKEINTRESOURCE32A(OBM_CHECK));
    hStdRadioCheck = LoadBitmap32A(0, MAKEINTRESOURCE32A(OBM_RADIOCHECK));
    hStdMnArrow = LoadBitmap32A(0, MAKEINTRESOURCE32A(OBM_MNARROW));

    if (hStdCheck)
    {
	BITMAP32 bm;
	GetObject32A( hStdCheck, sizeof(bm), &bm );
	check_bitmap_width = bm.bmWidth;
	check_bitmap_height = bm.bmHeight;
    } else
	 return FALSE;

    /* Assume that radio checks have the same size as regular check.  */
    if (!hStdRadioCheck)
	 return FALSE;

    if (hStdMnArrow)
	{
	 BITMAP32 bm;
	    GetObject32A( hStdMnArrow, sizeof(bm), &bm );
	    arrow_bitmap_width = bm.bmWidth;
	    arrow_bitmap_height = bm.bmHeight;
    } else
	 return FALSE;

    if ((hBitmap = CreateBitmap32( 8, 8, 1, 1, shade_bits)))
	    {
		if((hShadeBrush = CreatePatternBrush32( hBitmap )))
		{
		    DeleteObject32( hBitmap );
	      if ((MENU_DefSysPopup = MENU_CopySysPopup()))
		   return TRUE;
	}
    }

    return FALSE;
}

/***********************************************************************
 *           MENU_InitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
static void MENU_InitSysMenuPopup( HMENU32 hmenu, DWORD style, DWORD clsStyle )
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


/******************************************************************************
 *
 *   UINT32  MENU_GetStartOfNextColumn(
 *     HMENU32  hMenu )
 *
 *****************************************************************************/

static UINT32  MENU_GetStartOfNextColumn(
    HMENU32  hMenu )
{
    POPUPMENU  *menu = (POPUPMENU *)USER_HEAP_LIN_ADDR(hMenu);
    UINT32  i = menu->FocusedItem + 1;

    if(!menu)
	return NO_SELECTED_ITEM;

    if( i == NO_SELECTED_ITEM )
	return i;

    for( ; i < menu->nItems; ++i ) {
	if (menu->items[i].fType & MF_MENUBARBREAK)
	    return i;
    }

    return NO_SELECTED_ITEM;
}


/******************************************************************************
 *
 *   UINT32  MENU_GetStartOfPrevColumn(
 *     HMENU32  hMenu )
 *
 *****************************************************************************/

static UINT32  MENU_GetStartOfPrevColumn(
    HMENU32  hMenu )
{
    POPUPMENU const  *menu = (POPUPMENU *)USER_HEAP_LIN_ADDR(hMenu);
    UINT32  i;

    if( !menu )
	return NO_SELECTED_ITEM;

    if( menu->FocusedItem == 0 || menu->FocusedItem == NO_SELECTED_ITEM )
	return NO_SELECTED_ITEM;

    /* Find the start of the column */

    for(i = menu->FocusedItem; i != 0 &&
	 !(menu->items[i].fType & MF_MENUBARBREAK);
	--i); /* empty */

    if(i == 0)
	return NO_SELECTED_ITEM;

    for(--i; i != 0; --i) {
	if (menu->items[i].fType & MF_MENUBARBREAK)
	    break;
    }

    TRACE(menu, "ret %d.\n", i );

    return i;
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
	    if (item->wID == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	    else if (item->fType & MF_POPUP)
	    {
		HMENU32 hsubmenu = item->hSubMenu;
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
 *           MENU_FreeItemData
 */
static void MENU_FreeItemData( MENUITEM* item )
{
    /* delete text */
    if (IS_STRING_ITEM(item->fType) && item->text)
        HeapFree( SystemHeap, 0, item->text );
}

/***********************************************************************
 *           MENU_FindItemByCoords
 *
 * Find the item at the specified coordinates (screen coords). Does 
 * not work for child windows and therefore should not be called for 
 * an arbitrary system menu.
 */
static MENUITEM *MENU_FindItemByCoords( POPUPMENU *menu, 
					POINT32 pt, UINT32 *pos )
{
    MENUITEM *item;
    WND *wndPtr;
    UINT32 i;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd ))) return NULL;
    pt.x -= wndPtr->rectWindow.left;
    pt.y -= wndPtr->rectWindow.top;
    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
	if ((pt.x >= item->rect.left) && (pt.x < item->rect.right) &&
	    (pt.y >= item->rect.top) && (pt.y < item->rect.bottom))
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
static UINT32 MENU_FindItemByKey( HWND32 hwndOwner, HMENU32 hmenu, 
				  UINT32 key, BOOL32 forceMenuChar )
{
    TRACE(menu,"\tlooking for '%c' in [%04x]\n", (char)key, (UINT16)hmenu );

    if (!IsMenu32( hmenu )) 
    {
	WND* w = WIN_FindWndPtr(hwndOwner);
	hmenu = GetSubMenu32(w->hSysMenu, 0);
    }

    if (hmenu)
    {
	POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
	MENUITEM *item = menu->items;
	LONG menuchar;

	if( !forceMenuChar )
	{
	     UINT32 i;

	     key = toupper(key);
	     for (i = 0; i < menu->nItems; i++, item++)
	     {
		if (item->text && (IS_STRING_ITEM(item->fType)))
		{
		    char *p = strchr( item->text, '&' );
		    if (p && (p[1] != '&') && (toupper(p[1]) == key)) return i;
		}
	     }
	}
	menuchar = SendMessage32A( hwndOwner, WM_MENUCHAR, 
                                   MAKEWPARAM( key, menu->wFlags ), hmenu );
	if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
	if (HIWORD(menuchar) == 1) return (UINT32)(-2);
    }
    return (UINT32)(-1);
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

    TRACE(menu, "HDC 0x%x at (%d,%d)\n",
                 hdc, orgX, orgY);
    debug_print_menuitem("MENU_CalcItemSize: menuitem:", lpitem, 
			 (menuBar ? " (MenuBar)" : ""));

    SetRect32( &lpitem->rect, orgX, orgY, orgX, orgY );

    if (lpitem->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT32 mis;
        mis.CtlType    = ODT_MENU;
        mis.itemID     = lpitem->wID;
        mis.itemData   = (DWORD)lpitem->text;
        mis.itemHeight = 16;
        mis.itemWidth  = 30;
        SendMessage32A( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        lpitem->rect.bottom += mis.itemHeight;
        lpitem->rect.right  += mis.itemWidth;
        TRACE(menu, "%08x %dx%d\n",
                     lpitem->wID, mis.itemWidth, mis.itemHeight);
        return;
    } 

    if (lpitem->fType & MF_SEPARATOR)
    {
	lpitem->rect.bottom += SEPARATOR_HEIGHT;
	return;
    }

    if (!menuBar)
    {
	lpitem->rect.right += 2 * check_bitmap_width;
	if (lpitem->fType & MF_POPUP)
	    lpitem->rect.right += arrow_bitmap_width;
    }

    if (lpitem->fType & MF_BITMAP)
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

    if (IS_STRING_ITEM( lpitem->fType ))
    {
        dwSize = GetTextExtent( hdc, lpitem->text, strlen(lpitem->text) );
        lpitem->rect.right  += LOWORD(dwSize);
	if (TWEAK_Win95Look)
            lpitem->rect.bottom += MAX (HIWORD(dwSize), sysMetrics[SM_CYMENU]- 1);
        else
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
    start = 0;
    maxX = SYSMETRICS_CXBORDER;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = maxX;
	orgY = SYSMETRICS_CYBORDER;

	maxTab = maxTabWidth = 0;

	  /* Parse items until column break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    if(TWEAK_Win95Look)
		++orgY;

	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, FALSE );
            if (lpitem->fType & MF_MENUBARBREAK) orgX++;
	    maxX = MAX( maxX, lpitem->rect.right );
	    orgY = lpitem->rect.bottom;
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
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
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
                lpitem->xTab = maxTab;
	}
	lppop->Height = MAX( lppop->Height, orgY );
    }

    if(TWEAK_Win95Look)
	lppop->Height++;

    lppop->Width  = maxX;
    ReleaseDC32( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * FIXME: Word 6 implements its own MDI and its own 'close window' bitmap
 * height is off by 1 pixel which causes lengthy window relocations when
 * active document window is maximized/restored.
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
    TRACE(menu,"MENU_MenuBarCalcSize left=%d top=%d right=%d bottom=%d\n", 
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
	    if ((helpPos == -1) && (lpitem->fType & MF_HELP)) helpPos = i;
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    TRACE(menu, "calling MENU_CalcItemSize org=(%d, %d)\n", 
			 orgX, orgY );
	    debug_print_menuitem ("  item: ", lpitem, "");
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
			       UINT32 height, BOOL32 menuBar, UINT32 odaction )
{
    RECT32 rect;

    debug_print_menuitem("MENU_DrawMenuItem: ", lpitem, "");

    if (lpitem->fType & MF_SYSMENU)
    {
	if( !IsIconic32(hwnd) ) {
	    if(TWEAK_Win95Look)
		NC_DrawSysButton95( hwnd, hdc,
				    lpitem->fState &
				    (MF_HILITE | MF_MOUSESELECT) );
	    else
		NC_DrawSysButton( hwnd, hdc, 
				  lpitem->fState &
				  (MF_HILITE | MF_MOUSESELECT) );
	}

	return;
    }

    if (lpitem->fType & MF_OWNERDRAW)
    {
        DRAWITEMSTRUCT32 dis;

        dis.CtlType   = ODT_MENU;
        dis.itemID    = lpitem->wID;
        dis.itemData  = (DWORD)lpitem->text;
        dis.itemState = 0;
        if (lpitem->fState & MF_CHECKED) dis.itemState |= ODS_CHECKED;
        if (lpitem->fState & MF_GRAYED)  dis.itemState |= ODS_GRAYED;
        if (lpitem->fState & MF_HILITE)  dis.itemState |= ODS_SELECTED;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = hwnd;
        dis.hDC        = hdc;
        dis.rcItem     = lpitem->rect;
        TRACE(menu, "Ownerdraw: itemID=%d, itemState=%d, itemAction=%d, "
	      "hwndItem=%04x, hdc=%04x, rcItem={%d,%d,%d,%d}\n",dis.itemID,
	      dis.itemState, dis.itemAction, dis.hwndItem, dis.hDC,
	      dis.rcItem.left, dis.rcItem.top, dis.rcItem.right,
	      dis.rcItem.bottom );
        SendMessage32A( GetWindow32(hwnd,GW_OWNER), WM_DRAWITEM, 0, (LPARAM)&dis );
        return;
    }

    if (menuBar && (lpitem->fType & MF_SEPARATOR)) return;
    rect = lpitem->rect;

    /* Draw the background */
    if(TWEAK_Win95Look) {
	rect.left += 2;
	rect.right -= 2;

        /*
	if(menuBar) {
	    --rect.left;
	    ++rect.bottom;
	    --rect.top;
	}
	InflateRect32( &rect, -1, -1 );
	*/
    }

    if (lpitem->fState & MF_HILITE)
	FillRect32( hdc, &rect, GetSysColorBrush32(COLOR_HIGHLIGHT) );
    else
	FillRect32( hdc, &rect, GetSysColorBrush32(COLOR_MENU) );

    SetBkMode32( hdc, TRANSPARENT );

      /* Draw the separator bar (if any) */

    if (!menuBar && (lpitem->fType & MF_MENUBARBREAK))
    {
	if(TWEAK_Win95Look)
	    TWEAK_DrawMenuSeparatorVert95(hdc, rect.left - 1, 3, height - 3);
	else {
	    SelectObject32( hdc, GetSysColorPen32(COLOR_WINDOWFRAME) );
	    MoveTo( hdc, rect.left, 0 );
	    LineTo32( hdc, rect.left, height );
	}
    }
    if (lpitem->fType & MF_SEPARATOR)
    {
	if(TWEAK_Win95Look)
	    TWEAK_DrawMenuSeparatorHoriz95(hdc, rect.left + 1,
					   rect.top + SEPARATOR_HEIGHT / 2 + 1,
					   rect.right - 1);
	else {
	    SelectObject32( hdc, GetSysColorPen32(COLOR_WINDOWFRAME) );
	    MoveTo( hdc, rect.left, rect.top + SEPARATOR_HEIGHT/2 );
	    LineTo32( hdc, rect.right, rect.top + SEPARATOR_HEIGHT/2 );
	}

	return;
    }

      /* Setup colors */

    if (lpitem->fState & MF_HILITE)
    {
	if (lpitem->fState & MF_GRAYED)
	    SetTextColor32( hdc, GetSysColor32( COLOR_GRAYTEXT ) );
	else
	    SetTextColor32( hdc, GetSysColor32( COLOR_HIGHLIGHTTEXT ) );
	SetBkColor32( hdc, GetSysColor32( COLOR_HIGHLIGHT ) );
    }
    else
    {
	if (lpitem->fState & MF_GRAYED)
	    SetTextColor32( hdc, GetSysColor32( COLOR_GRAYTEXT ) );
	else
	    SetTextColor32( hdc, GetSysColor32( COLOR_MENUTEXT ) );
	SetBkColor32( hdc, GetSysColor32( COLOR_MENU ) );
    }

    if (!menuBar)
    {
	INT32	y = rect.top + rect.bottom;

	  /* Draw the check mark
	   *
	   * Custom checkmark bitmaps are monochrome but not always 1bpp. 
	   * In this case we want GRAPH_DrawBitmap() to copy a plane which 
	   * is 1 for a white pixel and 0 for a black one. 
	   */

	if (lpitem->fState & MF_CHECKED)
	{
	    HBITMAP32 bm =
		 lpitem->hCheckBit ? lpitem->hCheckBit :
		 ((lpitem->fType & MFT_RADIOCHECK)
		  ? hStdRadioCheck : hStdCheck);
            GRAPH_DrawBitmap( hdc, bm, rect.left,
			      (y - check_bitmap_height) / 2,
			      0, 0, check_bitmap_width,
			      check_bitmap_height, TRUE );
        } else if (lpitem->hUnCheckBit)
            GRAPH_DrawBitmap( hdc, lpitem->hUnCheckBit, rect.left,
			      (y - check_bitmap_height) / 2, 0, 0,
			      check_bitmap_width, check_bitmap_height, TRUE );

	  /* Draw the popup-menu arrow */

	if (lpitem->fType & MF_POPUP)
	{
	    GRAPH_DrawBitmap( hdc, hStdMnArrow,
			      rect.right-arrow_bitmap_width-1,
			      (y - arrow_bitmap_height) / 2, 0, 0, 
			      arrow_bitmap_width, arrow_bitmap_height, FALSE );
	}

	rect.left += check_bitmap_width;
	rect.right -= arrow_bitmap_width;
    }

      /* Draw the item text or bitmap */

    if (lpitem->fType & MF_BITMAP)
    {
	GRAPH_DrawBitmap( hdc, (HBITMAP32)lpitem->text,
                          rect.left, rect.top, 0, 0,
                          rect.right-rect.left, rect.bottom-rect.top, FALSE );
	return;
    }
    /* No bitmap - process text if present */
    else if (IS_STRING_ITEM(lpitem->fType))
    {
	register int i;

	if (menuBar)
	{
	    rect.left += MENU_BAR_ITEMS_SPACE / 2;
	    rect.right -= MENU_BAR_ITEMS_SPACE / 2;
	    i = strlen( lpitem->text );

	    rect.top += MENU_BarItemTopNudge;
	    rect.left += MENU_BarItemLeftNudge;
	}
	else
	{
	    for (i = 0; lpitem->text[i]; i++)
                if ((lpitem->text[i] == '\t') || (lpitem->text[i] == '\b'))
                    break;

	    rect.top += MENU_ItemTopNudge;
	    rect.left += MENU_ItemLeftNudge;
	}

	if(!TWEAK_Win95Look || !(lpitem->fState & MF_GRAYED)) {
	    DrawText32A( hdc, lpitem->text, i, &rect,
			 DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}
	else {
	    if (!(lpitem->fState & MF_HILITE))
            {
		++rect.left;
		++rect.top;
		++rect.right;
		++rect.bottom;
		SetTextColor32(hdc, RGB(0xff, 0xff, 0xff));
		DrawText32A( hdc, lpitem->text, i, &rect,
			     DT_LEFT | DT_VCENTER | DT_SINGLELINE );
		--rect.left;
		--rect.top;
		--rect.right;
		--rect.bottom;
	    }
	    SetTextColor32(hdc, RGB(0x80, 0x80, 0x80));
	    DrawText32A( hdc, lpitem->text, i, &rect,
			 DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}

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
    HBRUSH32 hPrevBrush = 0;
    RECT32 rect;

    GetClientRect32( hwnd, &rect );

/*    if(!TWEAK_Win95Look) { */
	rect.bottom -= POPUP_YSHADE * SYSMETRICS_CYBORDER;
	rect.right -= POPUP_XSHADE * SYSMETRICS_CXBORDER;
/*    } */

    if((hPrevBrush = SelectObject32( hdc, GetSysColorBrush32(COLOR_MENU) )))
    {
	HPEN32 hPrevPen;
	
	Rectangle32( hdc, rect.left, rect.top, rect.right, rect.bottom );

	hPrevPen = SelectObject32( hdc, GetStockObject32( NULL_PEN ) );
	if( hPrevPen )
	{
	    INT32 ropPrev, i;
	    POPUPMENU *menu;

	    /* draw 3-d shade */
	    if(!TWEAK_Win95Look) {
		SelectObject32( hdc, hShadeBrush );
		SetBkMode32( hdc, TRANSPARENT );
		ropPrev = SetROP232( hdc, R2_MASKPEN );

		i = rect.right;		/* why SetBrushOrg() doesn't? */
		PatBlt32( hdc, i & 0xfffffffe,
			  rect.top + POPUP_YSHADE*SYSMETRICS_CYBORDER, 
			  i%2 + POPUP_XSHADE*SYSMETRICS_CXBORDER,
			  rect.bottom - rect.top, 0x00a000c9 );
		i = rect.bottom;
		PatBlt32( hdc, rect.left + POPUP_XSHADE*SYSMETRICS_CXBORDER,
			  i & 0xfffffffe,rect.right - rect.left,
			  i%2 + POPUP_YSHADE*SYSMETRICS_CYBORDER, 0x00a000c9 );
		SelectObject32( hdc, hPrevPen );
		SelectObject32( hdc, hPrevBrush );
		SetROP232( hdc, ropPrev );
	    }
	    else
		TWEAK_DrawReliefRect95(hdc, &rect);

	    /* draw menu items */

	    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
	    if (menu && menu->nItems)
	    {
		MENUITEM *item;
		UINT32 u;

		for (u = menu->nItems, item = menu->items; u > 0; u--, item++)
		    MENU_DrawMenuItem( hwnd, hdc, item, menu->Height, FALSE,
				       ODA_DRAWENTIRE );

	    }
	} else SelectObject32( hdc, hPrevBrush );
    }
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
    TRACE(menu,"(%04x, %p, %p); !\n", 
		 hDC, lprect, lppop);
    if (lppop->Height == 0) MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);
    lprect->bottom = lprect->top + lppop->Height;
    if (suppress_draw) return lppop->Height;
    
    FillRect32(hDC, lprect, GetSysColorBrush32(COLOR_MENU) );

    if(!TWEAK_Win95Look) {
	SelectObject32( hDC, GetSysColorPen32(COLOR_WINDOWFRAME) );
	MoveTo( hDC, lprect->left, lprect->bottom );
	LineTo32( hDC, lprect->right, lprect->bottom );
    }
    else {
	SelectObject32( hDC, GetSysColorPen32(COLOR_3DFACE));
	MoveTo( hDC, lprect->left, lprect->bottom );
	LineTo32( hDC, lprect->right, lprect->bottom );
    }

    if (lppop->nItems == 0) return SYSMETRICS_CYMENU;
    for (i = 0; i < lppop->nItems; i++)
    {
	MENU_DrawMenuItem( hwnd, hDC, &lppop->items[i], lppop->Height, TRUE,
			   ODA_DRAWENTIRE );
    }
    return lppop->Height;
} 


/***********************************************************************
 *	     MENU_PatchResidentPopup
 */
BOOL32 MENU_PatchResidentPopup( HQUEUE16 checkQueue, WND* checkWnd )
{
    if( pTopPopupWnd )
    {
	HTASK16 hTask = 0;

	TRACE(menu,"patching resident popup: %04x %04x [%04x %04x]\n", 
		checkQueue, checkWnd ? checkWnd->hwndSelf : 0, pTopPopupWnd->hmemTaskQ, 
		pTopPopupWnd->owner ? pTopPopupWnd->owner->hwndSelf : 0);

	switch( checkQueue )
	{
	    case 0: /* checkWnd is the new popup owner */
		 if( checkWnd )
		 {
		     pTopPopupWnd->owner = checkWnd;
		     if( pTopPopupWnd->hmemTaskQ != checkWnd->hmemTaskQ )
			 hTask = QUEUE_GetQueueTask( checkWnd->hmemTaskQ );
		 }
		 break;

	    case 0xFFFF: /* checkWnd is destroyed */
		 if( pTopPopupWnd->owner == checkWnd )
		     pTopPopupWnd->owner = NULL;
		 return TRUE; 

	    default: /* checkQueue is exiting */
		 if( pTopPopupWnd->hmemTaskQ == checkQueue )
		 {
		     hTask = QUEUE_GetQueueTask( pTopPopupWnd->hmemTaskQ );
		     hTask = TASK_GetNextTask( hTask );
		 }
		 break;
	}

	if( hTask )
	{
	    TDB* task = (TDB*)GlobalLock16( hTask );
	    if( task )
	    {
		pTopPopupWnd->hInstance = task->hInstance;
		pTopPopupWnd->hmemTaskQ = task->hQueue;
		return TRUE;
	    } 
	    else WARN(menu,"failed to patch resident popup.\n");
	} 
    }
    return FALSE;
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
    WND 	*wndOwner = NULL;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	menu->items[menu->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }

    if( (wndOwner = WIN_FindWndPtr( hwndOwner )) )
    {
	UINT32	width, height;

	MENU_PopupMenuCalcSize( menu, hwndOwner );

	/* adjust popup menu pos so that it fits within the desktop */

	width = menu->Width + SYSMETRICS_CXBORDER;
	height = menu->Height + SYSMETRICS_CYBORDER; 

	if( x + width > SYSMETRICS_CXSCREEN )
	{
	    if( xanchor )
            	x -= width - xanchor;
            if( x + width > SYSMETRICS_CXSCREEN)
	    	x = SYSMETRICS_CXSCREEN - width;
	}
	if( x < 0 ) x = 0;

	if( y + height > SYSMETRICS_CYSCREEN )
	{ 
	    if( yanchor )
	    	y -= height + yanchor;
	    if( y + height > SYSMETRICS_CYSCREEN )
	    	y = SYSMETRICS_CYSCREEN - height;
	}
	if( y < 0 ) y = 0;

	width += POPUP_XSHADE * SYSMETRICS_CXBORDER;	/* add space for shading */
	height += POPUP_YSHADE * SYSMETRICS_CYBORDER;

	/* NOTE: In Windows, top menu popup is not owned. */
	if (!pTopPopupWnd)	/* create top level popup menu window */
	{
	    assert( uSubPWndLevel == 0 );

	    pTopPopupWnd = WIN_FindWndPtr(CreateWindow32A( POPUPMENU_CLASS_ATOM, NULL,
					  WS_POPUP, x, y, width, height,
					  hwndOwner, 0, wndOwner->hInstance,
					  (LPVOID)hmenu ));
	    if (!pTopPopupWnd) return FALSE;
	    menu->hWnd = pTopPopupWnd->hwndSelf;
	} 
	else
	    if( uSubPWndLevel )
	    {
		/* create a new window for the submenu */

		menu->hWnd = CreateWindow32A( POPUPMENU_CLASS_ATOM, NULL,
					  WS_POPUP, x, y, width, height,
					  menu->hWnd, 0, wndOwner->hInstance,
					  (LPVOID)hmenu );
		if( !menu->hWnd ) return FALSE;
	    }
	    else /* top level popup menu window already exists */
	    {
		menu->hWnd = pTopPopupWnd->hwndSelf;

		MENU_PatchResidentPopup( 0, wndOwner );
		SendMessage16( pTopPopupWnd->hwndSelf, MM_SETMENUHANDLE, (WPARAM16)hmenu, 0L);	

		/* adjust its size */

	        SetWindowPos32( menu->hWnd, 0, x, y, width, height,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
	    }

	uSubPWndLevel++;	/* menu level counter */

      /* Display the window */

	SetWindowPos32( menu->hWnd, HWND_TOP, 0, 0, 0, 0,
			SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
	UpdateWindow32( menu->hWnd );
	return TRUE;
    }
    return FALSE;
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
	(lppop->items[wIndex].fType & MF_SEPARATOR))
	wIndex = NO_SELECTED_ITEM;

    if (lppop->FocusedItem == wIndex) return;
    if (lppop->wFlags & MF_POPUP) hdc = GetDC32( lppop->hWnd );
    else hdc = GetDCEx32( lppop->hWnd, 0, DCX_CACHE | DCX_WINDOW);

      /* Clear previous highlighted item */
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	lppop->items[lppop->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	MENU_DrawMenuItem(lppop->hWnd,hdc,&lppop->items[lppop->FocusedItem],
                          lppop->Height, !(lppop->wFlags & MF_POPUP),
			  ODA_SELECT );
    }

      /* Highlight new item (if any) */
    lppop->FocusedItem = wIndex;
    if (lppop->FocusedItem != NO_SELECTED_ITEM) 
    {
	lppop->items[lppop->FocusedItem].fState |= MF_HILITE;
	MENU_DrawMenuItem( lppop->hWnd, hdc, &lppop->items[lppop->FocusedItem],
                           lppop->Height, !(lppop->wFlags & MF_POPUP),
			   ODA_SELECT );
        if (sendMenuSelect)
        {
            MENUITEM *ip = &lppop->items[lppop->FocusedItem];
	    SendMessage16( hwndOwner, WM_MENUSELECT, ip->wID,
                           MAKELONG(ip->fType | (ip->fState | MF_MOUSESELECT),
                                    hmenu) );
        }
    }
    else if (sendMenuSelect)
        SendMessage16( hwndOwner, WM_MENUSELECT, hmenu,
                       MAKELONG( lppop->wFlags | MF_MOUSESELECT, hmenu ) ); 

    ReleaseDC32( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_MoveSelection
 *
 * Moves currently selected item according to the offset parameter.
 * If there is no selection then it should select the last item if
 * offset is ITEM_PREV or the first item if offset is ITEM_NEXT.
 */
static void MENU_MoveSelection( HWND32 hwndOwner, HMENU32 hmenu, INT32 offset )
{
    INT32 i;
    POPUPMENU *menu;

    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    if (!menu->items) return;

    if ( menu->FocusedItem != NO_SELECTED_ITEM )
    {
	if( menu->nItems == 1 ) return; else
	for (i = menu->FocusedItem + offset ; i >= 0 && i < menu->nItems 
					    ; i += offset)
	    if (!(menu->items[i].fType & MF_SEPARATOR))
	    {
		MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
		return;
	    }
    }

    for ( i = (offset > 0) ? 0 : menu->nItems - 1; 
		  i >= 0 && i < menu->nItems ; i += offset)
	if (!(menu->items[i].fType & MF_SEPARATOR))
	{
	    MENU_SelectItem( hwndOwner, hmenu, i, TRUE );
	    return;
	}
}


/**********************************************************************
 *         MENU_SetItemData
 *
 * Set an item flags, id and text ptr. Called by InsertMenu() and
 * ModifyMenu().
 */
static BOOL32 MENU_SetItemData( MENUITEM *item, UINT32 flags, UINT32 id,
                                LPCSTR str )
{
    LPSTR prevText = IS_STRING_ITEM(item->fType) ? item->text : NULL;

    debug_print_menuitem("MENU_SetItemData from: ", item, "");

    if (IS_STRING_ITEM(flags))
    {
        if (!str || !*str)
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
    else item->text = NULL;

    if (flags & MF_OWNERDRAW) 
        item->dwItemData = (DWORD)str;
    else
        item->dwItemData = 0;

    if ((item->fType & MF_POPUP) && (flags & MF_POPUP) && (item->hSubMenu != id) )
	DestroyMenu32( item->hSubMenu );   /* ModifyMenu() spec */

    if (flags & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *)USER_HEAP_LIN_ADDR((UINT16)id);
        if (IS_A_MENU(menu)) menu->wFlags |= MF_POPUP;
	else
        {
            item->wID = 0;
            item->hSubMenu = 0;
            item->fType = 0;
            item->fState = 0;
	    return FALSE;
        }
    } 

    item->wID = id;
    if (flags & MF_POPUP)
      item->hSubMenu = id;

    if ((item->fType & MF_POPUP) && !(flags & MF_POPUP) )
      flags |= MF_POPUP; /* keep popup */

    item->fType = flags & TYPE_MASK;
    item->fState = (flags & STATE_MASK) &
        ~(MF_HILITE | MF_MOUSESELECT | MF_BYPOSITION);

    SetRectEmpty32( &item->rect );
    if (prevText) HeapFree( SystemHeap, 0, prevText );

    debug_print_menuitem("MENU_SetItemData to  : ", item, "");
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
        WARN(menu, "%04x not a menu handle\n",
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
            WARN(menu, "item %x not found\n",
                          pos );
            return NULL;
        }
        if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu)))
        {
            WARN(menu,"%04x not a menu handle\n",
                         hMenu);
            return NULL;
        }
    }

    /* Create new items array */

    newItems = HeapAlloc( SystemHeap, 0, sizeof(MENUITEM) * (menu->nItems+1) );
    if (!newItems)
    {
        WARN(menu, "allocation failed\n" );
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
            ERR(menu, "not a string item %04x\n", flags );
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


/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU32 hMenu)
{
    WORD resinfo;
    do {
	MENUITEMINFO32W mii;

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
	res += (~((int)res - 1)) & 1;
	mii.dwTypeData = (LPWSTR) res;
	res += (1 + lstrlen32W(mii.dwTypeData)) * sizeof(WCHAR);
	/* Align the following fields on a dword boundary.  */
	res += (~((int)res - 1)) & 3;

        /* FIXME: This is inefficient and cannot be optimised away by gcc.  */
	{
	    LPSTR newstr = HEAP_strdupWtoA(GetProcessHeap(),
					   0, mii.dwTypeData);
	    TRACE(menu, "Menu item: [%08x,%08x,%04x,%04x,%s]\n",
			 mii.fType, mii.fState, mii.wID, resinfo, newstr);
	    HeapFree( GetProcessHeap(), 0, newstr );
	}

	if (resinfo & 1) {	/* Pop-up? */
	    DWORD helpid = GET_DWORD(res); /* FIXME: use this.  */
	    res += sizeof(DWORD);
	    mii.hSubMenu = CreatePopupMenu32();
	    if (!mii.hSubMenu)
		return NULL;
	    if (!(res = MENUEX_ParseResource(res, mii.hSubMenu))) {
		DestroyMenu32(mii.hSubMenu);
                return NULL;
        }
	    mii.fMask |= MIIM_SUBMENU;
	    mii.fType |= MF_POPUP;
        }
	InsertMenuItem32W(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
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

    item = &menu->items[menu->FocusedItem];
    if ((item->fType & MF_POPUP) && (item->fState & MF_MOUSESELECT))
        return item->hSubMenu;
    return 0;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void MENU_HideSubPopups( HWND32 hwndOwner, HMENU32 hmenu,
                                BOOL32 sendMenuSelect )
{
    POPUPMENU *menu = (POPUPMENU*) USER_HEAP_LIN_ADDR( hmenu );;

    if (menu && uSubPWndLevel)
    {
	HMENU32 hsubmenu;
	POPUPMENU *submenu;
	MENUITEM *item;

	if (menu->FocusedItem != NO_SELECTED_ITEM)
	{
	    item = &menu->items[menu->FocusedItem];
	    if (!(item->fType & MF_POPUP) ||
		!(item->fState & MF_MOUSESELECT)) return;
	    item->fState &= ~MF_MOUSESELECT;
	    hsubmenu = item->hSubMenu;
	} else return;

	submenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hsubmenu );
	MENU_HideSubPopups( hwndOwner, hsubmenu, FALSE );
	MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, sendMenuSelect );

	if (submenu->hWnd == pTopPopupWnd->hwndSelf ) 
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
    RECT32 rect;
    POPUPMENU *menu;
    MENUITEM *item;
    WND *wndPtr;
    HDC32 hdc;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return hmenu;

    if (!(wndPtr = WIN_FindWndPtr( menu->hWnd )) ||
         (menu->FocusedItem == NO_SELECTED_ITEM)) return hmenu;

    item = &menu->items[menu->FocusedItem];
    if (!(item->fType & MF_POPUP) ||
         (item->fState & (MF_GRAYED | MF_DISABLED))) return hmenu;

    /* message must be send before using item,
       because nearly everything may by changed by the application ! */
    rect = item->rect;
    SendMessage16( hwndOwner, WM_INITMENUPOPUP, (WPARAM16)item->hSubMenu,
		   MAKELONG( menu->FocusedItem, IS_SYSTEM_MENU(menu) ));

    /* correct item if modified as a reaction to WM_INITMENUPOPUP-message */
    if (!(item->fState & MF_HILITE)) 
    {
        if (menu->wFlags & MF_POPUP) hdc = GetDC32( menu->hWnd );
        else hdc = GetDCEx32( menu->hWnd, 0, DCX_CACHE | DCX_WINDOW);
        item->fState |= MF_HILITE;
        MENU_DrawMenuItem( menu->hWnd, hdc, item, menu->Height, !(menu->wFlags & MF_POPUP), ODA_DRAWENTIRE ); 
	ReleaseDC32( menu->hWnd, hdc );
    }
    if (!item->rect.top && !item->rect.left && !item->rect.bottom && !item->rect.right)
      item->rect = rect;

    item->fState |= MF_MOUSESELECT;

    if (IS_SYSTEM_MENU(menu))
    {
	MENU_InitSysMenuPopup(item->hSubMenu, wndPtr->dwStyle, wndPtr->class->style);

	NC_GetSysPopupPos( wndPtr, &rect );
	rect.top = rect.bottom;
	rect.right = SYSMETRICS_CXSIZE;
        rect.bottom = SYSMETRICS_CYSIZE;
    }
    else
    {
	if (menu->wFlags & MF_POPUP)
	{
	    rect.left = wndPtr->rectWindow.left + item->rect.right-arrow_bitmap_width;
	    rect.top = wndPtr->rectWindow.top + item->rect.top;
	    rect.right = item->rect.left - item->rect.right + 2*arrow_bitmap_width;
	    rect.bottom = item->rect.top - item->rect.bottom;
	}
	else
	{
	    rect.left = wndPtr->rectWindow.left + item->rect.left;
	    rect.top = wndPtr->rectWindow.top + item->rect.bottom;
	    rect.right = item->rect.right - item->rect.left;
	    rect.bottom = item->rect.bottom - item->rect.top;
	}
    }

    MENU_ShowPopup( hwndOwner, item->hSubMenu, menu->FocusedItem,
		    rect.left, rect.top, rect.right, rect.bottom );
    if (selectFirst)
        MENU_MoveSelection( hwndOwner, item->hSubMenu, ITEM_NEXT );
    return item->hSubMenu;
}

/***********************************************************************
 *           MENU_PtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static HMENU32 MENU_PtMenu( HMENU32 hMenu, POINT16 pt )
{
   POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hMenu );
   register UINT32 ht = menu->FocusedItem;

   /* try subpopup first (if any) */
    ht = (ht != NO_SELECTED_ITEM &&
          (menu->items[ht].fType & MF_POPUP) &&
          (menu->items[ht].fState & MF_MOUSESELECT))
        ? (UINT32) MENU_PtMenu(menu->items[ht].hSubMenu, pt) : 0;

   if( !ht )	/* check the current window (avoiding WM_HITTEST) */
   {
	ht = (UINT32)NC_HandleNCHitTest( menu->hWnd, pt );
	if( menu->wFlags & MF_POPUP )
	    ht =  (ht != (UINT32)HTNOWHERE && 
		   ht != (UINT32)HTERROR) ? (UINT32)hMenu : 0;
	else
	{
	    WND* wndPtr = WIN_FindWndPtr(menu->hWnd);

	    ht = ( ht == HTSYSMENU ) ? (UINT32)(wndPtr->hSysMenu)
		 : ( ht == HTMENU ) ? (UINT32)(wndPtr->wIDmenu) : 0;
	}
   }
   return (HMENU32)ht;
}

/***********************************************************************
 *           MENU_ExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ExecFocusedItem( MTRACKER* pmt, HMENU32 hMenu )
{
    MENUITEM *item;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hMenu );
    if (!menu || !menu->nItems || 
	(menu->FocusedItem == NO_SELECTED_ITEM)) return TRUE;

    item = &menu->items[menu->FocusedItem];

    TRACE(menu, "%08x %08x %08x\n",
                 hMenu, item->wID, item->hSubMenu);

    if (!(item->fType & MF_POPUP))
    {
	if (!(item->fState & (MF_GRAYED | MF_DISABLED)))
	{
	    if( menu->wFlags & MF_SYSMENU )
	    {
		PostMessage16( pmt->hOwnerWnd, WM_SYSCOMMAND, item->wID,
			       MAKELPARAM((INT16)pmt->pt.x, (INT16)pmt->pt.y) );
	    }
	    else
		PostMessage16( pmt->hOwnerWnd, WM_COMMAND, item->wID, 0 );
	    return FALSE;
	}
	else return TRUE;
    }
    else
    {
	pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hMenu, TRUE );
	return TRUE;
    }
}


/***********************************************************************
 *           MENU_SwitchTracking
 *
 * Helper function for menu navigation routines.
 */
static void MENU_SwitchTracking( MTRACKER* pmt, HMENU32 hPtMenu, UINT32 id )
{
    POPUPMENU *ptmenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hPtMenu );
    POPUPMENU *topmenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( pmt->hTopMenu );

    if( pmt->hTopMenu != hPtMenu &&
	!((ptmenu->wFlags | topmenu->wFlags) & MF_POPUP) )
    {
	/* both are top level menus (system and menu-bar) */

	MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE );
        pmt->hTopMenu = hPtMenu;
    }
    else MENU_HideSubPopups( pmt->hOwnerWnd, hPtMenu, FALSE );
    MENU_SelectItem( pmt->hOwnerWnd, hPtMenu, id, TRUE );
}


/***********************************************************************
 *           MENU_ButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ButtonDown( MTRACKER* pmt, HMENU32 hPtMenu )
{
    if (hPtMenu)
    {
	UINT32 id = 0;
	POPUPMENU *ptmenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hPtMenu );
	MENUITEM *item;

	if( IS_SYSTEM_MENU(ptmenu) )
	    item = ptmenu->items;
	else
	    item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item )
	{
	    if( ptmenu->FocusedItem == id )
	    {
		/* nothing to do with already selected non-popup */
		if( !(item->fType & MF_POPUP) ) return TRUE;

	        if( item->fState & MF_MOUSESELECT )
		{
		    if( ptmenu->wFlags & MF_POPUP )
		    {
			/* hide selected subpopup */

			MENU_HideSubPopups( pmt->hOwnerWnd, hPtMenu, TRUE );
			pmt->hCurrentMenu = hPtMenu;
			return TRUE;
		    }
		    return FALSE; /* shouldn't get here */
		} 
	    }
	    else MENU_SwitchTracking( pmt, hPtMenu, id );

	    /* try to display a subpopup */

	    pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hPtMenu, FALSE );
	    return TRUE;
	} 
	else WARN(menu, "\tunable to find clicked item!\n");
    }
    return FALSE;
}

/***********************************************************************
 *           MENU_ButtonUp
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_ButtonUp( MTRACKER* pmt, HMENU32 hPtMenu )
{
    if (hPtMenu)
    {
	UINT32 id = 0;
	POPUPMENU *ptmenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hPtMenu );
	MENUITEM *item;

        if( IS_SYSTEM_MENU(ptmenu) )
            item = ptmenu->items;
        else
            item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item && (ptmenu->FocusedItem == id ))
	{
	    if( !(item->fType & MF_POPUP) )
		return MENU_ExecFocusedItem( pmt, hPtMenu );
	    hPtMenu = item->hSubMenu;
	    if( hPtMenu == pmt->hCurrentMenu )
	    {
	        /* Select first item of sub-popup */    

	        MENU_SelectItem( pmt->hOwnerWnd, hPtMenu, NO_SELECTED_ITEM, FALSE );
	        MENU_MoveSelection( pmt->hOwnerWnd, hPtMenu, ITEM_NEXT );
	    }
	    return TRUE;
	}
    }
    return FALSE;
}


/***********************************************************************
 *           MENU_MouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL32 MENU_MouseMove( MTRACKER* pmt, HMENU32 hPtMenu )
{
    UINT32 id = NO_SELECTED_ITEM;
    POPUPMENU *ptmenu = NULL;

    if( hPtMenu )
    {
	ptmenu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hPtMenu );
        if( IS_SYSTEM_MENU(ptmenu) )
	    id = 0;
        else
            MENU_FindItemByCoords( ptmenu, pmt->pt, &id );
    } 

    if( id == NO_SELECTED_ITEM )
    {
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu, 
			 NO_SELECTED_ITEM, TRUE );
    }
    else if( ptmenu->FocusedItem != id )
    {
	    MENU_SwitchTracking( pmt, hPtMenu, id );
	    pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hPtMenu, FALSE );
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_DoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win32 is a bit different.
 */
static LRESULT MENU_DoNextMenu( MTRACKER* pmt, UINT32 vk )
{
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( pmt->hTopMenu );

    if( (vk == VK_LEFT &&  menu->FocusedItem == 0 ) ||
        (vk == VK_RIGHT && menu->FocusedItem == menu->nItems - 1))
    {
	WND*    wndPtr;
	HMENU32 hNewMenu;
	HWND32  hNewWnd;
	UINT32  id = 0;
	LRESULT l = SendMessage16( pmt->hOwnerWnd, WM_NEXTMENU, (WPARAM16)vk, 
		(IS_SYSTEM_MENU(menu)) ? GetSubMenu16(pmt->hTopMenu,0) : pmt->hTopMenu );

	TRACE(menu,"%04x [%04x] -> %04x [%04x]\n",
		     (UINT16)pmt->hCurrentMenu, (UINT16)pmt->hOwnerWnd, LOWORD(l), HIWORD(l) );

	if( l == 0 )
	{
	    wndPtr = WIN_FindWndPtr(pmt->hOwnerWnd);

	    hNewWnd = pmt->hOwnerWnd;
	    if( IS_SYSTEM_MENU(menu) )
	    {
		/* switch to the menu bar */

		if( wndPtr->dwStyle & WS_CHILD || !wndPtr->wIDmenu ) 
		    return FALSE;

	        hNewMenu = wndPtr->wIDmenu;
	        if( vk == VK_LEFT )
	        {
		    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hNewMenu );
		    id = menu->nItems - 1;
	        }
	    }
	    else if( wndPtr->dwStyle & WS_SYSMENU ) 
	    {
		/* switch to the system menu */
	        hNewMenu = wndPtr->hSysMenu; 
	    }
	    else return FALSE;
	}
	else    /* application returned a new menu to switch to */
	{
	    hNewMenu = LOWORD(l); hNewWnd = HIWORD(l);

	    if( IsMenu32(hNewMenu) && IsWindow32(hNewWnd) )
	    {
		wndPtr = WIN_FindWndPtr(hNewWnd);

		if( wndPtr->dwStyle & WS_SYSMENU &&
		    GetSubMenu16(wndPtr->hSysMenu, 0) == hNewMenu )
		{
	            /* get the real system menu */
		    hNewMenu =  wndPtr->hSysMenu;
		}
	        else if( wndPtr->dwStyle & WS_CHILD || wndPtr->wIDmenu != hNewMenu )
		{
		    /* FIXME: Not sure what to do here, perhaps,
		     * try to track hNewMenu as a popup? */

		    TRACE(menu," -- got confused.\n");
		    return FALSE;
		}
	    }
	    else return FALSE;
	}

	if( hNewMenu != pmt->hTopMenu )
	{
	    MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE );
	    if( pmt->hCurrentMenu != pmt->hTopMenu ) 
		MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	}

	if( hNewWnd != pmt->hOwnerWnd )
	{
	    ReleaseCapture(); 
	    pmt->hOwnerWnd = hNewWnd;
	    EVENT_Capture( pmt->hOwnerWnd, HTMENU );
	}

	pmt->hTopMenu = pmt->hCurrentMenu = hNewMenu; /* all subpopups are hidden */
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, id, TRUE ); 

	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           MENU_SuspendPopup
 *
 * The idea is not to show the popup if the next input message is
 * going to hide it anyway.
 */
static BOOL32 MENU_SuspendPopup( MTRACKER* pmt, UINT16 uMsg )
{
    MSG16 msg;

    msg.hwnd = pmt->hOwnerWnd;

    PeekMessage16( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
    pmt->trackFlags |= TF_SKIPREMOVE;

    switch( uMsg )
    {
	case WM_KEYDOWN:
	     PeekMessage16( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	     if( msg.message == WM_KEYUP || msg.message == WM_PAINT )
	     {
		 PeekMessage16( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
	         PeekMessage16( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	         if( msg.message == WM_KEYDOWN &&
		    (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
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

/***********************************************************************
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu. 
 */
static void MENU_KeyLeft( MTRACKER* pmt )
{
    POPUPMENU *menu;
    HMENU32 hmenutmp, hmenuprev;
    UINT32  prevcol;

    hmenuprev = hmenutmp = pmt->hTopMenu;
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenutmp );

    /* Try to move 1 column left (if possible) */
    if( (prevcol = MENU_GetStartOfPrevColumn( pmt->hCurrentMenu )) != 
	NO_SELECTED_ITEM ) {
	
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 prevcol, TRUE );
	return;
    }

    /* close topmost popup */
    while (hmenutmp != pmt->hCurrentMenu)
    {
	hmenuprev = hmenutmp;
	hmenutmp = MENU_GetSubPopup( hmenuprev );
    }

    MENU_HideSubPopups( pmt->hOwnerWnd, hmenuprev, TRUE );
    pmt->hCurrentMenu = hmenuprev; 

    if ( (hmenuprev == pmt->hTopMenu) && !(menu->wFlags & MF_POPUP) )
    {
	/* move menu bar selection if no more popups are left */

	if( !MENU_DoNextMenu( pmt, VK_LEFT) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_PREV );

	if ( hmenuprev != hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	{
	   /* A sublevel menu was displayed - display the next one
	    * unless there is another displacement coming up */

	    if( !MENU_SuspendPopup( pmt, WM_KEYDOWN ) )
		pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd,
						pmt->hTopMenu, TRUE );
	}
    }
}


/***********************************************************************
 *           MENU_KeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
static void MENU_KeyRight( MTRACKER* pmt )
{
    HMENU32 hmenutmp;
    POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( pmt->hTopMenu );
    UINT32  nextcol;

    TRACE(menu, "MENU_KeyRight called, cur %x (%s), top %x (%s).\n",
		  pmt->hCurrentMenu,
		  ((POPUPMENU *)USER_HEAP_LIN_ADDR(pmt->hCurrentMenu))->
		     items[0].text,
		  pmt->hTopMenu, menu->items[0].text );

    if ( (menu->wFlags & MF_POPUP) || (pmt->hCurrentMenu != pmt->hTopMenu))
    {
	/* If already displaying a popup, try to display sub-popup */

	hmenutmp = pmt->hCurrentMenu;
	pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hmenutmp, TRUE );

	/* if subpopup was displayed then we are done */
	if (hmenutmp != pmt->hCurrentMenu) return;
    }

    /* Check to see if there's another column */
    if( (nextcol = MENU_GetStartOfNextColumn( pmt->hCurrentMenu )) != 
	NO_SELECTED_ITEM ) {
	TRACE(menu, "Going to %d.\n", nextcol );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 nextcol, TRUE );
	return;
    }

    if (!(menu->wFlags & MF_POPUP))	/* menu bar tracking */
    {
	if( pmt->hCurrentMenu != pmt->hTopMenu )
	{
	    MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE );
	    hmenutmp = pmt->hCurrentMenu = pmt->hTopMenu;
	} else hmenutmp = 0;

	/* try to move to the next item */
	if( !MENU_DoNextMenu( pmt, VK_RIGHT) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_NEXT );

	if( hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	    if( !MENU_SuspendPopup(pmt, WM_KEYDOWN) )
		pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, 
						       pmt->hTopMenu, TRUE );
    }
}


/***********************************************************************
 *           MENU_TrackMenu
 *
 * Menu tracking code.
 */
static BOOL32 MENU_TrackMenu( HMENU32 hmenu, UINT32 wFlags, INT32 x, INT32 y,
                              HWND32 hwnd, const RECT32 *lprect )
{
    MSG16 msg;
    POPUPMENU *menu;
    BOOL32 fRemove;
    MTRACKER mt = { 0, hmenu, hmenu, hwnd, {x, y} };	/* control struct */

    fEndMenu = FALSE;
    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu ))) return FALSE;

    if (wFlags & TPM_BUTTONDOWN) MENU_ButtonDown( &mt, hmenu );

    EVENT_Capture( mt.hOwnerWnd, HTMENU );

    while (!fEndMenu)
    {
	menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( mt.hCurrentMenu );
	msg.hwnd = (wFlags & TPM_ENTERIDLEEX && menu->wFlags & MF_POPUP) ? menu->hWnd : 0;

	/* we have to keep the message in the queue until it's
	 * clear that menu loop is not over yet. */

	if (!MSG_InternalGetMessage( &msg, msg.hwnd, mt.hOwnerWnd,
				     MSGF_MENU, PM_NOREMOVE, TRUE )) break;

        TranslateMessage16( &msg );
        CONV_POINT16TO32( &msg.pt, &mt.pt );

        fRemove = FALSE;
	if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
	{
	    /* Find a menu for this mouse event */

	    hmenu = MENU_PtMenu( mt.hTopMenu, msg.pt );

	    switch(msg.message)
	    {
		/* no WM_NC... messages in captured state */

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		    fEndMenu |= !MENU_ButtonDown( &mt, hmenu );
		    break;
		
		case WM_RBUTTONUP:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONUP:
		    /* If outside all menus but inside lprect, ignore it */
		    if (hmenu || !lprect || !PtInRect32(lprect, mt.pt))
		    {
			fEndMenu |= !MENU_ButtonUp( &mt, hmenu );
			fRemove = TRUE; 
		    }
		    break;
		
		case WM_MOUSEMOVE:
		    if ((msg.wParam & MK_LBUTTON) || ((wFlags & TPM_RIGHTBUTTON) 
						  && (msg.wParam & MK_RBUTTON)))
		    {
			fEndMenu |= !MENU_MouseMove( &mt, hmenu );
		    }
	    } /* switch(msg.message) - mouse */
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
		    MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu, 
				     NO_SELECTED_ITEM, FALSE );
		/* fall through */
		case VK_UP:
		    MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu, 
				       (msg.wParam == VK_HOME)? ITEM_NEXT : ITEM_PREV );
		    break;

		case VK_DOWN: /* If on menu bar, pull-down the menu */

		    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( mt.hCurrentMenu );
		    if (!(menu->wFlags & MF_POPUP))
			mt.hCurrentMenu = MENU_ShowSubPopup( mt.hOwnerWnd, mt.hTopMenu, TRUE );
		    else      /* otherwise try to move selection */
			MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu, ITEM_NEXT );
		    break;

		case VK_LEFT:
		    MENU_KeyLeft( &mt );
		    break;
		    
		case VK_RIGHT:
		    MENU_KeyRight( &mt );
		    break;
		    
		case VK_SPACE:
		case VK_RETURN:
		    fEndMenu |= !MENU_ExecFocusedItem( &mt, mt.hCurrentMenu );
		    break;

		case VK_ESCAPE:
		    fEndMenu = TRUE;
		    break;

		default:
		    break;
		}
		break;  /* WM_KEYDOWN */

	    case WM_SYSKEYDOWN:
		switch(msg.wParam)
		{
		case VK_MENU:
		    fEndMenu = TRUE;
		    break;
		    
		}
		break;  /* WM_SYSKEYDOWN */

	    case WM_CHAR:
		{
		    UINT32	pos;

		      /* Hack to avoid control chars. */
		      /* We will find a better way real soon... */
		    if ((msg.wParam <= 32) || (msg.wParam >= 127)) break;

		    pos = MENU_FindItemByKey( mt.hOwnerWnd, mt.hCurrentMenu, 
							msg.wParam, FALSE );
		    if (pos == (UINT32)-2) fEndMenu = TRUE;
		    else if (pos == (UINT32)-1) MessageBeep32(0);
		    else
		    {
			MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu, pos, TRUE );
			fEndMenu |= !MENU_ExecFocusedItem( &mt, mt.hCurrentMenu );
		    }
		}		    
		break;
	    }  /* switch(msg.message) - kbd */
	}
	else
	{
	    DispatchMessage16( &msg );
	}

	if (!fEndMenu) fRemove = TRUE;

	/* finally remove message from the queue */

        if (fRemove && !(mt.trackFlags & TF_SKIPREMOVE) )
	    PeekMessage16( &msg, 0, msg.message, msg.message, PM_REMOVE );
	else mt.trackFlags &= ~TF_SKIPREMOVE;
    }

    ReleaseCapture();
    if( IsWindow32( mt.hOwnerWnd ) )
    {
	MENU_HideSubPopups( mt.hOwnerWnd, mt.hTopMenu, FALSE );

	menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( mt.hTopMenu );
	if (menu && menu->wFlags & MF_POPUP) 
	{
	    ShowWindow32( menu->hWnd, SW_HIDE );
	    uSubPWndLevel = 0;
	}
	MENU_SelectItem( mt.hOwnerWnd, mt.hTopMenu, NO_SELECTED_ITEM, FALSE );
	SendMessage16( mt.hOwnerWnd, WM_MENUSELECT, 0, MAKELONG( 0xffff, 0 ) );
    }
    fEndMenu = FALSE;
    return TRUE;
}

/***********************************************************************
 *           MENU_InitTracking
 */
static BOOL32 MENU_InitTracking(HWND32 hWnd, HMENU32 hMenu)
{
    HideCaret32(0);
    SendMessage16( hWnd, WM_ENTERMENULOOP, 0, 0 );
    SendMessage16( hWnd, WM_SETCURSOR, hWnd, HTCAPTION );
    SendMessage16( hWnd, WM_INITMENU, hMenu, 0 );
    return TRUE;
}

/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( WND* wndPtr, INT32 ht, POINT32 pt )
{
    HWND32  hWnd = wndPtr->hwndSelf;
    HMENU32 hMenu = (ht == HTSYSMENU) ? wndPtr->hSysMenu : wndPtr->wIDmenu;

    if (IsMenu32(hMenu))
    {
	MENU_InitTracking( hWnd, hMenu );
	MENU_TrackMenu( hMenu, TPM_ENTERIDLEEX | TPM_BUTTONDOWN | 
		        TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, hWnd, NULL );

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
   UINT32 uItem = NO_SELECTED_ITEM;
   HMENU32 hTrackMenu; 

    /* find window that has a menu */
 
    while( wndPtr->dwStyle & WS_CHILD && !(wndPtr->dwStyle & WS_SYSMENU) )
	if( !(wndPtr = wndPtr->parent) ) return;

    /* check if we have to track a system menu */
          
    if( (wndPtr->dwStyle & (WS_CHILD | WS_MINIMIZE)) || 
	!wndPtr->wIDmenu || vkey == VK_SPACE )
    {
	if( !(wndPtr->dwStyle & WS_SYSMENU) ) return;
	hTrackMenu = wndPtr->hSysMenu;
	uItem = 0;
	wParam |= HTSYSMENU;	/* prevent item lookup */
    }
    else
	hTrackMenu = wndPtr->wIDmenu;

    if (IsMenu32( hTrackMenu ))
    {
	MENU_InitTracking( wndPtr->hwndSelf, hTrackMenu );

        if( vkey && vkey != VK_SPACE )
        {
            uItem = MENU_FindItemByKey( wndPtr->hwndSelf, hTrackMenu, 
					vkey, (wParam & HTSYSMENU) );
	    if( uItem >= (UINT32)(-2) )
	    {
	        if( uItem == (UINT32)(-1) ) MessageBeep32(0);
		hTrackMenu = 0;
	    }
        }

	if( hTrackMenu )
	{
 	    MENU_SelectItem( wndPtr->hwndSelf, hTrackMenu, uItem, TRUE );

	    if( uItem == NO_SELECTED_ITEM )
		MENU_MoveSelection( wndPtr->hwndSelf, hTrackMenu, ITEM_NEXT );
	    else if( vkey )
		PostMessage16( wndPtr->hwndSelf, WM_KEYDOWN, VK_DOWN, 0L );

	    MENU_TrackMenu( hTrackMenu, TPM_ENTERIDLEEX | TPM_LEFTALIGN | TPM_LEFTBUTTON,
			    0, 0, wndPtr->hwndSelf, NULL );
	}
        SendMessage16( wndPtr->hwndSelf, WM_EXITMENULOOP, 0, 0 );
        ShowCaret32(0);
    }
}


/**********************************************************************
 *           TrackPopupMenu16   (USER.416)
 */
BOOL16 WINAPI TrackPopupMenu16( HMENU16 hMenu, UINT16 wFlags, INT16 x, INT16 y,
                           INT16 nReserved, HWND16 hWnd, const RECT16 *lpRect )
{
    RECT32 r;
    if (lpRect)
   	 CONV_RECT16TO32( lpRect, &r );
    return TrackPopupMenu32( hMenu, wFlags, x, y, nReserved, hWnd,
                             lpRect ? &r : NULL );
}


/**********************************************************************
 *           TrackPopupMenu32   (USER32.549)
 */
BOOL32 WINAPI TrackPopupMenu32( HMENU32 hMenu, UINT32 wFlags, INT32 x, INT32 y,
                           INT32 nReserved, HWND32 hWnd, const RECT32 *lpRect )
{
    BOOL32 ret = FALSE;

    HideCaret32(0);
    SendMessage16( hWnd, WM_INITMENUPOPUP, (WPARAM16)hMenu, 0);
    if (MENU_ShowPopup( hWnd, hMenu, 0, x, y, 0, 0 )) 
	ret = MENU_TrackMenu( hMenu, wFlags & ~TPM_INTERNAL, 0, 0, hWnd, lpRect );
    ShowCaret32(0);
    return ret;
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.550)
 */
BOOL32 WINAPI TrackPopupMenuEx( HMENU32 hMenu, UINT32 wFlags, INT32 x, INT32 y,
                                HWND32 hWnd, LPTPMPARAMS lpTpm )
{
    FIXME(menu, "not fully implemented\n" );
    return TrackPopupMenu32( hMenu, wFlags, x, y, 0, hWnd,
                             lpTpm ? &lpTpm->rcExclude : NULL );
}

/***********************************************************************
 *           PopupMenuWndProc
 *
 * NOTE: Windows has totally different (and undocumented) popup wndproc.
 */
LRESULT WINAPI PopupMenuWndProc( HWND32 hwnd, UINT32 message, WPARAM32 wParam,
                                 LPARAM lParam )
{    
    WND* wndPtr = WIN_FindWndPtr(hwnd);

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
    case WM_ERASEBKGND:
	return 1;

    case WM_DESTROY:

	/* zero out global pointer in case resident popup window
	 * was somehow destroyed. */

	if( pTopPopupWnd )
	{
	    if( hwnd == pTopPopupWnd->hwndSelf )
	    {
		ERR(menu, "resident popup destroyed!\n");

		pTopPopupWnd = NULL;
		uSubPWndLevel = 0;
	    }
	    else
		uSubPWndLevel--;
	}
	break;

    case WM_SHOWWINDOW:

	if( wParam )
	{
	    if( !(*(HMENU32*)wndPtr->wExtra) )
		ERR(menu,"no menu to display\n");
	}
	else
	    *(HMENU32*)wndPtr->wExtra = 0;
	break;

    case MM_SETMENUHANDLE:

	*(HMENU32*)wndPtr->wExtra = (HMENU32)wParam;
        break;

    case MM_GETMENUHANDLE:

	return *(HMENU32*)wndPtr->wExtra;

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

    TRACE(menu, "HWND 0x%x, width %d, "
		  "at (%d, %d).\n", hwnd, menubarWidth, orgX, orgY );
    
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
BOOL16 WINAPI ChangeMenu16( HMENU16 hMenu, UINT16 pos, SEGPTR data,
                            UINT16 id, UINT16 flags )
{
    TRACE(menu,"menu=%04x pos=%d data=%08lx id=%04x flags=%04x\n",
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
 *         ChangeMenu32A    (USER32.23)
 */
BOOL32 WINAPI ChangeMenu32A( HMENU32 hMenu, UINT32 pos, LPCSTR data,
                             UINT32 id, UINT32 flags )
{
    TRACE(menu,"menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
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
 *         ChangeMenu32W    (USER32.24)
 */
BOOL32 WINAPI ChangeMenu32W( HMENU32 hMenu, UINT32 pos, LPCWSTR data,
                             UINT32 id, UINT32 flags )
{
    TRACE(menu,"menu=%08x pos=%d data=%08lx id=%08x flags=%08x\n",
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
BOOL16 WINAPI CheckMenuItem16( HMENU16 hMenu, UINT16 id, UINT16 flags )
{
    return (BOOL16)CheckMenuItem32( hMenu, id, flags );
}


/*******************************************************************
 *         CheckMenuItem32    (USER32.46)
 */
DWORD WINAPI CheckMenuItem32( HMENU32 hMenu, UINT32 id, UINT32 flags )
{
    MENUITEM *item;
    DWORD ret;

    TRACE(menu,"%04x %04x %04x\n", hMenu, id, flags );
    if (!(item = MENU_FindItem( &hMenu, &id, flags ))) return -1;
    ret = item->fState & MF_CHECKED;
    if (flags & MF_CHECKED) item->fState |= MF_CHECKED;
    else item->fState &= ~MF_CHECKED;
    return ret;
}


/**********************************************************************
 *         EnableMenuItem16    (USER.155)
 */
UINT16 WINAPI EnableMenuItem16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return EnableMenuItem32( hMenu, wItemID, wFlags );
}


/**********************************************************************
 *         EnableMenuItem32    (USER32.170)
 */
UINT32 WINAPI EnableMenuItem32( HMENU32 hMenu, UINT32 wItemID, UINT32 wFlags )
{
    UINT32    oldflags;
    MENUITEM *item;

    TRACE(menu,"(%04x, %04X, %04X) !\n", 
		 hMenu, wItemID, wFlags);

    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags )))
	return (UINT32)-1;

    oldflags = item->fState & (MF_GRAYED | MF_DISABLED);
    item->fState ^= (oldflags ^ wFlags) & (MF_GRAYED | MF_DISABLED);
    return oldflags;
}


/*******************************************************************
 *         GetMenuString16    (USER.161)
 */
INT16 WINAPI GetMenuString16( HMENU16 hMenu, UINT16 wItemID,
                              LPSTR str, INT16 nMaxSiz, UINT16 wFlags )
{
    return GetMenuString32A( hMenu, wItemID, str, nMaxSiz, wFlags );
}


/*******************************************************************
 *         GetMenuString32A    (USER32.268)
 */
INT32 WINAPI GetMenuString32A( HMENU32 hMenu, UINT32 wItemID,
                               LPSTR str, INT32 nMaxSiz, UINT32 wFlags )
{
    MENUITEM *item;

    TRACE(menu, "menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",
                 hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return 0;
    if (!IS_STRING_ITEM(item->fType)) return 0;
    lstrcpyn32A( str, item->text, nMaxSiz );
    TRACE(menu, "returning '%s'\n", str );
    return strlen(str);
}


/*******************************************************************
 *         GetMenuString32W    (USER32.269)
 */
INT32 WINAPI GetMenuString32W( HMENU32 hMenu, UINT32 wItemID,
                               LPWSTR str, INT32 nMaxSiz, UINT32 wFlags )
{
    MENUITEM *item;

    TRACE(menu, "menu=%04x item=%04x ptr=%p len=%d flags=%04x\n",
                 hMenu, wItemID, str, nMaxSiz, wFlags );
    if (!str || !nMaxSiz) return 0;
    str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return 0;
    if (!IS_STRING_ITEM(item->fType)) return 0;
    lstrcpynAtoW( str, item->text, nMaxSiz );
    return lstrlen32W(str);
}


/**********************************************************************
 *         HiliteMenuItem16    (USER.162)
 */
BOOL16 WINAPI HiliteMenuItem16( HWND16 hWnd, HMENU16 hMenu, UINT16 wItemID,
                                UINT16 wHilite )
{
    return HiliteMenuItem32( hWnd, hMenu, wItemID, wHilite );
}


/**********************************************************************
 *         HiliteMenuItem32    (USER32.318)
 */
BOOL32 WINAPI HiliteMenuItem32( HWND32 hWnd, HMENU32 hMenu, UINT32 wItemID,
                                UINT32 wHilite )
{
    LPPOPUPMENU menu;
    TRACE(menu,"(%04x, %04x, %04x, %04x);\n", 
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
UINT16 WINAPI GetMenuState16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return GetMenuState32( hMenu, wItemID, wFlags );
}


/**********************************************************************
 *         GetMenuState32    (USER32.267)
 */
UINT32 WINAPI GetMenuState32( HMENU32 hMenu, UINT32 wItemID, UINT32 wFlags )
{
    MENUITEM *item;
    TRACE(menu,"(%04x, %04x, %04x);\n", 
		 hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    debug_print_menuitem ("  item: ", item, "");
    if (item->fType & MF_POPUP)
    {
	POPUPMENU *menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( item->hSubMenu );
	if (!menu) return -1;
	else return (menu->nItems << 8) | ((item->fState|item->fType) & 0xff);
    }
    else
    {
	/* We used to (from way back then) mask the result to 0xff.  */
	/* I don't know why and it seems wrong as the documented */
	/* return flag MF_SEPARATOR is outside that mask.  */
	return (item->fType | item->fState);
    }
}


/**********************************************************************
 *         GetMenuItemCount16    (USER.263)
 */
INT16 WINAPI GetMenuItemCount16( HMENU16 hMenu )
{
    LPPOPUPMENU	menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!IS_A_MENU(menu)) return -1;
    TRACE(menu,"(%04x) returning %d\n", 
                  hMenu, menu->nItems );
    return menu->nItems;
}


/**********************************************************************
 *         GetMenuItemCount32    (USER32.262)
 */
INT32 WINAPI GetMenuItemCount32( HMENU32 hMenu )
{
    LPPOPUPMENU	menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);
    if (!IS_A_MENU(menu)) return -1;
    TRACE(menu,"(%04x) returning %d\n", 
                  hMenu, menu->nItems );
    return menu->nItems;
}


/**********************************************************************
 *         GetMenuItemID16    (USER.264)
 */
UINT16 WINAPI GetMenuItemID16( HMENU16 hMenu, INT16 nPos )
{
    LPPOPUPMENU	menu;

    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return -1;
    if ((nPos < 0) || ((UINT16) nPos >= menu->nItems)) return -1;
    if (menu->items[nPos].fType & MF_POPUP) return -1;
    return menu->items[nPos].wID;
}


/**********************************************************************
 *         GetMenuItemID32    (USER32.263)
 */
UINT32 WINAPI GetMenuItemID32( HMENU32 hMenu, INT32 nPos )
{
    LPPOPUPMENU	menu;

    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return -1;
    if ((nPos < 0) || (nPos >= menu->nItems)) return -1;
    return menu->items[nPos].wID;
}


/*******************************************************************
 *         InsertMenu16    (USER.410)
 */
BOOL16 WINAPI InsertMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
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
 *         InsertMenu32A    (USER32.322)
 */
BOOL32 WINAPI InsertMenu32A( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                             UINT32 id, LPCSTR str )
{
    MENUITEM *item;

    if (IS_STRING_ITEM(flags) && str)
        TRACE(menu, "hMenu %04x, pos %d, flags %08x, "
		      "id %04x, str '%s'\n",
                      hMenu, pos, flags, id, str );
    else TRACE(menu, "hMenu %04x, pos %d, flags %08x, "
		       "id %04x, str %08lx (not a string)\n",
                       hMenu, pos, flags, id, (DWORD)str );

    if (!(item = MENU_InsertItem( hMenu, pos, flags ))) return FALSE;

    if (!(MENU_SetItemData( item, flags, id, str )))
    {
        RemoveMenu32( hMenu, pos, flags );
        return FALSE;
    }

    if (flags & MF_POPUP)  /* Set the MF_POPUP flag on the popup-menu */
	((POPUPMENU *)USER_HEAP_LIN_ADDR((HMENU16)id))->wFlags |= MF_POPUP;

    item->hCheckBit = item->hUnCheckBit = 0;
    return TRUE;
}


/*******************************************************************
 *         InsertMenu32W    (USER32.325)
 */
BOOL32 WINAPI InsertMenu32W( HMENU32 hMenu, UINT32 pos, UINT32 flags,
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
BOOL16 WINAPI AppendMenu16(HMENU16 hMenu, UINT16 flags, UINT16 id, SEGPTR data)
{
    return InsertMenu16( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenu32A    (USER32.5)
 */
BOOL32 WINAPI AppendMenu32A( HMENU32 hMenu, UINT32 flags,
                             UINT32 id, LPCSTR data )
{
    return InsertMenu32A( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenu32W    (USER32.6)
 */
BOOL32 WINAPI AppendMenu32W( HMENU32 hMenu, UINT32 flags,
                             UINT32 id, LPCWSTR data )
{
    return InsertMenu32W( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/**********************************************************************
 *         RemoveMenu16    (USER.412)
 */
BOOL16 WINAPI RemoveMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return RemoveMenu32( hMenu, nPos, wFlags );
}


/**********************************************************************
 *         RemoveMenu32    (USER32.441)
 */
BOOL32 WINAPI RemoveMenu32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags )
{
    LPPOPUPMENU	menu;
    MENUITEM *item;

    TRACE(menu,"(%04x, %04x, %04x)\n",hMenu, nPos, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    if (!(menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return FALSE;
    
      /* Remove item */

    MENU_FreeItemData( item );

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
BOOL16 WINAPI DeleteMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return DeleteMenu32( hMenu, nPos, wFlags );
}


/**********************************************************************
 *         DeleteMenu32    (USER32.129)
 */
BOOL32 WINAPI DeleteMenu32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags )
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) return FALSE;
    if (item->fType & MF_POPUP) DestroyMenu32( item->hSubMenu );
      /* nPos is now the position of the item */
    RemoveMenu32( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}


/*******************************************************************
 *         ModifyMenu16    (USER.414)
 */
BOOL16 WINAPI ModifyMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                            UINT16 id, SEGPTR data )
{
    if (IS_STRING_ITEM(flags))
        return ModifyMenu32A( hMenu, pos, flags, id,
                              (LPSTR)PTR_SEG_TO_LIN(data) );
    return ModifyMenu32A( hMenu, pos, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         ModifyMenu32A    (USER32.397)
 */
BOOL32 WINAPI ModifyMenu32A( HMENU32 hMenu, UINT32 pos, UINT32 flags,
                             UINT32 id, LPCSTR str )
{
    MENUITEM *item;

    if (IS_STRING_ITEM(flags))
    {
	TRACE(menu, "%04x %d %04x %04x '%s'\n",
                      hMenu, pos, flags, id, str ? str : "#NULL#" );
        if (!str) return FALSE;
    }
    else
    {
	TRACE(menu, "%04x %d %04x %04x %08lx\n",
                      hMenu, pos, flags, id, (DWORD)str );
    }

    if (!(item = MENU_FindItem( &hMenu, &pos, flags ))) return FALSE;
    return MENU_SetItemData( item, flags, id, str );
}


/*******************************************************************
 *         ModifyMenu32W    (USER32.398)
 */
BOOL32 WINAPI ModifyMenu32W( HMENU32 hMenu, UINT32 pos, UINT32 flags,
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
HMENU16 WINAPI CreatePopupMenu16(void)
{
    return CreatePopupMenu32();
}


/**********************************************************************
 *         CreatePopupMenu32    (USER32.82)
 */
HMENU32 WINAPI CreatePopupMenu32(void)
{
    HMENU32 hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu32())) return 0;
    menu = (POPUPMENU *) USER_HEAP_LIN_ADDR( hmenu );
    menu->wFlags |= MF_POPUP;
    return hmenu;
}


/**********************************************************************
 *         GetMenuCheckMarkDimensions    (USER.417) (USER32.258)
 */
DWORD WINAPI GetMenuCheckMarkDimensions(void)
{
    return MAKELONG( check_bitmap_width, check_bitmap_height );
}


/**********************************************************************
 *         SetMenuItemBitmaps16    (USER.418)
 */
BOOL16 WINAPI SetMenuItemBitmaps16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags,
                                    HBITMAP16 hNewUnCheck, HBITMAP16 hNewCheck)
{
    return SetMenuItemBitmaps32( hMenu, nPos, wFlags, hNewUnCheck, hNewCheck );
}


/**********************************************************************
 *         SetMenuItemBitmaps32    (USER32.490)
 */
BOOL32 WINAPI SetMenuItemBitmaps32( HMENU32 hMenu, UINT32 nPos, UINT32 wFlags,
                                    HBITMAP32 hNewUnCheck, HBITMAP32 hNewCheck)
{
    MENUITEM *item;
    TRACE(menu,"(%04x, %04x, %04x, %04x, %04x)\n",
                 hMenu, nPos, wFlags, hNewCheck, hNewUnCheck);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;

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
    return TRUE;
}


/**********************************************************************
 *         CreateMenu16    (USER.151)
 */
HMENU16 WINAPI CreateMenu16(void)
{
    return CreateMenu32();
}


/**********************************************************************
 *         CreateMenu32    (USER32.81)
 */
HMENU32 WINAPI CreateMenu32(void)
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
    TRACE(menu, "return %04x\n", hMenu );
    return hMenu;
}


/**********************************************************************
 *         DestroyMenu16    (USER.152)
 */
BOOL16 WINAPI DestroyMenu16( HMENU16 hMenu )
{
    return DestroyMenu32( hMenu );
}


/**********************************************************************
 *         DestroyMenu32    (USER32.134)
 */
BOOL32 WINAPI DestroyMenu32( HMENU32 hMenu )
{
    TRACE(menu,"(%04x)\n", hMenu);

    /* Silently ignore attempts to destroy default system popup */

    if (hMenu && hMenu != MENU_DefSysPopup)
    {
	LPPOPUPMENU lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu);

	if( pTopPopupWnd && (hMenu == *(HMENU32*)pTopPopupWnd->wExtra) )
	  *(UINT32*)pTopPopupWnd->wExtra = 0;

	if (IS_A_MENU( lppop ))
	{
	    lppop->wMagic = 0;  /* Mark it as destroyed */

	    if ((lppop->wFlags & MF_POPUP) && lppop->hWnd &&
	        (!pTopPopupWnd || (lppop->hWnd != pTopPopupWnd->hwndSelf)))
	        DestroyWindow32( lppop->hWnd );

	    if (lppop->items)	/* recursively destroy submenus */
	    {
	        int i;
	        MENUITEM *item = lppop->items;
	        for (i = lppop->nItems; i > 0; i--, item++)
	        {
	            if (item->fType & MF_POPUP) DestroyMenu32(item->hSubMenu);
		    MENU_FreeItemData( item );
	        }
	        HeapFree( SystemHeap, 0, lppop->items );
	    }
	    USER_HEAP_FREE( hMenu );
	}
	else return FALSE;
    }
    return (hMenu != MENU_DefSysPopup);
}


/**********************************************************************
 *         GetSystemMenu16    (USER.156)
 */
HMENU16 WINAPI GetSystemMenu16( HWND16 hWnd, BOOL16 bRevert )
{
    return GetSystemMenu32( hWnd, bRevert );
}


/**********************************************************************
 *         GetSystemMenu32    (USER32.291)
 */
HMENU32 WINAPI GetSystemMenu32( HWND32 hWnd, BOOL32 bRevert )
{
    WND *wndPtr = WIN_FindWndPtr( hWnd );

    if (wndPtr)
    {
	if( wndPtr->hSysMenu )
	{
	    if( bRevert )
	    {
		DestroyMenu32(wndPtr->hSysMenu); 
		wndPtr->hSysMenu = 0;
	    }
	    else
	    {
		POPUPMENU *menu = (POPUPMENU*) 
			   USER_HEAP_LIN_ADDR(wndPtr->hSysMenu);
		if( menu->items[0].hSubMenu == MENU_DefSysPopup )
		    menu->items[0].hSubMenu = MENU_CopySysPopup();
	    }
	}

	if(!wndPtr->hSysMenu && (wndPtr->dwStyle & WS_SYSMENU) )
	    wndPtr->hSysMenu = MENU_GetSysMenu( hWnd, (HMENU32)(-1) );

	if( wndPtr->hSysMenu )
	    return GetSubMenu16(wndPtr->hSysMenu, 0);
    }
    return 0;
}


/*******************************************************************
 *         SetSystemMenu16    (USER.280)
 */
BOOL16 WINAPI SetSystemMenu16( HWND16 hwnd, HMENU16 hMenu )
{
    return SetSystemMenu32( hwnd, hMenu );
}


/*******************************************************************
 *         SetSystemMenu32    (USER32.508)
 */
BOOL32 WINAPI SetSystemMenu32( HWND32 hwnd, HMENU32 hMenu )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    if (wndPtr)
    {
	if (wndPtr->hSysMenu) DestroyMenu32( wndPtr->hSysMenu );
	wndPtr->hSysMenu = MENU_GetSysMenu( hwnd, hMenu );
	return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *         GetMenu16    (USER.157)
 */
HMENU16 WINAPI GetMenu16( HWND16 hWnd ) 
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD)) 
	return (HMENU16)wndPtr->wIDmenu;
    return 0;
}


/**********************************************************************
 *         GetMenu32    (USER32.257)
 */
HMENU32 WINAPI GetMenu32( HWND32 hWnd ) 
{ 
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD)) 
	return (HMENU32)wndPtr->wIDmenu;
    return 0;
}


/**********************************************************************
 *         SetMenu16    (USER.158)
 */
BOOL16 WINAPI SetMenu16( HWND16 hWnd, HMENU16 hMenu )
{
    return SetMenu32( hWnd, hMenu );
}


/**********************************************************************
 *         SetMenu32    (USER32.487)
 */
BOOL32 WINAPI SetMenu32( HWND32 hWnd, HMENU32 hMenu )
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);

    TRACE(menu,"(%04x, %04x);\n", hWnd, hMenu);

    if (wndPtr && !(wndPtr->dwStyle & WS_CHILD))
    {
	if (GetCapture32() == hWnd) ReleaseCapture();

	wndPtr->wIDmenu = (UINT32)hMenu;
	if (hMenu != 0)
	{
	    LPPOPUPMENU lpmenu;

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
    return FALSE;
}



/**********************************************************************
 *         GetSubMenu16    (USER.159)
 */
HMENU16 WINAPI GetSubMenu16( HMENU16 hMenu, INT16 nPos )
{
    return GetSubMenu32( hMenu, nPos );
}


/**********************************************************************
 *         GetSubMenu32    (USER32.288)
 */
HMENU32 WINAPI GetSubMenu32( HMENU32 hMenu, INT32 nPos )
{
    LPPOPUPMENU lppop;

    if (!(lppop = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hMenu))) return 0;
    if ((UINT32)nPos >= lppop->nItems) return 0;
    if (!(lppop->items[nPos].fType & MF_POPUP)) return 0;
    return lppop->items[nPos].hSubMenu;
}


/**********************************************************************
 *         DrawMenuBar16    (USER.160)
 */
void WINAPI DrawMenuBar16( HWND16 hWnd )
{
    DrawMenuBar32( hWnd );
}


/**********************************************************************
 *         DrawMenuBar32    (USER32.161)
 */
BOOL32 WINAPI DrawMenuBar32( HWND32 hWnd )
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
 *           EndMenu   (USER.187) (USER32.175)
 */
void WINAPI EndMenu(void)
{
    fEndMenu = TRUE;
}


/***********************************************************************
 *           LookupMenuHandle   (USER.217)
 */
HMENU16 WINAPI LookupMenuHandle( HMENU16 hmenu, INT16 id )
{
    HMENU32 hmenu32 = hmenu;
    INT32 id32 = id;
    if (!MENU_FindItem( &hmenu32, &id32, MF_BYCOMMAND )) return 0;
    else return hmenu32;
}


/**********************************************************************
 *	    LoadMenu16    (USER.150)
 */
HMENU16 WINAPI LoadMenu16( HINSTANCE16 instance, SEGPTR name )
{
    HRSRC16 hRsrc;
    HGLOBAL16 handle;
    HMENU16 hMenu;

    if (HIWORD(name))
    {
        char *str = (char *)PTR_SEG_TO_LIN( name );
        TRACE(menu, "(%04x,'%s')\n", instance, str );
        if (str[0] == '#') name = (SEGPTR)atoi( str + 1 );
    }
    else
        TRACE(resource,"(%04x,%04x)\n",instance,LOWORD(name));

    if (!name) return 0;
    
    /* check for Win32 module */
    if (HIWORD(instance))
        return LoadMenu32A(instance,PTR_SEG_TO_LIN(name));
    instance = GetExePtr( instance );

    if (!(hRsrc = FindResource16( instance, name, RT_MENU16 ))) return 0;
    if (!(handle = LoadResource16( instance, hRsrc ))) return 0;
    hMenu = LoadMenuIndirect16(LockResource16(handle));
    FreeResource16( handle );
    return hMenu;
}


/*****************************************************************
 *        LoadMenu32A   (USER32.370)
 */
HMENU32 WINAPI LoadMenu32A( HINSTANCE32 instance, LPCSTR name )
{
    HRSRC32 hrsrc = FindResource32A( instance, name, RT_MENU32A );
    if (!hrsrc) return 0;
    return LoadMenuIndirect32A( (LPCVOID)LoadResource32( instance, hrsrc ));
}


/*****************************************************************
 *        LoadMenu32W   (USER32.373)
 */
HMENU32 WINAPI LoadMenu32W( HINSTANCE32 instance, LPCWSTR name )
{
    HRSRC32 hrsrc = FindResource32W( instance, name, RT_MENU32W );
    if (!hrsrc) return 0;
    return LoadMenuIndirect32W( (LPCVOID)LoadResource32( instance, hrsrc ));
}


/**********************************************************************
 *	    LoadMenuIndirect16    (USER.220)
 */
HMENU16 WINAPI LoadMenuIndirect16( LPCVOID template )
{
    HMENU16 hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    TRACE(menu,"(%p)\n", template );
    version = GET_WORD(p);
    p += sizeof(WORD);
    if (version)
    {
        WARN(menu, "version must be 0 for Win16\n" );
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
 *	    LoadMenuIndirect32A    (USER32.371)
 */
HMENU32 WINAPI LoadMenuIndirect32A( LPCVOID template )
{
    HMENU16 hMenu;
    WORD version, offset;
    LPCSTR p = (LPCSTR)template;

    TRACE(menu,"%p\n", template );
    version = GET_WORD(p);
    p += sizeof(WORD);
    switch (version)
      {
      case 0:
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu32())) return 0;
	if (!MENU_ParseResource( p, hMenu, TRUE ))
	  {
	    DestroyMenu32( hMenu );
	    return 0;
	  }
	return hMenu;
      case 1:
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu32())) return 0;
	if (!MENUEX_ParseResource( p, hMenu))
	  {
	    DestroyMenu32( hMenu );
	    return 0;
	  }
	return hMenu;
      default:
        ERR(menu, "version %d not supported.\n", version);
        return 0;
      }
}


/**********************************************************************
 *	    LoadMenuIndirect32W    (USER32.372)
 */
HMENU32 WINAPI LoadMenuIndirect32W( LPCVOID template )
{
    /* FIXME: is there anything different between A and W? */
    return LoadMenuIndirect32A( template );
}


/**********************************************************************
 *		IsMenu16    (USER.358)
 */
BOOL16 WINAPI IsMenu16( HMENU16 hmenu )
{
    LPPOPUPMENU menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hmenu);
    return IS_A_MENU(menu);
}


/**********************************************************************
 *		IsMenu32    (USER32.346)
 */
BOOL32 WINAPI IsMenu32(HMENU32 hmenu)
{
    LPPOPUPMENU menu = (LPPOPUPMENU) USER_HEAP_LIN_ADDR(hmenu);
    return IS_A_MENU(menu);
}

/**********************************************************************
 *		GetMenuItemInfo32_common
 */

static BOOL32 GetMenuItemInfo32_common ( HMENU32 hmenu, UINT32 item,
					 BOOL32 bypos,
					 LPMENUITEMINFO32A lpmii,
					 BOOL32 unicode)
{
  MENUITEM *menu = MENU_FindItem (&hmenu, &item, bypos? MF_BYPOSITION : 0);
    debug_print_menuitem("GetMenuItemInfo32_common: ", menu, "");
    if (!menu)
	return FALSE;

    if (lpmii->fMask & MIIM_TYPE) {
	lpmii->fType = menu->fType;
	switch (MENU_ITEM_TYPE(menu->fType)) {
	case MF_STRING:
	    if (menu->text && lpmii->dwTypeData && lpmii->cch) {
	  if (unicode)
		    lstrcpynAtoW((LPWSTR) lpmii->dwTypeData,
				 menu->text,
				 lpmii->cch);
		else
		    lstrcpyn32A(lpmii->dwTypeData,
				menu->text,
				lpmii->cch);
	}
	    break;
	case MF_OWNERDRAW:
	case MF_BITMAP:
	    lpmii->dwTypeData = menu->text;
	    break;
	default:
	    break;
    }
    }
    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = menu->fState;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = menu->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
	lpmii->hSubMenu = menu->hSubMenu;

    if (lpmii->fMask & MIIM_CHECKMARKS) {
	lpmii->hbmpChecked = menu->hCheckBit;
	lpmii->hbmpUnchecked = menu->hUnCheckBit;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = menu->dwItemData;

  return TRUE;
}

/**********************************************************************
 *		GetMenuItemInfo32A    (USER32.264)
 */
BOOL32 WINAPI GetMenuItemInfo32A( HMENU32 hmenu, UINT32 item, BOOL32 bypos,
                                  LPMENUITEMINFO32A lpmii)
{
    return GetMenuItemInfo32_common (hmenu, item, bypos, lpmii, FALSE);
}

/**********************************************************************
 *		GetMenuItemInfo32W    (USER32.265)
 */
BOOL32 WINAPI GetMenuItemInfo32W( HMENU32 hmenu, UINT32 item, BOOL32 bypos,
                                  LPMENUITEMINFO32W lpmii)
{
    return GetMenuItemInfo32_common (hmenu, item, bypos,
                                     (LPMENUITEMINFO32A)lpmii, TRUE);
}

/**********************************************************************
 *		SetMenuItemInfo32_common
 */

static BOOL32 SetMenuItemInfo32_common(MENUITEM * menu,
				       const MENUITEMINFO32A *lpmii,
				       BOOL32 unicode)
{
    if (!menu) return FALSE;

    if (lpmii->fMask & MIIM_TYPE) {
	/* Get rid of old string.  */
	if (IS_STRING_ITEM(menu->fType) && menu->text)
	    HeapFree(SystemHeap, 0, menu->text);

	menu->fType = lpmii->fType;
	menu->text = lpmii->dwTypeData;
	if (IS_STRING_ITEM(menu->fType) && menu->text) {
	    menu->text =
		unicode
		? HEAP_strdupWtoA(SystemHeap, 0,
				  (LPWSTR) lpmii->dwTypeData)
		: HEAP_strdupA(SystemHeap, 0, lpmii->dwTypeData);
	}
    }
    if (lpmii->fMask & MIIM_STATE)
	menu->fState = lpmii->fState;

    if (lpmii->fMask & MIIM_ID)
	menu->wID = lpmii->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
	menu->hSubMenu = lpmii->hSubMenu;

    if (lpmii->fMask & MIIM_CHECKMARKS)
    {
	menu->hCheckBit = lpmii->hbmpChecked;
	menu->hUnCheckBit = lpmii->hbmpUnchecked;
    }
    if (lpmii->fMask & MIIM_DATA)
	menu->dwItemData = lpmii->dwItemData;

    debug_print_menuitem("SetMenuItemInfo32_common: ", menu, "");
    return TRUE;
}

/**********************************************************************
 *		SetMenuItemInfo32A    (USER32.491)
 */
BOOL32 WINAPI SetMenuItemInfo32A(HMENU32 hmenu, UINT32 item, BOOL32 bypos,
                                 const MENUITEMINFO32A *lpmii) 
{
    return SetMenuItemInfo32_common(MENU_FindItem(&hmenu, &item, bypos? MF_BYPOSITION : 0),
				    lpmii, FALSE);
}

/**********************************************************************
 *		SetMenuItemInfo32W    (USER32.492)
 */
BOOL32 WINAPI SetMenuItemInfo32W(HMENU32 hmenu, UINT32 item, BOOL32 bypos,
                                 const MENUITEMINFO32W *lpmii)
{
    return SetMenuItemInfo32_common(MENU_FindItem(&hmenu, &item, bypos? MF_BYPOSITION : 0),
				    (const MENUITEMINFO32A*)lpmii, TRUE);
}

/**********************************************************************
 *		SetMenuDefaultItem32    (USER32.489)
 */
BOOL32 WINAPI SetMenuDefaultItem32(HMENU32 hmenu, UINT32 item, BOOL32 bypos)
{
    MENUITEM *menuitem = MENU_FindItem(&hmenu, &item, bypos);
    POPUPMENU *menu;

    if (!menuitem) return FALSE;
    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR(hmenu))) return FALSE;

    menu->defitem = item; /* position */

    debug_print_menuitem("SetMenuDefaultItem32: ", menuitem, "");
    FIXME(menu, "(0x%x,%d,%d), empty stub!\n",
		  hmenu, item, bypos);
    return TRUE;
}

/**********************************************************************
 *		GetMenuDefaultItem32    (USER32.260)
 */
UINT32 WINAPI GetMenuDefaultItem32(HMENU32 hmenu, UINT32 bypos, UINT32 flags)
{
    POPUPMENU *menu;

    if (!(menu = (POPUPMENU *) USER_HEAP_LIN_ADDR(hmenu))) return 0; /*FIXME*/

    FIXME(menu, "(0x%x,%d,%d), stub!\n", hmenu, bypos, flags);
    if (bypos & MF_BYPOSITION)
    	return menu->defitem;
    else
    	return menu->items[menu->defitem].wID;
}

/*******************************************************************
 *              InsertMenuItem16   (USER.441)
 *
 * FIXME: untested
 */
BOOL16 WINAPI InsertMenuItem16( HMENU16 hmenu, UINT16 pos, BOOL16 byposition,
                                const MENUITEMINFO16 *mii )
{
    MENUITEMINFO32A miia;

    miia.cbSize        = sizeof(miia);
    miia.fMask         = mii->fMask;
    miia.dwTypeData    = mii->dwTypeData;
    miia.fType         = mii->fType;
    miia.fState        = mii->fState;
    miia.wID           = mii->wID;
    miia.hSubMenu      = mii->hSubMenu;
    miia.hbmpChecked   = mii->hbmpChecked;
    miia.hbmpUnchecked = mii->hbmpUnchecked;
    miia.dwItemData    = mii->dwItemData;
    miia.cch           = mii->cch;
    if (IS_STRING_ITEM(miia.fType))
        miia.dwTypeData = PTR_SEG_TO_LIN(miia.dwTypeData);
    return InsertMenuItem32A( hmenu, pos, byposition, &miia );
}


/**********************************************************************
 *		InsertMenuItem32A    (USER32.323)
 */
BOOL32 WINAPI InsertMenuItem32A(HMENU32 hMenu, UINT32 uItem, BOOL32 bypos,
                                const MENUITEMINFO32A *lpmii)
{
    MENUITEM *item = MENU_InsertItem(hMenu, uItem, bypos ? MF_BYPOSITION : 0 );
    return SetMenuItemInfo32_common(item, lpmii, FALSE);
}


/**********************************************************************
 *		InsertMenuItem32W    (USER32.324)
 */
BOOL32 WINAPI InsertMenuItem32W(HMENU32 hMenu, UINT32 uItem, BOOL32 bypos,
                                const MENUITEMINFO32W *lpmii)
{
    MENUITEM *item = MENU_InsertItem(hMenu, uItem, bypos ? MF_BYPOSITION : 0 );
    return SetMenuItemInfo32_common(item, (const MENUITEMINFO32A*)lpmii, TRUE);
}

/**********************************************************************
 *		CheckMenuRadioItem32    (USER32.47)
 */

BOOL32 WINAPI CheckMenuRadioItem32(HMENU32 hMenu,
				   UINT32 first, UINT32 last, UINT32 check,
				   BOOL32 bypos)
{
     MENUITEM *mifirst, *milast, *micheck;
     HMENU32 mfirst = hMenu, mlast = hMenu, mcheck = hMenu;

     TRACE(menu, "ox%x: %d-%d, check %d, bypos=%d\n",
		  hMenu, first, last, check, bypos);

     mifirst = MENU_FindItem (&mfirst, &first, bypos);
     milast = MENU_FindItem (&mlast, &last, bypos);
     micheck = MENU_FindItem (&mcheck, &check, bypos);

     if (mifirst == NULL || milast == NULL || micheck == NULL ||
	 mifirst > milast || mfirst != mlast || mfirst != mcheck ||
	 micheck > milast || micheck < mifirst)
	  return FALSE;

     while (mifirst <= milast)
     {
	  if (mifirst == micheck)
	  {
	       mifirst->fType |= MFT_RADIOCHECK;
	       mifirst->fState |= MFS_CHECKED;
	  } else {
	       mifirst->fType &= ~MFT_RADIOCHECK;
	       mifirst->fState &= ~MFS_CHECKED;
	  }
	  mifirst++;
     }

     return TRUE;
}

/**********************************************************************
 *		CheckMenuRadioItem16    (not a Windows API)
 */

BOOL16 WINAPI CheckMenuRadioItem16(HMENU16 hMenu,
				   UINT16 first, UINT16 last, UINT16 check,
				   BOOL16 bypos)
{
     return CheckMenuRadioItem32 (hMenu, first, last, check, bypos);
}

/**********************************************************************
 *		GetMenuItemRect32    (USER32.266)
 */

BOOL32 WINAPI GetMenuItemRect32 (HWND32 hwnd, HMENU32 hMenu, UINT32 uItem,
				 LPRECT32 rect)
{
     RECT32 saverect, clientrect;
     BOOL32 barp;
     HDC32 hdc;
     WND *wndPtr;
     MENUITEM *item;
     HMENU32 orghMenu = hMenu;

     TRACE(menu, "(0x%x,0x%x,%d,%p)\n",
		  hwnd, hMenu, uItem, rect);

     item = MENU_FindItem (&hMenu, &uItem, MF_BYPOSITION);
     wndPtr = WIN_FindWndPtr (hwnd);
     if (!rect || !item || !wndPtr) return FALSE;

     GetClientRect32( hwnd, &clientrect );
     hdc = GetDCEx32( hwnd, 0, DCX_CACHE | DCX_WINDOW );
     barp = (hMenu == orghMenu);

     saverect = item->rect;
     MENU_CalcItemSize (hdc, item, hwnd,
			clientrect.left, clientrect.top, barp);
     *rect = item->rect;
     item->rect = saverect;

     ReleaseDC32( hwnd, hdc );
     return TRUE;
}

/**********************************************************************
 *		GetMenuItemRect16    (USER.665)
 */

BOOL16 WINAPI GetMenuItemRect16 (HWND16 hwnd, HMENU16 hMenu, UINT16 uItem,
				 LPRECT16 rect)
{
     RECT32 r32;
     BOOL32 res;

     if (!rect) return FALSE;
     res = GetMenuItemRect32 (hwnd, hMenu, uItem, &r32);
     CONV_RECT32TO16 (&r32, rect);
     return res;
}
