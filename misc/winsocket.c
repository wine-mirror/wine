/*
 * based on Windows Sockets 1.1 specs
 * (ftp.microsoft.com:/Advsys/winsock/spec11/WINSOCK.TXT)
 * 
 * (C) 1993,1994 John Brezak, Erik Bos.
 */
 
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#ifdef __svr4__
#include <sys/filio.h>
#include <sys/ioccom.h>
#endif
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#undef TRANSPARENT
#include "winsock.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"

static WORD wsa_errno;
static int wsa_initted;
static key_t wine_key = 0;
static FARPROC BlockFunction;
static fd_set fd_in_use;

struct ipc_packet {
	long	mtype;
	HANDLE	handle;
	HWND	hWnd;
	WORD	wMsg;
	LONG	lParam;
};

#ifndef WINELIB
#pragma pack(1)
#endif

#define IPC_PACKET_SIZE (sizeof(struct ipc_packet) - sizeof(long))
#define MTYPE 0xb0b0eb05

/* These structures are Win16 only */

struct WIN_hostent  {
	SEGPTR	h_name WINE_PACKED;		/* official name of host */
	SEGPTR	h_aliases WINE_PACKED;	/* alias list */
	INT	h_addrtype WINE_PACKED;		/* host address type */
	INT	h_length WINE_PACKED;		/* length of address */
	char	**h_addr_list WINE_PACKED;	/* list of addresses from name server */
	char	*names[2];
	char    hostname[200];
};

struct	WIN_protoent {
	SEGPTR	p_name WINE_PACKED;		/* official protocol name */
	SEGPTR	p_aliases WINE_PACKED;	/* alias list */
	INT	p_proto WINE_PACKED;		/* protocol # */
};

struct	WIN_servent {
	SEGPTR	s_name WINE_PACKED;		/* official service name */
	SEGPTR	s_aliases WINE_PACKED;	/* alias list */
	INT	s_port WINE_PACKED;		/* port # */
	SEGPTR	s_proto WINE_PACKED;		/* protocol to use */
};

struct WinSockHeap {
	char	ntoa_buffer[32];

	struct	WIN_hostent hostent_addr;
	struct	WIN_hostent hostent_name;
	struct	WIN_protoent protoent_name;
	struct	WIN_protoent protoent_number;
	struct	WIN_servent servent_name;
	struct	WIN_servent servent_port;

	struct	WIN_hostent WSAhostent_addr;
	struct	WIN_hostent WSAhostent_name;
	struct	WIN_protoent WSAprotoent_name;
	struct	WIN_protoent WSAprotoent_number;	
	struct	WIN_servent WSAservent_name;
	struct	WIN_servent WSAservent_port;
	/* 8K scratch buffer for aliases and friends are hopefully enough */
	char scratch[8192];
};
static struct WinSockHeap *Heap;
static HANDLE HeapHandle;
#ifndef WINELIB32
static int ScratchPtr;
#endif

#ifndef WINELIB
#define GET_SEG_PTR(x)	MAKELONG((int)((char*)(x)-(char*)Heap),	\
							GlobalHandleToSel(HeapHandle))
#else
#define GET_SEG_PTR(x)	((SEGPTR)x)
#endif

#ifndef WINELIB
#pragma pack(4)
#endif

#define dump_sockaddr(a) \
	fprintf(stderr, "sockaddr_in: family %d, address %s, port %d\n", \
			((struct sockaddr_in *)a)->sin_family, \
			inet_ntoa(((struct sockaddr_in *)a)->sin_addr), \
			ntohs(((struct sockaddr_in *)a)->sin_port))

#ifndef WINELIB32
static void ResetScratch()
{
	ScratchPtr=0;
}

static void *scratch_alloc(int size)
{
	char *ret;
	if(ScratchPtr+size > sizeof(Heap->scratch))
		return 0;
	ret = Heap->scratch + ScratchPtr;
	ScratchPtr += size;
	return ret;
}

static SEGPTR scratch_strdup(char * s)
{
	char *ret=scratch_alloc(strlen(s)+1);
	strcpy(ret,s);
	return GET_SEG_PTR(ret);
}
#endif

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
	case EBADF:		return WSAEBADF;
	case EACCES:		return WSAEACCES;
	case EFAULT:		return WSAEFAULT;
	case EINVAL:		return WSAEINVAL;
	case EMFILE:		return WSAEMFILE;
	case EWOULDBLOCK:	return WSAEWOULDBLOCK;
	case EINPROGRESS:	return WSAEINPROGRESS;
	case EALREADY:		return WSAEALREADY;
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
#ifdef EPROCLIM
	case EPROCLIM:		return WSAEPROCLIM;
#endif
	case EUSERS:		return WSAEUSERS;
#ifdef EDQUOT
	case EDQUOT:		return WSAEDQUOT;
#endif
	case ESTALE:		return WSAESTALE;
	case EREMOTE:		return WSAEREMOTE;
/* just in case we ever get here and there are no problems */
	case 0:			return 0;

        default:
		fprintf(stderr, "winsock: unknown errorno %d!\n", errno);
		return WSAEOPNOTSUPP;
	}
}

static void errno_to_wsaerrno(void)
{
	wsa_errno = wsaerrno();
}


static WORD wsaherrno(void)
{
#if DEBUG_WINSOCK
#ifndef sun
#if defined(__FreeBSD__)
                fprintf(stderr, "winsock: h_errno %d, (%s).\n", 
                			h_errno, sys_errlist[h_errno]);
#else
                fprintf(stderr, "winsock: h_errno %d.\n", h_errno);
		herror("wine: winsock: wsaherrno");
#endif
#else
                fprintf(stderr, "winsock: h_errno %d\n", h_errno);
#endif
#endif

        switch(h_errno)
        {
	case TRY_AGAIN:		return WSATRY_AGAIN;
	case HOST_NOT_FOUND:	return WSAHOST_NOT_FOUND;
	case NO_RECOVERY:	return WSANO_RECOVERY;
	case NO_DATA:		return WSANO_DATA; 
/* just in case we ever get here and there are no problems */
	case 0:			return 0;


        default:
		fprintf(stderr, "winsock: unknown h_errorno %d!\n", h_errno);
		return WSAEOPNOTSUPP;
	}
}


static void herrno_to_wsaerrno(void)
{
	wsa_errno = wsaherrno();
}


static void convert_sockopt(INT *level, INT *optname)
{
/* $%#%!#! why couldn't they use the same values for both winsock and unix ? */

	switch (*level) {
		case -1: 
			*level = SOL_SOCKET;
			switch (*optname) {
			case 0x01:	*optname = SO_DEBUG;
					break;
			case 0x04:	*optname = SO_REUSEADDR;
					break;
			case 0x08:	*optname = SO_KEEPALIVE;
					break;
			case 0x10:	*optname = SO_DONTROUTE;
					break;
			case 0x20:	*optname = SO_BROADCAST;
					break;
			case 0x80:	*optname = SO_LINGER;
					break;
			case 0x100:	*optname = SO_OOBINLINE;
					break;
			case 0x1001:	*optname = SO_SNDBUF;
					break;
			case 0x1002:	*optname = SO_RCVBUF;
					break;
			case 0x1007:	*optname = SO_ERROR;
					break;
			case 0x1008:	*optname = SO_TYPE;
					break;
			default: 
					fprintf(stderr, "convert_sockopt() unknown optname %d\n", *optname);
					break;
			}
			break;
		case 6:	*optname = IPPROTO_TCP;
	}
}

#ifndef WINELIB
static SEGPTR copy_stringlist(char **list)
{
	SEGPTR *s_list;
	int i;
	for(i=0;list[i];i++)
		;
	s_list = scratch_alloc(sizeof(SEGPTR)*(i+1));
	for(i=0;list[i];i++)
	{
		void *copy = scratch_alloc(strlen(list[i])+1);
		strcpy(copy,list[i]);
		s_list[i]=GET_SEG_PTR(copy);
	}
	s_list[i]=0;
	return GET_SEG_PTR(s_list);
}
	

static void CONVERT_HOSTENT(struct WIN_hostent *heapent, struct hostent *host)
{
	SEGPTR *addr_list;
	int i;
	ResetScratch();
	strcpy(heapent->hostname,host->h_name);
	heapent->h_name = GET_SEG_PTR(heapent->hostname);
	/* Convert aliases. Have to create array with FAR pointers */
	if(!host->h_aliases)
		heapent->h_aliases = 0;
	else
		heapent->h_aliases = copy_stringlist(host->h_aliases);

	heapent->h_addrtype = host->h_addrtype;
	heapent->h_length = host->h_length;
	for(i=0;host->h_addr_list[i];i++)
		;
	addr_list=scratch_alloc(sizeof(SEGPTR)*(i+1));
	heapent->h_addr_list = (char**)GET_SEG_PTR(addr_list);
	for(i=0;host->h_addr_list[i];i++)
	{
		void *addr=scratch_alloc(host->h_length);
		memcpy(addr,host->h_addr_list[i],host->h_length);
		addr_list[i]=GET_SEG_PTR(addr);
	}
	addr_list[i]=0;
}

static void CONVERT_PROTOENT(struct WIN_protoent *heapent, 
	struct protoent *proto)
{
	ResetScratch();
	heapent->p_name= scratch_strdup(proto->p_name);
	heapent->p_aliases=proto->p_aliases ? 
			copy_stringlist(proto->p_aliases) : 0;
	heapent->p_proto = proto->p_proto;
}

static void CONVERT_SERVENT(struct WIN_servent *heapent, struct servent *serv)
{
	ResetScratch();
	heapent->s_name = scratch_strdup(serv->s_name);
	heapent->s_aliases = serv->s_aliases ?
			copy_stringlist(serv->s_aliases) : 0;
	heapent->s_port = serv->s_port;
	heapent->s_proto = scratch_strdup(serv->s_proto);
}
#else
#define CONVERT_HOSTENT(a,b)	memcpy(a, &b, sizeof(a))
#define CONVERT_PROTOENT(a,b)	memcpy(a, &b, sizeof(a))
#define CONVERT_SERVENT(a,b)	memcpy(a, &b, sizeof(a))
#endif

SOCKET WINSOCK_accept(SOCKET s, struct sockaddr *addr, INT *addrlen)
{
	int sock;

	dprintf_winsock(stddeb, "WSA_accept: socket %d, ptr %8x, length %d\n", s, (int) addr, *addrlen);

	if ((sock = accept(s, addr, (int *) addrlen)) < 0) {
        	errno_to_wsaerrno();
        	return INVALID_SOCKET;
	}
	return sock;
}

INT WINSOCK_bind(SOCKET s, struct sockaddr *name, INT namelen)
{
	dprintf_winsock(stddeb, "WSA_bind: socket %d, ptr %8x, length %d\n", s, (int) name, namelen);
	dump_sockaddr(name);

	if (bind(s, name, namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT WINSOCK_closesocket(SOCKET s)
{
	dprintf_winsock(stddeb, "WSA_closesocket: socket %d\n", s);

	FD_CLR(s, &fd_in_use);

	if (close(s) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT WINSOCK_connect(SOCKET s, struct sockaddr *name, INT namelen)
{
	dprintf_winsock(stddeb, "WSA_connect: socket %d, ptr %8x, length %d\n", s, (int) name, namelen);
	dump_sockaddr(name);

	if (connect(s, name, namelen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT WINSOCK_getpeername(SOCKET s, struct sockaddr *name, INT *namelen)
{
	dprintf_winsock(stddeb, "WSA_getpeername: socket: %d, ptr %8x, ptr %8x\n", s, (int) name, *namelen);
	dump_sockaddr(name);

	if (getpeername(s, name, (int *) namelen) < 0) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return SOCKET_ERROR;
	}
	return 0;
}

INT WINSOCK_getsockname(SOCKET s, struct sockaddr *name, INT *namelen)
{
	dprintf_winsock(stddeb, "WSA_getsockname: socket: %d, ptr %8x, ptr %8x\n", s, (int) name, (int) *namelen);
	if (getsockname(s, name, (int *) namelen) < 0) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return SOCKET_ERROR;
	}
	return 0;
}

INT
WINSOCK_getsockopt(SOCKET s, INT level, INT optname, char *optval, INT *optlen)
{
	dprintf_winsock(stddeb, "WSA_getsockopt: socket: %d, opt %d, ptr %8x, ptr %8x\n", s, level, (int) optval, (int) *optlen);
	convert_sockopt(&level, &optname);

	if (getsockopt(s, (int) level, optname, optval, (int *) optlen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

u_long WINSOCK_htonl(u_long hostlong)
{
	return( htonl(hostlong) );
}         

u_short WINSOCK_htons(u_short hostshort)
{
	return( htons(hostshort) );
}

u_long WINSOCK_inet_addr(char *cp)
{
	return( inet_addr(cp) );
}

char *WINSOCK_inet_ntoa(struct in_addr in)
{
	char *s;

/*	dprintf_winsock(stddeb, "WSA_inet_ntoa: %8lx\n", (int) in);*/

	if ((s = inet_ntoa(in)) == NULL) {
        	errno_to_wsaerrno();
        	return NULL;
	}

	strncpy(Heap->ntoa_buffer, s, sizeof(Heap->ntoa_buffer) );

	return (char *) GET_SEG_PTR(&Heap->ntoa_buffer);
}

INT WINSOCK_ioctlsocket(SOCKET s, u_long cmd, u_long *argp)
{
	long newcmd;
	u_long *newargp;
	char *ctlname;
	dprintf_winsock(stddeb, "WSA_ioctl: socket %d, cmd %lX, ptr %8x\n", s, cmd, (int) argp);

	/* Why can't they all use the same ioctl numbers */
	newcmd=cmd;
	newargp=argp;
	ctlname=0;
	if(cmd == _IOR('f',127,u_long))
	{
		ctlname="FIONREAD";
		newcmd=FIONREAD;
	}else 
	if(cmd == _IOW('f',126,u_long) || cmd == _IOR('f',126,u_long))
	{
		ctlname="FIONBIO";
		newcmd=FIONBIO;
	}else
	if(cmd == _IOW('f',125,u_long))
	{
		ctlname="FIOASYNC";
		newcmd=FIOASYNC;
	}

	if(!ctlname)
		fprintf(stderr,"Unknown winsock ioctl. Trying anyway\n");
	else
		dprintf_winsock(stddeb,"Recognized as %s\n", ctlname);
	

	if (ioctl(s, newcmd, newargp) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

INT WINSOCK_listen(SOCKET s, INT backlog)
{
	dprintf_winsock(stddeb, "WSA_listen: socket %d, backlog %d\n", s, backlog);

	if (listen(s, backlog) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

u_long WINSOCK_ntohl(u_long netlong)
{
	return( ntohl(netlong) );
}

u_short WINSOCK_ntohs(u_short netshort)
{
	return( ntohs(netshort) );
}

INT WINSOCK_recv(SOCKET s, char *buf, INT len, INT flags)
{
	int length;

	dprintf_winsock(stddeb, "WSA_recv: socket %d, ptr %8x, length %d, flags %d\n", s, (int) buf, len, flags);

	if ((length = recv(s, buf, len, flags)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT WINSOCK_recvfrom(SOCKET s, char *buf, INT len, INT flags, 
		struct sockaddr *from, int *fromlen)
{
	int length;

	dprintf_winsock(stddeb, "WSA_recvfrom: socket %d, ptr %8lx, length %d, flags %d\n", s, (unsigned long)buf, len, flags);

	if ((length = recvfrom(s, buf, len, flags, from, fromlen)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT WINSOCK_select(INT nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout)
{
	dprintf_winsock(stddeb, "WSA_select: fd # %d, ptr %8lx, ptr %8lx, ptr %8lX\n", nfds, (unsigned long) readfds, (unsigned long) writefds, (unsigned long) exceptfds);

	return(select(nfds, readfds, writefds, exceptfds, timeout));
}

INT WINSOCK_send(SOCKET s, char *buf, INT len, INT flags)
{
	int length;

	dprintf_winsock(stddeb, "WSA_send: socket %d, ptr %8lx, length %d, flags %d\n", s, (unsigned long) buf, len, flags);

	if ((length = send(s, buf, len, flags)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT WINSOCK_sendto(SOCKET s, char *buf, INT len, INT flags,
		struct sockaddr *to, INT tolen)
{
	int length;

	dprintf_winsock(stddeb, "WSA_sendto: socket %d, ptr %8lx, length %d, flags %d\n", s, (unsigned long) buf, len, flags);

	if ((length = sendto(s, buf, len, flags, to, tolen)) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return length;
}

INT WINSOCK_setsockopt(SOCKET s, INT level, INT optname, const char *optval, 
			INT optlen)
{
	dprintf_winsock(stddeb, "WSA_setsockopt: socket %d, level %d, opt %d, ptr %8x, len %d\n", s, level, optname, (int) optval, optlen);
	convert_sockopt(&level, &optname);
	
	if (setsockopt(s, level, optname, optval, optlen) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}                                         

INT WINSOCK_shutdown(SOCKET s, INT how)
{
	dprintf_winsock(stddeb, "WSA_shutdown: socket s %d, how %d\n", s, how);

	if (shutdown(s, how) < 0) {
        	errno_to_wsaerrno();
        	return SOCKET_ERROR;
	}
	return 0;
}

SOCKET WINSOCK_socket(INT af, INT type, INT protocol)
{
    int sock;

    dprintf_winsock(stddeb, "WSA_socket: af=%d type=%d protocol=%d\n", af, type, protocol);

    if ((sock = socket(af, type, protocol)) < 0) {
            errno_to_wsaerrno();
            dprintf_winsock(stddeb, "WSA_socket: failed !\n");
            return INVALID_SOCKET;
    }
    
    if (sock > 0xffff) {
	/* we set the value of wsa_errno directly, because 
	 * only support socket numbers up to 0xffff. The
	 * value return indicates there are no descriptors available
	 */
	wsa_errno = WSAEMFILE;
	return INVALID_SOCKET;
    }

    FD_SET(sock, &fd_in_use);

    dprintf_winsock(stddeb, "WSA_socket: fd %d\n", sock);
    return sock;
}

/*
struct WIN_hostent *
*/
SEGPTR WINSOCK_gethostbyaddr(const char *addr, INT len, INT type)
{
	struct hostent *host;

	dprintf_winsock(stddeb, "WSA_gethostbyaddr: ptr %8x, len %d, type %d\n", (int) addr, len, type);

	if ((host = gethostbyaddr(addr, len, type)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_HOSTENT(&Heap->hostent_addr, host);

	return GET_SEG_PTR(&Heap->hostent_addr);
}

/*
struct WIN_hostent *
*/
SEGPTR WINSOCK_gethostbyname(const char *name)
{
	struct hostent *host;

	dprintf_winsock(stddeb, "WSA_gethostbyname: %s\n", name);

	if ((host = gethostbyname(name)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_HOSTENT(&Heap->hostent_name, host);

	return GET_SEG_PTR(&Heap->hostent_name);
}

INT WINSOCK_gethostname(char *name, INT namelen)
{
	dprintf_winsock(stddeb, "WSA_gethostname: name %s, len %d\n", name, namelen);

	if (gethostname(name, namelen) < 0) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return SOCKET_ERROR;
	}
	return 0;
}          

/*
struct WIN_protoent *
*/
SEGPTR WINSOCK_getprotobyname(char *name)
{
	struct protoent *proto;

	dprintf_winsock(stddeb, "WSA_getprotobyname: name %s\n", name);

	if ((proto = getprotobyname(name)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_PROTOENT(&Heap->protoent_name, proto);

	return GET_SEG_PTR(&Heap->protoent_name);
}

/*
struct WIN_protoent *
*/
SEGPTR WINSOCK_getprotobynumber(INT number)
{
	struct protoent *proto;

	dprintf_winsock(stddeb, "WSA_getprotobynumber: num %d\n", number);

	if ((proto = getprotobynumber(number)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_PROTOENT(&Heap->protoent_number, proto);

	return GET_SEG_PTR(&Heap->protoent_number);
}

/*
struct WIN_servent *
*/
SEGPTR WINSOCK_getservbyname(const char *name, const char *proto)
{
	struct servent *service;

	if (proto == NULL)
		proto = "tcp";

	dprintf_winsock(stddeb, "WSA_getservbyname: name %s, proto %s\n", name, proto);

	if ((service = getservbyname(name, proto)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_SERVENT(&Heap->servent_name, service);

	return GET_SEG_PTR(&Heap->servent_name);
}

/*
struct WIN_servent *
*/
SEGPTR WINSOCK_getservbyport(INT port, const char *proto)
{
	struct servent *service;

	dprintf_winsock(stddeb, "WSA_getservbyport: port %d, name %s\n", port, proto);

	if ((service = getservbyport(port, proto)) == NULL) {
		if (h_errno < 0) {
        		errno_to_wsaerrno();
		} else {
			herrno_to_wsaerrno();
		}
        	return NULL;
	}
	CONVERT_SERVENT(&Heap->servent_port, service);

	return GET_SEG_PTR(&Heap->servent_port);
}

/******************** winsock specific functions ************************
 *
 */
static HANDLE new_handle = 0;

static HANDLE AllocWSAHandle(void)
{
	return new_handle++;
}

static void recv_message(int sig)
{
	struct ipc_packet message;

/* FIXME: something about no message of desired type */
	if (msgrcv(wine_key, (struct msgbuf*)&(message), 
		   IPC_PACKET_SIZE, MTYPE, IPC_NOWAIT) == -1)
		perror("wine: msgrcv");

	fprintf(stderr, 
		"WSA: PostMessage (hwnd "NPFMT", wMsg %d, wParam "NPFMT", lParam %ld)\n",
		message.hWnd,
		message.wMsg,
		message.handle,
		message.lParam);

	PostMessage(message.hWnd, message.wMsg, (WPARAM)message.handle, message.lParam);
		
	signal(SIGUSR1, recv_message);
}


static void send_message( HWND hWnd, u_int wMsg, HANDLE handle, long lParam)
{
	struct ipc_packet message;
	
	message.mtype = MTYPE;
	message.handle = handle;
	message.hWnd = hWnd;
	message.wMsg = wMsg;
	message.lParam = lParam;

	fprintf(stderr, 
		"WSA: send (hwnd "NPFMT", wMsg %d, handle "NPFMT", lParam %ld)\n",
		hWnd, wMsg, handle, lParam);
	
/* FIXME: something about invalid argument */
	if (msgsnd(wine_key, (struct msgbuf*)&(message),  
		   IPC_PACKET_SIZE, IPC_NOWAIT) == -1)
		perror("wine: msgsnd");
		
	kill(getppid(), SIGUSR1);
}


HANDLE WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg, const char *addr,
		 INT len, INT type, char *buf, INT buflen)
{
	HANDLE handle;
	struct hostent *host;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((host = gethostbyaddr(addr, len, type)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, host, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}


HANDLE WSAAsyncGetHostByName(HWND hWnd, u_int wMsg, const char *name, 
			char *buf, INT buflen)
{
	HANDLE handle;
	struct hostent *host;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((host = gethostbyname(name)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, host, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}                     


HANDLE WSAAsyncGetProtoByName(HWND hWnd, u_int wMsg, const char *name, 
			char *buf, INT buflen)
{
	HANDLE handle;
	struct protoent *proto;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((proto = getprotobyname(name)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, proto, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}


HANDLE WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg, INT number, 
			char *buf, INT buflen)
{
	HANDLE handle;
	struct protoent *proto;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((proto = getprotobynumber(number)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, proto, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}


HANDLE WSAAsyncGetServByName(HWND hWnd, u_int wMsg, const char *name, 
			const char *proto, char *buf, INT buflen)
{
	HANDLE handle;
	struct servent *service;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((service = getservbyname(name, proto)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, service, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}


HANDLE WSAAsyncGetServByPort(HWND hWnd, u_int wMsg, INT port, const char 
			*proto, char *buf, INT buflen)
{
	HANDLE handle;
	struct servent *service;

	handle = AllocWSAHandle();

	if (fork()) {
		return handle;
	} else {
		if ((service = getservbyport(port, proto)) == NULL) {
			if (h_errno < 0) {
        			errno_to_wsaerrno();
			} else {
				herrno_to_wsaerrno();
			}
			send_message(hWnd, wMsg, handle, wsaerrno() << 16);
			exit(0);
		}
		memcpy(buf, service, buflen);
		send_message(hWnd, wMsg, handle, 0);
		exit(0);
	}
}

INT WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg, long lEvent)
{
	long event;
	fd_set read_fds, write_fds, except_fds;

	dprintf_winsock(stddeb, "WSA_AsyncSelect: socket %d, HWND "NPFMT", wMsg %d, event %ld\n", s, hWnd, wMsg, lEvent);

	/* remove outstanding asyncselect() processes */
	/* kill */

	if (wMsg == 0 && lEvent == 0) 
		return 0;

	if (fork()) {
		return 0;
	} else {
		while (1) {
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&except_fds);

			if (lEvent & FD_READ)
				FD_SET(s, &read_fds);
			if (lEvent & FD_WRITE)
				FD_SET(s, &write_fds);

			fcntl(s, F_SETFL, O_NONBLOCK);
			select(s + 1, &read_fds, &write_fds, &except_fds, NULL);

			event = 0;
			if (FD_ISSET(s, &read_fds))
				event |= FD_READ;
			if (FD_ISSET(s, &write_fds))
				event |= FD_WRITE;
	/* FIXME: the first time through we get a winsock error of 2, why? */
			send_message(hWnd, wMsg, (HANDLE)s, (wsaerrno() << 16) | event);
		}
	}
}

INT WSAFDIsSet(INT fd, fd_set *set)
{
	return( FD_ISSET(fd, set) );
}

INT WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{
	dprintf_winsock(stddeb, "WSA_AsyncRequest: handle "NPFMT"\n", hAsyncTaskHandle);

	return 0;
}

INT WSACancelBlockingCall(void)
{
	dprintf_winsock(stddeb, "WSA_CancelBlockCall\n");
	return 0;
}
          
INT WSAGetLastError(void)
{
	dprintf_winsock(stddeb, "WSA_GetLastError = %x\n", wsa_errno);

    return wsa_errno;
}

void WSASetLastError(INT iError)
{
	dprintf_winsock(stddeb, "WSA_SetLastErorr %d\n", iError);

    wsa_errno = iError;
}

BOOL WSAIsBlocking(void)
{
	dprintf_winsock(stddeb, "WSA_IsBlocking\n");

	return 0;
}

FARPROC WSASetBlockingHook(FARPROC lpBlockFunc)
{
	dprintf_winsock(stddeb, "WSA_SetBlockHook %8lx, STUB!\n", (unsigned long) lpBlockFunc);
	BlockFunction = lpBlockFunc;

	return (FARPROC) lpBlockFunc;
}

INT WSAUnhookBlockingHook(void)
{
	dprintf_winsock(stddeb, "WSA_UnhookBlockingHook\n");
	BlockFunction = NULL;

	return 0;
}

WSADATA WINSOCK_data = {
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

    dprintf_winsock(stddeb, "WSAStartup: verReq=%x\n", wVersionRequested);

    if (LOBYTE(wVersionRequested) < 1 ||
        (LOBYTE(wVersionRequested) == 1 &&
         HIBYTE(wVersionRequested) < 1))
	return WSAVERNOTSUPPORTED;

    if (!lpWSAData)
        return WSAEINVAL;

	/* alloc winsock heap */

    if ((HeapHandle = GlobalAlloc(GMEM_FIXED,sizeof(struct WinSockHeap))) == 0)
	return WSASYSNOTREADY;

    Heap = (struct WinSockHeap *) GlobalLock(HeapHandle);
    bcopy(&WINSOCK_data, lpWSAData, sizeof(WINSOCK_data));

    /* ipc stuff */

    if ((wine_key = msgget(IPC_PRIVATE, 0600)) == -1)
	perror("wine: msgget"); 

    signal(SIGUSR1, recv_message);

    /* clear */
    
    FD_ZERO(&fd_in_use);

    wsa_initted = 1;
    return(0);
}

INT WSACleanup(void)
{
	int fd;

	if (wine_key)
		if (msgctl(wine_key, IPC_RMID, NULL) == -1)
			perror("wine: shmctl");

	for (fd = 0; fd != FD_SETSIZE; fd++)
		if (FD_ISSET(fd, &fd_in_use))
			close(fd);

	wsa_initted = 0;
	return 0;
}
