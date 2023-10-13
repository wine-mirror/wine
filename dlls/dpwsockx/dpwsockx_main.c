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

    dpwsData->started = TRUE;

    return S_OK;
}

static void DPWS_Stop( DPWS_DATA *dpwsData )
{
    if ( !dpwsData->started )
        return;

    dpwsData->started = FALSE;

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
    FIXME( "(%ld,0x%08lx,%p,%p) stub\n",
           data->idPlayer, data->dwFlags,
           data->lpSPMessageHeader, data->lpISP );
    return DPERR_UNSUPPORTED;
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
    FIXME( "(%u,%p,%p,%u,0x%08lx,0x%08lx) stub\n",
           data->bCreate, data->lpSPMessageHeader, data->lpISP,
           data->bReturnStatus, data->dwOpenFlags, data->dwSessionFlags );
    return DPERR_UNSUPPORTED;
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
    FIXME( "(%p,0x%08lx,%ld,%ld,%p,%ld,%ld,%ld,%ld,%p,%p,%u) stub\n",
           data->lpISP, data->dwFlags, data->idPlayerTo, data->idPlayerFrom,
           data->lpSendBuffers, data->cBuffers, data->dwMessageSize,
           data->dwPriority, data->dwTimeout, data->lpDPContext,
           data->lpdwSPMsgID, data->bSystemMessage );
    return DPERR_UNSUPPORTED;
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
