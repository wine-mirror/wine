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

#ifndef __WINE_UNIX_PRIVATE_H
#define __WINE_UNIX_PRIVATE_H

#include <stdarg.h>
#include <stdint.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/debug.h"

#define CL_SILENCE_DEPRECATION
#if defined(HAVE_CL_CL_H)
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#elif defined(HAVE_OPENCL_OPENCL_H)
#include <OpenCL/opencl.h>
#endif

#include "unixlib.h"

cl_int WINAPI wrap_clBuildProgram( cl_program program, cl_uint num_devices,
        const cl_device_id *device_list, const char *options,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data ) DECLSPEC_HIDDEN;

cl_context WINAPI wrap_clCreateContext( const cl_context_properties *properties,
        cl_uint num_devices, const cl_device_id *devices,
        void (WINAPI *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
        void *user_data, cl_int *errcode_ret ) DECLSPEC_HIDDEN;

cl_context WINAPI wrap_clCreateContextFromType( const cl_context_properties *properties, cl_device_type device_type,
        void (WINAPI *pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
        void *user_data, cl_int *errcode_ret ) DECLSPEC_HIDDEN;

cl_int WINAPI wrap_clEnqueueNativeKernel( cl_command_queue command_queue,
        void (WINAPI *user_func)(void *),
        void *args, size_t cb_args, cl_uint num_mem_objects, const cl_mem *mem_list, const void **args_mem_loc,
        cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event ) DECLSPEC_HIDDEN;

cl_int WINAPI wrap_clSetEventCallback( cl_event event, cl_int type,
        void (WINAPI *pfn_notify)(cl_event, cl_int, void *),
        void *user_data) DECLSPEC_HIDDEN;

cl_int WINAPI wrap_clSetMemObjectDestructorCallback(cl_mem memobj,
        void (WINAPI *pfn_notify)(cl_mem, void *),
        void *user_data) DECLSPEC_HIDDEN;

cl_int WINAPI wrap_clCompileProgram( cl_program program, cl_uint num_devices,
        const cl_device_id *device_list, const char *options, cl_uint num_input_headers,
        const cl_program *input_headers, const char **header_include_names,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data ) DECLSPEC_HIDDEN;

cl_program WINAPI wrap_clLinkProgram( cl_context context, cl_uint num_devices, const cl_device_id *device_list,
        const char *options, cl_uint num_input_programs, const cl_program *input_programs,
        void (WINAPI *pfn_notify)(cl_program program, void *user_data),
        void *user_data, cl_int *errcode_ret ) DECLSPEC_HIDDEN;

extern const struct opencl_funcs funcs;

#endif
