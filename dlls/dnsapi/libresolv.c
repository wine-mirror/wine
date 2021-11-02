/*
 * Unix interface for libresolv
 *
 * Copyright 2021 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#ifdef HAVE_RESOLV
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winnls.h"
#include "windns.h"
#define USE_WS_PREFIX
#include "ws2def.h"
#include "ws2ipdef.h"

#include "wine/debug.h"
#include "wine/heap.h"
#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

static CPTABLEINFO unix_cptable;
static ULONG unix_cp = CP_UTF8;

static DWORD WINAPI get_unix_codepage_once( RTL_RUN_ONCE *once, void *param, void **context )
{
    static const WCHAR wineunixcpW[] = { 'W','I','N','E','U','N','I','X','C','P',0 };
    UNICODE_STRING name, value;
    WCHAR value_buffer[13];
    SIZE_T size;
    void *ptr;

    RtlInitUnicodeString( &name, wineunixcpW );
    value.Buffer = value_buffer;
    value.MaximumLength = sizeof(value_buffer);
    if (!RtlQueryEnvironmentVariable_U( NULL, &name, &value ))
        RtlUnicodeStringToInteger( &value, 10, &unix_cp );
    if (unix_cp != CP_UTF8 && !NtGetNlsSectionPtr( 11, unix_cp, NULL, &ptr, &size ))
        RtlInitCodePageTable( ptr, &unix_cptable );
    return TRUE;
}

static BOOL get_unix_codepage( void )
{
    static RTL_RUN_ONCE once = RTL_RUN_ONCE_INIT;

    return !RtlRunOnceExecuteOnce( &once, get_unix_codepage_once, NULL, NULL );
}

static DWORD dnsapi_umbstowcs( const char *src, WCHAR *dst, DWORD dstlen )
{
    DWORD srclen = strlen( src ) + 1;
    DWORD len;

    get_unix_codepage();

    if (unix_cp == CP_UTF8)
    {
        RtlUTF8ToUnicodeN( dst, dstlen, &len, src, srclen );
        return len;
    }
    else
    {
        len = srclen * sizeof(WCHAR);
        if (dst) RtlCustomCPToUnicodeN( &unix_cptable, dst, dstlen, &len, src, srclen );
        return len;
    }
}

/* call res_init() just once because of a bug in Mac OS X 10.4 */
/* call once per thread on systems that have per-thread _res */
static void init_resolver( void )
{
    if (!(_res.options & RES_INIT)) res_init();
}

static unsigned long map_options( DWORD options )
{
    unsigned long ret = 0;

    if (options == DNS_QUERY_STANDARD)
        return RES_DEFAULT;

    if (options & DNS_QUERY_ACCEPT_TRUNCATED_RESPONSE)
        ret |= RES_IGNTC;
    if (options & DNS_QUERY_USE_TCP_ONLY)
        ret |= RES_USEVC;
    if (options & DNS_QUERY_NO_RECURSION)
        ret &= ~RES_RECURSE;
    if (options & DNS_QUERY_NO_LOCAL_NAME)
        ret &= ~RES_DNSRCH;
    if (options & DNS_QUERY_NO_HOSTS_FILE)
        ret |= RES_NOALIASES;
    if (options & DNS_QUERY_TREAT_AS_FQDN)
        ret &= ~RES_DEFNAMES;

    if (options & DNS_QUERY_DONT_RESET_TTL_VALUES)
        FIXME( "option DNS_QUERY_DONT_RESET_TTL_VALUES not implemented\n" );
    if (options & DNS_QUERY_RESERVED)
        FIXME( "option DNS_QUERY_RESERVED not implemented\n" );
    if (options & DNS_QUERY_WIRE_ONLY)
        FIXME( "option DNS_QUERY_WIRE_ONLY not implemented\n" );
    if (options & DNS_QUERY_NO_WIRE_QUERY)
        FIXME( "option DNS_QUERY_NO_WIRE_QUERY not implemented\n" );
    if (options & DNS_QUERY_BYPASS_CACHE)
        FIXME( "option DNS_QUERY_BYPASS_CACHE not implemented\n" );
    if (options & DNS_QUERY_RETURN_MESSAGE)
        FIXME( "option DNS_QUERY_RETURN_MESSAGE not implemented\n" );

    if (options & DNS_QUERY_NO_NETBT)
        TRACE( "netbios query disabled\n" );

    return ret;
}

static DNS_STATUS map_h_errno( int error )
{
    switch (error)
    {
    case NO_DATA:
    case HOST_NOT_FOUND: return DNS_ERROR_RCODE_NAME_ERROR;
    case TRY_AGAIN:      return DNS_ERROR_RCODE_SERVER_FAILURE;
    case NO_RECOVERY:    return DNS_ERROR_RCODE_REFUSED;
#ifdef NETDB_INTERNAL
    case NETDB_INTERNAL: return DNS_ERROR_RCODE;
#endif
    default:
        FIXME( "unmapped error code: %d\n", error );
        return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    }
}

static DNS_STATUS CDECL resolv_get_searchlist( DNS_TXT_DATAW *list, DWORD *len )
{
    DWORD i, needed, str_needed = 0;
    char *ptr, *end;

    init_resolver();

    for (i = 0; i < MAXDNSRCH + 1 && _res.dnsrch[i]; i++)
        str_needed += dnsapi_umbstowcs( _res.dnsrch[i], NULL, 0 );

    needed = FIELD_OFFSET(DNS_TXT_DATAW, pStringArray[i]) + str_needed;

    if (!list || *len < needed)
    {
        *len = needed;
        return !list ? ERROR_SUCCESS : ERROR_MORE_DATA;
    }

    *len = needed;
    list->dwStringCount = i;

    ptr = (char *)(list->pStringArray + i);
    end = ptr + str_needed;
    for (i = 0; i < MAXDNSRCH + 1 && _res.dnsrch[i]; i++)
    {
        list->pStringArray[i] = (WCHAR *)ptr;
        ptr += dnsapi_umbstowcs( _res.dnsrch[i], list->pStringArray[i], end - ptr );
    }
    return ERROR_SUCCESS;
}


static inline int filter( unsigned short sin_family, USHORT family )
{
    if (sin_family != AF_INET && sin_family != AF_INET6) return TRUE;
    if (sin_family == AF_INET6 && family == WS_AF_INET) return TRUE;
    if (sin_family == AF_INET && family == WS_AF_INET6) return TRUE;

    return FALSE;
}

#ifdef HAVE_RES_GETSERVERS

static DNS_STATUS CDECL resolv_get_serverlist( USHORT family, DNS_ADDR_ARRAY *addrs, DWORD *len )
{
    struct __res_state *state = &_res;
    DWORD i, found, total, needed;
    union res_sockaddr_union *buf;

    init_resolver();

    total = res_getservers( state, NULL, 0 );
    if (!total) return DNS_ERROR_NO_DNS_SERVERS;

    if (!addrs && family != WS_AF_INET && family != WS_AF_INET6)
    {
        *len = FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[total]);
        return ERROR_SUCCESS;
    }

    buf = malloc( total * sizeof(union res_sockaddr_union) );
    if (!buf) return ERROR_NOT_ENOUGH_MEMORY;

    total = res_getservers( state, buf, total );

    for (i = 0, found = 0; i < total; i++)
    {
        if (filter( buf[i].sin.sin_family, family )) continue;
        found++;
    }
    if (!found) return DNS_ERROR_NO_DNS_SERVERS;

    needed = FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[found]);
    if (!addrs || *len < needed)
    {
        *len = needed;
        return !addrs ? ERROR_SUCCESS : ERROR_MORE_DATA;
    }
    *len = needed;
    memset( addrs, 0, needed );
    addrs->AddrCount = addrs->MaxCount = found;

    for (i = 0, found = 0; i < total; i++)
    {
        if (filter( buf[i].sin.sin_family, family )) continue;

        if (buf[i].sin6.sin6_family == AF_INET6)
        {
            SOCKADDR_IN6 *sa = (SOCKADDR_IN6 *)addrs->AddrArray[found].MaxSa;
            sa->sin6_family = WS_AF_INET6;
            memcpy( &sa->sin6_addr, &buf[i].sin6.sin6_addr, sizeof(sa->sin6_addr) );
            addrs->AddrArray[found].Data.DnsAddrUserDword[0] = sizeof(*sa);
        }
        else
        {
            SOCKADDR_IN *sa = (SOCKADDR_IN *)addrs->AddrArray[found].MaxSa;
            sa->sin_family = WS_AF_INET;
            sa->sin_addr.WS_s_addr = buf[i].sin.sin_addr.s_addr;
            addrs->AddrArray[found].Data.DnsAddrUserDword[0] = sizeof(*sa);
        }
        found++;
    }

    free( buf );
    return ERROR_SUCCESS;
}

#else

static DNS_STATUS CDECL resolv_get_serverlist( USHORT family, DNS_ADDR_ARRAY *addrs, DWORD *len )
{
    DWORD needed, found, i;

    init_resolver();

    if (!_res.nscount) return DNS_ERROR_NO_DNS_SERVERS;
    if (!addrs && family != WS_AF_INET && family != WS_AF_INET6)
    {
        *len = FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[_res.nscount]);
        return ERROR_SUCCESS;
    }

    for (i = 0, found = 0; i < _res.nscount; i++)
    {
        unsigned short sin_family = AF_INET;
#ifdef HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6
        if (_res._u._ext.nsaddrs[i]) sin_family = _res._u._ext.nsaddrs[i]->sin6_family;
#endif
        if (filter( sin_family, family )) continue;
        found++;
    }
    if (!found) return DNS_ERROR_NO_DNS_SERVERS;

    needed = FIELD_OFFSET(DNS_ADDR_ARRAY, AddrArray[found]);
    if (!addrs || *len < needed)
    {
        *len = needed;
        return !addrs ? ERROR_SUCCESS : ERROR_MORE_DATA;
    }
    *len = needed;
    memset( addrs, 0, needed );
    addrs->AddrCount = addrs->MaxCount = found;

    for (i = 0, found = 0; i < _res.nscount; i++)
    {
        unsigned short sin_family = AF_INET;
#ifdef HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6
        if (_res._u._ext.nsaddrs[i]) sin_family = _res._u._ext.nsaddrs[i]->sin6_family;
#endif
        if (filter( sin_family, family )) continue;

#ifdef HAVE_STRUCT___RES_STATE__U__EXT_NSCOUNT6
        if (sin_family == AF_INET6)
        {
            SOCKADDR_IN6 *sa = (SOCKADDR_IN6 *)addrs->AddrArray[found].MaxSa;
            sa->sin6_family = WS_AF_INET6;
            memcpy( &sa->sin6_addr, &_res._u._ext.nsaddrs[i]->sin6_addr, sizeof(sa->sin6_addr) );
            addrs->AddrArray[found].Data.DnsAddrUserDword[0] = sizeof(*sa);
        }
        else
#endif
        {
            SOCKADDR_IN *sa = (SOCKADDR_IN *)addrs->AddrArray[found].MaxSa;
            sa->sin_family = WS_AF_INET;
            sa->sin_addr.WS_s_addr = _res.nsaddr_list[i].sin_addr.s_addr;
            addrs->AddrArray[found].Data.DnsAddrUserDword[0] = sizeof(*sa);
        }
        found++;
    }

    return ERROR_SUCCESS;
}
#endif

static DNS_STATUS CDECL resolv_set_serverlist( const IP4_ARRAY *addrs )
{
    int i;

    init_resolver();

    if (!addrs || !addrs->AddrCount) return ERROR_SUCCESS;
    if (addrs->AddrCount > MAXNS)
    {
        WARN( "too many servers: %d only using the first: %d\n",
              addrs->AddrCount, MAXNS );
        _res.nscount = MAXNS;
    }
    else _res.nscount = addrs->AddrCount;

    for (i = 0; i < _res.nscount; i++)
        _res.nsaddr_list[i].sin_addr.s_addr = addrs->AddrArray[i];

    return ERROR_SUCCESS;
}

static DNS_STATUS CDECL resolv_query( const char *name, WORD type, DWORD options, void *answer, DWORD *retlen )
{
    DNS_STATUS ret = ERROR_SUCCESS;
    int len;

    init_resolver();
    _res.options |= map_options( options );

    if ((len = res_query( name, ns_c_in, type, answer, *retlen )) < 0)
        ret = map_h_errno( h_errno );
    else
        *retlen = len;
    return ret;
}

static const struct resolv_funcs funcs =
{
    resolv_get_searchlist,
    resolv_get_serverlist,
    resolv_query,
    resolv_set_serverlist
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    *(const struct resolv_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}

#endif /* HAVE_RESOLV */
