/*
 * Internet control panel applet: security propsheet
 *
 * Copyright 2011 Detlef Riekenberg
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
#define NONAMELESSUNION

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <prsht.h>
#include "commctrl.h"

#include "ole2.h"
#include "urlmon.h"
#include "initguid.h"
#include "winreg.h"
#include "shlwapi.h"

#include "inetcpl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

typedef struct secdlg_data_s {
    HWND hsec;  /* security propsheet */
    HWND hlv;   /* listview */
    IInternetSecurityManager *sec_mgr;
    IInternetZoneManager *zone_mgr;
    DWORD zone_enumerator;
    DWORD num_zones;
} secdlg_data;

/*********************************************************************
 * security_cleanup_zones [internal]
 *
 */
static void security_cleanup_zones(secdlg_data *sd)
{
    if (sd->zone_enumerator) {
        IInternetZoneManager_DestroyZoneEnumerator(sd->zone_mgr, sd->zone_enumerator);
    }

    if (sd->zone_mgr) {
        IInternetZoneManager_Release(sd->zone_mgr);
    }

    if (sd->sec_mgr) {
        IInternetSecurityManager_Release(sd->sec_mgr);
    }
}

/*********************************************************************
 * security_enum_zones [internal]
 *
 */
static HRESULT security_enum_zones(secdlg_data * sd)
{
    HRESULT hr;

    hr = CoInternetCreateSecurityManager(NULL, &sd->sec_mgr, 0);
    if (SUCCEEDED(hr)) {
        hr = CoInternetCreateZoneManager(NULL, &sd->zone_mgr, 0);
        if (SUCCEEDED(hr)) {
            hr = IInternetZoneManager_CreateZoneEnumerator(sd->zone_mgr, &sd->zone_enumerator, &sd->num_zones, 0);
        }
    }
    return hr;
}

/*********************************************************************
 * security_on_destroy [internal]
 *
 * handle WM_NCDESTROY
 *
 */
static INT_PTR security_on_destroy(secdlg_data * sd)
{
    TRACE("(%p)\n", sd);

    security_cleanup_zones(sd);
    SetWindowLongPtrW(sd->hsec, DWLP_USER, 0);
    heap_free(sd);
    return TRUE;
}

/*********************************************************************
 * security_on_initdialog [internal]
 *
 * handle WM_INITDIALOG
 *
 */
static INT_PTR security_on_initdialog(HWND hsec)
{
    secdlg_data *sd;
    HRESULT hr;

    sd = heap_alloc_zero(sizeof(secdlg_data));
    SetWindowLongPtrW(hsec, DWLP_USER, (LONG_PTR) sd);
    if (!sd) {
        return FALSE;
    }

    sd->hsec = hsec;
    sd->hlv = GetDlgItem(hsec, IDC_SEC_LISTVIEW);

    hr = security_enum_zones(sd);
    if (FAILED(hr)) {
        ERR("got 0x%x\n", hr);
        security_on_destroy(sd);
        return FALSE;
    }

    TRACE("found %d zones\n", sd->num_zones);
    return TRUE;
}

/*********************************************************************
 * security_on_notify [internal]
 *
 * handle WM_NOTIFY
 *
 */
static INT_PTR security_on_notify(WPARAM wparam, LPARAM lparam)
{
    NMLISTVIEW *nm;

    nm = (NMLISTVIEW *) lparam;
    switch (nm->hdr.code)
    {
        case PSN_APPLY:
            TRACE("PSN_APPLY (0x%lx, 0x%lx) from %p with code: %d\n", wparam, lparam,
                    nm->hdr.hwndFrom, nm->hdr.code);
            break;

        default:
            TRACE("WM_NOTIFY (0x%lx, 0x%lx) from %p with code: %d\n", wparam, lparam,
                    nm->hdr.hwndFrom, nm->hdr.code);

    }
    return FALSE;
}

/*********************************************************************
 * security_dlgproc [internal]
 *
 */
INT_PTR CALLBACK security_dlgproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    secdlg_data *sd;

    if (msg == WM_INITDIALOG) {
        return security_on_initdialog(hwnd);
    }

    sd = (secdlg_data *)GetWindowLongPtrW(hwnd, DWLP_USER);
    if (sd) {
        switch (msg)
        {
            case WM_NOTIFY:
                return security_on_notify(wparam, lparam);

            case WM_NCDESTROY:
                return security_on_destroy(sd);

            default:
                /* do not flood the log */
                if ((msg == WM_SETCURSOR) || (msg == WM_NCHITTEST) ||
                    (msg == WM_MOUSEMOVE) || (msg == WM_MOUSEACTIVATE) || (msg == WM_PARENTNOTIFY))
                    return FALSE;

                TRACE("(%p, 0x%08x/%03d, 0x%08lx, 0x%08lx)\n", hwnd, msg, msg, wparam, lparam);
        }
    }
    return FALSE;
}
