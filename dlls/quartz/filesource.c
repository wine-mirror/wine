/*
 * File Source Filter
 *
 * Copyright 2003 Robert Shearman
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "quartz_private.h"

#include "wine/debug.h"
#include "pin.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wszOutputPinName[] = { 'O','u','t','p','u','t',0 };

static const AM_MEDIA_TYPE default_mt =
{
    {0xe436eb83,0x524f,0x11ce,{0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70}},   /* MEDIATYPE_Stream */
    {0,0,0,{0,0,0,0,0,0,0,0}},
    TRUE,
    FALSE,
    1,
    {0,0,0,{0,0,0,0,0,0,0,0}},
    NULL,
    0,
    NULL
};

typedef struct DATAREQUEST
{
    IMediaSample *pSample;
    DWORD_PTR dwUserData;
    OVERLAPPED ovl;
} DATAREQUEST;

typedef struct AsyncReader
{
    BaseFilter filter;
    IFileSourceFilter IFileSourceFilter_iface;

    BaseOutputPin source;
    IAsyncReader IAsyncReader_iface;

    LPOLESTR pszFileName;
    AM_MEDIA_TYPE *pmt;
    ALLOCATOR_PROPERTIES allocProps;
    HANDLE file;
    BOOL flushing;
    unsigned int queued_number;
    unsigned int samples;
    unsigned int oldest_sample;
    CRITICAL_SECTION sample_cs;
    DATAREQUEST *sample_list;
    /* Have a handle for every sample, and then one more as flushing handle */
    HANDLE *handle_list;
} AsyncReader;

static const IPinVtbl FileAsyncReaderPin_Vtbl;
static const BaseOutputPinFuncTable output_BaseOutputFuncTable;

static inline AsyncReader *impl_from_BaseFilter(BaseFilter *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, filter);
}

static inline AsyncReader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, IFileSourceFilter_iface);
}

static const IBaseFilterVtbl AsyncReader_Vtbl;
static const IFileSourceFilterVtbl FileSource_Vtbl;
static const IAsyncReaderVtbl FileAsyncReader_Vtbl;

static const WCHAR mediatype_name[] = {
    'M', 'e', 'd', 'i', 'a', ' ', 'T', 'y', 'p', 'e', 0 };
static const WCHAR subtype_name[] = {
    'S', 'u', 'b', 't', 'y', 'p', 'e', 0 };
static const WCHAR source_filter_name[] = {
    'S','o','u','r','c','e',' ','F','i','l','t','e','r',0};

static unsigned char byte_from_hex_char(WCHAR wHex)
{
    switch (towlower(wHex))
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return (wHex - '0') & 0xf;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return (wHex - 'a' + 10) & 0xf;
    default:
        return 0;
    }
}

static BOOL process_pattern_string(const WCHAR *pattern, HANDLE file)
{
    ULONG size, offset, i, ret_size;
    BYTE *mask, *expect, *actual;
    BOOL ret = TRUE;

    /* format: "offset, size, mask, value" */

    offset = wcstol(pattern, NULL, 10);

    if (!(pattern = wcschr(pattern, ',')))
        return FALSE;
    pattern++;

    size = wcstol(pattern, NULL, 10);
    mask = heap_alloc(size);
    expect = heap_alloc(size);
    memset(mask, 0xff, size);

    if (!(pattern = wcschr(pattern, ',')))
        return FALSE;
    pattern++;
    while (!iswxdigit(*pattern) && (*pattern != ','))
        pattern++;

    for (i = 0; iswxdigit(*pattern) && (i/2 < size); pattern++, i++)
    {
        if (i % 2)
            mask[i / 2] |= byte_from_hex_char(*pattern);
        else
            mask[i / 2] = byte_from_hex_char(*pattern) << 4;
    }

    if (!(pattern = wcschr(pattern, ',')))
    {
        heap_free(mask);
        heap_free(expect);
        return FALSE;
    }
    pattern++;
    while (!iswxdigit(*pattern) && (*pattern != ','))
        pattern++;

    for (i = 0; iswxdigit(*pattern) && (i/2 < size); pattern++, i++)
    {
        if (i % 2)
            expect[i / 2] |= byte_from_hex_char(*pattern);
        else
            expect[i / 2] = byte_from_hex_char(*pattern) << 4;
    }

    actual = heap_alloc(size);
    SetFilePointer(file, offset, NULL, FILE_BEGIN);
    if (!ReadFile(file, actual, size, &ret_size, NULL) || ret_size != size)
    {
        heap_free(actual);
        heap_free(expect);
        heap_free(mask);
        return FALSE;
    }

    for (i = 0; i < size; ++i)
    {
        if ((actual[i] & mask[i]) != expect[i])
        {
            ret = FALSE;
            break;
        }
    }

    heap_free(actual);
    heap_free(expect);
    heap_free(mask);

    /* If there is a following tuple, then we must match that as well. */
    if (ret && (pattern = wcschr(pattern, ',')))
        return process_pattern_string(pattern + 1, file);

    return ret;
}

BOOL get_media_type(const WCHAR *filename, GUID *majortype, GUID *subtype, GUID *source_clsid)
{
    WCHAR extensions_path[278] = {'M','e','d','i','a',' ','T','y','p','e','\\','E','x','t','e','n','s','i','o','n','s','\\',0};
    static const WCHAR wszExtensions[] = {'E','x','t','e','n','s','i','o','n','s',0};
    static const WCHAR wszMediaType[] = {'M','e','d','i','a',' ','T','y','p','e',0};
    DWORD majortype_idx, size;
    const WCHAR *ext;
    HKEY parent_key;
    HANDLE file;

    if ((ext = wcsrchr(filename, '.')))
    {
        WCHAR guidstr[39];
        HKEY key;

        wcscat(extensions_path, ext);
        if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, extensions_path, 0, KEY_READ, &key))
        {
            size = sizeof(guidstr);
            if (majortype && !RegQueryValueExW(key, mediatype_name, NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, majortype);

            size = sizeof(guidstr);
            if (subtype && !RegQueryValueExW(key, subtype_name, NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, subtype);

            size = sizeof(guidstr);
            if (source_clsid && !RegQueryValueExW(key, source_filter_name, NULL, NULL, (BYTE *)guidstr, &size))
                CLSIDFromString(guidstr, source_clsid);

            RegCloseKey(key);
            return FALSE;
        }
    }

    if ((file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        WARN("Failed to open file %s, error %u.\n", debugstr_w(filename), GetLastError());
        return FALSE;
    }

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszMediaType, 0, KEY_READ, &parent_key))
    {
        CloseHandle(file);
        return FALSE;
    }

    for (majortype_idx = 0; ; ++majortype_idx)
    {
        WCHAR majortype_str[39];
        HKEY majortype_key;
        DWORD subtype_idx;

        size = ARRAY_SIZE(majortype_str);
        if (RegEnumKeyExW(parent_key, majortype_idx, majortype_str, &size, NULL, NULL, NULL, NULL))
            break;

        if (!wcscmp(majortype_str, wszExtensions))
            continue;

        if (RegOpenKeyExW(parent_key, majortype_str, 0, KEY_READ, &majortype_key))
            continue;

        for (subtype_idx = 0; ; ++subtype_idx)
        {
            WCHAR subtype_str[39], *pattern;
            DWORD value_idx, max_size;
            HKEY subtype_key;

            size = ARRAY_SIZE(subtype_str);
            if (RegEnumKeyExW(majortype_key, subtype_idx, subtype_str, &size, NULL, NULL, NULL, NULL))
                break;

            if (RegOpenKeyExW(majortype_key, subtype_str, 0, KEY_READ, &subtype_key))
                continue;

            if (RegQueryInfoKeyW(subtype_key, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &max_size, NULL, NULL))
                continue;

            pattern = heap_alloc(max_size);

            for (value_idx = 0; ; ++value_idx)
            {
                /* The longest name we should encounter is "Source Filter". */
                WCHAR value_name[14], source_clsid_str[39];
                DWORD value_len = ARRAY_SIZE(value_name);

                size = max_size;
                if (RegEnumValueW(subtype_key, value_idx, value_name, &value_len,
                        NULL, NULL, (BYTE *)pattern, &max_size))
                    break;

                if (!wcscmp(value_name, source_filter_name))
                    continue;

                if (!process_pattern_string(pattern, file))
                    continue;

                if (majortype)
                    CLSIDFromString(majortype_str, majortype);
                if (subtype)
                    CLSIDFromString(subtype_str, subtype);
                size = sizeof(source_clsid_str);
                if (source_clsid && !RegQueryValueExW(subtype_key, source_filter_name,
                        NULL, NULL, (BYTE *)source_clsid_str, &size))
                    CLSIDFromString(source_clsid_str, source_clsid);

                heap_free(pattern);
                RegCloseKey(subtype_key);
                RegCloseKey(majortype_key);
                RegCloseKey(parent_key);
                CloseHandle(file);
                return TRUE;
            }

            heap_free(pattern);
            RegCloseKey(subtype_key);
        }

        RegCloseKey(majortype_key);
    }

    RegCloseKey(parent_key);
    CloseHandle(file);
    return FALSE;
}

static IPin *async_reader_get_pin(BaseFilter *iface, unsigned int index)
{
    AsyncReader *filter = impl_from_BaseFilter(iface);

    if (!index && filter->pszFileName)
        return &filter->source.pin.IPin_iface;
    return NULL;
}

static void async_reader_destroy(BaseFilter *iface)
{
    AsyncReader *filter = impl_from_BaseFilter(iface);

    if (filter->pszFileName)
    {
        unsigned int i;

        if (filter->source.pin.pConnectedTo)
            IPin_Disconnect(filter->source.pin.pConnectedTo);

        IPin_Disconnect(&filter->source.pin.IPin_iface);

        CoTaskMemFree(filter->sample_list);
        if (filter->handle_list)
        {
            for (i = 0; i <= filter->samples; ++i)
                CloseHandle(filter->handle_list[i]);
            CoTaskMemFree(filter->handle_list);
        }
        CloseHandle(filter->file);
        filter->sample_cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&filter->sample_cs);
        strmbase_source_cleanup(&filter->source);
    }
    CoTaskMemFree(filter->pszFileName);
    if (filter->pmt)
        DeleteMediaType(filter->pmt);
    strmbase_filter_cleanup(&filter->filter);
    CoTaskMemFree(filter);
}

static HRESULT async_reader_query_interface(BaseFilter *iface, REFIID iid, void **out)
{
    AsyncReader *filter = impl_from_BaseFilter(iface);

    if (IsEqualGUID(iid, &IID_IFileSourceFilter))
    {
        *out = &filter->IFileSourceFilter_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static const BaseFilterFuncTable BaseFuncTable =
{
    .filter_get_pin = async_reader_get_pin,
    .filter_destroy = async_reader_destroy,
    .filter_query_interface = async_reader_query_interface,
};

HRESULT AsyncReader_create(IUnknown *outer, void **out)
{
    AsyncReader *pAsyncRead;
    
    pAsyncRead = CoTaskMemAlloc(sizeof(AsyncReader));

    if (!pAsyncRead)
        return E_OUTOFMEMORY;

    strmbase_filter_init(&pAsyncRead->filter, &AsyncReader_Vtbl, outer, &CLSID_AsyncReader,
            (DWORD_PTR)(__FILE__ ": AsyncReader.csFilter"), &BaseFuncTable);

    pAsyncRead->IFileSourceFilter_iface.lpVtbl = &FileSource_Vtbl;

    pAsyncRead->IAsyncReader_iface.lpVtbl = &FileAsyncReader_Vtbl;

    pAsyncRead->pszFileName = NULL;
    pAsyncRead->pmt = NULL;

    *out = &pAsyncRead->filter.IUnknown_inner;

    TRACE("-- created at %p\n", pAsyncRead);

    return S_OK;
}

static const IBaseFilterVtbl AsyncReader_Vtbl =
{
    BaseFilterImpl_QueryInterface,
    BaseFilterImpl_AddRef,
    BaseFilterImpl_Release,
    BaseFilterImpl_GetClassID,
    BaseFilterImpl_Stop,
    BaseFilterImpl_Pause,
    BaseFilterImpl_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseFilterImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT WINAPI FileSource_QueryInterface(IFileSourceFilter * iface, REFIID riid, LPVOID * ppv)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI FileSource_AddRef(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI FileSource_Release(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI FileSource_Load(IFileSourceFilter * iface, LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
    HANDLE hFile;
    AsyncReader *This = impl_from_IFileSourceFilter(iface);
    PIN_INFO pin_info;

    TRACE("%p->(%s, %p)\n", This, debugstr_w(pszFileName), pmt);

    if (!pszFileName)
        return E_POINTER;

    /* open file */
    /* FIXME: check the sharing values that native uses */
    hFile = CreateFileW(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    /* create pin */
    pin_info.dir = PINDIR_OUTPUT;
    pin_info.pFilter = &This->filter.IBaseFilter_iface;
    lstrcpyW(pin_info.achName, wszOutputPinName);
    strmbase_source_init(&This->source, &FileAsyncReaderPin_Vtbl, &pin_info,
            &output_BaseOutputFuncTable, &This->filter.csFilter);
    BaseFilterImpl_IncrementPinVersion(&This->filter);

    This->file = hFile;
    This->flushing = FALSE;
    This->sample_list = NULL;
    This->handle_list = NULL;
    This->queued_number = 0;
    InitializeCriticalSection(&This->sample_cs);
    This->sample_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FileAsyncReader.sample_cs");

    CoTaskMemFree(This->pszFileName);
    if (This->pmt)
        DeleteMediaType(This->pmt);

    This->pszFileName = CoTaskMemAlloc((lstrlenW(pszFileName) + 1) * sizeof(WCHAR));
    lstrcpyW(This->pszFileName, pszFileName);

    This->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pmt)
    {
        CopyMediaType(This->pmt, &default_mt);
        if (get_media_type(pszFileName, &This->pmt->majortype, &This->pmt->subtype, NULL))
        {
            TRACE("Found major type %s, subtype %s.\n",
                    debugstr_guid(&This->pmt->majortype), debugstr_guid(&This->pmt->subtype));
        }
    }
    else
        CopyMediaType(This->pmt, pmt);

    return S_OK;
}

static HRESULT WINAPI FileSource_GetCurFile(IFileSourceFilter * iface, LPOLESTR * ppszFileName, AM_MEDIA_TYPE * pmt)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);
    
    TRACE("%p->(%p, %p)\n", This, ppszFileName, pmt);

    if (!ppszFileName)
        return E_POINTER;

    /* copy file name & media type if available, otherwise clear the outputs */
    if (This->pszFileName)
    {
        *ppszFileName = CoTaskMemAlloc((lstrlenW(This->pszFileName) + 1) * sizeof(WCHAR));
        lstrcpyW(*ppszFileName, This->pszFileName);
    }
    else
        *ppszFileName = NULL;

    if (pmt)
    {
        if (This->pmt)
            CopyMediaType(pmt, This->pmt);
        else
            ZeroMemory(pmt, sizeof(*pmt));
    }

    return S_OK;
}

static const IFileSourceFilterVtbl FileSource_Vtbl = 
{
    FileSource_QueryInterface,
    FileSource_AddRef,
    FileSource_Release,
    FileSource_Load,
    FileSource_GetCurFile
};

static inline AsyncReader *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, source.pin.IPin_iface);
}

static inline AsyncReader *impl_from_BasePin(BasePin *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, source.pin);
}

static inline AsyncReader *impl_from_BaseOutputPin(BaseOutputPin *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, source);
}

static inline AsyncReader *impl_from_IAsyncReader(IAsyncReader *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, IAsyncReader_iface);
}

static HRESULT WINAPI FileAsyncReaderPin_CheckMediaType(BasePin *iface, const AM_MEDIA_TYPE *pmt)
{
    AsyncReader *filter = impl_from_BasePin(iface);

    if (IsEqualGUID(&pmt->majortype, &filter->pmt->majortype) &&
        IsEqualGUID(&pmt->subtype, &filter->pmt->subtype))
        return S_OK;

    return S_FALSE;
}

static HRESULT WINAPI FileAsyncReaderPin_GetMediaType(BasePin *iface, int index, AM_MEDIA_TYPE *mt)
{
    AsyncReader *filter = impl_from_BasePin(iface);

    if (index < 0)
        return E_INVALIDARG;
    else if (index > 1)
        return VFW_S_NO_MORE_ITEMS;

    if (index == 0)
        CopyMediaType(mt, filter->pmt);
    else if (index == 1)
        CopyMediaType(mt, &default_mt);
    return S_OK;
}

/* overridden pin functions */

static HRESULT WINAPI FileAsyncReaderPin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    AsyncReader *filter = impl_from_IPin(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IPin))
        *out = &filter->source.pin.IPin_iface;
    else if (IsEqualIID(iid, &IID_IAsyncReader))
        *out = &filter->IAsyncReader_iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        *out = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const IPinVtbl FileAsyncReaderPin_Vtbl = 
{
    FileAsyncReaderPin_QueryInterface,
    BasePinImpl_AddRef,
    BasePinImpl_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* this differs from standard OutputPin_AttemptConnection only in that it
 * doesn't need the IMemInputPin interface on the receiving pin */
static HRESULT WINAPI FileAsyncReaderPin_AttemptConnection(BaseOutputPin *This,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    TRACE("%p->(%p, %p)\n", This, pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* FIXME: call queryacceptproc */

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &This->pin.IPin_iface, pmt);

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReaderPin_DecideBufferSize(BaseOutputPin *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    AsyncReader *This = impl_from_BaseOutputPin(iface);
    ALLOCATOR_PROPERTIES actual;

    if (ppropInputRequest->cbAlign && ppropInputRequest->cbAlign != This->allocProps.cbAlign)
        FIXME("Requested Buffer cbAlign mismatch %i,%i\n",This->allocProps.cbAlign, ppropInputRequest->cbAlign);
    if (ppropInputRequest->cbPrefix)
        FIXME("Requested Buffer cbPrefix mismatch %i,%i\n",This->allocProps.cbPrefix, ppropInputRequest->cbPrefix);
    if (ppropInputRequest->cbBuffer)
        FIXME("Requested Buffer cbBuffer mismatch %i,%i\n",This->allocProps.cbBuffer, ppropInputRequest->cbBuffer);
    if (ppropInputRequest->cBuffers)
        FIXME("Requested Buffer cBuffers mismatch %i,%i\n",This->allocProps.cBuffers, ppropInputRequest->cBuffers);

    return IMemAllocator_SetProperties(pAlloc, &This->allocProps, &actual);
}

static const BaseOutputPinFuncTable output_BaseOutputFuncTable = {
    {
        FileAsyncReaderPin_CheckMediaType,
        FileAsyncReaderPin_GetMediaType
    },
    FileAsyncReaderPin_AttemptConnection,
    FileAsyncReaderPin_DecideBufferSize,
    BaseOutputPinImpl_DecideAllocator,
};

static HRESULT WINAPI FileAsyncReader_QueryInterface(IAsyncReader *iface, REFIID iid, void **out)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI FileAsyncReader_AddRef(IAsyncReader * iface)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI FileAsyncReader_Release(IAsyncReader * iface)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

#define DEF_ALIGNMENT 1

static HRESULT WINAPI FileAsyncReader_RequestAllocator(IAsyncReader * iface, IMemAllocator * pPreferred, ALLOCATOR_PROPERTIES * pProps, IMemAllocator ** ppActual)
{
    AsyncReader *This = impl_from_IAsyncReader(iface);
    HRESULT hr = S_OK;

    TRACE("%p->(%p, %p, %p)\n", This, pPreferred, pProps, ppActual);

    if (!pProps->cbAlign || (pProps->cbAlign % DEF_ALIGNMENT) != 0)
        pProps->cbAlign = DEF_ALIGNMENT;

    if (pPreferred)
    {
        hr = IMemAllocator_SetProperties(pPreferred, pProps, pProps);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            IMemAllocator_AddRef(pPreferred);
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
            goto done;
        }
    }

    pPreferred = NULL;

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC, &IID_IMemAllocator, (LPVOID *)&pPreferred);

    if (SUCCEEDED(hr))
    {
        hr = IMemAllocator_SetProperties(pPreferred, pProps, pProps);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
        }
    }

done:
    if (SUCCEEDED(hr))
    {
        CoTaskMemFree(This->sample_list);
        if (This->handle_list)
        {
            int x;
            for (x = 0; x <= This->samples; ++x)
                CloseHandle(This->handle_list[x]);
            CoTaskMemFree(This->handle_list);
        }

        This->samples = pProps->cBuffers;
        This->oldest_sample = 0;
        TRACE("Samples: %u\n", This->samples);
        This->sample_list = CoTaskMemAlloc(sizeof(This->sample_list[0]) * pProps->cBuffers);
        This->handle_list = CoTaskMemAlloc(sizeof(HANDLE) * pProps->cBuffers * 2);

        if (This->sample_list && This->handle_list)
        {
            int x;
            ZeroMemory(This->sample_list, sizeof(This->sample_list[0]) * pProps->cBuffers);
            for (x = 0; x < This->samples; ++x)
            {
                This->sample_list[x].ovl.hEvent = This->handle_list[x] = CreateEventW(NULL, 0, 0, NULL);
                if (x + 1 < This->samples)
                    This->handle_list[This->samples + 1 + x] = This->handle_list[x];
            }
            This->handle_list[This->samples] = CreateEventW(NULL, 1, 0, NULL);
            This->allocProps = *pProps;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CoTaskMemFree(This->sample_list);
            CoTaskMemFree(This->handle_list);
            This->samples = 0;
            This->sample_list = NULL;
            This->handle_list = NULL;
        }
    }

    if (FAILED(hr))
    {
        *ppActual = NULL;
        if (pPreferred)
            IMemAllocator_Release(pPreferred);
    }

    TRACE("-- %x\n", hr);
    return hr;
}

/* we could improve the Request/WaitForNext mechanism by allowing out of order samples.
 * however, this would be quite complicated to do and may be a bit error prone */
static HRESULT WINAPI FileAsyncReader_Request(IAsyncReader * iface, IMediaSample * pSample, DWORD_PTR dwUser)
{
    AsyncReader *This = impl_from_IAsyncReader(iface);
    HRESULT hr = S_OK;
    REFERENCE_TIME Start;
    REFERENCE_TIME Stop;
    LPBYTE pBuffer = NULL;

    TRACE("%p->(%p, %lx)\n", This, pSample, dwUser);

    if (!pSample)
        return E_POINTER;

    /* get start and stop positions in bytes */
    if (SUCCEEDED(hr))
        hr = IMediaSample_GetTime(pSample, &Start, &Stop);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(pSample, &pBuffer);

    EnterCriticalSection(&This->sample_cs);
    if (This->flushing)
    {
        LeaveCriticalSection(&This->sample_cs);
        return VFW_E_WRONG_STATE;
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwLength = (DWORD) BYTES_FROM_MEDIATIME(Stop - Start);
        DATAREQUEST *pDataRq;
        int x;

        /* Try to insert above the waiting sample if possible */
        for (x = This->oldest_sample; x < This->samples; ++x)
        {
            if (!This->sample_list[x].pSample)
                break;
        }

        if (x >= This->samples)
            for (x = 0; x < This->oldest_sample; ++x)
            {
                if (!This->sample_list[x].pSample)
                    break;
            }

        /* There must be a sample we have found */
        assert(x < This->samples);
        ++This->queued_number;

        pDataRq = This->sample_list + x;

        pDataRq->ovl.u.s.Offset = (DWORD) BYTES_FROM_MEDIATIME(Start);
        pDataRq->ovl.u.s.OffsetHigh = (DWORD)(BYTES_FROM_MEDIATIME(Start) >> (sizeof(DWORD) * 8));
        pDataRq->dwUserData = dwUser;

        /* we violate traditional COM rules here by maintaining
         * a reference to the sample, but not calling AddRef, but
         * that's what MSDN says to do */
        pDataRq->pSample = pSample;

        /* this is definitely not how it is implemented on Win9x
         * as they do not support async reads on files, but it is
         * sooo much easier to use this than messing around with threads!
         */
        if (!ReadFile(This->file, pBuffer, dwLength, NULL, &pDataRq->ovl))
            hr = HRESULT_FROM_WIN32(GetLastError());

        /* ERROR_IO_PENDING is not actually an error since this is what we want! */
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            hr = S_OK;
    }

    LeaveCriticalSection(&This->sample_cs);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_WaitForNext(IAsyncReader * iface, DWORD dwTimeout, IMediaSample ** ppSample, DWORD_PTR * pdwUser)
{
    AsyncReader *This = impl_from_IAsyncReader(iface);
    HRESULT hr = S_OK;
    DWORD buffer = ~0;

    TRACE("%p->(%u, %p, %p)\n", This, dwTimeout, ppSample, pdwUser);

    *ppSample = NULL;
    *pdwUser = 0;

    EnterCriticalSection(&This->sample_cs);
    if (!This->flushing)
    {
        LONG oldest = This->oldest_sample;

        if (!This->queued_number)
        {
            /* It could be that nothing is queued right now, but that can be fixed */
            WARN("Called without samples in queue and not flushing!!\n");
        }
        LeaveCriticalSection(&This->sample_cs);

        /* wait for an object to read, or time out */
        buffer = WaitForMultipleObjectsEx(This->samples+1, This->handle_list + oldest, FALSE, dwTimeout, TRUE);

        EnterCriticalSection(&This->sample_cs);
        if (buffer <= This->samples)
        {
            /* Re-scale the buffer back to normal */
            buffer += oldest;

            /* Uh oh, we overshot the flusher handle, renormalize it back to 0..Samples-1 */
            if (buffer > This->samples)
                buffer -= This->samples + 1;
            assert(buffer <= This->samples);
        }

        if (buffer >= This->samples)
        {
            if (buffer != This->samples)
            {
                FIXME("Returned: %u (%08x)\n", buffer, GetLastError());
                hr = VFW_E_TIMEOUT;
            }
            else
                hr = VFW_E_WRONG_STATE;
            buffer = ~0;
        }
        else
            --This->queued_number;
    }

    if (This->flushing && buffer == ~0)
    {
        for (buffer = 0; buffer < This->samples; ++buffer)
        {
            if (This->sample_list[buffer].pSample)
            {
                ResetEvent(This->handle_list[buffer]);
                break;
            }
        }
        if (buffer == This->samples)
        {
            assert(!This->queued_number);
            hr = VFW_E_TIMEOUT;
        }
        else
        {
            --This->queued_number;
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME rtStart, rtStop;
        DATAREQUEST *pDataRq = This->sample_list + buffer;
        DWORD dwBytes = 0;

        /* get any errors */
        if (!This->flushing && !GetOverlappedResult(This->file, &pDataRq->ovl, &dwBytes, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());

        /* Return the sample no matter what so it can be destroyed */
        *ppSample = pDataRq->pSample;
        *pdwUser = pDataRq->dwUserData;

        if (This->flushing)
            hr = VFW_E_WRONG_STATE;

        if (FAILED(hr))
            dwBytes = 0;

        /* Set the time on the sample */
        IMediaSample_SetActualDataLength(pDataRq->pSample, dwBytes);

        rtStart = (DWORD64)pDataRq->ovl.u.s.Offset + ((DWORD64)pDataRq->ovl.u.s.OffsetHigh << 32);
        rtStart = MEDIATIME_FROM_BYTES(rtStart);
        rtStop = rtStart + MEDIATIME_FROM_BYTES(dwBytes);

        IMediaSample_SetTime(pDataRq->pSample, &rtStart, &rtStop);

        This->sample_list[buffer].pSample = NULL;
        assert(This->oldest_sample < This->samples);

        if (buffer == This->oldest_sample)
        {
            LONG x;
            for (x = This->oldest_sample + 1; x < This->samples; ++x)
                if (This->sample_list[x].pSample)
                    break;
            if (x >= This->samples)
                for (x = 0; x < This->oldest_sample; ++x)
                    if (This->sample_list[x].pSample)
                        break;
            if (This->oldest_sample == x)
                /* No samples found, reset to 0 */
                x = 0;
            This->oldest_sample = x;
        }
    }
    LeaveCriticalSection(&This->sample_cs);

    TRACE("-- %x\n", hr);
    return hr;
}

static BOOL sync_read(HANDLE file, LONGLONG offset, LONG length, BYTE *buffer, DWORD *read_len)
{
    OVERLAPPED ovl = {0};
    BOOL ret;

    ovl.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    ovl.u.s.Offset = (DWORD)offset;
    ovl.u.s.OffsetHigh = offset >> 32;

    *read_len = 0;

    ret = ReadFile(file, buffer, length, NULL, &ovl);
    if (ret || GetLastError() == ERROR_IO_PENDING)
        ret = GetOverlappedResult(file, &ovl, read_len, TRUE);

    TRACE("Returning %u bytes.\n", *read_len);

    CloseHandle(ovl.hEvent);
    return ret;
}

static HRESULT WINAPI FileAsyncReader_SyncReadAligned(IAsyncReader *iface, IMediaSample *sample)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    REFERENCE_TIME start_time, end_time;
    DWORD read_len;
    BYTE *buffer;
    LONG length;
    HRESULT hr;
    BOOL ret;

    TRACE("filter %p, sample %p.\n", filter, sample);

    hr = IMediaSample_GetTime(sample, &start_time, &end_time);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(sample, &buffer);

    if (SUCCEEDED(hr))
    {
        length = BYTES_FROM_MEDIATIME(end_time - start_time);
        ret = sync_read(filter->file, BYTES_FROM_MEDIATIME(start_time), length, buffer, &read_len);
        if (ret)
            hr = (read_len == length) ? S_OK : S_FALSE;
        else if (GetLastError() == ERROR_HANDLE_EOF)
            hr = S_OK;
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
        IMediaSample_SetActualDataLength(sample, read_len);

    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader *iface,
        LONGLONG offset, LONG length, BYTE *buffer)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    DWORD read_len;
    HRESULT hr;
    BOOL ret;

    TRACE("filter %p, offset %s, length %d, buffer %p.\n",
            filter, wine_dbgstr_longlong(offset), length, buffer);

    ret = sync_read(filter->file, offset, length, buffer, &read_len);
    if (ret)
        hr = (read_len == length) ? S_OK : S_FALSE;
    else if (GetLastError() == ERROR_HANDLE_EOF)
        hr = S_FALSE;
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

static HRESULT WINAPI FileAsyncReader_Length(IAsyncReader *iface, LONGLONG *total, LONGLONG *available)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    DWORD low, high;

    TRACE("iface %p, total %p, available %p.\n", iface, total, available);

    if ((low = GetFileSize(filter->file, &high)) == -1 && GetLastError() != NO_ERROR)
        return HRESULT_FROM_WIN32(GetLastError());

    *available = *total = (LONGLONG)low | (LONGLONG)high << (sizeof(DWORD) * 8);

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_BeginFlush(IAsyncReader * iface)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->sample_cs);

    filter->flushing = TRUE;
    CancelIoEx(filter->file, NULL);
    SetEvent(filter->handle_list[filter->samples]);

    LeaveCriticalSection(&filter->sample_cs);

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_EndFlush(IAsyncReader * iface)
{
    AsyncReader *filter = impl_from_IAsyncReader(iface);
    int x;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->sample_cs);

    ResetEvent(filter->handle_list[filter->samples]);
    filter->flushing = FALSE;
    for (x = 0; x < filter->samples; ++x)
        assert(!filter->sample_list[x].pSample);

    LeaveCriticalSection(&filter->sample_cs);

    return S_OK;
}

static const IAsyncReaderVtbl FileAsyncReader_Vtbl = 
{
    FileAsyncReader_QueryInterface,
    FileAsyncReader_AddRef,
    FileAsyncReader_Release,
    FileAsyncReader_RequestAllocator,
    FileAsyncReader_Request,
    FileAsyncReader_WaitForNext,
    FileAsyncReader_SyncReadAligned,
    FileAsyncReader_SyncRead,
    FileAsyncReader_Length,
    FileAsyncReader_BeginFlush,
    FileAsyncReader_EndFlush,
};
