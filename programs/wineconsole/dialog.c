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
#define NONAMELESSSTRUCT
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
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)di);

	SendMessage(GetDlgItem(hDlg,IDC_OPT_HIST_SIZE_UD), UDM_SETRANGE, 0, MAKELPARAM (500, 0));

	if (di->config.cursor_size <= 25)	idc = IDC_OPT_CURSOR_SMALL;
	else if (di->config.cursor_size <= 50)	idc = IDC_OPT_CURSOR_MEDIUM;
	else				        idc = IDC_OPT_CURSOR_LARGE;
	SendDlgItemMessage(hDlg, idc, BM_SETCHECK, BST_CHECKED, 0L);
	SetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, WINECON_GetHistorySize(di->data->hConIn),  FALSE);
        SendDlgItemMessage(hDlg, IDC_OPT_HIST_NODOUBLE, BM_SETCHECK,
                           (di->config.history_nodup) ? BST_CHECKED : BST_UNCHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_CONF_CTRL, BM_SETCHECK,
                           (di->config.menu_mask & MK_CONTROL) ? BST_CHECKED : BST_UNCHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_CONF_SHIFT, BM_SETCHECK,
                           (di->config.menu_mask & MK_SHIFT) ? BST_CHECKED : BST_UNCHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_QUICK_EDIT, BM_SETCHECK,
                           (di->config.quick_edit) ? BST_CHECKED : BST_UNCHECKED, 0L);
	return FALSE; /* because we set the focus */
    case WM_COMMAND:
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;
        DWORD   val;
        BOOL	done;

	di = (struct dialog_info*)GetWindowLongPtr(hDlg, DWLP_USER);

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
	    PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, idc), TRUE);
            di->hDlg = hDlg;
	    break;
	case PSN_APPLY:
	    if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_SMALL) == BST_CHECKED) val = 25;
	    else if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_MEDIUM) == BST_CHECKED) val = 50;
	    else val = 100;
            di->config.cursor_size = val;

 	    val = GetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, &done, FALSE);
	    if (done) di->config.history_size = val;

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_HIST_NODOUBLE) & BST_CHECKED) ? TRUE : FALSE;
            di->config.history_nodup = val;

            val = 0;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_CTRL) & BST_CHECKED)  val |= MK_CONTROL;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_SHIFT) & BST_CHECKED) val |= MK_SHIFT;
            di->config.menu_mask = val;

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_QUICK_EDIT) & BST_CHECKED) ? TRUE : FALSE;
            di->config.quick_edit = val;

            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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
 *		WCUSER_FontPreviewProc
 *
 * Window proc for font previewer in font property sheet
 */
static LRESULT WINAPI WCUSER_FontPreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hWnd, 0, 0);
        break;
    case WM_GETFONT:
        return GetWindowLongPtr(hWnd, 0);
    case WM_SETFONT:
        SetWindowLongPtr(hWnd, 0, wParam);
        if (LOWORD(lParam))
        {
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
        }
        break;
    case WM_DESTROY:
        {
            HFONT hFont = (HFONT)GetWindowLongPtr(hWnd, 0L);
            if (hFont) DeleteObject(hFont);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT	        ps;
            int		        font_idx;
            int		        size_idx;
            struct dialog_info*	di;
            HFONT	        hFont, hOldFont;

            di = (struct dialog_info*)GetWindowLongPtr(GetParent(hWnd), DWLP_USER);
            BeginPaint(hWnd, &ps);

            font_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0L, 0L);
            size_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);

            hFont = (HFONT)GetWindowLongPtr(hWnd, 0L);
            if (hFont)
            {
                WCHAR	buf1[256];
                WCHAR	buf2[256];
                int	len1, len2;

                len1 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_1,
                                  buf1, sizeof(buf1) / sizeof(WCHAR));
                len2 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_2,
                                  buf2, sizeof(buf2) / sizeof(WCHAR));
                buf1[len1] = buf2[len2] = 0;
                if (len1)
                {
                    hOldFont = SelectObject(ps.hdc, hFont);
                    SetBkColor(ps.hdc, WCUSER_ColorMap[GetWindowLong(GetDlgItem(di->hDlg, IDC_FNT_COLOR_BK), 0)]);
                    SetTextColor(ps.hdc, WCUSER_ColorMap[GetWindowLong(GetDlgItem(di->hDlg, IDC_FNT_COLOR_FG), 0)]);
                    TextOut(ps.hdc, 0, 0, buf1, len1);
                    if (len2)
                        TextOut(ps.hdc, 0, di->font[size_idx].height, buf2, len2);
                    SelectObject(ps.hdc, hOldFont);
                }
            }
            EndPaint(hWnd, &ps);
        }
        break;
    default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
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
            PAINTSTRUCT     ps;
            int             i, step;
            RECT            client, r;
            HBRUSH          hbr;

            BeginPaint(hWnd, &ps);
            GetClientRect(hWnd, &client);
            step = client.right / 8;

            for (i = 0; i < 16; i++)
            {
                r.top = (i / 8) * (client.bottom / 2);
                r.bottom = r.top + client.bottom / 2;
                r.left = (i & 7) * step;
                r.right = r.left + step;
                hbr = CreateSolidBrush(WCUSER_ColorMap[i]);
                FillRect(ps.hdc, &r, hbr);
                DeleteObject(hbr);
                if (GetWindowLong(hWnd, 0) == i)
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
            SetWindowLong(hWnd, 0, i);
            InvalidateRect(GetDlgItem(GetParent(hWnd), IDC_FNT_PREVIEW), NULL, FALSE);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0L;
}

/******************************************************************
 *		font_enum
 *
 * enumerates all the font names with at least one valid font
 */
static int CALLBACK font_enum_size2(const LOGFONT* lf, const TEXTMETRIC* tm,
				    DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;

    WCUSER_DumpTextMetric(tm, FontType);
    if (WCUSER_ValidateFontMetric(di->data, tm, FontType))
    {
	di->nFont++;
    }

    return 1;
}

static int CALLBACK font_enum(const LOGFONT* lf, const TEXTMETRIC* tm,
			      DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;

    WCUSER_DumpLogFont("DlgFamily: ", lf, FontType);
    if (WCUSER_ValidateFont(di->data, lf))
    {
        if (FontType & RASTER_FONTTYPE)
        {
            di->nFont = 0;
            EnumFontFamilies(PRIVATE(di->data)->hMemDC, lf->lfFaceName, font_enum_size2, (LPARAM)di);
        }
        else
            di->nFont = 1;

        if (di->nFont)
	{
 	    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_ADDSTRING,
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
static int CALLBACK font_enum_size(const LOGFONT* lf, const TEXTMETRIC* tm,
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
            lstrcpy(di->font[i].faceName, lf->lfFaceName);
            wsprintf(buf, fmt, sizes[i]);
            SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_INSERTSTRING, i, (LPARAM)buf);
        }
        /* don't need to enumerate other */
        return 0;
    }

    if (WCUSER_ValidateFontMetric(di->data, tm, FontType))
    {
	int	idx;

	/* we want the string to be sorted with a numeric order, not a lexicographic...
	 * do the job by hand... get where to insert the new string
	 */
	for (idx = 0; idx < di->nFont && tm->tmHeight > di->font[idx].height; idx++);
        while (idx < di->nFont &&
               tm->tmHeight == di->font[idx].height &&
               tm->tmWeight > di->font[idx].weight)
            idx++;
        if (idx == di->nFont ||
            tm->tmHeight != di->font[idx].height ||
            tm->tmWeight < di->font[idx].weight)
        {
            /* here we need to add the new entry */
            wsprintf(buf, fmt, tm->tmHeight);
            SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_INSERTSTRING, idx, (LPARAM)buf);

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
            lstrcpy(di->font[idx].faceName, lf->lfFaceName);
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
    LOGFONT     lf;
    HFONT       hFont, hOldFont;
    struct config_data config;

    font_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0L, 0L);
    size_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);

    if (font_idx < 0 || size_idx < 0 || size_idx >= di->nFont)
	return FALSE;

    WCUSER_FillLogFont(&lf, di->font[size_idx].faceName,
                       di->font[size_idx].height, di->font[size_idx].weight);
    hFont = WCUSER_CopyFont(&config, di->data->hWnd, &lf, NULL);
    if (!hFont) return FALSE;

    if (config.cell_height != di->font[size_idx].height)
        WINE_TRACE("mismatched heights (%u<>%u)\n",
                   config.cell_height, di->font[size_idx].height);
    hOldFont = (HFONT)SendDlgItemMessage(di->hDlg, IDC_FNT_PREVIEW, WM_GETFONT, 0L, 0L);

    SendDlgItemMessage(di->hDlg, IDC_FNT_PREVIEW, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (hOldFont) DeleteObject(hOldFont);

    LoadString(GetModuleHandle(NULL), IDS_FNT_DISPLAY, fmt, sizeof(fmt) / sizeof(WCHAR));
    wsprintf(buf, fmt, config.cell_width, config.cell_height);

    SendDlgItemMessage(di->hDlg, IDC_FNT_FONT_INFO, WM_SETTEXT, 0, (LPARAM)buf);

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

    idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0L, 0L);
    if (idx < 0) return FALSE;

    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETTEXT, idx, (LPARAM)lfFaceName);
    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_RESETCONTENT, 0L, 0L);
    HeapFree(GetProcessHeap(), 0, di->font);
    di->nFont = 0;
    di->font = NULL;

    EnumFontFamilies(PRIVATE(di->data)->hMemDC, lfFaceName, font_enum_size, (LPARAM)di);

    if (doInit)
    {
        int     ref = -1;

	for (idx = 0; idx < di->nFont; idx++)
	{
            if (!lstrcmp(di->font[idx].faceName, di->config.face_name) &&
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
    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_SETCURSEL, idx, 0L);
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
    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_RESETCONTENT, 0L, 0L);
    EnumFontFamilies(PRIVATE(di->data)->hMemDC, NULL, font_enum, (LPARAM)di);
    if (SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_SELECTSTRING,
			   (WPARAM)-1, (LPARAM)di->config.face_name) == LB_ERR)
	SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_SETCURSEL, 0L, 0L);
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
	SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR)di);
        /* remove dialog from this control, font will be reset when listboxes are filled */
        SendDlgItemMessage(hDlg, IDC_FNT_PREVIEW, WM_SETFONT, 0L, 0L);
	fill_list_font(di);
        SetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0, (di->config.def_attr >> 4) & 0x0F);
        SetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0, di->config.def_attr & 0x0F);
	break;
    case WM_COMMAND:
	di = (struct dialog_info*)GetWindowLongPtr(hDlg, DWLP_USER);
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

	di = (struct dialog_info*)GetWindowLongPtr(hDlg, DWLP_USER);
	switch (nmhdr->code)
	{
        case PSN_SETACTIVE:
            di->hDlg = hDlg;
            break;
	case PSN_APPLY:
 	    val = SendDlgItemMessage(hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);

	    if (val < di->nFont)
            {
                LOGFONT lf;

                WCUSER_FillLogFont(&lf, di->font[val].faceName,
                                   di->font[val].height, di->font[val].weight);
                DeleteObject(WCUSER_CopyFont(&di->config,
                                             di->data->hWnd, &lf, NULL));
            }

            val = (GetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0) << 4) |
                GetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0);
            di->config.def_attr = val;

            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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

        SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR)di);
        SetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,   di->config.sb_width,   FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT,  di->config.sb_height,  FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  di->config.win_width,  FALSE);
        SetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, di->config.win_height, FALSE);

        SendMessage(GetDlgItem(hDlg,IDC_CNF_WIN_HEIGHT_UD), UDM_SETRANGE, 0, MAKELPARAM (nMaxUD, 0));
        SendMessage(GetDlgItem(hDlg,IDC_CNF_WIN_WIDTH_UD), UDM_SETRANGE, 0, MAKELPARAM (nMaxUD, 0));
        SendMessage(GetDlgItem(hDlg,IDC_CNF_SB_HEIGHT_UD), UDM_SETRANGE, 0, MAKELPARAM (nMaxUD, 0));
        SendMessage(GetDlgItem(hDlg,IDC_CNF_SB_WIDTH_UD), UDM_SETRANGE, 0, MAKELPARAM (nMaxUD, 0));

        SendDlgItemMessage(hDlg, IDC_CNF_CLOSE_EXIT, BM_SETCHECK,
                           (di->config.exit_on_die) ? BST_CHECKED : BST_UNCHECKED, 0L);
        {
            static const WCHAR s1[] = {'W','i','n','3','2',0};
            static const WCHAR s2[] = {'E','m','a','c','s',0};

            SendDlgItemMessage(hDlg, IDC_CNF_EDITION_MODE, CB_ADDSTRING,
                               0, (LPARAM)s1);
            SendDlgItemMessage(hDlg, IDC_CNF_EDITION_MODE, CB_ADDSTRING,
                               0, (LPARAM)s2);
            SendDlgItemMessage(hDlg, IDC_CNF_EDITION_MODE, CB_SETCURSEL,
                               di->config.edition_mode, 0);
    	}

	break;
    case WM_COMMAND:
	di = (struct dialog_info*)GetWindowLongPtr(hDlg, DWLP_USER);
	switch (LOWORD(wParam))
	{
	}
	break;
    case WM_NOTIFY:
    {
	NMHDR*	        nmhdr = (NMHDR*)lParam;
        int             win_w, win_h, sb_w, sb_h;
        BOOL            st1, st2;

	di = (struct dialog_info*)GetWindowLongPtr(hDlg, DWLP_USER);
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
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);        
                return TRUE;
            }
            win_w = GetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  &st1, FALSE);
            win_h = GetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, &st2, FALSE);
            if (!st1 || !st2)
            {
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID); 
                return TRUE;
            }
            if (win_w > sb_w || win_h > sb_h)
            {
                WCHAR   cap[256];
                WCHAR   txt[256];

                LoadString(GetModuleHandle(NULL), IDS_DLG_TIT_ERROR, 
                           cap, sizeof(cap) / sizeof(WCHAR));
                LoadString(GetModuleHandle(NULL), IDS_DLG_ERR_SBWINSIZE, 
                           txt, sizeof(txt) / sizeof(WCHAR));
                
                MessageBox(hDlg, txt, cap, MB_OK);
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID); 
                return TRUE;
            }
            di->config.win_width  = win_w;
            di->config.win_height = win_h;
            di->config.sb_width  = sb_w;
            di->config.sb_height = sb_h;

            di->config.exit_on_die = IsDlgButtonChecked(hDlg, IDC_CNF_CLOSE_EXIT) ? 1 : 0;
            di->config.edition_mode = SendDlgItemMessage(hDlg, IDC_CNF_EDITION_MODE, CB_GETCURSEL,
                                                         0, 0);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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
        SendDlgItemMessage(hDlg, IDC_SAV_SESSION, BM_SETCHECK, BST_CHECKED, 0);
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
    HPROPSHEETPAGE	psPage[3];
    PROPSHEETPAGE	psp;
    PROPSHEETHEADER	psHead;
    WCHAR		buff[256];
    WNDCLASS		wndclass;
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
    wndclass.hInstance     = GetModuleHandle(NULL);
    wndclass.hIcon         = 0;
    wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szFntPreview;
    RegisterClass(&wndclass);

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WCUSER_ColorPreviewProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof(DWORD);
    wndclass.hInstance     = GetModuleHandle(NULL);
    wndclass.hIcon         = 0;
    wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szColorPreview;
    RegisterClass(&wndclass);

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = wndclass.hInstance;
    psp.lParam = (LPARAM)&di;

    psp.u.pszTemplate = MAKEINTRESOURCE(IDD_OPTION);
    psp.pfnDlgProc = WCUSER_OptionDlgProc;
    psPage[0] = CreatePropertySheetPage(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCE(IDD_FONT);
    psp.pfnDlgProc = WCUSER_FontDlgProc;
    psPage[1] = CreatePropertySheetPage(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCE(IDD_CONFIG);
    psp.pfnDlgProc = WCUSER_ConfigDlgProc;
    psPage[2] = CreatePropertySheetPage(&psp);

    memset(&psHead, 0, sizeof(psHead));
    psHead.dwSize = sizeof(psHead);

    if (!LoadString(GetModuleHandle(NULL),
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

    WINECON_DumpConfig("init", refcfg);

    PropertySheet(&psHead);

    if (memcmp(refcfg, &di.config, sizeof(*refcfg)) == 0)
        return TRUE;

    WINECON_DumpConfig("ref", refcfg);
    WINECON_DumpConfig("cur", &di.config);
    if (refcfg == &data->curcfg)
    {
        switch (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SAVE_SETTINGS),
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
