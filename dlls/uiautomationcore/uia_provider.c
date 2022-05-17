/*
 * Copyright 2022 Connor McAdams for CodeWeavers
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

#include "uiautomation.h"
#include "ocidl.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

static void variant_init_i4(VARIANT *v, int val)
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

static LONG msaa_role_to_uia_control_type(LONG role)
{
    switch (role)
    {
    case ROLE_SYSTEM_TITLEBAR:           return UIA_TitleBarControlTypeId;
    case ROLE_SYSTEM_MENUBAR:            return UIA_MenuBarControlTypeId;
    case ROLE_SYSTEM_SCROLLBAR:          return UIA_ScrollBarControlTypeId;
    case ROLE_SYSTEM_INDICATOR:
    case ROLE_SYSTEM_GRIP:               return UIA_ThumbControlTypeId;
    case ROLE_SYSTEM_APPLICATION:
    case ROLE_SYSTEM_WINDOW:             return UIA_WindowControlTypeId;
    case ROLE_SYSTEM_MENUPOPUP:          return UIA_MenuControlTypeId;
    case ROLE_SYSTEM_TOOLTIP:            return UIA_ToolTipControlTypeId;
    case ROLE_SYSTEM_DOCUMENT:           return UIA_DocumentControlTypeId;
    case ROLE_SYSTEM_PANE:               return UIA_PaneControlTypeId;
    case ROLE_SYSTEM_GROUPING:           return UIA_GroupControlTypeId;
    case ROLE_SYSTEM_SEPARATOR:          return UIA_SeparatorControlTypeId;
    case ROLE_SYSTEM_TOOLBAR:            return UIA_ToolBarControlTypeId;
    case ROLE_SYSTEM_STATUSBAR:          return UIA_StatusBarControlTypeId;
    case ROLE_SYSTEM_TABLE:              return UIA_TableControlTypeId;
    case ROLE_SYSTEM_COLUMNHEADER:
    case ROLE_SYSTEM_ROWHEADER:          return UIA_HeaderControlTypeId;
    case ROLE_SYSTEM_CELL:               return UIA_DataItemControlTypeId;
    case ROLE_SYSTEM_LINK:               return UIA_HyperlinkControlTypeId;
    case ROLE_SYSTEM_LIST:               return UIA_ListControlTypeId;
    case ROLE_SYSTEM_LISTITEM:           return UIA_ListItemControlTypeId;
    case ROLE_SYSTEM_OUTLINE:            return UIA_TreeControlTypeId;
    case ROLE_SYSTEM_OUTLINEITEM:        return UIA_TreeItemControlTypeId;
    case ROLE_SYSTEM_PAGETAB:            return UIA_TabItemControlTypeId;
    case ROLE_SYSTEM_GRAPHIC:            return UIA_ImageControlTypeId;
    case ROLE_SYSTEM_STATICTEXT:         return UIA_TextControlTypeId;
    case ROLE_SYSTEM_TEXT:               return UIA_EditControlTypeId;
    case ROLE_SYSTEM_CLOCK:
    case ROLE_SYSTEM_BUTTONDROPDOWNGRID:
    case ROLE_SYSTEM_PUSHBUTTON:         return UIA_ButtonControlTypeId;
    case ROLE_SYSTEM_CHECKBUTTON:        return UIA_CheckBoxControlTypeId;
    case ROLE_SYSTEM_RADIOBUTTON:        return UIA_RadioButtonControlTypeId;
    case ROLE_SYSTEM_COMBOBOX:           return UIA_ComboBoxControlTypeId;
    case ROLE_SYSTEM_PROGRESSBAR:        return UIA_ProgressBarControlTypeId;
    case ROLE_SYSTEM_SLIDER:             return UIA_SliderControlTypeId;
    case ROLE_SYSTEM_SPINBUTTON:         return UIA_SpinnerControlTypeId;
    case ROLE_SYSTEM_BUTTONMENU:
    case ROLE_SYSTEM_MENUITEM:           return UIA_MenuItemControlTypeId;
    case ROLE_SYSTEM_PAGETABLIST:        return UIA_TabControlTypeId;
    case ROLE_SYSTEM_BUTTONDROPDOWN:
    case ROLE_SYSTEM_SPLITBUTTON:        return UIA_SplitButtonControlTypeId;
    case ROLE_SYSTEM_SOUND:
    case ROLE_SYSTEM_CURSOR:
    case ROLE_SYSTEM_CARET:
    case ROLE_SYSTEM_ALERT:
    case ROLE_SYSTEM_CLIENT:
    case ROLE_SYSTEM_CHART:
    case ROLE_SYSTEM_DIALOG:
    case ROLE_SYSTEM_BORDER:
    case ROLE_SYSTEM_COLUMN:
    case ROLE_SYSTEM_ROW:
    case ROLE_SYSTEM_HELPBALLOON:
    case ROLE_SYSTEM_CHARACTER:
    case ROLE_SYSTEM_PROPERTYPAGE:
    case ROLE_SYSTEM_DROPLIST:
    case ROLE_SYSTEM_DIAL:
    case ROLE_SYSTEM_HOTKEYFIELD:
    case ROLE_SYSTEM_DIAGRAM:
    case ROLE_SYSTEM_ANIMATION:
    case ROLE_SYSTEM_EQUATION:
    case ROLE_SYSTEM_WHITESPACE:
    case ROLE_SYSTEM_IPADDRESS:
    case ROLE_SYSTEM_OUTLINEBUTTON:
        WARN("No UIA control type mapping for MSAA role %ld\n", role);
        break;

    default:
        FIXME("UIA control type mapping unimplemented for MSAA role %ld\n", role);
        break;
    }

    return 0;
}

/*
 * UiaProviderFromIAccessible IRawElementProviderSimple interface.
 */
struct msaa_provider {
    IRawElementProviderSimple IRawElementProviderSimple_iface;
    LONG refcount;

    IAccessible *acc;
    VARIANT cid;
    HWND hwnd;
    LONG control_type;
};

static inline struct msaa_provider *impl_from_msaa_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI msaa_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IRawElementProviderSimple_AddRef(iface);
    return S_OK;
}

ULONG WINAPI msaa_provider_AddRef(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedIncrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

ULONG WINAPI msaa_provider_Release(IRawElementProviderSimple *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    ULONG refcount = InterlockedDecrement(&msaa_prov->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
    {
        IAccessible_Release(msaa_prov->acc);
        heap_free(msaa_prov);
    }

    return refcount;
}

HRESULT WINAPI msaa_provider_get_ProviderOptions(IRawElementProviderSimple *iface,
        enum ProviderOptions *ret_val)
{
    TRACE("%p, %p\n", iface, ret_val);
    *ret_val = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
    return S_OK;
}

HRESULT WINAPI msaa_provider_GetPatternProvider(IRawElementProviderSimple *iface,
        PATTERNID pattern_id, IUnknown **ret_val)
{
    FIXME("%p, %d, %p: stub!\n", iface, pattern_id, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI msaa_provider_GetPropertyValue(IRawElementProviderSimple *iface,
        PROPERTYID prop_id, VARIANT *ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %d, %p\n", iface, prop_id, ret_val);

    VariantInit(ret_val);
    VariantInit(&v);
    switch (prop_id)
    {
    case UIA_ProviderDescriptionPropertyId:
        V_VT(ret_val) = VT_BSTR;
        V_BSTR(ret_val) = SysAllocString(L"Wine: MSAA Proxy");
        break;

    case UIA_ControlTypePropertyId:
        if (!msaa_prov->control_type)
        {
            hr = IAccessible_get_accRole(msaa_prov->acc, msaa_prov->cid, &v);
            if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
                msaa_prov->control_type = msaa_role_to_uia_control_type(V_I4(&v));
        }

        if (msaa_prov->control_type)
            variant_init_i4(ret_val, msaa_prov->control_type);

        break;

    default:
        FIXME("Unimplemented propertyId %d\n", prop_id);
        break;
    }

    return S_OK;
}

HRESULT WINAPI msaa_provider_get_HostRawElementProvider(IRawElementProviderSimple *iface,
        IRawElementProviderSimple **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static const IRawElementProviderSimpleVtbl msaa_provider_vtbl = {
    msaa_provider_QueryInterface,
    msaa_provider_AddRef,
    msaa_provider_Release,
    msaa_provider_get_ProviderOptions,
    msaa_provider_GetPatternProvider,
    msaa_provider_GetPropertyValue,
    msaa_provider_get_HostRawElementProvider,
};

/***********************************************************************
 *          UiaProviderFromIAccessible (uiautomationcore.@)
 */
HRESULT WINAPI UiaProviderFromIAccessible(IAccessible *acc, long child_id, DWORD flags,
        IRawElementProviderSimple **elprov)
{
    struct msaa_provider *msaa_prov;
    IServiceProvider *serv_prov;
    HWND hwnd = NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %#lx, %p)\n", acc, child_id, flags, elprov);

    if (elprov)
        *elprov = NULL;

    if (!elprov)
        return E_POINTER;
    if (!acc)
        return E_INVALIDARG;

    if (flags != UIA_PFIA_DEFAULT)
    {
        FIXME("unsupported flags %#lx\n", flags);
        return E_NOTIMPL;
    }

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&serv_prov);
    if (SUCCEEDED(hr))
    {
        IUnknown *unk;

        hr = IServiceProvider_QueryService(serv_prov, &IIS_IsOleaccProxy, &IID_IUnknown, (void **)&unk);
        if (SUCCEEDED(hr))
        {
            WARN("Cannot wrap an oleacc proxy IAccessible!\n");
            IUnknown_Release(unk);
            IServiceProvider_Release(serv_prov);
            return E_INVALIDARG;
        }

        IServiceProvider_Release(serv_prov);
    }

    hr = WindowFromAccessibleObject(acc, &hwnd);
    if (FAILED(hr))
       return hr;
    if (!hwnd)
        return E_FAIL;

    msaa_prov = heap_alloc_zero(sizeof(*msaa_prov));
    if (!msaa_prov)
        return E_OUTOFMEMORY;

    msaa_prov->IRawElementProviderSimple_iface.lpVtbl = &msaa_provider_vtbl;
    msaa_prov->refcount = 1;
    msaa_prov->hwnd = hwnd;
    variant_init_i4(&msaa_prov->cid, child_id);
    msaa_prov->acc = acc;
    IAccessible_AddRef(acc);
    *elprov = &msaa_prov->IRawElementProviderSimple_iface;

    return S_OK;
}
