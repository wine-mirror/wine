/* dialog management for wineconsole
 * USER32 backend
 * Copyright (c) 2001 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include "commctrl.h"
#include "prsht.h"
#include "winecon_user.h"

enum WCUSER_ApplyTo {
    /* Prop sheet CFG */
    WCUSER_ApplyToCursorSize, 
    WCUSER_ApplyToHistorySize, WCUSER_ApplyToHistoryMode, WCUSER_ApplyToMenuMask,
    WCUSER_ApplyToEditMode,
    /* Prop sheet FNT */
    WCUSER_ApplyToFont, WCUSER_ApplyToAttribute,
    /* Prop sheep CNF */
    WCUSER_ApplyToBufferSize, WCUSER_ApplyToWindow
};

struct dialog_info 
{
    struct config_data* config;         /* pointer to configuration used for dialog box */
    struct inner_data*	data;	        /* pointer to current winecon info */
    HWND		hDlg;		/* handle to active propsheet */
    int			nFont;		/* number of font size in size LB */
    struct font_info 
    {
	TEXTMETRIC		tm;
	LOGFONT			lf;
    } 			*font;		/* array of nFont. index sync'ed with SIZE LB */
    void        (*apply)(struct dialog_info*, HWND, enum WCUSER_ApplyTo, DWORD);
};

/******************************************************************
 *		WCUSER_ApplyDefault
 *
 *
 */
static void WCUSER_ApplyDefault(struct dialog_info* di, HWND hDlg, enum WCUSER_ApplyTo apply, DWORD val)
{
    switch (apply)
    {
    case WCUSER_ApplyToCursorSize:
    case WCUSER_ApplyToHistorySize:
    case WCUSER_ApplyToHistoryMode:
    case WCUSER_ApplyToBufferSize:
    case WCUSER_ApplyToWindow:
        /* not saving those for now... */
        break;
    case WCUSER_ApplyToMenuMask:
        di->config->menu_mask = val;
        break;
    case WCUSER_ApplyToEditMode:
        di->config->quick_edit = val;
        break;
    case WCUSER_ApplyToFont:
        WCUSER_CopyFont(di->config, &di->font[val].lf);
        break;
    case WCUSER_ApplyToAttribute:
        di->config->def_attr = val;
        break;
    }
    WINECON_RegSave(di->config);
}

/******************************************************************
 *		WCUSER_ApplyCurrent
 *
 *
 */
static void WCUSER_ApplyCurrent(struct dialog_info* di, HWND hDlg, enum WCUSER_ApplyTo apply, DWORD val)
{
    switch (apply)
    {
    case WCUSER_ApplyToCursorSize:
        {
            CONSOLE_CURSOR_INFO cinfo;
            cinfo.dwSize = val;
            cinfo.bVisible = di->config->cursor_visible;
            /* this shall update (through notif) curcfg */
            SetConsoleCursorInfo(di->data->hConOut, &cinfo);
        }
        break;
    case WCUSER_ApplyToHistorySize:
        di->config->history_size = val;
        WINECON_SetHistorySize(di->data->hConIn, val);
        break;
    case WCUSER_ApplyToHistoryMode:
        WINECON_SetHistoryMode(di->data->hConIn, val);
        break;
    case WCUSER_ApplyToMenuMask:
        di->config->menu_mask = val;
        break;
    case WCUSER_ApplyToEditMode:
        di->config->quick_edit = val;
        break;
    case WCUSER_ApplyToFont:
        WCUSER_SetFont(di->data, &di->font[val].lf);
        break;
    case WCUSER_ApplyToAttribute:
        di->config->def_attr = val;
        SetConsoleTextAttribute(di->data->hConOut, val);
        break;
    case WCUSER_ApplyToBufferSize:
        /* this shall update (through notif) curcfg */
        SetConsoleScreenBufferSize(di->data->hConOut, *(COORD*)val);
        break;
    case WCUSER_ApplyToWindow:
        /* this shall update (through notif) curcfg */
        SetConsoleWindowInfo(di->data->hConOut, FALSE, (SMALL_RECT*)val);
        break;
    }
}

/******************************************************************
 *		WCUSER_OptionDlgProc
 *
 * Dialog prop for the option property sheet
 */
static BOOL WINAPI WCUSER_OptionDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;
    unsigned 			idc;
    
    switch (msg) 
    {
    case WM_INITDIALOG:
	di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
	di->hDlg = hDlg;
	SetWindowLong(hDlg, DWL_USER, (DWORD)di);

	if (di->config->cursor_size <= 25)	idc = IDC_OPT_CURSOR_SMALL;
	else if (di->config->cursor_size <= 50)	idc = IDC_OPT_CURSOR_MEDIUM;
	else				        idc = IDC_OPT_CURSOR_LARGE;
	SendDlgItemMessage(hDlg, idc, BM_SETCHECK, BST_CHECKED, 0L);
	SetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, WINECON_GetHistorySize(di->data->hConIn),  FALSE);
	if (WINECON_GetHistoryMode(di->data->hConIn))
	    SendDlgItemMessage(hDlg, IDC_OPT_HIST_DOUBLE, BM_SETCHECK, BST_CHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_CONF_CTRL, BM_SETCHECK, 
                           (di->config->menu_mask & MK_CONTROL) ? BST_CHECKED : BST_UNCHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_CONF_SHIFT, BM_SETCHECK, 
                           (di->config->menu_mask & MK_SHIFT) ? BST_CHECKED : BST_UNCHECKED, 0L);
        SendDlgItemMessage(hDlg, IDC_OPT_QUICK_EDIT, BM_SETCHECK, 
                           (di->config->quick_edit) ? BST_CHECKED : BST_UNCHECKED, 0L);
	return FALSE; /* because we set the focus */
    case WM_COMMAND:
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;
        DWORD   val;
        BOOL	done;

	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);

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
	    else val = 99;
            (di->apply)(di, hDlg, WCUSER_ApplyToCursorSize, val);

 	    val = GetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, &done, FALSE);
	    if (done) (di->apply)(di, hDlg, WCUSER_ApplyToHistorySize, val);

	    (di->apply)(di, hDlg, WCUSER_ApplyToHistoryMode, 
                        IsDlgButtonChecked(hDlg, IDC_OPT_HIST_DOUBLE) & BST_CHECKED);

            val = 0;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_CTRL) & BST_CHECKED)  val |= MK_CONTROL;
            if (IsDlgButtonChecked(hDlg, IDC_OPT_CONF_SHIFT) & BST_CHECKED) val |= MK_SHIFT;
            (di->apply)(di, hDlg, WCUSER_ApplyToMenuMask, val);

            val = (IsDlgButtonChecked(hDlg, IDC_OPT_QUICK_EDIT) & BST_CHECKED) ? TRUE : FALSE;
            (di->apply)(di, hDlg, WCUSER_ApplyToEditMode, val);

            SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
    case WM_PAINT:
        {
            PAINTSTRUCT	        ps;
            int		        font_idx;
            int		        size_idx;
            struct dialog_info*	di;
            
            di = (struct dialog_info*)GetWindowLong(GetParent(hWnd), DWL_USER);
            BeginPaint(hWnd, &ps);

            font_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0L, 0L);
            size_idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);
            
            if (font_idx >= 0 && size_idx >= 0 && size_idx < di->nFont)
            {
                HFONT	hFont, hOldFont;
                WCHAR	buf1[256];
                WCHAR	buf2[256];
                int	len1, len2;
                
                hFont = CreateFontIndirect(&di->font[size_idx].lf);
                len1 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_1, 
                                  buf1, sizeof(buf1) / sizeof(WCHAR));
                len2 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_2,
                                  buf2, sizeof(buf2) / sizeof(WCHAR));
                buf1[len1] = buf2[len2] = 0;
                if (hFont && len1)
                {
                    hOldFont = SelectObject(ps.hdc, hFont);
                    SetBkColor(ps.hdc, WCUSER_ColorMap[GetWindowLong(GetDlgItem(di->hDlg, IDC_FNT_COLOR_BK), 0)]);
                    SetTextColor(ps.hdc, WCUSER_ColorMap[GetWindowLong(GetDlgItem(di->hDlg, IDC_FNT_COLOR_FG), 0)]);
                    TextOut(ps.hdc, 0, 0, buf1, len1);
                    if (len2)
                        TextOut(ps.hdc, 0, di->font[size_idx].tm.tmHeight, buf2, len2);
                    SelectObject(ps.hdc, hOldFont);
                    DeleteObject(hFont);
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

    if (WCUSER_ValidateFontMetric(di->data, tm))
    {
	di->nFont++;
    }
    return 1;
}

static int CALLBACK font_enum(const LOGFONT* lf, const TEXTMETRIC* tm, 
			      DWORD FontType, LPARAM lParam)
{
    struct dialog_info*	di = (struct dialog_info*)lParam;
    HDC	hdc;

    if (WCUSER_ValidateFont(di->data, lf) && (hdc = GetDC(di->hDlg)))
    {
	di->nFont = 0;
	EnumFontFamilies(hdc, lf->lfFaceName, font_enum_size2, (LPARAM)di);
	if (di->nFont)
	{
	    int idx;
	    idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_ADDSTRING, 
				     0, (LPARAM)lf->lfFaceName);
	}
	ReleaseDC(di->hDlg, hdc);
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

    if (WCUSER_ValidateFontMetric(di->data, tm))
    {
	WCHAR	buf[32];
        static const WCHAR fmt[] = {'%','l','d',0};
	int	idx;

	/* we want the string to be sorted with a numeric order, not a lexicographic...
	 * do the job by hand... get where to insert the new string
	 */
	for (idx = 0; idx < di->nFont && tm->tmHeight > di->font[idx].tm.tmHeight; idx++);
        if (idx < di->nFont &&
            tm->tmHeight == di->font[idx].tm.tmHeight && 
            tm->tmMaxCharWidth == di->font[idx].tm.tmMaxCharWidth)
        {
            /* we already have an entry with the same width & height...
             * try to see which TEXTMETRIC (old or new) we should keep... 
             */
            if (di->font[idx].tm.tmWeight != tm->tmWeight)
            {
                /* get the weight closer to 400, the default value */
                if (abs(tm->tmWeight - 400) < abs(di->font[idx].tm.tmWeight - 400))
                {
                    di->font[idx].tm = *tm;
                }
            }
            /* else FIXME: skip the new tm for now */
        }
        else
        {
            /* here we need to add the new entry */
            wsprintfW(buf, fmt, tm->tmHeight);
            SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_INSERTSTRING, idx, (LPARAM)buf);

            /* now grow our arrays and insert to values at the same index than in the list box */
            di->font = HeapReAlloc(GetProcessHeap(), 0, di->font, sizeof(*di->font) * (di->nFont + 1));
            if (idx != di->nFont)
                memmove(&di->font[idx + 1], &di->font[idx], (di->nFont - idx) * sizeof(*di->font));
            di->font[idx].tm = *tm;
            di->font[idx].lf = *lf;
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
    int		idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);
    WCHAR	buf[256];
    WCHAR	fmt[128];

    if (idx < 0 || idx >= di->nFont)
	return FALSE;

    LoadString(GetModuleHandle(NULL), IDS_FNT_DISPLAY, fmt, sizeof(fmt) / sizeof(WCHAR));
    wsprintfW(buf, fmt, di->font[idx].tm.tmMaxCharWidth, di->font[idx].tm.tmHeight);

    SendDlgItemMessage(di->hDlg, IDC_FNT_FONT_INFO, WM_SETTEXT, 0, (LPARAM)buf);
    InvalidateRect(GetDlgItem(di->hDlg, IDC_FNT_PREVIEW), NULL, TRUE);
    UpdateWindow(GetDlgItem(di->hDlg, IDC_FNT_PREVIEW));
    return TRUE;
}

/******************************************************************
 *		fill_list_size
 *
 * fills the size list box according to selected family in font LB
 */
static BOOL  fill_list_size(struct dialog_info* di, BOOL doInit)
{
    HDC 	hdc;
    int		idx;
    WCHAR	lfFaceName[LF_FACESIZE];

    idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETCURSEL, 0L, 0L);
    if (idx < 0) return FALSE;
    
    hdc = GetDC(di->hDlg);
    if (!hdc) return FALSE;

    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_GETTEXT, idx, (LPARAM)lfFaceName);
    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_RESETCONTENT, 0L, 0L);
    if (di->font) HeapFree(GetProcessHeap(), 0, di->font);
    di->nFont = 0;
    di->font = NULL;

    EnumFontFamilies(hdc, lfFaceName, font_enum_size, (LPARAM)di);
    ReleaseDC(di->hDlg, hdc);

    if (doInit)
    {
	for (idx = 0; idx < di->nFont; idx++)
	{
	    if (WCUSER_AreFontsEqual(di->config, &di->font[idx].lf))
		break;
	}
	if (idx == di->nFont) idx = 0;
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
    HDC hdc;

    hdc = GetDC(di->hDlg);
    if (!hdc) return FALSE;

    SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_RESETCONTENT, 0L, 0L);
    EnumFontFamilies(hdc, NULL, font_enum, (LPARAM)di);
    ReleaseDC(di->hDlg, hdc);
    if (SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_SELECTSTRING, 
			   (WPARAM)-1, (LPARAM)di->config->face_name) == LB_ERR)
	SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_FONT, LB_SETCURSEL, 0L, 0L);
    fill_list_size(di, TRUE);
    return TRUE;
}

/******************************************************************
 *		WCUSER_FontDlgProc
 *
 * Dialog proc for the Font property sheet
 */
static BOOL WINAPI WCUSER_FontDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;

    switch (msg) 
    {
    case WM_INITDIALOG:
	di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
	di->hDlg = hDlg;
	SetWindowLong(hDlg, DWL_USER, (DWORD)di);
	fill_list_font(di);
        SetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0, (di->config->def_attr >> 4) & 0x0F);
        SetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0, di->config->def_attr & 0x0F);
	break;
    case WM_COMMAND:
	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
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

	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (nmhdr->code) 
	{
        case PSN_SETACTIVE:
            di->hDlg = hDlg;
            break;
	case PSN_APPLY:
 	    val = SendDlgItemMessage(hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);

	    if (val < di->nFont) (di->apply)(di, hDlg, WCUSER_ApplyToFont, val);

            (di->apply)(di, hDlg, WCUSER_ApplyToAttribute, 
                        (GetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_BK), 0) << 4) |
                        GetWindowLong(GetDlgItem(hDlg, IDC_FNT_COLOR_FG), 0));

            SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
static BOOL WINAPI WCUSER_ConfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct dialog_info*		di;

    switch (msg) 
    {
    case WM_INITDIALOG:
	di = (struct dialog_info*)((PROPSHEETPAGEA*)lParam)->lParam;
	di->hDlg = hDlg;
	SetWindowLong(hDlg, DWL_USER, (DWORD)di);
	SetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,   di->config->sb_width,   FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT,  di->config->sb_height,  FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  di->config->win_width,  FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, di->config->win_height, FALSE);
	break;
    case WM_COMMAND:
	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (LOWORD(wParam)) 
	{
	}
	break;
    case WM_NOTIFY:
    {
	NMHDR*	        nmhdr = (NMHDR*)lParam;
        COORD           sb;
        SMALL_RECT	pos;
        BOOL	        st_w, st_h;

	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (nmhdr->code) 
	{
        case PSN_SETACTIVE:
            di->hDlg = hDlg;
            break;
	case PSN_APPLY:
            sb.X = GetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,  &st_w, FALSE);
            sb.Y = GetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT, &st_h, FALSE);
            if (st_w && st_h && (sb.X != di->config->sb_width || sb.Y != di->config->sb_height))
            {
                (di->apply)(di, hDlg, WCUSER_ApplyToBufferSize, (DWORD)&sb);
            }
            
            pos.Right  = GetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  &st_w, FALSE);
            pos.Bottom = GetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, &st_h, FALSE);
            if (st_w && st_h && 
                (pos.Right != di->config->win_width || pos.Bottom != di->config->win_height))
            {
                pos.Left = pos.Top = 0;
                pos.Right--; pos.Bottom--;
                (di->apply)(di, hDlg, WCUSER_ApplyToWindow, (DWORD)&pos);
            }
            SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
 *		WCUSER_GetProperties
 *
 * Runs the dialog box to set up the winconsole options
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

    InitCommonControls();

    di.data = data;
    if (current)
    {
        di.config = &data->curcfg;
        di.apply = WCUSER_ApplyCurrent;
    }
    else
    {
        di.config = &data->defcfg;
        di.apply = WCUSER_ApplyDefault;
    }
    di.nFont = 0;
    di.font = NULL;

    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WCUSER_FontPreviewProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
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
    psHead.hwndParent = PRIVATE(data)->hWnd;
    psHead.u3.phpage = psPage;
 
    PropertySheet(&psHead);

    return TRUE;
}


