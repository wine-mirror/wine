/*
 * Network Store Interface
 *
 * Copyright 2021 Huw Davies
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
#include "winsock2.h"
#include "winternl.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "netioapi.h"
#include "iptypes.h"
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 )
{
    FIXME( "%d %p %d %p %d %p %d %p %d %p %d %p %d: stub\n", unk, module, table, key_data, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size, count, unk2 );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data )
{
    FIXME( "%p %p %p %p: stub\n", key_data, rw_data, dynamic_data, static_data );
}
