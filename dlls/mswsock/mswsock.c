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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "config.h"
#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mswsock);

/******************************************************************************
 * This structure is used with the TransmitFile() function.
 *
 */

typedef struct _TRANSMIT_FILE_BUFFERS {
  PVOID Head;
  DWORD HeadLength;
  PVOID Tail;
  DWORD TailLength;
} TRANSMIT_FILE_BUFFERS;
typedef TRANSMIT_FILE_BUFFERS* LPTRANSMIT_FILE_BUFFERS;


/******************************************************************************
 * TransmitFile (MSWSOCK.@)
 *
 * This function is used to transmit a file over socket.
 *
 * TODO
 *  This function is currently implemented as a stub.
 */

void WINAPI TransmitFile(SOCKET                  s,
                         HANDLE                  f,
                         DWORD                   size,
                         DWORD                   numpersend,
                         LPOVERLAPPED            overlapped,
                         LPTRANSMIT_FILE_BUFFERS trans,
                         DWORD                   flags)
{
  FIXME("not implemented\n");
}


/******************************************************************************
 * AcceptEx (MSWSOCK.@)
 *
 * This function is used to accept a new connection, get the local and remote
 * address, and receive the initial block of data sent by the client.
 *
 * TODO
 *  This function is currently implemented as a stub.
 */

void WINAPI AcceptEx(SOCKET       listener,
                     SOCKET       acceptor,
                     PVOID        oput,
                     DWORD        recvlen,
                     DWORD        locaddrlen,
                     DWORD        remaddrlen,
                     LPDWORD      bytesrecv,
                     LPOVERLAPPED overlapped)
{
  FIXME("not implemented\n");
}
