/*
 * 16-bit socket functions
 *
 * Copyright (C) 1993,1994,1996,1997 John Brezak, Erik Bos, Alex Korobka
 * Copyright (C) 2003 Alexandre Julliard
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

#include "winsock2.h"
#include "wine/winbase16.h"
#include "winsock16.h"
#include "wownt32.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winsock);

struct async_query_header
{
    HWND     hWnd;
    UINT     uMsg;
    SEGPTR   sbuf;
    INT      sbuflen;
    HANDLE16 handle;
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

static INT num_startup;  /* reference counter */
static void *he_buffer;
static SEGPTR he_buffer_seg;
static void *se_buffer;
static SEGPTR se_buffer_seg;
static void *pe_buffer;
static SEGPTR pe_buffer_seg;
static SEGPTR dbuffer_seg;

static fd_set *ws_fdset_16_to_32( const ws_fd_set16 *set16, fd_set *set32 )
{
    UINT i;
    set32->fd_count = set16->fd_count;
    for (i = 0; i < set32->fd_count; i++) set32->fd_array[i] = set16->fd_array[i];
    return set32;
}

static ws_fd_set16 *ws_fdset_32_to_16( const fd_set *set32, ws_fd_set16 *set16 )
{
    UINT i;
    set16->fd_count = set32->fd_count;
    for (i = 0; i < set16->fd_count; i++) set16->fd_array[i] = set32->fd_array[i];
    return set16;
}

static DWORD finish_query( struct async_query_header *query, LPARAM lparam )
{
    PostMessageW( query->hWnd, query->uMsg, (WPARAM)query->handle, lparam );
    HeapFree( GetProcessHeap(), 0, query );
    return 0;
}

static int list_size(char** l, int item_size)
{
    int i,j = 0;
    if(l)
    {
        for(i=0;l[i];i++)
            j += (item_size) ? item_size : strlen(l[i]) + 1;
        j += (i + 1) * sizeof(char*);
    }
    return j;
}

static int list_dup(char** l_src, SEGPTR base, int item_size)
{
    int i, offset;
    char *ref = MapSL(base);
    SEGPTR *l_to = (SEGPTR *)ref;

    for (i = 0; l_src[i]; i++) ;
    offset = (i + 1) * sizeof(char*);
    for (i = 0; l_src[i]; i++)
    {
        int count = item_size ? item_size : strlen(l_src[i]) + 1;
        memcpy( ref + offset, l_src[i], count );
        l_to[i] = base + offset;
        offset += count;
    }
    l_to[i] = 0;
    return offset;
}

static SEGPTR get_buffer_he(int size)
{
    static int he_len;
    if (he_buffer)
    {
        if (he_len >= size ) return he_buffer_seg;
        UnMapLS( he_buffer_seg );
        HeapFree( GetProcessHeap(), 0, he_buffer );
    }
    he_buffer = HeapAlloc( GetProcessHeap(), 0, (he_len = size) );
    he_buffer_seg = MapLS( he_buffer );
    return he_buffer_seg;
}

static SEGPTR get_buffer_se(int size)
{
    static int se_len;
    if (se_buffer)
    {
        if (se_len >= size ) return se_buffer_seg;
        UnMapLS( se_buffer_seg );
        HeapFree( GetProcessHeap(), 0, se_buffer );
    }
    se_buffer = HeapAlloc( GetProcessHeap(), 0, (se_len = size) );
    se_buffer_seg = MapLS( se_buffer );
    return se_buffer_seg;
}

static SEGPTR get_buffer_pe(int size)
{
    static int pe_len;
    if (pe_buffer)
    {
        if (pe_len >= size ) return pe_buffer_seg;
        UnMapLS( pe_buffer_seg );
        HeapFree( GetProcessHeap(), 0, pe_buffer );
    }
    pe_buffer = HeapAlloc( GetProcessHeap(), 0, (pe_len = size) );
    pe_buffer_seg = MapLS( pe_buffer );
    return pe_buffer_seg;
}

/* duplicate hostent entry
 * and handle all Win16/Win32 dependent things (struct size, ...) *correctly*.
 * Ditto for protoent and servent.
 */
static SEGPTR ws_hostent_32_to_16( const struct hostent* he, SEGPTR base, int *buff_size )
{
    char *p;
    struct ws_hostent16 *p_to;

    int size = (sizeof(*p_to) +
                strlen(he->h_name) + 1 +
                list_size(he->h_aliases, 0) +
                list_size(he->h_addr_list, he->h_length));

    if (buff_size)
    {
        if (*buff_size < size)
        {
            *buff_size = size;
            return 0;
        }
        *buff_size = size;
    }
    else base = get_buffer_he(size);
    p_to = MapSL(base);

    p_to->h_addrtype = he->h_addrtype;
    p_to->h_length = he->h_length;

    p = (char *)(p_to + 1);
    p_to->h_name = base + (p - (char *)p_to);
    strcpy(p, he->h_name);
    p += strlen(p) + 1;

    p_to->h_aliases = base + (p - (char *)p_to);
    p += list_dup(he->h_aliases, p_to->h_aliases, 0);

    p_to->h_addr_list = base + (p - (char *)p_to);
    list_dup(he->h_addr_list, p_to->h_addr_list, he->h_length);

    return base;
}

static SEGPTR ws_protoent_32_to_16( const struct protoent *pe, SEGPTR base, int *buff_size )
{
    char *p;
    struct ws_protoent16 *p_to;

    int size = (sizeof(*p_to) +
                strlen(pe->p_name) + 1 +
                list_size(pe->p_aliases, 0));

    if (buff_size)
    {
        if (*buff_size < size)
        {
            *buff_size = size;
            return 0;
        }
        *buff_size = size;
    }
    else base = get_buffer_pe(size);
    p_to = MapSL(base);

    p_to->p_proto = pe->p_proto;
    p = (char *)(p_to + 1);

    p_to->p_name = base + (p - (char *)p_to);
    strcpy(p, pe->p_name);
    p += strlen(p) + 1;

    p_to->p_aliases = base + (p - (char *)p_to);
    list_dup(pe->p_aliases, p_to->p_aliases, 0);

    return base;
}

static SEGPTR ws_servent_32_to_16( const struct servent *se, SEGPTR base, int *buff_size )
{
    char *p;
    struct ws_servent16 *p_to;

    int size = (sizeof(*p_to) +
                strlen(se->s_proto) + 1 +
                strlen(se->s_name) + 1 +
                list_size(se->s_aliases, 0));

    if (buff_size)
    {
        if (*buff_size < size)
        {
            *buff_size = size;
            return 0;
        }
        *buff_size = size;
    }
    else base = get_buffer_se(size);
    p_to = MapSL(base);

    p_to->s_port = se->s_port;
    p = (char *)(p_to + 1);

    p_to->s_name = base + (p - (char *)p_to);
    strcpy(p, se->s_name);
    p += strlen(p) + 1;

    p_to->s_proto = base + (p - (char *)p_to);
    strcpy(p, se->s_proto);
    p += strlen(p) + 1;

    p_to->s_aliases = base + (p - (char *)p_to);
    list_dup(se->s_aliases, p_to->s_aliases, 0);

    return base;
}

static DWORD WINAPI async_gethostbyname(LPVOID arg)
{
    struct async_query_gethostbyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct hostent *he;

    if ((he = gethostbyname( aq->host_name )))
    {
        size = aq->query.sbuflen;
        if (!ws_hostent_32_to_16( he, aq->query.sbuf, &size )) fail = WSAENOBUFS;
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_gethostbyaddr(LPVOID arg)
{
    struct async_query_gethostbyaddr *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct hostent *he;

    if ((he = gethostbyaddr(aq->host_addr,aq->host_len,aq->host_type)))
    {
        size = aq->query.sbuflen;
        if (!ws_hostent_32_to_16( he, aq->query.sbuf, &size )) fail = WSAENOBUFS;
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getprotobyname(LPVOID arg)
{
    struct async_query_getprotobyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct protoent *pe;

    if ((pe = getprotobyname(aq->proto_name)))
    {
        size = aq->query.sbuflen;
        if (!ws_protoent_32_to_16( pe, aq->query.sbuf, &size )) fail = WSAENOBUFS;
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getprotobynumber(LPVOID arg)
{
    struct async_query_getprotobynumber *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct protoent *pe;

    if ((pe = getprotobynumber(aq->proto_number)))
    {
        size = aq->query.sbuflen;
        if (!ws_protoent_32_to_16( pe, aq->query.sbuf, &size )) fail = WSAENOBUFS;
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getservbyname(LPVOID arg)
{
    struct async_query_getservbyname *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct servent *se;

    if ((se = getservbyname(aq->serv_name,aq->serv_proto)))
    {
        size = aq->query.sbuflen;
        if (!ws_servent_32_to_16( se, aq->query.sbuf, &size )) fail = WSAENOBUFS;
    }
    else fail = GetLastError();

    return finish_query( &aq->query, MAKELPARAM( size, fail ));
}

static DWORD WINAPI async_getservbyport(LPVOID arg)
{
    struct async_query_getservbyport *aq = arg;
    int size = 0;
    WORD fail = 0;
    struct servent *se;

    if ((se = getservbyport(aq->serv_port,aq->serv_proto)))
    {
        size = aq->query.sbuflen;
        if (!ws_servent_32_to_16( se, aq->query.sbuf, &size )) fail = WSAENOBUFS;
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
static HANDLE16 run_query( HWND16 hWnd, UINT uMsg, LPTHREAD_START_ROUTINE func,
                           struct async_query_header *query, SEGPTR sbuf, INT sbuflen )
{
    static LONG next_handle = 0xdead;
    HANDLE thread;
    ULONG handle = LOWORD( InterlockedIncrement( &next_handle ));

    /* avoid handle 0 */
    while (!handle) handle = LOWORD( InterlockedIncrement( &next_handle ));

    query->hWnd    = HWND_32(hWnd);
    query->uMsg    = uMsg;
    query->handle  = handle;
    query->sbuf    = sbuf;
    query->sbuflen = sbuflen;

    thread = CreateThread( NULL, 0, func, query, 0, NULL );
    if (!thread)
    {
        SetLastError( WSAEWOULDBLOCK );
        return 0;
    }
    CloseHandle( thread );
    return handle;
}

/***********************************************************************
 *              accept		(WINSOCK.1)
 */
SOCKET16 WINAPI accept16(SOCKET16 s, struct sockaddr* addr, INT16* addrlen16 )
{
    INT addrlen32 = addrlen16 ? *addrlen16 : 0;
    SOCKET retSocket = accept( s, addr, &addrlen32 );
    if( addrlen16 ) *addrlen16 = addrlen32;
    return retSocket;
}

/***********************************************************************
 *              bind			(WINSOCK.2)
 */
INT16 WINAPI bind16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
    return bind( s, name, namelen );
}

/***********************************************************************
 *              closesocket           (WINSOCK.3)
 */
INT16 WINAPI closesocket16(SOCKET16 s)
{
    return closesocket(s);
}

/***********************************************************************
 *              connect               (WINSOCK.4)
 */
INT16 WINAPI connect16(SOCKET16 s, struct sockaddr *name, INT16 namelen)
{
    return connect( s, name, namelen );
}

/***********************************************************************
 *              getpeername		(WINSOCK.5)
 */
INT16 WINAPI getpeername16(SOCKET16 s, struct sockaddr *name, INT16 *namelen16)
{
    INT namelen32 = *namelen16;
    INT retVal = getpeername( s, name, &namelen32 );
   *namelen16 = namelen32;
    return retVal;
}

/***********************************************************************
 *              getsockname		(WINSOCK.6)
 */
INT16 WINAPI getsockname16(SOCKET16 s, struct sockaddr *name, INT16 *namelen16)
{
    INT retVal;

    if( namelen16 )
    {
        INT namelen32 = *namelen16;
        retVal = getsockname( s, name, &namelen32 );
       *namelen16 = namelen32;
    }
    else retVal = SOCKET_ERROR;
    return retVal;
}

/***********************************************************************
 *              getsockopt		(WINSOCK.7)
 */
INT16 WINAPI getsockopt16(SOCKET16 s, INT16 level, INT16 optname, char *optval, INT16 *optlen)
{
    INT optlen32;
    INT *p = &optlen32;
    INT retVal;

    if( optlen ) optlen32 = *optlen; else p = NULL;
    retVal = getsockopt( s, (WORD)level, optname, optval, p );
    if( optlen ) *optlen = optlen32;
    return retVal;
}

/***********************************************************************
 *		htonl			(WINSOCK.8)
 */
u_long WINAPI htonl16(u_long hostlong)
{
    return htonl(hostlong);
}


/***********************************************************************
 *		htons			(WINSOCK.9)
 */
u_short WINAPI htons16(u_short hostshort)
{
    return htons(hostshort);
}

/***********************************************************************
 *		inet_addr		(WINSOCK.10)
 */
u_long WINAPI inet_addr16(const char *cp)
{
    if (!cp) return INADDR_NONE;
    return inet_addr(cp);
}


/***********************************************************************
 *		inet_ntoa		(WINSOCK.11)
 */
SEGPTR WINAPI inet_ntoa16(struct in_addr in)
{
    char* retVal;
    if (!(retVal = inet_ntoa( in ))) return 0;
    if (!dbuffer_seg) dbuffer_seg = MapLS( retVal );
    return dbuffer_seg;
}

/***********************************************************************
 *              ioctlsocket           (WINSOCK.12)
 */
INT16 WINAPI ioctlsocket16(SOCKET16 s, LONG cmd, u_long *argp)
{
    return ioctlsocket( s, cmd, argp );
}

/***********************************************************************
 *              listen		(WINSOCK.13)
 */
INT16 WINAPI listen16(SOCKET16 s, INT16 backlog)
{
    return listen( s, backlog );
}

/***********************************************************************
 *		ntohl			(WINSOCK.14)
 */
u_long WINAPI ntohl16(u_long netlong)
{
    return ntohl(netlong);
}


/***********************************************************************
 *		ntohs			(WINSOCK.15)
 */
u_short WINAPI ntohs16(u_short netshort)
{
    return ntohs(netshort);
}

/***********************************************************************
 *              recv			(WINSOCK.16)
 */
INT16 WINAPI recv16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return recv( s, buf, len, flags );
}

/***********************************************************************
 *              recvfrom		(WINSOCK.17)
 */
INT16 WINAPI recvfrom16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                        struct sockaddr *from, INT16 *fromlen16)
{
    if (fromlen16)
    {
        INT fromlen32 = *fromlen16;
        INT retVal = recvfrom( s, buf, len, flags, from, &fromlen32 );
        *fromlen16 = fromlen32;
        return retVal;
    }
    else return recvfrom( s, buf, len, flags, from, NULL );
}

/***********************************************************************
 *		select			(WINSOCK.18)
 */
INT16 WINAPI select16(INT16 nfds, ws_fd_set16 *ws_readfds,
                      ws_fd_set16 *ws_writefds, ws_fd_set16 *ws_exceptfds,
                      struct timeval* timeout)
{
    fd_set read_set, write_set, except_set;
    fd_set *pread_set = NULL, *pwrite_set = NULL, *pexcept_set = NULL;
    int ret;

    if (ws_readfds) pread_set = ws_fdset_16_to_32( ws_readfds, &read_set );
    if (ws_writefds) pwrite_set = ws_fdset_16_to_32( ws_writefds, &write_set );
    if (ws_exceptfds) pexcept_set = ws_fdset_16_to_32( ws_exceptfds, &except_set );
    /* struct timeval is the same for both 32- and 16-bit code */
    ret = select( nfds, pread_set, pwrite_set, pexcept_set, timeout );
    if (ws_readfds) ws_fdset_32_to_16( &read_set, ws_readfds );
    if (ws_writefds) ws_fdset_32_to_16( &write_set, ws_writefds );
    if (ws_exceptfds) ws_fdset_32_to_16( &except_set, ws_exceptfds );
    return ret;
}

/***********************************************************************
 *              send			(WINSOCK.19)
 */
INT16 WINAPI send16(SOCKET16 s, char *buf, INT16 len, INT16 flags)
{
    return send( s, buf, len, flags );
}

/***********************************************************************
 *              sendto		(WINSOCK.20)
 */
INT16 WINAPI sendto16(SOCKET16 s, char *buf, INT16 len, INT16 flags,
                      struct sockaddr *to, INT16 tolen)
{
    return sendto( s, buf, len, flags, to, tolen );
}

/***********************************************************************
 *              setsockopt		(WINSOCK.21)
 */
INT16 WINAPI setsockopt16(SOCKET16 s, INT16 level, INT16 optname,
                          char *optval, INT16 optlen)
{
    if( !optval ) return SOCKET_ERROR;
    return setsockopt( s, (WORD)level, optname, optval, optlen );
}

/***********************************************************************
 *              shutdown		(WINSOCK.22)
 */
INT16 WINAPI shutdown16(SOCKET16 s, INT16 how)
{
    return shutdown( s, how );
}

/***********************************************************************
 *              socket		(WINSOCK.23)
 */
SOCKET16 WINAPI socket16(INT16 af, INT16 type, INT16 protocol)
{
    return socket( af, type, protocol );
}

/***********************************************************************
 *		gethostbyaddr		(WINSOCK.51)
 */
SEGPTR WINAPI gethostbyaddr16(const char *addr, INT16 len, INT16 type)
{
    struct hostent *he;

    if (!(he = gethostbyaddr( addr, len, type ))) return 0;
    return ws_hostent_32_to_16( he, 0, NULL );
}

/***********************************************************************
 *		gethostbyname		(WINSOCK.52)
 */
SEGPTR WINAPI gethostbyname16(const char *name)
{
    struct hostent *he;

    if (!(he = gethostbyname( name ))) return 0;
    return ws_hostent_32_to_16( he, 0, NULL );
}

/***********************************************************************
 *		getprotobyname		(WINSOCK.53)
 */
SEGPTR WINAPI getprotobyname16(const char *name)
{
    struct protoent *pe;

    if (!(pe = getprotobyname( name ))) return 0;
    return ws_protoent_32_to_16( pe, 0, NULL );
}

/***********************************************************************
 *		getprotobynumber	(WINSOCK.54)
 */
SEGPTR WINAPI getprotobynumber16(INT16 number)
{
    struct protoent *pe;

    if (!(pe = getprotobynumber( number ))) return 0;
    return ws_protoent_32_to_16( pe, 0, NULL );
}

/***********************************************************************
 *		getservbyname		(WINSOCK.55)
 */
SEGPTR WINAPI getservbyname16(const char *name, const char *proto)
{
    struct servent *se;

    if (!(se = getservbyname( name, proto ))) return 0;
    return ws_servent_32_to_16( se, 0, NULL );
}

/***********************************************************************
 *		getservbyport		(WINSOCK.56)
 */
SEGPTR WINAPI getservbyport16(INT16 port, const char *proto)
{
    struct servent *se;

    if (!(se = getservbyport( port, proto ))) return 0;
    return ws_servent_32_to_16( se, 0, NULL );
}

/***********************************************************************
 *              gethostname           (WINSOCK.57)
 */
INT16 WINAPI gethostname16(char *name, INT16 namelen)
{
    return gethostname(name, namelen);
}

/***********************************************************************
 *      WSAAsyncSelect		(WINSOCK.101)
 */
INT16 WINAPI WSAAsyncSelect16(SOCKET16 s, HWND16 hWnd, UINT16 wMsg, LONG lEvent)
{
    return WSAAsyncSelect( s, HWND_32(hWnd), wMsg, lEvent );
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
    return run_query( hWnd, uMsg, async_gethostbyaddr, &aq->query, sbuf, buflen );
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
    return run_query( hWnd, uMsg, async_gethostbyname, &aq->query, sbuf, buflen );
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
    return run_query( hWnd, uMsg, async_getprotobynumber, &aq->query, sbuf, buflen );
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
    return run_query( hWnd, uMsg, async_getprotobyname, &aq->query, sbuf, buflen );
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
    return run_query( hWnd, uMsg, async_getservbyport, &aq->query, sbuf, buflen );
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
    return run_query( hWnd, uMsg, async_getservbyname, &aq->query, sbuf, buflen );
}

/***********************************************************************
 *       WSACancelAsyncRequest	(WINSOCK.108)
 */
INT16 WINAPI WSACancelAsyncRequest16(HANDLE16 hAsyncTaskHandle)
{
    FIXME("(%04x),stub\n", hAsyncTaskHandle);
    return 0;
}

/***********************************************************************
 *      WSASetBlockingHook		(WINSOCK.109)
 */
FARPROC16 WINAPI WSASetBlockingHook16(FARPROC16 lpBlockFunc)
{
    /* FIXME: should deal with 16-bit proc */
    return (FARPROC16)WSASetBlockingHook( (FARPROC)lpBlockFunc );
}

/***********************************************************************
 *      WSAUnhookBlockingHook	(WINSOCK.110)
 */
INT16 WINAPI WSAUnhookBlockingHook16(void)
{
    return WSAUnhookBlockingHook();
}

/***********************************************************************
 *      WSAGetLastError		(WINSOCK.111)
 */
INT WINAPI WSAGetLastError16(void)
{
    return WSAGetLastError();
}

/***********************************************************************
 *      WSASetLastError		(WINSOCK.112)
 */
void WINAPI WSASetLastError16(INT16 iError)
{
    WSASetLastError(iError);
}

/***********************************************************************
 *      WSACancelBlockingCall		(WINSOCK.113)
 */
INT WINAPI WSACancelBlockingCall16(void)
{
    return WSACancelBlockingCall();
}

/***********************************************************************
 *      WSAIsBlocking			(WINSOCK.114)
 */
BOOL WINAPI WSAIsBlocking16(void)
{
    return WSAIsBlocking();
}

/***********************************************************************
 *      WSAStartup			(WINSOCK.115)
 *
 * Create socket control struct, attach it to the global list and
 * update a pointer in the task struct.
 */
INT16 WINAPI WSAStartup16(UINT16 wVersionRequested, LPWSADATA16 lpWSAData)
{
    WSADATA data;
    INT ret = WSAStartup( wVersionRequested, &data );

    if (!ret)
    {
        lpWSAData->wVersion     = 0x0101;
        lpWSAData->wHighVersion = 0x0101;
        strcpy( lpWSAData->szDescription, data.szDescription );
        strcpy( lpWSAData->szSystemStatus, data.szSystemStatus );
        lpWSAData->iMaxSockets = data.iMaxSockets;
        lpWSAData->iMaxUdpDg = data.iMaxUdpDg;
        lpWSAData->lpVendorInfo = 0;
        num_startup++;
   }
    return ret;
}

/***********************************************************************
 *      WSACleanup			(WINSOCK.116)
 */
INT WINAPI WSACleanup16(void)
{
    if (num_startup)
    {
        if (!--num_startup)
        {
            /* delete scratch buffers */
            UnMapLS( he_buffer_seg );
            UnMapLS( se_buffer_seg );
            UnMapLS( pe_buffer_seg );
            UnMapLS( dbuffer_seg );
            he_buffer_seg = 0;
            se_buffer_seg = 0;
            pe_buffer_seg = 0;
            dbuffer_seg = 0;
            HeapFree( GetProcessHeap(), 0, he_buffer );
            HeapFree( GetProcessHeap(), 0, se_buffer );
            HeapFree( GetProcessHeap(), 0, pe_buffer );
            he_buffer = NULL;
            se_buffer = NULL;
            pe_buffer = NULL;
        }
    }
    return WSACleanup();
}


/***********************************************************************
 *	__WSAFDIsSet			(WINSOCK.151)
 */
INT16 WINAPI __WSAFDIsSet16(SOCKET16 s, ws_fd_set16 *set)
{
  int i = set->fd_count;

  TRACE("(%d,%p(%i))\n", s, set, i);

  while (i--)
      if (set->fd_array[i] == s) return 1;
  return 0;
}

/***********************************************************************
 *              WSARecvEx			(WINSOCK.1107)
 *
 * See description for WSARecvEx()
 */
INT16 WINAPI WSARecvEx16(SOCKET16 s, char *buf, INT16 len, INT16 *flags)
{
    FIXME("(WSARecvEx16) partial packet return value not set\n");
    return recv16(s, buf, len, *flags);
}
