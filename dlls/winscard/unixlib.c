/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"

#include "wine/debug.h"
#include "wine/unixlib.h"
#include "unixlib.h"

LONG SCardEstablishContext( UINT64, const void *, const void *, UINT64 * );
LONG SCardReleaseContext( UINT64 );
LONG SCardIsValidContext( UINT64 );
LONG SCardGetStatusChange( UINT64, UINT64, struct reader_state *, UINT64 );
LONG SCardCancel( UINT64 );
LONG SCardListReaders( UINT64, const char *, char *, UINT64 * );
LONG SCardListReaderGroups( UINT64, char *, UINT64 * );
LONG SCardConnect( UINT64, const char *, UINT64, UINT64, UINT64 *, UINT64 * );
LONG SCardStatus( UINT64, char *, UINT64 *, UINT64 *, UINT64 *, BYTE *, UINT64 * );
LONG SCardDisconnect( UINT64, UINT64 );
LONG SCardReconnect( UINT64, UINT64, UINT64, UINT64, UINT64 * );
LONG SCardBeginTransaction( UINT64 );
LONG SCardEndTransaction( UINT64, UINT64 );
LONG SCardTransmit( UINT64, const struct io_request *, const BYTE *, UINT64, struct io_request *, BYTE *, UINT64 * );
LONG SCardControl( UINT64, UINT64, const void *, UINT64, void *, UINT64, UINT64 * );
LONG SCardGetAttrib( UINT64, UINT64, BYTE *, UINT64 * );
LONG SCardSetAttrib( UINT64, UINT64, const BYTE *, UINT64 );

static NTSTATUS scard_establish_context( void *args )
{
    struct scard_establish_context_params *params = args;
    return SCardEstablishContext( params->scope, NULL, NULL, params->handle );
}

static NTSTATUS scard_release_context( void *args )
{
    struct scard_release_context_params *params = args;
    return SCardReleaseContext( params->handle );
}

static NTSTATUS scard_is_valid_context( void *args )
{
    struct scard_is_valid_context_params *params = args;
    return SCardIsValidContext( params->handle );
}

static NTSTATUS scard_get_status_change( void *args )
{
    struct scard_get_status_change_params *params = args;
    return SCardGetStatusChange( params->handle, params->timeout, params->states, params->count );
}

static NTSTATUS scard_cancel( void *args )
{
    struct scard_cancel_params *params = args;
    return SCardCancel( params->handle );
}

static NTSTATUS scard_list_readers( void *args )
{
    struct scard_list_readers_params *params = args;
    return SCardListReaders( params->handle, params->groups, params->readers, params->readers_len );
}

static NTSTATUS scard_list_reader_groups( void *args )
{
    struct scard_list_reader_groups_params *params = args;
    return SCardListReaderGroups( params->handle, params->groups, params->groups_len );
}

static NTSTATUS scard_connect( void *args )
{
    struct scard_connect_params *params = args;
    return SCardConnect( params->context_handle, params->reader, params->share_mode, params->preferred_protocols,
                         params->connect_handle, params->protocol );
}

static NTSTATUS scard_status( void *args )
{
    struct scard_status_params *params = args;
    return SCardStatus( params->handle, params->names, params->names_len, params->state, params->protocol,
                        params->atr, params->atr_len );
}

static NTSTATUS scard_reconnect( void *args )
{
    struct scard_reconnect_params *params = args;
    return SCardReconnect( params->handle, params->share_mode, params->preferred_protocols,
                            params->initialization, params->protocol );
}

static NTSTATUS scard_disconnect( void *args )
{
    struct scard_disconnect_params *params = args;
    return SCardDisconnect( params->handle, params->disposition );
}

static NTSTATUS scard_begin_transaction( void *args )
{
    struct scard_begin_transaction_params *params = args;
    return SCardBeginTransaction( params->handle );
}

static NTSTATUS scard_end_transaction( void *args )
{
    struct scard_end_transaction_params *params = args;
    return SCardEndTransaction( params->handle, params->disposition );
}

static NTSTATUS scard_transmit( void *args )
{
    struct scard_transmit_params *params = args;
    return SCardTransmit( params->handle, params->send, params->send_buf, params->send_buflen, params->recv,
                          params->recv_buf, params->recv_buflen );
}

static NTSTATUS scard_control( void *args )
{
    struct scard_control_params *params = args;
    return SCardControl( params->handle, params->code, params->in_buf, params->in_buflen, params->out_buf,
                         params->out_buflen, params->ret_len );
}

static NTSTATUS scard_get_attrib( void *args )
{
    struct scard_get_attrib_params *params = args;
    return SCardGetAttrib( params->handle, params->id, params->attr, params->attr_len );
}

static NTSTATUS scard_set_attrib( void *args )
{
    struct scard_set_attrib_params *params = args;
    return SCardSetAttrib( params->handle, params->id, params->attr, params->attr_len );
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    scard_establish_context,
    scard_release_context,
    scard_is_valid_context,
    scard_get_status_change,
    scard_cancel,
    scard_list_readers,
    scard_list_reader_groups,
    scard_connect,
    scard_status,
    scard_reconnect,
    scard_disconnect,
    scard_begin_transaction,
    scard_end_transaction,
    scard_transmit,
    scard_control,
    scard_get_attrib,
    scard_set_attrib,
};

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_scard_establish_context( void *args )
{
    struct
    {
        UINT64 scope;
        PTR32 handle;
    } const *params32 = args;

    struct scard_establish_context_params params =
    {
        params32->scope,
        ULongToPtr(params32->handle)
    };
    return scard_establish_context( &params );
}

static NTSTATUS wow64_scard_release_context( void *args )
{
    struct
    {
        UINT64 handle;
    } const *params32 = args;

    struct scard_release_context_params params =
    {
        params32->handle
    };
    return scard_release_context( &params );
}

static NTSTATUS wow64_scard_is_valid_context( void *args )
{
    struct
    {
        UINT64 handle;
    } const *params32 = args;

    struct scard_is_valid_context_params params =
    {
        params32->handle
    };
    return scard_is_valid_context( &params );
}

static NTSTATUS wow64_scard_get_status_change( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 timeout;
        PTR32 states;
        UINT64 count;
    } const *params32 = args;

    struct scard_get_status_change_params params =
    {
        params32->handle,
        params32->timeout,
        ULongToPtr(params32->states),
        params32->count,
    };
    return scard_get_status_change( &params );
}

static NTSTATUS wow64_scard_cancel( void *args )
{
    struct
    {
        UINT64 handle;
    } const *params32 = args;

    struct scard_cancel_params params =
    {
        params32->handle
    };
    return scard_cancel( &params );
}

static NTSTATUS wow64_scard_list_readers( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 groups;
        PTR32 readers;
        PTR32 readers_len;
    } *params32 = args;

    struct scard_list_readers_params params =
    {
        params32->handle,
        ULongToPtr(params32->groups),
        ULongToPtr(params32->readers),
        ULongToPtr(params32->readers_len)
    };
    return scard_list_readers( &params );
};

static NTSTATUS wow64_scard_list_reader_groups( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 groups;
        PTR32 groups_len;
    } *params32 = args;

    struct scard_list_reader_groups_params params =
    {
        params32->handle,
        ULongToPtr(params32->groups),
        ULongToPtr(params32->groups_len)
    };
    return scard_list_reader_groups( &params );
}

static NTSTATUS wow64_scard_connect( void *args )
{
    struct
    {
        UINT64 context_handle;
        PTR32 reader;
        UINT64 share_mode;
        UINT64 preferred_protocols;
        PTR32 connect_handle;
        PTR32 protocol;
    } *params32 = args;

    struct scard_connect_params params =
    {
        params32->context_handle,
        ULongToPtr(params32->reader),
        params32->share_mode,
        params32->preferred_protocols,
        ULongToPtr(params32->connect_handle),
        ULongToPtr(params32->protocol)
    };
    return scard_connect( &params );
}

static NTSTATUS wow64_scard_status( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 names;
        PTR32 names_len;
        PTR32 state;
        PTR32 protocol;
        PTR32 atr;
        PTR32 atr_len;
    } *params32 = args;

    struct scard_status_params params =
    {
        params32->handle,
        ULongToPtr(params32->names),
        ULongToPtr(params32->names_len),
        ULongToPtr(params32->state),
        ULongToPtr(params32->protocol),
        ULongToPtr(params32->atr),
        ULongToPtr(params32->atr_len)
    };
    return scard_status( &params );
};

static NTSTATUS wow64_scard_reconnect( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 share_mode;
        UINT64 preferred_protocols;
        UINT64 initialization;
        PTR32 protocol;
    } *params32 = args;

    struct scard_reconnect_params params =
    {
        params32->handle,
        params32->share_mode,
        params32->preferred_protocols,
        params32->initialization,
        ULongToPtr(params32->protocol)
    };
    return scard_reconnect( &params );
}

static NTSTATUS wow64_scard_disconnect( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 disposition;
    } *params32 = args;

    struct scard_disconnect_params params =
    {
        params32->handle,
        params32->disposition
    };
    return scard_disconnect( &params );
}

static NTSTATUS wow64_scard_begin_transaction( void *args )
{
    struct
    {
        UINT64 handle;
    } *params32 = args;

    struct scard_begin_transaction_params params =
    {
        params32->handle
    };
    return scard_begin_transaction( &params );
}

static NTSTATUS wow64_scard_end_transaction( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 disposition;
    } *params32 = args;

    struct scard_end_transaction_params params =
    {
        params32->handle,
        params32->disposition
    };
    return scard_end_transaction( &params );
}

static NTSTATUS wow64_scard_transmit( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 send;
        PTR32 send_buf;
        UINT64 send_buflen;
        PTR32 recv;
        PTR32 recv_buf;
        PTR32 recv_buflen;
    } *params32 = args;

    struct scard_transmit_params params =
    {
        params32->handle,
        ULongToPtr(params32->send),
        ULongToPtr(params32->send_buf),
        params32->send_buflen,
        ULongToPtr(params32->recv),
        ULongToPtr(params32->recv_buf),
        ULongToPtr(params32->recv_buflen)
    };
    return scard_transmit( &params );
};

static NTSTATUS wow64_scard_control( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 code;
        PTR32 in_buf;
        UINT64 in_buflen;
        PTR32 out_buf;
        UINT64 out_buflen;
        PTR32 ret_len;
    } *params32 = args;

    struct scard_control_params params =
    {
        params32->handle,
        params32->code,
        ULongToPtr(params32->in_buf),
        params32->in_buflen,
        ULongToPtr(params32->out_buf),
        params32->out_buflen,
        ULongToPtr(params32->ret_len)
    };
    return scard_control( &params );
};

static NTSTATUS wow64_scard_get_attrib( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 id;
        PTR32 attr;
        PTR32 attr_len;
    } *params32 = args;

    struct scard_get_attrib_params params =
    {
        params32->handle,
        params32->id,
        ULongToPtr(params32->attr),
        ULongToPtr(params32->attr_len)
    };
    return scard_get_attrib( &params );
}

static NTSTATUS wow64_scard_set_attrib( void *args )
{
    struct
    {
        UINT64 handle;
        UINT64 id;
        PTR32 attr;
        UINT64 attr_len;
    } *params32 = args;

    struct scard_set_attrib_params params =
    {
        params32->handle,
        params32->id,
        ULongToPtr(params32->attr),
        params32->attr_len
    };
    return scard_set_attrib( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    wow64_scard_establish_context,
    wow64_scard_release_context,
    wow64_scard_is_valid_context,
    wow64_scard_get_status_change,
    wow64_scard_cancel,
    wow64_scard_list_readers,
    wow64_scard_list_reader_groups,
    wow64_scard_connect,
    wow64_scard_status,
    wow64_scard_reconnect,
    wow64_scard_disconnect,
    wow64_scard_begin_transaction,
    wow64_scard_end_transaction,
    wow64_scard_transmit,
    wow64_scard_control,
    wow64_scard_get_attrib,
    wow64_scard_set_attrib,
};

#endif  /* _WIN64 */
