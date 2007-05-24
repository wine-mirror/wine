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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "mswsock.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mswsock);

/***********************************************************************
 *		AcceptEx (MSWSOCK.@)
 *
 * This function is used to accept a new connection, get the local and remote
 * address, and receive the initial block of data sent by the client.
 *
 * TODO
 *  This function is currently implemented as a stub.
 */

BOOL WINAPI AcceptEx(
	SOCKET sListenSocket, /* [in] Descriptor identifying a socket that 
                                 has already been called with the listen
                                 function */
	SOCKET sAcceptSocket, /* [in] Descriptor identifying a socket on 
                                 which to accept an incoming connection */
	PVOID lpOutputBuffer, /* [in]  Pointer to a buffer */
	DWORD dwReceiveDataLength, /* [in] Number of bytes in lpOutputBuffer
				      that will be used for actual receive data
				      at the beginning of the buffer */
	DWORD dwLocalAddressLength, /* [in] Number of bytes reserved for the
				       local address information */
	DWORD dwRemoteAddressLength, /* [in] Number of bytes reserved for the
					remote address information */
	LPDWORD lpdwBytesReceived, /* [out] Pointer to a DWORD that receives
				      the count of bytes received */
	LPOVERLAPPED lpOverlapped) /* [in] Specify in order to achieve an 
                                      overlapped (asynchronous) I/O 
                                      operation */
{
    FIXME("(listen=%ld, accept=%ld, %p, %d, %d, %d, %p, %p), not implemented\n",
	sListenSocket,sAcceptSocket,lpOutputBuffer,dwReceiveDataLength,
	dwLocalAddressLength,dwRemoteAddressLength,lpdwBytesReceived,lpOverlapped
    );
    return FALSE;
}

/***********************************************************************
 *		GetAcceptExSockaddrs (MSWSOCK.@)
 */
VOID WINAPI GetAcceptExSockaddrs(
	PVOID lpOutputBuffer, /* [in] Pointer to a buffer */
	DWORD dwReceiveDataLength, /* [in] Number of bytes in the buffer used
				      for receiving the first data */
	DWORD dwLocalAddressLength, /* [in] Number of bytes reserved for the
				       local address information */
	DWORD dwRemoteAddressLength, /* [in] Number of bytes reserved for the
					remote address information */
	struct sockaddr **LocalSockaddr, /* [out] Pointer to the sockaddr
					    structure that receives the local
					    address of the connection  */
	LPINT LocalSockaddrLength, /* [out] Size in bytes of the local
				      address */
	struct sockaddr **RemoteSockaddr, /* [out] Pointer to the sockaddr
					     structure that receives the remote
					     address of the connection */
	LPINT RemoteSockaddrLength) /* [out] Size in bytes of the remote address */
{
    FIXME("not implemented\n");
}

/***********************************************************************
 *		TransmitFile (MSWSOCK.@)
 *
 * This function is used to transmit a file over socket.
 *
 * TODO
 *  This function is currently implemented as a stub.
 */

BOOL WINAPI TransmitFile(
        SOCKET hSocket, /* [in] Handle to a connected socket */
	HANDLE hFile,   /* [in] Handle to the open file that should be
                           transmited */
	DWORD nNumberOfBytesToWrite, /* [in] Number of file bytes to 
                                        transmit */
	DWORD nNumberOfBytesPerSend, /* [in] Size in bytes of each block of
                                         data sent in each send operation */
	LPOVERLAPPED lpOverlapped, /* [in] Specify in order to achieve an 
                                      overlapped (asynchronous) I/O 
                                      operation */
	LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers, 
		/* [in] Contains pointers to data to send before and after
                   the file data is sent */
	DWORD dwFlags) /* [in] Flags */
{
    FIXME("not implemented\n");

    return FALSE;
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
