/* WINSOCK.H--definitions to be used with the WINSOCK.DLL
 *
 * This header file corresponds to version 1.1 of the Windows Sockets
 * specification.
 */

#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include "windows.h"

#ifndef WINELIB
#pragma pack(1)                 /* tight alignment for the emulator */
#endif

/* Win16 socket-related types */

typedef UINT16		SOCKET16;

typedef struct ws_hostent
{
        SEGPTR  h_name;         /* official name of host */
        SEGPTR  h_aliases;      /* alias list */
        INT16   h_addrtype;     /* host address type */
        INT16   h_length;       /* length of address */
        SEGPTR  h_addr_list;    /* list of addresses from name server */
} _ws_hostent;

typedef struct ws_protoent
{
        SEGPTR  p_name;         /* official protocol name */
        SEGPTR  p_aliases;      /* alias list */
        INT16   p_proto;        /* protocol # */
} _ws_protoent;

typedef struct ws_servent 
{
        SEGPTR  s_name;         /* official service name */
        SEGPTR  s_aliases;      /* alias list */
        INT16   s_port;         /* port # */
        SEGPTR  s_proto;        /* protocol to use */
} _ws_servent;

typedef struct ws_netent
{
        SEGPTR  n_name;         /* official name of net */
        SEGPTR  n_aliases;      /* alias list */
        INT16   n_addrtype;     /* net address type */
        INT32   n_net;          /* network # */
} _ws_netent;

typedef struct sockaddr		ws_sockaddr;

typedef struct
{
        UINT16    fd_count;               /* how many are SET? */
        SOCKET16  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} ws_fd_set;

/* ws_fd_set operations */

INT16   __WSAFDIsSet( SOCKET16, ws_fd_set * );

#define WS_FD_CLR(fd, set) do { \
    UINT16 __i; \
    for (__i = 0; __i < ((ws_fd_set*)(set))->fd_count ; __i++) \
    { \
        if (((ws_fd_set*)(set))->fd_array[__i] == fd) \
	{ \
            while (__i < ((ws_fd_set*)(set))->fd_count-1) \
	    { \
                ((ws_fd_set*)(set))->fd_array[__i] = \
                    ((ws_fd_set*)(set))->fd_array[__i+1]; \
                __i++; \
            } \
            ((ws_fd_set*)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)

#define WS_FD_SET(fd, set) do { \
    if (((ws_fd_set*)(set))->fd_count < FD_SETSIZE) \
        ((ws_fd_set*)(set))->fd_array[((ws_fd_set*)(set))->fd_count++]=(fd);\
} while(0)

#define WS_FD_ZERO(set) (((ws_fd_set*)(set))->fd_count=0)

#define WS_FD_ISSET(fd, set) __WSAFDIsSet((SOCKET16)(fd), (ws_fd_set*)(set))

/* 
 * Internet address (old style... should be updated) 
 */

typedef struct ws_addr_in
{
        union {
                struct { BYTE   s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { UINT16 s_w1,s_w2; } S_un_w;
                UINT32 S_addr;
        } S_un;
#define ws_addr  S_un.S_addr		/* can be used for most tcp & ip code */
#define ws_host  S_un.S_un_b.s_b2	/* host on imp */
#define ws_net   S_un.S_un_b.s_b1	/* network */
#define ws_imp   S_un.S_un_w.s_w2	/* imp */
#define ws_impno S_un.S_un_b.s_b4	/* imp # */
#define ws_lh    S_un.S_un_b.s_b3	/* logical host */
} _ws_in_addr;

typedef struct ws_sockaddr_in
{
        INT16		sin_family;
        UINT16 		sin_port;
        _ws_in_addr 	sin_addr;
        char    	sin_zero[8];
} _ws_sockaddr_in;

#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

typedef struct WSAData {
        WORD                    wVersion;
        WORD                    wHighVersion;
        char                    szDescription[WSADESCRIPTION_LEN+1];
        char                    szSystemStatus[WSASYS_STATUS_LEN+1];
        UINT16			iMaxSockets;
        UINT16			iMaxUdpDg;
        SEGPTR			lpVendorInfo;
} WSADATA, *LPWSADATA;

#ifndef WINELIB
#pragma pack(4)
#endif

/* ----------------------------------- no Win16 structure defs beyond this line! */

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned.
 */
#define INVALID_SOCKET  (SOCKET16)(~0)
#define SOCKET_ERROR              (-1)

/*
 * Types
 */
#define WS_SOCK_STREAM     1               /* stream socket */
#define WS_SOCK_DGRAM      2               /* datagram socket */
#define WS_SOCK_RAW        3               /* raw-protocol interface */
#define WS_SOCK_RDM        4               /* reliably-delivered message */
#define WS_SOCK_SEQPACKET  5               /* sequenced packet stream */

#define WS_SOL_SOCKET		(-1)
#define WS_IPPROTO_TCP		6

/*
 * Option flags per-socket.
 */
#define WS_SO_DEBUG        0x0001          /* turn on debugging info recording */
#define WS_SO_ACCEPTCONN   0x0002          /* socket has had listen() */
#define WS_SO_REUSEADDR    0x0004          /* allow local address reuse */
#define WS_SO_KEEPALIVE    0x0008          /* keep connections alive */
#define WS_SO_DONTROUTE    0x0010          /* just use interface addresses */
#define WS_SO_BROADCAST    0x0020          /* permit sending of broadcast msgs */
#define WS_SO_USELOOPBACK  0x0040          /* bypass hardware when possible */
#define WS_SO_LINGER       0x0080          /* linger on close if data present */
#define WS_SO_OOBINLINE    0x0100          /* leave received OOB data in line */

#define WS_SO_DONTLINGER   (UINT16)(~WS_SO_LINGER)

/*
 * Additional options.
 */
#define WS_SO_SNDBUF       0x1001          /* send buffer size */
#define WS_SO_RCVBUF       0x1002          /* receive buffer size */
#define WS_SO_SNDLOWAT     0x1003          /* send low-water mark */
#define WS_SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define WS_SO_SNDTIMEO     0x1005          /* send timeout */
#define WS_SO_RCVTIMEO     0x1006          /* receive timeout */
#define WS_SO_ERROR        0x1007          /* get error status and clear */
#define WS_SO_TYPE         0x1008          /* get socket type */

#define WS_IOCPARM_MASK    0x7f            /* parameters must be < 128 bytes */
#define WS_IOC_VOID        0x20000000      /* no parameters */
#define WS_IOC_OUT         0x40000000      /* copy out parameters */
#define WS_IOC_IN          0x80000000      /* copy in parameters */
#define WS_IOC_INOUT       (WS_IOC_IN|WS_IOC_OUT)
#define WS_IOR(x,y,t)      (WS_IOC_OUT|(((UINT32)sizeof(t)&WS_IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define WS_IOW(x,y,t)      (WS_IOC_IN|(((UINT32)sizeof(t)&WS_IOCPARM_MASK)<<16)|((x)<<8)|(y))

/*
 * Socket I/O flags (supported by spec 1.1)
 */

#define WS_FIONREAD    WS_IOR('f', 127, u_long)
#define WS_FIONBIO     WS_IOW('f', 126, u_long)

#define WS_SIOCATMARK  WS_IOR('s',  7, u_long)

/*
 * Maximum queue length specifiable by listen.
 */
#ifdef SOMAXCONN
#undef SOMAXCONN
#endif
#define SOMAXCONN       5

#ifndef MSG_DONTROUTE
#define MSG_DONTROUTE   0x4             /* send without using routing tables */
#endif
#define MSG_MAXIOVLEN   16

/*
 * Define constant based on rfc883, used by gethostbyxxxx() calls.
 */
#define MAXGETHOSTSTRUCT        1024

/*
 * Define flags to be used with the WSAAsyncSelect() call.
 */
#define WS_FD_READ         0x01
#define WS_FD_WRITE        0x02
#define WS_FD_OOB          0x04
#define WS_FD_ACCEPT       0x08
#define WS_FD_CONNECT      0x10
#define WS_FD_CLOSE        0x20

#define WS_FD_NONBLOCK	   0x10000000	/* internal per-socket flags */
#define WS_FD_INACTIVE	   0x20000000
#define WS_FD_CONNECTED	   0x40000000
#define WS_FD_INTERNAL	   0xFFFF0000

/*
 * All Windows Sockets error constants are biased by WSABASEERR from
 * the "normal"
 */
#define WSABASEERR              10000
/*
 * Windows Sockets definitions of regular Microsoft C error constants
 */
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

/*
 * Windows Sockets definitions of regular Berkeley error constants
 */
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

/*
 * Extended Windows Sockets error constant definitions
 */
#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)
#define WSANOTINITIALISED       (WSABASEERR+93)

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (when using the resolver). Note that these errors are
 * retrieved via WSAGetLastError() and must therefore follow
 * the rules for avoiding clashes with error numbers from
 * specific implementations or language run-time systems.
 * For this reason the codes are based at WSABASEERR+1001.
 * Note also that [WSA]NO_ADDRESS is defined only for
 * compatibility purposes.
 */

/* #define h_errno         WSAGetLastError() */

/* Authoritative Answer: Host not found */
#define WSAHOST_NOT_FOUND       (WSABASEERR+1001)

/* Non-Authoritative: Host not found, or SERVERFAIL */
#define WSATRY_AGAIN            (WSABASEERR+1002)

/* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define WSANO_RECOVERY          (WSABASEERR+1003)

/* Valid name, no data record of requested type */
#define WSANO_DATA              (WSABASEERR+1004)

/* no address, look for MX record */
#define WSANO_ADDRESS           WSANO_DATA

/* Socket function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* Microsoft Windows Extension function prototypes */

INT16 	  WSAStartup(UINT16 wVersionRequired, LPWSADATA lpWSAData);
INT16 	  WSACleanup(void);
void 	  WSASetLastError(INT16 iError);
INT16 	  WSAGetLastError(void);
BOOL16 	  WSAIsBlocking(void);
INT16 	  WSAUnhookBlockingHook(void);
FARPROC16 WSASetBlockingHook(FARPROC16 lpBlockFunc);
INT16 	  WSACancelBlockingCall(void);
HANDLE16  WSAAsyncGetServByName(HWND16 hWnd, UINT16 wMsg,
                                LPCSTR name, LPCSTR proto,
                                SEGPTR buf, INT16 buflen);

HANDLE16  WSAAsyncGetServByPort(HWND16 hWnd, UINT16 wMsg, INT16 port,
                                LPCSTR proto, SEGPTR buf, INT16 buflen);

HANDLE16  WSAAsyncGetProtoByName(HWND16 hWnd, UINT16 wMsg,
                                 LPCSTR name, SEGPTR buf, INT16 buflen);

HANDLE16  WSAAsyncGetProtoByNumber(HWND16 hWnd, UINT16 wMsg,
                                   INT16 number, SEGPTR buf, INT16 buflen);

HANDLE16  WSAAsyncGetHostByName(HWND16 hWnd, UINT16 wMsg,
                                LPCSTR name, SEGPTR buf, INT16 buflen);

HANDLE16  WSAAsyncGetHostByAddr(HWND16 hWnd, UINT16 wMsg, LPCSTR addr, INT16 len,
                                INT16 type, SEGPTR buf, INT16 buflen);

INT16 	  WSACancelAsyncRequest(HANDLE16 hAsyncTaskHandle);
INT16     WSAAsyncSelect(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, UINT32 lEvent);

#ifdef __cplusplus
}
#endif

/* Microsoft Windows Extended data types */
typedef struct sockaddr SOCKADDR, *PSOCKADDR, *LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN, *LPSOCKADDR_IN;
typedef struct linger LINGER, *PLINGER, *LPLINGER;
typedef struct in_addr IN_ADDR, *PIN_ADDR, *LPIN_ADDR;
typedef struct fd_set FD_SET, *PFD_SET, *LPFD_SET;
typedef struct hostent HOSTENT, *PHOSTENT, *LPHOSTENT;
typedef struct servent SERVENT, *PSERVENT, *LPSERVENT;
typedef struct protoent PROTOENT, *PPROTOENT, *LPPROTOENT;
typedef struct timeval TIMEVAL, *PTIMEVAL, *LPTIMEVAL;

/*
 * Windows message parameter composition and decomposition
 * macros.
 *
 * WSAMAKEASYNCREPLY is intended for use by the Windows Sockets implementation
 * when constructing the response to a WSAAsyncGetXByY() routine.
 */
#define WSAMAKEASYNCREPLY(buflen,error)     MAKELONG(buflen,error)
/*
 * WSAMAKESELECTREPLY is intended for use by the Windows Sockets implementation
 * when constructing the response to WSAAsyncSelect().
 */
#define WSAMAKESELECTREPLY(event,error)     MAKELONG(event,error)
/*
 * WSAGETASYNCBUFLEN is intended for use by the Windows Sockets application
 * to extract the buffer length from the lParam in the response
 * to a WSAGetXByY().
 */
#define WSAGETASYNCBUFLEN(lParam)           LOWORD(lParam)
/*
 * WSAGETASYNCERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAGetXByY().
 */
#define WSAGETASYNCERROR(lParam)            HIWORD(lParam)
/*
 * WSAGETSELECTEVENT is intended for use by the Windows Sockets application
 * to extract the event code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTEVENT(lParam)           LOWORD(lParam)
/*
 * WSAGETSELECTERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTERROR(lParam)           HIWORD(lParam)

/* ----------------------------------- internal structures */

#define WS_DUP_NATIVE           0x0
#define WS_DUP_OFFSET           0x2
#define WS_DUP_SEGPTR           0x4

#define AOP_IO                  0x0000001	/* aop_control paramaters */

#define AOP_CONTROL_REMOVE      0x0000000	/* aop_control return values */
#define AOP_CONTROL_KEEP        0x0000001

typedef struct  __aop
{
  /* AOp header */

  struct __aop *next, *prev;
  int           fd[2];				/* pipe */
  int   (*aop_control)(struct __aop*, int);	/* SIGIO handler */
  pid_t         pid;			/* child process pid */

  /* custom data */

  HWND16        hWnd;
  UINT16        uMsg;

  unsigned	flags;
  SEGPTR	buffer_base;
  int           buflen;
  char*         init;			/* parameter data - length is in the async_ctl */
} ws_async_op;

#define WSMSG_ASYNC_SELECT      0x0000001
#define WSMSG_ASYNC_HOSTBYNAME  0x0000010
#define WSMSG_ASYNC_HOSTBYADDR  0x0000020
#define WSMSG_ASYNC_PROTOBYNAME 0x0000100
#define WSMSG_ASYNC_PROTOBYNUM  0x0000200
#define WSMSG_ASYNC_SERVBYNAME  0x0001000
#define WSMSG_ASYNC_SERVBYPORT  0x0002000

#define MTYPE_PARENT            0x50505357
#define MTYPE_CLIENT            0x50435357

#pragma pack(1)
typedef struct
{
  long          mtype;          /* WSMSG_... */

  UINT32        lParam;
  UINT16        wParam;         /* socket handle */
} ipc_packet;

#define MTYPE_PARENT_SIZE \
        (sizeof(UINT32))
#define MTYPE_CLIENT_SIZE \
        (sizeof(ipc_packet) - sizeof(long))
#pragma pack(4)

typedef struct
{
  int                   fd;
  unsigned              flags;
  ws_async_op*          p_aop;
} ws_socket;

typedef struct
{
  ws_async_op*  ws_aop;         /* init'ed for getXbyY */
  ws_socket*    ws_sock;        /* init'ed for AsyncSelect */
  int           lEvent;
  int           lLength;
  char*		buffer;
  ipc_packet    ip;
} ws_async_ctl;

#define WS_MAX_SOCKETS_PER_THREAD       16
#define WS_MAX_UDP_DATAGRAM             1024

#define WSI_BLOCKINGCALL	0x00000001	/* per-thread info flags */

typedef struct __WSINFO
{
  struct __WSINFO*      prev,*next;

  unsigned		flags;
  int			errno;
  int			num_startup;
  int                   num_async_rq;
  int                   last_free;
  ws_socket             sock[WS_MAX_SOCKETS_PER_THREAD];
  int			buflen;
  char*			buffer;
  FARPROC16		blocking_hook;
  HTASK16               tid;    /* owning thread id - better switch
                                 * to TLS when it gets fixed */
} WSINFO, *LPWSINFO;

int WS_dup_he(LPWSINFO pwsi, struct hostent* p_he, int flag);
int WS_dup_pe(LPWSINFO pwsi, struct protoent* p_pe, int flag);
int WS_dup_se(LPWSINFO pwsi, struct servent* p_se, int flag);

void WS_do_async_gethost(LPWSINFO, unsigned);
void WS_do_async_getproto(LPWSINFO, unsigned);
void WS_do_async_getserv(LPWSINFO, unsigned);

int WINSOCK_async_io(int fd, int async);
int WINSOCK_unblock_io(int fd, int noblock);

int  WINSOCK_check_async_op(ws_async_op* p_aop);
void WINSOCK_link_async_op(ws_async_op* p_aop);
void WINSOCK_unlink_async_op(ws_async_op* p_aop);
void WINSOCK_cancel_async_op(HTASK16 tid);
void WINSOCK_do_async_select(void);

UINT16 wsaErrno(void);
UINT16 wsaHerrno(void);

#endif  /* _WINSOCKAPI_ */

