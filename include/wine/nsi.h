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
    UINT admin_status;
    IF_COUNTED_STRING alias; /* .Length in bytes not including '\0' */
    IF_PHYSICAL_ADDRESS phys_addr;
    USHORT pad;
    IF_COUNTED_STRING name2;
    UINT unk;
};

struct nsi_ndis_ifinfo_dynamic
{
    UINT oper_status;
    struct
    {
        UINT unk : 1;
        UINT not_media_conn : 1;
        UINT unk2 : 30;
    } flags;
    UINT media_conn_state;
    UINT unk;
    UINT mtu;
    ULONG64 xmit_speed;
    ULONG64 rcv_speed;
    ULONG64 unk2[3];
    ULONG64 in_discards;
    ULONG64 in_errors;
    ULONG64 in_octets;
    ULONG64 in_ucast_pkts;
    ULONG64 in_mcast_pkts;
    ULONG64 in_bcast_pkts;
    ULONG64 out_octets;
    ULONG64 out_ucast_pkts;
    ULONG64 out_mcast_pkts;
    ULONG64 out_bcast_pkts;
    ULONG64 out_errors;
    ULONG64 out_discards;
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
    UINT if_index;
    IF_COUNTED_STRING descr;
    UINT type;
    UINT access_type;
    UINT unk;
    UINT conn_type;
    GUID if_guid;
    USHORT conn_present;
    IF_PHYSICAL_ADDRESS perm_phys_addr;
    struct
    {
        UINT hw : 1;
        UINT filter : 1;
        UINT unk : 30;
    } flags;
    UINT media_type;
    UINT phys_medium_type;
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
    UINT not_forwarding;
    UINT unk;
    UINT default_ttl;
    UINT unk2;
};

struct nsi_ip_cmpt_dynamic
{
    UINT num_ifs;
    UINT num_routes;
    UINT unk;
    UINT num_addrs;
};

struct nsi_ip_icmpstats_dynamic
{
    UINT in_msgs;
    UINT in_errors;
    UINT in_type_counts[256];
    UINT out_msgs;
    UINT out_errors;
    UINT out_type_counts[256];
};

struct nsi_ip_ipstats_dynamic
{
    UINT unk[4];
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
    UINT in_addr_errs;
    UINT in_unk_protos;
    UINT unk5;
    UINT reasm_reqds;
    UINT reasm_oks;
    UINT reasm_fails;
    UINT in_discards;
    UINT out_no_routes;
    UINT out_discards;
    UINT routing_discards;
    UINT frag_oks;
    UINT frag_fails;
    UINT frag_creates;
    UINT unk6[7];
};

struct nsi_ip_ipstats_static
{
    UINT reasm_timeout;
};

struct nsi_ipv4_unicast_key
{
    NET_LUID luid;
    IN_ADDR addr;
    UINT pad;
};

struct nsi_ipv6_unicast_key
{
    NET_LUID luid;
    IN6_ADDR addr;
};

struct nsi_ip_unicast_rw
{
    UINT preferred_lifetime;
    UINT valid_lifetime;
    UINT prefix_origin;
    UINT suffix_origin;
    UINT on_link_prefix;
    UINT unk[2];
};

struct nsi_ip_unicast_dynamic
{
    UINT scope_id;
    UINT dad_state;
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
    UINT pad;
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
    UINT state;
    UINT time;
    struct
    {
        BOOLEAN is_router;
        BOOLEAN is_unreachable;
    } flags;
    USHORT phys_addr_len;
    UINT unk;
};

struct nsi_ipv4_forward_key
{
    UINT unk;
    IN_ADDR prefix;
    BYTE prefix_len;
    BYTE unk2[3];
    UINT unk3[3];
    NET_LUID luid;
    NET_LUID luid2;
    IN_ADDR next_hop;
    UINT pad;
};

struct nsi_ipv6_forward_key
{
    UINT unk;
    IN6_ADDR prefix;
    BYTE prefix_len;
    BYTE unk2[3];
    UINT unk3[3];
    UINT pad;
    NET_LUID luid;
    NET_LUID luid2;
    IN6_ADDR next_hop;
};

struct nsi_ip_forward_rw
{
    UINT site_prefix_len;
    UINT valid_lifetime;
    UINT preferred_lifetime;
    UINT metric;
    UINT protocol;
    BYTE loopback;
    BYTE autoconf;
    BYTE publish;
    BYTE immortal;
    BYTE unk[4];
    UINT unk2;
};

struct nsi_ipv4_forward_dynamic
{
    UINT age;
    UINT unk[3];
    IN_ADDR addr2; /* often a repeat of prefix */
};

struct nsi_ipv6_forward_dynamic
{
    UINT age;
    UINT unk[3];
    IN6_ADDR addr2; /* often a repeat of prefix */
};

struct nsi_ip_forward_static
{
    UINT origin;
    UINT if_index;
};

/* Undocumented NSI TCP tables */
#define NSI_TCP_STATS_TABLE                0
#define NSI_TCP_ALL_TABLE                  3
#define NSI_TCP_ESTAB_TABLE                4
#define NSI_TCP_LISTEN_TABLE               5

struct nsi_tcp_stats_dynamic
{
    UINT active_opens;
    UINT passive_opens;
    UINT attempt_fails;
    UINT est_rsts;
    UINT cur_est;
    UINT pad; /* ? */
    ULONGLONG in_segs;
    ULONGLONG out_segs;
    UINT retrans_segs;
    UINT out_rsts;
    UINT in_errs;
    UINT num_conns;
    UINT unk[12];
};

struct nsi_tcp_stats_static
{
    UINT rto_algo;
    UINT rto_min;
    UINT rto_max;
    UINT max_conns;
    UINT unk;
};

struct nsi_tcp_conn_key
{
    SOCKADDR_INET local;
    SOCKADDR_INET remote;
};

struct nsi_tcp_conn_dynamic
{
    UINT state;
    UINT unk[4];
};

struct nsi_tcp_conn_static
{
    UINT unk[3];
    UINT pid;
    ULONGLONG create_time;
    ULONGLONG mod_info;
};

/* Undocumented NSI UDP tables */
#define NSI_UDP_STATS_TABLE                0
#define NSI_UDP_ENDPOINT_TABLE             1

struct nsi_udp_stats_dynamic
{
    ULONGLONG in_dgrams;
    UINT no_ports;
    UINT in_errs;
    ULONGLONG out_dgrams;
    UINT num_addrs;
    UINT unk[5];
};

struct nsi_udp_endpoint_key
{
    SOCKADDR_INET local;
};

struct nsi_udp_endpoint_static
{
    UINT pid;
    UINT unk;
    ULONGLONG create_time;
    UINT flags;
    UINT unk2;
    ULONGLONG mod_info;
};

/* Wine specific ioctl interface */

#define IOCTL_NSIPROXY_WINE_ENUMERATE_ALL         CTL_CODE(FILE_DEVICE_NETWORK, 0x400, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS    CTL_CODE(FILE_DEVICE_NETWORK, 0x401, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_GET_PARAMETER         CTL_CODE(FILE_DEVICE_NETWORK, 0x402, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_ICMP_ECHO             CTL_CODE(FILE_DEVICE_NETWORK, 0x403, METHOD_BUFFERED, 0)
#define IOCTL_NSIPROXY_WINE_CHANGE_NOTIFICATION   CTL_CODE(FILE_DEVICE_NETWORK, 0x404, METHOD_BUFFERED, 0)

/* input for IOCTL_NSIPROXY_WINE_ENUMERATE_ALL */
struct nsiproxy_enumerate_all
{
    NPI_MODULEID module;
    UINT first_arg;
    UINT second_arg;
    UINT table;
    UINT key_size;
    UINT rw_size;
    UINT dynamic_size;
    UINT static_size;
    UINT count;
};

/* input for IOCTL_NSIPROXY_WINE_GET_ALL_PARAMETERS */
struct nsiproxy_get_all_parameters
{
    NPI_MODULEID module;
    UINT first_arg;
    UINT table;
    UINT key_size;
    UINT rw_size;
    UINT dynamic_size;
    UINT static_size;
    BYTE key[1]; /* key_size */
};

/* input for IOCTL_NSIPROXY_WINE_GET_PARAMETER */
struct nsiproxy_get_parameter
{
    NPI_MODULEID module;
    UINT first_arg;
    UINT table;
    UINT key_size;
    UINT param_type;
    UINT data_offset;
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
    UINT opt_size;
    UINT req_size;
    UINT timeout;
    BYTE data[1]; /* ((opt_size + 3) & ~3) + req_size */
};

/* input for IOCTL_NSIPROXY_WINE_CHANGE_NOTIFICATION */
struct nsiproxy_request_notification
{
    NPI_MODULEID module;
    UINT table;
};

/* Undocumented Nsi api */

#define NSI_PARAM_TYPE_RW      0
#define NSI_PARAM_TYPE_DYNAMIC 1
#define NSI_PARAM_TYPE_STATIC  2

struct nsi_enumerate_all_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    UINT_PTR table;
    UINT first_arg;
    UINT second_arg;
    void *key_data;
    UINT key_size;
    void *rw_data;
    UINT rw_size;
    void *dynamic_data;
    UINT dynamic_size;
    void *static_data;
    UINT static_size;
    UINT_PTR count;
};

struct nsi_get_all_parameters_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    UINT_PTR table;
    UINT first_arg;
    UINT unknown2;
    const void *key;
    UINT key_size;
    void *rw_data;
    UINT rw_size;
    void *dynamic_data;
    UINT dynamic_size;
    void *static_data;
    UINT static_size;
};

struct nsi_get_parameter_ex
{
    void *unknown[2];
    const NPI_MODULEID *module;
    UINT_PTR table;
    UINT first_arg;
    UINT unknown2;
    const void *key;
    UINT key_size;
    UINT_PTR param_type;
    void *data;
    UINT data_size;
    UINT data_offset;
};

struct nsi_request_change_notification_ex
{
    DWORD unk;
    const NPI_MODULEID *module;
    UINT_PTR table;
    OVERLAPPED *ovr;
    HANDLE *handle;
};

DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 );
DWORD WINAPI NsiCancelChangeNotification( OVERLAPPED *ovr );
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
DWORD WINAPI NsiRequestChangeNotification( DWORD unk, const NPI_MODULEID *module, DWORD table, OVERLAPPED *ovr,
                                           HANDLE *handle );
DWORD WINAPI NsiRequestChangeNotificationEx( struct nsi_request_change_notification_ex *params );

#endif /* __WINE_NSI_H */
