/*
 * X11DRV configuration code
 *
 * Copyright 2003 Mark Westcott
 * Copyright 2003 Mike Hearn
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

#define RES_MAXLEN 5 /* the maximum number of characters in a screen dimension. 5 digits should be plenty, what kind of crazy person runs their screen >10,000 pixels across? */

int updatingUI;

void updateGUIForDesktopMode(HWND hDlg) {
    WINE_TRACE("\n");

    updatingUI = TRUE;
    
    /* do we have desktop mode enabled? */
    if (doesConfigValueExist("x11drv", "Desktop") == S_OK) {
	CheckDlgButton(hDlg, IDC_ENABLE_DESKTOP, BST_CHECKED);
	/* enable the controls */
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), 1);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), 1);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_SIZE), 1);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_BY), 1);
	SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), "640");
	SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), "480");	
    }
    else {
	CheckDlgButton(hDlg, IDC_ENABLE_DESKTOP, BST_UNCHECKED);
	/* disable the controls */
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), 0);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), 0);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_SIZE), 0);
	EnableWindow(GetDlgItem(hDlg, IDC_DESKTOP_BY), 0);

	SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), "");
	SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), "");
    }

    updatingUI = FALSE;
}

/* pokes the win32 api to setup the dialog from the config struct */
void initX11DrvDlg (HWND hDlg)
{
    char *buf;
    char *bufindex;

    updatingUI = TRUE;
    
    updateGUIForDesktopMode(hDlg);
    
    /* desktop size */
    buf = getConfigValue("x11drv", "Desktop", "640x480");
    bufindex = strchr(buf, 'x');
    *bufindex = '\0';
    bufindex++;
    SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), buf);
    SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), bufindex);
    free(buf);
    
    SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_ADDSTRING, 0, (LPARAM) "8 bit");
    SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_ADDSTRING, 0, (LPARAM) "16 bit");
    SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_ADDSTRING, 0, (LPARAM) "24 bit");
    SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_ADDSTRING, 0, (LPARAM) "32 bit"); /* is this valid? */

    buf = getConfigValue("x11drv", "ScreenDepth", "24");
    if (strcmp(buf, "8") == 0)
	SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_SETCURSEL, 0, 0);
    else if (strcmp(buf, "16") == 0)
	SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_SETCURSEL, 1, 0);
    else if (strcmp(buf, "24") == 0)
	SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_SETCURSEL, 2, 0);
    else if (strcmp(buf, "32") == 0)
	SendDlgItemMessage(hDlg, IDC_SCREEN_DEPTH, CB_SETCURSEL, 3, 0);
    else
	WINE_ERR("Invalid screen depth read from registry (%s)\n", buf);
    free(buf);

    SendDlgItemMessage(hDlg, IDC_DESKTOP_WIDTH, EM_LIMITTEXT, RES_MAXLEN, 0);
    SendDlgItemMessage(hDlg, IDC_DESKTOP_HEIGHT, EM_LIMITTEXT, RES_MAXLEN, 0);

    buf = getConfigValue("x11drv", "DXGrab", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(hDlg, IDC_DX_MOUSE_GRAB, BST_CHECKED);
    else
	CheckDlgButton(hDlg, IDC_DX_MOUSE_GRAB, BST_UNCHECKED);
    free(buf);

    buf = getConfigValue("x11drv", "DesktopDoubleBuffered", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(hDlg, IDC_DOUBLE_BUFFER, BST_CHECKED);
    else
	CheckDlgButton(hDlg, IDC_DOUBLE_BUFFER, BST_UNCHECKED);
    free(buf);
    
    buf = getConfigValue("x11drv", "UseTakeFocus", "N");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(hDlg, IDC_USE_TAKE_FOCUS, BST_CHECKED);
    else
	CheckDlgButton(hDlg, IDC_USE_TAKE_FOCUS, BST_UNCHECKED);
    free(buf);
    
    updatingUI = FALSE;
}



void setFromDesktopSizeEdits(HWND hDlg) {
    char *width = malloc(RES_MAXLEN+1);
    char *height = malloc(RES_MAXLEN+1);
    char *newStr = malloc((RES_MAXLEN*2) + 2);

    if (updatingUI) return;
    
    WINE_TRACE("\n");
    
    GetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), width, RES_MAXLEN+1);
    GetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), height, RES_MAXLEN+1);

    if (strcmp(width, "") == 0) strcpy(width, "640");
    if (strcmp(height, "") == 0) strcpy(height, "480");
    
    sprintf(newStr, "%sx%s", width, height);
    addTransaction("x11drv", "Desktop", ACTION_SET, newStr);

    free(width);
    free(height);
    free(newStr);
}

void onEnableDesktopClicked(HWND hDlg) {
    WINE_TRACE("\n");
    if (IsDlgButtonChecked(hDlg, IDC_ENABLE_DESKTOP) == BST_CHECKED) {
	/* it was just unchecked, so read the values of the edit boxes, set the config value */
	setFromDesktopSizeEdits(hDlg);
    } else {
	/* it was just checked, so remove the config values */
	addTransaction("x11drv", "Desktop", ACTION_REMOVE, NULL);
    }
    updateGUIForDesktopMode(hDlg);
}

void onScreenDepthChanged(HWND hDlg) {
    char *newvalue = getDialogItemText(hDlg, IDC_SCREEN_DEPTH);
    char *spaceIndex = strchr(newvalue, ' ');
    
    WINE_TRACE("newvalue=%s\n", newvalue);
    if (updatingUI) return;

    *spaceIndex = '\0';
    addTransaction("x11drv", "ScreenDepth", ACTION_SET, newvalue);
    free(newvalue);
}

void onDXMouseGrabClicked(HWND hDlg) {
    if (IsDlgButtonChecked(hDlg, IDC_DX_MOUSE_GRAB) == BST_CHECKED)
	addTransaction("x11drv", "DXGrab", ACTION_SET, "Y");
    else
	addTransaction("x11drv", "DXGrab", ACTION_SET, "N");
}


void onDoubleBufferClicked(HWND hDlg) {
    if (IsDlgButtonChecked(hDlg, IDC_DOUBLE_BUFFER) == BST_CHECKED)
	addTransaction("x11drv", "DesktopDoubleBuffered", ACTION_SET, "Y");
    else
	addTransaction("x11drv", "DesktopDoubleBuffered", ACTION_SET, "N");
}

void onUseTakeFocusClicked(HWND hDlg) {
    if (IsDlgButtonChecked(hDlg, IDC_USE_TAKE_FOCUS) == BST_CHECKED)
	addTransaction("x11drv", "UseTakeFocus", ACTION_SET, "Y");
    else
	addTransaction("x11drv", "UseTakeFocus", ACTION_SET, "N");
}


INT_PTR CALLBACK
X11DrvDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	case WM_INITDIALOG:
	    break;
	    
	case WM_COMMAND:
	    switch(HIWORD(wParam)) {
		case EN_CHANGE: {
		    SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
		    if ( (LOWORD(wParam) == IDC_DESKTOP_WIDTH) || (LOWORD(wParam) == IDC_DESKTOP_HEIGHT) ) setFromDesktopSizeEdits(hDlg);
		    break;
		}
		case BN_CLICKED: {
		    WINE_TRACE("%d\n", LOWORD(wParam));
		    switch(LOWORD(wParam)) {
			case IDC_ENABLE_DESKTOP: onEnableDesktopClicked(hDlg); break;
			case IDC_DX_MOUSE_GRAB:  onDXMouseGrabClicked(hDlg); break;
			case IDC_USE_TAKE_FOCUS: onUseTakeFocusClicked(hDlg); break;
			case IDC_DOUBLE_BUFFER:  onDoubleBufferClicked(hDlg); break;
		    };
		    break;
		}
		case CBN_SELCHANGE: {
		    if (LOWORD(wParam) == IDC_SCREEN_DEPTH) onScreenDepthChanged(hDlg);
		    break;
		}
		    
		default:
		    break;
	    }
	    break;
	
	
	case WM_NOTIFY:
	    switch (((LPNMHDR)lParam)->code) {
		case PSN_KILLACTIVE: {
		    SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
		    break;
		}
		case PSN_APPLY: {
		    SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
		    break;
		}
		case PSN_SETACTIVE: {
		    initX11DrvDlg (hDlg);
		    break;
		}
	    }
	    break;

	default:
	    break;
    }
    return FALSE;
}
