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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/wdm.h"
#include "ifdef.h"
#include "netiodef.h"
#include "wine/nsi.h"
#include "wine/debug.h"
#include "nsiproxy_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nsi);

static const struct module *modules[] =
{
    &ndis_module,
    &ipv4_module,
    &ipv6_module,
    &tcp_module,
};

static const struct module_table *get_module_table( const NPI_MODULEID *id, DWORD table )
{
    const struct module_table *entry;
    int i;

    for (i = 0; i < ARRAY_SIZE(modules); i++)
        if (NmrIsEqualNpiModuleId( modules[i]->module, id ))
            for (entry = modules[i]->tables; entry->table != ~0u; entry++)
                if (entry->table == table) return entry;

    return NULL;
}

NTSTATUS nsi_enumerate_all_ex( struct nsi_enumerate_all_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );
    DWORD sizes[4] = { params->key_size, params->rw_size, params->dynamic_size, params->static_size };
    void *data[4] = { params->key_data, params->rw_data, params->dynamic_data, params->static_data };
    int i;

    if (!entry || !entry->enumerate_all)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < ARRAY_SIZE(sizes); i++)
    {
        if (!sizes[i]) data[i] = NULL;
        else if (sizes[i] != entry->sizes[i]) return STATUS_INVALID_PARAMETER;
    }

    return entry->enumerate_all( data[0], sizes[0], data[1], sizes[1], data[2], sizes[2], data[3], sizes[3], &params->count );
}

NTSTATUS nsi_get_all_parameters_ex( struct nsi_get_all_parameters_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );
    void *rw = params->rw_data;
    void *dyn = params->dynamic_data;
    void *stat = params->static_data;

    if (!entry || !entry->get_all_parameters)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    if (params->key_size != entry->sizes[0]) return STATUS_INVALID_PARAMETER;
    if (!params->rw_size) rw = NULL;
    else if (params->rw_size != entry->sizes[1]) return STATUS_INVALID_PARAMETER;
    if (!params->dynamic_size) dyn = NULL;
    else if (params->dynamic_size != entry->sizes[2]) return STATUS_INVALID_PARAMETER;
    if (!params->static_size) stat = NULL;
    else if (params->static_size != entry->sizes[3]) return STATUS_INVALID_PARAMETER;

    return entry->get_all_parameters( params->key, params->key_size, rw, params->rw_size,
                                      dyn, params->dynamic_size, stat, params->static_size );
}

NTSTATUS nsi_get_parameter_ex( struct nsi_get_parameter_ex *params )
{
    const struct module_table *entry = get_module_table( params->module, params->table );

    if (!entry || !entry->get_parameter)
    {
        WARN( "table not found\n" );
        return STATUS_INVALID_PARAMETER;
    }

    if (params->param_type > 2) return STATUS_INVALID_PARAMETER;
    if (params->key_size != entry->sizes[0]) return STATUS_INVALID_PARAMETER;
    if (params->data_offset + params->data_size > entry->sizes[params->param_type + 1])
        return STATUS_INVALID_PARAMETER;
    return entry->get_parameter( params->key, params->key_size, params->param_type,
                                 params->data, params->data_size, params->data_offset );
}
