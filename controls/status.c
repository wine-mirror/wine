/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 */

#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "status.h"
#include "commctrl.h"
#include "heap.h"
#include "syscolor.h"
#include "win.h"

/*
 * Run tests using Waite Group Windows95 API Bible Vol. 1&2
 * The second cdrom contains executables drawstat.exe,gettext.exe,
 * simple.exe, getparts.exe, setparts.exe, statwnd.exe
 */

/*
 * Fixme/Todo
 * 1) Add size grip to status bar - SBARS_SIZEGRIP
 * 2) Don't hard code bar to bottom of window, allow CCS_TOP also
 * 3) Fix SBT_OWNERDRAW
 * 4) Add DrawStatusText32A funtion
 */

static STATUSWINDOWINFO *GetStatusInfo(HWND32 hwnd)
{
    WND *wndPtr;

    wndPtr = WIN_FindWndPtr(hwnd);
    return ((STATUSWINDOWINFO *) &wndPtr->wExtra[0]);
}

/***********************************************************************
 *           DrawStatusText32A   (COMCTL32.3)
 */
void WINAPI DrawStatusText32A( HDC32 hdc, LPRECT32 lprc, LPCSTR text,
                               UINT32 style )
{
    RECT32		r, rt;
    int	oldbkmode;

    r = *lprc;

    if (style == 0 ||
	style == SBT_POPOUT) {
	InflateRect32(&r, -1, -1);
	SelectObject32(hdc, sysColorObjects.hbrushScrollbar);
	Rectangle32(hdc, r.left, r.top, r.right, r.bottom);

	/* draw border */
	SelectObject32(hdc, sysColorObjects.hpenWindowFrame);
	if (style == 0)
	    DrawEdge32(hdc, &r, EDGE_SUNKEN, BF_RECT);
	else
	    DrawEdge32(hdc, &r, EDGE_RAISED, BF_RECT);
    }
    else if (style == SBT_NOBORDERS) {
	SelectObject32(hdc, sysColorObjects.hbrushScrollbar);
	Rectangle32(hdc, r.left, r.top, r.right, r.bottom);
    }
    else {	/* fixme for SBT_OWNERDRAW, SBT_RTLREADING */
	
    }

    /* now draw text */
    if ((style != SBT_OWNERDRAW) && text) {
	SelectObject32(hdc, sysColorObjects.hpenWindowText);
	oldbkmode = SetBkMode32(hdc, TRANSPARENT);
	rt = r;
	rt.left += 3;
	DrawText32A(hdc, text, lstrlen32A(text),
		    &rt, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	if (oldbkmode != TRANSPARENT)
	    SetBkMode32(hdc, oldbkmode);
    }
}

static BOOL32 SW_Refresh( HWND32 hwnd, HDC32 hdc, STATUSWINDOWINFO *self )
{
	int	i;

	if (!IsWindowVisible32(hwnd)) {
	    return (TRUE);
	}

	if (self->simple) {
	    DrawStatusText32A(hdc,
			      &self->part0.bound,
			      self->part0.text,
			      self->part0.style);
	}
	else {
	    for (i = 0; i < self->numParts; i++) {
		DrawStatusText32A(hdc,
				  &self->parts[i].bound,
				  self->parts[i].text,
				  self->parts[i].style);
	    }
	}

	return TRUE;
}


static LRESULT
SW_GetBorders(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    LPINT32	out;

    /* FIXME for sizegrips */
    out = (LPINT32) lParam;
    out[0] = 1; /* vertical border width */
    out[1] = 1; /* horizontal border width */
    out[2] = 1; /* width of border between rectangles */
    return TRUE;
}

static void
SW_SetPartBounds(HWND32 hwnd, STATUSWINDOWINFO *self)
{
    int	i;
    RECT32	rect, *r;
    STATUSWINDOWPART *part;
    int	sep = 1;

    /* get our window size */
    GetClientRect32(hwnd, &rect);

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
	    r->left = self->parts[i-1].bound.right+sep;
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

    if (part_num > 255)
	return FALSE;

    if (self->simple)
	part = &self->part0;
    else
	part = &self->parts[part_num];
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
SW_Create(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    RECT32	rect;
    LPCREATESTRUCT32A lpCreate = (LPCREATESTRUCT32A) lParam;
    int	height, width;
    HDC32	hdc;
    HWND32	parent;

    self->numParts = 0;
    self->parts = 0;
    self->simple = TRUE;
    GetClientRect32(hwnd, &rect);

    /* initialize simple case */
    self->part0.bound = rect;
    self->part0.text = 0;
    self->part0.x = 0;
    self->part0.style = 0;

    height = 40;
    if ((hdc = GetDC32(0))) {
	TEXTMETRIC32A tm;
	GetTextMetrics32A(hdc, &tm);
	self->textHeight = tm.tmHeight;
	ReleaseDC32(0, hdc);
    }

    parent = GetParent32(hwnd);
    GetClientRect32(parent, &rect);
    width = rect.right - rect.left;
    height = (self->textHeight * 3)/2;
    MoveWindow32(hwnd, lpCreate->x, lpCreate->y-1, width, height, FALSE);
    SW_SetPartBounds(hwnd, self);
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
    /* FIXME */
    /* size is wParam | 2*pixels_of_horz_border */
    return TRUE;
}

static LRESULT
SW_Simple(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    BOOL32 simple;
    HDC32	hdc;

    simple = (BOOL32) wParam;
    self->simple = simple;
    hdc = GetDC32(hwnd);
    SW_Refresh(hwnd, hdc, self);
    ReleaseDC32(hwnd, hdc);
    return TRUE;
}

static LRESULT
SW_Size(STATUSWINDOWINFO *self, HWND32 hwnd, WPARAM32 wParam, LPARAM lParam)
{
    /* Need to resize width to match parent */
    INT32	width, height, x, y;
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
	height = (self->textHeight * 3)/2;
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - height;
	MoveWindow32(hwnd, parent_rect.left, parent_rect.bottom - height - 1,
		     width, height, TRUE);
	SW_SetPartBounds(hwnd, self);
    }
    return 0;
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

LRESULT WINAPI StatusWindowProc( HWND32 hwnd, UINT32 msg,
                                 WPARAM32 wParam, LPARAM lParam )
{
    STATUSWINDOWINFO *self;

    self = GetStatusInfo(hwnd);

    switch (msg) {
    case SB_GETBORDERS:
	return SW_GetBorders(self, hwnd, wParam, lParam);
    case SB_GETPARTS:
	return SW_GetParts(self, hwnd, wParam, lParam);
    case SB_GETRECT:
	return SW_GetRect(self, hwnd, wParam, lParam);
    case SB_GETTEXT32A:
	return SW_GetText(self, hwnd, wParam, lParam);
    case SB_GETTEXTLENGTH32A:
	return SW_GetTextLength(self, hwnd, wParam, lParam);
    case SB_SETMINHEIGHT:
	return SW_SetMinHeight(self, hwnd, wParam, lParam);
    case SB_SETPARTS:	
	return SW_SetParts(self, hwnd, wParam, lParam);
    case SB_SETTEXT32A:
	return SW_SetText(self, hwnd, wParam, lParam);
    case SB_SIMPLE:
	return SW_Simple(self, hwnd, wParam, lParam);

    case WM_CREATE:
	return SW_Create(self, hwnd, wParam, lParam);
    case WM_DESTROY:
	return SW_Destroy(self, hwnd, wParam, lParam);
    case WM_PAINT:
	return SW_Paint(self, hwnd);
    case WM_SIZE:
	return SW_Size(self, hwnd, wParam, lParam);
    default:
	return DefWindowProc32A(hwnd, msg, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 *           CreateStatusWindow32A   (COMCTL32.4)
 */
HWND32 WINAPI CreateStatusWindow32A( INT32 style, LPCSTR text, HWND32 parent,
                                     UINT32 wid )
{
    HWND32 ret;
    ATOM atom;

    atom = GlobalFindAtom32A(STATUSCLASSNAME32A);
    if (!atom) {
	/* Some apps don't call InitCommonControls */
	InitCommonControls();
    }

    ret = CreateWindowEx32A(0, STATUSCLASSNAME32A, "Status Window",
			    style, CW_USEDEFAULT32, CW_USEDEFAULT32,
			    CW_USEDEFAULT32, CW_USEDEFAULT32, parent, 0, 0, 0);
    return (ret);
}
