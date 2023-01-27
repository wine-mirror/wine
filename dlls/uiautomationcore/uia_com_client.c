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

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(uiautomation);

/*
 * IUIAutomationElement interface.
 */
struct uia_element {
    IUIAutomationElement9 IUIAutomationElement9_iface;
    LONG ref;

    BOOL from_cui8;
    HUIANODE node;
};

static inline struct uia_element *impl_from_IUIAutomationElement9(IUIAutomationElement9 *iface)
{
    return CONTAINING_RECORD(iface, struct uia_element, IUIAutomationElement9_iface);
}

static HRESULT WINAPI uia_element_QueryInterface(IUIAutomationElement9 *iface, REFIID riid, void **ppv)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IUIAutomationElement) || (element->from_cui8 &&
            (IsEqualIID(riid, &IID_IUIAutomationElement2) || IsEqualIID(riid, &IID_IUIAutomationElement3) ||
            IsEqualIID(riid, &IID_IUIAutomationElement4) || IsEqualIID(riid, &IID_IUIAutomationElement5) ||
            IsEqualIID(riid, &IID_IUIAutomationElement6) || IsEqualIID(riid, &IID_IUIAutomationElement7) ||
            IsEqualIID(riid, &IID_IUIAutomationElement8) || IsEqualIID(riid, &IID_IUIAutomationElement9))))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomationElement9_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_element_AddRef(IUIAutomationElement9 *iface)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    ULONG ref = InterlockedIncrement(&element->ref);

    TRACE("%p, refcount %ld\n", element, ref);
    return ref;
}

static ULONG WINAPI uia_element_Release(IUIAutomationElement9 *iface)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    ULONG ref = InterlockedDecrement(&element->ref);

    TRACE("%p, refcount %ld\n", element, ref);
    if (!ref)
    {
        UiaNodeRelease(element->node);
        heap_free(element);
    }

    return ref;
}

static HRESULT WINAPI uia_element_SetFocus(IUIAutomationElement9 *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetRuntimeId(IUIAutomationElement9 *iface, SAFEARRAY **runtime_id)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirst(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAll(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirstBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req, IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAllBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req, IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_BuildUpdatedCache(IUIAutomationElement9 *iface, IUIAutomationCacheRequest *cache_req,
        IUIAutomationElement **updated_elem)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentPropertyValue(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT create_uia_element(IUIAutomationElement **iface, BOOL from_cui8, HUIANODE node);
static HRESULT WINAPI uia_element_GetCurrentPropertyValueEx(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        BOOL ignore_default, VARIANT *ret_val)
{
    const struct uia_prop_info *prop_info = uia_prop_info_from_id(prop_id);
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;

    TRACE("%p, %d, %d, %p\n", iface, prop_id, ignore_default, ret_val);

    if (!ignore_default)
        FIXME("Default values currently unimplemented\n");

    VariantInit(ret_val);
    if (prop_info->type == UIAutomationType_ElementArray)
    {
        FIXME("ElementArray property types currently unsupported for IUIAutomationElement\n");
        return E_NOTIMPL;
    }

    hr = UiaGetPropertyValue(element->node, prop_id, ret_val);
    if ((prop_info->type == UIAutomationType_Element) && (V_VT(ret_val) != VT_UNKNOWN))
    {
        IUIAutomationElement *out_elem;
        HUIANODE node;

        hr = UiaHUiaNodeFromVariant(ret_val, &node);
        VariantClear(ret_val);
        if (FAILED(hr))
            return hr;

        hr = create_uia_element(&out_elem, element->from_cui8, node);
        if (SUCCEEDED(hr))
        {
            V_VT(ret_val) = VT_UNKNOWN;
            V_UNKNOWN(ret_val) = (IUnknown *)out_elem;
        }
    }

    return hr;
}

static HRESULT WINAPI uia_element_GetCachedPropertyValue(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedPropertyValueEx(IUIAutomationElement9 *iface, PROPERTYID prop_id,
        BOOL ignore_default, VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentPatternAs(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        REFIID riid, void **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedPatternAs(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        REFIID riid, void **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentPattern(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        IUnknown **out_pattern)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedPattern(IUIAutomationElement9 *iface, PATTERNID pattern_id,
        IUnknown **patternObject)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedParent(IUIAutomationElement9 *iface, IUIAutomationElement **parent)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCachedChildren(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **children)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentProcessId(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentControlType(IUIAutomationElement9 *iface, CONTROLTYPEID *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    const struct uia_control_type_info *control_type_info = NULL;
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", iface, ret_val);

    VariantInit(&v);
    *ret_val = UIA_CustomControlTypeId;
    hr = UiaGetPropertyValue(element->node, UIA_ControlTypePropertyId, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_I4)
    {
        if ((control_type_info = uia_control_type_info_from_id(V_I4(&v))))
            *ret_val = control_type_info->control_type_id;
        else
            WARN("Provider returned invalid control type ID %ld\n", V_I4(&v));
    }

    VariantClear(&v);
    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentLocalizedControlType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", iface, ret_val);

    VariantInit(&v);
    hr = UiaGetPropertyValue(element->node, UIA_NamePropertyId, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == VT_BSTR && V_BSTR(&v))
        *ret_val = SysAllocString(V_BSTR(&v));
    else
        *ret_val = SysAllocString(L"");

    VariantClear(&v);
    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentAcceleratorKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAccessKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHasKeyboardFocus(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsKeyboardFocusable(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsEnabled(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAutomationId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentClassName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHelpText(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentCulture(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsControlElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsContentElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsPassword(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentNativeWindowHandle(IUIAutomationElement9 *iface, UIA_HWND *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentItemType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsOffscreen(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentOrientation(IUIAutomationElement9 *iface, enum OrientationType *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFrameworkId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsRequiredForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentItemStatus(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentBoundingRectangle(IUIAutomationElement9 *iface, RECT *ret_val)
{
    struct uia_element *element = impl_from_IUIAutomationElement9(iface);
    HRESULT hr;
    VARIANT v;

    TRACE("%p, %p\n", element, ret_val);

    memset(ret_val, 0, sizeof(*ret_val));
    VariantInit(&v);
    hr = UiaGetPropertyValue(element->node, UIA_BoundingRectanglePropertyId, &v);
    if (SUCCEEDED(hr) && V_VT(&v) == (VT_R8 | VT_ARRAY))
    {
        double vals[4];
        LONG idx;

        for (idx = 0; idx < ARRAY_SIZE(vals); idx++)
            SafeArrayGetElement(V_ARRAY(&v), &idx, &vals[idx]);

        ret_val->left = vals[0];
        ret_val->top = vals[1];
        ret_val->right = ret_val->left + vals[2];
        ret_val->bottom = ret_val->top + vals[3];
    }

    VariantClear(&v);
    return hr;
}

static HRESULT WINAPI uia_element_get_CurrentLabeledBy(IUIAutomationElement9 *iface, IUIAutomationElement **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAriaRole(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAriaProperties(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsDataValidForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentControllerFor(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentDescribedBy(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFlowsTo(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentProviderDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedProcessId(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedControlType(IUIAutomationElement9 *iface, CONTROLTYPEID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLocalizedControlType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAcceleratorKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAccessKey(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHasKeyboardFocus(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsKeyboardFocusable(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsEnabled(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAutomationId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedClassName(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHelpText(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedCulture(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsControlElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsContentElement(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsPassword(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedNativeWindowHandle(IUIAutomationElement9 *iface, UIA_HWND *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedItemType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsOffscreen(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedOrientation(IUIAutomationElement9 *iface,
        enum OrientationType *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFrameworkId(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsRequiredForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedItemStatus(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedBoundingRectangle(IUIAutomationElement9 *iface, RECT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLabeledBy(IUIAutomationElement9 *iface, IUIAutomationElement **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAriaRole(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAriaProperties(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsDataValidForForm(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedControllerFor(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedDescribedBy(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFlowsTo(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedProviderDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetClickablePoint(IUIAutomationElement9 *iface, POINT *clickable, BOOL *got_clickable)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentOptimizeForVisualContent(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedOptimizeForVisualContent(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLiveSetting(IUIAutomationElement9 *iface, enum LiveSetting *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLiveSetting(IUIAutomationElement9 *iface, enum LiveSetting *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFlowsFrom(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFlowsFrom(IUIAutomationElement9 *iface, IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_ShowContextMenu(IUIAutomationElement9 *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsPeripheral(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsPeripheral(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentPositionInSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentSizeOfSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLevel(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAnnotationTypes(IUIAutomationElement9 *iface, SAFEARRAY **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentAnnotationObjects(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedPositionInSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedSizeOfSet(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLevel(IUIAutomationElement9 *iface, int *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAnnotationTypes(IUIAutomationElement9 *iface, SAFEARRAY **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedAnnotationObjects(IUIAutomationElement9 *iface,
        IUIAutomationElementArray **ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLandmarkType(IUIAutomationElement9 *iface, LANDMARKTYPEID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentLocalizedLandmarkType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLandmarkType(IUIAutomationElement9 *iface, LANDMARKTYPEID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedLocalizedLandmarkType(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentFullDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedFullDescription(IUIAutomationElement9 *iface, BSTR *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirstWithOptions(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root,
        IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAllWithOptions(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root,
        IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindFirstWithOptionsBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req,
        enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root, IUIAutomationElement **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_FindAllWithOptionsBuildCache(IUIAutomationElement9 *iface, enum TreeScope scope,
        IUIAutomationCondition *condition, IUIAutomationCacheRequest *cache_req,
        enum TreeTraversalOptions traversal_opts, IUIAutomationElement *root, IUIAutomationElementArray **found)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_GetCurrentMetadataValue(IUIAutomationElement9 *iface, int target_id,
        METADATAID metadata_id, VARIANT *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentHeadingLevel(IUIAutomationElement9 *iface, HEADINGLEVELID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedHeadingLevel(IUIAutomationElement9 *iface, HEADINGLEVELID *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CurrentIsDialog(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_element_get_CachedIsDialog(IUIAutomationElement9 *iface, BOOL *ret_val)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static const IUIAutomationElement9Vtbl uia_element_vtbl = {
    uia_element_QueryInterface,
    uia_element_AddRef,
    uia_element_Release,
    uia_element_SetFocus,
    uia_element_GetRuntimeId,
    uia_element_FindFirst,
    uia_element_FindAll,
    uia_element_FindFirstBuildCache,
    uia_element_FindAllBuildCache,
    uia_element_BuildUpdatedCache,
    uia_element_GetCurrentPropertyValue,
    uia_element_GetCurrentPropertyValueEx,
    uia_element_GetCachedPropertyValue,
    uia_element_GetCachedPropertyValueEx,
    uia_element_GetCurrentPatternAs,
    uia_element_GetCachedPatternAs,
    uia_element_GetCurrentPattern,
    uia_element_GetCachedPattern,
    uia_element_GetCachedParent,
    uia_element_GetCachedChildren,
    uia_element_get_CurrentProcessId,
    uia_element_get_CurrentControlType,
    uia_element_get_CurrentLocalizedControlType,
    uia_element_get_CurrentName,
    uia_element_get_CurrentAcceleratorKey,
    uia_element_get_CurrentAccessKey,
    uia_element_get_CurrentHasKeyboardFocus,
    uia_element_get_CurrentIsKeyboardFocusable,
    uia_element_get_CurrentIsEnabled,
    uia_element_get_CurrentAutomationId,
    uia_element_get_CurrentClassName,
    uia_element_get_CurrentHelpText,
    uia_element_get_CurrentCulture,
    uia_element_get_CurrentIsControlElement,
    uia_element_get_CurrentIsContentElement,
    uia_element_get_CurrentIsPassword,
    uia_element_get_CurrentNativeWindowHandle,
    uia_element_get_CurrentItemType,
    uia_element_get_CurrentIsOffscreen,
    uia_element_get_CurrentOrientation,
    uia_element_get_CurrentFrameworkId,
    uia_element_get_CurrentIsRequiredForForm,
    uia_element_get_CurrentItemStatus,
    uia_element_get_CurrentBoundingRectangle,
    uia_element_get_CurrentLabeledBy,
    uia_element_get_CurrentAriaRole,
    uia_element_get_CurrentAriaProperties,
    uia_element_get_CurrentIsDataValidForForm,
    uia_element_get_CurrentControllerFor,
    uia_element_get_CurrentDescribedBy,
    uia_element_get_CurrentFlowsTo,
    uia_element_get_CurrentProviderDescription,
    uia_element_get_CachedProcessId,
    uia_element_get_CachedControlType,
    uia_element_get_CachedLocalizedControlType,
    uia_element_get_CachedName,
    uia_element_get_CachedAcceleratorKey,
    uia_element_get_CachedAccessKey,
    uia_element_get_CachedHasKeyboardFocus,
    uia_element_get_CachedIsKeyboardFocusable,
    uia_element_get_CachedIsEnabled,
    uia_element_get_CachedAutomationId,
    uia_element_get_CachedClassName,
    uia_element_get_CachedHelpText,
    uia_element_get_CachedCulture,
    uia_element_get_CachedIsControlElement,
    uia_element_get_CachedIsContentElement,
    uia_element_get_CachedIsPassword,
    uia_element_get_CachedNativeWindowHandle,
    uia_element_get_CachedItemType,
    uia_element_get_CachedIsOffscreen,
    uia_element_get_CachedOrientation,
    uia_element_get_CachedFrameworkId,
    uia_element_get_CachedIsRequiredForForm,
    uia_element_get_CachedItemStatus,
    uia_element_get_CachedBoundingRectangle,
    uia_element_get_CachedLabeledBy,
    uia_element_get_CachedAriaRole,
    uia_element_get_CachedAriaProperties,
    uia_element_get_CachedIsDataValidForForm,
    uia_element_get_CachedControllerFor,
    uia_element_get_CachedDescribedBy,
    uia_element_get_CachedFlowsTo,
    uia_element_get_CachedProviderDescription,
    uia_element_GetClickablePoint,
    uia_element_get_CurrentOptimizeForVisualContent,
    uia_element_get_CachedOptimizeForVisualContent,
    uia_element_get_CurrentLiveSetting,
    uia_element_get_CachedLiveSetting,
    uia_element_get_CurrentFlowsFrom,
    uia_element_get_CachedFlowsFrom,
    uia_element_ShowContextMenu,
    uia_element_get_CurrentIsPeripheral,
    uia_element_get_CachedIsPeripheral,
    uia_element_get_CurrentPositionInSet,
    uia_element_get_CurrentSizeOfSet,
    uia_element_get_CurrentLevel,
    uia_element_get_CurrentAnnotationTypes,
    uia_element_get_CurrentAnnotationObjects,
    uia_element_get_CachedPositionInSet,
    uia_element_get_CachedSizeOfSet,
    uia_element_get_CachedLevel,
    uia_element_get_CachedAnnotationTypes,
    uia_element_get_CachedAnnotationObjects,
    uia_element_get_CurrentLandmarkType,
    uia_element_get_CurrentLocalizedLandmarkType,
    uia_element_get_CachedLandmarkType,
    uia_element_get_CachedLocalizedLandmarkType,
    uia_element_get_CurrentFullDescription,
    uia_element_get_CachedFullDescription,
    uia_element_FindFirstWithOptions,
    uia_element_FindAllWithOptions,
    uia_element_FindFirstWithOptionsBuildCache,
    uia_element_FindAllWithOptionsBuildCache,
    uia_element_GetCurrentMetadataValue,
    uia_element_get_CurrentHeadingLevel,
    uia_element_get_CachedHeadingLevel,
    uia_element_get_CurrentIsDialog,
    uia_element_get_CachedIsDialog,
};

static HRESULT create_uia_element(IUIAutomationElement **iface, BOOL from_cui8, HUIANODE node)
{
    struct uia_element *element = heap_alloc_zero(sizeof(*element));

    *iface = NULL;
    if (!element)
        return E_OUTOFMEMORY;

    element->IUIAutomationElement9_iface.lpVtbl = &uia_element_vtbl;
    element->ref = 1;
    element->from_cui8 = from_cui8;
    element->node = node;

    *iface = (IUIAutomationElement *)&element->IUIAutomationElement9_iface;
    return S_OK;
}

/*
 * IUIAutomation interface.
 */
struct uia_iface {
    IUIAutomation6 IUIAutomation6_iface;
    LONG ref;

    BOOL is_cui8;
};

static inline struct uia_iface *impl_from_IUIAutomation6(IUIAutomation6 *iface)
{
    return CONTAINING_RECORD(iface, struct uia_iface, IUIAutomation6_iface);
}

static HRESULT WINAPI uia_iface_QueryInterface(IUIAutomation6 *iface, REFIID riid, void **ppv)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUIAutomation) || IsEqualIID(riid, &IID_IUnknown))
        *ppv = iface;
    else if (uia_iface->is_cui8 &&
            (IsEqualIID(riid, &IID_IUIAutomation2) ||
            IsEqualIID(riid, &IID_IUIAutomation3) ||
            IsEqualIID(riid, &IID_IUIAutomation4) ||
            IsEqualIID(riid, &IID_IUIAutomation5) ||
            IsEqualIID(riid, &IID_IUIAutomation6)))
        *ppv = iface;
    else
        return E_NOINTERFACE;

    IUIAutomation6_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI uia_iface_AddRef(IUIAutomation6 *iface)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    ULONG ref = InterlockedIncrement(&uia_iface->ref);

    TRACE("%p, refcount %ld\n", uia_iface, ref);
    return ref;
}

static ULONG WINAPI uia_iface_Release(IUIAutomation6 *iface)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    ULONG ref = InterlockedDecrement(&uia_iface->ref);

    TRACE("%p, refcount %ld\n", uia_iface, ref);
    if (!ref)
        heap_free(uia_iface);
    return ref;
}

static HRESULT WINAPI uia_iface_CompareElements(IUIAutomation6 *iface, IUIAutomationElement *elem1,
        IUIAutomationElement *elem2, BOOL *match)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem1, elem2, match);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CompareRuntimeIds(IUIAutomation6 *iface, SAFEARRAY *rt_id1, SAFEARRAY *rt_id2,
        BOOL *match)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, rt_id1, rt_id2, match);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetRootElement(IUIAutomation6 *iface, IUIAutomationElement **root)
{
    FIXME("%p, %p: stub\n", iface, root);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromHandle(IUIAutomation6 *iface, UIA_HWND hwnd, IUIAutomationElement **out_elem)
{
    struct uia_iface *uia_iface = impl_from_IUIAutomation6(iface);
    HUIANODE node;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, hwnd, out_elem);

    hr = UiaNodeFromHandle((HWND)hwnd, &node);
    if (FAILED(hr))
        return hr;

    return create_uia_element(out_elem, uia_iface->is_cui8, node);
}

static HRESULT WINAPI uia_iface_ElementFromPoint(IUIAutomation6 *iface, POINT pt, IUIAutomationElement **out_elem)
{
    FIXME("%p, %s, %p: stub\n", iface, wine_dbgstr_point(&pt), out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetFocusedElement(IUIAutomation6 *iface, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p: stub\n", iface, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetRootElementBuildCache(IUIAutomation6 *iface, IUIAutomationCacheRequest *cache_req,
        IUIAutomationElement **out_root)
{
    FIXME("%p, %p, %p: stub\n", iface, cache_req, out_root);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromHandleBuildCache(IUIAutomation6 *iface, UIA_HWND hwnd,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, hwnd, cache_req, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromPointBuildCache(IUIAutomation6 *iface, POINT pt,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %s, %p, %p: stub\n", iface, wine_dbgstr_point(&pt), cache_req, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetFocusedElementBuildCache(IUIAutomation6 *iface,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %p: stub\n", iface, cache_req, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateTreeWalker(IUIAutomation6 *iface, IUIAutomationCondition *cond,
        IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p, %p: stub\n", iface, cond, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ControlViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ContentViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_RawViewWalker(IUIAutomation6 *iface, IUIAutomationTreeWalker **out_walker)
{
    FIXME("%p, %p: stub\n", iface, out_walker);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_RawViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ControlViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ContentViewCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateCacheRequest(IUIAutomation6 *iface, IUIAutomationCacheRequest **out_cache_req)
{
    FIXME("%p, %p: stub\n", iface, out_cache_req);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateTrueCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateFalseCondition(IUIAutomation6 *iface, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p: stub\n", iface, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreatePropertyCondition(IUIAutomation6 *iface, PROPERTYID prop_id, VARIANT val,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %d, %s, %p: stub\n", iface, prop_id, debugstr_variant(&val), out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreatePropertyConditionEx(IUIAutomation6 *iface, PROPERTYID prop_id, VARIANT val,
        enum PropertyConditionFlags flags, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %d, %s, %#x, %p: stub\n", iface, prop_id, debugstr_variant(&val), flags, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond1,
        IUIAutomationCondition *cond2, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, cond1, cond2, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndConditionFromArray(IUIAutomation6 *iface, SAFEARRAY *conds,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p: stub\n", iface, conds, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateAndConditionFromNativeArray(IUIAutomation6 *iface, IUIAutomationCondition **conds,
        int conds_count, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, conds, conds_count, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateOrCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond1,
        IUIAutomationCondition *cond2, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, cond1, cond2, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateOrConditionFromArray(IUIAutomation6 *iface, SAFEARRAY *conds,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p: stub\n", iface, conds, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateOrConditionFromNativeArray(IUIAutomation6 *iface, IUIAutomationCondition **conds,
        int conds_count, IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, conds, conds_count, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateNotCondition(IUIAutomation6 *iface, IUIAutomationCondition *cond,
        IUIAutomationCondition **out_condition)
{
    FIXME("%p, %p, %p: stub\n", iface, cond, out_condition);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddAutomationEventHandler(IUIAutomation6 *iface, EVENTID event_id,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationEventHandler *handler)
{
    FIXME("%p, %d, %p, %#x, %p, %p: stub\n", iface, event_id, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveAutomationEventHandler(IUIAutomation6 *iface, EVENTID event_id,
        IUIAutomationElement *elem, IUIAutomationEventHandler *handler)
{
    FIXME("%p, %d, %p, %p: stub\n", iface, event_id, elem, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddPropertyChangedEventHandlerNativeArray(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationPropertyChangedEventHandler *handler, PROPERTYID *props, int props_count)
{
    FIXME("%p, %p, %#x, %p, %p, %p, %d: stub\n", iface, elem, scope, cache_req, handler, props, props_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddPropertyChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationPropertyChangedEventHandler *handler, SAFEARRAY *props)
{
    FIXME("%p, %p, %#x, %p, %p, %p: stub\n", iface, elem, scope, cache_req, handler, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemovePropertyChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationPropertyChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddStructureChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationStructureChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveStructureChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationStructureChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddFocusChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationCacheRequest *cache_req, IUIAutomationFocusChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveFocusChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationFocusChangedEventHandler *handler)
{
    FIXME("%p, %p: stub\n", iface, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveAllEventHandlers(IUIAutomation6 *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_IntNativeArrayToSafeArray(IUIAutomation6 *iface, int *arr, int arr_count,
        SAFEARRAY **out_sa)
{
    HRESULT hr = S_OK;
    SAFEARRAY *sa;
    int *sa_arr;

    TRACE("%p, %p, %d, %p\n", iface, arr, arr_count, out_sa);

    if (!out_sa || !arr || !arr_count)
        return E_INVALIDARG;

    *out_sa = NULL;
    if (!(sa = SafeArrayCreateVector(VT_I4, 0, arr_count)))
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void **)&sa_arr);
    if (FAILED(hr))
        goto exit;

    memcpy(sa_arr, arr, sizeof(*arr) * arr_count);
    hr = SafeArrayUnaccessData(sa);
    if (SUCCEEDED(hr))
        *out_sa = sa;

exit:
    if (FAILED(hr))
        SafeArrayDestroy(sa);

    return hr;
}

static HRESULT WINAPI uia_iface_IntSafeArrayToNativeArray(IUIAutomation6 *iface, SAFEARRAY *sa, int **out_arr,
        int *out_arr_count)
{
    LONG lbound, elems;
    int *arr, *sa_arr;
    VARTYPE vt;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", iface, sa, out_arr, out_arr_count);

    if (!sa || !out_arr || !out_arr_count)
        return E_INVALIDARG;

    *out_arr = NULL;
    hr = SafeArrayGetVartype(sa, &vt);
    if (FAILED(hr))
        return hr;

    if (vt != VT_I4)
        return E_INVALIDARG;

    hr = get_safearray_bounds(sa, &lbound, &elems);
    if (FAILED(hr))
        return hr;

    if (!(arr = CoTaskMemAlloc(elems * sizeof(*arr))))
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void **)&sa_arr);
    if (FAILED(hr))
        goto exit;

    memcpy(arr, sa_arr, sizeof(*arr) * elems);
    hr = SafeArrayUnaccessData(sa);
    if (FAILED(hr))
        goto exit;

    *out_arr = arr;
    *out_arr_count = elems;

exit:
    if (FAILED(hr))
        CoTaskMemFree(arr);

    return hr;
}

static HRESULT WINAPI uia_iface_RectToVariant(IUIAutomation6 *iface, RECT rect, VARIANT *out_var)
{
    FIXME("%p, %s, %p: stub\n", iface, wine_dbgstr_rect(&rect), out_var);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_VariantToRect(IUIAutomation6 *iface, VARIANT var, RECT *out_rect)
{
    FIXME("%p, %s, %p: stub\n", iface, debugstr_variant(&var), out_rect);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_SafeArrayToRectNativeArray(IUIAutomation6 *iface, SAFEARRAY *sa, RECT **out_rect_arr,
        int *out_rect_arr_count)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, sa, out_rect_arr, out_rect_arr_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CreateProxyFactoryEntry(IUIAutomation6 *iface, IUIAutomationProxyFactory *factory,
        IUIAutomationProxyFactoryEntry **out_entry)
{
    FIXME("%p, %p, %p: stub\n", iface, factory, out_entry);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ProxyFactoryMapping(IUIAutomation6 *iface,
        IUIAutomationProxyFactoryMapping **out_factory_map)
{
    FIXME("%p, %p: stub\n", iface, out_factory_map);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetPropertyProgrammaticName(IUIAutomation6 *iface, PROPERTYID prop_id, BSTR *out_name)
{
    FIXME("%p, %d, %p: stub\n", iface, prop_id, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_GetPatternProgrammaticName(IUIAutomation6 *iface, PATTERNID pattern_id, BSTR *out_name)
{
    FIXME("%p, %d, %p: stub\n", iface, pattern_id, out_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_PollForPotentialSupportedPatterns(IUIAutomation6 *iface, IUIAutomationElement *elem,
        SAFEARRAY **out_pattern_ids, SAFEARRAY **out_pattern_names)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem, out_pattern_ids, out_pattern_names);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_PollForPotentialSupportedProperties(IUIAutomation6 *iface, IUIAutomationElement *elem,
        SAFEARRAY **out_prop_ids, SAFEARRAY **out_prop_names)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, elem, out_prop_ids, out_prop_names);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_CheckNotSupported(IUIAutomation6 *iface, VARIANT in_val, BOOL *match)
{
    IUnknown *unk;

    TRACE("%p, %s, %p\n", iface, debugstr_variant(&in_val), match);

    *match = FALSE;
    UiaGetReservedNotSupportedValue(&unk);
    if (V_VT(&in_val) == VT_UNKNOWN && (V_UNKNOWN(&in_val) == unk))
        *match = TRUE;

    return S_OK;
}

static HRESULT WINAPI uia_iface_get_ReservedNotSupportedValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    TRACE("%p, %p\n", iface, out_unk);

    return UiaGetReservedNotSupportedValue(out_unk);
}

static HRESULT WINAPI uia_iface_get_ReservedMixedAttributeValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    TRACE("%p, %p\n", iface, out_unk);

    return UiaGetReservedMixedAttributeValue(out_unk);
}

static HRESULT WINAPI uia_iface_ElementFromIAccessible(IUIAutomation6 *iface, IAccessible *acc, int cid,
        IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %d, %p: stub\n", iface, acc, cid, out_elem);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_ElementFromIAccessibleBuildCache(IUIAutomation6 *iface, IAccessible *acc, int cid,
        IUIAutomationCacheRequest *cache_req, IUIAutomationElement **out_elem)
{
    FIXME("%p, %p, %d, %p, %p: stub\n", iface, acc, cid, cache_req, out_elem);
    return E_NOTIMPL;
}

/* IUIAutomation2 methods */
static HRESULT WINAPI uia_iface_get_AutoSetFocus(IUIAutomation6 *iface, BOOL *out_auto_set_focus)
{
    FIXME("%p, %p: stub\n", iface, out_auto_set_focus);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_AutoSetFocus(IUIAutomation6 *iface, BOOL auto_set_focus)
{
    FIXME("%p, %d: stub\n", iface, auto_set_focus);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ConnectionTimeout(IUIAutomation6 *iface, DWORD *out_timeout)
{
    FIXME("%p, %p: stub\n", iface, out_timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_ConnectionTimeout(IUIAutomation6 *iface, DWORD timeout)
{
    FIXME("%p, %ld: stub\n", iface, timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_TransactionTimeout(IUIAutomation6 *iface, DWORD *out_timeout)
{
    FIXME("%p, %p: stub\n", iface, out_timeout);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_TransactionTimeout(IUIAutomation6 *iface, DWORD timeout)
{
    FIXME("%p, %ld: stub\n", iface, timeout);
    return E_NOTIMPL;
}

/* IUIAutomation3 methods */
static HRESULT WINAPI uia_iface_AddTextEditTextChangedEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, enum TextEditChangeType change_type, IUIAutomationCacheRequest *cache_req,
        IUIAutomationTextEditTextChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %d, %p, %p: stub\n", iface, elem, scope, change_type, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveTextEditTextChangedEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationTextEditTextChangedEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation4 methods */
static HRESULT WINAPI uia_iface_AddChangesEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, int *change_types, int change_types_count, IUIAutomationCacheRequest *cache_req,
        IUIAutomationChangesEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %d, %p, %p: stub\n", iface, elem, scope, change_types, change_types_count, cache_req,
            handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveChangesEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationChangesEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation5 methods */
static HRESULT WINAPI uia_iface_AddNotificationEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        enum TreeScope scope, IUIAutomationCacheRequest *cache_req, IUIAutomationNotificationEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveNotificationEventHandler(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationNotificationEventHandler *handler)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler);
    return E_NOTIMPL;
}

/* IUIAutomation6 methods */
static HRESULT WINAPI uia_iface_CreateEventHandlerGroup(IUIAutomation6 *iface,
        IUIAutomationEventHandlerGroup **out_handler_group)
{
    FIXME("%p, %p: stub\n", iface, out_handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddEventHandlerGroup(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationEventHandlerGroup *handler_group)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveEventHandlerGroup(IUIAutomation6 *iface, IUIAutomationElement *elem,
        IUIAutomationEventHandlerGroup *handler_group)
{
    FIXME("%p, %p, %p: stub\n", iface, elem, handler_group);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ConnectionRecoveryBehavior(IUIAutomation6 *iface,
        enum ConnectionRecoveryBehaviorOptions *out_conn_recovery_opts)
{
    FIXME("%p, %p: stub\n", iface, out_conn_recovery_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_ConnectionRecoveryBehavior(IUIAutomation6 *iface,
        enum ConnectionRecoveryBehaviorOptions conn_recovery_opts)
{
    FIXME("%p, %#x: stub\n", iface, conn_recovery_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_CoalesceEvents(IUIAutomation6 *iface,
        enum CoalesceEventsOptions *out_coalesce_events_opts)
{
    FIXME("%p, %p: stub\n", iface, out_coalesce_events_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_put_CoalesceEvents(IUIAutomation6 *iface,
        enum CoalesceEventsOptions coalesce_events_opts)
{
    FIXME("%p, %#x: stub\n", iface, coalesce_events_opts);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_AddActiveTextPositionChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, enum TreeScope scope, IUIAutomationCacheRequest *cache_req,
        IUIAutomationActiveTextPositionChangedEventHandler *handler)
{
    FIXME("%p, %p, %#x, %p, %p: stub\n", iface, elem, scope, cache_req, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_RemoveActiveTextPositionChangedEventHandler(IUIAutomation6 *iface,
        IUIAutomationElement *elem, IUIAutomationActiveTextPositionChangedEventHandler *handler)
{
    FIXME("%p, %p, %p\n", iface, elem, handler);
    return E_NOTIMPL;
}

static const IUIAutomation6Vtbl uia_iface_vtbl = {
    uia_iface_QueryInterface,
    uia_iface_AddRef,
    uia_iface_Release,
    /* IUIAutomation methods */
    uia_iface_CompareElements,
    uia_iface_CompareRuntimeIds,
    uia_iface_GetRootElement,
    uia_iface_ElementFromHandle,
    uia_iface_ElementFromPoint,
    uia_iface_GetFocusedElement,
    uia_iface_GetRootElementBuildCache,
    uia_iface_ElementFromHandleBuildCache,
    uia_iface_ElementFromPointBuildCache,
    uia_iface_GetFocusedElementBuildCache,
    uia_iface_CreateTreeWalker,
    uia_iface_get_ControlViewWalker,
    uia_iface_get_ContentViewWalker,
    uia_iface_get_RawViewWalker,
    uia_iface_get_RawViewCondition,
    uia_iface_get_ControlViewCondition,
    uia_iface_get_ContentViewCondition,
    uia_iface_CreateCacheRequest,
    uia_iface_CreateTrueCondition,
    uia_iface_CreateFalseCondition,
    uia_iface_CreatePropertyCondition,
    uia_iface_CreatePropertyConditionEx,
    uia_iface_CreateAndCondition,
    uia_iface_CreateAndConditionFromArray,
    uia_iface_CreateAndConditionFromNativeArray,
    uia_iface_CreateOrCondition,
    uia_iface_CreateOrConditionFromArray,
    uia_iface_CreateOrConditionFromNativeArray,
    uia_iface_CreateNotCondition,
    uia_iface_AddAutomationEventHandler,
    uia_iface_RemoveAutomationEventHandler,
    uia_iface_AddPropertyChangedEventHandlerNativeArray,
    uia_iface_AddPropertyChangedEventHandler,
    uia_iface_RemovePropertyChangedEventHandler,
    uia_iface_AddStructureChangedEventHandler,
    uia_iface_RemoveStructureChangedEventHandler,
    uia_iface_AddFocusChangedEventHandler,
    uia_iface_RemoveFocusChangedEventHandler,
    uia_iface_RemoveAllEventHandlers,
    uia_iface_IntNativeArrayToSafeArray,
    uia_iface_IntSafeArrayToNativeArray,
    uia_iface_RectToVariant,
    uia_iface_VariantToRect,
    uia_iface_SafeArrayToRectNativeArray,
    uia_iface_CreateProxyFactoryEntry,
    uia_iface_get_ProxyFactoryMapping,
    uia_iface_GetPropertyProgrammaticName,
    uia_iface_GetPatternProgrammaticName,
    uia_iface_PollForPotentialSupportedPatterns,
    uia_iface_PollForPotentialSupportedProperties,
    uia_iface_CheckNotSupported,
    uia_iface_get_ReservedNotSupportedValue,
    uia_iface_get_ReservedMixedAttributeValue,
    uia_iface_ElementFromIAccessible,
    uia_iface_ElementFromIAccessibleBuildCache,
    /* IUIAutomation2 methods */
    uia_iface_get_AutoSetFocus,
    uia_iface_put_AutoSetFocus,
    uia_iface_get_ConnectionTimeout,
    uia_iface_put_ConnectionTimeout,
    uia_iface_get_TransactionTimeout,
    uia_iface_put_TransactionTimeout,
    /* IUIAutomation3 methods */
    uia_iface_AddTextEditTextChangedEventHandler,
    uia_iface_RemoveTextEditTextChangedEventHandler,
    /* IUIAutomation4 methods */
    uia_iface_AddChangesEventHandler,
    uia_iface_RemoveChangesEventHandler,
    /* IUIAutomation5 methods */
    uia_iface_AddNotificationEventHandler,
    uia_iface_RemoveNotificationEventHandler,
    /* IUIAutomation6 methods */
    uia_iface_CreateEventHandlerGroup,
    uia_iface_AddEventHandlerGroup,
    uia_iface_RemoveEventHandlerGroup,
    uia_iface_get_ConnectionRecoveryBehavior,
    uia_iface_put_ConnectionRecoveryBehavior,
    uia_iface_get_CoalesceEvents,
    uia_iface_put_CoalesceEvents,
    uia_iface_AddActiveTextPositionChangedEventHandler,
    uia_iface_RemoveActiveTextPositionChangedEventHandler,
};

HRESULT create_uia_iface(IUnknown **iface, BOOL is_cui8)
{
    struct uia_iface *uia;

    uia = heap_alloc_zero(sizeof(*uia));
    if (!uia)
        return E_OUTOFMEMORY;

    uia->IUIAutomation6_iface.lpVtbl = &uia_iface_vtbl;
    uia->is_cui8 = is_cui8;
    uia->ref = 1;

    *iface = (IUnknown *)&uia->IUIAutomation6_iface;
    return S_OK;
}
