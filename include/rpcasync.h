/*
 * Copyright (C) 2007 Francois Gouget
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
#ifndef __WINE_RPCASYNC_H
#define __WINE_RPCASYNC_H


typedef struct tagRPC_ERROR_ENUM_HANDLE
{
    ULONG Signature;
    void* CurrentPos;
    void* Head;
} RPC_ERROR_ENUM_HANDLE;


#ifdef __cplusplus
extern "C" {
#endif

RPCRTAPI RPC_STATUS RPC_ENTRY RpcErrorStartEnumeration(RPC_ERROR_ENUM_HANDLE*);

#ifdef __cplusplus
}
#endif

#endif
