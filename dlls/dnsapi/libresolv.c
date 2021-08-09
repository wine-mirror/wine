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

static const char *debugstr_type( unsigned short type )
{
    const char *str;

    switch (type)
    {
#define X(x)    case (x): str = #x; break;
    X(DNS_TYPE_ZERO)
    X(DNS_TYPE_A)
    X(DNS_TYPE_NS)
    X(DNS_TYPE_MD)
    X(DNS_TYPE_MF)
    X(DNS_TYPE_CNAME)
    X(DNS_TYPE_SOA)
    X(DNS_TYPE_MB)
    X(DNS_TYPE_MG)
    X(DNS_TYPE_MR)
    X(DNS_TYPE_NULL)
    X(DNS_TYPE_WKS)
    X(DNS_TYPE_PTR)
    X(DNS_TYPE_HINFO)
    X(DNS_TYPE_MINFO)
    X(DNS_TYPE_MX)
    X(DNS_TYPE_TEXT)
    X(DNS_TYPE_RP)
    X(DNS_TYPE_AFSDB)
    X(DNS_TYPE_X25)
    X(DNS_TYPE_ISDN)
    X(DNS_TYPE_RT)
    X(DNS_TYPE_NSAP)
    X(DNS_TYPE_NSAPPTR)
    X(DNS_TYPE_SIG)
    X(DNS_TYPE_KEY)
    X(DNS_TYPE_PX)
    X(DNS_TYPE_GPOS)
    X(DNS_TYPE_AAAA)
    X(DNS_TYPE_LOC)
    X(DNS_TYPE_NXT)
    X(DNS_TYPE_EID)
    X(DNS_TYPE_NIMLOC)
    X(DNS_TYPE_SRV)
    X(DNS_TYPE_ATMA)
    X(DNS_TYPE_NAPTR)
    X(DNS_TYPE_KX)
    X(DNS_TYPE_CERT)
    X(DNS_TYPE_A6)
    X(DNS_TYPE_DNAME)
    X(DNS_TYPE_SINK)
    X(DNS_TYPE_OPT)
    X(DNS_TYPE_UINFO)
    X(DNS_TYPE_UID)
    X(DNS_TYPE_GID)
    X(DNS_TYPE_UNSPEC)
    X(DNS_TYPE_ADDRS)
    X(DNS_TYPE_TKEY)
    X(DNS_TYPE_TSIG)
    X(DNS_TYPE_IXFR)
    X(DNS_TYPE_AXFR)
    X(DNS_TYPE_MAILB)
    X(DNS_TYPE_MAILA)
    X(DNS_TYPE_ANY)
    X(DNS_TYPE_WINS)
    X(DNS_TYPE_WINSR)
#undef X
    default:
        return wine_dbg_sprintf( "0x%04x", type );
    }

    return wine_dbg_sprintf( "%s", str );
}

static const char *debugstr_section( ns_sect section )
{
    switch (section)
    {
    case ns_s_qd:   return "Question";
    case ns_s_an:   return "Answer";
    case ns_s_ns:   return "Authority";
    case ns_s_ar:   return "Additional";
    default:
        return wine_dbg_sprintf( "0x%02x", section );
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

static DNS_STATUS map_error( int error )
{
    switch (error)
    {
    case ns_r_noerror:  return ERROR_SUCCESS;
    case ns_r_formerr:  return DNS_ERROR_RCODE_FORMAT_ERROR;
    case ns_r_servfail: return DNS_ERROR_RCODE_SERVER_FAILURE;
    case ns_r_nxdomain: return DNS_ERROR_RCODE_NAME_ERROR;
    case ns_r_notimpl:  return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    case ns_r_refused:  return DNS_ERROR_RCODE_REFUSED;
    case ns_r_yxdomain: return DNS_ERROR_RCODE_YXDOMAIN;
    case ns_r_yxrrset:  return DNS_ERROR_RCODE_YXRRSET;
    case ns_r_nxrrset:  return DNS_ERROR_RCODE_NXRRSET;
    case ns_r_notauth:  return DNS_ERROR_RCODE_NOTAUTH;
    case ns_r_notzone:  return DNS_ERROR_RCODE_NOTZONE;
    default:
        FIXME( "unmapped error code: %d\n", error );
        return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    }
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

DNS_STATUS CDECL resolv_get_searchlist( DNS_TXT_DATAW *list, DWORD *len )
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

DNS_STATUS CDECL resolv_get_serverlist( USHORT family, DNS_ADDR_ARRAY *addrs, DWORD *len )
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

DNS_STATUS CDECL resolv_get_serverlist( USHORT family, DNS_ADDR_ARRAY *addrs, DWORD *len )
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

DNS_STATUS CDECL resolv_set_serverlist( const IP4_ARRAY *addrs )
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

static char *dname_from_msg( ns_msg msg, const unsigned char *pos )
{
    char *str, dname[NS_MAXDNAME] = ".";

    /* returns *compressed* length, ignore it */
    ns_name_uncompress( ns_msg_base(msg), ns_msg_end(msg), pos, dname, sizeof(dname) );

    if ((str = RtlAllocateHeap( GetProcessHeap(), 0, strlen(dname) + 1 ))) strcpy( str, dname );
    return str;
}

static char *str_from_rdata( const unsigned char *rdata )
{
    char *str;
    unsigned int len = rdata[0];

    if ((str = RtlAllocateHeap( GetProcessHeap(), 0, len + 1 )))
    {
        memcpy( str, ++rdata, len );
        str[len] = 0;
    }
    return str;
}

static unsigned int get_record_size( const ns_rr *rr )
{
    const unsigned char *pos = rr->rdata;
    unsigned int num = 0, size = sizeof(DNS_RECORDA);

    switch (rr->type)
    {
    case ns_t_key:
    {
        pos += sizeof(WORD) + sizeof(BYTE) + sizeof(BYTE);
        size += rr->rdata + rr->rdlength - pos - 1;
        break;
    }
    case ns_t_sig:
    {
        pos += sizeof(PCHAR) + sizeof(WORD) + 2 * sizeof(BYTE);
        pos += 3 * sizeof(DWORD) + 2 * sizeof(WORD);
        size += rr->rdata + rr->rdlength - pos - 1;
        break;
    }
    case ns_t_hinfo:
    case ns_t_isdn:
    case ns_t_txt:
    case ns_t_x25:
    {
        while (pos[0] && pos < rr->rdata + rr->rdlength)
        {
            num++;
            pos += pos[0] + 1;
        }
        size += (num - 1) * sizeof(PCHAR);
        break;
    }
    case ns_t_null:
    case ns_t_opt:
    {
        size += rr->rdlength - 1;
        break;
    }
    case ns_t_nxt:
    case ns_t_wks:
    case 0xff01:  /* WINS */
    {
        FIXME( "unhandled type: %s\n", debugstr_type( rr->type ) );
        break;
    }
    default:
        break;
    }
    return size;
}

static DNS_STATUS copy_rdata( ns_msg msg, const ns_rr *rr, DNS_RECORDA *r, WORD *dlen )
{
    DNS_STATUS ret = ERROR_SUCCESS;
    const unsigned char *pos = rr->rdata;
    unsigned int i, size;

    switch (rr->type)
    {
    case ns_t_a:
    {
        r->Data.A.IpAddress = *(const DWORD *)pos;
        *dlen = sizeof(DNS_A_DATA);
        break;
    }
    case ns_t_aaaa:
    {
        for (i = 0; i < sizeof(IP6_ADDRESS)/sizeof(DWORD); i++)
        {
            r->Data.AAAA.Ip6Address.IP6Dword[i] = *(const DWORD *)pos;
            pos += sizeof(DWORD);
        }

        *dlen = sizeof(DNS_AAAA_DATA);
        break;
    }
    case ns_t_key:
    {
        /* FIXME: byte order? */
        r->Data.KEY.wFlags      = *(const WORD *)pos;   pos += sizeof(WORD);
        r->Data.KEY.chProtocol  = *pos++;
        r->Data.KEY.chAlgorithm = *pos++;

        size = rr->rdata + rr->rdlength - pos;

        for (i = 0; i < size; i++)
            r->Data.KEY.Key[i] = *pos++;

        *dlen = sizeof(DNS_KEY_DATA) + (size - 1) * sizeof(BYTE);
        break;
    }
    case ns_t_rp:
    case ns_t_minfo:
    {
        r->Data.MINFO.pNameMailbox = dname_from_msg( msg, pos );
        if (!r->Data.MINFO.pNameMailbox) return ERROR_NOT_ENOUGH_MEMORY;

        if (ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        r->Data.MINFO.pNameErrorsMailbox = dname_from_msg( msg, pos );
        if (!r->Data.MINFO.pNameErrorsMailbox)
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.MINFO.pNameMailbox );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        *dlen = sizeof(DNS_MINFO_DATAA);
        break;
    }
    case ns_t_afsdb:
    case ns_t_rt:
    case ns_t_mx:
    {
        r->Data.MX.wPreference = ntohs( *(const WORD *)pos );
        r->Data.MX.pNameExchange = dname_from_msg( msg, pos + sizeof(WORD) );
        if (!r->Data.MX.pNameExchange) return ERROR_NOT_ENOUGH_MEMORY;

        *dlen = sizeof(DNS_MX_DATAA);
        break;
    }
    case ns_t_null:
    {
        r->Data.Null.dwByteCount = rr->rdlength;
        memcpy( r->Data.Null.Data, rr->rdata, rr->rdlength );

        *dlen = sizeof(DNS_NULL_DATA) + rr->rdlength - 1;
        break;
    }
    case ns_t_opt:
    {
        r->Data.OPT.wDataLength = rr->rdlength;
        r->Data.OPT.wPad        = 0;
        memcpy( r->Data.OPT.Data, rr->rdata, rr->rdlength );

        *dlen = sizeof(DNS_OPT_DATA) + rr->rdlength - 1;
        break;
    }
    case ns_t_cname:
    case ns_t_ns:
    case ns_t_mb:
    case ns_t_md:
    case ns_t_mf:
    case ns_t_mg:
    case ns_t_mr:
    case ns_t_ptr:
    {
        r->Data.PTR.pNameHost = dname_from_msg( msg, pos );
        if (!r->Data.PTR.pNameHost) return ERROR_NOT_ENOUGH_MEMORY;

        *dlen = sizeof(DNS_PTR_DATAA);
        break;
    }
    case ns_t_sig:
    {
        r->Data.SIG.pNameSigner = dname_from_msg( msg, pos );
        if (!r->Data.SIG.pNameSigner) return ERROR_NOT_ENOUGH_MEMORY;

        if (ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        /* FIXME: byte order? */
        r->Data.SIG.wTypeCovered  = *(const WORD *)pos;   pos += sizeof(WORD);
        r->Data.SIG.chAlgorithm   = *pos++;
        r->Data.SIG.chLabelCount  = *pos++;
        r->Data.SIG.dwOriginalTtl = *(const DWORD *)pos;  pos += sizeof(DWORD);
        r->Data.SIG.dwExpiration  = *(const DWORD *)pos;  pos += sizeof(DWORD);
        r->Data.SIG.dwTimeSigned  = *(const DWORD *)pos;  pos += sizeof(DWORD);
        r->Data.SIG.wKeyTag       = *(const WORD *)pos;

        size = rr->rdata + rr->rdlength - pos;

        for (i = 0; i < size; i++)
            r->Data.SIG.Signature[i] = *pos++;

        *dlen = sizeof(DNS_SIG_DATAA) + (size - 1) * sizeof(BYTE);
        break;
    }
    case ns_t_soa:
    {
        r->Data.SOA.pNamePrimaryServer = dname_from_msg( msg, pos );
        if (!r->Data.SOA.pNamePrimaryServer) return ERROR_NOT_ENOUGH_MEMORY;

        if (ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        r->Data.SOA.pNameAdministrator = dname_from_msg( msg, pos );
        if (!r->Data.SOA.pNameAdministrator)
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.SOA.pNamePrimaryServer );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        r->Data.SOA.dwSerialNo   = ntohl( *(const DWORD *)pos ); pos += sizeof(DWORD);
        r->Data.SOA.dwRefresh    = ntohl( *(const DWORD *)pos ); pos += sizeof(DWORD);
        r->Data.SOA.dwRetry      = ntohl( *(const DWORD *)pos ); pos += sizeof(DWORD);
        r->Data.SOA.dwExpire     = ntohl( *(const DWORD *)pos ); pos += sizeof(DWORD);
        r->Data.SOA.dwDefaultTtl = ntohl( *(const DWORD *)pos ); pos += sizeof(DWORD);

        *dlen = sizeof(DNS_SOA_DATAA);
        break;
    }
    case ns_t_srv:
    {
        r->Data.SRV.wPriority = ntohs( *(const WORD *)pos ); pos += sizeof(WORD);
        r->Data.SRV.wWeight   = ntohs( *(const WORD *)pos ); pos += sizeof(WORD);
        r->Data.SRV.wPort     = ntohs( *(const WORD *)pos ); pos += sizeof(WORD);

        r->Data.SRV.pNameTarget = dname_from_msg( msg, pos );
        if (!r->Data.SRV.pNameTarget) return ERROR_NOT_ENOUGH_MEMORY;

        *dlen = sizeof(DNS_SRV_DATAA);
        break;
    }
    case ns_t_hinfo:
    case ns_t_isdn:
    case ns_t_x25:
    case ns_t_txt:
    {
        i = 0;
        while (pos[0] && pos < rr->rdata + rr->rdlength)
        {
            r->Data.TXT.pStringArray[i] = str_from_rdata( pos );
            if (!r->Data.TXT.pStringArray[i])
            {
                while (i > 0) RtlFreeHeap( GetProcessHeap(), 0, r->Data.TXT.pStringArray[--i] );
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            i++;
            pos += pos[0] + 1;
        }
        r->Data.TXT.dwStringCount = i;
        *dlen = sizeof(DNS_TXT_DATAA) + (i - 1) * sizeof(PCHAR);
        break;
    }
    case ns_t_atma:
    case ns_t_loc:
    case ns_t_nxt:
    case ns_t_tsig:
    case ns_t_wks:
    case 0x00f9:  /* TKEY */
    case 0xff01:  /* WINS */
    case 0xff02:  /* WINSR */
    default:
        FIXME( "unhandled type: %s\n", debugstr_type( rr->type ) );
        return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    }

    return ret;
}

static inline char *heap_strdup( const char *src )
{
    char *dst;
    if (!src) return NULL;
    if ((dst = RtlAllocateHeap( GetProcessHeap(), 0, (strlen( src ) + 1) * sizeof(char) ))) strcpy( dst, src );
    return dst;
}

static DNS_STATUS copy_record( ns_msg msg, ns_sect section, unsigned short num, DNS_RECORDA **recp )
{
    DNS_STATUS ret;
    DNS_RECORDA *record;
    WORD dlen;
    ns_rr rr;

    if (ns_parserr( &msg, section, num, &rr ) < 0)
        return DNS_ERROR_BAD_PACKET;

    if (!(record = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, get_record_size( &rr ) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    if (!(record->pName = heap_strdup( rr.name )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, record );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    record->wType = rr.type;
    record->Flags.S.Section = section;
    record->Flags.S.CharSet = DnsCharSetUtf8;
    record->dwTtl = rr.ttl;

    if ((ret = copy_rdata( msg, &rr, record, &dlen )))
    {
        RtlFreeHeap( GetProcessHeap(), 0, record->pName );
        RtlFreeHeap( GetProcessHeap(), 0, record );
        return ret;
    }
    record->wDataLength = dlen;
    *recp = record;

    TRACE( "found %s record in %s section\n", debugstr_type( rr.type ), debugstr_section( section ) );
    return ERROR_SUCCESS;
}

static void free_record_list( DNS_RECORD *list )
{
    DNS_RECORD *r, *next;
    unsigned int i;

    for (r = list; (list = r); r = next)
    {
        RtlFreeHeap( GetProcessHeap(), 0, r->pName );

        switch (r->wType)
        {
        case DNS_TYPE_HINFO:
        case DNS_TYPE_ISDN:
        case DNS_TYPE_TEXT:
        case DNS_TYPE_X25:
        {
            for (i = 0; i < r->Data.TXT.dwStringCount; i++)
                RtlFreeHeap( GetProcessHeap(), 0, r->Data.TXT.pStringArray[i] );
            break;
        }
        case DNS_TYPE_MINFO:
        case DNS_TYPE_RP:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.MINFO.pNameMailbox );
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.MINFO.pNameErrorsMailbox );
            break;
        }
        case DNS_TYPE_AFSDB:
        case DNS_TYPE_RT:
        case DNS_TYPE_MX:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.MX.pNameExchange );
            break;
        }
        case DNS_TYPE_NXT:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.NXT.pNameNext );
            break;
        }
        case DNS_TYPE_CNAME:
        case DNS_TYPE_MB:
        case DNS_TYPE_MD:
        case DNS_TYPE_MF:
        case DNS_TYPE_MG:
        case DNS_TYPE_MR:
        case DNS_TYPE_NS:
        case DNS_TYPE_PTR:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.PTR.pNameHost );
            break;
        }
        case DNS_TYPE_SIG:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.SIG.pNameSigner );
            break;
        }
        case DNS_TYPE_SOA:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.SOA.pNamePrimaryServer );
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.SOA.pNameAdministrator );
            break;
        }
        case DNS_TYPE_SRV:
        {
            RtlFreeHeap( GetProcessHeap(), 0, r->Data.SRV.pNameTarget );
            break;
        }
        default: break;
        }

        next = r->pNext;
        RtlFreeHeap( GetProcessHeap(), 0, r );
    }
}

#define DNS_MAX_PACKET_SIZE 4096
DNS_STATUS CDECL resolv_query( const char *name, WORD type, DWORD options, DNS_RECORDA **result )
{
    DNS_STATUS ret = DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    unsigned int i, num;
    unsigned char answer[DNS_MAX_PACKET_SIZE];
    ns_sect sections[] = { ns_s_an, ns_s_ar };
    ns_msg msg;
    DNS_RECORDA *record = NULL;
    DNS_RRSET rrset;
    int len;

    DNS_RRSET_INIT( rrset );

    init_resolver();
    _res.options |= map_options( options );

    if ((len = res_query( name, ns_c_in, type, answer, sizeof(answer) )) < 0)
    {
        ret = map_h_errno( h_errno );
        goto exit;
    }

    if (ns_initparse( answer, len, &msg ) < 0)
    {
        ret = DNS_ERROR_BAD_PACKET;
        goto exit;
    }

#define RCODE_MASK 0x0f
    if ((msg._flags & RCODE_MASK) != ns_r_noerror)
    {
        ret = map_error( msg._flags & RCODE_MASK );
        goto exit;
    }

    for (i = 0; i < ARRAY_SIZE(sections); i++)
    {
        for (num = 0; num < ns_msg_count( msg, sections[i] ); num++)
        {
            ret = copy_record( msg, sections[i], num, &record );
            if (ret != ERROR_SUCCESS) goto exit;

            DNS_RRSET_ADD( rrset, (DNS_RECORD *)record );
        }
    }

exit:
    DNS_RRSET_TERMINATE( rrset );

    if (ret != ERROR_SUCCESS)
        free_record_list( rrset.pFirstRR );
    else
        *result = (DNS_RECORDA *)rrset.pFirstRR;

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
