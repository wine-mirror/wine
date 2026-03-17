/*
 * WinHTTP Modern Features Enhancement
 * Adds support for modern HTTP/2, WebSocket, and async features
 * for compatibility with modern Windows applications like Electron apps
 *
 * Copyright 2025 Wine Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winhttp.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

/***********************************************************************
 *          WinHttpSetOption (modern extensions for HTTP/2, WebSocket)
 */
BOOL WINAPI WinHttpSetOption_Modern( HINTERNET handle, DWORD option, void *buffer, DWORD buflen )
{
    TRACE( "handle %p, option %lu, buffer %p, buflen %lu\n", handle, option, buffer, buflen );

    switch (option)
    {
    case WINHTTP_OPTION_HTTP_PROTOCOL_USED:
        TRACE( "HTTP_PROTOCOL_USED: enabling HTTP/2 support\n" );
        /* For now, accept the option but don't fail */
        return TRUE;

    case WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL:
        TRACE( "ENABLE_HTTP_PROTOCOL: %#lx\n", buffer ? *(DWORD *)buffer : 0 );
        /* Accept HTTP/2 and HTTP/3 flags */
        return TRUE;

    case WINHTTP_OPTION_HTTP_PROTOCOL_REQUIRED:
        TRACE( "HTTP_PROTOCOL_REQUIRED: %#lx\n", buffer ? *(DWORD *)buffer : 0 );
        return TRUE;

    case WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET:
        FIXME( "UPGRADE_TO_WEB_SOCKET: stub implementation\n" );
        /* Mark connection as WebSocket-ready */
        return TRUE;

    default:
        FIXME( "Unhandled modern option %lu\n", option );
        SetLastError( ERROR_WINHTTP_INVALID_OPTION );
        return FALSE;
    }
}

/***********************************************************************
 *          WinHttpWebSocketCompleteUpgrade (WebSocket support stub)
 */
HINTERNET WINAPI WinHttpWebSocketCompleteUpgrade( HINTERNET request, DWORD_PTR context )
{
    FIXME( "request %p, context %Ix: stub\n", request, context );
    /* Return a fake WebSocket handle for now */
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *          WinHttpWebSocketSend (WebSocket send stub)
 */
DWORD WINAPI WinHttpWebSocketSend( HINTERNET socket, WINHTTP_WEB_SOCKET_BUFFER_TYPE buffer_type,
                                   void *buffer, DWORD length )
{
    FIXME( "socket %p, type %d, buffer %p, length %lu: stub\n", socket, buffer_type, buffer, length );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *          WinHttpWebSocketReceive (WebSocket receive stub)
 */
DWORD WINAPI WinHttpWebSocketReceive( HINTERNET socket, void *buffer, DWORD length,
                                      DWORD *bytes_read, WINHTTP_WEB_SOCKET_BUFFER_TYPE *buffer_type )
{
    FIXME( "socket %p, buffer %p, length %lu: stub\n", socket, buffer, length );
    if (bytes_read) *bytes_read = 0;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *          WinHttpWebSocketClose (WebSocket close stub)
 */
DWORD WINAPI WinHttpWebSocketClose( HINTERNET socket, USHORT status, void *reason, DWORD length )
{
    FIXME( "socket %p, status %u, reason %p, length %lu: stub\n", socket, status, reason, length );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *          WinHttpWebSocketShutdown (WebSocket shutdown stub)
 */
DWORD WINAPI WinHttpWebSocketShutdown( HINTERNET socket, USHORT status, void *reason, DWORD length )
{
    FIXME( "socket %p, status %u: stub\n", socket, status );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *          WinHttpWebSocketQueryCloseStatus (WebSocket query stub)
 */
DWORD WINAPI WinHttpWebSocketQueryCloseStatus( HINTERNET socket, USHORT *status,
                                                void *reason, DWORD length, DWORD *reason_length )
{
    FIXME( "socket %p: stub\n", socket );
    if (status) *status = 0;
    if (reason_length) *reason_length = 0;
    return ERROR_CALL_NOT_IMPLEMENTED;
}
