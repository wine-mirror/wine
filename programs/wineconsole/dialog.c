/* dialog management for wineconsole
 * (c) 2001 Eric Pouech
 */

#include <stdio.h>
#include "winecon_private.h"
#include "commctrl.h"
#include "prsht.h"

/* FIXME: so far, part of the code is made in ASCII because the Uncode property sheet functions
 * are not implemented yet
 */
struct dialog_info 
{
    struct inner_data*	data;		/* pointer to current winecon info */
    HWND		hDlg;		/* handle to window dialog */
    int			nFont;		/* number of font size in size LB */
    struct font_info 
    {
	TEXTMETRIC		tm;
	LOGFONT			lf;
    } 			*font;		/* array of nFont. index sync'ed with SIZE LB */
};

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
	SetWindowLongA(hDlg, DWL_USER, (DWORD)di);
	if (di->data->cursor_size < 33)		idc = IDC_OPT_CURSOR_SMALL;
	else if (di->data->cursor_size < 66)	idc = IDC_OPT_CURSOR_MEDIUM;
	else					idc = IDC_OPT_CURSOR_LARGE;
	SendDlgItemMessage(hDlg, idc, BM_SETCHECK, BST_CHECKED, 0L);
	SetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, WINECON_GetHistorySize(di->data->hConIn),  FALSE);
	if (WINECON_GetHistoryMode(di->data->hConIn))
	    SendDlgItemMessage(hDlg, IDC_OPT_HIST_DOUBLE, BM_SETCHECK, BST_CHECKED, 0L);
	return FALSE; /* because we set the focus */
    case WM_COMMAND:
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;

	di = (struct dialog_info*)GetWindowLongA(hDlg, DWL_USER);
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
	    break;
	case PSN_APPLY:
	{
	    int		curs_size;
	    int		hist_size;
	    BOOL	done;

	    if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_SMALL) == BST_CHECKED)	curs_size = 33;
	    else if (IsDlgButtonChecked(hDlg, IDC_OPT_CURSOR_MEDIUM) == BST_CHECKED) curs_size = 66;
	    else curs_size = 99;
	    if (curs_size != di->data->cursor_size)
	    {
		CONSOLE_CURSOR_INFO cinfo;
		cinfo.dwSize = curs_size;
		cinfo.bVisible = di->data->cursor_visible;
		SetConsoleCursorInfo(di->data->hConOut, &cinfo);
	    }
	    hist_size = GetDlgItemInt(hDlg, IDC_OPT_HIST_SIZE, &done, FALSE);
	    if (done) WINECON_SetHistorySize(di->data->hConIn, hist_size);
	    SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	    WINECON_SetHistoryMode(di->data->hConIn, 
				   IsDlgButtonChecked(hDlg, IDC_OPT_HIST_DOUBLE) & BST_CHECKED);
	    return TRUE;
	}
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
	PAINTSTRUCT	ps;
	int		font_idx;
	int		size_idx;
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
	    int		len1, len2;

	    hFont = CreateFontIndirect(&di->font[size_idx].lf);
	    len1 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_1, 
			      buf1, sizeof(buf1) / sizeof(WCHAR));
	    len2 = LoadString(GetModuleHandle(NULL), IDS_FNT_PREVIEW_2,
			      buf2, sizeof(buf2) / sizeof(WCHAR));
	    if (hFont && len1)
	    {
		hOldFont = SelectObject(ps.hdc, hFont);
		SetBkColor(ps.hdc, RGB(0x00, 0x00, 0x00));
		SetTextColor(ps.hdc, RGB(0xFF, 0xFF, 0xFF));
		TextOut(ps.hdc, 0, 0, buf1, len1);
		if (len2)
		    TextOut(ps.hdc, 0, di->font[size_idx].tm.tmHeight, buf2, len2);
		SelectObject(ps.hdc, hOldFont);
		DeleteObject(hFont);
	    }
	}
	EndPaint(hWnd, &ps);
	break;
    }
    default:
	return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0L;
}

/******************************************************************
 *		font_enum
 *
 *
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
	WCHAR	fmt[] = {'%','l','d',0};
	int	idx;

	/* we want the string to be sorted with a numeric order, not a lexicographic...
	 * do the job by hand... get where to insert the new string
	 */
	for (idx = 0; idx < di->nFont && tm->tmHeight > di->font[idx].tm.tmHeight; idx++);
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
	    if (memcmp(&di->data->logFont, &di->font[idx].lf, sizeof(LOGFONT)) == 0)
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
			   (WPARAM)-1, (LPARAM)di->data->logFont.lfFaceName) == LB_ERR)
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

	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (nmhdr->code) 
	{
	case PSN_APPLY:
	{
	    int idx = SendDlgItemMessage(di->hDlg, IDC_FNT_LIST_SIZE, LB_GETCURSEL, 0L, 0L);

	    if (idx >= 0 && idx < di->nFont)
	    {
		WCUSER_SetFont(di->data, &di->font[idx].lf, &di->font[idx].tm);
	    }
	    SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	    return TRUE;
	}
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
	SetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,   di->data->sb_width,   FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT,  di->data->sb_height,  FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  di->data->win_width,  FALSE);
	SetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, di->data->win_height, FALSE);
	break;
    case WM_COMMAND:
	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (LOWORD(wParam)) 
	{
	}
	break;
    case WM_NOTIFY:
    {
	NMHDR*	nmhdr = (NMHDR*)lParam;

	di = (struct dialog_info*)GetWindowLong(hDlg, DWL_USER);
	switch (nmhdr->code) 
	{
	case PSN_APPLY:
	{
	    COORD	sb;
	    SMALL_RECT	pos;
	    BOOL	st_w, st_h;

	    sb.X = GetDlgItemInt(hDlg, IDC_CNF_SB_WIDTH,  &st_w, FALSE);
	    sb.Y = GetDlgItemInt(hDlg, IDC_CNF_SB_HEIGHT, &st_h, FALSE);
	    if (st_w && st_h && (sb.X != di->data->sb_width || sb.Y != di->data->sb_height))
	    {
		SetConsoleScreenBufferSize(di->data->hConOut, sb);
	    }

	    pos.Right  = GetDlgItemInt(hDlg, IDC_CNF_WIN_WIDTH,  &st_w, FALSE);
	    pos.Bottom = GetDlgItemInt(hDlg, IDC_CNF_WIN_HEIGHT, &st_h, FALSE);
	    if (st_w && st_h && 
		(pos.Right != di->data->win_width || pos.Bottom != di->data->win_height))
	    {
		pos.Left = pos.Top = 0;
		pos.Right--; pos.Bottom--;
		SetConsoleWindowInfo(di->data->hConOut, FALSE, &pos);
	    }

	    SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
	    return TRUE;
	}
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
BOOL WCUSER_GetProperties(struct inner_data* data)
{
    HPROPSHEETPAGE	psPage[3];
    PROPSHEETPAGEA	psp;
    PROPSHEETHEADERA	psHead;
    WNDCLASS		wndclass;
    static WCHAR	szFntPreview[] = {'W','i','n','e','C','o','n','F','o','n','t','P','r','e','v','i','e','w',0};
    struct dialog_info	di;

    InitCommonControls();

    di.data = data;
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

    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = wndclass.hInstance;
    psp.lParam = (LPARAM)&di;

    psp.u.pszTemplate = MAKEINTRESOURCEA(IDD_OPTION);
    psp.pfnDlgProc = WCUSER_OptionDlgProc;
    psPage[0] = CreatePropertySheetPageA(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCEA(IDD_FONT);
    psp.pfnDlgProc = WCUSER_FontDlgProc;
    psPage[1] = CreatePropertySheetPageA(&psp);

    psp.u.pszTemplate = MAKEINTRESOURCEA(IDD_CONFIG);
    psp.pfnDlgProc = WCUSER_ConfigDlgProc;
    psPage[2] = CreatePropertySheetPageA(&psp);

    memset(&psHead, 0, sizeof(psHead));
    psHead.dwSize = sizeof(psHead);
    psHead.pszCaption = "Setup";
    psHead.nPages = 3;
    psHead.hwndParent = data->hWnd;
    psHead.u3.phpage = psPage;
 
    PropertySheetA(&psHead);

    return TRUE;
}

