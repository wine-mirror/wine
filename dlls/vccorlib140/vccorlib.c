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

WINE_DEFAULT_DEBUG_CHANNEL(vccorlib);

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

void __cdecl Free(void *addr)
{
    TRACE("(%p)\n", addr);

    free(addr);
}

struct control_block
{
    IWeakReference IWeakReference_iface;
    LONG ref_weak;
    LONG ref_strong;
    IUnknown *object;
    bool is_inline;
    UINT16 unknown;
#ifdef _WIN32
    char _padding[4];
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

    return weakref->object;
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
    IClosable IClosable_iface;
    IUnknown *marshal;
    const struct __abi_type_descriptor *desc;
    LONG ref;
};

static inline struct platform_type *impl_from_IClosable(IClosable *iface)
{
    return CONTAINING_RECORD(iface, struct platform_type, IClosable_iface);
}

static HRESULT WINAPI platform_type_QueryInterface(IClosable *iface, const GUID *iid, void **out)
{
    struct platform_type *impl = impl_from_IClosable(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IClosable) ||
        IsEqualGUID(iid, &IID_IAgileObject))
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

static ULONG WINAPI platform_type_AddRef(IClosable *iface)
{
    struct platform_type *impl = impl_from_IClosable(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&impl->ref);
}

static ULONG WINAPI platform_type_Release(IClosable *iface)
{
    struct platform_type *impl = impl_from_IClosable(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("(%p)\n", iface);

    if (!ref)
    {
        IUnknown_Release(impl->marshal);
        Free(impl);
    }
    return ref;
}

static HRESULT WINAPI platform_type_GetIids(IClosable *iface, ULONG *count, GUID **iids)
{
    FIXME("(%p, %p, %p) stub\n", iface, count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI platform_type_GetRuntimeClassName(IClosable *iface, HSTRING *name)
{
    static const WCHAR *str = L"Platform.Type";

    TRACE("(%p, %p)\n", iface, name);
    return WindowsCreateString(str, wcslen(str), name);
}

static HRESULT WINAPI platform_type_GetTrustLevel(IClosable *iface, TrustLevel *level)
{
    FIXME("(%p, %p) stub\n", iface, level);
    return E_NOTIMPL;
}

static HRESULT WINAPI platform_type_Close(IClosable *iface)
{
    FIXME("(%p) stub\n", iface);
    return E_NOTIMPL;
}

static const IClosableVtbl platform_type_closable_vtbl =
{
    /* IUnknown */
    platform_type_QueryInterface,
    platform_type_AddRef,
    platform_type_Release,
    /* IInspectable */
    platform_type_GetIids,
    platform_type_GetRuntimeClassName,
    platform_type_GetTrustLevel,
    /* ICloseable */
    platform_type_Close
};

static const char *debugstr_abi_type_descriptor(const struct __abi_type_descriptor *desc)
{
    if (!desc) return "(null)";

    return wine_dbg_sprintf("{%s, %d}", debugstr_w(desc->name), desc->type_id);
}

void *WINAPI __abi_make_type_id(const struct __abi_type_descriptor *desc)
{
    /* TODO:
     * Emit RTTI for Platform::Type.
     * Implement IEquatable and IPrintable.
     * Throw a COMException if CoCreateFreeThreadedMarshaler fails. */
    struct platform_type *obj;
    HRESULT hr;

    TRACE("(%s)\n", debugstr_abi_type_descriptor(desc));

    obj = Allocate(sizeof(*obj));
    obj->IClosable_iface.lpVtbl = &platform_type_closable_vtbl;
    obj->desc = desc;
    obj->ref = 1;
    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&obj->IClosable_iface, &obj->marshal);
    if (FAILED(hr))
    {
        FIXME("CoCreateFreeThreadedMarshaler failed: %#lx\n", hr);
        Free(obj);
        return NULL;
    }
    return &obj->IClosable_iface;
}

bool __cdecl platform_type_Equals_Object(void *this, void *object)
{
    FIXME("(%p, %p) stub\n", this, object);

    return false;
}

int __cdecl platform_type_GetTypeCode(void *this)
{
    FIXME("(%p) stub\n", this);

    return 0;
}

HSTRING __cdecl platform_type_ToString(void *this)
{
    FIXME("(%p) stub\n", this);

    return NULL;
}

HSTRING __cdecl platform_type_get_FullName(void *type)
{
    FIXME("(%p) stub\n", type);

    return NULL;
}
