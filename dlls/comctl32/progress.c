/*		
 * Progress control
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 */

#include "commctrl.h"
#include "progress.h"
#include "win.h"
#include "debug.h"


/* Control configuration constants */

#define LED_GAP    2

/* Work constants */

#define UNKNOWN_PARAM(msg, wParam, lParam) WARN(progress, \
        "Unknown parameter(s) for message " #msg     \
	"(%04x): wp=%04x lp=%08lx\n", msg, wParam, lParam); 

#define PROGRESS_GetInfoPtr(wndPtr) ((PROGRESS_INFO *)wndPtr->wExtra[0])


/***********************************************************************
 * PROGRESS_Draw
 * Draws the progress bar.
 */
static void
PROGRESS_Draw (WND *wndPtr, HDC hdc)
{
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr);
  HBRUSH hbrBar, hbrBk;
  int rightBar, rightMost, ledWidth;
  RECT rect;

  TRACE(progress, "refresh pos=%d min=%d, max=%d\n",
	       infoPtr->CurVal, infoPtr->MinVal, infoPtr->MaxVal);

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
  GetClientRect (wndPtr->hwndSelf, &rect);

  /* draw the background */
  FillRect(hdc, &rect, hbrBk);

  rect.left++; rect.right--; rect.top++; rect.bottom--;

  /* compute extent of progress bar */
  if (wndPtr->dwStyle & PBS_VERTICAL)
  {
    rightBar = rect.bottom - 
      MulDiv(infoPtr->CurVal-infoPtr->MinVal,
	       rect.bottom - rect.top,
	       infoPtr->MaxVal-infoPtr->MinVal);
    ledWidth = MulDiv ((rect.right - rect.left), 2, 3);
    rightMost = rect.top;
  }
  else
  {
    rightBar = rect.left + 
      MulDiv(infoPtr->CurVal-infoPtr->MinVal,
	       rect.right - rect.left,
	       infoPtr->MaxVal-infoPtr->MinVal);
    ledWidth = MulDiv ((rect.bottom - rect.top), 2, 3);
    rightMost = rect.right;
  }

  /* now draw the bar */
  if (wndPtr->dwStyle & PBS_SMOOTH)
  {
    if (wndPtr->dwStyle & PBS_VERTICAL)
      rect.top = rightBar;
    else
      rect.right = rightBar;
    FillRect(hdc, &rect, hbrBar);
  }
  else
  {
    if (wndPtr->dwStyle & PBS_VERTICAL)
      while(rect.bottom > rightBar) { 
        rect.top = rect.bottom-ledWidth;
        if (rect.top < rightMost)
          rect.top = rightMost;
        FillRect(hdc, &rect, hbrBar);
        rect.bottom = rect.top-LED_GAP;
      }
    else
      while(rect.left < rightBar) { 
        rect.right = rect.left+ledWidth;
        if (rect.right > rightMost)
          rect.right = rightMost;
        FillRect(hdc, &rect, hbrBar);
        rect.left  = rect.right+LED_GAP;
      }
  }

  /* delete bar brush */
  if (infoPtr->ColorBar != CLR_DEFAULT)
      DeleteObject (hbrBar);

  /* delete background brush */
  if (infoPtr->ColorBk != CLR_DEFAULT)
      DeleteObject (hbrBk);
}

/***********************************************************************
 * PROGRESS_Refresh
 * Draw the progress bar. The background need not be erased.
 */
static void
PROGRESS_Refresh (WND *wndPtr)
{
    HDC hdc;

    hdc = GetDC (wndPtr->hwndSelf);
    PROGRESS_Draw (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);
}

/***********************************************************************
 * PROGRESS_Paint
 * Draw the progress bar. The background need not be erased.
 * If dc!=0, it draws on it
 */
static void
PROGRESS_Paint (WND *wndPtr)
{
    PAINTSTRUCT ps;
    HDC hdc;

    hdc = BeginPaint (wndPtr->hwndSelf, &ps);
    PROGRESS_Draw (wndPtr, hdc);
    EndPaint (wndPtr->hwndSelf, &ps);
}


/***********************************************************************
 *           PROGRESS_CoercePos
 * Makes sure the current position (CUrVal) is within bounds.
 */
static void PROGRESS_CoercePos(WND *wndPtr)
{
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr); 

  if(infoPtr->CurVal < infoPtr->MinVal)
    infoPtr->CurVal = infoPtr->MinVal;
  if(infoPtr->CurVal > infoPtr->MaxVal)
    infoPtr->CurVal = infoPtr->MaxVal;
}


/***********************************************************************
 *           PROGRESS_SetFont
 * Set new Font for progress bar
 */
static HFONT
PROGRESS_SetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr); 
  HFONT hOldFont = infoPtr->hFont;

  infoPtr->hFont = (HFONT)wParam;
  if (LOWORD(lParam))
    PROGRESS_Refresh (wndPtr);
  return hOldFont;
}


/***********************************************************************
 *           ProgressWindowProc
 */
LRESULT WINAPI ProgressWindowProc(HWND hwnd, UINT message, 
                                  WPARAM wParam, LPARAM lParam)
{
  WND *wndPtr = WIN_FindWndPtr(hwnd);
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr); 
  UINT temp;

  switch(message)
    {
    case WM_NCCREATE:
      wndPtr->dwExStyle |= WS_EX_STATICEDGE;
      return TRUE;

    case WM_CREATE:
      /* allocate memory for info struct */
      infoPtr = 
	(PROGRESS_INFO *)COMCTL32_Alloc (sizeof(PROGRESS_INFO));
      wndPtr->wExtra[0] = (DWORD)infoPtr;

      /* initialize the info struct */
      infoPtr->MinVal=0; 
      infoPtr->MaxVal=100;
      infoPtr->CurVal=0; 
      infoPtr->Step=10;
      infoPtr->ColorBar=CLR_DEFAULT;
      infoPtr->ColorBk=CLR_DEFAULT;
      infoPtr->hFont=(HANDLE)NULL;
      TRACE(progress, "Progress Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      TRACE (progress, "Progress Ctrl destruction, hwnd=%04x\n", hwnd);
      COMCTL32_Free (infoPtr);
      break;

    case WM_ERASEBKGND:
      /* pretend to erase it here, but we will do it in the paint
	 function to avoid flicker */
      return 1;
	
    case WM_GETFONT:
      return (LRESULT)infoPtr->hFont;

    case WM_SETFONT:
      return PROGRESS_SetFont (wndPtr, wParam, lParam);
/*      break; */

    case WM_PAINT:
      PROGRESS_Paint (wndPtr);
      break;
    
    case PBM_DELTAPOS:
      if(lParam)
	UNKNOWN_PARAM(PBM_DELTAPOS, wParam, lParam);
      temp = infoPtr->CurVal;
      if(wParam != 0){
	infoPtr->CurVal += (UINT16)wParam;
	PROGRESS_CoercePos(wndPtr);
	PROGRESS_Refresh (wndPtr);
      }
      return temp;

    case PBM_SETPOS:
      if (lParam)
	UNKNOWN_PARAM(PBM_SETPOS, wParam, lParam);
      temp = infoPtr->CurVal;
      if(temp != wParam){
	infoPtr->CurVal = (UINT16)wParam;
	PROGRESS_CoercePos(wndPtr);
	PROGRESS_Refresh (wndPtr);
      }
      return temp;          
      
    case PBM_SETRANGE16:
      if (wParam)
	UNKNOWN_PARAM(PBM_SETRANGE16, wParam, lParam);
      temp = MAKELONG(infoPtr->MinVal, infoPtr->MaxVal);
      if(temp != lParam){
	infoPtr->MinVal = LOWORD(lParam); 
	infoPtr->MaxVal = HIWORD(lParam);
	if(infoPtr->MaxVal <= infoPtr->MinVal)
	  infoPtr->MaxVal = infoPtr->MinVal+1;
	PROGRESS_CoercePos(wndPtr);
	PROGRESS_Refresh (wndPtr);
      }
      return temp;

    case PBM_SETSTEP:
      if (lParam)
	UNKNOWN_PARAM(PBM_SETSTEP, wParam, lParam);
      temp = infoPtr->Step;   
      infoPtr->Step = (UINT16)wParam;   
      return temp;

    case PBM_STEPIT:
      if (wParam || lParam)
	UNKNOWN_PARAM(PBM_STEPIT, wParam, lParam);
      temp = infoPtr->CurVal;   
      infoPtr->CurVal += infoPtr->Step;
      if(infoPtr->CurVal > infoPtr->MaxVal)
	infoPtr->CurVal = infoPtr->MinVal;
      if(temp != infoPtr->CurVal)
	PROGRESS_Refresh (wndPtr);
      return temp;

    case PBM_SETRANGE:
      temp = MAKELONG(infoPtr->MinVal, infoPtr->MaxVal);
      if((infoPtr->MinVal != (INT)wParam) ||
         (infoPtr->MaxVal != (INT)lParam)) {
	infoPtr->MinVal = (INT)wParam;
	infoPtr->MaxVal = (INT)lParam;
	if(infoPtr->MaxVal <= infoPtr->MinVal)
	  infoPtr->MaxVal = infoPtr->MinVal+1;
	PROGRESS_CoercePos(wndPtr);
	PROGRESS_Refresh (wndPtr);
      }
      return temp;
    
    case PBM_GETRANGE:
      if (lParam){
        ((PPBRANGE)lParam)->iLow = infoPtr->MinVal;
        ((PPBRANGE)lParam)->iHigh = infoPtr->MaxVal;
      }
      return (wParam) ? infoPtr->MinVal : infoPtr->MaxVal;

    case PBM_GETPOS:
      if (wParam || lParam)
	UNKNOWN_PARAM(PBM_STEPIT, wParam, lParam);
      return (infoPtr->CurVal);

    case PBM_SETBARCOLOR:
      if (wParam)
	UNKNOWN_PARAM(PBM_SETBARCOLOR, wParam, lParam);
      infoPtr->ColorBar = (COLORREF)lParam;     
      PROGRESS_Refresh (wndPtr);
      break;

    case PBM_SETBKCOLOR:
      if (wParam)
	UNKNOWN_PARAM(PBM_SETBKCOLOR, wParam, lParam);
      infoPtr->ColorBk = (COLORREF)lParam;
      PROGRESS_Refresh (wndPtr);
      break;

    default: 
      if (message >= WM_USER) 
	ERR(progress, "unknown msg %04x wp=%04x lp=%08lx\n", 
		    message, wParam, lParam );
      return DefWindowProcA( hwnd, message, wParam, lParam ); 
    } 

    return 0;
}


/***********************************************************************
 * PROGRESS_Register [Internal]
 *
 * Registers the progress bar window class.
 */

VOID
PROGRESS_Register (VOID)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (PROGRESS_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof( WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)ProgressWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (PROGRESS_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.lpszClassName = PROGRESS_CLASSA;

    RegisterClassA (&wndClass);
}


/***********************************************************************
 * PROGRESS_Unregister [Internal]
 *
 * Unregisters the progress bar window class.
 */

VOID
PROGRESS_Unregister (VOID)
{
    if (GlobalFindAtomA (PROGRESS_CLASSA))
	UnregisterClassA (PROGRESS_CLASSA, (HINSTANCE)NULL);
}

