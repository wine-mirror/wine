/*
 * Toolbar control
 *
 * Copyright 1998 Eric Kohl
 *
 * TODO:
 *   - A little bug in TOOLBAR_DrawMasked()
 *   - Button wrapping (under construction).
 *   - Messages.
 *   - Notifications.
 *   - Fix TB_SETROWS.
 *   - Tooltip support (almost complete).
 *   - Unicode suppport.
 *   - Internal COMMCTL32 bitmaps.
 *   - Fix TOOLBAR_SetButtonInfo32A.
 *   - Fix TOOLBAR_Customize. (Customize dialog.)
 *
 * Testing:
 *   - Run tests using Waite Group Windows95 API Bible Volume 2.
 *     The second cdrom contains executables addstr.exe, btncount.exe,
 *     btnstate.exe, butstrsz.exe, chkbtn.exe, chngbmp.exe, customiz.exe,
 *     enablebtn.exe, getbmp.exe, getbtn.exe, getflags.exe, hidebtn.exe,
 *     indetbtn.exe, insbtn.exe, pressbtn.exe, setbtnsz.exe, setcmdid.exe,
 *     setparnt.exe, setrows.exe, toolwnd.exe.
 *   - Microsofts controlspy examples.
 */

#include "commctrl.h"
#include "sysmetrics.h"
#include "cache.h"
#include "toolbar.h"
#include "win.h"
#include "debug.h"

/* #define __NEW_WRAP_CODE__ */

#define SEPARATOR_WIDTH    8
#define SEPARATOR_HEIGHT   5
#define TOP_BORDER         2
#define BOTTOM_BORDER      2



#define TOOLBAR_GetInfoPtr(wndPtr) ((TOOLBAR_INFO *)wndPtr->wExtra[0])


static void
TOOLBAR_DrawFlatSeparator (LPRECT32 lpRect, HDC32 hdc)
{
    INT32 x = (lpRect->left + lpRect->right) / 2 - 1;
    INT32 yBottom = lpRect->bottom - 3;
    INT32 yTop = lpRect->top + 1;

    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DSHADOW));
    MoveToEx32 (hdc, x, yBottom, NULL);
    LineTo32 (hdc, x, yTop);
    x++;
    SelectObject32 ( hdc, GetSysColorPen32 (COLOR_3DHILIGHT));
    MoveToEx32 (hdc, x, yBottom, NULL);
    LineTo32 (hdc, x, yTop);
}


static void
TOOLBAR_DrawString (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
		    HDC32 hdc, INT32 nState)
{
    RECT32   rcText = btnPtr->rect;
    HFONT32  hOldFont;
    INT32    nOldBkMode;
    COLORREF clrOld;

    /* draw text */
    if ((btnPtr->iString > -1) && (btnPtr->iString < infoPtr->nNumStrings)) {
	InflateRect32 (&rcText, -3, -3);
	rcText.top += infoPtr->nBitmapHeight;
	if (nState & (TBSTATE_PRESSED | TBSTATE_CHECKED))
	    OffsetRect32 (&rcText, 1, 1);

	hOldFont = SelectObject32 (hdc, infoPtr->hFont);
	nOldBkMode = SetBkMode32 (hdc, TRANSPARENT);
	if (!(nState & TBSTATE_ENABLED)) {
	    clrOld = SetTextColor32 (hdc, GetSysColor32 (COLOR_3DHILIGHT));
	    OffsetRect32 (&rcText, 1, 1);
	    DrawText32W (hdc, infoPtr->strings[btnPtr->iString], -1,
			 &rcText, infoPtr->dwDTFlags);
	    SetTextColor32 (hdc, GetSysColor32 (COLOR_3DSHADOW));
	    OffsetRect32 (&rcText, -1, -1);
	    DrawText32W (hdc, infoPtr->strings[btnPtr->iString], -1,
			 &rcText, infoPtr->dwDTFlags);
	}
	else if (nState & TBSTATE_INDETERMINATE) {
	    clrOld = SetTextColor32 (hdc, GetSysColor32 (COLOR_3DSHADOW));
	    DrawText32W (hdc, infoPtr->strings[btnPtr->iString], -1,
			 &rcText, infoPtr->dwDTFlags);
	}
	else {
	    clrOld = SetTextColor32 (hdc, GetSysColor32 (COLOR_BTNTEXT));
	    DrawText32W (hdc, infoPtr->strings[btnPtr->iString], -1,
			 &rcText, infoPtr->dwDTFlags);
	}

	SetTextColor32 (hdc, clrOld);
	SelectObject32 (hdc, hOldFont);
	if (nOldBkMode != TRANSPARENT)
	    SetBkMode32 (hdc, nOldBkMode);
    }
}


static void
TOOLBAR_DrawPattern (HDC32 hdc, LPRECT32 lpRect)
{
    HBRUSH32 hbr = SelectObject32 (hdc, CACHE_GetPattern55AABrush ());
    INT32 cx = lpRect->right - lpRect->left;
    INT32 cy = lpRect->bottom - lpRect->top;
    PatBlt32 (hdc, lpRect->left, lpRect->top, cx, cy, 0x00FA0089);
    SelectObject32 (hdc, hbr);
}


static void
TOOLBAR_DrawMasked (TOOLBAR_INFO *infoPtr, TBUTTON_INFO *btnPtr,
		    HDC32 hdc, INT32 x, INT32 y)
{
    /* FIXME: this function is a hack since it uses image list
	      internals directly */

    HDC32 hdcImageList = CreateCompatibleDC32 (0);
    HDC32 hdcMask = CreateCompatibleDC32 (0);
    HIMAGELIST himl = infoPtr->himlStd;
    HBITMAP32 hbmMask;

    /* create new bitmap */
    hbmMask = CreateBitmap32 (himl->cx, himl->cy, 1, 1, NULL);
    SelectObject32 (hdcMask, hbmMask);

    /* copy the mask bitmap */
    SelectObject32 (hdcImageList, himl->hbmMask);
    SetBkColor32 (hdcImageList, RGB(255, 255, 255));
    SetTextColor32 (hdcImageList, RGB(0, 0, 0));
    BitBlt32 (hdcMask, 0, 0, himl->cx, himl->cy,
	      hdcImageList, himl->cx * btnPtr->iBitmap, 0, SRCCOPY);

#if 0
    /* add white mask from image */
    SelectObject32 (hdcImageList, himl->hbmImage);
    SetBkColor32 (hdcImageList, RGB(0, 0, 0));
    BitBlt32 (hdcMask, 0, 0, himl->cx, himl->cy,
	      hdcImageList, himl->cx * btnPtr->iBitmap, 0, MERGEPAINT);
#endif

    /* draw the new mask */
    SelectObject32 (hdc, GetSysColorBrush32 (COLOR_3DHILIGHT));
    BitBlt32 (hdc, x+1, y+1, himl->cx, himl->cy,
	      hdcMask, 0, 0, 0xB8074A);

    SelectObject32 (hdc, GetSysColorBrush32 (COLOR_3DSHADOW));
    BitBlt32 (hdc, x, y, himl->cx, himl->cy,
	      hdcMask, 0, 0, 0xB8074A);

    DeleteObject32 (hbmMask);
    DeleteDC32 (hdcMask);
    DeleteDC32 (hdcImageList);
}


static void
TOOLBAR_DrawButton (WND *wndPtr, TBUTTON_INFO *btnPtr, HDC32 hdc)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    BOOL32 bFlat = (wndPtr->dwStyle & TBSTYLE_FLAT);
    RECT32 rc;

    if (btnPtr->fsState & TBSTATE_HIDDEN) return;

    rc = btnPtr->rect;
    if (btnPtr->fsStyle & TBSTYLE_SEP) {
	if ((bFlat) && (btnPtr->idCommand == 0))
	    TOOLBAR_DrawFlatSeparator (&btnPtr->rect, hdc);
	return;
    }

    /* disabled */
    if (!(btnPtr->fsState & TBSTATE_ENABLED)) {
	DrawEdge32 (hdc, &rc, EDGE_RAISED,
		    BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

	if (bFlat) {
/*	    if (infoPtr->himlDis) */
		ImageList_Draw (infoPtr->himlDis, btnPtr->iBitmap, hdc,
				rc.left+1, rc.top+1, ILD_NORMAL);
/*	    else */
/*		TOOLBAR_DrawMasked (infoPtr, btnPtr, hdc, rc.left+1, rc.top+1); */
	}
	else
	    TOOLBAR_DrawMasked (infoPtr, btnPtr, hdc, rc.left+1, rc.top+1);

	TOOLBAR_DrawString (infoPtr, btnPtr, hdc, btnPtr->fsState);
	return;
    }

    /* pressed TBSTYLE_BUTTON */
    if (btnPtr->fsState & TBSTATE_PRESSED) {
	DrawEdge32 (hdc, &rc, EDGE_SUNKEN,
		    BF_RECT | BF_MIDDLE | BF_ADJUST);
	ImageList_Draw (infoPtr->himlStd, btnPtr->iBitmap, hdc,
			rc.left+2, rc.top+2, ILD_NORMAL);
	TOOLBAR_DrawString (infoPtr, btnPtr, hdc, btnPtr->fsState);
	return;
    }

    /* checked TBSTYLE_CHECK*/
    if ((btnPtr->fsStyle & TBSTYLE_CHECK) &&
	(btnPtr->fsState & TBSTATE_CHECKED)) {
	if (bFlat)
	    DrawEdge32 (hdc, &rc, BDR_SUNKENOUTER,
			BF_RECT | BF_MIDDLE | BF_ADJUST);
	else
	    DrawEdge32 (hdc, &rc, EDGE_SUNKEN,
			BF_RECT | BF_MIDDLE | BF_ADJUST);

	TOOLBAR_DrawPattern (hdc, &rc);
	if (bFlat)
	    ImageList_Draw (infoPtr->himlDef, btnPtr->iBitmap, hdc,
			    rc.left+2, rc.top+2, ILD_NORMAL);
	else
	    ImageList_Draw (infoPtr->himlStd, btnPtr->iBitmap, hdc,
			    rc.left+2, rc.top+2, ILD_NORMAL);
	TOOLBAR_DrawString (infoPtr, btnPtr, hdc, btnPtr->fsState);
	return;
    }

    /* indeterminate */	
    if (btnPtr->fsState & TBSTATE_INDETERMINATE) {
	DrawEdge32 (hdc, &rc, EDGE_RAISED,
		    BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

	TOOLBAR_DrawPattern (hdc, &rc);
	TOOLBAR_DrawMasked (infoPtr, btnPtr, hdc, rc.left+1, rc.top+1);
	TOOLBAR_DrawString (infoPtr, btnPtr, hdc, btnPtr->fsState);
	return;
    }

    /* normal state */
    DrawEdge32 (hdc, &rc, EDGE_RAISED,
		BF_SOFT | BF_RECT | BF_MIDDLE | BF_ADJUST);

    if (bFlat)
	ImageList_Draw (infoPtr->himlDef, btnPtr->iBitmap, hdc,
			rc.left+1, rc.top+1, ILD_NORMAL);
    else
	ImageList_Draw (infoPtr->himlStd, btnPtr->iBitmap, hdc,
			rc.left+1, rc.top+1, ILD_NORMAL);

    TOOLBAR_DrawString (infoPtr, btnPtr, hdc, btnPtr->fsState);
}


static void
TOOLBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i;

    /* draw buttons */
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++)
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
}


static void
TOOLBAR_CalcStrings (WND *wndPtr, LPSIZE32 lpSize)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i;
    HDC32 hdc;
    HFONT32 hOldFont;
    SIZE32 sz;

    lpSize->cx = 0;
    lpSize->cy = 0;
    hdc = GetDC32 (0);
    hOldFont = SelectObject32 (hdc, infoPtr->hFont);

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (!(btnPtr->fsState & TBSTATE_HIDDEN) &&
	    (btnPtr->iString > -1) &&
	    (btnPtr->iString < infoPtr->nNumStrings)) {
	    LPWSTR lpText = infoPtr->strings[btnPtr->iString];
	    GetTextExtentPoint32W (hdc, lpText, lstrlen32W (lpText), &sz);
	    if (sz.cx > lpSize->cx)
		lpSize->cx = sz.cx;
	    if (sz.cy > lpSize->cy)
		lpSize->cy = sz.cy;
	}
    }

    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (0, hdc);

    TRACE (toolbar, "string size %d x %d!\n", lpSize->cx, lpSize->cy);
}


static void
TOOLBAR_CalcToolbar (WND *wndPtr)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr, *grpPtr;
    INT32 i, j, nRows;
    INT32 x, y, cx, cy;
    BOOL32 bWrap;
    SIZE32  sizeString;
/* --- new --- */
    INT32  nGrpCount = 0;
    INT32  grpX;
/* --- end new --- */

    TOOLBAR_CalcStrings (wndPtr, &sizeString);

    if (sizeString.cy > 0)
	infoPtr->nButtonHeight = sizeString.cy + infoPtr->nBitmapHeight + 6;
    else if (infoPtr->nButtonHeight < infoPtr->nBitmapHeight + 6)
	infoPtr->nButtonHeight = infoPtr->nBitmapHeight + 6;

    if (sizeString.cx > infoPtr->nBitmapWidth)
	infoPtr->nButtonWidth = sizeString.cx + 6;
    else if (infoPtr->nButtonWidth < infoPtr->nBitmapWidth + 6)
	infoPtr->nButtonWidth = infoPtr->nBitmapWidth + 6;

    x  = infoPtr->nIndent;
    y  = TOP_BORDER;
    cx = infoPtr->nButtonWidth;
    cy = infoPtr->nButtonHeight;
    nRows = 0;

    /* calculate the size of each button according to it's style */
/*     TOOLBAR_CalcButtons (wndPtr); */

    infoPtr->rcBound.top = y;
    infoPtr->rcBound.left = x;
    infoPtr->rcBound.bottom = y + cy;
    infoPtr->rcBound.right = x;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	bWrap = FALSE;

	if (btnPtr->fsState & TBSTATE_HIDDEN) {
	    SetRectEmpty32 (&btnPtr->rect);
	    continue;
	}

#ifdef __NEW_WRAP_CODE__
/*#if 0 */
	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    /* UNDOCUMENTED: If a separator has a non zero bitmap index, */
	    /* it is the actual width of the separator. This is used for */
	    /* custom controls in toolbars.                              */
	    if ((wndPtr->dwStyle & TBSTYLE_WRAPABLE) &&
		(btnPtr->fsState & TBSTATE_WRAP)) {
		x = 0;
		y += cy;
		cx = infoPtr->nWidth;
		cy = ((btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH) * 2 / 3;
/*		nRows++; */
/*		bWrap = TRUE; */
	    }
	    else
		cx = (btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH;
	}
	else {
	    /* this must be a button */
	    cx = infoPtr->nButtonWidth;
	}
/*#endif */

/* --- begin test --- */
	if ((i >= nGrpCount) && (btnPtr->fsStyle & TBSTYLE_GROUP)) {
	    for (j = i, grpX = x, nGrpCount = 0; j < infoPtr->nNumButtons; j++) {
		grpPtr = &infoPtr->buttons[j];
		if (grpPtr->fsState & TBSTATE_HIDDEN)
		    continue;

		grpX += cx;

		if ((grpPtr->fsStyle & TBSTYLE_SEP) ||
		    !(grpPtr->fsStyle & TBSTYLE_GROUP) ||
		    (grpX > infoPtr->nWidth)) {
		    nGrpCount = j;
		    break;
		}
		else if (grpX + x > infoPtr->nWidth) {
		    bWrap = TRUE;
		    nGrpCount = j;
		    break;
		}
	    }
	}

	bWrap = ((bWrap || (x + cx > infoPtr->nWidth)) &&
		 (wndPtr->dwStyle & TBSTYLE_WRAPABLE));
	if (bWrap) {
	    nRows++;
	    y += cy;
	    x  = infoPtr->nIndent;
	    bWrap = FALSE;
	}

	SetRect32 (&btnPtr->rect, x, y, x + cx, y + cy);

	btnPtr->nRow = nRows;
	x += cx;

	if (btnPtr->fsState & TBSTATE_WRAP) {
	    nRows++;
	    y += (cy + SEPARATOR_HEIGHT);
	    x  = infoPtr->nIndent;
	}

	infoPtr->nRows = nRows + 1;

/* --- end test --- */
#else
	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    /* UNDOCUMENTED: If a separator has a non zero bitmap index, */
	    /* it is the actual width of the separator. This is used for */
	    /* custom controls in toolbars.                              */
	    if ((wndPtr->dwStyle & TBSTYLE_WRAPABLE) &&
		(btnPtr->fsState & TBSTATE_WRAP)) {
		x = 0;
		y += cy;
		cx = infoPtr->nWidth;
		cy = ((btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH) * 2 / 3;
		nRows++;
		bWrap = TRUE;
	    }
	    else
		cx = (btnPtr->iBitmap > 0) ?
		     btnPtr->iBitmap : SEPARATOR_WIDTH;
	}
	else {
	    /* this must be a button */
	    cx = infoPtr->nButtonWidth;
	}

	btnPtr->rect.left   = x;
	btnPtr->rect.top    = y;
	btnPtr->rect.right  = x + cx;
	btnPtr->rect.bottom = y + cy;

	if (infoPtr->rcBound.left > x)
	    infoPtr->rcBound.left = x;
	if (infoPtr->rcBound.right < x + cx)
	    infoPtr->rcBound.right = x + cx;
	if (infoPtr->rcBound.bottom < y + cy)
	    infoPtr->rcBound.bottom = y + cy;

	if (infoPtr->hwndToolTip) {
	    TTTOOLINFO32A ti;

	    ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	    ti.cbSize = sizeof(TTTOOLINFO32A);
	    ti.hwnd = wndPtr->hwndSelf;
	    ti.uId = btnPtr->idCommand;
	    ti.rect = btnPtr->rect;
	    SendMessage32A (infoPtr->hwndToolTip, TTM_NEWTOOLRECT32A,
			    0, (LPARAM)&ti);
	}

	if (bWrap) {
	    x = 0;
	    y += cy;
	    if (i < infoPtr->nNumButtons)
		nRows++;
	}
	else
	    x += cx;
#endif
    }

    infoPtr->nHeight = y + cy + BOTTOM_BORDER;
    TRACE (toolbar, "toolbar height %d\n", infoPtr->nHeight);
}


static INT32
TOOLBAR_InternalHitTest (WND *wndPtr, LPPOINT32 lpPt)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 i;
    
    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->fsState & TBSTATE_HIDDEN)
	    continue;

	if (btnPtr->fsStyle & TBSTYLE_SEP) {
	    if (PtInRect32 (&btnPtr->rect, *lpPt)) {
		TRACE (toolbar, " ON SEPARATOR %d!\n", i);
		return -i;
	    }
	}
	else {
	    if (PtInRect32 (&btnPtr->rect, *lpPt)) {
		TRACE (toolbar, " ON BUTTON %d!\n", i);
		return i;
	    }
	}
    }

    TRACE (toolbar, " NOWHERE!\n");
    return -1;
}


static INT32
TOOLBAR_GetButtonIndex (TOOLBAR_INFO *infoPtr, INT32 idCommand)
{
    TBUTTON_INFO *btnPtr;
    INT32 i;

    btnPtr = infoPtr->buttons;
    for (i = 0; i < infoPtr->nNumButtons; i++, btnPtr++) {
	if (btnPtr->idCommand == idCommand) {
	    TRACE (toolbar, "command=%d index=%d\n", idCommand, i);
	    return i;
	}
    }
    TRACE (toolbar, "no index found for command=%d\n", idCommand);
    return -1;
}


static INT32
TOOLBAR_GetCheckedGroupButtonIndex (TOOLBAR_INFO *infoPtr, INT32 nIndex)
{
    TBUTTON_INFO *btnPtr;
    INT32 nRunIndex;

    if ((nIndex < 0) || (nIndex > infoPtr->nNumButtons))
	return -1;

    /* check index button */
    btnPtr = &infoPtr->buttons[nIndex];
    if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	if (btnPtr->fsState & TBSTATE_CHECKED)
	    return nIndex;
    }

    /* check previous buttons */
    nRunIndex = nIndex - 1;
    while (nRunIndex >= 0) {
	btnPtr = &infoPtr->buttons[nRunIndex];
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex--;
    }

    /* check next buttons */
    nRunIndex = nIndex + 1;
    while (nRunIndex < infoPtr->nNumButtons) {
	btnPtr = &infoPtr->buttons[nRunIndex];	
	if ((btnPtr->fsStyle & TBSTYLE_CHECKGROUP) == TBSTYLE_CHECKGROUP) {
	    if (btnPtr->fsState & TBSTATE_CHECKED)
		return nRunIndex;
	}
	else
	    break;
	nRunIndex++;
    }

    return -1;
}


static VOID
TOOLBAR_RelayEvent (HWND32 hwndTip, HWND32 hwndMsg, UINT32 uMsg,
		    WPARAM32 wParam, LPARAM lParam)
{
    MSG32 msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessage32A (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}


static LRESULT
TOOLBAR_AddBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBADDBITMAP lpAddBmp = (LPTBADDBITMAP)lParam;
    INT32 nIndex = 0;

    if ((!lpAddBmp) || ((INT32)wParam <= 0))
	return -1;

    TRACE (toolbar, "adding %d bitmaps!\n", wParam);

    if (!(infoPtr->himlDef)) {
	/* create new default image list */
	TRACE (toolbar, "creating default image list!\n");
	infoPtr->himlStd =
	    ImageList_Create (infoPtr->nBitmapWidth, infoPtr->nBitmapHeight,
			      ILC_COLOR | ILC_MASK, (INT32)wParam, 2);
    }

#if 0
    if (!(infoPtr->himlDis)) {
	/* create new disabled image list */
	TRACE (toolbar, "creating disabled image list!\n");
	infoPtr->himlDis =
	    ImageList_Create (infoPtr->nBitmapWidth, 
			      infoPtr->nBitmapHeight, ILC_COLOR | ILC_MASK,
			      (INT32)wParam, 2);
    }
#endif

    /* Add bitmaps to the default image list */
    if (lpAddBmp->hInst == (HINSTANCE32)0) {
	nIndex = 
	    ImageList_AddMasked (infoPtr->himlStd, (HBITMAP32)lpAddBmp->nID,
				 CLR_DEFAULT);
    }
    else if (lpAddBmp->hInst == HINST_COMMCTRL) {
	/* add internal bitmaps */
	FIXME (toolbar, "internal bitmaps not supported!\n");

	/* Hack to "add" some reserved images within the image list 
	   to get the right image indices */
	nIndex = ImageList_GetImageCount (infoPtr->himlStd);
	ImageList_SetImageCount (infoPtr->himlStd, nIndex + (INT32)wParam);
    }
    else {
	HBITMAP32 hBmp =
	    LoadBitmap32A (lpAddBmp->hInst, (LPSTR)lpAddBmp->nID);

	nIndex = ImageList_AddMasked (infoPtr->himlStd, hBmp, CLR_DEFAULT);

	DeleteObject32 (hBmp); 
    }


    infoPtr->nNumBitmaps += (INT32)wParam;

    return nIndex;
}


static LRESULT
TOOLBAR_AddButtons32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nOldButtons, nNewButtons, nAddButtons, nCount;
    HDC32 hdc;

    TRACE (toolbar, "adding %d buttons!\n", wParam);

    nAddButtons = (UINT32)wParam;
    nOldButtons = infoPtr->nNumButtons;
    nNewButtons = nOldButtons + nAddButtons;

    if (infoPtr->nNumButtons == 0) {
	infoPtr->buttons =
	    COMCTL32_Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
	infoPtr->buttons =
	    COMCTL32_Alloc (sizeof(TBUTTON_INFO) * nNewButtons);
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nOldButtons * sizeof(TBUTTON_INFO));
        COMCTL32_Free (oldButtons);
    }

    infoPtr->nNumButtons = nNewButtons;

    /* insert new button data */
    for (nCount = 0; nCount < nAddButtons; nCount++) {
	TBUTTON_INFO *btnPtr = &infoPtr->buttons[nOldButtons+nCount];
	btnPtr->iBitmap   = lpTbb[nCount].iBitmap;
	btnPtr->idCommand = lpTbb[nCount].idCommand;
	btnPtr->fsState   = lpTbb[nCount].fsState;
	btnPtr->fsStyle   = lpTbb[nCount].fsStyle;
	btnPtr->dwData    = lpTbb[nCount].dwData;
	btnPtr->iString   = lpTbb[nCount].iString;

	if ((infoPtr->hwndToolTip) && !(btnPtr->fsStyle & TBSTYLE_SEP)) {
	    TTTOOLINFO32A ti;

	    ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	    ti.cbSize   = sizeof (TTTOOLINFO32A);
	    ti.hwnd     = wndPtr->hwndSelf;
	    ti.uId      = btnPtr->idCommand;
	    ti.hinst    = 0;
	    ti.lpszText = LPSTR_TEXTCALLBACK32A;

	    SendMessage32A (infoPtr->hwndToolTip, TTM_ADDTOOL32A,
			    0, (LPARAM)&ti);
	}
    }

    TOOLBAR_CalcToolbar (wndPtr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << TOOLBAR_AddButtons32W >> */


static LRESULT
TOOLBAR_AddString32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    if ((wParam) && (HIWORD(lParam) == 0)) {
	char szString[256];
	INT32 len;
	TRACE (toolbar, "adding string from resource!\n");

	len = LoadString32A ((HINSTANCE32)wParam, (UINT32)lParam,
			     szString, 256);

	TRACE (toolbar, "len=%d \"%s\"\n", len, szString);
	nIndex = infoPtr->nNumStrings;
	if (infoPtr->nNumStrings == 0) {
	    infoPtr->strings =
		COMCTL32_Alloc (sizeof(LPWSTR));
	}
	else {
	    LPWSTR *oldStrings = infoPtr->strings;
	    infoPtr->strings =
		COMCTL32_Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
	    memcpy (&infoPtr->strings[0], &oldStrings[0],
		    sizeof(LPWSTR) * infoPtr->nNumStrings);
	    COMCTL32_Free (oldStrings);
	}

	infoPtr->strings[infoPtr->nNumStrings] =
	    COMCTL32_Alloc (sizeof(WCHAR)*(len+1));
	lstrcpyAtoW (infoPtr->strings[infoPtr->nNumStrings], szString);
	infoPtr->nNumStrings++;
    }
    else {
	LPSTR p = (LPSTR)lParam;
	INT32 len;

	if (p == NULL)
	    return -1;
	TRACE (toolbar, "adding string(s) from array!\n");
	nIndex = infoPtr->nNumStrings;
	while (*p) {
	    len = lstrlen32A (p);
	    TRACE (toolbar, "len=%d \"%s\"\n", len, p);

	    if (infoPtr->nNumStrings == 0) {
		infoPtr->strings =
		    COMCTL32_Alloc (sizeof(LPWSTR));
	    }
	    else {
		LPWSTR *oldStrings = infoPtr->strings;
		infoPtr->strings =
		    COMCTL32_Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
		memcpy (&infoPtr->strings[0], &oldStrings[0],
			sizeof(LPWSTR) * infoPtr->nNumStrings);
		COMCTL32_Free (oldStrings);
	    }

	    infoPtr->strings[infoPtr->nNumStrings] =
		COMCTL32_Alloc (sizeof(WCHAR)*(len+1));
	    lstrcpyAtoW (infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    return nIndex;
}


static LRESULT
TOOLBAR_AddString32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    if ((wParam) && (HIWORD(lParam) == 0)) {
	WCHAR szString[256];
	INT32 len;
	TRACE (toolbar, "adding string from resource!\n");

	len = LoadString32W ((HINSTANCE32)wParam, (UINT32)lParam,
			     szString, 256);

	TRACE (toolbar, "len=%d \"%s\"\n", len, debugstr_w(szString));
	nIndex = infoPtr->nNumStrings;
	if (infoPtr->nNumStrings == 0) {
	    infoPtr->strings =
		COMCTL32_Alloc (sizeof(LPWSTR));
	}
	else {
	    LPWSTR *oldStrings = infoPtr->strings;
	    infoPtr->strings =
		COMCTL32_Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
	    memcpy (&infoPtr->strings[0], &oldStrings[0],
		    sizeof(LPWSTR) * infoPtr->nNumStrings);
	    COMCTL32_Free (oldStrings);
	}

	infoPtr->strings[infoPtr->nNumStrings] =
	    COMCTL32_Alloc (sizeof(WCHAR)*(len+1));
	lstrcpy32W (infoPtr->strings[infoPtr->nNumStrings], szString);
	infoPtr->nNumStrings++;
    }
    else {
	LPWSTR p = (LPWSTR)lParam;
	INT32 len;

	if (p == NULL)
	    return -1;
	TRACE (toolbar, "adding string(s) from array!\n");
	nIndex = infoPtr->nNumStrings;
	while (*p) {
	    len = lstrlen32W (p);
	    TRACE (toolbar, "len=%d \"%s\"\n", len, debugstr_w(p));

	    if (infoPtr->nNumStrings == 0) {
		infoPtr->strings =
		    COMCTL32_Alloc (sizeof(LPWSTR));
	    }
	    else {
		LPWSTR *oldStrings = infoPtr->strings;
		infoPtr->strings =
		    COMCTL32_Alloc (sizeof(LPWSTR) * (infoPtr->nNumStrings + 1));
		memcpy (&infoPtr->strings[0], &oldStrings[0],
			sizeof(LPWSTR) * infoPtr->nNumStrings);
		COMCTL32_Free (oldStrings);
	    }

	    infoPtr->strings[infoPtr->nNumStrings] =
		COMCTL32_Alloc (sizeof(WCHAR)*(len+1));
	    lstrcpy32W (infoPtr->strings[infoPtr->nNumStrings], p);
	    infoPtr->nNumStrings++;

	    p += (len+1);
	}
    }

    return nIndex;
}


static LRESULT
TOOLBAR_AutoSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    RECT32 parent_rect;
    HWND32 parent;
    /* INT32  x, y; */
    INT32  cx, cy;
    UINT32 uPosFlags = 0;

    TRACE (toolbar, "resize forced!\n");

    parent = GetParent32 (wndPtr->hwndSelf);
    GetClientRect32(parent, &parent_rect);

    if (wndPtr->dwStyle & CCS_NORESIZE) {
	uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);
	cx = 0;
	cy = 0;
    }
    else {
	infoPtr->nWidth = parent_rect.right - parent_rect.left;
	TOOLBAR_CalcToolbar (wndPtr);
	cy = infoPtr->nHeight;
	cx = infoPtr->nWidth;
    }

    if (wndPtr->dwStyle & CCS_NOPARENTALIGN)
	uPosFlags |= SWP_NOMOVE;

    if (!(wndPtr->dwStyle & CCS_NODIVIDER))
	cy += sysMetrics[SM_CYEDGE];

    infoPtr->bAutoSize = TRUE;
    SetWindowPos32 (wndPtr->hwndSelf, HWND_TOP, parent_rect.left, parent_rect.top,
		    cx, cy, uPosFlags);

    return 0;
}


static LRESULT
TOOLBAR_ButtonCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return infoPtr->nNumButtons;
}


static LRESULT
TOOLBAR_ButtonStructSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) {
	ERR (toolbar, "(0x%08lx, 0x%08x, 0x%08lx)\n", (DWORD)wndPtr, wParam, lParam);
	ERR (toolbar, "infoPtr == NULL!\n");
	return 0;
    }

    infoPtr->dwStructSize = (DWORD)wParam;

    return 0;
}


static LRESULT
TOOLBAR_ChangeBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->iBitmap = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_CheckButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;
    INT32 nOldIndex = -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];

    if (!(btnPtr->fsStyle & TBSTYLE_CHECK))
	return FALSE;

    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_CHECKED;
    else {
	if (btnPtr->fsStyle & TBSTYLE_GROUP) {
	    nOldIndex = 
		TOOLBAR_GetCheckedGroupButtonIndex (infoPtr, nIndex);
	    if (nOldIndex == nIndex)
		return 0;
	    if (nOldIndex != -1)
		infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
	}
	btnPtr->fsState |= TBSTATE_CHECKED;
    }

    hdc = GetDC32 (wndPtr->hwndSelf);
    if (nOldIndex != -1)
	TOOLBAR_DrawButton (wndPtr, &infoPtr->buttons[nOldIndex], hdc);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    /* FIXME: Send a WM_NOTIFY?? */

    return TRUE;
}


static LRESULT
TOOLBAR_CommandToIndex (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
}


static LRESULT
TOOLBAR_Customize (WND *wndPtr)
{
    FIXME (toolbar, "customization not implemented!\n");

    return 0;
}


static LRESULT
TOOLBAR_DeleteButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex = (INT32)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    if ((infoPtr->hwndToolTip) && 
	!(infoPtr->buttons[nIndex].fsStyle & TBSTYLE_SEP)) {
	TTTOOLINFO32A ti;

	ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	ti.cbSize   = sizeof (TTTOOLINFO32A);
	ti.hwnd     = wndPtr->hwndSelf;
	ti.uId      = infoPtr->buttons[nIndex].idCommand;

	SendMessage32A (infoPtr->hwndToolTip, TTM_DELTOOL32A, 0, (LPARAM)&ti);
    }

    if (infoPtr->nNumButtons == 1) {
	TRACE (toolbar, " simple delete!\n");
	COMCTL32_Free (infoPtr->buttons);
	infoPtr->buttons = NULL;
	infoPtr->nNumButtons = 0;
    }
    else {
	TBUTTON_INFO *oldButtons = infoPtr->buttons;
        TRACE(toolbar, "complex delete! [nIndex=%d]\n", nIndex);

	infoPtr->nNumButtons--;
	infoPtr->buttons = COMCTL32_Alloc (sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
        if (nIndex > 0) {
            memcpy (&infoPtr->buttons[0], &oldButtons[0],
                    nIndex * sizeof(TBUTTON_INFO));
        }

        if (nIndex < infoPtr->nNumButtons) {
            memcpy (&infoPtr->buttons[nIndex], &oldButtons[nIndex+1],
                    (infoPtr->nNumButtons - nIndex) * sizeof(TBUTTON_INFO));
        }

	COMCTL32_Free (oldButtons);
    }

    TOOLBAR_CalcToolbar (wndPtr);

    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);
    UpdateWindow32 (wndPtr->hwndSelf);

    return TRUE;
}


static LRESULT
TOOLBAR_EnableButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~(TBSTATE_ENABLED | TBSTATE_PRESSED);
    else
	btnPtr->fsState |= TBSTATE_ENABLED;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << TOOLBAR_GetAnchorHighlight >> */


static LRESULT
TOOLBAR_GetBitmap (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return -1;

    return infoPtr->buttons[nIndex].iBitmap;
}


static __inline__ LRESULT
TOOLBAR_GetBitmapFlags (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return (GetDeviceCaps32 (0, LOGPIXELSX) >= 120) ? TBBF_LARGE : 0;
}


static LRESULT
TOOLBAR_GetButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nIndex = (INT32)wParam;
    TBUTTON_INFO *btnPtr;

    if (infoPtr == NULL) return FALSE;
    if (lpTbb == NULL) return FALSE;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    lpTbb->iBitmap   = btnPtr->iBitmap;
    lpTbb->idCommand = btnPtr->idCommand;
    lpTbb->fsState   = btnPtr->fsState;
    lpTbb->fsStyle   = btnPtr->fsStyle;
    lpTbb->dwData    = btnPtr->dwData;
    lpTbb->iString   = btnPtr->iString;

    return TRUE;
}


static LRESULT
TOOLBAR_GetButtonInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTONINFO32A lpTbInfo = (LPTBBUTTONINFO32A)lParam;
    TBUTTON_INFO *btnPtr;
    INT32 nIndex;

    if (infoPtr == NULL) return -1;
    if (lpTbInfo == NULL) return -1;
    if (lpTbInfo->cbSize < sizeof(LPTBBUTTONINFO32A)) return -1;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return -1;

    btnPtr = &infoPtr->buttons[nIndex];

    if (lpTbInfo->dwMask & TBIF_COMMAND)
	lpTbInfo->idCommand = btnPtr->idCommand;
    if (lpTbInfo->dwMask & TBIF_IMAGE)
	lpTbInfo->iImage = btnPtr->iBitmap;
    if (lpTbInfo->dwMask & TBIF_LPARAM)
	lpTbInfo->lParam = btnPtr->dwData;
    if (lpTbInfo->dwMask & TBIF_SIZE)
	lpTbInfo->cx = (WORD)(btnPtr->rect.right - btnPtr->rect.left);
    if (lpTbInfo->dwMask & TBIF_STATE)
	lpTbInfo->fsState = btnPtr->fsState;
    if (lpTbInfo->dwMask & TBIF_STYLE)
	lpTbInfo->fsStyle = btnPtr->fsStyle;
    if (lpTbInfo->dwMask & TBIF_TEXT) {
	if ((btnPtr->iString >= 0) || (btnPtr->iString < infoPtr->nNumStrings))
	    lstrcpyn32A (lpTbInfo->pszText, 
			 (LPSTR)infoPtr->strings[btnPtr->iString],
			 lpTbInfo->cchText);
    }

    return nIndex;
}


/* << TOOLBAR_GetButtonInfo32W >> */


static LRESULT
TOOLBAR_GetButtonSize (WND *wndPtr)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return MAKELONG((WORD)infoPtr->nButtonWidth,
		    (WORD)infoPtr->nButtonHeight);
}


static LRESULT
TOOLBAR_GetButtonText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex, nStringIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return -1;

    nStringIndex = infoPtr->buttons[nIndex].iString;

    TRACE (toolbar, "index=%d stringIndex=%d\n", nIndex, nStringIndex);

    if ((nStringIndex < 0) || (nStringIndex >= infoPtr->nNumStrings))
	return -1;

    if (lParam == 0) return -1;

    lstrcpy32A ((LPSTR)lParam, (LPSTR)infoPtr->strings[nStringIndex]);

    return lstrlen32A ((LPSTR)infoPtr->strings[nStringIndex]);
}


/* << TOOLBAR_GetButtonText32W >> */
/* << TOOLBAR_GetColorScheme >> */


static LRESULT
TOOLBAR_GetDisabledImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBSTYLE_FLAT)
	return (LRESULT)infoPtr->himlDis;
    else
	return 0;
}


__inline__ static LRESULT
TOOLBAR_GetExtendedStyle (WND *wndPtr)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    return infoPtr->dwExStyle;
}


static LRESULT
TOOLBAR_GetHotImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBSTYLE_FLAT)
	return (LRESULT)infoPtr->himlHot;
    else
	return 0;
}


/* << TOOLBAR_GetHotItem >> */


static LRESULT
TOOLBAR_GetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBSTYLE_FLAT)
	return (LRESULT)infoPtr->himlDef;
    else
	return 0;
}


/* << TOOLBAR_GetInsertMark >> */
/* << TOOLBAR_GetInsertMarkColor >> */


static LRESULT
TOOLBAR_GetItemRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    LPRECT32     lpRect;
    INT32        nIndex;

    if (infoPtr == NULL) return FALSE;
    nIndex = (INT32)wParam;
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;
    lpRect = (LPRECT32)lParam;
    if (lpRect == NULL) return FALSE;
    if (btnPtr->fsState & TBSTATE_HIDDEN) return FALSE;
    
    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


static LRESULT
TOOLBAR_GetMaxSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPSIZE32 lpSize = (LPSIZE32)lParam;

    if (lpSize == NULL)
	return FALSE;

    lpSize->cx = infoPtr->rcBound.right - infoPtr->rcBound.left;
    lpSize->cy = infoPtr->rcBound.bottom - infoPtr->rcBound.top;

    TRACE (toolbar, "maximum size %d x %d\n",
	   infoPtr->rcBound.right - infoPtr->rcBound.left,
	   infoPtr->rcBound.bottom - infoPtr->rcBound.top);

    return TRUE;
}


/* << TOOLBAR_GetObject >> */
/* << TOOLBAR_GetPadding >> */


static LRESULT
TOOLBAR_GetRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    LPRECT32     lpRect;
    INT32        nIndex;

    if (infoPtr == NULL) return FALSE;
    nIndex = (INT32)wParam;
    btnPtr = &infoPtr->buttons[nIndex];
    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;
    lpRect = (LPRECT32)lParam;
    if (lpRect == NULL) return FALSE;
    
    lpRect->left   = btnPtr->rect.left;
    lpRect->right  = btnPtr->rect.right;
    lpRect->bottom = btnPtr->rect.bottom;
    lpRect->top    = btnPtr->rect.top;

    return TRUE;
}


static LRESULT
TOOLBAR_GetRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBSTYLE_WRAPABLE)
	return infoPtr->nRows;
    else
	return 1;
}


static LRESULT
TOOLBAR_GetState (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1) return -1;

    return infoPtr->buttons[nIndex].fsState;
}


static LRESULT
TOOLBAR_GetStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1) return -1;

    return infoPtr->buttons[nIndex].fsStyle;
}


static LRESULT
TOOLBAR_GetTextRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL)
	return 0;

    return infoPtr->nMaxTextRows;
}


static LRESULT
TOOLBAR_GetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) return 0;
    return infoPtr->hwndToolTip;
}


static LRESULT
TOOLBAR_GetUnicodeFormat (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    TRACE (toolbar, "%s hwnd=0x%04x stub!\n", 
	   infoPtr->bUnicode ? "TRUE" : "FALSE", wndPtr->hwndSelf);

    return infoPtr->bUnicode;
}


static LRESULT
TOOLBAR_HideButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_HIDDEN;
    else
	btnPtr->fsState |= TBSTATE_HIDDEN;

    TOOLBAR_CalcToolbar (wndPtr);

    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);
    UpdateWindow32 (wndPtr->hwndSelf);

    return TRUE;
}


__inline__ static LRESULT
TOOLBAR_HitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    return TOOLBAR_InternalHitTest (wndPtr, (LPPOINT32)lParam);
}


static LRESULT
TOOLBAR_Indeterminate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_INDETERMINATE;
    else
	btnPtr->fsState |= TBSTATE_INDETERMINATE;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_InsertButton32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTON lpTbb = (LPTBBUTTON)lParam;
    INT32 nIndex = (INT32)wParam;
    TBUTTON_INFO *oldButtons;
    HDC32 hdc;

    if (lpTbb == NULL) return FALSE;
    if (nIndex < 0) return FALSE;

    TRACE (toolbar, "inserting button index=%d\n", nIndex);
    if (nIndex > infoPtr->nNumButtons) {
	nIndex = infoPtr->nNumButtons;
	TRACE (toolbar, "adjust index=%d\n", nIndex);
    }

    oldButtons = infoPtr->buttons;
    infoPtr->nNumButtons++;
    infoPtr->buttons = COMCTL32_Alloc (sizeof (TBUTTON_INFO) * infoPtr->nNumButtons);
    /* pre insert copy */
    if (nIndex > 0) {
	memcpy (&infoPtr->buttons[0], &oldButtons[0],
		nIndex * sizeof(TBUTTON_INFO));
    }

    /* insert new button */
    infoPtr->buttons[nIndex].iBitmap   = lpTbb->iBitmap;
    infoPtr->buttons[nIndex].idCommand = lpTbb->idCommand;
    infoPtr->buttons[nIndex].fsState   = lpTbb->fsState;
    infoPtr->buttons[nIndex].fsStyle   = lpTbb->fsStyle;
    infoPtr->buttons[nIndex].dwData    = lpTbb->dwData;
    infoPtr->buttons[nIndex].iString   = lpTbb->iString;

    if ((infoPtr->hwndToolTip) && !(lpTbb->fsStyle & TBSTYLE_SEP)) {
	TTTOOLINFO32A ti;

	ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	ti.cbSize   = sizeof (TTTOOLINFO32A);
	ti.hwnd     = wndPtr->hwndSelf;
	ti.uId      = lpTbb->idCommand;
	ti.hinst    = 0;
	ti.lpszText = LPSTR_TEXTCALLBACK32A;

	SendMessage32A (infoPtr->hwndToolTip, TTM_ADDTOOL32A,
			0, (LPARAM)&ti);
    }

    /* post insert copy */
    if (nIndex < infoPtr->nNumButtons - 1) {
	memcpy (&infoPtr->buttons[nIndex+1], &oldButtons[nIndex],
		(infoPtr->nNumButtons - nIndex - 1) * sizeof(TBUTTON_INFO));
    }

    COMCTL32_Free (oldButtons);

    TOOLBAR_CalcToolbar (wndPtr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << TOOLBAR_InsertButton32W >> */
/* << TOOLBAR_InsertMarkHitTest >> */


static LRESULT
TOOLBAR_IsButtonChecked (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_CHECKED);
}


static LRESULT
TOOLBAR_IsButtonEnabled (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_ENABLED);
}


static LRESULT
TOOLBAR_IsButtonHidden (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_HIDDEN);
}


static LRESULT
TOOLBAR_IsButtonHighlighted (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_MARKED);
}


static LRESULT
TOOLBAR_IsButtonIndeterminate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_INDETERMINATE);
}


static LRESULT
TOOLBAR_IsButtonPressed (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    return (infoPtr->buttons[nIndex].fsState & TBSTATE_PRESSED);
}


/* << TOOLBAR_LoadImages >> */
/* << TOOLBAR_MapAccelerator >> */
/* << TOOLBAR_MarkButton >> */
/* << TOOLBAR_MoveButton >> */


static LRESULT
TOOLBAR_PressButton (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (LOWORD(lParam) == FALSE)
	btnPtr->fsState &= ~TBSTATE_PRESSED;
    else
	btnPtr->fsState |= TBSTATE_PRESSED;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << TOOLBAR_ReplaceBitmap >> */


static LRESULT
TOOLBAR_SaveRestore32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
#if 0
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBSAVEPARAMS32A lpSave = (LPTBSAVEPARAMS32A)lParam;

    if (lpSave == NULL) return 0;

    if ((BOOL32)wParam) {
	/* save toolbar information */
	FIXME (toolbar, "save to \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
    else {
	/* restore toolbar information */

	FIXME (toolbar, "restore from \"%s\" \"%s\"\n",
	       lpSave->pszSubKey, lpSave->pszValueName);


    }
#endif

    return 0;
}


/* << TOOLBAR_SaveRestore32W >> */
/* << TOOLBAR_SetAnchorHighlight >> */


static LRESULT
TOOLBAR_SetBitmapSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if ((LOWORD(lParam) <= 0) || (HIWORD(lParam)<=0))
	return FALSE;

    infoPtr->nBitmapWidth = (INT32)LOWORD(lParam);
    infoPtr->nBitmapHeight = (INT32)HIWORD(lParam);

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPTBBUTTONINFO32A lptbbi = (LPTBBUTTONINFO32A)lParam;
    TBUTTON_INFO *btnPtr;
    INT32 nIndex;

    if (lptbbi == NULL)
	return FALSE;
    if (lptbbi->cbSize < sizeof(LPTBBUTTONINFO32A))
	return FALSE;
    
    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    if (lptbbi->dwMask & TBIF_COMMAND)
	btnPtr->idCommand = lptbbi->idCommand;
    if (lptbbi->dwMask & TBIF_IMAGE)
	btnPtr->iBitmap = lptbbi->iImage;
    if (lptbbi->dwMask & TBIF_LPARAM)
	btnPtr->dwData = lptbbi->lParam;
/*    if (lptbbi->dwMask & TBIF_SIZE) */
/*	btnPtr->cx = lptbbi->cx; */
    if (lptbbi->dwMask & TBIF_STATE)
	btnPtr->fsState = lptbbi->fsState;
    if (lptbbi->dwMask & TBIF_STYLE)
	btnPtr->fsStyle = lptbbi->fsStyle;

    if (lptbbi->dwMask & TBIF_TEXT) {
	if ((btnPtr->iString >= 0) || 
	    (btnPtr->iString < infoPtr->nNumStrings)) {
#if 0
	    CHAR **lpString = &infoPtr->strings[btnPtr->iString];
	    INT32 len = lstrlen32A (lptbbi->pszText);
	    *lpString = COMCTL32_ReAlloc (lpString, sizeof(char)*(len+1));
#endif

	    /* this is the ultimate sollution */
/*	    Str_SetPtrA (&infoPtr->strings[btnPtr->iString], lptbbi->pszText); */
	}
    }

    return TRUE;
}


/* << TOOLBAR_SetButtonInfo32W >> */


static LRESULT
TOOLBAR_SetButtonSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if ((LOWORD(lParam) <= 0) || (HIWORD(lParam)<=0))
	return FALSE;

    infoPtr->nButtonWidth = (INT32)LOWORD(lParam);
    infoPtr->nButtonHeight = (INT32)HIWORD(lParam);

    return TRUE;
}


static LRESULT
TOOLBAR_SetButtonWidth (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL)
	return FALSE;

    infoPtr->cxMin = (INT32)LOWORD(lParam);
    infoPtr->cxMax = (INT32)HIWORD(lParam);

    return TRUE;
}


static LRESULT
TOOLBAR_SetCmdId (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    INT32 nIndex = (INT32)wParam;

    if ((nIndex < 0) || (nIndex >= infoPtr->nNumButtons))
	return FALSE;

    infoPtr->buttons[nIndex].idCommand = (INT32)lParam;

    if (infoPtr->hwndToolTip) {

	FIXME (toolbar, "change tool tip!\n");

    }

    return TRUE;
}


/* << TOOLBAR_SetColorScheme >> */


static LRESULT
TOOLBAR_SetDisabledImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    if (!(wndPtr->dwStyle & TBSTYLE_FLAT))
	return 0;

    himlTemp = infoPtr->himlDis;
    infoPtr->himlDis = (HIMAGELIST)lParam;

    /* FIXME: redraw ? */

    return (LRESULT)himlTemp; 
}


static LRESULT
TOOLBAR_SetDrawTextFlags (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    DWORD dwTemp;

    dwTemp = infoPtr->dwDTFlags;
    infoPtr->dwDTFlags =
	(infoPtr->dwDTFlags & (DWORD)wParam) | (DWORD)lParam;

    return (LRESULT)dwTemp;
}


static LRESULT
TOOLBAR_SetExtendedStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    DWORD dwTemp;

    dwTemp = infoPtr->dwExStyle;
    infoPtr->dwExStyle = (DWORD)lParam;

    return (LRESULT)dwTemp; 
}


static LRESULT
TOOLBAR_SetHotImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    if (!(wndPtr->dwStyle & TBSTYLE_FLAT))
	return 0;

    himlTemp = infoPtr->himlHot;
    infoPtr->himlHot = (HIMAGELIST)lParam;

    /* FIXME: redraw ? */

    return (LRESULT)himlTemp; 
}


/* << TOOLBAR_SetHotItem >> */


static LRESULT
TOOLBAR_SetImageList (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HIMAGELIST himlTemp;

    if (!(wndPtr->dwStyle & TBSTYLE_FLAT))
	return 0;

    himlTemp = infoPtr->himlDef;
    infoPtr->himlDef = (HIMAGELIST)lParam;

    /* FIXME: redraw ? */

    return (LRESULT)himlTemp; 
}


static LRESULT
TOOLBAR_SetIndent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HDC32 hdc;

    infoPtr->nIndent = (INT32)wParam;
    TOOLBAR_CalcToolbar (wndPtr);
    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


/* << TOOLBAR_SetInsertMark >> */


static LRESULT
TOOLBAR_SetInsertMarkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    infoPtr->clrInsertMark = (COLORREF)lParam;

    /* FIXME : redraw ??*/

    return 0;
}


static LRESULT
TOOLBAR_SetMaxTextRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL)
	return FALSE;

    infoPtr->nMaxTextRows = (INT32)wParam;

    return TRUE;
}


/* << TOOLBAR_SetPadding >> */


static LRESULT
TOOLBAR_SetParent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    HWND32 hwndOldNotify;

    if (infoPtr == NULL) return 0;
    hwndOldNotify = infoPtr->hwndNotify;
    infoPtr->hwndNotify = (HWND32)wParam;

    return hwndOldNotify;
}


static LRESULT
TOOLBAR_SetRows (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPRECT32 lprc = (LPRECT32)lParam;
    HDC32 hdc;

    if (LOWORD(wParam) > 1) {

	FIXME (toolbar, "multiple rows not supported!\n");

    }

    /* recalculate toolbar */
    TOOLBAR_CalcToolbar (wndPtr);

    /* return bounding rectangle */
    if (lprc) {
	lprc->left   = infoPtr->rcBound.left;
	lprc->right  = infoPtr->rcBound.right;
	lprc->top    = infoPtr->rcBound.top;
	lprc->bottom = infoPtr->rcBound.bottom;
    }

    /* repaint toolbar */
    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
TOOLBAR_SetState (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->fsState = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
TOOLBAR_SetStyle (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    HDC32 hdc;
    INT32 nIndex;

    nIndex = TOOLBAR_GetButtonIndex (infoPtr, (INT32)wParam);
    if (nIndex == -1)
	return FALSE;

    btnPtr = &infoPtr->buttons[nIndex];
    btnPtr->fsStyle = LOWORD(lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    if (infoPtr->hwndToolTip) {

	FIXME (toolbar, "change tool tip!\n");

    }

    return TRUE;
}


__inline__ static LRESULT
TOOLBAR_SetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr == NULL) return 0;
    infoPtr->hwndToolTip = (HWND32)wParam;
    return 0;
}


static LRESULT
TOOLBAR_SetUnicodeFormat (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    BOOL32 bTemp;

    TRACE (toolbar, "%s hwnd=0x%04x stub!\n", 
	   ((BOOL32)wParam) ? "TRUE" : "FALSE", wndPtr->hwndSelf);

    bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL32)wParam;

    return bTemp;
}


static LRESULT
TOOLBAR_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LOGFONT32A logFont;

    /* initialize info structure */
    infoPtr->nButtonHeight = 22;
    infoPtr->nButtonWidth = 23;
    infoPtr->nBitmapHeight = 15;
    infoPtr->nBitmapWidth = 16;

    infoPtr->nHeight = infoPtr->nButtonHeight + TOP_BORDER + BOTTOM_BORDER;
    infoPtr->nRows = 1;
    infoPtr->nMaxTextRows = 1;
    infoPtr->cxMin = -1;
    infoPtr->cxMax = -1;

    infoPtr->bCaptured = FALSE;
    infoPtr->bUnicode = IsWindowUnicode (wndPtr->hwndSelf);
    infoPtr->nButtonDown = -1;
    infoPtr->nOldHit = -1;

    infoPtr->hwndNotify = GetParent32 (wndPtr->hwndSelf);
    infoPtr->bTransparent = (wndPtr->dwStyle & TBSTYLE_FLAT);
    infoPtr->nHotItem = -1;
    infoPtr->dwDTFlags = DT_CENTER;

    SystemParametersInfo32A (SPI_GETICONTITLELOGFONT, 0, &logFont, 0);
    infoPtr->hFont = CreateFontIndirect32A (&logFont);

    if (wndPtr->dwStyle & TBSTYLE_TOOLTIPS) {
	/* Create tooltip control */
	infoPtr->hwndToolTip =
	    CreateWindowEx32A (0, TOOLTIPS_CLASS32A, NULL, 0,
			       CW_USEDEFAULT32, CW_USEDEFAULT32,
			       CW_USEDEFAULT32, CW_USEDEFAULT32,
			       wndPtr->hwndSelf, 0, 0, 0);

	/* Send NM_TOOLTIPSCREATED notification */
	if (infoPtr->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hdr.hwndFrom = wndPtr->hwndSelf;
	    nmttc.hdr.idFrom = wndPtr->wIDmenu;
	    nmttc.hdr.code = NM_TOOLTIPSCREATED;
	    nmttc.hwndToolTips = infoPtr->hwndToolTip;

	    SendMessage32A (infoPtr->hwndNotify, WM_NOTIFY,
			    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmttc);
	}
    }

    return 0;
}


static LRESULT
TOOLBAR_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow32 (infoPtr->hwndToolTip);

    /* delete button data */
    if (infoPtr->buttons)
	COMCTL32_Free (infoPtr->buttons);

    /* delete strings */
    if (infoPtr->strings) {
	INT32 i;
	for (i = 0; i < infoPtr->nNumStrings; i++)
	    if (infoPtr->strings[i])
		COMCTL32_Free (infoPtr->strings[i]);

	COMCTL32_Free (infoPtr->strings);
    }

    /* destroy default image list */
    if (infoPtr->himlDef)
	ImageList_Destroy (infoPtr->himlDef);

    /* destroy disabled image list */
    if (infoPtr->himlDis)
	ImageList_Destroy (infoPtr->himlDis);

    /* destroy hot image list */
    if (infoPtr->himlHot)
	ImageList_Destroy (infoPtr->himlHot);

    /* delete default font */
    if (infoPtr->hFont)
	DeleteObject32 (infoPtr->hFont);

    /* free toolbar info data */
    COMCTL32_Free (infoPtr);

    return 0;
}


static LRESULT
TOOLBAR_EraseBackground (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);

    if (infoPtr->bTransparent)
	return SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_ERASEBKGND,
			       wParam, lParam);

    return DefWindowProc32A (wndPtr->hwndSelf, WM_ERASEBKGND, wParam, lParam);
}


static LRESULT
TOOLBAR_LButtonDblClk (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (nHit >= 0) {
	btnPtr = &infoPtr->buttons[nHit];
	if (!(btnPtr->fsState & TBSTATE_ENABLED))
	    return 0;
	SetCapture32 (wndPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;

	hdc = GetDC32 (wndPtr->hwndSelf);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }
    else if (wndPtr->dwStyle & CCS_ADJUSTABLE)
	TOOLBAR_Customize (wndPtr);

    return 0;
}


static LRESULT
TOOLBAR_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
			    WM_LBUTTONDOWN, wParam, lParam);

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (nHit >= 0) {
	btnPtr = &infoPtr->buttons[nHit];
	if (!(btnPtr->fsState & TBSTATE_ENABLED))
	    return 0;

	SetCapture32 (wndPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;
	infoPtr->nButtonDown = nHit;
	infoPtr->nOldHit = nHit;

	btnPtr->fsState |= TBSTATE_PRESSED;

	hdc = GetDC32 (wndPtr->hwndSelf);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TOOLBAR_LButtonUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    INT32   nOldIndex = -1;
    HDC32   hdc;
    BOOL32  bSendMessage = TRUE;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
			    WM_LBUTTONUP, wParam, lParam);

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if ((infoPtr->bCaptured) && (infoPtr->nButtonDown >= 0)) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	btnPtr->fsState &= ~TBSTATE_PRESSED;

	if (nHit == infoPtr->nButtonDown) {
	    if (btnPtr->fsStyle & TBSTYLE_CHECK) {
		if (btnPtr->fsStyle & TBSTYLE_GROUP) {
		    nOldIndex = TOOLBAR_GetCheckedGroupButtonIndex (infoPtr,
			infoPtr->nButtonDown);
		    if (nOldIndex == infoPtr->nButtonDown)
			bSendMessage = FALSE;
		    if ((nOldIndex != infoPtr->nButtonDown) && 
			(nOldIndex != -1))
			infoPtr->buttons[nOldIndex].fsState &= ~TBSTATE_CHECKED;
		    btnPtr->fsState |= TBSTATE_CHECKED;
		}
		else {
		    if (btnPtr->fsState & TBSTATE_CHECKED)
			btnPtr->fsState &= ~TBSTATE_CHECKED;
		    else
			btnPtr->fsState |= TBSTATE_CHECKED;
		}
	    }
	}
	else
	    bSendMessage = FALSE;

	hdc = GetDC32 (wndPtr->hwndSelf);
	if (nOldIndex != -1)
	    TOOLBAR_DrawButton (wndPtr, &infoPtr->buttons[nOldIndex], hdc);
	TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);

	if (bSendMessage)
	    SendMessage32A (infoPtr->hwndNotify, WM_COMMAND,
			    MAKEWPARAM(btnPtr->idCommand, 0),
			    (LPARAM)wndPtr->hwndSelf);

	infoPtr->nButtonDown = -1;
	infoPtr->nOldHit = -1;
    }

    return 0;
}


static LRESULT
TOOLBAR_MouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    TBUTTON_INFO *btnPtr;
    POINT32 pt;
    INT32   nHit;
    HDC32   hdc;

    if (infoPtr->hwndToolTip)
	TOOLBAR_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
			    WM_MOUSEMOVE, wParam, lParam);

    pt.x = (INT32)LOWORD(lParam);
    pt.y = (INT32)HIWORD(lParam);
    nHit = TOOLBAR_InternalHitTest (wndPtr, &pt);

    if (infoPtr->bCaptured) {
	if (infoPtr->nOldHit != nHit) {
	    btnPtr = &infoPtr->buttons[infoPtr->nButtonDown];
	    if (infoPtr->nOldHit == infoPtr->nButtonDown) {
		btnPtr->fsState &= ~TBSTATE_PRESSED;
		hdc = GetDC32 (wndPtr->hwndSelf);
		TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
	    }
	    else if (nHit == infoPtr->nButtonDown) {
		btnPtr->fsState |= TBSTATE_PRESSED;
		hdc = GetDC32 (wndPtr->hwndSelf);
		TOOLBAR_DrawButton (wndPtr, btnPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
	    }
	}
	infoPtr->nOldHit = nHit;
    }

    return 0;
}


__inline__ static LRESULT
TOOLBAR_NCActivate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
/*    if (wndPtr->dwStyle & CCS_NODIVIDER) */
	return DefWindowProc32A (wndPtr->hwndSelf, WM_NCACTIVATE,
				 wParam, lParam);
/*    else */
/*	return TOOLBAR_NCPaint (wndPtr, wParam, lParam); */
}


__inline__ static LRESULT
TOOLBAR_NCCalcSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    if (!(wndPtr->dwStyle & CCS_NODIVIDER))
	((LPRECT32)lParam)->top += sysMetrics[SM_CYEDGE];

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCALCSIZE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (TOOLBAR_INFO *)COMCTL32_Alloc (sizeof(TOOLBAR_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (toolbar, "could not allocate info memory!\n");
	return 0;
    }

    if ((TOOLBAR_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (toolbar, "pointer assignment error!\n");
	return 0;
    }

    /* paranoid!! */
    infoPtr->dwStructSize = sizeof(TBBUTTON);

    /* fix instance handle, if the toolbar was created by CreateToolbarEx() */
    if (!wndPtr->hInstance)
	wndPtr->hInstance = wndPtr->parent->hInstance;

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}


static LRESULT
TOOLBAR_NCPaint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HWND32 hwnd = wndPtr->hwndSelf;
    HDC32 hdc;

    if ( wndPtr->dwStyle & WS_MINIMIZE ||
	!WIN_IsWindowDrawable( wndPtr, 0 )) return 0; /* Nothing to do */

    DefWindowProc32A (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx32( hwnd, 0, DCX_USESTYLE | DCX_WINDOW ))) return 0;

    if (ExcludeVisRect( hdc, wndPtr->rectClient.left-wndPtr->rectWindow.left,
		        wndPtr->rectClient.top-wndPtr->rectWindow.top,
		        wndPtr->rectClient.right-wndPtr->rectWindow.left,
		        wndPtr->rectClient.bottom-wndPtr->rectWindow.top )
	== NULLREGION){
	ReleaseDC32( hwnd, hdc );
	return 0;
    }

    if (!(wndPtr->flags & WIN_MANAGED) && !(wndPtr->dwStyle & CCS_NODIVIDER))
	DrawEdge32 (hdc, &wndPtr->rectWindow, EDGE_ETCHED, BF_TOP);

    ReleaseDC32( hwnd, hdc );

    return 0;
}


__inline__ static LRESULT
TOOLBAR_Notify (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    TRACE (toolbar, "passing WM_NOTIFY!\n");

    if ((infoPtr->hwndToolTip) && (lpnmh->hwndFrom == infoPtr->hwndToolTip)) {
	SendMessage32A (infoPtr->hwndNotify, WM_NOTIFY,	wParam, lParam);

#if 0
	if (lpnmh->code == TTN_GETDISPINFO32A) {
	    LPNMTTDISPINFO32A lpdi = (LPNMTTDISPINFO32A)lParam;

	    FIXME (toolbar, "retrieving ASCII string\n");

	}
	else if (lpnmh->code == TTN_GETDISPINFO32W) {
	    LPNMTTDISPINFO32W lpdi = (LPNMTTDISPINFO32W)lParam;

	    FIXME (toolbar, "retrieving UNICODE string\n");

	}
#endif
    }

    return 0;
}


static LRESULT
TOOLBAR_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TOOLBAR_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TOOLBAR_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TOOLBAR_INFO *infoPtr = TOOLBAR_GetInfoPtr(wndPtr);
    RECT32 parent_rect;
    HWND32 parent;
    /* INT32  x, y; */
    INT32  cx, cy;
    INT32  flags;
    UINT32 uPosFlags = 0;

    /* Resize deadlock check */
    if (infoPtr->bAutoSize) {
	infoPtr->bAutoSize = FALSE;
	return 0;
    }

    flags = (INT32) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED
     */

    TRACE (toolbar, "sizing toolbar!\n");

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent32 (wndPtr->hwndSelf);
	GetClientRect32(parent, &parent_rect);

	if (wndPtr->dwStyle & CCS_NORESIZE) {
	    uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

	    /* FIXME */
/*	    infoPtr->nWidth = parent_rect.right - parent_rect.left; */
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	    TOOLBAR_CalcToolbar (wndPtr);
	    infoPtr->nWidth = cx;
	    infoPtr->nHeight = cy;
	}
	else {
	    infoPtr->nWidth = parent_rect.right - parent_rect.left;
	    TOOLBAR_CalcToolbar (wndPtr);
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	}

	if (wndPtr->dwStyle & CCS_NOPARENTALIGN) {
	    uPosFlags |= SWP_NOMOVE;
	    cy = infoPtr->nHeight;
	    cx = infoPtr->nWidth;
	}

	if (!(wndPtr->dwStyle & CCS_NODIVIDER))
	    cy += sysMetrics[SM_CYEDGE];

	SetWindowPos32 (wndPtr->hwndSelf, 0, parent_rect.left, parent_rect.top,
			cx, cy, uPosFlags | SWP_NOZORDER);
    }
    return 0;
}


static LRESULT
TOOLBAR_StyleChanged (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;

    TOOLBAR_AutoSize (wndPtr, wParam, lParam);

    hdc = GetDC32 (wndPtr->hwndSelf);
    TOOLBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}



LRESULT WINAPI
ToolbarWindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
	case TB_ADDBITMAP:
	    return TOOLBAR_AddBitmap (wndPtr, wParam, lParam);

	case TB_ADDBUTTONS32A:
	    return TOOLBAR_AddButtons32A (wndPtr, wParam, lParam);

/*	case TB_ADDBUTTONS32W: */

	case TB_ADDSTRING32A:
	    return TOOLBAR_AddString32A (wndPtr, wParam, lParam);

	case TB_ADDSTRING32W:
	    return TOOLBAR_AddString32W (wndPtr, wParam, lParam);

	case TB_AUTOSIZE:
	    return TOOLBAR_AutoSize (wndPtr, wParam, lParam);

	case TB_BUTTONCOUNT:
	    return TOOLBAR_ButtonCount (wndPtr, wParam, lParam);

	case TB_BUTTONSTRUCTSIZE:
	    return TOOLBAR_ButtonStructSize (wndPtr, wParam, lParam);

	case TB_CHANGEBITMAP:
	    return TOOLBAR_ChangeBitmap (wndPtr, wParam, lParam);

	case TB_CHECKBUTTON:
	    return TOOLBAR_CheckButton (wndPtr, wParam, lParam);

	case TB_COMMANDTOINDEX:
	    return TOOLBAR_CommandToIndex (wndPtr, wParam, lParam);

	case TB_CUSTOMIZE:
	    return TOOLBAR_Customize (wndPtr);

	case TB_DELETEBUTTON:
	    return TOOLBAR_DeleteButton (wndPtr, wParam, lParam);

	case TB_ENABLEBUTTON:
	    return TOOLBAR_EnableButton (wndPtr, wParam, lParam);

/*	case TB_GETANCHORHIGHLIGHT:		*/ /* 4.71 */

	case TB_GETBITMAP:
	    return TOOLBAR_GetBitmap (wndPtr, wParam, lParam);

	case TB_GETBITMAPFLAGS:
	    return TOOLBAR_GetBitmapFlags (wndPtr, wParam, lParam);

	case TB_GETBUTTON:
	    return TOOLBAR_GetButton (wndPtr, wParam, lParam);

	case TB_GETBUTTONINFO32A:
	    return TOOLBAR_GetButtonInfo32A (wndPtr, wParam, lParam);

/*	case TB_GETBUTTONINFO32W:		*/ /* 4.71 */

	case TB_GETBUTTONSIZE:
	    return TOOLBAR_GetButtonSize (wndPtr);

	case TB_GETBUTTONTEXT32A:
	    return TOOLBAR_GetButtonText32A (wndPtr, wParam, lParam);

/*	case TB_GETBUTTONTEXT32W: */
/*	case TB_GETCOLORSCHEME:			*/ /* 4.71 */

	case TB_GETDISABLEDIMAGELIST:
	    return TOOLBAR_GetDisabledImageList (wndPtr, wParam, lParam);

	case TB_GETEXTENDEDSTYLE:
	    return TOOLBAR_GetExtendedStyle (wndPtr);

	case TB_GETHOTIMAGELIST:
	    return TOOLBAR_GetHotImageList (wndPtr, wParam, lParam);

/*	case TB_GETHOTITEM:			*/ /* 4.71 */

	case TB_GETIMAGELIST:
	    return TOOLBAR_GetImageList (wndPtr, wParam, lParam);

/*	case TB_GETINSERTMARK:			*/ /* 4.71 */
/*	case TB_GETINSERTMARKCOLOR:		*/ /* 4.71 */

	case TB_GETITEMRECT:
	    return TOOLBAR_GetItemRect (wndPtr, wParam, lParam);

	case TB_GETMAXSIZE:
	    return TOOLBAR_GetMaxSize (wndPtr, wParam, lParam);

/*	case TB_GETOBJECT:			*/ /* 4.71 */
/*	case TB_GETPADDING:			*/ /* 4.71 */

	case TB_GETRECT:
	    return TOOLBAR_GetRect (wndPtr, wParam, lParam);

	case TB_GETROWS:
	    return TOOLBAR_GetRows (wndPtr, wParam, lParam);

	case TB_GETSTATE:
	    return TOOLBAR_GetState (wndPtr, wParam, lParam);

	case TB_GETSTYLE:
	    return TOOLBAR_GetStyle (wndPtr, wParam, lParam);

	case TB_GETTEXTROWS:
	    return TOOLBAR_GetTextRows (wndPtr, wParam, lParam);

	case TB_GETTOOLTIPS:
	    return TOOLBAR_GetToolTips (wndPtr, wParam, lParam);

	case TB_GETUNICODEFORMAT:
	    return TOOLBAR_GetUnicodeFormat (wndPtr, wParam, lParam);

	case TB_HIDEBUTTON:
	    return TOOLBAR_HideButton (wndPtr, wParam, lParam);

	case TB_HITTEST:
	    return TOOLBAR_HitTest (wndPtr, wParam, lParam);

	case TB_INDETERMINATE:
	    return TOOLBAR_Indeterminate (wndPtr, wParam, lParam);

	case TB_INSERTBUTTON32A:
	    return TOOLBAR_InsertButton32A (wndPtr, wParam, lParam);

/*	case TB_INSERTBUTTON32W: */
/*	case TB_INSERTMARKHITTEST:		*/ /* 4.71 */

	case TB_ISBUTTONCHECKED:
	    return TOOLBAR_IsButtonChecked (wndPtr, wParam, lParam);

	case TB_ISBUTTONENABLED:
	    return TOOLBAR_IsButtonEnabled (wndPtr, wParam, lParam);

	case TB_ISBUTTONHIDDEN:
	    return TOOLBAR_IsButtonHidden (wndPtr, wParam, lParam);

	case TB_ISBUTTONHIGHLIGHTED:
	    return TOOLBAR_IsButtonHighlighted (wndPtr, wParam, lParam);

	case TB_ISBUTTONINDETERMINATE:
	    return TOOLBAR_IsButtonIndeterminate (wndPtr, wParam, lParam);

	case TB_ISBUTTONPRESSED:
	    return TOOLBAR_IsButtonPressed (wndPtr, wParam, lParam);

/*	case TB_LOADIMAGES:			*/ /* 4.70 */
/*	case TB_MAPACCELERATOR32A:		*/ /* 4.71 */
/*	case TB_MAPACCELERATOR32W:		*/ /* 4.71 */
/*	case TB_MARKBUTTON:			*/ /* 4.71 */
/*	case TB_MOVEBUTTON:			*/ /* 4.71 */

	case TB_PRESSBUTTON:
	    return TOOLBAR_PressButton (wndPtr, wParam, lParam);

/*	case TB_REPLACEBITMAP: */

	case TB_SAVERESTORE32A:
	    return TOOLBAR_SaveRestore32A (wndPtr, wParam, lParam);

/*	case TB_SAVERESTORE32W: */
/*	case TB_SETANCHORHIGHLIGHT:		*/ /* 4.71 */

	case TB_SETBITMAPSIZE:
	    return TOOLBAR_SetBitmapSize (wndPtr, wParam, lParam);

	case TB_SETBUTTONINFO32A:
	    return TOOLBAR_SetButtonInfo32A (wndPtr, wParam, lParam);

/*	case TB_SETBUTTONINFO32W:		*/ /* 4.71 */

	case TB_SETBUTTONSIZE:
	    return TOOLBAR_SetButtonSize (wndPtr, wParam, lParam);

	case TB_SETBUTTONWIDTH:
	    return TOOLBAR_SetButtonWidth (wndPtr, wParam, lParam);

	case TB_SETCMDID:
	    return TOOLBAR_SetCmdId (wndPtr, wParam, lParam);

/*	case TB_SETCOLORSCHEME:			*/ /* 4.71 */

	case TB_SETDISABLEDIMAGELIST:
	    return TOOLBAR_SetDisabledImageList (wndPtr, wParam, lParam);

	case TB_SETDRAWTEXTFLAGS:
	    return TOOLBAR_SetDrawTextFlags (wndPtr, wParam, lParam);

	case TB_SETEXTENDEDSTYLE:
	    return TOOLBAR_SetExtendedStyle (wndPtr, wParam, lParam);

	case TB_SETHOTIMAGELIST:
	    return TOOLBAR_SetHotImageList (wndPtr, wParam, lParam);

/*	case TB_SETHOTITEM:			*/ /* 4.71 */

	case TB_SETIMAGELIST:
	    return TOOLBAR_SetImageList (wndPtr, wParam, lParam);

	case TB_SETINDENT:
	    return TOOLBAR_SetIndent (wndPtr, wParam, lParam);

/*	case TB_SETINSERTMARK:			*/ /* 4.71 */

	case TB_SETINSERTMARKCOLOR:
	    return TOOLBAR_SetInsertMarkColor (wndPtr, wParam, lParam);

	case TB_SETMAXTEXTROWS:
	    return TOOLBAR_SetMaxTextRows (wndPtr, wParam, lParam);

/*	case TB_SETPADDING:			*/ /* 4.71 */

	case TB_SETPARENT:
	    return TOOLBAR_SetParent (wndPtr, wParam, lParam);

	case TB_SETROWS:
	    return TOOLBAR_SetRows (wndPtr, wParam, lParam);

	case TB_SETSTATE:
	    return TOOLBAR_SetState (wndPtr, wParam, lParam);

	case TB_SETSTYLE:
	    return TOOLBAR_SetStyle (wndPtr, wParam, lParam);

	case TB_SETTOOLTIPS:
	    return TOOLBAR_SetToolTips (wndPtr, wParam, lParam);

	case TB_SETUNICODEFORMAT:
	    return TOOLBAR_SetUnicodeFormat (wndPtr, wParam, lParam);


/*	case WM_CHAR: */

	case WM_CREATE:
	    return TOOLBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TOOLBAR_Destroy (wndPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return TOOLBAR_EraseBackground (wndPtr, wParam, lParam);

/*	case WM_GETFONT: */
/*	case WM_KEYDOWN: */
/*	case WM_KILLFOCUS: */

	case WM_LBUTTONDBLCLK:
	    return TOOLBAR_LButtonDblClk (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TOOLBAR_LButtonDown (wndPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return TOOLBAR_LButtonUp (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TOOLBAR_MouseMove (wndPtr, wParam, lParam);

	case WM_NCACTIVATE:
	    return TOOLBAR_NCActivate (wndPtr, wParam, lParam);

	case WM_NCCALCSIZE:
	    return TOOLBAR_NCCalcSize (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return TOOLBAR_NCCreate (wndPtr, wParam, lParam);

	case WM_NCPAINT:
	    return TOOLBAR_NCPaint (wndPtr, wParam, lParam);

	case WM_NOTIFY:
	    return TOOLBAR_Notify (wndPtr, wParam, lParam);

/*	case WM_NOTIFYFORMAT: */

	case WM_PAINT:
	    return TOOLBAR_Paint (wndPtr, wParam);

	case WM_SIZE:
	    return TOOLBAR_Size (wndPtr, wParam, lParam);

	case WM_STYLECHANGED:
	    return TOOLBAR_StyleChanged (wndPtr, wParam, lParam);

/*	case WM_SYSCOLORCHANGE: */

/*	case WM_WININICHANGE: */

	case WM_CHARTOITEM:
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_MEASUREITEM:
	case WM_VKEYTOITEM:
	    return SendMessage32A (GetParent32 (hwnd), uMsg, wParam, lParam);

	default:
	    if (uMsg >= WM_USER)
		ERR (toolbar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TOOLBAR_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TOOLBARCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)ToolbarWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TOOLBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TOOLBARCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}


VOID
TOOLBAR_Unregister (VOID)
{
    if (GlobalFindAtom32A (TOOLBARCLASSNAME32A))
	UnregisterClass32A (TOOLBARCLASSNAME32A, (HINSTANCE32)NULL);
}

