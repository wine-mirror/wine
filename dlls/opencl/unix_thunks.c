/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "unix_private.h"

static NTSTATUS wrap_clCreateBuffer( void *args )
{
    struct clCreateBuffer_params *params = args;

    *params->__retval = clCreateBuffer( params->context, params->flags, params->size, params->host_ptr, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateCommandQueue( void *args )
{
    struct clCreateCommandQueue_params *params = args;

    *params->__retval = clCreateCommandQueue( params->context, params->device, params->properties, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateImage( void *args )
{
    struct clCreateImage_params *params = args;

    *params->__retval = clCreateImage( params->context, params->flags, params->image_format, params->image_desc, params->host_ptr, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateImage2D( void *args )
{
    struct clCreateImage2D_params *params = args;

    *params->__retval = clCreateImage2D( params->context, params->flags, params->image_format, params->image_width, params->image_height, params->image_row_pitch, params->host_ptr, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateImage3D( void *args )
{
    struct clCreateImage3D_params *params = args;

    *params->__retval = clCreateImage3D( params->context, params->flags, params->image_format, params->image_width, params->image_height, params->image_depth, params->image_row_pitch, params->image_slice_pitch, params->host_ptr, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateKernel( void *args )
{
    struct clCreateKernel_params *params = args;

    *params->__retval = clCreateKernel( params->program, params->kernel_name, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateKernelsInProgram( void *args )
{
    struct clCreateKernelsInProgram_params *params = args;

    return clCreateKernelsInProgram( params->program, params->num_kernels, params->kernels, params->num_kernels_ret );
}

static NTSTATUS wrap_clCreateProgramWithBinary( void *args )
{
    struct clCreateProgramWithBinary_params *params = args;

    *params->__retval = clCreateProgramWithBinary( params->context, params->num_devices, params->device_list, params->lengths, params->binaries, params->binary_status, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateProgramWithBuiltInKernels( void *args )
{
    struct clCreateProgramWithBuiltInKernels_params *params = args;

    *params->__retval = clCreateProgramWithBuiltInKernels( params->context, params->num_devices, params->device_list, params->kernel_names, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateProgramWithSource( void *args )
{
    struct clCreateProgramWithSource_params *params = args;

    *params->__retval = clCreateProgramWithSource( params->context, params->count, params->strings, params->lengths, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateSampler( void *args )
{
    struct clCreateSampler_params *params = args;

    *params->__retval = clCreateSampler( params->context, params->normalized_coords, params->addressing_mode, params->filter_mode, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateSubBuffer( void *args )
{
    struct clCreateSubBuffer_params *params = args;

    *params->__retval = clCreateSubBuffer( params->buffer, params->flags, params->buffer_create_type, params->buffer_create_info, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clCreateSubDevices( void *args )
{
    struct clCreateSubDevices_params *params = args;

    return clCreateSubDevices( params->in_device, params->properties, params->num_devices, params->out_devices, params->num_devices_ret );
}

static NTSTATUS wrap_clCreateUserEvent( void *args )
{
    struct clCreateUserEvent_params *params = args;

    *params->__retval = clCreateUserEvent( params->context, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clEnqueueBarrier( void *args )
{
    struct clEnqueueBarrier_params *params = args;

    return clEnqueueBarrier( params->command_queue );
}

static NTSTATUS wrap_clEnqueueBarrierWithWaitList( void *args )
{
    struct clEnqueueBarrierWithWaitList_params *params = args;

    return clEnqueueBarrierWithWaitList( params->command_queue, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueCopyBuffer( void *args )
{
    struct clEnqueueCopyBuffer_params *params = args;

    return clEnqueueCopyBuffer( params->command_queue, params->src_buffer, params->dst_buffer, params->src_offset, params->dst_offset, params->size, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueCopyBufferRect( void *args )
{
    struct clEnqueueCopyBufferRect_params *params = args;

    return clEnqueueCopyBufferRect( params->command_queue, params->src_buffer, params->dst_buffer, params->src_origin, params->dst_origin, params->region, params->src_row_pitch, params->src_slice_pitch, params->dst_row_pitch, params->dst_slice_pitch, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueCopyBufferToImage( void *args )
{
    struct clEnqueueCopyBufferToImage_params *params = args;

    return clEnqueueCopyBufferToImage( params->command_queue, params->src_buffer, params->dst_image, params->src_offset, params->dst_origin, params->region, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueCopyImage( void *args )
{
    struct clEnqueueCopyImage_params *params = args;

    return clEnqueueCopyImage( params->command_queue, params->src_image, params->dst_image, params->src_origin, params->dst_origin, params->region, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueCopyImageToBuffer( void *args )
{
    struct clEnqueueCopyImageToBuffer_params *params = args;

    return clEnqueueCopyImageToBuffer( params->command_queue, params->src_image, params->dst_buffer, params->src_origin, params->region, params->dst_offset, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueFillBuffer( void *args )
{
    struct clEnqueueFillBuffer_params *params = args;

    return clEnqueueFillBuffer( params->command_queue, params->buffer, params->pattern, params->pattern_size, params->offset, params->size, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueFillImage( void *args )
{
    struct clEnqueueFillImage_params *params = args;

    return clEnqueueFillImage( params->command_queue, params->image, params->fill_color, params->origin, params->region, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueMapBuffer( void *args )
{
    struct clEnqueueMapBuffer_params *params = args;

    *params->__retval = clEnqueueMapBuffer( params->command_queue, params->buffer, params->blocking_map, params->map_flags, params->offset, params->size, params->num_events_in_wait_list, params->event_wait_list, params->event, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clEnqueueMapImage( void *args )
{
    struct clEnqueueMapImage_params *params = args;

    *params->__retval = clEnqueueMapImage( params->command_queue, params->image, params->blocking_map, params->map_flags, params->origin, params->region, params->image_row_pitch, params->image_slice_pitch, params->num_events_in_wait_list, params->event_wait_list, params->event, params->errcode_ret );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_clEnqueueMarker( void *args )
{
    struct clEnqueueMarker_params *params = args;

    return clEnqueueMarker( params->command_queue, params->event );
}

static NTSTATUS wrap_clEnqueueMarkerWithWaitList( void *args )
{
    struct clEnqueueMarkerWithWaitList_params *params = args;

    return clEnqueueMarkerWithWaitList( params->command_queue, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueMigrateMemObjects( void *args )
{
    struct clEnqueueMigrateMemObjects_params *params = args;

    return clEnqueueMigrateMemObjects( params->command_queue, params->num_mem_objects, params->mem_objects, params->flags, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueNDRangeKernel( void *args )
{
    struct clEnqueueNDRangeKernel_params *params = args;

    return clEnqueueNDRangeKernel( params->command_queue, params->kernel, params->work_dim, params->global_work_offset, params->global_work_size, params->local_work_size, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueReadBuffer( void *args )
{
    struct clEnqueueReadBuffer_params *params = args;

    return clEnqueueReadBuffer( params->command_queue, params->buffer, params->blocking_read, params->offset, params->size, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueReadBufferRect( void *args )
{
    struct clEnqueueReadBufferRect_params *params = args;

    return clEnqueueReadBufferRect( params->command_queue, params->buffer, params->blocking_read, params->buffer_origin, params->host_origin, params->region, params->buffer_row_pitch, params->buffer_slice_pitch, params->host_row_pitch, params->host_slice_pitch, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueReadImage( void *args )
{
    struct clEnqueueReadImage_params *params = args;

    return clEnqueueReadImage( params->command_queue, params->image, params->blocking_read, params->origin, params->region, params->row_pitch, params->slice_pitch, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueTask( void *args )
{
    struct clEnqueueTask_params *params = args;

    return clEnqueueTask( params->command_queue, params->kernel, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueUnmapMemObject( void *args )
{
    struct clEnqueueUnmapMemObject_params *params = args;

    return clEnqueueUnmapMemObject( params->command_queue, params->memobj, params->mapped_ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueWaitForEvents( void *args )
{
    struct clEnqueueWaitForEvents_params *params = args;

    return clEnqueueWaitForEvents( params->command_queue, params->num_events, params->event_list );
}

static NTSTATUS wrap_clEnqueueWriteBuffer( void *args )
{
    struct clEnqueueWriteBuffer_params *params = args;

    return clEnqueueWriteBuffer( params->command_queue, params->buffer, params->blocking_write, params->offset, params->size, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueWriteBufferRect( void *args )
{
    struct clEnqueueWriteBufferRect_params *params = args;

    return clEnqueueWriteBufferRect( params->command_queue, params->buffer, params->blocking_write, params->buffer_origin, params->host_origin, params->region, params->buffer_row_pitch, params->buffer_slice_pitch, params->host_row_pitch, params->host_slice_pitch, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clEnqueueWriteImage( void *args )
{
    struct clEnqueueWriteImage_params *params = args;

    return clEnqueueWriteImage( params->command_queue, params->image, params->blocking_write, params->origin, params->region, params->input_row_pitch, params->input_slice_pitch, params->ptr, params->num_events_in_wait_list, params->event_wait_list, params->event );
}

static NTSTATUS wrap_clFinish( void *args )
{
    struct clFinish_params *params = args;

    return clFinish( params->command_queue );
}

static NTSTATUS wrap_clFlush( void *args )
{
    struct clFlush_params *params = args;

    return clFlush( params->command_queue );
}

static NTSTATUS wrap_clGetCommandQueueInfo( void *args )
{
    struct clGetCommandQueueInfo_params *params = args;

    return clGetCommandQueueInfo( params->command_queue, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetContextInfo( void *args )
{
    struct clGetContextInfo_params *params = args;

    return clGetContextInfo( params->context, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetDeviceIDs( void *args )
{
    struct clGetDeviceIDs_params *params = args;

    return clGetDeviceIDs( params->platform, params->device_type, params->num_entries, params->devices, params->num_devices );
}

static NTSTATUS wrap_clGetDeviceInfo( void *args )
{
    struct clGetDeviceInfo_params *params = args;

    return clGetDeviceInfo( params->device, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetEventInfo( void *args )
{
    struct clGetEventInfo_params *params = args;

    return clGetEventInfo( params->event, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetEventProfilingInfo( void *args )
{
    struct clGetEventProfilingInfo_params *params = args;

    return clGetEventProfilingInfo( params->event, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetImageInfo( void *args )
{
    struct clGetImageInfo_params *params = args;

    return clGetImageInfo( params->image, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetKernelArgInfo( void *args )
{
    struct clGetKernelArgInfo_params *params = args;

    return clGetKernelArgInfo( params->kernel, params->arg_index, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetKernelInfo( void *args )
{
    struct clGetKernelInfo_params *params = args;

    return clGetKernelInfo( params->kernel, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetKernelWorkGroupInfo( void *args )
{
    struct clGetKernelWorkGroupInfo_params *params = args;

    return clGetKernelWorkGroupInfo( params->kernel, params->device, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetMemObjectInfo( void *args )
{
    struct clGetMemObjectInfo_params *params = args;

    return clGetMemObjectInfo( params->memobj, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetPlatformIDs( void *args )
{
    struct clGetPlatformIDs_params *params = args;

    return clGetPlatformIDs( params->num_entries, params->platforms, params->num_platforms );
}

static NTSTATUS wrap_clGetPlatformInfo( void *args )
{
    struct clGetPlatformInfo_params *params = args;

    return clGetPlatformInfo( params->platform, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetProgramBuildInfo( void *args )
{
    struct clGetProgramBuildInfo_params *params = args;

    return clGetProgramBuildInfo( params->program, params->device, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetProgramInfo( void *args )
{
    struct clGetProgramInfo_params *params = args;

    return clGetProgramInfo( params->program, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetSamplerInfo( void *args )
{
    struct clGetSamplerInfo_params *params = args;

    return clGetSamplerInfo( params->sampler, params->param_name, params->param_value_size, params->param_value, params->param_value_size_ret );
}

static NTSTATUS wrap_clGetSupportedImageFormats( void *args )
{
    struct clGetSupportedImageFormats_params *params = args;

    return clGetSupportedImageFormats( params->context, params->flags, params->image_type, params->num_entries, params->image_formats, params->num_image_formats );
}

static NTSTATUS wrap_clReleaseCommandQueue( void *args )
{
    struct clReleaseCommandQueue_params *params = args;

    return clReleaseCommandQueue( params->command_queue );
}

static NTSTATUS wrap_clReleaseContext( void *args )
{
    struct clReleaseContext_params *params = args;

    return clReleaseContext( params->context );
}

static NTSTATUS wrap_clReleaseDevice( void *args )
{
    struct clReleaseDevice_params *params = args;

    return clReleaseDevice( params->device );
}

static NTSTATUS wrap_clReleaseEvent( void *args )
{
    struct clReleaseEvent_params *params = args;

    return clReleaseEvent( params->event );
}

static NTSTATUS wrap_clReleaseKernel( void *args )
{
    struct clReleaseKernel_params *params = args;

    return clReleaseKernel( params->kernel );
}

static NTSTATUS wrap_clReleaseMemObject( void *args )
{
    struct clReleaseMemObject_params *params = args;

    return clReleaseMemObject( params->memobj );
}

static NTSTATUS wrap_clReleaseProgram( void *args )
{
    struct clReleaseProgram_params *params = args;

    return clReleaseProgram( params->program );
}

static NTSTATUS wrap_clReleaseSampler( void *args )
{
    struct clReleaseSampler_params *params = args;

    return clReleaseSampler( params->sampler );
}

static NTSTATUS wrap_clRetainCommandQueue( void *args )
{
    struct clRetainCommandQueue_params *params = args;

    return clRetainCommandQueue( params->command_queue );
}

static NTSTATUS wrap_clRetainContext( void *args )
{
    struct clRetainContext_params *params = args;

    return clRetainContext( params->context );
}

static NTSTATUS wrap_clRetainDevice( void *args )
{
    struct clRetainDevice_params *params = args;

    return clRetainDevice( params->device );
}

static NTSTATUS wrap_clRetainEvent( void *args )
{
    struct clRetainEvent_params *params = args;

    return clRetainEvent( params->event );
}

static NTSTATUS wrap_clRetainKernel( void *args )
{
    struct clRetainKernel_params *params = args;

    return clRetainKernel( params->kernel );
}

static NTSTATUS wrap_clRetainMemObject( void *args )
{
    struct clRetainMemObject_params *params = args;

    return clRetainMemObject( params->memobj );
}

static NTSTATUS wrap_clRetainProgram( void *args )
{
    struct clRetainProgram_params *params = args;

    return clRetainProgram( params->program );
}

static NTSTATUS wrap_clRetainSampler( void *args )
{
    struct clRetainSampler_params *params = args;

    return clRetainSampler( params->sampler );
}

static NTSTATUS wrap_clSetKernelArg( void *args )
{
    struct clSetKernelArg_params *params = args;

    return clSetKernelArg( params->kernel, params->arg_index, params->arg_size, params->arg_value );
}

static NTSTATUS wrap_clSetUserEventStatus( void *args )
{
    struct clSetUserEventStatus_params *params = args;

    return clSetUserEventStatus( params->event, params->execution_status );
}

static NTSTATUS wrap_clUnloadCompiler( void *args )
{
    return clUnloadCompiler();
}

static NTSTATUS wrap_clUnloadPlatformCompiler( void *args )
{
    struct clUnloadPlatformCompiler_params *params = args;

    return clUnloadPlatformCompiler( params->platform );
}

static NTSTATUS wrap_clWaitForEvents( void *args )
{
    struct clWaitForEvents_params *params = args;

    return clWaitForEvents( params->num_events, params->event_list );
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    wrap_clBuildProgram,
    wrap_clCompileProgram,
    wrap_clCreateBuffer,
    wrap_clCreateCommandQueue,
    wrap_clCreateContext,
    wrap_clCreateContextFromType,
    wrap_clCreateImage,
    wrap_clCreateImage2D,
    wrap_clCreateImage3D,
    wrap_clCreateKernel,
    wrap_clCreateKernelsInProgram,
    wrap_clCreateProgramWithBinary,
    wrap_clCreateProgramWithBuiltInKernels,
    wrap_clCreateProgramWithSource,
    wrap_clCreateSampler,
    wrap_clCreateSubBuffer,
    wrap_clCreateSubDevices,
    wrap_clCreateUserEvent,
    wrap_clEnqueueBarrier,
    wrap_clEnqueueBarrierWithWaitList,
    wrap_clEnqueueCopyBuffer,
    wrap_clEnqueueCopyBufferRect,
    wrap_clEnqueueCopyBufferToImage,
    wrap_clEnqueueCopyImage,
    wrap_clEnqueueCopyImageToBuffer,
    wrap_clEnqueueFillBuffer,
    wrap_clEnqueueFillImage,
    wrap_clEnqueueMapBuffer,
    wrap_clEnqueueMapImage,
    wrap_clEnqueueMarker,
    wrap_clEnqueueMarkerWithWaitList,
    wrap_clEnqueueMigrateMemObjects,
    wrap_clEnqueueNDRangeKernel,
    wrap_clEnqueueNativeKernel,
    wrap_clEnqueueReadBuffer,
    wrap_clEnqueueReadBufferRect,
    wrap_clEnqueueReadImage,
    wrap_clEnqueueTask,
    wrap_clEnqueueUnmapMemObject,
    wrap_clEnqueueWaitForEvents,
    wrap_clEnqueueWriteBuffer,
    wrap_clEnqueueWriteBufferRect,
    wrap_clEnqueueWriteImage,
    wrap_clFinish,
    wrap_clFlush,
    wrap_clGetCommandQueueInfo,
    wrap_clGetContextInfo,
    wrap_clGetDeviceIDs,
    wrap_clGetDeviceInfo,
    wrap_clGetEventInfo,
    wrap_clGetEventProfilingInfo,
    wrap_clGetImageInfo,
    wrap_clGetKernelArgInfo,
    wrap_clGetKernelInfo,
    wrap_clGetKernelWorkGroupInfo,
    wrap_clGetMemObjectInfo,
    wrap_clGetPlatformIDs,
    wrap_clGetPlatformInfo,
    wrap_clGetProgramBuildInfo,
    wrap_clGetProgramInfo,
    wrap_clGetSamplerInfo,
    wrap_clGetSupportedImageFormats,
    wrap_clLinkProgram,
    wrap_clReleaseCommandQueue,
    wrap_clReleaseContext,
    wrap_clReleaseDevice,
    wrap_clReleaseEvent,
    wrap_clReleaseKernel,
    wrap_clReleaseMemObject,
    wrap_clReleaseProgram,
    wrap_clReleaseSampler,
    wrap_clRetainCommandQueue,
    wrap_clRetainContext,
    wrap_clRetainDevice,
    wrap_clRetainEvent,
    wrap_clRetainKernel,
    wrap_clRetainMemObject,
    wrap_clRetainProgram,
    wrap_clRetainSampler,
    wrap_clSetEventCallback,
    wrap_clSetKernelArg,
    wrap_clSetMemObjectDestructorCallback,
    wrap_clSetUserEventStatus,
    wrap_clUnloadCompiler,
    wrap_clUnloadPlatformCompiler,
    wrap_clWaitForEvents,
};
