/*
 * Copyright 2007 Misha Koshelev
 * Copyright 2009 Jacek Caban for CodeWeavers
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

#include "urlmon_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(urlmon);

HRESULT protocol_lock_request(Protocol *protocol)
{
    if (!InternetLockRequestFile(protocol->request, &protocol->lock))
        WARN("InternetLockRequest failed: %d\n", GetLastError());

    return S_OK;
}

HRESULT protocol_unlock_request(Protocol *protocol)
{
    if(!protocol->lock)
        return S_OK;

    if(!InternetUnlockRequestFile(protocol->lock))
        WARN("InternetUnlockRequest failed: %d\n", GetLastError());
    protocol->lock = 0;

    return S_OK;
}

void protocol_close_connection(Protocol *protocol)
{
    protocol->vtbl->close_connection(protocol);

    if(protocol->request)
        InternetCloseHandle(protocol->request);
    if(protocol->internet) {
        InternetCloseHandle(protocol->internet);
        protocol->internet = 0;
    }

    protocol->flags = 0;
}
