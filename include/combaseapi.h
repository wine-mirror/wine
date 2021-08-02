/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include <rpc.h>
#include <rpcndr.h>

#ifndef _COMBASEAPI_H_
#define _COMBASEAPI_H_

#ifndef RC_INVOKED
#include <stdlib.h>
#endif

#include <objidlbase.h>
#include <guiddef.h>

#ifndef INITGUID
#include <cguid.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagServerInformation
{
    DWORD   dwServerPid;
    DWORD   dwServerTid;
    UINT64  ui64ServerAddress;
} ServerInformation, *PServerInformation;

HRESULT WINAPI CoDecodeProxy(DWORD client_pid, UINT64 proxy_addr, ServerInformation *server_info);

#ifdef __cplusplus
}
#endif

#endif /* _COMBASEAPI_H_ */
