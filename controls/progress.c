/*		
 * Progress control
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 * TODO:
 *   - I do not know what to to on WM_[SG]ET_FONT
 * Problems:
 *   - I think I do not compute correctly the numer of leds to be drawn
 */

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"
#include "sysmetrics.h"
#include "progress.h"
#include "graphics.h"
#include "heap.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"

/* Control configuration constants */

#define LED_WIDTH  8
#define LED_GAP    2

/* Work constants */

#define UNKNOWN_PARAM(msg, wParam, lParam) dprintf_progress(stddeb, \
        "Progress Ctrl: Unknown parameter(s) for message " #msg     \
	"(%04x): wp=%04x lp=%08lx\n", msg, wParam, lParam); 

#define PROGRESS_GetInfoPtr(wndPtr) ((PROGRESS_INFO *)wndPtr->wExtra)


/***********************************************************************
 *           PROGRESS_Paint
 * Draw the arrows. The background need not be erased.
 * If dc!=0, it draws on it
 */
static void PROGRESS_Paint(WND *wndPtr, HDC32 dc)
{
  PROGRESS_INFO *infoPtr = PROGRESS_GetInfoPtr(wndPtr);
  HBRUSH32 ledBrush;
  int rightBar, rightMost;
  PAINTSTRUCT32 ps;
  RECT32 rect;
  HDC32 hdc;

  dprintf_progress(stddeb, "Progress Bar: paint pos=%d min=%d, max=%d\n",
		   infoPtr->CurVal, infoPtr->MinVal, infoPtr->MaxVal);

  /* get a dc */
  hdc = dc==0 ? BeginPaint32(wndPtr->hwndSelf, &ps) : dc;

  /* get the required brush */
  ledBrush = GetSysColorBrush32(COLOR_HIGHLIGHT);

  /* get rect for the bar, adjusted for the border */
  GetClientRect32(wndPtr->hwndSelf, &rect);

  /* draw the border */
  DrawEdge32(hdc, &rect, BDR_SUNKENOUTER, BF_RECT|BF_ADJUST|BF_MIDDLE);
  rect.left++; rect.right--; rect.top++; rect.bottom--;
  rightMost = rect.right;

  /* compute extent of progress bar */
  rightBar = rect.left + 
    MulDiv32(infoPtr->CurVal-infoPtr->MinVal,
	     rect.right - rect.left,
	     infoPtr->MaxVal-infoPtr->MinVal);

  /* now draw the bar */
  while(rect.left < rightBar) { 
    rect.right = rect.left+LED_WIDTH;
    FillRect32(hdc, &rect, ledBrush);
    rect.left  = rect.right+LED_GAP;
  }

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
      /* initialize the info struct */
      infoPtr->MinVal=0; 
      infoPtr->MaxVal=100;
      infoPtr->CurVal=0; 
      infoPtr->Step=10;
      dprintf_updown(stddeb, "Progress Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      dprintf_updown(stddeb, "Progress Ctrl destruction, hwnd=%04x\n", hwnd);
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
	PROGRESS_Paint(wndPtr, 0);
      }
      return temp;

    case PBM_SETPOS:
      if (lParam)
	UNKNOWN_PARAM(PBM_SETPOS, wParam, lParam);
      temp = infoPtr->CurVal;
      if(temp != wParam){
	infoPtr->CurVal = (UINT16)wParam;
	PROGRESS_CoercePos(wndPtr);
	PROGRESS_Paint(wndPtr, 0);
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
	PROGRESS_Paint(wndPtr, 0);
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
	PROGRESS_Paint(wndPtr, 0);
      return temp;
    
    default: 
      if (message >= WM_USER) 
	fprintf( stderr, "Progress Ctrl: unknown msg %04x wp=%04x lp=%08lx\n", 
		 message, wParam, lParam );
      return DefWindowProc32A( hwnd, message, wParam, lParam ); 
    } 

    return 0;
}



