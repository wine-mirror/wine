/*
 * Trackbar control
 *
 * Copyright 1998, 1999 Eric Kohl <ekohl@abo.rhein-zeitung.de>
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



#include "winbase.h"
#include "commctrl.h"
#include "trackbar.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(trackbar)


#define TRACKBAR_GetInfoPtr(wndPtr) ((TRACKBAR_INFO *)GetWindowLongA (hwnd,0))


/* Used by TRACKBAR_Refresh to find out which parts of the control 
   need to be recalculated */

#define TB_THUMBPOSCHANGED      1	
#define TB_THUMBSIZECHANGED     2
#define TB_THUMBCHANGED 	(TB_THUMBPOSCHANGED | TB_THUMBPOSCHANGED)
#define TB_SELECTIONCHANGED     4
#define TB_DRAG_MODE            16     /* we're dragging the slider */
#define TB_DRAGPOSVALID         32     /* current Position is in dragPos */
#define TB_SHOW_TOOLTIP         64     /* tooltip-style enabled and tooltip on */

/* helper defines for TRACKBAR_DrawTic */
#define TIC_LEFTEDGE            0x20
#define TIC_RIGHTEDGE           0x40
#define TIC_EDGE                (TIC_LEFTEDGE | TIC_RIGHTEDGE)
#define TIC_SELECTIONMARKMAX    0x80
#define TIC_SELECTIONMARKMIN    0x100
#define TIC_SELECTIONMARK       (TIC_SELECTIONMARKMAX | TIC_SELECTIONMARKMIN)

static BOOL TRACKBAR_SendNotify (HWND hwnd, UINT code);

void TRACKBAR_RecalculateTics (TRACKBAR_INFO *infoPtr)
{
    int i,tic,nrTics;

    if (infoPtr->uTicFreq && infoPtr->nRangeMax >= infoPtr->nRangeMin) 
    	nrTics=(infoPtr->nRangeMax - infoPtr->nRangeMin)/infoPtr->uTicFreq;
    else {
        nrTics=0;
        COMCTL32_Free (infoPtr->tics);
        infoPtr->tics=NULL;
        infoPtr->uNumTics=0;
        return;
    }

    if (nrTics!=infoPtr->uNumTics) {
    	infoPtr->tics=COMCTL32_ReAlloc (infoPtr->tics, 
                                        (nrTics+1)*sizeof (DWORD));
    	infoPtr->uNumTics=nrTics;
    }
    infoPtr->uNumTics=nrTics;
    tic=infoPtr->nRangeMin+infoPtr->uTicFreq;
    for (i=0; i<nrTics; i++,tic+=infoPtr->uTicFreq)
        infoPtr->tics[i]=tic;
}


/* converts from physical (mouse) position to logical position 
   (in range of trackbar) */

static inline DOUBLE
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
	
    if (pos > infoPtr->nRangeMax)
        pos = infoPtr->nRangeMax;
    else if (pos < infoPtr->nRangeMin)
        pos = infoPtr->nRangeMin;

    TRACE("%.2f\n",pos);
    return pos;
}


static VOID
TRACKBAR_CalcChannel (HWND hwnd, TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    INT cyChannel;
    RECT lpRect,*channel = & infoPtr->rcChannel;

    GetClientRect (hwnd, &lpRect);

    if (dwStyle & TBS_ENABLESELRANGE)
        cyChannel = MAX(infoPtr->uThumbLen - 8, 4);
    else
        cyChannel = 4;

    if (dwStyle & TBS_VERT) {
        channel->top    = lpRect.top + 8;
        channel->bottom = lpRect.bottom - 8;

	if (dwStyle & TBS_BOTH) {
            channel->left  = (lpRect.right - cyChannel) / 2;
            channel->right = (lpRect.right + cyChannel) / 2;
        }
        else if (dwStyle & TBS_LEFT) {
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
        if (dwStyle & TBS_BOTH) {
            channel->top		= (lpRect.bottom - cyChannel) / 2;
            channel->bottom 	= (lpRect.bottom + cyChannel) / 2;
        }
        else if (dwStyle & TBS_TOP) {
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
TRACKBAR_CalcThumb (HWND hwnd, TRACKBAR_INFO *infoPtr)
{
    RECT *thumb;
    int range, width;
	
    thumb=&infoPtr->rcThumb;
    range=infoPtr->nRangeMax - infoPtr->nRangeMin;
    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_VERT) {
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
TRACKBAR_CalcSelection (HWND hwnd, TRACKBAR_INFO *infoPtr)
{
    RECT *selection;
    int range, width;

    selection= & infoPtr->rcSelection;
    range=infoPtr->nRangeMax - infoPtr->nRangeMin;
    width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;

    if (range <= 0) 
        SetRectEmpty (selection);
    else 
        if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_VERT) {
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

/* Trackbar drawing code. I like my spaghetti done milanese.  */

/* ticPos is in tic-units, not in pixels */

static VOID
TRACKBAR_DrawHorizTic (TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos, 
                       int flags, COLORREF clrTic)
{
    RECT rcChannel=infoPtr->rcChannel;
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

   	SetPixel (hdc, x,y+6*side, clrTic);
   	SetPixel (hdc, x,y+7*side, clrTic);
	return;
    }

    if ((ticPos>infoPtr->nRangeMin) && (ticPos<infoPtr->nRangeMax)) {
   	x=rcChannel.left + (width*ticPos)/range;
   	SetPixel (hdc, x,y+5*side, clrTic);
   	SetPixel (hdc, x,y+6*side, clrTic);
   	SetPixel (hdc, x,y+7*side, clrTic);
    }

    if (flags & TIC_EDGE) {
	if (flags & TIC_LEFTEDGE)
            x=rcChannel.left;
	else 
            x=rcChannel.right;

   	SetPixel (hdc, x,y+5*side, clrTic);
   	SetPixel (hdc, x,y+6*side, clrTic);
   	SetPixel (hdc, x,y+7*side, clrTic);
	SetPixel (hdc, x,y+8*side, clrTic);
    }

}

static VOID
TRACKBAR_DrawVertTic (TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos, 
                      int flags, COLORREF clrTic)
{
    RECT rcChannel=infoPtr->rcChannel;
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

   	SetPixel (hdc, x+6*side, y, clrTic);
   	SetPixel (hdc, x+7*side, y, clrTic);
	return;
    }

    if ((ticPos>infoPtr->nRangeMin) && (ticPos<infoPtr->nRangeMax)) {
   	y=rcChannel.top + (width*ticPos)/range;
   	SetPixel (hdc, x+5*side, y, clrTic);
   	SetPixel (hdc, x+6*side, y, clrTic);
   	SetPixel (hdc, x+7*side, y, clrTic);
    }

    if (flags & TIC_EDGE) {
	if (flags & TIC_LEFTEDGE)
            y=rcChannel.top;
	else 
            y=rcChannel.bottom;

   	SetPixel (hdc, x+5*side, y, clrTic);
   	SetPixel (hdc, x+6*side, y, clrTic);
   	SetPixel (hdc, x+7*side, y, clrTic);
	SetPixel (hdc, x+8*side, y, clrTic);
    }

}


static VOID
TRACKBAR_DrawTics (TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos, 
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
TRACKBAR_Refresh (HWND hwnd, HDC hdc)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcClient, rcChannel, rcSelection;
    HBRUSH hBrush = CreateSolidBrush (infoPtr->clrBk);
    int i;

    GetClientRect (hwnd, &rcClient);
    hBrush = CreateSolidBrush (infoPtr->clrBk);
    FillRect (hdc, &rcClient, hBrush);
    DeleteObject (hBrush);

    if (infoPtr->flags & TB_DRAGPOSVALID)  {
        infoPtr->nPos=infoPtr->dragPos;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }
	
    if (infoPtr->flags & TB_THUMBCHANGED) {
        TRACKBAR_CalcThumb	(hwnd, infoPtr);
        if (infoPtr->flags & TB_THUMBSIZECHANGED) 
            TRACKBAR_CalcChannel (hwnd, infoPtr);
    }
    if (infoPtr->flags & TB_SELECTIONCHANGED)
        TRACKBAR_CalcSelection (hwnd, infoPtr);
    infoPtr->flags &= ~ (TB_THUMBCHANGED | TB_SELECTIONCHANGED | 
                         TB_DRAGPOSVALID);

    /* draw channel */

    rcChannel = infoPtr->rcChannel;
    rcSelection= infoPtr->rcSelection;
    DrawEdge (hdc, &rcChannel, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    if (dwStyle & TBS_ENABLESELRANGE) {		 /* fill the channel */
        HBRUSH hbr = CreateSolidBrush (RGB(255,255,255));
        FillRect (hdc, &rcChannel, hbr);
        if (((dwStyle & TBS_VERT) && 
             (rcSelection.left!=rcSelection.right)) || 
            ((!(dwStyle & TBS_VERT)) && 	
             (rcSelection.left!=rcSelection.right))) {
            hbr=CreateSolidBrush (COLOR_HIGHLIGHT); 
            FillRect (hdc, &rcSelection, hbr);
        }
        DeleteObject (hbr);
    }


    /* draw tics */

    if (!(dwStyle & TBS_NOTICKS)) {
        int ticFlags = dwStyle & 0x0f;
        COLORREF clrTic=GetSysColor (COLOR_3DDKSHADOW);

        for (i=0; i<infoPtr->uNumTics; i++) 
            TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->tics[i], 
                               ticFlags, clrTic);

    	TRACKBAR_DrawTics (infoPtr, hdc, 0, ticFlags | TIC_LEFTEDGE, clrTic);
    	TRACKBAR_DrawTics (infoPtr, hdc, 0, ticFlags | TIC_RIGHTEDGE, clrTic);
          
        if ((dwStyle & TBS_ENABLESELRANGE) && 
            (rcSelection.left!=rcSelection.right)) {
            TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->nSelMin, 
                               ticFlags | TIC_SELECTIONMARKMIN, clrTic);
            TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->nSelMax, 
                               ticFlags | TIC_SELECTIONMARKMAX, clrTic);
        }
    }

    /* draw thumb */

    if (!(dwStyle & TBS_NOTHUMB)) {
		
        HBRUSH hbr = CreateSolidBrush (COLOR_BACKGROUND);
        RECT thumb = infoPtr->rcThumb;

        SelectObject (hdc, hbr);
		
        if (dwStyle & TBS_BOTH) {
            FillRect (hdc, &thumb, hbr);
            DrawEdge (hdc, &thumb, EDGE_RAISED, BF_TOPLEFT);
        } else {

            POINT points[6];

            /* first, fill the thumb */
            /* FIXME: revamp. check for TBS_VERT */

            SetPolyFillMode (hdc,WINDING);
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
            Polygon (hdc, points, 6);

            if (dwStyle & TBS_VERT) {
                /*   draw edge  */
            } else {
                RECT triangle;	/* for correct shadows of thumb */
                DrawEdge (hdc, &thumb, EDGE_RAISED, BF_TOPLEFT);

                /* draw notch */

                triangle.right = thumb.right+5;
                triangle.left  = points[3].x+5;
                triangle.top   = thumb.bottom +5;
                triangle.bottom= thumb.bottom +1;
                DrawEdge (hdc, &triangle, EDGE_SUNKEN, 
                          BF_DIAGONAL | BF_TOP | BF_RIGHT);
                triangle.left  = thumb.left+6;
                triangle.right = points[3].x+6;
                DrawEdge (hdc, &triangle, EDGE_RAISED, 
                          BF_DIAGONAL | BF_TOP | BF_LEFT);
            }
        }
        DeleteObject (hbr);
    }

    if (infoPtr->bFocus)
        DrawFocusRect (hdc, &rcClient);
}


static VOID
TRACKBAR_AlignBuddies (HWND hwnd, TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    HWND hwndParent = GetParent (hwnd);
    RECT rcSelf, rcBuddy;
    INT x, y;

    GetWindowRect (hwnd, &rcSelf);
    MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcSelf, 2);

    /* align buddy left or above */
    if (infoPtr->hwndBuddyLA) {
	GetWindowRect (infoPtr->hwndBuddyLA, &rcBuddy);
	MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcBuddy, 2);

	if (dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.top - (rcBuddy.bottom - rcBuddy.top);
	}
	else {
	    x = rcSelf.left - (rcBuddy.right - rcBuddy.left);
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}

	SetWindowPos (infoPtr->hwndBuddyLA, 0, x, y, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE);
    }


    /* align buddy right or below */
    if (infoPtr->hwndBuddyRB) {
	GetWindowRect (infoPtr->hwndBuddyRB, &rcBuddy);
	MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcBuddy, 2);

	if (dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.bottom;
	}
	else {
	    x = rcSelf.right;
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}
	SetWindowPos (infoPtr->hwndBuddyRB, 0, x, y, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE);
    }
}


static LRESULT
TRACKBAR_ClearSel (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->nSelMin = 0;
    infoPtr->nSelMax = 0;
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if ((BOOL)wParam) 
	InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_ClearTics (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (infoPtr->tics) {
        COMCTL32_Free (infoPtr->tics);
        infoPtr->tics = NULL;
        infoPtr->uNumTics = 0;
    }

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_GetBuddy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (wParam)  /* buddy is left or above */
        return (LRESULT)infoPtr->hwndBuddyLA;

    /* buddy is right or below */
    return (LRESULT) infoPtr->hwndBuddyRB;
}


static LRESULT
TRACKBAR_GetChannelRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    LPRECT lprc = (LPRECT)lParam;

    if (lprc == NULL)
        return 0;

    lprc->left   = infoPtr->rcChannel.left;
    lprc->right  = infoPtr->rcChannel.right;
    lprc->bottom = infoPtr->rcChannel.bottom;
    lprc->top    = infoPtr->rcChannel.top;

    return 0;
}


static LRESULT
TRACKBAR_GetLineSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nLineSize;
}


static LRESULT
TRACKBAR_GetNumTics (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_NOTICKS)
        return 0;

    return infoPtr->uNumTics+2;
}


static LRESULT
TRACKBAR_GetPageSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nPageSize;
}


static LRESULT
TRACKBAR_GetPos (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nPos;
}


static LRESULT
TRACKBAR_GetRangeMax (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nRangeMax;
}


static LRESULT
TRACKBAR_GetRangeMin (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nRangeMin;
}


static LRESULT
TRACKBAR_GetSelEnd (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nSelMax;
}


static LRESULT
TRACKBAR_GetSelStart (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->nSelMin;
}


static LRESULT
TRACKBAR_GetThumbLength (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    return infoPtr->uThumbLen;
}

static LRESULT
TRACKBAR_GetPTics (HWND hwnd)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    
    return (LRESULT) infoPtr->tics;
}

static LRESULT
TRACKBAR_GetThumbRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    LPRECT lprc = (LPRECT)lParam;
    
    if (lprc == NULL)
        return 0; 
   
    lprc->left   = infoPtr->rcThumb.left;
    lprc->right  = infoPtr->rcThumb.right;
    lprc->bottom = infoPtr->rcThumb.bottom;
    lprc->top    = infoPtr->rcThumb.top;
   
    return 0;
}  


static LRESULT
TRACKBAR_GetTic (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT iTic;

    iTic=(INT) wParam;
    if ((iTic<0) || (iTic>infoPtr->uNumTics)) 
	return -1;

    return (LRESULT) infoPtr->tics[iTic];

}


static LRESULT
TRACKBAR_GetTicPos (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT iTic, range, width, pos;
 

    iTic=(INT ) wParam;
    if ((iTic<0) || (iTic>infoPtr->uNumTics)) 
	return -1;

    range=infoPtr->nRangeMax - infoPtr->nRangeMin;
    width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;
    pos=infoPtr->rcChannel.left + (width * infoPtr->tics[iTic]) / range;


    return (LRESULT) pos;
}


static LRESULT
TRACKBAR_GetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_TOOLTIPS)
        return (LRESULT)infoPtr->hwndToolTip;
    return 0;
}


/*	case TBM_GETUNICODEFORMAT: */


static LRESULT
TRACKBAR_SetBuddy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    HWND hwndTemp;

    if (wParam) {
	/* buddy is left or above */
	hwndTemp = infoPtr->hwndBuddyLA;
	infoPtr->hwndBuddyLA = (HWND)lParam;

	FIXME("move buddy!\n");
    }
    else {
        /* buddy is right or below */
        hwndTemp = infoPtr->hwndBuddyRB;
        infoPtr->hwndBuddyRB = (HWND)lParam;

        FIXME("move buddy!\n");
    }

    TRACKBAR_AlignBuddies (hwnd, infoPtr);

    return (LRESULT)hwndTemp;
}


static LRESULT
TRACKBAR_SetLineSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nLineSize;

    infoPtr->nLineSize = (INT)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPageSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT nTemp = infoPtr->nPageSize;

    infoPtr->nPageSize = (INT)lParam;

    return nTemp;
}


static LRESULT
TRACKBAR_SetPos (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->nPos = (INT)LOWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin)
	infoPtr->nPos = infoPtr->nRangeMin;

    if (infoPtr->nPos > infoPtr->nRangeMax)
	infoPtr->nPos = infoPtr->nRangeMax;
    infoPtr->flags |= TB_THUMBPOSCHANGED;

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetRange (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    infoPtr->nRangeMin = (INT)LOWORD(lParam);
    infoPtr->nRangeMax = (INT)HIWORD(lParam);

    if (infoPtr->nPos < infoPtr->nRangeMin) {
        infoPtr->nPos = infoPtr->nRangeMin;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    if (infoPtr->nPos > infoPtr->nRangeMax) {
        infoPtr->nPos = infoPtr->nRangeMax;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
    if (infoPtr->nPageSize == 0)
        infoPtr->nPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (wParam)
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMax (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->nRangeMax = (INT)lParam;
    if (infoPtr->nPos > infoPtr->nRangeMax) {
        infoPtr->nPos = infoPtr->nRangeMax;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
    if (infoPtr->nPageSize == 0)
        infoPtr->nPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetRangeMin (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->nRangeMin = (INT)lParam;
    if (infoPtr->nPos < infoPtr->nRangeMin) {
        infoPtr->nPos = infoPtr->nRangeMin;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->nPageSize=(infoPtr->nRangeMax -  infoPtr->nRangeMin)/5;
    if (infoPtr->nPageSize == 0)
        infoPtr->nPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (wParam)
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetTicFreq (HWND hwnd, WPARAM wParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
	
    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_AUTOTICKS)
        infoPtr->uTicFreq=(UINT) wParam; 
	
    TRACKBAR_RecalculateTics (infoPtr);

    InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}   


static LRESULT
TRACKBAR_SetSel (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->nSelMin = (INT)LOWORD(lParam);
    infoPtr->nSelMax = (INT)HIWORD(lParam);
    infoPtr->flags |=TB_SELECTIONCHANGED;

    if (!GetWindowLongA (hwnd, GWL_STYLE) & TBS_ENABLESELRANGE)
        return 0;

    if (infoPtr->nSelMin < infoPtr->nRangeMin)
        infoPtr->nSelMin = infoPtr->nRangeMin;
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
        infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);


    return 0;
}


static LRESULT
TRACKBAR_SetSelEnd (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (!GetWindowLongA (hwnd, GWL_STYLE) & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMax = (INT)lParam;
    infoPtr->flags |= TB_SELECTIONCHANGED;
	
    if (infoPtr->nSelMax > infoPtr->nRangeMax)
        infoPtr->nSelMax = infoPtr->nRangeMax;

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetSelStart (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (!GetWindowLongA (hwnd, GWL_STYLE) & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->nSelMin = (INT)lParam;
    infoPtr->flags  |=TB_SELECTIONCHANGED;

    if (infoPtr->nSelMin < infoPtr->nRangeMin)
        infoPtr->nSelMin = infoPtr->nRangeMin;

    if (wParam) 
        InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetThumbLength (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_FIXEDLENGTH)
        infoPtr->uThumbLen = (UINT)wParam;

    infoPtr->flags |= TB_THUMBSIZECHANGED;

    InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_SetTic (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT nPos = (INT)lParam;

    if ((nPos < infoPtr->nRangeMin) || (nPos> infoPtr->nRangeMax))
        return FALSE;

    infoPtr->uNumTics++;
    infoPtr->tics=COMCTL32_ReAlloc( infoPtr->tics,
                                    (infoPtr->uNumTics)*sizeof (DWORD));
    infoPtr->tics[infoPtr->uNumTics-1]=nPos;

    InvalidateRect (hwnd, NULL, FALSE);

    return TRUE;
}


static LRESULT
TRACKBAR_SetTipSide (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT fTemp = infoPtr->fLocation;

    infoPtr->fLocation = (INT)wParam;
	
    return fTemp;
}


static LRESULT
TRACKBAR_SetToolTips (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->hwndToolTip = (HWND)wParam;

    return 0;
}


/*	case TBM_SETUNICODEFORMAT: */


static LRESULT
TRACKBAR_InitializeThumb (HWND hwnd)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    infoPtr->uThumbLen = 23;   /* initial thumb length */

    TRACKBAR_CalcChannel (hwnd,infoPtr);
    TRACKBAR_CalcThumb (hwnd, infoPtr);
    infoPtr->flags &= ~TB_SELECTIONCHANGED;

    return 0;
}


static LRESULT
TRACKBAR_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr;

    infoPtr = (TRACKBAR_INFO *)COMCTL32_Alloc (sizeof(TRACKBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* set default values */
    infoPtr->nRangeMin = 0;
    infoPtr->nRangeMax = 100;
    infoPtr->nLineSize = 1;
    infoPtr->nPageSize = 20;
    infoPtr->nSelMin   = 0;
    infoPtr->nSelMax   = 0;
    infoPtr->nPos      = 0;

    infoPtr->uNumTics  = 0;    /* start and end tic are not included in count*/
    infoPtr->uTicFreq  = 1;
    infoPtr->tics	   = NULL;
    infoPtr->clrBk	   = GetSysColor (COLOR_BACKGROUND);
    infoPtr->hwndNotify = GetParent (hwnd);

    TRACKBAR_InitializeThumb (hwnd);

    /* Create tooltip control */
    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_TOOLTIPS) {
        TTTOOLINFOA ti;

    	infoPtr->hwndToolTip =
            CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hwnd, 0, 0, 0);

        /* Send NM_TOOLTIPSCREATED notification */
    	if (infoPtr->hwndToolTip) {
            NMTOOLTIPSCREATED nmttc;

            nmttc.hdr.hwndFrom = hwnd;
            nmttc.hdr.idFrom   = GetWindowLongA (hwnd, GWL_ID);
            nmttc.hdr.code = NM_TOOLTIPSCREATED;
            nmttc.hwndToolTips = infoPtr->hwndToolTip;

            SendMessageA (GetParent (hwnd), WM_NOTIFY,
                          (WPARAM)nmttc.hdr.idFrom, (LPARAM)&nmttc);
    	}

        ZeroMemory (&ti, sizeof(TTTOOLINFOA));
        ti.cbSize   = sizeof(TTTOOLINFOA);
     	ti.uFlags   = TTF_IDISHWND | TTF_TRACK;
	ti.hwnd     = hwnd;
        ti.uId      = 0;
        ti.lpszText = "Test"; /* LPSTR_TEXTCALLBACK */
        SetRectEmpty (&ti.rect);

        SendMessageA (infoPtr->hwndToolTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    }

    return 0;
}


static LRESULT
TRACKBAR_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
    	DestroyWindow (infoPtr->hwndToolTip);

    COMCTL32_Free (infoPtr);
    return 0;
}


static LRESULT
TRACKBAR_KillFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    TRACE("\n");

    infoPtr->bFocus = FALSE;
    infoPtr->flags &= ~TB_DRAG_MODE;

    InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    int clickPlace,prevPos,vertical;
    DOUBLE clickPos;

    SetFocus (hwnd);

    vertical = dwStyle & TBS_VERT;
    if (vertical)
        clickPlace=(INT)HIWORD(lParam);
    else
        clickPlace=(INT)LOWORD(lParam);

    if ((vertical && 
         (clickPlace>infoPtr->rcThumb.top) && 
         (clickPlace<infoPtr->rcThumb.bottom))   ||
        (!vertical && 
         (clickPlace>infoPtr->rcThumb.left) && 
         (clickPlace<infoPtr->rcThumb.right))) {
        infoPtr->flags |= TB_DRAG_MODE;
        if (dwStyle & TBS_TOOLTIPS) {  /* enable tooltip */
            TTTOOLINFOA ti;
            POINT pt;

            GetCursorPos (&pt);
            SendMessageA (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 0,
                          (LPARAM)MAKELPARAM(pt.x, pt.y));

            ti.cbSize   = sizeof(TTTOOLINFOA);
            ti.uId      = 0;
            ti.hwnd     = (UINT)hwnd;

            infoPtr->flags |= TB_SHOW_TOOLTIP;
            SetCapture (hwnd);
            SendMessageA (infoPtr->hwndToolTip, TTM_TRACKACTIVATE, 
                          (WPARAM)TRUE, (LPARAM)&ti);
        }
        return 0;
    }

    clickPos = TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace, vertical);
    prevPos = infoPtr->nPos;

    if (clickPos > prevPos) {  /* similar to VK_NEXT */
        infoPtr->nPos += infoPtr->nPageSize;
        if (infoPtr->nPos > infoPtr->nRangeMax)
            infoPtr->nPos = infoPtr->nRangeMax;
        TRACKBAR_SendNotify (hwnd, TB_PAGEUP);  
    } else {
        infoPtr->nPos -= infoPtr->nPageSize;  /* similar to VK_PRIOR */
        if (infoPtr->nPos < infoPtr->nRangeMin)
            infoPtr->nPos = infoPtr->nRangeMin;
        TRACKBAR_SendNotify (hwnd, TB_PAGEDOWN);
    }
	
    if (prevPos!=infoPtr->nPos) {
        infoPtr->flags |= TB_THUMBPOSCHANGED;
        InvalidateRect (hwnd, NULL, FALSE);
    }
	
    return 0;
}


static LRESULT
TRACKBAR_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    TRACKBAR_SendNotify (hwnd, TB_ENDTRACK);

    if (infoPtr->flags & TB_DRAG_MODE)
    {
        infoPtr->flags &= ~TB_DRAG_MODE;
        ReleaseCapture ();
    }

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_TOOLTIPS) {  /* disable tooltip */
    	TTTOOLINFOA ti;

        ti.cbSize   = sizeof(TTTOOLINFOA);
        ti.uId      = 0;
        ti.hwnd     = (UINT)hwnd;

        infoPtr->flags &= ~TB_SHOW_TOOLTIP;
        SendMessageA (infoPtr->hwndToolTip, TTM_TRACKACTIVATE,
                      (WPARAM)FALSE, (LPARAM)&ti);
    }
    
    InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_CaptureChanged (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
	
    if (infoPtr->flags & TB_DRAGPOSVALID) {
        infoPtr->nPos=infoPtr->dragPos;
        InvalidateRect (hwnd, NULL, FALSE);
    }
	
    infoPtr->flags &= ~ TB_DRAGPOSVALID;

    TRACKBAR_SendNotify (hwnd, TB_ENDTRACK);
    return 0;
}


static LRESULT
TRACKBAR_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    TRACKBAR_Refresh (hwnd, hdc);
    if(!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
TRACKBAR_SetFocus (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    TRACE("\n");
    infoPtr->bFocus = TRUE;

    InvalidateRect (hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);

    TRACKBAR_CalcChannel (hwnd, infoPtr);
    TRACKBAR_AlignBuddies (hwnd, infoPtr);

    return 0;
}


static BOOL
TRACKBAR_SendNotify (HWND hwnd, UINT code)
{
    TRACE("%x\n",code);

    if (GetWindowLongA (hwnd, GWL_STYLE) & TBS_VERT) 
    	return (BOOL) SendMessageA (GetParent (hwnd), 
                                    WM_VSCROLL, (WPARAM)code, (LPARAM)hwnd);

    return (BOOL) SendMessageA (GetParent (hwnd), 
                                WM_HSCROLL, (WPARAM)code, (LPARAM)hwnd);
}


static LRESULT
TRACKBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    SHORT clickPlace;
    DOUBLE dragPos;
    char buf[80];
			
    TRACE("%x\n",wParam);

    if (dwStyle & TBS_VERT)
        clickPlace=(SHORT)HIWORD(lParam);
    else
        clickPlace=(SHORT)LOWORD(lParam);

    if (!(infoPtr->flags & TB_DRAG_MODE))
	return TRUE;

    SetCapture (hwnd);
    dragPos = TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace, 
                                               dwStyle & TBS_VERT);
    if (dragPos > ((INT)dragPos) + 0.5)
        infoPtr->dragPos = dragPos + 1;
    else
        infoPtr->dragPos = dragPos;

    infoPtr->flags |= TB_DRAGPOSVALID;
    TRACKBAR_SendNotify (hwnd, TB_THUMBTRACK | (infoPtr->nPos>>16));

    if (infoPtr->flags & TB_SHOW_TOOLTIP) {
        POINT pt;
    	TTTOOLINFOA ti;
	
    	ti.cbSize = sizeof(TTTOOLINFOA);
	ti.hwnd = hwnd;
    	ti.uId = 0;
        ti.hinst=0;
        sprintf (buf,"%d",infoPtr->nPos);
    	ti.lpszText = (LPSTR) buf;
        GetCursorPos (&pt);
		
	if (dwStyle & TBS_VERT) {
            SendMessageA (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 
                          0, (LPARAM)MAKELPARAM(pt.x+5, pt.y+15));
        } else {
            SendMessageA (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 
                          0, (LPARAM)MAKELPARAM(pt.x+15, pt.y+5));
        }
    	SendMessageA (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTA,
                      0, (LPARAM)&ti);
    }

    InvalidateRect (hwnd, NULL, FALSE);
    UpdateWindow (hwnd);

    return TRUE;
}


static LRESULT
TRACKBAR_KeyDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = TRACKBAR_GetInfoPtr (hwnd);
    INT pos;

    TRACE("%x\n",wParam);

    pos=infoPtr->nPos;
    switch (wParam) {
    case VK_LEFT:
    case VK_UP: 
        if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
        infoPtr->nPos -= infoPtr->nLineSize;
        if (infoPtr->nPos < infoPtr->nRangeMin) 
            infoPtr->nPos = infoPtr->nRangeMin;
        TRACKBAR_SendNotify (hwnd, TB_LINEUP);
        break;
    case VK_RIGHT:
    case VK_DOWN: 
        if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
        infoPtr->nPos += infoPtr->nLineSize;
        if (infoPtr->nPos > infoPtr->nRangeMax) 
            infoPtr->nPos = infoPtr->nRangeMax;
        TRACKBAR_SendNotify (hwnd, TB_LINEDOWN);
        break;
    case VK_NEXT:
        if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
        infoPtr->nPos += infoPtr->nPageSize;
        if (infoPtr->nPos > infoPtr->nRangeMax) 
            infoPtr->nPos = infoPtr->nRangeMax;
        TRACKBAR_SendNotify (hwnd, TB_PAGEUP);
        break;
    case VK_PRIOR:
        if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
        infoPtr->nPos -= infoPtr->nPageSize;
        if (infoPtr->nPos < infoPtr->nRangeMin) 
            infoPtr->nPos = infoPtr->nRangeMin;
        TRACKBAR_SendNotify (hwnd, TB_PAGEDOWN);
        break;
    case VK_HOME: 
        if (infoPtr->nPos == infoPtr->nRangeMin) return FALSE;
        infoPtr->nPos = infoPtr->nRangeMin;
        TRACKBAR_SendNotify (hwnd, TB_TOP);
        break;
    case VK_END: 
        if (infoPtr->nPos == infoPtr->nRangeMax) return FALSE;
        infoPtr->nPos = infoPtr->nRangeMax;
        TRACKBAR_SendNotify (hwnd, TB_BOTTOM);
        break;
    }

    if (pos!=infoPtr->nPos) { 
	infoPtr->flags |=TB_THUMBPOSCHANGED;
	InvalidateRect (hwnd, NULL, FALSE);
    }

    return TRUE;
}


static LRESULT
TRACKBAR_KeyUp (HWND hwnd, WPARAM wParam)
{
    switch (wParam) {
    case VK_LEFT:
    case VK_UP: 
    case VK_RIGHT:
    case VK_DOWN: 
    case VK_NEXT:
    case VK_PRIOR:
    case VK_HOME: 
    case VK_END:
        TRACKBAR_SendNotify (hwnd, TB_ENDTRACK);
    }
    return TRUE;
}


LRESULT WINAPI
TRACKBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case TBM_CLEARSEL:
        return TRACKBAR_ClearSel (hwnd, wParam, lParam);

    case TBM_CLEARTICS:
        return TRACKBAR_ClearTics (hwnd, wParam, lParam);

    case TBM_GETBUDDY:
        return TRACKBAR_GetBuddy (hwnd, wParam, lParam);

    case TBM_GETCHANNELRECT:
        return TRACKBAR_GetChannelRect (hwnd, wParam, lParam);

    case TBM_GETLINESIZE:
        return TRACKBAR_GetLineSize (hwnd, wParam, lParam);

    case TBM_GETNUMTICS:
        return TRACKBAR_GetNumTics (hwnd, wParam, lParam);

    case TBM_GETPAGESIZE:
        return TRACKBAR_GetPageSize (hwnd, wParam, lParam);

    case TBM_GETPOS:
        return TRACKBAR_GetPos (hwnd, wParam, lParam);

    case TBM_GETPTICS:
        return TRACKBAR_GetPTics (hwnd);

    case TBM_GETRANGEMAX:
        return TRACKBAR_GetRangeMax (hwnd, wParam, lParam);

    case TBM_GETRANGEMIN:
        return TRACKBAR_GetRangeMin (hwnd, wParam, lParam);

    case TBM_GETSELEND:
        return TRACKBAR_GetSelEnd (hwnd, wParam, lParam);

    case TBM_GETSELSTART:
        return TRACKBAR_GetSelStart (hwnd, wParam, lParam);

    case TBM_GETTHUMBLENGTH:
        return TRACKBAR_GetThumbLength (hwnd, wParam, lParam);

    case TBM_GETTHUMBRECT:
        return TRACKBAR_GetThumbRect (hwnd, wParam, lParam);

    case TBM_GETTIC:
        return TRACKBAR_GetTic (hwnd, wParam, lParam);
 
    case TBM_GETTICPOS:
        return TRACKBAR_GetTicPos (hwnd, wParam, lParam);
 
    case TBM_GETTOOLTIPS:
        return TRACKBAR_GetToolTips (hwnd, wParam, lParam);

/*	case TBM_GETUNICODEFORMAT: */

    case TBM_SETBUDDY:
        return TRACKBAR_SetBuddy (hwnd, wParam, lParam);

    case TBM_SETLINESIZE:
        return TRACKBAR_SetLineSize (hwnd, wParam, lParam);

    case TBM_SETPAGESIZE:
        return TRACKBAR_SetPageSize (hwnd, wParam, lParam);

    case TBM_SETPOS:
        return TRACKBAR_SetPos (hwnd, wParam, lParam);

    case TBM_SETRANGE:
        return TRACKBAR_SetRange (hwnd, wParam, lParam);

    case TBM_SETRANGEMAX:
        return TRACKBAR_SetRangeMax (hwnd, wParam, lParam);

    case TBM_SETRANGEMIN:
        return TRACKBAR_SetRangeMin (hwnd, wParam, lParam);

    case TBM_SETSEL:
        return TRACKBAR_SetSel (hwnd, wParam, lParam);

    case TBM_SETSELEND:
        return TRACKBAR_SetSelEnd (hwnd, wParam, lParam);

    case TBM_SETSELSTART:
        return TRACKBAR_SetSelStart (hwnd, wParam, lParam);

    case TBM_SETTHUMBLENGTH:
        return TRACKBAR_SetThumbLength (hwnd, wParam, lParam);

    case TBM_SETTIC:
        return TRACKBAR_SetTic (hwnd, wParam, lParam);

    case TBM_SETTICFREQ:
        return TRACKBAR_SetTicFreq (hwnd, wParam);

    case TBM_SETTIPSIDE:
        return TRACKBAR_SetTipSide (hwnd, wParam, lParam);

    case TBM_SETTOOLTIPS:
        return TRACKBAR_SetToolTips (hwnd, wParam, lParam);

/*	case TBM_SETUNICODEFORMAT: */


    case WM_CAPTURECHANGED:
        return TRACKBAR_CaptureChanged (hwnd, wParam, lParam);

    case WM_CREATE:
        return TRACKBAR_Create (hwnd, wParam, lParam);

    case WM_DESTROY:
        return TRACKBAR_Destroy (hwnd, wParam, lParam);

/*	case WM_ENABLE: */

/*	case WM_ERASEBKGND: */
/*	    return 0; */

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_KEYDOWN:
        return TRACKBAR_KeyDown (hwnd, wParam, lParam);
        
    case WM_KEYUP:
        return TRACKBAR_KeyUp (hwnd, wParam);

    case WM_KILLFOCUS:
        return TRACKBAR_KillFocus (hwnd, wParam, lParam);

    case WM_LBUTTONDOWN:
        return TRACKBAR_LButtonDown (hwnd, wParam, lParam);

    case WM_LBUTTONUP:
        return TRACKBAR_LButtonUp (hwnd, wParam, lParam);

    case WM_MOUSEMOVE:
        return TRACKBAR_MouseMove (hwnd, wParam, lParam);

    case WM_PAINT:
        return TRACKBAR_Paint (hwnd, wParam);

    case WM_SETFOCUS:
        return TRACKBAR_SetFocus (hwnd, wParam, lParam);

    case WM_SIZE:
        return TRACKBAR_Size (hwnd, wParam, lParam);

    case WM_WININICHANGE:
        return TRACKBAR_InitializeThumb (hwnd);

    default:
        if (uMsg >= WM_USER)
            ERR("unknown msg %04x wp=%08x lp=%08lx\n",
                 uMsg, wParam, lParam);
        return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
TRACKBAR_Register (void)
{
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (TRACKBAR_CLASSA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)TRACKBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASSA;
 
    RegisterClassA (&wndClass);
}


VOID
TRACKBAR_Unregister (void)
{
    if (GlobalFindAtomA (TRACKBAR_CLASSA))
	UnregisterClassA (TRACKBAR_CLASSA, (HINSTANCE)NULL);
}

