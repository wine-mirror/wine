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
 *   - All messages.
 *   - All notifications.
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
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);



}



// << LISTVIEW_FindItem >>


static LRESULT
LISTVIEW_GetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->clrBk;
}


// << LISTVIEW_GetBkImage >>


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




__inline__ static LRESULT
LISTVIEW_GetHeader (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->hwndHeader;
}


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


static LRESULT
LISTVIEW_GetItem32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    FIXME (listview, "(0x%08x) empty stub!\n", wParam);



    return TRUE;
}



__inline__ static LRESULT
LISTVIEW_GetItemCount (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    return infoPtr->nItemCount;
}



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

    TRACE (listview, "(%s) empty stub!\n", lpsz);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject32 (SYSTEM_FONT);
    hdc = GetDC32 (0);
    hOldFont = SelectObject32 (hdc, hFont);
    GetTextExtentPoint32A (hdc, lpsz, lstrlen32A(lpsz), &size);
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (0, hdc);

    TRACE (listview, "-- ret=%d\n", size.cx);

    return (LRESULT)size.cx;
}




static LRESULT
LISTVIEW_InsertColumn32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LPLVCOLUMN32A lpcol = (LPLVCOLUMN32A)lParam;
    HDITEM32A hdi;

    if (!lpcol)
	return -1;

    TRACE (listview, "(%d %p) empty stub!\n", (INT32)wParam, lpcol);

    ZeroMemory (&hdi, sizeof(HDITEM32A));

    if (lpcol->mask & LVCF_FMT) {
	if (wParam == 0)
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

    return (LRESULT)SendMessage32A (infoPtr->hwndHeader, HDM_INSERTITEM32A,
				    wParam, (LPARAM)&hdi);
}




static LRESULT
LISTVIEW_SetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    if (!(infoPtr)) return FALSE;

    /* set background color */
    TRACE (listview, "0x%06x\n", (COLORREF)lParam);
    infoPtr->clrBk = (COLORREF)lParam;

    return TRUE;
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
LISTVIEW_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    /* info structure is created at NCCreate */
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);
    LOGFONT32A logFont;
    DWORD dwStyle = WS_CHILD | WS_VISIBLE; 


    /* initialize info structure */
    infoPtr->clrBk = CLR_NONE;

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


    infoPtr->hdsaItems = DSA_Create (sizeof(LISTVIEW_ITEM), 10);

    return 0;
}


static LRESULT
LISTVIEW_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    DSA_Destroy (infoPtr->hdsaItems);

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
    return FALSE;
}


__inline__ static LRESULT
LISTVIEW_GetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LISTVIEW_INFO *infoPtr = LISTVIEW_GetInfoPtr(wndPtr);

    return infoPtr->hFont;
}


// << LISTVIEW_HScroll >>
// << LISTVIEW_KeyDown >>
// << LISTVIEW_KillFocus >>


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

    GetClientRect32 (wndPtr->hwndSelf, &infoPtr->rcList);
    infoPtr->rcList.top += wp.cy;

    return 0;
}


LRESULT WINAPI
LISTVIEW_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);

    switch (uMsg)
    {
//	case LVM_DELETEALLITEMS:

        case LVM_GETBKCOLOR:
	    return LISTVIEW_GetBkColor (wndPtr, wParam, lParam);


        case LVM_GETCOLUMNWIDTH:
	    return LISTVIEW_GetColumnWidth (wndPtr, wParam, lParam);


        case LVM_GETHEADER:
	    return LISTVIEW_GetHeader (wndPtr, wParam, lParam);

//	case LVM_GETISEARCHSTRING:

	case LVM_GETIMAGELIST:
	    return LISTVIEW_GetImageList (wndPtr, wParam, lParam);

	case LVM_GETITEM32A:
	    return LISTVIEW_GetItem32A (wndPtr, wParam, lParam);

//	case LVM_GETITEM32W:

	case LVM_GETITEMCOUNT:
	    return LISTVIEW_GetItemCount (wndPtr, wParam, lParam);

//	case LVM_GETSELECTEDCOUNT:

	case LVM_GETSTRINGWIDTH32A:
	    return LISTVIEW_GetStringWidth32A (wndPtr, wParam, lParam);

//	case LVM_GETSTRINGWIDTH32W:
//	case LVM_GETSUBITEMRECT:

	case LVM_INSERTCOLUMN32A:
	    return LISTVIEW_InsertColumn32A (wndPtr, wParam, lParam);


//	case LVM_INSERTCOLUMN32W:

//	case LVM_INSERTITEM32A:
//	case LVM_INSERTITEM32W:


	case LVM_SETBKCOLOR:
	    return LISTVIEW_SetBkColor (wndPtr, wParam, lParam);

	
	case LVM_SETIMAGELIST:
	    return LISTVIEW_SetImageList (wndPtr, wParam, lParam);

//	case LVM_SETITEMPOSITION:

//	case LVM_SORTITEMS:


//	case WM_CHAR:
//	case WM_COMMAND:

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

//	case WM_HSCROLL:

//	case WM_MOUSEMOVE:
//	    return LISTVIEW_MouseMove (wndPtr, wParam, lParam);

	case WM_NCCREATE:
	    return LISTVIEW_NCCreate (wndPtr, wParam, lParam);

	case WM_NCDESTROY:
	    return LISTVIEW_NCDestroy (wndPtr, wParam, lParam);

//	case WM_NOTIFY:

	case WM_PAINT:
	    return LISTVIEW_Paint (wndPtr, wParam);

//	case WM_RBUTTONDOWN:
//	case WM_SETFOCUS:

	case WM_SETFONT:
	    return LISTVIEW_SetFont (wndPtr, wParam, lParam);

//	case WM_SETREDRAW:

	case WM_SIZE:
	    return LISTVIEW_Size (wndPtr, wParam, lParam);

//	case WM_TIMER:
//	case WM_VSCROLL:
//	case WM_WINDOWPOSCHANGED:
//	case WM_WININICHANGE:

	default:
	    if (uMsg >= WM_USER)
		ERR (listview, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
LISTVIEW_Register (void)
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

