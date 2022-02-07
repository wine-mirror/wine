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

    SI_ACCESS *access;
    ULONG access_count;

    struct user *users;
    unsigned int user_count;

    HWND dialog;
    HIMAGELIST image_list;
};

static HINSTANCE aclui_instance;

static WCHAR *WINAPIV load_formatstr(UINT resource, ...)
{
    va_list valist;
    WCHAR *str;
    DWORD ret;

    va_start(valist, resource);
    ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                         aclui_instance, resource, 0, (WCHAR*)&str, 0, &valist);
    va_end(valist);
    return ret ? str : NULL;
}

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
    {
        free(name);
        return;
    }
    page->users = new_array;
    user = &page->users[page->user_count++];

    user->name = name;
    user->sid = sid;

    item.mask     = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
    item.iItem    = -1;
    item.iSubItem = 0;
    item.pszText  = name;
    item.lParam   = (LPARAM)user;
    item.iImage = (sid_type == SidTypeGroup || sid_type == SidTypeWellKnownGroup) ? 0 : 1;

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

static void compute_access_masks(PSECURITY_DESCRIPTOR sd, PSID sid, ACCESS_MASK *allowed, ACCESS_MASK *denied)
{
    BOOL defaulted, present;
    ACE_HEADER *ace;
    PSID ace_sid;
    DWORD index;
    ACL *dacl;

    *allowed = 0;
    *denied  = 0;

    if (!GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted) || !present)
        return;

    for (index = 0; index < dacl->AceCount; index++)
    {
        if (!GetAce(dacl, index, (void**)&ace))
            break;

        ace_sid = get_sid_from_ace(ace);
        if (!ace_sid || !EqualSid(ace_sid, sid))
            continue;

        if (ace->AceType == ACCESS_ALLOWED_ACE_TYPE)
            *allowed |= ((ACCESS_ALLOWED_ACE*)ace)->Mask;
        else if (ace->AceType == ACCESS_DENIED_ACE_TYPE)
            *denied |= ((ACCESS_DENIED_ACE*)ace)->Mask;
    }
}

static void update_access_list(struct security_page *page, struct user *user)
{
    ACCESS_MASK allowed, denied;
    WCHAR *infotext;
    ULONG i, index;
    LVITEMW item;
    HWND control;

    compute_access_masks(page->sd, user->sid, &allowed, &denied);

    if ((infotext = load_formatstr(IDS_PERMISSION_FOR, user->name)))
    {
        SetDlgItemTextW(page->dialog, IDC_ACE_USER, infotext);
        LocalFree(infotext);
    }

    control = GetDlgItem(page->dialog, IDC_ACE);
    index = 0;
    for (i = 0; i < page->access_count; i++)
    {
        if (!(page->access[i].dwFlags & SI_ACCESS_GENERAL))
            continue;

        item.mask = LVIF_TEXT;
        item.iItem = index;

        item.iSubItem = 1;
        if ((page->access[i].mask & allowed) == page->access[i].mask)
            item.pszText = (WCHAR *)L"X";
        else
            item.pszText = (WCHAR *)L"-";
        SendMessageW(control, LVM_SETITEMW, 0, (LPARAM)&item);

        item.iSubItem = 2;
        if ((page->access[i].mask & denied) == page->access[i].mask)
            item.pszText = (WCHAR *)L"X";
        else
            item.pszText = (WCHAR *)L"-";
        SendMessageW(control, LVM_SETITEMW, 0, (LPARAM)&item);

        index++;
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
        ERR("Failed to query descriptor information, error %lu.\n", GetLastError());
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

static void init_access_list(struct security_page *page)
{
    ULONG i, index;
    WCHAR str[256];
    LVITEMW item;
    HWND control;

    control = GetDlgItem(page->dialog, IDC_ACE);
    index = 0;
    for (i = 0; i < page->access_count; i++)
    {
        if (!(page->access[i].dwFlags & SI_ACCESS_GENERAL))
            continue;

        item.mask     = LVIF_TEXT;
        item.iItem    = index;
        item.iSubItem = 0;
        if (IS_INTRESOURCE(page->access[i].pszName))
        {
            str[0] = 0;
            LoadStringW(page->info.hInstance, (DWORD_PTR)page->access[i].pszName, str, ARRAY_SIZE(str));
            item.pszText = str;
        }
        else
            item.pszText = (WCHAR *)page->access[i].pszName;
        SendMessageW(control, LVM_INSERTITEMW, 0, (LPARAM)&item);

        index++;
    }
}

static HIMAGELIST create_image_list(UINT resource, UINT width, UINT height, UINT count, COLORREF mask_color)
{
    HIMAGELIST image_list;
    HBITMAP image;
    INT ret;

    if (!(image_list = ImageList_Create(width, height, ILC_COLOR32 | ILC_MASK, 0, count)))
        return NULL;
    if (!(image = LoadBitmapW(aclui_instance, MAKEINTRESOURCEW(resource))))
    {
        ImageList_Destroy(image_list);
        return NULL;
    }

    ret = ImageList_AddMasked(image_list, image, mask_color);
    DeleteObject(image);
    if (ret == -1)
    {
        ImageList_Destroy(image_list);
        return NULL;
    }
    return image_list;
}

static void security_page_free(struct security_page *page)
{
    unsigned int i;

    for (i = 0; i < page->user_count; ++i)
        free(page->users[i].name);
    free(page->users);

    LocalFree(page->sd);
    if (page->image_list)
        ImageList_Destroy(page->image_list);
    if (page->security)
        ISecurityInformation_Release(page->security);
    free(page);
}

static void security_page_init_dlg(HWND hwnd, struct security_page *page)
{
    LVCOLUMNW column;
    HWND control;
    HRESULT hr;
    ULONG def;
    RECT rect;

    page->dialog = hwnd;

    if (FAILED(hr = ISecurityInformation_GetSecurity(page->security,
            DACL_SECURITY_INFORMATION, &page->sd, FALSE)))
    {
        ERR("Failed to get security descriptor, hr %#lx.\n", hr);
        return;
    }

    if (FAILED(hr = ISecurityInformation_GetAccessRights(page->security,
            NULL, 0, &page->access, &page->access_count, &def)))
    {
        ERR("Failed to get access mapping, hr %#lx.\n", hr);
        return;
    }

    /* user list */

    control = GetDlgItem(hwnd, IDC_USERS);
    SendMessageW(control, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    GetClientRect(control, &rect);
    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    column.cx = rect.right - rect.left;
    SendMessageW(control, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

    if (!(page->image_list = create_image_list(IDB_USER_ICONS, 18, 18, 2, RGB(255, 0, 255))))
        return;
    SendMessageW(control, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)page->image_list);

    init_users(page);

    /* ACE list */

    control = GetDlgItem(hwnd, IDC_ACE);
    SendMessageW(control, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

    column.mask = LVCF_FMT | LVCF_WIDTH;
    column.fmt = LVCFMT_LEFT;
    column.cx = 170;
    SendMessageW(control, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

    column.fmt = LVCFMT_CENTER;
    column.cx = 85;
    SendMessageW(control, LVM_INSERTCOLUMNW, 1, (LPARAM)&column);
    SendMessageW(control, LVM_INSERTCOLUMNW, 2, (LPARAM)&column);

    init_access_list(page);

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
            SetWindowLongPtrW(dialog, DWLP_USER, propsheet->lParam);
            security_page_init_dlg(dialog, (struct security_page *)propsheet->lParam);
            break;
        }

        case WM_NOTIFY:
        {
            struct security_page *page = (struct security_page *)GetWindowLongPtrW(dialog, DWLP_USER);
            NMHDR *hdr = (NMHDR *)lparam;

            if (hdr->hwndFrom == GetDlgItem(dialog, IDC_USERS) && hdr->code == LVN_ITEMCHANGED)
            {
                NMLISTVIEW *listview = (NMLISTVIEW *)lparam;
                if (!(listview->uOldState & LVIS_SELECTED) && (listview->uNewState & LVIS_SELECTED))
                    update_access_list(page, (struct user *)listview->lParam);
                return TRUE;
            }
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

BOOL WINAPI EditSecurity(HWND owner, ISecurityInformation *security)
{
    PROPSHEETHEADERW sheet = {0};
    HPROPSHEETPAGE pages[1];
    SI_OBJECT_INFO info;
    BOOL ret;

    TRACE("(%p, %p)\n", owner, security);

    if (FAILED(ISecurityInformation_GetObjectInformation(security, &info)))
        return FALSE;
    if (!(pages[0] = CreateSecurityPage(security)))
        return FALSE;

    sheet.dwSize = sizeof(sheet);
    sheet.dwFlags = PSH_DEFAULT;
    sheet.hwndParent = owner;
    sheet.hInstance = aclui_instance;
    sheet.pszCaption = load_formatstr(IDS_PERMISSION_FOR, info.pszObjectName);
    sheet.nPages = 1;
    sheet.nStartPage = 0;
    sheet.phpage = pages;

    ret = PropertySheetW(&sheet) != -1;
    LocalFree((void *)sheet.pszCaption);
    return ret;
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
