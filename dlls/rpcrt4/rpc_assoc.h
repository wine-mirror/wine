/*
 * Associations
 *
 * Copyright 2007 Robert Shearman (for CodeWeavers)
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

#include "wine/list.h"

typedef struct _RpcAssoc
{
    struct list entry; /* entry in the global list of associations */
    LONG refs;

    LPSTR Protseq;
    LPSTR NetworkAddr;
    LPSTR Endpoint;
    LPWSTR NetworkOptions;

    /* id of this association group */
    ULONG assoc_group_id;

    CRITICAL_SECTION cs;
    /* connections available to be used */
    struct list free_connection_pool;
} RpcAssoc;

RPC_STATUS RPCRT4_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, RpcAssoc **assoc);
RPC_STATUS RpcAssoc_GetClientConnection(RpcAssoc *assoc, const RPC_SYNTAX_IDENTIFIER *InterfaceId, const RPC_SYNTAX_IDENTIFIER *TransferSyntax, RpcAuthInfo *AuthInfo, RpcQualityOfService *QOS, RpcConnection **Connection);
void RpcAssoc_ReleaseIdleConnection(RpcAssoc *assoc, RpcConnection *Connection);
ULONG RpcAssoc_Release(RpcAssoc *assoc);
RPC_STATUS RpcServerAssoc_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr, LPCSTR Endpoint, LPCWSTR NetworkOptions, unsigned long assoc_gid, RpcAssoc **assoc_out);
