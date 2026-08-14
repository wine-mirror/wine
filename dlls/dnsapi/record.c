/*
 * DNS support
 *
 * Copyright (C) 2006 Hans Leidekker
 * Copyright (C) 2021 Alexandre Julliard
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
#include "winnls.h"
#include "windns.h"

#include "wine/debug.h"
#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

const char *debugstr_type( unsigned short type )
{
    switch (type)
    {
#define X(x)    case (x): return #x;
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
    default: return wine_dbg_sprintf( "0x%04x", type );
    }
}

static const char *debugstr_section( DNS_SECTION section )
{
    switch (section)
    {
    case DnsSectionQuestion:  return "Question";
    case DnsSectionAnswer:    return "Answer";
    case DnsSectionAuthority: return "Authority";
    case DnsSectionAddtional: return "Additional";
    default: return "??";
    }
}

static int strcmpX( LPCVOID str1, LPCVOID str2, BOOL wide )
{
    if (wide)
        return lstrcmpiW( str1, str2 );
    else
        return lstrcmpiA( str1, str2 );
}

static WORD get_word( const BYTE **ptr )
{
    WORD ret = ((*ptr)[0] << 8) + (*ptr)[1];
    *ptr += sizeof(WORD);
    return ret;
}

static DWORD get_dword( const BYTE **ptr )
{
    DWORD ret = ((*ptr)[0] << 24) + ((*ptr)[1] << 16) + ((*ptr)[2] << 8) + (*ptr)[3];
    *ptr += sizeof(DWORD);
    return ret;
}

static const BYTE *skip_name( const BYTE *ptr, const BYTE *end )
{
    BYTE len;

    while (ptr < end && (len = *ptr++))
    {
        switch (len & 0xc0)
        {
        case 0:
            ptr += len;
            continue;
        case 0xc0:
            ptr++;
            break;
        default:
            return NULL;
        }
        break;
    }
    if (ptr < end) return ptr;
    return NULL;
}

static const BYTE *skip_record( const BYTE *ptr, const BYTE *end, DNS_SECTION section )
{
    WORD len;

    if (!(ptr = skip_name( ptr, end ))) return NULL;
    ptr += 2;  /* type */
    ptr += 2;  /* class */
    if (section != DnsSectionQuestion)
    {
        ptr += 4;  /* ttl */
        if (ptr + 2 > end) return NULL;
        len = get_word( &ptr );
        ptr += len;
    }
    if (ptr > end) return NULL;
    return ptr;
}

static const BYTE *get_name( const BYTE *base, const BYTE *end, const BYTE *ptr,
                             char name[DNS_MAX_NAME_BUFFER_LENGTH] )
{
    BYTE len;
    char *out = name;
    const BYTE *next = NULL;
    int loop = 0;

    while (ptr < end && (len = *ptr++))
    {
        switch (len & 0xc0)
        {
        case 0:
            if (out + len + 1 >= name + DNS_MAX_NAME_BUFFER_LENGTH) return NULL;
            if (out > name) *out++ = '.';
            memcpy( out, ptr, len );
            out += len;
            ptr += len;
            continue;
        case 0xc0:
            if (!next) next = ptr + 1;
            if (++loop >= end - base) return NULL;
            if (ptr < end) ptr = base + ((len & 0x3f) << 8) + *ptr;
            break;
        default:
            return NULL;
        }
    }
    if (ptr > end) return NULL;
    if (out == name) *out++ = '.';
    *out = 0;
    return next ? next : ptr;
}

/******************************************************************************
 * DnsRecordCompare                        [DNSAPI.@]
 *
 */
BOOL WINAPI DnsRecordCompare( PDNS_RECORD r1, PDNS_RECORD r2 )
{
    BOOL wide;
    unsigned int i;

    TRACE( "(%p,%p)\n", r1, r2 );

    if (r1->wType            != r2->wType            ||
        r1->wDataLength      != r2->wDataLength      ||
        r1->Flags.S.Section  != r2->Flags.S.Section  ||
        r1->Flags.S.Delete   != r2->Flags.S.Delete   ||
        r1->Flags.S.Unused   != r2->Flags.S.Unused   ||
        r1->Flags.S.Reserved != r2->Flags.S.Reserved ||
        r1->dwReserved       != r2->dwReserved) return FALSE;

    wide = (r1->Flags.S.CharSet == DnsCharSetUnicode || r1->Flags.S.CharSet == DnsCharSetUnknown);
    if (strcmpX( r1->pName, r2->pName, wide )) return FALSE;

    switch (r1->wType)
    {
    case DNS_TYPE_A:
    {
        if (r1->Data.A.IpAddress != r2->Data.A.IpAddress) return FALSE;
        break;
    }
    case DNS_TYPE_SOA:
    {
        if (r1->Data.SOA.dwSerialNo   != r2->Data.SOA.dwSerialNo ||
            r1->Data.SOA.dwRefresh    != r2->Data.SOA.dwRefresh  ||
            r1->Data.SOA.dwRetry      != r2->Data.SOA.dwRetry    ||
            r1->Data.SOA.dwExpire     != r2->Data.SOA.dwExpire   ||
            r1->Data.SOA.dwDefaultTtl != r2->Data.SOA.dwDefaultTtl)
            return FALSE;
        if (strcmpX( r1->Data.SOA.pNamePrimaryServer, r2->Data.SOA.pNamePrimaryServer, wide ) ||
            strcmpX( r1->Data.SOA.pNameAdministrator, r2->Data.SOA.pNameAdministrator, wide ))
            return FALSE;
        break;
    }
    case DNS_TYPE_PTR:
    case DNS_TYPE_NS:
    case DNS_TYPE_CNAME:
    case DNS_TYPE_MB:
    case DNS_TYPE_MD:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:
    {
        if (strcmpX( r1->Data.PTR.pNameHost, r2->Data.PTR.pNameHost, wide )) return FALSE;
        break;
    }
    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:
    {
        if (strcmpX( r1->Data.MINFO.pNameMailbox, r2->Data.MINFO.pNameMailbox, wide ) ||
            strcmpX( r1->Data.MINFO.pNameErrorsMailbox, r2->Data.MINFO.pNameErrorsMailbox, wide ))
            return FALSE;
        break;
    }
    case DNS_TYPE_MX:
    case DNS_TYPE_AFSDB:
    case DNS_TYPE_RT:
    {
        if (r1->Data.MX.wPreference != r2->Data.MX.wPreference)
            return FALSE;
        if (strcmpX( r1->Data.MX.pNameExchange, r2->Data.MX.pNameExchange, wide ))
            return FALSE;
        break;
    }
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_TEXT:
    case DNS_TYPE_X25:
    {
        if (r1->Data.TXT.dwStringCount != r2->Data.TXT.dwStringCount)
            return FALSE;
        for (i = 0; i < r1->Data.TXT.dwStringCount; i++)
        {
            if (strcmpX( r1->Data.TXT.pStringArray[i], r2->Data.TXT.pStringArray[i], wide ))
                return FALSE;
        }
        break;
    }
    case DNS_TYPE_NULL:
    {
        if (r1->Data.Null.dwByteCount != r2->Data.Null.dwByteCount)
            return FALSE;
        if (memcmp( r1->Data.Null.Data,
                    r2->Data.Null.Data, r1->Data.Null.dwByteCount ))
            return FALSE;
        break;
    }
    case DNS_TYPE_OPT:
    {
        if (r1->Data.Opt.wDataLength != r2->Data.Opt.wDataLength)
            return FALSE;
        /* ignore wPad */
        if (memcmp( r1->Data.Opt.Data,
                    r2->Data.Opt.Data, r1->Data.Opt.wDataLength ))
            return FALSE;
        break;
    }
    case DNS_TYPE_AAAA:
    {
        for (i = 0; i < sizeof(IP6_ADDRESS)/sizeof(DWORD); i++)
        {
            if (r1->Data.AAAA.Ip6Address.IP6Dword[i] !=
                r2->Data.AAAA.Ip6Address.IP6Dword[i]) return FALSE;
        }
        break;
    }
    case DNS_TYPE_KEY:
    {
        if (r1->Data.KEY.wFlags      != r2->Data.KEY.wFlags      ||
            r1->Data.KEY.chProtocol  != r2->Data.KEY.chProtocol  ||
            r1->Data.KEY.chAlgorithm != r2->Data.KEY.chAlgorithm ||
            r1->Data.KEY.wKeyLength  != r2->Data.KEY.wKeyLength)
            return FALSE;
        if (memcmp( r1->Data.KEY.Key, r2->Data.KEY.Key, r1->Data.KEY.wKeyLength ))
            return FALSE;
        break;
    }
    case DNS_TYPE_SIG:
    {
        if (strcmpX( r1->Data.SIG.pNameSigner, r2->Data.SIG.pNameSigner, wide ))
            return FALSE;
        if (r1->Data.SIG.wTypeCovered  != r2->Data.SIG.wTypeCovered  ||
            r1->Data.SIG.chAlgorithm   != r2->Data.SIG.chAlgorithm   ||
            r1->Data.SIG.chLabelCount  != r2->Data.SIG.chLabelCount  ||
            r1->Data.SIG.dwOriginalTtl != r2->Data.SIG.dwOriginalTtl ||
            r1->Data.SIG.dwExpiration  != r2->Data.SIG.dwExpiration  ||
            r1->Data.SIG.dwTimeSigned  != r2->Data.SIG.dwTimeSigned  ||
            r1->Data.SIG.wKeyTag       != r2->Data.SIG.wKeyTag       ||
            r1->Data.SIG.wSignatureLength != r2->Data.SIG.wSignatureLength)
            return FALSE;
        if (memcmp( r1->Data.SIG.Signature, r2->Data.SIG.Signature, r1->Data.SIG.wSignatureLength ))
            return FALSE;
        break;
    }
    case DNS_TYPE_ATMA:
    {
        if (r1->Data.ATMA.AddressType != r2->Data.ATMA.AddressType)
            return FALSE;
        for (i = 0; i < DNS_ATMA_MAX_ADDR_LENGTH; i++)
        {
            if (r1->Data.ATMA.Address[i] != r2->Data.ATMA.Address[i])
                return FALSE;
        }
        break;
    }
    case DNS_TYPE_NXT:
    {
        if (strcmpX( r1->Data.NXT.pNameNext, r2->Data.NXT.pNameNext, wide )) return FALSE;
        if (r1->Data.NXT.wNumTypes != r2->Data.NXT.wNumTypes) return FALSE;
        if (memcmp( r1->Data.NXT.wTypes, r2->Data.NXT.wTypes,
                    r1->wDataLength - sizeof(DNS_NXT_DATAA) + sizeof(WORD) ))
            return FALSE;
        break;
    }
    case DNS_TYPE_SRV:
    {
        if (strcmpX( r1->Data.SRV.pNameTarget, r2->Data.SRV.pNameTarget, wide )) return FALSE;
        if (r1->Data.SRV.wPriority != r2->Data.SRV.wPriority ||
            r1->Data.SRV.wWeight   != r2->Data.SRV.wWeight   ||
            r1->Data.SRV.wPort     != r2->Data.SRV.wPort)
            return FALSE;
        break;
    }
    case DNS_TYPE_TKEY:
    {
        if (strcmpX( r1->Data.TKEY.pNameAlgorithm, r2->Data.TKEY.pNameAlgorithm, wide ))
            return FALSE;
        if (r1->Data.TKEY.dwCreateTime    != r2->Data.TKEY.dwCreateTime     ||
            r1->Data.TKEY.dwExpireTime    != r2->Data.TKEY.dwExpireTime     ||
            r1->Data.TKEY.wMode           != r2->Data.TKEY.wMode            ||
            r1->Data.TKEY.wError          != r2->Data.TKEY.wError           ||
            r1->Data.TKEY.wKeyLength      != r2->Data.TKEY.wKeyLength       ||
            r1->Data.TKEY.wOtherLength    != r2->Data.TKEY.wOtherLength     ||
            r1->Data.TKEY.cAlgNameLength  != r2->Data.TKEY.cAlgNameLength   ||
            r1->Data.TKEY.bPacketPointers != r2->Data.TKEY.bPacketPointers)
            return FALSE;

        /* FIXME: ignoring pAlgorithmPacket field */
        if (memcmp( r1->Data.TKEY.pKey, r2->Data.TKEY.pKey,
                    r1->Data.TKEY.wKeyLength ) ||
            memcmp( r1->Data.TKEY.pOtherData, r2->Data.TKEY.pOtherData,
                    r1->Data.TKEY.wOtherLength )) return FALSE;
        break;
    }
    case DNS_TYPE_TSIG:
    {
        if (strcmpX( r1->Data.TSIG.pNameAlgorithm, r2->Data.TSIG.pNameAlgorithm, wide ))
            return FALSE;
        if (r1->Data.TSIG.i64CreateTime   != r2->Data.TSIG.i64CreateTime    ||
            r1->Data.TSIG.wFudgeTime      != r2->Data.TSIG.wFudgeTime       ||
            r1->Data.TSIG.wOriginalXid    != r2->Data.TSIG.wOriginalXid     ||
            r1->Data.TSIG.wError          != r2->Data.TSIG.wError           ||
            r1->Data.TSIG.wSigLength      != r2->Data.TSIG.wSigLength       ||
            r1->Data.TSIG.wOtherLength    != r2->Data.TSIG.wOtherLength     ||
            r1->Data.TSIG.cAlgNameLength  != r2->Data.TSIG.cAlgNameLength   ||
            r1->Data.TSIG.bPacketPointers != r2->Data.TSIG.bPacketPointers)
            return FALSE;

        /* FIXME: ignoring pAlgorithmPacket field */
        if (memcmp( r1->Data.TSIG.pSignature, r2->Data.TSIG.pSignature,
                    r1->Data.TSIG.wSigLength ) ||
            memcmp( r1->Data.TSIG.pOtherData, r2->Data.TSIG.pOtherData,
                    r1->Data.TSIG.wOtherLength )) return FALSE;
        break;
    }
    case DNS_TYPE_WINS:
    {
        if (r1->Data.WINS.dwMappingFlag    != r2->Data.WINS.dwMappingFlag   ||
            r1->Data.WINS.dwLookupTimeout  != r2->Data.WINS.dwLookupTimeout ||
            r1->Data.WINS.dwCacheTimeout   != r2->Data.WINS.dwCacheTimeout  ||
            r1->Data.WINS.cWinsServerCount != r2->Data.WINS.cWinsServerCount)
            return FALSE;
        if (memcmp( r1->Data.WINS.WinsServers, r2->Data.WINS.WinsServers,
                    r1->wDataLength - sizeof(DNS_WINS_DATA) + sizeof(IP4_ADDRESS) ))
            return FALSE;
        break;
    }
    case DNS_TYPE_WINSR:
    {
        if (r1->Data.WINSR.dwMappingFlag   != r2->Data.WINSR.dwMappingFlag   ||
            r1->Data.WINSR.dwLookupTimeout != r2->Data.WINSR.dwLookupTimeout ||
            r1->Data.WINSR.dwCacheTimeout  != r2->Data.WINSR.dwCacheTimeout)
            return FALSE;
        if (strcmpX( r1->Data.WINSR.pNameResultDomain, r2->Data.WINSR.pNameResultDomain, wide ))
            return FALSE;
        break;
    }
    default:
        FIXME( "unknown type: %s\n", debugstr_type( r1->wType ) );
        return FALSE;
    }
    return TRUE;
}

static LPVOID strdupX( LPCVOID src, DNS_CHARSET in, DNS_CHARSET out )
{
    switch (in)
    {
    case DnsCharSetUnicode:
    {
        switch (out)
        {
        case DnsCharSetUnicode: return wcsdup( src );
        case DnsCharSetUtf8:    return strdup_wu( src );
        case DnsCharSetAnsi:    return strdup_wa( src );
        default:
            WARN( "unhandled target charset: %d\n", out );
            break;
        }
        break;
    }
    case DnsCharSetUtf8:
        switch (out)
        {
        case DnsCharSetUnicode: return strdup_uw( src );
        case DnsCharSetUtf8:    return strdup( src );
        case DnsCharSetAnsi:    return strdup_ua( src );
        default:
            WARN( "unhandled target charset: %d\n", out );
            break;
        }
        break;
    case DnsCharSetAnsi:
        switch (out)
        {
        case DnsCharSetUnicode: return strdup_aw( src );
        case DnsCharSetUtf8:    return strdup_au( src );
        case DnsCharSetAnsi:    return strdup( src );
        default:
            WARN( "unhandled target charset: %d\n", out );
            break;
        }
        break;
    default:
        WARN( "unhandled source charset: %d\n", in );
        break;
    }
    return NULL;
}

/******************************************************************************
 * DnsRecordCopyEx                         [DNSAPI.@]
 *
 */
PDNS_RECORD WINAPI DnsRecordCopyEx( PDNS_RECORD src, DNS_CHARSET in, DNS_CHARSET out )
{
    DNS_RECORD *dst;
    unsigned int i, size;

    TRACE( "(%p,%d,%d)\n", src, in, out );

    size = FIELD_OFFSET(DNS_RECORD, Data) + src->wDataLength;
    dst = malloc( size );
    if (!dst) return NULL;

    memcpy( dst, src, size );

    if (src->Flags.S.CharSet == DnsCharSetUtf8 ||
        src->Flags.S.CharSet == DnsCharSetAnsi ||
        src->Flags.S.CharSet == DnsCharSetUnicode) in = src->Flags.S.CharSet;

    dst->Flags.S.CharSet = out;
    dst->pName = strdupX( src->pName, in, out );
    if (!dst->pName) goto error;

    switch (src->wType)
    {
    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_TEXT:
    case DNS_TYPE_X25:
    {
        for (i = 0; i < src->Data.TXT.dwStringCount; i++)
        {
            dst->Data.TXT.pStringArray[i] = strdupX( src->Data.TXT.pStringArray[i], in, out );
            if (!dst->Data.TXT.pStringArray[i])
            {
                while (i > 0) free( dst->Data.TXT.pStringArray[--i] );
                goto error;
            }
        }
        break;
    }
    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:
    {
        dst->Data.MINFO.pNameMailbox = strdupX( src->Data.MINFO.pNameMailbox, in, out );
        if (!dst->Data.MINFO.pNameMailbox) goto error;

        dst->Data.MINFO.pNameErrorsMailbox = strdupX( src->Data.MINFO.pNameErrorsMailbox, in, out );
        if (!dst->Data.MINFO.pNameErrorsMailbox)
        {
            free( dst->Data.MINFO.pNameMailbox );
            goto error;
        }

        dst->wDataLength = sizeof(dst->Data.MINFO);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.MINFO.pNameMailbox ) + 1) * sizeof(WCHAR) +
            (wcslen( dst->Data.MINFO.pNameErrorsMailbox ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_AFSDB:
    case DNS_TYPE_RT:
    case DNS_TYPE_MX:
    {
        dst->Data.MX.pNameExchange = strdupX( src->Data.MX.pNameExchange, in, out );
        if (!dst->Data.MX.pNameExchange) goto error;

        dst->wDataLength = sizeof(dst->Data.MX);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.MX.pNameExchange ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_NXT:
    {
        dst->Data.NXT.pNameNext = strdupX( src->Data.NXT.pNameNext, in, out );
        if (!dst->Data.NXT.pNameNext) goto error;

        dst->wDataLength = sizeof(dst->Data.NXT);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.NXT.pNameNext ) + 1) * sizeof(WCHAR);
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
        dst->Data.PTR.pNameHost = strdupX( src->Data.PTR.pNameHost, in, out );
        if (!dst->Data.PTR.pNameHost) goto error;

        dst->wDataLength = sizeof(dst->Data.PTR);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.PTR.pNameHost ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_SIG:
    {
        dst->Data.SIG.pNameSigner = strdupX( src->Data.SIG.pNameSigner, in, out );
        if (!dst->Data.SIG.pNameSigner) goto error;

        dst->wDataLength = sizeof(dst->Data.SIG);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.SIG.pNameSigner ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_SOA:
    {
        dst->Data.SOA.pNamePrimaryServer = strdupX( src->Data.SOA.pNamePrimaryServer, in, out );
        if (!dst->Data.SOA.pNamePrimaryServer) goto error;

        dst->Data.SOA.pNameAdministrator = strdupX( src->Data.SOA.pNameAdministrator, in, out );
        if (!dst->Data.SOA.pNameAdministrator)
        {
            free( dst->Data.SOA.pNamePrimaryServer );
            goto error;
        }

        dst->wDataLength = sizeof(dst->Data.SOA);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.SOA.pNamePrimaryServer ) + 1) * sizeof(WCHAR) +
            (wcslen( dst->Data.SOA.pNameAdministrator ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_SRV:
    {
        dst->Data.SRV.pNameTarget = strdupX( src->Data.SRV.pNameTarget, in, out );
        if (!dst->Data.SRV.pNameTarget) goto error;

        dst->wDataLength = sizeof(dst->Data.SRV);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.SRV.pNameTarget ) + 1) * sizeof(WCHAR);
        break;
    }
    default:
        break;
    }
    return dst;

error:
    free( dst->pName );
    free( dst );
    return NULL;
}

/******************************************************************************
 * DnsRecordListFree                       [DNSAPI.@]
 *
 */
VOID WINAPI DnsRecordListFree( PDNS_RECORD list, DNS_FREE_TYPE type )
{
    DNS_RECORD *r, *next;
    unsigned int i;

    TRACE( "(%p,%d)\n", list, type );

    if (!list) return;

    switch (type)
    {
    case DnsFreeRecordList:
    {
        for (r = list; (list = r); r = next)
        {
            free( r->pName );

            switch (r->wType)
            {
            case DNS_TYPE_HINFO:
            case DNS_TYPE_ISDN:
            case DNS_TYPE_TEXT:
            case DNS_TYPE_X25:
                for (i = 0; i < r->Data.TXT.dwStringCount; i++)
                    free( r->Data.TXT.pStringArray[i] );

                break;

            case DNS_TYPE_MINFO:
            case DNS_TYPE_RP:
                free( r->Data.MINFO.pNameMailbox );
                free( r->Data.MINFO.pNameErrorsMailbox );
                break;

            case DNS_TYPE_AFSDB:
            case DNS_TYPE_RT:
            case DNS_TYPE_MX:
                free( r->Data.MX.pNameExchange );
                break;

            case DNS_TYPE_NXT:
                free( r->Data.NXT.pNameNext );
                break;

            case DNS_TYPE_CNAME:
            case DNS_TYPE_MB:
            case DNS_TYPE_MD:
            case DNS_TYPE_MF:
            case DNS_TYPE_MG:
            case DNS_TYPE_MR:
            case DNS_TYPE_NS:
            case DNS_TYPE_PTR:
                free( r->Data.PTR.pNameHost );
                break;

            case DNS_TYPE_SIG:
                free( r->Data.SIG.pNameSigner );
                break;

            case DNS_TYPE_SOA:
                free( r->Data.SOA.pNamePrimaryServer );
                free( r->Data.SOA.pNameAdministrator );
                break;

            case DNS_TYPE_SRV:
                free( r->Data.SRV.pNameTarget );
                break;
            }

            next = r->pNext;
            free( r );
        }
        break;
    }
    case DnsFreeFlat:
    case DnsFreeParsedMessageFields:
    {
        FIXME( "unhandled free type: %d\n", type );
        break;
    }
    default:
        WARN( "unknown free type: %d\n", type );
        break;
    }
}

/******************************************************************************
 * DnsFree                     [DNSAPI.@]
 *
 */
void WINAPI DnsFree( PVOID data, DNS_FREE_TYPE type )
{
    DnsRecordListFree( data, type );
}

/******************************************************************************
 * DnsRecordSetCompare                     [DNSAPI.@]
 *
 */
BOOL WINAPI DnsRecordSetCompare( PDNS_RECORD set1, PDNS_RECORD set2,
                                 PDNS_RECORD *diff1, PDNS_RECORD *diff2 )
{
    BOOL ret = TRUE;
    DNS_RECORD *r, *t, *u;
    DNS_RRSET rr1, rr2;

    TRACE( "(%p,%p,%p,%p)\n", set1, set2, diff1, diff2 );

    if (!set1 && !set2) return FALSE;

    if (diff1) *diff1 = NULL;
    if (diff2) *diff2 = NULL;

    if (set1 && !set2)
    {
        if (diff1) *diff1 = DnsRecordSetCopyEx( set1, 0, set1->Flags.S.CharSet );
        return FALSE;
    }
    if (!set1 && set2)
    {
        if (diff2) *diff2 = DnsRecordSetCopyEx( set2, 0, set2->Flags.S.CharSet );
        return FALSE;
    }

    DNS_RRSET_INIT( rr1 );
    DNS_RRSET_INIT( rr2 );

    for (r = set1; r; r = r->pNext)
    {
        for (t = set2; t; t = t->pNext)
        {
            u = DnsRecordCopyEx( r, r->Flags.S.CharSet, t->Flags.S.CharSet );
            if (!u) goto error;

            if (!DnsRecordCompare( t, u ))
            {
                DNS_RRSET_ADD( rr1, u );
                ret = FALSE;
            }
            else DnsRecordListFree( u, DnsFreeRecordList );
        }
    }

    for (t = set2; t; t = t->pNext)
    {
        for (r = set1; r; r = r->pNext)
        {
            u = DnsRecordCopyEx( t, t->Flags.S.CharSet, r->Flags.S.CharSet );
            if (!u) goto error;

            if (!DnsRecordCompare( r, u ))
            {
                DNS_RRSET_ADD( rr2, u );
                ret = FALSE;
            }
            else DnsRecordListFree( u, DnsFreeRecordList );
        }
    }

    DNS_RRSET_TERMINATE( rr1 );
    DNS_RRSET_TERMINATE( rr2 );

    if (diff1) *diff1 = rr1.pFirstRR;
    else DnsRecordListFree( rr1.pFirstRR, DnsFreeRecordList );

    if (diff2) *diff2 = rr2.pFirstRR;
    else DnsRecordListFree( rr2.pFirstRR, DnsFreeRecordList );

    return ret;

error:
    DNS_RRSET_TERMINATE( rr1 );
    DNS_RRSET_TERMINATE( rr2 );

    DnsRecordListFree( rr1.pFirstRR, DnsFreeRecordList );
    DnsRecordListFree( rr2.pFirstRR, DnsFreeRecordList );

    return FALSE;
}

/******************************************************************************
 * DnsRecordSetCopyEx                      [DNSAPI.@]
 *
 */
PDNS_RECORD WINAPI DnsRecordSetCopyEx( PDNS_RECORD src_set, DNS_CHARSET in, DNS_CHARSET out )
{
    DNS_RRSET dst_set;
    DNS_RECORD *src, *dst;

    TRACE( "(%p,%d,%d)\n", src_set, in, out );

    DNS_RRSET_INIT( dst_set );

    for (src = src_set; (src_set = src); src = src_set->pNext)
    {
        dst = DnsRecordCopyEx( src, in, out );
        if (!dst)
        {
            DNS_RRSET_TERMINATE( dst_set );
            DnsRecordListFree( dst_set.pFirstRR, DnsFreeRecordList );
            return NULL;
        }
        DNS_RRSET_ADD( dst_set, dst );
    }

    DNS_RRSET_TERMINATE( dst_set );
    return dst_set.pFirstRR;
}

/******************************************************************************
 * DnsRecordSetDetach                      [DNSAPI.@]
 *
 */
PDNS_RECORD WINAPI DnsRecordSetDetach( PDNS_RECORD set )
{
    DNS_RECORD *r, *s;

    TRACE( "(%p)\n", set );

    for (r = set; (set = r); r = set->pNext)
    {
        if (r->pNext && !r->pNext->pNext)
        {
            s = r->pNext;
            r->pNext = NULL;
            return s;
        }
    }
    return NULL;
}


static unsigned int get_record_size( WORD type, const BYTE *data, WORD len )
{
    switch (type)
    {
    case DNS_TYPE_KEY:
        return offsetof( DNS_RECORDA, Data.Key.Key[len] );

    case DNS_TYPE_SIG:
        return offsetof( DNS_RECORDA, Data.Sig.Signature[len] );

    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_TEXT:
    case DNS_TYPE_X25:
    {
        int i;
        const BYTE *pos = data;
        for (i = 0; pos < data + len; i++) pos += pos[0] + 1;
        return offsetof( DNS_RECORDA, Data.Txt.pStringArray[i] );
    }

    case DNS_TYPE_NULL:
    case DNS_TYPE_OPT:
        return offsetof( DNS_RECORDA, Data.Null.Data[len] );

    case DNS_TYPE_WKS:
        return offsetof( DNS_RECORDA, Data.Wks.BitMask[len / 8] );

    case DNS_TYPE_NXT:
        return offsetof( DNS_RECORDA, Data.Nxt.wTypes[len * 8] );

    case DNS_TYPE_WINS:
        return offsetof( DNS_RECORDA, Data.Wins.WinsServers[len / 4] );

    default:
        return sizeof(DNS_RECORDA);
    }
}

static DNS_STATUS extract_rdata( const BYTE *base, const BYTE *end, const BYTE *pos, WORD len, WORD type,
                                 DNS_RECORDA *r )
{
    DNS_CHARSET in = DnsCharSetUtf8, out = r->Flags.S.CharSet;
    char name[DNS_MAX_NAME_BUFFER_LENGTH];
    DNS_STATUS ret = ERROR_SUCCESS;
    const BYTE *rrend = pos + len;
    unsigned int i;

    switch (type)
    {
    case DNS_TYPE_A:
        if (pos + sizeof(DWORD) > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.A.IpAddress = *(const DWORD *)pos;
        r->wDataLength = sizeof(DNS_A_DATA);
        break;

    case DNS_TYPE_AAAA:
        if (pos + sizeof(IP6_ADDRESS) > rrend) return DNS_ERROR_BAD_PACKET;
        for (i = 0; i < sizeof(IP6_ADDRESS)/sizeof(DWORD); i++)
        {
            r->Data.AAAA.Ip6Address.IP6Dword[i] = *(const DWORD *)pos;
            pos += sizeof(DWORD);
        }
        r->wDataLength = sizeof(DNS_AAAA_DATA);
        break;

    case DNS_TYPE_KEY:
        if (pos + 4 > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.KEY.wFlags      = get_word( &pos );
        r->Data.KEY.chProtocol  = *pos++;
        r->Data.KEY.chAlgorithm = *pos++;
        r->Data.KEY.wKeyLength  = rrend - pos;
        memcpy( r->Data.KEY.Key, pos, r->Data.KEY.wKeyLength );
        r->wDataLength = offsetof( DNS_KEY_DATA, Key[r->Data.KEY.wKeyLength] );
        break;

    case DNS_TYPE_RP:
    case DNS_TYPE_MINFO:
        if (!(pos = get_name( base, end, pos, name ))) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.MINFO.pNameMailbox = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        if (!get_name( base, end, pos, name )) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.MINFO.pNameErrorsMailbox = strdupX( name, in, out )))
        {
            free( r->Data.MINFO.pNameMailbox );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        r->wDataLength = sizeof(DNS_MINFO_DATAA);
        break;

    case DNS_TYPE_AFSDB:
    case DNS_TYPE_RT:
    case DNS_TYPE_MX:
        if (pos + sizeof(WORD) > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.MX.wPreference = get_word( &pos );
        if (!get_name( base, end, pos, name )) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.MX.pNameExchange = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        r->wDataLength = sizeof(DNS_MX_DATAA) + sizeof(DWORD);
        break;

    case DNS_TYPE_NULL:
        r->Data.Null.dwByteCount = len;
        memcpy( r->Data.Null.Data, pos, len );
        r->wDataLength = offsetof( DNS_NULL_DATA, Data[len] );
        break;

    case DNS_TYPE_OPT:
        r->Data.OPT.wDataLength = len;
        r->Data.OPT.wPad        = 0;
        memcpy( r->Data.OPT.Data, pos, len );
        r->wDataLength = offsetof( DNS_OPT_DATA, Data[len] );
        break;

    case DNS_TYPE_CNAME:
    case DNS_TYPE_NS:
    case DNS_TYPE_MB:
    case DNS_TYPE_MD:
    case DNS_TYPE_MF:
    case DNS_TYPE_MG:
    case DNS_TYPE_MR:
    case DNS_TYPE_PTR:
        if (!get_name( base, end, pos, name )) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.PTR.pNameHost = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        r->wDataLength = sizeof(DNS_PTR_DATAA) + sizeof(DWORD);
        break;

    case DNS_TYPE_SIG:
        if (pos + 18 > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.SIG.wTypeCovered  = get_word( &pos );
        r->Data.SIG.chAlgorithm   = *pos++;
        r->Data.SIG.chLabelCount  = *pos++;
        r->Data.SIG.dwOriginalTtl = get_dword( &pos );
        r->Data.SIG.dwExpiration  = get_dword( &pos );
        r->Data.SIG.dwTimeSigned  = get_dword( &pos );
        r->Data.SIG.wKeyTag       = get_word( &pos );
        if (!(pos = get_name( base, end, pos, name ))) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.SIG.pNameSigner = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        r->Data.SIG.wSignatureLength = rrend - pos;
        memcpy( r->Data.SIG.Signature, pos, r->Data.SIG.wSignatureLength );
        r->wDataLength = offsetof( DNS_SIG_DATAA, Signature[r->Data.SIG.wSignatureLength] );
        break;

    case DNS_TYPE_SOA:
        if (!(pos = get_name( base, end, pos, name ))) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.SOA.pNamePrimaryServer = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        if (!(pos = get_name( base, end, pos, name ))) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.SOA.pNameAdministrator = strdupX( name, in, out )))
        {
            free( r->Data.SOA.pNamePrimaryServer );
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        if (pos + 5 * sizeof(DWORD) > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.SOA.dwSerialNo   = get_dword( &pos );
        r->Data.SOA.dwRefresh    = get_dword( &pos );
        r->Data.SOA.dwRetry      = get_dword( &pos );
        r->Data.SOA.dwExpire     = get_dword( &pos );
        r->Data.SOA.dwDefaultTtl = get_dword( &pos );
        r->wDataLength = sizeof(DNS_SOA_DATAA);
        break;

    case DNS_TYPE_SRV:
        if (pos + 3 * sizeof(WORD) > rrend) return DNS_ERROR_BAD_PACKET;
        r->Data.SRV.wPriority = get_word( &pos );
        r->Data.SRV.wWeight   = get_word( &pos );
        r->Data.SRV.wPort     = get_word( &pos );
        if (!get_name( base, end, pos, name )) return DNS_ERROR_BAD_PACKET;
        if (!(r->Data.SRV.pNameTarget = strdupX( name, in, out ))) return ERROR_NOT_ENOUGH_MEMORY;
        r->wDataLength = sizeof(DNS_SRV_DATAA);
        if (out == DnsCharSetUnicode)
            r->wDataLength += (wcslen( (const WCHAR *)r->Data.SRV.pNameTarget ) + 1) * sizeof(WCHAR);
        else
            r->wDataLength += strlen( r->Data.SRV.pNameTarget ) + 1;
        break;

    case DNS_TYPE_HINFO:
    case DNS_TYPE_ISDN:
    case DNS_TYPE_X25:
    case DNS_TYPE_TEXT:
        for (i = 0; pos < rrend; i++)
        {
            BYTE len = pos[0];
            if (pos + len + 1 > rrend) return DNS_ERROR_BAD_PACKET;
            memcpy( name, pos + 1, len );
            name[len] = 0;
            if (!(r->Data.TXT.pStringArray[i] = strdupX( name, in, out )))
            {
                while (i > 0) free( r->Data.TXT.pStringArray[--i] );
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            pos += len + 1;
        }
        r->Data.TXT.dwStringCount = i;
        r->wDataLength = offsetof( DNS_TXT_DATAA, pStringArray[r->Data.TXT.dwStringCount] );
        break;

    case DNS_TYPE_ATMA:
    case DNS_TYPE_LOC:
    case DNS_TYPE_NXT:
    case DNS_TYPE_TSIG:
    case DNS_TYPE_WKS:
    case DNS_TYPE_TKEY:
    case DNS_TYPE_WINS:
    case DNS_TYPE_WINSR:
    default:
        FIXME( "unhandled type: %s\n", debugstr_type( type ));
        return DNS_ERROR_RCODE_NOT_IMPLEMENTED;
    }

    return ret;
}

static DNS_STATUS extract_record( const DNS_MESSAGE_BUFFER *hdr, const BYTE *end, const BYTE **pos,
                                  DNS_SECTION section, DNS_CHARSET charset, DNS_RECORDA **recp )
{
    DNS_STATUS ret;
    DNS_RECORDA *record;
    char name[DNS_MAX_NAME_BUFFER_LENGTH];
    WORD type, rdlen;
    DWORD ttl;
    const BYTE *base = (const BYTE *)hdr;
    const BYTE *ptr = *pos;

    if (!(ptr = get_name( base, end, ptr, name ))) return DNS_ERROR_BAD_PACKET;

    if (ptr + 10 > end) return DNS_ERROR_BAD_PACKET;
    type = get_word( &ptr );
    ptr += sizeof(WORD); /* class */
    ttl = get_dword( &ptr );
    rdlen = get_word( &ptr );
    if (ptr + rdlen > end) return DNS_ERROR_BAD_PACKET;
    *pos = ptr + rdlen;

    if (!(record = calloc( 1, get_record_size( type, ptr, rdlen ) ))) return ERROR_NOT_ENOUGH_MEMORY;

    record->wType = type;
    record->Flags.S.Section = section;
    record->Flags.S.CharSet = charset;
    record->dwTtl = ttl;

    if (!(record->pName = strdupX( name, DnsCharSetUtf8, charset )))
    {
        free( record );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    if ((ret = extract_rdata( base, end, ptr, rdlen, type, record )))
    {
        free( record->pName );
        free( record );
        return ret;
    }
    *recp = record;
    TRACE( "found %s record in %s section\n", debugstr_type( record->wType ), debugstr_section( section ) );
    return ERROR_SUCCESS;
}

static DNS_STATUS extract_message_records( const DNS_MESSAGE_BUFFER *buffer, WORD len, DNS_CHARSET charset,
                                           DNS_RRSET *rrset )
{
    DNS_STATUS ret = ERROR_SUCCESS;
    const DNS_HEADER *hdr = &buffer->MessageHead;
    DNS_RECORDA *record;
    const BYTE *base = (const BYTE *)hdr;
    const BYTE *end = base + len;
    const BYTE *ptr = (const BYTE *)buffer->MessageBody;
    unsigned int num;

    if (hdr->IsResponse && !hdr->AnswerCount) return DNS_ERROR_BAD_PACKET;

    for (num = 0; num < hdr->QuestionCount; num++)
        if (!(ptr = skip_record( ptr, end, DnsSectionQuestion ))) return DNS_ERROR_BAD_PACKET;

    for (num = 0; num < hdr->AnswerCount; num++)
    {
        if ((ret = extract_record( buffer, end, &ptr, DnsSectionAnswer, charset, &record )))
            return ret;
        DNS_RRSET_ADD( *rrset, (DNS_RECORD *)record );
    }

    for (num = 0; num < hdr->NameServerCount; num++)
        if (!(ptr = skip_record( ptr, end, DnsSectionAuthority ))) return DNS_ERROR_BAD_PACKET;

    for (num = 0; num < hdr->AdditionalCount; num++)
    {
        if ((ret = extract_record( buffer, end, &ptr, DnsSectionAddtional, charset, &record ))) return ret;
        DNS_RRSET_ADD( *rrset, (DNS_RECORD *)record );
    }

    if (ptr != end) ret = DNS_ERROR_BAD_PACKET;
    return ret;
}

/******************************************************************************
 * DnsExtractRecordsFromMessage_UTF8       [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsExtractRecordsFromMessage_UTF8( DNS_MESSAGE_BUFFER *buffer, WORD len,
                                                     DNS_RECORDA **result )
{
    DNS_STATUS ret = DNS_ERROR_BAD_PACKET;
    DNS_RRSET rrset;

    if (len < sizeof(*buffer)) return ERROR_INVALID_PARAMETER;

    DNS_RRSET_INIT( rrset );
    ret = extract_message_records( buffer, len, DnsCharSetUtf8, &rrset );
    DNS_RRSET_TERMINATE( rrset );

    if (!ret) *result = (DNS_RECORDA *)rrset.pFirstRR;
    else DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );

    return ret;
}

/******************************************************************************
 * DnsExtractRecordsFromMessage_W          [DNSAPI.@]
 *
 */
DNS_STATUS WINAPI DnsExtractRecordsFromMessage_W( DNS_MESSAGE_BUFFER *buffer, WORD len,
                                                  DNS_RECORDW **result )
{
    DNS_STATUS ret = DNS_ERROR_BAD_PACKET;
    DNS_RRSET rrset;

    if (len < sizeof(*buffer)) return ERROR_INVALID_PARAMETER;

    DNS_RRSET_INIT( rrset );
    ret = extract_message_records( buffer, len, DnsCharSetUnicode, &rrset );
    DNS_RRSET_TERMINATE( rrset );

    if (!ret) *result = (DNS_RECORDW *)rrset.pFirstRR;
    else DnsRecordListFree( rrset.pFirstRR, DnsFreeRecordList );

    return ret;
}
