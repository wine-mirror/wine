/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

#include "opencl_private.h"
#include "opencl_types.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

cl_int WINAPI clBuildProgram( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data )
{
    TRACE( "(%p, %u, %p, %p, %p, %p)\n", program, num_devices, device_list, options, pfn_notify, user_data );
    return opencl_funcs->pclBuildProgram( program, num_devices, device_list, options, pfn_notify, user_data );
}

cl_int WINAPI clCompileProgram( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_headers, const cl_program* input_headers, const char** header_include_names, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data )
{
    TRACE( "(%p, %u, %p, %p, %u, %p, %p, %p, %p)\n", program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data );
    return opencl_funcs->pclCompileProgram( program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data );
}

cl_mem WINAPI clCreateBuffer( cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), size, host_ptr, errcode_ret );
    return opencl_funcs->pclCreateBuffer( context, flags, size, host_ptr, errcode_ret );
}

cl_command_queue WINAPI clCreateCommandQueue( cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %s, %p)\n", context, device, wine_dbgstr_longlong(properties), errcode_ret );
    return opencl_funcs->pclCreateCommandQueue( context, device, properties, errcode_ret );
}

cl_context WINAPI clCreateContext( const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p, %p)\n", properties, num_devices, devices, pfn_notify, user_data, errcode_ret );
    return opencl_funcs->pclCreateContext( properties, num_devices, devices, pfn_notify, user_data, errcode_ret );
}

cl_context WINAPI clCreateContextFromType( const cl_context_properties* properties, cl_device_type device_type, void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %p, %p)\n", properties, wine_dbgstr_longlong(device_type), pfn_notify, user_data, errcode_ret );
    return opencl_funcs->pclCreateContextFromType( properties, device_type, pfn_notify, user_data, errcode_ret );
}

cl_mem WINAPI clCreateImage( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, const cl_image_desc* image_desc, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %p, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_desc, host_ptr, errcode_ret );
    return opencl_funcs->pclCreateImage( context, flags, image_format, image_desc, host_ptr, errcode_ret );
}

cl_mem WINAPI clCreateImage2D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %Iu, %Iu, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
    return opencl_funcs->pclCreateImage2D( context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
}

cl_mem WINAPI clCreateImage3D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %p, %Iu, %Iu, %Iu, %Iu, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
    return opencl_funcs->pclCreateImage3D( context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
}

cl_kernel WINAPI clCreateKernel( cl_program program, const char* kernel_name, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %p)\n", program, kernel_name, errcode_ret );
    return opencl_funcs->pclCreateKernel( program, kernel_name, errcode_ret );
}

cl_int WINAPI clCreateKernelsInProgram( cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret )
{
    TRACE( "(%p, %u, %p, %p)\n", program, num_kernels, kernels, num_kernels_ret );
    return opencl_funcs->pclCreateKernelsInProgram( program, num_kernels, kernels, num_kernels_ret );
}

cl_program WINAPI clCreateProgramWithBinary( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p, %p, %p)\n", context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
    return opencl_funcs->pclCreateProgramWithBinary( context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
}

cl_program WINAPI clCreateProgramWithBuiltInKernels( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* kernel_names, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p)\n", context, num_devices, device_list, kernel_names, errcode_ret );
    return opencl_funcs->pclCreateProgramWithBuiltInKernels( context, num_devices, device_list, kernel_names, errcode_ret );
}

cl_program WINAPI clCreateProgramWithSource( cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %p)\n", context, count, strings, lengths, errcode_ret );
    return opencl_funcs->pclCreateProgramWithSource( context, count, strings, lengths, errcode_ret );
}

cl_sampler WINAPI clCreateSampler( cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %u, %u, %p)\n", context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
    return opencl_funcs->pclCreateSampler( context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
}

cl_mem WINAPI clCreateSubBuffer( cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret )
{
    TRACE( "(%p, %s, %u, %p, %p)\n", buffer, wine_dbgstr_longlong(flags), buffer_create_type, buffer_create_info, errcode_ret );
    return opencl_funcs->pclCreateSubBuffer( buffer, flags, buffer_create_type, buffer_create_info, errcode_ret );
}

cl_int WINAPI clCreateSubDevices( cl_device_id in_device, const cl_device_partition_property* properties, cl_uint num_devices, cl_device_id* out_devices, cl_uint* num_devices_ret )
{
    TRACE( "(%p, %p, %u, %p, %p)\n", in_device, properties, num_devices, out_devices, num_devices_ret );
    return opencl_funcs->pclCreateSubDevices( in_device, properties, num_devices, out_devices, num_devices_ret );
}

cl_event WINAPI clCreateUserEvent( cl_context context, cl_int* errcode_ret )
{
    TRACE( "(%p, %p)\n", context, errcode_ret );
    return opencl_funcs->pclCreateUserEvent( context, errcode_ret );
}

cl_int WINAPI clEnqueueBarrier( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return opencl_funcs->pclEnqueueBarrier( command_queue );
}

cl_int WINAPI clEnqueueBarrierWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %u, %p, %p)\n", command_queue, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueBarrierWithWaitList( command_queue, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueCopyBuffer( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueCopyBuffer( command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueCopyBufferRect( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueCopyBufferRect( command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueCopyBufferToImage( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %Iu, %p, %p, %u, %p, %p)\n", command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueCopyBufferToImage( command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueCopyImage( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %p, %u, %p, %p)\n", command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueCopyImage( command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueCopyImageToBuffer( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %Iu, %u, %p, %p)\n", command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueCopyImageToBuffer( command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueFillBuffer( cl_command_queue command_queue, cl_mem buffer, const void* pattern, size_t pattern_size, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueFillBuffer( command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueFillImage( cl_command_queue command_queue, cl_mem image, const void* fill_color, const size_t* origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %p, %p, %u, %p, %p)\n", command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueFillImage( command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event );
}

void* WINAPI clEnqueueMapBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %u, %s, %Iu, %Iu, %u, %p, %p, %p)\n", command_queue, buffer, blocking_map, wine_dbgstr_longlong(map_flags), offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    return opencl_funcs->pclEnqueueMapBuffer( command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

void* WINAPI clEnqueueMapImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags, const size_t* origin, const size_t* region, size_t* image_row_pitch, size_t* image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    TRACE( "(%p, %p, %u, %s, %p, %p, %p, %p, %u, %p, %p, %p)\n", command_queue, image, blocking_map, wine_dbgstr_longlong(map_flags), origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    return opencl_funcs->pclEnqueueMapImage( command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
}

cl_int WINAPI clEnqueueMarker( cl_command_queue command_queue, cl_event* event )
{
    TRACE( "(%p, %p)\n", command_queue, event );
    return opencl_funcs->pclEnqueueMarker( command_queue, event );
}

cl_int WINAPI clEnqueueMarkerWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %u, %p, %p)\n", command_queue, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueMarkerWithWaitList( command_queue, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueMigrateMemObjects( cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem* mem_objects, cl_mem_migration_flags flags, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %u, %p, %s, %u, %p, %p)\n", command_queue, num_mem_objects, mem_objects, wine_dbgstr_longlong(flags), num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueMigrateMemObjects( command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueNDRangeKernel( cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %p, %u, %p, %p)\n", command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueNDRangeKernel( command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueNativeKernel( cl_command_queue command_queue, void (WINAPI* user_func)(void*), void* args, size_t cb_args, cl_uint num_mem_objects, const cl_mem* mem_list, const void** args_mem_loc, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %Iu, %u, %p, %p, %u, %p, %p)\n", command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueNativeKernel( command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueReadBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueReadBuffer( command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueReadBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueReadBufferRect( command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueReadImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueReadImage( command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueTask( cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p)\n", command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueTask( command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueUnmapMemObject( cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %p, %u, %p, %p)\n", command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueUnmapMemObject( command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueWaitForEvents( cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list )
{
    TRACE( "(%p, %u, %p)\n", command_queue, num_events, event_list );
    return opencl_funcs->pclEnqueueWaitForEvents( command_queue, num_events, event_list );
}

cl_int WINAPI clEnqueueWriteBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueWriteBuffer( command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueWriteBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueWriteBufferRect( command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clEnqueueWriteImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    TRACE( "(%p, %p, %u, %p, %p, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return opencl_funcs->pclEnqueueWriteImage( command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
}

cl_int WINAPI clFinish( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return opencl_funcs->pclFinish( command_queue );
}

cl_int WINAPI clFlush( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return opencl_funcs->pclFlush( command_queue );
}

cl_int WINAPI clGetCommandQueueInfo( cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", command_queue, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetCommandQueueInfo( command_queue, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetContextInfo( cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", context, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetContextInfo( context, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetDeviceIDs( cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices )
{
    TRACE( "(%p, %s, %u, %p, %p)\n", platform, wine_dbgstr_longlong(device_type), num_entries, devices, num_devices );
    return opencl_funcs->pclGetDeviceIDs( platform, device_type, num_entries, devices, num_devices );
}

cl_int WINAPI clGetEventInfo( cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetEventInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetEventProfilingInfo( cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetEventProfilingInfo( event, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetImageInfo( cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", image, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetImageInfo( image, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetKernelArgInfo( cl_kernel kernel, cl_uint arg_index, cl_kernel_arg_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %u, %Iu, %p, %p)\n", kernel, arg_index, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetKernelArgInfo( kernel, arg_index, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetKernelInfo( cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", kernel, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetKernelInfo( kernel, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetKernelWorkGroupInfo( cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %p, %u, %Iu, %p, %p)\n", kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetKernelWorkGroupInfo( kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetMemObjectInfo( cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", memobj, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetMemObjectInfo( memobj, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetPlatformIDs( cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms )
{
    TRACE( "(%u, %p, %p)\n", num_entries, platforms, num_platforms );
    return opencl_funcs->pclGetPlatformIDs( num_entries, platforms, num_platforms );
}

cl_int WINAPI clGetProgramBuildInfo( cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %p, %u, %Iu, %p, %p)\n", program, device, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetProgramBuildInfo( program, device, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetProgramInfo( cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", program, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetProgramInfo( program, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetSamplerInfo( cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    TRACE( "(%p, %u, %Iu, %p, %p)\n", sampler, param_name, param_value_size, param_value, param_value_size_ret );
    return opencl_funcs->pclGetSamplerInfo( sampler, param_name, param_value_size, param_value, param_value_size_ret );
}

cl_int WINAPI clGetSupportedImageFormats( cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats )
{
    TRACE( "(%p, %s, %u, %u, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_type, num_entries, image_formats, num_image_formats );
    return opencl_funcs->pclGetSupportedImageFormats( context, flags, image_type, num_entries, image_formats, num_image_formats );
}

cl_program WINAPI clLinkProgram( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_programs, const cl_program* input_programs, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data, cl_int* errcode_ret )
{
    TRACE( "(%p, %u, %p, %p, %u, %p, %p, %p, %p)\n", context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret );
    return opencl_funcs->pclLinkProgram( context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret );
}

cl_int WINAPI clReleaseCommandQueue( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return opencl_funcs->pclReleaseCommandQueue( command_queue );
}

cl_int WINAPI clReleaseContext( cl_context context )
{
    TRACE( "(%p)\n", context );
    return opencl_funcs->pclReleaseContext( context );
}

cl_int WINAPI clReleaseDevice( cl_device_id device )
{
    TRACE( "(%p)\n", device );
    return opencl_funcs->pclReleaseDevice( device );
}

cl_int WINAPI clReleaseEvent( cl_event event )
{
    TRACE( "(%p)\n", event );
    return opencl_funcs->pclReleaseEvent( event );
}

cl_int WINAPI clReleaseKernel( cl_kernel kernel )
{
    TRACE( "(%p)\n", kernel );
    return opencl_funcs->pclReleaseKernel( kernel );
}

cl_int WINAPI clReleaseMemObject( cl_mem memobj )
{
    TRACE( "(%p)\n", memobj );
    return opencl_funcs->pclReleaseMemObject( memobj );
}

cl_int WINAPI clReleaseProgram( cl_program program )
{
    TRACE( "(%p)\n", program );
    return opencl_funcs->pclReleaseProgram( program );
}

cl_int WINAPI clReleaseSampler( cl_sampler sampler )
{
    TRACE( "(%p)\n", sampler );
    return opencl_funcs->pclReleaseSampler( sampler );
}

cl_int WINAPI clRetainCommandQueue( cl_command_queue command_queue )
{
    TRACE( "(%p)\n", command_queue );
    return opencl_funcs->pclRetainCommandQueue( command_queue );
}

cl_int WINAPI clRetainContext( cl_context context )
{
    TRACE( "(%p)\n", context );
    return opencl_funcs->pclRetainContext( context );
}

cl_int WINAPI clRetainDevice( cl_device_id device )
{
    TRACE( "(%p)\n", device );
    return opencl_funcs->pclRetainDevice( device );
}

cl_int WINAPI clRetainEvent( cl_event event )
{
    TRACE( "(%p)\n", event );
    return opencl_funcs->pclRetainEvent( event );
}

cl_int WINAPI clRetainKernel( cl_kernel kernel )
{
    TRACE( "(%p)\n", kernel );
    return opencl_funcs->pclRetainKernel( kernel );
}

cl_int WINAPI clRetainMemObject( cl_mem memobj )
{
    TRACE( "(%p)\n", memobj );
    return opencl_funcs->pclRetainMemObject( memobj );
}

cl_int WINAPI clRetainProgram( cl_program program )
{
    TRACE( "(%p)\n", program );
    return opencl_funcs->pclRetainProgram( program );
}

cl_int WINAPI clRetainSampler( cl_sampler sampler )
{
    TRACE( "(%p)\n", sampler );
    return opencl_funcs->pclRetainSampler( sampler );
}

cl_int WINAPI clSetEventCallback( cl_event event, cl_int command_exec_callback_type, void (WINAPI* pfn_notify)(cl_event event, cl_int event_command_status, void *user_data), void* user_data )
{
    TRACE( "(%p, %d, %p, %p)\n", event, command_exec_callback_type, pfn_notify, user_data );
    return opencl_funcs->pclSetEventCallback( event, command_exec_callback_type, pfn_notify, user_data );
}

cl_int WINAPI clSetKernelArg( cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value )
{
    TRACE( "(%p, %u, %Iu, %p)\n", kernel, arg_index, arg_size, arg_value );
    return opencl_funcs->pclSetKernelArg( kernel, arg_index, arg_size, arg_value );
}

cl_int WINAPI clSetMemObjectDestructorCallback( cl_mem memobj, void (WINAPI* pfn_notify)(cl_mem memobj, void* user_data), void* user_data )
{
    TRACE( "(%p, %p, %p)\n", memobj, pfn_notify, user_data );
    return opencl_funcs->pclSetMemObjectDestructorCallback( memobj, pfn_notify, user_data );
}

cl_int WINAPI clSetUserEventStatus( cl_event event, cl_int execution_status )
{
    TRACE( "(%p, %d)\n", event, execution_status );
    return opencl_funcs->pclSetUserEventStatus( event, execution_status );
}

cl_int WINAPI clUnloadCompiler( void  )
{
    TRACE( "()\n" );
    return opencl_funcs->pclUnloadCompiler();
}

cl_int WINAPI clUnloadPlatformCompiler( cl_platform_id platform )
{
    TRACE( "(%p)\n", platform );
    return opencl_funcs->pclUnloadPlatformCompiler( platform );
}

cl_int WINAPI clWaitForEvents( cl_uint num_events, const cl_event* event_list )
{
    TRACE( "(%u, %p)\n", num_events, event_list );
    return opencl_funcs->pclWaitForEvents( num_events, event_list );
}

BOOL extension_is_supported( const char *name, size_t len )
{
    unsigned int i;

    static const char *const unsupported[] =
    {
        "cl_apple_contextloggingfunctions",
        "cl_apple_setmemobjectdestructor",
        "cl_arm_import_memory",
        "cl_arm_shared_virtual_memory",
        "cl_ext_device_fission",
        "cl_ext_migrate_memobject",
        "cl_img_generate_mipmap",
        "cl_img_use_gralloc_ptr",
        "cl_intel_accelerator",
        "cl_intel_create_buffer_with_properties",
        "cl_intel_d3d11_nv12_media_sharing",
        "cl_intel_dx9_media_sharing",
        "cl_intel_unified_shared_memory",
        "cl_intel_va_api_media_sharing",
        "cl_khr_create_command_queue",
        "cl_khr_d3d10_sharing",
        "cl_khr_d3d11_sharing",
        "cl_khr_dx9_media_sharing",
        "cl_khr_egl_event",
        "cl_khr_egl_image",
        "cl_khr_gl_event",
        "cl_khr_gl_sharing",
        "cl_khr_icd",
        "cl_khr_il_program",
        "cl_khr_subgroups",
        "cl_khr_terminate_context",
        "cl_loader_layers",
        "cl_nv_d3d10_sharing",
        "cl_nv_d3d11_sharing",
        "cl_nv_d3d9_sharing",
        "cl_qcom_ext_host_ptr",
    };

    for (i = 0; i < ARRAY_SIZE(unsupported); ++i)
    {
        if (!strncasecmp( name, unsupported[i], len ))
            return FALSE;
    }
    return TRUE;
}
