/*
 * RPC messages
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
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
 *  - decide if OVERLAPPED_WORKS
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"

#include "rpc.h"
#include "rpcdcep.h"

#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_defs.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/***********************************************************************
 *           I_RpcGetBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcGetBuffer(PRPC_MESSAGE pMsg)
{
  void* buf;

  TRACE("(%p)\n", pMsg);
  /* FIXME: pfnAllocate? */
  buf = HeapReAlloc(GetProcessHeap(), 0, pMsg->Buffer, pMsg->BufferLength);
  if (buf) pMsg->Buffer = buf;
  /* FIXME: which errors to return? */
  return buf ? S_OK : E_OUTOFMEMORY;
}

/***********************************************************************
 *           I_RpcFreeBuffer [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcFreeBuffer(PRPC_MESSAGE pMsg)
{
  TRACE("(%p)\n", pMsg);
  /* FIXME: pfnFree? */
  HeapFree(GetProcessHeap(), 0, pMsg->Buffer);
  pMsg->Buffer = NULL;
  return S_OK;
}

/***********************************************************************
 *           I_RpcSend [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSend(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RPC_CLIENT_INTERFACE* cif = NULL;
  RPC_SERVER_INTERFACE* sif = NULL;
  RPC_STATUS status;
  RpcPktHdr hdr;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  if (bind->server) {
    sif = pMsg->RpcInterfaceInformation;
    if (!sif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
  } else {
    cif = pMsg->RpcInterfaceInformation;
    if (!cif) return RPC_S_INTERFACE_NOT_FOUND; /* ? */
  }

  status = RPCRT4_OpenBinding(bind);
  if (status != RPC_S_OK) return status;

  /* initialize packet header */
  memset(&hdr, 0, sizeof(hdr));
  hdr.rpc_ver = 4;
  hdr.ptype = PKT_REQUEST;
  hdr.object = bind->ObjectUuid; /* FIXME: IIRC iff no object, the header structure excludes this elt */
  hdr.if_id = (bind->server) ? sif->InterfaceId.SyntaxGUID : cif->InterfaceId.SyntaxGUID;
  hdr.if_vers = 
    (bind->server) ?
    MAKELONG(sif->InterfaceId.SyntaxVersion.MinorVersion, sif->InterfaceId.SyntaxVersion.MajorVersion) :
    MAKELONG(cif->InterfaceId.SyntaxVersion.MinorVersion, cif->InterfaceId.SyntaxVersion.MajorVersion);
  hdr.opnum = pMsg->ProcNum;
  /* only the low-order 3 octets of the DataRepresentation go in the header */
  hdr.drep[0] = LOBYTE(LOWORD(pMsg->DataRepresentation));
  hdr.drep[1] = HIBYTE(LOWORD(pMsg->DataRepresentation));
  hdr.drep[2] = LOBYTE(HIWORD(pMsg->DataRepresentation));
  hdr.len = pMsg->BufferLength;

  /* transmit packet */
  if (!WriteFile(bind->conn, &hdr, sizeof(hdr), NULL, NULL))
    return GetLastError();
  if (!WriteFile(bind->conn, pMsg->Buffer, pMsg->BufferLength, NULL, NULL))
    return GetLastError();

  /* success */
  return RPC_S_OK;
}

/***********************************************************************
 *           I_RpcReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcReceive(PRPC_MESSAGE pMsg)
{
  RpcBinding* bind = (RpcBinding*)pMsg->Handle;
  RPC_STATUS status;
  RpcPktHdr hdr;
  DWORD dwRead;

  TRACE("(%p)\n", pMsg);
  if (!bind) return RPC_S_INVALID_BINDING;

  status = RPCRT4_OpenBinding(bind);
  if (status != RPC_S_OK) return status;

  /* read packet header */
#ifdef OVERLAPPED_WORKS
  if (!ReadFile(bind->conn, &hdr, sizeof(hdr), &dwRead, &bind->ovl)) {
    DWORD err = GetLastError();
    if (err != ERROR_IO_PENDING) {
      return err;
    }
    if (!GetOverlappedResult(bind->conn, &bind->ovl, &dwRead, TRUE)) return GetLastError();
  }
#else
  if (!ReadFile(bind->conn, &hdr, sizeof(hdr), &dwRead, NULL))
    return GetLastError();
#endif
  if (dwRead != sizeof(hdr)) return RPC_S_PROTOCOL_ERROR;

  /* read packet body */
  pMsg->BufferLength = hdr.len;
  status = I_RpcGetBuffer(pMsg);
  if (status != RPC_S_OK) return status;
#ifdef OVERLAPPED_WORKS
  if (!ReadFile(bind->conn, pMsg->Buffer, hdr.len, &dwRead, &bind->ovl)) {
    DWORD err = GetLastError();
    if (err != ERROR_IO_PENDING) {
      return err;
    }
    if (!GetOverlappedResult(bind->conn, &bind->ovl, &dwRead, TRUE)) return GetLastError();
  }
#else
  if (!ReadFile(bind->conn, pMsg->Buffer, hdr.len, &dwRead, NULL))
    return GetLastError();
#endif
  if (dwRead != hdr.len) return RPC_S_PROTOCOL_ERROR;

  /* success */
  return RPC_S_OK;
}

/***********************************************************************
 *           I_RpcSendReceive [RPCRT4.@]
 */
RPC_STATUS WINAPI I_RpcSendReceive(PRPC_MESSAGE pMsg)
{
  RPC_STATUS status;

  TRACE("(%p)\n", pMsg);
  status = I_RpcSend(pMsg);
  if (status == RPC_S_OK)
    status = I_RpcReceive(pMsg);
  return status;
}
