/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 * Copyright 1998 Eric Kohl
 */

#include "windows.h"
#include "status.h"
#include "commctrl.h"
#include "heap.h"
#include "win.h"

/*
 * Run tests using Waite Group Windows95 API Bible Vol. 1&2
 * The second cdrom contains executables drawstat.exe,gettext.exe,
 * simple.exe, getparts.exe, setparts.exe, statwnd.exe
 */

/*
 * Fixme/Todo
 * 1) Don't hard code bar to bottom of window, allow CCS_TOP also.
 + 2) Tooltip support.
 */

#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define HORZ_BORDER 0
#define VERT_BORDER 2
#define HORZ_GAP    2


static STATUSWINDOWINFO *GetStatusInfo(HWND32 hwnd)
{
    WND *wndPtr;

    wndPtr = WIN_FindWndPtr(hwnd);
    return ((STATUSWINDOWINFO *) wndPtr->wExtra[0]);
}


static void
STATUS_DrawSizeGrip (HDC32 hdc, LPRECT32 lpRect)
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
SB_DrawPart( HDC32 hdc, LPRECT32 lprc, HICON32 hIcon,
             LPCSTR text, UINT32 style )
{
    RECT32 r = *lprc;
    UINT32 border = BDR_SUNKENOUTER;

    if (style==SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (style==SBT_NOBORDERS)
      border = 0;

    DrawEdge32(hdc, &r, border, BF_RECT|BF_ADJUST);

    /* draw the icon */
    if (hIcon) {
	INT32 cy = r.bottom - r.top;

	r.left += 2;
	DrawIconEx32 (hdc, r.left, r.top, hIcon, cy, cy, 0, 0, DI_NORMAL);
	r.left += cy;
    }

    /* now draw text */
    if (text) {
      int oldbkmode = SetBkMode32(hdc, TRANSPARENT);
      LPSTR p = (LPSTR)text;
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


static BOOL32
SW_Refresh( HWND32 hwnd, HDC32 hdc, STATUSWINDOWINFO *self )
{
    int      i;
    RECT32   rect;
    HBRUSH32 hbrBk;
    HFONT32 hOldFont;
    WND *wndPtr;

    wndPtr = WIN_FindWndPtr(hwnd);

    if (!IsWindowVisible32(hwnd)) {
        return (TRUE);
    }

    GetClientRect32 (hwnd, &rect);

    if (self->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush32 (self->clrBk);
    else
	hbrBk = GetSysColorBrush32 (COLOR_3DFACE);
    FillRect32(hdc, &rect, hbrBk);

    hOldFont = SelectObject32 (hdc, self->hFont ? self->hFont : self->hDefaultFont);

    if (self->simple) {
	SB_DrawPart (hdc,
		     &self->part0.bound,
		     self->part0.hIcon,
		     self->part0.text,
		     self->part0.style);
    }
    else {
	for (i = 0; i < self->numParts; i++) {
	    if (self->parts[i].style == SBT_OWNERDRAW) {
		DRAWITEMSTRUCT32 dis;
		WND *wndPtr = WIN_FindWndPtr(hwnd);

		dis.CtlID = wndPtr->wIDmenu;
		dis.itemID = -1;
		dis.hwndItem = hwnd;
		dis.hDC = hdc;
		dis.rcItem = self->part0.bound;
		dis.itemData = (INT32)self->part0.text;
		SendMessage32A (GetParent32 (hwnd), WM_DRAWITEM, 
				(WPARAM32)wndPtr->wIDmenu, (LPARAM)&dis);
	    }
	    else
		SB_DrawPart (hdc,
			     &self->parts[i].bound,
			     self->parts[i].hIcon,
			     self->parts[i].text,
			     self->parts[i].style);
	}
    }

    SelectObject32 (hdc, hOldFont);

    if (self->clrBk != CLR_DEFAULT)
	DeleteObject32 (hbrBk);

    if (wndPtr->dwStyle & SBARS_SIZEGRIP)
	STATUS_DrawSizeGrip (hdc, &rect);

    return TRUE;
}


__inline__ static LRESULT
SW_GetBorders (LPARAM lParam)
{
    LPINT32 out = (LPINT32) lParam;

    out[0] = HORZ_BORDER; /* horizontal border width */
    out[1] = VERT_BORDER; /* vertical border width */
    out[2] = HORZ_GAP; /* width of border between rectangles */

    return TRUE;
}

static void
SW_SetPartBounds(HWND32 hwnd, STATUSWINDOWINFO *self)
{
    int	i;
    RECT32	rect, *r;
    STATUSWINDOWPART *part;

    /* get our window size */
    GetClientRect32(hwnd, &rect);

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
    }
}

static LRESULT
SW_SetText(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    int	part_num;
    int	style;
    LPSTR	text;
    int	len;
    STATUSWINDOWPART *part;

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
	    HeapFree(SystemHeap, 0, part->text);
	part->text = 0;
	if (text && (len = lstrlen32A(text))) {
	    part->text = HeapAlloc(SystemHeap, 0, len+1);
	    lstrcpy32A(part->text, text);
	}
    }
    InvalidateRect32(hwnd, &part->bound, FALSE);
//    SW_RefreshPart (hdc, part);
    return TRUE;
}

static LRESULT
SW_SetParts(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    HDC32	hdc;
    LPINT32 parts;
    STATUSWINDOWPART *	tmp;
    int	i;
    int	oldNumParts;

    if (self->simple) {
	self->simple = FALSE;
    }
    oldNumParts = self->numParts;
    self->numParts = (INT32) wParam;
    parts = (LPINT32) lParam;
    if (oldNumParts > self->numParts) {
	for (i = self->numParts ; i < oldNumParts; i++) {
	    if (self->parts[i].text && (self->parts[i].style != SBT_OWNERDRAW))
		HeapFree(SystemHeap, 0, self->parts[i].text);
	}
    }
    else if (oldNumParts < self->numParts) {
	tmp = HeapAlloc(SystemHeap, HEAP_ZERO_MEMORY,
			sizeof(STATUSWINDOWPART) * self->numParts);
	for (i = 0; i < oldNumParts; i++) {
	    tmp[i] = self->parts[i];
	}
	if (self->parts)
	    HeapFree(SystemHeap, 0, self->parts);
	self->parts = tmp;
    }
    
    for (i = 0; i < self->numParts; i++) {
	self->parts[i].x = parts[i];
    }
    SW_SetPartBounds(hwnd, self);

    hdc = GetDC32(hwnd);
    SW_Refresh(hwnd, hdc, self);
    ReleaseDC32(hwnd, hdc);
    return TRUE;
}

static LRESULT
SW_GetParts(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    LPINT32 parts;
    INT32	num_parts;
    int	i;

    self = GetStatusInfo(hwnd);
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
SW_Create(HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    LPCREATESTRUCT32A lpCreate = (LPCREATESTRUCT32A) lParam;
    NONCLIENTMETRICS32A nclm;
    RECT32	rect;
    int	        width, len;
    HDC32	hdc;
    HWND32	parent;
    WND *wndPtr;
    STATUSWINDOWINFO *self;

    wndPtr = WIN_FindWndPtr(hwnd);
    self = (STATUSWINDOWINFO*)HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
					 sizeof(STATUSWINDOWINFO));
    wndPtr->wExtra[0] = (DWORD)self;

    self->numParts = 1;
    self->parts = 0;
    self->simple = FALSE;
    self->clrBk = CLR_DEFAULT;
    self->hFont = 0;
    GetClientRect32(hwnd, &rect);

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
    self->parts = HeapAlloc (SystemHeap, HEAP_ZERO_MEMORY,
			     sizeof(STATUSWINDOWPART));
    self->parts[0].bound = rect;
    self->parts[0].text = 0;
    self->parts[0].x = -1;
    self->parts[0].style = 0;
    self->parts[0].hIcon = 0;

    if ((len = lstrlen32A (lpCreate->lpszName))) {
        self->parts[0].text = HeapAlloc (SystemHeap, 0, len + 1);
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

    parent = GetParent32 (hwnd);
    GetClientRect32 (parent, &rect);
    width = rect.right - rect.left;
    self->height = self->textHeight + 4 + VERT_BORDER;
    MoveWindow32 (hwnd, lpCreate->x, lpCreate->y-1, width, self->height, FALSE);
    SW_SetPartBounds (hwnd, self);
    return 0;
}

static LRESULT
SW_GetRect(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
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
SW_GetText(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    int	part_num;
    LRESULT	result;
    STATUSWINDOWPART *part;
    LPSTR	out_text;

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

static LRESULT
SW_GetTextLength(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    int	part_num;
    STATUSWINDOWPART *part;
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

static LRESULT
SW_SetMinHeight(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    INT32	width, x, y;
    RECT32	parent_rect;
    HWND32	parent;

    if (IsWindowVisible32 (hwnd)) {
	parent = GetParent32(hwnd);
	GetClientRect32(parent, &parent_rect);
	self->height = (INT32)wParam + VERT_BORDER;
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - self->height;
	MoveWindow32(hwnd, parent_rect.left, parent_rect.bottom - self->height,
		     width, self->height, TRUE);
	SW_SetPartBounds(hwnd, self);
    }

    return TRUE;
}

static LRESULT
SW_SetBkColor(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    COLORREF oldBkColor;
    HDC32    hdc;

    oldBkColor = self->clrBk;
    self->clrBk = (COLORREF)lParam;
    hdc = GetDC32(hwnd);
    SW_Refresh(hwnd, hdc, self);
    ReleaseDC32(hwnd, hdc);
    return oldBkColor;
}


static LRESULT
SW_SetIcon (STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    INT32  nPart = (INT32)wParam & 0x00ff;

    if ((nPart < -1) || (nPart >= self->numParts)) return FALSE;

    if (nPart == -1) {
        self->part0.hIcon = (HICON32)lParam;
        if (self->simple) 
            InvalidateRect32(hwnd, &self->part0.bound, FALSE);
    }
    else {
        self->parts[nPart].hIcon = (HICON32)lParam;
        if (!(self->simple))
            InvalidateRect32(hwnd, &self->parts[nPart].bound, FALSE);
    }
    return TRUE;
}

static LRESULT
SW_GetIcon(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    INT32    nPart;

    nPart = (INT32)wParam & 0x00ff;
    if ((nPart < -1) || (nPart >= self->numParts)) return 0;

    if (nPart == -1)
        return (self->part0.hIcon);
    else
        return (self->parts[nPart].hIcon);
}

static LRESULT
SW_Simple(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    WND *wndPtr;
    BOOL32 simple;
    HDC32  hdc;
    NMHDR  nmhdr;

    wndPtr = WIN_FindWndPtr(hwnd);

    simple = (BOOL32) wParam;
    self->simple = simple;

    /* send notification */
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = SBN_SIMPLEMODECHANGE;
    SendMessage32A (GetParent32 (hwnd), WM_NOTIFY, 0, (LPARAM)&nmhdr);

    hdc = GetDC32(hwnd);
    SW_Refresh(hwnd, hdc, self);
    ReleaseDC32(hwnd, hdc);

    return TRUE;
}


static LRESULT
SW_Destroy(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    int	i;

    for (i = 0; i < self->numParts; i++) {
	if (self->parts[i].text && (self->parts[i].style != SBT_OWNERDRAW))
	    HeapFree(SystemHeap, 0, self->parts[i].text);
    }
    if (self->part0.text && (self->part0.style != SBT_OWNERDRAW))
	HeapFree(SystemHeap, 0, self->part0.text);
    HeapFree(SystemHeap, 0, self->parts);

    /* delete default font */
    if (self->hDefaultFont)
	DeleteObject32 (self->hDefaultFont);

    HeapFree(SystemHeap, 0, self);
    return 0;
}


static LRESULT
SW_WMGetText (STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
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
SW_WMGetTextLength (STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    if (!(self->parts[0].text))
        return 0;
    return (lstrlen32A (self->parts[0].text));
}


static LRESULT
SW_NcHitTest (WND *wndPtr, WPARAM32 wParam, LPARAM lParam)
{
    RECT32  rect;
    POINT32 pt;

    if (wndPtr->dwStyle & SBARS_SIZEGRIP) {
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


static LRESULT
SW_NcLButtonDown (HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    PostMessage32A (GetParent32 (hwnd), WM_NCLBUTTONDOWN,
		    wParam, lParam);
    return 0;
}


static LRESULT
SW_NcLButtonUp (HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    PostMessage32A (GetParent32 (hwnd), WM_NCLBUTTONUP,
		    wParam, lParam);
    return 0;
}


static LRESULT
SW_Paint(STATUSWINDOWINFO *self, HWND32 hwnd)
{
    HDC32 hdc;
    PAINTSTRUCT32	ps;

    hdc = BeginPaint32(hwnd, &ps);
    SW_Refresh(hwnd, hdc, self);
    EndPaint32(hwnd, &ps);
    return 0;
}


static LRESULT
SW_WMSetFont (STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    HDC32 hdc;

    self->hFont = (HFONT32)wParam;
    if (LOWORD(lParam) == TRUE) {
        hdc = GetDC32(hwnd);
        SW_Refresh(hwnd, hdc, self);
        ReleaseDC32(hwnd, hdc);
    }
    return 0;
}


static LRESULT
SW_WMSetText (STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    int len;
    STATUSWINDOWPART *part;

    if (self->numParts == 0) return FALSE;

    part = &self->parts[0];
    /* duplicate string */
    if (part->text)
        HeapFree(SystemHeap, 0, part->text);
    part->text = 0;
    if (lParam && (len = lstrlen32A((LPCSTR)lParam))) {
        part->text = HeapAlloc (SystemHeap, 0, len+1);
        lstrcpy32A (part->text, (LPCSTR)lParam);
    }
    InvalidateRect32(hwnd, &part->bound, FALSE);
//    SW_RefreshPart (hdc, part);

    return TRUE;
}


static LRESULT
SW_Size(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    /* Need to resize width to match parent */
    INT32	width, x, y;
    RECT32	parent_rect;
    HWND32	parent;

    INT32  	flags;

    flags = (INT32) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED, SIZE_RESTORED
     */

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent32(hwnd);
	GetClientRect32(parent, &parent_rect);
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - self->height;
	MoveWindow32(hwnd, parent_rect.left, parent_rect.bottom - self->height,
		     width, self->height, TRUE);
	SW_SetPartBounds(hwnd, self);
    }
    return 0;
}


static LRESULT
SW_SendNotify (HWND32 hwnd, UINT32 code)
{
    WND    *wndPtr;
    NMHDR  nmhdr;

    wndPtr = WIN_FindWndPtr(hwnd);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = code;
    SendMessage32A (GetParent32 (hwnd), WM_NOTIFY, 0, (LPARAM)&nmhdr);

    return 0;
}



LRESULT WINAPI
StatusWindowProc (HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self;
    WND *wndPtr;

    wndPtr = WIN_FindWndPtr(hwnd);
    self = GetStatusInfo(hwnd);

    switch (msg) {
    case SB_GETBORDERS:
	return SW_GetBorders (lParam);
    case SB_GETICON:
        return SW_GetIcon(self, hwnd, wParam, lParam);
    case SB_GETPARTS:
	return SW_GetParts(self, hwnd, wParam, lParam);
    case SB_GETRECT:
	return SW_GetRect(self, hwnd, wParam, lParam);
    case SB_GETTEXT32A:
	return SW_GetText(self, hwnd, wParam, lParam);
    case SB_GETTEXTLENGTH32A:
	return SW_GetTextLength(self, hwnd, wParam, lParam);
    case SB_ISSIMPLE:
        return self->simple;
    case SB_SETBKCOLOR:
        return SW_SetBkColor(self, hwnd, wParam, lParam);
    case SB_SETICON:
        return SW_SetIcon(self, hwnd, wParam, lParam);
    case SB_SETMINHEIGHT:
	return SW_SetMinHeight(self, hwnd, wParam, lParam);
    case SB_SETPARTS:	
	return SW_SetParts(self, hwnd, wParam, lParam);
    case SB_SETTEXT32A:
	return SW_SetText(self, hwnd, wParam, lParam);
    case SB_SIMPLE:
	return SW_Simple(self, hwnd, wParam, lParam);

    case WM_CREATE:
	return SW_Create(hwnd, wParam, lParam);
    case WM_DESTROY:
	return SW_Destroy(self, hwnd, wParam, lParam);
    case WM_GETFONT:
        return self->hFont;
    case WM_GETTEXT:
        return SW_WMGetText(self, hwnd, wParam, lParam);
    case WM_GETTEXTLENGTH:
        return SW_WMGetTextLength(self, hwnd, wParam, lParam);

    case WM_LBUTTONDBLCLK:
        return SW_SendNotify (hwnd, NM_DBLCLK);

    case WM_LBUTTONUP:
	return SW_SendNotify (hwnd, NM_CLICK);

    case WM_NCHITTEST:
        return SW_NcHitTest (wndPtr, wParam, lParam);

    case WM_NCLBUTTONDOWN:
	return SW_NcLButtonDown (hwnd, wParam, lParam);

    case WM_NCLBUTTONUP:
	return SW_NcLButtonUp (hwnd, wParam, lParam);

    case WM_PAINT:
	return SW_Paint(self, hwnd);
    case WM_RBUTTONDBLCLK:
        return SW_SendNotify (hwnd, NM_RDBLCLK);
    case WM_RBUTTONUP:
        return SW_SendNotify (hwnd, NM_RCLICK);
    case WM_SETFONT:
        return SW_WMSetFont(self, hwnd, wParam, lParam);
    case WM_SETTEXT:
        return SW_WMSetText(self, hwnd, wParam, lParam);
    case WM_SIZE:
	return SW_Size(self, hwnd, wParam, lParam);
    default:
	return DefWindowProc32A (hwnd, msg, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 * STATUS_Register [Internal]
 *
 * Registers the status window class.
 */
void STATUS_Register (void)
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

