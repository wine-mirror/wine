/*
 * Graphics configuration code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2003-2004 Mike Hearn
 * Copyright 2005 Raphael Junqueira
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
 *
 */

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <stdlib.h>

#include <windows.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define RES_MAXLEN 5 /* max number of digits in a screen dimension. 5 digits should be plenty */
#define MINDPI 96
#define MAXDPI 480
#define DEFDPI 96

#define IDT_DPIEDIT 0x1234

static const UINT dpi_values[] = { 96, 120, 144, 168, 192, 216, 240, 288, 336, 384, 432, 480 };

static BOOL updating_ui;

/* convert the x11 desktop key to the new explorer config */
static void convert_x11_desktop_key(void)
{
    WCHAR *buf;

    if (!(buf = get_reg_key(config_key, L"X11 Driver", L"Desktop", NULL))) return;
    set_reg_key(config_key, L"Explorer\\Desktops", L"Default", buf);
    set_reg_key(config_key, L"Explorer", L"Desktop", L"Default");
    set_reg_key(config_key, L"X11 Driver", L"Desktop", NULL);
    free(buf);
}

static void update_gui_for_desktop_mode(HWND dialog)
{
    WCHAR *buf, *bufindex;
    const WCHAR *desktop_name = current_app ? current_app : L"Default";

    WINE_TRACE("\n");
    updating_ui = TRUE;

    buf = get_reg_key(config_key, L"Explorer\\Desktops", desktop_name, NULL);
    if (buf && (bufindex = wcschr(buf, 'x')))
    {
        *bufindex++ = 0;

        SetDlgItemTextW(dialog, IDC_DESKTOP_WIDTH, buf);
        SetDlgItemTextW(dialog, IDC_DESKTOP_HEIGHT, bufindex);
    } else {
        SetDlgItemTextW(dialog, IDC_DESKTOP_WIDTH, L"800");
        SetDlgItemTextW(dialog, IDC_DESKTOP_HEIGHT, L"600");
    }
    free(buf);

    /* do we have desktop mode enabled? */
    if (reg_key_exists(config_key, keypath(L"Explorer"), L"Desktop"))
    {
	CheckDlgButton(dialog, IDC_ENABLE_DESKTOP, BST_CHECKED);
        enable(IDC_DESKTOP_WIDTH);
        enable(IDC_DESKTOP_HEIGHT);
        enable(IDC_DESKTOP_SIZE);
        enable(IDC_DESKTOP_BY);
    }
    else
    {
	CheckDlgButton(dialog, IDC_ENABLE_DESKTOP, BST_UNCHECKED);
	disable(IDC_DESKTOP_WIDTH);
	disable(IDC_DESKTOP_HEIGHT);
	disable(IDC_DESKTOP_SIZE);
	disable(IDC_DESKTOP_BY);
    }

    updating_ui = FALSE;
}

static BOOL can_enable_desktop(void)
{
    WCHAR *value;
    UINT guid_atom;
    BOOL ret = FALSE;
    WCHAR key[sizeof("System\\CurrentControlSet\\Control\\Video\\{}\\0000") + 40];

    guid_atom = HandleToULong(GetPropW(GetDesktopWindow(), L"__wine_display_device_guid"));
    wcscpy( key, L"System\\CurrentControlSet\\Control\\Video\\{" );
    if (!GlobalGetAtomNameW(guid_atom, key + wcslen(key), 40)) return ret;
    wcscat( key, L"}\\0000" );
    if ((value = get_reg_key(HKEY_LOCAL_MACHINE, key, L"GraphicsDriver", NULL)))
    {
        if(wcscmp(value, L"winemac.drv"))
            ret = TRUE;
        free(value);
    }
    return ret;
}

static void init_dialog(HWND dialog)
{
    WCHAR *buf;
    BOOL enable_desktop;

    convert_x11_desktop_key();
    if ((enable_desktop = can_enable_desktop()))
        update_gui_for_desktop_mode(dialog);
    else
        disable(IDC_ENABLE_DESKTOP);

    updating_ui = TRUE;

    if (enable_desktop)
    {
        SendDlgItemMessageW(dialog, IDC_DESKTOP_WIDTH, EM_LIMITTEXT, RES_MAXLEN, 0);
        SendDlgItemMessageW(dialog, IDC_DESKTOP_HEIGHT, EM_LIMITTEXT, RES_MAXLEN, 0);
    }

    buf = get_reg_key(config_key, keypath(L"X11 Driver"), L"GrabFullscreen", L"N");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_FULLSCREEN_GRAB, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_FULLSCREEN_GRAB, BST_UNCHECKED);
    free(buf);

    buf = get_reg_key(config_key, keypath(L"X11 Driver"), L"Managed", L"Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_UNCHECKED);
    free(buf);

    buf = get_reg_key(config_key, keypath(L"X11 Driver"), L"Decorated", L"Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_DECORATED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_DECORATED, BST_UNCHECKED);
    free(buf);

    updating_ui = FALSE;
}

static void set_from_desktop_edits(HWND dialog)
{
    WCHAR *width, *height;
    int w = 800, h = 600;
    WCHAR buffer[32];
    const WCHAR *desktop_name = current_app ? current_app : L"Default";

    if (updating_ui) return;

    WINE_TRACE("\n");

    width = get_text(dialog, IDC_DESKTOP_WIDTH);
    height = get_text(dialog, IDC_DESKTOP_HEIGHT);

    if (width && width[0]) w = max( 640, wcstol(width, NULL, 10) );
    if (height && height[0]) h = max( 480, wcstol(height, NULL, 10) );

    swprintf( buffer, ARRAY_SIZE(buffer), L"%ux%u", w, h );
    set_reg_key(config_key, L"Explorer\\Desktops", desktop_name, buffer);
    set_reg_key(config_key, keypath(L"Explorer"), L"Desktop", desktop_name);

    free(width);
    free(height);
}

static void on_enable_desktop_clicked(HWND dialog) {
    WINE_TRACE("\n");
    
    if (IsDlgButtonChecked(dialog, IDC_ENABLE_DESKTOP) == BST_CHECKED) {
        set_from_desktop_edits(dialog);
    } else {
        set_reg_key(config_key, keypath(L"Explorer"), L"Desktop", NULL);
    }
    
    update_gui_for_desktop_mode(dialog);
}

static void on_enable_managed_clicked(HWND dialog) {
    WINE_TRACE("\n");
    
    if (IsDlgButtonChecked(dialog, IDC_ENABLE_MANAGED) == BST_CHECKED) {
        set_reg_key(config_key, keypath(L"X11 Driver"), L"Managed", L"Y");
    } else {
        set_reg_key(config_key, keypath(L"X11 Driver"), L"Managed", L"N");
    }
}

static void on_enable_decorated_clicked(HWND dialog) {
    WINE_TRACE("\n");

    if (IsDlgButtonChecked(dialog, IDC_ENABLE_DECORATED) == BST_CHECKED) {
        set_reg_key(config_key, keypath(L"X11 Driver"), L"Decorated", L"Y");
    } else {
        set_reg_key(config_key, keypath(L"X11 Driver"), L"Decorated", L"N");
    }
}

static void on_fullscreen_grab_clicked(HWND dialog)
{
    if (IsDlgButtonChecked(dialog, IDC_FULLSCREEN_GRAB) == BST_CHECKED)
        set_reg_key(config_key, keypath(L"X11 Driver"), L"GrabFullscreen", L"Y");
    else
        set_reg_key(config_key, keypath(L"X11 Driver"), L"GrabFullscreen", L"N");
}

static INT read_logpixels_reg(void)
{
    DWORD dwLogPixels;
    WCHAR *buf = get_reg_key(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"LogPixels", NULL);
    if (!buf) buf = get_reg_key(HKEY_CURRENT_CONFIG, L"Software\\Fonts", L"LogPixels", NULL);
    dwLogPixels = buf ? *buf : DEFDPI;
    free(buf);
    return dwLogPixels;
}

static void init_dpi_editbox(HWND hDlg)
{
    DWORD dwLogpixels;

    updating_ui = TRUE;

    dwLogpixels = read_logpixels_reg();
    WINE_TRACE("%lu\n", dwLogpixels);

    SetDlgItemInt(hDlg, IDC_RES_DPIEDIT, dwLogpixels, FALSE);

    updating_ui = FALSE;
}

static int get_trackbar_pos( UINT dpi )
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(dpi_values) - 1; i++)
        if ((dpi_values[i] + dpi_values[i + 1]) / 2 >= dpi) break;
    return i;
}

static void init_trackbar(HWND hDlg)
{
    HWND hTrackBar = GetDlgItem(hDlg, IDC_RES_TRACKBAR);
    DWORD dwLogpixels;

    updating_ui = TRUE;

    dwLogpixels = read_logpixels_reg();

    SendMessageW(hTrackBar, TBM_SETRANGE, TRUE, MAKELONG(0, ARRAY_SIZE(dpi_values)-1));
    SendMessageW(hTrackBar, TBM_SETPAGESIZE, 0, 1);
    SendMessageW(hTrackBar, TBM_SETPOS, TRUE, get_trackbar_pos(dwLogpixels));

    updating_ui = FALSE;
}

static void update_dpi_trackbar_from_edit(HWND hDlg, BOOL fix)
{
    DWORD dpi;

    updating_ui = TRUE;

    dpi = GetDlgItemInt(hDlg, IDC_RES_DPIEDIT, NULL, FALSE);

    if (fix)
    {
        DWORD fixed_dpi = dpi;

        if (dpi < MINDPI) fixed_dpi = MINDPI;
        if (dpi > MAXDPI) fixed_dpi = MAXDPI;

        if (fixed_dpi != dpi)
        {
            dpi = fixed_dpi;
            SetDlgItemInt(hDlg, IDC_RES_DPIEDIT, dpi, FALSE);
        }
    }

    if (dpi >= MINDPI && dpi <= MAXDPI)
    {
        SendDlgItemMessageW(hDlg, IDC_RES_TRACKBAR, TBM_SETPOS, TRUE, get_trackbar_pos(dpi));
        set_reg_key_dword(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"LogPixels", dpi);
    }

    updating_ui = FALSE;
}

static void update_font_preview(HWND hDlg)
{
    DWORD dpi;

    updating_ui = TRUE;

    dpi = GetDlgItemInt(hDlg, IDC_RES_DPIEDIT, NULL, FALSE);

    if (dpi >= MINDPI && dpi <= MAXDPI)
    {
        LOGFONTW lf;
        HFONT hfont;

        hfont = (HFONT)SendDlgItemMessageW(hDlg, IDC_RES_FONT_PREVIEW, WM_GETFONT, 0, 0);

        GetObjectW(hfont, sizeof(lf), &lf);

        if (wcscmp(lf.lfFaceName, L"Tahoma") != 0)
            wcscpy(lf.lfFaceName, L"Tahoma");
        else
            DeleteObject(hfont);
        lf.lfHeight = MulDiv(-10, dpi, 72);
        hfont = CreateFontIndirectW(&lf);
        SendDlgItemMessageW(hDlg, IDC_RES_FONT_PREVIEW, WM_SETFONT, (WPARAM)hfont, 1);
    }

    updating_ui = FALSE;
}

INT_PTR CALLBACK
GraphDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	case WM_INITDIALOG:
	    init_dpi_editbox(hDlg);
	    init_trackbar(hDlg);
            update_font_preview(hDlg);
	    break;

        case WM_SHOWWINDOW:
            set_window_title(hDlg);
            break;

        case WM_TIMER:
            if (wParam == IDT_DPIEDIT)
            {
                KillTimer(hDlg, IDT_DPIEDIT);
                update_dpi_trackbar_from_edit(hDlg, TRUE);
                update_font_preview(hDlg);
            }
            break;
            
	case WM_COMMAND:
	    switch(HIWORD(wParam)) {
		case EN_CHANGE: {
		    if (updating_ui) break;
		    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    if ( ((LOWORD(wParam) == IDC_DESKTOP_WIDTH) || (LOWORD(wParam) == IDC_DESKTOP_HEIGHT)) && !updating_ui )
			set_from_desktop_edits(hDlg);
                    else if (LOWORD(wParam) == IDC_RES_DPIEDIT)
                    {
                        update_dpi_trackbar_from_edit(hDlg, FALSE);
                        update_font_preview(hDlg);
                        SetTimer(hDlg, IDT_DPIEDIT, 1500, NULL);
                    }
		    break;
		}
		case BN_CLICKED: {
		    if (updating_ui) break;
		    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    switch(LOWORD(wParam)) {
			case IDC_ENABLE_DESKTOP: on_enable_desktop_clicked(hDlg); break;
                        case IDC_ENABLE_MANAGED: on_enable_managed_clicked(hDlg); break;
                        case IDC_ENABLE_DECORATED: on_enable_decorated_clicked(hDlg); break;
			case IDC_FULLSCREEN_GRAB:  on_fullscreen_grab_clicked(hDlg); break;
		    }
		    break;
		}
		case CBN_SELCHANGE: {
		    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    break;
		}
		    
		default:
		    break;
	    }
	    break;
	
	
	case WM_NOTIFY:
	    switch (((LPNMHDR)lParam)->code) {
		case PSN_KILLACTIVE: {
		    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, FALSE);
		    break;
		}
		case PSN_APPLY: {
                    apply();
		    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
		    break;
		}
		case PSN_SETACTIVE: {
		    init_dialog (hDlg);
		    break;
		}
	    }
	    break;

	case WM_HSCROLL:
	    switch (wParam) {
		default: {
		    int i = SendMessageW(GetDlgItem(hDlg, IDC_RES_TRACKBAR), TBM_GETPOS, 0, 0);
		    SetDlgItemInt(hDlg, IDC_RES_DPIEDIT, dpi_values[i], TRUE);
		    update_font_preview(hDlg);
		    set_reg_key_dword(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"LogPixels", dpi_values[i]);
		    break;
		}
	    }
	    break;

	default:
	    break;
    }
    return FALSE;
}
