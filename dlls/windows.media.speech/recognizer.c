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
 * Statics for SpeechRecognizer
 *
 */

struct recognizer_statics
{
    IActivationFactory IActivationFactory_iface;
    ISpeechRecognizerFactory ISpeechRecognizerFactory_iface;
    ISpeechRecognizerStatics ISpeechRecognizerStatics_iface;
    ISpeechRecognizerStatics2 ISpeechRecognizerStatics2_iface;
    LONG ref;
};

/*
 *
 * IActivationFactory
 *
 */

static inline struct recognizer_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer_statics, IActivationFactory_iface);
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IInspectable_AddRef((*out = &impl->IActivationFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerFactory))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics2_iface));
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    TRACE("iface %p, instance %p\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    /* IUnknown methods */
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

/*
 *
 * ISpeechRecognizerFactory
 *
 */

DEFINE_IINSPECTABLE(recognizer_factory, ISpeechRecognizerFactory, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI recognizer_factory_Create( ISpeechRecognizerFactory *iface, ILanguage *language, ISpeechRecognizer **speechrecognizer )
{
    TRACE("iface %p, language %p, speechrecognizer %p.\n", iface, language, speechrecognizer);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerFactoryVtbl speech_recognizer_factory_vtbl =
{
    /* IUnknown methods */
    recognizer_factory_QueryInterface,
    recognizer_factory_AddRef,
    recognizer_factory_Release,
    /* IInspectable methods */
    recognizer_factory_GetIids,
    recognizer_factory_GetRuntimeClassName,
    recognizer_factory_GetTrustLevel,
    /* ISpeechRecognizerFactory methods */
    recognizer_factory_Create
};

/*
 *
 * ISpeechRecognizerStatics
 *
 */

DEFINE_IINSPECTABLE(statics, ISpeechRecognizerStatics, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics_get_SystemSpeechLanguage( ISpeechRecognizerStatics *iface, ILanguage **language )
{
    FIXME("iface %p, language %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedTopicLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedGrammarLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStaticsVtbl speech_recognizer_statics_vtbl =
{
    /* IUnknown methods */
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics_get_SystemSpeechLanguage,
    statics_get_SupportedTopicLanguages,
    statics_get_SupportedGrammarLanguages
};

/*
 *
 * ISpeechRecognizerStatics2
 *
 */

DEFINE_IINSPECTABLE(statics2, ISpeechRecognizerStatics2, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics2_TrySetSystemSpeechLanguageAsync( ISpeechRecognizerStatics2 *iface,
                                                                ILanguage *language,
                                                                IAsyncOperation_boolean **operation)
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStatics2Vtbl speech_recognizer_statics2_vtbl =
{
    /* IUnknown methods */
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics2_TrySetSystemSpeechLanguageAsync,
};

static struct recognizer_statics recognizer_statics =
{
    .IActivationFactory_iface = {&activation_factory_vtbl},
    .ISpeechRecognizerFactory_iface = {&speech_recognizer_factory_vtbl},
    .ISpeechRecognizerStatics_iface = {&speech_recognizer_statics_vtbl},
    .ISpeechRecognizerStatics2_iface = {&speech_recognizer_statics2_vtbl},
    .ref = 1
};

IActivationFactory *recognizer_factory = &recognizer_statics.IActivationFactory_iface;
