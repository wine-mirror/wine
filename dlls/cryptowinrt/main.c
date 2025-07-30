/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "initguid.h"
#include "private.h"

#include <assert.h>

#include "wine/debug.h"
#include "objbase.h"

#include "bcrypt.h"
#include "wincrypt.h"

#define WIDL_using_Windows_Security_Cryptography
#include "windows.security.cryptography.h"
#include "robuffer.h"

WINE_DEFAULT_DEBUG_CHANNEL(crypto);

struct cryptobuffer_factory
{
    IActivationFactory IActivationFactory_iface;
    ICryptographicBufferStatics ICryptographicBufferStatics_iface;
    LONG refcount;
};

static inline struct cryptobuffer_factory *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct cryptobuffer_factory, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct cryptobuffer_factory *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ICryptographicBufferStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICryptographicBufferStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE cryptobuffer_factory_AddRef(IActivationFactory *iface)
{
    struct cryptobuffer_factory *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE cryptobuffer_factory_Release(IActivationFactory *iface)
{
    struct cryptobuffer_factory *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->refcount);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl cryptobuffer_factory_vtbl =
{
    cryptobuffer_factory_QueryInterface,
    cryptobuffer_factory_AddRef,
    cryptobuffer_factory_Release,
    /* IInspectable methods */
    cryptobuffer_factory_GetIids,
    cryptobuffer_factory_GetRuntimeClassName,
    cryptobuffer_factory_GetTrustLevel,
    /* IActivationFactory methods */
    cryptobuffer_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE(cryptobuffer_statics, ICryptographicBufferStatics, struct cryptobuffer_factory, IActivationFactory_iface);

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_Compare(
        ICryptographicBufferStatics *iface, IBuffer *object1, IBuffer *object2, boolean *is_equal)
{
    FIXME("iface %p, object1 %p, object2 %p, is_equal %p stub!\n", iface, object1, object2, is_equal);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_GenerateRandom(
        ICryptographicBufferStatics *iface, UINT32 length, IBuffer **buffer)
{
    FIXME("iface %p, length %u, buffer %p stub!\n", iface, length, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_GenerateRandomNumber(
        ICryptographicBufferStatics *iface, UINT32 *value)
{
    TRACE("iface %p, value %p.\n", iface, value);

    BCryptGenRandom(NULL, (UCHAR *)value, sizeof(*value), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_CreateFromByteArray(
        ICryptographicBufferStatics *iface, UINT32 value_size, BYTE *value, IBuffer **buffer)
{
    FIXME("iface %p, value_size %u, value %p, buffer %p stub!\n", iface, value_size, value, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_CopyToByteArray(
        ICryptographicBufferStatics *iface, IBuffer *buffer, UINT32 *value_size, BYTE **value)
{
    FIXME("iface %p, buffer %p, value_size %p, value %p stub!\n", iface, buffer, value_size, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_DecodeFromHexString(
        ICryptographicBufferStatics *iface, HSTRING value, IBuffer **buffer)
{
    FIXME("iface %p, value %p, buffer %p stub!\n", iface, value, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_EncodeToHexString(
        ICryptographicBufferStatics *iface, IBuffer *buffer, HSTRING *value)
{
    FIXME("iface %p, buffer %p, value %p stub!\n", iface, buffer, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_DecodeFromBase64String(
        ICryptographicBufferStatics *iface, HSTRING value, IBuffer **buffer)
{
    FIXME("iface %p, value %p, buffer %p stub!\n", iface, value, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_EncodeToBase64String(
        ICryptographicBufferStatics *iface, IBuffer *buffer, HSTRING *value)
{
    IBufferByteAccess *buffer_access;
    HSTRING_BUFFER str_buffer;
    void *data = NULL;
    UINT32 length = 0;
    DWORD ret_length;
    WCHAR *str;
    HRESULT hr;

    TRACE("iface %p, buffer %p, value %p.\n", iface, buffer, value);

    if (buffer)
    {
        IBuffer_get_Length(buffer, &length);
        if (length)
        {
            if (SUCCEEDED(IBuffer_QueryInterface(buffer, &IID_IBufferByteAccess, (void **)&buffer_access)))
            {
                IBufferByteAccess_Buffer(buffer_access, (byte **)&data);
                IBufferByteAccess_Release(buffer_access);
            }
        }
    }

    if (!length)
        return WindowsCreateString(NULL, 0, value);

    if (!data)
        return E_FAIL;

    if (!CryptBinaryToStringW(data, length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &ret_length))
        return E_FAIL;

    if (FAILED(hr = WindowsPreallocateStringBuffer(ret_length, &str, &str_buffer)))
        return hr;

    if (!CryptBinaryToStringW(data, length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, str, &ret_length))
    {
        WindowsDeleteStringBuffer(str_buffer);
        return E_FAIL;
    }

    return WindowsPromoteStringBuffer(str_buffer, value);
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_ConvertStringToBinary(
        ICryptographicBufferStatics *iface, HSTRING value, BinaryStringEncoding encoding,
        IBuffer **buffer)
{
    FIXME("iface %p, value %p, encoding %d, buffer %p stub!\n", iface, value, encoding, buffer);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cryptobuffer_statics_ConvertBinaryToString(
        ICryptographicBufferStatics *iface, BinaryStringEncoding encoding, IBuffer *buffer, HSTRING *value)
{
    FIXME("iface %p, encoding %d, buffer %p, value %p stub!\n", iface, encoding, buffer, value);

    return E_NOTIMPL;
}

static const struct ICryptographicBufferStaticsVtbl cryptobuffer_statics_vtbl =
{
    cryptobuffer_statics_QueryInterface,
    cryptobuffer_statics_AddRef,
    cryptobuffer_statics_Release,
    /* IInspectable methods */
    cryptobuffer_statics_GetIids,
    cryptobuffer_statics_GetRuntimeClassName,
    cryptobuffer_statics_GetTrustLevel,
    /* ICryptographicBufferStatics methods */
    cryptobuffer_statics_Compare,
    cryptobuffer_statics_GenerateRandom,
    cryptobuffer_statics_GenerateRandomNumber,
    cryptobuffer_statics_CreateFromByteArray,
    cryptobuffer_statics_CopyToByteArray,
    cryptobuffer_statics_DecodeFromHexString,
    cryptobuffer_statics_EncodeToHexString,
    cryptobuffer_statics_DecodeFromBase64String,
    cryptobuffer_statics_EncodeToBase64String,
    cryptobuffer_statics_ConvertStringToBinary,
    cryptobuffer_statics_ConvertBinaryToString,
};

static struct cryptobuffer_factory cryptobuffer_factory =
{
    .IActivationFactory_iface.lpVtbl = &cryptobuffer_factory_vtbl,
    .ICryptographicBufferStatics_iface.lpVtbl = &cryptobuffer_statics_vtbl,
    .refcount = 1,
};

IActivationFactory *cryptobuffer_activation_factory = &cryptobuffer_factory.IActivationFactory_iface;

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    const WCHAR *name = WindowsGetStringRawBuffer(classid, NULL);

    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);

    *factory = NULL;

    if (!wcscmp(name, RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer))
        IActivationFactory_QueryInterface(cryptobuffer_activation_factory, &IID_IActivationFactory, (void **)factory);
    if (!wcscmp(name, RuntimeClass_Windows_Security_Credentials_KeyCredentialManager))
        IActivationFactory_QueryInterface(credentials_activation_factory, &IID_IActivationFactory, (void **)factory);

    if (*factory) return S_OK;
    return CLASS_E_CLASSNOTAVAILABLE;
}
