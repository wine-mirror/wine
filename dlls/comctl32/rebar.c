/*
 * Rebar control
 *
 * Copyright 1998, 1999 Eric Kohl
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

#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "commctrl.h"
#include "rebar.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(rebar)


/* fDraw flags */
#define DRAW_GRIPPER    1
#define DRAW_IMAGE      2
#define DRAW_TEXT       4
#define DRAW_CHILD      8

#define GRIPPER_WIDTH   13


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongA (hwnd, 0))


static VOID
REBAR_DrawBand (HDC hdc, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{

    DrawEdge (hdc, &lpBand->rcBand, BDR_RAISEDINNER, BF_MIDDLE);

    /* draw background */

    /* draw gripper */
    if (lpBand->fDraw & DRAW_GRIPPER)
	DrawEdge (hdc, &lpBand->rcGripper, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);

    /* draw caption image */
    if (lpBand->fDraw & DRAW_IMAGE) {
	/* FIXME: center image */
	POINT pt;

	pt.y = (lpBand->rcCapImage.bottom + lpBand->rcCapImage.top - infoPtr->imageSize.cy)/2;
	pt.x = (lpBand->rcCapImage.right + lpBand->rcCapImage.left - infoPtr->imageSize.cx)/2;

	ImageList_Draw (infoPtr->himl, lpBand->iImage, hdc,
/*			lpBand->rcCapImage.left, lpBand->rcCapImage.top, */
			pt.x, pt.y,
			ILD_TRANSPARENT);
    }

    /* draw caption text */
    if (lpBand->fDraw & DRAW_TEXT) {
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	INT oldBkMode = SetBkMode (hdc, TRANSPARENT);
	DrawTextW (hdc, lpBand->lpText, -1, &lpBand->rcCapText,
		     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (oldBkMode != TRANSPARENT)
	    SetBkMode (hdc, oldBkMode);
	SelectObject (hdc, hOldFont);
    }
}


static VOID
REBAR_Refresh (HWND hwnd, HDC hdc)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    UINT i;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((GetWindowLongA (hwnd, GWL_STYLE) & CCS_VERT) &&
	     (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	REBAR_DrawBand (hdc, infoPtr, lpBand);

    }
}


static VOID
REBAR_CalcHorzBand (REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    lpBand->fDraw = 0;

    /* set initial caption image rectangle */
    SetRect (&lpBand->rcCapImage, 0, 0, 0, 0);

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
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	lpBand->rcCapText.right += size.cx;

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
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
	OffsetRect (&lpBand->rcCapImage, GRIPPER_WIDTH, 0);
	OffsetRect (&lpBand->rcCapText, GRIPPER_WIDTH, 0);

	/* adjust child rectangle */
	lpBand->rcChild.left += GRIPPER_WIDTH;
    }


}


static VOID
REBAR_CalcVertBand (HWND hwnd, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
{
    lpBand->fDraw = 0;

    /* set initial caption image rectangle */
    SetRect (&lpBand->rcCapImage, 0, 0, 0, 0);

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
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
/*	lpBand->rcCapText.right += size.cx; */
	lpBand->rcCapText.bottom += size.cy;

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
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

	if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_VERTICALGRIPPER) {
	    /* adjust band width */
	    lpBand->rcBand.right += GRIPPER_WIDTH;
	    lpBand->uMinHeight += GRIPPER_WIDTH;

	    lpBand->rcGripper.left   = lpBand->rcBand.left + 3;
	    lpBand->rcGripper.right  = lpBand->rcGripper.left + 3;
	    lpBand->rcGripper.top    = lpBand->rcBand.top + 3;
	    lpBand->rcGripper.bottom = lpBand->rcBand.bottom - 3;

	    /* move caption rectangles */
	    OffsetRect (&lpBand->rcCapImage, GRIPPER_WIDTH, 0);
	    OffsetRect (&lpBand->rcCapText, GRIPPER_WIDTH, 0);
 
	    /* adjust child rectangle */
	    lpBand->rcChild.left += GRIPPER_WIDTH;
	}
	else {
	    lpBand->rcGripper.left   = lpBand->rcBand.left + 3;
	    lpBand->rcGripper.right  = lpBand->rcBand.right - 3;
	    lpBand->rcGripper.top    = lpBand->rcBand.top + 3;
	    lpBand->rcGripper.bottom = lpBand->rcGripper.top + 3;

	    /* move caption rectangles */
	    OffsetRect (&lpBand->rcCapImage, 0, GRIPPER_WIDTH);
	    OffsetRect (&lpBand->rcCapText, 0, GRIPPER_WIDTH);
 
	    /* adjust child rectangle */
	    lpBand->rcChild.top += GRIPPER_WIDTH;
	}
    }
}


static VOID
REBAR_Layout (HWND hwnd, LPRECT lpRect)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    REBAR_BAND *lpBand;
    RECT rcClient;
    INT x, y, cx, cy;
    UINT i;

    if (lpRect)
	rcClient = *lpRect;
    else
	GetClientRect (hwnd, &rcClient);

    x = 0;
    y = 0;

    if (dwStyle & CCS_VERT) {
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
	    ((dwStyle & CCS_VERT) && (lpBand->fStyle & RBBS_NOVERT)))
	    continue;


	if (dwStyle & CCS_VERT) {
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

	if (dwStyle & CCS_VERT) {
	    REBAR_CalcVertBand (hwnd, infoPtr, lpBand);
	    x += lpBand->uMinHeight;
	}
	else {
	    REBAR_CalcHorzBand (infoPtr, lpBand);
	    y += lpBand->uMinHeight;
	}
    }

    if (dwStyle & CCS_VERT) {
	infoPtr->calcSize.cx = x;
	infoPtr->calcSize.cy = rcClient.bottom - rcClient.top;
    }
    else {
	infoPtr->calcSize.cx = rcClient.right - rcClient.left;
	infoPtr->calcSize.cy = y;
    }
}


static VOID
REBAR_ForceResize (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rc;

    TRACE(" to [%d x %d]!\n",
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy);

    infoPtr->bAutoResize = TRUE;

    rc.left = 0;
    rc.top = 0;
    rc.right  = infoPtr->calcSize.cx;
    rc.bottom = infoPtr->calcSize.cy;

    if (GetWindowLongA (hwnd, GWL_STYLE) & WS_BORDER) {
	InflateRect (&rc, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    }

    SetWindowPos (hwnd, 0, 0, 0,
		    rc.right - rc.left, rc.bottom - rc.top,
		    SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}


static VOID
REBAR_MoveChildWindows (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    CHAR szClassName[40];
    UINT i;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if (lpBand->fStyle & RBBS_HIDDEN)
	    continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %x\n", lpBand->hwndChild);

	    GetClassNameA (lpBand->hwndChild, szClassName, 40);
	    if (!lstrcmpA (szClassName, "ComboBox")) {
		INT nEditHeight, yPos;
		RECT rc;

		/* special placement code for combo box */


		/* get size of edit line */
		GetWindowRect (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, /*lpBand->rcChild.top*/ yPos,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    nEditHeight,
			    SWP_SHOWWINDOW);
	    }
#if 0
	    else if (!lstrcmpA (szClassName, WC_COMBOBOXEXA)) {
		INT nEditHeight, yPos;
		RECT rc;
		HWND hwndEdit;

		/* special placement code for extended combo box */

		/* get size of edit line */
		hwndEdit = SendMessageA (lpBand->hwndChild, CBEM_GETEDITCONTROL, 0, 0);
		GetWindowRect (hwndEdit, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, /*lpBand->rcChild.top*/ yPos,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    nEditHeight,
			    SWP_SHOWWINDOW);

	    }
#endif
	    else {
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, lpBand->rcChild.top,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    lpBand->rcChild.bottom - lpBand->rcChild.top,
			    SWP_SHOWWINDOW);
	    }
	}
    }
}


static void
REBAR_InternalHitTest (HWND hwnd, LPPOINT lpPt, UINT *pFlags, INT *pBand)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    RECT rect;
    INT  iCount;

    GetClientRect (hwnd, &rect);

    *pFlags = RBHT_NOWHERE;
    if (PtInRect (&rect, *lpPt))
    {
	if (infoPtr->uNumBands == 0) {
	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;
	    TRACE("NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    for (iCount = 0; iCount < infoPtr->uNumBands; iCount++) {
		lpBand = &infoPtr->bands[iCount];
		if (PtInRect (&lpBand->rcBand, *lpPt)) {
		    if (pBand)
			*pBand = iCount;
		    if (PtInRect (&lpBand->rcGripper, *lpPt)) {
			*pFlags = RBHT_GRABBER;
			TRACE("ON GRABBER %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapImage, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapText, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcChild, *lpPt)) {
			*pFlags = RBHT_CLIENT;
			TRACE("ON CLIENT %d\n", iCount);
			return;
		    }
		    else {
			*pFlags = RBHT_NOWHERE;
			TRACE("NOWHERE %d\n", iCount);
			return;
		    }
		}
	    }

	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;

	    TRACE("NOWHERE\n");
	    return;
	}
    }
    else {
	*pFlags = RBHT_NOWHERE;
	if (pBand)
	    *pBand = -1;
	TRACE("NOWHERE\n");
	return;
    }

    TRACE("flags=0x%X\n", *pFlags);
    return;
}



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    UINT uBand = (UINT)wParam;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);

    if (infoPtr->uNumBands == 1) {
	TRACE(" simple delete!\n");
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
	infoPtr->uNumBands = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
        TRACE("complex delete! [uBand=%u]\n", uBand);

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

    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    /* LPRECT32 lpRect = (LPRECT32)lParam; */
    REBAR_BAND *lpBand;

    if (!lParam)
	return 0;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return 0;

    lpBand = &infoPtr->bands[(UINT)wParam];
    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_BANDBORDERS) {

    }
    else {

    }

    return 0;
}


inline static LRESULT
REBAR_GetBandCount (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
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
REBAR_GetBandInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && 
	(lprbbi->lpText) && (lpBand->lpText)) {
	    lstrcpynW (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
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
REBAR_GetBarHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT nHeight;

    REBAR_Layout (hwnd, NULL);
    nHeight = infoPtr->calcSize.cy;

    if (GetWindowLongA (hwnd, GWL_STYLE) & WS_BORDER)
	nHeight += (2 * GetSystemMetrics(SM_CYEDGE));


    FIXME("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("getting bar info!\n");

    if (infoPtr->himl) {
	lpInfo->himl = infoPtr->himl;
	lpInfo->fMask |= RBIM_IMAGELIST;
    }

    return TRUE;
}


inline static LRESULT
REBAR_GetBkColor (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("background color 0x%06lx!\n", infoPtr->clrBk);

    return infoPtr->clrBk;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT iBand = (INT)wParam;
    LPRECT lprc = (LPRECT)lParam;
    REBAR_BAND *lpBand;

    if ((iBand < 0) && ((UINT)iBand >= infoPtr->uNumBands))
	return FALSE;
    if (!lprc)
	return FALSE;

    TRACE("band %d\n", iBand);

    lpBand = &infoPtr->bands[iBand];
    CopyRect (lprc, &lpBand->rcBand);
/*
    lprc->left   = lpBand->rcBand.left;
    lprc->top    = lpBand->rcBand.top;
    lprc->right  = lpBand->rcBand.right;
    lprc->bottom = lpBand->rcBand.bottom;
*/

    return TRUE;
}


inline static LRESULT
REBAR_GetRowCount (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    FIXME("%u : semi stub!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetRowHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("-- height = 20: semi stub!\n");

    return 20;
}


inline static LRESULT
REBAR_GetTextColor (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


inline static LRESULT
REBAR_GetToolTips (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    return infoPtr->hwndToolTip;
}


inline static LRESULT
REBAR_GetUnicodeFormat (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    return infoPtr->bUnicode;
}


static LRESULT
REBAR_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam; 

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (hwnd, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    UINT i;

    if (infoPtr == NULL)
	return -1;

    if (infoPtr->uNumBands < 1)
	return -1;

    TRACE("id %u\n", (UINT)wParam);

    for (i = 0; i < infoPtr->uNumBands; i++) {
	if (infoPtr->bands[i].wID == (UINT)wParam) {
	    TRACE("band %u found!\n", i);
	    return i;
	}
    }

    TRACE("no band found!\n");
    return -1;
}


static LRESULT
REBAR_InsertBandA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;

    TRACE("insert band at %u!\n", uIndex);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
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

    TRACE("index %u!\n", uIndex);

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
	INT len = lstrlenA (lprbbi->lpText);
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
	TRACE("hwndChild = %x\n", lprbbi->hwndChild);
	lpBand->hwndChild = lprbbi->hwndChild;
	lpBand->hwndPrevParent =
	    SetParent (lpBand->hwndChild, hwnd);
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_InsertBandW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;

    TRACE("insert band at %u!\n", uIndex);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
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

    TRACE("index %u!\n", uIndex);

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
	INT len = lstrlenW (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;
    else
	lpBand->iImage = -1;

    if (lprbbi->fMask & RBBIM_CHILD) {
	TRACE("hwndChild = %x\n", lprbbi->hwndChild);
	lpBand->hwndChild = lprbbi->hwndChild;
	lpBand->hwndPrevParent =
	    SetParent (lpBand->hwndChild, hwnd);
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }


    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u fIdeal = %s)\n",
	   (UINT)wParam, lParam ? "TRUE" : "FALSE");

 
    return 0;
}


static LRESULT
REBAR_MinimizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u)\n", (UINT)wParam);

 
    return 0;
}


static LRESULT
REBAR_MoveBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(iFrom = %u iTof = %u)\n",
	   (UINT)wParam, (UINT)lParam);

 
    return FALSE;
}


static LRESULT
REBAR_SetBandInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

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
	    INT len = lstrlenA (lprbbi->lpText);
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
		SetParent (lpBand->hwndChild, hwnd);
	}
	else {
	    TRACE("child: 0x%x  prev parent: 0x%x\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }

    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_SetBandInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

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
	    INT len = lstrlenW (lprbbi->lpText);
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
	if (lprbbi->hwndChild) {
	    lpBand->hwndChild = lprbbi->hwndChild;
	    lpBand->hwndPrevParent =
		SetParent (lpBand->hwndChild, hwnd);
	}
	else {
	    TRACE("child: 0x%x  prev parent: 0x%x\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
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
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }

    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("setting bar info!\n");

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
REBAR_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;

    TRACE("background color 0x%06lx!\n", infoPtr->clrBk);

    return clrTemp;
}


/* << REBAR_SetColorScheme >> */
/* << REBAR_SetPalette >> */


static LRESULT
REBAR_SetParent (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    HWND hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = (COLORREF)lParam;

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


inline static LRESULT
REBAR_SetUnicodeFormat (HWND hwnd, WPARAM wParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    BOOL bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;
    return bTemp;
}


static LRESULT
REBAR_ShowBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;

    if (((INT)wParam < 0) || ((INT)wParam > infoPtr->uNumBands))
	return FALSE;

    lpBand = &infoPtr->bands[(INT)wParam];

    if ((BOOL)lParam) {
	TRACE("show band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle & ~RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
    }
    else {
	TRACE("hide band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle | RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
    }

    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPRECT lpRect = (LPRECT)lParam;

    if (lpRect == NULL)
	return FALSE;

    FIXME("layout change not implemented!\n");
    FIXME("[%d %d %d %d]\n",
	   lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

#if 0
    SetWindowPos (hwnd, 0, lpRect->left, lpRect->top,
		    lpRect->right - lpRect->left, lpRect->bottom - lpRect->top,
		    SWP_NOZORDER);
#endif

    infoPtr->calcSize.cx = lpRect->right - lpRect->left;
    infoPtr->calcSize.cy = lpRect->bottom - lpRect->top;

    REBAR_ForceResize (hwnd);
    return TRUE;
}



static LRESULT
REBAR_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)COMCTL32_Alloc (sizeof(REBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* initialize info structure */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = RGB(0, 0, 0);

    infoPtr->bAutoResize = FALSE;
    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->hcurHorz  = LoadCursorA (0, IDC_SIZEWEA);
    infoPtr->hcurVert  = LoadCursorA (0, IDC_SIZENSA);
    infoPtr->hcurDrag  = LoadCursorA (0, IDC_SIZEA);

    infoPtr->bUnicode = IsWindowUnicode (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_AUTOSIZE)
	FIXME("style RBS_AUTOSIZE set!\n");

#if 0
    SendMessageA (hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
#endif

    TRACE("created!\n");
    return 0;
}


static LRESULT
REBAR_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    INT i;


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
	    DestroyWindow (lpBand->hwndChild);
	}

	/* free band array */
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }




    DeleteObject (infoPtr->hcurArrow);
    DeleteObject (infoPtr->hcurHorz);
    DeleteObject (infoPtr->hcurVert);
    DeleteObject (infoPtr->hcurDrag);




    /* free rebar info data */
    COMCTL32_Free (infoPtr);

    TRACE("destroyed!\n");
    return 0;
}


static LRESULT
REBAR_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


#if 0
static LRESULT
REBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    return 0;
}
#endif


inline static LRESULT
REBAR_NCCalcSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (GetWindowLongA (hwnd, GWL_STYLE) & WS_BORDER) {
	((LPRECT)lParam)->left   += GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->top    += GetSystemMetrics(SM_CYEDGE);
	((LPRECT)lParam)->right  -= GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->bottom -= GetSystemMetrics(SM_CYEDGE);
    }

    return 0;
}


static LRESULT
REBAR_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcWindow;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    DefWindowProcA (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW )))
	return 0;

    if (dwStyle & WS_BORDER) {
	GetWindowRect (hwnd, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_RECT);
    }

    ReleaseDC( hwnd, hdc );

    return 0;
}


static LRESULT
REBAR_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    REBAR_Refresh (hwnd, hdc);
    if (!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
REBAR_SetCursor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (hwnd, &pt);

    REBAR_InternalHitTest (hwnd, &pt, &flags, NULL);

    if (flags == RBHT_GRABBER) {
	if ((dwStyle & CCS_VERT) &&
	    !(dwStyle & RBS_VERTICALGRIPPER))
	    SetCursor (infoPtr->hcurVert);
	else
	    SetCursor (infoPtr->hcurHorz);
    }
    else if (flags != RBHT_CLIENT)
	SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
REBAR_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    
    /* TEXTMETRIC32A tm; */
    HFONT hFont /*, hOldFont */;
    /* HDC32 hdc; */

    infoPtr->hFont = (HFONT)wParam;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
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
        REBAR_Layout (hwnd);
        hdc = GetDC32 (hwnd);
        REBAR_Refresh (hwnd, hdc);
        ReleaseDC32 (hwnd, hdc);
*/
    }

    return 0;
}

static LRESULT
REBAR_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    /* DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE); */
    RECT rcParent;
    /* INT32 x, y, cx, cy; */

    /* auto resize deadlock check */
    if (infoPtr->bAutoResize) {
	infoPtr->bAutoResize = FALSE;
	return 0;
    }

    TRACE("sizing rebar!\n");

    /* get parent rectangle */
    GetClientRect (GetParent (hwnd), &rcParent);
/*
    REBAR_Layout (hwnd, &rcParent);

    if (dwStyle & CCS_VERT) {
	if (dwStyle & CCS_LEFT == CCS_LEFT) {
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
	if (dwStyle & CCS_TOP) {
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

    SetWindowPos32 (hwnd, 0, x, y, cx, cy,
		    SWP_NOZORDER | SWP_SHOWWINDOW);
*/
    REBAR_Layout (hwnd, NULL);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return 0;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
/*	case RB_BEGINDRAG: */

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (hwnd, wParam, lParam);

/*	case RB_DRAGMOVE: */
/*	case RB_ENDDRAG: */

	case RB_GETBANDBORDERS:
	    return REBAR_GetBandBorders (hwnd, wParam, lParam);

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (hwnd);

/*	case RB_GETBANDINFO32:  */ /* outdated, just for compatibility */

	case RB_GETBANDINFOA:
	    return REBAR_GetBandInfoA (hwnd, wParam, lParam);

	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoW (hwnd, wParam, lParam);

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (hwnd, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (hwnd, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (hwnd);

/*	case RB_GETCOLORSCHEME: */
/*	case RB_GETDROPTARGET: */

	case RB_GETPALETTE:
	    return REBAR_GetPalette (hwnd, wParam, lParam);

	case RB_GETRECT:
	    return REBAR_GetRect (hwnd, wParam, lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (hwnd);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (hwnd, wParam, lParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (hwnd);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (hwnd);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (hwnd);

	case RB_HITTEST:
	    return REBAR_HitTest (hwnd, wParam, lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (hwnd, wParam, lParam);

	case RB_INSERTBANDA:
	    return REBAR_InsertBandA (hwnd, wParam, lParam);

	case RB_INSERTBANDW:
	    return REBAR_InsertBandW (hwnd, wParam, lParam);

	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (hwnd, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (hwnd, wParam, lParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (hwnd, wParam, lParam);

	case RB_SETBANDINFOA:
	    return REBAR_SetBandInfoA (hwnd, wParam, lParam);

	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoW (hwnd, wParam, lParam);

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (hwnd, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (hwnd, wParam, lParam);

/*	case RB_SETCOLORSCHEME: */
/*	case RB_SETPALETTE: */
/*	    return REBAR_GetPalette (hwnd, wParam, lParam); */

	case RB_SETPARENT:
	    return REBAR_SetParent (hwnd, wParam, lParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (hwnd, wParam, lParam);

/*	case RB_SETTOOLTIPS: */

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (hwnd, wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (hwnd, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (hwnd, wParam, lParam);


	case WM_COMMAND:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

	case WM_CREATE:
	    return REBAR_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (hwnd, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (hwnd, wParam, lParam);

/*	case WM_MOUSEMOVE: */
/*	    return REBAR_MouseMove (hwnd, wParam, lParam); */

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (hwnd, wParam, lParam);

	case WM_NCPAINT:
	    return REBAR_NCPaint (hwnd, wParam, lParam);

	case WM_NOTIFY:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

	case WM_PAINT:
	    return REBAR_Paint (hwnd, wParam);

	case WM_SETCURSOR:
	    return REBAR_SetCursor (hwnd, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (hwnd, wParam, lParam);

	case WM_SIZE:
	    return REBAR_Size (hwnd, wParam, lParam);

/*	case WM_TIMER: */

/*	case WM_WININICHANGE: */

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
REBAR_Register (void)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (REBARCLASSNAMEA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = REBARCLASSNAMEA;
 
    RegisterClassA (&wndClass);
}


VOID
REBAR_Unregister (void)
{
    if (GlobalFindAtomA (REBARCLASSNAMEA))
	UnregisterClassA (REBARCLASSNAMEA, (HINSTANCE)NULL);
}

