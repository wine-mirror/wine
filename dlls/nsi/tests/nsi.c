/*
 * Network Store Interface tests
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
#include "wine/test.h"

static int bounded( ULONG64 val, ULONG64 lo, ULONG64 hi )
{
    return lo <= val && val <= hi;
}

static void test_ndis_ifinfo( void )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, name2), FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, unk),
                         sizeof(struct nsi_ndis_ifinfo_rw) };
    struct nsi_ndis_ifinfo_rw *rw_tbl;
    struct nsi_ndis_ifinfo_dynamic *dyn_tbl, *dyn_tbl_2;
    struct nsi_ndis_ifinfo_static *stat_tbl;
    DWORD err, count, i, rw_size;
    NET_LUID *luid_tbl, *luid_tbl_2;
    MIB_IF_TABLE2 *table;

    /* Contents of GetIfTable2() keyed by luids */

    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiAllocateAndGetTable( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, (void **)&luid_tbl, sizeof(*luid_tbl),
                                      (void **)&rw_tbl, rw_sizes[i], (void **)&dyn_tbl, sizeof(*dyn_tbl),
                                      (void **)&stat_tbl, sizeof(*stat_tbl), &count, 0 );
        if (!err) break;
    }
todo_wine
    ok( !err, "got %d\n", err );
    if (err) return;
    rw_size = rw_sizes[i];

    err = GetIfTable2( &table );
    ok( !err, "got %d\n", err );
    ok( table->NumEntries == count, "table entries %d count %d\n", table->NumEntries, count );

    /* Grab the dyn table again to provide an upper bound for the stats returned from GetIfTable2().
       (The luids must be retrieved again otherwise NsiAllocateAndGetTable() fails). */
    err = NsiAllocateAndGetTable( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, (void **)&luid_tbl_2, sizeof(*luid_tbl_2),
                                  NULL, 0, (void **)&dyn_tbl_2, sizeof(*dyn_tbl_2),
                                  NULL, 0, &count, 0 );
    ok( !err, "got %d\n", err );
    ok( table->NumEntries == count, "table entries %d count %d\n", table->NumEntries, count );

    for (i = 0; i < count; i++)
    {
        MIB_IF_ROW2 *row = table->Table + i;
        NET_LUID *luid = luid_tbl + i;
        struct nsi_ndis_ifinfo_rw *rw = (struct nsi_ndis_ifinfo_rw *)((BYTE *)rw_tbl + i * rw_size);
        struct nsi_ndis_ifinfo_dynamic *dyn = dyn_tbl + i, *dyn_2 = dyn_tbl_2 + i;
        struct nsi_ndis_ifinfo_static *stat = stat_tbl + i;

        winetest_push_context( "%d", i );
        ok( row->InterfaceLuid.Value == luid->Value, "mismatch\n" );
        ok( row->InterfaceIndex == stat->if_index, "mismatch\n" );
        ok( IsEqualGUID( &row->InterfaceGuid, &stat->if_guid ), "mismatch\n" );
        ok( !memcmp( row->Alias, rw->alias.String, rw->alias.Length ), "mismatch\n" );
        ok( lstrlenW( row->Alias ) * 2 == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( row->Description, stat->descr.String, sizeof(row->Description) ), "mismatch\n" );
        ok( lstrlenW( row->Description ) * 2 == stat->descr.Length, "mismatch\n" );
        ok( row->PhysicalAddressLength == rw->phys_addr.Length, "mismatch\n" );
        ok( !memcmp( row->PhysicalAddress, rw->phys_addr.Address, IF_MAX_PHYS_ADDRESS_LENGTH ), "mismatch\n" );
        ok( row->PhysicalAddressLength == stat->perm_phys_addr.Length, "mismatch\n" );
        ok( !memcmp( row->PermanentPhysicalAddress, stat->perm_phys_addr.Address, IF_MAX_PHYS_ADDRESS_LENGTH ),
            "mismatch\n" );
        ok( row->Mtu == dyn->mtu, "mismatch\n" );
        ok( row->Type == stat->type, "mismatch\n" );
        /* TunnelType */
        ok( row->MediaType == stat->media_type, "mismatch\n" );
        ok( row->PhysicalMediumType == stat->phys_medium_type, "mismatch\n" );
        ok( row->AccessType == stat->access_type, "mismatch\n" );
        /* DirectionType */
        ok( row->InterfaceAndOperStatusFlags.HardwareInterface == stat->flags.hw, "mismatch\n" );
        ok( row->InterfaceAndOperStatusFlags.FilterInterface == stat->flags.filter, "mismatch\n" );
        ok( row->InterfaceAndOperStatusFlags.ConnectorPresent == stat->conn_present, "mismatch\n" );
        /* NotAuthenticated */
        ok( row->InterfaceAndOperStatusFlags.NotMediaConnected == dyn->flags.not_media_conn, "mismatch\n" );
        /* Paused */
        /* LowPower */
        /* EndPointInterface */
        ok( row->OperStatus == dyn->oper_status, "mismatch\n" );
        ok( row->AdminStatus == rw->admin_status, "mismatch\n" );
        ok( row->MediaConnectState == dyn->media_conn_state, "mismatch\n" );
        ok( IsEqualGUID( &row->NetworkGuid, &rw->network_guid ), "mismatch\n" );
        ok( row->ConnectionType == stat->conn_type, "mismatch\n" );
        if (dyn->xmit_speed == ~0ULL) dyn->xmit_speed = 0;
        ok( row->TransmitLinkSpeed == dyn->xmit_speed, "mismatch\n" );
        if (dyn->rcv_speed == ~0ULL) dyn->rcv_speed = 0;
        ok( row->ReceiveLinkSpeed == dyn->rcv_speed, "mismatch\n" );
        ok( bounded( row->InOctets, dyn->in_octets, dyn_2->in_octets ), "mismatch\n" );
        ok( bounded( row->InUcastPkts, dyn->in_ucast_pkts, dyn_2->in_ucast_pkts ), "mismatch\n" );
        ok( bounded( row->InNUcastPkts, dyn->in_mcast_pkts + dyn->in_bcast_pkts,
                     dyn_2->in_mcast_pkts + dyn_2->in_bcast_pkts ), "mismatch\n" );
        ok( bounded( row->InDiscards, dyn->in_discards, dyn_2->in_discards ), "mismatch\n" );
        ok( bounded( row->InErrors, dyn->in_errors, dyn_2->in_errors ), "mismatch\n" );
        /* InUnknownProtos */
        ok( bounded( row->InUcastOctets, dyn->in_ucast_octs, dyn_2->in_ucast_octs ), "mismatch\n" );
        ok( bounded( row->InMulticastOctets, dyn->in_mcast_octs, dyn_2->in_mcast_octs ), "mismatch\n" );
        ok( bounded( row->InBroadcastOctets, dyn->in_bcast_octs, dyn_2->in_bcast_octs ), "mismatch\n" );
        ok( bounded( row->OutOctets, dyn->out_octets, dyn_2->out_octets ), "mismatch\n" );
        ok( bounded( row->OutUcastPkts, dyn->out_ucast_pkts, dyn_2->out_ucast_pkts ), "mismatch\n" );
        ok( bounded( row->OutNUcastPkts, dyn->out_mcast_pkts + dyn->out_bcast_pkts,
                     dyn_2->out_mcast_pkts + dyn_2->out_bcast_pkts ), "mismatch\n" );
        ok( bounded( row->OutDiscards, dyn->out_discards, dyn_2->out_discards ), "mismatch\n" );
        ok( bounded( row->OutErrors, dyn->out_errors, dyn_2->out_errors ), "mismatch\n" );
        ok( bounded( row->OutUcastOctets, dyn->out_ucast_octs, dyn_2->out_ucast_octs ), "mismatch\n" );
        ok( bounded( row->OutMulticastOctets, dyn->out_mcast_octs, dyn_2->out_mcast_octs ), "mismatch\n" );
        ok( bounded( row->OutBroadcastOctets, dyn->out_bcast_octs, dyn_2->out_bcast_octs ), "mismatch\n" );
        /* OutQLen */
        winetest_pop_context();
    }

    FreeMibTable( table );
    NsiFreeTable( luid_tbl_2, NULL, dyn_tbl_2, NULL );
    NsiFreeTable( luid_tbl, rw_tbl, dyn_tbl, stat_tbl );
}

START_TEST( nsi )
{
    test_ndis_ifinfo();
}
