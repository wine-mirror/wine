/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_SpeechSynthesis
#include "windows.media.speechsynthesis.h"

#include "wine/test.h"

static HRESULT (WINAPI *pRoActivateInstance)(HSTRING, IInspectable **);
static HRESULT (WINAPI *pRoGetActivationFactory)(HSTRING, REFIID, void **);
static HRESULT (WINAPI *pRoInitialize)(RO_INIT_TYPE);
static void    (WINAPI *pRoUninitialize)(void);
static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);

static void test_SpeechSynthesizer(void)
{
    static const WCHAR *speech_synthesizer_name = L"Windows.Media.SpeechSynthesis.SpeechSynthesizer";

    IVectorView_VoiceInformation *voices = NULL;
    IInstalledVoicesStatic *voices_static = NULL;
    IActivationFactory *factory = NULL;
    IVoiceInformation *voice;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    HSTRING str;
    HRESULT hr;
    UINT32 size;

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = pWindowsCreateString(speech_synthesizer_name, wcslen(speech_synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IInstalledVoicesStatic, (void **)&voices_static);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInstalledVoicesStatic failed, hr %#x\n", hr);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IInspectable failed, hr %#x\n", hr);
    ok(tmp_inspectable == inspectable, "IInstalledVoicesStatic_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);
    ok(tmp_agile_object == agile_object, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IInstalledVoicesStatic_get_AllVoices(voices_static, &voices);
    ok(hr == S_OK, "IInstalledVoicesStatic_get_AllVoices failed, hr %#x\n", hr);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#x\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_VoiceInformation_QueryInterface voices returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == E_NOINTERFACE, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#x\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_VoiceInformation_get_Size(voices, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_get_Size voices failed, hr %#x\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_VoiceInformation_get_Size returned %u\n", size);

    voice = (IVoiceInformation *)0xdeadbeef;
    hr = IVectorView_VoiceInformation_GetAt(voices, size, &voice);
    ok(hr == E_BOUNDS, "IVectorView_VoiceInformation_GetAt failed, hr %#x\n", hr);
    ok(voice == NULL, "IVectorView_VoiceInformation_GetAt returned %p\n", voice);

    hr = IVectorView_VoiceInformation_GetMany(voices, size, 1, &voice, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_GetMany failed, hr %#x\n", hr);
    ok(size == 0, "IVectorView_VoiceInformation_GetMany returned count %u\n", size);

    IVectorView_VoiceInformation_Release(voices);

    IInstalledVoicesStatic_Release(voices_static);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    pWindowsDeleteString(str);

    pRoUninitialize();
}

static void test_VoiceInformation(void)
{
    static const WCHAR *voice_information_name = L"Windows.Media.SpeechSynthesis.VoiceInformation";

    IActivationFactory *factory = NULL;
    HSTRING str;
    HRESULT hr;

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = pWindowsCreateString(voice_information_name, wcslen(voice_information_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == REGDB_E_CLASSNOTREG, "RoGetActivationFactory returned unexpected hr %#x\n", hr);

    pWindowsDeleteString(str);

    pRoUninitialize();
}

START_TEST(speech)
{
    HMODULE combase;

    if (!(combase = LoadLibraryW(L"combase.dll")))
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return;
    }

#define LOAD_FUNCPTR(x) \
    if (!(p##x = (void*)GetProcAddress(combase, #x))) \
    { \
        win_skip("Failed to find %s in combase.dll, skipping tests.\n", #x); \
        return; \
    }

    LOAD_FUNCPTR(RoActivateInstance);
    LOAD_FUNCPTR(RoGetActivationFactory);
    LOAD_FUNCPTR(RoInitialize);
    LOAD_FUNCPTR(RoUninitialize);
    LOAD_FUNCPTR(WindowsCreateString);
    LOAD_FUNCPTR(WindowsDeleteString);
#undef LOAD_FUNCPTR

    test_SpeechSynthesizer();
    test_VoiceInformation();
}
