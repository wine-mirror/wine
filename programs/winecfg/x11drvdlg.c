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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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


static const char* D3D_VS_Modes[] = {
  "hardware",
  "none",
  "emulation",
  NULL
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
                WINE_TRACE("Desktop registry entry is malformed");
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

    buf = get_reg_key(config_key, keypath("X11 Driver"), "DesktopDoubleBuffered", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_DOUBLE_BUFFER, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_DOUBLE_BUFFER, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("X11 Driver"), "Managed", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_CHECKED);
    else
	CheckDlgButton(dialog, IDC_ENABLE_MANAGED, BST_UNCHECKED);
    HeapFree(GetProcessHeap(), 0, buf);

    SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_RESETCONTENT, 0, 0);
    for (it = 0; NULL != D3D_VS_Modes[it]; ++it) {
      SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_ADDSTRING, 0, (LPARAM) D3D_VS_Modes[it]);
    }  
    buf = get_reg_key(config_key, keypath("Direct3D"), "VertexShaderMode", "hardware"); 
    for (it = 0; NULL != D3D_VS_Modes[it]; ++it) {
      if (strcmp(buf, D3D_VS_Modes[it]) == 0) {
	SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_SETCURSEL, it, 0);
	break ;
      }
    }
    if (NULL == D3D_VS_Modes[it]) {
      WINE_ERR("Invalid Direct3D VertexShader Mode read from registry (%s)\n", buf);
    }
    HeapFree(GetProcessHeap(), 0, buf);

    buf = get_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "disabled");
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


static void on_double_buffer_clicked(HWND dialog) {
    if (IsDlgButtonChecked(dialog, IDC_DOUBLE_BUFFER) == BST_CHECKED)
        set_reg_key(config_key, keypath("X11 Driver"), "DesktopDoubleBuffered", "Y");
    else
        set_reg_key(config_key, keypath("X11 Driver"), "DesktopDoubleBuffered", "N");
}

static void on_d3d_vshader_mode_changed(HWND dialog) {
  int selected_mode = SendDlgItemMessage(dialog, IDC_D3D_VSHADER_MODE, CB_GETCURSEL, 0, 0);  
  set_reg_key(config_key, keypath("Direct3D"), "VertexShaderMode", D3D_VS_Modes[selected_mode]); 
}

static void on_d3d_pshader_mode_clicked(HWND dialog) {
    if (IsDlgButtonChecked(dialog, IDC_D3D_PSHADER_MODE) == BST_CHECKED)
        set_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "enabled");
    else
        set_reg_key(config_key, keypath("Direct3D"), "PixelShaderMode", "disabled");
}

INT_PTR CALLBACK
GraphDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	case WM_INITDIALOG:
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
                        case IDC_DOUBLE_BUFFER:  on_double_buffer_clicked(hDlg); break;
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

	default:
	    break;
    }
    return FALSE;
}
