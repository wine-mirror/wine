/*
 * Rebar control    rev 6e
 *
 * Copyright 1998, 1999 Eric Kohl
 *
 * NOTES
 *   An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - vertical placement
 *   - ComboBox and ComboBoxEx placement
 *   - center image 
 *   - Layout code.
 *   - Display code.
 *   - Some messages.
 *   - All notifications.

 * Changes Guy Albertelli <galberte@neo.lrun.com>
 *  rev 2,3,4
 *   - Implement initial version of row grouping, row separators,
 *     text and background colors. Support additional messages. 
 *     Support RBBS_BREAK. Implement ERASEBKGND and improve painting.
 *  rev 5
 *   - implement support for dragging Gripper left or right in a row. Supports
 *     WM_LBUTTONDOWN, WM_LBUTTONUP, and WM_MOUSEMOVE. Also support 
 *     RBS_BANDBORDERS.
 *  rev 6
 *  1. Make drawing of rebar attempt to match pixel by pixel with MS. (6a)
 *  2. Fix interlock between REBAR_ForceResize and REBAR_Size (AUTO_RESIZE flag) (6a)
 *  3. Fix up some of the notifications (RBN_HEIGHTCHANGE). (6a)
 *  4. Fix up DeleteBand by hiding child window. (6a)
 *  5. Change how band borders are drawn and rects are invalidated. (6b)
 *  6. Fix height of bar with only caption text. (6b)
 *  7. RBBS_NOGRIPPER trumps RBBS_GRIPPERALWAYS (gripper hidden), but 
 *     RBBS_GRIPPERALWAYS trumps RBBS_FIXEDSIZE (gripper displays). (6b)
 *  8. New algorithim for AdjustBand:
 *      For all bands in a row: bands left to right are sized to their "cx"
 *      size (if space available). Also use max of (cxHeader+cxMinChild) and
 *      cx. (6c)
 *  9. New alogrithim for Layout:
 *      Insert band in row if cxHeader space is available. If adjustment
 *      rect exists, then back out bands in row from last to second into
 *      separate rows till "y" adjustment rect equalled or exceeded. (6c)
 * 10. Implement vertical drag. (6c)
 * 11. Use DeferWindowPos to set child window position. (6c)
 * 12. Fixup RBN_CHILDSIZE notify. The rcBand rectangle should start
 *     after the header. (6d)
 * 13. Flags for DeferWindowPos seem to always be SWP_NOZORDER in the 
 *     traces. (6d)
 * 14. Make handling of ComboBox and ComboBoxEx the same in 
 *     _MoveChildWindow. (6d)
 * 15. Changes in phase 2 of _Layout for WinRAR example. (6e)
 * 16. Do notify change size of children for WM_SIZE message. (6e)
 * 17. Re-validate first band when second is added (for gripper add). (6e)
 *
 *
 *
 *    Still to do:
 *  1. default row height should be the max height of all visible bands
 *  2. Following still not handled: RBBS_FIXEDBMP, RBBS_CHILDEDGE,
 *            RBBS_USECHEVRON
 *  3. RBS_VARHEIGHT is assumed to always be on.
 *  4. RBBS_HIDDEN (and the CCS_VERT + RBBS_NOVERT case) is not really 
 *     supported by the following functions:
 *      _Layout (phase 2), _InternalEraseBkgnd, _InternalHitTest,
 *      _HandleLRDrag
 *  5. _HandleLRDrag does not properly position ComboBox or ComboBoxEx 
 *     children. (See code in _MoveChildWindow.)

 */

#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(rebar);

typedef struct
{
    UINT    fStyle;
    UINT    fMask;
    COLORREF  clrFore;
    COLORREF  clrBack;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;
    UINT    cyMinChild;
    UINT    cx;
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;
    UINT    cyMaxChild;
    UINT    cyIntegral;
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;

    UINT    lcx;            /* minimum cx for band */
    UINT    hcx;            /* maximum cx for band */
    UINT    lcy;            /* minimum cy for band */
    UINT    hcy;            /* maximum cy for band */

    SIZE    offChild;       /* x,y offset if child is not FIXEDSIZE */
    UINT    uMinHeight;
    INT     iRow;           /* row this band assigned to */
    UINT    fStatus;        /* status flags, reset only by _Validate */
    UINT    fDraw;          /* drawing flags, reset only by _Layout */
    RECT    rcBand;         /* calculated band rectangle */
    RECT    rcGripper;      /* calculated gripper rectangle */
    RECT    rcCapImage;     /* calculated caption image rectangle */
    RECT    rcCapText;      /* calculated caption text rectangle */
    RECT    rcChild;        /* calculated child rectangle */

    LPWSTR    lpText;
    HWND    hwndPrevParent;
} REBAR_BAND;

/* fStatus flags */
#define HAS_GRIPPER    0x00000001
#define HAS_IMAGE      0x00000002
#define HAS_TEXT       0x00000004

/* fDraw flags */
#define DRAW_GRIPPER    0x00000001
#define DRAW_IMAGE      0x00000002
#define DRAW_TEXT       0x00000004
#define DRAW_RIGHTSEP   0x00000010
#define DRAW_BOTTOMSEP  0x00000020
#define DRAW_SEPBOTH    (DRAW_RIGHTSEP | DRAW_BOTTOMSEP)
#define NTF_INVALIDATE  0x01000000
#define NTF_CHILDSIZE   0x02000000


typedef struct
{
    COLORREF   clrBk;       /* background color */
    COLORREF   clrText;     /* text color */
    HIMAGELIST himl;        /* handle to imagelist */
    UINT     uNumBands;   /* number of bands in the rebar */
    UINT     uNumRows;    /* number of rows of bands */
    HWND     hwndToolTip; /* handle to the tool tip control */
    HWND     hwndNotify;  /* notification window (parent) */
    HFONT    hFont;       /* handle to the rebar's font */
    SIZE     imageSize;   /* image size (image list) */

    SIZE     calcSize;    /* calculated rebar size */
    SIZE     oldSize;     /* previous calculated rebar size */
    BOOL     bUnicode;    /* Unicode flag */
    UINT     fStatus;     /* Status flags (see below)  */ 
    HCURSOR  hcurArrow;   /* handle to the arrow cursor */
    HCURSOR  hcurHorz;    /* handle to the EW cursor */
    HCURSOR  hcurVert;    /* handle to the NS cursor */
    HCURSOR  hcurDrag;    /* handle to the drag cursor */
    INT      iVersion;    /* version number */
    POINTS   dragStart;   /* x,y of button down */
    POINTS   dragNow;     /* x,y of this MouseMove */
    INT      ihitBand;    /* band number of band whose gripper was grabbed */
    INT      ihitoffset;  /* offset of hotspot from gripper.left */

    REBAR_BAND *bands;      /* pointer to the array of rebar bands */
} REBAR_INFO;

/* fStatus flags */
#define BEGIN_DRAG_ISSUED   1
#define AUTO_RESIZE         2
#define RESIZE_ANYHOW       4
#define NTF_HGHTCHG         8

/* ----   REBAR layout constants. Mostly determined by        ---- */
/* ----   experiment on WIN 98.                               ---- */

/* Width (or height) of separators between bands (either horz. or  */
/* vert.). True only if RBS_BANDBORDERS is set                     */
#define SEP_WIDTH_SIZE  2
#define SEP_WIDTH       ((dwStyle & RBS_BANDBORDERS) ? SEP_WIDTH_SIZE : 0)

/* Blank (background color) space between Gripper (if present)     */
/* and next item (image, text, or window). Always present          */
#define REBAR_ALWAYS_SPACE  4

/* Blank (background color) space after Image (if present).        */
#define REBAR_POST_IMAGE  2

/* Blank (background color) space after Text (if present).         */
#define REBAR_POST_TEXT  4

/* Height of vertical gripper in a CCS_VERT rebar.                 */
#define GRIPPER_HEIGHT  16

/* Blank (background color) space before Gripper (if present).     */
#define REBAR_PRE_GRIPPER   2

/* Width (of normal vertical gripper) or height (of horz. gripper) */
/* if present.                                                     */
#define GRIPPER_WIDTH  3

/* This is the increment that is used over the band height */
/* Determined by experiment.                               */ 
#define REBARSPACE      4

/* ----   End of REBAR layout constants.                      ---- */


/*  The following 4 defines return the proper rcBand element       */
/*  depending on whether CCS_VERT was set.                         */
#define rcBlt(b) ((dwStyle & CCS_VERT) ? b->rcBand.top : b->rcBand.left)
#define rcBrb(b) ((dwStyle & CCS_VERT) ? b->rcBand.bottom : b->rcBand.right)
#define ircBlt(b) ((dwStyle & CCS_VERT) ? b->rcBand.left : b->rcBand.top)
#define ircBrb(b) ((dwStyle & CCS_VERT) ? b->rcBand.right : b->rcBand.bottom)

/*  The following define determines if a given band is hidden      */
#define HIDDENBAND(a)  (((a)->fStyle & RBBS_HIDDEN) ||   \
                        ((dwStyle & CCS_VERT) &&         \
                         ((a)->fStyle & RBBS_NOVERT)))


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongA (hwnd, 0))


/* "constant values" retrieved when DLL was initialized    */
/* FIXME we do this when the classes are registered.       */
static UINT mindragx = 0;
static UINT mindragy = 0;


static VOID
REBAR_DumpBandInfo( LPREBARBANDINFOA pB)
{
	TRACE("band info: ID=%u, size=%u, style=0x%08x, mask=0x%08x, child=%04x\n",
	  pB->wID, pB->cbSize, pB->fStyle, pB->fMask, pB->hwndChild); 
	TRACE("band info: cx=%u, xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
          pB->cx, pB->cxMinChild, 
          pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
	TRACE("band info: xIdeal=%u, xHeader=%u, lParam=0x%08lx, clrF=0x%06lx, clrB=0x%06lx\n",
	  pB->cxIdeal, pB->cxHeader, pB->lParam, pB->clrFore, pB->clrBack);
}

static VOID
REBAR_DumpBand (HWND hwnd)
{
    REBAR_INFO *iP = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *pB;
    UINT i;

    if( TRACE_ON(rebar) ) {

      TRACE("hwnd=%04x: color=%08lx/%08lx, bands=%u, rows=%u, cSize=%ld,%ld\n", 
        hwnd, iP->clrText, iP->clrBk, iP->uNumBands, iP->uNumRows,
        iP->calcSize.cx, iP->calcSize.cy);
      TRACE("hwnd=%04x: flags=%08x, dragStart=%d,%d, dragNow=%d,%d, ihitBand=%d\n",
	    hwnd, iP->fStatus, iP->dragStart.x, iP->dragStart.y,
	    iP->dragNow.x, iP->dragNow.y,
	    iP->ihitBand);
      for (i = 0; i < iP->uNumBands; i++) {
	pB = &iP->bands[i];
	TRACE("band # %u: ID=%u, mask=0x%08x, style=0x%08x, child=%04x, row=%u\n",
	  i, pB->wID, pB->fMask, pB->fStyle, pB->hwndChild, pB->iRow);
	TRACE("band # %u: xMin=%u, yMin=%u, cx=%u, yChild=%u, yMax=%u, yIntgl=%u, uMinH=%u,\n",
	  i, pB->cxMinChild, pB->cyMinChild, pB->cx,
          pB->cyChild, pB->cyMaxChild, pB->cyIntegral, pB->uMinHeight);
	TRACE("band # %u: header=%u, lcx=%u, hcx=%u, lcy=%u, hcy=%u, offChild=%ld,%ld\n",
	  i, pB->cxHeader, pB->lcx, pB->hcx, pB->lcy, pB->hcy, pB->offChild.cx, pB->offChild.cy);
	TRACE("band # %u: fStatus=%08x, fDraw=%08x, Band=(%d,%d)-(%d,%d), Grip=(%d,%d)-(%d,%d)\n",
	  i, pB->fStatus, pB->fDraw,
	  pB->rcBand.left, pB->rcBand.top, pB->rcBand.right, pB->rcBand.bottom,
	  pB->rcGripper.left, pB->rcGripper.top, pB->rcGripper.right, pB->rcGripper.bottom);
	TRACE("band # %u: Img=(%d,%d)-(%d,%d), Txt=(%d,%d)-(%d,%d), Child=(%d,%d)-(%d,%d)\n",
	  i,
	  pB->rcCapImage.left, pB->rcCapImage.top, pB->rcCapImage.right, pB->rcCapImage.bottom,
	  pB->rcCapText.left, pB->rcCapText.top, pB->rcCapText.right, pB->rcCapText.bottom,
	  pB->rcChild.left, pB->rcChild.top, pB->rcChild.right, pB->rcChild.bottom);
      }

    }
}

static INT
REBAR_Notify (HWND hwnd, NMHDR *nmhdr, REBAR_INFO *infoPtr, UINT code)
{
    HWND parent, owner;

    parent = infoPtr->hwndNotify;
    if (!parent) {
        parent = GetParent (hwnd);
	owner = GetWindow (hwnd, GW_OWNER);
	if (owner) parent = owner;
    }
    nmhdr->idFrom = GetDlgCtrlID (hwnd);
    nmhdr->hwndFrom = hwnd;
    nmhdr->code = code;

    return SendMessageA (parent, WM_NOTIFY, (WPARAM) nmhdr->idFrom,
			 (LPARAM)nmhdr);
}

static INT
REBAR_Notify_NMREBAR (HWND hwnd, REBAR_INFO *infoPtr, UINT uBand, UINT code)
{
    NMREBAR notify_rebar;
    REBAR_BAND *lpBand;

    notify_rebar.dwMask = 0;
    if (uBand!=-1) {
	lpBand = &infoPtr->bands[uBand];
	if (lpBand->fMask & RBBIM_ID) {
	    notify_rebar.dwMask |= RBNM_ID;
	    notify_rebar.wID = lpBand->wID;
	}
	if (lpBand->fMask & RBBIM_LPARAM) {
	    notify_rebar.dwMask |= RBNM_LPARAM;
	    notify_rebar.lParam = lpBand->lParam;
	}
	if (lpBand->fMask & RBBIM_STYLE) {
	    notify_rebar.dwMask |= RBNM_STYLE;
	    notify_rebar.fStyle = lpBand->fStyle;
	}
    }
    notify_rebar.uBand = uBand;
    return REBAR_Notify (hwnd, (NMHDR *)&notify_rebar, infoPtr, code);
}

static VOID
REBAR_DrawBand (HDC hdc, REBAR_INFO *infoPtr, REBAR_BAND *lpBand, DWORD dwStyle)
{

    /* draw separators on both the bottom and right */
    if ((lpBand->fDraw & DRAW_SEPBOTH) == DRAW_SEPBOTH) {
        RECT rcSep;
	SetRect (&rcSep,
		 lpBand->rcBand.left,
		 lpBand->rcBand.top,
		 lpBand->rcBand.right + SEP_WIDTH_SIZE,
		 lpBand->rcBand.bottom + SEP_WIDTH_SIZE);
	DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_BOTTOMRIGHT);
	TRACE("drawing band separator both (%d,%d)-(%d,%d)\n",
	      rcSep.left, rcSep.top, rcSep.right, rcSep.bottom);
    }

    /* draw band separator between bands in a row */
    if ((lpBand->fDraw & DRAW_SEPBOTH) == DRAW_RIGHTSEP) {
        RECT rcSep;
	if (dwStyle & CCS_VERT) {
	    SetRect (&rcSep, 
		     lpBand->rcBand.left,
		     lpBand->rcBand.bottom,
		     lpBand->rcBand.right,
		     lpBand->rcBand.bottom + SEP_WIDTH_SIZE);
	    DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_BOTTOM);
	}
	else {
	    SetRect (&rcSep, 
		     lpBand->rcBand.right,
		     lpBand->rcBand.top,
		     lpBand->rcBand.right + SEP_WIDTH_SIZE,
		     lpBand->rcBand.bottom);
	    DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_RIGHT);
	}
	TRACE("drawing band separator right (%d,%d)-(%d,%d)\n",
	      rcSep.left, rcSep.top, rcSep.right, rcSep.bottom);
    }

    /* draw band separator between rows */
    if ((lpBand->fDraw & DRAW_SEPBOTH) == DRAW_BOTTOMSEP) {
        RECT rcRowSep;
        if (dwStyle & CCS_VERT) {
	    SetRect (&rcRowSep,
		     lpBand->rcBand.right,
		     lpBand->rcBand.top,
		     lpBand->rcBand.right + SEP_WIDTH_SIZE,
		     lpBand->rcBand.bottom);
	    DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_RIGHT);
	}
	else {
	    SetRect (&rcRowSep, 
		     lpBand->rcBand.left,
		     lpBand->rcBand.bottom,
		     lpBand->rcBand.right,
		     lpBand->rcBand.bottom + SEP_WIDTH_SIZE);
	    DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_BOTTOM);
	}
	TRACE ("drawing band separator bottom (%d,%d)-(%d,%d)\n",
	       rcRowSep.left, rcRowSep.top,
	       rcRowSep.right, rcRowSep.bottom);
    }

    /* draw gripper */
    if (lpBand->fDraw & DRAW_GRIPPER)
        DrawEdge (hdc, &lpBand->rcGripper, BDR_RAISEDINNER, BF_RECT | BF_MIDDLE);

    /* draw caption image */
    if (lpBand->fDraw & DRAW_IMAGE) {
	POINT pt;

	/* center image */
	pt.y = (lpBand->rcCapImage.bottom + lpBand->rcCapImage.top - infoPtr->imageSize.cy)/2;
	pt.x = (lpBand->rcCapImage.right + lpBand->rcCapImage.left - infoPtr->imageSize.cx)/2;

	ImageList_Draw (infoPtr->himl, lpBand->iImage, hdc,
			pt.x, pt.y,
			ILD_TRANSPARENT);
    }

    /* draw caption text */
    if (lpBand->fDraw & DRAW_TEXT) {
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	INT oldBkMode = SetBkMode (hdc, TRANSPARENT);
	COLORREF oldcolor = CLR_NONE;
	if (lpBand->clrFore != CLR_NONE)
	    oldcolor = SetTextColor (hdc, lpBand->clrFore);
	DrawTextW (hdc, lpBand->lpText, -1, &lpBand->rcCapText,
		   DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (oldBkMode != TRANSPARENT)
	    SetBkMode (hdc, oldBkMode);
	if (lpBand->clrFore != CLR_NONE)
	    SetTextColor (hdc, oldcolor);
	SelectObject (hdc, hOldFont);
    }
}


static VOID
REBAR_Refresh (HWND hwnd, HDC hdc)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    UINT i, oldrow;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    oldrow = infoPtr->bands[0].iRow;
    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((dwStyle & CCS_VERT) &&
	     (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	/* now draw the band */
	REBAR_DrawBand (hdc, infoPtr, lpBand, dwStyle);

    }
}

static void
REBAR_AdjustBands (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT maxx, INT usedx, INT mcy, DWORD dwStyle)
     /* Function: This routine distributes the extra space in a row */
     /*  by increasing bands from left to right to their "cx" width.*/
     /*  Uses "cxHeader"+"cxMinChild" if it is bigger than "cx".    */
{
    REBAR_BAND *lpBand;
    UINT i;
    INT j, k;
    INT incr, current_width, lastx=0;
    RECT oldband;

    TRACE("start=%u, end=%u, max x=%d, used x=%d, max y=%d\n",
	  rowstart, rowend-1, maxx, usedx, mcy);

    incr = maxx - usedx;

    for (i = rowstart; i<rowend; i++) {
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	j = 0;
	k = 0;
	oldband = lpBand->rcBand;

	/* get the current width of the band */
	if (dwStyle & CCS_VERT)
	    current_width = lpBand->rcBand.bottom - lpBand->rcBand.top;
	else
	    current_width = lpBand->rcBand.right - lpBand->rcBand.left;

	/* compute (in "j") the adjustment for this band */
	/* FIXME ??? should this not use "cx" and "cxHeader" and "cxMinChild" */
	if (!(lpBand->fStyle & RBBS_FIXEDSIZE)) {
	    if ((lpBand->fMask & RBBIM_SIZE) && (lpBand->cx > 0))
	        j = min(lpBand->cx - current_width, incr);
	    if ((lpBand->fMask & RBBIM_CHILDSIZE) &&
		(lpBand->cxMinChild > 0) &&
		(lpBand->fMask & RBBIM_CHILD) &&
		(lpBand->hwndChild)) {
	        k = lpBand->cxHeader + lpBand->cxMinChild - current_width;
		if (k > 0) {
		    j = max(k, j);
		    j = min(j, incr);
		}
	    }
	}

	incr -= j;

	/* validate values */
	if (incr < 0) {
	    ERR("failed, incr=%d, current_width=%d, j=%d, k=%d\n",
		incr, current_width, j, k);
	    j -= incr;
	    incr = 0;
	}
	if (lastx + j + current_width > maxx) {
	    ERR("exceeded maximum, lastx=%d, j=%d, current_width=%d\n",
		lastx, j, current_width);
	    j = maxx - lastx - current_width;
	    incr = 0;
	}

	/* adjust the band rectangle for adding width  */
	/* and setting height of all bands in row.     */
	if (dwStyle & CCS_VERT) {
	    lpBand->rcBand.top = lastx;
	    lpBand->rcBand.bottom = lastx + j + current_width;
	    if ((lpBand->rcBand.top != oldband.top) ||
		(lpBand->rcBand.bottom != oldband.bottom))
	        lpBand->fDraw |= NTF_INVALIDATE;
	    if (lpBand->rcBand.right != lpBand->rcBand.left + mcy) {
	        lpBand->rcBand.right = lpBand->rcBand.left + mcy;
		lpBand->fDraw |= NTF_INVALIDATE;
	    }
	}
	else {
	    lpBand->rcBand.left = lastx;
	    lpBand->rcBand.right = lastx + j + current_width;
	    if ((lpBand->rcBand.left != oldband.left) ||
		(lpBand->rcBand.right != oldband.right))
	        lpBand->fDraw |= NTF_INVALIDATE;
	    if (lpBand->rcBand.bottom != lpBand->rcBand.top + mcy) {
	        lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;
		lpBand->fDraw |= NTF_INVALIDATE;
	    }
	}

	/* update to the next band start */
	lastx += (j + current_width + SEP_WIDTH);
	if (j) {
	    TRACE("band %d row=%d: changed to (%d,%d)-(%d,%d)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom);
	}
	else {
	    TRACE("band %d row=%d: unchanged (%d,%d)-(%d,%d)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom);
	}
    }

    /* if any remaining space then add to the rowstart band */
    if (incr > 0) {
        lpBand = &infoPtr->bands[rowstart];
	lpBand->rcBand.right += incr;
	TRACE("band %d row=%d: extended to (%d,%d)-(%d,%d)\n",
	      rowstart, lpBand->iRow,
	      lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom);
	for (i=rowstart+1; i<rowend; i++) {
	    lpBand = &infoPtr->bands[i];
	    if (HIDDENBAND(lpBand)) continue;
	    lpBand->rcBand.left += incr;
	    lpBand->rcBand.right += incr;
	    lpBand->fDraw |= NTF_INVALIDATE;
	}
    }
}

static void
REBAR_CalcHorzBand (HWND hwnd, REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify, DWORD dwStyle)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Horizontal bars. ***                   */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    HWND parenthwnd;
    RECT oldChild, work;

    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (hwnd);

    for(i=rstart; i<rend; i++){
      lpBand = &infoPtr->bands[i];
      if (HIDDENBAND(lpBand)) {
          SetRect (&lpBand->rcChild,
		   lpBand->rcBand.right, lpBand->rcBand.top,
		   lpBand->rcBand.right, lpBand->rcBand.bottom);
	  continue;
      }

      oldChild = lpBand->rcChild;

      /* set initial gripper rectangle */
      SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
	       lpBand->rcBand.left, lpBand->rcBand.bottom);

      /* calculate gripper rectangle */
      if ( lpBand->fStatus & HAS_GRIPPER) {
	  lpBand->fDraw |= DRAW_GRIPPER;
	  lpBand->rcGripper.left   += REBAR_PRE_GRIPPER;
	  lpBand->rcGripper.right  = lpBand->rcGripper.left + GRIPPER_WIDTH;
	  lpBand->rcGripper.top    += 2;
	  lpBand->rcGripper.bottom -= 2;

	  SetRect (&lpBand->rcCapImage,
		   lpBand->rcGripper.right+REBAR_ALWAYS_SPACE, lpBand->rcBand.top,
		   lpBand->rcGripper.right+REBAR_ALWAYS_SPACE, lpBand->rcBand.bottom);
      }
      else {  /* no gripper will be drawn */
	  xoff = 0;
	  if (lpBand->fStatus & (HAS_IMAGE | HAS_TEXT))
	      /* if no gripper but either image or text, then leave space */
	      xoff = REBAR_ALWAYS_SPACE;
	  SetRect (&lpBand->rcCapImage, 
		   lpBand->rcBand.left+xoff, lpBand->rcBand.top,
		   lpBand->rcBand.left+xoff, lpBand->rcBand.bottom);
      }

      /* image is visible */
      if (lpBand->fStatus & HAS_IMAGE) {
	  lpBand->fDraw |= DRAW_IMAGE;
	  lpBand->rcCapImage.right  += infoPtr->imageSize.cx;
	  lpBand->rcCapImage.bottom = lpBand->rcCapImage.top + infoPtr->imageSize.cy;

	  /* set initial caption text rectangle */
	  SetRect (&lpBand->rcCapText,
		   lpBand->rcCapImage.right+REBAR_POST_IMAGE, lpBand->rcBand.top+1,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.bottom-1);
	  /* update band height 
	  if (lpBand->uMinHeight < infoPtr->imageSize.cy + 2) {
	      lpBand->uMinHeight = infoPtr->imageSize.cy + 2;
	      lpBand->rcBand.bottom = lpBand->rcBand.top + lpBand->uMinHeight;
	  }  */
      }
      else {
	  /* set initial caption text rectangle */
	  SetRect (&lpBand->rcCapText, lpBand->rcCapImage.right, lpBand->rcBand.top+1,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.bottom-1);
      }

      /* text is visible */
      if (lpBand->fStatus & HAS_TEXT) {
	  lpBand->fDraw |= DRAW_TEXT;
	  lpBand->rcCapText.right = max(lpBand->rcCapText.left, 
					lpBand->rcCapText.right-REBAR_POST_TEXT);
      }

      /* set initial child window rectangle if there is a child */
      if (lpBand->fMask & RBBIM_CHILD) {
	  xoff = lpBand->offChild.cx;
	  yoff = lpBand->offChild.cy;
	  SetRect (&lpBand->rcChild,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.top+yoff,
		   lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
      }
      else {
          SetRect (&lpBand->rcChild,
		   lpBand->rcBand.left+lpBand->cxHeader, lpBand->rcBand.top,
		   lpBand->rcBand.right, lpBand->rcBand.bottom);
      }

      /* flag if notify required and invalidate rectangle */
      if (notify && 
	  ((oldChild.right-oldChild.left != lpBand->rcChild.right-lpBand->rcChild.left) ||
	   (oldChild.bottom-oldChild.top != lpBand->rcChild.bottom-lpBand->rcChild.top))) {
	  TRACE("Child rectangle changed for band %u\n", i);
	  TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		oldChild.left, oldChild.top,
	        oldChild.right, oldChild.bottom,
		lpBand->rcChild.left, lpBand->rcChild.top,
	        lpBand->rcChild.right, lpBand->rcChild.bottom);
	  lpBand->fDraw |= NTF_CHILDSIZE;
      }
      if (lpBand->fDraw & NTF_INVALIDATE) {
	  TRACE("invalidating (%d,%d)-(%d,%d)\n",
		lpBand->rcBand.left, 
		lpBand->rcBand.top,
		lpBand->rcBand.right + ((lpBand->fDraw & DRAW_RIGHTSEP) ? SEP_WIDTH_SIZE : 0), 
		lpBand->rcBand.bottom + ((lpBand->fDraw & DRAW_BOTTOMSEP) ? SEP_WIDTH_SIZE : 0));
	  lpBand->fDraw &= ~NTF_INVALIDATE;
	  work = lpBand->rcBand;
	  if (lpBand->fDraw & DRAW_RIGHTSEP) work.right += SEP_WIDTH_SIZE;
	  if (lpBand->fDraw & DRAW_BOTTOMSEP) work.bottom += SEP_WIDTH_SIZE;
	  InvalidateRect(hwnd, &work, TRUE);
      }

    }

}


static VOID
REBAR_CalcVertBand (HWND hwnd, REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify, DWORD dwStyle)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Vertical bars. ***                     */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    NMREBARCHILDSIZE  rbcz;
    HWND parenthwnd;
    RECT oldChild, work;

    rbcz.hdr.hwndFrom = hwnd;
    rbcz.hdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (hwnd);

    for(i=rstart; i<rend; i++){
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	oldChild = lpBand->rcChild;

	/* set initial gripper rectangle */
	SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
		 lpBand->rcBand.right, lpBand->rcBand.top);

	/* calculate gripper rectangle */
	if (lpBand->fStatus & HAS_GRIPPER) {
	    lpBand->fDraw |= DRAW_GRIPPER;

	    if (dwStyle & RBS_VERTICALGRIPPER) {
		/*  vertical gripper  */
		lpBand->rcGripper.left   += 3;
		lpBand->rcGripper.right  = lpBand->rcGripper.left + GRIPPER_WIDTH;
		lpBand->rcGripper.top    += REBAR_PRE_GRIPPER;
		lpBand->rcGripper.bottom = lpBand->rcGripper.top + GRIPPER_HEIGHT;

		/* initialize Caption image rectangle  */
		SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 lpBand->rcBand.right,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE);
	    }
	    else {
		/*  horizontal gripper  */
		lpBand->rcGripper.left   += 3;
		lpBand->rcGripper.right  -= 3;
		lpBand->rcGripper.top    += REBAR_PRE_GRIPPER;
		lpBand->rcGripper.bottom  = lpBand->rcGripper.top + GRIPPER_WIDTH;

		/* initialize Caption image rectangle  */
		SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE,
			 lpBand->rcBand.right,
			 lpBand->rcGripper.bottom + REBAR_ALWAYS_SPACE);
	    }
	}
	else {  /* no gripper will be drawn */
	    xoff = 0;
	    if (lpBand->fStatus & (HAS_IMAGE | HAS_TEXT))
		/* if no gripper but either image or text, then leave space */
		xoff = REBAR_ALWAYS_SPACE;
	    /* initialize Caption image rectangle  */
	    SetRect (&lpBand->rcCapImage, 
		     lpBand->rcBand.left, lpBand->rcBand.top+xoff,
		     lpBand->rcBand.right, lpBand->rcBand.top+xoff);
	}

	/* image is visible */
	if (lpBand->fStatus & HAS_IMAGE) {
	    lpBand->fDraw |= DRAW_IMAGE;

	    lpBand->rcCapImage.right  = lpBand->rcCapImage.left + infoPtr->imageSize.cx;
	    lpBand->rcCapImage.bottom += infoPtr->imageSize.cy;

	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText, 
		     lpBand->rcBand.left, lpBand->rcCapImage.bottom+REBAR_POST_IMAGE,
		     lpBand->rcBand.right, lpBand->rcBand.top+lpBand->cxHeader);
	    /* update band height *
	       if (lpBand->uMinHeight < infoPtr->imageSize.cx + 2) {
	       lpBand->uMinHeight = infoPtr->imageSize.cx + 2;
	       lpBand->rcBand.right = lpBand->rcBand.left + lpBand->uMinHeight;
	       } */
	}
	else {
	    /* set initial caption text rectangle */
	    SetRect (&lpBand->rcCapText, 
		     lpBand->rcBand.left, lpBand->rcCapImage.bottom,
		     lpBand->rcBand.right, lpBand->rcBand.top+lpBand->cxHeader);
	}

	/* text is visible */
	if (lpBand->fStatus & HAS_TEXT) {
	    lpBand->fDraw |= DRAW_TEXT;
	    lpBand->rcCapText.bottom = max(lpBand->rcCapText.top,
					   lpBand->rcCapText.bottom-REBAR_POST_TEXT);
	}

	/* set initial child window rectangle if there is a child */
	if (lpBand->fMask & RBBIM_CHILD) {
	    yoff = lpBand->offChild.cx;
	    xoff = lpBand->offChild.cy;
	    SetRect (&lpBand->rcChild,
		     lpBand->rcBand.left+xoff, lpBand->rcBand.top+lpBand->cxHeader,
		     lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
	}
	else {
	    SetRect (&lpBand->rcChild,
		     lpBand->rcBand.left, lpBand->rcBand.top+lpBand->cxHeader,
		     lpBand->rcBand.right, lpBand->rcBand.bottom);
	}

	/* flag if notify required and invalidate rectangle */
	if (notify && 
	    ((oldChild.right-oldChild.left != lpBand->rcChild.right-lpBand->rcChild.left) ||
	     (oldChild.bottom-oldChild.top != lpBand->rcChild.bottom-lpBand->rcChild.top))) {
	    TRACE("Child rectangle changed for band %u\n", i);
	    TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		  oldChild.left, oldChild.top,
		  oldChild.right, oldChild.bottom,
		  lpBand->rcChild.left, lpBand->rcChild.top,
		  lpBand->rcChild.right, lpBand->rcChild.bottom);
	    lpBand->fDraw |= NTF_CHILDSIZE;
	}
	if (lpBand->fDraw & NTF_INVALIDATE) {
	    TRACE("invalidating (%d,%d)-(%d,%d)\n",
		  lpBand->rcBand.left, 
		  lpBand->rcBand.top,
		  lpBand->rcBand.right + ((lpBand->fDraw & DRAW_BOTTOMSEP) ? SEP_WIDTH_SIZE : 0), 
		  lpBand->rcBand.bottom + ((lpBand->fDraw & DRAW_RIGHTSEP) ? SEP_WIDTH_SIZE : 0));
	    lpBand->fDraw &= ~NTF_INVALIDATE;
	    work = lpBand->rcBand;
	    if (lpBand->fDraw & DRAW_RIGHTSEP) work.bottom += SEP_WIDTH_SIZE;
	    if (lpBand->fDraw & DRAW_BOTTOMSEP) work.right += SEP_WIDTH_SIZE;
	    InvalidateRect(hwnd, &work, TRUE);
	}

    }
}


static VOID
REBAR_Layout (HWND hwnd, LPRECT lpRect, BOOL notify, BOOL resetclient)
     /* Function: This routine is resposible for laying out all */
     /*  the bands in a rebar. It assigns each band to a row and*/
     /*  determines when to start a new row.                    */
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    REBAR_BAND *lpBand, *prevBand;
    RECT rcClient, rcAdj, rcoldBand;
    INT x, y, cx, cxsep, mcy, clientcx, clientcy;
    INT adjcx, adjcy, row, rightx, bottomy, origheight;
    UINT i, rowstartband;
    BOOL dobreak;

    GetClientRect (hwnd, &rcClient);
    TRACE("Client is (%d,%d)-(%d,%d)\n",
	  rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

    if (lpRect) {
	rcAdj = *lpRect;
	TRACE("adjustment rect is (%d,%d)-(%d,%d)\n",
	      rcAdj.left, rcAdj.top, rcAdj.right, rcAdj.bottom);
    }
    else {
        CopyRect (&rcAdj, &rcClient);
    }

    clientcx = rcClient.right - rcClient.left;
    clientcy = rcClient.bottom - rcClient.top;
    adjcx = rcAdj.right - rcAdj.left;
    adjcy = rcAdj.bottom - rcAdj.top;
    if (resetclient) {
        TRACE("window client rect will be set to adj rect\n");
        clientcx = adjcx;
        clientcy = adjcy;
    }

    /* save height of original control */
    if (dwStyle & CCS_VERT) 
        origheight = infoPtr->calcSize.cx;
    else
        origheight = infoPtr->calcSize.cy;


    /* ******* Start Phase 1 - all bands on row at minimum size ******* */

    x = 0;
    y = 0;
    row = 1;
    cx = 0;
    mcy = 0;
    prevBand = NULL;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	lpBand->fDraw = 0;
	lpBand->iRow = row;

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((dwStyle & CCS_VERT) && (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	rcoldBand = lpBand->rcBand;

	/* separator from previous band */
	cxsep = ( ((dwStyle & CCS_VERT) ? y : x)==0) ? 0 : SEP_WIDTH;  

	/* Header: includes gripper, text, image */
	cx = lpBand->cxHeader;   
	if (lpBand->fStyle & RBBS_FIXEDSIZE) cx += lpBand->lcx;

	if (dwStyle & CCS_VERT)
	    dobreak = (y + cx + cxsep > adjcy);
        else
	    dobreak = (x + cx + cxsep > adjcx);

	/* This is the check for whether we need to start a new row */
	if ( ( (lpBand->fStyle & RBBS_BREAK) && (i != 0) ) ||
	     ( ((dwStyle & CCS_VERT) ? (y != 0) : (x != 0)) && dobreak)) {
	    TRACE("Spliting to new row %d on band %u\n", row+1, i);
	    if (dwStyle & CCS_VERT) {
		y = 0;
		x += (mcy + SEP_WIDTH);
	    }
	    else {
		x = 0;
		y += (mcy + SEP_WIDTH);
	    }

	    /* FIXME: if not RBS_VARHEIGHT then find max */
	    mcy = 0;
	    cxsep = 0;
	    row++;
	    lpBand->iRow = row;
	    prevBand = NULL;
	}

	if (mcy < lpBand->lcy + REBARSPACE) mcy = lpBand->lcy + REBARSPACE;

	/* if boundary rect specified then limit mcy */
	if (lpRect) {
	    if (dwStyle & CCS_VERT) {
	        if (x+mcy > adjcx) {
		    mcy = adjcx - x;
		    TRACE("row %u limiting mcy=%d, adjcx=%d, x=%d\n",
			  i, mcy, adjcx, x);
		}
	    }
	    else {
	        if (y+mcy > adjcy) {
		    mcy = adjcy - y;
		    TRACE("row %u limiting mcy=%d, adjcy=%d, y=%d\n",
			  i, mcy, adjcy, y);
		}
	    }
	}

	if (dwStyle & CCS_VERT) {
	    /* bound the bottom side if we have a bounding rectangle */
	    if ((x>0) && (dwStyle & RBS_BANDBORDERS) && prevBand)
	        prevBand->fDraw |= DRAW_RIGHTSEP;
	    rightx = clientcx;
	    bottomy = (lpRect) ? min(clientcy, y+cxsep+cx) : y+cxsep+cx;
	    lpBand->rcBand.left   = x;
	    lpBand->rcBand.right  = x + min(mcy, lpBand->lcy+REBARSPACE);
	    lpBand->rcBand.top    = min(bottomy, y + cxsep);
	    lpBand->rcBand.bottom = bottomy;
	    lpBand->uMinHeight = lpBand->lcy;
	    if (!EqualRect(&rcoldBand, &lpBand->rcBand))
	        lpBand->fDraw |= NTF_INVALIDATE;
	    y = bottomy;
	}
	else {
	    /* bound the right side if we have a bounding rectangle */
	    if ((x>0) && (dwStyle & RBS_BANDBORDERS) && prevBand)
	        prevBand->fDraw |= DRAW_RIGHTSEP;
	    rightx = (lpRect) ? min(clientcx, x+cxsep+cx) : x+cxsep+cx;
	    bottomy = clientcy;
	    lpBand->rcBand.left   = min(rightx, x + cxsep);
	    lpBand->rcBand.right  = rightx;
	    lpBand->rcBand.top    = y;
	    lpBand->rcBand.bottom = y + min(mcy, lpBand->lcy+REBARSPACE);
	    lpBand->uMinHeight = lpBand->lcy;
	    if (!EqualRect(&rcoldBand, &lpBand->rcBand))
	        lpBand->fDraw |= NTF_INVALIDATE;
	    x = rightx;
	}
	TRACE("band %u, row %d, (%d,%d)-(%d,%d)\n",
	      i, row,
	      lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom);
	prevBand = lpBand;

    } /* for (i = 0; i < infoPtr->uNumBands... */

    if (dwStyle & CCS_VERT)
        x += mcy;
    else
        y += mcy;

    if (infoPtr->uNumBands)
        infoPtr->uNumRows = row;

    /* ******* End Phase 1 - all bands on row at minimum size ******* */


    /* ******* Start Phase 2 - split rows till adjustment height full ******* */

    /* assumes that the following variables contain:                 */
    /*   y/x     current height/width of all rows                    */
    if (lpRect) {
        INT i, j, prev_rh, current_rh, new_rh, adj_rh;
	REBAR_BAND *prev, *current, *walk;

/*	if (((dwStyle & CCS_VERT) ? (x < adjcx) : (y < adjcy)) && */

	if (((dwStyle & CCS_VERT) ? (adjcx - x > 4) : (adjcy - y > 4)) && 
	    (infoPtr->uNumBands > 1)) {
	    for (i=infoPtr->uNumBands-2; i>=0; i--) {
		TRACE("adjcx=%d, adjcy=%d, x=%d, y=%d\n",
		      adjcx, adjcy, x, y);
	        prev = &infoPtr->bands[i];
		prev_rh = ircBrb(prev) - ircBlt(prev);
		current = &infoPtr->bands[i+1];
		current_rh = ircBrb(current) - ircBlt(current);
		if (prev->iRow == current->iRow) {
		    new_rh = current->lcy + REBARSPACE;
		    adj_rh = prev_rh + new_rh + SEP_WIDTH - current_rh;
		    infoPtr->uNumRows++;
		    current->fDraw |= NTF_INVALIDATE;
		    current->iRow++;
		    if (dwStyle & CCS_VERT) {
		        current->rcBand.top = 0;
			current->rcBand.bottom = clientcy;
			current->rcBand.left += (prev_rh + SEP_WIDTH);
			current->rcBand.right = current->rcBand.left + new_rh;
			x += adj_rh;
		    }
		    else {
		        current->rcBand.left = 0;
			current->rcBand.right = clientcx;
			current->rcBand.top += (prev_rh + SEP_WIDTH);
			current->rcBand.bottom = current->rcBand.top + new_rh;
			y += adj_rh;
		    }
		    TRACE("moving band %d to own row at (%d,%d)-(%d,%d)\n",
			  i+1,
			  current->rcBand.left, current->rcBand.top,
			  current->rcBand.right, current->rcBand.bottom);
		    TRACE("prev band %d at (%d,%d)-(%d,%d)\n",
			  i,
			  prev->rcBand.left, prev->rcBand.top,
			  prev->rcBand.right, prev->rcBand.bottom);
		    TRACE("values: prev_rh=%d, current_rh=%d, new_rh=%d, adj_rh=%d\n",
			  prev_rh, current_rh, new_rh, adj_rh);
		    /* for bands below current adjust row # and top/bottom */
		    for (j = i+2; j<infoPtr->uNumBands; j++) {
		        walk = &infoPtr->bands[j];
			walk->fDraw |= NTF_INVALIDATE;
			walk->iRow++;
			if (dwStyle & CCS_VERT) {
			    walk->rcBand.left += adj_rh;
			    walk->rcBand.right += adj_rh;
			}
			else {
			    walk->rcBand.top += adj_rh;
			    walk->rcBand.bottom += adj_rh;
			}
		    }
		    if ((dwStyle & CCS_VERT) ? (x >= adjcx) : (y >= adjcy)) 
		        break; /* all done */
		}
	    }
	}
    }

    /* ******* End Phase 2 - split rows till adjustment height full ******* */



    /* ******* Start Phase 3 - adjust all bands for width full ******* */

    if (infoPtr->uNumBands) {
	REBAR_BAND *prev, *current;
	/* If RBS_BANDBORDERS set then indicate to draw bottom separator */
	if (dwStyle & RBS_BANDBORDERS) {
	    for (i = 0; i < infoPtr->uNumBands; i++) {
	        lpBand = &infoPtr->bands[i];
		if (HIDDENBAND(lpBand)) continue;
		if (lpBand->iRow < infoPtr->uNumRows) 
		    lpBand->fDraw |= DRAW_BOTTOMSEP;
	    }
	}

	/* Adjust the horizontal and vertical of each band */
	prev = &infoPtr->bands[0];
	current = prev;
	mcy = prev->lcy + REBARSPACE;
	rowstartband = 0;
	for (i=1; i<infoPtr->uNumBands; i++) {
	    prev = &infoPtr->bands[i-1];
	    current = &infoPtr->bands[i];
	    if (prev->iRow != current->iRow) {
	        REBAR_AdjustBands (infoPtr, rowstartband, i,
				   (dwStyle & CCS_VERT) ? clientcy : clientcx,
				   rcBrb(prev),
				   mcy, dwStyle);
		mcy = 0;
		rowstartband = i;
	    }
	    if (mcy < current->lcy + REBARSPACE)
	        mcy = current->lcy + REBARSPACE;
	}
	REBAR_AdjustBands (infoPtr, rowstartband, infoPtr->uNumBands,
			   (dwStyle & CCS_VERT) ? clientcy : clientcx,
			   rcBrb(current),
			   mcy, dwStyle);

	/* Calculate the other rectangles in each band */
	if (dwStyle & CCS_VERT) {
	    REBAR_CalcVertBand (hwnd, infoPtr, 0, infoPtr->uNumBands,
				notify, dwStyle);
	}
	else {
	    REBAR_CalcHorzBand (hwnd, infoPtr, 0, infoPtr->uNumBands, 
				notify, dwStyle);
	}
    }

    /* ******* End Phase 3 - adjust all bands for width full ******* */


    /* FIXME: if not RBS_VARHEIGHT then find max mcy and adj rect*/

    infoPtr->oldSize = infoPtr->calcSize;
    if (dwStyle & CCS_VERT) {
	infoPtr->calcSize.cx = x;
	infoPtr->calcSize.cy = clientcy;
	TRACE("vert, notify=%d, x=%d, origheight=%d\n",
	      notify, x, origheight);
	if (notify && (x != origheight)) infoPtr->fStatus |= NTF_HGHTCHG;
    }
    else {
	infoPtr->calcSize.cx = clientcx;
	infoPtr->calcSize.cy = y;
	TRACE("horz, notify=%d, y=%d, origheight=%d\n",
	      notify, y, origheight);
	if (notify && (y != origheight)) infoPtr->fStatus |= NTF_HGHTCHG; 
    }
    REBAR_DumpBand (hwnd);
}


static VOID
REBAR_ForceResize (HWND hwnd)
     /* Function: This changes the size of the REBAR window to that */
     /*  calculated by REBAR_Layout.                                */
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rc;

    TRACE( " from [%ld x %ld] to [%ld x %ld]!\n",
	   infoPtr->oldSize.cx, infoPtr->oldSize.cy,
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy);

    /* if size did not change then skip process */
    if ((infoPtr->oldSize.cx == infoPtr->calcSize.cx) &&
	(infoPtr->oldSize.cy == infoPtr->calcSize.cy) &&
	!(infoPtr->fStatus & RESIZE_ANYHOW))
        return;

    infoPtr->fStatus &= ~RESIZE_ANYHOW;
    /* Set flag to ignore next WM_SIZE message */
    infoPtr->fStatus |= AUTO_RESIZE;

    rc.left = 0;
    rc.top = 0;
    rc.right  = infoPtr->calcSize.cx;
    rc.bottom = infoPtr->calcSize.cy;

    if (GetWindowLongA (hwnd, GWL_STYLE) & WS_BORDER) {
	InflateRect (&rc, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    }

    SetWindowPos (hwnd, 0, 0, 0,
		    rc.right - rc.left, rc.bottom - rc.top,
		    SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}


static VOID
REBAR_MoveChildWindows (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    CHAR szClassName[40];
    UINT i;
    NMREBARCHILDSIZE  rbcz;
    NMHDR heightchange;
    HDWP deferpos;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    if (!(deferpos = BeginDeferWindowPos(8)))
        ERR("BeginDeferWindowPso returned NULL\n");

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %x\n", lpBand->hwndChild);

	    if (lpBand->fDraw & NTF_CHILDSIZE) {
	        lpBand->fDraw &= ~NTF_CHILDSIZE;
		rbcz.uBand = i;
		rbcz.wID = lpBand->wID;
		rbcz.rcChild = lpBand->rcChild;
		rbcz.rcBand = lpBand->rcBand;
		rbcz.rcBand.left += lpBand->cxHeader;
		REBAR_Notify (hwnd, (NMHDR *)&rbcz, infoPtr, RBN_CHILDSIZE);
		if (!EqualRect (&lpBand->rcChild, &rbcz.rcChild)) {
		    TRACE("Child rect changed by NOTIFY for band %u\n", i);
		    TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
			  lpBand->rcChild.left, lpBand->rcChild.top,
			  lpBand->rcChild.right, lpBand->rcChild.bottom,
			  rbcz.rcChild.left, rbcz.rcChild.top,
			  rbcz.rcChild.right, rbcz.rcChild.bottom);
		}
	    }

	    GetClassNameA (lpBand->hwndChild, szClassName, 40);
	    if (!lstrcmpA (szClassName, "ComboBox") ||
		!lstrcmpA (szClassName, WC_COMBOBOXEXA)) {
		INT nEditHeight, yPos;
		RECT rc;

		/* special placement code for combo or comboex box */


		/* get size of edit line */
		GetWindowRect (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		TRACE("moving child (Combo(Ex)) %04x to (%d,%d)-(%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, yPos,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      nEditHeight);
		deferpos = DeferWindowPos (deferpos, lpBand->hwndChild, HWND_TOP,
					   lpBand->rcChild.left,
					   /*lpBand->rcChild.top*/ yPos,
					   lpBand->rcChild.right - lpBand->rcChild.left,
					   nEditHeight,
					   SWP_NOZORDER);
		if (!deferpos)
		    ERR("DeferWindowPos returned NULL\n");
	    }
	    else {
		TRACE("moving child (Other) %04x to (%d,%d)-(%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, lpBand->rcChild.top,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      lpBand->rcChild.bottom - lpBand->rcChild.top);
		deferpos = DeferWindowPos (deferpos, lpBand->hwndChild, HWND_TOP,
					   lpBand->rcChild.left,
					   lpBand->rcChild.top,
					   lpBand->rcChild.right - lpBand->rcChild.left,
					   lpBand->rcChild.bottom - lpBand->rcChild.top,
					   SWP_NOZORDER);
		if (!deferpos)
		    ERR("DeferWindowPos returned NULL\n");
	    }
	}
    }
    if (!EndDeferWindowPos(deferpos))
        ERR("EndDeferWindowPos returned NULL\n");
    if (infoPtr->fStatus & NTF_HGHTCHG) {
        infoPtr->fStatus &= ~NTF_HGHTCHG;
        REBAR_Notify (hwnd, &heightchange, infoPtr, RBN_HEIGHTCHANGE);
    }
}


static VOID
REBAR_ValidateBand (HWND hwnd, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
     /* Function:  This routine evaluates the band specs supplied */
     /*  by the user and updates the following 5 fields in        */
     /*  the internal band structure: cxHeader, lcx, lcy, hcx, hcy*/
{
    UINT header=0;
    UINT textheight=0;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    lpBand->fStatus = 0;
    lpBand->lcx = 0;
    lpBand->lcy = 0;
    lpBand->hcx = 0;
    lpBand->hcy = 0;

    /* Data comming in from users into the cx... and cy... fields  */
    /* may be bad, just garbage, because the user never clears     */
    /* the fields. RB_{SET|INSERT}BAND{A|W} just passes the data   */
    /* along if the fields exist in the input area. Here we must   */
    /* determine if the data is valid. I have no idea how MS does  */
    /* the validation, but it does because the RB_GETBANDINFO      */
    /* returns a 0 when I know the sample program passed in an     */
    /* address. Here I will use the algorithim that if the value   */
    /* is greater than 65535 then it is bad and replace it with    */
    /* a zero. Feel free to improve the algorithim.  -  GA 12/2000 */
    if (lpBand->cxMinChild > 65535) lpBand->cxMinChild = 0;
    if (lpBand->cyMinChild > 65535) lpBand->cyMinChild = 0;
    if (lpBand->cx         > 65535) lpBand->cx         = 0;
    if (lpBand->cyChild    > 65535) lpBand->cyChild    = 0;
    if (lpBand->cyMaxChild > 65535) lpBand->cyMaxChild = 0;
    if (lpBand->cyIntegral > 65535) lpBand->cyIntegral = 0;
    if (lpBand->cxIdeal    > 65535) lpBand->cxIdeal    = 0;
    if (lpBand->cxHeader   > 65535) lpBand->cxHeader   = 0;

    /* Header is where the image, text and gripper exist  */
    /* in the band and preceed the child window.          */

    /* calculate gripper rectangle */
    if (  (!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	  ( (lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	    ( !(lpBand->fStyle & RBBS_FIXEDSIZE) && (infoPtr->uNumBands > 1)))
       ) {
	lpBand->fStatus |= HAS_GRIPPER;
        if (dwStyle & CCS_VERT)
	    if (dwStyle & RBS_VERTICALGRIPPER)
                header += (GRIPPER_HEIGHT + REBAR_PRE_GRIPPER);
            else
	        header += (GRIPPER_WIDTH + REBAR_PRE_GRIPPER);
        else
            header += (REBAR_PRE_GRIPPER + GRIPPER_WIDTH);
        /* Always have 4 pixels before anything else */
        header += REBAR_ALWAYS_SPACE;
    }

    /* image is visible */
    if ((lpBand->fMask & RBBIM_IMAGE) && (infoPtr->himl)) {
	lpBand->fStatus |= HAS_IMAGE;
        if (dwStyle & CCS_VERT) {
	   header += (infoPtr->imageSize.cy + REBAR_POST_IMAGE);
	   lpBand->lcy = infoPtr->imageSize.cx + 2;
	}
	else {
	   header += (infoPtr->imageSize.cx + REBAR_POST_IMAGE);
	   lpBand->lcy = infoPtr->imageSize.cy + 2;
	}
    }

    /* text is visible */
    if ((lpBand->fMask & RBBIM_TEXT) && (lpBand->lpText)) {
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fStatus |= HAS_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	header += ((dwStyle & CCS_VERT) ? (size.cy + REBAR_POST_TEXT) : (size.cx + REBAR_POST_TEXT));
	textheight = (dwStyle & CCS_VERT) ? 0 : size.cy;

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
    }

    /* if no gripper but either image or text, then leave space */
    if ((lpBand->fStatus & (HAS_IMAGE | HAS_TEXT)) &&
	!(lpBand->fStatus & HAS_GRIPPER)) {
	header += REBAR_ALWAYS_SPACE;
    }

    /* check if user overrode the header value */
    if (!(lpBand->fMask & RBBIM_HEADERSIZE))
        lpBand->cxHeader = header;


    /* Now compute minimum size of child window */
    lpBand->offChild.cx = 0;
    lpBand->offChild.cy = 0;
    lpBand->lcy = textheight;
    if (lpBand->fMask & RBBIM_CHILDSIZE) {
	if (!(lpBand->fStyle & RBBS_FIXEDSIZE)) {
	    lpBand->offChild.cx = 4;
	    lpBand->offChild.cy = 2;
        }
        lpBand->lcx = lpBand->cxMinChild;
        lpBand->lcy = max(lpBand->lcy, lpBand->cyMinChild);
        lpBand->hcy = lpBand->lcy;
        if (lpBand->fStyle & RBBS_VARIABLEHEIGHT) {
	    if (lpBand->cyChild != 0xffffffff)
	        lpBand->lcy = max (lpBand->cyChild, lpBand->lcy);
	    lpBand->hcy = lpBand->cyMaxChild;
        }
        TRACE("_CHILDSIZE\n");
    }
    if (lpBand->fMask & RBBIM_SIZE) {
        lpBand->hcx = max (lpBand->cx, lpBand->lcx);
        TRACE("_SIZE\n");
    }
    else
        lpBand->hcx = lpBand->lcx;

}

static void
REBAR_CommonSetupBand (HWND hwnd, LPREBARBANDINFOA lprbbi, REBAR_BAND *lpBand)
     /* Function:  This routine copies the supplied values from   */
     /*  user input (lprbbi) to the internal band structure.      */
{
    lpBand->fMask |= lprbbi->fMask;

    if (lprbbi->fMask & RBBIM_STYLE)
	lpBand->fStyle = lprbbi->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lpBand->clrFore = lprbbi->clrFore;
	lpBand->clrBack = lprbbi->clrBack;
    }

    if (lprbbi->fMask & RBBIM_IMAGE)
	lpBand->iImage = lprbbi->iImage;

    if (lprbbi->fMask & RBBIM_CHILD) {
	if (lprbbi->hwndChild) {
	    lpBand->hwndChild = lprbbi->hwndChild;
	    lpBand->hwndPrevParent =
		SetParent (lpBand->hwndChild, hwnd);
	}
	else {
	    TRACE("child: 0x%x  prev parent: 0x%x\n",
		   lpBand->hwndChild, lpBand->hwndPrevParent);
	    lpBand->hwndChild = 0;
	    lpBand->hwndPrevParent = 0;
	}
    }

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lpBand->cxMinChild = lprbbi->cxMinChild;
	lpBand->cyMinChild = lprbbi->cyMinChild;
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	    lpBand->cyChild    = lprbbi->cyChild;
	    lpBand->cyMaxChild = lprbbi->cyMaxChild;
	    lpBand->cyIntegral = lprbbi->cyIntegral;
	}
	else { /* special case - these should be zeroed out since   */
	       /* RBBIM_CHILDSIZE added these in WIN32_IE >= 0x0400 */
	    lpBand->cyChild    = 0;
	    lpBand->cyMaxChild = 0;
	    lpBand->cyIntegral = 0;
	}
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lpBand->cx = lprbbi->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lpBand->hbmBack = lprbbi->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lpBand->wID = lprbbi->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lpBand->cxIdeal = lprbbi->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lpBand->lParam = lprbbi->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lpBand->cxHeader = lprbbi->cxHeader;
    }
}

static LRESULT
REBAR_InternalEraseBkGnd (HWND hwnd, WPARAM wParam, LPARAM lParam, RECT *clip)
     /* Function:  This erases the background rectangle with the  */
     /*  default brush, then with any band that has a different   */
     /*  background color.                                        */
{
    HBRUSH hbrBackground = GetClassWord(hwnd, GCW_HBRBACKGROUND);
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT eraserect;
    REBAR_BAND *lpBand;
    INT i;

    if (hbrBackground)
        FillRect( (HDC) wParam, clip, hbrBackground);

    for(i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	if (lpBand->clrBack != CLR_NONE) {
	  if (IntersectRect (&eraserect, clip, &lpBand->rcBand)) {
	    /* draw background */
	    HBRUSH brh = CreateSolidBrush (lpBand->clrBack);
	    TRACE("backround color=0x%06lx, band (%d,%d)-(%d,%d), clip (%d,%d)-(%d,%d)\n",
		  lpBand->clrBack,
		  lpBand->rcBand.left,lpBand->rcBand.top,
		  lpBand->rcBand.right,lpBand->rcBand.bottom,
		  clip->left, clip->top,
		  clip->right, clip->bottom);
	    FillRect ( (HDC)wParam, &eraserect, brh);
	    DeleteObject (brh);
	  }
	}
    }
    return TRUE;
}

static void
REBAR_InternalHitTest (HWND hwnd, LPPOINT lpPt, UINT *pFlags, INT *pBand)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    RECT rect;
    INT  iCount;

    GetClientRect (hwnd, &rect);

    *pFlags = RBHT_NOWHERE;
    if (PtInRect (&rect, *lpPt))
    {
	if (infoPtr->uNumBands == 0) {
	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;
	    TRACE("NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    infoPtr->ihitBand = -1;
	    for (iCount = 0; iCount < infoPtr->uNumBands; iCount++) {
		lpBand = &infoPtr->bands[iCount];
		if (PtInRect (&lpBand->rcBand, *lpPt)) {
		    if (pBand)
			*pBand = iCount;
		    if (PtInRect (&lpBand->rcGripper, *lpPt)) {
			*pFlags = RBHT_GRABBER;
			infoPtr->ihitBand = iCount;
			TRACE("ON GRABBER %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapImage, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcCapText, *lpPt)) {
			*pFlags = RBHT_CAPTION;
			TRACE("ON CAPTION %d\n", iCount);
			return;
		    }
		    else if (PtInRect (&lpBand->rcChild, *lpPt)) {
			*pFlags = RBHT_CLIENT;
			TRACE("ON CLIENT %d\n", iCount);
			return;
		    }
		    else {
			*pFlags = RBHT_NOWHERE;
			TRACE("NOWHERE %d\n", iCount);
			return;
		    }
		}
	    }

	    *pFlags = RBHT_NOWHERE;
	    if (pBand)
		*pBand = -1;

	    TRACE("NOWHERE\n");
	    return;
	}
    }
    else {
	*pFlags = RBHT_NOWHERE;
	if (pBand)
	    *pBand = -1;
	TRACE("NOWHERE\n");
	return;
    }

    TRACE("flags=0x%X\n", *pFlags);
    return;
}

#define READJ(b,i) {if(dwStyle & CCS_VERT) b->rcBand.bottom+=(i); \
                    else b->rcBand.right += (i);}
#define LEADJ(b,i) {if(dwStyle & CCS_VERT) b->rcBand.top+=(i); \
                    else b->rcBand.left += (i);}


static INT
REBAR_Shrink (REBAR_BAND *band, INT movement, INT i, DWORD dwStyle)
     /* Function:  This attempts to shrink the given band by the  */
     /*  the amount in "movement". A shrink to the left is indi-  */
     /*  cated by "movement" being negative. "i" is merely the    */
     /*  band index for trace messages.                           */
{
    INT Leadjust, Readjust, avail, ret;

    /* Note: a left drag is indicated by "movement" being negative.  */
    /*       Similarly, a right drag is indicated by "movement"      */
    /*       being positive. "movement" should never be 0, but if    */
    /*       it is then the band does not move.                      */

    avail = rcBrb(band) - rcBlt(band) - band->cxHeader - band->lcx;

    /* now compute the Left End adjustment factor and Right End */
    /* adjustment factor. They may be different if shrinking.   */
    if (avail <= 0) {
        /* if this band is not shrinkable, then just move it */
        Leadjust = Readjust = movement;
	ret = movement;
    }
    else {
        if (movement < 0) {
	    /* Drag to left */
	    if (avail <= abs(movement)) {
	        Readjust = movement;
		Leadjust = movement + avail;
		ret = Leadjust;
	    }
	    else {
	        Readjust = movement;
		Leadjust = 0;
		ret = 0;
	    }
	}
	else {
	    /* Drag to right */
	    if (avail <= abs(movement)) {
	        Leadjust = movement;
		Readjust = movement - avail;
		ret = Readjust;
	    }
	    else {
	        Leadjust = movement;
		Readjust = 0;
		ret = 0;
	    }
	}
    }

    /* Reasonability Check */
    if (rcBlt(band) + Leadjust < 0) {
        ERR("adjustment will fail, band %d: left=%d, right=%d, move=%d, rtn=%d\n",
	    i, Leadjust, Readjust, movement, ret);
    }

    LEADJ(band, Leadjust);
    READJ(band, Readjust);

    TRACE("band %d:  left=%d, right=%d, move=%d, rtn=%d, rcBand=(%d,%d)-(%d,%d)\n",
	  i, Leadjust, Readjust, movement, ret,
	  band->rcBand.left, band->rcBand.top,
	  band->rcBand.right, band->rcBand.bottom);
    return ret;
}


static void
REBAR_HandleLRDrag (HWND hwnd, REBAR_INFO *infoPtr, POINTS *ptsmove, DWORD dwStyle)
     /* Function:  This will implement the functionality of a     */
     /*  Gripper drag within a row. It will not implement "out-   */
     /*  of-row" drags. (They are detected and handled in         */
     /*  REBAR_MouseMove.)                                        */
     /*  **** FIXME Switching order of bands in a row not   ****  */
     /*  ****       yet implemented.                        ****  */
{
    REBAR_BAND *hitBand, *band, *prevband, *mindBand, *maxdBand;
    HDWP deferpos;
    NMREBARCHILDSIZE cs;
    RECT newrect;
    INT imindBand = -1, imaxdBand, ihitBand, i, movement, tempx;
    INT RHeaderSum = 0, LHeaderSum = 0;
    INT compress;

    /* on first significant mouse movement, issue notify */

    if (!(infoPtr->fStatus & BEGIN_DRAG_ISSUED)) {
	if (REBAR_Notify_NMREBAR (hwnd, infoPtr, -1, RBN_BEGINDRAG)) {
	    /* Notify returned TRUE - abort drag */
	    infoPtr->dragStart.x = 0;
	    infoPtr->dragStart.y = 0;
	    infoPtr->dragNow = infoPtr->dragStart;
	    infoPtr->ihitBand = -1;
	    ReleaseCapture ();
	    return ;
	}
	infoPtr->fStatus |= BEGIN_DRAG_ISSUED;
    }

    ihitBand = infoPtr->ihitBand;
    hitBand = &infoPtr->bands[ihitBand];
    imaxdBand = ihitBand; /* to suppress warning message */

    /* find all the bands in the row of the one whose Gripper was seized */
    for (i=0; i<infoPtr->uNumBands; i++) {
        band = &infoPtr->bands[i];
	if (HIDDENBAND(band)) continue;
	if (band->iRow == hitBand->iRow) {
	    imaxdBand = i;
	    if (imindBand == -1) imindBand = i;
	    /* minimum size of each band is size of header plus            */
	    /* size of minimum child plus offset of child from header plus */
	    /* a one to separate each band.                                */
	    if (i < ihitBand)
	        LHeaderSum += (band->cxHeader + band->lcx + SEP_WIDTH);
	    else 
	        RHeaderSum += (band->cxHeader + band->lcx + SEP_WIDTH);

	}
    }
    if (RHeaderSum) RHeaderSum -= SEP_WIDTH; /* no separator afterlast band */

    mindBand = &infoPtr->bands[imindBand];
    maxdBand = &infoPtr->bands[imaxdBand];

    if (imindBand == imaxdBand) return; /* nothing to drag agains */
    if (imindBand == ihitBand) return; /* first band in row, cant drag */

    /* limit movement to inside adjustable bands - Left */
    if ( (ptsmove->x < mindBand->rcBand.left) ||
	 (ptsmove->x > maxdBand->rcBand.right) ||
	 (ptsmove->y < mindBand->rcBand.top) ||
	 (ptsmove->y > maxdBand->rcBand.bottom))
        return; /* should swap bands */

    if (dwStyle & CCS_VERT)
        movement = ptsmove->y - ((hitBand->rcBand.top+REBAR_PRE_GRIPPER) -
			     infoPtr->ihitoffset);
    else
        movement = ptsmove->x - ((hitBand->rcBand.left+REBAR_PRE_GRIPPER) -
			     infoPtr->ihitoffset);
    infoPtr->dragNow = *ptsmove;

    TRACE("before: movement=%d (%d,%d), imindBand=%d, ihitBand=%d, imaxdBand=%d, LSum=%d, RSum=%d\n",
	  movement, ptsmove->x, ptsmove->y, imindBand, ihitBand,
	  imaxdBand, LHeaderSum, RHeaderSum);
    REBAR_DumpBand (hwnd);

    if (movement < 0) {  

        /* ***  Drag left/up *** */
        compress = rcBlt(hitBand) - rcBlt(mindBand) -
	           LHeaderSum;
	if (compress < abs(movement)) {
	    TRACE("limiting left drag, was %d changed to %d\n",
		  movement, -compress);
	    movement = -compress;
	}
        for (i=ihitBand; i>=imindBand; i--) {
	    band = &infoPtr->bands[i];
	    if (i == ihitBand) {
		prevband = &infoPtr->bands[i-1];
		if (rcBlt(band) - movement <= rcBlt(prevband)) {
		    tempx = movement - (rcBrb(prevband)-rcBlt(band)+1);
		    ERR("movement bad. BUG!! was %d, left=%d, right=%d, setting to %d\n",
			movement, rcBlt(band), rcBlt(prevband), tempx);
		    movement = tempx;
		}
		LEADJ(band, movement)
	    }
	    else 
	        movement = REBAR_Shrink (band, movement, i, dwStyle);
	}
    }
    else {

        /* ***  Drag right/down *** */
        compress = rcBrb(maxdBand) - rcBlt(hitBand) -
	           RHeaderSum;
	if (compress < abs(movement)) {
	    TRACE("limiting right drag, was %d changed to %d\n",
		  movement, compress);
	    movement = compress;
	}
        for (i=ihitBand-1; i<=imaxdBand; i++) {
	    band = &infoPtr->bands[i];
	    if (HIDDENBAND(band)) continue;
	    if (i == ihitBand-1) {
		READJ(band, movement)
	    }
	    else 
	        movement = REBAR_Shrink (band, movement, i, dwStyle);
	}
    }

    /* recompute all rectangles */
    if (dwStyle & CCS_VERT) {
	REBAR_CalcVertBand (hwnd, infoPtr, imindBand, imaxdBand+1,
			    FALSE, dwStyle);
    }
    else {
	REBAR_CalcHorzBand (hwnd, infoPtr, imindBand, imaxdBand+1, 
			    FALSE, dwStyle);
    }

    TRACE("bands after adjustment, see band # %d, %d\n",
	  imindBand, imaxdBand);
    REBAR_DumpBand (hwnd);

    SetRect (&newrect, 
	     mindBand->rcBand.left,
	     mindBand->rcBand.top,
	     maxdBand->rcBand.right,
	     maxdBand->rcBand.bottom);

    if (!(deferpos = BeginDeferWindowPos (4))) {
        ERR("BeginDeferWindowPos returned NULL\n");
    }

    for (i=imindBand; i<=imaxdBand; i++) {
        band = &infoPtr->bands[i];
	if ((band->fMask & RBBIM_CHILD) && band->hwndChild) {
	    cs.uBand = i;
	    cs.wID = band->wID;
	    cs.rcChild = band->rcChild;
	    cs.rcBand = band->rcBand;
	    cs.rcBand.left += band->cxHeader;
	    REBAR_Notify (hwnd, (NMHDR *) &cs, infoPtr, RBN_CHILDSIZE);
	    deferpos = DeferWindowPos (deferpos, band->hwndChild, HWND_TOP,
				       cs.rcChild.left, cs.rcChild.top,
				       cs.rcChild.right - cs.rcChild.left,
				       cs.rcChild.bottom - cs.rcChild.top,
				       SWP_NOZORDER);
	    if (!deferpos) {
	        ERR("DeferWindowPos returned NULL\n");
	    }
	}
    }

    if (!EndDeferWindowPos (deferpos)) {
        ERR("EndDeferWindowPos failed\n");
    }

    InvalidateRect (hwnd, &newrect, TRUE);
    UpdateWindow (hwnd);

}
#undef READJ
#undef LEADJ



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    UINT uBand = (UINT)wParam;
    HWND childhwnd = 0;
    REBAR_BAND *lpBand;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);
    lpBand = &infoPtr->bands[uBand];
    REBAR_Notify_NMREBAR (hwnd, infoPtr, uBand, RBN_DELETINGBAND);

    if (infoPtr->uNumBands == 1) {
	TRACE(" simple delete!\n");
	if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild)
	    childhwnd = lpBand->hwndChild;
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
	infoPtr->uNumBands = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
        TRACE("complex delete! [uBand=%u]\n", uBand);

	if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild)
	    childhwnd = lpBand->hwndChild;

	infoPtr->uNumBands--;
	infoPtr->bands = COMCTL32_Alloc (sizeof (REBAR_BAND) * infoPtr->uNumBands);
        if (uBand > 0) {
            memcpy (&infoPtr->bands[0], &oldBands[0],
                    uBand * sizeof(REBAR_BAND));
        }

        if (uBand < infoPtr->uNumBands) {
            memcpy (&infoPtr->bands[uBand], &oldBands[uBand+1],
                    (infoPtr->uNumBands - uBand) * sizeof(REBAR_BAND));
        }

	COMCTL32_Free (oldBands);
    }

    if (childhwnd)
        ShowWindow (childhwnd, SW_HIDE);

    REBAR_Notify_NMREBAR (hwnd, infoPtr, -1, RBN_DELETEDBAND);

    /* if only 1 band left the re-validate to possible eliminate gripper */
    if (infoPtr->uNumBands == 1)
      REBAR_ValidateBand (hwnd, infoPtr, &infoPtr->bands[0]);

    REBAR_Layout (hwnd, NULL, TRUE, FALSE);
    infoPtr->fStatus |= RESIZE_ANYHOW;
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPRECT lpRect = (LPRECT)lParam;
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    if (!lParam)
	return 0;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return 0;

    lpBand = &infoPtr->bands[(UINT)wParam];

    /* FIXME - the following values were determined by experimentation */
    /* with the REBAR Control Spy. I have guesses as to what the 4 and */
    /* 1 are, but I am not sure. There doesn't seem to be any actual   */
    /* difference in size of the control area with and without the     */
    /* style.  -  GA                                                   */
    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_BANDBORDERS) {
	if (dwStyle & CCS_VERT) {
	    lpRect->left = 1;
	    lpRect->top = lpBand->cxHeader + 4;
	    lpRect->right = 1;
	    lpRect->bottom = 0;
	}
	else {
	    lpRect->left = lpBand->cxHeader + 4;
	    lpRect->top = 1;
	    lpRect->right = 0;
	    lpRect->bottom = 1;
	}
    }
    else {
	lpRect->left = lpBand->cxHeader;
    }
    FIXME("stub\n");
    return 0;
}


inline static LRESULT
REBAR_GetBandCount (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
	if (lprbbi->clrBack == CLR_NONE)
	    lprbbi->clrBack = GetSysColor (COLOR_BTNFACE);
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
      if (lpBand->lpText && (lpBand->fMask & RBBIM_TEXT))
      {
          if (!WideCharToMultiByte( CP_ACP, 0, lpBand->lpText, -1,
                                    lprbbi->lpText, lprbbi->cch, NULL, NULL ))
              lprbbi->lpText[lprbbi->cch-1] = 0;
      }
      else 
	*lprbbi->lpText = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE) {
      if (lpBand->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;
      else
	lprbbi->iImage = -1;
    }

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	    lprbbi->cyChild    = lpBand->cyChild;
	    lprbbi->cyMaxChild = lpBand->cyMaxChild;
	    lprbbi->cyIntegral = lpBand->cyIntegral;
	}
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lprbbi->cx = lpBand->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lprbbi->hbmBack = lpBand->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lprbbi->wID = lpBand->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOA)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    REBAR_DumpBandInfo (lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBandInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);

    /* copy band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    if (lprbbi->fMask & RBBIM_STYLE)
	lprbbi->fStyle = lpBand->fStyle;

    if (lprbbi->fMask & RBBIM_COLORS) {
	lprbbi->clrFore = lpBand->clrFore;
	lprbbi->clrBack = lpBand->clrBack;
	if (lprbbi->clrBack == CLR_NONE)
	    lprbbi->clrBack = GetSysColor (COLOR_BTNFACE);
    }

    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
      if (lpBand->lpText && (lpBand->fMask & RBBIM_TEXT))
	lstrcpynW (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
      else 
	*lprbbi->lpText = 0;
    }

    if (lprbbi->fMask & RBBIM_IMAGE) {
      if (lpBand->fMask & RBBIM_IMAGE)
	lprbbi->iImage = lpBand->iImage;
      else
	lprbbi->iImage = -1;
    }

    if (lprbbi->fMask & RBBIM_CHILD)
	lprbbi->hwndChild = lpBand->hwndChild;

    if (lprbbi->fMask & RBBIM_CHILDSIZE) {
	lprbbi->cxMinChild = lpBand->cxMinChild;
	lprbbi->cyMinChild = lpBand->cyMinChild;
	if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
	    lprbbi->cyChild    = lpBand->cyChild;
	    lprbbi->cyMaxChild = lpBand->cyMaxChild;
	    lprbbi->cyIntegral = lpBand->cyIntegral;
	}
    }

    if (lprbbi->fMask & RBBIM_SIZE)
	lprbbi->cx = lpBand->cx;

    if (lprbbi->fMask & RBBIM_BACKGROUND)
	lprbbi->hbmBack = lpBand->hbmBack;

    if (lprbbi->fMask & RBBIM_ID)
	lprbbi->wID = lpBand->wID;

    /* check for additional data */
    if (lprbbi->cbSize >= sizeof (REBARBANDINFOW)) {
	if (lprbbi->fMask & RBBIM_IDEALSIZE)
	    lprbbi->cxIdeal = lpBand->cxIdeal;

	if (lprbbi->fMask & RBBIM_LPARAM)
	    lprbbi->lParam = lpBand->lParam;

	if (lprbbi->fMask & RBBIM_HEADERSIZE)
	    lprbbi->cxHeader = lpBand->cxHeader;
    }

    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBarHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT nHeight;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    nHeight = (dwStyle & CCS_VERT) ? infoPtr->calcSize.cx : infoPtr->calcSize.cy;

    TRACE("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("getting bar info!\n");

    if (infoPtr->himl) {
	lpInfo->himl = infoPtr->himl;
	lpInfo->fMask |= RBIM_IMAGELIST;
    }

    return TRUE;
}


inline static LRESULT
REBAR_GetBkColor (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    COLORREF clr = infoPtr->clrBk;

    if (clr == CLR_NONE)
      clr = GetSysColor (COLOR_BTNFACE);

    TRACE("background color 0x%06lx!\n", clr);

    return clr;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT iBand = (INT)wParam;
    LPRECT lprc = (LPRECT)lParam;
    REBAR_BAND *lpBand;

    if ((iBand < 0) && ((UINT)iBand >= infoPtr->uNumBands))
	return FALSE;
    if (!lprc)
	return FALSE;

    lpBand = &infoPtr->bands[iBand];
    CopyRect (lprc, &lpBand->rcBand);

    TRACE("band %d, (%d,%d)-(%d,%d)\n", iBand,
	  lprc->left, lprc->top, lprc->right, lprc->bottom);

    return TRUE;
}


inline static LRESULT
REBAR_GetRowCount (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("%u\n", infoPtr->uNumRows);

    return infoPtr->uNumRows;
}


static LRESULT
REBAR_GetRowHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT iRow = (INT)wParam;
    int ret = 0;
    int i, j = 0;
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    for (i=0; i<infoPtr->uNumBands; i++) {
      lpBand = &infoPtr->bands[i];
      if (HIDDENBAND(lpBand)) continue;
      if (lpBand->iRow != iRow) continue;
      if (dwStyle & CCS_VERT)
	j = lpBand->rcBand.right - lpBand->rcBand.left;
      else
	j = lpBand->rcBand.bottom - lpBand->rcBand.top;
      if (j > ret) ret = j;
    }

    TRACE("row %d, height %d\n", iRow, ret);

    return ret;
}


inline static LRESULT
REBAR_GetTextColor (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


inline static LRESULT
REBAR_GetToolTips (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    return infoPtr->hwndToolTip;
}


inline static LRESULT
REBAR_GetUnicodeFormat (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    return infoPtr->bUnicode;
}


inline static LRESULT
REBAR_GetVersion (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    TRACE("version %d\n", infoPtr->iVersion);
    return infoPtr->iVersion;
}


static LRESULT
REBAR_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    /* REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam; 

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (hwnd, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    UINT i;

    if (infoPtr == NULL)
	return -1;

    if (infoPtr->uNumBands < 1)
	return -1;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	if (infoPtr->bands[i].wID == (UINT)wParam) {
	    TRACE("id %u is band %u found!\n", (UINT)wParam, i);
	    return i;
	}
    }

    TRACE("id %u is not found\n", (UINT)wParam);
    return -1;
}


static LRESULT
REBAR_InsertBandA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d!\n", (INT)uIndex);
    REBAR_DumpBandInfo (lprbbi);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex < infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex - 1) * sizeof(REBAR_BAND));
	}

	COMCTL32_Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE("index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];
    lpBand->fMask = 0;
    lpBand->fStatus = 0;
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->hwndChild = 0;
    lpBand->hwndPrevParent = 0;

    REBAR_CommonSetupBand (hwnd, lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
        INT len = MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, NULL, 0 );
        if (len > 1) {
            lpBand->lpText = (LPWSTR)COMCTL32_Alloc (len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, lpBand->lpText, len );
	}
    }

    REBAR_ValidateBand (hwnd, infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (hwnd, infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (hwnd);

    REBAR_Layout (hwnd, NULL, TRUE, FALSE);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_InsertBandW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    UINT uIndex = (UINT)wParam;
    REBAR_BAND *lpBand;

    if (infoPtr == NULL)
	return FALSE;
    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;

    /* trace the index as signed to see the -1 */
    TRACE("insert band at %d!\n", (INT)uIndex);
    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    if (infoPtr->uNumBands == 0) {
	infoPtr->bands = (REBAR_BAND *)COMCTL32_Alloc (sizeof (REBAR_BAND));
	uIndex = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
	infoPtr->bands =
	    (REBAR_BAND *)COMCTL32_Alloc ((infoPtr->uNumBands+1)*sizeof(REBAR_BAND));
	if (((INT)uIndex == -1) || (uIndex > infoPtr->uNumBands))
	    uIndex = infoPtr->uNumBands;

	/* pre insert copy */
	if (uIndex > 0) {
	    memcpy (&infoPtr->bands[0], &oldBands[0],
		    uIndex * sizeof(REBAR_BAND));
	}

	/* post copy */
	if (uIndex < infoPtr->uNumBands - 1) {
	    memcpy (&infoPtr->bands[uIndex+1], &oldBands[uIndex],
		    (infoPtr->uNumBands - uIndex - 1) * sizeof(REBAR_BAND));
	}

	COMCTL32_Free (oldBands);
    }

    infoPtr->uNumBands++;

    TRACE("index %u!\n", uIndex);

    /* initialize band (infoPtr->bands[uIndex])*/
    lpBand = &infoPtr->bands[uIndex];
    lpBand->fMask = 0;
    lpBand->fStatus = 0;
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->hwndChild = 0;
    lpBand->hwndPrevParent = 0;

    REBAR_CommonSetupBand (hwnd, (LPREBARBANDINFOA)lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT len = lstrlenW (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    REBAR_ValidateBand (hwnd, infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (hwnd, infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (hwnd);

    REBAR_Layout (hwnd, NULL, TRUE, FALSE);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u fIdeal = %s) stub\n",
	   (UINT)wParam, lParam ? "TRUE" : "FALSE");

 
    return 0;
}


static LRESULT
REBAR_MinimizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u) stub\n", (UINT)wParam);

 
    return 0;
}


static LRESULT
REBAR_MoveBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(iFrom = %u iTof = %u) stub\n",
	   (UINT)wParam, (UINT)lParam);

 
    return FALSE;
}


static LRESULT
REBAR_SetBandInfoA (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOA lprbbi = (LPREBARBANDINFOA)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEA)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);
    REBAR_DumpBandInfo (lprbbi);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    REBAR_CommonSetupBand (hwnd, lprbbi, lpBand);
    if (lprbbi->fMask & RBBIM_TEXT) {
	if (lpBand->lpText) {
	    COMCTL32_Free (lpBand->lpText);
	    lpBand->lpText = NULL;
	}
	if (lprbbi->lpText) {
            INT len = MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, NULL, 0 );
            lpBand->lpText = (LPWSTR)COMCTL32_Alloc (len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, lpBand->lpText, len );
	}
    }

    REBAR_ValidateBand (hwnd, infoPtr, lpBand);

    REBAR_DumpBand (hwnd);

    if (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE)) {
      REBAR_Layout (hwnd, NULL, TRUE, FALSE);
      REBAR_ForceResize (hwnd);
      REBAR_MoveChildWindows (hwnd);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBandInfoW (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARBANDINFOW lprbbi = (LPREBARBANDINFOW)lParam;
    REBAR_BAND *lpBand;

    if (lprbbi == NULL)
	return FALSE;
    if (lprbbi->cbSize < REBARBANDINFO_V3_SIZEW)
	return FALSE;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return FALSE;

    TRACE("index %u\n", (UINT)wParam);
    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    /* set band information */
    lpBand = &infoPtr->bands[(UINT)wParam];

    REBAR_CommonSetupBand (hwnd, (LPREBARBANDINFOA)lprbbi, lpBand);
    if (lprbbi->fMask & RBBIM_TEXT) {
	if (lpBand->lpText) {
	    COMCTL32_Free (lpBand->lpText);
	    lpBand->lpText = NULL;
	}
	if (lprbbi->lpText) {
	    INT len = lstrlenW (lprbbi->lpText);
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    REBAR_ValidateBand (hwnd, infoPtr, lpBand);

    REBAR_DumpBand (hwnd);

    if (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE)) {
      REBAR_Layout (hwnd, NULL, TRUE, FALSE);
      REBAR_ForceResize (hwnd);
      REBAR_MoveChildWindows (hwnd);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    LPREBARINFO lpInfo = (LPREBARINFO)lParam;
    REBAR_BAND *lpBand;
    UINT i;

    if (lpInfo == NULL)
	return FALSE;

    if (lpInfo->cbSize < sizeof (REBARINFO))
	return FALSE;

    TRACE("setting bar info!\n");

    if (lpInfo->fMask & RBIM_IMAGELIST) {
	infoPtr->himl = lpInfo->himl;
	if (infoPtr->himl) {
            INT cx, cy;
	    ImageList_GetIconSize (infoPtr->himl, &cx, &cy);
	    infoPtr->imageSize.cx = cx;
	    infoPtr->imageSize.cy = cy;
	}
	else {
	    infoPtr->imageSize.cx = 0;
	    infoPtr->imageSize.cy = 0;
	}
	TRACE("new image cx=%ld, cy=%ld\n", infoPtr->imageSize.cx,
	      infoPtr->imageSize.cy);
    }

    /* revalidate all bands to reset flags for images in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (hwnd, infoPtr, lpBand);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBkColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;

    TRACE("background color 0x%06lx!\n", infoPtr->clrBk);

    return clrTemp;
}


/* << REBAR_SetColorScheme >> */
/* << REBAR_SetPalette >> */


static LRESULT
REBAR_SetParent (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    HWND hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = (COLORREF)lParam;

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


inline static LRESULT
REBAR_SetUnicodeFormat (HWND hwnd, WPARAM wParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    BOOL bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;
    return bTemp;
}


static LRESULT
REBAR_SetVersion (HWND hwnd, INT iVersion)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT iOldVersion = infoPtr->iVersion;

    if (iVersion > COMCTL32_VERSION)
	return -1;

    infoPtr->iVersion = iVersion;

    TRACE("new version %d\n", iVersion);

    return iOldVersion;
}


static LRESULT
REBAR_ShowBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;

    if (((INT)wParam < 0) || ((INT)wParam > infoPtr->uNumBands))
	return FALSE;

    lpBand = &infoPtr->bands[(INT)wParam];

    if ((BOOL)lParam) {
	TRACE("show band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle & ~RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
    }
    else {
	TRACE("hide band %d\n", (INT)wParam);
	lpBand->fStyle = lpBand->fStyle | RBBS_HIDDEN;
	if (IsWindow (lpBand->hwndChild))
	    ShowWindow (lpBand->hwndChild, SW_HIDE);
    }

    REBAR_Layout (hwnd, NULL, TRUE, FALSE);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPRECT lpRect = (LPRECT)lParam;
    RECT t1;

    if (lpRect == NULL)
       return FALSE;

    TRACE("[%d %d %d %d]\n",
	  lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

    /*  what is going on???? */
    GetWindowRect(hwnd, &t1);
    TRACE("window rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);
    GetClientRect(hwnd, &t1);
    TRACE("client rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);

    REBAR_Layout (hwnd, lpRect, TRUE, FALSE);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);
    return TRUE;
}



static LRESULT
REBAR_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)COMCTL32_Alloc (sizeof(REBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* initialize info structure - initial values are 0 */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = GetSysColor (COLOR_BTNTEXT);
    infoPtr->ihitBand = -1;

    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->hcurHorz  = LoadCursorA (0, IDC_SIZEWEA);
    infoPtr->hcurVert  = LoadCursorA (0, IDC_SIZENSA);
    infoPtr->hcurDrag  = LoadCursorA (0, IDC_SIZEA);

    infoPtr->bUnicode = IsWindowUnicode (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_AUTOSIZE)
	FIXME("style RBS_AUTOSIZE set!\n");

#if 0
    SendMessageA (hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
#endif

    TRACE("created!\n");
    return 0;
}


static LRESULT
REBAR_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    INT i;


    /* free rebar bands */
    if ((infoPtr->uNumBands > 0) && infoPtr->bands) {
	/* clean up each band */
	for (i = 0; i < infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];

	    /* delete text strings */
	    if (lpBand->lpText) {
		COMCTL32_Free (lpBand->lpText);
		lpBand->lpText = NULL;
	    }
	    /* destroy child window */
	    DestroyWindow (lpBand->hwndChild);
	}

	/* free band array */
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
    }

    DeleteObject (infoPtr->hcurArrow);
    DeleteObject (infoPtr->hcurHorz);
    DeleteObject (infoPtr->hcurVert);
    DeleteObject (infoPtr->hcurDrag);

    /* free rebar info data */
    COMCTL32_Free (infoPtr);
    SetWindowLongA (hwnd, 0, 0);
    TRACE("destroyed!\n");
    return 0;
}


static LRESULT
REBAR_EraseBkGnd (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT cliprect;

    if (GetClipBox ( (HDC)wParam, &cliprect))
        return REBAR_InternalEraseBkGnd (hwnd, wParam, lParam, &cliprect);
    return 0;
}


static LRESULT
REBAR_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
REBAR_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    /* If InternalHitTest did not find a hit on the Gripper, */
    /* then ignore the button click.                         */
    if (infoPtr->ihitBand == -1) return 0;

    SetCapture (hwnd);

    /* save off the LOWORD and HIWORD of lParam as initial x,y */
    lpBand = &infoPtr->bands[infoPtr->ihitBand];
    infoPtr->dragStart = MAKEPOINTS(lParam);
    infoPtr->dragNow = infoPtr->dragStart;
    if (dwStyle & CCS_VERT)
        infoPtr->ihitoffset = infoPtr->dragStart.y - (lpBand->rcBand.top+REBAR_PRE_GRIPPER);
    else
        infoPtr->ihitoffset = infoPtr->dragStart.x - (lpBand->rcBand.left+REBAR_PRE_GRIPPER);

    return 0;
}


static LRESULT
REBAR_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    NMHDR layout;
    RECT rect;

    /* If InternalHitTest did not find a hit on the Gripper, */
    /* then ignore the button click.                         */
    if (infoPtr->ihitBand == -1) return 0;

    infoPtr->dragStart.x = 0;
    infoPtr->dragStart.y = 0;
    infoPtr->dragNow = infoPtr->dragStart;
    infoPtr->ihitBand = -1;

    ReleaseCapture ();

    if (infoPtr->fStatus & BEGIN_DRAG_ISSUED) {
        REBAR_Notify(hwnd, (NMHDR *) &layout, infoPtr, RBN_LAYOUTCHANGED);
	REBAR_Notify_NMREBAR (hwnd, infoPtr, -1, RBN_ENDDRAG);
	infoPtr->fStatus &= ~BEGIN_DRAG_ISSUED;
    }

    GetClientRect(hwnd, &rect);
    InvalidateRect(hwnd, NULL, TRUE);

    return 0;
}


static LRESULT
REBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    REBAR_BAND *band1, *band2;
    POINTS ptsmove;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    /* Validate entry as hit on Gripper has occured */
    if (GetCapture() != hwnd) return 0;
    if (infoPtr->ihitBand == -1) return 0;

    ptsmove = MAKEPOINTS(lParam);

    /* if mouse did not move much, exit */
    if ((abs(ptsmove.x - infoPtr->dragNow.x) <= mindragx) &&
	(abs(ptsmove.y - infoPtr->dragNow.y) <= mindragy)) return 0;

    band1 = &infoPtr->bands[infoPtr->ihitBand-1];
    band2 = &infoPtr->bands[infoPtr->ihitBand];

    /* Test for valid drag case - must not be first band in row */
    if (dwStyle & CCS_VERT) {
	if ((ptsmove.x < band2->rcBand.left) ||
	    (ptsmove.x > band2->rcBand.right) ||
	    ((infoPtr->ihitBand > 0) && (band1->iRow != band2->iRow))) {
	    FIXME("Cannot drag to other rows yet!!\n");
	}
	else {
	    REBAR_HandleLRDrag (hwnd, infoPtr, &ptsmove, dwStyle);
	}
    }
    else {
	if ((ptsmove.y < band2->rcBand.top) ||
	    (ptsmove.y > band2->rcBand.bottom) ||
	    ((infoPtr->ihitBand > 0) && (band1->iRow != band2->iRow))) {
	    FIXME("Cannot drag to other rows yet!!\n");
	}
	else {
	    REBAR_HandleLRDrag (hwnd, infoPtr, &ptsmove, dwStyle);
	}
    }
    return 0;
}


inline static LRESULT
REBAR_NCCalcSize (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (GetWindowLongA (hwnd, GWL_STYLE) & WS_BORDER) {
	((LPRECT)lParam)->left   += GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->top    += GetSystemMetrics(SM_CYEDGE);
	((LPRECT)lParam)->right  -= GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->bottom -= GetSystemMetrics(SM_CYEDGE);
    }

    return 0;
}


static LRESULT
REBAR_NCPaint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    RECT rcWindow;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    DefWindowProcA (hwnd, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW )))
	return 0;

    if (dwStyle & WS_BORDER) {
	GetWindowRect (hwnd, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_RECT);
    }

    ReleaseDC( hwnd, hdc );

    return 0;
}


static LRESULT
REBAR_Paint (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;

    TRACE("painting (%d,%d)-(%d,%d)\n",
	  ps.rcPaint.left, ps.rcPaint.top,
	  ps.rcPaint.right, ps.rcPaint.bottom);

    if (ps.fErase) {
	/* Erase area of paint if requested */
        REBAR_InternalEraseBkGnd (hwnd, wParam, lParam, &ps.rcPaint);
    }

    REBAR_Refresh (hwnd, hdc);
    if (!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
REBAR_SetCursor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (hwnd, &pt);

    REBAR_InternalHitTest (hwnd, &pt, &flags, NULL);

    if (flags == RBHT_GRABBER) {
	if ((dwStyle & CCS_VERT) &&
	    !(dwStyle & RBS_VERTICALGRIPPER))
	    SetCursor (infoPtr->hcurVert);
	else
	    SetCursor (infoPtr->hcurHorz);
    }
    else if (flags != RBHT_CLIENT)
	SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
REBAR_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rcClient;
    REBAR_BAND *lpBand;
    UINT i;

    infoPtr->hFont = (HFONT)wParam;

    /* revalidate all bands to change sizes of text in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (hwnd, infoPtr, lpBand);
    }


    if (lParam) {
        GetClientRect (hwnd, &rcClient);
        REBAR_Layout (hwnd, &rcClient, FALSE, TRUE);
	REBAR_ForceResize (hwnd);
	REBAR_MoveChildWindows (hwnd);
    }

    return 0;
}


static LRESULT
REBAR_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rcClient;

    /* auto resize deadlock check */
    if (infoPtr->fStatus & AUTO_RESIZE) {
	infoPtr->fStatus &= ~AUTO_RESIZE;
	TRACE("AUTO_RESIZE was set, reset, fStatus=%08x\n",
	      infoPtr->fStatus);
	return 0;
    }

    GetClientRect (hwnd, &rcClient);
    if ((lParam == 0) && (rcClient.right == 0) && (rcClient.bottom == 0)) {
      /* native control seems to do this */
      GetClientRect (GetParent(hwnd), &rcClient);
      TRACE("sizing rebar, message and client zero, parent client (%d,%d)\n", 
	    rcClient.right, rcClient.bottom);
    }
    else {
      TRACE("sizing rebar from (%ld,%ld) to (%d,%d), client (%d,%d)\n", 
	    infoPtr->calcSize.cx, infoPtr->calcSize.cy,
	    LOWORD(lParam), HIWORD(lParam),
	    rcClient.right, rcClient.bottom);
    }

    REBAR_Layout (hwnd, &rcClient, TRUE, TRUE);
    REBAR_ForceResize (hwnd);
    infoPtr->fStatus &= ~AUTO_RESIZE;
    REBAR_MoveChildWindows (hwnd);

    return 0;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!REBAR_GetInfoPtr (hwnd) && (uMsg != WM_CREATE))
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
/*	case RB_BEGINDRAG: */

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (hwnd, wParam, lParam);

/*	case RB_DRAGMOVE: */
/*	case RB_ENDDRAG: */

	case RB_GETBANDBORDERS:
	    return REBAR_GetBandBorders (hwnd, wParam, lParam);

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (hwnd);

	case RB_GETBANDINFO:	/* obsoleted after IE3, but we have to
				   support it anyway. */
	case RB_GETBANDINFOA:
	    return REBAR_GetBandInfoA (hwnd, wParam, lParam);

	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoW (hwnd, wParam, lParam);

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (hwnd, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (hwnd, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (hwnd);

/*	case RB_GETCOLORSCHEME: */
/*	case RB_GETDROPTARGET: */

	case RB_GETPALETTE:
	    return REBAR_GetPalette (hwnd, wParam, lParam);

	case RB_GETRECT:
	    return REBAR_GetRect (hwnd, wParam, lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (hwnd);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (hwnd, wParam, lParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (hwnd);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (hwnd);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (hwnd);

	case CCM_GETVERSION:
	    return REBAR_GetVersion (hwnd);

	case RB_HITTEST:
	    return REBAR_HitTest (hwnd, wParam, lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (hwnd, wParam, lParam);

	case RB_INSERTBANDA:
	    return REBAR_InsertBandA (hwnd, wParam, lParam);

	case RB_INSERTBANDW:
	    return REBAR_InsertBandW (hwnd, wParam, lParam);

	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (hwnd, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (hwnd, wParam, lParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (hwnd, wParam, lParam);

	case RB_SETBANDINFOA:
	    return REBAR_SetBandInfoA (hwnd, wParam, lParam);

	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoW (hwnd, wParam, lParam);

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (hwnd, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (hwnd, wParam, lParam);

/*	case RB_SETCOLORSCHEME: */
/*	case RB_SETPALETTE: */
/*	    return REBAR_GetPalette (hwnd, wParam, lParam); */

	case RB_SETPARENT:
	    return REBAR_SetParent (hwnd, wParam, lParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (hwnd, wParam, lParam);

/*	case RB_SETTOOLTIPS: */

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (hwnd, wParam);

	case CCM_SETVERSION:
	    return REBAR_SetVersion (hwnd, (INT)wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (hwnd, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (hwnd, wParam, lParam);


/*    Messages passed to parent */
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_NOTIFY:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);


/*      case WM_CHARTOITEM:     supported according to ControlSpy */

	case WM_CREATE:
	    return REBAR_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (hwnd, wParam, lParam);

        case WM_ERASEBKGND:
	    return REBAR_EraseBkGnd (hwnd, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (hwnd, wParam, lParam);

/*      case WM_LBUTTONDBLCLK:  supported according to ControlSpy */

	case WM_LBUTTONDOWN:
	    return REBAR_LButtonDown (hwnd, wParam, lParam);

	case WM_LBUTTONUP:
	    return REBAR_LButtonUp (hwnd, wParam, lParam);

/*      case WM_MEASUREITEM:    supported according to ControlSpy */

	case WM_MOUSEMOVE:
	    return REBAR_MouseMove (hwnd, wParam, lParam);

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (hwnd, wParam, lParam);

/*      case WM_NCCREATE:       supported according to ControlSpy */
/*      case WM_NCHITTEST:      supported according to ControlSpy */

	case WM_NCPAINT:
	    return REBAR_NCPaint (hwnd, wParam, lParam);

/*      case WM_NOTIFYFORMAT:   supported according to ControlSpy */

	case WM_PAINT:
	    return REBAR_Paint (hwnd, wParam, lParam);

/*      case WM_PALETTECHANGED: supported according to ControlSpy */
/*      case WM_PRINTCLIENT:    supported according to ControlSpy */
/*      case WM_QUERYNEWPALETTE:supported according to ControlSpy */
/*      case WM_RBUTTONDOWN:    supported according to ControlSpy */
/*      case WM_RBUTTONUP:      supported according to ControlSpy */

	case WM_SETCURSOR:
	    return REBAR_SetCursor (hwnd, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (hwnd, wParam, lParam);

/*      case WM_SETREDRAW:      supported according to ControlSpy */

	case WM_SIZE:
	    return REBAR_Size (hwnd, wParam, lParam);

/*      case WM_STYLECHANGED:   supported according to ControlSpy */
/*      case WM_SYSCOLORCHANGE: supported according to ControlSpy */
/*      case WM_VKEYTOITEM:     supported according to ControlSpy */
/*	case WM_WININICHANGE: */

	default:
	    if (uMsg >= WM_USER)
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
REBAR_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = (WNDPROC)REBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(REBAR_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = REBARCLASSNAMEA;
 
    RegisterClassA (&wndClass);

    mindragx = GetSystemMetrics (SM_CXDRAG);
    mindragy = GetSystemMetrics (SM_CYDRAG);

}


VOID
REBAR_Unregister (void)
{
    UnregisterClassA (REBARCLASSNAMEA, (HINSTANCE)NULL);
}

