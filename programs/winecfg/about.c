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

#include <windows.h>
#include <commctrl.h>
#include <wine/debug.h>

#include "resource.h"
#include "winecfg.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecfg);

INT_PTR CALLBACK
AboutDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    char *owner, *org;

    switch (uMsg)
    {
    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_APPLY:
            /*save registration info to registry */
            owner = get_text(hDlg, IDC_ABT_OWNER);
            org   = get_text(hDlg, IDC_ABT_ORG);

            set_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion",
                        "RegisteredOwner", owner ? owner : "");
            set_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion",
                        "RegisteredOrganization", org ? org : "");
            set_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion",
                        "RegisteredOwner", owner ? owner : "");
            set_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion",
                        "RegisteredOrganization", org ? org : "");
            apply();

            HeapFree(GetProcessHeap(), 0, owner);
            HeapFree(GetProcessHeap(), 0, org);
            break;
        }
        break;

    case WM_INITDIALOG:
        /* read owner and organization info from registry, load it into text box */
        owner = get_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion",
                            "RegisteredOwner", "");
        org =   get_reg_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion",
                            "RegisteredOrganization", "");

        SetDlgItemText(hDlg, IDC_ABT_OWNER, owner);
        SetDlgItemText(hDlg, IDC_ABT_ORG, org);

        SendMessage(GetParent(hDlg), PSM_UNCHANGED, 0, 0);

        HeapFree(GetProcessHeap(), 0, owner);
        HeapFree(GetProcessHeap(), 0, org);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case EN_CHANGE:
            /* enable apply button */
            SendMessage(GetParent(hDlg), PSM_CHANGED, 0, 0);
            break;
        }
        break;
    }
    return FALSE;
}
