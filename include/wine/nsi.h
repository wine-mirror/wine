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

#include "inaddr.h"
#include "in6addr.h"
#include "ws2def.h"
#include "ws2ipdef.h"
#include "winioctl.h"

/* Undocumented NSI NDIS tables */
#define NSI_NDIS_IFINFO_TABLE              0
#define NSI_NDIS_INDEX_LUID_TABLE          2

struct nsi_ndis_ifinfo_rw
{
    GUID network_guid;
    DWORD admin_status;
    IF_COUNTED_STRING alias; /* .Length in bytes not including '\0' */
    IF_PHYSICAL_ADDRESS phys_addr;
    USHORT pad;
    IF_COUNTED_STRING name2;
    DWORD unk;
};

struct nsi_ndis_ifinfo_dynamic
{
    DWORD oper_status;
    struct
    {
        DWORD unk : 1;
        DWORD not_media_conn : 1;
        DWORD unk2 : 30;
    } flags;
    DWORD media_conn_state;
    DWORD unk;
    DWORD mtu;
    ULONG64 xmit_speed;
    ULONG64 rcv_speed;
    ULONG64 in_errors;
    ULONG64 in_discards;
    ULONG64 out_errors;
    ULONG64 out_discards;
    ULONG64 unk2;
    ULONG64 in_octets;
    ULONG64 in_ucast_pkts;
    ULONG64 in_mcast_pkts;
    ULONG64 in_bcast_pkts;
    ULONG64 out_octets;
    ULONG64 out_ucast_pkts;
    ULONG64 out_mcast_pkts;
    ULONG64 out_bcast_pkts;
    ULONG64 unk3[2];
    ULONG64 in_ucast_octs;
    ULONG64 in_mcast_octs;
    ULONG64 in_bcast_octs;
    ULONG64 out_ucast_octs;
    ULONG64 out_mcast_octs;
    ULONG64 out_bcast_octs;
    ULONG64 unk4;
};

struct nsi_ndis_ifinfo_static
{
    DWORD if_index;
    IF_COUNTED_STRING descr;
    DWORD type;
    DWORD access_type;
    DWORD unk;
    DWORD conn_type;
    GUID if_guid;
    USHORT conn_present;
    IF_PHYSICAL_ADDRESS perm_phys_addr;
    struct
    {
        DWORD hw : 1;
        DWORD filter : 1;
        DWORD unk : 30;
    } flags;
    DWORD media_type;
    DWORD phys_medium_type;
};

/* Undocumented NSI IP tables */
#define NSI_IP_COMPARTMENT_TABLE           2
#define NSI_IP_ICMPSTATS_TABLE             3
#define NSI_IP_IPSTATS_TABLE               6
#define NSI_IP_UNICAST_TABLE              10
#define NSI_IP_NEIGHBOUR_TABLE            11
#define NSI_IP_FORWARD_TABLE              16

struct nsi_ip_cmpt_rw
{
    DWORD not_forwarding;
    DWORD unk;
    DWORD default_ttl;
    DWORD unk2;
};

struct nsi_ip_cmpt_dynamic
{
    DWORD num_ifs;
    DWORD num_routes;
    DWORD unk;
    DWORD num_addrs;
};

struct nsi_ip_icmpstats_dynamic
{
    DWORD in_msgs;
    DWORD in_errors;
    DWORD in_type_counts[256];
    DWORD out_msgs;
    DWORD out_errors;
    DWORD out_type_counts[256];
};

struct nsi_ip_ipstats_dynamic
{
    DWORD unk[4];
    ULONGLONG in_recv;
    ULONGLONG in_octets;
    ULONGLONG fwd_dgrams;
    ULONGLONG in_delivers;
    ULONGLONG out_reqs;
    ULONGLONG unk2;
    ULONGLONG unk3;
    ULONGLONG out_octets;
    ULONGLONG unk4[6];
    ULONGLONG in_hdr_errs;
    DWORD in_addr_errs;
    DWORD in_unk_protos;
    DWORD unk5;
    DWORD reasm_reqds;
    DWORD reasm_oks;
    DWORD reasm_fails;
    DWORD in_discards;
    DWORD out_no_routes;
    DWORD out_discards;
    DWORD routing_discards;
    DWORD frag_oks;
    DWORD frag_fails;
    DWORD frag_creates;
    DWORD unk6[7];
};

struct nsi_ip_ipstats_static
{
    DWORD reasm_timeout;
};

struct nsi_ipv4_unicast_key
{
    NET_LUID luid;
    IN_ADDR addr;
    DWORD pad;
};

struct nsi_ipv6_unicast_key
{
    NET_LUID luid;
    IN6_ADDR addr;
};

struct nsi_ip_unicast_rw
{
    DWORD preferred_lifetime;
    DWORD valid_lifetime;
    DWORD prefix_origin;
    DWORD suffix_origin;
    DWORD on_link_prefix;
    DWORD unk[2];
};

struct nsi_ip_unicast_dynamic
{
    DWORD scope_id;
    DWORD dad_state;
};

struct nsi_ip_unicast_static
{
    ULONG64 creation_time;
};

struct nsi_ipv4_neighbour_key
{
    NET_LUID luid;
    NET_LUID luid2;
    IN_ADDR addr;
    DWORD pad;
};

struct nsi_ipv6_neighbour_key
{
    NET_LUID luid;
    NET_LUID luid2;
    IN6_ADDR addr;
};

struct nsi_ip_neighbour_rw
{
    BYTE phys_addr[IF_MAX_PHYS_ADDRESS_LENGTH];
};

struct nsi_ip_neighbour_dynamic
{
    DWORD state;
    DWORD time;
    struct
    {
        BOOLEAN is_router;
        BOOLEAN is_unreachable;
    } flags;
    USHORT phys_addr_len;
    DWORD unk;
};

struct nsi_ipv4_forward_key
{
    DWORD unk;
    IN_ADDR prefix;
    BYTE prefix_len;
    BYTE unk2[3];
    DWORD unk3[3];
    NET_LUID luid;
    NET_LUID luid2;
    IN_ADDR next_hop;
    DWORD pad;
};

struct nsi_ipv6_forward_key
{
    DWORD unk;
    IN6_ADDR prefix;
    BYTE prefix_len;
    BYTE unk2[3];
    DWORD unk3[3];
    DWORD pad;
    NET_LUID luid;
    NET_LUID luid2;
    IN6_ADDR next_hop;
};

struct nsi_ip_forward_rw
{
    DWORD site_prefix_len;
    DWORD valid_lifetime;
    DWORD preferred_lifetime;
    DWORD metric;
    DWORD protocol;
    BYTE loopback;
    BYTE autoconf;
    BYTE publish;
    BYTE immortal;
    BYTE unk[4];
    DWORD unk2;
};

struct nsi_ipv4_forward_dynamic
{
    DWORD age;
    DWORD unk[3];
    IN_ADDR addr2; /* often a repeat of prefix */
};

struct nsi_ipv6_forward_dynamic
{
    DWORD age;
    DWORD unk[3];
    IN6_ADDR addr2; /* often a repeat of prefix */
};

struct nsi_ip_forward_static
{
    DWORD origin;
    DWORD if_index;
};

/* Undocumented NSI TCP tables */
#define NSI_TCP_STATS_TABLE                0
#define NSI_TCP_ALL_TABLE                  3
#define NSI_TCP_ESTAB_TABLE                4
#define NSI_TCP_LISTEN_TABLE               5

struct nsi_tcp_stats_dynamic
{
    DWORD active_opens;
    DWORD passive_opens;
    DWORD attempt_fails;
    DWORD est_rsts;
    DWORD cur_est;
    DWORD pad; /* ? */
    ULONGLONG in_segs;
    ULONGLONG out_segs;
    DWORD retrans_segs;
    DWORD out_rsts;
    DWORD in_errs;
    DWORD num_conns;
    DWORD unk[12];
};

struct nsi_tcp_stats_static
{
    DWORD rto_algo;
    DWORD rto_min;
    DWORD rto_max;
    DWORD max_conns;
    DWORD unk;
};

struct nsi_tcp_conn_key
{
    SOCKADDR_INET local;
    SOCKADDR_INET remote;
};

struct nsi_tcp_conn_dynamic
{
    DWORD state;
    DWORD unk[3];
};

struct nsi_tcp_conn_static
{
    DWORD unk[3];
    DWORD pid;
    ULONGLONG create_time;
    ULONGLONG mod_info;
};

/* Undocumented NSI UDP tables */
#define NSI_UDP_STATS_TABLE                0
#define NSI_UDP_ENDPOINT_TABLE             1

struct nsi_udp_stats_dynamic
{
    ULONGLONG in_dgrams;
    DWORD no_ports;
    DWORD in_errs;
    ULONGLONG out_dgrams;
    DWORD num_addrs;
    DWORD unk[5];
};

struct nsi_udp_endpoint_key
{
    SOCKADDR_INET local;
};

struct nsi_udp_endpoint_static
{
    DWORD pid;
    DWORD unk;
    ULONGLONG create_time;
    DWORD flags;
    DWORD unk2;
    ULONGLONG mod_info;
};

/* Wine specific ioctl interface */

#define IOCTL_NSIPROXY_WINE_ENUMERATE_ALL         CTL_CODE(FILE_DEVICE_NETWORK, 0x400, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS    CTL_CODE(FILE_DEVICE_NETWORK, 0x401, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_GET_PARAMETER         CTL_CODE(FILE_DEVICE_NETWORK, 0x402, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_ICMP_ECHO             CTL_CODE(FILE_DEVICE_NETWORK, 0x403, METHOD_BUFFERED, 0)

/* input for IOCTL_NSIPROXY_WINE_ENUMERATE_ALL */
struct nsiproxy_enumerate_all
{
    NPI_MODULEID module;
    DWORD first_arg;
    DWORD second_arg;
    DWORD table;
    DWORD key_size;
    DWORD rw_size;
    DWORD dynamic_size;
    DWORD static_size;
    DWORD count;
};

/* input for IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS */
struct nsiproxy_get_all_parameters
{
    NPI_MODULEID module;
    DWORD first_arg;
    DWORD table;
    DWORD key_size;
    DWORD rw_size;
    DWORD dynamic_size;
    DWORD static_size;
    BYTE key[1]; /* key_size */
};

/* input for IOCTL_NSIPROXY_WINE_GET_PARAMETER */
struct nsiproxy_get_parameter
{
    NPI_MODULEID module;
    DWORD first_arg;
    DWORD table;
    DWORD key_size;
    DWORD param_type;
    DWORD data_offset;
    BYTE key[1]; /* key_size */
};

/* input for IOCTL_NSIPROXY_WINE_ICMP_ECHO */
struct nsiproxy_icmp_echo
{
    SOCKADDR_INET src;
    SOCKADDR_INET dst;
    ULONGLONG user_reply_ptr;
    BYTE bits;
    BYTE ttl;
    BYTE tos;
    BYTE flags;
    DWORD opt_size;
    DWORD req_size;
    DWORD timeout;
    BYTE data[1]; /* ((opt_size + 3) & ~3) + req_size */
};

/* Undocumented Nsi api */

#define NSI_PARAM_TYPE_RW      0
#define NSI_PARAM_TYPE_DYNAMIC 1
#define NSI_PARAM_TYPE_STATIC  2

struct nsi_enumerate_all_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    DWORD_PTR table;
    DWORD first_arg;
    DWORD second_arg;
    void *key_data;
    DWORD key_size;
    void *rw_data;
    DWORD rw_size;
    void *dynamic_data;
    DWORD dynamic_size;
    void *static_data;
    DWORD static_size;
    DWORD_PTR count;
};

struct nsi_get_all_parameters_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    DWORD_PTR table;
    DWORD first_arg;
    DWORD unknown2;
    const void *key;
    DWORD key_size;
    void *rw_data;
    DWORD rw_size;
    void *dynamic_data;
    DWORD dynamic_size;
    void *static_data;
    DWORD static_size;
};

struct nsi_get_parameter_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    DWORD_PTR table;
    DWORD first_arg;
    DWORD unknown2;
    const void *key;
    DWORD key_size;
    DWORD_PTR param_type;
    void *data;
    DWORD data_size;
    DWORD data_offset;
};

DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 );
DWORD WINAPI NsiEnumerateObjectsAllParameters( DWORD unk, DWORD unk2, const NPI_MODULEID *module, DWORD table,
                                               void *key_data, DWORD key_size, void *rw_data, DWORD rw_size,
                                               void *dynamic_data, DWORD dynamic_size, void *static_data, DWORD static_size,
                                               DWORD *count );
DWORD WINAPI NsiEnumerateObjectsAllParametersEx( struct nsi_enumerate_all_ex *params );
void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data );
DWORD WINAPI NsiGetAllParameters( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                                  void *rw_data, DWORD rw_size, void *dynamic_data, DWORD dynamic_size,
                                  void *static_data, DWORD static_size );
DWORD WINAPI NsiGetAllParametersEx( struct nsi_get_all_parameters_ex *params );
DWORD WINAPI NsiGetParameter( DWORD unk, const NPI_MODULEID *module, DWORD table, const void *key, DWORD key_size,
                              DWORD param_type, void *data, DWORD data_size, DWORD data_offset );
DWORD WINAPI NsiGetParameterEx( struct nsi_get_parameter_ex *params );

#endif /* __WINE_NSI_H */
