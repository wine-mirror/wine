/*
 *     Copyright 2002 Juergen Schmied
 *     Copyright 2002 Marcus Meissner
 *     Copyright 2004 Mike Hearn, for CodeWeavers
 *     Copyright 2004 Rob Shearman, for CodeWeavers
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
#include "objbase.h"

#include "combase_private.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HRESULT WINAPI unmarshal_object(const STDOBJREF *stdobjref, struct apartment *apt,
                                MSHCTX dest_context, void *dest_context_data,
                                REFIID riid, const OXID_INFO *oxid_info,
                                void **object);

/* private flag indicating that the object was marshaled as table-weak */
#define SORFP_TABLEWEAK SORF_OXRES1

struct ftmarshaler
{
    IUnknown IUnknown_inner;
    IMarshal IMarshal_iface;
    IUnknown *outer_unk;
    LONG refcount;
};

static struct ftmarshaler *impl_ft_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct ftmarshaler, IUnknown_inner);
}

static struct ftmarshaler *impl_ft_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, struct ftmarshaler, IMarshal_iface);
}

/***********************************************************************
 *            CoMarshalHresult        (combase.@)
 */
HRESULT WINAPI CoMarshalHresult(IStream *stream, HRESULT hresult)
{
    return IStream_Write(stream, &hresult, sizeof(hresult), NULL);
}

/***********************************************************************
 *            CoUnmarshalHresult      (combase.@)
 */
HRESULT WINAPI CoUnmarshalHresult(IStream *stream, HRESULT *phresult)
{
    return IStream_Read(stream, phresult, sizeof(*phresult), NULL);
}

/***********************************************************************
 *            CoGetInterfaceAndReleaseStream    (combase.@)
 */
HRESULT WINAPI CoGetInterfaceAndReleaseStream(IStream *stream, REFIID riid, void **obj)
{
    HRESULT hr;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), obj);

    if (!stream) return E_INVALIDARG;
    hr = CoUnmarshalInterface(stream, riid, obj);
    IStream_Release(stream);
    return hr;
}

/***********************************************************************
 *            CoMarshalInterThreadInterfaceInStream    (combase.@)
 */
HRESULT WINAPI CoMarshalInterThreadInterfaceInStream(REFIID riid, IUnknown *unk, IStream **stream)
{
    ULARGE_INTEGER xpos;
    LARGE_INTEGER seekto;
    HRESULT hr;

    TRACE("%s, %p, %p\n", debugstr_guid(riid), unk, stream);

    hr = CreateStreamOnHGlobal(NULL, TRUE, stream);
    if (FAILED(hr)) return hr;
    hr = CoMarshalInterface(*stream, riid, unk, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);

    if (SUCCEEDED(hr))
    {
        memset(&seekto, 0, sizeof(seekto));
        IStream_Seek(*stream, seekto, STREAM_SEEK_SET, &xpos);
    }
    else
    {
        IStream_Release(*stream);
        *stream = NULL;
    }

    return hr;
}

static HRESULT WINAPI ftmarshaler_inner_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);

    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(&IID_IUnknown, riid))
        *obj = &marshaler->IUnknown_inner;
    else if (IsEqualIID(&IID_IMarshal, riid))
        *obj = &marshaler->IMarshal_iface;
    else
    {
        FIXME("No interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);

    return S_OK;
}

static ULONG WINAPI ftmarshaler_inner_AddRef(IUnknown *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&marshaler->refcount);

    TRACE("%p, refcount %u\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ftmarshaler_inner_Release(IUnknown *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&marshaler->refcount);

    TRACE("%p, refcount %u\n", iface, refcount);

    if (!refcount)
        heap_free(marshaler);

    return refcount;
}

static const IUnknownVtbl ftmarshaler_inner_vtbl =
{
    ftmarshaler_inner_QueryInterface,
    ftmarshaler_inner_AddRef,
    ftmarshaler_inner_Release
};

static HRESULT WINAPI ftmarshaler_QueryInterface(IMarshal *iface, REFIID riid, void **obj)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), obj);

    return IUnknown_QueryInterface(marshaler->outer_unk, riid, obj);
}

static ULONG WINAPI ftmarshaler_AddRef(IMarshal *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p\n", iface);

    return IUnknown_AddRef(marshaler->outer_unk);
}

static ULONG WINAPI ftmarshaler_Release(IMarshal *iface)
{
    struct ftmarshaler *marshaler = impl_ft_from_IMarshal(iface);

    TRACE("%p\n", iface);

    return IUnknown_Release(marshaler->outer_unk);
}

static HRESULT WINAPI ftmarshaler_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dest_context, void *pvDestContext, DWORD mshlflags, CLSID *clsid)
{
    TRACE("%s, %p, %#x, %p, %#x, %p\n", debugstr_guid(riid), pv, dest_context, pvDestContext, mshlflags, clsid);

    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
        *clsid = CLSID_InProcFreeMarshaler;
    else
        *clsid = CLSID_StdMarshal;

    return S_OK;
}

static HRESULT WINAPI ftmarshaler_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dest_context, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    IMarshal *marshal = NULL;
    HRESULT hr;

    TRACE("%s, %p, %#x, %p, %#x, %p\n", debugstr_guid(riid), pv, dest_context, pvDestContext, mshlflags, size);

    /* If the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
    {
        *size = sizeof(mshlflags) + sizeof(pv) + sizeof(DWORD) + sizeof(GUID);
        return S_OK;
    }

    /* Use the standard marshaller to handle all other cases */
    CoGetStandardMarshal(riid, pv, dest_context, pvDestContext, mshlflags, &marshal);
    hr = IMarshal_GetMarshalSizeMax(marshal, riid, pv, dest_context, pvDestContext, mshlflags, size);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI ftmarshaler_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid,
        void *pv, DWORD dest_context, void *pvDestContext, DWORD mshlflags)
{
    IMarshal *marshal = NULL;
    HRESULT hr;

    TRACE("%p, %s, %p, %#x, %p, %#x\n", stream, debugstr_guid(riid), pv,
            dest_context, pvDestContext, mshlflags);

    /* If the marshalling happens inside the same process the interface pointer is
       copied between the apartments */
    if (dest_context == MSHCTX_INPROC || dest_context == MSHCTX_CROSSCTX)
    {
        void *object;
        DWORD constant = 0;
        GUID unknown_guid = { 0 };

        hr = IUnknown_QueryInterface((IUnknown *)pv, riid, &object);
        if (FAILED(hr))
            return hr;

        /* don't hold a reference to table-weak marshaled interfaces */
        if (mshlflags & MSHLFLAGS_TABLEWEAK)
            IUnknown_Release((IUnknown *)object);

        hr = IStream_Write(stream, &mshlflags, sizeof(mshlflags), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        hr = IStream_Write(stream, &object, sizeof(object), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        if (sizeof(object) == sizeof(DWORD))
        {
            hr = IStream_Write(stream, &constant, sizeof(constant), NULL);
            if (hr != S_OK) return STG_E_MEDIUMFULL;
        }

        hr = IStream_Write(stream, &unknown_guid, sizeof(unknown_guid), NULL);
        if (hr != S_OK) return STG_E_MEDIUMFULL;

        return S_OK;
    }

    /* Use the standard marshaler to handle all other cases */
    CoGetStandardMarshal(riid, pv, dest_context, pvDestContext, mshlflags, &marshal);
    hr = IMarshal_MarshalInterface(marshal, stream, riid, pv, dest_context, pvDestContext, mshlflags);
    IMarshal_Release(marshal);
    return hr;
}

static HRESULT WINAPI ftmarshaler_UnmarshalInterface(IMarshal *iface, IStream *stream, REFIID riid, void **ppv)
{
    DWORD mshlflags;
    IUnknown *object;
    DWORD constant;
    GUID unknown_guid;
    HRESULT hr;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), ppv);

    hr = IStream_Read(stream, &mshlflags, sizeof(mshlflags), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &object, sizeof(object), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    if (sizeof(object) == sizeof(DWORD))
    {
        hr = IStream_Read(stream, &constant, sizeof(constant), NULL);
        if (hr != S_OK) return STG_E_READFAULT;
        if (constant != 0)
            FIXME("constant is 0x%x instead of 0\n", constant);
    }

    hr = IStream_Read(stream, &unknown_guid, sizeof(unknown_guid), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IUnknown_QueryInterface(object, riid, ppv);
    if (!(mshlflags & (MSHLFLAGS_TABLEWEAK | MSHLFLAGS_TABLESTRONG)))
        IUnknown_Release(object);

    return hr;
}

static HRESULT WINAPI ftmarshaler_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    DWORD mshlflags;
    IUnknown *object;
    DWORD constant;
    GUID unknown_guid;
    HRESULT hr;

    TRACE("%p\n", stream);

    hr = IStream_Read(stream, &mshlflags, sizeof(mshlflags), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(stream, &object, sizeof(object), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    if (sizeof(object) == sizeof(DWORD))
    {
        hr = IStream_Read(stream, &constant, sizeof(constant), NULL);
        if (hr != S_OK) return STG_E_READFAULT;
        if (constant != 0)
            FIXME("constant is 0x%x instead of 0\n", constant);
    }

    hr = IStream_Read(stream, &unknown_guid, sizeof(unknown_guid), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    IUnknown_Release(object);
    return S_OK;
}

static HRESULT WINAPI ftmarshaler_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    TRACE("\n");

    /* nothing to do */
    return S_OK;
}

static const IMarshalVtbl ftmarshaler_vtbl =
{
    ftmarshaler_QueryInterface,
    ftmarshaler_AddRef,
    ftmarshaler_Release,
    ftmarshaler_GetUnmarshalClass,
    ftmarshaler_GetMarshalSizeMax,
    ftmarshaler_MarshalInterface,
    ftmarshaler_UnmarshalInterface,
    ftmarshaler_ReleaseMarshalData,
    ftmarshaler_DisconnectObject
};

/***********************************************************************
 *          CoCreateFreeThreadedMarshaler    (combase.@)
 */
HRESULT WINAPI CoCreateFreeThreadedMarshaler(IUnknown *outer, IUnknown **marshaler)
{
    struct ftmarshaler *object;

    TRACE("%p, %p\n", outer, marshaler);

    object = heap_alloc(sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IUnknown_inner.lpVtbl = &ftmarshaler_inner_vtbl;
    object->IMarshal_iface.lpVtbl = &ftmarshaler_vtbl;
    object->refcount = 1;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;

    *marshaler = &object->IUnknown_inner;

    return S_OK;
}

/***********************************************************************
 *            CoGetMarshalSizeMax        (combase.@)
 */
HRESULT WINAPI CoGetMarshalSizeMax(ULONG *size, REFIID riid, IUnknown *unk,
         DWORD dest_context, void *pvDestContext, DWORD mshlFlags)
{
    BOOL std_marshal = FALSE;
    IMarshal *marshal;
    HRESULT hr;

    if (!unk)
        return E_POINTER;

    hr = IUnknown_QueryInterface(unk, &IID_IMarshal, (void **)&marshal);
    if (hr != S_OK)
    {
        std_marshal = TRUE;
        hr = CoGetStandardMarshal(riid, unk, dest_context, pvDestContext, mshlFlags, &marshal);
    }
    if (hr != S_OK)
        return hr;

    hr = IMarshal_GetMarshalSizeMax(marshal, riid, unk, dest_context, pvDestContext, mshlFlags, size);
    if (!std_marshal)
        /* add on the size of the whole OBJREF structure like native does */
        *size += sizeof(OBJREF);

    IMarshal_Release(marshal);
    return hr;
}

static void dump_mshflags(MSHLFLAGS flags)
{
    if (flags & MSHLFLAGS_TABLESTRONG)
        TRACE(" MSHLFLAGS_TABLESTRONG");
    if (flags & MSHLFLAGS_TABLEWEAK)
        TRACE(" MSHLFLAGS_TABLEWEAK");
    if (!(flags & (MSHLFLAGS_TABLESTRONG|MSHLFLAGS_TABLEWEAK)))
        TRACE(" MSHLFLAGS_NORMAL");
    if (flags & MSHLFLAGS_NOPING)
        TRACE(" MSHLFLAGS_NOPING");
}

/***********************************************************************
 *        CoMarshalInterface    (combase.@)
 */
HRESULT WINAPI CoMarshalInterface(IStream *stream, REFIID riid, IUnknown *unk,
        DWORD dest_context, void *pvDestContext, DWORD mshlFlags)
{
    CLSID marshaler_clsid;
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p, %s, %p, %x, %p, ", stream, debugstr_guid(riid), unk, dest_context, pvDestContext);
    dump_mshflags(mshlFlags);
    TRACE("\n");

    if (!unk || !stream)
        return E_INVALIDARG;

    hr = IUnknown_QueryInterface(unk, &IID_IMarshal, (void **)&marshal);
    if (hr != S_OK)
        hr = CoGetStandardMarshal(riid, unk, dest_context, pvDestContext, mshlFlags, &marshal);
    if (hr != S_OK)
    {
        ERR("Failed to get marshaller, %#x\n", hr);
        return hr;
    }

    hr = IMarshal_GetUnmarshalClass(marshal, riid, unk, dest_context, pvDestContext, mshlFlags,
            &marshaler_clsid);
    if (hr != S_OK)
    {
        ERR("IMarshal::GetUnmarshalClass failed, %#x\n", hr);
        goto cleanup;
    }

    /* FIXME: implement handler marshaling too */
    if (IsEqualCLSID(&marshaler_clsid, &CLSID_StdMarshal))
    {
        TRACE("Using standard marshaling\n");
    }
    else
    {
        OBJREF objref;

        TRACE("Using custom marshaling\n");
        objref.signature = OBJREF_SIGNATURE;
        objref.iid = *riid;
        objref.flags = OBJREF_CUSTOM;
        objref.u_objref.u_custom.clsid = marshaler_clsid;
        objref.u_objref.u_custom.cbExtension = 0;
        objref.u_objref.u_custom.size = 0;
        hr = IMarshal_GetMarshalSizeMax(marshal, riid, unk, dest_context, pvDestContext, mshlFlags,
                &objref.u_objref.u_custom.size);
        if (hr != S_OK)
        {
            ERR("Failed to get max size of marshal data, error %#x\n", hr);
            goto cleanup;
        }
        /* write constant sized common header and OR_CUSTOM data into stream */
        hr = IStream_Write(stream, &objref, FIELD_OFFSET(OBJREF, u_objref.u_custom.pData), NULL);
        if (hr != S_OK)
        {
            ERR("Failed to write OR_CUSTOM header to stream with %#x\n", hr);
            goto cleanup;
        }
    }

    TRACE("Calling IMarshal::MarshalInterface\n");

    hr = IMarshal_MarshalInterface(marshal, stream, riid, unk, dest_context, pvDestContext, mshlFlags);
    if (hr != S_OK)
    {
        ERR("Failed to marshal the interface %s, hr %#x\n", debugstr_guid(riid), hr);
        goto cleanup;
    }

cleanup:
    IMarshal_Release(marshal);

    TRACE("completed with hr %#x\n", hr);

    return hr;
}

/* Creates an IMarshal* object according to the data marshaled to the stream.
 * The function leaves the stream pointer at the start of the data written
 * to the stream by the IMarshal* object.
 */
static HRESULT get_unmarshaler_from_stream(IStream *stream, IMarshal **marshal, IID *iid)
{
    OBJREF objref;
    HRESULT hr;
    ULONG res;

    /* read common OBJREF header */
    hr = IStream_Read(stream, &objref, FIELD_OFFSET(OBJREF, u_objref), &res);
    if (hr != S_OK || (res != FIELD_OFFSET(OBJREF, u_objref)))
    {
        ERR("Failed to read common OBJREF header, 0x%08x\n", hr);
        return STG_E_READFAULT;
    }

    /* sanity check on header */
    if (objref.signature != OBJREF_SIGNATURE)
    {
        ERR("Bad OBJREF signature 0x%08x\n", objref.signature);
        return RPC_E_INVALID_OBJREF;
    }

    if (iid) *iid = objref.iid;

    /* FIXME: handler marshaling */
    if (objref.flags & OBJREF_STANDARD)
    {
        TRACE("Using standard unmarshaling\n");
        *marshal = NULL;
        return S_FALSE;
    }
    else if (objref.flags & OBJREF_CUSTOM)
    {
        ULONG custom_header_size = FIELD_OFFSET(OBJREF, u_objref.u_custom.pData) -
                                   FIELD_OFFSET(OBJREF, u_objref.u_custom);
        TRACE("Using custom unmarshaling\n");
        /* read constant sized OR_CUSTOM data from stream */
        hr = IStream_Read(stream, &objref.u_objref.u_custom,
                          custom_header_size, &res);
        if (hr != S_OK || (res != custom_header_size))
        {
            ERR("Failed to read OR_CUSTOM header, 0x%08x\n", hr);
            return STG_E_READFAULT;
        }
        /* now create the marshaler specified in the stream */
        hr = CoCreateInstance(&objref.u_objref.u_custom.clsid, NULL,
                              CLSCTX_INPROC_SERVER, &IID_IMarshal,
                              (LPVOID*)marshal);
    }
    else
    {
        FIXME("Invalid or unimplemented marshaling type specified: %x\n",
            objref.flags);
        return RPC_E_INVALID_OBJREF;
    }

    if (hr != S_OK)
        ERR("Failed to create marshal, 0x%08x\n", hr);

    return hr;
}

static HRESULT std_release_marshal_data(IStream *stream)
{
    struct stub_manager *stubmgr;
    struct OR_STANDARD  obj;
    struct apartment *apt;
    ULONG res;
    HRESULT hr;

    hr = IStream_Read(stream, &obj, FIELD_OFFSET(struct OR_STANDARD, saResAddr.aStringArray), &res);
    if (hr != S_OK) return STG_E_READFAULT;

    if (obj.saResAddr.wNumEntries)
    {
        ERR("unsupported size of DUALSTRINGARRAY\n");
        return E_NOTIMPL;
    }

    TRACE("oxid = %s, oid = %s, ipid = %s\n", wine_dbgstr_longlong(obj.std.oxid),
            wine_dbgstr_longlong(obj.std.oid), wine_dbgstr_guid(&obj.std.ipid));

    if (!(apt = apartment_findfromoxid(obj.std.oxid)))
    {
        WARN("Could not map OXID %s to apartment object\n",
            wine_dbgstr_longlong(obj.std.oxid));
        return RPC_E_INVALID_OBJREF;
    }

    if (!(stubmgr = get_stub_manager(apt, obj.std.oid)))
    {
        apartment_release(apt);
        ERR("could not map object ID to stub manager, oxid=%s, oid=%s\n",
            wine_dbgstr_longlong(obj.std.oxid), wine_dbgstr_longlong(obj.std.oid));
        return RPC_E_INVALID_OBJREF;
    }

    stub_manager_release_marshal_data(stubmgr, obj.std.cPublicRefs, &obj.std.ipid, obj.std.flags & SORFP_TABLEWEAK);

    stub_manager_int_release(stubmgr);
    apartment_release(apt);

    return S_OK;
}

/***********************************************************************
 *            CoReleaseMarshalData        (combase.@)
 */
HRESULT WINAPI CoReleaseMarshalData(IStream *stream)
{
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p\n", stream);

    hr = get_unmarshaler_from_stream(stream, &marshal, NULL);
    if (hr == S_FALSE)
    {
        hr = std_release_marshal_data(stream);
        if (hr != S_OK)
            ERR("StdMarshal ReleaseMarshalData failed with error %#x\n", hr);
        return hr;
    }
    if (hr != S_OK)
        return hr;

    /* call the helper object to do the releasing of marshal data */
    hr = IMarshal_ReleaseMarshalData(marshal, stream);
    if (hr != S_OK)
        ERR("IMarshal::ReleaseMarshalData failed with error %#x\n", hr);

    IMarshal_Release(marshal);
    return hr;
}

static HRESULT std_unmarshal_interface(MSHCTX dest_context, void *dest_context_data,
        IStream *stream, REFIID riid, void **ppv)
{
    struct stub_manager *stubmgr = NULL;
    struct OR_STANDARD obj;
    ULONG res;
    HRESULT hres;
    struct apartment *apt, *stub_apt;

    TRACE("(...,%s,....)\n", debugstr_guid(riid));

    /* we need an apartment to unmarshal into */
    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("Apartment not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    /* read STDOBJREF from wire */
    hres = IStream_Read(stream, &obj, FIELD_OFFSET(struct OR_STANDARD, saResAddr.aStringArray), &res);
    if (hres != S_OK)
    {
        apartment_release(apt);
        return STG_E_READFAULT;
    }

    if (obj.saResAddr.wNumEntries)
    {
        ERR("unsupported size of DUALSTRINGARRAY\n");
        return E_NOTIMPL;
    }

    /* check if we're marshalling back to ourselves */
    if ((apartment_getoxid(apt) == obj.std.oxid) && (stubmgr = get_stub_manager(apt, obj.std.oid)))
    {
        TRACE("Unmarshalling object marshalled in same apartment for iid %s, "
              "returning original object %p\n", debugstr_guid(riid), stubmgr->object);

        hres = IUnknown_QueryInterface(stubmgr->object, riid, ppv);

        /* unref the ifstub. FIXME: only do this on success? */
        if (!stub_manager_is_table_marshaled(stubmgr, &obj.std.ipid))
            stub_manager_ext_release(stubmgr, obj.std.cPublicRefs, obj.std.flags & SORFP_TABLEWEAK, FALSE);

        stub_manager_int_release(stubmgr);
        apartment_release(apt);
        return hres;
    }

    /* notify stub manager about unmarshal if process-local object.
     * note: if the oxid is not found then we and native will quite happily
     * ignore table marshaling and normal marshaling rules regarding number of
     * unmarshals, etc, but if you abuse these rules then your proxy could end
     * up returning RPC_E_DISCONNECTED. */
    if ((stub_apt = apartment_findfromoxid(obj.std.oxid)))
    {
        if ((stubmgr = get_stub_manager(stub_apt, obj.std.oid)))
        {
            if (!stub_manager_notify_unmarshal(stubmgr, &obj.std.ipid))
                hres = CO_E_OBJNOTCONNECTED;
        }
        else
        {
            WARN("Couldn't find object for OXID %s, OID %s, assuming disconnected\n",
                wine_dbgstr_longlong(obj.std.oxid),
                wine_dbgstr_longlong(obj.std.oid));
            hres = CO_E_OBJNOTCONNECTED;
        }
    }
    else
        TRACE("Treating unmarshal from OXID %s as inter-process\n",
            wine_dbgstr_longlong(obj.std.oxid));

    if (hres == S_OK)
        hres = unmarshal_object(&obj.std, apt, dest_context,
                                dest_context_data, riid,
                                stubmgr ? &stubmgr->oxid_info : NULL, ppv);

    if (stubmgr) stub_manager_int_release(stubmgr);
    if (stub_apt) apartment_release(stub_apt);

    if (hres != S_OK) WARN("Failed with error 0x%08x\n", hres);
    else TRACE("Successfully created proxy %p\n", *ppv);

    apartment_release(apt);
    return hres;
}

/***********************************************************************
 *            CoUnmarshalInterface        (combase.@)
 */
HRESULT WINAPI CoUnmarshalInterface(IStream *stream, REFIID riid, void **ppv)
{
    IMarshal *marshal;
    IUnknown *object;
    HRESULT hr;
    IID iid;

    TRACE("%p, %s, %p\n", stream, debugstr_guid(riid), ppv);

    if (!stream || !ppv)
        return E_INVALIDARG;

    hr = get_unmarshaler_from_stream(stream, &marshal, &iid);
    if (hr == S_FALSE)
    {
        hr = std_unmarshal_interface(0, NULL, stream, &iid, (void **)&object);
        if (hr != S_OK)
            ERR("StdMarshal UnmarshalInterface failed, hr %#x\n", hr);
    }
    else if (hr == S_OK)
    {
        /* call the helper object to do the actual unmarshaling */
        hr = IMarshal_UnmarshalInterface(marshal, stream, &iid, (void **)&object);
        IMarshal_Release(marshal);
        if (hr != S_OK)
            ERR("IMarshal::UnmarshalInterface failed, hr %#x\n", hr);
    }

    if (hr == S_OK)
    {
        /* IID_NULL means use the interface ID of the marshaled object */
        if (!IsEqualIID(riid, &IID_NULL) && !IsEqualIID(riid, &iid))
        {
            TRACE("requested interface != marshalled interface, additional QI needed\n");
            hr = IUnknown_QueryInterface(object, riid, ppv);
            if (hr != S_OK)
                ERR("Couldn't query for interface %s, hr %#x\n", debugstr_guid(riid), hr);
            IUnknown_Release(object);
        }
        else
        {
            *ppv = object;
        }
    }

    TRACE("completed with hr 0x%x\n", hr);

    return hr;
}
