/*
 * Copyright 2024 Paul Gofman for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mf_private.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#include "initguid.h"
#include "devpkey.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

static HRESULT audio_capture_create_object(IMFAttributes *attributes, void *user_context, IUnknown **obj)
{
    FIXME("%p, %p, %p stub.\n", attributes, user_context, obj);

    return E_NOTIMPL;
}

static void audio_capture_shutdown_object(void *user_context, IUnknown *obj)
{
    TRACE("%p %p.\n", user_context, obj);
}

static void audio_capture_free_private(void *user_context)
{
    TRACE("%p.\n", user_context);
}

static const struct activate_funcs audio_capture_activate_funcs =
{
    audio_capture_create_object,
    audio_capture_shutdown_object,
    audio_capture_free_private,
};

HRESULT enum_audio_capture_sources(IMFAttributes *attributes, IMFActivate ***ret_sources, UINT32 *ret_count)
{
    IMMDeviceEnumerator *devenum = NULL;
    IMMDeviceCollection *devices = NULL;
    UINT32 i, count, created_count = 0;
    IMFActivate **sources = NULL;
    ERole role;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator,
            (void **)&devenum)))
        goto done;
    if (SUCCEEDED(IMFAttributes_GetUINT32(attributes, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ROLE, (UINT32 *)&role)))
        FIXME("Specifying role (%d) is not supported.\n", role);
    if (FAILED(hr = IMMDeviceEnumerator_EnumAudioEndpoints(devenum, eCapture, DEVICE_STATE_ACTIVE, &devices)))
        goto done;
    if (FAILED(hr = IMMDeviceCollection_GetCount(devices, &count)))
        goto done;
    sources = CoTaskMemAlloc(count * sizeof(*sources));
    for (i = 0; i < count; ++i)
    {
        IMMDevice *device = NULL;
        IPropertyStore *ps;
        WCHAR *str = NULL;
        PROPVARIANT pv;

        if (FAILED(hr = create_activation_object(NULL, &audio_capture_activate_funcs, &sources[i])))
            goto done;
        ++created_count;

        hr = IMFActivate_SetGUID(sources[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
        if (SUCCEEDED(hr))
            hr = IMMDeviceCollection_Item(devices, i, &device);
        if (SUCCEEDED(hr))
            hr = IMMDevice_GetId(device, &str);
        if (SUCCEEDED(hr))
            hr = IMFActivate_SetString(sources[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID, str);
        CoTaskMemFree(str);

        PropVariantInit(&pv);
        if (SUCCEEDED(hr) && SUCCEEDED((hr = IMMDevice_OpenPropertyStore(device, STGM_READ, &ps))))
        {
            hr = IPropertyStore_GetValue(ps, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv);
            IPropertyStore_Release(ps);
        }
        if (device)
            IMMDevice_Release(device);

        if (SUCCEEDED(hr) && pv.vt != VT_LPWSTR)
            hr = E_FAIL;

        if (SUCCEEDED(hr))
            hr = IMFActivate_SetString(sources[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, pv.pwszVal);
        PropVariantClear(&pv);
        if (FAILED(hr))
            goto done;
    }
    *ret_count = count;
    *ret_sources = sources;

done:
    if (FAILED(hr))
    {
        for (i = 0; i < created_count; ++i)
            IMFActivate_Release(sources[i]);
        CoTaskMemFree(sources);
    }
    if (devices)
        IMMDeviceCollection_Release(devices);
    if (devenum)
        IMMDeviceEnumerator_Release(devenum);
    TRACE("ret %#lx, *ret_count %u.\n", hr, *ret_count);
    return hr;
}
