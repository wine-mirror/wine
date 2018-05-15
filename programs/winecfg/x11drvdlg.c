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
#include <stdio.h>

#include <windows.h>
#include <wine/unicode.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define RES_MAXLEN 5 /* max number of digits in a screen dimension. 5 digits should be plenty */
#define MINDPI 96
#define MAXDPI 480
#define DEFDPI 96

#define IDT_DPIEDIT 0x1234

static const WCHAR logpixels_reg[] = {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\','D','e','s','k','t','o','p','\0'};
static const WCHAR def_logpixels_reg[] = {'S','o','f','t','w','a','r','e','\\','F','o','n','t','s','\0'};
static const WCHAR logpixels[] = {'L','o','g','P','i','x','e','l','s',0};

static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
static const WCHAR explorerW[] = {'E','x','p','l','o','r','e','r',0};
static const WCHAR explorer_desktopsW[] = {'E','x','p','l','o','r','e','r','\\',
                                           'D','e','s','k','t','o','p','s',0};

static const UINT dpi_values[] = { 96, 120, 144, 168, 192, 216, 240, 288, 336, 384, 432, 480 };

static BOOL updating_ui;

/* convert the x11 desktop key to the new explorer config */
static void convert_x11_desktop_key(void)
{
    char *buf;

    if (!(buf = get_reg_key(config_key, "X11 Driver", "Desktop", NULL))) return;
    set_reg_key(config_key, "Explorer\\Desktops", "Default", buf);
    set_reg_key(config_key, "Explorer", "Desktop", "Default");
    set_reg_key(config_key, "X11 Driver", "Desktop", NULL);
    HeapFree(GetProcessHeap(), 0, buf);
}

static void update_gui_for_desktop_mode(HWND dialog)
{
    WCHAR *buf, *bufindex;
    const WCHAR *desktop_name = current_app ? current_app : defaultW;

    WINE_TRACE("\n");
    updating_ui = TRUE;

    buf = get_reg_keyW(config_key, explorer_desktopsW, desktop_name, NULL);
    if (buf && (bufindex = strchrW(buf, 'x')))
    {
        *bufindex++ = 0;

        SetDlgItemTextW(dialog, IDC_DESKTOP_WIDTH, buf);
        SetDlgItemTextW(dialog, IDC_DESKTOP_HEIGHT, bufindex);
    } else {
        SetDlgItemTextA(dialog, IDC_DESKTOP_WIDTH, "800");
        SetDlgItemTextA(dialog, IDC_DESKTOP_HEIGHT, "600");
    }
    HeapFree(GetProcessHeap(), 0, buf);

    /* do we have desktop mode enabled? */
    if (reg_key_exists(config_key, keypath("Explorer"), "Desktop"))
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

static void init_dialog(HWND dialog)
{
    char* buf;

    convert_x11_desktop_key();
    update_gui_for_desktop_mode(dialog);

    updating_ui = TRUE;

    SendDlgItemMessageW(dialog, IDC_DESKTOP_WIDTH, EM_LIMITTEXT, RES_MAXLEN, 0);
    SendDlgItemMessageW(dialog, IDC_DESKTOP_HEIGHT, EM_LIMITTEXT, RES_MAXLEN, 0);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "GrabFullscreen", "N");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_FULLSCREEN_GRAB, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_FULLSCREEN_GRAB, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "Managed", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "Decorated", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_DECORATED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_DECORATED, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    updating_ui = FALSE;
}

static void set_from_desktop_edits(HWND dialog)
{
    static const WCHAR x[] = {'x',0};
    static const WCHAR def_width[]  = {'8','0','0',0};
    static const WCHAR def_height[] = {'6','0','0',0};
    static const WCHAR min_width[]  = {'6','4','0',0};
    static const WCHAR min_height[] = {'4','8','0',0};
    WCHAR *width, *height, *new;
    const WCHAR *desktop_name = current_app ? current_app : defaultW;

    if (updating_ui) return;
    
    WINE_TRACE("\n");

    width = get_textW(dialog, IDC_DESKTOP_WIDTH);
    height = get_textW(dialog, IDC_DESKTOP_HEIGHT);

    if (!width || !width[0]) {
        HeapFree(GetProcessHeap(), 0, width);
        width = strdupW(def_width);
    }
    else if (atoiW(width) < atoiW(min_width))
    {
        HeapFree(GetProcessHeap(), 0, width);
        width = strdupW(min_width);
    }
    if (!height || !height[0]) {
        HeapFree(GetProcessHeap(), 0, height);
        height = strdupW(def_height);
    }
    else if (atoiW(height) < atoiW(min_height))
    {
        HeapFree(GetProcessHeap(), 0, height);
        height = strdupW(min_height);
    }

    new = HeapAlloc(GetProcessHeap(), 0, (strlenW(width) + strlenW(height) + 2) * sizeof(WCHAR));
    strcpyW( new, width );
    strcatW( new, x );
    strcatW( new, height );
    set_reg_keyW(config_key, explorer_desktopsW, desktop_name, new);
    set_reg_keyW(config_key, keypathW(explorerW), desktopW, desktop_name);

    HeapFree(GetProcessHeap(), 0, width);
    HeapFree(GetProcessHeap(), 0, height);
    HeapFree(GetProcessHeap(), 0, new);
}

static void on_enable_desktop_clicked(HWND dialog) {
    WINE_TRACE("\n");
    
    if (IsDlgButtonChecked(dialog, IDC_ENABLE_DESKTOP) == BST_CHECKED) {
        set_from_desktop_edits(dialog);
    } else {
        set_reg_key(config_key, keypath("Explorer"), "Desktop", NULL);
    }
    
    update_gui_for_desktop_mode(dialog);
}

static void on_enable_managed_clicked(HWND dialog) {
    WINE_TRACE("\n");
    
    if (IsDlgButtonChecked(dialog, IDC_ENABLE_MANAGED) == BST_CHECKED) {
        set_reg_key(config_key, keypath("X11 Driver"), "Managed", "Y");
    } else {
        set_reg_key(config_key, keypath("X11 Driver"), "Managed", "N");
    }
}

static void on_enable_decorated_clicked(HWND dialog) {
    WINE_TRACE("\n");

    if (IsDlgButtonChecked(dialog, IDC_ENABLE_DECORATED) == BST_CHECKED) {
        set_reg_key(config_key, keypath("X11 Driver"), "Decorated", "Y");
    } else {
        set_reg_key(config_key, keypath("X11 Driver"), "Decorated", "N");
    }
}

static void on_fullscreen_grab_clicked(HWND dialog)
{
    if (IsDlgButtonChecked(dialog, IDC_FULLSCREEN_GRAB) == BST_CHECKED)
        set_reg_key(config_key, keypath("X11 Driver"), "GrabFullscreen", "Y");
    else
        set_reg_key(config_key, keypath("X11 Driver"), "GrabFullscreen", "N");
}

static INT read_logpixels_reg(void)
{
    DWORD dwLogPixels;
    WCHAR *buf = get_reg_keyW(HKEY_CURRENT_USER, logpixels_reg, logpixels, NULL);
    if (!buf) buf = get_reg_keyW(HKEY_CURRENT_CONFIG, def_logpixels_reg, logpixels, NULL);
    dwLogPixels = buf ? *buf : DEFDPI;
    HeapFree(GetProcessHeap(), 0, buf);
    return dwLogPixels;
}

static void init_dpi_editbox(HWND hDlg)
{
    DWORD dwLogpixels;

    updating_ui = TRUE;

    dwLogpixels = read_logpixels_reg();
    WINE_TRACE("%u\n", dwLogpixels);

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
        set_reg_key_dwordW(HKEY_CURRENT_USER, logpixels_reg, logpixels, dpi);
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
        static const WCHAR tahomaW[] = {'T','a','h','o','m','a',0};
        LOGFONTW lf;
        HFONT hfont;

        hfont = (HFONT)SendDlgItemMessageW(hDlg, IDC_RES_FONT_PREVIEW, WM_GETFONT, 0, 0);

        GetObjectW(hfont, sizeof(lf), &lf);

        if (strcmpW(lf.lfFaceName, tahomaW) != 0)
            strcpyW(lf.lfFaceName, tahomaW);
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
		    set_reg_key_dwordW(HKEY_CURRENT_USER, logpixels_reg, logpixels, dpi_values[i]);
		    break;
		}
	    }
	    break;

	default:
	    break;
    }
    return FALSE;
}
