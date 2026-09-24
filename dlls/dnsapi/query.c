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

#define DEFAULT_TTL_NETBIOS 1200
#define DEFAULT_TTL_HOSTS   557890   /* resets when hosts file changes */

static CRITICAL_SECTION hosts_cs;
static CRITICAL_SECTION_DEBUG hosts_debug =
{
    0, 0, &hosts_cs, { &hosts_debug.ProcessLocksList, &hosts_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": hosts_cs") }
};
static CRITICAL_SECTION hosts_cs = { &hosts_debug, -1, 0, 0, 0, 0 };

static DNS_RECORDA *alloc_record( const char *name )
{
    DNS_RECORDA *rec;

    if (!(rec = calloc( 1, sizeof(*rec) ))) return NULL;
    if (!(rec->pName = strdup( name )))
    {
        free( rec );
        return NULL;
    }
    return rec;
}

static DNS_STATUS do_query_netbios( const char *name, DNS_RECORDA **result )
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
        if (!(record = alloc_record( name )))
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        record->wType            = DNS_TYPE_A;
        record->Flags.S.Section  = DnsSectionAnswer;
        record->Flags.S.CharSet  = DnsCharSetUtf8;
        record->dwTtl            = DEFAULT_TTL_NETBIOS;
        /* FIXME: network byte order? */
        record->Data.A.IpAddress = *(DWORD *)((char *)buffer[i].destination_addr + 2);

        DNS_RRSET_ADD( rrset, (DNS_RECORD *)record );
    }
    status = ERROR_SUCCESS;

exit:
    DNS_RRSET_TERMINATE( rrset );

    if (status != ERROR_SUCCESS)
        DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );
    else
        *result = (DNS_RECORDA *)rrset.pFirstRR;

    return status;
}

static char *read_etc_hosts( DWORD *ret_size )
{
    HANDLE file;
    DWORD size;
    char *data;

    file = CreateFileW( L"\\\\?\\unix/etc/hosts", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if (file == INVALID_HANDLE_VALUE)
    {
        ERR( "failed to open /etc/hosts: %lu\n", GetLastError() );
        return NULL;
    }

    size = GetFileSize( file, NULL );
    if (!(data = malloc( size + 1)) || !ReadFile( file, data, size, ret_size, NULL ))
    {
        WARN( "failed to read file: %lu\n", GetLastError() );
        free( data );
        data = NULL;
    }
    data[size] = '\0';
    CloseHandle( file );
    return data;
}

static struct host_entry
{
    IP4_ADDRESS *ip4;
    IP6_ADDRESS *ip6;
    char        *name;
    char        *aliases;
} **host_entries;

static size_t host_entries_count;
static size_t host_entries_allocated;

static struct host_entry *create_host_entry( const IP4_ADDRESS *ip4, const IP6_ADDRESS *ip6, const char *name,
                                             size_t len_name, const char *aliases, size_t len_aliases )
{
    struct host_entry *ret;
    size_t size = sizeof(*ret) + len_name + 1 + len_aliases + 1;
    char *ptr;

    if (ip4) size += sizeof(*ip4);
    else if (ip6) size += sizeof(*ip6);

    if (!(ret = calloc( 1, size ))) return NULL;
    ptr = (char *)(ret + 1);

    if (ip4)
    {
        ret->ip4 = (IP4_ADDRESS *)ptr;
        *ret->ip4 = *ip4;
        ptr += sizeof(*ip4);
    }
    else if (ip6)
    {
        ret->ip6 = (IP6_ADDRESS *)ptr;
        *ret->ip6 = *ip6;
        ptr += sizeof(*ip6);
    }

    ret->name = ptr;
    memcpy( ret->name, name, len_name );
    ret->name[len_name] = 0;

    if (aliases)
    {
        ret->aliases = ptr + len_name + 1;
        memcpy( ret->aliases, aliases, len_aliases );
        ret->aliases[len_aliases] = 0;
    }
    return ret;
}

/* returns "end" if there was no space */
static char *next_space( const char *p, const char *end )
{
    while (p < end && !isspace( *p )) p++;
    return (char *)p;
}

/* returns "end" if there was no non-space */
static char *next_non_space( const char *p, const char *end )
{
    while (p < end && isspace( *p )) p++;
    return (char *)p;
}

static struct host_entry *get_next_host_entry( const char **cursor, const char *end )
{
    const char *p = *cursor;

    while (p < end)
    {
        const char *q, *line_end, *next_line, *addr, *name;
        char *str, *aliases = NULL;
        size_t len, len_name, len_aliases = 0;
        struct host_entry *entry;
        struct in_addr in4;
        struct in6_addr in6;
        IP4_ADDRESS *ip4 = NULL;
        IP6_ADDRESS *ip6 = NULL;

        for (line_end = p; line_end < end && *line_end != '\n' && *line_end != '#'; line_end++) ;
        TRACE( "parsing line %s\n", debugstr_an(p, line_end - p) );

        for (next_line = line_end; next_line < end && *next_line != '\n'; next_line++) ;
        if (next_line < end) next_line++; /* skip over newline */

        if ((p = next_non_space( p, line_end )) == line_end) { p = next_line; continue; }

        /* parse address */
        addr = p;
        if ((p = next_space( p, line_end )) == line_end) { p = next_line; continue; }

        if (!RtlIpv4StringToAddressA( addr, TRUE, &q, &in4 ) && q == p) ip4 = (IP4_ADDRESS *)&in4;
        else if (!RtlIpv6StringToAddressA( addr, &q, &in6 ) && q == p) ip6 = (IP6_ADDRESS *)&in6;
        else
        {
            WARN( "can't parse address %s, skipping this line\n", debugstr_an(addr, p - addr) );
            p = next_line;
            continue;
        }

        if ((p = next_non_space( p, line_end )) == line_end) { p = next_line; continue; }

        /* parse name */
        name = p;
        p = next_space( p, line_end );
        len_name = p - name;

        /* parse aliases */
        while ((p = next_non_space( p, line_end )) < line_end && (aliases || (str = aliases = malloc( line_end - p + 1 ))))
        {
            q = next_space( p, line_end );
            len = q - p;
            memcpy( str, p, len );
            str[len++] = 0;

            len_aliases += len;
            str += len;
            p = q;
        }

        entry = create_host_entry( ip4, ip6, name, len_name, aliases, len_aliases );
        free( aliases );
        if (!entry) return NULL;

        *cursor = next_line;
        return entry;
    }
    return NULL;
}

static DNS_STATUS append_host_entry( struct host_entry *entry )
{
    if (host_entries_count >= host_entries_allocated)
    {
        struct host_entry **tmp;
        size_t count = host_entries_allocated ? host_entries_allocated * 2 : 8;

        if (!(tmp = realloc( host_entries, count * sizeof(*tmp) ))) return ERROR_NOT_ENOUGH_MEMORY;
        host_entries_allocated = count;
        host_entries = tmp;
    }

    host_entries[host_entries_count++] = entry;
    return ERROR_SUCCESS;
}

void free_host_entries( void )
{
    unsigned int i;
    for (i = 0; i < host_entries_count; i++) free( host_entries[i] );
    host_entries_count = host_entries_allocated = 0;
    free( host_entries );
    host_entries = NULL;
}

static DNS_STATUS parse_etc_hosts( void )
{
    DNS_STATUS status = ERROR_SUCCESS;
    const char *cursor;
    char *data;
    struct host_entry *entry;
    DWORD size;

    if (host_entries) return ERROR_SUCCESS;

    EnterCriticalSection( &hosts_cs );

    if (!(cursor = data = read_etc_hosts( &size )))
    {
        LeaveCriticalSection( &hosts_cs );
        return DNS_ERROR_DATAFILE_OPEN_FAILURE;
    }

    while ((entry = get_next_host_entry( &cursor, data + size )))
    {
        if ((status = append_host_entry( entry )))
        {
            free_host_entries();
            break;
        }
    }

    free( data );
    LeaveCriticalSection( &hosts_cs );
    return status;
}

static DNS_RECORDA *create_a_record( const char *name, const IP4_ADDRESS *ip4  )
{
    DNS_RECORDA *rec;

    if (!(rec = alloc_record( name ))) return NULL;
    rec->wType            = DNS_TYPE_A;
    rec->wDataLength      = sizeof(rec->Data.A);
    rec->Flags.S.Section  = DnsSectionAnswer;
    rec->Flags.S.CharSet  = DnsCharSetUtf8;
    rec->Flags.S.Reserved = 0x20;
    rec->dwTtl            = DEFAULT_TTL_HOSTS;
    rec->Data.A.IpAddress = *ip4;
    return rec;
}

static DNS_RECORDA *create_aaaa_record( const char *name, const IP6_ADDRESS *ip6 )
{
    DNS_RECORDA *rec;

    if (!(rec = alloc_record( name ))) return NULL;
    rec->wType                = DNS_TYPE_AAAA;
    rec->wDataLength          = sizeof(rec->Data.AAAA);
    rec->Flags.S.Section      = DnsSectionAnswer;
    rec->Flags.S.CharSet      = DnsCharSetUtf8;
    rec->Flags.S.Reserved     = 0x20;
    rec->dwTtl                = DEFAULT_TTL_HOSTS;
    rec->Data.AAAA.Ip6Address = *ip6;
    return rec;
}

static DNS_RECORDA *create_cname_record( const char *name, const char *cname )
{
    DNS_RECORDA *rec;

    if (!(rec = alloc_record( name ))) return NULL;
    rec->wType            = DNS_TYPE_CNAME;
    rec->wDataLength      = sizeof(rec->Data.CNAME);
    rec->Flags.S.Section  = DnsSectionAnswer;
    rec->Flags.S.CharSet  = DnsCharSetUtf8;
    rec->Flags.S.Reserved = 0x30;
    rec->dwTtl            = DEFAULT_TTL_HOSTS;
    if (!(rec->Data.CNAME.pNameHost = strdup( cname )))
    {
        free( rec );
        return NULL;
    }
    return rec;
}

static DNS_STATUS do_query_hosts( const char *name, WORD type, DNS_RECORDA **result )
{
    DNS_RRSET rrset;
    DNS_STATUS status;
    unsigned int i;

    if ((status = parse_etc_hosts())) return status;

    DNS_RRSET_INIT( rrset );

    status = DNS_ERROR_RECORD_DOES_NOT_EXIST;

    for (i = 0; i < host_entries_count; i++)
    {
        const struct host_entry *entry = host_entries[i];
        DNS_RECORDA *rec = NULL;
        const char *ptr;

        switch (type)
        {
        case DNS_TYPE_A:
            if (!entry->ip4) break;
            for (ptr = entry->aliases; ptr && *ptr; ptr += strlen( ptr ) + 1)
            {
                if (!strcasecmp( ptr, name ) && (rec = create_cname_record( ptr, entry->name )))
                {
                    DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
                    if ((rec = create_a_record( entry->name, entry->ip4 ))) DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
                    break;
                }
            }
            if (!rec && !strcasecmp( entry->name, name ) && (rec = create_a_record( entry->name, entry->ip4 )))
                DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
            break;

        case DNS_TYPE_AAAA:
            if (!entry->ip6) break;
            for (ptr = entry->aliases; ptr && *ptr; ptr += strlen( ptr ) + 1)
            {
                if (!strcasecmp( ptr, name ) && (rec = create_cname_record( ptr, entry->name )))
                {
                    DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
                    if ((rec = create_aaaa_record( entry->name, entry->ip6 ))) DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
                    break;
                }
            }
            if (!rec && !strcasecmp( entry->name, name ) && (rec = create_aaaa_record( entry->name, entry->ip6 )))
                DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
            break;

        case DNS_TYPE_CNAME:
            for (ptr = entry->aliases; ptr && *ptr; ptr += strlen( ptr ) + 1)
                if (!strcasecmp( ptr, name ) && (rec = create_cname_record( ptr, entry->name )))
                {
                    DNS_RRSET_ADD( rrset, (DNS_RECORD *)rec );
                    break;
                }
            break;

        default: break;
        }

        if (rec)
        {
            status = ERROR_SUCCESS;
            break;
        }
    }

    DNS_RRSET_TERMINATE( rrset );

    if (status != ERROR_SUCCESS)
        DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );
    else
        *result = (DNS_RECORDA *)rrset.pFirstRR;

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
    DNS_STATUS ret;
    unsigned char answer[4096];
    DWORD len = sizeof(answer);
    struct query_params query_params = { name, type, options, answer, &len };
    const char *end;

    TRACE( "(%s, %s, %#lx, %p, %p, %p)\n", debugstr_a(name), debugstr_type( type ),
           options, servers, result, reserved );

    if (!name || !result) return ERROR_INVALID_PARAMETER;

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

    if ((ret = DnsValidateName_UTF8( name, DnsNameDomain )) && ret != DNS_ERROR_NON_RFC_NAME) return ret;

    if ((type == DNS_TYPE_A || type == DNS_TYPE_AAAA || type == DNS_TYPE_CNAME) &&
        !(options & DNS_QUERY_NO_HOSTS_FILE))
    {
        if (!do_query_hosts( name, type, result )) return ERROR_SUCCESS;
        TRACE( "hosts lookup failed, trying dns query\n" );
    }

    if ((ret = RESOLV_CALL( set_serverlist, servers ))) return ret;
    if (!(ret = RESOLV_CALL( query, &query_params )))
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

    if (ret == DNS_ERROR_RCODE_NAME_ERROR && type == DNS_TYPE_A && !(options & DNS_QUERY_NO_NETBT))
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
