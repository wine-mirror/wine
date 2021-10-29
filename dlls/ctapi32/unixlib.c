/*
 * WINE ct-api wrapper
 *
 * Copyright 2007 Christian Eggers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "unixlib.h"

static IS8 (*pCT_init)(IU16 ctn, IU16 pn);
static IS8 (*pCT_data)(IU16 ctn, IU8 *dad, IU8 *sad, IU16 lenc, IU8 *command, IU16 *lenr, IU8 *response);
static IS8 (*pCT_close)(IU16 ctn);

static void *ctapi_handle;

static NTSTATUS attach( void *args )
{
    struct attach_params *params = args;

    if (!(ctapi_handle = dlopen( params->libname, RTLD_NOW ))) return STATUS_DLL_NOT_FOUND;

#define LOAD_FUNCPTR(f) if((p##f = dlsym(ctapi_handle, #f)) == NULL) return STATUS_ENTRYPOINT_NOT_FOUND
    LOAD_FUNCPTR(CT_init);
    LOAD_FUNCPTR(CT_data);
    LOAD_FUNCPTR(CT_close);
#undef LOAD_FUNCPTR
    return STATUS_SUCCESS;
}

static NTSTATUS detach( void *args )
{
    dlclose( ctapi_handle );
    return STATUS_SUCCESS;
}

static NTSTATUS ct_init( void *args )
{
    struct ct_init_params *params = args;

    return pCT_init(params->ctn, params->pn);
}

static NTSTATUS ct_data( void *args )
{
    struct ct_data_params *params = args;

    return pCT_data(params->ctn, params->dad, params->sad, params->lenc,
                    params->command, params->lenr, params->response);
}

static NTSTATUS ct_close( void *args )
{
    struct ct_close_params *params = args;

    return pCT_close(params->ctn);
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    attach,
    detach,
    ct_init,
    ct_data,
    ct_close,
};
