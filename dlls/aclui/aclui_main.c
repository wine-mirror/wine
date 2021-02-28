/*
 * Implementation of the AclUI
 *
 * Copyright (C) 2009 Nikolay Sivov
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
 */

#include <stdarg.h>
#define COBJMACROS
#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnt.h"
#include "aclui.h"
#include "resource.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aclui);

/* the aclui.h files does not contain the necessary COBJMACROS */
#define ISecurityInformation_AddRef(This) (This)->lpVtbl->AddRef(This)
#define ISecurityInformation_Release(This) (This)->lpVtbl->Release(This)
#define ISecurityInformation_GetObjectInformation(This, obj) (This)->lpVtbl->GetObjectInformation(This, obj)
#define ISecurityInformation_GetSecurity(This, info, sd, def) (This)->lpVtbl->GetSecurity(This, info, sd, def)
#define ISecurityInformation_GetAccessRights(This, type, flags, access, count, def) (This)->lpVtbl->GetAccessRights(This, type, flags, access, count, def)

struct security_page
{
    SI_OBJECT_INFO info;

    HWND dialog;
};

static HINSTANCE aclui_instance;

static void security_page_free(struct security_page *page)
{
    free(page);
}

static void security_page_init_dlg(HWND hwnd, struct security_page *page)
{
    page->dialog = hwnd;
}

static INT_PTR CALLBACK security_page_proc(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGEW *propsheet = (PROPSHEETPAGEW *)lparam;
            security_page_init_dlg(dialog, (struct security_page *)propsheet->lParam);
            break;
        }
    }
    return FALSE;
}

static UINT CALLBACK security_page_callback(HWND hwnd, UINT msg, PROPSHEETPAGEW *ppsp)
{
    struct security_page *page = (struct security_page *)ppsp->lParam;

    if (msg == PSPCB_RELEASE)
        security_page_free(page);

    return 1;
}

HPROPSHEETPAGE WINAPI CreateSecurityPage(ISecurityInformation *security)
{
    struct security_page *page;
    PROPSHEETPAGEW propsheet;
    HPROPSHEETPAGE ret;

    TRACE("%p\n", security);

    InitCommonControls();

    if (!(page = calloc(1, sizeof(*page))))
        return NULL;

    if (FAILED(ISecurityInformation_GetObjectInformation(security, &page->info)))
    {
        free(page);
        return NULL;
    }

    memset(&propsheet, 0, sizeof(propsheet));
    propsheet.dwSize        = sizeof(propsheet);
    propsheet.dwFlags       = PSP_DEFAULT | PSP_USECALLBACK;
    propsheet.hInstance     = aclui_instance;
    propsheet.pszTemplate   = (WCHAR *)MAKEINTRESOURCE(IDD_SECURITY_PROPERTIES);
    propsheet.pfnDlgProc    = security_page_proc;
    propsheet.pfnCallback   = security_page_callback;
    propsheet.lParam        = (LPARAM)page;

    if (page->info.dwFlags & SI_PAGE_TITLE)
    {
        propsheet.pszTitle = page->info.pszPageTitle;
        propsheet.dwFlags |= PSP_USETITLE;
    }

    if (!(ret = CreatePropertySheetPageW(&propsheet)))
    {
        ERR("Failed to create property sheet page.\n");
        free(page);
        return NULL;
    }
    return ret;
}

BOOL WINAPI EditSecurity(HWND owner, LPSECURITYINFO psi)
{
    FIXME("(%p, %p): stub\n", owner, psi);
    return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        aclui_instance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}
