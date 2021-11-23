/*
 * Copyright 2021 Zebediah Figura
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
#include <stdlib.h>
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

NTSTATUS wrap_clBuildProgram( void *args )
{
    struct clBuildProgram_params *params = args;

    if (params->pfn_notify) FIXME( "notify callback not supported\n" );
    return clBuildProgram( params->program, params->num_devices, params->device_list,
                           params->options, NULL, NULL );
}

NTSTATUS wrap_clCreateContext( void *args )
{
    struct clCreateContext_params *params = args;

    if (params->pfn_notify) FIXME( "notify callback not supported\n" );
    *params->__retval = clCreateContext( params->properties, params->num_devices, params->devices,
                                         NULL, NULL, params->errcode_ret );
    return STATUS_SUCCESS;
}

NTSTATUS wrap_clCreateContextFromType( void *args )
{
    struct clCreateContextFromType_params *params = args;

    if (params->pfn_notify) FIXME( "notify callback not supported\n" );
    *params->__retval = clCreateContextFromType( params->properties, params->device_type,
                                                 NULL, NULL, params->errcode_ret );
    return STATUS_SUCCESS;
}

NTSTATUS wrap_clEnqueueNativeKernel( void *args )
{
    /* we have no clear way to wrap user_func */
    FIXME( "not implemented\n" );
    return CL_INVALID_OPERATION;
}

NTSTATUS wrap_clSetEventCallback( void *args )
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

NTSTATUS wrap_clSetMemObjectDestructorCallback( void *args )
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

NTSTATUS wrap_clCompileProgram( void *args )
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

NTSTATUS wrap_clLinkProgram( void *args )
{
    struct clLinkProgram_params *params = args;

    FIXME( "not yet implemented\n" );
    *params->errcode_ret = CL_INVALID_OPERATION;
    return STATUS_SUCCESS;
}
