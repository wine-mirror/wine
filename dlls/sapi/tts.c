/*
 * Speech API (SAPI) text-to-speech implementation.
 *
 * Copyright 2019 Jactry Zeng for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "sapiddk.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct speech_voice
{
    ISpeechVoice ISpeechVoice_iface;
    LONG ref;
};

static inline struct speech_voice *impl_from_ISpeechVoice(ISpeechVoice *iface)
{
    return CONTAINING_RECORD(iface, struct speech_voice, ISpeechVoice_iface);
}

/* ISpeechVoice interface */
static HRESULT WINAPI speech_voice_QueryInterface(ISpeechVoice *iface, REFIID iid, void **obj)
{
    struct speech_voice *This = impl_from_ISpeechVoice(iface);

    TRACE("(%p, %s %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IDispatch) ||
        IsEqualIID(iid, &IID_ISpeechVoice))
        *obj = &This->ISpeechVoice_iface;
    else
    {
        *obj = NULL;
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI speech_voice_AddRef(ISpeechVoice *iface)
{
    struct speech_voice *This = impl_from_ISpeechVoice(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI speech_voice_Release(ISpeechVoice *iface)
{
    struct speech_voice *This = impl_from_ISpeechVoice(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%u.\n", iface, ref);

    if (!ref)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI speech_voice_GetTypeInfoCount(ISpeechVoice *iface, UINT *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_GetTypeInfo(ISpeechVoice *iface, UINT info, LCID lcid,
                                               ITypeInfo **type_info)
{
    FIXME("(%p, %u, %u, %p): stub.\n", iface, info, lcid, type_info);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_GetIDsOfNames(ISpeechVoice *iface, REFIID riid, LPOLESTR *names,
                                                 UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("(%p, %s, %p, %u, %u, %p): stub.\n", iface, debugstr_guid(riid), names, count, lcid, dispid);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Invoke(ISpeechVoice *iface, DISPID dispid, REFIID riid, LCID lcid,
                                          WORD flags, DISPPARAMS *params, VARIANT *result,
                                          EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("(%p, %d, %s, %#x, %#x, %p, %p, %p, %p): stub.\n", iface, dispid, debugstr_guid(riid),
          lcid, flags, params, result, excepinfo, argerr);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Status(ISpeechVoice *iface, ISpeechVoiceStatus **status)
{
    FIXME("(%p, %p): stub.\n", iface, status);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Voice(ISpeechVoice *iface, ISpeechObjectToken **voice)
{
    FIXME("(%p, %p): stub.\n", iface, voice);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_putref_Voice(ISpeechVoice *iface, ISpeechObjectToken *voice)
{
    FIXME("(%p, %p): stub.\n", iface, voice);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_AudioOutput(ISpeechVoice *iface, ISpeechObjectToken **output)
{
    FIXME("(%p, %p): stub.\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_putref_AudioOutput(ISpeechVoice *iface, ISpeechObjectToken *output)
{
    FIXME("(%p, %p): stub.\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_AudioOutputStream(ISpeechVoice *iface, ISpeechBaseStream **output)
{
    FIXME("(%p, %p): stub.\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_putref_AudioOutputStream(ISpeechVoice *iface, ISpeechBaseStream *output)
{
    FIXME("(%p, %p): stub.\n", iface, output);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Rate(ISpeechVoice *iface, LONG *rate)
{
    FIXME("(%p, %p): stub.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_Rate(ISpeechVoice *iface, LONG rate)
{
    FIXME("(%p, %d): stub.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Volume(ISpeechVoice *iface, LONG *volume)
{
    FIXME("(%p, %p): stub.\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_Volume(ISpeechVoice *iface, LONG volume)
{
    FIXME("(%p, %d): stub.\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_AllowAudioOutputFormatChangesOnNextSet(ISpeechVoice *iface,
                                                                              VARIANT_BOOL allow)
{
    FIXME("(%p, %d): stub.\n", iface, allow);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_AllowAudioOutputFormatChangesOnNextSet(ISpeechVoice *iface, VARIANT_BOOL *allow)
{
    FIXME("(%p, %p): stub.\n", iface, allow);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_EventInterests(ISpeechVoice *iface, SpeechVoiceEvents *flags)
{
    FIXME("(%p, %p): stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_EventInterests(ISpeechVoice *iface, SpeechVoiceEvents flags)
{
    FIXME("(%p, %#x): stub.\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_Priority(ISpeechVoice *iface, SpeechVoicePriority priority)
{
    FIXME("(%p, %d): stub.\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Priority(ISpeechVoice *iface, SpeechVoicePriority *priority)
{
    FIXME("(%p, %p): stub.\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_AlertBoundary(ISpeechVoice *iface, SpeechVoiceEvents boundary)
{
    FIXME("(%p, %#x): stub.\n", iface, boundary);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_AlertBoundary(ISpeechVoice *iface, SpeechVoiceEvents *boundary)
{
    FIXME("(%p, %p): stub.\n", iface, boundary);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_SynchronousSpeakTimeout(ISpeechVoice *iface, LONG timeout)
{
    FIXME("(%p, %d): stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_SynchronousSpeakTimeout(ISpeechVoice *iface, LONG *timeout)
{
    FIXME("(%p, %p): stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Speak(ISpeechVoice *iface, BSTR text, SpeechVoiceSpeakFlags flags, LONG *number)
{
    FIXME("(%p, %s, %#x, %p): stub.\n", iface, debugstr_w(text), flags, number);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_SpeakStream(ISpeechVoice *iface, ISpeechBaseStream *stream,
                                               SpeechVoiceSpeakFlags flags, LONG *number)
{
    FIXME("(%p, %p, %#x, %p): stub.\n", iface, stream, flags, number);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Pause(ISpeechVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Resume(ISpeechVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Skip(ISpeechVoice *iface, const BSTR type, LONG items, LONG *skipped)
{
    FIXME("(%p, %s, %d, %p): stub.\n", iface, debugstr_w(type), items, skipped);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_GetVoices(ISpeechVoice *iface, BSTR required, BSTR optional,
                                             ISpeechObjectTokens **tokens)
{
    FIXME("(%p, %s, %s, %p): stub.\n", iface, debugstr_w(required), debugstr_w(optional), tokens);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_GetAudioOutputs(ISpeechVoice *iface, BSTR required, BSTR optional,
                                                   ISpeechObjectTokens **tokens)
{
    FIXME("(%p, %s, %s, %p): stub.\n", iface, debugstr_w(required), debugstr_w(optional), tokens);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_WaitUntilDone(ISpeechVoice *iface, LONG timeout, VARIANT_BOOL *done)
{
    FIXME("(%p, %d, %p): stub.\n", iface, timeout, done);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_SpeakCompleteEvent(ISpeechVoice *iface, LONG *handle)
{
    FIXME("(%p, %p): stub.\n", iface, handle);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_IsUISupported(ISpeechVoice *iface, const BSTR typeui, const VARIANT *data,
                                                 VARIANT_BOOL *supported)
{
    FIXME("(%p, %s, %p, %p): stub.\n", iface, debugstr_w(typeui), data, supported);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_DisplayUI(ISpeechVoice *iface, LONG hwnd, BSTR title,
                                             const BSTR typeui, const VARIANT *data)
{
    FIXME("(%p, %d, %s, %s, %p): stub.\n", iface, hwnd, debugstr_w(title), debugstr_w(typeui), data);

    return E_NOTIMPL;
}

static const ISpeechVoiceVtbl speech_voice_vtbl =
{
    speech_voice_QueryInterface,
    speech_voice_AddRef,
    speech_voice_Release,
    speech_voice_GetTypeInfoCount,
    speech_voice_GetTypeInfo,
    speech_voice_GetIDsOfNames,
    speech_voice_Invoke,
    speech_voice_get_Status,
    speech_voice_get_Voice,
    speech_voice_putref_Voice,
    speech_voice_get_AudioOutput,
    speech_voice_putref_AudioOutput,
    speech_voice_get_AudioOutputStream,
    speech_voice_putref_AudioOutputStream,
    speech_voice_get_Rate,
    speech_voice_put_Rate,
    speech_voice_get_Volume,
    speech_voice_put_Volume,
    speech_voice_put_AllowAudioOutputFormatChangesOnNextSet,
    speech_voice_get_AllowAudioOutputFormatChangesOnNextSet,
    speech_voice_get_EventInterests,
    speech_voice_put_EventInterests,
    speech_voice_put_Priority,
    speech_voice_get_Priority,
    speech_voice_put_AlertBoundary,
    speech_voice_get_AlertBoundary,
    speech_voice_put_SynchronousSpeakTimeout,
    speech_voice_get_SynchronousSpeakTimeout,
    speech_voice_Speak,
    speech_voice_SpeakStream,
    speech_voice_Pause,
    speech_voice_Resume,
    speech_voice_Skip,
    speech_voice_GetVoices,
    speech_voice_GetAudioOutputs,
    speech_voice_WaitUntilDone,
    speech_voice_SpeakCompleteEvent,
    speech_voice_IsUISupported,
    speech_voice_DisplayUI,
};

HRESULT speech_voice_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct speech_voice *This = heap_alloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpeechVoice_iface.lpVtbl = &speech_voice_vtbl;
    This->ref = 1;

    hr = ISpeechVoice_QueryInterface(&This->ISpeechVoice_iface, iid, obj);

    ISpeechVoice_Release(&This->ISpeechVoice_iface);
    return hr;
}
