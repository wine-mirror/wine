/*
 * File System Bind Data object to use as parameter for the bind context to
 * IShellFolder_ParseDisplayName
 *
 * Copyright 2003 Rolf Kalbermatter
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
 *
 */

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shlobj.h"
#include "shell32_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pidl);

/***********************************************************************
 * IFileSystemBindData implementation
 */
typedef struct
{
    IFileSystemBindData IFileSystemBindData_iface;
    LONG ref;
    WIN32_FIND_DATAW findFile;
} FileSystemBindData;

static inline FileSystemBindData *impl_from_IFileSystemBindData(IFileSystemBindData *iface)
{
    return CONTAINING_RECORD(iface, FileSystemBindData, IFileSystemBindData_iface);
}

static HRESULT WINAPI FileSystemBindData_QueryInterface(
                IFileSystemBindData *iface, REFIID riid, LPVOID *ppV)
{
    FileSystemBindData *This = impl_from_IFileSystemBindData(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppV);

    *ppV = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IFileSystemBindData))
    {
        *ppV = &This->IFileSystemBindData_iface;
    }

    if (*ppV)
    {
        IFileSystemBindData_AddRef(iface);
        TRACE("-- Interface: (%p)->(%p)\n", ppV, *ppV);
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI FileSystemBindData_AddRef(IFileSystemBindData *iface)
{
    FileSystemBindData *This = impl_from_IFileSystemBindData(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI FileSystemBindData_Release(IFileSystemBindData *iface)
{
    FileSystemBindData *This = impl_from_IFileSystemBindData(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI FileSystemBindData_GetFindData(IFileSystemBindData *iface, WIN32_FIND_DATAW *pfd)
{
    FileSystemBindData *This = impl_from_IFileSystemBindData(iface);

    TRACE("(%p)->(%p)\n", This, pfd);

    if (!pfd)
        return E_INVALIDARG;

    *pfd = This->findFile;
    return S_OK;
}

static HRESULT WINAPI FileSystemBindData_SetFindData(IFileSystemBindData *iface, const WIN32_FIND_DATAW *pfd)
{
    FileSystemBindData *This = impl_from_IFileSystemBindData(iface);

    TRACE("(%p)->(%p)\n", This, pfd);

    if (pfd)
        This->findFile = *pfd;
    else
        memset(&This->findFile, 0, sizeof(WIN32_FIND_DATAW));
    return S_OK;
}

static const IFileSystemBindDataVtbl FileSystemBindDataVtbl = {
    FileSystemBindData_QueryInterface,
    FileSystemBindData_AddRef,
    FileSystemBindData_Release,
    FileSystemBindData_SetFindData,
    FileSystemBindData_GetFindData,
};

HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *find_data, LPBC *ppV)
{
    FileSystemBindData *This;
    HRESULT ret;

    TRACE("(%p %p)\n", find_data, ppV);

    if (!ppV)
       return E_INVALIDARG;

    *ppV = NULL;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IFileSystemBindData_iface.lpVtbl = &FileSystemBindDataVtbl;
    This->ref = 1;
    IFileSystemBindData_SetFindData(&This->IFileSystemBindData_iface, find_data);

    ret = CreateBindCtx(0, ppV);
    if (SUCCEEDED(ret))
    {
        BIND_OPTS bindOpts;

        bindOpts.cbStruct = sizeof(BIND_OPTS);
        bindOpts.grfFlags = 0;
        bindOpts.grfMode = STGM_CREATE;
        bindOpts.dwTickCountDeadline = 0;
        IBindCtx_SetBindOptions(*ppV, &bindOpts);
        IBindCtx_RegisterObjectParam(*ppV, (WCHAR*)L"File System Bind Data", (IUnknown*)&This->IFileSystemBindData_iface);

        IFileSystemBindData_Release(&This->IFileSystemBindData_iface);
    }
    else
        free(This);
    return ret;
}
