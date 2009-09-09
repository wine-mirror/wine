/* OLE DB Conversion library
 *
 * Copyright 2009 Huw Davies
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msdadc.h"
#include "oledberr.h"

#include "oledb_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

typedef struct
{
    const struct IDataConvertVtbl *lpVtbl;
    const struct IDCInfoVtbl *lpDCInfoVtbl;

    LONG ref;

    UINT version; /* Set by IDCInfo_SetInfo */
} convert;

static inline convert *impl_from_IDataConvert(IDataConvert *iface)
{
    return (convert *)((char*)iface - FIELD_OFFSET(convert, lpVtbl));
}

static inline convert *impl_from_IDCInfo(IDCInfo *iface)
{
    return (convert *)((char*)iface - FIELD_OFFSET(convert, lpDCInfoVtbl));
}

static HRESULT WINAPI convert_QueryInterface(IDataConvert* iface,
                                             REFIID riid,
                                             void **obj)
{
    convert *This = impl_from_IDataConvert(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDataConvert))
    {
        *obj = iface;
    }
    else if(IsEqualIID(riid, &IID_IDCInfo))
    {
        *obj = &This->lpDCInfoVtbl;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDataConvert_AddRef(iface);
    return S_OK;
}


static ULONG WINAPI convert_AddRef(IDataConvert* iface)
{
    convert *This = impl_from_IDataConvert(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}


static ULONG WINAPI convert_Release(IDataConvert* iface)
{
    convert *This = impl_from_IDataConvert(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI convert_DataConvert(IDataConvert* iface,
                                          DBTYPE wSrcType, DBTYPE wDstType,
                                          DBLENGTH cbSrcLength, DBLENGTH *pcbDstLength,
                                          void *pSrc, void *pDst,
                                          DBLENGTH cbDstMaxLength,
                                          DBSTATUS dbsSrcStatus, DBSTATUS *pdbsDstStatus,
                                          BYTE bPrecision, BYTE bScale,
                                          DBDATACONVERT dwFlags)
{
    convert *This = impl_from_IDataConvert(iface);
    FIXME("(%p)->(%d, %d, %d, %p, %p, %p, %d, %d, %p, %d, %d, %x): stub\n", This,
          wSrcType, wDstType, cbSrcLength, pcbDstLength, pSrc, pDst, cbDstMaxLength,
          dbsSrcStatus, pdbsDstStatus, bPrecision, bScale, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI convert_CanConvert(IDataConvert* iface,
                                         DBTYPE wSrcType, DBTYPE wDstType)
{
    convert *This = impl_from_IDataConvert(iface);
    FIXME("(%p)->(%d, %d): stub\n", This, wSrcType, wDstType);

    return E_NOTIMPL;
}

static HRESULT WINAPI convert_GetConversionSize(IDataConvert* iface,
                                                DBTYPE wSrcType, DBTYPE wDstType,
                                                DBLENGTH *pcbSrcLength, DBLENGTH *pcbDstLength,
                                                void *pSrc)
{
    convert *This = impl_from_IDataConvert(iface);
    FIXME("(%p)->(%d, %d, %p, %p, %p): stub\n", This, wSrcType, wDstType, pcbSrcLength, pcbDstLength, pSrc);

    return E_NOTIMPL;
}

static const struct IDataConvertVtbl convert_vtbl =
{
    convert_QueryInterface,
    convert_AddRef,
    convert_Release,
    convert_DataConvert,
    convert_CanConvert,
    convert_GetConversionSize
};

static HRESULT WINAPI dcinfo_QueryInterface(IDCInfo* iface, REFIID riid, void **obj)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_QueryInterface((IDataConvert *)This, riid, obj);
}

static ULONG WINAPI dcinfo_AddRef(IDCInfo* iface)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_AddRef((IDataConvert *)This);
}

static ULONG WINAPI dcinfo_Release(IDCInfo* iface)
{
    convert *This = impl_from_IDCInfo(iface);

    return IDataConvert_Release((IDataConvert *)This);
}

static HRESULT WINAPI dcinfo_GetInfo(IDCInfo *iface, ULONG num, DCINFOTYPE types[], DCINFO **info_ptr)
{
    convert *This = impl_from_IDCInfo(iface);
    ULONG i;
    DCINFO *infos;

    TRACE("(%p)->(%d, %p, %p)\n", This, num, types, info_ptr);

    *info_ptr = infos = CoTaskMemAlloc(num * sizeof(*infos));
    if(!infos) return E_OUTOFMEMORY;

    for(i = 0; i < num; i++)
    {
        infos[i].eInfoType = types[i];
        VariantInit(&infos[i].vData);

        switch(types[i])
        {
        case DCINFOTYPE_VERSION:
            V_VT(&infos[i].vData) = VT_UI4;
            V_UI4(&infos[i].vData) = This->version;
            break;
        }
    }

    return S_OK;
}

static HRESULT WINAPI dcinfo_SetInfo(IDCInfo* iface, ULONG num, DCINFO info[])
{
    convert *This = impl_from_IDCInfo(iface);
    ULONG i;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%d, %p)\n", This, num, info);

    for(i = 0; i < num; i++)
    {
        switch(info[i].eInfoType)
        {
        case DCINFOTYPE_VERSION:
            if(V_VT(&info[i].vData) != VT_UI4)
            {
                FIXME("VERSION with vt %x\n", V_VT(&info[i].vData));
                hr = DB_S_ERRORSOCCURRED;
                break;
            }
            This->version = V_UI4(&info[i].vData);
            break;

        default:
            FIXME("Unhandled info type %d (vt %x)\n", info[i].eInfoType, V_VT(&info[i].vData));
        }
    }
    return hr;
}

static const struct IDCInfoVtbl dcinfo_vtbl =
{
    dcinfo_QueryInterface,
    dcinfo_AddRef,
    dcinfo_Release,
    dcinfo_GetInfo,
    dcinfo_SetInfo
};

HRESULT create_oledb_convert(IUnknown *outer, void **obj)
{
    convert *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->lpVtbl = &convert_vtbl;
    This->lpDCInfoVtbl = &dcinfo_vtbl;
    This->ref = 1;
    This->version = 0x110;

    *obj = &This->lpVtbl;

    return S_OK;
}
