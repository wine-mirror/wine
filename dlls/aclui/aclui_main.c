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

struct user
{
    WCHAR *name;
    PSID sid;
};

struct security_page
{
    ISecurityInformation *security;
    SI_OBJECT_INFO info;
    PSECURITY_DESCRIPTOR sd;

    struct user *users;
    unsigned int user_count;

    HWND dialog;
};

static HINSTANCE aclui_instance;

static WCHAR *get_sid_name(PSID sid, SID_NAME_USE *sid_type)
{
    WCHAR *name, *domain;
    DWORD domain_len = 0;
    DWORD name_len = 0;
    BOOL ret;

    LookupAccountSidW(NULL, sid, NULL, &name_len, NULL, &domain_len, sid_type);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return NULL;
    if (!(name = malloc(name_len * sizeof(WCHAR))))
        return NULL;
    if (!(domain = malloc(domain_len * sizeof(WCHAR))))
    {
        free(name);
        return NULL;
    }

    ret = LookupAccountSidW(NULL, sid, name, &name_len, domain, &domain_len, sid_type);
    free(domain);
    if (ret) return name;
    free(name);
    return NULL;
}

static void add_user(struct security_page *page, PSID sid)
{
    struct user *new_array, *user;
    SID_NAME_USE sid_type;
    unsigned int i;
    LVITEMW item;
    WCHAR *name;

    /* check if we already processed this user or group */
    for (i = 0; i < page->user_count; ++i)
    {
        if (EqualSid(sid, page->users[i].sid))
            return;
    }

    if (!(name = get_sid_name(sid, &sid_type)))
        return;

    if (!(new_array = realloc(page->users, (page->user_count + 1) * sizeof(*page->users))))
        return;
    page->users = new_array;
    user = &page->users[page->user_count++];

    user->name = name;
    user->sid = sid;

    item.mask     = LVIF_PARAM | LVIF_TEXT;
    item.iItem    = -1;
    item.iSubItem = 0;
    item.pszText  = name;
    item.lParam   = (LPARAM)user;

    SendMessageW(GetDlgItem(page->dialog, IDC_USERS), LVM_INSERTITEMW, 0, (LPARAM)&item);
}

static PSID get_sid_from_ace(ACE_HEADER *ace)
{
    switch (ace->AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            return &((ACCESS_ALLOWED_ACE *)ace)->SidStart;
        case ACCESS_DENIED_ACE_TYPE:
            return &((ACCESS_DENIED_ACE *)ace)->SidStart;
        default:
            FIXME("Unhandled ACE type %#x.\n", ace->AceType);
            return NULL;
    }
}

static void init_users(struct security_page *page)
{
    BOOL defaulted, present;
    ACE_HEADER *ace;
    DWORD index;
    ACL *dacl;
    PSID sid;

    if (!GetSecurityDescriptorDacl(page->sd, &present, &dacl, &defaulted))
    {
        ERR("Failed to query descriptor information, error %u.\n", GetLastError());
        return;
    }

    if (!present)
        return;

    for (index = 0; index < dacl->AceCount; index++)
    {
        if (!GetAce(dacl, index, (void **)&ace))
            break;
        if (!(sid = get_sid_from_ace(ace)))
            continue;
        add_user(page, sid);
    }
}

static void security_page_free(struct security_page *page)
{
    unsigned int i;

    for (i = 0; i < page->user_count; ++i)
        free(page->users[i].name);
    free(page->users);

    LocalFree(page->sd);
    if (page->security)
        ISecurityInformation_Release(page->security);
    free(page);
}

static void security_page_init_dlg(HWND hwnd, struct security_page *page)
{
    LVCOLUMNW column;
    HWND control;
    HRESULT hr;
    RECT rect;

    page->dialog = hwnd;

    if (FAILED(hr = ISecurityInformation_GetSecurity(page->security,
            DACL_SECURITY_INFORMATION, &page->sd, FALSE)))
    {
        ERR("Failed to get security descriptor, hr %#x.\n", hr);
        return;
    }

    control = GetDlgItem(hwnd, IDC_USERS);
    SendMessageW(control, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    GetClientRect(control, &rect);
    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    column.cx = rect.right - rect.left;
    SendMessageW(control, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

    init_users(page);

    if (page->user_count)
    {
        LVITEMW item;
        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_FOCUSED | LVIS_SELECTED;
        item.stateMask = item.state;
        SendMessageW(control, LVM_SETITEMW, 0, (LPARAM)&item);
    }
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

    page->security = security;
    ISecurityInformation_AddRef(security);

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
        ISecurityInformation_Release(security);
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
