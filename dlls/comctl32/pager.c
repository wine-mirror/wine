/*
 * Pager control
 *
 * Copyright 1998, 1999 Eric Kohl
 *
 * NOTES
 *    Tested primarily with the controlspy Pager application.
 *       Susan Farley (susan@codeweavers.com)
 *
 * TODO:
 *    Implement repetitive button press.
 *    Adjust arrow size relative to size of button.
 *    Allow border size changes.
 *    Implement drag and drop style.
 */

#include "winbase.h"
#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(pager);

typedef struct
{
    HWND   hwndChild;  /* handle of the contained wnd */
    BOOL   bHorizontal;/* orientation of the control */
    COLORREF clrBk;    /* background color */
    INT    nBorder;    /* border size for the control */
    INT    nButtonSize;/* size of the pager btns */
    INT    nPos;       /* scroll position */
    INT    nDelta;     /* scroll delta */
    INT    nWidth;     /* from child wnd's response to PGN_CALCSIZE */
    INT    nHeight;    /* from child wnd's response to PGN_CALCSIZE */ 
    BOOL   bForward;   /* forward WM_MOUSEMOVE msgs to the contained wnd */
    INT    TLbtnState; /* state of top or left btn */
    INT    BRbtnState; /* state of bottom or right btn */

} PAGER_INFO;

#define PAGER_GetInfoPtr(hwnd) ((PAGER_INFO *)GetWindowLongA(hwnd, 0))

#define MIN_ARROW_WIDTH  8
#define MIN_ARROW_HEIGHT 5

/* the horizontal arrows are: 
 *
 * 01234    01234
 * 1  *      *
 * 2 **      **
 * 3***      ***
 * 4***      ***
 * 5 **      **
 * 6  *      *
 * 7  
 *
 */
static void
PAGER_DrawHorzArrow (HDC hdc, RECT r, INT colorRef, BOOL left)
{
    INT x, y, w, h;
    HPEN hOldPen;
    
    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    if ((h < MIN_ARROW_WIDTH) || (w < MIN_ARROW_HEIGHT))
        return;  /* refuse to draw partial arrow */

    hOldPen = SelectObject ( hdc, GetSysColorPen (colorRef));
    if (left)
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 3;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x--, y+5); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x--, y+3); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x, y+1);
    }
    else
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x++, y+5); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x++, y+3); y++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x, y+1);
    }

    SelectObject( hdc, hOldPen );
}

/* the vertical arrows are: 
 *
 * 01234567    01234567
 * 1******        **  
 * 2 ****        ****
 * 3  **        ******
 * 4
 *
 */
static void
PAGER_DrawVertArrow (HDC hdc, RECT r, INT colorRef, BOOL up)
{
    INT x, y, w, h;
    HPEN hOldPen;
    
    w = r.right - r.left + 1;
    h = r.bottom - r.top + 1;
    if ((h < MIN_ARROW_WIDTH) || (w < MIN_ARROW_HEIGHT))
        return;  /* refuse to draw partial arrow */

    hOldPen = SelectObject ( hdc, GetSysColorPen (colorRef));
    if (up)
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 3;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+5, y--); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+3, y--); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+1, y);
    }
    else
    {
        x = r.left + ((w - MIN_ARROW_HEIGHT) / 2) + 1;
        y = r.top + ((h - MIN_ARROW_WIDTH) / 2) + 1;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+5, y++); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+3, y++); x++;
        MoveToEx (hdc, x, y, NULL);
        LineTo (hdc, x+1, y);
    }

    SelectObject( hdc, hOldPen );
}

static void
PAGER_DrawButton(HDC hdc, COLORREF clrBk, RECT arrowRect,
                 BOOL horz, BOOL topLeft, INT btnState)
{
    HBRUSH   hBrush, hOldBrush;
    RECT     rc = arrowRect;

    if (!btnState) /* PGF_INVISIBLE */
        return;

    if ((rc.right - rc.left <= 0) || (rc.bottom - rc.top <= 0))
        return;  

    hBrush = CreateSolidBrush(clrBk);
    hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    FillRect(hdc, &rc, hBrush);

    if (btnState == PGF_HOT) 
    {
       rc.left++, rc.top++; rc.right++, rc.bottom++;
       DrawEdge( hdc, &rc, EDGE_RAISED, BF_RECT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       rc.left--, rc.top--; rc.right--, rc.bottom--;
    }
    else if (btnState == PGF_NORMAL) 
    {
       DrawEdge (hdc, &rc, BDR_OUTER, BF_FLAT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
    }
    else if (btnState == PGF_DEPRESSED) 
    {
       rc.left++, rc.top++;
       DrawEdge( hdc, &rc, BDR_SUNKENOUTER, BF_RECT);
       if (horz)
           PAGER_DrawHorzArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       else
           PAGER_DrawVertArrow(hdc, rc, COLOR_WINDOWFRAME, topLeft);
       rc.left--, rc.top--;
    }
    else if (btnState == PGF_GRAYED) 
    {
       DrawEdge (hdc, &rc, BDR_OUTER, BF_FLAT);
       if (horz)
       {
           PAGER_DrawHorzArrow(hdc, rc, COLOR_3DHIGHLIGHT, topLeft);
           rc.left++, rc.top++; rc.right++, rc.bottom++;
           PAGER_DrawHorzArrow(hdc, rc, COLOR_3DSHADOW, topLeft);
       }
       else
       {
           PAGER_DrawVertArrow(hdc, rc, COLOR_3DHIGHLIGHT, topLeft);
           rc.left++, rc.top++; rc.right++, rc.bottom++;
           PAGER_DrawVertArrow(hdc, rc, COLOR_3DSHADOW, topLeft);
       }
       rc.left--, rc.top--; rc.right--, rc.bottom--;
    }

    SelectObject( hdc, hOldBrush );
    DeleteObject(hBrush);
}

/* << PAGER_GetDropTarget >> */

static inline LRESULT
PAGER_ForwardMouse (HWND hwnd, WPARAM wParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%04x]\n", hwnd);

    infoPtr->bForward = (BOOL)wParam;

    return 0;
}

static inline LRESULT
PAGER_GetButtonState (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); 
    LRESULT btnState = PGF_INVISIBLE;
    INT btn = (INT)lParam;
    TRACE("[%04x]\n", hwnd);

    if (btn == PGB_TOPORLEFT)
        btnState = infoPtr->TLbtnState;
    else if (btn == PGB_BOTTOMORRIGHT)
        btnState = infoPtr->BRbtnState;

    return btnState;
}


static inline LRESULT
PAGER_GetPos(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); 
    TRACE("[%04x] returns %d\n", hwnd, infoPtr->nPos);
    return (LRESULT)infoPtr->nPos;
}

static inline LRESULT
PAGER_GetButtonSize(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); 
    TRACE("[%04x] returns %d\n", hwnd, infoPtr->nButtonSize);
    return (LRESULT)infoPtr->nButtonSize;
}

static inline LRESULT
PAGER_GetBorder(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); 
    TRACE("[%04x] returns %d\n", hwnd, infoPtr->nBorder);
    return (LRESULT)infoPtr->nBorder;
}

static inline LRESULT
PAGER_GetBkColor(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd); 
    TRACE("[%04x] returns %06lx\n", hwnd, infoPtr->clrBk);
    return (LRESULT)infoPtr->clrBk;
}

static void 
PAGER_CalcSize (HWND hwnd, INT* size, BOOL getWidth) 
{
    NMPGCALCSIZE nmpgcs;
    ZeroMemory (&nmpgcs, sizeof (NMPGCALCSIZE));
    nmpgcs.hdr.hwndFrom = hwnd;
    nmpgcs.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
    nmpgcs.hdr.code = PGN_CALCSIZE;
    nmpgcs.dwFlag = getWidth ? PGF_CALCWIDTH : PGF_CALCHEIGHT;
    nmpgcs.iWidth = getWidth ? *size : 0;
    nmpgcs.iHeight = getWidth ? 0 : *size;
    SendMessageA (hwnd, WM_NOTIFY,
                  (WPARAM)nmpgcs.hdr.idFrom, (LPARAM)&nmpgcs);

    *size = getWidth ? nmpgcs.iWidth : nmpgcs.iHeight;

    TRACE("[%04x] PGN_CALCSIZE returns %s=%d\n", hwnd,
                  getWidth ? "width" : "height", *size);
}

static void
PAGER_PositionChildWnd(HWND hwnd, PAGER_INFO* infoPtr)
{
    if (infoPtr->hwndChild)
    {
        int nPos = infoPtr->nPos;

        /* compensate for a grayed btn, which will soon become invisible */
        if (infoPtr->TLbtnState == PGF_GRAYED)
            nPos += infoPtr->nButtonSize;

        if (infoPtr->bHorizontal)
        {
            TRACE("[%04x] SWP %dx%d at (%d,%d)\n", hwnd,
                         infoPtr->nWidth, infoPtr->nHeight,
                         -nPos, 0);
            SetWindowPos(infoPtr->hwndChild, 0,
                         -nPos, 0,
                         infoPtr->nWidth, infoPtr->nHeight,
    	     SWP_NOZORDER);
        }
        else
        {
            TRACE("[%04x] SWP %dx%d at (%d,%d)\n", hwnd, 
	         infoPtr->nWidth, infoPtr->nHeight,
                         0, -nPos);
            SetWindowPos(infoPtr->hwndChild, 0,
                         0, -nPos,
                         infoPtr->nWidth, infoPtr->nHeight,
                         SWP_NOZORDER);
        }

        InvalidateRect(infoPtr->hwndChild, NULL, FALSE);
    }
}

static INT
PAGER_GetScrollRange(HWND hwnd, PAGER_INFO* infoPtr)
{
    INT scrollRange = 0;

    if (infoPtr->hwndChild)
    {
        INT wndSize, childSize;
        RECT wndRect;
        GetWindowRect(hwnd, &wndRect);

        if (infoPtr->bHorizontal)
        {
            wndSize = wndRect.right - wndRect.left;
            PAGER_CalcSize(hwnd, &infoPtr->nWidth, TRUE);
            childSize = infoPtr->nWidth;
        }
        else
        {
            wndSize = wndRect.bottom - wndRect.top;
            PAGER_CalcSize(hwnd, &infoPtr->nHeight, FALSE);
            childSize = infoPtr->nHeight;
        }

        if (childSize > wndSize)
            scrollRange = childSize - wndSize + infoPtr->nButtonSize;
    }

    TRACE("[%04x] returns %d\n", hwnd, scrollRange);
    return scrollRange;
}

static void 
PAGER_GrayAndRestoreBtns(PAGER_INFO* infoPtr, INT scrollRange,
                         BOOL* needsResize, BOOL* needsRepaint)
{
    if (infoPtr->nPos > 0)
    {
        *needsResize |= !infoPtr->TLbtnState; /* PGF_INVISIBLE */
        if (infoPtr->TLbtnState != PGF_DEPRESSED)
            infoPtr->TLbtnState = PGF_NORMAL;
    }
    else
    {
        *needsRepaint |= (infoPtr->TLbtnState != PGF_GRAYED);
        infoPtr->TLbtnState = PGF_GRAYED;
    }

    if (scrollRange <= 0)
    {
        *needsRepaint |= (infoPtr->TLbtnState != PGF_GRAYED);
        infoPtr->TLbtnState = PGF_GRAYED;
        *needsRepaint |= (infoPtr->BRbtnState != PGF_GRAYED);
        infoPtr->BRbtnState = PGF_GRAYED;
    }
    else if (infoPtr->nPos < scrollRange)
    {
        *needsResize |= !infoPtr->BRbtnState; /* PGF_INVISIBLE */
        if (infoPtr->BRbtnState != PGF_DEPRESSED)
            infoPtr->BRbtnState = PGF_NORMAL;
    }
    else
    {
        *needsRepaint |= (infoPtr->BRbtnState != PGF_GRAYED);
        infoPtr->BRbtnState = PGF_GRAYED;
    }
}


static void 
PAGER_NormalizeBtns(PAGER_INFO* infoPtr, BOOL* needsRepaint)
{
    if (infoPtr->TLbtnState & (PGF_HOT | PGF_DEPRESSED))
    {
        infoPtr->TLbtnState = PGF_NORMAL;
        *needsRepaint = TRUE;
    }

    if (infoPtr->BRbtnState & (PGF_HOT | PGF_DEPRESSED))
    {
        infoPtr->BRbtnState = PGF_NORMAL;
        *needsRepaint = TRUE;
    }
}

static void 
PAGER_HideGrayBtns(PAGER_INFO* infoPtr, BOOL* needsResize)
{
    if (infoPtr->TLbtnState == PGF_GRAYED)
    {
        infoPtr->TLbtnState = PGF_INVISIBLE;
        *needsResize = TRUE;
    }

    if (infoPtr->BRbtnState == PGF_GRAYED)
    {
        infoPtr->BRbtnState = PGF_INVISIBLE;
        *needsResize = TRUE;
    }
}

static void
PAGER_UpdateBtns(HWND hwnd, PAGER_INFO *infoPtr,
                 INT scrollRange, BOOL hideGrayBtns)
{
    BOOL resizeClient = FALSE;
    BOOL repaintBtns = FALSE;

    if (scrollRange < 0)
        PAGER_NormalizeBtns(infoPtr, &repaintBtns);
    else
        PAGER_GrayAndRestoreBtns(infoPtr, scrollRange, &resizeClient, &repaintBtns);

    if (hideGrayBtns)
        PAGER_HideGrayBtns(infoPtr, &resizeClient);

    if (resizeClient) /* initiate NCCalcSize to resize client wnd */
        SetWindowPos(hwnd, 0,0,0,0,0, 
                     SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |
                     SWP_NOZORDER | SWP_NOACTIVATE);

    if (repaintBtns)
        SendMessageA(hwnd, WM_NCPAINT, 0, 0); 
}

static LRESULT  
PAGER_SetPos(HWND hwnd, INT newPos, BOOL fromBtnPress)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT scrollRange = PAGER_GetScrollRange(hwnd, infoPtr);

    if (scrollRange <= 0)
        newPos = 0;
    else if (newPos < 0)
        newPos = 0;
    else if (newPos > scrollRange)
        newPos = scrollRange;

    if (newPos != infoPtr->nPos)
    {
        infoPtr->nPos = newPos;
        TRACE("[%04x] pos=%d\n", hwnd, infoPtr->nPos);

        /* gray and restore btns, and if from WM_SETPOS, hide the gray btns */
        PAGER_UpdateBtns(hwnd, infoPtr, scrollRange, !fromBtnPress);

        PAGER_PositionChildWnd(hwnd, infoPtr);
    }

    return 0;
}

static LRESULT
PAGER_RecalcSize(HWND hwnd)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT scrollRange;

    TRACE("[%04x]\n", hwnd);
    scrollRange = PAGER_GetScrollRange(hwnd, infoPtr);

    if (scrollRange <= 0)
    PAGER_SetPos(hwnd, 0, FALSE);
    else 
    {
        PAGER_UpdateBtns(hwnd, infoPtr, scrollRange, TRUE);
        PAGER_PositionChildWnd(hwnd, infoPtr);
    }

    return 0;
}


static LRESULT
PAGER_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    COLORREF clrTemp = infoPtr->clrBk;

    infoPtr->clrBk = (COLORREF)lParam;
    TRACE("[%04x] %06lx\n", hwnd, infoPtr->clrBk);

    PAGER_RecalcSize(hwnd);

    return (LRESULT)clrTemp;
}


static LRESULT
PAGER_SetBorder (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nBorder;

    infoPtr->nBorder = (INT)lParam;
    TRACE("[%04x] %d\n", hwnd, infoPtr->nBorder);

    PAGER_RecalcSize(hwnd);

    return (LRESULT)nTemp;
}


static LRESULT
PAGER_SetButtonSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nButtonSize;

    infoPtr->nButtonSize = (INT)lParam;
    TRACE("[%04x] %d\n", hwnd, infoPtr->nButtonSize);

    PAGER_RecalcSize(hwnd);

    return (LRESULT)nTemp;
}



static LRESULT
PAGER_SetChild (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    infoPtr->hwndChild = IsWindow ((HWND)lParam) ? (HWND)lParam : 0;

    if (infoPtr->hwndChild)
    {
        RECT wndRect;
        INT wndSizeScrollable;

        TRACE("[%04x] hwndChild=%04x\n", hwnd, infoPtr->hwndChild);

        GetWindowRect(hwnd, &wndRect);
        wndSizeScrollable = infoPtr->bHorizontal ?
                            wndRect.right - wndRect.left :
                            wndRect.bottom - wndRect.top;

        infoPtr->nPos = 0;
        infoPtr->nDelta = wndSizeScrollable;

        PAGER_CalcSize(hwnd, &infoPtr->nWidth, TRUE);
        PAGER_CalcSize(hwnd, &infoPtr->nHeight, FALSE);

        /* adjust non-scrollable dimension to fit the child */
        SetWindowPos(hwnd, 0,
                     0,0,
                     infoPtr->bHorizontal ? wndSizeScrollable : infoPtr->nWidth,
                     infoPtr->bHorizontal ? infoPtr->nHeight : wndSizeScrollable, 
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER);

        /* position child within the page scroller */
        SetWindowPos(infoPtr->hwndChild, HWND_TOP,
                     0,0,0,0,
                     SWP_SHOWWINDOW | SWP_NOSIZE);

        PAGER_SetPos(hwnd, 0, FALSE);
    }

    return 0;
}

static void
PAGER_Scroll(HWND hwnd, INT dir)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    NMPGSCROLL nmpgScroll;

    if (infoPtr->hwndChild)
    {
        ZeroMemory (&nmpgScroll, sizeof (NMPGSCROLL));
        nmpgScroll.hdr.hwndFrom = hwnd;
        nmpgScroll.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
        nmpgScroll.hdr.code = PGN_SCROLL;

        GetClientRect(hwnd, &nmpgScroll.rcParent);  
        nmpgScroll.iXpos = nmpgScroll.iYpos = 0;
        nmpgScroll.iDir = dir;
        nmpgScroll.iScroll = infoPtr->nDelta;

        if (infoPtr->bHorizontal)
            nmpgScroll.iXpos = infoPtr->nPos;
        else
            nmpgScroll.iYpos = infoPtr->nPos;

        TRACE("[%04x] sending PGN_SCROLL\n", hwnd);
        SendMessageA (hwnd, WM_NOTIFY,
                    (WPARAM)nmpgScroll.hdr.idFrom, (LPARAM)&nmpgScroll);

        if (infoPtr->nDelta != nmpgScroll.iScroll)
        {
            TRACE("delta changing from %d to %d\n",
            infoPtr->nDelta, nmpgScroll.iScroll);
            infoPtr->nDelta = nmpgScroll.iScroll;
        }

        if (dir == PGF_SCROLLLEFT || dir == PGF_SCROLLUP)
            PAGER_SetPos(hwnd, infoPtr->nPos - infoPtr->nDelta, TRUE);
        else
            PAGER_SetPos(hwnd, infoPtr->nPos + infoPtr->nDelta, TRUE);
    }
}

static LRESULT
PAGER_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, dwStyle & WS_CLIPCHILDREN);

    /* allocate memory for info structure */
    infoPtr = (PAGER_INFO *)COMCTL32_Alloc (sizeof(PAGER_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* set default settings */
    infoPtr->hwndChild = (HWND)NULL;
    infoPtr->clrBk = GetSysColor(COLOR_BTNFACE);
    infoPtr->nBorder = 0;
    infoPtr->nButtonSize = 12;
    infoPtr->nPos = 0;
    infoPtr->nWidth = 0;
    infoPtr->nHeight = 0;
    infoPtr->bForward = FALSE;
    infoPtr->TLbtnState = PGF_INVISIBLE;
    infoPtr->BRbtnState = PGF_INVISIBLE;

    if (dwStyle & PGS_AUTOSCROLL)
        FIXME("[%04x] Autoscroll style is not implemented yet.\n", hwnd);
    if (dwStyle & PGS_DRAGNDROP)
        FIXME("[%04x] Drag and Drop style is not implemented yet.\n", hwnd);
    if ((dwStyle & PGS_HORZ) && (dwStyle & PGS_VERT))
    {
        ERR("[%04x] Cannot have both horizontal and vertical styles.\n", hwnd);
        ERR("[%04x] Defaulting to vertical.\n", hwnd);
        dwStyle &= ~PGS_HORZ;
    }
    else if (!(dwStyle & PGS_HORZ) && !(dwStyle & PGS_VERT))
        dwStyle |= PGS_VERT; /* the default according to MSDN */

    infoPtr->bHorizontal = dwStyle & PGS_HORZ;

    TRACE("[%04x] orientation = %s\n", hwnd,
                  infoPtr->bHorizontal ? "PGS_HORZ" : "PGS_VERT");

    return 0;
}


static LRESULT
PAGER_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    /* free pager info data */
    COMCTL32_Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);
    return 0;
}

static LRESULT
PAGER_NCCalcSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);                                                                         
    /*
     * lParam points to a RECT struct.  On entry, the struct
     * contains the proposed wnd rectangle for the window. 
     * On exit, the struct should contain the screen
     * coordinates of the corresponding window's client area.
     */
    LPRECT lpRect = (LPRECT)lParam;

    if (infoPtr->bHorizontal)
    {
        if (infoPtr->TLbtnState) /* != PGF_INVISIBLE */
            lpRect->left += infoPtr->nButtonSize;
        if (infoPtr->BRbtnState) 
            lpRect->right -= infoPtr->nButtonSize;
    }
    else
    {
        if (infoPtr->TLbtnState)
            lpRect->top += infoPtr->nButtonSize;
        if (infoPtr->BRbtnState)
            lpRect->bottom -= infoPtr->nButtonSize;
    }

    TRACE("[%04x] client rect set to %dx%d at (%d,%d)\n", hwnd,
                lpRect->right-lpRect->left,
                lpRect->bottom-lpRect->top,
                lpRect->left, lpRect->top);

    return 0;
}

static LRESULT
PAGER_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO* infoPtr = PAGER_GetInfoPtr(hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcWindow, rcBottomRight, rcTopLeft;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
        return 0;

    DefWindowProcA (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx (hwnd, 0, DCX_USESTYLE | DCX_WINDOW)))
        return 0;

    GetWindowRect (hwnd, &rcWindow);
    OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);

    rcTopLeft = rcBottomRight = rcWindow;
    if (infoPtr->bHorizontal)
    {
        rcTopLeft.right = rcTopLeft.left + infoPtr->nButtonSize;
        rcBottomRight.left = rcBottomRight.right - infoPtr->nButtonSize;
    }
    else
    {
        rcTopLeft.bottom = rcTopLeft.top + infoPtr->nButtonSize;
        rcBottomRight.top = rcBottomRight.bottom - infoPtr->nButtonSize;
    }

    PAGER_DrawButton(hdc, infoPtr->clrBk, rcTopLeft,
                    infoPtr->bHorizontal, TRUE, infoPtr->TLbtnState);
    PAGER_DrawButton(hdc, infoPtr->clrBk, rcBottomRight,
                    infoPtr->bHorizontal, FALSE, infoPtr->BRbtnState); 

    ReleaseDC( hwnd, hdc );
    return 0;
}

static INT 
PAGER_HitTest (HWND hwnd, LPPOINT pt)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    RECT clientRect;

    GetClientRect (hwnd, &clientRect);

    if (PtInRect(&clientRect, *pt))
    {
       /* TRACE("HTCLIENT\n"); */
        return HTCLIENT;
    }

    if (infoPtr->TLbtnState && infoPtr->TLbtnState != PGF_GRAYED)
    {
        if (infoPtr->bHorizontal)
        {
            if (pt->x < clientRect.left)
            {
                /* TRACE("HTLEFT\n"); */
                return HTLEFT;
            }
        }
        else
        {
            if (pt->y < clientRect.top)
            {
                /* TRACE("HTTOP\n"); */
                return HTTOP;
            }
        }
    }

    if (infoPtr->BRbtnState && infoPtr->BRbtnState != PGF_GRAYED)
    {
        if (infoPtr->bHorizontal)
        {
            if (pt->x > clientRect.right)
            {
                /* TRACE("HTRIGHT\n"); */
                return HTRIGHT;
            }
        }
        else
        {
            if (pt->y > clientRect.bottom)
            {
               /* TRACE("HTBOTTOM\n"); */
                return HTBOTTOM;
            }
        }
    }

    /* TRACE("HTNOWHERE\n"); */
    return HTNOWHERE;
}

static LRESULT
PAGER_NCHitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt = { SLOWORD(lParam), SHIWORD(lParam) };
    ScreenToClient (hwnd, &pt);
    return PAGER_HitTest(hwnd, &pt);
}

static LRESULT
PAGER_SetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    BOOL notCaptured = FALSE;

    switch(LOWORD(lParam))
    {
        case HTLEFT:
        case HTTOP:
            if ((notCaptured = infoPtr->TLbtnState != PGF_HOT))
                infoPtr->TLbtnState = PGF_HOT;
            break;
        case HTRIGHT:
        case HTBOTTOM:
            if ((notCaptured = infoPtr->BRbtnState != PGF_HOT))
               infoPtr->BRbtnState = PGF_HOT;
            break;
        default:
            return FALSE;
    }

    if (notCaptured)
    {
        TRACKMOUSEEVENT trackinfo;

        TRACE("[%04x] SetCapture\n", hwnd);
        SetCapture(hwnd);

        trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
        trackinfo.dwFlags = TME_QUERY;
        trackinfo.hwndTrack = hwnd;
        trackinfo.dwHoverTime = HOVER_DEFAULT;

        /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
        _TrackMouseEvent(&trackinfo);

        /* Make sure tracking is enabled so we recieve a WM_MOUSELEAVE message */
        if(!(trackinfo.dwFlags & TME_LEAVE)) {
            trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */
 
           /* call TRACKMOUSEEVENT so we recieve a WM_MOUSELEAVE message */
           /* and can properly deactivate the hot button */
           _TrackMouseEvent(&trackinfo);
        }

        SendMessageA(hwnd, WM_NCPAINT, 0, 0); 
    }

    return TRUE;
}

static LRESULT
PAGER_MouseLeave (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    TRACE("[%04x] ReleaseCapture\n", hwnd);
    ReleaseCapture();

    /* Notify parent of released mouse capture */
    if (infoPtr->hwndChild)
    {
        NMHDR nmhdr;
        ZeroMemory (&nmhdr, sizeof (NMHDR));
        nmhdr.hwndFrom = hwnd;
        nmhdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
        nmhdr.code = NM_RELEASEDCAPTURE;
        SendMessageA (GetParent (infoPtr->hwndChild), WM_NOTIFY,
                        (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
    }

    /* make HOT btns NORMAL and hide gray btns */
    PAGER_UpdateBtns(hwnd, infoPtr, -1, TRUE);

    return TRUE;
}

static LRESULT
PAGER_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    BOOL repaintBtns = FALSE;
    POINT pt = { SLOWORD(lParam), SHIWORD(lParam) };
    INT hit;

    TRACE("[%04x]\n", hwnd);
	
    if (infoPtr->nDelta <= 0)
        return FALSE;

    hit = PAGER_HitTest(hwnd, &pt);

    /* put btn in DEPRESSED state */
    if (hit == HTLEFT || hit == HTTOP)
    {
        repaintBtns = infoPtr->TLbtnState != PGF_DEPRESSED;
        infoPtr->TLbtnState = PGF_DEPRESSED;
    }
    else if (hit == HTRIGHT || hit == HTBOTTOM)
    {
        repaintBtns = infoPtr->BRbtnState != PGF_DEPRESSED;
        infoPtr->BRbtnState = PGF_DEPRESSED;
    }

    if (repaintBtns)
        SendMessageA(hwnd, WM_NCPAINT, 0, 0); 

    switch(hit)
    {
    case HTLEFT:
        TRACE("[%04x] PGF_SCROLLLEFT\n", hwnd);
        PAGER_Scroll(hwnd, PGF_SCROLLLEFT);
        break;
    case HTTOP:
        TRACE("[%04x] PGF_SCROLLUP\n", hwnd);
        PAGER_Scroll(hwnd, PGF_SCROLLUP);
        break;
    case HTRIGHT:
        TRACE("[%04x] PGF_SCROLLRIGHT\n", hwnd);
        PAGER_Scroll(hwnd, PGF_SCROLLRIGHT);
        break;
    case HTBOTTOM:
        TRACE("[%04x] PGF_SCROLLDOWN\n", hwnd);
        PAGER_Scroll(hwnd, PGF_SCROLLDOWN);
        break;
    default:
        break;
    }

    return TRUE;
}

static LRESULT
PAGER_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    TRACE("[%04x]\n", hwnd);

    /* make PRESSED btns NORMAL but don't hide gray btns */
    PAGER_UpdateBtns(hwnd, infoPtr, -1, FALSE);

    return 0;
}

static LRESULT
PAGER_EraseBackground (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    HBRUSH hBrush = CreateSolidBrush(infoPtr->clrBk);
    RECT rect;
    GetClientRect (hwnd, &rect);

    FillRect ((HDC)wParam, &rect, hBrush);
    DeleteObject (hBrush);
    return TRUE;
}


static LRESULT
PAGER_Size (HWND hwnd)
{
    /* note that WM_SIZE is sent whenever NCCalcSize resizes the client wnd */

    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);
    RECT wndRect;
    GetWindowRect(hwnd, &wndRect);

    infoPtr->nDelta = infoPtr->bHorizontal ?
                      wndRect.right - wndRect.left :
                      wndRect.bottom - wndRect.top;
    infoPtr->nDelta -= 2*infoPtr->nButtonSize;
	
    TRACE("[%04x] nDelta=%d\n", hwnd, infoPtr->nDelta);

    PAGER_PositionChildWnd(hwnd, infoPtr);
	
    return TRUE;
}


static LRESULT WINAPI
PAGER_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAGER_INFO *infoPtr = PAGER_GetInfoPtr (hwnd);

    if (!infoPtr && (uMsg != WM_CREATE))
    return DefWindowProcA (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
        case PGM_FORWARDMOUSE:
            return PAGER_ForwardMouse (hwnd, wParam);

        case PGM_GETBKCOLOR:
            return PAGER_GetBkColor(hwnd);

        case PGM_GETBORDER:
            return PAGER_GetBorder(hwnd);

        case PGM_GETBUTTONSIZE:
            return PAGER_GetButtonSize(hwnd);

        case PGM_GETPOS:
            return PAGER_GetPos(hwnd);

        case PGM_GETBUTTONSTATE:
            return PAGER_GetButtonState (hwnd, wParam, lParam);

/*      case PGM_GETDROPTARGET: */

        case PGM_RECALCSIZE:
            return PAGER_RecalcSize(hwnd);
    
        case PGM_SETBKCOLOR:
            return PAGER_SetBkColor (hwnd, wParam, lParam);

        case PGM_SETBORDER:
            return PAGER_SetBorder (hwnd, wParam, lParam);

        case PGM_SETBUTTONSIZE:
            return PAGER_SetButtonSize (hwnd, wParam, lParam);

        case PGM_SETCHILD:
            return PAGER_SetChild (hwnd, wParam, lParam);

        case PGM_SETPOS:
            return PAGER_SetPos(hwnd, (INT)lParam, FALSE);

        case WM_CREATE:
            return PAGER_Create (hwnd, wParam, lParam);

        case WM_DESTROY:
            return PAGER_Destroy (hwnd, wParam, lParam);

        case WM_SIZE:
            return PAGER_Size (hwnd);

        case WM_NCPAINT:
            return PAGER_NCPaint (hwnd, wParam, lParam);

        case WM_NCCALCSIZE:
            return PAGER_NCCalcSize (hwnd, wParam, lParam);

        case WM_NCHITTEST:
            return PAGER_NCHitTest (hwnd, wParam, lParam);

        case WM_SETCURSOR:
        {
            if (hwnd == (HWND)wParam)
                return PAGER_SetCursor(hwnd, wParam, lParam);
            else /* its for the child */
                return 0;
        }

        case WM_MOUSEMOVE:
            if (infoPtr->bForward && infoPtr->hwndChild)
                PostMessageA(infoPtr->hwndChild, WM_MOUSEMOVE, wParam, lParam);
            return TRUE;			

        case WM_MOUSELEAVE:
            return PAGER_MouseLeave (hwnd, wParam, lParam);	

        case WM_LBUTTONDOWN:
            return PAGER_LButtonDown (hwnd, wParam, lParam);

        case WM_LBUTTONUP:
            return PAGER_LButtonUp (hwnd, wParam, lParam);

        case WM_ERASEBKGND:
            return PAGER_EraseBackground (hwnd, wParam, lParam);
/*
        case WM_PAINT:
            return PAGER_Paint (hwnd, wParam); 
*/
        case WM_NOTIFY:
        case WM_COMMAND:
            return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

        default:
            return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


VOID
PAGER_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS;
    wndClass.lpfnWndProc   = (WNDPROC)PAGER_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PAGER_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_PAGESCROLLERA;
 
    RegisterClassA (&wndClass);
}


VOID
PAGER_Unregister (void)
{
    UnregisterClassA (WC_PAGESCROLLERA, (HINSTANCE)NULL);
}

