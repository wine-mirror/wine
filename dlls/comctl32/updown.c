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
#include "commctrl.h"
#include "winnls.h"
#include "sysmetrics.h"
#include "updown.h"
#include "win.h"
#include "debug.h"

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

#define UPDOWN_GetInfoPtr(wndPtr) ((UPDOWN_INFO *)wndPtr->wExtra[0])


/***********************************************************************
 *           UPDOWN_InBounds
 * Tests if a given value 'val' is between the Min&Max limits
 */
static BOOL32 UPDOWN_InBounds(WND *wndPtr, int val)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);

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
static BOOL32 UPDOWN_OffsetVal(WND *wndPtr, int delta)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);

  /* check if we can do the modification first */
  if(!UPDOWN_InBounds(wndPtr, infoPtr->CurVal+delta)){
    if(wndPtr->dwStyle & UDS_WRAP)
      delta += (delta < 0 ? -1 : 1) *
	(infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1) *
	(infoPtr->MinVal - infoPtr->MaxVal) +
	(delta < 0 ? 1 : -1);
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
static void UPDOWN_GetArrowRect(WND *wndPtr, RECT32 *rect, BOOL32 incr)
{
  int len; /* will hold the width or height */

  GetClientRect32(wndPtr->hwndSelf, rect);

  if (wndPtr->dwStyle & UDS_HORZ) {
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
static int UPDOWN_GetArrowFromPoint(WND *wndPtr, RECT32 *rect, POINT32 pt)
{
  UPDOWN_GetArrowRect(wndPtr, rect, TRUE);
  if(PtInRect32(rect, pt))
    return TRUE;

  UPDOWN_GetArrowRect(wndPtr, rect, FALSE);
  return FALSE;
}


/***********************************************************************
 *           UPDOWN_GetThousandSep
 * Returns the thousand sep. If an error occurs, it returns ','.
 */
static char UPDOWN_GetThousandSep()
{
  char sep[2];

  if(GetLocaleInfo32A(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, 
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
static BOOL32 UPDOWN_GetBuddyInt(WND *wndPtr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);
  char txt[20], sep, *src, *dst;
  int newVal;

  if (!IsWindow32(infoPtr->Buddy))
    return FALSE;

  /*if the buddy is a list window, we must set curr index */
  if(WIDGETS_IsControl32(WIN_FindWndPtr(infoPtr->Buddy), BIC32_LISTBOX)){
    newVal = SendMessage32A(infoPtr->Buddy, LB_GETCARETINDEX32, 0, 0);
    if(newVal < 0)
      return FALSE;
  }
  else{
    /* we have a regular window, so will get the text */
    if (!GetWindowText32A(infoPtr->Buddy, txt, sizeof(txt)))
      return FALSE;

    sep = UPDOWN_GetThousandSep(); 

    /* now get rid of the separators */
    for(src = dst = txt; *src; src++)
      if(*src != sep)
	*dst++ = *src;
    *dst = 0;

    /* try to convert the number and validate it */
    newVal = strtol(txt, &src, infoPtr->Base);
    if(*src || !UPDOWN_InBounds(wndPtr, newVal)) 
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
static BOOL32 UPDOWN_SetBuddyInt(WND *wndPtr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);
  char txt1[20], sep;
  int len;

  if (!IsWindow32(infoPtr->Buddy)) 
    return FALSE;

  TRACE(updown, "set new value(%d) to buddy.\n",
	       infoPtr->CurVal);

  /*if the buddy is a list window, we must set curr index */
  if(WIDGETS_IsControl32(WIN_FindWndPtr(infoPtr->Buddy), BIC32_LISTBOX)){
    SendMessage32A(infoPtr->Buddy, LB_SETCURSEL32, infoPtr->CurVal, 0);
  }
  else{ /* Regular window, so set caption to the number */
    len = sprintf(txt1, (infoPtr->Base==16) ? "%X" : "%d", infoPtr->CurVal);

    sep = UPDOWN_GetThousandSep(); 

    /* Do thousands seperation if necessary */
    if (!(wndPtr->dwStyle & UDS_NOTHOUSANDS) && (len > 3)) {
      char txt2[20], *src = txt1, *dst = txt2;
      if(len%3 > 0){
	lstrcpyn32A (dst, src, len%3 + 1);      /* need to include the null */ 
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
    SetWindowText32A(infoPtr->Buddy, txt1);
  }

  return TRUE;
} 

/***********************************************************************
 * UPDOWN_Draw [Internal]
 *
 * Draw the arrows. The background need not be erased.
 */
static void UPDOWN_Draw (WND *wndPtr, HDC32 hdc)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);
  BOOL32 prssed;
  RECT32 rect;
  
  /* Draw the incr button */
  UPDOWN_GetArrowRect(wndPtr, &rect, TRUE);
  prssed = (infoPtr->Flags & FLAG_INCR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl32(hdc, &rect, DFC_SCROLL, 
	(wndPtr->dwStyle & UDS_HORZ ? DFCS_SCROLLLEFT : DFCS_SCROLLUP) |
	(prssed ? DFCS_PUSHED : 0) |
	(wndPtr->dwStyle&WS_DISABLED ? DFCS_INACTIVE : 0) );

  /* Draw the space between the buttons */
  rect.top = rect.bottom; rect.bottom++;
  DrawEdge32(hdc, &rect, 0, BF_MIDDLE);
		    
  /* Draw the decr button */
  UPDOWN_GetArrowRect(wndPtr, &rect, FALSE);
  prssed = (infoPtr->Flags & FLAG_DECR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl32(hdc, &rect, DFC_SCROLL, 
	(wndPtr->dwStyle & UDS_HORZ ? DFCS_SCROLLRIGHT : DFCS_SCROLLDOWN) |
	(prssed ? DFCS_PUSHED : 0) |
	(wndPtr->dwStyle&WS_DISABLED ? DFCS_INACTIVE : 0) );
}

/***********************************************************************
 * UPDOWN_Refresh [Internal]
 *
 * Synchronous drawing (must NOT be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Refresh (WND *wndPtr)
{
    HDC32 hdc;
  
    hdc = GetDC32 (wndPtr->hwndSelf);
    UPDOWN_Draw (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
}


/***********************************************************************
 * UPDOWN_Paint [Internal]
 *
 * Asynchronous drawing (must ONLY be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Paint (WND *wndPtr)
{
    PAINTSTRUCT32 ps;
    HDC32 hdc;
  
    hdc = BeginPaint32 (wndPtr->hwndSelf, &ps);
    UPDOWN_Draw (wndPtr, hdc);
    EndPaint32 (wndPtr->hwndSelf, &ps);
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
static BOOL32 UPDOWN_SetBuddy(WND *wndPtr, HWND32 hwndBud)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr); 
  RECT32 budRect; /* new coord for the buddy */
  int x;          /* new x position and width for the up-down */
 	  
  /* Is is a valid bud? */
  if(!IsWindow32(hwndBud))
    return FALSE;

  if(wndPtr->dwStyle & UDS_ARROWKEYS){
    FIXME(updown, "we need to subclass the buddy to process the arrow keys.\n");
  }

  /* do we need to do any adjustments? */
  if(!(wndPtr->dwStyle & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT)))
    return TRUE;

  /* Get the rect of the buddy relative to its parent */
  GetWindowRect32(infoPtr->Buddy, &budRect);
  MapWindowPoints32(HWND_DESKTOP, GetParent32(infoPtr->Buddy),
		  (POINT32 *)(&budRect.left), 2);

  /* now do the positioning */
  if(wndPtr->dwStyle & UDS_ALIGNRIGHT){
    budRect.right -= DEFAULT_WIDTH+DEFAULT_XSEP;
    x  = budRect.right+DEFAULT_XSEP;
  }
  else{ /* UDS_ALIGNLEFT */
    x  = budRect.left;
    budRect.left += DEFAULT_WIDTH+DEFAULT_XSEP;
  }

  /* first adjust the buddy to accomodate the up/down */
  SetWindowPos32(infoPtr->Buddy, 0, budRect.left, budRect.top,
	       budRect.right  - budRect.left, budRect.bottom - budRect.top, 
	       SWP_NOACTIVATE|SWP_NOZORDER);

  /* now position the up/down */
  /* Since the UDS_ALIGN* flags were used, */
  /* we will pick the position and size of the window. */

  SetWindowPos32(wndPtr->hwndSelf,0,x,budRect.top-DEFAULT_ADDTOP,DEFAULT_WIDTH,
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
static void UPDOWN_DoAction(WND *wndPtr, int delta, BOOL32 incr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr); 
  int old_val = infoPtr->CurVal;
  NM_UPDOWN ni;

  TRACE(updown, "%s by %d\n", incr ? "inc" : "dec", delta);

  /* check if we can do the modification first */
  delta *= (incr ? 1 : -1) * (infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1);
  if(!UPDOWN_OffsetVal(wndPtr, delta))
    return;

  /* so, if we can do the change, recompute delta and restore old value */
  delta = infoPtr->CurVal - old_val;
  infoPtr->CurVal = old_val;

  /* We must notify parent now to obtain permission */
  ni.iPos = infoPtr->CurVal;
  ni.iDelta = delta;
  ni.hdr.hwndFrom = wndPtr->hwndSelf;
  ni.hdr.idFrom = wndPtr->wIDmenu;
  ni.hdr.code = UDN_DELTAPOS; 
  if(SendMessage32A(wndPtr->parent->hwndSelf, 
		    WM_NOTIFY, wndPtr->wIDmenu, (LPARAM)&ni))
    return; /* we are not allowed to change */
  
  /* Now adjust value with (maybe new) delta */
  if(!UPDOWN_OffsetVal(wndPtr, ni.iDelta))
    return;

  /* Now take care about our buddy */
  if(!IsWindow32(infoPtr->Buddy)) 
    return; /* Nothing else to do */


  if(wndPtr->dwStyle & UDS_SETBUDDYINT)
    UPDOWN_SetBuddyInt(wndPtr);

  /* Also, notify it */
  /* FIXME: do we need to send the notification only if
            we do not have the UDS_SETBUDDYINT style set? */

  SendMessage32A(GetParent32 (wndPtr->hwndSelf), 
		 wndPtr->dwStyle & UDS_HORZ ? WM_HSCROLL : WM_VSCROLL, 
		 MAKELONG(incr ? SB_LINEUP : SB_LINEDOWN, infoPtr->CurVal),
		 wndPtr->hwndSelf);
}

/***********************************************************************
 *           UPDOWN_IsEnabled
 *
 * Returns TRUE if it is enabled as well as its buddy (if any)
 *         FALSE otherwise
 */
static BOOL32 UPDOWN_IsEnabled(WND *wndPtr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);

  if(wndPtr->dwStyle & WS_DISABLED)
    return FALSE;
  return IsWindowEnabled32(infoPtr->Buddy);
}

/***********************************************************************
 *           UPDOWN_CancelMode
 *
 * Deletes any timers, releases the mouse and does  redraw if necessary.
 * If the control is not in "capture" mode, it does nothing.
 * If the control was not in cancel mode, it returns FALSE. 
 * If the control was in cancel mode, it returns TRUE.
 */
static BOOL32 UPDOWN_CancelMode(WND *wndPtr)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);
 
  /* if not in 'capture' mode, do nothing */
  if(!(infoPtr->Flags & FLAG_CLICKED))
    return FALSE;

  KillTimer32(wndPtr->hwndSelf, TIMERID1); /* kill all possible timers */
  KillTimer32(wndPtr->hwndSelf, TIMERID2);
  
  if(GetCapture32() == wndPtr->hwndSelf)   /* let the mouse go         */
    ReleaseCapture();          /* if we still have it      */  
  
  infoPtr->Flags = 0;          /* get rid of any flags     */
  UPDOWN_Refresh (wndPtr);     /* redraw the control just in case */
  
  return TRUE;
}

/***********************************************************************
 *           UPDOWN_HandleMouseEvent
 *
 * Handle a mouse event for the updown.
 * 'pt' is the location of the mouse event in client or
 * windows coordinates. 
 */
static void UPDOWN_HandleMouseEvent(WND *wndPtr, UINT32 msg, POINT32 pt)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr);
  RECT32 rect;
  int temp;

  switch(msg)
    {
    case WM_LBUTTONDOWN:  /* Initialise mouse tracking */
      /* If we are already in the 'clicked' mode, then nothing to do */
      if(infoPtr->Flags & FLAG_CLICKED)
	return;

      /* If the buddy is an edit, will set focus to it */
      if(WIDGETS_IsControl32(WIN_FindWndPtr(infoPtr->Buddy), BIC32_EDIT))
	SetFocus32(infoPtr->Buddy);

      /* Now see which one is the 'active' arrow */
      temp = UPDOWN_GetArrowFromPoint(wndPtr, &rect, pt);

      /* Update the CurVal if necessary */
      if(wndPtr->dwStyle & UDS_SETBUDDYINT)
	UPDOWN_GetBuddyInt(wndPtr);
	
      /* Before we proceed, see if we can spin... */
      if(!(wndPtr->dwStyle & UDS_WRAP))
	if(( temp && infoPtr->CurVal==infoPtr->MaxVal) ||
	   (!temp && infoPtr->CurVal==infoPtr->MinVal))
	  return;

      /* Set up the correct flags */
      infoPtr->Flags  = 0; 
      infoPtr->Flags |= temp ? FLAG_INCR : FLAG_DECR;
      infoPtr->Flags |= FLAG_MOUSEIN;
      
      /* repaint the control */
      UPDOWN_Refresh (wndPtr);

      /* process the click */
      UPDOWN_DoAction(wndPtr, 1, infoPtr->Flags & FLAG_INCR);

      /* now capture all mouse messages */
      SetCapture32(wndPtr->hwndSelf);

      /* and startup the first timer */
      SetTimer32(wndPtr->hwndSelf, TIMERID1, INITIAL_DELAY, 0); 
      break;

    case WM_MOUSEMOVE:
      /* If we are not in the 'clicked' mode, then nothing to do */
      if(!(infoPtr->Flags & FLAG_CLICKED))
	return;

      /* save the flags to see if any got modified */
      temp = infoPtr->Flags;

      /* Now get the 'active' arrow rectangle */
      if (infoPtr->Flags & FLAG_INCR)
	UPDOWN_GetArrowRect(wndPtr, &rect, TRUE);
      else
	UPDOWN_GetArrowRect(wndPtr, &rect, FALSE);

      /* Update the flags if we are in/out */
      if(PtInRect32(&rect, pt))
	infoPtr->Flags |=  FLAG_MOUSEIN;
      else{
	infoPtr->Flags &= ~FLAG_MOUSEIN;
	if(accelIndex != -1) /* if we have accel info */
	  accelIndex = 0;    /* reset it              */
      }
      /* If state changed, redraw the control */
      if(temp != infoPtr->Flags)
	UPDOWN_Refresh (wndPtr);
      break;

      default:
	ERR(updown, "Impossible case!\n");
    }

}

/***********************************************************************
 *           UpDownWndProc
 */
LRESULT WINAPI UpDownWindowProc(HWND32 hwnd, UINT32 message, WPARAM32 wParam,
				LPARAM lParam)
{
  WND *wndPtr = WIN_FindWndPtr(hwnd);
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(wndPtr); 
  int temp;

  switch(message)
    {
    case WM_NCCREATE:
      /* get rid of border, if any */
      wndPtr->dwStyle &= ~WS_BORDER;
      return TRUE;

    case WM_CREATE:
      infoPtr = (UPDOWN_INFO*)COMCTL32_Alloc (sizeof(UPDOWN_INFO));
      wndPtr->wExtra[0] = (DWORD)infoPtr;

      /* initialize the info struct */
      infoPtr->AccelCount=0; infoPtr->AccelVect=0; 
      infoPtr->CurVal=0; infoPtr->MinVal=0; infoPtr->MaxVal=100; /*FIXME*/
      infoPtr->Base  = 10; /* Default to base 10  */
      infoPtr->Buddy = 0;  /* No buddy window yet */
      infoPtr->Flags = 0;  /* And no flags        */

      /* Do we pick the buddy win ourselves? */
      if(wndPtr->dwStyle & UDS_AUTOBUDDY)
	UPDOWN_SetBuddy(wndPtr, GetWindow32(wndPtr->hwndSelf, GW_HWNDPREV));
	
      TRACE(updown, "UpDown Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      if(infoPtr->AccelVect)
	COMCTL32_Free (infoPtr->AccelVect);

      COMCTL32_Free (infoPtr);
      wndPtr->wExtra[0] = 0;

      TRACE(updown, "UpDown Ctrl destruction, hwnd=%04x\n", hwnd);
      break;
	
    case WM_ENABLE:
      if(wndPtr->dwStyle & WS_DISABLED)
	UPDOWN_CancelMode(wndPtr);
      UPDOWN_Paint(wndPtr);
      break;

    case WM_TIMER:
      /* if initial timer, kill it and start the repeat timer */
      if(wParam == TIMERID1){
	KillTimer32(hwnd, TIMERID1);
	/* if no accel info given, used default timer */
	if(infoPtr->AccelCount==0 || infoPtr->AccelVect==0){
	  accelIndex = -1;
	  temp = REPEAT_DELAY;
	}
	else{
	  accelIndex = 0; /* otherwise, use it */
	  temp = infoPtr->AccelVect[accelIndex].nSec * 1000 + 1;
	}
	SetTimer32(hwnd, TIMERID2, temp, 0);
      }

      /* now, if the mouse is above us, do the thing...*/
      if(infoPtr->Flags & FLAG_MOUSEIN){
	temp = accelIndex==-1 ? 1 : infoPtr->AccelVect[accelIndex].nInc;
	UPDOWN_DoAction(wndPtr, temp, infoPtr->Flags & FLAG_INCR);
	
	if(accelIndex!=-1 && accelIndex < infoPtr->AccelCount-1){
	  KillTimer32(hwnd, TIMERID2);
	  accelIndex++; /* move to the next accel info */
	  temp = infoPtr->AccelVect[accelIndex].nSec * 1000 + 1;
	  /* make sure we have at least 1ms intervals */
	  SetTimer32(hwnd, TIMERID2, temp, 0);	    
	}
      }
      break;

    case WM_CANCELMODE:
      UPDOWN_CancelMode(wndPtr);
      break;

    case WM_LBUTTONUP:
      if(!UPDOWN_CancelMode(wndPtr))
	break;
      /*If we released the mouse and our buddy is an edit */
      /* we must select all text in it.                   */
      if(WIDGETS_IsControl32(WIN_FindWndPtr(infoPtr->Buddy), BIC32_EDIT))
	SendMessage32A(infoPtr->Buddy, EM_SETSEL32, 0, MAKELONG(0, -1));
      break;
      
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
      if(UPDOWN_IsEnabled(wndPtr)){
	POINT32 pt;
	CONV_POINT16TO32( (POINT16 *)&lParam, &pt );
	UPDOWN_HandleMouseEvent( wndPtr, message, pt );
      }
    break;

    case WM_KEYDOWN:
      if((wndPtr->dwStyle & UDS_ARROWKEYS) && UPDOWN_IsEnabled(wndPtr)){
	switch(wParam){
	case VK_UP:  
	case VK_DOWN:
	  UPDOWN_GetBuddyInt(wndPtr);
	  UPDOWN_DoAction(wndPtr, 1, wParam==VK_UP);
	  break;
	}
      }
      break;
      
    case WM_PAINT:
      UPDOWN_Paint(wndPtr);
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
      UPDOWN_SetBuddy(wndPtr, wParam);
      TRACE(updown, "UpDown Ctrl new buddy(%04x), hwnd=%04x\n", 
		     infoPtr->Buddy, hwnd);
      return temp;

    case UDM_GETPOS:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = UPDOWN_GetBuddyInt(wndPtr);
      return MAKELONG(infoPtr->CurVal, temp ? 0 : 1);

    case UDM_SETPOS:
      if (wParam || HIWORD(lParam))
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = SLOWORD(lParam);
      TRACE(updown, "UpDown Ctrl new value(%d), hwnd=%04x\n",
		     temp, hwnd);
      if(!UPDOWN_InBounds(wndPtr, temp)){
	if(temp < infoPtr->MinVal)  
	  temp = infoPtr->MinVal;
	if(temp > infoPtr->MaxVal)
	  temp = infoPtr->MaxVal;
      }
      wParam = infoPtr->CurVal; /* save prev value   */
      infoPtr->CurVal = temp;   /* set the new value */
      if(wndPtr->dwStyle & UDS_SETBUDDYINT)
	UPDOWN_SetBuddyInt(wndPtr);
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
	*(LPINT32)wParam = infoPtr->MinVal;
      if (lParam)
	*(LPINT32)lParam = infoPtr->MaxVal;
      break;

    case UDM_SETRANGE32:
      infoPtr->MinVal = (INT32)wParam;
      infoPtr->MaxVal = (INT32)lParam;
      if (infoPtr->MaxVal <= infoPtr->MinVal)
	infoPtr->MaxVal = infoPtr->MinVal + 1;
      TRACE(updown, "UpDown Ctrl new range(%d to %d), hwnd=%04x\n", 
		     infoPtr->MinVal, infoPtr->MaxVal, hwnd);
      break;

    default: 
      if (message >= WM_USER) 
	ERR (updown, "unknown msg %04x wp=%04x lp=%08lx\n", 
	     message, wParam, lParam);
      return DefWindowProc32A (hwnd, message, wParam, lParam); 
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
    WNDCLASS32A wndClass;

    if( GlobalFindAtom32A( UPDOWN_CLASS32A ) ) return;

    ZeroMemory( &wndClass, sizeof( WNDCLASS32A ) );
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC32)UpDownWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(UPDOWN_INFO*);
    wndClass.hCursor       = LoadCursor32A( 0, IDC_ARROW32A );
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = UPDOWN_CLASS32A;
 
    RegisterClass32A( &wndClass );
}


/***********************************************************************
 *		UPDOWN_Unregister	[Internal]
 *
 * Unregisters the updown window class.
 */

VOID
UPDOWN_Unregister (VOID)
{
    if (GlobalFindAtom32A (UPDOWN_CLASS32A))
	UnregisterClass32A (UPDOWN_CLASS32A, (HINSTANCE32)NULL);
}

