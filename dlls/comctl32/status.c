/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 * Copyright 1998 Eric Kohl
 */

#include "windows.h"
#include "commctrl.h"
#include "status.h"
#include "win.h"
#include "debug.h"

/*
 * Run tests using Waite Group Windows95 API Bible Vol. 1&2
 * The second cdrom contains executables drawstat.exe,gettext.exe,
 * simple.exe, getparts.exe, setparts.exe, statwnd.exe
 */

/*
 * Fixme/Todo
 * 1) Don't hard code bar to bottom of window, allow CCS_TOP also.
 + 2) Tooltip support (almost done).
 */

#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define HORZ_BORDER 0
#define VERT_BORDER 2
#define HORZ_GAP    2

#define STATUSBAR_GetInfoPtr(wndPtr) ((STATUSWINDOWINFO *)wndPtr->wExtra[0])


static void
STATUSBAR_DrawSizeGrip (HDC32 hdc, LPRECT32 lpRect)
{
    HPEN32 hOldPen;
    POINT32 pt;
    INT32 i;

    pt.x = lpRect->right - 1;
    pt.y = lpRect->bottom - 1;

    hOldPen = SelectObject32 (hdc, GetSysColorPen32 (COLOR_3DFACE));
    MoveToEx32 (hdc, pt.x - 12, pt.y, NULL);
    LineTo32 (hdc, pt.x, pt.y);
    LineTo32 (hdc, pt.x, pt.y - 12);

    pt.x--;
    pt.y--;

    SelectObject32 (hdc, GetSysColorPen32 (COLOR_3DSHADOW));
    for (i = 1; i < 11; i += 4) {
	MoveToEx32 (hdc, pt.x - i, pt.y, NULL);
	LineTo32 (hdc, pt.x, pt.y - i);

	MoveToEx32 (hdc, pt.x - i-1, pt.y, NULL);
	LineTo32 (hdc, pt.x, pt.y - i-1);
    }

    SelectObject32 (hdc, GetSysColorPen32 (COLOR_3DHIGHLIGHT));
    for (i = 3; i < 13; i += 4) {
	MoveToEx32 (hdc, pt.x - i, pt.y, NULL);
	LineTo32 (hdc, pt.x, pt.y - i);
    }

    SelectObject32 (hdc, hOldPen);
}


static void 
STATUSBAR_DrawPart (HDC32 hdc, STATUSWINDOWPART *part)
{
    RECT32 r = part->bound;
    UINT32 border = BDR_SUNKENOUTER;

    if (part->style==SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (part->style==SBT_NOBORDERS)
      border = 0;

    DrawEdge32(hdc, &r, border, BF_RECT|BF_ADJUST);

    /* draw the icon */
    if (part->hIcon) {
	INT32 cy = r.bottom - r.top;

	r.left += 2;
	DrawIconEx32 (hdc, r.left, r.top, part->hIcon, cy, cy, 0, 0, DI_NORMAL);
	r.left += cy;
    }

    /* now draw text */
    if (part->text) {
      int oldbkmode = SetBkMode32(hdc, TRANSPARENT);
      LPSTR p = (LPSTR)part->text;
      UINT32 align = DT_LEFT;
      if (*p == '\t') {
	p++;
	align = DT_CENTER;

	if (*p == '\t') {
	  p++;
	  align = DT_RIGHT;
	}
      }
      r.left += 3;
      DrawText32A(hdc, p, lstrlen32A(p), &r, align|DT_VCENTER|DT_SINGLELINE);
      if (oldbkmode != TRANSPARENT)
	SetBkMode32(hdc, oldbkmode);
    }
}


static VOID
STATUSBAR_RefreshPart (WND *wndPtr, STATUSWINDOWPART *part, HDC32 hdc)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    HBRUSH32 hbrBk;
    HFONT32  hOldFont;

    if (!IsWindowVisible32(wndPtr->hwndSelf))
        return;

    if (self->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush32 (self->clrBk);
    else
	hbrBk = GetSysColorBrush32 (COLOR_3DFACE);
    FillRect32(hdc, &part->bound, hbrBk);

    hOldFont = SelectObject32 (hdc, self->hFont ? self->hFont : self->hDefaultFont);

    if (part->style == SBT_OWNERDRAW) {
	DRAWITEMSTRUCT32 dis;

	dis.CtlID = wndPtr->wIDmenu;
	dis.itemID = -1;
	dis.hwndItem = wndPtr->hwndSelf;
	dis.hDC = hdc;
	dis.rcItem = part->bound;
	dis.itemData = (INT32)part->text;
	SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_DRAWITEM,
			(WPARAM32)wndPtr->wIDmenu, (LPARAM)&dis);
    }
    else
	STATUSBAR_DrawPart (hdc, part);

    SelectObject32 (hdc, hOldFont);

    if (self->clrBk != CLR_DEFAULT)
	DeleteObject32 (hbrBk);

    if (wndPtr->dwStyle & SBARS_SIZEGRIP) {
	RECT32 rect;

	GetClientRect32 (wndPtr->hwndSelf, &rect);
	STATUSBAR_DrawSizeGrip (hdc, &rect);
    }
}


static BOOL32
STATUSBAR_Refresh (WND *wndPtr, HDC32 hdc)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    int      i;
    RECT32   rect;
    HBRUSH32 hbrBk;
    HFONT32  hOldFont;

    if (!IsWindowVisible32(wndPtr->hwndSelf))
        return (TRUE);

    GetClientRect32 (wndPtr->hwndSelf, &rect);

    if (self->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush32 (self->clrBk);
    else
	hbrBk = GetSysColorBrush32 (COLOR_3DFACE);
    FillRect32(hdc, &rect, hbrBk);

    hOldFont = SelectObject32 (hdc, self->hFont ? self->hFont : self->hDefaultFont);

    if (self->simple) {
	STATUSBAR_DrawPart (hdc, &self->part0);
    }
    else {
	for (i = 0; i < self->numParts; i++) {
	    if (self->parts[i].style == SBT_OWNERDRAW) {
		DRAWITEMSTRUCT32 dis;

		dis.CtlID = wndPtr->wIDmenu;
		dis.itemID = -1;
		dis.hwndItem = wndPtr->hwndSelf;
		dis.hDC = hdc;
		dis.rcItem = self->parts[i].bound;
		dis.itemData = (INT32)self->parts[i].text;
		SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_DRAWITEM,
				(WPARAM32)wndPtr->wIDmenu, (LPARAM)&dis);
	    }
	    else
		STATUSBAR_DrawPart (hdc, &self->parts[i]);
	}
    }

    SelectObject32 (hdc, hOldFont);

    if (self->clrBk != CLR_DEFAULT)
	DeleteObject32 (hbrBk);

    if (wndPtr->dwStyle & SBARS_SIZEGRIP)
	STATUSBAR_DrawSizeGrip (hdc, &rect);

    return TRUE;
}


static void
STATUSBAR_SetPartBounds (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    RECT32 rect, *r;
    int	i;

    /* get our window size */
    GetClientRect32 (wndPtr->hwndSelf, &rect);

    rect.top += VERT_BORDER;

    /* set bounds for simple rectangle */
    self->part0.bound = rect;

    /* set bounds for non-simple rectangles */
    for (i = 0; i < self->numParts; i++) {
	part = &self->parts[i];
	r = &self->parts[i].bound;
	r->top = rect.top;
	r->bottom = rect.bottom;
	if (i == 0)
	    r->left = 0;
	else
	    r->left = self->parts[i-1].bound.right + HORZ_GAP;
	if (part->x == -1)
	    r->right = rect.right;
	else
	    r->right = part->x;

	if (self->hwndToolTip) {
	    TTTOOLINFO32A ti;

	    ti.cbSize = sizeof(TTTOOLINFO32A);
	    ti.hwnd = wndPtr->hwndSelf;
	    ti.uId = i;
	    ti.rect = *r;
	    SendMessage32A (self->hwndToolTip, TTM_NEWTOOLRECT32A,
			    0, (LPARAM)&ti);
	}
    }
}


static VOID
STATUSBAR_RelayEvent (HWND32 hwndTip, HWND32 hwndMsg, UINT32 uMsg,
		      WPARAM32 wParam, LPARAM lParam)
{
    MSG32 msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessage32A (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}


__inline__ static LRESULT
STATUSBAR_GetBorders (LPARAM lParam)
{
    LPINT32 out = (LPINT32) lParam;

    out[0] = HORZ_BORDER; /* horizontal border width */
    out[1] = VERT_BORDER; /* vertical border width */
    out[2] = HORZ_GAP; /* width of border between rectangles */

    return TRUE;
}


static LRESULT
STATUSBAR_GetIcon (WND *wndPtr, WPARAM32 wParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT32 nPart;

    nPart = (INT32)wParam & 0x00ff;
    if ((nPart < -1) || (nPart >= self->numParts)) return 0;

    if (nPart == -1)
        return (self->part0.hIcon);
    else
        return (self->parts[nPart].hIcon);
}


static LRESULT
STATUSBAR_GetParts (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    LPINT32 parts;
    INT32   num_parts;
    int	    i;

    num_parts = (INT32) wParam;
    parts = (LPINT32) lParam;
    if (parts) {
	return (self->numParts);
	for (i = 0; i < num_parts; i++) {
	    parts[i] = self->parts[i].x;
	}
    }
    return (self->numParts);
}


static LRESULT
STATUSBAR_GetRect (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    int	part_num;
    LPRECT32  rect;

    part_num = ((INT32) wParam) & 0x00ff;
    rect = (LPRECT32) lParam;
    if (self->simple)
	*rect = self->part0.bound;
    else
	*rect = self->parts[part_num].bound;
    return TRUE;
}


static LRESULT
STATUSBAR_GetText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int	part_num;
    LRESULT result;
    LPSTR   out_text;

    part_num = ((INT32) wParam) & 0x00ff;
    out_text = (LPSTR) lParam;
    if (self->simple)
	part = &self->part0;
    else
	part = &self->parts[part_num];

    if (part->style == SBT_OWNERDRAW)
	result = (LRESULT) part->text;
    else {
	result = part->text ? lstrlen32A(part->text) : 0;
	result |= (part->style << 16);
	if (out_text) {
	    lstrcpy32A(out_text, part->text);
	}
    }
    return result;
}


//  << STATUSBAR_GetText32W >>


static LRESULT
STATUSBAR_GetTextLength32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int	part_num;
    DWORD	result;

    part_num = ((INT32) wParam) & 0x00ff;

    if (self->simple)
	part = &self->part0;
    else
	part = &self->parts[part_num];

    if (part->text)
	result = lstrlen32A(part->text);
    else
	result = 0;

    result |= (part->style << 16);
    return result;
}


//  << STATUSBAR_GetTextLength32W >>


static LRESULT
STATUSBAR_GetTipText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    if (infoPtr->hwndToolTip) {
	TTTOOLINFO32A ti;

	ti.cbSize = sizeof(TTTOOLINFO32A);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = LOWORD(wParam);
	SendMessage32A (infoPtr->hwndToolTip, TTM_GETTEXT32A, 0, (LPARAM)&ti);

	if (ti.lpszText)
	    lstrcpyn32A ((LPSTR)lParam, ti.lpszText, HIWORD(wParam));
    }

    return 0;
}


//  << STATUSBAR_GetTipText32W >>
//  << STATUSBAR_GetUnicodeFormat >>


__inline__ static LRESULT
STATUSBAR_IsSimple (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    return self->simple;
}


static LRESULT
STATUSBAR_SetBkColor (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    COLORREF oldBkColor;
    HDC32    hdc;

    oldBkColor = self->clrBk;
    self->clrBk = (COLORREF)lParam;
    hdc = GetDC32 (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return oldBkColor;
}


static LRESULT
STATUSBAR_SetIcon (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT32 nPart = (INT32)wParam & 0x00ff;
    HDC32 hdc; 

    if ((nPart < -1) || (nPart >= self->numParts)) return FALSE;

    hdc = GetDC32 (wndPtr->hwndSelf);
    if (nPart == -1) {
	self->part0.hIcon = (HICON32)lParam;
	if (self->simple)
	    STATUSBAR_RefreshPart (wndPtr, &self->part0, hdc);
    }
    else {
	self->parts[nPart].hIcon = (HICON32)lParam;
	if (!(self->simple))
	    STATUSBAR_RefreshPart (wndPtr, &self->parts[nPart], hdc);
    }
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetMinHeight (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    if (IsWindowVisible32 (wndPtr->hwndSelf)) {
	HWND32 parent = GetParent32 (wndPtr->hwndSelf);
	INT32  width, x, y;
	RECT32 parent_rect;

	GetClientRect32 (parent, &parent_rect);
	self->height = (INT32)wParam + VERT_BORDER;
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - self->height;
	MoveWindow32 (wndPtr->hwndSelf, parent_rect.left,
		      parent_rect.bottom - self->height,
		      width, self->height, TRUE);
	STATUSBAR_SetPartBounds (wndPtr);
    }

    return TRUE;
}


static LRESULT
STATUSBAR_SetParts (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    HDC32	hdc;
    LPINT32 parts;
    STATUSWINDOWPART *	tmp;
    int	i;
    int	oldNumParts;

    if (self->simple)
	self->simple = FALSE;

    oldNumParts = self->numParts;
    self->numParts = (INT32) wParam;
    parts = (LPINT32) lParam;
    if (oldNumParts > self->numParts) {
	for (i = self->numParts ; i < oldNumParts; i++) {
	    if (self->parts[i].text && (self->parts[i].style != SBT_OWNERDRAW))
		COMCTL32_Free (self->parts[i].text);
	}
    }
    else if (oldNumParts < self->numParts) {
	tmp = COMCTL32_Alloc (sizeof(STATUSWINDOWPART) * self->numParts);
	for (i = 0; i < oldNumParts; i++) {
	    tmp[i] = self->parts[i];
	}
	if (self->parts)
	    COMCTL32_Free (self->parts);
	self->parts = tmp;
    }
    
    for (i = 0; i < self->numParts; i++) {
	self->parts[i].x = parts[i];
    }

    if (self->hwndToolTip) {
	INT32 nTipCount =
	    SendMessage32A (self->hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);

	if (nTipCount < self->numParts) {
	    /* add tools */
	    TTTOOLINFO32A ti;
	    INT32 i;

	    ZeroMemory (&ti, sizeof(TTTOOLINFO32A));
	    ti.cbSize = sizeof(TTTOOLINFO32A);
	    ti.hwnd = wndPtr->hwndSelf;
	    for (i = nTipCount; i < self->numParts; i++) {
		TRACE (statusbar, "add tool %d\n", i);
		ti.uId = i;
		SendMessage32A (self->hwndToolTip, TTM_ADDTOOL32A,
				0, (LPARAM)&ti);
	    }
	}
	else if (nTipCount > self->numParts) {
	    /* delete tools */
	    INT32 i;

	    for (i = nTipCount - 1; i >= self->numParts; i--) {

		TRACE (statusbar, "delete tool %d\n", i);

	    }
	}
    }

    STATUSBAR_SetPartBounds (wndPtr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int	part_num;
    int	style;
    LPSTR text;
    int	len;
    HDC32 hdc;

    text = (LPSTR) lParam;
    part_num = ((INT32) wParam) & 0x00ff;
    style = ((INT32) wParam) & 0xff00;

    if ((self->simple) || (self->parts==NULL) || (part_num==255))
	part = &self->part0;
    else
	part = &self->parts[part_num];
    if (!part) return FALSE;
    part->style = style;
    if (style == SBT_OWNERDRAW) {
	part->text = text;
    }
    else {
	/* duplicate string */
	if (part->text)
	    COMCTL32_Free (part->text);
	part->text = 0;
	if (text && (len = lstrlen32A(text))) {
	    part->text = COMCTL32_Alloc (len+1);
	    lstrcpy32A(part->text, text);
	}
    }

    hdc = GetDC32 (wndPtr->hwndSelf);
    STATUSBAR_RefreshPart (wndPtr, part, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


//  << STATUSBAR_SetText32W >>


static LRESULT
STATUSBAR_SetTipText32A (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    TTTOOLINFO32A ti;

    TRACE (statusbar, "part %d: \"%s\"\n", (INT32)wParam, (LPSTR)lParam);
    if (self->hwndToolTip) {
	ti.cbSize = sizeof(TTTOOLINFO32A);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = (INT32)wParam;
	ti.hinst = 0;
	ti.lpszText = (LPSTR)lParam;
	SendMessage32A (self->hwndToolTip, TTM_UPDATETIPTEXT32A,
			0, (LPARAM)&ti);
    }

    return 0;
}


//  << STATUSBAR_SetTipText32W >>
//  << STATUSBAR_SetUnicodeFormat >>


static LRESULT
STATUSBAR_Simple (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    BOOL32 simple;
    HDC32  hdc;
    NMHDR  nmhdr;

    simple = (BOOL32) wParam;
    self->simple = simple;

    /* send notification */
    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = SBN_SIMPLEMODECHANGE;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    0, (LPARAM)&nmhdr);

    hdc = GetDC32 (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_WMCreate (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    LPCREATESTRUCT32A lpCreate = (LPCREATESTRUCT32A) lParam;
    NONCLIENTMETRICS32A nclm;
    RECT32	rect;
    int	        width, len;
    HDC32	hdc;
    STATUSWINDOWINFO *self;

    self = (STATUSWINDOWINFO*)COMCTL32_Alloc (sizeof(STATUSWINDOWINFO));
    wndPtr->wExtra[0] = (DWORD)self;

    self->numParts = 1;
    self->parts = 0;
    self->simple = FALSE;
    self->clrBk = CLR_DEFAULT;
    self->hFont = 0;
    GetClientRect32 (wndPtr->hwndSelf, &rect);

    nclm.cbSize = sizeof(NONCLIENTMETRICS32A);
    SystemParametersInfo32A (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    self->hDefaultFont = CreateFontIndirect32A (&nclm.lfStatusFont);

    /* initialize simple case */
    self->part0.bound = rect;
    self->part0.text = 0;
    self->part0.x = 0;
    self->part0.style = 0;
    self->part0.hIcon = 0;

    /* initialize first part */
    self->parts = COMCTL32_Alloc (sizeof(STATUSWINDOWPART));
    self->parts[0].bound = rect;
    self->parts[0].text = 0;
    self->parts[0].x = -1;
    self->parts[0].style = 0;
    self->parts[0].hIcon = 0;

    if ((len = lstrlen32A (lpCreate->lpszName))) {
        self->parts[0].text = COMCTL32_Alloc (len + 1);
        lstrcpy32A (self->parts[0].text, lpCreate->lpszName);
    }

    if ((hdc = GetDC32 (0))) {
	TEXTMETRIC32A tm;
	HFONT32 hOldFont;

	hOldFont = SelectObject32 (hdc,self->hDefaultFont);
	GetTextMetrics32A(hdc, &tm);
	self->textHeight = tm.tmHeight;
	SelectObject32 (hdc, hOldFont);
	ReleaseDC32(0, hdc);
    }

    if (wndPtr->dwStyle & SBT_TOOLTIPS) {
	self->hwndToolTip =
	    CreateWindowEx32A (0, TOOLTIPS_CLASS32A, NULL, 0,
			       CW_USEDEFAULT32, CW_USEDEFAULT32,
			       CW_USEDEFAULT32, CW_USEDEFAULT32,
			       wndPtr->hwndSelf, 0,
			       wndPtr->hInstance, NULL);

	if (self->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hdr.hwndFrom = wndPtr->hwndSelf;
	    nmttc.hdr.idFrom = wndPtr->wIDmenu;
	    nmttc.hdr.code = NM_TOOLTIPSCREATED;
	    nmttc.hwndToolTips = self->hwndToolTip;

	    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
			    (WPARAM32)wndPtr->wIDmenu, (LPARAM)&nmttc);
	}
    }

    GetClientRect32 (GetParent32 (wndPtr->hwndSelf), &rect);
    width = rect.right - rect.left;
    self->height = self->textHeight + 4 + VERT_BORDER;
    MoveWindow32 (wndPtr->hwndSelf, lpCreate->x, lpCreate->y-1,
		  width, self->height, FALSE);
    STATUSBAR_SetPartBounds (wndPtr);

    return 0;
}


static LRESULT
STATUSBAR_WMDestroy (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    int	i;

    for (i = 0; i < self->numParts; i++) {
	if (self->parts[i].text && (self->parts[i].style != SBT_OWNERDRAW))
	    COMCTL32_Free (self->parts[i].text);
    }
    if (self->part0.text && (self->part0.style != SBT_OWNERDRAW))
	COMCTL32_Free (self->part0.text);
    COMCTL32_Free (self->parts);

    /* delete default font */
    if (self->hDefaultFont)
	DeleteObject32 (self->hDefaultFont);

    /* delete tool tip control */
    if (self->hwndToolTip)
	DestroyWindow32 (self->hwndToolTip);

    COMCTL32_Free (self);

    return 0;
}


static __inline__ LRESULT
STATUSBAR_WMGetFont (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    return self->hFont;
}


static LRESULT
STATUSBAR_WMGetText (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT32 len;

    if (!(self->parts[0].text))
        return 0;
    len = lstrlen32A (self->parts[0].text);
    if (wParam > len) {
        lstrcpy32A ((LPSTR)lParam, self->parts[0].text);
        return len;
    }
    else
        return -1;
}


static LRESULT
STATUSBAR_WMGetTextLength (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    if (!(self->parts[0].text))
        return 0;

    return (lstrlen32A (self->parts[0].text));
}


__inline__ static LRESULT
STATUSBAR_WMMouseMove (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    if (self->hwndToolTip)
	STATUSBAR_RelayEvent (self->hwndToolTip, wndPtr->hwndSelf,
			      WM_MOUSEMOVE, wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMNCHitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    if (wndPtr->dwStyle & SBARS_SIZEGRIP) {
	RECT32  rect;
	POINT32 pt;

	GetClientRect32 (wndPtr->hwndSelf, &rect);

	pt.x = (INT32)LOWORD(lParam);
	pt.y = (INT32)HIWORD(lParam);
	ScreenToClient32 (wndPtr->hwndSelf, &pt);

	rect.left = rect.right - 13;
	rect.top += 2;

	if (PtInRect32 (&rect, pt))
	    return HTBOTTOMRIGHT;
    }

    return DefWindowProc32A (wndPtr->hwndSelf, WM_NCHITTEST, wParam, lParam);
}


static __inline__ LRESULT
STATUSBAR_WMNCLButtonDown (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    PostMessage32A (wndPtr->parent->hwndSelf, WM_NCLBUTTONDOWN,
		    wParam, lParam);
    return 0;
}


static __inline__ LRESULT
STATUSBAR_WMNCLButtonUp (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    PostMessage32A (wndPtr->parent->hwndSelf, WM_NCLBUTTONUP,
		    wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMPaint (WND *wndPtr, WPARAM32 wParam)
{
    HDC32 hdc;
    PAINTSTRUCT32 ps;

    hdc = wParam==0 ? BeginPaint32 (wndPtr->hwndSelf, &ps) : (HDC32)wParam;
    STATUSBAR_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint32 (wndPtr->hwndSelf, &ps);

    return 0;
}


static LRESULT
STATUSBAR_WMSetFont (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    self->hFont = (HFONT32)wParam;
    if (LOWORD(lParam) == TRUE) {
	HDC32 hdc = GetDC32 (wndPtr->hwndSelf);
        STATUSBAR_Refresh (wndPtr, hdc);
        ReleaseDC32 (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
STATUSBAR_WMSetText (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int len;
    HDC32 hdc;

    if (self->numParts == 0)
	return FALSE;

    part = &self->parts[0];
    /* duplicate string */
    if (part->text)
        COMCTL32_Free (part->text);
    part->text = 0;
    if (lParam && (len = lstrlen32A((LPCSTR)lParam))) {
        part->text = COMCTL32_Alloc (len+1);
        lstrcpy32A (part->text, (LPCSTR)lParam);
    }

    hdc = GetDC32 (wndPtr->hwndSelf);
    STATUSBAR_RefreshPart (wndPtr, part, hdc);
    ReleaseDC32 (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_WMSize (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT32	width, x, y, flags;
    RECT32	parent_rect;
    HWND32	parent;

    /* Need to resize width to match parent */
    flags = (INT32) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED, SIZE_RESTORED
     */

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent32 (wndPtr->hwndSelf);
	GetClientRect32 (parent, &parent_rect);
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - self->height;
	MoveWindow32 (wndPtr->hwndSelf, parent_rect.left, 
		      parent_rect.bottom - self->height,
		      width, self->height, TRUE);
	STATUSBAR_SetPartBounds (wndPtr);
    }
    return 0;
}


static LRESULT
STATUSBAR_SendNotify (WND *wndPtr, UINT32 code)
{
    NMHDR  nmhdr;

    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = code;
    SendMessage32A (GetParent32 (wndPtr->hwndSelf), WM_NOTIFY,
		    0, (LPARAM)&nmhdr);
    return 0;
}



LRESULT WINAPI
StatusWindowProc (HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr = WIN_FindWndPtr (hwnd);

    switch (msg) {
	case SB_GETBORDERS:
	    return STATUSBAR_GetBorders (lParam);

	case SB_GETICON:
	    return STATUSBAR_GetIcon (wndPtr, wParam);

	case SB_GETPARTS:
	    return STATUSBAR_GetParts (wndPtr, wParam, lParam);

	case SB_GETRECT:
	    return STATUSBAR_GetRect (wndPtr, wParam, lParam);

	case SB_GETTEXT32A:
	    return STATUSBAR_GetText32A (wndPtr, wParam, lParam);

//	case SB_GETTEXT32W:

	case SB_GETTEXTLENGTH32A:
	    return STATUSBAR_GetTextLength32A (wndPtr, wParam, lParam);

//	case SB_GETTEXTLENGHT32W:

	case SB_GETTIPTEXT32A:
	    return STATUSBAR_GetTipText32A (wndPtr, wParam, lParam);

//	case SB_GETTIPTEXT32W:
//	case SB_GETUNICODEFORMAT:

	case SB_ISSIMPLE:
	    return STATUSBAR_IsSimple (wndPtr);

	case SB_SETBKCOLOR:
	    return STATUSBAR_SetBkColor (wndPtr, wParam, lParam);

	case SB_SETICON:
	    return STATUSBAR_SetIcon (wndPtr, wParam, lParam);

	case SB_SETMINHEIGHT:
	    return STATUSBAR_SetMinHeight (wndPtr, wParam, lParam);

	case SB_SETPARTS:	
	    return STATUSBAR_SetParts (wndPtr, wParam, lParam);

	case SB_SETTEXT32A:
	    return STATUSBAR_SetText32A (wndPtr, wParam, lParam);

//	case SB_SETTEXT32W:

	case SB_SETTIPTEXT32A:
	    return STATUSBAR_SetTipText32A (wndPtr, wParam, lParam);

//	case SB_SETTIPTEXT32W:
//	case SB_SETUNICODEFORMAT:

	case SB_SIMPLE:
	    return STATUSBAR_Simple (wndPtr, wParam, lParam);


	case WM_CREATE:
	    return STATUSBAR_WMCreate (wndPtr, wParam, lParam);

	case WM_DESTROY:
	    return STATUSBAR_WMDestroy (wndPtr);

	case WM_GETFONT:
            return STATUSBAR_WMGetFont (wndPtr);

	case WM_GETTEXT:
            return STATUSBAR_WMGetText (wndPtr, wParam, lParam);

	case WM_GETTEXTLENGTH:
            return STATUSBAR_WMGetTextLength (wndPtr);

	case WM_LBUTTONDBLCLK:
            return STATUSBAR_SendNotify (wndPtr, NM_DBLCLK);

	case WM_LBUTTONUP:
	    return STATUSBAR_SendNotify (wndPtr, NM_CLICK);

	case WM_MOUSEMOVE:
            return STATUSBAR_WMMouseMove (wndPtr, wParam, lParam);

	case WM_NCHITTEST:
            return STATUSBAR_WMNCHitTest (wndPtr, wParam, lParam);

	case WM_NCLBUTTONDOWN:
	    return STATUSBAR_WMNCLButtonDown (wndPtr, wParam, lParam);

	case WM_NCLBUTTONUP:
	    return STATUSBAR_WMNCLButtonUp (wndPtr, wParam, lParam);

	case WM_PAINT:
	    return STATUSBAR_WMPaint (wndPtr, wParam);

	case WM_RBUTTONDBLCLK:
	    return STATUSBAR_SendNotify (wndPtr, NM_RDBLCLK);

	case WM_RBUTTONUP:
	    return STATUSBAR_SendNotify (wndPtr, NM_RCLICK);

	case WM_SETFONT:
	    return STATUSBAR_WMSetFont (wndPtr, wParam, lParam);

	case WM_SETTEXT:
	    return STATUSBAR_WMSetText (wndPtr, wParam, lParam);

	case WM_SIZE:
	    return STATUSBAR_WMSize (wndPtr, wParam, lParam);

	default:
	    if (msg >= WM_USER)
		ERR (statusbar, "unknown msg %04x wp=%04x lp=%08lx\n",
		     msg, wParam, lParam);
	    return DefWindowProc32A (hwnd, msg, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 * STATUS_Register [Internal]
 *
 * Registers the status window class.
 */

VOID
STATUS_Register (VOID)
{
    WNDCLASS32A wndClass;

    if (GlobalFindAtom32A (STATUSCLASSNAME32A)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASS32A));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC32)StatusWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(STATUSWINDOWINFO *);
    wndClass.hCursor       = LoadCursor32A (0, IDC_ARROW32A);
    wndClass.hbrBackground = (HBRUSH32)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = STATUSCLASSNAME32A;
 
    RegisterClass32A (&wndClass);
}


/***********************************************************************
 * STATUS_Unregister [Internal]
 *
 * Unregisters the status window class.
 */

VOID
STATUS_Unregister (VOID)
{
    if (GlobalFindAtom32A (STATUSCLASSNAME32A))
	UnregisterClass32A (STATUSCLASSNAME32A, (HINSTANCE32)NULL);
}

