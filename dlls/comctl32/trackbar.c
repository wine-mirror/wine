/*
 * Trackbar control
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 * Copyright 1998,1999 Alex Priem <alexp@sci.kun.nl>
 *
 *
 * TODO:
 *   - Some messages.
 *   - more display code.
 *   - handle dragging slider better
 *   - better tic handling.
 *   - more notifications.
 *   
 */

/* known bugs:

	-TBM_SETRANGEMAX & TBM_SETRANGEMIN should only change the view of the
   trackbar, not the actual amount of tics in the list.
	-TBM_GETTIC & TBM_GETTICPOS shouldn't rely on infoPtr->tics being sorted.
	- Make drawing code exact match of w95 drawing.
*/



#include "commctrl.h"
#include "trackbar.h"
#include "win.h"
#include "debug.h"


#define TRACKBAR_GetInfoPtr(wndPtr) ((TRACKBAR_INFO *)wndPtr->wExtra[0])


/* Used by TRACKBAR_Refresh to find out which parts of the control 
	need to be recalculated */

#define TB_THUMBPOSCHANGED 	1	
#define TB_THUMBSIZECHANGED 2
#define TB_THUMBCHANGED 	(TB_THUMBPOSCHANGED | TB_THUMBPOSCHANGED)
#define TB_SELECTIONCHANGED 4

#define TB_DRAG_MODE		16		/* we're dragging the slider */
#define TB_DRAGPOSVALID  	32		/* current Position is in dragPos */
#define TB_SHOW_TOOLTIP  	64		/* tooltip-style enabled and tooltip on */
#define TB_REFRESH_TIMER_SET	128     /* is a TRACBKAR_Refresh queued?*/


/* helper defines for TRACKBAR_DrawTic */
#define TIC_LEFTEDGE 			0x20
#define TIC_RIGHTEDGE			0x40
#define TIC_EDGE				(TIC_LEFTEDGE | TIC_RIGHTEDGE)
#define TIC_SELECTIONMARKMAX 	0x80
#define TIC_SELECTIONMARKMIN 	0x100
#define TIC_SELECTIONMARK		(TIC_SELECTIONMARKMAX | TIC_SELECTIONMARKMIN)

static BOOL32 TRACKBAR_SendNotify (WND *wndPtr, UINT32 code);

void TRACKBAR_RecalculateTics (TRACKBAR_INFO *infoPtr)

{
    int i,tic,nrTics;

	if (infoPtr->uTicFreq) 
    	nrTics=(infoPtr->nRangeMax - infoPtr->nRangeMin)/infoPtr->uTicFreq;
	else {
		nrTics=0;
		COMCTL32_Free (infoPtr->tics);
		infoPtr->tics=NULL;
		infoPtr->uNumTics=0;
		return;
	}

    if (nrTics!=infoPtr->uNumTics) {
    	infoPtr->tics=COMCTL32_ReAlloc (infoPtr->tics, (nrTics+1)*sizeof (DWORD));
    	infoPtr->uNumTics=nrTics;
    }
	infoPtr->uNumTics=nrTics;
    tic=infoPtr->nRangeMin+infoPtr->uTicFreq;
    for (i=0; i<nrTics; i++,tic+=infoPtr->uTicFreq)
               infoPtr->tics[i]=tic;
}


/* converts from physical (mouse) position to logical position 
   (in range of trackbar) */

static inline INT32
TRACKBAR_ConvertPlaceToPosition (TRACKBAR_INFO *infoPtr, int place, 
								int vertical) 
{
	double range,width,pos;

    range=infoPtr->nRangeMax - infoPtr->nRangeMin;
    if (vertical) {
    	width=infoPtr->rcChannel.bottom - infoPtr->rcChannel.top;
		pos=(range*(place - infoPtr->rcChannel.top)) / width;
	} else {
    	width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;
		pos=(range*(place - infoPtr->rcChannel.left)) / width;
	}
	
    TRACE (trackbar,"%.2f\n",pos);
    return pos;
}



static VOID
TRACKBAR_CalcChannel (WND *wndPtr, TRACKBAR_INFO *infoPtr)
{
    INT32 cyChannel;
	RECT32 lpRect,*channel = & infoPtr->rcChannel;

    GetClientRect32 (wndPtr->hwndSelf, &lpRect);

    if (wndPtr->dwStyle & TBS_ENABLESELRANGE)
		cyChannel = MAX(infoPtr->uThumbLen - 8, 4);
    else
		cyChannel = 4;

    if (wndPtr->dwStyle & TBS_VERT) {
		channel->top    = lpRect.top + 8;
		channel->bottom = lpRect.bottom - 8;

			if (wndPtr->dwStyle & TBS_BOTH) {
	    		channel->left  = (lpRect.right - cyChannel) / 2;
	    		channel->right = (lpRect.right + cyChannel) / 2;
			}
			else if (wndPtr->dwStyle & TBS_LEFT) {
	    			channel->left  = lpRect.left + 10;
	    			channel->right = channel->left + cyChannel;
				}
				else { /* TBS_RIGHT */
	    			channel->right = lpRect.right - 10;
	    			channel->left  = channel->right - cyChannel;
				}
   	}
   	else {
			channel->left = lpRect.left + 8;
			channel->right = lpRect.right - 8;
			if (wndPtr->dwStyle & TBS_BOTH) {
	    		channel->top		= (lpRect.bottom - cyChannel) / 2;
	    		channel->bottom 	= (lpRect.bottom + cyChannel) / 2;
			}
			else if (wndPtr->dwStyle & TBS_TOP) {
	    			channel->top    = lpRect.top + 10;
	    			channel->bottom = channel->top + cyChannel;
				}
				else { /* TBS_BOTTOM */
	    			channel->bottom = lpRect.bottom - 10;
	    			channel->top    = channel->bottom - cyChannel;
				}
	}
}

static VOID
TRACKBAR_CalcThumb (WND *wndPtr, TRACKBAR_INFO *infoPtr)

{
	RECT32 *thumb;
	int range, width;
	
	thumb=&infoPtr->rcThumb;
	range=infoPtr->nRangeMax - infoPtr->nRangeMin;
	if (wndPtr->dwStyle & TBS_VERT) {
    	width=infoPtr->rcChannel.bottom - infoPtr->rcChannel.top;
		thumb->left  = infoPtr->rcChannel.left - 1;
		thumb->right  = infoPtr->rcChannel.left + infoPtr->uThumbLen - 8;
		thumb->top	 = infoPtr->rcChannel.top +
						(width*infoPtr->nPos)/range - 5;
		thumb->bottom = thumb->top + infoPtr->uThumbLen/3;

	} else {
    	width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;
		thumb->left   = infoPtr->rcChannel.left +
					     (width*infoPtr->nPos)/range - 5;
		thumb->right  = thumb->left + infoPtr->uThumbLen/3;
		thumb->top	  = infoPtr->rcChannel.top - 1;
		thumb->bottom = infoPtr->rcChannel.top + infoPtr->uThumbLen - 8;
	}
}

static VOID
TRACKBAR_CalcSelection (WND *wndPtr, TRACKBAR_INFO *infoPtr)
{
	RECT32 *selection;
	int range, width;

	selection= & infoPtr->rcSelection;
	range=infoPtr->nRangeMax - infoPtr->nRangeMin;
	width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;

	if (range <= 0) 
		SetRectEmpty32 (selection);
	else 
		if (wndPtr->dwStyle & TBS_VERT) {
			selection->left   = infoPtr->rcChannel.left +
								(width*infoPtr->nSelMin)/range;
			selection->right  = infoPtr->rcChannel.left +
								(width*infoPtr->nSelMax)/range;
			selection->top    = infoPtr->rcChannel.top + 2;
			selection->bottom = infoPtr->rcChannel.bottom - 2;
		} else {
			selection->top    = infoPtr->rcChannel.top +
								(width*infoPtr->nSelMin)/range;
			selection->bottom = infoPtr->rcChannel.top +
								(width*infoPtr->nSelMax)/range;
			selection->left   = infoPtr->rcChannel.left + 2;
			selection->right  = infoPtr->rcChannel.right - 2;
		}
}


static void
TRACKBAR_QueueRefresh (WND *wndPtr)

{
 TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

 TRACE (trackbar,"queued\n");
 if (infoPtr->flags & TB_REFRESH_TIMER_SET) {
    KillTimer32 (wndPtr->hwndSelf, TB_REFRESH_TIMER);
 }

 SetTimer32 (wndPtr->hwndSelf, TB_REFRESH_TIMER, TB_REFRESH_DELAY, 0);
 infoPtr->flags|=TB_REFRESH_TIMER_SET;
}





/* Trackbar drawing code. I like my spaghetti done milanese.  */

/* ticPos is in tic-units, not in pixels */

static VOID
TRACKBAR_DrawHorizTic (TRACKBAR_INFO *infoPtr, HDC32 hdc, LONG ticPos, 
		int flags, COLORREF clrTic)

{
  RECT32 rcChannel=infoPtr->rcChannel;
  int x,y,width,range,side;

  range=infoPtr->nRangeMax - infoPtr->nRangeMin;
  width=rcChannel.right - rcChannel.left;

  if (flags & TBS_TOP) {
	y=rcChannel.top-2;
	side=-1;
  } else {
  	y=rcChannel.bottom+2;
	side=1;
  }

  if (flags & TIC_SELECTIONMARK) {
  	if (flags & TIC_SELECTIONMARKMIN) 
		x=rcChannel.left + (width*ticPos)/range - 1;
	else 
		x=rcChannel.left + (width*ticPos)/range + 1;

   	SetPixel32 (hdc, x,y+6*side, clrTic);
   	SetPixel32 (hdc, x,y+7*side, clrTic);
	return;
  }

  if ((ticPos>infoPtr->nRangeMin) && (ticPos<infoPtr->nRangeMax)) {
   	x=rcChannel.left + (width*ticPos)/range;
   	SetPixel32 (hdc, x,y+5*side, clrTic);
   	SetPixel32 (hdc, x,y+6*side, clrTic);
   	SetPixel32 (hdc, x,y+7*side, clrTic);
	}

  if (flags & TIC_EDGE) {
	if (flags & TIC_LEFTEDGE)
   		x=rcChannel.left;
	else 
   		x=rcChannel.right;

   	SetPixel32 (hdc, x,y+5*side, clrTic);
   	SetPixel32 (hdc, x,y+6*side, clrTic);
   	SetPixel32 (hdc, x,y+7*side, clrTic);
	SetPixel32 (hdc, x,y+8*side, clrTic);
  }

}

static VOID
TRACKBAR_DrawVertTic (TRACKBAR_INFO *infoPtr, HDC32 hdc, LONG ticPos, 
		int flags, COLORREF clrTic)

{
  RECT32 rcChannel=infoPtr->rcChannel;
  int x,y,width,range,side;

  range=infoPtr->nRangeMax - infoPtr->nRangeMin;
  width=rcChannel.bottom - rcChannel.top;

  if (flags & TBS_TOP) {
	x=rcChannel.right-2;
	side=-1;
  } else {
  	x=rcChannel.left+2;
	side=1;
  }


  if (flags & TIC_SELECTIONMARK) {
  	if (flags & TIC_SELECTIONMARKMIN) 
		y=rcChannel.top + (width*ticPos)/range - 1;
	else 
		y=rcChannel.top + (width*ticPos)/range + 1;

   	SetPixel32 (hdc, x+6*side, y, clrTic);
   	SetPixel32 (hdc, x+7*side, y, clrTic);
	return;
  }

  if ((ticPos>infoPtr->nRangeMin) && (ticPos<infoPtr->nRangeMax)) {
   	y=rcChannel.top + (width*ticPos)/range;
   	SetPixel32 (hdc, x+5*side, y, clrTic);
   	SetPixel32 (hdc, x+6*side, y, clrTic);
   	SetPixel32 (hdc, x+7*side, y, clrTic);
	}

  if (flags & TIC_EDGE) {
	if (flags & TIC_LEFTEDGE)
   		y=rcChannel.top;
	else 
   		y=rcChannel.bottom;

   	SetPixel32 (hdc, x+5*side, y, clrTic);
   	SetPixel32 (hdc, x+6*side, y, clrTic);
   	SetPixel32 (hdc, x+7*side, y, clrTic);
	SetPixel32 (hdc, x+8*side, y, clrTic);
  }

}


static VOID
TRACKBAR_DrawTics (TRACKBAR_INFO *infoPtr, HDC32 hdc, LONG ticPos, 
		int flags, COLORREF clrTic)

{

 if (flags & TBS_VERT) {
		if ((flags & TBS_TOP) || (flags & TBS_BOTH)) 
			TRACKBAR_DrawVertTic (infoPtr, hdc, ticPos, 
										flags | TBS_TOP , clrTic);
		if (!(flags & TBS_TOP) || (flags & TBS_BOTH)) 
			TRACKBAR_DrawVertTic (infoPtr, hdc, ticPos, flags, clrTic);
 		return;
 }

 if ((flags & TBS_TOP) || (flags & TBS_BOTH)) 
		TRACKBAR_DrawHorizTic (infoPtr, hdc, ticPos, flags | TBS_TOP , clrTic);

 if (!(flags & TBS_TOP) || (flags & TBS_BOTH)) 
		TRACKBAR_DrawHorizTic (infoPtr, hdc, ticPos, flags, clrTic);

}


static VOID
TRACKBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
	TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	RECT32 rcClient, rcChannel, rcSelection;
	HBRUSH32 hBrush = CreateSolidBrush32 (infoPtr->clrBk);
	int i;

    GetClientRect32 (wndPtr->hwndSelf, &rcClient);
	hBrush = CreateSolidBrush32 (infoPtr->clrBk);
	FillRect32 (hdc, &rcClient, hBrush);
    DeleteObject32 (hBrush);

    if (infoPtr->flags & TB_REFRESH_TIMER_SET) {
        KillTimer32 (wndPtr->hwndSelf, TB_REFRESH_TIMER);
        infoPtr->flags &= ~TB_REFRESH_TIMER_SET;
    }

	if (infoPtr->flags & TB_DRAGPOSVALID)  {
			infoPtr->nPos=infoPtr->dragPos;
			infoPtr->flags |= TB_THUMBPOSCHANGED;
	}
	
	if (infoPtr->flags & TB_THUMBCHANGED) {
		TRACKBAR_CalcThumb	(wndPtr, infoPtr);
		if (infoPtr->flags & TB_THUMBSIZECHANGED) 
			TRACKBAR_CalcChannel (wndPtr, infoPtr);
	}
	if (infoPtr->flags & TB_SELECTIONCHANGED)
		TRACKBAR_CalcSelection (wndPtr, infoPtr);
	infoPtr->flags &= ~ (TB_THUMBCHANGED | TB_SELECTIONCHANGED | TB_DRAGPOSVALID);

    /* draw channel */

    rcChannel = infoPtr->rcChannel;
    rcSelection= infoPtr->rcSelection;
    DrawEdge32 (hdc, &rcChannel, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    if (wndPtr->dwStyle & TBS_ENABLESELRANGE) {		 /* fill the channel */
		HBRUSH32 hbr = CreateSolidBrush32 (RGB(255,255,255));
		FillRect32 (hdc, &rcChannel, hbr);
		if (((wndPtr->dwStyle & TBS_VERT) && 
		    (rcSelection.left!=rcSelection.right)) || 
		    ((!(wndPtr->dwStyle & TBS_VERT)) && 	
		    (rcSelection.left!=rcSelection.right))) {
				hbr=CreateSolidBrush32 (COLOR_HIGHLIGHT); 
				FillRect32 (hdc, &rcSelection, hbr);
		}
		DeleteObject32 (hbr);
    }


    /* draw tics */

    if (!(wndPtr->dwStyle & TBS_NOTICKS)) {
		int ticFlags=wndPtr->dwStyle & 0x0f;
		COLORREF clrTic=GetSysColor32 (COLOR_3DDKSHADOW);

        for (i=0; i<infoPtr->uNumTics; i++) 
			TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->tics[i], 
									ticFlags, clrTic);

    	TRACKBAR_DrawTics (infoPtr, hdc, 0, ticFlags | TIC_LEFTEDGE, clrTic);
    	TRACKBAR_DrawTics (infoPtr, hdc, 0, ticFlags | TIC_RIGHTEDGE, clrTic);
          
		if ((wndPtr->dwStyle & TBS_ENABLESELRANGE) && 
			(rcSelection.left!=rcSelection.right)) {
			TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->nSelMin, 
								ticFlags | TIC_SELECTIONMARKMIN, clrTic);
			TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->nSelMax, 
								ticFlags | TIC_SELECTIONMARKMAX, clrTic);
		}
    }

     /* draw thumb */

     if (!(wndPtr->dwStyle & TBS_NOTHUMB)) {
		
        HBRUSH32 hbr = CreateSolidBrush32 (COLOR_BACKGROUND);
		RECT32 thumb = infoPtr->rcThumb;

		SelectObject32 (hdc, hbr);
		
		if (wndPtr->dwStyle & TBS_BOTH) {
        	FillRect32 (hdc, &thumb, hbr);
  			DrawEdge32 (hdc, &thumb, EDGE_RAISED, BF_TOPLEFT);
		} else {

		POINT32 points[6];

 			/* first, fill the thumb */
			/* FIXME: revamp. check for TBS_VERT */

		SetPolyFillMode32 (hdc,WINDING);
		points[0].x=thumb.left;
		points[0].y=thumb.top;
		points[1].x=thumb.right - 1;
		points[1].y=thumb.top;
		points[2].x=thumb.right - 1;
		points[2].y=thumb.bottom -2;
		points[3].x=(thumb.right + thumb.left-1)/2;
		points[3].y=thumb.bottom+4;
		points[4].x=thumb.left;
		points[4].y=thumb.bottom -2;
		points[5].x=points[0].x;
		points[5].y=points[0].y;
		Polygon32 (hdc, points, 6);

		if (wndPtr->dwStyle & TBS_VERT) {
                    /*   draw edge  */
		} else {
			RECT32 triangle;	/* for correct shadows of thumb */
			DrawEdge32 (hdc, &thumb, EDGE_RAISED, BF_TOPLEFT);

			/* draw notch */

			triangle.right = thumb.right+5;
			triangle.left  = points[3].x+5;
			triangle.top   = thumb.bottom +5;
			triangle.bottom= thumb.bottom +1;
			DrawEdge32 (hdc, &triangle, EDGE_SUNKEN, 
							BF_DIAGONAL | BF_TOP | BF_RIGHT);
			triangle.left  = thumb.left+6;
			triangle.right = points[3].x+6;
			DrawEdge32 (hdc, &triangle, EDGE_RAISED, 
							BF_DIAGONAL | BF_TOP | BF_LEFT);
			}
		}
		DeleteObject32 (hbr);
     }

    if (infoPtr->bFocus)
		DrawFocusRect32 (hdc, &rcClient);
}




static VOID
TRACKBAR_AlignBuddies (WND *wndPtr, TRACKBAR_INFO *infoPtr)
{
    HWND32 hwndParent = GetParent32 (wndPtr->hwndSelf);
    RECT32 rcSelf, rcBuddy;
    INT32 x, y;

    GetWindowRect32 (wndPtr->hwndSelf, &rcSelf);
    MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcSelf, 2);

    /* align buddy left or above */
    if (infoPtr->hwndBuddyLA) {
	GetWindowRect32 (infoPtr->hwndBuddyLA, &rcBuddy);
	MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcBuddy, 2);

	if (wndPtr->dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.top - (rcBuddy.bottom - rcBuddy.top);
	}
	else {
	    x = rcSelf.left - (rcBuddy.right - rcBuddy.left);
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}

	SetWindowPos32 (infoPtr->hwndBuddyLA, 0, x, y, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
    }


    /* align buddy right or below */
    if (infoPtr->hwndBuddyRB) {
	GetWindowRect32 (infoPtr->hwndBuddyRB, &rcBuddy);
	MapWindowPoints32 (HWND_DESKTOP, hwndParent, (LPPOINT32)&rcBuddy, 2);

	if (wndPtr->dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.bottom;
	}
	else {
	    x = rcSelf.right;
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}
	SetWindowPos32 (infoPtr->hwndBuddyRB, 0, x, y, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
    }
}


static LRESULT
TRACKBAR_ClearSel (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nSelMin = 0;
    infoPtr->nSelMax = 0;
	infoPtr->flags |=TB_SELECTIONCHANGED;

    if ((BOOL32)wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_ClearTics (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (infoPtr->tics) {
		COMCTL32_Free (infoPtr->tics);
		infoPtr->tics = NULL;
		infoPtr->uNumTics = 0;
    }

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_GetBuddy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wParam)		 /* buddy is left or above */
		return (LRESULT)infoPtr->hwndBuddyLA;

    /* buddy is right or below */
    return (LRESULT) infoPtr->hwndBuddyRB;
}


static LRESULT
TRACKBAR_GetChannelRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    LPRECT32 lprc = (LPRECT32)lParam;

    if (lprc == NULL)
		return 0;

    lprc->left   = infoPtr->rcChannel.left;
    lprc->right  = infoPtr->rcChannel.right;
    lprc->bottom = infoPtr->rcChannel.bottom;
    lprc->top    = infoPtr->rcChannel.top;

    return 0;
}


static LRESULT
TRACKBAR_GetLineSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nLineSize;
}


static LRESULT
TRACKBAR_GetNumTics (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBS_NOTICKS)
		return 0;

    return infoPtr->uNumTics+2;
}


static LRESULT
TRACKBAR_GetPageSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nPageSize;
}


static LRESULT
TRACKBAR_GetPos (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nPos;
}




static LRESULT
TRACKBAR_GetRangeMax (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nRangeMax;
}


static LRESULT
TRACKBAR_GetRangeMin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nRangeMin;
}


static LRESULT
TRACKBAR_GetSelEnd (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nSelMax;
}


static LRESULT
TRACKBAR_GetSelStart (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->nSelMin;
}


static LRESULT
TRACKBAR_GetThumbLength (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    return infoPtr->uThumbLen;
}

static LRESULT
TRACKBAR_GetPTics (WND *wndPtr)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    
   return (LRESULT) infoPtr->tics;
}

static LRESULT
TRACKBAR_GetThumbRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    LPRECT32 lprc = (LPRECT32)lParam;
    
    if (lprc == NULL)
   		return 0; 
   
    lprc->left   = infoPtr->rcThumb.left;
    lprc->right  = infoPtr->rcThumb.right;
    lprc->bottom = infoPtr->rcThumb.bottom;
    lprc->top    = infoPtr->rcThumb.top;
   
    return 0;
}  





static LRESULT
TRACKBAR_GetTic (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
 TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
 INT32 iTic;

 iTic=(INT32) wParam;
 if ((iTic<0) || (iTic>infoPtr->uNumTics)) 
	return -1;

 return (LRESULT) infoPtr->tics[iTic];

}


static LRESULT
TRACKBAR_GetTicPos (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)

{
 TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
 INT32 iTic, range, width, pos;
 

 iTic=(INT32 ) wParam;
 if ((iTic<0) || (iTic>infoPtr->uNumTics)) 
	return -1;

 range=infoPtr->nRangeMax - infoPtr->nRangeMin;
 width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;
 pos=infoPtr->rcChannel.left + (width * infoPtr->tics[iTic]) / range;


 return (LRESULT) pos;
}

static LRESULT
TRACKBAR_GetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (wndPtr->dwStyle & TBS_TOOLTIPS)
		return (LRESULT)infoPtr->hwndToolTip;
    return 0;
}


/*	case TBM_GETUNICODEFORMAT: */


static LRESULT
TRACKBAR_SetBuddy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    HWND32 hwndTemp;

    if (wParam) {
	/* buddy is left or above */
	hwndTemp = infoPtr->hwndBuddyLA;
	infoPtr->hwndBuddyLA = (HWND32)lParam;

	FIXME (trackbar, "move buddy!\n");
    }
    else {
		/* buddy is right or below */
		hwndTemp = infoPtr->hwndBuddyRB;
		infoPtr->hwndBuddyRB = (HWND32)lParam;

		FIXME (trackbar, "move buddy!\n");
    }

    TRACKBAR_AlignBuddies (wndPtr, infoPtr);

    return (LRESULT)hwndTemp;
}


static LRESULT
TRACKBAR_SetLineSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nLineSize;

    infoPtr->nLineSize = (INT32)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPageSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nTemp = infoPtr->nPageSize;

    infoPtr->nPageSize = (INT32)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPos (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nPos = (INT32)HIWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin)
	infoPtr->nPos = infoPtr->nRangeMin;

    if (infoPtr->nPos > infoPtr->nRangeMax)
	infoPtr->nPos = infoPtr->nRangeMax;
	infoPtr->flags |=TB_THUMBPOSCHANGED;

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRange (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    infoPtr->nRangeMin = (INT32)LOWORD(lParam);
    infoPtr->nRangeMax = (INT32)HIWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin) {
		infoPtr->nPos = infoPtr->nRangeMin;
		infoPtr->flags |=TB_THUMBPOSCHANGED;
	}

    if (infoPtr->nPos > infoPtr->nRangeMax) {
		infoPtr->nPos = infoPtr->nRangeMax;
		infoPtr->flags |=TB_THUMBPOSCHANGED;
	}

	infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
	TRACKBAR_RecalculateTics (infoPtr);

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMax (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nRangeMax = (INT32)lParam;
    if (infoPtr->nPos > infoPtr->nRangeMax) {
		infoPtr->nPos = infoPtr->nRangeMax;
		infoPtr->flags |=TB_THUMBPOSCHANGED;
	}

	infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
	TRACKBAR_RecalculateTics (infoPtr);

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMin (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nRangeMin = (INT32)lParam;
    if (infoPtr->nPos < infoPtr->nRangeMin) {
		infoPtr->nPos = infoPtr->nRangeMin;
		infoPtr->flags |=TB_THUMBPOSCHANGED;
	}

	infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
	TRACKBAR_RecalculateTics (infoPtr);

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}

static LRESULT
TRACKBAR_SetTicFreq (WND *wndPtr, WPARAM32 wParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	HDC32 hdc;
	
    if (wndPtr->dwStyle & TBS_AUTOTICKS) 
           	infoPtr->uTicFreq=(UINT32) wParam; 
	
	TRACKBAR_RecalculateTics (infoPtr);

	hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);
	return 0;
}   


static LRESULT
TRACKBAR_SetSel (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->nSelMin = (INT32)LOWORD(lParam);
    infoPtr->nSelMax = (INT32)HIWORD(lParam);
	infoPtr->flags |=TB_SELECTIONCHANGED;

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
		return 0;

    if (infoPtr->nSelMin < infoPtr->nRangeMin)
		infoPtr->nSelMin = infoPtr->nRangeMin;
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
		infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetSelEnd (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMax = (INT32)lParam;
	infoPtr->flags  |=TB_SELECTIONCHANGED;
	
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
		infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetSelStart (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    if (!wndPtr->dwStyle & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMin = (INT32)lParam;
	infoPtr->flags  |=TB_SELECTIONCHANGED;
    if (infoPtr->nSelMin < infoPtr->nRangeMin)
		infoPtr->nSelMin = infoPtr->nRangeMin;

    if (wParam) {
		HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
		TRACKBAR_Refresh (wndPtr, hdc);
		ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetThumbLength (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	HDC32 hdc;

    if (wndPtr->dwStyle & TBS_FIXEDLENGTH)
		infoPtr->uThumbLen = (UINT32)wParam;

	hdc = GetDC32 (wndPtr->hwndSelf);
	infoPtr->flags |=TB_THUMBSIZECHANGED;
	TRACKBAR_Refresh (wndPtr, hdc);
	ReleaseDC32 (wndPtr->hwndSelf, hdc);
	
    return 0;
}


static LRESULT
TRACKBAR_SetTic (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 nPos = (INT32)lParam;
	HDC32 hdc;

    if ((nPos < infoPtr->nRangeMin) || (nPos> infoPtr->nRangeMax))
		return FALSE;

	infoPtr->uNumTics++;
    infoPtr->tics=COMCTL32_ReAlloc( infoPtr->tics,
                           (infoPtr->uNumTics)*sizeof (DWORD));
    infoPtr->tics[infoPtr->uNumTics-1]=nPos;

 	hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}




static LRESULT
TRACKBAR_SetTipSide (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    INT32 fTemp = infoPtr->fLocation;

    infoPtr->fLocation = (INT32)wParam;
	
    return fTemp;
}


static LRESULT
TRACKBAR_SetToolTips (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->hwndToolTip = (HWND32)wParam;

    return 0;
}


/*	case TBM_SETUNICODEFORMAT: */


static LRESULT
TRACKBAR_InitializeThumb (WND *wndPtr)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    infoPtr->uThumbLen = 23;   /* initial thumb length */

	TRACKBAR_CalcChannel (wndPtr,infoPtr);
	TRACKBAR_CalcThumb (wndPtr, infoPtr);
	infoPtr->flags &= ~TB_SELECTIONCHANGED;

	return 0;
}


static LRESULT
TRACKBAR_Create (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr;

    infoPtr = (TRACKBAR_INFO *)COMCTL32_Alloc (sizeof(TRACKBAR_INFO));
    wndPtr->wExtra[0] = (DWORD)infoPtr;


    infoPtr->nRangeMin = 0;			 /* default values */
    infoPtr->nRangeMax = 100;
    infoPtr->nLineSize = 1;
    infoPtr->nPageSize = 20;
    infoPtr->nSelMin   = 0;
    infoPtr->nSelMax   = 0;
    infoPtr->nPos      = 0;

    infoPtr->uNumTics  = 0;    /* start and end tic are not included in count*/
	infoPtr->uTicFreq  = 1;
	infoPtr->tics	   = NULL;
	infoPtr->clrBk	   = GetSysColor32 (COLOR_BACKGROUND);
	infoPtr->hwndNotify = GetParent32 (wndPtr->hwndSelf);

	TRACKBAR_InitializeThumb (wndPtr);

	 if (wndPtr->dwStyle & TBS_TOOLTIPS) { /* Create tooltip control */
		TTTOOLINFO32A ti;

    	infoPtr->hwndToolTip =
        	CreateWindowEx32A (0, TOOLTIPS_CLASS32A, NULL, 0,
                   CW_USEDEFAULT32, CW_USEDEFAULT32,
                   CW_USEDEFAULT32, CW_USEDEFAULT32,
                   wndPtr->hwndSelf, 0, 0, 0);

    /* Send NM_TOOLTIPSCREATED notification */
    	if (infoPtr->hwndToolTip) {
        	NMTOOLTIPSCREATED nmttc;

        	nmttc.hdr.hwndFrom = wndPtr->hwndSelf;
        	nmttc.hdr.idFrom = wndPtr->wIDmenu;
        	nmttc.hdr.code = NM_TOOLTIPSCREATED;
        	nmttc.hwndToolTips = infoPtr->hwndToolTip;

        	SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
                (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmttc);
    	}

		ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	 	ti.cbSize   = sizeof(TTTOOLINFO32A);
     	ti.uFlags   = TTF_IDISHWND | TTF_TRACK;
     	ti.hwnd     = wndPtr->hwndSelf;
        ti.uId      = 0;
        ti.lpszText = "Test"; /* LPSTR_TEXTCALLBACK */
        SetRectEmpty32 (&ti.rect);

        SendMessage32A (infoPtr->hwndToolTip, TTM_ADDTOOL32A, 0, (LPARAM)&ti);

	}
    return 0;
}


static LRESULT
TRACKBAR_Destroy (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

	if (infoPtr->flags & TB_REFRESH_TIMER_SET) 
        KillTimer32 (wndPtr->hwndSelf, TB_REFRESH_TIMER);

	 /* delete tooltip control */
    if (infoPtr->hwndToolTip)
    	DestroyWindow32 (infoPtr->hwndToolTip);

    COMCTL32_Free (infoPtr);
    return 0;
}


static LRESULT
TRACKBAR_KillFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

	TRACE (trackbar,"\n");

    infoPtr->bFocus = FALSE;
	infoPtr->flags &= ~TB_DRAG_MODE;
    TRACKBAR_QueueRefresh (wndPtr);
    InvalidateRect32 (wndPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
TRACKBAR_LButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	int clickPlace,prevPos,clickPos,vertical;
	
    SetFocus32 (wndPtr->hwndSelf);

	vertical=wndPtr->dwStyle & TBS_VERT;
	if (vertical)
		clickPlace=(INT32)HIWORD(lParam);
	else
		clickPlace=(INT32)LOWORD(lParam);

	if ((vertical && 
	     (clickPlace>infoPtr->rcThumb.top) && 
		 (clickPlace<infoPtr->rcThumb.bottom))   ||
	   (!vertical && 
	   (clickPlace>infoPtr->rcThumb.left) && 
		(clickPlace<infoPtr->rcThumb.right))) {
		infoPtr->flags |= TB_DRAG_MODE;
	 	if (wndPtr->dwStyle & TBS_TOOLTIPS) {  /* enable tooltip */
        	TTTOOLINFO32A ti;
        	POINT32 pt;

        	GetCursorPos32 (&pt);
        	SendMessage32A (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 0,
                             (LPARAM)MAKELPARAM(pt.x, pt.y));

        	ti.cbSize   = sizeof(TTTOOLINFO32A);
        	ti.uId      = 0;
        	ti.hwnd     = (UINT32)wndPtr->hwndSelf;

        	infoPtr->flags |= TB_SHOW_TOOLTIP;
        	SetCapture32 (wndPtr->hwndSelf);
        	SendMessage32A (infoPtr->hwndToolTip, TTM_TRACKACTIVATE, 
						(WPARAM32)TRUE, (LPARAM)&ti);
	 	}
		return 0;
	}

	clickPos=TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace, vertical);
	prevPos	= infoPtr->nPos;

	if (clickPos > prevPos) {					/* similar to VK_NEXT */
		infoPtr->nPos += infoPtr->nPageSize;
        if (infoPtr->nPos > infoPtr->nRangeMax)
			infoPtr->nPos = infoPtr->nRangeMax;
		TRACKBAR_SendNotify (wndPtr, TB_PAGEUP);  
	} else {
        infoPtr->nPos -= infoPtr->nPageSize;	/* similar to VK_PRIOR */
        if (infoPtr->nPos < infoPtr->nRangeMin)
            infoPtr->nPos = infoPtr->nRangeMin;
        TRACKBAR_SendNotify (wndPtr, TB_PAGEDOWN);
	}
	
	if (prevPos!=infoPtr->nPos) {
		infoPtr->flags |=TB_THUMBPOSCHANGED;
    	TRACKBAR_QueueRefresh (wndPtr);
 	}
	
    return 0;
}

static LRESULT
TRACKBAR_LButtonUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

 	TRACKBAR_QueueRefresh (wndPtr);
	TRACKBAR_SendNotify (wndPtr, TB_ENDTRACK);

	infoPtr->flags &= ~TB_DRAG_MODE;

    if (wndPtr->dwStyle & TBS_TOOLTIPS) {  /* disable tooltip */
    	TTTOOLINFO32A ti;

        ti.cbSize   = sizeof(TTTOOLINFO32A);
        ti.uId      = 0;
        ti.hwnd     = (UINT32)wndPtr->hwndSelf;

        infoPtr->flags &= ~TB_SHOW_TOOLTIP;
        ReleaseCapture ();
        SendMessage32A (infoPtr->hwndToolTip, TTM_TRACKACTIVATE,
                         (WPARAM32)FALSE, (LPARAM)&ti);
	}

    return 0;
}

static LRESULT
TRACKBAR_CaptureChanged (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	
	if (infoPtr->flags & TB_DRAGPOSVALID) {
		infoPtr->nPos=infoPtr->dragPos;
		TRACKBAR_QueueRefresh (wndPtr);
	}
	
	infoPtr->flags &= ~ TB_DRAGPOSVALID;

	TRACKBAR_SendNotify (wndPtr, TB_ENDTRACK);
	return 0;
}

static LRESULT
TRACKBAR_Paint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    TRACKBAR_Refresh (wndPtr, hdc);
    if(!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
TRACKBAR_SetFocus (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
    HDC32 hdc;

	TRACE (trackbar,"\n");
    infoPtr->bFocus = TRUE;

    hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
TRACKBAR_Size (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);

    TRACKBAR_CalcChannel (wndPtr, infoPtr);
    TRACKBAR_AlignBuddies (wndPtr, infoPtr);

    return 0;
}



static BOOL32
TRACKBAR_SendNotify (WND *wndPtr, UINT32 code)

{
    TRACE (trackbar, "%x\n",code);
	if (wndPtr->dwStyle & TBS_VERT) 
    	return (BOOL32) SendMessage32A (GetParent32 (wndPtr->hwndSelf), 
						WM_VSCROLL, (WPARAM32)code, (LPARAM) wndPtr->hwndSelf);

   	return (BOOL32) SendMessage32A (GetParent32 (wndPtr->hwndSelf), 
						WM_HSCROLL, (WPARAM32)code, (LPARAM) wndPtr->hwndSelf);
	return 0;
}

static LRESULT
TRACKBAR_MouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	INT32 clickPlace,vertical;
	HDC32 hdc;
	char buf[80];
			
    TRACE (trackbar, "%x\n",wParam);

	vertical=wndPtr->dwStyle & TBS_VERT;
	if (vertical)
		clickPlace=(INT32)HIWORD(lParam);
	else
		clickPlace=(INT32)LOWORD(lParam);

	if (!(infoPtr->flags & TB_DRAG_MODE)) return TRUE;

	infoPtr->dragPos=TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace, vertical);
	infoPtr->flags|= TB_DRAGPOSVALID;
	TRACKBAR_SendNotify (wndPtr, TB_THUMBTRACK | (infoPtr->nPos>>16));

    if (infoPtr->flags & TB_SHOW_TOOLTIP) {
		POINT32 pt;
    	TTTOOLINFO32A ti;
	
    	ti.cbSize = sizeof(TTTOOLINFO32A);
    	ti.hwnd = wndPtr->hwndSelf;
    	ti.uId = 0;
		ti.hinst=0;
		sprintf (buf,"%d",infoPtr->nPos);
    	ti.lpszText = (LPSTR) buf;
		GetCursorPos32 (&pt);
		
		if (vertical) {
        	SendMessage32A (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 
							0, (LPARAM)MAKELPARAM(pt.x+5, pt.y+15));
		} else {
        	SendMessage32A (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 
							0, (LPARAM)MAKELPARAM(pt.x+15, pt.y+5));
		}
    	SendMessage32A (infoPtr->hwndToolTip, TTM_UPDATETIPTEXT32A,
            0, (LPARAM)&ti);
	}

    hdc = GetDC32 (wndPtr->hwndSelf);
    TRACKBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

 	return TRUE;
}


static LRESULT
TRACKBAR_KeyDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
	INT32 pos;

    TRACE (trackbar, "%x\n",wParam);

	pos=infoPtr->nPos;
	switch (wParam) {
		case VK_LEFT:
		case VK_UP: 
			if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
			infoPtr->nPos -= infoPtr->nLineSize;
			if (infoPtr->nPos < infoPtr->nRangeMin) 
				infoPtr->nPos = infoPtr->nRangeMin;
			TRACKBAR_SendNotify (wndPtr, TB_LINEUP);
			break;
		case VK_RIGHT:
		case VK_DOWN: 
			if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
			infoPtr->nPos += infoPtr->nLineSize;
			if (infoPtr->nPos > infoPtr->nRangeMax) 
				infoPtr->nPos = infoPtr->nRangeMax;
			TRACKBAR_SendNotify (wndPtr, TB_LINEDOWN);
            break;
		case VK_NEXT:
			if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
			infoPtr->nPos += infoPtr->nPageSize;
			if (infoPtr->nPos > infoPtr->nRangeMax) 
				infoPtr->nPos = infoPtr->nRangeMax;
			 TRACKBAR_SendNotify (wndPtr, TB_PAGEUP);
            break;
		case VK_PRIOR:
			if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
			infoPtr->nPos -= infoPtr->nPageSize;
			if (infoPtr->nPos < infoPtr->nRangeMin) 
				infoPtr->nPos = infoPtr->nRangeMin;
			TRACKBAR_SendNotify (wndPtr, TB_PAGEDOWN);
            break;
		case VK_HOME: 
			if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
			infoPtr->nPos = infoPtr->nRangeMin;
			TRACKBAR_SendNotify (wndPtr, TB_TOP);
            break;
		case VK_END: 
			if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
			infoPtr->nPos = infoPtr->nRangeMax;
	 		TRACKBAR_SendNotify (wndPtr, TB_BOTTOM);
            break;
	}

 if (pos!=infoPtr->nPos) { 
	infoPtr->flags |=TB_THUMBPOSCHANGED;
	TRACKBAR_QueueRefresh (wndPtr);
 }

 return TRUE;
}

static LRESULT
TRACKBAR_KeyUp (WND *wndPtr, WPARAM32 wParam)
{
	switch (wParam) {
		case VK_LEFT:
		case VK_UP: 
		case VK_RIGHT:
		case VK_DOWN: 
		case VK_NEXT:
		case VK_PRIOR:
		case VK_HOME: 
		case VK_END: 	TRACKBAR_SendNotify (wndPtr, TB_ENDTRACK);
	}
 return TRUE;
}


static LRESULT 
TRACKBAR_HandleTimer ( WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
 TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr(wndPtr);
 HDC32 hdc;

 TRACE (trackbar,"timer\n");

 switch (wParam) {
	case TB_REFRESH_TIMER: 
		KillTimer32 (wndPtr->hwndSelf, TB_REFRESH_TIMER );
		if (infoPtr->flags & TB_DRAGPOSVALID)  {
			infoPtr->nPos=infoPtr->dragPos;
			infoPtr->flags |= TB_THUMBPOSCHANGED;
		}
		infoPtr->flags &= ~ (TB_REFRESH_TIMER_SET | TB_DRAGPOSVALID);
        hdc=GetDC32 (wndPtr->hwndSelf);
        TRACKBAR_Refresh (wndPtr, hdc);
        ReleaseDC32 (wndPtr->hwndSelf, hdc);
		return 0;
	}
 return 1;
}


LRESULT WINAPI
TRACKBAR_WindowProc (HWND32 hwnd, UINT32 uMsg, WPARAM32 wParam, LPARAM lParam)
{
     WND *wndPtr = WIN_FindWndPtr(hwnd);

/*	TRACE (trackbar, "msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam); */

    switch (uMsg)
    {
	case TBM_CLEARSEL:
	    return TRACKBAR_ClearSel (wndPtr, wParam, lParam);

	case TBM_CLEARTICS:
	    return TRACKBAR_ClearTics (wndPtr, wParam, lParam);

	case TBM_GETBUDDY:
	    return TRACKBAR_GetBuddy (wndPtr, wParam, lParam);

	case TBM_GETCHANNELRECT:
	    return TRACKBAR_GetChannelRect (wndPtr, wParam, lParam);

	case TBM_GETLINESIZE:
	    return TRACKBAR_GetLineSize (wndPtr, wParam, lParam);

	case TBM_GETNUMTICS:
	    return TRACKBAR_GetNumTics (wndPtr, wParam, lParam);

	case TBM_GETPAGESIZE:
	    return TRACKBAR_GetPageSize (wndPtr, wParam, lParam);

	case TBM_GETPOS:
	    return TRACKBAR_GetPos (wndPtr, wParam, lParam);

	case TBM_GETPTICS:
		 return TRACKBAR_GetPTics (wndPtr);

	case TBM_GETRANGEMAX:
	    return TRACKBAR_GetRangeMax (wndPtr, wParam, lParam);

	case TBM_GETRANGEMIN:
	    return TRACKBAR_GetRangeMin (wndPtr, wParam, lParam);

	case TBM_GETSELEND:
	    return TRACKBAR_GetSelEnd (wndPtr, wParam, lParam);

	case TBM_GETSELSTART:
	    return TRACKBAR_GetSelStart (wndPtr, wParam, lParam);

	case TBM_GETTHUMBLENGTH:
	    return TRACKBAR_GetThumbLength (wndPtr, wParam, lParam);

	case TBM_GETTHUMBRECT:
        return TRACKBAR_GetThumbRect (wndPtr, wParam, lParam);

    case TBM_GETTIC:
        return TRACKBAR_GetTic (wndPtr, wParam, lParam);
 
    case TBM_GETTICPOS:
        return TRACKBAR_GetTicPos (wndPtr, wParam, lParam);
 
	case TBM_GETTOOLTIPS:
	    return TRACKBAR_GetToolTips (wndPtr, wParam, lParam);

/*	case TBM_GETUNICODEFORMAT: */

	case TBM_SETBUDDY:
	    return TRACKBAR_SetBuddy (wndPtr, wParam, lParam);

	case TBM_SETLINESIZE:
	    return TRACKBAR_SetLineSize (wndPtr, wParam, lParam);

	case TBM_SETPAGESIZE:
	    return TRACKBAR_SetPageSize (wndPtr, wParam, lParam);

	case TBM_SETPOS:
	    return TRACKBAR_SetPos (wndPtr, wParam, lParam);

	case TBM_SETRANGE:
	    return TRACKBAR_SetRange (wndPtr, wParam, lParam);

	case TBM_SETRANGEMAX:
	    return TRACKBAR_SetRangeMax (wndPtr, wParam, lParam);

	case TBM_SETRANGEMIN:
	    return TRACKBAR_SetRangeMin (wndPtr, wParam, lParam);

	case TBM_SETSEL:
	    return TRACKBAR_SetSel (wndPtr, wParam, lParam);

	case TBM_SETSELEND:
	    return TRACKBAR_SetSelEnd (wndPtr, wParam, lParam);

	case TBM_SETSELSTART:
	    return TRACKBAR_SetSelStart (wndPtr, wParam, lParam);

	case TBM_SETTHUMBLENGTH:
	    return TRACKBAR_SetThumbLength (wndPtr, wParam, lParam);

	case TBM_SETTIC:
	    return TRACKBAR_SetTic (wndPtr, wParam, lParam);

    case TBM_SETTICFREQ:
       return TRACKBAR_SetTicFreq (wndPtr, wParam);


	case TBM_SETTIPSIDE:
	    return TRACKBAR_SetTipSide (wndPtr, wParam, lParam);

	case TBM_SETTOOLTIPS:
	    return TRACKBAR_SetToolTips (wndPtr, wParam, lParam);

/*	case TBM_SETUNICODEFORMAT: */


	case WM_CAPTURECHANGED:
	    return TRACKBAR_CaptureChanged (wndPtr, wParam, lParam);

	case WM_CREATE:
	    return TRACKBAR_Create (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return TRACKBAR_Destroy (wndPtr, wParam, lParam);

/*	case WM_ENABLE: */

/*	case WM_ERASEBKGND: */
/*	    return 0; */

	case WM_GETDLGCODE:
	    return DLGC_WANTARROWS;

 	case WM_KEYDOWN:
       return TRACKBAR_KeyDown (wndPtr, wParam, lParam);
        
  	case WM_KEYUP:
       return TRACKBAR_KeyUp (wndPtr, wParam);

	case WM_KILLFOCUS:
	    return TRACKBAR_KillFocus (wndPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return TRACKBAR_LButtonDown (wndPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return TRACKBAR_LButtonUp (wndPtr, wParam, lParam);

	case WM_MOUSEMOVE:
	    return TRACKBAR_MouseMove (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return TRACKBAR_Paint (wndPtr, wParam);

	case WM_SETFOCUS:
	    return TRACKBAR_SetFocus (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return TRACKBAR_Size (wndPtr, wParam, lParam);

  	case WM_TIMER:
		return TRACKBAR_HandleTimer (wndPtr, wParam, lParam);

	case WM_WININICHANGE:
		return TRACKBAR_InitializeThumb (wndPtr);

	default:
		if (uMsg >= WM_USER)
		ERR (trackbar, "unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProc32A (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TRACKBAR_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (TRACKBAR_CLASS32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC32)TRACKBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASS32A;
 
    RegisterClass32A (&wndClass);
}


VOID
TRACKBAR_Unregister (VOID)
{
    if (GlobalFindAtom32A (TRACKBAR_CLASS32A))
	UnregisterClass32A (TRACKBAR_CLASS32A, (HINSTANCE32)NULL);
}

