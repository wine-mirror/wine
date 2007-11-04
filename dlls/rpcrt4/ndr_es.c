/*
 * NDR Serialization Services
 *
 * Copyright (c) 2007 Robert Shearman for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "rpc.h"
#include "midles.h"

#include "ndr_misc.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static inline void init_MIDL_ES_MESSAGE(MIDL_ES_MESSAGE *pEsMsg)
{
    memset(pEsMsg, 0, sizeof(*pEsMsg));
    /* even if we are unmarshalling, as we don't want pointers to be pointed
     * to buffer memory */
    pEsMsg->StubMsg.IsClient = TRUE;
}

/***********************************************************************
 *            MesEncodeIncrementalHandleCreate [RPCRT4.@]
 */
RPC_STATUS WINAPI MesEncodeIncrementalHandleCreate(
    void *UserState, MIDL_ES_ALLOC AllocFn, MIDL_ES_WRITE WriteFn,
    handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %p, %p, %p)\n", UserState, AllocFn, WriteFn, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_ENCODE;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Alloc = AllocFn;
    pEsMsg->Write = WriteFn;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesDecodeIncrementalHandleCreate [RPCRT4.@]
 */
RPC_STATUS WINAPI MesDecodeIncrementalHandleCreate(
    void *UserState, MIDL_ES_READ ReadFn, handle_t *pHandle)
{
    MIDL_ES_MESSAGE *pEsMsg;

    TRACE("(%p, %p, %p)\n", UserState, ReadFn, pHandle);

    pEsMsg = HeapAlloc(GetProcessHeap(), 0, sizeof(*pEsMsg));
    if (!pEsMsg)
        return ERROR_OUTOFMEMORY;

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = MES_DECODE;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Read = ReadFn;

    *pHandle = (handle_t)pEsMsg;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesIncrementalHandleReset [RPCRT4.@]
 */
RPC_STATUS WINAPI MesIncrementalHandleReset(
    handle_t Handle, void *UserState, MIDL_ES_ALLOC AllocFn,
    MIDL_ES_WRITE WriteFn, MIDL_ES_READ ReadFn, MIDL_ES_CODE Operation)
{
    MIDL_ES_MESSAGE *pEsMsg = (MIDL_ES_MESSAGE *)Handle;

    TRACE("(%p, %p, %p, %p, %p, %d)\n", Handle, UserState, AllocFn,
        WriteFn, ReadFn, Operation);

    init_MIDL_ES_MESSAGE(pEsMsg);

    pEsMsg->Operation = Operation;
    pEsMsg->UserState = UserState;
    pEsMsg->HandleStyle = MES_INCREMENTAL_HANDLE;
    pEsMsg->Alloc = AllocFn;
    pEsMsg->Write = WriteFn;
    pEsMsg->Read = ReadFn;

    return RPC_S_OK;
}

/***********************************************************************
 *            MesHandleFree [RPCRT4.@]
 */
RPC_STATUS WINAPI MesHandleFree(handle_t Handle)
{
    TRACE("(%p)\n", Handle);
    HeapFree(GetProcessHeap(), 0, Handle);
    return RPC_S_OK;
}
