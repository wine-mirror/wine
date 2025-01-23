/*
 * Copyright 2022-2024 Zhiyi Zhang for CodeWeavers
 * Copyright 2025 Jactry Zeng for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(wintypes);

struct data_writer
{
    IActivationFactory IActivationFactory_iface;
    LONG ref;
};

static inline struct data_writer *impl_data_writer_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct data_writer, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_QueryInterface(IActivationFactory *iface, REFIID iid,
        void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE data_writer_activation_factory_AddRef(IActivationFactory *iface)
{
    struct data_writer *impl = impl_data_writer_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE data_writer_activation_factory_Release(IActivationFactory *iface)
{
    struct data_writer *impl = impl_data_writer_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetIids(IActivationFactory *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetRuntimeClassName(IActivationFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_GetTrustLevel(IActivationFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE data_writer_activation_factory_ActivateInstance(IActivationFactory *iface,
        IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return S_OK;
}

static const struct IActivationFactoryVtbl data_writer_activation_factory_vtbl =
{
    data_writer_activation_factory_QueryInterface,
    data_writer_activation_factory_AddRef,
    data_writer_activation_factory_Release,
    /* IInspectable methods */
    data_writer_activation_factory_GetIids,
    data_writer_activation_factory_GetRuntimeClassName,
    data_writer_activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    data_writer_activation_factory_ActivateInstance,
};

struct data_writer data_writer =
{
    {&data_writer_activation_factory_vtbl},
    1
};

IActivationFactory *data_writer_activation_factory = &data_writer.IActivationFactory_iface;

struct stream_reference_statics
{
    IActivationFactory IActivationFactory_iface;
    IRandomAccessStreamReferenceStatics IRandomAccessStreamReferenceStatics_iface;
    LONG ref;
};

static inline struct stream_reference_statics *impl_stream_reference_statics_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct stream_reference_statics, IActivationFactory_iface);
}

static inline struct stream_reference_statics *impl_stream_reference_statics_from_IRandomAccessStreamReferenceStatics(IRandomAccessStreamReferenceStatics *iface)
{
    return CONTAINING_RECORD(iface, struct stream_reference_statics, IRandomAccessStreamReferenceStatics_iface);
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_activation_factory_QueryInterface(IActivationFactory *iface,
        REFIID iid, void **out)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IInspectable)
            || IsEqualGUID(iid, &IID_IAgileObject)
            || IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IRandomAccessStreamReferenceStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IRandomAccessStreamReferenceStatics_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE stream_reference_statics_activation_factory_AddRef(IActivationFactory *iface)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE stream_reference_statics_activation_factory_Release(IActivationFactory *iface)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_activation_factory_GetIids(IActivationFactory *iface,
        ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_activation_factory_GetRuntimeClassName(IActivationFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_activation_factory_GetTrustLevel(IActivationFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_activation_factory_ActivateInstance(IActivationFactory *iface,
        IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return S_OK;
}

static const struct IActivationFactoryVtbl stream_reference_statics_activation_factory_vtbl =
{
    stream_reference_statics_activation_factory_QueryInterface,
    stream_reference_statics_activation_factory_AddRef,
    stream_reference_statics_activation_factory_Release,
    /* IInspectable methods */
    stream_reference_statics_activation_factory_GetIids,
    stream_reference_statics_activation_factory_GetRuntimeClassName,
    stream_reference_statics_activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    stream_reference_statics_activation_factory_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_QueryInterface(IRandomAccessStreamReferenceStatics *iface,
        REFIID iid, void **out)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IRandomAccessStreamReferenceStatics(iface);
    return IActivationFactory_QueryInterface(&impl->IActivationFactory_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE stream_reference_statics_statics_AddRef(IRandomAccessStreamReferenceStatics *iface)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IRandomAccessStreamReferenceStatics(iface);
    return IActivationFactory_AddRef(&impl->IActivationFactory_iface);
}

static ULONG STDMETHODCALLTYPE stream_reference_statics_statics_Release(IRandomAccessStreamReferenceStatics *iface)
{
    struct stream_reference_statics *impl = impl_stream_reference_statics_from_IRandomAccessStreamReferenceStatics(iface);
    return IActivationFactory_Release(&impl->IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_GetIids(IRandomAccessStreamReferenceStatics *iface,
        ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_GetRuntimeClassName(IRandomAccessStreamReferenceStatics *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_GetTrustLevel(IRandomAccessStreamReferenceStatics *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_CreateFromFile(IRandomAccessStreamReferenceStatics *iface,
        IStorageFile *file, IRandomAccessStreamReference **stream_reference_statics)
{
    FIXME("iface %p, file %p, reference %p stub!\n", iface, file, stream_reference_statics);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_CreateFromUri(IRandomAccessStreamReferenceStatics *iface,
        IUriRuntimeClass *uri, IRandomAccessStreamReference **stream_reference_statics)
{
    FIXME("iface %p, uri %p, reference %p stub!\n", iface, uri, stream_reference_statics);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE stream_reference_statics_statics_CreateFromStream(IRandomAccessStreamReferenceStatics *iface,
        IRandomAccessStream *stream,
        IRandomAccessStreamReference **stream_reference_statics)
{
    FIXME("iface %p, stream %p, reference %p stub!\n", iface, stream, stream_reference_statics);
    return E_NOTIMPL;
}

static const struct IRandomAccessStreamReferenceStaticsVtbl stream_reference_statics_statics_vtbl =
{
    stream_reference_statics_statics_QueryInterface,
    stream_reference_statics_statics_AddRef,
    stream_reference_statics_statics_Release,
    /* IInspectable methods */
    stream_reference_statics_statics_GetIids,
    stream_reference_statics_statics_GetRuntimeClassName,
    stream_reference_statics_statics_GetTrustLevel,
    /* IRandomAccessStreamReferenceStatics */
    stream_reference_statics_statics_CreateFromFile,
    stream_reference_statics_statics_CreateFromUri,
    stream_reference_statics_statics_CreateFromStream
};

struct stream_reference_statics stream_reference_statics =
{
    {&stream_reference_statics_activation_factory_vtbl},
    {&stream_reference_statics_statics_vtbl},
    1
};

IActivationFactory *stream_reference_statics_activation_factory = &stream_reference_statics.IActivationFactory_iface;
