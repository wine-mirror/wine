/*
 * nsiproxy.sys unixlib private header
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

static inline NTSTATUS nsi_enumerate_all( UINT unk, UINT unk2, const NPI_MODULEID *module, UINT table,
                                          void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                                          void *dynamic_data, UINT dynamic_size, void *static_data, UINT static_size,
                                          UINT *count )
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

static inline BOOL convert_luid_to_index( const NET_LUID *luid, UINT *index )
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

static inline BOOL convert_index_to_luid( UINT index, NET_LUID *luid )
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

struct ipv6_addr_scope
{
    IN6_ADDR addr;
    UINT scope;
};

struct ipv6_addr_scope *get_ipv6_addr_scope_table( unsigned int *size ) DECLSPEC_HIDDEN;
UINT find_ipv6_addr_scope( const IN6_ADDR *addr, const struct ipv6_addr_scope *table, unsigned int size ) DECLSPEC_HIDDEN;

struct pid_map
{
    unsigned int pid;
    unsigned int unix_pid;
};

struct pid_map *get_pid_map( unsigned int *num_entries ) DECLSPEC_HIDDEN;
unsigned int find_owning_pid( struct pid_map *map, unsigned int num_entries, UINT_PTR inode ) DECLSPEC_HIDDEN;

struct module_table
{
    UINT table;
    UINT sizes[4];
    NTSTATUS (*enumerate_all)( void *key_data, UINT key_size, void *rw_data, UINT rw_size,
                               void *dynamic_data, UINT dynamic_size,
                               void *static_data, UINT static_size, UINT_PTR *count );
    NTSTATUS (*get_all_parameters)( const void *key, UINT key_size, void *rw_data, UINT rw_size,
                                    void *dynamic_data, UINT dynamic_size,
                                    void *static_data, UINT static_size );
    NTSTATUS (*get_parameter)( const void *key, UINT key_size, UINT param_type,
                               void *data, UINT data_size, UINT data_offset );
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
extern const struct module udp_module DECLSPEC_HIDDEN;

static inline int ascii_strncasecmp( const char *s1, const char *s2, size_t n )
{
    int l1, l2;

    while (n--)
    {
        l1 = (unsigned char)((*s1 >= 'A' && *s1 <= 'Z') ? *s1 + ('a' - 'A') : *s1);
        l2 = (unsigned char)((*s2 >= 'A' && *s2 <= 'Z') ? *s2 + ('a' - 'A') : *s2);
        if (l1 != l2) return l1 - l2;
        if (!l1) return 0;
        s1++;
        s2++;
    }
    return 0;
}

static inline int ascii_strcasecmp( const char *s1, const char *s2 )
{
    return ascii_strncasecmp( s1, s2, -1 );
}

NTSTATUS icmp_cancel_listen( void *args ) DECLSPEC_HIDDEN;
NTSTATUS icmp_close( void *args ) DECLSPEC_HIDDEN;
NTSTATUS icmp_listen( void *args ) DECLSPEC_HIDDEN;
NTSTATUS icmp_send_echo( void *args ) DECLSPEC_HIDDEN;
