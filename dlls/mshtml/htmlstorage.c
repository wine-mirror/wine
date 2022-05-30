/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlobj.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    DispatchEx dispex;
    IHTMLStorage IHTMLStorage_iface;
    LONG ref;
    WCHAR *filename;
    HANDLE mutex;
} HTMLStorage;

static inline HTMLStorage *impl_from_IHTMLStorage(IHTMLStorage *iface)
{
    return CONTAINING_RECORD(iface, HTMLStorage, IHTMLStorage_iface);
}

static HRESULT WINAPI HTMLStorage_QueryInterface(IHTMLStorage *iface, REFIID riid, void **ppv)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStorage_iface;
    }else if(IsEqualGUID(&IID_IHTMLStorage, riid)) {
        *ppv = &This->IHTMLStorage_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStorage_AddRef(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStorage_Release(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This->filename);
        CloseHandle(This->mutex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStorage_GetTypeInfoCount(IHTMLStorage *iface, UINT *pctinfo)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStorage_GetTypeInfo(IHTMLStorage *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStorage_GetIDsOfNames(IHTMLStorage *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStorage_Invoke(IHTMLStorage *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static BOOL create_path(const WCHAR *path)
{
    BOOL ret = TRUE;
    WCHAR *new_path;
    int len;

    new_path = heap_alloc((wcslen(path) + 1) * sizeof(WCHAR));
    if(!new_path)
        return FALSE;
    wcscpy(new_path, path);

    while((len = wcslen(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;

    while(!CreateDirectoryW(new_path, NULL)) {
        WCHAR *slash;
        DWORD error = GetLastError();
        if(error == ERROR_ALREADY_EXISTS) break;
        if(error != ERROR_PATH_NOT_FOUND) {
            ret = FALSE;
            break;
        }
        slash = wcsrchr(new_path, '\\');
        if(!slash) {
            ret = FALSE;
            break;
        }
        len = slash - new_path;
        new_path[len] = 0;
        if(!create_path(new_path)) {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }
    heap_free(new_path);
    return ret;
}

static HRESULT open_document(const WCHAR *filename, IXMLDOMDocument **ret)
{
    IXMLDOMDocument *doc = NULL;
    HRESULT hres = E_OUTOFMEMORY;
    WCHAR *ptr, *path;
    VARIANT var;
    VARIANT_BOOL success;

    path = heap_strdupW(filename);
    if(!path)
        return E_OUTOFMEMORY;

    *(ptr = wcsrchr(path, '\\')) = 0;
    if(!create_path(path))
        goto done;

    if(GetFileAttributesW(filename) == INVALID_FILE_ATTRIBUTES) {
        DWORD count;
        HANDLE file = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
        if(file == INVALID_HANDLE_VALUE) {
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
        if(!WriteFile(file, "<root/>", sizeof("<root/>") - 1, &count, NULL)) {
            CloseHandle(file);
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
        CloseHandle(file);
    }

    hres = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void**)&doc);
    if(hres != S_OK)
        goto done;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(filename);
    if(!V_BSTR(&var)) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMDocument_load(doc, var, &success);
    if(hres == S_FALSE || success == VARIANT_FALSE)
        hres = E_FAIL;

    SysFreeString(V_BSTR(&var));

done:
    heap_free(path);
    if(hres == S_OK)
        *ret = doc;
    else if(doc)
        IXMLDOMDocument_Release(doc);

    return hres;
}

static HRESULT WINAPI HTMLStorage_get_length(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_get_remainingSpace(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_key(IHTMLStorage *iface, LONG lIndex, BSTR *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%ld %p)\n", This, lIndex, p);
    return E_NOTIMPL;
}

static BSTR build_query(const WCHAR *key)
{
    static const WCHAR fmt[] = L"item[@name='%s']";
    const WCHAR *str = key ? key : L"";
    UINT len = ARRAY_SIZE(fmt) + wcslen(str);
    BSTR ret = SysAllocStringLen(NULL, len);

    if(ret) swprintf(ret, len, fmt, str);
    return ret;
}

static HRESULT get_root_node(IXMLDOMDocument *doc, IXMLDOMNode **root)
{
    HRESULT hres;
    BSTR str;

    str = SysAllocString(L"root");
    if(!str)
        return E_OUTOFMEMORY;

    hres = IXMLDOMDocument_selectSingleNode(doc, str, root);
    SysFreeString(str);
    return hres;
}

static HRESULT get_item(const WCHAR *filename, BSTR key, VARIANT *value)
{
    IXMLDOMDocument *doc;
    BSTR query = NULL;
    IXMLDOMNode *root = NULL, *node = NULL;
    IXMLDOMElement *elem = NULL;
    HRESULT hres;

    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
        if(hres != S_OK)
            goto done;

        hres = IXMLDOMElement_getAttribute(elem, (BSTR)L"value", value);
    }else {
        V_VT(value) = VT_NULL;
        hres = S_OK;
    }

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    if(elem)
        IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    return hres;
}

static HRESULT WINAPI HTMLStorage_getItem(IHTMLStorage *iface, BSTR bstrKey, VARIANT *value)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrKey), value);

    if(!value)
        return E_POINTER;

    if(!This->filename) {
        FIXME("session storage not supported\n");
        V_VT(value) = VT_NULL;
        return S_OK;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = get_item(This->filename, bstrKey, value);
    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT set_attribute(IXMLDOMElement *elem, const WCHAR *name, BSTR value)
{
    BSTR str;
    VARIANT var;
    HRESULT hres;

    str = SysAllocString(name);
    if(!str)
        return E_OUTOFMEMORY;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = value;

    hres = IXMLDOMElement_setAttribute(elem, str, var);
    SysFreeString(str);
    return hres;
}

static HRESULT save_document(IXMLDOMDocument *doc, const WCHAR *filename)
{
    VARIANT var;
    HRESULT hres;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(filename);
    if(!V_BSTR(&var))
        return E_OUTOFMEMORY;

    hres = IXMLDOMDocument_save(doc, var);
    SysFreeString(V_BSTR(&var));
    return hres;
}

static HRESULT set_item(const WCHAR *filename, BSTR key, BSTR value)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *root = NULL, *node = NULL;
    IXMLDOMElement *elem = NULL;
    BSTR query = NULL;
    HRESULT hres;

    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        hres = IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)&elem);
        if(hres != S_OK)
            goto done;

        hres = set_attribute(elem, L"value", value);
        if(hres != S_OK)
            goto done;
    }else {
        BSTR str = SysAllocString(L"item");
        hres = IXMLDOMDocument_createElement(doc, str, &elem);
        SysFreeString(str);
        if(hres != S_OK)
            goto done;

        hres = set_attribute(elem, L"name", key);
        if(hres != S_OK)
            goto done;

        hres = set_attribute(elem, L"value", value);
        if(hres != S_OK)
            goto done;

        hres = IXMLDOMNode_appendChild(root, (IXMLDOMNode*)elem, NULL);
        if(hres != S_OK)
            goto done;
    }

    hres = save_document(doc, filename);

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    if(elem)
        IXMLDOMElement_Release(elem);
    IXMLDOMDocument_Release(doc);
    return hres;
}

static HRESULT WINAPI HTMLStorage_setItem(IHTMLStorage *iface, BSTR bstrKey, BSTR bstrValue)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    HRESULT hres;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(bstrKey), debugstr_w(bstrValue));

    if(!This->filename) {
        FIXME("session storage not supported\n");
        return E_NOTIMPL;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = set_item(This->filename, bstrKey, bstrValue);
    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT remove_item(const WCHAR *filename, BSTR key)
{
    IXMLDOMDocument *doc;
    IXMLDOMNode *root = NULL, *node = NULL;
    BSTR query = NULL;
    HRESULT hres;

    hres = open_document(filename, &doc);
    if(hres != S_OK)
        return hres;

    hres = get_root_node(doc, &root);
    if(hres != S_OK)
        goto done;

    query = build_query(key);
    if(!query) {
        hres = E_OUTOFMEMORY;
        goto done;
    }

    hres = IXMLDOMNode_selectSingleNode(root, query, &node);
    if(hres == S_OK) {
        hres = IXMLDOMNode_removeChild(root, node, NULL);
        if(hres != S_OK)
            goto done;
    }

    hres = save_document(doc, filename);

done:
    SysFreeString(query);
    if(root)
        IXMLDOMNode_Release(root);
    if(node)
        IXMLDOMNode_Release(node);
    IXMLDOMDocument_Release(doc);
    return hres;
}

static HRESULT WINAPI HTMLStorage_removeItem(IHTMLStorage *iface, BSTR bstrKey)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(bstrKey));

    if(!This->filename) {
        FIXME("session storage not supported\n");
        return E_NOTIMPL;
    }

    WaitForSingleObject(This->mutex, INFINITE);
    hres = remove_item(This->filename, bstrKey);
    ReleaseMutex(This->mutex);

    return hres;
}

static HRESULT WINAPI HTMLStorage_clear(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IHTMLStorageVtbl HTMLStorageVtbl = {
    HTMLStorage_QueryInterface,
    HTMLStorage_AddRef,
    HTMLStorage_Release,
    HTMLStorage_GetTypeInfoCount,
    HTMLStorage_GetTypeInfo,
    HTMLStorage_GetIDsOfNames,
    HTMLStorage_Invoke,
    HTMLStorage_get_length,
    HTMLStorage_get_remainingSpace,
    HTMLStorage_key,
    HTMLStorage_getItem,
    HTMLStorage_setItem,
    HTMLStorage_removeItem,
    HTMLStorage_clear
};

static const tid_t HTMLStorage_iface_tids[] = {
    IHTMLStorage_tid,
    0
};
static dispex_static_data_t HTMLStorage_dispex = {
    L"Storage",
    NULL,
    IHTMLStorage_tid,
    HTMLStorage_iface_tids
};

static WCHAR *build_filename(IUri *uri)
{
    static const WCHAR store[] = L"\\Microsoft\\Internet Explorer\\DOMStore\\";
    WCHAR path[MAX_PATH], *ret;
    BSTR hostname;
    HRESULT hres;
    int len;

    hres = IUri_GetHost(uri, &hostname);
    if(hres != S_OK)
        return NULL;

    if(!SHGetSpecialFolderPathW(NULL, path, CSIDL_LOCAL_APPDATA, TRUE)) {
        ERR("Can't get folder path %lu\n", GetLastError());
        SysFreeString(hostname);
        return NULL;
    }

    len = wcslen(path);
    if(len + ARRAY_SIZE(store) > ARRAY_SIZE(path)) {
        ERR("Path too long\n");
        SysFreeString(hostname);
        return NULL;
    }
    memcpy(path + len, store, sizeof(store));

    len += ARRAY_SIZE(store);
    ret = heap_alloc((len + wcslen(hostname) + ARRAY_SIZE(L".xml")) * sizeof(WCHAR));
    if(!ret) {
        SysFreeString(hostname);
        return NULL;
    }

    wcscpy(ret, path);
    wcscat(ret, hostname);
    wcscat(ret, L".xml");

    SysFreeString(hostname);
    return ret;
}

static WCHAR *build_mutexname(const WCHAR *filename)
{
    WCHAR *ret, *ptr;
    ret = heap_strdupW(filename);
    if(!ret)
        return NULL;
    for(ptr = ret; *ptr; ptr++)
        if(*ptr == '\\') *ptr = '_';
    return ret;
}

HRESULT create_html_storage(compat_mode_t compat_mode, IUri *uri, IHTMLStorage **p)
{
    HTMLStorage *storage;

    storage = heap_alloc_zero(sizeof(*storage));
    if(!storage)
        return E_OUTOFMEMORY;

    if(uri) {
        WCHAR *mutexname;
        if(!(storage->filename = build_filename(uri))) {
            heap_free(storage);
            return E_OUTOFMEMORY;
        }
        mutexname = build_mutexname(storage->filename);
        if(!mutexname) {
            heap_free(storage->filename);
            heap_free(storage);
            return E_OUTOFMEMORY;
        }
        storage->mutex = CreateMutexW(NULL, FALSE, mutexname);
        heap_free(mutexname);
        if(!storage->mutex) {
            heap_free(storage->filename);
            heap_free(storage);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    storage->IHTMLStorage_iface.lpVtbl = &HTMLStorageVtbl;
    storage->ref = 1;
    init_dispatch(&storage->dispex, (IUnknown*)&storage->IHTMLStorage_iface, &HTMLStorage_dispex, compat_mode);

    *p = &storage->IHTMLStorage_iface;
    return S_OK;
}
