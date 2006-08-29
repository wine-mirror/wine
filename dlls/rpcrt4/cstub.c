/*
 * COM stub (CStdStubBuffer) implementation
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "excpt.h"

#include "objbase.h"
#include "rpcproxy.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "cpsf.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define STUB_HEADER(This) (((CInterfaceStubHeader*)((This)->lpVtbl))[-1])

static WINE_EXCEPTION_FILTER(stub_filter)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    return EXCEPTION_EXECUTE_HANDLER;
}

HRESULT WINAPI CStdStubBuffer_Construct(REFIID riid,
                                       LPUNKNOWN pUnkServer,
                                       PCInterfaceName name,
                                       CInterfaceStubVtbl *vtbl,
                                       LPPSFACTORYBUFFER pPSFactory,
                                       LPRPCSTUBBUFFER *ppStub)
{
  CStdStubBuffer *This;
  IUnknown *pvServer;
  HRESULT r;
  TRACE("(%p,%p,%p,%p) %s\n", pUnkServer, vtbl, pPSFactory, ppStub, name);
  TRACE("iid=%s\n", debugstr_guid(vtbl->header.piid));
  TRACE("vtbl=%p\n", &vtbl->Vtbl);

  if (!IsEqualGUID(vtbl->header.piid, riid)) {
    ERR("IID mismatch during stub creation\n");
    return RPC_E_UNEXPECTED;
  }

  r = IUnknown_QueryInterface(pUnkServer, riid, (void**)&pvServer);
  if(FAILED(r))
    return r;

  This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CStdStubBuffer));
  if (!This) {
    IUnknown_Release(pvServer);
    return E_OUTOFMEMORY;
  }

  This->lpVtbl = &vtbl->Vtbl;
  This->RefCount = 1;
  This->pvServerObject = pvServer;
  This->pPSFactory = pPSFactory;
  *ppStub = (LPRPCSTUBBUFFER)This;

  IPSFactoryBuffer_AddRef(pPSFactory);
  return S_OK;
}

HRESULT WINAPI CStdStubBuffer_QueryInterface(LPRPCSTUBBUFFER iface,
                                            REFIID riid,
                                            LPVOID *obj)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualGUID(&IID_IUnknown,riid) ||
      IsEqualGUID(&IID_IRpcStubBuffer,riid)) {
    *obj = This;
    This->RefCount++;
    return S_OK;
  }
  return E_NOINTERFACE;
}

ULONG WINAPI CStdStubBuffer_AddRef(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->AddRef()\n",This);
  return ++(This->RefCount);
}

ULONG WINAPI NdrCStdStubBuffer_Release(LPRPCSTUBBUFFER iface,
                                      LPPSFACTORYBUFFER pPSF)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->Release()\n",This);

  if (!--(This->RefCount)) {
    IRpcStubBuffer_Disconnect(iface);
    if(This->pPSFactory)
        IPSFactoryBuffer_Release(This->pPSFactory);
    HeapFree(GetProcessHeap(),0,This);
    return 0;
  }
  return This->RefCount;
}

ULONG WINAPI NdrCStdStubBuffer2_Release(LPRPCSTUBBUFFER iface,
                                        LPPSFACTORYBUFFER pPSF)
{
    FIXME("Not implemented\n");
    return 0;
}

HRESULT WINAPI CStdStubBuffer_Connect(LPRPCSTUBBUFFER iface,
                                     LPUNKNOWN lpUnkServer)
{
    CStdStubBuffer *This = (CStdStubBuffer *)iface;
    HRESULT r;
    IUnknown *new = NULL;

    TRACE("(%p)->Connect(%p)\n",This,lpUnkServer);

    r = IUnknown_QueryInterface(lpUnkServer, STUB_HEADER(This).piid, (void**)&new);
    new = InterlockedExchangePointer((void**)&This->pvServerObject, new);
    if(new)
        IUnknown_Release(new);
    return r;
}

void WINAPI CStdStubBuffer_Disconnect(LPRPCSTUBBUFFER iface)
{
    CStdStubBuffer *This = (CStdStubBuffer *)iface;
    IUnknown *old;
    TRACE("(%p)->Disconnect()\n",This);

    old = InterlockedExchangePointer((void**)&This->pvServerObject, NULL);

    if(old)
        IUnknown_Release(old);
}

HRESULT WINAPI CStdStubBuffer_Invoke(LPRPCSTUBBUFFER iface,
                                    PRPCOLEMESSAGE pMsg,
                                    LPRPCCHANNELBUFFER pChannel)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  DWORD dwPhase = STUB_UNMARSHAL;
  HRESULT hr = S_OK;

  TRACE("(%p)->Invoke(%p,%p)\n",This,pMsg,pChannel);

  __TRY
  {
    if (STUB_HEADER(This).pDispatchTable)
      STUB_HEADER(This).pDispatchTable[pMsg->iMethod](iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
    else /* pure interpreted */
      NdrStubCall2(iface, pChannel, (PRPC_MESSAGE)pMsg, &dwPhase);
  }
  __EXCEPT(stub_filter)
  {
    DWORD dwExceptionCode = GetExceptionCode();
    WARN("a stub call failed with exception 0x%08lx (%ld)\n", dwExceptionCode, dwExceptionCode);
    if (FAILED(dwExceptionCode))
      hr = dwExceptionCode;
    else
      hr = HRESULT_FROM_WIN32(dwExceptionCode);
  }
  __ENDTRY

  return hr;
}

LPRPCSTUBBUFFER WINAPI CStdStubBuffer_IsIIDSupported(LPRPCSTUBBUFFER iface,
                                                    REFIID riid)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->IsIIDSupported(%s)\n",This,debugstr_guid(riid));
  return IsEqualGUID(STUB_HEADER(This).piid, riid) ? iface : NULL;
}

ULONG WINAPI CStdStubBuffer_CountRefs(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->CountRefs()\n",This);
  return This->RefCount;
}

HRESULT WINAPI CStdStubBuffer_DebugServerQueryInterface(LPRPCSTUBBUFFER iface,
                                                       LPVOID *ppv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->DebugServerQueryInterface(%p)\n",This,ppv);
  return S_OK;
}

void WINAPI CStdStubBuffer_DebugServerRelease(LPRPCSTUBBUFFER iface,
                                             LPVOID pv)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->DebugServerRelease(%p)\n",This,pv);
}

const IRpcStubBufferVtbl CStdStubBuffer_Vtbl =
{
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    NULL,
    CStdStubBuffer_Connect,
    CStdStubBuffer_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease
};

const MIDL_SERVER_INFO *CStdStubBuffer_GetServerInfo(IRpcStubBuffer *iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  return STUB_HEADER(This).pServerInfo;
}

/************************************************************************
 *           NdrStubForwardingFunction [RPCRT4.@]
 */
void __RPC_STUB NdrStubForwardingFunction( IRpcStubBuffer *This, IRpcChannelBuffer *pChannel,
                                           PRPC_MESSAGE pMsg, DWORD *pdwStubPhase )
{
    /* Once stub delegation is implemented, this should call
       IRpcStubBuffer_Invoke on the stub's base interface.  The
       IRpcStubBuffer for this interface is stored at (void**)This-1.
       The pChannel and pMsg parameters are passed intact
       (RPCOLEMESSAGE is basically a RPC_MESSAGE).  If Invoke returns
       with a failure then an exception is raised (to see this, change
       the return value in the test).

    IRpcStubBuffer *base_this = *(IRpcStubBuffer**)((void**)This - 1);
    HRESULT r = IRpcStubBuffer_Invoke(base_this, (RPCOLEMESSAGE*)pMsg, pChannel);
    if(FAILED(r)) RpcRaiseException(r);
    return;
    */

    FIXME("Not implemented\n");
    return;
}

/***********************************************************************
 *           NdrStubInitialize [RPCRT4.@]
 */
void WINAPI NdrStubInitialize(PRPC_MESSAGE pRpcMsg,
                             PMIDL_STUB_MESSAGE pStubMsg,
                             PMIDL_STUB_DESC pStubDescriptor,
                             LPRPCCHANNELBUFFER pRpcChannelBuffer)
{
  TRACE("(%p,%p,%p,%p)\n", pRpcMsg, pStubMsg, pStubDescriptor, pRpcChannelBuffer);
  NdrServerInitializeNew(pRpcMsg, pStubMsg, pStubDescriptor);
  pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;
}

/***********************************************************************
 *           NdrStubGetBuffer [RPCRT4.@]
 */
void WINAPI NdrStubGetBuffer(LPRPCSTUBBUFFER iface,
                            LPRPCCHANNELBUFFER pRpcChannelBuffer,
                            PMIDL_STUB_MESSAGE pStubMsg)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  HRESULT hr;

  TRACE("(%p, %p, %p)\n", This, pRpcChannelBuffer, pStubMsg);

  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  hr = IRpcChannelBuffer_GetBuffer(pRpcChannelBuffer,
    (RPCOLEMESSAGE *)pStubMsg->RpcMsg, STUB_HEADER(This).piid);
  if (FAILED(hr))
  {
    RpcRaiseException(hr);
    return;
  }

  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
}
