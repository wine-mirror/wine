/*
 * Definitions for the client side of the Wine server communication
 *
 * Copyright (C) 1998 Alexandre Julliard
 */

#ifndef __WINE_WINE_SERVER_H
#define __WINE_WINE_SERVER_H

#include "thread.h"
#include "ntddk.h"
#include "wine/exception.h"
#include "wine/server_protocol.h"

/* client communication functions */

extern unsigned int wine_server_call( union generic_request *req, size_t size );
extern void server_protocol_error( const char *err, ... ) WINE_NORETURN;
extern void server_protocol_perror( const char *err ) WINE_NORETURN;
extern void wine_server_alloc_req( union generic_request *req, size_t size );
extern void wine_server_send_fd( int fd );
extern int wine_server_recv_fd( handle_t handle );
extern const char *get_config_dir(void);

/* do a server call and set the last error code */
inline static unsigned int __server_call_err( union generic_request *req, size_t size )
{
    unsigned int res = wine_server_call( req, size );
    if (res) SetLastError( RtlNtStatusToDosError(res) );
    return res;
}

/* get a pointer to the variable part of the request */
inline static void *server_data_ptr( const void *req )
{
    return (char *)NtCurrentTeb()->buffer + ((struct request_header *)req)->var_offset;
}

/* get the size of the variable part of the request */
inline static size_t server_data_size( const void *req )
{
    return ((struct request_header *)req)->var_size;
}


/* exception support for server calls */

extern DWORD __wine_server_exception_handler( PEXCEPTION_RECORD record, EXCEPTION_FRAME *frame,
                                              CONTEXT *context, EXCEPTION_FRAME **pdispatcher );

struct __server_exception_frame
{
    EXCEPTION_FRAME  frame;
    unsigned int     buffer_pos;  /* saved buffer position */
};


/* macros for server requests */

#define SERVER_START_REQ(type) \
    do { \
        union generic_request __req; \
        struct type##_request * const req = &__req.type; \
        __req.header.req = REQ_##type; \
        __req.header.var_size = 0; \
        do

#define SERVER_END_REQ \
        while(0); \
    } while(0)

#define SERVER_START_VAR_REQ(type,size) \
    do { \
        struct __server_exception_frame __f; \
        union generic_request __req; \
        struct type##_request * const req = &__req.type; \
        __f.frame.Handler = __wine_server_exception_handler; \
        __f.buffer_pos = NtCurrentTeb()->buffer_pos; \
        __wine_push_frame( &__f.frame ); \
        __req.header.req = REQ_##type; \
        wine_server_alloc_req( &__req, (size) ); \
        do

#define SERVER_END_VAR_REQ \
        while(0); \
        NtCurrentTeb()->buffer_pos = __f.buffer_pos; \
        __wine_pop_frame( &__f.frame ); \
    } while(0)

#define SERVER_CALL()      (wine_server_call( &__req, sizeof(*req) ))
#define SERVER_CALL_ERR()  (__server_call_err( &__req, sizeof(*req) ))


extern void CLIENT_InitServer(void);
extern void CLIENT_InitThread(void);
extern void CLIENT_BootDone( int debug_level );
extern int CLIENT_IsBootThread(void);

#endif  /* __WINE_WINE_SERVER_H */
