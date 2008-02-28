/*
 * Queue Manager (BITS) File Enumerator
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

static void EnumBackgroundCopyFilesDestructor(EnumBackgroundCopyFilesImpl *This)
{
    ULONG i;

    for(i = 0; i < This->numFiles; i++)
        IBackgroundCopyFile_Release(This->files[i]);

    HeapFree(GetProcessHeap(), 0, This->files);
    HeapFree(GetProcessHeap(), 0, This);
}

static ULONG WINAPI BITS_IEnumBackgroundCopyFiles_AddRef(
    IEnumBackgroundCopyFiles* iface)
{
    EnumBackgroundCopyFilesImpl *This = (EnumBackgroundCopyFilesImpl *) iface;
    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_QueryInterface(
    IEnumBackgroundCopyFiles* iface,
    REFIID riid,
    void **ppvObject)
{
    EnumBackgroundCopyFilesImpl *This = (EnumBackgroundCopyFilesImpl *) iface;
    TRACE("IID: %s\n", debugstr_guid(riid));

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IEnumBackgroundCopyFiles))
    {
        *ppvObject = &This->lpVtbl;
        BITS_IEnumBackgroundCopyFiles_AddRef(iface);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI BITS_IEnumBackgroundCopyFiles_Release(
    IEnumBackgroundCopyFiles* iface)
{
    EnumBackgroundCopyFilesImpl *This = (EnumBackgroundCopyFilesImpl *) iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        EnumBackgroundCopyFilesDestructor(This);

    return ref;
}

/*** IEnumBackgroundCopyFiles methods ***/
static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Next(
    IEnumBackgroundCopyFiles* iface,
    ULONG celt,
    IBackgroundCopyFile **rgelt,
    ULONG *pceltFetched)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Skip(
    IEnumBackgroundCopyFiles* iface,
    ULONG celt)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Reset(
    IEnumBackgroundCopyFiles* iface)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_Clone(
    IEnumBackgroundCopyFiles* iface,
    IEnumBackgroundCopyFiles **ppenum)
{
    FIXME("Not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI BITS_IEnumBackgroundCopyFiles_GetCount(
    IEnumBackgroundCopyFiles* iface,
    ULONG *puCount)
{
    EnumBackgroundCopyFilesImpl *This = (EnumBackgroundCopyFilesImpl *) iface;
    *puCount = This->numFiles;
    return S_OK;
}

static const IEnumBackgroundCopyFilesVtbl BITS_IEnumBackgroundCopyFiles_Vtbl =
{
    BITS_IEnumBackgroundCopyFiles_QueryInterface,
    BITS_IEnumBackgroundCopyFiles_AddRef,
    BITS_IEnumBackgroundCopyFiles_Release,
    BITS_IEnumBackgroundCopyFiles_Next,
    BITS_IEnumBackgroundCopyFiles_Skip,
    BITS_IEnumBackgroundCopyFiles_Reset,
    BITS_IEnumBackgroundCopyFiles_Clone,
    BITS_IEnumBackgroundCopyFiles_GetCount
};

HRESULT EnumBackgroundCopyFilesConstructor(LPVOID *ppObj, IBackgroundCopyJob* iCopyJob)
{
    EnumBackgroundCopyFilesImpl *This;
    BackgroundCopyFileImpl *file;
    BackgroundCopyJobImpl *job = (BackgroundCopyJobImpl *) iCopyJob;
    ULONG i;

    TRACE("%p, %p)\n", ppObj, job);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &BITS_IEnumBackgroundCopyFiles_Vtbl;
    This->ref = 1;

    /* Create array of files */
    This->indexFiles = 0;
    This->numFiles = list_count(&job->files);
    This->files = NULL;
    if (This->numFiles > 0)
    {
        This->files = HeapAlloc(GetProcessHeap(), 0,
                                This->numFiles * sizeof This->files[0]);
        if (!This->files)
        {
            HeapFree(GetProcessHeap(), 0, This);
            return E_OUTOFMEMORY;
        }
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(file, &job->files, BackgroundCopyFileImpl, entryFromJob)
    {
        file->lpVtbl->AddRef((IBackgroundCopyFile *) file);
        This->files[i] = (IBackgroundCopyFile *) file;
        ++i;
    }

    *ppObj = &This->lpVtbl;
    return S_OK;
}
