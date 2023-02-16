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

HANDLE WINAPI SCardAccessStartedEvent(void)
{
    return g_startedEvent;
}

LONG WINAPI SCardAddReaderToGroupA(SCARDCONTEXT context, LPCSTR reader, LPCSTR group)
{
    LONG retval;
    UNICODE_STRING readerW, groupW;

    if(reader) RtlCreateUnicodeStringFromAsciiz(&readerW,reader);
    else readerW.Buffer = NULL;
    if(group) RtlCreateUnicodeStringFromAsciiz(&groupW,group);
    else groupW.Buffer = NULL;

    retval = SCardAddReaderToGroupW(context, readerW.Buffer, groupW.Buffer);

    RtlFreeUnicodeString(&readerW);
    RtlFreeUnicodeString(&groupW);

    return retval;
}

LONG WINAPI SCardAddReaderToGroupW(SCARDCONTEXT context, LPCWSTR reader, LPCWSTR group)
{
    FIXME("%x %s %s\n", (unsigned int) context, debugstr_w(reader), debugstr_w(group));
    return SCARD_S_SUCCESS;
}

#define CONTEXT_MAGIC (('C' << 24) | ('T' << 16) | ('X' << 8) | '0')
struct handle
{
    DWORD  magic;
    UINT64 unix_handle;
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

LONG WINAPI SCardListCardsA(SCARDCONTEXT hContext, LPCBYTE pbAtr, LPCGUID rgguidInterfaces, DWORD cguidInterfaceCount, LPSTR mszCards, LPDWORD pcchCards)
{
    FIXME(": stub\n");
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
    handle->magic = 0;
    free( handle );

    TRACE( "returning %#lx\n", ret );
    return ret;
}

LONG WINAPI SCardStatusA(SCARDHANDLE context, LPSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState, LPDWORD pdwProtocol, LPBYTE pbAtr, LPDWORD pcbAtrLen)
{
    FIXME("(%Ix) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

LONG WINAPI SCardStatusW(SCARDHANDLE context, LPWSTR szReaderName, LPDWORD pcchReaderLen, LPDWORD pdwState,LPDWORD pdwProtocol,LPBYTE pbAtr,LPDWORD pcbArtLen)
{
    FIXME("(%Ix) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
}

void WINAPI SCardReleaseStartedEvent(void)
{
    FIXME("stub\n");
}

LONG WINAPI SCardListReadersA(SCARDCONTEXT context, const CHAR *groups, CHAR *readers, DWORD *buflen)
{
    FIXME("(%Ix, %s, %p, %p) stub\n", context, debugstr_a(groups), readers, buflen);
    return SCARD_E_NO_READERS_AVAILABLE;
}

LONG WINAPI SCardListReadersW(SCARDCONTEXT context, const WCHAR *groups, WCHAR *readers, DWORD *buflen)
{
    FIXME("(%Ix, %s, %p, %p) stub\n", context, debugstr_w(groups), readers, buflen);
    return SCARD_E_NO_READERS_AVAILABLE;
}

LONG WINAPI SCardCancel(SCARDCONTEXT context)
{
    FIXME("(%Ix) stub\n", context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return SCARD_F_INTERNAL_ERROR;
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
