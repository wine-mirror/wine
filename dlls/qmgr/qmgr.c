/*
 * Queue Manager (BITS) core functions
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

/* Destructor for instances of background copy manager */
static void BackgroundCopyManagerDestructor(BackgroundCopyManagerImpl *This)
{
    TRACE("%p\n", This);
    HeapFree(GetProcessHeap(), 0, This);
}

/* Add a reference to the iface pointer */
static ULONG WINAPI BITS_IBackgroundCopyManager_AddRef(
        IBackgroundCopyManager* iface)
{
    BackgroundCopyManagerImpl * This = (BackgroundCopyManagerImpl *)iface;
    ULONG ref;

    TRACE("\n");

    ref = InterlockedIncrement(&This->ref);
    return ref;
}

/* Attempt to provide a new interface to interact with iface */
static HRESULT WINAPI BITS_IBackgroundCopyManager_QueryInterface(
        IBackgroundCopyManager* iface,
        REFIID riid,
        LPVOID *ppvObject)
{
    BackgroundCopyManagerImpl * This = (BackgroundCopyManagerImpl *)iface;

    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IBackgroundCopyManager))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IBackgroundCopyManager_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

/* Release an interface to iface */
static ULONG WINAPI BITS_IBackgroundCopyManager_Release(
        IBackgroundCopyManager* iface)
{
    BackgroundCopyManagerImpl * This = (BackgroundCopyManagerImpl *)iface;
    ULONG ref;

    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        BackgroundCopyManagerDestructor(This);
    }
    return ref;
}

/*** IBackgroundCopyManager interface methods ***/

static HRESULT WINAPI BITS_IBackgroundCopyManager_CreateJob(
        IBackgroundCopyManager* iface,
        LPCWSTR DisplayName,
        BG_JOB_TYPE Type,
        GUID *pJobId,
        IBackgroundCopyJob **ppJob)
{
    TRACE("\n");
    return BackgroundCopyJobConstructor(DisplayName, Type, pJobId,
                                        (LPVOID *) ppJob);
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_GetJob(
        IBackgroundCopyManager* iface,
        REFGUID jobID,
        IBackgroundCopyJob **ppJob)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_EnumJobs(
        IBackgroundCopyManager* iface,
        DWORD dwFlags,
        IEnumBackgroundCopyJobs **ppEnum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IBackgroundCopyManager_GetErrorDescription(
        IBackgroundCopyManager* iface,
        HRESULT hResult,
        DWORD LanguageId,
        LPWSTR *pErrorDescription)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}


static const IBackgroundCopyManagerVtbl BITS_IBackgroundCopyManager_Vtbl =
{
    BITS_IBackgroundCopyManager_QueryInterface,
    BITS_IBackgroundCopyManager_AddRef,
    BITS_IBackgroundCopyManager_Release,
    BITS_IBackgroundCopyManager_CreateJob,
    BITS_IBackgroundCopyManager_GetJob,
    BITS_IBackgroundCopyManager_EnumJobs,
    BITS_IBackgroundCopyManager_GetErrorDescription
};

/* Constructor for instances of background copy manager */
HRESULT BackgroundCopyManagerConstructor(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    BackgroundCopyManagerImpl *This;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        return E_OUTOFMEMORY;
    }

    This->lpVtbl = &BITS_IBackgroundCopyManager_Vtbl;
    This->ref = 1;

    *ppObj = &This->lpVtbl;
    return S_OK;
}
