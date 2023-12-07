/*
 * Default allocator-presenter for the VMR7
 *
 * Copyright 2023 Zebediah Figura for CodeWeavers
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

#include "quartz_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct vmr7_presenter
{
    IVMRImagePresenter IVMRImagePresenter_iface;
    IVMRSurfaceAllocator IVMRSurfaceAllocator_iface;
    LONG refcount;
};

static struct vmr7_presenter *impl_from_IVMRImagePresenter(IVMRImagePresenter *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7_presenter, IVMRImagePresenter_iface);
}

static HRESULT WINAPI image_presenter_QueryInterface(IVMRImagePresenter *iface, REFIID iid, void **out)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);

    TRACE("presenter %p, iid %s, out %p.\n", presenter, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IVMRImagePresenter))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IVMRSurfaceAllocator))
        *out = &presenter->IVMRSurfaceAllocator_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI image_presenter_AddRef(IVMRImagePresenter *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);
    ULONG refcount = InterlockedIncrement(&presenter->refcount);

    TRACE("%p increasing refcount to %lu.\n", presenter, refcount);
    return refcount;
}

static ULONG WINAPI image_presenter_Release(IVMRImagePresenter *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);
    ULONG refcount = InterlockedDecrement(&presenter->refcount);

    TRACE("%p decreasing refcount to %lu.\n", presenter, refcount);
    if (!refcount)
        free(presenter);
    return refcount;
}

static HRESULT WINAPI image_presenter_StartPresenting(IVMRImagePresenter *iface, DWORD_PTR cookie)
{
    FIXME("iface %p, cookie %#Ix, stub!\n", iface, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI image_presenter_StopPresenting(IVMRImagePresenter *iface, DWORD_PTR cookie)
{
    FIXME("iface %p, cookie %#Ix, stub!\n", iface, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI image_presenter_PresentImage(IVMRImagePresenter *iface,
        DWORD_PTR cookie, VMRPRESENTATIONINFO *info)
{
    FIXME("iface %p, cookie %#Ix, info %p, stub!\n", iface, cookie, info);
    return E_NOTIMPL;
}

static const IVMRImagePresenterVtbl image_presenter_vtbl =
{
    image_presenter_QueryInterface,
    image_presenter_AddRef,
    image_presenter_Release,
    image_presenter_StartPresenting,
    image_presenter_StopPresenting,
    image_presenter_PresentImage,
};

static struct vmr7_presenter *impl_from_IVMRSurfaceAllocator(IVMRSurfaceAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7_presenter, IVMRSurfaceAllocator_iface);
}

static HRESULT WINAPI surface_allocator_QueryInterface(IVMRSurfaceAllocator *iface, REFIID iid, void **out)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_QueryInterface(&presenter->IVMRImagePresenter_iface, iid, out);
}

static ULONG WINAPI surface_allocator_AddRef(IVMRSurfaceAllocator *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_AddRef(&presenter->IVMRImagePresenter_iface);
}

static ULONG WINAPI surface_allocator_Release(IVMRSurfaceAllocator *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_Release(&presenter->IVMRImagePresenter_iface);
}

static HRESULT WINAPI surface_allocator_AllocateSurface(IVMRSurfaceAllocator *iface,
        DWORD_PTR id, VMRALLOCATIONINFO *info, DWORD *count, IDirectDrawSurface7 **surfaces)
{
    FIXME("iface %p, id %#Ix, info %p, count %p, surfaces %p, stub!\n", iface, id, info, count, surfaces);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_FreeSurface(IVMRSurfaceAllocator *iface, DWORD_PTR id)
{
    FIXME("iface %p, id %#Ix, stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_PrepareSurface(IVMRSurfaceAllocator *iface,
        DWORD_PTR id, IDirectDrawSurface7 *surface, DWORD flags)
{
    FIXME("iface %p, id %#Ix, surface %p, flags %#lx, stub!\n", iface, id, surface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_AdviseNotify(IVMRSurfaceAllocator *iface,
        IVMRSurfaceAllocatorNotify *notify)
{
    FIXME("iface %p, notify %p, stub!\n", iface, notify);
    return S_OK;
}

static const IVMRSurfaceAllocatorVtbl surface_allocator_vtbl =
{
    surface_allocator_QueryInterface,
    surface_allocator_AddRef,
    surface_allocator_Release,
    surface_allocator_AllocateSurface,
    surface_allocator_FreeSurface,
    surface_allocator_PrepareSurface,
    surface_allocator_AdviseNotify,
};

HRESULT vmr7_presenter_create(IUnknown *outer, IUnknown **out)
{
    struct vmr7_presenter *object;

    TRACE("outer %p, out %p.\n", outer, out);

    if (outer)
        FIXME("Ignoring outer %p.\n", outer);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;
    object->IVMRImagePresenter_iface.lpVtbl = &image_presenter_vtbl;
    object->IVMRSurfaceAllocator_iface.lpVtbl = &surface_allocator_vtbl;
    object->refcount = 1;

    TRACE("Created VMR7 default presenter %p.\n", object);
    *out = (IUnknown *)&object->IVMRSurfaceAllocator_iface;
    return S_OK;
}
