/*
 * Copyright 2022-2024 Zhiyi Zhang for CodeWeavers
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
#include "rometadataresolution.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Metadata
#include "windows.foundation.metadata.h"
#include "wintypes_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintypes);

static const struct
{
    const WCHAR *name;
    unsigned int max_major;
}
present_contracts[] =
{
    { L"Windows.Foundation.UniversalApiContract", 10, },
};

static BOOLEAN is_api_contract_present( const HSTRING hname, unsigned int version )
{
    const WCHAR *name = WindowsGetStringRawBuffer( hname, NULL );
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(present_contracts); ++i)
        if (!wcsicmp( name, present_contracts[i].name )) return version <= present_contracts[i].max_major;

    return FALSE;
}

struct wintypes
{
    IActivationFactory IActivationFactory_iface;
    IApiInformationStatics IApiInformationStatics_iface;
    IPropertyValueStatics IPropertyValueStatics_iface;
    LONG ref;
};

static inline struct wintypes *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct wintypes, IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE wintypes_QueryInterface(IActivationFactory *iface, REFIID iid,
        void **out)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);

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

    if (IsEqualGUID(iid, &IID_IApiInformationStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IApiInformationStatics_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IPropertyValueStatics))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IPropertyValueStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE wintypes_AddRef(IActivationFactory *iface)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE wintypes_Release(IActivationFactory *iface)
{
    struct wintypes *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetIids(IActivationFactory *iface, ULONG *iid_count,
        IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetRuntimeClassName(IActivationFactory *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_GetTrustLevel(IActivationFactory *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE wintypes_ActivateInstance(IActivationFactory *iface,
        IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    wintypes_QueryInterface,
    wintypes_AddRef,
    wintypes_Release,
    /* IInspectable methods */
    wintypes_GetIids,
    wintypes_GetRuntimeClassName,
    wintypes_GetTrustLevel,
    /* IActivationFactory methods */
    wintypes_ActivateInstance,
};

DEFINE_IINSPECTABLE(api_information_statics, IApiInformationStatics, struct wintypes, IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE api_information_statics_IsTypePresent(
        IApiInformationStatics *iface, HSTRING type_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, value %p stub!\n", iface, debugstr_hstring(type_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsMethodPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING method_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, method_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(method_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsMethodPresentWithArity(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING method_name,
        UINT32 input_parameter_count, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, method_name %s, input_parameter_count %u, value %p stub!\n",
            iface, debugstr_hstring(type_name), debugstr_hstring(method_name),
            input_parameter_count, value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsEventPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING event_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, event_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(event_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsPropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsReadOnlyPropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name,
        BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsWriteablePropertyPresent(
        IApiInformationStatics *iface, HSTRING type_name, HSTRING property_name, BOOLEAN *value)
{
    FIXME("iface %p, type_name %s, property_name %s, value %p stub!\n", iface,
            debugstr_hstring(type_name), debugstr_hstring(property_name), value);

    if (!type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsEnumNamedValuePresent(
        IApiInformationStatics *iface, HSTRING enum_type_name, HSTRING value_name, BOOLEAN *value)
{
    FIXME("iface %p, enum_type_name %s, value_name %s, value %p stub!\n", iface,
            debugstr_hstring(enum_type_name), debugstr_hstring(value_name), value);

    if (!enum_type_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsApiContractPresentByMajor(
        IApiInformationStatics *iface, HSTRING contract_name, UINT16 major_version, BOOLEAN *value)
{
    FIXME("iface %p, contract_name %s, major_version %u, value %p semi-stub.\n", iface,
            debugstr_hstring(contract_name), major_version, value);

    if (!contract_name)
        return E_INVALIDARG;

    *value = is_api_contract_present( contract_name, major_version );
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE api_information_statics_IsApiContractPresentByMajorAndMinor(
        IApiInformationStatics *iface, HSTRING contract_name, UINT16 major_version,
        UINT16 minor_version, BOOLEAN *value)
{
    FIXME("iface %p, contract_name %s, major_version %u, minor_version %u, value %p stub!\n", iface,
            debugstr_hstring(contract_name), major_version, minor_version, value);

    if (!contract_name)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static const struct IApiInformationStaticsVtbl api_information_statics_vtbl =
{
    api_information_statics_QueryInterface,
    api_information_statics_AddRef,
    api_information_statics_Release,
    /* IInspectable methods */
    api_information_statics_GetIids,
    api_information_statics_GetRuntimeClassName,
    api_information_statics_GetTrustLevel,
    /* IApiInformationStatics methods */
    api_information_statics_IsTypePresent,
    api_information_statics_IsMethodPresent,
    api_information_statics_IsMethodPresentWithArity,
    api_information_statics_IsEventPresent,
    api_information_statics_IsPropertyPresent,
    api_information_statics_IsReadOnlyPropertyPresent,
    api_information_statics_IsWriteablePropertyPresent,
    api_information_statics_IsEnumNamedValuePresent,
    api_information_statics_IsApiContractPresentByMajor,
    api_information_statics_IsApiContractPresentByMajorAndMinor
};

DEFINE_IINSPECTABLE(property_value_statics, IPropertyValueStatics, struct wintypes, IActivationFactory_iface)

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateEmpty(IPropertyValueStatics *iface,
        IInspectable **property_value)
{
    FIXME("iface %p, property_value %p stub!\n", iface, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt8(IPropertyValueStatics *iface,
        BYTE value, IInspectable **property_value)
{
    FIXME("iface %p, value %#x, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt16(IPropertyValueStatics *iface,
        INT16 value, IInspectable **property_value)
{
    FIXME("iface %p, value %d, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt16(IPropertyValueStatics *iface,
        UINT16 value, IInspectable **property_value)
{
    FIXME("iface %p, value %u, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt32(IPropertyValueStatics *iface,
        INT32 value, IInspectable **property_value)
{
    FIXME("iface %p, value %d, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt32(IPropertyValueStatics *iface,
        UINT32 value, IInspectable **property_value)
{
    FIXME("iface %p, value %u, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt64(IPropertyValueStatics *iface,
        INT64 value, IInspectable **property_value)
{
    FIXME("iface %p, value %I64d, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt64(IPropertyValueStatics *iface,
        UINT64 value, IInspectable **property_value)
{
    FIXME("iface %p, value %I64u, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSingle(IPropertyValueStatics *iface,
        FLOAT value, IInspectable **property_value)
{
    FIXME("iface %p, value %f, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDouble(IPropertyValueStatics *iface,
        DOUBLE value, IInspectable **property_value)
{
    FIXME("iface %p, value %f, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateChar16(IPropertyValueStatics *iface,
        WCHAR value, IInspectable **property_value)
{
    FIXME("iface %p, value %s, property_value %p stub!\n", iface, wine_dbgstr_wn(&value, 1), property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateBoolean(IPropertyValueStatics *iface,
        boolean value, IInspectable **property_value)
{
    FIXME("iface %p, value %d, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateString(IPropertyValueStatics *iface,
        HSTRING value, IInspectable **property_value)
{
    FIXME("iface %p, value %s, property_value %p stub!\n", iface, debugstr_hstring(value), property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInspectable(IPropertyValueStatics *iface,
        IInspectable *value, IInspectable **property_value)
{
    FIXME("iface %p, value %p, property_value %p stub!\n", iface, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateGuid(IPropertyValueStatics *iface,
        GUID value, IInspectable **property_value)
{
    FIXME("iface %p, value %s, property_value %p stub!\n", iface, debugstr_guid(&value), property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDateTime(IPropertyValueStatics *iface,
        DateTime value, IInspectable **property_value)
{
    FIXME("iface %p, value %I64d, property_value %p stub!\n", iface, value.UniversalTime, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateTimeSpan(IPropertyValueStatics *iface,
        TimeSpan value, IInspectable **property_value)
{
    FIXME("iface %p, value %I64d, property_value %p stub!\n", iface, value.Duration, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreatePoint(IPropertyValueStatics *iface,
        Point value, IInspectable **property_value)
{
    FIXME("iface %p, value (%f, %f), property_value %p stub!\n", iface, value.X, value.Y, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSize(IPropertyValueStatics *iface,
        Size value, IInspectable **property_value)
{
    FIXME("iface %p, value (%fx%f), property_value %p stub!\n", iface, value.Width, value.Height, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateRect(IPropertyValueStatics *iface,
        Rect value, IInspectable **property_value)
{
    FIXME("iface %p, value (%f, %f %fx%f), property_value %p stub!\n", iface, value.X, value.Y, value.Width,
            value.Height, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt8Array(IPropertyValueStatics *iface,
        UINT32 value_size, BYTE *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt16Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT16 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt16Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT16 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt32Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT32 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt32Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT32 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInt64Array(IPropertyValueStatics *iface,
        UINT32 value_size, INT64 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateUInt64Array(IPropertyValueStatics *iface,
        UINT32 value_size, UINT64 *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSingleArray(IPropertyValueStatics *iface,
        UINT32 value_size, FLOAT *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDoubleArray(IPropertyValueStatics *iface,
        UINT32 value_size, DOUBLE *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateChar16Array(IPropertyValueStatics *iface,
        UINT32 value_size, WCHAR *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateBooleanArray(IPropertyValueStatics *iface,
        UINT32 value_size, boolean *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateStringArray(IPropertyValueStatics *iface,
        UINT32 value_size, HSTRING *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateInspectableArray(IPropertyValueStatics *iface,
        UINT32 value_size, IInspectable **value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateGuidArray(IPropertyValueStatics *iface,
        UINT32 value_size, GUID *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateDateTimeArray(IPropertyValueStatics *iface,
        UINT32 value_size, DateTime *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateTimeSpanArray(IPropertyValueStatics *iface,
        UINT32 value_size, TimeSpan *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreatePointArray(IPropertyValueStatics *iface,
        UINT32 value_size, Point *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateSizeArray(IPropertyValueStatics *iface,
        UINT32 value_size, Size *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE property_value_statics_CreateRectArray(IPropertyValueStatics *iface,
        UINT32 value_size, Rect *value, IInspectable **property_value)
{
    FIXME("iface %p, value_size %u, value %p, property_value %p stub!\n", iface, value_size, value, property_value);
    return E_NOTIMPL;
}

static const struct IPropertyValueStaticsVtbl property_value_statics_vtbl =
{
    property_value_statics_QueryInterface,
    property_value_statics_AddRef,
    property_value_statics_Release,
    /* IInspectable methods */
    property_value_statics_GetIids,
    property_value_statics_GetRuntimeClassName,
    property_value_statics_GetTrustLevel,
    /* IPropertyValueStatics methods */
    property_value_statics_CreateEmpty,
    property_value_statics_CreateUInt8,
    property_value_statics_CreateInt16,
    property_value_statics_CreateUInt16,
    property_value_statics_CreateInt32,
    property_value_statics_CreateUInt32,
    property_value_statics_CreateInt64,
    property_value_statics_CreateUInt64,
    property_value_statics_CreateSingle,
    property_value_statics_CreateDouble,
    property_value_statics_CreateChar16,
    property_value_statics_CreateBoolean,
    property_value_statics_CreateString,
    property_value_statics_CreateInspectable,
    property_value_statics_CreateGuid,
    property_value_statics_CreateDateTime,
    property_value_statics_CreateTimeSpan,
    property_value_statics_CreatePoint,
    property_value_statics_CreateSize,
    property_value_statics_CreateRect,
    property_value_statics_CreateUInt8Array,
    property_value_statics_CreateInt16Array,
    property_value_statics_CreateUInt16Array,
    property_value_statics_CreateInt32Array,
    property_value_statics_CreateUInt32Array,
    property_value_statics_CreateInt64Array,
    property_value_statics_CreateUInt64Array,
    property_value_statics_CreateSingleArray,
    property_value_statics_CreateDoubleArray,
    property_value_statics_CreateChar16Array,
    property_value_statics_CreateBooleanArray,
    property_value_statics_CreateStringArray,
    property_value_statics_CreateInspectableArray,
    property_value_statics_CreateGuidArray,
    property_value_statics_CreateDateTimeArray,
    property_value_statics_CreateTimeSpanArray,
    property_value_statics_CreatePointArray,
    property_value_statics_CreateSizeArray,
    property_value_statics_CreateRectArray,
};

static struct wintypes wintypes =
{
    {&activation_factory_vtbl},
    {&api_information_statics_vtbl},
    {&property_value_statics_vtbl},
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
    *factory = &wintypes.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}

HRESULT WINAPI RoIsApiContractMajorVersionPresent(const WCHAR *name, UINT16 major, BOOL *result)
{
    FIXME("name %s, major %u, result %p\n", debugstr_w(name), major, result);
    *result = FALSE;
    return S_OK;
}

HRESULT WINAPI RoResolveNamespace(HSTRING name, HSTRING windowsMetaDataDir,
                                  DWORD packageGraphDirsCount, const HSTRING *packageGraphDirs,
                                  DWORD *metaDataFilePathsCount, HSTRING **metaDataFilePaths,
                                  DWORD *subNamespacesCount, HSTRING **subNamespaces)
{
    FIXME("name %s, windowsMetaDataDir %s, metaDataFilePaths %p, subNamespaces %p stub!\n",
            debugstr_hstring(name), debugstr_hstring(windowsMetaDataDir),
            metaDataFilePaths, subNamespaces);

    if (!metaDataFilePaths && !subNamespaces)
        return E_INVALIDARG;

    return RO_E_METADATA_NAME_NOT_FOUND;
}
