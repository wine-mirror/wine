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

#pragma pack(1)

/* Win16 socket-related types */

typedef UINT16		SOCKET16;
typedef UINT32		SOCKET32;

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
} ws_fd_set16;

typedef struct
{
        UINT32    fd_count;               /* how many are SET? */
        SOCKET32  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
} ws_fd_set32;

/* ws_fd_set operations */

INT16 WINAPI __WSAFDIsSet16( SOCKET16, ws_fd_set16 * );
INT32 WINAPI __WSAFDIsSet32( SOCKET32, ws_fd_set32 * );
#define __WSAFDIsSet WINELIB_NAME(__WSAFDIsSet);

#define __WS_FD_CLR(fd, set, cast) do { \
    UINT16 __i; \
    for (__i = 0; __i < ((cast*)(set))->fd_count ; __i++) \
    { \
        if (((cast*)(set))->fd_array[__i] == fd) \
	{ \
            while (__i < ((cast*)(set))->fd_count-1) \
	    { \
                ((cast*)(set))->fd_array[__i] = \
                    ((cast*)(set))->fd_array[__i+1]; \
                __i++; \
            } \
            ((cast*)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)
#define WS_FD_CLR16(fd, set)	__WS_FD_CLR((fd),(set), ws_fd_set16)
#define WS_FD_CLR32(fd, set)	__WS_FD_CLR((fd),(set), ws_fd_set32)
#define WS_FD_CLR 		WINELIB_NAME(WS_FD_CLR);

#define __WS_FD_SET(fd, set, cast) do { \
    if (((cast*)(set))->fd_count < FD_SETSIZE) \
        ((cast*)(set))->fd_array[((cast*)(set))->fd_count++]=(fd);\
} while(0)
#define WS_FD_SET16(fd, set)    __WS_FD_SET((fd),(set), ws_fd_set16)
#define WS_FD_SET32(fd, set)    __WS_FD_SET((fd),(set), ws_fd_set32)
#define WS_FD_SET		WINELIB_NAME(WS_FD_SET);

#define WS_FD_ZERO16(set) (((ws_fd_set16*)(set))->fd_count=0)
#define WS_FD_ZERO32(set) (((ws_fd_set32*)(set))->fd_count=0)
#define WS_FD_ZERO		WINELIB_NAME(WS_FD_ZERO);

#define WS_FD_ISSET16(fd, set) __WSAFDIsSet16((SOCKET16)(fd), (ws_fd_set16*)(set))
#define WS_FD_ISSET32(fd, set) __WSAFDIsSet32((SOCKET32)(fd), (ws_fd_set32*)(set))
#define WS_FD_ISSET		WINELIB_NAME(WS_FD_ISSET);

/* 
 * Internet address (old style... should be updated) 
 */

struct ws_in_addr
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
};

struct ws_sockaddr_in
{
        INT16		sin_family;
        UINT16 		sin_port;
        struct ws_in_addr sin_addr;
        BYTE    	sin_zero[8];
};

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

#pragma pack(4)

/* ----------------------------------- no Win16 structure defs beyond this line! */

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned.
 */
#define INVALID_SOCKET16 	   (~0)
#define INVALID_SOCKET32 	   (~0)
#define SOCKET_ERROR               (-1)

DECL_WINELIB_TYPE(INVALID_SOCKET);
DECL_WINELIB_TYPE(SOCKET);

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

#define WS_SO_DONTLINGER   (UINT32)(~WS_SO_LINGER)

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
#define WS_FD_READ         0x0001
#define WS_FD_WRITE        0x0002
#define WS_FD_OOB          0x0004
#define WS_FD_ACCEPT       0x0008
#define WS_FD_CONNECT      0x0010
#define WS_FD_CLOSE        0x0020

#define WS_FD_LISTENING	   0x10000000	/* internal per-socket flags */
#define WS_FD_INACTIVE	   0x20000000
#define WS_FD_CONNECTED	   0x40000000
#define WS_FD_RAW	   0x80000000
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

INT16     WINAPI WSAStartup16(UINT16 wVersionRequired, LPWSADATA lpWSAData);
INT32     WINAPI WSAStartup32(UINT32 wVersionRequired, LPWSADATA lpWSAData);
#define   WSAStartup WINELIB_NAME(WSAStartup);
void      WINAPI WSASetLastError16(INT16 iError);
void      WINAPI WSASetLastError32(INT32 iError);
#define   WSASetLastError WINELIB_NAME(WSASetLastError);
INT32     WINAPI WSACleanup(void);
INT32     WINAPI WSAGetLastError(void);
BOOL32    WINAPI WSAIsBlocking(void);
INT32     WINAPI WSACancelBlockingCall(void);
INT16     WINAPI WSAUnhookBlockingHook16(void);
INT32     WINAPI WSAUnhookBlockingHook32(void);
#define   WSAUnhookBlockingHook WINELIB_NAME(WSAUnhookBlockingHook)
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc);
FARPROC32 WINAPI WSASetBlockingHook32(FARPROC32 lpBlockFunc);
#define   WSASetBlockingHook WINELIB_NAME(WSASetBlockingHook)

HANDLE16  WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 wMsg, LPCSTR name, LPCSTR proto,
                                         SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetServByName32(HWND32 hWnd, UINT32 uMsg, LPCSTR name, LPCSTR proto,
					 LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetServByName WINELIB_NAME(WSAAsyncGetServByName)

HANDLE16  WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 wMsg, INT16 port,
                                         LPCSTR proto, SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetServByPort32(HWND32 hWnd, UINT32 uMsg, INT32 port,
					 LPCSTR proto, LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetServByPort WINELIB_NAME(WSAAsyncGetServByPort)

HANDLE16  WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 wMsg,
                                          LPCSTR name, SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetProtoByName32(HWND32 hWnd, UINT32 uMsg,
					  LPCSTR name, LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetProtoByByName WINELIB_NAME(WSAAsyncGetProtoByByName)

HANDLE16  WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd, UINT16 wMsg,
                                            INT16 number, SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetProtoByNumber32(HWND32 hWnd, UINT32 uMsg,
					    INT32 number, LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetProtoByNumber WINELIB_NAME(WSAAsyncGetProtoByNumber)

HANDLE16  WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 wMsg,
                                         LPCSTR name, SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetHostByName32(HWND32 hWnd, UINT32 uMsg,
					 LPCSTR name, LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetHostByName WINELIB_NAME(WSAAsyncGetHostByName)

HANDLE16  WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 wMsg, LPCSTR addr,
                              INT16 len, INT16 type, SEGPTR buf, INT16 buflen);
HANDLE32  WINAPI WSAAsyncGetHostByAddr32(HWND32 hWnd, UINT32 uMsg, LPCSTR addr,
			      INT32 len, INT32 type, LPSTR sbuf, INT32 buflen);
#define   WSAAsyncGetHostByAddr WINELIB_NAME(WSAAsyncGetHostByAddr)

INT16 	  WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle);
INT32     WINAPI WSACancelAsyncRequest32(HANDLE32 hAsyncTaskHandle);
#define   WSACancelAsyncRequest WINELIB_NAME(WSACancelAsyncRequest)

INT16     WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, UINT32 lEvent);
INT32     WINAPI WSAAsyncSelect32(SOCKET32 s, HWND32 hWnd, UINT32 uMsg, UINT32 lEvent);
#define   WSAAsyncSelect WINELIB_NAME(WSAAsyncSelect)


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

/* ws_... struct conversion flags */

#define WS_DUP_LINEAR		0x0000
#define WS_DUP_NATIVE           0x0001		/* native structure format (not ws_..) */
#define WS_DUP_OFFSET           0x0002		/* internal pointers are offsets */
#define WS_DUP_SEGPTR           0x0004		/* internal pointers are SEGPTRs */
						/* by default, internal pointers are linear */
/* async DNS op flags */

#define AOP_IO                  0x0000001	/* aop_control paramaters */

#define AOP_CONTROL_REMOVE      0x0000000	/* aop_control return values */
#define AOP_CONTROL_KEEP        0x0000001

typedef struct  __aop
{
  /* AOp header */

  struct __aop *next, *prev;
  int           fd[2];				/* pipe */
  int   (*aop_control)(struct __aop*, int);	/* SIGIO handler */
  pid_t         pid;				/* child process pid */

  /* custom data */

  HWND32        hWnd;				/* hWnd to post */ 
  UINT32        uMsg;				/* uMsg message to. */

  union 
  {
    SEGPTR seg_base;
    LPSTR  lin_base;
    void*  ptr_base;
  } 		b;				/* buffer to copy result to */

  UINT32        buflen;
  UINT32	flags;				/* WSMSG_ASYNC_... */
} ws_async_op;

#define WSMSG_ASYNC_HOSTBYNAME  0x0001
#define WSMSG_ASYNC_HOSTBYADDR  0x0002
#define WSMSG_ASYNC_PROTOBYNAME 0x0010
#define WSMSG_ASYNC_PROTOBYNUM  0x0020
#define WSMSG_ASYNC_SERVBYNAME  0x0100
#define WSMSG_ASYNC_SERVBYPORT  0x0200
#define WSMSG_ASYNC_WIN32	0x1000
#define WSMSG_DEAD_AOP		0x8000

typedef struct __sop	/* WSAAsyncSelect() control struct */
{
  struct __sop *next, *prev;

  struct __ws*  pws;
  HWND32        hWnd;
  UINT32        uMsg;
} ws_select_op;

typedef struct	__ws	/* socket */
{
  int		fd;
  unsigned	flags;
  ws_select_op*	psop;
} ws_socket;

#define WS_MAX_SOCKETS_PER_THREAD       16
#define WS_MAX_UDP_DATAGRAM             1024

#define WSI_BLOCKINGCALL	0x00000001	/* per-thread info flags */
#define WSI_BLOCKINGHOOK32	0x00000002	/* 32-bit callback */

typedef struct _WSINFO
{
  struct _WSINFO*       prev,*next;

  unsigned		flags;
  INT32			err;			/* WSAGetLastError() return value */
  INT16			num_startup;		/* reference counter */
  INT16			num_async_rq;
  INT16			last_free;		/* entry in the socket table */
  UINT16		buflen;
  char*			buffer;			/* allocated from SEGPTR heap */
  char*			dbuffer;		/* buffer for dummies (32 bytes) */

  ws_socket		sock[WS_MAX_SOCKETS_PER_THREAD];
  DWORD			blocking_hook;
  HTASK16               tid;    		/* owning task id - process might be better */
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
int  WINSOCK_cancel_async_op(ws_async_op* p_aop);

void WINSOCK_cancel_task_aops(HTASK16, void (*__memfree)(void*) );

BOOL32 WINSOCK_HandleIO(int* fd_max, int num_pending, fd_set io_set[3] );
void   WINSOCK_Shutdown(void);
UINT16 wsaErrno(void);
UINT16 wsaHerrno(void);

#endif  /* _WINSOCKAPI_ */

