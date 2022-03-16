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
 * SpeechContinuousRecognitionSession
 *
 */

struct session
{
    ISpeechContinuousRecognitionSession ISpeechContinuousRecognitionSession_iface;
    LONG ref;

    struct list completed_handlers;
    struct list result_handlers;
};

/*
 *
 * ISpeechContinuousRecognitionSession
 *
 */

static inline struct session *impl_from_ISpeechContinuousRecognitionSession( ISpeechContinuousRecognitionSession *iface )
{
    return CONTAINING_RECORD(iface, struct session, ISpeechContinuousRecognitionSession_iface);
}

static HRESULT WINAPI session_QueryInterface( ISpeechContinuousRecognitionSession *iface, REFIID iid, void **out )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISpeechContinuousRecognitionSession))
    {
        IInspectable_AddRef((*out = &impl->ISpeechContinuousRecognitionSession_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_AddRef( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI session_Release( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        typed_event_handlers_clear(&impl->completed_handlers);
        typed_event_handlers_clear(&impl->result_handlers);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI session_GetIids( ISpeechContinuousRecognitionSession *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetRuntimeClassName( ISpeechContinuousRecognitionSession *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetTrustLevel( ISpeechContinuousRecognitionSession *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_get_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_set_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan value )
{
    FIXME("iface %p, value %#I64x stub!\n", iface, value.Duration);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_StartAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p semi stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_StartWithModeAsync( ISpeechContinuousRecognitionSession *iface,
                                                  SpeechContinuousRecognitionMode mode,
                                                  IAsyncAction **action )
{
    FIXME("iface %p, mode %u, action %p stub!\n", iface, mode, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_StopAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_CancelAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_PauseAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_Resume( ISpeechContinuousRecognitionSession *iface )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_add_Completed( ISpeechContinuousRecognitionSession *iface,
                                             ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs *handler,
                                             EventRegistrationToken *token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->completed_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_Completed( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->completed_handlers, &token);
}

static HRESULT WINAPI session_add_ResultGenerated( ISpeechContinuousRecognitionSession *iface,
                                                   ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs *handler,
                                                   EventRegistrationToken *token)
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->result_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_ResultGenerated( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->result_handlers, &token);
}

static const struct ISpeechContinuousRecognitionSessionVtbl session_vtbl =
{
    /* IUnknown methods */
    session_QueryInterface,
    session_AddRef,
    session_Release,
    /* IInspectable methods */
    session_GetIids,
    session_GetRuntimeClassName,
    session_GetTrustLevel,
    /* ISpeechContinuousRecognitionSession methods */
    session_get_AutoStopSilenceTimeout,
    session_set_AutoStopSilenceTimeout,
    session_StartAsync,
    session_StartWithModeAsync,
    session_StopAsync,
    session_CancelAsync,
    session_PauseAsync,
    session_Resume,
    session_add_Completed,
    session_remove_Completed,
    session_add_ResultGenerated,
    session_remove_ResultGenerated
};

/*
 *
 * SpeechRecognizer
 *
 */

struct recognizer
{
    ISpeechRecognizer ISpeechRecognizer_iface;
    IClosable IClosable_iface;
    ISpeechRecognizer2 ISpeechRecognizer2_iface;
    LONG ref;

    ISpeechContinuousRecognitionSession *session;
};

/*
 *
 * ISpeechRecognizer
 *
 */

static inline struct recognizer *impl_from_ISpeechRecognizer( ISpeechRecognizer *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer, ISpeechRecognizer_iface);
}

static HRESULT WINAPI recognizer_QueryInterface( ISpeechRecognizer *iface, REFIID iid, void **out )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognizer))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IInspectable_AddRef((*out = &impl->IClosable_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizer2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer2_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognizer_AddRef( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognizer_Release( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        ISpeechContinuousRecognitionSession_Release(impl->session);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognizer_GetIids( ISpeechRecognizer *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetRuntimeClassName( ISpeechRecognizer *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetTrustLevel( ISpeechRecognizer *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Constraints( ISpeechRecognizer *iface, IVector_ISpeechRecognitionConstraint **vector )
{
    FIXME("iface %p, operation %p stub!\n", iface, vector);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_CurrentLanguage( ISpeechRecognizer *iface, ILanguage **language )
{
    FIXME("iface %p, operation %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Timeouts( ISpeechRecognizer *iface, ISpeechRecognizerTimeouts **timeouts )
{
    FIXME("iface %p, operation %p stub!\n", iface, timeouts);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_UIOptions( ISpeechRecognizer *iface, ISpeechRecognizerUIOptions **options )
{
    FIXME("iface %p, operation %p stub!\n", iface, options);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_CompileConstraintsAsync( ISpeechRecognizer *iface,
                                                          IAsyncOperation_SpeechRecognitionCompilationResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_RecognizeAsync( ISpeechRecognizer *iface,
                                                 IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_RecognizeWithUIAsync( ISpeechRecognizer *iface,
                                                       IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_RecognitionQualityDegrading( ISpeechRecognizer *iface,
                                                                  ITypedEventHandler_SpeechRecognizer_SpeechRecognitionQualityDegradingEventArgs *handler,
                                                                  EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_RecognitionQualityDegrading( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_StateChanged( ISpeechRecognizer *iface,
                                                   ITypedEventHandler_SpeechRecognizer_SpeechRecognizerStateChangedEventArgs *handler,
                                                   EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_StateChanged( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerVtbl speech_recognizer_vtbl =
{
    /* IUnknown methods */
    recognizer_QueryInterface,
    recognizer_AddRef,
    recognizer_Release,
    /* IInspectable methods */
    recognizer_GetIids,
    recognizer_GetRuntimeClassName,
    recognizer_GetTrustLevel,
    /* ISpeechRecognizer methods */
    recognizer_get_CurrentLanguage,
    recognizer_get_Constraints,
    recognizer_get_Timeouts,
    recognizer_get_UIOptions,
    recognizer_CompileConstraintsAsync,
    recognizer_RecognizeAsync,
    recognizer_RecognizeWithUIAsync,
    recognizer_add_RecognitionQualityDegrading,
    recognizer_remove_RecognitionQualityDegrading,
    recognizer_add_StateChanged,
    recognizer_remove_StateChanged,
};

/*
 *
 * IClosable
 *
 */

DEFINE_IINSPECTABLE(closable, IClosable, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI closable_Close( IClosable *iface )
{
    FIXME("iface %p stub.\n", iface);
    return E_NOTIMPL;
}

static const struct IClosableVtbl closable_vtbl =
{
    /* IUnknown methods */
    closable_QueryInterface,
    closable_AddRef,
    closable_Release,
    /* IInspectable methods */
    closable_GetIids,
    closable_GetRuntimeClassName,
    closable_GetTrustLevel,
    /* IClosable methods */
    closable_Close,
};

/*
 *
 * ISpeechRecognizer2
 *
 */

DEFINE_IINSPECTABLE(recognizer2, ISpeechRecognizer2, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI recognizer2_get_ContinuousRecognitionSession( ISpeechRecognizer2 *iface,
                                                                    ISpeechContinuousRecognitionSession **session )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer2(iface);
    TRACE("iface %p, session %p.\n", iface, session);
    ISpeechContinuousRecognitionSession_QueryInterface(impl->session, &IID_ISpeechContinuousRecognitionSession, (void **)session);
    return S_OK;
}

static HRESULT WINAPI recognizer2_get_State( ISpeechRecognizer2 *iface, SpeechRecognizerState *state )
{
    FIXME("iface %p, state %p stub!\n", iface, state);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_StopRecognitionAsync( ISpeechRecognizer2 *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_add_HypothesisGenerated( ISpeechRecognizer2 *iface,
                                                           ITypedEventHandler_SpeechRecognizer_SpeechRecognitionHypothesisGeneratedEventArgs *handler,
                                                           EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_remove_HypothesisGenerated( ISpeechRecognizer2 *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizer2Vtbl speech_recognizer2_vtbl =
{
    /* IUnknown methods */
    recognizer2_QueryInterface,
    recognizer2_AddRef,
    recognizer2_Release,
    /* IInspectable methods */
    recognizer2_GetIids,
    recognizer2_GetRuntimeClassName,
    recognizer2_GetTrustLevel,
    /* ISpeechRecognizer2 methods */
    recognizer2_get_ContinuousRecognitionSession,
    recognizer2_get_State,
    recognizer2_StopRecognitionAsync,
    recognizer2_add_HypothesisGenerated,
    recognizer2_remove_HypothesisGenerated,
};

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
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, instance %p.\n", iface, instance);
    return ISpeechRecognizerFactory_Create(&impl->ISpeechRecognizerFactory_iface, NULL, (ISpeechRecognizer **)instance);
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
    struct recognizer *impl;
    struct session *session;

    TRACE("iface %p, language %p, speechrecognizer %p.\n", iface, language, speechrecognizer);

    *speechrecognizer = NULL;

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    if (!(session = calloc(1, sizeof(*session)))) return E_OUTOFMEMORY;

    if (language)
        FIXME("language parameter unused. Stub!\n");

    session->ISpeechContinuousRecognitionSession_iface.lpVtbl = &session_vtbl;
    session->ref = 1;
    list_init(&session->completed_handlers);
    list_init(&session->result_handlers);

    impl->ISpeechRecognizer_iface.lpVtbl = &speech_recognizer_vtbl;
    impl->IClosable_iface.lpVtbl = &closable_vtbl;
    impl->ISpeechRecognizer2_iface.lpVtbl = &speech_recognizer2_vtbl;
    impl->session = &session->ISpeechContinuousRecognitionSession_iface;
    impl->ref = 1;

    TRACE("created SpeechRecognizer %p.\n", impl);

    *speechrecognizer = &impl->ISpeechRecognizer_iface;
    return S_OK;
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
