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
#include "winnls.h"
#include "windns.h"

#include "wine/debug.h"
#include "dnsapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(dnsapi);

const char *type_to_str( unsigned short type )
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
    default: { static char tmp[7]; sprintf( tmp, "0x%04x", type ); return tmp; }
    }
}

static int strcmpX( LPCVOID str1, LPCVOID str2, BOOL wide )
{
    if (wide)
        return lstrcmpiW( str1, str2 );
    else
        return lstrcmpiA( str1, str2 );
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
            r1->Data.KEY.chAlgorithm != r2->Data.KEY.chAlgorithm)
            return FALSE;
        if (memcmp( r1->Data.KEY.Key, r2->Data.KEY.Key,
                    r1->wDataLength - sizeof(DNS_KEY_DATA) + 1 ))
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
            r1->Data.SIG.wKeyTag       != r2->Data.SIG.wKeyTag)
            return FALSE;
        if (memcmp( r1->Data.SIG.Signature, r2->Data.SIG.Signature,
                    r1->wDataLength - sizeof(DNS_SIG_DATAA) + 1 ))
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
        FIXME( "unknown type: %s\n", type_to_str( r1->wType ) );
        return FALSE;
    }
    return TRUE;
}

static LPVOID strcpyX( LPCVOID src, DNS_CHARSET in, DNS_CHARSET out )
{
    switch (in)
    {
    case DnsCharSetUnicode:
    {
        switch (out)
        {
        case DnsCharSetUnicode: return strdup_w( src );
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
        case DnsCharSetUtf8:    return strdup_u( src );
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
        case DnsCharSetAnsi:    return strdup_a( src );
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
    dst = heap_alloc_zero( size );
    if (!dst) return NULL;

    memcpy( dst, src, size );

    if (src->Flags.S.CharSet == DnsCharSetUtf8 ||
        src->Flags.S.CharSet == DnsCharSetAnsi ||
        src->Flags.S.CharSet == DnsCharSetUnicode) in = src->Flags.S.CharSet;

    dst->Flags.S.CharSet = out;
    dst->pName = strcpyX( src->pName, in, out );
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
            dst->Data.TXT.pStringArray[i] = strcpyX( src->Data.TXT.pStringArray[i], in, out );
            if (!dst->Data.TXT.pStringArray[i])
            {
                while (i > 0) heap_free( dst->Data.TXT.pStringArray[--i] );
                goto error;
            }
        }
        break;
    }
    case DNS_TYPE_MINFO:
    case DNS_TYPE_RP:
    {
        dst->Data.MINFO.pNameMailbox = strcpyX( src->Data.MINFO.pNameMailbox, in, out );
        if (!dst->Data.MINFO.pNameMailbox) goto error;

        dst->Data.MINFO.pNameErrorsMailbox = strcpyX( src->Data.MINFO.pNameErrorsMailbox, in, out );
        if (!dst->Data.MINFO.pNameErrorsMailbox)
        {
            heap_free( dst->Data.MINFO.pNameMailbox );
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
        dst->Data.MX.pNameExchange = strcpyX( src->Data.MX.pNameExchange, in, out );
        if (!dst->Data.MX.pNameExchange) goto error;

        dst->wDataLength = sizeof(dst->Data.MX);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.MX.pNameExchange ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_NXT:
    {
        dst->Data.NXT.pNameNext = strcpyX( src->Data.NXT.pNameNext, in, out );
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
        dst->Data.PTR.pNameHost = strcpyX( src->Data.PTR.pNameHost, in, out );
        if (!dst->Data.PTR.pNameHost) goto error;

        dst->wDataLength = sizeof(dst->Data.PTR);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.PTR.pNameHost ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_SIG:
    {
        dst->Data.SIG.pNameSigner = strcpyX( src->Data.SIG.pNameSigner, in, out );
        if (!dst->Data.SIG.pNameSigner) goto error;

        dst->wDataLength = sizeof(dst->Data.SIG);
        if (out == DnsCharSetUnicode) dst->wDataLength +=
            (wcslen( dst->Data.SIG.pNameSigner ) + 1) * sizeof(WCHAR);
        break;
    }
    case DNS_TYPE_SOA:
    {
        dst->Data.SOA.pNamePrimaryServer = strcpyX( src->Data.SOA.pNamePrimaryServer, in, out );
        if (!dst->Data.SOA.pNamePrimaryServer) goto error;

        dst->Data.SOA.pNameAdministrator = strcpyX( src->Data.SOA.pNameAdministrator, in, out );
        if (!dst->Data.SOA.pNameAdministrator)
        {
            heap_free( dst->Data.SOA.pNamePrimaryServer );
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
        dst->Data.SRV.pNameTarget = strcpyX( src->Data.SRV.pNameTarget, in, out );
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
    heap_free( dst->pName );
    heap_free( dst );
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
            heap_free( r->pName );

            switch (r->wType)
            {
            case DNS_TYPE_HINFO:
            case DNS_TYPE_ISDN:
            case DNS_TYPE_TEXT:
            case DNS_TYPE_X25:
            {
                for (i = 0; i < r->Data.TXT.dwStringCount; i++)
                    heap_free( r->Data.TXT.pStringArray[i] );

                break;
            }
            case DNS_TYPE_MINFO:
            case DNS_TYPE_RP:
            {
                heap_free( r->Data.MINFO.pNameMailbox );
                heap_free( r->Data.MINFO.pNameErrorsMailbox );
                break;
            }
            case DNS_TYPE_AFSDB:
            case DNS_TYPE_RT:
            case DNS_TYPE_MX:
            {
                heap_free( r->Data.MX.pNameExchange );
                break;
            }
            case DNS_TYPE_NXT:
            {
                heap_free( r->Data.NXT.pNameNext );
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
                heap_free( r->Data.PTR.pNameHost );
                break;
            }
            case DNS_TYPE_SIG:
            {
                heap_free( r->Data.SIG.pNameSigner );
                break;
            }
            case DNS_TYPE_SOA:
            {
                heap_free( r->Data.SOA.pNamePrimaryServer );
                heap_free( r->Data.SOA.pNameAdministrator );
                break;
            }
            case DNS_TYPE_SRV:
            {
                heap_free( r->Data.SRV.pNameTarget );
                break;
            }
            default:
                break;
            }

            next = r->pNext;
            heap_free( r );
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
