/*
 *    Row and rowset servers / proxies.
 *
 * Copyright 2010 Huw Davies
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
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "oledb.h"

#include "row_server.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

HRESULT create_row_server(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}

HRESULT create_rowset_server(IUnknown *outer, void **obj)
{
    FIXME("(%p, %p): stub\n", outer, obj);
    *obj = NULL;
    return E_NOTIMPL;
}

/* Marshal impl */

typedef struct
{
    const IMarshalVtbl *marshal_vtbl;

    LONG ref;
    CLSID unmarshal_class;
    IUnknown *outer;
} marshal;

static inline marshal *impl_from_IMarshal(IMarshal *iface)
{
    return (marshal *)((char*)iface - FIELD_OFFSET(marshal, marshal_vtbl));
}

static HRESULT WINAPI marshal_QueryInterface(IMarshal *iface, REFIID iid, void **obj)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), obj);

    if(IsEqualIID(iid, &IID_IUnknown) ||
       IsEqualIID(iid, &IID_IMarshal))
    {
        *obj = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IMarshal_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI marshal_AddRef(IMarshal *iface)
{
    marshal *This = impl_from_IMarshal(iface);
    TRACE("(%p)\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI marshal_Release(IMarshal *iface)
{
    marshal *This = impl_from_IMarshal(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI marshal_GetUnmarshalClass(IMarshal *iface, REFIID iid, void *obj,
                                                DWORD dwDestContext, void *pvDestContext,
                                                DWORD mshlflags, CLSID *clsid)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%s, %p, %08x, %p, %08x, %p): stub\n", This, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags, clsid);

    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_GetMarshalSizeMax(IMarshal *iface, REFIID iid, void *obj,
                                                DWORD dwDestContext, void *pvDestContext,
                                                DWORD mshlflags, DWORD *size)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%s, %p, %08x, %p, %08x, %p): stub\n", This, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID iid,
                                               void *obj, DWORD dwDestContext, void *pvDestContext,
                                               DWORD mshlflags)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%p, %s, %p, %08x, %p, %08x): stub\n", This, stream, debugstr_guid(iid), obj, dwDestContext,
          pvDestContext, mshlflags);

    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_UnmarshalInterface(IMarshal *iface, IStream *stream,
                                                 REFIID iid, void **obj)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%p, %s, %p): stub\n", This, stream, debugstr_guid(iid), obj);
    *obj = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%p): stub\n", This, stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI marshal_DisconnectObject(IMarshal *iface, DWORD dwReserved)
{
    marshal *This = impl_from_IMarshal(iface);
    FIXME("(%p)->(%08x)\n", This, dwReserved);

    return E_NOTIMPL;
}

static const IMarshalVtbl marshal_vtbl =
{
    marshal_QueryInterface,
    marshal_AddRef,
    marshal_Release,
    marshal_GetUnmarshalClass,
    marshal_GetMarshalSizeMax,
    marshal_MarshalInterface,
    marshal_UnmarshalInterface,
    marshal_ReleaseMarshalData,
    marshal_DisconnectObject
};

static HRESULT create_marshal(IUnknown *outer, const CLSID *class, void **obj)
{
    marshal *marshal;

    TRACE("(%p, %p)\n", outer, obj);
    *obj = NULL;

    marshal = HeapAlloc(GetProcessHeap(), 0, sizeof(*marshal));
    if(!marshal) return E_OUTOFMEMORY;

    marshal->unmarshal_class = *class;
    marshal->outer = outer; /* don't ref outer unk */
    marshal->marshal_vtbl = &marshal_vtbl;
    marshal->ref = 1;

    *obj = &marshal->marshal_vtbl;
    TRACE("returing %p\n", *obj);
    return S_OK;
}

HRESULT create_row_marshal(IUnknown *outer, void **obj)
{
    TRACE("(%p, %p)\n", outer, obj);
    return create_marshal(outer, &CLSID_wine_row_proxy, obj);
}

HRESULT create_rowset_marshal(IUnknown *outer, void **obj)
{
    TRACE("(%p, %p)\n", outer, obj);
    return create_marshal(outer, &CLSID_wine_rowset_proxy, obj);
}
