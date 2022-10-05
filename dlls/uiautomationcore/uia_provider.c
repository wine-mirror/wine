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

#include "uia_private.h"
#include "ocidl.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/rbtree.h"
#include "initguid.h"
#include "wine/iaccessible2.h"

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

static IAccessible2 *msaa_acc_get_ia2(IAccessible *acc)
{
    IServiceProvider *serv_prov;
    IAccessible2 *ia2 = NULL;
    HRESULT hr;

    hr = IAccessible_QueryInterface(acc, &IID_IServiceProvider, (void **)&serv_prov);
    if (SUCCEEDED(hr))
    {
        hr = IServiceProvider_QueryService(serv_prov, &IID_IAccessible2, &IID_IAccessible2, (void **)&ia2);
        IServiceProvider_Release(serv_prov);
        if (SUCCEEDED(hr) && ia2)
            return ia2;
    }

    hr = IAccessible_QueryInterface(acc, &IID_IAccessible2, (void **)&ia2);
    if (SUCCEEDED(hr) && ia2)
        return ia2;

    return NULL;
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
    IAccessible2 *ia2[2] = { NULL, NULL };
    IUnknown *unk, *unk2;
    BOOL matched = FALSE;
    LONG unique_id[2];
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

    ia2[0] = msaa_acc_get_ia2(acc);
    ia2[1] = msaa_acc_get_ia2(acc2);
    if (!ia2[0] != !ia2[1])
        goto exit;
    if (ia2[0])
    {
        hr = IAccessible2_get_uniqueID(ia2[0], &unique_id[0]);
        if (SUCCEEDED(hr))
        {
            hr = IAccessible2_get_uniqueID(ia2[1], &unique_id[1]);
            if (SUCCEEDED(hr))
            {
                if (unique_id[0] == unique_id[1])
                    matched = TRUE;

                goto exit;
            }
        }
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
    if (ia2[0])
        IAccessible2_Release(ia2[0]);
    if (ia2[1])
        IAccessible2_Release(ia2[1]);

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

#define DIR_FORWARD 0
#define DIR_REVERSE 1
static HRESULT msaa_acc_get_next_child(IAccessible *acc, LONG start_pos, LONG direction,
        IAccessible **child, LONG *child_id, LONG *child_pos, BOOL check_visible)
{
    LONG child_count, cur_pos;
    IDispatch *disp;
    VARIANT cid;
    HRESULT hr;

    *child = NULL;
    *child_id = 0;
    cur_pos = start_pos;
    while (1)
    {
        hr = IAccessible_get_accChildCount(acc, &child_count);
        if (FAILED(hr) || (cur_pos > child_count))
            break;

        variant_init_i4(&cid, cur_pos);
        hr = IAccessible_get_accChild(acc, cid, &disp);
        if (FAILED(hr))
            break;

        if (hr == S_FALSE)
        {
            if (!check_visible || !msaa_check_acc_state(acc, cid, STATE_SYSTEM_INVISIBLE))
            {
                *child = acc;
                *child_id = *child_pos = cur_pos;
                IAccessible_AddRef(acc);
                return S_OK;
            }
        }
        else
        {
            IAccessible *acc_child = NULL;

            hr = IDispatch_QueryInterface(disp, &IID_IAccessible, (void **)&acc_child);
            IDispatch_Release(disp);
            if (FAILED(hr))
                break;

            variant_init_i4(&cid, CHILDID_SELF);
            if (!check_visible || !msaa_check_acc_state(acc_child, cid, STATE_SYSTEM_INVISIBLE))
            {
                *child = acc_child;
                *child_id = CHILDID_SELF;
                *child_pos = cur_pos;
                return S_OK;
            }

            IAccessible_Release(acc_child);
        }

        if (direction == DIR_FORWARD)
            cur_pos++;
        else
            cur_pos--;

        if ((cur_pos > child_count) || (cur_pos <= 0))
            break;
    }

    return hr;
}

static HRESULT msaa_acc_get_child_pos(IAccessible *acc, IAccessible **out_parent,
        LONG *out_child_pos)
{
    LONG child_count, child_id, child_pos, match_pos;
    IAccessible *child, *parent, *match, **children;
    HRESULT hr;
    int i;

    *out_parent = NULL;
    *out_child_pos = 0;
    hr = msaa_acc_get_parent(acc, &parent);
    if (FAILED(hr) || !parent)
        return hr;

    hr = IAccessible_get_accChildCount(parent, &child_count);
    if (FAILED(hr) || !child_count)
    {
        IAccessible_Release(parent);
        return hr;
    }

    children = heap_alloc_zero(sizeof(*children) * child_count);
    if (!children)
        return E_OUTOFMEMORY;

    match = NULL;
    for (i = 0; i < child_count; i++)
    {
        hr = msaa_acc_get_next_child(parent, i + 1, DIR_FORWARD, &child, &child_id, &child_pos, FALSE);
        if (FAILED(hr) || !child)
            goto exit;

        if (child != parent)
            children[i] = child;
        else
            IAccessible_Release(child);
    }

    for (i = 0; i < child_count; i++)
    {
        if (!children[i])
            continue;

        if (msaa_acc_compare(acc, children[i]))
        {
            if (!match)
            {
                match = children[i];
                match_pos = i + 1;
            }
            /* Can't have more than one IAccessible match. */
            else
            {
                match = NULL;
                match_pos = 0;
                break;
            }
        }
    }

exit:
    if (match)
    {
        *out_parent = parent;
        *out_child_pos = match_pos;
    }
    else
        IAccessible_Release(parent);

    for (i = 0; i < child_count; i++)
    {
        if (children[i])
            IAccessible_Release(children[i]);
    }

    heap_free(children);

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
    IAccessible2 *ia2;
    VARIANT cid;
    HWND hwnd;
    LONG control_type;

    BOOL root_acc_check_ran;
    BOOL is_root_acc;

    IAccessible *parent;
    INT child_pos;
};

static BOOL msaa_check_root_acc(struct msaa_provider *msaa_prov)
{
    IAccessible *acc;
    HRESULT hr;

    if (msaa_prov->root_acc_check_ran)
        return msaa_prov->is_root_acc;

    msaa_prov->root_acc_check_ran = TRUE;
    if (V_I4(&msaa_prov->cid) != CHILDID_SELF || msaa_prov->parent)
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
        if (msaa_prov->parent)
            IAccessible_Release(msaa_prov->parent);
        if (msaa_prov->ia2)
            IAccessible2_Release(msaa_prov->ia2);
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
    LONG child_count, child_id, child_pos;
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
        if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
            break;

        hr = IAccessible_get_accChildCount(msaa_prov->acc, &child_count);
        if (FAILED(hr) || !child_count)
            break;

        if (direction == NavigateDirection_FirstChild)
            hr = msaa_acc_get_next_child(msaa_prov->acc, 1, DIR_FORWARD, &acc, &child_id,
                    &child_pos, TRUE);
        else
            hr = msaa_acc_get_next_child(msaa_prov->acc, child_count, DIR_REVERSE, &acc, &child_id,
                    &child_pos, TRUE);

        if (FAILED(hr) || !acc)
            break;

        hr = UiaProviderFromIAccessible(acc, child_id, 0, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);

            *ret_val = &prov->IRawElementProviderFragment_iface;
            prov->parent = msaa_prov->acc;
            IAccessible_AddRef(msaa_prov->acc);
            if (acc != msaa_prov->acc)
                prov->child_pos = child_pos;
            else
                prov->child_pos = child_id;
        }
        IAccessible_Release(acc);

        break;

    case NavigateDirection_NextSibling:
    case NavigateDirection_PreviousSibling:
        if (msaa_check_root_acc(msaa_prov))
            break;

        if (!msaa_prov->parent)
        {
            if (V_I4(&msaa_prov->cid) != CHILDID_SELF)
            {
                msaa_prov->parent = msaa_prov->acc;
                IAccessible_AddRef(msaa_prov->acc);
                msaa_prov->child_pos = V_I4(&msaa_prov->cid);
            }
            else
            {
                hr = msaa_acc_get_child_pos(msaa_prov->acc, &acc, &child_pos);
                if (FAILED(hr) || !acc)
                    break;
                msaa_prov->parent = acc;
                msaa_prov->child_pos = child_pos;
            }
        }

        if (direction == NavigateDirection_NextSibling)
            hr = msaa_acc_get_next_child(msaa_prov->parent, msaa_prov->child_pos + 1, DIR_FORWARD,
                    &acc, &child_id, &child_pos, TRUE);
        else
            hr = msaa_acc_get_next_child(msaa_prov->parent, msaa_prov->child_pos - 1, DIR_REVERSE,
                    &acc, &child_id, &child_pos, TRUE);

        if (FAILED(hr) || !acc)
            break;

        hr = UiaProviderFromIAccessible(acc, child_id, 0, &elprov);
        if (SUCCEEDED(hr))
        {
            struct msaa_provider *prov = impl_from_msaa_provider(elprov);

            *ret_val = &prov->IRawElementProviderFragment_iface;
            prov->parent = msaa_prov->parent;
            IAccessible_AddRef(msaa_prov->parent);
            if (acc != msaa_prov->acc)
                prov->child_pos = child_pos;
            else
                prov->child_pos = child_id;
        }
        IAccessible_Release(acc);

        break;

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
    msaa_prov->ia2 = msaa_acc_get_ia2(acc);
    *elprov = &msaa_prov->IRawElementProviderSimple_iface;

    return S_OK;
}

/*
 * UI Automation provider thread functions.
 */
struct uia_provider_thread
{
    struct rb_tree node_map;
    struct list nodes_list;
    HANDLE hthread;
    HWND hwnd;
    LONG ref;
};

static struct uia_provider_thread provider_thread;
static CRITICAL_SECTION provider_thread_cs;
static CRITICAL_SECTION_DEBUG provider_thread_cs_debug =
{
    0, 0, &provider_thread_cs,
    { &provider_thread_cs_debug.ProcessLocksList, &provider_thread_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": provider_thread_cs") }
};
static CRITICAL_SECTION provider_thread_cs = { &provider_thread_cs_debug, -1, 0, 0, 0, 0 };

struct uia_provider_thread_map_entry
{
    struct rb_entry entry;

    SAFEARRAY *runtime_id;
    struct list nodes_list;
};

static int uia_runtime_id_compare(const void *key, const struct rb_entry *entry)
{
    struct uia_provider_thread_map_entry *prov_entry = RB_ENTRY_VALUE(entry, struct uia_provider_thread_map_entry, entry);
    return uia_compare_runtime_ids(prov_entry->runtime_id, (SAFEARRAY *)key);
}

void uia_provider_thread_remove_node(HUIANODE node)
{
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);

    TRACE("Removing node %p\n", node);

    EnterCriticalSection(&provider_thread_cs);

    list_remove(&node_data->prov_thread_list_entry);
    list_init(&node_data->prov_thread_list_entry);
    if (!list_empty(&node_data->node_map_list_entry))
    {
        list_remove(&node_data->node_map_list_entry);
        list_init(&node_data->node_map_list_entry);
        if (list_empty(&node_data->map->nodes_list))
        {
            rb_remove(&provider_thread.node_map, &node_data->map->entry);
            SafeArrayDestroy(node_data->map->runtime_id);
            heap_free(node_data->map);
        }
        node_data->map = NULL;
    }

    LeaveCriticalSection(&provider_thread_cs);
}

static void uia_provider_thread_disconnect_node(SAFEARRAY *sa)
{
    struct rb_entry *rb_entry;

    EnterCriticalSection(&provider_thread_cs);

    /* Provider thread hasn't been started, no nodes to disconnect. */
    if (!provider_thread.ref)
        goto exit;

    rb_entry = rb_get(&provider_thread.node_map, sa);
    if (rb_entry)
    {
        struct uia_provider_thread_map_entry *prov_map;
        struct list *cursor, *cursor2;
        struct uia_node *node_data;

        prov_map = RB_ENTRY_VALUE(rb_entry, struct uia_provider_thread_map_entry, entry);
        LIST_FOR_EACH_SAFE(cursor, cursor2, &prov_map->nodes_list)
        {
            node_data = LIST_ENTRY(cursor, struct uia_node, node_map_list_entry);

            list_remove(cursor);
            list_remove(&node_data->prov_thread_list_entry);
            list_init(&node_data->prov_thread_list_entry);
            list_init(&node_data->node_map_list_entry);
            node_data->map = NULL;

            IWineUiaNode_disconnect(&node_data->IWineUiaNode_iface);
        }

        rb_remove(&provider_thread.node_map, &prov_map->entry);
        SafeArrayDestroy(prov_map->runtime_id);
        heap_free(prov_map);
    }

exit:
    LeaveCriticalSection(&provider_thread_cs);
}

static HRESULT uia_provider_thread_add_node(HUIANODE node)
{
    struct uia_node *node_data = impl_from_IWineUiaNode((IWineUiaNode *)node);
    struct uia_provider *prov_data = impl_from_IWineUiaProvider(node_data->prov);
    SAFEARRAY *sa;
    HRESULT hr;

    node_data->nested_node = prov_data->return_nested_node = TRUE;
    hr = UiaGetRuntimeId(node, &sa);
    if (FAILED(hr))
        return hr;

    TRACE("Adding node %p\n", node);

    EnterCriticalSection(&provider_thread_cs);
    list_add_tail(&provider_thread.nodes_list, &node_data->prov_thread_list_entry);

    /* If we have a runtime ID, create an entry in the rb tree. */
    if (sa)
    {
        struct uia_provider_thread_map_entry *prov_map;
        struct rb_entry *rb_entry;

        if ((rb_entry = rb_get(&provider_thread.node_map, sa)))
        {
            prov_map = RB_ENTRY_VALUE(rb_entry, struct uia_provider_thread_map_entry, entry);
            SafeArrayDestroy(sa);
        }
        else
        {
            prov_map = heap_alloc_zero(sizeof(*prov_map));
            if (!prov_map)
            {
                SafeArrayDestroy(sa);
                LeaveCriticalSection(&provider_thread_cs);
                return E_OUTOFMEMORY;
            }

            prov_map->runtime_id = sa;
            list_init(&prov_map->nodes_list);
            rb_put(&provider_thread.node_map, sa, &prov_map->entry);
        }

        list_add_tail(&prov_map->nodes_list, &node_data->node_map_list_entry);
        node_data->map = prov_map;
    }

    LeaveCriticalSection(&provider_thread_cs);

    return S_OK;
}

#define WM_GET_OBJECT_UIA_NODE (WM_USER + 1)
#define WM_UIA_PROVIDER_THREAD_STOP (WM_USER + 2)
static LRESULT CALLBACK uia_provider_thread_msg_proc(HWND hwnd, UINT msg, WPARAM wparam,
        LPARAM lparam)
{
    switch (msg)
    {
    case WM_GET_OBJECT_UIA_NODE:
    {
        HUIANODE node = (HUIANODE)lparam;
        LRESULT lr;

        if (FAILED(uia_provider_thread_add_node(node)))
        {
            WARN("Failed to add node %p to provider thread list.\n", node);
            UiaNodeRelease(node);
            return 0;
        }

        /*
         * LresultFromObject returns an index into the global atom string table,
         * which has a valid range of 0xc000-0xffff.
         */
        lr = LresultFromObject(&IID_IWineUiaNode, 0, (IUnknown *)node);
        if ((lr > 0xffff) || (lr < 0xc000))
        {
            WARN("Got invalid lresult %Ix\n", lr);
            lr = 0;
        }

        /*
         * LresultFromObject increases refcnt by 1. If LresultFromObject
         * failed, this is expected to release the node.
         */
        UiaNodeRelease(node);
        return lr;
    }

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static DWORD WINAPI uia_provider_thread_proc(void *arg)
{
    HANDLE initialized_event = arg;
    HWND hwnd;
    MSG msg;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    hwnd = CreateWindowW(L"Message", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!hwnd)
    {
        WARN("CreateWindow failed: %ld\n", GetLastError());
        CoUninitialize();
        FreeLibraryAndExitThread(huia_module, 1);
    }

    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)uia_provider_thread_msg_proc);
    provider_thread.hwnd = hwnd;

    /* Initialization complete, thread can now process window messages. */
    SetEvent(initialized_event);
    TRACE("Provider thread started.\n");
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_UIA_PROVIDER_THREAD_STOP)
            break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    TRACE("Shutting down UI Automation provider thread.\n");

    DestroyWindow(hwnd);
    CoUninitialize();
    FreeLibraryAndExitThread(huia_module, 0);
}

static BOOL uia_start_provider_thread(void)
{
    BOOL started = TRUE;

    EnterCriticalSection(&provider_thread_cs);
    if (++provider_thread.ref == 1)
    {
        HANDLE ready_event;
        HANDLE events[2];
        HMODULE hmodule;
        DWORD wait_obj;

        /* Increment DLL reference count. */
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)uia_start_provider_thread, &hmodule);

        list_init(&provider_thread.nodes_list);
        rb_init(&provider_thread.node_map, uia_runtime_id_compare);
        events[0] = ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!(provider_thread.hthread = CreateThread(NULL, 0, uia_provider_thread_proc,
                ready_event, 0, NULL)))
        {
            FreeLibrary(hmodule);
            started = FALSE;
            goto exit;
        }

        events[1] = provider_thread.hthread;
        wait_obj = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (wait_obj != WAIT_OBJECT_0)
        {
            CloseHandle(provider_thread.hthread);
            started = FALSE;
        }

exit:
        CloseHandle(ready_event);
        if (!started)
        {
            WARN("Failed to start provider thread\n");
            memset(&provider_thread, 0, sizeof(provider_thread));
        }
    }

    LeaveCriticalSection(&provider_thread_cs);
    return started;
}

void uia_stop_provider_thread(void)
{
    EnterCriticalSection(&provider_thread_cs);
    if (!--provider_thread.ref)
    {
        PostMessageW(provider_thread.hwnd, WM_UIA_PROVIDER_THREAD_STOP, 0, 0);
        CloseHandle(provider_thread.hthread);
        if (!list_empty(&provider_thread.nodes_list))
            ERR("Provider thread shutdown with nodes still in the list\n");
        memset(&provider_thread, 0, sizeof(provider_thread));
    }
    LeaveCriticalSection(&provider_thread_cs);
}

/*
 * Pass our IWineUiaNode interface to the provider thread for marshaling. UI
 * Automation has to work regardless of whether or not COM is initialized on
 * the thread calling UiaReturnRawElementProvider.
 */
LRESULT uia_lresult_from_node(HUIANODE huianode)
{
    if (!uia_start_provider_thread())
    {
        UiaNodeRelease(huianode);
        return 0;
    }

    return SendMessageW(provider_thread.hwnd, WM_GET_OBJECT_UIA_NODE, 0, (LPARAM)huianode);
}

/***********************************************************************
 *          UiaReturnRawElementProvider (uiautomationcore.@)
 */
LRESULT WINAPI UiaReturnRawElementProvider(HWND hwnd, WPARAM wparam,
        LPARAM lparam, IRawElementProviderSimple *elprov)
{
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p, %Ix, %#Ix, %p)\n", hwnd, wparam, lparam, elprov);

    if (!wparam && !lparam && !elprov)
    {
        FIXME("UIA-to-MSAA bridge not implemented, no provider map to free.\n");
        return 0;
    }

    if (lparam != UiaRootObjectId)
    {
        FIXME("Unsupported object id %Id, ignoring.\n", lparam);
        return 0;
    }

    hr = UiaNodeFromProvider(elprov, &node);
    if (FAILED(hr))
    {
        WARN("Failed to create HUIANODE with hr %#lx\n", hr);
        return 0;
    }

    return uia_lresult_from_node(node);
}

/***********************************************************************
 *          UiaDisconnectProvider (uiautomationcore.@)
 */
HRESULT WINAPI UiaDisconnectProvider(IRawElementProviderSimple *elprov)
{
    SAFEARRAY *sa;
    HUIANODE node;
    HRESULT hr;

    TRACE("(%p)\n", elprov);

    hr = UiaNodeFromProvider(elprov, &node);
    if (FAILED(hr))
        return hr;

    hr = UiaGetRuntimeId(node, &sa);
    UiaNodeRelease(node);
    if (FAILED(hr))
        return hr;

    if (!sa)
        return E_INVALIDARG;

    uia_provider_thread_disconnect_node(sa);

    SafeArrayDestroy(sa);

    return S_OK;
}
