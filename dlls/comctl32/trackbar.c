/*
 * Trackbar control
 *
 * Copyright 1998, 1999 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 * Copyright 1998, 1999 Alex Priem <alexp@sci.kun.nl>
 * Copyright 2002 Dimitrie O. Paun <dimi@bigfoot.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *   - handle dragging slider better
 *   - increase with of the paging area
 *   - possition buddy controls
 *   - better tic handling.
 *   - more notifications.
 *   - TBM_SETRANGEMAX & TBM_SETRANGEMIN should only change the view of the
 *     trackbar, not the actual amount of tics in the list.
 *   - TBM_GETTIC & TBM_GETTICPOS shouldn't rely on infoPtr->tics being sorted.
 */

#include <stdio.h>
#include <string.h>

#include "winbase.h"
#include "commctrl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(trackbar);

typedef struct
{
    HWND hwndSelf;
    LONG lRangeMin;
    LONG lRangeMax;
    LONG lLineSize;
    LONG lPageSize;
    LONG lSelMin;
    LONG lSelMax;
    LONG lPos;
    UINT uThumbLen;
    UINT uNumTics;
    UINT uTicFreq;
    HWND hwndNotify;
    HWND hwndToolTip;
    HWND hwndBuddyLA;
    HWND hwndBuddyRB;
    INT  fLocation;
    COLORREF clrBk;
    INT  flags;
    BOOL bFocus;
    BOOL bUnicode;
    RECT rcChannel;
    RECT rcSelection;
    RECT rcThumb;
    INT  dragPos;
    LPLONG tics;
} TRACKBAR_INFO;

/* #define TB_REFRESH_TIMER       1 */
/* #define TB_REFRESH_DELAY       1 */


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

static BOOL TRACKBAR_SendNotify (TRACKBAR_INFO *infoPtr, UINT code);

static void TRACKBAR_RecalculateTics (TRACKBAR_INFO *infoPtr)
{
    int i,tic,nrTics;

    if (infoPtr->uTicFreq && infoPtr->lRangeMax >= infoPtr->lRangeMin)
    	nrTics=(infoPtr->lRangeMax - infoPtr->lRangeMin)/infoPtr->uTicFreq;
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
    tic=infoPtr->lRangeMin+infoPtr->uTicFreq;
    for (i=0; i<nrTics; i++,tic+=infoPtr->uTicFreq)
        infoPtr->tics[i]=tic;
}


/* converts from physical (mouse) position to logical position
   (in range of trackbar) */

static inline double
TRACKBAR_ConvertPlaceToPosition (TRACKBAR_INFO *infoPtr, int place,
                                 int vertical)
{
    double range, width, pos;

    range=infoPtr->lRangeMax - infoPtr->lRangeMin;
    if (vertical) {
    	width=infoPtr->rcChannel.bottom - infoPtr->rcChannel.top;
        pos=(range*(place - infoPtr->rcChannel.top)) / width;
    } else {
    	width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;
        pos=(range*(place - infoPtr->rcChannel.left)) / width;
    }
    pos += infoPtr->lRangeMin;
    if (pos > infoPtr->lRangeMax)
        pos = infoPtr->lRangeMax;
    else if (pos < infoPtr->lRangeMin)
        pos = infoPtr->lRangeMin;

    TRACE("%.2f\n", pos);
    return pos;
}


static void
TRACKBAR_CalcChannel (TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    INT cyChannel,offsettop,offsetedge;
    RECT lpRect,*channel = & infoPtr->rcChannel;

    GetClientRect (infoPtr->hwndSelf, &lpRect);

    if (dwStyle & TBS_ENABLESELRANGE)
        cyChannel = ((int)(infoPtr->uThumbLen/4.5)+1)*3;
    else
        cyChannel = 4;

    offsettop  = (int)(infoPtr->uThumbLen/4.5);
    offsetedge = (int)(infoPtr->uThumbLen/4.5) + 3;

    if (dwStyle & TBS_VERT) {
        channel->top    = lpRect.top + offsetedge;
        channel->bottom = lpRect.bottom - offsetedge;

	if (dwStyle & (TBS_BOTH | TBS_LEFT)) {
            channel->left  = lpRect.left + offsettop + 8 ;
            channel->right = channel->left + cyChannel;
        }
        else { /* TBS_RIGHT */
            channel->left = lpRect.left + offsettop;
            channel->right = channel->left + cyChannel;
        }
    }
    else {
        channel->left = lpRect.left + offsetedge;
        channel->right = lpRect.right - offsetedge;

        if (dwStyle & (TBS_BOTH|TBS_TOP)) {
            channel->top	= lpRect.top + offsettop + 8 ;
            channel->bottom 	= channel->top + cyChannel;
        }
        else { /* TBS_BOTTOM */
            channel->top = lpRect.top + offsettop;
            channel->bottom   = channel->top + cyChannel;
        }
    }
}

static void
TRACKBAR_CalcThumb (TRACKBAR_INFO *infoPtr)
{
    RECT *thumb;
    int range, width, thumbdepth;
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);

    thumb=&infoPtr->rcThumb;
    range=infoPtr->lRangeMax - infoPtr->lRangeMin;
    thumbdepth = ((INT)((FLOAT)infoPtr->uThumbLen / 4.5) * 2) + 2;

    if (!range) range = 1;

    if (dwStyle & TBS_VERT)
    {
    	width=infoPtr->rcChannel.bottom - infoPtr->rcChannel.top;

        if (dwStyle & (TBS_BOTH | TBS_LEFT))
            thumb->left = 10;
        else
            thumb->left = 2;
        thumb->right = thumb -> left + infoPtr->uThumbLen;
        thumb->top = infoPtr->rcChannel.top +
                     (width*(infoPtr->lPos - infoPtr->lRangeMin))/range -
                     thumbdepth/2;
        thumb->bottom = thumb->top + thumbdepth;
    }
    else
    {
    	width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;

        thumb->left = infoPtr->rcChannel.left +
                      (width*(infoPtr->lPos - infoPtr->lRangeMin))/range -
                      thumbdepth/2;
        thumb->right = thumb->left + thumbdepth;
        if (dwStyle & (TBS_BOTH | TBS_TOP))
              thumb->top = 10;
        else
              thumb->top = 2;
        thumb->bottom = thumb->top + infoPtr->uThumbLen;
    }
}

static void
TRACKBAR_CalcSelection (TRACKBAR_INFO *infoPtr)
{
    RECT *selection;
    int range, width;

    selection= & infoPtr->rcSelection;
    range=infoPtr->lRangeMax - infoPtr->lRangeMin;
    width=infoPtr->rcChannel.right - infoPtr->rcChannel.left;

    if (range <= 0)
        SetRectEmpty (selection);
    else
        if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT) {
            selection->left   = infoPtr->rcChannel.left +
                (width*infoPtr->lSelMin)/range;
            selection->right  = infoPtr->rcChannel.left +
                (width*infoPtr->lSelMax)/range;
            selection->top    = infoPtr->rcChannel.top + 2;
            selection->bottom = infoPtr->rcChannel.bottom - 2;
        } else {
            selection->top    = infoPtr->rcChannel.top +
                (width*infoPtr->lSelMin)/range;
            selection->bottom = infoPtr->rcChannel.top +
                (width*infoPtr->lSelMax)/range;
            selection->left   = infoPtr->rcChannel.left + 2;
            selection->right  = infoPtr->rcChannel.right - 2;
        }
}

/* Trackbar drawing code. I like my spaghetti done milanese.  */

/* ticPos is in tic-units, not in pixels */

static void
TRACKBAR_DrawHorizTic (TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos,
                       int flags, COLORREF clrTic)
{
    RECT rcChannel=infoPtr->rcChannel;
    int x,y,width,range,side;

    range=infoPtr->lRangeMax - infoPtr->lRangeMin;
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
            x=rcChannel.left + (width*(ticPos - infoPtr->lRangeMin))/range - 1;
	else
            x=rcChannel.left + (width*(ticPos - infoPtr->lRangeMin))/range + 1;

   	SetPixel (hdc, x,y+6*side, clrTic);
   	SetPixel (hdc, x,y+7*side, clrTic);
	return;
    }

    if ((ticPos>infoPtr->lRangeMin) && (ticPos<infoPtr->lRangeMax)) {
   	x=rcChannel.left + (width*(ticPos - infoPtr->lRangeMin))/range;
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

static void
TRACKBAR_DrawVertTic (TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos,
                      int flags, COLORREF clrTic)
{
    RECT rcChannel=infoPtr->rcChannel;
    int x,y,width,range,side;

    range=infoPtr->lRangeMax - infoPtr->lRangeMin;
    width=rcChannel.bottom - rcChannel.top;

    if (flags & TBS_TOP) {
	x=rcChannel.left-2;
	side=-1;
    } else {
  	x=rcChannel.right+2;
	side=1;
    }


    if (flags & TIC_SELECTIONMARK) {
  	if (flags & TIC_SELECTIONMARKMIN)
            y=rcChannel.top + (width*(ticPos - infoPtr->lRangeMin))/range - 1;
	else
            y=rcChannel.top + (width*(ticPos - infoPtr->lRangeMin))/range + 1;

   	SetPixel (hdc, x+6*side, y, clrTic);
   	SetPixel (hdc, x+7*side, y, clrTic);
	return;
    }

    if ((ticPos>infoPtr->lRangeMin) && (ticPos<infoPtr->lRangeMax)) {
   	y=rcChannel.top + (width*(ticPos - infoPtr->lRangeMin))/range;
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


static void
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

static void
TRACKBAR_DrawThumb(TRACKBAR_INFO *infoPtr, HDC hdc, DWORD dwStyle)
{
    HBRUSH oldbr,hbr = GetSysColorBrush(COLOR_BTNFACE);
    HPEN  oldpen=(HPEN)NULL,hpn;
    RECT thumb = infoPtr->rcThumb;
    int BlackUntil=3;
    int PointCount=6;
    POINT points[6];

    static INT PointDepth = 4;

    oldbr = SelectObject (hdc, hbr);
    SetPolyFillMode (hdc,WINDING);

    if (dwStyle & TBS_BOTH)
    {
       points[0].x=thumb.right;
       points[0].y=thumb.top;
       points[1].x=thumb.right;
       points[1].y=thumb.bottom;
       points[2].x=thumb.left;
       points[2].y=thumb.bottom;
       points[3].x=thumb.left;
       points[3].y=thumb.top;
       points[4].x=points[0].x;
       points[4].y=points[0].y;
       PointCount = 5;
       BlackUntil = 3;
    }
    else
    {
        if (dwStyle & TBS_VERT)
        {
          if (dwStyle & TBS_LEFT)
          {
            points[0].x=thumb.right;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.bottom;
            points[2].x=thumb.left + PointDepth;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=(thumb.bottom - thumb.top) / 2 + thumb.top;
            points[4].x=thumb.left + PointDepth;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
            BlackUntil = 4;
          }
          else
          {
            points[0].x=thumb.right;
            points[0].y=(thumb.bottom - thumb.top) / 2 + thumb.top;
            points[1].x=thumb.right - PointDepth;
            points[1].y=thumb.bottom;
            points[2].x=thumb.left;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.top;
            points[4].x=thumb.right - PointDepth;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
          }
        }
        else
        {
          if (dwStyle & TBS_TOP)
          {
            points[0].x=(thumb.right - thumb.left) / 2 + thumb.left ;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.top + PointDepth;
            points[2].x=thumb.right;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.bottom;
            points[4].x=thumb.left;
            points[4].y=thumb.top + PointDepth;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
            BlackUntil = 4;
          }
          else
          {
            points[0].x=thumb.right;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.bottom - PointDepth;
            points[2].x=(thumb.right - thumb.left) / 2 + thumb.left ;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.bottom - PointDepth;
            points[4].x=thumb.left;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
          }
        }

    }

    /*
     * Fill the shape
     */
    Polygon (hdc, points, PointCount);

    /*
     * Draw the edge
     */
    hpn = GetStockObject(BLACK_PEN);
    oldpen = SelectObject(hdc,hpn);

    /*
     * Black part
     */
    Polyline(hdc,points,BlackUntil);

    SelectObject(hdc,oldpen);
    hpn = GetStockObject(WHITE_PEN);
    SelectObject(hdc,hpn);

    /*
     * White Part
     */
    Polyline(hdc,&points[BlackUntil-1],PointCount+1-BlackUntil);

    /*
     * restore the brush and pen
     */
    SelectObject(hdc,oldbr);
    if (oldpen)
      SelectObject(hdc,oldpen);
}

static void
TRACKBAR_Refresh (TRACKBAR_INFO *infoPtr, HDC hdc)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    RECT rcClient, rcChannel, rcSelection;
    HBRUSH hBrush;
    int i;

    GetClientRect (infoPtr->hwndSelf, &rcClient);
    hBrush = CreateSolidBrush (infoPtr->clrBk);
    FillRect (hdc, &rcClient, hBrush);
    DeleteObject (hBrush);

    if (infoPtr->flags & TB_DRAGPOSVALID)  {
        infoPtr->lPos=infoPtr->dragPos;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }

    if (infoPtr->flags & TB_THUMBCHANGED) {
        TRACKBAR_CalcThumb	(infoPtr);
        if (infoPtr->flags & TB_THUMBSIZECHANGED)
            TRACKBAR_CalcChannel (infoPtr);
    }
    if (infoPtr->flags & TB_SELECTIONCHANGED)
        TRACKBAR_CalcSelection (infoPtr);
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
            TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->lSelMin,
                               ticFlags | TIC_SELECTIONMARKMIN, clrTic);
            TRACKBAR_DrawTics (infoPtr, hdc, infoPtr->lSelMax,
                               ticFlags | TIC_SELECTIONMARKMAX, clrTic);
        }
    }

    /* draw thumb */

    if (!(dwStyle & TBS_NOTHUMB))
    {
      TRACKBAR_DrawThumb(infoPtr,hdc,dwStyle);
    }

    if (infoPtr->bFocus)
        DrawFocusRect (hdc, &rcClient);
}


static void
TRACKBAR_AlignBuddies (TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    HWND hwndParent = GetParent (infoPtr->hwndSelf);
    RECT rcSelf, rcBuddy;
    INT x, y;

    GetWindowRect (infoPtr->hwndSelf, &rcSelf);
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
TRACKBAR_ClearSel (TRACKBAR_INFO *infoPtr, BOOL fRedraw)
{
    infoPtr->lSelMin = 0;
    infoPtr->lSelMax = 0;
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if (fRedraw)
	InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_ClearTics (TRACKBAR_INFO *infoPtr, BOOL fRedraw)
{
    if (infoPtr->tics) {
        COMCTL32_Free (infoPtr->tics);
        infoPtr->tics = NULL;
        infoPtr->uNumTics = 0;
    }

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static HWND inline
TRACKBAR_GetBuddy (TRACKBAR_INFO *infoPtr, BOOL fLocation)
{
    return fLocation ? infoPtr->hwndBuddyLA : infoPtr->hwndBuddyRB;
}


static LRESULT inline
TRACKBAR_GetChannelRect (TRACKBAR_INFO *infoPtr, LPRECT lprc)
{
    if (lprc == NULL) return 0;

    lprc->left   = infoPtr->rcChannel.left;
    lprc->right  = infoPtr->rcChannel.right;
    lprc->bottom = infoPtr->rcChannel.bottom;
    lprc->top    = infoPtr->rcChannel.top;

    return 0;
}


static LONG inline
TRACKBAR_GetLineSize (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lLineSize;
}


static LONG inline
TRACKBAR_GetNumTics (TRACKBAR_INFO *infoPtr)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_NOTICKS)
        return 0;

    return infoPtr->uNumTics + 2;
}


static LONG inline
TRACKBAR_GetPageSize (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lPageSize;
}


static LONG inline
TRACKBAR_GetPos (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lPos;
}


static LONG inline
TRACKBAR_GetRangeMax (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lRangeMax;
}


static LONG inline
TRACKBAR_GetRangeMin (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lRangeMin;
}


static LONG inline
TRACKBAR_GetSelEnd (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lSelMax;
}


static LONG inline
TRACKBAR_GetSelStart (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lSelMin;
}


static UINT inline
TRACKBAR_GetThumbLength (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->uThumbLen;
}

static LPLONG inline
TRACKBAR_GetPTics (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->tics;
}

static LRESULT inline
TRACKBAR_GetThumbRect (TRACKBAR_INFO *infoPtr, LPRECT lprc)
{
    if (lprc == NULL)
        return 0;

    lprc->left   = infoPtr->rcThumb.left;
    lprc->right  = infoPtr->rcThumb.right;
    lprc->bottom = infoPtr->rcThumb.bottom;
    lprc->top    = infoPtr->rcThumb.top;

    return 0;
}


static LONG inline
TRACKBAR_GetTic (TRACKBAR_INFO *infoPtr, INT iTic)
{
    if ((iTic < 0) || (iTic > infoPtr->uNumTics))
	return -1;

    return infoPtr->tics[iTic];

}


static LONG inline
TRACKBAR_GetTicPos (TRACKBAR_INFO *infoPtr, INT iTic)
{
    LONG range, width, pos;

    if ((iTic < 0) || (iTic > infoPtr->uNumTics))
	return -1;

    range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    width = infoPtr->rcChannel.right - infoPtr->rcChannel.left;
    pos   = infoPtr->rcChannel.left + (width * infoPtr->tics[iTic]) / range;

    return pos;
}


static HWND inline
TRACKBAR_GetToolTips (TRACKBAR_INFO *infoPtr)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_TOOLTIPS)
        return infoPtr->hwndToolTip;
    return 0;
}


static BOOL inline
TRACKBAR_GetUnicodeFormat (TRACKBAR_INFO *infoPtr)
{
    return infoPtr->bUnicode;
}


static HWND
TRACKBAR_SetBuddy (TRACKBAR_INFO *infoPtr, BOOL fLocation, HWND hwndBuddy)
{
    HWND hwndTemp;

    if (fLocation) {
	/* buddy is left or above */
	hwndTemp = infoPtr->hwndBuddyLA;
	infoPtr->hwndBuddyLA = hwndBuddy;

	FIXME("move buddy!\n");
    }
    else {
        /* buddy is right or below */
        hwndTemp = infoPtr->hwndBuddyRB;
        infoPtr->hwndBuddyRB = hwndBuddy;

        FIXME("move buddy!\n");
    }

    TRACKBAR_AlignBuddies (infoPtr);

    return hwndTemp;
}


static LONG inline
TRACKBAR_SetLineSize (TRACKBAR_INFO *infoPtr, LONG lLineSize)
{
    LONG lTemp = infoPtr->lLineSize;

    infoPtr->lLineSize = lLineSize;

    return lTemp;
}


static LONG inline
TRACKBAR_SetPageSize (TRACKBAR_INFO *infoPtr, LONG lPageSize)
{
    LONG lTemp = infoPtr->lPageSize;

    infoPtr->lPageSize = lPageSize;

    return lTemp;
}


static LRESULT inline
TRACKBAR_SetPos (TRACKBAR_INFO *infoPtr, BOOL fPosition, LONG lPosition)
{
    infoPtr->lPos = lPosition;

    if (infoPtr->lPos < infoPtr->lRangeMin)
	infoPtr->lPos = infoPtr->lRangeMin;

    if (infoPtr->lPos > infoPtr->lRangeMax)
	infoPtr->lPos = infoPtr->lRangeMax;
    infoPtr->flags |= TB_THUMBPOSCHANGED;

    if (fPosition)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetRange (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lRange)
{
    infoPtr->lRangeMin = LOWORD(lRange);
    infoPtr->lRangeMax = HIWORD(lRange);

    if (infoPtr->lPos < infoPtr->lRangeMin) {
        infoPtr->lPos = infoPtr->lRangeMin;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    if (infoPtr->lPos > infoPtr->lRangeMax) {
        infoPtr->lPos = infoPtr->lRangeMax;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize=(infoPtr->lRangeMax -  infoPtr->lRangeMin)/5;
    if (infoPtr->lPageSize == 0)
        infoPtr->lPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetRangeMax (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lMax)
{
    infoPtr->lRangeMax = lMax;
    if (infoPtr->lPos > infoPtr->lRangeMax) {
        infoPtr->lPos = infoPtr->lRangeMax;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize=(infoPtr->lRangeMax -  infoPtr->lRangeMin)/5;
    if (infoPtr->lPageSize == 0)
        infoPtr->lPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetRangeMin (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lMin)
{
    infoPtr->lRangeMin = lMin;
    if (infoPtr->lPos < infoPtr->lRangeMin) {
        infoPtr->lPos = infoPtr->lRangeMin;
        infoPtr->flags |=TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize=(infoPtr->lRangeMax -  infoPtr->lRangeMin)/5;
    if (infoPtr->lPageSize == 0)
        infoPtr->lPageSize = 1;
    TRACKBAR_RecalculateTics (infoPtr);

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetSel (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lSel)
{
    infoPtr->lSelMin = LOWORD(lSel);
    infoPtr->lSelMax = HIWORD(lSel);
    infoPtr->flags |=TB_SELECTIONCHANGED;

    if (!GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)
        return 0;

    if (infoPtr->lSelMin < infoPtr->lRangeMin)
        infoPtr->lSelMin = infoPtr->lRangeMin;
    if (infoPtr->lSelMax > infoPtr->lRangeMax)
        infoPtr->lSelMax = infoPtr->lRangeMax;

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetSelEnd (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lEnd)
{
    if (!GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->lSelMax = lEnd;
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if (infoPtr->lSelMax > infoPtr->lRangeMax)
        infoPtr->lSelMax = infoPtr->lRangeMax;

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetSelStart (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lStart)
{
    if (!GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)
	return 0;

    infoPtr->lSelMin = lStart;
    infoPtr->flags  |=TB_SELECTIONCHANGED;

    if (infoPtr->lSelMin < infoPtr->lRangeMin)
        infoPtr->lSelMin = infoPtr->lRangeMin;

    if (fRedraw)
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetThumbLength (TRACKBAR_INFO *infoPtr, UINT iLength)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_FIXEDLENGTH)
        infoPtr->uThumbLen = iLength;

    infoPtr->flags |= TB_THUMBSIZECHANGED;

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT inline
TRACKBAR_SetTic (TRACKBAR_INFO *infoPtr, LONG lPos)
{
    if ((lPos < infoPtr->lRangeMin) || (lPos> infoPtr->lRangeMax))
        return FALSE;

    infoPtr->uNumTics++;
    infoPtr->tics=COMCTL32_ReAlloc( infoPtr->tics,
                                    (infoPtr->uNumTics)*sizeof (DWORD));
    infoPtr->tics[infoPtr->uNumTics-1]=lPos;

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return TRUE;
}


static LRESULT inline
TRACKBAR_SetTicFreq (TRACKBAR_INFO *infoPtr, WORD wFreq)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_AUTOTICKS)
        infoPtr->uTicFreq = wFreq;

    TRACKBAR_RecalculateTics (infoPtr);

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static INT inline
TRACKBAR_SetTipSide (TRACKBAR_INFO *infoPtr, INT fLocation)
{
    INT fTemp = infoPtr->fLocation;

    infoPtr->fLocation = fLocation;

    return fTemp;
}


static LRESULT inline
TRACKBAR_SetToolTips (TRACKBAR_INFO *infoPtr, HWND hwndTT)
{
    infoPtr->hwndToolTip = hwndTT;

    return 0;
}


static BOOL inline
TRACKBAR_SetUnicodeFormat (TRACKBAR_INFO *infoPtr, BOOL fUnicode)
{
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = fUnicode;

    return bTemp;
}


static LRESULT
TRACKBAR_InitializeThumb (TRACKBAR_INFO *infoPtr)
{
    infoPtr->uThumbLen = 23;   /* initial thumb length */

    TRACKBAR_CalcChannel (infoPtr);
    TRACKBAR_CalcThumb (infoPtr);
    infoPtr->flags &= ~TB_SELECTIONCHANGED;

    return 0;
}


static LRESULT
TRACKBAR_Create (HWND hwnd, LPCREATESTRUCTW lpcs)
{
    TRACKBAR_INFO *infoPtr;

    infoPtr = (TRACKBAR_INFO *)COMCTL32_Alloc (sizeof(TRACKBAR_INFO));
    if (!infoPtr) return -1;
    SetWindowLongW (hwnd, 0, (DWORD)infoPtr);

    /* set default values */
    infoPtr->hwndSelf  = hwnd;
    infoPtr->lRangeMin = 0;
    infoPtr->lRangeMax = 100;
    infoPtr->lLineSize = 1;
    infoPtr->lPageSize = 20;
    infoPtr->lSelMin   = 0;
    infoPtr->lSelMax   = 0;
    infoPtr->lPos      = 0;

    infoPtr->uNumTics  = 0;    /* start and end tic are not included in count*/
    infoPtr->uTicFreq  = 1;
    infoPtr->tics      = NULL;
    infoPtr->clrBk     = GetSysColor (COLOR_BTNFACE);
    infoPtr->hwndNotify= GetParent (hwnd);

    TRACKBAR_InitializeThumb (infoPtr);

    /* Create tooltip control */
    if (GetWindowLongW (hwnd, GWL_STYLE) & TBS_TOOLTIPS) {
        TTTOOLINFOW ti;
	WCHAR testStrW[] = { 'T', 'e', 's', 't', 0 };

    	infoPtr->hwndToolTip =
            CreateWindowExW (0, TOOLTIPS_CLASSW, NULL, 0,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hwnd, 0, 0, 0);

        /* Send NM_TOOLTIPSCREATED notification */
    	if (infoPtr->hwndToolTip) {
            NMTOOLTIPSCREATED nmttc;

            nmttc.hdr.hwndFrom = hwnd;
            nmttc.hdr.idFrom   = GetWindowLongW (hwnd, GWL_ID);
            nmttc.hdr.code = NM_TOOLTIPSCREATED;
            nmttc.hwndToolTips = infoPtr->hwndToolTip;

            SendMessageW (GetParent (hwnd), WM_NOTIFY,
                          (WPARAM)nmttc.hdr.idFrom, (LPARAM)&nmttc);
    	}

        ZeroMemory (&ti, sizeof(TTTOOLINFOW));
        ti.cbSize   = sizeof(TTTOOLINFOW);
     	ti.uFlags   = TTF_IDISHWND | TTF_TRACK;
	ti.hwnd     = hwnd;
        ti.uId      = 0;
        ti.lpszText = testStrW; /* LPSTR_TEXTCALLBACK */
        SetRectEmpty (&ti.rect);

        SendMessageW (infoPtr->hwndToolTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }

    return 0;
}


static LRESULT
TRACKBAR_Destroy (TRACKBAR_INFO *infoPtr)
{
    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
    	DestroyWindow (infoPtr->hwndToolTip);

    COMCTL32_Free (infoPtr);
    SetWindowLongW (infoPtr->hwndSelf, 0, 0);
    return 0;
}


static LRESULT
TRACKBAR_KillFocus (TRACKBAR_INFO *infoPtr, HWND hwndGetFocus)
{
    TRACE("\n");

    infoPtr->bFocus = FALSE;
    infoPtr->flags &= ~TB_DRAG_MODE;

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_LButtonDown (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT xPos, INT yPos)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    POINT clickPoint = { xPos, yPos };

    SetFocus (infoPtr->hwndSelf);

    if (PtInRect(&(infoPtr->rcThumb),clickPoint))
    {
        infoPtr->flags |= TB_DRAG_MODE;
        if (dwStyle & TBS_TOOLTIPS) {  /* enable tooltip */
            TTTOOLINFOW ti;
            POINT pt;

            GetCursorPos (&pt);
            SendMessageW (infoPtr->hwndToolTip, TTM_TRACKPOSITION, 0,
                          (LPARAM)MAKELPARAM(pt.x, pt.y));

            ti.cbSize   = sizeof(TTTOOLINFOW);
            ti.uId      = 0;
            ti.hwnd     = infoPtr->hwndSelf;

            infoPtr->flags |= TB_SHOW_TOOLTIP;
            SetCapture (infoPtr->hwndSelf);
            SendMessageW (infoPtr->hwndToolTip, TTM_TRACKACTIVATE,
                          (WPARAM)TRUE, (LPARAM)&ti);
        }
        return 0;
    }
    else if (PtInRect(&(infoPtr->rcChannel),clickPoint))
    {
        int clickPlace,prevPos,vertical;
        DOUBLE clickPos;

        vertical = (dwStyle & TBS_VERT) ? 1 : 0;
	clickPlace = vertical ? yPos : xPos;

        clickPos = TRACKBAR_ConvertPlaceToPosition(infoPtr, clickPlace,
                                                   vertical);
        prevPos = infoPtr->lPos;
        if (clickPos > (int)prevPos)
        {  /* similar to VK_NEXT */
            infoPtr->lPos += infoPtr->lPageSize;
            if (infoPtr->lPos > infoPtr->lRangeMax)
                infoPtr->lPos = infoPtr->lRangeMax;
            TRACKBAR_SendNotify (infoPtr, TB_PAGEUP);
        }
        else
        {
            infoPtr->lPos -= infoPtr->lPageSize;  /* similar to VK_PRIOR */
            if (infoPtr->lPos < infoPtr->lRangeMin)
                infoPtr->lPos = infoPtr->lRangeMin;
            TRACKBAR_SendNotify (infoPtr, TB_PAGEDOWN);
        }

        if (prevPos!=infoPtr->lPos) {
            infoPtr->flags |= TB_THUMBPOSCHANGED;
            InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);
        }
    }

    return 0;
}


static LRESULT
TRACKBAR_LButtonUp (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT xPos, INT yPos)
{
    TRACKBAR_SendNotify (infoPtr, TB_ENDTRACK);

    if (infoPtr->flags & TB_DRAG_MODE)
    {
        infoPtr->flags &= ~TB_DRAG_MODE;
        ReleaseCapture ();
    }

    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_TOOLTIPS) {
    	TTTOOLINFOW ti;

        ti.cbSize   = sizeof(TTTOOLINFOW);
        ti.uId      = 0;
        ti.hwnd     = infoPtr->hwndSelf;

        infoPtr->flags &= ~TB_SHOW_TOOLTIP;
        SendMessageW (infoPtr->hwndToolTip, TTM_TRACKACTIVATE,
                      (WPARAM)FALSE, (LPARAM)&ti);
    }

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_CaptureChanged (TRACKBAR_INFO *infoPtr)
{
    if (infoPtr->flags & TB_DRAGPOSVALID) {
        infoPtr->lPos=infoPtr->dragPos;
        InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);
    }

    infoPtr->flags &= ~ TB_DRAGPOSVALID;

    TRACKBAR_SendNotify (infoPtr, TB_ENDTRACK);
    return 0;
}


static LRESULT
TRACKBAR_Paint (TRACKBAR_INFO *infoPtr, HDC hdc)
{
    if (hdc) {
	TRACKBAR_Refresh(infoPtr, hdc);
    } else {
	PAINTSTRUCT ps;
    	hdc = BeginPaint (infoPtr->hwndSelf, &ps);
    	TRACKBAR_Refresh (infoPtr, hdc);
    	EndPaint (infoPtr->hwndSelf, &ps);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetFocus (TRACKBAR_INFO *infoPtr, HWND hwndLoseFocus)
{
    TRACE("\n");

    infoPtr->bFocus = TRUE;

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);

    return 0;
}


static LRESULT
TRACKBAR_Size (TRACKBAR_INFO *infoPtr, DWORD fwSizeType, INT nWidth, INT nHeight)
{
    TRACKBAR_CalcChannel (infoPtr);
    TRACKBAR_AlignBuddies (infoPtr);

    return 0;
}


static BOOL
TRACKBAR_SendNotify (TRACKBAR_INFO *infoPtr, UINT code)
{
    BOOL bVert = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT;

    TRACE("%x\n", code);

    return (BOOL) SendMessageW (GetParent (infoPtr->hwndSelf),
                                bVert ? WM_VSCROLL : WM_HSCROLL,
				(WPARAM)code, (LPARAM)infoPtr->hwndSelf);
}


static LRESULT
TRACKBAR_MouseMove (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT xPos, INT yPos)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    INT clickPlace;
    DOUBLE dragPos;

    TRACE("(x=%d. y=%d)\n", xPos, yPos);

    if (dwStyle & TBS_VERT)
        clickPlace = yPos;
    else
        clickPlace = xPos;

    if (!(infoPtr->flags & TB_DRAG_MODE))
	return TRUE;

    SetCapture (infoPtr->hwndSelf);
    dragPos = TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace,
                                               dwStyle & TBS_VERT);
    if (dragPos > ((INT)dragPos) + 0.5)
        infoPtr->dragPos = dragPos + 1;
    else
        infoPtr->dragPos = dragPos;

    infoPtr->flags |= TB_DRAGPOSVALID;
    TRACKBAR_SendNotify (infoPtr, TB_THUMBTRACK | (infoPtr->lPos<<16));

    if (infoPtr->flags & TB_SHOW_TOOLTIP) {
        POINT pt;
    	TTTOOLINFOW ti;
    	WCHAR buf[80], fmt[] = { '%', 'l', 'd', 0 };

    	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd   = infoPtr->hwndSelf;
    	ti.uId    = 0;
        ti.hinst  = 0;
        wsprintfW (buf, fmt, infoPtr->lPos);
    	ti.lpszText = buf;
        GetCursorPos (&pt);

	if (dwStyle & TBS_VERT) {
            SendMessageW (infoPtr->hwndToolTip, TTM_TRACKPOSITION,
                          0, (LPARAM)MAKELPARAM(pt.x+5, pt.y+15));
        } else {
            SendMessageW (infoPtr->hwndToolTip, TTM_TRACKPOSITION,
                          0, (LPARAM)MAKELPARAM(pt.x+15, pt.y+5));
        }
    	SendMessageW (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTW,
                      0, (LPARAM)&ti);
    }

    InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);
    UpdateWindow (infoPtr->hwndSelf);

    return TRUE;
}


static BOOL
TRACKBAR_KeyDown (TRACKBAR_INFO *infoPtr, INT nVirtKey, DWORD lKeyData)
{
    LONG pos = infoPtr->lPos;

    TRACE("%x\n", nVirtKey);

    switch (nVirtKey) {
    case VK_LEFT:
    case VK_UP:
        if (infoPtr->lPos == infoPtr->lRangeMin) return FALSE;
        infoPtr->lPos -= infoPtr->lLineSize;
        if (infoPtr->lPos < infoPtr->lRangeMin)
            infoPtr->lPos = infoPtr->lRangeMin;
        TRACKBAR_SendNotify (infoPtr, TB_LINEUP);
        break;
    case VK_RIGHT:
    case VK_DOWN:
        if (infoPtr->lPos == infoPtr->lRangeMax) return FALSE;
        infoPtr->lPos += infoPtr->lLineSize;
        if (infoPtr->lPos > infoPtr->lRangeMax)
            infoPtr->lPos = infoPtr->lRangeMax;
        TRACKBAR_SendNotify (infoPtr, TB_LINEDOWN);
        break;
    case VK_NEXT:
        if (infoPtr->lPos == infoPtr->lRangeMax) return FALSE;
        infoPtr->lPos += infoPtr->lPageSize;
        if (infoPtr->lPos > infoPtr->lRangeMax)
            infoPtr->lPos = infoPtr->lRangeMax;
        TRACKBAR_SendNotify (infoPtr, TB_PAGEUP);
        break;
    case VK_PRIOR:
        if (infoPtr->lPos == infoPtr->lRangeMin) return FALSE;
        infoPtr->lPos -= infoPtr->lPageSize;
        if (infoPtr->lPos < infoPtr->lRangeMin)
            infoPtr->lPos = infoPtr->lRangeMin;
        TRACKBAR_SendNotify (infoPtr, TB_PAGEDOWN);
        break;
    case VK_HOME:
        if (infoPtr->lPos == infoPtr->lRangeMin) return FALSE;
        infoPtr->lPos = infoPtr->lRangeMin;
        TRACKBAR_SendNotify (infoPtr, TB_TOP);
        break;
    case VK_END:
        if (infoPtr->lPos == infoPtr->lRangeMax) return FALSE;
        infoPtr->lPos = infoPtr->lRangeMax;
        TRACKBAR_SendNotify (infoPtr, TB_BOTTOM);
        break;
    }

    if (pos != infoPtr->lPos) {
	infoPtr->flags |=TB_THUMBPOSCHANGED;
	InvalidateRect (infoPtr->hwndSelf, NULL, FALSE);
    }

    return TRUE;
}


static BOOL inline
TRACKBAR_KeyUp (TRACKBAR_INFO *infoPtr, INT nVirtKey, DWORD lKeyData)
{
    switch (nVirtKey) {
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_NEXT:
    case VK_PRIOR:
    case VK_HOME:
    case VK_END:
        TRACKBAR_SendNotify (infoPtr, TB_ENDTRACK);
    }
    return TRUE;
}


static LRESULT WINAPI
TRACKBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = (TRACKBAR_INFO *)GetWindowLongW (hwnd, 0);

    TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case TBM_CLEARSEL:
        return TRACKBAR_ClearSel (infoPtr, (BOOL)wParam);

    case TBM_CLEARTICS:
        return TRACKBAR_ClearTics (infoPtr, (BOOL)wParam);

    case TBM_GETBUDDY:
        return (LRESULT)TRACKBAR_GetBuddy (infoPtr, (BOOL)wParam);

    case TBM_GETCHANNELRECT:
        return TRACKBAR_GetChannelRect (infoPtr, (LPRECT)lParam);

    case TBM_GETLINESIZE:
        return TRACKBAR_GetLineSize (infoPtr);

    case TBM_GETNUMTICS:
        return TRACKBAR_GetNumTics (infoPtr);

    case TBM_GETPAGESIZE:
        return TRACKBAR_GetPageSize (infoPtr);

    case TBM_GETPOS:
        return TRACKBAR_GetPos (infoPtr);

    case TBM_GETPTICS:
        return (LRESULT)TRACKBAR_GetPTics (infoPtr);

    case TBM_GETRANGEMAX:
        return TRACKBAR_GetRangeMax (infoPtr);

    case TBM_GETRANGEMIN:
        return TRACKBAR_GetRangeMin (infoPtr);

    case TBM_GETSELEND:
        return TRACKBAR_GetSelEnd (infoPtr);

    case TBM_GETSELSTART:
        return TRACKBAR_GetSelStart (infoPtr);

    case TBM_GETTHUMBLENGTH:
        return TRACKBAR_GetThumbLength (infoPtr);

    case TBM_GETTHUMBRECT:
        return TRACKBAR_GetThumbRect (infoPtr, (LPRECT)lParam);

    case TBM_GETTIC:
        return TRACKBAR_GetTic (infoPtr, (INT)wParam);

    case TBM_GETTICPOS:
        return TRACKBAR_GetTicPos (infoPtr, (INT)wParam);

    case TBM_GETTOOLTIPS:
        return (LRESULT)TRACKBAR_GetToolTips (infoPtr);

    case TBM_GETUNICODEFORMAT:
	return TRACKBAR_GetUnicodeFormat(infoPtr);

    case TBM_SETBUDDY:
        return (LRESULT) TRACKBAR_SetBuddy(infoPtr, (BOOL)wParam, (HWND)lParam);

    case TBM_SETLINESIZE:
        return TRACKBAR_SetLineSize (infoPtr, (LONG)lParam);

    case TBM_SETPAGESIZE:
        return TRACKBAR_SetPageSize (infoPtr, (LONG)lParam);

    case TBM_SETPOS:
        return TRACKBAR_SetPos (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGE:
        return TRACKBAR_SetRange (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGEMAX:
        return TRACKBAR_SetRangeMax (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGEMIN:
        return TRACKBAR_SetRangeMin (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSEL:
        return TRACKBAR_SetSel (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSELEND:
        return TRACKBAR_SetSelEnd (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSELSTART:
        return TRACKBAR_SetSelStart (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETTHUMBLENGTH:
        return TRACKBAR_SetThumbLength (infoPtr, (UINT)wParam);

    case TBM_SETTIC:
        return TRACKBAR_SetTic (infoPtr, (LONG)lParam);

    case TBM_SETTICFREQ:
        return TRACKBAR_SetTicFreq (infoPtr, (WORD)wParam);

    case TBM_SETTIPSIDE:
        return TRACKBAR_SetTipSide (infoPtr, (INT)wParam);

    case TBM_SETTOOLTIPS:
        return TRACKBAR_SetToolTips (infoPtr, (HWND)wParam);

    case TBM_SETUNICODEFORMAT:
	return TRACKBAR_SetUnicodeFormat (infoPtr, (BOOL)wParam);


    case WM_CAPTURECHANGED:
        return TRACKBAR_CaptureChanged (infoPtr);

    case WM_CREATE:
        return TRACKBAR_Create (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_DESTROY:
        return TRACKBAR_Destroy (infoPtr);

/*	case WM_ENABLE: */

/*	case WM_ERASEBKGND: */
/*	    return 0; */

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_KEYDOWN:
        return TRACKBAR_KeyDown (infoPtr, (INT)wParam, (DWORD)lParam);

    case WM_KEYUP:
        return TRACKBAR_KeyUp (infoPtr, (INT)wParam, (DWORD)lParam);

    case WM_KILLFOCUS:
        return TRACKBAR_KillFocus (infoPtr, (HWND)wParam);

    case WM_LBUTTONDOWN:
        return TRACKBAR_LButtonDown (infoPtr, wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_LBUTTONUP:
        return TRACKBAR_LButtonUp (infoPtr, wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_MOUSEMOVE:
        return TRACKBAR_MouseMove (infoPtr, wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_PAINT:
        return TRACKBAR_Paint (infoPtr, (HDC)wParam);

    case WM_SETFOCUS:
        return TRACKBAR_SetFocus (infoPtr, (HWND)wParam);

    case WM_SIZE:
        return TRACKBAR_Size (infoPtr, wParam, LOWORD(lParam), HIWORD(lParam));

/*	case WM_TIMER: */

    case WM_WININICHANGE:
        return TRACKBAR_InitializeThumb (infoPtr);

    default:
        if ((uMsg >= WM_USER) && (uMsg < WM_APP))
            ERR("unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void TRACKBAR_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)TRACKBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursorW (0, IDC_ARROWW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASSW;

    RegisterClassW (&wndClass);
}


void TRACKBAR_Unregister (void)
{
    UnregisterClassW (TRACKBAR_CLASSW, (HINSTANCE)NULL);
}
