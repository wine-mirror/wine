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
    const char *libname = args;

    if (!(ctapi_handle = dlopen( libname, RTLD_NOW ))) return STATUS_DLL_NOT_FOUND;

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
    const struct ct_init_params *params = args;

    return pCT_init(params->ctn, params->pn);
}

static NTSTATUS ct_data( void *args )
{
    const struct ct_data_params *params = args;

    return pCT_data(params->ctn, params->dad, params->sad, params->lenc,
                    params->command, params->lenr, params->response);
}

static NTSTATUS ct_close( void *args )
{
    const struct ct_close_params *params = args;

    return pCT_close(params->ctn);
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    attach,
    detach,
    ct_init,
    ct_data,
    ct_close,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_ct_data( void *args )
{
    struct
    {
        IU16  ctn;
        PTR32 dad;
        PTR32 sad;
        IU16  lenc;
        PTR32 command;
        PTR32 lenr;
        PTR32 response;
    } const *params32 = args;

    struct ct_data_params params =
    {
        params32->ctn,
        ULongToPtr(params32->dad),
        ULongToPtr(params32->sad),
        params32->lenc,
        ULongToPtr(params32->command),
        ULongToPtr(params32->lenr),
        ULongToPtr(params32->response)
    };

    return ct_data( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    attach,
    detach,
    ct_init,
    wow64_ct_data,
    ct_close,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif  /* _WIN64 */
