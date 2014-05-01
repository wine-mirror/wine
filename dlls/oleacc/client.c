/*
 * Copyright 2014 Piotr Caban for CodeWeavers
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

#define COBJMACROS

#include "oleacc_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

typedef struct {
    IAccessible IAccessible_iface;

    LONG ref;

    HWND hwnd;
} Client;

static inline Client* impl_from_Client(IAccessible *iface)
{
    return CONTAINING_RECORD(iface, Client, IAccessible_iface);
}

static HRESULT WINAPI Client_QueryInterface(IAccessible *iface, REFIID riid, void **ppv)
{
    Client *This = impl_from_Client(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualIID(riid, &IID_IAccessible) ||
            IsEqualIID(riid, &IID_IDispatch) ||
            IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        IAccessible_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI Client_AddRef(IAccessible *iface)
{
    Client *This = impl_from_Client(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI Client_Release(IAccessible *iface)
{
    Client *This = impl_from_Client(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);

    if(!ref)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI Client_GetTypeInfoCount(IAccessible *iface, UINT *pctinfo)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_GetTypeInfo(IAccessible *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%u %x %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_GetIDsOfNames(IAccessible *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p %u %x %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_Invoke(IAccessible *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%x %s %x %x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accParent(IAccessible *iface, IDispatch **ppdispParent)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, ppdispParent);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accChildCount(IAccessible *iface, LONG *pcountChildren)
{
    Client *This = impl_from_Client(iface);
    HWND cur;

    TRACE("(%p)->(%p)\n", This, pcountChildren);

    *pcountChildren = 0;
    for(cur = GetWindow(This->hwnd, GW_CHILD); cur; cur = GetWindow(cur, GW_HWNDNEXT))
        (*pcountChildren)++;

    return S_OK;
}

static HRESULT WINAPI Client_get_accChild(IAccessible *iface,
        VARIANT varChildID, IDispatch **ppdispChild)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varChildID), ppdispChild);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accName(IAccessible *iface, VARIANT varID, BSTR *pszName)
{
    Client *This = impl_from_Client(iface);
    WCHAR name[1024];
    UINT i, len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszName);

    *pszName = NULL;
    if(convert_child_id(&varID) != CHILDID_SELF || !IsWindow(This->hwnd))
        return E_INVALIDARG;

    len = SendMessageW(This->hwnd, WM_GETTEXT, sizeof(name)/sizeof(WCHAR), (LPARAM)name);
    if(!len)
        return S_FALSE;

    for(i=0; i<len; i++) {
        if(name[i] == '&') {
            len--;
            memmove(name+i, name+i+1, (len-i)*sizeof(WCHAR));
            break;
        }
    }

    *pszName = SysAllocStringLen(name, len);
    return *pszName ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI Client_get_accValue(IAccessible *iface, VARIANT varID, BSTR *pszValue)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accDescription(IAccessible *iface,
        VARIANT varID, BSTR *pszDescription)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDescription);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accRole(IAccessible *iface, VARIANT varID, VARIANT *pvarRole)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarRole);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accState(IAccessible *iface, VARIANT varID, VARIANT *pvarState)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pvarState);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accHelp(IAccessible *iface, VARIANT varID, BSTR *pszHelp)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszHelp);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accHelpTopic(IAccessible *iface,
        BSTR *pszHelpFile, VARIANT varID, LONG *pidTopic)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p %s %p)\n", This, pszHelpFile, debugstr_variant(&varID), pidTopic);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accKeyboardShortcut(IAccessible *iface,
        VARIANT varID, BSTR *pszKeyboardShortcut)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszKeyboardShortcut);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accFocus(IAccessible *iface, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accSelection(IAccessible *iface, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p)\n", This, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_get_accDefaultAction(IAccessible *iface,
        VARIANT varID, BSTR *pszDefaultAction)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_variant(&varID), pszDefaultAction);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accSelect(IAccessible *iface, LONG flagsSelect, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%x %s)\n", This, flagsSelect, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accLocation(IAccessible *iface, LONG *pxLeft,
        LONG *pyTop, LONG *pcxWidth, LONG *pcyHeight, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%p %p %p %p %s)\n", This, pxLeft, pyTop,
            pcxWidth, pcyHeight, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accNavigate(IAccessible *iface,
        LONG navDir, VARIANT varStart, VARIANT *pvarEnd)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%d %s %p)\n", This, navDir, debugstr_variant(&varStart), pvarEnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accHitTest(IAccessible *iface,
        LONG xLeft, LONG yTop, VARIANT *pvarID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%d %d %p)\n", This, xLeft, yTop, pvarID);
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_accDoDefaultAction(IAccessible *iface, VARIANT varID)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&varID));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_put_accName(IAccessible *iface, VARIANT varID, BSTR pszName)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI Client_put_accValue(IAccessible *iface, VARIANT varID, BSTR pszValue)
{
    Client *This = impl_from_Client(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_variant(&varID), debugstr_w(pszValue));
    return E_NOTIMPL;
}

static const IAccessibleVtbl ClientVtbl = {
    Client_QueryInterface,
    Client_AddRef,
    Client_Release,
    Client_GetTypeInfoCount,
    Client_GetTypeInfo,
    Client_GetIDsOfNames,
    Client_Invoke,
    Client_get_accParent,
    Client_get_accChildCount,
    Client_get_accChild,
    Client_get_accName,
    Client_get_accValue,
    Client_get_accDescription,
    Client_get_accRole,
    Client_get_accState,
    Client_get_accHelp,
    Client_get_accHelpTopic,
    Client_get_accKeyboardShortcut,
    Client_get_accFocus,
    Client_get_accSelection,
    Client_get_accDefaultAction,
    Client_accSelect,
    Client_accLocation,
    Client_accNavigate,
    Client_accHitTest,
    Client_accDoDefaultAction,
    Client_put_accName,
    Client_put_accValue
};

HRESULT create_client_object(HWND hwnd, const IID *iid, void **obj)
{
    Client *client;
    HRESULT hres;

    if(!IsWindow(hwnd))
        return E_FAIL;

    client = heap_alloc_zero(sizeof(Client));
    if(!client)
        return E_OUTOFMEMORY;

    client->IAccessible_iface.lpVtbl = &ClientVtbl;
    client->ref = 1;
    client->hwnd = hwnd;

    hres = IAccessible_QueryInterface(&client->IAccessible_iface, iid, obj);
    IAccessible_Release(&client->IAccessible_iface);
    return hres;
}
