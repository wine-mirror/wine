/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

#include "config.h"
#include "opencl_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

cl_mem WINAPI wine_clCreateBuffer( cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %zu, %p, %p)\n", context, wine_dbgstr_longlong(flags), size, host_ptr, errcode_ret );
    return clCreateBuffer( context, flags, size, host_ptr, errcode_ret );
}

cl_command_queue WINAPI wine_clCreateCommandQueue( cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %s, %p)\n", context, device, wine_dbgstr_longlong(properties), errcode_ret );
    return clCreateCommandQueue( context, device, properties, errcode_ret );
}

cl_mem WINAPI wine_clCreateImage2D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %zu, %zu, %zu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
    return clCreateImage2D( context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
}

cl_mem WINAPI wine_clCreateImage3D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %zu, %zu, %zu, %zu, %zu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
    return clCreateImage3D( context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
}

cl_kernel WINAPI wine_clCreateKernel( cl_program program, const char* kernel_name, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %p)\n", program, kernel_name, errcode_ret );
    return clCreateKernel( program, kernel_name, errcode_ret );
}

cl_int WINAPI wine_clCreateKernelsInProgram( cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret )
{
    TRACE( "(%p, %u, %p, %p)\n", program, num_kernels, kernels, num_kernels_ret );
    return clCreateKernelsInProgram( program, num_kernels, kernels, num_kernels_ret );
}

cl_program WINAPI wine_clCreateProgramWithBinary( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p, %p, %p)\n", context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
    return clCreateProgramWithBinary( context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
}

cl_program WINAPI wine_clCreateProgramWithSource( cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p)\n", context, count, strings, lengths, errcode_ret );
    return clCreateProgramWithSource( context, count, strings, lengths, errcode_ret );
}

cl_sampler WINAPI wine_clCreateSampler( cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %u, %u, %p)\n", context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
    return clCreateSampler( context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
}

cl_int WINAPI wine_clEnqueueBarrier( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return clEnqueueBarrier( command_queue );
}

cl_int WINAPI wine_clEnqueueCopyBuffer( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %zu, %zu, %zu, %u, %p, %p)\n", command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueCopyBuffer( command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueCopyBufferToImage( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %zu, %p, %p, %u, %p, %p)\n", command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueCopyBufferToImage( command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueCopyImage( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %p, %u, %p, %p)\n", command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueCopyImage( command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueCopyImageToBuffer( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %zu, %u, %p, %p)\n", command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueCopyImageToBuffer( command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
}

void* WINAPI wine_clEnqueueMapBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %u, %s, %zu, %zu, %u, %p, %p, %p)\n", command_queue, buffer, blocking_map, wine_dbgstr_longlong(map_flags), offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    return clEnqueueMapBuffer( command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

void* WINAPI wine_clEnqueueMapImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags, const size_t* origin, const size_t* region, size_t* image_row_pitch, size_t* image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %u, %s, %p, %p, %p, %p, %u, %p, %p, %p)\n", command_queue, image, blocking_map, wine_dbgstr_longlong(map_flags), origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    return clEnqueueMapImage( command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

cl_int WINAPI wine_clEnqueueMarker( cl_command_queue command_queue, cl_event* event )
{
    TRACE( "(%p, %p)\n", command_queue, event );
    return clEnqueueMarker( command_queue, event );
}

cl_int WINAPI wine_clEnqueueNDRangeKernel( cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %p, %u, %p, %p)\n", command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueNDRangeKernel( command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueReadBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %zu, %zu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueReadBuffer( command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueReadImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %zu, %zu, %p, %u, %p, %p)\n", command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueReadImage( command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueTask( cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p)\n", command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueTask( command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueUnmapMemObject( cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %u, %p, %p)\n", command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueUnmapMemObject( command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueWaitForEvents( cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list )
{
    TRACE( "(%p, %u, %p)\n", command_queue, num_events, event_list );
    return clEnqueueWaitForEvents( command_queue, num_events, event_list );
}

cl_int WINAPI wine_clEnqueueWriteBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %zu, %zu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueWriteBuffer( command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clEnqueueWriteImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %zu, %zu, %p, %u, %p, %p)\n", command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return clEnqueueWriteImage( command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI wine_clFinish( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return clFinish( command_queue );
}

cl_int WINAPI wine_clFlush( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return clFlush( command_queue );
}

cl_int WINAPI wine_clGetCommandQueueInfo( cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", command_queue, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetCommandQueueInfo( command_queue, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetContextInfo( cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", context, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetContextInfo( context, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetDeviceIDs( cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices )
{
    TRACE( "(%p, %s, %u, %p, %p)\n", platform, wine_dbgstr_longlong(device_type), num_entries, devices, num_devices );
    return clGetDeviceIDs( platform, device_type, num_entries, devices, num_devices );
}

cl_int WINAPI wine_clGetEventInfo( cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetEventInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetEventProfilingInfo( cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetEventProfilingInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetImageInfo( cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", image, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetImageInfo( image, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetKernelInfo( cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", kernel, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetKernelInfo( kernel, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetKernelWorkGroupInfo( cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %p, %u, %zu, %p, %p)\n", kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetKernelWorkGroupInfo( kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetMemObjectInfo( cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", memobj, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetMemObjectInfo( memobj, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetPlatformIDs( cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms )
{
    TRACE( "(%u, %p, %p)\n", num_entries, platforms, num_platforms );
    return clGetPlatformIDs( num_entries, platforms, num_platforms );
}

cl_int WINAPI wine_clGetProgramBuildInfo( cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %p, %u, %zu, %p, %p)\n", program, device, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetProgramBuildInfo( program, device, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetProgramInfo( cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", program, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetProgramInfo( program, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetSamplerInfo( cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %zu, %p, %p)\n", sampler, param_name, param_value_size, param_value, param_value_size_ret );
    return clGetSamplerInfo( sampler, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI wine_clGetSupportedImageFormats( cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats )
{
    TRACE( "(%p, %s, %u, %u, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_type, num_entries, image_formats, num_image_formats );
    return clGetSupportedImageFormats( context, flags, image_type, num_entries, image_formats, num_image_formats );
}

cl_int WINAPI wine_clReleaseCommandQueue( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return clReleaseCommandQueue( command_queue );
}

cl_int WINAPI wine_clReleaseContext( cl_context context )
{
    TRACE( "(%p)\n", context );
    return clReleaseContext( context );
}

cl_int WINAPI wine_clReleaseEvent( cl_event event )
{
    TRACE( "(%p)\n", event );
    return clReleaseEvent( event );
}

cl_int WINAPI wine_clReleaseKernel( cl_kernel kernel )
{
    TRACE( "(%p)\n", kernel );
    return clReleaseKernel( kernel );
}

cl_int WINAPI wine_clReleaseMemObject( cl_mem memobj )
{
    TRACE( "(%p)\n", memobj );
    return clReleaseMemObject( memobj );
}

cl_int WINAPI wine_clReleaseProgram( cl_program program )
{
    TRACE( "(%p)\n", program );
    return clReleaseProgram( program );
}

cl_int WINAPI wine_clReleaseSampler( cl_sampler sampler )
{
    TRACE( "(%p)\n", sampler );
    return clReleaseSampler( sampler );
}

cl_int WINAPI wine_clRetainCommandQueue( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return clRetainCommandQueue( command_queue );
}

cl_int WINAPI wine_clRetainContext( cl_context context )
{
    TRACE( "(%p)\n", context );
    return clRetainContext( context );
}

cl_int WINAPI wine_clRetainEvent( cl_event event )
{
    TRACE( "(%p)\n", event );
    return clRetainEvent( event );
}

cl_int WINAPI wine_clRetainKernel( cl_kernel kernel )
{
    TRACE( "(%p)\n", kernel );
    return clRetainKernel( kernel );
}

cl_int WINAPI wine_clRetainMemObject( cl_mem memobj )
{
    TRACE( "(%p)\n", memobj );
    return clRetainMemObject( memobj );
}

cl_int WINAPI wine_clRetainProgram( cl_program program )
{
    TRACE( "(%p)\n", program );
    return clRetainProgram( program );
}

cl_int WINAPI wine_clRetainSampler( cl_sampler sampler )
{
    TRACE( "(%p)\n", sampler );
    return clRetainSampler( sampler );
}

cl_int WINAPI wine_clSetCommandQueueProperty( cl_command_queue command_queue, cl_command_queue_properties properties, cl_bool enable, cl_command_queue_properties* old_properties )
{
    TRACE( "(%p, %s, %u, %p)\n", command_queue, wine_dbgstr_longlong(properties), enable, old_properties );
    return clSetCommandQueueProperty( command_queue, properties, enable, old_properties );
}

cl_int WINAPI wine_clSetKernelArg( cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value )
{
    TRACE( "(%p, %u, %zu, %p)\n", kernel, arg_index, arg_size, arg_value );
    return clSetKernelArg( kernel, arg_index, arg_size, arg_value );
}

cl_int WINAPI wine_clUnloadCompiler( void  )
{
    TRACE( "()\n" );
    return clUnloadCompiler();
}

cl_int WINAPI wine_clWaitForEvents( cl_uint num_events, const cl_event* event_list )
{
    TRACE( "(%u, %p)\n", num_events, event_list );
    return clWaitForEvents( num_events, event_list );
}
