/*
 * Definitions for the client side of the Wine server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
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

#ifndef __WINE_WINE_SERVER_H
#define __WINE_WINE_SERVER_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winternl.h>
#include <wine/exception.h>
#include <wine/server_protocol.h>

/* client communication functions */

struct __server_iovec
{
    const void  *ptr;
    unsigned int size;
};

#define __SERVER_MAX_DATA 5

struct __server_request_info
{
    union
    {
        union generic_request req;    /* request structure */
        union generic_reply   reply;  /* reply structure */
    } u;
    unsigned int          data_count; /* count of request data pointers */
    void                 *reply_data; /* reply data pointer */
    struct __server_iovec data[__SERVER_MAX_DATA];  /* request variable size data */
};

extern unsigned int wine_server_call( void *req_ptr );
extern void wine_server_send_fd( int fd );
extern int wine_server_fd_to_handle( int fd, unsigned int access, int inherit, obj_handle_t *handle );
extern int wine_server_handle_to_fd( obj_handle_t handle, unsigned int access, int *unix_fd,
                                     enum fd_type *type, int *flags );

/* do a server call and set the last error code */
inline static unsigned int wine_server_call_err( void *req_ptr )
{
    unsigned int res = wine_server_call( req_ptr );
    if (res) SetLastError( RtlNtStatusToDosError(res) );
    return res;
}

/* get the size of the variable part of the returned reply */
inline static size_t wine_server_reply_size( const void *reply )
{
    return ((struct reply_header *)reply)->reply_size;
}

/* add some data to be sent along with the request */
inline static void wine_server_add_data( void *req_ptr, const void *ptr, unsigned int size )
{
    struct __server_request_info * const req = req_ptr;
    if (size)
    {
        req->data[req->data_count].ptr = ptr;
        req->data[req->data_count++].size = size;
        req->u.req.request_header.request_size += size;
    }
}

/* set the pointer and max size for the reply var data */
inline static void wine_server_set_reply( void *req_ptr, void *ptr, unsigned int max_size )
{
    struct __server_request_info * const req = req_ptr;
    req->reply_data = ptr;
    req->u.req.request_header.reply_size = max_size;
}


/* macros for server requests */

#define SERVER_START_REQ(type) \
    do { \
        struct __server_request_info __req; \
        struct type##_request * const req = &__req.u.req.type##_request; \
        const struct type##_reply * const reply = &__req.u.reply.type##_reply; \
        memset( &__req.u.req, 0, sizeof(__req.u.req) ); \
        __req.u.req.request_header.req = REQ_##type; \
        __req.data_count = 0; \
        (void)reply; \
        do

#define SERVER_END_REQ \
        while(0); \
    } while(0)


/* non-exported functions */
extern void DECLSPEC_NORETURN server_protocol_error( const char *err, ... );
extern void DECLSPEC_NORETURN server_protocol_perror( const char *err );
extern void CLIENT_InitServer(void);
extern void CLIENT_InitThread(void);
extern void CLIENT_BootDone( int debug_level );
extern int CLIENT_IsBootThread(void);

#endif  /* __WINE_WINE_SERVER_H */
