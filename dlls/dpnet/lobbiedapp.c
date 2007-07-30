/*
 * DirectPlay8 LobbiedApplication
 *
 * Copyright 2007 Jason Edmeades
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dplobby8.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

/* IDirectPlay8LobbiedApplication IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_QueryInterface(
                                  PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                  REFIID riid,
                                  LPVOID *ppobj) {
    IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirectPlay8LobbiedApplication)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8LobbiedApplicationImpl_AddRef(
                                PDIRECTPLAY8LOBBIEDAPPLICATION iface) {
    IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDirectPlay8LobbiedApplicationImpl_Release(
                                PDIRECTPLAY8LOBBIEDAPPLICATION iface) {
    IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

/* IDirectPlay8LobbiedApplication Interface follow: */

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Initialize(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                                CONST PVOID pvUserContext,
                                                                CONST PFNDPNMESSAGEHANDLER pfn,
                                                                DPNHANDLE* CONST pdpnhConnection,
                                                                CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_RegisterProgram(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                                PDPL_PROGRAM_DESC pdplProgramDesc,
                                                                CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_UnRegisterProgram(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                                GUID* pguidApplication,
                                                                CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Send(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST DPNHANDLE hConnection,
                                                          BYTE* CONST pBuffer,
                                                          CONST DWORD pBufferSize,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_SetAppAvailable(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST BOOL fAvailable,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_UpdateStatus(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST DPNHANDLE hConnection,
                                                          CONST DWORD dwStatus,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_Close(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_GetConnectionSettings(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST DPNHANDLE hConnection,
                                                          DPL_CONNECTION_SETTINGS* CONST pdplSessionInfo,
                                                          DWORD* pdwInfoSize,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8LobbiedApplicationImpl_SetConnectionSettings(PDIRECTPLAY8LOBBIEDAPPLICATION iface,
                                                          CONST DPNHANDLE hConnection,
                                                          CONST DPL_CONNECTION_SETTINGS* CONST pdplSessionInfo,
                                                          CONST DWORD dwFlags) {
  IDirectPlay8LobbiedApplicationImpl *This = (IDirectPlay8LobbiedApplicationImpl *)iface;
  FIXME("(%p): stub\n", This);
  return DPN_OK;
}

static const IDirectPlay8LobbiedApplicationVtbl DirectPlay8LobbiedApplication_Vtbl =
{
    IDirectPlay8LobbiedApplicationImpl_QueryInterface,
    IDirectPlay8LobbiedApplicationImpl_AddRef,
    IDirectPlay8LobbiedApplicationImpl_Release,
    IDirectPlay8LobbiedApplicationImpl_Initialize,
    IDirectPlay8LobbiedApplicationImpl_RegisterProgram,
    IDirectPlay8LobbiedApplicationImpl_UnRegisterProgram,
    IDirectPlay8LobbiedApplicationImpl_Send,
    IDirectPlay8LobbiedApplicationImpl_SetAppAvailable,
    IDirectPlay8LobbiedApplicationImpl_UpdateStatus,
    IDirectPlay8LobbiedApplicationImpl_Close,
    IDirectPlay8LobbiedApplicationImpl_GetConnectionSettings,
    IDirectPlay8LobbiedApplicationImpl_SetConnectionSettings
};



HRESULT DPNET_CreateDirectPlay8LobbiedApp(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj) {
  IDirectPlay8LobbiedApplicationImpl* app;

  TRACE("(%p, %s, %p)\n", punkOuter, debugstr_guid(riid), ppobj);

  app = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8LobbiedApplicationImpl));
  if (NULL == app) {
    *ppobj = NULL;
    return E_OUTOFMEMORY;
  }
  app->lpVtbl = &DirectPlay8LobbiedApplication_Vtbl;
  app->ref = 0; /* will be inited with QueryInterface */
  return IDirectPlay8LobbiedApplicationImpl_QueryInterface ((PDIRECTPLAY8LOBBIEDAPPLICATION)app, riid, ppobj);
}
