/*
 * Copyright 2021 Andrew Eikum for CodeWeavers
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
#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "activation.h"
#include "mmdeviceapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_Devices
#include "windows.media.devices.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

static ERole AudioDeviceRole_to_ERole(AudioDeviceRole role)
{
    switch(role){
        case AudioDeviceRole_Communications:
            return eCommunications;
        default:
            return eMultimedia;
    }
}

struct windows_media_devices
{
    IActivationFactory IActivationFactory_iface;
    IMediaDeviceStatics IMediaDeviceStatics_iface;
    LONG ref;
};

static inline struct windows_media_devices *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_devices, IActivationFactory_iface);
}

static inline struct windows_media_devices *impl_from_IMediaDeviceStatics(IMediaDeviceStatics *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_devices, IMediaDeviceStatics_iface);
}

static HRESULT STDMETHODCALLTYPE windows_media_devices_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_media_devices *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
            IsEqualGUID(iid, &IID_IInspectable) ||
            IsEqualGUID(iid, &IID_IAgileObject) ||
            IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IMediaDeviceStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IMediaDeviceStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_media_devices_AddRef(
        IActivationFactory *iface)
{
    struct windows_media_devices *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_media_devices_Release(
        IActivationFactory *iface)
{
    struct windows_media_devices *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_media_devices_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_devices_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_devices_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_devices_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_media_devices_QueryInterface,
    windows_media_devices_AddRef,
    windows_media_devices_Release,
    /* IInspectable methods */
    windows_media_devices_GetIids,
    windows_media_devices_GetRuntimeClassName,
    windows_media_devices_GetTrustLevel,
    /* IActivationFactory methods */
    windows_media_devices_ActivateInstance,
};

static HRESULT get_default_device_id(EDataFlow direction, AudioDeviceRole role, HSTRING *device_id_hstr)
{
    HRESULT hr;
    WCHAR *devid, *s;
    IMMDevice *dev;
    IMMDeviceEnumerator *devenum;
    ERole mmdev_role = AudioDeviceRole_to_ERole(role);

    static const WCHAR id_fmt_pre[] = L"\\\\?\\SWD#MMDEVAPI#";
    static const WCHAR id_fmt_hash[] = L"#";
    static const size_t GUID_STR_LEN = 38; /* == strlen("{00000000-0000-0000-0000-000000000000}") */

    *device_id_hstr = NULL;

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void**)&devenum);
    if (FAILED(hr))
    {
        WARN("Failed to create MMDeviceEnumerator: %08lx\n", hr);
        return hr;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(devenum, direction, mmdev_role, &dev);
    if (FAILED(hr))
    {
        WARN("GetDefaultAudioEndpoint failed: %08lx\n", hr);
        IMMDeviceEnumerator_Release(devenum);
        return hr;
    }

    hr = IMMDevice_GetId(dev, &devid);
    if (FAILED(hr))
    {
        WARN("GetId failed: %08lx\n", hr);
        IMMDevice_Release(dev);
        IMMDeviceEnumerator_Release(devenum);
        return hr;
    }

    s = malloc((sizeof(id_fmt_pre) - sizeof(WCHAR)) +
            (sizeof(id_fmt_hash) - sizeof(WCHAR)) +
            (wcslen(devid) + GUID_STR_LEN + 1 /* nul */) * sizeof(WCHAR));

    wcscpy(s, id_fmt_pre);
    wcscat(s, devid);
    wcscat(s, id_fmt_hash);

    if(direction == eRender)
        StringFromGUID2(&DEVINTERFACE_AUDIO_RENDER, s + wcslen(s), GUID_STR_LEN + 1);
    else
        StringFromGUID2(&DEVINTERFACE_AUDIO_CAPTURE, s + wcslen(s), GUID_STR_LEN + 1);

    hr = WindowsCreateString(s, wcslen(s), device_id_hstr);
    if (FAILED(hr))
        WARN("WindowsCreateString failed: %08lx\n", hr);

    free(s);

    CoTaskMemFree(devid);
    IMMDevice_Release(dev);
    IMMDeviceEnumerator_Release(devenum);

    return hr;
}

static HRESULT WINAPI media_device_statics_QueryInterface(IMediaDeviceStatics *iface,
        REFIID riid, void **ppvObject)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_QueryInterface(&This->IActivationFactory_iface, riid, ppvObject);
}

static ULONG WINAPI media_device_statics_AddRef(IMediaDeviceStatics *iface)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_AddRef(&This->IActivationFactory_iface);
}

static ULONG WINAPI media_device_statics_Release(IMediaDeviceStatics *iface)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_Release(&This->IActivationFactory_iface);
}

static HRESULT WINAPI media_device_statics_GetIids(IMediaDeviceStatics *iface,
        ULONG *iidCount, IID **iids)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_GetIids(&This->IActivationFactory_iface, iidCount, iids);
}

static HRESULT WINAPI media_device_statics_GetRuntimeClassName(IMediaDeviceStatics *iface,
        HSTRING *className)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_GetRuntimeClassName(&This->IActivationFactory_iface, className);
}

static HRESULT WINAPI media_device_statics_GetTrustLevel(IMediaDeviceStatics *iface,
        TrustLevel *trustLevel)
{
    struct windows_media_devices *This = impl_from_IMediaDeviceStatics(iface);
    return windows_media_devices_GetTrustLevel(&This->IActivationFactory_iface, trustLevel);
}

static HRESULT WINAPI media_device_statics_GetAudioCaptureSelector(IMediaDeviceStatics *iface,
        HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_device_statics_GetAudioRenderSelector(IMediaDeviceStatics *iface,
        HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_device_statics_GetVideoCaptureSelector(IMediaDeviceStatics *iface,
        HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI media_device_statics_GetDefaultAudioCaptureId(IMediaDeviceStatics *iface,
        AudioDeviceRole role, HSTRING *value)
{
    TRACE("iface %p, role 0x%x, value %p\n", iface, role, value);
    return get_default_device_id(eCapture, role, value);
}

static HRESULT WINAPI media_device_statics_GetDefaultAudioRenderId(IMediaDeviceStatics *iface,
        AudioDeviceRole role, HSTRING *value)
{
    TRACE("iface %p, role 0x%x, value %p\n", iface, role, value);
    return get_default_device_id(eRender, role, value);
}

static HRESULT WINAPI media_device_statics_add_DefaultAudioCaptureDeviceChanged(
        IMediaDeviceStatics *iface,
        ITypedEventHandler_IInspectable_DefaultAudioCaptureDeviceChangedEventArgs *handler,
        EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p token %p stub!\n", iface, handler, token);
    if(!token)
        return E_POINTER;
    token->value = 1;
    return S_OK;
}

static HRESULT WINAPI media_device_statics_remove_DefaultAudioCaptureDeviceChanged(
        IMediaDeviceStatics *iface,
        EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static HRESULT WINAPI media_device_statics_add_DefaultAudioRenderDeviceChanged(
        IMediaDeviceStatics *iface,
        ITypedEventHandler_IInspectable_DefaultAudioRenderDeviceChangedEventArgs *handler,
        EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p token %p stub!\n", iface, handler, token);
    if(!token)
        return E_POINTER;
    token->value = 1;
    return S_OK;
}

static HRESULT WINAPI media_device_statics_remove_DefaultAudioRenderDeviceChanged(
        IMediaDeviceStatics *iface,
        EventRegistrationToken token)
{
    FIXME("iface %p, token %#I64x stub!\n", iface, token.value);
    return S_OK;
}

static IMediaDeviceStaticsVtbl media_device_statics_vtbl = {
    media_device_statics_QueryInterface,
    media_device_statics_AddRef,
    media_device_statics_Release,
    media_device_statics_GetIids,
    media_device_statics_GetRuntimeClassName,
    media_device_statics_GetTrustLevel,
    media_device_statics_GetAudioCaptureSelector,
    media_device_statics_GetAudioRenderSelector,
    media_device_statics_GetVideoCaptureSelector,
    media_device_statics_GetDefaultAudioCaptureId,
    media_device_statics_GetDefaultAudioRenderId,
    media_device_statics_add_DefaultAudioCaptureDeviceChanged,
    media_device_statics_remove_DefaultAudioCaptureDeviceChanged,
    media_device_statics_add_DefaultAudioRenderDeviceChanged,
    media_device_statics_remove_DefaultAudioRenderDeviceChanged,
};

static struct windows_media_devices windows_media_devices =
{
    {&activation_factory_vtbl},
    {&media_device_statics_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);
    *factory = &windows_media_devices.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
