/*
 * COM proxy/stub factory (CStdPSFactory) implementation
 *
 * Copyright 2001 Ove Kåven, TransGaming Technologies
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
#include <stdio.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "objbase.h"

#include "rpcproxy.h"

#include "wine/debug.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static void format_clsid( WCHAR *buffer, const CLSID *clsid )
{
    swprintf( buffer, 39, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              clsid->Data1, clsid->Data2, clsid->Data3,
              clsid->Data4[0], clsid->Data4[1], clsid->Data4[2], clsid->Data4[3],
              clsid->Data4[4], clsid->Data4[5], clsid->Data4[6], clsid->Data4[7] );

}

static BOOL FindProxyInfo(const ProxyFileInfo **pProxyFileList, REFIID riid, const ProxyFileInfo **pProxyInfo, int *pIndex)
{
  while (*pProxyFileList) {
    if ((*pProxyFileList)->pIIDLookupRtn(riid, pIndex)) {
      *pProxyInfo = *pProxyFileList;
      TRACE("found: ProxyInfo %p Index %d\n", *pProxyInfo, *pIndex);
      return TRUE;
    }
    pProxyFileList++;
  }
  TRACE("not found\n");
  return FALSE;
}

static HRESULT WINAPI CStdPSFactory_QueryInterface(LPPSFACTORYBUFFER iface,
                                                  REFIID riid,
                                                  LPVOID *obj)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->QueryInterface(%s,%p)\n",iface,debugstr_guid(riid),obj);
  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(&IID_IPSFactoryBuffer,riid)) {
    *obj = This;
    InterlockedIncrement( &This->RefCount );
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG WINAPI CStdPSFactory_AddRef(LPPSFACTORYBUFFER iface)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->AddRef()\n",iface);
  return InterlockedIncrement( &This->RefCount );
}

static ULONG WINAPI CStdPSFactory_Release(LPPSFACTORYBUFFER iface)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  TRACE("(%p)->Release()\n",iface);
  return InterlockedDecrement( &This->RefCount );
}

static HRESULT WINAPI CStdPSFactory_CreateProxy(LPPSFACTORYBUFFER iface,
                                               LPUNKNOWN pUnkOuter,
                                               REFIID riid,
                                               LPRPCPROXYBUFFER *ppProxy,
                                               LPVOID *ppv)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  const ProxyFileInfo *ProxyInfo;
  int Index;
  TRACE("(%p)->CreateProxy(%p,%s,%p,%p)\n",iface,pUnkOuter,
       debugstr_guid(riid),ppProxy,ppv);
  if (!FindProxyInfo(This->pProxyFileList,riid,&ProxyInfo,&Index))
    return E_NOINTERFACE;
  return StdProxy_Construct(riid, pUnkOuter, ProxyInfo, Index, iface, ppProxy, ppv);
}

static HRESULT WINAPI CStdPSFactory_CreateStub(LPPSFACTORYBUFFER iface,
                                              REFIID riid,
                                              LPUNKNOWN pUnkServer,
                                              LPRPCSTUBBUFFER *ppStub)
{
  CStdPSFactoryBuffer *This = (CStdPSFactoryBuffer *)iface;
  const ProxyFileInfo *ProxyInfo;
  int Index;
  TRACE("(%p)->CreateStub(%s,%p,%p)\n",iface,debugstr_guid(riid),
       pUnkServer,ppStub);
  if (!FindProxyInfo(This->pProxyFileList,riid,&ProxyInfo,&Index))
    return E_NOINTERFACE;

  if(ProxyInfo->pDelegatedIIDs && ProxyInfo->pDelegatedIIDs[Index])
    return  CStdStubBuffer_Delegating_Construct(riid, pUnkServer, ProxyInfo->pNamesArray[Index],
                                                ProxyInfo->pStubVtblList[Index], ProxyInfo->pDelegatedIIDs[Index],
                                                iface, ppStub);

  return CStdStubBuffer_Construct(riid, pUnkServer, ProxyInfo->pNamesArray[Index],
                                  ProxyInfo->pStubVtblList[Index], iface, ppStub);
}

static const IPSFactoryBufferVtbl CStdPSFactory_Vtbl =
{
  CStdPSFactory_QueryInterface,
  CStdPSFactory_AddRef,
  CStdPSFactory_Release,
  CStdPSFactory_CreateProxy,
  CStdPSFactory_CreateStub
};


static void init_psfactory( CStdPSFactoryBuffer *psfac, const ProxyFileInfo **file_list )
{
    DWORD i, j, k;

    psfac->lpVtbl = &CStdPSFactory_Vtbl;
    psfac->RefCount = 0;
    psfac->pProxyFileList = file_list;
    for (i = 0; file_list[i]; i++)
    {
        const PCInterfaceProxyVtblList *proxies = file_list[i]->pProxyVtblList;
        const PCInterfaceStubVtblList *stubs = file_list[i]->pStubVtblList;

        for (j = 0; j < file_list[i]->TableSize; j++)
        {
            /* FIXME: i think that different vtables should be copied for
             * async interfaces */
            void * const *pSrcRpcStubVtbl = (void * const *)&CStdStubBuffer_Vtbl;
            void **pRpcStubVtbl = (void **)&stubs[j]->Vtbl;

            if (file_list[i]->pDelegatedIIDs && file_list[i]->pDelegatedIIDs[j])
            {
                void **vtbl = proxies[j]->Vtbl;
                if (file_list[i]->TableVersion > 1) vtbl++;
                fill_delegated_proxy_table( (IUnknownVtbl *)vtbl, stubs[j]->header.DispatchTableCount );
                pSrcRpcStubVtbl = (void * const *)&CStdStubBuffer_Delegating_Vtbl;
            }

            for (k = 0; k < sizeof(IRpcStubBufferVtbl)/sizeof(void *); k++)
                if (!pRpcStubVtbl[k]) pRpcStubVtbl[k] = pSrcRpcStubVtbl[k];
        }
    }
}


/***********************************************************************
 *           NdrDllGetClassObject [RPCRT4.@]
 */
HRESULT WINAPI NdrDllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv,
                                   const ProxyFileInfo **pProxyFileList,
                                   const CLSID *pclsid,
                                   CStdPSFactoryBuffer *pPSFactoryBuffer)
{
  TRACE("(%s, %s, %p, %p, %s, %p)\n", debugstr_guid(rclsid),
    debugstr_guid(iid), ppv, pProxyFileList, debugstr_guid(pclsid),
    pPSFactoryBuffer);

  *ppv = NULL;
  if (!pPSFactoryBuffer->lpVtbl) init_psfactory( pPSFactoryBuffer, pProxyFileList );

  if (pclsid && IsEqualGUID(rclsid, pclsid))
    return IPSFactoryBuffer_QueryInterface((LPPSFACTORYBUFFER)pPSFactoryBuffer, iid, ppv);
  else {
    const ProxyFileInfo *info;
    int index;
    /* otherwise, the dll may be using the iid as the clsid, so
     * search for it in the proxy file list */
    if (FindProxyInfo(pProxyFileList, rclsid, &info, &index))
      return IPSFactoryBuffer_QueryInterface((LPPSFACTORYBUFFER)pPSFactoryBuffer, iid, ppv);

    WARN("class %s not available\n", debugstr_guid(rclsid));
    return CLASS_E_CLASSNOTAVAILABLE;
  }
}

/***********************************************************************
 *           NdrDllCanUnloadNow [RPCRT4.@]
 */
HRESULT WINAPI NdrDllCanUnloadNow(CStdPSFactoryBuffer *pPSFactoryBuffer)
{
  return pPSFactoryBuffer->RefCount != 0 ? S_FALSE : S_OK;
}


/***********************************************************************
 *           NdrDllRegisterProxy [RPCRT4.@]
 */
HRESULT WINAPI NdrDllRegisterProxy(HMODULE hDll,
                                  const ProxyFileInfo **pProxyFileList,
                                  const CLSID *pclsid)
{
  WCHAR clsid[39], keyname[50], module[MAX_PATH];
  HKEY key, subkey;
  DWORD len;

  TRACE("(%p,%p,%s)\n", hDll, pProxyFileList, debugstr_guid(pclsid));

  if (!hDll) return E_HANDLE;
  if (!*pProxyFileList) return E_NOINTERFACE;

  if (pclsid)
      format_clsid( clsid, pclsid );
  else if ((*pProxyFileList)->TableSize > 0)
      format_clsid( clsid,(*pProxyFileList)->pStubVtblList[0]->header.piid);
  else
      return E_NOINTERFACE;

  /* register interfaces to point to clsid */
  while (*pProxyFileList) {
    unsigned u;
    for (u=0; u<(*pProxyFileList)->TableSize; u++) {
      CInterfaceStubVtbl *proxy = (*pProxyFileList)->pStubVtblList[u];
      PCInterfaceName name = (*pProxyFileList)->pNamesArray[u];

      TRACE("registering %s %s => %s\n",
            debugstr_a(name), debugstr_guid(proxy->header.piid), debugstr_w(clsid));

      lstrcpyW( keyname, L"Interface\\" );
      format_clsid( keyname + lstrlenW(keyname), proxy->header.piid );
      if (RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key) == ERROR_SUCCESS) {
        WCHAR num[10];
        if (name)
          RegSetValueExA(key, NULL, 0, REG_SZ, (const BYTE *)name, strlen(name)+1);
        RegSetValueW( key, L"ProxyStubClsid32", REG_SZ, clsid, 0 );
        swprintf(num, ARRAY_SIZE(num), L"%u", proxy->header.DispatchTableCount);
        RegSetValueW( key, L"NumMethods", REG_SZ, num, 0 );
        RegCloseKey(key);
      }
    }
    pProxyFileList++;
  }

  /* register clsid to point to module */
  lstrcpyW( keyname, L"CLSID\\" );
  lstrcatW( keyname, clsid );
  len = GetModuleFileNameW(hDll, module, ARRAY_SIZE(module));
  if (len && len < sizeof(module)) {
      TRACE("registering CLSID %s => %s\n", debugstr_w(clsid), debugstr_w(module));
      if (RegCreateKeyW(HKEY_CLASSES_ROOT, keyname, &key) == ERROR_SUCCESS) {
          RegSetValueExW(key, NULL, 0, REG_SZ, (const BYTE *)L"PSFactoryBuffer", sizeof(L"PSFactoryBuffer"));
          if (RegCreateKeyW(key, L"InProcServer32", &subkey) == ERROR_SUCCESS) {
              RegSetValueExW(subkey, NULL, 0, REG_SZ, (LPBYTE)module, (lstrlenW(module)+1)*sizeof(WCHAR));
              RegSetValueExW(subkey, L"ThreadingModel", 0, REG_SZ, (const BYTE *)L"Both", sizeof(L"Both"));
              RegCloseKey(subkey);
          }
          RegCloseKey(key);
      }
  }

  return S_OK;
}

/***********************************************************************
 *           NdrDllUnregisterProxy [RPCRT4.@]
 */
HRESULT WINAPI NdrDllUnregisterProxy(HMODULE hDll,
                                    const ProxyFileInfo **pProxyFileList,
                                    const CLSID *pclsid)
{
  WCHAR keyname[50];
  WCHAR clsid[39];

  TRACE("(%p,%p,%s)\n", hDll, pProxyFileList, debugstr_guid(pclsid));
  if (pclsid)
      format_clsid( clsid, pclsid );
  else if ((*pProxyFileList)->TableSize > 0)
      format_clsid( clsid,(*pProxyFileList)->pStubVtblList[0]->header.piid);
  else
      return E_NOINTERFACE;

  /* unregister interfaces */
  while (*pProxyFileList) {
    unsigned u;
    for (u=0; u<(*pProxyFileList)->TableSize; u++) {
      CInterfaceStubVtbl *proxy = (*pProxyFileList)->pStubVtblList[u];
      PCInterfaceName name = (*pProxyFileList)->pNamesArray[u];

      TRACE("unregistering %s %s\n", debugstr_a(name), debugstr_guid(proxy->header.piid));

      lstrcpyW( keyname, L"Interface\\" );
      format_clsid( keyname + lstrlenW(keyname), proxy->header.piid );
      RegDeleteTreeW(HKEY_CLASSES_ROOT, keyname);
    }
    pProxyFileList++;
  }

  /* unregister clsid */
  lstrcpyW( keyname, L"CLSID\\" );
  lstrcatW( keyname, clsid );
  RegDeleteTreeW(HKEY_CLASSES_ROOT, keyname);

  return S_OK;
}
