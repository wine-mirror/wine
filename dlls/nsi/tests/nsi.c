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

static int unstable( int val )
{
    return !winetest_interactive || val;
}

static void test_nsi_api( void )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, name2), FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, unk),
                         sizeof(struct nsi_ndis_ifinfo_rw) };
    struct nsi_ndis_ifinfo_rw *rw_tbl, *rw, get_rw, *enum_rw_tbl, *enum_rw;
    struct nsi_ndis_ifinfo_dynamic *dyn_tbl, *dyn, get_dyn, *enum_dyn_tbl, *enum_dyn;
    struct nsi_ndis_ifinfo_static *stat_tbl, *stat, get_stat, *enum_stat_tbl, *enum_stat;
    struct nsi_get_all_parameters_ex get_all_params;
    struct nsi_get_parameter_ex get_param;
    struct nsi_enumerate_all_ex enum_params;
    DWORD err, count, i, rw_size, enum_count;
    NET_LUID *luid_tbl, *enum_luid_tbl;

    /* Use the NDIS ifinfo table to test various api */
    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiAllocateAndGetTable( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, (void **)&luid_tbl, sizeof(*luid_tbl),
                                      (void **)&rw_tbl, rw_sizes[i], (void **)&dyn_tbl, sizeof(*dyn_tbl),
                                      (void **)&stat_tbl, sizeof(*stat_tbl), &count, 0 );
        if (!err) break;
    }
    ok( !err, "got %d\n", err );
    rw_size = rw_sizes[i];

    for (i = 0; i < count; i++)
    {
        winetest_push_context( "%d", i );
        rw = (struct nsi_ndis_ifinfo_rw *)((BYTE *)rw_tbl + i * rw_size);
        dyn = dyn_tbl + i;
        stat = stat_tbl + i;

        err = NsiGetAllParameters( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl + i, sizeof(*luid_tbl),
                                   &get_rw, rw_size, &get_dyn, sizeof(get_dyn), &get_stat, sizeof(get_stat) );
        ok( !err, "got %d\n", err );
        /* test a selection of members */
        ok( IsEqualGUID( &get_rw.network_guid, &rw->network_guid ), "mismatch\n" );
        ok( get_rw.alias.Length == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( get_rw.alias.String, rw->alias.String, rw->alias.Length ), "mismatch\n" );
        ok( get_rw.phys_addr.Length == rw->phys_addr.Length, "mismatch\n" );
        ok( !memcmp( get_rw.phys_addr.Address, rw->phys_addr.Address, IF_MAX_PHYS_ADDRESS_LENGTH ), "mismatch\n" );
        ok( get_dyn.oper_status == dyn->oper_status, "mismatch\n" );
        ok( get_stat.if_index == stat->if_index, "mismatch\n" );
        ok( IsEqualGUID( &get_stat.if_guid, &stat->if_guid ), "mismatch\n" );

        memset( &get_rw, 0xcc, sizeof(get_rw) );
        memset( &get_dyn, 0xcc, sizeof(get_dyn) );
        memset( &get_stat, 0xcc, sizeof(get_stat) );

        memset( &get_all_params, 0, sizeof(get_all_params) );
        get_all_params.first_arg = 1;
        get_all_params.module = &NPI_MS_NDIS_MODULEID;
        get_all_params.table = NSI_NDIS_IFINFO_TABLE;
        get_all_params.key = luid_tbl + i;
        get_all_params.key_size = sizeof(*luid_tbl);
        get_all_params.rw_data = &get_rw;
        get_all_params.rw_size = rw_size;
        get_all_params.dynamic_data = &get_dyn;
        get_all_params.dynamic_size = sizeof(get_dyn);
        get_all_params.static_data = &get_stat;
        get_all_params.static_size = sizeof(get_stat);

        err = NsiGetAllParametersEx( &get_all_params );
        ok( !err, "got %d\n", err );
        /* test a selection of members */
        ok( IsEqualGUID( &get_rw.network_guid, &rw->network_guid ), "mismatch\n" );
        ok( get_rw.alias.Length == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( get_rw.alias.String, rw->alias.String, rw->alias.Length ), "mismatch\n" );
        ok( get_rw.phys_addr.Length == rw->phys_addr.Length, "mismatch\n" );
        ok( !memcmp( get_rw.phys_addr.Address, rw->phys_addr.Address, IF_MAX_PHYS_ADDRESS_LENGTH ), "mismatch\n" );
        ok( get_dyn.oper_status == dyn->oper_status, "mismatch\n" );
        ok( get_stat.if_index == stat->if_index, "mismatch\n" );
        ok( IsEqualGUID( &get_stat.if_guid, &stat->if_guid ), "mismatch\n" );

        memset( &get_rw, 0xcc, sizeof(get_rw) );
        memset( &get_dyn, 0xcc, sizeof(get_dyn) );
        memset( &get_stat, 0xcc, sizeof(get_stat) );

        err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl + i, sizeof(*luid_tbl),
                               NSI_PARAM_TYPE_RW, &get_rw.alias, sizeof(get_rw.alias),
                               FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, alias) );
        ok( !err, "got %d\n", err );
        ok( get_rw.alias.Length == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( get_rw.alias.String, rw->alias.String, rw->alias.Length ), "mismatch\n" );

        err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl + i, sizeof(*luid_tbl),
                               NSI_PARAM_TYPE_STATIC, &get_stat.if_index, sizeof(get_stat.if_index),
                               FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_index) );
        ok( !err, "got %d\n", err );
        ok( get_stat.if_index == stat->if_index, "mismatch\n" );

        err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl + i, sizeof(*luid_tbl),
                               NSI_PARAM_TYPE_STATIC, &get_stat.if_guid, sizeof(get_stat.if_guid),
                               FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_guid) );
        ok( !err, "got %d\n", err );
        ok( IsEqualGUID( &get_stat.if_guid, &stat->if_guid ), "mismatch\n" );

        memset( &get_rw, 0xcc, sizeof(get_rw) );
        memset( &get_dyn, 0xcc, sizeof(get_dyn) );
        memset( &get_stat, 0xcc, sizeof(get_stat) );

        memset( &get_param, 0, sizeof(get_param) );
        get_param.first_arg = 1;
        get_param.module = &NPI_MS_NDIS_MODULEID;
        get_param.table = NSI_NDIS_IFINFO_TABLE;
        get_param.key = luid_tbl + i;
        get_param.key_size = sizeof(*luid_tbl);

        get_param.param_type = NSI_PARAM_TYPE_RW;
        get_param.data = &get_rw.alias;
        get_param.data_size = sizeof(get_rw.alias);
        get_param.data_offset = FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, alias);
        err = NsiGetParameterEx( &get_param );
        ok( !err, "got %d\n", err );
        ok( get_rw.alias.Length == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( get_rw.alias.String, rw->alias.String, rw->alias.Length ), "mismatch\n" );

        get_param.param_type = NSI_PARAM_TYPE_STATIC;
        get_param.data = &get_stat.if_index;
        get_param.data_size = sizeof(get_stat.if_index);
        get_param.data_offset = FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_index);
        err = NsiGetParameterEx( &get_param );
        ok( !err, "got %d\n", err );
        ok( get_stat.if_index == stat->if_index, "mismatch\n" );

        get_param.param_type = NSI_PARAM_TYPE_STATIC;
        get_param.data = &get_stat.if_guid;
        get_param.data_size = sizeof(get_stat.if_guid);
        get_param.data_offset = FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_guid);
        err = NsiGetParameterEx( &get_param );
        ok( !err, "got %d\n", err );
        ok( IsEqualGUID( &get_stat.if_guid, &stat->if_guid ), "mismatch\n" );
        winetest_pop_context();
    }

    enum_count = 0;
    err = NsiEnumerateObjectsAllParameters( 1, 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE,
                                            NULL, 0, NULL, 0,
                                            NULL, 0, NULL, 0, &enum_count );
    ok( !err, "got %d\n", err );
    ok( enum_count == count, "mismatch\n" );

    enum_luid_tbl = malloc( count * sizeof(*enum_luid_tbl) );
    enum_rw_tbl = malloc( count * rw_size );
    enum_dyn_tbl = malloc( count * sizeof(*enum_dyn_tbl) );
    enum_stat_tbl = malloc( count * sizeof(*enum_stat_tbl) );

    err = NsiEnumerateObjectsAllParameters( 1, 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE,
                                            enum_luid_tbl, sizeof(*enum_luid_tbl), enum_rw_tbl, rw_size,
                                            enum_dyn_tbl, sizeof(*enum_dyn_tbl), enum_stat_tbl, sizeof(*enum_stat_tbl),
                                            &enum_count );
    ok( !err, "got %d\n", err );
    ok( enum_count == count, "mismatch\n" );

    for (i = 0; i < count; i++)
    {
        winetest_push_context( "%d", i );
        rw = (struct nsi_ndis_ifinfo_rw *)((BYTE *)rw_tbl + i * rw_size);
        enum_rw = (struct nsi_ndis_ifinfo_rw *)((BYTE *)enum_rw_tbl + i * rw_size);
        dyn = dyn_tbl + i;
        enum_dyn = enum_dyn_tbl + i;
        stat = stat_tbl + i;
        enum_stat = enum_stat_tbl + i;

        /* test a selection of members */
        ok( enum_luid_tbl[i].Value == luid_tbl[i].Value, "mismatch\n" );
        ok( IsEqualGUID( &enum_rw->network_guid, &rw->network_guid ), "mismatch\n" );
        ok( enum_rw->alias.Length == rw->alias.Length, "mismatch\n" );
        ok( !memcmp( enum_rw->alias.String, rw->alias.String, rw->alias.Length ), "mismatch\n" );
        ok( enum_rw->phys_addr.Length == rw->phys_addr.Length, "mismatch\n" );
        ok( !memcmp( enum_rw->phys_addr.Address, rw->phys_addr.Address, IF_MAX_PHYS_ADDRESS_LENGTH ), "mismatch\n" );
        ok( enum_dyn->oper_status == dyn->oper_status, "mismatch\n" );
        ok( enum_stat->if_index == stat->if_index, "mismatch\n" );
        ok( IsEqualGUID( &enum_stat->if_guid, &stat->if_guid ), "mismatch\n" );
        winetest_pop_context();
    }

    if (count > 0)
    {
        enum_count--;
        memset( enum_luid_tbl, 0xcc, count * sizeof(*enum_luid_tbl) );

        err = NsiEnumerateObjectsAllParameters( 1, 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE,
                                                enum_luid_tbl, sizeof(*enum_luid_tbl), enum_rw_tbl, rw_size,
                                                enum_dyn_tbl, sizeof(*enum_dyn_tbl), enum_stat_tbl, sizeof(*enum_stat_tbl),
                                                &enum_count );
        ok( err == ERROR_MORE_DATA, "got %d\n", err );
        ok( enum_count == count - 1, "mismatch\n" );

        for (i = 0; i < enum_count; i++) /* for simplicity just check the luids */
            ok( enum_luid_tbl[i].Value == luid_tbl[i].Value, "%d: mismatch\n", i );
    }

    memset( &enum_params, 0, sizeof(enum_params) );
    enum_params.first_arg = 1;
    enum_params.second_arg = 1;
    enum_params.module = &NPI_MS_NDIS_MODULEID;
    enum_params.table = NSI_NDIS_IFINFO_TABLE;
    enum_params.key_data = enum_luid_tbl;
    enum_params.key_size = sizeof(*enum_luid_tbl);
    enum_params.rw_data = enum_rw_tbl;
    enum_params.rw_size = rw_size;
    enum_params.dynamic_data = enum_dyn_tbl;
    enum_params.dynamic_size = sizeof(*enum_dyn_tbl);
    enum_params.static_data = enum_stat_tbl;
    enum_params.static_size = sizeof(*enum_stat_tbl);
    enum_params.count = count;

    err = NsiEnumerateObjectsAllParametersEx( &enum_params );
    ok( !err, "got %d\n", err );
    ok( enum_params.count == count, "mismatch\n" );

    free( enum_luid_tbl );
    free( enum_rw_tbl );
    free( enum_dyn_tbl );
    free( enum_stat_tbl );
    NsiFreeTable( luid_tbl, rw_tbl, dyn_tbl, stat_tbl );
}

static void test_ndis_ifinfo( void )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, name2), FIELD_OFFSET(struct nsi_ndis_ifinfo_rw, unk),
                         sizeof(struct nsi_ndis_ifinfo_rw) };
    struct nsi_ndis_ifinfo_rw *rw_tbl, rw_get;
    struct nsi_ndis_ifinfo_dynamic *dyn_tbl, *dyn_tbl_2, dyn_get;
    struct nsi_ndis_ifinfo_static *stat_tbl, stat_get;
    DWORD err, count, i, rw_size;
    NET_LUID *luid_tbl, *luid_tbl_2, luid_get;
    MIB_IF_TABLE2 *table;

    /* Contents of GetIfTable2() keyed by luids */

    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiAllocateAndGetTable( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, (void **)&luid_tbl, sizeof(*luid_tbl),
                                      (void **)&rw_tbl, rw_sizes[i], (void **)&dyn_tbl, sizeof(*dyn_tbl),
                                      (void **)&stat_tbl, sizeof(*stat_tbl), &count, 0 );
        if (!err) break;
    }
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
    err = NsiGetAllParameters( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl, sizeof(*luid_tbl),
                               &rw_get, rw_size, &dyn_get, sizeof(dyn_get),
                               &stat_get, sizeof(stat_get) );
    ok( !err, "got %d\n", err );
    ok( IsEqualGUID( &stat_tbl[0].if_guid, &stat_get.if_guid ), "mismatch\n" );

    err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, luid_tbl, sizeof(*luid_tbl),
                           NSI_PARAM_TYPE_STATIC, &stat_get.if_guid, sizeof(stat_get.if_guid),
                           FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_guid) );
    ok( !err, "got %d\n", err );
    ok( IsEqualGUID( &stat_tbl[0].if_guid, &stat_get.if_guid ), "mismatch\n" );

    luid_get.Value = ~0u;
    err = NsiGetAllParameters( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, &luid_get, sizeof(luid_get),
                               &rw_get, rw_size, &dyn_get, sizeof(dyn_get),
                               &stat_get, sizeof(stat_get) );
    ok( err == ERROR_FILE_NOT_FOUND, "got %d\n", err );

    err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, &luid_get, sizeof(luid_get),
                           NSI_PARAM_TYPE_STATIC, &stat_get.if_guid, sizeof(stat_get.if_guid),
                           FIELD_OFFSET(struct nsi_ndis_ifinfo_static, if_guid) );
    ok( err == ERROR_FILE_NOT_FOUND, "got %d\n", err );

    FreeMibTable( table );
    NsiFreeTable( luid_tbl_2, NULL, dyn_tbl_2, NULL );
    NsiFreeTable( luid_tbl, rw_tbl, dyn_tbl, stat_tbl );
}

static void test_ndis_index_luid( void )
{
    DWORD err, count, i;
    NET_LUID *luids, luid;
    DWORD index;

    /* index -> luid map.  NsiAllocateAndGetTable and NsiGetAllParameters fail */

    /* first get the luids */
    err = NsiAllocateAndGetTable( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_IFINFO_TABLE, (void **)&luids, sizeof(*luids),
                                  NULL, 0, NULL, 0, NULL, 0, &count, 0 );
    ok( !err, "got %d\n", err );

    for (i = 0; i < count; i++)
    {
        ConvertInterfaceLuidToIndex( luids + i, &index );
        err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_INDEX_LUID_TABLE, &index, sizeof(index),
                               NSI_PARAM_TYPE_STATIC, &luid, sizeof(luid), 0 );
        ok( !err, "got %d\n", err );
        ok( luid.Value == luids[i].Value, "%d: luid mismatch\n", i );
    }
    NsiFreeTable( luids, NULL, NULL, NULL );

    index = ~0u;
    err = NsiGetParameter( 1, &NPI_MS_NDIS_MODULEID, NSI_NDIS_INDEX_LUID_TABLE, &index, sizeof(index),
                           NSI_PARAM_TYPE_STATIC, &luid, sizeof(luid), 0 );
    ok( err == ERROR_FILE_NOT_FOUND, "got %d\n", err );
}

static void test_ip_cmpt( int family )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ip_cmpt_rw, unk2), sizeof(struct nsi_ip_cmpt_rw) };
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    struct nsi_ip_cmpt_dynamic dyn;
    struct nsi_ip_cmpt_rw rw;
    MIB_IPSTATS table;
    DWORD err, key, i;

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    /* key = 0 also seems to work, but NsiAllocateAndGetTable returns
       a single table with key == 1.  Presumably this is the compartment id */
    key = 1;
    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiGetAllParameters( 1, mod, 2, &key, sizeof(key), &rw, rw_sizes[i], &dyn, sizeof(dyn), NULL, 0 );
        if (!err) break;
    }
    ok( !err, "got %x\n", err );

    err = GetIpStatisticsEx( &table, family );
    ok( !err, "got %d\n", err );
    if (err) goto err;

    ok( table.dwForwarding - 1 == rw.not_forwarding, "%x vs %x\n", table.dwForwarding, rw.not_forwarding );
    ok( table.dwDefaultTTL == rw.default_ttl, "%x vs %x\n", table.dwDefaultTTL, rw.default_ttl );
    ok( table.dwNumIf == dyn.num_ifs, "%x vs %x\n", table.dwNumIf, dyn.num_ifs );
    ok( table.dwNumAddr == dyn.num_addrs, "%x vs %x\n", table.dwNumAddr, dyn.num_addrs );
    ok( table.dwNumRoutes == dyn.num_routes, "%x vs %x\n", table.dwNumRoutes, dyn.num_routes );

err:
    winetest_pop_context();
}

static void test_ip_icmpstats( int family )
{
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    struct nsi_ip_icmpstats_dynamic nsi_stats, nsi_stats2;
    MIB_ICMP_EX table;
    DWORD err, i;

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    err = NsiGetAllParameters( 1, mod, NSI_IP_ICMPSTATS_TABLE, NULL, 0, NULL, 0, &nsi_stats, sizeof(nsi_stats), NULL, 0 );
    ok( !err, "got %d\n", err );
    if (err) goto err;

    err = GetIcmpStatisticsEx( &table, family );
    ok( !err, "got %d\n", err );
    if (err) goto err;

    err = NsiGetAllParameters( 1, mod, NSI_IP_ICMPSTATS_TABLE, NULL, 0, NULL, 0, &nsi_stats2, sizeof(nsi_stats2), NULL, 0 );
    ok( !err, "got %d\n", err );

    ok( bounded( table.icmpInStats.dwMsgs, nsi_stats.in_msgs, nsi_stats2.in_msgs ),
        "%d vs [%d %d]\n", table.icmpInStats.dwMsgs, nsi_stats.in_msgs, nsi_stats2.in_msgs );
    ok( bounded( table.icmpInStats.dwErrors, nsi_stats.in_errors, nsi_stats2.in_errors ),
        "%d vs [%d %d]\n", table.icmpInStats.dwErrors, nsi_stats.in_errors, nsi_stats2.in_errors );
    ok( bounded( table.icmpOutStats.dwMsgs, nsi_stats.out_msgs, nsi_stats2.out_msgs ),
        "%d vs [%d %d]\n", table.icmpOutStats.dwMsgs, nsi_stats.out_msgs, nsi_stats2.out_msgs );
    ok( bounded( table.icmpOutStats.dwErrors, nsi_stats.out_errors, nsi_stats2.out_errors ),
        "%d vs [%d %d]\n", table.icmpOutStats.dwErrors, nsi_stats.out_errors, nsi_stats2.out_errors );
    for (i = 0; i < ARRAY_SIZE(nsi_stats.in_type_counts); i++)
    {
        winetest_push_context( "%d", i );
        ok( bounded( table.icmpInStats.rgdwTypeCount[i], nsi_stats.in_type_counts[i], nsi_stats2.in_type_counts[i] ),
            "%d vs [%d %d]\n", table.icmpInStats.rgdwTypeCount[i], nsi_stats.in_type_counts[i], nsi_stats2.in_type_counts[i] );
        ok( bounded( table.icmpOutStats.rgdwTypeCount[i], nsi_stats.out_type_counts[i], nsi_stats2.out_type_counts[i] ),
            "%d vs [%d %d]\n", table.icmpOutStats.rgdwTypeCount[i], nsi_stats.out_type_counts[i], nsi_stats2.out_type_counts[i] );
        winetest_pop_context();
    }
err:
    winetest_pop_context();
}


static void test_ip_ipstats( int family )
{
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    struct nsi_ip_ipstats_dynamic dyn, dyn2;
    struct nsi_ip_ipstats_static stat;
    MIB_IPSTATS table;
    DWORD err;

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    /* The table appears to consist of a single object without a key.  The rw data does exist but
       isn't part of GetIpStatisticsEx() and isn't yet tested */
    err = NsiGetAllParameters( 1, mod, NSI_IP_IPSTATS_TABLE, NULL, 0, NULL, 0, &dyn, sizeof(dyn), &stat, sizeof(stat) );
    ok( !err, "got %x\n", err );
    if (err) goto err;

    err = GetIpStatisticsEx( &table, family );
    ok( !err, "got %d\n", err );

    err = NsiGetAllParameters( 1, mod, NSI_IP_IPSTATS_TABLE, NULL, 0, NULL, 0, &dyn2, sizeof(dyn2), NULL, 0 );
    ok( !err, "got %x\n", err );

    /* dwForwarding and dwDefaultTTL come from the compartment table */
    ok( bounded( table.dwInReceives, dyn.in_recv, dyn2.in_recv ), "mismatch\n" );
    ok( bounded( table.dwInHdrErrors, dyn.in_hdr_errs, dyn2.in_hdr_errs ), "mismatch\n" );
    ok( bounded( table.dwInAddrErrors, dyn.in_addr_errs, dyn2.in_addr_errs ), "mismatch\n" );
    ok( bounded( table.dwForwDatagrams, dyn.fwd_dgrams, dyn2.fwd_dgrams ), "mismatch\n" );
    ok( bounded( table.dwInUnknownProtos, dyn.in_unk_protos, dyn2.in_unk_protos ), "mismatch\n" );
    ok( bounded( table.dwInDiscards, dyn.in_discards, dyn2.in_discards ), "mismatch\n" );
    ok( bounded( table.dwInDelivers, dyn.in_delivers, dyn2.in_delivers ), "mismatch\n" );
    ok( bounded( table.dwOutRequests, dyn.out_reqs, dyn2.out_reqs ), "mismatch\n" );
    ok( bounded( table.dwRoutingDiscards, dyn.routing_discards, dyn2.routing_discards ), "mismatch\n" );
    ok( bounded( table.dwOutDiscards, dyn.out_discards, dyn2.out_discards ), "mismatch\n" );
    ok( bounded( table.dwOutNoRoutes, dyn.out_no_routes, dyn2.out_no_routes ), "mismatch\n" );
    ok( table.dwReasmTimeout == stat.reasm_timeout, "mismatch\n" );
    ok( bounded( table.dwReasmReqds, dyn.reasm_reqds, dyn2.reasm_reqds ), "mismatch\n" );
    ok( bounded( table.dwReasmOks, dyn.reasm_oks, dyn2.reasm_oks ), "mismatch\n" );
    ok( bounded( table.dwReasmFails, dyn.reasm_fails, dyn2.reasm_fails ), "mismatch\n" );
    ok( bounded( table.dwFragOks, dyn.frag_oks, dyn2.frag_oks ), "mismatch\n" );
    ok( bounded( table.dwFragFails, dyn.frag_fails, dyn2.frag_fails ), "mismatch\n" );
    ok( bounded( table.dwFragCreates, dyn.frag_creates, dyn2.frag_creates ), "mismatch\n" );
    /* dwNumIf, dwNumAddr and dwNumRoutes come from the compartment table */

err:
    winetest_pop_context();
}

static void test_ip_unicast( int family )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ip_unicast_rw, unk[0]), FIELD_OFFSET(struct nsi_ip_unicast_rw, unk[1]),
                         sizeof(struct nsi_ip_unicast_rw) };
    struct nsi_ipv4_unicast_key *key_tbl, *key4, get_key;
    struct nsi_ipv6_unicast_key *key6;
    struct nsi_ip_unicast_rw *rw_tbl, *rw, get_rw;
    struct nsi_ip_unicast_dynamic *dyn_tbl, *dyn, get_dyn;
    struct nsi_ip_unicast_static *stat_tbl, *stat, get_stat;
    MIB_UNICASTIPADDRESS_TABLE *table;
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    DWORD err, count, i, rw_size, key_size = (family == AF_INET) ? sizeof(*key4) : sizeof(*key6);

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiAllocateAndGetTable( 1, mod, NSI_IP_UNICAST_TABLE, (void **)&key_tbl, key_size,
                                      (void **)&rw_tbl, rw_sizes[i], (void **)&dyn_tbl, sizeof(*dyn_tbl),
                                      (void **)&stat_tbl, sizeof(*stat_tbl), &count, 0 );
        if (!err) break;
    }
    ok( !err, "got %d\n", err );
    rw_size = rw_sizes[i];

    err = GetUnicastIpAddressTable( family, &table );
    ok( !err, "got %d\n", err );
    ok( table->NumEntries == count, "table entries %d count %d\n", table->NumEntries, count );

    for (i = 0; i < count; i++)
    {
        MIB_UNICASTIPADDRESS_ROW *row = table->Table + i;
        rw = (struct nsi_ip_unicast_rw *)((BYTE *)rw_tbl + i * rw_size);
        dyn = dyn_tbl + i;
        stat = stat_tbl + i;
        winetest_push_context( "%d", i );

        ok( row->Address.si_family == family, "mismatch\n" );

        if (family == AF_INET)
        {
            key4 = key_tbl + i;
            ok( !memcmp( &row->Address.Ipv4.sin_addr, &key4->addr, sizeof(struct in_addr) ), "mismatch\n" );
            ok( row->InterfaceLuid.Value == key4->luid.Value, "mismatch\n" );
        }
        else
        {
            key6 = (struct nsi_ipv6_unicast_key *)key_tbl + i;
            ok( !memcmp( &row->Address.Ipv6.sin6_addr, &key6->addr, sizeof(struct in6_addr) ), "mismatch\n" );
            ok( row->InterfaceLuid.Value == key6->luid.Value, "mismatch\n" );
        }
        ok( row->PrefixOrigin == rw->prefix_origin, "mismatch\n" );
        ok( row->SuffixOrigin == rw->suffix_origin, "mismatch\n" );
        ok( row->ValidLifetime == rw->valid_lifetime, "mismatch\n" );
        ok( row->PreferredLifetime == rw->preferred_lifetime, "mismatch\n" );
        ok( row->OnLinkPrefixLength == rw->on_link_prefix, "mismatch\n" );
        /* SkipAsSource */
        ok( row->DadState == dyn->dad_state, "mismatch\n" );
        ok( row->ScopeId.Value == dyn->scope_id, "mismatch\n" );
        ok( row->CreationTimeStamp.QuadPart == stat->creation_time, "mismatch\n" );
        winetest_pop_context();
    }

    get_key.luid.Value = ~0u;
    get_key.addr.s_addr = 0;
    err = NsiGetAllParameters( 1, &NPI_MS_IPV4_MODULEID, NSI_IP_UNICAST_TABLE, &get_key, sizeof(get_key),
                                   &get_rw, rw_size, &get_dyn, sizeof(get_dyn), &get_stat, sizeof(get_stat) );
    ok( err == ERROR_NOT_FOUND, "got %d\n", err );

    FreeMibTable( table );
    NsiFreeTable( key_tbl, rw_tbl, dyn_tbl, stat_tbl );
    winetest_pop_context();
}

static void test_ip_neighbour( int family )
{
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    DWORD err, i, count, count2, attempt;
    struct nsi_ipv4_neighbour_key *key_tbl, *key_tbl_2, *key4;
    struct nsi_ipv6_neighbour_key *key6;
    struct nsi_ip_neighbour_rw *rw_tbl, *rw;
    struct nsi_ip_neighbour_dynamic *dyn_tbl, *dyn_tbl_2, *dyn;
    MIB_IPNET_TABLE2 *table;
    DWORD key_size = (family == AF_INET) ? sizeof(struct nsi_ipv4_neighbour_key) : sizeof(struct nsi_ipv6_neighbour_key);

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    for (attempt = 0; attempt < 5; attempt++)
    {
        err = NsiAllocateAndGetTable( 1, mod, NSI_IP_NEIGHBOUR_TABLE, (void **)&key_tbl, key_size,
                                      (void **)&rw_tbl, sizeof(*rw), (void **)&dyn_tbl, sizeof(*dyn),
                                      NULL, 0, &count, 0 );
todo_wine_if( family == AF_INET6 )
        ok( !err, "got %x\n", err );
        if (err) goto err;

        err = GetIpNetTable2( family, &table );
        ok( !err, "got %x\n", err );

        err = NsiAllocateAndGetTable( 1, mod, NSI_IP_NEIGHBOUR_TABLE, (void **)&key_tbl_2, key_size,
                                      NULL, 0, (void **)&dyn_tbl_2, sizeof(*dyn),
                                      NULL, 0, &count2, 0 );
        ok( !err, "got %x\n", err );
        if (count == count2 && !memcmp( dyn_tbl, dyn_tbl_2, count * sizeof(*dyn) )) break;
        NsiFreeTable( key_tbl_2, NULL, dyn_tbl_2, NULL );
        NsiFreeTable( key_tbl, rw_tbl, dyn_tbl, NULL );
    }

    ok( count == table->NumEntries, "%d vs %d\n", count, table->NumEntries );

    for (i = 0; i < count; i++)
    {
        MIB_IPNET_ROW2 *row = table->Table + i;
        rw = rw_tbl + i;
        dyn = dyn_tbl + i;

        if (family == AF_INET)
        {
            key4 = key_tbl + i;
            ok( key4->addr.s_addr == row->Address.Ipv4.sin_addr.s_addr, "%08x vs %08x\n", key4->addr.s_addr,
                row->Address.Ipv4.sin_addr.s_addr );
            ok( key4->luid.Value == row->InterfaceLuid.Value, "%s vs %s\n", wine_dbgstr_longlong( key4->luid.Value ),
                wine_dbgstr_longlong( row->InterfaceLuid.Value ) );
            ok( key4->luid2.Value == row->InterfaceLuid.Value, "mismatch\n" );
        }
        else if (family == AF_INET6)
        {
            key6 = (struct nsi_ipv6_neighbour_key *)key_tbl + i;
            ok( !memcmp( key6->addr.s6_addr, row->Address.Ipv6.sin6_addr.s6_addr, sizeof(IN6_ADDR) ), "mismatch\n" );
            ok( key6->luid.Value == row->InterfaceLuid.Value, "mismatch\n" );
            ok( key6->luid2.Value == row->InterfaceLuid.Value, "mismatch\n" );
        }

        ok( dyn->phys_addr_len == row->PhysicalAddressLength, "mismatch\n" );
        ok( !memcmp( rw->phys_addr, row->PhysicalAddress, dyn->phys_addr_len ), "mismatch\n" );
        ok( dyn->state == row->State, "%x vs %x\n", dyn->state, row->State );
        ok( dyn->flags.is_router == row->IsRouter, "%x vs %x\n", dyn->flags.is_router, row->IsRouter );
        ok( dyn->flags.is_unreachable == row->IsUnreachable, "%x vs %x\n", dyn->flags.is_unreachable, row->IsUnreachable );
        ok( dyn->time == row->ReachabilityTime.LastReachable, "%x vs %x\n", dyn->time, row->ReachabilityTime.LastReachable );
    }

    NsiFreeTable( key_tbl_2, NULL, dyn_tbl_2, NULL );
    NsiFreeTable( key_tbl, rw_tbl, dyn_tbl, NULL );

err:
    winetest_pop_context();
}

static void test_ip_forward( int family )
{
    DWORD rw_sizes[] = { FIELD_OFFSET(struct nsi_ip_forward_rw, unk),
                         FIELD_OFFSET(struct nsi_ip_forward_rw, unk2), sizeof(struct nsi_ip_forward_rw) };
    DWORD dyn_sizes4[] = { sizeof(struct nsi_ipv4_forward_dynamic) - 3 * sizeof(DWORD),
                           sizeof(struct nsi_ipv4_forward_dynamic) };
    DWORD dyn_sizes6[] = { sizeof(struct nsi_ipv6_forward_dynamic) - 3 * sizeof(DWORD),
                           sizeof(struct nsi_ipv6_forward_dynamic) };
    DWORD *dyn_sizes = family == AF_INET ? dyn_sizes4 : dyn_sizes6;
    struct nsi_ipv4_forward_key *key_tbl, *key4;
    struct nsi_ipv6_forward_key *key6;
    struct nsi_ip_forward_rw *rw_tbl, *rw;
    struct nsi_ipv4_forward_dynamic *dyn_tbl, *dyn4;
    struct nsi_ipv6_forward_dynamic *dyn6;
    struct nsi_ip_forward_static *stat_tbl, *stat;
    MIB_IPFORWARD_TABLE2 *table;
    const NPI_MODULEID *mod = (family == AF_INET) ? &NPI_MS_IPV4_MODULEID : &NPI_MS_IPV6_MODULEID;
    DWORD key_size = (family == AF_INET) ? sizeof(*key4) : sizeof(*key6);
    DWORD err, count, i, rw_size, dyn_size;

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    for (i = 0; i < ARRAY_SIZE(rw_sizes); i++)
    {
        err = NsiAllocateAndGetTable( 1, mod, NSI_IP_FORWARD_TABLE, (void **)&key_tbl, key_size,
                                      (void **)&rw_tbl, rw_sizes[i], NULL, 0,
                                      NULL, 0, &count, 0 );
        if (!err) break;
    }
    ok( !err, "got %d\n", err );
    if (err) { winetest_pop_context(); return; }
    rw_size = rw_sizes[i];
    NsiFreeTable( key_tbl, rw_tbl, NULL, NULL );

    for (i = 0; i < ARRAY_SIZE(dyn_sizes4); i++)
    {
        err = NsiAllocateAndGetTable( 1, mod, NSI_IP_FORWARD_TABLE, (void **)&key_tbl, key_size,
                                      (void **)&rw_tbl, rw_size, (void **)&dyn_tbl, dyn_sizes[i],
                                      (void **)&stat_tbl, sizeof(*stat_tbl), &count, 0 );
        if (!err) break;
    }
    ok( !err, "got %d\n", err );
    dyn_size = dyn_sizes[i];

    err = GetIpForwardTable2( family, &table );
    ok( !err, "got %d\n", err );
    ok( table->NumEntries == count, "table entries %d count %d\n", table->NumEntries, count );

    for (i = 0; i < count; i++)
    {
        MIB_IPFORWARD_ROW2 *row = table->Table + i;
        rw = (struct nsi_ip_forward_rw *)((BYTE *)rw_tbl + i * rw_size);
        stat = stat_tbl + i;
        winetest_push_context( "%d", i );

        ok( row->DestinationPrefix.Prefix.si_family == family, "mismatch\n" );

        if (family == AF_INET)
        {
            key4 = key_tbl + i;
            dyn4 = (struct nsi_ipv4_forward_dynamic *)((BYTE *)dyn_tbl + i * dyn_size);

            ok( row->InterfaceLuid.Value == key4->luid.Value, "mismatch\n" );
            ok( row->InterfaceLuid.Value == key4->luid2.Value, "mismatch\n" );
            ok( row->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr == key4->prefix.s_addr, "mismatch\n" );
            ok( row->DestinationPrefix.Prefix.Ipv4.sin_port == 0, "mismatch\n" );
            ok( row->DestinationPrefix.PrefixLength == key4->prefix_len, "mismatch\n" );
            ok( row->NextHop.Ipv4.sin_addr.s_addr == key4->next_hop.s_addr, "mismatch\n" );
            ok( row->NextHop.Ipv4.sin_port == 0, "mismatch\n" );
            ok( row->Age == dyn4->age, "mismatch\n" );
        }
        else
        {
            key6 = (struct nsi_ipv6_forward_key *)key_tbl + i;
            dyn6 = (struct nsi_ipv6_forward_dynamic *)((BYTE *)dyn_tbl + i * dyn_size);

            ok( row->InterfaceLuid.Value == key6->luid.Value, "mismatch\n" );
            ok( row->InterfaceLuid.Value == key6->luid2.Value, "mismatch\n" );
            ok( !memcmp( &row->DestinationPrefix.Prefix.Ipv6.sin6_addr, &key6->prefix, sizeof(key6->prefix) ),
                "mismatch\n" );
            ok( row->DestinationPrefix.Prefix.Ipv6.sin6_port == 0, "mismatch\n" );
            ok( row->DestinationPrefix.Prefix.Ipv6.sin6_flowinfo == 0, "mismatch\n" );
            ok( row->DestinationPrefix.Prefix.Ipv6.sin6_scope_id == 0, "mismatch\n" );
            ok( row->DestinationPrefix.PrefixLength == key6->prefix_len, "mismatch\n" );
            ok( !memcmp( &row->NextHop.Ipv6.sin6_addr, &key6->next_hop, sizeof(key6->next_hop) ), "mismatch\n" );
            ok( row->NextHop.Ipv6.sin6_port == 0, "mismatch\n" );
            ok( row->NextHop.Ipv6.sin6_flowinfo == 0, "mismatch\n" );
            ok( row->NextHop.Ipv6.sin6_scope_id == 0, "mismatch\n" );
            ok( row->Age == dyn6->age, "mismatch\n" );
        }

        ok( row->InterfaceIndex == stat->if_index, "mismatch\n" );
        ok( row->SitePrefixLength == rw->site_prefix_len, "mismatch\n" );
        ok( row->ValidLifetime == rw->valid_lifetime, "mismatch\n" );
        ok( row->PreferredLifetime == rw->preferred_lifetime, "mismatch\n" );

        ok( row->Metric == rw->metric, "mismatch\n" );
        ok( row->Protocol == rw->protocol, "mismatch\n" );
        ok( row->Loopback == rw->loopback, "mismatch\n" );
        ok( row->AutoconfigureAddress == rw->autoconf, "mismatch\n" );
        ok( row->Publish == rw->publish, "mismatch\n" );
        ok( row->Immortal == rw->immortal, "mismatch\n" );
        ok( row->Origin == stat->origin, "mismatch\n" );

        winetest_pop_context();
    }

    FreeMibTable( table );
    NsiFreeTable( key_tbl, rw_tbl, dyn_tbl, stat_tbl );
    winetest_pop_context();
}

static void test_tcp_stats( int family )
{
    DWORD err;
    USHORT key = family;
    struct nsi_tcp_stats_dynamic dyn, dyn2;
    struct nsi_tcp_stats_static stat;
    MIB_TCPSTATS table;

    winetest_push_context( family == AF_INET ? "AF_INET" : "AF_INET6" );

    err = NsiGetAllParameters( 1, &NPI_MS_TCP_MODULEID, NSI_TCP_STATS_TABLE, &key, sizeof(key), NULL, 0,
                               &dyn, sizeof(dyn), &stat, sizeof(stat) );
    ok( !err, "got %x\n", err );

    err = GetTcpStatisticsEx( &table, family );
    ok( !err, "got %d\n", err );

    err = NsiGetAllParameters( 1, &NPI_MS_TCP_MODULEID, NSI_TCP_STATS_TABLE, &key, sizeof(key), NULL, 0,
                               &dyn2, sizeof(dyn), NULL, 0 );
    ok( !err, "got %x\n", err );

    ok( table.dwRtoAlgorithm == stat.rto_algo, "%d vs %d\n", table.dwRtoAlgorithm, stat.rto_algo );
    ok( table.dwRtoMin == stat.rto_min, "%d vs %d\n", table.dwRtoMin, stat.rto_min );
    ok( table.dwRtoMax == stat.rto_max,  "%d vs %d\n", table.dwRtoMax, stat.rto_max );
    ok( table.dwMaxConn == stat.max_conns, "%d vs %d\n", table.dwMaxConn, stat.max_conns );

    ok( unstable( table.dwActiveOpens == dyn.active_opens ), "%d vs %d\n", table.dwActiveOpens, dyn.active_opens );
    ok( unstable( table.dwPassiveOpens == dyn.passive_opens ), "%d vs %d\n", table.dwPassiveOpens, dyn.passive_opens );
    ok( bounded( table.dwAttemptFails, dyn.attempt_fails, dyn2.attempt_fails ), "%d vs [%d %d]\n",
        table.dwAttemptFails, dyn.attempt_fails, dyn2.attempt_fails );
    ok( bounded( table.dwEstabResets, dyn.est_rsts, dyn2.est_rsts ), "%d vs [%d %d]\n",
        table.dwEstabResets, dyn.est_rsts, dyn2.est_rsts );
    ok( unstable( table.dwCurrEstab == dyn.cur_est ), "%d vs %d\n", table.dwCurrEstab, dyn.cur_est );
    ok( bounded( table.dwInSegs, dyn.in_segs, dyn2.in_segs ), "%d vs [%I64d %I64d]\n",
        table.dwInSegs, dyn.in_segs, dyn2.in_segs );
    ok( bounded( table.dwOutSegs, dyn.out_segs, dyn2.out_segs ), "%d vs [%I64d %I64d]\n",
        table.dwOutSegs, dyn.out_segs, dyn2.out_segs );
    ok( bounded( table.dwRetransSegs, dyn.retrans_segs, dyn2.retrans_segs ), "%d vs [%d %d]\n",
        table.dwRetransSegs, dyn.retrans_segs, dyn2.retrans_segs );
    ok( bounded( table.dwInErrs, dyn.in_errs, dyn2.in_errs ), "%d vs [%d %d]\n",
        table.dwInErrs, dyn.in_errs, dyn2.in_errs );
    ok( bounded( table.dwOutRsts, dyn.out_rsts, dyn2.out_rsts ), "%d vs [%d %d]\n",
        table.dwOutRsts, dyn.out_rsts, dyn2.out_rsts );
    ok( unstable( table.dwNumConns == dyn.num_conns ), "%d vs %d\n", table.dwNumConns, dyn.num_conns );

    winetest_pop_context();
}

START_TEST( nsi )
{
    test_nsi_api();

    test_ndis_ifinfo();
    test_ndis_index_luid();

    test_ip_cmpt( AF_INET );
    test_ip_cmpt( AF_INET6 );
    test_ip_icmpstats( AF_INET );
    test_ip_icmpstats( AF_INET6 );
    test_ip_ipstats( AF_INET );
    test_ip_ipstats( AF_INET6 );
    test_ip_unicast( AF_INET );
    test_ip_unicast( AF_INET6 );
    test_ip_neighbour( AF_INET );
    test_ip_neighbour( AF_INET6 );
    test_ip_forward( AF_INET );
    test_ip_forward( AF_INET6 );

    test_tcp_stats( AF_INET );
    test_tcp_stats( AF_INET6 );
}
