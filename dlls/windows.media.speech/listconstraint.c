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
    return E_NOTIMPL;
}

static HRESULT WINAPI constraint_factory_CreateWithTag( ISpeechRecognitionListConstraintFactory *iface,
                                                        IIterable_HSTRING *commands,
                                                        HSTRING tag,
                                                        ISpeechRecognitionListConstraint** listconstraint )
{
    TRACE("iface %p, commands %p, tag %p, listconstraint %p.\n", iface, commands, tag, listconstraint);
    return E_NOTIMPL;
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
