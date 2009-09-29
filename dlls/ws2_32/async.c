/* Async WINSOCK DNS services
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka.
 * Copyright (C) 1999 Marcus Meissner
 * Copyright (C) 2009 Alexandre Julliard
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
#include "wine/port.h"

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winsock2.h"
#include "ws2spi.h"
#include "wownt32.h"
#include "wine/winsock16.h"
#include "winnt.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);


/* critical section to protect some non-reentrant net function */
CRITICAL_SECTION csWSgetXXXbyYYY;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csWSgetXXXbyYYY,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csWSgetXXXbyYYY") }
};
CRITICAL_SECTION csWSgetXXXbyYYY = { &critsect_debug, -1, 0, 0, 0, 0 };

#define AQ_WIN16	0x00
#define AQ_WIN32	0x04
#define HB_WIN32(hb) (hb->query.flags & AQ_WIN32)

/* The handles used are pseudo-handles that can be simply casted. */
/* 16-bit values are used internally (to be sure handle comparison works right in 16-bit apps). */
#define WSA_H32(h16) ((HANDLE)(ULONG_PTR)(h16))
#define WSA_H16(h32) (LOWORD(h32))

struct async_query_header
{
    HWND   hWnd;
    UINT   uMsg;
    void  *sbuf;
    INT    sbuflen;
    UINT   flags;
    HANDLE handle;
};

struct async_query_gethostbyname
{
    struct async_query_header query;
    char *host_name;
};

struct async_query_gethostbyaddr
{
    struct async_query_header query;
    char *host_addr;
    int   host_len;
    int   host_type;
};

struct async_query_getprotobyname
{
    struct async_query_header query;
    char *proto_name;
};

struct async_query_getprotobynumber
{
    struct async_query_header query;
    int   proto_number;
};

struct async_query_getservbyname
{
    struct async_query_header query;
    char *serv_name;
    char *serv_proto;
};

struct async_query_getservbyport
{
    struct async_query_header query;
    char *serv_proto;
    int   serv_port;
};


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

static DWORD finish_query( struct async_query_header *query, LPARAM lparam )
{
    PostMessageW( query->hWnd, query->uMsg, (WPARAM)query->handle, lparam );
    HeapFree( GetProcessHeap(), 0, query );
    return 0;
}

/* ----- hostent */

static int hostent_size(const struct WS_hostent* p_he)
{
  int size = 0;
  if( p_he )
  { size  = sizeof(struct WS_hostent);
    size += strlen(p_he->h_name) + 1;
    size += list_size(p_he->h_aliases, 0);
    size += list_size(p_he->h_addr_list, p_he->h_length ); }
  return size;
}

/* Copy hostent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_he(char *p_to,char *p_base,int t_size,const struct WS_hostent* p_he, int flag)
{
	char* p_name,*p_aliases,*p_addr,*p;
	struct ws_hostent16 *p_to16 = (struct ws_hostent16*)p_to;
	struct WS_hostent *p_to32 = (struct WS_hostent*)p_to;
	int	size = hostent_size(p_he) +
		(
		 (flag & AQ_WIN32) ? sizeof(struct WS_hostent) : sizeof(struct ws_hostent16)
		- sizeof(struct WS_hostent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN32) ?
	     sizeof(struct WS_hostent) : sizeof(struct ws_hostent16);
	p_name = p;
	strcpy(p, p_he->h_name); p += strlen(p) + 1;
	p_aliases = p;
	p += list_dup(p_he->h_aliases, p, p_base + (p - p_to), 0);
	p_addr = p;
	list_dup(p_he->h_addr_list, p, p_base + (p - p_to), p_he->h_length);

	if (flag & AQ_WIN32)
	{
	    p_to32->h_addrtype = p_he->h_addrtype;
	    p_to32->h_length = p_he->h_length;
	    p_to32->h_name = (p_base + (p_name - p_to));
	    p_to32->h_aliases = (char **)(p_base + (p_aliases - p_to));
	    p_to32->h_addr_list = (char **)(p_base + (p_addr - p_to));
	}
	else
	{
	    p_to16->h_addrtype = (INT16)p_he->h_addrtype;
	    p_to16->h_length = (INT16)p_he->h_length;
	    p_to16->h_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->h_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	    p_to16->h_addr_list = (SEGPTR)(p_base + (p_addr - p_to));
	}

	return size;
}

static DWORD WINAPI async_gethostbyname(LPVOID arg)
{
    struct async_query_gethostbyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_hostent *he;
    char *copy_hostent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    he = WS_gethostbyname(aq->host_name);
    if (he) {
        size = WS_copy_he(copy_hostent, aq->query.sbuf, aq->query.sbuflen, he, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_gethostbyaddr(LPVOID arg)
{
    struct async_query_gethostbyaddr *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_hostent *he;
    char *copy_hostent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    he = WS_gethostbyaddr(aq->host_addr,aq->host_len,aq->host_type);
    if (he) {
        size = WS_copy_he(copy_hostent, aq->query.sbuf, aq->query.sbuflen, he, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

/* ----- protoent */

static int protoent_size(const struct WS_protoent* p_pe)
{
  int size = 0;
  if( p_pe )
  { size  = sizeof(struct WS_protoent);
    size += strlen(p_pe->p_name) + 1;
    size += list_size(p_pe->p_aliases, 0); }
  return size;
}

/* Copy protoent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 */
static int WS_copy_pe(char *p_to,char *p_base,int t_size,const struct WS_protoent* p_pe, int flag)
{
	char* p_name,*p_aliases,*p;
	struct ws_protoent16 *p_to16 = (struct ws_protoent16*)p_to;
	struct WS_protoent *p_to32 = (struct WS_protoent*)p_to;
	int	size = protoent_size(p_pe) +
		(
		 (flag & AQ_WIN32) ? sizeof(struct WS_protoent) : sizeof(struct ws_protoent16)
		- sizeof(struct WS_protoent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN32) ? sizeof(struct WS_protoent) : sizeof(struct ws_protoent16);
	p_name = p;
	strcpy(p, p_pe->p_name); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_pe->p_aliases, p, p_base + (p - p_to), 0);

	if (flag & AQ_WIN32)
	{
	    p_to32->p_proto = p_pe->p_proto;
	    p_to32->p_name = (p_base) + (p_name - p_to);
	    p_to32->p_aliases = (char **)((p_base) + (p_aliases - p_to));
	}
	else
	{
	    p_to16->p_proto = (INT16)p_pe->p_proto;
	    p_to16->p_name = (SEGPTR)(p_base) + (p_name - p_to);
	    p_to16->p_aliases = (SEGPTR)((p_base) + (p_aliases - p_to));
	}

	return size;
}

static DWORD WINAPI async_getprotobyname(LPVOID arg)
{
    struct async_query_getprotobyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_protoent *pe;
    char *copy_protoent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    pe = WS_getprotobyname(aq->proto_name);
    if (pe) {
        size = WS_copy_pe(copy_protoent, aq->query.sbuf, aq->query.sbuflen, pe, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getprotobynumber(LPVOID arg)
{
    struct async_query_getprotobynumber *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_protoent *pe;
    char *copy_protoent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    pe = WS_getprotobynumber(aq->proto_number);
    if (pe) {
        size = WS_copy_pe(copy_protoent, aq->query.sbuf, aq->query.sbuflen, pe, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

/* ----- servent */

static int servent_size(const struct WS_servent* p_se)
{
	int size = 0;
	if( p_se ) {
		size += sizeof(struct WS_servent);
		size += strlen(p_se->s_proto) + strlen(p_se->s_name) + 2;
		size += list_size(p_se->s_aliases, 0);
	}
	return size;
}

/* Copy servent to p_to, fix up inside pointers using p_base (different for
 * Win16 (linear vs. segmented). Return -neededsize on overrun.
 * Take care of different Win16/Win32 servent structs (packing !)
 */
static int WS_copy_se(char *p_to,char *p_base,int t_size,const struct WS_servent* p_se, int flag)
{
	char* p_name,*p_aliases,*p_proto,*p;
	struct ws_servent16 *p_to16 = (struct ws_servent16*)p_to;
	struct WS_servent *p_to32 = (struct WS_servent*)p_to;
	int	size = servent_size(p_se) +
		(
		 (flag & AQ_WIN32) ? sizeof(struct WS_servent) : sizeof(struct ws_servent16)
		- sizeof(struct WS_servent)
		);

	if (t_size < size)
		return -size;
	p = p_to;
	p += (flag & AQ_WIN32) ? sizeof(struct WS_servent) : sizeof(struct ws_servent16);
	p_name = p;
	strcpy(p, p_se->s_name); p += strlen(p) + 1;
	p_proto = p;
	strcpy(p, p_se->s_proto); p += strlen(p) + 1;
	p_aliases = p;
	list_dup(p_se->s_aliases, p, p_base + (p - p_to), 0);

	if (flag & AQ_WIN32)
	{
	    p_to32->s_port = p_se->s_port;
	    p_to32->s_name = (p_base + (p_name - p_to));
	    p_to32->s_proto = (p_base + (p_proto - p_to));
	    p_to32->s_aliases = (char **)(p_base + (p_aliases - p_to));
	}
	else
	{
	    p_to16->s_port = (INT16)p_se->s_port;
	    p_to16->s_name = (SEGPTR)(p_base + (p_name - p_to));
	    p_to16->s_proto = (SEGPTR)(p_base + (p_proto - p_to));
	    p_to16->s_aliases = (SEGPTR)(p_base + (p_aliases - p_to));
	}

	return size;
}

static DWORD WINAPI async_getservbyname(LPVOID arg)
{
    struct async_query_getservbyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_servent *se;
    char *copy_servent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    se = WS_getservbyname(aq->serv_name,aq->serv_proto);
    if (se) {
        size = WS_copy_se(copy_servent, aq->query.sbuf, aq->query.sbuflen, se, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getservbyport(LPVOID arg)
{
    struct async_query_getservbyport *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct WS_servent *se;
    char *copy_servent = HB_WIN32(aq) ? aq->query.sbuf : MapSL((SEGPTR)aq->query.sbuf);

    se = WS_getservbyport(aq->serv_port,aq->serv_proto);
    if (se) {
        size = WS_copy_se(copy_servent, aq->query.sbuf, aq->query.sbuflen, se, aq->query.flags);
        if (size < 0) {
            fail = WSAENOBUFS;
            size = -size;
        }
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}


/****************************************************************************
 * The main async help function.
 *
 * It either starts a thread or just calls the function directly for platforms
 * with no thread support. This relies on the fact that PostMessage() does
 * not actually call the windowproc before the function returns.
 */
static HANDLE run_query( HWND hWnd, UINT uMsg, LPTHREAD_START_ROUTINE func,
                         struct async_query_header *query, void *sbuf, INT sbuflen, UINT flags )
{
    static LONG next_handle = 0xdead;
    HANDLE thread;
    ULONG handle = LOWORD( InterlockedIncrement( &next_handle ));

    /* avoid handle 0 */
    while (!handle) handle = LOWORD( InterlockedIncrement( &next_handle ));

    query->hWnd    = hWnd;
    query->uMsg    = uMsg;
    query->handle  = UlongToHandle( handle );
    query->flags   = flags;
    query->sbuf    = sbuf;
    query->sbuflen = sbuflen;

    thread = CreateThread( NULL, 0, func, query, 0, NULL );
    if (!thread)
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    CloseHandle( thread );
    return UlongToHandle( handle );
}


/***********************************************************************
 *       WSAAsyncGetHostByAddr	(WINSOCK.102)
 */
HANDLE16 WINAPI WSAAsyncGetHostByAddr16(HWND16 hWnd, UINT16 uMsg, LPCSTR addr,
                               INT16 len, INT16 type, SEGPTR sbuf, INT16 buflen)
{
    struct async_query_gethostbyaddr *aq;

    TRACE("hwnd %04x, msg %04x, addr %p[%i]\n", hWnd, uMsg, addr, len );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_addr = (char *)(aq + 1);
    aq->host_len  = len;
    aq->host_type = type;
    memcpy( aq->host_addr, addr, len );
    return WSA_H16( run_query( HWND_32(hWnd), uMsg, async_gethostbyaddr, &aq->query,
                               (void*)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetHostByAddr        (WS2_32.102)
 */
HANDLE WINAPI WSAAsyncGetHostByAddr(HWND hWnd, UINT uMsg, LPCSTR addr,
                               INT len, INT type, LPSTR sbuf, INT buflen)
{
    struct async_query_gethostbyaddr *aq;

    TRACE("hwnd %p, msg %04x, addr %p[%i]\n", hWnd, uMsg, addr, len );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_addr = (char *)(aq + 1);
    aq->host_len  = len;
    aq->host_type = type;
    memcpy( aq->host_addr, addr, len );
    return run_query( hWnd, uMsg, async_gethostbyaddr, &aq->query, sbuf, buflen, AQ_WIN32 );
}

/***********************************************************************
 *       WSAAsyncGetHostByName	(WINSOCK.103)
 */
HANDLE16 WINAPI WSAAsyncGetHostByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                      SEGPTR sbuf, INT16 buflen)
{
    struct async_query_gethostbyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %04x, msg %04x, host %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_name = (char *)(aq + 1);
    strcpy( aq->host_name, name );
    return WSA_H16( run_query( HWND_32(hWnd), uMsg, async_gethostbyname, &aq->query,
                               (void*)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetHostByName	(WS2_32.103)
 */
HANDLE WINAPI WSAAsyncGetHostByName(HWND hWnd, UINT uMsg, LPCSTR name,
					LPSTR sbuf, INT buflen)
{
    struct async_query_gethostbyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %p, msg %04x, host %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->host_name = (char *)(aq + 1);
    strcpy( aq->host_name, name );
    return run_query( hWnd, uMsg, async_gethostbyname, &aq->query, sbuf, buflen, AQ_WIN32 );
}

/***********************************************************************
 *       WSAAsyncGetProtoByName	(WINSOCK.105)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                         SEGPTR sbuf, INT16 buflen)
{
    struct async_query_getprotobyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %04x, msg %04x, proto %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_name = (char *)(aq + 1);
    strcpy( aq->proto_name, name );
    return WSA_H16( run_query( HWND_32(hWnd), uMsg, async_getprotobyname, &aq->query,
                               (void*)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetProtoByName       (WS2_32.105)
 */
HANDLE WINAPI WSAAsyncGetProtoByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                         LPSTR sbuf, INT buflen)
{
    struct async_query_getprotobyname *aq;
    unsigned int len = strlen(name) + 1;

    TRACE("hwnd %p, msg %04x, proto %s, buffer %i\n", hWnd, uMsg, debugstr_a(name), buflen );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_name = (char *)(aq + 1);
    strcpy( aq->proto_name, name );
    return run_query( hWnd, uMsg, async_getprotobyname, &aq->query, sbuf, buflen, AQ_WIN32 );
}


/***********************************************************************
 *       WSAAsyncGetProtoByNumber	(WINSOCK.104)
 */
HANDLE16 WINAPI WSAAsyncGetProtoByNumber16(HWND16 hWnd,UINT16 uMsg,INT16 number,
                                           SEGPTR sbuf, INT16 buflen)
{
    struct async_query_getprotobynumber *aq;

    TRACE("hwnd %04x, msg %04x, num %i\n", hWnd, uMsg, number );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_number = number;
    return WSA_H16( run_query( HWND_32(hWnd), uMsg, async_getprotobynumber, &aq->query,
                               (void *)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetProtoByNumber     (WS2_32.104)
 */
HANDLE WINAPI WSAAsyncGetProtoByNumber(HWND hWnd, UINT uMsg, INT number,
                                           LPSTR sbuf, INT buflen)
{
    struct async_query_getprotobynumber *aq;

    TRACE("hwnd %p, msg %04x, num %i\n", hWnd, uMsg, number );

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->proto_number = number;
    return run_query( hWnd, uMsg, async_getprotobynumber, &aq->query, sbuf, buflen, AQ_WIN32 );
}

/***********************************************************************
 *       WSAAsyncGetServByName	(WINSOCK.107)
 */
HANDLE16 WINAPI WSAAsyncGetServByName16(HWND16 hWnd, UINT16 uMsg, LPCSTR name,
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
    struct async_query_getservbyname *aq;
    unsigned int len1 = strlen(name) + 1;
    unsigned int len2 = strlen(proto) + 1;

    TRACE("hwnd %04x, msg %04x, name %s, proto %s\n", hWnd, uMsg, debugstr_a(name), debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len1 + len2 )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->serv_name  = (char *)(aq + 1);
    aq->serv_proto = aq->serv_name + len1;
    strcpy( aq->serv_name, name );
    strcpy( aq->serv_proto, proto );
    return WSA_H16( run_query( HWND_32(hWnd), uMsg,async_getservbyname, &aq->query,
                               (void *)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetServByName        (WS2_32.107)
 */
HANDLE WINAPI WSAAsyncGetServByName(HWND hWnd, UINT uMsg, LPCSTR name,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
    struct async_query_getservbyname *aq;
    unsigned int len1 = strlen(name) + 1;
    unsigned int len2 = strlen(proto) + 1;

    TRACE("hwnd %p, msg %04x, name %s, proto %s\n", hWnd, uMsg, debugstr_a(name), debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len1 + len2 )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->serv_name  = (char *)(aq + 1);
    aq->serv_proto = aq->serv_name + len1;
    strcpy( aq->serv_name, name );
    strcpy( aq->serv_proto, proto );
    return run_query( hWnd, uMsg, async_getservbyname, &aq->query, sbuf, buflen, AQ_WIN32 );
}

/***********************************************************************
 *       WSAAsyncGetServByPort	(WINSOCK.106)
 */
HANDLE16 WINAPI WSAAsyncGetServByPort16(HWND16 hWnd, UINT16 uMsg, INT16 port,
                                        LPCSTR proto, SEGPTR sbuf, INT16 buflen)
{
    struct async_query_getservbyport *aq;
    unsigned int len = strlen(proto) + 1;

    TRACE("hwnd %04x, msg %04x, port %i, proto %s\n", hWnd, uMsg, port, debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->serv_proto = (char *)(aq + 1);
    aq->serv_port  = port;
    strcpy( aq->serv_proto, proto );
    return WSA_H16( run_query( HWND_32(hWnd), uMsg, async_getservbyport, &aq->query,
                               (void *)sbuf, buflen, AQ_WIN16 ));
}

/***********************************************************************
 *       WSAAsyncGetServByPort        (WS2_32.106)
 */
HANDLE WINAPI WSAAsyncGetServByPort(HWND hWnd, UINT uMsg, INT port,
                                        LPCSTR proto, LPSTR sbuf, INT buflen)
{
    struct async_query_getservbyport *aq;
    unsigned int len = strlen(proto) + 1;

    TRACE("hwnd %p, msg %04x, port %i, proto %s\n", hWnd, uMsg, port, debugstr_a(proto));

    if (!(aq = HeapAlloc( GetProcessHeap(), 0, sizeof(*aq) + len )))
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    aq->serv_proto = (char *)(aq + 1);
    aq->serv_port  = port;
    strcpy( aq->serv_proto, proto );
    return run_query( hWnd, uMsg, async_getservbyport, &aq->query, sbuf, buflen, AQ_WIN32 );
}

/***********************************************************************
 *       WSACancelAsyncRequest	(WS2_32.108)
 */
INT WINAPI WSACancelAsyncRequest(HANDLE hAsyncTaskHandle)
{
    FIXME("(%p),stub\n", hAsyncTaskHandle);
    return 0;
}

/***********************************************************************
 *       WSACancelAsyncRequest	(WINSOCK.108)
 */
INT16 WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle)
{
    return (INT16)WSACancelAsyncRequest(WSA_H32 (hAsyncTaskHandle));
}

/***********************************************************************
 *       WSApSetPostRoutine	(WS2_32.24)
 */
INT WINAPI WSApSetPostRoutine(LPWPUPOSTMESSAGE lpPostRoutine)
{
    FIXME("(%p), stub !\n", lpPostRoutine);
    return 0;
}

/***********************************************************************
 *        (WS2_32.25)
 */
WSAEVENT WINAPI WPUCompleteOverlappedRequest(SOCKET s, LPWSAOVERLAPPED overlapped,
                                             DWORD error, DWORD transferred, LPINT errcode)
{
    FIXME("(0x%08lx,%p,0x%08x,0x%08x,%p), stub !\n", s, overlapped, error, transferred, errcode);

    if (errcode)
        *errcode = WSAEINVAL;

    return NULL;
}
