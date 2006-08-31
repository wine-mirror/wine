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

typedef struct
{
    IUnknownVtbl *base_obj;
    IRpcStubBuffer *base_stub;
    CStdStubBuffer stub_buffer;
} cstdstubbuffer_delegating_t;

static inline cstdstubbuffer_delegating_t *impl_from_delegating( IRpcStubBuffer *iface )
{
    return (cstdstubbuffer_delegating_t*)((char *)iface - FIELD_OFFSET(cstdstubbuffer_delegating_t, stub_buffer));
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

static CRITICAL_SECTION delegating_vtbl_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &delegating_vtbl_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": delegating_vtbl_section") }
};
static CRITICAL_SECTION delegating_vtbl_section = { &critsect_debug, -1, 0, 0, 0, 0 };

typedef struct
{
    DWORD ref;
    IUnknownVtbl vtbl;
} ref_counted_vtbl;

static struct
{
    ref_counted_vtbl *table;
    DWORD size;
} current_vtbl;


static HRESULT WINAPI delegating_QueryInterface(IUnknown *pUnk, REFIID iid, void **ppv)
{
    *ppv = (void *)pUnk;
    return S_OK;
}

static ULONG WINAPI delegating_AddRef(IUnknown *pUnk)
{
    return 1;
}

static ULONG WINAPI delegating_Release(IUnknown *pUnk)
{
    return 1;
}

#if defined(__i386__)

/* The idea here is to replace the first param on the stack
   ie. This (which will point to cstdstubbuffer_delegating_t)
   with This->stub_buffer.pvServerObject and then jump to the
   relevant offset in This->stub_buffer.pvServerObject's vtbl.
*/
#include "pshpack1.h"
typedef struct {
    DWORD mov1;    /* mov 0x4(%esp), %eax      8b 44 24 04 */
    WORD mov2;     /* mov 0x10(%eax), %eax     8b 40 */
    BYTE sixteen;  /*                          10   */
    DWORD mov3;    /* mov %eax, 0x4(%esp)      89 44 24 04 */
    WORD mov4;     /* mov (%eax), %eax         8b 00 */
    WORD mov5;     /* mov offset(%eax), %eax   8b 80 */
    DWORD offset;  /*                          xx xx xx xx */
    WORD jmp;      /* jmp *%eax                ff e0 */
    BYTE pad[3];   /* lea 0x0(%esi), %esi      8d 76 00 */
} vtbl_method_t;
#include "poppack.h"

static void fill_table(IUnknownVtbl *vtbl, DWORD num)
{
    vtbl_method_t *method;
    void **entry;
    DWORD i;

    vtbl->QueryInterface = delegating_QueryInterface;
    vtbl->AddRef = delegating_AddRef;
    vtbl->Release = delegating_Release;

    method = (vtbl_method_t*)((void **)vtbl + num);
    entry = (void**)(vtbl + 1);

    for(i = 3; i < num; i++)
    {
        *entry = method;
        method->mov1 = 0x0424448b;
        method->mov2 = 0x408b;
        method->sixteen = 0x10;
        method->mov3 = 0x04244489;
        method->mov4 = 0x008b;
        method->mov5 = 0x808b;
        method->offset = i << 2;
        method->jmp = 0xe0ff;
        method->pad[0] = 0x8d;
        method->pad[1] = 0x76;
        method->pad[2] = 0x00;

        method++;
        entry++;
    }
}

#else  /* __i386__ */

typedef struct {int dummy;} vtbl_method_t;
static void fill_table(IUnknownVtbl *vtbl, DWORD num)
{
    ERR("delegated stubs are not supported on this architecture\n");
}

#endif  /* __i386__ */

void create_delegating_vtbl(DWORD num_methods)
{
    TRACE("%ld\n", num_methods);
    if(num_methods <= 3)
    {
        ERR("should have more than %ld methods\n", num_methods);
        return;
    }

    EnterCriticalSection(&delegating_vtbl_section);
    if(num_methods > current_vtbl.size)
    {
        DWORD size;
        if(current_vtbl.table && current_vtbl.table->ref == 0)
        {
            TRACE("freeing old table\n");
            HeapFree(GetProcessHeap(), 0, current_vtbl.table);
        }
        size = sizeof(DWORD) + num_methods * sizeof(void*) + (num_methods - 3) * sizeof(vtbl_method_t);
        current_vtbl.table = HeapAlloc(GetProcessHeap(), 0, size);
        fill_table(&current_vtbl.table->vtbl, num_methods);
        current_vtbl.table->ref = 0;
        current_vtbl.size = num_methods;
    }
    LeaveCriticalSection(&delegating_vtbl_section);
}

static IUnknownVtbl *get_delegating_vtbl(void)
{
    IUnknownVtbl *ret;

    EnterCriticalSection(&delegating_vtbl_section);
    current_vtbl.table->ref++;
    ret = &current_vtbl.table->vtbl;
    LeaveCriticalSection(&delegating_vtbl_section);
    return ret;
}

static void release_delegating_vtbl(IUnknownVtbl *vtbl)
{
    ref_counted_vtbl *table = (ref_counted_vtbl*)((DWORD *)vtbl - 1);

    EnterCriticalSection(&delegating_vtbl_section);
    table->ref--;
    TRACE("ref now %ld\n", table->ref);
    if(table->ref == 0 && table != current_vtbl.table)
    {
        TRACE("... and we're not current so free'ing\n");
        HeapFree(GetProcessHeap(), 0, table);
    }
    LeaveCriticalSection(&delegating_vtbl_section);
}

HRESULT WINAPI CStdStubBuffer_QueryInterface(LPRPCSTUBBUFFER iface,
                                            REFIID riid,
                                            LPVOID *obj)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->QueryInterface(%s,%p)\n",This,debugstr_guid(riid),obj);

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IRpcStubBuffer, riid))
  {
    IUnknown_AddRef(iface);
    *obj = iface;
    return S_OK;
  }
  *obj = NULL;
  return E_NOINTERFACE;
}

ULONG WINAPI CStdStubBuffer_AddRef(LPRPCSTUBBUFFER iface)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  TRACE("(%p)->AddRef()\n",This);
  return InterlockedIncrement(&This->RefCount);
}

ULONG WINAPI NdrCStdStubBuffer_Release(LPRPCSTUBBUFFER iface,
                                      LPPSFACTORYBUFFER pPSF)
{
  CStdStubBuffer *This = (CStdStubBuffer *)iface;
  ULONG refs;

  TRACE("(%p)->Release()\n",This);

  refs = InterlockedDecrement(&This->RefCount);
  if (!refs)
  {
    /* test_Release shows that native doesn't call Disconnect here.
       We'll leave it in for the time being. */
    IRpcStubBuffer_Disconnect(iface);

    IPSFactoryBuffer_Release(pPSF);
    HeapFree(GetProcessHeap(),0,This);
  }
  return refs;
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
void __RPC_STUB NdrStubForwardingFunction( IRpcStubBuffer *iface, IRpcChannelBuffer *pChannel,
                                           PRPC_MESSAGE pMsg, DWORD *pdwStubPhase )
{
    /* Once stub delegation is implemented, this should call
       IRpcStubBuffer_Invoke on the stub's base interface.  The
       IRpcStubBuffer for this interface is stored at (void**)iface-1.
       The pChannel and pMsg parameters are passed intact
       (RPCOLEMESSAGE is basically a RPC_MESSAGE).  If Invoke returns
       with a failure then an exception is raised (to see this, change
       the return value in the test).

    cstdstubbuffer_delegating_t *This = impl_from_delegating(iface);
    HRESULT r = IRpcStubBuffer_Invoke(This->base_stub, (RPCOLEMESSAGE*)pMsg, pChannel);
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
