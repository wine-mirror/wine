/*
 * WineCfg main entry point
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <wine/debug.h>

#include "properties.h"
#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

void CALLBACK
PropSheetCallback (HWND hWnd, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
	/*
	 * hWnd = NULL, lParam == dialog resource
	 */
    case PSCB_PRECREATE:
	break;

    case PSCB_INITIALIZED:
	break;

    default:
	break;
    }
}

void
initGeneralDlg (HWND hDlg)
{
    int i;
    const VERSION_DESC *pVer = NULL;
    char *curWinVer = getConfigValue("Version", "Windows", "win98");
    char *curDOSVer = getConfigValue("Version", "DOS", "6.22");
    char *curWineLook = getConfigValue("Tweak.Layout", "WineLook", "win95");

    if ((pVer = getWinVersions ()))
    {
	for (i = 0; *pVer->szVersion; i++, pVer++)
	{
	    SendDlgItemMessage (hDlg, IDC_WINVER, CB_ADDSTRING,
				0, (LPARAM) pVer->szDescription);
	    if (!strcmp (pVer->szVersion, curWinVer))
		SendDlgItemMessage (hDlg, IDC_WINVER, CB_SETCURSEL,
				    (WPARAM) i, 0);
	}
    }
    if ((pVer = getDOSVersions ()))
    {
	for (i = 0; *pVer->szVersion; i++, pVer++)
	{
	    SendDlgItemMessage (hDlg, IDC_DOSVER, CB_ADDSTRING,
				0, (LPARAM) pVer->szDescription);
	    if (!strcmp (pVer->szVersion, curDOSVer))
		SendDlgItemMessage (hDlg, IDC_DOSVER, CB_SETCURSEL,
				    (WPARAM) i, 0);
	}
    }
    if ((pVer = getWinelook ()))
    {
	for (i = 0; *pVer->szVersion; i++, pVer++)
	{
	    SendDlgItemMessage (hDlg, IDC_WINELOOK, CB_ADDSTRING,
				0, (LPARAM) pVer->szDescription);
	    if (!strcmp (pVer->szVersion, curWineLook))
		SendDlgItemMessage (hDlg, IDC_WINELOOK, CB_SETCURSEL,
				    (WPARAM) i, 0);
	}
    }

    free(curWinVer);
    free(curDOSVer);
    free(curWineLook);
}

INT_PTR CALLBACK
GeneralDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
	
	case WM_INITDIALOG:
	    initGeneralDlg (hDlg);
	    break;

	case WM_COMMAND:
	    switch (LOWORD(wParam)) {
		case IDC_WINVER: if (HIWORD(wParam) == CBN_SELCHANGE) {
		    /* user changed the wine version combobox */
		    int selection = SendDlgItemMessage( hDlg, IDC_WINVER, CB_GETCURSEL, 0, 0);
		    VERSION_DESC *desc = getWinVersions();

		    while (selection > 0) {
			desc++; selection--;
		    }
		    addTransaction("Version", "Windows", ACTION_SET, desc->szVersion);
		}
	        break;
	    }
	    break;
	    
	default:
	    break;
	    
    }
    return FALSE;
}


INT_PTR CALLBACK
DllDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
	break;

    default:
	break;
    }
    return FALSE;
}


INT_PTR CALLBACK
AppDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
	break;

    default:
	break;
    }
    return FALSE;
}

#define NUM_PROPERTY_PAGES 5
INT_PTR
doPropertySheet (HINSTANCE hInstance, HWND hOwner)
{
    PROPSHEETPAGE psp[NUM_PROPERTY_PAGES];
    PROPSHEETHEADER psh;

    /*
     * Fill out the (General) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[0].dwSize = sizeof (PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = hInstance;
    psp[0].u.pszTemplate = MAKEINTRESOURCE (IDD_GENERALCFG);
    psp[0].u2.pszIcon = NULL;
    psp[0].pfnDlgProc = GeneralDlgProc;
    psp[0].pszTitle = "General";
    psp[0].lParam = 0;

    /*
     * Fill out the (Libraries) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[1].dwSize = sizeof (PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = hInstance;
    psp[1].u.pszTemplate = MAKEINTRESOURCE (IDD_DLLCFG);
    psp[1].u2.pszIcon = NULL;
    psp[1].pfnDlgProc = DllDlgProc;
    psp[1].pszTitle = "Libraries";
    psp[1].lParam = 0;

    /*
     * Fill out the (Applications) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[2].dwSize = sizeof (PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = hInstance;
    psp[2].u.pszTemplate = MAKEINTRESOURCE (IDD_APPCFG);
    psp[2].u2.pszIcon = NULL;
    psp[2].pfnDlgProc = AppDlgProc;
    psp[2].pszTitle = "Applications";
    psp[2].lParam = 0;

    /*
     * Fill out the (X11Drv) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[3].dwSize = sizeof (PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = hInstance;
    psp[3].u.pszTemplate = MAKEINTRESOURCE (IDD_X11DRVCFG);
    psp[3].u2.pszIcon = NULL;
    psp[3].pfnDlgProc = X11DrvDlgProc;
    psp[3].pszTitle = "X11 Driver";
    psp[3].lParam = 0;

    psp[4].dwSize = sizeof (PROPSHEETPAGE);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = hInstance;
    psp[4].u.pszTemplate = MAKEINTRESOURCE (IDD_DRIVECFG);
    psp[4].u2.pszIcon = NULL;
    psp[4].pfnDlgProc = DriveDlgProc;
    psp[4].pszTitle = "Drives";
    psp[4].lParam = 0;
    
    /*
     * Fill out the PROPSHEETHEADER
     */
    psh.dwSize = sizeof (PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hOwner;
    psh.hInstance = hInstance;
    psh.u.pszIcon = NULL;
    psh.pszCaption = "Wine Configuration";
    psh.nPages = NUM_PROPERTY_PAGES;
    psh.u3.ppsp = (LPCPROPSHEETPAGE) & psp;
    psh.pfnCallback = (PFNPROPSHEETCALLBACK) PropSheetCallback;

    /*
     * Display the modal property sheet
     */
    return PropertySheet (&psh);
}


/*****************************************************************************
 * Name       : WinMain
 * Description: Main windows entry point
 * Parameters : hInstance
 *              hPrev
 *              szCmdLine
 *              nShow
 * Returns    : Program exit code
 */
int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrev, LPSTR szCmdLine, int nShow)
{

    /* Until winecfg is fully functional, warn users that it is incomplete and doesn't do anything */
    WINE_FIXME("The winecfg tool is not yet complete, and does not actually alter your configuration.\n");
    WINE_FIXME("If you want to alter the way Wine works, look in the ~/.wine/config file for more information.\n");

    if (initialize() != 0) {
	WINE_ERR("initialization failed, aborting\n");
	ExitProcess(1);
    }
    
    /*
     * The next 3 lines should be all that is needed
     * for the Wine Configuration property sheet
     */
    InitCommonControls ();
    if (doPropertySheet (hInstance, NULL) > 0) {
	WINE_TRACE("OK\n");
    } else
	WINE_TRACE("Cancel\n");
    
    ExitProcess (0);

    return 0;
}
