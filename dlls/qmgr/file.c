/*
 * Queue Manager (BITS) File
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

static void BackgroundCopyFileDestructor(BackgroundCopyFileImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This->info.LocalName);
    HeapFree(GetProcessHeap(), 0, This->info.RemoteName);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IBackgroundCopyFile_AddRef(IBackgroundCopyFile* iface)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IBackgroundCopyFile_QueryInterface(
    IBackgroundCopyFile* iface,
    REFIID riid,
    void **ppvObject)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IBackgroundCopyFile))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IBackgroundCopyFile_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}


static ULONG WINAPI BITS_IBackgroundCopyFile_Release(
    IBackgroundCopyFile* iface)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        BackgroundCopyFileDestructor(This);

    return ref;
}

/* Get the remote name of a background copy file */
static HRESULT WINAPI BITS_IBackgroundCopyFile_GetRemoteName(
    IBackgroundCopyFile* iface,
    LPWSTR *pVal)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;
    int n = (lstrlenW(This->info.RemoteName) + 1) * sizeof(WCHAR);

    *pVal = CoTaskMemAlloc(n);
    if (!*pVal)
        return E_OUTOFMEMORY;

    memcpy(*pVal, This->info.RemoteName, n);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyFile_GetLocalName(
    IBackgroundCopyFile* iface,
    LPWSTR *pVal)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;
    int n = (lstrlenW(This->info.LocalName) + 1) * sizeof(WCHAR);

    *pVal = CoTaskMemAlloc(n);
    if (!*pVal)
        return E_OUTOFMEMORY;

    memcpy(*pVal, This->info.LocalName, n);
    return S_OK;
}

static HRESULT WINAPI BITS_IBackgroundCopyFile_GetProgress(
    IBackgroundCopyFile* iface,
    BG_FILE_PROGRESS *pVal)
{
    BackgroundCopyFileImpl *This = (BackgroundCopyFileImpl *) iface;

    pVal->BytesTotal = This->fileProgress.BytesTotal;
    pVal->BytesTransferred = This->fileProgress.BytesTransferred;
    pVal->Completed = This->fileProgress.Completed;

    return S_OK;
}

static const IBackgroundCopyFileVtbl BITS_IBackgroundCopyFile_Vtbl =
{
    BITS_IBackgroundCopyFile_QueryInterface,
    BITS_IBackgroundCopyFile_AddRef,
    BITS_IBackgroundCopyFile_Release,
    BITS_IBackgroundCopyFile_GetRemoteName,
    BITS_IBackgroundCopyFile_GetLocalName,
    BITS_IBackgroundCopyFile_GetProgress
};

HRESULT BackgroundCopyFileConstructor(LPCWSTR remoteName,
                                      LPCWSTR localName, LPVOID *ppObj)
{
    BackgroundCopyFileImpl *This;
    int n;

    TRACE("(%s,%s,%p)\n", debugstr_w(remoteName),
            debugstr_w(localName), ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    n = (lstrlenW(remoteName) + 1) * sizeof(WCHAR);
    This->info.RemoteName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->info.RemoteName)
    {
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->info.RemoteName, remoteName, n);

    n = (lstrlenW(localName) + 1) * sizeof(WCHAR);
    This->info.LocalName = HeapAlloc(GetProcessHeap(), 0, n);
    if (!This->info.LocalName)
    {
        HeapFree(GetProcessHeap(), 0, This->info.RemoteName);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }
    memcpy(This->info.LocalName, localName, n);

    This->lpVtbl = &BITS_IBackgroundCopyFile_Vtbl;
    This->ref = 1;

    This->fileProgress.BytesTotal = BG_SIZE_UNKNOWN;
    This->fileProgress.BytesTransferred = 0;
    This->fileProgress.Completed = FALSE;

    *ppObj = &This->lpVtbl;
    return S_OK;
}
