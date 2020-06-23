/*
 * Copyright 2019 Alistair Leslie-Hughes
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "oleidl.h"
#include "rpcproxy.h"
#include "wine/heap.h"
#include "wine/debug.h"

#include "directmanipulation.h"

WINE_DEFAULT_DEBUG_CHANNEL(manipulation);

static HINSTANCE dm_instance;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    TRACE("(%p, %u, %p)\n", instance, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            dm_instance = instance;
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( dm_instance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( dm_instance );
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}


struct directmanipulation
{
    IDirectManipulationManager2 IDirectManipulationManager2_iface;
    IDirectManipulationUpdateManager *updatemanager;
    LONG ref;
};

struct directupdatemanager
{
    IDirectManipulationUpdateManager IDirectManipulationUpdateManager_iface;
    LONG ref;
};

static inline struct directmanipulation *impl_from_IDirectManipulationManager2(IDirectManipulationManager2 *iface)
{
    return CONTAINING_RECORD(iface, struct directmanipulation, IDirectManipulationManager2_iface);
}

static inline struct directupdatemanager *impl_from_IDirectManipulationUpdateManager(IDirectManipulationUpdateManager *iface)
{
    return CONTAINING_RECORD(iface, struct directupdatemanager, IDirectManipulationUpdateManager_iface);
}

static HRESULT WINAPI update_manager_QueryInterface(IDirectManipulationUpdateManager *iface, REFIID riid,void **ppv)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationUpdateManager)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p), not found\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

ULONG WINAPI update_manager_AddRef(IDirectManipulationUpdateManager *iface)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

ULONG WINAPI update_manager_Release(IDirectManipulationUpdateManager *iface)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI update_manager_RegisterWaitHandleCallback(IDirectManipulationUpdateManager *iface, HANDLE handle,
            IDirectManipulationUpdateHandler *handler, DWORD *cookie)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);
    FIXME("%p, %p, %p, %p\n", This, handle, handler, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI update_manager_UnregisterWaitHandleCallback(IDirectManipulationUpdateManager *iface, DWORD cookie)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);
    FIXME("%p, %x\n", This, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI update_manager_Update(IDirectManipulationUpdateManager *iface, IDirectManipulationFrameInfoProvider *provider)
{
    struct directupdatemanager *This = impl_from_IDirectManipulationUpdateManager(iface);
    FIXME("%p, %p\n", This, provider);
    return E_NOTIMPL;
}

struct IDirectManipulationUpdateManagerVtbl updatemanagerVtbl =
{
    update_manager_QueryInterface,
    update_manager_AddRef,
    update_manager_Release,
    update_manager_RegisterWaitHandleCallback,
    update_manager_UnregisterWaitHandleCallback,
    update_manager_Update
};

static HRESULT create_update_manager(IDirectManipulationUpdateManager **obj)
{
    struct directupdatemanager *object;

    object = heap_alloc(sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    object->IDirectManipulationUpdateManager_iface.lpVtbl = &updatemanagerVtbl;
    object->ref = 1;

    *obj = &object->IDirectManipulationUpdateManager_iface;

    return S_OK;
}

struct primarycontext
{
    IDirectManipulationPrimaryContent IDirectManipulationPrimaryContent_iface;
    IDirectManipulationContent IDirectManipulationContent_iface;
    LONG ref;
};

static inline struct primarycontext *impl_from_IDirectManipulationPrimaryContent(IDirectManipulationPrimaryContent *iface)
{
    return CONTAINING_RECORD(iface, struct primarycontext, IDirectManipulationPrimaryContent_iface);
}

static inline struct primarycontext *impl_from_IDirectManipulationContent(IDirectManipulationContent *iface)
{
    return CONTAINING_RECORD(iface, struct primarycontext, IDirectManipulationContent_iface);
}

static HRESULT WINAPI primary_QueryInterface(IDirectManipulationPrimaryContent *iface, REFIID riid, void **ppv)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationPrimaryContent))
    {
        IDirectManipulationPrimaryContent_AddRef(&This->IDirectManipulationPrimaryContent_iface);
        *ppv = &This->IDirectManipulationPrimaryContent_iface;
        return S_OK;
    }
    else if(IsEqualGUID(riid, &IID_IDirectManipulationContent))
    {
        IUnknown_AddRef(iface);
        *ppv = &This->IDirectManipulationContent_iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI primary_AddRef(IDirectManipulationPrimaryContent *iface)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI primary_Release(IDirectManipulationPrimaryContent *iface)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI primary_SetSnapInterval(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_MOTION_TYPES motion,
            float interval, float offset)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d, %f, %f\n", This, motion, interval, offset);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetSnapPoints(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_MOTION_TYPES motion,
            const float *points, DWORD count)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d, %p, %d\n", This, motion, points, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetSnapType(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_MOTION_TYPES motion,
            DIRECTMANIPULATION_SNAPPOINT_TYPE type)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d, %d\n", This, motion, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetSnapCoordinate(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_MOTION_TYPES motion,
            DIRECTMANIPULATION_SNAPPOINT_COORDINATE coordinate, float origin)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d, %d, %f\n", This, motion, coordinate, origin);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetZoomBoundaries(IDirectManipulationPrimaryContent *iface, float minimum, float maximum)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %f, %f\n", This, minimum, maximum);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetHorizontalAlignment(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_HORIZONTALALIGNMENT alignment)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_SetVerticalAlignment(IDirectManipulationPrimaryContent *iface, DIRECTMANIPULATION_VERTICALALIGNMENT alignment)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %d\n", This, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_GetInertiaEndTransform(IDirectManipulationPrimaryContent *iface, float *matrix, DWORD count)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p,  %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI primary_GetCenterPoint(IDirectManipulationPrimaryContent *iface, float *x, float *y)
{
    struct primarycontext *This = impl_from_IDirectManipulationPrimaryContent(iface);
    FIXME("%p, %p, %p\n", This, x, y);
    return E_NOTIMPL;
}

static const IDirectManipulationPrimaryContentVtbl primaryVtbl =
{
    primary_QueryInterface,
    primary_AddRef,
    primary_Release,
    primary_SetSnapInterval,
    primary_SetSnapPoints,
    primary_SetSnapType,
    primary_SetSnapCoordinate,
    primary_SetZoomBoundaries,
    primary_SetHorizontalAlignment,
    primary_SetVerticalAlignment,
    primary_GetInertiaEndTransform,
    primary_GetCenterPoint
};


static HRESULT WINAPI content_QueryInterface(IDirectManipulationContent *iface, REFIID riid, void **ppv)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);

    return IDirectManipulationPrimaryContent_QueryInterface(&This->IDirectManipulationPrimaryContent_iface,
            riid, ppv);
}

static ULONG WINAPI content_AddRef(IDirectManipulationContent *iface)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    return IDirectManipulationPrimaryContent_AddRef(&This->IDirectManipulationPrimaryContent_iface);
}

static ULONG WINAPI content_Release(IDirectManipulationContent *iface)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    return IDirectManipulationPrimaryContent_Release(&This->IDirectManipulationPrimaryContent_iface);
}

static HRESULT WINAPI content_GetContentRect(IDirectManipulationContent *iface, RECT *size)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_SetContentRect(IDirectManipulationContent *iface, const RECT *size)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p\n", This, size);
    return S_OK;
}

static HRESULT WINAPI content_GetViewport(IDirectManipulationContent *iface, REFIID riid, void **object)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %p\n", This, debugstr_guid(riid), object);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_GetTag(IDirectManipulationContent *iface, REFIID riid, void **object, UINT32 *id)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %p, %p\n", This, debugstr_guid(riid), object, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_SetTag(IDirectManipulationContent *iface, IUnknown *object, UINT32 id)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %d\n", This, object, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_GetOutputTransform(IDirectManipulationContent *iface,
                    float *matrix, DWORD count)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_GetContentTransform(IDirectManipulationContent *iface,
                    float *matrix, DWORD count)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI content_SyncContentTransform(IDirectManipulationContent *iface,
                    const float *matrix, DWORD count)
{
    struct primarycontext *This = impl_from_IDirectManipulationContent(iface);
    FIXME("%p, %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static const IDirectManipulationContentVtbl contentVtbl =
{
    content_QueryInterface,
    content_AddRef,
    content_Release,
    content_GetContentRect,
    content_SetContentRect,
    content_GetViewport,
    content_GetTag,
    content_SetTag,
    content_GetOutputTransform,
    content_GetContentTransform,
    content_SyncContentTransform
};

struct directviewport
{
    IDirectManipulationViewport2 IDirectManipulationViewport2_iface;
    LONG ref;
};

static inline struct directviewport *impl_from_IDirectManipulationViewport2(IDirectManipulationViewport2 *iface)
{
    return CONTAINING_RECORD(iface, struct directviewport, IDirectManipulationViewport2_iface);
}

static HRESULT WINAPI viewport_QueryInterface(IDirectManipulationViewport2 *iface, REFIID riid, void **ppv)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationViewport) ||
        IsEqualGUID(riid, &IID_IDirectManipulationViewport2))
    {
        IDirectManipulationViewport2_AddRef(&This->IDirectManipulationViewport2_iface);
        *ppv = &This->IDirectManipulationViewport2_iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI viewport_AddRef(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI viewport_Release(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI viewport_Enable(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_Disable(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetContact(IDirectManipulationViewport2 *iface, UINT32 id)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_ReleaseContact(IDirectManipulationViewport2 *iface, UINT32 id)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_ReleaseAllContacts(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_GetStatus(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_STATUS *status)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p\n", This, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_GetTag(IDirectManipulationViewport2 *iface, REFIID riid, void **object, UINT32 *id)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %s, %p, %p\n", This, debugstr_guid(riid), object, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetTag(IDirectManipulationViewport2 *iface, IUnknown *object, UINT32 id)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p, %u\n", This, object, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_GetViewportRect(IDirectManipulationViewport2 *iface, RECT *viewport)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p\n", This, viewport);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetViewportRect(IDirectManipulationViewport2 *iface, const RECT *viewport)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p\n", This, viewport);
    return S_OK;
}

static HRESULT WINAPI viewport_ZoomToRect(IDirectManipulationViewport2 *iface, const float left,
                    const float top, const float right, const float bottom, BOOL animate)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %f, %f, %f, %f, %d\n", This, left, top, right, bottom, animate);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetViewportTransform(IDirectManipulationViewport2 *iface,
                    const float *matrix, DWORD count)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SyncDisplayTransform(IDirectManipulationViewport2 *iface,
                    const float *matrix, DWORD count)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p, %d\n", This, matrix, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_GetPrimaryContent(IDirectManipulationViewport2 *iface, REFIID riid, void **object)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    TRACE("%p, %s, %p\n", This, debugstr_guid(riid), object);
    if(IsEqualGUID(riid, &IID_IDirectManipulationPrimaryContent))
    {
        struct primarycontext *primary;
        TRACE("IDirectManipulationPrimaryContent\n");
        primary = heap_alloc( sizeof(*primary));
        if(!primary)
            return E_OUTOFMEMORY;

        primary->IDirectManipulationPrimaryContent_iface.lpVtbl = &primaryVtbl;
        primary->IDirectManipulationContent_iface.lpVtbl = &contentVtbl;
        primary->ref = 1;

        *object = &primary->IDirectManipulationPrimaryContent_iface;

        return S_OK;
    }
    else
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_AddContent(IDirectManipulationViewport2 *iface, IDirectManipulationContent *content)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p\n", This, content);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_RemoveContent(IDirectManipulationViewport2 *iface, IDirectManipulationContent *content)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p\n", This, content);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetViewportOptions(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_VIEWPORT_OPTIONS options)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, options);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_AddConfiguration(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_CONFIGURATION configuration)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, configuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_RemoveConfiguration(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_CONFIGURATION configuration)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, configuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_ActivateConfiguration(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_CONFIGURATION configuration)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, configuration);
    return S_OK;
}

static HRESULT WINAPI viewport_SetManualGesture(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_GESTURE_CONFIGURATION configuration)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, configuration);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetChaining(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_MOTION_TYPES enabledTypes)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, enabledTypes);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_AddEventHandler(IDirectManipulationViewport2 *iface, HWND window,
                    IDirectManipulationViewportEventHandler *eventHandler, DWORD *cookie)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p, %p, %p\n", This, window, eventHandler, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_RemoveEventHandler(IDirectManipulationViewport2 *iface, DWORD cookie)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetInputMode(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_INPUT_MODE mode)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_SetUpdateMode(IDirectManipulationViewport2 *iface, DIRECTMANIPULATION_INPUT_MODE mode)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_Stop(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_Abandon(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_AddBehavior(IDirectManipulationViewport2 *iface, IUnknown *behavior, DWORD *cookie)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %p, %p\n", This, behavior, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_RemoveBehavior(IDirectManipulationViewport2 *iface, DWORD cookie)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p, %d\n", This, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI viewport_RemoveAllBehaviors(IDirectManipulationViewport2 *iface)
{
    struct directviewport *This = impl_from_IDirectManipulationViewport2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const IDirectManipulationViewport2Vtbl viewportVtbl =
{
    viewport_QueryInterface,
    viewport_AddRef,
    viewport_Release,
    viewport_Enable,
    viewport_Disable,
    viewport_SetContact,
    viewport_ReleaseContact,
    viewport_ReleaseAllContacts,
    viewport_GetStatus,
    viewport_GetTag,
    viewport_SetTag,
    viewport_GetViewportRect,
    viewport_SetViewportRect,
    viewport_ZoomToRect,
    viewport_SetViewportTransform,
    viewport_SyncDisplayTransform,
    viewport_GetPrimaryContent,
    viewport_AddContent,
    viewport_RemoveContent,
    viewport_SetViewportOptions,
    viewport_AddConfiguration,
    viewport_RemoveConfiguration,
    viewport_ActivateConfiguration,
    viewport_SetManualGesture,
    viewport_SetChaining,
    viewport_AddEventHandler,
    viewport_RemoveEventHandler,
    viewport_SetInputMode,
    viewport_SetUpdateMode,
    viewport_Stop,
    viewport_Abandon,
    viewport_AddBehavior,
    viewport_RemoveBehavior,
    viewport_RemoveAllBehaviors
};

static HRESULT create_viewport(IDirectManipulationViewport2 **obj)
{
    struct directviewport *object;

    object = heap_alloc(sizeof(*object));
    if(!object)
        return E_OUTOFMEMORY;

    object->IDirectManipulationViewport2_iface.lpVtbl = &viewportVtbl;
    object->ref = 1;

    *obj = &object->IDirectManipulationViewport2_iface;

    return S_OK;
}

static HRESULT WINAPI direct_manip_QueryInterface(IDirectManipulationManager2 *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationManager) ||
        IsEqualGUID(riid, &IID_IDirectManipulationManager2)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI direct_manip_AddRef(IDirectManipulationManager2 *iface)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI direct_manip_Release(IDirectManipulationManager2 *iface)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        if(This->updatemanager)
            IDirectManipulationUpdateManager_Release(This->updatemanager);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI direct_manip_Activate(IDirectManipulationManager2 *iface, HWND window)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_Deactivate(IDirectManipulationManager2 *iface, HWND window)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p\n", This, window);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_RegisterHitTestTarget(IDirectManipulationManager2 *iface, HWND window,
                HWND hittest, DIRECTMANIPULATION_HITTEST_TYPE type)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p, %p, %d\n", This, window, hittest, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_ProcessInput(IDirectManipulationManager2 *iface, const MSG *msg, BOOL *handled)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p, %p\n", This, msg, handled);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_GetUpdateManager(IDirectManipulationManager2 *iface, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    HRESULT hr = E_FAIL;

    TRACE("%p, %s, %p\n", This, debugstr_guid(riid), obj);

    *obj = NULL;
    if(IsEqualGUID(riid, &IID_IDirectManipulationUpdateManager))
    {
        if(!This->updatemanager)
        {
            hr = create_update_manager(&This->updatemanager);
        }
        else
        {
            hr = S_OK;
        }

        if(hr == S_OK)
        {
            IDirectManipulationUpdateManager_AddRef(This->updatemanager);
            *obj = This->updatemanager;
        }
    }
    else
        FIXME("Interface %s currently not supported.\n", debugstr_guid(riid));

    return hr;
}

static HRESULT WINAPI direct_manip_CreateViewport(IDirectManipulationManager2 *iface, IDirectManipulationFrameInfoProvider *frame,
                HWND window, REFIID riid, void **obj)
{
    HRESULT hr = E_NOTIMPL;
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    TRACE("%p, %p, %p, %s, %p\n", This, frame, window, debugstr_guid(riid), obj);

    if(IsEqualGUID(riid, &IID_IDirectManipulationViewport) ||
       IsEqualGUID(riid, &IID_IDirectManipulationViewport2) )
    {
        hr = create_viewport( (IDirectManipulationViewport2**)obj);
    }
    else
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
    return hr;
}

static HRESULT WINAPI direct_manip_CreateContent(IDirectManipulationManager2 *iface, IDirectManipulationFrameInfoProvider *frame,
                REFCLSID clsid, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %p,  %s, %p\n", This, frame, debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI direct_manip_CreateBehavior(IDirectManipulationManager2 *iface, REFCLSID clsid, REFIID riid, void **obj)
{
    struct directmanipulation *This = impl_from_IDirectManipulationManager2(iface);
    FIXME("%p, %s,  %s, %p\n", This, debugstr_guid(clsid), debugstr_guid(riid), obj);
    return E_NOTIMPL;
}

static const struct IDirectManipulationManager2Vtbl directmanipVtbl =
{
    direct_manip_QueryInterface,
    direct_manip_AddRef,
    direct_manip_Release,
    direct_manip_Activate,
    direct_manip_Deactivate,
    direct_manip_RegisterHitTestTarget,
    direct_manip_ProcessInput,
    direct_manip_GetUpdateManager,
    direct_manip_CreateViewport,
    direct_manip_CreateContent,
    direct_manip_CreateBehavior
};

static HRESULT WINAPI DirectManipulation_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct directmanipulation *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectManipulationManager2_iface.lpVtbl = &directmanipVtbl;
    object->ref = 1;

    ret = direct_manip_QueryInterface(&object->IDirectManipulationManager2_iface, riid, ppv);
    direct_manip_Release(&object->IDirectManipulationManager2_iface);

    return ret;
}

struct directcompositor
{
    IDirectManipulationCompositor2 IDirectManipulationCompositor2_iface;
    IDirectManipulationFrameInfoProvider IDirectManipulationFrameInfoProvider_iface;
    IDirectManipulationUpdateManager *manager;
    LONG ref;
};

static inline struct directcompositor *impl_from_IDirectManipulationCompositor2(IDirectManipulationCompositor2 *iface)
{
    return CONTAINING_RECORD(iface, struct directcompositor, IDirectManipulationCompositor2_iface);
}

static inline struct directcompositor *impl_from_IDirectManipulationFrameInfoProvider(IDirectManipulationFrameInfoProvider *iface)
{
    return CONTAINING_RECORD(iface, struct directcompositor, IDirectManipulationFrameInfoProvider_iface);
}

static HRESULT WINAPI compositor_QueryInterface(IDirectManipulationCompositor2 *iface, REFIID riid, void **ppv)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectManipulationCompositor) ||
        IsEqualGUID(riid, &IID_IDirectManipulationCompositor2))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }
    else if(IsEqualGUID(riid, &IID_IDirectManipulationFrameInfoProvider))
    {
        IUnknown_AddRef(iface);
        *ppv = &This->IDirectManipulationFrameInfoProvider_iface;
        return S_OK;
    }

    FIXME("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI compositor_AddRef(IDirectManipulationCompositor2 *iface)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI compositor_Release(IDirectManipulationCompositor2 *iface)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        if(This->manager)
            IDirectManipulationUpdateManager_Release(This->manager);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI compositor_AddContent(IDirectManipulationCompositor2 *iface, IDirectManipulationContent *content,
                IUnknown *device, IUnknown *parent, IUnknown *child)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    FIXME("%p, %p, %p, %p, %p\n", This, content, device, parent, child);
    return E_NOTIMPL;
}

static HRESULT WINAPI compositor_RemoveContent(IDirectManipulationCompositor2 *iface, IDirectManipulationContent *content)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    FIXME("%p, %p\n", This, content);
    return E_NOTIMPL;
}

static HRESULT WINAPI compositor_SetUpdateManager(IDirectManipulationCompositor2 *iface, IDirectManipulationUpdateManager *manager)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    TRACE("%p, %p\n", This, manager);

    if(!manager)
        return E_INVALIDARG;

    This->manager = manager;
    IDirectManipulationUpdateManager_AddRef(This->manager);
    return S_OK;
}

static HRESULT WINAPI compositor_Flush(IDirectManipulationCompositor2 *iface)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI compositor_AddContentWithCrossProcessChaining(IDirectManipulationCompositor2 *iface,
                IDirectManipulationPrimaryContent *content, IUnknown *device, IUnknown *parentvisual, IUnknown *childvisual)
{
    struct directcompositor *This = impl_from_IDirectManipulationCompositor2(iface);
    FIXME("%p %p %p %p %p\n", This, content, device, parentvisual, childvisual);
    return E_NOTIMPL;
}

static const struct IDirectManipulationCompositor2Vtbl compositorVtbl =
{
    compositor_QueryInterface,
    compositor_AddRef,
    compositor_Release,
    compositor_AddContent,
    compositor_RemoveContent,
    compositor_SetUpdateManager,
    compositor_Flush,
    compositor_AddContentWithCrossProcessChaining
};

static HRESULT WINAPI provider_QueryInterface(IDirectManipulationFrameInfoProvider *iface, REFIID riid, void **ppv)
{
    struct directcompositor *This = impl_from_IDirectManipulationFrameInfoProvider(iface);
    return IDirectManipulationCompositor2_QueryInterface(&This->IDirectManipulationCompositor2_iface, riid, ppv);
}

static ULONG WINAPI provider_AddRef(IDirectManipulationFrameInfoProvider *iface)
{
    struct directcompositor *This = impl_from_IDirectManipulationFrameInfoProvider(iface);
    return IDirectManipulationCompositor2_AddRef(&This->IDirectManipulationCompositor2_iface);
}

static ULONG WINAPI provider_Release(IDirectManipulationFrameInfoProvider *iface)
{
    struct directcompositor *This = impl_from_IDirectManipulationFrameInfoProvider(iface);
    return IDirectManipulationCompositor2_Release(&This->IDirectManipulationCompositor2_iface);
}

static HRESULT WINAPI provider_GetNextFrameInfo(IDirectManipulationFrameInfoProvider *iface, ULONGLONG *time,
            ULONGLONG *process, ULONGLONG *composition)
{
    struct directcompositor *This = impl_from_IDirectManipulationFrameInfoProvider(iface);
    FIXME("%p, %p, %p, %p\n", This, time, process, composition);
    return E_NOTIMPL;
}

static const struct IDirectManipulationFrameInfoProviderVtbl providerVtbl =
{
    provider_QueryInterface,
    provider_AddRef,
    provider_Release,
    provider_GetNextFrameInfo
};

static HRESULT WINAPI DirectCompositor_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct directcompositor *object;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectManipulationCompositor2_iface.lpVtbl = &compositorVtbl;
    object->IDirectManipulationFrameInfoProvider_iface.lpVtbl = &providerVtbl;
    object->ref = 1;

    ret = compositor_QueryInterface(&object->IDirectManipulationCompositor2_iface, riid, ppv);
    compositor_Release(&object->IDirectManipulationCompositor2_iface);

    return ret;
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IClassFactory, riid)) {
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl DirectFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    DirectManipulation_CreateInstance,
    ClassFactory_LockServer
};

static const IClassFactoryVtbl DirectCompositorVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    DirectCompositor_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory direct_factory = { &DirectFactoryVtbl };
static IClassFactory compositor_factory = { &DirectCompositorVtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_DirectManipulationManager, rclsid) ||
       IsEqualGUID(&CLSID_DirectManipulationSharedManager, rclsid) ) {
        TRACE("(CLSID_DirectManipulationManager %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&direct_factory, riid, ppv);
    }
    else if(IsEqualGUID(&CLSID_DCompManipulationCompositor, rclsid)) {
        TRACE("(CLSID_DCompManipulationCompositor %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&compositor_factory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
