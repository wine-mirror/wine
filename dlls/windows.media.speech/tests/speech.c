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
#define WIDL_using_Windows_Globalization
#include "windows.globalization.h"
#define WIDL_using_Windows_Media_SpeechRecognition
#include "windows.media.speechrecognition.h"
#define WIDL_using_Windows_Media_SpeechSynthesis
#include "windows.media.speechsynthesis.h"

#include "wine/test.h"

HRESULT WINAPI (*pDllGetActivationFactory)(HSTRING, IActivationFactory **);

static inline LONG get_ref(IUnknown *obj)
{
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

#define check_refcount(obj, exp) check_refcount_(__LINE__, obj, exp)
static inline void check_refcount_(unsigned int line, void *obj, LONG exp)
{
    LONG ref = get_ref(obj);
    ok_(__FILE__, line)(exp == ref, "Unexpected refcount %lu, expected %lu\n", ref, exp);
}

#define check_interface(obj, iid, exp) check_interface_(__LINE__, obj, iid, exp)
static void check_interface_(unsigned int line, void *obj, const IID *iid, BOOL supported)
{
    IUnknown *iface = obj;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

static void test_ActivationFactory(void)
{
    static const WCHAR *synthesizer_name = L"Windows.Media.SpeechSynthesis.SpeechSynthesizer";
    static const WCHAR *recognizer_name = L"Windows.Media.SpeechRecognition.SpeechRecognizer";
    static const WCHAR *garbage_name = L"Some.Garbage.Class";
    IActivationFactory *factory = NULL, *factory2 = NULL, *factory3 = NULL, *factory4 = NULL;
    ISpeechRecognizerStatics2 *recognizer_statics2 = NULL;
    HSTRING str, str2, str3;
    HMODULE hdll;
    HRESULT hr;
    ULONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(synthesizer_name, wcslen(synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(recognizer_name, wcslen(recognizer_name), &str2);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(garbage_name, wcslen(garbage_name), &str3);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx.\n", hr);

    check_refcount(factory, 2);

    check_interface(factory, &IID_IInspectable, TRUE);
    check_interface(factory, &IID_IAgileObject, TRUE);
    check_interface(factory, &IID_IInstalledVoicesStatic, TRUE);
    check_interface(factory, &IID_ISpeechRecognizerFactory, FALSE);
    check_interface(factory, &IID_ISpeechRecognizerStatics, FALSE);
    check_interface(factory, &IID_ISpeechRecognizerStatics2, FALSE);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory2);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx.\n", hr);
    ok(factory == factory2, "Factories pointed at factory %p factory2 %p.\n", factory, factory2);
    check_refcount(factory2, 3);

    hr = RoGetActivationFactory(str2, &IID_IActivationFactory, (void **)&factory3);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "Got unexpected hr %#lx.\n", hr);

    if (hr == S_OK) /* Win10+ only */
    {
        check_refcount(factory3, 2);
        ok(factory != factory3, "Factories pointed at factory %p factory3 %p.\n", factory, factory3);

        check_interface(factory3, &IID_IInspectable, TRUE);
        check_interface(factory3, &IID_IAgileObject, TRUE);
        check_interface(factory3, &IID_ISpeechRecognizerFactory, TRUE);
        check_interface(factory3, &IID_ISpeechRecognizerStatics, TRUE);

        hr = IActivationFactory_QueryInterface(factory3, &IID_ISpeechRecognizerStatics2, (void **)&recognizer_statics2);
        ok(hr == S_OK || broken(hr == E_NOINTERFACE), "IActivationFactory_QueryInterface failed, hr %#lx.\n", hr);

        if (hr == S_OK) /* ISpeechRecognizerStatics2 not available in Win10 1507 */
        {
            ref = ISpeechRecognizerStatics2_Release(recognizer_statics2);
            ok(ref == 2, "Got unexpected refcount: %lu.\n", ref);
        }

        check_interface(factory3, &IID_IInstalledVoicesStatic, FALSE);

        ref = IActivationFactory_Release(factory3);
        ok(ref == 1, "Got unexpected refcount: %lu.\n", ref);
    }

    hdll = LoadLibraryW(L"windows.media.speech.dll");

    if(hdll)
    {
        pDllGetActivationFactory = (void *)GetProcAddress(hdll, "DllGetActivationFactory");
        ok(!!pDllGetActivationFactory, "DllGetActivationFactory not found.\n");

        hr = pDllGetActivationFactory(str3, &factory4);
        ok((hr == CLASS_E_CLASSNOTAVAILABLE), "Got unexpected hr %#lx.\n", hr);
        FreeLibrary(hdll);
    }

    hr = RoGetActivationFactory(str3, &IID_IActivationFactory, (void **)&factory4);
    ok((hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);

    ref = IActivationFactory_Release(factory2);
    ok(ref == 2, "Got unexpected refcount: %lu.\n", ref);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected refcount: %lu.\n", ref);

    WindowsDeleteString(str);
    WindowsDeleteString(str2);
    WindowsDeleteString(str3);

    RoUninitialize();
}

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
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(speech_synthesizer_name, wcslen(speech_synthesizer_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hdll = LoadLibraryW(L"windows.media.speech.dll");
    if (hdll)
    {
        pDllGetActivationFactory = (void *)GetProcAddress(hdll, "DllGetActivationFactory");
        ok(!!pDllGetActivationFactory, "DllGetActivationFactory not found.\n");

        hr = WindowsCreateString(unknown_class_name, wcslen(unknown_class_name), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

        hr = pDllGetActivationFactory(str2, &factory);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

        WindowsDeleteString(str2);

        hr = WindowsCreateString(speech_synthesizer_name2, wcslen(speech_synthesizer_name2), &str2);
        ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

        hr = pDllGetActivationFactory(str2, &factory2);
        ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

        WindowsDeleteString(str2);

        hr = pDllGetActivationFactory(str, &factory2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }
    else
    {
        win_skip("Failed to load library, err %lu.\n", GetLastError());
    }

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK, "RoGetActivationFactory failed, hr %#lx\n", hr);

    if (hdll)
    {
        ok(factory == factory2, "Got unexpected factory %p, factory2 %p.\n", factory, factory2);
        IActivationFactory_Release(factory2);
        FreeLibrary(hdll);
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IInstalledVoicesStatic, (void **)&voices_static);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInstalledVoicesStatic failed, hr %#lx\n", hr);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IInstalledVoicesStatic_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IInstalledVoicesStatic_QueryInterface(voices_static, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IInstalledVoicesStatic_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IInstalledVoicesStatic_get_AllVoices(voices_static, &voices);
    ok(hr == S_OK, "IInstalledVoicesStatic_get_AllVoices failed, hr %#lx\n", hr);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#lx\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_VoiceInformation_QueryInterface voices returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_VoiceInformation_QueryInterface(voices, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == E_NOINTERFACE, "IVectorView_VoiceInformation_QueryInterface voices failed, hr %#lx\n", hr);

    size = 0xdeadbeef;
    hr = IVectorView_VoiceInformation_get_Size(voices, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_get_Size voices failed, hr %#lx\n", hr);
    todo_wine ok(size != 0 && size != 0xdeadbeef, "IVectorView_VoiceInformation_get_Size returned %u\n", size);

    voice = (IVoiceInformation *)0xdeadbeef;
    hr = IVectorView_VoiceInformation_GetAt(voices, size, &voice);
    ok(hr == E_BOUNDS, "IVectorView_VoiceInformation_GetAt failed, hr %#lx\n", hr);
    ok(voice == NULL, "IVectorView_VoiceInformation_GetAt returned %p\n", voice);

    hr = IVectorView_VoiceInformation_GetMany(voices, size, 1, &voice, &size);
    ok(hr == S_OK, "IVectorView_VoiceInformation_GetMany failed, hr %#lx\n", hr);
    ok(size == 0, "IVectorView_VoiceInformation_GetMany returned count %u\n", size);

    IVectorView_VoiceInformation_Release(voices);

    IInstalledVoicesStatic_Release(voices_static);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    hr = RoActivateInstance(str, &inspectable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechSynthesizer, (void **)&synthesizer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IInspectable_QueryInterface(inspectable, &IID_IClosable, (void **)&closable);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ref = IClosable_Release(closable);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = ISpeechSynthesizer_Release(synthesizer);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    ref = IInspectable_Release(inspectable);
    ok(!ref, "Got unexpected ref %lu.\n", ref);

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
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(voice_information_name, wcslen(voice_information_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == REGDB_E_CLASSNOTREG, "RoGetActivationFactory returned unexpected hr %#lx\n", hr);

    WindowsDeleteString(str);

    RoUninitialize();
}

static void test_SpeechRecognizer(void)
{
    static const WCHAR *speech_recognition_name = L"Windows.Media.SpeechRecognition.SpeechRecognizer";
    ISpeechRecognizerFactory *sr_factory = NULL;
    ISpeechRecognizerStatics *sr_statics = NULL;
    ISpeechRecognizerStatics2 *sr_statics2 = NULL;
    ISpeechRecognizer *recognizer = NULL;
    ISpeechRecognizer2 *recognizer2 = NULL;
    IActivationFactory *factory = NULL;
    IInspectable *inspectable = NULL;
    IClosable *closable = NULL;
    ILanguage *language = NULL;
    HSTRING hstr, hstr_lang;
    HRESULT hr;
    LONG ref;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx.\n", hr);

    hr = WindowsCreateString(speech_recognition_name, wcslen(speech_recognition_name), &hstr);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx.n", hr);

    hr = RoGetActivationFactory(hstr, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx.\n", hr);

    if(hr == REGDB_E_CLASSNOTREG) /* Win 8 and 8.1 */
    {
        win_skip("SpeechRecognizer activation factory not available!\n");
        goto done;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerFactory, (void **)&sr_factory);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_ISpeechRecognizer failed, hr %#lx.\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerStatics, (void **)&sr_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_ISpeechRecognizerStatics failed, hr %#lx.\n", hr);

    hr = ISpeechRecognizerStatics_get_SystemSpeechLanguage(sr_statics, &language);
    todo_wine ok(hr == S_OK, "ISpeechRecognizerStatics_SystemSpeechLanguage failed, hr %#lx.\n", hr);

    if(hr == S_OK)
    {
        hr = ILanguage_get_LanguageTag(language, &hstr_lang);
        ok(hr == S_OK, "ILanguage_get_LanguageTag failed, hr %#lx.\n", hr);

        trace("SpeechRecognizer default language %s.\n", debugstr_hstring(hstr_lang));

        ILanguage_Release(language);
    }

    ref = ISpeechRecognizerStatics_Release(sr_statics);
    ok(ref == 3, "Got unexpected ref %lu.\n", ref);

    hr = IActivationFactory_QueryInterface(factory, &IID_ISpeechRecognizerStatics2, (void **)&sr_statics2);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE), "IActivationFactory_QueryInterface IID_ISpeechRecognizerStatics2 failed, hr %#lx.\n", hr);

    if(hr == S_OK) /* SpeechRecognizerStatics2 not implemented on Win10 1507 */
    {
        ref = ISpeechRecognizerStatics2_Release(sr_statics2);
        ok(ref == 3, "Got unexpected ref %lu.\n", ref);
    }

    ref = ISpeechRecognizerFactory_Release(sr_factory);
    ok(ref == 2, "Got unexpected ref %lu.\n", ref);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected ref %lu.\n", ref);

    hr = RoActivateInstance(hstr, &inspectable);
    ok(hr == S_OK || broken(hr == 0x800455a0), "Got unexpected hr %#lx.\n", hr);

    if(hr == S_OK)
    {
        hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer, (void **)&recognizer);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IInspectable_QueryInterface(inspectable, &IID_ISpeechRecognizer2, (void **)&recognizer2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IInspectable_QueryInterface(inspectable, &IID_IClosable, (void **)&closable);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ref = IClosable_Release(closable);
        ok(ref == 3, "Got unexpected ref %lu.\n", ref);

        ref = ISpeechRecognizer2_Release(recognizer2);
        ok(ref == 2, "Got unexpected ref %lu.\n", ref);

        ref = ISpeechRecognizer_Release(recognizer);
        ok(ref == 1, "Got unexpected ref %lu.\n", ref);

        ref = IInspectable_Release(inspectable);
        ok(!ref, "Got unexpected ref %lu.\n", ref);
    }
    else if(hr == 0x800455a0) /* Not sure what this hr is... Probably if a language pack is not installed. */
    {
        win_skip("Could not init SpeechRecognizer with default language!\n");
    }

done:
    WindowsDeleteString(hstr);

    RoUninitialize();
}

START_TEST(speech)
{
    test_ActivationFactory();
    test_SpeechSynthesizer();
    test_VoiceInformation();
    test_SpeechRecognizer();
}
