/*
 * Known bugs that need fixing:
 *
 * 1.  REBAR_ForceResize and REBAR_MoveChildWindows calls moved into 
 *     REBAR_Layout. However order matters on how things work:
 *   if _ForceResize first, IE 4 works
 *   if _ForceResize last,  IE 4 does not draw correctly
 *  **** native does it AT BOTTOM!!!!!
 */
#define MATCH_NATIVE 0
/*
 *
 * 2.  At "FIXME:  problem # 2" WinRAR:
 *   if "#if 1" then last band draws in separate row
 *   if "#if 0" then last band draws in previous row *** just like native ***
 *
 */
#define PROBLEM2 0

/*
 * Rebar control    rev 7c
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
 *   - Fix or implement notifications for RBN_HEIGHTCHANGE, RBN_CHILDSIZE.
 *   - Correct styles RBBS_NOGRIPPER, RBBS_GRIPPERALWAYS, and RBBS_FIXEDSIZE.
 *   - Fix algorithm for Layout and AdjustBand.
 *
 * rev 7
 *  1. Redesign _Layout to better emulate native.
 *     a. Flag rebar when a band needs to be (and all bands) need to
 *        go through rest of _Layout
 *  2. Fix RBN_ENDDRAG contents.
 *  3. REBAR_ForceResize changed to handle shrink of client.
 * rev 7b
 *  4. Support RBBS_HIDDEN in _Layout (phase 2 & 3), _InternalEraseBkgnd,
 *     and _InternalHitTest
 *  5. Support RBS_VARHEIGHT.
 *  6. Rewrite _AdjustBands with new algorithm.
 * rev 7c
 *  7. Format mask and style with labels.
 *  8. Move calls to _ForceResize and _MoveChildWindows into _Layout to
 *     centralize processing.
 *  9. Pass "infoPtr" to all routines instead of "hwnd" to eliminate extra
 *     calls to GetWindowLongA.
 * 10. Use _MoveChildWindows in _HandleLRDrag.
 * 11. Another rewrite of _AdjustBands with new algorithm.
 * 12. Correct drawing of rebar borders and handling of RBS_BORDERS.
 *
 *
 *    Still to do:
 *  2. Following still not handled: RBBS_FIXEDBMP, RBBS_CHILDEDGE,
 *            RBBS_USECHEVRON
 *  4. RBBS_HIDDEN (and the CCS_VERT + RBBS_NOVERT case) is not really 
 *     supported by the following functions:
 *      _HandleLRDrag
 */

#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "wine/unicode.h"
#include "commctrl.h"
/* #include "spy.h" */
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
    UINT    cxMinChild;     /* valid if _CHILDSIZE */
    UINT    cyMinChild;     /* valid if _CHILDSIZE */
    UINT    cx;             /* valid if _SIZE */
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;        /* valid if _CHILDSIZE */
    UINT    cyMaxChild;     /* valid if _CHILDSIZE */
    UINT    cyIntegral;     /* valid if _CHILDSIZE */
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;

    UINT    lcx;            /* minimum cx for band */
    UINT    ccx;            /* current cx for band */
    UINT    hcx;            /* maximum cx for band */
    UINT    lcy;            /* minimum cy for band */
    UINT    ccy;            /* current cy for band */
    UINT    hcy;            /* maximum cy for band */

    SIZE    offChild;       /* x,y offset if child is not FIXEDSIZE */
    UINT    uMinHeight;
    INT     iRow;           /* row this band assigned to */
    UINT    fStatus;        /* status flags, reset only by _Validate */
    UINT    fDraw;          /* drawing flags, reset only by _Layout */
    RECT    rcoldBand;      /* previous calculated band rectangle */
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
    UINT     uNumBands;   /* # of bands in rebar (first=0, last=uNumRows-1 */
    UINT     uNumRows;    /* # of rows of bands (first=1, last=uNumRows */
    HWND     hwndSelf;    /* handle of REBAR window itself */
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
#define BEGIN_DRAG_ISSUED   0x00000001
#define AUTO_RESIZE         0x00000002
#define RESIZE_ANYHOW       0x00000004
#define NTF_HGHTCHG         0x00000008
#define BAND_NEEDS_LAYOUT   0x00000010

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
#define rcBw(b)  ((dwStyle & CCS_VERT) ? (b->rcBand.bottom - b->rcBand.top) : \
		  (b->rcBand.right - b->rcBand.left))
#define ircBlt(b) ((dwStyle & CCS_VERT) ? b->rcBand.left : b->rcBand.top)
#define ircBrb(b) ((dwStyle & CCS_VERT) ? b->rcBand.right : b->rcBand.bottom)
#define ircBw(b)  ((dwStyle & CCS_VERT) ? (b->rcBand.right - b->rcBand.left) : \
		  (b->rcBand.bottom - b->rcBand.top))

/*  The following define determines if a given band is hidden      */
#define HIDDENBAND(a)  (((a)->fStyle & RBBS_HIDDEN) ||   \
                        ((dwStyle & CCS_VERT) &&         \
                         ((a)->fStyle & RBBS_NOVERT)))


#define REBAR_GetInfoPtr(wndPtr) ((REBAR_INFO *)GetWindowLongA (hwnd, 0))


/* "constant values" retrieved when DLL was initialized    */
/* FIXME we do this when the classes are registered.       */
static UINT mindragx = 0;
static UINT mindragy = 0;

static char *band_stylename[] = {
    "RBBS_BREAK",              /* 0001 */
    "RBBS_FIXEDSIZE",          /* 0002 */
    "RBBS_CHILDEDGE",          /* 0004 */
    "RBBS_HIDDEN",             /* 0008 */
    "RBBS_NOVERT",             /* 0010 */
    "RBBS_FIXEDBMP",           /* 0020 */
    "RBBS_VARIABLEHEIGHT",     /* 0040 */
    "RBBS_GRIPPERALWAYS",      /* 0080 */
    "RBBS_NOGRIPPER",          /* 0100 */
    NULL };

static char *band_maskname[] = {
    "RBBIM_STYLE",         /*    0x00000001 */
    "RBBIM_COLORS",        /*    0x00000002 */
    "RBBIM_TEXT",          /*    0x00000004 */
    "RBBIM_IMAGE",         /*    0x00000008 */
    "RBBIM_CHILD",         /*    0x00000010 */
    "RBBIM_CHILDSIZE",     /*    0x00000020 */
    "RBBIM_SIZE",          /*    0x00000040 */
    "RBBIM_BACKGROUND",    /*    0x00000080 */
    "RBBIM_ID",            /*    0x00000100 */
    "RBBIM_IDEALSIZE",     /*    0x00000200 */
    "RBBIM_LPARAM",        /*    0x00000400 */
    "RBBIM_HEADERSIZE",    /*    0x00000800 */
    NULL };


static CHAR line[200];


static CHAR *
REBAR_FmtStyle( UINT style)
{
    INT i = 0;

    *line = 0;
    while (band_stylename[i]) {
	if (style & (1<<i)) {
	    if (*line != 0) strcat(line, " | ");
	    strcat(line, band_stylename[i]);
	}
	i++;
    }
    return line;
}


static CHAR *
REBAR_FmtMask( UINT mask)
{
    INT i = 0;

    *line = 0;
    while (band_maskname[i]) {
	if (mask & (1<<i)) {
	    if (*line != 0) strcat(line, " | ");
	    strcat(line, band_maskname[i]);
	}
	i++;
    }
    return line;
}


static VOID
REBAR_DumpBandInfo( LPREBARBANDINFOA pB)
{
    if( !TRACE_ON(rebar) ) return;
    TRACE("band info: ID=%u, size=%u, child=%04x, clrF=0x%06lx, clrB=0x%06lx\n",
	  pB->wID, pB->cbSize, pB->hwndChild, pB->clrFore, pB->clrBack); 
    TRACE("band info: mask=0x%08x (%s)\n", pB->fMask, REBAR_FmtMask(pB->fMask));
    if (pB->fMask & RBBIM_STYLE)
	TRACE("band info: style=0x%08x (%s)\n", pB->fStyle, REBAR_FmtStyle(pB->fStyle));
    if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_HEADERSIZE | RBBIM_LPARAM )) {
	TRACE("band info:");
	if (pB->fMask & RBBIM_SIZE)
	    DPRINTF(" cx=%u", pB->cx);
	if (pB->fMask & RBBIM_IDEALSIZE)
	    DPRINTF(" xIdeal=%u", pB->cxIdeal);
	if (pB->fMask & RBBIM_HEADERSIZE)
	    DPRINTF(" xHeader=%u", pB->cxHeader);
	if (pB->fMask & RBBIM_LPARAM)
	    DPRINTF(" lParam=0x%08lx", pB->lParam);
	DPRINTF("\n");
    }
    if (pB->fMask & RBBIM_CHILDSIZE)
	TRACE("band info: xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
	      pB->cxMinChild, 
	      pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
}

static VOID
REBAR_DumpBand (REBAR_INFO *iP)
{
    REBAR_BAND *pB;
    UINT i;

    if(! TRACE_ON(rebar) ) return;

    TRACE("hwnd=%04x: color=%08lx/%08lx, bands=%u, rows=%u, cSize=%ld,%ld\n", 
	  iP->hwndSelf, iP->clrText, iP->clrBk, iP->uNumBands, iP->uNumRows,
	  iP->calcSize.cx, iP->calcSize.cy);
    TRACE("hwnd=%04x: flags=%08x, dragStart=%d,%d, dragNow=%d,%d, ihitBand=%d\n",
	  iP->hwndSelf, iP->fStatus, iP->dragStart.x, iP->dragStart.y,
	  iP->dragNow.x, iP->dragNow.y,
	  iP->ihitBand);
    for (i = 0; i < iP->uNumBands; i++) {
	pB = &iP->bands[i];
	TRACE("band # %u: ID=%u, child=%04x, row=%u, clrF=0x%06lx, clrB=0x%06lx\n",
	      i, pB->wID, pB->hwndChild, pB->iRow, pB->clrFore, pB->clrBack);
	TRACE("band # %u: mask=0x%08x (%s)\n", i, pB->fMask, REBAR_FmtMask(pB->fMask));
	if (pB->fMask & RBBIM_STYLE)
	    TRACE("band # %u: style=0x%08x (%s)\n", 
		  i, pB->fStyle, REBAR_FmtStyle(pB->fStyle));
	TRACE("band # %u: uMinH=%u xHeader=%u", 
	      i, pB->uMinHeight, pB->cxHeader);
	if (pB->fMask & (RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_LPARAM )) {
	    if (pB->fMask & RBBIM_SIZE)
		DPRINTF(" cx=%u", pB->cx);
	    if (pB->fMask & RBBIM_IDEALSIZE)
		DPRINTF(" xIdeal=%u", pB->cxIdeal);
	    if (pB->fMask & RBBIM_LPARAM)
		DPRINTF(" lParam=0x%08lx", pB->lParam);
	}
	DPRINTF("\n");
	if (RBBIM_CHILDSIZE)
	    TRACE("band # %u: xMin=%u, yMin=%u, yChild=%u, yMax=%u, yIntgl=%u\n",
		  i, pB->cxMinChild, pB->cyMinChild, pB->cyChild, pB->cyMaxChild, pB->cyIntegral);
	TRACE("band # %u: lcx=%u, ccx=%u, hcx=%u, lcy=%u, ccy=%u, hcy=%u, offChild=%ld,%ld\n",
	      i, pB->lcx, pB->ccx, pB->hcx, pB->lcy, pB->ccy, pB->hcy, pB->offChild.cx, pB->offChild.cy);
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
REBAR_Notify_NMREBAR (REBAR_INFO *infoPtr, UINT uBand, UINT code)
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
    return REBAR_Notify (infoPtr->hwndSelf, (NMHDR *)&notify_rebar, infoPtr, code);
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
REBAR_Refresh (REBAR_INFO *infoPtr, HDC hdc)
{
    REBAR_BAND *lpBand;
    UINT i, oldrow;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

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
REBAR_FixVert (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT mcy, DWORD dwStyle)
     /* Function:                                                    */ 
     /*   Cycle through bands in row and fix height of each band.    */
     /*   Also determine whether each band has changed.              */
     /* On entry:                                                    */
     /*   all bands at desired size.                                 */
     /*   start and end bands are *not* hidden                       */
{
    REBAR_BAND *lpBand;
    INT i;

    for (i = (INT)rowstart; i<=(INT)rowend; i++) {
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;

	/* adjust height of bands in row to "mcy" value */
	if (dwStyle & CCS_VERT) {
	    if (lpBand->rcBand.right != lpBand->rcBand.left + mcy)
	        lpBand->rcBand.right = lpBand->rcBand.left + mcy;
	}
	else {
	    if (lpBand->rcBand.bottom != lpBand->rcBand.top + mcy)
	        lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;

	}

	/* mark whether we need to invalidate this band and trace */
	if ((lpBand->rcoldBand.left !=lpBand->rcBand.left) ||
	    (lpBand->rcoldBand.top !=lpBand->rcBand.top) ||
	    (lpBand->rcoldBand.right !=lpBand->rcBand.right) ||
	    (lpBand->rcoldBand.bottom !=lpBand->rcBand.bottom)) {
	    lpBand->fDraw |= NTF_INVALIDATE;
	    TRACE("band %d row=%d: changed to (%d,%d)-(%d,%d) from (%d,%d)-(%d,%d)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom,
		  lpBand->rcoldBand.left, lpBand->rcoldBand.top,
		  lpBand->rcoldBand.right, lpBand->rcoldBand.bottom);
	}
	else
	    TRACE("band %d row=%d: unchanged (%d,%d)-(%d,%d)\n",
		  i, lpBand->iRow,
		  lpBand->rcBand.left, lpBand->rcBand.top,
		  lpBand->rcBand.right, lpBand->rcBand.bottom);
    }
}


static void
REBAR_AdjustBands (REBAR_INFO *infoPtr, UINT rowstart, UINT rowend,
		   INT maxx, INT mcy, DWORD dwStyle)
     /* Function: This routine distributes the extra space in a row. */
     /*  See algorithm below.                                        */
     /* On entry:                                                    */
     /*   all bands @ ->cxHeader size                                */
     /*   start and end bands are *not* hidden                       */
{
    REBAR_BAND *lpBand;
    UINT x, xsep, extra, curwidth, fudge;
    INT i, last_adjusted;

    TRACE("start=%u, end=%u, max x=%d, max y=%d\n",
	  rowstart, rowend, maxx, mcy);

    /* *******************  Phase 1  ************************ */
    /* Alg:                                                   */
    /*  For each visible band with valid child and not        */
    /*    RBBS_FIXEDSIZE:                                     */
    /*      a. inflate band till either all extra space used  */
    /*         or band's ->ccx reached.                       */
    /*  If any band modified, add any space left to last band */
    /*  adjusted.                                             */ 
    /*                                                        */
    /* ****************************************************** */
    lpBand = &infoPtr->bands[rowend];
    extra = maxx - rcBrb(lpBand);
    x = 0;
    last_adjusted = 0;
    for (i=(INT)rowstart; i<=(INT)rowend; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	xsep = (x == 0) ? 0 : SEP_WIDTH;
	curwidth = rcBw(lpBand);

	/* set new left/top point */
	if (dwStyle & CCS_VERT)
	    lpBand->rcBand.top = x + xsep;
	else
	    lpBand->rcBand.left = x + xsep;

	/* compute new width */
	if (!(lpBand->fStyle & RBBS_FIXEDSIZE) && 
	    lpBand->hwndChild &&
	    extra) {
	    /* set to the "current" band size less the header */
	    fudge = lpBand->ccx;
	    last_adjusted = i;
	    if ((lpBand->fMask & RBBIM_SIZE) && (lpBand->cx > 0) &&
		(fudge > curwidth)) {
		TRACE("adjusting band %d by %d, fudge=%d, curwidth=%d, extra=%d\n",
		      i, fudge-curwidth, fudge, curwidth, extra);
		if ((fudge - curwidth) > extra)
		    fudge = curwidth + extra;
		extra -= (fudge - curwidth);
		curwidth = fudge;
	    }
	    else {
		TRACE("adjusting band %d by %d, fudge=%d, curwidth=%d\n",
		      i, extra, fudge, curwidth);
		curwidth += extra;
		extra = 0;
	    }
	}

	/* set new right/bottom point */
	if (dwStyle & CCS_VERT)
	    lpBand->rcBand.bottom = lpBand->rcBand.top + curwidth;
	else
	    lpBand->rcBand.right = lpBand->rcBand.left + curwidth;
	TRACE("Phase 1 band %d, (%d,%d)-(%d,%d), orig x=%d, xsep=%d\n",
	      i, lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom, x, xsep);
	x = rcBrb(lpBand);
    }
    if ((x >= maxx) || last_adjusted) {
	if (x > maxx) {
	    ERR("Phase 1 failed, x=%d, maxx=%d\n", x, maxx);
	}
	/* done, so spread extra space */
	if (x < maxx) {
	    fudge = maxx - x;
	    TRACE("Need to spread %d on last adjusted band %d\n",
		fudge, last_adjusted);
	    for (i=(INT)last_adjusted; i<=(INT)rowend; i++) {
		lpBand = &infoPtr->bands[i];
		if (HIDDENBAND(lpBand)) continue;

		/* set right/bottom point */
		if (i != last_adjusted) {
		    if (dwStyle & CCS_VERT)
			lpBand->rcBand.top += fudge;
		    else
			lpBand->rcBand.left += fudge;
		}

		/* set left/bottom point */
		if (dwStyle & CCS_VERT)
		    lpBand->rcBand.bottom += fudge;
		else
		    lpBand->rcBand.right += fudge;
	    }
	}
	TRACE("Phase 1 succeeded, used x=%d\n", x);
	REBAR_FixVert (infoPtr, rowstart, rowend, mcy, dwStyle);
 	return;
    }

    /* *******************  Phase 2  ************************ */
    /* Alg:                                                   */
    /*  Find first visible band not RBBS_FIXEDSIZE, put all   */
    /*    extra space there.                                  */
    /*                                                        */
    /* ****************************************************** */

    x = 0;
    for (i=(INT)rowstart; i<=(INT)rowend; i++) {
	lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
	xsep = (x == 0) ? 0 : SEP_WIDTH;
	curwidth = rcBw(lpBand);

	/* set new left/top point */
	if (dwStyle & CCS_VERT)
	    lpBand->rcBand.top = x + xsep;
	else
	    lpBand->rcBand.left = x + xsep;

	/* compute new width */
	if (!(lpBand->fStyle & RBBS_FIXEDSIZE) && extra) {
	    curwidth += extra;
	    extra = 0;
	}

	/* set new right/bottom point */
	if (dwStyle & CCS_VERT)
	    lpBand->rcBand.bottom = lpBand->rcBand.top + curwidth;
	else
	    lpBand->rcBand.right = lpBand->rcBand.left + curwidth;
	TRACE("Phase 2 band %d, (%d,%d)-(%d,%d), orig x=%d, xsep=%d\n",
	      i, lpBand->rcBand.left, lpBand->rcBand.top,
	      lpBand->rcBand.right, lpBand->rcBand.bottom, x, xsep);
	x = rcBrb(lpBand);
    }
    if (x >= maxx) {
	if (x > maxx) {
	    ERR("Phase 2 failed, x=%d, maxx=%d\n", x, maxx);
	}
	/* done, so spread extra space */
	TRACE("Phase 2 succeeded, used x=%d\n", x);
	REBAR_FixVert (infoPtr, rowstart, rowend, mcy, dwStyle);
	return;
    }

    /* *******************  Phase 3  ************************ */
    /* at this point everything is back to ->cxHeader values  */
    /* and should not have gotten here.                       */
    /* ****************************************************** */

    lpBand = &infoPtr->bands[rowstart];
    ERR("Serious problem adjusting row %d, start band %d, end band %d\n",
	lpBand->iRow, rowstart, rowend);
    REBAR_DumpBand (infoPtr);
    return;
}


static void
REBAR_CalcHorzBand (REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify, DWORD dwStyle)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Horizontal bars. ***                   */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    HWND parenthwnd;
    RECT oldChild, work;

    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (infoPtr->hwndSelf);

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
	  InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
      }

    }

}


static VOID
REBAR_CalcVertBand (REBAR_INFO *infoPtr, UINT rstart, UINT rend, BOOL notify, DWORD dwStyle)
     /* Function: this routine initializes all the rectangles in */
     /*  each band in a row to fit in the adjusted rcBand rect.  */
     /* *** Supports only Vertical bars. ***                     */
{
    REBAR_BAND *lpBand;
    UINT i, xoff, yoff;
    HWND parenthwnd;
    RECT oldChild, work;

    /* MS seems to use GetDlgCtrlID() for above GetWindowLong call */
    parenthwnd = GetParent (infoPtr->hwndSelf);

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
	    InvalidateRect(infoPtr->hwndSelf, &work, TRUE);
	}

    }
}


static VOID
REBAR_ForceResize (REBAR_INFO *infoPtr)
     /* Function: This changes the size of the REBAR window to that */
     /*  calculated by REBAR_Layout.                                */
{
    RECT rc;

    /* TEST TEST TEST */
    GetWindowRect (infoPtr->hwndSelf, &rc);
    /* END TEST END TEST END TEST */


    GetClientRect (infoPtr->hwndSelf, &rc);

    TRACE( " old [%ld x %ld], new [%ld x %ld], client [%d x %d]\n",
	   infoPtr->oldSize.cx, infoPtr->oldSize.cy,
	   infoPtr->calcSize.cx, infoPtr->calcSize.cy,
	   rc.right, rc.bottom);

    /* If we need to shrink client, then skip size test */
    if ((infoPtr->calcSize.cy >= rc.bottom) &&
	(infoPtr->calcSize.cx >= rc.right)) {

	/* if size did not change then skip process */
	if ((infoPtr->oldSize.cx == infoPtr->calcSize.cx) &&
	    (infoPtr->oldSize.cy == infoPtr->calcSize.cy) &&
	    !(infoPtr->fStatus & RESIZE_ANYHOW))
	    {
		TRACE("skipping reset\n");
		return;
	    }
    }

    infoPtr->fStatus &= ~RESIZE_ANYHOW;
    /* Set flag to ignore next WM_SIZE message */
    infoPtr->fStatus |= AUTO_RESIZE;

    rc.left = 0;
    rc.top = 0;
    rc.right  = infoPtr->calcSize.cx;
    rc.bottom = infoPtr->calcSize.cy;

    InflateRect (&rc, 0, GetSystemMetrics(SM_CYEDGE));
    /* see comments in _NCCalcSize for reason below is not done */
#if 0
    if (GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE) & WS_BORDER) {
	InflateRect (&rc, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));
    }
#endif

    TRACE("setting to (0,0)-(%d,%d)\n",
	  rc.right - rc.left, rc.bottom - rc.top);
    SetWindowPos (infoPtr->hwndSelf, 0, 0, 0,
		    rc.right - rc.left, rc.bottom - rc.top,
		    SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}


static VOID
REBAR_MoveChildWindows (REBAR_INFO *infoPtr, UINT start, UINT endplus)
{
    REBAR_BAND *lpBand;
    CHAR szClassName[40];
    UINT i;
    NMREBARCHILDSIZE  rbcz;
    NMHDR heightchange;
    HDWP deferpos;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

    if (!(deferpos = BeginDeferWindowPos(infoPtr->uNumBands)))
        ERR("BeginDeferWindowPos returned NULL\n");

    for (i = start; i < endplus; i++) {
	lpBand = &infoPtr->bands[i];

	if (HIDDENBAND(lpBand)) continue;
	if (lpBand->hwndChild) {
	    TRACE("hwndChild = %x\n", lpBand->hwndChild);

#if 0
	    if (lpBand->fDraw & NTF_CHILDSIZE) {
#else
	    if (TRUE) {
#endif
	        lpBand->fDraw &= ~NTF_CHILDSIZE;
		rbcz.uBand = i;
		rbcz.wID = lpBand->wID;
		rbcz.rcChild = lpBand->rcChild;
		rbcz.rcBand = lpBand->rcBand;
		rbcz.rcBand.left += lpBand->cxHeader;
		REBAR_Notify (infoPtr->hwndSelf, (NMHDR *)&rbcz, infoPtr, RBN_CHILDSIZE);
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
        REBAR_Notify (infoPtr->hwndSelf, &heightchange, infoPtr, RBN_HEIGHTCHANGE);
    }
}


static VOID
REBAR_Layout (REBAR_INFO *infoPtr, LPRECT lpRect, BOOL notify, BOOL resetclient)
     /* Function: This routine is resposible for laying out all */
     /*  the bands in a rebar. It assigns each band to a row and*/
     /*  determines when to start a new row.                    */
{
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
    REBAR_BAND *lpBand, *prevBand;
    RECT rcClient, rcAdj;
    INT x, y, cx, cxsep, mmcy, mcy, clientcx, clientcy;
    INT adjcx, adjcy, row, rightx, bottomy, origheight;
    UINT i, j, rowstart;
    BOOL dobreak;

    if (!(infoPtr->fStatus & BAND_NEEDS_LAYOUT)) {
	TRACE("no layout done. No band changed.\n");
	REBAR_DumpBand (infoPtr);
	return;
    }
    infoPtr->fStatus &= ~BAND_NEEDS_LAYOUT;

    GetClientRect (infoPtr->hwndSelf, &rcClient);
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
    rowstart = 0;
    prevBand = NULL;

    for (i = 0; i < infoPtr->uNumBands; i++) {
	lpBand = &infoPtr->bands[i];
	lpBand->fDraw = 0;
	lpBand->iRow = row;

	if (HIDDENBAND(lpBand)) continue;

	lpBand->rcoldBand = lpBand->rcBand;

	/* separator from previous band */
	cxsep = ( ((dwStyle & CCS_VERT) ? y : x)==0) ? 0 : SEP_WIDTH;  

	/* Header: includes gripper, text, image */
	cx = lpBand->cxHeader;   
	if (lpBand->fStyle & RBBS_FIXEDSIZE) cx = lpBand->lcx;

	if (dwStyle & CCS_VERT)
	    dobreak = (y + cx + cxsep > adjcy);
        else
	    dobreak = (x + cx + cxsep > adjcx);

	/* This is the check for whether we need to start a new row */
	if ( ( (lpBand->fStyle & RBBS_BREAK) && (i != 0) ) ||
	     ( ((dwStyle & CCS_VERT) ? (y != 0) : (x != 0)) && dobreak)) {

	    for (j = rowstart; j < i; j++) {
		REBAR_BAND *lpB;
		lpB = &infoPtr->bands[j];
		if (dwStyle & CCS_VERT) {
		    lpB->rcBand.right  = lpB->rcBand.left + mcy;
		}
		else {
		    lpB->rcBand.bottom = lpB->rcBand.top + mcy;
		}
	    }

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
	    rowstart = i;
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

    for (j = rowstart; j < infoPtr->uNumBands; j++) {
	lpBand = &infoPtr->bands[j];
	if (dwStyle & CCS_VERT) {
	    lpBand->rcBand.right  = lpBand->rcBand.left + mcy;
	}
	else {
	    lpBand->rcBand.bottom = lpBand->rcBand.top + mcy;
	}
    }

    if (infoPtr->uNumBands)
        infoPtr->uNumRows = row;

    /* ******* End Phase 1 - all bands on row at minimum size ******* */


    /* ******* Start Phase 1a - Adjust heights for RBS_VARHEIGHT off ******* */

    mmcy = 0;
    if (!(dwStyle & RBS_VARHEIGHT)) {
	INT xy;

	/* get the max height of all bands */
	for (i=0; i<infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];
	    if (HIDDENBAND(lpBand)) continue;
	    if (dwStyle & CCS_VERT)
		mmcy = max(mmcy, lpBand->rcBand.right - lpBand->rcBand.left);
	    else
		mmcy = max(mmcy, lpBand->rcBand.bottom - lpBand->rcBand.top);
	}

	/* now adjust all rectangles by using the height found above */
	xy = 0;
	row = 1;
	for (i=0; i<infoPtr->uNumBands; i++) {
	    lpBand = &infoPtr->bands[i];
	    if (HIDDENBAND(lpBand)) continue;
	    if (lpBand->iRow != row)
		xy += (mmcy + SEP_WIDTH);
	    if (dwStyle & CCS_VERT) {
		lpBand->rcBand.left = xy;
		lpBand->rcBand.right = xy + mmcy;
	    }
	    else {
		lpBand->rcBand.top = xy;
		lpBand->rcBand.bottom = xy + mmcy;
	    }
	}

	/* set the x/y values to the correct maximum */
	if (dwStyle & CCS_VERT)
	    x = xy + mmcy;
	else
	    y = xy + mmcy;
    }

    /* ******* End Phase 1a - Adjust heights for RBS_VARHEIGHT off ******* */


    /* ******* Start Phase 2 - split rows till adjustment height full ******* */

    /* assumes that the following variables contain:                 */
    /*   y/x     current height/width of all rows                    */
    if (lpRect) {
        INT i, j, prev_rh, new_rh, adj_rh, prev_idx, current_idx;
	REBAR_BAND *prev, *current, *walk;

/* FIXME:  problem # 2 */
	if (((dwStyle & CCS_VERT) ? 
#if PROBLEM2
	     (x < adjcx) : (y < adjcy)
#else
	     (adjcx - x > 4) : (adjcy - y > 4)
#endif
	     ) &&
	    (infoPtr->uNumBands > 1)) {
	    for (i=(INT)infoPtr->uNumBands-2; i>=0; i--) {
		TRACE("adjcx=%d, adjcy=%d, x=%d, y=%d\n",
		      adjcx, adjcy, x, y);

		/* find the current band (starts at i+1) */
		current = &infoPtr->bands[i+1];
		current_idx = i+1;
		while (HIDDENBAND(current)) {
		    i--;
		    if (i < 0) break; /* out of bands */
		    current = &infoPtr->bands[i+1];
		    current_idx = i+1;
		}
		if (i < 0) break; /* out of bands */

		/* now find the prev band (starts at i) */
	        prev = &infoPtr->bands[i];
		prev_idx = i;
		while (HIDDENBAND(prev)) {
		    i--;
		    if (i < 0) break; /* out of bands */
		    prev = &infoPtr->bands[i];
		    prev_idx = i;
		}
		if (i < 0) break; /* out of bands */

		prev_rh = ircBw(prev);
		if (prev->iRow == current->iRow) {
		    new_rh = (dwStyle & RBS_VARHEIGHT) ?
			current->lcy + REBARSPACE :
			mmcy;
		    adj_rh = new_rh + SEP_WIDTH;
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
			  current_idx,
			  current->rcBand.left, current->rcBand.top,
			  current->rcBand.right, current->rcBand.bottom);
		    TRACE("prev band %d at (%d,%d)-(%d,%d)\n",
			  prev_idx,
			  prev->rcBand.left, prev->rcBand.top,
			  prev->rcBand.right, prev->rcBand.bottom);
		    TRACE("values: prev_rh=%d, new_rh=%d, adj_rh=%d\n",
			  prev_rh, new_rh, adj_rh);
		    /* for bands below current adjust row # and top/bottom */
		    for (j = current_idx+1; j<infoPtr->uNumBands; j++) {
		        walk = &infoPtr->bands[j];
			if (HIDDENBAND(walk)) continue;
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
	INT bandnum, bandnum_start, bandnum_end;

	/* If RBS_BANDBORDERS set then indicate to draw bottom separator */
	/* on all bands in all rows but last row.                        */
	if (dwStyle & RBS_BANDBORDERS) {
	    for (i = 0; i < infoPtr->uNumBands; i++) {
	        lpBand = &infoPtr->bands[i];
		if (HIDDENBAND(lpBand)) continue;
		if (lpBand->iRow < infoPtr->uNumRows) 
		    lpBand->fDraw |= DRAW_BOTTOMSEP;
	    }
	}

	/* Distribute the extra space on the horizontal and adjust  */
	/* all bands in row to same height.                         */
	bandnum = 0;
	for (i=1; i<=infoPtr->uNumRows; i++) {
	    bandnum_start = -1;
	    bandnum_end = -1;
	    mcy = 0;
	    TRACE("processing row %d, starting band %d\n", i, bandnum);
	    while (TRUE) {
		lpBand = &infoPtr->bands[bandnum];
		if ((bandnum >= infoPtr->uNumBands) ||
		    ((lpBand->iRow != i) &&
		     !HIDDENBAND(lpBand))) {
		    if ((bandnum_start == -1) ||
			(bandnum_end == -1)) {
			ERR("logic error? bands=%d, rows=%d, start=%d, end=%d\n",
			    infoPtr->uNumBands, infoPtr->uNumRows, 
			    (INT)bandnum_start, (INT)bandnum_end);
			ERR("  current row=%d, band=%d\n",
			    i, bandnum);
			break;
		    }
		    REBAR_AdjustBands (infoPtr, bandnum_start, bandnum_end,
				       (dwStyle & CCS_VERT) ? clientcy : 
				       clientcx,     
				       mcy, dwStyle);
		    break;
		}
		if (!HIDDENBAND(lpBand)) {
		    if (bandnum_start == -1) bandnum_start = bandnum;
		    if (bandnum_end < bandnum) bandnum_end = bandnum;
		    if (mcy < ircBw(lpBand)) 
			mcy = ircBw(lpBand);
		}
		bandnum++;
		TRACE("point 1, bandnum=%d\n", bandnum);
	    }
	    TRACE("point 2, i=%d, numrows=%d, bandnum=%d\n",
		  i, infoPtr->uNumRows, bandnum);
	}
	TRACE("point 3\n");

	/* Calculate the other rectangles in each band */
	if (dwStyle & CCS_VERT) {
	    REBAR_CalcVertBand (infoPtr, 0, infoPtr->uNumBands,
				notify, dwStyle);
	}
	else {
	    REBAR_CalcHorzBand (infoPtr, 0, infoPtr->uNumBands, 
				notify, dwStyle);
	}
    }

    /* ******* End Phase 3 - adjust all bands for width full ******* */

    /* now compute size of Rebar itself */
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

    REBAR_DumpBand (infoPtr);

#if !MATCH_NATIVE
    REBAR_ForceResize (infoPtr);
#endif

    REBAR_MoveChildWindows (infoPtr, 0, infoPtr->uNumBands);

#if MATCH_NATIVE
    REBAR_ForceResize (infoPtr);
#endif

}


static VOID
REBAR_ValidateBand (REBAR_INFO *infoPtr, REBAR_BAND *lpBand)
     /* Function:  This routine evaluates the band specs supplied */
     /*  by the user and updates the following 5 fields in        */
     /*  the internal band structure: cxHeader, lcx, lcy, hcx, hcy*/
{
    UINT header=0;
    UINT textheight=0;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

    lpBand->fStatus = 0;
    lpBand->lcx = 0;
    lpBand->lcy = 0;
    lpBand->ccx = 0;
    lpBand->ccy = 0;
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

    /* FIXME: probably should only set NEEDS_LAYOUT flag when */
    /*        values change. Till then always set it.         */
    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;

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
    lpBand->ccy = lpBand->lcy;
    if (lpBand->fMask & RBBIM_CHILDSIZE) {
	if (!(lpBand->fStyle & RBBS_FIXEDSIZE)) {
	    lpBand->offChild.cx = 4;
	    lpBand->offChild.cy = 2;
        }
        lpBand->lcx = lpBand->cxMinChild;

	/* Set the .cy values for CHILDSIZE case */
        lpBand->lcy = max(lpBand->lcy, lpBand->cyMinChild);
	lpBand->ccy = lpBand->lcy;
        lpBand->hcy = lpBand->lcy;
        if (lpBand->cyMaxChild != 0xffffffff) {
	    lpBand->hcy = lpBand->cyMaxChild;
        }
	if (lpBand->cyChild != 0xffffffff)
	    lpBand->ccy = max (lpBand->cyChild, lpBand->lcy);

        TRACE("_CHILDSIZE\n");
    }
    if (lpBand->fMask & RBBIM_SIZE) {
        lpBand->hcx = max (lpBand->cx-lpBand->cxHeader, lpBand->lcx);
        TRACE("_SIZE\n");
    }
    else
        lpBand->hcx = lpBand->lcx;
    lpBand->ccx = lpBand->hcx;

    /* make ->.cx include header size for _Layout */
    lpBand->lcx += lpBand->cxHeader;
    lpBand->ccx += lpBand->cxHeader;
    lpBand->hcx += lpBand->cxHeader;

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
	    /* below in trace fro WinRAR */
	    ShowWindow(lpBand->hwndChild, SW_SHOWNOACTIVATE | SW_SHOWNORMAL);
	    /* above in trace fro WinRAR */
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
REBAR_InternalEraseBkGnd (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam, RECT *clip)
     /* Function:  This erases the background rectangle with the  */
     /*  default brush, then with any band that has a different   */
     /*  background color.                                        */
{
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
    HBRUSH hbrBackground = GetClassWord(infoPtr->hwndSelf, GCW_HBRBACKGROUND);
    RECT eraserect;
    REBAR_BAND *lpBand;
    INT i;

    if (hbrBackground)
        FillRect( (HDC) wParam, clip, hbrBackground);

    for(i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	if (HIDDENBAND(lpBand)) continue;
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
REBAR_InternalHitTest (REBAR_INFO *infoPtr, LPPOINT lpPt, UINT *pFlags, INT *pBand)
{
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
    REBAR_BAND *lpBand;
    RECT rect;
    INT  iCount;

    GetClientRect (infoPtr->hwndSelf, &rect);

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
		if (HIDDENBAND(lpBand)) continue;
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

    avail = rcBw(band) - band->lcx;

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
REBAR_HandleLRDrag (REBAR_INFO *infoPtr, POINTS *ptsmove, DWORD dwStyle)
     /* Function:  This will implement the functionality of a     */
     /*  Gripper drag within a row. It will not implement "out-   */
     /*  of-row" drags. (They are detected and handled in         */
     /*  REBAR_MouseMove.)                                        */
     /*  **** FIXME Switching order of bands in a row not   ****  */
     /*  ****       yet implemented.                        ****  */
{
    REBAR_BAND *hitBand, *band, *prevband, *mindBand, *maxdBand;
    RECT newrect;
    INT imindBand = -1, imaxdBand, ihitBand, i, movement, tempx;
    INT RHeaderSum = 0, LHeaderSum = 0;
    INT compress;

    /* on first significant mouse movement, issue notify */

    if (!(infoPtr->fStatus & BEGIN_DRAG_ISSUED)) {
	if (REBAR_Notify_NMREBAR (infoPtr, -1, RBN_BEGINDRAG)) {
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
	        LHeaderSum += (band->lcx + SEP_WIDTH);
	    else 
	        RHeaderSum += (band->lcx + SEP_WIDTH);

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
    REBAR_DumpBand (infoPtr);

    if (movement < 0) {  

        /* ***  Drag left/up *** */
        compress = rcBlt(hitBand) - rcBlt(mindBand) -
	           LHeaderSum;
	if (compress < abs(movement)) {
	    TRACE("limiting left drag, was %d changed to %d\n",
		  movement, -compress);
	    movement = -compress;
	}

	/*** FIXME: This loop does not support RBBS_HIDDEN bands yet ***/
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
	    band->ccx = rcBw(band);
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
	    band->ccx = rcBw(band);
	}
    }

    /* recompute all rectangles */
    if (dwStyle & CCS_VERT) {
	REBAR_CalcVertBand (infoPtr, imindBand, imaxdBand+1,
			    FALSE, dwStyle);
    }
    else {
	REBAR_CalcHorzBand (infoPtr, imindBand, imaxdBand+1, 
			    FALSE, dwStyle);
    }

    TRACE("bands after adjustment, see band # %d, %d\n",
	  imindBand, imaxdBand);
    REBAR_DumpBand (infoPtr);

    SetRect (&newrect, 
	     mindBand->rcBand.left,
	     mindBand->rcBand.top,
	     maxdBand->rcBand.right,
	     maxdBand->rcBand.bottom);

    REBAR_MoveChildWindows (infoPtr, imindBand, imaxdBand+1);

    InvalidateRect (infoPtr->hwndSelf, &newrect, TRUE);
    UpdateWindow (infoPtr->hwndSelf);

}
#undef READJ
#undef LEADJ



/* << REBAR_BeginDrag >> */


static LRESULT
REBAR_DeleteBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    UINT uBand = (UINT)wParam;
    HWND childhwnd = 0;
    REBAR_BAND *lpBand;

    if (uBand >= infoPtr->uNumBands)
	return FALSE;

    TRACE("deleting band %u!\n", uBand);
    lpBand = &infoPtr->bands[uBand];
    REBAR_Notify_NMREBAR (infoPtr, uBand, RBN_DELETINGBAND);

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

    REBAR_Notify_NMREBAR (infoPtr, -1, RBN_DELETEDBAND);

    /* if only 1 band left the re-validate to possible eliminate gripper */
    if (infoPtr->uNumBands == 1)
      REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    infoPtr->fStatus |= RESIZE_ANYHOW;
    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


/* << REBAR_DragMove >> */
/* << REBAR_EndDrag >> */


static LRESULT
REBAR_GetBandBorders (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRECT lpRect = (LPRECT)lParam;
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

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
    if (dwStyle & RBS_BANDBORDERS) {
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
REBAR_GetBandCount (REBAR_INFO *infoPtr)
{
    TRACE("band count %u!\n", infoPtr->uNumBands);

    return infoPtr->uNumBands;
}


static LRESULT
REBAR_GetBandInfoA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
REBAR_GetBandInfoW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
REBAR_GetBarHeight (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT nHeight;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

    nHeight = (dwStyle & CCS_VERT) ? infoPtr->calcSize.cx : infoPtr->calcSize.cy;

    TRACE("height = %d\n", nHeight);

    return nHeight;
}


static LRESULT
REBAR_GetBarInfo (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
REBAR_GetBkColor (REBAR_INFO *infoPtr)
{
    COLORREF clr = infoPtr->clrBk;

    if (clr == CLR_NONE)
      clr = GetSysColor (COLOR_BTNFACE);

    TRACE("background color 0x%06lx!\n", clr);

    return clr;
}


/* << REBAR_GetColorScheme >> */
/* << REBAR_GetDropTarget >> */


static LRESULT
REBAR_GetPalette (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("empty stub!\n");

    return 0;
}


static LRESULT
REBAR_GetRect (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
REBAR_GetRowCount (REBAR_INFO *infoPtr)
{
    TRACE("%u\n", infoPtr->uNumRows);

    return infoPtr->uNumRows;
}


static LRESULT
REBAR_GetRowHeight (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iRow = (INT)wParam;
    int ret = 0;
    int i, j = 0;
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

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
REBAR_GetTextColor (REBAR_INFO *infoPtr)
{
    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return infoPtr->clrText;
}


inline static LRESULT
REBAR_GetToolTips (REBAR_INFO *infoPtr)
{
    return infoPtr->hwndToolTip;
}


inline static LRESULT
REBAR_GetUnicodeFormat (REBAR_INFO *infoPtr)
{
    return infoPtr->bUnicode;
}


inline static LRESULT
REBAR_GetVersion (REBAR_INFO *infoPtr)
{
    TRACE("version %d\n", infoPtr->iVersion);
    return infoPtr->iVersion;
}


static LRESULT
REBAR_HitTest (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRBHITTESTINFO lprbht = (LPRBHITTESTINFO)lParam; 

    if (!lprbht)
	return -1;

    REBAR_InternalHitTest (infoPtr, &lprbht->pt, &lprbht->flags, &lprbht->iBand);

    return lprbht->iBand;
}


static LRESULT
REBAR_IdToIndex (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
REBAR_InsertBandA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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

    REBAR_CommonSetupBand (infoPtr->hwndSelf, lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
        INT len = MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, NULL, 0 );
        if (len > 1) {
            lpBand->lpText = (LPWSTR)COMCTL32_Alloc (len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, lprbbi->lpText, -1, lpBand->lpText, len );
	}
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (infoPtr);

    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


static LRESULT
REBAR_InsertBandW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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

    REBAR_CommonSetupBand (infoPtr->hwndSelf, (LPREBARBANDINFOA)lprbbi, lpBand);
    lpBand->lpText = NULL;
    if ((lprbbi->fMask & RBBIM_TEXT) && (lprbbi->lpText)) {
	INT len = lstrlenW (lprbbi->lpText);
	if (len > 0) {
	    lpBand->lpText = (LPWSTR)COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (lpBand->lpText, lprbbi->lpText);
	}
    }

    REBAR_ValidateBand (infoPtr, lpBand);
    /* On insert of second band, revalidate band 1 to possible add gripper */
    if (infoPtr->uNumBands == 2)
	REBAR_ValidateBand (infoPtr, &infoPtr->bands[0]);

    REBAR_DumpBand (infoPtr);

    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


static LRESULT
REBAR_MaximizeBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("(uBand = %u fIdeal = %s) stub\n",
	   (UINT)wParam, lParam ? "TRUE" : "FALSE");

    return 0;
}


static LRESULT
REBAR_MinimizeBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("(uBand = %u) stub\n", (UINT)wParam);

    return 0;
}


static LRESULT
REBAR_MoveBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    FIXME("(iFrom = %u iTof = %u) stub\n",
	   (UINT)wParam, (UINT)lParam);

    return FALSE;
}


static LRESULT
REBAR_SetBandInfoA (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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

    REBAR_CommonSetupBand (infoPtr->hwndSelf, lprbbi, lpBand);
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

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE))
      REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


static LRESULT
REBAR_SetBandInfoW (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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

    REBAR_CommonSetupBand (infoPtr->hwndSelf, (LPREBARBANDINFOA)lprbbi, lpBand);
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

    REBAR_ValidateBand (infoPtr, lpBand);

    REBAR_DumpBand (infoPtr);

    if (lprbbi->fMask & (RBBIM_CHILDSIZE | RBBIM_SIZE))
      REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


static LRESULT
REBAR_SetBarInfo (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
	REBAR_ValidateBand (infoPtr, lpBand);
    }

    return TRUE;
}


static LRESULT
REBAR_SetBkColor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;

    TRACE("background color 0x%06lx!\n", infoPtr->clrBk);

    return clrTemp;
}


/* << REBAR_SetColorScheme >> */
/* << REBAR_SetPalette >> */


static LRESULT
REBAR_SetParent (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HWND hwndTemp = infoPtr->hwndNotify;

    infoPtr->hwndNotify = (HWND)wParam;

    return (LRESULT)hwndTemp;
}


static LRESULT
REBAR_SetTextColor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    COLORREF clrTemp;

    clrTemp = infoPtr->clrText;
    infoPtr->clrText = (COLORREF)lParam;

    TRACE("text color 0x%06lx!\n", infoPtr->clrText);

    return clrTemp;
}


/* << REBAR_SetTooltips >> */


inline static LRESULT
REBAR_SetUnicodeFormat (REBAR_INFO *infoPtr, WPARAM wParam)
{
    BOOL bTemp = infoPtr->bUnicode;
    infoPtr->bUnicode = (BOOL)wParam;
    return bTemp;
}


static LRESULT
REBAR_SetVersion (REBAR_INFO *infoPtr, INT iVersion)
{
    INT iOldVersion = infoPtr->iVersion;

    if (iVersion > COMCTL32_VERSION)
	return -1;

    infoPtr->iVersion = iVersion;

    TRACE("new version %d\n", iVersion);

    return iOldVersion;
}


static LRESULT
REBAR_ShowBand (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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

    REBAR_Layout (infoPtr, NULL, TRUE, FALSE);

    return TRUE;
}


static LRESULT
REBAR_SizeToRect (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPRECT lpRect = (LPRECT)lParam;
    RECT t1;

    if (lpRect == NULL)
       return FALSE;

    TRACE("[%d %d %d %d]\n",
	  lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);

    /*  what is going on???? */
    GetWindowRect(infoPtr->hwndSelf, &t1);
    TRACE("window rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);
    GetClientRect(infoPtr->hwndSelf, &t1);
    TRACE("client rect [%d %d %d %d]\n",
	  t1.left, t1.top, t1.right, t1.bottom);

    /* force full _Layout processing */
    TRACE("setting NEEDS_LAYOUT\n");
    infoPtr->fStatus |= BAND_NEEDS_LAYOUT;
    REBAR_Layout (infoPtr, lpRect, TRUE, FALSE);
    return TRUE;
}



static LRESULT
REBAR_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA cs = (LPCREATESTRUCTA) lParam;
    REBAR_INFO *infoPtr;
    RECT wnrc1, clrc1;

    if (TRACE_ON(rebar)) {
	GetWindowRect(hwnd, &wnrc1);
	GetClientRect(hwnd, &clrc1);
	TRACE("window=(%d,%d)-(%d,%d) client=(%d,%d)-(%d,%d) cs=(%d,%d %dx%d)\n",
	      wnrc1.left, wnrc1.top, wnrc1.right, wnrc1.bottom,
	      clrc1.left, clrc1.top, clrc1.right, clrc1.bottom,
	      cs->x, cs->y, cs->cx, cs->cy);
    }

    /* allocate memory for info structure */
    infoPtr = (REBAR_INFO *)COMCTL32_Alloc (sizeof(REBAR_INFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    /* initialize info structure - initial values are 0 */
    infoPtr->clrBk = CLR_NONE;
    infoPtr->clrText = GetSysColor (COLOR_BTNTEXT);
    infoPtr->ihitBand = -1;
    infoPtr->hwndSelf = hwnd;

    infoPtr->hcurArrow = LoadCursorA (0, IDC_ARROWA);
    infoPtr->hcurHorz  = LoadCursorA (0, IDC_SIZEWEA);
    infoPtr->hcurVert  = LoadCursorA (0, IDC_SIZENSA);
    infoPtr->hcurDrag  = LoadCursorA (0, IDC_SIZEA);

    infoPtr->bUnicode = IsWindowUnicode (hwnd);

    if (GetWindowLongA (hwnd, GWL_STYLE) & RBS_AUTOSIZE)
	FIXME("style RBS_AUTOSIZE set!\n");

    SendMessageA (hwnd, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);

    TRACE("created!\n");
    return 0;
}


static LRESULT
REBAR_Destroy (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
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
    SetWindowLongA (infoPtr->hwndSelf, 0, 0);

    /* free rebar info data */
    COMCTL32_Free (infoPtr);
    TRACE("destroyed!\n");
    return 0;
}


static LRESULT
REBAR_EraseBkGnd (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT cliprect;

    if (GetClipBox ( (HDC)wParam, &cliprect))
        return REBAR_InternalEraseBkGnd (infoPtr, wParam, lParam, &cliprect);
    return 0;
}


static LRESULT
REBAR_GetFont (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)infoPtr->hFont;
}


static LRESULT
REBAR_LButtonDown (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *lpBand;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

    /* If InternalHitTest did not find a hit on the Gripper, */
    /* then ignore the button click.                         */
    if (infoPtr->ihitBand == -1) return 0;

    SetCapture (infoPtr->hwndSelf);

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
REBAR_LButtonUp (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    NMHDR layout;
    RECT rect;
    INT ihitBand;

    /* If InternalHitTest did not find a hit on the Gripper, */
    /* then ignore the button click.                         */
    if (infoPtr->ihitBand == -1) return 0;

    ihitBand = infoPtr->ihitBand;
    infoPtr->dragStart.x = 0;
    infoPtr->dragStart.y = 0;
    infoPtr->dragNow = infoPtr->dragStart;
    infoPtr->ihitBand = -1;

    ReleaseCapture ();

    if (infoPtr->fStatus & BEGIN_DRAG_ISSUED) {
        REBAR_Notify(infoPtr->hwndSelf, (NMHDR *) &layout, infoPtr, RBN_LAYOUTCHANGED);
	REBAR_Notify_NMREBAR (infoPtr, ihitBand, RBN_ENDDRAG);
	infoPtr->fStatus &= ~BEGIN_DRAG_ISSUED;
    }

    GetClientRect(infoPtr->hwndSelf, &rect);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
REBAR_MouseMove (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    REBAR_BAND *band1, *band2;
    POINTS ptsmove;
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

    /* Validate entry as hit on Gripper has occured */
    if (GetCapture() != infoPtr->hwndSelf) return 0;
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
	    REBAR_HandleLRDrag (infoPtr, &ptsmove, dwStyle);
	}
    }
    else {
	if ((ptsmove.y < band2->rcBand.top) ||
	    (ptsmove.y > band2->rcBand.bottom) ||
	    ((infoPtr->ihitBand > 0) && (band1->iRow != band2->iRow))) {
	    FIXME("Cannot drag to other rows yet!!\n");
	}
	else {
	    REBAR_HandleLRDrag (infoPtr, &ptsmove, dwStyle);
	}
    }
    return 0;
}


inline static LRESULT
REBAR_NCCalcSize (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    ((LPRECT)lParam)->top    += GetSystemMetrics(SM_CYEDGE);
    ((LPRECT)lParam)->bottom -= GetSystemMetrics(SM_CYEDGE);

    /* While the code below seems to be the reasonable way of   */
    /*  handling the WS_BORDER style, the native version (as    */
    /*  of 4.71 seems to only do the above. Go figure!!         */
#if 0
    if (GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE) & WS_BORDER) {
	((LPRECT)lParam)->left   += GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->top    += GetSystemMetrics(SM_CYEDGE);
	((LPRECT)lParam)->right  -= GetSystemMetrics(SM_CXEDGE);
	((LPRECT)lParam)->bottom -= GetSystemMetrics(SM_CYEDGE);
    }
#endif

    return 0;
}


static LRESULT
REBAR_NCPaint (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
    RECT rcWindow;
    HDC hdc;

    if (dwStyle & WS_MINIMIZE)
	return 0; /* Nothing to do */

    DefWindowProcA (infoPtr->hwndSelf, WM_NCPAINT, wParam, lParam);

    if (!(hdc = GetDCEx( infoPtr->hwndSelf, 0, DCX_USESTYLE | DCX_WINDOW )))
	return 0;

    if (dwStyle & WS_BORDER) {
	GetWindowRect (infoPtr->hwndSelf, &rcWindow);
	OffsetRect (&rcWindow, -rcWindow.left, -rcWindow.top);
	TRACE("rect (%d,%d)-(%d,%d)\n",
	      rcWindow.left, rcWindow.top,
	      rcWindow.right, rcWindow.bottom);
	/* see comments in _NCCalcSize for reason this is not done */
	/* DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_RECT); */
	DrawEdge (hdc, &rcWindow, EDGE_ETCHED, BF_TOP | BF_BOTTOM);
    }

    ReleaseDC( infoPtr->hwndSelf, hdc );

    return 0;
}


static LRESULT
REBAR_Paint (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(infoPtr->hwndSelf, &rc);
    hdc = wParam==0 ? BeginPaint (infoPtr->hwndSelf, &ps) : (HDC)wParam;

    TRACE("painting (%d,%d)-(%d,%d) client (%d,%d)-(%d,%d)\n",
	  ps.rcPaint.left, ps.rcPaint.top,
	  ps.rcPaint.right, ps.rcPaint.bottom,
	  rc.left, rc.top, rc.right, rc.bottom);

    if (ps.fErase) {
	/* Erase area of paint if requested */
        REBAR_InternalEraseBkGnd (infoPtr, wParam, lParam, &ps.rcPaint);
    }

    REBAR_Refresh (infoPtr, hdc);
    if (!wParam)
	EndPaint (infoPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
REBAR_SetCursor (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);
    POINT pt;
    UINT  flags;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (infoPtr->hwndSelf, &pt);

    REBAR_InternalHitTest (infoPtr, &pt, &flags, NULL);

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
REBAR_SetFont (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT rcClient;
    REBAR_BAND *lpBand;
    UINT i;

    infoPtr->hFont = (HFONT)wParam;

    /* revalidate all bands to change sizes of text in headers of bands */
    for (i=0; i<infoPtr->uNumBands; i++) {
        lpBand = &infoPtr->bands[i];
	REBAR_ValidateBand (infoPtr, lpBand);
    }


    if (lParam) {
        GetClientRect (infoPtr->hwndSelf, &rcClient);
        REBAR_Layout (infoPtr, &rcClient, FALSE, TRUE);
    }

    return 0;
}


static LRESULT
REBAR_Size (REBAR_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    RECT rcClient;

    /* auto resize deadlock check */
    if (infoPtr->fStatus & AUTO_RESIZE) {
	DWORD dwStyle = GetWindowLongA (infoPtr->hwndSelf, GWL_STYLE);

	infoPtr->fStatus &= ~AUTO_RESIZE;
	TRACE("AUTO_RESIZE was set, reset, fStatus=%08x lparam=%08lx\n",
	      infoPtr->fStatus, lParam);
	if (dwStyle & RBS_AUTOSIZE) {
	    NMRBAUTOSIZE autosize;

	    GetClientRect(infoPtr->hwndSelf, &autosize.rcTarget);
	    autosize.fChanged = 0;  /* ??? */
	    autosize.rcActual = autosize.rcTarget;  /* ??? */
	    REBAR_Notify(infoPtr->hwndSelf, (NMHDR *) &autosize, infoPtr, RBN_AUTOSIZE);
	    TRACE("RBN_AUTOSIZE client=%d, lp=%08lx\n", 
		  autosize.rcTarget.bottom, lParam);
	}
	return 0;
    }

    GetClientRect (infoPtr->hwndSelf, &rcClient);
    if ((lParam == 0) && (rcClient.right == 0) && (rcClient.bottom == 0)) {
      /* native control seems to do this */
      GetClientRect (GetParent(infoPtr->hwndSelf), &rcClient);
      TRACE("sizing rebar, message and client zero, parent client (%d,%d)\n", 
	    rcClient.right, rcClient.bottom);
    }
    else {
      TRACE("sizing rebar from (%ld,%ld) to (%d,%d), client (%d,%d)\n", 
	    infoPtr->calcSize.cx, infoPtr->calcSize.cy,
	    LOWORD(lParam), HIWORD(lParam),
	    rcClient.right, rcClient.bottom);
    }

    if ((LOWORD(lParam) == 764) && (HIWORD(lParam) == 91)) {
	/* #include "../../../bomb.h" */
    }

    if ((infoPtr->calcSize.cx != rcClient.right) ||
	(infoPtr->calcSize.cy != rcClient.bottom))
	infoPtr->fStatus |= BAND_NEEDS_LAYOUT;

    REBAR_Layout (infoPtr, &rcClient, TRUE, TRUE);
    infoPtr->fStatus &= ~AUTO_RESIZE;

    return 0;
}


static LRESULT WINAPI
REBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    REBAR_INFO *infoPtr = REBAR_GetInfoPtr (hwnd);

    TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", 
	  hwnd, uMsg, /* SPY_GetMsgName(uMsg), */ wParam, lParam);
    if (!infoPtr && (uMsg != WM_CREATE))
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
/*	case RB_BEGINDRAG: */

	case RB_DELETEBAND:
	    return REBAR_DeleteBand (infoPtr, wParam, lParam);

/*	case RB_DRAGMOVE: */
/*	case RB_ENDDRAG: */

	case RB_GETBANDBORDERS:
	    return REBAR_GetBandBorders (infoPtr, wParam, lParam);

	case RB_GETBANDCOUNT:
	    return REBAR_GetBandCount (infoPtr);

	case RB_GETBANDINFO:	/* obsoleted after IE3, but we have to
				   support it anyway. */
	case RB_GETBANDINFOA:
	    return REBAR_GetBandInfoA (infoPtr, wParam, lParam);

	case RB_GETBANDINFOW:
	    return REBAR_GetBandInfoW (infoPtr, wParam, lParam);

	case RB_GETBARHEIGHT:
	    return REBAR_GetBarHeight (infoPtr, wParam, lParam);

	case RB_GETBARINFO:
	    return REBAR_GetBarInfo (infoPtr, wParam, lParam);

	case RB_GETBKCOLOR:
	    return REBAR_GetBkColor (infoPtr);

/*	case RB_GETCOLORSCHEME: */
/*	case RB_GETDROPTARGET: */

	case RB_GETPALETTE:
	    return REBAR_GetPalette (infoPtr, wParam, lParam);

	case RB_GETRECT:
	    return REBAR_GetRect (infoPtr, wParam, lParam);

	case RB_GETROWCOUNT:
	    return REBAR_GetRowCount (infoPtr);

	case RB_GETROWHEIGHT:
	    return REBAR_GetRowHeight (infoPtr, wParam, lParam);

	case RB_GETTEXTCOLOR:
	    return REBAR_GetTextColor (infoPtr);

	case RB_GETTOOLTIPS:
	    return REBAR_GetToolTips (infoPtr);

	case RB_GETUNICODEFORMAT:
	    return REBAR_GetUnicodeFormat (infoPtr);

	case CCM_GETVERSION:
	    return REBAR_GetVersion (infoPtr);

	case RB_HITTEST:
	    return REBAR_HitTest (infoPtr, wParam, lParam);

	case RB_IDTOINDEX:
	    return REBAR_IdToIndex (infoPtr, wParam, lParam);

	case RB_INSERTBANDA:
	    return REBAR_InsertBandA (infoPtr, wParam, lParam);

	case RB_INSERTBANDW:
	    return REBAR_InsertBandW (infoPtr, wParam, lParam);

	case RB_MAXIMIZEBAND:
	    return REBAR_MaximizeBand (infoPtr, wParam, lParam);

	case RB_MINIMIZEBAND:
	    return REBAR_MinimizeBand (infoPtr, wParam, lParam);

	case RB_MOVEBAND:
	    return REBAR_MoveBand (infoPtr, wParam, lParam);

	case RB_SETBANDINFOA:
	    return REBAR_SetBandInfoA (infoPtr, wParam, lParam);

	case RB_SETBANDINFOW:
	    return REBAR_SetBandInfoW (infoPtr, wParam, lParam);

	case RB_SETBARINFO:
	    return REBAR_SetBarInfo (infoPtr, wParam, lParam);

	case RB_SETBKCOLOR:
	    return REBAR_SetBkColor (infoPtr, wParam, lParam);

/*	case RB_SETCOLORSCHEME: */
/*	case RB_SETPALETTE: */
/*	    return REBAR_GetPalette (infoPtr, wParam, lParam); */

	case RB_SETPARENT:
	    return REBAR_SetParent (infoPtr, wParam, lParam);

	case RB_SETTEXTCOLOR:
	    return REBAR_SetTextColor (infoPtr, wParam, lParam);

/*	case RB_SETTOOLTIPS: */

	case RB_SETUNICODEFORMAT:
	    return REBAR_SetUnicodeFormat (infoPtr, wParam);

	case CCM_SETVERSION:
	    return REBAR_SetVersion (infoPtr, (INT)wParam);

	case RB_SHOWBAND:
	    return REBAR_ShowBand (infoPtr, wParam, lParam);

	case RB_SIZETORECT:
	    return REBAR_SizeToRect (infoPtr, wParam, lParam);


/*    Messages passed to parent */
	case WM_COMMAND:
	case WM_DRAWITEM:
	case WM_NOTIFY:
	    return SendMessageA (GetParent (hwnd), uMsg, wParam, lParam);


/*      case WM_CHARTOITEM:     supported according to ControlSpy */

	case WM_CREATE:
	    return REBAR_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return REBAR_Destroy (infoPtr, wParam, lParam);

        case WM_ERASEBKGND:
	    return REBAR_EraseBkGnd (infoPtr, wParam, lParam);

	case WM_GETFONT:
	    return REBAR_GetFont (infoPtr, wParam, lParam);

/*      case WM_LBUTTONDBLCLK:  supported according to ControlSpy */

	case WM_LBUTTONDOWN:
	    return REBAR_LButtonDown (infoPtr, wParam, lParam);

	case WM_LBUTTONUP:
	    return REBAR_LButtonUp (infoPtr, wParam, lParam);

/*      case WM_MEASUREITEM:    supported according to ControlSpy */

	case WM_MOUSEMOVE:
	    return REBAR_MouseMove (infoPtr, wParam, lParam);

	case WM_NCCALCSIZE:
	    return REBAR_NCCalcSize (infoPtr, wParam, lParam);

/*      case WM_NCCREATE:       supported according to ControlSpy */
/*      case WM_NCHITTEST:      supported according to ControlSpy */

	case WM_NCPAINT:
	    return REBAR_NCPaint (infoPtr, wParam, lParam);

/*      case WM_NOTIFYFORMAT:   supported according to ControlSpy */

	case WM_PAINT:
	    return REBAR_Paint (infoPtr, wParam, lParam);

/*      case WM_PALETTECHANGED: supported according to ControlSpy */
/*      case WM_PRINTCLIENT:    supported according to ControlSpy */
/*      case WM_QUERYNEWPALETTE:supported according to ControlSpy */
/*      case WM_RBUTTONDOWN:    supported according to ControlSpy */
/*      case WM_RBUTTONUP:      supported according to ControlSpy */

	case WM_SETCURSOR:
	    return REBAR_SetCursor (infoPtr, wParam, lParam);

	case WM_SETFONT:
	    return REBAR_SetFont (infoPtr, wParam, lParam);

/*      case WM_SETREDRAW:      supported according to ControlSpy */

	case WM_SIZE:
	    return REBAR_Size (infoPtr, wParam, lParam);

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

