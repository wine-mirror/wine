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

struct program_callback
{
    void (WINAPI *pfn_notify)(cl_program program, void *user_data);
    void *user_data;
};

static void CL_CALLBACK program_callback_wrapper(cl_program program, void *user_data)
{
    struct program_callback *callback = user_data;
    TRACE("(%p, %p)\n", program, user_data);
    callback->pfn_notify(program, callback->user_data);
    free(callback);
}

cl_int WINAPI wrap_clBuildProgram( cl_program program, cl_uint num_devices,
        const cl_device_id *device_list, const char *options,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data )
{
    if (pfn_notify)
    {
        struct program_callback *callback;
        cl_int ret;

        if (!(callback = malloc(sizeof(*callback))))
            return CL_OUT_OF_HOST_MEMORY;
        callback->pfn_notify = pfn_notify;
        callback->user_data = user_data;
        if ((ret = clBuildProgram( program, num_devices, device_list, options,
                program_callback_wrapper, callback )) != CL_SUCCESS)
            free( callback );
        return ret;
    }

    return clBuildProgram( program, num_devices, device_list, options, NULL, NULL );
}

struct context_callback
{
    void (WINAPI *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data);
    void *user_data;
};

static void CL_CALLBACK context_callback_wrapper(const char *errinfo,
        const void *private_info, size_t cb, void *user_data)
{
    struct context_callback *callback = user_data;
    TRACE("(%s, %p, %zu, %p)\n", debugstr_a(errinfo), private_info, cb, user_data);
    callback->pfn_notify(errinfo, private_info, cb, callback->user_data);
}

cl_context WINAPI wrap_clCreateContext( const cl_context_properties *properties,
        cl_uint num_devices, const cl_device_id *devices,
        void (WINAPI *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
        void *user_data, cl_int *errcode_ret )
{
    if (pfn_notify)
    {
        struct context_callback *callback;
        cl_context ret;

        /* FIXME: the callback structure is currently leaked */
        if (!(callback = malloc(sizeof(*callback))))
        {
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return NULL;
        }
        callback->pfn_notify = pfn_notify;
        callback->user_data = user_data;
        if (!(ret = clCreateContext( properties, num_devices, devices, context_callback_wrapper, callback, errcode_ret )))
            free( callback );
        return ret;
    }

    return clCreateContext( properties, num_devices, devices, NULL, NULL, errcode_ret );
}

cl_context WINAPI wrap_clCreateContextFromType( const cl_context_properties *properties, cl_device_type device_type,
        void (WINAPI *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
        void *user_data, cl_int *errcode_ret )
{
    if (pfn_notify)
    {
        struct context_callback *callback;
        cl_context ret;

        /* FIXME: the callback structure is currently leaked */
        if (!(callback = malloc(sizeof(*callback))))
        {
            *errcode_ret = CL_OUT_OF_HOST_MEMORY;
            return NULL;
        }
        callback->pfn_notify = pfn_notify;
        callback->user_data = user_data;
        if (!(ret = clCreateContextFromType( properties, device_type, context_callback_wrapper, callback, errcode_ret )))
            free( callback );
        return ret;
    }

    return clCreateContextFromType( properties, device_type, NULL, NULL, errcode_ret );
}

cl_int WINAPI wrap_clEnqueueNativeKernel( cl_command_queue command_queue,
        void (WINAPI *user_func)(void *),
        void *args, size_t cb_args, cl_uint num_mem_objects, const cl_mem *mem_list, const void **args_mem_loc,
        cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event )
{
    /* we have no clear way to wrap user_func */
    FIXME( "not implemented\n" );
    return CL_INVALID_OPERATION;
}

cl_int WINAPI wrap_clSetEventCallback( cl_event event, cl_int type,
        void (WINAPI *pfn_notify)(cl_event, cl_int, void *),
        void *user_data)
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

cl_int WINAPI wrap_clSetMemObjectDestructorCallback(cl_mem memobj,
        void (WINAPI *pfn_notify)(cl_mem, void *),
        void *user_data)
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

cl_int WINAPI wrap_clCompileProgram( cl_program program, cl_uint num_devices,
        const cl_device_id *device_list, const char *options, cl_uint num_input_headers,
        const cl_program *input_headers, const char **header_include_names,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data )
{
    FIXME( "not yet implemented\n" );
    return CL_INVALID_OPERATION;
}

cl_program WINAPI wrap_clLinkProgram( cl_context context, cl_uint num_devices, const cl_device_id *device_list,
        const char *options, cl_uint num_input_programs, const cl_program *input_programs,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data, cl_int *errcode_ret )
{
    FIXME( "not yet implemented\n" );
    *errcode_ret = CL_INVALID_OPERATION;
    return NULL;
}

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    *(const struct opencl_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
