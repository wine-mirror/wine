/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xolehlp);

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %u, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
    }
    return TRUE;
}

HRESULT WINAPI DtcGetTransactionManager(char *host, char *tm_name, REFIID riid,
        DWORD dwReserved1, WORD wcbReserved2, void *pvReserved2, void **ppv)
{
    FIXME("(%s, %s, %s, %d, %d, %p, %p): stub\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), dwReserved1, wcbReserved2, pvReserved2, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI DtcGetTransactionManagerExA(CHAR *host, CHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    FIXME("(%s, %s, %s, %d, %p, %p): stub\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), options, config, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI DtcGetTransactionManagerExW(WCHAR *host, WCHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    FIXME("(%s, %s, %s, %d, %p, %p): stub\n", debugstr_w(host), debugstr_w(tm_name),
            debugstr_guid(riid), options, config, ppv);
    return E_NOTIMPL;
}
