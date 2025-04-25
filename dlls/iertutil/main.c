/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iertutil);

struct uri
{
    IUriRuntimeClass IUriRuntimeClass_iface;
    HSTRING raw_uri;
    LONG ref;
};

static inline struct uri *impl_from_IUriRuntimeClass(IUriRuntimeClass *iface)
{
    return CONTAINING_RECORD(iface, struct uri, IUriRuntimeClass_iface);
}

static HRESULT STDMETHODCALLTYPE uri_QueryInterface(IUriRuntimeClass *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IUriRuntimeClass))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE uri_AddRef(IUriRuntimeClass *iface)
{
    struct uri *impl = impl_from_IUriRuntimeClass(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE uri_Release(IUriRuntimeClass *iface)
{
    struct uri *impl = impl_from_IUriRuntimeClass(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        WindowsDeleteString(impl->raw_uri);
        free(impl);
    }

    return ref;
}

static HRESULT STDMETHODCALLTYPE uri_GetIids(IUriRuntimeClass *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_GetRuntimeClassName(IUriRuntimeClass *iface,
                                                         HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_GetTrustLevel(IUriRuntimeClass *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_AbsoluteUri(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p semi-stub!\n", iface, value);

    /* TODO: Parse the raw URI and reconstruct it from parts according to RFC 3986 or RFC 3987 */
    return IUriRuntimeClass_get_RawUri(iface, value);
}

static HRESULT STDMETHODCALLTYPE uri_DisplayUri(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Domain(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Extension(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Fragment(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Host(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Password(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Path(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Query(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_QueryParsed(IUriRuntimeClass *iface,
                                                 IWwwFormUrlDecoderRuntimeClass **decoder)
{
    FIXME("iface %p, decoder %p stub!\n", iface, decoder);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_RawUri(IUriRuntimeClass *iface, HSTRING *value)
{
    struct uri *impl = impl_from_IUriRuntimeClass(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return WindowsDuplicateString(impl->raw_uri, value);
}

static HRESULT STDMETHODCALLTYPE uri_SchemeName(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_UserName(IUriRuntimeClass *iface, HSTRING *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Port(IUriRuntimeClass *iface, INT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Suspicious(IUriRuntimeClass *iface, boolean *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_Equals(IUriRuntimeClass *iface, IUriRuntimeClass *uri,
                                            boolean *value)
{
    FIXME("iface %p, uri %p, value %p stub!\n", iface, uri, value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE uri_CombineUri(IUriRuntimeClass *iface, HSTRING relative_uri,
                                                IUriRuntimeClass **instance)
{
    FIXME("iface %p, relative_uri %s, instance %p stub!\n", iface, debugstr_hstring(relative_uri), instance);
    return E_NOTIMPL;
}

static const struct IUriRuntimeClassVtbl uri_vtbl =
{
    uri_QueryInterface,
    uri_AddRef,
    uri_Release,
    /* IInspectable methods */
    uri_GetIids,
    uri_GetRuntimeClassName,
    uri_GetTrustLevel,
    /* IUriRuntimeClass methods */
    uri_AbsoluteUri,
    uri_DisplayUri,
    uri_Domain,
    uri_Extension,
    uri_Fragment,
    uri_Host,
    uri_Password,
    uri_Path,
    uri_Query,
    uri_QueryParsed,
    uri_RawUri,
    uri_SchemeName,
    uri_UserName,
    uri_Port,
    uri_Suspicious,
    uri_Equals,
    uri_CombineUri,
};

struct iertutil
{
    IActivationFactory IActivationFactory_iface;
    IUriRuntimeClassFactory IUriRuntimeClassFactory_iface;
    LONG ref;
};

static inline struct iertutil *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct iertutil, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE iertutil_QueryInterface(IActivationFactory *iface, REFIID iid,
                                                         void **out)
{
    struct iertutil *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
        || IsEqualGUID(iid, &IID_IInspectable)
        || IsEqualGUID(iid, &IID_IAgileObject)
        || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IUriRuntimeClassFactory))
    {
        IUriRuntimeClassFactory_AddRef(&impl->IUriRuntimeClassFactory_iface);
        *out = &impl->IUriRuntimeClassFactory_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE iertutil_AddRef(IActivationFactory *iface)
{
    struct iertutil *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE iertutil_Release(IActivationFactory *iface)
{
    struct iertutil *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE iertutil_GetIids(IActivationFactory *iface, ULONG *iid_count,
                                                  IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE iertutil_GetRuntimeClassName(IActivationFactory *iface,
                                                              HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE iertutil_GetTrustLevel(IActivationFactory *iface,
                                                        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE iertutil_ActivateInstance(IActivationFactory *iface,
                                                           IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    iertutil_QueryInterface,
    iertutil_AddRef,
    iertutil_Release,
    /* IInspectable methods */
    iertutil_GetIids,
    iertutil_GetRuntimeClassName,
    iertutil_GetTrustLevel,
    /* IActivationFactory methods */
    iertutil_ActivateInstance,
};

DEFINE_IINSPECTABLE(uri_factory, IUriRuntimeClassFactory, struct iertutil, IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE uri_factory_CreateUri(IUriRuntimeClassFactory *iface, HSTRING uri,
                                                       IUriRuntimeClass **instance)
{
    const WCHAR *raw_buffer;
    struct uri *uri_impl;

    FIXME("iface %p, uri %s, instance %p semi-stub!\n", iface, debugstr_hstring(uri), instance);

    if (!uri)
        return E_POINTER;

    uri_impl = calloc(1, sizeof(*uri_impl));
    if (!uri_impl)
        return E_OUTOFMEMORY;

    uri_impl->IUriRuntimeClass_iface.lpVtbl = &uri_vtbl;
    uri_impl->ref = 1;

    raw_buffer = WindowsGetStringRawBuffer(uri, NULL);
    WindowsCreateString(raw_buffer, wcslen(raw_buffer), &uri_impl->raw_uri);

    /* TODO: Parse the URI according to RFC 3986 and RFC 3987 */

    *instance = &uri_impl->IUriRuntimeClass_iface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE uri_factory_CreateWithRelativeUri(IUriRuntimeClassFactory *iface,
                                                                   HSTRING base_uri,
                                                                   HSTRING relative_uri,
                                                                   IUriRuntimeClass **instance)
{
    FIXME("iface %p, base_uri %s, relative_uri %s, instance %p stub!\n", iface,
          debugstr_hstring(base_uri), debugstr_hstring(relative_uri), instance);
    return E_NOTIMPL;
}

static const struct IUriRuntimeClassFactoryVtbl uri_factory_vtbl =
{
    uri_factory_QueryInterface,
    uri_factory_AddRef,
    uri_factory_Release,
    /* IInspectable methods */
    uri_factory_GetIids,
    uri_factory_GetRuntimeClassName,
    uri_factory_GetTrustLevel,
    /* IUriRuntimeClassFactory methods */
    uri_factory_CreateUri,
    uri_factory_CreateWithRelativeUri,
};

static struct iertutil iertutil =
{
    {&activation_factory_vtbl},
    {&uri_factory_vtbl},
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
    *factory = &iertutil.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
