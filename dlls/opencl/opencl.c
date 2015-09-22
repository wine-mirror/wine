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
#include "wine/port.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

#if defined(HAVE_CL_CL_H)
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.h>
#elif defined(HAVE_OPENCL_OPENCL_H)
#include <OpenCL/opencl.h>
#endif

/* TODO: Figure out how to provide GL context sharing before enabling OpenGL */
#define OPENCL_WITH_GL 0


/*---------------------------------------------------------------*/
/* Platform API */

cl_int WINAPI wine_clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms, cl_uint *num_platforms)
{
    cl_int ret;
    TRACE("(%d, %p, %p)\n", num_entries, platforms, num_platforms);
    ret = clGetPlatformIDs(num_entries, platforms, num_platforms);
    TRACE("(%d, %p, %p)=%d\n", num_entries, platforms, num_platforms, ret);
    return ret;
}

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


/*---------------------------------------------------------------*/
/* Device APIs */

cl_int WINAPI wine_clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
                                  cl_uint num_entries, cl_device_id * devices, cl_uint * num_devices)
{
    cl_int ret;
    TRACE("(%p, 0x%lx, %d, %p, %p)\n", platform, (long unsigned int)device_type, num_entries, devices, num_devices);
    ret = clGetDeviceIDs(platform, device_type, num_entries, devices, num_devices);
    TRACE("(%p, 0x%lx, %d, %p, %p)=%d\n", platform, (long unsigned int)device_type, num_entries, devices, num_devices, ret);
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


/*---------------------------------------------------------------*/
/* Context APIs  */

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

cl_int WINAPI wine_clRetainContext(cl_context context)
{
    cl_int ret;
    TRACE("(%p)\n", context);
    ret = clRetainContext(context);
    TRACE("(%p)=%d\n", context, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseContext(cl_context context)
{
    cl_int ret;
    TRACE("(%p)\n", context);
    ret = clReleaseContext(context);
    TRACE("(%p)=%d\n", context, ret);
    return ret;
}

cl_int WINAPI wine_clGetContextInfo(cl_context context, cl_context_info param_name,
                                    SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("(%p, 0x%x, %ld, %p, %p)\n", context, param_name, param_value_size, param_value, param_value_size_ret);
    ret = clGetContextInfo(context, param_name, param_value_size, param_value, param_value_size_ret);
    TRACE("(%p, 0x%x, %ld, %p, %p)=%d\n", context, param_name, param_value_size, param_value, param_value_size_ret, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Command Queue APIs */

cl_command_queue WINAPI wine_clCreateCommandQueue(cl_context context, cl_device_id device,
                                                  cl_command_queue_properties properties, cl_int * errcode_ret)
{
    cl_command_queue ret;
    TRACE("(%p, %p, 0x%lx, %p)\n", context, device, (long unsigned int)properties, errcode_ret);
    ret = clCreateCommandQueue(context, device, properties, errcode_ret);
    TRACE("(%p, %p, 0x%lx, %p)=%p\n", context, device, (long unsigned int)properties, errcode_ret, ret);
    return ret;
}

cl_int WINAPI wine_clRetainCommandQueue(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    ret = clRetainCommandQueue(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseCommandQueue(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    ret = clReleaseCommandQueue(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clGetCommandQueueInfo(cl_command_queue command_queue, cl_command_queue_info param_name,
                                         SIZE_T param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("%p, %d, %ld, %p, %p\n", command_queue, param_name, param_value_size, param_value, param_value_size_ret);
    ret = clGetCommandQueueInfo(command_queue, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clSetCommandQueueProperty(cl_command_queue command_queue, cl_command_queue_properties properties, cl_bool enable,
                                             cl_command_queue_properties * old_properties)
{
    FIXME("(%p, 0x%lx, %d, %p): deprecated\n", command_queue, (long unsigned int)properties, enable, old_properties);
    return CL_INVALID_QUEUE_PROPERTIES;
}


/*---------------------------------------------------------------*/
/* Memory Object APIs  */

cl_mem WINAPI wine_clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    ret = clCreateBuffer(context, flags, size, host_ptr, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateImage2D(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                   size_t image_width, size_t image_height, size_t image_row_pitch, void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    ret = clCreateImage2D(context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret);
    return ret;
}

cl_mem WINAPI wine_clCreateImage3D(cl_context context, cl_mem_flags flags, cl_image_format * image_format,
                                   size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch,
                                   void * host_ptr, cl_int * errcode_ret)
{
    cl_mem ret;
    TRACE("\n");
    ret = clCreateImage3D(context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainMemObject(cl_mem memobj)
{
    cl_int ret;
    TRACE("(%p)\n", memobj);
    ret = clRetainMemObject(memobj);
    TRACE("(%p)=%d\n", memobj, ret);
    return ret;
}

cl_int WINAPI wine_clReleaseMemObject(cl_mem memobj)
{
    cl_int ret;
    TRACE("(%p)\n", memobj);
    ret = clReleaseMemObject(memobj);
    TRACE("(%p)=%d\n", memobj, ret);
    return ret;
}

cl_int WINAPI wine_clGetSupportedImageFormats(cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries,
                                              cl_image_format * image_formats, cl_uint * num_image_formats)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetSupportedImageFormats(context, flags, image_type, num_entries, image_formats, num_image_formats);
    return ret;
}

cl_int WINAPI wine_clGetMemObjectInfo(cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetMemObjectInfo(memobj, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetImageInfo(cl_mem image, cl_image_info param_name, size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetImageInfo(image, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Sampler APIs  */

cl_sampler WINAPI wine_clCreateSampler(cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode,
                                       cl_filter_mode filter_mode, cl_int * errcode_ret)
{
    cl_sampler ret;
    TRACE("\n");
    ret = clCreateSampler(context, normalized_coords, addressing_mode, filter_mode, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainSampler(cl_sampler sampler)
{
    cl_int ret;
    TRACE("\n");
    ret = clRetainSampler(sampler);
    return ret;
}

cl_int WINAPI wine_clReleaseSampler(cl_sampler sampler)
{
    cl_int ret;
    TRACE("\n");
    ret = clReleaseSampler(sampler);
    return ret;
}

cl_int WINAPI wine_clGetSamplerInfo(cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size,
                                    void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetSamplerInfo(sampler, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Program Object APIs  */

cl_program WINAPI wine_clCreateProgramWithSource(cl_context context, cl_uint count, const char ** strings,
                                                 const size_t * lengths, cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    ret = clCreateProgramWithSource(context, count, strings, lengths, errcode_ret);
    return ret;
}

cl_program WINAPI wine_clCreateProgramWithBinary(cl_context context, cl_uint num_devices, const cl_device_id * device_list,
                                                 const size_t * lengths, const unsigned char ** binaries, cl_int * binary_status,
                                                 cl_int * errcode_ret)
{
    cl_program ret;
    TRACE("\n");
    ret = clCreateProgramWithBinary(context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clRetainProgram(cl_program program)
{
    cl_int ret;
    TRACE("\n");
    ret = clRetainProgram(program);
    return ret;
}

cl_int WINAPI wine_clReleaseProgram(cl_program program)
{
    cl_int ret;
    TRACE("\n");
    ret = clReleaseProgram(program);
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

cl_int WINAPI wine_clUnloadCompiler(void)
{
    cl_int ret;
    TRACE("()\n");
    ret = clUnloadCompiler();
    TRACE("()=%d\n", ret);
    return ret;
}

cl_int WINAPI wine_clGetProgramInfo(cl_program program, cl_program_info param_name,
                                    size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetProgramInfo(program, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetProgramBuildInfo(cl_program program, cl_device_id device,
                                         cl_program_build_info param_name, size_t param_value_size, void * param_value,
                                         size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetProgramBuildInfo(program, device, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Kernel Object APIs */

cl_kernel WINAPI wine_clCreateKernel(cl_program program, char * kernel_name, cl_int * errcode_ret)
{
    cl_kernel ret;
    TRACE("\n");
    ret = clCreateKernel(program, kernel_name, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                                            cl_kernel * kernels, cl_uint * num_kernels_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clCreateKernelsInProgram(program, num_kernels, kernels, num_kernels_ret);
    return ret;
}

cl_int WINAPI wine_clRetainKernel(cl_kernel kernel)
{
    cl_int ret;
    TRACE("\n");
    ret = clRetainKernel(kernel);
    return ret;
}

cl_int WINAPI wine_clReleaseKernel(cl_kernel kernel)
{
    cl_int ret;
    TRACE("\n");
    ret = clReleaseKernel(kernel);
    return ret;
}

cl_int WINAPI wine_clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, void * arg_value)
{
    cl_int ret;
    TRACE("\n");
    ret = clSetKernelArg(kernel, arg_index, arg_size, arg_value);
    return ret;
}

cl_int WINAPI wine_clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                                   size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetKernelInfo(kernel, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clGetKernelWorkGroupInfo(cl_kernel kernel, cl_device_id device,
                                            cl_kernel_work_group_info param_name, size_t param_value_size,
                                            void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetKernelWorkGroupInfo(kernel, device, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Event Object APIs  */

cl_int WINAPI wine_clWaitForEvents(cl_uint num_events, cl_event * event_list)
{
    cl_int ret;
    TRACE("\n");
    ret = clWaitForEvents(num_events, event_list);
    return ret;
}

cl_int WINAPI wine_clGetEventInfo(cl_event event, cl_event_info param_name, size_t param_value_size,
                                  void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetEventInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}

cl_int WINAPI wine_clRetainEvent(cl_event event)
{
    cl_int ret;
    TRACE("\n");
    ret = clRetainEvent(event);
    return ret;
}

cl_int WINAPI wine_clReleaseEvent(cl_event event)
{
    cl_int ret;
    TRACE("\n");
    ret = clReleaseEvent(event);
    return ret;
}


/*---------------------------------------------------------------*/
/* Profiling APIs  */

cl_int WINAPI wine_clGetEventProfilingInfo(cl_event event, cl_profiling_info param_name, size_t param_value_size,
                                           void * param_value, size_t * param_value_size_ret)
{
    cl_int ret;
    TRACE("\n");
    ret = clGetEventProfilingInfo(event, param_name, param_value_size, param_value, param_value_size_ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Flush and Finish APIs */

cl_int WINAPI wine_clFlush(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    ret = clFlush(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}

cl_int WINAPI wine_clFinish(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("(%p)\n", command_queue);
    ret = clFinish(command_queue);
    TRACE("(%p)=%d\n", command_queue, ret);
    return ret;
}


/*---------------------------------------------------------------*/
/* Enqueued Commands APIs */

cl_int WINAPI wine_clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read,
                                       size_t offset, size_t cb, void * ptr,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueReadBuffer(command_queue, buffer, blocking_read, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write,
                                        size_t offset, size_t cb, const void * ptr,
                                        cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueWriteBuffer(command_queue, buffer, blocking_write, offset, cb, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer,
                                       size_t src_offset, size_t dst_offset, size_t cb,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueCopyBuffer(command_queue, src_buffer, dst_buffer, src_offset, dst_offset, cb, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueReadImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read,
                                      const size_t * origin, const size_t * region,
                                      SIZE_T row_pitch, SIZE_T slice_pitch, void * ptr,
                                      cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("(%p, %p, %d, %p, %p, %ld, %ld, %p, %d, %p, %p)\n", command_queue, image, blocking_read,
          origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    ret = clEnqueueReadImage(command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    TRACE("(%p, %p, %d, %p, %p, %ld, %ld, %p, %d, %p, %p)=%d\n", command_queue, image, blocking_read,
          origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event, ret);
    return ret;
}

cl_int WINAPI wine_clEnqueueWriteImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write,
                                       const size_t * origin, const size_t * region,
                                       size_t input_row_pitch, size_t input_slice_pitch, const void * ptr,
                                       cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueWriteImage(command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyImage(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image,
                                      size_t * src_origin, size_t * dst_origin, size_t * region,
                                      cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueCopyImage(command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyImageToBuffer(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer,
                                              size_t * src_origin, size_t * region, size_t dst_offset,
                                              cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueCopyImageToBuffer(command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueCopyBufferToImage(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image,
                                              size_t src_offset, size_t * dst_origin, size_t * region,
                                              cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueCopyBufferToImage(command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

void * WINAPI wine_clEnqueueMapBuffer(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map,
                                      cl_map_flags map_flags, size_t offset, size_t cb,
                                      cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret)
{
    void * ret;
    TRACE("\n");
    ret = clEnqueueMapBuffer(command_queue, buffer, blocking_map, map_flags, offset, cb, num_events_in_wait_list, event_wait_list, event, errcode_ret);
    return ret;
}

void * WINAPI wine_clEnqueueMapImage(cl_command_queue command_queue, cl_mem image, cl_bool blocking_map,
                                     cl_map_flags map_flags, size_t * origin, size_t * region,
                                     size_t * image_row_pitch, size_t * image_slice_pitch,
                                     cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event, cl_int * errcode_ret)
{
    void * ret;
    TRACE("\n");
    ret = clEnqueueMapImage(command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret);
    return ret;
}

cl_int WINAPI wine_clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj, void * mapped_ptr,
                                           cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueUnmapMemObject(command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueNDRangeKernel(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim,
                                          size_t * global_work_offset, size_t * global_work_size, size_t * local_work_size,
                                          cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueNDRangeKernel(command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueTask(cl_command_queue command_queue, cl_kernel kernel,
                                 cl_uint num_events_in_wait_list, cl_event * event_wait_list, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueTask(command_queue, kernel, num_events_in_wait_list, event_wait_list, event);
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

cl_int WINAPI wine_clEnqueueMarker(cl_command_queue command_queue, cl_event * event)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueMarker(command_queue, event);
    return ret;
}

cl_int WINAPI wine_clEnqueueWaitForEvents(cl_command_queue command_queue, cl_uint num_events, cl_event * event_list)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueWaitForEvents(command_queue, num_events, event_list);
    return ret;
}

cl_int WINAPI wine_clEnqueueBarrier(cl_command_queue command_queue)
{
    cl_int ret;
    TRACE("\n");
    ret = clEnqueueBarrier(command_queue);
    return ret;
}


/*---------------------------------------------------------------*/
/* Extension function access */

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


#if OPENCL_WITH_GL
/*---------------------------------------------------------------*/
/* Khronos-approved (KHR) OpenCL extensions which have OpenGL dependencies. */

cl_mem WINAPI wine_clCreateFromGLBuffer(cl_context context, cl_mem_flags flags, cl_GLuint bufobj, int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLTexture2D(cl_context context, cl_mem_flags flags, cl_GLenum target,
                                           cl_GLint miplevel, cl_GLuint texture, cl_int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLTexture3D(cl_context context, cl_mem_flags flags, cl_GLenum target,
                                           cl_GLint miplevel, cl_GLuint texture, cl_int * errcode_ret)
{
}

cl_mem WINAPI wine_clCreateFromGLRenderbuffer(cl_context context, cl_mem_flags flags, cl_GLuint renderbuffer, cl_int * errcode_ret)
{
}

cl_int WINAPI wine_clGetGLObjectInfo(cl_mem memobj, cl_gl_object_type * gl_object_type, cl_GLuint * gl_object_name)
{
}

cl_int WINAPI wine_clGetGLTextureInfo(cl_mem memobj, cl_gl_texture_info param_name, size_t param_value_size,
                                      void * param_value, size_t * param_value_size_ret)
{
}

cl_int WINAPI wine_clEnqueueAcquireGLObjects(cl_command_queue command_queue, cl_uint num_objects, const cl_mem * mem_objects,
                                             cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
}

cl_int WINAPI wine_clEnqueueReleaseGLObjects(cl_command_queue command_queue, cl_uint num_objects, const cl_mem * mem_objects,
                                             cl_uint num_events_in_wait_list, const cl_event * event_wait_list, cl_event * event)
{
}


/*---------------------------------------------------------------*/
/* cl_khr_gl_sharing extension  */

cl_int WINAPI wine_clGetGLContextInfoKHR(const cl_context_properties * properties, cl_gl_context_info param_name,
                                         size_t param_value_size, void * param_value, size_t * param_value_size_ret)
{
}

#endif


#if 0
/*---------------------------------------------------------------*/
/* cl_khr_icd extension */

cl_int WINAPI wine_clIcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id * platforms, cl_uint * num_platforms)
{
}
#endif
