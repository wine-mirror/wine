/*
 * Winsock 2 definitions
 *
 * FIXME!!!!
 */
 
#ifndef __WINSOCK2API__
#define __WINSOCK2API__

#include "winsock.h"

#define FD_MAX_EVENTS 10

#define FD_READ_BIT	0
#define FD_WRITE_BIT	1
#define FD_OOB_BIT	2
#define FD_ACCEPT_BIT	3
#define FD_CONNECT_BIT	4
#define FD_CLOSE_BIT	5

typedef struct _WSANETWORKEVENTS {
  long lNetworkEvents;
  int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

typedef HANDLE WSAEVENT;

#define WSACreateEvent() CreateEvent(NULL, TRUE, FALSE, NULL)
/* etc */

int WINAPI WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents);
int WINAPI WSAEventSelect(SOCKET s, WSAEVENT hEventObject, long lNetworkEvents);

#endif
