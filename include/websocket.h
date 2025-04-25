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

#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_HANDLE(WEB_SOCKET_HANDLE);

typedef enum  _WEB_SOCKET_PROPERTY_TYPE
{
    WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE,
    WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE,
    WEB_SOCKET_DISABLE_MASKING_PROPERTY_TYPE,
    WEB_SOCKET_ALLOCATED_MASKING_PROPERTY_TYPE,
    WEB_SOCKET_DISABLE_UTF8_VERIFICATION_PROPERTY_TYPE,
    WEB_SOCKET_KEEPALIVE_INTERVAL_PROPERTY_TYPE,
    WEB_SOCKET_SUPPORTED_VERSIONS_PROPERTY_TYPE,
} WEB_SOCKET_PROPERTY_TYPE;

typedef struct _WEB_SOCKET_PROPERTY
{
    WEB_SOCKET_PROPERTY_TYPE Type;
    PVOID                    pvValue;
    ULONG                    ulValueSize;
} WEB_SOCKET_PROPERTY, *PWEB_SOCKET_PROPERTY;

VOID WINAPI WebSocketAbortHandle(WEB_SOCKET_HANDLE);
HRESULT WINAPI WebSocketCreateClientHandle(const PWEB_SOCKET_PROPERTY, ULONG, WEB_SOCKET_HANDLE*);
VOID WINAPI WebSocketDeleteHandle(WEB_SOCKET_HANDLE);

#ifdef __cplusplus
}
#endif

#endif /* __WEBSOCKET_H__ */
