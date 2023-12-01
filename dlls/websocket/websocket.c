/*
 * Copyright 2023 Vijay Kiran Kamuju
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

#include "wine/debug.h"

#include "websocket.h"

WINE_DEFAULT_DEBUG_CHANNEL(websocket);

HRESULT WINAPI WebSocketCreateClientHandle(const PWEB_SOCKET_PROPERTY properties, 
    ULONG count, WEB_SOCKET_HANDLE *handle)
{
    FIXME("(%p, %ld, %p): stub\n", properties, count, handle);

    return E_NOTIMPL;
}

VOID WINAPI WebSocketAbortHandle(WEB_SOCKET_HANDLE handle)
{
    FIXME("(%p): stub\n", handle);
}

VOID WINAPI WebSocketDeleteHandle(WEB_SOCKET_HANDLE handle)
{
    FIXME("(%p): stub\n", handle);
}
