/*
 * a GUI application for displaying a console
 *	USER32 back end
 * Copyright 2001 Eric Pouech
 */

#include <stdio.h>
#include "winecon_private.h"

/* mapping console colors to RGB values */
static	COLORREF	color_map[16] = 
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x80), RGB(0x00, 0x80, 0x00), RGB(0x00, 0x80, 0x80),
    RGB(0x80, 0x00, 0x00), RGB(0x80, 0x00, 0x80), RGB(0x80, 0x80, 0x00), RGB(0x80, 0x80, 0x80),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xFF), RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0xFF),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0xFF, 0x00), RGB(0xFF, 0xFF, 0xFF),
};

/******************************************************************
 *		WCUSER_FillMemDC
 *
 * Fills the Mem DC with current cells values
 */
static void WCUSER_FillMemDC(const struct inner_data* data, int upd_tp, int upd_bm)
{
    unsigned		i, j, k;
    CHAR_INFO*		cell;
    HFONT		hOldFont;
    WORD		attr;
    WCHAR*		line;

    if (!(line = HeapAlloc(GetProcessHeap(), 0, data->sb_width * sizeof(WCHAR))))
    {Trace(0, "OOM\n"); return;}

    hOldFont = SelectObject(data->hMemDC, data->hFont);
    for (j = upd_tp; j <= upd_bm; j++)
    {
	cell = &data->cells[j * data->sb_width];
	for (i = 0; i < data->win_width; i++)
	{
	    attr = cell[i].Attributes;
	    SetBkColor(data->hMemDC, color_map[attr & 0x0F]);
	    SetTextColor(data->hMemDC, color_map[(attr >> 4) & 0x0F]);
	    for (k = i; k < data->win_width && cell[k].Attributes == attr; k++)
	    {
		line[k - i] = cell[k].Char.UnicodeChar;
	    }
	    TextOut(data->hMemDC, i * data->cell_width, j * data->cell_height, 
		    line, k - i);
	    i = k - 1;
	}
    }
    SelectObject(data->hMemDC, hOldFont);
    HeapFree(GetProcessHeap(), 0, line);
}

/******************************************************************
 *		WCUSER_NewBitmap
 *
 * Either the font geometry or the sb geometry has changed. we need to recreate the
 * bitmap geometry
 */
static void WCUSER_NewBitmap(struct inner_data* data, BOOL fill)
{
    HBITMAP	hnew, hold;

    if (!data->sb_width || !data->sb_height)
	return;
    hnew = CreateCompatibleBitmap(data->hMemDC, 
				  data->sb_width  * data->cell_width, 
				  data->sb_height * data->cell_height);
    hold = SelectObject(data->hMemDC, hnew);

    if (data->hBitmap)
    {
	if (hold == data->hBitmap)
	    DeleteObject(data->hBitmap);
	else
	    Trace(0, "leak\n");
    }
    data->hBitmap = hnew;
    if (fill)
	WCUSER_FillMemDC(data, 0, data->sb_height - 1);
}

/******************************************************************
 *		WCUSER_ResizeScreenBuffer
 *
 *
 */
static void WCUSER_ResizeScreenBuffer(struct inner_data* data)
{
    WCUSER_NewBitmap(data, FALSE);
}

/******************************************************************
 *		WCUSER_PosCursor
 *
 * Set a new position for the cursor
 */
static void	WCUSER_PosCursor(const struct inner_data* data)
{
    if (data->hWnd != GetFocus() || !data->cursor_visible) return;

    SetCaretPos((data->cursor.X - data->win_pos.X) * data->cell_width, 
		(data->cursor.Y - data->win_pos.Y) * data->cell_height);
    ShowCaret(data->hWnd); 
}

/******************************************************************
 *		WCUSER_ShapeCursor
 *
 * Sets a new shape for the cursor
 */
void	WCUSER_ShapeCursor(struct inner_data* data, int size, int vis, BOOL force)
{
    if (force || size != data->cursor_size)
    {
	if (data->cursor_visible && data->hWnd == GetFocus()) DestroyCaret();
	if (data->cursor_bitmap) DeleteObject(data->cursor_bitmap);
	data->cursor_bitmap = (HBITMAP)0;
	if (size != 100)
	{
	    int		w16b; /* number of byets per row, aligned on word size */
	    BYTE*	ptr;
	    int		i, j, nbl;

	    w16b = ((data->cell_width + 15) & ~15) / 8;
	    ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, w16b * data->cell_height);
	    if (!ptr) {Trace(0, "OOM\n"); return;}
	    nbl = max((data->cell_height * size) / 100, 1);
	    for (j = data->cell_height - nbl; j < data->cell_height; j++)
	    {
		for (i = 0; i < data->cell_width; i++)
		{
		    ptr[w16b * j + (i / 8)] |= 0x80 >> (i & 7);
		}
	    }
	    data->cursor_bitmap = CreateBitmap(data->cell_width, data->cell_height, 1, 1, ptr);
	    HeapFree(GetProcessHeap(), 0, ptr);
	}
	data->cursor_size = size;
	data->cursor_visible = -1;
    }

    vis = (vis) ? TRUE : FALSE;
    if (force || vis != data->cursor_visible)
    {
	data->cursor_visible = vis;
	if (data->hWnd == GetFocus())
	{
	    if (vis)
	    {
		CreateCaret(data->hWnd, data->cursor_bitmap, data->cell_width, data->cell_height);
		WCUSER_PosCursor(data);
	    }
	    else
	    {
		DestroyCaret();
	    }
	}
    }
}

/******************************************************************
 *		INECON_ComputePositions
 *
 * Recomputes all the components (mainly scroll bars) positions
 */
void	WCUSER_ComputePositions(struct inner_data* data)
{
    RECT		r;
    int			dx, dy;

    /* compute window size from desired client size */
    r.left = r.top = 0;
    r.right = data->win_width * data->cell_width;
    r.bottom = data->win_height * data->cell_height;

    if (IsRectEmpty(&r))
    {
	ShowWindow(data->hWnd, SW_HIDE);
	return;
    }

    AdjustWindowRect(&r, GetWindowLong(data->hWnd, GWL_STYLE), FALSE);

    dx = dy = 0;
    if (data->sb_width > data->win_width)
    {
	dy = GetSystemMetrics(SM_CYHSCROLL);
	SetScrollRange(data->hWnd, SB_HORZ, 0, data->sb_width - data->win_width, FALSE);
	SetScrollPos(data->hWnd, SB_HORZ, 0, FALSE); /* FIXME */
	ShowScrollBar(data->hWnd, SB_HORZ, TRUE);
    }
    else
    {
	ShowScrollBar(data->hWnd, SB_HORZ, FALSE);
    }

    if (data->sb_height > data->win_height)
    {
	dx = GetSystemMetrics(SM_CXVSCROLL);
	SetScrollRange(data->hWnd, SB_VERT, 0, data->sb_height - data->win_height, FALSE);
	SetScrollPos(data->hWnd, SB_VERT, 0, FALSE); /* FIXME */
	ShowScrollBar(data->hWnd, SB_VERT, TRUE);
    }	
    else
    {
	ShowScrollBar(data->hWnd, SB_VERT, FALSE);
    }

    SetWindowPos(data->hWnd, 0, 0, 0, r.right - r.left + dx, r.bottom - r.top + dy,
		 SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
    WCUSER_ShapeCursor(data, data->cursor_size, data->cursor_visible, TRUE);
    WCUSER_PosCursor(data);
}

/******************************************************************
 *		WCUSER_SetTitle
 *
 * Sets the title to the wine console
 */
static void	WCUSER_SetTitle(const struct inner_data* data)
{
    WCHAR	buffer[256];	

    if (WINECON_GetConsoleTitle(data->hConIn, buffer, sizeof(buffer)))
	SetWindowText(data->hWnd, buffer);
}

/******************************************************************
 *		WCUSER_SetFont
 *
 *
 */
BOOL	WCUSER_SetFont(struct inner_data* data, const LOGFONT* logfont, const TEXTMETRIC* tm)
{
    if (!memcmp(logfont, &data->logFont, sizeof(LOGFONT))) return TRUE;
    if (data->hFont) DeleteObject(data->hFont);
    data->hFont = CreateFontIndirect(logfont);
    if (!data->hFont) {Trace(0, "wrong font\n");return FALSE;}
    data->cell_width = tm->tmMaxCharWidth; /* font is fixed Avg == Max */
    data->cell_height = tm->tmHeight;
    data->logFont = *logfont;
    WCUSER_ComputePositions(data);
    WCUSER_NewBitmap(data, TRUE);
    InvalidateRect(data->hWnd, NULL, FALSE);
    UpdateWindow(data->hWnd);
    return TRUE;
}

/******************************************************************
 *		WCUSER_GetCell
 *
 * Get a cell from the a relative coordinate in window (takes into
 * account the scrolling)
 */
static COORD	WCUSER_GetCell(const struct inner_data* data, LPARAM lParam)
{
    COORD	c;

    c.X = data->win_pos.X + (short)LOWORD(lParam) / data->cell_width;
    c.Y = data->win_pos.Y + (short)HIWORD(lParam) / data->cell_height;

    return c;
}

/******************************************************************
 *		WCUSER_GetSelectionRect
 *
 * Get the selection rectangle
 */
static void	WCUSER_GetSelectionRect(const struct inner_data* data, LPRECT r)
{
    r->left   = (min(data->selectPt1.X, data->selectPt2.X)    ) * data->cell_width;
    r->top    = (min(data->selectPt1.Y, data->selectPt2.Y)    ) * data->cell_height;
    r->right  = (max(data->selectPt1.X, data->selectPt2.X) + 1) * data->cell_width;
    r->bottom = (max(data->selectPt1.Y, data->selectPt2.Y) + 1) * data->cell_height;
}

/******************************************************************
 *		WCUSER_SetSelection
 *
 *
 */
static void	WCUSER_SetSelection(const struct inner_data* data, HDC hRefDC)
{
    HDC		hDC;
    RECT	r;

    WCUSER_GetSelectionRect(data, &r);
    hDC = hRefDC ? hRefDC : GetDC(data->hWnd);
    if (hDC)
    {
	if (data->hWnd == GetFocus() && data->cursor_visible)
	    HideCaret(data->hWnd);
	InvertRect(hDC, &r);
	if (hDC != hRefDC)
	    ReleaseDC(data->hWnd, hDC);
	if (data->hWnd == GetFocus() && data->cursor_visible)
	    ShowCaret(data->hWnd);
    }
}

/******************************************************************
 *		WCUSER_MoveSelection
 *
 *
 */
static void	WCUSER_MoveSelection(struct inner_data* data, COORD dst, BOOL final)
{
    RECT	r;
    HDC		hDC;

    WCUSER_GetSelectionRect(data, &r);
    hDC = GetDC(data->hWnd);
    if (hDC)
    {
	if (data->hWnd == GetFocus() && data->cursor_visible)
	    HideCaret(data->hWnd);
	InvertRect(hDC, &r);
    }
    data->selectPt2 = dst;
    if (hDC)
    {
	WCUSER_GetSelectionRect(data, &r);
	InvertRect(hDC, &r);
	ReleaseDC(data->hWnd, hDC);
	if (data->hWnd == GetFocus() && data->cursor_visible)
	    ShowCaret(data->hWnd);
    }
    if (final)
    {
	ReleaseCapture();
	data->hasSelection = TRUE;
    }
}

/******************************************************************
 *		WCUSER_CopySelectionToClipboard
 *
 * Copies the current selection into the clipboard
 */
static void	WCUSER_CopySelectionToClipboard(const struct inner_data* data)
{
    HANDLE	hMem;
    LPWSTR	p;
    unsigned	w, h;

    w = abs(data->selectPt1.X - data->selectPt2.X) + 2;
    h = abs(data->selectPt1.Y - data->selectPt2.Y) + 1;

    if (!OpenClipboard(data->hWnd)) return;
    EmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, (w * h - 1) * sizeof(WCHAR));
    if (hMem && (p = GlobalLock(hMem)))
    {
	COORD	c;
	int	y;

	c.X = data->win_pos.X + min(data->selectPt1.X, data->selectPt2.X);
	c.Y = data->win_pos.Y + min(data->selectPt1.Y, data->selectPt2.Y);
	
	for (y = 0; y < h; y++, c.Y++)
	{
	    ReadConsoleOutputCharacter(data->hConOut, &p[y * w], w - 1, c, NULL);
	    if (y < h - 1) p[y * w + w - 1] = '\n';
	}
	GlobalUnlock(hMem);
	SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
}

/******************************************************************
 *		WCUSER_PasteFromClipboard
 *
 *
 */
static void	WCUSER_PasteFromClipboard(struct inner_data* data)
{
    HANDLE	h;
    WCHAR*	ptr;

    if (!OpenClipboard(data->hWnd)) return;
    h = GetClipboardData(CF_UNICODETEXT);
    if (h && (ptr = GlobalLock(h)))
    {
	int		i, len = GlobalSize(h) / sizeof(WCHAR);
	INPUT_RECORD	ir[2];
	DWORD		n;
	SHORT		sh;

	ir[0].EventType = KEY_EVENT;
	ir[0].Event.KeyEvent.wRepeatCount = 0;
	ir[0].Event.KeyEvent.dwControlKeyState = 0;
	ir[0].Event.KeyEvent.bKeyDown = TRUE;

	/* generate the corresponding input records */
	for (i = 0; i < len; i++)
	{
	    /* FIXME: the modifying keys are not generated (shift, ctrl...) */
	    sh = VkKeyScan(ptr[i]);
	    ir[0].Event.KeyEvent.wVirtualKeyCode = LOBYTE(sh);
	    ir[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(LOBYTE(sh), 0);
	    ir[0].Event.KeyEvent.uChar.UnicodeChar = ptr[i];
	    
	    ir[1] = ir[0];
	    ir[1].Event.KeyEvent.bKeyDown = FALSE;
	    
	    WriteConsoleInput(data->hConIn, ir, 2, &n);
	}
	GlobalUnlock(h);
    }
    CloseClipboard();
}

static void WCUSER_Refresh(const struct inner_data* data, int tp, int bm)
{
    if (data->win_pos.Y <= bm && data->win_pos.Y + data->win_height >= tp)
    {
	RECT	r;

	r.left   = 0;
	r.right  = data->win_width * data->cell_width;
	r.top    = (tp - data->win_pos.Y) * data->cell_height;
	r.bottom = (bm - data->win_pos.Y + 1) * data->cell_height;
	InvalidateRect(data->hWnd, &r, FALSE);
	WCUSER_FillMemDC(data, tp, bm);
	UpdateWindow(data->hWnd);
    }
}

/******************************************************************
 *		WCUSER_Paint
 *
 *
 */
static void	WCUSER_Paint(const struct inner_data* data)
{
    PAINTSTRUCT		ps;

    BeginPaint(data->hWnd, &ps);
    BitBlt(ps.hdc, 0, 0, data->win_width * data->cell_width, data->win_height * data->cell_height,
	   data->hMemDC, data->win_pos.X * data->cell_width, data->win_pos.Y * data->cell_height,
	   SRCCOPY);
    if (data->hasSelection)
	WCUSER_SetSelection(data, ps.hdc);
    EndPaint(data->hWnd, &ps);
}

/******************************************************************
 *		WCUSER_Scroll
 *
 *
 */
static void WCUSER_Scroll(struct inner_data* data, int pos, BOOL horz)
{
    if (horz)
    {
	SetScrollPos(data->hWnd, SB_HORZ, pos, TRUE);
	data->win_pos.X = pos;
	InvalidateRect(data->hWnd, NULL, FALSE);
    }
    else
    {
	SetScrollPos(data->hWnd, SB_VERT, pos, TRUE);
	data->win_pos.Y = pos;
    }
    InvalidateRect(data->hWnd, NULL, FALSE);
}

struct font_chooser {
    struct inner_data*	data;
    int			done;
};

/******************************************************************
 *		WCUSER_ValidateFontMetric
 *
 * Returns true if the font described in tm is usable as a font for the renderer
 */
BOOL	WCUSER_ValidateFontMetric(const struct inner_data* data, const TEXTMETRIC* tm)
{
    return tm->tmMaxCharWidth * data->win_width < GetSystemMetrics(SM_CXSCREEN) &&
	tm->tmHeight * data->win_height < GetSystemMetrics(SM_CYSCREEN) &&
	!tm->tmItalic && !tm->tmUnderlined && !tm->tmStruckOut;
}

/******************************************************************
 *		WCUSER_ValidateFont
 *
 * Returns true if the font family described in lf is usable as a font for the renderer
 */
BOOL	WCUSER_ValidateFont(const struct inner_data* data, const LOGFONT* lf)
{
    return (lf->lfPitchAndFamily & 3) == FIXED_PITCH && (lf->lfPitchAndFamily & 0xF0) == FF_MODERN;
}

/******************************************************************
 *		get_first_font_enum_2
 *		get_first_font_enum
 *
 * Helper functions to get a decent font for the renderer
 */
static int CALLBACK get_first_font_enum_2(const LOGFONT* lf, const TEXTMETRIC* tm, 
					  DWORD FontType, LPARAM lParam)
{
    struct font_chooser*	fc = (struct font_chooser*)lParam;

    if (WCUSER_ValidateFontMetric(fc->data, tm))
    {
	WCUSER_SetFont(fc->data, lf, tm);
	fc->done = 1;
	return 0;
    }
    return 1;
}

static int CALLBACK get_first_font_enum(const LOGFONT* lf, const TEXTMETRIC* tm, 
					DWORD FontType, LPARAM lParam)
{
    struct font_chooser*	fc = (struct font_chooser*)lParam;

    if (WCUSER_ValidateFont(fc->data, lf))
    {
	EnumFontFamilies(fc->data->hMemDC, lf->lfFaceName, get_first_font_enum_2, lParam);
	return !fc->done; /* we just need the first matching one... */
    }
    return 1;
}

/******************************************************************
 *		WCUSER_Create
 *
 * Creates the window for the rendering
 */
static LRESULT WCUSER_Create(HWND hWnd, LPCREATESTRUCT lpcs)
{
    struct inner_data*	data;
    HMENU		hMenu;
    HMENU		hSubMenu;
    WCHAR		buff[256];
    HINSTANCE		hInstance = GetModuleHandle(NULL);

    data = lpcs->lpCreateParams;
    SetWindowLong(hWnd, 0L, (DWORD)data);
    data->hWnd = hWnd;

    data->cursor_size = 101; /* invalid value, will trigger a complete cleanup */
    /* FIXME: error handling & memory cleanup */
    hSubMenu = CreateMenu();
    if (!hSubMenu) return 0;

    hMenu = GetSystemMenu(hWnd, FALSE);
    if (!hMenu) return 0;

    LoadString(hInstance, IDS_MARK, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_MARK, buff);
    LoadString(hInstance, IDS_COPY, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_COPY, buff);
    LoadString(hInstance, IDS_PASTE, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_PASTE, buff);
    LoadString(hInstance, IDS_SELECTALL, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SELECTALL, buff);
    LoadString(hInstance, IDS_SCROLL, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SCROLL, buff);
    LoadString(hInstance, IDS_SEARCH, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SEARCH, buff);

    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    LoadString(hInstance, IDS_EDIT, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT_PTR)hSubMenu, buff);
    LoadString(hInstance, IDS_DEFAULT, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING, IDS_DEFAULT, buff);
    LoadString(hInstance, IDS_PROPERTY, buff, sizeof(buff) / sizeof(WCHAR));
    InsertMenu(hMenu, -1, MF_BYPOSITION|MF_STRING, IDS_PROPERTY, buff);

    data->hMemDC = CreateCompatibleDC(0);
    if (!data->hMemDC) {Trace(0, "no mem dc\n");return 0;}

    return 0;
}

/******************************************************************
 *		WCUSER_SetMenuDetails
 *
 * Grays / ungrays the menu items according to their state
 */
static void	WCUSER_SetMenuDetails(const struct inner_data* data)
{
    HMENU		hMenu = GetSystemMenu(data->hWnd, FALSE);

    if (!hMenu) {Trace(0, "Issue in getting menu bits\n");return;}

    /* FIXME: set the various menu items to their state (if known) */
    EnableMenuItem(hMenu, IDS_DEFAULT, MF_BYCOMMAND|MF_GRAYED);

    EnableMenuItem(hMenu, IDS_MARK, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hMenu, IDS_COPY, MF_BYCOMMAND|(data->hasSelection ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(hMenu, IDS_PASTE, 
		   MF_BYCOMMAND|(IsClipboardFormatAvailable(CF_UNICODETEXT) 
				 ? MF_ENABLED : MF_GRAYED));
    /* Select all: always active */
    EnableMenuItem(hMenu, IDS_SCROLL, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hMenu, IDS_SEARCH, MF_BYCOMMAND|MF_GRAYED);
}

/******************************************************************
 *		WCUSER_GenerateInputRecord
 *
 * generates input_record from windows WM_KEYUP/WM_KEYDOWN messages
 */
static void    WCUSER_GenerateInputRecord(struct inner_data* data, BOOL down, 
					   WPARAM wParam, LPARAM lParam, BOOL sys)
{
    INPUT_RECORD	ir;
    DWORD		n;
    WCHAR		buf[2];
    BYTE		keyState[256];
    static	WCHAR	last; /* keep last char seen as feed for key up message */

    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = down;
    ir.Event.KeyEvent.wRepeatCount = LOWORD(lParam);
    ir.Event.KeyEvent.wVirtualKeyCode = wParam;
    
    ir.Event.KeyEvent.wVirtualScanCode = HIWORD(lParam) & 0xFF;
    GetKeyboardState(keyState);
    
    ir.Event.KeyEvent.uChar.UnicodeChar = 0;
    ir.Event.KeyEvent.dwControlKeyState = 0;
    if (lParam & (1L << 24))		ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;
    if (keyState[VK_SHIFT]    & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
    if (keyState[VK_CONTROL]  & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED; /* FIXME: gotta choose one */
    if (keyState[VK_LCONTROL] & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
    if (keyState[VK_RCONTROL] & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
    if (sys)				ir.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED; /* FIXME: gotta choose one */
    if (keyState[VK_LMENU]    & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
    if (keyState[VK_RMENU]    & 0x80)	ir.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
    if (keyState[VK_CAPITAL]  & 0x01)	ir.Event.KeyEvent.dwControlKeyState |= CAPSLOCK_ON;	    
    if (keyState[VK_NUMLOCK]  & 0x01)	ir.Event.KeyEvent.dwControlKeyState |= NUMLOCK_ON;
    if (keyState[VK_SCROLL]   & 0x01)	ir.Event.KeyEvent.dwControlKeyState |= SCROLLLOCK_ON;

    if (data->hasSelection && ir.Event.KeyEvent.dwControlKeyState == 0 && 
	ir.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
    {
	data->hasSelection = FALSE;
	WCUSER_SetSelection(data, 0);
	WCUSER_CopySelectionToClipboard(data);
	return;
    }
    
    if (!(ir.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
    {
	if (down)
	{
	    switch (ToUnicode(wParam, HIWORD(lParam), keyState, buf, 2, 0))
	    {
	    case 2:
		/* FIXME... should generate two events... */
		/* fall thru */
	    case 1:	
		last = buf[0];
		break;
	    default:
		last = 0;
		break;
	    }
	}
	ir.Event.KeyEvent.uChar.UnicodeChar = last; /* FIXME HACKY... and buggy 'coz it should be a stack, not a single value */
	if (!down) last = 0;
    }

    WriteConsoleInput(data->hConIn, &ir, 1, &n);
}

/******************************************************************
 *		WCUSER_Proc
 *
 *
 */
static LRESULT CALLBACK WCUSER_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct inner_data*	data = (struct inner_data*)GetWindowLong(hWnd, 0);

    switch (uMsg)
    {
    case WM_CREATE:
        return WCUSER_Create(hWnd, (LPCREATESTRUCT)lParam);
    case WM_DESTROY:
	data->hWnd = 0;
	PostQuitMessage(0);
	break;
    case WM_PAINT:
	WCUSER_Paint(data);
	break;
    case WM_KEYDOWN:
    case WM_KEYUP:
	WCUSER_GenerateInputRecord(data, uMsg == WM_KEYDOWN, wParam, lParam, FALSE);
	break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
	WCUSER_GenerateInputRecord(data, uMsg == WM_SYSKEYDOWN, wParam, lParam, TRUE);
	break;
    case WM_LBUTTONDOWN:
	/* EPP if (wParam != MK_LBUTTON) */
	if (data->hasSelection)
	{
	    data->hasSelection = FALSE;
	}
	else
	{
	    data->selectPt1 = data->selectPt2 = WCUSER_GetCell(data, lParam);
	    SetCapture(data->hWnd);
	}
	WCUSER_SetSelection(data, 0);
	break;
    case WM_MOUSEMOVE:
	/* EPP if (wParam != MK_LBUTTON) */
        if (GetCapture() == data->hWnd)
	{
	    WCUSER_MoveSelection(data, WCUSER_GetCell(data, lParam), FALSE);
	}
	break;
    case WM_LBUTTONUP:
	/* EPP if (wParam != MK_LBUTTON) */
        if (GetCapture() == data->hWnd)
	{
	    WCUSER_MoveSelection(data, WCUSER_GetCell(data, lParam), TRUE);
	}
	break;
    case WM_SETFOCUS:
	if (data->cursor_visible)
	{
	    CreateCaret(data->hWnd, data->cursor_bitmap, data->cell_width, data->cell_height); 
	    WCUSER_PosCursor(data);
	}
        break; 
    case WM_KILLFOCUS: 
	if (data->cursor_visible)
	    DestroyCaret(); 
	break;
    case WM_HSCROLL: 
        {
	    int	pos = data->win_pos.X;

	    switch (LOWORD(wParam)) 
	    { 
            case SB_PAGEUP: 	pos -= 8; 		break; 
            case SB_PAGEDOWN: 	pos += 8; 		break; 
            case SB_LINEUP: 	pos--;			break;
	    case SB_LINEDOWN: 	pos++;	 		break;
            case SB_THUMBTRACK: pos = HIWORD(wParam);	break;
            default: 					break;
	    } 
	    if (pos < 0) pos = 0;
	    if (pos > data->sb_width - data->win_width) pos = data->sb_width - data->win_width;
	    if (pos != data->win_pos.X)
	    {
		ScrollWindow(hWnd, (data->win_pos.X - pos) * data->cell_width, 0, NULL, NULL);
		data->win_pos.X = pos;
		SetScrollPos(hWnd, SB_HORZ, pos, TRUE); 
		UpdateWindow(hWnd); 
		WCUSER_PosCursor(data);
		WINECON_NotifyWindowChange(data);
	    }
        } 
	break;
    case WM_VSCROLL: 
        {
	    int	pos = data->win_pos.Y;

	    switch (LOWORD(wParam)) 
	    { 
            case SB_PAGEUP: 	pos -= 8; 		break; 
            case SB_PAGEDOWN: 	pos += 8; 		break; 
            case SB_LINEUP: 	pos--;			break;
	    case SB_LINEDOWN: 	pos++;	 		break;
            case SB_THUMBTRACK: pos = HIWORD(wParam);	break;
            default: 					break;
	    } 
	    if (pos < 0) pos = 0;
	    if (pos > data->sb_height - data->win_height) pos = data->sb_height - data->win_height;
	    if (pos != data->win_pos.Y)
	    {
		ScrollWindow(hWnd, 0, (data->win_pos.Y - pos) * data->cell_height, NULL, NULL);
		data->win_pos.Y = pos;
		SetScrollPos(hWnd, SB_VERT, pos, TRUE); 
		UpdateWindow(hWnd); 
		WCUSER_PosCursor(data);
		WINECON_NotifyWindowChange(data);
	    }
        } 
	break;
    case WM_SYSCOMMAND:
	switch (wParam)
	{
	case IDS_DEFAULT:
	    Trace(0, "unhandled yet command: %x\n", wParam);
	    break;
	case IDS_PROPERTY:
	    WCUSER_GetProperties(data);
	    break;
	default: 
	    return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	break;
    case WM_RBUTTONDOWN:
	WCUSER_GetProperties(data);
	break;    
    case WM_COMMAND:
	switch (wParam)
	{
	case IDS_MARK:
	    goto niy;
	case IDS_COPY:
	    data->hasSelection = FALSE;
	    WCUSER_SetSelection(data, 0);
	    WCUSER_CopySelectionToClipboard(data);
	    break;
	case IDS_PASTE:
	    WCUSER_PasteFromClipboard(data);
	    break;
	case IDS_SELECTALL:
	    data->selectPt1.X = data->selectPt1.Y = 0;
	    data->selectPt2.X = (data->sb_width - 1) * data->cell_width;
	    data->selectPt2.Y = (data->sb_height - 1) * data->cell_height;
	    WCUSER_SetSelection(data, 0);
	    data->hasSelection = TRUE;
	    break;
	case IDS_SCROLL:
	case IDS_SEARCH:
	niy:
	    Trace(0, "unhandled yet command: %x\n", wParam);
	    break;
	default: 
	    return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	break;
    case WM_INITMENUPOPUP:
	if (!HIWORD(lParam))	return DefWindowProc(hWnd, uMsg, wParam, lParam);
	WCUSER_SetMenuDetails(data);
	break;
    default:
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/******************************************************************
 *		WCUSER_DeleteBackend
 *
 *
 */
void WCUSER_DeleteBackend(struct inner_data* data)
{
    if (data->hWnd)		DestroyWindow(data->hWnd);
    if (data->hFont)		DeleteObject(data->hFont);
    if (data->cursor_bitmap)	DeleteObject(data->cursor_bitmap);
    if (data->hMemDC)		DeleteDC(data->hMemDC);
    if (data->hBitmap)		DeleteObject(data->hBitmap);
}

/******************************************************************
 *		WCUSER_MainLoop
 *
 *
 */
static int WCUSER_MainLoop(struct inner_data* data)
{
    MSG		msg;

    for (;;) 
    {
	switch (MsgWaitForMultipleObjects(1, &data->hSynchro, FALSE, INFINITE, QS_ALLINPUT))
	{
	case WAIT_OBJECT_0:
	    if (!WINECON_GrabChanges(data))
		PostQuitMessage(0);
	    break;
	case WAIT_OBJECT_0+1:
	    switch (GetMessage(&msg, 0, 0, 0))
	    {
	    case -1: /* the event handle became invalid, so exit */
		return -1;
	    case 0: /* WM_QUIT has been posted */
		return 0;
	    default:
		DispatchMessage(&msg);
		break;
	    }
	    break;
	default:
	    Trace(0, "got pb\n");
	    /* err */
	    break;
	}
    }	
}

/******************************************************************
 *		WCUSER_InitBackend
 *
 * Initialisation part II: creation of window.
 *
 */
BOOL WCUSER_InitBackend(struct inner_data* data)
{
    static WCHAR wClassName[] = {'W','i','n','e','C','o','n','s','o','l','e','C','l','a','s','s',0};

    WNDCLASS		wndclass;
    struct font_chooser fc;

    data->fnMainLoop = WCUSER_MainLoop;
    data->fnPosCursor = WCUSER_PosCursor;
    data->fnShapeCursor = WCUSER_ShapeCursor;
    data->fnComputePositions = WCUSER_ComputePositions;
    data->fnRefresh = WCUSER_Refresh;
    data->fnResizeScreenBuffer = WCUSER_ResizeScreenBuffer;
    data->fnSetTitle = WCUSER_SetTitle;
    data->fnScroll = WCUSER_Scroll;
    data->fnDeleteBackend = WCUSER_DeleteBackend;

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WCUSER_Proc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD);
    wndclass.hInstance     = GetModuleHandle(NULL);
    wndclass.hIcon         = LoadIcon(0, IDI_WINLOGO);
    wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = wClassName;
  
    RegisterClass(&wndclass);

    CreateWindow(wndclass.lpszClassName, NULL,
		 WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_HSCROLL|WS_VSCROLL, 
		 CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, wndclass.hInstance, data);   
    if (!data->hWnd) return FALSE;

    /* force update of current data */
    WINECON_GrabChanges(data);

    /* try to find an acceptable font */
    fc.data = data;
    fc.done = 0;
    EnumFontFamilies(data->hMemDC, NULL, get_first_font_enum, (LPARAM)&fc);

    return fc.done;
}



