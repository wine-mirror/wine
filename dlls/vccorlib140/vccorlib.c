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

#include <stdbool.h>

#include "initguid.h"
#include "roapi.h"
#include "weakreference.h"
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

void *__cdecl Allocate(size_t size)
{
    void *addr;

    TRACE("(%Iu)\n", size);

    addr = malloc(size);
    /* TODO: Throw a COMException on allocation failure. */
    if (!addr)
        FIXME("allocation failure\n");
    return addr;
}

void *__cdecl AllocateException(size_t size)
{
    FIXME("(%Iu): stub!\n", size);
    return NULL;
}

void __cdecl Free(void *addr)
{
    TRACE("(%p)\n", addr);

    free(addr);
}

void __cdecl FreeException(void *addr)
{
    FIXME("(%p): stub!\n", addr);
}

struct control_block
{
    IWeakReference IWeakReference_iface;
    LONG ref_weak;
    LONG ref_strong;
    IUnknown *object;
    bool is_inline;
    bool unknown;
    bool is_exception;
#ifdef _WIN32
    char _padding[5];
#endif
};

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
        object = Allocate(size);
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
    FIXME("(%Iu, %Iu): stub!\n", offset, size);
    return NULL;
}

DEFINE_THISCALL_WRAPPER(control_block_ReleaseTarget, 4)
void __thiscall control_block_ReleaseTarget(struct control_block *weakref)
{
    void *object;

    TRACE("(%p)\n", weakref);

    if (weakref->is_inline || ReadNoFence(&weakref->ref_strong) >= 0) return;
    if ((object = InterlockedCompareExchangePointer((void *)&weakref->object, NULL, weakref->object)))
        Free(object);
}

struct __abi_type_descriptor
{
    const WCHAR *name;
    int type_id;
};

struct platform_type
{
    IInspectable IInspectable_iface;
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
     * Implement IEquatable and IPrintable.
     * Throw a COMException if CoCreateFreeThreadedMarshaler fails. */
    struct platform_type *obj;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_abi_type_descriptor(desc));

    obj = Allocate(sizeof(*obj));
    obj->IInspectable_iface.lpVtbl = &platform_type_vtable.vtable;
    obj->IClosable_iface.lpVtbl = &platform_type_closable_vtable.vtable;
    obj->desc = desc;
    obj->ref = 1;
    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&obj->IInspectable_iface, &obj->marshal);
    if (FAILED(hr))
    {
        FIXME("CoCreateFreeThreadedMarshaler failed: %#lx\n", hr);
        Free(obj);
        return NULL;
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

    /* TODO: Throw a COMException if this fails */
    hr = WindowsCreateString(this->desc->name, this->desc->name ? wcslen(this->desc->name) : 0, &str);
    if (FAILED(hr))
        FIXME("WindowsCreateString failed: %#lx\n", hr);
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
    {
        FIXME("GetActivationFactoryByPCWSTR failed: %#lx\n", hr);
        return NULL;
    }
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
    {
        FIXME("Failed to create IPropertyValue object: %#lx\n", hr);
        return NULL;
    }
    return obj;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        init_platform_type(inst);
    return TRUE;
}
