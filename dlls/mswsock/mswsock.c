/*
 * MSWSOCK specific functions
 *
 * Copyright (C) 2003 André Johansen
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
#include "mswsock.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mswsock);

static LPFN_ACCEPTEX acceptex_fn;
static LPFN_GETACCEPTEXSOCKADDRS acceptexsockaddrs_fn;
static LPFN_TRANSMITFILE transmitfile_fn;
static BOOL initialised;

/* Get pointers to the ws2_32 implementations.
 * NOTE: This assumes that ws2_32 contains only one implementation
 * of these functions, i.e. that you cannot get different functions
 * back by passing another socket in. If that ever changes, we'll need
 * to think about associating the functions with the socket and
 * exposing that information to this dll somehow.
 */
static void get_fn(SOCKET s, GUID* guid, FARPROC* fn)
{
    FARPROC func;
    DWORD len;
    if (!WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, guid, sizeof(*guid),
                  &func, sizeof(func), &len, NULL, NULL))
        *fn = func;
}

static void get_fn_pointers(SOCKET s)
{
    GUID acceptex_guid = WSAID_ACCEPTEX;
    GUID acceptexsockaddrs_guid = WSAID_GETACCEPTEXSOCKADDRS;
    GUID transmitfile_guid = WSAID_TRANSMITFILE;

    get_fn(s, &acceptex_guid, (FARPROC*)&acceptex_fn);
    get_fn(s, &acceptexsockaddrs_guid, (FARPROC*)&acceptexsockaddrs_fn);
    get_fn(s, &transmitfile_guid, (FARPROC*)&transmitfile_fn);
    initialised = TRUE;
}

/***********************************************************************
 *		AcceptEx (MSWSOCK.@)
 *
 * Accept a new connection, retrieving the connected addresses and initial data.
 *
 * listener       [I] Listening socket
 * acceptor       [I] Socket to accept on
 * dest           [O] Destination for initial data
 * dest_len       [I] Size of dest in bytes
 * local_addr_len [I] Number of bytes reserved in dest for local address
 * rem_addr_len   [I] Number of bytes reserved in dest for remote address
 * received       [O] Destination for number of bytes of initial data
 * overlapped     [I] For asynchronous execution
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE. Use WSAGetLastError() for details of the error.
 */
BOOL WINAPI AcceptEx(SOCKET listener, SOCKET acceptor, PVOID dest, DWORD dest_len,
                     DWORD local_addr_len, DWORD rem_addr_len, LPDWORD received,
                     LPOVERLAPPED overlapped)
{
    if (!initialised)
        get_fn_pointers(acceptor);

    if (!acceptex_fn)
        return FALSE;

    return acceptex_fn(listener, acceptor, dest, dest_len, local_addr_len,
                       rem_addr_len, received, overlapped);
}

/***********************************************************************
 *		GetAcceptExSockaddrs (MSWSOCK.@)
 *
 * Get information about an accepted socket.
 *
 * data           [O] Destination for the first block of data from AcceptEx()
 * data_len       [I] length of data in bytes
 * local_len      [I] Bytes reserved for local addrinfo
 * rem_len        [I] Bytes reserved for remote addrinfo
 * local_addr     [O] Destination for local sockaddr
 * local_addr_len [I] Size of local_addr
 * rem_addr       [O] Destination for remote sockaddr
 * rem_addr_len   [I] Size of rem_addr
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI GetAcceptExSockaddrs(PVOID data, DWORD data_len, DWORD local_len, DWORD rem_len,
                                 struct sockaddr **local_addr, LPINT local_addr_len,
                                 struct sockaddr **rem_addr, LPINT rem_addr_len)
{
    if (acceptexsockaddrs_fn)
        acceptexsockaddrs_fn(data, data_len, local_len, rem_len,
                             local_addr, local_addr_len, rem_addr, rem_addr_len);
}


/***********************************************************************
 *		TransmitFile (MSWSOCK.@)
 *
 * Transmit a file over a socket.
 *
 * PARAMS
 * s          [I] Handle to a connected socket
 * file       [I] Opened file handle for file to send
 * total_len  [I] Total number of file bytes to send
 * chunk_len  [I] Chunk size to send file in (0=default)
 * overlapped [I] For asynchronous operation
 * buffers    [I] Head/tail data, or NULL if none
 * flags      [I] TF_ Flags from mswsock.h
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE. Use WSAGetLastError() for details of the error.
 */
BOOL WINAPI TransmitFile(SOCKET s, HANDLE file, DWORD total_len,
                         DWORD chunk_len, LPOVERLAPPED overlapped,
                         LPTRANSMIT_FILE_BUFFERS buffers, DWORD flags)
{
    if (!initialised)
        get_fn_pointers(s);

    if (!transmitfile_fn)
        return FALSE;

    return transmitfile_fn(s, file, total_len, chunk_len, overlapped, buffers, flags);
}

/***********************************************************************
 *		WSARecvEx (MSWSOCK.@)
 */
INT WINAPI WSARecvEx(
	SOCKET s,   /* [in] Descriptor identifying a connected socket */
	char *buf,  /* [out] Buffer for the incoming data */
	INT len,    /* [in] Length of buf, in bytes */
        INT *flags) /* [in/out] Indicator specifying whether the message is
	               fully or partially received for datagram sockets */
{
    FIXME("not implemented\n");
    
    return SOCKET_ERROR;
}
