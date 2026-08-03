/* WinRT Windows.ApplicationModel.Core.CoreCursor implementation
 *
 * Copyright 2024 Onni Kukkonen
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
#include <assert.h>
#include <winuser.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(corecursor);

struct factory_impl
{
    IActivationFactory IActivationFactory_iface;
    ICoreCursorFactory ICoreCursorFactory_iface;
    LONG ref;
};

static inline struct factory_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct factory_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct factory_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ICoreCursorFactory))
    {
        IUnknown_AddRef(iface);

        *out = &factory->ICoreCursorFactory_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct factory_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct factory_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI factory_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI factory_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    FIXME("iface %p, instance %p stub!\n", iface, instance);
    return E_NOTIMPL;
}

static const struct IActivationFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    /* IInspectable methods */
    factory_GetIids,
    factory_GetRuntimeClassName,
    factory_GetTrustLevel,
    /* IActivationFactory methods */
    factory_ActivateInstance,
};

static inline struct factory_impl *impl_from_ICoreCursorFactory(ICoreCursorFactory *iface)
{
    return CONTAINING_RECORD(iface, struct factory_impl, ICoreCursorFactory_iface);
}

static HRESULT WINAPI cursorfactory_QueryInterface(
        ICoreCursorFactory *iface, REFIID iid, void **out)
{
    struct factory_impl *factory = impl_from_ICoreCursorFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ICoreCursorFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICoreCursorFactory_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI cursorfactory_AddRef(ICoreCursorFactory *iface)
{
    struct factory_impl *factory = impl_from_ICoreCursorFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI cursorfactory_Release(ICoreCursorFactory *iface)
{
    struct factory_impl *factory = impl_from_ICoreCursorFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI cursorfactory_GetIids(
        ICoreCursorFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI cursorfactory_GetRuntimeClassName(
        ICoreCursorFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI cursorfactory_GetTrustLevel(
        ICoreCursorFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI cursorfactory_CreateCursor(
        ICoreCursorFactory *iface, CoreCursorType type, UINT32 id, ICoreCursor **instance)
{
    *instance = create_cursor(id, type);
    if (*instance == NULL) {
        ERR("create_cursor failed!\n");
        return E_FAIL;
    }

    return S_OK;
}

static const struct ICoreCursorFactoryVtbl cursorfactory_vtbl =
{
    cursorfactory_QueryInterface,
    cursorfactory_AddRef,
    cursorfactory_Release,
    /* IInspectable methods */
    cursorfactory_GetIids,
    cursorfactory_GetRuntimeClassName,
    cursorfactory_GetTrustLevel,
    /* ICoreCursorFactory methods */
    cursorfactory_CreateCursor,
};

static struct factory_impl factory_impl =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .ICoreCursorFactory_iface.lpVtbl = &cursorfactory_vtbl,
    .ref = 1,
};

IActivationFactory *cursor_factory = &factory_impl.IActivationFactory_iface;

struct corecursor_impl
{
    ICoreCursor ICoreCursor_iface;
    UINT32 id;
    CoreCursorType type;
    LONG ref;
};

static inline struct corecursor_impl *impl_from_ICoreCursor(ICoreCursor *iface)
{
    return CONTAINING_RECORD(iface, struct corecursor_impl, ICoreCursor_iface);
}

static HRESULT WINAPI corecursor_QueryInterface(
        ICoreCursor *iface, REFIID iid, void **out)
{
    struct corecursor_impl *factory = impl_from_ICoreCursor(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ICoreCursor))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ICoreCursor_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI corecursor_AddRef(ICoreCursor *iface)
{
    struct corecursor_impl *factory = impl_from_ICoreCursor(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI corecursor_Release(ICoreCursor *iface)
{
    struct corecursor_impl *factory = impl_from_ICoreCursor(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI corecursor_GetIids(
        ICoreCursor *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI corecursor_GetRuntimeClassName(
        ICoreCursor *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI corecursor_GetTrustLevel(
        ICoreCursor *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI corecursor_get_Id(
        ICoreCursor *iface, UINT32 *value)
{
    struct corecursor_impl *impl = impl_from_ICoreCursor(iface);
    *value = impl->id;
    return S_OK;
}

static HRESULT WINAPI corecursor_get_Type(
        ICoreCursor *iface, CoreCursorType *value)
{
    struct corecursor_impl *impl = impl_from_ICoreCursor(iface);
    *value = impl->type;
    return S_OK;
}

static const struct ICoreCursorVtbl cursor_vtbl =
{
    corecursor_QueryInterface,
    corecursor_AddRef,
    corecursor_Release,
    /* IInspectable methods */
    corecursor_GetIids,
    corecursor_GetRuntimeClassName,
    corecursor_GetTrustLevel,
    /* ICoreCursor methods */
    corecursor_get_Id,
    corecursor_get_Type,
};

ICoreCursor *create_cursor(UINT32 id, CoreCursorType type) {
    struct corecursor_impl *object;

    TRACE("id %u, type %d.\n", id, type);

    if (!(object = calloc(1, sizeof(*object))))
        return NULL;

    object->ICoreCursor_iface.lpVtbl = &cursor_vtbl;
    object->ref = 1;
    object->id = id;
    object->type = type;

    return &object->ICoreCursor_iface;
}

