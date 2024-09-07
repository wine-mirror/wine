/* Internet TCP/IP and IPX Connection For DirectPlay
 *
 * Copyright 2008 Ismael Barros Barros
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


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "dpwsockx_dll.h"
#include "wine/debug.h"
#include "dplay.h"
#include "wine/dplaysp.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

#define DPWS_PORT           47624
#define DPWS_START_TCP_PORT 2300
#define DPWS_END_TCP_PORT   2350

static void DPWS_MessageBodyReceiveCompleted( DPWS_IN_CONNECTION *connection );

static HRESULT DPWS_BindToFreePort( SOCKET sock, SOCKADDR_IN *addr, int startPort, int endPort )
{
    int port;

    memset( addr, 0, sizeof( *addr ) );
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl( INADDR_ANY );

    for ( port = startPort; port < endPort; ++port )
    {
        addr->sin_port = htons( port );

        if ( SOCKET_ERROR != bind( sock, (SOCKADDR *) addr, sizeof( *addr ) ) )
            return DP_OK;

        if ( WSAGetLastError() == WSAEADDRINUSE )
            continue;

        ERR( "bind() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    ERR( "no free ports\n" );
    return DPERR_UNAVAILABLE;
}

static void DPWS_RemoveInConnection( DPWS_IN_CONNECTION *connection )
{
    list_remove( &connection->entry );
    closesocket( connection->tcpSock );
    free( connection->buffer );
    free( connection );
}

static void WINAPI DPWS_TcpReceiveCompleted( DWORD error, DWORD transferred,
                                             WSAOVERLAPPED *overlapped, DWORD flags )
{
    DPWS_IN_CONNECTION *connection = (DPWS_IN_CONNECTION *)overlapped->hEvent;

    if ( error != ERROR_SUCCESS )
    {
        ERR( "WSARecv() failed\n" );
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( !transferred )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( transferred < connection->wsaBuffer.len )
    {
        connection->wsaBuffer.len -= transferred;
        connection->wsaBuffer.buf += transferred;

        if ( SOCKET_ERROR == WSARecv( connection->tcpSock, &connection->wsaBuffer, 1, &transferred,
                                      &flags, &connection->overlapped, DPWS_TcpReceiveCompleted ) )
        {
            if ( WSAGetLastError() != WSA_IO_PENDING )
            {
                ERR( "WSARecv() failed\n" );
                DPWS_RemoveInConnection( connection );
                return;
            }
        }
        return;
    }

    connection->completionRoutine( connection );
}

static HRESULT DPWS_TcpReceive( DPWS_IN_CONNECTION *connection, void *data, DWORD size,
                                DPWS_COMPLETION_ROUTINE *completionRoutine )
{
    DWORD transferred;
    DWORD flags = 0;

    connection->wsaBuffer.len = size;
    connection->wsaBuffer.buf = data;

    connection->completionRoutine = completionRoutine;

    if ( SOCKET_ERROR == WSARecv( connection->tcpSock, &connection->wsaBuffer, 1, &transferred,
                                  &flags, &connection->overlapped, DPWS_TcpReceiveCompleted ) )
    {
        if ( WSAGetLastError() != WSA_IO_PENDING )
        {
            ERR( "WSARecv() failed\n" );
            return DPERR_UNAVAILABLE;
        }
    }

    return DP_OK;
}

static void DPWS_HeaderReceiveCompleted( DPWS_IN_CONNECTION *connection )
{
    int messageBodySize;
    int messageSize;

    messageSize = DPSP_MSG_SIZE( connection->header.mixed );
    if ( messageSize < sizeof( DPSP_MSG_HEADER ))
    {
        ERR( "message is too short: %d\n", messageSize );
        DPWS_RemoveInConnection( connection );
        return;
    }
    messageBodySize = messageSize - sizeof( DPSP_MSG_HEADER );

    if ( messageBodySize > DPWS_GUARANTEED_MAXBUFFERSIZE )
    {
        ERR( "message is too long: %d\n", messageSize );
        DPWS_RemoveInConnection( connection );
        return;
    }

    if ( connection->bufferSize < messageBodySize )
    {
        int newSize = max( connection->bufferSize * 2, messageBodySize );
        char *newBuffer = malloc( newSize );
        if ( !newBuffer )
        {
            ERR( "failed to allocate required memory.\n" );
            DPWS_RemoveInConnection( connection );
            return;
        }
        free( connection->buffer );
        connection->buffer = newBuffer;
        connection->bufferSize = newSize;
    }

    if ( FAILED( DPWS_TcpReceive( connection, connection->buffer, messageBodySize,
                                  DPWS_MessageBodyReceiveCompleted ) ) )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }
}

static void DPWS_MessageBodyReceiveCompleted( DPWS_IN_CONNECTION *connection )
{
    int messageBodySize;
    int messageSize;

    if ( connection->header.SockAddr.sin_addr.s_addr == INADDR_ANY )
        connection->header.SockAddr.sin_addr = connection->addr.sin_addr;

    messageSize = DPSP_MSG_SIZE( connection->header.mixed );
    messageBodySize = messageSize - sizeof( DPSP_MSG_HEADER );

    IDirectPlaySP_HandleMessage( connection->sp, connection->buffer, messageBodySize,
                                 &connection->header );

    if ( FAILED( DPWS_TcpReceive( connection, &connection->header, sizeof( DPSP_MSG_HEADER ),
                                  DPWS_HeaderReceiveCompleted ) ) )
    {
        DPWS_RemoveInConnection( connection );
        return;
    }
}

static DWORD WINAPI DPWS_ThreadProc( void *param )
{
    DPWS_DATA *dpwsData = (DPWS_DATA *)param;

    SetThreadDescription( GetCurrentThread(), L"dpwsockx" );

    for ( ;; )
    {
        DPWS_IN_CONNECTION *connection;
        WSANETWORKEVENTS networkEvents;
        WSAEVENT events[2];
        SOCKADDR_IN addr;
        DWORD waitResult;
        int addrSize;
        SOCKET sock;

        events[ 0 ] = dpwsData->stopEvent;
        events[ 1 ] = dpwsData->acceptEvent;
        waitResult = WSAWaitForMultipleEvents( ARRAYSIZE( events ), events, FALSE, WSA_INFINITE,
                                               TRUE );
        if ( waitResult == WSA_WAIT_FAILED )
        {
            ERR( "WSAWaitForMultipleEvents() failed\n" );
            break;
        }
        if ( waitResult == WSA_WAIT_IO_COMPLETION )
            continue;
        if ( waitResult == WSA_WAIT_EVENT_0 )
            break;

        if ( SOCKET_ERROR == WSAEnumNetworkEvents( dpwsData->tcpSock, dpwsData->acceptEvent,
                                                   &networkEvents ) )
        {
            ERR( "WSAEnumNetworkEvents() failed\n" );
            break;
        }

        addrSize = sizeof( addr );
        sock = accept( dpwsData->tcpSock, (SOCKADDR *)&addr, &addrSize );
        if ( sock == INVALID_SOCKET )
        {
            if ( WSAGetLastError() == WSAEWOULDBLOCK )
                continue;
            ERR( "accept() failed\n" );
            break;
        }

        connection = calloc( 1, sizeof( DPWS_IN_CONNECTION ) );
        if ( !connection )
        {
            ERR( "failed to allocate required memory.\n" );
            closesocket( sock );
            continue;
        }

        connection->addr = addr;
        connection->tcpSock = sock;
        connection->overlapped.hEvent = connection;
        connection->sp = dpwsData->lpISP;

        list_add_tail( &dpwsData->inConnections, &connection->entry );

        if ( FAILED( DPWS_TcpReceive( connection, &connection->header, sizeof( DPSP_MSG_HEADER ),
                                      DPWS_HeaderReceiveCompleted ) ) )
        {
            DPWS_RemoveInConnection( connection );
            continue;
        }
    }

    return 0;
}

static HRESULT DPWS_Start( DPWS_DATA *dpwsData )
{
    HRESULT hr;

    if ( dpwsData->started )
        return S_OK;

    dpwsData->tcpSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( dpwsData->tcpSock == INVALID_SOCKET )
    {
        ERR( "socket() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    hr = DPWS_BindToFreePort( dpwsData->tcpSock, &dpwsData->tcpAddr, DPWS_START_TCP_PORT,
                              DPWS_END_TCP_PORT );
    if ( FAILED( hr ) )
    {
        closesocket( dpwsData->tcpSock );
        return hr;
    }

    if ( SOCKET_ERROR == listen( dpwsData->tcpSock, SOMAXCONN ) )
    {
        ERR( "listen() failed\n" );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    dpwsData->acceptEvent = WSACreateEvent();
    if ( !dpwsData->acceptEvent )
    {
        ERR( "WSACreateEvent() failed\n" );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    if ( SOCKET_ERROR == WSAEventSelect( dpwsData->tcpSock, dpwsData->acceptEvent, FD_ACCEPT ) )
    {
        ERR( "WSAEventSelect() failed\n" );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    list_init( &dpwsData->inConnections );

    dpwsData->stopEvent = WSACreateEvent();
    if ( !dpwsData->stopEvent )
    {
        ERR( "WSACreateEvent() failed\n" );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    InitializeCriticalSection( &dpwsData->sendCs );

    dpwsData->thread = CreateThread( NULL, 0, DPWS_ThreadProc, dpwsData, 0, NULL );
    if ( !dpwsData->thread )
    {
        ERR( "CreateThread() failed\n" );
        DeleteCriticalSection( &dpwsData->sendCs );
        WSACloseEvent( dpwsData->stopEvent );
        WSACloseEvent( dpwsData->acceptEvent );
        closesocket( dpwsData->tcpSock );
        return DPERR_UNAVAILABLE;
    }

    dpwsData->started = TRUE;

    return S_OK;
}

static void DPWS_Stop( DPWS_DATA *dpwsData )
{
    DPWS_IN_CONNECTION *inConnection2;
    DPWS_IN_CONNECTION *inConnection;

    if ( !dpwsData->started )
        return;

    dpwsData->started = FALSE;

    WSASetEvent( dpwsData->stopEvent );
    WaitForSingleObject( dpwsData->thread, INFINITE );
    CloseHandle( dpwsData->thread );

    if ( dpwsData->nameserverConnection.tcpSock != INVALID_SOCKET )
        closesocket( dpwsData->nameserverConnection.tcpSock );

    LIST_FOR_EACH_ENTRY_SAFE( inConnection, inConnection2, &dpwsData->inConnections,
                              DPWS_IN_CONNECTION, entry )
        DPWS_RemoveInConnection( inConnection );

    DeleteCriticalSection( &dpwsData->sendCs );
    WSACloseEvent( dpwsData->stopEvent );
    WSACloseEvent( dpwsData->acceptEvent );
    closesocket( dpwsData->tcpSock );
}

static HRESULT WINAPI DPWSCB_EnumSessions( LPDPSP_ENUMSESSIONSDATA data )
{
    DPSP_MSG_HEADER *header = (DPSP_MSG_HEADER *) data->lpMessage;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    SOCKADDR_IN addr;
    BOOL true = TRUE;
    SOCKET sock;
    HRESULT hr;

    TRACE( "(%p,%ld,%p,%u)\n",
           data->lpMessage, data->dwMessageSize,
           data->lpISP, data->bReturnStatus );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = DPWS_Start( dpwsData );
    if ( FAILED (hr) )
        return hr;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if ( sock == INVALID_SOCKET )
    {
        ERR( "socket() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    if ( SOCKET_ERROR == setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (const char *) &true,
                                     sizeof( true ) ) )
    {
        ERR( "setsockopt() failed\n" );
        closesocket( sock );
        return DPERR_UNAVAILABLE;
    }

    memset( header, 0, sizeof( DPSP_MSG_HEADER ) );
    header->mixed = DPSP_MSG_MAKE_MIXED( data->dwMessageSize, DPSP_MSG_TOKEN_REMOTE );
    header->SockAddr.sin_family = AF_INET;
    header->SockAddr.sin_port = dpwsData->tcpAddr.sin_port;

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl( INADDR_BROADCAST );
    addr.sin_port = htons( DPWS_PORT );

    if ( SOCKET_ERROR == sendto( sock, data->lpMessage, data->dwMessageSize, 0, (SOCKADDR *) &addr,
                                 sizeof( addr ) ) )
    {
        ERR( "sendto() failed\n" );
        closesocket( sock );
        return DPERR_UNAVAILABLE;
    }

    closesocket( sock );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_Reply( LPDPSP_REPLYDATA data )
{
    FIXME( "(%p,%p,%ld,%ld,%p) stub\n",
           data->lpSPMessageHeader, data->lpMessage, data->dwMessageSize,
           data->idNameServer, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_Send( LPDPSP_SENDDATA data )
{
    FIXME( "(0x%08lx,%ld,%ld,%p,%ld,%u,%p) stub\n",
           data->dwFlags, data->idPlayerTo, data->idPlayerFrom,
           data->lpMessage, data->dwMessageSize,
           data->bSystemMessage, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_CreatePlayer( LPDPSP_CREATEPLAYERDATA data )
{
    DPWS_PLAYERDATA *playerData;
    DWORD playerDataSize;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%ld,0x%08lx,%p,%p)\n",
           data->idPlayer, data->dwFlags,
           data->lpSPMessageHeader, data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    if ( !data->lpSPMessageHeader )
    {
        DPWS_PLAYERDATA playerDataPlaceholder = { 0 };
        hr = IDirectPlaySP_SetSPPlayerData( data->lpISP, data->idPlayer,
                                            (void *) &playerDataPlaceholder,
                                            sizeof( playerDataPlaceholder ), DPSET_REMOTE );
        if ( FAILED( hr ) )
            return hr;
    }

    hr = IDirectPlaySP_GetSPPlayerData( data->lpISP, data->idPlayer, (void **) &playerData,
                                        &playerDataSize, DPSET_REMOTE );
    if ( FAILED( hr ) )
        return hr;
    if ( playerDataSize != sizeof( DPWS_PLAYERDATA ) )
        return DPERR_GENERIC;

    if ( data->lpSPMessageHeader )
    {
        DPSP_MSG_HEADER *header = data->lpSPMessageHeader;
        if ( !playerData->tcpAddr.sin_addr.s_addr )
            playerData->tcpAddr.sin_addr = header->SockAddr.sin_addr;
        if ( !playerData->udpAddr.sin_addr.s_addr )
            playerData->udpAddr.sin_addr = header->SockAddr.sin_addr;
    }
    else
    {
        playerData->tcpAddr.sin_family = AF_INET;
        playerData->tcpAddr.sin_port = dpwsData->tcpAddr.sin_port;
        playerData->udpAddr.sin_family = AF_INET;
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_DeletePlayer( LPDPSP_DELETEPLAYERDATA data )
{
    FIXME( "(%ld,0x%08lx,%p) stub\n",
           data->idPlayer, data->dwFlags, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_GetAddress( LPDPSP_GETADDRESSDATA data )
{
    FIXME( "(%ld,0x%08lx,%p,%p,%p) stub\n",
           data->idPlayer, data->dwFlags, data->lpAddress,
           data->lpdwAddressSize, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_GetCaps( LPDPSP_GETCAPSDATA data )
{
    TRACE( "(%ld,%p,0x%08lx,%p)\n",
           data->idPlayer, data->lpCaps, data->dwFlags, data->lpISP );

    data->lpCaps->dwFlags = ( DPCAPS_ASYNCSUPPORTED |
                              DPCAPS_GUARANTEEDOPTIMIZED |
                              DPCAPS_GUARANTEEDSUPPORTED );

    data->lpCaps->dwMaxQueueSize    = DPWS_MAXQUEUESIZE;
    data->lpCaps->dwHundredBaud     = DPWS_HUNDREDBAUD;
    data->lpCaps->dwLatency         = DPWS_LATENCY;
    data->lpCaps->dwMaxLocalPlayers = DPWS_MAXLOCALPLAYERS;
    data->lpCaps->dwHeaderLength    = sizeof(DPSP_MSG_HEADER);
    data->lpCaps->dwTimeout         = DPWS_TIMEOUT;

    if ( data->dwFlags & DPGETCAPS_GUARANTEED )
    {
        data->lpCaps->dwMaxBufferSize = DPWS_GUARANTEED_MAXBUFFERSIZE;
        data->lpCaps->dwMaxPlayers    = DPWS_GUARANTEED_MAXPLAYERS;
    }
    else
    {
        data->lpCaps->dwMaxBufferSize = DPWS_MAXBUFFERSIZE;
        data->lpCaps->dwMaxPlayers    = DPWS_MAXPLAYERS;
    }

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_Open( LPDPSP_OPENDATA data )
{
    DPSP_MSG_HEADER *header = (DPSP_MSG_HEADER *) data->lpSPMessageHeader;
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%u,%p,%p,%u,0x%08lx,0x%08lx)\n",
           data->bCreate, data->lpSPMessageHeader, data->lpISP,
           data->bReturnStatus, data->dwOpenFlags, data->dwSessionFlags );

    if ( data->bCreate )
    {
        FIXME( "session creation is not yet supported\n" );
        return DPERR_UNSUPPORTED;
    }

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **)&dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    hr = DPWS_Start( dpwsData );
    if ( FAILED (hr) )
        return hr;

    dpwsData->nameserverConnection.addr.sin_family = AF_INET;
    dpwsData->nameserverConnection.addr.sin_addr = header->SockAddr.sin_addr;
    dpwsData->nameserverConnection.addr.sin_port = header->SockAddr.sin_port;

    dpwsData->nameserverConnection.tcpSock = INVALID_SOCKET;

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_CloseEx( LPDPSP_CLOSEDATA data )
{
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%p)\n", data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    DPWS_Stop( dpwsData );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_ShutdownEx( LPDPSP_SHUTDOWNDATA data )
{
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    HRESULT hr;

    TRACE( "(%p)\n", data->lpISP );

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    DPWS_Stop( dpwsData );

    WSACleanup();

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_GetAddressChoices( LPDPSP_GETADDRESSCHOICESDATA data )
{
    FIXME( "(%p,%p,%p) stub\n",
           data->lpAddress, data->lpdwAddressSize, data->lpISP );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_SendEx( LPDPSP_SENDEXDATA data )
{
    DPWS_OUT_CONNECTION *connection;
    DPSP_MSG_HEADER header = { 0 };
    DPWS_DATA *dpwsData;
    DWORD dpwsDataSize;
    DWORD transferred;
    HRESULT hr;

    TRACE( "(%p,0x%08lx,%ld,%ld,%p,%ld,%ld,%ld,%ld,%p,%p,%u)\n",
           data->lpISP, data->dwFlags, data->idPlayerTo, data->idPlayerFrom,
           data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
           data->dwPriority, data->dwTimeout, data->lpDPContext,
           data->lpdwSPMsgID, data->bSystemMessage );

    if ( data->idPlayerTo )
    {
        FIXME( "only sending to nameserver is currently implemented\n" );
        return DPERR_UNSUPPORTED;
    }

    if ( !( data->dwFlags & DPSEND_GUARANTEED ) )
    {
        FIXME( "non-guaranteed delivery is not yet supported\n" );
        return DPERR_UNSUPPORTED;
    }

    if ( data->dwFlags & DPSEND_ASYNC )
    {
        FIXME("asynchronous send is not yet supported\n");
        return DPERR_UNSUPPORTED;
    }

    hr = IDirectPlaySP_GetSPData( data->lpISP, (void **) &dpwsData, &dpwsDataSize, DPSET_LOCAL );
    if ( FAILED( hr ) )
        return hr;

    header.mixed = DPSP_MSG_MAKE_MIXED( data->dwMessageSize, DPSP_MSG_TOKEN_REMOTE );
    header.SockAddr.sin_family = AF_INET;
    header.SockAddr.sin_port = dpwsData->tcpAddr.sin_port;

    data->lpSendBuffers[ 0 ].pData = (unsigned char *) &header;

    EnterCriticalSection( &dpwsData->sendCs );

    connection = &dpwsData->nameserverConnection;

    if ( connection->tcpSock == INVALID_SOCKET )
    {
        connection->tcpSock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
        if ( connection->tcpSock == INVALID_SOCKET )
        {
            ERR( "socket() failed\n" );
            LeaveCriticalSection( &dpwsData->sendCs );
            return DPERR_UNAVAILABLE;
        }

        if ( SOCKET_ERROR == connect( connection->tcpSock, (SOCKADDR *) &connection->addr,
                                      sizeof( connection->addr ) ) )
        {
            ERR( "connect() failed\n" );
            closesocket( connection->tcpSock );
            connection->tcpSock = INVALID_SOCKET;
            LeaveCriticalSection( &dpwsData->sendCs );
            return DPERR_UNAVAILABLE;
        }
    }

    if ( SOCKET_ERROR == WSASend( connection->tcpSock, (WSABUF *) data->lpSendBuffers,
                                  data->cBuffers, &transferred, 0, NULL, NULL ) )
    {
        if ( WSAGetLastError() != WSA_IO_PENDING )
        {
            ERR( "WSASend() failed\n" );
            LeaveCriticalSection( &dpwsData->sendCs );
            return DPERR_UNAVAILABLE;
        }
    }

    if ( transferred < data->dwMessageSize )
    {
        ERR( "lost connection\n" );
        closesocket( connection->tcpSock );
        connection->tcpSock = INVALID_SOCKET;
        LeaveCriticalSection( &dpwsData->sendCs );
        return DPERR_CONNECTIONLOST;
    }

    LeaveCriticalSection( &dpwsData->sendCs );

    return DP_OK;
}

static HRESULT WINAPI DPWSCB_SendToGroupEx( LPDPSP_SENDTOGROUPEXDATA data )
{
    FIXME( "(%p,0x%08lx,%ld,%ld,%p,%ld,%ld,%ld,%ld,%p,%p) stub\n",
           data->lpISP, data->dwFlags, data->idGroupTo, data->idPlayerFrom,
           data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
           data->dwPriority, data->dwTimeout, data->lpDPContext,
           data->lpdwSPMsgID );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_Cancel( LPDPSP_CANCELDATA data )
{
    FIXME( "(%p,0x%08lx,%p,%ld,0x%08lx,0x%08lx) stub\n",
           data->lpISP, data->dwFlags, data->lprglpvSPMsgID, data->cSPMsgID,
           data->dwMinPriority, data->dwMaxPriority );
    return DPERR_UNSUPPORTED;
}

static HRESULT WINAPI DPWSCB_GetMessageQueue( LPDPSP_GETMESSAGEQUEUEDATA data )
{
    FIXME( "(%p,0x%08lx,%ld,%ld,%p,%p) stub\n",
           data->lpISP, data->dwFlags, data->idFrom, data->idTo,
           data->lpdwNumMsgs, data->lpdwNumBytes );
    return DPERR_UNSUPPORTED;
}

static void setup_callbacks( LPDPSP_SPCALLBACKS lpCB )
{
    lpCB->EnumSessions           = DPWSCB_EnumSessions;
    lpCB->Reply                  = DPWSCB_Reply;
    lpCB->Send                   = DPWSCB_Send;
    lpCB->CreatePlayer           = DPWSCB_CreatePlayer;
    lpCB->DeletePlayer           = DPWSCB_DeletePlayer;
    lpCB->GetAddress             = DPWSCB_GetAddress;
    lpCB->GetCaps                = DPWSCB_GetCaps;
    lpCB->Open                   = DPWSCB_Open;
    lpCB->CloseEx                = DPWSCB_CloseEx;
    lpCB->ShutdownEx             = DPWSCB_ShutdownEx;
    lpCB->GetAddressChoices      = DPWSCB_GetAddressChoices;
    lpCB->SendEx                 = DPWSCB_SendEx;
    lpCB->SendToGroupEx          = DPWSCB_SendToGroupEx;
    lpCB->Cancel                 = DPWSCB_Cancel;
    lpCB->GetMessageQueue        = DPWSCB_GetMessageQueue;

    lpCB->AddPlayerToGroup       = NULL;
    lpCB->Close                  = NULL;
    lpCB->CreateGroup            = NULL;
    lpCB->DeleteGroup            = NULL;
    lpCB->RemovePlayerFromGroup  = NULL;
    lpCB->SendToGroup            = NULL;
    lpCB->Shutdown               = NULL;
}



/******************************************************************
 *		SPInit (DPWSOCKX.1)
 */
HRESULT WINAPI SPInit( LPSPINITDATA lpspData )
{
    WSADATA wsaData;
    DPWS_DATA dpwsData;

    TRACE( "Initializing library for %s (%s)\n",
           wine_dbgstr_guid(lpspData->lpGuid), debugstr_w(lpspData->lpszName) );

    /* We only support TCP/IP service */
    if ( !IsEqualGUID(lpspData->lpGuid, &DPSPGUID_TCPIP) )
    {
        return DPERR_UNAVAILABLE;
    }

    /* Assign callback functions */
    setup_callbacks( lpspData->lpCB );

    /* Load Winsock 2.0 DLL */
    if ( WSAStartup( MAKEWORD(2, 0), &wsaData ) != 0 )
    {
        ERR( "WSAStartup() failed\n" );
        return DPERR_UNAVAILABLE;
    }

    /* Initialize internal data */
    memset( &dpwsData, 0, sizeof(DPWS_DATA) );
    dpwsData.lpISP = lpspData->lpISP;
    IDirectPlaySP_SetSPData( lpspData->lpISP, &dpwsData, sizeof(DPWS_DATA),
                             DPSET_LOCAL );

    /* dplay needs to know the size of the header */
    lpspData->dwSPHeaderSize = sizeof(DPSP_MSG_HEADER);

    return DP_OK;
}
