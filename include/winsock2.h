/*
 * Winsock 2 definitions - used for ws2_32.dll
 *
 * FIXME: Still missing required Winsock 2 definitions.
 */
 
#ifndef __WINSOCK2API__
#define __WINSOCK2API__

#include "winsock.h"
#include "wtypes.h"

#define FD_MAX_EVENTS   10
#define FD_READ_BIT	0
#define FD_WRITE_BIT	1
#define FD_OOB_BIT	2
#define FD_ACCEPT_BIT	3
#define FD_CONNECT_BIT	4
#define FD_CLOSE_BIT	5

/*
 * Constants for WSAIoctl()
 */
#define IOC_UNIX                      0x00000000
#define IOC_WS2                       0x08000000
#define IOC_PROTOCOL                  0x10000000
#define IOC_VENDOR                    0x18000000
#define _WSAIO(x,y)                   (IOC_VOID|(x)|(y))
#define _WSAIOR(x,y)                  (IOC_OUT|(x)|(y))
#define _WSAIOW(x,y)                  (IOC_IN|(x)|(y))
#define _WSAIORW(x,y)                 (IOC_INOUT|(x)|(y))
#define SIO_ASSOCIATE_HANDLE          _WSAIOW(IOC_WS2,1)
#define SIO_ENABLE_CIRCULAR_QUEUEING  _WSAIO(IOC_WS2,2)
#define SIO_FIND_ROUTE                _WSAIOR(IOC_WS2,3)
#define SIO_FLUSH                     _WSAIO(IOC_WS2,4)
#define SIO_GET_BROADCAST_ADDRESS     _WSAIOR(IOC_WS2,5)
#define SIO_GET_EXTENSION_FUNCTION_POINTER  _WSAIORW(IOC_WS2,6)
#define SIO_GET_QOS                   _WSAIORW(IOC_WS2,7)
#define SIO_GET_GROUP_QOS             _WSAIORW(IOC_WS2,8)
#define SIO_MULTIPOINT_LOOPBACK       _WSAIOW(IOC_WS2,9)
#define SIO_MULTICAST_SCOPE           _WSAIOW(IOC_WS2,10)
#define SIO_SET_QOS                   _WSAIOW(IOC_WS2,11)
#define SIO_SET_GROUP_QOS             _WSAIOW(IOC_WS2,12)
#define SIO_TRANSLATE_HANDLE          _WSAIORW(IOC_WS2,13)
#define SIO_ROUTING_INTERFACE_QUERY   _WSAIORW(IOC_WS2,20)
#define SIO_ROUTING_INTERFACE_CHANGE  _WSAIOW(IOC_WS2,21)
#define SIO_ADDRESS_LIST_QUERY        _WSAIOR(IOC_WS2,22)
#define SIO_ADDRESS_LIST_CHANGE       _WSAIO(IOC_WS2,23)
#define SIO_QUERY_TARGET_PNP_HANDLE   _WSAIOR(IOC_W32,24)
#define SIO_GET_INTERFACE_LIST        WS_IOR ('t', 127, u_long)

/* Unfortunately the sockaddr_in6 structure doesn't
   seem to be defined in a standard place, even across 
   different Linux distributions.  Until IPv6 support settles
   down, let's do our own here so the sockaddr_gen 
   union is the correct size.*/
#ifdef s6_addr
#undef s6_addr
#endif
struct ws_in_addr6
{
   unsigned char s6_addr[16];   /* IPv6 address */
};
struct ws_sockaddr_in6
{
   short   sin6_family;            /* AF_INET6 */
   u_short sin6_port;              /* Transport level port number */
   u_long  sin6_flowinfo;          /* IPv6 flow information */
   struct  ws_in_addr6 sin6_addr;  /* IPv6 address */
};

typedef union sockaddr_gen
{
   struct sockaddr Address;
   struct ws_sockaddr_in  AddressIn;
   struct ws_sockaddr_in6 AddressIn6;
} sockaddr_gen;

/* Structure to keep interface specific information */
typedef struct _INTERFACE_INFO
{
   u_long        iiFlags;             /* Interface flags */
   sockaddr_gen  iiAddress;           /* Interface address */
   sockaddr_gen  iiBroadcastAddress;  /* Broadcast address */
   sockaddr_gen  iiNetmask;           /* Network mask */
} INTERFACE_INFO, * LPINTERFACE_INFO;

/* Possible flags for the  iiFlags - bitmask  */ 
#ifndef HAVE_NET_IF_H
#  define IFF_UP                0x00000001 /* Interface is up */
#  define IFF_BROADCAST         0x00000002 /* Broadcast is  supported */
#  define IFF_LOOPBACK          0x00000004 /* this is loopback interface */
#  define IFF_POINTTOPOINT      0x00000008 /* this is point-to-point interface */
#  define IFF_MULTICAST         0x00000010 /* multicast is supported */
#endif

#define MAX_PROTOCOL_CHAIN 7
#define BASE_PROTOCOL      1
#define LAYERED_PROTOCOL   0

typedef struct _WSAPROTOCOLCHAIN 
{
    int ChainLen;                                 /* the length of the chain,     */
                                                  /* length = 0 means layered protocol, */
                                                  /* length = 1 means base protocol, */
                                                  /* length > 1 means protocol chain */
    DWORD ChainEntries[MAX_PROTOCOL_CHAIN];       /* a list of dwCatalogEntryIds */
} WSAPROTOCOLCHAIN, * LPWSAPROTOCOLCHAIN;
#define WSAPROTOCOL_LEN  255

typedef struct _WSAPROTOCOL_INFOA 
{
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    int iVersion;
    int iAddressFamily;
    int iMaxSockAddr;
    int iMinSockAddr;
    int iSocketType;
    int iProtocol;
    int iProtocolMaxOffset;
    int iNetworkByteOrder;
    int iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    CHAR   szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOA, * LPWSAPROTOCOL_INFOA;

typedef struct _WSANETWORKEVENTS 
{
  long lNetworkEvents;
  int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

typedef struct _OVERLAPPED *  LPWSAOVERLAPPED;
typedef HANDLE WSAEVENT;
typedef unsigned int   GROUP;

typedef void 
(CALLBACK * LPWSAOVERLAPPED_COMPLETION_ROUTINE)
(
     DWORD dwError,
     DWORD cbTransferred,
     LPWSAOVERLAPPED lpOverlapped,
     DWORD dwFlags
);


/* Function declarations */
int WINAPI WSAEnumNetworkEvents(SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents);
int WINAPI WSAEventSelect(SOCKET s, WSAEVENT hEventObject, long lNetworkEvents);
WSAEVENT WINAPI WSACreateEvent(void);
BOOL WINAPI WSACloseEvent(WSAEVENT event);
SOCKET WINAPI WSASocketA(int af, int type, int protocol,
                         LPWSAPROTOCOL_INFOA lpProtocolInfo,
                         GROUP g, DWORD dwFlags);

#endif
