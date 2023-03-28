/* WinRT Windows.Media.SpeechRecognition implementation
 *
 * Copyright 2022 Bernhard Kölbl
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

#include "private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

/*
 *
 * SpeechRecognitionListConstraint
 *
 */

struct list_constraint
{
    ISpeechRecognitionListConstraint ISpeechRecognitionListConstraint_iface;
    ISpeechRecognitionConstraint ISpeechRecognitionConstraint_iface;
    LONG ref;

    BOOLEAN enabled;
    IVector_HSTRING *commands;
};

/*
 *
 * ISpeechRecognitionListConstraint
 *
 */

static inline struct list_constraint *impl_from_ISpeechRecognitionListConstraint( ISpeechRecognitionListConstraint *iface )
{
    return CONTAINING_RECORD(iface, struct list_constraint, ISpeechRecognitionListConstraint_iface);
}

static HRESULT WINAPI list_constraint_QueryInterface( ISpeechRecognitionListConstraint *iface, REFIID iid, void **out )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionListConstraint(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionListConstraint))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionListConstraint_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognitionConstraint))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionConstraint_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI list_constraint_AddRef( ISpeechRecognitionListConstraint *iface )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionListConstraint(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI list_constraint_Release( ISpeechRecognitionListConstraint *iface )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionListConstraint(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        IVector_HSTRING_Release(impl->commands);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI list_constraint_GetIids( ISpeechRecognitionListConstraint *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI list_constraint_GetRuntimeClassName( ISpeechRecognitionListConstraint *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI list_constraint_GetTrustLevel( ISpeechRecognitionListConstraint *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI list_constraint_get_Commands(  ISpeechRecognitionListConstraint *iface, IVector_HSTRING **value )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionListConstraint(iface);
    IIterable_HSTRING *iterable;
    HRESULT hr;

    TRACE("iface %p, value %p.\n", iface, value);

    hr = IVector_HSTRING_QueryInterface(impl->commands, &IID_IIterable_HSTRING, (void **)&iterable);
    if (FAILED(hr)) return hr;

    hr = vector_hstring_create_copy(iterable, value);
    IIterable_HSTRING_Release(iterable);

    return hr;
}

static const struct ISpeechRecognitionListConstraintVtbl speech_recognition_list_constraint_vtbl =
{
    /* IUnknown methods */
    list_constraint_QueryInterface,
    list_constraint_AddRef,
    list_constraint_Release,
    /* IInspectable methods */
    list_constraint_GetIids,
    list_constraint_GetRuntimeClassName,
    list_constraint_GetTrustLevel,
    /* ISpeechRecognitionListConstraint methods */
    list_constraint_get_Commands,
};

/*
 *
 * ISpeechRecognitionConstraint
 *
 */

DEFINE_IINSPECTABLE(constraint, ISpeechRecognitionConstraint, struct list_constraint, ISpeechRecognitionListConstraint_iface)

static HRESULT WINAPI constraint_get_IsEnabled( ISpeechRecognitionConstraint *iface, BOOLEAN *value )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionConstraint(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    *value = impl->enabled;
    return S_OK;
}

static HRESULT WINAPI constraint_put_IsEnabled( ISpeechRecognitionConstraint *iface, BOOLEAN value )
{
    struct list_constraint *impl = impl_from_ISpeechRecognitionConstraint(iface);
    TRACE("iface %p, value %u.\n", iface, value);
    impl->enabled = value;
    return S_OK;
}

static HRESULT WINAPI constraint_get_Tag( ISpeechRecognitionConstraint *iface, HSTRING *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI constraint_put_Tag( ISpeechRecognitionConstraint *iface, HSTRING value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI constraint_get_Type( ISpeechRecognitionConstraint *iface, SpeechRecognitionConstraintType *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI constraint_get_Probability( ISpeechRecognitionConstraint *iface, SpeechRecognitionConstraintProbability *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI constraint_put_Probability( ISpeechRecognitionConstraint *iface, SpeechRecognitionConstraintProbability value )
{
    FIXME("iface %p, value %u stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognitionConstraintVtbl speech_recognition_constraint_vtbl =
{
    /* IUnknown methods */
    constraint_QueryInterface,
    constraint_AddRef,
    constraint_Release,
    /* IInspectable methods */
    constraint_GetIids,
    constraint_GetRuntimeClassName,
    constraint_GetTrustLevel,
    /* ISpeechRecognitionConstraint methods */
    constraint_get_IsEnabled,
    constraint_put_IsEnabled,
    constraint_get_Tag,
    constraint_put_Tag,
    constraint_get_Type,
    constraint_get_Probability,
    constraint_put_Probability,
};

/*
 *
 * Statics for SpeechRecognitionListConstraint
 *
 */

struct listconstraint_statics
{
    IActivationFactory IActivationFactory_iface;
    ISpeechRecognitionListConstraintFactory ISpeechRecognitionListConstraintFactory_iface;
    LONG ref;
};

/*
 *
 * IActivationFactory
 *
 */

static inline struct listconstraint_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD(iface, struct listconstraint_statics, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct listconstraint_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IInspectable_AddRef((*out = &impl->IActivationFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognitionListConstraintFactory))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionListConstraintFactory_iface));
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef( IActivationFactory *iface )
{
    struct listconstraint_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI factory_Release( IActivationFactory *iface )
{
    struct listconstraint_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    TRACE("iface %p, instance %p\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    /* IUnknown methods */
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

/*
 *
 * ISpeechRecognitionListConstraintFactory
 *
 */

DEFINE_IINSPECTABLE(constraint_factory, ISpeechRecognitionListConstraintFactory, struct listconstraint_statics, IActivationFactory_iface)

static HRESULT WINAPI constraint_factory_Create( ISpeechRecognitionListConstraintFactory *iface,
                                                 IIterable_HSTRING *commands,
                                                 ISpeechRecognitionListConstraint** listconstraint )
{
    TRACE("iface %p, commands %p, listconstraint %p.\n", iface, commands, listconstraint);
    return ISpeechRecognitionListConstraintFactory_CreateWithTag(iface, commands, NULL, listconstraint);
}

static HRESULT WINAPI constraint_factory_CreateWithTag( ISpeechRecognitionListConstraintFactory *iface,
                                                        IIterable_HSTRING *commands,
                                                        HSTRING tag,
                                                        ISpeechRecognitionListConstraint** listconstraint )
{
    struct list_constraint *impl;
    HRESULT hr;

    TRACE("iface %p, commands %p, tag %p, listconstraint %p.\n", iface, commands, tag, listconstraint);

    *listconstraint = NULL;

    if (!commands)
        return E_POINTER;

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    if (FAILED(hr = vector_hstring_create_copy(commands, &impl->commands))) goto error;

    impl->ISpeechRecognitionListConstraint_iface.lpVtbl = &speech_recognition_list_constraint_vtbl;
    impl->ISpeechRecognitionConstraint_iface.lpVtbl = &speech_recognition_constraint_vtbl;
    impl->ref = 1;

    TRACE("created SpeechRecognitionListConstraint %p.\n", impl);

    *listconstraint = &impl->ISpeechRecognitionListConstraint_iface;
    return S_OK;

error:
    if (impl->commands) IVector_HSTRING_Release(impl->commands);
    free(impl);
    return hr;
}

static const struct ISpeechRecognitionListConstraintFactoryVtbl speech_recognition_list_constraint_factory_vtbl =
{
    /* IUnknown methods */
    constraint_factory_QueryInterface,
    constraint_factory_AddRef,
    constraint_factory_Release,
    /* IInspectable methods */
    constraint_factory_GetIids,
    constraint_factory_GetRuntimeClassName,
    constraint_factory_GetTrustLevel,
    /* ISpeechRecognitionListConstraintFactory methods */
    constraint_factory_Create,
    constraint_factory_CreateWithTag,
};

/*
 *
 * ActivationFactory instances
 *
 */

static struct listconstraint_statics listconstraint_statics =
{
    .IActivationFactory_iface = {&activation_factory_vtbl},
    .ISpeechRecognitionListConstraintFactory_iface = {&speech_recognition_list_constraint_factory_vtbl},
    .ref = 1
};

IActivationFactory *listconstraint_factory  = &listconstraint_statics.IActivationFactory_iface;
