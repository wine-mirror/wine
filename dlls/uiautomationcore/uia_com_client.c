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
    FIXME("%p, %p, %p: stub\n", iface, hwnd, out_elem);
    return E_NOTIMPL;
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
    FIXME("%p, %p, %d, %p: stub\n", iface, arr, arr_count, out_sa);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_IntSafeArrayToNativeArray(IUIAutomation6 *iface, SAFEARRAY *sa, int **out_arr,
        int *out_arr_count)
{
    FIXME("%p, %p, %p, %p: stub\n", iface, sa, out_arr, out_arr_count);
    return E_NOTIMPL;
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
    FIXME("%p, %s, %p: stub\n", iface, debugstr_variant(&in_val), match);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ReservedNotSupportedValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    FIXME("%p, %p: stub\n", iface, out_unk);
    return E_NOTIMPL;
}

static HRESULT WINAPI uia_iface_get_ReservedMixedAttributeValue(IUIAutomation6 *iface, IUnknown **out_unk)
{
    FIXME("%p, %p: stub\n", iface, out_unk);
    return E_NOTIMPL;
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
