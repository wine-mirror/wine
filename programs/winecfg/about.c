/*
 * WineCfg about panel
 *
 * Copyright 2002 Jaco Greeff
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Mike Hearn
 * Copyright 2010 Joel Holdsworth
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

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include "resource.h"
#include "winecfg.h"


static HICON logo = NULL;
static HFONT titleFont = NULL;

INT_PTR CALLBACK
AboutDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const char * (CDECL *wine_get_version)(void);
    HWND hWnd;
    HDC hDC;
    RECT rcClient, rcRect;
    WCHAR *owner, *org;

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_APPLY:
            /*save registration info to registry */
            owner = get_text(hDlg, IDC_ABT_OWNER);
            org   = get_text(hDlg, IDC_ABT_ORG);

            set_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion",
                        L"RegisteredOwner", owner ? owner : L"");
            set_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion",
                        L"RegisteredOrganization", org ? org : L"");
            set_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                        L"RegisteredOwner", owner ? owner : L"");
            set_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                        L"RegisteredOrganization", org ? org : L"");
            apply();

            free(owner);
            free(org);
            break;

        case NM_CLICK:
        case NM_RETURN:
            if(wParam == IDC_ABT_WEB_LINK)
                ShellExecuteW(NULL, L"open", ((NMLINK *)lParam)->item.szUrl, NULL, NULL, SW_SHOW);
            break;
        }
        break;

    case WM_INITDIALOG:
        hDC = GetDC(hDlg);

        /* read owner and organization info from registry, load it into text box */
        owner = get_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                            L"RegisteredOwner", L"");
        org =   get_reg_key(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion",
                            L"RegisteredOrganization", L"");

        SetDlgItemTextW(hDlg, IDC_ABT_OWNER, owner);
        SetDlgItemTextW(hDlg, IDC_ABT_ORG, org);

        SendMessageW(GetParent(hDlg), PSM_UNCHANGED, 0, 0);

        free(owner);
        free(org);

        /* prepare the panel */
        hWnd = GetDlgItem(hDlg, IDC_ABT_PANEL);
        if(hWnd)
        {
            GetClientRect(hDlg, &rcClient);
            GetClientRect(hWnd, &rcRect);
            MoveWindow(hWnd, 0, 0, rcClient.right, rcRect.bottom, FALSE);

            logo = LoadImageW((HINSTANCE)GetWindowLongPtrW(hDlg, GWLP_HINSTANCE),
                MAKEINTRESOURCEW(IDI_LOGO), IMAGE_ICON, 0, 0, LR_SHARED);
        }

        /* prepare the title text */
        titleFont = CreateFontW( -MulDiv(24, GetDeviceCaps(hDC, LOGPIXELSY), 72),
                                 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Tahoma" );
        SendDlgItemMessageW(hDlg, IDC_ABT_TITLE_TEXT, WM_SETFONT, (WPARAM)titleFont, TRUE);

        wine_get_version = (void *)GetProcAddress( GetModuleHandleW(L"ntdll.dll"), "wine_get_version" );
        if (wine_get_version) SetDlgItemTextA(hDlg, IDC_ABT_PANEL_TEXT, wine_get_version());

        ReleaseDC(hDlg, hDC);

        break;

    case WM_DESTROY:
        if(logo)
        {
            DestroyIcon(logo);
            logo = NULL;
        }

        if(titleFont)
        {
            DeleteObject(titleFont);
            titleFont = NULL;
        }

        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case EN_CHANGE:
            /* enable apply button */
            SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
            break;
        }
        break;

    case WM_DRAWITEM:
        if(wParam == IDC_ABT_PANEL)
        {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            FillRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH) (COLOR_WINDOW+1));
            DrawIconEx(pDIS->hDC, 0, 0, logo, 0, 0, 0, 0, DI_IMAGE);
            DrawEdge(pDIS->hDC, &pDIS->rcItem, EDGE_SUNKEN, BF_BOTTOM);
        }
        break;

    case WM_CTLCOLORSTATIC:
        switch(GetDlgCtrlID((HWND)lParam))
        {
        case IDC_ABT_TITLE_TEXT:
            /* set the title to a wine color */
            SetTextColor((HDC)wParam, 0x0000007F);
            SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        case IDC_ABT_PANEL_TEXT:
        case IDC_ABT_LICENSE_TEXT:
        case IDC_ABT_WEB_LINK:
            SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}
