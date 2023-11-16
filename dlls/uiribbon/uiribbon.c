/*
 * Copyright 2017 Fabian Maurer
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

#include "wine/debug.h"

#define COBJMACROS

#include "uiribbon_private.h"

#include <stdio.h>

WINE_DEFAULT_DEBUG_CHANNEL(uiribbon);

static inline UIRibbonFrameworkImpl *impl_from_IUIFramework(IUIFramework *iface)
{
    return CONTAINING_RECORD(iface, UIRibbonFrameworkImpl, IUIFramework_iface);
}

/*** IUnknown methods ***/

static HRESULT WINAPI UIRibbonFrameworkImpl_QueryInterface(IUIFramework *iface, REFIID riid, void **ppvObject)
{
    UIRibbonFrameworkImpl *This = impl_from_IUIFramework(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IUIFramework))
    {
        IUnknown_AddRef(iface);
        *ppvObject = &This->IUIFramework_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI UIRibbonFrameworkImpl_AddRef(IUIFramework *iface)
{
    UIRibbonFrameworkImpl *This = impl_from_IUIFramework(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref %ld\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI UIRibbonFrameworkImpl_Release(IUIFramework *iface)
{
    UIRibbonFrameworkImpl *This = impl_from_IUIFramework(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref %ld\n", iface, This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IUIFramework methods ***/

static HRESULT WINAPI UIRibbonFrameworkImpl_Initialize(IUIFramework *iface, HWND frameWnd, IUIApplication *application)
{
    FIXME("(%p, %p): stub!\n", frameWnd, application);

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_Destroy(IUIFramework *iface)
{
    FIXME("(): stub!\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_LoadUI(IUIFramework *iface, HINSTANCE instance, LPCWSTR resourceName)
{
    FIXME("(%p, %s): stub!\n", instance, debugstr_w(resourceName));

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_GetView(IUIFramework *iface, UINT32 viewId, REFIID riid, void **ppv)
{
    FIXME("(%u, %p, %p): stub!\n", viewId, riid, ppv);

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_GetUICommandProperty(IUIFramework *iface, UINT32 commandId, REFPROPERTYKEY key, PROPVARIANT *value)
{
    FIXME("(%u, %p, %p): stub!\n", commandId, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_SetUICommandProperty(IUIFramework *iface, UINT32 commandId, REFPROPERTYKEY key, REFPROPVARIANT value)
{
    FIXME("(%u, %p, %p): stub!\n", commandId, key, value);

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_InvalidateUICommand(IUIFramework *iface, UINT32 commandId, UI_INVALIDATIONS flags, const PROPERTYKEY *key)
{
    FIXME("(%u, %#x, %p): stub!\n", commandId, flags, key);

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_FlushPendingInvalidations(IUIFramework *iface)
{
    FIXME("(): stub!\n");

    return E_NOTIMPL;
}

static HRESULT WINAPI UIRibbonFrameworkImpl_SetModes(IUIFramework *iface, INT32 iModes)
{
    FIXME("(%d): stub!\n", iModes);

    return E_NOTIMPL;
}

static const IUIFrameworkVtbl IUIFramework_Vtbl =
{
    UIRibbonFrameworkImpl_QueryInterface,
    UIRibbonFrameworkImpl_AddRef,
    UIRibbonFrameworkImpl_Release,
    UIRibbonFrameworkImpl_Initialize,
    UIRibbonFrameworkImpl_Destroy,
    UIRibbonFrameworkImpl_LoadUI,
    UIRibbonFrameworkImpl_GetView,
    UIRibbonFrameworkImpl_GetUICommandProperty,
    UIRibbonFrameworkImpl_SetUICommandProperty,
    UIRibbonFrameworkImpl_InvalidateUICommand,
    UIRibbonFrameworkImpl_FlushPendingInvalidations,
    UIRibbonFrameworkImpl_SetModes
};

HRESULT UIRibbonFrameworkImpl_Create(IUnknown *pUnkOuter, void **ppObj)
{
    UIRibbonFrameworkImpl *object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(UIRibbonFrameworkImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IUIFramework_iface.lpVtbl = &IUIFramework_Vtbl;
    object->ref = 1;

    *ppObj = &object->IUIFramework_iface;

    return S_OK;
}
