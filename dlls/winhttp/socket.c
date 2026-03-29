/*
 * Copyright 2004 Mike McCormack for CodeWeavers
 * Copyright 2006 Rob Shearman for CodeWeavers
 * Copyright 2008, 2011 Hans Leidekker for CodeWeavers
 * Copyright 2009 Juan Lang
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ws2tcpip.h"
#include "ntsecapi.h"
#include "winternl.h"
#include "winhttp.h"

#include "wine/debug.h"
#include "winhttp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhttp);

static void socket_handle_closing( struct object_header *hdr )
{
    struct socket *socket = (struct socket *)hdr;
    BOOL pending_tasks;

    pending_tasks = cancel_queue( &socket->send_q );
    pending_tasks = cancel_queue( &socket->recv_q ) || pending_tasks;

    if (pending_tasks) netconn_cancel_io( socket->netconn );
}

static BOOL socket_query_option( struct object_header *hdr, DWORD option, void *buffer, DWORD *buflen )
{
    switch (option)
    {
    case WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    default:
        break;
    }

    FIXME( "unimplemented option %lu\n", option );
    SetLastError( ERROR_WINHTTP_INVALID_OPTION );
    return FALSE;
}

static void socket_destroy( struct object_header *hdr )
{
    struct socket *socket = (struct socket *)hdr;

    TRACE("%p\n", socket);

    stop_queue( &socket->send_q );
    stop_queue( &socket->recv_q );

    netconn_release( socket->netconn );
    free( socket->read_buffer );
    free( socket->send_frame_buffer );
    free( socket );
}

static BOOL socket_set_option( struct object_header *hdr, DWORD option, void *buffer, DWORD buflen )
{
    struct socket *socket = (struct socket *)hdr;

    switch (option)
    {
    case WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL:
    {
        DWORD interval;

        if (buflen != sizeof(DWORD) || (interval = *(DWORD *)buffer) < 15000)
        {
            WARN( "invalid parameters for WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL\n" );
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        socket->keepalive_interval = interval;
        netconn_set_timeout( socket->netconn, FALSE, socket->keepalive_interval );
        SetLastError( ERROR_SUCCESS );
        TRACE( "WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL %lu\n", interval);
        return TRUE;
    }
    default:
        break;
    }

    FIXME( "unimplemented option %lu\n", option );
    SetLastError( ERROR_WINHTTP_INVALID_OPTION );
    return FALSE;
}

static const struct object_vtbl socket_vtbl =
{
    socket_handle_closing,
    socket_destroy,
    socket_query_option,
    socket_set_option,
};

HINTERNET WINAPI WinHttpWebSocketCompleteUpgrade( HINTERNET hrequest, DWORD_PTR context )
{
    struct socket *socket;
    struct request *request;
    HINTERNET hsocket = NULL;

    TRACE( "%p, %Ix\n", hrequest, context );

    if (!(request = (struct request *)grab_object( hrequest )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return NULL;
    }
    if (request->hdr.type != WINHTTP_HANDLE_TYPE_REQUEST)
    {
        release_object( &request->hdr );
        SetLastError( ERROR_WINHTTP_INCORRECT_HANDLE_TYPE );
        return NULL;
    }
    if (!(socket = calloc( 1, sizeof(*socket) )))
    {
        release_object( &request->hdr );
        return NULL;
    }
    socket->hdr.type = WINHTTP_HANDLE_TYPE_WEBSOCKET;
    socket->hdr.vtbl = &socket_vtbl;
    socket->hdr.refs = 1;
    socket->hdr.callback = request->hdr.callback;
    socket->hdr.notify_mask = request->hdr.notify_mask;
    socket->hdr.context = context;
    socket->hdr.flags = request->connect->hdr.flags & WINHTTP_FLAG_ASYNC;
    socket->keepalive_interval = 30000;
    socket->send_buffer_size = request->websocket_send_buffer_size;
    if (request->read.size)
    {
        if (!(socket->read_buffer = malloc( request->read.size )))
        {
            free( socket );
            release_object( &request->hdr );
            return NULL;
        }
        socket->bytes_in_read_buffer = request->read.size;
        memcpy( socket->read_buffer, request->read.buf + request->read.pos, request->read.size );
        request->read.pos = request->read.size = 0;
    }
    InitializeSRWLock( &socket->send_lock );
    init_queue( &socket->send_q );
    init_queue( &socket->recv_q );
    netconn_addref( request->netconn );
    socket->netconn = request->netconn;

    netconn_set_timeout( socket->netconn, FALSE, socket->keepalive_interval );

    if ((hsocket = alloc_handle( &socket->hdr )))
    {
        send_callback( &request->hdr, WINHTTP_CALLBACK_STATUS_HANDLE_CREATED, &hsocket, sizeof(hsocket) );
    }

    release_object( &socket->hdr );
    release_object( &request->hdr );
    TRACE("returning %p\n", hsocket);
    if (hsocket) SetLastError( ERROR_SUCCESS );
    return hsocket;
}

static DWORD send_bytes( struct socket *socket, char *bytes, int len, int *sent, WSAOVERLAPPED *ovr )
{
    int count;
    DWORD err;
    err = netconn_send( socket->netconn, bytes, len, &count, ovr );
    if (sent) *sent = count;
    if (err) return err;
    return (count == len || (ovr && count)) ? ERROR_SUCCESS : ERROR_INTERNAL_ERROR;
}

#define FIN_BIT      (1 << 7)
#define MASK_BIT     (1 << 7)
#define RESERVED_BIT (7 << 4)
#define CONTROL_BIT  (1 << 3)

static DWORD send_frame( struct socket *socket, enum socket_opcode opcode, USHORT status, const char *buf,
                         DWORD buflen, BOOL final, WSAOVERLAPPED *ovr )
{
    DWORD i, offset = 2, len = buflen, buffer_size, ret = 0;
    int sent_size;
    char hdr[14];
    char *ptr;

    TRACE( "sending %02x frame, len %lu\n", opcode, len );

    if (opcode == SOCKET_OPCODE_CLOSE) len += sizeof(status);

    hdr[0] = final ? (char)FIN_BIT : 0;
    hdr[0] |= opcode;
    hdr[1] = (char)MASK_BIT;
    if (len < 126) hdr[1] |= len;
    else if (len < 65536)
    {
        hdr[1] |= 126;
        hdr[2] = len >> 8;
        hdr[3] = len & 0xff;
        offset += 2;
    }
    else
    {
        hdr[1] |= 127;
        hdr[2] = hdr[3] = hdr[4] = hdr[5] = 0;
        hdr[6] = len >> 24;
        hdr[7] = (len >> 16) & 0xff;
        hdr[8] = (len >> 8) & 0xff;
        hdr[9] = len & 0xff;
        offset += 8;
    }

    buffer_size = len + offset + 4;
    assert( buffer_size - len < socket->send_buffer_size );
    if (buffer_size > socket->send_frame_buffer_size && socket->send_frame_buffer_size < socket->send_buffer_size)
    {
        DWORD new_size;
        void *new;

        new_size = min( buffer_size, socket->send_buffer_size );
        if (!(new = realloc( socket->send_frame_buffer, new_size )))
        {
            ERR( "out of memory, buffer_size %lu\n", buffer_size );
            return ERROR_OUTOFMEMORY;
        }
        socket->send_frame_buffer = new;
        socket->send_frame_buffer_size = new_size;
    }
    ptr = socket->send_frame_buffer;

    memcpy(ptr, hdr, offset);
    ptr += offset;

    RtlGenRandom( socket->mask, 4 );
    memcpy( ptr, socket->mask, 4 );
    ptr += 4;
    socket->mask_index = 0;

    if (opcode == SOCKET_OPCODE_CLOSE) /* prepend status code */
    {
        *ptr++ = (status >> 8) ^ socket->mask[socket->mask_index++ % 4];
        *ptr++ = (status & 0xff) ^ socket->mask[socket->mask_index++ % 4];
    }

    offset = ptr - socket->send_frame_buffer;
    socket->send_remaining_size = offset + buflen;
    socket->client_buffer_offset = 0;
    while (socket->send_remaining_size)
    {
        len = min( buflen, socket->send_buffer_size - offset );
        for (i = 0; i < len; ++i)
        {
            socket->send_frame_buffer[offset++] = buf[socket->client_buffer_offset++]
                                                  ^ socket->mask[socket->mask_index++ % 4];
        }

        sent_size = 0;
        ret = send_bytes( socket, socket->send_frame_buffer, offset, &sent_size, ovr );
        socket->send_remaining_size -= sent_size;
        if (ret)
        {
            if (ovr && ret == WSA_IO_PENDING)
            {
                memmove( socket->send_frame_buffer, socket->send_frame_buffer + sent_size, offset - sent_size );
                socket->bytes_in_send_frame_buffer = offset - sent_size;
            }
            return ret;
        }
        assert( sent_size == offset );
        offset = 0;
        buflen -= len;
    }
    return ERROR_SUCCESS;
}

static DWORD complete_send_frame( struct socket *socket, WSAOVERLAPPED *ovr, const char *buf )
{
    DWORD ret, len, i;

    if (!netconn_wait_overlapped_result( socket->netconn, ovr, &len )) return WSAGetLastError();

    if (socket->bytes_in_send_frame_buffer)
    {
        ret = send_bytes( socket, socket->send_frame_buffer, socket->bytes_in_send_frame_buffer, NULL, NULL );
        if (ret) return ret;
    }

    assert( socket->bytes_in_send_frame_buffer <= socket->send_remaining_size );
    socket->send_remaining_size -= socket->bytes_in_send_frame_buffer;

    while (socket->send_remaining_size)
    {
        len = min( socket->send_remaining_size, socket->send_buffer_size );
        for (i = 0; i < len; ++i)
        {
            socket->send_frame_buffer[i] = buf[socket->client_buffer_offset++]
                                           ^ socket->mask[socket->mask_index++ % 4];
        }
        ret = send_bytes( socket, socket->send_frame_buffer, len, NULL, NULL );
        if (ret) return ret;
        socket->send_remaining_size -= len;
    }
    return ERROR_SUCCESS;
}

static void send_io_complete( struct object_header *hdr )
{
    LONG count = InterlockedDecrement( &hdr->pending_sends );
    assert( count >= 0 );
}

static void receive_io_complete( struct socket *socket )
{
    LONG count = InterlockedDecrement( &socket->hdr.pending_receives );
    assert( count >= 0 );
}

static BOOL socket_can_send( struct socket *socket )
{
    return socket->state == SOCKET_STATE_OPEN && !socket->close_frame_received;
}

static BOOL socket_can_receive( struct socket *socket )
{
    return socket->state <= SOCKET_STATE_SHUTDOWN && !socket->close_frame_received;
}

static BOOL validate_buffer_type( WINHTTP_WEB_SOCKET_BUFFER_TYPE type, enum fragment_type current_fragment )
{
    switch (current_fragment)
    {
    case SOCKET_FRAGMENT_NONE:
        return type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE ||
               type == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE ||
               type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
               type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;

    case SOCKET_FRAGMENT_BINARY:
        return type == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE ||
               type == WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;

    case SOCKET_FRAGMENT_UTF8:
        return type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE ||
               type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
    }
    assert( 0 );
    return FALSE;
}

static enum socket_opcode map_buffer_type( struct socket *socket, WINHTTP_WEB_SOCKET_BUFFER_TYPE type )
{
    switch (type)
    {
    case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
        if (socket->sending_fragment_type)
        {
            socket->sending_fragment_type = SOCKET_FRAGMENT_NONE;
            return SOCKET_OPCODE_CONTINUE;
        }
        return SOCKET_OPCODE_TEXT;

    case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
        if (socket->sending_fragment_type)
        {
            socket->sending_fragment_type = SOCKET_FRAGMENT_NONE;
            return SOCKET_OPCODE_CONTINUE;
        }
        return SOCKET_OPCODE_BINARY;

    case WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
        if (!socket->sending_fragment_type)
        {
            socket->sending_fragment_type = SOCKET_FRAGMENT_UTF8;
            return SOCKET_OPCODE_TEXT;
        }
        return SOCKET_OPCODE_CONTINUE;

    case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
        if (!socket->sending_fragment_type)
        {
            socket->sending_fragment_type = SOCKET_FRAGMENT_BINARY;
            return SOCKET_OPCODE_BINARY;
        }
        return SOCKET_OPCODE_CONTINUE;

    case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
        return SOCKET_OPCODE_CLOSE;

    default:
        FIXME("buffer type %u not supported\n", type);
        return SOCKET_OPCODE_INVALID;
    }
}

static void socket_send_complete( struct socket *socket, DWORD ret, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, DWORD len )
{
    if (!ret)
    {
        WINHTTP_WEB_SOCKET_STATUS status;
        status.dwBytesTransferred = len;
        status.eBufferType        = type;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE, &status, sizeof(status) );
    }
    else
    {
        WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
        result.AsyncResult.dwResult = API_WRITE_DATA;
        result.AsyncResult.dwError  = ret;
        result.Operation = WINHTTP_WEB_SOCKET_SEND_OPERATION;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
    }
}

static DWORD socket_send( struct socket *socket, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, const void *buf, DWORD len,
                          WSAOVERLAPPED *ovr )
{
    enum socket_opcode opcode = map_buffer_type( socket, type );
    BOOL final = (type != WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE &&
                  type != WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE);

    return send_frame( socket, opcode, 0, buf, len, final, ovr );
}

static void task_socket_send( void *ctx, BOOL abort )
{
    struct socket_send *s = ctx;
    struct socket *socket = (struct socket *)s->task_hdr.obj;
    DWORD ret;

    if (abort) return;

    TRACE("running %p\n", ctx);

    if (s->complete_async) ret = complete_send_frame( socket, &s->ovr, s->buf );
    else ret = socket_send( socket, s->type, s->buf, s->len, NULL );

    send_io_complete( &socket->hdr );
    InterlockedExchange( &socket->pending_noncontrol_send, 0 );
    socket_send_complete( socket, ret, s->type, s->len );
}

DWORD WINAPI WinHttpWebSocketSend( HINTERNET hsocket, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, void *buf, DWORD len )
{
    struct socket *socket;
    DWORD ret = 0;

    TRACE( "%p, %u, %p, %lu\n", hsocket, type, buf, len );

    if (len && !buf) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_WEBSOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (!socket_can_send( socket ))
    {
        release_object( &socket->hdr );
        return ERROR_INVALID_OPERATION;
    }

    if (socket->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        BOOL async_send, complete_async = FALSE;
        struct socket_send *s;

        if (InterlockedCompareExchange( &socket->pending_noncontrol_send, 1, 0 ))
        {
            WARN( "previous send is still queued\n" );
            release_object( &socket->hdr );
            return ERROR_INVALID_OPERATION;
        }
        if (!validate_buffer_type( type, socket->sending_fragment_type ))
        {
            WARN( "invalid buffer type %u, sending_fragment_type %u\n", type, socket->sending_fragment_type );
            InterlockedExchange( &socket->pending_noncontrol_send, 0 );
            release_object( &socket->hdr );
            return ERROR_INVALID_PARAMETER;
        }

        if (!(s = malloc( sizeof(*s) )))
        {
            InterlockedExchange( &socket->pending_noncontrol_send, 0 );
            release_object( &socket->hdr );
            return ERROR_OUTOFMEMORY;
        }

        AcquireSRWLockExclusive( &socket->send_lock );
        async_send = InterlockedIncrement( &socket->hdr.pending_sends ) > 1 || socket->hdr.recursion_count >= 3;
        if (!async_send)
        {
            memset( &s->ovr, 0, sizeof(s->ovr) );
            if ((ret = socket_send( socket, type, buf, len, &s->ovr )) == WSA_IO_PENDING)
            {
                async_send = TRUE;
                complete_async = TRUE;
            }
        }

        if (async_send)
        {
            s->complete_async = complete_async;
            TRACE( "queueing complete_async %#x\n", complete_async );
            s->type   = type;
            s->buf    = buf;
            s->len    = len;

            if ((ret = queue_task( &socket->send_q, task_socket_send, &s->task_hdr, &socket->hdr )))
                free( s );
        }
        if (!async_send || ret)
        {
            InterlockedDecrement( &socket->hdr.pending_sends );
            InterlockedExchange( &socket->pending_noncontrol_send, 0 );
        }
        ReleaseSRWLockExclusive( &socket->send_lock );
        if (!async_send)
        {
            TRACE( "sent sync\n" );
            free( s );
            socket_send_complete( socket, ret, type, len );
            ret = ERROR_SUCCESS;
        }
    }
    else
    {
        if (validate_buffer_type( type, socket->sending_fragment_type ))
        {
            ret = socket_send( socket, type, buf, len, NULL );
        }
        else
        {
            WARN( "invalid buffer type %u, sending_fragment_type %u\n", type, socket->sending_fragment_type );
            ret = ERROR_INVALID_PARAMETER;
        }
    }

    release_object( &socket->hdr );
    return ret;
}

static DWORD receive_bytes( struct socket *socket, char *buf, DWORD len, DWORD *ret_len, BOOL read_full_buffer )
{
    DWORD err, size = 0, needed = len;
    char *ptr = buf;
    int received;

    if (socket->bytes_in_read_buffer)
    {
        size = min( needed, socket->bytes_in_read_buffer );
        memcpy( ptr, socket->read_buffer, size );
        memmove( socket->read_buffer, socket->read_buffer + size, socket->bytes_in_read_buffer - size );
        socket->bytes_in_read_buffer -= size;
        needed -= size;
        ptr += size;
    }
    while (size != len)
    {
        if ((err = netconn_recv( socket->netconn, ptr, needed, 0, &received ))) return err;
        if (!received) break;
        size += received;
        if (!read_full_buffer) break;
        needed -= received;
        ptr += received;
    }
    *ret_len = size;
    if (size != len && (read_full_buffer || !size)) return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    return ERROR_SUCCESS;
}

static BOOL is_supported_opcode( enum socket_opcode opcode )
{
    switch (opcode)
    {
    case SOCKET_OPCODE_CONTINUE:
    case SOCKET_OPCODE_TEXT:
    case SOCKET_OPCODE_BINARY:
    case SOCKET_OPCODE_CLOSE:
    case SOCKET_OPCODE_PING:
    case SOCKET_OPCODE_PONG:
        return TRUE;
    default:
        FIXME( "opcode %02x not handled\n", opcode );
        return FALSE;
    }
}

static DWORD receive_frame( struct socket *socket, DWORD *ret_len, enum socket_opcode *opcode, BOOL *final )
{
    DWORD ret, len, count;
    char hdr[2];

    if ((ret = receive_bytes( socket, hdr, sizeof(hdr), &count, TRUE ))) return ret;
    if ((hdr[0] & RESERVED_BIT) || (hdr[1] & MASK_BIT) || !is_supported_opcode( hdr[0] & 0xf ))
    {
        return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    }
    *opcode = hdr[0] & 0xf;
    *final = hdr[0] & FIN_BIT;
    TRACE("received %02x frame, final %#x\n", *opcode, *final);

    len = hdr[1] & ~MASK_BIT;
    if (len == 126)
    {
        USHORT len16;
        if ((ret = receive_bytes( socket, (char *)&len16, sizeof(len16), &count, TRUE ))) return ret;
        len = RtlUshortByteSwap( len16 );
    }
    else if (len == 127)
    {
        ULONGLONG len64;
        if ((ret = receive_bytes( socket, (char *)&len64, sizeof(len64), &count, TRUE ))) return ret;
        if ((len64 = RtlUlonglongByteSwap( len64 )) > ~0u) return ERROR_NOT_SUPPORTED;
        len = len64;
    }

    *ret_len = len;
    return ERROR_SUCCESS;
}

static void task_socket_send_pong( void *ctx, BOOL abort )
{
    struct socket_send *s = ctx;
    struct socket *socket = (struct socket *)s->task_hdr.obj;

    if (abort) return;

    TRACE("running %p\n", ctx);

    if (s->complete_async) complete_send_frame( socket, &s->ovr, NULL );
    else send_frame( socket, SOCKET_OPCODE_PONG, 0, NULL, 0, TRUE, NULL );

    send_io_complete( &socket->hdr );
}

static DWORD socket_send_pong( struct socket *socket )
{
    BOOL async_send, complete_async = FALSE;
    struct socket_send *s;
    DWORD ret = 0;

    if (!(socket->hdr.flags & WINHTTP_FLAG_ASYNC))
        return send_frame( socket, SOCKET_OPCODE_PONG, 0, NULL, 0, TRUE, NULL );

    if (!(s = malloc( sizeof(*s) ))) return ERROR_OUTOFMEMORY;

    AcquireSRWLockExclusive( &socket->send_lock );
    async_send = InterlockedIncrement( &socket->hdr.pending_sends ) > 1;
    if (!async_send)
    {
        memset( &s->ovr, 0, sizeof(s->ovr) );
        if ((ret = send_frame( socket, SOCKET_OPCODE_PONG, 0, NULL, 0, TRUE, &s->ovr )) == WSA_IO_PENDING)
        {
            async_send = TRUE;
            complete_async = TRUE;
        }
    }

    if (async_send)
    {
        s->complete_async = complete_async;
        if ((ret = queue_task( &socket->send_q, task_socket_send_pong, &s->task_hdr, &socket->hdr )))
        {
            InterlockedDecrement( &socket->hdr.pending_sends );
            free( s );
        }
    }
    else
    {
        InterlockedDecrement( &socket->hdr.pending_sends );
        free( s );
    }
    ReleaseSRWLockExclusive( &socket->send_lock );
    return ret;
}

static DWORD socket_drain( struct socket *socket )
{
    DWORD ret, count;

    while (socket->read_size)
    {
        char buf[1024];
        if ((ret = receive_bytes( socket, buf, min(socket->read_size, sizeof(buf)), &count, TRUE ))) return ret;
        socket->read_size -= count;
    }
    return ERROR_SUCCESS;
}

static DWORD receive_close_status( struct socket *socket, DWORD len )
{
    DWORD reason_len, ret;

    socket->close_frame_received = TRUE;
    if ((len && (len < sizeof(socket->status) || len > sizeof(socket->status) + sizeof(socket->reason))))
        return (socket->close_frame_receive_err = ERROR_WINHTTP_INVALID_SERVER_RESPONSE);

    if (!len) return (socket->close_frame_receive_err = ERROR_SUCCESS);

    reason_len = len - sizeof(socket->status);
    if ((ret = receive_bytes( socket, (char *)&socket->status, sizeof(socket->status), &len, TRUE )))
        return (socket->close_frame_receive_err = ret);

    socket->status = RtlUshortByteSwap( socket->status );
    return (socket->close_frame_receive_err
            = receive_bytes( socket, socket->reason, reason_len, &socket->reason_len, TRUE ));
}

static DWORD handle_control_frame( struct socket *socket )
{
    DWORD ret;

    TRACE( "opcode %u\n", socket->opcode );

    switch (socket->opcode)
    {
    case SOCKET_OPCODE_PING:
        return socket_send_pong( socket );

    case SOCKET_OPCODE_PONG:
        return socket_drain( socket );

    case SOCKET_OPCODE_CLOSE:
        if (socket->state < SOCKET_STATE_SHUTDOWN)
            WARN( "SOCKET_OPCODE_CLOSE received, socket->state %u\n", socket->state );
        if (socket->close_frame_received)
        {
            FIXME( "close frame already received\n" );
            return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        }

        ret = receive_close_status( socket, socket->read_size );
        socket->read_size = 0;
        return ret;

    default:
        ERR( "unhandled control opcode %02x\n", socket->opcode );
        return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
    }

    return ERROR_SUCCESS;
}

static WINHTTP_WEB_SOCKET_BUFFER_TYPE map_opcode( struct socket *socket, enum socket_opcode opcode, BOOL fragment )
{
    enum fragment_type frag_type = socket->receiving_fragment_type;

    switch (opcode)
    {
    case SOCKET_OPCODE_TEXT:
        if (frag_type && frag_type != SOCKET_FRAGMENT_UTF8)
            FIXME( "received SOCKET_OPCODE_TEXT with prev fragment %u\n", frag_type );
        if (fragment)
        {
            socket->receiving_fragment_type = SOCKET_FRAGMENT_UTF8;
            return WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
        }
        socket->receiving_fragment_type = SOCKET_FRAGMENT_NONE;
        return WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

    case SOCKET_OPCODE_BINARY:
        if (frag_type && frag_type != SOCKET_FRAGMENT_BINARY)
            FIXME( "received SOCKET_OPCODE_BINARY with prev fragment %u\n", frag_type );
        if (fragment)
        {
            socket->receiving_fragment_type = SOCKET_FRAGMENT_BINARY;
            return WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
        }
        socket->receiving_fragment_type = SOCKET_FRAGMENT_NONE;
        return WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE;

    case SOCKET_OPCODE_CONTINUE:
        if (!frag_type)
        {
            FIXME( "received SOCKET_OPCODE_CONTINUE without starting fragment\n" );
            return ~0u;
        }
        if (fragment)
        {
            return frag_type == SOCKET_FRAGMENT_BINARY ? WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE
                                                       : WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
        }
        socket->receiving_fragment_type = SOCKET_FRAGMENT_NONE;
        return frag_type == SOCKET_FRAGMENT_BINARY ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE
                                                   : WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE;

    case SOCKET_OPCODE_CLOSE:
        return WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE;

    default:
        FIXME( "opcode %02x not handled\n", opcode );
        return ~0u;
    }
}

static DWORD socket_receive( struct socket *socket, void *buf, DWORD len, DWORD *ret_len,
                             WINHTTP_WEB_SOCKET_BUFFER_TYPE *ret_type )
{
    BOOL final = socket->last_receive_final;
    DWORD count, ret = ERROR_SUCCESS;

    if (!socket->read_size)
    {
        for (;;)
        {
            if (!(ret = receive_frame( socket, &socket->read_size, &socket->opcode, &final )))
            {
                if (!(socket->opcode & CONTROL_BIT) || (ret = handle_control_frame( socket ))
                    || socket->opcode == SOCKET_OPCODE_CLOSE) break;
            }
            else if (ret == WSAETIMEDOUT) ret = socket_send_pong( socket );
            if (ret) break;
        }
    }
    if (!ret)
    {
        socket->last_receive_final = final;
        ret = receive_bytes( socket, buf, min(len, socket->read_size), &count, FALSE );
    }
    if (!ret)
    {
        if (count < socket->read_size) WARN( "short read\n" );

        socket->read_size -= count;
        *ret_len = count;
        *ret_type = map_opcode( socket, socket->opcode, !final || socket->read_size != 0 );
        TRACE( "len %lu, *ret_len %lu, *ret_type %u\n", len, *ret_len, *ret_type );
        if (*ret_type == ~0u)
        {
            FIXME( "unexpected opcode %u\n", socket->opcode );
            socket->read_size = 0;
            return ERROR_WINHTTP_INVALID_SERVER_RESPONSE;
        }
    }
    return ret;
}

static void socket_receive_complete( struct socket *socket, DWORD ret, WINHTTP_WEB_SOCKET_BUFFER_TYPE type, DWORD len )
{
    if (!ret)
    {
        WINHTTP_WEB_SOCKET_STATUS status;
        status.dwBytesTransferred = len;
        status.eBufferType        = type;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_READ_COMPLETE, &status, sizeof(status) );
    }
    else
    {
        WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
        result.AsyncResult.dwResult = 0;
        result.AsyncResult.dwError  = ret;
        result.Operation = WINHTTP_WEB_SOCKET_RECEIVE_OPERATION;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
    }
}

static void task_socket_receive( void *ctx, BOOL abort )
{
    struct socket_receive *r = ctx;
    struct socket *socket = (struct socket *)r->task_hdr.obj;
    DWORD ret, count;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;

    if (abort)
    {
        socket_receive_complete( socket, ERROR_WINHTTP_OPERATION_CANCELLED, 0, 0 );
        return;
    }

    TRACE( "running %p\n", ctx );
    ret = socket_receive( socket, r->buf, r->len, &count, &type );
    receive_io_complete( socket );
    if (task_needs_completion( &r->task_hdr )) socket_receive_complete( socket, ret, type, count );
}

DWORD WINAPI WinHttpWebSocketReceive( HINTERNET hsocket, void *buf, DWORD len, DWORD *ret_len,
                                      WINHTTP_WEB_SOCKET_BUFFER_TYPE *ret_type )
{
    struct socket *socket;
    DWORD ret;

    TRACE( "%p, %p, %lu, %p, %p\n", hsocket, buf, len, ret_len, ret_type );

    if (!buf || !len) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_WEBSOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (!socket_can_receive( socket ))
    {
        release_object( &socket->hdr );
        return ERROR_INVALID_OPERATION;
    }

    if (socket->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_receive *r;

        if (InterlockedIncrement( &socket->hdr.pending_receives ) > 1)
        {
            InterlockedDecrement( &socket->hdr.pending_receives );
            WARN( "attempt to queue receive while another is pending\n" );
            release_object( &socket->hdr );
            return ERROR_INVALID_OPERATION;
        }

        if (!(r = malloc( sizeof(*r) )))
        {
            InterlockedDecrement( &socket->hdr.pending_receives );
            release_object( &socket->hdr );
            return ERROR_OUTOFMEMORY;
        }
        r->buf = buf;
        r->len = len;

        if ((ret = queue_task( &socket->recv_q, task_socket_receive, &r->task_hdr, &socket->hdr )))
        {
            InterlockedDecrement( &socket->hdr.pending_receives );
            free( r );
        }
    }
    else ret = socket_receive( socket, buf, len, ret_len, ret_type );

    release_object( &socket->hdr );
    return ret;
}

static void socket_shutdown_complete( struct socket *socket, DWORD ret )
{
    if (!ret) send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE, NULL, 0 );
    else
    {
        WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
        result.AsyncResult.dwResult = API_WRITE_DATA;
        result.AsyncResult.dwError  = ret;
        result.Operation = WINHTTP_WEB_SOCKET_SHUTDOWN_OPERATION;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
    }
}

static void task_socket_shutdown( void *ctx, BOOL abort )
{
    struct socket_shutdown *s = ctx;
    struct socket *socket = (struct socket *)s->task_hdr.obj;
    DWORD ret;

    if (abort) return;

    TRACE( "running %p\n", ctx );

    if (s->complete_async) ret = complete_send_frame( socket, &s->ovr, s->reason );
    else ret = send_frame( socket, SOCKET_OPCODE_CLOSE, s->status, s->reason, s->len, TRUE, NULL );

    send_io_complete( &socket->hdr );
    if (s->send_callback) socket_shutdown_complete( socket, ret );
}

static DWORD send_socket_shutdown( struct socket *socket, USHORT status, const void *reason, DWORD len,
                                   BOOL send_callback)
{
    BOOL async_send, complete_async = FALSE;
    struct socket_shutdown *s;
    DWORD ret;

    if (socket->state < SOCKET_STATE_SHUTDOWN) socket->state = SOCKET_STATE_SHUTDOWN;

    if (!(socket->hdr.flags & WINHTTP_FLAG_ASYNC))
        return send_frame( socket, SOCKET_OPCODE_CLOSE, status, reason, len, TRUE, NULL );

    if (!(s = malloc( sizeof(*s) ))) return FALSE;

    AcquireSRWLockExclusive( &socket->send_lock );
    async_send = InterlockedIncrement( &socket->hdr.pending_sends ) > 1 || socket->hdr.recursion_count >= 3;
    if (!async_send)
    {
        memset( &s->ovr, 0, sizeof(s->ovr) );
        if ((ret = send_frame( socket, SOCKET_OPCODE_CLOSE, status, reason, len, TRUE, &s->ovr )) == WSA_IO_PENDING)
        {
            async_send = TRUE;
            complete_async = TRUE;
        }
    }

    if (async_send)
    {
        s->complete_async = complete_async;
        s->status = status;
        memcpy( s->reason, reason, len );
        s->len    = len;
        s->send_callback = send_callback;

        if ((ret = queue_task( &socket->send_q, task_socket_shutdown, &s->task_hdr, &socket->hdr )))
        {
            InterlockedDecrement( &socket->hdr.pending_sends );
            free( s );
        }
    }
    else InterlockedDecrement( &socket->hdr.pending_sends );

    ReleaseSRWLockExclusive( &socket->send_lock );
    if (!async_send)
    {
        free( s );
        if (send_callback)
        {
            socket_shutdown_complete( socket, ret );
            ret = ERROR_SUCCESS;
        }
    }
    return ret;
}

DWORD WINAPI WinHttpWebSocketShutdown( HINTERNET hsocket, USHORT status, void *reason, DWORD len )
{
    struct socket *socket;
    DWORD ret;

    TRACE( "%p, %u, %p, %lu\n", hsocket, status, reason, len );

    if ((len && !reason) || len > sizeof(socket->reason)) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_WEBSOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state >= SOCKET_STATE_SHUTDOWN)
    {
        release_object( &socket->hdr );
        return ERROR_INVALID_OPERATION;
    }

    ret = send_socket_shutdown( socket, status, reason, len, TRUE );
    release_object( &socket->hdr );
    return ret;
}

static DWORD socket_close( struct socket *socket )
{
    BOOL final = FALSE;
    DWORD ret, count;

    if (socket->close_frame_received) return socket->close_frame_receive_err;

    if ((ret = socket_drain( socket ))) return ret;

    while (1)
    {
        if ((ret = receive_frame( socket, &count, &socket->opcode, &final ))) return ret;
        if (socket->opcode == SOCKET_OPCODE_CLOSE) break;

        socket->read_size = count;
        if ((ret = socket_drain( socket ))) return ret;
    }
    if (!final) FIXME( "received close opcode without FIN bit\n" );

    return receive_close_status( socket, count );
}

static void socket_close_complete( struct socket *socket, DWORD ret )
{
    if (!ret) send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE, NULL, 0 );
    else
    {
        WINHTTP_WEB_SOCKET_ASYNC_RESULT result;
        result.AsyncResult.dwResult = API_READ_DATA; /* FIXME */
        result.AsyncResult.dwError  = ret;
        result.Operation = WINHTTP_WEB_SOCKET_CLOSE_OPERATION;
        send_callback( &socket->hdr, WINHTTP_CALLBACK_STATUS_REQUEST_ERROR, &result, sizeof(result) );
    }
}

static void task_socket_close( void *ctx, BOOL abort )
{
    struct socket_shutdown *s = ctx;
    struct socket *socket = (struct socket *)s->task_hdr.obj;
    DWORD ret;

    if (abort)
    {
        socket_close_complete( socket, ERROR_WINHTTP_OPERATION_CANCELLED );
        return;
    }

    TRACE( "running %p\n", ctx );

    ret = socket_close( socket );
    receive_io_complete( socket );
    if (task_needs_completion( &s->task_hdr )) socket_close_complete( socket, ret );
}

DWORD WINAPI WinHttpWebSocketClose( HINTERNET hsocket, USHORT status, void *reason, DWORD len )
{
    enum socket_state prev_state;
    LONG pending_receives = 0;
    struct socket *socket;
    DWORD ret;

    TRACE( "%p, %u, %p, %lu\n", hsocket, status, reason, len );

    if ((len && !reason) || len > sizeof(socket->reason)) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_WEBSOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }
    if (socket->state >= SOCKET_STATE_CLOSED)
    {
        release_object( &socket->hdr );
        return ERROR_INVALID_OPERATION;
    }

    prev_state = socket->state;
    socket->state = SOCKET_STATE_CLOSED;

    if (socket->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        pending_receives = InterlockedIncrement( &socket->hdr.pending_receives );
        cancel_queue( &socket->recv_q );
    }

    if (prev_state < SOCKET_STATE_SHUTDOWN && (ret = send_socket_shutdown( socket, status, reason, len, FALSE )))
        goto done;

    if (pending_receives == 1 && socket->close_frame_received)
    {
        if (socket->hdr.flags & WINHTTP_FLAG_ASYNC) socket_close_complete( socket, socket->close_frame_receive_err );
        goto done;
    }

    if (socket->hdr.flags & WINHTTP_FLAG_ASYNC)
    {
        struct socket_shutdown *s;

        if (!(s = calloc( 1, sizeof(*s) )))
        {
            ret = ERROR_OUTOFMEMORY;
            goto done;
        }
        if ((ret = queue_task( &socket->recv_q, task_socket_close, &s->task_hdr, &socket->hdr )))
        {
            InterlockedDecrement( &socket->hdr.pending_receives );
            free( s );
        }
    }
    else ret = socket_close( socket );

done:
    release_object( &socket->hdr );
    return ret;
}

DWORD WINAPI WinHttpWebSocketQueryCloseStatus( HINTERNET hsocket, USHORT *status, void *reason, DWORD len,
                                               DWORD *ret_len )
{
    struct socket *socket;
    DWORD ret;

    TRACE( "%p, %p, %p, %lu, %p\n", hsocket, status, reason, len, ret_len );

    if (!status || (len && !reason) || !ret_len) return ERROR_INVALID_PARAMETER;

    if (!(socket = (struct socket *)grab_object( hsocket ))) return ERROR_INVALID_HANDLE;
    if (socket->hdr.type != WINHTTP_HANDLE_TYPE_WEBSOCKET)
    {
        release_object( &socket->hdr );
        return ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    }

    if (!socket->close_frame_received || socket->close_frame_receive_err)
    {
        ret = socket->close_frame_received ? socket->close_frame_receive_err : ERROR_INVALID_OPERATION;
        release_object( &socket->hdr );
        return ret;
    }

    *status = socket->status;
    *ret_len = socket->reason_len;
    if (socket->reason_len > len) ret = ERROR_INSUFFICIENT_BUFFER;
    else
    {
        memcpy( reason, socket->reason, socket->reason_len );
        ret = ERROR_SUCCESS;
    }

    release_object( &socket->hdr );
    return ret;
}
