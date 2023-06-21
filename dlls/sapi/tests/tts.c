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

static void test_interfaces(void)
{
    ISpeechVoice *speech_voice, *speech_voice2;
    IConnectionPointContainer *container;
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

    ISpeechVoice_Release(speech_voice);
}

static void test_spvoice(void)
{
    ISpVoice *voice;
    ISpMMSysAudio *audio_out;
    ISpObjectTokenCategory *token_cat;
    ISpObjectToken *token;
    WCHAR *token_id = NULL, *default_token_id = NULL;
    LONG rate;
    USHORT volume;
    HRESULT hr;

    if (waveOutGetNumDevs() == 0) {
        skip("no wave out devices.\n");
        return;
    }

    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpVoice, (void **)&voice);
    ok(hr == S_OK, "Failed to create SpVoice: %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, NULL, TRUE);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = CoCreateInstance(&CLSID_SpMMAudioOut, NULL, CLSCTX_INPROC_SERVER,
                          &IID_ISpMMSysAudio, (void **)&audio_out);
    ok(hr == S_OK, "Failed to create SpMMAudioOut: %#lx.\n", hr);

    hr = ISpVoice_SetOutput(voice, (IUnknown *)audio_out, TRUE);
    todo_wine ok(hr == S_FALSE, "got %#lx.\n", hr);

    hr = ISpVoice_SetVoice(voice, NULL);
    todo_wine ok(hr == S_OK, "got %#lx.\n", hr);

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

    ISpVoice_Release(voice);
    ISpMMSysAudio_Release(audio_out);
}

START_TEST(tts)
{
    CoInitialize(NULL);
    test_interfaces();
    test_spvoice();
    CoUninitialize();
}
