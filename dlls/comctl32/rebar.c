/*
 * Rebar control    rev 4a
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
 *  1. Correct use of fStyle for fMask
 *  2. Rewrite to support putting bands in rows, also supports GETROWCOUNT
 *     and GETROWHEIGHT.
 *  3. Make fMask in internal band info indicate what is valid rather than -1.
 *  4. Correct output of GETBANDINFO to match MS for iImage, lpText
 *  5. Handle RBN_CHILDSIZE notification (not really working)
 *  6. Draw separators between bands and fix size of grippers.
 *  7. Draw separators between rows.
 *  8. Support RBBS_BREAK if specified by user.
 *  9. Support background color for band and draw the background.
 * 10. Eliminated duplicate code in SetBandInfo[AW] and InsertBand[AW]. 
 * 11. Support text color and propagate colors from Bar to Band
 *    Still to do:
 *  1. default row height should be the max height of all visible bands
 *  2. Following still not handled: RBBS_FIXEDBMP, RBBS_CHILDEDGE,
 *            RBBS_VARIABLEHEIGHT, RBBS_USECHEVRON
 *  3. GETBANDINFO seems to return correct colors in native but not here.

 */

#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "wine/winestring.h"
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

    UINT    uMinHeight;
    INT     iRow;           /* row this band assigned to */
    UINT    fDraw;          /* drawing flags */
    RECT    rcBand;         /* calculated band rectangle */
    RECT    rcGripper;      /* calculated gripper rectangle */
    RECT    rcCapImage;     /* calculated caption image rectangle */
    RECT    rcCapText;      /* calculated caption text rectangle */
    RECT    rcChild;        /* calculated child rectangle */

    LPWSTR    lpText;
    HWND    hwndPrevParent;
} REBAR_BAND;

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
    BOOL     bAutoResize; /* auto resize deadlock flag */
    BOOL     bUnicode;    /* Unicode flag */
    HCURSOR  hcurArrow;   /* handle to the arrow cursor */
    HCURSOR  hcurHorz;    /* handle to the EW cursor */
    HCURSOR  hcurVert;    /* handle to the NS cursor */
    HCURSOR  hcurDrag;    /* handle to the drag cursor */
    INT      iVersion;    /* version number */

    REBAR_BAND *bands;      /* pointer to the array of rebar bands */
} REBAR_INFO;


/* fDraw flags */
#define DRAW_GRIPPER    1
#define DRAW_IMAGE      2
#define DRAW_TEXT       4
#define DRAW_CHILD      8
#define DRAW_SEP        16

#define GRIPPER_WIDTH   8
#define GRIPPER_HEIGHT  16
#define SEP_WIDTH       3

/* This is the increment that is used over the band height */
/* Determined by experiment.                               */ 
#define REBARSPACE      4

#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongA (hwnd, 0))

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
      for (i = 0; i < iP->uNumBands; i++) {
	pB = &iP->bands[i];
	TRACE("band # %u: ID=%u, mask=0x%08x, style=0x%08x, child=%04x, row=%u\n",
	  i, pB->wID, pB->fMask, pB->fStyle, pB->hwndChild, pB->iRow);
	TRACE("band # %u: xMin=%u, yMin=%u, cx=%u, yChild=%u, yMax=%u, yIntgl=%u, uMinH=%u,\n",
	  i, pB->cxMinChild, pB->cyMinChild, pB->cx,
          pB->cyChild, pB->cyMaxChild, pB->cyIntegral, pB->uMinHeight);
	TRACE("band # %u: header=%u, lcx=%u, hcx=%u, lcy=%u, hcy=%u\n",
	  i, pB->cxHeader, pB->lcx, pB->hcx, pB->lcy, pB->hcy);
	TRACE("band # %u: fDraw=%08x, Band=(%d,%d)-(%d,%d), Grip=(%d,%d)-(%d,%d)\n",
	  i, pB->fDraw,
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

static VOID
REBAR_DrawBand (HDC hdc, REBAR_INFO *infoPtr, REBAR_BAND *lpBand, DWORD dwStyle)
{

    /* draw separator */
    if (lpBand->fDraw & DRAW_SEP) {
        RECT rcSep;
	if (dwStyle & CCS_VERT)
	  SetRect (&rcSep, lpBand->rcBand.left-2, lpBand->rcBand.top-SEP_WIDTH,
		   lpBand->rcBand.right+2, lpBand->rcBand.top-1);
	else
	  SetRect (&rcSep, lpBand->rcBand.left-SEP_WIDTH, lpBand->rcBand.top-2,
		   lpBand->rcBand.left-1, lpBand->rcBand.bottom+2);
	TRACE("drawing band separator (%d,%d)-(%d,%d)\n",
	      rcSep.left, rcSep.top, rcSep.right, rcSep.bottom);
	DrawEdge (hdc, &rcSep, EDGE_ETCHED, BF_LEFT | BF_TOP | BF_MIDDLE);
    }

    /* draw background */
    if (lpBand->clrBack != CLR_NONE) {
        HBRUSH brh = CreateSolidBrush (lpBand->clrBack);
	TRACE("backround color=0x%06lx, rect (%d,%d)-(%d,%d)\n",
	      lpBand->clrBack,
	      lpBand->rcBand.left,lpBand->rcBand.top,
	      lpBand->rcBand.right,lpBand->rcBand.bottom);
        FillRect (hdc, &lpBand->rcBand, brh);
	DeleteObject (brh);
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
    RECT rcRowSep;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    oldrow = infoPtr->bands[0].iRow;
    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((dwStyle & CCS_VERT) &&
	     (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	/* if a new row then draw a separator */
	if (oldrow != lpBand->iRow) {
	  if (dwStyle & CCS_VERT) {
	    SetRect (&rcRowSep, lpBand->rcBand.left-2, 0,
		     lpBand->rcBand.left-1, infoPtr->calcSize.cy);
	  }
	  else {
	    SetRect (&rcRowSep, 0, lpBand->rcBand.top-2,
		     infoPtr->calcSize.cx, lpBand->rcBand.top-1);
	  } 
	  TRACE ("drawing row sep (%d,%d)-(%d,%d)\n",
		 rcRowSep.left, rcRowSep.top, rcRowSep.right, rcRowSep.bottom);
	  DrawEdge (hdc, &rcRowSep, EDGE_ETCHED, BF_TOP|BF_LEFT);
	  oldrow = lpBand->iRow;
	}

	/* now draw the band */
	REBAR_DrawBand (hdc, infoPtr, lpBand, dwStyle);

    }
}

static void
REBAR_AdjustBands (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT maxx, INT usedx, INT mcy, DWORD dwStyle)
     /* Function: This routine distributes the extra space in a row */
     /*  evenly over the adjustable bands in the row. It also makes */
     /*  sure that all bands in the row are the same height.        */
{
    REBAR_BAND *lpBand;
    UINT i, j;
    INT incr, lastx=0;

    TRACE("start=%u, end=%u, max x=%d, used x=%d, max y=%d\n",
	  rowstart, rowend-1, maxx, usedx, mcy);

    j = 0;
    for (i = rowstart; i<rowend; i++) {
      lpBand = &infoPtr->bands[i];
      if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild &&
	  !(lpBand->fStyle & RBBS_FIXEDSIZE)) 
         j++;
    }
    if (j)
      incr = (maxx-usedx) / j;
    else
      incr = 0;

    TRACE("adjusting %u of %u bands in row, incr=%d\n", 
	  j, rowend-rowstart, incr);

    for (i = rowstart; i<rowend; i++) {
      lpBand = &infoPtr->bands[i];
      if (dwStyle & CCS_VERT) {
        lpBand->rcBand.bottom = lastx + 
                                lpBand->rcBand.bottom - lpBand->rcBand.top;
        if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild &&
	    !(lpBand->fStyle & RBBS_FIXEDSIZE)) {
           /* if a child window exists and not fixed then widen it */
           lpBand->rcBand.bottom += incr;
        }
        lpBand->rcBand.top = lastx;
        lastx = lpBand->rcBand.bottom + 1;
        lpBand->rcBand.right = lpBand->rcBand.left + mcy;
      }
      else {
        lpBand->rcBand.right = lastx + 
                               lpBand->rcBand.right - lpBand->rcBand.left;
        if ((lpBand->fMask & RBBIM_CHILD) && lpBand->hwndChild &&
	    !(lpBand->fStyle & RBBS_FIXEDSIZE)) {
           /* if a child window exists and not fixed then widen it */
           lpBand->rcBand.right += incr;
        }
        lpBand->rcBand.left = lastx;
        lastx = lpBand->rcBand.right + 1;
        lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;
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
    NMREBARCHILDSIZE  rbcz;
    HWND parenthwnd;
    RECT oldChild;

    rbcz.hdr.hwndFrom = hwnd;
    rbcz.hdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (hwnd);

    for(i=rstart; i<rend; i++){
      lpBand = &infoPtr->bands[i];
      oldChild = lpBand->rcChild;

      /* set initial gripper rectangle */
      SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
	     lpBand->rcBand.left, lpBand->rcBand.bottom);

      /* calculate gripper rectangle */
      if ((!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	  (!(lpBand->fStyle & RBBS_FIXEDSIZE)) &&
	  ((lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	   (infoPtr->uNumBands > 1))) {
	lpBand->fDraw |= DRAW_GRIPPER;
	lpBand->rcGripper.left   += 1;
	lpBand->rcGripper.right  = lpBand->rcGripper.left + 3;
	lpBand->rcGripper.top    += 3;
	lpBand->rcGripper.bottom -= 3;

	SetRect (&lpBand->rcCapImage, lpBand->rcGripper.left + GRIPPER_WIDTH,
		 lpBand->rcBand.top, lpBand->rcGripper.left + GRIPPER_WIDTH,
		 lpBand->rcBand.bottom);
      }
      else {
	SetRect (&lpBand->rcCapImage, lpBand->rcBand.left, lpBand->rcBand.top,
		 lpBand->rcBand.left, lpBand->rcBand.bottom);
      }

      /* image is visible */
      if ((lpBand->fMask & RBBIM_IMAGE) && (infoPtr->himl)) {
	lpBand->fDraw |= DRAW_IMAGE;

	lpBand->rcCapImage.right  += infoPtr->imageSize.cx;
	lpBand->rcCapImage.bottom = lpBand->rcCapImage.top + infoPtr->imageSize.cy;

	/* update band height */
	if (lpBand->uMinHeight < infoPtr->imageSize.cy + 2) {
	    lpBand->uMinHeight = infoPtr->imageSize.cy + 2;
	    lpBand->rcBand.bottom = lpBand->rcBand.top + lpBand->uMinHeight;
	}
      }

      /* set initial caption text rectangle */
      SetRect (&lpBand->rcCapText, lpBand->rcCapImage.right, lpBand->rcBand.top+1,
	       lpBand->rcCapImage.right, lpBand->rcBand.bottom-1);

      /* text is visible */
      if ((lpBand->fMask & RBBIM_TEXT) && (lpBand->lpText)) {
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	lpBand->rcCapText.right += (size.cx + 2);

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
      }

      /* set initial child window rectangle if there is a child */
      if (lpBand->fMask & RBBIM_CHILD) {
	if (lpBand->fStyle & RBBS_FIXEDSIZE) {
	  xoff = 0;
	  yoff = 0;
        }
        else {
	  xoff = 4;
	  yoff = 2;
        }
        SetRect (&lpBand->rcChild,
	         lpBand->rcBand.left+lpBand->cxHeader+xoff, lpBand->rcBand.top+yoff,
	         lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
      }
      else {
        SetRect (&lpBand->rcChild,
	         lpBand->rcBand.right, lpBand->rcBand.top,
	         lpBand->rcBand.right, lpBand->rcBand.bottom);
      }

#if 1
      /* do notify is child rectangle changed */
      if (notify && !EqualRect (&oldChild, &lpBand->rcChild)) {
	TRACE("Child rectangle changed for band %u\n", i);
	TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		oldChild.left, oldChild.top,
	        oldChild.right, oldChild.bottom,
		lpBand->rcChild.left, lpBand->rcChild.top,
	        lpBand->rcChild.right, lpBand->rcChild.bottom);
	rbcz.hdr.code = RBN_CHILDSIZE;
	rbcz.uBand = i;
	rbcz.wID = lpBand->wID;
	rbcz.rcChild = lpBand->rcChild;
	rbcz.rcBand = lpBand->rcBand;
	SendMessageA (parenthwnd, WM_NOTIFY, (WPARAM) rbcz.hdr.idFrom, 
		      (LPARAM)&rbcz);
	if (!EqualRect (&lpBand->rcChild, &rbcz.rcChild)) {
	  TRACE("Child rect changed by NOTIFY for band %u\n", i);
	  TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		lpBand->rcChild.left, lpBand->rcChild.top,
		lpBand->rcChild.right, lpBand->rcChild.bottom,
		rbcz.rcChild.left, rbcz.rcChild.top,
		rbcz.rcChild.right, rbcz.rcChild.bottom);
	}
      }
#endif

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
    RECT oldChild;

    rbcz.hdr.hwndFrom = hwnd;
    rbcz.hdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (hwnd);

    for(i=rstart; i<rend; i++){
      lpBand = &infoPtr->bands[i];
      oldChild = lpBand->rcChild;

      /* set initial gripper rectangle */
      SetRect (&lpBand->rcGripper, lpBand->rcBand.left, lpBand->rcBand.top,
	     lpBand->rcBand.right, lpBand->rcBand.top);

      /* calculate gripper rectangle */
      if ((!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	  (!(lpBand->fStyle & RBBS_FIXEDSIZE)) &&
	  ((lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	   (infoPtr->uNumBands > 1))) {
	lpBand->fDraw |= DRAW_GRIPPER;

	if (dwStyle & RBS_VERTICALGRIPPER) {
	    /*  vertical gripper  */
	    lpBand->rcGripper.left   += 3;
	    lpBand->rcGripper.right  = lpBand->rcGripper.left + 3;
	    lpBand->rcGripper.top    += 3;
	    lpBand->rcGripper.bottom = lpBand->rcGripper.top + GRIPPER_HEIGHT;

	    /* initialize Caption image rectangle  */
	    SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
		     lpBand->rcGripper.top + GRIPPER_HEIGHT,
		     lpBand->rcBand.right,
		     lpBand->rcGripper.top + GRIPPER_HEIGHT);
	}
	else {
	    /*  horizontal gripper  */
	    lpBand->rcGripper.left   += 3;
	    lpBand->rcGripper.right  -= 3;
	    lpBand->rcGripper.top    += 3;
	    lpBand->rcGripper.bottom  = lpBand->rcGripper.top + 3;

	    /* initialize Caption image rectangle  */
	    SetRect (&lpBand->rcCapImage, lpBand->rcBand.left,
		     lpBand->rcGripper.top + GRIPPER_WIDTH,
		     lpBand->rcBand.right,
		     lpBand->rcGripper.top + GRIPPER_WIDTH);
	}
      }
      else {
	/* initialize Caption image rectangle  */
	SetRect (&lpBand->rcCapImage, lpBand->rcBand.left, lpBand->rcBand.top,
		 lpBand->rcBand.right, lpBand->rcBand.top);
      }

      /* image is visible */
      if ((lpBand->fMask & RBBIM_IMAGE) && (infoPtr->himl)) {
	lpBand->fDraw |= DRAW_IMAGE;

	lpBand->rcCapImage.right  += infoPtr->imageSize.cx;
	lpBand->rcCapImage.bottom += infoPtr->imageSize.cy;

	/* update band height */
	if (lpBand->uMinHeight < infoPtr->imageSize.cx + 2) {
	    lpBand->uMinHeight = infoPtr->imageSize.cx + 2;
	    lpBand->rcBand.right = lpBand->rcBand.left + lpBand->uMinHeight;
	}
      }

      /* set initial caption text rectangle */
      lpBand->rcCapText.left   = lpBand->rcBand.left + 1;
      lpBand->rcCapText.top    = lpBand->rcCapImage.bottom;
      lpBand->rcCapText.right  = lpBand->rcBand.right - 1;
      lpBand->rcCapText.bottom = lpBand->rcCapText.top;

      /* text is visible */
      if (lpBand->lpText) {
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	lpBand->fDraw |= DRAW_TEXT;
	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	lpBand->rcCapText.bottom += (size.cy + 2);

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
      }

      /* set initial child window rectangle if there is a child */
      if (lpBand->fMask & RBBIM_CHILD) {
	if (lpBand->fStyle & RBBS_FIXEDSIZE) {
	  xoff = 0;
	  yoff = 0;
        }
        else {
	  xoff = 2;
	  yoff = 4;
        }
        SetRect (&lpBand->rcChild,
	         lpBand->rcBand.left+xoff, 
		 lpBand->rcBand.top+lpBand->cxHeader+yoff,
	         lpBand->rcBand.right-xoff, lpBand->rcBand.bottom-yoff);
      }
      else {
        SetRect (&lpBand->rcChild,
	         lpBand->rcBand.right, lpBand->rcBand.top,
	         lpBand->rcBand.right, lpBand->rcBand.top);
      }

#if 1
      /* do notify is child rectangle changed */
      if (notify && !EqualRect (&oldChild, &lpBand->rcChild)) {
	TRACE("Child rectangle changed for band %u\n", i);
	TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
	      oldChild.left, oldChild.top,
	      oldChild.right, oldChild.bottom,
	      lpBand->rcChild.left, lpBand->rcChild.top,
	      lpBand->rcChild.right, lpBand->rcChild.bottom);
	rbcz.hdr.code = RBN_CHILDSIZE;
	rbcz.uBand = i;
	rbcz.wID = lpBand->wID;
	rbcz.rcChild = lpBand->rcChild;
	rbcz.rcBand = lpBand->rcBand;
	SendMessageA (parenthwnd, WM_NOTIFY, (WPARAM) rbcz.hdr.idFrom, 
		      (LPARAM)&rbcz);
	if (!EqualRect (&lpBand->rcChild, &rbcz.rcChild)) {
	  TRACE("Child rect changed by NOTIFY for band %u\n", i);
	  TRACE("    from (%d,%d)-(%d,%d)  to (%d,%d)-(%d,%d)\n",
		lpBand->rcChild.left, lpBand->rcChild.top,
		lpBand->rcChild.right, lpBand->rcChild.bottom,
		rbcz.rcChild.left, rbcz.rcChild.top,
		rbcz.rcChild.right, rbcz.rcChild.bottom);
	}
      }
#endif

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
    REBAR_BAND *lpBand;
    RECT rcClient, rcAdj;
    INT x, y, cx, cxsep, mcy, clientcx, clientcy, adjcx, adjcy, row, rightx, bottomy;
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
    x = 0;
    y = 0;
    rowstartband = 0;
    row = 1;
    cx = 0;
    mcy = 0;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	lpBand->fDraw = 0;
	lpBand->iRow = row;

	if ((lpBand->fStyle & RBBS_HIDDEN) || 
	    ((dwStyle & CCS_VERT) && (lpBand->fStyle & RBBS_NOVERT)))
	    continue;

	cxsep = (x==0) ? 0 : SEP_WIDTH;  /* separator from previous band */
	cx = lpBand->cxHeader +   /* Header: includes gripper, text, image */
             lpBand->hcx;         /* coumpted size of child */

	if (dwStyle & CCS_VERT)
	  dobreak = (y + cx + cxsep > adjcy);
        else
	  dobreak = (x + cx + cxsep > adjcx);
	/* This is the check for whether we need to start a new row */
	if ( (lpBand->fStyle & RBBS_BREAK) ||
	     ( ((dwStyle & CCS_VERT) ? (y != 0) : (x != 0)) && dobreak)) {
	  TRACE("Spliting to new row %d on band %u\n", row+1, i);

	  if (dwStyle & CCS_VERT) {
	    /* first adjust all bands in previous current row to */
	    /* redistribute extra x pixels                       */
	    REBAR_AdjustBands (infoPtr, rowstartband, i,
			       clientcy, y, mcy, dwStyle);
	    /* calculate band subrectangles and break to new row */
	    REBAR_CalcVertBand (hwnd, infoPtr, rowstartband, i,
				notify, dwStyle);
	    y = 0;
	    x += (mcy + 2);
	  }
	  else {
	    /* first adjust all bands in previous current row to */
	    /* redistribute extra x pixels                       */
	    REBAR_AdjustBands (infoPtr, rowstartband, i,
			       clientcx, x, mcy, dwStyle);
	    /* calculate band subrectangles and break to new row */
	    REBAR_CalcHorzBand (hwnd, infoPtr, rowstartband, i,
				notify, dwStyle);
	    x = 0;
	    y += (mcy + 2);
	  }

	  /* FIXME: if not RBS_VARHEIGHT then find max */
	  mcy = 0;
	  cxsep = 0;
	  row++;
	  rowstartband = i;
	  lpBand->iRow = row;
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
	  lpBand->fDraw |= (y==0) ? 0 : DRAW_SEP;
	  rightx = clientcx;
	  bottomy = (lpRect) ? min(clientcy, y+cxsep+cx) : y+cxsep+cx;
	  lpBand->rcBand.left   = x;
	  lpBand->rcBand.right  = y + min(mcy, lpBand->lcy+REBARSPACE);
	  lpBand->rcBand.top    = min(bottomy, y + cxsep);
	  lpBand->rcBand.bottom = bottomy;
	  lpBand->uMinHeight = lpBand->lcy;
	  y = bottomy;
	}
	else {
	  /* bound the right side if we have a bounding rectangle */
	  lpBand->fDraw |= (x==0) ? 0 : DRAW_SEP;
	  rightx = (lpRect) ? min(clientcx, x+cxsep+cx) : x+cxsep+cx;
	  bottomy = clientcy;
	  lpBand->rcBand.left   = min(rightx, x + cxsep);
	  lpBand->rcBand.right  = rightx;
	  lpBand->rcBand.top    = y;
	  lpBand->rcBand.bottom = y + min(mcy, lpBand->lcy+REBARSPACE);
	  lpBand->uMinHeight = lpBand->lcy;
	  x = rightx;
	}
	TRACE("band %u, row %d, (%d,%d)-(%d,%d)\n",
	      i, row,
	      lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom);

    }
    if (infoPtr->uNumBands) {
      infoPtr->uNumRows = row;

      if (dwStyle & CCS_VERT) {
	REBAR_AdjustBands (infoPtr, rowstartband, infoPtr->uNumBands,
			   clientcy, y, mcy, dwStyle);
	REBAR_CalcVertBand (hwnd, infoPtr, rowstartband, infoPtr->uNumBands,
			    notify, dwStyle);
	x += mcy;
      }
      else {
	REBAR_AdjustBands (infoPtr, rowstartband, infoPtr->uNumBands,
			   clientcx, x, mcy, dwStyle);
	REBAR_CalcHorzBand (hwnd, infoPtr, rowstartband, infoPtr->uNumBands, 
			    notify, dwStyle);
	y += mcy;
      }
    }

    /* FIXME: if not RBS_VARHEIGHT then find max mcy and adj rect*/

    if (dwStyle & CCS_VERT) {
	infoPtr->calcSize.cx = x;
	infoPtr->calcSize.cy = clientcy;
    }
    else {
	infoPtr->calcSize.cx = clientcx;
	infoPtr->calcSize.cy = y;
    }
    REBAR_DumpBand (hwnd);
}


static VOID
REBAR_ForceResize (HWND hwnd)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rc;

    TRACE( " to [%ld x %ld]!\n",
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy);

    infoPtr->bAutoResize = TRUE;

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

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];

	if (lpBand->fStyle & RBBS_HIDDEN)
	    continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %x\n", lpBand->hwndChild);

	    GetClassNameA (lpBand->hwndChild, szClassName, 40);
	    if (!lstrcmpA (szClassName, "ComboBox")) {
		INT nEditHeight, yPos;
		RECT rc;

		/* special placement code for combo box */


		/* get size of edit line */
		GetWindowRect (lpBand->hwndChild, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		TRACE("moving child %04x to (%d,%d)-(%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, yPos,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      nEditHeight);
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, /*lpBand->rcChild.top*/ yPos,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    nEditHeight,
			    SWP_SHOWWINDOW);
	    }
#if 0
	    else if (!lstrcmpA (szClassName, WC_COMBOBOXEXA)) {
		INT nEditHeight, yPos;
		RECT rc;
		HWND hwndEdit;

		/* special placement code for extended combo box */

		/* get size of edit line */
		hwndEdit = SendMessageA (lpBand->hwndChild, CBEM_GETEDITCONTROL, 0, 0);
		GetWindowRect (hwndEdit, &rc);
		nEditHeight = rc.bottom - rc.top;
		yPos = (lpBand->rcChild.bottom + lpBand->rcChild.top - nEditHeight)/2;

		/* center combo box inside child area */
		TRACE("moving child %04x to (%d,%d)-(%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, yPos,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      nEditHeight);
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, /*lpBand->rcChild.top*/ yPos,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    nEditHeight,
			    SWP_SHOWWINDOW);

	    }
#endif
	    else {
		TRACE("moving child %04x to (%d,%d)-(%d,%d)\n",
		      lpBand->hwndChild,
		      lpBand->rcChild.left, lpBand->rcChild.top,
		      lpBand->rcChild.right - lpBand->rcChild.left,
		      lpBand->rcChild.bottom - lpBand->rcChild.top);
		SetWindowPos (lpBand->hwndChild, HWND_TOP,
			    lpBand->rcChild.left, lpBand->rcChild.top,
			    lpBand->rcChild.right - lpBand->rcChild.left,
			    lpBand->rcChild.bottom - lpBand->rcChild.top,
			    SWP_SHOWWINDOW);
	    }
	}
    }
}


static VOID
REBAR_ValidateBand (HWND hwnd, REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
     /* Function:  This routine evaluates the band specs supplied */
     /*  by the user and updates the following 5 fields in        */
     /*  the internal band structure: cxHeader, lcx, lcy, hcx, hcy*/
{
    UINT header=0;
    DWORD dwStyle = GetWindowLongA (hwnd, GWL_STYLE);

    lpBand->lcx = 0;
    lpBand->lcy = 0;
    lpBand->hcx = 0;
    lpBand->hcy = 0;

    /* Header is where the image, text and gripper exist  */
    /* in the band and preceed the child window.          */

    /* calculate gripper rectangle */
    if ((!(lpBand->fStyle & RBBS_NOGRIPPER)) &&
	(!(lpBand->fStyle & RBBS_FIXEDSIZE)) &&
	((lpBand->fStyle & RBBS_GRIPPERALWAYS) || 
	 (infoPtr->uNumBands > 1))) {
       if (dwStyle & CCS_VERT)
	  if (dwStyle & RBS_VERTICALGRIPPER)
             header += (GRIPPER_HEIGHT + 3);
          else
	     header += (GRIPPER_WIDTH + 3);
       else
          header += (GRIPPER_WIDTH + 1);
    }

    /* image is visible */
    if ((lpBand->fMask & RBBIM_IMAGE) && (infoPtr->himl)) {
        if (dwStyle & CCS_VERT) {
	   header += infoPtr->imageSize.cy;
	   lpBand->lcy = infoPtr->imageSize.cx + 2;
	}
	else {
	   header += infoPtr->imageSize.cx;
	   lpBand->lcy = infoPtr->imageSize.cy + 2;
	}
    }

    /* text is visible */
    if ((lpBand->fMask & RBBIM_TEXT) && (lpBand->lpText)) {
	HDC hdc = GetDC (0);
	HFONT hOldFont = SelectObject (hdc, infoPtr->hFont);
	SIZE size;

	GetTextExtentPoint32W (hdc, lpBand->lpText,
			       lstrlenW (lpBand->lpText), &size);
	header += ((dwStyle & CCS_VERT) ? (size.cy + 2) : (size.cx + 2));

	SelectObject (hdc, hOldFont);
	ReleaseDC (0, hdc);
    }

    /* check if user overrode the header value */
    if (!(lpBand->fMask & RBBIM_HEADERSIZE))
        lpBand->cxHeader = header;


    /* Now compute minimum size of child window */
    if (lpBand->fMask & RBBIM_CHILDSIZE) {
        lpBand->lcx = lpBand->cxMinChild;
        lpBand->lcy = lpBand->cyMinChild;
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
	lpBand->cyMaxChild = lprbbi->cyMaxChild;
	lpBand->cyChild    = lprbbi->cyChild;
	lpBand->cyIntegral = lprbbi->cyIntegral;
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
	    for (iCount = 0; iCount < infoPtr->uNumBands; iCount++) {
		lpBand = &infoPtr->bands[iCount];
		if (PtInRect (&lpBand->rcBand, *lpPt)) {
		    if (pBand)
			*pBand = iCount;
		    if (PtInRect (&lpBand->rcGripper, *lpPt)) {
			*pFlags = RBHT_GRABBER;
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



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    UINT uBand = (UINT)wParam;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);

    if (infoPtr->uNumBands == 1) {
	TRACE(" simple delete!\n");
	COMCTL32_Free (infoPtr->bands);
	infoPtr->bands = NULL;
	infoPtr->uNumBands = 0;
    }
    else {
	REBAR_BAND *oldBands = infoPtr->bands;
        TRACE("complex delete! [uBand=%u]\n", uBand);

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

    REBAR_Layout (hwnd, NULL, FALSE, FALSE);
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
    /* LPRECT32 lpRect = (LPRECT32)lParam; */
    REBAR_BAND *lpBand;

    if (!lParam)
	return 0;
    if ((UINT)wParam >= infoPtr->uNumBands)
	return 0;

    lpBand = &infoPtr->bands[(UINT)wParam];
    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_BANDBORDERS) {
      /*lpRect.left   = ??? */
      /*lpRect.top    = ??? */
      /*lpRect.right  = ??? */
      /*lpRect.bottom = ??? */
    }
    else {
      /*lpRect.left   = ??? */
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
	lstrcpynWtoA (lprbbi->lpText, lpBand->lpText, lprbbi->cch);
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
	lprbbi->cyMaxChild = lpBand->cyMaxChild;
	lprbbi->cyChild    = lpBand->cyChild;
	lprbbi->cyIntegral = lpBand->cyIntegral;
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
	lprbbi->cyMaxChild = lpBand->cyMaxChild;
	lprbbi->cyChild    = lpBand->cyChild;
	lprbbi->cyIntegral = lpBand->cyIntegral;
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

    REBAR_DumpBandInfo ((LPREBARBANDINFOA)lprbbi);

    return TRUE;
}


static LRESULT
REBAR_GetBarHeight (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    INT nHeight;

    nHeight = infoPtr->calcSize.cy;

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
    lpBand->clrFore = infoPtr->clrText;
    lpBand->clrBack = infoPtr->clrBk;
    lpBand->hwndChild = 0;
    lpBand->hwndPrevParent = 0;

    REBAR_CommonSetupBand (hwnd, lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT len = lstrlenA (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (lpBand->lpText, lprbbi->lpText);
	}
    }

    REBAR_ValidateBand (hwnd, infoPtr, lpBand);

    REBAR_DumpBand (hwnd);

    REBAR_Layout (hwnd, NULL, FALSE, FALSE);
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

    REBAR_DumpBand (hwnd);

    REBAR_Layout (hwnd, NULL, FALSE, FALSE);
    REBAR_ForceResize (hwnd);
    REBAR_MoveChildWindows (hwnd);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u fIdeal = %s)\n",
	   (UINT)wParam, lParam ? "TRUE" : "FALSE");

 
    return 0;
}


static LRESULT
REBAR_MinimizeBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(uBand = %u)\n", (UINT)wParam);

 
    return 0;
}


static LRESULT
REBAR_MoveBand (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
/*    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd); */

    FIXME("(iFrom = %u iTof = %u)\n",
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
	    INT len = lstrlenA (lprbbi->lpText);
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (lpBand->lpText, lprbbi->lpText);
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
	    ShowWindow (lpBand->hwndChild, SW_SHOW);
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

    /* initialize info structure */
    infoPtr->iVersion = 0;
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = GetSysColor (COLOR_BTNTEXT);

    infoPtr->bAutoResize = FALSE;
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
REBAR_GetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


#if 0
static LRESULT
REBAR_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    return 0;
}
#endif


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
REBAR_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
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
    
    /* TEXTMETRIC32A tm; */
    HFONT hFont /*, hOldFont */;
    /* HDC32 hdc; */

    infoPtr->hFont = (HFONT)wParam;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
/*
    hdc = GetDC32 (0);
    hOldFont = SelectObject32 (hdc, hFont);
    GetTextMetrics32A (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject32 (hdc, hOldFont);
    ReleaseDC32 (0, hdc);
*/
    if (lParam) {
/*
        REBAR_Layout (hwnd);
        hdc = GetDC32 (hwnd);
        REBAR_Refresh (hwnd, hdc);
        ReleaseDC32 (hwnd, hdc);
*/
    }

    return 0;
}

static LRESULT
REBAR_Size (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);
    RECT rcClient;

    /* auto resize deadlock check */
    if (infoPtr->bAutoResize) {
	infoPtr->bAutoResize = FALSE;
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
      TRACE("sizing rebar to (%d,%d), client (%d,%d)\n", 
	    LOWORD(lParam), HIWORD(lParam), rcClient.right, rcClient.bottom);
    }

    REBAR_Layout (hwnd, &rcClient, FALSE, TRUE);
    REBAR_ForceResize (hwnd);
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

/*	case RB_GETBANDINFO32:  */ /* outdated, just for compatibility */

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


	case WM_COMMAND:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

	case WM_CREATE:
	    return REBAR_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (hwnd, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (hwnd, wParam, lParam);

/*	case WM_MOUSEMOVE: */
/*	    return REBAR_MouseMove (hwnd, wParam, lParam); */

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (hwnd, wParam, lParam);

	case WM_NCPAINT:
	    return REBAR_NCPaint (hwnd, wParam, lParam);

	case WM_NOTIFY:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);

	case WM_PAINT:
	    return REBAR_Paint (hwnd, wParam);

	case WM_SETCURSOR:
	    return REBAR_SetCursor (hwnd, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (hwnd, wParam, lParam);

	case WM_SIZE:
	    return REBAR_Size (hwnd, wParam, lParam);

/*	case WM_TIMER: */

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
}


VOID
REBAR_Unregister (void)
{
    UnregisterClassA (REBARCLASSNAMEA, (HINSTANCE)NULL);
}

