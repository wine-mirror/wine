/* WinRT Windows.Media.Speech implementation
 *
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_SpeechSynthesis
#include "windows.media.speechsynthesis.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct voice_information_vector
{
    IVectorView_VoiceInformation IVectorView_VoiceInformation_iface;
    LONG ref;
};

static inline struct voice_information_vector *impl_from_IVectorView_VoiceInformation(IVectorView_VoiceInformation *iface)
{
    return CONTAINING_RECORD(iface, struct voice_information_vector, IVectorView_VoiceInformation_iface);
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_QueryInterface(
        IVectorView_VoiceInformation *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IVectorView_VoiceInformation))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vector_view_voice_information_AddRef(
        IVectorView_VoiceInformation *iface)
{
    struct voice_information_vector *impl = impl_from_IVectorView_VoiceInformation(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE vector_view_voice_information_Release(
        IVectorView_VoiceInformation *iface)
{
    struct voice_information_vector *impl = impl_from_IVectorView_VoiceInformation(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetIids(
        IVectorView_VoiceInformation *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetRuntimeClassName(
        IVectorView_VoiceInformation *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetTrustLevel(
        IVectorView_VoiceInformation *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetAt(
        IVectorView_VoiceInformation *iface, UINT32 index, IVoiceInformation **value)
{
    FIXME("iface %p, index %#x, value %p stub!\n", iface, index, value);
    *value = NULL;
    return E_BOUNDS;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_get_Size(
        IVectorView_VoiceInformation *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_IndexOf(
        IVectorView_VoiceInformation *iface, IVoiceInformation *element, UINT32 *index, BOOLEAN *found)
{
    FIXME("iface %p, element %p, index %p, found %p stub!\n", iface, element, index, found);
    *index = 0;
    *found = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetMany(
        IVectorView_VoiceInformation *iface, UINT32 start_index,
        UINT32 items_size, IVoiceInformation **items, UINT *value)
{
    FIXME("iface %p, start_index %#x, items %p, value %p stub!\n", iface, start_index, items, value);
    *value = 0;
    return S_OK;
}

static const struct IVectorView_VoiceInformationVtbl vector_view_voice_information_vtbl =
{
    vector_view_voice_information_QueryInterface,
    vector_view_voice_information_AddRef,
    vector_view_voice_information_Release,
    /* IInspectable methods */
    vector_view_voice_information_GetIids,
    vector_view_voice_information_GetRuntimeClassName,
    vector_view_voice_information_GetTrustLevel,
    /* IVectorView<VoiceInformation> methods */
    vector_view_voice_information_GetAt,
    vector_view_voice_information_get_Size,
    vector_view_voice_information_IndexOf,
    vector_view_voice_information_GetMany,
};

static struct voice_information_vector all_voices =
{
    {&vector_view_voice_information_vtbl},
    0
};

struct windows_media_speech
{
    IActivationFactory IActivationFactory_iface;
    IInstalledVoicesStatic IInstalledVoicesStatic_iface;
    LONG ref;
};

static inline struct windows_media_speech *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_speech, IActivationFactory_iface);
}

static inline struct windows_media_speech *impl_from_IInstalledVoicesStatic(IInstalledVoicesStatic *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_speech, IInstalledVoicesStatic_iface);
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IInstalledVoicesStatic))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IInstalledVoicesStatic_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_media_speech_AddRef(
        IActivationFactory *iface)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_media_speech_Release(
        IActivationFactory *iface)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_media_speech_QueryInterface,
    windows_media_speech_AddRef,
    windows_media_speech_Release,
    /* IInspectable methods */
    windows_media_speech_GetIids,
    windows_media_speech_GetRuntimeClassName,
    windows_media_speech_GetTrustLevel,
    /* IActivationFactory methods */
    windows_media_speech_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE installed_voices_static_QueryInterface(
        IInstalledVoicesStatic *iface, REFIID iid, void **out)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_QueryInterface(&impl->IActivationFactory_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE installed_voices_static_AddRef(
        IInstalledVoicesStatic *iface)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_AddRef(&impl->IActivationFactory_iface);
}

static ULONG STDMETHODCALLTYPE installed_voices_static_Release(
        IInstalledVoicesStatic *iface)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_Release(&impl->IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetIids(
        IInstalledVoicesStatic *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetRuntimeClassName(
        IInstalledVoicesStatic *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetTrustLevel(
        IInstalledVoicesStatic *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_get_AllVoices(
    IInstalledVoicesStatic *iface, IVectorView_VoiceInformation **value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    *value = &all_voices.IVectorView_VoiceInformation_iface;
    IVectorView_VoiceInformation_AddRef(*value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_get_DefaultVoice(
    IInstalledVoicesStatic *iface, IVoiceInformation **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IInstalledVoicesStaticVtbl installed_voices_static_vtbl =
{
    installed_voices_static_QueryInterface,
    installed_voices_static_AddRef,
    installed_voices_static_Release,
    /* IInspectable methods */
    installed_voices_static_GetIids,
    installed_voices_static_GetRuntimeClassName,
    installed_voices_static_GetTrustLevel,
    /* IInstalledVoicesStatic methods */
    installed_voices_static_get_AllVoices,
    installed_voices_static_get_DefaultVoice,
};

static struct windows_media_speech windows_media_speech =
{
    {&activation_factory_vtbl},
    {&installed_voices_static_vtbl},
    1
};

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &windows_media_speech.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
