/*
 * Protocol enumeration functions
 *
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2004 Hans Leidekker
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

/*  02/11/2004
 *  The protocol enumeration functions were verified to match Win2k versions
 *  for these protocols: IPX, SPX, SPXII, TCP/IP and UDP/IP.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "nspapi.h"
#include "winsock2.h"
#include "wsipx.h"
#include "wshisotp.h"
#include "ws2spi.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/* names of the protocols */
static const CHAR NameIpx[]   = "IPX";
static const CHAR NameSpx[]   = "SPX";
static const CHAR NameSpxII[] = "SPX II";
static const CHAR NameTcp[]   = "TCP/IP";
static const CHAR NameUdp[]   = "UDP/IP";

static const WCHAR NameIpxW[]   = {'I', 'P', 'X', '\0'};
static const WCHAR NameSpxW[]   = {'S', 'P', 'X', '\0'};
static const WCHAR NameSpxIIW[] = {'S', 'P', 'X', ' ', 'I', 'I', '\0'};
static const WCHAR NameTcpW[]   = {'T', 'C', 'P', '/', 'I', 'P', '\0'};
static const WCHAR NameUdpW[]   = {'U', 'D', 'P', '/', 'I', 'P', '\0'};

/* Taken from Win2k */
static const GUID ProviderIdIP = { 0xe70f1aa0, 0xab8b, 0x11cf,
                                   { 0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };
static const GUID ProviderIdIPX = { 0x11058240, 0xbe47, 0x11cf,
                                    { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };
static const GUID ProviderIdSPX = { 0x11058241, 0xbe47, 0x11cf,
                                    { 0x95, 0xc8, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92 } };

/*****************************************************************************
 *          WINSOCK_EnterSingleProtocolW [internal]
 *
 *    enters the protocol information of one given protocol into the given
 *    buffer.
 *
 * RETURNS
 *    1 if a protocol was entered into the buffer.
 *    SOCKET_ERROR otherwise.
 *
 * BUGS
 *    - only implemented for IPX, SPX, SPXII, TCP, UDP
 *    - there is no check that the operating system supports the returned
 *      protocols
 */
static INT WINSOCK_EnterSingleProtocolW( INT protocol, WSAPROTOCOL_INFOW* info )
{
    memset( info, 0, sizeof(WSAPROTOCOL_INFOW) );
    info->iProtocol = protocol;

    switch (protocol)
    {
    case WS_IPPROTO_TCP:
        info->dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_EXPEDITED_DATA |
                                XP1_GRACEFUL_CLOSE | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdIP, sizeof(GUID) );
        info->dwCatalogEntryId = 0x3e9;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_STREAM;
        strcpyW( info->szProtocol, NameTcpW );
        break;

    case WS_IPPROTO_UDP:
        info->dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        memcpy( &info->ProviderId, &ProviderIdIP, sizeof(GUID) );
        info->dwCatalogEntryId = 0x3ea;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_DGRAM;
        info->dwMessageSize = 0xffbb;
        strcpyW( info->szProtocol, NameUdpW );
        break;

    case NSPROTO_IPX:
        info->dwServiceFlags1 = XP1_PARTIAL_MESSAGE | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        memcpy( &info->ProviderId, &ProviderIdIPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x406;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = WS_SOCK_DGRAM;
        info->iProtocolMaxOffset = 0xff;
        info->dwMessageSize = 0x240;
        strcpyW( info->szProtocol, NameIpxW );
        break;

    case NSPROTO_SPX:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_PSEUDO_STREAM |
                                XP1_MESSAGE_ORIENTED | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdSPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x407;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = 5;
        info->dwMessageSize = 0xffffffff;
        strcpyW( info->szProtocol, NameSpxW );
        break;

    case NSPROTO_SPXII:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_GRACEFUL_CLOSE |
                                XP1_PSEUDO_STREAM | XP1_MESSAGE_ORIENTED |
                                XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdSPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x409;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = 5;
        info->dwMessageSize = 0xffffffff;
        strcpyW( info->szProtocol, NameSpxIIW );
        break;

    default:
        if ((protocol == ISOPROTO_TP4) || (protocol == NSPROTO_SPX))
            FIXME("Protocol <%s> not implemented\n",
                  (protocol == ISOPROTO_TP4) ? "ISOPROTO_TP4" : "NSPROTO_SPX");
        else
            FIXME("unknown Protocol <0x%08x>\n", protocol);
        return SOCKET_ERROR;
    }
    return 1;
}

/*****************************************************************************
 *          WINSOCK_EnterSingleProtocolA [internal]
 *
 *    see function WINSOCK_EnterSingleProtocolW
 *
 */
static INT WINSOCK_EnterSingleProtocolA( INT protocol, WSAPROTOCOL_INFOA* info )
{
    memset( info, 0, sizeof(WSAPROTOCOL_INFOA) );
    info->iProtocol = protocol;

    switch (protocol)
    {
    case WS_IPPROTO_TCP:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_EXPEDITED_DATA |
                                XP1_GRACEFUL_CLOSE | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdIP, sizeof(GUID) );
        info->dwCatalogEntryId = 0x3e9;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_STREAM;
        strcpy( info->szProtocol, NameTcp );
        break;

    case WS_IPPROTO_UDP:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        memcpy( &info->ProviderId, &ProviderIdIP, sizeof(GUID) );
        info->dwCatalogEntryId = 0x3ea;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_INET;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x10;
        info->iSocketType = WS_SOCK_DGRAM;
        info->dwMessageSize = 0xffbb;
        strcpy( info->szProtocol, NameUdp );
        break;

    case NSPROTO_IPX:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_SUPPORT_BROADCAST |
                                XP1_SUPPORT_MULTIPOINT | XP1_MESSAGE_ORIENTED |
                                XP1_CONNECTIONLESS;
        memcpy( &info->ProviderId, &ProviderIdIPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x406;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = WS_SOCK_DGRAM;
        info->iProtocolMaxOffset = 0xff;
        info->dwMessageSize = 0x240;
        strcpy( info->szProtocol, NameIpx );
        break;

    case NSPROTO_SPX:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_PSEUDO_STREAM |
                                XP1_MESSAGE_ORIENTED | XP1_GUARANTEED_ORDER |
                                XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdSPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x407;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = 5;
        info->dwMessageSize = 0xffffffff;
        strcpy( info->szProtocol, NameSpx );
        break;

    case NSPROTO_SPXII:
        info->dwServiceFlags1 = XP1_IFS_HANDLES | XP1_GRACEFUL_CLOSE |
                                XP1_PSEUDO_STREAM | XP1_MESSAGE_ORIENTED |
                                XP1_GUARANTEED_ORDER | XP1_GUARANTEED_DELIVERY;
        memcpy( &info->ProviderId, &ProviderIdSPX, sizeof(GUID) );
        info->dwCatalogEntryId = 0x409;
        info->ProtocolChain.ChainLen = 1;
        info->iVersion = 2;
        info->iAddressFamily = WS_AF_IPX;
        info->iMaxSockAddr = 0x10;
        info->iMinSockAddr = 0x0e;
        info->iSocketType = 5;
        info->dwMessageSize = 0xffffffff;
        strcpy( info->szProtocol, NameSpxII );
        break;

    default:
        if ((protocol == ISOPROTO_TP4) || (protocol == NSPROTO_SPX))
            FIXME("Protocol <%s> not implemented\n",
                  (protocol == ISOPROTO_TP4) ? "ISOPROTO_TP4" : "NSPROTO_SPX");
        else
            FIXME("unknown Protocol <0x%08x>\n", protocol);
        return SOCKET_ERROR;
    }
    return 1;
}

/*****************************************************************************
 *          WSAEnumProtocolsA       [WS2_32.@]
 *
 *    see function WSAEnumProtocolsW
 */
INT WINAPI WSAEnumProtocolsA( LPINT protocols, LPWSAPROTOCOL_INFOA buffer, LPDWORD len )
{
    INT i = 0;
    DWORD size = 0;
    INT local[] = { WS_IPPROTO_TCP, WS_IPPROTO_UDP, NSPROTO_IPX, NSPROTO_SPX, NSPROTO_SPXII, 0 };

    if (!buffer)
        return SOCKET_ERROR;

    if (!protocols) protocols = local;

    while (protocols[i]) i++;

    size = i * sizeof(WSAPROTOCOL_INFOA);

    if (*len < size)
    {
        *len = size;
        return SOCKET_ERROR;
    }

    for (i = 0; protocols[i]; i++)
    {
        if (WINSOCK_EnterSingleProtocolA( protocols[i], &buffer[i] ) == SOCKET_ERROR)
            return i;
    }
    return i;
}

/*****************************************************************************
 *          WSAEnumProtocolsW       [WS2_32.@]
 *
 * Retrieves information about specified set of active network protocols.
 *
 * PARAMS
 *  protocols [I]   Pointer to null-terminated array of protocol id's. NULL
 *                  retrieves information on all available protocols.
 *  buffer    [I]   Pointer to a buffer to be filled with WSAPROTOCOL_INFO
 *                  structures.
 *  len       [I/O] Pointer to a variable specifying buffer size. On output
 *                  the variable holds the number of bytes needed when the
 *                  specified size is too small.
 *
 * RETURNS
 *  Success: number of WSAPROTOCOL_INFO structures in buffer.
 *  Failure: SOCKET_ERROR
 *
 * NOTES
 *  NT4SP5 does not return SPX if protocols == NULL
 *
 * BUGS
 *  - NT4SP5 returns in addition these list of NETBIOS protocols
 *    (address family 17), each entry two times one for socket type 2 and 5
 *
 *    iProtocol   szProtocol
 *    0x80000000  \Device\NwlnkNb
 *    0xfffffffa  \Device\NetBT_CBENT7
 *    0xfffffffb  \Device\Nbf_CBENT7
 *    0xfffffffc  \Device\NetBT_NdisWan5
 *    0xfffffffd  \Device\NetBT_El9202
 *    0xfffffffe  \Device\Nbf_El9202
 *    0xffffffff  \Device\Nbf_NdisWan4
 *
 *  - there is no check that the operating system supports the returned
 *    protocols
 */
INT WINAPI WSAEnumProtocolsW( LPINT protocols, LPWSAPROTOCOL_INFOW buffer, LPDWORD len )
{
    INT i = 0;
    DWORD size = 0;
    INT local[] = { WS_IPPROTO_TCP, WS_IPPROTO_UDP, NSPROTO_IPX, NSPROTO_SPX, NSPROTO_SPXII, 0 };

    if (!buffer)
        return SOCKET_ERROR;

    if (!protocols) protocols = local;

    while (protocols[i]) i++;

    size = i * sizeof(WSAPROTOCOL_INFOW);

    if (*len < size)
    {
        *len = size;
        return SOCKET_ERROR;
    }

    for (i = 0; protocols[i]; i++)
    {
        if (WINSOCK_EnterSingleProtocolW( protocols[i], &buffer[i] ) == SOCKET_ERROR)
            return i;
    }
    return i;
}

/*****************************************************************************
 *          WSCEnumProtocols        [WS2_32.@]
 *
 * PARAMS
 *  protocols [I]   Null-terminated array of iProtocol values.
 *  buffer    [O]   Buffer of WSAPROTOCOL_INFOW structures.
 *  len       [I/O] Size of buffer on input/output.
 *  errno     [O]   Error code.
 *
 * RETURNS
 *  Success: number of protocols to be reported on.
 *  Failure: SOCKET_ERROR. error is in errno.
 *
 * BUGS
 *  Doesn't supply info on layered protocols.
 *
 */
INT WINAPI WSCEnumProtocols( LPINT protocols, LPWSAPROTOCOL_INFOW buffer, LPDWORD len, LPINT errno )
{
    INT ret = WSAEnumProtocolsW( protocols, buffer, len );

    if (ret == SOCKET_ERROR) *errno = WSAENOBUFS;

    return ret;
}
