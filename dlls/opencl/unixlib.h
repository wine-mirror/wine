/* Automatically generated from OpenCL registry files; DO NOT EDIT! */

struct clBuildProgram_params
{
    cl_program program;
    uint32_t num_devices;
    const cl_device_id* device_list;
    const char* options;
    void (WINAPI* pfn_notify)(cl_program program, void* user_data);
    void* user_data;
};

struct clCompileProgram_params
{
    cl_program program;
    uint32_t num_devices;
    const cl_device_id* device_list;
    const char* options;
    uint32_t num_input_headers;
    const cl_program* input_headers;
    const char** header_include_names;
    void (WINAPI* pfn_notify)(cl_program program, void* user_data);
    void* user_data;
};

struct clCreateBuffer_params
{
    cl_mem* __retval;
    cl_context context;
    ULONGLONG flags;
    SIZE_T size;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateCommandQueue_params
{
    cl_command_queue* __retval;
    cl_context context;
    cl_device_id device;
    ULONGLONG properties;
    cl_int* errcode_ret;
};

struct clCreateContext_params
{
    cl_context* __retval;
    const cl_context_properties* properties;
    uint32_t num_devices;
    const cl_device_id* devices;
    void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data);
    void* user_data;
    cl_int* errcode_ret;
};

struct clCreateContextFromType_params
{
    cl_context* __retval;
    const cl_context_properties* properties;
    ULONGLONG device_type;
    void (WINAPI* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data);
    void* user_data;
    cl_int* errcode_ret;
};

struct clCreateImage_params
{
    cl_mem* __retval;
    cl_context context;
    ULONGLONG flags;
    const cl_image_format* image_format;
    const cl_image_desc* image_desc;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateImage2D_params
{
    cl_mem* __retval;
    cl_context context;
    ULONGLONG flags;
    const cl_image_format* image_format;
    SIZE_T image_width;
    SIZE_T image_height;
    SIZE_T image_row_pitch;
    void* host_ptr;
    cl_int* errcode_ret;
};

struct clCreateImage3D_params
{
    cl_mem* __retval;
    cl_context context;
    ULONGLONG flags;
    const cl_image_format* image_format;
    SIZE_T image_width;
    SIZE_T image_height;
    SIZE_T image_depth;
    SIZE_T image_row_pitch;
    SIZE_T image_slice_pitch;
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
    uint32_t num_kernels;
    cl_kernel* kernels;
    cl_uint* num_kernels_ret;
};

struct clCreateProgramWithBinary_params
{
    cl_program* __retval;
    cl_context context;
    uint32_t num_devices;
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
    uint32_t num_devices;
    const cl_device_id* device_list;
    const char* kernel_names;
    cl_int* errcode_ret;
};

struct clCreateProgramWithSource_params
{
    cl_program* __retval;
    cl_context context;
    uint32_t count;
    const char** strings;
    const size_t* lengths;
    cl_int* errcode_ret;
};

struct clCreateSampler_params
{
    cl_sampler* __retval;
    cl_context context;
    uint32_t normalized_coords;
    uint32_t addressing_mode;
    uint32_t filter_mode;
    cl_int* errcode_ret;
};

struct clCreateSubBuffer_params
{
    cl_mem* __retval;
    cl_mem buffer;
    ULONGLONG flags;
    uint32_t buffer_create_type;
    const void* buffer_create_info;
    cl_int* errcode_ret;
};

struct clCreateSubDevices_params
{
    cl_device_id in_device;
    const cl_device_partition_property* properties;
    uint32_t num_devices;
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
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyBuffer_params
{
    cl_command_queue command_queue;
    cl_mem src_buffer;
    cl_mem dst_buffer;
    SIZE_T src_offset;
    SIZE_T dst_offset;
    SIZE_T size;
    uint32_t num_events_in_wait_list;
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
    SIZE_T src_row_pitch;
    SIZE_T src_slice_pitch;
    SIZE_T dst_row_pitch;
    SIZE_T dst_slice_pitch;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueCopyBufferToImage_params
{
    cl_command_queue command_queue;
    cl_mem src_buffer;
    cl_mem dst_image;
    SIZE_T src_offset;
    const size_t* dst_origin;
    const size_t* region;
    uint32_t num_events_in_wait_list;
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
    uint32_t num_events_in_wait_list;
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
    SIZE_T dst_offset;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueFillBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    const void* pattern;
    SIZE_T pattern_size;
    SIZE_T offset;
    SIZE_T size;
    uint32_t num_events_in_wait_list;
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
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueMapBuffer_params
{
    void** __retval;
    cl_command_queue command_queue;
    cl_mem buffer;
    uint32_t blocking_map;
    ULONGLONG map_flags;
    SIZE_T offset;
    SIZE_T size;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
    cl_int* errcode_ret;
};

struct clEnqueueMapImage_params
{
    void** __retval;
    cl_command_queue command_queue;
    cl_mem image;
    uint32_t blocking_map;
    ULONGLONG map_flags;
    const size_t* origin;
    const size_t* region;
    size_t* image_row_pitch;
    size_t* image_slice_pitch;
    uint32_t num_events_in_wait_list;
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
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueMigrateMemObjects_params
{
    cl_command_queue command_queue;
    uint32_t num_mem_objects;
    const cl_mem* mem_objects;
    ULONGLONG flags;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueNDRangeKernel_params
{
    cl_command_queue command_queue;
    cl_kernel kernel;
    uint32_t work_dim;
    const size_t* global_work_offset;
    const size_t* global_work_size;
    const size_t* local_work_size;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueNativeKernel_params
{
    cl_command_queue command_queue;
    void (WINAPI* user_func)(void*);
    void* args;
    SIZE_T cb_args;
    uint32_t num_mem_objects;
    const cl_mem* mem_list;
    const void** args_mem_loc;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    uint32_t blocking_read;
    SIZE_T offset;
    SIZE_T size;
    void* ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadBufferRect_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    uint32_t blocking_read;
    const size_t* buffer_origin;
    const size_t* host_origin;
    const size_t* region;
    SIZE_T buffer_row_pitch;
    SIZE_T buffer_slice_pitch;
    SIZE_T host_row_pitch;
    SIZE_T host_slice_pitch;
    void* ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueReadImage_params
{
    cl_command_queue command_queue;
    cl_mem image;
    uint32_t blocking_read;
    const size_t* origin;
    const size_t* region;
    SIZE_T row_pitch;
    SIZE_T slice_pitch;
    void* ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueTask_params
{
    cl_command_queue command_queue;
    cl_kernel kernel;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueUnmapMemObject_params
{
    cl_command_queue command_queue;
    cl_mem memobj;
    void* mapped_ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWaitForEvents_params
{
    cl_command_queue command_queue;
    uint32_t num_events;
    const cl_event* event_list;
};

struct clEnqueueWriteBuffer_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    uint32_t blocking_write;
    SIZE_T offset;
    SIZE_T size;
    const void* ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWriteBufferRect_params
{
    cl_command_queue command_queue;
    cl_mem buffer;
    uint32_t blocking_write;
    const size_t* buffer_origin;
    const size_t* host_origin;
    const size_t* region;
    SIZE_T buffer_row_pitch;
    SIZE_T buffer_slice_pitch;
    SIZE_T host_row_pitch;
    SIZE_T host_slice_pitch;
    const void* ptr;
    uint32_t num_events_in_wait_list;
    const cl_event* event_wait_list;
    cl_event* event;
};

struct clEnqueueWriteImage_params
{
    cl_command_queue command_queue;
    cl_mem image;
    uint32_t blocking_write;
    const size_t* origin;
    const size_t* region;
    SIZE_T input_row_pitch;
    SIZE_T input_slice_pitch;
    const void* ptr;
    uint32_t num_events_in_wait_list;
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
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetContextInfo_params
{
    cl_context context;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetDeviceIDs_params
{
    cl_platform_id platform;
    ULONGLONG device_type;
    uint32_t num_entries;
    cl_device_id* devices;
    cl_uint* num_devices;
};

struct clGetDeviceInfo_params
{
    cl_device_id device;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetEventInfo_params
{
    cl_event event;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetEventProfilingInfo_params
{
    cl_event event;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetImageInfo_params
{
    cl_mem image;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelArgInfo_params
{
    cl_kernel kernel;
    uint32_t arg_index;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelInfo_params
{
    cl_kernel kernel;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetKernelWorkGroupInfo_params
{
    cl_kernel kernel;
    cl_device_id device;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetMemObjectInfo_params
{
    cl_mem memobj;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetPlatformIDs_params
{
    uint32_t num_entries;
    cl_platform_id* platforms;
    cl_uint* num_platforms;
};

struct clGetPlatformInfo_params
{
    cl_platform_id platform;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetProgramBuildInfo_params
{
    cl_program program;
    cl_device_id device;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetProgramInfo_params
{
    cl_program program;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetSamplerInfo_params
{
    cl_sampler sampler;
    uint32_t param_name;
    SIZE_T param_value_size;
    void* param_value;
    size_t* param_value_size_ret;
};

struct clGetSupportedImageFormats_params
{
    cl_context context;
    ULONGLONG flags;
    uint32_t image_type;
    uint32_t num_entries;
    cl_image_format* image_formats;
    cl_uint* num_image_formats;
};

struct clLinkProgram_params
{
    cl_program* __retval;
    cl_context context;
    uint32_t num_devices;
    const cl_device_id* device_list;
    const char* options;
    uint32_t num_input_programs;
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
    int32_t command_exec_callback_type;
    void (WINAPI* pfn_notify)(cl_event event, cl_int event_command_status, void *user_data);
    void* user_data;
};

struct clSetKernelArg_params
{
    cl_kernel kernel;
    uint32_t arg_index;
    SIZE_T arg_size;
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
    int32_t execution_status;
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
    uint32_t num_events;
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
