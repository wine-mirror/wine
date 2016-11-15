/* OLE DB ErrorInfo
 *
 * Copyright 2013 Alistair Leslie-Hughes
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

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "oleauto.h"
#include "winerror.h"
#include "oledb.h"
#include "oledberr.h"

#include "oledb_private.h"

#include "wine/unicode.h"
#include "wine/list.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

struct ErrorEntry
{
    ERRORINFO       info;
    DISPPARAMS      dispparams;
    IUnknown        *unknown;
    DWORD           lookupID;
};

typedef struct ErrorInfoImpl
{
    IErrorInfo     IErrorInfo_iface;
    IErrorRecords  IErrorRecords_iface;
    LONG ref;

    struct ErrorEntry *records;
    unsigned int allocated;
    unsigned int count;
} ErrorInfoImpl;

static inline ErrorInfoImpl *impl_from_IErrorInfo( IErrorInfo *iface )
{
    return CONTAINING_RECORD(iface, ErrorInfoImpl, IErrorInfo_iface);
}

static inline ErrorInfoImpl *impl_from_IErrorRecords( IErrorRecords *iface )
{
    return CONTAINING_RECORD(iface, ErrorInfoImpl, IErrorRecords_iface);
}

static HRESULT WINAPI IErrorInfoImpl_QueryInterface(IErrorInfo* iface, REFIID riid, void **ppvoid)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid),ppvoid);

    *ppvoid = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IErrorInfo))
    {
      *ppvoid = &This->IErrorInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IErrorRecords))
    {
      *ppvoid = &This->IErrorRecords_iface;
    }

    if(*ppvoid)
    {
      IUnknown_AddRef( (IUnknown*)*ppvoid );
      return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IErrorInfoImpl_AddRef(IErrorInfo* iface)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
    TRACE("(%p)->%u\n",This,This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IErrorInfoImpl_Release(IErrorInfo* iface)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->%u\n",This,ref+1);

    if (!ref)
    {
        unsigned int i;

        for (i = 0; i < This->count; i++)
        {
            if (This->records[i].unknown)
                IUnknown_Release(This->records[i].unknown);
        }
        heap_free(This->records);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI IErrorInfoImpl_GetGUID(IErrorInfo* iface, GUID *guid)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, guid);

    if (!guid)
        return E_INVALIDARG;

    *guid = GUID_NULL;

    return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetSource(IErrorInfo* iface, BSTR *source)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, source);

    if (!source)
        return E_INVALIDARG;

    *source = NULL;

    return E_FAIL;
}

static HRESULT WINAPI IErrorInfoImpl_GetDescription(IErrorInfo* iface, BSTR *description)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, description);

    if (!description)
        return E_INVALIDARG;

    *description = NULL;

    return E_FAIL;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpFile(IErrorInfo* iface, BSTR *helpfile)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, helpfile);

    if (!helpfile)
        return E_INVALIDARG;

    *helpfile = NULL;

    return E_FAIL;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpContext(IErrorInfo* iface, DWORD *context)
{
    ErrorInfoImpl *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, context);

    if (!context)
        return E_INVALIDARG;

    *context = 0;

    return E_FAIL;
}

static const IErrorInfoVtbl ErrorInfoVtbl =
{
    IErrorInfoImpl_QueryInterface,
    IErrorInfoImpl_AddRef,
    IErrorInfoImpl_Release,
    IErrorInfoImpl_GetGUID,
    IErrorInfoImpl_GetSource,
    IErrorInfoImpl_GetDescription,
    IErrorInfoImpl_GetHelpFile,
    IErrorInfoImpl_GetHelpContext
};

static HRESULT WINAPI errorrec_QueryInterface(IErrorRecords *iface, REFIID riid, void **ppvObject)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_QueryInterface(&This->IErrorInfo_iface, riid, ppvObject);
}

static ULONG WINAPI WINAPI errorrec_AddRef(IErrorRecords *iface)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_AddRef(&This->IErrorInfo_iface);
}

static ULONG WINAPI WINAPI errorrec_Release(IErrorRecords *iface)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_Release(&This->IErrorInfo_iface);
}

static HRESULT WINAPI errorrec_AddErrorRecord(IErrorRecords *iface, ERRORINFO *pErrorInfo,
        DWORD dwLookupID, DISPPARAMS *pdispparams, IUnknown *punkCustomError, DWORD dwDynamicErrorID)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);
    struct ErrorEntry *entry;

    TRACE("(%p)->(%p %d %p %p %d)\n", This, pErrorInfo, dwLookupID, pdispparams, punkCustomError, dwDynamicErrorID);

    if(!pErrorInfo)
        return E_INVALIDARG;

    if (!This->records)
    {
        const unsigned int initial_size = 16;
        if (!(This->records = heap_alloc(initial_size * sizeof(*This->records))))
            return E_OUTOFMEMORY;

        This->allocated = initial_size;
    }
    else if (This->count == This->allocated)
    {
        struct ErrorEntry *new_ptr;

        new_ptr = heap_realloc(This->records, 2 * This->allocated * sizeof(*This->records));
        if (!new_ptr)
            return E_OUTOFMEMORY;

        This->records = new_ptr;
        This->allocated *= 2;
    }

    entry = This->records + This->count;
    entry->info = *pErrorInfo;
    if(pdispparams)
        entry->dispparams = *pdispparams;
    entry->unknown = punkCustomError;
    if(entry->unknown)
        IUnknown_AddRef(entry->unknown);
    entry->lookupID = dwDynamicErrorID;

    This->count++;

    return S_OK;
}

static HRESULT WINAPI errorrec_GetBasicErrorInfo(IErrorRecords *iface, ULONG ulRecordNum,
        ERRORINFO *pErrorInfo)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);

    FIXME("(%p)->(%d %p)\n", This, ulRecordNum, pErrorInfo);

    if(!pErrorInfo)
        return E_INVALIDARG;

    if(ulRecordNum > This->count)
        return DB_E_BADRECORDNUM;

    return E_NOTIMPL;
}

static HRESULT WINAPI errorrec_GetCustomErrorObject(IErrorRecords *iface, ULONG ulRecordNum,
        REFIID riid, IUnknown **ppObject)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);

    FIXME("(%p)->(%d %s, %p)\n", This, ulRecordNum, debugstr_guid(riid), ppObject);

    if (!ppObject)
        return E_INVALIDARG;

    *ppObject = NULL;

    if(ulRecordNum > This->count)
        return DB_E_BADRECORDNUM;

    return E_NOTIMPL;
}

static HRESULT WINAPI errorrec_GetErrorInfo(IErrorRecords *iface, ULONG ulRecordNum,
        LCID lcid, IErrorInfo **ppErrorInfo)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);

    FIXME("(%p)->(%d %d, %p)\n", This, ulRecordNum, lcid, ppErrorInfo);

    if (!ppErrorInfo)
        return E_INVALIDARG;

    if(ulRecordNum > This->count)
        return DB_E_BADRECORDNUM;

    return E_NOTIMPL;
}

static HRESULT WINAPI errorrec_GetErrorParameters(IErrorRecords *iface, ULONG ulRecordNum,
        DISPPARAMS *pdispparams)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);

    FIXME("(%p)->(%d %p)\n", This, ulRecordNum, pdispparams);

    if (!pdispparams)
        return E_INVALIDARG;

    if(ulRecordNum > This->count)
        return DB_E_BADRECORDNUM;

    return E_NOTIMPL;
}

static HRESULT WINAPI errorrec_GetRecordCount(IErrorRecords *iface, ULONG *count)
{
    ErrorInfoImpl *This = impl_from_IErrorRecords(iface);

    TRACE("(%p)->(%p)\n", This, count);

    if(!count)
        return E_INVALIDARG;

    *count = This->count;

    TRACE("<--(%u)\n", *count);

    return S_OK;
}

static const IErrorRecordsVtbl ErrorRecordsVtbl =
{
    errorrec_QueryInterface,
    errorrec_AddRef,
    errorrec_Release,
    errorrec_AddErrorRecord,
    errorrec_GetBasicErrorInfo,
    errorrec_GetCustomErrorObject,
    errorrec_GetErrorInfo,
    errorrec_GetErrorParameters,
    errorrec_GetRecordCount
};

HRESULT create_error_info(IUnknown *outer, void **obj)
{
    ErrorInfoImpl *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = heap_alloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IErrorInfo_iface.lpVtbl = &ErrorInfoVtbl;
    This->IErrorRecords_iface.lpVtbl = &ErrorRecordsVtbl;
    This->ref = 1;

    This->records = NULL;
    This->allocated = 0;
    This->count = 0;

    *obj = &This->IErrorInfo_iface;

    return S_OK;
}
