/*
 * based on Windows Sockets 1.1 specs
 * (ftp.microsoft.com:/Advsys/winsock/spec11/WINSOCK.TXT)
 * 
 * (C) 1993,1994 John Brezak, Erik Bos.
 */
 
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include "heap.h"
#include "winsock.h"

#define DEBUG_WINSOCK

static WORD wsa_errno;
static int wsa_initted;

#define dump_sockaddr(a) \
	fprintf(stderr, "sockaddr_in: family %d, address %s, port %d\n", \
			((struct sockaddr_in *)a)->sin_family, \
			inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
			ntohs(((struct sockaddr_in *)a)->sin_port))

struct WinSockHeap {
	char	ntoa_buffer[32];

	struct	hostent hostent_addr;
	struct	hostent hostent_name;
	struct	protoent protoent_name;
	struct	protoent protoent_number;
	struct	servent servent_name;
	struct	servent servent_port;

	struct	hostent WSAhostent_addr;
	struct	hostent WSAhostent_name;
	struct	protoent WSAprotoent_name;
	struct	protoent WSAprotoent_number;	
	struct	servent WSAservent_name;
	struct	servent WSAservent_port;
};

static struct WinSockHeap *heap;

static WORD wsaerrno(void)
{
#ifdef DEBUG_WINSOCK
#ifndef sun
#if defined(__FreeBSD__)
                fprintf(stderr, "winsock: errno %d, (%s).\n", 
                			errno, sys_errlist[errno]);
#else
                fprintf(stderr, "winsock: errno %d, (%s).\n", 
                			errno, strerror(errno));
#endif
#else
                fprintf(stderr, "winsock: errno %d\n", errno);
#endif
#endif

        switch(errno)
        {
	case EINTR:		return WSAEINTR;
	case EACCES:		return WSAEACCES;
	case EFAULT:		return WSAEFAULT;
	case EINVAL:		return WSAEINVAL;
	case EMFILE:		return WSAEMFILE;
	case EWOULDBLOCK:	return WSAEWOULDBLOCK;
	case EINPROGRESS:	return WSAEINPROGRESS;
	case EALREADY:		return WSAEALREADY;
	case EBADF:
	case ENOTSOCK:		return WSAENOTSOCK;
	case EDESTADDRREQ:	return WSAEDESTADDRREQ;
	case EMSGSIZE:		return WSAEMSGSIZE;
	case EPROTOTYPE:	return WSAEPROTOTYPE;
	case ENOPROTOOPT:	return WSAENOPROTOOPT;
	case EPROTONOSUPPORT:	return WSAEPROTONOSUPPORT;
	case ESOCKTNOSUPPORT:	return WSAESOCKTNOSUPPORT;
	case EOPNOTSUPP:	return WSAEOPNOTSUPP;
	case EPFNOSUPPORT:	return WSAEPFNOSUPPORT;
	case EAFNOSUPPORT:	return WSAEAFNOSUPPORT;
	case EADDRINUSE:	return WSAEADDRINUSE;
	case EADDRNOTAVAIL:	return WSAEADDRNOTAVAIL;
	case ENETDOWN:		return WSAENETDOWN;
	case ENETUNREACH:	return WSAENETUNREACH;
	case ENETRESET:		return WSAENETRESET;
	case ECONNABORTED:	return WSAECONNABORTED;
	case ECONNRESET:	return WSAECONNRESET;
	case ENOBUFS:		return WSAENOBUFS;
	case EISCONN:		return WSAEISCONN;
	case ENOTCONN:		return WSAENOTCONN;
	case ESHUTDOWN:		return WSAESHUTDOWN;
	case ETOOMANYREFS:	return WSAETOOMANYREFS;
	case ETIMEDOUT:		return WSAETIMEDOUT;
	case ECONNREFUSED:	return WSAECONNREFUSED;
	case ELOOP:		return WSAELOOP;
	case ENAMETOOLONG:	return WSAENAMETOOLONG;
	case EHOSTDOWN:		return WSAEHOSTDOWN;
	case EHOSTUNREACH:	return WSAEHOSTUNREACH;
	case ENOTEMPTY:		return WSAENOTEMPTY;
/*	case EPROCLIM:		return WSAEPROCLIM; */
	case EUSERS:		return WSAEUSERS;
	case EDQUOT:		return WSAEDQUOT;
	case ESTALE:		return WSAESTALE;
	case EREMOTE:		return WSAEREMOTE;

        default:
		fprintf(stderr, "winsock: unknown error!\n");
		return WSAEOPNOTSUPP;
	}
}

static WORD errno_to_wsaerrno(void)
{
	wsa_errno = wsaerrno();
}
 
SOCKET Winsock_accept(SOCKET s, struct sockaddr FAR *addr, INT FAR *addrlen)
{
	int sock;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_accept: socket %d, ptr %8x, length %d\n", s, (int) addr, addrlen);
#endif

	if ((sock = accept(s, addr, (int *) addrlen)) < 0) {
        	errno_to_wsaerrno();
        	return INVALID_SOCKET;
	}
	return sock;
}

INT Winsock_bind(SOCKET s, struct sockaddr FAR *name, INT namelen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_bind: socket %d, ptr %8x, length %d\n", s, (int) name, namelen);
	dump_sockaddr(name);
#endif

	if (bind(s, name, namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_closesocket(SOCKET s)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_closesocket: socket %d\n", s);
#endif

	if (close(s) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_connect(SOCKET s, struct sockaddr FAR *name, INT namelen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_connect: socket %d, ptr %8x, length %d\n", s, (int) name, namelen);
	dump_sockaddr(name);
#endif

	if (connect(s, name, namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_getpeername(SOCKET s, struct sockaddr FAR *name, INT FAR *namelen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getpeername: socket: %d, ptr %8x, ptr %8x\n", s, (int) name, *namelen);
	dump_sockaddr(name);
#endif

	if (getpeername(s, name, (int *) namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_getsockname(SOCKET s, struct sockaddr FAR *name, INT FAR *namelen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getsockname: socket: %d, ptr %8x, ptr %8x\n", s, (int) name, (int) *namelen);
#endif
	if (getsockname(s, name, (int *) namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_getsockopt(SOCKET s, INT loptname, char FAR *optval, INT FAR *optlen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getsockopt: socket: %d, opt %d, ptr %8x, ptr %8x\n", s, loptname, (int) optval, (int) *optlen);
#endif
	if (getsockopt(s, 0, (int) loptname, optval, (int *) optlen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

u_long Winsock_htonl(u_long hostlong)
{
	return( htonl(hostlong) );
}         

u_short Winsock_htons(u_short hostshort)
{
	return( htons(hostshort) );
}

u_long Winsock_inet_addr(char FAR *cp)
{
	return( inet_addr(cp) );
}

char *Winsock_inet_ntoa(struct in_addr in)
{
	char *s;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_inet_ntoa: %8x\n", in);
#endif

	if ((s = inet_ntoa(in)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}

	strncpy(heap->ntoa_buffer, s, sizeof(heap->ntoa_buffer) );

	return (char *) &heap->ntoa_buffer;
}

INT Winsock_ioctlsocket(SOCKET s, long cmd, u_long FAR *argp)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_ioctl: socket %d, cmd %d, ptr %8x\n", s, cmd, (int) argp);
#endif

	if (ioctl(s, cmd, argp) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT Winsock_listen(SOCKET s, INT backlog)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_listen: socket %d, backlog %d\n", s, backlog);
#endif

	if (listen(s, backlog) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

u_long Winsock_ntohl(u_long netlong)
{
	return( ntohl(netlong) );
}

u_short Winsock_ntohs(u_short netshort)
{
	return( ntohs(netshort) );
}

INT Winsock_recv(SOCKET s, char FAR *buf, INT len, INT flags)
{
	int length;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_recv: socket %d, ptr %8x, length %d, flags %d\n", s, (int) buf, len, flags);
#endif

	if ((length = recv(s, buf, len, flags)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT Winsock_recvfrom(SOCKET s, char FAR *buf, INT len, INT flags, 
		struct sockaddr FAR *from, int FAR *fromlen)
{
	int length;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_recvfrom: socket %d, ptr %8x, length %d, flags %d\n", s, buf, len, flags);
#endif

	if ((length = recvfrom(s, buf, len, flags, from, fromlen)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT Winsock_select(INT nfds, fd_set FAR *readfds, fd_set FAR *writefds,
	fd_set FAR *exceptfds, struct timeval FAR *timeout)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_select: fd # %d, ptr %8x, ptr %8x, ptr %*X\n", nfds, readfds, writefds, exceptfds);
#endif

	return(select(nfds, readfds, writefds, exceptfds, timeout));
}

INT Winsock_send(SOCKET s, char FAR *buf, INT len, INT flags)
{
	int length;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_send: socket %d, ptr %8x, length %d, flags %d\n", s, buf, len, flags);
#endif

	if ((length = send(s, buf, len, flags)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT Winsock_sendto(SOCKET s, char FAR *buf, INT len, INT flags,
		struct sockaddr FAR *to, INT tolen)
{
	int length;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_sendto: socket %d, ptr %8x, length %d, flags %d\n", s, buf, len, flags);
#endif

	if ((length = sendto(s, buf, len, flags, to, tolen)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT Winsock_setsockopt(SOCKET s, INT level, INT optname, const char FAR *optval, 
			INT optlen)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_setsockopt: socket %d, level %d, opt %d, ptr %8x, len %d\n", s, level, optname, (int) optval, optlen);
#endif

	if (setsockopt(s, level, optname, optval, optlen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}                                         

INT Winsock_shutdown(SOCKET s, INT how)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_shutdown: socket s %d, how %d\n", s, how);
#endif

	if (shutdown(s, how) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

SOCKET Winsock_socket(INT af, INT type, INT protocol)
{
    int sock;

#ifdef DEBUG_WINSOCK
    fprintf(stderr, "WSA_socket: af=%d type=%d protocol=%d\n", af, type, protocol);
#endif

    if ((sock = socket(af, type, protocol)) < 0) {
            errno_to_wsaerrno();
#ifdef DEBUG_WINSOCK
    fprintf(stderr, "WSA_socket: failed !\n");
#endif
            return INVALID_SOCKET;
    }
    if (sock > 0xffff) {
	wsa_errno = WSAEMFILE;
	return INVALID_SOCKET;
    }

#ifdef DEBUG_WINSOCK
    fprintf(stderr, "WSA_socket: fd %d\n", sock);
#endif
    return sock;
}

struct hostent *Winsock_gethostbyaddr(const char FAR *addr, INT len, INT type)
{
	struct hostent *host;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_gethostbyaddr: ptr %8x, len %d, type %d\n", (int) addr, len, type);
#endif

	if ((host = gethostbyaddr(addr, len, type)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->hostent_addr, host, sizeof(struct hostent));

	return (struct hostent *) &heap->hostent_addr;
}

struct hostent *Winsock_gethostbyname(const char FAR *name)
{
	struct hostent *host;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_gethostbyname: name %s\n", name);
#endif

	if ((host = gethostbyname(name)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->hostent_name, host, sizeof(struct hostent));

	return (struct hostent *) &heap->hostent_name;
}

int Winsock_gethostname(char FAR *name, INT namelen)
{

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_gethostname: name %d, len %d\n", name, namelen);
#endif

	if (gethostname(name, namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}          

struct protoent *Winsock_getprotobyname(char FAR *name)
{
	struct protoent *proto;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getprotobyname: name %s\n", name);
#endif

	if ((proto = getprotobyname(name)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->protoent_name, proto, sizeof(struct protoent));

	return (struct protoent *) &heap->protoent_name;
}

struct protoent *Winsock_getprotobynumber(INT number)
{
	struct protoent *proto;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getprotobynumber: num %d\n", number);
#endif

	if ((proto = getprotobynumber(number)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->protoent_number, proto, sizeof(struct protoent));

	return (struct protoent *) &heap->protoent_number;
}

struct servent *Winsock_getservbyname(const char FAR *name, const char FAR *proto)
{
	struct servent *service;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getservbyname: name %s, proto %s\n", name, proto);
#endif

	if ((service = getservbyname(name, proto)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->servent_name, service, sizeof(struct servent));

	return (struct servent *) &heap->servent_name;
}

struct servent *Winsock_getservbyport(INT port, const char FAR *proto)
{
	struct servent *service;

#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_getservbyport: port %d, name %s\n", port, proto);
#endif

	if ((service = getservbyport(port, proto)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}
	memcpy(&heap->servent_port, service, sizeof(struct servent));

	return (struct servent *) &heap->servent_port;
}

/******************** winsock specific functions ************************/

HANDLE WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char FAR *addr,
		 INT len, INT type, char FAR *buf, INT buflen)
{
	struct hostent *host;

	if ((host = gethostbyaddr(addr, len, type)) == NULL) {
		PostMessage(hWnd, wMsg, 1, wsaerrno() << 8);

		return 1;
	}

	memcpy(buf, host, buflen);
	PostMessage(hWnd, wMsg, 1, 0);

	return 1;
}                    

HANDLE WSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			char FAR *buf, INT buflen)
{
	struct hostent *host;

	if ((host = gethostbyname(name)) == NULL) {
		PostMessage(hWnd, wMsg, 2, wsaerrno() << 8);
		return 2;
	}

	memcpy(buf, host, buflen);
	PostMessage(hWnd, wMsg, 2, 0);

	return 2;
}                     

HANDLE WSAAsyncGetProtoByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			char FAR *buf, INT buflen)
{
	struct protoent *proto;

	if ((proto = getprotobyname(name)) == NULL) {
		PostMessage(hWnd, wMsg, 3, wsaerrno() << 8);
		return 3;
	}

	memcpy(buf, proto, buflen);
	PostMessage(hWnd, wMsg, 3, 0);

	return 3;
}

HANDLE WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg, INT number, 
			char FAR *buf, INT buflen)
{
	struct protoent *proto;

	if ((proto = getprotobynumber(number)) == NULL) {
		PostMessage(hWnd, wMsg, 4, wsaerrno() << 8);
		return 4;
	}

	memcpy(buf, proto, buflen);
	PostMessage(hWnd, wMsg, 4, 0);

	return 4;
}

HANDLE WSAAsyncGetServByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			const char FAR *proto, char FAR *buf, INT buflen)
{
	struct servent *service;

	if ((service = getservbyname(name, proto)) == NULL) {
		PostMessage(hWnd, wMsg, 5, wsaerrno() << 8);
        
        	return 5;
	}
	memcpy(buf, service, buflen);
	PostMessage(hWnd, wMsg, 5, 0);
	
	return 5;
}

HANDLE WSAAsyncGetServByPort(HWND hWnd, u_int wMsg, INT port, const char FAR 
			*proto, char FAR *buf, INT buflen)
{
	struct servent *service;

	if ((service = getservbyport(port, proto)) == NULL) {
		PostMessage(hWnd, wMsg, 6, wsaerrno() << 8);
        
        	return 6;
	}
	memcpy(buf, service, buflen);
	PostMessage(hWnd, wMsg, 6, 0);
	
	return 6;
}

INT WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_AsyncSelect: socket %d, HWND %d, wMsg %d, event %d\n", s, hWnd, wMsg, lEvent);
#endif
	fcntl(s, F_SETFL, O_NONBLOCK);


	return 0;
}

INT WSAFDIsSet(INT fd, fd_set *set)
{
	return( FD_ISSET(fd, set) );
}

INT WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_AsyncRequest: handle %d\n", hAsyncTaskHandle);
#endif
	return 0;
}

INT WSACancelBlockingCall(void)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_CancelBlockCall\n");
#endif
	return 0;
}
          
INT WSAGetLastError(void)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_GetLastError\n");
#endif

    return wsa_errno;
}

void WSASetLastError(INT iError)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_SetLastErorr %d\n", iError);
#endif

    wsa_errno = iError;
}

BOOL WSAIsBlocking(void)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_IsBlocking\n");
#endif
}

FARPROC WSASetBlockingHook(FARPROC lpBlockFunc)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_SetBlockHook\n %8x", lpBlockFunc);
#endif
}

INT WSAUnhookBlockingHook(void)
{
#ifdef DEBUG_WINSOCK
	fprintf(stderr, "WSA_UnhookBlockingHook\n");
#endif
}

WSADATA Winsock_data = {
        0x0101,
        0x0101,
        "WINE Sockets",
#ifdef linux
        "LINUX/i386",
#endif
#ifdef __NetBSD__
        "NetBSD/i386",
#endif
#ifdef sunos
	"SunOS",
#endif
        128,
	1024,
        NULL
};

INT WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
	int HeapHandle;
	MDESC *MyHeap;

#ifdef DEBUG_WINSOCK
    fprintf(stderr, "WSAStartup: verReq=%x\n", wVersionRequested);
#endif

    if (LOBYTE(wVersionRequested) < 1 ||
        (LOBYTE(wVersionRequested) == 1 &&
         HIBYTE(wVersionRequested) < 1))
	return WSAVERNOTSUPPORTED;

    if (!lpWSAData)
        return WSAEINVAL;

    if ((HeapHandle = GlobalAlloc(GMEM_FIXED,sizeof(struct WinSockHeap))) == 0)
	return WSASYSNOTREADY;

    heap = (struct WinSockHeap *) GlobalLock(HeapHandle);
    HEAP_Init(&MyHeap, heap, sizeof(struct WinSockHeap));
    
    bcopy(&Winsock_data, lpWSAData, sizeof(Winsock_data));

    wsa_initted = 1;
    
    return(0);
}

INT WSACleanup(void)
{
    wsa_initted = 0;
    return 0;
}
