/*
 * Unit test suite for winsock functions
 *
 * Copyright 2002 Martin Wilck
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>
#include <mswsock.h>
#include "wine/test.h"
#include <winnt.h>
#include <winerror.h>

#define MAX_CLIENTS 4      /* Max number of clients */
#define NUM_TESTS   2      /* Number of tests performed */
#define FIRST_CHAR 'A'     /* First character in transferred pattern */
#define BIND_SLEEP 10      /* seconds to wait between attempts to bind() */
#define BIND_TRIES 6       /* Number of bind() attempts */
#define TEST_TIMEOUT 30    /* seconds to wait before killing child threads
                              after server initialization, if something hangs */

#define wsa_ok(op, cond, msg) \
   do { \
        int tmp, err = 0; \
        tmp = op; \
        if ( !(cond tmp) ) err = WSAGetLastError(); \
        ok ( cond tmp, msg, GetCurrentThreadId(), err); \
   } while (0);


/**************** Structs and typedefs ***************/

typedef struct thread_info
{
    HANDLE thread;
    DWORD id;
} thread_info;

/* Information in the server about open client connections */
typedef struct sock_info
{
    SOCKET                 s;
    struct sockaddr_in     addr;
    struct sockaddr_in     peer;
    char                  *buf;
    int                    nread;
} sock_info;

/* Test parameters for both server & client */
typedef struct test_params
{
    int          sock_type;
    int          sock_prot;
    const char  *inet_addr;
    short        inet_port;
    int          chunk_size;
    int          n_chunks;
    int          n_clients;
} test_params;

/* server-specific test parameters */
typedef struct server_params
{
    test_params   *general;
    DWORD          sock_flags;
    int            buflen;
} server_params;

/* client-specific test parameters */
typedef struct client_params
{
    test_params   *general;
    DWORD          sock_flags;
    int            buflen;
} client_params;

/* This type combines all information for setting up a test scenario */
typedef struct test_setup
{
    test_params              general;
    LPVOID                   srv;
    server_params            srv_params;
    LPVOID                   clt;
    client_params            clt_params;
} test_setup;

/* Thread local storage for server */
typedef struct server_memory
{
    SOCKET                  s;
    struct sockaddr_in      addr;
    sock_info               sock[MAX_CLIENTS];
} server_memory;

/* Thread local storage for client */
typedef struct client_memory
{
    SOCKET s;
    struct sockaddr_in      addr;
    char                   *send_buf;
    char                   *recv_buf;
} client_memory;

/**************** Static variables ***************/

static DWORD      tls;              /* Thread local storage index */
static HANDLE     thread[1+MAX_CLIENTS];
static DWORD      thread_id[1+MAX_CLIENTS];
static HANDLE     server_ready;
static HANDLE     client_ready[MAX_CLIENTS];
static int        client_id;

/**************** General utility functions ***************/

static void set_so_opentype ( BOOL overlapped )
{
    int optval = !overlapped, newval, len = sizeof (int);

    ok ( setsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
                      (LPVOID) &optval, sizeof (optval) ) == 0,
         "setting SO_OPENTYPE failed\n" );
    ok ( getsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE,
                      (LPVOID) &newval, &len ) == 0,
         "getting SO_OPENTYPE failed\n" );
    ok ( optval == newval, "failed to set SO_OPENTYPE\n" );
}

static int set_blocking ( SOCKET s, BOOL blocking )
{
    u_long val = !blocking;
    return ioctlsocket ( s, FIONBIO, &val );
}

static void fill_buffer ( char *buf, int chunk_size, int n_chunks )
{
    char c, *p;
    for ( c = FIRST_CHAR, p = buf; c < FIRST_CHAR + n_chunks; c++, p += chunk_size )
        memset ( p, c, chunk_size );
}

static char* test_buffer ( char *buf, int chunk_size, int n_chunks )
{
    char c, *p;
    int i;
    for ( c = FIRST_CHAR, p = buf; c < FIRST_CHAR + n_chunks; c++, p += chunk_size )
    {
        for ( i = 0; i < chunk_size; i++ )
            if ( p[i] != c ) return p + i;
    }
    return NULL;
}

/*
 * This routine is called when a client / server does not expect any more data,
 * but needs to acknowedge the closing of the connection (by reasing 0 bytes).
 */
static void read_zero_bytes ( SOCKET s )
{
    char buf[256];
    int tmp, n = 0;
    while ( ( tmp = recv ( s, buf, 256, 0 ) ) > 0 )
        n += tmp;
    ok ( n <= 0, "garbage data received: %d bytes\n", n );
}

static int do_synchronous_send ( SOCKET s, char *buf, int buflen, int sendlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; p += n )
        n = send ( s, p, min ( sendlen, last - p ), 0 );
    wsa_ok ( n, 0 <=, "do_synchronous_send (%lx): error %d\n" );
    return p - buf;
}

static int do_synchronous_recv ( SOCKET s, char *buf, int buflen, int recvlen )
{
    char* last = buf + buflen, *p;
    int n = 1;
    for ( p = buf; n > 0 && p < last; p += n )
        n = recv ( s, p, min ( recvlen, last - p ), 0 );
    wsa_ok ( n, 0 <=, "do_synchronous_recv (%lx): error %d:\n" );
    return p - buf;
}

/*
 *  Call this routine right after thread startup.
 *  SO_OPENTYPE must by 0, regardless what the server did.
 */
static void check_so_opentype (void)
{
    int tmp = 1, len;
    len = sizeof (tmp);
    getsockopt ( INVALID_SOCKET, SOL_SOCKET, SO_OPENTYPE, (LPVOID) &tmp, &len );
    ok ( tmp == 0, "check_so_opentype: wrong startup value of SO_OPENTYPE: %d\n", tmp );
}

/**************** Server utility functions ***************/

/*
 *  Even if we have closed our server socket cleanly,
 *  the OS may mark the address "in use" for some time -
 *  this happens with native Linux apps, too.
 */
static void do_bind ( SOCKET s, struct sockaddr* addr, int addrlen )
{
    int err, wsaerr = 0, n_try = BIND_TRIES;

    while ( ( err = bind ( s, addr, addrlen ) ) != 0 &&
            ( wsaerr = WSAGetLastError () ) == WSAEADDRINUSE &&
            n_try-- >= 0)
    {
        trace ( "address in use, waiting ...\n" );
        Sleep ( 1000 * BIND_SLEEP );
    }
    ok ( err == 0, "failed to bind: %d\n", wsaerr );
}

static void server_start ( server_params *par )
{
    int i;
    test_params *gen = par->general;
    server_memory *mem = (LPVOID) LocalAlloc ( LPTR, sizeof (server_memory));

    TlsSetValue ( tls, mem );
    mem->s = WSASocketA ( AF_INET, gen->sock_type, gen->sock_prot,
                          NULL, 0, par->sock_flags );
    ok ( mem->s != INVALID_SOCKET, "Server: WSASocket failed\n" );

    mem->addr.sin_family = AF_INET;
    mem->addr.sin_addr.s_addr = inet_addr ( gen->inet_addr );
    mem->addr.sin_port = htons ( gen->inet_port );

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        mem->sock[i].s = INVALID_SOCKET;
        mem->sock[i].buf = (LPVOID) LocalAlloc ( LPTR, gen->n_chunks * gen->chunk_size );
        mem->sock[i].nread = 0;
    }

    if ( gen->sock_type == SOCK_STREAM )
        do_bind ( mem->s, (struct sockaddr*) &mem->addr, sizeof (mem->addr) );
}

static void server_stop (void)
{
    int i;
    server_memory *mem = TlsGetValue ( tls );

    for (i = 0; i < MAX_CLIENTS; i++ )
    {
        LocalFree ( (HANDLE) mem->sock[i].buf );
        if ( mem->sock[i].s != INVALID_SOCKET )
            closesocket ( mem->sock[i].s );
    }
    ok ( closesocket ( mem->s ) == 0, "closesocket failed\n" );
    LocalFree ( (HANDLE) mem );
    ExitThread ( GetCurrentThreadId () );
}

/**************** Client utilitiy functions ***************/

static void client_start ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem = (LPVOID) LocalAlloc (LPTR, sizeof (client_memory));

    TlsSetValue ( tls, mem );

    WaitForSingleObject ( server_ready, INFINITE );

    mem->s = WSASocketA ( AF_INET, gen->sock_type, gen->sock_prot,
                          NULL, 0, par->sock_flags );

    mem->addr.sin_family = AF_INET;
    mem->addr.sin_addr.s_addr = inet_addr ( gen->inet_addr );
    mem->addr.sin_port = htons ( gen->inet_port );

    ok ( mem->s != INVALID_SOCKET, "Client: WSASocket failed\n" );

    mem->send_buf = (LPVOID) LocalAlloc ( LPTR, 2 * gen->n_chunks * gen->chunk_size );
    mem->recv_buf = mem->send_buf + gen->n_chunks * gen->chunk_size;
    fill_buffer ( mem->send_buf, gen->chunk_size, gen->n_chunks );

    SetEvent ( client_ready[client_id] );
    /* Wait for the other clients to come up */
    WaitForMultipleObjects ( min ( gen->n_clients, MAX_CLIENTS ), client_ready, TRUE, INFINITE );
}

static void client_stop (void)
{
    client_memory *mem = TlsGetValue ( tls );
    wsa_ok ( closesocket ( mem->s ), 0 ==, "closesocket error (%lx): %d\n" );
    LocalFree ( (HANDLE) mem->send_buf );
    LocalFree ( (HANDLE) mem );
    ExitThread(0);
}

/**************** Servers ***************/

/*
 * simple_server: A very basic server doing synchronous IO.
 */
static VOID WINAPI simple_server ( server_params *par )
{
    test_params *gen = par->general;
    server_memory *mem;
    int n_recvd, n_sent, n_expected = gen->n_chunks * gen->chunk_size, tmp, i,
        id = GetCurrentThreadId();
    char *p;

    trace ( "simple_server (%x) starting\n", id );

    set_so_opentype ( FALSE ); /* non-overlapped */
    server_start ( par );
    mem = TlsGetValue ( tls );

    wsa_ok ( set_blocking ( mem->s, TRUE ), 0 ==, "simple_server (%lx): failed to set blocking mode: %d\n");
    wsa_ok ( listen ( mem->s, SOMAXCONN ), 0 ==, "simple_server (%lx): listen failed: %d\n");

    trace ( "simple_server (%x) ready\n", id );
    SetEvent ( server_ready ); /* notify clients */

    for ( i = 0; i < min ( gen->n_clients, MAX_CLIENTS ); i++ )
    {
        trace ( "simple_server (%x): waiting for client\n", id );

        /* accept a single connection */
        tmp = sizeof ( mem->sock[0].peer );
        mem->sock[0].s = accept ( mem->s, (struct sockaddr*) &mem->sock[0].peer, &tmp );
        wsa_ok ( mem->sock[0].s, INVALID_SOCKET !=, "simple_server (%lx): accept failed: %d\n" );

        ok ( mem->sock[0].peer.sin_addr.s_addr == inet_addr ( gen->inet_addr ),
             "simple_server (%x): strange peer address\n", id );

        /* Receive data & check it */
        n_recvd = do_synchronous_recv ( mem->sock[0].s, mem->sock[0].buf, n_expected, par->buflen );
        ok ( n_recvd == n_expected,
             "simple_server (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );
        p = test_buffer ( mem->sock[0].buf, gen->chunk_size, gen->n_chunks );
        ok ( p == NULL, "simple_server (%x): test pattern error: %d\n", id, p - mem->sock[0].buf);

        /* Echo data back */
        n_sent = do_synchronous_send ( mem->sock[0].s, mem->sock[0].buf, n_expected, par->buflen );
        ok ( n_sent == n_expected,
             "simple_server (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

        /* cleanup */
        read_zero_bytes ( mem->sock[0].s );
        wsa_ok ( closesocket ( mem->sock[0].s ),  0 ==, "simple_server (%lx): closesocket error: %d\n" );
        mem->sock[0].s = INVALID_SOCKET;
    }

    trace ( "simple_server (%x) exiting\n", id );
    server_stop ();
}

/**************** Clients ***************/

/*
 * simple_client: A very basic client doing synchronous IO.
 */
static VOID WINAPI simple_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int n_sent, n_recvd, n_expected = gen->n_chunks * gen->chunk_size, id;
    char *p;

    id = GetCurrentThreadId();
    trace ( "simple_client (%x): starting\n", id );
    /* wait here because we want to call set_so_opentype before creating a socket */
    WaitForSingleObject ( server_ready, INFINITE );
    trace ( "simple_client (%x): server ready\n", id );

    check_so_opentype ();
    set_so_opentype ( FALSE ); /* non-overlapped */
    client_start ( par );
    mem = TlsGetValue ( tls );

    /* Connect */
    wsa_ok ( connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) ),
             0 ==, "simple_client (%lx): connect error: %d\n" );
    ok ( set_blocking ( mem->s, TRUE ) == 0,
         "simple_client (%x): failed to set blocking mode\n", id );
    trace ( "simple_client (%x) connected\n", id );

    /* send data to server */
    n_sent = do_synchronous_send ( mem->s, mem->send_buf, n_expected, par->buflen );
    ok ( n_sent == n_expected,
         "simple_client (%x): sent less data than expected: %d of %d\n", id, n_sent, n_expected );

    /* shutdown send direction */
    wsa_ok ( shutdown ( mem->s, SD_SEND ), 0 ==, "simple_client (%lx): shutdown failed: %d\n" );

    /* Receive data echoed back & check it */
    n_recvd = do_synchronous_recv ( mem->s, mem->recv_buf, n_expected, par->buflen );
    ok ( n_recvd == n_expected,
         "simple_client (%x): received less data than expected: %d of %d\n", id, n_recvd, n_expected );

    /* check data */
    p = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( p == NULL, "simple_client (%x): test pattern error: %d\n", id, p - mem->recv_buf);

    /* cleanup */
    read_zero_bytes ( mem->s );
    trace ( "simple_client (%x) exiting\n", id );
    client_stop ();
}

/*
 * event_client: An event-driven client
 */
static void WINAPI event_client ( client_params *par )
{
    test_params *gen = par->general;
    client_memory *mem;
    int id = GetCurrentThreadId(), n_expected = gen->n_chunks * gen->chunk_size,
        tmp, err, n;
    HANDLE event;
    WSANETWORKEVENTS wsa_events;
    char *send_last, *recv_last, *send_p, *recv_p;
    long mask = FD_READ | FD_WRITE | FD_CLOSE;

    trace ( "event_client (%x): starting\n", id );
    client_start ( par );
    trace ( "event_client (%x): server ready\n", id );

    mem = TlsGetValue ( tls );

    /* Prepare event notification for connect, makes socket nonblocking */
    event = WSACreateEvent ();
    WSAEventSelect ( mem->s, event, FD_CONNECT );
    tmp = connect ( mem->s, (struct sockaddr*) &mem->addr, sizeof ( mem->addr ) );
    if ( tmp != 0 && ( err = WSAGetLastError () ) != WSAEWOULDBLOCK )
        ok ( 0, "event_client (%x): connect error: %d\n", id, err );

    tmp = WaitForSingleObject ( event, INFINITE );
    ok ( tmp == WAIT_OBJECT_0, "event_client (%x): wait for connect event failed: %d\n", id, tmp );
    err = WSAEnumNetworkEvents ( mem->s, event, &wsa_events );
    wsa_ok ( err, 0 ==, "event_client (%lx): WSAEnumNetworkEvents error: %d\n" );

    err = wsa_events.iErrorCode[ FD_CONNECT_BIT ];
    ok ( err == 0, "event_client (%x): connect error: %d\n", id, err );
    if ( err ) goto out;

    trace ( "event_client (%x) connected\n", id );

    WSAEventSelect ( mem->s, event, mask );

    recv_p = mem->recv_buf;
    recv_last = mem->recv_buf + n_expected;
    send_p = mem->send_buf;
    send_last = mem->send_buf + n_expected;

    while ( TRUE )
    {
        err = WaitForSingleObject ( event, INFINITE );
        ok ( err == WAIT_OBJECT_0, "event_client (%x): wait failed\n", id );

        err = WSAEnumNetworkEvents ( mem->s, event, &wsa_events );
        wsa_ok ( err, 0 ==, "event_client (%lx): WSAEnumNetworkEvents error: %d\n" );

        if ( wsa_events.lNetworkEvents & FD_WRITE )
        {
            err = wsa_events.iErrorCode[ FD_WRITE_BIT ];
            ok ( err == 0, "event_client (%x): FD_WRITE error code: %d\n", id, err );

            if ( err== 0 )
                do
                {
                    n = send ( mem->s, send_p, min ( send_last - send_p, par->buflen ), 0 );
                    if ( n < 0 )
                    {
                        err = WSAGetLastError ();
                        ok ( err == WSAEWOULDBLOCK, "event_client (%x): send error: %d\n", id, err );
                    }
                    else
                        send_p += n;
                }
                while ( n >= 0 && send_p < send_last );

            if ( send_p == send_last )
            {
                trace ( "event_client (%x): all data sent - shutdown\n", id );
                shutdown ( mem->s, SD_SEND );
                mask &= ~FD_WRITE;
                WSAEventSelect ( mem->s, event, mask );
            }
        }
        if ( wsa_events.lNetworkEvents & FD_READ )
        {
            err = wsa_events.iErrorCode[ FD_READ_BIT ];
            ok ( err == 0, "event_client (%x): FD_READ error code: %d\n", id, err );
            if ( err != 0 ) break;
            
            /* First read must succeed */
            n = recv ( mem->s, recv_p, min ( recv_last - recv_p, par->buflen ), 0 );
            wsa_ok ( n, 0 <=, "event_client (%lx): recv error: %d\n" );

            while ( n >= 0 ) {
                recv_p += n;
                if ( recv_p == recv_last )
                {
                    mask &= ~FD_READ;
                    trace ( "event_client (%x): all data received\n", id );
                    WSAEventSelect ( mem->s, event, mask );
                    break;
                }
                n = recv ( mem->s, recv_p, min ( recv_last - recv_p, par->buflen ), 0 );
                if ( n < 0 && ( err = WSAGetLastError()) != WSAEWOULDBLOCK )
                    ok ( 0, "event_client (%x): read error: %d\n", id, err );
                
            }
        }   
        if ( wsa_events.lNetworkEvents & FD_CLOSE )
        {
            trace ( "event_client (%x): close event\n", id );
            err = wsa_events.iErrorCode[ FD_CLOSE_BIT ];
            ok ( err == 0, "event_client (%x): FD_CLOSE error code: %d\n", id, err );
            break;
        }
    }

    ok ( send_p == send_last,
         "simple_client (%x): sent less data than expected: %d of %d\n",
         id, send_p - mem->send_buf, n_expected );
    ok ( recv_p == recv_last,
         "simple_client (%x): received less data than expected: %d of %d\n",
         id, recv_p - mem->recv_buf, n_expected );
    recv_p = test_buffer ( mem->recv_buf, gen->chunk_size, gen->n_chunks );
    ok ( recv_p == NULL, "event_client (%x): test pattern error: %d\n", id, recv_p - mem->recv_buf);

out:
    WSACloseEvent ( event );
    trace ( "event_client (%x) exiting\n", id );
    client_stop ();
}

/**************** Main program utility functions ***************/

static void Init (void)
{
    WORD ver = MAKEWORD (2, 2);
    WSADATA data;

    ok ( WSAStartup ( ver, &data ) == 0, "WSAStartup failed\n" );
    tls = TlsAlloc();
}

static void Exit (void)
{
    TlsFree ( tls );
    ok ( WSACleanup() == 0, "WSACleanup failed\n" );
}

static void StartServer (LPTHREAD_START_ROUTINE routine,
                         test_params *general, server_params *par)
{
    par->general = general;
    thread[0] = CreateThread ( NULL, 0, routine, par, 0, &thread_id[0] );
    ok ( thread[0] != NULL, "Failed to create server thread\n" );
}

static void StartClients (LPTHREAD_START_ROUTINE routine,
                          test_params *general, client_params *par)
{
    int i;
    par->general = general;
    for ( i = 1; i <= min ( general->n_clients, MAX_CLIENTS ); i++ )
    {
        client_id = i - 1;
        thread[i] = CreateThread ( NULL, 0, routine, par, 0, &thread_id[i] );
        ok ( thread[i] != NULL, "Failed to create client thread\n" );
        /* Make sure the client is up and running */
        WaitForSingleObject ( client_ready[client_id], INFINITE );
    };
}

static void do_test( test_setup *test )
{
    DWORD i, n = min (test->general.n_clients, MAX_CLIENTS);
    DWORD wait;

    server_ready = CreateEventA ( NULL, TRUE, FALSE, NULL );
    for (i = 0; i <= n; i++)
        client_ready[i] = CreateEventA ( NULL, TRUE, FALSE, NULL );

    StartServer ( test->srv, &test->general, &test->srv_params );
    StartClients ( test->clt, &test->general, &test->clt_params );
    WaitForSingleObject ( server_ready, INFINITE );

    wait = WaitForMultipleObjects ( 1 + n, thread, TRUE, 1000 * TEST_TIMEOUT );
    ok ( wait >= WAIT_OBJECT_0 && wait <= WAIT_OBJECT_0 + n , 
         "some threads have not completed: %lx\n", wait );

    if ( ! ( wait >= WAIT_OBJECT_0 && wait <= WAIT_OBJECT_0 + n ) )
    {
        for (i = 0; i <= n; i++)
        {
            trace ("terminating thread %08lx\n", thread_id[i]);
            if ( WaitForSingleObject ( thread[i], 0 ) != WAIT_OBJECT_0 )
                TerminateThread ( thread [i], 0 );
        }
    }
    CloseHandle ( server_ready );
    for (i = 0; i <= n; i++)
        CloseHandle ( client_ready[i] );
}

static void test_so_reuseaddr()
{
    struct sockaddr_in saddr;
    SOCKET s1,s2;
    int rc,reuse,size;

    saddr.sin_family      = AF_INET;
    saddr.sin_port        = htons(9375);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    s1=socket(AF_INET, SOCK_STREAM, 0);
    ok(s1!=INVALID_SOCKET, "socket() failed error: %d\n", WSAGetLastError());
    rc = bind(s1, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc!=SOCKET_ERROR, "bind(s1) failed error: %d\n", WSAGetLastError());

    s2=socket(AF_INET, SOCK_STREAM, 0);
    ok(s2!=INVALID_SOCKET, "socket() failed error: %d\n", WSAGetLastError());

    reuse=0x1234;
    size=sizeof(reuse);
    rc=getsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, &size );
    ok(rc==0 && reuse==0,"wrong result in getsockopt(SO_REUSEADDR): rc=%d reuse=%d\n",rc,reuse);

    rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc==SOCKET_ERROR, "bind() succeeded\n");

    reuse = 1;
    rc = setsockopt(s2, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));
    ok(rc==0, "setsockopt() failed error: %d\n", WSAGetLastError());

    todo_wine {
    rc = bind(s2, (struct sockaddr*)&saddr, sizeof(saddr));
    ok(rc==0, "bind() failed error: %d\n", WSAGetLastError());
    }

    closesocket(s2);
    closesocket(s1);
}

/************* Array containing the tests to run **********/

#define STD_STREAM_SOCKET \
            SOCK_STREAM, \
            0, \
            "127.0.0.1", \
            9374

static test_setup tests [NUM_TESTS] =
{
    /* Test 0: synchronous client and server */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        simple_server,
        {
            NULL,
            0,
            64
        },
        simple_client,
        {
            NULL,
            0,
            128
        }
    },
    /* Test 1: event-driven client, synchronous server */
    {
        {
            STD_STREAM_SOCKET,
            2048,
            16,
            2
        },
        simple_server,
        {
            NULL,
            0,
            64
        },
        event_client,
        {
            NULL,
            WSA_FLAG_OVERLAPPED,
            128
        }
    }
};

/**************** Main program  ***************/

START_TEST( sock )
{
    int i;
    Init();

    test_so_reuseaddr();

    for (i = 0; i < NUM_TESTS; i++)
    {
        trace ( " **** STARTING TEST %d **** \n", i );
        do_test (  &tests[i] );
        trace ( " **** TEST %d COMPLETE **** \n", i );
    }

    Exit();
}
