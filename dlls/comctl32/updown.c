/*		
 * Updown control
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 * TODO:
 *   - I think I do not handle correctly the WS_BORDER style.
 *     (Should be fixed. <ekohl@abo.rhein-zeitung.de>)
 *
 * Testing:
 *   Not much. The following  have not been tested at all:
 *     - horizontal arrows
 *     - listbox as buddy window
 *     - acceleration
 *     - base 16
 *     - integers with thousand separators.
 *       (fixed bugs. <noel@macadamian.com>)
 *
 *   Even though the above list seems rather large, the control seems to
 *   behave very well so I am confident it does work in most (all) of the
 *   untested cases.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "winnls.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(updown);

#define UPDOWN_BUDDYCLASSNAMELEN 40

typedef struct
{
  HWND      Self;         /* Handle to this up-down control */
  UINT      AccelCount;   /* Number of elements in AccelVect */
  UDACCEL*  AccelVect;    /* Vector containing AccelCount elements */
  INT       AccelIndex;   /* Current accel index, -1 if not accelerating */
  INT       Base;         /* Base to display nr in the buddy window */
  INT       CurVal;       /* Current up-down value */
  INT       MinVal;       /* Minimum up-down value */
  INT       MaxVal;       /* Maximum up-down value */
  HWND      Buddy;        /* Handle to the buddy window */
  CHAR      szBuddyClass[UPDOWN_BUDDYCLASSNAMELEN]; /* Buddy window class name */
  INT       Flags;        /* Internal Flags FLAG_* */
} UPDOWN_INFO;

/* Control configuration constants */

#define INITIAL_DELAY    500 /* initial timer until auto-increment kicks in */
#define REPEAT_DELAY     50  /* delay between auto-increments */

#define DEFAULT_WIDTH       14  /* default width of the ctrl */
#define DEFAULT_XSEP         0  /* default separation between buddy and crtl */
#define DEFAULT_ADDTOP       0  /* amount to extend above the buddy window */
#define DEFAULT_ADDBOT       0  /* amount to extend below the buddy window */
#define DEFAULT_BUDDYBORDER  2  /* Width/height of the buddy border */


/* Work constants */

#define FLAG_INCR        0x01
#define FLAG_DECR        0x02
#define FLAG_MOUSEIN     0x04
#define FLAG_CLICKED     (FLAG_INCR | FLAG_DECR)

#define TIMERID1         1
#define TIMERID2         2
#define BUDDY_UPDOWN_HWND        "buddyUpDownHWND"
#define BUDDY_SUPERCLASS_WNDPROC "buddySupperClassWndProc"

#define UNKNOWN_PARAM(msg, wParam, lParam) WARN(\
        "Unknown parameter(s) for message " #msg \
	"(%04x): wp=%04x lp=%08lx\n", msg, wParam, lParam);

#define UPDOWN_GetInfoPtr(hwnd) ((UPDOWN_INFO *)GetWindowLongA (hwnd,0))

static LRESULT CALLBACK
UPDOWN_Buddy_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/***********************************************************************
 *           UPDOWN_InBounds
 * Tests if a given value 'val' is between the Min&Max limits
 */
static BOOL UPDOWN_InBounds(UPDOWN_INFO *infoPtr, int val)
{
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
static BOOL UPDOWN_OffsetVal(UPDOWN_INFO *infoPtr, int delta)
{
  /* check if we can do the modification first */
  if(!UPDOWN_InBounds (infoPtr, infoPtr->CurVal+delta)){
    if (GetWindowLongW (infoPtr->Self, GWL_STYLE) & UDS_WRAP)
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
 * UPDOWN_HasBuddyBorder [Internal]
 *
 * When we have a buddy set and that we are aligned on our buddy, we
 * want to draw a sunken edge to make like we are part of that control.
 */
static BOOL UPDOWN_HasBuddyBorder(UPDOWN_INFO* infoPtr)
{  
  DWORD dwStyle = GetWindowLongW (infoPtr->Self, GWL_STYLE);

  return  ( ((dwStyle & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT)) != 0) &&
	    (SendMessageW(infoPtr->Self, UDM_GETBUDDY, 0, 0) != 0) &&
	    (lstrcmpiA(infoPtr->szBuddyClass, "EDIT") == 0 ) );
}

/***********************************************************************
 *           UPDOWN_GetArrowRect
 * wndPtr   - pointer to the up-down wnd
 * rect     - will hold the rectangle
 * incr     - TRUE  get the "increment" rect (up or right)
 *            FALSE get the "decrement" rect (down or left)
 */
static void UPDOWN_GetArrowRect (UPDOWN_INFO* infoPtr, RECT *rect, BOOL incr)
{
  DWORD dwStyle = GetWindowLongW (infoPtr->Self, GWL_STYLE);
  int   len; /* will hold the width or height */

  GetClientRect (infoPtr->Self, rect);

  /*
   * Make sure we calculate the rectangle to fit even if we draw the
   * border.
   */
  if (UPDOWN_HasBuddyBorder(infoPtr)) {
    if (dwStyle & UDS_ALIGNLEFT)
      rect->left += DEFAULT_BUDDYBORDER;
    else
      rect->right -= DEFAULT_BUDDYBORDER;
    
    InflateRect(rect, 0, -DEFAULT_BUDDYBORDER);
  }

  /*
   * We're calculating the midpoint to figure-out where the
   * separation between the buttons will lay. We make sure that we
   * round the uneven numbers by adding 1.
   */
  if (dwStyle & UDS_HORZ) {
    len = rect->right - rect->left + 1; /* compute the width */
    if (incr)
      rect->left = rect->left + len/2; 
    else
      rect->right =  rect->left + len/2;
  } else {
    len = rect->bottom - rect->top + 1; /* compute the height */
    if (incr)
      rect->bottom =  rect->top + len/2;
    else
      rect->top =  rect->top + len/2;
  }
}

/***********************************************************************
 *           UPDOWN_GetArrowFromPoint
 * Returns the rectagle (for the up or down arrow) that contains pt.
 * If it returns the up rect, it returns TRUE.
 * If it returns the down rect, it returns FALSE.
 */
static BOOL UPDOWN_GetArrowFromPoint (UPDOWN_INFO* infoPtr, RECT *rect, POINT pt)
{
  UPDOWN_GetArrowRect (infoPtr, rect, TRUE);
  if(PtInRect(rect, pt)) return TRUE;

  UPDOWN_GetArrowRect (infoPtr, rect, FALSE);
  return FALSE;
}


/***********************************************************************
 *           UPDOWN_GetThousandSep
 * Returns the thousand sep. If an error occurs, it returns ','.
 */
static CHAR UPDOWN_GetThousandSep()
{
  CHAR sep[2];

  if(GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 2) != 1)
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
static BOOL UPDOWN_GetBuddyInt (UPDOWN_INFO *infoPtr)
{
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
      if(*src != sep) *dst++ = *src;
    *dst = 0;

    /* try to convert the number and validate it */
    newVal = strtol(txt, &src, infoPtr->Base);
    if(*src || !UPDOWN_InBounds (infoPtr, newVal)) 
      return FALSE;

    TRACE("new value(%d) from buddy (old=%d)\n", newVal, infoPtr->CurVal);
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
static BOOL UPDOWN_SetBuddyInt (UPDOWN_INFO *infoPtr)
{
  char txt1[20], sep;
  int len;

  if (!IsWindow(infoPtr->Buddy)) 
    return FALSE;

  TRACE("set new value(%d) to buddy.\n", infoPtr->CurVal);

  /*if the buddy is a list window, we must set curr index */
  if(!lstrcmpA (infoPtr->szBuddyClass, "ListBox")){
    SendMessageA(infoPtr->Buddy, LB_SETCURSEL, infoPtr->CurVal, 0);
  }
  else{ /* Regular window, so set caption to the number */
    len = sprintf(txt1, (infoPtr->Base==16) ? "%X" : "%d", infoPtr->CurVal);

    sep = UPDOWN_GetThousandSep(); 

    /* Do thousands seperation if necessary */
    if (!(GetWindowLongA (infoPtr->Self, GWL_STYLE) & UDS_NOTHOUSANDS) && (len > 3)) {
      char txt2[20], *src = txt1, *dst = txt2;
      if(len % 3 > 0) {
	lstrcpynA (dst, src, len%3 + 1);      /* need to include the null */ 
	dst += len%3;
	src += len%3;
      }
      for(len=0; *src; len++) {
	if(len%3==0) *dst++ = sep;
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
 * UPDOWN_DrawBuddyBorder [Internal]
 *
 * When we have a buddy set and that we are aligned on our buddy, we
 * want to draw a sunken edge to make like we are part of that control.
 */
static void UPDOWN_DrawBuddyBorder (UPDOWN_INFO *infoPtr, HDC hdc)
{
  DWORD dwStyle = GetWindowLongA (infoPtr->Self, GWL_STYLE);
  RECT  clientRect;

  GetClientRect(infoPtr->Self, &clientRect);

  if (dwStyle & UDS_ALIGNLEFT)
    DrawEdge(hdc, &clientRect, EDGE_SUNKEN, BF_BOTTOM | BF_LEFT | BF_TOP);
  else
    DrawEdge(hdc, &clientRect, EDGE_SUNKEN, BF_BOTTOM | BF_RIGHT | BF_TOP);
}

/***********************************************************************
 * UPDOWN_Draw [Internal]
 *
 * Draw the arrows. The background need not be erased.
 */
static void UPDOWN_Draw (UPDOWN_INFO *infoPtr, HDC hdc)
{
  DWORD dwStyle = GetWindowLongA (infoPtr->Self, GWL_STYLE);
  BOOL prssed;
  RECT rect;

  /*
   * Draw the common border between ourselves and our buddy.
   */
  if (UPDOWN_HasBuddyBorder(infoPtr))
    UPDOWN_DrawBuddyBorder(infoPtr, hdc);
  
  /* Draw the incr button */
  UPDOWN_GetArrowRect (infoPtr, &rect, TRUE);
  prssed = (infoPtr->Flags & FLAG_INCR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl(hdc, &rect, DFC_SCROLL, 
	(dwStyle & UDS_HORZ ? DFCS_SCROLLRIGHT : DFCS_SCROLLUP) |
	(prssed ? DFCS_PUSHED : 0) |
	(dwStyle&WS_DISABLED ? DFCS_INACTIVE : 0) );

  /* Draw the space between the buttons */
  rect.top = rect.bottom; rect.bottom++;
  DrawEdge(hdc, &rect, 0, BF_MIDDLE);
		    
  /* Draw the decr button */
  UPDOWN_GetArrowRect(infoPtr, &rect, FALSE);
  prssed = (infoPtr->Flags & FLAG_DECR) && (infoPtr->Flags & FLAG_MOUSEIN);
  DrawFrameControl(hdc, &rect, DFC_SCROLL, 
	(dwStyle & UDS_HORZ ? DFCS_SCROLLLEFT : DFCS_SCROLLDOWN) |
	(prssed ? DFCS_PUSHED : 0) |
	(dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
}

/***********************************************************************
 * UPDOWN_Refresh [Internal]
 *
 * Synchronous drawing (must NOT be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Refresh (UPDOWN_INFO *infoPtr)
{
    HDC hdc = GetDC (infoPtr->Self);
    UPDOWN_Draw (infoPtr, hdc);
    ReleaseDC (infoPtr->Self, hdc);
}


/***********************************************************************
 * UPDOWN_Paint [Internal]
 *
 * Asynchronous drawing (must ONLY be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static void UPDOWN_Paint (UPDOWN_INFO *infoPtr, HDC hdc)
{
    if (hdc) {
      UPDOWN_Draw (infoPtr, hdc);
    } else {
      PAINTSTRUCT ps;

      hdc = BeginPaint (infoPtr->Self, &ps);
      UPDOWN_Draw (infoPtr, hdc);
      EndPaint (infoPtr->Self, &ps);
    }
}

/***********************************************************************
 *           UPDOWN_SetBuddy
 * Tests if 'bud' is a valid window handle. If not, returns FALSE.
 * Else, sets it as a new Buddy.
 * Then, it should subclass the buddy 
 * If window has the UDS_ARROWKEYS, it subcalsses the buddy window to
 * process the UP/DOWN arrow keys.
 * If window has the UDS_ALIGNLEFT or UDS_ALIGNRIGHT style
 * the size/pos of the buddy and the control are adjusted accordingly.
 */
static BOOL UPDOWN_SetBuddy (UPDOWN_INFO* infoPtr, HWND bud)
{
  DWORD        dwStyle = GetWindowLongA (infoPtr->Self, GWL_STYLE);
  RECT         budRect;  /* new coord for the buddy */
  int          x,width;  /* new x position and width for the up-down */
  WNDPROC baseWndProc, currWndProc;
 	  
  /* Is it a valid bud? */
  if(!IsWindow(bud)) return FALSE;

  /* there is already a body assigned */
  if ( infoPtr->Buddy )  
    RemovePropA(infoPtr->Buddy, BUDDY_UPDOWN_HWND);

  /* Store buddy window handle */
  infoPtr->Buddy = bud;   

  /* keep upDown ctrl hwnd in a buddy property */            
  SetPropA( bud, BUDDY_UPDOWN_HWND, infoPtr->Self); 

  /* Store buddy window clas name */
  memset(infoPtr->szBuddyClass, 0, UPDOWN_BUDDYCLASSNAMELEN);
  GetClassNameA (bud, infoPtr->szBuddyClass, UPDOWN_BUDDYCLASSNAMELEN-1);

  if(dwStyle & UDS_ARROWKEYS){
    /* Note that I don't clear the BUDDY_SUPERCLASS_WNDPROC property 
       when we reset the upDown ctrl buddy to another buddy because it is not 
       good to break the window proc chain. */

	currWndProc = (WNDPROC) GetWindowLongA(bud, GWL_WNDPROC);
	if (currWndProc != UPDOWN_Buddy_SubclassProc) 
	{
		// replace the buddy's WndProc with ours
		baseWndProc = (WNDPROC)SetWindowLongA(bud, GWL_WNDPROC,
			      (LPARAM)UPDOWN_Buddy_SubclassProc); 
		// and save the base class' WndProc 
 		SetPropA(bud, BUDDY_SUPERCLASS_WNDPROC, (HANDLE)baseWndProc);
	}
	// else
	// its already been subclassed, don't overwrite BUDDY_SUPERCLASS_WNDPROC
  }

  /* Get the rect of the buddy relative to its parent */
  GetWindowRect(infoPtr->Buddy, &budRect);
  MapWindowPoints(HWND_DESKTOP, GetParent(infoPtr->Buddy),
		  (POINT *)(&budRect.left), 2);

  /* now do the positioning */
  if  (dwStyle & UDS_ALIGNLEFT) {
    x  = budRect.left;
    budRect.left += DEFAULT_WIDTH+DEFAULT_XSEP;
  }
  else if (dwStyle & UDS_ALIGNRIGHT){
    budRect.right -= DEFAULT_WIDTH+DEFAULT_XSEP;
    x  = budRect.right+DEFAULT_XSEP;
  }
  else {
    x  = budRect.right+DEFAULT_XSEP;
  }

  /* first adjust the buddy to accomodate the up/down */
  SetWindowPos(infoPtr->Buddy, 0, budRect.left, budRect.top,
	       budRect.right  - budRect.left, budRect.bottom - budRect.top, 
	       SWP_NOACTIVATE|SWP_NOZORDER);

  /* now position the up/down */
  /* Since the UDS_ALIGN* flags were used, */
  /* we will pick the position and size of the window. */
  width = DEFAULT_WIDTH;

  /*
   * If the updown has a buddy border, it has to overlap with the buddy
   * to look as if it is integrated with the buddy control. 
   * We nudge the control or change it size to overlap.
   */
  if (UPDOWN_HasBuddyBorder(infoPtr))
  {
    if(dwStyle & UDS_ALIGNLEFT)
      width+=DEFAULT_BUDDYBORDER;
    else
      x-=DEFAULT_BUDDYBORDER;
  }

  SetWindowPos (infoPtr->Self, infoPtr->Buddy, 
		x, budRect.top-DEFAULT_ADDTOP,
		width, (budRect.bottom-budRect.top)+DEFAULT_ADDTOP+DEFAULT_ADDBOT,
		SWP_NOACTIVATE);

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
static void UPDOWN_DoAction (UPDOWN_INFO *infoPtr, int delta, BOOL incr)
{
  DWORD dwStyle = GetWindowLongA (infoPtr->Self, GWL_STYLE);
  NM_UPDOWN ni;

  TRACE("%s by %d\n", incr ? "inc" : "dec", delta);

  /* check if we can do the modification first */
  delta *= (incr ? 1 : -1) * (infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1);

  /* We must notify parent now to obtain permission */
  ni.iPos = infoPtr->CurVal;
  ni.iDelta = delta;
  ni.hdr.hwndFrom = infoPtr->Self;
  ni.hdr.idFrom   = GetWindowLongA (infoPtr->Self, GWL_ID);
  ni.hdr.code = UDN_DELTAPOS; 
  if (!SendMessageA(GetParent (infoPtr->Self), WM_NOTIFY,
		   (WPARAM)ni.hdr.idFrom, (LPARAM)&ni))
  {
     /* Parent said: OK to adjust */

     /* Now adjust value with (maybe new) delta */
     if (UPDOWN_OffsetVal (infoPtr, ni.iDelta))
     {
        /* Now take care about our buddy */
        if(infoPtr->Buddy && IsWindow(infoPtr->Buddy)
           && (dwStyle & UDS_SETBUDDYINT) )
           UPDOWN_SetBuddyInt (infoPtr);
     }
  }
  
  /* Also, notify it. This message is sent in any case. */
  SendMessageA (GetParent (infoPtr->Self), 
		dwStyle & UDS_HORZ ? WM_HSCROLL : WM_VSCROLL, 
		 MAKELONG(SB_THUMBPOSITION, infoPtr->CurVal), infoPtr->Self);
}

/***********************************************************************
 *           UPDOWN_IsEnabled
 *
 * Returns TRUE if it is enabled as well as its buddy (if any)
 *         FALSE otherwise
 */
static BOOL UPDOWN_IsEnabled (UPDOWN_INFO *infoPtr)
{
  if(GetWindowLongA (infoPtr->Self, GWL_STYLE) & WS_DISABLED)
    return FALSE;
  if(infoPtr->Buddy)
    return IsWindowEnabled(infoPtr->Buddy);
  return TRUE;
}

/***********************************************************************
 *           UPDOWN_CancelMode
 *
 * Deletes any timers, releases the mouse and does  redraw if necessary.
 * If the control is not in "capture" mode, it does nothing.
 * If the control was not in cancel mode, it returns FALSE. 
 * If the control was in cancel mode, it returns TRUE.
 */
static BOOL UPDOWN_CancelMode (UPDOWN_INFO *infoPtr)
{
  /* if not in 'capture' mode, do nothing */
  if(!(infoPtr->Flags & FLAG_CLICKED))
    return FALSE;

  KillTimer (infoPtr->Self, TIMERID1); /* kill all possible timers */
  KillTimer (infoPtr->Self, TIMERID2);
  
  if (GetCapture() == infoPtr->Self)   /* let the mouse go         */
    ReleaseCapture();                  /* if we still have it      */  
  
  infoPtr->Flags = 0;                  /* get rid of any flags     */
  UPDOWN_Refresh (infoPtr);            /* redraw the control just in case */
  
  return TRUE;
}

/***********************************************************************
 *           UPDOWN_HandleMouseEvent
 *
 * Handle a mouse event for the updown.
 * 'pt' is the location of the mouse event in client or
 * windows coordinates. 
 */
static void UPDOWN_HandleMouseEvent (UPDOWN_INFO *infoPtr, UINT msg, POINT pt)
{
  DWORD dwStyle = GetWindowLongA (infoPtr->Self, GWL_STYLE);
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
      temp = UPDOWN_GetArrowFromPoint (infoPtr, &rect, pt);

      /* Update the CurVal if necessary */
      if (dwStyle & UDS_SETBUDDYINT)
	UPDOWN_GetBuddyInt (infoPtr);
	
      /* Set up the correct flags */
      infoPtr->Flags  = 0; 
      infoPtr->Flags |= temp ? FLAG_INCR : FLAG_DECR;
      infoPtr->Flags |= FLAG_MOUSEIN;
      
      /* repaint the control */
      UPDOWN_Refresh (infoPtr);

      /* process the click */
      UPDOWN_DoAction (infoPtr, 1, infoPtr->Flags & FLAG_INCR);

      /* now capture all mouse messages */
      SetCapture (infoPtr->Self);

      /* and startup the first timer */
      SetTimer(infoPtr->Self, TIMERID1, INITIAL_DELAY, 0); 
      break;

    case WM_MOUSEMOVE:
      /* If we are not in the 'clicked' mode, then nothing to do */
      if(!(infoPtr->Flags & FLAG_CLICKED))
	return;

      /* save the flags to see if any got modified */
      temp = infoPtr->Flags;

      /* Now get the 'active' arrow rectangle */
      if (infoPtr->Flags & FLAG_INCR)
	UPDOWN_GetArrowRect (infoPtr, &rect, TRUE);
      else
	UPDOWN_GetArrowRect (infoPtr, &rect, FALSE);

      /* Update the flags if we are in/out */
      if(PtInRect(&rect, pt))
	infoPtr->Flags |=  FLAG_MOUSEIN;
      else{
	infoPtr->Flags &= ~FLAG_MOUSEIN;
	if(infoPtr->AccelIndex != -1) /* if we have accel info */
	  infoPtr->AccelIndex = 0;    /* reset it              */
      }
      /* If state changed, redraw the control */
      if(temp != infoPtr->Flags)
	UPDOWN_Refresh (infoPtr);
      break;

      default:
	ERR("Impossible case!\n");
    }

}

/***********************************************************************
 *           UpDownWndProc
 */
static LRESULT WINAPI UpDownWindowProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
  UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
  DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
  int temp;
  if (!infoPtr && (message != WM_CREATE) && (message != WM_NCCREATE))
      return DefWindowProcA (hwnd, message, wParam, lParam); 
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
      infoPtr->Self = hwnd;
      infoPtr->AccelCount = 0;
      infoPtr->AccelVect = 0;
      infoPtr->AccelIndex = -1;
      infoPtr->CurVal = 0; 
      infoPtr->MinVal = 0; 
      infoPtr->MaxVal = 9999;
      infoPtr->Base  = 10; /* Default to base 10  */
      infoPtr->Buddy = 0;  /* No buddy window yet */
      infoPtr->Flags = 0;  /* And no flags        */

      /* Do we pick the buddy win ourselves? */
      if (dwStyle & UDS_AUTOBUDDY)
	UPDOWN_SetBuddy (infoPtr, GetWindow (hwnd, GW_HWNDPREV));
	
      TRACE("UpDown Ctrl creation, hwnd=%04x\n", hwnd);
      break;
    
    case WM_DESTROY:
      if(infoPtr->AccelVect)
	COMCTL32_Free (infoPtr->AccelVect);

      if ( IsWindow(infoPtr->Buddy) ) /* Cleanup */
        RemovePropA(infoPtr->Buddy, BUDDY_UPDOWN_HWND);

      COMCTL32_Free (infoPtr);
      SetWindowLongA (hwnd, 0, 0);
      TRACE("UpDown Ctrl destruction, hwnd=%04x\n", hwnd);
      break;
	
    case WM_ENABLE:
      if (dwStyle & WS_DISABLED)
	UPDOWN_CancelMode (infoPtr);

      UPDOWN_Refresh (infoPtr);
      break;

    case WM_TIMER:
      /* if initial timer, kill it and start the repeat timer */
      if(wParam == TIMERID1){
	KillTimer(hwnd, TIMERID1);
	/* if no accel info given, used default timer */
	if(infoPtr->AccelCount==0 || infoPtr->AccelVect==0){
	  infoPtr->AccelIndex = -1;
	  temp = REPEAT_DELAY;
	}
	else{
	  infoPtr->AccelIndex = 0; /* otherwise, use it */
	  temp = infoPtr->AccelVect[infoPtr->AccelIndex].nSec * 1000 + 1;
	}
	SetTimer(hwnd, TIMERID2, temp, 0);
      }

      /* now, if the mouse is above us, do the thing...*/
      if(infoPtr->Flags & FLAG_MOUSEIN){
	temp = infoPtr->AccelIndex == -1 ? 1 : infoPtr->AccelVect[infoPtr->AccelIndex].nInc;
	UPDOWN_DoAction(infoPtr, temp, infoPtr->Flags & FLAG_INCR);
	
	if(infoPtr->AccelIndex != -1 && infoPtr->AccelIndex < infoPtr->AccelCount-1){
	  KillTimer(hwnd, TIMERID2);
	  infoPtr->AccelIndex++; /* move to the next accel info */
	  temp = infoPtr->AccelVect[infoPtr->AccelIndex].nSec * 1000 + 1;
	  /* make sure we have at least 1ms intervals */
	  SetTimer(hwnd, TIMERID2, temp, 0);	    
	}
      }
      break;

    case WM_CANCELMODE:
      UPDOWN_CancelMode (infoPtr);
      break;

    case WM_LBUTTONUP:
      if(!UPDOWN_CancelMode(infoPtr))
	break;

      SendMessageA(GetParent(hwnd), dwStyle & UDS_HORZ ? WM_HSCROLL : WM_VSCROLL,
                  MAKELONG(SB_ENDSCROLL, infoPtr->CurVal), hwnd);

      /*If we released the mouse and our buddy is an edit */
      /* we must select all text in it.                   */
      if (!lstrcmpA (infoPtr->szBuddyClass, "Edit"))
	  SendMessageA(infoPtr->Buddy, EM_SETSEL, 0, MAKELONG(0, -1));
      break;
      
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
      if(UPDOWN_IsEnabled(infoPtr)){
	POINT pt;
        pt.x = SLOWORD(lParam);
        pt.y = SHIWORD(lParam);
	UPDOWN_HandleMouseEvent (infoPtr, message, pt );
      }
    break;

    case WM_KEYDOWN:
      if((dwStyle & UDS_ARROWKEYS) && UPDOWN_IsEnabled(infoPtr)){
	switch(wParam){
	case VK_UP:  
	case VK_DOWN:
	  UPDOWN_GetBuddyInt (infoPtr);
	  /* FIXME: Paint the according button pressed for some time, like win95 does*/
	  UPDOWN_DoAction (infoPtr, 1, wParam==VK_UP);
	  break;
	}
      }
      break;
      
    case WM_PAINT:
      UPDOWN_Paint (infoPtr, (HDC)wParam);
      break;
    
    case UDM_GETACCEL:
      if (wParam==0 && lParam==0)    /*if both zero, */
	return infoPtr->AccelCount;  /*just return the accel count*/
      if (wParam || lParam){
	UNKNOWN_PARAM(UDM_GETACCEL, wParam, lParam);
	return 0;
      }
      temp = min(infoPtr->AccelCount, wParam);
      memcpy((void *)lParam, infoPtr->AccelVect, temp*sizeof(UDACCEL));
      return temp;

    case UDM_SETACCEL:
      TRACE("UpDown Ctrl new accel info, hwnd=%04x\n", hwnd);
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
      TRACE("UpDown Ctrl new base(%d), hwnd=%04x\n", wParam, hwnd);
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
      UPDOWN_SetBuddy (infoPtr, wParam);
      TRACE("UpDown Ctrl new buddy(%04x), hwnd=%04x\n", infoPtr->Buddy, hwnd);
      return temp;

    case UDM_GETPOS:
      if (wParam || lParam)
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = UPDOWN_GetBuddyInt (infoPtr);
      return MAKELONG(infoPtr->CurVal, temp ? 0 : 1);

    case UDM_SETPOS:
      if (wParam || HIWORD(lParam))
	UNKNOWN_PARAM(UDM_GETPOS, wParam, lParam);
      temp = SLOWORD(lParam);
      TRACE("UpDown Ctrl new value(%d), hwnd=%04x\n", temp, hwnd);
      if(!UPDOWN_InBounds(infoPtr, temp)){
	if(temp < infoPtr->MinVal)  
	  temp = infoPtr->MinVal;
	if(temp > infoPtr->MaxVal)
	  temp = infoPtr->MaxVal;
      }
      wParam = infoPtr->CurVal; /* save prev value   */
      infoPtr->CurVal = temp;   /* set the new value */
      if(dwStyle & UDS_SETBUDDYINT)
	UPDOWN_SetBuddyInt (infoPtr);
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
      TRACE("UpDown Ctrl new range(%d to %d), hwnd=%04x\n", 
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
      TRACE("UpDown Ctrl new range(%d to %d), hwnd=%04x\n", 
	    infoPtr->MinVal, infoPtr->MaxVal, hwnd);
      break;

    case UDM_GETPOS32:
      if ((LPBOOL)lParam != NULL)
	*((LPBOOL)lParam) = TRUE;
      return infoPtr->CurVal;

    case UDM_SETPOS32:
      if(!UPDOWN_InBounds(infoPtr, (int)lParam)){
	if((int)lParam < infoPtr->MinVal)
	  lParam = infoPtr->MinVal;
	if((int)lParam > infoPtr->MaxVal)
	  lParam = infoPtr->MaxVal;
      }
      temp = infoPtr->CurVal; /* save prev value   */
      infoPtr->CurVal = (int)lParam;   /* set the new value */
      if(dwStyle & UDS_SETBUDDYINT)
	UPDOWN_SetBuddyInt (infoPtr);
      return temp;            /* return prev value */

    default: 
      if (message >= WM_USER) 
      	ERR("unknown msg %04x wp=%04x lp=%08lx\n", message, wParam, lParam);
      return DefWindowProcA (hwnd, message, wParam, lParam); 
    } 

    return 0;
}

/***********************************************************************
 * UPDOWN_Buddy_SubclassProc used to handle messages sent to the buddy 
 *                           control.
 */
LRESULT CALLBACK
UPDOWN_Buddy_SubclassProc (
  HWND   hwnd, 
  UINT   uMsg, 
  WPARAM wParam, 
  LPARAM lParam)
{
  WNDPROC superClassWndProc = (WNDPROC)GetPropA(hwnd, BUDDY_SUPERCLASS_WNDPROC);
  TRACE("hwnd=%04x, wndProc=%d, uMsg=%04x, wParam=%d, lParam=%d\n", 
	 hwnd, (INT)superClassWndProc, uMsg, wParam, (UINT)lParam);

  switch (uMsg) 
  {
    case WM_KEYDOWN:
    {
      if ( ((int)wParam == VK_UP ) || ((int)wParam == VK_DOWN ) )
      {
        HWND upDownHwnd      = GetPropA(hwnd, BUDDY_UPDOWN_HWND);
        UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr(upDownHwnd);
      
        if (!lstrcmpA (infoPtr->szBuddyClass, "ListBox"))
        {
          /* if the buddy is a list window, we must update curr index */
          INT oldVal = SendMessageA(hwnd, LB_GETCURSEL, 0, 0);
          SendMessageA(hwnd, LB_SETCURSEL, oldVal+1, 0);
        }
        else
        {
	  UPDOWN_GetBuddyInt(infoPtr);
          UPDOWN_DoAction(infoPtr, 1, wParam==VK_UP);
        }

        break;
      }
      /* else Fall Through */
    }
  }
  return CallWindowProcA( superClassWndProc, hwnd, uMsg, wParam, lParam);
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
UPDOWN_Unregister (void)
{
    UnregisterClassA (UPDOWN_CLASSA, (HINSTANCE)NULL);
}

