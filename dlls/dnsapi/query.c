/*
 * DNS support
 *
 * Copyright (C) 2006 Hans Leidekker
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
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winerror.h"
#include "winnls.h"
#include "windns.h"
#include "nb30.h"
#include "ws2def.h"
#include "in6addr.h"
#include "inaddr.h"
#include "ip2string.h"

#include "wine/debug.h"
#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

#define DEFAULT_TTL  1200

static DNS_STATUS do_query_netbios( PCSTR name, DNS_RECORDA **recp )
{
    NCB ncb;
    UCHAR ret;
    DNS_RRSET rrset;
    FIND_NAME_BUFFER *buffer;
    FIND_NAME_HEADER *header;
    DNS_RECORDA *record = NULL;
    unsigned int i, len;
    DNS_STATUS status = ERROR_INVALID_NAME;

    len = strlen( name );
    if (len >= NCBNAMSZ) return DNS_ERROR_RCODE_NAME_ERROR;

    DNS_RRSET_INIT( rrset );

    memset( &ncb, 0, sizeof(ncb) );
    ncb.ncb_command = NCBFINDNAME;

    memset( ncb.ncb_callname, ' ', sizeof(ncb.ncb_callname) );
    memcpy( ncb.ncb_callname, name, len );
    ncb.ncb_callname[NCBNAMSZ - 1] = '\0';

    ret = Netbios( &ncb );
    if (ret != NRC_GOODRET) return ERROR_INVALID_NAME;

    header = (FIND_NAME_HEADER *)ncb.ncb_buffer;
    buffer = (FIND_NAME_BUFFER *)((char *)header + sizeof(FIND_NAME_HEADER));

    for (i = 0; i < header->node_count; i++)
    {
        record = calloc( 1, sizeof(DNS_RECORDA) );
        if (!record)
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        else
        {
            record->pName = strdup( name );
            if (!record->pName)
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
                free( record );
                goto exit;
            }

            record->wType = DNS_TYPE_A;
            record->Flags.S.Section = DnsSectionAnswer;
            record->Flags.S.CharSet = DnsCharSetUtf8;
            record->dwTtl = DEFAULT_TTL;

            /* FIXME: network byte order? */
            record->Data.A.IpAddress = *(DWORD *)((char *)buffer[i].destination_addr + 2);

            DNS_RRSET_ADD( rrset, (DNS_RECORD *)record );
        }
    }
    status = ERROR_SUCCESS;

exit:
    DNS_RRSET_TERMINATE( rrset );

    if (status != ERROR_SUCCESS)
        DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );
    else
        *recp = (DNS_RECORDA *)rrset.pFirstRR;

    return status;
}

static const char *debugstr_query_request(const DNS_QUERY_REQUEST *req)
{
    if (!req) return "(null)";
    return wine_dbg_sprintf("{%lu %s %s %I64x %p %lu %p %p}", req->Version,
            debugstr_w(req->QueryName), debugstr_type(req->QueryType),
            req->QueryOptions, req->pDnsServerList, req->InterfaceIndex,
            req->pQueryCompletionCallback, req->pQueryContext);
}

/******************************************************************************
 * DnsQueryEx           [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQueryEx(DNS_QUERY_REQUEST *request, DNS_QUERY_RESULT *result, DNS_QUERY_CANCEL *cancel)
{
    FIXME("(%s, %p, %p)\n", debugstr_query_request(request), result, cancel);
    return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
}

/******************************************************************************
 * DnsQuery_A           [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQuery_A( const char *name, WORD type, DWORD options, void *servers, DNS_RECORDA **result,
                              void **reserved )
{
    WCHAR *nameW;
    DNS_RECORDW *resultW;
    DNS_STATUS status;

    TRACE( "(%s, %s, %#lx, %p, %p, %p)\n", debugstr_a(name), debugstr_type( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    nameW = strdup_aw( name );
    if (!nameW) return ERROR_NOT_ENOUGH_MEMORY;

    status = DnsQuery_W( nameW, type, options, servers, &resultW, reserved );

    if (status == ERROR_SUCCESS)
    {
        *result = (DNS_RECORDA *)DnsRecordSetCopyEx(
             (DNS_RECORD *)resultW, DnsCharSetUnicode, DnsCharSetAnsi );

        if (!*result) status = ERROR_NOT_ENOUGH_MEMORY;
        DnsRecordListFree( (DNS_RECORD *)resultW, DnsFreeRecordList );
    }

    free( nameW );
    return status;
}

/******************************************************************************
 * DnsQuery_UTF8              [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQuery_UTF8( const char *name, WORD type, DWORD options, void *servers, DNS_RECORDA **result,
                                 void **reserved )
{
    DNS_STATUS ret = DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    unsigned char answer[4096];
    DWORD len = sizeof(answer);
    struct query_params query_params = { name, type, options, answer, &len };
    const char *end;

    TRACE( "(%s, %s, %#lx, %p, %p, %p)\n", debugstr_a(name), debugstr_type( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    if (type == DNS_TYPE_A)
    {
        struct in_addr addr;

        if (!RtlIpv4StringToAddressA(name, TRUE, &end, &addr) && !*end)
        {
            DNS_RECORDA *r = calloc(1, sizeof(*r));
            r->pName = strdup(name);
            r->wType = DNS_TYPE_A;
            r->wDataLength = sizeof(r->Data.A);
            r->dwTtl = 604800;
            r->Flags.S.Reserved = 0x20;
            r->Flags.S.CharSet = DnsCharSetUtf8;
            r->Data.A.IpAddress = addr.s_addr;
            *result = r;
            return ERROR_SUCCESS;
        }
    }
    else if (type == DNS_TYPE_AAAA)
    {
        struct in6_addr addr;

        if (!RtlIpv6StringToAddressA(name, &end, &addr) && !*end)
        {
            DNS_RECORDA *r = calloc(1, sizeof(*r));
            r->pName = strdup(name);
            r->wType = DNS_TYPE_AAAA;
            r->wDataLength = sizeof(r->Data.AAAA);
            r->dwTtl = 604800;
            r->Flags.S.Reserved = 0x20;
            r->Flags.S.CharSet = DnsCharSetUtf8;
            memcpy(&r->Data.AAAA.Ip6Address, &addr, sizeof(r->Data.AAAA.Ip6Address));
            *result = r;
            return ERROR_SUCCESS;
        }
    }

    if ((ret = RESOLV_CALL( set_serverlist, servers ))) return ret;

    ret = RESOLV_CALL( query, &query_params );
    if (!ret)
    {
        DNS_MESSAGE_BUFFER *buffer = (DNS_MESSAGE_BUFFER *)answer;

        if (len < sizeof(buffer->MessageHead)) return DNS_ERROR_BAD_PACKET;
        DNS_BYTE_FLIP_HEADER_COUNTS( &buffer->MessageHead );
        switch (buffer->MessageHead.ResponseCode)
        {
        case DNS_RCODE_NOERROR:  ret = DnsExtractRecordsFromMessage_UTF8( buffer, len, result ); break;
        case DNS_RCODE_FORMERR:  ret = DNS_ERROR_RCODE_FORMAT_ERROR; break;
        case DNS_RCODE_SERVFAIL: ret = DNS_ERROR_RCODE_SERVER_FAILURE; break;
        case DNS_RCODE_NXDOMAIN: ret = DNS_ERROR_RCODE_NAME_ERROR; break;
        case DNS_RCODE_NOTIMPL:  ret = DNS_ERROR_RCODE_NOT_IMPLEMENTED; break;
        case DNS_RCODE_REFUSED:  ret = DNS_ERROR_RCODE_REFUSED; break;
        case DNS_RCODE_YXDOMAIN: ret = DNS_ERROR_RCODE_YXDOMAIN; break;
        case DNS_RCODE_YXRRSET:  ret = DNS_ERROR_RCODE_YXRRSET; break;
        case DNS_RCODE_NXRRSET:  ret = DNS_ERROR_RCODE_NXRRSET; break;
        case DNS_RCODE_NOTAUTH:  ret = DNS_ERROR_RCODE_NOTAUTH; break;
        case DNS_RCODE_NOTZONE:  ret = DNS_ERROR_RCODE_NOTZONE; break;
        default:                 ret = DNS_ERROR_RCODE_NOT_IMPLEMENTED; break;
        }
    }

    if (ret == DNS_ERROR_RCODE_NAME_ERROR && type == DNS_TYPE_A &&
        !(options & DNS_QUERY_NO_NETBT))
    {
        TRACE( "dns lookup failed, trying netbios query\n" );
        ret = do_query_netbios( name, result );
    }

    return ret;
}

/******************************************************************************
 * DnsQuery_W              [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQuery_W( const WCHAR *name, WORD type, DWORD options, void *servers, DNS_RECORDW **result,
                              void **reserved )
{
    char *nameU;
    DNS_RECORDA *resultA;
    DNS_STATUS status;

    TRACE( "(%s, %s, %#lx, %p, %p, %p)\n", debugstr_w(name), debugstr_type( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    nameU = strdup_wu( name );
    if (!nameU) return ERROR_NOT_ENOUGH_MEMORY;

    status = DnsQuery_UTF8( nameU, type, options, servers, &resultA, reserved );

    if (status == ERROR_SUCCESS)
    {
        *result = (DNS_RECORDW *)DnsRecordSetCopyEx(
            (DNS_RECORD *)resultA, DnsCharSetUtf8, DnsCharSetUnicode );

        if (!*result) status = ERROR_NOT_ENOUGH_MEMORY;
        DnsRecordListFree( (DNS_RECORD *)resultA, DnsFreeRecordList );
    }

    free( nameU );
    return status;
}

static DNS_STATUS get_hostname_a( COMPUTER_NAME_FORMAT format, PSTR buffer, PDWORD len )
{
    char name[256];
    DWORD size = ARRAY_SIZE(name);

    if (!GetComputerNameExA( format, name, &size ))
        return DNS_ERROR_NAME_DOES_NOT_EXIST;

    if (!buffer || (size = lstrlenA( name ) + 1) > *len)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    lstrcpyA( buffer, name );
    return ERROR_SUCCESS;
}

static DNS_STATUS get_hostname_w( COMPUTER_NAME_FORMAT format, PWSTR buffer, PDWORD len )
{
    WCHAR name[256];
    DWORD size = ARRAY_SIZE(name);

    if (!GetComputerNameExW( format, name, &size ))
        return DNS_ERROR_NAME_DOES_NOT_EXIST;

    if (!buffer || (size = lstrlenW( name ) + 1) > *len)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    lstrcpyW( buffer, name );
    return ERROR_SUCCESS;
}

static DNS_STATUS get_dns_server_list( IP4_ARRAY *out, DWORD *len )
{
    char buf[FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[3])];
    DWORD ret, needed, i, num, array_len = sizeof(buf);
    struct get_serverlist_params params = { AF_INET, (DNS_ADDR_ARRAY *)buf, &array_len };

    for (;;)
    {
        ret = RESOLV_CALL( get_serverlist, &params );
        if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA) goto err;
        num = (array_len - FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[0])) / sizeof(DNS_ADDR);
        needed = FIELD_OFFSET(IP4_ARRAY, AddrArray[num]);
        if (!out || *len < needed)
        {
            *len = needed;
            ret = !out ? ERROR_SUCCESS : ERROR_MORE_DATA;
            goto err;
        }
        if (!ret) break;

        if ((char *)params.addrs != buf) free( params.addrs );
        params.addrs = malloc( array_len );
        if (!params.addrs)
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto err;
        }
    }

    out->AddrCount = num;
    for (i = 0; i < num; i++)
        out->AddrArray[i] = ((SOCKADDR_IN *)params.addrs->AddrArray[i].MaxSa)->sin_addr.s_addr;
    *len = needed;
    ret = ERROR_SUCCESS;

err:
    if ((char *)params.addrs != buf) free( params.addrs );
    return ret;
}

/******************************************************************************
 * DnsQueryConfig          [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQueryConfig( DNS_CONFIG_TYPE config, DWORD flag, const WCHAR *adapter, void *reserved,
                                  void *buffer, DWORD *len )
{
    DNS_STATUS ret = ERROR_INVALID_PARAMETER;

    TRACE( "(%d, %#lx, %s, %p, %p, %p)\n", config, flag, debugstr_w(adapter), reserved, buffer, len );

    if (!len) return ERROR_INVALID_PARAMETER;

    switch (config)
    {
    case DnsConfigDnsServerList:
        return get_dns_server_list( buffer, len );

    case DnsConfigHostName_A:
    case DnsConfigHostName_UTF8:
        return get_hostname_a( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_A:
    case DnsConfigFullHostName_UTF8:
        return get_hostname_a( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_A:
    case DnsConfigPrimaryDomainName_UTF8:
        return get_hostname_a( ComputerNameDnsDomain, buffer, len );

    case DnsConfigHostName_W:
        return get_hostname_w( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_W:
        return get_hostname_w( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_W:
        return get_hostname_w( ComputerNameDnsDomain, buffer, len );

    case DnsConfigAdapterDomainName_A:
    case DnsConfigAdapterDomainName_W:
    case DnsConfigAdapterDomainName_UTF8:
    case DnsConfigAdapterInfo:
    case DnsConfigPrimaryHostNameRegistrationEnabled:
    case DnsConfigAdapterHostNameRegistrationEnabled:
    case DnsConfigAddressRegistrationMaxCount:
        FIXME( "unimplemented config type %d\n", config );
        break;

    case DnsConfigDnsServersUnspec:
    {
        struct get_serverlist_params params = { AF_UNSPEC, buffer, len };
        return RESOLV_CALL( get_serverlist, &params );
    }
    case DnsConfigDnsServersIpv4:
    {
        struct get_serverlist_params params = { AF_INET, buffer, len };
        return RESOLV_CALL( get_serverlist, &params );
    }
    case DnsConfigDnsServersIpv6:
    {
        struct get_serverlist_params params = { AF_INET6, buffer, len };
        return RESOLV_CALL( get_serverlist, &params );
    }
    /* Windows does not implement this, but we need it in iphlpapi. */
    case DnsConfigSearchList:
    {
        struct get_searchlist_params params = { buffer, len };
        return RESOLV_CALL( get_searchlist, &params );
    }
    default:
        WARN( "unknown config type: %d\n", config );
        break;
    }
    return ret;
}
