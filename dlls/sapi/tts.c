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
#include "sperror.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

struct speech_voice
{
    ISpeechVoice ISpeechVoice_iface;
    ISpVoice ISpVoice_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    LONG ref;

    ISpStreamFormat *output;
    ISpObjectToken *engine_token;
    ISpTTSEngine *engine;
    LONG cur_stream_num;
    DWORD actions;
    USHORT volume;
    LONG rate;
    struct async_queue queue;
    CRITICAL_SECTION cs;
};

static inline struct speech_voice *impl_from_ISpeechVoice(ISpeechVoice *iface)
{
    return CONTAINING_RECORD(iface, struct speech_voice, ISpeechVoice_iface);
}

static inline struct speech_voice *impl_from_ISpVoice(ISpVoice *iface)
{
    return CONTAINING_RECORD(iface, struct speech_voice, ISpVoice_iface);
}

static inline struct speech_voice *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, struct speech_voice, IConnectionPointContainer_iface);
}

struct tts_engine_site
{
    ISpTTSEngineSite ISpTTSEngineSite_iface;
    LONG ref;

    struct speech_voice *voice;
    ULONG stream_num;
};

static inline struct tts_engine_site *impl_from_ISpTTSEngineSite(ISpTTSEngineSite *iface)
{
    return CONTAINING_RECORD(iface, struct tts_engine_site, ISpTTSEngineSite_iface);
}

static HRESULT create_default_token(const WCHAR *cat_id, ISpObjectToken **token)
{
    ISpObjectTokenCategory *cat;
    WCHAR *default_token_id = NULL;
    HRESULT hr;

    TRACE("(%s, %p).\n", debugstr_w(cat_id), token);

    if (FAILED(hr = CoCreateInstance(&CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                                     &IID_ISpObjectTokenCategory, (void **)&cat)))
        return hr;

    if (FAILED(hr = ISpObjectTokenCategory_SetId(cat, cat_id, FALSE)) ||
        FAILED(hr = ISpObjectTokenCategory_GetDefaultTokenId(cat, &default_token_id)))
    {
        ISpObjectTokenCategory_Release(cat);
        return hr;
    }
    ISpObjectTokenCategory_Release(cat);

    if (FAILED(hr = CoCreateInstance(&CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                                     &IID_ISpObjectToken, (void **)token)))
        goto done;

    if (FAILED(hr = ISpObjectToken_SetId(*token, NULL, default_token_id, FALSE)))
    {
        ISpObjectToken_Release(*token);
        *token = NULL;
    }

done:
    CoTaskMemFree(default_token_id);
    return hr;
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
    else if (IsEqualIID(iid, &IID_ISpVoice))
        *obj = &This->ISpVoice_iface;
    else if (IsEqualIID(iid, &IID_IConnectionPointContainer))
        *obj = &This->IConnectionPointContainer_iface;
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

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI speech_voice_Release(ISpeechVoice *iface)
{
    struct speech_voice *This = impl_from_ISpeechVoice(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        async_cancel_queue(&This->queue);
        if (This->output) ISpStreamFormat_Release(This->output);
        if (This->engine_token) ISpObjectToken_Release(This->engine_token);
        if (This->engine) ISpTTSEngine_Release(This->engine);
        DeleteCriticalSection(&This->cs);

        free(This);
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
    FIXME("(%p, %u, %lu, %p): stub.\n", iface, info, lcid, type_info);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_GetIDsOfNames(ISpeechVoice *iface, REFIID riid, LPOLESTR *names,
                                                 UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("(%p, %s, %p, %u, %lu, %p): stub.\n", iface, debugstr_guid(riid), names, count, lcid, dispid);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_Invoke(ISpeechVoice *iface, DISPID dispid, REFIID riid, LCID lcid,
                                          WORD flags, DISPPARAMS *params, VARIANT *result,
                                          EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("(%p, %ld, %s, %#lx, %#x, %p, %p, %p, %p): stub.\n", iface, dispid, debugstr_guid(riid),
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
    FIXME("(%p, %ld): stub.\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_get_Volume(ISpeechVoice *iface, LONG *volume)
{
    FIXME("(%p, %p): stub.\n", iface, volume);

    return E_NOTIMPL;
}

static HRESULT WINAPI speech_voice_put_Volume(ISpeechVoice *iface, LONG volume)
{
    FIXME("(%p, %ld): stub.\n", iface, volume);

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
    FIXME("(%p, %ld): stub.\n", iface, timeout);

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
    FIXME("(%p, %s, %ld, %p): stub.\n", iface, debugstr_w(type), items, skipped);

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
    FIXME("(%p, %ld, %p): stub.\n", iface, timeout, done);

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
    FIXME("(%p, %ld, %s, %s, %p): stub.\n", iface, hwnd, debugstr_w(title), debugstr_w(typeui), data);

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

/* ISpVoice interface */
static HRESULT WINAPI spvoice_QueryInterface(ISpVoice *iface, REFIID iid, void **obj)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %s %p).\n", iface, debugstr_guid(iid), obj);

    return ISpeechVoice_QueryInterface(&This->ISpeechVoice_iface, iid, obj);
}

static ULONG WINAPI spvoice_AddRef(ISpVoice *iface)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p).\n", iface);

    return ISpeechVoice_AddRef(&This->ISpeechVoice_iface);
}

static ULONG WINAPI spvoice_Release(ISpVoice *iface)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p).\n", iface);

    return ISpeechVoice_Release(&This->ISpeechVoice_iface);
}

static HRESULT WINAPI spvoice_SetNotifySink(ISpVoice *iface, ISpNotifySink *sink)
{
    FIXME("(%p, %p): stub.\n", iface, sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetNotifyWindowMessage(ISpVoice *iface, HWND hwnd, UINT msg,
                                                     WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %u, %Ix, %Ix): stub.\n", iface, hwnd, msg, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetNotifyCallbackFunction(ISpVoice *iface, SPNOTIFYCALLBACK *callback,
                                                        WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %Ix, %Ix): stub.\n", iface, callback, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetNotifyCallbackInterface(ISpVoice *iface, ISpNotifyCallback *callback,
                                                         WPARAM wparam, LPARAM lparam)
{
    FIXME("(%p, %p, %Ix, %Ix): stub.\n", iface, callback, wparam, lparam);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetNotifyWin32Event(ISpVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_WaitForNotifyEvent(ISpVoice *iface, DWORD milliseconds)
{
    FIXME("(%p, %ld): stub.\n", iface, milliseconds);

    return E_NOTIMPL;
}

static HANDLE WINAPI spvoice_GetNotifyEventHandle(ISpVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return NULL;
}

static HRESULT WINAPI spvoice_SetInterest(ISpVoice *iface, ULONGLONG event, ULONGLONG queued)
{
    FIXME("(%p, %s, %s): stub.\n", iface, wine_dbgstr_longlong(event), wine_dbgstr_longlong(queued));

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetEvents(ISpVoice *iface, ULONG count, SPEVENT *array, ULONG *fetched)
{
    FIXME("(%p, %lu, %p, %p): stub.\n", iface, count, array, fetched);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetInfo(ISpVoice *iface, SPEVENTSOURCEINFO *info)
{
    FIXME("(%p, %p): stub.\n", iface, info);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetOutput(ISpVoice *iface, IUnknown *unk, BOOL allow_format_changes)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);
    ISpStreamFormat *stream = NULL;
    ISpObjectToken *token = NULL;
    HRESULT hr;

    TRACE("(%p, %p, %d).\n", iface, unk, allow_format_changes);

    if (!allow_format_changes)
        FIXME("ignoring allow_format_changes = FALSE.\n");

    if (FAILED(hr = async_start_queue(&This->queue)))
        return hr;

    if (!unk)
    {
        /* TODO: Create the default SpAudioOut token here once SpMMAudioEnum is implemented. */
        if (FAILED(hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                                         &IID_ISpStreamFormat, (void **)&stream)))
            return hr;
    }
    else
    {
        if (FAILED(IUnknown_QueryInterface(unk, &IID_ISpStreamFormat, (void **)&stream)))
        {
            if (FAILED(IUnknown_QueryInterface(unk, &IID_ISpObjectToken, (void **)&token)))
                return E_INVALIDARG;
        }
    }

    if (!stream)
    {
        hr = ISpObjectToken_CreateInstance(token, NULL, CLSCTX_ALL, &IID_ISpStreamFormat, (void **)&stream);
        ISpObjectToken_Release(token);
        if (FAILED(hr))
            return hr;
    }

    EnterCriticalSection(&This->cs);

    if (This->output)
        ISpStreamFormat_Release(This->output);
    This->output = stream;

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI spvoice_GetOutputObjectToken(ISpVoice *iface, ISpObjectToken **token)
{
    FIXME("(%p, %p): stub.\n", iface, token);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetOutputStream(ISpVoice *iface, ISpStreamFormat **stream)
{
    FIXME("(%p, %p): stub.\n", iface, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_Pause(ISpVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_Resume(ISpVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetVoice(ISpVoice *iface, ISpObjectToken *token)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);
    WCHAR *id = NULL, *old_id = NULL;
    HRESULT hr;

    TRACE("(%p, %p).\n", iface, token);

    if (!token)
    {
        if (FAILED(hr = create_default_token(SPCAT_VOICES, &token)))
            return hr;
    }
    else
        ISpObjectToken_AddRef(token);

    EnterCriticalSection(&This->cs);

    if (This->engine_token &&
        SUCCEEDED(ISpObjectToken_GetId(token, &id)) &&
        SUCCEEDED(ISpObjectToken_GetId(This->engine_token, &old_id)) &&
        !wcscmp(id, old_id))
    {
        ISpObjectToken_Release(token);
        goto done;
    }

    if (This->engine_token)
        ISpObjectToken_Release(This->engine_token);
    This->engine_token = token;

    if (This->engine)
    {
        ISpTTSEngine_Release(This->engine);
        This->engine = NULL;
    }

done:
    LeaveCriticalSection(&This->cs);
    CoTaskMemFree(id);
    CoTaskMemFree(old_id);
    return S_OK;
}

static HRESULT WINAPI spvoice_GetVoice(ISpVoice *iface, ISpObjectToken **token)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %p).\n", iface, token);

    if (!token)
        return E_POINTER;

    EnterCriticalSection(&This->cs);

    if (!This->engine_token)
    {
        LeaveCriticalSection(&This->cs);
        return create_default_token(SPCAT_VOICES, token);
    }

    ISpObjectToken_AddRef(This->engine_token);
    *token = This->engine_token;

    LeaveCriticalSection(&This->cs);

    return S_OK;
}

struct async_result
{
    HANDLE done;
    HRESULT hr;
};

struct speak_task
{
    struct async_task task;
    struct async_result *result;

    struct speech_voice *voice;
    ISpTTSEngine *engine;
    SPVTEXTFRAG *frag_list;
    ISpTTSEngineSite *site;
    DWORD flags;
};

static HRESULT set_output_format(ISpStreamFormat *output, ISpTTSEngine *engine, GUID *fmtid, WAVEFORMATEX **wfx)
{
    GUID output_fmtid;
    WAVEFORMATEX *output_wfx = NULL;
    ISpAudio *audio = NULL;
    HRESULT hr;

    if (FAILED(hr = ISpStreamFormat_GetFormat(output, &output_fmtid, &output_wfx)))
        return hr;
    if (FAILED(hr = ISpTTSEngine_GetOutputFormat(engine, &output_fmtid, output_wfx, fmtid, wfx)))
        goto done;
    if (!IsEqualGUID(fmtid, &SPDFID_WaveFormatEx))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (memcmp(output_wfx, *wfx, sizeof(WAVEFORMATEX)) ||
        memcmp(output_wfx + 1, *wfx + 1, output_wfx->cbSize))
    {
        if (FAILED(hr = ISpStreamFormat_QueryInterface(output, &IID_ISpAudio, (void **)&audio)) ||
            FAILED(hr = ISpAudio_SetFormat(audio, &SPDFID_WaveFormatEx, *wfx)))
            goto done;
    }

done:
    CoTaskMemFree(output_wfx);
    if (audio) ISpAudio_Release(audio);
    return hr;
}

static void speak_proc(struct async_task *task)
{
    struct speak_task *speak_task = (struct speak_task *)task;
    struct speech_voice *This = speak_task->voice;
    GUID fmtid;
    WAVEFORMATEX *wfx = NULL;
    ISpAudio *audio = NULL;
    HRESULT hr;

    TRACE("(%p).\n", task);

    EnterCriticalSection(&This->cs);

    if (This->actions & SPVES_ABORT)
    {
        LeaveCriticalSection(&This->cs);
        hr = S_OK;
        goto done;
    }

    if (FAILED(hr = set_output_format(This->output, This->engine, &fmtid, &wfx)))
    {
        LeaveCriticalSection(&This->cs);
        ERR("failed setting output format: %#lx.\n", hr);
        goto done;
    }

    if (SUCCEEDED(ISpStreamFormat_QueryInterface(This->output, &IID_ISpAudio, (void **)&audio)))
        ISpAudio_SetState(audio, SPAS_RUN, 0);

    This->actions = SPVES_RATE | SPVES_VOLUME;

    LeaveCriticalSection(&This->cs);

    hr = ISpTTSEngine_Speak(speak_task->engine, speak_task->flags, &fmtid, wfx, speak_task->frag_list, speak_task->site);
    if (SUCCEEDED(hr))
    {
        ISpStreamFormat_Commit(This->output, STGC_DEFAULT);
        if (audio)
            WaitForSingleObject(ISpAudio_EventHandle(audio), INFINITE);
    }
    else
        WARN("ISpTTSEngine_Speak failed: %#lx.\n", hr);

done:
    if (audio)
    {
        ISpAudio_SetState(audio, SPAS_CLOSED, 0);
        ISpAudio_Release(audio);
    }
    CoTaskMemFree(wfx);
    ISpTTSEngine_Release(speak_task->engine);
    free(speak_task->frag_list);
    ISpTTSEngineSite_Release(speak_task->site);

    if (speak_task->result)
    {
        speak_task->result->hr = hr;
        SetEvent(speak_task->result->done);
    }
}

static HRESULT ttsenginesite_create(struct speech_voice *voice, ULONG stream_num, ISpTTSEngineSite **site);

static HRESULT WINAPI spvoice_Speak(ISpVoice *iface, const WCHAR *contents, DWORD flags, ULONG *stream_num_out)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);
    ISpTTSEngineSite *site = NULL;
    ISpTTSEngine *engine = NULL;
    SPVTEXTFRAG *frag;
    struct speak_task *speak_task = NULL;
    struct async_result *result = NULL;
    size_t contents_len, contents_size;
    ULONG stream_num;
    HRESULT hr;

    TRACE("(%p, %p, %#lx, %p).\n", iface, contents, flags, stream_num_out);

    flags &= ~SPF_IS_NOT_XML;
    if (flags & ~(SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_NLP_SPEAK_PUNC))
    {
        FIXME("flags %#lx not implemented.\n", flags & ~(SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_NLP_SPEAK_PUNC));
        return E_NOTIMPL;
    }

    if (flags & SPF_PURGEBEFORESPEAK)
    {
        ISpAudio *audio;

        EnterCriticalSection(&This->cs);

        This->actions = SPVES_ABORT;
        if (This->output && SUCCEEDED(ISpStreamFormat_QueryInterface(This->output, &IID_ISpAudio, (void **)&audio)))
        {
            ISpAudio_SetState(audio, SPAS_CLOSED, 0);
            ISpAudio_Release(audio);
        }

        LeaveCriticalSection(&This->cs);

        async_empty_queue(&This->queue);

        EnterCriticalSection(&This->cs);
        This->actions = SPVES_CONTINUE;
        LeaveCriticalSection(&This->cs);

        if (!contents || !*contents)
            return S_OK;
    }
    else if (!contents)
        return E_POINTER;

    contents_len = wcslen(contents);
    contents_size = sizeof(WCHAR) * (contents_len + 1);

    if (!This->output)
    {
        /* Create a new output stream with the default output. */
        if (FAILED(hr = ISpVoice_SetOutput(iface, NULL, TRUE)))
            return hr;
    }

    EnterCriticalSection(&This->cs);

    if (!This->engine_token)
    {
        /* Set the engine token to default. */
        if (FAILED(hr = ISpVoice_SetVoice(iface, NULL)))
        {
            LeaveCriticalSection(&This->cs);
            return hr;
        }
    }
    if (!This->engine &&
        FAILED(hr = ISpObjectToken_CreateInstance(This->engine_token, NULL, CLSCTX_ALL, &IID_ISpTTSEngine, (void **)&This->engine)))
    {
        LeaveCriticalSection(&This->cs);
        ERR("Failed to create engine: %#lx.\n", hr);
        return hr;
    }
    engine = This->engine;
    ISpTTSEngine_AddRef(engine);

    LeaveCriticalSection(&This->cs);

    if (!(frag = malloc(sizeof(*frag) + contents_size)))
        return E_OUTOFMEMORY;
    memset(frag, 0, sizeof(*frag));
    memcpy(frag + 1, contents, contents_size);
    frag->State.eAction = SPVA_Speak;
    frag->State.Volume  = 100;
    frag->pTextStart    = (WCHAR *)(frag + 1);
    frag->ulTextLen     = contents_len;
    frag->ulTextSrcOffset = 0;

    stream_num = InterlockedIncrement(&This->cur_stream_num);
    if (FAILED(hr = ttsenginesite_create(This, stream_num, &site)))
    {
        FIXME("Failed to create ttsenginesite: %#lx.\n", hr);
        goto fail;
    }

    speak_task = malloc(sizeof(*speak_task));

    speak_task->task.proc = speak_proc;
    speak_task->result    = NULL;
    speak_task->voice     = This;
    speak_task->engine    = engine;
    speak_task->frag_list = frag;
    speak_task->site      = site;
    speak_task->flags     = flags & SPF_NLP_SPEAK_PUNC;

    if (!(flags & SPF_ASYNC))
    {
        if (!(result = malloc(sizeof(*result))))
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }
        result->hr = E_FAIL;
        result->done = CreateEventW(NULL, FALSE, FALSE, NULL);
        speak_task->result = result;
    }

    if (FAILED(hr = async_queue_task(&This->queue, (struct async_task *)speak_task)))
    {
        WARN("Failed to queue task: %#lx.\n", hr);
        goto fail;
    }

    if (stream_num_out)
        *stream_num_out = stream_num;

    if (flags & SPF_ASYNC)
        return S_OK;
    else
    {
        WaitForSingleObject(result->done, INFINITE);
        hr = result->hr;
        CloseHandle(result->done);
        free(result);
        return hr;
    }

fail:
    if (site) ISpTTSEngineSite_Release(site);
    if (engine) ISpTTSEngine_Release(engine);
    free(frag);
    free(speak_task);
    if (result)
    {
        CloseHandle(result->done);
        free(result);
    }
    return hr;
}

static HRESULT WINAPI spvoice_SpeakStream(ISpVoice *iface, IStream *stream, DWORD flags, ULONG *number)
{
    FIXME("(%p, %p, %#lx, %p): stub.\n", iface, stream, flags, number);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetStatus(ISpVoice *iface, SPVOICESTATUS *status, WCHAR **bookmark)
{
    FIXME("(%p, %p, %p): stub.\n", iface, status, bookmark);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_Skip(ISpVoice *iface, const WCHAR *type, LONG items, ULONG *skipped)
{
    FIXME("(%p, %s, %ld, %p): stub.\n", iface, debugstr_w(type), items, skipped);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetPriority(ISpVoice *iface, SPVPRIORITY priority)
{
    FIXME("(%p, %d): stub.\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetPriority(ISpVoice *iface, SPVPRIORITY *priority)
{
    FIXME("(%p, %p): stub.\n", iface, priority);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetAlertBoundary(ISpVoice *iface, SPEVENTENUM boundary)
{
    FIXME("(%p, %d): stub.\n", iface, boundary);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetAlertBoundary(ISpVoice *iface, SPEVENTENUM *boundary)
{
    FIXME("(%p, %p): stub.\n", iface, boundary);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_SetRate(ISpVoice *iface, LONG rate)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %ld).\n", iface, rate);

    EnterCriticalSection(&This->cs);
    This->rate = rate;
    This->actions |= SPVES_RATE;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI spvoice_GetRate(ISpVoice *iface, LONG *rate)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %p).\n", iface, rate);

    EnterCriticalSection(&This->cs);
    *rate = This->rate;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI spvoice_SetVolume(ISpVoice *iface, USHORT volume)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %d).\n", iface, volume);

    if (volume > 100)
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);
    This->volume = volume;
    This->actions |= SPVES_VOLUME;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI spvoice_GetVolume(ISpVoice *iface, USHORT *volume)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);

    TRACE("(%p, %p).\n", iface, volume);

    EnterCriticalSection(&This->cs);
    *volume = This->volume;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI spvoice_WaitUntilDone(ISpVoice *iface, ULONG timeout)
{
    struct speech_voice *This = impl_from_ISpVoice(iface);
    HRESULT hr;

    TRACE("(%p, %ld).\n", iface, timeout);

    hr = async_wait_queue_empty(&This->queue, timeout);

    if (hr == WAIT_OBJECT_0) return S_OK;
    else if (hr == WAIT_TIMEOUT) return S_FALSE;
    return hr;
}

static HRESULT WINAPI spvoice_SetSyncSpeakTimeout(ISpVoice *iface, ULONG timeout)
{
    FIXME("(%p, %ld): stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_GetSyncSpeakTimeout(ISpVoice *iface, ULONG *timeout)
{
    FIXME("(%p, %p): stub.\n", iface, timeout);

    return E_NOTIMPL;
}

static HANDLE WINAPI spvoice_SpeakCompleteEvent(ISpVoice *iface)
{
    FIXME("(%p): stub.\n", iface);

    return NULL;
}

static HRESULT WINAPI spvoice_IsUISupported(ISpVoice *iface, const WCHAR *type, void *extra,
                                            ULONG count, BOOL *supported)
{
    FIXME("(%p, %p, %p, %ld, %p): stub.\n", iface, type, extra, count, supported);

    return E_NOTIMPL;
}

static HRESULT WINAPI spvoice_DisplayUI(ISpVoice *iface, HWND parent, const WCHAR *title,
                                        const WCHAR *type, void *extra, ULONG count)
{
    FIXME("(%p, %p, %p, %p, %p, %ld): stub.\n", iface, parent, title, type, extra, count);

    return E_NOTIMPL;
}

static const ISpVoiceVtbl spvoice_vtbl =
{
    spvoice_QueryInterface,
    spvoice_AddRef,
    spvoice_Release,
    spvoice_SetNotifySink,
    spvoice_SetNotifyWindowMessage,
    spvoice_SetNotifyCallbackFunction,
    spvoice_SetNotifyCallbackInterface,
    spvoice_SetNotifyWin32Event,
    spvoice_WaitForNotifyEvent,
    spvoice_GetNotifyEventHandle,
    spvoice_SetInterest,
    spvoice_GetEvents,
    spvoice_GetInfo,
    spvoice_SetOutput,
    spvoice_GetOutputObjectToken,
    spvoice_GetOutputStream,
    spvoice_Pause,
    spvoice_Resume,
    spvoice_SetVoice,
    spvoice_GetVoice,
    spvoice_Speak,
    spvoice_SpeakStream,
    spvoice_GetStatus,
    spvoice_Skip,
    spvoice_SetPriority,
    spvoice_GetPriority,
    spvoice_SetAlertBoundary,
    spvoice_GetAlertBoundary,
    spvoice_SetRate,
    spvoice_GetRate,
    spvoice_SetVolume,
    spvoice_GetVolume,
    spvoice_WaitUntilDone,
    spvoice_SetSyncSpeakTimeout,
    spvoice_GetSyncSpeakTimeout,
    spvoice_SpeakCompleteEvent,
    spvoice_IsUISupported,
    spvoice_DisplayUI
};

/* ISpTTSEngineSite interface */
static HRESULT WINAPI ttsenginesite_QueryInterface(ISpTTSEngineSite *iface, REFIID iid, void **obj)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);

    TRACE("(%p, %s %p).\n", iface, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISpTTSEngineSite))
        *obj = &This->ISpTTSEngineSite_iface;
    else
    {
        *obj = NULL;
        FIXME("interface %s not implemented.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI ttsenginesite_AddRef(ISpTTSEngineSite *iface)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI ttsenginesite_Release(ISpTTSEngineSite *iface)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);

    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): ref=%lu.\n", iface, ref);

    if (!ref)
    {
        if (This->voice)
            ISpeechVoice_Release(&This->voice->ISpeechVoice_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ttsenginesite_AddEvents(ISpTTSEngineSite *iface, const SPEVENT *events, ULONG count)
{
    FIXME("(%p, %p, %ld): stub.\n", iface, events, count);

    return S_OK;
}

static HRESULT WINAPI ttsenginesite_GetEventInterest(ISpTTSEngineSite *iface, ULONGLONG *interest)
{
    FIXME("(%p, %p): stub.\n", iface, interest);

    return E_NOTIMPL;
}

static DWORD WINAPI ttsenginesite_GetActions(ISpTTSEngineSite *iface)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);
    DWORD actions;

    TRACE("(%p).\n", iface);

    EnterCriticalSection(&This->voice->cs);
    actions = This->voice->actions;
    LeaveCriticalSection(&This->voice->cs);

    return actions;
}

static HRESULT WINAPI ttsenginesite_Write(ISpTTSEngineSite *iface, const void *buf, ULONG cb, ULONG *cb_written)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);

    TRACE("(%p, %p, %ld, %p).\n", iface, buf, cb, cb_written);

    if (!This->voice->output)
        return SPERR_UNINITIALIZED;

    return ISpStreamFormat_Write(This->voice->output, buf, cb, cb_written);
}

static HRESULT WINAPI ttsenginesite_GetRate(ISpTTSEngineSite *iface, LONG *rate)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);

    TRACE("(%p, %p).\n", iface, rate);

    EnterCriticalSection(&This->voice->cs);
    *rate = This->voice->rate;
    This->voice->actions &= ~SPVES_RATE;
    LeaveCriticalSection(&This->voice->cs);

    return S_OK;
}

static HRESULT WINAPI ttsenginesite_GetVolume(ISpTTSEngineSite *iface, USHORT *volume)
{
    struct tts_engine_site *This = impl_from_ISpTTSEngineSite(iface);

    TRACE("(%p, %p).\n", iface, volume);

    EnterCriticalSection(&This->voice->cs);
    *volume = This->voice->volume;
    This->voice->actions &= ~SPVES_VOLUME;
    LeaveCriticalSection(&This->voice->cs);

    return S_OK;
}

static HRESULT WINAPI ttsenginesite_GetSkipInfo(ISpTTSEngineSite *iface, SPVSKIPTYPE *type, LONG *skip_count)
{
    FIXME("(%p, %p, %p): stub.\n", iface, type, skip_count);

    return E_NOTIMPL;
}

static HRESULT WINAPI ttsenginesite_CompleteSkip(ISpTTSEngineSite *iface, LONG num_skipped)
{
    FIXME("(%p, %ld): stub.\n", iface, num_skipped);

    return E_NOTIMPL;
}

const static ISpTTSEngineSiteVtbl ttsenginesite_vtbl =
{
    ttsenginesite_QueryInterface,
    ttsenginesite_AddRef,
    ttsenginesite_Release,
    ttsenginesite_AddEvents,
    ttsenginesite_GetEventInterest,
    ttsenginesite_GetActions,
    ttsenginesite_Write,
    ttsenginesite_GetRate,
    ttsenginesite_GetVolume,
    ttsenginesite_GetSkipInfo,
    ttsenginesite_CompleteSkip
};

static HRESULT ttsenginesite_create(struct speech_voice *voice, ULONG stream_num, ISpTTSEngineSite **site)
{
    struct tts_engine_site *This = malloc(sizeof(*This));

    if (!This) return E_OUTOFMEMORY;

    This->ISpTTSEngineSite_iface.lpVtbl = &ttsenginesite_vtbl;

    This->ref = 1;
    This->voice = voice;
    This->stream_num = stream_num;

    ISpeechVoice_AddRef(&This->voice->ISpeechVoice_iface);

    *site = &This->ISpTTSEngineSite_iface;

    return S_OK;
}

/* IConnectionPointContainer interface */
static HRESULT WINAPI container_QueryInterface(IConnectionPointContainer *iface, REFIID iid, void **obj)
{
    struct speech_voice *This = impl_from_IConnectionPointContainer(iface);

    TRACE("(%p, %s %p).\n", iface, debugstr_guid(iid), obj);

    return ISpeechVoice_QueryInterface(&This->ISpeechVoice_iface, iid, obj);
}

static ULONG WINAPI container_AddRef(IConnectionPointContainer *iface)
{
    struct speech_voice *This = impl_from_IConnectionPointContainer(iface);

    TRACE("(%p).\n", iface);

    return ISpeechVoice_AddRef(&This->ISpeechVoice_iface);
}

static ULONG WINAPI container_Release(IConnectionPointContainer *iface)
{
    struct speech_voice *This = impl_from_IConnectionPointContainer(iface);

    TRACE("(%p).\n", iface);

    return ISpeechVoice_Release(&This->ISpeechVoice_iface);
}

static HRESULT WINAPI container_EnumConnectionPoints(IConnectionPointContainer *iface,
                                                     IEnumConnectionPoints **enum_cp)
{
    FIXME("(%p, %p): stub.\n", iface, enum_cp);

    return E_NOTIMPL;
}

static HRESULT WINAPI container_FindConnectionPoint(IConnectionPointContainer *iface, REFIID riid,
                                                    IConnectionPoint **cp)
{
    FIXME("(%p, %s, %p): stub.\n", iface, debugstr_guid(riid), cp);

    return E_NOTIMPL;
}

const static IConnectionPointContainerVtbl container_vtbl =
{
    container_QueryInterface,
    container_AddRef,
    container_Release,
    container_EnumConnectionPoints,
    container_FindConnectionPoint
};

HRESULT speech_voice_create(IUnknown *outer, REFIID iid, void **obj)
{
    struct speech_voice *This = malloc(sizeof(*This));
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->ISpeechVoice_iface.lpVtbl = &speech_voice_vtbl;
    This->ISpVoice_iface.lpVtbl = &spvoice_vtbl;
    This->IConnectionPointContainer_iface.lpVtbl = &container_vtbl;
    This->ref = 1;

    This->output = NULL;
    This->engine_token = NULL;
    This->engine = NULL;
    This->cur_stream_num = 0;
    This->actions = SPVES_CONTINUE;
    This->volume = 100;
    This->rate = 0;
    memset(&This->queue, 0, sizeof(This->queue));

    InitializeCriticalSection(&This->cs);

    hr = ISpeechVoice_QueryInterface(&This->ISpeechVoice_iface, iid, obj);

    ISpeechVoice_Release(&This->ISpeechVoice_iface);
    return hr;
}
