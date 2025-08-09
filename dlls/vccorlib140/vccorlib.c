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
