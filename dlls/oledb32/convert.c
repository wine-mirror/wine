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

#include "oledb_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

typedef struct
{
    const struct IDataConvertVtbl *lpVtbl;

    LONG ref;
} convert;

static inline convert *impl_from_IDataConvert(IDataConvert *iface)
{
    return (convert *)((char*)iface - FIELD_OFFSET(convert, lpVtbl));
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

HRESULT create_oledb_convert(IUnknown *outer, void **obj)
{
    convert *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->lpVtbl = &convert_vtbl;
    This->ref = 1;

    *obj = &This->lpVtbl;

    return S_OK;
}
