/*
 * Listbox controls
 *
 * Copyright 1996 Alexandre Julliard
 */

#include <string.h>
#include <stdlib.h>
#include "wine/winuser16.h"
#include "winuser.h"
#include "winerror.h"
#include "drive.h"
#include "heap.h"
#include "spy.h"
#include "win.h"
#include "combo.h"
#include "debug.h"
#include "tweak.h"

DECLARE_DEBUG_CHANNEL(combo)
DECLARE_DEBUG_CHANNEL(listbox)

/* Unimplemented yet:
 * - LBS_NOSEL
 * - LBS_USETABSTOPS
 * - Unicode
 * - Locale handling
 */

/* Items array granularity */
#define LB_ARRAY_GRANULARITY 16

/* Scrolling timeout in ms */
#define LB_SCROLL_TIMEOUT 50

/* Listbox system timer id */
#define LB_TIMER_ID  2

/* Item structure */
typedef struct
{
    LPSTR     str;       /* Item text */
    BOOL    selected;  /* Is item selected? */
    UINT    height;    /* Item height (only for OWNERDRAWVARIABLE) */
    DWORD     data;      /* User data */
} LB_ITEMDATA;

/* Listbox structure */
typedef struct
{
    HANDLE      heap;           /* Heap for this listbox */
    HWND        owner;          /* Owner window to send notifications to */
    UINT        style;          /* Window style */
    INT         width;          /* Window width */
    INT         height;         /* Window height */
    LB_ITEMDATA  *items;          /* Array of items */
    INT         nb_items;       /* Number of items */
    INT         top_item;       /* Top visible item */
    INT         selected_item;  /* Selected item */
    INT         focus_item;     /* Item that has the focus */
    INT         anchor_item;    /* Anchor item for extended selection */
    INT         item_height;    /* Default item height */
    INT         page_size;      /* Items per listbox page */
    INT         column_width;   /* Column width for multi-column listboxes */
    INT         horz_extent;    /* Horizontal extent (0 if no hscroll) */
    INT         horz_pos;       /* Horizontal position */
    INT         nb_tabs;        /* Number of tabs in array */
    INT        *tabs;           /* Array of tabs */
    BOOL        caret_on;       /* Is caret on? */
    BOOL        captured;       /* Is mouse captured? */
    HFONT       font;           /* Current font */
    LCID          locale;         /* Current locale for string comparisons */
    LPHEADCOMBO   lphc;		  /* ComboLBox */
} LB_DESCR;


#define IS_OWNERDRAW(descr) \
    ((descr)->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))

#define HAS_STRINGS(descr) \
    (!IS_OWNERDRAW(descr) || ((descr)->style & LBS_HASSTRINGS))

#define SEND_NOTIFICATION(wnd,descr,code) \
    (SendMessageA( (descr)->owner, WM_COMMAND, \
     MAKEWPARAM((((descr)->lphc)?ID_CB_LISTBOX:(wnd)->wIDmenu), (code) ), (wnd)->hwndSelf ))

/* Current timer status */
typedef enum
{
    LB_TIMER_NONE,
    LB_TIMER_UP,
    LB_TIMER_LEFT,
    LB_TIMER_DOWN,
    LB_TIMER_RIGHT
} TIMER_DIRECTION;

static TIMER_DIRECTION LISTBOX_Timer = LB_TIMER_NONE;


/***********************************************************************
 *           LISTBOX_Dump
 */
void LISTBOX_Dump( WND *wnd )
{
    INT i;
    LB_ITEMDATA *item;
    LB_DESCR *descr = *(LB_DESCR **)wnd->wExtra;

    DUMP( "Listbox:\n" );
    DUMP( "hwnd=%04x descr=%08x heap=%08x items=%d top=%d\n",
	  wnd->hwndSelf, (UINT)descr, descr->heap, descr->nb_items,
	  descr->top_item );
    for (i = 0, item = descr->items; i < descr->nb_items; i++, item++)
    {
        DUMP( "%4d: %-40s %d %08lx %3d\n",
	      i, item->str, item->selected, item->data, item->height );
    }
}


/***********************************************************************
 *           LISTBOX_GetCurrentPageSize
 *
 * Return the current page size
 */
static INT LISTBOX_GetCurrentPageSize( WND *wnd, LB_DESCR *descr )
{
    INT i, height;
    if (!(descr->style & LBS_OWNERDRAWVARIABLE)) return descr->page_size;
    for (i = descr->top_item, height = 0; i < descr->nb_items; i++)
    {
        if ((height += descr->items[i].height) > descr->height) break;
    }
    if (i == descr->top_item) return 1;
    else return i - descr->top_item;
}


/***********************************************************************
 *           LISTBOX_GetMaxTopIndex
 *
 * Return the maximum possible index for the top of the listbox.
 */
static INT LISTBOX_GetMaxTopIndex( WND *wnd, LB_DESCR *descr )
{
    INT max, page;

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        page = descr->height;
        for (max = descr->nb_items - 1; max >= 0; max--)
            if ((page -= descr->items[max].height) < 0) break;
        if (max < descr->nb_items - 1) max++;
    }
    else if (descr->style & LBS_MULTICOLUMN)
    {
        if ((page = descr->width / descr->column_width) < 1) page = 1;
        max = (descr->nb_items + descr->page_size - 1) / descr->page_size;
        max = (max - page) * descr->page_size;
    }
    else
    {
        max = descr->nb_items - descr->page_size;
    }
    if (max < 0) max = 0;
    return max;
}


/***********************************************************************
 *           LISTBOX_UpdateScroll
 *
 * Update the scrollbars. Should be called whenever the content
 * of the listbox changes.
 */
static void LISTBOX_UpdateScroll( WND *wnd, LB_DESCR *descr )
{
    SCROLLINFO info;

    if (!(descr->style & WS_VSCROLL)) return; 
    /*   It is important that we check descr->style, and not wnd->dwStyle, 
       for WS_VSCROLL, as the former is exactly the one passed in 
       argument to CreateWindow.  
         In Windows (and from now on in Wine :) a listbox created 
       with such a style (no WS_SCROLL) does not update 
       the scrollbar with listbox-related data, thus letting 
       the programmer use it for his/her own purposes. */

    if (descr->style & LBS_NOREDRAW) return;
    info.cbSize = sizeof(info);

    if (descr->style & LBS_MULTICOLUMN)
    {
        info.nMin  = 0;
        info.nMax  = (descr->nb_items - 1) / descr->page_size;
        info.nPos  = descr->top_item / descr->page_size;
        info.nPage = descr->width / descr->column_width;
        if (info.nPage < 1) info.nPage = 1;
        info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        if (descr->style & LBS_DISABLENOSCROLL)
            info.fMask |= SIF_DISABLENOSCROLL;
        SetScrollInfo( wnd->hwndSelf, SB_HORZ, &info, TRUE );
        info.nMax = 0;
        info.fMask = SIF_RANGE;
        SetScrollInfo( wnd->hwndSelf, SB_VERT, &info, TRUE );
    }
    else
    {
        info.nMin  = 0;
        info.nMax  = descr->nb_items - 1;
        info.nPos  = descr->top_item;
        info.nPage = LISTBOX_GetCurrentPageSize( wnd, descr );
        info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        if (descr->style & LBS_DISABLENOSCROLL)
            info.fMask |= SIF_DISABLENOSCROLL;
        SetScrollInfo( wnd->hwndSelf, SB_VERT, &info, TRUE );

        if (descr->horz_extent)
        {
            info.nMin  = 0;
            info.nMax  = descr->horz_extent - 1;
            info.nPos  = descr->horz_pos;
            info.nPage = descr->width;
            info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            if (descr->style & LBS_DISABLENOSCROLL)
                info.fMask |= SIF_DISABLENOSCROLL;
            SetScrollInfo( wnd->hwndSelf, SB_HORZ, &info, TRUE );
        }
    }
}


/***********************************************************************
 *           LISTBOX_SetTopItem
 *
 * Set the top item of the listbox, scrolling up or down if necessary.
 */
static LRESULT LISTBOX_SetTopItem( WND *wnd, LB_DESCR *descr, INT index,
                                   BOOL scroll )
{
    INT max = LISTBOX_GetMaxTopIndex( wnd, descr );
    if (index > max) index = max;
    if (index < 0) index = 0;
    if (descr->style & LBS_MULTICOLUMN) index -= index % descr->page_size;
    if (descr->top_item == index) return LB_OKAY;
    if (descr->style & LBS_MULTICOLUMN)
    {
        INT diff = (descr->top_item - index) / descr->page_size * descr->column_width;
        if (scroll && (abs(diff) < descr->width))
            ScrollWindowEx( wnd->hwndSelf, diff, 0, NULL, NULL, 0, NULL, 
                              SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );

        else
            scroll = FALSE;
    }
    else if (scroll)
    {
        INT diff;
        if (descr->style & LBS_OWNERDRAWVARIABLE)
        {
            INT i;
            diff = 0;
            if (index > descr->top_item)
            {
                for (i = index - 1; i >= descr->top_item; i--)
                    diff -= descr->items[i].height;
            }
            else
            {
                for (i = index; i < descr->top_item; i++)
                    diff += descr->items[i].height;
            }
        }
        else 
            diff = (descr->top_item - index) * descr->item_height;

        if (abs(diff) < descr->height)
            ScrollWindowEx( wnd->hwndSelf, 0, diff, NULL, NULL, 0, NULL,
                              SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
        else
            scroll = FALSE;
    }
    if (!scroll) InvalidateRect( wnd->hwndSelf, NULL, TRUE );
    descr->top_item = index;
    LISTBOX_UpdateScroll( wnd, descr );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_UpdatePage
 *
 * Update the page size. Should be called when the size of
 * the client area or the item height changes.
 */
static void LISTBOX_UpdatePage( WND *wnd, LB_DESCR *descr )
{
    INT page_size;

    if ((page_size = descr->height / descr->item_height) < 1) page_size = 1;
    if (page_size == descr->page_size) return;
    descr->page_size = page_size;
    if (descr->style & LBS_MULTICOLUMN)
        InvalidateRect( wnd->hwndSelf, NULL, TRUE );
    LISTBOX_SetTopItem( wnd, descr, descr->top_item, FALSE );
}


/***********************************************************************
 *           LISTBOX_UpdateSize
 *
 * Update the size of the listbox. Should be called when the size of
 * the client area changes.
 */
static void LISTBOX_UpdateSize( WND *wnd, LB_DESCR *descr )
{
    RECT rect;

    GetClientRect( wnd->hwndSelf, &rect );
    descr->width  = rect.right - rect.left;
    descr->height = rect.bottom - rect.top;
    if (!(descr->style & LBS_NOINTEGRALHEIGHT) && !IS_OWNERDRAW(descr))
    {
        if ((descr->height > descr->item_height) &&
            (descr->height % descr->item_height))
        {
            TRACE(listbox, "[%04x]: changing height %d -> %d\n",
			 wnd->hwndSelf, descr->height,
			 descr->height - descr->height%descr->item_height );
            SetWindowPos( wnd->hwndSelf, 0, 0, 0,
                            wnd->rectWindow.right - wnd->rectWindow.left,
                            wnd->rectWindow.bottom - wnd->rectWindow.top -
                                (descr->height % descr->item_height),
                            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );
            return;
        }
    }
    TRACE(listbox, "[%04x]: new size = %d,%d\n",
		 wnd->hwndSelf, descr->width, descr->height );
    LISTBOX_UpdatePage( wnd, descr );
    LISTBOX_UpdateScroll( wnd, descr );
}


/***********************************************************************
 *           LISTBOX_GetItemRect
 *
 * Get the rectangle enclosing an item, in listbox client coordinates.
 * Return 1 if the rectangle is (partially) visible, 0 if hidden, -1 on error.
 */
static LRESULT LISTBOX_GetItemRect( WND *wnd, LB_DESCR *descr, INT index,
                                    RECT *rect )
{
    /* Index <= 0 is legal even on empty listboxes */
    if (index && (index >= descr->nb_items)) return -1;
    SetRect( rect, 0, 0, descr->width, descr->height );
    if (descr->style & LBS_MULTICOLUMN)
    {
        INT col = (index / descr->page_size) -
                        (descr->top_item / descr->page_size);
        rect->left += col * descr->column_width;
        rect->right = rect->left + descr->column_width;
        rect->top += (index % descr->page_size) * descr->item_height;
        rect->bottom = rect->top + descr->item_height;
    }
    else if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT i;
        rect->right += descr->horz_pos;
        if ((index >= 0) && (index < descr->nb_items))
        {
            if (index < descr->top_item)
            {
                for (i = descr->top_item-1; i >= index; i--)
                    rect->top -= descr->items[i].height;
            }
            else
            {
                for (i = descr->top_item; i < index; i++)
                    rect->top += descr->items[i].height;
            }
            rect->bottom = rect->top + descr->items[index].height;

        }
    }
    else
    {
        rect->top += (index - descr->top_item) * descr->item_height;
        rect->bottom = rect->top + descr->item_height;
        rect->right += descr->horz_pos;
    }

    return ((rect->left < descr->width) && (rect->right > 0) &&
            (rect->top < descr->height) && (rect->bottom > 0));
}


/***********************************************************************
 *           LISTBOX_GetItemFromPoint
 *
 * Return the item nearest from point (x,y) (in client coordinates).
 */
static INT LISTBOX_GetItemFromPoint( WND *wnd, LB_DESCR *descr,
                                       INT x, INT y )
{
    INT index = descr->top_item;

    if (!descr->nb_items) return -1;  /* No items */
    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT pos = 0;
        if (y >= 0)
        {
            while (index < descr->nb_items)
            {
                if ((pos += descr->items[index].height) > y) break;
                index++;
            }
        }
        else
        {
            while (index > 0)
            {
                index--;
                if ((pos -= descr->items[index].height) <= y) break;
            }
        }
    }
    else if (descr->style & LBS_MULTICOLUMN)
    {
        if (y >= descr->item_height * descr->page_size) return -1;
        if (y >= 0) index += y / descr->item_height;
        if (x >= 0) index += (x / descr->column_width) * descr->page_size;
        else index -= (((x + 1) / descr->column_width) - 1) * descr->page_size;
    }
    else
    {
        index += (y / descr->item_height);
    }
    if (index < 0) return 0;
    if (index >= descr->nb_items) return -1;
    return index;
}


/***********************************************************************
 *           LISTBOX_PaintItem
 *
 * Paint an item.
 */
static void LISTBOX_PaintItem( WND *wnd, LB_DESCR *descr, HDC hdc,
                               const RECT *rect, INT index, UINT action )
{
    LB_ITEMDATA *item = NULL;
    if (index < descr->nb_items) item = &descr->items[index];

    if (IS_OWNERDRAW(descr))
    {
        DRAWITEMSTRUCT dis;
	UINT		 id = (descr->lphc) ? ID_CB_LISTBOX : wnd->wIDmenu;

	if (!item)
	{
	    if (action == ODA_FOCUS) 
		DrawFocusRect( hdc, rect );
	    else
	        FIXME(listbox,"called with an out of bounds index %d(%d) in owner draw, Not good.\n",index,descr->nb_items);
	    return;
	}
        dis.CtlType      = ODT_LISTBOX;
        dis.CtlID        = id;
        dis.hwndItem     = wnd->hwndSelf;
        dis.itemAction   = action;
        dis.hDC          = hdc;
        dis.itemID       = index;
        dis.itemState    = 0;
        if (item && item->selected) dis.itemState |= ODS_SELECTED;
        if ((descr->focus_item == index) &&
            (descr->caret_on) &&
            (GetFocus() == wnd->hwndSelf)) dis.itemState |= ODS_FOCUS;
        if (wnd->dwStyle & WS_DISABLED) dis.itemState |= ODS_DISABLED;
        dis.itemData     = item ? item->data : 0;
        dis.rcItem       = *rect;
        TRACE(listbox, "[%04x]: drawitem %d (%s) action=%02x "
		     "state=%02x rect=%d,%d-%d,%d\n",
		     wnd->hwndSelf, index, item ? item->str : "", action,
		     dis.itemState, rect->left, rect->top,
		     rect->right, rect->bottom );
        SendMessageA(descr->owner, WM_DRAWITEM, id, (LPARAM)&dis);
    }
    else
    {
        COLORREF oldText = 0, oldBk = 0;

        if (action == ODA_FOCUS)
        {
            DrawFocusRect( hdc, rect );
            return;
        }
        if (item && item->selected)
        {
            oldBk = SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
            oldText = SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }

        TRACE(listbox, "[%04x]: painting %d (%s) action=%02x "
		     "rect=%d,%d-%d,%d\n",
		     wnd->hwndSelf, index, item ? item->str : "", action,
		     rect->left, rect->top, rect->right, rect->bottom );
        if (!item)
            ExtTextOutA( hdc, rect->left + 1, rect->top + 1,
                           ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
        else if (!(descr->style & LBS_USETABSTOPS)) 
	    ExtTextOutA( hdc, rect->left + 1, rect->top + 1,
			   ETO_OPAQUE | ETO_CLIPPED, rect, item->str,
			   strlen(item->str), NULL );
        else
	{
	    /* Output empty string to paint background in the full width. */
	    ExtTextOutA( hdc, rect->left + 1, rect->top + 1,
                           ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
	    TabbedTextOutA( hdc, rect->left + 1 , rect->top + 1,
			      item->str, strlen(item->str), 
			      descr->nb_tabs, descr->tabs, 0);
	}
        if (item && item->selected)
        {
            SetBkColor( hdc, oldBk );
            SetTextColor( hdc, oldText );
        }
        if ((descr->focus_item == index) &&
            (descr->caret_on) &&
            (GetFocus() == wnd->hwndSelf)) DrawFocusRect( hdc, rect );
    }
}


/***********************************************************************
 *           LISTBOX_SetRedraw
 *
 * Change the redraw flag.
 */
static void LISTBOX_SetRedraw( WND *wnd, LB_DESCR *descr, BOOL on )
{
    if (on)
    {
        if (!(descr->style & LBS_NOREDRAW)) return;
        descr->style &= ~LBS_NOREDRAW;
        LISTBOX_UpdateScroll( wnd, descr );
    }
    else descr->style |= LBS_NOREDRAW;
}


/***********************************************************************
 *           LISTBOX_RepaintItem
 *
 * Repaint a single item synchronously.
 */
static void LISTBOX_RepaintItem( WND *wnd, LB_DESCR *descr, INT index,
                                 UINT action )
{
    HDC hdc;
    RECT rect;
    HFONT oldFont = 0;
    HBRUSH hbrush, oldBrush = 0;

    if (descr->style & LBS_NOREDRAW) return;
    if (LISTBOX_GetItemRect( wnd, descr, index, &rect ) != 1) return;
    if (!(hdc = GetDCEx( wnd->hwndSelf, 0, DCX_CACHE ))) return;
    if (descr->font) oldFont = SelectObject( hdc, descr->font );
    hbrush = SendMessageA( descr->owner, WM_CTLCOLORLISTBOX,
                             hdc, (LPARAM)wnd->hwndSelf );
    if (hbrush) oldBrush = SelectObject( hdc, hbrush );
    if (wnd->dwStyle & WS_DISABLED)
        SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
    SetWindowOrgEx( hdc, descr->horz_pos, 0, NULL );
    LISTBOX_PaintItem( wnd, descr, hdc, &rect, index, action );
    if (oldFont) SelectObject( hdc, oldFont );
    if (oldBrush) SelectObject( hdc, oldBrush );
    ReleaseDC( wnd->hwndSelf, hdc );
}


/***********************************************************************
 *           LISTBOX_InitStorage
 */
static LRESULT LISTBOX_InitStorage( WND *wnd, LB_DESCR *descr, INT nb_items,
                                    DWORD bytes )
{
    LB_ITEMDATA *item;

    nb_items += LB_ARRAY_GRANULARITY - 1;
    nb_items -= (nb_items % LB_ARRAY_GRANULARITY);
    if (descr->items)
        nb_items += HeapSize( descr->heap, 0, descr->items ) / sizeof(*item);
    if (!(item = HeapReAlloc( descr->heap, 0, descr->items,
                              nb_items * sizeof(LB_ITEMDATA) )))
    {
        SEND_NOTIFICATION( wnd, descr, LBN_ERRSPACE );
        return LB_ERRSPACE;
    }
    descr->items = item;
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetTabStops
 */
static BOOL LISTBOX_SetTabStops( WND *wnd, LB_DESCR *descr, INT count,
                                   LPINT tabs, BOOL short_ints )
{
    if (!(descr->style & LBS_USETABSTOPS)) return TRUE;
    if (descr->tabs) HeapFree( descr->heap, 0, descr->tabs );
    if (!(descr->nb_tabs = count))
    {
        descr->tabs = NULL;
        return TRUE;
    }
    /* FIXME: count = 1 */
    if (!(descr->tabs = (INT *)HeapAlloc( descr->heap, 0,
                                            descr->nb_tabs * sizeof(INT) )))
        return FALSE;
    if (short_ints)
    {
        INT i;
        LPINT16 p = (LPINT16)tabs;
	dbg_decl_str(listbox, 256);

        for (i = 0; i < descr->nb_tabs; i++) {
	    descr->tabs[i] = *p++<<1; /* FIXME */
	    if(TRACE_ON(listbox))
              dsprintf(listbox, "%hd ", descr->tabs[i]);
	}
        TRACE(listbox, "[%04x]: settabstops %s\n", 
		     wnd->hwndSelf, dbg_str(listbox));
    }
    else memcpy( descr->tabs, tabs, descr->nb_tabs * sizeof(INT) );
    /* FIXME: repaint the window? */
    return TRUE;
}


/***********************************************************************
 *           LISTBOX_GetText
 */
static LRESULT LISTBOX_GetText( WND *wnd, LB_DESCR *descr, INT index,
                                LPSTR buffer )
{
    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
    if (HAS_STRINGS(descr))
    {
        if (!buffer)
		return strlen(descr->items[index].str);
        lstrcpyA( buffer, descr->items[index].str );
        return strlen(buffer);
    } else {
    	if (buffer)
        	*((LPDWORD)buffer)=*(LPDWORD)(&descr->items[index].data);
        return sizeof(DWORD);
    }
}


/***********************************************************************
 *           LISTBOX_FindStringPos
 *
 * Find the nearest string located before a given string in sort order.
 * If 'exact' is TRUE, return an error if we don't get an exact match.
 */
static INT LISTBOX_FindStringPos( WND *wnd, LB_DESCR *descr, LPCSTR str,
                                    BOOL exact )
{
    INT index, min, max, res = -1;

    if (!(descr->style & LBS_SORT)) return -1;  /* Add it at the end */
    min = 0;
    max = descr->nb_items;
    while (min != max)
    {
        index = (min + max) / 2;
        if (HAS_STRINGS(descr))
            res = lstrcmpiA( descr->items[index].str, str );
        else
        {
            COMPAREITEMSTRUCT cis;
	    UINT		id = (descr->lphc) ? ID_CB_LISTBOX : wnd->wIDmenu;

            cis.CtlType    = ODT_LISTBOX;
            cis.CtlID      = id;
            cis.hwndItem   = wnd->hwndSelf;
            cis.itemID1    = index;
            cis.itemData1  = descr->items[index].data;
            cis.itemID2    = -1;
            cis.itemData2  = (DWORD)str;
            cis.dwLocaleId = descr->locale;
            res = SendMessageA( descr->owner, WM_COMPAREITEM,
                                  id, (LPARAM)&cis );
        }
        if (!res) return index;
        if (res > 0) max = index;
        else min = index + 1;
    }
    return exact ? -1 : max;
}


/***********************************************************************
 *           LISTBOX_FindFileStrPos
 *
 * Find the nearest string located before a given string in directory
 * sort order (i.e. first files, then directories, then drives).
 */
static INT LISTBOX_FindFileStrPos( WND *wnd, LB_DESCR *descr, LPCSTR str )
{
    INT min, max, res = -1;

    if (!HAS_STRINGS(descr))
        return LISTBOX_FindStringPos( wnd, descr, str, FALSE );
    min = 0;
    max = descr->nb_items;
    while (min != max)
    {
        INT index = (min + max) / 2;
        const char *p = descr->items[index].str;
        if (*p == '[')  /* drive or directory */
        {
            if (*str != '[') res = -1;
            else if (p[1] == '-')  /* drive */
            {
                if (str[1] == '-') res = str[2] - p[2];
                else res = -1;
            }
            else  /* directory */
            {
                if (str[1] == '-') res = 1;
                else res = lstrcmpiA( str, p );
            }
        }
        else  /* filename */
        {
            if (*str == '[') res = 1;
            else res = lstrcmpiA( str, p );
        }
        if (!res) return index;
        if (res < 0) max = index;
        else min = index + 1;
    }
    return max;
}


/***********************************************************************
 *           LISTBOX_FindString
 *
 * Find the item beginning with a given string.
 */
static INT LISTBOX_FindString( WND *wnd, LB_DESCR *descr, INT start,
                                 LPCSTR str, BOOL exact )
{
    INT i;
    LB_ITEMDATA *item;

    if (start >= descr->nb_items) start = -1;
    item = descr->items + start + 1;
    if (HAS_STRINGS(descr))
    {
        if (!str) return LB_ERR;
        if (exact)
        {
            for (i = start + 1; i < descr->nb_items; i++, item++)
                if (!lstrcmpiA( str, item->str )) return i;
            for (i = 0, item = descr->items; i <= start; i++, item++)
                if (!lstrcmpiA( str, item->str )) return i;
        }
        else
        {
 /* Special case for drives and directories: ignore prefix */
#define CHECK_DRIVE(item) \
    if ((item)->str[0] == '[') \
    { \
        if (!lstrncmpiA( str, (item)->str+1, len )) return i; \
        if (((item)->str[1] == '-') && !lstrncmpiA(str,(item)->str+2,len)) \
        return i; \
    }

            INT len = strlen(str);
            for (i = start + 1; i < descr->nb_items; i++, item++)
            {
               if (!lstrncmpiA( str, item->str, len )) return i;
               CHECK_DRIVE(item);
            }
            for (i = 0, item = descr->items; i <= start; i++, item++)
            {
               if (!lstrncmpiA( str, item->str, len )) return i;
               CHECK_DRIVE(item);
            }
#undef CHECK_DRIVE
        }
    }
    else
    {
        if (exact && (descr->style & LBS_SORT))
            /* If sorted, use a WM_COMPAREITEM binary search */
            return LISTBOX_FindStringPos( wnd, descr, str, TRUE );

        /* Otherwise use a linear search */
        for (i = start + 1; i < descr->nb_items; i++, item++)
            if (item->data == (DWORD)str) return i;
        for (i = 0, item = descr->items; i <= start; i++, item++)
            if (item->data == (DWORD)str) return i;
    }
    return LB_ERR;
}


/***********************************************************************
 *           LISTBOX_GetSelCount
 */
static LRESULT LISTBOX_GetSelCount( WND *wnd, LB_DESCR *descr )
{
    INT i, count;
    LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; i < descr->nb_items; i++, item++)
        if (item->selected) count++;
    return count;
}


/***********************************************************************
 *           LISTBOX_GetSelItems16
 */
static LRESULT LISTBOX_GetSelItems16( WND *wnd, LB_DESCR *descr, INT16 max,
                                      LPINT16 array )
{
    INT i, count;
    LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; (i < descr->nb_items) && (count < max); i++, item++)
        if (item->selected) array[count++] = (INT16)i;
    return count;
}


/***********************************************************************
 *           LISTBOX_GetSelItems32
 */
static LRESULT LISTBOX_GetSelItems( WND *wnd, LB_DESCR *descr, INT max,
                                      LPINT array )
{
    INT i, count;
    LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; (i < descr->nb_items) && (count < max); i++, item++)
        if (item->selected) array[count++] = i;
    return count;
}


/***********************************************************************
 *           LISTBOX_Paint
 */
static LRESULT LISTBOX_Paint( WND *wnd, LB_DESCR *descr, HDC hdc )
{
    INT i, col_pos = descr->page_size - 1;
    RECT rect;
    HFONT oldFont = 0;
    HBRUSH hbrush, oldBrush = 0;

    SetRect( &rect, 0, 0, descr->width, descr->height );
    if (descr->style & LBS_NOREDRAW) return 0;
    if (descr->style & LBS_MULTICOLUMN)
        rect.right = rect.left + descr->column_width;
    else if (descr->horz_pos)
    {
        SetWindowOrgEx( hdc, descr->horz_pos, 0, NULL );
        rect.right += descr->horz_pos;
    }

    if (descr->font) oldFont = SelectObject( hdc, descr->font );
    hbrush = SendMessageA( descr->owner, WM_CTLCOLORLISTBOX,
                             hdc, (LPARAM)wnd->hwndSelf );
    if (hbrush) oldBrush = SelectObject( hdc, hbrush );
    if (wnd->dwStyle & WS_DISABLED)
        SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );

    if (!descr->nb_items && (descr->focus_item != -1) && descr->caret_on &&
        (GetFocus() == wnd->hwndSelf))
    {
        /* Special case for empty listbox: paint focus rect */
        rect.bottom = rect.top + descr->item_height;
        LISTBOX_PaintItem( wnd, descr, hdc, &rect, descr->focus_item,
                           ODA_FOCUS );
        rect.top = rect.bottom;
    }

    for (i = descr->top_item; i < descr->nb_items; i++)
    {
        if (!(descr->style & LBS_OWNERDRAWVARIABLE))
            rect.bottom = rect.top + descr->item_height;
        else
            rect.bottom = rect.top + descr->items[i].height;

        LISTBOX_PaintItem( wnd, descr, hdc, &rect, i, ODA_DRAWENTIRE );
        rect.top = rect.bottom;

        if ((descr->style & LBS_MULTICOLUMN) && !col_pos)
        {
            if (!IS_OWNERDRAW(descr))
            {
                /* Clear the bottom of the column */
                SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
                if (rect.top < descr->height)
                {
                    rect.bottom = descr->height;
                    ExtTextOutA( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                                   &rect, NULL, 0, NULL );
                }
            }

            /* Go to the next column */
            rect.left += descr->column_width;
            rect.right += descr->column_width;
            rect.top = 0;
            col_pos = descr->page_size - 1;
        }
        else
        {
            col_pos--;
            if (rect.top >= descr->height) break;
        }
    }

    if (!IS_OWNERDRAW(descr))
    {
        /* Clear the remainder of the client area */
        SetBkColor( hdc, GetSysColor( COLOR_WINDOW ) );
        if (rect.top < descr->height)
        {
            rect.bottom = descr->height;
            ExtTextOutA( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                           &rect, NULL, 0, NULL );
        }
        if (rect.right < descr->width)
        {
            rect.left   = rect.right;
            rect.right  = descr->width;
            rect.top    = 0;
            rect.bottom = descr->height;
            ExtTextOutA( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                           &rect, NULL, 0, NULL );
        }
    }
    if (oldFont) SelectObject( hdc, oldFont );
    if (oldBrush) SelectObject( hdc, oldBrush );
    return 0;
}


/***********************************************************************
 *           LISTBOX_InvalidateItems
 *
 * Invalidate all items from a given item. If the specified item is not
 * visible, nothing happens.
 */
static void LISTBOX_InvalidateItems( WND *wnd, LB_DESCR *descr, INT index )
{
    RECT rect;

    if (LISTBOX_GetItemRect( wnd, descr, index, &rect ) == 1)
    {
        rect.bottom = descr->height;
        InvalidateRect( wnd->hwndSelf, &rect, TRUE );
        if (descr->style & LBS_MULTICOLUMN)
        {
            /* Repaint the other columns */
            rect.left  = rect.right;
            rect.right = descr->width;
            rect.top   = 0;
            InvalidateRect( wnd->hwndSelf, &rect, TRUE );
        }
    }
}


/***********************************************************************
 *           LISTBOX_GetItemHeight
 */
static LRESULT LISTBOX_GetItemHeight( WND *wnd, LB_DESCR *descr, INT index )
{
    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
        return descr->items[index].height;
    }
    else return descr->item_height;
}


/***********************************************************************
 *           LISTBOX_SetItemHeight
 */
static LRESULT LISTBOX_SetItemHeight( WND *wnd, LB_DESCR *descr, INT index,
                                      UINT height )
{
    if (!height) height = 1;

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
        TRACE(listbox, "[%04x]: item %d height = %d\n",
		     wnd->hwndSelf, index, height );
        descr->items[index].height = height;
        LISTBOX_UpdateScroll( wnd, descr );
        LISTBOX_InvalidateItems( wnd, descr, index );
    }
    else if (height != descr->item_height)
    {
        TRACE(listbox, "[%04x]: new height = %d\n",
		     wnd->hwndSelf, height );
        descr->item_height = height;
        LISTBOX_UpdatePage( wnd, descr );
        LISTBOX_UpdateScroll( wnd, descr );
        InvalidateRect( wnd->hwndSelf, 0, TRUE );
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetHorizontalPos
 */
static void LISTBOX_SetHorizontalPos( WND *wnd, LB_DESCR *descr, INT pos )
{
    INT diff;

    if (pos > descr->horz_extent - descr->width)
        pos = descr->horz_extent - descr->width;
    if (pos < 0) pos = 0;
    if (!(diff = descr->horz_pos - pos)) return;
    TRACE(listbox, "[%04x]: new horz pos = %d\n",
		 wnd->hwndSelf, pos );
    descr->horz_pos = pos;
    LISTBOX_UpdateScroll( wnd, descr );
    if (abs(diff) < descr->width)
        ScrollWindowEx( wnd->hwndSelf, diff, 0, NULL, NULL, 0, NULL,
                          SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    else
        InvalidateRect( wnd->hwndSelf, NULL, TRUE );
}


/***********************************************************************
 *           LISTBOX_SetHorizontalExtent
 */
static LRESULT LISTBOX_SetHorizontalExtent( WND *wnd, LB_DESCR *descr,
                                            UINT extent )
{
    if (!descr->horz_extent || (descr->style & LBS_MULTICOLUMN))
        return LB_OKAY;
    if (extent <= 0) extent = 1;
    if (extent == descr->horz_extent) return LB_OKAY;
    TRACE(listbox, "[%04x]: new horz extent = %d\n",
		 wnd->hwndSelf, extent );
    descr->horz_extent = extent;
    if (descr->horz_pos > extent - descr->width)
        LISTBOX_SetHorizontalPos( wnd, descr, extent - descr->width );
    else
        LISTBOX_UpdateScroll( wnd, descr );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetColumnWidth
 */
static LRESULT LISTBOX_SetColumnWidth( WND *wnd, LB_DESCR *descr, UINT width)
{
    width += 2;  /* For left and right margin */
    if (width == descr->column_width) return LB_OKAY;
    TRACE(listbox, "[%04x]: new column width = %d\n",
		 wnd->hwndSelf, width );
    descr->column_width = width;
    LISTBOX_UpdatePage( wnd, descr );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetFont
 *
 * Returns the item height.
 */
static INT LISTBOX_SetFont( WND *wnd, LB_DESCR *descr, HFONT font )
{
    HDC hdc;
    HFONT oldFont = 0;
    TEXTMETRICA tm;

    descr->font = font;

    if (!(hdc = GetDCEx( wnd->hwndSelf, 0, DCX_CACHE )))
    {
        ERR(listbox, "unable to get DC.\n" );
        return 16;
    }
    if (font) oldFont = SelectObject( hdc, font );
    GetTextMetricsA( hdc, &tm );
    if (oldFont) SelectObject( hdc, oldFont );
    ReleaseDC( wnd->hwndSelf, hdc );
    if (!IS_OWNERDRAW(descr))
        LISTBOX_SetItemHeight( wnd, descr, 0, tm.tmHeight );
    return tm.tmHeight ;
}


/***********************************************************************
 *           LISTBOX_MakeItemVisible
 *
 * Make sure that a given item is partially or fully visible.
 */
static void LISTBOX_MakeItemVisible( WND *wnd, LB_DESCR *descr, INT index,
                                     BOOL fully )
{
    INT top;

    if (index <= descr->top_item) top = index;
    else if (descr->style & LBS_MULTICOLUMN)
    {
        INT cols = descr->width;
        if (!fully) cols += descr->column_width - 1;
        if (cols >= descr->column_width) cols /= descr->column_width;
        else cols = 1;
        if (index < descr->top_item + (descr->page_size * cols)) return;
        top = index - descr->page_size * (cols - 1);
    }
    else if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT height = fully ? descr->items[index].height : 1;
        for (top = index; top > descr->top_item; top--)
            if ((height += descr->items[top-1].height) > descr->height) break;
    }
    else
    {
        if (index < descr->top_item + descr->page_size) return;
        if (!fully && (index == descr->top_item + descr->page_size) &&
            (descr->height > (descr->page_size * descr->item_height))) return;
        top = index - descr->page_size + 1;
    }
    LISTBOX_SetTopItem( wnd, descr, top, TRUE );
}


/***********************************************************************
 *           LISTBOX_SelectItemRange
 *
 * Select a range of items. Should only be used on a MULTIPLESEL listbox.
 */
static LRESULT LISTBOX_SelectItemRange( WND *wnd, LB_DESCR *descr, INT first,
                                        INT last, BOOL on )
{
    INT i;

    /* A few sanity checks */

    if ((last == -1) && (descr->nb_items == 0)) return LB_OKAY;
    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    if (last == -1) last = descr->nb_items - 1;
    if ((first < 0) || (first >= descr->nb_items)) return LB_ERR;
    if ((last < 0) || (last >= descr->nb_items)) return LB_ERR;
    /* selected_item reflects last selected/unselected item on multiple sel */
    descr->selected_item = last;

    if (on)  /* Turn selection on */
    {
        for (i = first; i <= last; i++)
        {
            if (descr->items[i].selected) continue;
            descr->items[i].selected = TRUE;
            LISTBOX_RepaintItem( wnd, descr, i, ODA_SELECT );
        }
    }
    else  /* Turn selection off */
    {
        for (i = first; i <= last; i++)
        {
            if (!descr->items[i].selected) continue;
            descr->items[i].selected = FALSE;
            LISTBOX_RepaintItem( wnd, descr, i, ODA_SELECT );
        }
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetCaretIndex
 *
 * NOTES
 *   index must be between 0 and descr->nb_items-1, or LB_ERR is returned.
 *
 */
static LRESULT LISTBOX_SetCaretIndex( WND *wnd, LB_DESCR *descr, INT index,
                                      BOOL fully_visible )
{
    INT oldfocus = descr->focus_item;          

    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
    if (index == oldfocus) return LB_OKAY;
    descr->focus_item = index;
    if ((oldfocus != -1) && descr->caret_on && (GetFocus() == wnd->hwndSelf))
        LISTBOX_RepaintItem( wnd, descr, oldfocus, ODA_FOCUS );

    LISTBOX_MakeItemVisible( wnd, descr, index, fully_visible );
    if (descr->caret_on && (GetFocus() == wnd->hwndSelf))
        LISTBOX_RepaintItem( wnd, descr, index, ODA_FOCUS );

    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetSelection
 */
static LRESULT LISTBOX_SetSelection( WND *wnd, LB_DESCR *descr, INT index,
                                     BOOL on, BOOL send_notify )
{
    if ((index < -1) || (index >= descr->nb_items)) return LB_ERR;
    if (descr->style & LBS_MULTIPLESEL)
    {
        if (index == -1)  /* Select all items */
            return LISTBOX_SelectItemRange( wnd, descr, 0, -1, on );
        else  /* Only one item */
            return LISTBOX_SelectItemRange( wnd, descr, index, index, on );
    }
    else
    {
        INT oldsel = descr->selected_item;
        if (index == oldsel) return LB_OKAY;
        if (oldsel != -1) descr->items[oldsel].selected = FALSE;
        if (index != -1) descr->items[index].selected = TRUE;
        descr->selected_item = index;
        if (oldsel != -1) LISTBOX_RepaintItem( wnd, descr, oldsel, ODA_SELECT);
        if (index != -1) LISTBOX_RepaintItem( wnd, descr, index, ODA_SELECT );
        if (send_notify) SEND_NOTIFICATION( wnd, descr,
                               (index != -1) ? LBN_SELCHANGE : LBN_SELCANCEL );
	else
	    if( descr->lphc ) /* set selection change flag for parent combo */
		descr->lphc->wState |= CBF_SELCHANGE;
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_MoveCaret
 *
 * Change the caret position and extend the selection to the new caret.
 */
static void LISTBOX_MoveCaret( WND *wnd, LB_DESCR *descr, INT index,
                               BOOL fully_visible )
{
    LISTBOX_SetCaretIndex( wnd, descr, index, fully_visible );
    if (descr->style & LBS_EXTENDEDSEL)
    {
        if (descr->anchor_item != -1)
        {
            INT first = MIN( descr->focus_item, descr->anchor_item );
            INT last  = MAX( descr->focus_item, descr->anchor_item );
            if (first > 0)
                LISTBOX_SelectItemRange( wnd, descr, 0, first - 1, FALSE );
            LISTBOX_SelectItemRange( wnd, descr, last + 1, -1, FALSE );
            LISTBOX_SelectItemRange( wnd, descr, first, last, TRUE );
        }
    }
    else if (!(descr->style & LBS_MULTIPLESEL) && (descr->selected_item != -1))
    {
        /* Set selection to new caret item */
        LISTBOX_SetSelection( wnd, descr, index, TRUE, FALSE );
    }
}


/***********************************************************************
 *           LISTBOX_InsertItem
 */
static LRESULT LISTBOX_InsertItem( WND *wnd, LB_DESCR *descr, INT index,
                                   LPSTR str, DWORD data )
{
    LB_ITEMDATA *item;
    INT max_items;

    if (index == -1) index = descr->nb_items;
    else if ((index < 0) || (index > descr->nb_items)) return LB_ERR;
    if (!descr->items) max_items = 0;
    else max_items = HeapSize( descr->heap, 0, descr->items ) / sizeof(*item);
    if (descr->nb_items == max_items)
    {
        /* We need to grow the array */
        max_items += LB_ARRAY_GRANULARITY;
        if (!(item = HeapReAlloc( descr->heap, 0, descr->items,
                                  max_items * sizeof(LB_ITEMDATA) )))
        {
            SEND_NOTIFICATION( wnd, descr, LBN_ERRSPACE );
            return LB_ERRSPACE;
        }
        descr->items = item;
    }

    /* Insert the item structure */

    item = &descr->items[index];
    if (index < descr->nb_items)
        RtlMoveMemory( item + 1, item,
                       (descr->nb_items - index) * sizeof(LB_ITEMDATA) );
    item->str      = str;
    item->data     = data;
    item->height   = 0;
    item->selected = FALSE;
    descr->nb_items++;

    /* Get item height */

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        MEASUREITEMSTRUCT mis;
	UINT		    id = (descr->lphc) ? ID_CB_LISTBOX : wnd->wIDmenu;

        mis.CtlType    = ODT_LISTBOX;
        mis.CtlID      = id;
        mis.itemID     = index;
        mis.itemData   = descr->items[index].data;
        mis.itemHeight = descr->item_height;
        SendMessageA( descr->owner, WM_MEASUREITEM, id, (LPARAM)&mis );
        item->height = mis.itemHeight ? mis.itemHeight : 1;
        TRACE(listbox, "[%04x]: measure item %d (%s) = %d\n",
		     wnd->hwndSelf, index, str ? str : "", item->height );
    }

    /* Repaint the items */

    LISTBOX_UpdateScroll( wnd, descr );
    LISTBOX_InvalidateItems( wnd, descr, index );

    /* Move selection and focused item */

    if (index <= descr->selected_item) descr->selected_item++;
    if (index <= descr->focus_item)
    {
        descr->focus_item++;
        LISTBOX_MoveCaret( wnd, descr, descr->focus_item, FALSE );
    }

    /* If listbox was empty, set focus to the first item */

    if (descr->nb_items == 1) LISTBOX_SetCaretIndex( wnd, descr, 0, FALSE );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_InsertString
 */
static LRESULT LISTBOX_InsertString( WND *wnd, LB_DESCR *descr, INT index,
                                     LPCSTR str )
{
    LPSTR new_str = NULL;
    DWORD data = 0;
    LRESULT ret;

    if (HAS_STRINGS(descr))
    {
        if (!(new_str = HEAP_strdupA( descr->heap, 0, str )))
        {
            SEND_NOTIFICATION( wnd, descr, LBN_ERRSPACE );
            return LB_ERRSPACE;
        }
    }
    else data = (DWORD)str;

    if (index == -1) index = descr->nb_items;
    if ((ret = LISTBOX_InsertItem( wnd, descr, index, new_str, data )) != 0)
    {
        if (new_str) HeapFree( descr->heap, 0, new_str );
        return ret;
    }

    TRACE(listbox, "[%04x]: added item %d '%s'\n",
		 wnd->hwndSelf, index, HAS_STRINGS(descr) ? new_str : "" );
    return index;
}


/***********************************************************************
 *           LISTBOX_DeleteItem
 *
 * Delete the content of an item. 'index' must be a valid index.
 */
static void LISTBOX_DeleteItem( WND *wnd, LB_DESCR *descr, INT index )
{
    /* Note: Win 3.1 only sends DELETEITEM on owner-draw items,
     *       while Win95 sends it for all items with user data.
     *       It's probably better to send it too often than not
     *       often enough, so this is what we do here.
     */
    if (IS_OWNERDRAW(descr) || descr->items[index].data)
    {
        DELETEITEMSTRUCT dis;
	UINT		   id = (descr->lphc) ? ID_CB_LISTBOX : wnd->wIDmenu;

        dis.CtlType  = ODT_LISTBOX;
        dis.CtlID    = id;
        dis.itemID   = index;
        dis.hwndItem = wnd->hwndSelf;
        dis.itemData = descr->items[index].data;
        SendMessageA( descr->owner, WM_DELETEITEM, id, (LPARAM)&dis );
    }
    if (HAS_STRINGS(descr) && descr->items[index].str)
        HeapFree( descr->heap, 0, descr->items[index].str );
}


/***********************************************************************
 *           LISTBOX_RemoveItem
 *
 * Remove an item from the listbox and delete its content.
 */
static LRESULT LISTBOX_RemoveItem( WND *wnd, LB_DESCR *descr, INT index )
{
    LB_ITEMDATA *item;
    INT max_items;

    if (index == -1) index = descr->nb_items - 1;
    else if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
    LISTBOX_DeleteItem( wnd, descr, index );

    /* Remove the item */

    item = &descr->items[index];
    if (index < descr->nb_items-1)
        RtlMoveMemory( item, item + 1,
                       (descr->nb_items - index - 1) * sizeof(LB_ITEMDATA) );
    descr->nb_items--;
    if (descr->anchor_item == descr->nb_items) descr->anchor_item--;

    /* Shrink the item array if possible */

    max_items = HeapSize( descr->heap, 0, descr->items ) / sizeof(LB_ITEMDATA);
    if (descr->nb_items < max_items - 2*LB_ARRAY_GRANULARITY)
    {
        max_items -= LB_ARRAY_GRANULARITY;
        item = HeapReAlloc( descr->heap, 0, descr->items,
                            max_items * sizeof(LB_ITEMDATA) );
        if (item) descr->items = item;
    }

    /* Repaint the items */

    LISTBOX_UpdateScroll( wnd, descr );
    LISTBOX_InvalidateItems( wnd, descr, index );

    /* Move selection and focused item */

    if (index <= descr->selected_item) descr->selected_item--;
    if (index <= descr->focus_item)
    {
        descr->focus_item--;
        LISTBOX_MoveCaret( wnd, descr, descr->focus_item, FALSE );
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_ResetContent
 */
static void LISTBOX_ResetContent( WND *wnd, LB_DESCR *descr )
{
    INT i;

    for (i = 0; i < descr->nb_items; i++) LISTBOX_DeleteItem( wnd, descr, i );
    if (descr->items) HeapFree( descr->heap, 0, descr->items );
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->items         = NULL;
    LISTBOX_UpdateScroll( wnd, descr );
    InvalidateRect( wnd->hwndSelf, NULL, TRUE );
}


/***********************************************************************
 *           LISTBOX_SetCount
 */
static LRESULT LISTBOX_SetCount( WND *wnd, LB_DESCR *descr, INT count )
{
    LRESULT ret;

    if (HAS_STRINGS(descr)) return LB_ERR;
    /* FIXME: this is far from optimal... */
    if (count > descr->nb_items)
    {
        while (count > descr->nb_items)
            if ((ret = LISTBOX_InsertString( wnd, descr, -1, 0 )) < 0)
                return ret;
    }
    else if (count < descr->nb_items)
    {
        while (count < descr->nb_items)
            if ((ret = LISTBOX_RemoveItem( wnd, descr, -1 )) < 0)
                return ret;
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_Directory
 */
static LRESULT LISTBOX_Directory( WND *wnd, LB_DESCR *descr, UINT attrib,
                                  LPCSTR filespec, BOOL long_names )
{
    HANDLE handle;
    LRESULT ret = LB_OKAY;
    WIN32_FIND_DATAA entry;
    int pos;

    if ((handle = FindFirstFileA(filespec,&entry)) == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() != ERROR_NO_MORE_FILES) return LB_ERR;
    }
    else
    {
        do
        {
            char buffer[270];
            if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!(attrib & DDL_DIRECTORY) ||
                    !strcmp( entry.cAlternateFileName, "." )) continue;
                if (long_names) sprintf( buffer, "[%s]", entry.cFileName );
                else sprintf( buffer, "[%s]", entry.cAlternateFileName );
            }
            else  /* not a directory */
            {
#define ATTRIBS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
                 FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)

                if ((attrib & DDL_EXCLUSIVE) &&
                    ((attrib & ATTRIBS) != (entry.dwFileAttributes & ATTRIBS)))
                    continue;
#undef ATTRIBS
                if (long_names) strcpy( buffer, entry.cFileName );
                else strcpy( buffer, entry.cAlternateFileName );
            }
            if (!long_names) CharLowerA( buffer );
            pos = LISTBOX_FindFileStrPos( wnd, descr, buffer );
            if ((ret = LISTBOX_InsertString( wnd, descr, pos, buffer )) < 0)
                break;
        } while (FindNextFileA( handle, &entry ));
        FindClose( handle );
    }

    if ((ret >= 0) && (attrib & DDL_DRIVES))
    {
        char buffer[] = "[-a-]";
        int drive;
        for (drive = 0; drive < MAX_DOS_DRIVES; drive++, buffer[2]++)
        {
            if (!DRIVE_IsValid(drive)) continue;
            if ((ret = LISTBOX_InsertString( wnd, descr, -1, buffer )) < 0)
                break;
        }
    }
    return ret;
}


/***********************************************************************
 *           LISTBOX_HandleVScroll
 */
static LRESULT LISTBOX_HandleVScroll( WND *wnd, LB_DESCR *descr,
                                      WPARAM wParam, LPARAM lParam )
{
    SCROLLINFO info;

    if (descr->style & LBS_MULTICOLUMN) return 0;
    switch(LOWORD(wParam))
    {
    case SB_LINEUP:
        LISTBOX_SetTopItem( wnd, descr, descr->top_item - 1, TRUE );
        break;
    case SB_LINEDOWN:
        LISTBOX_SetTopItem( wnd, descr, descr->top_item + 1, TRUE );
        break;
    case SB_PAGEUP:
        LISTBOX_SetTopItem( wnd, descr, descr->top_item -
                            LISTBOX_GetCurrentPageSize( wnd, descr ), TRUE );
        break;
    case SB_PAGEDOWN:
        LISTBOX_SetTopItem( wnd, descr, descr->top_item +
                            LISTBOX_GetCurrentPageSize( wnd, descr ), TRUE );
        break;
    case SB_THUMBPOSITION:
        LISTBOX_SetTopItem( wnd, descr, HIWORD(wParam), TRUE );
        break;
    case SB_THUMBTRACK:
        info.cbSize = sizeof(info);
        info.fMask = SIF_TRACKPOS;
        GetScrollInfo( wnd->hwndSelf, SB_VERT, &info );
        LISTBOX_SetTopItem( wnd, descr, info.nTrackPos, TRUE );
        break;
    case SB_TOP:
        LISTBOX_SetTopItem( wnd, descr, 0, TRUE );
        break;
    case SB_BOTTOM:
        LISTBOX_SetTopItem( wnd, descr, descr->nb_items, TRUE );
        break;
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleHScroll
 */
static LRESULT LISTBOX_HandleHScroll( WND *wnd, LB_DESCR *descr,
                                      WPARAM wParam, LPARAM lParam )
{
    SCROLLINFO info;
    INT page;

    if (descr->style & LBS_MULTICOLUMN)
    {
        switch(LOWORD(wParam))
        {
        case SB_LINELEFT:
            LISTBOX_SetTopItem( wnd, descr, descr->top_item-descr->page_size,
                                TRUE );
            break;
        case SB_LINERIGHT:
            LISTBOX_SetTopItem( wnd, descr, descr->top_item+descr->page_size,
                                TRUE );
            break;
        case SB_PAGELEFT:
            page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            LISTBOX_SetTopItem( wnd, descr,
                             descr->top_item - page * descr->page_size, TRUE );
            break;
        case SB_PAGERIGHT:
            page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            LISTBOX_SetTopItem( wnd, descr,
                             descr->top_item + page * descr->page_size, TRUE );
            break;
        case SB_THUMBPOSITION:
            LISTBOX_SetTopItem( wnd, descr, HIWORD(wParam)*descr->page_size,
                                TRUE );
            break;
        case SB_THUMBTRACK:
            info.cbSize = sizeof(info);
            info.fMask  = SIF_TRACKPOS;
            GetScrollInfo( wnd->hwndSelf, SB_VERT, &info );
            LISTBOX_SetTopItem( wnd, descr, info.nTrackPos*descr->page_size,
                                TRUE );
            break;
        case SB_LEFT:
            LISTBOX_SetTopItem( wnd, descr, 0, TRUE );
            break;
        case SB_RIGHT:
            LISTBOX_SetTopItem( wnd, descr, descr->nb_items, TRUE );
            break;
        }
    }
    else if (descr->horz_extent)
    {
        switch(LOWORD(wParam))
        {
        case SB_LINELEFT:
            LISTBOX_SetHorizontalPos( wnd, descr, descr->horz_pos - 1 );
            break;
        case SB_LINERIGHT:
            LISTBOX_SetHorizontalPos( wnd, descr, descr->horz_pos + 1 );
            break;
        case SB_PAGELEFT:
            LISTBOX_SetHorizontalPos( wnd, descr,
                                      descr->horz_pos - descr->width );
            break;
        case SB_PAGERIGHT:
            LISTBOX_SetHorizontalPos( wnd, descr,
                                      descr->horz_pos + descr->width );
            break;
        case SB_THUMBPOSITION:
            LISTBOX_SetHorizontalPos( wnd, descr, HIWORD(wParam) );
            break;
        case SB_THUMBTRACK:
            info.cbSize = sizeof(info);
            info.fMask = SIF_TRACKPOS;
            GetScrollInfo( wnd->hwndSelf, SB_HORZ, &info );
            LISTBOX_SetHorizontalPos( wnd, descr, info.nTrackPos );
            break;
        case SB_LEFT:
            LISTBOX_SetHorizontalPos( wnd, descr, 0 );
            break;
        case SB_RIGHT:
            LISTBOX_SetHorizontalPos( wnd, descr,
                                      descr->horz_extent - descr->width );
            break;
        }
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleLButtonDown
 */
static LRESULT LISTBOX_HandleLButtonDown( WND *wnd, LB_DESCR *descr,
                                          WPARAM wParam, INT x, INT y )
{
    INT index = LISTBOX_GetItemFromPoint( wnd, descr, x, y );
    TRACE(listbox, "[%04x]: lbuttondown %d,%d item %d\n",
		 wnd->hwndSelf, x, y, index );
    if (!descr->caret_on && (GetFocus() == wnd->hwndSelf)) return 0;
    if (index != -1)
    {
        if (descr->style & LBS_EXTENDEDSEL)
        {
            if (!(wParam & MK_SHIFT)) descr->anchor_item = index;
            if (wParam & MK_CONTROL)
            {
                LISTBOX_SetCaretIndex( wnd, descr, index, FALSE );
                LISTBOX_SetSelection( wnd, descr, index,
                                      !descr->items[index].selected, FALSE );
            }
            else LISTBOX_MoveCaret( wnd, descr, index, FALSE );
        }
        else
        {
            LISTBOX_MoveCaret( wnd, descr, index, FALSE );
            LISTBOX_SetSelection( wnd, descr, index,
                                  (!(descr->style & LBS_MULTIPLESEL) || 
                                   !descr->items[index].selected), FALSE );
        }
    }

    if( !descr->lphc ) SetFocus( wnd->hwndSelf );
    else SetFocus( (descr->lphc->hWndEdit) ? descr->lphc->hWndEdit
                                             : descr->lphc->self->hwndSelf ) ;

    descr->captured = TRUE;
    SetCapture( wnd->hwndSelf );
    if (index != -1 && !descr->lphc)
    {
        if (descr->style & LBS_NOTIFY )
            SendMessageA( descr->owner, WM_LBTRACKPOINT, index,
                            MAKELPARAM( x, y ) );
        if (wnd->dwExStyle & WS_EX_DRAGDETECT)
        {
            POINT pt = { x, y };
            if (DragDetect( wnd->hwndSelf, pt ))
                SendMessageA( descr->owner, WM_BEGINDRAG, 0, 0 );
        }
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleLButtonUp
 */
static LRESULT LISTBOX_HandleLButtonUp( WND *wnd, LB_DESCR *descr )
{
    if (LISTBOX_Timer != LB_TIMER_NONE)
        KillSystemTimer( wnd->hwndSelf, LB_TIMER_ID );
    LISTBOX_Timer = LB_TIMER_NONE;
    if (descr->captured)
    {
        descr->captured = FALSE;
        if (GetCapture() == wnd->hwndSelf) ReleaseCapture();
        if (descr->style & LBS_NOTIFY)
            SEND_NOTIFICATION( wnd, descr, LBN_SELCHANGE );
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleTimer
 *
 * Handle scrolling upon a timer event.
 * Return TRUE if scrolling should continue.
 */
static LRESULT LISTBOX_HandleTimer( WND *wnd, LB_DESCR *descr,
                                    INT index, TIMER_DIRECTION dir )
{
    switch(dir)
    {
    case LB_TIMER_UP:
        if (descr->top_item) index = descr->top_item - 1;
        else index = 0;
        break;
    case LB_TIMER_LEFT:
        if (descr->top_item) index -= descr->page_size;
        break;
    case LB_TIMER_DOWN:
        index = descr->top_item + LISTBOX_GetCurrentPageSize( wnd, descr );
        if (index == descr->focus_item) index++;
        if (index >= descr->nb_items) index = descr->nb_items - 1;
        break;
    case LB_TIMER_RIGHT:
        if (index + descr->page_size < descr->nb_items)
            index += descr->page_size;
        break;
    case LB_TIMER_NONE:
        break;
    }
    if (index == descr->focus_item) return FALSE;
    LISTBOX_MoveCaret( wnd, descr, index, FALSE );
    return TRUE;
}


/***********************************************************************
 *           LISTBOX_HandleSystemTimer
 *
 * WM_SYSTIMER handler.
 */
static LRESULT LISTBOX_HandleSystemTimer( WND *wnd, LB_DESCR *descr )
{
    if (!LISTBOX_HandleTimer( wnd, descr, descr->focus_item, LISTBOX_Timer ))
    {
        KillSystemTimer( wnd->hwndSelf, LB_TIMER_ID );
        LISTBOX_Timer = LB_TIMER_NONE;
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleMouseMove
 *
 * WM_MOUSEMOVE handler.
 */
static void LISTBOX_HandleMouseMove( WND *wnd, LB_DESCR *descr,
                                     INT x, INT y )
{
    INT index;
    TIMER_DIRECTION dir;

    if (!descr->captured) return;

    if (descr->style & LBS_MULTICOLUMN)
    {
        if (y < 0) y = 0;
        else if (y >= descr->item_height * descr->page_size)
            y = descr->item_height * descr->page_size - 1;

        if (x < 0)
        {
            dir = LB_TIMER_LEFT;
            x = 0;
        }
        else if (x >= descr->width)
        {
            dir = LB_TIMER_RIGHT;
            x = descr->width - 1;
        }
        else dir = LB_TIMER_NONE;  /* inside */
    }
    else
    {
        if (y < 0) dir = LB_TIMER_UP;  /* above */
        else if (y >= descr->height) dir = LB_TIMER_DOWN;  /* below */
        else dir = LB_TIMER_NONE;  /* inside */
    }

    index = LISTBOX_GetItemFromPoint( wnd, descr, x, y );
    if (index == -1) index = descr->focus_item;
    if (!LISTBOX_HandleTimer( wnd, descr, index, dir )) dir = LB_TIMER_NONE;

    /* Start/stop the system timer */

    if (dir != LB_TIMER_NONE)
        SetSystemTimer( wnd->hwndSelf, LB_TIMER_ID, LB_SCROLL_TIMEOUT, NULL);
    else if (LISTBOX_Timer != LB_TIMER_NONE)
        KillSystemTimer( wnd->hwndSelf, LB_TIMER_ID );
    LISTBOX_Timer = dir;
}


/***********************************************************************
 *           LISTBOX_HandleKeyDown
 */
static LRESULT LISTBOX_HandleKeyDown( WND *wnd, LB_DESCR *descr, WPARAM wParam )
{
    INT caret = -1;
    if (descr->style & LBS_WANTKEYBOARDINPUT)
    {
        caret = SendMessageA( descr->owner, WM_VKEYTOITEM,
                                MAKEWPARAM(LOWORD(wParam), descr->focus_item),
                                wnd->hwndSelf );
        if (caret == -2) return 0;
    }
    if (caret == -1) switch(wParam)
    {
    case VK_LEFT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            if (descr->focus_item >= descr->page_size)
                caret = descr->focus_item - descr->page_size;
            break;
        }
        /* fall through */
    case VK_UP:
        caret = descr->focus_item - 1;
        if (caret < 0) caret = 0;
        break;
    case VK_RIGHT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            if (descr->focus_item + descr->page_size < descr->nb_items)
                caret = descr->focus_item + descr->page_size;
            break;
        }
        /* fall through */
    case VK_DOWN:
        caret = descr->focus_item + 1;
        if (caret >= descr->nb_items) caret = descr->nb_items - 1;
        break;
    case VK_PRIOR:
        if (descr->style & LBS_MULTICOLUMN)
        {
            INT page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            caret = descr->focus_item - (page * descr->page_size) + 1;
        }
        else caret = descr->focus_item-LISTBOX_GetCurrentPageSize(wnd,descr)+1;
        if (caret < 0) caret = 0;
        break;
    case VK_NEXT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            INT page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            caret = descr->focus_item + (page * descr->page_size) - 1;
        }
        else caret = descr->focus_item+LISTBOX_GetCurrentPageSize(wnd,descr)-1;
        if (caret >= descr->nb_items) caret = descr->nb_items - 1;
        break;
    case VK_HOME:
        caret = 0;
        break;
    case VK_END:
        caret = descr->nb_items - 1;
        break;
    case VK_SPACE:
        if (descr->style & LBS_EXTENDEDSEL) caret = descr->focus_item;
        else if (descr->style & LBS_MULTIPLESEL)
        {
            LISTBOX_SetSelection( wnd, descr, descr->focus_item,
                                  !descr->items[descr->focus_item].selected,
                                  (descr->style & LBS_NOTIFY) != 0 );
        }
        else if (descr->selected_item == -1)
        {
            LISTBOX_SetSelection( wnd, descr, descr->focus_item, TRUE,
                                  (descr->style & LBS_NOTIFY) != 0 );
        }
        break;
    }
    if (caret >= 0)
    {
        if ((descr->style & LBS_EXTENDEDSEL) &&
            !(GetKeyState( VK_SHIFT ) & 0x8000))
            descr->anchor_item = caret;
        LISTBOX_MoveCaret( wnd, descr, caret, TRUE );
        if (descr->style & LBS_NOTIFY)
        {
	    if( descr->lphc && CB_GETTYPE(descr->lphc) != CBS_SIMPLE )
            {
		/* make sure that combo parent doesn't hide us */
		descr->lphc->wState |= CBF_NOROLLUP;
	    }
            SEND_NOTIFICATION( wnd, descr, LBN_SELCHANGE );
        }
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleChar
 */
static LRESULT LISTBOX_HandleChar( WND *wnd, LB_DESCR *descr,
                                   WPARAM wParam )
{
    INT caret = -1;
    char str[2] = { wParam & 0xff, '\0' };

    if (descr->style & LBS_WANTKEYBOARDINPUT)
    {
        caret = SendMessageA( descr->owner, WM_CHARTOITEM,
                                MAKEWPARAM(LOWORD(wParam), descr->focus_item),
                                wnd->hwndSelf );
        if (caret == -2) return 0;
    }
    if (caret == -1)
        caret = LISTBOX_FindString( wnd, descr, descr->focus_item, str, FALSE);
    if (caret != -1)
    {
        LISTBOX_MoveCaret( wnd, descr, caret, TRUE );
        if (descr->style & LBS_NOTIFY)
            SEND_NOTIFICATION( wnd, descr, LBN_SELCHANGE );
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_Create
 */
static BOOL LISTBOX_Create( WND *wnd, LPHEADCOMBO lphc )
{
    LB_DESCR *descr;
    MEASUREITEMSTRUCT mis;
    RECT rect;

    if (!(descr = HeapAlloc( GetProcessHeap(), 0, sizeof(*descr) )))
        return FALSE;
    if (!(descr->heap = HeapCreate( 0, 0x10000, 0 )))
    {
        HeapFree( GetProcessHeap(), 0, descr );
        return FALSE;
    }
    GetClientRect( wnd->hwndSelf, &rect );
    descr->owner         = GetParent( wnd->hwndSelf );
    descr->style         = wnd->dwStyle;
    descr->width         = rect.right - rect.left;
    descr->height        = rect.bottom - rect.top;
    descr->items         = NULL;
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->item_height   = 1;
    descr->page_size     = 1;
    descr->column_width  = 150;
    descr->horz_extent   = (wnd->dwStyle & WS_HSCROLL) ? 1 : 0;
    descr->horz_pos      = 0;
    descr->nb_tabs       = 0;
    descr->tabs          = NULL;
    descr->caret_on      = TRUE;
    descr->captured      = FALSE;
    descr->font          = 0;
    descr->locale        = 0;  /* FIXME */
    descr->lphc		 = lphc;

    if( lphc )
    {
	TRACE(combo,"[%04x]: resetting owner %04x -> %04x\n",
		     wnd->hwndSelf, descr->owner, lphc->self->hwndSelf );
	descr->owner = lphc->self->hwndSelf;
    }

    *(LB_DESCR **)wnd->wExtra = descr;

/*    if (wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) descr->style &= ~LBS_NOTIFY;
 */
    if (descr->style & LBS_EXTENDEDSEL) descr->style |= LBS_MULTIPLESEL;
    if (descr->style & LBS_MULTICOLUMN) descr->style &= ~LBS_OWNERDRAWVARIABLE;
    if (descr->style & LBS_OWNERDRAWVARIABLE) descr->style |= LBS_NOINTEGRALHEIGHT;
    descr->item_height = LISTBOX_SetFont( wnd, descr, 0 );

    if (descr->style & LBS_OWNERDRAWFIXED)
    {
	if( descr->lphc && (descr->lphc->dwStyle & CBS_DROPDOWN))
	{
	    /* WinWord gets VERY unhappy if we send WM_MEASUREITEM from here */
	  descr->item_height = lphc->fixedOwnerDrawHeight;
	}
	else
	{
	    UINT	id = (descr->lphc ) ? ID_CB_LISTBOX : wnd->wIDmenu;

            mis.CtlType    = ODT_LISTBOX;
            mis.CtlID      = id;
            mis.itemID     = -1;
            mis.itemWidth  =  0;
            mis.itemData   =  0;
            mis.itemHeight = descr->item_height;
            SendMessageA( descr->owner, WM_MEASUREITEM, id, (LPARAM)&mis );
            descr->item_height = mis.itemHeight ? mis.itemHeight : 1;
	}
    }

    return TRUE;
}


/***********************************************************************
 *           LISTBOX_Destroy
 */
static BOOL LISTBOX_Destroy( WND *wnd, LB_DESCR *descr )
{
    LISTBOX_ResetContent( wnd, descr );
    HeapDestroy( descr->heap );
    HeapFree( GetProcessHeap(), 0, descr );
    wnd->wExtra[0] = 0;
    return TRUE;
}


/***********************************************************************
 *           ListBoxWndProc
 */
LRESULT WINAPI ListBoxWndProc( HWND hwnd, UINT msg,
                               WPARAM wParam, LPARAM lParam )
{
    LRESULT ret;
    LB_DESCR *descr;
    WND *wnd = WIN_FindWndPtr( hwnd );
    LRESULT retvalue;

    if (!wnd) return 0;
    if (!(descr = *(LB_DESCR **)wnd->wExtra))
    {
        if (msg == WM_CREATE)
        {
            if (!LISTBOX_Create( wnd, NULL ))
            {
                retvalue = -1;
                goto END;
            }
            TRACE(listbox, "creating wnd=%04x descr=%p\n",
			 hwnd, *(LB_DESCR **)wnd->wExtra );
            retvalue = 0;
            goto END;
        }
        /* Ignore all other messages before we get a WM_CREATE */
        retvalue = DefWindowProcA( hwnd, msg, wParam, lParam );
        goto END;
    }

    TRACE(listbox, "[%04x]: msg %s wp %08x lp %08lx\n",
		 wnd->hwndSelf, SPY_GetMsgName(msg), wParam, lParam );
    switch(msg)
    {
    case LB_RESETCONTENT16:
    case LB_RESETCONTENT:
        LISTBOX_ResetContent( wnd, descr );
        retvalue = 0;
        goto END;

    case LB_ADDSTRING16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_ADDSTRING:
        wParam = LISTBOX_FindStringPos( wnd, descr, (LPCSTR)lParam, FALSE );
        retvalue = LISTBOX_InsertString( wnd, descr, wParam, (LPCSTR)lParam );
        goto END;

    case LB_INSERTSTRING16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case LB_INSERTSTRING:
        retvalue = LISTBOX_InsertString( wnd, descr, wParam, (LPCSTR)lParam );
        goto END;

    case LB_ADDFILE16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_ADDFILE:
        wParam = LISTBOX_FindFileStrPos( wnd, descr, (LPCSTR)lParam );
        retvalue = LISTBOX_InsertString( wnd, descr, wParam, (LPCSTR)lParam );
        goto END;

    case LB_DELETESTRING16:
    case LB_DELETESTRING:
        retvalue = LISTBOX_RemoveItem( wnd, descr, wParam );
        goto END;

    case LB_GETITEMDATA16:
    case LB_GETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            retvalue = LB_ERR;
            goto END;
        }
        retvalue = descr->items[wParam].data;
        goto END;

    case LB_SETITEMDATA16:
    case LB_SETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            retvalue = LB_ERR;
            goto END;
        }
        descr->items[wParam].data = (DWORD)lParam;
        retvalue = LB_OKAY;
        goto END;

    case LB_GETCOUNT16:
    case LB_GETCOUNT:
        retvalue = descr->nb_items;
        goto END;

    case LB_GETTEXT16:
        lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_GETTEXT:
        retvalue = LISTBOX_GetText( wnd, descr, wParam, (LPSTR)lParam );
        goto END;

    case LB_GETTEXTLEN16:
        /* fall through */
    case LB_GETTEXTLEN:
        if (wParam >= descr->nb_items)
        {
            retvalue = LB_ERR;
            goto END;
        }
        retvalue = (HAS_STRINGS(descr) ? strlen(descr->items[wParam].str)
                                   : sizeof(DWORD));
        goto END;

    case LB_GETCURSEL16:
    case LB_GETCURSEL:
        if (descr->nb_items==0)
	  retvalue = LB_ERR;
	else
	{
	  retvalue = descr->selected_item;
	  if (retvalue == -1) retvalue = descr->focus_item;
	}
        /* otherwise, if the user tries to move the selection with the    */
        /* arrow keys, we will give the application something to choke on */
        goto END;

    case LB_GETTOPINDEX16:
    case LB_GETTOPINDEX:
        retvalue = descr->top_item;
        goto END;

    case LB_GETITEMHEIGHT16:
    case LB_GETITEMHEIGHT:
        retvalue = LISTBOX_GetItemHeight( wnd, descr, wParam );
        goto END;

    case LB_SETITEMHEIGHT16:
        lParam = LOWORD(lParam);
        /* fall through */
    case LB_SETITEMHEIGHT:
        retvalue = LISTBOX_SetItemHeight( wnd, descr, wParam, lParam );
        goto END;

    case LB_ITEMFROMPOINT:
        {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            RECT rect = { 0, 0, descr->width, descr->height };
            retvalue = MAKELONG( LISTBOX_GetItemFromPoint(wnd, descr, pt.x, pt.y),
                             PtInRect( &rect, pt ) );
            goto END;
        }

    case LB_SETCARETINDEX16:
    case LB_SETCARETINDEX:
        retvalue = LISTBOX_SetCaretIndex( wnd, descr, wParam, !lParam );
        goto END;

    case LB_GETCARETINDEX16:
    case LB_GETCARETINDEX:
        retvalue = descr->focus_item;
        goto END;

    case LB_SETTOPINDEX16:
    case LB_SETTOPINDEX:
        retvalue = LISTBOX_SetTopItem( wnd, descr, wParam, TRUE );
        goto END;

    case LB_SETCOLUMNWIDTH16:
    case LB_SETCOLUMNWIDTH:
        retvalue = LISTBOX_SetColumnWidth( wnd, descr, wParam );
        goto END;

    case LB_GETITEMRECT16:
        {
            RECT rect;
            ret = LISTBOX_GetItemRect( wnd, descr, (INT16)wParam, &rect );
            CONV_RECT32TO16( &rect, (RECT16 *)PTR_SEG_TO_LIN(lParam) );
        }
        retvalue = ret;
        goto END;

    case LB_GETITEMRECT:
        retvalue = LISTBOX_GetItemRect( wnd, descr, wParam, (RECT *)lParam );
        goto END;

    case LB_FINDSTRING16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_FINDSTRING:
        retvalue = LISTBOX_FindString( wnd, descr, wParam, (LPCSTR)lParam, FALSE );
        goto END;

    case LB_FINDSTRINGEXACT16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_FINDSTRINGEXACT:
        retvalue = LISTBOX_FindString( wnd, descr, wParam, (LPCSTR)lParam, TRUE );
        goto END;

    case LB_SELECTSTRING16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)PTR_SEG_TO_LIN(lParam);
        /* fall through */
    case LB_SELECTSTRING:
        {
            INT index = LISTBOX_FindString( wnd, descr, wParam,
                                              (LPCSTR)lParam, FALSE );
            if (index == LB_ERR)
            {
                retvalue = LB_ERR;
                goto END;
            }
            LISTBOX_SetSelection( wnd, descr, index, TRUE, FALSE );
            retvalue = index;
            goto END;
        }

    case LB_GETSEL16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case LB_GETSEL:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            retvalue = LB_ERR;
            goto END;
        }
        retvalue = descr->items[wParam].selected;
        goto END;

    case LB_SETSEL16:
        lParam = (INT)(INT16)lParam;
        /* fall through */
    case LB_SETSEL:
        retvalue = LISTBOX_SetSelection( wnd, descr, lParam, wParam, FALSE );
        goto END;

    case LB_SETCURSEL16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case LB_SETCURSEL:
        LISTBOX_SetCaretIndex( wnd, descr, wParam, TRUE );  
        retvalue = LISTBOX_SetSelection( wnd, descr, wParam, TRUE, FALSE );
        goto END;

    case LB_GETSELCOUNT16:
    case LB_GETSELCOUNT:
        retvalue = LISTBOX_GetSelCount( wnd, descr );
        goto END;

    case LB_GETSELITEMS16:
        retvalue = LISTBOX_GetSelItems16( wnd, descr, wParam,
                                      (LPINT16)PTR_SEG_TO_LIN(lParam) );
        goto END;

    case LB_GETSELITEMS:
        retvalue = LISTBOX_GetSelItems( wnd, descr, wParam, (LPINT)lParam );
        goto END;

    case LB_SELITEMRANGE16:
    case LB_SELITEMRANGE:
        if (LOWORD(lParam) <= HIWORD(lParam))
        {
            retvalue = LISTBOX_SelectItemRange( wnd, descr, LOWORD(lParam),
                                            HIWORD(lParam), wParam );
        }
        else
        {
            retvalue = LISTBOX_SelectItemRange( wnd, descr, HIWORD(lParam),
                                            LOWORD(lParam), wParam );
        }
        goto END;

    case LB_SELITEMRANGEEX16:
    case LB_SELITEMRANGEEX:
        if ((INT)lParam >= (INT)wParam)
            retvalue = LISTBOX_SelectItemRange( wnd, descr, wParam, lParam, TRUE );
        else
            retvalue = LISTBOX_SelectItemRange( wnd, descr, lParam, wParam, FALSE);
        goto END;

    case LB_GETHORIZONTALEXTENT16:
    case LB_GETHORIZONTALEXTENT:
        retvalue = descr->horz_extent;
        goto END;

    case LB_SETHORIZONTALEXTENT16:
    case LB_SETHORIZONTALEXTENT:
        retvalue = LISTBOX_SetHorizontalExtent( wnd, descr, wParam );
        goto END;

    case LB_GETANCHORINDEX16:
    case LB_GETANCHORINDEX:
        retvalue = descr->anchor_item;
        goto END;

    case LB_SETANCHORINDEX16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
    case LB_SETANCHORINDEX:
        if (((INT)wParam < -1) || ((INT)wParam >= descr->nb_items))
        {
            retvalue = LB_ERR;
            goto END;
        }
        descr->anchor_item = (INT)wParam;
        retvalue = LB_OKAY;
        goto END;

    case LB_DIR16:
        retvalue = LISTBOX_Directory( wnd, descr, wParam,
                                  (LPCSTR)PTR_SEG_TO_LIN(lParam), FALSE );
        goto END;

    case LB_DIR:
        retvalue = LISTBOX_Directory( wnd, descr, wParam, (LPCSTR)lParam, TRUE );
        goto END;

    case LB_GETLOCALE:
        retvalue = descr->locale;
        goto END;

    case LB_SETLOCALE:
        descr->locale = (LCID)wParam;  /* FIXME: should check for valid lcid */
        retvalue = LB_OKAY;
        goto END;

    case LB_INITSTORAGE:
        retvalue = LISTBOX_InitStorage( wnd, descr, wParam, (DWORD)lParam );
        goto END;

    case LB_SETCOUNT:
        retvalue = LISTBOX_SetCount( wnd, descr, (INT)wParam );
        goto END;

    case LB_SETTABSTOPS16:
        retvalue = LISTBOX_SetTabStops( wnd, descr, (INT)(INT16)wParam,
                                    (LPINT)PTR_SEG_TO_LIN(lParam), TRUE );
        goto END;

    case LB_SETTABSTOPS:
        retvalue = LISTBOX_SetTabStops( wnd, descr, wParam,
                                    (LPINT)lParam, FALSE );
        goto END;

    case LB_CARETON16:
    case LB_CARETON:
        if (descr->caret_on)
        {
            retvalue = LB_OKAY;
            goto END;
        }
        descr->caret_on = TRUE;
        if ((descr->focus_item != -1) && (GetFocus() == wnd->hwndSelf))
            LISTBOX_RepaintItem( wnd, descr, descr->focus_item, ODA_FOCUS );
        retvalue = LB_OKAY;
        goto END;

    case LB_CARETOFF16:
    case LB_CARETOFF:
        if (!descr->caret_on)
        {
            retvalue = LB_OKAY;
            goto END;
        }
        descr->caret_on = FALSE;
        if ((descr->focus_item != -1) && (GetFocus() == wnd->hwndSelf))
            LISTBOX_RepaintItem( wnd, descr, descr->focus_item, ODA_FOCUS );
        retvalue = LB_OKAY;
        goto END;

    case WM_DESTROY:
        retvalue = LISTBOX_Destroy( wnd, descr );
        goto END;

    case WM_ENABLE:
        InvalidateRect( hwnd, NULL, TRUE );
        retvalue = 0;
        goto END;

    case WM_SETREDRAW:
        LISTBOX_SetRedraw( wnd, descr, wParam != 0 );
        retvalue = 0;
        goto END;

    case WM_GETDLGCODE:
        retvalue =DLGC_WANTARROWS | DLGC_WANTCHARS;
        goto END;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = ( wParam ) ? ((HDC)wParam)
				   :  BeginPaint( hwnd, &ps );
            ret = LISTBOX_Paint( wnd, descr, hdc );
            if( !wParam ) EndPaint( hwnd, &ps );
        }
        retvalue =ret;
        goto END;
    case WM_SIZE:
        LISTBOX_UpdateSize( wnd, descr );
        retvalue =0;
        goto END;
    case WM_GETFONT:
        retvalue =descr->font;
        goto END;
    case WM_SETFONT:
        LISTBOX_SetFont( wnd, descr, (HFONT)wParam );
        if (lParam) InvalidateRect( wnd->hwndSelf, 0, TRUE );
        retvalue =0;
        goto END;
    case WM_SETFOCUS:
        descr->caret_on = TRUE;
        if (descr->focus_item != -1)
            LISTBOX_RepaintItem( wnd, descr, descr->focus_item, ODA_FOCUS );
        SEND_NOTIFICATION( wnd, descr, LBN_SETFOCUS );
        retvalue =0;
        goto END;
    case WM_KILLFOCUS:
        if ((descr->focus_item != -1) && descr->caret_on)
            LISTBOX_RepaintItem( wnd, descr, descr->focus_item, ODA_FOCUS );
        SEND_NOTIFICATION( wnd, descr, LBN_KILLFOCUS );
        retvalue =0;
        goto END;
    case WM_HSCROLL:
        retvalue =LISTBOX_HandleHScroll( wnd, descr, wParam, lParam );
        goto END;
    case WM_VSCROLL:
        retvalue =LISTBOX_HandleVScroll( wnd, descr, wParam, lParam );
        goto END;
    case WM_LBUTTONDOWN:
        retvalue =LISTBOX_HandleLButtonDown( wnd, descr, wParam,
                                          (INT16)LOWORD(lParam),
                                          (INT16)HIWORD(lParam) );
        goto END;
    case WM_LBUTTONDBLCLK:
        if (descr->style & LBS_NOTIFY)
            SEND_NOTIFICATION( wnd, descr, LBN_DBLCLK );
        retvalue =0;
        goto END;
    case WM_MOUSEMOVE:
        if (GetCapture() == hwnd)
            LISTBOX_HandleMouseMove( wnd, descr, (INT16)LOWORD(lParam),
                                     (INT16)HIWORD(lParam) );
        retvalue =0;
        goto END;
    case WM_LBUTTONUP:
        retvalue =LISTBOX_HandleLButtonUp( wnd, descr );
        goto END;
    case WM_KEYDOWN:
        retvalue =LISTBOX_HandleKeyDown( wnd, descr, wParam );
        goto END;
    case WM_CHAR:
        retvalue =LISTBOX_HandleChar( wnd, descr, wParam );
        goto END;
    case WM_SYSTIMER:
        retvalue =LISTBOX_HandleSystemTimer( wnd, descr );
        goto END;
    case WM_ERASEBKGND:
        if (IS_OWNERDRAW(descr))
        {
            RECT rect;
            HBRUSH hbrush = SendMessageA( descr->owner, WM_CTLCOLORLISTBOX,
                                              wParam, (LPARAM)wnd->hwndSelf );
            GetClientRect(hwnd, &rect);
            if (hbrush) FillRect( (HDC)wParam, &rect, hbrush );
        }
        retvalue =1;
        goto END;
    case WM_DROPFILES:
        if( !descr->lphc ) 
        {
            retvalue =SendMessageA( descr->owner, msg, wParam, lParam );
            goto END;
        }
	break;

    case WM_DROPOBJECT:
    case WM_QUERYDROPOBJECT:
    case WM_DRAGSELECT:
    case WM_DRAGMOVE:
	if( !descr->lphc )
        {
            LPDRAGINFO dragInfo = (LPDRAGINFO)PTR_SEG_TO_LIN( (SEGPTR)lParam );
            dragInfo->l = LISTBOX_GetItemFromPoint( wnd, descr, dragInfo->pt.x,
                                                dragInfo->pt.y );
            retvalue =SendMessageA( descr->owner, msg, wParam, lParam );
            goto END;
        }
	break;

    case WM_NCCREATE:
	if (TWEAK_WineLook > WIN31_LOOK)
	    wnd->dwExStyle |= WS_EX_CLIENTEDGE;
        retvalue =DefWindowProcA( hwnd, msg, wParam, lParam );
        goto END;
    default:
        if ((msg >= WM_USER) && (msg < 0xc000))
            WARN(listbox, "[%04x]: unknown msg %04x wp %08x lp %08lx\n",
			 hwnd, msg, wParam, lParam );
        retvalue =DefWindowProcA( hwnd, msg, wParam, lParam );
        goto END;
    }
    retvalue =0;
END:
    WIN_ReleaseWndPtr(wnd);
    return retvalue;
}

/***********************************************************************
 *           COMBO_Directory
 */
LRESULT COMBO_Directory( LPHEADCOMBO lphc, UINT attrib, LPSTR dir, BOOL bLong)
{
    WND *wnd = WIN_FindWndPtr( lphc->hWndLBox );

    if( wnd )
    {
        LB_DESCR *descr = *(LB_DESCR **)wnd->wExtra;
	if( descr )
	{
	    LRESULT	lRet = LISTBOX_Directory( wnd, descr, attrib, dir, bLong );

	    RedrawWindow( lphc->self->hwndSelf, NULL, 0, 
			    RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW );
            WIN_ReleaseWndPtr(wnd);
	    return lRet;
	}
        WIN_ReleaseWndPtr(wnd);
    }
    return CB_ERR;
}

/***********************************************************************
 *           ComboLBWndProc
 *
 *  NOTE: in Windows, winproc address of the ComboLBox is the same 
 *	  as that of the Listbox.
 */
LRESULT WINAPI ComboLBWndProc( HWND hwnd, UINT msg,
                               WPARAM wParam, LPARAM lParam )
{
    LRESULT lRet = 0;
    WND *wnd = WIN_FindWndPtr( hwnd );

    if (wnd)
    {
	LB_DESCR *descr = *(LB_DESCR **)wnd->wExtra;

        TRACE(combo, "[%04x]: msg %s wp %08x lp %08lx\n",
		     wnd->hwndSelf, SPY_GetMsgName(msg), wParam, lParam );

	if( descr || msg == WM_CREATE )
        {
	    LPHEADCOMBO lphc = (descr) ? descr->lphc : NULL;

	    switch( msg )
	    {
		case WM_CREATE:
#define lpcs	((LPCREATESTRUCTA)lParam)
		     TRACE(combo, "\tpassed parent handle = 0x%08x\n", 
				  (UINT)lpcs->lpCreateParams);

		     lphc = (LPHEADCOMBO)(lpcs->lpCreateParams);
#undef  lpcs
		     lRet =LISTBOX_Create( wnd, lphc );
                     goto END;
		case WM_LBUTTONDOWN:
		     lRet =LISTBOX_HandleLButtonDown( wnd, descr, wParam,
                             (INT16)LOWORD(lParam), (INT16)HIWORD(lParam));
                     goto END;
		/* avoid activation at all costs */

		case WM_MOUSEACTIVATE:
		     lRet =MA_NOACTIVATE;
                     goto END;
                case WM_NCACTIVATE:
                     lRet =FALSE;
                     goto END;
		case WM_KEYDOWN:
		     if( CB_GETTYPE(lphc) != CBS_SIMPLE )
		     {
			 /* for some reason(?) Windows makes it possible to
			  * show/hide ComboLBox by sending it WM_KEYDOWNs */

		         if( (!(lphc->wState & CBF_EUI) && wParam == VK_F4) ||
			     ( (lphc->wState & CBF_EUI) && !(lphc->wState & CBF_DROPPED)
			       && (wParam == VK_DOWN || wParam == VK_UP)) )
			 {
			     COMBO_FlipListbox( lphc, FALSE );
                             lRet =0;
                             goto END;
			 }
		     }
		     lRet =LISTBOX_HandleKeyDown( wnd, descr, wParam );
                     goto END;

		case LB_SETCURSEL16:
		case LB_SETCURSEL:
		     lRet = ListBoxWndProc( hwnd, msg, wParam, lParam );
		     lRet =(lRet == LB_ERR) ? lRet : descr->selected_item; 
                     goto END;
		case WM_NCDESTROY:
		     if( CB_GETTYPE(lphc) != CBS_SIMPLE )
			 lphc->hWndLBox = 0;
		     /* fall through */

	        default:
                    lRet =ListBoxWndProc( hwnd, msg, wParam, lParam );
                    goto END;
	    }
        }
        lRet = DefWindowProcA( hwnd, msg, wParam, lParam );

        TRACE(combo,"\t default on msg [%04x]\n", (UINT16)msg );
    }
END:
    WIN_ReleaseWndPtr(wnd);
    return lRet;
}

