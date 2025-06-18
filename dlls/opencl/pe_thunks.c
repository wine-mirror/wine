/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

#include "opencl_private.h"
#include "opencl_types.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(opencl);

cl_int WINAPI clBuildProgram( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data )
{
    struct clBuildProgram_params params = { program, num_devices, device_list, options, pfn_notify, user_data };
    TRACE( "(%p, %u, %p, %p, %p, %p)\n", program, num_devices, device_list, options, pfn_notify, user_data );
    return OPENCL_CALL( clBuildProgram, &params );
}

cl_int WINAPI clCompileProgram( cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_headers, const cl_program* input_headers, const char** header_include_names, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data )
{
    struct clCompileProgram_params params = { program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data };
    TRACE( "(%p, %u, %p, %p, %u, %p, %p, %p, %p)\n", program, num_devices, device_list, options, num_input_headers, input_headers, header_include_names, pfn_notify, user_data );
    return OPENCL_CALL( clCompileProgram, &params );
}

cl_mem WINAPI clCreateBuffer( cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret )
{
    cl_mem __retval;
    struct clCreateBuffer_params params = { &__retval, context, flags, size, host_ptr, errcode_ret };
    TRACE( "(%p, %s, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), size, host_ptr, errcode_ret );
    OPENCL_CALL( clCreateBuffer, &params );
    return __retval;
}

cl_command_queue WINAPI clCreateCommandQueue( cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret )
{
    cl_command_queue __retval;
    struct clCreateCommandQueue_params params = { &__retval, context, device, properties, errcode_ret };
    TRACE( "(%p, %p, %s, %p)\n", context, device, wine_dbgstr_longlong(properties), errcode_ret );
    OPENCL_CALL( clCreateCommandQueue, &params );
    return __retval;
}

cl_context WINAPI clCreateContext( const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret )
{
    cl_context __retval;
    struct clCreateContext_params params = { &__retval, properties, num_devices, devices, pfn_notify, user_data, errcode_ret };
    TRACE( "(%p, %u, %p, %p, %p, %p)\n", properties, num_devices, devices, pfn_notify, user_data, errcode_ret );
    OPENCL_CALL( clCreateContext, &params );
    return __retval;
}

cl_context WINAPI clCreateContextFromType( const cl_context_properties* properties, cl_device_type device_type, void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret )
{
    cl_context __retval;
    struct clCreateContextFromType_params params = { &__retval, properties, device_type, pfn_notify, user_data, errcode_ret };
    TRACE( "(%p, %s, %p, %p, %p)\n", properties, wine_dbgstr_longlong(device_type), pfn_notify, user_data, errcode_ret );
    OPENCL_CALL( clCreateContextFromType, &params );
    return __retval;
}

cl_mem WINAPI clCreateImage( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, const cl_image_desc* image_desc, void* host_ptr, cl_int* errcode_ret )
{
    cl_mem __retval;
    struct clCreateImage_params params = { &__retval, context, flags, image_format, image_desc, host_ptr, errcode_ret };
    TRACE( "(%p, %s, %p, %p, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_desc, host_ptr, errcode_ret );
    OPENCL_CALL( clCreateImage, &params );
    return __retval;
}

cl_mem WINAPI clCreateImage2D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret )
{
    cl_mem __retval;
    struct clCreateImage2D_params params = { &__retval, context, flags, image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret };
    TRACE( "(%p, %s, %p, %Iu, %Iu, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_row_pitch, host_ptr, errcode_ret );
    OPENCL_CALL( clCreateImage2D, &params );
    return __retval;
}

cl_mem WINAPI clCreateImage3D( cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_depth, size_t image_row_pitch, size_t image_slice_pitch, void* host_ptr, cl_int* errcode_ret )
{
    cl_mem __retval;
    struct clCreateImage3D_params params = { &__retval, context, flags, image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret };
    TRACE( "(%p, %s, %p, %Iu, %Iu, %Iu, %Iu, %Iu, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_format, image_width, image_height, image_depth, image_row_pitch, image_slice_pitch, host_ptr, errcode_ret );
    OPENCL_CALL( clCreateImage3D, &params );
    return __retval;
}

cl_kernel WINAPI clCreateKernel( cl_program program, const char* kernel_name, cl_int* errcode_ret )
{
    cl_kernel __retval;
    struct clCreateKernel_params params = { &__retval, program, kernel_name, errcode_ret };
    TRACE( "(%p, %p, %p)\n", program, kernel_name, errcode_ret );
    OPENCL_CALL( clCreateKernel, &params );
    return __retval;
}

cl_int WINAPI clCreateKernelsInProgram( cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret )
{
    struct clCreateKernelsInProgram_params params = { program, num_kernels, kernels, num_kernels_ret };
    TRACE( "(%p, %u, %p, %p)\n", program, num_kernels, kernels, num_kernels_ret );
    return OPENCL_CALL( clCreateKernelsInProgram, &params );
}

cl_program WINAPI clCreateProgramWithBinary( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const size_t* lengths, const unsigned char** binaries, cl_int* binary_status, cl_int* errcode_ret )
{
    cl_program __retval;
    struct clCreateProgramWithBinary_params params = { &__retval, context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret };
    TRACE( "(%p, %u, %p, %p, %p, %p, %p)\n", context, num_devices, device_list, lengths, binaries, binary_status, errcode_ret );
    OPENCL_CALL( clCreateProgramWithBinary, &params );
    return __retval;
}

cl_program WINAPI clCreateProgramWithBuiltInKernels( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* kernel_names, cl_int* errcode_ret )
{
    cl_program __retval;
    struct clCreateProgramWithBuiltInKernels_params params = { &__retval, context, num_devices, device_list, kernel_names, errcode_ret };
    TRACE( "(%p, %u, %p, %p, %p)\n", context, num_devices, device_list, kernel_names, errcode_ret );
    OPENCL_CALL( clCreateProgramWithBuiltInKernels, &params );
    return __retval;
}

cl_program WINAPI clCreateProgramWithSource( cl_context context, cl_uint count, const char** strings, const size_t* lengths, cl_int* errcode_ret )
{
    cl_program __retval;
    struct clCreateProgramWithSource_params params = { &__retval, context, count, strings, lengths, errcode_ret };
    TRACE( "(%p, %u, %p, %p, %p)\n", context, count, strings, lengths, errcode_ret );
    OPENCL_CALL( clCreateProgramWithSource, &params );
    return __retval;
}

cl_sampler WINAPI clCreateSampler( cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret )
{
    cl_sampler __retval;
    struct clCreateSampler_params params = { &__retval, context, normalized_coords, addressing_mode, filter_mode, errcode_ret };
    TRACE( "(%p, %u, %u, %u, %p)\n", context, normalized_coords, addressing_mode, filter_mode, errcode_ret );
    OPENCL_CALL( clCreateSampler, &params );
    return __retval;
}

cl_mem WINAPI clCreateSubBuffer( cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret )
{
    cl_mem __retval;
    struct clCreateSubBuffer_params params = { &__retval, buffer, flags, buffer_create_type, buffer_create_info, errcode_ret };
    TRACE( "(%p, %s, %u, %p, %p)\n", buffer, wine_dbgstr_longlong(flags), buffer_create_type, buffer_create_info, errcode_ret );
    OPENCL_CALL( clCreateSubBuffer, &params );
    return __retval;
}

cl_int WINAPI clCreateSubDevices( cl_device_id in_device, const cl_device_partition_property* properties, cl_uint num_devices, cl_device_id* out_devices, cl_uint* num_devices_ret )
{
    struct clCreateSubDevices_params params = { in_device, properties, num_devices, out_devices, num_devices_ret };
    TRACE( "(%p, %p, %u, %p, %p)\n", in_device, properties, num_devices, out_devices, num_devices_ret );
    return OPENCL_CALL( clCreateSubDevices, &params );
}

cl_event WINAPI clCreateUserEvent( cl_context context, cl_int* errcode_ret )
{
    cl_event __retval;
    struct clCreateUserEvent_params params = { &__retval, context, errcode_ret };
    TRACE( "(%p, %p)\n", context, errcode_ret );
    OPENCL_CALL( clCreateUserEvent, &params );
    return __retval;
}

cl_int WINAPI clEnqueueBarrier( cl_command_queue command_queue )
{
    struct clEnqueueBarrier_params params = { command_queue };
    TRACE( "(%p)\n", command_queue );
    return OPENCL_CALL( clEnqueueBarrier, &params );
}

cl_int WINAPI clEnqueueBarrierWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueBarrierWithWaitList_params params = { command_queue, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %u, %p, %p)\n", command_queue, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueBarrierWithWaitList, &params );
}

cl_int WINAPI clEnqueueCopyBuffer( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueCopyBuffer_params params = { command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, src_buffer, dst_buffer, src_offset, dst_offset, size, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueCopyBuffer, &params );
}

cl_int WINAPI clEnqueueCopyBufferRect( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueCopyBufferRect_params params = { command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueCopyBufferRect, &params );
}

cl_int WINAPI clEnqueueCopyBufferToImage( cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueCopyBufferToImage_params params = { command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %Iu, %p, %p, %u, %p, %p)\n", command_queue, src_buffer, dst_image, src_offset, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueCopyBufferToImage, &params );
}

cl_int WINAPI clEnqueueCopyImage( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueCopyImage_params params = { command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %p, %p, %p, %u, %p, %p)\n", command_queue, src_image, dst_image, src_origin, dst_origin, region, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueCopyImage, &params );
}

cl_int WINAPI clEnqueueCopyImageToBuffer( cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueCopyImageToBuffer_params params = { command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %p, %p, %Iu, %u, %p, %p)\n", command_queue, src_image, dst_buffer, src_origin, region, dst_offset, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueCopyImageToBuffer, &params );
}

cl_int WINAPI clEnqueueFillBuffer( cl_command_queue command_queue, cl_mem buffer, const void* pattern, size_t pattern_size, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueFillBuffer_params params = { command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %Iu, %Iu, %Iu, %u, %p, %p)\n", command_queue, buffer, pattern, pattern_size, offset, size, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueFillBuffer, &params );
}

cl_int WINAPI clEnqueueFillImage( cl_command_queue command_queue, cl_mem image, const void* fill_color, const size_t* origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueFillImage_params params = { command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %p, %p, %u, %p, %p)\n", command_queue, image, fill_color, origin, region, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueFillImage, &params );
}

void* WINAPI clEnqueueMapBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_map, cl_map_flags map_flags, size_t offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    void* __retval;
    struct clEnqueueMapBuffer_params params = { &__retval, command_queue, buffer, blocking_map, map_flags, offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret };
    TRACE( "(%p, %p, %u, %s, %Iu, %Iu, %u, %p, %p, %p)\n", command_queue, buffer, blocking_map, wine_dbgstr_longlong(map_flags), offset, size, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    OPENCL_CALL( clEnqueueMapBuffer, &params );
    return __retval;
}

void* WINAPI clEnqueueMapImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_map, cl_map_flags map_flags, const size_t* origin, const size_t* region, size_t* image_row_pitch, size_t* image_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event, cl_int* errcode_ret )
{
    void* __retval;
    struct clEnqueueMapImage_params params = { &__retval, command_queue, image, blocking_map, map_flags, origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret };
    TRACE( "(%p, %p, %u, %s, %p, %p, %p, %p, %u, %p, %p, %p)\n", command_queue, image, blocking_map, wine_dbgstr_longlong(map_flags), origin, region, image_row_pitch, image_slice_pitch, num_events_in_wait_list, event_wait_list, event, errcode_ret );
    OPENCL_CALL( clEnqueueMapImage, &params );
    return __retval;
}

cl_int WINAPI clEnqueueMarker( cl_command_queue command_queue, cl_event* event )
{
    struct clEnqueueMarker_params params = { command_queue, event };
    TRACE( "(%p, %p)\n", command_queue, event );
    return OPENCL_CALL( clEnqueueMarker, &params );
}

cl_int WINAPI clEnqueueMarkerWithWaitList( cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueMarkerWithWaitList_params params = { command_queue, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %u, %p, %p)\n", command_queue, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueMarkerWithWaitList, &params );
}

cl_int WINAPI clEnqueueMigrateMemObjects( cl_command_queue command_queue, cl_uint num_mem_objects, const cl_mem* mem_objects, cl_mem_migration_flags flags, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueMigrateMemObjects_params params = { command_queue, num_mem_objects, mem_objects, flags, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %u, %p, %s, %u, %p, %p)\n", command_queue, num_mem_objects, mem_objects, wine_dbgstr_longlong(flags), num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueMigrateMemObjects, &params );
}

cl_int WINAPI clEnqueueNDRangeKernel( cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueNDRangeKernel_params params = { command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p, %p, %u, %p, %p)\n", command_queue, kernel, work_dim, global_work_offset, global_work_size, local_work_size, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueNDRangeKernel, &params );
}

cl_int WINAPI clEnqueueNativeKernel( cl_command_queue command_queue, void (WINAPI* user_func)(void*), void* args, size_t cb_args, cl_uint num_mem_objects, const cl_mem* mem_list, const void** args_mem_loc, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueNativeKernel_params params = { command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %Iu, %u, %p, %p, %u, %p, %p)\n", command_queue, user_func, args, cb_args, num_mem_objects, mem_list, args_mem_loc, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueNativeKernel, &params );
}

cl_int WINAPI clEnqueueReadBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueReadBuffer_params params = { command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_read, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueReadBuffer, &params );
}

cl_int WINAPI clEnqueueReadBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueReadBufferRect_params params = { command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_read, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueReadBufferRect, &params );
}

cl_int WINAPI clEnqueueReadImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueReadImage_params params = { command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, image, blocking_read, origin, region, row_pitch, slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueReadImage, &params );
}

cl_int WINAPI clEnqueueTask( cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueTask_params params = { command_queue, kernel, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p)\n", command_queue, kernel, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueTask, &params );
}

cl_int WINAPI clEnqueueUnmapMemObject( cl_command_queue command_queue, cl_mem memobj, void* mapped_ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueUnmapMemObject_params params = { command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %p, %u, %p, %p)\n", command_queue, memobj, mapped_ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueUnmapMemObject, &params );
}

cl_int WINAPI clEnqueueWaitForEvents( cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list )
{
    struct clEnqueueWaitForEvents_params params = { command_queue, num_events, event_list };
    TRACE( "(%p, %u, %p)\n", command_queue, num_events, event_list );
    return OPENCL_CALL( clEnqueueWaitForEvents, &params );
}

cl_int WINAPI clEnqueueWriteBuffer( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueWriteBuffer_params params = { command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_write, offset, size, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueWriteBuffer, &params );
}

cl_int WINAPI clEnqueueWriteBufferRect( cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueWriteBufferRect_params params = { command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p, %p, %Iu, %Iu, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, buffer, blocking_write, buffer_origin, host_origin, region, buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueWriteBufferRect, &params );
}

cl_int WINAPI clEnqueueWriteImage( cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event )
{
    struct clEnqueueWriteImage_params params = { command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event };
    TRACE( "(%p, %p, %u, %p, %p, %Iu, %Iu, %p, %u, %p, %p)\n", command_queue, image, blocking_write, origin, region, input_row_pitch, input_slice_pitch, ptr, num_events_in_wait_list, event_wait_list, event );
    return OPENCL_CALL( clEnqueueWriteImage, &params );
}

cl_int WINAPI clFinish( cl_command_queue command_queue )
{
    struct clFinish_params params = { command_queue };
    TRACE( "(%p)\n", command_queue );
    return OPENCL_CALL( clFinish, &params );
}

cl_int WINAPI clFlush( cl_command_queue command_queue )
{
    struct clFlush_params params = { command_queue };
    TRACE( "(%p)\n", command_queue );
    return OPENCL_CALL( clFlush, &params );
}

cl_int WINAPI clGetCommandQueueInfo( cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetCommandQueueInfo_params params = { command_queue, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", command_queue, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetCommandQueueInfo, &params );
}

cl_int WINAPI clGetContextInfo( cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetContextInfo_params params = { context, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", context, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetContextInfo, &params );
}

cl_int WINAPI clGetDeviceIDs( cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices )
{
    struct clGetDeviceIDs_params params = { platform, device_type, num_entries, devices, num_devices };
    TRACE( "(%p, %s, %u, %p, %p)\n", platform, wine_dbgstr_longlong(device_type), num_entries, devices, num_devices );
    return OPENCL_CALL( clGetDeviceIDs, &params );
}

cl_int WINAPI clGetEventInfo( cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetEventInfo_params params = { event, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetEventInfo, &params );
}

cl_int WINAPI clGetEventProfilingInfo( cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetEventProfilingInfo_params params = { event, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", event, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetEventProfilingInfo, &params );
}

cl_int WINAPI clGetImageInfo( cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetImageInfo_params params = { image, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", image, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetImageInfo, &params );
}

cl_int WINAPI clGetKernelArgInfo( cl_kernel kernel, cl_uint arg_index, cl_kernel_arg_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetKernelArgInfo_params params = { kernel, arg_index, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %u, %Iu, %p, %p)\n", kernel, arg_index, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetKernelArgInfo, &params );
}

cl_int WINAPI clGetKernelInfo( cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetKernelInfo_params params = { kernel, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", kernel, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetKernelInfo, &params );
}

cl_int WINAPI clGetKernelWorkGroupInfo( cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetKernelWorkGroupInfo_params params = { kernel, device, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %p, %u, %Iu, %p, %p)\n", kernel, device, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetKernelWorkGroupInfo, &params );
}

cl_int WINAPI clGetMemObjectInfo( cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetMemObjectInfo_params params = { memobj, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", memobj, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetMemObjectInfo, &params );
}

cl_int WINAPI clGetPlatformIDs( cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms )
{
    struct clGetPlatformIDs_params params = { num_entries, platforms, num_platforms };
    TRACE( "(%u, %p, %p)\n", num_entries, platforms, num_platforms );
    return OPENCL_CALL( clGetPlatformIDs, &params );
}

cl_int WINAPI clGetProgramBuildInfo( cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetProgramBuildInfo_params params = { program, device, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %p, %u, %Iu, %p, %p)\n", program, device, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetProgramBuildInfo, &params );
}

cl_int WINAPI clGetProgramInfo( cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetProgramInfo_params params = { program, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", program, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetProgramInfo, &params );
}

cl_int WINAPI clGetSamplerInfo( cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret )
{
    struct clGetSamplerInfo_params params = { sampler, param_name, param_value_size, param_value, param_value_size_ret };
    TRACE( "(%p, %u, %Iu, %p, %p)\n", sampler, param_name, param_value_size, param_value, param_value_size_ret );
    return OPENCL_CALL( clGetSamplerInfo, &params );
}

cl_int WINAPI clGetSupportedImageFormats( cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats )
{
    struct clGetSupportedImageFormats_params params = { context, flags, image_type, num_entries, image_formats, num_image_formats };
    TRACE( "(%p, %s, %u, %u, %p, %p)\n", context, wine_dbgstr_longlong(flags), image_type, num_entries, image_formats, num_image_formats );
    return OPENCL_CALL( clGetSupportedImageFormats, &params );
}

cl_program WINAPI clLinkProgram( cl_context context, cl_uint num_devices, const cl_device_id* device_list, const char* options, cl_uint num_input_programs, const cl_program* input_programs, void (WINAPI* pfn_notify)(cl_program program, void* user_data), void* user_data, cl_int* errcode_ret )
{
    cl_program __retval;
    struct clLinkProgram_params params = { &__retval, context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret };
    TRACE( "(%p, %u, %p, %p, %u, %p, %p, %p, %p)\n", context, num_devices, device_list, options, num_input_programs, input_programs, pfn_notify, user_data, errcode_ret );
    OPENCL_CALL( clLinkProgram, &params );
    return __retval;
}

cl_int WINAPI clReleaseCommandQueue( cl_command_queue command_queue )
{
    struct clReleaseCommandQueue_params params = { command_queue };
    TRACE( "(%p)\n", command_queue );
    return OPENCL_CALL( clReleaseCommandQueue, &params );
}

cl_int WINAPI clReleaseContext( cl_context context )
{
    struct clReleaseContext_params params = { context };
    TRACE( "(%p)\n", context );
    return OPENCL_CALL( clReleaseContext, &params );
}

cl_int WINAPI clReleaseDevice( cl_device_id device )
{
    struct clReleaseDevice_params params = { device };
    TRACE( "(%p)\n", device );
    return OPENCL_CALL( clReleaseDevice, &params );
}

cl_int WINAPI clReleaseEvent( cl_event event )
{
    struct clReleaseEvent_params params = { event };
    TRACE( "(%p)\n", event );
    return OPENCL_CALL( clReleaseEvent, &params );
}

cl_int WINAPI clReleaseKernel( cl_kernel kernel )
{
    struct clReleaseKernel_params params = { kernel };
    TRACE( "(%p)\n", kernel );
    return OPENCL_CALL( clReleaseKernel, &params );
}

cl_int WINAPI clReleaseMemObject( cl_mem memobj )
{
    struct clReleaseMemObject_params params = { memobj };
    TRACE( "(%p)\n", memobj );
    return OPENCL_CALL( clReleaseMemObject, &params );
}

cl_int WINAPI clReleaseProgram( cl_program program )
{
    struct clReleaseProgram_params params = { program };
    TRACE( "(%p)\n", program );
    return OPENCL_CALL( clReleaseProgram, &params );
}

cl_int WINAPI clReleaseSampler( cl_sampler sampler )
{
    struct clReleaseSampler_params params = { sampler };
    TRACE( "(%p)\n", sampler );
    return OPENCL_CALL( clReleaseSampler, &params );
}

cl_int WINAPI clRetainCommandQueue( cl_command_queue command_queue )
{
    struct clRetainCommandQueue_params params = { command_queue };
    TRACE( "(%p)\n", command_queue );
    return OPENCL_CALL( clRetainCommandQueue, &params );
}

cl_int WINAPI clRetainContext( cl_context context )
{
    struct clRetainContext_params params = { context };
    TRACE( "(%p)\n", context );
    return OPENCL_CALL( clRetainContext, &params );
}

cl_int WINAPI clRetainDevice( cl_device_id device )
{
    struct clRetainDevice_params params = { device };
    TRACE( "(%p)\n", device );
    return OPENCL_CALL( clRetainDevice, &params );
}

cl_int WINAPI clRetainEvent( cl_event event )
{
    struct clRetainEvent_params params = { event };
    TRACE( "(%p)\n", event );
    return OPENCL_CALL( clRetainEvent, &params );
}

cl_int WINAPI clRetainKernel( cl_kernel kernel )
{
    struct clRetainKernel_params params = { kernel };
    TRACE( "(%p)\n", kernel );
    return OPENCL_CALL( clRetainKernel, &params );
}

cl_int WINAPI clRetainMemObject( cl_mem memobj )
{
    struct clRetainMemObject_params params = { memobj };
    TRACE( "(%p)\n", memobj );
    return OPENCL_CALL( clRetainMemObject, &params );
}

cl_int WINAPI clRetainProgram( cl_program program )
{
    struct clRetainProgram_params params = { program };
    TRACE( "(%p)\n", program );
    return OPENCL_CALL( clRetainProgram, &params );
}

cl_int WINAPI clRetainSampler( cl_sampler sampler )
{
    struct clRetainSampler_params params = { sampler };
    TRACE( "(%p)\n", sampler );
    return OPENCL_CALL( clRetainSampler, &params );
}

cl_int WINAPI clSetEventCallback( cl_event event, cl_int command_exec_callback_type, void (WINAPI* pfn_notify)(cl_event event, cl_int event_command_status, void *user_data), void* user_data )
{
    struct clSetEventCallback_params params = { event, command_exec_callback_type, pfn_notify, user_data };
    TRACE( "(%p, %d, %p, %p)\n", event, command_exec_callback_type, pfn_notify, user_data );
    return OPENCL_CALL( clSetEventCallback, &params );
}

cl_int WINAPI clSetKernelArg( cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value )
{
    struct clSetKernelArg_params params = { kernel, arg_index, arg_size, arg_value };
    TRACE( "(%p, %u, %Iu, %p)\n", kernel, arg_index, arg_size, arg_value );
    return OPENCL_CALL( clSetKernelArg, &params );
}

cl_int WINAPI clSetMemObjectDestructorCallback( cl_mem memobj, void (WINAPI* pfn_notify)(cl_mem memobj, void* user_data), void* user_data )
{
    struct clSetMemObjectDestructorCallback_params params = { memobj, pfn_notify, user_data };
    TRACE( "(%p, %p, %p)\n", memobj, pfn_notify, user_data );
    return OPENCL_CALL( clSetMemObjectDestructorCallback, &params );
}

cl_int WINAPI clSetUserEventStatus( cl_event event, cl_int execution_status )
{
    struct clSetUserEventStatus_params params = { event, execution_status };
    TRACE( "(%p, %d)\n", event, execution_status );
    return OPENCL_CALL( clSetUserEventStatus, &params );
}

cl_int WINAPI clUnloadCompiler( void  )
{
    struct clUnloadCompiler_params params = {};
    TRACE( "()\n" );
    return OPENCL_CALL( clUnloadCompiler, &params );
}

cl_int WINAPI clUnloadPlatformCompiler( cl_platform_id platform )
{
    struct clUnloadPlatformCompiler_params params = { platform };
    TRACE( "(%p)\n", platform );
    return OPENCL_CALL( clUnloadPlatformCompiler, &params );
}

cl_int WINAPI clWaitForEvents( cl_uint num_events, const cl_event* event_list )
{
    struct clWaitForEvents_params params = { num_events, event_list };
    TRACE( "(%u, %p)\n", num_events, event_list );
    return OPENCL_CALL( clWaitForEvents, &params );
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
