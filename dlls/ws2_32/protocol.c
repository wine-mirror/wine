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
