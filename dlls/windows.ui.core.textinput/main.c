/* WinRT Windows.UI.ViewManagement.Core.CoreInputView Implementation
 *
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

#include "initguid.h"
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(coreinputview);

struct core_input_view
{
    ICoreInputView ICoreInputView_iface;
    ICoreInputView2 ICoreInputView2_iface;
    ICoreInputView3 ICoreInputView3_iface;
    ICoreInputView4 ICoreInputView4_iface;
    LONG ref;
};

static inline struct core_input_view *impl_from_ICoreInputView(ICoreInputView *iface)
{
    return CONTAINING_RECORD(iface, struct core_input_view, ICoreInputView_iface);
}

static HRESULT WINAPI core_input_view_QueryInterface(ICoreInputView *iface, REFIID iid, void **out)
{
    struct core_input_view *impl = impl_from_ICoreInputView(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    *out = NULL;

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_ICoreInputView))
    {
        *out = &impl->ICoreInputView_iface;
    }
    else if (IsEqualGUID(iid, &IID_ICoreInputView2))
    {
        *out = &impl->ICoreInputView2_iface;
    }
    else if (IsEqualGUID(iid, &IID_ICoreInputView3))
    {
        *out = &impl->ICoreInputView3_iface;
    }
    else if (IsEqualGUID(iid, &IID_ICoreInputView4))
    {
        *out = &impl->ICoreInputView4_iface;
    }

    if (*out)
    {
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI core_input_view_AddRef(ICoreInputView *iface)
{
    struct core_input_view *impl = impl_from_ICoreInputView(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI core_input_view_Release(ICoreInputView *iface)
{
    struct core_input_view *impl = impl_from_ICoreInputView(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(impl);
    return ref;
}

static HRESULT WINAPI core_input_view_GetIids(ICoreInputView *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_GetRuntimeClassName(ICoreInputView *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_GetTrustLevel(ICoreInputView *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_add_OcclusionsChanged(ICoreInputView *iface,
                                                            ITypedEventHandler_CoreInputView_CoreInputViewOcclusionsChangedEventArgs *handler,
                                                            EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_remove_OcclusionsChanged(ICoreInputView *iface,
                                                               EventRegistrationToken token)
{
    FIXME("iface %p, token %I64x stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_GetCoreInputViewOcclusions(ICoreInputView *iface,
                                                                 IVectorView_CoreInputViewOcclusion **result)
{
    static const struct vector_iids iids =
    {
        .vector = &IID_IVector_CoreInputViewOcclusion,
        .view = &IID_IVectorView_CoreInputViewOcclusion,
        .iterable = &IID_IIterable_CoreInputViewOcclusion,
        .iterator = &IID_IIterator_CoreInputViewOcclusion,
    };
    IVector_CoreInputViewOcclusion *vector;
    HRESULT hr;

    FIXME("iface %p, result %p stub!\n", iface, result);

    if (SUCCEEDED(hr = vector_create(&iids, (void **)&vector)))
    {
        hr = IVector_CoreInputViewOcclusion_GetView(vector, result);
        IVector_CoreInputViewOcclusion_Release(vector);
    }

    return hr;
}

static HRESULT WINAPI core_input_view_TryShowPrimaryView(ICoreInputView *iface, boolean *result)
{
    FIXME("iface %p, boolean %p stub!\n", iface, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view_TryHidePrimaryView(ICoreInputView *iface, boolean *result)
{
    FIXME("iface %p, boolean %p stub!\n", iface, result);
    return E_NOTIMPL;
}

static const struct ICoreInputViewVtbl core_input_view_vtbl =
{
    core_input_view_QueryInterface,
    core_input_view_AddRef,
    core_input_view_Release,
    /* IInspectable methods */
    core_input_view_GetIids,
    core_input_view_GetRuntimeClassName,
    core_input_view_GetTrustLevel,
    /* ICoreInputView methods */
    core_input_view_add_OcclusionsChanged,
    core_input_view_remove_OcclusionsChanged,
    core_input_view_GetCoreInputViewOcclusions,
    core_input_view_TryShowPrimaryView,
    core_input_view_TryHidePrimaryView,
};

DEFINE_IINSPECTABLE(core_input_view2, ICoreInputView2, struct core_input_view, ICoreInputView_iface)

static HRESULT WINAPI core_input_view2_add_XYFocusTransferringFromPrimaryView(ICoreInputView2 *iface,
                                                                              ITypedEventHandler_CoreInputView_CoreInputViewTransferringXYFocusEventArgs *handler,
                                                                              EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view2_remove_XYFocusTransferringFromPrimaryView(ICoreInputView2 *iface,
                                                                                 EventRegistrationToken token)
{
    FIXME("iface %p, token %I64x stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view2_add_XYFocusTransferredToPrimaryView(ICoreInputView2 *iface,
                                                                           ITypedEventHandler_CoreInputView_IInspectable *handler,
                                                                           EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view2_remove_XYFocusTransferredToPrimaryView(ICoreInputView2 *iface,
                                                                              EventRegistrationToken token)
{
    FIXME("iface %p, token %I64x stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view2_TryTransferXYFocusToPrimaryView(ICoreInputView2 *iface,
                                                                       Rect origin,
                                                                       CoreInputViewXYFocusTransferDirection direction,
                                                                       boolean *result)
{
    FIXME("iface %p, origin (%f,%f %fx%f), direction %d, result %p stub!\n", iface, origin.X,
          origin.Y, origin.Width, origin.Height, direction, result);
    return E_NOTIMPL;
}

static const struct ICoreInputView2Vtbl core_input_view2_vtbl =
{
    core_input_view2_QueryInterface,
    core_input_view2_AddRef,
    core_input_view2_Release,
    /* IInspectable methods */
    core_input_view2_GetIids,
    core_input_view2_GetRuntimeClassName,
    core_input_view2_GetTrustLevel,
    /* ICoreInputView2 methods */
    core_input_view2_add_XYFocusTransferringFromPrimaryView,
    core_input_view2_remove_XYFocusTransferringFromPrimaryView,
    core_input_view2_add_XYFocusTransferredToPrimaryView,
    core_input_view2_remove_XYFocusTransferredToPrimaryView,
    core_input_view2_TryTransferXYFocusToPrimaryView,
};

DEFINE_IINSPECTABLE(core_input_view3, ICoreInputView3, struct core_input_view, ICoreInputView_iface)

static HRESULT WINAPI core_input_view3_TryShow(ICoreInputView3 *iface, boolean *result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view3_TryShowWithKind(ICoreInputView3 *iface,
                                                       CoreInputViewKind type,
                                                       boolean *result)
{
    FIXME("iface %p, type %d, result %p stub!\n", iface, type, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view3_TryHide(ICoreInputView3 *iface, boolean *result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);
    return E_NOTIMPL;
}

static const struct ICoreInputView3Vtbl core_input_view3_vtbl =
{
    core_input_view3_QueryInterface,
    core_input_view3_AddRef,
    core_input_view3_Release,
    /* IInspectable methods */
    core_input_view3_GetIids,
    core_input_view3_GetRuntimeClassName,
    core_input_view3_GetTrustLevel,
    /* ICoreInputView3 methods */
    core_input_view3_TryShow,
    core_input_view3_TryShowWithKind,
    core_input_view3_TryHide,
};

DEFINE_IINSPECTABLE(core_input_view4, ICoreInputView4, struct core_input_view, ICoreInputView_iface)

static HRESULT WINAPI core_input_view4_add_PrimaryViewShowing(ICoreInputView4 *iface,
                                                              ITypedEventHandler_CoreInputView_CoreInputViewShowingEventArgs *handler,
                                                              EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view4_remove_PrimaryViewShowing(ICoreInputView4 *iface,
                                                                 EventRegistrationToken token)
{
    FIXME("iface %p, token %I64x stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view4_add_PrimaryViewHiding(ICoreInputView4 *iface,
                                                             ITypedEventHandler_CoreInputView_CoreInputViewHidingEventArgs *handler,
                                                             EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI core_input_view4_remove_PrimaryViewHiding(ICoreInputView4 *iface,
                                                                EventRegistrationToken token)
{
    FIXME("iface %p, token %I64x stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ICoreInputView4Vtbl core_input_view4_vtbl =
{
    core_input_view4_QueryInterface,
    core_input_view4_AddRef,
    core_input_view4_Release,
    /* IInspectable methods */
    core_input_view4_GetIids,
    core_input_view4_GetRuntimeClassName,
    core_input_view4_GetTrustLevel,
    /* ICoreInputView4 methods */
    core_input_view4_add_PrimaryViewShowing,
    core_input_view4_remove_PrimaryViewShowing,
    core_input_view4_add_PrimaryViewHiding,
    core_input_view4_remove_PrimaryViewHiding,
};

struct core_input_view_statics
{
    IActivationFactory IActivationFactory_iface;
    ICoreInputViewStatics ICoreInputViewStatics_iface;
    LONG ref;
};

static inline struct core_input_view_statics *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct core_input_view_statics, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IActivationFactory *iface, REFIID iid, void **out)
{
    struct core_input_view_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        *out = &impl->IActivationFactory_iface;
        IActivationFactory_AddRef(*out);
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ICoreInputViewStatics))
    {
        *out = &impl->ICoreInputViewStatics_iface;
        ICoreInputViewStatics_AddRef(*out);
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct core_input_view_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct core_input_view_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI factory_GetIids(IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

DEFINE_IINSPECTABLE(core_input_view_statics, ICoreInputViewStatics, struct core_input_view_statics,
                    IActivationFactory_iface)

static HRESULT WINAPI core_input_view_statics_GetForCurrentView(ICoreInputViewStatics *iface,
                                                                ICoreInputView **result)
{
    struct core_input_view *view;

    FIXME("iface %p, result %p semi-stub.\n", iface, result);

    if (!(view = calloc(1, sizeof(*view))))
    {
        *result = NULL;
        return E_OUTOFMEMORY;
    }

    view->ICoreInputView_iface.lpVtbl = &core_input_view_vtbl;
    view->ICoreInputView2_iface.lpVtbl = &core_input_view2_vtbl;
    view->ICoreInputView3_iface.lpVtbl = &core_input_view3_vtbl;
    view->ICoreInputView4_iface.lpVtbl = &core_input_view4_vtbl;
    view->ref = 1;

    *result = &view->ICoreInputView_iface;
    return S_OK;
}

static const struct ICoreInputViewStaticsVtbl core_input_view_statics_vtbl =
{
    core_input_view_statics_QueryInterface,
    core_input_view_statics_AddRef,
    core_input_view_statics_Release,
    /* IInspectable methods */
    core_input_view_statics_GetIids,
    core_input_view_statics_GetRuntimeClassName,
    core_input_view_statics_GetTrustLevel,
    /* ICoreInputViewStatics methods */
    core_input_view_statics_GetForCurrentView,
};

static struct core_input_view_statics core_input_view_statics =
{
    {&factory_vtbl},
    {&core_input_view_statics_vtbl},
    1,
};

static IActivationFactory *core_input_view_factory = &core_input_view_statics.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    const WCHAR *name = WindowsGetStringRawBuffer(classid, NULL);

    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);

    *factory = NULL;

    if (!wcscmp(name, RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView))
        IActivationFactory_QueryInterface(core_input_view_factory, &IID_IActivationFactory, (void **)factory);

    return *factory ? S_OK : CLASS_E_CLASSNOTAVAILABLE;
}
