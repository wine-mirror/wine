/* Async WINSOCK DNS services
 * 
 * (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * (C) 1999 Marcus Meissner
 *
 * NOTE: If you make any changes to fix a particular app, make sure
 * they don't break something else like Netscape or telnet and ftp
 * clients and servers (www.winsite.com got a lot of those).
 * 
 * FIXME:
 *	- Add WSACancel* and correct handle management. (works rather well for
 *	  now without it.)
 *	- Verify & Check all calls for correctness
 *	  (currently only WSAGetHostByName*, WSAGetServByPort* calls)
 *	- Check error returns.
 *	- mirc/mirc32 Finger @linux.kernel.org sometimes fails in threaded mode.
 *	  (not sure why)
 *	- This implementation did ignore the "NOTE:" section above (since the
 *	  whole stuff did not work anyway to other changes).
 */
 
#include "config.h"

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#if defined(__svr4__)
#include <sys/ioccom.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#endif

#if defined(__EMX__)
# include <sys/so_ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#include "wine/winbase16.h"
#include "winsock.h"
#include "winnt.h"
#include "heap.h"
#include "task.h"
#include "message.h"
#include "miscemu.h"
#include "wine/port.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(winsock)

/* ----------------------------------- helper functions - */

static int list_size(char** l, int item_size)
{
  int i,j = 0;
  if(l)
  { for(i=0;l[i];i++) 
	j += (item_size) ? item_size : strlen(l[i]) + 1;
    j += (i + 1) * sizeof(char*); }
  return j;
}

static int list_dup(char** l_src, char* ref, char* base, int item_size)
{ 
   /* base is either either equal to ref or 0 or SEGPTR */

   char*		p = ref;
   char**		l_to = (char**)ref;
   int			i,j,k;

   for(j=0;l_src[j];j++) ;
   p += (j + 1) * sizeof(char*);
   for(i=0;i<j;i++)
   { l_to[i] = base + (p - ref);
     k = ( item_size ) ? item_size : strlen(l_src[i]) + 1;
     memcpy(p, l_src[i], k); p += k; }
   l_to[i] = NULL;
   return (p - ref);
}

/* ----- hostent */

static int hostent_size(struct hostent* p_he)
{
  int size = 0;
  if( p_he )
  { size  = sizeof(struct hostent); 
    size += strlen(p_he->h_name) + 1;
    size += list_size(p_he->h_aliases, 0);  
    size += list_size(p_he->h_addr_list, p_he->h_length ); }
  return size;
}

/* Copy hostent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_he(struct ws_hostent *p_to,char *p_base,int t_size,struct hostent* p_he)
{
	char* p_name,*p_aliases,*p_addr,*p;
	int	size=hostent_size(p_he)+(sizeof(struct ws_hostent)-sizeof(struct hostent));

	if (t_size < size)
		return -size;
	p = (char*)p_to;
	p += sizeof(struct ws_hostent);
	p_name = p;
	strcpy(p, p_he->h_name); p += strlen(p) + 1;
	p_aliases = p;
	p += list_dup(p_he->h_aliases, p, p_base + (p - (char*)p_to), 0);
	p_addr = p;
	list_dup(p_he->h_addr_list, p, p_base + (p - (char*)p_to), p_he->h_length);

	p_to->h_addrtype = (INT16)p_he->h_addrtype; 
	p_to->h_length = (INT16)p_he->h_length;
	p_to->h_name = (SEGPTR)(p_base + (p_name - (char*)p_to));
	p_to->h_aliases = (SEGPTR)(p_base + (p_aliases - (char*)p_to));
	p_to->h_addr_list = (SEGPTR)(p_base + (p_addr - (char*)p_to));

	return size;
}

/* ----- protoent */

static int protoent_size(struct protoent* p_pe)
{
  int size = 0;
  if( p_pe )
  { size  = sizeof(struct protoent);
    size += strlen(p_pe->p_name) + 1;
    size += list_size(p_pe->p_aliases, 0); }
  return size;
}

/* Copy protoent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_pe(struct ws_protoent *p_to,char *p_base,int t_size,struct protoent* p_pe)
{
	char* p_name,*p_aliases,*p;
	int	size=protoent_size(p_pe)+(sizeof(struct ws_protoent)-sizeof(struct protoent));

	if (t_size < size)
		return -size;
	p = (char*)p_to;
	p += sizeof(struct ws_protoent);
	p_name = p;
	strcpy(p, p_pe->p_name); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_pe->p_aliases, p, p_base + (p - (char*)p_to), 0);

	p_to->p_proto = (INT16)p_pe->p_proto;
	p_to->p_name = (SEGPTR)(p_base) + (p_name - (char*)p_to);
	p_to->p_aliases = (SEGPTR)((p_base) + (p_aliases - (char*)p_to)); 


	return size;
}

/* ----- servent */

static int servent_size(struct servent* p_se)
{
	int size = 0;
	if( p_se ) {
		size += sizeof(struct servent);
		size += strlen(p_se->s_proto) + strlen(p_se->s_name) + 2;
		size += list_size(p_se->s_aliases, 0);
	}
	return size;
}

/* Copy servent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_se(struct ws_servent *p_to,char *p_base,int t_size,struct servent* p_se)
{
	char* p_name,*p_aliases,*p_proto,*p;
	int	size = servent_size(p_se)+(sizeof(struct ws_servent)-sizeof(struct servent));

	if (t_size < size )
		return -size;
	p = (char*)p_to;
	p += sizeof(struct ws_servent);
	p_name = p;
	strcpy(p, p_se->s_name); p += strlen(p) + 1;
	p_proto = p;
	strcpy(p, p_se->s_proto); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_se->s_aliases, p, p_base + (p - (char*)p_to), 0);

	p_to->s_port = (INT16)p_se->s_port;
	p_to->s_name = (SEGPTR)(p_base + (p_name - (char*)p_to));
	p_to->s_proto = (SEGPTR)(p_base + (p_proto - (char*)p_to));
	p_to->s_aliases = (SEGPTR)(p_base + (p_aliases - (char*)p_to)); 

	return size;
}

static HANDLE __ws_async_handle = 0xdead;

/* Generic async query struct. we use symbolic names for the different queries
 * for readability.
 */
typedef struct _async_query {
	HWND16		hWnd;
	UINT16		uMsg;
	LPCSTR		ptr1;
#define host_name	ptr1
#define host_addr	ptr1
#define serv_name	ptr1
#define proto_name	ptr1
	LPCSTR		ptr2;
#define serv_proto	ptr2
	int		int1;
#define host_len	int1
#define proto_number	int1
#define serv_port	int1
	int		int2;
#define host_type	int2
	SEGPTR		sbuf;
	INT16		sbuflen;

	HANDLE16	async_handle;
	int		flags;
#define AQ_WIN16	0
#define AQ_WIN32	4
#define HB_WIN32(hb) (hb->flags & AQ_WIN32)
#define AQ_NUMBER	0
#define AQ_NAME		8

#define AQ_GETHOST	0
#define AQ_GETPROTO	1
#define AQ_GETSERV	2
#define AQ_GETMASK	3
	int		qt;
} async_query;

/****************************************************************************
 * The async query function.
 *
 * It is either called as a thread startup routine or directly. It has
 * to free the passed arg from the process heap and PostMessageA the async
 * result or the error code.
 * 
 * FIXME:
 *	- errorhandling not verified.
 */
static DWORD WINAPI _async_queryfun(LPVOID arg) {
	async_query	*aq = (async_query*)arg;
	int		size = 0;
	WORD		fail = 0;
	char		*targetptr = (HB_WIN32(aq)?(char*)aq->sbuf:(char*)PTR_SEG_TO_LIN(aq->sbuf));

	switch (aq->flags & AQ_GETMASK) {
	case AQ_GETHOST: {
			struct hostent		*he;
			struct ws_hostent	*wshe = (struct ws_hostent*)targetptr;

			he = (aq->flags & AQ_NAME) ?
				gethostbyname(aq->host_name):
				gethostbyaddr(aq->host_addr,aq->host_len,aq->host_type);
			if (he) {
				size = WS_copy_he(wshe,(char*)aq->sbuf,aq->sbuflen,he);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			} else {
				fail = WSAENOBUFS;
			}
		}
		break;
	case AQ_GETPROTO: {
			struct	protoent	*pe;
			struct ws_protoent	*wspe = (struct ws_protoent*)targetptr;
			pe = (aq->flags & AQ_NAME)?
				getprotobyname(aq->proto_name) : 
				getprotobynumber(aq->proto_number);
			if (pe) {
				size = WS_copy_pe(wspe,(char*)aq->sbuf,aq->sbuflen,pe);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			} else {
				fail = WSAENOBUFS;
			}
		}
		break;
	case AQ_GETSERV: {
			struct	servent	*se;
			struct ws_servent	*wsse = (struct ws_servent*)targetptr;
			se = (aq->flags & AQ_NAME)?
				getservbyname(aq->serv_name,aq->serv_proto) : 
				getservbyport(aq->serv_port,aq->serv_proto);
			if (se) {
				size = WS_copy_se(wsse,(char*)aq->sbuf,aq->sbuflen,se);
				if (size < 0) {
					fail = WSAENOBUFS;
					size = -size;
				}
			} else {
				fail = WSAENOBUFS;
			}
		}
		break;
	}
	PostMessageA(aq->hWnd,aq->uMsg,aq->async_handle,size|(fail<<16));
	HeapFree(GetProcessHeap(),0,arg);
	return 0;
}

/****************************************************************************
 * The main async help function. 
 * 
 * It either starts a thread or just calls the function directly for platforms
 * with no thread support. This relies on the fact that PostMessage() does
 * not actually call the windowproc before the function returns.
 */
static HANDLE16	__WSAsyncDBQuery(
	HWND hWnd, UINT uMsg,INT int1,LPCSTR ptr1, INT int2, LPCSTR ptr2,
	void *sbuf, INT sbuflen, UINT flags
) {
	async_query	*aq = HeapAlloc(GetProcessHeap(),0,sizeof(async_query));
	HANDLE	hthread;

	aq->hWnd	= hWnd;
	aq->uMsg	= uMsg;
	aq->int1	= int1;
	aq->ptr1	= ptr1;
	aq->int2	= int2;
	aq->ptr2	= ptr2;
	aq->async_handle = ++__ws_async_handle;
	aq->flags	= flags;
	aq->sbuf	= (SEGPTR)sbuf;
	aq->sbuflen	= sbuflen;
#if 1
	hthread = CreateThread(NULL,0,_async_queryfun,aq,0,NULL);
	if (hthread==INVALID_HANDLE_VALUE)
#endif
		_async_queryfun(aq);
	return __ws_async_handle;
}


/***********************************************************************
 *       WSAAsyncGetHostByAddr()	(WINSOCK.102)
 */
HANDLE16 WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 uMsg, LPCSTR addr,
                               INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, addr %08x[%i]\n",
	       hWnd, uMsg, (unsigned)addr , len );
	return __WSAsyncDBQuery(hWnd,uMsg,len,addr,type,NULL,(void*)sbuf,buflen,AQ_NUMBER|AQ_WIN16|AQ_GETHOST);
}

/***********************************************************************
 *       WSAAsyncGetHostByAddr()        (WSOCK32.102)
 */
HANDLE WINAPI WSAAsyncGetHostByAddr(HWND hWnd, UINT uMsg, LPCSTR addr,
                               INT len, INT type, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %04x, addr %08x[%i]\n",
	       hWnd, uMsg, (unsigned)addr , len );
	return __WSAsyncDBQuery(hWnd,uMsg,len,addr,type,NULL,sbuf,buflen,AQ_NUMBER|AQ_WIN32|AQ_GETHOST);
}

/***********************************************************************
 *       WSAAsyncGetHostByName()	(WINSOCK.103)
 */
HANDLE16 WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                      SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, host %s, buffer %i\n", hWnd, uMsg, (name)?name:"<null>", (int)buflen );

	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,(void*)sbuf,buflen,AQ_NAME|AQ_WIN16|AQ_GETHOST);
}                     

/***********************************************************************
 *       WSAAsyncGetHostByName32()	(WSOCK32.103)
 */
HANDLE WINAPI WSAAsyncGetHostByName(HWND hWnd, UINT uMsg, LPCSTR name, 
					LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %08x, host %s, buffer %i\n", 
	       (HWND16)hWnd, uMsg, (name)?name:"<null>", (int)buflen );
	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,sbuf,buflen,AQ_NAME|AQ_WIN32|AQ_GETHOST);
}

/***********************************************************************
 *       WSAAsyncGetProtoByName()	(WINSOCK.105)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                         SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %08x, protocol %s\n",
	       (HWND16)hWnd, uMsg, (name)?name:"<null>" );
	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,(void*)sbuf,buflen,AQ_GETPROTO|AQ_NAME|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetProtoByName()       (WSOCK32.105)
 */
HANDLE WINAPI WSAAsyncGetProtoByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                         LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %08x, protocol %s\n",
	       (HWND16)hWnd, uMsg, (name)?name:"<null>" );
	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,NULL,sbuf,buflen,AQ_GETPROTO|AQ_NAME|AQ_WIN32);
}


/***********************************************************************
 *       WSAAsyncGetProtoByNumber()	(WINSOCK.104)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd,UINT16 uMsg,INT16 number,
                                           SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, num %i\n", hWnd, uMsg, number );
	return __WSAsyncDBQuery(hWnd,uMsg,number,NULL,0,NULL,(void*)sbuf,buflen,AQ_GETPROTO|AQ_NUMBER|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetProtoByNumber()     (WSOCK32.104)
 */
HANDLE WINAPI WSAAsyncGetProtoByNumber(HWND hWnd, UINT uMsg, INT number,
                                           LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %04x, num %i\n", hWnd, uMsg, number );

	return __WSAsyncDBQuery(hWnd,uMsg,number,NULL,0,NULL,sbuf,buflen,AQ_GETPROTO|AQ_NUMBER|AQ_WIN32);
}

/***********************************************************************
 *       WSAAsyncGetServByName()	(WINSOCK.107)
 */
HANDLE16 WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name, 
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, name %s, proto %s\n",
	       hWnd, uMsg, (name)?name:"<null>", (proto)?proto:"<null>" );

	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,proto,(void*)sbuf,buflen,AQ_GETSERV|AQ_NAME|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetServByName()        (WSOCK32.107)
 */
HANDLE WINAPI WSAAsyncGetServByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %04x, name %s, proto %s\n",
	       hWnd, uMsg, (name)?name:"<null>", (proto)?proto:"<null>" );
	return __WSAsyncDBQuery(hWnd,uMsg,0,name,0,proto,sbuf,buflen,AQ_GETSERV|AQ_NAME|AQ_WIN32);
}

/***********************************************************************
 *       WSAAsyncGetServByPort()	(WINSOCK.106)
 */
HANDLE16 WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 uMsg, INT16 port, 
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
	TRACE("hwnd %04x, msg %04x, port %i, proto %s\n",
	       hWnd, uMsg, port, (proto)?proto:"<null>" );
	return __WSAsyncDBQuery(hWnd,uMsg,port,NULL,0,proto,(void*)sbuf,buflen,AQ_GETSERV|AQ_NUMBER|AQ_WIN16);
}

/***********************************************************************
 *       WSAAsyncGetServByPort()        (WSOCK32.106)
 */
HANDLE WINAPI WSAAsyncGetServByPort(HWND hWnd, UINT uMsg, INT port,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
	TRACE("hwnd %04x, msg %04x, port %i, proto %s\n",
	       hWnd, uMsg, port, (proto)?proto:"<null>" );
	return __WSAsyncDBQuery(hWnd,uMsg,port,NULL,0,proto,sbuf,buflen,AQ_GETSERV|AQ_NUMBER|AQ_WIN32);
}

/***********************************************************************
 *       WSACancelAsyncRequest()	(WINSOCK.108)(WSOCK32.109)
 */
INT WINAPI WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{
    FIXME("(%08x),stub\n", hAsyncTaskHandle);
    return 0;
}

INT16 WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle)
{
    return (HANDLE16)WSACancelAsyncRequest((HANDLE)hAsyncTaskHandle);
}
