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
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include "winsock.h"

#define DEBUG_WINSOCK

/* XXX per task */
WORD wsa_errno;
int  wsa_initted;

WORD errno_to_wsaerrno(int errno)
{
        switch(errno) {
        case ENETDOWN:
                return WSAENETDOWN;
        case EAFNOSUPPORT:
                return WSAEAFNOSUPPORT;
        case EMFILE:
                return WSAEMFILE;
        case ENOBUFS:
                return WSAENOBUFS;
        case EPROTONOSUPPORT:
                return EPROTONOSUPPORT;
        case EPROTOTYPE:
                return WSAEPROTOTYPE;
	case EBADF:
	case ENOTSOCK:
		return WSAENOTSOCK;

        default:
#ifndef sun
#if defined(__FreeBSD__)
                fprintf(stderr, "winsock: errno_to_wsaerrno translation failure.\n\t: %s (%d)\n",
                        sys_errlist[errno], errno);
#else
                fprintf(stderr, "winsock: errno_to_wsaerrno translation failure.\n\t: %s (%d)\n",
                        strerror[errno], errno);
#endif
#else
		fprintf (stderr, "winsock: errno_to_wsaerrno translation failure.\n");
#endif
		return WSAENETDOWN;
        }
}
 
SOCKET Winsock_accept(SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen)
{
	int sock;

	if ((sock = accept(s, addr, addrlen)) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return INVALID_SOCKET;
	}
	return sock;
}

int Winsock_bind(SOCKET s, struct sockaddr FAR *name, int namelen)
{
	if (bind(s, name, namelen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_closesocket(SOCKET s)
{
	if (close(s) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_connect(SOCKET s, struct sockaddr FAR *name, int namelen)
{
	if (connect(s, name, namelen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_getpeername(SOCKET s, struct sockaddr FAR *name, int FAR *namelen)
{
	if (getpeername(s, name, namelen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_getsockname(SOCKET s, struct sockaddr FAR *name, int FAR *namelen)
{
	if (getsockname(s, name, namelen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_getsockopt(SOCKET s, int loptname, char FAR *optval, int FAR *optlen)
{
	if (getsockopt(s, 0, loptname, optval, optlen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
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

	if ((s = inet_ntoa(in)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return s;
}

int Winsock_ioctlsocket(SOCKET s, long cmd, u_long FAR *argp)
{
	if (ioctl(s, cmd, argp) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

int Winsock_listen(SOCKET s, int backlog)
{
	if (listen(s, backlog) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
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

int Winsock_recv(SOCKET s, char FAR *buf, int len, int flags)
{
	int length;

	if ((length = recv(s, buf, len, flags)) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return length;
}

int Winsock_recvfrom(SOCKET s, char FAR *buf, int len, int flags, 
		struct sockaddr FAR *from, int FAR *fromlen)
{
	int length;

	if ((length = recvfrom(s, buf, len, flags, from, fromlen)) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return length;
}

int Winsock_select(int nfds, fd_set FAR *readfds, fd_set FAR *writefds,
	fd_set FAR *exceptfds, struct timeval FAR *timeout)
{
	return(select(nfds, readfds, writefds, exceptfds, timeout));
}

int Winsock_send(SOCKET s, char FAR *buf, int len, int flags)
{
	int length;

	if ((length = send(s, buf, len, flags)) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return length;
}

int Winsock_sendto(SOCKET s, char FAR *buf, int len, int flags,
		struct sockaddr FAR *to, int tolen)
{
	int length;

	if ((length = sendto(s, buf, len, flags, to, tolen)) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return length;
}

int Winsock_setsockopt(SOCKET s, int level, int optname, const char FAR *optval, 
		int optlen)
{
	if (setsockopt(s, level, optname, optval, optlen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}                                         

int Winsock_shutdown(SOCKET s, int how)
{
	if (shutdown(s, how) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}

SOCKET Winsock_socket(WORD af, WORD type, WORD protocol)
{
    int sock;

#ifdef DEBUG_WINSOCK
    printf("Winsock_socket: af=%d type=%d protocol=%d\n", af, type, protocol);
#endif

/*  let the kernel do the dirty work..

    if (!wsa_initted) {
            wsa_errno = WSANOTINITIALISED;
            return INVALID_SOCKET;
    }
*/
    if ((sock = socket(af, type, protocol)) < 0) {
            wsa_errno = errno_to_wsaerrno(errno);
            return INVALID_SOCKET;
    }
    return sock;
}

struct hostent *Winsock_gethostbyaddr(const char FAR *addr, int len,  int type)
{
	struct hostent *host;

	if ((host = gethostbyaddr(addr, len, type)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return host;
}

struct hostent *Winsock_gethostbyname(const char FAR *name)
{
	struct hostent *host;

	if ((host = gethostbyname(name)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return host;
}

int Winsock_gethostname(char FAR *name, int namelen)
{
	if (gethostname(name, namelen) < 0) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return SOCKET_ERROR;
	}
	return 0;
}          

struct protoent *Winsock_getprotobyname(char FAR *name)
{
	struct protoent *proto;

	if ((proto = getprotobyname(name)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return proto;
}

struct protoent *Winsock_getprotobynumber(int number)
{
	struct protoent *proto;

	if ((proto = getprotobynumber(number)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return proto;
}

struct servent *Winsock_getservbyname(const char FAR *name, const char FAR *proto)
{
	struct servent *service;

	if ((service = getservbyname(name, proto)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return service;
}

struct servent *Winsock_getservbyport(int port, const char FAR *proto)
{
	struct servent *service;

	if ((service = getservbyport(port, proto)) == NULL) {
        	wsa_errno = errno_to_wsaerrno(errno);
        	return NULL;
	}
	return service;
}

HANDLE WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char FAR *addr,
		 int len, int type, const char FAR *buf, int buflen)
{

}                    

HANDLE WSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			char FAR *buf, int buflen)
{

}                     

HANDLE WSAAsyncGetProtoByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			char FAR *buf, int buflen)
{

}

HANDLE WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg, int number, 
			char FAR *buf, int buflen)
{

}

HANDLE WSAAsyncGetServByName(HWND hWnd, u_int wMsg, const char FAR *name, 
			const char FAR *proto, char FAR *buf, int buflen)
{

}

HANDLE WSAAsyncGetServByPort(HWND hWnd, u_int wMsg, int port, const char FAR 
			*proto, char FAR *buf, int buflen)
{

}

int WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{

}

int WSAFDIsSet(int fd, fd_set *set)
{
	return( FD_ISSET(fd, set) );
}

WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{

}

WSACancelBlockingCall ( void )
{

}
          
int WSAGetLastError(void)
{
    return wsa_errno;
}

void WSASetLastError(int iError)
{
    wsa_errno = iError;
}

BOOL WSAIsBlocking (void)
{

}

FARPROC WSASetBlockingHook(FARPROC lpBlockFunc)
{

}

int WSAUnhookBlockingHook(void)
{

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

int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
#ifdef DEBUG_WINSOCK
    fprintf(stderr, "WSAStartup: verReq=%x\n", wVersionRequested);
#endif

    if (LOBYTE(wVersionRequested) < 1 ||
        (LOBYTE(wVersionRequested) == 1 &&
         HIBYTE(wVersionRequested) < 1))
        return WSAVERNOTSUPPORTED;

    if (!lpWSAData)
        return WSAEINVAL;
    
    bcopy(&Winsock_data, lpWSAData, sizeof(Winsock_data));

    wsa_initted = 1;
    
    return(0);
}

int WSACleanup(void)
{
    wsa_initted = 0;
    return 0;
}
