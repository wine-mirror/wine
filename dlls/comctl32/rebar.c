/*
 * Rebar control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - vertical placement
 *   - ComboBox and ComboBoxEx placement
 *   - center image 
 *   - Layout code.
 *   - Display code.
 *   - Some messages.
 *   - All notifications.
 */

#include "windows.h"
#include "commctrl.h"
#include "rebar.h"
#include "win.h"
#include "sysmetrics.h"
#include "debug.h"


/* fDraw flags */
#define DRAW_GRIPPER    1
#define DRAW_IMAGE      2
#define DRAW_TEXT       4
#define DRAW_CHILD      8

#define GRIPPER_WIDTH   13


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)wndPtr->wExtra[0])


static VOID
REBAR_DrawBand (HDC32 hdc, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{

    DrawEdge32 (hdc, &lpBand->rcBand, BDR_RAISEDINNER, BF_MIDDLE);

    /* draw background */

    /* draw gripper */
    if (lpBand->fDraw & DRAW_GRIPPER)
	DrawEdge32 (hdc, &lpBand->rcGripper, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);

    /* draw caption image */
    if (lpBand->fDraw & DRAW_IMAGE) {
	/* FIXME: center image */
	POINT32 pt;

	pt.y = (lpBand->rcCapImage.bottom + lpBand->rcCapImage.top - infoPtr->imageSize.cy)/2;
	pt.x = (lpBand->rcCapImage.right + lpBand->rcCapImage.left - infoPtr->imageSize.cx)/2;

	ImageList_Draw (infoPtr->himl, lpBand->iImage, hdc,
//			lpBand->rcCapImage.left, lpBand->rcCapImage.top,
			pt.x, pt.y,
			ILD_TRANSPARENT);
    }

    /* draw caption text */
    if (lpBand->fDraw & DRAW_TEXT) {
	HFONT32 hOldFont = SelectObject32 (hdc, infoPtr->hFont);
	INT32 oldBkMode = SetBkMode32 (hdc, TRANSPARENT);
	DrawText32W (hdc, lpBand->lpText, -1, &lpBand->rcCapText,
		     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (oldBkMode != TRANSPARENT)
	    SetBkMode32 (hdc, oldBkMode);
	SelectObject32 (hdc, hOldFont);
    }
}


static VOID
REBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    REBAR_BAND *lpBand;
    UINT32 i;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((wndPtr->dwStyle & CCS_VERT) && (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	REBAR_DrawBand (hdc, infoPtr, lpBand);

    }
}


static VOID
REBAR_CalcHorzBand (REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    lpBand->fDraw = 0;

    /* set initial caption image rectangle */
    SetRect32 (&lpBand->rcCapImage, 0, 0, 0, 0);

    /* image is visible */
    if ((lpBand->iImage > -1) && (infoPtr->himl)) {
	lpBand->fDraw |= DRAW_IMAGE;

	lpBand->rcCapImage.right  = lpBand->rcCapImage.left + infoPtr->imageSize.cx;
	lpBand->rcCapImage.bottom = lpBand->rcCapImage.top + infoPtr->imageSize.cy;

	/* update band height */
	if (lpBand->uMinHeight < infoPtr->imageSize.cy + 2) {
	    lpBand->uMinHeight = infoPtr->imageSize.cy + 2;
	    lpBand->rcBand.bottom = lpBand->rcBand.top + lpBand->uMinHeight;
	}
    }

    /* set initial caption text rectangle */
    lpBand->rcCapText.left   = lpBand->rcCapImage.right;
    lpBand->rcCapText.top    = lpBand->rcBand.top + 1;
    lpBand->rcCapText.right  = lpBand->rcCapText.left;
    lpBand->rcCapText.bottom = lpBand->rcBand.bottom - 1;

    /* text is visible */
    if (lpBand->lpText) {
	HDC32 hdc = GetDC32 (0);
	HFONT32 hOldFont = SelectObject32 (hdc, infoPtr->hFont);
	SIZE32 size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlen32W (lpBand->lpText), &size);
	lpBand->rcCapText.right += size.cx;

	SelectObject32 (hdc, hOldFont);
	ReleaseDC32 (0, hdc);
    }

    /* set initial child window rectangle */
    if (lpBand->fStyle & RBBS_FIXEDSIZE) {
	lpBand->rcChild.left   = lpBand->rcCapText.right;
	lpBand->rcChild.top    = lpBand->rcBand.top;
	lpBand->rcChild.right  = lpBand->rcBand.right;
	lpBand->rcChild.bottom = lpBand->rcBand.bottom;
    }
    else {
	lpBand->rcChild.left   = lpBand->rcCapText.right + 4;
	lpBand->rcChild.top    = lpBand->rcBand.top + 2;
	lpBand->rcChild.right  = lpBand->rcBand.right - 4;
	lpBand->rcChild.bottom = lpBand->rcBand.bottom - 2;
    }

    /* calculate gripper rectangle */
    if ((!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	(!(lpBand->fStyle & RBBS_FIXEDSIZE)) &&
	((lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	 (infoPtr->uNumBands > 1))) {
	lpBand->fDraw |= DRAW_GRIPPER;
	lpBand->rcGripper.left   = lpBand->rcBand.left + 3;
	lpBand->rcGripper.right  = lpBand->rcGripper.left + 3;
	lpBand->rcGripper.top    = lpBand->rcBand.top + 3;
	lpBand->rcGripper.bottom = lpBand->rcBand.bottom - 3;

	/* move caption rectangles */
	OffsetRect32 (&lpBand->rcCapImage, GRIPPER_WIDTH, 0);
	OffsetRect32 (&lpBand->rcCapText, GRIPPER_WIDTH, 0);

	/* adjust child rectangle */
	lpBand->rcChild.left += GRIPPER_WIDTH;
    }


}


static VOID
REBAR_CalcVertBand (WND *wndPtr, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    lpBand->fDraw = 0;

    /* set initial caption image rectangle */
    SetRect32 (&lpBand->rcCapImage, 0, 0, 0, 0);

    /* image is visible */
    if ((lpBand->iImage > -1) && (infoPtr->himl)) {
	lpBand->fDraw |= DRAW_IMAGE;

	lpBand->rcCapImage.right  = lpBand->rcCapImage.left + infoPtr->imageSize.cx;
	lpBand->rcCapImage.bottom = lpBand->rcCapImage.top + infoPtr->imageSize.cy;

	/* update band width */
	if (lpBand->uMinHeight < infoPtr->imageSize.cx + 2) {
	    lpBand->uMinHeight = infoPtr->imageSize.cx + 2;
	    lpBand->rcBand.right = lpBand->rcBand.left + lpBand->uMinHeight;
	}
    }

    /* set initial caption text rectangle */
    lpBand->rcCapText.left   = lpBand->rcBand.left + 1;
    lpBand->rcCapText.top    = lpBand->rcCapImage.bottom;
    lpBand->rcCapText.right  = lpBand->rcBand.right - 1;
    lpBand->rcCapText.bottom = lpBand->rcCapText.top;

    /* text is visible */
    if (lpBand->lpText) {
	HDC32 hdc = GetDC32 (0);
	HFONT32 hOldFont = SelectObject32 (hdc, infoPtr->hFont);
	SIZE32 size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlen32W (lpBand->lpText), &size);
//	lpBand->rcCapText.right += size.cx;
	lpBand->rcCapText.bottom += size.cy;

	SelectObject32 (hdc, hOldFont);
	ReleaseDC32 (0, hdc);
    }

    /* set initial child window rectangle */
    if (lpBand->fStyle & RBBS_FIXEDSIZE) {
	lpBand->rcChild.left   = lpBand->rcBand.left;
	lpBand->rcChild.top    = lpBand->rcCapText.bottom;
	lpBand->rcChild.right  = lpBand->rcBand.right;
	lpBand->rcChild.bottom = lpBand->rcBand.bottom;
    }
    else {
	lpBand->rcChild.left   = lpBand->rcBand.left + 2;
	lpBand->rcChild.top    = lpBand->rcCapText.bottom + 4;
	lpBand->rcChild.right  = lpBand->rcBand.right - 2;
	lpBand->rcChild.bottom = lpBand->rcBand.bottom - 4;
    }

    /* calculate gripper rectangle */
    if ((!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	(!(lpBand->fStyle & RBBS_FIXEDSIZE)) &&
	((lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	 (infoPtr->uNumBands > 1))) {
	lpBand->fDraw |= DRAW_GRIPPER;

	if (wndPtr->dwStyle & RBS_VERTICALGRIPPER) {
	    /* adjust band width */
	    lpBand->rcBand.right += GRIPPER_WIDTH;
	    lpBand->uMinHeight += GRIPPER_WIDTH;

	    lpBand->rcGripper.left   = lpBand->rcBand.left + 3;
	    lpBand->rcGripper.right  = lpBand->rcGripper.left + 3;
	    lpBand->rcGripper.top    = lpBand->rcBand.top + 3;
	    lpBand->rcGripper.bottom = lpBand->rcBand.bottom - 3;

	    /* move caption rectangles */
	    OffsetRect32 (&lpBand->rcCapImage, GRIPPER_WIDTH, 0);
	    OffsetRect32 (&lpBand->rcCapText, GRIPPER_WIDTH, 0);
 
	    /* adjust child rectangle */
	    lpBand->rcChild.left += GRIPPER_WIDTH;
	}
	else {
	    lpBand->rcGripper.left   = lpBand->rcBand.left + 3;
	    lpBand->rcGripper.right  = lpBand->rcBand.right - 3;
	    lpBand->rcGripper.top    = lpBand->rcBand.top + 3;
	    lpBand->rcGripper.bottom = lpBand->rcGripper.top + 3;

	    /* move caption rectangles */
	    OffsetRect32 (&lpBand->rcCapImage, 0, GRIPPER_WIDTH);
	    OffsetRect32 (&lpBand->rcCapText, 0, GRIPPER_WIDTH);
 
	    /* adjust child rectangle */
	    lpBand->rcChild.top += GRIPPER_WIDTH;
	}
    }
}


static VOID
REBAR_Layout (WND *wndPtr, LPRECT32 lpRect)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    REBAR_BAND *lpBand;
    RECT32 rcClient;
    INT32 x, y, cx, cy;
    UINT32 i;

    if (lpRect)
	rcClient = *lpRect;
    else
	GetClientRect32 (wndPtr->hwndSelf, &rcClient);

    x = 0;
    y = 0;

    if (wndPtr->dwStyle & CCS_VERT) {
	cx = 20;    /* FIXME: fixed height */
	cy = rcClient.bottom - rcClient.top;
    }
    else {
	cx = rcClient.right - rcClient.left;
	cy = 20;    /* FIXME: fixed height */
    }

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((wndPtr->dwStyle & CCS_VERT) && (lpBand->fStyle & RBBS_NOVERT)))
	    continue;


	if (wndPtr->dwStyle & CCS_VERT) {
	    if (lpBand->fStyle & RBBS_VARIABLEHEIGHT)
		cx = lpBand->cyMaxChild;
	    else if (lpBand->fStyle & RBBIM_CHILDSIZE)
		cx = lpBand->cyMinChild;
	    else
		cx = 20; /* FIXME */

	    lpBand->rcBand.left   = x;
	    lpBand->rcBand.right  = x + cx;
	    lpBand->rcBand.top    = y;
	    lpBand->rcBand.bottom = y + cy;
	    lpBand->uMinHeight = cx;
	}
	else {
	    if (lpBand->fStyle & RBBS_VARIABLEHEIGHT)
		cy = lpBand->cyMaxChild;
	    else if (lpBand->fStyle & RBBIM_CHILDSIZE)
		cy = lpBand->cyMinChild;
	    else
		cy = 20; /* FIXME */

	    lpBand->rcBand.left   = x;
	    lpBand->rcBand.right  = x + cx;
	    lpBand->rcBand.top    = y;
	    lpBand->rcBand.bottom = y + cy;
	    lpBand->uMinHeight = cy;
	}

	if (wndPtr->dwStyle & CCS_VERT) {
	    REBAR_CalcVertBand (wndPtr, infoPtr, lpBand);
	    x += lpBand->uMinHeight;
	}
	else {
	    REBAR_CalcHorzBand (infoPtr, lpBand);
	    y += lpBand->uMinHeight;
	}
    }

    if (wndPtr->dwStyle & CCS_VERT) {
	infoPtr->calcSize.cx = x;
	infoPtr->calcSize.cy = rcClient.bottom - rcClient.top;
    }
    else {
	infoPtr->calcSize.cx = rcClient.right - rcClient.left;
	infoPtr->calcSize.cy = y;
    }
}


static VOID
REBAR_ForceResize (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    RECT32 rc;

    TRACE (rebar, " to [%d x %d]!\n",
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy);

    infoPtr->bAutoResize = TRUE;

    rc.left = 0;
    rc.top = 0;
    rc.right  = infoPtr->calcSize.cx;
    rc.bottom = infoPtr->calcSize.cy;

    if (wndPtr->dwStyle & WS_BORDER) {
	InflateRect32 (&rc, sysMetrics[SM_CXEDGE], sysMetrics[SM_CYEDGE]);
    }

    SetWindowPos32 (wndPtr->hwndSelf, 0, 0, 0,
		    rc.right - rc.left, rc.bottom - rc.top,
		    SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}


static VOID
REBAR_MoveChildWindows (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    REBAR_BAND *lpBand;
    UINT32 i;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if (lpBand->fStyle & RBBS_HIDDEN)
	    continue;
	if (lpBand->hwndChild) {
	    TRACE (rebar, "hwndChild = %x\n", lpBand->hwndChild);

	    if (WIDGETS_IsControl32 (WIN_FindWndPtr(lpBand->hwndChild), BIC32_COMBO)) {
		INT32 nEditHeight, yPos;
		RECT32 rc;

		/* special placement code for combo box */


		/* get size of edit line */
		GetWindowRect32 (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		SetWindowPos32 (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, /*lpBand->rcChild.top*/ yPos,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    nEditHeight,
			    SWP_SHOWWINDOW);
	    }
#if 0
	    else if () {
		/* special placement code for extended combo box */


	    }
#endif
	    else {
		SetWindowPos32 (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, lpBand->rcChild.top,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    lpBand->rcChild.bottom - lpBand->rcChild.top,
			    SWP_SHOWWINDOW);
	    }
	}
    }
}


static void
REBAR_InternalHitTest (WND *wndPtr, LPPOINT32 lpPt, UINT32 *pFlags, INT32 *pBand)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);
    REBAR_BAND *lpBand;
    RECT32 rect;
    INT32  iCount;

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    *pFlags = RBHT_NOWHERE;
    if (PtInRect32 (&rect, *lpPt))
    {
	if (infoPtr->uNumBands == 0) {
	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;
	    TRACE (rebar, "NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    for (iCount = 0; iCount < infoPtr->uNumBands; iCount++) {
		lpBand = &infoPtr->bands[iCount];
		if (PtInRect32 (&lpBand->rcBand, *lpPt)) {
		    if (pBand)
			*pBand = iCount;
		    if (PtInRect32 (&lpBand->rcGripper, *lpPt)) {
			*pFlags = RBHT_GRABBER;
			TRACE (rebar, "ON GRABBER %d\n", iCount);
			return;
		    }
		    else if (PtInRect32 (&lpBand->rcCapImage, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE (rebar, "ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect32 (&lpBand->rcCapText, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE (rebar, "ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect32 (&lpBand->rcChild, *lpPt)) {
			*pFlags = RBHT_CLIENT;
			TRACE (rebar, "ON CLIENT %d\n", iCount);
			return;
		    }
		    else {
			*pFlags = RBHT_NOWHERE;
			TRACE (rebar, "NOWHERE %d\n", iCount);
			return;
		    }
		}
	    }

	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;

	    TRACE (rebar, "NOWHERE\n");
	    return;
	}
    }
    else {
	*pFlags = RBHT_NOWHERE;
	if (pBand)
	    *pBand = -1;
	TRACE (rebar, "NOWHERE\n");
	return;
    }

    TRACE (rebar, "flags=0x%X\n", *pFlags);
    return;
}



// << REBAR_BeginDrag >>


static LRESULT
REBAR_DeleteBand (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    UINT32 uBand = (UINT32)wParam;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    FIXME (rebar, "deleting band %u!\n", uBand);

    if (infoPtr->uNumBands == 1) {
	TRACE (rebar, " simple delete!\n");
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
	infoPtr->uNumBands = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
        TRACE(rebar, "complex delete! [uBand=%u]\n", uBand);

	infoPtr->uNumBands--;
	infoPtr->bands = COMCTL32_Alloc (sizeof (REBAR_BAND) * infoPtr->uNumBands);
        if (uBand > 0) {
            memcpy (&infoPtr->bands[0], &oldBands[0],
                    uBand * sizeof(REBAR_BAND));
        }

        if (uBand < infoPtr->uNumBands) {
            memcpy (&infoPtr->bands[uBand], &oldBands[uBand+1],
                    (infoPtr->uNumBands - uBand) * sizeof(REBAR_BAND));
        }

	COMCTL32_Free (oldBands);
    }

    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


// << REBAR_DragMove >>
// << REBAR_EndDrag >>


static LRESULT
REBAR_GetBandBorders (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    return 0;
}


__inline__ static LRESULT
REBAR_GetBandCount (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    TRACE (rebar, "band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32A lprbbi = (LPREBARBANDINFO32A)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32A)
	return FALSE;
    if ((UINT32)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE (rebar, "index %u\n", (UINT32)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT32)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && 
	(lprbbi->lpText) && (lpBand->lpText)) {
	    lstrcpynWtoA (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	lprbbi->cyMaxChild = lpBand->cyMaxChild;
	lprbbi->cyChild    = lpBand->cyChild;
	lprbbi->cyIntegral = lpBand->cyIntegral;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lprbbi->cx = lpBand->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lprbbi->hbmBack = lpBand->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lprbbi->wID = lpBand->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32A)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    return TRUE;
}


static LRESULT
REBAR_GetBandInfo32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32W lprbbi = (LPREBARBANDINFO32W)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32W)
	return FALSE;
    if ((UINT32)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE (rebar, "index %u\n", (UINT32)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT32)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && 
	(lprbbi->lpText) && (lpBand->lpText)) {
	    lstrcpyn32W (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	lprbbi->cyMaxChild = lpBand->cyMaxChild;
	lprbbi->cyChild    = lpBand->cyChild;
	lprbbi->cyIntegral = lpBand->cyIntegral;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lprbbi->cx = lpBand->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lprbbi->hbmBack = lpBand->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lprbbi->wID = lpBand->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32A)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    return TRUE;
}


static LRESULT
REBAR_GetBarHeight (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    INT32 nHeight;

    REBAR_Layout (wndPtr, NULL);
    nHeight = infoPtr->calcSize.cy;

    if (wndPtr->dwStyle & WS_BORDER)
	nHeight += (2 * sysMetrics[SM_CYEDGE]);

    FIXME (rebar, "height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE (rebar, "getting bar info!\n");

    if (infoPtr->himl) {
	lpInfo->himl = infoPtr->himl;
	lpInfo->fMask |= RBIM_IMAGELIST;
    }

    return TRUE;
}


__inline__ static LRESULT
REBAR_GetBkColor (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    TRACE (rebar, "background color 0x%06lx!\n", infoPtr->clrBk);

    return infoPtr->clrBk;
}


// << REBAR_GetColorScheme >>
// << REBAR_GetDropTarget >>


static LRESULT
REBAR_GetPalette (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    FIXME (rebar, "empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    INT32 iBand = (INT32)wParam;
    LPRECT32 lprc = (LPRECT32)lParam;
    REBAR_BAND *lpBand;

    if ((iBand < 0) && ((UINT32)iBand >= infoPtr->uNumBands))
	return FALSE;
    if (!lprc)
	return FALSE;

    TRACE (rebar, "band %d\n", iBand);

    lpBand = &infoPtr->bands[iBand];
    CopyRect32 (lprc, &lpBand->rcBand);
/*
    lprc->left   = lpBand->rcBand.left;
    lprc->top    = lpBand->rcBand.top;
    lprc->right  = lpBand->rcBand.right;
    lprc->bottom = lpBand->rcBand.bottom;
*/

    return TRUE;
}


__inline__ static LRESULT
REBAR_GetRowCount (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    FIXME (rebar, "%u : semi stub!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetRowHeight (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
//    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    FIXME (rebar, "-- height = 20: semi stub!\n");

    return 20;
}


__inline__ static LRESULT
REBAR_GetTextColor (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    TRACE (rebar, "text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


__inline__ static LRESULT
REBAR_GetToolTips (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    return infoPtr->hwndToolTip;
}


__inline__ static LRESULT
REBAR_GetUnicodeFormat (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    return infoPtr->bUnicode;
}


static LRESULT
REBAR_HitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam; 

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (wndPtr, &lprbht->pt,
			   &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    UINT32 i;

    if (infoPtr == NULL)
	return -1;

    if (infoPtr->uNumBands < 1)
	return -1;

    TRACE (rebar, "id %u\n", (UINT32)wParam);

    for (i = 0; i < infoPtr->uNumBands; i++) {
	if (infoPtr->bands[i].wID == (UINT32)wParam) {
	    TRACE (rebar, "band %u found!\n", i);
	    return i;
	}
    }

    TRACE (rebar, "no band found!\n");
    return -1;
}


static LRESULT
REBAR_InsertBand32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32A lprbbi = (LPREBARBANDINFO32A)lParam;
    UINT32 uIndex = (UINT32)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32A)
	return FALSE;

    TRACE (rebar, "insert band at %u!\n", uIndex);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT32)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex < infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex - 1) * sizeof(REBAR_BAND));
	}

	COMCTL32_Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE (rebar, "index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];

    if (lprbbi->fMask & RBBIM_STYLE)
	lpBand->fStyle = lprbbi->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
    }
    else {
	lpBand->clrFore = CLR_NONE;
	lpBand->clrBack = CLR_NONE;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT32 len = lstrlen32A (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;
    else
	lpBand->iImage = -1;

    if (lprbbi->fMask & RBBIM_CHILD) {
	TRACE (rebar, "hwndChild = %x\n", lprbbi->hwndChild);
	lpBand->hwndChild = lprbbi->hwndChild;
	lpBand->hwndPrevParent =
	    SetParent32 (lpBand->hwndChild, wndPtr->hwndSelf);
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
	lpBand->cyMaxChild = lprbbi->cyMaxChild;
	lpBand->cyChild    = lprbbi->cyChild;
	lpBand->cyIntegral = lprbbi->cyIntegral;
    }
    else {
	lpBand->cxMinChild = -1;
	lpBand->cyMinChild = -1;
	lpBand->cyMaxChild = -1;
	lpBand->cyChild    = -1;
	lpBand->cyIntegral = -1;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lpBand->cx = lprbbi->cx;
    else
	lpBand->cx = -1;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lpBand->hbmBack = lprbbi->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lpBand->wID = lprbbi->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32A)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


static LRESULT
REBAR_InsertBand32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32W lprbbi = (LPREBARBANDINFO32W)lParam;
    UINT32 uIndex = (UINT32)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32W)
	return FALSE;

    TRACE (rebar, "insert band at %u!\n", uIndex);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT32)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex < infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex - 1) * sizeof(REBAR_BAND));
	}

	COMCTL32_Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE (rebar, "index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];

    if (lprbbi->fMask & RBBIM_STYLE)
	lpBand->fStyle = lprbbi->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
    }
    else {
	lpBand->clrFore = CLR_NONE;
	lpBand->clrBack = CLR_NONE;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT32 len = lstrlen32W (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpy32W (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;
    else
	lpBand->iImage = -1;

    if (lprbbi->fMask & RBBIM_CHILD) {
	TRACE (rebar, "hwndChild = %x\n", lprbbi->hwndChild);
	lpBand->hwndChild = lprbbi->hwndChild;
	lpBand->hwndPrevParent =
	    SetParent32 (lpBand->hwndChild, wndPtr->hwndSelf);
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
	lpBand->cyMaxChild = lprbbi->cyMaxChild;
	lpBand->cyChild    = lprbbi->cyChild;
	lpBand->cyIntegral = lprbbi->cyIntegral;
    }
    else {
	lpBand->cxMinChild = -1;
	lpBand->cyMinChild = -1;
	lpBand->cyMaxChild = -1;
	lpBand->cyChild    = -1;
	lpBand->cyIntegral = -1;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lpBand->cx = lprbbi->cx;
    else
	lpBand->cx = -1;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lpBand->hbmBack = lprbbi->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lpBand->wID = lprbbi->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32W)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


// << REBAR_MaximizeBand >>
// << REBAR_MinimizeBand >>
// << REBAR_MoveBand >>


static LRESULT
REBAR_SetBandInfo32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32A lprbbi = (LPREBARBANDINFO32A)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32A)
	return FALSE;
    if ((UINT32)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE (rebar, "index %u\n", (UINT32)wParam);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT32)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lpBand->fStyle = lprbbi->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
    }

    if (lprbbi->fMask & RBBIM_TEXT) {
	if (lpBand->lpText) {
	    COMCTL32_Free (lpBand->lpText);
	    lpBand->lpText = NULL;
	}
	if (lprbbi->lpText) {
	    INT32 len = lstrlen32A (lprbbi->lpText);
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
	if (lprbbi->hwndChild) {
	    lpBand->hwndChild = lprbbi->hwndChild;
	    lpBand->hwndPrevParent =
		SetParent32 (lpBand->hwndChild, wndPtr->hwndSelf);
	}
	else {
	    FIXME (rebar, "child: 0x%x  prev parent: 0x%x\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
//	    SetParent32 (lpBand->hwndChild, lpBand->hwndPrevParent);
	    lpBand->hwndChild = 0;
	    lpBand->hwndPrevParent = 0;
	}
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
	lpBand->cyMaxChild = lprbbi->cyMaxChild;
	lpBand->cyChild    = lprbbi->cyChild;
	lpBand->cyIntegral = lprbbi->cyIntegral;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lpBand->cx = lprbbi->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lpBand->hbmBack = lprbbi->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lpBand->wID = lprbbi->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32A)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


static LRESULT
REBAR_SetBandInfo32W (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARBANDINFO32W lprbbi = (LPREBARBANDINFO32W)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZE32W)
	return FALSE;
    if ((UINT32)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE (rebar, "index %u\n", (UINT32)wParam);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT32)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lpBand->fStyle = lprbbi->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
    }

    if (lprbbi->fMask & RBBIM_TEXT) {
	if (lpBand->lpText) {
	    COMCTL32_Free (lpBand->lpText);
	    lpBand->lpText = NULL;
	}
	if (lprbbi->lpText) {
	    INT32 len = lstrlen32W (lprbbi->lpText);
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpy32W (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
	if (lprbbi->hwndChild) {
	    lpBand->hwndChild = lprbbi->hwndChild;
	    lpBand->hwndPrevParent =
		SetParent32 (lpBand->hwndChild, wndPtr->hwndSelf);
	}
	else {
	    FIXME (rebar, "child: 0x%x  prev parent: 0x%x\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
//		SetParent32 (lpBand->hwndChild, lpBand->hwndPrevParent);
	    lpBand->hwndChild = 0;
	    lpBand->hwndPrevParent = 0;
	}
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
	lpBand->cyMaxChild = lprbbi->cyMaxChild;
	lpBand->cyChild    = lprbbi->cyChild;
	lpBand->cyIntegral = lprbbi->cyIntegral;
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lpBand->cx = lprbbi->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lpBand->hbmBack = lprbbi->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lpBand->wID = lprbbi->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFO32W)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE (rebar, "setting bar info!\n");

    if (lpInfo->fMask & RBIM_IMAGELIST) {
	infoPtr->himl = lpInfo->himl;
	if (infoPtr->himl) {
	    ImageList_GetIconSize (infoPtr->himl, &infoPtr->imageSize.cx,
				   &infoPtr->imageSize.cy);
	}
	else {
	    infoPtr->imageSize.cx = 0;
	    infoPtr->imageSize.cy = 0;
	}
    }

    return TRUE;
}


static LRESULT
REBAR_SetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;

    TRACE (rebar, "background color 0x%06lx!\n", infoPtr->clrBk);

    return clrTemp;
}


// << REBAR_SetColorScheme >>
// << REBAR_SetPalette >>


static LRESULT
REBAR_SetParent (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    HWND32 hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = (HWND32)wParam;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = (COLORREF)lParam;

    TRACE (rebar, "text color 0x%06lx!\n", infoPtr->clrText);

    return clrTemp;
}


// << REBAR_SetTooltips >>


__inline__ static LRESULT
REBAR_SetUnicodeFormat (WND *wndPtr, WPARAM32 wParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    BOOL32 bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL32)wParam;
    return bTemp;
}


static LRESULT
REBAR_ShowBand (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    REBAR_BAND *lpBand;

    if (((INT32)wParam < 0) || ((INT32)wParam > infoPtr->uNumBands))
	return FALSE;

    lpBand = &infoPtr->bands[(INT32)wParam];

    if ((BOOL32)lParam) {
	TRACE (rebar, "show band %d\n", (INT32)wParam);
	lpBand->fStyle = lpBand->fStyle & ~RBBS_HIDDEN;
	if (IsWindow32 (lpBand->hwndChild))
	    ShowWindow32 (lpBand->hwndChild, SW_SHOW);
    }
    else {
	TRACE (rebar, "hide band %d\n", (INT32)wParam);
	lpBand->fStyle = lpBand->fStyle | RBBS_HIDDEN;
	if (IsWindow32 (lpBand->hwndChild))
	    ShowWindow32 (lpBand->hwndChild, SW_SHOW);
    }

    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);
    LPRECT32 lpRect = (LPRECT32)lParam;

    if (lpRect == NULL)
	return FALSE;

    FIXME (rebar, "layout change not implemented!\n");
    FIXME (rebar, "[%d %d %d %d]\n",
	   lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

#if 0
    SetWindowPos32 (wndPtr->hwndSelf, 0, lpRect->left, lpRect->top,
		    lpRect->right - lpRect->left, lpRect->bottom - lpRect->top,
		    SWP_NOZORDER);
#endif

    infoPtr->calcSize.cx = lpRect->right - lpRect->left;
    infoPtr->calcSize.cy = lpRect->bottom - lpRect->top;

    REBAR_ForceResize (wndPtr);
    return TRUE;
}



static LRESULT
REBAR_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)COMCTL32_Alloc (sizeof(REBAR_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;

    if (infoPtr == NULL) {
	ERR (rebar, "could not allocate info memory!\n");
	return 0;
    }

    if ((REBAR_INFO*)wndPtr->wExtra[0] != infoPtr) {
	ERR (rebar, "pointer assignment error!\n");
	return 0;
    }

    /* initialize info structure */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = RGB(0, 0, 0);

    infoPtr->bAutoResize = FALSE;
    infoPtr->hcurArrow = LoadCursor32A (0, IDC_ARROW32A);
    infoPtr->hcurHorz  = LoadCursor32A (0, IDC_SIZEWE32A);
    infoPtr->hcurVert  = LoadCursor32A (0, IDC_SIZENS32A);
    infoPtr->hcurDrag  = LoadCursor32A (0, IDC_SIZE32A);

    infoPtr->bUnicode = IsWindowUnicode (wndPtr->hwndSelf);

    if (wndPtr->dwStyle & RBS_AUTOSIZE)
	FIXME (rebar, "style RBS_AUTOSIZE set!\n");

#if 0
    SendMessage32A (wndPtr->parent->hwndSelf, WM_NOTIFYFORMAT,
		    (WPARAM32)wndPtr->hwndSelf, NF_QUERY);
#endif

    TRACE (rebar, "created!\n");
    return 0;
}


static LRESULT
REBAR_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);
    REBAR_BAND *lpBand;
    INT32 i;


    /* free rebar bands */
    if ((infoPtr->uNumBands > 0) && infoPtr->bands) {
	/* clean up each band */
	for (i = 0; i < infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];

	    /* delete text strings */
	    if (lpBand->lpText) {
		COMCTL32_Free (lpBand->lpText);
		lpBand->lpText = NULL;
	    }
	    /* destroy child window */
	    DestroyWindow32 (lpBand->hwndChild);
	}

	/* free band array */
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }




    DeleteObject32 (infoPtr->hcurArrow);
    DeleteObject32 (infoPtr->hcurHorz);
    DeleteObject32 (infoPtr->hcurVert);
    DeleteObject32 (infoPtr->hcurDrag);




    /* free rebar info data */
    COMCTL32_Free (infoPtr);

    TRACE (rebar, "destroyed!\n");
    return 0;
}


static LRESULT
REBAR_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);

    return (LRESULT)infoPtr->hFont;
}


#if 0
static LRESULT
REBAR_MouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);

    return 0;
}
#endif


__inline__ static LRESULT
REBAR_NCCalcSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    if (wndPtr->dwStyle & WS_BORDER) {
	((LPRECT32)lParam)->left   += sysMetrics[SM_CXEDGE];
	((LPRECT32)lParam)->top    += sysMetrics[SM_CYEDGE];
	((LPRECT32)lParam)->right  -= sysMetrics[SM_CXEDGE];
	((LPRECT32)lParam)->bottom -= sysMetrics[SM_CYEDGE];
    }

    return 0;
}


static LRESULT
REBAR_NCPaint (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
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

    if (!(wndPtr->flags & WIN_MANAGED) && (wndPtr->dwStyle & WS_BORDER))
	DrawEdge32 (hdc, &wndPtr->rectWindow, EDGE_ETCHED, BF_RECT);

    ReleaseDC32( hwnd, hdc );

    return 0;
}


static LRESULT
REBAR_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    REBAR_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
REBAR_SetCursor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);
    POINT32 pt;
    UINT32  flags;

    TRACE (rebar, "code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos32 (&pt);
    ScreenToClient32 (wndPtr->hwndSelf, &pt);

    REBAR_InternalHitTest (wndPtr, &pt, &flags, NULL);

    if (flags == RBHT_GRABBER) {
	if ((wndPtr->dwStyle & CCS_VERT) &&
	    !(wndPtr->dwStyle & RBS_VERTICALGRIPPER))
	    SetCursor32 (infoPtr->hcurVert);
	else
	    SetCursor32 (infoPtr->hcurHorz);
    }
    else if (flags != RBHT_CLIENT)
	SetCursor32 (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
REBAR_SetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);
    TEXTMETRIC32A tm;
    HFONT32 hFont, hOldFont;
    HDC32 hdc;

    infoPtr->hFont = (HFONT32)wParam;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (SYSTEM_FONT);
/*
    hdc = GetDC32 (0);
    hOldFont = SelectObject32 (hdc, hFont);
    GetTextMetrics32A (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (0, hdc);
*/
    if (lParam) {
/*
        REBAR_Layout (wndPtr);
        hdc = GetDC32 (wndPtr->hwndSelf);
        REBAR_Refresh (wndPtr, hdc);
        ReleaseDC32 (wndPtr->hwndSelf, hdc);
*/
    }

    return 0;
}

static LRESULT
REBAR_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr(wndPtr);
    RECT32 rcParent;
    INT32 x, y, cx, cy;

    /* auto resize deadlock check */
    if (infoPtr->bAutoResize) {
	infoPtr->bAutoResize = FALSE;
	return 0;
    }

    TRACE (rebar, "sizing rebar!\n");

    /* get parent rectangle */
    GetClientRect32 (GetParent32 (wndPtr->hwndSelf), &rcParent);
/*
    REBAR_Layout (wndPtr, &rcParent);

    if (wndPtr->dwStyle & CCS_VERT) {
	if (wndPtr->dwStyle & CCS_LEFT == CCS_LEFT) {
	    x = rcParent.left;
	    y = rcParent.top;
	    cx = infoPtr->calcSize.cx;
	    cy = infoPtr->calcSize.cy;
	}
	else {
	    x = rcParent.right - infoPtr->calcSize.cx;
	    y = rcParent.top;
	    cx = infoPtr->calcSize.cx;
	    cy = infoPtr->calcSize.cy;
	}
    }
    else {
	if (wndPtr->dwStyle & CCS_TOP) {
	    x = rcParent.left;
	    y = rcParent.top;
	    cx = infoPtr->calcSize.cx;
	    cy = infoPtr->calcSize.cy;
	}
	else {
	    x = rcParent.left;
	    y = rcParent.bottom - infoPtr->calcSize.cy;
	    cx = infoPtr->calcSize.cx;
	    cy = infoPtr->calcSize.cy;
	}
    }

    SetWindowPos32 (wndPtr->hwndSelf, 0, x, y, cx, cy,
		    SWP_NOZORDER | SWP_SHOWWINDOW);
*/
    REBAR_Layout (wndPtr, NULL);
    REBAR_ForceResize (wndPtr);
    REBAR_MoveChildWindows (wndPtr);

    return 0;
}


LRESULT WINAPI
REBAR_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case RB_BEGINDRAG:

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (wndPtr, wParam, lParam);

//	case RB_DRAGMOVE:
//	case RB_ENDDRAG:
//	case RB_GETBANDBORDERS:

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (wndPtr);

//	case RB_GETBANDINFO32:  /* outdated, just for compatibility */

	case RB_GETBANDINFO32A:
	    return REBAR_GetBandInfo32A (wndPtr, wParam, lParam);

	case RB_GETBANDINFO32W:
	    return REBAR_GetBandInfo32W (wndPtr, wParam, lParam);

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (wndPtr, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (wndPtr, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (wndPtr);

//	case RB_GETCOLORSCHEME:
//	case RB_GETDROPTARGET:

	case RB_GETPALETTE:
	    return REBAR_GetPalette (wndPtr, wParam, lParam);

	case RB_GETRECT:
	    return REBAR_GetRect (wndPtr, wParam, lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (wndPtr);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (wndPtr, wParam, lParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (wndPtr);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (wndPtr);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (wndPtr);

	case RB_HITTEST:
	    return REBAR_HitTest (wndPtr, wParam, lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (wndPtr, wParam, lParam);

	case RB_INSERTBAND32A:
	    return REBAR_InsertBand32A (wndPtr, wParam, lParam);

	case RB_INSERTBAND32W:
	    return REBAR_InsertBand32W (wndPtr, wParam, lParam);

//	case RB_MAXIMIZEBAND:
//	case RB_MINIMIZEBAND:
//	case RB_MOVEBAND:

	case RB_SETBANDINFO32A:
	    return REBAR_SetBandInfo32A (wndPtr, wParam, lParam);

	case RB_SETBANDINFO32W:
	    return REBAR_SetBandInfo32W (wndPtr, wParam, lParam);

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (wndPtr, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (wndPtr, wParam, lParam);

//	case RB_SETCOLORSCHEME:
//	case RB_SETPALETTE:

	case RB_SETPARENT:
	    return REBAR_SetParent (wndPtr, wParam, lParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (wndPtr, wParam, lParam);

//	case RB_SETTOOLTIPS:

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (wndPtr, wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (wndPtr, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (wndPtr, wParam, lParam);


	case WM_CREATE:
	    return REBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (wndPtr, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (wndPtr, wParam, lParam);

//	case WM_MOUSEMOVE:
//	    return REBAR_MouseMove (wndPtr, wParam, lParam);

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (wndPtr, wParam, lParam);

	case WM_NCPAINT:
	    return REBAR_NCPaint (wndPtr, wParam, lParam);

	case WM_NOTIFY:
	    return SendMessage32A (wndPtr->parent->hwndSelf, WM_NOTIFY,
				   wParam, lParam);

	case WM_PAINT:
	    return REBAR_Paint (wndPtr, wParam);

	case WM_SETCURSOR:
	    return REBAR_SetCursor (wndPtr, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return REBAR_Size (wndPtr, wParam, lParam);

//	case WM_TIMER:

//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (rebar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
REBAR_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (REBARCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH32)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = REBARCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}


VOID
REBAR_Unregister (VOID)
{
    if (GlobalFindAtom32A (REBARCLASSNAME32A))
	UnregisterClass32A (REBARCLASSNAME32A, (HINSTANCE32)NULL);
}

