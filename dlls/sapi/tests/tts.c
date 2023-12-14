/*
 * Speech API (SAPI) text-to-speech tests.
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

#define COBJMACROS

#include "sapiddk.h"
#include "sperror.h"

#include "wine/test.h"

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown *obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "Unexpected refcount %ld, expected %ld.\n", rc, ref);
}

#define APTTYPE_UNITIALIZED APTTYPE_CURRENT
static struct
{
    APTTYPE type;
    APTTYPEQUALIFIER qualifier;
} test_apt_data;

static DWORD WINAPI test_apt_thread(void *param)
{
    HRESULT hr;

    hr = CoGetApartmentType(&test_apt_data.type, &test_apt_data.qualifier);
    if (hr == CO_E_NOTINITIALIZED)
    {
        test_apt_data.type = APTTYPE_UNITIALIZED;
        test_apt_data.qualifier = 0;
    }

    return 0;
}

static void check_apttype(void)
{
    HANDLE thread;
    MSG msg;

    memset(&test_apt_data, 0xde, sizeof(test_apt_data));

    thread = CreateThread(NULL, 0, test_apt_thread, NULL, 0, NULL);
    while (MsgWaitForMultipleObjects(1, &thread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    CloseHandle(thread);
}

static void test_interfaces(void)
{
    ISpeechVoice *speech_voice, *speech_voice2;
    IConnectionPointContainer *container;
    ISpTTSEngineSite *site;
    ISpVoice *spvoice, *spvoice2;
    IDispatch *dispatch;
    IUnknown *unk;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpeechVoice, (void **)&speech_voice);
    ok(hr == S_OK, "Failed to create ISpeechVoice interface: %#lx.\n", hr);
    EXPECT_REF(speech_voice, 1);

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IDispatch, (void **)&dispatch);
    ok(hr == S_OK, "Failed to create IDispatch interface: %#lx.\n", hr);
    EXPECT_REF(dispatch, 1);
    EXPECT_REF(speech_voice, 1);
    IDispatch_Release(dispatch);

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&unk);
    ok(hr == S_OK, "Failed to create IUnknown interface: %#lx.\n", hr);
    EXPECT_REF(unk, 1);
    EXPECT_REF(speech_voice, 1);
    IUnknown_Release(unk);

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpVoice, (void **)&spvoice);
    ok(hr == S_OK, "Failed to create ISpVoice interface: %#lx.\n", hr);
    EXPECT_REF(spvoice, 1);
    EXPECT_REF(speech_voice, 1);

    hr = ISpVoice_QueryInterface(spvoice, &IID_ISpeechVoice, (void **)&speech_voice2);
    ok(hr == S_OK, "ISpVoice_QueryInterface failed: %#lx.\n", hr);
    EXPECT_REF(speech_voice2, 2);
    EXPECT_REF(spvoice, 2);
    EXPECT_REF(speech_voice, 1);
    ISpeechVoice_Release(speech_voice2);

    hr = ISpeechVoice_QueryInterface(speech_voice, &IID_ISpVoice, (void **)&spvoice2);
    ok(hr == S_OK, "ISpeechVoice_QueryInterface failed: %#lx.\n", hr);
    EXPECT_REF(speech_voice, 2);
    EXPECT_REF(spvoice2, 2);
    EXPECT_REF(spvoice, 1);
    ISpVoice_Release(spvoice2);
    ISpVoice_Release(spvoice);

    hr = ISpeechVoice_QueryInterface(speech_voice, &IID_IConnectionPointContainer,
                                     (void **)&container);
    ok(hr == S_OK, "ISpeechVoice_QueryInterface failed: %#lx.\n", hr);
    EXPECT_REF(speech_voice, 2);
    EXPECT_REF(container, 2);
    IConnectionPointContainer_Release(container);

    hr = ISpeechVoice_QueryInterface(speech_voice, &IID_ISpTTSEngineSite,
                                     (void **)&site);
    ok(hr == E_NOINTERFACE, "ISpeechVoice_QueryInterface for ISpTTSEngineSite returned: %#lx.\n", hr);

    ISpeechVoice_Release(speech_voice);
}

#define TESTENGINE_CLSID L"{57C7E6B1-2FC2-4E8E-B968-1410A39E7198}"
static const GUID CLSID_TestEngine = {0x57C7E6B1,0x2FC2,0x4E8E,{0xB9,0x68,0x14,0x10,0xA3,0x9E,0x71,0x98}};

struct test_engine
{
    ISpTTSEngine ISpTTSEngine_iface;
    ISpObjectWithToken ISpObjectWithToken_iface;

    ISpObjectToken *token;

    BOOL speak_called;
    DWORD flags;
    GUID fmtid;
    SPVTEXTFRAG *frag_list;
    LONG rate;
    USHORT volume;
};

static void copy_frag_list(const SPVTEXTFRAG *frag_list, SPVTEXTFRAG **ret_frag_list)
{
    SPVTEXTFRAG *frag, *prev = NULL;

    if (!frag_list)
    {
        *ret_frag_list = NULL;
        return;
    }

    while (frag_list)
    {
        frag = malloc(sizeof(*frag) + frag_list->ulTextLen * sizeof(WCHAR));
        memcpy(frag, frag_list, sizeof(*frag));

        if (frag_list->pTextStart)
        {
            frag->pTextStart = (WCHAR *)(frag + 1);
            memcpy(frag + 1, frag_list->pTextStart, frag->ulTextLen * sizeof(WCHAR));
        }

        frag->pNext = NULL;

        if (prev)
            prev->pNext = frag;
        else
            *ret_frag_list = frag;

        prev = frag;
        frag_list = frag_list->pNext;
    }
}

static void reset_engine_params(struct test_engine *engine)
{
    SPVTEXTFRAG *frag, *next;

    engine->speak_called = FALSE;
    engine->flags = 0xdeadbeef;
    memset(&engine->fmtid, 0xde, sizeof(engine->fmtid));
    engine->rate = 0xdeadbeef;
    engine->volume = 0xbeef;

    for (frag = engine->frag_list; frag; frag = next)
    {
        next = frag->pNext;
        free(frag);
    }
    engine->frag_list = NULL;
}

static inline struct test_engine *impl_from_ISpTTSEngine(ISpTTSEngine *iface)
{
    return CONTAINING_RECORD(iface, struct test_engine, ISpTTSEngine_iface);
}

static inline struct test_engine *impl_from_ISpObjectWithToken(ISpObjectWithToken *iface)
{
    return CONTAINING_RECORD(iface, struct test_engine, ISpObjectWithToken_iface);
}

static HRESULT WINAPI test_engine_QueryInterface(ISpTTSEngine *iface, REFIID iid, void **obj)
{
    struct test_engine *engine = impl_from_ISpTTSEngine(iface);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ISpTTSEngine))
        *obj = &engine->ISpTTSEngine_iface;
    else if (IsEqualIID(iid, &IID_ISpObjectWithToken))
        *obj = &engine->ISpObjectWithToken_iface;
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI test_engine_AddRef(ISpTTSEngine *iface)
{
    return 2;
}

static ULONG WINAPI test_engine_Release(ISpTTSEngine *iface)
{
    return 1;
}

static HRESULT WINAPI test_engine_Speak(ISpTTSEngine *iface, DWORD flags, REFGUID fmtid,
                                        const WAVEFORMATEX *wfx, const SPVTEXTFRAG *frag_list,
                                        ISpTTSEngineSite *site)
{
    struct test_engine *engine = impl_from_ISpTTSEngine(iface);
    DWORD actions;
    char *buf;
    int i;
    HRESULT hr;

    engine->flags = flags;
    engine->fmtid = *fmtid;
    copy_frag_list(frag_list, &engine->frag_list);
    engine->speak_called = TRUE;

    actions = ISpTTSEngineSite_GetActions(site);
    ok(actions == (SPVES_CONTINUE | SPVES_RATE | SPVES_VOLUME), "got %#lx.\n", actions);

    hr = ISpTTSEngineSite_GetRate(site, &engine->rate);
    ok(hr == S_OK, "got %#lx.\n", hr);
    actions = ISpTTSEngineSite_GetActions(site);
    ok(actions == (SPVES_CONTINUE | SPVES_VOLUME), "got %#lx.\n", actions);

    hr = ISpTTSEngineSite_GetVolume(site, &engine->volume);
    ok(hr == S_OK, "got %#lx.\n", hr);
    actions = ISpTTSEngineSite_GetActions(site);
    ok(actions == SPVES_CONTINUE, "got %#lx.\n", actions);

    buf = calloc(1, 22050 * 2 / 5);
    for (i = 0; i < 5; i++)
    {
        if (ISpTTSEngineSite_GetActions(site) & SPVES_ABORT)
            break;
        hr = ISpTTSEngineSite_Write(site, buf, 22050 * 2 / 5, NULL);
        ok(hr == S_OK || hr == SP_AUDIO_STOPPED, "got %#lx.\n", hr);
        Sleep(100);
    }
    free(buf);

    return S_OK;
}

static HRESULT WINAPI test_engine_GetOutputFormat(ISpTTSEngine *iface, const GUID *fmtid,
                                                  const WAVEFORMATEX *wfx, GUID *out_fmtid,
                                                  WAVEFORMATEX **out_wfx)
{
    *out_fmtid = SPDFID_WaveFormatEx;
    *out_wfx = CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    (*out_wfx)->wFormatTag = WAVE_FORMAT_PCM;
    (*out_wfx)->nChannels = 1;
    (*out_wfx)->nSamplesPerSec = 22050;
    (*out_wfx)->wBitsPerSample = 16;
    (*out_wfx)->nBlockAlign = 2;
    (*out_wfx)->nAvgBytesPerSec = 22050 * 2;
    (*out_wfx)->cbSize = 0;

    return S_OK;
}

static ISpTTSEngineVtbl test_engine_vtbl =
{
    test_engine_QueryInterface,
    test_engine_AddRef,
    test_engine_Release,
    test_engine_Speak,
    test_engine_GetOutputFormat,
};

static HRESULT WINAPI objwithtoken_QueryInterface(ISpObjectWithToken *iface, REFIID iid, void **obj)
{
    struct test_engine *engine = impl_from_ISpObjectWithToken(iface);

    return ISpTTSEngine_QueryInterface(&engine->ISpTTSEngine_iface, iid, obj);
}

static ULONG WINAPI objwithtoken_AddRef(ISpObjectWithToken *iface)
{
    return 2;
}

static ULONG WINAPI objwithtoken_Release(ISpObjectWithToken *iface)
{
    return 1;
}

static HRESULT WINAPI objwithtoken_SetObjectToken(ISpObjectWithToken *iface, ISpObjectToken *token)
{
    struct test_engine *engine = impl_from_ISpObjectWithToken(iface);

    if (!token)
        return E_INVALIDARG;

    ISpObjectToken_AddRef(token);
    engine->token = token;

    return S_OK;
}

static HRESULT WINAPI objwithtoken_GetObjectToken(ISpObjectWithToken *iface, ISpObjectToken **token)
{
    struct test_engine *engine = impl_from_ISpObjectWithToken(iface);

    *token = engine->token;
    if (*token)
        ISpObjectToken_AddRef(*token);

    return S_OK;
}

static const ISpObjectWithTokenVtbl objwithtoken_vtbl =
{
    objwithtoken_QueryInterface,
    objwithtoken_AddRef,
    objwithtoken_Release,
    objwithtoken_SetObjectToken,
    objwithtoken_GetObjectToken
};

static struct test_engine test_engine = {{&test_engine_vtbl}, {&objwithtoken_vtbl}};

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
        IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ok(pUnkOuter == NULL, "pUnkOuter != NULL.\n");
    ok(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISpTTSEngine),
       "riid = %s.\n", wine_dbgstr_guid(riid));

    *ppv = &test_engine.ISpTTSEngine_iface;
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    ok(0, "unexpected call.\n");
    return E_NOTIMPL;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory test_engine_cf = { &ClassFactoryVtbl };

static void test_spvoice(void)
{
    static const WCHAR test_token_id[] = L"HKEY_LOCAL_MACHINE\\Software\\Wine\\Winetest\\sapi\\tts\\TestEngine";
    static const WCHAR test_text[] = L"Hello! This is a test sentence.";

    ISpVoice *voice;
    IUnknown *dummy;
    ISpMMSysAudio *audio_out;
    ISpObjectTokenCategory *token_cat;
    ISpObjectToken *token;
    WCHAR *token_id = NULL, *default_token_id = NULL;
    ISpDataKey *attrs_key;
    LONG rate;
    USHORT volume;
    ULONG stream_num;
    DWORD regid;
    DWORD start, duration;
    HRESULT hr;

    if (waveOutGetNumDevs() == 0) {
        skip("no wave out devices.\n");
        return;
    }

    check_apttype();
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpVoice, (void **)&voice);
    ok(hr == S_OK, "Failed to create SpVoice: %#lx.\n", hr);

    check_apttype();
    ok(test_apt_data.type == APTTYPE_UNITIALIZED, "got apt type %d.\n", test_apt_data.type);

    /* SpVoice initializes a MTA in SetOutput even if an invalid output object is given. */
    hr = CoCreateInstance(&CLSID_SpDataKey, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IUnknown, (void **)&dummy);
    ok(hr == S_OK, "Failed to create dummy: %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, dummy, TRUE);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    check_apttype();
    ok(test_apt_data.type == APTTYPE_MTA || broken(test_apt_data.type == APTTYPE_UNITIALIZED) /* w8, w10v1507 */,
       "got apt type %d.\n", test_apt_data.type);
    if (test_apt_data.type == APTTYPE_MTA)
        ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
           "got apt type qualifier %d.\n", test_apt_data.qualifier);
    else
        win_skip("apt type is not MTA.\n");

    IUnknown_Release(dummy);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&audio_out);
    ok(hr == S_OK, "Failed to create SpMMAudioOut: %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, NULL, TRUE);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, (IUnknown *)audio_out, TRUE);
    todo_wine ok(hr == S_FALSE, "got %#lx.\n", hr);

    hr = ISpVoice_SetVoice(voice, NULL);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);

    check_apttype();
    ok(test_apt_data.type == APTTYPE_MTA || broken(test_apt_data.type == APTTYPE_UNITIALIZED) /* w8, w10v1507 */,
       "got apt type %d.\n", test_apt_data.type);
    if (test_apt_data.type == APTTYPE_MTA)
        ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
           "got apt type qualifier %d.\n", test_apt_data.qualifier);
    else
        win_skip("apt type is not MTA.\n");

    hr = ISpVoice_GetVoice(voice, &token);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = ISpObjectToken_GetId(token, &token_id);
        ok(hr == S_OK, "got %#lx.\n", hr);

        hr = CoCreateInstance(&CLSID_SpObjectTokenCategory, NULL, CLSCTX_INPROC_SERVER,
                              &IID_ISpObjectTokenCategory, (void **)&token_cat);
        ok(hr == S_OK, "Failed to create SpObjectTokenCategory: %#lx.\n", hr);

        hr = ISpObjectTokenCategory_SetId(token_cat, SPCAT_VOICES, FALSE);
        ok(hr == S_OK, "got %#lx.\n", hr);
        hr = ISpObjectTokenCategory_GetDefaultTokenId(token_cat, &default_token_id);
        ok(hr == S_OK, "got %#lx.\n", hr);

        ok(!wcscmp(token_id, default_token_id), "token_id != default_token_id\n");

        CoTaskMemFree(token_id);
        CoTaskMemFree(default_token_id);
        ISpObjectToken_Release(token);
        ISpObjectTokenCategory_Release(token_cat);
    }

    rate = 0xdeadbeef;
    hr = ISpVoice_GetRate(voice, &rate);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(rate == 0, "rate = %ld\n", rate);

    hr = ISpVoice_SetRate(voice, 1);
    ok(hr == S_OK, "got %#lx.\n", hr);

    rate = 0xdeadbeef;
    hr = ISpVoice_GetRate(voice, &rate);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(rate == 1, "rate = %ld\n", rate);

    hr = ISpVoice_SetRate(voice, -1000);
    ok(hr == S_OK, "got %#lx.\n", hr);

    rate = 0xdeadbeef;
    hr = ISpVoice_GetRate(voice, &rate);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(rate == -1000, "rate = %ld\n", rate);

    hr = ISpVoice_SetRate(voice, 1000);
    ok(hr == S_OK, "got %#lx.\n", hr);

    rate = 0xdeadbeef;
    hr = ISpVoice_GetRate(voice, &rate);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(rate == 1000, "rate = %ld\n", rate);

    volume = 0xbeef;
    hr = ISpVoice_GetVolume(voice, &volume);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(volume == 100, "volume = %d\n", volume);

    hr = ISpVoice_SetVolume(voice, 0);
    ok(hr == S_OK, "got %#lx.\n", hr);

    volume = 0xbeef;
    hr = ISpVoice_GetVolume(voice, &volume);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(volume == 0, "volume = %d\n", volume);

    hr = ISpVoice_SetVolume(voice, 100);
    ok(hr == S_OK, "got %#lx.\n", hr);

    volume = 0xbeef;
    hr = ISpVoice_GetVolume(voice, &volume);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(volume == 100, "volume = %d\n", volume);

    hr = ISpVoice_SetVolume(voice, 101);
    ok(hr == E_INVALIDARG, "got %#lx.\n", hr);

    hr = CoRegisterClassObject(&CLSID_TestEngine, (IUnknown *)&test_engine_cf,
                               CLSCTX_INPROC_SERVER, REGCLS_MULTIPLEUSE, &regid);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpObjectToken, (void **)&token);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpObjectToken_SetId(token, NULL, test_token_id, TRUE);
    ok(hr == S_OK || broken(hr == E_ACCESSDENIED) /* w1064_adm */, "got %#lx.\n", hr);
    if (hr == E_ACCESSDENIED)
    {
        win_skip("token SetId access denied.\n");
        goto done;
    }

    ISpObjectToken_SetStringValue(token, L"CLSID", TESTENGINE_CLSID);
    hr = ISpObjectToken_CreateKey(token, L"Attributes", &attrs_key);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ISpDataKey_SetStringValue(attrs_key, L"Language", L"409");
    ISpDataKey_Release(attrs_key);

    hr = ISpVoice_SetVoice(voice, token);
    ok(hr == S_OK, "got %#lx.\n", hr);

    check_apttype();
    ok(test_apt_data.type == APTTYPE_MTA || broken(test_apt_data.type == APTTYPE_UNITIALIZED) /* w8, w10v1507 */,
       "got apt type %d.\n", test_apt_data.type);
    if (test_apt_data.type == APTTYPE_MTA)
        ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
           "got apt type qualifier %d.\n", test_apt_data.qualifier);
    else
        win_skip("apt type is not MTA.\n");

    test_engine.speak_called = FALSE;
    hr = ISpVoice_Speak(voice, NULL, SPF_PURGEBEFORESPEAK, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(!test_engine.speak_called, "ISpTTSEngine::Speak was called.\n");

    stream_num = 0xdeadbeef;
    hr = ISpVoice_Speak(voice, NULL, SPF_PURGEBEFORESPEAK, &stream_num);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(stream_num == 0xdeadbeef, "got %lu.\n", stream_num);

    ISpVoice_SetRate(voice, 0);
    ISpVoice_SetVolume(voice, 100);

    reset_engine_params(&test_engine);
    stream_num = 0xdeadbeef;
    start = GetTickCount();
    hr = ISpVoice_Speak(voice, test_text, SPF_DEFAULT, &stream_num);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.speak_called, "ISpTTSEngine::Speak was not called.\n");
    ok(test_engine.flags == SPF_DEFAULT, "got %#lx.\n", test_engine.flags);
    ok(test_engine.frag_list != NULL, "frag_list is NULL.\n");
    ok(test_engine.frag_list->pNext == NULL, "frag_list->pNext != NULL.\n");
    ok(test_engine.frag_list->ulTextLen == wcslen(test_text), "got %lu.\n", test_engine.frag_list->ulTextLen);
    ok(!wcsncmp(test_text, test_engine.frag_list->pTextStart, wcslen(test_text)),
       "got %s.\n", wine_dbgstr_w(test_engine.frag_list->pTextStart));
    ok(test_engine.rate == 0, "got %ld.\n", test_engine.rate);
    ok(test_engine.volume == 100, "got %d.\n", test_engine.volume);
    ok(stream_num == 1, "got %lu.\n", stream_num);
    ok(duration > 800 && duration < 3500, "took %lu ms.\n", duration);

    check_apttype();
    ok(test_apt_data.type == APTTYPE_MTA, "got apt type %d.\n", test_apt_data.type);
    ok(test_apt_data.qualifier == APTTYPEQUALIFIER_IMPLICIT_MTA,
       "got apt type qualifier %d.\n", test_apt_data.qualifier);

    start = GetTickCount();
    hr = ISpVoice_WaitUntilDone(voice, INFINITE);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(duration < 200, "took %lu ms.\n", duration);

    reset_engine_params(&test_engine);
    stream_num = 0xdeadbeef;
    start = GetTickCount();
    hr = ISpVoice_Speak(voice, test_text, SPF_DEFAULT | SPF_ASYNC | SPF_NLP_SPEAK_PUNC, &stream_num);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    todo_wine ok(stream_num == 1, "got %lu.\n", stream_num);
    ok(duration < 500, "took %lu ms.\n", duration);

    hr = ISpVoice_WaitUntilDone(voice, 100);
    ok(hr == S_FALSE, "got %#lx.\n", hr);

    hr = ISpVoice_WaitUntilDone(voice, INFINITE);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(duration > 800 && duration < 3500, "took %lu ms.\n", duration);

    ok(test_engine.speak_called, "ISpTTSEngine::Speak was not called.\n");
    ok(test_engine.flags == SPF_NLP_SPEAK_PUNC, "got %#lx.\n", test_engine.flags);
    ok(test_engine.frag_list != NULL, "frag_list is NULL.\n");
    ok(test_engine.frag_list->pNext == NULL, "frag_list->pNext != NULL.\n");
    ok(test_engine.frag_list->ulTextLen == wcslen(test_text), "got %lu.\n", test_engine.frag_list->ulTextLen);
    ok(!wcsncmp(test_text, test_engine.frag_list->pTextStart, wcslen(test_text)),
       "got %s.\n", wine_dbgstr_w(test_engine.frag_list->pTextStart));
    ok(test_engine.rate == 0, "got %ld.\n", test_engine.rate);
    ok(test_engine.volume == 100, "got %d.\n", test_engine.volume);

    reset_engine_params(&test_engine);
    hr = ISpVoice_Speak(voice, test_text, SPF_DEFAULT | SPF_ASYNC, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    Sleep(200);
    start = GetTickCount();
    hr = ISpVoice_Speak(voice, NULL, SPF_PURGEBEFORESPEAK, NULL);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(duration < 300, "took %lu ms.\n", duration);

done:
    reset_engine_params(&test_engine);
    ISpVoice_Release(voice);
    ISpObjectToken_Release(token);
    ISpMMSysAudio_Release(audio_out);

    RegDeleteTreeA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Winetest\\sapi" );
}

START_TEST(tts)
{
    CoInitialize(NULL);
    /* Run spvoice tests before interface tests so that a MTA won't be created before this test is run. */
    test_spvoice();
    test_interfaces();
    CoUninitialize();
}
