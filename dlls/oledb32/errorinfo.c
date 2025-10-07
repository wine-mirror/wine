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

struct error_record
{
    IErrorLookup   *lookup_service;
    ERRORINFO       info;
    DISPPARAMS      dispparams;
    IUnknown        *custom_error;
    DWORD           lookup_id;
    DWORD           dynamic_id;
};

typedef struct errorrecords
{
    IErrorInfo     IErrorInfo_iface;
    IErrorRecords  IErrorRecords_iface;
    LONG ref;

    IErrorInfo *error_info;
    struct error_record *records;
    unsigned int allocated;
    unsigned int count;
} errorrecords;

enum error_info_flags
{
    ERROR_INFO_HAS_DESCRIPTION = 0x1,
    ERROR_INFO_HAS_HELPINFO = 0x2,
    ERROR_INFO_NO_PARENT_REF = 0x4,
};

struct error_info
{
    IErrorInfo IErrorInfo_iface;
    LONG refcount;

    LCID lcid;
    unsigned int index;
    struct errorrecords *records;
    unsigned int flags;

    BSTR source;
    BSTR description;
    BSTR helpfile;
    DWORD helpcontext;
};

static inline errorrecords *impl_records_from_IErrorInfo( IErrorInfo *iface )
{
    return CONTAINING_RECORD(iface, errorrecords, IErrorInfo_iface);
}

static inline errorrecords *impl_from_IErrorRecords( IErrorRecords *iface )
{
    return CONTAINING_RECORD(iface, errorrecords, IErrorRecords_iface);
}

static inline struct error_info *impl_from_IErrorInfo( IErrorInfo *iface )
{
    return CONTAINING_RECORD(iface, struct error_info, IErrorInfo_iface);
}

static HRESULT create_error_info(errorrecords *records, unsigned int index, LCID lcid,
        unsigned int flags, IErrorInfo **out);

static HRESULT return_bstr(BSTR src, BSTR *dst)
{
    *dst = SysAllocStringLen(src, SysStringLen(src));
    return *dst ? S_OK : E_OUTOFMEMORY;
}

static HRESULT errorrecords_get_record(errorrecords *records, unsigned int index,
        struct error_record **record)
{
    *record = NULL;

    if (index >= records->count)
        return DB_E_BADRECORDNUM;

    *record = &records->records[records->count - index - 1];

    return S_OK;
}

static HRESULT error_record_get_lookup_service(struct error_record *record, IErrorLookup **lookup_service)
{
    WCHAR name[64], clsidW[39];
    CLSID clsid;
    LSTATUS ret;
    DWORD size;
    HRESULT hr;
    HKEY hkey;

    if (!record->lookup_service)
    {
        StringFromGUID2(&record->info.clsid, clsidW, ARRAY_SIZE(clsidW));
        wcscpy(name, L"CLSID\\");
        wcscat(name, clsidW);
        wcscat(name, L"\\ExtendedErrors");
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, name, 0, KEY_READ, &hkey))
            return E_FAIL;

        size = ARRAY_SIZE(clsidW);
        ret = RegEnumKeyExW(hkey, 0, clsidW, &size, NULL, NULL, NULL, NULL);
        RegCloseKey(hkey);
        if (ret)
            return E_FAIL;

        if (FAILED(CLSIDFromString(clsidW, &clsid)))
            return E_FAIL;

        if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IErrorLookup,
                (void **)&record->lookup_service)))
        {
            return hr;
        }
    }

    *lookup_service = record->lookup_service;
    IErrorLookup_AddRef(*lookup_service);
    return S_OK;
}

static HRESULT error_info_fetch_description(struct error_info *info)
{
    IErrorLookup *lookup_service;
    struct error_record *record;
    BSTR source, description;
    HRESULT hr;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (FAILED(hr = error_record_get_lookup_service(record, &lookup_service)))
        return hr;

    hr = IErrorLookup_GetErrorDescription(lookup_service, record->info.hrError, record->lookup_id,
            &record->dispparams, info->lcid, &source, &description);
    IErrorLookup_Release(lookup_service);
    if (FAILED(hr))
        return hr;

    info->source = source;
    info->description = description;
    info->flags |= ERROR_INFO_HAS_DESCRIPTION;

    return S_OK;
}

static HRESULT error_info_fetch_helpinfo(struct error_info *info)
{
    IErrorLookup *lookup_service;
    struct error_record *record;
    DWORD helpcontext;
    BSTR helpfile;
    HRESULT hr;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (FAILED(hr = error_record_get_lookup_service(record, &lookup_service)))
        return hr;

    hr = IErrorLookup_GetHelpInfo(lookup_service, record->info.hrError, record->lookup_id,
            info->lcid, &helpfile, &helpcontext);
    IErrorLookup_Release(lookup_service);
    if (FAILED(hr))
        return hr;

    info->helpfile = helpfile;
    info->helpcontext = helpcontext;
    info->flags |= ERROR_INFO_HAS_HELPINFO;

    return S_OK;
}

static HRESULT error_info_get_source(struct error_info *info, BSTR *source)
{
    struct error_record *record;
    HRESULT hr;

    if (!source)
        return E_INVALIDARG;

    *source = NULL;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (record->lookup_id == IDENTIFIER_SDK_ERROR)
        return E_FAIL;

    if (info->flags & ERROR_INFO_HAS_DESCRIPTION)
        return return_bstr(info->source, source);

    if (FAILED(hr = error_info_fetch_description(info)))
        return hr;

    return return_bstr(info->source, source);
}

static HRESULT error_info_get_description(struct error_info *info, BSTR *description)
{
    struct error_record *record;
    WCHAR str[32];
    HRESULT hr;

    if (!description)
        return E_INVALIDARG;

    *description = NULL;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (record->lookup_id == IDENTIFIER_SDK_ERROR)
    {
        /* TODO: description should come from resources */

        swprintf(str, ARRAY_SIZE(str), L"Return code %#lx.", record->info.hrError);
        *description = SysAllocString(str);
        return S_OK;
    }

    if (info->flags & ERROR_INFO_HAS_DESCRIPTION)
        return return_bstr(info->description, description);

    if (FAILED(hr = error_info_fetch_description(info)))
        return hr;

    return return_bstr(info->description, description);
}

static HRESULT error_info_get_helpfile(struct error_info *info, BSTR *helpfile)
{
    struct error_record *record;
    HRESULT hr = S_OK;

    if (!helpfile)
        return E_INVALIDARG;

    *helpfile = NULL;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (record->lookup_id == IDENTIFIER_SDK_ERROR)
        return E_FAIL;

    if (!(info->flags & ERROR_INFO_HAS_HELPINFO))
        hr = error_info_fetch_helpinfo(info);

    return SUCCEEDED(hr) ? return_bstr(info->helpfile, helpfile) : hr;
}

static HRESULT error_info_get_helpcontext(struct error_info *info, DWORD *helpcontext)
{
    struct error_record *record;
    HRESULT hr;

    if (!helpcontext)
        return E_INVALIDARG;

    *helpcontext = 0;

    if (FAILED(hr = errorrecords_get_record(info->records, info->index, &record)))
        return hr;

    if (record->lookup_id == IDENTIFIER_SDK_ERROR)
        return E_FAIL;

    if (!(info->flags & ERROR_INFO_HAS_HELPINFO))
        hr = error_info_fetch_helpinfo(info);

    if (SUCCEEDED(hr))
        *helpcontext = info->helpcontext;

    return hr;
}

static HRESULT WINAPI errorrecords_QueryInterface(IErrorInfo* iface, REFIID riid, void **ppvoid)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvoid);

    *ppvoid = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IErrorInfo))
    {
        *ppvoid = &records->IErrorInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IErrorRecords))
    {
        *ppvoid = &records->IErrorRecords_iface;
    }

    if (*ppvoid)
    {
        IUnknown_AddRef( (IUnknown*)*ppvoid );
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI errorrecords_AddRef(IErrorInfo* iface)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    ULONG ref = InterlockedIncrement(&records->ref);

    TRACE("%p refcount %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI errorrecords_Release(IErrorInfo* iface)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    ULONG ref = InterlockedDecrement(&records->ref);

    TRACE("%p refcount %lu.\n", iface, ref);

    if (!ref)
    {
        unsigned int i;

        for (i = 0; i < records->count; i++)
        {
            struct error_record *record = &records->records[i];
            unsigned int j;

            if (record->custom_error)
                IUnknown_Release(record->custom_error);

            for (j = 0; j < record->dispparams.cArgs && record->dispparams.rgvarg; j++)
                VariantClear(&record->dispparams.rgvarg[j]);
            CoTaskMemFree(record->dispparams.rgvarg);
            CoTaskMemFree(record->dispparams.rgdispidNamedArgs);

            if (record->lookup_service)
            {
                if (record->dynamic_id)
                    IErrorLookup_ReleaseErrors(record->lookup_service, record->dynamic_id);
                IErrorLookup_Release(record->lookup_service);
            }
        }
        free(records->records);
        free(records);
    }
    return ref;
}

static HRESULT errorrecords_get_error_info(errorrecords *records, IErrorInfo **info)
{
    HRESULT hr;

    *info = NULL;

    if (!records->count)
        return E_FAIL;

    if (!records->error_info)
    {
        if (FAILED(hr = create_error_info(records, 0, GetUserDefaultLCID(), ERROR_INFO_NO_PARENT_REF, &records->error_info)))
            return hr;
    }

    if (records->error_info)
    {
        *info = records->error_info;
        IErrorInfo_AddRef(*info);
    }

    return S_OK;
}

static HRESULT WINAPI errorrecords_GetGUID(IErrorInfo* iface, GUID *guid)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    struct error_record *record;

    TRACE("%p, %p.\n", iface, guid);

    if (!guid)
        return E_INVALIDARG;

    errorrecords_get_record(records, 0, &record);
    *guid = record ? record->info.iid : GUID_NULL;

    return S_OK;
}

static HRESULT WINAPI errorrecords_GetSource(IErrorInfo* iface, BSTR *source)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    IErrorInfo *error_info;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, source);

    if (!source)
        return E_INVALIDARG;

    *source = NULL;

    if (SUCCEEDED(errorrecords_get_error_info(records, &error_info)))
    {
        hr = IErrorInfo_GetSource(error_info, source);
        IErrorInfo_Release(error_info);
        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetDescription(IErrorInfo* iface, BSTR *description)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    IErrorInfo *error_info;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, description);

    if (!description)
        return E_INVALIDARG;

    *description = NULL;

    if (SUCCEEDED(errorrecords_get_error_info(records, &error_info)))
    {
        hr = IErrorInfo_GetDescription(error_info, description);
        IErrorInfo_Release(error_info);
        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetHelpFile(IErrorInfo* iface, BSTR *helpfile)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    IErrorInfo *error_info;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, helpfile);

    if (!helpfile)
        return E_INVALIDARG;

    *helpfile = NULL;

    if (SUCCEEDED(errorrecords_get_error_info(records, &error_info)))
    {
        hr = IErrorInfo_GetHelpFile(error_info, helpfile);
        IErrorInfo_Release(error_info);
        return hr;
    }

    return E_FAIL;
}

static HRESULT WINAPI errorrecords_GetHelpContext(IErrorInfo* iface, DWORD *context)
{
    errorrecords *records = impl_records_from_IErrorInfo(iface);
    IErrorInfo *error_info;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, context);

    if (!context)
        return E_INVALIDARG;

    *context = 0;

    if (SUCCEEDED(errorrecords_get_error_info(records, &error_info)))
    {
        hr = IErrorInfo_GetHelpContext(error_info, context);
        IErrorInfo_Release(error_info);
        return hr;
    }

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
    struct error_record *entry, *new_ptr;
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
    entry->lookup_id = dwLookupID;
    entry->dynamic_id = dwDynamicErrorID;
    entry->lookup_service = NULL;

    This->count++;

    return S_OK;
}

static HRESULT WINAPI errorrec_GetBasicErrorInfo(IErrorRecords *iface, ULONG index, ERRORINFO *info)
{
    errorrecords *records = impl_from_IErrorRecords(iface);
    struct error_record *record;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, info);

    if (!info)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = errorrecords_get_record(records, index, &record)))
        *info = record->info;

    return hr;
}

static HRESULT WINAPI errorrec_GetCustomErrorObject(IErrorRecords *iface, ULONG index,
        REFIID riid, IUnknown **object)
{
    errorrecords *records = impl_from_IErrorRecords(iface);
    struct error_record *record;
    HRESULT hr;

    TRACE("%p, %lu, %s, %p.\n", iface, index, debugstr_guid(riid), object);

    if (!object)
        return E_INVALIDARG;

    *object = NULL;

    if (SUCCEEDED(hr = errorrecords_get_record(records, index, &record)) && record->custom_error)
        return IUnknown_QueryInterface(record->custom_error, riid, (void **)object);

    return hr;
}

static HRESULT WINAPI errorrec_GetErrorInfo(IErrorRecords *iface, ULONG index,
        LCID lcid, IErrorInfo **ppErrorInfo)
{
    errorrecords *records = impl_from_IErrorRecords(iface);

    TRACE("%p, %lu, %ld, %p.\n", iface, index, lcid, ppErrorInfo);

    if (!ppErrorInfo)
        return E_INVALIDARG;

    if (index >= records->count)
        return DB_E_BADRECORDNUM;

    if (!index)
        return IErrorInfo_QueryInterface(&records->IErrorInfo_iface, &IID_IErrorInfo, (void **)ppErrorInfo);

    return create_error_info(records, index, lcid, 0, ppErrorInfo);
}

static HRESULT WINAPI errorrec_GetErrorParameters(IErrorRecords *iface, ULONG index, DISPPARAMS *pdispparams)
{
    errorrecords *records = impl_from_IErrorRecords(iface);
    struct error_record *record;
    HRESULT hr;

    TRACE("%p, %lu, %p.\n", iface, index, pdispparams);

    if (!pdispparams)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = errorrecords_get_record(records, index, &record)))
        return dup_dispparams(&record->dispparams, pdispparams);

    return hr;
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

    This = calloc(1, sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IErrorInfo_iface.lpVtbl = &ErrorInfoVtbl;
    This->IErrorRecords_iface.lpVtbl = &ErrorRecordsVtbl;
    This->ref = 1;

    *obj = &This->IErrorInfo_iface;

    return S_OK;
}

static HRESULT WINAPI error_info_QueryInterface(IErrorInfo *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IErrorInfo) || IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IErrorInfo_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI error_info_AddRef(IErrorInfo *iface)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);
    LONG refcount = InterlockedIncrement(&error_info->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI error_info_Release(IErrorInfo *iface)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);
    LONG refcount = InterlockedDecrement(&error_info->refcount);

    TRACE("%p, %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (!(error_info->flags & ERROR_INFO_NO_PARENT_REF))
            IErrorRecords_Release(&error_info->records->IErrorRecords_iface);
        SysFreeString(error_info->source);
        SysFreeString(error_info->description);
        SysFreeString(error_info->helpfile);
        free(error_info);
    }

    return refcount;
}

static HRESULT WINAPI error_info_GetGUID(IErrorInfo *iface, GUID *guid)
{
    FIXME("%p, %p.\n", iface, guid);

    if (!guid)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

static HRESULT WINAPI error_info_GetSource(IErrorInfo *iface, BSTR *source)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);

    TRACE("%p, %p.\n", iface, source);

    return error_info_get_source(error_info, source);
}

static HRESULT WINAPI error_info_GetDescription(IErrorInfo *iface, BSTR *description)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);

    TRACE("%p, %p.\n", iface, description);

    return error_info_get_description(error_info, description);
}

static HRESULT WINAPI error_info_GetHelpFile(IErrorInfo *iface, BSTR *helpfile)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);

    TRACE("%p, %p.\n", iface, helpfile);

    return error_info_get_helpfile(error_info, helpfile);
}

static HRESULT WINAPI error_info_GetHelpContext(IErrorInfo *iface, DWORD *context)
{
    struct error_info *error_info = impl_from_IErrorInfo(iface);

    TRACE("%p, %p.\n", iface, context);

    return error_info_get_helpcontext(error_info, context);
}

static const IErrorInfoVtbl error_info_vtbl =
{
    error_info_QueryInterface,
    error_info_AddRef,
    error_info_Release,
    error_info_GetGUID,
    error_info_GetSource,
    error_info_GetDescription,
    error_info_GetHelpFile,
    error_info_GetHelpContext,
};

static HRESULT create_error_info(errorrecords *records, unsigned int index, LCID lcid,
        unsigned int flags, IErrorInfo **out)
{
    struct error_info *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IErrorInfo_iface.lpVtbl = &error_info_vtbl;
    object->refcount = 1;
    object->lcid = lcid;
    object->records = records;
    if (!(flags & ERROR_INFO_NO_PARENT_REF))
        IErrorRecords_AddRef(&records->IErrorRecords_iface);
    object->index = index;
    object->flags = flags;

    *out = &object->IErrorInfo_iface;

    return S_OK;
}
