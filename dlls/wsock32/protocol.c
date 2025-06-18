/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 2001 Stefan Leichter
 * Copyright (C) 2008 Hans Leidekker
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
#include "winsock2.h"
#include "nspapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

/*****************************************************************************
 *          inet_network       [WSOCK32.1100]
 */
UINT WINAPI WSOCK32_inet_network(const char *cp)
{
    return ntohl( inet_addr( cp ) );
}

/*****************************************************************************
 *          getnetbyname       [WSOCK32.1101]
 */
struct netent * WINAPI WSOCK32_getnetbyname(const char *name)
{
    ERR( "%s: not supported\n", debugstr_a(name) );
    return NULL;
}

static DWORD map_service(DWORD wsaflags)
{
    DWORD flags = 0;

    if (wsaflags & XP1_CONNECTIONLESS)      flags |= XP_CONNECTIONLESS;
    if (wsaflags & XP1_GUARANTEED_DELIVERY) flags |= XP_GUARANTEED_DELIVERY;
    if (wsaflags & XP1_GUARANTEED_ORDER)    flags |= XP_GUARANTEED_ORDER;
    if (wsaflags & XP1_MESSAGE_ORIENTED)    flags |= XP_MESSAGE_ORIENTED;
    if (wsaflags & XP1_PSEUDO_STREAM)       flags |= XP_PSEUDO_STREAM;
    if (wsaflags & XP1_GRACEFUL_CLOSE)      flags |= XP_GRACEFUL_CLOSE;
    if (wsaflags & XP1_EXPEDITED_DATA)      flags |= XP_EXPEDITED_DATA;
    if (wsaflags & XP1_CONNECT_DATA)        flags |= XP_CONNECT_DATA;
    if (wsaflags & XP1_DISCONNECT_DATA)     flags |= XP_DISCONNECT_DATA;
    if (wsaflags & XP1_SUPPORT_BROADCAST)   flags |= XP_SUPPORTS_BROADCAST;
    if (wsaflags & XP1_SUPPORT_MULTIPOINT)  flags |= XP_SUPPORTS_MULTICAST;
    if (wsaflags & XP1_QOS_SUPPORTED)       flags |= XP_BANDWIDTH_ALLOCATION;
    if (wsaflags & XP1_PARTIAL_MESSAGE)     flags |= XP_FRAGMENTATION;
    return flags;
}

/*****************************************************************************
 *          EnumProtocolsA       [WSOCK32.1111]
 */
INT WINAPI EnumProtocolsA(LPINT protocols, LPVOID buffer, LPDWORD buflen)
{
    INT ret;
    DWORD size, string_size = WSAPROTOCOL_LEN + 1;

    TRACE("%p, %p, %p\n", protocols, buffer, buflen);

    if (!buflen) return SOCKET_ERROR;

    size = 0;
    ret = WSAEnumProtocolsA(protocols, NULL, &size);

    if (ret == SOCKET_ERROR && WSAGetLastError() == WSAENOBUFS)
    {
        DWORD num_protocols = size / sizeof(WSAPROTOCOL_INFOA);
        if (*buflen < num_protocols * (sizeof(PROTOCOL_INFOA) + string_size))
        {
            *buflen = num_protocols * (sizeof(PROTOCOL_INFOA) + string_size);
            return SOCKET_ERROR;
        }
        if (buffer)
        {
            WSAPROTOCOL_INFOA *wsabuf;
            PROTOCOL_INFOA *pi = buffer;
            unsigned int string_offset;
            INT i;

            if (!(wsabuf = HeapAlloc(GetProcessHeap(), 0, size))) return SOCKET_ERROR;

            ret = WSAEnumProtocolsA(protocols, wsabuf, &size);
            string_offset = ret * sizeof(PROTOCOL_INFOA);

            for (i = 0; i < ret; i++)
            {
                pi[i].dwServiceFlags = map_service(wsabuf[i].dwServiceFlags1);
                pi[i].iAddressFamily = wsabuf[i].iAddressFamily;
                pi[i].iMaxSockAddr   = wsabuf[i].iMaxSockAddr;
                pi[i].iMinSockAddr   = wsabuf[i].iMinSockAddr;
                pi[i].iSocketType    = wsabuf[i].iSocketType;
                pi[i].iProtocol      = wsabuf[i].iProtocol;
                pi[i].dwMessageSize  = wsabuf[i].dwMessageSize;

                memcpy((char *)buffer + string_offset, wsabuf[i].szProtocol, string_size);
                pi[i].lpProtocol = (char *)buffer + string_offset;
                string_offset += string_size;
            }
            HeapFree(GetProcessHeap(), 0, wsabuf);
        }
    }
    return ret;
}

/*****************************************************************************
 *          EnumProtocolsW       [WSOCK32.1112]
 */
INT WINAPI EnumProtocolsW(LPINT protocols, LPVOID buffer, LPDWORD buflen)
{
    INT ret;
    DWORD size, string_size = (WSAPROTOCOL_LEN + 1) * sizeof(WCHAR);

    TRACE("%p, %p, %p\n", protocols, buffer, buflen);

    if (!buflen) return SOCKET_ERROR;

    size = 0;
    ret = WSAEnumProtocolsW(protocols, NULL, &size);

    if (ret == SOCKET_ERROR && WSAGetLastError() == WSAENOBUFS)
    {
        DWORD num_protocols = size / sizeof(WSAPROTOCOL_INFOW);
        if (*buflen < num_protocols * (sizeof(PROTOCOL_INFOW) + string_size))
        {
            *buflen = num_protocols * (sizeof(PROTOCOL_INFOW) + string_size);
            return SOCKET_ERROR;
        }
        if (buffer)
        {
            WSAPROTOCOL_INFOW *wsabuf;
            PROTOCOL_INFOW *pi = buffer;
            unsigned int string_offset;
            INT i;

            if (!(wsabuf = HeapAlloc(GetProcessHeap(), 0, size))) return SOCKET_ERROR;

            ret = WSAEnumProtocolsW(protocols, wsabuf, &size);
            string_offset = ret * sizeof(PROTOCOL_INFOW);

            for (i = 0; i < ret; i++)
            {
                pi[i].dwServiceFlags = map_service(wsabuf[i].dwServiceFlags1);
                pi[i].iAddressFamily = wsabuf[i].iAddressFamily;
                pi[i].iMaxSockAddr   = wsabuf[i].iMaxSockAddr;
                pi[i].iMinSockAddr   = wsabuf[i].iMinSockAddr;
                pi[i].iSocketType    = wsabuf[i].iSocketType;
                pi[i].iProtocol      = wsabuf[i].iProtocol;
                pi[i].dwMessageSize  = wsabuf[i].dwMessageSize;

                memcpy((char *)buffer + string_offset, wsabuf[i].szProtocol, string_size);
                pi[i].lpProtocol = (WCHAR *)((char *)buffer + string_offset);
                string_offset += string_size;
            }
            HeapFree(GetProcessHeap(), 0, wsabuf);
        }
    }
    return ret;
}
