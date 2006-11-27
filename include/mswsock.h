/*
 * Copyright (C) the Wine project
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
#ifndef _MSWSOCK_
#define _MSWSOCK_

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef USE_WS_PREFIX
#define SO_CONNDATA        0x7000
#define SO_CONNOPT         0x7001
#define SO_DISCDATA        0x7002
#define SO_DISCOPT         0x7003
#define SO_CONNDATALEN     0x7004
#define SO_CONNOPTLEN      0x7005
#define SO_DISCDATALEN     0x7006
#define SO_DISCOPTLEN      0x7007
#else
#define WS_SO_CONNDATA     0x7000
#define WS_SO_CONNOPT      0x7001
#define WS_SO_DISCDATA     0x7002
#define WS_SO_DISCOPT      0x7003
#define WS_SO_CONNDATALEN  0x7004
#define WS_SO_CONNOPTLEN   0x7005
#define WS_SO_DISCDATALEN  0x7006
#define WS_SO_DISCOPTLEN   0x7007
#endif

#ifndef USE_WS_PREFIX
#define SO_OPENTYPE     0x7008
#else
#define WS_SO_OPENTYPE  0x7008
#endif

#ifndef USE_WS_PREFIX
#define SO_SYNCHRONOUS_ALERT       0x10
#define SO_SYNCHRONOUS_NONALERT    0x20
#else
#define WS_SO_SYNCHRONOUS_ALERT    0x10
#define WS_SO_SYNCHRONOUS_NONALERT 0x20
#endif

#ifndef USE_WS_PREFIX
#define SO_MAXDG                      0x7009
#define SO_MAXPATHDG                  0x700A
#define SO_UPDATE_ACCEPT_CONTEXT      0x700B
#define SO_CONNECT_TIME               0x700C
#define SO_UPDATE_CONNECT_CONTEXT     0x7010
#else
#define WS_SO_MAXDG                   0x7009
#define WS_SO_MAXPATHDG               0x700A
#define WS_SO_UPDATE_ACCEPT_CONTEXT   0x700B
#define WS_SO_CONNECT_TIME            0x700C
#define WS_SO_UPDATE_CONNECT_CONTEXT  0x7010
#endif

#ifndef USE_WS_PREFIX
#define TCP_BSDURGENT              0x7000
#else
#define WS_TCP_BSDURGENT              0x7000
#endif

#ifndef USE_WS_PREFIX
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#else
#define WS_SIO_UDP_CONNRESET _WSAIOW(WS_IOC_VENDOR,12)
#endif

#define DE_REUSE_SOCKET TF_REUSE_SOCKET

#ifndef USE_WS_PREFIX
#define MSG_TRUNC   0x0100
#define MSG_CTRUNC  0x0200
#define MSG_BCAST   0x0400
#define MSG_MCAST   0x0800
#else
#define WS_MSG_TRUNC   0x0100
#define WS_MSG_CTRUNC  0x0200
#define WS_MSG_BCAST   0x0400
#define WS_MSG_MCAST   0x0800
#endif

#define TF_DISCONNECT          0x01
#define TF_REUSE_SOCKET        0x02
#define TF_WRITE_BEHIND        0x04
#define TF_USE_DEFAULT_WORKER  0x00
#define TF_USE_SYSTEM_THREAD   0x10
#define TF_USE_KERNEL_APC      0x20

#define TP_DISCONNECT           TF_DISCONNECT
#define TP_REUSE_SOCKET         TF_REUSE_SOCKET
#define TP_USE_DEFAULT_WORKER   TF_USE_DEFAULT_WORKER
#define TP_USE_SYSTEM_THREAD    TF_USE_SYSTEM_THREAD
#define TP_USE_KERNEL_APC       TF_USE_KERNEL_APC

#define TP_ELEMENT_MEMORY   1
#define TP_ELEMENT_FILE     2
#define TP_ELEMENT_EOP      4

#define WSAID_ACCEPTEX \
	{0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_CONNECTEX \
	{0x25a207b9,0xddf3,0x4660,{0x8e,0xe9,0x76,0xe5,0x8c,0x74,0x06,0x3e}}
#define WSAID_DISCONNECTEX \
	{0x7fda2e11,0x8630,0x436f,{0xa0, 0x31, 0xf5, 0x36, 0xa6, 0xee, 0xc1, 0x57}}
#define WSAID_GETACCEPTEXSOCKADDRS \
	{0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITFILE \
	{0xb5367df0,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}
#define WSAID_TRANSMITPACKETS \
	{0xd9689da0,0x1f90,0x11d3,{0x99,0x71,0x00,0xc0,0x4f,0x68,0xc8,0x76}}
#define WSAID_WSARECVMSG \
	{0xf689d7c8,0x6f1f,0x436b,{0x8a,0x53,0xe5,0x4f,0xe3,0x51,0xc3,0x22}}

typedef struct _TRANSMIT_FILE_BUFFERS {
    LPVOID  Head;
    DWORD   HeadLength;
    LPVOID  Tail;
    DWORD   TailLength;
} TRANSMIT_FILE_BUFFERS, *PTRANSMIT_FILE_BUFFERS, *LPTRANSMIT_FILE_BUFFERS;

typedef struct _TRANSMIT_PACKETS_ELEMENT {
    ULONG  dwElFlags;
    ULONG  cLength;
    union {
      struct {
	LARGE_INTEGER  nFileOffset;
	HANDLE         hFile;
      } DUMMYSTRUCTNAME;
      PVOID  pBuffer;
    } DUMMYUNIONNAME;
} TRANSMIT_PACKETS_ELEMENT, *PTRANSMIT_PACKETS_ELEMENT, *LPTRANSMIT_PACKETS_ELEMENT;

typedef struct _WSAMSG {
    LPSOCKADDR  name;
    INT         namelen;
    LPWSABUF    lpBuffers;
    DWORD       dwBufferCount;
    WSABUF      Control;
    DWORD       dwFlags;
} WSAMSG, *PWSAMSG, *LPWSAMSG;

typedef struct _WSACMSGHDR {
    SIZE_T      cmsg_len;
    INT         cmsg_level;
    INT         cmsg_type;
    /* followed by UCHAR cmsg_data[] */
} WSACMSGHDR, *PWSACMSGHDR, *LPWSACMSGHDR;

typedef BOOL (WINAPI * LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI * LPFN_CONNECTEX)(SOCKET, const struct sockaddr *, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (WINAPI * LPFN_DISCONNECTEX)(SOCKET, LPOVERLAPPED, DWORD, DWORD);
typedef VOID (WINAPI * LPFN_GETACCEPTEXSOCKADDRS)(PVOID, DWORD, DWORD, DWORD, struct sockaddr **, LPINT, struct sockaddr **, LPINT);
typedef BOOL (WINAPI * LPFN_TRANSMITFILE)(SOCKET, HANDLE, DWORD, DWORD, LPOVERLAPPED, LPTRANSMIT_FILE_BUFFERS, DWORD);
typedef BOOL (WINAPI * LPFN_TRANSMITPACKETS)(SOCKET, LPTRANSMIT_PACKETS_ELEMENT, DWORD, DWORD, LPOVERLAPPED, DWORD);
typedef INT  (WINAPI * LPFN_WSARECVMSG)(SOCKET, LPWSAMSG, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

BOOL WINAPI AcceptEx(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
VOID WINAPI GetAcceptExSockaddrs(PVOID, DWORD, DWORD, DWORD, struct sockaddr **, LPINT, struct sockaddr **, LPINT);
BOOL WINAPI TransmitFile(SOCKET, HANDLE, DWORD, DWORD, LPOVERLAPPED, LPTRANSMIT_FILE_BUFFERS, DWORD);
INT  WINAPI WSARecvEx(SOCKET, char *, INT, INT *);

#ifdef __cplusplus
}
#endif

#endif /* _MSWSOCK_ */
