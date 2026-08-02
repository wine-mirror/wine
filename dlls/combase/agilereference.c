/*
 *    Copyright 2024 Onni Kukkonen
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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define USE_COM_CONTEXT_DEF
#include "objbase.h"
#include "ctxtcall.h"
#include "oleauto.h"
#include "dde.h"
#include "winternl.h"

#include "combase_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

struct agileref {
    IAgileReference IAgileReference_iface;
    IUnknown *storedObject;
    IID impl_iid;
    LONG ref;
};

static inline struct agileref *impl_from_IAgileReference(IAgileReference *iface)
{
    return CONTAINING_RECORD(iface, struct agileref, IAgileReference_iface);
}

static HRESULT WINAPI agileref_QueryInterface( IAgileReference *iface, REFIID iid, void **out )
{
    struct agileref *impl = impl_from_IAgileReference( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IAgileReference ))
    {
        *out = &impl->IAgileReference_iface;
        return S_OK;
    }

    if (IsEqualGUID( iid, &impl->impl_iid )) {
        *out = impl->storedObject;
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI agileref_AddRef( IAgileReference *iface )
{
    struct agileref *impl = impl_from_IAgileReference( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI agileref_Release( IAgileReference *iface )
{
    struct agileref *impl = impl_from_IAgileReference( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref) free( impl );
    return ref;
}

static HRESULT WINAPI agileref_Resolve( IAgileReference *iface, REFIID riid, void **object_reference )
{
    struct agileref *impl = impl_from_IAgileReference( iface );

    FIXME( "iface %p, riid %s, object_reference %p semi-stub!\n", iface, debugstr_guid(riid), object_reference);
    if (impl == NULL || impl->storedObject == NULL || impl->storedObject->lpVtbl == NULL || impl->storedObject->lpVtbl->QueryInterface == NULL) {
        ERR("NULL stored object pointer\n");
        return E_POINTER;
    }

    return impl->storedObject->lpVtbl->QueryInterface(impl->storedObject, riid, object_reference);
}

static const struct IAgileReferenceVtbl agileref_vtbl =
{
    agileref_QueryInterface,
    agileref_AddRef,
    agileref_Release,
    /* IAgileReference methods */
    agileref_Resolve,
};


HRESULT create_agile_reference(REFIID iid, IUnknown *unk, IAgileReference **agile_reference) {
    HRESULT ret;
    struct agileref *object;
    
    FIXME("iid %s, unk %p, agile_reference %p.\n", debugstr_guid(iid), unk, agile_reference);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;
    
    object->IAgileReference_iface.lpVtbl = &agileref_vtbl;
    object->impl_iid = *iid;
    ret = IUnknown_QueryInterface(unk, iid, (void**)&object->storedObject);
    if (!SUCCEEDED(ret)) {
        ERR("Failed to query\n");
        return ret;
    }

    object->ref = 1;

    *agile_reference = &object->IAgileReference_iface;
    return S_OK;
}
