/*
 * MIDL proxy/stub stuff
 *
 * Copyright 2002 Ove Kåven, TransGaming Technologies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * TODO:
 *  - figure out whether we *really* got this right
 *  - check for errors and throw exceptions
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "wine/obj_base.h"
#include "wine/obj_channel.h"

#include "rpcproxy.h"

#include "wine/debug.h"

#include "cpsf.h"
#include "ndr_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *           NdrProxyInitialize [RPCRT4.@]
 */
void WINAPI NdrProxyInitialize(void *This,
                              PRPC_MESSAGE pRpcMsg,
                              PMIDL_STUB_MESSAGE pStubMsg,
                              PMIDL_STUB_DESC pStubDescriptor,
                              unsigned int ProcNum)
{
  HRESULT hr;

  TRACE("(%p,%p,%p,%p,%d)\n", This, pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);
  memset(pRpcMsg, 0, sizeof(RPC_MESSAGE));
  memset(pStubMsg, 0, sizeof(MIDL_STUB_MESSAGE));
  pRpcMsg->ProcNum = ProcNum;
  pRpcMsg->RpcInterfaceInformation = pStubDescriptor->RpcInterfaceInformation;
  pStubMsg->RpcMsg = pRpcMsg;
  pStubMsg->IsClient = 1;
  pStubMsg->ReuseBuffer = 1;
  pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
  pStubMsg->pfnFree = pStubDescriptor->pfnFree;
  pStubMsg->StubDesc = pStubDescriptor;
  if (This) StdProxy_GetChannel(This, &pStubMsg->pRpcChannelBuffer);
  if (pStubMsg->pRpcChannelBuffer) {
    hr = IRpcChannelBuffer_GetDestCtx(pStubMsg->pRpcChannelBuffer,
                                     &pStubMsg->dwDestContext,
                                     &pStubMsg->pvDestContext);
  }
  TRACE("channel=%p\n", pStubMsg->pRpcChannelBuffer);
}

/***********************************************************************
 *           NdrProxyGetBuffer [RPCRT4.@]
 */
void WINAPI NdrProxyGetBuffer(void *This,
                             PMIDL_STUB_MESSAGE pStubMsg)
{
  HRESULT hr;
  const IID *riid = NULL;

  TRACE("(%p,%p)\n", This, pStubMsg);
  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  pStubMsg->dwStubPhase = PROXY_GETBUFFER;
  hr = StdProxy_GetIID(This, &riid);
  hr = IRpcChannelBuffer_GetBuffer(pStubMsg->pRpcChannelBuffer,
                                  (RPCOLEMESSAGE*)pStubMsg->RpcMsg,
                                  riid);
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
  pStubMsg->dwStubPhase = PROXY_MARSHAL;
}

/***********************************************************************
 *           NdrProxySendReceive [RPCRT4.@]
 */
void WINAPI NdrProxySendReceive(void *This,
                               PMIDL_STUB_MESSAGE pStubMsg)
{
  ULONG Status = 0;
  HRESULT hr;

  TRACE("(%p,%p)\n", This, pStubMsg);
  pStubMsg->dwStubPhase = PROXY_SENDRECEIVE;
  hr = IRpcChannelBuffer_SendReceive(pStubMsg->pRpcChannelBuffer,
                                    (RPCOLEMESSAGE*)pStubMsg->RpcMsg,
                                    &Status);
  pStubMsg->dwStubPhase = PROXY_UNMARSHAL;
  pStubMsg->BufferLength = pStubMsg->RpcMsg->BufferLength;
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
}

/***********************************************************************
 *           NdrProxyFreeBuffer [RPCRT4.@]
 */
void WINAPI NdrProxyFreeBuffer(void *This,
                              PMIDL_STUB_MESSAGE pStubMsg)
{
  HRESULT hr;

  TRACE("(%p,%p)\n", This, pStubMsg);
  hr = IRpcChannelBuffer_FreeBuffer(pStubMsg->pRpcChannelBuffer,
                                   (RPCOLEMESSAGE*)pStubMsg->RpcMsg);
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
  memset(pStubMsg, 0, sizeof(MIDL_STUB_MESSAGE));
  pStubMsg->RpcMsg = pRpcMsg;
  pStubMsg->IsClient = 0;
  pStubMsg->ReuseBuffer = 1;
  pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
  pStubMsg->pfnFree = pStubDescriptor->pfnFree;
  pStubMsg->StubDesc = pStubDescriptor;
  pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;
  pStubMsg->BufferLength = pStubMsg->RpcMsg->BufferLength;
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
}

/***********************************************************************
 *           NdrStubGetBuffer [RPCRT4.@]
 */
void WINAPI NdrStubGetBuffer(LPRPCSTUBBUFFER This,
                            LPRPCCHANNELBUFFER pRpcChannelBuffer,
                            PMIDL_STUB_MESSAGE pStubMsg)
{
  TRACE("(%p,%p)\n", This, pStubMsg);
  pStubMsg->pRpcChannelBuffer = pRpcChannelBuffer;
  pStubMsg->RpcMsg->BufferLength = pStubMsg->BufferLength;
  I_RpcGetBuffer(pStubMsg->RpcMsg); /* ? */
  pStubMsg->BufferStart = pStubMsg->RpcMsg->Buffer;
  pStubMsg->BufferEnd = pStubMsg->BufferStart + pStubMsg->BufferLength;
  pStubMsg->Buffer = pStubMsg->BufferStart;
}

/************************************************************************
 *             NdrClientInitializeNew [RPCRT4.@]
 */
void WINAPI NdrClientInitializeNew( PRPC_MESSAGE pRpcMessage, PMIDL_STUB_MESSAGE pStubMsg, 
                                    PMIDL_STUB_DESC pStubDesc, unsigned int ProcNum )
{
  TRACE("(pRpcMessage == ^%p, pStubMsg == ^%p, pStubDesc == ^%p, ProcNum == %d)\n",
    pRpcMessage, pStubMsg, pStubDesc, ProcNum);

  memset(pRpcMessage, 0, sizeof(RPC_MESSAGE));
  memset(pStubMsg, 0, sizeof(MIDL_STUB_MESSAGE));

  pStubMsg->ReuseBuffer = TRUE;
  pStubMsg->IsClient = TRUE;
  pStubMsg->StubDesc = pStubDesc;
  pStubMsg->pfnAllocate = pStubDesc->pfnAllocate;
  pStubMsg->pfnFree = pStubDesc->pfnFree;
  pStubMsg->RpcMsg = pRpcMessage;

  pRpcMessage->ProcNum = ProcNum;
  pRpcMessage->RpcInterfaceInformation = pStubDesc->RpcInterfaceInformation;
}

/***********************************************************************
 *           NdrGetBuffer [RPCRT4.@]
 */
unsigned char *WINAPI NdrGetBuffer(MIDL_STUB_MESSAGE *stubmsg, unsigned long buflen, RPC_BINDING_HANDLE handle)
{
  TRACE("(stubmsg == ^%p, buflen == %lu, handle == %p): wild guess.\n", stubmsg, buflen, handle);
  
  /* FIXME: What are we supposed to do with the handle? */
  
  stubmsg->RpcMsg->BufferLength = buflen;
  if (I_RpcGetBuffer(stubmsg->RpcMsg) != S_OK)
    return NULL;

  stubmsg->BufferLength = stubmsg->RpcMsg->BufferLength;
  stubmsg->BufferEnd = stubmsg->BufferStart = 0;
  return (stubmsg->Buffer = (unsigned char *)stubmsg->RpcMsg->Buffer);
}
/***********************************************************************
 *           NdrFreeBuffer [RPCRT4.@]
 */
void WINAPI NdrFreeBuffer(MIDL_STUB_MESSAGE *pStubMsg)
{
  TRACE("(pStubMsg == ^%p): wild guess.\n", pStubMsg);
  I_RpcFreeBuffer(pStubMsg->RpcMsg);
  pStubMsg->BufferLength = 0;
  pStubMsg->Buffer = (unsigned char *)(pStubMsg->RpcMsg->Buffer = NULL);
}

/************************************************************************
 *           NdrSendReceive [RPCRT4.@]
 */
unsigned char *WINAPI NdrSendReceive( MIDL_STUB_MESSAGE *stubmsg, unsigned char *buffer  )
{
  FIXME("stub\n");
  return NULL;
}
