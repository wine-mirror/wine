/*
 * capi20 Unix library
 *
 * Copyright 2002-2003 AVM Computersysteme Vertriebs GmbH
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
 *
 */

#if 0
#pragma makedep unix
#endif

#define __NO_CAPIUTILS__

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#define __user
#ifdef HAVE_LINUX_CAPI_H
# include <linux/capi.h>
#endif
#ifdef HAVE_CAPI20_H
# include <capi20.h>
#endif
#include "unixlib.h"

static NTSTATUS capi_register( void *args )
{
    struct register_params *params = args;

    return capi20_register( params->maxLogicalConnection, params->maxBDataBlocks, params->maxBDataLen,
                            (unsigned int *)params->pApplID );
}

static NTSTATUS capi_release( void *args )
{
    struct release_params *params = args;

    return capi20_release( params->ApplID );
}

static NTSTATUS capi_put_message( void *args )
{
    struct put_message_params *params = args;

    return capi20_put_message( params->ApplID, params->pCAPIMessage );
}

static NTSTATUS capi_get_message( void *args )
{
    struct get_message_params *params = args;

    /* FIXME: allocate buffers based on capi_register sizes and copy returned data */
    return capi20_get_message( params->ApplID, (unsigned char **)params->ppCAPIMessage );
}

static NTSTATUS capi_waitformessage( void *args )
{
    struct waitformessage_params *params = args;

    return capi20_waitformessage( params->ApplID, NULL );
}

static NTSTATUS capi_get_manufacturer( void *args )
{
    struct get_manufacturer_params *params = args;

    return capi20_get_manufacturer( 0, (unsigned char *)params->SzBuffer ) ? 0 : 0x1108;
}

static NTSTATUS capi_get_version( void *args )
{
    struct get_version_params *params = args;
    unsigned int version[4];

    if (!capi20_get_version( 0, (unsigned char *)version )) return 0x1108;
    *params->pCAPIMajor         = version[0];
    *params->pCAPIMinor         = version[1];
    *params->pManufacturerMajor = version[2];
    *params->pManufacturerMinor = version[3];
    return 0;
}

static NTSTATUS capi_get_serial_number( void *args )
{
    struct get_serial_number_params *params = args;

    return capi20_get_serial_number( 0, (unsigned char*)params->SzBuffer ) ? 0 : 0x1108;
}

static NTSTATUS capi_get_profile( void *args )
{
    struct get_profile_params *params = args;

    return capi20_get_profile( params->CtlrNr, params->SzBuffer );
}

static NTSTATUS capi_isinstalled( void *args )
{
    return capi20_isinstalled();
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    capi_register,
    capi_release,
    capi_put_message,
    capi_get_message,
    capi_waitformessage,
    capi_get_manufacturer,
    capi_get_version,
    capi_get_serial_number,
    capi_get_profile,
    capi_isinstalled,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );
