/*
 * Copyright 2007 Mounir IDRASSI  (mounir.idrassi@idrix.fr, for IDRIX)
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

#include <stdarg.h>
#define WINSCARDAPI
#include "windef.h"
#include "winbase.h"
#include "winscard.h"
#include "winternl.h"

#include "wine/debug.h"
#include "wine/unixlib.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(winscard);

#define UNIX_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

static HANDLE g_startedEvent;

const SCARD_IO_REQUEST g_rgSCardT0Pci = { SCARD_PROTOCOL_T0, 8 };
const SCARD_IO_REQUEST g_rgSCardT1Pci = { SCARD_PROTOCOL_T1, 8 };
const SCARD_IO_REQUEST g_rgSCardRawPci = { SCARD_PROTOCOL_RAW, 8 };

static inline int utf16_to_utf8( const WCHAR *src, char **dst )
{
    int len = WideCharToMultiByte( CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL );
    if (dst)
    {
        if (!(*dst = malloc( len ))) return -1;
        WideCharToMultiByte( CP_UTF8, 0, src, -1, *dst, len, NULL, NULL );
    }
    return len;
}

static inline int ansi_to_utf16( const char *src, WCHAR **dst )
{
    int len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if (dst)
    {
        if (!(*dst = malloc( len * sizeof(WCHAR) ))) return -1;
        MultiByteToWideChar( CP_ACP, 0, src, -1, *dst, len );
    }
    return len;
}

static inline int ansi_to_utf8( const char *src, char **dst )
{
    WCHAR *tmp;
    int len;

    if (ansi_to_utf16( src, &tmp ) < 0) return -1;
    len = utf16_to_utf8( tmp, dst );
    free( tmp );
    return len;
}

static inline int utf8_to_utf16( const char *src, WCHAR **dst )
{
    int len = MultiByteToWideChar( CP_UTF8, 0, src, -1, NULL, 0 );
    if (dst)
    {
        if (!(*dst = malloc( len * sizeof(WCHAR) ))) return -1;
        MultiByteToWideChar( CP_UTF8, 0, src, -1, *dst, len );
    }
    return len;
}

static inline int utf16_to_ansi( const WCHAR *src, char **dst )
{
    int len = WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, src, -1, NULL, 0, NULL, NULL );
    if (*src && !len)
    {
        FIXME( "can't convert %s to ANSI codepage\n", debugstr_w(src) );
        return -1;
    }
    if (dst)
    {
        if (!(*dst = malloc( len ))) return -1;
        WideCharToMultiByte( CP_ACP, WC_NO_BEST_FIT_CHARS, src, -1, *dst, len, NULL, NULL );
    }
    return len;
}

static inline int utf8_to_ansi( const char *src, char **dst )
{
    WCHAR *tmp;
    int len;

    if (utf8_to_utf16( src, &tmp ) < 0) return -1;
    len = utf16_to_ansi( tmp, dst );
    free( tmp );
    return len;
}

HANDLE WINAPI SCardAccessStartedEvent(void)
{
    FIXME( "stub\n" );
    return g_startedEvent;
}

LONG WINAPI SCardAddReaderToGroupA( SCARDCONTEXT context, const char *reader, const char *group )
{
    WCHAR *readerW = NULL, *groupW = NULL;
    LONG ret;

    TRACE( "%Ix, %s, %s\n", context, debugstr_a(reader), debugstr_a(group) );

    if (reader && ansi_to_utf16( reader, &readerW ) < 0) return SCARD_E_NO_MEMORY;
    if (group && ansi_to_utf16( group, &groupW ) < 0)
    {
        free( readerW );
        return SCARD_E_NO_MEMORY;
    }
    ret = SCardAddReaderToGroupW( context, readerW, groupW );
    free( readerW );
    free( groupW );
    return ret;
}

LONG WINAPI SCardAddReaderToGroupW( SCARDCONTEXT context, const WCHAR *reader, const WCHAR *group )
{
    FIXME( "%Ix, %s, %s\n", context, debugstr_w(reader), debugstr_w(group) );
    return SCARD_S_SUCCESS;
}

#define CONTEXT_MAGIC (('C' << 24) | ('T' << 16) | ('X' << 8) | '0')
#define CONNECT_MAGIC (('C' << 24) | ('O' << 16) | ('N' << 8) | '0')
struct handle
{
    DWORD  magic;
    UINT64 unix_handle;
    DWORD  protocol;
};

LONG WINAPI SCardEstablishContext( DWORD scope, const void *reserved1, const void *reserved2, SCARDCONTEXT *context )
{
    struct scard_establish_context_params params;
    struct handle *handle;
    LONG ret;

    TRACE( "%#lx, %p, %p, %p\n", scope, reserved1, reserved2, context );

    if (!context) return SCARD_E_INVALID_PARAMETER;
    if (!(handle = malloc( sizeof(*handle) ))) return SCARD_E_NO_MEMORY;
    handle->magic = CONTEXT_MAGIC;

    params.scope = scope;
    params.handle = &handle->unix_handle;
    if (!(ret = UNIX_CALL( scard_establish_context, &params ))) *context = (SCARDCONTEXT)handle;
    else free( handle );

    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardIsValidContext( SCARDCONTEXT context )
{
    struct handle *handle = (struct handle *)context;
    struct scard_is_valid_context_params params;
    LONG ret;

    TRACE( "%Ix\n", context );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    ret = UNIX_CALL( scard_is_valid_context, &params );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardListCardsA( SCARDCONTEXT context, const BYTE *atr, const GUID *interfaces, DWORD interface_count,
                             char *cards, DWORD *cards_len )
{
    FIXME( "%Ix, %p, %p, %lu, %p, %p stub\n", context, atr, interfaces, interface_count, cards, cards_len );
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardReleaseContext( SCARDCONTEXT context )
{
    struct handle *handle = (struct handle *)context;
    struct scard_release_context_params params;
    LONG ret;

    TRACE( "%Ix\n", context );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    ret = UNIX_CALL( scard_release_context, &params );
    /* Ensure compiler doesn't optimize out the assignment with 0. */
    SecureZeroMemory( &handle->magic, sizeof(handle->magic) );
    free( handle );

    TRACE( "returning %#lx\n", ret );
    return ret;
}

static LONG copy_multiszA( const char *src, char *dst, DWORD *dst_len )
{
    int len, total_len = 0;
    const char *src_ptr = src;
    char *dst_ptr;

    if (!dst && !dst_len) return SCARD_S_SUCCESS;

    while (*src_ptr)
    {
        if ((len = utf8_to_ansi( src_ptr, NULL )) < 0) return SCARD_E_INVALID_PARAMETER;
        total_len += len;
        src_ptr += len;
    }
    total_len++; /* double null */

    if (*dst_len == SCARD_AUTOALLOCATE)
    {
        if (!(dst_ptr = malloc( total_len ))) return SCARD_E_NO_MEMORY;
    }
    else
    {
        if (dst && *dst_len < total_len)
        {
            *dst_len = total_len;
            return SCARD_E_INSUFFICIENT_BUFFER;
        }
        if (!dst)
        {
            *dst_len = total_len;
            return SCARD_S_SUCCESS;
        }
        dst_ptr = dst;
    }

    src_ptr = src;
    total_len = 0;
    while (*src_ptr)
    {
        char *str;
        if ((len = utf8_to_ansi( src_ptr, &str )) < 0)
        {
            if (dst_ptr != dst) free( dst_ptr );
            return SCARD_E_NO_MEMORY;
        }
        memcpy( dst_ptr + total_len, str, len );
        total_len += len;
        src_ptr += len;
        free( str );
    }

    dst_ptr[total_len] = 0;
    if (dst_ptr != dst) *(char **)dst = dst_ptr;
    *dst_len = ++total_len;
    return SCARD_S_SUCCESS;
}

LONG WINAPI SCardStatusA( SCARDHANDLE connect, char *names, DWORD *names_len, DWORD *state, DWORD *protocol,
                          BYTE *atr, DWORD *atr_len )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_status_params params;
    UINT64 state64, protocol64, atr_len64, names_len_utf8 = 0;
    LONG ret;

    TRACE( "%Ix, %p, %p, %p, %p, %p, %p\n", connect, names, names_len, state, protocol, atr, atr_len );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;
    if (atr_len && *atr_len == SCARD_AUTOALLOCATE)
    {
        FIXME( "SCARD_AUTOALLOCATE not supported for attr\n" );
        return SCARD_F_INTERNAL_ERROR;
    }

    params.handle = handle->unix_handle;
    params.names = NULL;
    params.names_len = &names_len_utf8;
    params.state = &state64;
    params.protocol = &protocol64;
    params.atr = atr;
    if (!atr_len) params.atr_len = NULL;
    else
    {
        atr_len64 = *atr_len;
        params.atr_len = &atr_len64;
    }
    if ((ret = UNIX_CALL( scard_status, &params ))) return ret;

    if (!(params.names = malloc( names_len_utf8 ))) return SCARD_E_NO_MEMORY;
    if (!(ret = UNIX_CALL( scard_status, &params )) && !(ret = copy_multiszA( params.names, names, names_len )))
    {
        handle->protocol = protocol64;
        if (state) *state = state64;
        if (protocol) *protocol = protocol64;
        if (atr_len) *atr_len = atr_len64;
    }

    free( params.names );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

static LONG copy_multiszW( const char *src, WCHAR *dst, DWORD *dst_len )
{
    int len, total_len = 0;
    const char *src_ptr = src;
    WCHAR *dst_ptr;

    if (!dst && !dst_len) return SCARD_S_SUCCESS;

    while (*src_ptr)
    {
        if ((len = utf8_to_utf16( src_ptr, NULL )) < 0) return SCARD_E_INVALID_PARAMETER;
        total_len += len;
        src_ptr += len;
    }
    total_len++; /* double null */

    if (*dst_len == SCARD_AUTOALLOCATE)
    {
        if (!(dst_ptr = malloc( total_len * sizeof(WCHAR) ))) return SCARD_E_NO_MEMORY;
    }
    else
    {
        if (dst && *dst_len < total_len)
        {
            *dst_len = total_len;
            return SCARD_E_INSUFFICIENT_BUFFER;
        }
        if (!dst)
        {
            *dst_len = total_len;
            return SCARD_S_SUCCESS;
        }
        dst_ptr = dst;
    }

    src_ptr = src;
    total_len = 0;
    while (*src_ptr)
    {
        WCHAR *str;
        if ((len = utf8_to_utf16( src_ptr, &str )) < 0)
        {
            if (dst_ptr != dst) free( dst_ptr );
            return SCARD_E_NO_MEMORY;
        }
        memcpy( dst_ptr + total_len, str, len * sizeof(WCHAR) );
        total_len += len;
        src_ptr += len;
        free( str );
    }

    dst_ptr[total_len] = 0;
    if (dst_ptr != dst) *(WCHAR **)dst = dst_ptr;
    *dst_len = ++total_len;
    return SCARD_S_SUCCESS;
}

LONG WINAPI SCardStatusW( SCARDHANDLE connect, WCHAR *names, DWORD *names_len, DWORD *state, DWORD *protocol,
                          BYTE *atr, DWORD *atr_len )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_status_params params;
    UINT64 state64, protocol64, atr_len64, names_len_utf8 = 0;
    LONG ret;

    TRACE( "%Ix, %p, %p, %p, %p, %p, %p\n", connect, names, names_len, state, protocol, atr, atr_len );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;
    if (atr_len && *atr_len == SCARD_AUTOALLOCATE)
    {
        FIXME( "SCARD_AUTOALLOCATE not supported for attr\n" );
        return SCARD_F_INTERNAL_ERROR;
    }

    params.handle = handle->unix_handle;
    params.names = NULL;
    params.names_len = &names_len_utf8;
    params.state = &state64;
    params.protocol = &protocol64;
    params.atr = atr;
    if (!atr_len) params.atr_len = NULL;
    else
    {
        atr_len64 = *atr_len;
        params.atr_len = &atr_len64;
    }
    if ((ret = UNIX_CALL( scard_status, &params ))) return ret;

    if (!(params.names = malloc( names_len_utf8 ))) return SCARD_E_NO_MEMORY;
    if (!(ret = UNIX_CALL( scard_status, &params )) && !(ret = copy_multiszW( params.names, names, names_len )))
    {
        handle->protocol = protocol64;
        if (state) *state = state64;
        if (protocol) *protocol = protocol64;
        if (atr_len) *atr_len = atr_len64;
    }

    free( params.names );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

void WINAPI SCardReleaseStartedEvent(void)
{
    FIXME( "stub\n" );
}

LONG WINAPI SCardListReadersA( SCARDCONTEXT context, const char *groups, char *readers, DWORD *readers_len )
{
    struct handle *handle = (struct handle *)context;
    struct scard_list_readers_params params;
    UINT64 readers_len_utf8;
    LONG ret;

    TRACE( "%Ix, %s, %p, %p\n", context, debugstr_a(groups), readers, readers_len );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!readers_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    if (!groups) params.groups = NULL;
    else if (ansi_to_utf8( groups, (char **)&params.groups ) < 0) return SCARD_E_NO_MEMORY;
    params.readers = NULL;
    params.readers_len = &readers_len_utf8;
    if ((ret = UNIX_CALL( scard_list_readers, &params ))) goto done;

    if (!(params.readers = malloc( readers_len_utf8 )))
    {
        free( (void *)params.groups );
        return SCARD_E_NO_MEMORY;
    }
    if (!(ret = UNIX_CALL( scard_list_readers, &params )))
    {
        ret = copy_multiszA( params.readers, readers, readers_len );
    }

done:
    free( (void *)params.groups );
    free( params.readers );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardListReadersW( SCARDCONTEXT context, const WCHAR *groups, WCHAR *readers, DWORD *readers_len )
{
    struct handle *handle = (struct handle *)context;
    struct scard_list_readers_params params;
    UINT64 readers_len_utf8;
    LONG ret;

    TRACE( "%Ix, %s, %p, %p\n", context, debugstr_w(groups), readers, readers_len );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!readers_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    if (!groups) params.groups = NULL;
    else if (utf16_to_utf8( groups, (char **)&params.groups ) < 0) return SCARD_E_NO_MEMORY;
    params.readers = NULL;
    params.readers_len = &readers_len_utf8;
    if ((ret = UNIX_CALL( scard_list_readers, &params ))) goto done;

    params.handle = handle->unix_handle;
    if (!(params.readers = malloc( readers_len_utf8 )))
    {
        free( (void *)params.groups );
        return SCARD_E_NO_MEMORY;
    }
    if (!(ret = UNIX_CALL( scard_list_readers, &params )))
    {
        ret = copy_multiszW( params.readers, readers, readers_len );
    }

done:
    free( (void *)params.groups );
    free( params.readers );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardCancel( SCARDCONTEXT context )
{
    struct handle *handle = (struct handle *)context;
    struct scard_cancel_params params;
    LONG ret;

    TRACE( "%Ix\n", context );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    ret = UNIX_CALL( scard_cancel, &params );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardListReaderGroupsA( SCARDCONTEXT context, char *groups, DWORD *groups_len )
{
    struct handle *handle = (struct handle *)context;
    struct scard_list_reader_groups_params params;
    UINT64 groups_len_utf8;
    LONG ret;

    TRACE( "%Ix, %p, %p\n", context, groups, groups_len );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!groups_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    params.groups = NULL;
    params.groups_len = &groups_len_utf8;
    if ((ret = UNIX_CALL( scard_list_reader_groups, &params ))) goto done;

    params.handle = handle->unix_handle;
    if (!(params.groups = malloc( groups_len_utf8 ))) return SCARD_E_NO_MEMORY;
    if (!(ret = UNIX_CALL( scard_list_reader_groups, &params )))
    {
        ret = copy_multiszA( params.groups, groups, groups_len );
    }

done:
    free( params.groups );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardListReaderGroupsW( SCARDCONTEXT context, WCHAR *groups, DWORD *groups_len )
{
    struct handle *handle = (struct handle *)context;
    struct scard_list_reader_groups_params params;
    UINT64 groups_len_utf8;
    LONG ret;

    TRACE( "%Ix, %p, %p\n", context, groups, groups_len );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!groups_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    params.groups = NULL;
    params.groups_len = &groups_len_utf8;
    if ((ret = UNIX_CALL( scard_list_reader_groups, &params ))) goto done;

    if (!(params.groups = malloc( groups_len_utf8 ))) return SCARD_E_NO_MEMORY;
    if (!(ret = UNIX_CALL( scard_list_reader_groups, &params )))
    {
        ret = copy_multiszW( params.groups, groups, groups_len );
    }

done:
    TRACE( "returning %#lx\n", ret );
    free( params.groups );
    return ret;
}

static LONG map_states_inA( const SCARD_READERSTATEA *src, struct reader_state *dst, DWORD count )
{
    DWORD i;
    memset( dst, 0, sizeof(*dst) * count );
    for (i = 0; i < count; i++)
    {
        if (src[i].szReader && ansi_to_utf8( src[i].szReader, (char **)&dst[i].reader ) < 0)
            return SCARD_E_NO_MEMORY;
    }
    return SCARD_S_SUCCESS;
}

static void map_states_out( const struct reader_state *src, SCARD_READERSTATEA *dst, DWORD count )
{
    DWORD i;
    for (i = 0; i < count; i++)
    {
        dst[i].dwCurrentState = src[i].current_state;
        dst[i].dwEventState = src[i].event_state;
        dst[i].cbAtr = src[i].atr_size;
        memcpy( dst[i].rgbAtr, src[i].atr, src[i].atr_size );
    }
}

static void free_states( struct reader_state *states, DWORD count )
{
    DWORD i;
    for (i = 0; i < count; i++) free( (void *)(ULONG_PTR)states[i].reader );
    free( states );
}

LONG WINAPI SCardGetStatusChangeA( SCARDCONTEXT context, DWORD timeout, SCARD_READERSTATEA *states, DWORD count )
{
    struct handle *handle = (struct handle *)context;
    struct scard_get_status_change_params params;
    struct reader_state *states_utf8 = NULL;
    LONG ret;

    TRACE( "%Ix, %lu, %p, %lu\n", context, timeout, states, count );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;

    if (!(states_utf8 = calloc( count, sizeof(*states_utf8) ))) return SCARD_E_NO_MEMORY;
    if ((ret = map_states_inA( states, states_utf8, count )))
    {
        free_states( states_utf8, count );
        return ret;
    }

    params.handle = handle->unix_handle;
    params.timeout = timeout;
    params.states = states_utf8;
    params.count = count;
    if (!(ret = UNIX_CALL( scard_get_status_change, &params )) && states)
    {
        map_states_out( states_utf8, states, count );
    }

    free_states( states_utf8, count );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

static LONG map_states_inW( SCARD_READERSTATEW *src, struct reader_state *dst, DWORD count )
{
    DWORD i;
    memset( dst, 0, sizeof(*dst) * count );
    for (i = 0; i < count; i++)
    {
        if (src[i].szReader && utf16_to_utf8( src[i].szReader, (char **)&dst[i].reader ) < 0)
            return SCARD_E_NO_MEMORY;
    }
    return SCARD_S_SUCCESS;
}

LONG WINAPI SCardGetStatusChangeW( SCARDCONTEXT context, DWORD timeout, SCARD_READERSTATEW *states, DWORD count )
{
    struct handle *handle = (struct handle *)context;
    struct scard_get_status_change_params params;
    struct reader_state *states_utf8;
    LONG ret;

    TRACE( "%Ix, %lu, %p, %lu\n", context, timeout, states, count );

    if (!handle || handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;

    if (!(states_utf8 = calloc( count, sizeof(*states_utf8) ))) return SCARD_E_NO_MEMORY;
    if ((ret = map_states_inW( states, states_utf8, count )))
    {
        free_states( states_utf8, count );
        return ret;
    }

    params.handle = handle->unix_handle;
    params.timeout = timeout;
    params.states = states_utf8;
    params.count = count;
    if (!(ret = UNIX_CALL( scard_get_status_change, &params )))
    {
        map_states_out( states_utf8, (SCARD_READERSTATEA *)states, count );
    }

    free_states( states_utf8, count );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardConnectA( SCARDCONTEXT context, const char *reader, DWORD share_mode, DWORD preferred_protocols,
                           SCARDHANDLE *connect, DWORD *protocol )
{
    struct handle *context_handle = (struct handle *)context, *connect_handle;
    struct scard_connect_params params;
    char *reader_utf8;
    UINT64 protocol64;
    LONG ret;

    TRACE( "%Ix, %s, %#lx, %#lx, %p, %p\n", context, debugstr_a(reader), share_mode, preferred_protocols, connect,
           protocol );

    if (!context_handle || context_handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!connect) return SCARD_E_INVALID_PARAMETER;

    if (!(connect_handle = malloc( sizeof(*connect_handle) ))) return SCARD_E_NO_MEMORY;
    connect_handle->magic = CONNECT_MAGIC;

    if (ansi_to_utf8( reader, &reader_utf8 ) < 0)
    {
        free( connect_handle );
        return SCARD_E_NO_MEMORY;
    }

    params.context_handle = context_handle->unix_handle;
    params.reader = reader_utf8;
    params.share_mode = share_mode;
    params.preferred_protocols = preferred_protocols;
    params.connect_handle = &connect_handle->unix_handle;
    params.protocol = &protocol64;
    if ((ret = UNIX_CALL( scard_connect, &params ))) free( connect_handle );
    else
    {
        connect_handle->protocol = protocol64;
        *connect = (SCARDHANDLE)connect_handle;
        if (protocol) *protocol = protocol64;
    }

    free( reader_utf8 );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardConnectW( SCARDCONTEXT context, const WCHAR *reader, DWORD share_mode, DWORD preferred_protocols,
                           SCARDHANDLE *connect, DWORD *protocol )
{
    struct handle *context_handle = (struct handle *)context, *connect_handle;
    struct scard_connect_params params;
    char *reader_utf8;
    UINT64 protocol64;
    LONG ret;

    TRACE( "%Ix, %s, %#lx, %#lx, %p, %p\n", context, debugstr_w(reader), share_mode, preferred_protocols, connect,
           protocol );

    if (!context_handle || context_handle->magic != CONTEXT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!connect) return SCARD_E_INVALID_PARAMETER;

    if (!(connect_handle = malloc( sizeof(*connect_handle) ))) return SCARD_E_NO_MEMORY;
    connect_handle->magic = CONNECT_MAGIC;

    if (utf16_to_utf8( reader, &reader_utf8 ) < 0)
    {
        free( connect_handle );
        return SCARD_E_NO_MEMORY;
    }

    params.context_handle = context_handle->unix_handle;
    params.reader = reader_utf8;
    params.share_mode = share_mode;
    params.preferred_protocols = preferred_protocols;
    params.connect_handle = &connect_handle->unix_handle;
    params.protocol = &protocol64;
    if ((ret = UNIX_CALL( scard_connect, &params ))) free( connect_handle );
    else
    {
        connect_handle->protocol = protocol64;
        *connect = (SCARDHANDLE)connect_handle;
        if (protocol) *protocol = protocol64;
    }

    free( reader_utf8 );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardReconnect( SCARDHANDLE connect, DWORD share_mode, DWORD preferred_protocols, DWORD initialization,
                            DWORD *protocol )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_reconnect_params params;
    UINT64 protocol64;
    LONG ret;

    TRACE( "%Ix, %#lx, %#lx, %#lx, %p\n", connect, share_mode, preferred_protocols, initialization, protocol );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    params.share_mode = share_mode;
    params.preferred_protocols = preferred_protocols;
    params.initialization = initialization;
    params.protocol = &protocol64;
    if (!(ret = UNIX_CALL( scard_reconnect, &params )))
    {
        handle->protocol = protocol64;
        if (protocol) *protocol = protocol64;
    }
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardDisconnect( SCARDHANDLE connect, DWORD disposition )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_disconnect_params params;
    LONG ret;

    TRACE( "%Ix, %#lx\n", connect, disposition );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    params.disposition = disposition;
    if (!(ret = UNIX_CALL( scard_disconnect, &params )))
    {
        /* Ensure compiler doesn't optimize out the assignment with 0. */
        SecureZeroMemory( &handle->magic, sizeof(handle->magic) );
        free( handle );
    }
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardBeginTransaction( SCARDHANDLE connect )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_begin_transaction_params params;
    LONG ret;

    TRACE( "%Ix\n", connect );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    ret = UNIX_CALL( scard_begin_transaction, &params );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardEndTransaction( SCARDHANDLE connect, DWORD disposition )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_end_transaction_params params;
    LONG ret;

    TRACE( "%Ix, %#lx\n", connect, disposition );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    params.disposition = disposition;
    ret = UNIX_CALL( scard_end_transaction, &params );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardTransmit( SCARDHANDLE connect, const SCARD_IO_REQUEST *send, const BYTE *send_buf,
                           DWORD send_buflen, SCARD_IO_REQUEST *recv, BYTE *recv_buf, DWORD *recv_buflen )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_transmit_params params;
    struct io_request send64, recv64;
    UINT64 recv_buflen64;
    LONG ret;

    TRACE( "%Ix, %p, %p, %lu, %p, %p, %p\n", connect, send, send_buf, send_buflen, recv, recv_buf, recv_buflen );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!recv_buflen) return SCARD_E_INVALID_PARAMETER;

    if (send)
    {
        send64.protocol = send->dwProtocol;
        send64.pci_len = send->cbPciLength;
    }
    else
    {
        send64.protocol = handle->protocol;
        send64.pci_len = sizeof(send64);
    }

    params.handle = handle->unix_handle;
    params.send = &send64;
    params.send_buf = send_buf;
    params.send_buflen = send_buflen;
    params.recv = &recv64;
    params.recv_buf = recv_buf;
    recv_buflen64 = *recv_buflen;
    params.recv_buflen = &recv_buflen64;
    if (!(ret = UNIX_CALL( scard_transmit, &params )))
    {
        if (recv)
        {
            recv->dwProtocol = recv64.protocol;
            recv->cbPciLength = recv64.pci_len;
        }
        *recv_buflen = recv_buflen64;
    }

    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardControl( SCARDHANDLE connect, DWORD code, const void *in_buf, DWORD in_buflen, void *out_buf,
                          DWORD out_buflen, DWORD *ret_len )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_control_params params;
    UINT64 ret_len64;
    LONG ret;

    TRACE( "%Ix, %#lx, %p, %lu, %p, %lu, %p\n", connect, code, in_buf, in_buflen, out_buf, out_buflen, ret_len );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!ret_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    params.code = code;
    params.in_buf = in_buf;
    params.in_buflen = in_buflen;
    params.out_buf = out_buf;
    params.out_buflen = out_buflen;
    params.ret_len = &ret_len64;
    if (!(ret = UNIX_CALL( scard_control, &params ))) *ret_len = ret_len64;
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardGetAttrib( SCARDHANDLE connect, DWORD id, BYTE *attr, DWORD *attr_len )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_get_attrib_params params;
    UINT64 attr_len64;
    LONG ret;

    TRACE( "%Ix, %#lx, %p, %p\n", connect, id, attr, attr_len );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;
    if (!attr_len) return SCARD_E_INVALID_PARAMETER;

    params.handle = handle->unix_handle;
    params.id = id;
    params.attr = attr;
    attr_len64 = *attr_len;
    params.attr_len = &attr_len64;
    if (!(ret = UNIX_CALL( scard_get_attrib, &params ))) *attr_len = attr_len64;
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardSetAttrib( SCARDHANDLE connect, DWORD id, const BYTE *attr, DWORD attr_len )
{
    struct handle *handle = (struct handle *)connect;
    struct scard_set_attrib_params params;
    LONG ret;

    TRACE( "%Ix, %#lx, %p, %lu\n", connect, id, attr, attr_len );

    if (!handle || handle->magic != CONNECT_MAGIC) return ERROR_INVALID_HANDLE;

    params.handle = handle->unix_handle;
    params.id = id;
    params.attr = attr;
    params.attr_len = attr_len;
    ret = UNIX_CALL( scard_set_attrib, &params );
    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardFreeMemory( SCARDCONTEXT context, const void *mem )
{
    TRACE( "%Ix, %p\n", context, mem );

    free( (void *)mem );
    return SCARD_S_SUCCESS;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        if (__wine_init_unix_call()) ERR( "no pcsclite support, expect problems\n" );

        /* FIXME: for now, we act as if the pcsc daemon is always started */
        g_startedEvent = CreateEventW( NULL, TRUE, TRUE, NULL );
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        CloseHandle( g_startedEvent );
        g_startedEvent = NULL;
        break;
    }

    return TRUE;
}
