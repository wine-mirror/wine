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
