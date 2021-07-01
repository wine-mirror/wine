/*
 * nsiproxy.sys
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

NTSTATUS nsi_enumerate_all_ex( struct nsi_enumerate_all_ex *params ) DECLSPEC_HIDDEN;
NTSTATUS nsi_get_all_parameters_ex( struct nsi_get_all_parameters_ex *params ) DECLSPEC_HIDDEN;
NTSTATUS nsi_get_parameter_ex( struct nsi_get_parameter_ex *params ) DECLSPEC_HIDDEN;

struct module_table
{
    DWORD table;
    DWORD sizes[4];
    NTSTATUS (*enumerate_all)( void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                               void *dynamic_data, DWORD dynamic_size,
                               void *static_data, DWORD static_size, DWORD_PTR *count );
    NTSTATUS (*get_all_parameters)( const void *key, DWORD key_size, void *rw_data, DWORD rw_size,
                                    void *dynamic_data, DWORD dynamic_size,
                                    void *static_data, DWORD static_size );
    NTSTATUS (*get_parameter)( const void *key, DWORD key_size, DWORD param_type,
                               void *data, DWORD data_size, DWORD data_offset );
};

struct module
{
    const NPI_MODULEID *module;
    const struct module_table *tables;
};

extern const struct module ndis_module DECLSPEC_HIDDEN;
