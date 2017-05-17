/*
 * a GUI application for displaying a console
 *	USER32 back end
 * Copyright 2001 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include "winecon_user.h"
#include "winnls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineconsole);
WINE_DECLARE_DEBUG_CHANNEL(wc_font);

UINT g_uiDefaultCharset;

static BOOL WCUSER_SetFont(struct inner_data* data, const LOGFONTW* font);

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
    RECT                r;
    HBRUSH              hbr;
    INT *dx;

    /* no font has been set up yet, don't worry about filling the bitmap,
     * we'll do it once a font is chosen
     */
    if (!PRIVATE(data)->hFont) return;

    /* FIXME: could set up a mechanism to reuse the line between different
     * calls to this function
     */
    if (!(line = HeapAlloc(GetProcessHeap(), 0, data->curcfg.sb_width * sizeof(WCHAR)))) return;
    dx = HeapAlloc( GetProcessHeap(), 0, data->curcfg.sb_width * sizeof(*dx) );

    hOldFont = SelectObject(PRIVATE(data)->hMemDC, PRIVATE(data)->hFont);
    for (j = upd_tp; j <= upd_bm; j++)
    {
	cell = &data->cells[j * data->curcfg.sb_width];
	for (i = 0; i < data->curcfg.sb_width; i++)
	{
	    attr = cell[i].Attributes;
	    SetBkColor(PRIVATE(data)->hMemDC, data->curcfg.color_map[(attr >> 4) & 0x0F]);
	    SetTextColor(PRIVATE(data)->hMemDC, data->curcfg.color_map[attr & 0x0F]);
	    for (k = i; k < data->curcfg.sb_width && cell[k].Attributes == attr; k++)
	    {
		line[k - i] = cell[k].Char.UnicodeChar;
                dx[k - i] = data->curcfg.cell_width;
	    }
            ExtTextOutW( PRIVATE(data)->hMemDC, i * data->curcfg.cell_width, j * data->curcfg.cell_height,
                         0, NULL, line, k - i, dx );
            if (PRIVATE(data)->ext_leading &&
                (hbr = CreateSolidBrush(data->curcfg.color_map[(attr >> 4) & 0x0F])))
            {
                r.left   = i * data->curcfg.cell_width;
                r.top    = (j + 1) * data->curcfg.cell_height - PRIVATE(data)->ext_leading;
                r.right  = k * data->curcfg.cell_width;
                r.bottom = (j + 1) * data->curcfg.cell_height;
                FillRect(PRIVATE(data)->hMemDC, &r, hbr);
                DeleteObject(hbr);
            }
	    i = k - 1;
	}
    }
    SelectObject(PRIVATE(data)->hMemDC, hOldFont);
    HeapFree(GetProcessHeap(), 0, dx);
    HeapFree(GetProcessHeap(), 0, line);
}

/******************************************************************
 *		WCUSER_NewBitmap
 *
 * Either the font geometry or the sb geometry has changed. we need
 * to recreate the bitmap geometry.
 */
static void WCUSER_NewBitmap(struct inner_data* data)
{
    HDC         hDC;
    HBITMAP	hnew, hold;

    if (!data->curcfg.sb_width || !data->curcfg.sb_height ||
        !PRIVATE(data)->hFont || !(hDC = GetDC(data->hWnd)))
        return;
    hnew = CreateCompatibleBitmap(hDC,
				  data->curcfg.sb_width  * data->curcfg.cell_width,
				  data->curcfg.sb_height * data->curcfg.cell_height);
    ReleaseDC(data->hWnd, hDC);
    hold = SelectObject(PRIVATE(data)->hMemDC, hnew);

    if (PRIVATE(data)->hBitmap)
    {
	if (hold == PRIVATE(data)->hBitmap)
	    DeleteObject(PRIVATE(data)->hBitmap);
	else
	    WINE_FIXME("leak\n");
    }
    PRIVATE(data)->hBitmap = hnew;
    WCUSER_FillMemDC(data, 0, data->curcfg.sb_height - 1);
}

/******************************************************************
 *		WCUSER_ResizeScreenBuffer
 *
 *
 */
static void WCUSER_ResizeScreenBuffer(struct inner_data* data)
{
    WCUSER_NewBitmap(data);
}

/******************************************************************
 *		WCUSER_PosCursor
 *
 * Set a new position for the cursor
 */
static void	WCUSER_PosCursor(const struct inner_data* data)
{
    if (data->hWnd != GetFocus() || !data->curcfg.cursor_visible) return;

    SetCaretPos((data->cursor.X - data->curcfg.win_pos.X) * data->curcfg.cell_width,
		(data->cursor.Y - data->curcfg.win_pos.Y) * data->curcfg.cell_height);
    ShowCaret(data->hWnd);
}

/******************************************************************
 *		WCUSER_ShapeCursor
 *
 * Sets a new shape for the cursor
 */
static void	WCUSER_ShapeCursor(struct inner_data* data, int size, int vis, BOOL force)
{
    if (force || size != data->curcfg.cursor_size)
    {
	if (data->curcfg.cursor_visible && data->hWnd == GetFocus()) DestroyCaret();
	if (PRIVATE(data)->cursor_bitmap) DeleteObject(PRIVATE(data)->cursor_bitmap);
	PRIVATE(data)->cursor_bitmap = NULL;
	if (size != 100)
	{
	    int		w16b; /* number of bytes per row, aligned on word size */
	    BYTE*	ptr;
	    int		i, j, nbl;

	    w16b = ((data->curcfg.cell_width + 15) & ~15) / 8;
	    ptr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, w16b * data->curcfg.cell_height);
	    if (!ptr) return;
	    nbl = max((data->curcfg.cell_height * size) / 100, 1);
	    for (j = data->curcfg.cell_height - nbl; j < data->curcfg.cell_height; j++)
	    {
		for (i = 0; i < data->curcfg.cell_width; i++)
		{
		    ptr[w16b * j + (i / 8)] |= 0x80 >> (i & 7);
		}
	    }
	    PRIVATE(data)->cursor_bitmap = CreateBitmap(data->curcfg.cell_width,
                                               data->curcfg.cell_height, 1, 1, ptr);
	    HeapFree(GetProcessHeap(), 0, ptr);
	}
	data->curcfg.cursor_size = size;
	data->curcfg.cursor_visible = -1;
    }

    vis = vis != 0;
    if (force || vis != data->curcfg.cursor_visible)
    {
	data->curcfg.cursor_visible = vis;
	if (data->hWnd == GetFocus())
	{
	    if (vis)
	    {
		CreateCaret(data->hWnd, PRIVATE(data)->cursor_bitmap,
                            data->curcfg.cell_width, data->curcfg.cell_height);
		WCUSER_PosCursor(data);
	    }
	    else
	    {
		DestroyCaret();
	    }
	}
    }
    WINECON_DumpConfig("crsr", &data->curcfg);
}

/******************************************************************
 *		WCUSER_ComputePositions
 *
 * Recomputes all the components (mainly scroll bars) positions
 */
static void	WCUSER_ComputePositions(struct inner_data* data)
{
    RECT		r;
    int			dx, dy;

    /* compute window size from desired client size */
    r.left = r.top = 0;
    r.right = data->curcfg.win_width * data->curcfg.cell_width;
    r.bottom = data->curcfg.win_height * data->curcfg.cell_height;

    if (IsRectEmpty(&r)) return;

    AdjustWindowRect(&r, GetWindowLongW(data->hWnd, GWL_STYLE), FALSE);

    dx = dy = 0;
    if (data->curcfg.sb_width > data->curcfg.win_width)
    {
	dy = GetSystemMetrics(SM_CYHSCROLL);
	SetScrollRange(data->hWnd, SB_HORZ, 0,
                       data->curcfg.sb_width - data->curcfg.win_width, FALSE);
	SetScrollPos(data->hWnd, SB_HORZ, 0, FALSE); /* FIXME */
	ShowScrollBar(data->hWnd, SB_HORZ, TRUE);
    }
    else
    {
	ShowScrollBar(data->hWnd, SB_HORZ, FALSE);
    }

    if (data->curcfg.sb_height > data->curcfg.win_height)
    {
	dx = GetSystemMetrics(SM_CXVSCROLL);
	SetScrollRange(data->hWnd, SB_VERT, 0,
                       data->curcfg.sb_height - data->curcfg.win_height, FALSE);
	SetScrollPos(data->hWnd, SB_VERT, 0, FALSE); /* FIXME */
	ShowScrollBar(data->hWnd, SB_VERT, TRUE);
    }
    else
    {
	ShowScrollBar(data->hWnd, SB_VERT, FALSE);
    }

    SetWindowPos(data->hWnd, 0, 0, 0, r.right - r.left + dx, r.bottom - r.top + dy,
		 SWP_NOMOVE|SWP_NOZORDER);
    WCUSER_ShapeCursor(data, data->curcfg.cursor_size, data->curcfg.cursor_visible, TRUE);
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
        SetWindowTextW(data->hWnd, buffer);
}

void WCUSER_DumpLogFont(const char* pfx, const LOGFONTW* lf, DWORD ft)
{
    WINE_TRACE_(wc_font)("%s %s%s%s%s\n"
                         "\tlf.lfHeight=%d lf.lfWidth=%d lf.lfEscapement=%d lf.lfOrientation=%d\n"
                         "\tlf.lfWeight=%d lf.lfItalic=%u lf.lfUnderline=%u lf.lfStrikeOut=%u\n"
                         "\tlf.lfCharSet=%u lf.lfOutPrecision=%u lf.lfClipPrecision=%u lf.lfQuality=%u\n"
                         "\tlf->lfPitchAndFamily=%u lf.lfFaceName=%s\n",
                         pfx,
                         (ft & RASTER_FONTTYPE) ? "raster" : "",
                         (ft & TRUETYPE_FONTTYPE) ? "truetype" : "",
                         ((ft & (RASTER_FONTTYPE|TRUETYPE_FONTTYPE)) == 0) ? "vector" : "",
                         (ft & DEVICE_FONTTYPE) ? "|device" : "",
                         lf->lfHeight, lf->lfWidth, lf->lfEscapement, lf->lfOrientation,
                         lf->lfWeight, lf->lfItalic, lf->lfUnderline, lf->lfStrikeOut, lf->lfCharSet,
                         lf->lfOutPrecision, lf->lfClipPrecision, lf->lfQuality, lf->lfPitchAndFamily,
                         wine_dbgstr_w(lf->lfFaceName));
}

void WCUSER_DumpTextMetric(const TEXTMETRICW* tm, DWORD ft)
{
    WINE_TRACE_(wc_font)("%s%s%s%s\n"
                         "\ttmHeight=%d tmAscent=%d tmDescent=%d tmInternalLeading=%d tmExternalLeading=%d\n"
                         "\ttmAveCharWidth=%d tmMaxCharWidth=%d tmWeight=%d tmOverhang=%d\n"
                         "\ttmDigitizedAspectX=%d tmDigitizedAspectY=%d\n"
                         "\ttmFirstChar=%d tmLastChar=%d tmDefaultChar=%d tmBreakChar=%d\n"
                         "\ttmItalic=%u tmUnderlined=%u tmStruckOut=%u tmPitchAndFamily=%u tmCharSet=%u\n",
                         (ft & RASTER_FONTTYPE) ? "raster" : "",
                         (ft & TRUETYPE_FONTTYPE) ? "truetype" : "",
                         ((ft & (RASTER_FONTTYPE|TRUETYPE_FONTTYPE)) == 0) ? "vector" : "",
                         (ft & DEVICE_FONTTYPE) ? "|device" : "",
                         tm->tmHeight, tm->tmAscent, tm->tmDescent, tm->tmInternalLeading, tm->tmExternalLeading, tm->tmAveCharWidth,
                         tm->tmMaxCharWidth, tm->tmWeight, tm->tmOverhang, tm->tmDigitizedAspectX, tm->tmDigitizedAspectY,
                         tm->tmFirstChar, tm->tmLastChar, tm->tmDefaultChar, tm->tmBreakChar, tm->tmItalic, tm->tmUnderlined, tm->tmStruckOut,
                         tm->tmPitchAndFamily, tm->tmCharSet);
}

/******************************************************************
 *		WCUSER_AreFontsEqual
 *
 *
 */
static BOOL WCUSER_AreFontsEqual(const struct config_data* config, const LOGFONTW* lf)
{
    return lf->lfHeight == config->cell_height &&
        lf->lfWeight == config->font_weight &&
        !lf->lfItalic && !lf->lfUnderline && !lf->lfStrikeOut &&
        !lstrcmpW(lf->lfFaceName, config->face_name);
}

struct font_chooser
{
    struct inner_data*	data;
    int                 pass;
    BOOL                done;
};

/******************************************************************
 *		WCUSER_ValidateFontMetric
 *
 * Returns true if the font described in tm is usable as a font for the renderer
 */
BOOL WCUSER_ValidateFontMetric(const struct inner_data* data, const TEXTMETRICW* tm,
                               DWORD type, int pass)
{
    switch (pass)  /* we get increasingly lenient in later passes */
    {
    case 0:
        if (type & RASTER_FONTTYPE)
        {
            if (tm->tmMaxCharWidth * data->curcfg.win_width >= GetSystemMetrics(SM_CXSCREEN) ||
                tm->tmHeight * data->curcfg.win_height >= GetSystemMetrics(SM_CYSCREEN))
                return FALSE;
        }
        /* fall through */
    case 1:
        if (tm->tmCharSet != DEFAULT_CHARSET && tm->tmCharSet != g_uiDefaultCharset) return FALSE;
        /* fall through */
    case 2:
        if (tm->tmItalic || tm->tmUnderlined || tm->tmStruckOut) return FALSE;
        break;
    }
    return TRUE;
}

/******************************************************************
 *		WCUSER_ValidateFont
 *
 * Returns true if the font family described in lf is usable as a font for the renderer
 */
BOOL WCUSER_ValidateFont(const struct inner_data* data, const LOGFONTW* lf, int pass)
{
    switch (pass)  /* we get increasingly lenient in later passes */
    {
    case 0:
    case 1:
        if (lf->lfCharSet != DEFAULT_CHARSET && lf->lfCharSet != g_uiDefaultCharset) return FALSE;
        /* fall through */
    case 2:
        if ((lf->lfPitchAndFamily & 3) != FIXED_PITCH) return FALSE;
        /* fall through */
    case 3:
        if (lf->lfFaceName[0] == '@') return FALSE;
        break;
    }
    return TRUE;
}

/******************************************************************
 *		get_first_font_enum_2
 *		get_first_font_enum
 *
 * Helper functions to get a decent font for the renderer
 */
static int CALLBACK get_first_font_enum_2(const LOGFONTW* lf, const TEXTMETRICW* tm,
                                          DWORD FontType, LPARAM lParam)
{
    struct font_chooser*	fc = (struct font_chooser*)lParam;

    WCUSER_DumpTextMetric(tm, FontType);
    if (WCUSER_ValidateFontMetric(fc->data, tm, FontType, fc->pass))
    {
        LOGFONTW mlf = *lf;

        /* Use the default sizes for the font (this is needed, especially for
         * TrueType fonts, so that we get a decent size, not the max size)
         */
        mlf.lfWidth  = fc->data->curcfg.cell_width;
        mlf.lfHeight = fc->data->curcfg.cell_height;
        if (WCUSER_SetFont(fc->data, &mlf))
        {
            struct      config_data     defcfg;

            WCUSER_DumpLogFont("InitChoosing: ", &mlf, FontType);
            fc->done = 1;
            /* since we've modified the current config with new font information,
             * set this information as the new default.
             */
            WINECON_RegLoad(NULL, &defcfg);
            defcfg.cell_width = fc->data->curcfg.cell_width;
            defcfg.cell_height = fc->data->curcfg.cell_height;
            lstrcpyW(defcfg.face_name, fc->data->curcfg.face_name);
            /* Force also its writing back to the registry so that we can get it
             * the next time.
             */
            WINECON_RegSave(&defcfg);
            return 0;
        }
    }
    return 1;
}

static int CALLBACK get_first_font_enum(const LOGFONTW* lf, const TEXTMETRICW* tm,
                                        DWORD FontType, LPARAM lParam)
{
    struct font_chooser*	fc = (struct font_chooser*)lParam;

    WCUSER_DumpLogFont("InitFamily: ", lf, FontType);
    if (WCUSER_ValidateFont(fc->data, lf, fc->pass))
    {
        EnumFontFamiliesW(PRIVATE(fc->data)->hMemDC, lf->lfFaceName,
                          get_first_font_enum_2, lParam);
	return !fc->done; /* we just need the first matching one... */
    }
    return 1;
}

/******************************************************************
 *		WCUSER_CopyFont
 *
 * get the relevant information from the font described in lf and store them
 * in config
 */
HFONT WCUSER_CopyFont(struct config_data* config, HWND hWnd, const LOGFONTW* lf, LONG* el)
{
    TEXTMETRICW tm;
    HDC         hDC;
    HFONT       hFont, hOldFont;
    CPINFO cpinfo;

    if (!(hDC = GetDC(hWnd))) return NULL;
    if (!(hFont = CreateFontIndirectW(lf)))
    {
        ReleaseDC(hWnd, hDC);
        return NULL;
    }
    hOldFont = SelectObject(hDC, hFont);
    GetTextMetricsW(hDC, &tm);
    SelectObject(hDC, hOldFont);
    ReleaseDC(hWnd, hDC);

    config->cell_width  = tm.tmAveCharWidth;
    config->cell_height = tm.tmHeight + tm.tmExternalLeading;
    config->font_weight = tm.tmWeight;
    lstrcpyW(config->face_name, lf->lfFaceName);
    if (el) *el = tm.tmExternalLeading;

    /* FIXME: use maximum width for DBCS codepages since some chars take two cells */
    if (GetCPInfo( GetConsoleOutputCP(), &cpinfo ) && cpinfo.MaxCharSize > 1)
        config->cell_width  = tm.tmMaxCharWidth;

    return hFont;
}

/******************************************************************
 *		WCUSER_FillLogFont
 *
 *
 */
void WCUSER_FillLogFont(LOGFONTW* lf, const WCHAR* name, UINT height, UINT weight)
{
    lf->lfHeight        = height;
    lf->lfWidth         = 0;
    lf->lfEscapement    = 0;
    lf->lfOrientation   = 0;
    lf->lfWeight        = weight;
    lf->lfItalic        = FALSE;
    lf->lfUnderline     = FALSE;
    lf->lfStrikeOut     = FALSE;
    lf->lfCharSet       = DEFAULT_CHARSET;
    lf->lfOutPrecision  = OUT_DEFAULT_PRECIS;
    lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf->lfQuality       = DEFAULT_QUALITY;
    lf->lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpyW(lf->lfFaceName, name);
}

/******************************************************************
 *		WCUSER_SetFont
 *
 * sets logfont as the new font for the console
 */
BOOL WCUSER_SetFont(struct inner_data* data, const LOGFONTW* logfont)
{
    HFONT       hFont;
    LONG        el;

    if (PRIVATE(data)->hFont != 0 && WCUSER_AreFontsEqual(&data->curcfg, logfont))
        return TRUE;

    hFont = WCUSER_CopyFont(&data->curcfg, data->hWnd, logfont, &el);
    if (!hFont) {WINE_ERR("wrong font\n"); return FALSE;}

    if (PRIVATE(data)->hFont) DeleteObject(PRIVATE(data)->hFont);
    PRIVATE(data)->hFont = hFont;
    PRIVATE(data)->ext_leading = el;

    WCUSER_ComputePositions(data);
    WCUSER_NewBitmap(data);
    InvalidateRect(data->hWnd, NULL, FALSE);
    UpdateWindow(data->hWnd);

    return TRUE;
}

/******************************************************************
 *		WCUSER_SetFontPmt
 *
 * Sets a new font for the console.
 * In fact a wrapper for WCUSER_SetFont
 */
static void     WCUSER_SetFontPmt(struct inner_data* data, const WCHAR* font,
                                  unsigned height, unsigned weight)
{
    LOGFONTW            lf;
    struct font_chooser fc;

    WINE_TRACE_(wc_font)("=> %s h=%u w=%u\n",
                         wine_dbgstr_wn(font, -1), height, weight);

    if (font[0] != '\0' && height != 0 && weight != 0)
    {
        WCUSER_FillLogFont(&lf, font, height, weight);
        if (WCUSER_SetFont(data, &lf))
        {
            WCUSER_DumpLogFont("InitReuses: ", &lf, 0);
            return;
        }
    }

    /* try to find an acceptable font */
    WINE_WARN("Couldn't match the font from registry... trying to find one\n");
    fc.data = data;
    fc.done = FALSE;
    for (fc.pass = 0; fc.pass <= 4; fc.pass++)
    {
        EnumFontFamiliesW(PRIVATE(data)->hMemDC, NULL, get_first_font_enum, (LPARAM)&fc);
        if (fc.done) return;
    }
    WINECON_Fatal("Couldn't find a decent font, aborting");
}

/******************************************************************
 *		WCUSER_GetCell
 *
 * Get a cell from a relative coordinate in window (takes into
 * account the scrolling)
 */
static COORD	WCUSER_GetCell(const struct inner_data* data, LPARAM lParam)
{
    COORD	c;

    c.X = data->curcfg.win_pos.X + (short)LOWORD(lParam) / data->curcfg.cell_width;
    c.Y = data->curcfg.win_pos.Y + (short)HIWORD(lParam) / data->curcfg.cell_height;

    return c;
}

/******************************************************************
 *		WCUSER_GetSelectionRect
 *
 * Get the selection rectangle
 */
static void	WCUSER_GetSelectionRect(const struct inner_data* data, LPRECT r)
{
    r->left   = (min(PRIVATE(data)->selectPt1.X, PRIVATE(data)->selectPt2.X)     - data->curcfg.win_pos.X) * data->curcfg.cell_width;
    r->top    = (min(PRIVATE(data)->selectPt1.Y, PRIVATE(data)->selectPt2.Y)     - data->curcfg.win_pos.Y) * data->curcfg.cell_height;
    r->right  = (max(PRIVATE(data)->selectPt1.X, PRIVATE(data)->selectPt2.X) + 1 - data->curcfg.win_pos.X) * data->curcfg.cell_width;
    r->bottom = (max(PRIVATE(data)->selectPt1.Y, PRIVATE(data)->selectPt2.Y) + 1 - data->curcfg.win_pos.Y) * data->curcfg.cell_height;
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
	if (data->hWnd == GetFocus() && data->curcfg.cursor_visible)
	    HideCaret(data->hWnd);
	InvertRect(hDC, &r);
	if (hDC != hRefDC)
	    ReleaseDC(data->hWnd, hDC);
	if (data->hWnd == GetFocus() && data->curcfg.cursor_visible)
	    ShowCaret(data->hWnd);
    }
}

/******************************************************************
 *		WCUSER_MoveSelection
 *
 *
 */
static void	WCUSER_MoveSelection(struct inner_data* data, COORD c1, COORD c2)
{
    RECT	r;
    HDC		hDC;

    if (c1.X < 0 || c1.X >= data->curcfg.sb_width ||
        c2.X < 0 || c2.X >= data->curcfg.sb_width ||
        c1.Y < 0 || c1.Y >= data->curcfg.sb_height ||
        c2.Y < 0 || c2.Y >= data->curcfg.sb_height)
        return;

    WCUSER_GetSelectionRect(data, &r);
    hDC = GetDC(data->hWnd);
    if (hDC)
    {
	if (data->hWnd == GetFocus() && data->curcfg.cursor_visible)
	    HideCaret(data->hWnd);
	InvertRect(hDC, &r);
    }
    PRIVATE(data)->selectPt1 = c1;
    PRIVATE(data)->selectPt2 = c2;
    if (hDC)
    {
	WCUSER_GetSelectionRect(data, &r);
	InvertRect(hDC, &r);
	ReleaseDC(data->hWnd, hDC);
	if (data->hWnd == GetFocus() && data->curcfg.cursor_visible)
	    ShowCaret(data->hWnd);
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

    w = abs(PRIVATE(data)->selectPt1.X - PRIVATE(data)->selectPt2.X) + 2;
    h = abs(PRIVATE(data)->selectPt1.Y - PRIVATE(data)->selectPt2.Y) + 1;

    if (!OpenClipboard(data->hWnd)) return;
    EmptyClipboard();

    hMem = GlobalAlloc(GMEM_MOVEABLE, (w * h) * sizeof(WCHAR));
    if (hMem && (p = GlobalLock(hMem)))
    {
	COORD	c;
	int	y;

	c.X = min(PRIVATE(data)->selectPt1.X, PRIVATE(data)->selectPt2.X);
	c.Y = min(PRIVATE(data)->selectPt1.Y, PRIVATE(data)->selectPt2.Y);

	for (y = 0; y < h; y++, c.Y++)
	{
	    LPWSTR end;
	    DWORD count;

	    ReadConsoleOutputCharacterW(data->hConOut, p, w - 1, c, &count);

	    /* strip spaces from the end of the line */
	    end = p + w - 1;
	    while (end > p && *(end - 1) == ' ')
	        end--;
	    *end = (y < h - 1) ? '\n' : '\0';
	    p = end + 1;
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
            sh = VkKeyScanW(ptr[i]);
	    ir[0].Event.KeyEvent.wVirtualKeyCode = LOBYTE(sh);
            ir[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKeyW(LOBYTE(sh), 0);
	    ir[0].Event.KeyEvent.uChar.UnicodeChar = ptr[i];

	    ir[1] = ir[0];
	    ir[1].Event.KeyEvent.bKeyDown = FALSE;

            WriteConsoleInputW(data->hConIn, ir, 2, &n);
	}
	GlobalUnlock(h);
    }
    CloseClipboard();
}

/******************************************************************
 *		WCUSER_Refresh
 *
 *
 */
static void WCUSER_Refresh(const struct inner_data* data, int tp, int bm)
{
    WCUSER_FillMemDC(data, tp, bm);
    if (data->curcfg.win_pos.Y <= bm && data->curcfg.win_pos.Y + data->curcfg.win_height >= tp)
    {
	RECT	r;

	r.left   = 0;
	r.right  = data->curcfg.win_width * data->curcfg.cell_width;
	r.top    = (tp - data->curcfg.win_pos.Y) * data->curcfg.cell_height;
	r.bottom = (bm - data->curcfg.win_pos.Y + 1) * data->curcfg.cell_height;
	InvalidateRect(data->hWnd, &r, FALSE);
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

    if (data->in_set_config) return; /* in order to avoid some flicker */
    BeginPaint(data->hWnd, &ps);
    BitBlt(ps.hdc, 0, 0,
           data->curcfg.win_width * data->curcfg.cell_width,
           data->curcfg.win_height * data->curcfg.cell_height,
	   PRIVATE(data)->hMemDC,
           data->curcfg.win_pos.X * data->curcfg.cell_width,
           data->curcfg.win_pos.Y * data->curcfg.cell_height,
	   SRCCOPY);
    if (PRIVATE(data)->has_selection)
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
        ScrollWindow(data->hWnd, (data->curcfg.win_pos.X - pos) * data->curcfg.cell_width, 0, NULL, NULL);
	SetScrollPos(data->hWnd, SB_HORZ, pos, TRUE);
	data->curcfg.win_pos.X = pos;
    }
    else
    {
        ScrollWindow(data->hWnd, 0, (data->curcfg.win_pos.Y - pos) * data->curcfg.cell_height, NULL, NULL);
	SetScrollPos(data->hWnd, SB_VERT, pos, TRUE);
	data->curcfg.win_pos.Y = pos;
    }
    InvalidateRect(data->hWnd, NULL, FALSE);
}

/******************************************************************
 *		WCUSER_FillMenu
 *
 *
 */
static BOOL WCUSER_FillMenu(HMENU hMenu, BOOL sep)
{
    HMENU     hSubMenu;
    HINSTANCE hInstance = GetModuleHandleW(NULL);
    WCHAR     buff[256];

    if (!hMenu) return FALSE;

    /* FIXME: error handling & memory cleanup */
    hSubMenu = CreateMenu();
    if (!hSubMenu) return FALSE;

    LoadStringW(hInstance, IDS_MARK, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_MARK, buff);
    LoadStringW(hInstance, IDS_COPY, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_COPY, buff);
    LoadStringW(hInstance, IDS_PASTE, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_PASTE, buff);
    LoadStringW(hInstance, IDS_SELECTALL, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SELECTALL, buff);
    LoadStringW(hInstance, IDS_SCROLL, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SCROLL, buff);
    LoadStringW(hInstance, IDS_SEARCH, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION|MF_STRING, IDS_SEARCH, buff);

    if (sep) InsertMenuW(hMenu, -1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    LoadStringW(hInstance, IDS_EDIT, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hMenu, -1, MF_BYPOSITION|MF_STRING|MF_POPUP, (UINT_PTR)hSubMenu, buff);
    LoadStringW(hInstance, IDS_DEFAULT, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hMenu, -1, MF_BYPOSITION|MF_STRING, IDS_DEFAULT, buff);
    LoadStringW(hInstance, IDS_PROPERTIES, buff, sizeof(buff) / sizeof(buff[0]));
    InsertMenuW(hMenu, -1, MF_BYPOSITION|MF_STRING, IDS_PROPERTIES, buff);

    return TRUE;
}

/******************************************************************
 *		WCUSER_SetMenuDetails
 *
 * Grays / ungrays the menu items according to their state
 */
static void	WCUSER_SetMenuDetails(const struct inner_data* data, HMENU hMenu)
{
    if (!hMenu) {WINE_ERR("Issue in getting menu bits\n");return;}

    EnableMenuItem(hMenu, IDS_COPY,
                   MF_BYCOMMAND|(PRIVATE(data)->has_selection ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(hMenu, IDS_PASTE,
		   MF_BYCOMMAND|(IsClipboardFormatAvailable(CF_UNICODETEXT)
				 ? MF_ENABLED : MF_GRAYED));
    EnableMenuItem(hMenu, IDS_SCROLL, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hMenu, IDS_SEARCH, MF_BYCOMMAND|MF_GRAYED);
}

/******************************************************************
 *		WCUSER_Create
 *
 * Creates the window for the rendering
 */
static LRESULT WCUSER_Create(HWND hWnd, LPCREATESTRUCTW lpcs)
{
    struct inner_data*	data;
    HMENU		hSysMenu;

    data = lpcs->lpCreateParams;
    SetWindowLongPtrW(hWnd, 0, (DWORD_PTR)data);
    data->hWnd = hWnd;

    hSysMenu = GetSystemMenu(hWnd, FALSE);
    if (!hSysMenu) return 0;
    PRIVATE(data)->hPopMenu = CreatePopupMenu();
    if (!PRIVATE(data)->hPopMenu) return 0;

    WCUSER_FillMenu(hSysMenu, TRUE);
    WCUSER_FillMenu(PRIVATE(data)->hPopMenu, FALSE);

    PRIVATE(data)->hMemDC = CreateCompatibleDC(0);
    if (!PRIVATE(data)->hMemDC) {WINE_ERR("no mem dc\n");return 0;}

    data->curcfg.quick_edit = FALSE;
    return 0;
}

/******************************************************************
 *		WCUSER_GetCtrlKeyState
 *
 * Get the console bit mask equivalent to the VK_ status in keyState
 */
static DWORD    WCUSER_GetCtrlKeyState(BYTE* keyState)
{
    DWORD               ret = 0;

    GetKeyboardState(keyState);
    if (keyState[VK_SHIFT]    & 0x80)	ret |= SHIFT_PRESSED;
    if (keyState[VK_LCONTROL] & 0x80)	ret |= LEFT_CTRL_PRESSED;
    if (keyState[VK_RCONTROL] & 0x80)	ret |= RIGHT_CTRL_PRESSED;
    if (keyState[VK_LMENU]    & 0x80)	ret |= LEFT_ALT_PRESSED;
    if (keyState[VK_RMENU]    & 0x80)	ret |= RIGHT_ALT_PRESSED;
    if (keyState[VK_CAPITAL]  & 0x01)	ret |= CAPSLOCK_ON;
    if (keyState[VK_NUMLOCK]  & 0x01)	ret |= NUMLOCK_ON;
    if (keyState[VK_SCROLL]   & 0x01)	ret |= SCROLLLOCK_ON;

    return ret;
}

/******************************************************************
 *		WCUSER_HandleSelectionKey
 *
 * Handles keys while selecting an area
 */
static void WCUSER_HandleSelectionKey(struct inner_data* data, BOOL down,
                                      WPARAM wParam, LPARAM lParam)
{
    BYTE	keyState[256];
    DWORD       state = WCUSER_GetCtrlKeyState(keyState) & ~(CAPSLOCK_ON|NUMLOCK_ON|SCROLLLOCK_ON);
    COORD       c1, c2;

    if (!down) return;

    switch (state)
    {
    case 0:
        switch (wParam)
        {
        case VK_RETURN:
            PRIVATE(data)->has_selection = FALSE;
            WCUSER_SetSelection(data, 0);
            WCUSER_CopySelectionToClipboard(data);
            return;
        case VK_RIGHT:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c1.X++; c2.X++;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_LEFT:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c1.X--; c2.X--;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_UP:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c1.Y--; c2.Y--;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_DOWN:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c1.Y++; c2.Y++;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        }
        break;
    case SHIFT_PRESSED:
        switch (wParam)
        {
        case VK_RIGHT:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c2.X++;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_LEFT:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c2.X--;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_UP:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c2.Y--;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        case VK_DOWN:
            c1 = PRIVATE(data)->selectPt1;
            c2 = PRIVATE(data)->selectPt2;
            c2.Y++;
            WCUSER_MoveSelection(data, c1, c2);
            return;
        }
        break;
    }

    if (wParam < VK_SPACE)  /* Shift, Alt, Ctrl, Num Lock etc. */
        return;
    
    WCUSER_SetSelection(data, 0);    
    PRIVATE(data)->has_selection = FALSE;
}

/******************************************************************
 *		WCUSER_GenerateKeyInputRecord
 *
 * generates input_record from windows WM_KEYUP/WM_KEYDOWN messages
 */
static void    WCUSER_GenerateKeyInputRecord(struct inner_data* data, BOOL down,
                                             WPARAM wParam, LPARAM lParam)
{
    INPUT_RECORD	ir;
    DWORD		n;
    WCHAR		buf[2];
    static	WCHAR	last; /* keep last char seen as feed for key up message */
    BYTE		keyState[256];

    ir.EventType = KEY_EVENT;
    ir.Event.KeyEvent.bKeyDown = down;
    ir.Event.KeyEvent.wRepeatCount = LOWORD(lParam);
    ir.Event.KeyEvent.wVirtualKeyCode = wParam;

    ir.Event.KeyEvent.wVirtualScanCode = HIWORD(lParam) & 0xFF;

    ir.Event.KeyEvent.uChar.UnicodeChar = 0;
    ir.Event.KeyEvent.dwControlKeyState = WCUSER_GetCtrlKeyState(keyState);
    if (lParam & (1L << 24))		ir.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;

    if (down)
    {
        switch (ToUnicode(wParam, HIWORD(lParam), keyState, buf, 2, 0))
        {
        case 2:
            /* FIXME... should generate two events... */
            /* fall through */
        case 1:
            last = buf[0];
            break;
        default:
            last = 0;
            break;
        }
    }
    ir.Event.KeyEvent.uChar.UnicodeChar = last; /* FIXME: HACKY... and buggy because it should be a stack, not a single value */
    if (!down) last = 0;

    WriteConsoleInputW(data->hConIn, &ir, 1, &n);
}

/******************************************************************
 *		WCUSER_GenerateMouseInputRecord
 *
 *
 */
static void    WCUSER_GenerateMouseInputRecord(struct inner_data* data, COORD c,
                                               WPARAM wParam, DWORD event)
{
    INPUT_RECORD	ir;
    BYTE		keyState[256];
    DWORD               mode, n;

    /* MOUSE_EVENTs shouldn't be sent unless ENABLE_MOUSE_INPUT is active */
    if (!GetConsoleMode(data->hConIn, &mode) || !(mode & ENABLE_MOUSE_INPUT))
        return;

    ir.EventType = MOUSE_EVENT;
    ir.Event.MouseEvent.dwMousePosition = c;
    ir.Event.MouseEvent.dwButtonState = 0;
    if (wParam & MK_LBUTTON) ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;
    if (wParam & MK_MBUTTON) ir.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;
    if (wParam & MK_RBUTTON) ir.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;
    if (wParam & MK_CONTROL) ir.Event.MouseEvent.dwButtonState |= LEFT_CTRL_PRESSED;
    if (wParam & MK_SHIFT)   ir.Event.MouseEvent.dwButtonState |= SHIFT_PRESSED;
    if (event == MOUSE_WHEELED) ir.Event.MouseEvent.dwButtonState |= wParam & 0xFFFF0000;
    ir.Event.MouseEvent.dwControlKeyState = WCUSER_GetCtrlKeyState(keyState);
    ir.Event.MouseEvent.dwEventFlags = event;

    WriteConsoleInputW(data->hConIn, &ir, 1, &n);
}

/******************************************************************
 *		WCUSER_Proc
 *
 *
 */
static LRESULT CALLBACK WCUSER_Proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    struct inner_data* data = (struct inner_data*)GetWindowLongPtrW(hWnd, 0);

    switch (uMsg)
    {
    case WM_CREATE:
        return WCUSER_Create(hWnd, (LPCREATESTRUCTW)lParam);
    case WM_DESTROY:
	data->hWnd = 0;
	PostQuitMessage(0);
	break;
    case WM_PAINT:
	WCUSER_Paint(data);
	break;
    case WM_KEYDOWN:
    case WM_KEYUP:
        if (PRIVATE(data)->has_selection)
            WCUSER_HandleSelectionKey(data, uMsg == WM_KEYDOWN, wParam, lParam);
        else
            WCUSER_GenerateKeyInputRecord(data, uMsg == WM_KEYDOWN, wParam, lParam);
	break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
	WCUSER_GenerateKeyInputRecord(data, uMsg == WM_SYSKEYDOWN, wParam, lParam);
	break;
    case WM_LBUTTONDOWN:
        if (data->curcfg.quick_edit || PRIVATE(data)->has_selection)
        {
            if (PRIVATE(data)->has_selection)
                WCUSER_SetSelection(data, 0);
            
            if (data->curcfg.quick_edit && PRIVATE(data)->has_selection)
            {
                PRIVATE(data)->has_selection = FALSE;
            }
            else
            {
                PRIVATE(data)->selectPt1 = PRIVATE(data)->selectPt2 = WCUSER_GetCell(data, lParam);
                SetCapture(data->hWnd);
                WCUSER_SetSelection(data, 0);
                PRIVATE(data)->has_selection = TRUE;
            }
        }
        else
        {
            WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, 0);
        }
	break;
    case WM_MOUSEMOVE:
        if (data->curcfg.quick_edit || PRIVATE(data)->has_selection)
        {
            if (GetCapture() == data->hWnd && PRIVATE(data)->has_selection &&
                (wParam & MK_LBUTTON))
            {
                WCUSER_MoveSelection(data, PRIVATE(data)->selectPt1, WCUSER_GetCell(data, lParam));
            }
        }
        else
        {
            WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, MOUSE_MOVED);
        }
	break;
    case WM_LBUTTONUP:
        if (data->curcfg.quick_edit || PRIVATE(data)->has_selection)
        {
            if (GetCapture() == data->hWnd && PRIVATE(data)->has_selection)
            {
                WCUSER_MoveSelection(data, PRIVATE(data)->selectPt1, WCUSER_GetCell(data, lParam));
                ReleaseCapture();
            }
        }
        else
        {
            WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, 0);
        }
	break;
    case WM_RBUTTONDOWN:
        if ((wParam & (MK_CONTROL|MK_SHIFT)) == data->curcfg.menu_mask)
        {
            POINT       pt;

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            ClientToScreen(hWnd, &pt);
            WCUSER_SetMenuDetails(data, PRIVATE(data)->hPopMenu);
            TrackPopupMenu(PRIVATE(data)->hPopMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON,
                           pt.x, pt.y, 0, hWnd, NULL);
        }
        else
        {
            WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, 0);
        }
	break;
    case WM_RBUTTONUP:
        /* no need to track for rbutton up when opening the popup... the event will be
         * swallowed by TrackPopupMenu */
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, 0);
	break;
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WCUSER_GenerateMouseInputRecord(data, WCUSER_GetCell(data, lParam), wParam, DOUBLE_CLICK);
        break;
    case WM_SETFOCUS:
	if (data->curcfg.cursor_visible)
	{
	    CreateCaret(data->hWnd, PRIVATE(data)->cursor_bitmap,
                        data->curcfg.cell_width, data->curcfg.cell_height);
	    WCUSER_PosCursor(data);
	}
        break;
    case WM_KILLFOCUS:
	if (data->curcfg.cursor_visible)
	    DestroyCaret();
	break;
    case WM_HSCROLL:
        {
            struct config_data  cfg = data->curcfg;

            switch (LOWORD(wParam))
            {
            case SB_PAGEUP: 	cfg.win_pos.X -= 8; 		break;
            case SB_PAGEDOWN: 	cfg.win_pos.X += 8; 		break;
            case SB_LINEUP: 	cfg.win_pos.X--;		break;
            case SB_LINEDOWN: 	cfg.win_pos.X++; 		break;
            case SB_THUMBTRACK: cfg.win_pos.X = HIWORD(wParam);	break;
            default: 					        break;
            }
            if (cfg.win_pos.X < 0) cfg.win_pos.X = 0;
            if (cfg.win_pos.X > data->curcfg.sb_width - data->curcfg.win_width)
                cfg.win_pos.X = data->curcfg.sb_width - data->curcfg.win_width;
            if (cfg.win_pos.X != data->curcfg.win_pos.X)
            {
                WINECON_SetConfig(data, &cfg);
            }
        }
	break;
    case WM_MOUSEWHEEL:
        if (data->curcfg.sb_height <= data->curcfg.win_height)
        {
            WCUSER_GenerateMouseInputRecord(data,  WCUSER_GetCell(data, lParam), wParam, MOUSE_WHEELED);
            break;
        }
        /* else fallthrough */
    case WM_VSCROLL:
        {
	    struct config_data  cfg = data->curcfg;

            if (uMsg == WM_MOUSEWHEEL)
            {
                UINT scrollLines = 3;
                SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &scrollLines, 0);
                scrollLines *= -GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
                cfg.win_pos.Y += scrollLines;
            } else {
                switch (LOWORD(wParam))
                {
                case SB_PAGEUP:     cfg.win_pos.Y -= 8; 		break;
                case SB_PAGEDOWN:   cfg.win_pos.Y += 8; 		break;
                case SB_LINEUP:     cfg.win_pos.Y--;			break;
                case SB_LINEDOWN:   cfg.win_pos.Y++;	 		break;
                case SB_THUMBTRACK: cfg.win_pos.Y = HIWORD(wParam);	break;
                default: 					        break;
                }
            }

	    if (cfg.win_pos.Y < 0) cfg.win_pos.Y = 0;
	    if (cfg.win_pos.Y > data->curcfg.sb_height - data->curcfg.win_height)
                cfg.win_pos.Y = data->curcfg.sb_height - data->curcfg.win_height;
	    if (cfg.win_pos.Y != data->curcfg.win_pos.Y)
	    {
                WINECON_SetConfig(data, &cfg);
	    }
        }
        break;
    case WM_SYSCOMMAND:
	switch (wParam)
	{
	case IDS_DEFAULT:
	    WCUSER_GetProperties(data, FALSE);
	    break;
	case IDS_PROPERTIES:
	    WCUSER_GetProperties(data, TRUE);
	    break;
	default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	break;
    case WM_COMMAND:
	switch (wParam)
	{
	case IDS_DEFAULT:
	    WCUSER_GetProperties(data, FALSE);
	    break;
	case IDS_PROPERTIES:
	    WCUSER_GetProperties(data, TRUE);
	    break;
	case IDS_MARK:
            PRIVATE(data)->selectPt1.X = PRIVATE(data)->selectPt1.Y = 0;
            PRIVATE(data)->selectPt2.X = PRIVATE(data)->selectPt2.Y = 0;
            WCUSER_SetSelection(data, 0);
            PRIVATE(data)->has_selection = TRUE;
	    break;
	case IDS_COPY:
            if (PRIVATE(data)->has_selection)
            {
                PRIVATE(data)->has_selection = FALSE;
                WCUSER_SetSelection(data, 0);
                WCUSER_CopySelectionToClipboard(data);
            }
	    break;
	case IDS_PASTE:
	    WCUSER_PasteFromClipboard(data);
	    break;
	case IDS_SELECTALL:
            PRIVATE(data)->selectPt1.X = PRIVATE(data)->selectPt1.Y = 0;
            PRIVATE(data)->selectPt2.X = (data->curcfg.sb_width - 1) * data->curcfg.cell_width;
            PRIVATE(data)->selectPt2.Y = (data->curcfg.sb_height - 1) * data->curcfg.cell_height;
            WCUSER_SetSelection(data, 0);
            PRIVATE(data)->has_selection = TRUE;
	    break;
	case IDS_SCROLL:
	case IDS_SEARCH:
	    WINE_FIXME("Unhandled yet command: %lx\n", wParam);
	    break;
	default:
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	break;
    case WM_INITMENUPOPUP:
        if (!HIWORD(lParam)) return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	WCUSER_SetMenuDetails(data, GetSystemMenu(data->hWnd, FALSE));
	break;
    case WM_SIZE:
        WINECON_ResizeWithContainer(data, LOWORD(lParam) / data->curcfg.cell_width,
                                    HIWORD(lParam) / data->curcfg.cell_height);
        break;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

/******************************************************************
 *		WCUSER_DeleteBackend
 *
 *
 */
static void WCUSER_DeleteBackend(struct inner_data* data)
{
    if (!PRIVATE(data)) return;
    if (PRIVATE(data)->hMemDC)		DeleteDC(PRIVATE(data)->hMemDC);
    if (data->hWnd)		        DestroyWindow(data->hWnd);
    if (PRIVATE(data)->hFont)		DeleteObject(PRIVATE(data)->hFont);
    if (PRIVATE(data)->cursor_bitmap)	DeleteObject(PRIVATE(data)->cursor_bitmap);
    if (PRIVATE(data)->hBitmap)		DeleteObject(PRIVATE(data)->hBitmap);
    HeapFree(GetProcessHeap(), 0, PRIVATE(data));
}

/******************************************************************
 *		WCUSER_MainLoop
 *
 *
 */
static int WCUSER_MainLoop(struct inner_data* data)
{
    MSG		msg;

    ShowWindow(data->hWnd, data->nCmdShow);
    while (!data->dying || !data->curcfg.exit_on_die)
    {
	switch (MsgWaitForMultipleObjects(1, &data->hSynchro, FALSE, INFINITE, QS_ALLINPUT))
	{
	case WAIT_OBJECT_0:
	    WINECON_GrabChanges(data);
	    break;
	case WAIT_OBJECT_0+1:
            /* need to use PeekMessageW loop instead of simple GetMessage:
	     * multiple messages might have arrived in between,
	     * so GetMessage would lead to delayed processing */
            while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
	    {
                if (msg.message == WM_QUIT) return 1;
                WINE_TRACE("dispatching msg %04x\n", msg.message);
                DispatchMessageW(&msg);
	    }
	    break;
	default:
	    WINE_ERR("got pb\n");
	    /* err */
	    break;
	}
    }
    PostQuitMessage(0);
    return 0;
}

/******************************************************************
 *		WCUSER_InitBackend
 *
 * Initialisation part II: creation of window.
 *
 */
enum init_return WCUSER_InitBackend(struct inner_data* data)
{
    static const WCHAR wClassName[] = {'W','i','n','e','C','o','n','s','o','l','e','C','l','a','s','s',0};

    WNDCLASSW   wndclass;
    CHARSETINFO ci;

    if (!TranslateCharsetInfo((DWORD *)(INT_PTR)GetACP(), &ci, TCI_SRCCODEPAGE))
        return init_failed;
    g_uiDefaultCharset = ci.ciCharset;
    WINE_TRACE_(wc_font)("Code page %d => Default charset: %d\n", GetACP(), g_uiDefaultCharset);

    data->private = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct inner_data_user));
    if (!data->private) return init_failed;

    data->fnMainLoop = WCUSER_MainLoop;
    data->fnPosCursor = WCUSER_PosCursor;
    data->fnShapeCursor = WCUSER_ShapeCursor;
    data->fnComputePositions = WCUSER_ComputePositions;
    data->fnRefresh = WCUSER_Refresh;
    data->fnResizeScreenBuffer = WCUSER_ResizeScreenBuffer;
    data->fnSetTitle = WCUSER_SetTitle;
    data->fnSetFont = WCUSER_SetFontPmt;
    data->fnScroll = WCUSER_Scroll;
    data->fnDeleteBackend = WCUSER_DeleteBackend;

    wndclass.style         = CS_DBLCLKS;
    wndclass.lpfnWndProc   = WCUSER_Proc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD_PTR);
    wndclass.hInstance     = GetModuleHandleW(NULL);
    wndclass.hIcon         = LoadIconW(0, (LPCWSTR)IDI_WINLOGO);
    wndclass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = wClassName;

    RegisterClassW(&wndclass);

    data->hWnd = CreateWindowW(wndclass.lpszClassName, NULL,
                               WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_HSCROLL|WS_VSCROLL,
                               CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, wndclass.hInstance, data);
    if (!data->hWnd) return init_not_supported;

    return init_success;
}
