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

#include <winreg.h>
#include <wine/debug.h>
#include <stdlib.h>
#include <stdio.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

/* pokes the win32 api to setup the dialog from the config struct */
void initX11DrvDlg (HWND hDlg)
{
    char szBuf[20];

    /* system colors */
    sprintf (szBuf, "%d", config.sX11Drv.nSysColors);
    SendDlgItemMessage (hDlg, IDC_SYSCOLORS, WM_SETTEXT, 0, (LPARAM) szBuf);

    /* private color map */
    if (config.sX11Drv.nPrivateMap)
	SendDlgItemMessage( hDlg, IDC_PRIVATEMAP, BM_SETCHECK, BST_CHECKED, 0);

    /* perfect graphics */
    if (config.sX11Drv.nPerfect)
	SendDlgItemMessage( hDlg, IDC_PERFECTGRAPH, BM_SETCHECK, BST_CHECKED, 0);

    /* desktop size */
    sprintf (szBuf, "%d", config.sX11Drv.nDesktopSizeX);
    SendDlgItemMessage (hDlg, IDC_DESKTOP_WIDTH, WM_SETTEXT, 0, (LPARAM) szBuf);
    sprintf (szBuf, "%d", config.sX11Drv.nDesktopSizeY);
    SendDlgItemMessage (hDlg, IDC_DESKTOP_HEIGHT, WM_SETTEXT, 0, (LPARAM) szBuf);

    if (config.sX11Drv.nDGA) SendDlgItemMessage( hDlg, IDC_XDGA, BM_SETCHECK, BST_CHECKED, 0);
    if (config.sX11Drv.nXShm) SendDlgItemMessage( hDlg, IDC_XSHM, BM_SETCHECK, BST_CHECKED, 0);
}

void
saveX11DrvDlgSettings (HWND hDlg)
{
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
		    break;
		}

		default:
		    break;
	    }
	    break;

	case WM_NOTIFY:
	    switch (((LPNMHDR)lParam)->code) {
		case PSN_KILLACTIVE: {
		    /* validate user info.  Lets just assume everything is okay for now */
		    SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
		    break;
		}
		case PSN_APPLY: {
		    /* should probably check everything is really all rosy :) */
		    saveX11DrvDlgSettings (hDlg);
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
