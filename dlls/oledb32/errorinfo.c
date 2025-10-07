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

#include "wine/list.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oledb);

struct ErrorEntry
{
    ERRORINFO       info;
    DISPPARAMS      dispparams;
    IUnknown        *custom_error;
    DWORD           lookupID;
};

typedef struct errorrecords
{
    IErrorInfo     IErrorInfo_iface;
    IErrorRecords  IErrorRecords_iface;
    LONG ref;

    struct ErrorEntry *records;
    unsigned int allocated;
    unsigned int count;
} errorrecords;

static inline errorrecords *impl_from_IErrorInfo( IErrorInfo *iface )
{
    return CONTAINING_RECORD(iface, errorrecords, IErrorInfo_iface);
}

static inline errorrecords *impl_from_IErrorRecords( IErrorRecords *iface )
{
    return CONTAINING_RECORD(iface, errorrecords, IErrorRecords_iface);
}

static HRESULT WINAPI errorrecords_QueryInterface(IErrorInfo* iface, REFIID riid, void **ppvoid)
{
    errorrecords *This = impl_from_IErrorInfo(iface);
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

static ULONG WINAPI errorrecords_AddRef(IErrorInfo* iface)
{
    errorrecords *This = impl_from_IErrorInfo(iface);
    TRACE("(%p)->%lu\n",This,This->ref);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI errorrecords_Release(IErrorInfo* iface)
{
    errorrecords *This = impl_from_IErrorInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->%lu\n",This,ref+1);

    if (!ref)
    {
        unsigned int i;

        for (i = 0; i < This->count; i++)
        {
            DISPPARAMS *dispparams = &This->records[i].dispparams;
            unsigned int j;

            if (This->records[i].custom_error)
                IUnknown_Release(This->records[i].custom_error);

            for (j = 0; j < dispparams->cArgs && dispparams->rgvarg; j++)
                VariantClear(&dispparams->rgvarg[j]);
            CoTaskMemFree(dispparams->rgvarg);
            CoTaskMemFree(dispparams->rgdispidNamedArgs);
        }
        free(This->records);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI errorrecords_GetGUID(IErrorInfo* iface, GUID *guid)
{
    errorrecords *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, guid);

    if (!guid)
        return E_INVALIDARG;

    *guid = GUID_NULL;

    return S_OK;
}

static HRESULT WINAPI errorrecords_GetSource(IErrorInfo* iface, BSTR *source)
{
    errorrecords *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, source);

    if (!source)
        return E_INVALIDARG;

    *source = NULL;

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetDescription(IErrorInfo* iface, BSTR *description)
{
    errorrecords *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, description);

    if (!description)
        return E_INVALIDARG;

    *description = NULL;

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetHelpFile(IErrorInfo* iface, BSTR *helpfile)
{
    errorrecords *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, helpfile);

    if (!helpfile)
        return E_INVALIDARG;

    *helpfile = NULL;

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetHelpContext(IErrorInfo* iface, DWORD *context)
{
    errorrecords *This = impl_from_IErrorInfo(iface);

    TRACE("(%p)->(%p)\n", This, context);

    if (!context)
        return E_INVALIDARG;

    *context = 0;

    return E_FAIL;
}

static const IErrorInfoVtbl ErrorInfoVtbl =
{
    errorrecords_QueryInterface,
    errorrecords_AddRef,
    errorrecords_Release,
    errorrecords_GetGUID,
    errorrecords_GetSource,
    errorrecords_GetDescription,
    errorrecords_GetHelpFile,
    errorrecords_GetHelpContext
};

static HRESULT WINAPI errorrec_QueryInterface(IErrorRecords *iface, REFIID riid, void **ppvObject)
{
    errorrecords *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_QueryInterface(&This->IErrorInfo_iface, riid, ppvObject);
}

static ULONG WINAPI errorrec_AddRef(IErrorRecords *iface)
{
    errorrecords *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_AddRef(&This->IErrorInfo_iface);
}

static ULONG WINAPI errorrec_Release(IErrorRecords *iface)
{
    errorrecords *This = impl_from_IErrorRecords(iface);
    return IErrorInfo_Release(&This->IErrorInfo_iface);
}

static HRESULT dup_dispparams(DISPPARAMS *src, DISPPARAMS *dest)
{
    unsigned int i;

    if (!src)
    {
        memset(dest, 0, sizeof(*dest));
        return S_OK;
    }

    *dest = *src;

    if (src->cArgs)
    {
        dest->rgvarg = CoTaskMemAlloc(dest->cArgs * sizeof(*dest->rgvarg));
        for (i = 0; i < src->cArgs; i++)
        {
            VariantInit(&dest->rgvarg[i]);
            VariantCopy(&dest->rgvarg[i], &src->rgvarg[i]);
        }
    }

    if (src->cNamedArgs)
    {
        dest->rgdispidNamedArgs = CoTaskMemAlloc(dest->cNamedArgs * sizeof(*dest->rgdispidNamedArgs));
        memcpy(dest->rgdispidNamedArgs, src->rgdispidNamedArgs, dest->cNamedArgs * sizeof(*dest->rgdispidNamedArgs));
    }

    return S_OK;
}

static HRESULT WINAPI errorrec_AddErrorRecord(IErrorRecords *iface, ERRORINFO *pErrorInfo,
        DWORD dwLookupID, DISPPARAMS *pdispparams, IUnknown *punkCustomError, DWORD dwDynamicErrorID)
{
    errorrecords *This = impl_from_IErrorRecords(iface);
    struct ErrorEntry *entry;
    HRESULT hr;

    TRACE("(%p)->(%p %ld %p %p %ld)\n", This, pErrorInfo, dwLookupID, pdispparams, punkCustomError, dwDynamicErrorID);

    if(!pErrorInfo)
        return E_INVALIDARG;

    if (!This->records)
    {
        const unsigned int initial_size = 16;
        if (!(This->records = malloc(initial_size * sizeof(*This->records))))
            return E_OUTOFMEMORY;

        This->allocated = initial_size;
    }
    else if (This->count == This->allocated)
    {
        struct ErrorEntry *new_ptr;

        new_ptr = realloc(This->records, 2 * This->allocated * sizeof(*This->records));
        if (!new_ptr)
            return E_OUTOFMEMORY;

        This->records = new_ptr;
        This->allocated *= 2;
    }

    entry = This->records + This->count;
    entry->info = *pErrorInfo;

    if (FAILED(hr = dup_dispparams(pdispparams, &entry->dispparams)))
        return hr;

    entry->custom_error = punkCustomError;
    if (entry->custom_error)
        IUnknown_AddRef(entry->custom_error);
    entry->lookupID = dwDynamicErrorID;

    This->count++;

    return S_OK;
}

static HRESULT WINAPI errorrec_GetBasicErrorInfo(IErrorRecords *iface, ULONG index, ERRORINFO *info)
{
    errorrecords *This = impl_from_IErrorRecords(iface);

    TRACE("(%p)->(%lu %p)\n", This, index, info);

    if (!info)
        return E_INVALIDARG;

    if (index >= This->count)
        return DB_E_BADRECORDNUM;

    index = This->count - index - 1;
    *info = This->records[index].info;
    return S_OK;
}

static HRESULT WINAPI errorrec_GetCustomErrorObject(IErrorRecords *iface, ULONG index,
        REFIID riid, IUnknown **object)
{
    errorrecords *This = impl_from_IErrorRecords(iface);

    TRACE("(%p)->(%lu %s %p)\n", This, index, debugstr_guid(riid), object);

    if (!object)
        return E_INVALIDARG;

    *object = NULL;

    if (index >= This->count)
        return DB_E_BADRECORDNUM;

    index = This->count - index - 1;
    if (This->records[index].custom_error)
        return IUnknown_QueryInterface(This->records[index].custom_error, riid, (void **)object);
    else
        return S_OK;
}

static HRESULT WINAPI errorrec_GetErrorInfo(IErrorRecords *iface, ULONG index,
        LCID lcid, IErrorInfo **ppErrorInfo)
{
    errorrecords *This = impl_from_IErrorRecords(iface);

    FIXME("(%p)->(%lu %ld, %p)\n", This, index, lcid, ppErrorInfo);

    if (!ppErrorInfo)
        return E_INVALIDARG;

    if (index >= This->count)
        return DB_E_BADRECORDNUM;

    return IErrorInfo_QueryInterface(&This->IErrorInfo_iface, &IID_IErrorInfo, (void **)ppErrorInfo);
}

static HRESULT WINAPI errorrec_GetErrorParameters(IErrorRecords *iface, ULONG index, DISPPARAMS *pdispparams)
{
    errorrecords *This = impl_from_IErrorRecords(iface);

    TRACE("(%p)->(%lu %p)\n", This, index, pdispparams);

    if (!pdispparams)
        return E_INVALIDARG;

    if (index >= This->count)
        return DB_E_BADRECORDNUM;

    index = This->count - index - 1;
    return dup_dispparams(&This->records[index].dispparams, pdispparams);
}

static HRESULT WINAPI errorrec_GetRecordCount(IErrorRecords *iface, ULONG *count)
{
    errorrecords *This = impl_from_IErrorRecords(iface);

    TRACE("(%p)->(%p)\n", This, count);

    if(!count)
        return E_INVALIDARG;

    *count = This->count;

    TRACE("<--(%lu)\n", *count);

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

HRESULT create_error_object(IUnknown *outer, void **obj)
{
    errorrecords *This;

    TRACE("(%p, %p)\n", outer, obj);

    *obj = NULL;

    if(outer) return CLASS_E_NOAGGREGATION;

    This = malloc(sizeof(*This));
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
