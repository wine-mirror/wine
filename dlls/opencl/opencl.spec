# OpenCL 1.0
@ stdcall clGetPlatformIDs( long ptr ptr ) wine_clGetPlatformIDs
@ stdcall clGetPlatformInfo( long long long ptr ptr ) wine_clGetPlatformInfo

@ stdcall clGetDeviceIDs( long long long ptr ptr ) wine_clGetDeviceIDs
@ stdcall clGetDeviceInfo( long long long ptr ptr ) wine_clGetDeviceInfo

@ stdcall clCreateContext(ptr long ptr ptr ptr ptr) wine_clCreateContext
@ stdcall clCreateContextFromType(ptr long ptr ptr ptr) wine_clCreateContextFromType
@ stdcall clRetainContext( long ) wine_clRetainContext
@ stdcall clReleaseContext( long ) wine_clReleaseContext
@ stdcall clGetContextInfo( long long long ptr ptr ) wine_clGetContextInfo

@ stdcall clCreateCommandQueue( long long long ptr ) wine_clCreateCommandQueue
@ stdcall clRetainCommandQueue( long ) wine_clRetainCommandQueue
@ stdcall clReleaseCommandQueue( long ) wine_clReleaseCommandQueue
@ stdcall clGetCommandQueueInfo( long long long ptr ptr ) wine_clGetCommandQueueInfo
@ stdcall clSetCommandQueueProperty( long long long ptr ) wine_clSetCommandQueueProperty

@ stdcall clCreateBuffer( long long long ptr ptr ) wine_clCreateBuffer
@ stdcall clCreateImage2D( long long ptr long long long ptr ptr ) wine_clCreateImage2D
@ stdcall clCreateImage3D( long long ptr long long long long long ptr ptr ) wine_clCreateImage3D
@ stdcall clRetainMemObject( long ) wine_clRetainMemObject
@ stdcall clReleaseMemObject( long ) wine_clReleaseMemObject
@ stdcall clGetSupportedImageFormats( long long long long ptr ptr ) wine_clGetSupportedImageFormats
@ stdcall clGetMemObjectInfo( long long long ptr ptr ) wine_clGetMemObjectInfo
@ stdcall clGetImageInfo( long long long ptr ptr ) wine_clGetImageInfo

@ stdcall clCreateSampler( long long long long ptr ) wine_clCreateSampler
@ stdcall clRetainSampler( long ) wine_clRetainSampler
@ stdcall clReleaseSampler( long ) wine_clReleaseSampler
@ stdcall clGetSamplerInfo( long long long ptr ptr ) wine_clGetSamplerInfo

@ stdcall clCreateProgramWithSource( long long ptr ptr ptr ) wine_clCreateProgramWithSource
@ stdcall clCreateProgramWithBinary( long long ptr ptr ptr ptr ptr ) wine_clCreateProgramWithBinary
@ stdcall clRetainProgram( long ) wine_clRetainProgram
@ stdcall clReleaseProgram( long ) wine_clReleaseProgram
@ stdcall clBuildProgram( long long ptr str ptr ptr ) wine_clBuildProgram
@ stdcall clUnloadCompiler() wine_clUnloadCompiler
@ stdcall clGetProgramInfo( long long long ptr ptr ) wine_clGetProgramInfo
@ stdcall clGetProgramBuildInfo( long long long long ptr ptr ) wine_clGetProgramBuildInfo

@ stdcall clCreateKernel( long str ptr ) wine_clCreateKernel
@ stdcall clCreateKernelsInProgram( long long ptr ptr ) wine_clCreateKernelsInProgram
@ stdcall clRetainKernel( long ) wine_clRetainKernel
@ stdcall clReleaseKernel( long ) wine_clReleaseKernel
@ stdcall clSetKernelArg( long long long ptr ) wine_clSetKernelArg
@ stdcall clGetKernelInfo( long long long ptr ptr ) wine_clGetKernelInfo
@ stdcall clGetKernelWorkGroupInfo( long long long long ptr ptr ) wine_clGetKernelWorkGroupInfo

@ stdcall clWaitForEvents( long ptr ) wine_clWaitForEvents
@ stdcall clGetEventInfo( long long long ptr ptr ) wine_clGetEventInfo
@ stdcall clReleaseEvent( long ) wine_clReleaseEvent
@ stdcall clRetainEvent( long ) wine_clRetainEvent

@ stdcall clGetEventProfilingInfo( long long long ptr ptr ) wine_clGetEventProfilingInfo

@ stdcall clFlush( long ) wine_clFlush
@ stdcall clFinish( long ) wine_clFinish

@ stdcall clEnqueueReadBuffer( long long long long long ptr long ptr ptr ) wine_clEnqueueReadBuffer
@ stdcall clEnqueueWriteBuffer( long long long long long ptr long ptr ptr ) wine_clEnqueueWriteBuffer
@ stdcall clEnqueueCopyBuffer( long long long long long long long ptr ptr ) wine_clEnqueueCopyBuffer
@ stdcall clEnqueueReadImage( long long long ptr ptr long long ptr long ptr ptr ) wine_clEnqueueReadImage
@ stdcall clEnqueueWriteImage( long long long ptr ptr long long ptr long ptr ptr ) wine_clEnqueueWriteImage
@ stdcall clEnqueueCopyImage( long long long ptr ptr ptr long ptr ptr ) wine_clEnqueueCopyImage
@ stdcall clEnqueueCopyImageToBuffer( long long long ptr ptr long long ptr ptr ) wine_clEnqueueCopyImageToBuffer
@ stdcall clEnqueueCopyBufferToImage( long long long long ptr ptr long ptr ptr ) wine_clEnqueueCopyBufferToImage
@ stdcall clEnqueueMapBuffer( long long long long long long long ptr ptr ptr ) wine_clEnqueueMapBuffer
@ stdcall clEnqueueMapImage( long long long long ptr ptr ptr ptr long ptr ptr ptr ) wine_clEnqueueMapImage
@ stdcall clEnqueueUnmapMemObject( long long ptr long ptr ptr ) wine_clEnqueueUnmapMemObject
@ stdcall clEnqueueNDRangeKernel( long long long ptr ptr ptr long ptr ptr ) wine_clEnqueueNDRangeKernel
@ stdcall clEnqueueTask( long long long ptr ptr ) wine_clEnqueueTask
@ stdcall clEnqueueNativeKernel(long ptr ptr long long ptr ptr long ptr ptr) wine_clEnqueueNativeKernel
@ stdcall clEnqueueMarker( long ptr ) wine_clEnqueueMarker
@ stdcall clEnqueueWaitForEvents( long long ptr ) wine_clEnqueueWaitForEvents
@ stdcall clEnqueueBarrier( long ) wine_clEnqueueBarrier

@ stdcall clGetExtensionFunctionAddress( str ) wine_clGetExtensionFunctionAddress

@ stub clCreateFromGLBuffer
@ stub clCreateFromGLTexture2D
@ stub clCreateFromGLTexture3D
@ stub clCreateFromGLRenderbuffer
@ stub clGetGLObjectInfo
@ stub clGetGLTextureInfo
@ stub clEnqueueAcquireGLObjects
@ stub clEnqueueReleaseGLObjects
# @ stdcall clCreateFromGLBuffer( long long long ptr ) wine_clCreateFromGLBuffer
# @ stdcall clCreateFromGLTexture2D( long long long long long ptr ) wine_clCreateFromGLTexture2D
# @ stdcall clCreateFromGLTexture3D( long long long long long ptr ) wine_clCreateFromGLTexture3D
# @ stdcall clCreateFromGLRenderbuffer( long long long ptr ) wine_clCreateFromGLRenderbuffer
# @ stdcall clGetGLObjectInfo( long ptr ptr ) wine_clGetGLObjectInfo
# @ stdcall clGetGLTextureInfo( long long long ptr ptr ) wine_clGetGLTextureInfo
# @ stdcall clEnqueueAcquireGLObjects( long long ptr long ptr ptr ) wine_clEnqueueAcquireGLObjects
# @ stdcall clEnqueueReleaseGLObjects( long long ptr long ptr ptr ) wine_clEnqueueReleaseGLObjects
