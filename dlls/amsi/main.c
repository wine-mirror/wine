/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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
#include "amsi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(amsi);

HRESULT WINAPI AmsiInitialize( const WCHAR *appname, HAMSICONTEXT *context )
{
    FIXME( "%s, %p\n", debugstr_w(appname), context );
    *context = (HAMSICONTEXT)0xdeadbeef;
    return S_OK;
}

HRESULT WINAPI AmsiOpenSession( HAMSICONTEXT context, HAMSISESSION *session )
{
    FIXME( "%p, %p\n", context, session );
    *session = (HAMSISESSION)0xdeadbeef;
    return S_OK;
}

void WINAPI AmsiCloseSession( HAMSICONTEXT context, HAMSISESSION session )
{
    FIXME( "%p, %p\n", context, session );
}

HRESULT WINAPI AmsiScanBuffer( HAMSICONTEXT context, void *buffer, ULONG length, const WCHAR *name,
                               HAMSISESSION session, AMSI_RESULT *result )
{
    FIXME( "%p, %p, %lu, %s, %p, %p\n", context, buffer, length, debugstr_w(name), session, result );
    *result = AMSI_RESULT_NOT_DETECTED;
    return S_OK;
}

HRESULT WINAPI AmsiScanString( HAMSICONTEXT context, const WCHAR *string, const WCHAR *name,
                               HAMSISESSION session, AMSI_RESULT *result )
{
    FIXME( "%p, %s, %s, %p, %p\n", context, debugstr_w(string), debugstr_w(name), session, result );
    *result = AMSI_RESULT_NOT_DETECTED;
    return S_OK;
}

void WINAPI AmsiUninitialize( HAMSICONTEXT context )
{
    FIXME( "%p\n", context );
}
