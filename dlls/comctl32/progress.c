/*		
 * Progress control
 *
 * Copyright 1997, 2002 Dimitrie O. Paun
 * Copyright 1998, 1999 Eric Kohl
 *
 */

#include <string.h>
#include "winbase.h"
#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(progress);

typedef struct
{
    HWND      Self;         /* The window handle for this control */
    INT       CurVal;       /* Current progress value */
    INT       MinVal;       /* Minimum progress value */
    INT       MaxVal;       /* Maximum progress value */
    INT       Step;         /* Step to use on PMB_STEPIT */
    COLORREF  ColorBar;     /* Bar color */
    COLORREF  ColorBk;      /* Background color */
    HFONT     Font;         /* Handle to font (not unused) */
} PROGRESS_INFO;

/* Control configuration constants */

#define LED_GAP    2

#define UNKNOWN_PARAM(msg, wParam, lParam) WARN(       \
   "Unknown parameter(s) for message " #msg            \
   "(%04x): wp=%04x lp=%08lx\n", msg, wParam, lParam); 

/***********************************************************************
 * PROGRESS_Draw
 * Draws the progress bar.
 */
static LRESULT PROGRESS_Draw (PROGRESS_INFO *infoPtr, HDC hdc)
{
    HBRUSH hbrBar, hbrBk;
    int rightBar, rightMost, ledWidth;
    RECT rect;
    DWORD dwStyle;

    TRACE("(infoPtr=%p, hdc=%x)\n", infoPtr, hdc);

    /* get the required bar brush */
    if (infoPtr->ColorBar == CLR_DEFAULT)
        hbrBar = GetSysColorBrush(COLOR_HIGHLIGHT);
    else
        hbrBar = CreateSolidBrush (infoPtr->ColorBar);

    /* get the required background brush */
    if (infoPtr->ColorBk == CLR_DEFAULT)
        hbrBk = GetSysColorBrush (COLOR_3DFACE);
    else
        hbrBk = CreateSolidBrush (infoPtr->ColorBk);

    /* get client rectangle */
    GetClientRect (infoPtr->Self, &rect);

    /* draw the background */
    FillRect(hdc, &rect, hbrBk);

    rect.left++; rect.right--; rect.top++; rect.bottom--;

    /* get the window style */
    dwStyle = GetWindowLongW (infoPtr->Self, GWL_STYLE);

    /* compute extent of progress bar */
    if (dwStyle & PBS_VERTICAL) {
        rightBar  = rect.bottom - 
                    MulDiv (infoPtr->CurVal - infoPtr->MinVal,
	                    rect.bottom - rect.top,
	                    infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth  = MulDiv (rect.right - rect.left, 2, 3);
        rightMost = rect.top;
    } else {
        rightBar = rect.left + 
                   MulDiv (infoPtr->CurVal - infoPtr->MinVal,
	                   rect.right - rect.left,
	                   infoPtr->MaxVal - infoPtr->MinVal);
        ledWidth = MulDiv (rect.bottom - rect.top, 2, 3);
        rightMost = rect.right;
    }

    /* now draw the bar */
    if (dwStyle & PBS_SMOOTH) {
        if (dwStyle & PBS_VERTICAL) 
	    rect.top = rightBar;
        else 
	    rect.right = rightBar;
        FillRect(hdc, &rect, hbrBar);
    } else {
        if (dwStyle & PBS_VERTICAL) {
            while(rect.bottom > rightBar) { 
                rect.top = rect.bottom - ledWidth;
                if (rect.top < rightMost)
                    rect.top = rightMost;
                FillRect(hdc, &rect, hbrBar);
                rect.bottom = rect.top - LED_GAP;
            }
        } else {
            while(rect.left < rightBar) { 
                rect.right = rect.left + ledWidth;
                if (rect.right > rightMost)
                    rect.right = rightMost;
                FillRect(hdc, &rect, hbrBar);
                rect.left  = rect.right + LED_GAP;
            }
        }
    }

    /* delete bar brush */
    if (infoPtr->ColorBar != CLR_DEFAULT)
        DeleteObject (hbrBar);

    /* delete background brush */
    if (infoPtr->ColorBk != CLR_DEFAULT)
        DeleteObject (hbrBk);

    return 0;
}

/***********************************************************************
 * PROGRESS_Refresh
 * Draw the progress bar. The background need not be erased.
 */
static LRESULT PROGRESS_Refresh (PROGRESS_INFO *infoPtr)
{
    HDC hdc = GetDC (infoPtr->Self);
    LRESULT res = PROGRESS_Draw (infoPtr, hdc);
    ReleaseDC (infoPtr->Self, hdc);
    return res;
}

/***********************************************************************
 * PROGRESS_Paint
 * Draw the progress bar. The background need not be erased.
 * If dc!=0, it draws on it
 */
static LRESULT PROGRESS_Paint (PROGRESS_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;
    if (hdc) return PROGRESS_Draw (infoPtr, hdc);
    hdc = BeginPaint (infoPtr->Self, &ps);
    PROGRESS_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}


/***********************************************************************
 *           PROGRESS_CoercePos
 * Makes sure the current position (CurVal) is within bounds.
 */
static void PROGRESS_CoercePos(PROGRESS_INFO *infoPtr)
{
    if(infoPtr->CurVal < infoPtr->MinVal)
        infoPtr->CurVal = infoPtr->MinVal;
    if(infoPtr->CurVal > infoPtr->MaxVal)
        infoPtr->CurVal = infoPtr->MaxVal;
}


/***********************************************************************
 *           PROGRESS_SetFont
 * Set new Font for progress bar
 */
static HFONT PROGRESS_SetFont (PROGRESS_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    HFONT hOldFont = infoPtr->Font;
    infoPtr->Font = hFont;
    if (bRedraw) PROGRESS_Refresh (infoPtr);
    return hOldFont;
}

static DWORD PROGRESS_SetRange (PROGRESS_INFO *infoPtr, int low, int high)
{
    DWORD res = MAKELONG(LOWORD(infoPtr->MinVal), LOWORD(infoPtr->MaxVal));

    /* if nothing changes, simply return */
    if(infoPtr->MinVal == low && infoPtr->MaxVal == high) return res;
    
    /* if things are different, adjust values and repaint the control */
    if (high <= low) high = low + 1;
    infoPtr->MinVal = low;
    infoPtr->MaxVal = high;
    PROGRESS_CoercePos(infoPtr);
    PROGRESS_Refresh (infoPtr);
    return res;
}

/***********************************************************************
 *           ProgressWindowProc
 */
static LRESULT WINAPI ProgressWindowProc(HWND hwnd, UINT message, 
                                  WPARAM wParam, LPARAM lParam)
{
    PROGRESS_INFO *infoPtr = (PROGRESS_INFO *)GetWindowLongW(hwnd, 0);
    DWORD dwExStyle;
    UINT temp;
  
    if (!infoPtr && message != WM_CREATE && message != WM_NCCREATE)
        return DefWindowProcW( hwnd, message, wParam, lParam ); 
  
    switch(message) {
    case WM_NCCREATE:
        dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);
        SetWindowLongW (hwnd, GWL_EXSTYLE, dwExStyle | WS_EX_STATICEDGE);
        return TRUE;

    case WM_CREATE:
        /* allocate memory for info struct */
        infoPtr = (PROGRESS_INFO *)COMCTL32_Alloc (sizeof(PROGRESS_INFO));
        if (!infoPtr) return -1;
        SetWindowLongW (hwnd, 0, (DWORD)infoPtr);

        /* initialize the info struct */
        infoPtr->Self = hwnd;
        infoPtr->MinVal = 0; 
        infoPtr->MaxVal = 100;
        infoPtr->CurVal = 0; 
        infoPtr->Step = 10;
        infoPtr->ColorBar = CLR_DEFAULT;
        infoPtr->ColorBk = CLR_DEFAULT;
        infoPtr->Font = 0;
        TRACE("Progress Ctrl creation, hwnd=%04x\n", hwnd);
        return 0;
    
    case WM_DESTROY:
        TRACE("Progress Ctrl destruction, hwnd=%04x\n", hwnd);
        COMCTL32_Free (infoPtr);
        SetWindowLongW(hwnd, 0, 0);
        return 0;

    case WM_ERASEBKGND:
        /* pretend to erase it here, but we will do it in the paint
	   function to avoid flicker */
        return TRUE;
	
    case WM_GETFONT:
        return (LRESULT)infoPtr->Font;

    case WM_SETFONT:
        return PROGRESS_SetFont (infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_PAINT:
        return PROGRESS_Paint (infoPtr, (HDC)wParam);
    
    case PBM_DELTAPOS:
        if(lParam) UNKNOWN_PARAM(PBM_DELTAPOS, wParam, lParam);
        temp = infoPtr->CurVal;
        if(wParam != 0) {
	    infoPtr->CurVal += (WORD)wParam;
	    PROGRESS_CoercePos (infoPtr);
	    PROGRESS_Refresh (infoPtr);
        }
        return temp;

    case PBM_SETPOS:
        if (lParam) UNKNOWN_PARAM(PBM_SETPOS, wParam, lParam);
        temp = infoPtr->CurVal;
        if(temp != wParam) {
	    infoPtr->CurVal = (WORD)wParam;
	    PROGRESS_CoercePos(infoPtr);
	    PROGRESS_Refresh (infoPtr);
        }
        return temp;          
      
    case PBM_SETRANGE:
        if (wParam) UNKNOWN_PARAM(PBM_SETRANGE, wParam, lParam);
        return PROGRESS_SetRange (infoPtr, (int)LOWORD(lParam), (int)HIWORD(lParam));

    case PBM_SETSTEP:
        if (lParam) UNKNOWN_PARAM(PBM_SETSTEP, wParam, lParam);
        temp = infoPtr->Step;   
        infoPtr->Step = (WORD)wParam;   
        return temp;

    case PBM_STEPIT:
        if (wParam || lParam) UNKNOWN_PARAM(PBM_STEPIT, wParam, lParam);
        temp = infoPtr->CurVal;   
        infoPtr->CurVal += infoPtr->Step;
        if(infoPtr->CurVal > infoPtr->MaxVal)
	    infoPtr->CurVal = infoPtr->MinVal;
        if(temp != infoPtr->CurVal)
	    PROGRESS_Refresh (infoPtr);
        return temp;

    case PBM_SETRANGE32:
        return PROGRESS_SetRange (infoPtr, (int)wParam, (int)lParam);
    
    case PBM_GETRANGE:
        if (lParam) {
            ((PPBRANGE)lParam)->iLow = infoPtr->MinVal;
            ((PPBRANGE)lParam)->iHigh = infoPtr->MaxVal;
        }
        return wParam ? infoPtr->MinVal : infoPtr->MaxVal;

    case PBM_GETPOS:
        if (wParam || lParam) UNKNOWN_PARAM(PBM_STEPIT, wParam, lParam);
        return infoPtr->CurVal;

    case PBM_SETBARCOLOR:
        if (wParam) UNKNOWN_PARAM(PBM_SETBARCOLOR, wParam, lParam);
        infoPtr->ColorBar = (COLORREF)lParam;     
        return PROGRESS_Refresh (infoPtr);

    case PBM_SETBKCOLOR:
        if (wParam) UNKNOWN_PARAM(PBM_SETBKCOLOR, wParam, lParam);
        infoPtr->ColorBk = (COLORREF)lParam;
        return PROGRESS_Refresh (infoPtr);

    default: 
        if (message >= WM_USER) 
	    ERR("unknown msg %04x wp=%04x lp=%08lx\n", message, wParam, lParam );
        return DefWindowProcW( hwnd, message, wParam, lParam ); 
    } 
}


/***********************************************************************
 * PROGRESS_Register [Internal]
 *
 * Registers the progress bar window class.
 */
VOID PROGRESS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(wndClass));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)ProgressWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (PROGRESS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, IDC_ARROWW);
    wndClass.lpszClassName = PROGRESS_CLASSW;

    RegisterClassW (&wndClass);
}


/***********************************************************************
 * PROGRESS_Unregister [Internal]
 *
 * Unregisters the progress bar window class.
 */
VOID PROGRESS_Unregister (void)
{
    UnregisterClassW (PROGRESS_CLASSW, (HINSTANCE)NULL);
}

