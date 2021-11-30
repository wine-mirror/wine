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

HRESULT WINAPI (*pDllGetActivationFactory)(HSTRING, IActivationFactory **);

static void test_SpeechSynthesizer(void)
{
    static const WCHAR *speech_synthesizer_name = L"Windows.Media.SpeechSynthesis.SpeechSynthesizer";
    static const WCHAR *speech_synthesizer_name2 = L"windows.media.speechsynthesis.speechsynthesizer";
    static const WCHAR *unknown_class_name = L"Unknown.Class";
    IActivationFactory *factory = NULL, *factory2 = NULL;
    IVectorView_VoiceInformation *voices = NULL;
    IInstalledVoicesStatic *voices_static = NULL;
    IVoiceInformation *voice;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    ISpeechSynthesizer *synthesizer;
    IClosable *closable;
    HMODULE hdll;
    HSTRING str, str2;
    HRESULT hr;
    UINT32 size;
    ULONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = WindowsCreateString(speech_synthesizer_name, wcslen(speech_synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hdll = LoadLibraryW(L"windows.media.speech.dll");
    if (hdll)
    {
        pDllGetActivationFactory = (void *)GetProcAddress(hdll, "DllGetActivationFactory");
        ok(!!pDllGetActivationFactory, "DllGetActivationFactory not found.\n");

        hr = WindowsCreateString(unknown_class_name, wcslen(unknown_class_name), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

        hr = pDllGetActivationFactory(str2, &factory);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

        WindowsDeleteString(str2);

        hr = WindowsCreateString(speech_synthesizer_name2, wcslen(speech_synthesizer_name2), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

        hr = pDllGetActivationFactory(str2, &factory2);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

        WindowsDeleteString(str2);

        hr = pDllGetActivationFactory(str, &factory2);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        win_skip("Failed to load library, err %u.\n", GetLastError());
    }

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#x\n", hr);

    if (hdll)
    {
        ok(factory == factory2, "Got unexpected factory %p, factory2 %p.\n", factory, factory2);
        IActivationFactory_Release(factory2);
        FreeLibrary(hdll);
    }

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

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);

    hr = RoActivateInstance(str, &inspectable);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_IClosable, (void **)&closable);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    ref = IClosable_Release(closable);
    ok(ref == 2, "Got unexpected ref %u.\n", ref);

    ref = ISpeechSynthesizer_Release(synthesizer);
    ok(ref == 1, "Got unexpected ref %u.\n", ref);

    ref = IInspectable_Release(inspectable);
    ok(!ref, "Got unexpected ref %u.\n", ref);

    IActivationFactory_Release(factory);
    WindowsDeleteString(str);

    RoUninitialize();
}

static void test_VoiceInformation(void)
{
    static const WCHAR *voice_information_name = L"Windows.Media.SpeechSynthesis.VoiceInformation";

    IActivationFactory *factory = NULL;
    HSTRING str;
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = WindowsCreateString(voice_information_name, wcslen(voice_information_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == REGDB_E_CLASSNOTREG, "RoGetActivationFactory returned unexpected hr %#x\n", hr);

    WindowsDeleteString(str);

    RoUninitialize();
}

START_TEST(speech)
{
    test_SpeechSynthesizer();
    test_VoiceInformation();
}
