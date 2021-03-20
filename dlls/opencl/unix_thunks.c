/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "unix_private.h"

static cl_mem WINAPI wrap_clCreateBuffer( cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret )
{
    return clCreateBuffer( context, flags, size, host_ptr, errcode_ret );
}

static cl_command_queue WINAPI wrap_clCreateCommandQueue( cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret )
{
    return clCreateCommandQueue( context, device, properties, errcode_ret );
}

static cl_mem WINAPI wrap_clCreateImage( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, const cl_image_desc* image_desc, void* host_ptr, cl_int* errcode_ret )
{
    return clCreateImage( context, flags, image_format, image_desc, host_ptr, errcode_ret );
}

static cl_mem WINAPI wrap_clCreateImage2D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret )
{
    return clCreateImage2D( context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
}

static cl_mem WINAPI wrap_clCreateImage3D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret )
{
    return clCreateImage3D( context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
}

static cl_kernel WINAPI wrap_clCreateKernel( cl_program program, const char* kernel_name, cl_int* errcode_ret )
{
    return clCreateKernel( program, kernel_name, errcode_ret );
}

static cl_int WINAPI wrap_clCreateKernelsInProgram( cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret )
{
    return clCreateKernelsInProgram( program, num_kernels, kernels, num_kernels_ret );
}

static cl_program WINAPI wrap_clCreateProgramWithBinary( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret )
{
    return clCreateProgramWithBinary( context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
}

static cl_program WINAPI wrap_clCreateProgramWithBuiltInKernels( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* kernel_names, cl_int* errcode_ret )
{
    return clCreateProgramWithBuiltInKernels( context, num_devices, device_list, kernel_names, errcode_ret );
}

static cl_program WINAPI wrap_clCreateProgramWithSource( cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret )
{
    return clCreateProgramWithSource( context, count, strings, lengths, errcode_ret );
}

static cl_sampler WINAPI wrap_clCreateSampler( cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret )
{
    return clCreateSampler( context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
}

static cl_mem WINAPI wrap_clCreateSubBuffer( cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret )
{
    return clCreateSubBuffer( buffer, flags, buffer_create_type, buffer_create_info, errcode_ret );
}

static cl_int WINAPI wrap_clCreateSubDevices( cl_device_id in_device, const cl_device_partition_property* properties, cl_uint num_devices, cl_device_id* out_devices, cl_uint* num_devices_ret )
{
    return clCreateSubDevices( in_device, properties, num_devices, out_devices, num_devices_ret );
}

static cl_event WINAPI wrap_clCreateUserEvent( cl_context context, cl_int* errcode_ret )
{
    return clCreateUserEvent( context, errcode_ret );
}

static cl_int WINAPI wrap_clEnqueueBarrier( cl_command_queue command_queue )
{
    return clEnqueueBarrier( command_queue );
}

static cl_int WINAPI wrap_clEnqueueBarrierWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueBarrierWithWaitList( command_queue, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueCopyBuffer( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueCopyBuffer( command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueCopyBufferRect( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueCopyBufferRect( command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueCopyBufferToImage( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueCopyBufferToImage( command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueCopyImage( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueCopyImage( command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueCopyImageToBuffer( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueCopyImageToBuffer( command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueFillBuffer( cl_command_queue command_queue, cl_mem buffer, const void* pattern, size_t pattern_size, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueFillBuffer( command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueFillImage( cl_command_queue command_queue, cl_mem image, const void* fill_color, const size_t* origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueFillImage( command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event );
}

static void* WINAPI wrap_clEnqueueMapBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    return clEnqueueMapBuffer( command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

static void* WINAPI wrap_clEnqueueMapImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags, const size_t* origin, const size_t* region, size_t* image_row_pitch, size_t* image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    return clEnqueueMapImage( command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

static cl_int WINAPI wrap_clEnqueueMarker( cl_command_queue command_queue, cl_event* event )
{
    return clEnqueueMarker( command_queue, event );
}

static cl_int WINAPI wrap_clEnqueueMarkerWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueMarkerWithWaitList( command_queue, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueMigrateMemObjects( cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem* mem_objects, cl_mem_migration_flags flags, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueMigrateMemObjects( command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueNDRangeKernel( cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueNDRangeKernel( command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueReadBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueReadBuffer( command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueReadBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueReadBufferRect( command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueReadImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueReadImage( command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueTask( cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueTask( command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueUnmapMemObject( cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueUnmapMemObject( command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueWaitForEvents( cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list )
{
    return clEnqueueWaitForEvents( command_queue, num_events, event_list );
}

static cl_int WINAPI wrap_clEnqueueWriteBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueWriteBuffer( command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueWriteBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueWriteBufferRect( command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clEnqueueWriteImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    return clEnqueueWriteImage( command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

static cl_int WINAPI wrap_clFinish( cl_command_queue command_queue )
{
    return clFinish( command_queue );
}

static cl_int WINAPI wrap_clFlush( cl_command_queue command_queue )
{
    return clFlush( command_queue );
}

static cl_int WINAPI wrap_clGetCommandQueueInfo( cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetCommandQueueInfo( command_queue, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetContextInfo( cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetContextInfo( context, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetDeviceIDs( cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices )
{
    return clGetDeviceIDs( platform, device_type, num_entries, devices, num_devices );
}

static cl_int WINAPI wrap_clGetDeviceInfo( cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetDeviceInfo( device, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetEventInfo( cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetEventInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetEventProfilingInfo( cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetEventProfilingInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetImageInfo( cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetImageInfo( image, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetKernelArgInfo( cl_kernel kernel, cl_uint arg_index, cl_kernel_arg_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetKernelArgInfo( kernel, arg_index, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetKernelInfo( cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetKernelInfo( kernel, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetKernelWorkGroupInfo( cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetKernelWorkGroupInfo( kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetMemObjectInfo( cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetMemObjectInfo( memobj, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetPlatformIDs( cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms )
{
    return clGetPlatformIDs( num_entries, platforms, num_platforms );
}

static cl_int WINAPI wrap_clGetPlatformInfo( cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetPlatformInfo( platform, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetProgramBuildInfo( cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetProgramBuildInfo( program, device, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetProgramInfo( cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetProgramInfo( program, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetSamplerInfo( cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    return clGetSamplerInfo( sampler, param_name, param_value_size, param_value, param_value_size_ret );
}

static cl_int WINAPI wrap_clGetSupportedImageFormats( cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats )
{
    return clGetSupportedImageFormats( context, flags, image_type, num_entries, image_formats, num_image_formats );
}

static cl_int WINAPI wrap_clReleaseCommandQueue( cl_command_queue command_queue )
{
    return clReleaseCommandQueue( command_queue );
}

static cl_int WINAPI wrap_clReleaseContext( cl_context context )
{
    return clReleaseContext( context );
}

static cl_int WINAPI wrap_clReleaseDevice( cl_device_id device )
{
    return clReleaseDevice( device );
}

static cl_int WINAPI wrap_clReleaseEvent( cl_event event )
{
    return clReleaseEvent( event );
}

static cl_int WINAPI wrap_clReleaseKernel( cl_kernel kernel )
{
    return clReleaseKernel( kernel );
}

static cl_int WINAPI wrap_clReleaseMemObject( cl_mem memobj )
{
    return clReleaseMemObject( memobj );
}

static cl_int WINAPI wrap_clReleaseProgram( cl_program program )
{
    return clReleaseProgram( program );
}

static cl_int WINAPI wrap_clReleaseSampler( cl_sampler sampler )
{
    return clReleaseSampler( sampler );
}

static cl_int WINAPI wrap_clRetainCommandQueue( cl_command_queue command_queue )
{
    return clRetainCommandQueue( command_queue );
}

static cl_int WINAPI wrap_clRetainContext( cl_context context )
{
    return clRetainContext( context );
}

static cl_int WINAPI wrap_clRetainDevice( cl_device_id device )
{
    return clRetainDevice( device );
}

static cl_int WINAPI wrap_clRetainEvent( cl_event event )
{
    return clRetainEvent( event );
}

static cl_int WINAPI wrap_clRetainKernel( cl_kernel kernel )
{
    return clRetainKernel( kernel );
}

static cl_int WINAPI wrap_clRetainMemObject( cl_mem memobj )
{
    return clRetainMemObject( memobj );
}

static cl_int WINAPI wrap_clRetainProgram( cl_program program )
{
    return clRetainProgram( program );
}

static cl_int WINAPI wrap_clRetainSampler( cl_sampler sampler )
{
    return clRetainSampler( sampler );
}

static cl_int WINAPI wrap_clSetKernelArg( cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value )
{
    return clSetKernelArg( kernel, arg_index, arg_size, arg_value );
}

static cl_int WINAPI wrap_clSetUserEventStatus( cl_event event, cl_int execution_status )
{
    return clSetUserEventStatus( event, execution_status );
}

static cl_int WINAPI wrap_clUnloadCompiler( void  )
{
    return clUnloadCompiler();
}

static cl_int WINAPI wrap_clUnloadPlatformCompiler( cl_platform_id platform )
{
    return clUnloadPlatformCompiler( platform );
}

static cl_int WINAPI wrap_clWaitForEvents( cl_uint num_events, const cl_event* event_list )
{
    return clWaitForEvents( num_events, event_list );
}

const struct opencl_funcs funcs =
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
