/*
 *  Header control
 *
 *  Copyright 1998 Eric Kohl
 *
 *  TODO:
 *   - Imagelist support (partially).
 *   - Callback items (under construction).
 *   - Order list support.
 *   - Control specific cursors (over dividers).
 *   - Hottrack support (partially).
 *   - Custom draw support (including Notifications).
 *   - Drag and Drop support (including Notifications).
 *   - Unicode support.
 *
 *  FIXME:
 *   - Replace DrawText32A by DrawTextEx32A(...|DT_ENDELLIPSIS) in
 *     HEADER_DrawItem.
 *   - Little flaw when drawing a bitmap on the right side of the text.
 */

#include <string.h>

#include "winbase.h"
#include "commctrl.h"
#include "header.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(header)


#define __HDM_LAYOUT_HACK__


#define VERT_BORDER     4
#define DIVIDER_WIDTH  10

#define HEADER_GetInfoPtr(hwnd) ((HEADER_INFO *)GetWindowLongA(hwnd,0))


static INT
HEADER_DrawItem (HWND hwnd, HDC hdc, INT iItem, BOOL bHotTrack)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *phdi = &infoPtr->items[iItem];
    RECT r;
    INT  oldBkMode;

    r = phdi->rect;
    if (r.right - r.left == 0)
	return phdi->rect.right;

    if (GetWindowLongA (hwnd, GWL_STYLE) & HDS_BUTTONS) {
	if (phdi->bDown) {
	    DrawEdge (hdc, &r, BDR_RAISEDOUTER,
			BF_RECT | BF_FLAT | BF_MIDDLE | BF_ADJUST);
	    r.left += 2;
            r.top  += 2;
	}
	else
	    DrawEdge (hdc, &r, EDGE_RAISED,
			BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
    }
    else
        DrawEdge (hdc, &r, EDGE_ETCHED, BF_BOTTOM | BF_RIGHT | BF_ADJUST);

    if (phdi->fmt & HDF_OWNERDRAW) {
	DRAWITEMSTRUCT dis;
	dis.CtlType    = ODT_HEADER;
	dis.CtlID      = GetWindowLongA (hwnd, GWL_ID);
	dis.itemID     = iItem;
	dis.itemAction = ODA_DRAWENTIRE;
	dis.itemState  = phdi->bDown ? ODS_SELECTED : 0;
	dis.hwndItem   = hwnd;
	dis.hDC        = hdc;
	dis.rcItem     = r;
	dis.itemData   = phdi->lParam;
	SendMessageA (GetParent (hwnd), WM_DRAWITEM,
			(WPARAM)dis.CtlID, (LPARAM)&dis);
    }
    else {
        UINT uTextJustify = DT_LEFT;

        if ((phdi->fmt & HDF_JUSTIFYMASK) == HDF_CENTER)
            uTextJustify = DT_CENTER;
        else if ((phdi->fmt & HDF_JUSTIFYMASK) == HDF_RIGHT)
            uTextJustify = DT_RIGHT;

	if ((phdi->fmt & HDF_BITMAP) && (phdi->hbm)) {
	    BITMAP bmp;
	    HDC    hdcBitmap;
	    INT    yD, yS, cx, cy, rx, ry;

	    GetObjectA (phdi->hbm, sizeof(BITMAP), (LPVOID)&bmp);

	    ry = r.bottom - r.top;
	    rx = r.right - r.left;

	    if (ry >= bmp.bmHeight) {
		cy = bmp.bmHeight;
		yD = r.top + (ry - bmp.bmHeight) / 2;
		yS = 0;
	    }
	    else {
		cy = ry;
		yD = r.top;
		yS = (bmp.bmHeight - ry) / 2;

	    }

	    if (rx >= bmp.bmWidth + 6) {
		cx = bmp.bmWidth;
	    }
	    else {
		cx = rx - 6;
	    }

	    hdcBitmap = CreateCompatibleDC (hdc);
	    SelectObject (hdcBitmap, phdi->hbm);
	    BitBlt (hdc, r.left + 3, yD, cx, cy, hdcBitmap, 0, yS, SRCCOPY);
	    DeleteDC (hdcBitmap);

	    r.left += (bmp.bmWidth + 3);
	}


	if ((phdi->fmt & HDF_BITMAP_ON_RIGHT) && (phdi->hbm)) {
	    BITMAP bmp;
	    HDC    hdcBitmap;
	    INT    xD, yD, yS, cx, cy, rx, ry, tx;
	    RECT   textRect;

	    GetObjectA (phdi->hbm, sizeof(BITMAP), (LPVOID)&bmp);

	    textRect = r;
            DrawTextW (hdc, phdi->pszText, lstrlenW (phdi->pszText),
	   	  &textRect, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_CALCRECT);
	    tx = textRect.right - textRect.left;
	    ry = r.bottom - r.top;
	    rx = r.right - r.left;

	    if (ry >= bmp.bmHeight) {
		cy = bmp.bmHeight;
		yD = r.top + (ry - bmp.bmHeight) / 2;
		yS = 0;
	    }
	    else {
		cy = ry;
		yD = r.top;
		yS = (bmp.bmHeight - ry) / 2;

	    }

	    if (r.left + tx + bmp.bmWidth + 9 <= r.right) {
		cx = bmp.bmWidth;
		xD = r.left + tx + 6;
	    }
	    else {
		if (rx >= bmp.bmWidth + 6) {
		    cx = bmp.bmWidth;
		    xD = r.right - bmp.bmWidth - 3;
		    r.right = xD - 3;
		}
		else {
		    cx = rx - 3;
		    xD = r.left;
		    r.right = r.left;
		}
	    }

	    hdcBitmap = CreateCompatibleDC (hdc);
	    SelectObject (hdcBitmap, phdi->hbm);
	    BitBlt (hdc, xD, yD, cx, cy, hdcBitmap, 0, yS, SRCCOPY);
	    DeleteDC (hdcBitmap);
	}

	if (phdi->fmt & HDF_IMAGE) {


/*	    ImageList_Draw (infoPtr->himl, phdi->iImage,...); */
	}

        if ((phdi->fmt & HDF_STRING) && (phdi->pszText)) {
            oldBkMode = SetBkMode(hdc, TRANSPARENT);
            r.left += 3;
	    r.right -= 3;
	    SetTextColor (hdc, bHotTrack ? COLOR_HIGHLIGHT : COLOR_BTNTEXT);
            DrawTextW (hdc, phdi->pszText, lstrlenW (phdi->pszText),
	   	  &r, uTextJustify|DT_VCENTER|DT_SINGLELINE);
            if (oldBkMode != TRANSPARENT)
                SetBkMode(hdc, oldBkMode);
        }
    }

    return phdi->rect.right;
}


static void 
HEADER_Refresh (HWND hwnd, HDC hdc)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HFONT hFont, hOldFont;
    RECT rect;
    HBRUSH hbrBk;
    INT i, x;

    /* get rect for the bar, adjusted for the border */
    GetClientRect (hwnd, &rect);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    /* draw Background */
    hbrBk = GetSysColorBrush(COLOR_3DFACE);
    FillRect(hdc, &rect, hbrBk);

    x = rect.left;
    for (i = 0; i < infoPtr->uNumItem; i++) {
        x = HEADER_DrawItem (hwnd, hdc, i, FALSE);
    }

    if ((x <= rect.right) && (infoPtr->uNumItem > 0)) {
        rect.left = x;
        if (GetWindowLongA (hwnd, GWL_STYLE) & HDS_BUTTONS)
            DrawEdge (hdc, &rect, EDGE_RAISED, BF_TOP|BF_LEFT|BF_BOTTOM|BF_SOFT);
        else
            DrawEdge (hdc, &rect, EDGE_ETCHED, BF_BOTTOM);
    }

    SelectObject (hdc, hOldFont);
}


static void
HEADER_RefreshItem (HWND hwnd, HDC hdc, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HFONT hFont, hOldFont;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);
    HEADER_DrawItem (hwnd, hdc, iItem, FALSE);
    SelectObject (hdc, hOldFont);
}


static void
HEADER_SetItemBounds (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *phdi;
    RECT rect;
    int i, x;

    if (infoPtr->uNumItem == 0)
        return;

    GetClientRect (hwnd, &rect);

    x = rect.left;
    for (i = 0; i < infoPtr->uNumItem; i++) {
        phdi = &infoPtr->items[i];
        phdi->rect.top = rect.top;
        phdi->rect.bottom = rect.bottom;
        phdi->rect.left = x;
        phdi->rect.right = phdi->rect.left + phdi->cxy;
        x = phdi->rect.right;
    }
}


static void
HEADER_ForceItemBounds (HWND hwnd, INT cy)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *phdi;
    int i, x;

    if (infoPtr->uNumItem == 0)
	return;

    x = 0;
    for (i = 0; i < infoPtr->uNumItem; i++) {
	phdi = &infoPtr->items[i];
	phdi->rect.top = 0;
	phdi->rect.bottom = cy;
	phdi->rect.left = x;
	phdi->rect.right = phdi->rect.left + phdi->cxy;
	x = phdi->rect.right;
    }
}


static void
HEADER_InternalHitTest (HWND hwnd, LPPOINT lpPt, UINT *pFlags, INT *pItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    RECT rect, rcTest;
    INT  iCount, width;
    BOOL bNoWidth;

    GetClientRect (hwnd, &rect);

    *pFlags = 0;
    bNoWidth = FALSE;
    if (PtInRect (&rect, *lpPt))
    {
	if (infoPtr->uNumItem == 0) {
	    *pFlags |= HHT_NOWHERE;
	    *pItem = 1;
	    TRACE (header, "NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    for (iCount = 0; iCount < infoPtr->uNumItem; iCount++) {
		rect = infoPtr->items[iCount].rect;
		width = rect.right - rect.left;
		if (width == 0) {
		    bNoWidth = TRUE;
		    continue;
		}
		if (PtInRect (&rect, *lpPt)) {
		    if (width <= 2 * DIVIDER_WIDTH) {
			*pFlags |= HHT_ONHEADER;
			*pItem = iCount;
			TRACE (header, "ON HEADER %d\n", iCount);
			return;
		    }
		    if (iCount > 0) {
			rcTest = rect;
			rcTest.right = rcTest.left + DIVIDER_WIDTH;
			if (PtInRect (&rcTest, *lpPt)) {
			    if (bNoWidth) {
				*pFlags |= HHT_ONDIVOPEN;
				*pItem = iCount - 1;
				TRACE (header, "ON DIVOPEN %d\n", *pItem);
				return;
			    }
			    else {
				*pFlags |= HHT_ONDIVIDER;
				*pItem = iCount - 1;
				TRACE (header, "ON DIVIDER %d\n", *pItem);
				return;
			    }
			}
		    }
		    rcTest = rect;
		    rcTest.left = rcTest.right - DIVIDER_WIDTH;
		    if (PtInRect (&rcTest, *lpPt)) {
			*pFlags |= HHT_ONDIVIDER;
			*pItem = iCount;
			TRACE (header, "ON DIVIDER %d\n", *pItem);
			return;
		    }

		    *pFlags |= HHT_ONHEADER;
		    *pItem = iCount;
		    TRACE (header, "ON HEADER %d\n", iCount);
		    return;
		}
	    }

	    /* check for last divider part (on nowhere) */
	    rect = infoPtr->items[infoPtr->uNumItem-1].rect;
	    rect.left = rect.right;
	    rect.right += DIVIDER_WIDTH;
	    if (PtInRect (&rect, *lpPt)) {
		if (bNoWidth) {
		    *pFlags |= HHT_ONDIVOPEN;
		    *pItem = infoPtr->uNumItem - 1;
		    TRACE (header, "ON DIVOPEN %d\n", *pItem);
		    return;
		}
		else {
		    *pFlags |= HHT_ONDIVIDER;
		    *pItem = infoPtr->uNumItem-1;
		    TRACE (header, "ON DIVIDER %d\n", *pItem);
		    return;
		}
	    }

	    *pFlags |= HHT_NOWHERE;
	    *pItem = 1;
	    TRACE (header, "NOWHERE\n");
	    return;
	}
    }
    else {
	if (lpPt->x < rect.left) {
	   TRACE (header, "TO LEFT\n");
	   *pFlags |= HHT_TOLEFT;
	}
	else if (lpPt->x > rect.right) {
	    TRACE (header, "TO LEFT\n");
	    *pFlags |= HHT_TORIGHT;
	}

	if (lpPt->y < rect.top) {
	    TRACE (header, "ABOVE\n");
	    *pFlags |= HHT_ABOVE;
	}
	else if (lpPt->y > rect.bottom) {
	    TRACE (header, "BELOW\n");
	    *pFlags |= HHT_BELOW;
	}
    }

    *pItem = 1;
    TRACE (header, "flags=0x%X\n", *pFlags);
    return;
}


static void
HEADER_DrawTrackLine (HWND hwnd, HDC hdc, INT x)
{
    RECT rect;
    HPEN hOldPen;
    INT  oldRop;

    GetClientRect (hwnd, &rect);

    hOldPen = SelectObject (hdc, GetStockObject (BLACK_PEN));
    oldRop = SetROP2 (hdc, R2_XORPEN);
    MoveToEx (hdc, x, rect.top, NULL);
    LineTo (hdc, x, rect.bottom);
    SetROP2 (hdc, oldRop);
    SelectObject (hdc, hOldPen);
}


static BOOL
HEADER_SendSimpleNotify (HWND hwnd, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
    nmhdr.code     = code;

    return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
				   (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}


static BOOL
HEADER_SendHeaderNotify (HWND hwnd, UINT code, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    NMHEADERA nmhdr;
    HDITEMA nmitem;

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
    nmhdr.hdr.code = code;
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = &nmitem;
    nmitem.mask = 0;
    nmitem.cxy = infoPtr->items[iItem].cxy;
    nmitem.hbm = infoPtr->items[iItem].hbm;
    nmitem.pszText = NULL;
    nmitem.cchTextMax = 0;
/*    nmitem.pszText = infoPtr->items[iItem].pszText; */
/*    nmitem.cchTextMax = infoPtr->items[iItem].cchTextMax; */
    nmitem.fmt = infoPtr->items[iItem].fmt;
    nmitem.lParam = infoPtr->items[iItem].lParam;
    nmitem.iOrder = infoPtr->items[iItem].iOrder;
    nmitem.iImage = infoPtr->items[iItem].iImage;

    return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
			       (WPARAM)nmhdr.hdr.idFrom, (LPARAM)&nmhdr);
}


static BOOL
HEADER_SendClickNotify (HWND hwnd, UINT code, INT iItem)
{
    NMHEADERA nmhdr;

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
    nmhdr.hdr.code = code;
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = NULL;

    return (BOOL)SendMessageA (GetParent (hwnd), WM_NOTIFY,
			       (WPARAM)nmhdr.hdr.idFrom, (LPARAM)&nmhdr);
}


static LRESULT
HEADER_CreateDragImage (HWND hwnd, WPARAM wParam)
{
    FIXME (header, "empty stub!\n");
    return 0;
}


static LRESULT
HEADER_DeleteItem (HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(hwnd);
    INT iItem = (INT)wParam;
    HDC hdc;

    TRACE(header, "[iItem=%d]\n", iItem);
    
    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    if (infoPtr->uNumItem == 1) {
        TRACE(header, "Simple delete!\n");
        if (infoPtr->items[0].pszText)
            COMCTL32_Free (infoPtr->items[0].pszText);
        COMCTL32_Free (infoPtr->items);
        infoPtr->items = 0;
        infoPtr->uNumItem = 0;
    }
    else {
        HEADER_ITEM *oldItems = infoPtr->items;
        TRACE(header, "Complex delete! [iItem=%d]\n", iItem);

        if (infoPtr->items[iItem].pszText)
            COMCTL32_Free (infoPtr->items[iItem].pszText);

        infoPtr->uNumItem--;
        infoPtr->items = COMCTL32_Alloc (sizeof (HEADER_ITEM) * infoPtr->uNumItem);
        /* pre delete copy */
        if (iItem > 0) {
            memcpy (&infoPtr->items[0], &oldItems[0],
                    iItem * sizeof(HEADER_ITEM));
        }

        /* post delete copy */
        if (iItem < infoPtr->uNumItem) {
            memcpy (&infoPtr->items[iItem], &oldItems[iItem+1],
                    (infoPtr->uNumItem - iItem) * sizeof(HEADER_ITEM));
        }

        COMCTL32_Free (oldItems);
    }

    HEADER_SetItemBounds (hwnd);

    hdc = GetDC (hwnd);
    HEADER_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);
    
    return TRUE;
}


static LRESULT
HEADER_GetImageList (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->himl;
}


static LRESULT
HEADER_GetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMA   *phdi = (HDITEMA*)lParam;
    INT       nItem = (INT)wParam;
    HEADER_ITEM *lpItem;

    if (!phdi)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (phdi->mask == 0)
	return TRUE;

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    if (phdi->mask & HDI_BITMAP)
	phdi->hbm = lpItem->hbm;

    if (phdi->mask & HDI_FORMAT)
	phdi->fmt = lpItem->fmt;

    if (phdi->mask & HDI_WIDTH)
	phdi->cxy = lpItem->cxy;

    if (phdi->mask & HDI_LPARAM)
	phdi->lParam = lpItem->lParam;

    if (phdi->mask & HDI_TEXT) {
	if (lpItem->pszText != LPSTR_TEXTCALLBACKW)
	    lstrcpynWtoA (phdi->pszText, lpItem->pszText, phdi->cchTextMax);
	else
	    phdi->pszText = LPSTR_TEXTCALLBACKA;
    }

    if (phdi->mask & HDI_IMAGE)
	phdi->iImage = lpItem->iImage;

    if (phdi->mask & HDI_ORDER)
	phdi->iOrder = lpItem->iOrder;

    return TRUE;
}


static LRESULT
HEADER_GetItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMW   *phdi = (HDITEMW*)lParam;
    INT       nItem = (INT)wParam;
    HEADER_ITEM *lpItem;

    if (!phdi)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (phdi->mask == 0)
	return TRUE;

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    if (phdi->mask & HDI_BITMAP)
	phdi->hbm = lpItem->hbm;

    if (phdi->mask & HDI_FORMAT)
	phdi->fmt = lpItem->fmt;

    if (phdi->mask & HDI_WIDTH)
	phdi->cxy = lpItem->cxy;

    if (phdi->mask & HDI_LPARAM)
	phdi->lParam = lpItem->lParam;

    if (phdi->mask & HDI_TEXT) {
	if (lpItem->pszText != LPSTR_TEXTCALLBACKW)
	    lstrcpynW (phdi->pszText, lpItem->pszText, phdi->cchTextMax);
	else
	    phdi->pszText = LPSTR_TEXTCALLBACKW;
    }

    if (phdi->mask & HDI_IMAGE)
	phdi->iImage = lpItem->iImage;

    if (phdi->mask & HDI_ORDER)
	phdi->iOrder = lpItem->iOrder;

    return TRUE;
}


inline static LRESULT
HEADER_GetItemCount (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    return infoPtr->uNumItem;
}


static LRESULT
HEADER_GetItemRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    INT iItem = (INT)wParam;
    LPRECT lpRect = (LPRECT)lParam;

    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    lpRect->left   = infoPtr->items[iItem].rect.left;
    lpRect->right  = infoPtr->items[iItem].rect.right;
    lpRect->top    = infoPtr->items[iItem].rect.top;
    lpRect->bottom = infoPtr->items[iItem].rect.bottom;

    return TRUE;
}


/* << HEADER_GetOrderArray >> */


inline static LRESULT
HEADER_GetUnicodeFormat (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    return infoPtr->bUnicode;
}


static LRESULT
HEADER_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPHDHITTESTINFO phti = (LPHDHITTESTINFO)lParam;

    HEADER_InternalHitTest (hwnd, &phti->pt, &phti->flags, &phti->iItem);

    return phti->flags;
}


static LRESULT
HEADER_InsertItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMA   *phdi = (HDITEMA*)lParam;
    INT       nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC       hdc;
    INT       len;

    if ((phdi == NULL) || (nItem < 0))
	return -1;

    if (nItem > infoPtr->uNumItem)
        nItem = infoPtr->uNumItem;

    if (infoPtr->uNumItem == 0) {
        infoPtr->items = COMCTL32_Alloc (sizeof (HEADER_ITEM));
        infoPtr->uNumItem++;
    }
    else {
        HEADER_ITEM *oldItems = infoPtr->items;

        infoPtr->uNumItem++;
        infoPtr->items = COMCTL32_Alloc (sizeof (HEADER_ITEM) * infoPtr->uNumItem);
        if (nItem == 0) {
            memcpy (&infoPtr->items[1], &oldItems[0],
                    (infoPtr->uNumItem-1) * sizeof(HEADER_ITEM));
        }
        else
        {
              /* pre insert copy */
            if (nItem > 0) {
                 memcpy (&infoPtr->items[0], &oldItems[0],
                         nItem * sizeof(HEADER_ITEM));
            }

            /* post insert copy */
            if (nItem < infoPtr->uNumItem - 1) {
                memcpy (&infoPtr->items[nItem+1], &oldItems[nItem],
                        (infoPtr->uNumItem - nItem) * sizeof(HEADER_ITEM));
            }
        }
    
        COMCTL32_Free (oldItems);
    }

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    lpItem->bDown = FALSE;

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_TEXT) {
	if (!phdi->pszText) /* null pointer check */
	    phdi->pszText = "";
	if (phdi->pszText != LPSTR_TEXTCALLBACKA) {
	    len = lstrlenA (phdi->pszText);
	    lpItem->pszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyAtoW (lpItem->pszText, phdi->pszText);
	}
	else
	    lpItem->pszText = LPSTR_TEXTCALLBACKW;
    }

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (lpItem->fmt == 0)
	lpItem->fmt = HDF_LEFT;

    if (phdi->mask & HDI_BITMAP)
        lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_LPARAM)
        lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_IMAGE)
        lpItem->iImage = phdi->iImage;

    if (phdi->mask & HDI_ORDER)
        lpItem->iOrder = phdi->iOrder;

    HEADER_SetItemBounds (hwnd);

    hdc = GetDC (hwnd);
    HEADER_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return nItem;
}


static LRESULT
HEADER_InsertItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMW   *phdi = (HDITEMW*)lParam;
    INT       nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC       hdc;
    INT       len;

    if ((phdi == NULL) || (nItem < 0))
	return -1;

    if (nItem > infoPtr->uNumItem)
        nItem = infoPtr->uNumItem;

    if (infoPtr->uNumItem == 0) {
        infoPtr->items = COMCTL32_Alloc (sizeof (HEADER_ITEM));
        infoPtr->uNumItem++;
    }
    else {
        HEADER_ITEM *oldItems = infoPtr->items;

        infoPtr->uNumItem++;
        infoPtr->items = COMCTL32_Alloc (sizeof (HEADER_ITEM) * infoPtr->uNumItem);
        /* pre insert copy */
        if (nItem > 0) {
            memcpy (&infoPtr->items[0], &oldItems[0],
                    nItem * sizeof(HEADER_ITEM));
        }

        /* post insert copy */
        if (nItem < infoPtr->uNumItem - 1) {
            memcpy (&infoPtr->items[nItem+1], &oldItems[nItem],
                    (infoPtr->uNumItem - nItem) * sizeof(HEADER_ITEM));
        }

	COMCTL32_Free (oldItems);
    }

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    lpItem->bDown = FALSE;

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_TEXT) {
	WCHAR wide_null_char = 0;
	if (!phdi->pszText) /* null pointer check */
	    phdi->pszText = &wide_null_char;	
	if (phdi->pszText != LPSTR_TEXTCALLBACKW) {
	    len = lstrlenW (phdi->pszText);
	    lpItem->pszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyW (lpItem->pszText, phdi->pszText);
	}
	else
	    lpItem->pszText = LPSTR_TEXTCALLBACKW;
    }

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (lpItem->fmt == 0)
	lpItem->fmt = HDF_LEFT;

    if (phdi->mask & HDI_BITMAP)
        lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_LPARAM)
        lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_IMAGE)
        lpItem->iImage = phdi->iImage;

    if (phdi->mask & HDI_ORDER)
        lpItem->iOrder = phdi->iOrder;

    HEADER_SetItemBounds (hwnd);

    hdc = GetDC (hwnd);
    HEADER_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return nItem;
}


static LRESULT
HEADER_Layout (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    LPHDLAYOUT lpLayout = (LPHDLAYOUT)lParam;

    lpLayout->pwpos->hwnd = hwnd;
    lpLayout->pwpos->hwndInsertAfter = 0;
    lpLayout->pwpos->x = lpLayout->prc->left;
    lpLayout->pwpos->y = lpLayout->prc->top;
    lpLayout->pwpos->cx = lpLayout->prc->right - lpLayout->prc->left;
    if (GetWindowLongA (hwnd, GWL_STYLE) & HDS_HIDDEN)
        lpLayout->pwpos->cy = 0;
    else
        lpLayout->pwpos->cy = infoPtr->nHeight;
    lpLayout->pwpos->flags = SWP_NOZORDER;

    TRACE (header, "Layout x=%d y=%d cx=%d cy=%d\n",
           lpLayout->pwpos->x, lpLayout->pwpos->y,
           lpLayout->pwpos->cx, lpLayout->pwpos->cy);

    HEADER_ForceItemBounds (hwnd, lpLayout->pwpos->cy);

    /* hack */
#ifdef __HDM_LAYOUT_HACK__
    MoveWindow (lpLayout->pwpos->hwnd, lpLayout->pwpos->x, lpLayout->pwpos->y,
                  lpLayout->pwpos->cx, lpLayout->pwpos->cy, TRUE);
#endif

    return TRUE;
}


static LRESULT
HEADER_SetImageList (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HIMAGELIST himlOld;

    himlOld = infoPtr->himl;
    infoPtr->himl = (HIMAGELIST)lParam;

    /* FIXME: Refresh needed??? */

    return (LRESULT)himlOld;
}


static LRESULT
HEADER_SetItemA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMA *phdi = (HDITEMA*)lParam;
    INT nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC hdc;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGINGA, nItem))
	return FALSE;

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    if (phdi->mask & HDI_BITMAP)
	lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (phdi->mask & HDI_LPARAM)
	lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_TEXT) {
	if (phdi->pszText != LPSTR_TEXTCALLBACKA) {
	    if (lpItem->pszText) {
		COMCTL32_Free (lpItem->pszText);
		lpItem->pszText = NULL;
	    }
	    if (phdi->pszText) {
		INT len = lstrlenA (phdi->pszText);
		lpItem->pszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyAtoW (lpItem->pszText, phdi->pszText);
	    }
	}
	else
	    lpItem->pszText = LPSTR_TEXTCALLBACKW;
    }

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_IMAGE)
	lpItem->iImage = phdi->iImage;

    if (phdi->mask & HDI_ORDER)
	lpItem->iOrder = phdi->iOrder;

    HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGEDA, nItem);

    HEADER_SetItemBounds (hwnd);
    hdc = GetDC (hwnd);
    HEADER_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return TRUE;
}


static LRESULT
HEADER_SetItemW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HDITEMW *phdi = (HDITEMW*)lParam;
    INT nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC hdc;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGINGA, nItem))
	return FALSE;

    lpItem = (HEADER_ITEM*)&infoPtr->items[nItem];
    if (phdi->mask & HDI_BITMAP)
	lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (phdi->mask & HDI_LPARAM)
	lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_TEXT) {
	if (phdi->pszText != LPSTR_TEXTCALLBACKW) {
	    if (lpItem->pszText) {
		COMCTL32_Free (lpItem->pszText);
		lpItem->pszText = NULL;
	    }
	    if (phdi->pszText) {
		INT len = lstrlenW (phdi->pszText);
		lpItem->pszText = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
		lstrcpyW (lpItem->pszText, phdi->pszText);
	    }
	}
	else
	    lpItem->pszText = LPSTR_TEXTCALLBACKW;
    }

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_IMAGE)
	lpItem->iImage = phdi->iImage;

    if (phdi->mask & HDI_ORDER)
	lpItem->iOrder = phdi->iOrder;

    HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGEDA, nItem);

    HEADER_SetItemBounds (hwnd);
    hdc = GetDC (hwnd);
    HEADER_Refresh (hwnd, hdc);
    ReleaseDC (hwnd, hdc);

    return TRUE;
}


/* << HEADER_SetOrderArray >> */


inline static LRESULT
HEADER_SetUnicodeFormat (HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
HEADER_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr;
    TEXTMETRICA tm;
    HFONT hOldFont;
    HDC   hdc;

    infoPtr = (HEADER_INFO *)COMCTL32_Alloc (sizeof(HEADER_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    infoPtr->uNumItem = 0;
    infoPtr->nHeight = 20;
    infoPtr->hFont = 0;
    infoPtr->items = 0;
    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->hcurDivider = LoadCursorA (0, IDC_SIZEWEA);
    infoPtr->hcurDivopen = LoadCursorA (0, IDC_SIZENSA);
    infoPtr->bPressed  = FALSE;
    infoPtr->bTracking = FALSE;
    infoPtr->iMoveItem = 0;
    infoPtr->himl = 0;
    infoPtr->iHotItem = -1;
    infoPtr->bUnicode = IsWindowUnicode (hwnd);

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, GetStockObject (SYSTEM_FONT));
    GetTextMetricsA (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    return 0;
}


static LRESULT
HEADER_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;
    INT nItem;

    if (infoPtr->items) {
	lpItem = (HEADER_ITEM*)infoPtr->items;
        for (nItem = 0; nItem < infoPtr->uNumItem; nItem++, lpItem++) {
	    if ((lpItem->pszText) && (lpItem->pszText != LPSTR_TEXTCALLBACKW))
		COMCTL32_Free (lpItem->pszText);
        }
        COMCTL32_Free (infoPtr->items);
    }

    if (infoPtr->himl)
	ImageList_Destroy (infoPtr->himl);

    COMCTL32_Free (infoPtr);

    return 0;
}


static inline LRESULT
HEADER_GetFont (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
HEADER_LButtonDblClk (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    UINT  flags;
    INT   nItem;

    pt.x = (INT)LOWORD(lParam); 
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if ((GetWindowLongA (hwnd, GWL_STYLE) & HDS_BUTTONS) && (flags == HHT_ONHEADER))
	HEADER_SendHeaderNotify (hwnd, HDN_ITEMDBLCLICKA, nItem);
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN))
	HEADER_SendHeaderNotify (hwnd, HDN_DIVIDERDBLCLICKA, nItem);

    return 0;
}


static LRESULT
HEADER_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;
    INT   nItem;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam); 
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if ((dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER)) {
	SetCapture (hwnd);
	infoPtr->bCaptured = TRUE;   
	infoPtr->bPressed  = TRUE;
	infoPtr->iMoveItem = nItem;

	infoPtr->items[nItem].bDown = TRUE;

	/* Send WM_CUSTOMDRAW */
	hdc = GetDC (hwnd);
	HEADER_RefreshItem (hwnd, hdc, nItem);
	ReleaseDC (hwnd, hdc);

	TRACE (header, "Pressed item %d!\n", nItem);
    } 
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN)) {
	if (!(HEADER_SendHeaderNotify (hwnd, HDN_BEGINTRACKA, nItem))) {
	    SetCapture (hwnd);
	    infoPtr->bCaptured = TRUE;   
	    infoPtr->bTracking = TRUE;
	    infoPtr->iMoveItem = nItem;
	    infoPtr->nOldWidth = infoPtr->items[nItem].cxy;
	    infoPtr->xTrackOffset = infoPtr->items[nItem].rect.right - pt.x;

	    if (!(dwStyle & HDS_FULLDRAG)) {
		infoPtr->xOldTrack = infoPtr->items[nItem].rect.right;
		hdc = GetDC (hwnd);
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		ReleaseDC (hwnd, hdc);
	    }

	    TRACE (header, "Begin tracking item %d!\n", nItem);
	}
    }

    return 0;
}


static LRESULT
HEADER_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if (infoPtr->bPressed) {
	if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER)) {
	    infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
	    hdc = GetDC (hwnd);
	    HEADER_RefreshItem (hwnd, hdc, infoPtr->iMoveItem);
	    ReleaseDC (hwnd, hdc);

	    HEADER_SendClickNotify (hwnd, HDN_ITEMCLICKA, infoPtr->iMoveItem);
	}
	TRACE (header, "Released item %d!\n", infoPtr->iMoveItem);
	infoPtr->bPressed = FALSE;
    }
    else if (infoPtr->bTracking) {
	TRACE (header, "End tracking item %d!\n", infoPtr->iMoveItem);
	infoPtr->bTracking = FALSE;

	HEADER_SendHeaderNotify (hwnd, HDN_ENDTRACKA, infoPtr->iMoveItem);

	if (!(dwStyle & HDS_FULLDRAG)) {
	    hdc = GetDC (hwnd);
	    HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
	    ReleaseDC (hwnd, hdc);
	    if (HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGINGA, infoPtr->iMoveItem))
		infoPtr->items[infoPtr->iMoveItem].cxy = infoPtr->nOldWidth;
	    else {
		nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		if (nWidth < 0)
		    nWidth = 0;
		infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
		HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGEDA, infoPtr->iMoveItem);
	    }

	    HEADER_SetItemBounds (hwnd);
	    hdc = GetDC (hwnd);
	    HEADER_Refresh (hwnd, hdc);
	    ReleaseDC (hwnd, hdc);
	}
    }

    if (infoPtr->bCaptured) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	HEADER_SendSimpleNotify (hwnd, NM_RELEASEDCAPTURE);
    }

    return 0;
}


static LRESULT
HEADER_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if ((dwStyle & HDS_BUTTONS) && (dwStyle & HDS_HOTTRACK)) {
	if (flags & (HHT_ONHEADER | HHT_ONDIVIDER | HHT_ONDIVOPEN))
	    infoPtr->iHotItem = nItem;
	else
	    infoPtr->iHotItem = -1;
	hdc = GetDC (hwnd);
	HEADER_Refresh (hwnd, hdc);
	ReleaseDC (hwnd, hdc);
    }

    if (infoPtr->bCaptured) {
	if (infoPtr->bPressed) {
	    if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER))
		infoPtr->items[infoPtr->iMoveItem].bDown = TRUE;
	    else
		infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
	    hdc = GetDC (hwnd);
	    HEADER_RefreshItem (hwnd, hdc, infoPtr->iMoveItem);
	    ReleaseDC (hwnd, hdc);

	    TRACE (header, "Moving pressed item %d!\n", infoPtr->iMoveItem);
	}
	else if (infoPtr->bTracking) {
	    if (dwStyle & HDS_FULLDRAG) {
		if (HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGINGA, infoPtr->iMoveItem))
		    infoPtr->items[infoPtr->iMoveItem].cxy = infoPtr->nOldWidth;
		else {
		    nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		    if (nWidth < 0)
			nWidth = 0;
		    infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
		    HEADER_SendHeaderNotify (hwnd, HDN_ITEMCHANGEDA,
					     infoPtr->iMoveItem);
		}
		HEADER_SetItemBounds (hwnd);
		hdc = GetDC (hwnd);
		HEADER_Refresh (hwnd, hdc);
		ReleaseDC (hwnd, hdc);
	    }
	    else {
		hdc = GetDC (hwnd);
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		infoPtr->xOldTrack = pt.x + infoPtr->xTrackOffset;
		if (infoPtr->xOldTrack < infoPtr->items[infoPtr->iMoveItem].rect.left)
		    infoPtr->xOldTrack = infoPtr->items[infoPtr->iMoveItem].rect.left;
		infoPtr->items[infoPtr->iMoveItem].cxy = 
		    infoPtr->xOldTrack - infoPtr->items[infoPtr->iMoveItem].rect.left;
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		ReleaseDC (hwnd, hdc);
	    }

	    HEADER_SendHeaderNotify (hwnd, HDN_TRACKA, infoPtr->iMoveItem);
	    TRACE (header, "Tracking item %d!\n", infoPtr->iMoveItem);
	}
    }

    if ((dwStyle & HDS_BUTTONS) && (dwStyle & HDS_HOTTRACK)) {
	FIXME (header, "hot track support!\n");
    }

    return 0;
}


static LRESULT
HEADER_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    HEADER_Refresh (hwnd, hdc);
    if(!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
HEADER_RButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    return HEADER_SendSimpleNotify (hwnd, NM_RCLICK);
}


static LRESULT
HEADER_SetCursor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    POINT pt;
    UINT  flags;
    INT   nItem;

    TRACE (header, "code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (hwnd, &pt);

    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if (flags == HHT_ONDIVIDER)
        SetCursor (infoPtr->hcurDivider);
    else if (flags == HHT_ONDIVOPEN)
        SetCursor (infoPtr->hcurDivopen);
    else
        SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
HEADER_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    TEXTMETRICA tm;
    HFONT hFont, hOldFont;
    HDC hdc;

    infoPtr->hFont = (HFONT)wParam;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, hFont);
    GetTextMetricsA (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    if (lParam) {
        HEADER_ForceItemBounds (hwnd, infoPtr->nHeight);
        hdc = GetDC (hwnd);
        HEADER_Refresh (hwnd, hdc);
        ReleaseDC (hwnd, hdc);
    }

    return 0;
}


LRESULT WINAPI
HEADER_WindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	case HDM_CREATEDRAGIMAGE:
	    return HEADER_CreateDragImage (hwnd, wParam);

	case HDM_DELETEITEM:
	    return HEADER_DeleteItem (hwnd, wParam);

	case HDM_GETIMAGELIST:
	    return HEADER_GetImageList (hwnd);

	case HDM_GETITEMA:
	    return HEADER_GetItemA (hwnd, wParam, lParam);

	case HDM_GETITEMW:
	    return HEADER_GetItemW (hwnd, wParam, lParam);

	case HDM_GETITEMCOUNT:
	    return HEADER_GetItemCount (hwnd);

	case HDM_GETITEMRECT:
	    return HEADER_GetItemRect (hwnd, wParam, lParam);

/*	case HDM_GETORDERARRAY: */

	case HDM_GETUNICODEFORMAT:
	    return HEADER_GetUnicodeFormat (hwnd);

	case HDM_HITTEST:
	    return HEADER_HitTest (hwnd, wParam, lParam);

	case HDM_INSERTITEMA:
	    return HEADER_InsertItemA (hwnd, wParam, lParam);

	case HDM_INSERTITEMW:
	    return HEADER_InsertItemW (hwnd, wParam, lParam);

	case HDM_LAYOUT:
	    return HEADER_Layout (hwnd, wParam, lParam);

	case HDM_SETIMAGELIST:
	    return HEADER_SetImageList (hwnd, wParam, lParam);

	case HDM_SETITEMA:
	    return HEADER_SetItemA (hwnd, wParam, lParam);

	case HDM_SETITEMW:
	    return HEADER_SetItemW (hwnd, wParam, lParam);

/*	case HDM_SETORDERARRAY: */

	case HDM_SETUNICODEFORMAT:
	    return HEADER_SetUnicodeFormat (hwnd, wParam);


        case WM_CREATE:
            return HEADER_Create (hwnd, wParam, lParam);

        case WM_DESTROY:
            return HEADER_Destroy (hwnd, wParam, lParam);

        case WM_ERASEBKGND:
            return 1;

        case WM_GETDLGCODE:
            return DLGC_WANTTAB | DLGC_WANTARROWS;

        case WM_GETFONT:
            return HEADER_GetFont (hwnd);

        case WM_LBUTTONDBLCLK:
            return HEADER_LButtonDblClk (hwnd, wParam, lParam);

        case WM_LBUTTONDOWN:
            return HEADER_LButtonDown (hwnd, wParam, lParam);

        case WM_LBUTTONUP:
            return HEADER_LButtonUp (hwnd, wParam, lParam);

        case WM_MOUSEMOVE:
            return HEADER_MouseMove (hwnd, wParam, lParam);

/*	case WM_NOTIFYFORMAT: */

        case WM_PAINT:
            return HEADER_Paint (hwnd, wParam);

        case WM_RBUTTONUP:
            return HEADER_RButtonUp (hwnd, wParam, lParam);

        case WM_SETCURSOR:
            return HEADER_SetCursor (hwnd, wParam, lParam);

        case WM_SETFONT:
            return HEADER_SetFont (hwnd, wParam, lParam);

        default:
            if (msg >= WM_USER) 
		ERR (header, "unknown msg %04x wp=%04x lp=%08lx\n",
		     msg, wParam, lParam );
	    return DefWindowProcA (hwnd, msg, wParam, lParam);
    }
    return 0;
}


VOID
HEADER_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (WC_HEADERA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)HEADER_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(HEADER_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.lpszClassName = WC_HEADERA;
 
    RegisterClassA (&wndClass);
}


VOID
HEADER_Unregister (VOID)
{
    if (GlobalFindAtomA (WC_HEADERA))
	UnregisterClassA (WC_HEADERA, (HINSTANCE)NULL);
}

