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

#include <stdarg.h>
#include <assert.h>

#include "rpc.h"
#include "rpcndr.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "rpc_binding.h"
#include "rpc_assoc.h"
#include "rpc_message.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpc);

static CRITICAL_SECTION assoc_list_cs;
static CRITICAL_SECTION_DEBUG assoc_list_cs_debug =
{
    0, 0, &assoc_list_cs,
    { &assoc_list_cs_debug.ProcessLocksList, &assoc_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": assoc_list_cs") }
};
static CRITICAL_SECTION assoc_list_cs = { &assoc_list_cs_debug, -1, 0, 0, 0, 0 };

static struct list client_assoc_list = LIST_INIT(client_assoc_list);
static struct list server_assoc_list = LIST_INIT(server_assoc_list);

static LONG last_assoc_group_id;

static RPC_STATUS RpcAssoc_Alloc(LPCSTR Protseq, LPCSTR NetworkAddr,
                                 LPCSTR Endpoint, LPCWSTR NetworkOptions,
                                 RpcAssoc **assoc_out)
{
    RpcAssoc *assoc;
    assoc = HeapAlloc(GetProcessHeap(), 0, sizeof(*assoc));
    if (!assoc)
        return RPC_S_OUT_OF_RESOURCES;
    assoc->refs = 1;
    list_init(&assoc->free_connection_pool);
    InitializeCriticalSection(&assoc->cs);
    assoc->Protseq = RPCRT4_strdupA(Protseq);
    assoc->NetworkAddr = RPCRT4_strdupA(NetworkAddr);
    assoc->Endpoint = RPCRT4_strdupA(Endpoint);
    assoc->NetworkOptions = NetworkOptions ? RPCRT4_strdupW(NetworkOptions) : NULL;
    assoc->assoc_group_id = 0;
    list_init(&assoc->entry);
    *assoc_out = assoc;
    return RPC_S_OK;
}

RPC_STATUS RPCRT4_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr,
                                 LPCSTR Endpoint, LPCWSTR NetworkOptions,
                                 RpcAssoc **assoc_out)
{
    RpcAssoc *assoc;
    RPC_STATUS status;

    EnterCriticalSection(&assoc_list_cs);
    LIST_FOR_EACH_ENTRY(assoc, &client_assoc_list, RpcAssoc, entry)
    {
        if (!strcmp(Protseq, assoc->Protseq) &&
            !strcmp(NetworkAddr, assoc->NetworkAddr) &&
            !strcmp(Endpoint, assoc->Endpoint) &&
            ((!assoc->NetworkOptions && !NetworkOptions) || !strcmpW(NetworkOptions, assoc->NetworkOptions)))
        {
            assoc->refs++;
            *assoc_out = assoc;
            LeaveCriticalSection(&assoc_list_cs);
            TRACE("using existing assoc %p\n", assoc);
            return RPC_S_OK;
        }
    }

    status = RpcAssoc_Alloc(Protseq, NetworkAddr, Endpoint, NetworkOptions, &assoc);
    if (status != RPC_S_OK)
    {
        LeaveCriticalSection(&assoc_list_cs);
        return status;
    }
    list_add_head(&client_assoc_list, &assoc->entry);
    *assoc_out = assoc;

    LeaveCriticalSection(&assoc_list_cs);

    TRACE("new assoc %p\n", assoc);

    return RPC_S_OK;
}

RPC_STATUS RpcServerAssoc_GetAssociation(LPCSTR Protseq, LPCSTR NetworkAddr,
                                         LPCSTR Endpoint, LPCWSTR NetworkOptions,
                                         unsigned long assoc_gid,
                                         RpcAssoc **assoc_out)
{
    RpcAssoc *assoc;
    RPC_STATUS status;

    EnterCriticalSection(&assoc_list_cs);
    if (assoc_gid)
    {
        LIST_FOR_EACH_ENTRY(assoc, &server_assoc_list, RpcAssoc, entry)
        {
            /* FIXME: NetworkAddr shouldn't be NULL */
            if (assoc->assoc_group_id == assoc_gid &&
                !strcmp(Protseq, assoc->Protseq) &&
                (!NetworkAddr || !assoc->NetworkAddr || !strcmp(NetworkAddr, assoc->NetworkAddr)) &&
                !strcmp(Endpoint, assoc->Endpoint) &&
                ((!assoc->NetworkOptions == !NetworkOptions) &&
                 (!NetworkOptions || !strcmpW(NetworkOptions, assoc->NetworkOptions))))
            {
                assoc->refs++;
                *assoc_out = assoc;
                LeaveCriticalSection(&assoc_list_cs);
                TRACE("using existing assoc %p\n", assoc);
                return RPC_S_OK;
            }
        }
        *assoc_out = NULL;
        return RPC_S_NO_CONTEXT_AVAILABLE;
    }

    status = RpcAssoc_Alloc(Protseq, NetworkAddr, Endpoint, NetworkOptions, &assoc);
    if (status != RPC_S_OK)
    {
        LeaveCriticalSection(&assoc_list_cs);
        return status;
    }
    assoc->assoc_group_id = InterlockedIncrement(&last_assoc_group_id);
    list_add_head(&server_assoc_list, &assoc->entry);
    *assoc_out = assoc;

    LeaveCriticalSection(&assoc_list_cs);

    TRACE("new assoc %p\n", assoc);

    return RPC_S_OK;
}

ULONG RpcAssoc_Release(RpcAssoc *assoc)
{
    ULONG refs;

    EnterCriticalSection(&assoc_list_cs);
    refs = --assoc->refs;
    if (!refs)
        list_remove(&assoc->entry);
    LeaveCriticalSection(&assoc_list_cs);

    if (!refs)
    {
        RpcConnection *Connection, *cursor2;

        TRACE("destroying assoc %p\n", assoc);

        LIST_FOR_EACH_ENTRY_SAFE(Connection, cursor2, &assoc->free_connection_pool, RpcConnection, conn_pool_entry)
        {
            list_remove(&Connection->conn_pool_entry);
            RPCRT4_DestroyConnection(Connection);
        }

        HeapFree(GetProcessHeap(), 0, assoc->NetworkOptions);
        HeapFree(GetProcessHeap(), 0, assoc->Endpoint);
        HeapFree(GetProcessHeap(), 0, assoc->NetworkAddr);
        HeapFree(GetProcessHeap(), 0, assoc->Protseq);

        DeleteCriticalSection(&assoc->cs);

        HeapFree(GetProcessHeap(), 0, assoc);
    }

    return refs;
}

#define ROUND_UP(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment)-1))

static RPC_STATUS RpcAssoc_BindConnection(const RpcAssoc *assoc, RpcConnection *conn,
                                          const RPC_SYNTAX_IDENTIFIER *InterfaceId,
                                          const RPC_SYNTAX_IDENTIFIER *TransferSyntax)
{
    RpcPktHdr *hdr;
    RpcPktHdr *response_hdr;
    RPC_MESSAGE msg;
    RPC_STATUS status;

    TRACE("sending bind request to server\n");

    hdr = RPCRT4_BuildBindHeader(NDR_LOCAL_DATA_REPRESENTATION,
                                 RPC_MAX_PACKET_SIZE, RPC_MAX_PACKET_SIZE,
                                 assoc->assoc_group_id,
                                 InterfaceId, TransferSyntax);

    status = RPCRT4_Send(conn, hdr, NULL, 0);
    RPCRT4_FreeHeader(hdr);
    if (status != RPC_S_OK)
        return status;

    status = RPCRT4_Receive(conn, &response_hdr, &msg);
    if (status != RPC_S_OK)
    {
        ERR("receive failed\n");
        return status;
    }

    switch (response_hdr->common.ptype)
    {
    case PKT_BIND_ACK:
    {
        RpcAddressString *server_address = msg.Buffer;
        if ((msg.BufferLength >= FIELD_OFFSET(RpcAddressString, string[0])) ||
            (msg.BufferLength >= ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4)))
        {
            unsigned short remaining = msg.BufferLength -
            ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4);
            RpcResults *results = (RpcResults*)((ULONG_PTR)server_address +
                                                ROUND_UP(FIELD_OFFSET(RpcAddressString, string[server_address->length]), 4));
            if ((results->num_results == 1) && (remaining >= sizeof(*results)))
            {
                switch (results->results[0].result)
                {
                case RESULT_ACCEPT:
                    conn->assoc_group_id = response_hdr->bind_ack.assoc_gid;
                    conn->MaxTransmissionSize = response_hdr->bind_ack.max_tsize;
                    conn->ActiveInterface = *InterfaceId;
                    break;
                case RESULT_PROVIDER_REJECTION:
                    switch (results->results[0].reason)
                    {
                    case REASON_ABSTRACT_SYNTAX_NOT_SUPPORTED:
                        ERR("syntax %s, %d.%d not supported\n",
                            debugstr_guid(&InterfaceId->SyntaxGUID),
                            InterfaceId->SyntaxVersion.MajorVersion,
                            InterfaceId->SyntaxVersion.MinorVersion);
                        status = RPC_S_UNKNOWN_IF;
                        break;
                    case REASON_TRANSFER_SYNTAXES_NOT_SUPPORTED:
                        ERR("transfer syntax not supported\n");
                        status = RPC_S_SERVER_UNAVAILABLE;
                        break;
                    case REASON_NONE:
                    default:
                        status = RPC_S_CALL_FAILED_DNE;
                    }
                    break;
                case RESULT_USER_REJECTION:
                default:
                    ERR("rejection result %d\n", results->results[0].result);
                    status = RPC_S_CALL_FAILED_DNE;
                }
            }
            else
            {
                ERR("incorrect results size\n");
                status = RPC_S_CALL_FAILED_DNE;
            }
        }
        else
        {
            ERR("bind ack packet too small (%d)\n", msg.BufferLength);
            status = RPC_S_PROTOCOL_ERROR;
        }
        break;
    }
    case PKT_BIND_NACK:
        switch (response_hdr->bind_nack.reject_reason)
        {
        case REJECT_LOCAL_LIMIT_EXCEEDED:
        case REJECT_TEMPORARY_CONGESTION:
            ERR("server too busy\n");
            status = RPC_S_SERVER_TOO_BUSY;
            break;
        case REJECT_PROTOCOL_VERSION_NOT_SUPPORTED:
            ERR("protocol version not supported\n");
            status = RPC_S_PROTOCOL_ERROR;
            break;
        case REJECT_UNKNOWN_AUTHN_SERVICE:
            ERR("unknown authentication service\n");
            status = RPC_S_UNKNOWN_AUTHN_SERVICE;
            break;
        case REJECT_INVALID_CHECKSUM:
            ERR("invalid checksum\n");
            status = ERROR_ACCESS_DENIED;
            break;
        default:
            ERR("rejected bind for reason %d\n", response_hdr->bind_nack.reject_reason);
            status = RPC_S_CALL_FAILED_DNE;
        }
        break;
    default:
        ERR("wrong packet type received %d\n", response_hdr->common.ptype);
        status = RPC_S_PROTOCOL_ERROR;
        break;
    }

    I_RpcFreeBuffer(&msg);
    RPCRT4_FreeHeader(response_hdr);
    return status;
}

static RpcConnection *RpcAssoc_GetIdleConnection(RpcAssoc *assoc,
                                                 const RPC_SYNTAX_IDENTIFIER *InterfaceId,
                                                 const RPC_SYNTAX_IDENTIFIER *TransferSyntax, const RpcAuthInfo *AuthInfo,
                                                 const RpcQualityOfService *QOS)
{
    RpcConnection *Connection;
    EnterCriticalSection(&assoc->cs);
    /* try to find a compatible connection from the connection pool */
    LIST_FOR_EACH_ENTRY(Connection, &assoc->free_connection_pool, RpcConnection, conn_pool_entry)
    {
        if (!memcmp(&Connection->ActiveInterface, InterfaceId,
                    sizeof(RPC_SYNTAX_IDENTIFIER)) &&
            RpcAuthInfo_IsEqual(Connection->AuthInfo, AuthInfo) &&
            RpcQualityOfService_IsEqual(Connection->QOS, QOS))
        {
            list_remove(&Connection->conn_pool_entry);
            LeaveCriticalSection(&assoc->cs);
            TRACE("got connection from pool %p\n", Connection);
            return Connection;
        }
    }

    LeaveCriticalSection(&assoc->cs);
    return NULL;
}

RPC_STATUS RpcAssoc_GetClientConnection(RpcAssoc *assoc,
                                        const RPC_SYNTAX_IDENTIFIER *InterfaceId,
                                        const RPC_SYNTAX_IDENTIFIER *TransferSyntax, RpcAuthInfo *AuthInfo,
                                        RpcQualityOfService *QOS, RpcConnection **Connection)
{
    RpcConnection *NewConnection;
    RPC_STATUS status;

    *Connection = RpcAssoc_GetIdleConnection(assoc, InterfaceId, TransferSyntax, AuthInfo, QOS);
    if (*Connection)
        return RPC_S_OK;

    /* create a new connection */
    status = RPCRT4_CreateConnection(&NewConnection, FALSE /* is this a server connection? */,
        assoc->Protseq, assoc->NetworkAddr,
        assoc->Endpoint, assoc->NetworkOptions,
        AuthInfo, QOS);
    if (status != RPC_S_OK)
        return status;

    status = RPCRT4_OpenClientConnection(NewConnection);
    if (status != RPC_S_OK)
    {
        RPCRT4_DestroyConnection(NewConnection);
        return status;
    }

    status = RpcAssoc_BindConnection(assoc, NewConnection, InterfaceId, TransferSyntax);
    if (status != RPC_S_OK)
    {
        RPCRT4_DestroyConnection(NewConnection);
        return status;
    }

    *Connection = NewConnection;

    return RPC_S_OK;
}

void RpcAssoc_ReleaseIdleConnection(RpcAssoc *assoc, RpcConnection *Connection)
{
    assert(!Connection->server);
    EnterCriticalSection(&assoc->cs);
    if (!assoc->assoc_group_id) assoc->assoc_group_id = Connection->assoc_group_id;
    list_add_head(&assoc->free_connection_pool, &Connection->conn_pool_entry);
    LeaveCriticalSection(&assoc->cs);
}
