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

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#include "windns.h"
#include "nb30.h"

#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

#ifdef HAVE_RESOLV

/* call res_init() just once because of a bug in Mac OS X 10.4 */
/* call once per thread on systems that have per-thread _res */
static void initialise_resolver( void )
{
    if ((_res.options & RES_INIT) == 0)
        res_init();
}

static const char *dns_section_to_str( ns_sect section )
{
    switch (section)
    {
    case ns_s_qd:   return "Question";
    case ns_s_an:   return "Answer";
    case ns_s_ns:   return "Authority";
    case ns_s_ar:   return "Additional";
    default:
    {
        static char tmp[5];
        FIXME( "unknown section: 0x%02x\n", section );
        sprintf( tmp, "0x%02x", section );
        return tmp;
    }
    }
}

static unsigned long dns_map_options( DWORD options )
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

static DNS_STATUS dns_map_error( int error )
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

static DNS_STATUS dns_map_h_errno( int error )
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

static char *dns_dname_from_msg( ns_msg msg, const unsigned char *pos )
{
    int len;
    char *str, dname[NS_MAXDNAME] = ".";

    /* returns *compressed* length, ignore it */
    dns_ns_name_uncompress( ns_msg_base(msg), ns_msg_end(msg), pos, dname, sizeof(dname) );

    len = strlen( dname );
    str = heap_alloc( len + 1 );
    if (str) strcpy( str, dname );
    return str;
}

static char *dns_str_from_rdata( const unsigned char *rdata )
{
    char *str;
    unsigned int len = rdata[0];

    str = heap_alloc( len + 1 );
    if (str)
    {
        memcpy( str, ++rdata, len );
        str[len] = '\0';
    }
    return str;
}

static unsigned int dns_get_record_size( const ns_rr *rr )
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
    {
        size += rr->rdlength - 1;
        break;
    }
    case ns_t_nxt:
    case ns_t_wks:
    case 0xff01:  /* WINS */
    {
        FIXME( "unhandled type: %s\n", dns_type_to_str( rr->type ) );
        break;
    }
    default:
        break;
    }
    return size;
}

static DNS_STATUS dns_copy_rdata( ns_msg msg, const ns_rr *rr, DNS_RECORDA *r, WORD *dlen )
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
        r->Data.MINFO.pNameMailbox = dns_dname_from_msg( msg, pos );
        if (!r->Data.MINFO.pNameMailbox) return ERROR_NOT_ENOUGH_MEMORY;

        if (dns_ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        r->Data.MINFO.pNameErrorsMailbox = dns_dname_from_msg( msg, pos );
        if (!r->Data.MINFO.pNameErrorsMailbox)
        {
            heap_free( r->Data.MINFO.pNameMailbox );
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
        r->Data.MX.pNameExchange = dns_dname_from_msg( msg, pos + sizeof(WORD) );
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
    case ns_t_cname:
    case ns_t_ns:
    case ns_t_mb:
    case ns_t_md:
    case ns_t_mf:
    case ns_t_mg:
    case ns_t_mr:
    case ns_t_ptr:
    {
        r->Data.PTR.pNameHost = dns_dname_from_msg( msg, pos );
        if (!r->Data.PTR.pNameHost) return ERROR_NOT_ENOUGH_MEMORY;

        *dlen = sizeof(DNS_PTR_DATAA);
        break;
    }
    case ns_t_sig:
    {
        r->Data.SIG.pNameSigner = dns_dname_from_msg( msg, pos );
        if (!r->Data.SIG.pNameSigner) return ERROR_NOT_ENOUGH_MEMORY;

        if (dns_ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
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
        r->Data.SOA.pNamePrimaryServer = dns_dname_from_msg( msg, pos );
        if (!r->Data.SOA.pNamePrimaryServer) return ERROR_NOT_ENOUGH_MEMORY;

        if (dns_ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
            return DNS_ERROR_BAD_PACKET;

        r->Data.SOA.pNameAdministrator = dns_dname_from_msg( msg, pos );
        if (!r->Data.SOA.pNameAdministrator)
        {
            heap_free( r->Data.SOA.pNamePrimaryServer );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (dns_ns_name_skip( &pos, ns_msg_end( msg ) ) < 0)
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

        r->Data.SRV.pNameTarget = dns_dname_from_msg( msg, pos );
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
            r->Data.TXT.pStringArray[i] = dns_str_from_rdata( pos );
            if (!r->Data.TXT.pStringArray[i])
            {
                while (i > 0) heap_free( r->Data.TXT.pStringArray[--i] );
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
        FIXME( "unhandled type: %s\n", dns_type_to_str( rr->type ) );
        return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    }

    return ret;
} 

static DNS_STATUS dns_copy_record( ns_msg msg, ns_sect section,
                                   unsigned short num, DNS_RECORDA **recp )
{
    DNS_STATUS ret;
    DNS_RECORDA *record;
    WORD dlen;
    ns_rr rr;

    if (dns_ns_parserr( &msg, section, num, &rr ) < 0)
        return DNS_ERROR_BAD_PACKET;

    if (!(record = heap_alloc_zero( dns_get_record_size( &rr ) )))
        return ERROR_NOT_ENOUGH_MEMORY;

    record->pName = dns_strdup_u( rr.name );
    if (!record->pName)
    {
        heap_free( record );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    record->wType = rr.type;
    record->Flags.S.Section = section;
    record->Flags.S.CharSet = DnsCharSetUtf8;
    record->dwTtl = rr.ttl;

    if ((ret = dns_copy_rdata( msg, &rr, record, &dlen )))
    {
        heap_free( record->pName );
        heap_free( record );
        return ret;
    }
    record->wDataLength = dlen;
    *recp = record;

    TRACE( "found %s record in %s section\n",
           dns_type_to_str( rr.type ), dns_section_to_str( section ) );
    return ERROR_SUCCESS;
}

#define DEFAULT_TTL  1200

static DNS_STATUS dns_do_query_netbios( PCSTR name, DNS_RECORDA **recp )
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
            record->pName = dns_strdup_u( name );
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

/* res_init() must have been called before calling these three functions.
 */
static DNS_STATUS dns_set_serverlist( const IP4_ARRAY *addrs )
{
    int i;

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

static DNS_STATUS dns_get_serverlist( PIP4_ARRAY addrs, PDWORD len )
{
    unsigned int size;
    int i;

    size = FIELD_OFFSET(IP4_ARRAY, AddrArray[_res.nscount]);
    if (!addrs || *len < size)
    {
        *len = size;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    addrs->AddrCount = _res.nscount;

    for (i = 0; i < _res.nscount; i++)
        addrs->AddrArray[i] = _res.nsaddr_list[i].sin_addr.s_addr;

    return ERROR_SUCCESS;
}

#define DNS_MAX_PACKET_SIZE 4096
static DNS_STATUS dns_do_query( PCSTR name, WORD type, DWORD options, PDNS_RECORDA *result )
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

    len = res_query( name, ns_c_in, type, answer, sizeof(answer) );
    if (len < 0)
    {
        ret = dns_map_h_errno( h_errno );
        goto exit;
    }

    if (dns_ns_initparse( answer, len, &msg ) < 0)
    {
        ret = DNS_ERROR_BAD_PACKET;
        goto exit;
    }

#define RCODE_MASK 0x0f
    if ((msg._flags & RCODE_MASK) != ns_r_noerror)
    {
        ret = dns_map_error( msg._flags & RCODE_MASK );
        goto exit;
    }

    for (i = 0; i < sizeof(sections)/sizeof(sections[0]); i++)
    {
        for (num = 0; num < ns_msg_count( msg, sections[i] ); num++)
        {
            ret = dns_copy_record( msg, sections[i], num, &record );
            if (ret != ERROR_SUCCESS) goto exit;

            DNS_RRSET_ADD( rrset, (DNS_RECORD *)record );
        }
    }

exit:
    DNS_RRSET_TERMINATE( rrset );

    if (ret != ERROR_SUCCESS)
        DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );
    else
        *result = (DNS_RECORDA *)rrset.pFirstRR;

    return ret;
}

#endif /* HAVE_RESOLV */

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

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_a(name), dns_type_to_str( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    nameW = dns_strdup_aw( name );
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
#ifdef HAVE_RESOLV

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_a(name), dns_type_to_str( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    initialise_resolver();
    _res.options |= dns_map_options( options );

    if (servers && (ret = dns_set_serverlist( servers )))
        return ret;

    ret = dns_do_query( name, type, options, result );

    if (ret == DNS_ERROR_RCODE_NAME_ERROR && type == DNS_TYPE_A &&
        !(options & DNS_QUERY_NO_NETBT))
    {
        TRACE( "dns lookup failed, trying netbios query\n" );
        ret = dns_do_query_netbios( name, result );
    }

#endif
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

    TRACE( "(%s,%s,0x%08x,%p,%p,%p)\n", debugstr_w(name), dns_type_to_str( type ),
           options, servers, result, reserved );

    if (!name || !result)
        return ERROR_INVALID_PARAMETER;

    nameU = dns_strdup_wu( name );
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

static DNS_STATUS dns_get_hostname_a( COMPUTER_NAME_FORMAT format,
                                      PSTR buffer, PDWORD len )
{
    char name[256];
    DWORD size = sizeof(name)/sizeof(name[0]);

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

static DNS_STATUS dns_get_hostname_w( COMPUTER_NAME_FORMAT format,
                                      PWSTR buffer, PDWORD len )
{
    WCHAR name[256];
    DWORD size = sizeof(name)/sizeof(name[0]);

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
    {
#ifdef HAVE_RESOLV
        initialise_resolver();
        ret = dns_get_serverlist( buffer, len );
        break;
#else
        WARN( "compiled without resolver support\n" );
        break;
#endif
    }
    case DnsConfigHostName_A:
    case DnsConfigHostName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_A:
    case DnsConfigFullHostName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_A:
    case DnsConfigPrimaryDomainName_UTF8:
        return dns_get_hostname_a( ComputerNameDnsDomain, buffer, len );

    case DnsConfigHostName_W:
        return dns_get_hostname_w( ComputerNameDnsHostname, buffer, len );

    case DnsConfigFullHostName_W:
        return dns_get_hostname_w( ComputerNameDnsFullyQualified, buffer, len );

    case DnsConfigPrimaryDomainName_W:
        return dns_get_hostname_w( ComputerNameDnsDomain, buffer, len );

    case DnsConfigAdapterDomainName_A:
    case DnsConfigAdapterDomainName_W:
    case DnsConfigAdapterDomainName_UTF8:
    case DnsConfigSearchList:
    case DnsConfigAdapterInfo:
    case DnsConfigPrimaryHostNameRegistrationEnabled:
    case DnsConfigAdapterHostNameRegistrationEnabled:
    case DnsConfigAddressRegistrationMaxCount:
        FIXME( "unimplemented config type %d\n", config );
        break;

    default:
        WARN( "unknown config type: %d\n", config );
        break;
    }
    return ret;
}
