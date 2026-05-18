/*
* Copyright 2026 Alistair Leslie-Hughes
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
#include "winsock2.h"

#include "ndfapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ndfapi);

STDAPI NdfCloseIncident(NDFHANDLE handle)
{
    FIXME("%p\n", handle);

    return E_NOTIMPL;
}

STDAPI NdfCreateConnectivityIncident(NDFHANDLE *handle)
{
    FIXME("%p\n", handle);

    return E_NOTIMPL;
}

STDAPI NdfCreateWebIncident(LPCWSTR url, NDFHANDLE *handle)
{
    FIXME("%s, %p\n", debugstr_w(url), handle);

    return E_NOTIMPL;
}

STDAPI NdfCreateWinSockIncident(SOCKET sock, LPCWSTR host, USHORT port, LPCWSTR appId, SID *userId, NDFHANDLE *handle)
{
    FIXME("%Ix, %s, %s, %p, %p\n", sock, debugstr_w(host), debugstr_w(appId), userId, handle);

    return E_NOTIMPL;
}

STDAPI NdfDiagnoseIncident(NDFHANDLE handle, ULONG *count, RootCauseInfo **causes, DWORD wait, DWORD flags)
{
    FIXME("%p, %p, %p, %ld, %ld\n", handle, count, causes, wait, flags);

    return E_NOTIMPL;
}

STDAPI NdfExecuteDiagnosis(NDFHANDLE handle, HWND hwnd)
{
    FIXME("%p, %p\n", handle, hwnd);

    return E_NOTIMPL;
}
