/*
 * Dynamic HyperText Markup Language Editing Control
 *
 * Copyright 2017 Alex Henrie
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
#include "dhtmled.h"
#include "dhtmled_private.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dhtmled);

typedef struct
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create)(REFIID iid, void **out);
} ClassFactoryImpl;

static inline ClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(&IID_IUnknown, iid) || IsEqualGUID(&IID_IClassFactory, iid))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2; /* non-heap based object */
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1; /* non-heap based object */
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **out)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);

    TRACE("(%p)->(%p, %s, %p)\n", iface, outer, debugstr_guid(iid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return This->create(iid, out);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("(%p)->(%x)\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl ClassFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HINSTANCE dhtmled_instance;

/******************************************************************
 *          DllMain (dhtmled.ocx.@)
 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, VOID *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            dhtmled_instance = instance;
            break;
    }

    return TRUE;
}

/***********************************************************************
 *          DllGetClassObject (dhtmled.ocx.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *out)
{
    static ClassFactoryImpl dhtml_edit_cf = { {&ClassFactoryVtbl}, dhtml_edit_create };

    TRACE("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualGUID(clsid, &CLSID_DHTMLEdit))
        return IClassFactory_QueryInterface(&dhtml_edit_cf.IClassFactory_iface, iid, out);

    FIXME("no class for %s\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *          DllCanUnloadNow (dhtmled.ocx.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    TRACE("()\n");
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer (dhtmled.ocx.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(dhtmled_instance);
}

/***********************************************************************
 *          DllUnregisterServer (dhtmled.ocx.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(dhtmled_instance);
}
