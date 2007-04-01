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
#include "wine/unicode.h"
#include "pin.h"
#include "uuids.h"
#include "vfwmsgs.h"
#include "winbase.h"
#include "winreg.h"
#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wszOutputPinName[] = { 'O','u','t','p','u','t',0 };

typedef struct AsyncReader
{
    const IBaseFilterVtbl * lpVtbl;
    const IFileSourceFilterVtbl * lpVtblFSF;

    LONG refCount;
    FILTER_INFO filterInfo;
    FILTER_STATE state;
    CRITICAL_SECTION csFilter;

    IPin * pOutputPin;
    LPOLESTR pszFileName;
    AM_MEDIA_TYPE * pmt;
} AsyncReader;

static const IBaseFilterVtbl AsyncReader_Vtbl;
static const IFileSourceFilterVtbl FileSource_Vtbl;
static const IAsyncReaderVtbl FileAsyncReader_Vtbl;

static HRESULT FileAsyncReader_Construct(HANDLE hFile, IBaseFilter * pBaseFilter, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

static inline AsyncReader *impl_from_IFileSourceFilter( IFileSourceFilter *iface )
{
    return (AsyncReader *)((char*)iface - FIELD_OFFSET(AsyncReader, lpVtblFSF));
}

static HRESULT process_extensions(HKEY hkeyExtensions, LPCOLESTR pszFileName, GUID * majorType, GUID * minorType)
{
    /* FIXME: implement */
    return E_NOTIMPL;
}

static unsigned char byte_from_hex_char(WCHAR wHex)
{
    switch (tolowerW(wHex))
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

static HRESULT process_pattern_string(LPCWSTR wszPatternString, IAsyncReader * pReader)
{
    ULONG ulOffset;
    ULONG ulBytes;
    BYTE * pbMask;
    BYTE * pbValue;
    BYTE * pbFile;
    HRESULT hr = S_OK;
    ULONG strpos;

    TRACE("\t\tPattern string: %s\n", debugstr_w(wszPatternString));
    
    /* format: "offset, bytestocompare, mask, value" */

    ulOffset = strtolW(wszPatternString, NULL, 10);

    if (!(wszPatternString = strchrW(wszPatternString, ',')))
        return E_INVALIDARG;

    wszPatternString++; /* skip ',' */

    ulBytes = strtolW(wszPatternString, NULL, 10);

    pbMask = HeapAlloc(GetProcessHeap(), 0, ulBytes);
    pbValue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulBytes);
    pbFile = HeapAlloc(GetProcessHeap(), 0, ulBytes);

    /* default mask is match everything */
    memset(pbMask, 0xFF, ulBytes);

    if (!(wszPatternString = strchrW(wszPatternString, ',')))
        hr = E_INVALIDARG;

    wszPatternString++; /* skip ',' */

    if (hr == S_OK)
    {
        for ( ; !isxdigitW(*wszPatternString) && (*wszPatternString != ','); wszPatternString++)
            ;

        for (strpos = 0; isxdigitW(*wszPatternString) && (strpos/2 < ulBytes); wszPatternString++, strpos++)
        {
            if ((strpos % 2) == 1) /* odd numbered position */
                pbMask[strpos / 2] |= byte_from_hex_char(*wszPatternString);
            else
                pbMask[strpos / 2] = byte_from_hex_char(*wszPatternString) << 4;
        }

        if (!(wszPatternString = strchrW(wszPatternString, ',')))
            hr = E_INVALIDARG;
    
        wszPatternString++; /* skip ',' */
    }

    if (hr == S_OK)
    {
        for ( ; !isxdigitW(*wszPatternString) && (*wszPatternString != ','); wszPatternString++)
            ;

        for (strpos = 0; isxdigitW(*wszPatternString) && (strpos/2 < ulBytes); wszPatternString++, strpos++)
        {
            if ((strpos % 2) == 1) /* odd numbered position */
                pbValue[strpos / 2] |= byte_from_hex_char(*wszPatternString);
            else
                pbValue[strpos / 2] = byte_from_hex_char(*wszPatternString) << 4;
        }
    }

    if (hr == S_OK)
        hr = IAsyncReader_SyncRead(pReader, ulOffset, ulBytes, pbFile);

    if (hr == S_OK)
    {
        ULONG i;
        for (i = 0; i < ulBytes; i++)
            if ((pbFile[i] & pbMask[i]) != pbValue[i])
            {
                hr = S_FALSE;
                break;
            }
    }

    HeapFree(GetProcessHeap(), 0, pbMask);
    HeapFree(GetProcessHeap(), 0, pbValue);
    HeapFree(GetProcessHeap(), 0, pbFile);

    /* if we encountered no errors with this string, and there is a following tuple, then we
     * have to match that as well to succeed */
    if ((hr == S_OK) && (wszPatternString = strchrW(wszPatternString, ',')))
        return process_pattern_string(wszPatternString + 1, pReader);
    else
        return hr;
}

static HRESULT GetClassMediaFile(IAsyncReader * pReader, LPCOLESTR pszFileName, GUID * majorType, GUID * minorType)
{
    HKEY hkeyMediaType = NULL;
    LONG lRet;
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    static const WCHAR wszMediaType[] = {'M','e','d','i','a',' ','T','y','p','e',0};

    CopyMemory(majorType, &GUID_NULL, sizeof(*majorType));
    CopyMemory(minorType, &GUID_NULL, sizeof(*minorType));

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszMediaType, 0, KEY_READ, &hkeyMediaType);
    hr = HRESULT_FROM_WIN32(lRet);

    if (SUCCEEDED(hr))
    {
        DWORD indexMajor;

        for (indexMajor = 0; !bFound; indexMajor++)
        {
            HKEY hkeyMajor;
            WCHAR wszMajorKeyName[CHARS_IN_GUID];
            DWORD dwKeyNameLength = sizeof(wszMajorKeyName) / sizeof(wszMajorKeyName[0]);
            static const WCHAR wszExtensions[] = {'E','x','t','e','n','s','i','o','n','s',0};
    
            if (RegEnumKeyExW(hkeyMediaType, indexMajor, wszMajorKeyName, &dwKeyNameLength, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;
            if (RegOpenKeyExW(hkeyMediaType, wszMajorKeyName, 0, KEY_READ, &hkeyMajor) != ERROR_SUCCESS)
                break;
            TRACE("%s\n", debugstr_w(wszMajorKeyName));
            if (!strcmpW(wszExtensions, wszMajorKeyName))
            {
                if (process_extensions(hkeyMajor, pszFileName, majorType, minorType) == S_OK)
                    bFound = TRUE;
            }
            else
            {
                DWORD indexMinor;

                for (indexMinor = 0; !bFound; indexMinor++)
                {
                    HKEY hkeyMinor;
                    WCHAR wszMinorKeyName[CHARS_IN_GUID];
                    DWORD dwMinorKeyNameLen = sizeof(wszMinorKeyName) / sizeof(wszMinorKeyName[0]);
                    DWORD maxValueLen;
                    DWORD indexValue;

                    if (RegEnumKeyExW(hkeyMajor, indexMinor, wszMinorKeyName, &dwMinorKeyNameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                        break;

                    if (RegOpenKeyExW(hkeyMajor, wszMinorKeyName, 0, KEY_READ, &hkeyMinor) != ERROR_SUCCESS)
                        break;

                    TRACE("\t%s\n", debugstr_w(wszMinorKeyName));
        
                    if (RegQueryInfoKeyW(hkeyMinor, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &maxValueLen, NULL, NULL) != ERROR_SUCCESS)
                        break;

                    for (indexValue = 0; !bFound; indexValue++)
                    {
                        DWORD dwType;
                        WCHAR wszValueName[14]; /* longest name we should encounter will be "Source Filter" */
                        LPWSTR wszPatternString = HeapAlloc(GetProcessHeap(), 0, maxValueLen);
                        DWORD dwValueNameLen = sizeof(wszValueName) / sizeof(wszValueName[0]); /* remember this is in chars */
                        DWORD dwDataLen = maxValueLen; /* remember this is in bytes */
                        static const WCHAR wszSourceFilter[] = {'S','o','u','r','c','e',' ','F','i','l','t','e','r',0};
                        LONG temp;

                        if ((temp = RegEnumValueW(hkeyMinor, indexValue, wszValueName, &dwValueNameLen, NULL, &dwType, (LPBYTE)wszPatternString, &dwDataLen)) != ERROR_SUCCESS)
                        {
                            HeapFree(GetProcessHeap(), 0, wszPatternString);
                            break;
                        }

                        /* if it is not the source filter value */
                        if (strcmpW(wszValueName, wszSourceFilter))
                        {
                            if (process_pattern_string(wszPatternString, pReader) == S_OK)
                            {
                                if (SUCCEEDED(CLSIDFromString(wszMajorKeyName, majorType)) &&
                                    SUCCEEDED(CLSIDFromString(wszMinorKeyName, minorType)))
                                    bFound = TRUE;
                            }
                        }
                        HeapFree(GetProcessHeap(), 0, wszPatternString);
                    }
                    CloseHandle(hkeyMinor);
                }
            }
            CloseHandle(hkeyMajor);
        }
    }
    CloseHandle(hkeyMediaType);

    if (SUCCEEDED(hr) && !bFound)
    {
        ERR("Media class not found\n");
        hr = E_FAIL;
    }
    else if (bFound)
        TRACE("Found file's class: major = %s, subtype = %s\n", qzdebugstr_guid(majorType), qzdebugstr_guid(minorType));

    return hr;
}

HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    AsyncReader *pAsyncRead;
    
    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;
    
    pAsyncRead = CoTaskMemAlloc(sizeof(AsyncReader));

    if (!pAsyncRead)
        return E_OUTOFMEMORY;

    pAsyncRead->lpVtbl = &AsyncReader_Vtbl;
    pAsyncRead->lpVtblFSF = &FileSource_Vtbl;
    pAsyncRead->refCount = 1;
    pAsyncRead->filterInfo.achName[0] = '\0';
    pAsyncRead->filterInfo.pGraph = NULL;
    pAsyncRead->pOutputPin = NULL;

    InitializeCriticalSection(&pAsyncRead->csFilter);
    pAsyncRead->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": AsyncReader.csFilter");

    pAsyncRead->pszFileName = NULL;
    pAsyncRead->pmt = NULL;

    *ppv = (LPVOID)pAsyncRead;

    TRACE("-- created at %p\n", pAsyncRead);

    return S_OK;
}

/** IUnknown methods **/

static HRESULT WINAPI AsyncReader_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IFileSourceFilter))
        *ppv = (LPVOID)(&This->lpVtblFSF);

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI AsyncReader_AddRef(IBaseFilter * iface)
{
    AsyncReader *This = (AsyncReader *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);
    
    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);
    
    return refCount;
}

static ULONG WINAPI AsyncReader_Release(IBaseFilter * iface)
{
    AsyncReader *This = (AsyncReader *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);
    
    TRACE("(%p)->() Release from %d\n", This, refCount + 1);
    
    if (!refCount)
    {
        if (This->pOutputPin)
        {
            IPin *pConnectedTo;
            if(SUCCEEDED(IPin_ConnectedTo(This->pOutputPin, &pConnectedTo)))
            {
                IPin_Disconnect(pConnectedTo);
                IPin_Release(pConnectedTo);
            }
            IPin_Disconnect(This->pOutputPin);
            IPin_Release(This->pOutputPin);
        }
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);
        This->lpVtbl = NULL;
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

/** IPersist methods **/

static HRESULT WINAPI AsyncReader_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    TRACE("(%p)\n", pClsid);

    *pClsid = CLSID_AsyncReader;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI AsyncReader_Stop(IBaseFilter * iface)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("()\n");

    This->state = State_Stopped;
    
    return S_OK;
}

static HRESULT WINAPI AsyncReader_Pause(IBaseFilter * iface)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("()\n");

    This->state = State_Paused;

    return S_OK;
}

static HRESULT WINAPI AsyncReader_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%x%08x)\n", (ULONG)(tStart >> 32), (ULONG)tStart);

    This->state = State_Running;

    return S_OK;
}

static HRESULT WINAPI AsyncReader_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%u, %p)\n", dwMilliSecsTimeout, pState);

    *pState = This->state;
    
    return S_OK;
}

static HRESULT WINAPI AsyncReader_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
/*    AsyncReader *This = (AsyncReader *)iface;*/

    TRACE("(%p)\n", pClock);

    return S_OK;
}

static HRESULT WINAPI AsyncReader_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
/*    AsyncReader *This = (AsyncReader *)iface;*/

    TRACE("(%p)\n", ppClock);

    return S_OK;
}

/** IBaseFilter methods **/

static HRESULT WINAPI AsyncReader_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    ENUMPINDETAILS epd;
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%p)\n", ppEnum);

    epd.cPins = This->pOutputPin ? 1 : 0;
    epd.ppPins = &This->pOutputPin;
    return IEnumPinsImpl_Construct(&epd, ppEnum);
}

static HRESULT WINAPI AsyncReader_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("(%s, %p)\n", debugstr_w(Id), ppPin);

    return E_NOTIMPL;
}

static HRESULT WINAPI AsyncReader_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%p)\n", pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);
    
    return S_OK;
}

static HRESULT WINAPI AsyncReader_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    AsyncReader *This = (AsyncReader *)iface;

    TRACE("(%p, %s)\n", pGraph, debugstr_w(pName));

    if (pName)
        strcpyW(This->filterInfo.achName, pName);
    else
        *This->filterInfo.achName = 0;
    This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */

    return S_OK;
}

static HRESULT WINAPI AsyncReader_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    FIXME("(%p)\n", pVendorInfo);

    return E_NOTIMPL;
}

static const IBaseFilterVtbl AsyncReader_Vtbl =
{
    AsyncReader_QueryInterface,
    AsyncReader_AddRef,
    AsyncReader_Release,
    AsyncReader_GetClassID,
    AsyncReader_Stop,
    AsyncReader_Pause,
    AsyncReader_Run,
    AsyncReader_GetState,
    AsyncReader_SetSyncSource,
    AsyncReader_GetSyncSource,
    AsyncReader_EnumPins,
    AsyncReader_FindPin,
    AsyncReader_QueryFilterInfo,
    AsyncReader_JoinFilterGraph,
    AsyncReader_QueryVendorInfo
};

static HRESULT WINAPI FileSource_QueryInterface(IFileSourceFilter * iface, REFIID riid, LPVOID * ppv)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface((IFileSourceFilter*)&This->lpVtbl, riid, ppv);
}

static ULONG WINAPI FileSource_AddRef(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef((IFileSourceFilter*)&This->lpVtbl);
}

static ULONG WINAPI FileSource_Release(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release((IFileSourceFilter*)&This->lpVtbl);
}

static HRESULT WINAPI FileSource_Load(IFileSourceFilter * iface, LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    HANDLE hFile;
    IAsyncReader * pReader = NULL;
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    TRACE("(%s, %p)\n", debugstr_w(pszFileName), pmt);

    /* open file */
    /* FIXME: check the sharing values that native uses */
    hFile = CreateFileW(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    /* create pin */
    hr = FileAsyncReader_Construct(hFile, (IBaseFilter *)&This->lpVtbl, &This->csFilter, &This->pOutputPin);

    if (SUCCEEDED(hr))
        hr = IPin_QueryInterface(This->pOutputPin, &IID_IAsyncReader, (LPVOID *)&pReader);

    /* store file name & media type */
    if (SUCCEEDED(hr))
    {
        This->pszFileName = CoTaskMemAlloc((strlenW(pszFileName) + 1) * sizeof(WCHAR));
        strcpyW(This->pszFileName, pszFileName);
        This->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if (!pmt)
        {
            This->pmt->bFixedSizeSamples = TRUE;
            This->pmt->bTemporalCompression = FALSE;
            This->pmt->cbFormat = 0;
            This->pmt->pbFormat = NULL;
            This->pmt->pUnk = NULL;
            This->pmt->lSampleSize = 0;
            memcpy(&This->pmt->formattype, &FORMAT_None, sizeof(FORMAT_None));
            hr = GetClassMediaFile(pReader, pszFileName, &This->pmt->majortype, &This->pmt->subtype);
            if (FAILED(hr))
            {
                CoTaskMemFree(This->pmt);
                This->pmt = NULL;
            }
        }
        else
            CopyMediaType(This->pmt, pmt);
    }

    if (pReader)
        IAsyncReader_Release(pReader);

    if (FAILED(hr))
    {
        if (This->pOutputPin)
        {
            IPin_Release(This->pOutputPin);
            This->pOutputPin = NULL;
        }

        CoTaskMemFree(This->pszFileName);
        This->pszFileName = NULL;

        CloseHandle(hFile);
    }

    /* FIXME: check return codes */
    return hr;
}

static HRESULT WINAPI FileSource_GetCurFile(IFileSourceFilter * iface, LPOLESTR * ppszFileName, AM_MEDIA_TYPE * pmt)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);
    
    TRACE("(%p, %p)\n", ppszFileName, pmt);

    if (!ppszFileName)
        return E_POINTER;

    /* copy file name & media type if available, otherwise clear the outputs */
    if (This->pszFileName)
    {
        *ppszFileName = CoTaskMemAlloc((strlenW(This->pszFileName) + 1) * sizeof(WCHAR));
        strcpyW(*ppszFileName, This->pszFileName);
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


/* the dwUserData passed back to user */
typedef struct DATAREQUEST
{
    IMediaSample * pSample; /* sample passed to us by user */
    DWORD_PTR dwUserData; /* user data passed to us */
    OVERLAPPED ovl; /* our overlapped structure */

    struct DATAREQUEST * pNext; /* next data request in list */
} DATAREQUEST;

static void queue(DATAREQUEST * pHead, DATAREQUEST * pItem)
{
    DATAREQUEST * pCurrent;
    for (pCurrent = pHead; pCurrent->pNext; pCurrent = pCurrent->pNext)
        ;
    pCurrent->pNext = pItem;
}

typedef struct FileAsyncReader
{
    OutputPin pin;
    const struct IAsyncReaderVtbl * lpVtblAR;

    HANDLE hFile;
    HANDLE hEvent;
    BOOL bFlushing;
    DATAREQUEST * pHead; /* head of data request list */
    CRITICAL_SECTION csList; /* critical section to protect operations on list */
} FileAsyncReader;

static inline FileAsyncReader *impl_from_IAsyncReader( IAsyncReader *iface )
{
    return (FileAsyncReader *)((char*)iface - FIELD_OFFSET(FileAsyncReader, lpVtblAR));
}

static HRESULT AcceptProcAFR(LPVOID iface, const AM_MEDIA_TYPE *pmt)
{
    AsyncReader *This = (AsyncReader *)iface;
    
    FIXME("(%p, %p)\n", iface, pmt);

    if (IsEqualGUID(&pmt->majortype, &This->pmt->majortype) &&
        IsEqualGUID(&pmt->subtype, &This->pmt->subtype) &&
        IsEqualGUID(&pmt->formattype, &FORMAT_None))
        return S_OK;
    
    return S_FALSE;
}

/* overriden pin functions */

static HRESULT WINAPI FileAsyncReaderPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    FileAsyncReader *This = (FileAsyncReader *)iface;
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IPin))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IAsyncReader))
        *ppv = (LPVOID)&This->lpVtblAR;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI FileAsyncReaderPin_Release(IPin * iface)
{
    FileAsyncReader *This = (FileAsyncReader *)iface;
    ULONG refCount = InterlockedDecrement(&This->pin.pin.refCount);
    
    TRACE("(%p)->() Release from %d\n", This, refCount + 1);
    
    if (!refCount)
    {
        DATAREQUEST * pCurrent;
        DATAREQUEST * pNext;
        for (pCurrent = This->pHead; pCurrent; pCurrent = pNext)
        {
            pNext = pCurrent->pNext;
            CoTaskMemFree(pCurrent);
        }
        CloseHandle(This->hFile);
        CloseHandle(This->hEvent);
        This->csList.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csList);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI FileAsyncReaderPin_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    ENUMMEDIADETAILS emd;
    FileAsyncReader *This = (FileAsyncReader *)iface;

    TRACE("(%p)\n", ppEnum);

    emd.cMediaTypes = 1;
    emd.pMediaTypes = ((AsyncReader *)This->pin.pin.pinInfo.pFilter)->pmt;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

static const IPinVtbl FileAsyncReaderPin_Vtbl = 
{
    FileAsyncReaderPin_QueryInterface,
    IPinImpl_AddRef,
    FileAsyncReaderPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    FileAsyncReaderPin_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* this differs from standard OutputPin_ConnectSpecific only in that it
 * doesn't need the IMemInputPin interface on the receiving pin */
static HRESULT FileAsyncReaderPin_ConnectSpecific(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    OutputPin *This = (OutputPin *)iface;
    HRESULT hr;

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* FIXME: call queryacceptproc */

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, iface, pmt);

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

static HRESULT FileAsyncReader_Construct(HANDLE hFile, IBaseFilter * pBaseFilter, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    FileAsyncReader * pPinImpl;
    PIN_INFO piOutput;

    *ppPin = NULL;

    pPinImpl = CoTaskMemAlloc(sizeof(*pPinImpl));

    if (!pPinImpl)
        return E_OUTOFMEMORY;

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = pBaseFilter;
    strcpyW(piOutput.achName, wszOutputPinName);

    if (SUCCEEDED(OutputPin_Init(&piOutput, NULL, pBaseFilter, AcceptProcAFR, pCritSec, &pPinImpl->pin)))
    {
        pPinImpl->pin.pin.lpVtbl = &FileAsyncReaderPin_Vtbl;
        pPinImpl->lpVtblAR = &FileAsyncReader_Vtbl;
        pPinImpl->hFile = hFile;
        pPinImpl->hEvent = CreateEventW(NULL, 0, 0, NULL);
        pPinImpl->bFlushing = FALSE;
        pPinImpl->pHead = NULL;
        pPinImpl->pin.pConnectSpecific = FileAsyncReaderPin_ConnectSpecific;
        InitializeCriticalSection(&pPinImpl->csList);
        pPinImpl->csList.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FileAsyncReader.csList");

        *ppPin = (IPin *)(&pPinImpl->pin.pin.lpVtbl);
        return S_OK;
    }
    return E_FAIL;
}

/* IAsyncReader */

static HRESULT WINAPI FileAsyncReader_QueryInterface(IAsyncReader * iface, REFIID riid, LPVOID * ppv)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_QueryInterface((IPin *)This, riid, ppv);
}

static ULONG WINAPI FileAsyncReader_AddRef(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_AddRef((IPin *)This);
}

static ULONG WINAPI FileAsyncReader_Release(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_Release((IPin *)This);
}

#define DEF_ALIGNMENT 1

static HRESULT WINAPI FileAsyncReader_RequestAllocator(IAsyncReader * iface, IMemAllocator * pPreferred, ALLOCATOR_PROPERTIES * pProps, IMemAllocator ** ppActual)
{
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %p)\n", pPreferred, pProps, ppActual);

    if (!pProps->cbAlign || (pProps->cbAlign % DEF_ALIGNMENT) != 0)
        pProps->cbAlign = DEF_ALIGNMENT;

    if (pPreferred)
    {
        ALLOCATOR_PROPERTIES PropsActual;
        hr = IMemAllocator_SetProperties(pPreferred, pProps, &PropsActual);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            IMemAllocator_AddRef(pPreferred);
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
            return S_OK;
        }
    }

    pPreferred = NULL;

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC, &IID_IMemAllocator, (LPVOID *)&pPreferred);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES PropsActual;
        hr = IMemAllocator_SetProperties(pPreferred, pProps, &PropsActual);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
            return S_OK;
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
    REFERENCE_TIME Start;
    REFERENCE_TIME Stop;
    DATAREQUEST * pDataRq;
    BYTE * pBuffer;
    HRESULT hr = S_OK;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%p, %lx)\n", pSample, dwUser);

    /* check flushing state */
    if (This->bFlushing)
        return VFW_E_WRONG_STATE;

    if (!(pDataRq = CoTaskMemAlloc(sizeof(*pDataRq))))
        hr = E_OUTOFMEMORY;

    /* get start and stop positions in bytes */
    if (SUCCEEDED(hr))
        hr = IMediaSample_GetTime(pSample, &Start, &Stop);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(pSample, &pBuffer);

    if (SUCCEEDED(hr))
    {
        DWORD dwLength = (DWORD) BYTES_FROM_MEDIATIME(Stop - Start);

        pDataRq->ovl.u.s.Offset = (DWORD) BYTES_FROM_MEDIATIME(Start);
        pDataRq->ovl.u.s.OffsetHigh = (DWORD)(BYTES_FROM_MEDIATIME(Start) >> (sizeof(DWORD) * 8));
        pDataRq->ovl.hEvent = This->hEvent;
        pDataRq->dwUserData = dwUser;
        pDataRq->pNext = NULL;
        /* we violate traditional COM rules here by maintaining
         * a reference to the sample, but not calling AddRef, but
         * that's what MSDN says to do */
        pDataRq->pSample = pSample;

        EnterCriticalSection(&This->csList);
        {
            if (This->pHead)
                /* adds data request to end of list */
                queue(This->pHead, pDataRq);
            else
                This->pHead = pDataRq;
        }
        LeaveCriticalSection(&This->csList);

        /* this is definitely not how it is implemented on Win9x
         * as they do not support async reads on files, but it is
         * sooo much easier to use this than messing around with threads!
         */
        if (!ReadFile(This->hFile, pBuffer, dwLength, NULL, &pDataRq->ovl))
            hr = HRESULT_FROM_WIN32(GetLastError());

        /* ERROR_IO_PENDING is not actually an error since this is what we want! */
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            hr = S_OK;
    }

    if (FAILED(hr) && pDataRq)
    {
        EnterCriticalSection(&This->csList);
        {
            DATAREQUEST * pCurrent;
            for (pCurrent = This->pHead; pCurrent && pCurrent->pNext; pCurrent = pCurrent->pNext)
                if (pCurrent->pNext == pDataRq)
                {
                    pCurrent->pNext = pDataRq->pNext;
                    break;
                }
        }
        LeaveCriticalSection(&This->csList);
        CoTaskMemFree(pDataRq);
    }

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_WaitForNext(IAsyncReader * iface, DWORD dwTimeout, IMediaSample ** ppSample, DWORD_PTR * pdwUser)
{
    HRESULT hr = S_OK;
    DWORD dwBytes = 0;
    DATAREQUEST * pDataRq = NULL;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%u, %p, %p)\n", dwTimeout, ppSample, pdwUser);

    /* FIXME: we could do with improving this by waiting for an array of event handles
     * and then determining which one finished and removing that from the list, otherwise
     * we will end up waiting for longer than we should do, if a later request finishes
     * before an earlier one */

    *ppSample = NULL;
    *pdwUser = 0;

    /* we return immediately if flushing */
    if (This->bFlushing)
        hr = VFW_E_WRONG_STATE;

    if (SUCCEEDED(hr))
    {
        /* wait for the read to finish or timeout */
        if (WaitForSingleObject(This->hEvent, dwTimeout) == WAIT_TIMEOUT)
            hr = VFW_E_TIMEOUT;
    }
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&This->csList);
        {
            pDataRq = This->pHead;
            if (pDataRq == NULL)
                hr = E_FAIL;
            else
                This->pHead = pDataRq->pNext;
        }
        LeaveCriticalSection(&This->csList);
    }

    if (SUCCEEDED(hr))
    {
        /* get any errors */
        if (!GetOverlappedResult(This->hFile, &pDataRq->ovl, &dwBytes, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (SUCCEEDED(hr))
    {
        IMediaSample_SetActualDataLength(pDataRq->pSample, dwBytes);
        *ppSample = pDataRq->pSample;
        *pdwUser = pDataRq->dwUserData;
    }

    /* no need to close event handle since we will close it when the pin is destroyed */
    CoTaskMemFree(pDataRq);
    
    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader * iface, LONGLONG llPosition, LONG lLength, BYTE * pBuffer);

static HRESULT WINAPI FileAsyncReader_SyncReadAligned(IAsyncReader * iface, IMediaSample * pSample)
{
    BYTE * pBuffer;
    REFERENCE_TIME tStart;
    REFERENCE_TIME tStop;
    HRESULT hr;

    TRACE("(%p)\n", pSample);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(pSample, &pBuffer);

    if (SUCCEEDED(hr))
        hr = FileAsyncReader_SyncRead(iface, 
            BYTES_FROM_MEDIATIME(tStart),
            (LONG) BYTES_FROM_MEDIATIME(tStop - tStart),
            pBuffer);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader * iface, LONGLONG llPosition, LONG lLength, BYTE * pBuffer)
{
    OVERLAPPED ovl;
    HRESULT hr = S_OK;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%x%08x, %d, %p)\n", (ULONG)(llPosition >> 32), (ULONG)llPosition, lLength, pBuffer);

    ZeroMemory(&ovl, sizeof(ovl));

    ovl.hEvent = CreateEventW(NULL, 0, 0, NULL);
    /* NOTE: llPosition is the actual byte position to start reading from */
    ovl.u.s.Offset = (DWORD) llPosition;
    ovl.u.s.OffsetHigh = (DWORD) (llPosition >> (sizeof(DWORD) * 8));

    if (!ReadFile(This->hFile, pBuffer, lLength, NULL, &ovl))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
        hr = S_OK;

    if (SUCCEEDED(hr))
    {
        DWORD dwBytesRead;

        if (!GetOverlappedResult(This->hFile, &ovl, &dwBytesRead, TRUE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(ovl.hEvent);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_Length(IAsyncReader * iface, LONGLONG * pTotal, LONGLONG * pAvailable)
{
    DWORD dwSizeLow;
    DWORD dwSizeHigh;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%p, %p)\n", pTotal, pAvailable);

    if (((dwSizeLow = GetFileSize(This->hFile, &dwSizeHigh)) == -1) &&
        (GetLastError() != NO_ERROR))
        return HRESULT_FROM_WIN32(GetLastError());

    *pTotal = (LONGLONG)dwSizeLow | (LONGLONG)dwSizeHigh << (sizeof(DWORD) * 8);

    *pAvailable = *pTotal;

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_BeginFlush(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("()\n");

    This->bFlushing = TRUE;
    CancelIo(This->hFile);
    SetEvent(This->hEvent);
    
    /* FIXME: free list */

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_EndFlush(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("()\n");

    This->bFlushing = FALSE;

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
