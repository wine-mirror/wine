/* dialog management for wineconsole
 * USER32 backend
 * Copyright (c) 2001, 2002 Eric Pouech
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
#include <stdarg.h>

#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "prsht.h"
#include "winecon_user.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineconsole);

struct dialog_info
{
    struct config_data  config;         /* configuration used for dialog box */
    struct inner_data*	data;	        /* pointer to current winecon info */
    HWND		hDlg;		/* handle to active propsheet */
    int			nFont;		/* number of font size in size LB */
    struct font_info
    {
        UINT                    height;
        UINT                    weight;
        WCHAR                   faceName[LF_FACESIZE];
    } 			*font;		/* array of nFont. index sync'ed with SIZE LB */
};

/******************************************************************
 *		WCUSER_OptionDlgProc
 *
 * Dialog prop for the option property sheet
 */
static INT_PTR WINAPI WCUSER_OptionDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;
    unsigned 			idc;

    switch (msg)
    {
    case WM_INITDIALOG:
	di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
	di->hDlg = hDlg;
        SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)di);

        SendMessageW(GetDlgItem(hDlg,IDC_OPT_HIST_SIZE_UD), UDM_SETRANGE, 0, MAKELPARAM(500, 0));

	if (di->config.cursor_size <= 25)	idc = IDC_OPT_CURSOR_SMALL;
	else if (di->config.cursor_size <= 50)	idc = IDC_OPT_CURSOR_MEDIUM;
	else				        idc = IDC_OPT_CURSOR_LARGE;
        SendDlgItemMessageW(hDlg, idc, BM_SETCHECK, BST_CHECKED, 0);
	SetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, di->config.history_size,  FALSE);
        SendDlgItemMessageW(hDlg, IDC_OPT_HIST_NODOUBLE, BM_SETCHECK,
                            (di->config.history_nodup) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hDlg, IDC_OPT_INSERT_MODE, BM_SETCHECK,
                            (di->config.insert_mode) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hDlg, IDC_OPT_CONF_CTRL, BM_SETCHECK,
                            (di->config.menu_mask & MK_CONTROL) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hDlg, IDC_OPT_CONF_SHIFT, BM_SETCHECK,
                            (di->config.menu_mask & MK_SHIFT) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hDlg, IDC_OPT_QUICK_EDIT, BM_SETCHECK,
                            (di->config.quick_edit) ? BST_CHECKED : BST_UNCHECKED, 0);
	return FALSE; /* because we set the focus */
    case WM_COMMAND:
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;
        DWORD   val;
        BOOL	done;

        di = (struct dialog_info*)GetWindowLongPtrW(hDlg, DWLP_USER);

	switch (nmhdr->code)
	{
	case PSN_SETACTIVE:
	    /* needed in propsheet to keep properly the selected radio button
	     * otherwise, the focus would be set to the first tab stop in the
	     * propsheet, which would always activate the first radio button
	     */
	    if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_SMALL) == BST_CHECKED)
		idc = IDC_OPT_CURSOR_SMALL;
	    else if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_MEDIUM) == BST_CHECKED)
		idc = IDC_OPT_CURSOR_MEDIUM;
	    else
		idc = IDC_OPT_CURSOR_LARGE;
            PostMessageW(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, idc), TRUE);
            di->hDlg = hDlg;
	    break;
	case PSN_APPLY:
	    if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_SMALL) == BST_CHECKED) val = 25;
	    else if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_MEDIUM) == BST_CHECKED) val = 50;
	    else val = 100;
            di->config.cursor_size = val;

 	    val = GetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, &done, FALSE);
	    if (done) di->config.history_size = val;

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_HIST_NODOUBLE) & BST_CHECKED) != 0;
            di->config.history_nodup = val;

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_INSERT_MODE) & BST_CHECKED) != 0;
            di->config.insert_mode = val;

            val = 0;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_CTRL) & BST_CHECKED)  val |= MK_CONTROL;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_SHIFT) & BST_CHECKED) val |= MK_SHIFT;
            di->config.menu_mask = val;

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_QUICK_EDIT) & BST_CHECKED) != 0;
            di->config.quick_edit = val;

            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	    return TRUE;
	default:
	    return FALSE;
	}
	break;
    }
    default:
	return FALSE;
    }
    return TRUE;
}

static COLORREF get_color(struct dialog_info *di, unsigned int idc)
{
    LONG_PTR index;

    index = GetWindowLongPtrW(GetDlgItem(di->hDlg, idc), 0);
    return di->config.color_map[index];
}

/******************************************************************
 *		WCUSER_FontPreviewProc
 *
 * Window proc for font previewer in font property sheet
 */
static LRESULT WINAPI WCUSER_FontPreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetWindowLongPtrW(hWnd, 0, 0);
        break;
    case WM_GETFONT:
        return GetWindowLongPtrW(hWnd, 0);
    case WM_SETFONT:
        SetWindowLongPtrW(hWnd, 0, wParam);
        if (LOWORD(lParam))
        {
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
        }
        break;
    case WM_DESTROY:
        {
            HFONT hFont = (HFONT)GetWindowLongPtrW(hWnd, 0);
            if (hFont) DeleteObject(hFont);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT	        ps;
            int		        size_idx;
            struct dialog_info*	di;
            HFONT	        hFont, hOldFont;

            di = (struct dialog_info*)GetWindowLongPtrW(GetParent(hWnd), DWLP_USER);
            BeginPaint(hWnd, &ps);

            size_idx = SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0, 0);

            hFont = (HFONT)GetWindowLongPtrW(hWnd, 0);
            if (hFont)
            {
                COLORREF bkcolor;
                WCHAR ascii[] = {'A','S','C','I','I',':',' ','a','b','c','X','Y','Z','\0'};
                WCHAR buf[256];
                int   len;

                hOldFont = SelectObject(ps.hdc, hFont);
                bkcolor = get_color(di, IDC_FNT_COLOR_BK);
                FillRect(ps.hdc, &ps.rcPaint, CreateSolidBrush(bkcolor));
                SetBkColor(ps.hdc, bkcolor);
                SetTextColor(ps.hdc, get_color(di, IDC_FNT_COLOR_FG));
                len = LoadStringW(GetModuleHandleW(NULL), IDS_FNT_PREVIEW,
                                  buf, sizeof(buf) / sizeof(buf[0]));
                if (len)
                    TextOutW(ps.hdc, 0, 0, buf, len);
                TextOutW(ps.hdc, 0, di->font[size_idx].height, ascii,
                         sizeof(ascii)/sizeof(ascii[0]) - 1);
                SelectObject(ps.hdc, hOldFont);
            }
            EndPaint(hWnd, &ps);
        }
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0L;
}

/******************************************************************
 *		WCUSER_ColorPreviewProc
 *
 * Window proc for color previewer in font property sheet
 */
static LRESULT WINAPI WCUSER_ColorPreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            int i, step;
            RECT client, r;
            struct dialog_info *di;
            HBRUSH hbr;

            BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &client);
            step = client.right / 8;

            di = (struct dialog_info *)GetWindowLongPtrW(GetParent(hWnd), DWLP_USER);

            for (i = 0; i < 16; i++)
            {
                r.top = (i / 8) * (client.bottom / 2);
                r.bottom = r.top + client.bottom / 2;
                r.left = (i & 7) * step;
                r.right = r.left + step;
                hbr = CreateSolidBrush(di->config.color_map[i]);
                FillRect(ps.hdc, &r, hbr);
                DeleteObject(hbr);
                if (GetWindowLongW(hWnd, 0) == i)
                {
                    HPEN        hOldPen;
                    int         i = 2;

                    hOldPen = SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
                    r.right--; r.bottom--;
                    for (;;)
                    {
                        MoveToEx(ps.hdc, r.left, r.bottom, NULL);
                        LineTo(ps.hdc, r.left, r.top);
                        LineTo(ps.hdc, r.right, r.top);
                        SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
                        LineTo(ps.hdc, r.right, r.bottom);
                        LineTo(ps.hdc, r.left, r.bottom);

                        if (--i == 0) break;
                        r.left++; r.top++; r.right--; r.bottom--;
                        SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
                    }
                    SelectObject(ps.hdc, hOldPen);
                }
            }
            EndPaint(hWnd, &ps);
            break;
        }
    case WM_LBUTTONDOWN:
        {
            int             i, step;
            RECT            client;

            GetClientRect(hWnd, &client);
            step = client.right / 8;
            i = (HIWORD(lParam) >= client.bottom / 2) ? 8 : 0;
            i += LOWORD(lParam) / step;
            SetWindowLongW(hWnd, 0, i);
            InvalidateRect(GetDlgItem(GetParent(hWnd), IDC_FNT_PREVIEW), NULL, FALSE);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0L;
}

/******************************************************************
 *		font_enum
 *
 * enumerates all the font names with at least one valid font
 */
static int CALLBACK font_enum_size2(const LOGFONTW* lf, const TEXTMETRICW* tm,
                                    DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;

    WCUSER_DumpTextMetric(tm, FontType);
    if (WCUSER_ValidateFontMetric(di->data, tm, FontType, 0))
    {
	di->nFont++;
    }

    return 1;
}

static int CALLBACK font_enum(const LOGFONTW* lf, const TEXTMETRICW* tm,
                              DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;

    WCUSER_DumpLogFont("DlgFamily: ", lf, FontType);
    if (WCUSER_ValidateFont(di->data, lf, 0))
    {
        if (FontType & RASTER_FONTTYPE)
        {
            di->nFont = 0;
            EnumFontFamiliesW(PRIVATE(di->data)->hMemDC, lf->lfFaceName, font_enum_size2, (LPARAM)di);
        }
        else
            di->nFont = 1;

        if (di->nFont)
	{
            SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_ADDSTRING,
                                0, (LPARAM)lf->lfFaceName);
        }
    }

    return 1;
}

/******************************************************************
 *		font_enum_size
 *
 *
 */
static int CALLBACK font_enum_size(const LOGFONTW* lf, const TEXTMETRICW* tm,
                                   DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;
    WCHAR	        buf[32];
    static const WCHAR  fmt[] = {'%','l','d',0};

    WCUSER_DumpTextMetric(tm, FontType);
    if (di->nFont == 0 && !(FontType & RASTER_FONTTYPE))
    {
        static const int sizes[] = {8,9,10,11,12,14,16,18,20,22,24,26,28,36,48,72};
        int i;

        di->nFont = sizeof(sizes) / sizeof(sizes[0]);
        di->font = HeapAlloc(GetProcessHeap(), 0, di->nFont * sizeof(di->font[0]));
        for (i = 0; i < di->nFont; i++)
        {
            /* drop sizes where window size wouldn't fit on screen */
            if (sizes[i] * di->data->curcfg.win_height > GetSystemMetrics(SM_CYSCREEN))
            {
                di->nFont = i;
                break;
            }
            di->font[i].height = sizes[i];
            di->font[i].weight = 400;
            lstrcpyW(di->font[i].faceName, lf->lfFaceName);
            wsprintfW(buf, fmt, sizes[i]);
            SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_INSERTSTRING, i, (LPARAM)buf);
        }
        /* don't need to enumerate other */
        return 0;
    }

    if (WCUSER_ValidateFontMetric(di->data, tm, FontType, 0))
    {
	int	idx = 0;

	/* we want the string to be sorted with a numeric order, not a lexicographic...
	 * do the job by hand... get where to insert the new string
	 */
	while (idx < di->nFont && tm->tmHeight > di->font[idx].height)
            idx++;
        while (idx < di->nFont &&
               tm->tmHeight == di->font[idx].height &&
               tm->tmWeight > di->font[idx].weight)
            idx++;
        if (idx == di->nFont ||
            tm->tmHeight != di->font[idx].height ||
            tm->tmWeight < di->font[idx].weight)
        {
            /* here we need to add the new entry */
            wsprintfW(buf, fmt, tm->tmHeight);
            SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_INSERTSTRING, idx, (LPARAM)buf);

            /* now grow our arrays and insert the values at the same index than in the list box */
            if (di->nFont)
            {
                di->font = HeapReAlloc(GetProcessHeap(), 0, di->font, sizeof(*di->font) * (di->nFont + 1));
                if (idx != di->nFont)
                    memmove(&di->font[idx + 1], &di->font[idx], (di->nFont - idx) * sizeof(*di->font));
            }
            else
                di->font = HeapAlloc(GetProcessHeap(), 0, sizeof(*di->font));
            di->font[idx].height = tm->tmHeight;
            di->font[idx].weight = tm->tmWeight;
            lstrcpyW(di->font[idx].faceName, lf->lfFaceName);
            di->nFont++;
        }
    }
    return 1;
}

/******************************************************************
 *		select_font
 *
 *
 */
static BOOL  select_font(struct dialog_info* di)
{
    int		font_idx, size_idx;
    WCHAR	buf[256];
    WCHAR	fmt[128];
    DWORD_PTR   args[2];
    LOGFONTW    lf;
    HFONT       hFont, hOldFont;
    struct config_data config;

    font_idx = SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0, 0);
    size_idx = SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0, 0);

    if (font_idx < 0 || size_idx < 0 || size_idx >= di->nFont)
	return FALSE;

    WCUSER_FillLogFont(&lf, di->font[size_idx].faceName,
                       di->font[size_idx].height, di->font[size_idx].weight);
    hFont = WCUSER_CopyFont(&config, di->data->hWnd, &lf, NULL);
    if (!hFont) return FALSE;

    if (config.cell_height != di->font[size_idx].height)
        WINE_TRACE("mismatched heights (%u<>%u)\n",
                   config.cell_height, di->font[size_idx].height);
    hOldFont = (HFONT)SendDlgItemMessageW(di->hDlg, IDC_FNT_PREVIEW, WM_GETFONT, 0, 0);

    SendDlgItemMessageW(di->hDlg, IDC_FNT_PREVIEW, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (hOldFont) DeleteObject(hOldFont);

    LoadStringW(GetModuleHandleW(NULL), IDS_FNT_DISPLAY, fmt, sizeof(fmt) / sizeof(fmt[0]));
    args[0] = config.cell_width;
    args[1] = config.cell_height;
    FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                   fmt, 0, 0, buf, sizeof(buf)/sizeof(*buf), (__ms_va_list*)args);

    SendDlgItemMessageW(di->hDlg, IDC_FNT_FONT_INFO, WM_SETTEXT, 0, (LPARAM)buf);

    return TRUE;
}

/******************************************************************
 *		fill_list_size
 *
 * fills the size list box according to selected family in font LB
 */
static BOOL  fill_list_size(struct dialog_info* di, BOOL doInit)
{
    int		idx;
    WCHAR	lfFaceName[LF_FACESIZE];

    idx = SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0, 0);
    if (idx < 0) return FALSE;

    SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_GETTEXT, idx, (LPARAM)lfFaceName);
    SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_RESETCONTENT, 0, 0);
    HeapFree(GetProcessHeap(), 0, di->font);
    di->nFont = 0;
    di->font = NULL;

    EnumFontFamiliesW(PRIVATE(di->data)->hMemDC, lfFaceName, font_enum_size, (LPARAM)di);

    if (doInit)
    {
        int     ref = -1;

	for (idx = 0; idx < di->nFont; idx++)
	{
            if (!lstrcmpW(di->font[idx].faceName, di->config.face_name) &&
                di->font[idx].height == di->config.cell_height &&
                di->font[idx].weight == di->config.font_weight)
            {
                if (ref == -1) ref = idx;
                else WINE_TRACE("Several matches found: ref=%d idx=%d\n", ref, idx);
            }
	}
	idx = (ref == -1) ? 0 : ref;
    }
    else
	idx = 0;
    SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_SIZE, LB_SETCURSEL, idx, 0);
    select_font(di);
    return TRUE;
}

/******************************************************************
 *		fill_list_font
 *
 * Fills the font LB
 */
static BOOL fill_list_font(struct dialog_info* di)
{
    SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_RESETCONTENT, 0, 0);
    EnumFontFamiliesW(PRIVATE(di->data)->hMemDC, NULL, font_enum, (LPARAM)di);
    if (SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_SELECTSTRING,
                            -1, (LPARAM)di->config.face_name) == LB_ERR)
        SendDlgItemMessageW(di->hDlg, IDC_FNT_LIST_FONT, LB_SETCURSEL, 0, 0);
    fill_list_size(di, TRUE);
    return TRUE;
}

/******************************************************************
 *		WCUSER_FontDlgProc
 *
 * Dialog proc for the Font property sheet
 */
static INT_PTR WINAPI WCUSER_FontDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;

    switch (msg)
    {
    case WM_INITDIALOG:
	di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
	di->hDlg = hDlg;
        SetWindowLongPtrW(hDlg, DWLP_USER, (DWORD_PTR)di);
        /* remove dialog from this control, font will be reset when listboxes are filled */
        SendDlgItemMessageW(hDlg, IDC_FNT_PREVIEW, WM_SETFONT, 0, 0);
	fill_list_font(di);
        SetWindowLongW(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0, (di->config.def_attr >> 4) & 0x0F);
        SetWindowLongW(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0, di->config.def_attr & 0x0F);
	break;
    case WM_COMMAND:
        di = (struct dialog_info*)GetWindowLongPtrW(hDlg, DWLP_USER);
	switch (LOWORD(wParam))
	{
	case IDC_FNT_LIST_FONT:
	    if (HIWORD(wParam) == LBN_SELCHANGE)
	    {
		fill_list_size(di, FALSE);
	    }
	    break;
	case IDC_FNT_LIST_SIZE:
	    if (HIWORD(wParam) == LBN_SELCHANGE)
	    {
		select_font(di);
	    }
	    break;
	}
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;
        DWORD   val;

        di = (struct dialog_info*)GetWindowLongPtrW(hDlg, DWLP_USER);
	switch (nmhdr->code)
	{
        case PSN_SETACTIVE:
            di->hDlg = hDlg;
            break;
	case PSN_APPLY:
            val = SendDlgItemMessageW(hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0, 0);

	    if (val < di->nFont)
            {
                LOGFONTW lf;

                WCUSER_FillLogFont(&lf, di->font[val].faceName,
                                   di->font[val].height, di->font[val].weight);
                DeleteObject(WCUSER_CopyFont(&di->config,
                                             di->data->hWnd, &lf, NULL));
            }

            val = (GetWindowLongW(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0) << 4) |
                   GetWindowLongW(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0);
            di->config.def_attr = val;

            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	    return TRUE;
	default:
	    return FALSE;
	}
	break;
    }
    default:
	return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		WCUSER_ConfigDlgProc
 *
 * Dialog proc for the config property sheet
 */
static INT_PTR WINAPI WCUSER_ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;
    int                     nMaxUD = 2000;

    switch (msg)
    {
    case WM_INITDIALOG:
        di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
        di->hDlg = hDlg;

        SetWindowLongPtrW(hDlg, DWLP_USER, (DWORD_PTR)di);
        SetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,   di->config.sb_width,   FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT,  di->config.sb_height,  FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  di->config.win_width,  FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, di->config.win_height, FALSE);

        SendMessageW(GetDlgItem(hDlg,IDC_CNF_WIN_HEIGHT_UD), UDM_SETRANGE, 0, MAKELPARAM(nMaxUD, 0));
        SendMessageW(GetDlgItem(hDlg,IDC_CNF_WIN_WIDTH_UD), UDM_SETRANGE, 0, MAKELPARAM(nMaxUD, 0));
        SendMessageW(GetDlgItem(hDlg,IDC_CNF_SB_HEIGHT_UD), UDM_SETRANGE, 0, MAKELPARAM(nMaxUD, 0));
        SendMessageW(GetDlgItem(hDlg,IDC_CNF_SB_WIDTH_UD), UDM_SETRANGE, 0, MAKELPARAM(nMaxUD, 0));

        SendDlgItemMessageW(hDlg, IDC_CNF_CLOSE_EXIT, BM_SETCHECK,
                            (di->config.exit_on_die) ? BST_CHECKED : BST_UNCHECKED, 0);
        {
            static const WCHAR s1[] = {'W','i','n','3','2',0};
            static const WCHAR s2[] = {'E','m','a','c','s',0};

            SendDlgItemMessageW(hDlg, IDC_CNF_EDITION_MODE, CB_ADDSTRING, 0, (LPARAM)s1);
            SendDlgItemMessageW(hDlg, IDC_CNF_EDITION_MODE, CB_ADDSTRING, 0, (LPARAM)s2);
            SendDlgItemMessageW(hDlg, IDC_CNF_EDITION_MODE, CB_SETCURSEL, di->config.edition_mode, 0);
    	}

	break;
    case WM_COMMAND:
        di = (struct dialog_info*)GetWindowLongPtrW(hDlg, DWLP_USER);
	switch (LOWORD(wParam))
	{
	}
	break;
    case WM_NOTIFY:
    {
	NMHDR*	        nmhdr = (NMHDR*)lParam;
        int             win_w, win_h, sb_w, sb_h;
        BOOL            st1, st2;

        di = (struct dialog_info*)GetWindowLongPtrW(hDlg, DWLP_USER);
	switch (nmhdr->code)
	{
        case PSN_SETACTIVE:
            di->hDlg = hDlg;
            break;
	case PSN_APPLY:
            sb_w = GetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,  &st1, FALSE);
            sb_h = GetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT, &st2, FALSE);
            if (!st1 || ! st2)
            {
                SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                return TRUE;
            }
            win_w = GetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  &st1, FALSE);
            win_h = GetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, &st2, FALSE);
            if (!st1 || !st2)
            {
                SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                return TRUE;
            }
            if (win_w > sb_w || win_h > sb_h)
            {
                WCHAR   cap[256];
                WCHAR   txt[256];

                LoadStringW(GetModuleHandleW(NULL), IDS_DLG_TIT_ERROR,
                            cap, sizeof(cap) / sizeof(cap[0]));
                LoadStringW(GetModuleHandleW(NULL), IDS_DLG_ERR_SBWINSIZE,
                            txt, sizeof(txt) / sizeof(cap[0]));

                MessageBoxW(hDlg, txt, cap, MB_OK);
                SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                return TRUE;
            }
            di->config.win_width  = win_w;
            di->config.win_height = win_h;
            di->config.sb_width  = sb_w;
            di->config.sb_height = sb_h;

            di->config.exit_on_die = IsDlgButtonChecked(hDlg, IDC_CNF_CLOSE_EXIT) ? 1 : 0;
            di->config.edition_mode = SendDlgItemMessageW(hDlg, IDC_CNF_EDITION_MODE,
                                                          CB_GETCURSEL, 0, 0);
            SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
	    return TRUE;
	default:
	    return FALSE;
	}
	break;
    }
    default:
	return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		WCUSER_SaveDlgProc
 *
 *      Dialog Procedure for choosing how to handle modification to the
 * console settings.
 */
static INT_PTR WINAPI WCUSER_SaveDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hDlg, IDC_SAV_SESSION, BM_SETCHECK, BST_CHECKED, 0);
	break;
    case WM_COMMAND:
	switch (LOWORD(wParam))
	{
        case IDOK:
            EndDialog(hDlg,
                      (IsDlgButtonChecked(hDlg, IDC_SAV_SAVE) == BST_CHECKED) ?
                      IDC_SAV_SAVE : IDC_SAV_SESSION);
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL); break;
	}
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		WCUSER_GetProperties
 *
 * Runs the dialog box to set up the wineconsole options
 */
BOOL WCUSER_GetProperties(struct inner_data* data, BOOL current)
{
    HPROPSHEETPAGE   psPage[3];
    PROPSHEETPAGEW   psp;
    PROPSHEETHEADERW psHead;
    WCHAR            buff[256];
    WNDCLASSW        wndclass;
    static const WCHAR szFntPreview[] = {'W','i','n','e','C','o','n','F','o','n','t','P','r','e','v','i','e','w',0};
    static const WCHAR szColorPreview[] = {'W','i','n','e','C','o','n','C','o','l','o','r','P','r','e','v','i','e','w',0};
    struct dialog_info	di;
    struct config_data  defcfg;
    struct config_data* refcfg;
    BOOL                save, modify_session;

    InitCommonControls();

    di.data = data;
    if (current)
    {
        refcfg = &data->curcfg;
        save = FALSE;
    }
    else
    {
        WINECON_RegLoad(NULL, refcfg = &defcfg);
        save = TRUE;
    }
    di.config = *refcfg;
    di.nFont = 0;
    di.font = NULL;

    modify_session = FALSE;

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WCUSER_FontPreviewProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof (DWORD_PTR); /* for hFont */
    wndclass.hInstance     = GetModuleHandleW(NULL);
    wndclass.hIcon         = 0;
    wndclass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szFntPreview;
    RegisterClassW(&wndclass);

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WCUSER_ColorPreviewProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD);
    wndclass.hInstance     = GetModuleHandleW(NULL);
    wndclass.hIcon         = 0;
    wndclass.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szColorPreview;
    RegisterClassW(&wndclass);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = wndclass.hInstance;
    psp.lParam = (LPARAM)&di;

    psp.u.pszTemplate = MAKEINTRESOURCEW(IDD_OPTION);
    psp.pfnDlgProc = WCUSER_OptionDlgProc;
    psPage[0] = CreatePropertySheetPageW(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCEW(IDD_FONT);
    psp.pfnDlgProc = WCUSER_FontDlgProc;
    psPage[1] = CreatePropertySheetPageW(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCEW(IDD_CONFIG);
    psp.pfnDlgProc = WCUSER_ConfigDlgProc;
    psPage[2] = CreatePropertySheetPageW(&psp);

    memset(&psHead, 0, sizeof(psHead));
    psHead.dwSize = sizeof(psHead);

    if (!LoadStringW(GetModuleHandleW(NULL),
                     (current) ? IDS_DLG_TIT_CURRENT : IDS_DLG_TIT_DEFAULT,
                     buff, sizeof(buff) / sizeof(buff[0])))
    {
        buff[0] = 'S';
        buff[1] = 'e';
        buff[2] = 't';
        buff[3] = 'u';
        buff[4] = 'p';
        buff[5] = '\0';
    }

    psHead.pszCaption = buff;
    psHead.nPages = 3;
    psHead.hwndParent = data->hWnd;
    psHead.u3.phpage = psPage;
    psHead.dwFlags = PSH_NOAPPLYNOW;

    WINECON_DumpConfig("init", refcfg);

    PropertySheetW(&psHead);

    if (memcmp(refcfg, &di.config, sizeof(*refcfg)) == 0)
        return TRUE;

    WINECON_DumpConfig("ref", refcfg);
    WINECON_DumpConfig("cur", &di.config);
    if (refcfg == &data->curcfg)
    {
        switch (DialogBoxW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_SAVE_SETTINGS),
                           data->hWnd, WCUSER_SaveDlgProc))
        {
        case IDC_SAV_SAVE:      save = TRUE; modify_session = TRUE; break;
        case IDC_SAV_SESSION:   modify_session = TRUE; break;
        case IDCANCEL:          break;
        default: WINE_ERR("ooch\n");
        }
    }

    if (modify_session) WINECON_SetConfig(data, &di.config);
    if (save)           WINECON_RegSave(&di.config);

    return TRUE;
}
