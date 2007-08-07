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
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define RES_MAXLEN 5 /* the maximum number of characters in a screen dimension. 5 digits should be plenty, what kind of crazy person runs their screen >10,000 pixels across? */
#define MINDPI 96
#define MAXDPI 120
#define DEFDPI 96

static const char logpixels_reg[] = "System\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts";

static struct SHADERMODE
{
  UINT displayStrID;
  const char* settingStr;
} const D3D_VS_Modes[] = {
  {IDS_SHADER_MODE_HARDWARE,  "hardware"},
  {IDS_SHADER_MODE_NONE,      "none"},
  {0, 0}
};


int updating_ui;

static void update_gui_for_desktop_mode(HWND dialog) {
    int desktopenabled = FALSE;

    WINE_TRACE("\n");
    updating_ui = TRUE;

    if (current_app)
    {
        disable(IDC_ENABLE_DESKTOP);
        disable(IDC_DESKTOP_WIDTH);
        disable(IDC_DESKTOP_HEIGHT);
        disable(IDC_DESKTOP_SIZE);
        disable(IDC_DESKTOP_BY);
        return;
    }
    enable(IDC_ENABLE_DESKTOP);

    /* do we have desktop mode enabled? */
    if (reg_key_exists(config_key, keypath("X11 Driver"), "Desktop"))
    {
        char* buf, *bufindex;
	CheckDlgButton(dialog, IDC_ENABLE_DESKTOP, BST_CHECKED);

        buf = get_reg_key(config_key, keypath("X11 Driver"), "Desktop", "640x480");
        /* note: this test must match the one in x11drv */
        if( buf[0] != 'n' &&  buf[0] != 'N' &&  buf[0] != 'F' &&  buf[0] != 'f'
                &&  buf[0] != '0') {
            desktopenabled = TRUE;
            enable(IDC_DESKTOP_WIDTH);
            enable(IDC_DESKTOP_HEIGHT);
            enable(IDC_DESKTOP_SIZE);
            enable(IDC_DESKTOP_BY);

            bufindex = strchr(buf, 'x');
            if (bufindex) {
                *bufindex = 0;
                ++bufindex;
                SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_WIDTH), buf);
                SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_HEIGHT), bufindex);
            } else {
                WINE_TRACE("Desktop registry entry is malformed\n");
                SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_WIDTH), "640");
                SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_HEIGHT), "480");
            }
        }
        HeapFree(GetProcessHeap(), 0, buf);
    }
    if (!desktopenabled)
    {
	CheckDlgButton(dialog, IDC_ENABLE_DESKTOP, BST_UNCHECKED);
	
	disable(IDC_DESKTOP_WIDTH);
	disable(IDC_DESKTOP_HEIGHT);
	disable(IDC_DESKTOP_SIZE);
	disable(IDC_DESKTOP_BY);

	SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_WIDTH), "");
	SetWindowText(GetDlgItem(dialog, IDC_DESKTOP_HEIGHT), "");
    }

    updating_ui = FALSE;
}

static void init_dialog(HWND dialog)
{
    unsigned int it;
    char* buf;

    update_gui_for_desktop_mode(dialog);

    updating_ui = TRUE;
    
    SendDlgItemMessage(dialog, IDC_DESKTOP_WIDTH, EM_LIMITTEXT, RES_MAXLEN, 0);
    SendDlgItemMessage(dialog, IDC_DESKTOP_HEIGHT, EM_LIMITTEXT, RES_MAXLEN, 0);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "DXGrab", "N");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_DX_MOUSE_GRAB, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_DX_MOUSE_GRAB, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "Managed", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_RESETCONTENT, 0, 0);
    for (it = 0; 0 != D3D_VS_Modes[it].displayStrID; ++it) {
      SendDlgItemMessageW (dialog, IDC_D3D_VSHADER_MODE, CB_ADDSTRING, 0,
          (LPARAM)load_string (D3D_VS_Modes[it].displayStrID));
    }  
    buf = get_reg_key(config_key, keypath("Direct3D"), "VertexShaderMode", "hardware"); 
    for (it = 0; NULL != D3D_VS_Modes[it].settingStr; ++it) {
      if (strcmp(buf, D3D_VS_Modes[it].settingStr) == 0) {
	SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_SETCURSEL, it, 0);
	break ;
      }
    }
    if (NULL == D3D_VS_Modes[it].settingStr) {
      WINE_ERR("Invalid Direct3D VertexShader Mode read from registry (%s)\n", buf);
    }
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "enabled");
    if (!strcmp(buf, "enabled"))
      CheckDlgButton(dialog, IDC_D3D_PSHADER_MODE, BST_CHECKED);
    else
      CheckDlgButton(dialog, IDC_D3D_PSHADER_MODE, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    updating_ui = FALSE;
}

static void set_from_desktop_edits(HWND dialog) {
    char *width, *height, *new;

    if (updating_ui) return;
    
    WINE_TRACE("\n");

    width = get_text(dialog, IDC_DESKTOP_WIDTH);
    height = get_text(dialog, IDC_DESKTOP_HEIGHT);

    if (width == NULL || strcmp(width, "") == 0) {
        HeapFree(GetProcessHeap(), 0, width);
        width = strdupA("640");
    }
    
    if (height == NULL || strcmp(height, "") == 0) {
        HeapFree(GetProcessHeap(), 0, height);
        height = strdupA("480");
    }

    new = HeapAlloc(GetProcessHeap(), 0, strlen(width) + strlen(height) + 2 /* x + terminator */);
    sprintf(new, "%sx%s", width, height);
    set_reg_key(config_key, keypath("X11 Driver"), "Desktop", new);
    
    HeapFree(GetProcessHeap(), 0, width);
    HeapFree(GetProcessHeap(), 0, height);
    HeapFree(GetProcessHeap(), 0, new);
}

static void on_enable_desktop_clicked(HWND dialog) {
    WINE_TRACE("\n");
    
    if (IsDlgButtonChecked(dialog, IDC_ENABLE_DESKTOP) == BST_CHECKED) {
        set_from_desktop_edits(dialog);
    } else {
        set_reg_key(config_key, keypath("X11 Driver"), "Desktop", NULL);
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

static void on_dx_mouse_grab_clicked(HWND dialog) {
    if (IsDlgButtonChecked(dialog, IDC_DX_MOUSE_GRAB) == BST_CHECKED) 
        set_reg_key(config_key, keypath("X11 Driver"), "DXGrab", "Y");
    else
        set_reg_key(config_key, keypath("X11 Driver"), "DXGrab", "N");
}

static void on_d3d_vshader_mode_changed(HWND dialog) {
  int selected_mode = SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_GETCURSEL, 0, 0);  
  set_reg_key(config_key, keypath("Direct3D"), "VertexShaderMode",
      D3D_VS_Modes[selected_mode].settingStr); 
}

static void on_d3d_pshader_mode_clicked(HWND dialog) {
    if (IsDlgButtonChecked(dialog, IDC_D3D_PSHADER_MODE) == BST_CHECKED)
        set_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "enabled");
    else
        set_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "disabled");
}
static INT read_logpixels_reg(void)
{
    DWORD dwLogPixels;
    char *buf  = get_reg_key(HKEY_LOCAL_MACHINE, logpixels_reg,
                             "LogPixels", NULL);
    dwLogPixels = buf ? *buf : DEFDPI;
    HeapFree(GetProcessHeap(), 0, buf);
    return dwLogPixels;
}

static void init_dpi_editbox(HWND hDlg)
{
    HWND hDpiEditBox = GetDlgItem(hDlg, IDC_RES_DPIEDIT);
    DWORD dwLogpixels;
    char szLogpixels[MAXBUFLEN];

    updating_ui = TRUE;

    dwLogpixels = read_logpixels_reg();
    WINE_TRACE("%d\n", (int) dwLogpixels);

    szLogpixels[0] = 0;
    sprintf(szLogpixels, "%d", dwLogpixels);
    SendMessage(hDpiEditBox, WM_SETTEXT, 0, (LPARAM) szLogpixels);

    updating_ui = FALSE;
}

static void init_trackbar(HWND hDlg)
{
    HWND hTrackBar = GetDlgItem(hDlg, IDC_RES_TRACKBAR);
    DWORD dwLogpixels;

    updating_ui = TRUE;

    dwLogpixels = read_logpixels_reg();

    SendMessageW(hTrackBar, TBM_SETRANGE, TRUE, MAKELONG(MINDPI, MAXDPI));
    SendMessageW(hTrackBar, TBM_SETPOS, TRUE, dwLogpixels);

    updating_ui = FALSE;
}

INT_PTR CALLBACK
GraphDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	case WM_INITDIALOG:
	    init_dpi_editbox(hDlg);
	    init_trackbar(hDlg);
	    break;

        case WM_SHOWWINDOW:
            set_window_title(hDlg);
            break;
            
	case WM_COMMAND:
	    switch(HIWORD(wParam)) {
		case EN_CHANGE: {
		    if (updating_ui) break;
		    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    if ( ((LOWORD(wParam) == IDC_DESKTOP_WIDTH) || (LOWORD(wParam) == IDC_DESKTOP_HEIGHT)) && !updating_ui )
			set_from_desktop_edits(hDlg);
		    break;
		}
		case BN_CLICKED: {
		    if (updating_ui) break;
		    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    switch(LOWORD(wParam)) {
			case IDC_ENABLE_DESKTOP: on_enable_desktop_clicked(hDlg); break;
                        case IDC_ENABLE_MANAGED: on_enable_managed_clicked(hDlg); break;
			case IDC_DX_MOUSE_GRAB:  on_dx_mouse_grab_clicked(hDlg); break;
		        case IDC_D3D_PSHADER_MODE: on_d3d_pshader_mode_clicked(hDlg); break;
		    }
		    break;
		}
		case CBN_SELCHANGE: {
		    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    switch (LOWORD(wParam)) {
		    case IDC_D3D_VSHADER_MODE: on_d3d_vshader_mode_changed(hDlg); break;
		    }
		    break;
		}
		    
		default:
		    break;
	    }
	    break;
	
	
	case WM_NOTIFY:
	    switch (((LPNMHDR)lParam)->code) {
		case PSN_KILLACTIVE: {
		    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
		    break;
		}
		case PSN_APPLY: {
                    apply();
		    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
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
		    char buf[MAXBUFLEN];
		    int i = SendMessageW(GetDlgItem(hDlg, IDC_RES_TRACKBAR), TBM_GETPOS, 0, 0);
		    buf[0] = 0;
		    sprintf(buf, "%d", i);
		    SendMessage(GetDlgItem(hDlg, IDC_RES_DPIEDIT), WM_SETTEXT, 0, (LPARAM) buf);
		    set_reg_key_dword(HKEY_LOCAL_MACHINE, logpixels_reg, "LogPixels", i);
		    break;
		}
	    }
	    break;

	default:
	    break;
    }
    return FALSE;
}
