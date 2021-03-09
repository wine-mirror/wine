@ stdcall clBuildProgram(ptr long ptr ptr ptr ptr) wine_clBuildProgram
@ stdcall clCreateBuffer(ptr int64 long ptr ptr) wine_clCreateBuffer
@ stdcall clCreateCommandQueue(ptr ptr int64 ptr) wine_clCreateCommandQueue
@ stdcall clCreateContext(ptr long ptr ptr ptr ptr) wine_clCreateContext
@ stdcall clCreateContextFromType(ptr int64 ptr ptr ptr) wine_clCreateContextFromType
@ stdcall clCreateImage2D(ptr int64 ptr long long long ptr ptr) wine_clCreateImage2D
@ stdcall clCreateImage3D(ptr int64 ptr long long long long long ptr ptr) wine_clCreateImage3D
@ stdcall clCreateKernel(ptr ptr ptr) wine_clCreateKernel
@ stdcall clCreateKernelsInProgram(ptr long ptr ptr) wine_clCreateKernelsInProgram
@ stdcall clCreateProgramWithBinary(ptr long ptr ptr ptr ptr ptr) wine_clCreateProgramWithBinary
@ stdcall clCreateProgramWithSource(ptr long ptr ptr ptr) wine_clCreateProgramWithSource
@ stdcall clCreateSampler(ptr long long long ptr) wine_clCreateSampler
@ stdcall clEnqueueBarrier(ptr) wine_clEnqueueBarrier
@ stdcall clEnqueueCopyBuffer(ptr ptr ptr long long long long ptr ptr) wine_clEnqueueCopyBuffer
@ stdcall clEnqueueCopyBufferToImage(ptr ptr ptr long ptr ptr long ptr ptr) wine_clEnqueueCopyBufferToImage
@ stdcall clEnqueueCopyImage(ptr ptr ptr ptr ptr ptr long ptr ptr) wine_clEnqueueCopyImage
@ stdcall clEnqueueCopyImageToBuffer(ptr ptr ptr ptr ptr long long ptr ptr) wine_clEnqueueCopyImageToBuffer
@ stdcall clEnqueueMapBuffer(ptr ptr long int64 long long long ptr ptr ptr) wine_clEnqueueMapBuffer
@ stdcall clEnqueueMapImage(ptr ptr long int64 ptr ptr ptr ptr long ptr ptr ptr) wine_clEnqueueMapImage
@ stdcall clEnqueueMarker(ptr ptr) wine_clEnqueueMarker
@ stdcall clEnqueueNDRangeKernel(ptr ptr long ptr ptr ptr long ptr ptr) wine_clEnqueueNDRangeKernel
@ stdcall clEnqueueNativeKernel(ptr ptr ptr long long ptr ptr long ptr ptr) wine_clEnqueueNativeKernel
@ stdcall clEnqueueReadBuffer(ptr ptr long long long ptr long ptr ptr) wine_clEnqueueReadBuffer
@ stdcall clEnqueueReadImage(ptr ptr long ptr ptr long long ptr long ptr ptr) wine_clEnqueueReadImage
@ stdcall clEnqueueTask(ptr ptr long ptr ptr) wine_clEnqueueTask
@ stdcall clEnqueueUnmapMemObject(ptr ptr ptr long ptr ptr) wine_clEnqueueUnmapMemObject
@ stdcall clEnqueueWaitForEvents(ptr long ptr) wine_clEnqueueWaitForEvents
@ stdcall clEnqueueWriteBuffer(ptr ptr long long long ptr long ptr ptr) wine_clEnqueueWriteBuffer
@ stdcall clEnqueueWriteImage(ptr ptr long ptr ptr long long ptr long ptr ptr) wine_clEnqueueWriteImage
@ stdcall clFinish(ptr) wine_clFinish
@ stdcall clFlush(ptr) wine_clFlush
@ stdcall clGetCommandQueueInfo(ptr long long ptr ptr) wine_clGetCommandQueueInfo
@ stdcall clGetContextInfo(ptr long long ptr ptr) wine_clGetContextInfo
@ stdcall clGetDeviceIDs(ptr int64 long ptr ptr) wine_clGetDeviceIDs
@ stdcall clGetDeviceInfo(ptr long long ptr ptr) wine_clGetDeviceInfo
@ stdcall clGetEventInfo(ptr long long ptr ptr) wine_clGetEventInfo
@ stdcall clGetEventProfilingInfo(ptr long long ptr ptr) wine_clGetEventProfilingInfo
@ stdcall clGetExtensionFunctionAddress(ptr) wine_clGetExtensionFunctionAddress
@ stdcall clGetImageInfo(ptr long long ptr ptr) wine_clGetImageInfo
@ stdcall clGetKernelInfo(ptr long long ptr ptr) wine_clGetKernelInfo
@ stdcall clGetKernelWorkGroupInfo(ptr ptr long long ptr ptr) wine_clGetKernelWorkGroupInfo
@ stdcall clGetMemObjectInfo(ptr long long ptr ptr) wine_clGetMemObjectInfo
@ stdcall clGetPlatformIDs(long ptr ptr) wine_clGetPlatformIDs
@ stdcall clGetPlatformInfo(ptr long long ptr ptr) wine_clGetPlatformInfo
@ stdcall clGetProgramBuildInfo(ptr ptr long long ptr ptr) wine_clGetProgramBuildInfo
@ stdcall clGetProgramInfo(ptr long long ptr ptr) wine_clGetProgramInfo
@ stdcall clGetSamplerInfo(ptr long long ptr ptr) wine_clGetSamplerInfo
@ stdcall clGetSupportedImageFormats(ptr int64 long long ptr ptr) wine_clGetSupportedImageFormats
@ stdcall clReleaseCommandQueue(ptr) wine_clReleaseCommandQueue
@ stdcall clReleaseContext(ptr) wine_clReleaseContext
@ stdcall clReleaseEvent(ptr) wine_clReleaseEvent
@ stdcall clReleaseKernel(ptr) wine_clReleaseKernel
@ stdcall clReleaseMemObject(ptr) wine_clReleaseMemObject
@ stdcall clReleaseProgram(ptr) wine_clReleaseProgram
@ stdcall clReleaseSampler(ptr) wine_clReleaseSampler
@ stdcall clRetainCommandQueue(ptr) wine_clRetainCommandQueue
@ stdcall clRetainContext(ptr) wine_clRetainContext
@ stdcall clRetainEvent(ptr) wine_clRetainEvent
@ stdcall clRetainKernel(ptr) wine_clRetainKernel
@ stdcall clRetainMemObject(ptr) wine_clRetainMemObject
@ stdcall clRetainProgram(ptr) wine_clRetainProgram
@ stdcall clRetainSampler(ptr) wine_clRetainSampler
@ stdcall clSetCommandQueueProperty(ptr int64 long ptr) wine_clSetCommandQueueProperty
@ stdcall clSetKernelArg(ptr long long ptr) wine_clSetKernelArg
@ stdcall clUnloadCompiler() wine_clUnloadCompiler
@ stdcall clWaitForEvents(long ptr) wine_clWaitForEvents
