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
    char *i;

    updatingUI = TRUE;
    
    updateGUIForDesktopMode(hDlg);
    
    /* desktop size */
    buf = getConfigValue("x11drv", "Desktop", "640x480");
    i = strchr(buf, 'x');
    *i = '\0';
    i++;
    SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_WIDTH), buf);
    SetWindowText(GetDlgItem(hDlg, IDC_DESKTOP_HEIGHT), i);
    free(buf);

    SendDlgItemMessage(hDlg, IDC_DESKTOP_WIDTH, EM_LIMITTEXT, RES_MAXLEN, 0);
    SendDlgItemMessage(hDlg, IDC_DESKTOP_HEIGHT, EM_LIMITTEXT, RES_MAXLEN, 0);

    buf = getConfigValue("x11drv", "AllocSysColors", "100");
    SetWindowText(GetDlgItem(hDlg, IDC_SYSCOLORS), buf);
    free(buf);

    buf = getConfigValue("x11drv", "Managed", "Y");
    if (IS_OPTION_TRUE(*buf))
	CheckDlgButton(hDlg, IDC_ENABLE_MANAGED, BST_CHECKED);
    else
	CheckDlgButton(hDlg, IDC_ENABLE_MANAGED, BST_UNCHECKED);
    
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

void onSysColorsChange(HWND hDlg) {
    char *newvalue = getDialogItemText(hDlg, IDC_SYSCOLORS);
    
    WINE_TRACE("\n");
    if (updatingUI) return;
    if (!newvalue) return;

    addTransaction("x11drv", "AllocSystemColors", ACTION_SET, newvalue);
    free(newvalue);
}

void onEnableManagedClicked(HWND hDlg) {
    if (IsDlgButtonChecked(hDlg, IDC_ENABLE_MANAGED) == BST_CHECKED)
	addTransaction("x11drv", "Managed", ACTION_SET, "Y");
    else
	addTransaction("x11drv", "Managed", ACTION_SET, "N");
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
		    if (LOWORD(wParam) == IDC_SYSCOLORS) onSysColorsChange(hDlg);
		    break;
		}
		case BN_CLICKED: {
		    WINE_TRACE("%d\n", LOWORD(wParam));
		    switch(LOWORD(wParam)) {
			case IDC_ENABLE_DESKTOP: onEnableDesktopClicked(hDlg); break;
			case IDC_ENABLE_MANAGED: onEnableManagedClicked(hDlg); break;
		    };
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
