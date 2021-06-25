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

/* Undocumented Nsi api */
DWORD WINAPI NsiAllocateAndGetTable( DWORD unk, const NPI_MODULEID *module, DWORD table, void **key_data, DWORD key_size,
                                     void **rw_data, DWORD rw_size, void **dynamic_data, DWORD dynamic_size,
                                     void **static_data, DWORD static_size, DWORD *count, DWORD unk2 );
void WINAPI NsiFreeTable( void *key_data, void *rw_data, void *dynamic_data, void *static_data );

#endif /* __WINE_NSI_H */
