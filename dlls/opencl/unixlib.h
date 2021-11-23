/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

struct clBuildProgram_params
{
    cl_program program;
    cl_uint num_devices;
    const cl_device_id* device_list;
    const char* options;
    void (WINAPI* pfn_notify)(cl_program program, void* user_data);
    void* user_data;
};

struct clCompileProgram_params
{
    cl_program program;
    cl_uint num_devices;
    const cl_device_id* device_list;
    const char* options;
    cl_uint num_input_headers;
    const cl_program* input_headers;
    const char** header_include_names;
    void (WINAPI* pfn_notify)(cl_program program, void* user_data);
    void* user_data;
};

struct clCreateBuffer_params
{
    cl_mem* __retval;
    cl_context context;
    cl_mem_flags flags;
    size_t size;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateCommandQueue_params
{
    cl_command_queue* __retval;
    cl_context context;
    cl_device_id device;
    cl_command_queue_properties properties;
    cl_int* errcode_ret;
};

struct clCreateContext_params
{
    cl_context* __retval;
    const cl_context_properties* properties;
    cl_uint num_devices;
    const cl_device_id* devices;
    void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data);
    void* user_data;
    cl_int* errcode_ret;
};

struct clCreateContextFromType_params
{
    cl_context* __retval;
    const cl_context_properties* properties;
    cl_device_type device_type;
    void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data);
    void* user_data;
    cl_int* errcode_ret;
};

struct clCreateImage_params
{
    cl_mem* __retval;
    cl_context context;
    cl_mem_flags flags;
    const cl_image_format* image_format;
    const cl_image_desc* image_desc;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateImage2D_params
{
    cl_mem* __retval;
    cl_context context;
    cl_mem_flags flags;
    const cl_image_format* image_format;
    size_t image_width;
    size_t image_height;
    size_t image_row_pitch;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateImage3D_params
{
    cl_mem* __retval;
    cl_context context;
    cl_mem_flags flags;
    const cl_image_format* image_format;
    size_t image_width;
    size_t image_height;
    size_t image_depth;
    size_t image_row_pitch;
    size_t image_slice_pitch;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateKernel_params
{
    cl_kernel* __retval;
    cl_program program;
    const char* kernel_name;
    cl_int* errcode_ret;
};

struct clCreateKernelsInProgram_params
{
    cl_program program;
    cl_uint num_kernels;
    cl_kernel* kernels;
    cl_uint* num_kernels_ret;
};

struct clCreateProgramWithBinary_params
{
    cl_program* __retval;
    cl_context context;
    cl_uint num_devices;
    const cl_device_id* device_list;
    const size_t* lengths;
    const unsigned char** binaries;
    cl_int* binary_status;
    cl_int* errcode_ret;
};

struct clCreateProgramWithBuiltInKernels_params
{
    cl_program* __retval;
    cl_context context;
    cl_uint num_devices;
    const cl_device_id* device_list;
    const char* kernel_names;
    cl_int* errcode_ret;
};

struct clCreateProgramWithSource_params
{
    cl_program* __retval;
    cl_context context;
    cl_uint count;
    const char** strings;
    const size_t* lengths;
    cl_int* errcode_ret;
};

struct clCreateSampler_params
{
    cl_sampler* __retval;
    cl_context context;
    cl_bool normalized_coords;
    cl_addressing_mode addressing_mode;
    cl_filter_mode filter_mode;
    cl_int* errcode_ret;
};

struct clCreateSubBuffer_params
{
    cl_mem* __retval;
    cl_mem buffer;
    cl_mem_flags flags;
    cl_buffer_create_type buffer_create_type;
    const void* buffer_create_info;
    cl_int* errcode_ret;
};

struct clCreateSubDevices_params
{
    cl_device_id in_device;
    const cl_device_partition_property* properties;
    cl_uint num_devices;
    cl_device_id* out_devices;
    cl_uint* num_devices_ret;
};

struct clCreateUserEvent_params
{
    cl_event* __retval;
    cl_context context;
    cl_int* errcode_ret;
};

struct clEnqueueBarrier_params
{
    cl_command_queue command_queue;
};

struct clEnqueueBarrierWithWaitList_params
{
    cl_command_queue command_queue;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyBuffer_params
{
    cl_command_queue command_queue;
    cl_mem src_buffer;
    cl_mem dst_buffer;
    size_t src_offset;
    size_t dst_offset;
    size_t size;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyBufferRect_params
{
    cl_command_queue command_queue;
    cl_mem src_buffer;
    cl_mem dst_buffer;
    const size_t* src_origin;
    const size_t* dst_origin;
    const size_t* region;
    size_t src_row_pitch;
    size_t src_slice_pitch;
    size_t dst_row_pitch;
    size_t dst_slice_pitch;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyBufferToImage_params
{
    cl_command_queue command_queue;
    cl_mem src_buffer;
    cl_mem dst_image;
    size_t src_offset;
    const size_t* dst_origin;
    const size_t* region;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyImage_params
{
    cl_command_queue command_queue;
    cl_mem src_image;
    cl_mem dst_image;
    const size_t* src_origin;
    const size_t* dst_origin;
    const size_t* region;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyImageToBuffer_params
{
    cl_command_queue command_queue;
    cl_mem src_image;
    cl_mem dst_buffer;
    const size_t* src_origin;
    const size_t* region;
    size_t dst_offset;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueFillBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    const void* pattern;
    size_t pattern_size;
    size_t offset;
    size_t size;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueFillImage_params
{
    cl_command_queue command_queue;
    cl_mem image;
    const void* fill_color;
    const size_t* origin;
    const size_t* region;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueMapBuffer_params
{
    void** __retval;
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_map;
    cl_map_flags map_flags;
    size_t offset;
    size_t size;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
    cl_int* errcode_ret;
};

struct clEnqueueMapImage_params
{
    void** __retval;
    cl_command_queue command_queue;
    cl_mem image;
    cl_bool blocking_map;
    cl_map_flags map_flags;
    const size_t* origin;
    const size_t* region;
    size_t* image_row_pitch;
    size_t* image_slice_pitch;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
    cl_int* errcode_ret;
};

struct clEnqueueMarker_params
{
    cl_command_queue command_queue;
    cl_event* event;
};

struct clEnqueueMarkerWithWaitList_params
{
    cl_command_queue command_queue;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueMigrateMemObjects_params
{
    cl_command_queue command_queue;
    cl_uint num_mem_objects;
    const cl_mem* mem_objects;
    cl_mem_migration_flags flags;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueNDRangeKernel_params
{
    cl_command_queue command_queue;
    cl_kernel kernel;
    cl_uint work_dim;
    const size_t* global_work_offset;
    const size_t* global_work_size;
    const size_t* local_work_size;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueNativeKernel_params
{
    cl_command_queue command_queue;
    void (WINAPI* user_func)(void*);
    void* args;
    size_t cb_args;
    cl_uint num_mem_objects;
    const cl_mem* mem_list;
    const void** args_mem_loc;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_read;
    size_t offset;
    size_t size;
    void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadBufferRect_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_read;
    const size_t* buffer_origin;
    const size_t* host_origin;
    const size_t* region;
    size_t buffer_row_pitch;
    size_t buffer_slice_pitch;
    size_t host_row_pitch;
    size_t host_slice_pitch;
    void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadImage_params
{
    cl_command_queue command_queue;
    cl_mem image;
    cl_bool blocking_read;
    const size_t* origin;
    const size_t* region;
    size_t row_pitch;
    size_t slice_pitch;
    void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueTask_params
{
    cl_command_queue command_queue;
    cl_kernel kernel;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueUnmapMemObject_params
{
    cl_command_queue command_queue;
    cl_mem memobj;
    void* mapped_ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWaitForEvents_params
{
    cl_command_queue command_queue;
    cl_uint num_events;
    const cl_event* event_list;
};

struct clEnqueueWriteBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_write;
    size_t offset;
    size_t size;
    const void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWriteBufferRect_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    cl_bool blocking_write;
    const size_t* buffer_origin;
    const size_t* host_origin;
    const size_t* region;
    size_t buffer_row_pitch;
    size_t buffer_slice_pitch;
    size_t host_row_pitch;
    size_t host_slice_pitch;
    const void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWriteImage_params
{
    cl_command_queue command_queue;
    cl_mem image;
    cl_bool blocking_write;
    const size_t* origin;
    const size_t* region;
    size_t input_row_pitch;
    size_t input_slice_pitch;
    const void* ptr;
    cl_uint num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clFinish_params
{
    cl_command_queue command_queue;
};

struct clFlush_params
{
    cl_command_queue command_queue;
};

struct clGetCommandQueueInfo_params
{
    cl_command_queue command_queue;
    cl_command_queue_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetContextInfo_params
{
    cl_context context;
    cl_context_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetDeviceIDs_params
{
    cl_platform_id platform;
    cl_device_type device_type;
    cl_uint num_entries;
    cl_device_id* devices;
    cl_uint* num_devices;
};

struct clGetDeviceInfo_params
{
    cl_device_id device;
    cl_device_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetEventInfo_params
{
    cl_event event;
    cl_event_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetEventProfilingInfo_params
{
    cl_event event;
    cl_profiling_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetImageInfo_params
{
    cl_mem image;
    cl_image_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelArgInfo_params
{
    cl_kernel kernel;
    cl_uint arg_index;
    cl_kernel_arg_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelInfo_params
{
    cl_kernel kernel;
    cl_kernel_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelWorkGroupInfo_params
{
    cl_kernel kernel;
    cl_device_id device;
    cl_kernel_work_group_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetMemObjectInfo_params
{
    cl_mem memobj;
    cl_mem_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetPlatformIDs_params
{
    cl_uint num_entries;
    cl_platform_id* platforms;
    cl_uint* num_platforms;
};

struct clGetPlatformInfo_params
{
    cl_platform_id platform;
    cl_platform_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetProgramBuildInfo_params
{
    cl_program program;
    cl_device_id device;
    cl_program_build_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetProgramInfo_params
{
    cl_program program;
    cl_program_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetSamplerInfo_params
{
    cl_sampler sampler;
    cl_sampler_info param_name;
    size_t param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetSupportedImageFormats_params
{
    cl_context context;
    cl_mem_flags flags;
    cl_mem_object_type image_type;
    cl_uint num_entries;
    cl_image_format* image_formats;
    cl_uint* num_image_formats;
};

struct clLinkProgram_params
{
    cl_program* __retval;
    cl_context context;
    cl_uint num_devices;
    const cl_device_id* device_list;
    const char* options;
    cl_uint num_input_programs;
    const cl_program* input_programs;
    void (WINAPI* pfn_notify)(cl_program program, void* user_data);
    void* user_data;
    cl_int* errcode_ret;
};

struct clReleaseCommandQueue_params
{
    cl_command_queue command_queue;
};

struct clReleaseContext_params
{
    cl_context context;
};

struct clReleaseDevice_params
{
    cl_device_id device;
};

struct clReleaseEvent_params
{
    cl_event event;
};

struct clReleaseKernel_params
{
    cl_kernel kernel;
};

struct clReleaseMemObject_params
{
    cl_mem memobj;
};

struct clReleaseProgram_params
{
    cl_program program;
};

struct clReleaseSampler_params
{
    cl_sampler sampler;
};

struct clRetainCommandQueue_params
{
    cl_command_queue command_queue;
};

struct clRetainContext_params
{
    cl_context context;
};

struct clRetainDevice_params
{
    cl_device_id device;
};

struct clRetainEvent_params
{
    cl_event event;
};

struct clRetainKernel_params
{
    cl_kernel kernel;
};

struct clRetainMemObject_params
{
    cl_mem memobj;
};

struct clRetainProgram_params
{
    cl_program program;
};

struct clRetainSampler_params
{
    cl_sampler sampler;
};

struct clSetEventCallback_params
{
    cl_event event;
    cl_int command_exec_callback_type;
    void (WINAPI* pfn_notify)(cl_event event, cl_int event_command_status, void *user_data);
    void* user_data;
};

struct clSetKernelArg_params
{
    cl_kernel kernel;
    cl_uint arg_index;
    size_t arg_size;
    const void* arg_value;
};

struct clSetMemObjectDestructorCallback_params
{
    cl_mem memobj;
    void (WINAPI* pfn_notify)(cl_mem memobj, void* user_data);
    void* user_data;
};

struct clSetUserEventStatus_params
{
    cl_event event;
    cl_int execution_status;
};

struct clUnloadCompiler_params
{
};

struct clUnloadPlatformCompiler_params
{
    cl_platform_id platform;
};

struct clWaitForEvents_params
{
    cl_uint num_events;
    const cl_event* event_list;
};

enum opencl_funcs
{
    unix_clBuildProgram,
    unix_clCompileProgram,
    unix_clCreateBuffer,
    unix_clCreateCommandQueue,
    unix_clCreateContext,
    unix_clCreateContextFromType,
    unix_clCreateImage,
    unix_clCreateImage2D,
    unix_clCreateImage3D,
    unix_clCreateKernel,
    unix_clCreateKernelsInProgram,
    unix_clCreateProgramWithBinary,
    unix_clCreateProgramWithBuiltInKernels,
    unix_clCreateProgramWithSource,
    unix_clCreateSampler,
    unix_clCreateSubBuffer,
    unix_clCreateSubDevices,
    unix_clCreateUserEvent,
    unix_clEnqueueBarrier,
    unix_clEnqueueBarrierWithWaitList,
    unix_clEnqueueCopyBuffer,
    unix_clEnqueueCopyBufferRect,
    unix_clEnqueueCopyBufferToImage,
    unix_clEnqueueCopyImage,
    unix_clEnqueueCopyImageToBuffer,
    unix_clEnqueueFillBuffer,
    unix_clEnqueueFillImage,
    unix_clEnqueueMapBuffer,
    unix_clEnqueueMapImage,
    unix_clEnqueueMarker,
    unix_clEnqueueMarkerWithWaitList,
    unix_clEnqueueMigrateMemObjects,
    unix_clEnqueueNDRangeKernel,
    unix_clEnqueueNativeKernel,
    unix_clEnqueueReadBuffer,
    unix_clEnqueueReadBufferRect,
    unix_clEnqueueReadImage,
    unix_clEnqueueTask,
    unix_clEnqueueUnmapMemObject,
    unix_clEnqueueWaitForEvents,
    unix_clEnqueueWriteBuffer,
    unix_clEnqueueWriteBufferRect,
    unix_clEnqueueWriteImage,
    unix_clFinish,
    unix_clFlush,
    unix_clGetCommandQueueInfo,
    unix_clGetContextInfo,
    unix_clGetDeviceIDs,
    unix_clGetDeviceInfo,
    unix_clGetEventInfo,
    unix_clGetEventProfilingInfo,
    unix_clGetImageInfo,
    unix_clGetKernelArgInfo,
    unix_clGetKernelInfo,
    unix_clGetKernelWorkGroupInfo,
    unix_clGetMemObjectInfo,
    unix_clGetPlatformIDs,
    unix_clGetPlatformInfo,
    unix_clGetProgramBuildInfo,
    unix_clGetProgramInfo,
    unix_clGetSamplerInfo,
    unix_clGetSupportedImageFormats,
    unix_clLinkProgram,
    unix_clReleaseCommandQueue,
    unix_clReleaseContext,
    unix_clReleaseDevice,
    unix_clReleaseEvent,
    unix_clReleaseKernel,
    unix_clReleaseMemObject,
    unix_clReleaseProgram,
    unix_clReleaseSampler,
    unix_clRetainCommandQueue,
    unix_clRetainContext,
    unix_clRetainDevice,
    unix_clRetainEvent,
    unix_clRetainKernel,
    unix_clRetainMemObject,
    unix_clRetainProgram,
    unix_clRetainSampler,
    unix_clSetEventCallback,
    unix_clSetKernelArg,
    unix_clSetMemObjectDestructorCallback,
    unix_clSetUserEventStatus,
    unix_clUnloadCompiler,
    unix_clUnloadPlatformCompiler,
    unix_clWaitForEvents,
};
