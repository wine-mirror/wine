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
#include "winerror.h"
#include "winnls.h"
#include "windns.h"
#include "nb30.h"
#include "ws2def.h"

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
        record = heap_alloc_zero( sizeof(DNS_RECORDA) );
        if (!record)
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        else
        {
            record->pName = strdup_u( name );
            if (!record->pName)
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
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
    if (!req)
        return "(null)";

    return wine_dbg_sprintf("{%d %s %s %x%08x %p %d %p %p}", req->Version,
            debugstr_w(req->QueryName), type_to_str(req->QueryType),
            (UINT32)(req->QueryOptions>>32u), (UINT32)req->QueryOptions, req->pDnsServerList,
            req->InterfaceIndex, req->pQueryCompletionCallback, req->pQueryContext);
}

/******************************************************************************
 * DnsQueryEx           [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQueryEx(DNS_QUERY_REQUEST *request, DNS_QUERY_RESULT *result, DNS_QUERY_CANCEL *cancel)
{
    FIXME("(%s %p %p)\n", debugstr_query_request(request), result, cancel);
    return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
}

/******************************************************************************
 * DnsQuery_A           [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQuery_A( PCSTR name, WORD type, DWORD options, PVOID servers,
                              PDNS_RECORDA *result, PVOID *reserved )
{
    WCHAR *nameW;
    DNS_RECORDW *resultW;
    DNS_STATUS status;

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_a(name), type_to_str( type ),
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

    heap_free( nameW );
    return status;
}

/******************************************************************************
 * DnsQuery_UTF8              [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQuery_UTF8( PCSTR name, WORD type, DWORD options, PVOID servers,
                                 PDNS_RECORDA *result, PVOID *reserved )
{
    DNS_STATUS ret = DNS_ERROR_RCODE_NOT_IMPLEMENTED;

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_a(name), type_to_str( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    if ((ret = resolv_funcs->set_serverlist( servers ))) return ret;

    ret = resolv_funcs->query( name, type, options, result );

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
DNS_STATUS WINAPI DnsQuery_W( PCWSTR name, WORD type, DWORD options, PVOID servers,
                              PDNS_RECORDW *result, PVOID *reserved )
{
    char *nameU;
    DNS_RECORDA *resultA;
    DNS_STATUS status;

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_w(name), type_to_str( type ),
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

    heap_free( nameU );
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
    DNS_ADDR_ARRAY *servers = (DNS_ADDR_ARRAY *)buf;
    DWORD ret, needed, i, num, array_len = sizeof(buf);

    for (;;)
    {
        ret = resolv_funcs->get_serverlist( AF_INET, servers, &array_len );
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

        if ((char *)servers != buf) heap_free( servers );
        servers = heap_alloc( array_len );
        if (!servers)
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto err;
        }
    }

    out->AddrCount = num;
    for (i = 0; i < num; i++)
        out->AddrArray[i] = ((SOCKADDR_IN *)servers->AddrArray[i].MaxSa)->sin_addr.s_addr;
    *len = needed;
    ret = ERROR_SUCCESS;

err:
    if ((char *)servers != buf) heap_free( servers );
    return ret;
}

/******************************************************************************
 * DnsQueryConfig          [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsQueryConfig( DNS_CONFIG_TYPE config, DWORD flag, PCWSTR adapter,
                                  PVOID reserved, PVOID buffer, PDWORD len )
{
    DNS_STATUS ret = ERROR_INVALID_PARAMETER;

    TRACE( "(%d,0x%08x,%s,%p,%p,%p)\n", config, flag, debugstr_w(adapter),
           reserved, buffer, len );

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
        return resolv_funcs->get_serverlist( AF_UNSPEC, buffer, len );
    case DnsConfigDnsServersIpv4:
        return resolv_funcs->get_serverlist( AF_INET, buffer, len );
    case DnsConfigDnsServersIpv6:
        return resolv_funcs->get_serverlist( AF_INET6, buffer, len );

    case DnsConfigSearchList:
        return resolv_funcs->get_searchlist( buffer, len );

    default:
        WARN( "unknown config type: %d\n", config );
        break;
    }
    return ret;
}
