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

INT_PTR CALLBACK
AboutDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {

	case WM_NOTIFY:
	    if (((LPNMHDR)lParam)->code != PSN_SETACTIVE) break;
	    /* otherwise fall through, we want to refresh the page as well */
	case WM_INITDIALOG:
	    break;

	case WM_COMMAND:
	    break;
	    
	default:
	    break;
	    
    }
    return FALSE;
}

#define NUM_PROPERTY_PAGES 6

INT_PTR
doPropertySheet (HINSTANCE hInstance, HWND hOwner)
{
    PROPSHEETPAGE psp[NUM_PROPERTY_PAGES];
    PROPSHEETHEADER psh;
    int pg = 0; /* start with page 0 */

    /*
     * Fill out the (Applications) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_APPCFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = AppDlgProc;
    psp[pg].pszTitle = "Applications";
    psp[pg].lParam = 0;
    pg++;

    /*
     * Fill out the (Libraries) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_DLLCFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = LibrariesDlgProc;
    psp[pg].pszTitle = "Libraries";
    psp[pg].lParam = 0;
    pg++;
    
    /*
     * Fill out the (X11Drv) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_X11DRVCFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = X11DrvDlgProc;
    psp[pg].pszTitle = "X11 Driver";
    psp[pg].lParam = 0;
    pg++;

    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_DRIVECFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = DriveDlgProc;
    psp[pg].pszTitle = "Drives";
    psp[pg].lParam = 0;
    pg++;

    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_AUDIOCFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = AudioDlgProc;
    psp[pg].pszTitle = "Audio";
    psp[pg].lParam = 0;
    pg++;

    /*
     * Fill out the (General) PROPSHEETPAGE data structure 
     * for the property sheet
     */
    psp[pg].dwSize = sizeof (PROPSHEETPAGE);
    psp[pg].dwFlags = PSP_USETITLE;
    psp[pg].hInstance = hInstance;
    psp[pg].u.pszTemplate = MAKEINTRESOURCE (IDD_ABOUTCFG);
    psp[pg].u2.pszIcon = NULL;
    psp[pg].pfnDlgProc = AboutDlgProc;
    psp[pg].pszTitle = "About";
    psp[pg].lParam = 0;
    pg++;

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
    psh.u2.nStartPage = 0;

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
    } else {
	WINE_TRACE("Cancel\n");
    }
    
    ExitProcess (0);

    return 0;
}
