/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 * Copyright 1998, 1999 Eric Kohl
 */
/*
 * FIXME/TODO
 * 1) Don't hard code bar to bottom of window, allow CCS_TOP also.
 * 2) Tooltip support (almost done).
 * 3) where else should we use infoPtr->hwndParent instead of GetParent() ?
 * 4) send WM_QUERYFORMAT
 */

#include <string.h>
#include "winbase.h"
#include "wine/unicode.h"
#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(statusbar);

typedef struct
{
    INT	x;
    INT	style;
    RECT	bound;
    LPWSTR	text;
    HICON     hIcon;
} STATUSWINDOWPART;

typedef struct
{
    HWND              hwndParent;
    WORD              numParts;
    WORD              textHeight;
    UINT              height;
    BOOL              simple;
    HWND              hwndToolTip;
    HFONT             hFont;
    HFONT             hDefaultFont;
    COLORREF            clrBk;     /* background color */
    BOOL              bUnicode;  /* unicode flag */
    STATUSWINDOWPART	part0;	   /* simple window */
    STATUSWINDOWPART   *parts;
} STATUSWINDOWINFO;

/*
 * Run tests using Waite Group Windows95 API Bible Vol. 1&2
 * The second cdrom contains executables drawstat.exe, gettext.exe,
 * simple.exe, getparts.exe, setparts.exe, statwnd.exe
 */


#define _MAX(a,b) (((a)>(b))?(a):(b))
#define _MIN(a,b) (((a)>(b))?(b):(a))

#define HORZ_BORDER 0
#define VERT_BORDER 2
#define HORZ_GAP    2

#define STATUSBAR_GetInfoPtr(hwnd) ((STATUSWINDOWINFO *)GetWindowLongA (hwnd, 0))

/* prototype */
static void
STATUSBAR_SetPartBounds (STATUSWINDOWINFO *infoPtr, HWND hwnd);

static void
STATUSBAR_DrawSizeGrip (HDC hdc, LPRECT lpRect)
{
    HPEN hOldPen;
    POINT pt;
    INT i;

    TRACE("draw size grip %d,%d - %d,%d\n", lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
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

    TRACE("part bound %d,%d - %d,%d\n", r.left, r.top, r.right, r.bottom);
    if (part->style & SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (part->style & SBT_NOBORDERS)
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
      TRACE("%s at %d,%d - %d,%d\n", debugstr_w(p), r.left, r.top, r.right, r.bottom);
      DrawTextW (hdc, p, -1, &r, align|DT_VCENTER|DT_SINGLELINE);
      if (oldbkmode != TRANSPARENT)
	SetBkMode(hdc, oldbkmode);
    }
}


static VOID
STATUSBAR_RefreshPart (STATUSWINDOWINFO *infoPtr, HWND hwnd, STATUSWINDOWPART *part, HDC hdc, int itemID)
{
    HBRUSH hbrBk;
    HFONT  hOldFont;

    TRACE("item %d\n", itemID);
    if (!IsWindowVisible (hwnd))
        return;

    if (part->bound.right < part->bound.left) return;

    if (infoPtr->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush (infoPtr->clrBk);
    else
	hbrBk = GetSysColorBrush (COLOR_3DFACE);
    FillRect(hdc, &part->bound, hbrBk);

    hOldFont = SelectObject (hdc, infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont);

    if (part->style & SBT_OWNERDRAW) {
	DRAWITEMSTRUCT dis;

	dis.CtlID = GetWindowLongA (hwnd, GWL_ID);
	dis.itemID = itemID;
	dis.hwndItem = hwnd;
	dis.hDC = hdc;
	dis.rcItem = part->bound;
	dis.itemData = (INT)part->text;
	SendMessageA (GetParent (hwnd), WM_DRAWITEM,
		(WPARAM)dis.CtlID, (LPARAM)&dis);
    } else
	STATUSBAR_DrawPart (hdc, part);

    SelectObject (hdc, hOldFont);

    if (infoPtr->clrBk != CLR_DEFAULT)
	DeleteObject (hbrBk);

    if (GetWindowLongA (hwnd, GWL_STYLE) & SBARS_SIZEGRIP) {
	RECT rect;

	GetClientRect (hwnd, &rect);
	STATUSBAR_DrawSizeGrip (hdc, &rect);
    }
}


static BOOL
STATUSBAR_Refresh (STATUSWINDOWINFO *infoPtr, HWND hwnd, HDC hdc)
{
    int      i;
    RECT   rect;
    HBRUSH hbrBk;
    HFONT  hOldFont;

    TRACE("\n");
    if (!IsWindowVisible(hwnd))
        return (TRUE);

    STATUSBAR_SetPartBounds(infoPtr, hwnd);

    GetClientRect (hwnd, &rect);

    if (infoPtr->clrBk != CLR_DEFAULT)
	hbrBk = CreateSolidBrush (infoPtr->clrBk);
    else
	hbrBk = GetSysColorBrush (COLOR_3DFACE);
    FillRect(hdc, &rect, hbrBk);

    hOldFont = SelectObject (hdc, infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont);

    if (infoPtr->simple) {
	STATUSBAR_RefreshPart (infoPtr, hwnd, &infoPtr->part0, hdc, 0);
    } else {
	for (i = 0; i < infoPtr->numParts; i++) {
	    if (infoPtr->parts[i].style & SBT_OWNERDRAW) {
		DRAWITEMSTRUCT dis;

		dis.CtlID = GetWindowLongA (hwnd, GWL_ID);
		dis.itemID = i;
		dis.hwndItem = hwnd;
		dis.hDC = hdc;
		dis.rcItem = infoPtr->parts[i].bound;
		dis.itemData = (INT)infoPtr->parts[i].text;
		SendMessageA (GetParent (hwnd), WM_DRAWITEM,
			(WPARAM)dis.CtlID, (LPARAM)&dis);
	    } else
		STATUSBAR_RefreshPart (infoPtr, hwnd, &infoPtr->parts[i], hdc, i);
	}
    }

    SelectObject (hdc, hOldFont);

    if (infoPtr->clrBk != CLR_DEFAULT)
	DeleteObject (hbrBk);

    if (GetWindowLongA(hwnd, GWL_STYLE) & SBARS_SIZEGRIP)
	STATUSBAR_DrawSizeGrip (hdc, &rect);

    return TRUE;
}


static void
STATUSBAR_SetPartBounds (STATUSWINDOWINFO *infoPtr, HWND hwnd)
{
    STATUSWINDOWPART *part;
    RECT rect, *r;
    int	i;

    /* get our window size */
    GetClientRect (hwnd, &rect);
    TRACE("client wnd size is %d,%d - %d,%d\n", rect.left, rect.top, rect.right, rect.bottom);

    rect.top += VERT_BORDER;

    /* set bounds for simple rectangle */
    infoPtr->part0.bound = rect;

    /* set bounds for non-simple rectangles */
    for (i = 0; i < infoPtr->numParts; i++) {
	part = &infoPtr->parts[i];
	r = &infoPtr->parts[i].bound;
	r->top = rect.top;
	r->bottom = rect.bottom;
	if (i == 0)
	    r->left = 0;
	else
	    r->left = infoPtr->parts[i-1].bound.right + HORZ_GAP;
	if (part->x == -1)
	    r->right = rect.right;
	else
	    r->right = part->x;

	if (infoPtr->hwndToolTip) {
	    TTTOOLINFOA ti;

	    ti.cbSize = sizeof(TTTOOLINFOA);
	    ti.hwnd = hwnd;
	    ti.uId = i;
	    ti.rect = *r;
	    SendMessageA (infoPtr->hwndToolTip, TTM_NEWTOOLRECTA,
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


inline static LRESULT
STATUSBAR_GetBorders (LPARAM lParam)
{
    LPINT out = (LPINT) lParam;

    TRACE("\n");
    out[0] = HORZ_BORDER; /* horizontal border width */
    out[1] = VERT_BORDER; /* vertical border width */
    out[2] = HORZ_GAP; /* width of border between rectangles */

    return TRUE;
}


static LRESULT
STATUSBAR_GetIcon (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam)
{
    INT nPart;

    nPart = (INT)wParam & 0x00ff;
    TRACE("%d\n", nPart);
    if ((nPart < -1) || (nPart >= infoPtr->numParts))
	return 0;

    if (nPart == -1)
        return (infoPtr->part0.hIcon);
    else
        return (infoPtr->parts[nPart].hIcon);
}


static LRESULT
STATUSBAR_GetParts (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPINT parts;
    INT   num_parts;
    INT   i;

    num_parts = (INT) wParam;
    TRACE("(%d)\n", num_parts);
    parts = (LPINT) lParam;
    if (parts) {
	for (i = 0; i < num_parts; i++) {
	    parts[i] = infoPtr->parts[i].x;
	}
    }
    return (infoPtr->numParts);
}


static LRESULT
STATUSBAR_GetRect (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int	nPart;
    LPRECT  rect;

    nPart = ((INT) wParam) & 0x00ff;
    TRACE("part %d\n", nPart);
    rect = (LPRECT) lParam;
    if (infoPtr->simple)
	*rect = infoPtr->part0.bound;
    else
	*rect = infoPtr->parts[nPart].bound;
    return TRUE;
}


static LRESULT
STATUSBAR_GetTextA (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *part;
    INT   nPart;
    LRESULT result;

    nPart = ((INT) wParam) & 0x00ff;
    TRACE("part %d\n", nPart);
    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if (part->style & SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
        DWORD len = part->text ? WideCharToMultiByte( CP_ACP, 0, part->text, -1,
                                                      NULL, 0, NULL, NULL ) - 1 : 0;
        result = MAKELONG( len, part->style );
        if (lParam && len)
            WideCharToMultiByte( CP_ACP, 0, part->text, -1, (LPSTR)lParam, len+1, NULL, NULL );
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextW (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *part;
    INT   nPart;
    LRESULT result;

    nPart = ((INT)wParam) & 0x00ff;
    TRACE("part %d\n", nPart);
    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if (part->style & SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
	result = part->text ? strlenW (part->text) : 0;
	result |= (part->style << 16);
	if (part->text && lParam)
	    strcpyW ((LPWSTR)lParam, part->text);
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextLength (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam)
{
    STATUSWINDOWPART *part;
    INT nPart;
    DWORD result;

    nPart = ((INT) wParam) & 0x00ff;

    TRACE("part %d\n", nPart);
    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if (part->text)
	result = strlenW(part->text);
    else
	result = 0;

    result |= (part->style << 16);
    return result;
}


static LRESULT
STATUSBAR_GetTipTextA (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPSTR tip = (LPSTR)lParam;

    if (tip) {
        CHAR buf[INFOTIPSIZE];
        buf[0]='\0';

        if (infoPtr->hwndToolTip) {
            TTTOOLINFOA ti;
            ti.cbSize = sizeof(TTTOOLINFOA);
            ti.hwnd = hwnd;
            ti.uId = LOWORD(wParam);
            ti.lpszText = buf;
            SendMessageA(infoPtr->hwndToolTip, TTM_GETTEXTA, 0, (LPARAM)&ti);
        }
        lstrcpynA(tip, buf, HIWORD(wParam));
    }
    return 0;
}


static LRESULT
STATUSBAR_GetTipTextW (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPWSTR tip = (LPWSTR)lParam;

    TRACE("\n");
    if (tip) {
        WCHAR buf[INFOTIPSIZE];
        buf[0]=0;

	if (infoPtr->hwndToolTip) {
	    TTTOOLINFOW ti;
	    ti.cbSize = sizeof(TTTOOLINFOW);
	    ti.hwnd = hwnd;
	    ti.uId = LOWORD(wParam);
            ti.lpszText = buf;
	    SendMessageW(infoPtr->hwndToolTip, TTM_GETTEXTW, 0, (LPARAM)&ti);
	}
	lstrcpynW(tip, buf, HIWORD(wParam));
    }

    return 0;
}


inline static LRESULT
STATUSBAR_GetUnicodeFormat (STATUSWINDOWINFO *infoPtr, HWND hwnd)
{
    return infoPtr->bUnicode;
}


inline static LRESULT
STATUSBAR_IsSimple (STATUSWINDOWINFO *infoPtr, HWND hwnd)
{
    return infoPtr->simple;
}


static LRESULT
STATUSBAR_SetBkColor (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    COLORREF oldBkColor;

    oldBkColor = infoPtr->clrBk;
    infoPtr->clrBk = (COLORREF)lParam;
    InvalidateRect(hwnd, NULL, FALSE);

    TRACE("CREF: %08lx -> %08lx\n", oldBkColor, infoPtr->clrBk);
    return oldBkColor;
}


static LRESULT
STATUSBAR_SetIcon (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT nPart = (INT)wParam & 0x00ff;

    if ((nPart < -1) || (nPart >= infoPtr->numParts))
	return FALSE;

    TRACE("setting part %d, icon %lx\n",nPart,lParam);

    if (nPart == -1) {
	if (infoPtr->part0.hIcon == (HICON)lParam) /* same as - no redraw */
	    return TRUE;
	infoPtr->part0.hIcon = (HICON)lParam;
	if (infoPtr->simple)
            InvalidateRect(hwnd, &infoPtr->part0.bound, FALSE);
    } else {
	if (infoPtr->parts[nPart].hIcon == (HICON)lParam) /* same as - no redraw */
	    return TRUE;

	infoPtr->parts[nPart].hIcon = (HICON)lParam;
	if (!(infoPtr->simple))
            InvalidateRect(hwnd, &infoPtr->parts[nPart].bound, FALSE);
    }
    return TRUE;
}


static LRESULT
STATUSBAR_SetMinHeight (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{

    TRACE("\n");
    if (IsWindowVisible (hwnd)) {
	HWND parent = GetParent (hwnd);
	INT  width, x, y;
	RECT parent_rect;

	GetClientRect (parent, &parent_rect);
	infoPtr->height = (INT)wParam + VERT_BORDER;
	width = parent_rect.right - parent_rect.left;
	x = parent_rect.left;
	y = parent_rect.bottom - infoPtr->height;
	MoveWindow (hwnd, parent_rect.left,
		      parent_rect.bottom - infoPtr->height,
		      width, infoPtr->height, TRUE);
	STATUSBAR_SetPartBounds (infoPtr, hwnd);
    }

    return TRUE;
}


static LRESULT
STATUSBAR_SetParts (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *tmp;
    LPINT parts;
    int	i;
    int	oldNumParts;

    TRACE("(%d,%p)\n",wParam,(LPVOID)lParam);

    /* FIXME: should return FALSE sometimes (maybe when wParam == 0 ?) */
    if (infoPtr->simple)
	infoPtr->simple = FALSE;

    oldNumParts = infoPtr->numParts;
    infoPtr->numParts = (INT) wParam;
    parts = (LPINT) lParam;
    if (oldNumParts > infoPtr->numParts) {
	for (i = infoPtr->numParts ; i < oldNumParts; i++) {
	    if (infoPtr->parts[i].text && !(infoPtr->parts[i].style & SBT_OWNERDRAW))
		COMCTL32_Free (infoPtr->parts[i].text);
	}
    }
    if (oldNumParts < infoPtr->numParts) {
	tmp = COMCTL32_Alloc (sizeof(STATUSWINDOWPART) * infoPtr->numParts);
	for (i = 0; i < oldNumParts; i++) {
	    tmp[i] = infoPtr->parts[i];
	}
	if (infoPtr->parts)
	    COMCTL32_Free (infoPtr->parts);
	infoPtr->parts = tmp;
    }
    if (oldNumParts == infoPtr->numParts) {
	for (i=0;i<oldNumParts;i++)
	    if (infoPtr->parts[i].x != parts[i])
		break;
	if (i==oldNumParts) /* Unchanged? no need to redraw! */
	    return TRUE;
    }
    
    for (i = 0; i < infoPtr->numParts; i++)
	infoPtr->parts[i].x = parts[i];

    if (infoPtr->hwndToolTip) {
	INT nTipCount =
	    SendMessageA (infoPtr->hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);

	if (nTipCount < infoPtr->numParts) {
	    /* add tools */
	    TTTOOLINFOA ti;
	    INT i;

	    ZeroMemory (&ti, sizeof(TTTOOLINFOA));
	    ti.cbSize = sizeof(TTTOOLINFOA);
	    ti.hwnd = hwnd;
	    for (i = nTipCount; i < infoPtr->numParts; i++) {
		TRACE("add tool %d\n", i);
		ti.uId = i;
		SendMessageA (infoPtr->hwndToolTip, TTM_ADDTOOLA,
				0, (LPARAM)&ti);
	    }
	}
	else if (nTipCount > infoPtr->numParts) {
	    /* delete tools */
	    INT i;

	    for (i = nTipCount - 1; i >= infoPtr->numParts; i--) {
		FIXME("delete tool %d\n", i);
	    }
	}
    }
    STATUSBAR_SetPartBounds (infoPtr, hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
}


static LRESULT
STATUSBAR_SetTextA (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *part=NULL;
    int	nPart;
    int	style;
    LPSTR text;
    BOOL	changed = FALSE;

    text = (LPSTR) lParam;
    nPart = ((INT) wParam) & 0x00ff;
    style = ((INT) wParam) & 0xff00;

    TRACE("part %d, text %s\n",nPart,debugstr_a(text));

    if (nPart==255)
	part = &infoPtr->part0;
    else if (!infoPtr->simple && infoPtr->parts!=NULL)
	part = &infoPtr->parts[nPart];
    if (!part) return FALSE;

    if (part->style != style)
	changed = TRUE;

    part->style = style;
    if (style & SBT_OWNERDRAW) {
	if (part->text == (LPWSTR)text)
	    return TRUE;
	part->text = (LPWSTR)text;
    } else {
	LPWSTR ntext;

	/* check if text is unchanged -> no need to redraw */
	if (text) {
            DWORD len = MultiByteToWideChar( CP_ACP, 0, text, -1, NULL, 0 );
	    LPWSTR tmptext = COMCTL32_Alloc(len*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, text, -1, tmptext, len );

	    if (!changed && part->text && !lstrcmpW(tmptext,part->text)) {
		COMCTL32_Free(tmptext);
		return TRUE;
	    }
	    ntext = tmptext;
	} else {
	    if (!changed && !part->text) 
		return TRUE;
	    ntext = 0;
	}

	if (part->text)
	    COMCTL32_Free (part->text);
	part->text = ntext;
    }
    InvalidateRect(hwnd, &part->bound, FALSE);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTextW (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *part;
    INT  nPart, style, len;
    LPWSTR text;
    BOOL bRedraw = FALSE;

    text = (LPWSTR) lParam;
    nPart = ((INT) wParam) & 0x00ff;
    style = ((INT) wParam) & 0xff00;

    TRACE("part %d -> '%s' with style %04x\n", nPart, debugstr_w(text), style);
    if ((infoPtr->simple) || (infoPtr->parts==NULL) || (nPart==255))
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];
    if (!part) return FALSE;

    if(part->style != style)
        bRedraw = TRUE;

    part->style = style;

    /* FIXME: not sure how/if we can check for change in string with ownerdraw(remove this if we can't)... */
    if (style & SBT_OWNERDRAW)
    {
	part->text = text;
        bRedraw = TRUE;
    } else if(!text)
    {
        if(part->text)
        {
            COMCTL32_Free(part->text);
            bRedraw = TRUE;
        }
        part->text = 0;
    } else if(!part->text || strcmpW(part->text, text)) /* see if the new string differs from the existing string */
    {
	if(part->text) COMCTL32_Free(part->text);

        len = strlenW(text);
        part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	strcpyW(part->text, text);
        bRedraw = TRUE;
    }

    if(bRedraw)
        InvalidateRect(hwnd, &part->bound, FALSE);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTipTextA (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("part %d: \"%s\"\n", (INT)wParam, (LPSTR)lParam);
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOA ti;
	ti.cbSize = sizeof(TTTOOLINFOA);
	ti.hwnd = hwnd;
	ti.uId = (INT)wParam;
	ti.hinst = 0;
	ti.lpszText = (LPSTR)lParam;
	SendMessageA (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTA,
			0, (LPARAM)&ti);
    }

    return 0;
}


static LRESULT
STATUSBAR_SetTipTextW (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("part %d: \"%s\"\n", (INT)wParam, (LPSTR)lParam);
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOW ti;
	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd = hwnd;
	ti.uId = (INT)wParam;
	ti.hinst = 0;
	ti.lpszText = (LPWSTR)lParam;
	SendMessageW (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTW,
			0, (LPARAM)&ti);
    }

    return 0;
}


inline static LRESULT
STATUSBAR_SetUnicodeFormat (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam)
{
    BOOL bOld = infoPtr->bUnicode;

    TRACE("(0x%x)\n", (BOOL)wParam);
    infoPtr->bUnicode = (BOOL)wParam;

    return bOld;
}


static LRESULT
STATUSBAR_Simple (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    NMHDR  nmhdr;

    TRACE("(is simple: %d)\n", wParam);
    if (infoPtr->simple == wParam) /* no need to change */
	return TRUE;

    infoPtr->simple = (BOOL)wParam;

    /* send notification */
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
    nmhdr.code = SBN_SIMPLEMODECHANGE;
    SendMessageA (GetParent (hwnd), WM_NOTIFY, 0, (LPARAM)&nmhdr);
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
}


static LRESULT
STATUSBAR_WMCreate (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPCREATESTRUCTA lpCreate = (LPCREATESTRUCTA)lParam;
    NONCLIENTMETRICSA nclm;
    DWORD dwStyle;
    RECT	rect;
    int	        width, len;
    HDC	hdc;
    STATUSWINDOWINFO *infoPtr;

    TRACE("\n");
    infoPtr = (STATUSWINDOWINFO*)COMCTL32_Alloc (sizeof(STATUSWINDOWINFO));
    SetWindowLongA (hwnd, 0, (DWORD)infoPtr);

    infoPtr->hwndParent = lpCreate->hwndParent;
    infoPtr->numParts = 1;
    infoPtr->parts = 0;
    infoPtr->simple = FALSE;
    infoPtr->clrBk = CLR_DEFAULT;
    infoPtr->hFont = 0;

    /* TODO: send unicode parent notification query (WM_QUERYFORMAT) here */

    GetClientRect (hwnd, &rect);
    InvalidateRect (hwnd, &rect, 0);
    UpdateWindow(hwnd);

    nclm.cbSize = sizeof(NONCLIENTMETRICSA);
    SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, nclm.cbSize, &nclm, 0);
    infoPtr->hDefaultFont = CreateFontIndirectA (&nclm.lfStatusFont);

    /* initialize simple case */
    infoPtr->part0.bound = rect;
    infoPtr->part0.text = 0;
    infoPtr->part0.x = 0;
    infoPtr->part0.style = 0;
    infoPtr->part0.hIcon = 0;

    /* initialize first part */
    infoPtr->parts = COMCTL32_Alloc (sizeof(STATUSWINDOWPART));
    infoPtr->parts[0].bound = rect;
    infoPtr->parts[0].text = 0;
    infoPtr->parts[0].x = -1;
    infoPtr->parts[0].style = 0;
    infoPtr->parts[0].hIcon = 0;

    if (IsWindowUnicode (hwnd)) {
	infoPtr->bUnicode = TRUE;
	if (lpCreate->lpszName &&
	    (len = strlenW ((LPCWSTR)lpCreate->lpszName))) {
	    infoPtr->parts[0].text = COMCTL32_Alloc ((len + 1)*sizeof(WCHAR));
	    strcpyW (infoPtr->parts[0].text, (LPCWSTR)lpCreate->lpszName);
	}
    }
    else {
	if (lpCreate->lpszName &&
	    (len = strlen((LPCSTR)lpCreate->lpszName))) {
            DWORD lenW = MultiByteToWideChar( CP_ACP, 0, (LPCSTR)lpCreate->lpszName, -1, NULL, 0 );
	    infoPtr->parts[0].text = COMCTL32_Alloc (lenW*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, (LPCSTR)lpCreate->lpszName, -1,
                                 infoPtr->parts[0].text, lenW );
	}
    }

    dwStyle = GetWindowLongA(hwnd, GWL_STYLE);

    /* statusbars on managed windows should not have SIZEGRIP style */
    if ((dwStyle & SBARS_SIZEGRIP) && lpCreate->hwndParent)
        if (GetWindowLongA(lpCreate->hwndParent, GWL_EXSTYLE) & WS_EX_MANAGED)
            SetWindowLongA (hwnd, GWL_STYLE, dwStyle & ~SBARS_SIZEGRIP);

    if ((hdc = GetDC (0))) {
	TEXTMETRICA tm;
	HFONT hOldFont;

	hOldFont = SelectObject (hdc,infoPtr->hDefaultFont);
	GetTextMetricsA(hdc, &tm);
	infoPtr->textHeight = tm.tmHeight;
	SelectObject (hdc, hOldFont);
	ReleaseDC(0, hdc);
    }

    if (dwStyle & SBT_TOOLTIPS) {
	infoPtr->hwndToolTip =
	    CreateWindowExA (0, TOOLTIPS_CLASSA, NULL, 0,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			       CW_USEDEFAULT, CW_USEDEFAULT,
			     hwnd, 0,
			     GetWindowLongA (hwnd, GWL_HINSTANCE), NULL);

	if (infoPtr->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hdr.hwndFrom = hwnd;
	    nmttc.hdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
	    nmttc.hdr.code = NM_TOOLTIPSCREATED;
	    nmttc.hwndToolTips = infoPtr->hwndToolTip;

	    SendMessageA (lpCreate->hwndParent, WM_NOTIFY,
			    (WPARAM)nmttc.hdr.idFrom, (LPARAM)&nmttc);
	}
    }

    if (!dwStyle & CCS_NORESIZE) /* don't resize wnd if it doesn't want it ! */
    {
        GetClientRect (GetParent (hwnd), &rect);
        width = rect.right - rect.left;
        infoPtr->height = infoPtr->textHeight + 4 + VERT_BORDER;
        SetWindowPos(hwnd, 0, lpCreate->x, lpCreate->y - 1,
			width, infoPtr->height, SWP_NOZORDER);
        STATUSBAR_SetPartBounds (infoPtr, hwnd);
    }

    return 0;
}


static LRESULT
STATUSBAR_WMDestroy (STATUSWINDOWINFO *infoPtr, HWND hwnd)
{
    int	i;

    TRACE("\n");
    for (i = 0; i < infoPtr->numParts; i++) {
	if (infoPtr->parts[i].text && !(infoPtr->parts[i].style & SBT_OWNERDRAW))
	    COMCTL32_Free (infoPtr->parts[i].text);
    }
    if (infoPtr->part0.text && !(infoPtr->part0.style & SBT_OWNERDRAW))
	COMCTL32_Free (infoPtr->part0.text);
    COMCTL32_Free (infoPtr->parts);

    /* delete default font */
    if (infoPtr->hDefaultFont)
	DeleteObject (infoPtr->hDefaultFont);

    /* delete tool tip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow (infoPtr->hwndToolTip);

    COMCTL32_Free (infoPtr);
    SetWindowLongA(hwnd, 0, 0);
    return 0;
}


static inline LRESULT
STATUSBAR_WMGetFont (STATUSWINDOWINFO *infoPtr, HWND hwnd)
{
    TRACE("\n");
    return infoPtr->hFont? infoPtr->hFont : infoPtr->hDefaultFont;
}


/* in contrast to SB_GETTEXT*, WM_GETTEXT handles the text
 * of the first part only (usual behaviour) */
static LRESULT
STATUSBAR_WMGetText (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT len;

    TRACE("\n");
    if (!(infoPtr->parts[0].text))
        return 0;
    if (infoPtr->bUnicode)
        len = strlenW (infoPtr->parts[0].text);
    else
        len = WideCharToMultiByte( CP_ACP, 0, infoPtr->parts[0].text, -1, NULL, 0, NULL, NULL )-1;

    if (wParam > len) {
	if (infoPtr->bUnicode)
	    strcpyW ((LPWSTR)lParam, infoPtr->parts[0].text);
	else
            WideCharToMultiByte( CP_ACP, 0, infoPtr->parts[0].text, -1,
                                 (LPSTR)lParam, len+1, NULL, NULL );
	return len;
    }

    return -1;
}


inline static LRESULT
STATUSBAR_WMMouseMove (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (infoPtr->hwndToolTip)
	STATUSBAR_RelayEvent (infoPtr->hwndToolTip, hwnd,
			      WM_MOUSEMOVE, wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMNCHitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (GetWindowLongA (hwnd, GWL_STYLE) & SBARS_SIZEGRIP) {
	RECT  rect;
	POINT pt;

	GetClientRect (hwnd, &rect);

	pt.x = (INT)LOWORD(lParam);
	pt.y = (INT)HIWORD(lParam);
	ScreenToClient (hwnd, &pt);

	rect.left = rect.right - 13;
	rect.top += 2;

	if (PtInRect (&rect, pt))
	    return HTBOTTOMRIGHT;
    }

    /* FIXME: instead check result in StatusWindowProc and call if needed ? */
    return DefWindowProcA (hwnd, WM_NCHITTEST, wParam, lParam);
}


static inline LRESULT
STATUSBAR_WMNCLButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("\n");
    PostMessageA (GetParent (hwnd), WM_NCLBUTTONDOWN, wParam, lParam);
    return 0;
}


static inline LRESULT
STATUSBAR_WMNCLButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("\n");
    PostMessageA (GetParent (hwnd), WM_NCLBUTTONUP, wParam, lParam);
    return 0;
}


static LRESULT
STATUSBAR_WMPaint (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    TRACE("\n");
    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    STATUSBAR_Refresh (infoPtr, hwnd, hdc);
    if (!wParam)
	EndPaint (hwnd, &ps);

    return 0;
}


static LRESULT
STATUSBAR_WMSetFont (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    infoPtr->hFont = (HFONT)wParam;
    TRACE("%04x\n", infoPtr->hFont);
    if (LOWORD(lParam) == TRUE)
        InvalidateRect(hwnd, NULL, FALSE);

    return 0;
}


static LRESULT
STATUSBAR_WMSetText (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWPART *part;
    int len;

    TRACE("\n");
    if (infoPtr->numParts == 0)
	return FALSE;

    part = &infoPtr->parts[0];
    /* duplicate string */
    if (part->text)
        COMCTL32_Free (part->text);
    part->text = 0;
    if (infoPtr->bUnicode) {
	if (lParam && (len = strlenW((LPCWSTR)lParam))) {
	    part->text = COMCTL32_Alloc ((len+1)*sizeof(WCHAR));
	    strcpyW (part->text, (LPCWSTR)lParam);
	}
    }
    else {
	if (lParam && (len = lstrlenA((LPCSTR)lParam))) {
            DWORD lenW = MultiByteToWideChar( CP_ACP, 0, (LPCSTR)lParam, -1, NULL, 0 );
            part->text = COMCTL32_Alloc (lenW*sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, (LPCSTR)lParam, -1, part->text, lenW );
	}
    }

    InvalidateRect(hwnd, &part->bound, FALSE);

    return TRUE;
}


static LRESULT
STATUSBAR_WMSize (STATUSWINDOWINFO *infoPtr, HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    INT  width, x, y, flags;
    RECT parent_rect;
    DWORD dwStyle;

    /* Need to resize width to match parent */
    flags = (INT) wParam;

    TRACE("flags %04x\n", flags);
    /* FIXME for flags =
     * SIZE_MAXIMIZED, SIZE_MAXSHOW, SIZE_MINIMIZED, SIZE_RESTORED
     */

    dwStyle = GetWindowLongA(hwnd, GWL_STYLE);
    if (!dwStyle & CCS_NORESIZE) /* don't resize wnd if it doesn't want it ! */
    {
        if (flags == SIZE_RESTORED) {
	    /* width and height don't apply */
	    GetClientRect (infoPtr->hwndParent, &parent_rect);
	    width = parent_rect.right - parent_rect.left;
	    x = parent_rect.left;
	    y = parent_rect.bottom - infoPtr->height;
	    MoveWindow (hwnd, parent_rect.left, 
		      parent_rect.bottom - infoPtr->height,
		      width, infoPtr->height, TRUE);
	    STATUSBAR_SetPartBounds (infoPtr, hwnd);
        }
	return 0; /* FIXME: ok to return here ? */
    }

    /* FIXME: instead check result in StatusWindowProc and call if needed ? */
    return DefWindowProcA (hwnd, WM_SIZE, wParam, lParam);
}


static LRESULT
STATUSBAR_SendNotify (HWND hwnd, UINT code)
{
    NMHDR  nmhdr;

    TRACE("code %04x\n", code);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = GetWindowLongA (hwnd, GWL_ID);
    nmhdr.code = code;
    SendMessageA (GetParent (hwnd), WM_NOTIFY, 0, (LPARAM)&nmhdr);
    return 0;
}



static LRESULT WINAPI
StatusWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    STATUSWINDOWINFO *infoPtr = STATUSBAR_GetInfoPtr(hwnd);

    TRACE("hwnd=%x msg=%x wparam=%x lparam=%lx\n", hwnd, msg, wParam, lParam);
    if (!(infoPtr) && (msg != WM_CREATE))
        return DefWindowProcA (hwnd, msg, wParam, lParam);

    switch (msg) {
	case SB_GETBORDERS:
	    return STATUSBAR_GetBorders (lParam);

	case SB_GETICON:
	    return STATUSBAR_GetIcon (infoPtr, hwnd, wParam);

	case SB_GETPARTS:
	    return STATUSBAR_GetParts (infoPtr, hwnd, wParam, lParam);

	case SB_GETRECT:
	    return STATUSBAR_GetRect (infoPtr, hwnd, wParam, lParam);

	case SB_GETTEXTA:
	    return STATUSBAR_GetTextA (infoPtr, hwnd, wParam, lParam);

	case SB_GETTEXTW:
	    return STATUSBAR_GetTextW (infoPtr, hwnd, wParam, lParam);

	case SB_GETTEXTLENGTHA:
	case SB_GETTEXTLENGTHW:
	    return STATUSBAR_GetTextLength (infoPtr, hwnd, wParam);

	case SB_GETTIPTEXTA:
	    return STATUSBAR_GetTipTextA (infoPtr, hwnd, wParam, lParam);

	case SB_GETTIPTEXTW:
	    return STATUSBAR_GetTipTextW (infoPtr, hwnd, wParam, lParam);

	case SB_GETUNICODEFORMAT:
	    return STATUSBAR_GetUnicodeFormat (infoPtr, hwnd);

	case SB_ISSIMPLE:
	    return STATUSBAR_IsSimple (infoPtr, hwnd);

	case SB_SETBKCOLOR:
	    return STATUSBAR_SetBkColor (infoPtr, hwnd, wParam, lParam);

	case SB_SETICON:
	    return STATUSBAR_SetIcon (infoPtr, hwnd, wParam, lParam);

	case SB_SETMINHEIGHT:
	    return STATUSBAR_SetMinHeight (infoPtr, hwnd, wParam, lParam);

	case SB_SETPARTS:	
	    return STATUSBAR_SetParts (infoPtr, hwnd, wParam, lParam);

	case SB_SETTEXTA:
	    return STATUSBAR_SetTextA (infoPtr, hwnd, wParam, lParam);

	case SB_SETTEXTW:
	    return STATUSBAR_SetTextW (infoPtr, hwnd, wParam, lParam);

	case SB_SETTIPTEXTA:
	    return STATUSBAR_SetTipTextA (infoPtr, hwnd, wParam, lParam);

	case SB_SETTIPTEXTW:
	    return STATUSBAR_SetTipTextW (infoPtr, hwnd, wParam, lParam);

	case SB_SETUNICODEFORMAT:
	    return STATUSBAR_SetUnicodeFormat (infoPtr, hwnd, wParam);

	case SB_SIMPLE:
	    return STATUSBAR_Simple (infoPtr, hwnd, wParam, lParam);


	case WM_CREATE:
	    return STATUSBAR_WMCreate (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return STATUSBAR_WMDestroy (infoPtr, hwnd);

	case WM_GETFONT:
            return STATUSBAR_WMGetFont (infoPtr, hwnd);

	case WM_GETTEXT:
            return STATUSBAR_WMGetText (infoPtr, hwnd, wParam, lParam);

	case WM_GETTEXTLENGTH:
	    return STATUSBAR_GetTextLength (infoPtr, hwnd, 0);

	case WM_LBUTTONDBLCLK:
            return STATUSBAR_SendNotify (hwnd, NM_DBLCLK);

	case WM_LBUTTONUP:
	    return STATUSBAR_SendNotify (hwnd, NM_CLICK);

	case WM_MOUSEMOVE:
            return STATUSBAR_WMMouseMove (infoPtr, hwnd, wParam, lParam);

	case WM_NCHITTEST:
            return STATUSBAR_WMNCHitTest (hwnd, wParam, lParam);

	case WM_NCLBUTTONDOWN:
	    return STATUSBAR_WMNCLButtonDown (hwnd, wParam, lParam);

	case WM_NCLBUTTONUP:
	    return STATUSBAR_WMNCLButtonUp (hwnd, wParam, lParam);

	case WM_PAINT:
	    return STATUSBAR_WMPaint (infoPtr, hwnd, wParam);

	case WM_RBUTTONDBLCLK:
	    return STATUSBAR_SendNotify (hwnd, NM_RDBLCLK);

	case WM_RBUTTONUP:
	    return STATUSBAR_SendNotify (hwnd, NM_RCLICK);

	case WM_SETFONT:
	    return STATUSBAR_WMSetFont (infoPtr, hwnd, wParam, lParam);

	case WM_SETTEXT:
	    return STATUSBAR_WMSetText (infoPtr, hwnd, wParam, lParam);

	case WM_SIZE:
	    return STATUSBAR_WMSize (infoPtr, hwnd, wParam, lParam);

	default:
	    if (msg >= WM_USER)
		ERR("unknown msg %04x wp=%04x lp=%08lx\n",
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
STATUS_Register (void)
{
    WNDCLASSA wndClass;

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
STATUS_Unregister (void)
{
    UnregisterClassA (STATUSCLASSNAMEA, (HINSTANCE)NULL);
}

