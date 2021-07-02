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

START_TEST( nsi )
{
    test_nsi_api();

    test_ndis_ifinfo();
    test_ndis_index_luid();
}
