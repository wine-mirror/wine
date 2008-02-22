/*
 * Background Copy Job Interface for BITS
 *
 * Copyright 2007 Google (Roy Shea)
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

#include "qmgr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qmgr);

static void BackgroundCopyJobDestructor(BackgroundCopyJobImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This->displayName);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IBackgroundCopyJob_AddRef(IBackgroundCopyJob* iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_QueryInterface(
    IBackgroundCopyJob* iface, REFIID riid, LPVOID *ppvObject)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IBackgroundCopyJob))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IBackgroundCopyJob_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IBackgroundCopyJob_Release(IBackgroundCopyJob* iface)
{
    BackgroundCopyJobImpl *This = (BackgroundCopyJobImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        BackgroundCopyJobDestructor(This);

    return ref;
}

/*** IBackgroundCopyJob methods ***/

static HRESULT WINAPI BITS_IBackgroundCopyJob_AddFileSet(
    IBackgroundCopyJob* iface,
    ULONG cFileCount,
    BG_FILE_INFO *pFileSet)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_AddFile(
    IBackgroundCopyJob* iface,
    LPCWSTR RemoteUrl,
    LPCWSTR LocalName)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_EnumFiles(
    IBackgroundCopyJob* iface,
    IEnumBackgroundCopyFiles **pEnum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Suspend(
    IBackgroundCopyJob* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Resume(
    IBackgroundCopyJob* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Cancel(
    IBackgroundCopyJob* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_Complete(
    IBackgroundCopyJob* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetId(
    IBackgroundCopyJob* iface,
    GUID *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetType(
    IBackgroundCopyJob* iface,
    BG_JOB_TYPE *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetProgress(
    IBackgroundCopyJob* iface,
    BG_JOB_PROGRESS *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetTimes(
    IBackgroundCopyJob* iface,
    BG_JOB_TIMES *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetState(
    IBackgroundCopyJob* iface,
    BG_JOB_STATE *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetError(
    IBackgroundCopyJob* iface,
    IBackgroundCopyError **ppError)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetOwner(
    IBackgroundCopyJob* iface,
    LPWSTR *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetDisplayName(
    IBackgroundCopyJob* iface,
    LPCWSTR Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetDisplayName(
    IBackgroundCopyJob* iface,
    LPWSTR *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetDescription(
    IBackgroundCopyJob* iface,
    LPCWSTR Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetDescription(
    IBackgroundCopyJob* iface,
    LPWSTR *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetPriority(
    IBackgroundCopyJob* iface,
    BG_JOB_PRIORITY Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetPriority(
    IBackgroundCopyJob* iface,
    BG_JOB_PRIORITY *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNotifyFlags(
    IBackgroundCopyJob* iface,
    ULONG Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNotifyFlags(
    IBackgroundCopyJob* iface,
    ULONG *pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNotifyInterface(
    IBackgroundCopyJob* iface,
    IUnknown *Val)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNotifyInterface(
    IBackgroundCopyJob* iface,
    IUnknown **pVal)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetMinimumRetryDelay(
    IBackgroundCopyJob* iface,
    ULONG Seconds)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetMinimumRetryDelay(
    IBackgroundCopyJob* iface,
    ULONG *Seconds)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetNoProgressTimeout(
    IBackgroundCopyJob* iface,
    ULONG Seconds)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetNoProgressTimeout(
    IBackgroundCopyJob* iface,
    ULONG *Seconds)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetErrorCount(
    IBackgroundCopyJob* iface,
    ULONG *Errors)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_SetProxySettings(
    IBackgroundCopyJob* iface,
    BG_JOB_PROXY_USAGE ProxyUsage,
    const WCHAR *ProxyList,
    const WCHAR *ProxyBypassList)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_GetProxySettings(
    IBackgroundCopyJob* iface,
    BG_JOB_PROXY_USAGE *pProxyUsage,
    LPWSTR *pProxyList,
    LPWSTR *pProxyBypassList)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyJob_TakeOwnership(
    IBackgroundCopyJob* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}


static const IBackgroundCopyJobVtbl BITS_IBackgroundCopyJob_Vtbl =
{
    BITS_IBackgroundCopyJob_QueryInterface,
    BITS_IBackgroundCopyJob_AddRef,
    BITS_IBackgroundCopyJob_Release,
    BITS_IBackgroundCopyJob_AddFileSet,
    BITS_IBackgroundCopyJob_AddFile,
    BITS_IBackgroundCopyJob_EnumFiles,
    BITS_IBackgroundCopyJob_Suspend,
    BITS_IBackgroundCopyJob_Resume,
    BITS_IBackgroundCopyJob_Cancel,
    BITS_IBackgroundCopyJob_Complete,
    BITS_IBackgroundCopyJob_GetId,
    BITS_IBackgroundCopyJob_GetType,
    BITS_IBackgroundCopyJob_GetProgress,
    BITS_IBackgroundCopyJob_GetTimes,
    BITS_IBackgroundCopyJob_GetState,
    BITS_IBackgroundCopyJob_GetError,
    BITS_IBackgroundCopyJob_GetOwner,
    BITS_IBackgroundCopyJob_SetDisplayName,
    BITS_IBackgroundCopyJob_GetDisplayName,
    BITS_IBackgroundCopyJob_SetDescription,
    BITS_IBackgroundCopyJob_GetDescription,
    BITS_IBackgroundCopyJob_SetPriority,
    BITS_IBackgroundCopyJob_GetPriority,
    BITS_IBackgroundCopyJob_SetNotifyFlags,
    BITS_IBackgroundCopyJob_GetNotifyFlags,
    BITS_IBackgroundCopyJob_SetNotifyInterface,
    BITS_IBackgroundCopyJob_GetNotifyInterface,
    BITS_IBackgroundCopyJob_SetMinimumRetryDelay,
    BITS_IBackgroundCopyJob_GetMinimumRetryDelay,
    BITS_IBackgroundCopyJob_SetNoProgressTimeout,
    BITS_IBackgroundCopyJob_GetNoProgressTimeout,
    BITS_IBackgroundCopyJob_GetErrorCount,
    BITS_IBackgroundCopyJob_SetProxySettings,
    BITS_IBackgroundCopyJob_GetProxySettings,
    BITS_IBackgroundCopyJob_TakeOwnership,
};

HRESULT BackgroundCopyJobConstructor(LPCWSTR displayName, BG_JOB_TYPE type,
                                     GUID *pJobId, LPVOID *ppObj)
{
    HRESULT hr;
    BackgroundCopyJobImpl *This;
    int n;

    TRACE("(%s,%d,%p)\n", debugstr_w(displayName), type, ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &BITS_IBackgroundCopyJob_Vtbl;
    This->ref = 1;
    This->type = type;

    n = (lstrlenW(displayName) + 1) *  sizeof *displayName;
    This->displayName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->displayName)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->displayName, displayName, n);

    hr = CoCreateGuid(&This->jobId);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, This->displayName);
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }
    memcpy(pJobId, &This->jobId, sizeof(GUID));

    *ppObj = &This->lpVtbl;
    return S_OK;
}
