/* Data Links
 *
 * Copyright 2013 Alistair Leslie-Hughes
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
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"
#include "oledb.h"
#include "oledberr.h"
#include "msdasc.h"
#include "prsht.h"
#include "commctrl.h"

#include "oledb_private.h"
#include "resource.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

struct datasource
{
    CLSID clsid;
    IDBProperties *provider;
    DBPROPINFOSET *propinfoset;
    WCHAR *description;
};

static struct datasource *create_datasource(WCHAR *guid)
{
    struct datasource *data = calloc(1, sizeof(struct datasource));
    if (data)
    {
        CLSIDFromString(guid, &data->clsid);
    }

    return data;
}

static void destroy_datasource(struct datasource *data)
{
    if (data->propinfoset)
    {
        ULONG i;

        for (i = 0; i < data->propinfoset->cPropertyInfos; i++)
            VariantClear(&data->propinfoset->rgPropertyInfos[i].vValues);

        CoTaskMemFree(data->propinfoset->rgPropertyInfos);
        CoTaskMemFree(data->propinfoset);
    }
    CoTaskMemFree(data->description);

    if (data->provider)
        IDBProperties_Release(data->provider);

    free(data);
}

static BOOL initialize_datasource(struct datasource *data)
{
    HRESULT hr;
    DBPROPIDSET propidset;
    ULONG infocount;

    hr = CoCreateInstance(&data->clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDBProperties, (void**)&data->provider);
    if (FAILED(hr))
    {
        WARN("Datasource cannot be created (0x%08lx)\n", hr);
        return FALSE;
    }

    propidset.rgPropertyIDs = NULL;
    propidset.cPropertyIDs  = 0;
    propidset.guidPropertySet = DBPROPSET_DBINITALL;

    hr = IDBProperties_GetPropertyInfo(data->provider, 1, &propidset, &infocount, &data->propinfoset, &data->description);
    if (FAILED(hr))
    {
        WARN("Failed to get DB Properties (0x%08lx)\n", hr);

        IDBProperties_Release(data->provider);
        data->provider = NULL;
        return FALSE;
    }

    return TRUE;
}

typedef struct DSLocatorImpl
{
    IDataSourceLocator IDataSourceLocator_iface;
    IDataInitialize IDataInitialize_iface;
    LONG ref;

    HWND hwnd;
} DSLocatorImpl;

static inline DSLocatorImpl *impl_from_IDataSourceLocator( IDataSourceLocator *iface )
{
    return CONTAINING_RECORD(iface, DSLocatorImpl, IDataSourceLocator_iface);
}

static inline DSLocatorImpl *impl_from_IDataInitialize(IDataInitialize *iface)
{
    return CONTAINING_RECORD(iface, DSLocatorImpl, IDataInitialize_iface);
}

static HRESULT WINAPI dslocator_QueryInterface(IDataSourceLocator *iface, REFIID riid, void **ppvoid)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid),ppvoid);

    *ppvoid = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IDataSourceLocator))
    {
      *ppvoid = &This->IDataSourceLocator_iface;
    }
    else if (IsEqualIID(riid, &IID_IDataInitialize))
    {
      *ppvoid = &This->IDataInitialize_iface;
    }
    else if (IsEqualIID(riid, &IID_IRunnableObject))
    {
      TRACE("IID_IRunnableObject returning NULL\n");
      return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
      TRACE("IID_IProvideClassInfo returning NULL\n");
      return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IMarshal))
    {
      TRACE("IID_IMarshal returning NULL\n");
      return E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, &IID_IRpcOptions))
    {
      TRACE("IID_IRpcOptions returning NULL\n");
      return E_NOINTERFACE;
    }

    if(*ppvoid)
    {
      IUnknown_AddRef( (IUnknown*)*ppvoid );
      return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI dslocator_AddRef(IDataSourceLocator *iface)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);
    TRACE("(%p)->%lu\n",This,This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI dslocator_Release(IDataSourceLocator *iface)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->%lu\n",This,ref+1);

    if (!ref)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI dslocator_GetTypeInfoCount(IDataSourceLocator *iface, UINT *pctinfo)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    FIXME("(%p)->()\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI dslocator_GetTypeInfo(IDataSourceLocator *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    FIXME("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI dslocator_GetIDsOfNames(IDataSourceLocator *iface, REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    FIXME("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    return E_NOTIMPL;
}

static HRESULT WINAPI dslocator_Invoke(IDataSourceLocator *iface, DISPID dispIdMember, REFIID riid,
    LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    FIXME("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return E_NOTIMPL;
}

static HRESULT WINAPI dslocator_get_hWnd(IDataSourceLocator *iface, COMPATIBLE_LONG *phwndParent)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    TRACE("(%p)->(%p)\n",This, phwndParent);

    *phwndParent = (COMPATIBLE_LONG)This->hwnd;

    return S_OK;
}

static HRESULT WINAPI dslocator_put_hWnd(IDataSourceLocator *iface, COMPATIBLE_LONG hwndParent)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    TRACE("(%p)->(%p)\n",This, (HWND)hwndParent);

    This->hwnd = (HWND)hwndParent;

    return S_OK;
}

static void create_connections_columns(HWND lv)
{
    RECT rc;
    WCHAR buf[256];
    LVCOLUMNW column;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    GetWindowRect(lv, &rc);
    LoadStringW(instance, IDS_COL_PROVIDER, buf, ARRAY_SIZE(buf));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right - rc.left) - 5;
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
}

static void add_connections_providers(HWND lv)
{
    LONG res;
    HKEY key = NULL, subkey;
    DWORD index = 0;
    LONG next_key;
    WCHAR provider[MAX_PATH];
    WCHAR guidkey[MAX_PATH];
    LONG size;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_READ, &key);
    if (res == ERROR_FILE_NOT_FOUND)
        return;

    next_key = RegEnumKeyW(key, index, provider, MAX_PATH);
    while (next_key == ERROR_SUCCESS)
    {
        WCHAR description[MAX_PATH];

        lstrcpyW(guidkey, provider);
        lstrcatW(guidkey, L"\\OLE DB Provider");

        res = RegOpenKeyW(key, guidkey, &subkey);
        if (res == ERROR_SUCCESS)
        {
            TRACE("Found %s\n", debugstr_w(guidkey));

            size = MAX_PATH;
            res = RegQueryValueW(subkey, NULL, description, &size);
            if (res == ERROR_SUCCESS)
            {
                LVITEMW item;
                struct datasource *data;

                data = create_datasource(guidkey);

                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
                item.iSubItem = 0;
                item.pszText = description;
                item.lParam = (LPARAM)data;

                SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
            }
            RegCloseKey(subkey);
        }

        index++;
        next_key = RegEnumKeyW(key, index, provider, MAX_PATH);
    }

    RegCloseKey(key);
}

static INT_PTR CALLBACK data_link_properties_dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("(%p, %08x, %08Ix, %08Ix)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND btn, lv = GetDlgItem(hwnd, IDC_LST_CONNECTIONS);
            create_connections_columns(lv);
            add_connections_providers(lv);

            btn = GetDlgItem(GetParent(hwnd), IDOK);
            EnableWindow(btn, FALSE);

            break;
        }
        case WM_DESTROY:
        {
            HWND lv = GetDlgItem(hwnd, IDC_LST_CONNECTIONS);
            LVITEMA item;

            item.iItem = 0;
            item.iSubItem = 0;
            item.mask = LVIF_PARAM;

            while(ListView_GetItemA(lv, &item))
            {
                destroy_datasource( (struct datasource *)item.lParam);
                item.iItem++;
            }
            break;
        }
        case WM_NOTIFY:
        {
            NMHDR *hdr = ((LPNMHDR)lp);
            switch(hdr->code)
            {
                case PSN_KILLACTIVE:
                {
                    LVITEMA item;
                    /*
                     * FIXME: This needs to replace the connection page based off the selection.
                     *   We only care about the ODBC for now which is the default.
                     */

                    HWND lv = GetDlgItem(hwnd, IDC_LST_CONNECTIONS);
                    if (!SendMessageW(lv, LVM_GETSELECTEDCOUNT, 0, 0))
                    {
                        WCHAR title[256], msg[256];

                        LoadStringW(instance, IDS_PROVIDER_TITLE, title, ARRAY_SIZE(title));
                        LoadStringW(instance, IDS_PROVIDER_ERROR, msg, ARRAY_SIZE(msg));
                        MessageBoxW(hwnd, msg, title, MB_OK | MB_ICONEXCLAMATION);
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }

                    item.iItem = 0;
                    item.iSubItem = 0;
                    item.stateMask = LVIS_SELECTED;
                    item.mask = LVIF_PARAM | LVIF_STATE;

                    if(ListView_GetItemA(lv, &item))
                    {
                        if(!initialize_datasource( (struct datasource*)item.lParam))
                        {
                            WCHAR title[256], msg[256];
                            LoadStringW(instance, IDS_PROVIDER_TITLE, title, ARRAY_SIZE(title));
                            LoadStringW(instance, IDS_PROVIDER_NOT_AVAIL, msg, ARRAY_SIZE(msg));
                            MessageBoxW(hwnd, msg, title, MB_OK | MB_ICONEXCLAMATION);
                            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                            return TRUE;
                        }
                    }
                    else
                        ERR("Failed to get selected item\n");

                    return FALSE;
                }
            }

            break;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wp) == IDC_BTN_NEXT)
                SendMessageW(GetParent(hwnd), PSM_SETCURSEL, 1, 0);
            break;
        }
        default:
            break;
    }
    return 0;
}

static void connection_fill_odbc_list(HWND parent)
{
    LONG res;
    HKEY key;
    DWORD index = 0;
    WCHAR name[MAX_PATH];
    DWORD nameLen;

    HWND combo = GetDlgItem(parent, IDC_CBO_NAMES);
    if (!combo)
        return;

    res = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ODBC\\ODBC.INI\\ODBC Data Sources", 0, KEY_READ, &key);
    if (res == ERROR_FILE_NOT_FOUND)
        return;

    SendMessageW (combo, CB_RESETCONTENT, 0, 0);

    for(;; index++)
    {
        nameLen = MAX_PATH;
        if (RegEnumValueW(key, index, name, &nameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)name);
    }

    RegCloseKey(key);
}

static void connection_initialize_controls(HWND parent)
{
    HWND hwnd = GetDlgItem(parent, IDC_RDO_SRC_NAME);
    if (hwnd)
        SendMessageA(hwnd, BM_SETCHECK, BST_CHECKED, 0);
}

static void connection_toggle_controls(HWND parent)
{
    BOOL checked = TRUE;
    HWND hwnd = GetDlgItem(parent, IDC_RDO_SRC_NAME);
    if (hwnd)
        checked = SendMessageA(hwnd, BM_GETCHECK, 0, 0);

    EnableWindow(GetDlgItem(parent, IDC_CBO_NAMES), checked);
    EnableWindow(GetDlgItem(parent, IDC_BTN_REFRESH), checked);

    EnableWindow(GetDlgItem(parent, IDC_LBL_CONNECTION), !checked);
    EnableWindow(GetDlgItem(parent, IDC_EDT_CONNECTION), !checked);
    EnableWindow(GetDlgItem(parent, IDC_BTN_BUILD), !checked);
}

static INT_PTR CALLBACK data_link_connection_dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("(%p, %08x, %08Ix, %08Ix)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            connection_initialize_controls(hwnd);
            connection_fill_odbc_list(hwnd);
            connection_toggle_controls(hwnd);
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wp))
            {
                case IDC_RDO_SRC_NAME:
                case IDC_BTN_CONNECTION:
                    connection_toggle_controls(hwnd);
                    break;
                case IDC_BTN_REFRESH:
                    connection_fill_odbc_list(hwnd);
                    break;
                case IDC_BTN_BUILD:
                case IDC_BTN_TEST:
                    /* TODO: Implement dialogs */
                    MessageBoxA(hwnd, "Not implemented yet.", "Error", MB_OK | MB_ICONEXCLAMATION);
                    break;
            }

            break;
        }
        default:
            break;
    }
    return 0;
}

static void advanced_fill_permission_list(HWND parent)
{
    LVITEMW item;
    LVCOLUMNW column;
    RECT rc;
    int resources[] = {IDS_PERM_READ, IDS_PERM_READWRITE, IDS_PERM_SHAREDENYNONE,
                        IDS_PERM_SHAREDENYREAD, IDS_PERM_SHAREDENYWRITE, IDS_PERM_SHAREEXCLUSIVE,
                        IDS_PERM_WRITE};
    int i;
    WCHAR buf[256];
    HWND lv = GetDlgItem(parent, IDC_LST_PERMISSIONS);
    if (!lv)
        return;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_CHECKBOXES);
    GetWindowRect(lv, &rc);
    column.mask = LVCF_WIDTH | LVCF_FMT;
    column.fmt = LVCFMT_FIXED_WIDTH;
    column.cx = (rc.right - rc.left) - 25;
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

    for(i =0; i < ARRAY_SIZE(resources); i++)
    {
        item.mask = LVIF_TEXT;
        item.iItem = SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
        item.iSubItem = 0;
        LoadStringW(instance, resources[i], buf, ARRAY_SIZE(buf));
        item.pszText = buf;
        SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM)&item);
    }
}

static INT_PTR CALLBACK data_link_advanced_dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("(%p, %08x, %08Ix, %08Ix)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            EnableWindow(GetDlgItem(hwnd, IDC_LBL_LEVEL), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CBO_LEVEL), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_LBL_PROTECTION), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_CBO_PROTECTION), FALSE);

            advanced_fill_permission_list(hwnd);

            break;
        }
        default:
            break;
    }
    return 0;
}

static void create_page_all_columns(HWND lv)
{
    RECT rc;
    WCHAR buf[256];
    LVCOLUMNW column;

    SendMessageW(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    GetWindowRect(lv, &rc);
    LoadStringW(instance, IDS_COL_NAME, buf, ARRAY_SIZE(buf));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right / 2);
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);

    LoadStringW(instance, IDS_COL_VALUE, buf, ARRAY_SIZE(buf));
    column.mask = LVCF_WIDTH | LVCF_TEXT;
    column.cx = (rc.right / 2);
    column.pszText = buf;
    SendMessageW(lv, LVM_INSERTCOLUMNW, 0, (LPARAM)&column);
}

static INT_PTR CALLBACK data_link_all_dlg_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    TRACE("(%p, %08x, %08Ix, %08Ix)\n", hwnd, msg, wp, lp);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND lv = GetDlgItem(hwnd, IDC_LST_PROPERTIES);
            create_page_all_columns(lv);
            break;
        }
        case WM_COMMAND:
        {
            if (LOWORD(wp) == IDC_BTN_EDIT)
            {
                 /* TODO: Implement Connection dialog */
                 MessageBoxA(hwnd, "Not implemented yet.", "Error", MB_OK | MB_ICONEXCLAMATION);
            }
         }
     }

     return 0;
 }

static HRESULT WINAPI dslocator_PromptNew(IDataSourceLocator *iface, IDispatch **connection)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);
    PROPSHEETHEADERW hdr;
    PROPSHEETPAGEW pages[4];
    INT_PTR ret;

    FIXME("(%p, %p) Semi-stub\n", iface, connection);

    if(!connection)
        return E_INVALIDARG;

    *connection = NULL;

    memset(&pages, 0, sizeof(pages));

    pages[0].dwSize = sizeof(PROPSHEETPAGEW);
    pages[0].hInstance = instance;
    pages[0].pszTemplate = MAKEINTRESOURCEW(IDD_PROVIDER);
    pages[0].pfnDlgProc = data_link_properties_dlg_proc;

    pages[1].dwSize = sizeof(PROPSHEETPAGEW);
    pages[1].hInstance = instance;
    pages[1].pszTemplate = MAKEINTRESOURCEW(IDD_CONNECTION);
    pages[1].pfnDlgProc = data_link_connection_dlg_proc;

    pages[2].dwSize = sizeof(PROPSHEETPAGEW);
    pages[2].hInstance = instance;
    pages[2].pszTemplate = MAKEINTRESOURCEW(IDD_ADVANCED);
    pages[2].pfnDlgProc = data_link_advanced_dlg_proc;

    pages[3].dwSize = sizeof(pages[0]);
    pages[3].hInstance = instance;
    pages[3].pszTemplate = MAKEINTRESOURCEW(IDD_ALL);
    pages[3].pfnDlgProc = data_link_all_dlg_proc;

    memset(&hdr, 0, sizeof(hdr));
    hdr.dwSize = sizeof(hdr);
    hdr.hwndParent = This->hwnd;
    hdr.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE;
    hdr.hInstance = instance;
    hdr.pszCaption = MAKEINTRESOURCEW(IDS_PROPSHEET_TITLE);
    hdr.ppsp = pages;
    hdr.nPages = ARRAY_SIZE(pages);
    ret = PropertySheetW(&hdr);

    return ret ? S_OK : S_FALSE;
}

static HRESULT WINAPI dslocator_PromptEdit(IDataSourceLocator *iface, IDispatch **ppADOConnection, VARIANT_BOOL *success)
{
    DSLocatorImpl *This = impl_from_IDataSourceLocator(iface);

    FIXME("(%p)->(%p %p)\n",This, ppADOConnection, success);

    return E_NOTIMPL;
}

static const IDataSourceLocatorVtbl DSLocatorVtbl =
{
    dslocator_QueryInterface,
    dslocator_AddRef,
    dslocator_Release,
    dslocator_GetTypeInfoCount,
    dslocator_GetTypeInfo,
    dslocator_GetIDsOfNames,
    dslocator_Invoke,
    dslocator_get_hWnd,
    dslocator_put_hWnd,
    dslocator_PromptNew,
    dslocator_PromptEdit
};

static HRESULT WINAPI datainitialize_QueryInterface(IDataInitialize *iface, REFIID riid, void **obj)
{
    DSLocatorImpl *This = impl_from_IDataInitialize(iface);
    return IDataSourceLocator_QueryInterface(&This->IDataSourceLocator_iface, riid, obj);
}

static ULONG WINAPI datainitialize_AddRef(IDataInitialize *iface)
{
    DSLocatorImpl *This = impl_from_IDataInitialize(iface);
    return IDataSourceLocator_AddRef(&This->IDataSourceLocator_iface);
}

static ULONG WINAPI datainitialize_Release(IDataInitialize *iface)
{
    DSLocatorImpl *This = impl_from_IDataInitialize(iface);
    return IDataSourceLocator_Release(&This->IDataSourceLocator_iface);
}

static HRESULT WINAPI datainitialize_GetDataSource(IDataInitialize *iface,
    IUnknown *outer, DWORD context, LPCOLESTR initstring, REFIID riid, IUnknown **datasource)
{
    TRACE("(%p)->(%p %#lx %s %s %p)\n", iface, outer, context, debugstr_w(initstring), debugstr_guid(riid),
        datasource);

    return get_data_source(outer, context, initstring, riid, datasource);
}

static HRESULT WINAPI datainitialize_GetInitializationString(IDataInitialize *iface, IUnknown *datasource,
    boolean include_password, LPWSTR *initstring)
{
    FIXME("(%p)->(%d %p): stub\n", iface, include_password, initstring);
    return E_NOTIMPL;
}

static HRESULT WINAPI datainitialize_CreateDBInstance(IDataInitialize *iface, REFCLSID prov, IUnknown *outer,
    DWORD clsctx, LPWSTR reserved, REFIID riid, IUnknown **datasource)
{
    FIXME("(%p)->(%s %p %#lx %p %s %p): stub\n", iface, debugstr_guid(prov), outer, clsctx, reserved,
        debugstr_guid(riid), datasource);
    return E_NOTIMPL;
}

static HRESULT WINAPI datainitialize_CreateDBInstanceEx(IDataInitialize *iface, REFCLSID prov, IUnknown *outer,
    DWORD clsctx, LPWSTR reserved, COSERVERINFO *server_info, DWORD cmq, MULTI_QI *results)
{
    FIXME("(%p)->(%s %p %#lx %p %p %lu %p): stub\n", iface, debugstr_guid(prov), outer, clsctx, reserved,
        server_info, cmq, results);
    return E_NOTIMPL;
}

static HRESULT WINAPI datainitialize_LoadStringFromStorage(IDataInitialize *iface, LPCOLESTR filename, LPWSTR *initstring)
{
    FIXME("(%p)->(%s %p): stub\n", iface, debugstr_w(filename), initstring);
    return E_NOTIMPL;
}

static HRESULT WINAPI datainitialize_WriteStringToStorage(IDataInitialize *iface, LPCOLESTR filename, LPCOLESTR initstring,
    DWORD disposition)
{
    FIXME("(%p)->(%s %s %#lx): stub\n", iface, debugstr_w(filename), debugstr_w(initstring), disposition);
    return E_NOTIMPL;
}

static const IDataInitializeVtbl ds_datainitialize_vtbl =
{
    datainitialize_QueryInterface,
    datainitialize_AddRef,
    datainitialize_Release,
    datainitialize_GetDataSource,
    datainitialize_GetInitializationString,
    datainitialize_CreateDBInstance,
    datainitialize_CreateDBInstanceEx,
    datainitialize_LoadStringFromStorage,
    datainitialize_WriteStringToStorage,
};

HRESULT create_dslocator(IUnknown *outer, void **obj)
{
    DSLocatorImpl *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = malloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDataSourceLocator_iface.lpVtbl = &DSLocatorVtbl;
    This->IDataInitialize_iface.lpVtbl = &ds_datainitialize_vtbl;
    This->ref = 1;
    This->hwnd = 0;

    *obj = &This->IDataSourceLocator_iface;

    return S_OK;
}
