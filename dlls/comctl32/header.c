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

#include "commctrl.h"
#include "win.h"
#include "header.h"
#include "debug.h"


#define __HDM_LAYOUT_HACK__


#define VERT_BORDER     4
#define DIVIDER_WIDTH  10

#define HEADER_GetInfoPtr(wndPtr) ((HEADER_INFO *)wndPtr->wExtra[0])


static INT
HEADER_DrawItem (WND *wndPtr, HDC hdc, INT iItem, BOOL bHotTrack)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HEADER_ITEM *phdi = &infoPtr->items[iItem];
    RECT r;
    INT  oldBkMode;

    r = phdi->rect;
    if (r.right - r.left == 0)
	return phdi->rect.right;

    if (wndPtr->dwStyle & HDS_BUTTONS) {
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
	dis.CtlID      = wndPtr->wIDmenu;
	dis.itemID     = iItem;
	dis.itemAction = ODA_DRAWENTIRE;
	dis.itemState  = phdi->bDown ? ODS_SELECTED : 0;
	dis.hwndItem   = wndPtr->hwndSelf;
	dis.hDC        = hdc;
	dis.rcItem     = r;
	dis.itemData   = phdi->lParam;
	SendMessageA (GetParent (wndPtr->hwndSelf), WM_DRAWITEM,
			(WPARAM)wndPtr->wIDmenu, (LPARAM)&dis);
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
HEADER_Refresh (WND *wndPtr, HDC hdc)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HFONT hFont, hOldFont;
    RECT rect;
    HBRUSH hbrBk;
    INT i, x;

    /* get rect for the bar, adjusted for the border */
    GetClientRect (wndPtr->hwndSelf, &rect);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    /* draw Background */
    hbrBk = GetSysColorBrush(COLOR_3DFACE);
    FillRect(hdc, &rect, hbrBk);

    x = rect.left;
    for (i = 0; i < infoPtr->uNumItem; i++) {
        x = HEADER_DrawItem (wndPtr, hdc, i, FALSE);
    }

    if ((x <= rect.right) && (infoPtr->uNumItem > 0)) {
        rect.left = x;
        if (wndPtr->dwStyle & HDS_BUTTONS)
            DrawEdge (hdc, &rect, EDGE_RAISED, BF_TOP|BF_LEFT|BF_BOTTOM|BF_SOFT);
        else
            DrawEdge (hdc, &rect, EDGE_ETCHED, BF_BOTTOM);
    }

    SelectObject (hdc, hOldFont);
}


static void
HEADER_RefreshItem (WND *wndPtr, HDC hdc, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HFONT hFont, hOldFont;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);
    HEADER_DrawItem (wndPtr, hdc, iItem, FALSE);
    SelectObject (hdc, hOldFont);
}


static void
HEADER_SetItemBounds (WND *wndPtr)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HEADER_ITEM *phdi;
    RECT rect;
    int i, x;

    if (infoPtr->uNumItem == 0)
        return;

    GetClientRect (wndPtr->hwndSelf, &rect);

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
HEADER_ForceItemBounds (WND *wndPtr, INT cy)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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
HEADER_InternalHitTest (WND *wndPtr, LPPOINT lpPt, UINT *pFlags, INT *pItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    RECT rect, rcTest;
    INT  iCount, width;
    BOOL bNoWidth;

    GetClientRect (wndPtr->hwndSelf, &rect);

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
HEADER_DrawTrackLine (WND *wndPtr, HDC hdc, INT x)
{
    RECT rect;
    HPEN hOldPen;
    INT  oldRop;

    GetClientRect (wndPtr->hwndSelf, &rect);

    hOldPen = SelectObject (hdc, GetStockObject (BLACK_PEN));
    oldRop = SetROP2 (hdc, R2_XORPEN);
    MoveToEx (hdc, x, rect.top, NULL);
    LineTo (hdc, x, rect.bottom);
    SetROP2 (hdc, oldRop);
    SelectObject (hdc, hOldPen);
}


static BOOL
HEADER_SendSimpleNotify (WND *wndPtr, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom   = wndPtr->wIDmenu;
    nmhdr.code     = code;

    return (BOOL)SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
				   (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}


static BOOL
HEADER_SendHeaderNotify (WND *wndPtr, UINT code, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);   
    NMHEADERA nmhdr;
    HDITEMA nmitem;

    nmhdr.hdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.hdr.idFrom = wndPtr->wIDmenu;
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

    return (BOOL)SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
				   (WPARAM)wndPtr->wIDmenu, (LPARAM)&nmhdr);
}


static BOOL
HEADER_SendClickNotify (WND *wndPtr, UINT code, INT iItem)
{
    NMHEADERA nmhdr;

    nmhdr.hdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.hdr.idFrom = wndPtr->wIDmenu;
    nmhdr.hdr.code = code;
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = NULL;

    return (BOOL)SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
				   (WPARAM)wndPtr->wIDmenu, (LPARAM)&nmhdr);
}


static LRESULT
HEADER_CreateDragImage (WND *wndPtr, WPARAM wParam)
{
    FIXME (header, "empty stub!\n");
    return 0;
}


static LRESULT
HEADER_DeleteItem (WND *wndPtr, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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

    HEADER_SetItemBounds (wndPtr);

    hdc = GetDC (wndPtr->hwndSelf);
    HEADER_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);
    
    return TRUE;
}


static LRESULT
HEADER_GetImageList (WND *wndPtr)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->himl;
}


static LRESULT
HEADER_GetItemA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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
HEADER_GetItemW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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


__inline__ static LRESULT
HEADER_GetItemCount (WND *wndPtr)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    return infoPtr->uNumItem;
}


static LRESULT
HEADER_GetItemRect (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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


__inline__ static LRESULT
HEADER_GetUnicodeFormat (WND *wndPtr)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    return infoPtr->bUnicode;
}


static LRESULT
HEADER_HitTest (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    LPHDHITTESTINFO phti = (LPHDHITTESTINFO)lParam;

    HEADER_InternalHitTest (wndPtr, &phti->pt, &phti->flags, &phti->iItem);

    return phti->flags;
}


static LRESULT
HEADER_InsertItemA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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

    HEADER_SetItemBounds (wndPtr);

    hdc = GetDC (wndPtr->hwndSelf);
    HEADER_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return nItem;
}


static LRESULT
HEADER_InsertItemW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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

    HEADER_SetItemBounds (wndPtr);

    hdc = GetDC (wndPtr->hwndSelf);
    HEADER_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return nItem;
}


static LRESULT
HEADER_Layout (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    LPHDLAYOUT lpLayout = (LPHDLAYOUT)lParam;

    lpLayout->pwpos->hwnd = wndPtr->hwndSelf;
    lpLayout->pwpos->hwndInsertAfter = 0;
    lpLayout->pwpos->x = lpLayout->prc->left;
    lpLayout->pwpos->y = lpLayout->prc->top;
    lpLayout->pwpos->cx = lpLayout->prc->right - lpLayout->prc->left;
    if (wndPtr->dwStyle & HDS_HIDDEN)
        lpLayout->pwpos->cy = 0;
    else
        lpLayout->pwpos->cy = infoPtr->nHeight;
    lpLayout->pwpos->flags = SWP_NOZORDER;

    TRACE (header, "Layout x=%d y=%d cx=%d cy=%d\n",
           lpLayout->pwpos->x, lpLayout->pwpos->y,
           lpLayout->pwpos->cx, lpLayout->pwpos->cy);

    HEADER_ForceItemBounds (wndPtr, lpLayout->pwpos->cy);

    /* hack */
#ifdef __HDM_LAYOUT_HACK__
    MoveWindow (lpLayout->pwpos->hwnd, lpLayout->pwpos->x, lpLayout->pwpos->y,
                  lpLayout->pwpos->cx, lpLayout->pwpos->cy, TRUE);
#endif

    return TRUE;
}


static LRESULT
HEADER_SetImageList (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HIMAGELIST himlOld;

    himlOld = infoPtr->himl;
    infoPtr->himl = (HIMAGELIST)lParam;

    /* FIXME: Refresh needed??? */

    return (LRESULT)himlOld;
}


static LRESULT
HEADER_SetItemA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HDITEMA *phdi = (HDITEMA*)lParam;
    INT nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC hdc;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGINGA, nItem))
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

    HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGEDA, nItem);

    HEADER_SetItemBounds (wndPtr);
    hdc = GetDC (wndPtr->hwndSelf);
    HEADER_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
HEADER_SetItemW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    HDITEMW *phdi = (HDITEMW*)lParam;
    INT nItem = (INT)wParam;
    HEADER_ITEM *lpItem;
    HDC hdc;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE (header, "[nItem=%d]\n", nItem);

    if (HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGINGA, nItem))
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

    HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGEDA, nItem);

    HEADER_SetItemBounds (wndPtr);
    hdc = GetDC (wndPtr->hwndSelf);
    HEADER_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << HEADER_SetOrderArray >> */


__inline__ static LRESULT
HEADER_SetUnicodeFormat (WND *wndPtr, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
HEADER_Create (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr;
    TEXTMETRICA tm;
    HFONT hOldFont;
    HDC   hdc;

    infoPtr = (HEADER_INFO *)COMCTL32_Alloc (sizeof(HEADER_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

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
    infoPtr->bUnicode = IsWindowUnicode (wndPtr->hwndSelf);

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, GetStockObject (SYSTEM_FONT));
    GetTextMetricsA (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    return 0;
}


static LRESULT
HEADER_Destroy (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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


static __inline__ LRESULT
HEADER_GetFont (WND *wndPtr)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
HEADER_LButtonDblClk (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    UINT  flags;
    INT   nItem;

    pt.x = (INT)LOWORD(lParam); 
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (wndPtr, &pt, &flags, &nItem);

    if ((wndPtr->dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER))
	HEADER_SendHeaderNotify (wndPtr, HDN_ITEMDBLCLICKA, nItem);
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN))
	HEADER_SendHeaderNotify (wndPtr, HDN_DIVIDERDBLCLICKA, nItem);

    return 0;
}


static LRESULT
HEADER_LButtonDown (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    POINT pt;
    UINT  flags;
    INT   nItem;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam); 
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (wndPtr, &pt, &flags, &nItem);

    if ((wndPtr->dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER)) {
	SetCapture (wndPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;   
	infoPtr->bPressed  = TRUE;
	infoPtr->iMoveItem = nItem;

	infoPtr->items[nItem].bDown = TRUE;

	/* Send WM_CUSTOMDRAW */
	hdc = GetDC (wndPtr->hwndSelf);
	HEADER_RefreshItem (wndPtr, hdc, nItem);
	ReleaseDC (wndPtr->hwndSelf, hdc);

	TRACE (header, "Pressed item %d!\n", nItem);
    } 
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN)) {
	if (!(HEADER_SendHeaderNotify (wndPtr, HDN_BEGINTRACKA, nItem))) {
	    SetCapture (wndPtr->hwndSelf);
	    infoPtr->bCaptured = TRUE;   
	    infoPtr->bTracking = TRUE;
	    infoPtr->iMoveItem = nItem;
	    infoPtr->nOldWidth = infoPtr->items[nItem].cxy;
	    infoPtr->xTrackOffset = infoPtr->items[nItem].rect.right - pt.x;

	    if (!(wndPtr->dwStyle & HDS_FULLDRAG)) {
		infoPtr->xOldTrack = infoPtr->items[nItem].rect.right;
		hdc = GetDC (wndPtr->hwndSelf);
		HEADER_DrawTrackLine (wndPtr, hdc, infoPtr->xOldTrack);
		ReleaseDC (wndPtr->hwndSelf, hdc);
	    }

	    TRACE (header, "Begin tracking item %d!\n", nItem);
	}
    }

    return 0;
}


static LRESULT
HEADER_LButtonUp (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (wndPtr, &pt, &flags, &nItem);

    if (infoPtr->bPressed) {
	if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER)) {
	    infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
	    hdc = GetDC (wndPtr->hwndSelf);
	    HEADER_RefreshItem (wndPtr, hdc, infoPtr->iMoveItem);
	    ReleaseDC (wndPtr->hwndSelf, hdc);

	    HEADER_SendClickNotify (wndPtr, HDN_ITEMCLICKA, infoPtr->iMoveItem);
	}
	TRACE (header, "Released item %d!\n", infoPtr->iMoveItem);
	infoPtr->bPressed = FALSE;
    }
    else if (infoPtr->bTracking) {
	TRACE (header, "End tracking item %d!\n", infoPtr->iMoveItem);
	infoPtr->bTracking = FALSE;

	HEADER_SendHeaderNotify (wndPtr, HDN_ENDTRACKA, infoPtr->iMoveItem);

	if (!(wndPtr->dwStyle & HDS_FULLDRAG)) {
	    hdc = GetDC (wndPtr->hwndSelf);
	    HEADER_DrawTrackLine (wndPtr, hdc, infoPtr->xOldTrack);
	    ReleaseDC (wndPtr->hwndSelf, hdc);
	    if (HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGINGA, infoPtr->iMoveItem))
		infoPtr->items[infoPtr->iMoveItem].cxy = infoPtr->nOldWidth;
	    else {
		nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		if (nWidth < 0)
		    nWidth = 0;
		infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
		HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGEDA, infoPtr->iMoveItem);
	    }

	    HEADER_SetItemBounds (wndPtr);
	    hdc = GetDC (wndPtr->hwndSelf);
	    HEADER_Refresh (wndPtr, hdc);
	    ReleaseDC (wndPtr->hwndSelf, hdc);
	}
    }

    if (infoPtr->bCaptured) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	HEADER_SendSimpleNotify (wndPtr, NM_RELEASEDCAPTURE);
    }

    return 0;
}


static LRESULT
HEADER_MouseMove (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (wndPtr, &pt, &flags, &nItem);

    if ((wndPtr->dwStyle & HDS_BUTTONS) && (wndPtr->dwStyle & HDS_HOTTRACK)) {
	if (flags & (HHT_ONHEADER | HHT_ONDIVIDER | HHT_ONDIVOPEN))
	    infoPtr->iHotItem = nItem;
	else
	    infoPtr->iHotItem = -1;
	hdc = GetDC (wndPtr->hwndSelf);
	HEADER_Refresh (wndPtr, hdc);
	ReleaseDC (wndPtr->hwndSelf, hdc);
    }

    if (infoPtr->bCaptured) {
	if (infoPtr->bPressed) {
	    if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER))
		infoPtr->items[infoPtr->iMoveItem].bDown = TRUE;
	    else
		infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
	    hdc = GetDC (wndPtr->hwndSelf);
	    HEADER_RefreshItem (wndPtr, hdc, infoPtr->iMoveItem);
	    ReleaseDC (wndPtr->hwndSelf, hdc);

	    TRACE (header, "Moving pressed item %d!\n", infoPtr->iMoveItem);
	}
	else if (infoPtr->bTracking) {
	    if (wndPtr->dwStyle & HDS_FULLDRAG) {
		if (HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGINGA, infoPtr->iMoveItem))
		    infoPtr->items[infoPtr->iMoveItem].cxy = infoPtr->nOldWidth;
		else {
		    nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		    if (nWidth < 0)
			nWidth = 0;
		    infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
		    HEADER_SendHeaderNotify (wndPtr, HDN_ITEMCHANGEDA,
					     infoPtr->iMoveItem);
		}
		HEADER_SetItemBounds (wndPtr);
		hdc = GetDC (wndPtr->hwndSelf);
		HEADER_Refresh (wndPtr, hdc);
		ReleaseDC (wndPtr->hwndSelf, hdc);
	    }
	    else {
		hdc = GetDC (wndPtr->hwndSelf);
		HEADER_DrawTrackLine (wndPtr, hdc, infoPtr->xOldTrack);
		infoPtr->xOldTrack = pt.x + infoPtr->xTrackOffset;
		if (infoPtr->xOldTrack < infoPtr->items[infoPtr->iMoveItem].rect.left)
		    infoPtr->xOldTrack = infoPtr->items[infoPtr->iMoveItem].rect.left;
		infoPtr->items[infoPtr->iMoveItem].cxy = 
		    infoPtr->xOldTrack - infoPtr->items[infoPtr->iMoveItem].rect.left;
		HEADER_DrawTrackLine (wndPtr, hdc, infoPtr->xOldTrack);
		ReleaseDC (wndPtr->hwndSelf, hdc);
	    }

	    HEADER_SendHeaderNotify (wndPtr, HDN_TRACKA, infoPtr->iMoveItem);
	    TRACE (header, "Tracking item %d!\n", infoPtr->iMoveItem);
	}
    }

    if ((wndPtr->dwStyle & HDS_BUTTONS) && (wndPtr->dwStyle & HDS_HOTTRACK)) {
	FIXME (header, "hot track support!\n");
    }

    return 0;
}


static LRESULT
HEADER_Paint (WND *wndPtr, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (wndPtr->hwndSelf, &ps) : (HDC)wParam;
    HEADER_Refresh (wndPtr, hdc);
    if(!wParam)
	EndPaint (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
HEADER_RButtonUp (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    return HEADER_SendSimpleNotify (wndPtr, NM_RCLICK);
}


static LRESULT
HEADER_SetCursor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
    POINT pt;
    UINT  flags;
    INT   nItem;

    TRACE (header, "code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (wndPtr->hwndSelf, &pt);

    HEADER_InternalHitTest (wndPtr, &pt, &flags, &nItem);

    if (flags == HHT_ONDIVIDER)
        SetCursor (infoPtr->hcurDivider);
    else if (flags == HHT_ONDIVOPEN)
        SetCursor (infoPtr->hcurDivopen);
    else
        SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
HEADER_SetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(wndPtr);
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
        HEADER_ForceItemBounds (wndPtr, infoPtr->nHeight);
        hdc = GetDC (wndPtr->hwndSelf);
        HEADER_Refresh (wndPtr, hdc);
        ReleaseDC (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


LRESULT WINAPI
HEADER_WindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (msg) {
	case HDM_CREATEDRAGIMAGE:
	    return HEADER_CreateDragImage (wndPtr, wParam);

	case HDM_DELETEITEM:
	    return HEADER_DeleteItem (wndPtr, wParam);

	case HDM_GETIMAGELIST:
	    return HEADER_GetImageList (wndPtr);

	case HDM_GETITEMA:
	    return HEADER_GetItemA (wndPtr, wParam, lParam);

	case HDM_GETITEMW:
	    return HEADER_GetItemW (wndPtr, wParam, lParam);

	case HDM_GETITEMCOUNT:
	    return HEADER_GetItemCount (wndPtr);

	case HDM_GETITEMRECT:
	    return HEADER_GetItemRect (wndPtr, wParam, lParam);

/*	case HDM_GETORDERARRAY: */

	case HDM_GETUNICODEFORMAT:
	    return HEADER_GetUnicodeFormat (wndPtr);

	case HDM_HITTEST:
	    return HEADER_HitTest (wndPtr, wParam, lParam);

	case HDM_INSERTITEMA:
	    return HEADER_InsertItemA (wndPtr, wParam, lParam);

	case HDM_INSERTITEMW:
	    return HEADER_InsertItemW (wndPtr, wParam, lParam);

	case HDM_LAYOUT:
	    return HEADER_Layout (wndPtr, wParam, lParam);

	case HDM_SETIMAGELIST:
	    return HEADER_SetImageList (wndPtr, wParam, lParam);

	case HDM_SETITEMA:
	    return HEADER_SetItemA (wndPtr, wParam, lParam);

	case HDM_SETITEMW:
	    return HEADER_SetItemW (wndPtr, wParam, lParam);

/*	case HDM_SETORDERARRAY: */

	case HDM_SETUNICODEFORMAT:
	    return HEADER_SetUnicodeFormat (wndPtr, wParam);


        case WM_CREATE:
            return HEADER_Create (wndPtr, wParam, lParam);

        case WM_DESTROY:
            return HEADER_Destroy (wndPtr, wParam, lParam);

        case WM_ERASEBKGND:
            return 1;

        case WM_GETDLGCODE:
            return DLGC_WANTTAB | DLGC_WANTARROWS;

        case WM_GETFONT:
            return HEADER_GetFont (wndPtr);

        case WM_LBUTTONDBLCLK:
            return HEADER_LButtonDblClk (wndPtr, wParam, lParam);

        case WM_LBUTTONDOWN:
            return HEADER_LButtonDown (wndPtr, wParam, lParam);

        case WM_LBUTTONUP:
            return HEADER_LButtonUp (wndPtr, wParam, lParam);

        case WM_MOUSEMOVE:
            return HEADER_MouseMove (wndPtr, wParam, lParam);

/*	case WM_NOTIFYFORMAT: */

        case WM_PAINT:
            return HEADER_Paint (wndPtr, wParam);

        case WM_RBUTTONUP:
            return HEADER_RButtonUp (wndPtr, wParam, lParam);

        case WM_SETCURSOR:
            return HEADER_SetCursor (wndPtr, wParam, lParam);

        case WM_SETFONT:
            return HEADER_SetFont (wndPtr, wParam, lParam);

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

