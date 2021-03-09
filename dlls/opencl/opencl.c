/*
 * OpenCL.dll proxy for native OpenCL implementation.
 *
 * Copyright 2010 Peter Urbanec
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

#include "config.h"
#include "opencl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

cl_int WINAPI wine_clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                                     SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n", platform, param_name, param_value_size, param_value, param_value_size_ret);

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
    if (param_name == CL_PLATFORM_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else
    {
        ret = clGetPlatformInfo(platform, param_name, param_value_size, param_value, param_value_size_ret);
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n", platform, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


cl_int WINAPI wine_clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
                                   SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n",device, param_name, param_value_size, param_value, param_value_size_ret);

    /* Hide all extensions.
     * TODO: Add individual extension support as needed.
     */
    if (param_name == CL_DEVICE_EXTENSIONS)
    {
        ret = CL_INVALID_VALUE;

        if (param_value && param_value_size > 0)
        {
            char *exts = (char *) param_value;
            exts[0] = '\0';
            ret = CL_SUCCESS;
        }

        if (param_value_size_ret)
        {
            *param_value_size_ret = 1;
            ret = CL_SUCCESS;
        }
    }
    else
    {
        ret = clGetDeviceInfo(device, param_name, param_value_size, param_value, param_value_size_ret);
    }

    /* Filter out the CL_EXEC_NATIVE_KERNEL flag */
    if (param_name == CL_DEVICE_EXECUTION_CAPABILITIES)
    {
        cl_device_exec_capabilities *caps = (cl_device_exec_capabilities *) param_value;
        *caps &= ~CL_EXEC_NATIVE_KERNEL;
    }

    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n",device, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


typedef struct
{
    void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data);
    void *user_data;
} CONTEXT_CALLBACK;

static void context_fn_notify(const char *errinfo, const void *private_info, size_t cb, void *user_data)
{
    CONTEXT_CALLBACK *ccb;
    TRACE("(%s, %p, %ld, %p)\n", errinfo, private_info, (SIZE_T)cb, user_data);
    ccb = (CONTEXT_CALLBACK *) user_data;
    if(ccb->pfn_notify) ccb->pfn_notify(errinfo, private_info, cb, ccb->user_data);
    TRACE("Callback COMPLETED\n");
}

cl_context WINAPI wine_clCreateContext(const cl_context_properties * properties, cl_uint num_devices, const cl_device_id * devices,
                                       void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                       void * user_data, cl_int * errcode_ret)
{
    cl_context ret;
    CONTEXT_CALLBACK *ccb;
    TRACE("(%p, %d, %p, %p, %p, %p)\n", properties, num_devices, devices, pfn_notify, user_data, errcode_ret);
    /* FIXME: The CONTEXT_CALLBACK structure is currently leaked.
     * Pointers to callback redirectors should be remembered and free()d when the context is destroyed.
     * The problem is determining when a context is being destroyed. clReleaseContext only decrements
     * the use count for a context, its destruction can come much later and therefore there is a risk
     * that the callback could be invoked after the user_data memory has been free()d.
     */
    ccb = HeapAlloc(GetProcessHeap(), 0, sizeof(CONTEXT_CALLBACK));
    ccb->pfn_notify = pfn_notify;
    ccb->user_data = user_data;
    ret = clCreateContext(properties, num_devices, devices, context_fn_notify, ccb, errcode_ret);
    TRACE("(%p, %d, %p, %p, %p, %p (%d)))=%p\n", properties, num_devices, devices, &pfn_notify, user_data, errcode_ret, errcode_ret ? *errcode_ret : 0, ret);
    return ret;
}


cl_context WINAPI wine_clCreateContextFromType(const cl_context_properties * properties, cl_device_type device_type,
                                               void WINAPI (*pfn_notify)(const char *errinfo, const void *private_info, size_t cb, void *user_data),
                                               void * user_data, cl_int * errcode_ret)
{
    cl_context ret;
    CONTEXT_CALLBACK *ccb;
    TRACE("(%p, 0x%lx, %p, %p, %p)\n", properties, (long unsigned int)device_type, pfn_notify, user_data, errcode_ret);
    /* FIXME: The CONTEXT_CALLBACK structure is currently leaked.
     * Pointers to callback redirectors should be remembered and free()d when the context is destroyed.
     * The problem is determining when a context is being destroyed. clReleaseContext only decrements
     * the use count for a context, its destruction can come much later and therefore there is a risk
     * that the callback could be invoked after the user_data memory has been free()d.
     */
    ccb = HeapAlloc(GetProcessHeap(), 0, sizeof(CONTEXT_CALLBACK));
    ccb->pfn_notify = pfn_notify;
    ccb->user_data = user_data;
    ret = clCreateContextFromType(properties, device_type, context_fn_notify, ccb, errcode_ret);
    TRACE("(%p, 0x%lx, %p, %p, %p (%d)))=%p\n", properties, (long unsigned int)device_type, pfn_notify, user_data, errcode_ret, errcode_ret ? *errcode_ret : 0, ret);
    return ret;
}

typedef struct
{
    void WINAPI (*pfn_notify)(cl_program program, void * user_data);
    void *user_data;
} PROGRAM_CALLBACK;

static void program_fn_notify(cl_program program, void * user_data)
{
    PROGRAM_CALLBACK *pcb;
    TRACE("(%p, %p)\n", program, user_data);
    pcb = (PROGRAM_CALLBACK *) user_data;
    pcb->pfn_notify(program, pcb->user_data);
    HeapFree(GetProcessHeap(), 0, pcb);
    TRACE("Callback COMPLETED\n");
}

cl_int WINAPI wine_clBuildProgram(cl_program program, cl_uint num_devices, const cl_device_id * device_list, const char * options,
                                  void WINAPI (*pfn_notify)(cl_program program, void * user_data),
                                  void * user_data)
{
    cl_int ret;
    TRACE("\n");
    if(pfn_notify)
    {
        /* When pfn_notify is provided, clBuildProgram is asynchronous */
        PROGRAM_CALLBACK *pcb;
        pcb = HeapAlloc(GetProcessHeap(), 0, sizeof(PROGRAM_CALLBACK));
        pcb->pfn_notify = pfn_notify;
        pcb->user_data = user_data;
        ret = clBuildProgram(program, num_devices, device_list, options, program_fn_notify, pcb);
    }
    else
    {
        /* When pfn_notify is NULL, clBuildProgram is synchronous */
        ret = clBuildProgram(program, num_devices, device_list, options, NULL, user_data);
    }
    return ret;
}


cl_int WINAPI wine_clEnqueueNativeKernel(cl_command_queue command_queue,
                                         void WINAPI (*user_func)(void *args),
                                         void * args, size_t cb_args,
                                         cl_uint num_mem_objects, const cl_mem * mem_list, const void ** args_mem_loc,
                                         cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret = CL_INVALID_OPERATION;
    /* FIXME: There appears to be no obvious method for translating the ABI for user_func.
     * There is no opaque user_data structure passed, that could encapsulate the return address.
     * The OpenCL specification seems to indicate that args has an implementation specific
     * structure that cannot be used to stash away a return address for the WINAPI user_func.
     */
#if 0
    ret = clEnqueueNativeKernel(command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc,
                                 num_events_in_wait_list, event_wait_list, event);
#else
    FIXME("not supported due to user_func ABI mismatch\n");
#endif
    return ret;
}


void * WINAPI wine_clGetExtensionFunctionAddress(const char * func_name)
{
    void * ret = 0;
    TRACE("(%s)\n",func_name);
#if 0
    ret = clGetExtensionFunctionAddress(func_name);
#else
    FIXME("extensions not implemented\n");
#endif
    TRACE("(%s)=%p\n",func_name, ret);
    return ret;
}
