/*		
 * Progress control
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 * TODO:
 *   - I do not know what to to on WM_[SG]ET_FONT
 */

#include "windows.h"
#include "progress.h"
#include "commctrl.h"
#include "heap.h"
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
 *           PROGRESS_Paint
 * Draw the arrows. The background need not be erased.
 * If dc!=0, it draws on it
 */
static void PROGRESS_Paint(WND *wndPtr, HDC32 dc)
{
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr);
  HBRUSH32 hbrBar, hbrBk;
  int rightBar, rightMost, ledWidth;
  PAINTSTRUCT32 ps;
  RECT32 rect;
  HDC32 hdc;

  TRACE(progress, "paint pos=%d min=%d, max=%d\n",
	       infoPtr->CurVal, infoPtr->MinVal, infoPtr->MaxVal);

  /* get a dc */
  hdc = dc==0 ? BeginPaint32(wndPtr->hwndSelf, &ps) : dc;

  /* get the required bar brush */
  if (infoPtr->ColorBar == CLR_DEFAULT)
    hbrBar = GetSysColorBrush32(COLOR_HIGHLIGHT);
  else
    hbrBar = CreateSolidBrush32 (infoPtr->ColorBar);

  /* get the required background brush */
  if (infoPtr->ColorBk == CLR_DEFAULT)
    hbrBk = GetSysColorBrush32 (COLOR_3DFACE);
  else
    hbrBk = CreateSolidBrush32 (infoPtr->ColorBk);

  /* get rect for the bar, adjusted for the border */
  GetClientRect32 (wndPtr->hwndSelf, &rect);

  /* draw the border */
  DrawEdge32(hdc, &rect, BDR_SUNKENOUTER, BF_RECT|BF_ADJUST);
  FillRect32(hdc, &rect, hbrBk);

  rect.left++; rect.right--; rect.top++; rect.bottom--;

  /* compute extent of progress bar */
  if (wndPtr->dwStyle & PBS_VERTICAL)
  {
    rightBar = rect.bottom - 
      MulDiv32(infoPtr->CurVal-infoPtr->MinVal,
	       rect.bottom - rect.top,
	       infoPtr->MaxVal-infoPtr->MinVal);
    ledWidth = MulDiv32 ((rect.right - rect.left), 2, 3);
    rightMost = rect.top;
  }
  else
  {
    rightBar = rect.left + 
      MulDiv32(infoPtr->CurVal-infoPtr->MinVal,
	       rect.right - rect.left,
	       infoPtr->MaxVal-infoPtr->MinVal);
    ledWidth = MulDiv32 ((rect.bottom - rect.top), 2, 3);
    rightMost = rect.right;
  }

  /* now draw the bar */
  if (wndPtr->dwStyle & PBS_SMOOTH)
  {
    if (wndPtr->dwStyle & PBS_VERTICAL)
      rect.top = rightBar;
    else
      rect.right = rightBar;
    FillRect32(hdc, &rect, hbrBar);
  }
  else
  {
    if (wndPtr->dwStyle & PBS_VERTICAL)
      while(rect.bottom > rightBar) { 
        rect.top = rect.bottom-ledWidth;
        if (rect.top < rightMost)
          rect.top = rightMost;
        FillRect32(hdc, &rect, hbrBar);
        rect.bottom = rect.top-LED_GAP;
      }
    else
      while(rect.left < rightBar) { 
        rect.right = rect.left+ledWidth;
        if (rect.right > rightMost)
          rect.right = rightMost;
        FillRect32(hdc, &rect, hbrBar);
        rect.left  = rect.right+LED_GAP;
      }
  }

  /* delete bar brush */
  if (infoPtr->ColorBar != CLR_DEFAULT)
      DeleteObject32 (hbrBar);

  /* delete background brush */
  if (infoPtr->ColorBk != CLR_DEFAULT)
      DeleteObject32 (hbrBk);

  /* clean-up */  
  if(!dc)
    EndPaint32(wndPtr->hwndSelf, &ps);
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
 *           ProgressWindowProc
 */
LRESULT WINAPI ProgressWindowProc(HWND32 hwnd, UINT32 message, 
                                  WPARAM32 wParam, LPARAM lParam)
{
  WND *wndPtr = WIN_FindWndPtr(hwnd);
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr); 
  UINT32 temp;

  switch(message)
    {
    case WM_CREATE:
      /* allocate memory for info struct */
      wndPtr->wExtra[0] = HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
                                     sizeof(PROGRESS_INFO));
      infoPtr = (PROGRESS_INFO *)wndPtr->wExtra[0];
      /* initialize the info struct */
      infoPtr->MinVal=0; 
      infoPtr->MaxVal=100;
      infoPtr->CurVal=0; 
      infoPtr->Step=10;
      infoPtr->ColorBar=CLR_DEFAULT;
      infoPtr->ColorBk=CLR_DEFAULT;
      TRACE(updown, "Progress Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      TRACE(updown, "Progress Ctrl destruction, hwnd=%04x\n", hwnd);
      HeapFree (SystemHeap, 0, infoPtr);
      break;

    case WM_ERASEBKGND:
      /* pretend to erase it here, but we will do it in the paint
	 function to avoid flicker */
      return 1;
	
    case WM_GETFONT:
      /* FIXME: What do we need to do? */
      break;

    case WM_SETFONT:
      /* FIXME: What do we need to do? */
      break;

    case WM_PAINT:
      PROGRESS_Paint(wndPtr, wParam);
      break;
    
    case PBM_DELTAPOS:
      if(lParam)
	UNKNOWN_PARAM(PBM_DELTAPOS, wParam, lParam);
      temp = infoPtr->CurVal;
      if(wParam != 0){
	infoPtr->CurVal += (UINT16)wParam;
	PROGRESS_CoercePos(wndPtr);
        InvalidateRect32 (hwnd, NULL, FALSE);
        UpdateWindow32 (hwnd);
      }
      return temp;

    case PBM_SETPOS:
      if (lParam)
	UNKNOWN_PARAM(PBM_SETPOS, wParam, lParam);
      temp = infoPtr->CurVal;
      if(temp != wParam){
	infoPtr->CurVal = (UINT16)wParam;
	PROGRESS_CoercePos(wndPtr);
        InvalidateRect32 (hwnd, NULL, FALSE);
        UpdateWindow32 (hwnd);
      }
      return temp;          
      
    case PBM_SETRANGE:
      if (wParam)
	UNKNOWN_PARAM(PBM_SETRANGE, wParam, lParam);
      temp = MAKELONG(infoPtr->MinVal, infoPtr->MaxVal);
      if(temp != lParam){
	infoPtr->MinVal = LOWORD(lParam); 
	infoPtr->MaxVal = HIWORD(lParam);
	if(infoPtr->MaxVal <= infoPtr->MinVal)
	  infoPtr->MaxVal = infoPtr->MinVal+1;
	PROGRESS_CoercePos(wndPtr);
        InvalidateRect32 (hwnd, NULL, FALSE);
        UpdateWindow32 (hwnd);
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
      {
        InvalidateRect32 (hwnd, NULL, FALSE);
        UpdateWindow32 (hwnd);
      }
      return temp;

    case PBM_SETRANGE32:
      temp = MAKELONG(infoPtr->MinVal, infoPtr->MaxVal);
      if((infoPtr->MinVal != (INT32)wParam) ||
         (infoPtr->MaxVal != (INT32)lParam)) {
	infoPtr->MinVal = (INT32)wParam;
	infoPtr->MaxVal = (INT32)lParam;
	if(infoPtr->MaxVal <= infoPtr->MinVal)
	  infoPtr->MaxVal = infoPtr->MinVal+1;
	PROGRESS_CoercePos(wndPtr);
        InvalidateRect32 (hwnd, NULL, FALSE);
        UpdateWindow32 (hwnd);
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
      InvalidateRect32 (hwnd, NULL, FALSE);
      UpdateWindow32 (hwnd);
      break;

    case PBM_SETBKCOLOR:
      if (wParam)
	UNKNOWN_PARAM(PBM_SETBKCOLOR, wParam, lParam);
      infoPtr->ColorBk = (COLORREF)lParam;
      InvalidateRect32 (hwnd, NULL, FALSE);
      UpdateWindow32 (hwnd);
      break;

    default: 
      if (message >= WM_USER) 
	ERR(progress, "unknown msg %04x wp=%04x lp=%08lx\n", 
		    message, wParam, lParam );
      return DefWindowProc32A( hwnd, message, wParam, lParam ); 
    } 

    return 0;
}


/***********************************************************************
 *           PROGRESS_Register [Internal]
 *
 * Registers the progress bar window class.
 * 
 */
void PROGRESS_Register(void)
{
    WNDCLASS32A wndClass;

    if( GlobalFindAtom32A( PROGRESS_CLASS32A ) ) return;

    ZeroMemory( &wndClass, sizeof( WNDCLASS32A ) );
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC32)ProgressWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(PROGRESS_INFO *);
    wndClass.hCursor       = LoadCursor32A( 0, IDC_ARROW32A );
    wndClass.lpszClassName = PROGRESS_CLASS32A;

    RegisterClass32A( &wndClass );
}

