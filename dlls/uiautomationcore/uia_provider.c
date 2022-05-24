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
#include "initguid.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

DEFINE_GUID(SID_AccFromDAWrapper, 0x33f139ee, 0xe509, 0x47f7, 0xbf,0x39, 0x83,0x76,0x44,0xf7,0x45,0x76);

static void variant_init_i4(VARIANT *v, int val)
{
    V_VT(v) = VT_I4;
    V_I4(v) = val;
}

static void variant_init_bool(VARIANT *v, BOOL val)
{
    V_VT(v) = VT_BOOL;
    V_BOOL(v) = val ? VARIANT_TRUE : VARIANT_FALSE;
}

static BOOL msaa_check_acc_state(IAccessible *acc, VARIANT cid, ULONG flag)
{
    HRESULT hr;
    VARIANT v;

    VariantInit(&v);
    hr = IAccessible_get_accState(acc, cid, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_I4 && (V_I4(&v) & flag))
        return TRUE;

    return FALSE;
}

static IAccessible *msaa_acc_da_unwrap(IAccessible *acc)
{
    IServiceProvider *sp;
    IAccessible *acc2;
    HRESULT hr;

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void**)&sp);
    if (SUCCEEDED(hr))
    {
        hr = IServiceProvider_QueryService(sp, &SID_AccFromDAWrapper, &IID_IAccessible, (void**)&acc2);
        IServiceProvider_Release(sp);
    }

    if (SUCCEEDED(hr) && acc2)
        return acc2;

    IAccessible_AddRef(acc);
    return acc;
}

/*
 * Compare role, state, child count, and location properties of the two
 * IAccessibles. If all four are successfully retrieved and are equal, this is
 * considered a match.
 */
static HRESULT msaa_acc_prop_match(IAccessible *acc, IAccessible *acc2)
{
    BOOL role_match, state_match, child_count_match, location_match;
    LONG child_count[2], left[2], top[2], width[2], height[2];
    VARIANT cid, v, v2;
    HRESULT hr, hr2;

    role_match = state_match = child_count_match = location_match = FALSE;
    variant_init_i4(&cid, CHILDID_SELF);
    hr = IAccessible_get_accRole(acc, cid, &v);
    if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
    {
        VariantInit(&v2);
        hr = IAccessible_get_accRole(acc2, cid, &v2);
        if (SUCCEEDED(hr) && (V_VT(&v2) == VT_I4))
        {
            if (V_I4(&v) != V_I4(&v2))
                return E_FAIL;

            role_match = TRUE;
        }
    }

    VariantInit(&v);
    hr = IAccessible_get_accState(acc, cid, &v);
    if (SUCCEEDED(hr) && (V_VT(&v) == VT_I4))
    {
        VariantInit(&v2);
        hr = IAccessible_get_accState(acc2, cid, &v2);
        if (SUCCEEDED(hr) && (V_VT(&v2) == VT_I4))
        {
            if (V_I4(&v) != V_I4(&v2))
                return E_FAIL;

            state_match = TRUE;
        }
    }

    hr = IAccessible_get_accChildCount(acc, &child_count[0]);
    hr2 = IAccessible_get_accChildCount(acc2, &child_count[1]);
    if (SUCCEEDED(hr) && SUCCEEDED(hr2))
    {
        if (child_count[0] != child_count[1])
            return E_FAIL;

        child_count_match = TRUE;
    }

    hr = IAccessible_accLocation(acc, &left[0], &top[0], &width[0], &height[0], cid);
    if (SUCCEEDED(hr))
    {
        hr = IAccessible_accLocation(acc2, &left[1], &top[1], &width[1], &height[1], cid);
        if (SUCCEEDED(hr))
        {
            if ((left[0] != left[1]) || (top[0] != top[1]) || (width[0] != width[1]) ||
                    (height[0] != height[1]))
                return E_FAIL;

            location_match = TRUE;
        }
    }

    if (role_match && state_match && child_count_match && location_match)
        return S_OK;

    return S_FALSE;
}

static BOOL msaa_acc_compare(IAccessible *acc, IAccessible *acc2)
{
    IUnknown *unk, *unk2;
    BOOL matched = FALSE;
    BSTR name[2];
    VARIANT cid;
    HRESULT hr;

    acc = msaa_acc_da_unwrap(acc);
    acc2 = msaa_acc_da_unwrap(acc2);
    IAccessible_QueryInterface(acc, &IID_IUnknown, (void**)&unk);
    IAccessible_QueryInterface(acc2, &IID_IUnknown, (void**)&unk2);
    if (unk == unk2)
    {
        matched = TRUE;
        goto exit;
    }

    hr = msaa_acc_prop_match(acc, acc2);
    if (FAILED(hr))
        goto exit;
    if (hr == S_OK)
        matched = TRUE;

    variant_init_i4(&cid, CHILDID_SELF);
    hr = IAccessible_get_accName(acc, cid, &name[0]);
    if (SUCCEEDED(hr))
    {
        hr = IAccessible_get_accName(acc2, cid, &name[1]);
        if (SUCCEEDED(hr))
        {
            if (!name[0] && !name[1])
                matched = TRUE;
            else if (!name[0] || !name[1])
                matched = FALSE;
            else
            {
                if (!wcscmp(name[0], name[1]))
                    matched = TRUE;
                else
                    matched = FALSE;
            }

            SysFreeString(name[1]);
        }

        SysFreeString(name[0]);
    }

exit:
    IUnknown_Release(unk);
    IUnknown_Release(unk2);
    IAccessible_Release(acc);
    IAccessible_Release(acc2);

    return matched;
}

static HRESULT msaa_acc_get_parent(IAccessible *acc, IAccessible **parent)
{
    IDispatch *disp = NULL;
    HRESULT hr;

    *parent = NULL;
    hr = IAccessible_get_accParent(acc, &disp);
    if (FAILED(hr) || !disp)
        return hr;

    hr = IDispatch_QueryInterface(disp, &IID_IAccessible, (void**)parent);
    IDispatch_Release(disp);
    return hr;
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
    IRawElementProviderFragment IRawElementProviderFragment_iface;
    ILegacyIAccessibleProvider ILegacyIAccessibleProvider_iface;
    LONG refcount;

    IAccessible *acc;
    VARIANT cid;
    HWND hwnd;
    LONG control_type;

    BOOL root_acc_check_ran;
    BOOL is_root_acc;
};

static BOOL msaa_check_root_acc(struct msaa_provider *msaa_prov)
{
    IAccessible *acc;
    HRESULT hr;

    if (msaa_prov->root_acc_check_ran)
        return msaa_prov->is_root_acc;

    msaa_prov->root_acc_check_ran = TRUE;
    if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
        return FALSE;

    hr = AccessibleObjectFromWindow(msaa_prov->hwnd, OBJID_CLIENT, &IID_IAccessible, (void **)&acc);
    if (FAILED(hr))
        return FALSE;

    if (msaa_acc_compare(msaa_prov->acc, acc))
        msaa_prov->is_root_acc = TRUE;

    IAccessible_Release(acc);
    return msaa_prov->is_root_acc;
}

static inline struct msaa_provider *impl_from_msaa_provider(IRawElementProviderSimple *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderSimple_iface);
}

HRESULT WINAPI msaa_provider_QueryInterface(IRawElementProviderSimple *iface, REFIID riid, void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IRawElementProviderSimple) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IRawElementProviderFragment))
        *ppv = &msaa_prov->IRawElementProviderFragment_iface;
    else if (IsEqualIID(riid, &IID_ILegacyIAccessibleProvider))
        *ppv = &msaa_prov->ILegacyIAccessibleProvider_iface;
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
    TRACE("%p, %d, %p\n", iface, pattern_id, ret_val);

    *ret_val = NULL;
    switch (pattern_id)
    {
    case UIA_LegacyIAccessiblePatternId:
        return IRawElementProviderSimple_QueryInterface(iface, &IID_IUnknown, (void **)ret_val);

    default:
        FIXME("Unimplemented patternId %d\n", pattern_id);
        break;
    }

    return S_OK;
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

    case UIA_HasKeyboardFocusPropertyId:
        variant_init_bool(ret_val, msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid,
                    STATE_SYSTEM_FOCUSED));
        break;

    case UIA_IsKeyboardFocusablePropertyId:
        variant_init_bool(ret_val, msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid,
                    STATE_SYSTEM_FOCUSABLE));
        break;

    case UIA_IsEnabledPropertyId:
        variant_init_bool(ret_val, !msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid,
                    STATE_SYSTEM_UNAVAILABLE));
        break;

    case UIA_IsPasswordPropertyId:
        variant_init_bool(ret_val, msaa_check_acc_state(msaa_prov->acc, msaa_prov->cid,
                    STATE_SYSTEM_PROTECTED));
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
    struct msaa_provider *msaa_prov = impl_from_msaa_provider(iface);

    TRACE("%p, %p\n", iface, ret_val);

    *ret_val = NULL;
    if (msaa_check_root_acc(msaa_prov))
        return UiaHostProviderFromHwnd(msaa_prov->hwnd, ret_val);

    return S_OK;
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

/*
 * IRawElementProviderFragment interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_fragment(IRawElementProviderFragment *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, IRawElementProviderFragment_iface);
}

static HRESULT WINAPI msaa_fragment_QueryInterface(IRawElementProviderFragment *iface, REFIID riid,
        void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_fragment_AddRef(IRawElementProviderFragment *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_fragment_Release(IRawElementProviderFragment *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_fragment_Navigate(IRawElementProviderFragment *iface,
        enum NavigateDirection direction, IRawElementProviderFragment **ret_val)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_fragment(iface);
    IRawElementProviderSimple *elprov;
    IAccessible *acc;
    HRESULT hr;

    TRACE("%p, %d, %p\n", iface, direction, ret_val);

    *ret_val = NULL;
    switch (direction)
    {
    case NavigateDirection_Parent:
        if (msaa_check_root_acc(msaa_prov))
            break;

        if (V_I4(&msaa_prov->cid) == CHILDID_SELF)
        {
            hr = msaa_acc_get_parent(msaa_prov->acc, &acc);
            if (FAILED(hr) || !acc)
                break;
        }
        else
            acc = msaa_prov->acc;

        hr = UiaProviderFromIAccessible(acc, CHILDID_SELF, 0, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);
            *ret_val = &prov->IRawElementProviderFragment_iface;
        }

        if (acc != msaa_prov->acc)
            IAccessible_Release(acc);

        break;

    case NavigateDirection_FirstChild:
    case NavigateDirection_LastChild:
    case NavigateDirection_NextSibling:
    case NavigateDirection_PreviousSibling:
        FIXME("Unimplemented NavigateDirection %d\n", direction);
        return E_NOTIMPL;

    default:
        FIXME("Invalid NavigateDirection %d\n", direction);
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI msaa_fragment_GetRuntimeId(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_get_BoundingRectangle(IRawElementProviderFragment *iface,
        struct UiaRect *ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_GetEmbeddedFragmentRoots(IRawElementProviderFragment *iface,
        SAFEARRAY **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_SetFocus(IRawElementProviderFragment *iface)
{
    FIXME("%p: stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_fragment_get_FragmentRoot(IRawElementProviderFragment *iface,
        IRawElementProviderFragmentRoot **ret_val)
{
    FIXME("%p, %p: stub!\n", iface, ret_val);
    *ret_val = NULL;
    return E_NOTIMPL;
}

static const IRawElementProviderFragmentVtbl msaa_fragment_vtbl = {
    msaa_fragment_QueryInterface,
    msaa_fragment_AddRef,
    msaa_fragment_Release,
    msaa_fragment_Navigate,
    msaa_fragment_GetRuntimeId,
    msaa_fragment_get_BoundingRectangle,
    msaa_fragment_GetEmbeddedFragmentRoots,
    msaa_fragment_SetFocus,
    msaa_fragment_get_FragmentRoot,
};

/*
 * ILegacyIAccessibleProvider interface for UiaProviderFromIAccessible
 * providers.
 */
static inline struct msaa_provider *impl_from_msaa_acc_provider(ILegacyIAccessibleProvider *iface)
{
    return CONTAINING_RECORD(iface, struct msaa_provider, ILegacyIAccessibleProvider_iface);
}

static HRESULT WINAPI msaa_acc_provider_QueryInterface(ILegacyIAccessibleProvider *iface, REFIID riid, void **ppv)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_QueryInterface(&msaa_prov->IRawElementProviderSimple_iface, riid, ppv);
}

static ULONG WINAPI msaa_acc_provider_AddRef(ILegacyIAccessibleProvider *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_AddRef(&msaa_prov->IRawElementProviderSimple_iface);
}

static ULONG WINAPI msaa_acc_provider_Release(ILegacyIAccessibleProvider *iface)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);
    return IRawElementProviderSimple_Release(&msaa_prov->IRawElementProviderSimple_iface);
}

static HRESULT WINAPI msaa_acc_provider_Select(ILegacyIAccessibleProvider *iface, LONG select_flags)
{
    FIXME("%p, %#lx: stub!\n", iface, select_flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_DoDefaultAction(ILegacyIAccessibleProvider *iface)
{
    FIXME("%p: stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_SetValue(ILegacyIAccessibleProvider *iface, LPCWSTR val)
{
    FIXME("%p, %p<%s>: stub!\n", iface, val, debugstr_w(val));
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_GetIAccessible(ILegacyIAccessibleProvider *iface,
        IAccessible **out_acc)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);

    TRACE("%p, %p\n", iface, out_acc);

    IAccessible_AddRef(msaa_prov->acc);
    *out_acc = msaa_prov->acc;

    return S_OK;
}

static HRESULT WINAPI msaa_acc_provider_get_ChildId(ILegacyIAccessibleProvider *iface, int *out_cid)
{
    struct msaa_provider *msaa_prov = impl_from_msaa_acc_provider(iface);

    TRACE("%p, %p\n", iface, out_cid);
    *out_cid = V_I4(&msaa_prov->cid);

    return S_OK;
}

static HRESULT WINAPI msaa_acc_provider_get_Name(ILegacyIAccessibleProvider *iface, BSTR *out_name)
{
    FIXME("%p, %p: stub!\n", iface, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Value(ILegacyIAccessibleProvider *iface, BSTR *out_value)
{
    FIXME("%p, %p: stub!\n", iface, out_value);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Description(ILegacyIAccessibleProvider *iface,
        BSTR *out_description)
{
    FIXME("%p, %p: stub!\n", iface, out_description);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Role(ILegacyIAccessibleProvider *iface, DWORD *out_role)
{
    FIXME("%p, %p: stub!\n", iface, out_role);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_State(ILegacyIAccessibleProvider *iface, DWORD *out_state)
{
    FIXME("%p, %p: stub!\n", iface, out_state);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_Help(ILegacyIAccessibleProvider *iface, BSTR *out_help)
{
    FIXME("%p, %p: stub!\n", iface, out_help);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_KeyboardShortcut(ILegacyIAccessibleProvider *iface,
        BSTR *out_kbd_shortcut)
{
    FIXME("%p, %p: stub!\n", iface, out_kbd_shortcut);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_GetSelection(ILegacyIAccessibleProvider *iface,
        SAFEARRAY **out_selected)
{
    FIXME("%p, %p: stub!\n", iface, out_selected);
    return E_NOTIMPL;
}

static HRESULT WINAPI msaa_acc_provider_get_DefaultAction(ILegacyIAccessibleProvider *iface,
        BSTR *out_default_action)
{
    FIXME("%p, %p: stub!\n", iface, out_default_action);
    return E_NOTIMPL;
}

static const ILegacyIAccessibleProviderVtbl msaa_acc_provider_vtbl = {
    msaa_acc_provider_QueryInterface,
    msaa_acc_provider_AddRef,
    msaa_acc_provider_Release,
    msaa_acc_provider_Select,
    msaa_acc_provider_DoDefaultAction,
    msaa_acc_provider_SetValue,
    msaa_acc_provider_GetIAccessible,
    msaa_acc_provider_get_ChildId,
    msaa_acc_provider_get_Name,
    msaa_acc_provider_get_Value,
    msaa_acc_provider_get_Description,
    msaa_acc_provider_get_Role,
    msaa_acc_provider_get_State,
    msaa_acc_provider_get_Help,
    msaa_acc_provider_get_KeyboardShortcut,
    msaa_acc_provider_GetSelection,
    msaa_acc_provider_get_DefaultAction,
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
    msaa_prov->IRawElementProviderFragment_iface.lpVtbl = &msaa_fragment_vtbl;
    msaa_prov->ILegacyIAccessibleProvider_iface.lpVtbl = &msaa_acc_provider_vtbl;
    msaa_prov->refcount = 1;
    msaa_prov->hwnd = hwnd;
    variant_init_i4(&msaa_prov->cid, child_id);
    msaa_prov->acc = acc;
    IAccessible_AddRef(acc);
    *elprov = &msaa_prov->IRawElementProviderSimple_iface;

    return S_OK;
}
