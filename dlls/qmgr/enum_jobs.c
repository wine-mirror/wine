/*
 * Queue Manager (BITS) Job Enumerator
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

static void EnumBackgroundCopyJobsDestructor(EnumBackgroundCopyJobsImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IEnumBackgroundCopyJobs_AddRef(
    IEnumBackgroundCopyJobs* iface)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_QueryInterface(
    IEnumBackgroundCopyJobs* iface,
    REFIID riid,
    void **ppvObject)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumBackgroundCopyJobs))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IEnumBackgroundCopyJobs_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IEnumBackgroundCopyJobs_Release(
    IEnumBackgroundCopyJobs* iface)
{
    EnumBackgroundCopyJobsImpl *This = (EnumBackgroundCopyJobsImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        EnumBackgroundCopyJobsDestructor(This);

    return ref;
}

/*** IEnumBackgroundCopyJobs methods ***/
static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Next(
    IEnumBackgroundCopyJobs* iface,
    ULONG celt,
    IBackgroundCopyJob **rgelt,
    ULONG *pceltFetched)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Skip(
    IEnumBackgroundCopyJobs* iface,
    ULONG celt)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Reset(
    IEnumBackgroundCopyJobs* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_Clone(
    IEnumBackgroundCopyJobs* iface,
    IEnumBackgroundCopyJobs **ppenum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyJobs_GetCount(
    IEnumBackgroundCopyJobs* iface,
    ULONG *puCount)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static const IEnumBackgroundCopyJobsVtbl BITS_IEnumBackgroundCopyJobs_Vtbl =
{
    BITS_IEnumBackgroundCopyJobs_QueryInterface,
    BITS_IEnumBackgroundCopyJobs_AddRef,
    BITS_IEnumBackgroundCopyJobs_Release,
    BITS_IEnumBackgroundCopyJobs_Next,
    BITS_IEnumBackgroundCopyJobs_Skip,
    BITS_IEnumBackgroundCopyJobs_Reset,
    BITS_IEnumBackgroundCopyJobs_Clone,
    BITS_IEnumBackgroundCopyJobs_GetCount
};

HRESULT EnumBackgroundCopyJobsConstructor(LPVOID *ppObj,
                                          IBackgroundCopyManager* copyManager)
{
    EnumBackgroundCopyJobsImpl *This;

    TRACE("%p, %p)\n", ppObj, copyManager);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &BITS_IEnumBackgroundCopyJobs_Vtbl;
    This->ref = 1;

    *ppObj = &This->lpVtbl;
    return S_OK;
}
