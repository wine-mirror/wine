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

    BOOL simulate_output;
    BOOL speak_called;
    DWORD flags;
    GUID fmtid;
    SPVTEXTFRAG *frags;
    size_t frag_count;
    LONG rate;
    USHORT volume;
};

/* Copy frag_list into a contiguous array allocated by a single malloc().
 * The texts are allocated at the end of the array. */
static void copy_frag_list(const SPVTEXTFRAG *frag_list, SPVTEXTFRAG **ret_frags, size_t *frag_count)
{
    const SPVTEXTFRAG *frag;
    SPVTEXTFRAG *cur;
    WCHAR *cur_text;
    size_t size = 0;

    *frag_count = 0;

    if (!frag_list)
    {
        *ret_frags = NULL;
        return;
    }

    for (frag = frag_list; frag; frag = frag->pNext)
    {
        size += sizeof(*frag) + (frag->ulTextLen + 1) * sizeof(WCHAR);
        (*frag_count)++;
    }

    *ret_frags = malloc(size);
    cur = *ret_frags;
    cur_text = (WCHAR *)(*ret_frags + (*frag_count));

    for (frag = frag_list; frag; frag = frag->pNext, ++cur)
    {
        memcpy(cur, frag, sizeof(*frag));

        cur->pNext = frag->pNext ? cur + 1 : NULL;

        if (frag->pTextStart)
        {
            memcpy(cur_text, frag->pTextStart, frag->ulTextLen * sizeof(WCHAR));
            cur_text[frag->ulTextLen] = L'\0';

            cur->pTextStart = (WCHAR *)cur_text;
            cur_text += frag->ulTextLen + 1;
        }
    }
}

static void reset_engine_params(struct test_engine *engine)
{
    engine->simulate_output = FALSE;
    engine->speak_called = FALSE;
    engine->flags = 0xdeadbeef;
    memset(&engine->fmtid, 0xde, sizeof(engine->fmtid));
    engine->rate = 0xdeadbeef;
    engine->volume = 0xbeef;

    free(engine->frags);
    engine->frags = NULL;
    engine->frag_count = 0;
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
    copy_frag_list(frag_list, &engine->frags, &engine->frag_count);
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

    if (!engine->simulate_output)
        return S_OK;

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

static const WCHAR test_token_id[] = L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Speech\\Voices\\Tokens\\WinetestVoice";

static BOOL test_token_created = FALSE;

#define check_frag_text(i, exp) \
    ok(!wcscmp(test_engine.frags[i].pTextStart, exp), "frag %d text: got %s.\n", \
        i, wine_dbgstr_w(test_engine.frags[i].pTextStart))

#define check_frag_text_src_offset(i, exp) \
    ok(test_engine.frags[i].ulTextSrcOffset == exp, "frag %d text src offset: got %lu.\n", \
        i, test_engine.frags[i].ulTextSrcOffset)

#define check_frag_state_field(i, name, exp, fmt) \
    ok(test_engine.frags[i].State.name == exp, "frag %d state " #name ": got " fmt ".\n", \
        i, test_engine.frags[i].State.name)

static void test_spvoice(void)
{
    static const WCHAR test_text[] = L"Hello! This is a test sentence.";
    static const WCHAR *get_voices = L"GetVoices";

    ISpVoice *voice;
    IUnknown *dummy;
    ISpMMSysAudio *audio_out;
    ISpObjectTokenCategory *token_cat;
    ISpObjectToken *token, *token2;
    WCHAR *token_id = NULL, *default_token_id = NULL;
    ISpDataKey *attrs_key;
    LONG rate;
    USHORT volume;
    ULONG stream_num;
    DWORD regid;
    DWORD start, duration;
    ISpeechVoice *speech_voice;
    ISpeechObjectTokens *speech_tokens;
    LONG count, volume_long;
    ISpeechObjectToken *speech_token;
    BSTR req = NULL, opt = NULL;
    UINT info_count;
    ITypeInfo *typeinfo;
    TYPEATTR *typeattr;
    DISPID dispid;
    DISPPARAMS params;
    VARIANT args[2], ret;
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
    ISpDataKey_SetStringValue(attrs_key, L"Vendor", L"Winetest");
    ISpDataKey_Release(attrs_key);

    test_token_created = TRUE;

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
    test_engine.simulate_output = TRUE;
    stream_num = 0xdeadbeef;
    start = GetTickCount();
    hr = ISpVoice_Speak(voice, test_text, SPF_DEFAULT, &stream_num);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.speak_called, "ISpTTSEngine::Speak was not called.\n");
    ok(test_engine.flags == SPF_DEFAULT, "got %#lx.\n", test_engine.flags);
    ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
    check_frag_text(0, test_text);
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
    test_engine.simulate_output = TRUE;
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
    ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
    check_frag_text(0, test_text);
    check_frag_text_src_offset(0, 0);
    ok(test_engine.rate == 0, "got %ld.\n", test_engine.rate);
    ok(test_engine.volume == 100, "got %d.\n", test_engine.volume);

    reset_engine_params(&test_engine);
    test_engine.simulate_output = TRUE;
    hr = ISpVoice_Speak(voice, test_text, SPF_DEFAULT | SPF_ASYNC, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    Sleep(200);
    start = GetTickCount();
    hr = ISpVoice_Speak(voice, NULL, SPF_PURGEBEFORESPEAK, NULL);
    duration = GetTickCount() - start;
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(duration < 300, "took %lu ms.\n", duration);

    hr = ISpVoice_QueryInterface(voice, &IID_ISpeechVoice, (void **)&speech_voice);
    ok(hr == S_OK, "got %#lx.\n", hr);

    count = -1;
    hr = ISpeechVoice_GetVoices(speech_voice, NULL, NULL, &speech_tokens);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = ISpeechObjectTokens_get_Count(speech_tokens, &count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(count > 0, "got %ld.\n", count);
    ISpeechObjectTokens_Release(speech_tokens);

    req = SysAllocString(L"Vendor=Winetest");
    opt = SysAllocString(L"Language=409;Gender=Male");

    count = 0xdeadbeef;
    hr = ISpeechVoice_GetVoices(speech_voice, req, opt, &speech_tokens);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = ISpeechObjectTokens_get_Count(speech_tokens, &count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(count == 1, "got %ld.\n", count);
    ISpeechObjectTokens_Release(speech_tokens);

    volume_long = 0xdeadbeef;
    hr = ISpeechVoice_put_Volume(speech_voice, 80);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = ISpeechVoice_get_Volume(speech_voice, &volume_long);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(volume_long == 80, "got %ld.\n", volume_long);

    hr = ISpObjectToken_QueryInterface(token, &IID_ISpeechObjectToken, (void **)&speech_token);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = ISpeechVoice_putref_Voice(speech_voice, speech_token);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ISpeechObjectToken_Release(speech_token);

    speech_token = (ISpeechObjectToken *)0xdeadbeef;
    hr = ISpeechVoice_get_Voice(speech_voice, &speech_token);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(speech_token && speech_token != (ISpeechObjectToken *)0xdeadbeef, "got %p.\n", speech_token);
    hr = ISpeechObjectToken_QueryInterface(speech_token, &IID_ISpObjectToken, (void **)&token2);
    ok(hr == S_OK, "got %#lx.\n", hr);
    token_id = NULL;
    hr = ISpObjectToken_GetId(token2, &token_id);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(!wcscmp(token_id, test_token_id), "got %s.\n", wine_dbgstr_w(token_id));
    CoTaskMemFree(token_id);
    ISpObjectToken_Release(token2);

    hr = ISpeechVoice_Speak(speech_voice, NULL, SVSFPurgeBeforeSpeak, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    info_count = 0xdeadbeef;
    hr = ISpeechVoice_GetTypeInfoCount(speech_voice, &info_count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(info_count == 1, "got %u.\n", info_count);

    typeinfo = NULL;
    typeattr = NULL;
    hr = ISpeechVoice_GetTypeInfo(speech_voice, 0, 0, &typeinfo);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = ITypeInfo_GetTypeAttr(typeinfo, &typeattr);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(typeattr->typekind == TKIND_DISPATCH, "got %u.\n", typeattr->typekind);
    ok(IsEqualGUID(&typeattr->guid, &IID_ISpeechVoice), "got %s.\n", wine_dbgstr_guid(&typeattr->guid));
    ITypeInfo_ReleaseTypeAttr(typeinfo, typeattr);
    ITypeInfo_Release(typeinfo);

    dispid = 0xdeadbeef;
    hr = ISpeechVoice_GetIDsOfNames(speech_voice, &IID_NULL, (WCHAR **)&get_voices, 1, 0x409, &dispid);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(dispid == DISPID_SVGetVoices, "got %#lx.\n", dispid);

    memset(&params, 0, sizeof(params));
    params.cArgs = 2;
    params.cNamedArgs = 0;
    params.rgvarg = args;
    VariantInit(&args[0]);
    VariantInit(&args[1]);
    V_VT(&args[0]) = VT_BSTR;
    V_VT(&args[1]) = VT_BSTR;
    V_BSTR(&args[0]) = opt;
    V_BSTR(&args[1]) = req;
    VariantInit(&ret);
    hr = ISpeechVoice_Invoke(speech_voice, dispid, &IID_NULL, 0, DISPATCH_METHOD, &params, &ret, NULL, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(V_VT(&ret) == VT_DISPATCH, "got %#x.\n", V_VT(&ret));
    hr = IDispatch_QueryInterface(V_DISPATCH(&ret), &IID_ISpeechObjectTokens, (void **)&speech_tokens);
    ok(hr == S_OK, "got %#lx.\n", hr);
    count = -1;
    hr = ISpeechObjectTokens_get_Count(speech_tokens, &count);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(count == 1, "got %ld.\n", count);
    ISpeechObjectTokens_Release(speech_tokens);
    VariantClear(&ret);

    ISpeechVoice_Release(speech_voice);

done:
    reset_engine_params(&test_engine);
    ISpVoice_Release(voice);
    ISpObjectToken_Release(token);
    ISpMMSysAudio_Release(audio_out);
    SysFreeString(req);
    SysFreeString(opt);
}

static void test_spvoice_ssml(void)
{
    static const WCHAR text1[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>text1</speak>";

    /* Only version 1.0 is supported in SAPI. */
    static const WCHAR bad_text1[] =
        L"<speak version='1.1' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>text1</speak>";

    /* version attribute is required in <speak>. */
    static const WCHAR bad_text2[] =
        L"<speak xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>text1</speak>";

    /* xml:lang attribute is required in <speak>. */
    static const WCHAR bad_text3[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis'>text1</speak>";

    /* xmlns is not required in <speak>. */
    static const WCHAR text2[] =
        L"<speak version='1.0' xml:lang='en-US'>text2</speak>";

    static const WCHAR text3[] =
        L"<?xml version='1.0' encoding='utf-8'?>"
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>\n"
        L"P1S1. P1S2.\n"
        L"\n"
        L"P2."
        L"<p>P3.</p>"
        L"<p><s>P4, S1. P4S2.</s><s>P4S3.</s></p>"
        L"<p>\u4F0D</p>"
        L"<p>\U0001240B</p>" /* Two WCHARs needed for \U0001240B */
        L"<p>P7.</p>"
        L"</speak>";

    static const WCHAR text4[] =
        L"<?xml version='1.0' encoding='utf-16'?>"
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>"
        L"<s>One, <prosody rate='-85%'>two.</prosody></s>"
        L"</speak>";

    static const WCHAR text5[] =
        L"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        L"<speak version=\"1.0\" xmlns=\"http://www.w3.org/2001/10/synthesis\" xml:lang=\"en-us\">"
        L"<prosody rate=\"50%\">50%.</prosody>"
        L"<prosody rate='+50%'>+50%.</prosody>"
        L"<prosody rate='6'>6.</prosody>"
        L"<prosody rate='0.01000001'>0.01000001.</prosody>"
        L"<prosody rate='0.01'>0.01.</prosody>"
        L"<prosody rate='0'>0.</prosody>"
        L"<prosody rate='-1.0'>-1.0.</prosody>"
        L"<prosody rate='6'><prosody rate='3'>3.</prosody></prosody>"
        L"</speak>";

    static const WCHAR text6[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>"
        L"<prosody rate='x-slow'>x-slow.</prosody>"
        L"<prosody rate='slow'>slow.</prosody>"
        L"<prosody rate='medium'>medium.</prosody>"
        L"<prosody rate='fast'>fast.</prosody>"
        L"<prosody rate='x-fast'>x-fast.</prosody>"
        L"</speak>";

    static const WCHAR text7[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>"
        L"One,<prosody rate='x-fast'/> Two." /* Empty tags are ignored. */
        L"<prosody rate='fast'>"
        L"  Three.<prosody rate='x-fast'>Four.</prosody>"
        L"</prosody>"
        L"Five."
        L"</speak>";

    static const WCHAR text8[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>"
        L"<prosody volume='50%'>50%.</prosody>"
        L"<prosody volume='-50%'>-50%.</prosody>"
        L"<prosody volume='10'>10.</prosody>"
        L"<prosody volume='+10'>+10.</prosody>"
        L"<prosody volume='-10.1' rate='+200%'>-10.1.</prosody>"
        L"<prosody volume='-50%'><prosody volume='-50%'>25.</prosody></prosody>"
        L"<prosody volume='-50%'><prosody volume='50%'>75.</prosody></prosody>"
        L"<prosody volume='-50%'><prosody volume='+50'>100.</prosody></prosody>"
        L"<prosody volume='-50%'><prosody volume='50'>50.</prosody></prosody>"
        L"</speak>";

    static const WCHAR text9[] =
        L"<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='en-us'>"
        L"<prosody volume='silent'>silent.</prosody>"
        L"<prosody volume='x-soft'>x-soft.</prosody>"
        L"<prosody volume='soft'>soft.</prosody>"
        L"<prosody volume='medium'>medium.</prosody>"
        L"<prosody volume='loud'>loud.</prosody>"
        L"<prosody volume='x-loud'>x-loud.</prosody>"
        L"<prosody volume='loud'><prosody volume='soft'>soft.</prosody></prosody>"
        L"</speak>";


    ISpVoice *voice;
    ISpObjectToken *token;
    HRESULT hr;

    if (waveOutGetNumDevs() == 0) {
        skip("no wave out devices.\n");
        return;
    }

    if (!test_token_created) {
        /* w1064_adm */
        win_skip("Test token not created.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpVoice, (void **)&voice);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, NULL, TRUE);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SpObjectToken, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpObjectToken, (void **)&token);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpObjectToken_SetId(token, NULL, test_token_id, FALSE);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = ISpVoice_SetVoice(voice, token);
    ok(hr == S_OK, "got %#lx.\n", hr);

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text1, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
    check_frag_text(0, L"text1");

    check_frag_state_field(0, eAction, SPVA_Speak, "%d");
    ok(test_engine.frags[0].State.LangID == 0x409 || broken(test_engine.frags[0].State.LangID == 0) /* win7 */,
       "got %#hx.\n", test_engine.frags[0].State.LangID);
    check_frag_state_field(0, EmphAdj, 0, "%ld");
    check_frag_state_field(0, RateAdj, 0, "%ld");
    check_frag_state_field(0, Volume, 100, "%lu");
    check_frag_state_field(0, PitchAdj.MiddleAdj, 0, "%ld");
    check_frag_state_field(0, PitchAdj.RangeAdj, 0, "%ld");
    check_frag_state_field(0, SilenceMSecs, 0, "%lu");
    check_frag_state_field(0, ePartOfSpeech, SPPS_Unknown, "%#x");

    reset_engine_params(&test_engine);

    /* SSML autodetection when SPF_PARSE_SSML is not specified. */
    hr = ISpVoice_Speak(voice, text1, SPF_IS_XML, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
    check_frag_text(0, L"text1");

    reset_engine_params(&test_engine);

    /* XML and SSML autodetection when SPF_IS_XML is not specified. */
    hr = ISpVoice_Speak(voice, text1, SPF_DEFAULT, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
    check_frag_text(0, L"text1");

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, bad_text1, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    ok(hr == SPERR_UNSUPPORTED_FORMAT, "got %#lx.\n", hr);

    hr = ISpVoice_Speak(voice, bad_text2, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    ok(hr == SPERR_UNSUPPORTED_FORMAT, "got %#lx.\n", hr);

    hr = ISpVoice_Speak(voice, bad_text3, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    ok(hr == SPERR_UNSUPPORTED_FORMAT || broken(hr == S_OK) /* win7 */, "got %#lx.\n", hr);

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text2, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    ok(hr == S_OK || broken(hr == SPERR_UNSUPPORTED_FORMAT) /* win7 */, "got %#lx.\n", hr);

    if (hr == S_OK) {
        ok(test_engine.frag_count == 1, "got %Iu.\n", test_engine.frag_count);
        check_frag_text(0, L"text2");
        check_frag_state_field(0, eAction, SPVA_Speak, "%d");
        check_frag_state_field(0, LangID, 0x409, "%#hx");
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text3, SPF_IS_XML, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    ok(test_engine.frag_count == 7 || broken(test_engine.frag_count == 1) /* win7 */,
       "got %Iu.\n", test_engine.frag_count);

    if (test_engine.frag_count == 7) {
        check_frag_text(0, L"\nP1S1. P1S2.\n\nP2.");
        check_frag_text_src_offset(0, 120);
        check_frag_state_field(0, eAction, SPVA_Speak, "%d");

        check_frag_text(1, L"P3.");
        check_frag_text_src_offset(1, 140);
        check_frag_state_field(1, eAction, SPVA_Speak, "%d");

        check_frag_text(2, L"P4, S1. P4S2.");
        check_frag_text_src_offset(2, 153);
        check_frag_state_field(2, eAction, SPVA_Speak, "%d");

        check_frag_text(3, L"P4S3.");
        check_frag_text_src_offset(3, 173);
        check_frag_state_field(3, eAction, SPVA_Speak, "%d");

        check_frag_text(4, L"\u4F0D");
        check_frag_text_src_offset(4, 189);
        check_frag_state_field(4, eAction, SPVA_Speak, "%d");

        check_frag_text(5, L"\U0001240B");
        ok(test_engine.frags[5].ulTextSrcOffset == 197 ||       /* 189 + 8 = 197 */
           broken(test_engine.frags[5].ulTextSrcOffset == 196), /* Windows gives incorrect offset here */
           "got %lu.\n", test_engine.frags[5].ulTextSrcOffset);

        check_frag_text(6, L"P7.");
        check_frag_text_src_offset(6, test_engine.frags[5].ulTextSrcOffset + 9);
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text4, SPF_DEFAULT, NULL);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);
    todo_wine ok(test_engine.frag_count == 2, "got %Iu.\n", test_engine.frag_count);

    if (test_engine.frag_count == 2) {
        check_frag_text(0, L"One, ");
        check_frag_state_field(0, eAction, SPVA_Speak, "%d");
        check_frag_state_field(0, RateAdj, 0, "%ld");

        check_frag_text(1, L"two.");
        check_frag_state_field(1, eAction, SPVA_Speak, "%d");
        check_frag_state_field(1, RateAdj, -17, "%ld"); /* 3^(-17/10) ~= 0.15 */
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text5, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);
    todo_wine ok(test_engine.frag_count == 8 || broken(test_engine.frag_count == 3) /* win7 */,
       "got %Iu.\n", test_engine.frag_count);

    if (test_engine.frag_count == 8) {
        check_frag_state_field(0, RateAdj,   4, "%ld"); /* 3^(4/10)   ~= 1.5          */
        check_frag_state_field(1, RateAdj,   4, "%ld"); /* 3^(4/10)   ~= 1.5          */
        check_frag_state_field(2, RateAdj,  16, "%ld"); /* 3^(16/10)  ~= 6            */
        check_frag_state_field(3, RateAdj, -42, "%ld"); /* 3^(-42/10) ~= 0.01000001   */
        check_frag_state_field(4, RateAdj, -10, "%ld"); /* rate = 0.01                */
        check_frag_state_field(5, RateAdj, -10, "%ld"); /* rate = 0                   */
        check_frag_state_field(6, RateAdj,   0, "%ld"); /* negative rates are ignored */
        check_frag_state_field(7, RateAdj,  10, "%ld"); /* 3^(10/10)   = 3            */
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text6, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    todo_wine ok(hr == S_OK || broken(hr == SPERR_UNSUPPORTED_FORMAT) /* win7 */, "got %#lx.\n", hr);

    if (hr == S_OK) {
        ok(test_engine.frag_count == 5, "got %Iu.\n", test_engine.frag_count);

        check_frag_state_field(0, RateAdj, -9, "%ld"); /* x-slow */
        check_frag_state_field(1, RateAdj, -4, "%ld"); /* slow   */
        check_frag_state_field(2, RateAdj,  0, "%ld"); /* medium */
        check_frag_state_field(3, RateAdj,  4, "%ld"); /* fast   */
        check_frag_state_field(4, RateAdj,  9, "%ld"); /* x-fast */
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text7, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    todo_wine ok(hr == S_OK || broken(hr == SPERR_UNSUPPORTED_FORMAT) /* win7 */, "got %#lx.\n", hr);

    if (hr == S_OK) {
        ok(test_engine.frag_count == 5, "got %Iu.\n", test_engine.frag_count);

        check_frag_text(0, L"One,");
        check_frag_state_field(0, RateAdj, 0, "%ld");

        check_frag_text(1, L" Two.");
        check_frag_state_field(1, RateAdj, 0, "%ld");

        check_frag_text(2, L"  Three.");
        check_frag_state_field(2, RateAdj, 4, "%ld");

        check_frag_text(3, L"Four.");
        check_frag_state_field(3, RateAdj, 9, "%ld");

        check_frag_text(4, L"Five.");
        check_frag_state_field(4, RateAdj, 0, "%ld");
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text8, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);

    if (hr == S_OK) {
        ok(test_engine.frag_count == 9, "got %Iu.\n", test_engine.frag_count);

        ok(test_engine.frags[0].State.Volume == 100 || broken(test_engine.frags[0].State.Volume == 50) /* win7 */,
           "got %lu.\n", test_engine.frags[0].State.Volume);
        check_frag_state_field(1,  Volume,  50, "%ld");
        check_frag_state_field(2,  Volume,  10, "%lu");
        check_frag_state_field(3,  Volume, 100, "%lu");

        check_frag_state_field(4,  Volume,  90, "%lu");
        check_frag_state_field(4, RateAdj,  10, "%ld");

        check_frag_state_field(5,  Volume,  25, "%lu");
        ok(test_engine.frags[6].State.Volume == 75 || broken(test_engine.frags[6].State.Volume == 25) /* win7 */,
           "got %lu.\n", test_engine.frags[6].State.Volume);
        check_frag_state_field(7,  Volume, 100, "%lu");
        check_frag_state_field(8,  Volume,  50, "%lu");
    }

    reset_engine_params(&test_engine);

    hr = ISpVoice_Speak(voice, text9, SPF_IS_XML | SPF_PARSE_SSML, NULL);
    todo_wine ok(hr == S_OK || broken(hr == SPERR_UNSUPPORTED_FORMAT) /* win7 */, "got %#lx.\n", hr);

    if (hr == S_OK) {
        ok(test_engine.frag_count == 7, "got %Iu.\n", test_engine.frag_count);

        check_frag_state_field(0, Volume,   0, "%lu"); /* silent */
        check_frag_state_field(1, Volume,  20, "%lu"); /* x-soft */
        check_frag_state_field(2, Volume,  40, "%lu"); /* soft   */
        check_frag_state_field(3, Volume,  60, "%lu"); /* medium */
        check_frag_state_field(4, Volume,  80, "%lu"); /* loud   */
        check_frag_state_field(5, Volume, 100, "%lu"); /* x-loud */

        check_frag_state_field(6, Volume,  40, "%lu"); /* soft   */
    }

    reset_engine_params(&test_engine);
    ISpVoice_Release(voice);
    ISpObjectToken_Release(token);
}

START_TEST(tts)
{
    CoInitialize(NULL);
    RegDeleteTreeA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Speech\\Voices\\WinetestVoice");

    /* Run spvoice tests before interface tests so that a MTA won't be created before this test is run. */
    test_spvoice();
    test_spvoice_ssml();
    test_interfaces();

    RegDeleteTreeA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Speech\\Voices\\WinetestVoice");
    CoUninitialize();
}
