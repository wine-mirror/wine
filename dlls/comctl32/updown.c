/*		
 * Updown control
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 * TODO:
 *   - subclass the buddy window (in UPDOWN_SetBuddy) to process the
 *     arrow keys
 *   - I am not sure about the default values for the Min, Max, Pos
 *     (in the UPDOWN_INFO the fields: MinVal, MaxVal, CurVal)
 *   - I think I do not handle correctly the WS_BORDER style.
 *     (Should be fixed. <ekohl@abo.rhein-zeitung.de>)
 *
 * Testing:
 *   Not much. The following  have not been tested at all:
 *     - horizontal arrows
 *     - listbox as buddy window
 *     - acceleration
 *     - base 16
 *     - UDS_ALIGNLEFT, ~UDS_WRAP
 *       (tested - they work)
 *     - integers with thousand separators.
 *       (fixed bugs. <noel@macadamian.com>)
 *
 *   Even though the above list seems rather large, the control seems to
 *   behave very well so I am confident it does work in most (all) of the
 *   untested cases.
 * Problems:
 *   I do not like the arrows yet, I'll work more on them later on.
 */

#include <stdlib.h>
#include <string.h>

#include "win.h"
#include "commctrl.h"
#include "winnls.h"
#include "updown.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(updown)

/* Control configuration constants */

#define INITIAL_DELAY    500 /* initial timer until auto-increment kicks in */
#define REPEAT_DELAY     50  /* delay between auto-increments */

#define DEFAULT_WIDTH    14  /* default width of the ctrl */
#define DEFAULT_XSEP      0  /* default separation between buddy and crtl */
#define DEFAULT_ADDTOP    0  /* amount to extend above the buddy window */
#define DEFAULT_ADDBOT    0  /* amount to extend below the buddy window */


/* Work constants */

#define FLAG_INCR        0x01
#define FLAG_DECR        0x02
#define FLAG_MOUSEIN     0x04
#define FLAG_CLICKED     (FLAG_INCR | FLAG_DECR)

#define TIMERID1         1
#define TIMERID2         2

static int accelIndex = -1;

#define UNKNOWN_PARAM(msg, wParam, lParam) WARN(updown, \
        "UpDown Ctrl: Unknown parameter(s) for message " #msg     \
	"(%04x): wp=%04x lp=%08lx\n", msg, wParam, lParam);

#define UPDOWN_GetInfoPtr(hwnd) ((UPDOWN_INFO *)GetWindowLongA (hwnd,0))


/***********************************************************************
 *           UPDOWN_InBounds
 * Tests if a given value 'val' is between the Min&Max limits
 */
static BOOL UPDOWN_InBounds(HWND hwnd, int val)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);

  if(infoPtr->MaxVal > infoPtr->MinVal)
    return (infoPtr->MinVal <= val) && (val <= infoPtr->MaxVal);
  else
    return (infoPtr->MaxVal <= val) && (val <= infoPtr->MinVal);
}

/***********************************************************************
 *           UPDOWN_OffsetVal
 * Tests if we can change the current value by delta. If so, it changes
 * it and returns TRUE. Else, it leaves it unchanged and returns FALSE.
 */
static BOOL UPDOWN_OffsetVal(HWND hwnd, int delta)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);

  /* check if we can do the modification first */
  if(!UPDOWN_InBounds (hwnd, infoPtr->CurVal+delta)){
    if (GetWindowLongA (hwnd, GWL_STYLE) & UDS_WRAP)
    {
      delta += (delta < 0 ? -1 : 1) *
	(infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1) *
	(infoPtr->MinVal - infoPtr->MaxVal) +
	(delta < 0 ? 1 : -1);
    }
    else
      return FALSE;
  }

  infoPtr->CurVal += delta;
  return TRUE;
}

/***********************************************************************
 *           UPDOWN_GetArrowRect
 * wndPtr   - pointer to the up-down wnd
 * rect     - will hold the rectangle
 * incr     - TRUE  get the "increment" rect (up or right)
 *            FALSE get the "decrement" rect (down or left)
 *          
 */
static void UPDOWN_GetArrowRect (HWND hwnd, RECT *rect, BOOL incr)
{
  int len; /* will hold the width or height */

  GetClientRect (hwnd, rect);

  if (GetWindowLongA (hwnd, GWL_STYLE) & UDS_HORZ) {
    len = rect->right - rect->left; /* compute the width */
    if (incr)
      rect->left = len/2+1; 
    else
      rect->right = len/2;
  }
  else {
    len = rect->bottom - rect->top; /* compute the height */
    if (incr)
      rect->bottom = len/2;
    else
      rect->top = len/2+1;
  }
}

/***********************************************************************
 *           UPDOWN_GetArrowFromPoint
 * Returns the rectagle (for the up or down arrow) that contains pt.
 * If it returns the up rect, it returns TRUE.
 * If it returns the down rect, it returns FALSE.
 */
static BOOL
UPDOWN_GetArrowFromPoint (HWND hwnd, RECT *rect, POINT pt)
{
    UPDOWN_GetArrowRect (hwnd, rect, TRUE);
  if(PtInRect(rect, pt))
    return TRUE;

    UPDOWN_GetArrowRect (hwnd, rect, FALSE);
  return FALSE;
}


/***********************************************************************
 *           UPDOWN_GetThousandSep
 * Returns the thousand sep. If an error occurs, it returns ','.
 */
static char UPDOWN_GetThousandSep()
{
  char sep[2];

  if(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, 
		      sep, sizeof(sep)) != 1)
    return ',';

  return sep[0];
}

/***********************************************************************
 *           UPDOWN_GetBuddyInt
 * Tries to read the pos from the buddy window and if it succeeds,
 * it stores it in the control's CurVal
 * returns:
 *   TRUE  - if it read the integer from the buddy successfully
 *   FALSE - if an error occured
 */
static BOOL UPDOWN_GetBuddyInt (HWND hwnd)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  char txt[20], sep, *src, *dst;
  int newVal;

  if (!IsWindow(infoPtr->Buddy))
    return FALSE;

  /*if the buddy is a list window, we must set curr index */
  if (!lstrcmpA (infoPtr->szBuddyClass, "ListBox")){
    newVal = SendMessageA(infoPtr->Buddy, LB_GETCARETINDEX, 0, 0);
    if(newVal < 0)
      return FALSE;
  }
  else{
    /* we have a regular window, so will get the text */
    if (!GetWindowTextA(infoPtr->Buddy, txt, sizeof(txt)))
      return FALSE;

    sep = UPDOWN_GetThousandSep(); 

    /* now get rid of the separators */
    for(src = dst = txt; *src; src++)
      if(*src != sep)
	*dst++ = *src;
    *dst = 0;

    /* try to convert the number and validate it */
    newVal = strtol(txt, &src, infoPtr->Base);
    if(*src || !UPDOWN_InBounds (hwnd, newVal)) 
      return FALSE;

    TRACE(updown, "new value(%d) read from buddy (old=%d)\n", 
		 newVal, infoPtr->CurVal);
  }
  
  infoPtr->CurVal = newVal;
  return TRUE;
}


/***********************************************************************
 *           UPDOWN_SetBuddyInt
 * Tries to set the pos to the buddy window based on current pos
 * returns:
 *   TRUE  - if it set the caption of the  buddy successfully
 *   FALSE - if an error occured
 */
static BOOL UPDOWN_SetBuddyInt (HWND hwnd)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  char txt1[20], sep;
  int len;

  if (!IsWindow(infoPtr->Buddy)) 
    return FALSE;

  TRACE(updown, "set new value(%d) to buddy.\n",
	       infoPtr->CurVal);

  /*if the buddy is a list window, we must set curr index */
  if(!lstrcmpA (infoPtr->szBuddyClass, "ListBox")){
    SendMessageA(infoPtr->Buddy, LB_SETCURSEL, infoPtr->CurVal, 0);
  }
  else{ /* Regular window, so set caption to the number */
    len = sprintf(txt1, (infoPtr->Base==16) ? "%X" : "%d", infoPtr->CurVal);

    sep = UPDOWN_GetThousandSep(); 

    /* Do thousands seperation if necessary */
    if (!(GetWindowLongA (hwnd, GWL_STYLE) & UDS_NOTHOUSANDS) && (len > 3)) {
      char txt2[20], *src = txt1, *dst = txt2;
      if(len%3 > 0){
	lstrcpynA (dst, src, len%3 + 1);      /* need to include the null */ 
	dst += len%3;
	src += len%3;
      }
      for(len=0; *src; len++){
	if(len%3==0)
	  *dst++ = sep;
	*dst++ = *src++;
      }
      *dst = 0;           /* null terminate it */
      strcpy(txt1, txt2); /* move it to the proper place */
    }
    SetWindowTextA(infoPtr->Buddy, txt1);
  }

  return TRUE;
} 

/***********************************************************************
 * UPDOWN_Draw [Internal]
 *
 * Draw the arrows. The background need not be erased.
 */
static void UPDOWN_Draw (HWND hwnd, HDC hdc)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  BOOL prssed;
  RECT rect;
  
  /* Draw the incr button */
  UPDOWN_GetArrowRect (hwnd, &rect, TRUE);
  prssed = (infoPtr->Flags & FLAG_INCR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl(hdc, &rect, DFC_SCROLL, 
	(dwStyle & UDS_HORZ ? DFCS_SCROLLLEFT : DFCS_SCROLLUP) |
	(prssed ? DFCS_PUSHED : 0) |
	(dwStyle&WS_DISABLED ? DFCS_INACTIVE : 0) );

  /* Draw the space between the buttons */
  rect.top = rect.bottom; rect.bottom++;
  DrawEdge(hdc, &rect, 0, BF_MIDDLE);
		    
  /* Draw the decr button */
  UPDOWN_GetArrowRect(hwnd, &rect, FALSE);
  prssed = (infoPtr->Flags & FLAG_DECR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl(hdc, &rect, DFC_SCROLL, 
	(dwStyle & UDS_HORZ ? DFCS_SCROLLRIGHT : DFCS_SCROLLDOWN) |
	(prssed ? DFCS_PUSHED : 0) |
	(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
}

/***********************************************************************
 * UPDOWN_Refresh [Internal]
 *
 * Synchronous drawing (must NOT be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Refresh (HWND hwnd)
{
    HDC hdc;
  
    hdc = GetDC (hwnd);
    UPDOWN_Draw (hwnd, hdc);
    ReleaseDC (hwnd, hdc);
}


/***********************************************************************
 * UPDOWN_Paint [Internal]
 *
 * Asynchronous drawing (must ONLY be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Paint (HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
  
    hdc = BeginPaint (hwnd, &ps);
    UPDOWN_Draw (hwnd, hdc);
    EndPaint (hwnd, &ps);
}

/***********************************************************************
 *           UPDOWN_SetBuddy
 * Tests if 'hwndBud' is a valid window handle. If not, returns FALSE.
 * Else, sets it as a new Buddy.
 * Then, it should subclass the buddy 
 * If window has the UDS_ARROWKEYS, it subcalsses the buddy window to
 * process the UP/DOWN arrow keys.
 * If window has the UDS_ALIGNLEFT or UDS_ALIGNRIGHT style
 * the size/pos of the buddy and the control are adjusted accordingly.
 */
static BOOL UPDOWN_SetBuddy (HWND hwnd, HWND hwndBud)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd); 
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  RECT budRect; /* new coord for the buddy */
  int x;          /* new x position and width for the up-down */
 	  
  *infoPtr->szBuddyClass = '\0';

  /* Is is a valid bud? */
  if(!IsWindow(hwndBud))
    return FALSE;

  /* Store buddy window clas name */
  GetClassNameA (hwndBud, infoPtr->szBuddyClass, 40);

  if(dwStyle & UDS_ARROWKEYS){
    FIXME(updown, "we need to subclass the buddy to process the arrow keys.\n");
  }

  /* do we need to do any adjustments? */
  if(!(dwStyle & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT)))
    return TRUE;

  /* Get the rect of the buddy relative to its parent */
  GetWindowRect(infoPtr->Buddy, &budRect);
  MapWindowPoints(HWND_DESKTOP, GetParent(infoPtr->Buddy),
		  (POINT *)(&budRect.left), 2);

  /* now do the positioning */
  if(dwStyle & UDS_ALIGNRIGHT){
    budRect.right -= DEFAULT_WIDTH+DEFAULT_XSEP;
    x  = budRect.right+DEFAULT_XSEP;
  }
  else{ /* UDS_ALIGNLEFT */
    x  = budRect.left;
    budRect.left += DEFAULT_WIDTH+DEFAULT_XSEP;
  }

  /* first adjust the buddy to accomodate the up/down */
  SetWindowPos(infoPtr->Buddy, 0, budRect.left, budRect.top,
	       budRect.right  - budRect.left, budRect.bottom - budRect.top, 
	       SWP_NOACTIVATE|SWP_NOZORDER);

  /* now position the up/down */
  /* Since the UDS_ALIGN* flags were used, */
  /* we will pick the position and size of the window. */

  SetWindowPos (hwnd, 0, x, budRect.top-DEFAULT_ADDTOP,DEFAULT_WIDTH,
		 (budRect.bottom-budRect.top)+DEFAULT_ADDTOP+DEFAULT_ADDBOT,
		 SWP_NOACTIVATE|SWP_NOZORDER);

  return TRUE;
}	  

/***********************************************************************
 *           UPDOWN_DoAction
 *
 * This function increments/decrements the CurVal by the 
 * 'delta' amount according to the 'incr' flag
 * It notifies the parent as required.
 * It handles wraping and non-wraping correctly.
 * It is assumed that delta>0
 */
static void UPDOWN_DoAction (HWND hwnd, int delta, BOOL incr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd); 
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  int old_val = infoPtr->CurVal;
  NM_UPDOWN ni;

  TRACE(updown, "%s by %d\n", incr ? "inc" : "dec", delta);

  /* check if we can do the modification first */
  delta *= (incr ? 1 : -1) * (infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1);
  if(!UPDOWN_OffsetVal (hwnd, delta))
    return;

  /* so, if we can do the change, recompute delta and restore old value */
  delta = infoPtr->CurVal - old_val;
  infoPtr->CurVal = old_val;

  /* We must notify parent now to obtain permission */
  ni.iPos = infoPtr->CurVal;
  ni.iDelta = delta;
  ni.hdr.hwndFrom = hwnd;
  ni.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
  ni.hdr.code = UDN_DELTAPOS; 
  if (SendMessageA(GetParent (hwnd), WM_NOTIFY,
		   (WPARAM)ni.hdr.idFrom, (LPARAM)&ni))
    return; /* we are not allowed to change */
  
  /* Now adjust value with (maybe new) delta */
  if (!UPDOWN_OffsetVal (hwnd, ni.iDelta))
    return;

  /* Now take care about our buddy */
  if(!IsWindow(infoPtr->Buddy)) 
    return; /* Nothing else to do */


  if (dwStyle & UDS_SETBUDDYINT)
    UPDOWN_SetBuddyInt (hwnd);

  /* Also, notify it */
  /* FIXME: do we need to send the notification only if
            we do not have the UDS_SETBUDDYINT style set? */

  SendMessageA (GetParent (hwnd), 
		dwStyle & UDS_HORZ ? WM_HSCROLL : WM_VSCROLL, 
		 MAKELONG(incr ? SB_LINEUP : SB_LINEDOWN, infoPtr->CurVal),
		hwnd);
}

/***********************************************************************
 *           UPDOWN_IsEnabled
 *
 * Returns TRUE if it is enabled as well as its buddy (if any)
 *         FALSE otherwise
 */
static BOOL UPDOWN_IsEnabled (HWND hwnd)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);

  if(GetWindowLongA (hwnd, GWL_STYLE) & WS_DISABLED)
    return FALSE;
  return IsWindowEnabled(infoPtr->Buddy);
}

/***********************************************************************
 *           UPDOWN_CancelMode
 *
 * Deletes any timers, releases the mouse and does  redraw if necessary.
 * If the control is not in "capture" mode, it does nothing.
 * If the control was not in cancel mode, it returns FALSE. 
 * If the control was in cancel mode, it returns TRUE.
 */
static BOOL UPDOWN_CancelMode (HWND hwnd)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
 
  /* if not in 'capture' mode, do nothing */
  if(!(infoPtr->Flags & FLAG_CLICKED))
    return FALSE;

  KillTimer (hwnd, TIMERID1); /* kill all possible timers */
  KillTimer (hwnd, TIMERID2);
  
  if (GetCapture() == hwnd)    /* let the mouse go         */
    ReleaseCapture();          /* if we still have it      */  
  
  infoPtr->Flags = 0;          /* get rid of any flags     */
  UPDOWN_Refresh (hwnd);       /* redraw the control just in case */
  
  return TRUE;
}

/***********************************************************************
 *           UPDOWN_HandleMouseEvent
 *
 * Handle a mouse event for the updown.
 * 'pt' is the location of the mouse event in client or
 * windows coordinates. 
 */
static void UPDOWN_HandleMouseEvent (HWND hwnd, UINT msg, POINT pt)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  RECT rect;
  int temp;

  switch(msg)
    {
    case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
      /* If we are already in the 'clicked' mode, then nothing to do */
      if(infoPtr->Flags & FLAG_CLICKED)
	return;

      /* If the buddy is an edit, will set focus to it */
      if (!lstrcmpA (infoPtr->szBuddyClass, "Edit"))
	SetFocus(infoPtr->Buddy);

      /* Now see which one is the 'active' arrow */
      temp = UPDOWN_GetArrowFromPoint (hwnd, &rect, pt);

      /* Update the CurVal if necessary */
      if (dwStyle & UDS_SETBUDDYINT)
	UPDOWN_GetBuddyInt (hwnd);
	
      /* Before we proceed, see if we can spin... */
      if(!(dwStyle & UDS_WRAP))
	if(( temp && infoPtr->CurVal==infoPtr->MaxVal) ||
	   (!temp && infoPtr->CurVal==infoPtr->MinVal))
	  return;

      /* Set up the correct flags */
      infoPtr->Flags  = 0; 
      infoPtr->Flags |= temp ? FLAG_INCR : FLAG_DECR;
      infoPtr->Flags |= FLAG_MOUSEIN;
      
      /* repaint the control */
      UPDOWN_Refresh (hwnd);

      /* process the click */
      UPDOWN_DoAction (hwnd, 1, infoPtr->Flags & FLAG_INCR);

      /* now capture all mouse messages */
      SetCapture (hwnd);

      /* and startup the first timer */
      SetTimer(hwnd, TIMERID1, INITIAL_DELAY, 0); 
      break;

    case WM_MOUSEMOVE:
      /* If we are not in the 'clicked' mode, then nothing to do */
      if(!(infoPtr->Flags & FLAG_CLICKED))
	return;

      /* save the flags to see if any got modified */
      temp = infoPtr->Flags;

      /* Now get the 'active' arrow rectangle */
      if (infoPtr->Flags & FLAG_INCR)
	UPDOWN_GetArrowRect (hwnd, &rect, TRUE);
      else
	UPDOWN_GetArrowRect (hwnd, &rect, FALSE);

      /* Update the flags if we are in/out */
      if(PtInRect(&rect, pt))
	infoPtr->Flags |=  FLAG_MOUSEIN;
      else{
	infoPtr->Flags &= ~FLAG_MOUSEIN;
	if(accelIndex != -1) /* if we have accel info */
	  accelIndex = 0;    /* reset it              */
      }
      /* If state changed, redraw the control */
      if(temp != infoPtr->Flags)
	UPDOWN_Refresh (hwnd);
      break;

      default:
	ERR(updown, "Impossible case!\n");
    }

}

/***********************************************************************
 *           UpDownWndProc
 */
LRESULT WINAPI UpDownWindowProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  int temp;

  switch(message)
    {
    case WM_NCCREATE:
      /* get rid of border, if any */
      SetWindowLongA (hwnd, GWL_STYLE, dwStyle & ~WS_BORDER);
      return TRUE;

    case WM_CREATE:
      infoPtr = (UPDOWN_INFO*)COMCTL32_Alloc (sizeof(UPDOWN_INFO));
      SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

      /* initialize the info struct */
      infoPtr->AccelCount=0; infoPtr->AccelVect=0; 
      infoPtr->CurVal=0; infoPtr->MinVal=0; infoPtr->MaxVal=100; /*FIXME*/
      infoPtr->Base  = 10; /* Default to base 10  */
      infoPtr->Buddy = 0;  /* No buddy window yet */
      infoPtr->Flags = 0;  /* And no flags        */

      /* Do we pick the buddy win ourselves? */
      if (dwStyle & UDS_AUTOBUDDY)
	UPDOWN_SetBuddy (hwnd, GetWindow (hwnd, GW_HWNDPREV));
	
      TRACE(updown, "UpDown Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      if(infoPtr->AccelVect)
	COMCTL32_Free (infoPtr->AccelVect);

      COMCTL32_Free (infoPtr);

      TRACE(updown, "UpDown Ctrl destruction, hwnd=%04x\n", hwnd);
      break;
	
    case WM_ENABLE:
      if (dwStyle & WS_DISABLED)
	UPDOWN_CancelMode (hwnd);
      UPDOWN_Paint (hwnd);
      break;

    case WM_TIMER:
      /* if initial timer, kill it and start the repeat timer */
      if(wParam == TIMERID1){
	KillTimer(hwnd, TIMERID1);
	/* if no accel info given, used default timer */
	if(infoPtr->AccelCount==0 || infoPtr->AccelVect==0){
	  accelIndex = -1;
	  temp = REPEAT_DELAY;
	}
	else{
	  accelIndex = 0; /* otherwise, use it */
	  temp = infoPtr->AccelVect[accelIndex].nSec * 1000 + 1;
	}
	SetTimer(hwnd, TIMERID2, temp, 0);
      }

      /* now, if the mouse is above us, do the thing...*/
      if(infoPtr->Flags & FLAG_MOUSEIN){
	temp = accelIndex==-1 ? 1 : infoPtr->AccelVect[accelIndex].nInc;
	UPDOWN_DoAction(hwnd, temp, infoPtr->Flags & FLAG_INCR);
	
	if(accelIndex!=-1 && accelIndex < infoPtr->AccelCount-1){
	  KillTimer(hwnd, TIMERID2);
	  accelIndex++; /* move to the next accel info */
	  temp = infoPtr->AccelVect[accelIndex].nSec * 1000 + 1;
	  /* make sure we have at least 1ms intervals */
	  SetTimer(hwnd, TIMERID2, temp, 0);	    
	}
      }
      break;

    case WM_CANCELMODE:
      UPDOWN_CancelMode (hwnd);
      break;

    case WM_LBUTTONUP:
      if(!UPDOWN_CancelMode(hwnd))
	break;
      /*If we released the mouse and our buddy is an edit */
      /* we must select all text in it.                   */
      {
          WND *tmpWnd = WIN_FindWndPtr(infoPtr->Buddy);
          if(WIDGETS_IsControl(tmpWnd, BIC32_EDIT))
              SendMessageA(infoPtr->Buddy, EM_SETSEL, 0, MAKELONG(0, -1));
          WIN_ReleaseWndPtr(tmpWnd);
      }
      break;
      
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
      if(UPDOWN_IsEnabled(hwnd)){
	POINT pt;
	CONV_POINT16TO32( (POINT16 *)&lParam, &pt );
	UPDOWN_HandleMouseEvent (hwnd, message, pt );
      }
    break;

    case WM_KEYDOWN:
      if((dwStyle & UDS_ARROWKEYS) && UPDOWN_IsEnabled(hwnd)){
	switch(wParam){
	case VK_UP:  
	case VK_DOWN:
	  UPDOWN_GetBuddyInt (hwnd);
	  UPDOWN_DoAction (hwnd, 1, wParam==VK_UP);
	  break;
	}
      }
      break;
      
    case WM_PAINT:
      UPDOWN_Paint (hwnd);
      break;
    
    case UDM_GETACCEL:
      if (wParam==0 && lParam==0)    /*if both zero, */
	return infoPtr->AccelCount;  /*just return the accel count*/
      if (wParam || lParam){
	UNKNOWN_PARAM(UDM_GETACCEL, wParam, lParam);
	return 0;
      }
      temp = MIN(infoPtr->AccelCount, wParam);
      memcpy((void *)lParam, infoPtr->AccelVect, temp*sizeof(UDACCEL));
      return temp;

    case UDM_SETACCEL:
      TRACE(updown, "UpDown Ctrl new accel info, hwnd=%04x\n", hwnd);
      if(infoPtr->AccelVect){
	COMCTL32_Free (infoPtr->AccelVect);
	infoPtr->AccelCount = 0;
	infoPtr->AccelVect  = 0;
      }
      if(wParam==0)
	return TRUE;
      infoPtr->AccelVect = COMCTL32_Alloc (wParam*sizeof(UDACCEL));
      if(infoPtr->AccelVect==0)
	return FALSE;
      memcpy(infoPtr->AccelVect, (void*)lParam, wParam*sizeof(UDACCEL));
      return TRUE;

    case UDM_GETBASE:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETBASE, wParam, lParam);
      return infoPtr->Base;

    case UDM_SETBASE:
      TRACE(updown, "UpDown Ctrl new base(%d), hwnd=%04x\n", 
		     wParam, hwnd);
      if ( !(wParam==10 || wParam==16) || lParam)
	UNKNOWN_PARAM(UDM_SETBASE, wParam, lParam);
      if (wParam==10 || wParam==16){
	temp = infoPtr->Base;
	infoPtr->Base = wParam;
	return temp;       /* return the prev base */
      }
      break;

    case UDM_GETBUDDY:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETBUDDY, wParam, lParam);
      return infoPtr->Buddy;

    case UDM_SETBUDDY:
      if (lParam)
	UNKNOWN_PARAM(UDM_SETBUDDY, wParam, lParam);
      temp = infoPtr->Buddy;
      infoPtr->Buddy = wParam;
      UPDOWN_SetBuddy (hwnd, wParam);
      TRACE(updown, "UpDown Ctrl new buddy(%04x), hwnd=%04x\n", 
		     infoPtr->Buddy, hwnd);
      return temp;

    case UDM_GETPOS:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = UPDOWN_GetBuddyInt (hwnd);
      return MAKELONG(infoPtr->CurVal, temp ? 0 : 1);

    case UDM_SETPOS:
      if (wParam || HIWORD(lParam))
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = SLOWORD(lParam);
      TRACE(updown, "UpDown Ctrl new value(%d), hwnd=%04x\n",
		     temp, hwnd);
      if(!UPDOWN_InBounds(hwnd, temp)){
	if(temp < infoPtr->MinVal)  
	  temp = infoPtr->MinVal;
	if(temp > infoPtr->MaxVal)
	  temp = infoPtr->MaxVal;
      }
      wParam = infoPtr->CurVal; /* save prev value   */
      infoPtr->CurVal = temp;   /* set the new value */
      if(dwStyle & UDS_SETBUDDYINT)
	UPDOWN_SetBuddyInt (hwnd);
      return wParam;            /* return prev value */
      
    case UDM_GETRANGE:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETRANGE, wParam, lParam);
      return MAKELONG(infoPtr->MaxVal, infoPtr->MinVal);

    case UDM_SETRANGE:
      if (wParam)
	UNKNOWN_PARAM(UDM_SETRANGE, wParam, lParam); /* we must have:     */
      infoPtr->MaxVal = SLOWORD(lParam); /* UD_MINVAL <= Max <= UD_MAXVAL */
      infoPtr->MinVal = SHIWORD(lParam); /* UD_MINVAL <= Min <= UD_MAXVAL */
                                         /* |Max-Min| <= UD_MAXVAL        */
      TRACE(updown, "UpDown Ctrl new range(%d to %d), hwnd=%04x\n", 
		     infoPtr->MinVal, infoPtr->MaxVal, hwnd);
      break;                             

    case UDM_GETRANGE32:
      if (wParam)
	*(LPINT)wParam = infoPtr->MinVal;
      if (lParam)
	*(LPINT)lParam = infoPtr->MaxVal;
      break;

    case UDM_SETRANGE32:
      infoPtr->MinVal = (INT)wParam;
      infoPtr->MaxVal = (INT)lParam;
      if (infoPtr->MaxVal <= infoPtr->MinVal)
	infoPtr->MaxVal = infoPtr->MinVal + 1;
      TRACE(updown, "UpDown Ctrl new range(%d to %d), hwnd=%04x\n", 
		     infoPtr->MinVal, infoPtr->MaxVal, hwnd);
      break;

    default: 
      if (message >= WM_USER) 
	ERR (updown, "unknown msg %04x wp=%04x lp=%08lx\n", 
	     message, wParam, lParam);
      return DefWindowProcA (hwnd, message, wParam, lParam); 
    } 

    return 0;
}


/***********************************************************************
 *		UPDOWN_Register	[Internal]
 *
 * Registers the updown window class.
 */

VOID
UPDOWN_Register(void)
{
    WNDCLASSA wndClass;

    if( GlobalFindAtomA( UPDOWN_CLASSA ) ) return;

    ZeroMemory( &wndClass, sizeof( WNDCLASSA ) );
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)UpDownWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(UPDOWN_INFO*);
    wndClass.hCursor       = LoadCursorA( 0, IDC_ARROWA );
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = UPDOWN_CLASSA;
 
    RegisterClassA( &wndClass );
}


/***********************************************************************
 *		UPDOWN_Unregister	[Internal]
 *
 * Unregisters the updown window class.
 */

VOID
UPDOWN_Unregister (VOID)
{
    if (GlobalFindAtomA (UPDOWN_CLASSA))
	UnregisterClassA (UPDOWN_CLASSA, (HINSTANCE)NULL);
}

