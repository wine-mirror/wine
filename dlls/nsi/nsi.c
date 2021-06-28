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

DWORD WINAPI NsiEnumerateObjectsAllParameters( DWORD unk, DWORD unk2, const NPI_MODULEID *module, DWORD table,
                                               void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                               void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size,
                                               DWORD *count )
{
    struct nsi_enumerate_all_ex params;
    DWORD err;

    FIXME( "%d %d %p %d %p %d %p %d %p %d %p %d %p: stub\n", unk, unk2, module, table, key_data, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size, count );

    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.module = module;
    params.table = table;
    params.first_arg = unk;
    params.second_arg = unk2;
    params.key_data = key_data;
    params.key_size = key_size;
    params.rw_data = rw_data;
    params.rw_size = rw_size;
    params.dynamic_data = dynamic_data;
    params.dynamic_size = dynamic_size;
    params.static_data = static_data;
    params.static_size = static_size;
    params.count = *count;

    err = NsiEnumerateObjectsAllParametersEx( &params );
    *count = params.count;
    return err;
}

DWORD WINAPI NsiEnumerateObjectsAllParametersEx( struct nsi_enumerate_all_ex *params )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data )
{
    FIXME( "%p %p %p %p: stub\n", key_data, rw_data, dynamic_data, static_data );
}

DWORD WINAPI NsiGetAllParameters( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                                  void *rw_data, DWORD rw_size, void *dynamic_data, DWORD dynamic_size,
                                  void *static_data, DWORD static_size )
{
    struct nsi_get_all_parameters_ex params;

    FIXME( "%d %p %d %p %d %p %d %p %d %p %d: stub\n", unk, module, table, key, key_size,
           rw_data, rw_size, dynamic_data, dynamic_size, static_data, static_size );

    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.module = module;
    params.table = table;
    params.first_arg = unk;
    params.unknown2 = 0;
    params.key = key;
    params.key_size = key_size;
    params.rw_data = rw_data;
    params.rw_size = rw_size;
    params.dynamic_data = dynamic_data;
    params.dynamic_size = dynamic_size;
    params.static_data = static_data;
    params.static_size = static_size;

    return NsiGetAllParametersEx( &params );
}

DWORD WINAPI NsiGetAllParametersEx( struct nsi_get_all_parameters_ex *params )
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI NsiGetParameter( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                              DWORD param_type, void *data, DWORD data_size, DWORD data_offset )
{
    FIXME( "%d %p %d %p %d %d %p %d %d: stub\n", unk, module, table, key, key_size,
           param_type, data, data_size, data_offset );
    return ERROR_CALL_NOT_IMPLEMENTED;
}
