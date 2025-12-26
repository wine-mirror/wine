/*
 * Copyright 2025 Piotr Caban
 * Copyright 2025 Vibhav Pant
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

#include "initguid.h"
#include "roapi.h"
#include "winstring.h"
#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"
#include "wine/asm.h"
#include "wine/debug.h"

#include "cxx.h"
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

CREATE_TYPE_INFO_VTABLE

HRESULT __cdecl InitializeData(int type)
{
    HRESULT hres;

    FIXME("(%d) semi-stub\n", type);

    if (!type) return S_OK;

    hres = RoInitialize(type == 1 ? RO_INIT_SINGLETHREADED : RO_INIT_MULTITHREADED);
    if (FAILED(hres)) return hres;
    return S_OK;
}

void __cdecl UninitializeData(int type)
{
    TRACE("(%d)\n", type);

    if (type) RoUninitialize();
}

HRESULT WINAPI GetActivationFactoryByPCWSTR(const WCHAR *name, const GUID *iid, void **out)
{
    HSTRING_HEADER hdr;
    HSTRING str;
    HRESULT hr;

    TRACE("(%s, %s, %p)\n", debugstr_w(name), debugstr_guid(iid), out);

    /* Use a fast-pass string to avoid an allocation. */
    hr = WindowsCreateStringReference(name, wcslen(name), &hdr, &str);
    if (FAILED(hr)) return hr;

    return RoGetActivationFactory(str, iid, out);
}

HRESULT WINAPI GetIidsFn(unsigned int count, unsigned int *copied, const GUID *src, GUID **dest)
{
    TRACE("(%u, %p, %p, %p)\n", count, copied, src, dest);

    if (!(*dest = CoTaskMemAlloc(count * sizeof(*src))))
    {
        *copied = 0;
        return E_OUTOFMEMORY;
    }

    *copied = count;
    memcpy(*dest, src, count * sizeof(*src));

    return S_OK;
}

static void *try_Allocate(size_t size)
{
    return malloc(size);
}

void *__cdecl Allocate(size_t size)
{
    void *addr;

    TRACE("(%Iu)\n", size);

    if (!(addr = try_Allocate(size)))
        __abi_WinRTraiseOutOfMemoryException();
    return addr;
}

static void *try_AllocateException(size_t size)
{
    struct exception_alloc *base;

    if (!(base = try_Allocate(offsetof(struct exception_alloc, data[size]))))
        return NULL;
    return &base->data;
}

void *__cdecl AllocateException(size_t size)
{
    void *addr;

    TRACE("(%Iu)\n", size);

    if (!(addr = try_AllocateException(size)))
        __abi_WinRTraiseOutOfMemoryException();
    return addr;
}

void __cdecl Free(void *addr)
{
    TRACE("(%p)\n", addr);

    free(addr);
}

void __cdecl FreeException(void *addr)
{
    struct exception_alloc *base = CONTAINING_RECORD(addr, struct exception_alloc, data);

    TRACE("(%p)\n", addr);

    Free(base);
}

static inline struct control_block *impl_from_IWeakReference(IWeakReference *iface)
{
    return CONTAINING_RECORD(iface, struct control_block, IWeakReference_iface);
}

static HRESULT WINAPI control_block_QueryInterface(IWeakReference *iface, const GUID *iid, void **out)
{
    struct control_block *impl = impl_from_IWeakReference(iface);

    TRACE("(%p, %s, %p)", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IWeakReference))
    {
        IWeakReference_AddRef((*out = &impl->IWeakReference_iface));
        return S_OK;
    }

    *out = NULL;
    ERR("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI control_block_AddRef(IWeakReference *iface)
{
    struct control_block *impl = impl_from_IWeakReference(iface);

    TRACE("(%p)\n", iface);

    return InterlockedIncrement(&impl->ref_weak);
}

static ULONG WINAPI control_block_Release(IWeakReference *iface)
{
    struct control_block *impl = impl_from_IWeakReference(iface);
    ULONG ref = InterlockedDecrement(&impl->ref_weak);

    TRACE("(%p)\n", iface);

    if (!ref) Free(impl);
    return ref;
}

static HRESULT WINAPI control_block_Resolve(IWeakReference *iface, const GUID *iid, IInspectable **out)
{
    struct control_block *impl = impl_from_IWeakReference(iface);
    HRESULT hr;
    LONG ref;

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    do
    {
        ref = ReadNoFence(&impl->ref_strong);
        if (ref <= 0)
            return S_OK;
    } while (ref != InterlockedCompareExchange(&impl->ref_strong, ref + 1, ref));

    hr = IUnknown_QueryInterface(impl->object, iid, (void **)out);
    IUnknown_Release(impl->object);
    return hr;
}

static const IWeakReferenceVtbl control_block_vtbl =
{
    control_block_QueryInterface,
    control_block_AddRef,
    control_block_Release,
    control_block_Resolve,
};

void *__cdecl AllocateWithWeakRef(ptrdiff_t offset, size_t size)
{
    const size_t inline_max = sizeof(void *) == 8 ? 128 : 64;
    struct control_block *weakref;
    void *object;

    TRACE("(%Iu, %Iu)\n", offset, size);

    if (size > inline_max)
    {
        weakref = Allocate(sizeof(*weakref));
        if (!(object = try_Allocate(size)))
        {
            Free(weakref);
            __abi_WinRTraiseOutOfMemoryException();
        }
        weakref->is_inline = FALSE;
    }
    else /* Perform an inline allocation */
    {
        weakref = Allocate(sizeof(*weakref) + size);
        object = (char *)weakref + sizeof(*weakref);
        weakref->is_inline = TRUE;
    }

    *(struct control_block **)((char *)object + offset) = weakref;
    weakref->IWeakReference_iface.lpVtbl = &control_block_vtbl;
    weakref->object = object;
    weakref->ref_strong = weakref->ref_weak = 1;
    weakref->unknown = 0;
    weakref->is_exception = FALSE;

    return weakref->object;
}

void *__cdecl AllocateExceptionWithWeakRef(ptrdiff_t offset, size_t size)
{
    struct control_block *weakref;
    void *excp;

    TRACE("(%Iu, %Iu)\n", offset, size);

    /* AllocateExceptionWithWeakRef does not store the control block inline, regardless of size. */
    weakref = Allocate(sizeof(*weakref));
    if (!(excp = try_AllocateException(size)))
    {
        Free(weakref);
        __abi_WinRTraiseOutOfMemoryException();
    }
    *(struct control_block **)((char *)excp + offset) = weakref;
    weakref->IWeakReference_iface.lpVtbl = &control_block_vtbl;
    weakref->object = excp;
    weakref->ref_strong = weakref->ref_weak = 1;
    weakref->is_inline = FALSE;
    weakref->unknown = 0;
    weakref->is_exception = TRUE;

    return excp;
}

DEFINE_THISCALL_WRAPPER(control_block_ReleaseTarget, 4)
void __thiscall control_block_ReleaseTarget(struct control_block *weakref)
{
    void *object;

    TRACE("(%p)\n", weakref);

    if (weakref->is_inline || ReadNoFence(&weakref->ref_strong) >= 0) return;
    if ((object = InterlockedCompareExchangePointer((void *)&weakref->object, NULL, weakref->object)))
    {
        if (weakref->is_exception)
            FreeException(object);
        else
            Free(object);
    }
}

IWeakReference *WINAPI GetWeakReference(IUnknown *obj)
{
    IWeakReferenceSource *src;
    IWeakReference *ref;
    HRESULT hr;

    TRACE("(%p)\n", obj);

    if (!obj)
        __abi_WinRTraiseInvalidArgumentException();
    if (SUCCEEDED((hr = IUnknown_QueryInterface(obj, &IID_IWeakReferenceSource, (void **)&src))))
    {
        hr = IWeakReferenceSource_GetWeakReference(src, &ref);
        IWeakReferenceSource_Release(src);
    }
    if (FAILED(hr))
        __abi_WinRTraiseCOMException(hr);

    return ref;
}

IUnknown *WINAPI ResolveWeakReference(const GUID *iid, IWeakReference **weakref)
{
    IUnknown *obj = NULL;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(iid), weakref);

    if (*weakref && FAILED((hr = IWeakReference_Resolve(*weakref, iid, (IInspectable **)&obj))))
        __abi_WinRTraiseCOMException(hr);

    return obj;
}

struct __abi_type_descriptor
{
    const WCHAR *name;
    int type_id;
};

struct platform_type
{
    IInspectable IInspectable_iface;
    IStringable IPrintable_iface;
    IClosable IClosable_iface;
    IUnknown *marshal;
    const struct __abi_type_descriptor *desc;
    LONG ref;
};

static inline struct platform_type *impl_from_IInspectable(IInspectable *iface)
{
    return CONTAINING_RECORD(iface, struct platform_type, IInspectable_iface);
}

HRESULT WINAPI platform_type_QueryInterface(IInspectable *iface, const GUID *iid, void **out)
{
    struct platform_type *impl = impl_from_IInspectable(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject))
    {
        IInspectable_AddRef((*out = &impl->IInspectable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IPrintable))
    {
        IStringable_AddRef((*out = &impl->IPrintable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IClosable_AddRef((*out = &impl->IClosable_iface));
        return S_OK;
    }
    if (IsEqualGUID(iid, &IID_IMarshal))
        return IUnknown_QueryInterface(impl->marshal, iid, out);

    ERR("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI platform_type_AddRef(IInspectable *iface)
{
    struct platform_type *impl = impl_from_IInspectable(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&impl->ref);
}

static ULONG WINAPI platform_type_Release(IInspectable *iface)
{
    struct platform_type *impl = impl_from_IInspectable(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("(%p)\n", iface);

    if (!ref)
    {
        IUnknown_Release(impl->marshal);
        Free(impl);
    }
    return ref;
}

static HRESULT WINAPI platform_type_GetIids(IInspectable *iface, ULONG *count, GUID **iids)
{
    FIXME("(%p, %p, %p) stub\n", iface, count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI platform_type_GetRuntimeClassName(IInspectable *iface, HSTRING *name)
{
    static const WCHAR *str = L"Platform.Type";

    TRACE("(%p, %p)\n", iface, name);
    return WindowsCreateString(str, wcslen(str), name);
}

static HRESULT WINAPI platform_type_GetTrustLevel(IInspectable *iface, TrustLevel *level)
{
    FIXME("(%p, %p) stub\n", iface, level);
    return E_NOTIMPL;
}

DEFINE_RTTI_DATA(platform_type, 0, ".?AVType@Platform@@");
COM_VTABLE_RTTI_START(IInspectable, platform_type)
COM_VTABLE_ENTRY(platform_type_QueryInterface)
COM_VTABLE_ENTRY(platform_type_AddRef)
COM_VTABLE_ENTRY(platform_type_Release)
COM_VTABLE_ENTRY(platform_type_GetIids)
COM_VTABLE_ENTRY(platform_type_GetRuntimeClassName)
COM_VTABLE_ENTRY(platform_type_GetTrustLevel)
COM_VTABLE_RTTI_END;

DEFINE_IINSPECTABLE_(platform_type_printable, IStringable, struct platform_type,
    impl_platform_type_from_IStringable, IPrintable_iface, &impl->IInspectable_iface);

static HRESULT WINAPI platform_type_printable_ToString(IStringable *iface, HSTRING *str)
{
    struct platform_type *impl = impl_platform_type_from_IStringable(iface);

    TRACE("(%p, %p)\n", iface, str);

    return WindowsCreateString(impl->desc->name, impl->desc->name ? wcslen(impl->desc->name ) : 0, str);
}

DEFINE_RTTI_DATA(platform_type_printable, offsetof(struct platform_type, IPrintable_iface), ".?AVType@Platform@@");
COM_VTABLE_RTTI_START(IStringable, platform_type_printable)
COM_VTABLE_ENTRY(platform_type_printable_QueryInterface)
COM_VTABLE_ENTRY(platform_type_printable_AddRef)
COM_VTABLE_ENTRY(platform_type_printable_Release)
COM_VTABLE_ENTRY(platform_type_printable_GetIids)
COM_VTABLE_ENTRY(platform_type_printable_GetRuntimeClassName)
COM_VTABLE_ENTRY(platform_type_printable_GetTrustLevel)
COM_VTABLE_ENTRY(platform_type_printable_ToString)
COM_VTABLE_RTTI_END;

DEFINE_IINSPECTABLE(platform_type_closable, IClosable, struct platform_type, IInspectable_iface);

static HRESULT WINAPI platform_type_closable_Close(IClosable *iface)
{
    FIXME("(%p) stub\n", iface);
    return E_NOTIMPL;
}

DEFINE_RTTI_DATA(platform_type_closable, offsetof(struct platform_type, IClosable_iface), ".?AVType@Platform@@");
COM_VTABLE_RTTI_START(IClosable, platform_type_closable)
COM_VTABLE_ENTRY(platform_type_closable_QueryInterface)
COM_VTABLE_ENTRY(platform_type_closable_AddRef)
COM_VTABLE_ENTRY(platform_type_closable_Release)
COM_VTABLE_ENTRY(platform_type_closable_GetIids)
COM_VTABLE_ENTRY(platform_type_closable_GetRuntimeClassName)
COM_VTABLE_ENTRY(platform_type_closable_GetTrustLevel)
COM_VTABLE_ENTRY(platform_type_closable_Close)
COM_VTABLE_RTTI_END;

static void init_platform_type(void *base)
{
    INIT_RTTI(type_info, base);
    INIT_RTTI(platform_type, base);
    INIT_RTTI(platform_type_printable, base);
    INIT_RTTI(platform_type_closable, base);
}

static const char *debugstr_abi_type_descriptor(const struct __abi_type_descriptor *desc)
{
    if (!desc) return "(null)";

    return wine_dbg_sprintf("{%s, %d}", debugstr_w(desc->name), desc->type_id);
}

void *WINAPI __abi_make_type_id(const struct __abi_type_descriptor *desc)
{
    /* TODO:
     * Implement IEquatable. */
    struct platform_type *obj;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_abi_type_descriptor(desc));

    obj = Allocate(sizeof(*obj));
    obj->IInspectable_iface.lpVtbl = &platform_type_vtable.vtable;
    obj->IPrintable_iface.lpVtbl = &platform_type_printable_vtable.vtable;
    obj->IClosable_iface.lpVtbl = &platform_type_closable_vtable.vtable;
    obj->desc = desc;
    obj->ref = 1;
    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&obj->IInspectable_iface, &obj->marshal);
    if (FAILED(hr))
    {
        Free(obj);
        __abi_WinRTraiseCOMException(hr);
    }
    return &obj->IInspectable_iface;
}

bool __cdecl platform_type_Equals_Object(struct platform_type *this, struct platform_type *object)
{
    TRACE("(%p, %p)\n", this, object);

    return this == object || (this && object && this->desc == object->desc);
}

int __cdecl platform_type_GetTypeCode(struct platform_type *this)
{
    TRACE("(%p)\n", this);

    return this->desc->type_id;
}

HSTRING __cdecl platform_type_ToString(struct platform_type *this)
{
    HSTRING str = NULL;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED(hr = IStringable_ToString(&this->IPrintable_iface, &str)))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl platform_type_get_FullName(struct platform_type *type)
{
    TRACE("(%p)\n", type);

    return platform_type_ToString(type);
}

enum typecode
{
    TYPECODE_BOOLEAN = 3,
    TYPECODE_CHAR16 = 4,
    TYPECODE_UINT8 = 6,
    TYPECODE_INT16 = 7,
    TYPECODE_UINT16 = 8,
    TYPECODE_INT32 = 9,
    TYPECODE_UINT32 = 10,
    TYPECODE_INT64 = 11,
    TYPECODE_UINT64 = 12,
    TYPECODE_SINGLE = 13,
    TYPECODE_DOUBLE = 14,
    TYPECODE_DATETIME = 16,
    TYPECODE_STRING = 18,
    TYPECODE_TIMESPAN = 19,
    TYPECODE_POINT = 20,
    TYPECODE_SIZE = 21,
    TYPECODE_RECT = 22,
    TYPECODE_GUID = 23,
};

static const char *debugstr_typecode(int typecode)
{
    static const char *str[] =  {
        [TYPECODE_BOOLEAN] = "Boolean",
        [TYPECODE_CHAR16] = "char16",
        [TYPECODE_UINT8] = "uint8",
        [TYPECODE_INT16] = "int16",
        [TYPECODE_UINT16] = "uint16",
        [TYPECODE_INT32] = "int32",
        [TYPECODE_UINT32] = "uint32",
        [TYPECODE_INT64] = "int64",
        [TYPECODE_UINT64] = "uint64",
        [TYPECODE_SINGLE] = "float32",
        [TYPECODE_DOUBLE] = "double",
        [TYPECODE_DATETIME] = "DateTime",
        [TYPECODE_STRING] = "String",
        [TYPECODE_POINT] = "Point",
        [TYPECODE_SIZE] = "Size",
        [TYPECODE_RECT] = "Rect",
        [TYPECODE_GUID] = "Guid",
    };
    if (typecode < ARRAY_SIZE(str) && str[typecode]) return wine_dbg_sprintf("%s", str[typecode]);
    return wine_dbg_sprintf("%d", typecode);
}

void *WINAPI CreateValue(int typecode, const void *val)
{
    IPropertyValueStatics *statics;
    IInspectable *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_typecode(typecode), val);

    hr = GetActivationFactoryByPCWSTR(RuntimeClass_Windows_Foundation_PropertyValue, &IID_IPropertyValueStatics,
                                      (void **)&statics);
    if (FAILED(hr))
        __abi_WinRTraiseCOMException(hr);
    switch (typecode)
    {
    case TYPECODE_BOOLEAN:
        hr = IPropertyValueStatics_CreateBoolean(statics, *(boolean *)val, &obj);
        break;
    case TYPECODE_CHAR16:
        hr = IPropertyValueStatics_CreateChar16(statics, *(WCHAR *)val, &obj);
        break;
    case TYPECODE_UINT8:
        hr = IPropertyValueStatics_CreateUInt8(statics, *(BYTE *)val, &obj);
        break;
    case TYPECODE_UINT16:
        hr = IPropertyValueStatics_CreateUInt16(statics, *(UINT16 *)val, &obj);
        break;
    case TYPECODE_INT16:
        hr = IPropertyValueStatics_CreateInt16(statics, *(INT16 *)val, &obj);
        break;
    case TYPECODE_INT32:
        hr = IPropertyValueStatics_CreateInt32(statics, *(INT32 *)val, &obj);
        break;
    case TYPECODE_UINT32:
        hr = IPropertyValueStatics_CreateUInt32(statics, *(UINT32 *)val, &obj);
        break;
    case TYPECODE_INT64:
        hr = IPropertyValueStatics_CreateInt64(statics, *(INT64 *)val, &obj);
        break;
    case TYPECODE_UINT64:
        hr = IPropertyValueStatics_CreateUInt64(statics, *(UINT64 *)val, &obj);
        break;
    case TYPECODE_SINGLE:
        hr = IPropertyValueStatics_CreateSingle(statics, *(FLOAT *)val, &obj);
        break;
    case TYPECODE_DOUBLE:
        hr = IPropertyValueStatics_CreateDouble(statics, *(DOUBLE *)val, &obj);
        break;
    case TYPECODE_DATETIME:
        hr = IPropertyValueStatics_CreateDateTime(statics, *(DateTime *)val, &obj);
        break;
    case TYPECODE_STRING:
        hr = IPropertyValueStatics_CreateString(statics, *(HSTRING *)val, &obj);
        break;
    case TYPECODE_TIMESPAN:
        hr = IPropertyValueStatics_CreateTimeSpan(statics, *(TimeSpan *)val, &obj);
        break;
    case TYPECODE_POINT:
        hr = IPropertyValueStatics_CreatePoint(statics, *(Point *)val, &obj);
        break;
    case TYPECODE_SIZE:
        hr = IPropertyValueStatics_CreateSize(statics, *(Size *)val, &obj);
        break;
    case TYPECODE_RECT:
        hr = IPropertyValueStatics_CreateRect(statics, *(Rect *)val, &obj);
        break;
    case TYPECODE_GUID:
        hr = IPropertyValueStatics_CreateGuid(statics, *(GUID *)val, &obj);
        break;
    default:
        obj = NULL;
        hr = S_OK;
        break;
    }

    IPropertyValueStatics_Release(statics);
    if (FAILED(hr))
        __abi_WinRTraiseCOMException(hr);
    return obj;
}

static HRESULT hstring_sprintf(HSTRING *out, const WCHAR *fmt, ...)
{
    WCHAR buf[100];
    va_list args;
    int len;

    va_start(args, fmt);
    len = vswprintf(buf, ARRAY_SIZE(buf), fmt, args);
    va_end(args);
    return WindowsCreateString(buf, len, out);
}

HSTRING __cdecl Guid_ToString(const GUID *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_guid(this));

    hr = hstring_sprintf(&str, L"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", this->Data1, this->Data2,
                         this->Data3, this->Data4[0], this->Data4[1], this->Data4[2], this->Data4[3], this->Data4[4],
                         this->Data4[5], this->Data4[6], this->Data4[7]);
    if (FAILED(hr))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl Boolean_ToString(const boolean *this)
{
    const WCHAR *strW;
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    strW = *this ? L"true" : L"false";
    if (FAILED((hr = WindowsCreateString(strW, wcslen(strW), &str))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl char16_ToString(const WCHAR *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p): stub!\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%c", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl float32_ToString(const FLOAT *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%g", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl float64_ToString(const DOUBLE *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%g", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl int16_ToString(const INT16 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%hd", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl int32_ToString(const INT32 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%I32d", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl int64_ToString(const INT64 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%I64d", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl int8_ToString(const INT8 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%hhd", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl uint16_ToString(const UINT16 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%hu", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl uint32_ToString(const UINT32 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%I32u", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl uint64_ToString(const UINT64 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%I64u", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING __cdecl uint8_ToString(const UINT8 *this)
{
    HSTRING str;
    HRESULT hr;

    TRACE("(%p)\n", this);

    if (FAILED((hr = hstring_sprintf(&str, L"%hhu", *this))))
        __abi_WinRTraiseCOMException(hr);
    return str;
}

HSTRING WINAPI __abi_ObjectToString(IUnknown *obj, bool try_stringable)
{
    IInspectable *inspectable;
    IPropertyValue *propval;
    IStringable *stringable;
    HSTRING val = NULL;
    HRESULT hr = S_OK;

    TRACE("(%p, %d)\n", obj, try_stringable);

    if (!obj) return NULL;
    /* If try_stringable is true, native will first query for IStringable, and then IPrintable (which is just an alias
     * for IStringable). */
    if (try_stringable && (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IStringable, (void **)&stringable)) ||
                           SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IPrintable, (void **)&stringable))))
    {
        hr = IStringable_ToString(stringable, &val);
        IStringable_Release(stringable);
    }
    /* Next, native checks if this is an boxed type (IPropertyValue) storing a numeric-like or string value. */
    else if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IPropertyValue, (void **)&propval)))
    {
        PropertyType type;

        if (SUCCEEDED((hr = IPropertyValue_get_Type(propval, &type))))
        {
#define PROPVAL_SIMPLE(prop_type, pfx, c_type)                                        \
    case PropertyType_##prop_type:                                                    \
    {                                                                                 \
        c_type prop_type_##val;                                                       \
        if (SUCCEEDED(hr = IPropertyValue_Get##prop_type(propval, &prop_type_##val))) \
        {                                                                             \
           IPropertyValue_Release(propval);                                           \
           return pfx##_ToString(&prop_type_##val);                                   \
        }                                                                             \
        break;                                                                        \
    }
            switch (type)
            {
            PROPVAL_SIMPLE(Char16, char16, WCHAR)
            PROPVAL_SIMPLE(UInt8, uint8, UINT8)
            PROPVAL_SIMPLE(Int16, int16, INT16)
            PROPVAL_SIMPLE(UInt16, uint16, UINT16)
            PROPVAL_SIMPLE(Int32, int32, INT32)
            PROPVAL_SIMPLE(UInt32, uint32, UINT32)
            PROPVAL_SIMPLE(Int64, int64, INT64)
            PROPVAL_SIMPLE(UInt64, uint64, UINT64)
            PROPVAL_SIMPLE(Single, float32, FLOAT)
            PROPVAL_SIMPLE(Double, float64, DOUBLE)
            PROPVAL_SIMPLE(Boolean, Boolean, boolean)
            PROPVAL_SIMPLE(Guid, Guid, GUID)
            case PropertyType_String:
                hr = IPropertyValue_GetString(propval, &val);
                break;
            default:
                /* For other types, use the WinRT class name. */
                hr = IPropertyValue_GetRuntimeClassName(propval, &val);
            }
#undef PROPVAL_SIMPLE
        }
        IPropertyValue_Release(propval);
    }
    /* Finally, if this is an IInspectable, use the WinRT class name. Otherwise, return NULL. */
    else if (SUCCEEDED(IUnknown_QueryInterface(obj, &IID_IInspectable, (void **)&inspectable)))
    {
        hr = IInspectable_GetRuntimeClassName(inspectable, &val);
        IInspectable_Release(inspectable);
    }

    if (FAILED(hr))
        __abi_WinRTraiseCOMException(hr);

    return val;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        init_exception(inst);
        init_platform_type(inst);
        init_delegate(inst);
    }
    return TRUE;
}
