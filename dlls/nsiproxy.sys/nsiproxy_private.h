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

static inline NTSTATUS nsi_enumerate_all( DWORD unk, DWORD unk2, const NPI_MODULEID *module, DWORD table,
                                          void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                          void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size,
                                          DWORD *count )
{
    struct nsi_enumerate_all_ex params;
    NTSTATUS status;

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

    status = nsi_enumerate_all_ex( &params );
    *count = params.count;
    return status;
}

BOOL convert_luid_to_unix_name( const NET_LUID *luid, const char **unix_name ) DECLSPEC_HIDDEN;
BOOL convert_unix_name_to_luid( const char *unix_name, NET_LUID *luid ) DECLSPEC_HIDDEN;

static inline BOOL convert_luid_to_index( const NET_LUID *luid, DWORD *index )
{
    struct nsi_get_parameter_ex params;
    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.first_arg = 1;
    params.unknown2 = 0;
    params.module = &NPI_MS_NDIS_MODULEID;
    params.table = NSI_NDIS_IFINFO_TABLE;
    params.key = luid;
    params.key_size = sizeof(*luid);
    params.param_type = NSI_PARAM_TYPE_STATIC;
    params.data = index;
    params.data_size = sizeof(*index);
    params.data_offset = FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_index);

    return !nsi_get_parameter_ex( &params );
}

static inline BOOL convert_index_to_luid( DWORD index, NET_LUID *luid )
{
    struct nsi_get_parameter_ex params;
    params.unknown[0] = 0;
    params.unknown[1] = 0;
    params.first_arg = 1;
    params.unknown2 = 0;
    params.module = &NPI_MS_NDIS_MODULEID;
    params.table = NSI_NDIS_INDEX_LUID_TABLE;
    params.key = &index;
    params.key_size = sizeof(index);
    params.param_type = NSI_PARAM_TYPE_STATIC;
    params.data = luid;
    params.data_size = sizeof(*luid);
    params.data_offset = 0;

    return !nsi_get_parameter_ex( &params );
}

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
extern const struct module ipv4_module DECLSPEC_HIDDEN;
extern const struct module ipv6_module DECLSPEC_HIDDEN;
extern const struct module tcp_module DECLSPEC_HIDDEN;
