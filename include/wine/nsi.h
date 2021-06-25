/*
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

#ifndef __WINE_NSI_H
#define __WINE_NSI_H

/* Undocumented Nsi api */
DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 );
void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data );

#endif /* __WINE_NSI_H */
