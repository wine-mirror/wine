/*
 * Listview control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Most messages.
 *   - Most notifications.
 */

#include "windows.h"
#include "commctrl.h"
#include "listview.h"
#include "win.h"
#include "debug.h"


#define LISTVIEW_GetInfoPtr(wndPtr) ((LISTVIEW_INFO *)wndPtr->wExtra[0])


static VOID
LISTVIEW_Refresh (WND *wndPtr, HDC32 hdc)
{
/*    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr); */



}



/* << LISTVIEW_ApproximateViewRect >> */
/* << LISTVIEW_Arrange >> */
/* << LISTVIEW_CreateDragImage >> */


static LRESULT
LISTVIEW_DeleteAllItems (WND *wndPtr)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    INT32 nItem;
    LISTVIEW_ITEM *lpItem;
    NMLISTVIEW nmlv;
    BOOL32 bNotify;

    if (infoPtr->nItemCount == 0)
	return TRUE;

    TRACE (listview, "\n");

    /* send LVN_DELETEALLITEMS notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code     = LVN_DELETEALLITEMS;
    nmlv.iItem        = -1;
    bNotify =
	!(BOOL32)SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
				 (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    nmlv.hdr.code     = LVN_DELETEITEM;

    for (nItem = 0; nItem < infoPtr->nItemCount; nItem++) {
	/* send notification */
	if (bNotify) {
	    nmlv.iItem = nItem;
	    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
			    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);
	}

	/* get item pointer */
	lpItem = (LISTVIEW_ITEM*)DPA_GetPtr (infoPtr->hdpaItems, nItem);
	if (lpItem) {
	    /* delete item strings */
	    if ((lpItem->pszText) && (lpItem->pszText != LPSTR_TEXTCALLBACK32A))
		COMCTL32_Free (lpItem->pszText);

	    /* free item data */
	    COMCTL32_Free (lpItem);
	}
    }

    DPA_DeleteAllPtrs (infoPtr->hdpaItems);
    infoPtr->nItemCount = 0;

    return TRUE;
}


static LRESULT
LISTVIEW_DeleteColumn (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    INT32 nColumn = (INT32)wParam;

    /* FIXME ??? */
    if (infoPtr->nItemCount > 0)
	return FALSE;

    if (!SendMessage32A (infoPtr->hwndHeader, HDM_DELETEITEM, wParam, 0))
	return FALSE;

    infoPtr->nColumnCount--;

    FIXME (listview, "semi stub!\n");

    return TRUE;
}


static LRESULT
LISTVIEW_DeleteItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    INT32 nItem = (INT32)wParam;
    LISTVIEW_ITEM *lpItem;
    NMLISTVIEW nmlv;

    if ((nItem < 0) || (nItem >= infoPtr->nItemCount))
	return FALSE;

    TRACE (listview, "(%d)\n", nItem);

    /* send notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code     = LVN_DELETEITEM;
    nmlv.iItem        = nItem;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    /* remove from item array */
    lpItem = (LISTVIEW_ITEM*)DPA_DeletePtr (infoPtr->hdpaItems, nItem);

    /* delete item strings */
    if ((lpItem->pszText) && (lpItem->pszText != LPSTR_TEXTCALLBACK32A))
	COMCTL32_Free (lpItem->pszText);

    /* free item data */
    COMCTL32_Free (lpItem);

    infoPtr->nItemCount--;

    return TRUE;
}


/* << LISTVIEW_EditLabel >> */
/* << LISTVIEW_EnsureVisible >> */
/* << LISTVIEW_FindItem >> */


static LRESULT
LISTVIEW_GetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->clrBk;
}


/* << LISTVIEW_GetBkImage >> */
/* << LISTVIEW_GetCallbackMask >> */


static LRESULT
LISTVIEW_GetColumn32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVCOLUMN32A lpcol = (LPLVCOLUMN32A)lParam;
    INT32 nIndex = (INT32)wParam;
    HDITEM32A hdi;

    if (!lpcol)
	return FALSE;

    TRACE (listview, "(%d %p)\n", nIndex, lpcol);

    ZeroMemory (&hdi, sizeof(HDITEM32A));

    if (lpcol->mask & LVCF_FMT)
	hdi.mask |= HDI_FORMAT;

    if (lpcol->mask & LVCF_WIDTH)
        hdi.mask |= HDI_WIDTH;

    if (lpcol->mask & LVCF_TEXT)
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);

    if (lpcol->mask & LVCF_IMAGE)
        hdi.mask |= HDI_IMAGE;

    if (lpcol->mask & LVCF_ORDER)
        hdi.mask |= HDI_ORDER;

    if (!SendMessage32A (infoPtr->hwndHeader, HDM_GETITEM32A,
		    wParam, (LPARAM)&hdi))
	return FALSE;

    if (lpcol->mask & LVCF_FMT) {
	lpcol->fmt = 0;

	if (hdi.fmt & HDF_LEFT)
	    lpcol->fmt |= LVCFMT_LEFT;
	else if (hdi.fmt & HDF_RIGHT)
	    lpcol->fmt |= LVCFMT_RIGHT;
	else if (hdi.fmt & HDF_CENTER)
	    lpcol->fmt |= LVCFMT_CENTER;

	if (hdi.fmt & HDF_IMAGE)
	    lpcol->fmt |= LVCFMT_COL_HAS_IMAGES;
    }

    if (lpcol->mask & LVCF_WIDTH)
	lpcol->cx = hdi.cxy;

    if ((lpcol->mask & LVCF_TEXT) && (lpcol->pszText) && (hdi.pszText))
	lstrcpyn32A (lpcol->pszText, hdi.pszText, lpcol->cchTextMax);

    if (lpcol->mask & LVCF_IMAGE)
	lpcol->iImage = hdi.iImage;

    if (lpcol->mask & LVCF_ORDER)
	lpcol->iOrder = hdi.iOrder;

    return TRUE;
}


/* << LISTVIEW_GetColumn32W >> */
/* << LISTVIEW_GetColumnOrderArray >> */


__inline__ static LRESULT
LISTVIEW_GetColumnWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    HDITEM32A hdi;

    hdi.mask = HDI_WIDTH;
    if (SendMessage32A (infoPtr->hwndHeader, HDM_GETITEM32A,
			wParam, (LPARAM)&hdi))
	return hdi.cxy;

    return 0;
}


/* << LISTVIEW_GetCountPerPage >> */
/* << LISTVIEW_GetEditControl >> */
/* << LISTVIEW_GetExtendedListviewStyle >> */


__inline__ static LRESULT
LISTVIEW_GetHeader (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->hwndHeader;
}


/* << LISTVIEW_GetHotCursor >> */
/* << LISTVIEW_GetHotItem >> */
/* << LISTVIEW_GetHoverTime >> */


static LRESULT
LISTVIEW_GetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    TRACE (listview, "(0x%08x)\n", wParam);

    switch (wParam) {
	case LVSIL_NORMAL:
	    return (LRESULT)infoPtr->himlNormal;

	case LVSIL_SMALL:
	    return (LRESULT)infoPtr->himlSmall;

	case LVSIL_STATE:
	    return (LRESULT)infoPtr->himlState;
    }

    return (LRESULT)NULL;
}


/* << LISTVIEW_GetISearchString >> */


static LRESULT
LISTVIEW_GetItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVITEM32A lpItem = (LPLVITEM32A)lParam;
    LISTVIEW_ITEM *lpRow, *lpSubItem;

    if (!lpItem)
	return FALSE;

    if ((lpItem->iItem < 0) || (lpItem->iItem >= infoPtr->nItemCount))
	return FALSE;

    if ((lpItem->iSubItem < 0) || (lpItem->iSubItem >= infoPtr->nColumnCount))
	return FALSE;

    FIXME (listview, "(%d %d %p)\n",
	   lpItem->iItem, lpItem->iSubItem, lpItem);

    lpRow = DPA_GetPtr (infoPtr->hdpaItems, lpItem->iItem);
    if (!lpRow)
	return FALSE;

    lpSubItem = &lpRow[lpItem->iSubItem];
    if (!lpSubItem)
	return FALSE;

    if (lpItem->mask & LVIF_STATE)
	lpItem->state = lpSubItem->state & lpItem->stateMask;

    if (lpItem->mask & LVIF_TEXT) {
	if (lpSubItem->pszText == LPSTR_TEXTCALLBACK32A)
	    lpItem->pszText = LPSTR_TEXTCALLBACK32A;
	else
	    Str_GetPtr32A (lpSubItem->pszText, lpItem->pszText,
			   lpItem->cchTextMax);
    }

    if (lpItem->mask & LVIF_IMAGE)
	 lpItem->iImage = lpSubItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
	lpItem->lParam = lpSubItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
	lpItem->iIndent = lpSubItem->iIndent;

    return TRUE;
}


/* << LISTVIEW_GetItem32W >> */


__inline__ static LRESULT
LISTVIEW_GetItemCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    return infoPtr->nItemCount;
}


static LRESULT
LISTVIEW_GetItemPosition (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPPOINT32 lpPt = (LPPOINT32)lParam;
    INT32 nIndex = (INT32)wParam;

    if (!lpPt)
	return FALSE;
    if ((nIndex < 0) || (nIndex >= infoPtr->nItemCount))
	return FALSE;

    FIXME (listview, "returning position [0,0]!\n");
    lpPt->x = 0;
    lpPt->y = 0;

    return TRUE;
}


/* << LISTVIEW_GetItemRect >> */
/* << LISTVIEW_GetItemSpacing >> */
/* << LISTVIEW_GetItemState >> */


static LRESULT
LISTVIEW_GetItemText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVITEM32A lpItem = (LPLVITEM32A)lParam;
    INT32 nItem = (INT32)wParam;
    LISTVIEW_ITEM *lpRow, *lpSubItem;

    TRACE (listview, "(%d %p)\n", nItem, lpItem);

    lpRow = DPA_GetPtr (infoPtr->hdpaItems, lpItem->iItem);
    if (!lpRow)
	return 0;

    lpSubItem = &lpRow[lpItem->iSubItem];
    if (!lpSubItem)
	return 0;

    if (lpSubItem->pszText == LPSTR_TEXTCALLBACK32A) {
	lpItem->pszText = LPSTR_TEXTCALLBACK32A;
	return 0;
    }
    else
	return Str_GetPtr32A (lpSubItem->pszText, lpItem->pszText,
			      lpItem->cchTextMax);
}


/* << LISTVIEW_GetItemText32A >> */


static LRESULT
LISTVIEW_GetNextItem (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    INT32 nStart = (INT32)wParam;
    UINT32 uFlags = (UINT32)LOWORD(lParam);

    FIXME (listview, "(%d, 0x%x): semi stub!\n", nStart, uFlags);

    if (infoPtr->nItemCount <= 0)
	return -1;

    /* just a simple (preliminary) hack */
    if (nStart == -1)
	return 0;
    else if (nStart < infoPtr->nItemCount - 1)
	return nStart + 1;
    else
	return -1;

    return -1;
}


/* << LISTVIEW_GetNumberOfWorkAreas >> */
/* << LISTVIEW_GetOrigin >> */


static LRESULT
LISTVIEW_GetSelectedCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    TRACE (listview, ": empty stub (returns 0)!\n");

    return 0;
}


/* << LISTVIEW_GetSelectionMark >> */


static LRESULT
LISTVIEW_GetStringWidth32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPSTR lpsz = (LPSTR)lParam;
    HFONT32 hFont, hOldFont;
    HDC32 hdc;
    SIZE32 size;

    if (!lpsz)
	return 0;

    TRACE (listview, "(%s)\n", lpsz);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (SYSTEM_FONT);
    hdc = GetDC32 (0);
    hOldFont = SelectObject32 (hdc, hFont);
    GetTextExtentPoint32A (hdc, lpsz, lstrlen32A(lpsz), &size);
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (0, hdc);

    TRACE (listview, "-- ret=%d\n", size.cx);

    return (LRESULT)size.cx;
}




__inline__ static LRESULT
LISTVIEW_GetTextBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->clrTextBk;
}


__inline__ static LRESULT
LISTVIEW_GetTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->clrText;
}


static LRESULT
LISTVIEW_HitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr); */
    LPLVHITTESTINFO lpht = (LPLVHITTESTINFO)lParam;

    FIXME (listview, "(%p): stub!\n", lpht);

    /* FIXME: preliminary */
    lpht->flags = LVHT_NOWHERE;
    lpht->iItem = -1;
    lpht->iSubItem = 0;

    return -1;
}


static LRESULT
LISTVIEW_InsertColumn32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVCOLUMN32A lpcol = (LPLVCOLUMN32A)lParam;
    INT32 nIndex = (INT32)wParam;
    HDITEM32A hdi;
    INT32 nResult;

    if ((!lpcol) || (infoPtr->nItemCount > 0))
	return -1;

    FIXME (listview, "(%d %p): semi stub!\n", nIndex, lpcol);

    ZeroMemory (&hdi, sizeof(HDITEM32A));

    if (lpcol->mask & LVCF_FMT) {
	if (nIndex == 0)
	    hdi.fmt |= HDF_LEFT;
	else if (lpcol->fmt & LVCFMT_LEFT)
	    hdi.fmt |= HDF_LEFT;
	else if (lpcol->fmt & LVCFMT_RIGHT)
	    hdi.fmt |= HDF_RIGHT;
	else if (lpcol->fmt & LVCFMT_CENTER)
	    hdi.fmt |= HDF_CENTER;

	if (lpcol->fmt & LVCFMT_COL_HAS_IMAGES)
	    hdi.fmt |= HDF_IMAGE;
	    
	hdi.mask |= HDI_FORMAT;
    }

    if (lpcol->mask & LVCF_WIDTH) {
        hdi.mask |= HDI_WIDTH;
	hdi.cxy = lpcol->cx;
    }
    
    if (lpcol->mask & LVCF_TEXT) {
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);
	hdi.pszText = lpcol->pszText;
	hdi.fmt |= HDF_STRING;
    }

    if (lpcol->mask & LVCF_IMAGE) {
        hdi.mask |= HDI_IMAGE;
	hdi.iImage = lpcol->iImage;
    }

    if (lpcol->mask & LVCF_ORDER) {
        hdi.mask |= HDI_ORDER;
	hdi.iOrder = lpcol->iOrder;
    }

    nResult = SendMessage32A (infoPtr->hwndHeader, HDM_INSERTITEM32A,
			      wParam, (LPARAM)&hdi);
    if (nResult == -1)
	return -1;

    infoPtr->nColumnCount++;

    return nResult;
}


/* << LISTVIEW_InsertColumn32W >> */


static LRESULT
LISTVIEW_InsertItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVITEM32A lpItem = (LPLVITEM32A)lParam;
    LISTVIEW_ITEM *lpListItem;
    INT32 nIndex;
    NMLISTVIEW nmlv;

    if (!lpItem)
	return -1;

    if ((!infoPtr->nColumnCount) || (lpItem->iSubItem))
	return -1;

    FIXME (listview, "(%d %p)\n", lpItem->iItem, lpItem);
    FIXME (listview, "(%p %p)\n", infoPtr, infoPtr->hdpaItems);

    lpListItem = (LISTVIEW_ITEM*)COMCTL32_Alloc (infoPtr->nColumnCount * sizeof(LISTVIEW_ITEM));
    nIndex = DPA_InsertPtr (infoPtr->hdpaItems, lpItem->iItem, lpListItem);
    if (nIndex == -1)
	return -1;

    if (lpItem->mask & LVIF_STATE)
	lpListItem[0].state = lpItem->state;

    if (lpItem->mask & LVIF_TEXT) {
	if (lpItem->pszText == LPSTR_TEXTCALLBACK32A)
	    lpListItem[0].pszText = LPSTR_TEXTCALLBACK32A;
	else
	    Str_SetPtr32A (&lpListItem[0].pszText, lpItem->pszText);
    }

    if (lpItem->mask & LVIF_IMAGE)
	lpListItem[0].iImage = lpItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
	lpListItem[0].lParam = lpItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
	lpListItem[0].iIndent = lpItem->iIndent;

    infoPtr->nItemCount++;

    /* send notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code     = LVN_INSERTITEM;
    nmlv.iItem        = nIndex;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    return nIndex;
}


/* << LISTVIEW_InsertItem32W >> */


static LRESULT
LISTVIEW_RedrawItems (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr); */

    FIXME (listview, "(%d - %d): empty stub!\n",
	   (INT32)wParam, (INT32)lParam);

    return TRUE;
}



static LRESULT
LISTVIEW_SetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (!infoPtr)
	return FALSE;

    /* set background color */
    TRACE (listview, "0x%06lx\n", (COLORREF)lParam);
    infoPtr->clrBk = (COLORREF)lParam;

    return TRUE;
}


static LRESULT
LISTVIEW_SetColumn32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVCOLUMN32A lpcol = (LPLVCOLUMN32A)lParam;
    INT32 nIndex = (INT32)wParam;
    HDITEM32A hdi;

    if (!lpcol)
	return -1;

    FIXME (listview, "(%d %p): semi stub!\n", nIndex, lpcol);

    ZeroMemory (&hdi, sizeof(HDITEM32A));

    if (lpcol->mask & LVCF_FMT) {
	if (nIndex == 0)
	    hdi.fmt |= HDF_LEFT;
	else if (lpcol->fmt & LVCFMT_LEFT)
	    hdi.fmt |= HDF_LEFT;
	else if (lpcol->fmt & LVCFMT_RIGHT)
	    hdi.fmt |= HDF_RIGHT;
	else if (lpcol->fmt & LVCFMT_CENTER)
	    hdi.fmt |= HDF_CENTER;

	if (lpcol->fmt & LVCFMT_COL_HAS_IMAGES)
	    hdi.fmt |= HDF_IMAGE;
	    
	hdi.mask |= HDI_FORMAT;
    }

    if (lpcol->mask & LVCF_WIDTH) {
        hdi.mask |= HDI_WIDTH;
	hdi.cxy = lpcol->cx;
    }
    
    if (lpcol->mask & LVCF_TEXT) {
        hdi.mask |= (HDI_TEXT | HDI_FORMAT);
	hdi.pszText = lpcol->pszText;
	hdi.fmt |= HDF_STRING;
    }

    if (lpcol->mask & LVCF_IMAGE) {
        hdi.mask |= HDI_IMAGE;
	hdi.iImage = lpcol->iImage;
    }

    if (lpcol->mask & LVCF_ORDER) {
        hdi.mask |= HDI_ORDER;
	hdi.iOrder = lpcol->iOrder;
    }

    return (LRESULT)SendMessage32A (infoPtr->hwndHeader, HDM_SETITEM32A,
				    wParam, (LPARAM)&hdi);
}




static LRESULT
LISTVIEW_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp = 0;

    TRACE (listview, "(0x%08x 0x%08lx)\n", wParam, lParam);

    switch (wParam) {
	case LVSIL_NORMAL:
	    himlTemp = infoPtr->himlNormal;
	    infoPtr->himlNormal = (HIMAGELIST)lParam;
	    return (LRESULT)himlTemp;

	case LVSIL_SMALL:
	    himlTemp = infoPtr->himlSmall;
	    infoPtr->himlSmall = (HIMAGELIST)lParam;
	    return (LRESULT)himlTemp;

	case LVSIL_STATE:
	    himlTemp = infoPtr->himlState;
	    infoPtr->himlState = (HIMAGELIST)lParam;
	    return (LRESULT)himlTemp;
    }

    return (LRESULT)NULL;
}



static LRESULT
LISTVIEW_SetItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVITEM32A lpItem = (LPLVITEM32A)lParam;
    LISTVIEW_ITEM *lpRow, *lpSubItem;
    NMLISTVIEW nmlv;

    if (!lpItem)
	return FALSE;

    if ((lpItem->iItem < 0) || (lpItem->iItem >= infoPtr->nItemCount))
	return FALSE;

    if ((lpItem->iSubItem < 0) || (lpItem->iSubItem >= infoPtr->nColumnCount))
	return FALSE;

    /* send LVN_ITEMCHANGING notification */
    ZeroMemory (&nmlv, sizeof (NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code     = LVN_ITEMCHANGING;
    nmlv.iItem        = lpItem->iItem;
    nmlv.iSubItem     = lpItem->iSubItem;
    nmlv.uChanged     = lpItem->mask;

    if (!SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
			 (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv))
	return FALSE;

    TRACE (listview, "(%d %d %p)\n",
	   lpItem->iItem, lpItem->iSubItem, lpItem);

    lpRow = DPA_GetPtr (infoPtr->hdpaItems, lpItem->iItem);
    if (!lpRow)
	return FALSE;

    lpSubItem = &lpRow[lpItem->iSubItem];
    if (!lpSubItem)
	return FALSE;

    if (lpItem->mask & LVIF_STATE)
	lpSubItem->state = (lpSubItem->state & lpItem->stateMask) | lpItem->state;

    if (lpItem->mask & LVIF_TEXT) {
	if (lpItem->pszText == LPSTR_TEXTCALLBACK32A) {
	    if ((lpSubItem->pszText) &&
		(lpSubItem->pszText != LPSTR_TEXTCALLBACK32A))
		COMCTL32_Free (lpSubItem->pszText);
	    lpSubItem->pszText = LPSTR_TEXTCALLBACK32A;
	}
	else {
	    if (lpSubItem->pszText == LPSTR_TEXTCALLBACK32A)
		lpSubItem->pszText = NULL;
	    Str_SetPtr32A (&lpSubItem->pszText, lpItem->pszText);
	}
    }

    if (lpItem->mask & LVIF_IMAGE)
	lpSubItem->iImage = lpItem->iImage;

    if (lpItem->mask & LVIF_PARAM)
	lpSubItem->lParam = lpItem->lParam;

    if (lpItem->mask & LVIF_INDENT)
	lpSubItem->iIndent = lpItem->iIndent;

    /* send LVN_ITEMCHANGED notification */
    nmlv.hdr.code = LVN_ITEMCHANGED;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    return TRUE;
}


/* << LISTVIEW_SetItem32W >> */
/* << LISTVIEW_SetItemCount >> */


static LRESULT
LISTVIEW_SetItemPosition (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    INT32 nIndex = (INT32)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nItemCount))
	return FALSE;

    FIXME (listview, "setting position [%d, %d]!\n",
	   (INT32)LOWORD(lParam), (INT32)HIWORD(lParam));

    /* FIXME: set position */

    return TRUE;
}


static LRESULT
LISTVIEW_SetTextBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (!infoPtr)
	return FALSE;

    /* set text background color */
    TRACE (listview, "0x%06lx\n", (COLORREF)lParam);
    infoPtr->clrTextBk = (COLORREF)lParam;

    return TRUE;
}


static LRESULT
LISTVIEW_SetTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (!infoPtr)
	return FALSE;

    /* set text color */
    TRACE (listview, "0x%06lx\n", (COLORREF)lParam);
    infoPtr->clrText = (COLORREF)lParam;

    return TRUE;
}



static LRESULT
LISTVIEW_SortItems (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr); */

    FIXME (listview, "empty stub!\n");

    /* fake success */
    return TRUE;
}




static LRESULT
LISTVIEW_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    /* info structure is created at NCCreate */
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LOGFONT32A logFont;
    DWORD dwStyle = WS_CHILD | WS_VISIBLE; 

    TRACE (listview, "styles 0x%08lx 0x%08lx\n",
	   wndPtr->dwStyle, wndPtr->dwExStyle);

    /* initialize info structure */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = RGB(0, 0, 0); /* preliminary */
    infoPtr->clrTextBk = RGB(255, 255, 255); /* preliminary */

    if (!(wndPtr->dwStyle & LVS_REPORT) ||
	 (wndPtr->dwStyle & LVS_NOCOLUMNHEADER))
	dwStyle |= HDS_HIDDEN;
    if (!(wndPtr->dwStyle & LVS_NOSORTHEADER))
	dwStyle |= HDS_BUTTONS;

    /* create header */
    infoPtr->hwndHeader =
	CreateWindow32A (WC_HEADER32A, "", dwStyle,
			 0, 0, 0, 0, wndPtr->hwndSelf,
			 (HMENU32)0, wndPtr->hInstance, NULL);

    /* get default font (icon title) */
    SystemParametersInfo32A (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hDefaultFont = CreateFontIndirect32A (&logFont);
    infoPtr->hFont = infoPtr->hDefaultFont;

    /* set header font */
    SendMessage32A (infoPtr->hwndHeader, WM_SETFONT,
		    (WPARAM32)infoPtr->hFont, (LPARAM)TRUE);

    infoPtr->hdpaItems = DPA_Create (10);

    return 0;
}


static LRESULT
LISTVIEW_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    /* delete all items */
    LISTVIEW_DeleteAllItems (wndPtr);

    /* destroy dpa */
    DPA_Destroy (infoPtr->hdpaItems);

    /* destroy header */
    if (infoPtr->hwndHeader)
	DestroyWindow32 (infoPtr->hwndHeader);

    /* destroy font */
    infoPtr->hFont = (HFONT32)0;
    if (infoPtr->hDefaultFont)
	DeleteObject32 (infoPtr->hDefaultFont);

    

    return 0;
}


static LRESULT
LISTVIEW_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (infoPtr->clrBk == CLR_NONE) {
	return SendMessage32A (GetParent32 (wndPtr->hwndSelf),
			       WM_ERASEBKGND, wParam, lParam);
    }
    else {
	HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
	FillRect32 ((HDC32)wParam, &infoPtr->rcList, hBrush);
	DeleteObject32 (hBrush);
	return FALSE;
    }

    return TRUE;
}


__inline__ static LRESULT
LISTVIEW_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


/* << LISTVIEW_HScroll >> */
/* << LISTVIEW_KeyDown >> */


static LRESULT
LISTVIEW_KillFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMHDR nmh;

    FIXME (listview, "semi stub!\n");

    nmh.hwndFrom = wndPtr->hwndSelf;
    nmh.idFrom   = wndPtr->wIDmenu;
    nmh.code = NM_KILLFOCUS;

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmh);

    infoPtr->bFocus = FALSE;

    return 0;
}


static LRESULT
LISTVIEW_LButtonDblClk (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMLISTVIEW nmlv;

    FIXME (listview, "semi stub!\n");

    ZeroMemory (&nmlv, sizeof(NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code = NM_DBLCLK;
    nmlv.iItem    = -1;
    nmlv.iSubItem = 0;
    nmlv.ptAction.x = (INT32)LOWORD(lParam);
    nmlv.ptAction.y = (INT32)HIWORD(lParam);

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    return 0;
}


static LRESULT
LISTVIEW_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMLISTVIEW nmlv;

    FIXME (listview, "semi stub!\n");

    ZeroMemory (&nmlv, sizeof(NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code = NM_CLICK;
    nmlv.iItem    = -1;
    nmlv.iSubItem = 0;
    nmlv.ptAction.x = (INT32)LOWORD(lParam);
    nmlv.ptAction.y = (INT32)HIWORD(lParam);

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    if (!infoPtr->bFocus)
	SetFocus32 (wndPtr->hwndSelf);

    return 0;
}


static LRESULT
LISTVIEW_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (LISTVIEW_INFO *)COMCTL32_Alloc (sizeof(LISTVIEW_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (listview, "could not allocate info memory!\n");
	return 0;
    }

    if ((LISTVIEW_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (listview, "pointer assignment error!\n");
	return 0;
    }

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}


static LRESULT
LISTVIEW_NCDestroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);




    /* free list view info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
LISTVIEW_Notify (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    if (lpnmh->hwndFrom == infoPtr->hwndHeader) {

	FIXME (listview, "WM_NOTIFY from header!\n");
    }
    else {

	FIXME (listview, "WM_NOTIFY from unknown source!\n");
    }

    return 0;
}


static LRESULT
LISTVIEW_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    LISTVIEW_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
LISTVIEW_RButtonDblClk (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMLISTVIEW nmlv;

    FIXME (listview, "semi stub!\n");

    ZeroMemory (&nmlv, sizeof(NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code = NM_RDBLCLK;
    nmlv.iItem    = -1;
    nmlv.iSubItem = 0;
    nmlv.ptAction.x = (INT32)LOWORD(lParam);
    nmlv.ptAction.y = (INT32)HIWORD(lParam);

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    return 0;
}


static LRESULT
LISTVIEW_RButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMLISTVIEW nmlv;

    FIXME (listview, "semi stub!\n");

    ZeroMemory (&nmlv, sizeof(NMLISTVIEW));
    nmlv.hdr.hwndFrom = wndPtr->hwndSelf;
    nmlv.hdr.idFrom   = wndPtr->wIDmenu;
    nmlv.hdr.code = NM_RCLICK;
    nmlv.iItem    = -1;
    nmlv.iSubItem = 0;
    nmlv.ptAction.x = (INT32)LOWORD(lParam);
    nmlv.ptAction.y = (INT32)HIWORD(lParam);

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmlv);

    return 0;
}


static LRESULT
LISTVIEW_SetFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    NMHDR nmh;

    FIXME (listview, "semi stub!\n");

    nmh.hwndFrom = wndPtr->hwndSelf;
    nmh.idFrom   = wndPtr->wIDmenu;
    nmh.code = NM_SETFOCUS;

    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmh);

    infoPtr->bFocus = TRUE;

    return 0;
}


static LRESULT
LISTVIEW_SetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    HFONT32 hFont = (HFONT32)wParam;

    infoPtr->hFont = hFont ? hFont : infoPtr->hDefaultFont;

    /* set header font */
    SendMessage32A (infoPtr->hwndHeader, WM_SETFONT, wParam, lParam);

    /* reinitialize the listview */
    


    if (lParam) {
	/* force redraw */


    }

    return 0;
}




static LRESULT
LISTVIEW_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    GetClientRect32 (wndPtr->hwndSelf, &infoPtr->rcList);

    if (wndPtr->dwStyle & LVS_REPORT) {
	HDLAYOUT hl;
	WINDOWPOS32 wp;
	RECT32 rc;

	rc.top = 0;
	rc.left = 0;
	rc.right = LOWORD(lParam);
	rc.bottom = HIWORD(lParam);

	hl.prc = &rc;
	hl.pwpos = &wp;
	SendMessage32A (infoPtr->hwndHeader, HDM_LAYOUT, 0, (LPARAM)&hl);

	SetWindowPos32 (infoPtr->hwndHeader, wndPtr->hwndSelf,
			wp.x, wp.y, wp.cx, wp.cy, wp.flags);

	infoPtr->rcList.top += wp.cy;
    }

    return 0;
}


LRESULT WINAPI
LISTVIEW_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
/*	case LVM_APPROXIMATEVIEWRECT: */
/*	case LVM_ARRANGE: */
/*	case LVM_CREATEDRAGIMAGE: */

	case LVM_DELETEALLITEMS:
	    return LISTVIEW_DeleteAllItems (wndPtr);

	case LVM_DELETECOLUMN:
	    return LISTVIEW_DeleteColumn (wndPtr, wParam, lParam);

	case LVM_DELETEITEM:
	    return LISTVIEW_DeleteItem (wndPtr, wParam, lParam);

/*	case LVM_EDITLABEL: */
/*	case LVM_ENSUREVISIBLE: */
/*	case LVM_FINDITEM: */

	case LVM_GETBKCOLOR:
	    return LISTVIEW_GetBkColor (wndPtr, wParam, lParam);

/*	case LVM_GETBKIMAGE: */
/*	case LVM_GETCALLBACKMASK: */

	case LVM_GETCOLUMN32A:
	    return LISTVIEW_GetColumn32A (wndPtr, wParam, lParam);

/*	case LVM_GETCOLUMN32W: */
/*	case LVM_GETCOLUMNORDERARRAY: */

	case LVM_GETCOLUMNWIDTH:
	    return LISTVIEW_GetColumnWidth (wndPtr, wParam, lParam);

/*	case LVM_GETCOUNTPERPAGE: */
/*	case LVM_GETEDITCONTROL: */
/*	case LVM_GETEXTENDEDLISTVIEWSTYLE: */

	case LVM_GETHEADER:
	    return LISTVIEW_GetHeader (wndPtr, wParam, lParam);

/*	case LVM_GETHOTCURSOR: */
/*	case LVM_GETHOTITEM: */
/*	case LVM_GETHOVERTIME: */

	case LVM_GETIMAGELIST:
	    return LISTVIEW_GetImageList (wndPtr, wParam, lParam);

/*	case LVM_GETISEARCHSTRING: */

	case LVM_GETITEM32A:
	    return LISTVIEW_GetItem32A (wndPtr, wParam, lParam);

/*	case LVM_GETITEM32W: */

	case LVM_GETITEMCOUNT:
	    return LISTVIEW_GetItemCount (wndPtr, wParam, lParam);

	case LVM_GETITEMPOSITION:
	    return LISTVIEW_GetItemPosition (wndPtr, wParam, lParam);

/*	case LVM_GETITEMRECT: */
/*	case LVM_GETITEMSPACING: */
/*	case LVM_GETITEMSTATE: */

	case LVM_GETITEMTEXT32A:
	    return LISTVIEW_GetItemText32A (wndPtr, wParam, lParam);

/*	case LVM_GETITEMTEXT32W: */

	case LVM_GETNEXTITEM:
	    return LISTVIEW_GetNextItem (wndPtr, wParam, lParam);

/*	case LVM_GETNUMBEROFWORKAREAS: */
/*	case LVM_GETORIGIN: */

	case LVM_GETSELECTEDCOUNT:
	    return LISTVIEW_GetSelectedCount (wndPtr, wParam, lParam);

/*	case LVM_GETSELECTIONMARK: */

	case LVM_GETSTRINGWIDTH32A:
	    return LISTVIEW_GetStringWidth32A (wndPtr, wParam, lParam);

/*	case LVM_GETSTRINGWIDTH32W: */
/*	case LVM_GETSUBITEMRECT: */

	case LVM_GETTEXTBKCOLOR:
	    return LISTVIEW_GetTextBkColor (wndPtr, wParam, lParam);

	case LVM_GETTEXTCOLOR:
	    return LISTVIEW_GetTextColor (wndPtr, wParam, lParam);

/*	case LVM_GETTOOLTIPS: */
/*	case LVM_GETTOPINDEX: */
/*	case LVM_GETUNICODEFORMAT: */
/*	case LVM_GETVIEWRECT: */
/*	case LVM_GETWORKAREAS: */

	case LVM_HITTEST:
	    return LISTVIEW_HitTest (wndPtr, wParam, lParam);

	case LVM_INSERTCOLUMN32A:
	    return LISTVIEW_InsertColumn32A (wndPtr, wParam, lParam);

/*	case LVM_INSERTCOLUMN32W: */

	case LVM_INSERTITEM32A:
	    return LISTVIEW_InsertItem32A (wndPtr, wParam, lParam);

/*	case LVM_INSERTITEM32W: */

	case LVM_REDRAWITEMS:
	    return LISTVIEW_RedrawItems (wndPtr, wParam, lParam);

/*	case LVM_SCROLL: */

	case LVM_SETBKCOLOR:
	    return LISTVIEW_SetBkColor (wndPtr, wParam, lParam);

/*	case LVM_SETBKIMAGE: */
/*	case LVM_SETCALLBACKMASK: */

	case LVM_SETCOLUMN32A:
	    return LISTVIEW_SetColumn32A (wndPtr, wParam, lParam);

/*	case LVM_SETCOLUMN32W: */
/*	case LVM_SETCOLUMNORDERARRAY: */
/*	case LVM_SETCOLUMNWIDTH: */
/*	case LVM_SETEXTENDEDLISTVIEWSTYLE: */
/*	case LVM_SETHOTCURSOR: */
/*	case LVM_SETHOTITEM: */
/*	case LVM_SETHOVERTIME: */
/*	case LVM_SETICONSPACING: */
	
	case LVM_SETIMAGELIST:
	    return LISTVIEW_SetImageList (wndPtr, wParam, lParam);

	case LVM_SETITEM32A:
	    return LISTVIEW_SetItem32A (wndPtr, wParam, lParam);

/*	case LVM_SETITEM32W: */
/*	case LVM_SETITEMCOUNT: */

	case LVM_SETITEMPOSITION:
	    return LISTVIEW_SetItemPosition (wndPtr, wParam, lParam);

/*	case LVM_SETITEMPOSITION32: */
/*	case LVM_SETITEMSTATE: */
/*	case LVM_SETITEMTEXT: */
/*	case LVM_SETSELECTIONMARK: */

	case LVM_SETTEXTBKCOLOR:
	    return LISTVIEW_SetTextBkColor (wndPtr, wParam, lParam);

	case LVM_SETTEXTCOLOR:
	    return LISTVIEW_SetTextColor (wndPtr, wParam, lParam);

/*	case LVM_SETTOOLTIPS: */
/*	case LVM_SETUNICODEFORMAT: */
/*	case LVM_SETWORKAREAS: */

	case LVM_SORTITEMS:
	    return LISTVIEW_SortItems (wndPtr, wParam, lParam);

/*	case LVM_SUBITEMHITTEST: */
/*	case LVM_UPDATE: */



/*	case WM_CHAR: */
/*	case WM_COMMAND: */

	case WM_CREATE:
	    return LISTVIEW_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return LISTVIEW_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return LISTVIEW_EraseBackground (wndPtr, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTTAB | DLGC_WANTARROWS;

	case WM_GETFONT:
	    return LISTVIEW_GetFont (wndPtr, wParam, lParam);

/*	case WM_HSCROLL: */
/*	case WM_KEYDOWN: */

	case WM_KILLFOCUS:
	    return LISTVIEW_KillFocus (wndPtr, wParam, lParam);

	case WM_LBUTTONDBLCLK:
	    return LISTVIEW_LButtonDblClk (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return LISTVIEW_LButtonDown (wndPtr, wParam, lParam);

/*	case WM_MOUSEMOVE: */
/*	    return LISTVIEW_MouseMove (wndPtr, wParam, lParam); */

	case WM_NCCREATE:
	    return LISTVIEW_NCCreate (wndPtr, wParam, lParam);

	case WM_NCDESTROY:
	    return LISTVIEW_NCDestroy (wndPtr, wParam, lParam);

	case WM_NOTIFY:
	    return LISTVIEW_Notify (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return LISTVIEW_Paint (wndPtr, wParam);

	case WM_RBUTTONDBLCLK:
	    return LISTVIEW_RButtonDblClk (wndPtr, wParam, lParam);

	case WM_RBUTTONDOWN:
	    return LISTVIEW_RButtonDown (wndPtr, wParam, lParam);

	case WM_SETFOCUS:
	    return LISTVIEW_SetFocus (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return LISTVIEW_SetFont (wndPtr, wParam, lParam);

/*	case WM_SETREDRAW: */

	case WM_SIZE:
	    return LISTVIEW_Size (wndPtr, wParam, lParam);

/*	case WM_TIMER: */
/*	case WM_VSCROLL: */
/*	case WM_WINDOWPOSCHANGED: */
/*	case WM_WININICHANGE: */

	default:
	    if (uMsg >= WM_USER)
		ERR (listview, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
LISTVIEW_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (WC_LISTVIEW32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)LISTVIEW_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(LISTVIEW_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_LISTVIEW32A;
 
    RegisterClass32A (&wndClass);
}


VOID
LISTVIEW_Unregister (VOID)
{
    if (GlobalFindAtom32A (WC_LISTVIEW32A))
	UnregisterClass32A (WC_LISTVIEW32A, (HINSTANCE32)NULL);
}

