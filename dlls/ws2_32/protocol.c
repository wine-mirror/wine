/*
 * Protocol-level socket functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
 * Copyright (C) 2005 Marcus Meissner
 * Copyright (C) 2006-2008 Kai Blin
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

#include "ws2_32_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

DECLARE_CRITICAL_SECTION(csWSgetXXXbyYYY);

#define MAP_OPTION(opt) { WS_##opt, opt }

static const int ws_aiflag_map[][2] =
{
    MAP_OPTION( AI_PASSIVE ),
    MAP_OPTION( AI_CANONNAME ),
    MAP_OPTION( AI_NUMERICHOST ),
#ifdef AI_NUMERICSERV
    MAP_OPTION( AI_NUMERICSERV ),
#endif
#ifdef AI_V4MAPPED
    MAP_OPTION( AI_V4MAPPED ),
#endif
    MAP_OPTION( AI_ALL ),
    MAP_OPTION( AI_ADDRCONFIG ),
};

static const int ws_eai_map[][2] =
{
    MAP_OPTION( EAI_AGAIN ),
    MAP_OPTION( EAI_BADFLAGS ),
    MAP_OPTION( EAI_FAIL ),
    MAP_OPTION( EAI_FAMILY ),
    MAP_OPTION( EAI_MEMORY ),
/* Note: EAI_NODATA is deprecated, but still used by Windows and Linux. We map
 * the newer EAI_NONAME to EAI_NODATA for now until Windows changes too. */
#ifdef EAI_NODATA
    MAP_OPTION( EAI_NODATA ),
#endif
#ifdef EAI_NONAME
    { WS_EAI_NODATA, EAI_NONAME },
#endif
    MAP_OPTION( EAI_SERVICE ),
    MAP_OPTION( EAI_SOCKTYPE ),
    { 0, 0 }
};

static const int ws_af_map[][2] =
{
    MAP_OPTION( AF_UNSPEC ),
    MAP_OPTION( AF_INET ),
    MAP_OPTION( AF_INET6 ),
#ifdef HAS_IPX
    MAP_OPTION( AF_IPX ),
#endif
#ifdef AF_IRDA
    MAP_OPTION( AF_IRDA ),
#endif
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

static const int ws_proto_map[][2] =
{
    MAP_OPTION( IPPROTO_IP ),
    MAP_OPTION( IPPROTO_TCP ),
    MAP_OPTION( IPPROTO_UDP ),
    MAP_OPTION( IPPROTO_IPV6 ),
    MAP_OPTION( IPPROTO_ICMP ),
    MAP_OPTION( IPPROTO_IGMP ),
    MAP_OPTION( IPPROTO_RAW ),
    {WS_IPPROTO_IPV4, IPPROTO_IPIP},
    {FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO},
};

#define IS_IPX_PROTO(X) ((X) >= WS_NSPROTO_IPX && (X) <= WS_NSPROTO_IPX + 255)

static int convert_af_w2u( int family )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_af_map); i++)
    {
        if (ws_af_map[i][0] == family)
            return ws_af_map[i][1];
    }
    FIXME( "unhandled Windows address family %d\n", family );
    return -1;
}

static int convert_af_u2w( int family )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_af_map); i++)
    {
        if (ws_af_map[i][1] == family)
            return ws_af_map[i][0];
    }
    FIXME( "unhandled UNIX address family %d\n", family );
    return -1;
}

static int convert_proto_w2u( int protocol )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_proto_map); i++)
    {
        if (ws_proto_map[i][0] == protocol)
            return ws_proto_map[i][1];
    }

    if (IS_IPX_PROTO(protocol))
      return protocol;

    FIXME( "unhandled Windows socket protocol %d\n", protocol );
    return -1;
}

static int convert_proto_u2w( int protocol )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(ws_proto_map); i++)
    {
        if (ws_proto_map[i][1] == protocol)
            return ws_proto_map[i][0];
    }

    /* if value is inside IPX range just return it - the kernel simply
     * echoes the value used in the socket() function */
    if (IS_IPX_PROTO(protocol))
        return protocol;

    FIXME("unhandled UNIX socket protocol %d\n", protocol);
    return -1;
}

static int convert_aiflag_w2u( int winflags )
{
    unsigned int i;
    int unixflags = 0;

    for (i = 0; i < ARRAY_SIZE(ws_aiflag_map); i++)
    {
        if (ws_aiflag_map[i][0] & winflags)
        {
            unixflags |= ws_aiflag_map[i][1];
            winflags &= ~ws_aiflag_map[i][0];
        }
    }
    if (winflags)
        FIXME( "Unhandled windows AI_xxx flags 0x%x\n", winflags );
    return unixflags;
}

static int convert_aiflag_u2w( int unixflags )
{
    unsigned int i;
    int winflags = 0;

    for (i = 0; i < ARRAY_SIZE(ws_aiflag_map); i++)
    {
        if (ws_aiflag_map[i][1] & unixflags)
        {
            winflags |= ws_aiflag_map[i][0];
            unixflags &= ~ws_aiflag_map[i][1];
        }
    }
    if (unixflags)
        WARN( "Unhandled UNIX AI_xxx flags 0x%x\n", unixflags );
    return winflags;
}

int convert_eai_u2w( int unixret )
{
    int i;

    if (!unixret) return 0;

    for (i = 0; ws_eai_map[i][0]; i++)
    {
        if (ws_eai_map[i][1] == unixret)
            return ws_eai_map[i][0];
    }

    if (unixret == EAI_SYSTEM)
        /* There are broken versions of glibc which return EAI_SYSTEM
         * and set errno to 0 instead of returning EAI_NONAME. */
        return errno ? sock_get_error( errno ) : WS_EAI_NONAME;

    FIXME("Unhandled unix EAI_xxx ret %d\n", unixret);
    return unixret;
}

static char *get_fqdn(void)
{
    char *ret;
    DWORD size = 0;

    GetComputerNameExA( ComputerNamePhysicalDnsFullyQualified, NULL, &size );
    if (GetLastError() != ERROR_MORE_DATA) return NULL;
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!GetComputerNameExA( ComputerNamePhysicalDnsFullyQualified, ret, &size ))
    {
        HeapFree( GetProcessHeap(), 0, ret );
        return NULL;
    }
    return ret;
}

static BOOL addrinfo_in_list( const struct WS_addrinfo *list, const struct WS_addrinfo *ai )
{
    const struct WS_addrinfo *cursor = list;
    while (cursor)
    {
        if (ai->ai_flags == cursor->ai_flags &&
            ai->ai_family == cursor->ai_family &&
            ai->ai_socktype == cursor->ai_socktype &&
            ai->ai_protocol == cursor->ai_protocol &&
            ai->ai_addrlen == cursor->ai_addrlen &&
            !memcmp(ai->ai_addr, cursor->ai_addr, ai->ai_addrlen) &&
            ((ai->ai_canonname && cursor->ai_canonname && !strcmp(ai->ai_canonname, cursor->ai_canonname))
            || (!ai->ai_canonname && !cursor->ai_canonname)))
        {
            return TRUE;
        }
        cursor = cursor->ai_next;
    }
    return FALSE;
}


/***********************************************************************
 *      getaddrinfo   (ws2_32.@)
 */
int WINAPI WS_getaddrinfo( const char *nodename, const char *servname,
                           const struct WS_addrinfo *hints, struct WS_addrinfo **res )
{
#ifdef HAVE_GETADDRINFO
    struct addrinfo *unixaires = NULL;
    int result;
    struct addrinfo unixhints, *punixhints = NULL;
    char *nodev6 = NULL, *fqdn = NULL;
    const char *node;

    *res = NULL;
    if (!nodename && !servname)
    {
        SetLastError(WSAHOST_NOT_FOUND);
        return WSAHOST_NOT_FOUND;
    }

    if (!nodename)
        node = NULL;
    else if (!nodename[0])
    {
        if (!(fqdn = get_fqdn())) return WSA_NOT_ENOUGH_MEMORY;
        node = fqdn;
    }
    else
    {
        node = nodename;

        /* Check for [ipv6] or [ipv6]:portnumber, which are supported by Windows */
        if (!hints || hints->ai_family == WS_AF_UNSPEC || hints->ai_family == WS_AF_INET6)
        {
            char *close_bracket;

            if (node[0] == '[' && (close_bracket = strchr(node + 1, ']')))
            {
                nodev6 = HeapAlloc( GetProcessHeap(), 0, close_bracket - node );
                if (!nodev6) return WSA_NOT_ENOUGH_MEMORY;
                lstrcpynA( nodev6, node + 1, close_bracket - node );
                node = nodev6;
            }
        }
    }

    /* servname tweak required by OSX and BSD kernels */
    if (servname && !servname[0]) servname = "0";

    if (hints)
    {
        punixhints = &unixhints;

        memset( &unixhints, 0, sizeof(unixhints) );
        punixhints->ai_flags = convert_aiflag_w2u( hints->ai_flags );

        /* zero is a wildcard, no need to convert */
        if (hints->ai_family)
            punixhints->ai_family = convert_af_w2u( hints->ai_family );
        if (hints->ai_socktype)
            punixhints->ai_socktype = convert_socktype_w2u( hints->ai_socktype );
        if (hints->ai_protocol)
            punixhints->ai_protocol = max( convert_proto_w2u( hints->ai_protocol ), 0 );

        if (punixhints->ai_socktype < 0)
        {
            SetLastError( WSAESOCKTNOSUPPORT );
            HeapFree( GetProcessHeap(), 0, fqdn );
            HeapFree( GetProcessHeap(), 0, nodev6 );
            return -1;
        }

        /* windows allows invalid combinations of socket type and protocol, unix does not.
         * fix the parameters here to make getaddrinfo call always work */
        if (punixhints->ai_protocol == IPPROTO_TCP
                && punixhints->ai_socktype != SOCK_STREAM
                && punixhints->ai_socktype != SOCK_SEQPACKET)
            punixhints->ai_socktype = 0;
        else if (punixhints->ai_protocol == IPPROTO_UDP && punixhints->ai_socktype != SOCK_DGRAM)
            punixhints->ai_socktype = 0;
        else if (IS_IPX_PROTO(punixhints->ai_protocol) && punixhints->ai_socktype != SOCK_DGRAM)
            punixhints->ai_socktype = 0;
        else if (punixhints->ai_protocol == IPPROTO_IPV6)
            punixhints->ai_protocol = 0;
    }

    /* getaddrinfo(3) is thread safe, no need to wrap in CS */
    result = getaddrinfo( node, servname, punixhints, &unixaires );

    if (result && (!hints || !(hints->ai_flags & WS_AI_NUMERICHOST)) && node)
    {
        if (!fqdn && !(fqdn = get_fqdn()))
        {
            HeapFree( GetProcessHeap(), 0, nodev6 );
            return WSA_NOT_ENOUGH_MEMORY;
        }
        if (!strcmp( fqdn, node ) || (!strncmp( fqdn, node, strlen( node ) ) && fqdn[strlen( node )] == '.'))
        {
            /* If it didn't work it means the host name IP is not in /etc/hosts, try again
             * by sending a NULL host and avoid sending a NULL servname too because that
             * is invalid */
            ERR_(winediag)( "Failed to resolve your host name IP\n" );
            result = getaddrinfo( NULL, servname ? servname : "0", punixhints, &unixaires );
            if (!result && punixhints && (punixhints->ai_flags & AI_CANONNAME) && unixaires && !unixaires->ai_canonname)
            {
                freeaddrinfo( unixaires );
                result = EAI_NONAME;
            }
        }
    }
    TRACE( "%s, %s %p -> %p %d\n", debugstr_a(nodename), debugstr_a(servname), hints, res, result );
    HeapFree( GetProcessHeap(), 0, fqdn );
    HeapFree( GetProcessHeap(), 0, nodev6 );

    if (!result)
    {
        struct addrinfo *xuai = unixaires;
        struct WS_addrinfo **xai = res;

        *xai = NULL;
        while (xuai)
        {
            struct WS_addrinfo *ai = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct WS_addrinfo) );
            SIZE_T len;

            if (!ai)
                goto outofmem;

            ai->ai_flags    = convert_aiflag_u2w( xuai->ai_flags );
            ai->ai_family   = convert_af_u2w( xuai->ai_family );
            /* copy whatever was sent in the hints */
            if (hints)
            {
                ai->ai_socktype = hints->ai_socktype;
                ai->ai_protocol = hints->ai_protocol;
            }
            else
            {
                ai->ai_socktype = convert_socktype_u2w( xuai->ai_socktype );
                ai->ai_protocol = convert_proto_u2w( xuai->ai_protocol );
            }
            if (xuai->ai_canonname)
            {
                TRACE( "canon name - %s\n", debugstr_a(xuai->ai_canonname) );
                ai->ai_canonname = HeapAlloc( GetProcessHeap(), 0, strlen( xuai->ai_canonname ) + 1 );
                if (!ai->ai_canonname)
                    goto outofmem;
                strcpy( ai->ai_canonname, xuai->ai_canonname );
            }
            len = xuai->ai_addrlen;
            ai->ai_addr = HeapAlloc( GetProcessHeap(), 0, len );
            if (!ai->ai_addr)
                goto outofmem;
            ai->ai_addrlen = len;
            do
            {
                int winlen = ai->ai_addrlen;

                if (!ws_sockaddr_u2ws( xuai->ai_addr, ai->ai_addr, &winlen ))
                {
                    ai->ai_addrlen = winlen;
                    break;
                }
                len *= 2;
                ai->ai_addr = HeapReAlloc( GetProcessHeap(), 0, ai->ai_addr, len );
                if (!ai->ai_addr)
                    goto outofmem;
                ai->ai_addrlen = len;
            } while (1);

            if (addrinfo_in_list( *res, ai ))
            {
                HeapFree( GetProcessHeap(), 0, ai->ai_canonname );
                HeapFree( GetProcessHeap(), 0, ai->ai_addr );
                HeapFree( GetProcessHeap(), 0, ai );
            }
            else
            {
                *xai = ai;
                xai = &ai->ai_next;
            }
            xuai = xuai->ai_next;
        }
        freeaddrinfo( unixaires );

        if (TRACE_ON(winsock))
        {
            struct WS_addrinfo *ai = *res;
            while (ai)
            {
                TRACE( "=> %p, flags %#x, family %d, type %d, protocol %d, len %ld, name %s, addr %s\n",
                       ai, ai->ai_flags, ai->ai_family, ai->ai_socktype, ai->ai_protocol, ai->ai_addrlen,
                       ai->ai_canonname, debugstr_sockaddr(ai->ai_addr) );
                ai = ai->ai_next;
            }
        }
    }
    else
        result = convert_eai_u2w( result );

    SetLastError( result );
    return result;

outofmem:
    if (*res) WS_freeaddrinfo( *res );
    if (unixaires) freeaddrinfo( unixaires );
    return WSA_NOT_ENOUGH_MEMORY;
#else
    FIXME( "getaddrinfo() failed, not found during build time.\n" );
    return EAI_FAIL;
#endif
}


static ADDRINFOEXW *addrinfo_AtoW( const struct WS_addrinfo *ai )
{
    ADDRINFOEXW *ret;

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, sizeof(ADDRINFOEXW) ))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_blob      = NULL;
    ret->ai_bloblen   = 0;
    ret->ai_provider  = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, ai->ai_canonname, -1, NULL, 0 );
        if (!(ret->ai_canonname = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }
        MultiByteToWideChar( CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len );
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc( GetProcessHeap(), 0, ai->ai_addrlen )))
        {
            HeapFree( GetProcessHeap(), 0, ret->ai_canonname );
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }
        memcpy( ret->ai_addr, ai->ai_addr, ai->ai_addrlen );
    }
    return ret;
}

static ADDRINFOEXW *addrinfo_list_AtoW( const struct WS_addrinfo *info )
{
    ADDRINFOEXW *ret, *infoW;

    if (!(ret = infoW = addrinfo_AtoW( info ))) return NULL;
    while (info->ai_next)
    {
        if (!(infoW->ai_next = addrinfo_AtoW( info->ai_next )))
        {
            FreeAddrInfoExW( ret );
            return NULL;
        }
        infoW = infoW->ai_next;
        info = info->ai_next;
    }
    return ret;
}

static struct WS_addrinfo *addrinfo_WtoA( const struct WS_addrinfoW *ai )
{
    struct WS_addrinfo *ret;

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, sizeof(struct WS_addrinfo) ))) return NULL;
    ret->ai_flags     = ai->ai_flags;
    ret->ai_family    = ai->ai_family;
    ret->ai_socktype  = ai->ai_socktype;
    ret->ai_protocol  = ai->ai_protocol;
    ret->ai_addrlen   = ai->ai_addrlen;
    ret->ai_canonname = NULL;
    ret->ai_addr      = NULL;
    ret->ai_next      = NULL;
    if (ai->ai_canonname)
    {
        int len = WideCharToMultiByte( CP_ACP, 0, ai->ai_canonname, -1, NULL, 0, NULL, NULL );
        if (!(ret->ai_canonname = HeapAlloc( GetProcessHeap(), 0, len )))
        {
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }
        WideCharToMultiByte( CP_ACP, 0, ai->ai_canonname, -1, ret->ai_canonname, len, NULL, NULL );
    }
    if (ai->ai_addr)
    {
        if (!(ret->ai_addr = HeapAlloc( GetProcessHeap(), 0, sizeof(struct WS_sockaddr) )))
        {
            HeapFree( GetProcessHeap(), 0, ret->ai_canonname );
            HeapFree( GetProcessHeap(), 0, ret );
            return NULL;
        }
        memcpy( ret->ai_addr, ai->ai_addr, sizeof(struct WS_sockaddr) );
    }
    return ret;
}

struct getaddrinfo_args
{
    OVERLAPPED *overlapped;
    LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine;
    ADDRINFOEXW **result;
    char *nodename;
    char *servname;
};

static void WINAPI getaddrinfo_callback(TP_CALLBACK_INSTANCE *instance, void *context)
{
    struct getaddrinfo_args *args = context;
    OVERLAPPED *overlapped = args->overlapped;
    HANDLE event = overlapped->hEvent;
    LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine = args->completion_routine;
    struct WS_addrinfo *res;
    int ret;

    ret = WS_getaddrinfo( args->nodename, args->servname, NULL, &res );
    if (res)
    {
        *args->result = addrinfo_list_AtoW(res);
        overlapped->u.Pointer = args->result;
        WS_freeaddrinfo(res);
    }

    HeapFree( GetProcessHeap(), 0, args->nodename );
    HeapFree( GetProcessHeap(), 0, args->servname );
    HeapFree( GetProcessHeap(), 0, args );

    overlapped->Internal = ret;
    if (completion_routine) completion_routine( ret, 0, overlapped );
    if (event) SetEvent( event );
}

static int WS_getaddrinfoW( const WCHAR *nodename, const WCHAR *servname,
                            const struct WS_addrinfo *hints, ADDRINFOEXW **res, OVERLAPPED *overlapped,
                            LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine )
{
    int ret = EAI_MEMORY, len, i;
    char *nodenameA = NULL, *servnameA = NULL;
    struct WS_addrinfo *resA;
    WCHAR *local_nodenameW = (WCHAR *)nodename;

    *res = NULL;
    if (nodename)
    {
        /* Is this an IDN? Most likely if any char is above the Ascii table, this
         * is the simplest validation possible, further validation will be done by
         * the native getaddrinfo() */
        for (i = 0; nodename[i]; i++)
        {
            if (nodename[i] > 'z')
                break;
        }
        if (nodename[i])
        {
            if (hints && (hints->ai_flags & WS_AI_DISABLE_IDN_ENCODING))
            {
                /* Name requires conversion but it was disabled */
                ret = WSAHOST_NOT_FOUND;
                SetLastError( ret );
                goto end;
            }

            len = IdnToAscii( 0, nodename, -1, NULL, 0 );
            if (!len)
            {
                ERR("Failed to convert %s to punycode\n", debugstr_w(nodename));
                ret = EAI_FAIL;
                goto end;
            }
            if (!(local_nodenameW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) goto end;
            IdnToAscii( 0, nodename, -1, local_nodenameW, len );
        }
    }
    if (local_nodenameW)
    {
        len = WideCharToMultiByte( CP_ACP, 0, local_nodenameW, -1, NULL, 0, NULL, NULL );
        if (!(nodenameA = HeapAlloc( GetProcessHeap(), 0, len ))) goto end;
        WideCharToMultiByte( CP_ACP, 0, local_nodenameW, -1, nodenameA, len, NULL, NULL );
    }
    if (servname)
    {
        len = WideCharToMultiByte( CP_ACP, 0, servname, -1, NULL, 0, NULL, NULL );
        if (!(servnameA = HeapAlloc( GetProcessHeap(), 0, len ))) goto end;
        WideCharToMultiByte( CP_ACP, 0, servname, -1, servnameA, len, NULL, NULL );
    }

    if (overlapped)
    {
        struct getaddrinfo_args *args;

        if (overlapped->hEvent && completion_routine)
        {
            ret = WSAEINVAL;
            goto end;
        }

        if (!(args = HeapAlloc( GetProcessHeap(), 0, sizeof(*args) ))) goto end;
        args->overlapped = overlapped;
        args->completion_routine = completion_routine;
        args->result = res;
        args->nodename = nodenameA;
        args->servname = servnameA;

        overlapped->Internal = WSAEINPROGRESS;
        if (!TrySubmitThreadpoolCallback( getaddrinfo_callback, args, NULL ))
        {
            HeapFree( GetProcessHeap(), 0, args );
            ret = GetLastError();
            goto end;
        }

        if (local_nodenameW != nodename)
            HeapFree( GetProcessHeap(), 0, local_nodenameW );
        SetLastError( ERROR_IO_PENDING );
        return ERROR_IO_PENDING;
    }

    ret = WS_getaddrinfo( nodenameA, servnameA, hints, &resA );
    if (!ret)
    {
        *res = addrinfo_list_AtoW( resA );
        WS_freeaddrinfo( resA );
    }

end:
    if (local_nodenameW != nodename)
        HeapFree( GetProcessHeap(), 0, local_nodenameW );
    HeapFree( GetProcessHeap(), 0, nodenameA );
    HeapFree( GetProcessHeap(), 0, servnameA );
    return ret;
}


/***********************************************************************
 *      GetAddrInfoExW   (ws2_32.@)
 */
int WINAPI GetAddrInfoExW( const WCHAR *name, const WCHAR *servname, DWORD namespace,
                           GUID *namespace_id, const ADDRINFOEXW *hints, ADDRINFOEXW **result,
                           struct WS_timeval *timeout, OVERLAPPED *overlapped,
                           LPLOOKUPSERVICE_COMPLETION_ROUTINE completion_routine, HANDLE *handle )
{
    int ret;

    TRACE( "name %s, servname %s, namespace %u, namespace_id %s)\n",
           debugstr_w(name), debugstr_w(servname), namespace, debugstr_guid(namespace_id) );

    if (namespace != NS_DNS)
        FIXME( "Unsupported namespace %u\n", namespace );
    if (namespace_id)
        FIXME( "Unsupported namespace_id %s\n", debugstr_guid(namespace_id) );
    if (hints)
        FIXME( "Unsupported hints\n" );
    if (timeout)
        FIXME( "Unsupported timeout\n" );
    if (handle)
        FIXME( "Unsupported cancel handle\n" );

    ret = WS_getaddrinfoW( name, servname, NULL, result, overlapped, completion_routine );
    if (ret) return ret;
    if (handle) *handle = (HANDLE)0xdeadbeef;
    return 0;
}


/***********************************************************************
 *      GetAddrInfoExOverlappedResult   (ws2_32.@)
 */
int WINAPI GetAddrInfoExOverlappedResult( OVERLAPPED *overlapped )
{
    TRACE( "(%p)\n", overlapped );
    return overlapped->Internal;
}


/***********************************************************************
 *      GetAddrInfoExCancel   (ws2_32.@)
 */
int WINAPI GetAddrInfoExCancel( HANDLE *handle )
{
    FIXME( "(%p)\n", handle );
    return WSA_INVALID_HANDLE;
}


/***********************************************************************
 *      GetAddrInfoW   (ws2_32.@)
 */
int WINAPI GetAddrInfoW(const WCHAR *nodename, const WCHAR *servname, const ADDRINFOW *hints, ADDRINFOW **res)
{
    struct WS_addrinfo *hintsA = NULL;
    ADDRINFOEXW *resex;
    int ret = EAI_MEMORY;

    TRACE( "nodename %s, servname %s, hints %p, result %p\n",
           debugstr_w(nodename), debugstr_w(servname), hints, res );

    *res = NULL;
    if (hints) hintsA = addrinfo_WtoA( hints );
    ret = WS_getaddrinfoW( nodename, servname, hintsA, &resex, NULL, NULL );
    WS_freeaddrinfo( hintsA );
    if (ret) return ret;

    if (resex)
    {
        /* ADDRINFOEXW has a layout compatible with ADDRINFOW except for the
         * ai_next field, so we may convert it in place */
        *res = (ADDRINFOW *)resex;
        do
        {
            ((ADDRINFOW *)resex)->ai_next = (ADDRINFOW *)resex->ai_next;
            resex = resex->ai_next;
        } while (resex);
    }
    return 0;
}


/***********************************************************************
 *      freeaddrinfo   (ws2_32.@)
 */
void WINAPI WS_freeaddrinfo( struct WS_addrinfo *res )
{
    while (res)
    {
        struct WS_addrinfo *next;

        HeapFree( GetProcessHeap(), 0, res->ai_canonname );
        HeapFree( GetProcessHeap(), 0, res->ai_addr );
        next = res->ai_next;
        HeapFree( GetProcessHeap(), 0, res );
        res = next;
    }
}


/***********************************************************************
 *      FreeAddrInfoW   (ws2_32.@)
 */
void WINAPI FreeAddrInfoW( ADDRINFOW *ai )
{
    while (ai)
    {
        ADDRINFOW *next;
        HeapFree( GetProcessHeap(), 0, ai->ai_canonname );
        HeapFree( GetProcessHeap(), 0, ai->ai_addr );
        next = ai->ai_next;
        HeapFree( GetProcessHeap(), 0, ai );
        ai = next;
    }
}


/***********************************************************************
 *      FreeAddrInfoEx   (ws2_32.@)
 */
void WINAPI FreeAddrInfoEx( ADDRINFOEXA *ai )
{
    TRACE( "(%p)\n", ai );

    while (ai)
    {
        ADDRINFOEXA *next;
        HeapFree( GetProcessHeap(), 0, ai->ai_canonname );
        HeapFree( GetProcessHeap(), 0, ai->ai_addr );
        next = ai->ai_next;
        HeapFree( GetProcessHeap(), 0, ai );
        ai = next;
    }
}


/***********************************************************************
 *      FreeAddrInfoExW   (ws2_32.@)
 */
void WINAPI FreeAddrInfoExW( ADDRINFOEXW *ai )
{
    TRACE( "(%p)\n", ai );

    while (ai)
    {
        ADDRINFOEXW *next;
        HeapFree( GetProcessHeap(), 0, ai->ai_canonname );
        HeapFree( GetProcessHeap(), 0, ai->ai_addr );
        next = ai->ai_next;
        HeapFree( GetProcessHeap(), 0, ai );
        ai = next;
    }
}


static const int ws_niflag_map[][2] =
{
    MAP_OPTION( NI_NOFQDN ),
    MAP_OPTION( NI_NUMERICHOST ),
    MAP_OPTION( NI_NAMEREQD ),
    MAP_OPTION( NI_NUMERICSERV ),
    MAP_OPTION( NI_DGRAM ),
};

static int convert_niflag_w2u( int winflags )
{
    unsigned int i;
    int unixflags = 0;

    for (i = 0; i < ARRAY_SIZE(ws_niflag_map); i++)
    {
        if (ws_niflag_map[i][0] & winflags)
        {
            unixflags |= ws_niflag_map[i][1];
            winflags &= ~ws_niflag_map[i][0];
        }
    }
    if (winflags)
        FIXME("Unhandled windows NI_xxx flags 0x%x\n", winflags);
    return unixflags;
}


/***********************************************************************
 *      getnameinfo   (ws2_32.@)
 */
int WINAPI WS_getnameinfo( const SOCKADDR *addr, WS_socklen_t addr_len, char *host,
                           DWORD host_len, char *serv, DWORD serv_len, int flags )
{
#ifdef HAVE_GETNAMEINFO
    int ret;
    union generic_unix_sockaddr uaddr;
    unsigned int uaddr_len;

    TRACE( "addr %s, addr_len %d, host %p, host_len %u, serv %p, serv_len %d, flags %#x\n",
           debugstr_sockaddr(addr), addr_len, host, host_len, serv, serv_len, flags );

    uaddr_len = ws_sockaddr_ws2u( addr, addr_len, &uaddr );
    if (!uaddr_len)
    {
        SetLastError( WSAEFAULT );
        return WSA_NOT_ENOUGH_MEMORY;
    }
    ret = getnameinfo( &uaddr.addr, uaddr_len, host, host_len, serv, serv_len, convert_niflag_w2u(flags) );
    return convert_eai_u2w( ret );
#else
    FIXME( "getnameinfo() failed, not found during buildtime.\n" );
    return EAI_FAIL;
#endif
}


/***********************************************************************
 *      GetNameInfoW   (ws2_32.@)
 */
int WINAPI GetNameInfoW( const SOCKADDR *addr, WS_socklen_t addr_len, WCHAR *host,
                         DWORD host_len, WCHAR *serv, DWORD serv_len, int flags )
{
    int ret;
    char *hostA = NULL, *servA = NULL;

    if (host && (!(hostA = HeapAlloc( GetProcessHeap(), 0, host_len ))))
        return EAI_MEMORY;
    if (serv && (!(servA = HeapAlloc( GetProcessHeap(), 0, serv_len ))))
    {
        HeapFree( GetProcessHeap(), 0, hostA );
        return EAI_MEMORY;
    }

    ret = WS_getnameinfo( addr, addr_len, hostA, host_len, servA, serv_len, flags );
    if (!ret)
    {
        if (host) MultiByteToWideChar( CP_ACP, 0, hostA, -1, host, host_len );
        if (serv) MultiByteToWideChar( CP_ACP, 0, servA, -1, serv, serv_len );
    }

    HeapFree( GetProcessHeap(), 0, hostA );
    HeapFree( GetProcessHeap(), 0, servA );
    return ret;
}


static UINT host_errno_from_unix( int err )
{
    WARN( "%d\n", err );

    switch (err)
    {
        case HOST_NOT_FOUND:    return WSAHOST_NOT_FOUND;
        case TRY_AGAIN:         return WSATRY_AGAIN;
        case NO_RECOVERY:       return WSANO_RECOVERY;
        case NO_DATA:           return WSANO_DATA;
        case ENOBUFS:           return WSAENOBUFS;
        case 0:                 return 0;
        default:
            WARN( "Unknown h_errno %d!\n", err );
            return WSAEOPNOTSUPP;
    }
}

static struct WS_hostent *get_hostent_buffer( unsigned int size )
{
    struct per_thread_data *data = get_per_thread_data();
    if (data->he_buffer)
    {
        if (data->he_len >= size) return data->he_buffer;
        HeapFree( GetProcessHeap(), 0, data->he_buffer );
    }
    data->he_buffer = HeapAlloc( GetProcessHeap(), 0, (data->he_len = size) );
    if (!data->he_buffer) SetLastError(WSAENOBUFS);
    return data->he_buffer;
}

/* create a hostent entry
 *
 * Creates the entry with enough memory for the name, aliases
 * addresses, and the address pointers.  Also copies the name
 * and sets up all the pointers.
 *
 * NOTE: The alias and address lists must be allocated with room
 * for the NULL item terminating the list.  This is true even if
 * the list has no items ("aliases" and "addresses" must be
 * at least "1", a truly empty list is invalid).
 */
static struct WS_hostent *create_hostent( char *name, int alias_count, int aliases_size,
                                          int address_count, int address_length )
{
    struct WS_hostent *p_to;
    char *p;
    unsigned int size = sizeof(struct WS_hostent), i;

    size += strlen(name) + 1;
    size += alias_count * sizeof(char *);
    size += aliases_size;
    size += address_count * sizeof(char *);
    size += (address_count - 1) * address_length;

    if (!(p_to = get_hostent_buffer( size ))) return NULL;
    memset( p_to, 0, size );

    /* Use the memory in the same way winsock does.
     * First set the pointer for aliases, second set the pointers for addresses.
     * Third fill the addresses indexes, fourth jump aliases names size.
     * Fifth fill the hostname.
     * NOTE: This method is valid for OS versions >= XP.
     */
    p = (char *)(p_to + 1);
    p_to->h_aliases = (char **)p;
    p += alias_count * sizeof(char *);

    p_to->h_addr_list = (char **)p;
    p += address_count * sizeof(char *);

    for (i = 0, address_count--; i < address_count; i++, p += address_length)
        p_to->h_addr_list[i] = p;

    /* h_aliases must be filled in manually because we don't know each string
     * size. Leave these pointers NULL (already set to NULL by memset earlier).
     */
    p += aliases_size;

    p_to->h_name = p;
    strcpy( p, name );

    return p_to;
}

static struct WS_hostent *hostent_from_unix( const struct hostent *p_he )
{
    int i, addresses = 0, alias_size = 0;
    struct WS_hostent *p_to;
    char *p;

    for (i = 0; p_he->h_aliases[i]; i++)
        alias_size += strlen( p_he->h_aliases[i] ) + 1;
    while (p_he->h_addr_list[addresses])
        addresses++;

    p_to = create_hostent( p_he->h_name, i + 1, alias_size, addresses + 1, p_he->h_length );

    if (!p_to) return NULL;
    p_to->h_addrtype = convert_af_u2w( p_he->h_addrtype );
    p_to->h_length = p_he->h_length;

    for (i = 0, p = p_to->h_addr_list[0]; p_he->h_addr_list[i]; i++, p += p_to->h_length)
        memcpy( p, p_he->h_addr_list[i], p_to->h_length );

    /* Fill the aliases after the IP data */
    for (i = 0; p_he->h_aliases[i]; i++)
    {
        p_to->h_aliases[i] = p;
        strcpy( p, p_he->h_aliases[i] );
        p += strlen(p) + 1;
    }

    return p_to;
}


/***********************************************************************
 *      gethostbyaddr   (ws2_32.51)
 */
struct WS_hostent * WINAPI WS_gethostbyaddr( const char *addr, int len, int type )
{
    struct WS_hostent *retval = NULL;
    struct hostent *host;
    int unixtype = convert_af_w2u(type);
    const char *paddr = addr;
    unsigned long loopback;
#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize = 1024;
    struct hostent hostentry;
    int locerr = ENOBUFS;
#endif

    /* convert back the magic loopback address if necessary */
    if (unixtype == AF_INET && len == 4 && !memcmp( addr, magic_loopback_addr, 4 ))
    {
        loopback = htonl( INADDR_LOOPBACK );
        paddr = (char *)&loopback;
    }

#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    host = NULL;
    extrabuf = HeapAlloc( GetProcessHeap(), 0, ebufsize );
    while (extrabuf)
    {
        int res = gethostbyaddr_r( paddr, len, unixtype, &hostentry, extrabuf, ebufsize, &host, &locerr );
        if (res != ERANGE) break;
        ebufsize *= 2;
        extrabuf = HeapReAlloc( GetProcessHeap(), 0, extrabuf, ebufsize );
    }
    if (host)
        retval = hostent_from_unix( host );
    else
        SetLastError( (locerr < 0) ? sock_get_error( errno ) : host_errno_from_unix( locerr ) );
    HeapFree( GetProcessHeap(), 0, extrabuf );
#else
    EnterCriticalSection( &csWSgetXXXbyYYY );
    host = gethostbyaddr( paddr, len, unixtype );
    if (host)
        retval = hostent_from_unix( host );
    else
        SetLastError( (h_errno < 0) ? sock_get_error( errno ) : host_errno_from_unix( h_errno ) );
    LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif

    TRACE( "ptr %p, len %d, type %d ret %p\n", addr, len, type, retval );
    return retval;
}


struct route
{
    struct in_addr addr;
    IF_INDEX interface;
    DWORD metric, default_route;
};

static int compare_routes_by_metric_asc( const void *left, const void *right )
{
    const struct route *a = left, *b = right;
    if (a->default_route && b->default_route)
        return a->default_route - b->default_route;
    if (a->default_route && !b->default_route)
        return -1;
    if (b->default_route && !a->default_route)
        return 1;
    return a->metric - b->metric;
}

/* Returns the list of local IP addresses by going through the network
 * adapters and using the local routing table to sort the addresses
 * from highest routing priority to lowest routing priority. This
 * functionality is inferred from the description for obtaining local
 * IP addresses given in the Knowledge Base Article Q160215.
 *
 * Please note that the returned hostent is only freed when the thread
 * closes and is replaced if another hostent is requested.
 */
static struct WS_hostent *get_local_ips( char *hostname )
{
    int numroutes = 0, i, j, default_routes = 0;
    IP_ADAPTER_INFO *adapters = NULL, *k;
    struct WS_hostent *hostlist = NULL;
    MIB_IPFORWARDTABLE *routes = NULL;
    struct route *route_addrs = NULL;
    DWORD adap_size, route_size, n;

    /* Obtain the size of the adapter list and routing table, also allocate memory */
    if (GetAdaptersInfo( NULL, &adap_size ) != ERROR_BUFFER_OVERFLOW)
        return NULL;
    if (GetIpForwardTable( NULL, &route_size, FALSE ) != ERROR_INSUFFICIENT_BUFFER)
        return NULL;

    adapters = HeapAlloc( GetProcessHeap(), 0, adap_size );
    routes = HeapAlloc( GetProcessHeap(), 0, route_size );
    if (!adapters || !routes)
        goto cleanup;

    /* Obtain the adapter list and the full routing table */
    if (GetAdaptersInfo( adapters, &adap_size ) != NO_ERROR)
        goto cleanup;
    if (GetIpForwardTable( routes, &route_size, FALSE ) != NO_ERROR)
        goto cleanup;

    /* Store the interface associated with each route */
    for (n = 0; n < routes->dwNumEntries; n++)
    {
        IF_INDEX ifindex;
        DWORD ifmetric, ifdefault = 0;
        BOOL exists = FALSE;

        /* Check if this is a default route (there may be more than one) */
        if (!routes->table[n].dwForwardDest)
            ifdefault = ++default_routes;
        else if (routes->table[n].u1.ForwardType != MIB_IPROUTE_TYPE_DIRECT)
            continue;
        ifindex = routes->table[n].dwForwardIfIndex;
        ifmetric = routes->table[n].dwForwardMetric1;
        /* Only store the lowest valued metric for an interface */
        for (j = 0; j < numroutes; j++)
        {
            if (route_addrs[j].interface == ifindex)
            {
                if (route_addrs[j].metric > ifmetric)
                    route_addrs[j].metric = ifmetric;
                exists = TRUE;
            }
        }
        if (exists)
            continue;
        route_addrs = heap_realloc( route_addrs, (numroutes + 1) * sizeof(struct route) );
        if (!route_addrs)
            goto cleanup;
        route_addrs[numroutes].interface = ifindex;
        route_addrs[numroutes].metric = ifmetric;
        route_addrs[numroutes].default_route = ifdefault;
        /* If no IP is found in the next step (for whatever reason)
         * then fall back to the magic loopback address.
         */
        memcpy( &route_addrs[numroutes].addr.s_addr, magic_loopback_addr, 4 );
        numroutes++;
    }
    if (numroutes == 0)
       goto cleanup; /* No routes, fall back to the Magic IP */

    /* Find the IP address associated with each found interface */
    for (i = 0; i < numroutes; i++)
    {
        for (k = adapters; k != NULL; k = k->Next)
        {
            char *ip = k->IpAddressList.IpAddress.String;

            if (route_addrs[i].interface == k->Index)
                route_addrs[i].addr.s_addr = inet_addr(ip);
        }
    }

    /* Allocate a hostent and enough memory for all the IPs,
     * including the NULL at the end of the list.
     */
    hostlist = create_hostent( hostname, 1, 0, numroutes+1, sizeof(struct in_addr) );
    if (hostlist == NULL)
        goto cleanup;
    hostlist->h_addr_list[numroutes] = NULL;
    hostlist->h_aliases[0] = NULL;
    hostlist->h_addrtype = AF_INET;
    hostlist->h_length = sizeof(struct in_addr);

    /* Reorder the entries before placing them in the host list. Windows expects
     * the IP list in order from highest priority to lowest (the critical thing
     * is that most applications expect the first IP to be the default route).
     */
    if (numroutes > 1)
        qsort( route_addrs, numroutes, sizeof(struct route), compare_routes_by_metric_asc );

    for (i = 0; i < numroutes; i++)
        *(struct in_addr *)hostlist->h_addr_list[i] = route_addrs[i].addr;

cleanup:
    HeapFree( GetProcessHeap(), 0, route_addrs );
    HeapFree( GetProcessHeap(), 0, adapters );
    HeapFree( GetProcessHeap(), 0, routes );
    return hostlist;
}


/***********************************************************************
 *      gethostbyname   (ws2_32.52)
 */
struct WS_hostent * WINAPI WS_gethostbyname( const char *name )
{
    struct WS_hostent *retval = NULL;
    struct hostent *host;
#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
    char *extrabuf;
    int ebufsize = 1024;
    struct hostent hostentry;
    int locerr = ENOBUFS;
#endif
    char hostname[100];

    if (!num_startup)
    {
        SetLastError( WSANOTINITIALISED );
        return NULL;
    }

    if (gethostname( hostname, 100 ) == -1)
    {
        SetLastError( WSAENOBUFS );
        return retval;
    }

    if (!name || !name[0])
        name = hostname;

    /* If the hostname of the local machine is requested then return the
     * complete list of local IP addresses */
    if (!strcmp( name, hostname ))
        retval = get_local_ips( hostname );

    /* If any other hostname was requested (or the routing table lookup failed)
     * then return the IP found by the host OS */
    if (!retval)
    {
#ifdef HAVE_LINUX_GETHOSTBYNAME_R_6
        host = NULL;
        extrabuf = HeapAlloc( GetProcessHeap(), 0, ebufsize );
        while (extrabuf)
        {
            int res = gethostbyname_r( name, &hostentry, extrabuf, ebufsize, &host, &locerr );
            if (res != ERANGE) break;
            ebufsize *= 2;
            extrabuf = HeapReAlloc( GetProcessHeap(), 0, extrabuf, ebufsize );
        }
        if (!host) SetLastError( (locerr < 0) ? sock_get_error( errno ) : host_errno_from_unix( locerr ) );
#else
        EnterCriticalSection( &csWSgetXXXbyYYY );
        host = gethostbyname( name );
        if (!host) SetLastError( (h_errno < 0) ? sock_get_error( errno ) : host_errno_from_unix( h_errno ) );
#endif
        if (host) retval = hostent_from_unix( host );
#ifdef  HAVE_LINUX_GETHOSTBYNAME_R_6
        HeapFree( GetProcessHeap(), 0, extrabuf );
#else
        LeaveCriticalSection( &csWSgetXXXbyYYY );
#endif
    }

    if (retval && retval->h_addr_list[0][0] == 127 && strcmp( name, "localhost" ))
    {
        /* hostname != "localhost" but has loopback address. replace by our
         * special address.*/
        memcpy( retval->h_addr_list[0], magic_loopback_addr, 4 );
    }

    TRACE( "%s ret %p\n", debugstr_a(name), retval );
    return retval;
}


/***********************************************************************
 *      gethostname   (ws2_32.57)
 */
int WINAPI WS_gethostname( char *name, int namelen )
{
    char buf[256];
    int len;

    TRACE( "name %p, len %d\n", name, namelen );

    if (!name)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    if (gethostname( buf, sizeof(buf) ) != 0)
    {
        SetLastError( sock_get_error( errno ) );
        return -1;
    }

    TRACE( "<- %s\n", debugstr_a(buf) );
    len = strlen( buf );
    if (len > 15)
        WARN( "Windows supports NetBIOS name length up to 15 bytes!\n" );
    if (namelen <= len)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }
    strcpy( name, buf );
    return 0;
}


/***********************************************************************
 *      GetHostNameW   (ws2_32.@)
 */
int WINAPI GetHostNameW( WCHAR *name, int namelen )
{
    char buf[256];

    TRACE( "name %p, len %d\n", name, namelen );

    if (!name)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }

    if (gethostname( buf, sizeof(buf) ))
    {
        SetLastError( sock_get_error( errno ) );
        return -1;
    }

    if (MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 ) > namelen)
    {
        SetLastError( WSAEFAULT );
        return -1;
    }
    MultiByteToWideChar( CP_ACP, 0, buf, -1, name, namelen );
    return 0;
}
