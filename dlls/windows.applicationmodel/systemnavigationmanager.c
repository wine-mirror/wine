/* WinRT Windows.UI.Core.SystemNavigationManager implementation
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
#include <libxml/parser.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <pathcch.h>
#include <shlwapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(navmgr);

struct navigationmanager_impl
{
    IActivationFactory IActivationFactory_iface;
    ISystemNavigationManager ISystemNavigationManager_iface;
    ISystemNavigationManagerStatics ISystemNavigationManagerStatics_iface;
    LONG ref;
};

static inline struct navigationmanager_impl *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct navigationmanager_impl, IActivationFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct navigationmanager_impl *factory = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = &factory->IActivationFactory_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISystemNavigationManagerStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ISystemNavigationManagerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IActivationFactory *iface)
{
    struct navigationmanager_impl *factory = impl_from_IActivationFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI factory_Release(IActivationFactory *iface)
{
    struct navigationmanager_impl *factory = impl_from_IActivationFactory(iface);
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

static inline struct navigationmanager_impl *impl_from_ISystemNavigationManagerStatics(ISystemNavigationManagerStatics *iface)
{
    return CONTAINING_RECORD(iface, struct navigationmanager_impl, ISystemNavigationManagerStatics_iface);
}

static HRESULT WINAPI sysnavstatic_QueryInterface(
        ISystemNavigationManagerStatics *iface, REFIID iid, void **out)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManagerStatics(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISystemNavigationManagerStatics))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ISystemNavigationManagerStatics_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sysnavstatic_AddRef(ISystemNavigationManagerStatics *iface)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManagerStatics(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sysnavstatic_Release(ISystemNavigationManagerStatics *iface)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManagerStatics(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI sysnavstatic_GetIids(
        ISystemNavigationManagerStatics *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnavstatic_GetRuntimeClassName(
        ISystemNavigationManagerStatics *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnavstatic_GetTrustLevel(
        ISystemNavigationManagerStatics *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnavstatic_GetForCurrentView(
        ISystemNavigationManagerStatics *iface, ISystemNavigationManager** loader)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManagerStatics(iface);
    FIXME("iface %p, loader %p stub!\n", iface, loader);
    *loader = &factory->ISystemNavigationManager_iface;
    return S_OK;
}

static const struct ISystemNavigationManagerStaticsVtbl sysnavstatic_vtbl =
{
    sysnavstatic_QueryInterface,
    sysnavstatic_AddRef,
    sysnavstatic_Release,
    /* IInspectable methods */
    sysnavstatic_GetIids,
    sysnavstatic_GetRuntimeClassName,
    sysnavstatic_GetTrustLevel,
    /* ISystemNavigationManagerStatics methods */
    sysnavstatic_GetForCurrentView,
};

static inline struct navigationmanager_impl *impl_from_ISystemNavigationManager(ISystemNavigationManager *iface)
{
    return CONTAINING_RECORD(iface, struct navigationmanager_impl, ISystemNavigationManager_iface);
}

static HRESULT WINAPI sysnav_QueryInterface(
        ISystemNavigationManager *iface, REFIID iid, void **out)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManager(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISystemNavigationManager))
    {
        IUnknown_AddRef(iface);
        *out = &factory->ISystemNavigationManager_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI sysnav_AddRef(ISystemNavigationManager *iface)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManager(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI sysnav_Release(ISystemNavigationManager *iface)
{
    struct navigationmanager_impl *factory = impl_from_ISystemNavigationManager(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("iface %p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static HRESULT WINAPI sysnav_GetIids(
        ISystemNavigationManager *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnav_GetRuntimeClassName(
        ISystemNavigationManager *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnav_GetTrustLevel(
        ISystemNavigationManager *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI sysnav_add_BackRequested(
        ISystemNavigationManager *iface, IEventHandler_BackRequestedEventArgs *handler, EventRegistrationToken *token)
{
    FIXME("iface %p, handler %p, token %p stub!\n", iface, handler, token);
    return S_OK;
}

static HRESULT WINAPI sysnav_remove_BackRequested(
        ISystemNavigationManager *iface, EventRegistrationToken token)
{
    FIXME("iface %p, token %I64u stub!\n", iface, token.value);
    return S_OK;
}

static const struct ISystemNavigationManagerVtbl sysnav_vtbl =
{
    sysnav_QueryInterface,
    sysnav_AddRef,
    sysnav_Release,
    /* IInspectable methods */
    sysnav_GetIids,
    sysnav_GetRuntimeClassName,
    sysnav_GetTrustLevel,
    /* ISystemNavigationManager methods */
    sysnav_add_BackRequested,
    sysnav_remove_BackRequested,
};

static struct navigationmanager_impl navigationmanager_impl_global =
{
    .IActivationFactory_iface.lpVtbl = &factory_vtbl,
    .ISystemNavigationManager_iface.lpVtbl = &sysnav_vtbl,
    .ISystemNavigationManagerStatics_iface.lpVtbl = &sysnavstatic_vtbl,
    .ref = 1,
};

IActivationFactory *sysnav_factory = &navigationmanager_impl_global.IActivationFactory_iface;
