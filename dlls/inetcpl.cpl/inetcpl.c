/*
 * Internet control panel applet
 *
 * Copyright 2010 Detlef Riekenberg
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

#define COBJMACROS
#define CONST_VTABLE

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <commctrl.h>
#include <cpl.h>
#include "ole2.h"

#include "wine/debug.h"

#include "inetcpl.h"


WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

HMODULE hcpl;

/*********************************************************************
 *  DllMain (inetcpl.@)
 */
BOOL WINAPI DllMain(HINSTANCE hdll, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %ld, %p)\n", hdll, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hdll);
            hcpl = hdll;
    }
    return TRUE;
}

/***********************************************************************
 *  DllInstall (inetcpl.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("(%s, %s): stub\n", bInstall ? "TRUE" : "FALSE", debugstr_w(cmdline));
    return S_OK;
}

/******************************************************************************
 * propsheet_callback [internal]
 *
 */
static int CALLBACK propsheet_callback(HWND hwnd, UINT msg, LPARAM lparam)
{

    TRACE("(%p, 0x%08x/%d, 0x%Ix)\n", hwnd, msg, msg, lparam);
    switch (msg)
    {
        case PSCB_INITIALIZED:
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM) LoadIconW(hcpl, MAKEINTRESOURCEW(ICO_MAIN)));
            break;
    }
    return 0;
}

/******************************************************************************
 * display_cpl_sheets [internal]
 *
 * Build and display the dialog with all control panel propertysheets
 *
 */
static void display_cpl_sheets(HWND parent)
{
    INITCOMMONCONTROLSEX icex;
    PROPSHEETPAGEW psp[NUM_PROPERTY_PAGES];
    PROPSHEETHEADERW psh;
    DWORD id = 0;

    OleInitialize(NULL);
    /* Initialize common controls */
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(psp, sizeof(psp));

    /* Fill out all PROPSHEETPAGE */
    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].pszTemplate = MAKEINTRESOURCEW(IDD_GENERAL);
    psp[id].pfnDlgProc = general_dlgproc;
    id++;

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].pszTemplate = MAKEINTRESOURCEW(IDD_SECURITY);
    psp[id].pfnDlgProc = security_dlgproc;
    id++;

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].pszTemplate = MAKEINTRESOURCEW(IDD_CONTENT);
    psp[id].pfnDlgProc = content_dlgproc;
    id++;

    psp[id].dwSize = sizeof (PROPSHEETPAGEW);
    psp[id].hInstance = hcpl;
    psp[id].pszTemplate = MAKEINTRESOURCEW(IDD_CONNECTIONS);
    psp[id].pfnDlgProc = connections_dlgproc;
    id++;

    /* Fill out the PROPSHEETHEADER */
    psh.dwSize = sizeof (PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = parent;
    psh.hInstance = hcpl;
    psh.pszIcon = MAKEINTRESOURCEW(ICO_MAIN);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_CPL_NAME);
    psh.nPages = id;
    psh.ppsp = psp;
    psh.pfnCallback = propsheet_callback;

    /* display the dialog */
    PropertySheetW(&psh);

    OleUninitialize();
}

/*********************************************************************
 * CPlApplet (inetcpl.@)
 *
 * Control Panel entry point
 *
 * PARAMS
 *  hWnd    [I] Handle for the Control Panel Window
 *  command [I] CPL_* Command
 *  lParam1 [I] first extra Parameter
 *  lParam2 [I] second extra Parameter
 *
 * RETURNS
 *  Depends on the command
 *
 */
LONG CALLBACK CPlApplet(HWND hWnd, UINT command, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%p, %u, 0x%Ix, 0x%Ix)\n", hWnd, command, lParam1, lParam2);

    switch (command)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
        {
            CPLINFO *appletInfo = (CPLINFO *) lParam2;

            appletInfo->idIcon = ICO_MAIN;
            appletInfo->idName = IDS_CPL_NAME;
            appletInfo->idInfo = IDS_CPL_INFO;
            appletInfo->lData = 0;
            return TRUE;
        }

        case CPL_DBLCLK:
            display_cpl_sheets(hWnd);
            break;
    }

    return FALSE;
}

/*********************************************************************
 * LaunchInternetControlPanel (inetcpl.@)
 *
 * Launch the Internet Control Panel dialog
 *
 * PARAMS
 *  parent  [I] Handle for the parent window
 *
 * RETURNS
 *  Success: TRUE
 *
 * NOTES
 *  rundll32 callable function: rundll32 inetcpl.cpl,LaunchInternetControlPanel
 *
 */
BOOL WINAPI LaunchInternetControlPanel(HWND parent)
{
    display_cpl_sheets(parent);
    return TRUE;
}

/*********************************************************************
 * LaunchConnectionDialog (inetcpl.@)
 *
 */
BOOL WINAPI LaunchConnectionDialog(HWND hParent)
{
    FIXME("(%p): stub\n", hParent);
    return FALSE;
}

/*********************************************************************
 * LaunchInternetControlPanel (inetcpl.@)
 *
 */
BOOL WINAPI LaunchPrivacyDialog(HWND hParent)
{
    FIXME("(%p): stub\n", hParent);
    return FALSE;
}
