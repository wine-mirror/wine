/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 * Copyright 1998 Eric Kohl
 */

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
 * 2) Tooltip support (almost done).
 */

#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define HORZ_BORDER 0
#define VERT_BORDER 2
#define HORZ_GAP    2

#define STATUSBAR_GetInfoPtr(wndPtr) ((STATUSWINDOWINFO *)wndPtr->wExtra[0])


static void
STATUSBAR_DrawSizeGrip (HDC hdc, LPRECT lpRect)
{
    HPEN hOldPen;
    POINT pt;
    INT i;

    pt.x = lpRect->right - 1;
    pt.y = lpRect->bottom - 1;

    hOldPen = SelectObject (hdc, GetSysColorPen (COLOR_3DFACE));
    MoveToEx (hdc, pt.x - 12, pt.y, NULL);
    LineTo (hdc, pt.x, pt.y);
    LineTo (hdc, pt.x, pt.y - 12);

    pt.x--;
    pt.y--;

    SelectObject (hdc, GetSysColorPen (COLOR_3DSHADOW));
    for (i = 1; i < 11; i += 4) {
	MoveToEx (hdc, pt.x - i, pt.y, NULL);
	LineTo (hdc, pt.x, pt.y - i);

	MoveToEx (hdc, pt.x - i-1, pt.y, NULL);
	LineTo (hdc, pt.x, pt.y - i-1);
    }

    SelectObject (hdc, GetSysColorPen (COLOR_3DHIGHLIGHT));
    for (i = 3; i < 13; i += 4) {
	MoveToEx (hdc, pt.x - i, pt.y, NULL);
	LineTo (hdc, pt.x, pt.y - i);
    }

    SelectObject (hdc, hOldPen);
}


static void 
STATUSBAR_DrawPart (HDC hdc, STATUSWINDOWPART *part)
{
    RECT r = part->bound;
    UINT border = BDR_SUNKENOUTER;

    if (part->style==SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (part->style==SBT_NOBORDERS)
      border = 0;

    DrawEdge(hdc, &r, border, BF_RECT|BF_ADJUST);

    /* draw the icon */
    if (part->hIcon) {
	INT cy = r.bottom - r.top;

	r.left += 2;
	DrawIconEx (hdc, r.left, r.top, part->hIcon, cy, cy, 0, 0, DI_NORMAL);
	r.left += cy;
    }

    /* now draw text */
    if (part->text) {
      int oldbkmode = SetBkMode(hdc, TRANSPARENT);
      LPWSTR p = (LPWSTR)part->text;
      UINT align = DT_LEFT;
      if (*p == L'\t') {
	p++;
	align = DT_CENTER;

	if (*p == L'\t') {
	  p++;
	  align = DT_RIGHT;
	}
      }
      r.left += 3;
      DrawTextW (hdc, p, lstrlenW (p), &r, align|DT_VCENTER|DT_SINGLELINE);
      if (oldbkmode != TRANSPARENT)
	SetBkMode(hdc, oldbkmode);
    }
}


static VOID
STATUSBAR_RefreshPart (WND *wndPtr, STATUSWINDOWPART *part, HDC hdc)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    HBRUSH hbrBk;
    HFONT  hOldFont;

    if (!IsWindowVisible(wndPtr->hwndSelf))
        return;

    if (self->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush (self->clrBk);
    else
	hbrBk = GetSysColorBrush (COLOR_3DFACE);
    FillRect(hdc, &part->bound, hbrBk);

    hOldFont = SelectObject (hdc, self->hFont ? self->hFont : self->hDefaultFont);

    if (part->style == SBT_OWNERDRAW) {
	DRAWITEMSTRUCT dis;

	dis.CtlID = wndPtr->wIDmenu;
	dis.itemID = -1;
	dis.hwndItem = wndPtr->hwndSelf;
	dis.hDC = hdc;
	dis.rcItem = part->bound;
	dis.itemData = (INT)part->text;
	SendMessageA (GetParent (wndPtr->hwndSelf), WM_DRAWITEM,
			(WPARAM)wndPtr->wIDmenu, (LPARAM)&dis);
    }
    else
	STATUSBAR_DrawPart (hdc, part);

    SelectObject (hdc, hOldFont);

    if (self->clrBk != CLR_DEFAULT)
	DeleteObject (hbrBk);

    if (wndPtr->dwStyle & SBARS_SIZEGRIP) {
	RECT rect;

	GetClientRect (wndPtr->hwndSelf, &rect);
	STATUSBAR_DrawSizeGrip (hdc, &rect);
    }
}


static BOOL
STATUSBAR_Refresh (WND *wndPtr, HDC hdc)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    int      i;
    RECT   rect;
    HBRUSH hbrBk;
    HFONT  hOldFont;

    if (!IsWindowVisible(wndPtr->hwndSelf))
        return (TRUE);

    GetClientRect (wndPtr->hwndSelf, &rect);

    if (self->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush (self->clrBk);
    else
	hbrBk = GetSysColorBrush (COLOR_3DFACE);
    FillRect(hdc, &rect, hbrBk);

    hOldFont = SelectObject (hdc, self->hFont ? self->hFont : self->hDefaultFont);

    if (self->simple) {
	STATUSBAR_DrawPart (hdc, &self->part0);
    }
    else {
	for (i = 0; i < self->numParts; i++) {
	    if (self->parts[i].style == SBT_OWNERDRAW) {
		DRAWITEMSTRUCT dis;

		dis.CtlID = wndPtr->wIDmenu;
		dis.itemID = -1;
		dis.hwndItem = wndPtr->hwndSelf;
		dis.hDC = hdc;
		dis.rcItem = self->parts[i].bound;
		dis.itemData = (INT)self->parts[i].text;
		SendMessageA (GetParent (wndPtr->hwndSelf), WM_DRAWITEM,
				(WPARAM)wndPtr->wIDmenu, (LPARAM)&dis);
	    }
	    else
		STATUSBAR_DrawPart (hdc, &self->parts[i]);
	}
    }

    SelectObject (hdc, hOldFont);

    if (self->clrBk != CLR_DEFAULT)
	DeleteObject (hbrBk);

    if (wndPtr->dwStyle & SBARS_SIZEGRIP)
	STATUSBAR_DrawSizeGrip (hdc, &rect);

    return TRUE;
}


static void
STATUSBAR_SetPartBounds (WND *wndPtr)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    RECT rect, *r;
    int	i;

    /* get our window size */
    GetClientRect (wndPtr->hwndSelf, &rect);

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
	    TTTOOLINFOA ti;

	    ti.cbSize = sizeof(TTTOOLINFOA);
	    ti.hwnd = wndPtr->hwndSelf;
	    ti.uId = i;
	    ti.rect = *r;
	    SendMessageA (self->hwndToolTip, TTM_NEWTOOLRECTA,
			    0, (LPARAM)&ti);
	}
    }
}


static VOID
STATUSBAR_RelayEvent (HWND hwndTip, HWND hwndMsg, UINT uMsg,
		      WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = LOWORD(GetMessagePos ());
    msg.pt.y = HIWORD(GetMessagePos ());

    SendMessageA (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}


__inline__ static LRESULT
STATUSBAR_GetBorders (LPARAM lParam)
{
    LPINT out = (LPINT) lParam;

    out[0] = HORZ_BORDER; /* horizontal border width */
    out[1] = VERT_BORDER; /* vertical border width */
    out[2] = HORZ_GAP; /* width of border between rectangles */

    return TRUE;
}


static LRESULT
STATUSBAR_GetIcon (WND *wndPtr, WPARAM wParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT nPart;

    nPart = (INT)wParam & 0x00ff;
    if ((nPart < -1) || (nPart >= self->numParts)) return 0;

    if (nPart == -1)
        return (self->part0.hIcon);
    else
        return (self->parts[nPart].hIcon);
}


static LRESULT
STATUSBAR_GetParts (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    LPINT parts;
    INT   num_parts;
    int	    i;

    num_parts = (INT) wParam;
    parts = (LPINT) lParam;
    if (parts) {
	return (self->numParts);
	for (i = 0; i < num_parts; i++) {
	    parts[i] = self->parts[i].x;
	}
    }
    return (self->numParts);
}


static LRESULT
STATUSBAR_GetRect (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    int	part_num;
    LPRECT  rect;

    part_num = ((INT) wParam) & 0x00ff;
    rect = (LPRECT) lParam;
    if (infoPtr->simple)
	*rect = infoPtr->part0.bound;
    else
	*rect = infoPtr->parts[part_num].bound;
    return TRUE;
}


static LRESULT
STATUSBAR_GetTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    INT   nPart;
    LRESULT result;

    nPart = ((INT) wParam) & 0x00ff;
    if (self->simple)
	part = &self->part0;
    else
	part = &self->parts[nPart];

    if (part->style == SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
	result = part->text ? lstrlenW (part->text) : 0;
	result |= (part->style << 16);
	if (lParam && LOWORD(result))
	    lstrcpyWtoA ((LPSTR)lParam, part->text);
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    INT   nPart;
    LRESULT result;

    nPart = ((INT)wParam) & 0x00ff;
    if (self->simple)
	part = &self->part0;
    else
	part = &self->parts[nPart];

    if (part->style == SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
	result = part->text ? lstrlenW (part->text) : 0;
	result |= (part->style << 16);
	if (lParam)
	    lstrcpyW ((LPWSTR)lParam, part->text);
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextLength (WND *wndPtr, WPARAM wParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    INT part_num;
    DWORD result;

    part_num = ((INT) wParam) & 0x00ff;

    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[part_num];

    if (part->text)
	result = lstrlenW(part->text);
    else
	result = 0;

    result |= (part->style << 16);
    return result;
}


static LRESULT
STATUSBAR_GetTipTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    if (infoPtr->hwndToolTip) {
	TTTOOLINFOA ti;
	ti.cbSize = sizeof(TTTOOLINFOA);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = LOWORD(wParam);
	SendMessageA (infoPtr->hwndToolTip, TTM_GETTEXTA, 0, (LPARAM)&ti);

	if (ti.lpszText)
	    lstrcpynA ((LPSTR)lParam, ti.lpszText, HIWORD(wParam));
    }

    return 0;
}


static LRESULT
STATUSBAR_GetTipTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    if (infoPtr->hwndToolTip) {
	TTTOOLINFOW ti;
	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = LOWORD(wParam);
	SendMessageW (infoPtr->hwndToolTip, TTM_GETTEXTW, 0, (LPARAM)&ti);

	if (ti.lpszText)
	    lstrcpynW ((LPWSTR)lParam, ti.lpszText, HIWORD(wParam));
    }

    return 0;
}


__inline__ static LRESULT
STATUSBAR_GetUnicodeFormat (WND *wndPtr)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    return infoPtr->bUnicode;
}


__inline__ static LRESULT
STATUSBAR_IsSimple (WND *wndPtr)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    return infoPtr->simple;
}


static LRESULT
STATUSBAR_SetBkColor (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    COLORREF oldBkColor;
    HDC    hdc;

    oldBkColor = self->clrBk;
    self->clrBk = (COLORREF)lParam;
    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return oldBkColor;
}


static LRESULT
STATUSBAR_SetIcon (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    INT nPart = (INT)wParam & 0x00ff;
    HDC hdc; 

    if ((nPart < -1) || (nPart >= self->numParts))
	return FALSE;

    hdc = GetDC (wndPtr->hwndSelf);
    if (nPart == -1) {
	self->part0.hIcon = (HICON)lParam;
	if (self->simple)
	    STATUSBAR_RefreshPart (wndPtr, &self->part0, hdc);
    }
    else {
	self->parts[nPart].hIcon = (HICON)lParam;
	if (!(self->simple))
	    STATUSBAR_RefreshPart (wndPtr, &self->parts[nPart], hdc);
    }
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetMinHeight (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);

    if (IsWindowVisible (wndPtr->hwndSelf)) {
	HWND parent = GetParent (wndPtr->hwndSelf);
	INT  width, x, y;
	RECT parent_rect;

	GetClientRect (parent, &parent_rect);
	self->height = (INT)wParam + VERT_BORDER;
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - self->height;
	MoveWindow (wndPtr->hwndSelf, parent_rect.left,
		      parent_rect.bottom - self->height,
		      width, self->height, TRUE);
	STATUSBAR_SetPartBounds (wndPtr);
    }

    return TRUE;
}


static LRESULT
STATUSBAR_SetParts (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *tmp;
    HDC	hdc;
    LPINT parts;
    int	i;
    int	oldNumParts;

    if (self->simple)
	self->simple = FALSE;

    oldNumParts = self->numParts;
    self->numParts = (INT) wParam;
    parts = (LPINT) lParam;
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
	INT nTipCount =
	    SendMessageA (self->hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);

	if (nTipCount < self->numParts) {
	    /* add tools */
	    TTTOOLINFOA ti;
	    INT i;

	    ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	    ti.cbSize = sizeof(TTTOOLINFOA);
	    ti.hwnd = wndPtr->hwndSelf;
	    for (i = nTipCount; i < self->numParts; i++) {
		TRACE (statusbar, "add tool %d\n", i);
		ti.uId = i;
		SendMessageA (self->hwndToolTip, TTM_ADDTOOLA,
				0, (LPARAM)&ti);
	    }
	}
	else if (nTipCount > self->numParts) {
	    /* delete tools */
	    INT i;

	    for (i = nTipCount - 1; i >= self->numParts; i--) {

		FIXME (statusbar, "delete tool %d\n", i);

	    }
	}
    }

    STATUSBAR_SetPartBounds (wndPtr);

    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int	part_num;
    int	style;
    LPSTR text;
    int	len;
    HDC hdc;

    text = (LPSTR) lParam;
    part_num = ((INT) wParam) & 0x00ff;
    style = ((INT) wParam) & 0xff00;

    if ((self->simple) || (self->parts==NULL) || (part_num==255))
	part = &self->part0;
    else
	part = &self->parts[part_num];
    if (!part) return FALSE;
    part->style = style;
    if (style == SBT_OWNERDRAW) {
	part->text = (LPWSTR)text;
    }
    else {
	/* duplicate string */
	if (part->text)
	    COMCTL32_Free (part->text);
	part->text = 0;
	if (text && (len = lstrlenA(text))) {
	    part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyAtoW (part->text, text);
	}
    }

    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_RefreshPart (wndPtr, part, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *self = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    INT  part_num, style, len;
    LPWSTR text;
    HDC  hdc;

    text = (LPWSTR) lParam;
    part_num = ((INT) wParam) & 0x00ff;
    style = ((INT) wParam) & 0xff00;

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
	if (text && (len = lstrlenW(text))) {
	    part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyW(part->text, text);
	}
    }

    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_RefreshPart (wndPtr, part, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTipTextA (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    TRACE (statusbar, "part %d: \"%s\"\n", (INT)wParam, (LPSTR)lParam);
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOA ti;
	ti.cbSize = sizeof(TTTOOLINFOA);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = (INT)wParam;
	ti.hinst = 0;
	ti.lpszText = (LPSTR)lParam;
	SendMessageA (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTA,
			0, (LPARAM)&ti);
    }

    return 0;
}


static LRESULT
STATUSBAR_SetTipTextW (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    TRACE (statusbar, "part %d: \"%s\"\n", (INT)wParam, (LPSTR)lParam);
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOW ti;
	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd = wndPtr->hwndSelf;
	ti.uId = (INT)wParam;
	ti.hinst = 0;
	ti.lpszText = (LPWSTR)lParam;
	SendMessageW (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTW,
			0, (LPARAM)&ti);
    }

    return 0;
}


__inline__ static LRESULT
STATUSBAR_SetUnicodeFormat (WND *wndPtr, WPARAM wParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    BOOL bTemp = infoPtr->bUnicode;

    TRACE (statusbar, "(0x%x)\n", (BOOL)wParam);
    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
STATUSBAR_Simple (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    HDC  hdc;
    NMHDR  nmhdr;

    infoPtr->simple = (BOOL)wParam;

    /* send notification */
    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = SBN_SIMPLEMODECHANGE;
    SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
		    0, (LPARAM)&nmhdr);

    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_Refresh (wndPtr, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_WMCreate (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA lpCreate = (LPCREATESTRUCTA)lParam;
    NONCLIENTMETRICSA nclm;
    RECT	rect;
    int	        width, len;
    HDC	hdc;
    STATUSWINDOWINFO *self;

    self = (STATUSWINDOWINFO*)COMCTL32_Alloc (sizeof(STATUSWINDOWINFO));
    wndPtr->wExtra[0] = (DWORD)self;

    self->numParts = 1;
    self->parts = 0;
    self->simple = FALSE;
    self->clrBk = CLR_DEFAULT;
    self->hFont = 0;
    GetClientRect (wndPtr->hwndSelf, &rect);

    nclm.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
    self->hDefaultFont = CreateFontIndirectA (&nclm.lfStatusFont);

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

    if (IsWindowUnicode (wndPtr->hwndSelf)) {
	self->bUnicode = TRUE;
	if ((len = lstrlenW ((LPCWSTR)lpCreate->lpszName))) {
	    self->parts[0].text = COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyW (self->parts[0].text, (LPCWSTR)lpCreate->lpszName);
	}
    }
    else {
	if ((len = lstrlenA ((LPCSTR)lpCreate->lpszName))) {
	    self->parts[0].text = COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    lstrcpyAtoW (self->parts[0].text, (LPCSTR)lpCreate->lpszName);
	}
    }

    if ((hdc = GetDC (0))) {
	TEXTMETRICA tm;
	HFONT hOldFont;

	hOldFont = SelectObject (hdc,self->hDefaultFont);
	GetTextMetricsA(hdc, &tm);
	self->textHeight = tm.tmHeight;
	SelectObject (hdc, hOldFont);
	ReleaseDC(0, hdc);
    }

    if (wndPtr->dwStyle & SBT_TOOLTIPS) {
	self->hwndToolTip =
	    CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       wndPtr->hwndSelf, 0,
			       wndPtr->hInstance, NULL);

	if (self->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hdr.hwndFrom = wndPtr->hwndSelf;
	    nmttc.hdr.idFrom = wndPtr->wIDmenu;
	    nmttc.hdr.code = NM_TOOLTIPSCREATED;
	    nmttc.hwndToolTips = self->hwndToolTip;

	    SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
			    (WPARAM)wndPtr->wIDmenu, (LPARAM)&nmttc);
	}
    }

    GetClientRect (GetParent (wndPtr->hwndSelf), &rect);
    width = rect.right - rect.left;
    self->height = self->textHeight + 4 + VERT_BORDER;
    MoveWindow (wndPtr->hwndSelf, lpCreate->x, lpCreate->y-1,
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
	DeleteObject (self->hDefaultFont);

    /* delete tool tip control */
    if (self->hwndToolTip)
	DestroyWindow (self->hwndToolTip);

    COMCTL32_Free (self);

    return 0;
}


static __inline__ LRESULT
STATUSBAR_WMGetFont (WND *wndPtr)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    return infoPtr->hFont;
}


static LRESULT
STATUSBAR_WMGetText (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    INT len;

    if (!(infoPtr->parts[0].text))
        return 0;
    len = lstrlenW (infoPtr->parts[0].text);
    if (wParam > len) {
	if (infoPtr->bUnicode)
	    lstrcpyW ((LPWSTR)lParam, infoPtr->parts[0].text);
	else
	    lstrcpyWtoA ((LPSTR)lParam, infoPtr->parts[0].text);
	return len;
    }

    return -1;
}


__inline__ static LRESULT
STATUSBAR_WMMouseMove (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    if (infoPtr->hwndToolTip)
	STATUSBAR_RelayEvent (infoPtr->hwndToolTip, wndPtr->hwndSelf,
			      WM_MOUSEMOVE, wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMNCHitTest (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    if (wndPtr->dwStyle & SBARS_SIZEGRIP) {
	RECT  rect;
	POINT pt;

	GetClientRect (wndPtr->hwndSelf, &rect);

	pt.x = (INT)LOWORD(lParam);
	pt.y = (INT)HIWORD(lParam);
	ScreenToClient (wndPtr->hwndSelf, &pt);

	rect.left = rect.right - 13;
	rect.top += 2;

	if (PtInRect (&rect, pt))
	    return HTBOTTOMRIGHT;
    }

    return DefWindowProcA (wndPtr->hwndSelf, WM_NCHITTEST, wParam, lParam);
}


static __inline__ LRESULT
STATUSBAR_WMNCLButtonDown (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PostMessageA (wndPtr->parent->hwndSelf, WM_NCLBUTTONDOWN,
		    wParam, lParam);
    return 0;
}


static __inline__ LRESULT
STATUSBAR_WMNCLButtonUp (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    PostMessageA (wndPtr->parent->hwndSelf, WM_NCLBUTTONUP,
		    wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMPaint (WND *wndPtr, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (wndPtr->hwndSelf, &ps) : (HDC)wParam;
    STATUSBAR_Refresh (wndPtr, hdc);
    if (!wParam)
	EndPaint (wndPtr->hwndSelf, &ps);

    return 0;
}


static LRESULT
STATUSBAR_WMSetFont (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);

    infoPtr->hFont = (HFONT)wParam;
    if (LOWORD(lParam) == TRUE) {
	HDC hdc = GetDC (wndPtr->hwndSelf);
        STATUSBAR_Refresh (wndPtr, hdc);
        ReleaseDC (wndPtr->hwndSelf, hdc);
    }

    return 0;
}


static LRESULT
STATUSBAR_WMSetText (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    STATUSWINDOWPART *part;
    int len;
    HDC hdc;

    if (infoPtr->numParts == 0)
	return FALSE;

    part = &infoPtr->parts[0];
    /* duplicate string */
    if (part->text)
        COMCTL32_Free (part->text);
    part->text = 0;
    if (infoPtr->bUnicode) {
	if (lParam && (len = lstrlenW((LPCWSTR)lParam))) {
	    part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyW (part->text, (LPCWSTR)lParam);
	}
    }
    else {
	if (lParam && (len = lstrlenA((LPCSTR)lParam))) {
	    part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    lstrcpyAtoW (part->text, (LPCSTR)lParam);
	}
    }

    hdc = GetDC (wndPtr->hwndSelf);
    STATUSBAR_RefreshPart (wndPtr, part, hdc);
    ReleaseDC (wndPtr->hwndSelf, hdc);

    return TRUE;
}


static LRESULT
STATUSBAR_WMSize (WND *wndPtr, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr (wndPtr);
    INT  width, x, y, flags;
    RECT parent_rect;
    HWND parent;

    /* Need to resize width to match parent */
    flags = (INT) wParam;

    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED, SIZE_RESTORED
     */

    if (flags == SIZE_RESTORED) {
	/* width and height don't apply */
	parent = GetParent (wndPtr->hwndSelf);
	GetClientRect (parent, &parent_rect);
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - infoPtr->height;
	MoveWindow (wndPtr->hwndSelf, parent_rect.left, 
		      parent_rect.bottom - infoPtr->height,
		      width, infoPtr->height, TRUE);
	STATUSBAR_SetPartBounds (wndPtr);
    }
    return 0;
}


static LRESULT
STATUSBAR_SendNotify (WND *wndPtr, UINT code)
{
    NMHDR  nmhdr;

    nmhdr.hwndFrom = wndPtr->hwndSelf;
    nmhdr.idFrom = wndPtr->wIDmenu;
    nmhdr.code = code;
    SendMessageA (GetParent (wndPtr->hwndSelf), WM_NOTIFY,
		    0, (LPARAM)&nmhdr);
    return 0;
}



LRESULT WINAPI
StatusWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

	case SB_GETTEXTA:
	    return STATUSBAR_GetTextA (wndPtr, wParam, lParam);

	case SB_GETTEXTW:
	    return STATUSBAR_GetTextW (wndPtr, wParam, lParam);

	case SB_GETTEXTLENGTHA:
	case SB_GETTEXTLENGTHW:
	    return STATUSBAR_GetTextLength (wndPtr, wParam);

	case SB_GETTIPTEXTA:
	    return STATUSBAR_GetTipTextA (wndPtr, wParam, lParam);

	case SB_GETTIPTEXTW:
	    return STATUSBAR_GetTipTextW (wndPtr, wParam, lParam);

	case SB_GETUNICODEFORMAT:
	    return STATUSBAR_GetUnicodeFormat (wndPtr);

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

	case SB_SETTEXTA:
	    return STATUSBAR_SetTextA (wndPtr, wParam, lParam);

	case SB_SETTEXTW:
	    return STATUSBAR_SetTextW (wndPtr, wParam, lParam);

	case SB_SETTIPTEXTA:
	    return STATUSBAR_SetTipTextA (wndPtr, wParam, lParam);

	case SB_SETTIPTEXTW:
	    return STATUSBAR_SetTipTextW (wndPtr, wParam, lParam);

	case SB_SETUNICODEFORMAT:
	    return STATUSBAR_SetUnicodeFormat (wndPtr, wParam);

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
	    return STATUSBAR_GetTextLength (wndPtr, 0);

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
	    return DefWindowProcA (hwnd, msg, wParam, lParam);
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
    WNDCLASSA wndClass;

    if (GlobalFindAtomA (STATUSCLASSNAMEA)) return;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW;
    wndClass.lpfnWndProc   = (WNDPROC)StatusWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(STATUSWINDOWINFO *);
    wndClass.hCursor       = LoadCursorA (0, IDC_ARROWA);
    wndClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wndClass.lpszClassName = STATUSCLASSNAMEA;
 
    RegisterClassA (&wndClass);
}


/***********************************************************************
 * STATUS_Unregister [Internal]
 *
 * Unregisters the status window class.
 */

VOID
STATUS_Unregister (VOID)
{
    if (GlobalFindAtomA (STATUSCLASSNAMEA))
	UnregisterClassA (STATUSCLASSNAMEA, (HINSTANCE)NULL);
}

