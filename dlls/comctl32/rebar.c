/*
 * Rebar control
 *
 * Copyright 1998 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 */

#include "windows.h"
#include "commctrl.h"
#include "rebar.h"
#include "win.h"
#include "debug.h"


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)wndPtr->wExtra[0])




static VOID
REBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    RECT32 rect;
    HBRUSH32 hbrBk;

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    hbrBk = CreateSolidBrush32 (RGB (192, 192, 192));
    FillRect32 (hdc, &rect, hbrBk);
    DeleteObject32 (hbrBk);
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

    return TRUE;
}


// << REBAR_DragMove >>
// << REBAR_EndDrag >>
// << REBAR_GetBandBorders >>


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
	    lstrcpyn32A (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
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


// << REBAR_GetBandInfo32W >>


static LRESULT
REBAR_GetBarHeight (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    FIXME (rebar, "returns fixed height of 40 pixels!\n");

    return 40;
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

// << REBAR_GetRowHeight >>


__inline__ static LRESULT
REBAR_GetTextColor (WND *wndPtr)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    TRACE (rebar, "text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


// << REBAR_GetToolTips >>
// << REBAR_GetUnicodeFormat >>
// << REBAR_HitTest >>


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

	COMCTL32_Free (&oldBands);
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

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT32 len = lstrlen32A (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPSTR)COMCTL32_Alloc (len + 1);
	    lstrcpy32A (lpBand->lpText, lprbbi->lpText);
	}
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
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

    return TRUE;
}


// << REBAR_InsertBand32W >>
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

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
/*
	INT32 len = lstrlen32A (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPSTR)COMCTL32_Alloc (len + 1);
	    lstrcpy32A (lpBand->lpText, lprbbi->lpText);
	}
*/
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
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

    return TRUE;
}


// << REBAR_SetBandInfo32W >>


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

    if (lpInfo->fMask & RBIM_IMAGELIST)
	infoPtr->himl = lpInfo->himl;

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
// << REBAR_SetParent >>


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
// << REBAR_SetUnicodeFormat >>


static LRESULT
REBAR_ShowBand (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (wndPtr);

    if (((INT32)wParam < 0) || ((INT32)wParam > infoPtr->uNumBands))
	return FALSE;

    if ((BOOL32)lParam)
	FIXME (rebar, "show band %d\n", (INT32)wParam);
    else
	FIXME (rebar, "hide band %d\n", (INT32)wParam);

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
    infoPtr->clrText = CLR_NONE;
    infoPtr->clrText = RGB(0, 0, 0);

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
	}

	/* free band array */
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }

    /* free rebar info data */
    COMCTL32_Free (infoPtr);

    TRACE (rebar, "destroyed!\n");
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

//	case RB_GETBANDINFO32W:

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (wndPtr, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (wndPtr, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (wndPtr);

//	case RB_GETCOLORSCHEME:
//	case RB_GETDROPTARGET:
//	case RB_GETPALETTE:
//	case RB_GETRECT:
//	case RB_GETROWCOUNT:
//	case RB_GETROWHEIGHT:

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (wndPtr);

//	case RB_GETTOOLTIPS:
//	case RB_GETUNICODEFORMAT:
//	case RB_HITTEST:

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (wndPtr, wParam, lParam);

	case RB_INSERTBAND32A:
	    return REBAR_InsertBand32A (wndPtr, wParam, lParam);

//	case RB_INSERTBAND32W:
//	case RB_MAXIMIZEBAND:
//	case RB_MINIMIZEBAND:
//	case RB_MOVEBAND:

	case RB_SETBANDINFO32A:
	    return REBAR_SetBandInfo32A (wndPtr, wParam, lParam);

//	case RB_SETBANDINFO32W:

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (wndPtr, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (wndPtr, wParam, lParam);

//	case RB_SETCOLORSCHEME:
//	case RB_SETPALETTE:
//	case RB_SETPARENT:

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (wndPtr, wParam, lParam);

//	case RB_SETTOOLTIPS:
//	case RB_SETUNICODEFORMAT:

	case RB_SHOWBAND:
	    return REBAR_ShowBand (wndPtr, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (wndPtr, wParam, lParam);


	case WM_CREATE:
	    return REBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (wndPtr, wParam, lParam);

//	case WM_GETFONT:

//	case WM_MOUSEMOVE:
//	    return REBAR_MouseMove (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return REBAR_Paint (wndPtr, wParam);

//	case WM_SETFONT:

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


void
REBAR_Register (void)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (REBARCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC32)REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH32) COLOR_BTNFACE + 1;
    wndClass.lpszClassName = REBARCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}

